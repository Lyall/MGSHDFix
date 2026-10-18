#pragma once

#include "mg_developer_menu.hpp"

namespace MG_StageSelect
{
    enum class Game
    {
        MG1,
        MG2
    };
    enum class Action
    {
        Run,
        Pause,
        Travel
    };

    struct Destination
    {
        int nArea;
        int nRoom;
        int nEntrance;
        int nSpawnX = -1;
        int nSpawnY = -1;
        bool bCrawlSpawn = false;
        int nSpawnFacing = -1;
        const char* pszSpawnName = "Room spawn";
    };

    Action Service(Game eGame, bool bAvailable, int nCurrentArea, int nCurrentRoom, Destination& stDestination);
    void InitializeMG1();
    void InitializeMG2();
}
