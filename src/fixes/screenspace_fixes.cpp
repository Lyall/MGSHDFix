#include "stdafx.h"

#include "common.hpp"
#include "logging.hpp"
#include "gamevars.hpp"

#include "screenspace_fixes.hpp"
#include "effect_speeds.hpp"
#include "mgs2_flare_occlusion.hpp"


// These effects run a point through eye_pers, then scale it again by hand using ASPECT_X()/ASPECT_Y() to get its final screen position.
//
// Problem: eye_pers bakes in BP's cinema-camera offset, and the second scale throws it off from its intended position.
// 
// eye_pers_no_offset is the same projection without the incorrect offsets baked in.

namespace {
    FMATRIX* g_pRainCamMat = nullptr;

    // Final plant_sun alpha marks active feedback scenes.
    std::atomic<ULONGLONG> g_plantSunLastActiveMs { 0 };
    SafetyHookInline g_plantSunActHook {};
    SafetyHookMid g_plantSunProjectionHook {};
    SafetyHookMid g_plantSunActivityHook {};

    void __fastcall PlantSunAct_Detour(uintptr_t work)
    {
        if (EffectSpeedFix::IsFeedbackHoldTick())
        {
            return;
        }
        g_plantSunActHook.fastcall<void>(work);
    }

    safetyhook::MidHook h_RainCameraDemoGate;


    uint8_t* g_pRainDemoGateTo = nullptr;

    void RainCameraDemo_LightDir_hook(SafetyHookContext& ctx)
    {
        const float A = ctx.xmm6.f32[0];
        const float B = ctx.xmm2.f32[0];
        const float C = *reinterpret_cast<float*>(ctx.r13 + ctx.rax * 8 + 8);

        const FMATRIX& m = g_GameVars.DG_Chanl(0)->eye_pers_no_offset;

        ctx.xmm5.f32[0] = A * m.m[0][0] + B * m.m[1][0] + C * m.m[2][0]; // x
        ctx.xmm6.f32[0] = A * m.m[0][1] + B * m.m[1][1] + C * m.m[2][1]; // y
        ctx.xmm4.f32[0] = std::fabs(A * m.m[0][3] + B * m.m[1][3] + C * m.m[2][3]); // w

        ctx.rip = reinterpret_cast<uint64_t>(g_pRainDemoGateTo);
    }

}

bool ScreenspaceFixes::IsPlantSunFeedbackActive()
{
    const ULONGLONG last = g_plantSunLastActiveMs.load(std::memory_order_relaxed);
    return last && (GetTickCount64() - last) < 100;
}

