#include "stdafx.h"
#include "cutscene_pausing.hpp"
#include "common.hpp"
#include "gamevars.hpp"
#include "input_handler.hpp"
#include "logging.hpp"


void CutscenePausing::Setup()
{
    if (!(eGameType & MGS2))
    {
        return;
    }
    using namespace MGS2_StatusFlags;

    if (!bEnabled)
    {
        return;
    }

    g_InputHandler.RegisterHotkey(VK_F9, "Toggle Cutscene Pausing", []()
                                  {
                                      g_GameVars.GV_PauseLevel() ^= MGS2_GV_PAUSE_DEBUG;
                    spdlog::info("Pause Status: {:X}", g_GameVars.GV_PauseLevel());
                                  }); 

}
