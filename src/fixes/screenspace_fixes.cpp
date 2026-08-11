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
        

        MAKE_HOOK_MID(baseModule, "44 0F 29 94 24 ?? ?? ?? ?? 44 0F 29 64 24 ?? 44 0F 29 6C 24", "MGS 2: Screenspace Fixes: user\\shibata\\effect\\plant_sun.c -> NewPlantSunMain() -> Act() -> Flare_Act() - Projection | @l273: ", {
                RETARGET_STRUCT_ENTRY(ctx.rcx, DG_CHANL, eye_pers, eye_pers_no_offset);
            });

        MAKE_HOOK_MID(baseModule, "F3 44 0F 2C D0 66 0F 1F 84 00", "MGS 2: Screenspace Fixes: user\\shibata\\effect\\plant_sun.c -> NewPlantSunMain() -> Act() -> Flare_Act() - Feedback Activity | @l331: ", {
                if (ctx.xmm0.f32[0] >= 1.0f)
                    g_plantSunLastActiveMs.store(GetTickCount64(), std::memory_order_relaxed);
            });
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

        MAKE_HOOK_MID(baseModule, "44 0F 29 84 24 ?? ?? ?? ?? 44 0F 29 94 24 ?? ?? ?? ?? 44 0F 29 9C 24", "MGS 2: Screenspace Fixes: user\\shibata\\demo\\lens_flare.c -> NewLensFlare_Demo() -> Act() - Projection | @l230: ", {
                RETARGET_STRUCT_ENTRY(ctx.rcx, DG_CHANL, eye_pers, eye_pers_no_offset);
            });

        MAKE_HOOK_MID(baseModule, "0F 2F C1 89 44 24 ?? 0F 83 ?? ?? ?? ?? F3 44 0F 10 44 24", "MGS 2: Screenspace Fixes: user\\shibata\\demo\\lens_flare.c -> NewLensFlare_Demo() -> Act() - Sun State | @l241: ", {
                MGS2FlareOcclusion::SetSunState(reinterpret_cast<const float*>(ctx.rbx + 0x60), ctx.xmm13.f32[0], ctx.xmm14.f32[0], ctx.xmm0.f32[0], ctx.xmm1.f32[0]);
            });

        MAKE_HOOK_MID(baseModule, "48 8B CB E8 ?? ?? ?? ?? 41 0F 28 E5", "MGS 2: Screenspace Fixes: user\\shibata\\demo\\lens_flare.c -> NewLensFlare_Demo() -> Act() - GS Alpha | @l259: ", {
                ctx.xmm9.f32[0] *= MGS2FlareOcclusion::GetVisibility();
            });

        // Run the cull and edge-fade in the original 4:3 screen space.
        constexpr float kCinemaCrop = (16.0f / 9.0f) / (4.0f / 3.0f);

        MAKE_HOOK_MID(baseModule, "44 0F 2F C0 0F 87 ?? ?? ?? ?? F3 0F 10 7C 24", "MGS 2: Screenspace Fixes: user\\shibata\\demo\\lens_flare.c -> NewLensFlare_Demo() -> Act() - Cull | @l241: ", {
                ctx.xmm0.f32[0] *= kCinemaCrop;
            });

        MAKE_HOOK_MID(baseModule, "F3 44 0F 5C C1 F3 0F 5E C2", "MGS 2: Screenspace Fixes: user\\shibata\\demo\\lens_flare.c -> NewLensFlare_Demo() -> Act() - Edge Fade Start | @l250: ", {
                ctx.xmm1.f32[0] *= kCinemaCrop;
            });

        MAKE_HOOK_MID(baseModule, "F3 44 0F 5E 15 ?? ?? ?? ?? 76", "MGS 2: Screenspace Fixes: user\\shibata\\demo\\lens_flare.c -> NewLensFlare_Demo() -> Act() - Edge Fade Band | @l252: ", {
                ctx.xmm0.f32[0] *= kCinemaCrop;
            });
#pragma endregion

    
    
    
    
    
    }









}
