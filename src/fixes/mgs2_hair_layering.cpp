#include "stdafx.h"
#include "mgs2_hair_layering.hpp"

#include "common.hpp"
#include "logging.hpp"

namespace
{
    // Layered-hair unit type; packs at [unit+0x118] stride 0x38, texture record at pack+0x20.
    constexpr uint16_t kHairUnitType = 0x4801;

    SafetyHookInline EmitRooms_hook {};
    SafetyHookInline EmitChars_hook {};
    SafetyHookMid PackBiasRooms_hook {};
    SafetyHookMid PackBiasChars_hook {};
    SafetyHookMid RoomSegEnd_hook {};

    // globals the character emitter reads; the flush must refill them per unit
    uint32_t* pCharEmitTri = nullptr;
    uint64_t* pCharEmitMtx = nullptr;

    uint8_t* gStash[8] {};
    int gStashN = 0;
    int gPackBias = 0;

    struct Group { uint16_t start; uint16_t count; uintptr_t tex; };

    int PartitionGroups(uint8_t* packs, uint16_t nPacks, Group* groups)
    {
        int ng = 0;
        for (int i = 0; i < nPacks; i++)
        {
            const uintptr_t tex = *reinterpret_cast<const uintptr_t*>(packs + i * 0x38 + 0x20);
            if (ng > 0 && groups[ng - 1].tex == tex)
            {
                groups[ng - 1].count++;
            }
            else if (ng < 16)
            {
                groups[ng] = { static_cast<uint16_t>(i), 1, tex };
                ng++;
            }
            else
            {
                return 0; // too many groups: leave stock
            }
        }
        return ng;
    }

    // Emit texture groups back-to-front; gPackBias keeps packet pack-ids absolute.
    void EmitGroupsReversed(SafetyHookInline& orig, uint8_t* unit, uint8_t* packs, uint16_t nPacks,
        const Group* groups, int ng)
    {
        for (int g = ng - 1; g >= 0; g--)
        {
            *reinterpret_cast<uint8_t**>(unit + 0x118) = packs + groups[g].start * 0x38;
            *reinterpret_cast<uint16_t*>(unit + 0xCC) = groups[g].count;
            gPackBias = groups[g].start;
            orig.call<void, uint8_t*>(unit);
        }
        gPackBias = 0;
        *reinterpret_cast<uint8_t**>(unit + 0x118) = packs;
        *reinterpret_cast<uint16_t*>(unit + 0xCC) = nPacks;
    }

    void EmitUnit(SafetyHookInline& orig, uint8_t* unit, bool charSide)
    {
        if (*reinterpret_cast<const uint16_t*>(unit + 0xCE) != kHairUnitType)
        {
            orig.call<void, uint8_t*>(unit);
            return;
        }

        const uint16_t nPacks = *reinterpret_cast<const uint16_t*>(unit + 0xCC);
        uint8_t* packs = *reinterpret_cast<uint8_t**>(unit + 0x118);
        Group groups[16];
        int ng = 0;
        if (packs && nPacks > 0 && nPacks <= 64)
        {
            ng = PartitionGroups(packs, nPacks, groups);
        }
        if (ng < 2)
        {
            orig.call<void, uint8_t*>(unit);
            return;
        }

        // Defer: characters build before rooms, so hair fringes would blend against the fog
        // fill. Stash and flush after the room segment; z-test handles occlusion.
        if (charSide && RoomSegEnd_hook)
        {
            for (int i = 0; i < gStashN; i++)
            {
                if (gStash[i] == unit) // stale (flush never ran): draw in place
                {
                    gStash[i] = gStash[--gStashN];
                    EmitGroupsReversed(orig, unit, packs, nPacks, groups, ng);
                    return;
                }
            }
            if (gStashN < 8)
            {
                gStash[gStashN++] = unit;
                return;
            }
        }

        EmitGroupsReversed(orig, unit, packs, nPacks, groups, ng);
    }

    void __fastcall EmitRooms_Hook(uint8_t* unit) { EmitUnit(EmitRooms_hook, unit, false); }
    void __fastcall EmitChars_Hook(uint8_t* unit) { EmitUnit(EmitChars_hook, unit, true); }

    void PackBias_Hook(SafetyHookContext& ctx)
    {
        if (gPackBias != 0)
        {
            *reinterpret_cast<uint32_t*>(ctx.rbx + 0x1B0) += gPackBias;
        }
    }

