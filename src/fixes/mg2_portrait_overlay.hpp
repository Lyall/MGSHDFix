#pragma once

namespace MG2_PortraitOverlay
{
    void Initialize();
    [[nodiscard]] bool IsContactRevealPending();

    inline bool bEnabled = false;
}
