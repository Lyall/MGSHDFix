#pragma once

class DepthOfFieldFixes final
{
public:
    bool bEnabled = true;
    float fBlurUvMultiplier = 6.0f;

    void Initialize();
};

inline DepthOfFieldFixes g_DepthOfFieldFixes;
