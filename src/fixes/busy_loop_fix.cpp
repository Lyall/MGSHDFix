#include "stdafx.h"
#include "common.hpp"
#include "busy_loop_fix.hpp"
#include "logging.hpp"
#include "helper.hpp"
#include <timeapi.h>
#include <gamevars.hpp>
#include <algorithm>
#include <cmath>

#pragma comment(lib, "winmm.lib")

static BusyLoopFix* g_instance = nullptr;
typedef double GetElapsedTime_t();
GetElapsedTime_t* GetElapsedTime;

// Null when the OS won't give us a high resolution timer, in which case we keep
// the old Sleep(1) behaviour.
static HANDLE g_waitTimer = nullptr;
static bool g_waited = false;

// Left for the game's own loop to spin out, so a slightly late wake can't push us
// past the frame deadline.
static constexpr double kSpinMarginMs = 1.0;

// A bad clock read must not park the game.
static constexpr double kMaxWaitMs = 50.0;

static void PreciseWait(double ms)
{
    LARGE_INTEGER due {};
    due.QuadPart = -static_cast<LONGLONG>(ms * 10000.0);   // relative, 100ns units

    if (SetWaitableTimer(g_waitTimer, &due, 0, nullptr, nullptr, FALSE))
    {
        WaitForSingleObject(g_waitTimer, static_cast<DWORD>(ms) + 2);
    }
}

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

    if (result != 0)
    {
        g_waited = false;
        return result;
    }

    if (!g_waitTimer)
    {
        // No high resolution timer, so sleep in small steps and let the game spin
        // out the last few ms itself.
        if ((g_GameVars.ActorWaitValue() - GetElapsedTime()) * 1000.0 > 3.0)
            Sleep(1);

        return result;
    }

    // One sleep per wait. After that we just return, so the game spins the tail on
    // its own timing instead of us re-reading the clock every time round the loop.
    if (!g_waited)
    {
        g_waited = true;

        const double remainingMs = (g_GameVars.ActorWaitValue() - GetElapsedTime()) * 1000.0;
        if (std::isfinite(remainingMs) && remainingMs > kSpinMarginMs)
            PreciseWait(std::min(remainingMs - kSpinMarginMs, kMaxWaitMs));
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
        spdlog::error("MG | MGS 2 | MGS 3: BusyLoopFix - Failed to find PeekMessageW!");
        return;
    }

    m_peekMessageHook = safetyhook::create_inline(target, PeekMessageW_Hook);

    if (iOption == 1)
    {
        return;
    }

    timeBeginPeriod(1);

    // A standard waitable timer is no better than Sleep(1), so only take the high
    // resolution one.
    g_waitTimer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

    if (!g_waitTimer)
        spdlog::warn("MG | MGS 2 | MGS 3: BusyLoopFix - no high resolution timer, falling back to Sleep(1).");

    // MGSFPSUnlock hooks this function, so we pattern scan after the hook
    GetElapsedTime = (GetElapsedTime_t*)(Memory::PatternScan(baseModule, "FF 15 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 0F 57 C0", "MG | MGS 2 | MGS 3: GetElapsedTime") - 0xB);

    if (!GetElapsedTime)
    {
        spdlog::error("MG | MGS 2 | MGS 3: BusyLoopFix - Failed to find GetElapsedTime!");
        return;
    }
    
    auto targetActorWait = Memory::PatternScan(baseModule, "48 83 EC ?? E8 ?? ?? ?? ?? 83 3D ?? ?? ?? ?? ?? 74", "MG | MGS 2 | MGS 3: ActorWait");

    if (!targetActorWait)
    {
        spdlog::error("MG | MGS 2 | MGS 3: BusyLoopFix - Failed to find ActorWait!");
        return;
    }

    m_actorWaitHook = safetyhook::create_inline(reinterpret_cast<void*>(targetActorWait), ActorWait_Hook);
}

void BusyLoopFix::Shutdown()
{
    if (!iOption)
    {
        return;
    }
    m_peekMessageHook = {};
    m_actorWaitHook = {};
    g_instance = nullptr;

    if (g_waitTimer)
    {
        CloseHandle(g_waitTimer);
        g_waitTimer = nullptr;
    }

    if (iOption > 1)
        timeEndPeriod(1);
}
