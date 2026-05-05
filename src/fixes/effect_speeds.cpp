#include "stdafx.h"

#include "common.hpp"

#include "effect_speeds.hpp"
#include "gamevars.hpp"
#include "logging.hpp"

///PS2's I/O subprocessor clock speed in MHz. If an effect ran via a loop on the PS2, it was likely limited by this clock speed as opposed to running at 30 FPS.
constexpr double PS2_IOP_CLOCKSPEED = 36.864;
constexpr double FRAME_IOP_DIVIDER = (PS2_IOP_CLOCKSPEED / 60);
constexpr double FRAME_IOP_MULTIPLIER = (60 / PS2_IOP_CLOCKSPEED);


namespace
{
    uintptr_t rain_slow_copyback_addr = reinterpret_cast<uintptr_t>(nullptr);
}

/////////////////////////////////////////////////////////////////
/// Corrects various visual effects in MGS2 which were
/// hardcoded for the PS2's cutscene physics speed (30 FPS)
/// and have been running at double speed in all ports since
/// the 2002 Xbox release.
/// 
/// SolidusFireAct & CreateDebrisTexture fixes originally made as
/// part of a modding fix bounty claimed by Cipherxof/Triggerhappy
/// and originally included in the MGSFPSUnlock mod.
/// They have been updated to fix several bugs, and upgraded (where needed) 
/// to use RTC timesteps for 1:1 PS2 accurate frame-timing.
/// 
/// Overlapping integrations originating from MGSFPSUnlock are 
/// automatically disabled when older versions of MGSFPSUnlock are
/// detected to maintain backwards compatibility until it's updated.
///
/////////////////////////////////////////////////////////////////

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


#pragma region RAIN_EFFECTS

    uint8_t* MGS2_RainSlowBackScanResult = Memory::PatternScan(baseModule, "48 8B 4D ?? 48 33 CC E8 ?? ?? ?? ?? 4C 8D 9C 24 ?? ?? ?? ?? 49 8B 5B ?? 45 0F 28 4B ?? 49 8B E3 41 5D", "MGS 2: Effect Speed Fix : rain_slow.c - return address");
    rain_slow_copyback_addr = reinterpret_cast<uintptr_t>(MGS2_RainSlowBackScanResult);

    //rain length is calculated as distance traveled since last frame via DG_COPY_VEC(last_frame_cam_pos, currentcam_pos) @ L250
    //ergo, scaling vel*0.5 directly results in the rain size also being reduced by 50%.
    if (rain_slow_copyback_addr)
    {
        MAKE_HOOK_MID(baseModule, "?? ?? ?? B8 ?? ?? ?? ?? 41 B9 ?? ?? ?? ?? 2B 81 ?? ?? ?? ?? 89 81 ?? ?? ?? ?? 45 8D 41 ?? ?? ?? ?? ?? B8", "MGS 2: Effect Speed Fix : user\\okajima\\effect\\rain_slow.c -> NewRainSlow() - Frameskip", {
                if (!g_GameVars.InCutscene())
                {
                    return;
                }

                if (g_EffectSpeedFix.g_rain_slow_skip && rain_slow_copyback_addr)
                {
                    ctx.rip = rain_slow_copyback_addr;
                }
            });
    }
    else
    {
        spdlog::error("MGS 2: Effect Speed Fix : rain_slow.c - Failed to find rain_slow copyback address, rain_slow.c frameskip is disabled.");
    }



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

///Called on GameVars::OnLevelTransition() to reset counters between cutscenes/levels.
void EffectSpeedFix::Reset() 
{
    iDebrisIteration = 0;
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



    /*
    if (uint8_t* MGS2_splashPartsSlowScanResult = Memory::PatternScan(baseModule, "0F 28 D6 E8 ?? ?? ?? ?? 41 8B 06", "MGS 2: Effect Speed Fix : demo_effect\\d_splash_parts_slow.c"))
    *
    *
#ifdef _MGSDEBUGGING
/*
SafetyHookInline splashSplash_hook {};
int64_t __fastcall MGS2_splashSplash(struct _exception* a1)
{
    return 120;
}
*/

/*
SafetyHookInline splashPartsSlow_hook {};
int64_t __fastcall MGS2_splashPartsSlow(DWORD* a1, __int16* a2, float duration)
{
    if (strcmp(currentStage, "d001p01") == 0)
    {
        return splashPartsSlow_hook.fastcall<int64_t>(a1, a2, duration / 2);
    }
    if (strcmp(currentStage, "d13t") == 0)
    {
        return splashPartsSlow_hook.fastcall<int64_t>(a1, a2, duration / 2);
    }
    return splashPartsSlow_hook.fastcall<int64_t>(a1, a2, duration);
}
*/
