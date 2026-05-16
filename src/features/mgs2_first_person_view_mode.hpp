#pragma once

namespace MGS2_First_Person_View
{
    void Activate();
    void HandleLevelTransition();


    inline bool bFirst_Person_View_Enabled = false;
    inline bool bFirst_Person_View_Movement_Enabled_By_Default = true;
    inline int vkToggle_First_Person_View = 0;
    inline int vkToggle_First_Person_View_Movement = 0;

}

