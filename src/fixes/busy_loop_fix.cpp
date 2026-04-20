#include "stdafx.h"
#include "common.hpp"
#include "busy_loop_fix.hpp"
#include "logging.hpp"

static BusyLoopFix* g_instance = nullptr;

BOOL WINAPI BusyLoopFix::PeekMessageW_Hook(LPMSG m, HWND h, UINT a, UINT b, UINT c)
{
    BOOL r = g_instance->m_hook.call<BOOL>(m, h, a, b, c);

    if (!r)
    {
        MsgWaitForMultipleObjects(0, NULL, FALSE, 1, QS_ALLINPUT);
    }

    return r;
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

    m_hook = safetyhook::create_inline(target, PeekMessageW_Hook);
}

void BusyLoopFix::Shutdown()
{
    m_hook = {};
    g_instance = nullptr;
}
