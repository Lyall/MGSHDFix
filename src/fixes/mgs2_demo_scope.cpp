#include "stdafx.h"
#include "mgs2_demo_scope.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "logging.hpp"
#include "mgs2_linkvarbuf.hpp"


namespace
{
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

        if (g_psg1SightDieHook)
        {
            g_scopeWork = work;
        }

        return work;
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
            "48 83 EC ?? 8B 49 ?? 85 C9 78 ?? E8 ?? ?? ?? ?? 33 D2",
            "MGS 2: Demo Scope: user\\skoba\\weapon\\psg_layout.c -> NewPsg1Sight() -> Die()"))
    {
        g_psg1SightDieHook = safetyhook::create_inline(address, reinterpret_cast<void*>(Psg1SightDie_Detour));
        LOG_HOOK(g_psg1SightDieHook, "MGS 2: Demo Scope: user\\skoba\\weapon\\psg_layout.c -> NewPsg1Sight() -> Die()");
    }

    if (uint8_t* address = Memory::PatternScan(
            baseModule,
            "48 89 5C 24 ?? 57 48 83 EC ?? 8B F9 BA ?? ?? ?? ?? B9 ?? ?? ?? ?? 44 8D 49 ?? 45 8D 41",
            "MGS 2: Demo Scope: user\\skoba\\weapon\\psg_layout.c -> NewPsg1Sight()"))
    {
        g_newPsg1SightHook = safetyhook::create_inline(address, reinterpret_cast<void*>(NewPsg1Sight_Detour));
        LOG_HOOK(g_newPsg1SightHook, "MGS 2: Demo Scope: user\\skoba\\weapon\\psg_layout.c -> NewPsg1Sight()");
    }
}
