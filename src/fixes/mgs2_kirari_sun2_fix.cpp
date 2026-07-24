#include "stdafx.h"

#include "mgs2_kirari_sun2_fix.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "input_handler.hpp"
#include "logging.hpp"


void MGS2_Kirari_Sun2Fix::ApplyFix()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    spdlog::info("MGS2 Kirari Sun Sparkle Fix: Initializing...");


    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 8B 55 ?? 33 C0 4C 8B 45 ?? 0F 28 CE", "MGS2: Sun Sparkle Fix: okajima\\effect3\\kirari_water_sun2.c -> NewKirariWaterSun2() -> Act() | @l384: ", {
        float* sparkle_effect_root_pos = reinterpret_cast<float*>(ctx.rcx);

        const float cam_x = ctx.xmm6.f32[0];
        const float cam_y = ctx.xmm13.f32[0];
        const float cam_z = ctx.xmm7.f32[0];
        const float water_level = g_GameVars.get_GM_WaterLevel();

        float camera_offset = cam_y - sparkle_effect_root_pos[1];
        if (camera_offset == 0.0f)
        {
            camera_offset = 0.0001f;
        }
        const float scale = (cam_y - water_level) / camera_offset;

        sparkle_effect_root_pos[0] = (sparkle_effect_root_pos[0] - cam_x) * scale;
        sparkle_effect_root_pos[1] = (sparkle_effect_root_pos[1] - cam_y) * scale;
        sparkle_effect_root_pos[2] = (sparkle_effect_root_pos[2] - cam_z) * scale;

                  });


    //readd compiled out KP_XBOX NaN protection

    MAKE_HOOK_MID(baseModule, "F3 0F 10 45 ?? 48 8D 55 ?? F3 0F 10 74 24", "MGS2: Kirari Water Sun Fix: okajima\\effect3\\kirari_water_sun.c -> NewKirariWaterSun() -> Act() | @l285 (sun_pos.vw)", {
        float* w = reinterpret_cast<float*>(ctx.rbp - 0x34);
        if (*w == 0.0f) *w = 0.00001f;
                  });

    MAKE_HOOK_MID(baseModule, "F3 0F 10 4D ?? 48 8D 55 ?? F3 0F 10 55 ?? 48 8D 4D ?? F3 0F 10 5D ?? 0F 28 C6", "MGS2: Kirari Water Sun Fix: okajima\\effect3\\kirari_water_sun.c -> NewKirariWaterSun() -> Act() | @l293 (sun_pos_mirror.vw)", {
        float* w = reinterpret_cast<float*>(ctx.rbp - 0x44);
        if (*w == 0.0f) *w = 0.00001f;
                  });

    MAKE_HOOK_MID(baseModule, "F3 0F 10 45 ?? 0F 28 D7 F3 0F 5E 75", "MGS2: Kirari Water Sun Fix: okajima\\effect3\\kirari_water_sun.c -> NewKirariWaterSun() -> Act() | @l302 (near_limit.vw)", {
        float* w = reinterpret_cast<float*>(ctx.rbp - 0x4);
        if (*w == 0.0f) *w = 0.00001f;
                  });

    MAKE_HOOK_MID(baseModule, "69 0D ?? ?? ?? ?? ?? ?? ?? ?? 0F 57 C0 ?? ?? ?? ?? ?? 0F 28 CF", "MGS2: Kirari Water Sun Fix: okajima\\effect3\\kirari_water_sun.c -> NewKirariWaterSun() -> Act() | @l359 (cal_pos_after->vw)", {
        float* w = reinterpret_cast<float*>(ctx.r14 + 0xC);
        if (*w == 0.0f) *w = 0.00001f;
                  });
}
