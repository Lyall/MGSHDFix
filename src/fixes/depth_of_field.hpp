#pragma once

class DepthOfFieldFixes final
{
public:
    bool bEnabled = true;
    float fBlurUvMultiplier = 3.0f;

    void Initialize();
    void HandleLevelTransition();
};

inline DepthOfFieldFixes g_DepthOfFieldFixes;
