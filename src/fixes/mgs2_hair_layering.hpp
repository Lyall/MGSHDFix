#pragma once

// MGS2: Snake's mullet packs its texture layers outer-first, so top strands blend against
// the background before the layers beneath draw. Re-emit the groups back-to-front.
namespace MGS2HairLayering
{
    inline bool bEnabled = true;
    void Initialize();
}
