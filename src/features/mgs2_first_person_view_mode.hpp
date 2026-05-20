#pragma once

namespace MGS2_First_Person_View
{
    void Activate();

    void Tick();


    inline bool bFirst_Person_View_Enabled = true;
    inline bool bFirst_Person_View_Movement_Enabled_By_Default = true;
    inline bool bFirst_Person_View_Toggle_Hold = true;

    inline int vkToggle_First_Person_Override = 0;
    inline int vkToggle_Hold_First_Person_View = 0;
    inline int vkToggle_First_Person_View_Movement = 0;

}

