#include "stdafx.h"
#include "original_camera_positions.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "input_handler.hpp"
#include "logging.hpp"

namespace
{
    bool toggleValue = true;
    bool codec_camera_init = false;
}

bool OriginalCameraPositions::NeedsCameraCorrection()
{
    if (!bEnabled || !toggleValue || codec_camera_init)
    {
        return false;
    }

    return !g_GameVars.InCutscene();
}

void OriginalCameraPositions::Activate()
{
    if (!(eGameType & (MGS2 | MGS3)))
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
        MAKE_HOOK_MID(baseModule, "48 89 05 ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 0F 10 25", "MGS2: BP_Camera_Init() -> codec compute offsets before", {
            codec_camera_init = true;
        });

        MAKE_HOOK_MID(baseModule, "F3 0F 10 25 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 89 05", "MGS2: BP_Camera_Init() -> codec compute offsets after", {
            codec_camera_init = false;
        });
    }
    else
    {
        MAKE_HOOK_MID(baseModule, "F3 0F 59 0D ?? ?? ?? ?? F3 0F 58 C8 F3 0F 10 25", "MGS3: Camera RatioX", {
            if (NeedsCameraCorrection())
            {
                ctx.xmm1.f32[0] = 0.0f;
            }
        });

        MAKE_HOOK_MID(baseModule, "F3 0F 59 15 ?? ?? ?? ?? F3 0F 58 D4 F3 0F 5E C8", "MGS3: Camera RatioY", {
            if (NeedsCameraCorrection())
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
