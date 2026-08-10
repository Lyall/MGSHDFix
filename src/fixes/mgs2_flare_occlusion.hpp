#pragma once

// Restores the demo flare's occlusion pulse.
namespace MGS2FlareOcclusion
{
    void  Initialize();
    void  SetSunState(const float* sunWorldPos, float scrX, float scrY, float clipZ, float clipW);
    float GetVisibility();
}
