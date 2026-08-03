#pragma once

namespace PressureInputs
{
    inline bool bEnabled = false;

    // Drop the stand-ins the MC added for the pressure it could not read, so the controls the
    // games shipped with are the only ones. Needs a pad that reports pressure.
    inline bool bSuppressAlternates = false;

    void Initialize();
}
