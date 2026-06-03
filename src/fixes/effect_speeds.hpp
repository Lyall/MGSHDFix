#pragma once
#include <chrono>

class EffectSpeedFix final
{
public:
    static void Initialize();
    static void Reset();
    static void Tick();
    bool isEnabled = true;

    std::chrono::time_point<std::chrono::high_resolution_clock> solidusDashAct_NextUpdate;

};

inline EffectSpeedFix g_EffectSpeedFix;