    void RoomSegEnd_Hook(SafetyHookContext&)
    {
        for (int i = 0; i < gStashN; i++)
        {
            uint8_t* unit = gStash[i];
            const uint16_t nPacks = *reinterpret_cast<const uint16_t*>(unit + 0xCC);
            uint8_t* packs = *reinterpret_cast<uint8_t**>(unit + 0x118);
            if (!packs || nPacks == 0)
            {
                continue;
            }
            *pCharEmitTri = *reinterpret_cast<const uint32_t*>(unit + 0x120);
            *pCharEmitMtx = *reinterpret_cast<const uint64_t*>(unit + 0x128);

            Group groups[16];
            const int ng = PartitionGroups(packs, nPacks, groups);
            if (ng > 0)
            {
                EmitGroupsReversed(EmitChars_hook, unit, packs, nPacks, groups, ng);
            }
        }
        gStashN = 0;
    }
}

void MGS2HairLayering::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    // each walk copies unit+0x120/+0x128 to globals then calls its emitter; two hits: rooms, characters
    auto walks = Memory::FindMultiplePatternMatches(baseModule,
        "8B 83 20 01 00 00 48 8B CB 89 05 ?? ?? ?? ?? 48 8B 83 28 01 00 00 48 89 05 ?? ?? ?? ?? E8");
    if (walks.size() != 2)
    {
        spdlog::error("MGS 2: Hair Layering : expected 2 chain2 walks, found {}.", walks.size());
        return;
    }

    auto emitterOf = [](uint8_t* walk) {
        return walk + 34 + *reinterpret_cast<int32_t*>(walk + 30);
    };
    pCharEmitTri = reinterpret_cast<uint32_t*>(walks[1] + 15 + *reinterpret_cast<int32_t*>(walks[1] + 11));
    pCharEmitMtx = reinterpret_cast<uint64_t*>(walks[1] + 29 + *reinterpret_cast<int32_t*>(walks[1] + 25));

    EmitRooms_hook = safetyhook::create_inline(emitterOf(walks[0]), EmitRooms_Hook);
    LOG_HOOK(EmitRooms_hook, "MGS 2: Hair Layering : chain2 unit emitter (rooms)")
    EmitChars_hook = safetyhook::create_inline(emitterOf(walks[1]), EmitChars_Hook);
    LOG_HOOK(EmitChars_hook, "MGS 2: Hair Layering : chain2 unit emitter (characters)")

    // pack-index stores in each emitter - bias them back to absolute
    if (uint8_t* bias = Memory::PatternScan(baseModule,
        "89 AB B0 01 00 00 48 C7 83 B4 01 00 00 01",
        "MGS 2: Hair Layering : chain2 emitter (rooms) -> pack index store"))
    {
        PackBiasRooms_hook = safetyhook::create_mid(bias + 6, PackBias_Hook);
        LOG_HOOK(PackBiasRooms_hook, "MGS 2: Hair Layering : pack-index bias (rooms)")
    }
    if (uint8_t* bias = Memory::PatternScan(baseModule,
        "44 89 A3 B0 01 00 00 48 89 44 24 78",
        "MGS 2: Hair Layering : chain2 emitter (characters) -> pack index store"))
    {
        PackBiasChars_hook = safetyhook::create_mid(bias + 7, PackBias_Hook);
        LOG_HOOK(PackBiasChars_hook, "MGS 2: Hair Layering : pack-index bias (characters)")
    }
    if (!PackBiasRooms_hook || !PackBiasChars_hook)
    {
        spdlog::error("MGS 2: Hair Layering : pack-index bias hooks failed; disabling.");
        EmitRooms_hook = {};
        EmitChars_hook = {};
        return;
    }

    // room-segment end = the defer flush slot
    if (uint8_t* seg = Memory::PatternScan(baseModule,
        "85 ED 74 ?? 33 D2 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 44 8B C0 48 8D 15 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 1D",
        "MGS 2: Hair Layering : libdg\\chain2.c -> room segment end"))
    {
        RoomSegEnd_hook = safetyhook::create_mid(seg + 0x28, RoomSegEnd_Hook);
        LOG_HOOK(RoomSegEnd_hook, "MGS 2: Hair Layering : hair defer flush")
    }
    // without the flush slot the reorder still applies, hair just draws in place
}
