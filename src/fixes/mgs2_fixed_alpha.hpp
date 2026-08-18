#pragma once

namespace MGS2FixedAlpha
{
    void Setup();
    void OnDeviceReady();

    // True if a destination-scaling blend was repaired since the last call.
    bool ConsumeDestScaleRepair();

    inline bool bEnabled = true;
    inline bool bLoaded = false;
};
