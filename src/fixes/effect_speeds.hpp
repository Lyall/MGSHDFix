#pragma once

class EffectSpeedFix
{
public:
    bool isEnabled;
    std::chrono::time_point<std::chrono::high_resolution_clock> solidusDashAct_NextUpdate;

    void Initialize() const;
};

inline EffectSpeedFix g_EffectSpeedFix;


