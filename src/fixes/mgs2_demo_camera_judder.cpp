#include "stdafx.h"
#include "mgs2_demo_camera_judder.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "logging.hpp"

namespace
{
    SafetyHookInline gSetCameraAdjustHook {};

    // Ambient rumbles (etc/scncamvib.c) are a few units; dramatic knocks are much larger.
    constexpr float kAmbientShakeMax = 8.0f;

    // GM_SetCameraAdjust( int chanl, FVECTOR *adj )
    void SetCameraAdjust_Detour(int chanl, float* adj)
    {
        if (chanl == 0 && adj != nullptr && g_GameVars.InScriptedSequence()
            && fabsf(adj[0]) < kAmbientShakeMax && fabsf(adj[1]) < kAmbientShakeMax)
        {
            return;   // drop the ambient rumble; the camera stays on the exact demo track
        }
        gSetCameraAdjustHook.fastcall<void>(chanl, adj);
    }
}

void MGS2DemoCameraJudder::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }
    if (g_GameVars.DG_Chanl(0) == nullptr)
    {
        spdlog::info("MGS2: Demo Camera Judder - DG_Chanls gamevars patternscan failure; fix disabled.");
        return;
    }

    if (uint8_t* setAdjust = Memory::PatternScan(baseModule,
        "48 63 C1 48 8D 0D ?? ?? ?? ?? C7 84 81 ?? ?? ?? ?? 01 00 00 00 48 03 C0 F3 0F 10 02 0F 57 05",
        "MGS2: Demo Camera Judder: game/cam_set.c -> GM_SetCameraAdjust()"))
    {
        gSetCameraAdjustHook = safetyhook::create_inline(setAdjust, reinterpret_cast<void*>(SetCameraAdjust_Detour));
        LOG_HOOK(gSetCameraAdjustHook, "MGS2: Demo Camera Judder: game/cam_set.c -> GM_SetCameraAdjust()");
    }
}
