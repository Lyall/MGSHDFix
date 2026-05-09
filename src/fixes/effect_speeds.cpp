// ReSharper disable CppClangTidyModernizeRawStringLiteral
#include "stdafx.h"

#include "common.hpp"

#include "effect_speeds.hpp"
#include "gamevars.hpp"
#include "logging.hpp"


/////////////////////////////////////////////////////////////////
/// Corrects various visual effects in MGS2 which were
/// hardcoded for the PS2's cutscene physics speed (30 FPS)
/// and run at 2x with the HDC/MC's 60 FPS cutscene physics speed
/// 
/// SolidusFireAct & CreateDebrisTexture fixes originally made as
/// part of a modding fix bounty claimed by Cipherxof/Triggerhappy
/// and originally included in the MGSFPSUnlock mod.
/// They have been updated to fix several bugs, and upgraded (where needed) 
/// to use RTC timesteps for 1:1 PS2 accurate frame-timing.
/// 
/////////////////////////////////////////////////////////////////


#define CUTSCENE_FRAMESKIP_TICK(name) \
    if (name##_first_hit)             \
    {                                 \
        name##_skip = !name##_skip;   \
    }

#define CUTSCENE_FRAMESKIP_RESET(name) \
    do                                 \
    {                                  \
        name##_first_hit = false;      \
        name##_skip = false;           \
    } while (0)

#define CUTSCENE_FRAMESKIP_VARS(name)                            \
    bool name##_first_hit = false;                               \
    bool name##_skip = false;                                    \
    uintptr_t name##_copyback_addr = static_cast<uintptr_t>(0);

#define CUTSCENE_FRAMESKIP_MIDHOOK(name, ctx) \
    do                                     \
    {                                      \
        if (!g_GameVars.InCutscene())      \
        {                                  \
            return;                        \
        }                                  \
                                           \
        if (name##_skip)                   \
        {                                  \
            ctx.rip = name##_copyback_addr; \
        }                                  \
                                           \
        name##_first_hit = true;           \
    } while (0)

namespace
{

    ///PS2's I/O subprocessor clock speed in MHz. If an effect ran via a loop on the PS2, it was likely limited by this clock speed as opposed to running at 30 FPS.
    constexpr double PS2_IOP_CLOCKSPEED = 36.864;
    constexpr double FRAME_IOP_DIVIDER = (PS2_IOP_CLOCKSPEED / 60);
    constexpr double FRAME_IOP_MULTIPLIER = (60 / PS2_IOP_CLOCKSPEED);


    CUTSCENE_FRAMESKIP_VARS(rain_slow);
    CUTSCENE_FRAMESKIP_VARS(NewSplashPartsSlow_Demo);
    CUTSCENE_FRAMESKIP_VARS(SPH_ActBrkVol1);
    CUTSCENE_FRAMESKIP_VARS(NewSplushSurfaceMan);
    CUTSCENE_FRAMESKIP_VARS(NewSplushSurface2Man);

}

/// Called every frame during Present()
void EffectSpeedFix::Tick()
{
    CUTSCENE_FRAMESKIP_TICK(rain_slow);
    CUTSCENE_FRAMESKIP_TICK(NewSplashPartsSlow_Demo);
    CUTSCENE_FRAMESKIP_TICK(SPH_ActBrkVol1);
    CUTSCENE_FRAMESKIP_TICK(NewSplushSurfaceMan);
    CUTSCENE_FRAMESKIP_TICK(NewSplushSurface2Man);


}


///Called on GameVars::OnLevelTransition() to reset counters between cutscenes/levels.
void EffectSpeedFix::Reset()
{
    CUTSCENE_FRAMESKIP_RESET(rain_slow);
    CUTSCENE_FRAMESKIP_RESET(NewSplashPartsSlow_Demo);
    CUTSCENE_FRAMESKIP_RESET(SPH_ActBrkVol1);
    CUTSCENE_FRAMESKIP_RESET(NewSplushSurfaceMan);
    CUTSCENE_FRAMESKIP_RESET(NewSplushSurface2Man);

    iDebrisIteration = 0;
}


SafetyHookInline solidusFireDashAct_hook {};
int64_t __fastcall MGS2_solidusFireDashAct(int64_t work)
{
    if (!g_GameVars.InCutscene()) // only slow down during cutscenes. the boss fight (which includes pad demos/scripted sequences) runs properly at normal game speed.
    {
        return solidusFireDashAct_hook.fastcall<int64_t>(work);
    }


    std::chrono::time_point<std::chrono::high_resolution_clock> current_time = std::chrono::high_resolution_clock::now();
    if (current_time >= g_EffectSpeedFix.solidusDashAct_NextUpdate)
    {
        if (current_time >= g_EffectSpeedFix.solidusDashAct_NextUpdate + std::chrono::seconds(2)) // Reset the next update timer if we're in a new cutscene.
        {
            g_EffectSpeedFix.solidusDashAct_NextUpdate = current_time;
        }

        constexpr double duration = (PS2_IOP_CLOCKSPEED - 1);
        /*if (strcmp(g_GameVars.GetCurrentStage(), "d045p01") != 0) //P045_01P01 enter the Harrier 1 polygon demo 1 (MC) - Connecting bridge between Shells 1 and 2
        {
            duration -= 1; // Slightly slower than PS2_IOP_CLOCKSPEED to account for particle related performance slowdown on PS2 hardware had during closeup shots.
        }*/
        g_EffectSpeedFix.solidusDashAct_NextUpdate += std::chrono::microseconds(static_cast<int64_t>(std::chrono::microseconds::period::den / duration));
        return solidusFireDashAct_hook.fastcall<int64_t>(work);
    }

    return 0;
}



SafetyHookInline MGS2_SPH_ActBrkVol1_hook {};
static void MGS2_SPH_ActBrkVol1_Act_227(int64_t work)
{
    if (g_GameVars.InCutscene() && SPH_ActBrkVol1_skip)
    {
        return;
    }
    SPH_ActBrkVol1_first_hit = true;
    MGS2_SPH_ActBrkVol1_hook.call(work);
}


SafetyHookInline MGS2_NewSplushSurfaceMan_hook {};
static void MGS2_NewSplushSurfaceMan_Act_392(int64_t work)
{
    if (g_GameVars.InCutscene() && NewSplushSurfaceMan_skip)
    {
        return;
    }
    NewSplushSurfaceMan_first_hit = true;
    MGS2_NewSplushSurfaceMan_hook.call(work);
}


SafetyHookInline MGS2_NewSplushSurface2Man_hook {};
static void MGS2_NewSplushSurface2Man_Act_465(int64_t work)
{
    if (g_GameVars.InCutscene() && NewSplushSurface2Man_skip)
    {
        return;
    }
    NewSplushSurface2Man_first_hit = true;
    MGS2_NewSplushSurface2Man_hook.call(work);
}

/*
SafetyHookInline MGS2_d_splash_parts_slow_hook {};
static void MGS2_d_splash_parts_slow_Act_424(int64_t work)
{
    if (g_GameVars.InCutscene() && NewSplashPartsSlow_Demo_skip)
    {
        return;
    }
    NewSplashPartsSlow_Demo_first_hit = true;
    MGS2_d_splash_parts_slow_hook.call(work);

}

SafetyHookInline MGS2_NewSplashPartsSlow_Demo_hook {};
static void MGS2_NewSplashPartsSlow_Demo_Act_424(int64_t work)
{
    if (g_GameVars.InCutscene() && NewSplashPartsSlow_Demo_skip)
    {
        return;
    }
    NewSplashPartsSlow_Demo_first_hit = true;
    MGS2_NewSplashPartsSlow_Demo_hook.call(work);

}*/

safetyhook::MidHook debrisVelocityHook;

void EffectSpeedFix::Initialize()
{
    if (!(eGameType & MGS2)) //current limited to MGS2 in initsubsys.
    {
        return;
    }

    if (!g_EffectSpeedFix.isEnabled)
    {
        SPDLOG_INFO("MGS 2: Effect Speed Fix: Config disabled. Skipping");
        return;
    }


#pragma region D00A

    uint8_t* MGS2_RainSlowBackScanResult = Memory::PatternScan(baseModule, "48 8B 4D ?? 48 33 CC E8 ?? ?? ?? ?? 4C 8D 9C 24 ?? ?? ?? ?? 49 8B 5B ?? 45 0F 28 4B ?? 49 8B E3 41 5D", "MGS 2: Effect Speed Fix : rain_slow.c - return address");
    rain_slow_copyback_addr = reinterpret_cast<uintptr_t>(MGS2_RainSlowBackScanResult);

    //rain length is calculated as distance traveled since last frame via DG_COPY_VEC(last_frame_cam_pos, currentcam_pos) @ L250
    //ergo, scaling vel*0.5 directly results in the rain size also being reduced by 50%.
    if (rain_slow_copyback_addr)
    {
        MAKE_HOOK_MID(baseModule, "?? ?? ?? B8 ?? ?? ?? ?? 41 B9 ?? ?? ?? ?? 2B 81 ?? ?? ?? ?? 89 81 ?? ?? ?? ?? 45 8D 41 ?? ?? ?? ?? ?? B8", "MGS 2: Effect Speed Fix : user\\okajima\\effect\\rain_slow.c -> NewRainSlow() - Frameskip", {
                CUTSCENE_FRAMESKIP_MIDHOOK(rain_slow, ctx);
            });
    }
    else
    {
        spdlog::error("MGS 2: Effect Speed Fix : rain_slow.c - Failed to find rain_slow copyback address, rain_slow.c frameskip is disabled.");
    }


    MGS2_SPH_ActBrkVol1_hook = safetyhook::create_inline(reinterpret_cast<void*>(Memory::PatternScan(baseModule, "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B F9 45 33 C0", "MGS 2: Effect Speed Fix : user\\morita\\splash\\splash.c -> SPH_ActBrkVol1()")), MGS2_SPH_ActBrkVol1_Act_227);
    LOG_HOOK(MGS2_SPH_ActBrkVol1_hook, "MGS 2: Effect Speed Fix : user\\morita\\splash\\splash.c -> SPH_ActBrkVol1()")



    MGS2_NewSplushSurfaceMan_hook = safetyhook::create_inline(reinterpret_cast<void*>(Memory::PatternScan(baseModule, "48 8B C4 48 89 48 ?? 41 55", "MGS 2: Effect Speed Fix : user\\okajima\\effect2\\splush_surface_man.c -> NewSplushSurfaceMan()")), MGS2_NewSplushSurfaceMan_Act_392);
    LOG_HOOK(MGS2_NewSplushSurfaceMan_hook, "MGS 2: Effect Speed Fix : user\\okajima\\effect2\\splush_surface_man.c -> NewSplushSurfaceMan()")


    MGS2_NewSplushSurface2Man_hook = safetyhook::create_inline(reinterpret_cast<void*>(Memory::PatternScan(baseModule, "48 89 5C 24 ?? 48 89 74 24 ?? 48 89 4C 24", "MGS 2: Effect Speed Fix : user\\okajima\\effect2\\splush_surface_gravity_man.c -> NewSplushSurface2Man()")), MGS2_NewSplushSurface2Man_Act_465);
    LOG_HOOK(MGS2_NewSplushSurface2Man_hook, "MGS 2: Effect Speed Fix : user\\okajima\\effect2\\splush_surface_gravity_man.c -> NewSplushSurface2Man()")





        
        /*
    MGS2_NewSplashPartsSlow_Demo_hook = safetyhook::create_inline(reinterpret_cast<void*>(Memory::PatternScan(baseModule, "40 56 57 48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 ?? 48 8B F1", "MGS 2: Effect Speed Fix : NewSplashPartsSlow_Demo")), MGS2_NewSplashPartsSlow_Demo_Act_424);
    LOG_HOOK(MGS2_NewSplashPartsSlow_Demo_hook, "MGS 2: Effect Speed Fix : NewSplashPartsSlow_Demo.c")
    
    uintptr_t MGS2_d_splash_parts_slowScanResult = Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? ?? ?? ?? 48 83 C5 ?? 89 43", "MGS 2: Effect Speed Fix : d_splash_parts_slow") + 1);
    MGS2_d_splash_parts_slowScanResult = Memory::GetAbsolute(MGS2_d_splash_parts_slowScanResult + 0x4D);
    MGS2_d_splash_parts_slow_hook = safetyhook::create_inline(reinterpret_cast<void*>(MGS2_d_splash_parts_slowScanResult), MGS2_d_splash_parts_slow_Act_424);
    LOG_HOOK(MGS2_d_splash_parts_slow_hook, "MGS 2: Effect Speed Fix : d_splash_parts_slow.c\\Act_424()")
    */
#pragma endregion






    
    if (uint8_t* MGS2_flyingSmokeSlowScanResult = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? FF 4B ?? 83 7B ?? ?? 7D", "MGS 2: Effect Speed Fix : effect3\\flying_smoke_slow.c"))
    {
        static SafetyHookMid flyingSmokeSlow_MidHook {};
        flyingSmokeSlow_MidHook = safetyhook::create_mid(MGS2_flyingSmokeSlowScanResult,
            [](SafetyHookContext& ctx)
            {
                //spdlog::info("flying_smoke_slow before {}", reghelpers::Getr8d(ctx));
                reghelpers::set_r8d(ctx, static_cast<unsigned int>((g_GameVars.ActorWaitMultiplier() * (g_GameVars.InCutscene() ? 2.0 : 1.0)) * reghelpers::get_r8d(ctx)));
#ifdef _MGSDEBUGGING
                spdlog::info("flying_smoke_slow after {}", reghelpers::get_r8d(ctx));
#endif
            });
        LOG_HOOK(flyingSmokeSlow_MidHook, "MGS 2: Effect Speed Fix: effect3\\flying_smoke_slow.c")
    }

    if (Util::CheckForASIFiles("MGSFPSUnlock", false, false, "2025-05-25"))
    {
        spdlog::info("MGS 2: Effect Speed Fix: Outdated version of MGSFPSUnlock detected, Large explosion & Solidus's Firedash fixes are disabled.");
        return;
    }

    if (uint8_t* MGS2_DEMO_CreateDebrisTex_SetupResult = Memory::PatternScan(baseModule,"F3 0F 58 43 ?? 48 83 C6","MGS2_DEMO_CreateDebrisTex_Setup velocity"))
    {
        debrisVelocityHook = safetyhook::create_mid(MGS2_DEMO_CreateDebrisTex_SetupResult,
        [](SafetyHookContext& ctx)
            {
                if (g_GameVars.IsStage(MGS2Stages::D12T3)) // T12a1D The Seizure of Metal gear Demo (liquid ocelot first encounter)
                {
                    switch (g_EffectSpeedFix.iDebrisIteration) //28 total, last 3 are at the end.
                    {
                    case 1:
                    case 2:
                    case 3:
                        ctx.xmm0.f32[0] /= 4.0f;
                        break;
                    case 15:
                    case 16:
                    case 17:
                    case 18:
                    case 26:
                    case 27:
                    case 28:
                        ctx.xmm0.f32[0] /= 2.0f;
                        break;
                    default:
                        break;
                    }
                    
                }
                else if (g_GameVars.IsStage(MGS2Stages::D012P01)) // P012_01_P01 Fortune encounter 1 polygon demo 1 (BC connecting bridge - Fortune vs Seals encounter)
                {
                    ctx.xmm0.f32[0] /= 18.0f;
                }
                else
                {
                    ctx.xmm0.f32[0] /= 2.0f;
                }
            }
        );
        LOG_HOOK(debrisVelocityHook, "MGS 2: Effect Speed Fix: demo\\debris_tex.c\\CreateDebrisTexture velocity");
    }

    
    if (uint8_t* MGS2_createDebrisTexOffset = Memory::PatternScan(baseModule, "45 89 46 ?? E8", "MGS 2: Effect Speed Fix : demo\\debris_tex.c\\CreateDebrisTexture()"))
    {
        static SafetyHookMid MGS2_createDebrisTexMidHook {};
        MGS2_createDebrisTexMidHook = safetyhook::create_mid(MGS2_createDebrisTexOffset,
            [](SafetyHookContext& ctx)
            {
                g_EffectSpeedFix.iDebrisIteration++;
                g_EffectSpeedFix.iExplosionDuration = 75.0 * FRAME_IOP_MULTIPLIER; //default to double

                /*if (strcmp(g_GameVars.GetCurrentStage(), "d12t3") == 0) // T12a1D The Seizure of Metal gear Demo (liquid ocelot first encounter)
                {                    
                    switch (g_EffectSpeedFix.iDebrisIteration) //28 total, last 3 are at the end.
                    {
                        case 1:
                        case 2:
                        case 3:
                            g_EffectSpeedFix.iExplosionDuration *= FRAME_IOP_MULTIPLIER * 7;
                            break;
                        case 15:
                        case 16:
                        case 18: //double check if 17 or 18 - is it the left one or the black rubble. black rubble needs to be the shorter one.
                            g_EffectSpeedFix.iExplosionDuration *= FRAME_IOP_MULTIPLIER * 10;
                            break;
                        case 26:
                        case 27:
                        case 28:
                            g_EffectSpeedFix.iExplosionDuration *= FRAME_IOP_MULTIPLIER * 10;
                            break;
                        default:
                            //std::string CountString = "Explosion" + std::to_string(g_EffectSpeedFix.iDebrisIteration);
                            //inipp::get_value(ini.sections["Debug"], CountString, g_EffectSpeedFix.iExplosionDuration);
                            break;
                    }

                }
                else */if (g_GameVars.IsStage(MGS2Stages::D012P01))
                {
                    // P012_01_P01 Fortune encounter 1 polygon demo 1 (BC connecting bridge - Fortune vs Seals encounter)
                    g_EffectSpeedFix.iExplosionDuration *= static_cast<int>(FRAME_IOP_MULTIPLIER) * 10;
                }
                
#ifdef _MGSDEBUGGING
                spdlog::info("CreateDebrisTexture before {}. Config target: {}, Iteration: {}, Stage: {}", reghelpers::get_r8d(ctx), static_cast<int>(g_EffectSpeedFix.iExplosionDuration), g_EffectSpeedFix.iDebrisIteration, g_GameVars.GetCurrentStage());
#endif
                reghelpers::set_r8d(ctx, static_cast<int>(g_EffectSpeedFix.iExplosionDuration));


            });
        LOG_HOOK(MGS2_createDebrisTexMidHook, "MGS 2: Effect Speed Fix: demo\\debris_tex.c\\CreateDebrisTexture()")
 
    }

    if (uint8_t* MGS2_solidusFireDashActScanResult = Memory::PatternScan(baseModule, "?? ?? ?? ?? ?? 49 8D AB 68 FE FF FF 48 81 EC 88", "MGS 2: Effect Speed Fix : effect\\solidas_dash_fire.c"))
    {
        solidusFireDashAct_hook = safetyhook::create_inline(reinterpret_cast<void*>(MGS2_solidusFireDashActScanResult), reinterpret_cast<void*>(MGS2_solidusFireDashAct));
        LOG_HOOK(solidusFireDashAct_hook, "MGS 2: Effect Speed Fix: effect\\solidas_dash_fire.c")
    }
    
}


////////
///     old tests on broken shit. need to redo these properly. :3
/*
#ifdef _MGSDEBUGGING
    /*
    if (uint8_t* MGS2_traffic_c_Result = Memory::PatternScan(baseModule, "89 53 ?? 33 C9", "MGS 2: Effect Speed Fix : demo\\traffic.c"))
    {

    /*
    if (uint8_t* MGS2_traffic_c_2_Result = Memory::PatternScan(baseModule, "41 8B F9 0F 29 74 24 ?? 45 33 C9 45 8B F0", "MGS 2: Effect Speed Fix : demo\\traffic.c #2"))
    {


    *//*
    if (uint8_t* MGS2_crosfade_c_Result = Memory::PatternScan(baseModule, "89 5F ?? 79 ?? 89 77", "MGS 2: Effect Speed Fix : effect1\\crosfade.c"))
    {


    /*
    if (uint8_t* Tidal4Result = Memory::PatternScan(baseModule, "F3 0F 58 83 ?? ?? ?? ?? F3 0F 11 83 ?? ?? ?? ?? 41 0F 28 C3", "MGS 2: Effect Speed Fix : effect\\tidal4.c"))
    {


    if (uint8_t* MGS2_splushSurfaceGravityManScanResult = Memory::PatternScan(baseModule, "F3 0F 11 43 ?? 45 8D 41", "MGS 2: Effect Speed Fix : effect2\\splush_surface_gravity_man.c 1"))


   /* if (uint8_t* MGS2_splushSurfaceGravityMan2ScanResult = Memory::PatternScan(baseModule, "F3 0F 11 05 ?? ?? ?? ?? F3 0F 11 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 0F 10 46", "MGS 2: Effect Speed Fix : effect2\\splush_surface_gravity_man.c 2"))

*/
