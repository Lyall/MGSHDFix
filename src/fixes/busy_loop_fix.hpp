
#pragma once
#include <safetyhook.hpp>

class BusyLoopFix {
public:
    void Initialize();
    void Shutdown();
    static BOOL WINAPI PeekMessageW_Hook(LPMSG m, HWND h, UINT a, UINT b, UINT c);

    bool bEnabled = false;
private:
    SafetyHookInline m_hook;
};

inline BusyLoopFix g_BusyLoopFix;