void ScreenspaceFixes::Apply()
{
    if (eGameType & MGS2)
    {
        if (uint8_t* plantSunAct = Memory::PatternScan(
                baseModule,
                "48 89 5C 24 10 57 48 83 EC 70 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 60 F3 0F 10 99 98 01 00 00",
                "MGS2: Screenspace Fixes: user\\shibata\\effect\\plant_sun.c -> Act()"))
        {
            g_plantSunActHook = safetyhook::create_inline(
                plantSunAct, reinterpret_cast<void*>(PlantSunAct_Detour));
            LOG_HOOK(g_plantSunActHook, "MGS 2: Screenspace Fixes: plant_sun Act cadence")
        }
        
        MAKE_HOOK_MID(baseModule, "F3 0F 59 F1 F3 0F 59 F9 F3 44 0F 59 E1", "MGS2: Screenspace Fixes: user\\okajima\\effect3\\kirari_water_sun.c -> NewKirariWaterSun() -> Act() : @ l281", {
                RETARGET_STRUCT_ENTRY(ctx.rcx, DG_CHANL, eye_pers, eye_pers_no_offset);
                      });

        
        MAKE_HOOK_MID(baseModule, "F3 0F 59 F1 F3 0F 59 F9 F3 44 0F 59 C1 F3 0F 11 75 ?? F3 0F 11 7D ?? F3 44 0F 11 45 ?? F3 0F 59 F0", "MGS2: Screenspace Fixes: user\\okajima\\effect3\\kirari_water_sun2.c -> NewKirariWaterSun2() -> Act() : @l255", {
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


        MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 41 0F B6 46 ?? 41 BF ?? ?? ?? ?? F3 0F 10 0D ?? ?? ?? ?? F3 0F 10 15 ?? ?? ?? ?? 4C 63 E7", "MGS2: Screenspace Fixes: user\\okajima\\effect3\\gas_pers_fast.c -> NewRainFogPersFast()", {
                RETARGET_STRUCT_ENTRY(ctx.rcx, DG_CHANL, eye_pers, eye_pers_no_offset);
                      });

        MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F B6 46", "MGS2: Screenspace Fixes: user\\okajima\\effect3\\gas2_pers_fast.c -> NewRainFogPersFast2()", {
                RETARGET_STRUCT_ENTRY(ctx.rcx, DG_CHANL, eye_pers, eye_pers_no_offset);
                      });


        MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 41 0F B6 46 ?? 41 BF ?? ?? ?? ?? F3 0F 10 0D ?? ?? ?? ?? F3 0F 10 15 ?? ?? ?? ?? 4C 63 E3", "MGS2: Screenspace Fixes: user\\okajima\\effect3\\gas_in_water.c -> NewGasInWater()", {
                RETARGET_STRUCT_ENTRY(ctx.rcx, DG_CHANL, eye_pers, eye_pers_no_offset);
                      });

        
        MAKE_HOOK_MID(baseModule, "44 0F 29 94 24 ?? ?? ?? ?? 44 0F 29 6C 24 ?? 44 0F 29 74 24", "MGS2: Screenspace Fixes: user\\kunibe\\effect\\demo_sun.c -> NewDemoSun() -> Flare_Act()", {
                RETARGET_STRUCT_ENTRY(ctx.rcx, DG_CHANL, eye_pers, eye_pers_no_offset);
                      });
        

        constexpr auto kPlantSunPattern =
            "44 0F 29 94 24 ?? ?? ?? ?? 44 0F 29 64 24 ?? 44 0F 29 6C 24";
        if (uint8_t* plantSun = Memory::PatternScan(
                baseModule, kPlantSunPattern,
                "MGS2: Screenspace Fixes: user\\shibata\\effect\\plant_sun.c -> NewPlantSunMain() -> Flare_Act()"))
        {
            // Use the final flare alpha as the activity signal.
            if (memcmp(plantSun + 0x1B5, "\xF3\x44\x0F\x2C\xD0", 5) == 0)
            {
                g_plantSunActivityHook = safetyhook::create_mid(
                    plantSun + 0x1B5,
                    [](SafetyHookContext& ctx)
                    {
                        if (ctx.xmm0.f32[0] >= 1.0f)
                            g_plantSunLastActiveMs.store(GetTickCount64(), std::memory_order_relaxed);
                    });
                LOG_HOOK(g_plantSunActivityHook, "MGS 2: Screenspace Fixes: plant_sun feedback activity")
            }
            else
            {
                spdlog::warn("MGS 2: Screenspace Fixes: plant_sun alpha scan failed; feedback cadence limiter disabled.");
            }

            g_plantSunProjectionHook = safetyhook::create_mid(
                plantSun,
                [](SafetyHookContext& ctx)
                {
                    RETARGET_STRUCT_ENTRY(ctx.rcx, DG_CHANL, eye_pers, eye_pers_no_offset);
                });
            LOG_HOOK(g_plantSunProjectionHook, "MGS 2: Screenspace Fixes: plant_sun projection")
        }
        // Prim.fx handles the GS alpha scale.



        MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? F3 0F 10 1D ?? ?? ?? ?? F3 0F 10 25 ?? ?? ?? ?? F3 0F 10 2D ?? ?? ?? ?? F3 44 0F 10 05 ?? ?? ?? ?? F3 44 0F 10 0D ?? ?? ?? ?? F3 44 0F 10 1D ?? ?? ?? ?? 44 0F 28 7C 24 ?? 44 0F 28 B4 24 ?? ?? ?? ?? 44 0F 28 AC 24 ?? ?? ?? ?? 44 0F 28 A4 24 ?? ?? ?? ?? 66 66 0F 1F 84 00", "MGS2: Screenspace Fixes: user\\okajima\\effect\\rain_gas_pers_demo.c -> NewRainFogPersDemo()", {
                RETARGET_STRUCT_ENTRY(ctx.rcx, DG_CHANL, eye_pers, eye_pers_no_offset);
                      });


        if (uint8_t* pMatWrite = Memory::PatternScan(baseModule,  "0F 29 05 ?? ?? ?? ?? 0F 28 05 ?? ?? ?? ?? 0F 29 0D ?? ?? ?? ?? 0F 28 0D ?? ?? ?? ?? 49 63 D6", "MGS2: Screenspace Fixes: okajima\\effect\\rain_cm.c -> NewRainCamera() -> _mat"))
        {
            g_pRainCamMat = reinterpret_cast<FMATRIX*>(Memory::GetRipRelativeAddress(pMatWrite, 3, 7));

            MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? F3 44 0F 10 45 ?? B8", "MGS2: Screenspace Fixes: okajima\\effect\\rain_cm.c -> Act()", {
                    memcpy(g_pRainCamMat, &g_GameVars.DG_Chanl(0)->eye_pers_no_offset, sizeof(FMATRIX));
                          });
        }



        uint8_t* pGateFrom = Memory::PatternScan(baseModule, "F3 41 0F 10 74 C5", "MGS2: Screenspace Fixes: okajima\\demo_effect\\d_rain_cm.c -> NewRainCamera_Demo() -> Act()+gate_from");
        uint8_t* pGateTo = Memory::PatternScan(baseModule, "0F 54 E7 F3 0F 58 F3", "MGS2: Screenspace Fixes: okajima\\demo_effect\\d_rain_cm.c -> NewRainCamera_Demo() -> Act()+gate_to");

        if (pGateFrom && pGateTo)
        {
            g_pRainDemoGateTo = pGateTo + 7;
            h_RainCameraDemoGate = safetyhook::create_mid(pGateFrom + 10, RainCameraDemo_LightDir_hook);
        }







