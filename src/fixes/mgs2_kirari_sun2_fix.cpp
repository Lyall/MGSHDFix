#include "stdafx.h"

#include "mgs2_kirari_sun2_fix.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "input_handler.hpp"
#include "logging.hpp"

// todo:
// 1) depth is incorrect, the sparkle is rendering behind the level geometry, should be a mix of infront and behind. pos 0/2 likely need tweaking
// 2) might need to apply per-angle height offsets. count # of calls per shot -> switch height offset off that num
// 3) confirm both letterbox & fullscreen are good. ~0.26f is generally right for both, but i did notice the spread is a bit off
// 4) reffing doc of mgs2 -> seems sparkles in second angle should be a bit more to the right.
//      - third also seems to be a bit far right too
// 5) check if it's at the right speed lol.

namespace
{



    float HEIGHT_OFFSET = 0.27f; //fullscreen
    //float HEIGHT_OFFSET = 0.26f; //letterbox
    
}

void MGS2_Kirari_Sun2Fix::ApplyFix()
{
#if defined(RELEASE_BUILD)
    return; //still being cooked 
#endif

    if (!(eGameType & MGS2))
    {
        return;
    }



    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 8B 55 ?? 33 C0 4C 8B 45 ?? 0F 28 CE", "MGS2: Sun Sparkle Fix: okajima\\effect3\\kirari_water_sun2.c -> NewKirariWaterSun2() : ", {
        float* sparkle_effect_root_pos = reinterpret_cast<float*>(ctx.rcx);

        float cam_x = ctx.xmm6.f32[0];
        float cam_y = ctx.xmm13.f32[0];
        float cam_z = ctx.xmm7.f32[0];
        float water_level = g_GameVars.get_GM_WaterLevel();

        float camera_offset = cam_y - sparkle_effect_root_pos[1];
        if (camera_offset == 0.0f)
        {
            camera_offset = 0.0001f;//0.000001f; //0.0001f original - however they were too small. larger epsilon is from the old working version;
        }
        float scale = (cam_y - water_level) / camera_offset;
        //spdlog::info("scale {}, camera_offset {}, cam_y {}, GM_WaterLevel {}", scale, camera_offset, cam_y, water_level);
        sparkle_effect_root_pos[0] = (sparkle_effect_root_pos[0] - cam_x) * scale;
        sparkle_effect_root_pos[1] = (sparkle_effect_root_pos[1] - cam_y) * scale * HEIGHT_OFFSET;
        sparkle_effect_root_pos[2] = (sparkle_effect_root_pos[2] - cam_z) * scale;

                  });

    g_InputHandler.RegisterHotkey(VK_ADD, "offset_plus", []()
                                  {
                                      HEIGHT_OFFSET += 0.01f;
                                      spdlog::info("Height Offset increased: {}", HEIGHT_OFFSET);
                                  });

    g_InputHandler.RegisterHotkey(VK_SUBTRACT, "offset_minus", []()
                                  {
                                      HEIGHT_OFFSET -= 0.01f;
                                      spdlog::info("Height Offset decreased: {}", HEIGHT_OFFSET);
                                  });

    /* fullscreen
    [2026 - 05 - 10 18:04 : 51.443] [info] scale 188021.83, dmmd 2.7304688, cam_vy - 36208.715, wl - 549596.44

        [2026 - 05 - 10 18:04 : 51.443][info] scale 49334.555, dmmd 10.40625, cam_vy - 36208.715, wl - 549596.44

        [2026 - 05 - 10 18:04 : 51.443][info] scale - 67537.13, dmmd - 7.6015625, cam_vy - 36208.715, wl - 549596.44

        [2026 - 05 - 10 18:04 : 51.443][info] scale - 134521.25, dmmd - 3.8164062, cam_vy - 36208.715, wl - 549596.44

        [2026 - 05 - 10 18:04 : 51.443][info] scale 32236.266, dmmd 15.925781, cam_vy - 36208.715, wl - 549596.44

        [2026 - 05 - 10 18:04 : 51.443][info] scale 70812.1, dmmd 7.25, cam_vy - 36208.715, wl - 549596.44

        [2026 - 05 - 10 18:04 : 51.460][info] scale 95168.18, dmmd 5.3945312, cam_vy - 36208.715, wl - 549596.44

        [2026 - 05 - 10 18:04 : 51.460][info] scale - 1684964.9, dmmd - 0.3046875, cam_vy - 36208.715, wl - 549596.44

        [2026 - 05 - 10 18:04 : 51.460][info] scale - 495951.9, dmmd - 1.0351562, cam_vy - 36208.715, wl - 549596.44

        [2026 - 05 - 10 18:04 : 51.460][info] scale 194707.05, dmmd 2.6367188, cam_vy - 36208.715, wl - 549596.44

        [2026 - 05 - 10 18:04 : 51.460][info] scale - 278447.56, dmmd - 1.84375, cam_vy - 36208.715, wl - 549596.44

        */

}
