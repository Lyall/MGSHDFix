#include "common.hpp"
#include "gamevars.hpp"
#include "effect_speeds.hpp"
#include <spdlog/spdlog.h>

#define PS2_IOP_CLOCKSPEED 36.864

/////////////////////////////////////////////////////////////////
/// Corrects various visual effects in MGS2 which were
/// hardcoded for the PS2's cutscene physics speed (30 FPS)
/// and have been running at double speed in all ports since
/// the 2002 Xbox release.
/// 
/// SolidusFireAct & CreateDebrisTexture fixes originally made as
/// part of a modding fix bounty claimed by Cipherxof/Triggerhappy
/// and originally included in the MGSFPSUnlock mod.
/// It has been modified to use RTC timesteps for 1:1 PS2 accurate frame-timing.
/// 
/// Overlapping integrations originating from MGSFPSUnlock are 
/// automatically disabled so that MGSFPSUnlock's unlocked 
/// framerate support can take priority.
///
/// Please submit any new additions to this file to their
/// codebase as well so there's parity in functionality
/// at 144FPS & beyond. <3
/////////////////////////////////////////////////////////////////

SafetyHookInline solidusFireDashAct_hook {};
int64_t __fastcall MGS2_solidusFireDashAct(int64_t work)
{
    if (!g_GameVars.InCutscene()) // only slow down during cutscenes. boss fight (which includes pad demos) runs at normal gamespeed.
        return solidusFireDashAct_hook.fastcall<int64_t>(work);

    std::chrono::time_point<std::chrono::high_resolution_clock> current_time = std::chrono::high_resolution_clock::now();
    if (current_time >= g_EffectSpeedFix.solidusDashAct_NextUpdate + std::chrono::seconds(2))
    {
        g_EffectSpeedFix.solidusDashAct_NextUpdate = current_time;
    }

    if (current_time >= g_EffectSpeedFix.solidusDashAct_NextUpdate)
    {
        g_EffectSpeedFix.solidusDashAct_NextUpdate = g_EffectSpeedFix.solidusDashAct_NextUpdate + std::chrono::microseconds(static_cast<int64_t>(std::chrono::microseconds::period::den / 31.5));
        return solidusFireDashAct_hook.fastcall<int64_t>(work);
    }

    return 0;
}

SafetyHookInline createDebrisTex_hook {};
int64_t __fastcall MGS2_createDebrisTex(int64_t a1, float* a2, float* a3, unsigned int a4, int a5, int a6, float a7)
{
    auto result = createDebrisTex_hook.fastcall<int64_t>(a1, a2, a3, a4, a5, a6, a7);
    // Adjust duration based on cutscene
    int effect_duration = 150;
    
    // Check current stage to adjust effect duration
    if (strcmp(g_GameVars.GetCurrentStage(), "d012p01") == 0)
    {
        // P012_01_P01 Fortune encounter 1 polygon demo 1 (BC connecting bridge - Fortune vs Seals encounter)
        effect_duration = 300;
    }
    else if (strcmp(g_GameVars.GetCurrentStage(), "d12t1") == 0)
    {
        // T12a1D The Seizure of Metal gear Demo (liquid ocelot first encounter)
        effect_duration = 300;
    }
    
    // Apply the effect duration
    *(int*)(a1 + 100) = effect_duration;
    return result;
}

SafetyHookInline splashSplash_hook {};
int64_t __fastcall MGS2_splashSplash(struct _exception* a1)
{
    return 120;
}


SafetyHookInline splashPartsSlow_hook {};
int64_t __fastcall MGS2_splashPartsSlow(DWORD* a1, __int16* a2, float duration)
{
#ifdef cutscenes
    if (strcmp(currentStage, "d001p01") == 0)
    {
        return splashPartsSlow_hook.fastcall<int64_t>(a1, a2, duration / 2);
    }
    if (strcmp(currentStage, "d13t") == 0)
    {
        return splashPartsSlow_hook.fastcall<int64_t>(a1, a2, duration / 2);
    }
#endif
    return splashPartsSlow_hook.fastcall<int64_t>(a1, a2, duration);
}

