// ReSharper disable CppClangTidyModernizeRawStringLiteral
#include "stdafx.h"


#include "common.hpp"

#include "effect_speeds.hpp"

#include "game_funcs.hpp"
#include "gamevars.hpp"
#include "logging.hpp"
#include "mgs2_flare_occlusion.hpp"
#include "mgs2_linkvarbuf.hpp"
#include "mgs2_railgun_beam.hpp"
#include "custom_resolution_and_borderless.hpp"


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

// tracks every individual actor instance to ensure that the first tick of every actor is never skipped
class FirstTickGuard
{
public:
    bool ConsumeFirstTick(uintptr_t work)
    {
        const size_t slot = Slot(work);
        if (m_seen[slot] != work)
        {
            m_seen[slot] = work; // collisions just force an extra un-skipped tick later - harmless
            return true;
        }
        return false;
    }

    void ClearOnDeath(uintptr_t work)
    {
        const size_t slot = Slot(work);
        if (m_seen[slot] == work)
        {
            m_seen[slot] = 0;
        }
    }

private:
    static size_t Slot(uintptr_t work) { return (work >> 4) & 63; }
    uintptr_t m_seen[64] = {};
};


#define DEFINE_FULL_SKIP_ACT_DIE_PAIR(actName, dieName)                                    \
    SafetyHookInline actName##_hook{};                                                     \
    SafetyHookInline dieName##_hook{};                                                     \
    FirstTickGuard g_##actName##_guard;                                                    \
                                                                                             \
    int64_t __fastcall actName##_Hook(int64_t work)                                        \
    {                                                                                       \
        const bool firstTick = g_##actName##_guard.ConsumeFirstTick(static_cast<uintptr_t>(work)); \
        if (SkipFrame() && !firstTick)                                                      \
        {                                                                                   \
            return 0;                                                                       \
        }                                                                                   \
        return actName##_hook.call<int64_t>(work);                                          \
    }                                                                                       \
                                                                                             \
    void __fastcall dieName##_Hook(int64_t work)                                            \
    {                                                                                       \
        g_##actName##_guard.ClearOnDeath(static_cast<uintptr_t>(work));                     \
        dieName##_hook.call<void>(work);                                                    \
    }

