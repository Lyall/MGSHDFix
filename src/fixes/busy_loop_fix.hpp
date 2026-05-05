
#pragma once
#include <safetyhook.hpp>

class BusyLoopFix {
public:
    void Initialize();
    void Shutdown();
    static BOOL WINAPI PeekMessageW_Hook(LPMSG m, HWND h, UINT a, UINT b, UINT c);
    static __int64 ActorWait_Hook();

    int iOption = 0;

private:
    SafetyHookInline m_peekMessageHook;
    SafetyHookInline m_actorWaitHook;
};

inline BusyLoopFix g_BusyLoopFix;