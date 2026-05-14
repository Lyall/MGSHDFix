#include "stdafx.h"
#include "original_camera_positions.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "input_handler.hpp"
#include "logging.hpp"

namespace
{
    bool toggleValue = true;
    bool OverrideCameraPositions()
    {
        if (!toggleValue)
        {
            return false;
        }
        return !g_GameVars.InCutscene();
    }
}

void OriginalCameraPositions::Activate()
{
    if (!(eGameType & (MGS2|MGS3)))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS 2 | MGS 3: Original Camera Positions: Config disabled, skipping.");
        return;
    }

    if (eGameType & MGS2)
    {
        /*  don't seem needed. origin y / x.
        MAKE_HOOK_MID(baseModule, "F3 41 0F 11 41 ?? F3 41 0F 59 58", "Original Camera Positions: xmm0", {
            if (OverrideCameraPositions())
            {
                spdlog::info("xmm0 {},", ctx.xmm0.f32[0]);
                ctx.xmm0.f32[0] = 0.0f;
            }
                      });

        MAKE_HOOK_MID(baseModule, "F3 41 0F 11 59 ?? C3", "Original Camera Positions: xmm3", {
            if (OverrideCameraPositions())
            {
                spdlog::info("xmm3 {},", ctx.xmm3.f32[0]);
                ctx.xmm3.f32[0] = 0.0f;
            }
                      });
                      */

        MAKE_HOOK_MID(baseModule, "F3 0F 59 0D ?? ?? ?? ?? F3 0F 58 0D ?? ?? ?? ?? F3 0F 11 49 14", "MGS2: Camera RatioX", {
            if (OverrideCameraPositions())
            {
            ctx.xmm1.f32[0] = 0.0f;
            }
                      });

        MAKE_HOOK_MID(baseModule, "F3 0F 59 15 ?? ?? ?? ?? F3 0F 58 15 ?? ?? ?? ?? F3 0F 59 0D", "MGS2: Camera RatioY", {
            if (OverrideCameraPositions())
            {
            ctx.xmm2.f32[0] = 0.0f;
            }
                      });
    }
    else /*eGameType & MGS3*/
    {
        MAKE_HOOK_MID(baseModule, "F3 0F 59 0D ?? ?? ?? ?? F3 0F 58 C8 F3 0F 10 25", "MGS3: Camera RatioX", {
            if (OverrideCameraPositions())
            {
            ctx.xmm1.f32[0] = 0.0f;
            }
                      });

        MAKE_HOOK_MID(baseModule, "F3 0F 59 15 ?? ?? ?? ?? F3 0F 58 D4 F3 0F 5E C8", "MGS3: Camera RatioY", {
            if (OverrideCameraPositions())
            {
            ctx.xmm2.f32[0] = 0.0f;
            }
                      });
        
    }


    g_InputHandler.RegisterHotkey(vkToggle_HDC_CameraPositions, "Toggle HDC Camera Positions", []()
                                  {
                                      toggleValue = !toggleValue;
                                  });
}