#pragma region demo_lens_flare

        MAKE_HOOK_MID(baseModule, "44 0F 29 84 24 ?? ?? ?? ?? 44 0F 29 94 24 ?? ?? ?? ?? 44 0F 29 9C 24", "MGS 2: Screenspace Fixes: user\\okajima\\demo\\lens_flare.c -> Act()", {
                RETARGET_STRUCT_ENTRY(ctx.rcx, DG_CHANL, eye_pers, eye_pers_no_offset);
            }); 


        uint8_t* lensFlareRamp = Memory::PatternScan(baseModule,
                                                     "8B 05 ?? ?? ?? ?? FF C0 66 0F 6E C0 0F 5B C0 F3 0F 59 83 ?? ?? ?? ?? 44 39 93",
                                                     "MGS 2: Screenspace Fixes: user\\shibata\\demo\\lens_flare.c -> Act()");
        if (lensFlareRamp)
        {
            // The hook sits past the matched bytes; make sure it lands on the expected instruction
            // so a changed exe fails cleanly instead of hooking mid-instruction.
            if (memcmp(lensFlareRamp + 0x106, "\x44\x0F\x2F\xC0", 4) != 0 ||    // comiss xmm8, xmm0
                memcmp(lensFlareRamp + 0x166, "\xF3\x44\x0F\x5C\xC1", 5) != 0 || // subss xmm8, xmm1
                memcmp(lensFlareRamp + 0x180, "\xF3\x44\x0F\x5E", 4) != 0 ||    // divss xmm10
                memcmp(lensFlareRamp + 0x1C3, "\x48\x8B\xCB", 3) != 0)          // mov rcx, rbx
            {
                spdlog::error("MGS 2: Screenspace Fixes: lens flare hook offsets no longer match; flare fixes disabled.");
                lensFlareRamp = nullptr;
            }
        }
        if (lensFlareRamp)
        {
            static SafetyHookMid lensFlareSunStateHook {};
            lensFlareSunStateHook = safetyhook::create_mid(lensFlareRamp + 0xEA,   // post-projection: sun screen/clip pos
                                                          [](SafetyHookContext& ctx)
                                                          {
                                                              MGS2FlareOcclusion::SetSunState(reinterpret_cast<const float*>(ctx.rbx + 0x60),
                                                                                              ctx.xmm13.f32[0], ctx.xmm14.f32[0],
                                                                                              ctx.xmm0.f32[0], ctx.xmm1.f32[0]);
                                                          });
            LOG_HOOK(lensFlareSunStateHook, "MGS 2: Screenspace Fixes: lens flare sun state feed")

            static SafetyHookMid lensFlareAlphaHook {};
            lensFlareAlphaHook = safetyhook::create_mid(lensFlareRamp + 0x1C3,     // xmm9 = final sprite alpha
                                                         [](SafetyHookContext& ctx)
                                                         {
                                                             ctx.xmm9.f32[0] *= MGS2FlareOcclusion::GetVisibility();
                                                         });
            LOG_HOOK(lensFlareAlphaHook, "MGS 2: Screenspace Fixes: lens flare GS alpha")
                




            // Run the cull and edge-fade in the original 4:3 screen space so flares only cull once they leave
            // where the PS2 frame would have been, not 33% early at the cropped edge.
            constexpr float kCinemaCrop = (16.0f / 9.0f) / (4.0f / 3.0f);
            static SafetyHookMid lensFlareCullHook {};
            lensFlareCullHook = safetyhook::create_mid(lensFlareRamp + 0x106,      // xmm0 = cull threshold (1.4)
                                                       [](SafetyHookContext& ctx) { ctx.xmm0.f32[0] *= kCinemaCrop; });
            LOG_HOOK(lensFlareCullHook, "MGS 2: Screenspace Fixes: lens flare cull")
                static SafetyHookMid lensFlareEdgeStartHook {};
            lensFlareEdgeStartHook = safetyhook::create_mid(lensFlareRamp + 0x166, // xmm1 = edge-fade start (1.0)
                                                            [](SafetyHookContext& ctx) { ctx.xmm1.f32[0] *= kCinemaCrop; });
            LOG_HOOK(lensFlareEdgeStartHook, "MGS 2: Screenspace Fixes: lens flare edge-fade start")
                static SafetyHookMid lensFlareEdgeBandHook {};
            lensFlareEdgeBandHook = safetyhook::create_mid(lensFlareRamp + 0x180,  // xmm0 = edge-fade band (0.3)
                                                           [](SafetyHookContext& ctx) { ctx.xmm0.f32[0] *= kCinemaCrop; });
            LOG_HOOK(lensFlareEdgeBandHook, "MGS 2: Screenspace Fixes: lens flare edge-fade band")
            
        }
#pragma endregion

    
    
    
    
    
    }









}
