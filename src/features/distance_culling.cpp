#include "common.hpp"
#include "distance_culling.hpp"

#include "input_handler.hpp"
#include "logging.hpp"



void DistanceCulling::Initialize() const
{

    if (eGameType & MGS3)
    {
        if (bForceGrassAlways || fGrassDistanceScalar != 1.0f)
        {
            MAKE_HOOK_MID(baseModule, "F3 0F 11 83 ?? ?? ?? ?? 41 8B FC", "MGS3: Grass Farclip", {
               ctx.xmm0.f32[0] = g_DistanceCulling.bForceGrassAlways ? std::numeric_limits<float>::max() : ctx.xmm0.f32[0] * g_DistanceCulling.fGrassDistanceScalar;
                })

            if (bForceGrassAlways)
            {
                g_InputHandler.RegisterHotkey(vkForceGrassAlwaysToggle, "Force Grass Distance", []()
                    {
                        g_DistanceCulling.bForceGrassAlways = !g_DistanceCulling.bForceGrassAlways;
                    });
            }
        }
    }
}
