#include "stdafx.h"
#include "mg_stage_select.hpp"

#include "common.hpp"
#include "mg2_linkvarbuf.hpp"

namespace
{
    SafetyHookInline s_hSceneTick;
    SafetyHookMid s_hRoomSpawn;
    MG_StageSelect::Destination s_stRoomSpawn {};
    bool s_bRoomSpawnPending = false;

    void ApplyRoomSpawn(MG2_LinkVarBuf::ObjectPosition* pPlayer)
    {
        using namespace MG2_LinkVarBuf;
        if (!s_bRoomSpawnPending)
        {
            return;
        }
        s_bRoomSpawnPending = false;
        EngineState* pState = MG2_Gamefuncs::pfnGetEngineState();
        if (!pPlayer || !pState || GM_Area != s_stRoomSpawn.nArea)
        {
            spdlog::error("MG2: Stage Select : Room spawn interrupted.");
            return;
        }
        pPlayer->nX = s_stRoomSpawn.nSpawnX * nFixedPointOne;
        pPlayer->nY = s_stRoomSpawn.nSpawnY * nFixedPointOne;
        // player init automatially chooses standing or crawling from the destination tile.
        pPlayer->nState = 0;
        if (s_stRoomSpawn.nSpawnFacing >= 0)
        {
            pState->GM_Facing = s_stRoomSpawn.nSpawnFacing;
        }
        if (s_stRoomSpawn.bCrawlSpawn && (pState->GM_SelectedItem == MG2_ITEM_INDEX_CARDBOARD_BOX || pState->GM_SelectedItem == MG2_ITEM_INDEX_BUCKET))
        {
            pState->GM_SelectedItem = -1;
        }
        GM_EntryX = s_stRoomSpawn.bCrawlSpawn ? -1 : pPlayer->nX;
        GM_EntryY = s_stRoomSpawn.bCrawlSpawn ? -1 : pPlayer->nY;
        GM_EntryFacing = s_stRoomSpawn.bCrawlSpawn ? -1 : pState->GM_Facing;
    }

    void SceneTick_hook()
    {
        using namespace MG2_LinkVarBuf;
        const bool bAvailable = pScriptGlobals && GM_Scene == MG2_SCENE_GAMEPLAY && !GM_SceneChangePending && GM_InputMode == 0 && MG2_Gamefuncs::pfnIsGameplayActive() && !MG2_Gamefuncs::pfnIsBossSurvival();
        MG_StageSelect::Destination stDestination {};
        const auto eAction = MG_StageSelect::Service(MG_StageSelect::Game::MG2, bAvailable, GM_Area, GM_Room, stDestination);
        if (eAction == MG_StageSelect::Action::Pause)
        {
            return;
        }
        if (eAction == MG_StageSelect::Action::Travel)
        {
            EngineState* pState = MG2_Gamefuncs::pfnGetEngineState();
            ObjectPosition* pPlayer = MG2_Gamefuncs::pfnGetPlayer();
            Sprite* pSprite = pPlayer ? MG2_Gamefuncs::pfnGetObjectSprite(pPlayer) : nullptr;
            if (pState && pPlayer && pSprite)
            {
                s_stRoomSpawn = stDestination;
                s_bRoomSpawnPending = stDestination.nEntrance < 0 && stDestination.nSpawnX >= 0;
                // Room scripts wait for the entry flag set by normal doorway travel.
                MG2_Gamefuncs::pfnPrepareRoomEntry(pPlayer, pState, pSprite, stDestination.nEntrance, 0, 0);
                pState->GM_NextArea = stDestination.nArea;
                pState->GM_NextEntrance = stDestination.nEntrance;
                MG2_Gamefuncs::pfnRequestScene(MG2_SCENE_LOAD_AREA);
                MG2_Gamefuncs::pfnRunSceneTransition();
                spdlog::info("MG2: Stage Select : Area {}, room {}, entrance {}.", stDestination.nArea, stDestination.nRoom, stDestination.nEntrance);
            }
        }
        s_hSceneTick.call<void>();
    }
}

void MG_StageSelect::InitializeMG2()
{
    if (!MG_DeveloperMenu::bEnabled)
    {
        return;
    }
    //sub_180025A90+6A8
    uint8_t* pRoomSpawn = Memory::PatternScan(mg2Module, "48 8B CB E8 ?? ?? ?? ?? 8B 53", "MG2: Stage Select : sub_180025A90()+0x6A8 : place Snake before room initialization");
    if (!pRoomSpawn || !MG2_Gamefuncs::pfnSceneTick || !MG2_Gamefuncs::pfnGetEngineState || !MG2_Gamefuncs::pfnGetPlayer || !MG2_Gamefuncs::pfnGetObjectSprite || !MG2_Gamefuncs::pfnPrepareRoomEntry || !MG2_Gamefuncs::pfnRequestScene || !MG2_Gamefuncs::pfnRunSceneTransition || !MG2_Gamefuncs::pfnIsGameplayActive || !MG2_Gamefuncs::pfnIsBossSurvival || !MG2_LinkVarBuf::pScriptGlobals)
    {
        spdlog::error("MG2: Stage Select : Missing transition addresses. Skipping.");
    }
    else
    {
        s_hRoomSpawn = safetyhook::create_mid(pRoomSpawn, [](SafetyHookContext& ctx)
        {
            ApplyRoomSpawn(reinterpret_cast<MG2_LinkVarBuf::ObjectPosition*>(ctx.rbx));
        });
        LOG_HOOK(s_hRoomSpawn, "MG2: Stage Select : sub_180025A90()+0x6A8")
        if (!s_hRoomSpawn)
        {
            return;
        }
        s_hSceneTick = safetyhook::create_inline(MG2_Gamefuncs::pfnSceneTick, SceneTick_hook);
        LOG_HOOK(s_hSceneTick, "MG2: Stage Select : sub_180025A90()")
    }
}
