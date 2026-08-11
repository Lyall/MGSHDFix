#include "stdafx.h"
#include "mgs2_demo_scope.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "helper.hpp"
#include "logging.hpp"
#include "mgs2_linkvarbuf.hpp"
#include "original_camera_positions.hpp"


namespace
{
    constexpr ptrdiff_t kActorDieOffset = 0x20; // GV_ACT::die

    SafetyHookInline g_newPsg1SightHook{};
    SafetyHookInline g_psg1SightDieHook{};
    std::atomic<uintptr_t> g_scopeWork = 0;

    void __fastcall Psg1SightDie_Detour(uintptr_t work)
    {
        if (g_scopeWork == work)
        {
            g_scopeWork = 0;
        }

        g_psg1SightDieHook.fastcall<void>(work);
    }

    uintptr_t __fastcall NewPsg1Sight_Detour(int name)
    {
        const uintptr_t work = g_newPsg1SightHook.fastcall<uintptr_t>(name);
        if (!work)
        {
            return 0;
        }

        if (!g_psg1SightDieHook)
        {
            const uintptr_t die = Memory::ReadField<uintptr_t>(work, kActorDieOffset);
            if (die)
            {
                g_psg1SightDieHook = safetyhook::create_inline(reinterpret_cast<void*>(die), reinterpret_cast<void*>(Psg1SightDie_Detour));
                LOG_HOOK(g_psg1SightDieHook, "MGS 2: Demo Scope: user\\skoba\\weapon\\psg_layout.c -> Die()");
            }
        }

        if (g_psg1SightDieHook)
        {
            g_scopeWork = work;
        }

        return work;
    }

    void InstallCameraHooks()
    {
        MAKE_HOOK_MID(baseModule, "F3 0F 59 0D ?? ?? ?? ?? F3 0F 58 0D ?? ?? ?? ?? F3 0F 11 49 14", "MGS 2: Demo Scope: Camera Ratio X", {
            if (MGS2DemoScope::NeedsCameraCorrection())
            {
                ctx.xmm1.f32[0] = 0.0f;
            }
        });

        MAKE_HOOK_MID(baseModule, "F3 0F 59 15 ?? ?? ?? ?? F3 0F 58 15 ?? ?? ?? ?? F3 0F 59 0D", "MGS 2: Demo Scope: Camera Ratio Y", {
            if (MGS2DemoScope::NeedsCameraCorrection())
            {
                ctx.xmm2.f32[0] = 0.0f;
            }
        });
    }
}

bool MGS2DemoScope::NeedsCameraCorrection()
{
    if (!g_scopeWork || !g_GameVars.InCutscene())
    {
        return false;
    }

    return !(MGS2_LinkVarBuf::GM_Configuration & MGS2_LinkVarBuf::GM_CONFIG_CUTSCENES_LETTERBOXED);
}

void MGS2DemoScope::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (uint8_t* address = Memory::PatternScan(
            baseModule,
            "48 89 5C 24 ?? 57 48 83 EC ?? 8B F9 BA ?? ?? ?? ?? B9 ?? ?? ?? ?? 44 8D 49 ?? 45 8D 41",
            "MGS 2: Demo Scope: user\\skoba\\weapon\\psg_layout.c -> NewPsg1Sight()"))
    {
        g_newPsg1SightHook = safetyhook::create_inline(address, reinterpret_cast<void*>(NewPsg1Sight_Detour));
        LOG_HOOK(g_newPsg1SightHook, "MGS 2: Demo Scope: user\\skoba\\weapon\\psg_layout.c -> NewPsg1Sight()");
    }

    if (!OriginalCameraPositions::bEnabled)
    {
        InstallCameraHooks();
    }
}
