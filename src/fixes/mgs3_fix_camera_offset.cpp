#include "stdafx.h"
#include "mgs3_fix_camera_offset.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "logging.hpp"
#include "water_reflections.hpp"

namespace
{

    SafetyHookInline MGS3_BP_GetScreenOffsetY_hook {};

    float BP_GetScreenOffsetY()
    {
        if (!g_WaterReflectionFix.MGS3_UseAdjustedOffsetY)
        {
            return 0.0f;
        }
        if (g_GameVars.InCutscene())
        {
            //bluepoint's viewport offset math was off a hair and put the screen wayyyyy higher than it should've been, cutting off about 10% of the bottom of the screen during all cutscenes.
            return MGS3_BP_GetScreenOffsetY_hook.stdcall<float>() - 0.132548f;
        }
        return MGS3_BP_GetScreenOffsetY_hook.stdcall<float>();
    }

}


void MGS3FixCameraOffset::Activate()
{
    if (!(eGameType & MGS3))
    {
        return;
    }

    spdlog::info("MGS 3: Cutscene camera Y offset: Initializing...");

    uint8_t* MGS3_BP_GetScreenOffsetYScanResult = Memory::PatternScanSilent(baseModule, "E8 ?? ?? ?? ?? F3 44 ?? ?? ?? E8 ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? 00 00");
    uintptr_t MGS3_BP_GetScreenOffsetYScanAddress = Memory::GetAbsolute((uintptr_t)MGS3_BP_GetScreenOffsetYScanResult + 0xB);
    if (MGS3_BP_GetScreenOffsetYScanResult && MGS3_BP_GetScreenOffsetYScanAddress)
    {
        MGS3_BP_GetScreenOffsetY_hook = safetyhook::create_inline(reinterpret_cast<void*>(MGS3_BP_GetScreenOffsetYScanAddress), reinterpret_cast<void*>(BP_GetScreenOffsetY));
        LOG_HOOK(MGS3_BP_GetScreenOffsetY_hook, "MGS 3: BP_GetScreenOffsetY")
    }
    else
    {
        spdlog::error("MGS 3: BP_GetScreenOffsetY: Pattern scan failed.");
    }
    /*
    MAKE_HOOK_MID(baseModule, "F3 44 0F 10 94 24 ?? ?? ?? ?? F3 44 0F 58 D0", "MGS3: Cutscene camera Y offset", {
        if (g_WaterReflectionFix.MGS3_UseAdjustedOffsetY && g_GameVars.InCutscene())
        {
            //spdlog::info("xmm0 {}", ctx.xmm0.f32[0]);
            ctx.xmm0.f32[0] -= 0.132548f;
            //0.140280f = 7 pixels too high @ 4k
        }
        });*/
}
