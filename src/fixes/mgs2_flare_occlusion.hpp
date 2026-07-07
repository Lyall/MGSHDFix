#pragma once

// The PS2 dims the lens flares by a visibility ratio from four line-of-sight checks at the corners of
// a quad around the sun: ratio = (4 + sum(+-1)) / 8. The cutscene flare lacks those checks, so the sun
// never pulses when the fence crosses it. Recreated here from the scene depth, with the game's own
// collision check vetoing occluders that aren't world geometry (characters aren't in it, so Raiden
// doesn't block - same as the original checks).
namespace MGS2FlareOcclusion
{
    void  Initialize();
    void  SetSunState(const float* sunWorldPos, float scrX, float scrY, float clipZ, float clipW);
    float GetVisibility();   // 0..1, 1 = unoccluded
}
