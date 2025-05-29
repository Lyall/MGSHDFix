#pragma once

class EffectSpeedFix
{
public:
    bool isEnabled;
    void Initialize() const;
    std::chrono::time_point<std::chrono::high_resolution_clock> solidusDashAct_NextUpdate;
};

inline EffectSpeedFix g_EffectSpeedFix;


