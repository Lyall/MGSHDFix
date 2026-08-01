#include "stdafx.h"
#include "mgs2_enhanced_demos.hpp"

#include "common.hpp"
#include "logging.hpp"

#include "game_funcs.hpp"
#include "gamevars.hpp"

#include <unordered_map>
#include <vector>

// Cutscene mod support: any effect chara id works in any stage, and the effect prim
// pool is bigger for dense scenes. Stock demos are unaffected.
namespace
{
    SafetyHookInline gGetCharaIDHook {};

    std::unordered_map<uint32_t, void*> gUnion;
    bool gUnionBuilt = false;

    // Stage chara tables: sorted static CHARA arrays (game_funcs.hpp)

    void BuildUnion()
    {
        gUnionBuilt = true;

        const auto base = reinterpret_cast<uintptr_t>(baseModule);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);

        uintptr_t textLo = 0, textHi = 0;
        std::vector<std::pair<uintptr_t, uintptr_t>> dataSecs;
        const auto* sec = IMAGE_FIRST_SECTION(nt);
        for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
        {
            const uintptr_t lo = base + sec->VirtualAddress;
            const uintptr_t hi = lo + sec->Misc.VirtualSize;
            if (sec->Characteristics & IMAGE_SCN_MEM_EXECUTE)
            {
                if (textLo == 0) { textLo = lo; textHi = hi; }
                else { textHi = hi > textHi ? hi : textHi; }
            }
            else if ((sec->Characteristics & IMAGE_SCN_MEM_READ)
                  && (sec->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA))
            {
                dataSecs.emplace_back(lo, hi);
            }
        }

        const auto valid = [&](const CHARA* r)
        {
            // the alignment padding after class_id is always zero in the static tables
            return r->class_id > 0 && r->class_id < 0x02000000
                && reinterpret_cast<const uint32_t*>(r)[1] == 0
                && reinterpret_cast<uintptr_t>(r->new_) >= textLo
                && reinterpret_cast<uintptr_t>(r->new_) < textHi;
        };

        size_t tables = 0;
        for (const auto& [lo, hi] : dataSecs)
        {
            const auto* end = reinterpret_cast<const uint8_t*>(hi) - sizeof(CHARA) * 2;
            for (const auto* q = reinterpret_cast<const uint8_t*>(lo); q <= end; q += 8)
            {
                const auto* r = reinterpret_cast<const CHARA*>(q);
                if (!valid(r) || !valid(r + 1) || (r + 1)->class_id <= r->class_id)
                {
                    continue;
                }
                size_t n = 1;
                const CHARA* rr = r;
                while (reinterpret_cast<const uint8_t*>(rr + 2) <= reinterpret_cast<const uint8_t*>(hi)
                    && valid(rr + 1) && (rr + 1)->class_id > rr->class_id)
                {
                    rr++;
                    n++;
                }
                if (n >= 12)
                {
                    for (const CHARA* k = r; k <= rr; k++)
                    {
                        void* fn = reinterpret_cast<void*>(k->new_);
                        const auto [at, added] = gUnion.emplace(k->class_id, fn);
                        if (!added && at->second != fn)
                        {
                            // the union is only sound while an id means one function
                            spdlog::warn("MGS 2: Enhanced Demos: chara {:08x} has two constructors ({} and {}), keeping the first.",
                                k->class_id, at->second, fn);
                        }
                    }
                    tables++;
                }
                q = reinterpret_cast<const uint8_t*>(rr + 1) - 8;
            }
        }
        spdlog::info("MGS 2: Enhanced Demos: harvested {} chara tables, {} unique ids.", tables, gUnion.size());
    }

    void* GetCharaID_Detour(int nID)
    {
        // The stock lookup asserts on a miss, so check the union first. An id maps
        // to the same function in every stage table that carries it.
        if (!gUnionBuilt)
        {
            BuildUnion();
        }
        const auto it = gUnion.find(static_cast<uint32_t>(nID));
        if (it != gUnion.end())
        {
            return it->second;
        }
        return gGetCharaIDHook.call<void*>(nID);
    }

    void PatchPrim2Pool()
    {
        // The prim2 pool fills up in dense scenes and effects stop drawing. Raise it to
        // 2048; the buffer advance after it has to match.
        uint8_t* anchor = g_GameVars.DG_InitChanlSystem_ObjQueueInit();
        if (anchor == nullptr)
        {
            return;
        }
        // prim2 is the last of the four 512-slot buffers set up after this one. Bail if
        // there aren't exactly four, the layout has changed.
        uint8_t* store = nullptr;
        int stores = 0;
        for (uint8_t* p = anchor; p < anchor + 0x100; p++)
        {
            if (p[0] == 0x48 && p[1] == 0xC7 && p[2] == 0x05
             && p[7] == 0x00 && p[8] == 0x02 && p[9] == 0x00 && p[10] == 0x00)
            {
                if (++stores == 4)
                {
                    store = p;
                }
            }
        }
        uint8_t* lea = nullptr;
        for (uint8_t* p = anchor; p < anchor + 0x100; p++)
        {
            if (p[0] == 0x8D && p[1] == 0x81
             && p[2] == 0x00 && p[3] == 0x02 && p[4] == 0x00 && p[5] == 0x00)
            {
                lea = p;
                break;
            }
        }
        if (store == nullptr || lea == nullptr || stores != 4)
        {
            spdlog::error("MGS 2: Enhanced Demos: DG_InitChanlSystem() carves {} buffers of 512, expected 4.", stores);
            return;
        }
        Memory::Write<int>(reinterpret_cast<uintptr_t>(store + 7), 2048);
        Memory::Write<int>(reinterpret_cast<uintptr_t>(lea + 2), 2048);
        spdlog::info("MGS 2: Enhanced Demos: DG_ObjQueue prim2 pool 512 -> 2048.");
    }
}

void MGS2EnhancedDemos::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    spdlog::info("MGS 2: Enhanced Demos - Initializing...");

    uint8_t* site = Memory::PatternScan(baseModule,
        "E8 ?? ?? ?? ?? 48 8B F0 48 85 C0 75 ?? 8D 46 ?? 48 83 C4",
        "MGS 2: Enhanced Demos: NewChara() -> GM_GetCharaID");
    if (site == nullptr)
    {
        return;
    }
    gGetCharaIDHook = safetyhook::create_inline(Memory::ResolveCall(site), GetCharaID_Detour);
    LOG_HOOK(gGetCharaIDHook, "MGS 2: Enhanced Demos - GM_GetCharaID");

    PatchPrim2Pool();
}
