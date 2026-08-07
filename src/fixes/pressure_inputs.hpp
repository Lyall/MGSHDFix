#pragma once

#include <string>

namespace PressureInputs
{
    inline bool bEnabled = false;

    // Drop the stand-ins the MC added for the pressure it could not read, so the controls the
    // games shipped with are the only ones. Needs a pad that reports pressure.
    inline bool bSuppressAlternates = false;

    void Initialize();

    // The open DsHidMini device, for rumble to open its own write handle: fills path and
    // returns the mode (0 SXS, 1 SDF, 2 native), or -1 without a pad.
    int Ds3DeviceMode(std::wstring& path);
    bool HavePad();
}
