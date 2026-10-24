#include "stdafx.h"
#include "mg_stage_select.hpp"

#include "common.hpp"
#include "mg1_linkvarbuf.hpp"

namespace
{
    SafetyHookInline s_hSceneTick;

    void SceneTick_hook()
    {
        using namespace MG1_LinkVarBuf;
        const bool bAvailable = sceneState && roomPosition && sceneState->GM_CurrentScene == MG1_SCENE_GAMEPLAY && !sceneState->GM_SceneChangePending && GM_Life > 0 && !GM_BossSurvival && !GM_TelescopeMode;
        MG_StageSelect::Destination stDestination {};
        const auto eAction = MG_StageSelect::Service(MG_StageSelect::Game::MG1, bAvailable, roomPosition ? roomPosition->GM_Stage : 0, roomPosition ? roomPosition->GM_Room : 0, stDestination);
        if (eAction == MG_StageSelect::Action::Pause)
        {
            return;
        }
        if (eAction == MG_StageSelect::Action::Travel)
        {
            roomPosition->GM_EntranceKind = stDestination.nEntrance;
            MG1_Gamefuncs::pfnGotoStage(stDestination.nArea, stDestination.nRoom);
            // GotoStage releases the old room data; switch scenes before running another room tick.
            MG1_Gamefuncs::pfnSwitchScene();
            spdlog::info("MG1: Stage Select : Area {}, room {}, entrance {}.", stDestination.nArea, stDestination.nRoom, stDestination.nEntrance);
        }
        s_hSceneTick.call<void>();
    }
}

void MG_StageSelect::InitializeMG1()
{
    if (!MG_DeveloperMenu::bEnabled)
    {
        return;
    }
    if (!MG1_Gamefuncs::pfnSceneTick || !MG1_Gamefuncs::pfnGotoStage || !MG1_Gamefuncs::pfnSwitchScene || !MG1_LinkVarBuf::sceneState || !MG1_LinkVarBuf::roomPosition || !MG1_LinkVarBuf::playStats || !MG1_LinkVarBuf::pBossSurvival || !MG1_LinkVarBuf::pTelescopeMode)
    {
        spdlog::error("MG1: Stage Select : Missing transition addresses. Skipping.");
    }
    else
    {
        s_hSceneTick = safetyhook::create_inline(MG1_Gamefuncs::pfnSceneTick, SceneTick_hook);
        LOG_HOOK(s_hSceneTick, "MG1: Stage Select : TickGameScene()")
    }
}
