#include "stdafx.h"

#include "common.hpp"
#include "logging.hpp"
#include "gamevars.hpp"

#include "screenspace_fixes.hpp"

#include "mgs2_flare_occlusion.hpp"

// These effects run a point through eye_pers, then scale it again by hand using ASPECT_X()/ASPECT_Y() to get its final screen position.
//
// Problem: eye_pers bakes in BP's cinema-camera offset, and the second scale throws it off from its intended position.
// 
// eye_pers_no_offset is the same projection without the incorrect offsets baked in.


void ScreenspaceFixes::Apply()
{
    if (eGameType & MGS2)
    {



            MAKE_HOOK_MID(baseModule, "F3 0F 59 F1 F3 0F 59 F9 F3 44 0F 59 C1 F3 0F 11 75 ?? F3 0F 11 7D ?? F3 44 0F 11 45 ?? F3 0F 59 F0", "MGS2: Screenspace Fixes: okajima\\effect3\\kirari_water_sun2.c -> NewKirariWaterSun2() -> Act() : @l255", {
                    RETARGET_STRUCT_ENTRY(ctx.rcx, DG_CHANL, eye_pers, eye_pers_no_offset);
                });

            MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 48 8D 55 ?? 48 8D 4D ?? E8 ?? ?? ?? ?? B9", "MGS2: Screenspace Fixes: user\\shibata\\effect\\ray_eye.c -> InitTailsData()", {
                    RETARGET_STRUCT_ENTRY(ctx.rcx, DG_CHANL, eye_pers, eye_pers_no_offset);
                });

            MAKE_HOOK_MID(baseModule, "41 89 86 ?? ?? ?? ?? 8B 47 ?? 41 89 86 ?? ?? ?? ?? ?? ?? 41 89 86", "MGS2: Screenspace Fixes: user\\shibata\\effect\\ray_eye.c -> TaileAct_NoCheck()", {
                    RETARGET_STRUCT_ENTRY(ctx.rcx, DG_CHANL, eye_pers, eye_pers_no_offset);
                });

            MAKE_HOOK_MID(baseModule, "41 89 46 ?? 8B 43 ?? 41 89 46 ?? ?? ?? ?? ?? ?? 8B 43 ?? 41 89 46", "MGS2: Screenspace Fixes: user\\shibata\\effect\\ray_eye.c -> TaileAct_Check()", {
                    RETARGET_STRUCT_ENTRY(ctx.rcx, DG_CHANL, eye_pers, eye_pers_no_offset);
                });
    



#pragma region demo_lens_flare

            MAKE_HOOK_MID(baseModule, "44 0F 29 84 24 ?? ?? ?? ?? 44 0F 29 94 24 ?? ?? ?? ?? 44 0F 29 9C 24", "MGS 2: Screenspace Fixes: user\\okajima\\demo\\lens_flare.c -> Act()", {
                    RETARGET_STRUCT_ENTRY(ctx.rcx, DG_CHANL, eye_pers, eye_pers_no_offset);
                });


    // Lens flare: the PS2 blends the sprites at As/128, D3D at As/255, so the flare renders at half
    // brightness. The ramp is also authored for the 30fps demo tick, and the cull / edge-fade thresholds
    // fire ~33% early under the cinema's vertical crop. Fix all three, and feed the sun's position to
    // mgs2_flare_occlusion so the flares dim when the sun is occluded.
            uint8_t* lensFlareRamp = Memory::PatternScan(baseModule,
                                                         "8B 05 ?? ?? ?? ?? FF C0 66 0F 6E C0 0F 5B C0 F3 0F 59 83 ?? ?? ?? ?? 44 39 93",
                                                         "MGS 2: Effect Speed Fix: user\\shibata\\demo\\lens_flare.c -> Act()");
            if (lensFlareRamp)
            {
                // The hooks below sit past the matched bytes; make sure each lands on the expected instruction
                // so a changed exe fails cleanly instead of hooking mid-instruction.
                const bool offsetsValid =
                    memcmp(lensFlareRamp + 0xEA, "\x0F\x2F\xC1", 3) == 0 &&         // comiss xmm0, xmm1
                    memcmp(lensFlareRamp + 0x106, "\x44\x0F\x2F\xC0", 4) == 0 &&     // comiss xmm8, xmm0
                    memcmp(lensFlareRamp + 0x166, "\xF3\x44\x0F\x5C\xC1", 5) == 0 && // subss xmm8, xmm1
                    memcmp(lensFlareRamp + 0x180, "\xF3\x44\x0F\x5E", 4) == 0 &&     // divss xmm10, [rip]
                    memcmp(lensFlareRamp + 0x1C3, "\x48\x8B\xCB", 3) == 0;           // mov rcx, rbx
                if (!offsetsValid)
                {
                    spdlog::error("MGS 2: Effect Speed Fix: lens flare hook offsets no longer match; flare fixes disabled.");
                    lensFlareRamp = nullptr;
                }
            }
            if (lensFlareRamp)
            {
                static SafetyHookMid lensFlareRampRateHook {};
                lensFlareRampRateHook = safetyhook::create_mid(lensFlareRamp + 0x17,   // xmm0 = ramp advance
                                                               [](SafetyHookContext& ctx)
                                                               {
                                                                   ctx.xmm0.f32[0] *= 0.5f;   // Act runs at 60fps, the ramp delta is authored for 30
                                                               });
                LOG_HOOK(lensFlareRampRateHook, "MGS 2: Effect Speed Fix: lens flare pulse rate")
                    static SafetyHookMid lensFlareAlphaHook {};
                lensFlareAlphaHook = safetyhook::create_mid(lensFlareRamp + 0x1C3,     // xmm9 = final sprite alpha
                                                            [](SafetyHookContext& ctx)
                                                            {
                                                                // GS As/128 -> D3D As/255, scaled by the sun occlusion ratio.
                                                                ctx.xmm9.f32[0] *= 1.9921875f * 0.85f * MGS2FlareOcclusion::GetVisibility();
                                                            });
                LOG_HOOK(lensFlareAlphaHook, "MGS 2: Effect Speed Fix: lens flare GS alpha")


            }
#pragma endregion

    
    
    
    
    
    }









}
