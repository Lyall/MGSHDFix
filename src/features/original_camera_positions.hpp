#pragma once

namespace OriginalCameraPositions
{
    void Activate();
    bool NeedsCameraCorrection();

    inline bool bEnabled = false;
    inline int vkToggle_HDC_CameraPositions = 0;
}