#define INSTALL_FULL_SKIP_ACT_DIE_PAIR(actName, actPattern, actLabel, dieName, diePattern, dieLabel) \
    if (uint8_t* addr = Memory::PatternScan(baseModule, actPattern, actLabel))              \
    {                                                                                       \
        actName##_hook = safetyhook::create_inline(reinterpret_cast<void*>(addr), actName##_Hook); \
        LOG_HOOK(actName##_hook, actLabel)                                                  \
                                                                                             \
        if (uint8_t* dieAddr = Memory::PatternScan(baseModule, diePattern, dieLabel))       \
        {                                                                                   \
            dieName##_hook = safetyhook::create_inline(reinterpret_cast<void*>(dieAddr), dieName##_Hook); \
            LOG_HOOK(dieName##_hook, dieLabel)                                              \
        }                                                                                   \
        else                                                                                \
        {                                                                                   \
            spdlog::error("MGS 2: Effect Speed Fix : {} scan failed - {} throttle is active. Disabling.", dieLabel, actLabel); \
            actName##_hook = {};                                                            \
        }                                                                                   \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        spdlog::error("MGS 2: Effect Speed Fix : {} scan failed. Skipping throttle hooks.", actLabel); \
    }


// Act/Die pairs, tracks every individual actor instance to ensure that the first tick of every actor is never skipped
#define MGS2_FULL_SKIP_ACT_DIE_PAIRS(X) \
    X(Act_227, "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B F9 45 33 C0", "MGS 2: Effect Speed Fix : user\\morita\\splash\\splash.c -> Act() (Act_227) | NewSplash() | NewWaveSplash_Demo() | NewWaveSplash_Demo2()", \
        Die_182, "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B 79 ?? 48 8B D9 48 85 FF 74 ?? 48 8B CF E8 ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 48 8B BB", "MGS 2: Effect Speed Fix : user\\morita\\splash\\splash.c -> Die() (Die_182)") \
        \
    X(Act_234, "48 8B C4 48 89 58 ?? 48 89 70 ?? 48 89 78 ?? 55 41 56 41 57 48 8D 68 ?? 48 81 EC ?? ?? ?? ?? 0F 29 70 ?? 48 8B D9", "MGS 2: Effect Speed Fix : user\\morita\\demo_bul\\demo_bullet.c -> NewDemoBulletCall() -> Act() | (Act_234)", \
        Die_188, "40 53 48 83 EC ?? 48 8B D9 48 8B 89 ?? ?? ?? ?? 48 85 C9 74 ?? E8 ?? ?? ?? ?? 48 8B 8B ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 9B", "MGS 2: Effect Speed Fix : user\\morita\\demo_bul\\demo_bullet.c -> NewDemoBulletCall() -> Die() (Die_188)")

#define DEFINE_FULL_SKIP_HOOK(actName, actPattern, actLabel, dieName, diePattern, dieLabel) \
    DEFINE_FULL_SKIP_ACT_DIE_PAIR(actName, dieName)

#define CREATE_FULL_SKIP_HOOK(actName, actPattern, actLabel, dieName, diePattern, dieLabel) \
    INSTALL_FULL_SKIP_ACT_DIE_PAIR(actName, actPattern, actLabel, dieName, diePattern, dieLabel)


#define MGS2_CUTSCENE_FRAMESKIP_MIDHOOK(name, ctx)    \
    do                                           \
    {                                            \
        if (SkipFrame())                         \
        {                                        \
            ctx.rip = name##_copyback_addr;      \
        }                                        \
                                                 \
    } while (false)

#define DEFINE_MGS2_CUTSCENE_FRAMESKIP_HOOK(name)             \
    SafetyHookInline name##_hook {};                    \
    static int64_t name##_Hook(int64_t work)              \
    {                                                    \
        if (SkipFrame())       \
        {                                                \
            /* Skipped New* constructors must return NULL - callers keep the pointer. */ \
            return 0;                                    \
        }                                                \
                                                         \
        return name##_hook.call<int64_t>(work);          \
    }

#define CREATE_MGS2_CUTSCENE_FRAMESKIP_HOOK(name, pattern, label)                    \
    if (uint8_t* addr = Memory::PatternScan(baseModule, pattern, label))        \
    {                                                                          \
        name##_hook = safetyhook::create_inline(                                \
            reinterpret_cast<void*>(addr),                                      \
            name##_Hook);                                                       \
        LOG_HOOK(name##_hook, label)                                            \
    }

//GOOD EFFECT - DON'T USE  (NewDemoSplashBlood, "48 8B C4 48 89 58 ?? 48 89 70 ?? 57 48 81 EC ?? ?? ?? ?? 0F 29 70 ?? 48 8B D9", "MGS 2: Effect Speed Fix : user\\kunibe\\effect\\demo_splash_blood.c -> NewDemoSplashBlood()")

//todo: need to investigate OK_PutSplush | mgs2x\source\user\okajima\effect2\splush_man.c

#define MGS2_CUTSCENE_FRAMESKIP_INLINE_HOOKS(X) \
    X(NewSplashMotion_Demo, "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 45 33 C9 0F 29 74 24 ?? 41 8B F0 48 8B F9 BA ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 0F 28 F1 41 8D 49 ?? E8 ?? ?? ?? ?? 48 8B D8 48 85 C0 0F 84 ?? ?? ?? ?? 4C 8D 05 ?? ?? ?? ?? 48 8B C8 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 81 4B ?? ?? ?? ?? ?? 48 8D 05", "MGS 2 : Effect Speed Fix : user\\okajima\\demo_effect\\d_splash_motion.c -> NewSplashMotion_Demo() (spawner cadence: keeps droplet population PS2-sized so the shared GV heap never starves plasma/other effects)") \
    X(NewDropBodySplush, "40 57 41 57 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24", "MGS 2: Effect Speed Fix : user\\okajima\\effect2\\drop_body_splush.c -> NewDropBodySplush()") \
    X(NewRipBubbleMan, "40 55 56 57 41 55 48 8D 6C 24", "MGS 2: Effect Speed Fix : user\\okajima\\effect2\\ripple_bubble.c -> NewRipBubbleMan_DEMO() | NewRipBubbleMan()") \
    X(NewSplushTidalParts4, "40 53 56 48 81 EC ?? ?? ?? ?? 48 8B F1 48 83 E9 ?? E8 ?? ?? ?? ?? F3 0F 10 0D ?? ?? ?? ?? 0F 2F C8 48 8B 46 ?? 76 ?? 81 88 ?? ?? ?? ?? ?? ?? ?? ?? 48 8B CE 48 81 C4 ?? ?? ?? ?? 5E 5B E9 ?? ?? ?? ?? 81 A0 ?? ?? ?? ?? ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 48 89 AC 24 ?? ?? ?? ?? 48 89 BC 24 ?? ?? ?? ?? 4C 89 A4 24 ?? ?? ?? ?? 4C 89 B4 24 ?? ?? ?? ?? 4C 89 BC 24 ?? ?? ?? ?? 4C 8B 7E ?? 0F 29 B4 24 ?? ?? ?? ?? 0F 29 BC 24 ?? ?? ?? ?? 44 0F 29 84 24 ?? ?? ?? ?? 44 0F 29 8C 24 ?? ?? ?? ?? 44 0F 29 54 24 ?? 44 0F 29 5C 24 ?? 44 0F 29 64 24 ?? 44 0F 29 6C 24 ?? F3 44 0F 10 2D ?? ?? ?? ?? 44 0F 29 74 24 ?? F3 44 0F 10 35 ?? ?? ?? ?? 44 0F 29 7C 24 ?? F3 44 0F 10 3D ?? ?? ?? ?? E8 ?? ?? ?? ?? 4D 63 8F ?? ?? ?? ?? BA ?? ?? ?? ?? 41 2B D1 41 89 87 ?? ?? ?? ?? 41 89 97 ?? ?? ?? ?? 41 BE ?? ?? ?? ?? 4C 63 46 ?? 45 3B C6 7D ?? 41 8B C6 49 8D 48 ?? 48 03 C9 41 2B C0 66 0F 6E D0 0F 5B D2 F3 0F 5E 15 ?? ?? ?? ?? 0F 28 C2 0F 28 CA F3 0F 59 86 ?? ?? ?? ?? F3 0F 59 C2 ?? ?? ?? ?? ?? 0F 28 C2 F3 0F 59 86 ?? ?? ?? ?? F3 0F 11 44 CE ?? F3 0F 59 8E ?? ?? ?? ?? F3 0F 59 CA F3 0F 11 4C CE ?? FF 46 ?? 66 44 0F 6E 66 ?? B9 ?? ?? ?? ?? F3 0F 10 15 ?? ?? ?? ?? 45 0F 5B E4 4C 63 E2 4B 8B 94 E7 ?? ?? ?? ?? F3 44 0F 5E 25 ?? ?? ?? ?? 48 83 C2 ?? 41 0F 28 CC F3 0F 59 0D ?? ?? ?? ?? 0F 1F 40 ?? 0F 1F 84 00 ?? ?? ?? ?? 66 0F 6E C1 48 8D 52 ?? 0F 5B C0 FF C9 F3 0F 59 C1 F3 0F 59 C2 F3 0F 2C C0 88 42 ?? 85 C9 7F ?? 4B 8B 94 CF ?? ?? ?? ?? 48 8D 9E ?? ?? ?? ?? ?? ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? F3 0F 10 B6 ?? ?? ?? ?? 41 B9 ?? ?? ?? ?? F3 44 0F 10 5B ?? F3 0F 10 BE ?? ?? ?? ?? F3 44 0F 10 4B ?? F3 44 0F 10 86 ?? ?? ?? ?? 45 8D 41 ?? E8 ?? ?? ?? ?? ?? ?? ?? ?? F3 44 0F 5C DF F3 0F 10 4B ?? F3 45 0F 5C C8 F3 0F 10 53 ?? F3 44 0F 5C D6 0F 28 E8 0F 28 DA 0F 28 E1 F3 41 0F 5C ED 44 0F 28 6C 24 ?? F3 44 0F 5C D9 F3 44 0F 5C CA 33 ED F3 44 0F 5C D0 F3 41 0F 5C E6 44 0F 28 74 24 ?? F3 41 0F 5C DF 44 0F 28 7C 24 ?? 41 0F 28 C3 F3 44 0F 59 DD 41 0F 28 FA 45 0F 28 C1 F3 44 0F 59 CD F3 0F 59 C3 F3 44 0F 59 C4 F3 0F 59 FB F3 44 0F 5C C0 F3 44 0F 59 D4 F3 41 0F 5C F9 F3 45 0F 5C DA 44 0F 28 54 24 ?? 41 0F 28 C0 F3 41 0F 59 C0 0F 28 D7 F3 0F 59 D7 41 0F 28 CB F3 41 0F 59 CB F3 0F 58 D0 0F 57 C0 F3 0F 58 D1 0F 54 15 ?? ?? ?? ?? 0F 2E C2 77 ?? 0F 57 C0 F3 0F 51 C2 EB ?? 0F 28 C2 E8 ?? ?? ?? ?? 0F 2F 05 ?? ?? ?? ?? F3 0F 10 0D ?? ?? ?? ?? 45 0F 57 C9 76 ?? 0F 28 F1 F3 0F 5E F0 EB ?? 0F 57 F6 F3 44 0F 59 C6 F3 41 0F 5C CC 48 8D 3D ?? ?? ?? ?? 44 0F 28 64 24 ?? F3 0F 59 FE F3 0F 59 4E ?? F3 44 0F 59 DE F3 41 0F 59 F1 F3 44 0F 59 C1 F3 0F 59 F9 F3 44 0F 59 D9 F3 0F 59 F1 0F 1F 40 ?? 66 66 0F 1F 84 00 ?? ?? ?? ?? F3 0F 10 83 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? F3 0F 10 4B ?? F3 0F 58 8B ?? ?? ?? ?? F3 0F 11 4B ?? F3 0F 10 83 ?? ?? ?? ?? F3 0F 58 43 ?? F3 0F 11 43 ?? F3 0F 10 4B ?? F3 0F 58 8B ?? ?? ?? ?? F3 0F 11 4B ?? E8 ?? ?? ?? ?? 41 0F 2F F9 F3 0F 58 83 ?? ?? ?? ?? F3 0F 11 83 ?? ?? ?? ?? 41 0F 28 C3", "MGS 2 : user\\okajima\\effect2\\splush_tidal_parts4.c -> NewSplushTidalParts4() | NewWallTidal()") \
    X(NewSplushTidalParts, "40 53 56 48 81 EC ?? ?? ?? ?? 48 8B F1 48 83 E9 ?? E8 ?? ?? ?? ?? F3 0F 10 0D ?? ?? ?? ?? 0F 2F C8 48 8B 46 ?? 76 ?? 81 88 ?? ?? ?? ?? ?? ?? ?? ?? 48 8B CE 48 81 C4 ?? ?? ?? ?? 5E 5B E9 ?? ?? ?? ?? 81 A0 ?? ?? ?? ?? ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 48 89 AC 24 ?? ?? ?? ?? 48 89 BC 24 ?? ?? ?? ?? 4C 89 A4 24 ?? ?? ?? ?? 4C 89 B4 24 ?? ?? ?? ?? 4C 89 BC 24 ?? ?? ?? ?? 4C 8B 7E ?? 0F 29 B4 24 ?? ?? ?? ?? 0F 29 BC 24 ?? ?? ?? ?? 44 0F 29 84 24 ?? ?? ?? ?? 44 0F 29 8C 24 ?? ?? ?? ?? 44 0F 29 54 24 ?? 44 0F 29 5C 24 ?? 44 0F 29 64 24 ?? 44 0F 29 6C 24 ?? F3 44 0F 10 2D ?? ?? ?? ?? 44 0F 29 74 24 ?? F3 44 0F 10 35 ?? ?? ?? ?? 44 0F 29 7C 24 ?? F3 44 0F 10 3D ?? ?? ?? ?? E8 ?? ?? ?? ?? 4D 63 8F ?? ?? ?? ?? BA ?? ?? ?? ?? 41 2B D1 41 89 87 ?? ?? ?? ?? 41 89 97 ?? ?? ?? ?? 41 BE ?? ?? ?? ?? 4C 63 46 ?? 45 3B C6 7D ?? 41 8B C6 49 8D 48 ?? 48 03 C9 41 2B C0 66 0F 6E D0 0F 5B D2 F3 0F 5E 15 ?? ?? ?? ?? 0F 28 C2 0F 28 CA F3 0F 59 86 ?? ?? ?? ?? F3 0F 59 C2 ?? ?? ?? ?? ?? 0F 28 C2 F3 0F 59 86 ?? ?? ?? ?? F3 0F 11 44 CE ?? F3 0F 59 8E ?? ?? ?? ?? F3 0F 59 CA F3 0F 11 4C CE ?? FF 46 ?? 66 44 0F 6E 66 ?? B9 ?? ?? ?? ?? F3 0F 10 15 ?? ?? ?? ?? 45 0F 5B E4 4C 63 E2 4B 8B 94 E7 ?? ?? ?? ?? F3 44 0F 5E 25 ?? ?? ?? ?? 48 83 C2 ?? 41 0F 28 CC F3 0F 59 0D ?? ?? ?? ?? 0F 1F 40 ?? 0F 1F 84 00 ?? ?? ?? ?? 66 0F 6E C1 48 8D 52 ?? 0F 5B C0 FF C9 F3 0F 59 C1 F3 0F 59 C2 F3 0F 2C C0 88 42 ?? 85 C9 7F ?? 4B 8B 94 CF ?? ?? ?? ?? 48 8D 9E ?? ?? ?? ?? ?? ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? F3 0F 10 B6 ?? ?? ?? ?? 41 B9 ?? ?? ?? ?? F3 44 0F 10 5B ?? F3 0F 10 BE ?? ?? ?? ?? F3 44 0F 10 4B ?? F3 44 0F 10 86 ?? ?? ?? ?? 45 8D 41 ?? E8 ?? ?? ?? ?? ?? ?? ?? ?? F3 44 0F 5C DF F3 0F 10 4B ?? F3 45 0F 5C C8 F3 0F 10 53 ?? F3 44 0F 5C D6 0F 28 E8 0F 28 DA 0F 28 E1 F3 41 0F 5C ED 44 0F 28 6C 24 ?? F3 44 0F 5C D9 F3 44 0F 5C CA 33 ED F3 44 0F 5C D0 F3 41 0F 5C E6 44 0F 28 74 24 ?? F3 41 0F 5C DF 44 0F 28 7C 24 ?? 41 0F 28 C3 F3 44 0F 59 DD 41 0F 28 FA 45 0F 28 C1 F3 44 0F 59 CD F3 0F 59 C3 F3 44 0F 59 C4 F3 0F 59 FB F3 44 0F 5C C0 F3 44 0F 59 D4 F3 41 0F 5C F9 F3 45 0F 5C DA 44 0F 28 54 24 ?? 41 0F 28 C0 F3 41 0F 59 C0 0F 28 D7 F3 0F 59 D7 41 0F 28 CB F3 41 0F 59 CB F3 0F 58 D0 0F 57 C0 F3 0F 58 D1 0F 54 15 ?? ?? ?? ?? 0F 2E C2 77 ?? 0F 57 C0 F3 0F 51 C2 EB ?? 0F 28 C2 E8 ?? ?? ?? ?? 0F 2F 05 ?? ?? ?? ?? F3 0F 10 0D ?? ?? ?? ?? 45 0F 57 C9 76 ?? 0F 28 F1 F3 0F 5E F0 EB ?? 0F 57 F6 F3 44 0F 59 C6 F3 41 0F 5C CC 48 8D 3D ?? ?? ?? ?? 44 0F 28 64 24 ?? F3 0F 59 FE F3 0F 59 4E ?? F3 44 0F 59 DE F3 41 0F 59 F1 F3 44 0F 59 C1 F3 0F 59 F9 F3 44 0F 59 D9 F3 0F 59 F1 0F 1F 40 ?? 66 66 0F 1F 84 00 ?? ?? ?? ?? F3 0F 10 83 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? F3 0F 10 4B ?? F3 0F 58 8B ?? ?? ?? ?? F3 0F 11 4B ?? F3 0F 10 83 ?? ?? ?? ?? F3 0F 58 43 ?? F3 0F 11 43 ?? F3 0F 10 4B ?? F3 0F 58 8B ?? ?? ?? ?? F3 0F 11 4B ?? E8 ?? ?? ?? ?? 41 0F 2F F9 F3 0F 58 83 ?? ?? ?? ?? F3 0F 11 83 ?? ?? ?? ?? 0F 28 C7", "MGS 2 : user\\okajima\\effect2\\splush_tidal_parts.c -> NewSplushTidalParts()") \
    X(NewSplushTidalParts2, "40 53 57 48 81 EC ?? ?? ?? ?? 48 8B F9 48 83 E9", "MGS 2: Effect Speed Fix : okajima\\effect2\\splush_tidal_parts2.c -> NewSplushTidalParts2()") \
    X(NewBubbleMany, "40 56 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 ?? 48 83 79 ?? 00 48 8B F1 75", "MGS 2: Effect Speed Fix : user\\okajima\\demo_effect\\bubble_many.c -> NewBubbleMany_Demo() | NewBubbleMany() | NewRaidenMaskBubbleDemo()") \
    X(NewBreath, "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 83 B9 ?? ?? ?? ?? ?? 48 8B F1", "MGS 2: Effect Speed Fix : user\\okajima\\t_effect\\breath.c -> NewBreath() | NewBreathDemo()") \
    X(NewRippleMan, "48 89 4C 24 ?? 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ?? 48 8B 69", "MGS 2: Effect Speed Fix : user\\okajima\\effect2\\ripple_man.c -> NewRippleMan()") \
    X(NewBombGasEffect, "4C 8B DC 55 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 ?? 48 8B 41", "MGS 2: Effect Speed Fix : user\\okajima\\effect\\bomb_gas.c -> NewBombGasEffect()") \
    X(NewSpark2, "48 89 5C 24 ?? 56 48 83 EC ?? FF 49", "MGS 2 : user\\okajima\\t_effect\\spark.c -> NewSpark2() | NewSpark()") \
    X(NewDiveSplash, "40 57 48 83 EC 20 83 B9 ?? ?? ?? ?? 00 48 8B F9 7E 33 48 89 5C 24 ?? 48 8B 59 ?? 8B 0D ?? ?? ?? ?? E8 DA 47 D6 FF", "MGS 2: Effect Speed Fix : user\\okajima\\demo_effect\\dive_splash.c-> NewDiveSplash() | NewDiveSplash2() | NewDiveSplashScn() | NewDiveSplash_Parent() ") \
    X(NewRopeModel3, "40 53 48 83 EC ?? 48 8B D9 48 8D 91 ?? ?? ?? ?? 48 8B 49 ?? E8 ?? ?? ?? ?? 48 8D 05", "MGS 2: Effect Speed Fix : user\\kano\\rope\\ropemain.c -> NewRopeModel3() | NewRopeModel_called() | NewRopeModel3_called()") \
    X(NewFootSplash, "40 53 48 83 EC ?? 48 83 B9 ?? ?? ?? ?? 00 48 8B D9 0F 84 ?? ?? ?? ?? 48 83 B9 ?? ?? ?? ?? 00", "MGS 2: Effect Speed Fix : user\\okajima\\effect\\ft_splsh.c -> NewFootSplash()") \
    X(NewRainCamera_Demo, "48 8B C4 55 57 41 54 41 55", "MGS 2: Effect Speed Fix : user\\okajima\\demo_effect\\d_rain_cm.c -> NewRainCamera_Demo()") \
    X(NewBodySplash, "40 56 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 ?? 48 83 79 ?? 00 48 8B F1 74", "MGS 2: Effect Speed Fix : user\\okajima\\effect\\body_sph.c -> NewBodySplashScn() | NewBodySplash() | NewBodySplash2() | NewBodySplash3()") \
    X(NewRayEye, "F6 41 ?? ?? 0F 85 ?? ?? ?? ?? E9", "MGS 2: Effect Speed Fix : user\\shibata\\effect\\ray_eye.c -> NewRayEye()") \
    X(NewDemoHarrierDamageSmoke, "4C 8B DC 53 57 48 81 EC ?? ?? ?? ?? 45 0F 29 53", "MGS 2: Effect Speed Fix : user\\kunibe\\effect\\harrier_damage_smoke.c-> NewDemoHarrierDamageSmoke()") \
    X(NewDemoBladeSpark, "40 53 48 83 EC ?? 48 8B 51 ?? 41 B9 ?? ?? ?? ?? 41 8B C1 0F 29 74 24", "MGS 2: Effect Speed Fix : user\\kunibe\\effect\\blade_spark.c -> NewDemoBladeSpark()") \
    X(NewLightSpark, "48 89 74 24 ?? 57 48 83 EC ?? F3 0F 10 89", "MGS 2 : Effect Speed Fix : user\\kunibe\\effect\\light_spark.c -> NewLightSpark()") \
    X(NewSmoke2Strip, "48 8B C4 48 89 58 ?? 48 89 68 ?? 48 89 70 ?? 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 4C 8B B1", "MGS 2: Effect Speed Fix : user\\okajima\\effect3\\smoke2_strip.c -> NewSmoke2Strip()") \
    X(NewSmokeStrip, "48 8B C4 53 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 48 8B B9", "MGS 2: Effect Speed Fix : user\\okajima\\effect3\\smoke_strip.c -> NewSmokeStrip()") \
    X(NewCypherRisingSmoke, "48 8B C4 48 89 58 ?? 48 89 68 ?? 48 89 70 ?? 48 89 48 ?? 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 48 8B 51", "MGS 2: Effect Speed Fix : user\\kunibe\\effect\\cypher_rising_smoke.c -> NewCypherRisingSmoke()") \
    X(NewC4_LampEX, "40 53 48 83 EC ?? 48 8B 51 ?? B8 ?? ?? ?? ?? 48 8B D9 2B 82 ?? ?? ?? ?? 89 82 ?? ?? ?? ?? 8B 89", "MGS 2: Effect Speed Fix : user\\skoba\\test\\c4_eff.c -> NewC4_LampEX()") \
    X(NewTs_Spark, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? FF 41", "MGS 2: Effect Speed Fix : user\\shibata\\radio_break\\ts_spark.c -> NewTs_Spark()") \
    X(NewBombKasu, "48 8B C4 48 89 58 ?? 48 89 68 ?? 48 89 70 ?? 57 41 54 41 55 41 56 41 57 48 83 EC ?? 0F 29 70 ?? 4C 8B E9", "MGS 2 : Effect Speed Fix : user\\okajima\\effect\\bomb_kasu.c -> NewBombKasu()") \
    X(NewRayMissileShower, "48 8B C4 48 89 58 ?? 48 89 68 ?? 48 89 70 ?? 48 89 78 ?? 41 54 41 56 41 57 48 81 EC ?? ?? ?? ?? 4C 8B B9", "MGS 2: Effect Speed Fix : user\\okajima\\effect3\\ray_missile_shower.c -> NewRayMissileShower()") \
    X(NewDemoRisingSmokeFix, "4C 8B DC 49 89 5B ?? 49 89 6B ?? 56 57 41 55", "MGS 2: Effect Speed Fix : user\\kunibe\\effect\\demo_rising_smoke.c -> NewDemoRisingSmokeFix()") \
    X(NewCommonSmoke, "40 57 48 83 EC ?? 48 8B 41 ?? 48 8B F9 48 89 5C 24 ?? 48 89 6C 24", "MGS 2: Effect Speed Fix : user\\okajima\\demo_effect\\common_smoke.c -> NewCommonSmoke()") \
    X(NewDemoStageFire, "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 ?? 48 8B F9 33 DB", "MGS 2 : Effect Speed Fix : user\\kunibe\\effect\\stage_fire.c -> NewDemoStageFire()") \
    X(NewFlyingSmokeSlow, "40 56 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 ?? 8B 05", "MGS 2: Effect Speed Fix : NewBombEffect() -> user\\okajima\\effect3\\flying_smoke_slow.c -> NewFlyingSmokeSlow()") \
    X(NewHarrierLight, "48 8B C4 48 89 58 ?? 48 89 70 ?? 57 48 83 EC ?? 48 8B 51", "MGS 2: Effect Speed Fix : user\\kunibe\\effect\\harrier_light.c -> NewHarrierLight()") \
    X(NewLineSmoke, "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B D9 48 8B 49 ?? 48 85 C9", "MGS 2: Effect Speed Fix : user\\shibata\\effect\\line_smoke.c -> NewLineSmoke()") \
    X(NewHexagonalPattern, "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B F9 48 8D 51 ?? 8B 49", "MGS 2: Effect Speed Fix : user\\okuta\\effect\\hexagonal.c -> NewHexagonalPattern()") \
    X(NewSplushSurfaceMan, "48 8B C4 48 89 48 ?? 41 55", "MGS 2: Effect Speed Fix : user\\okajima\\effect2\\splush_surface_man.c -> NewSplushSurfaceMan() | OK_PutSplushSurface()") \
    X(NewSplushSurface2Man, "48 89 5C 24 ?? 48 89 74 24 ?? 48 89 4C 24", "MGS 2: Effect Speed Fix : user\\okajima\\effect2\\splush_surface_gravity_man.c -> NewSplushSurface2Man()") \
    X(NewTraffic_Flush, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 41 56 48 83 EC ?? 48 8B 51", "MGS 2: Effect Speed Fix : NewTraffic_Flush") \
    X(NewDebris_Tex, "40 57 48 83 EC ?? 48 89 5C 24 ?? 48 8B F9 48 89 6C 24", "MGS 2: Effect Speed Fix : user\\okajima\\effect2\\debris_tex.c -> NewDebris_Tex()") \
    X(NewFortSplineBulletDemo, NEW_FORT_SPLINE_BULLET_DEMO_PATTERN, "MGS 2: Effect Speed Fix : user\\morita\\demo_fort\\fort_b_line.c() -> Act()")

// Same, but these also run during gameplay firing, so not gated to cutscenes.
#define MGS2_RAILGUN_PLAYTIME_SKIPS_ALWAYS(X) \
    X(RailgunTrailMed, "48 8B C4 48 89 58 ?? 48 89 70 ?? 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 0F 29 70 ?? 0F 29 78 ?? 44 0F 29 40 ?? 44 0F 29 48 ?? 44 0F 29 50 ?? 44 0F 29 98 ?? ?? ?? ?? 44 0F 29 A0 ?? ?? ?? ?? 44 0F 29 6C 24", "MGS 2: Effect Speed Fix : Fortune railgun demo effect Act (med)") \
    X(RailgunTrailA, "4C 8B DC 49 89 5B ?? 49 89 7B ?? 55 49 8D 6B ?? 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 ?? 66 83 B9 ?? ?? ?? ?? ?? 48 8B F9 0F 85 ?? ?? ?? ?? 8B 05 ?? ?? ?? ?? 33 DB", "MGS 2: Effect Speed Fix : Fortune railgun trail Act A") \
    X(RailgunTrailB, "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 8B 41 ?? 48 8B D9", "MGS 2: Effect Speed Fix : Fortune railgun trail Act B")

#define DEFINE_MGS2_PLAYTIME_ALWAYS_HOOK(name, pattern, label)      \
    SafetyHookInline name##_hook {};                                \
    static int64_t name##_Hook(int64_t work)                        \
    {                                                               \
        if (MGS2_LinkVarBuf::linkvarbuf && ((MGS2_LinkVarBuf::GM_StagePlayTime & 1) == 0)) \
        {                                                           \
            return 0;                                               \
        }                                                           \
        return name##_hook.call<int64_t>(work);                     \
    }

// DG_FrameCount==2 = the game's own "this section ran 30fps on PS2" flag. More reliable
// than InCutscene() - demo blips set 1, real 30fps windows set 2.
inline int* g_pWindowFrameCount = nullptr;
inline bool In30fpsWindow() { return g_pWindowFrameCount && *g_pWindowFrameCount == 2; }

// Ribbons born inside a real 30fps window, latched at spawn. Fixed-size and alloc-free on
// purpose: hook bodies must never allocate. Slot collisions just drop a latch early (the
// ribbon decays at 60Hz from there) - never a crash.
inline uintptr_t g_windowBornRibbons[64] = {};
inline void RibbonSetWindowBorn(uintptr_t work, bool born)
{
    const size_t slot = (work >> 4) & 63;
    if (born) g_windowBornRibbons[slot] = work;
    else if (g_windowBornRibbons[slot] == work) g_windowBornRibbons[slot] = 0;
}
inline bool RibbonIsWindowBorn(uintptr_t work) { return g_windowBornRibbons[(work >> 4) & 63] == work; }

inline void HookRailgunVortexRate(HMODULE baseModule)
{
    // NewPlasmaEvade: reveal/fade are authored per 30fps frame and never tick-adjusted.
    uint8_t* act = Memory::PatternScan(baseModule,
        "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B F1 C7 44 24 ?? 00 00 00 00",
        "MGS 2: Effect Speed Fix : NewPlasmaEvade Act");
    if (!act) return;
    // Single-buffered prims - correct the rates inside the update instead of skipping the Act.
    if (memcmp(act + 0xF9, "\xF3\x0F\x11\x83\x24\x01\x00\x00", 8) != 0)
    {
        spdlog::error("MGS 2: Effect Speed Fix: plasma fade store no longer matches; skipping.");
        return;
    }
    static SafetyHookMid plasmaFadeHook{};
    plasmaFadeHook = safetyhook::create_mid(act + 0xF9,   // about to store fAlphaMax * 0.9
        [](SafetyHookContext& ctx)
        {
            if (g_GameVars.InCutscene() && (*reinterpret_cast<const int32_t*>(ctx.rdi + 0x10) & 1))
                ctx.xmm0.f32[0] *= 1.0f / 0.9f;   // net no-op this frame - fade at 30fps
        });
    LOG_HOOK(plasmaFadeHook, "MGS 2: Effect Speed Fix : NewPlasmaEvade 30fps fade")

    uint8_t* drawing2 = Memory::PatternScan(baseModule,
        "48 83 EC ?? 8B 69 ?? 33 F6 03 69",
        "MGS 2: Effect Speed Fix : NewPlasmaEvade reveal step");
    if (!drawing2) return;
    static SafetyHookMid plasmaRevealHook{};
    plasmaRevealHook = safetyhook::create_mid(drawing2 + 0xC,   // ebp = nDrawNum + nDrawSpd
        [](SafetyHookContext& ctx)
        {
            if (g_GameVars.InCutscene() && (*reinterpret_cast<const int32_t*>(ctx.rcx + 0x10) & 1))
                ctx.rbp -= *reinterpret_cast<const int32_t*>(ctx.rcx + 0xC);   // cancel this frame's step
        });
    LOG_HOOK(plasmaRevealHook, "MGS 2: Effect Speed Fix : NewPlasmaEvade 30fps reveal")

    // The vortex ribbons draw per frame; hold only the life decrement.
    uint8_t* vortexAct = Memory::PatternScan(baseModule,
        "48 81 EC ?? ?? ?? ?? 83 3D ?? ?? ?? ?? 00 4C 8B C1",
        "MGS 2: Effect Speed Fix : Fortune railgun vortex ribbons Act");
    if (!vortexAct) return;
    if (memcmp(vortexAct + 0x3E, "\x41\x89\x80\x70\x01\x00\x00", 7) != 0)
    {
        spdlog::error("MGS 2: Effect Speed Fix: vortex ribbon life offset no longer matches; skipping.");
        return;
    }
    static SafetyHookMid vortexLifeHook{};
    vortexLifeHook = safetyhook::create_mid(vortexAct + 0x3E,   // about to store life-1
        [](SafetyHookContext& ctx)
        {
            // Only window-born ribbons take the 30fps hold; latched at spawn so demo blips
            // ending mid-flight cannot disengage it. Everything else uses the 0xF0 init below.
            if (!RibbonIsWindowBorn(ctx.r8)) return;
            static std::unordered_map<uintptr_t, uint32_t> s_ticks;
            if (s_ticks.size() > 64) s_ticks.clear();
            // Native decay for the tail so the impact smoke dies before the camera cut.
            if (static_cast<int32_t>(ctx.rcx & 0xFFFFFFFF) > 96 && (++s_ticks[ctx.r8] & 1) == 0)
                ctx.rax = static_cast<uint32_t>(ctx.rcx);   // hold the decrement this frame
        });
    LOG_HOOK(vortexLifeHook, "MGS 2: Effect Speed Fix : vortex ribbon 30fps life")

    // Swirl phase follows the life value, so extend the span instead of holding the decrement.
    if (memcmp(vortexAct + 0xA99, "\xC7\x81\x70\x01\x00\x00\x78\x00\x00\x00", 10) == 0)
    {
        static SafetyHookMid vortexInitHook{};
        vortexInitHook = safetyhook::create_mid(vortexAct + 0xAA3,   // right after: mov [rcx+0x170], 0x78
            [](SafetyHookContext& ctx)
            {
                // Window-born: keep 0x78 + hold + native tail (verified polygon-demo look).
                // Fight blips and gameplay: 0xF0 @ 60Hz = 4.0s = PS2's 0x78 @ 30Hz.
                const bool windowBorn = In30fpsWindow();
                RibbonSetWindowBorn(ctx.rcx, windowBorn);
                if (!windowBorn)
                    *reinterpret_cast<int32_t*>(ctx.rcx + 0x170) = 0xF0;
            });
        LOG_HOOK(vortexInitHook, "MGS 2: Effect Speed Fix : vortex ribbon gameplay life span")
    }
    else
        spdlog::error("MGS 2: Effect Speed Fix: vortex ribbon life init no longer matches; skipping.");

    // Ribbon alpha scale (48.0) - hooked at the load; the vertex loop below has a branch target.
    if (memcmp(vortexAct + 0x7F0, "\x4C\x8D\x3D", 3) == 0)
    {
        static SafetyHookMid vortexAlphaHook{};
        vortexAlphaHook = safetyhook::create_mid(vortexAct + 0x7F0,   // right after: movss xmm7, [rip] (=48.0)
            [](SafetyHookContext& ctx)
            {
                ctx.xmm7.f32[0] *= In30fpsWindow() ? 0.5f : 0.125f;   // stable through fight blips
            });
        LOG_HOOK(vortexAlphaHook, "MGS 2: Effect Speed Fix : vortex ribbon gameplay alpha")
    }
    else
        spdlog::error("MGS 2: Effect Speed Fix: vortex ribbon alpha scale offset no longer matches; skipping.");

    // The bolt Act also runs during gameplay firing.
    static SafetyHookInline boltHook{};
    uint8_t* bolt = Memory::PatternScan(baseModule,
        "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B 99 ?? ?? ?? ?? 48 8B F1 8B 0D",
        "MGS 2: Effect Speed Fix : liner_gun_plasma bolt Act");
    if (bolt)
    {
        static void (*boltThunk)(int64_t) = nullptr;
        boltHook = safetyhook::create_inline(reinterpret_cast<void*>(bolt),
            static_cast<void (*)(int64_t)>([](int64_t work)
            {
                // Hold flight bolts only; the 128-tick impact bolt reads better at native decay.
                const int lifeMax = work ? *reinterpret_cast<const int32_t*>(work + 0x88) : 0;
                if (lifeMax <= 64 && MGS2_LinkVarBuf::linkvarbuf &&
                    ((static_cast<int>(MGS2_LinkVarBuf::GM_StagePlayTime) & 1) == 0))
                    return;
                boltHook.call(work);
            }));
        LOG_HOOK(boltHook, "MGS 2: Effect Speed Fix : liner_gun_plasma bolt 30fps (gameplay + demo)")
    }

    // ElectroField must run every frame; its x0.97 fade is authored for 30fps.
    uint8_t* field = Memory::PatternScan(baseModule,
        "48 8B C4 53 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 0F 29 70 ?? 48 8D B9",
        "MGS 2: Effect Speed Fix : NewElectroField Act");
    if (!field) return;
    if (memcmp(field + 0x33E, "\xF3\x0F\x59\x05", 4) != 0)
    {
        spdlog::error("MGS 2: Effect Speed Fix: electro field fade offset no longer matches; skipping.");
        return;
    }
    static SafetyHookMid fieldFadeHook{};
    fieldFadeHook = safetyhook::create_mid(field + 0x346,   // right after: mulss xmm0, [rip] (=0.97)
        [](SafetyHookContext& ctx)
        {
            static bool s_fieldPhase = false;
            s_fieldPhase = !s_fieldPhase;
            if (g_GameVars.InCutscene() && s_fieldPhase)
                ctx.xmm0.f32[0] *= 1.0f / 0.97f;   // net no-op this frame - fade at 30fps
        });
    LOG_HOOK(fieldFadeHook, "MGS 2: Effect Speed Fix : NewElectroField 30fps fade")

    // liner_gun_plasma_flush: the port NaN-poisons the arc's node fractal, so only a stub ever
    // rendered. Rebuild it in a private scratch and skip the game's builder.
    {
        uint8_t* scrAct = Memory::PatternScan(baseModule,
            "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B 99 ?? ?? ?? ?? 48 8B E9",
            "MGS 2: Effect Speed Fix : arc private scratch (act)");
        uint8_t* scrRes = Memory::PatternScan(baseModule,
            "48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 56 48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 ?? 48 8B F9 48 8D 51",
            "MGS 2: Effect Speed Fix : arc private scratch (res)");
        bool ok = scrAct && scrRes;
        uint8_t* buf = nullptr;
        if (ok)
        {
            for (uintptr_t offs = 0x02000000; offs < 0x70000000 && !buf; offs += 0x01000000)
                buf = static_cast<uint8_t*>(VirtualAlloc(
                    reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(baseModule) + offs),
                    0x2000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
            ok = buf != nullptr;
        }
        const int actLea[] = { 0x2F, 0x79, 0x8C, 0xAB, 0xCC };
        const int resLea[] = { 0x140, 0x17F, 0x225, 0x233, 0x24A, 0x268, 0x287, 0x28E, 0x2A1, 0x2C1 };
        if (ok)
        {
            const auto repoint = [&](uint8_t* insn, int dispOfs, int len, uint8_t* target) -> bool
            {
                const intptr_t disp = static_cast<intptr_t>(target - (insn + len));
                if (disp != static_cast<int32_t>(disp)) return false;
                Memory::Write(reinterpret_cast<uintptr_t>(insn + dispOfs), static_cast<int32_t>(disp));
                return true;
            };
            for (int o : actLea)
                ok = ok && scrAct[o] == 0x48 && scrAct[o+1] == 0x8D && repoint(scrAct + o, 3, 7, buf);
            for (int o : resLea)
                ok = ok && scrRes[o] == 0x48 && scrRes[o+1] == 0x8D && repoint(scrRes + o, 3, 7, buf);
            ok = ok && scrRes[0x135] == 0x48 && scrRes[0x136] == 0xC7 && repoint(scrRes + 0x135, 3, 11, buf);          // node[0].vx/vy = 0
            ok = ok && scrRes[0x147] == 0xC7 && scrRes[0x148] == 0x05 && repoint(scrRes + 0x147, 2, 10, buf + 8);      // node[0].vz = 0
            ok = ok && scrRes[0x153] == 0x48 && scrRes[0x154] == 0xC7 && repoint(scrRes + 0x153, 3, 11, buf + 0xE00);  // node[224].vx/vy = 0
            ok = ok && scrRes[0x16C] == 0xF3 && scrRes[0x16E] == 0x11 && repoint(scrRes + 0x16C, 4, 8, buf + 0xE08);   // node[224].vz = len
        }
        if (ok)
        {
            static uint8_t* s_arcScratch = nullptr;
            s_arcScratch = buf;
            struct ArcVec { float x, y, z, w; };
            struct ArcFractal
            {
                static float Rnd()
                {
                    static uint32_t s = 0x2545F491;
                    s = s * 0x5D588B65 + 1;
                    return static_cast<float>(s) * (1.0f / 4294967296.0f);
                }
                static void Build(ArcVec* v, int n0, int n1)
                {
                    if (n1 - n0 <= 1) return;
                    const int nc = (n0 + n1) / 2;
                    ArcVec& f0 = v[n0]; ArcVec& f1 = v[n1]; ArcVec& f2 = v[nc];
                    f0.w = f1.y - f0.y;
                    f1.w = f1.z - f0.z;
                    f2.w = f1.w * 0.25f * (Rnd() * 2.0f - 1.0f);
                    float t = (f1.w != 0.0f) ? f0.w / f1.w : 0.0f;
                    if (t != t) t = 0.0f; else if (t > 1.0f) t = 1.0f; else if (t < -1.0f) t = -1.0f;
                    const float th = asinf(t);
                    f2.x = (f1.x - f0.x) * 0.5f + f0.x + f2.w * (Rnd() * 2.0f - 1.0f);
                    f2.y = f0.w * 0.5f + f0.y + f2.w * cosf(th);
                    f2.z = f1.w * 0.5f + f0.z - f2.w * sinf(th);
                    Build(v, n0, nc);
                    Build(v, nc, n1);
                }
            };
            static SafetyHookMid arcInitHook{};
            arcInitHook = safetyhook::create_mid(scrRes + 0x174,   // at: call CalcInitNode
                [](SafetyHookContext& ctx)
                {
                    ArcFractal::Build(reinterpret_cast<ArcVec*>(s_arcScratch), 0, 224);
                    ctx.rip += 5;   // skip the game's builder
                });
            LOG_HOOK(arcInitHook, "MGS 2: Effect Speed Fix : arc fractal replacement")
        }
        // Spawn knob: drop every third arc (callers treat null as a failed allocation).
        uint8_t* arcNew = Memory::PatternScan(baseModule,
            "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 45 33 C9 41 8B E8 48 8B FA 48 8B F1 BA ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 41 8D 49 ?? E8 ?? ?? ?? ?? 48 8B D8 48 85 C0 0F 84 ?? ?? ?? ?? 4C 8D 05 ?? ?? ?? ?? 48 8B C8 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 81 4B ?? ?? ?? ?? ?? 33 C0 48 89 43 ?? 8B CD",
            "MGS 2: Effect Speed Fix : arc spawn rate");
        if (ok && arcNew)
        {
            static uint8_t* s_nullRet = nullptr;
            s_nullRet = buf + 0x1FF0;
            s_nullRet[0] = 0x33; s_nullRet[1] = 0xC0; s_nullRet[2] = 0xC3;   // xor eax,eax; ret
            static SafetyHookMid arcRateHook{};
            arcRateHook = safetyhook::create_mid(arcNew,
                [](SafetyHookContext& ctx)
                {
                    static uint32_t s_n = 0;
                    if (++s_n % 3 == 0)
                        ctx.rip = reinterpret_cast<uintptr_t>(s_nullRet);
                });
            LOG_HOOK(arcRateHook, "MGS 2: Effect Speed Fix : arc spawn rate")
        }
        if (ok)
            spdlog::info("MGS 2: Effect Speed Fix: arc private scratch installed.");
        else
            spdlog::error("MGS 2: Effect Speed Fix: arc private scratch sites mismatch; skipping.");
    }

    // The arc's linear alpha ramp crushes to black on sRGB displays; fade only the tip quarter.
    uint8_t* arcAct = Memory::PatternScan(baseModule,
        "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B 99 ?? ?? ?? ?? 48 8B E9",
        "MGS 2: Effect Speed Fix : muzzle arc Act");
    if (arcAct && memcmp(arcAct + 0x14C, "\x41\x88\x50\xE0", 4) == 0)
    {
        static SafetyHookMid arcRampHook{};
        arcRampHook = safetyhook::create_mid(arcAct + 0x14C,   // dl = k*i/224, r9d = k
            [](SafetyHookContext& ctx)
            {
                const uint32_t a = static_cast<uint32_t>(ctx.rdx & 0xFF) * 4;
                const uint32_t k = static_cast<uint32_t>(ctx.r9 & 0xFF);
                ctx.rdx = (ctx.rdx & ~0xFFull) | static_cast<uint8_t>(a < k ? a : k);
            });
        LOG_HOOK(arcRampHook, "MGS 2: Effect Speed Fix : muzzle arc tail visibility")
    }
    else if (arcAct)
        spdlog::error("MGS 2: Effect Speed Fix: muzzle arc ramp offset no longer matches; skipping.");

    // Tazer arc growth compounds per frame; square-root the factors so 60fps matches 30fps.
    uint8_t* mini = Memory::PatternScan(baseModule,
        "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B 99 ?? ?? ?? ?? 48 8B F9",
        "MGS 2: Effect Speed Fix : tazer arc Act");
    if (mini &&
        memcmp(mini + 0x274, "\xF3\x0F\x10\x35", 4) == 0 &&
        memcmp(mini + 0x304, "\xF3\x0F\x10\x35", 4) == 0 &&
        memcmp(mini - 0x0E, "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC", 14) == 0)
    {
        Memory::Write(reinterpret_cast<uintptr_t>(mini - 0x0C), 1.0049875f);   // sqrt(1.01)
        Memory::Write(reinterpret_cast<uintptr_t>(mini - 0x08), 1.0344080f);   // sqrt(1.07)
        const auto retarget = [&](uint8_t* insn, uint8_t* target)
        {
            const int32_t disp = static_cast<int32_t>(target - (insn + 8));
            Memory::Write(reinterpret_cast<uintptr_t>(insn + 4), disp);
        };
        retarget(mini + 0x274, mini - 0x0C);
        retarget(mini + 0x304, mini - 0x08);
        spdlog::info("MGS 2: Effect Speed Fix: tazer arc growth rebased to 30fps.");
    }
    else if (mini)
        spdlog::error("MGS 2: Effect Speed Fix: tazer growth sites no longer match; skipping.");

    // Size knob: arc dot width = 5 * resY / 448. Dots must overlap to fuse into a stroke.
    uint8_t* arcTemplate = Memory::PatternScan(baseModule,
        "?? ?? 20 ?? ?? 20 C6 42 ?? 40",
        "MGS 2: Effect Speed Fix : flush sprite template");
    if (arcTemplate)
    {
        static SafetyHookMid arcSizeHook{};
        arcSizeHook = safetyhook::create_mid(arcTemplate,   // rdx/rcx = the two vertex slots, wh just stored
            [](SafetyHookContext& ctx)
            {
                const int resY = CustomResolutionAndBorderless::iInternalResY;
                if (resY <= 448) return;
                int size = 5 * resY / 448;
                if (size > 32) size = 32;
                static bool s_logged = false;
                if (!s_logged)
                {
                    s_logged = true;
                    spdlog::info("MGS 2: Effect Speed Fix: flush dot size {} (resY {})", size, resY);
                }
                const int16_t v = static_cast<int16_t>(size);
                *reinterpret_cast<int16_t*>(ctx.rdx + 0x10) = v;
                *reinterpret_cast<int16_t*>(ctx.rdx + 0x12) = v;
                *reinterpret_cast<int16_t*>(ctx.rcx + 0x10) = v;
                *reinterpret_cast<int16_t*>(ctx.rcx + 0x12) = v;
            });
        LOG_HOOK(arcSizeHook, "MGS 2: Effect Speed Fix : flush sprite size")
    }

    // liner_plasma_small counts life in raw frames; hold the decrement every other frame.
    uint8_t* mini2 = Memory::PatternScan(baseModule,
        "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B 99 ?? ?? ?? ?? 48 8B F9",
        "MGS 2: Effect Speed Fix : liner_plasma_small Act");
    if (mini2 && memcmp(mini2 + 0x221, "\x8B\x47\x7C\x85\xC0", 5) == 0)
    {
        static uint8_t* s_miniSkip = nullptr;
        s_miniSkip = mini2 + 0x237;   // past the decrement and destroy check
        static SafetyHookMid miniLifeHook{};
        miniLifeHook = safetyhook::create_mid(mini2 + 0x221,
            [](SafetyHookContext& ctx)
            {
                static std::unordered_map<uintptr_t, uint32_t> s_ticks;
                if (s_ticks.size() > 64) s_ticks.clear();
                if (*reinterpret_cast<const int32_t*>(ctx.rdi + 0x7C) > 0 &&
                    (++s_ticks[ctx.rdi] & 1) == 0)
                    ctx.rip = reinterpret_cast<uintptr_t>(s_miniSkip);
            });
        LOG_HOOK(miniLifeHook, "MGS 2: Effect Speed Fix : liner_plasma_small 60fps life")
    }
    else if (mini2)
        spdlog::error("MGS 2: Effect Speed Fix: liner_plasma_small life offset no longer matches; skipping.");

}

namespace
{

    ///PS2's I/O subprocessor clock speed in MHz. If an effect ran via a loop on the PS2, it was likely limited by this clock speed as opposed to running at 30 FPS.
    constexpr double PS2_IOP_CLOCKSPEED = 36.864;
    constexpr double FRAME_IOP_DIVIDER = PS2_IOP_CLOCKSPEED / 60.0;
    constexpr double FRAME_IOP_MULTIPLIER = 60.0 / PS2_IOP_CLOCKSPEED;

    constexpr char NEW_FORT_SPLINE_BULLET_DEMO_PATTERN[] = "48 89 5C 24 ?? 55 56 57 41 56 41 57 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 ?? 48 8B 51 ?? 33 ED 48 8B F1 4C 89 A4 24 ?? ?? ?? ?? 41 BE ?? ?? ?? ?? 44 8B FD 41 8B C6 44 8B CD 48 63 8A ?? ?? ?? ?? 44 8D 65 ?? 2B C1 48 8B BC CA ?? ?? ?? ?? 89 82 ?? ?? ?? ?? 48 98 4C 8B 84 C2 ?? ?? ?? ?? 66 0F 1F 84 00 ?? ?? ?? ?? 0F B6 57 ?? 41 88 50 ?? 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 44 3B C9 7D ?? 41 C6 40 ?? ?? B2 ?? 44 0F B6 DA EB ?? 44 0F B6 DA 44 3B C8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D8 45 88 58 ?? 0F B6 57 ?? 41 8D 59 ?? 41 88 50 ?? 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 3B D9 7D ?? 41 C6 40 ?? ?? B2 ?? 44 0F B6 D2 EB ?? 44 0F B6 D2 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D0 45 88 50 ?? 0F B6 57 ?? 41 8D 59 ?? 41 88 50 ?? 45 0A D3 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 3B D9 7D ?? 41 C6 40 ?? ?? B2 ?? 44 0F B6 DA EB ?? 44 0F B6 DA 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D8 45 88 58 ?? 0F B6 57 ?? 41 8D 59 ?? 41 88 50 ?? 45 0A DA 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 3B D9 7D ?? 41 C6 40 ?? ?? B2 ?? 44 0F B6 D2 EB ?? 44 0F B6 D2 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D0 45 88 50 ?? 0F B6 57 ?? 41 8D 59 ?? 41 88 50 ?? 45 0A D3 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 3B D9 7D ?? 41 C6 40 ?? ?? B2 ?? 44 0F B6 DA EB ?? 44 0F B6 DA 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D8 45 88 58 ?? 0F B6 57 ?? 41 8D 59 ?? 41 88 50 ?? 45 0A DA 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 3B D9 7D ?? 41 C6 40 ?? ?? B2 ?? 44 0F B6 D2 EB ?? 44 0F B6 D2 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D0 45 88 50 ?? 0F B6 57 ?? 41 8D 59 ?? 41 88 50 ?? 45 0A D3 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 3B D9 7D ?? 41 C6 40 ?? ?? B2 ?? 44 0F B6 DA EB ?? 44 0F B6 DA 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D8 45 88 58 ?? 0F B6 57 ?? 45 0A DA 41 88 50 ?? 45 8D 51 ?? 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 44 3B D1 7D ?? 41 C6 40 ?? ?? B2 ?? 0F B6 DA EB ?? 0F B6 DA 44 3B D0 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 2A D8 41 88 58 ?? 0F B6 97 ?? ?? ?? ?? 41 0A DB 41 88 90 ?? ?? ?? ?? 45 8D 59 ?? 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 44 3B D9 7D ?? 41 C6 80 ?? ?? ?? ?? ?? B2 ?? 44 0F B6 D2 EB ?? 44 0F B6 D2 44 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 44 2A D0 45 88 90 ?? ?? ?? ?? 0F B6 97 ?? ?? ?? ?? 45 8D 59 ?? 41 88 90 ?? ?? ?? ?? 44 0A D3 8B 46 ?? 85 C0 79 ?? 83 C0 ?? ?? ?? ?? 44 3B D9 7D ?? 41 C6 80 ?? ?? ?? ?? ?? B2 ?? 0F B6 CA EB ?? 0F B6 CA 44 3B D8 7D ?? 84 D2 74 ?? 41 3A D4 8B C2 41 0F 47 C4 2A C8 41 88 88 ?? ?? ?? ?? 0F B6 C9 48 81 C7 ?? ?? ?? ?? 41 0F B6 C2 49 81 C0 ?? ?? ?? ?? 0B C8 41 83 C1 ?? 44 0B F9 41 83 F9 ?? 0F 8C ?? ?? ?? ?? 8B 46 ?? 4C 8B A4 24 ?? ?? ?? ?? 85 C0 0F 88 ?? ?? ?? ?? 4C 8B 46 ?? 0F 29 B4 24 ?? ?? ?? ?? 0F 29 BC 24 ?? ?? ?? ?? 44 0F 29 84 24 ?? ?? ?? ?? 49 63 88 ?? ?? ?? ?? F3 44 0F 10 05 ?? ?? ?? ?? 44 2B F1 44 0F 29 8C 24 ?? ?? ?? ?? F3 44 0F 10 0D ?? ?? ?? ?? 49 8B 94 C8 ?? ?? ?? ?? 44 0F 29 9C 24 ?? ?? ?? ?? F3 44 0F 10 1D ?? ?? ?? ?? 44 0F 29 A4 24 ?? ?? ?? ?? F3 44 0F 10 25 ?? ?? ?? ?? 44 0F 29 AC 24 ?? ?? ?? ?? 45 0F 57 ED 49 63 C6 45 89 B0 ?? ?? ?? ?? 4C 8D 72 ?? 44 0F 29 B4 24 ?? ?? ?? ?? F3 44 0F 10 35 ?? ?? ?? ?? 49 8B 9C C0 ?? ?? ?? ?? 49 8B BC C0 ?? ?? ?? ?? 48 83 C3 ?? 44 0F 29 7C 24 ?? F3 44 0F 10 3D ?? ?? ?? ?? 44 0F 29 94 24 ?? ?? ?? ?? 0F 1F 40 ?? 3B 6E ?? 0F 8D ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 84 C0 0F 84 ?? ?? ?? ?? 8B 46 ?? 0F 57 C0 F3 44 0F 10 15 ?? ?? ?? ?? 0F 57 F6 2B C5 0F 57 FF F3 0F 2A FD F3 0F 2A C0 48 8B 05 ?? ?? ?? ?? 0F B6 88 ?? ?? ?? ?? F3 0F 59 05 ?? ?? ?? ?? F3 0F 2A F1 F3 44 0F 5D D0 F3 41 0F 59 F1 F3 0F 59 35 ?? ?? ?? ?? F3 0F 58 F7 0F 28 C6 F3 41 0F 58 C6 E8 ?? ?? ?? ?? 41 0F 2F F1 F3 41 0F 59 C2 F3 0F 2C C0 66 89 43 ?? 76 ?? 66 66 0F 1F 84 00 ?? ?? ?? ?? F3 41 0F 58 F3 41 0F 2F F1 77 ?? 44 0F 2F C6 76 ?? F3 41 0F 58 F4 44 0F 2F C6 77 ?? 41 0F 2F F6 76 ?? 41 0F 28 C1 F3 0F 5C C6 0F 28 F0 F3 0F 10 05 ?? ?? ?? ?? 0F 2F C6 76 ?? 41 0F 28 C0 F3 0F 5C C6 0F 28 F0 0F 28 C6 E8 ?? ?? ?? ?? F3 41 0F 59 C2 F3 0F 2C C0 66 89 43 ?? 48 8B 05 ?? ?? ?? ?? 8B 88 ?? ?? ?? ?? 03 CD F6 C1 ?? 75 ?? ?? ?? 8B 46 ?? 2B C5 66 0F 6E C0 0F 5B C0 0F 2F 05 ?? ?? ?? ?? 76 ?? 41 0F 28 CD EB ?? F3 0F 59 05 ?? ?? ?? ?? F3 0F 10 0D ?? ?? ?? ?? F3 0F 5C C8 F3 0F 59 3D ?? ?? ?? ?? 4C 8D 46 ?? F3 0F 5C F9 44 0F 2F EF 76 ?? 41 0F 28 FF 41 0F 2F 78 ?? 76 ?? 49 83 C0 ?? 41 0F 2F 78 ?? 77 ?? 49 8D 50 ?? 45 33 C9 48 8D 4C 24 ?? E8 ?? ?? ?? ?? 0F 28 CF 48 8D 4C 24 ?? 4C 8B C7 E8 ?? ?? ?? ?? 8B 46 ?? 2B C5 66 0F 6E C0 0F 5B C0 F3 0F 5E 05 ?? ?? ?? ?? F3 0F 58 47 ?? F3 0F 11 47 ?? ?? ?? ?? 49 83 C6 ?? 44 0B F8 48 83 C3 ?? 48 83 C7 ?? FF C5 83 FD ?? 0F 8C ?? ?? ?? ?? 44 0F 28 7C 24 ?? 44 0F 28 B4 24 ?? ?? ?? ?? 44 0F 28 AC 24 ?? ?? ?? ?? 44 0F 28 A4 24 ?? ?? ?? ?? 44 0F 28 9C 24 ?? ?? ?? ?? 44 0F 28 94 24 ?? ?? ?? ?? 44 0F 28 8C 24 ?? ?? ?? ?? 44 0F 28 84 24 ?? ?? ?? ?? 0F 28 BC 24 ?? ?? ?? ?? 0F 28 B4 24 ?? ?? ?? ?? 45 85 FF 75 ?? 48 8B CE E8 ?? ?? ?? ?? 83 46 ?? ?? EB ?? FF C0 89 46 ?? 48 8B 4C 24 ?? 48 33 CC E8 ?? ?? ?? ?? 48 8B 9C 24 ?? ?? ?? ?? 48 81 C4 ?? ?? ?? ?? 41 5F 41 5E 5F 5E 5D C3 ?? ?? ?? ?? ?? ?? ?? ?? 48 83 EC ?? F3 0F 10 69 ?? ?? ?? ?? 0F 29 74 24 ?? 0F 28 F1 F3 0F 5C 71 ?? 0F 28 DE F3 0F 59 DE 0F 28 E3 0F 28 C3 F3 0F 59 41 ?? F3 0F 59 E6 0F 28 CC 0F 28 D4 F3 0F 59 49 ?? F3 0F 58 C8 0F 28 C6 F3 0F 59 41 ?? F3 0F 5E CD F3 0F 58 C8 0F 28 C3 F3 0F 5E CD ?? ?? ?? ?? ?? ?? ?? ?? ?? F3 0F 59 51 ?? ?? ?? ?? F3 0F 59 41 ?? F3 0F 58 D0 0F 28 C6 F3 0F 59 41 ?? F3 0F 5E D5 F3 0F 58 D0 0F 28 C6 F3 0F 5E D5 F3 0F 58 50 ?? F3 41 0F 11 50 ?? F3 0F 59 61 ?? ?? ?? ?? F3 0F 59 59 ?? F3 0F 59 41 ?? F3 0F 58 E3 F3 0F 5E E5 F3 0F 58 E0 F3 0F 5E E5 F3 0F 58 60 ?? 41 C7 40 ?? ?? ?? ?? ?? F3 41 0F 11 60 ?? 0F 2F 71 ?? 72 ?? 48 8B 51 ?? 45 33 C9 4C 8D 42 ?? E8 ?? ?? ?? ?? 0F 28 74 24 ?? 48 83 C4 ?? C3 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 48 83 EC ?? F3 0F 10 2D ?? ?? ?? ?? F3 41 0F 10 48 ?? 0F 28 D5 F3 0F 10 25 ?? ?? ?? ?? 0F 28 C1 0F 29 74 24 ?? F3 0F 58 C5 F3 41 0F 10 70 ?? 0F 28 DD 0F 29 7C 24 ?? 0F 28 FD 44 0F 29 44 24 ?? F3 0F 5C FE ?? ?? ?? ?? ?? F3 0F 58 F5 44 0F 28 CD F3 41 0F 5C 50 ?? F3 0F 5C 5A ?? F3 0F 59 FA F3 0F 59 F2 F3 0F 10 52 ?? F3 0F 59 F8 0F 28 C5 F3 0F 5C C1 F3 0F 10 4A ?? F3 44 0F 5C C9 44 0F 28 C1 F3 0F 59 FC F3 44 0F 58 C5 F3 0F 59 F0 0F 28 C2 F3 44 0F 59 CB F3 0F 58 C5 F3 44 0F 59 C3 F3 0F 5C EA F3 0F 59 F4 F3 44 0F 59 C0 F3 44 0F 59 CD F3 44 0F 59 C4 F3 44 0F 59 CC 45 85 C9 74 ?? 66 41 0F 6E E9 0F 5B ED F3 0F 11 69";

    uintptr_t rain_slow_copyback_addr = static_cast<uintptr_t>(0);

    // g_pWindowFrameCount / In30fpsWindow() are defined at file scope above
    // HookRailgunVortexRate (the vortex hooks share them); resolved in Initialize().
    bool SkipFrame()
    {
        // Allow skipping in any cutscene, not just flagged 30fps windows - slow-running
        // PS2 demos can skip too for the right feel, even when authored at 60.
        return g_GameVars.InCutscene() && (g_GameVars.DG_Clock() & 1) != 0;
    }

    bool SkipFrameWindow()
    {
        // Real 30fps windows only - for the window countdown itself.
        return In30fpsWindow() && (g_GameVars.DG_Clock() & 1) != 0;
    }

    // Kamome (seagull) demo pacing. PS2 ran the bird demos below 60fps, so birds moved and
    // flapped at ~half rate. Draw stays at full 60 (prims are double-buffered) - the rates
    // get halved instead. Gameplay birds untouched.

#define DEFINE_MGS2_FRAMESKIP_HOOK(name, pattern, label) \
    DEFINE_MGS2_CUTSCENE_FRAMESKIP_HOOK(name);

    MGS2_CUTSCENE_FRAMESKIP_INLINE_HOOKS(DEFINE_MGS2_FRAMESKIP_HOOK)

#undef DEFINE_MGS2_FRAMESKIP_HOOK

    MGS2_RAILGUN_PLAYTIME_SKIPS_ALWAYS(DEFINE_MGS2_PLAYTIME_ALWAYS_HOOK)

    uintptr_t cigaretteMouthSmokeSpawnAfterLoad = 0;
    uintptr_t cigaretteMouthSmokeSpawnAfterInit = 0;

    constexpr uint32_t CIGARETTE_MOUTH_SMOKE_ALPHA_RISE = 0x3D000000; // 0.03125f
    constexpr uint32_t CIGARETTE_MOUTH_SMOKE_ALPHA_FALL = 0xBC2AAAAB; // -0.010416667f
    constexpr uint32_t CIGARETTE_MOUTH_SMOKE_EMIT_FADE = 0xBB888889;  // -0.004166667f


    safetyhook::InlineHook h_KMM_Routine;
    safetyhook::InlineHook h_KMM_ActSystem;
    safetyhook::InlineHook h_MEMMOT_MakeMotion;
    safetyhook::InlineHook h_MEMMOT_MakeMotionSkip;

    inline bool g_kamomeMemmotHalfStep = false;

    // Desync the flock with a whole-frame phase offset. Whole frames only (kmtest.c checks
    // exact phase values). Looping motions only - fixed so birds not scheduled early at 60fps.
    void KamomeScatterPhase(void* mmt_ctrl)
    {
        uint8_t* mmt = static_cast<uint8_t*>(mmt_ctrl);
        float* phase = reinterpret_cast<float*>(mmt + 0x18);
        const int* loops = reinterpret_cast<const int*>(mmt + 0x14);
        if (*loops != 0 || *phase != 0.0f)
        {
            return;
        }
        const int idx = *reinterpret_cast<const int*>(mmt); // KMM_MOT_FLYING/FLYING03/HOVER/HOVER02
        if (idx != 2 && idx != 5 && idx != 7 && idx != 8)
        {
            return;
        }
        const uint8_t* bank = *reinterpret_cast<const uint8_t* const*>(mmt + 0x50);
        if (!bank)
        {
            return;
        }
        const uint16_t* lens = *reinterpret_cast<const uint16_t* const*>(bank + 0x20);
        if (!lens)
        {
            return;
        }
        const int half = lens[idx] / 2;
        if (half < 2)
        {
            return;
        }
        const uint32_t h = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(mmt) >> 4);
        *phase = static_cast<float>(static_cast<int>((h ^ (h >> 7)) & 0x7FFFFFFF) % half);
    }

    // Sample the pose every tick (skipping shows bind pose), advance the phase every other
    // tick at full step (fractional steps break kmact.c's IsEnd/nLoop checks).
    void __fastcall MEMMOT_MakeMotion_hook(void* mmt_ctrl)
    {
        if (g_kamomeMemmotHalfStep)
        {
            KamomeScatterPhase(mmt_ctrl);
            if ((g_GameVars.DG_Clock() & 1) != 0)
            {
                float* phase = reinterpret_cast<float*>(static_cast<uint8_t*>(mmt_ctrl) + 0x18);
                const float pre = *phase;
                h_MEMMOT_MakeMotion.call<void>(mmt_ctrl);
                if (*phase > pre) // undo plain advances, keep wraps
                {
                    *phase = pre;
                }
                return;
            }
        }
        h_MEMMOT_MakeMotion.call<void>(mmt_ctrl);
    }

    void __fastcall MEMMOT_MakeMotionSkip_hook(void* mmt_ctrl)
    {
        if (g_kamomeMemmotHalfStep)
        {
            KamomeScatterPhase(mmt_ctrl);
            if ((g_GameVars.DG_Clock() & 1) != 0)
            {
                return; // advance-only variant: plain hold
            }
        }
        h_MEMMOT_MakeMotionSkip.call<void>(mmt_ctrl);
    }

    // KMM_ActControl(): halve the movement deltas at the accumulation adds. Y comes from the
    // flap-height table (already phase-halved) so it needs nothing.
    safetyhook::MidHook h_KamomeDeltaScaleX;
    safetyhook::MidHook h_KamomeDeltaScaleZ;
    safetyhook::MidHook h_KamomeDeltaScaleW;
    void KamomeDeltaScaleX_hook(SafetyHookContext& ctx)
    {
        if (g_kamomeMemmotHalfStep)
        {
            ctx.xmm8.f32[0] *= 0.5f;
        }
    }
    void KamomeDeltaScaleZ_hook(SafetyHookContext& ctx)
    {
        if (g_kamomeMemmotHalfStep)
        {
            ctx.xmm6.f32[0] *= 0.5f;
        }
    }
    void KamomeDeltaScaleW_hook(SafetyHookContext& ctx)
    {
        if (g_kamomeMemmotHalfStep)
        {
            ctx.xmm2.f32[0] *= 0.5f;
        }
    }

    // KMM_ActControl()'s heading chase has no time compensation - at 60Hz turns finish 2x
    // fast and kmact.c's glide-entry check never passes. Hold it every other demo tick.
    uint8_t* g_pKamomeIntegrator = nullptr;
    safetyhook::MidHook h_KamomeTurnPace;
    void KamomeTurnPace_hook(SafetyHookContext& ctx)
    {
        if (g_kamomeMemmotHalfStep && (g_GameVars.DG_Clock() & 1) != 0)
        {
            ctx.rip = reinterpret_cast<uint64_t>(g_pKamomeIntegrator) + 0xB5;
        }
    }

    void __fastcall KMM_ActSystem_hook(void* kamome)
    {
        g_kamomeMemmotHalfStep = g_GameVars.InCutscene() || In30fpsWindow();
        h_KMM_ActSystem.call<void>(kamome);
        g_kamomeMemmotHalfStep = false;
    }

    uint8_t* g_pKMM_Routine = nullptr;
    safetyhook::MidHook h_KMM_Routine_ThinkGate;
    bool g_kamomeHoldThink = false;

    void KMM_Routine_ThinkGate_hook(SafetyHookContext& ctx)
    {
        if (!g_kamomeHoldThink)
        {
            return;
        }
        // Hold think modes 0/1 only. The order path must run every tick or demo orders get
        // cleared before pickup.
        const int mode = *reinterpret_cast<const int*>(ctx.rdi + 0x168);
        if (mode == 0 || mode == 1)
        {
            ctx.rip = reinterpret_cast<uint64_t>(g_pKMM_Routine) + 0x616;
        }
    }

    int64_t __fastcall KMM_Routine_hook(int64_t kamome)
    {
        // Gate think to every other demo tick - the movers all live in it, so this is what
        // halves movement. Head/tail (draw refresh, orders, SFX) still run every tick.
        const bool holdThink = g_GameVars.InCutscene() || In30fpsWindow()
            ? (g_GameVars.DG_Clock() & 1) != 0
            : false;
        // The head clears the LOD field and gated think can't re-set it - restore it on held
        // ticks or the models flicker.
        int32_t* lodMode = reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(kamome) + 0x134);
        const int32_t prevLod = *lodMode;
        int64_t result;
        if (holdThink)
        {
            g_kamomeHoldThink = true;
            result = h_KMM_Routine.call<int64_t>(kamome);
            g_kamomeHoldThink = false;
            *lodMode = prevLod;
        }
        else
        {
            result = h_KMM_Routine.call<int64_t>(kamome);
            if (!(g_GameVars.InCutscene() || In30fpsWindow()))
            {
                // Gameplay: max LOD. Vanilla freezes distant birds into a static pose - fine
                // at 480i, obvious at 4K.
                *lodMode = 0;
            }
        }
        return result;
    }

    /*
    uint8_t* g_pGateTo = nullptr;
    safetyhook::MidHook h_Act16_HairPhysicsGate;

    void Act16_HairPhysicsGate_hook(SafetyHookContext& ctx)
    {
        if (!SkipFrame())
        {
            return;
        }
        ctx.rip = reinterpret_cast<uint64_t>(g_pGateTo);
    }
*/

    uint8_t* g_pPlasmaGateTo = nullptr;
    safetyhook::MidHook h_Act389_PlasmaGate;

    void Act389_PlasmaGate_hook(SafetyHookContext& ctx)
    {
        if (!SkipFrame())
        {
            return;
        }
        ctx.rip = reinterpret_cast<uint64_t>(g_pPlasmaGateTo);
    }

    // The 30fps window countdown ticks per call - at 60fps windows expired in half their
    // real time. Hold it on skip ticks; Die still restores the divisor.
    SafetyHookInline h_DemoFrameCountAct{};
    int64_t __fastcall DemoFrameCountAct_hook(int64_t work)
    {
        if (SkipFrameWindow())
        {
            return 0;
        }
        return h_DemoFrameCountAct.call<int64_t>(work);
    }

    MGS2_FULL_SKIP_ACT_DIE_PAIRS(DEFINE_FULL_SKIP_HOOK)

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


// Re-enabled on the window clock. The old breakage was the Present-counter clock and
// whole-cutscene scope, not this hook - it resubmits the prim every frame.
SafetyHookInline d_splash_parts__c_Act_hook {};
// Struct offsets parsed from the matched Act bytes at install time (the pattern wildcards
// these displacements, so hardcoding them risks silent divergence between the twins/versions).
int32_t g_splashLifeDisp = -1;
int32_t g_splashPrimDisp = -1;
int32_t g_splashGroupDisp = -1;
void __fastcall MGS2_d_splash_parts__c_Act_hook(uint8_t* work)
{
    // Run ticks and the dead path go through the untouched original Act, so its own update,
    // draw and destroy logic (child cleanup included) stays fully authentic.
    if (!SkipFrame() || *reinterpret_cast<int32_t*>(work + g_splashLifeDisp) <= 0)
    {
        d_splash_parts__c_Act_hook.call(work);
        return;
    }

    // Held tick: only resubmit the prim group so the droplet stays visible without advancing.
    if (uint8_t* prim = *reinterpret_cast<uint8_t**>(work + g_splashPrimDisp))
    {
        *reinterpret_cast<int32_t*>(prim + g_splashGroupDisp) = MGS2_GameFuncs::GM_GetDGGroupID(g_GameVars.GM_CurrentStageMap());
    }
}

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

    // Resolve DG_FrameCount from NewDemoFrameCountCall's ctor (save-old + write-2 idiom).
    // No match = no cutscene half-rating at all, which is the safe fallback.
    if (uint8_t* windowWrite = Memory::PatternScan(baseModule,
        "89 4B 58 8B 05 ?? ?? ?? ?? 89 43 5C 48 8B C3 C7 05 ?? ?? ?? ?? 02 00 00 00",
        "MGS 2: Effect Speed Fix : scripted 30fps window flag (DG_FrameCount)"))
    {
        g_pWindowFrameCount = reinterpret_cast<int*>(Memory::GetRipRelativeAddress(windowWrite + 15, 2, 10));
        spdlog::info("MGS 2: Effect Speed Fix: 30fps window flag at {:s}+{:X}", sExeName.c_str(),
            reinterpret_cast<uintptr_t>(g_pWindowFrameCount) - reinterpret_cast<uintptr_t>(baseModule));
    }
    else
    {
        spdlog::error("MGS 2: Effect Speed Fix: 30fps window flag scan failed - cutscene speed corrections inactive this run.");
    }

    spdlog::info("MGS2: Effect Speed Fix - Initializing...");


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

#define CREATE_PLAYTIME_HOOK(name, pattern, label)                                   \
    if (uint8_t* addr = Memory::PatternScan(baseModule, pattern, label))           \
    {                                                                              \
        name##_hook = safetyhook::create_inline(reinterpret_cast<void*>(addr),    \
            name##_Hook);                                                          \
        LOG_HOOK(name##_hook, label)                                               \
    }

    // Re-enabled: these parent trail/beam actors cap the whole railgun shot tree via the
    // GV_DestroyActor cascade - with them unhooked, vortex ribbons died at half span no
    // matter what their own life held. Skips now return 0 (constructor-safe) and key on
    // GM_StagePlayTime, not Present parity. Follow-up: convert to in-Act life holds.
    if (MGS2RailgunBeam::bEnabled)
    {
        MGS2_RAILGUN_PLAYTIME_SKIPS_ALWAYS(CREATE_PLAYTIME_HOOK)
    }

#undef CREATE_PLAYTIME_HOOK

#undef INSTALL_MGS2_FRAMESKIP_HOOK

    if (MGS2RailgunBeam::bEnabled) HookRailgunVortexRate(baseModule);


    if (uint8_t* MGS2_NewFortSplineBulletDemo_Scan = Memory::PatternScan(baseModule, "F6 C1 0F 75 ?? ?? ?? 8B 46 ?? 2B C5 66 0F 6E C0 0F 5B C0 0F 2F 05 ?? ?? ?? ?? 76 ?? 41 0F 28 CD EB ?? F3 0F 59 05 ?? ?? ?? ?? F3 0F 10 0D ?? ?? ?? ?? F3 0F 5C C8 F3 0F 59 3D ?? ?? ?? ?? 4C 8D 46 ?? F3 0F 5C F9 44 0F 2F EF 76 ?? 41 0F 28 FF 41 0F 2F 78 ?? 76 ?? 49 83 C0 20 41 0F 2F 78 ?? 77 ?? 49 8D 50 ?? 45 33 C9 48 8D 4C 24 ?? E8 ?? ?? ?? ?? 0F 28 CF 48 8D 4C 24 ?? 4C 8B C7 E8 ?? ?? ?? ?? 8B 46 ?? 2B C5 66 0F 6E C0 0F 5B C0 F3 0F 5E 05 ?? ?? ?? ?? F3 0F 58 47 ?? F3 0F 11 47 ?? ?? ?? ?? 49 83 C6 20 44 0B F8 48 83 C3 20 48 83 C7 10 FF C5 83 FD 28 0F 8C ?? ?? ?? ?? 44 0F 28 7C 24 ?? 44 0F 28 B4 24 ?? ?? ?? ?? 44 0F 28 AC 24 ?? ?? ?? ?? 44 0F 28 A4 24 ?? ?? ?? ?? 44 0F 28 9C 24 ?? ?? ?? ?? 44 0F 28 94 24 ?? ?? ?? ?? 44 0F 28 8C 24 ?? ?? ?? ?? 44 0F 28 84 24 ?? ?? ?? ?? 0F 28 BC 24 ?? ?? ?? ?? 0F 28 B4 24 ?? ?? ?? ?? 45 85 FF 75 ?? 48 8B CE E8 ?? ?? ?? ?? 83 46 ?? 06 EB ?? FF C0 89 46 ?? 48 8B 4C 24 ?? 48 33 CC E8 ?? ?? ?? ?? 48 8B 9C 24 ?? ?? ?? ?? 48 81 C4 10 01 00 00 41 5F 41 5E 5F 5E 5D C3 CC CC CC CC CC CC CC CC 48 83 EC 38 F3 0F 10 69 ?? ?? ?? ?? 0F 29 74 24 ?? 0F 28 F1 F3 0F 5C 71 ?? 0F 28 DE F3 0F 59 DE 0F 28 E3 0F 28 C3 F3 0F 59 41 ?? F3 0F 59 E6 0F 28 CC 0F 28 D4 F3 0F 59 49 ?? F3 0F 58 C8 0F 28 C6 F3 0F 59 41 ?? F3 0F 5E CD F3 0F 58 C8 0F 28 C3 F3 0F 5E CD ?? ?? ?? ?? ?? ?? ?? ?? ?? F3 0F 59 51 ?? ?? ?? ?? F3 0F 59 41 ?? F3 0F 58 D0 0F 28 C6 F3 0F 59 41 ?? F3 0F 5E D5 F3 0F 58 D0 0F 28 C6 F3 0F 5E D5 F3 0F 58 50 ?? F3 41 0F 11 50 ?? F3 0F 59 61 ?? ?? ?? ?? F3 0F 59 59 ?? F3 0F 59 41 ?? F3 0F 58 E3 F3 0F 5E E5 F3 0F 58 E0 F3 0F 5E E5 F3 0F 58 60 ?? 41 C7 40 ?? 00 00 80 3F F3 41 0F 11 60 ?? 0F 2F 71 ?? 72 ?? 48 8B 51 ?? 45 33 C9 4C 8D 42 ?? E8 ?? ?? ?? ?? 0F 28 74 24 ?? 48 83 C4 38 C3 CC CC CC CC CC CC CC CC CC CC 48 83 EC 48 F3 0F 10 2D ?? ?? ?? ?? F3 41 0F 10 48 ?? 0F 28 D5 F3 0F 10 25 ?? ?? ?? ?? 0F 28 C1 0F 29 74 24 ?? F3 0F 58 C5 F3 41 0F 10 70 ?? 0F 28 DD 0F 29 7C 24 ?? 0F 28 FD 44 0F 29 44 24 ?? F3 0F 5C FE ?? ?? ?? ?? ?? F3 0F 58 F5 44 0F 28 CD F3 41 0F 5C 50 ?? F3 0F 5C 5A ?? F3 0F 59 FA F3 0F 59 F2 F3 0F 10 52 ?? F3 0F 59 F8 0F 28 C5 F3 0F 5C C1 F3 0F 10 4A ?? F3 44 0F 5C C9 44 0F 28 C1 F3 0F 59 FC F3 44 0F 58 C5 F3 0F 59 F0 0F 28 C2 F3 44 0F 59 CB F3 0F 58 C5 F3 44 0F 59 C3 F3 0F 5C EA F3 0F 59 F4 F3 44 0F 59 C0 F3 44 0F 59 CD F3 44 0F 59 C4 F3 44 0F 59 CC 45 85 C9 74 ?? 66 41 0F 6E E9 0F 5B ED F3 0F 11 69", "MGS2: MGS2_NewFortSplineBulletDemo_Scan"))
    {
        Memory::PatchBytes((uintptr_t)MGS2_NewFortSplineBulletDemo_Scan, "\xF6\xC1\x0E", 3);
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

    if (uint8_t* MGS2_d_splash_parts__c_ActScanResult = (MGS2_GameFuncs::UpdateVectors_4 && MGS2_GameFuncs::GM_GetDGGroupID && MGS2_GameFuncs::GV_DestroyActor)
        ? Memory::PatternScan(baseModule, "40 57 48 83 EC 20 83 B9 ?? ?? ?? ?? 00 48 8B F9 7E ?? 48 89 5C 24 ?? 48 8B 59 ?? 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B CF 89 83 ?? ?? ?? ?? E8 ?? ?? ?? ?? FF 8F ?? ?? ?? ?? 48 8B 5C 24 ?? 48 83 C4 20 5F C3 48 83 C4 20 5F E9 ?? ?? ?? ?? CC 48 89 5C 24 ?? 57 48 83 EC 40 48 8B FA 0F 29 74 24 ?? 48 8B CF 0F 29 7C 24 ?? 49 8B D0 49 8B D8 E8 ?? ?? ?? ?? 48 C7 87 ?? ?? ?? ?? 44 00 00 00 0F 57 F6 F3 0F 10 3D ?? ?? ?? ?? F3 0F 10 2D ?? ?? ?? ?? F3 0F 10 25 ?? ?? ?? ?? F3 0F 10 1D ?? ?? ?? ?? F3 0F 10 15 ?? ?? ?? ?? F3 0F 11 2D ?? ?? ?? ?? F3 0F 11 25 ?? ?? ?? ?? F3 0F 11 1D ?? ?? ?? ?? F3 0F 11 15 ?? ?? ?? ?? F3 0F 11 2D ?? ?? ?? ?? F3 0F 11 25 ?? ?? ?? ?? F3 0F 11 1D ?? ?? ?? ?? F3 0F 11 15 ?? ?? ?? ?? F3 0F 11 2D ?? ?? ?? ?? F3 0F 11 25 ?? ?? ?? ?? F3 0F 11 1D ?? ?? ?? ?? F3 0F 11 15 ?? ?? ?? ?? F3 0F 11 2D ?? ?? ?? ?? F3 0F 11 25 ?? ?? ?? ?? F3 0F 11 1D ?? ?? ?? ?? F3 0F 11 15 ?? ?? ?? ?? F3 0F 10 43 ?? F3 0F 59 C6 ?? ?? ?? ?? F3 0F 59 C7 F3 0F 2C C0 66 89 05 ?? ?? ?? ?? F3 0F 10 43 ?? F3 0F 59 C6 F3 0F 58 43 ?? C7 05 ?? ?? ?? ?? 00 10 FF 8F C6 05 ?? ?? ?? ?? FF C6 05 ?? ?? ?? ?? FF C6 05 ?? ?? ?? ?? FF F3 0F 59 C7 C6 05 ?? ?? ?? ?? 5A", "MGS 2: Effect Speed Fix : user\\okajima\\demo_effect\\d_splash_parts_slow.c") : nullptr)
    {
        // Parse the wildcarded displacements from the matched bytes so they can't diverge.
        uint8_t* act = MGS2_d_splash_parts__c_ActScanResult;
        g_splashLifeDisp = *reinterpret_cast<int32_t*>(act + 8);
        g_splashPrimDisp = g_splashGroupDisp = -1;
        for (int i = 0x10; i < 0x40; ++i)
        {
            if (g_splashPrimDisp < 0 && act[i] == 0x48 && act[i+1] == 0x8B && act[i+2] == 0x59)
                g_splashPrimDisp = act[i+3];
            if (g_splashGroupDisp < 0 && act[i] == 0x89 && act[i+1] == 0x83)
                g_splashGroupDisp = *reinterpret_cast<int32_t*>(act + i + 2);
        }
        if (g_splashLifeDisp >= 0 && g_splashPrimDisp >= 0 && g_splashGroupDisp >= 0)
        {
            d_splash_parts__c_Act_hook = safetyhook::create_inline(reinterpret_cast<void*>(act), reinterpret_cast<void*>(MGS2_d_splash_parts__c_Act_hook));
            LOG_HOOK(d_splash_parts__c_Act_hook, "MGS 2: Effect Speed Fix: user\\okajima\\demo_effect\\d_splash_parts_slow.c")
            spdlog::info("MGS 2: Effect Speed Fix: splash offsets life=+0x{:X} prim=+0x{:X} group=+0x{:X}", g_splashLifeDisp, g_splashPrimDisp, g_splashGroupDisp);
        }
        else
        {
            spdlog::error("MGS 2: Effect Speed Fix: splash displacement parse failed; hook skipped.");
        }
    }

    // Kamome bird pacing (see the kamome hook block above).
    uint8_t* pKMM_ActSystem = Memory::PatternScan(baseModule, "40 57 48 83 EC ?? 83 B9 ?? ?? ?? ?? 00 48 8B F9 0F 85", "MGS 2: Effect Speed Fix : user\\okuta\\kamome\\kmtest.c -> KMM_ActSystem()");
    uint8_t* pKMM_ActControl = Memory::PatternScan(baseModule, "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 ?? 48 89 AC 24", "MGS 2: Effect Speed Fix : user\\okuta\\kamome\\kmtest.c -> KMM_ActControl()");
    uint8_t* pMEMMOT_MakeMotion = Memory::PatternScan(baseModule, "4C 8B DC 57 41 57 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 ?? ?? ?? ?? 48 8B F9", "MGS 2: Effect Speed Fix : user\\okuta\\conv\\memmot.c -> MEMMOT_MakeMotion()");
    uint8_t* pMEMMOT_MakeMotionSkip = Memory::PatternScan(baseModule, "48 8B 41 ?? F3 0F 10 51 ?? ?? ?? ?? 0F 28 C2", "MGS 2: Effect Speed Fix : user\\okuta\\conv\\memmot.c -> MEMMOT_MakeMotionSkip()");
    uint8_t* pKMM_Routine = Memory::PatternScan(baseModule, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B F9 48 8D 91", "MGS 2: Effect Speed Fix : user\\okuta\\kamome\\kmtest.c -> KMM_Routine()");

    if (!pKMM_ActControl || !pKMM_ActSystem || !pMEMMOT_MakeMotion || !pMEMMOT_MakeMotionSkip || !pKMM_Routine)
    {
        spdlog::error("MGS 2: Effect Speed Fix : Failed to find Kamome throttle hook addresses. Skipping Kamome throttle hooks.");
    }
    else
    {
        h_KMM_ActSystem = safetyhook::create_inline(reinterpret_cast<void*>(pKMM_ActSystem), KMM_ActSystem_hook);
        LOG_HOOK(h_KMM_ActSystem, "MGS 2: Effect Speed Fix : user\\okuta\\kamome\\kmtest.c -> KMM_ActSystem()")
        h_MEMMOT_MakeMotion = safetyhook::create_inline(reinterpret_cast<void*>(pMEMMOT_MakeMotion), MEMMOT_MakeMotion_hook);
        LOG_HOOK(h_MEMMOT_MakeMotion, "MGS 2: Effect Speed Fix : user\\okuta\\conv\\memmot.c -> MEMMOT_MakeMotion()")
        h_MEMMOT_MakeMotionSkip = safetyhook::create_inline(reinterpret_cast<void*>(pMEMMOT_MakeMotionSkip), MEMMOT_MakeMotionSkip_hook);
        LOG_HOOK(h_MEMMOT_MakeMotionSkip, "MGS 2: Effect Speed Fix : user\\okuta\\conv\\memmot.c -> MEMMOT_MakeMotionSkip()")
        // Mid-hook before the inline so the gate exists by the wrapper's first held call.
        g_pKMM_Routine = pKMM_Routine;
        h_KMM_Routine_ThinkGate = safetyhook::create_mid(pKMM_Routine + 0xBF, KMM_Routine_ThinkGate_hook);
        LOG_HOOK(h_KMM_Routine_ThinkGate, "MGS 2: Effect Speed Fix : user\\okuta\\kamome\\kmtest.c -> KMM_Routine() think gate")
        h_KMM_Routine = safetyhook::create_inline(reinterpret_cast<void*>(pKMM_Routine), KMM_Routine_hook);
        LOG_HOOK(h_KMM_Routine, "MGS 2: Effect Speed Fix : user\\okuta\\kamome\\kmtest.c -> KMM_Routine()")

        // Byte-verify the mid-hook sites. Note the "KMM_ActControl" pattern actually lands on
        // the position integrator.
        if (memcmp(pKMM_ActControl + 0x5C, "\x8B\xAB\x90\x02\x00\x00", 6) == 0 &&
            memcmp(pKMM_ActControl + 0xB5, "\x8B\x8B\x60\x01\x00\x00", 6) == 0)
        {
            g_pKamomeIntegrator = pKMM_ActControl;
            h_KamomeTurnPace = safetyhook::create_mid(pKMM_ActControl + 0x5C, KamomeTurnPace_hook);
            LOG_HOOK(h_KamomeTurnPace, "MGS 2: Effect Speed Fix : user\\okuta\\kamome\\kmtest.c -> KMM_ActControl() turn pace")
        }
        else
        {
            spdlog::error("MGS 2: Effect Speed Fix : Kamome turn-pace site mismatch; skipping.");
        }
        if (memcmp(pKMM_ActControl + 0x21D, "\xF3\x44\x0F\x58\x83\x10\x03\x00\x00", 9) == 0 &&
            memcmp(pKMM_ActControl + 0x27E, "\xF3\x0F\x58\xB3\x18\x03\x00\x00", 8) == 0 &&
            memcmp(pKMM_ActControl + 0x28E, "\xF3\x0F\x58\x93\x1C\x03\x00\x00", 8) == 0)
        {
            h_KamomeDeltaScaleX = safetyhook::create_mid(pKMM_ActControl + 0x21D, KamomeDeltaScaleX_hook);
            LOG_HOOK(h_KamomeDeltaScaleX, "MGS 2: Effect Speed Fix : user\\okuta\\kamome\\kmtest.c -> KMM_ActControl() delta X")
            h_KamomeDeltaScaleZ = safetyhook::create_mid(pKMM_ActControl + 0x27E, KamomeDeltaScaleZ_hook);
            LOG_HOOK(h_KamomeDeltaScaleZ, "MGS 2: Effect Speed Fix : user\\okuta\\kamome\\kmtest.c -> KMM_ActControl() delta Z")
            h_KamomeDeltaScaleW = safetyhook::create_mid(pKMM_ActControl + 0x28E, KamomeDeltaScaleW_hook);
            LOG_HOOK(h_KamomeDeltaScaleW, "MGS 2: Effect Speed Fix : user\\okuta\\kamome\\kmtest.c -> KMM_ActControl() delta W")
        }
        else
        {
            spdlog::error("MGS 2: Effect Speed Fix : Kamome delta-scale sites mismatch; skipping.");
        }

    }

    /*
    uint8_t* pAct_16 = Memory::PatternScan(baseModule,"4C 8B DC 57 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 84 24 ?? ?? ?? ?? 49 89 5B ?? 48 8B F9","MGS 2: Effect Speed Fix : user\\kano\\hair\\hairevm.c -> Act()");
    uint8_t* pGateFrom = Memory::PatternScan(baseModule, "41 8B D4 48 8D 4F", "MGS 2: Effect Speed Fix : user\\kano\\hair\\hairevm.c -> Act()+0x842");
    uint8_t* pGateTo = Memory::PatternScan(baseModule, "48 8B 8C 24 ?? ?? ?? ?? 48 33 CC E8 ?? ?? ?? ?? 48 81 C4 ?? ?? ?? ?? 5F C3 90", "MGS 2: Effect Speed Fix : user\\kano\\hair\\hairevm.c -> Act()+0x8A6");

    if (!pAct_16 || !pGateFrom || !pGateTo)
    {
        spdlog::error("MGS 2: Effect Speed Fix : Failed to find Hair physics hook addresses. Skipping Hair physics fix.");
    }
    else
    {
        g_pGateTo = pGateTo;
        h_Act16_HairPhysicsGate = safetyhook::create_mid(pGateFrom, Act16_HairPhysicsGate_hook);
    }
    */


    // d_plasma_poly (camo-break plasma lines): the gate jumps the Act body to its own
    // epilogue, so no return-value hazard.
    uint8_t* pAct_389 = Memory::PatternScan(baseModule, "48 8B C4 48 89 58 ?? 48 89 70 ?? 48 89 78 ?? 55 41 54 41 55 41 56 41 57 48 8D 68 ?? 48 81 EC ?? ?? ?? ?? 0F 29 70 ?? 0F 29 78 ?? 44 0F 29 40 ?? 44 0F 29 48 ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 ?? 48 8B 99", "MGS 2: Effect Speed Fix : user\\okajima\\demo_effect\\d_plasma_poly.c -> Act()");
    uint8_t* pGateFrom_plasma_scan = Memory::PatternScan(baseModule, "4D 8D A6 ?? ?? ?? ?? 49 8B 8E", "MGS 2: Effect Speed Fix : user\\okajima\\demo_effect\\d_plasma_poly.c -> Act()+0x8E");
    uint8_t* pGateTo_plasma_scan = Memory::PatternScan(baseModule, "48 8B 4D ?? 48 33 CC E8 ?? ?? ?? ?? 4C 8D 9C 24 ?? ?? ?? ?? 49 8B 5B ?? 49 8B 73 ?? 49 8B 7B ?? 41 0F 28 73 ?? 41 0F 28 7B ?? 45 0F 28 43 ?? 45 0F 28 4B ?? 49 8B E3 41 5F 41 5E 41 5D 41 5C 5D C3 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 4C 8B DC", "MGS 2: Effect Speed Fix : user\\okajima\\demo_effect\\d_plasma_poly.c -> Act()+0x1861");

    if (!pAct_389 || !pGateFrom_plasma_scan || !pGateTo_plasma_scan)
    {
        spdlog::error("MGS 2: Effect Speed Fix : Failed to find Plasma Poly throttle hook addresses. Skipping Plasma Poly throttle hooks.");
    }
    else
    {
        g_pPlasmaGateTo = pGateTo_plasma_scan;
        h_Act389_PlasmaGate = safetyhook::create_mid(pGateFrom_plasma_scan, Act389_PlasmaGate_hook);
    }

    // Installed last: rewrites the countdown Act's prologue, and nothing may pattern-scan after.
    if (uint8_t* pWindowCountdownAct = Memory::PatternScan(baseModule,
        "8B 41 ?? 85 C0 74 ?? FF C8 89 41 ?? C3",
        "MGS 2: Effect Speed Fix : user\\morita\\demo_frmcnt\\demo_frmcnt.c -> NewDemoFrameCountCall() -> Act()"))
    {
        h_DemoFrameCountAct = safetyhook::create_inline(reinterpret_cast<void*>(pWindowCountdownAct), DemoFrameCountAct_hook);
        LOG_HOOK(h_DemoFrameCountAct, "MGS 2: Effect Speed Fix : user\\morita\\demo_frmcnt\\demo_frmcnt.c -> NewDemoFrameCountCall() -> Act()")
    }
    else
    {
        spdlog::error("MGS 2: Effect Speed Fix : window countdown Act scan failed; windows will expire at half PS2 duration.");
    }

    MAKE_HOOK_MID(baseModule, "48 83 C6 ?? 49 83 C6 ?? 48 83 C5", "MGS 2: Effect Speed Fix : user\\morita\\splash\\splash.c -> SPH_ActBrkVol1() alpha freeze fix", {
            const int16_t pad = *reinterpret_cast<const int16_t*>(ctx.rbx + 6);
            if (pad <= 0)
            {
                *reinterpret_cast<uint8_t*>(ctx.rsi - 0xC) = 0; // don't let alpha freeze at its last positive-pad value
            }
                  });

    MGS2_FULL_SKIP_ACT_DIE_PAIRS(CREATE_FULL_SKIP_HOOK)


}

