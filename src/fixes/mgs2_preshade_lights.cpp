#include "stdafx.h"

#include "mgs2_preshade_lights.hpp"

#include "common.hpp"
#include "logging.hpp"

// Make MGS2's preshade directional and point lights two-sided, so surfaces whose normals face away from a
// light are lit instead of baking to near-black (fixes black watertight doors and dark poster light pools).

namespace
{
    // Half-lambert wrap strength for the directional light (1.0 => i = 0.5*dot + 0.5).
    constexpr float kWrapScale = 1.0f;
    // Scale applied to the away-facing side, tuned so front-facing surfaces stay unchanged.
    constexpr float kBackSideScale = 0.6f;
}

void MGS2PreshadeLights::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    // Directional light: light away-facing normals with a half-lambert wrap instead of clamping to ambient.
    MAKE_HOOK_MID(baseModule, "F3 0F 5F CA F3 0F 59 F1 F3 0F 59 F9",
        "MGS 2: Two-Sided Preshade Lighting : system\\libdg\\pshade.c -> BP_ParallelAmbientCalc()", {
        const float d = ctx.xmm2.f32[0];                        // dot(normal, light direction)
        const float wrap = kWrapScale * (d * 0.5f + 0.5f);
        const float back = d < 0.0f ? -d * kBackSideScale : 0.0f;
        float i = wrap > back ? wrap : back;
        if (i > 1.0f) i = 1.0f;
        ctx.xmm1.f32[0] = i;
    });

    // Point light: apply the scaled away-facing side so those normals receive the light instead of none.
    MAKE_HOOK_MID(baseModule, "44 0F 2F CE 77 60 0F B6 43 10 0F 28 C2",
        "MGS 2: Two-Sided Preshade Lighting : system\\libdg\\pshade.c -> BP_PointLightCalc()", {
        const float d = ctx.xmm6.f32[0];                        // dot(direction-to-vertex, normal)
        if (d < 0.0f)
            ctx.xmm6.f32[0] = -d * kBackSideScale;
    });
}
