#include "stdafx.h"

#include "mgs2_kirari_sun2_fix.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "input_handler.hpp"
#include "logging.hpp"

// todo:
// 1) depth is incorrect, the sparkle is rendering behind the level geometry, should be a mix of infront and behind. pos 0/2 likely need tweaking
// 2) reffing doc of mgs2 -> seems sparkles in second angle should be a bit more to the right.
//      - third also seems to be a bit far right too
// 3) check if it's at the right speed lol.

namespace
{


    //bool print_next = true;
    //int current_angle = 1;
    //int last_camera_angle = 0;
    //float HEIGHT_OFFSET = 0.24f; //fullscreen
    //float HEIGHT_OFFSET = 0.26f; //letterbox

    constexpr float camera_angle_one = -34698.402f;
    constexpr float camera_angle_two = -38726.996f;
    //constexpr float camera_angle_three = -36208.715f;
    constexpr float fullscreen_camera_angle_one_height_offset = 0.30f;
    constexpr float fullscreen_camera_angle_two_height_offset = 0.13f;
    constexpr float fullscreen_camera_angle_three_height_offset = 0.15f;
    
}

void MGS2_Kirari_Sun2Fix::ApplyFix()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    spdlog::info("MGS2 Kirari Sun Sparkle Fix: Initializing...");

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 8B 55 ?? 33 C0 4C 8B 45 ?? 0F 28 CE", "MGS2: Sun Sparkle Fix: okajima\\effect3\\kirari_water_sun2.c -> NewKirariWaterSun2() : ", {
        float* sparkle_effect_root_pos = reinterpret_cast<float*>(ctx.rcx);

        const float cam_x = ctx.xmm6.f32[0];
        const float cam_y = ctx.xmm13.f32[0];
        const float cam_z = ctx.xmm7.f32[0];
        const float water_level = g_GameVars.get_GM_WaterLevel(); //probably able to simplify to 549596.44, this effect is only used in P070_02_P02 from the look of it.

        float camera_offset = cam_y - sparkle_effect_root_pos[1];
        if (camera_offset == 0.0f)
        {
            camera_offset = 0.0001f;//0.000001f; //0.0001f original - however they were too small. larger epsilon is from the old working version;
        }
        const float scale = (cam_y - water_level) / camera_offset;
        /*current_angle = (cam_y == camera_angle_one) ? 1 : ((cam_y == camera_angle_two) ? 2 : 3);
        if (last_camera_angle != current_angle)
        {
            print_next = true;
            last_camera_angle = current_angle;
            HEIGHT_OFFSET = 
        }
        if (print_next)
        {
            spdlog::info("current_angle = {}, scale {}, camera_offset {}, cam_y {}", current_angle, scale, camera_offset, cam_y);
            print_next = false;
        }*/
        sparkle_effect_root_pos[0] = (sparkle_effect_root_pos[0] - cam_x) * scale;
        sparkle_effect_root_pos[1] = (sparkle_effect_root_pos[1] - cam_y) * scale * ((cam_y == camera_angle_one) ? fullscreen_camera_angle_one_height_offset : ((cam_y == camera_angle_two) ? fullscreen_camera_angle_two_height_offset : fullscreen_camera_angle_three_height_offset));
        //sparkle_effect_root_pos[1] = (sparkle_effect_root_pos[1] - cam_y) * scale * HEIGHT_OFFSET; 
        sparkle_effect_root_pos[2] = (sparkle_effect_root_pos[2] - cam_z) * scale;

                  });
    /*
    g_InputHandler.RegisterHotkey(VK_ADD, "offset_plus", []()
                                  {
                                      HEIGHT_OFFSET += 0.005f;
                                      spdlog::info("Height Offset increased: {}", HEIGHT_OFFSET);
                                      print_next = true;
                                  });

    g_InputHandler.RegisterHotkey(VK_SUBTRACT, "offset_minus", []()
                                  {
                                      HEIGHT_OFFSET -= 0.005f;
                                      spdlog::info("Height Offset decreased: {}", HEIGHT_OFFSET);
                                      print_next = true;
                                  });
*/
}