void EffectSpeedFix::Initialize() const
{
    if (!(eGameType & MGS2))
    {
        return;
    }


    if (IsDebuggerPresent())
    {
        /*
        if (uint8_t* Tidal4Result = Memory::PatternScan(baseModule, "F3 0F 58 83 ?? ?? ?? ?? F3 0F 11 83 ?? ?? ?? ?? 41 0F 28 C3", "MGS 2: Effect Speed Fix : effect\\tidal4.c", NULL, NULL))
        {
            static SafetyHookMid Tidal4MidHook {};
            Tidal4MidHook = safetyhook::create_mid(Tidal4Result,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm0.f32[0] *= 2;
                });
            LOG_HOOK(Tidal4MidHook, "MGS 2: Effect Speed Fix: effect2\\tidal4.c 1", NULL, NULL)
        }
        */
        /*
        if (uint8_t* MGS2_splushSurfaceGravityManScanResult = Memory::PatternScan(baseModule, "F3 0F 11 43 ?? 45 8D 41", "MGS 2: Effect Speed Fix : effect2\\splush_surface_gravity_man.c 1", NULL, NULL))
        { // 2025-05-19 14-32-21
            static SafetyHookMid splushSurfaceGravityMan1MidHook {};
            splushSurfaceGravityMan1MidHook = safetyhook::create_mid(MGS2_splushSurfaceGravityManScanResult,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm0.f32[0] /= 2;
                });
            LOG_HOOK(splushSurfaceGravityMan1MidHook, "MGS 2: Effect Speed Fix: effect2\\splush_surface_gravity_man.c 1", NULL, NULL)
        }
        */
        /*
        if (uint8_t* MGS2_splushSurfaceGravityMan2ScanResult = Memory::PatternScan(baseModule, "F3 0F 11 43 ?? E8 ?? ?? ?? ?? F3 0F 58 43 ?? 41 B9 ?? ?? ?? ?? 48 8D 15", "MGS 2: Effect Speed Fix : effect2\\splush_surface_gravity_man.c 2", NULL, NULL))
        {
            static SafetyHookMid splushSurfaceGravityManMidHook {};
            splushSurfaceGravityManMidHook = safetyhook::create_mid(MGS2_splushSurfaceGravityMan2ScanResult,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm0.f32[0] /= 2;
                });
            LOG_HOOK(splushSurfaceGravityManMidHook, "MGS 2: Effect Speed Fix: effect2\\splush_surface_gravity_man.c 2", NULL, NULL)
        }
        */

        /*
        if (uint8_t* MGS2_splashPartsSlowScanResult = Memory::PatternScan(baseModule, "0F 28 D6 E8 ?? ?? ?? ?? 41 8B 06", "MGS 2: Effect Speed Fix : demo_effect\\d_splash_parts_slow.c", NULL, NULL))
        {
            uintptr_t MGS2_splashPartsSlowScanAddress = Memory::GetAbsolute((uintptr_t)MGS2_splashPartsSlowScanResult + 0x4);
            splashPartsSlow_hook = safetyhook::create_inline(reinterpret_cast<void*>(MGS2_splashPartsSlowScanAddress), reinterpret_cast<void*>(MGS2_splashPartsSlow));
            LOG_HOOK(splashPartsSlow_hook, "MGS 2: Effect Speed Fix: demo_effect\\d_splash_parts_slow.c", NULL, NULL)
        }
        */
    }


    if (uint8_t* MGS2_flyingSmokeSlowScanResult = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? FF 4B ?? 83 7B ?? ?? 7D", "MGS 2: Effect Speed Fix : effect3\\flying_smoke_slow.c", NULL, NULL))
    {
        static SafetyHookMid flyingSmokeSlow_MidHook {};
        flyingSmokeSlow_MidHook = safetyhook::create_mid(MGS2_flyingSmokeSlowScanResult,
            [](SafetyHookContext& ctx)
            {
                spdlog::info("duration before {}", ctx.r8);
                ctx.r8 = (unsigned int)(ctx.r8 * g_GameVars.ActorWaitMultiplier() * (g_GameVars.InCutscene() ? 2 : 1)); // double the duration in cutscenes
                spdlog::info("duration after {}", ctx.r8);
            });
        LOG_HOOK(flyingSmokeSlow_MidHook, "MGS 2: Effect Speed Fix: effect3\\flying_smoke_slow.c", NULL, NULL)
    }

    if (Util::CheckForASIFiles("MGSFPSUnlock", false, false))
    {
        spdlog::info("MGS 2: Effect Speed Fix: MGSFPSUnlock detected, disabling.");
        return;
    }

    if (uint8_t* MGS2_createDebrisTexOffset = Memory::PatternScan(baseModule, "44 6B C0 0F 8B CB",
        "MGS 2: Effect Speed Fix: demo\\debris_tex.c\\CreateDebrisTexture()", nullptr, nullptr))
    {
        /*
        createDebrisTex_hook;
        MGS2_createDebrisTex
        */
        Memory::PatchBytes((uintptr_t)MGS2_createDebrisTexOffset + 0x3, "\x1E", 1);
        spdlog::info("MGS 2: Effect Speed Fix: demo\\debris_tex.c\\CreateDebrisTexture() patched.");
    }

    if (uint8_t* MGS2_solidusFireDashActScanResult = Memory::PatternScan(baseModule, "?? ?? ?? ?? ?? 49 8D AB 68 FE FF FF 48 81 EC 88", "MGS 2: Effect Speed Fix : effect\\solidas_dash_fire.c", NULL, NULL))
    {
        solidusFireDashAct_hook = safetyhook::create_inline(reinterpret_cast<void*>(MGS2_solidusFireDashActScanResult), reinterpret_cast<void*>(MGS2_solidusFireDashAct));
        LOG_HOOK(solidusFireDashAct_hook, "MGS 2: Effect Speed Fix: effect\\solidas_dash_fire.c", NULL, NULL)
    }

}
