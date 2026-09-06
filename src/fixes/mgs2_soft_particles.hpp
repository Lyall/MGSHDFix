#pragma once

namespace MGS2SoftParticles
{
    inline bool bEnabled = false;   // costs real GPU time on handhelds, so it is opt-in
    void OnDeviceReady();
}
