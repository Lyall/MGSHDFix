#pragma once

namespace MGS3_StatusFlags
{

    enum MGS3_GameStateFlags : uint32_t
    {
        STATE_VIB_PAUSE0 = 0x1000,  /// Level 0 vibration disabled
        

    };


    enum MGS3_MenuStatusFlags : uint32_t
    {
        MENU_VIBRATE_DISABLE = 0x80000, /// Vibration disabled

    };


    enum MGS3PauseLevelFlags : int
    {
        MGS3_GV_PAUSE_NOSTOP = 0x00,
        MGS3_GV_PAUSE_PAUSE = 0x01,
    };
}
