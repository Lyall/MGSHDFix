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
/// SolidusFireAct fix originally made as
/// part of a modding fix bounty claimed by Cipherxof/Triggerhappy
/// and originally included in the MGSFPSUnlock mod.
/// They have been updated to fix several bugs, and upgraded (where needed) 
/// to use RTC timesteps for 1:1 PS2 accurate frame-timing.
/// 
/////////////////////////////////////////////////////////////////


#define MGS2_CUTSCENE_FRAMESKIP_TICK(name, isCutscene) \
    do                                            \
    {                                             \
        if ((isCutscene) && name##_first_hit)     \
        {                                         \
            name##_skip = !name##_skip;           \
        }                                         \
    } while (false)

#define MGS2_CUTSCENE_FRAMESKIP_RESET(name)      \
    do                                      \
    {                                       \
        name##_first_hit = false;           \
        name##_skip = false;                \
    } while (false)

#define MGS2_CUTSCENE_FRAMESKIP_VARS(name)       \
    bool name##_first_hit = false;          \
    bool name##_skip = false

#define MGS2_CUTSCENE_FRAMESKIP_MIDHOOK(name, ctx)    \
    do                                           \
    {                                            \
        if (!g_GameVars.InCutscene())            \
        {                                        \
            return;                              \
        }                                        \
                                                 \
        if (name##_skip)                         \
        {                                        \
            ctx.rip = name##_copyback_addr;      \
        }                                        \
                                                 \
        name##_first_hit = true;                 \
    } while (false)

#define DEFINE_MGS2_CUTSCENE_FRAMESKIP_HOOK(name)             \
    SafetyHookInline name##_hook {};                    \
    static void name##_Hook(int64_t work)                 \
    {                                                    \
        if (name##_skip && g_GameVars.InCutscene())       \
        {                                                \
            return;                                      \
        }                                                \
                                                         \
        name##_first_hit = true;                         \
        name##_hook.call(work);                          \
    }

#define CREATE_MGS2_CUTSCENE_FRAMESKIP_HOOK(name, pattern, label)                    \
    if (uint8_t* addr = Memory::PatternScan(baseModule, pattern, label))        \
    {                                                                          \
        name##_hook = safetyhook::create_inline(                                \
            reinterpret_cast<void*>(addr),                                      \
            name##_Hook);                                                       \
        LOG_HOOK(name##_hook, label)                                            \
    }

#define MGS2_CUTSCENE_FRAMESKIP_INLINE_HOOKS(X) \
    X(NewLineSmoke, "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B D9 48 8B 49 ?? 48 85 C9", "MGS 2: Effect Speed Fix : user\\shibata\\effect\\line_smoke.c -> NewLineSmoke()") \
    X(NewHexagonalPattern, "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B F9 48 8D 51 ?? 8B 49", "MGS 2: Effect Speed Fix : user\\okuta\\effect\\hexagonal.c -> NewHexagonalPattern()") \
    X(NewKamomeManager, "40 53 48 83 EC ?? 8B 41 ?? 48 8B D9 89 05", "MGS 2: Effect Speed Fix : user\\okuta\\kamome\\kmmng.c -> NewKamomeManager()") \
    X(NewFogSet_Demo, "40 53 48 83 EC ?? 83 3D ?? ?? ?? ?? 00 48 8B D9 0F 85", "MGS 2: Effect Speed Fix : user\\okajima\\demo_effect\\d_fog_set.c -> NewFogSet_Demo()") \
    X(NewRainFogPersFast, "40 55 53 41 55 41 56 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 48 8B 05", "MGS 2: Effect Speed Fix: user\\okajima\\effect3\\gas_pers_fast.c -> NewRainFogPersFast()") \
    X(NewRainFogPersDemo, "48 89 5C 24 ?? 48 89 74 24 ?? 55 57 41 57 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 ?? 48 8B F9 48 8D 54 24 ?? 8B 49 ?? E8 ?? ?? ?? ?? 48 8B 54 24 ?? 45 33 FF BE ?? ?? ?? ?? 8D 58 ?? 48 63 C3 48 C1 E0 ?? 48 03 D0 48 89 54 24 ?? 85 DB 78 ?? 0F 1F 40 ?? 48 8B 42 ?? ?? ?? 83 F9 ?? 74 ?? 85 C9 74 ?? 3B CE 74 ?? 83 F9 ?? 75 ?? 48 8B CF E8 ?? ?? ?? ?? 48 8B 54 24 ?? 48 83 EA ?? 2B DE 48 89 54 24 ?? 79 ?? 48 8B 05 ?? ?? ?? ?? 48 8B 8F ?? ?? ?? ?? 25 ?? ?? ?? ?? 48 3D ?? ?? ?? ?? 75 ?? 81 89 ?? ?? ?? ?? ?? ?? ?? ?? E9 ?? ?? ?? ?? 89 77 ?? EB ?? 44 89 7F ?? EB ?? 44 39 7F ?? 74 ?? 81 A1 ?? ?? ?? ?? ?? ?? ?? ?? 4C 89 B4 24 ?? ?? ?? ?? 41 BE ?? ?? ?? ?? 0F 29 B4 24 ?? ?? ?? ?? 0F 29 BC 24 ?? ?? ?? ?? 44 0F 29 84 24 ?? ?? ?? ?? 44 0F 29 8C 24 ?? ?? ?? ?? 44 0F 29 94 24 ?? ?? ?? ?? 44 0F 29 9C 24 ?? ?? ?? ?? 44 0F 29 A4 24 ?? ?? ?? ?? 44 0F 29 AC 24 ?? ?? ?? ?? 44 39 7F ?? 75 ?? 39 35 ?? ?? ?? ?? 75 ?? 33 D2 41 8B CE E8 ?? ?? ?? ?? 8B 05 ?? ?? ?? ?? 0F 57 F6 F3 0F 10 0D", "MGS 2: Effect Speed Fix: user\\okajima\\effect\\rain_gas_pers_demo.c -> NewRainFogPersDemo()") \
    X(MGS2_SPH_ActBrkVol1, "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B F9 45 33 C0", "MGS 2: Effect Speed Fix : user\\morita\\splash\\splash.c -> SPH_ActBrkVol1()") \
    X(NewSplushSurfaceMan, "48 8B C4 48 89 48 ?? 41 55", "MGS 2: Effect Speed Fix : user\\okajima\\effect2\\splush_surface_man.c -> NewSplushSurfaceMan()") \
    X(NewSplushSurface2Man, "48 89 5C 24 ?? 48 89 74 24 ?? 48 89 4C 24", "MGS 2: Effect Speed Fix : user\\okajima\\effect2\\splush_surface_gravity_man.c -> NewSplushSurface2Man()") \
    X(NewTraffic_Flush, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 41 56 48 83 EC ?? 48 8B 51", "MGS 2: Effect Speed Fix : NewTraffic_Flush") \
    X(NewDebris_Tex, "40 57 48 83 EC ?? 48 89 5C 24 ?? 48 8B F9 48 89 6C 24", "MGS 2: Effect Speed Fix : user\\okajima\\effect2\\debris_tex.c -> NewDebris_Tex()") \
    X(NewLinerGunPlasma, "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B 99 ?? ?? ?? ?? 48 8B F1 8B 0D", "MGS 2: Effect Speed Fix : okajima\\effect2\\liner_gun_plasma.c -> NewLinerGunPlasma() -> Act()") \
    X(NewFortSplineBulletDemo, NEW_FORT_SPLINE_BULLET_DEMO_PATTERN, "MGS 2: Effect Speed Fix : user\\morita\\demo_fort\\fort_b_line.c() -> Act()")

namespace
{

    ///PS2's I/O subprocessor clock speed in MHz. If an effect ran via a loop on the PS2, it was likely limited by this clock speed as opposed to running at 30 FPS.
    constexpr double PS2_IOP_CLOCKSPEED = 36.864;
    constexpr double FRAME_IOP_DIVIDER = PS2_IOP_CLOCKSPEED / 60.0;
    constexpr double FRAME_IOP_MULTIPLIER = 60.0 / PS2_IOP_CLOCKSPEED;

    constexpr char NEW_FORT_SPLINE_BULLET_DEMO_PATTERN[] = "48 89 5C 24 ?? 55 56 57 41 56 41 57 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 ?? 48 8B 51 ?? 33 ED 48 8B F1 4C 89 A4 24 ?? ?? ?? ?? 41 BE ?? ?? ?? ?? 44 8B FD 41 8B C6 44 8B CD 48 63 8A ?? ?? ?? ?? 44 8D 65 ?? 2B C1 48 8B BC CA ?? ?? ?? ?? 89 82 ?? ?? ?? ?? 48 98 4C 8B 84 C2 ?? ?? ?? ?? 66 0F 1F 84 00 ?? ?? ?? ?? 0F B6 57 ?? 41 88 50 ?? 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 44 3B C9 7D ?? 41 C6 40 ?? ?? B2 ?? 44 0F B6 DA EB ?? 44 0F B6 DA 44 3B C8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D8 45 88 58 ?? 0F B6 57 ?? 41 8D 59 ?? 41 88 50 ?? 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 3B D9 7D ?? 41 C6 40 ?? ?? B2 ?? 44 0F B6 D2 EB ?? 44 0F B6 D2 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D0 45 88 50 ?? 0F B6 57 ?? 41 8D 59 ?? 41 88 50 ?? 45 0A D3 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 3B D9 7D ?? 41 C6 40 ?? ?? B2 ?? 44 0F B6 DA EB ?? 44 0F B6 DA 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D8 45 88 58 ?? 0F B6 57 ?? 41 8D 59 ?? 41 88 50 ?? 45 0A DA 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 3B D9 7D ?? 41 C6 40 ?? ?? B2 ?? 44 0F B6 D2 EB ?? 44 0F B6 D2 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D0 45 88 50 ?? 0F B6 57 ?? 41 8D 59 ?? 41 88 50 ?? 45 0A D3 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 3B D9 7D ?? 41 C6 40 ?? ?? B2 ?? 44 0F B6 DA EB ?? 44 0F B6 DA 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D8 45 88 58 ?? 0F B6 57 ?? 41 8D 59 ?? 41 88 50 ?? 45 0A DA 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 3B D9 7D ?? 41 C6 40 ?? ?? B2 ?? 44 0F B6 D2 EB ?? 44 0F B6 D2 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D0 45 88 50 ?? 0F B6 57 ?? 41 8D 59 ?? 41 88 50 ?? 45 0A D3 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 3B D9 7D ?? 41 C6 40 ?? ?? B2 ?? 44 0F B6 DA EB ?? 44 0F B6 DA 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D8 45 88 58 ?? 0F B6 57 ?? 45 0A DA 41 88 50 ?? 45 8D 51 ?? 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 44 3B D1 7D ?? 41 C6 40 ?? ?? B2 ?? 0F B6 DA EB ?? 0F B6 DA 44 3B D0 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 2A D8 41 88 58 ?? 0F B6 97 ?? ?? ?? ?? 41 0A DB 41 88 90 ?? ?? ?? ?? 45 8D 59 ?? 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 44 3B D9 7D ?? 41 C6 80 ?? ?? ?? ?? ?? B2 ?? 44 0F B6 D2 EB ?? 44 0F B6 D2 44 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D0 45 88 90 ?? ?? ?? ?? 0F B6 97 ?? ?? ?? ?? 45 8D 59 ?? 41 88 90 ?? ?? ?? ?? 44 0A D3 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 44 3B D9 7D ?? 41 C6 80 ?? ?? ?? ?? ?? B2 ?? 0F B6 CA EB ?? 0F B6 CA 44 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 2A C8 41 88 88 ?? ?? ?? ?? 0F B6 C9 48 81 C7 ?? ?? ?? ?? 41 0F B6 C2 49 81 C0 ?? ?? ?? ?? 0B C8 41 83 C1 ?? 44 0B F9 41 83 F9 ?? 0F 8C ?? ?? ?? ?? 8B 46 ?? 4C 8B A4 24 ?? ?? ?? ?? 85 C0 0F 88 ?? ?? ?? ?? 4C 8B 46 ?? 0F 29 B4 24 ?? ?? ?? ?? 0F 29 BC 24 ?? ?? ?? ?? 44 0F 29 84 24 ?? ?? ?? ?? 49 63 88 ?? ?? ?? ?? F3 44 0F 10 05 ?? ?? ?? ?? 44 2B F1 44 0F 29 8C 24 ?? ?? ?? ?? F3 44 0F 10 0D ?? ?? ?? ?? 49 8B 94 C8 ?? ?? ?? ?? 44 0F 29 9C 24 ?? ?? ?? ?? F3 44 0F 10 1D ?? ?? ?? ?? 44 0F 29 A4 24 ?? ?? ?? ?? F3 44 0F 10 25 ?? ?? ?? ?? 44 0F 29 AC 24 ?? ?? ?? ?? 45 0F 57 ED 49 63 C6 45 89 B0 ?? ?? ?? ?? 4C 8D 72 ?? 44 0F 29 B4 24 ?? ?? ?? ?? F3 44 0F 10 35 ?? ?? ?? ?? 49 8B 9C C0 ?? ?? ?? ?? 49 8B BC C0 ?? ?? ?? ?? 48 83 C3 ?? 44 0F 29 7C 24 ?? F3 44 0F 10 3D ?? ?? ?? ?? 44 0F 29 94 24 ?? ?? ?? ?? 0F 1F 40 ?? 3B 6E ?? 0F 8D ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 84 C0 0F 84 ?? ?? ?? ?? 8B 46 ?? 0F 57 C0 F3 44 0F 10 15 ?? ?? ?? ?? 0F 57 F6 2B C5 0F 57 FF F3 0F 2A FD F3 0F 2A C0 48 8B 05 ?? ?? ?? ?? 0F B6 88 ?? ?? ?? ?? F3 0F 59 05 ?? ?? ?? ?? F3 0F 2A F1 F3 44 0F 5D D0 F3 41 0F 59 F1 F3 0F 59 35 ?? ?? ?? ?? F3 0F 58 F7 0F 28 C6 F3 41 0F 58 C6 E8 ?? ?? ?? ?? 41 0F 2F F1 F3 41 0F 59 C2 F3 0F 2C C0 66 89 43 ?? 76 ?? 66 66 0F 1F 84 00 ?? ?? ?? ?? F3 41 0F 58 F3 41 0F 2F F1 77 ?? 44 0F 2F C6 76 ?? F3 41 0F 58 F4 44 0F 2F C6 77 ?? 41 0F 2F F6 76 ?? 41 0F 28 C1 F3 0F 5C C6 0F 28 F0 F3 0F 10 05 ?? ?? ?? ?? 0F 2F C6 76 ?? 41 0F 28 C0 F3 0F 5C C6 0F 28 F0 0F 28 C6 E8 ?? ?? ?? ?? F3 41 0F 59 C2 F3 0F 2C C0 66 89 43 ?? 48 8B 05 ?? ?? ?? ?? 8B 88 ?? ?? ?? ?? 03 CD F6 C1 ?? 75 ?? ?? ?? 8B 46 ?? 2B C5 66 0F 6E C0 0F 5B C0 0F 2F 05 ?? ?? ?? ?? 76 ?? 41 0F 28 CD EB ?? F3 0F 59 05 ?? ?? ?? ?? F3 0F 10 0D ?? ?? ?? ?? F3 0F 5C C8 F3 0F 59 3D ?? ?? ?? ?? 4C 8D 46 ?? F3 0F 5C F9 44 0F 2F EF 76 ?? 41 0F 28 FF 41 0F 2F 78 ?? 76 ?? 49 83 C0 ?? 41 0F 2F 78 ?? 77 ?? 49 8D 50 ?? 45 33 C9 48 8D 4C 24 ?? E8 ?? ?? ?? ?? 0F 28 CF 48 8D 4C 24 ?? 4C 8B C7 E8 ?? ?? ?? ?? 8B 46 ?? 2B C5 66 0F 6E C0 0F 5B C0 F3 0F 5E 05 ?? ?? ?? ?? F3 0F 58 47 ?? F3 0F 11 47 ?? ?? ?? ?? 49 83 C6 ?? 44 0B F8 48 83 C3 ?? 48 83 C7 ?? FF C5 83 FD ?? 0F 8C ?? ?? ?? ?? 44 0F 28 7C 24 ?? 44 0F 28 B4 24 ?? ?? ?? ?? 44 0F 28 AC 24 ?? ?? ?? ?? 44 0F 28 A4 24 ?? ?? ?? ?? 44 0F 28 9C 24 ?? ?? ?? ?? 44 0F 28 94 24 ?? ?? ?? ?? 44 0F 28 8C 24 ?? ?? ?? ?? 44 0F 28 84 24 ?? ?? ?? ?? 0F 28 BC 24 ?? ?? ?? ?? 0F 28 B4 24 ?? ?? ?? ?? 45 85 FF 75 ?? 48 8B CE E8 ?? ?? ?? ?? 83 46 ?? ?? EB ?? FF C0 89 46 ?? 48 8B 4C 24 ?? 48 33 CC E8 ?? ?? ?? ?? 48 8B 9C 24 ?? ?? ?? ?? 48 81 C4 ?? ?? ?? ?? 41 5F 41 5E 5F 5E 5D C3 ?? ?? ?? ?? ?? ?? ?? ?? 48 83 EC ?? F3 0F 10 69 ?? ?? ?? ?? 0F 29 74 24 ?? 0F 28 F1 F3 0F 5C 71 ?? 0F 28 DE F3 0F 59 DE 0F 28 E3 0F 28 C3 F3 0F 59 41 ?? F3 0F 59 E6 0F 28 CC 0F 28 D4 F3 0F 59 49 ?? F3 0F 58 C8 0F 28 C6 F3 0F 59 41 ?? F3 0F 5E CD F3 0F 58 C8 0F 28 C3 F3 0F 5E CD ?? ?? ?? ?? ?? ?? ?? ?? ?? F3 0F 59 51 ?? ?? ?? ?? F3 0F 59 41 ?? F3 0F 58 D0 0F 28 C6 F3 0F 59 41 ?? F3 0F 5E D5 F3 0F 58 D0 0F 28 C6 F3 0F 5E D5 F3 0F 58 50 ?? F3 41 0F 11 50 ?? F3 0F 59 61 ?? ?? ?? ?? F3 0F 59 59 ?? F3 0F 59 41 ?? F3 0F 58 E3 F3 0F 5E E5 F3 0F 58 E0 F3 0F 5E E5 F3 0F 58 60 ?? 41 C7 40 ?? ?? ?? ?? ?? F3 41 0F 11 60 ?? 0F 2F 71 ?? 72 ?? 48 8B 51 ?? 45 33 C9 4C 8D 42 ?? E8 ?? ?? ?? ?? 0F 28 74 24 ?? 48 83 C4 ?? C3 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 48 83 EC ?? F3 0F 10 2D ?? ?? ?? ?? F3 41 0F 10 48 ?? 0F 28 D5 F3 0F 10 25 ?? ?? ?? ?? 0F 28 C1 0F 29 74 24 ?? F3 0F 58 C5 F3 41 0F 10 70 ?? 0F 28 DD 0F 29 7C 24 ?? 0F 28 FD 44 0F 29 44 24 ?? F3 0F 5C FE ?? ?? ?? ?? ?? F3 0F 58 F5 44 0F 28 CD F3 41 0F 5C 50 ?? F3 0F 5C 5A ?? F3 0F 59 FA F3 0F 59 F2 F3 0F 10 52 ?? F3 0F 59 F8 0F 28 C5 F3 0F 5C C1 F3 0F 10 4A ?? F3 44 0F 5C C9 44 0F 28 C1 F3 0F 59 FC F3 44 0F 58 C5 F3 0F 59 F0 0F 28 C2 F3 44 0F 59 CB F3 0F 58 C5 F3 44 0F 59 C3 F3 0F 5C EA F3 0F 59 F4 F3 44 0F 59 C0 F3 44 0F 59 CD F3 44 0F 59 C4 F3 44 0F 59 CC 45 85 C9 74 ?? 66 41 0F 6E E9 0F 5B ED F3 0F 11 69";

    bool rain_slow_first_hit = false;
    bool rain_slow_skip = false;
    uintptr_t rain_slow_copyback_addr = static_cast<uintptr_t>(0);

#define DECLARE_MGS2_FRAMESKIP_STATE(name, pattern, label) \
    MGS2_CUTSCENE_FRAMESKIP_VARS(name);

    MGS2_CUTSCENE_FRAMESKIP_INLINE_HOOKS(DECLARE_MGS2_FRAMESKIP_STATE)

#undef DECLARE_MGS2_FRAMESKIP_STATE

#define DEFINE_MGS2_FRAMESKIP_HOOK(name, pattern, label) \
    DEFINE_MGS2_CUTSCENE_FRAMESKIP_HOOK(name);

    MGS2_CUTSCENE_FRAMESKIP_INLINE_HOOKS(DEFINE_MGS2_FRAMESKIP_HOOK)

#undef DEFINE_MGS2_FRAMESKIP_HOOK

    uintptr_t cigaretteMouthSmokeSpawnAfterLoad = 0;
    uintptr_t cigaretteMouthSmokeSpawnAfterInit = 0;

    constexpr uint32_t CIGARETTE_MOUTH_SMOKE_ALPHA_RISE = 0x3D000000; // 0.03125f
    constexpr uint32_t CIGARETTE_MOUTH_SMOKE_ALPHA_FALL = 0xBC2AAAAB; // -0.010416667f
    constexpr uint32_t CIGARETTE_MOUTH_SMOKE_EMIT_FADE = 0xBB888889;  // -0.004166667f


}

/// Called every frame during Present()
void EffectSpeedFix::Tick()
{
    if (eGameType & MGS2)
    {
        const bool isCutscene = g_GameVars.InCutscene();

        MGS2_CUTSCENE_FRAMESKIP_TICK(rain_slow, isCutscene);

#define TICK_MGS2_FRAMESKIP_HOOK(name, pattern, label) \
        MGS2_CUTSCENE_FRAMESKIP_TICK(name, isCutscene);

        MGS2_CUTSCENE_FRAMESKIP_INLINE_HOOKS(TICK_MGS2_FRAMESKIP_HOOK)

#undef TICK_MGS2_FRAMESKIP_HOOK
    }
}


///Called on GameVars::OnLevelTransition() to reset counters between cutscenes/levels.
void EffectSpeedFix::Reset()
{
    if (eGameType & MGS2)
    {
        MGS2_CUTSCENE_FRAMESKIP_RESET(rain_slow);

#define RESET_MGS2_FRAMESKIP_HOOK(name, pattern, label) \
        MGS2_CUTSCENE_FRAMESKIP_RESET(name);

        MGS2_CUTSCENE_FRAMESKIP_INLINE_HOOKS(RESET_MGS2_FRAMESKIP_HOOK)

#undef RESET_MGS2_FRAMESKIP_HOOK
    }
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
                MGS2_CUTSCENE_FRAMESKIP_MIDHOOK(rain_slow, ctx);
            });
    }
    else
    {
        spdlog::error("MGS 2: Effect Speed Fix : rain_slow.c - Failed to find rain_slow copyback address, rain_slow.c frameskip is disabled.");
    }


#define INSTALL_MGS2_FRAMESKIP_HOOK(name, pattern, label) \
    CREATE_MGS2_CUTSCENE_FRAMESKIP_HOOK(name, pattern, label);

    MGS2_CUTSCENE_FRAMESKIP_INLINE_HOOKS(INSTALL_MGS2_FRAMESKIP_HOOK)

#undef INSTALL_MGS2_FRAMESKIP_HOOK

//todo - ripple man - 55 8B EC 56 57 6A 00 6A ?? 68 ?? ?? ?? ?? 6A ?? E8 ?? ?? ?? ?? 8B F0 83 C4 ?? 85 F6 0F 84 ?? ?? ?? ?? 68 ?? ?? ?? ?? 68 ?? ?? ?? ?? 68 ?? ?? ?? ?? 56 E8 ?? ?? ?? ?? 8B 45 ?? 81 4E ?? ?? ?? ?? ?? 89 46 ?? 8B 45 ?? 68 ?? ?? ?? ?? C7 46 ?? 00 00 00 00 C7 46 ?? 00 00 00 00 C7 46 ?? 00 00 00 00 C7 46 ?? ?? ?? ?? ?? 89 46 ?? E8
        
        /*
    MGS2_NewSplashPartsSlow_Demo_hook = safetyhook::create_inline(reinterpret_cast<void*>(Memory::PatternScan(baseModule, "40 56 57 48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 ?? 48 8B F1", "MGS 2: Effect Speed Fix : NewSplashPartsSlow_Demo")), MGS2_NewSplashPartsSlow_Demo_Act_424);
    LOG_HOOK(MGS2_NewSplashPartsSlow_Demo_hook, "MGS 2: Effect Speed Fix : NewSplashPartsSlow_Demo.c")
    
    uintptr_t MGS2_d_splash_parts_slowScanResult = Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? ?? ?? ?? 48 83 C5 ?? 89 43", "MGS 2: Effect Speed Fix : d_splash_parts_slow") + 1);
    MGS2_d_splash_parts_slowScanResult = Memory::GetAbsolute(MGS2_d_splash_parts_slowScanResult + 0x4D);
    MGS2_d_splash_parts_slow_hook = safetyhook::create_inline(reinterpret_cast<void*>(MGS2_d_splash_parts_slowScanResult), MGS2_d_splash_parts_slow_Act_424);
    LOG_HOOK(MGS2_d_splash_parts_slow_hook, "MGS 2: Effect Speed Fix : d_splash_parts_slow.c\\Act_424()")
    */
#pragma endregion


#pragma region D012P01


    if (uint8_t* MGS2_NewFortSplineBulletDemo_Scan = Memory::PatternScan(baseModule, "F6 C1 0F 75 ?? ?? ?? 8B 46 ?? 2B C5 66 0F 6E C0 0F 5B C0 0F 2F 05 ?? ?? ?? ?? 76 ?? 41 0F 28 CD EB ?? F3 0F 59 05 ?? ?? ?? ?? F3 0F 10 0D ?? ?? ?? ?? F3 0F 5C C8 F3 0F 59 3D ?? ?? ?? ?? 4C 8D 46 ?? F3 0F 5C F9 44 0F 2F EF 76 ?? 41 0F 28 FF 41 0F 2F 78 ?? 76 ?? 49 83 C0 20 41 0F 2F 78 ?? 77 ?? 49 8D 50 ?? 45 33 C9 48 8D 4C 24 ?? E8 ?? ?? ?? ?? 0F 28 CF 48 8D 4C 24 ?? 4C 8B C7 E8 ?? ?? ?? ?? 8B 46 ?? 2B C5 66 0F 6E C0 0F 5B C0 F3 0F 5E 05 ?? ?? ?? ?? F3 0F 58 47 ?? F3 0F 11 47 ?? ?? ?? ?? 49 83 C6 20 44 0B F8 48 83 C3 20 48 83 C7 10 FF C5 83 FD 28 0F 8C ?? ?? ?? ?? 44 0F 28 7C 24 ?? 44 0F 28 B4 24 ?? ?? ?? ?? 44 0F 28 AC 24 ?? ?? ?? ?? 44 0F 28 A4 24 ?? ?? ?? ?? 44 0F 28 9C 24 ?? ?? ?? ?? 44 0F 28 94 24 ?? ?? ?? ?? 44 0F 28 8C 24 ?? ?? ?? ?? 44 0F 28 84 24 ?? ?? ?? ?? 0F 28 BC 24 ?? ?? ?? ?? 0F 28 B4 24 ?? ?? ?? ?? 45 85 FF 75 ?? 48 8B CE E8 ?? ?? ?? ?? 83 46 ?? 06 EB ?? FF C0 89 46 ?? 48 8B 4C 24 ?? 48 33 CC E8 ?? ?? ?? ?? 48 8B 9C 24 ?? ?? ?? ?? 48 81 C4 10 01 00 00 41 5F 41 5E 5F 5E 5D C3 CC CC CC CC CC CC CC CC 48 83 EC 38 F3 0F 10 69 ?? ?? ?? ?? 0F 29 74 24 ?? 0F 28 F1 F3 0F 5C 71 ?? 0F 28 DE F3 0F 59 DE 0F 28 E3 0F 28 C3 F3 0F 59 41 ?? F3 0F 59 E6 0F 28 CC 0F 28 D4 F3 0F 59 49 ?? F3 0F 58 C8 0F 28 C6 F3 0F 59 41 ?? F3 0F 5E CD F3 0F 58 C8 0F 28 C3 F3 0F 5E CD ?? ?? ?? ?? ?? ?? ?? ?? ?? F3 0F 59 51 ?? ?? ?? ?? F3 0F 59 41 ?? F3 0F 58 D0 0F 28 C6 F3 0F 59 41 ?? F3 0F 5E D5 F3 0F 58 D0 0F 28 C6 F3 0F 5E D5 F3 0F 58 50 ?? F3 41 0F 11 50 ?? F3 0F 59 61 ?? ?? ?? ?? F3 0F 59 59 ?? F3 0F 59 41 ?? F3 0F 58 E3 F3 0F 5E E5 F3 0F 58 E0 F3 0F 5E E5 F3 0F 58 60 ?? 41 C7 40 ?? 00 00 80 3F F3 41 0F 11 60 ?? 0F 2F 71 ?? 72 ?? 48 8B 51 ?? 45 33 C9 4C 8D 42 ?? E8 ?? ?? ?? ?? 0F 28 74 24 ?? 48 83 C4 38 C3 CC CC CC CC CC CC CC CC CC CC 48 83 EC 48 F3 0F 10 2D ?? ?? ?? ?? F3 41 0F 10 48 ?? 0F 28 D5 F3 0F 10 25 ?? ?? ?? ?? 0F 28 C1 0F 29 74 24 ?? F3 0F 58 C5 F3 41 0F 10 70 ?? 0F 28 DD 0F 29 7C 24 ?? 0F 28 FD 44 0F 29 44 24 ?? F3 0F 5C FE ?? ?? ?? ?? ?? F3 0F 58 F5 44 0F 28 CD F3 41 0F 5C 50 ?? F3 0F 5C 5A ?? F3 0F 59 FA F3 0F 59 F2 F3 0F 10 52 ?? F3 0F 59 F8 0F 28 C5 F3 0F 5C C1 F3 0F 10 4A ?? F3 44 0F 5C C9 44 0F 28 C1 F3 0F 59 FC F3 44 0F 58 C5 F3 0F 59 F0 0F 28 C2 F3 44 0F 59 CB F3 0F 58 C5 F3 44 0F 59 C3 F3 0F 5C EA F3 0F 59 F4 F3 44 0F 59 C0 F3 44 0F 59 CD F3 44 0F 59 C4 F3 44 0F 59 CC 45 85 C9 74 ?? 66 41 0F 6E E9 0F 5B ED F3 0F 11 69", "MGS2: MGS2_NewFortSplineBulletDemo_Scan"))
    {
        Memory::PatchBytes((uintptr_t)MGS2_NewFortSplineBulletDemo_Scan, "\xF6\xC1\x0E", 3);
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

#pragma region demo_cigarette_smoke

    Memory::PatchFloatImmediate(
        baseModule,
        "49 C7 46 FC ?? ?? ?? ?? 41 89 4C 3F F8",
        4,
        CIGARETTE_MOUTH_SMOKE_ALPHA_RISE,
        "MGS 2: Effect Speed Fix: Cigarette mouth smoke alpha rise");

    Memory::PatchFloatImmediate(
        baseModule,
        "C7 83 00 18 00 00 ?? ?? ?? ?? EB ?? 44 0F 2F E8",
        6,
        CIGARETTE_MOUTH_SMOKE_ALPHA_FALL,
        "MGS 2: Effect Speed Fix: Cigarette mouth smoke alpha fall");

    Memory::PatchFloatImmediate(
        baseModule,
        "C7 83 6C 28 00 00 ?? ?? ?? ?? 8B 8B 04 28 00 00",
        6,
        CIGARETTE_MOUTH_SMOKE_EMIT_FADE,
        "MGS 2: Effect Speed Fix: Cigarette mouth smoke emit fade");

    if (uint8_t* cigaretteMouthSmokeEmitTimer = Memory::PatternScan(baseModule, "C7 83 6C 28 00 00 ?? ?? ?? ?? 8B 8B 04 28 00 00", "MGS 2: Effect Speed Fix: Cigarette mouth smoke emit lifetime"))
    {
        static SafetyHookMid cigaretteMouthSmokeEmitTimerHook {};
        cigaretteMouthSmokeEmitTimerHook = safetyhook::create_mid(cigaretteMouthSmokeEmitTimer + 10,
            [](SafetyHookContext& ctx)
            {
                auto* timer = reinterpret_cast<int32_t*>(ctx.rbx + 0x2804);
                const auto localTimer = *reinterpret_cast<uint32_t*>(ctx.rbx + 0x2808);

                if (*timer >= 0 && (localTimer & 1) == 0)
                {
                    ++*timer;
                }
            });
        LOG_HOOK(cigaretteMouthSmokeEmitTimerHook, "MGS 2: Effect Speed Fix: Cigarette mouth smoke emit lifetime")
    }

    MAKE_HOOK_MID(baseModule, "F3 0F 58 83 ?? ?? ?? ?? F3 0F 59 4C 24 ?? F3 0F 11 43 ??", "MGS 2: Effect Speed Fix: Cigarette mouth smoke X movement", {
            ctx.xmm0.f32[0] *= 0.5f;
        });

    MAKE_HOOK_MID(baseModule, "F3 0F 58 83 ?? ?? ?? ?? F3 0F 58 64 24 ?? F3 0F 11 03", "MGS 2: Effect Speed Fix: Cigarette mouth smoke Y movement", {
            ctx.xmm0.f32[0] *= 0.5f;
        });

    MAKE_HOOK_MID(baseModule, "F3 0F 58 83 ?? ?? ?? ?? F3 0F 59 6C 24 ?? F3 0F 11 43 ??", "MGS 2: Effect Speed Fix: Cigarette mouth smoke Z movement", {
            ctx.xmm0.f32[0] *= 0.5f;
        });

    if (uint8_t* cigaretteMouthSmokeSpawn = Memory::PatternScan(baseModule, "41 8B 86 08 28 00 00 25 3F 00 00 80 7D 07 FF C8 83 C8 C0 FF C0 03 C0 48 8D 0D ?? ?? ?? ?? 48 63 F0", "MGS 2: Effect Speed Fix: Cigarette mouth smoke spawn cadence"))
    {
        cigaretteMouthSmokeSpawnAfterLoad = reinterpret_cast<uintptr_t>(cigaretteMouthSmokeSpawn) + 7;
        cigaretteMouthSmokeSpawnAfterInit = reinterpret_cast<uintptr_t>(cigaretteMouthSmokeSpawn) + 0x1CA;

        static SafetyHookMid cigaretteMouthSmokeSpawnHook {};
        cigaretteMouthSmokeSpawnHook = safetyhook::create_mid(cigaretteMouthSmokeSpawn,
            [](SafetyHookContext& ctx)
            {
                const uint32_t timer = *reinterpret_cast<uint32_t*>(ctx.r14 + 0x2808);

                if ((timer & 1) == 0)
                {
                    ctx.rbx = 0;
                    ctx.rcx = 0;
                    ctx.rip = cigaretteMouthSmokeSpawnAfterInit;
                    return;
                }

                ctx.rax = timer >> 1;
                ctx.rip = cigaretteMouthSmokeSpawnAfterLoad;
            });
        LOG_HOOK(cigaretteMouthSmokeSpawnHook, "MGS 2: Effect Speed Fix: Cigarette mouth smoke spawn cadence")
    }

    /*
    MAKE_HOOK_MID(baseModule, "48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 8B E9 0F 29 74 24", "user\\okajima\\demo_effect\\d_fog_set.c -> NewFogSet_Demo()", {
        if (g_GameVars.InCutscene())
        {
            *reinterpret_cast<int*>(ctx.rsp + 0x30) *= 2;
        }
    })*/

#pragma endregion

    if (Util::CheckForASIFiles("MGSFPSUnlock", false, false, "2025-05-25"))
    {
        spdlog::info("MGS 2: Effect Speed Fix: Outdated version of MGSFPSUnlock detected, Large explosion & Solidus's Firedash fixes are disabled.");
        return;
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

    *//*
    if (uint8_t* MGS2_crosfade_c_Result = Memory::PatternScan(baseModule, "89 5F ?? 79 ?? 89 77", "MGS 2: Effect Speed Fix : effect1\\crosfade.c"))
    {


    /*
    if (uint8_t* Tidal4Result = Memory::PatternScan(baseModule, "F3 0F 58 83 ?? ?? ?? ?? F3 0F 11 83 ?? ?? ?? ?? 41 0F 28 C3", "MGS 2: Effect Speed Fix : effect\\tidal4.c"))
    {


*/
