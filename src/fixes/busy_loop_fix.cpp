#include "stdafx.h"
#include "common.hpp"
#include "busy_loop_fix.hpp"
#include "logging.hpp"
#include "helper.hpp"

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

#include <windows.h>
#include <timeapi.h>

#pragma comment(lib, "winmm.lib")

__int64 BusyLoopFix::ActorWait_Hook()
{
    
    
    __int64 result = g_instance->m_actorWaitHook.call<__int64>();

    if (result == 0)
    {
        double elapsed = GetElapsedTime();
        double target = 0.01666;
        double remaining = target - elapsed;

        // sleep isn't perfectly accruate, so we only sleep if more than 3ms remain
        if (remaining * 1000.0 > 3.0) {
            //spdlog::info("sleep");
            timeBeginPeriod(1);
            Sleep(1);
            timeEndPeriod(1);
        }
    }

    return result;
}

void BusyLoopFix::Initialize()
{
    if (!bEnabled)
    {
        return;
    }

    g_instance = this;

    auto target = GetProcAddress(GetModuleHandleW(L"user32.dll"),"PeekMessageW");

    if (!target)
    {
        spdlog::info("MGS 2 | MGS 3: BusyLoopFix - Failed to find PeekMessageW!");
        return;
    }

    m_peekMessageHook = safetyhook::create_inline(target, PeekMessageW_Hook);

    if (!Util::IsRunningUnderWine())
    {
        //return;
    }

    GetElapsedTime = (GetElapsedTime_t*)Memory::PatternScan(baseModule, "48 83 EC ?? 48 8D 0D ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8B 05", "MGS 2 | MGS 3: GetElapsedTime");
    auto targetActorWait = Memory::PatternScan(
        baseModule,
        !(eGameType & MGS2)
        ? "48 83 EC ?? 48 8D 0D ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8B 05"
        : "48 83 EC ?? E8 ?? ?? ?? ?? 83 3D ?? ?? ?? ?? ?? 74",
        "MGS 2 | MGS 3: ActorWait"
    );

    if (!targetActorWait)
    {
        spdlog::info("MGS 3: BusyLoopFix - Failed to find ActorWait!");
        return;
    }

    //m_actorWaitValue = (double*)Memory::PatternScan(baseModule, "83 3D ?? ?? ?? ?? 00 ?? ?? F2 0F 10 0D", "MGS 2 | MGS 3: ActorWaitValue");
    m_actorWaitHook = safetyhook::create_inline(reinterpret_cast<void*>(targetActorWait), ActorWait_Hook);
}

void BusyLoopFix::Shutdown()
{
    m_peekMessageHook = {};
    m_actorWaitHook = {};
    g_instance = nullptr;
}
