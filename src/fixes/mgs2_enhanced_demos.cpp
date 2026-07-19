#include "stdafx.h"
#include "mgs2_enhanced_demos.hpp"

#include "common.hpp"
#include "logging.hpp"

#include <unordered_map>
#include <vector>

// Support for enhanced/restored demos (cutscene mods): lets a demo use any effect
// chara the build contains, not just the ones its stage happened to link, and gives
// the effect prim registry enough headroom for denser scenes. Inert on stock demos.
namespace
{
    SafetyHookInline gGetCharaIDHook {};

    std::unordered_map<uint32_t, void*> gUnion;
    bool gUnionBuilt = false;

    // Stage chara tables are sorted static arrays of { int id; pad; NEWCHARA *fn; }.
    // Harvest every table from the data sections into one union.
    struct Rec
    {
        uint32_t id;
        uint32_t pad;
        void* fn;
    };

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

        const auto valid = [&](const Rec* r)
        {
            return r->id > 0 && r->id < 0x02000000 && r->pad == 0
                && reinterpret_cast<uintptr_t>(r->fn) >= textLo
                && reinterpret_cast<uintptr_t>(r->fn) < textHi;
        };

        size_t tables = 0;
        for (const auto& [lo, hi] : dataSecs)
        {
            const auto* end = reinterpret_cast<const uint8_t*>(hi) - sizeof(Rec) * 2;
            for (const auto* q = reinterpret_cast<const uint8_t*>(lo); q <= end; q += 8)
            {
                const auto* r = reinterpret_cast<const Rec*>(q);
                if (!valid(r) || !valid(r + 1) || (r + 1)->id <= r->id)
                {
                    continue;
                }
                size_t n = 1;
                const Rec* rr = r;
                while (reinterpret_cast<const uint8_t*>(rr + 2) <= reinterpret_cast<const uint8_t*>(hi)
                    && valid(rr + 1) && (rr + 1)->id > rr->id)
                {
                    rr++;
                    n++;
                }
                if (n >= 12)
                {
                    for (const Rec* k = r; k <= rr; k++)
                    {
                        gUnion.emplace(k->id, k->fn);
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
        // The shipped lookup asserts on a miss, so consult the union first. Safe:
        // launcher glue is shared code -- an id maps to the same function in every
        // stage table that carries it.
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
        // The DG prim2 registry holds every live effect prim (512 slots); dense
        // modded scenes saturate it and effects silently stop rendering. Raise to
        // 2048; the watermark advance after the prim2 carve must grow identically.
        uint8_t* anchor = Memory::PatternScan(baseModule, "48 C7 05 ?? ?? ?? ?? 00 03 00 00",
            "MGS 2: Enhanced Demos -> DG object queue init");
        if (anchor == nullptr)
        {
            return;
        }
        uint8_t* store = nullptr;
        int seen = 0;
        for (uint8_t* p = anchor; p < anchor + 0x100; p++)
        {
            if (p[0] == 0x48 && p[1] == 0xC7 && p[2] == 0x05
             && p[7] == 0x00 && p[8] == 0x02 && p[9] == 0x00 && p[10] == 0x00)
            {
                if (++seen == 4) { store = p; break; }
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
        if (store == nullptr || lea == nullptr)
        {
            spdlog::error("MGS 2: Enhanced Demos: prim2 pool patch sites not found.");
            return;
        }
        Memory::Write<int>(reinterpret_cast<uintptr_t>(store + 7), 2048);
        Memory::Write<int>(reinterpret_cast<uintptr_t>(lea + 2), 2048);
        spdlog::info("MGS 2: Enhanced Demos: prim2 pool 512 -> 2048.");
    }
}

void MGS2EnhancedDemos::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    spdlog::info("MGS 2: Enhanced Demos - Initializing...");

    uint8_t* site = Memory::PatternScan(baseModule,
        "E8 ?? ?? ?? ?? 48 8B F0 48 85 C0 75 ?? 8D 46 ?? 48 83 C4",
        "MGS 2: Enhanced Demos -> GM_GetCharaID call site");
    if (site == nullptr)
    {
        return;
    }
    gGetCharaIDHook = safetyhook::create_inline(Memory::ResolveCall(site), GetCharaID_Detour);
    LOG_HOOK(gGetCharaIDHook, "MGS 2: Enhanced Demos - GM_GetCharaID");

    PatchPrim2Pool();
}
