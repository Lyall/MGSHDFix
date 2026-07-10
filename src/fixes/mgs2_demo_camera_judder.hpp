#pragma once

// MGS2: scripted camera vibration (irnd()%level whole-unit offsets) was authored for 480i where
// it read as a subtle rumble; at modern resolutions it judders the camera during demo cutscenes
// (e.g. the d001p01 doorjamb dolly). Suppress the small ambient offsets during scripted
// sequences; large one-shot knocks (explosions) pass through.
namespace MGS2DemoCameraJudder
{
    inline bool bEnabled = true;
    void Initialize();
}
