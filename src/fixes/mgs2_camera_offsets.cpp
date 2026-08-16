#include "stdafx.h"
#include "mgs2_camera_offsets.hpp"

#include "common.hpp"
#include "logging.hpp"
#include "mgs2_demo_scope.hpp"
#include "original_camera_positions.hpp"

namespace
{
    bool NeedsCameraCorrection()
    {
        return MGS2DemoScope::NeedsCameraCorrection() || OriginalCameraPositions::NeedsCameraCorrection();
    }
}

void MGS2CameraOffsets::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    MAKE_HOOK_MID(baseModule, "F3 0F 59 0D ?? ?? ?? ?? F3 0F 58 0D ?? ?? ?? ?? F3 0F 11 49 14", "MGS 2: Camera Offsets: Ratio X", {
        if (NeedsCameraCorrection())
        {
            ctx.xmm1.f32[0] = 0.0f;
        }
    });

    MAKE_HOOK_MID(baseModule, "F3 0F 59 15 ?? ?? ?? ?? F3 0F 58 15 ?? ?? ?? ?? F3 0F 59 0D", "MGS 2: Camera Offsets: Ratio Y", {
        if (NeedsCameraCorrection())
        {
            ctx.xmm2.f32[0] = 0.0f;
        }
    });
}
