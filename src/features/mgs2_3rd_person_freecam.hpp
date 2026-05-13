#pragma once

namespace MGS2_ThirdPersonFreecam
{
    void Activate();

    void HandleLevelTransition();

    inline bool b3rd_Person_Camera_Enabled = false;
    inline int vkToggle_3rd_Person_Camera = 0;
    inline int vkToggle_3rd_Person_Camera_Inherit_Camera_Rotation = 0;

    inline bool b3rd_Person_Camera_Inherit_Camera_Rotation = false;

    inline int i3rd_Person_Max_Camera_Distance = 0;
    inline float f3rd_Person_Horizontal_Sensitivity = 0.0f;
    inline float f3rd_Person_Vertical_Sensitivity = 0.0f;


}
