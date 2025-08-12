#include "common.hpp"
#include "mgs3_hud_fixes.hpp"

#include "logging.hpp"

void MGS3HudFixes::Initialize()
{
    if (!(eGameType & MGS3))
    {
        return;
    }

    MAKE_HOOK_MID(baseModule, "F3 0F 59 C7 F3 0F 2C C8 66 89 48 ?? 66 3B D1 74 ?? 81 48 ?? ?? ?? ?? ?? 0F B7 48", "MGS3: NVG Cross", {
        ctx.xmm1.f32[0] += 160.0f; //y axis
        ctx.xmm0.f32[0] = ((((ctx.xmm0.f32[0] - 256.0f) * 2) + 512.0f) - 32.0f); //x axis
        });

    MAKE_HOOK_MID(baseModule, "F3 0F 59 C7 F3 0F 2C C8 66 89 48 ?? 66 3B D1 74 ?? 81 48 ?? ?? ?? ?? ?? 0F B7 48", "MGS3: NVG Cross 2", {
        spdlog::info("MGS3: NVG Cross 2 - Location 1");
        });
}
