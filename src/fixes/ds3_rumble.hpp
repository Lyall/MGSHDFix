#pragma once

namespace Ds3Rumble
{
    inline bool bEnabled = false;
    inline int iStrength = 100;     // big-motor scale, percent

    void Initialize();
    void Shutdown();
}
