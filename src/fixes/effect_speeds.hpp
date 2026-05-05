#pragma once
#include <chrono>

class EffectSpeedFix final
{
public:
    static void Initialize();
    void Reset();
    bool isEnabled = true;

    double iExplosionDuration = 0;
    int iDebrisIteration = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> solidusDashAct_NextUpdate;

    bool g_rain_slow_skip = false;
};

inline EffectSpeedFix g_EffectSpeedFix;


