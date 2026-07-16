#pragma once

namespace ResolutionScalingFixes
{
    void ApplyFixes();
    void HandleLevelTransition();

    inline bool bIncreaseShadowResolution = true;

    inline bool bFixM92FPV = false;

    inline bool bRestorePS2NVGLineHeight = true;
}
