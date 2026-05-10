#include "stdafx.h"

#include"resolution_scaling_fixes.hpp"

#include "common.hpp"
#include "custom_resolution_and_borderless.hpp"
#include "logging.hpp"

namespace
{
    double scaleX_fromPs2 = 1.0;
    double scaleX_fromPs2_4by3 = 1.0;

    double scaleY_fromPs2 = 1.0;
}



void ResolutionScalingFixes::ApplyFixes()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    scaleX_fromPs2 = CustomResolutionAndBorderless::iInternalResX / 512.0;
    scaleX_fromPs2_4by3 = CustomResolutionAndBorderless::iInternalResX / 512.0;
    scaleY_fromPs2 = CustomResolutionAndBorderless::iInternalResY / 448.0;

    SPDLOG_INFO("Resolution Scaling Fixes: Internal Width = {}, Internal Height = {}", CustomResolutionAndBorderless::iInternalResX, CustomResolutionAndBorderless::iInternalResY);
    SPDLOG_INFO("Resolution Scaling Fixes: PS2 Height Delta = {}, PS2 Width Delta = {}, PS2 Width 4:3 Delta = {}", scaleY_fromPs2, scaleX_fromPs2, scaleX_fromPs2_4by3);

    {   // user/mode/codec/face_bug.c -> NewBugFace() - not affected by resolution, might've just been the layout change which caused it.

        MAKE_HOOK_MID(baseModule, "48 89 83 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 8B ?? ?? ?? ?? BA", "NewBugFace", {
            ctx.xmm1.f32[0] = 3018.0f; //width | 512.0f original
            ctx.xmm2.f32[0] = 860.4896f;    //height | 384.0f original | 872.0f was a little over in case there's some effect i missed at the bottom of the sprite. 860.4896f = pixel perfect in photoshop
        });
    }

    MAKE_HOOK_MID(baseModule, "F3 0F 11 74 24 ?? E8 ?? ?? ?? ?? 81 67", "MGS2: Resolution Scaling Fixes : user\\kira\\radar\\bomb_sensor.c -> setup_bomb_object() - Scale y", {
        ctx.xmm6.f32[0] *= 0.75f;

                  });

}


