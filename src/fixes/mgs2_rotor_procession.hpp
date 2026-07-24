#pragma once

// MGS2: helicopter rotors advance exactly one blade-spacing per demo key (72deg, 5 blades),
// so the disc strobes in place like a frozen wagon wheel. Spot the spinners at the packet
// emitter and drift them slowly backwards so the blades process like they would on film.
namespace MGS2RotorProcession
{
    inline bool bEnabled = true;
    void Initialize();

    void HandleLevelTransition();
}
