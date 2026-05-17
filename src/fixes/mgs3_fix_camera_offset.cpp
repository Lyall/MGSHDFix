#include "stdafx.h"
#include "mgs3_fix_camera_offset.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "logging.hpp"

//bluepoint's viewport offset math was off a hair and put the screen wayyyyy higher than it should've been, cutting off about 10% of the bottom of the screen during all cutscenes.

void MGS3FixCameraOffset::Activate()
{
    if (!(eGameType & MGS3))
    {
        return;
    }

    MAKE_HOOK_MID(baseModule, "F3 44 0F 10 94 24 ?? ?? ?? ?? F3 44 0F 58 D0", "MGS3: Cutscene camera Y offset", {
        if (g_GameVars.InCutscene())
        {
            //spdlog::info("xmm0 {}", ctx.xmm0.f32[0]);
            ctx.xmm0.f32[0] -= 0.132548f;
            //0.140280f = 7 pixels too high @ 4k
        }
        });
}
