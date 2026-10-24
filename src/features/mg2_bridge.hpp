#pragma once

namespace MG2_Bridge
{
    inline bool bEnabled = true;
    inline bool bReducePhotosensitiveEffects = false;
    void Initialize();
    void CaptureDyingMessage(const char* pText, int nLength);
    int GetDyingEyeState();
    void HandleLevelTransition();
}
