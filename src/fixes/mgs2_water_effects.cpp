#include "stdafx.h"
#include "common.hpp"
#include "mgs2_water_effects.hpp"

#include "logging.hpp"
#include "mgs2_autopacket.hpp"
#include "gamevars.hpp"

namespace
{
    using namespace MGS2Autopacket;

    constexpr ptrdiff_t kWork_Mverts  = 0x978;
    constexpr ptrdiff_t kWork_Dmapack = 0x988;

    constexpr ptrdiff_t kVertStride   = 0x30;
    constexpr ptrdiff_t kVertHeader   = 0x10;
    constexpr int       kPosOffX      = 1792;
    constexpr int       kPosOffY      = 1824;

    constexpr int       kStrips        = 11;
    constexpr int       kVertsPerStrip = 24;

    constexpr uint64_t kMdlTest  = 0x30000ULL;
    constexpr uint64_t kMdlAlpha = 0x800000002aULL;
    constexpr uint64_t kMdlPrim  = 0x11cULL;

    SafetyHookInline g_mgrSpawnHook{};

    SafetyHookInline g_actHook{};
    InitMdlDrawFn    g_initMdlDraw = nullptr;

    alignas(16) uint8_t g_autopacket[0x4000];

    void BuildWarpAutopacket(uintptr_t work)
    {
        __try
        {
            const uintptr_t dmapack = Memory::ReadField<uintptr_t>(work, kWork_Dmapack);
            if (!dmapack) return;

            const int clock = g_GameVars.DG_Clock() & 1;
            const uintptr_t buffer = Memory::ReadField<uintptr_t>(work, kWork_Mverts + (ptrdiff_t)clock * 8);
            if (!buffer) return;

            const uint8_t* v = reinterpret_cast<const uint8_t*>(buffer + kVertHeader);
            uint8_t* const end = g_autopacket + sizeof(g_autopacket) - 0x40;
            uint8_t* p = g_autopacket;

            p = WriteTrBuffer(p);
            p = WriteDrawOffset(p);
            p = reinterpret_cast<uint8_t*>(g_initMdlDraw(p, kMdlTest, kMdlAlpha, kMdlPrim));

            for (int s = 0; s < kStrips; ++s)
            {
                if (p + 0x20 + (ptrdiff_t)kVertsPerStrip * kVertexSize > end) break;

                uint32_t* countField = nullptr;
                p = WriteStripHeader(p, &countField);

                for (int j = 0; j < kVertsPerStrip; ++j, v += kVertStride)
                {
                    const uint64_t rgbaq = *reinterpret_cast<const uint64_t*>(v + 0x00);
                    const uint64_t uv    = *reinterpret_cast<const uint64_t*>(v + 0x10);
                    const uint64_t xyz   = *reinterpret_cast<const uint64_t*>(v + 0x20);

                    const int sx = (int)(uint16_t)(xyz & 0xFFFF) >> 4;
                    const int sy = (int)(uint16_t)((xyz >> 16) & 0xFFFF) >> 4;
                    const int up = (int)(uint16_t)(uv & 0xFFFF) >> 4;
                    const int vp = (int)(uint16_t)((uv >> 16) & 0xFFFF) >> 4;
                    const uint32_t color = (uint32_t)(rgbaq & 0xFFFFFFFFu);

                    p = WriteVertex(p, (float)(sx - kPosOffX), (float)(sy - kPosOffY),
                                    (float)up / kDrawW, (float)vp / kDrawH, color);
                }
                *countField = (uint32_t)kVertsPerStrip;
            }
            WriteEnd(p);

            Commit(dmapack, g_autopacket);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void __fastcall Act_Detour(uintptr_t work)
    {
        g_actHook.fastcall<void>(work);
        if (g_initMdlDraw && work)
        {
            BuildWarpAutopacket(work);
        }
    }

    // On PS2 this splash manager can't allocate in d001p01 (effect memory full) so it never appears;
    // the MC's larger pool spawns it. Block the spawn in d001p01 to match PS2
    void* __fastcall MgrSpawn_Detour(int name, int map)
    {
        if (g_GameVars.IsStage(MGS2Stages::D001P01)) //afevis -> todo, check how many calls there are
        {
            return nullptr;   // PS2 hits EFFECT MEMORY LIMIT here; keep the manager null so the splash stays suppressed
        }
        return g_mgrSpawnHook.fastcall<void*>(name, map);
    }
}

void MGS2WaterEffects::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    spdlog::info("MGS 2 - Water Effects: Initializing...");
	
    if (uint8_t* address = Memory::PatternScan(baseModule, "41 C6 44 03 ?? 00 41 C6 44 03 ?? 00 41 C6 44 03 ?? 00 41 C6 44 03 ?? 00", "MGS 2: Water Effects - Surface Splash Alpha | okajima\\effect2\\splush_surface_man.c -> CulcVector()"))
    {
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(address) + 5,  "\xFF", 1);
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(address) + 11, "\xFF", 1);
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(address) + 17, "\xFF", 1);
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(address) + 23, "\xFF", 1);
        spdlog::info("MGS 2: Water Effects - Surface splash alpha restored.");
    }

    if (uint8_t* mdlDraw = Memory::PatternScan(baseModule, kInitMdlDrawSig, "MGS 2: Water Effects - InitMdlDraw"))
    {
        g_initMdlDraw = reinterpret_cast<InitMdlDrawFn>(mdlDraw);

        if (uint8_t* address = Memory::PatternScan(baseModule, "40 53 48 83 EC ?? 48 8B D9 E8 ?? ?? ?? ?? 48 8B CB 66 0F 6E C0", "MGS 2: Water Effects - Underwater Warp Act (shibata\\demo\\scr_water_demo.c -> NewScrWater_Demo() -> Act())"))
        {
            g_actHook = safetyhook::create_inline(address, reinterpret_cast<void*>(Act_Detour));
            LOG_HOOK(g_actHook, "MGS 2: Water Effects - Underwater Warp Act");
        }
    }

    if (uint8_t* address = Memory::PatternScan(baseModule, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 45 33 C9 8B FA 8B F1 BA 00 10 00 00 41 B8 90 00 01 00", "MGS 2: Water Effects - Suppress p01 body splash (NewSplushSurfaceMan())"))
    {
        g_mgrSpawnHook = safetyhook::create_inline(address, reinterpret_cast<void*>(MgrSpawn_Detour));
        LOG_HOOK(g_mgrSpawnHook, "MGS 2: Water Effects - Suppress p01 body splash");
    }
}
