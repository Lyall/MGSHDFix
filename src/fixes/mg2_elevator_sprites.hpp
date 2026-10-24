#pragma once

namespace MG2_ElevatorSprites
{
    struct UnlitLamp
    {
        int nArea;
        int nRoom;
        int nX;
        int nY;
    };

    inline constexpr UnlitLamp astUnlitLamps[] =
    {
        { 8, 0, 116, 66 },
        { 8, 7, 84, 66 },
        { 8, 15, 84, 66 },
    };

    inline constexpr uint8_t anUnlitLampPixels[] =
    {
        0x67, 0x61, 0x61, 0x61, 0x67, 0x61, 0x61, 0x61, 0x67, 0x61, 0x61, 0x61, 0x67, 0x61, 0x61, 0x61, 0x67, 0x61, 0x61, 0x61, 0x67, 0x61, 0x61, 0x61,
    };
}
