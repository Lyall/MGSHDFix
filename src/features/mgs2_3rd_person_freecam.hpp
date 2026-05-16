#pragma once

namespace MGS2_ThirdPersonFreecam
{
    void Activate();

    void HandleLevelTransition();

    void Tick();

    inline bool bEnabled = false;
    inline int vkToggle_Camera = 0;
    inline int vkToggle_Inherit_Camera_Rotation = 0;

    inline bool bInherit_Camera_Rotation = false;

    inline int iMax_Camera_Distance = 0;
    inline float fHorizontal_Sensitivity = 0.0f;
    inline float fVertical_Sensitivity = 0.0f;

    inline int iCameraDistanceStep = 250;
    inline int vkToggle_Increase_Camera_Distance = 0;
    inline int vkToggle_Decrease_Camera_Distance = 0;
    inline int vkToggle_Reset_Camera_Distance = 0;
    inline int iCameraDistanceChangeSpeed = 25;



}
