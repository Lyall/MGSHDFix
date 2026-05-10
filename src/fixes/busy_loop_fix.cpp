#include "stdafx.h"
#include "common.hpp"
#include "busy_loop_fix.hpp"
#include "logging.hpp"
#include "helper.hpp"
#include <timeapi.h>
#include <gamevars.hpp>

#pragma comment(lib, "winmm.lib")

static BusyLoopFix* g_instance = nullptr;
typedef double GetElapsedTime_t();
GetElapsedTime_t* GetElapsedTime;

BOOL WINAPI BusyLoopFix::PeekMessageW_Hook(LPMSG m, HWND h, UINT a, UINT b, UINT c)
{
    BOOL r = g_instance->m_peekMessageHook.call<BOOL>(m, h, a, b, c);

    if (!r)
    {
        MsgWaitForMultipleObjects(0, NULL, FALSE, 1, QS_ALLINPUT);
    }

    return r;
}

__int64 BusyLoopFix::ActorWait_Hook()
{
    __int64 result = g_instance->m_actorWaitHook.call<__int64>();

    if (result == 0)
    {
        double remaining = g_GameVars.ActorWaitValue() - GetElapsedTime();

        // sleep may not be fully accurate, even with timeBeginPeriod.
        // a 3ms buffer should be more than enough to avoid overshooting our 
        // wait and to reduce cpu usage
        if (remaining * 1000.0 > 3.0) 
            Sleep(1);
    }

    return result;
}

void BusyLoopFix::Initialize()
{
    if (!iOption)
        return;

    g_instance = this;

    auto target = GetProcAddress(GetModuleHandleW(L"user32.dll"),"PeekMessageW");

    if (!target)
    {
        spdlog::error("MGS 2 | MGS 3: BusyLoopFix - Failed to find PeekMessageW!");
        return;
    }

    m_peekMessageHook = safetyhook::create_inline(target, PeekMessageW_Hook);

    if (iOption == 1)
    {
        return;
    }

    timeBeginPeriod(1);

    // MGSFPSUnlock hooks this function, so we pattern scan after the hook
    GetElapsedTime = (GetElapsedTime_t*)(Memory::PatternScan(baseModule, "FF 15 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 0F 57 C0", "MGS 2 | MGS 3: GetElapsedTime") - 0xB);

    if (!GetElapsedTime)
    {
        spdlog::error("MGS 2 | MGS 3: BusyLoopFix - Failed to find GetElapsedTime!");
        return;
    }
    
    auto targetActorWait = Memory::PatternScan(baseModule, "48 83 EC ?? E8 ?? ?? ?? ?? 83 3D ?? ?? ?? ?? ?? 74", "MGS 2 | MGS 3: ActorWait");

    if (!targetActorWait)
    {
        spdlog::error("MGS 2 | MGS 3: BusyLoopFix - Failed to find ActorWait!");
        return;
    }

    m_actorWaitHook = safetyhook::create_inline(reinterpret_cast<void*>(targetActorWait), ActorWait_Hook);
}

void BusyLoopFix::Shutdown()
{
    m_peekMessageHook = {};
    m_actorWaitHook = {};
    g_instance = nullptr;

    if (iOption > 1)
        timeEndPeriod(1);
}
