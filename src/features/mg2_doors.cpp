#include "stdafx.h"
#include "mg2_doors.hpp"

#include "mg2_door_sprites.hpp"
#include "mg2_door_fixes.hpp"
#include "common.hpp"
#include "mg2_linkvarbuf.hpp"

using namespace MG2_LinkVarBuf;

namespace
{
    struct Threshold
    {
        int nArea;
        int nRoom;
        int nX;
        int nY;
    };

    constexpr Threshold astThresholds[] =
    {
        { 8, 0, 112, 95 },
        { 8, 7, 80, 95 },
        { 8, 15, 80, 95 },
    };
    const Threshold* s_pThreshold = nullptr;
    std::vector<const MG2_DoorFixes::Backing*> s_vBackings;
    SafetyHookInline s_hDrawSprites {};

    void __fastcall DrawSprites_hook()
    {
        if (!s_vBackings.empty() && GM_Scene == MG2_SCENE_GAMEPLAY && GM_InsideElevator == 0)
        {
            for (const auto* pBacking : s_vBackings)
            {
                for (int nY = 0; nY < pBacking->nHeight; nY++)
                {
                    for (int nX = 0; nX < pBacking->nWidth;)
                    {
                        const int nColour = pBacking->anPixels[nY * pBacking->nWidth + nX];
                        const int nStart = nX++;
                        while (nX < pBacking->nWidth && pBacking->anPixels[nY * pBacking->nWidth + nX] == nColour)
                        {
                            nX++;
                        }
                        if (nColour == 0)
                        {
                            continue;
                        }
                        const int anRectangle[] = { pBacking->nX + nStart, pBacking->nY + nY, nX - nStart, 1 };
                        MG2_Gamefuncs::pfnSetDrawColor(MG2_LAYER_ROOM, nColour);
                        MG2_Gamefuncs::pfnLayerFillRect(MG2_LAYER_ROOM, anRectangle);
                    }
                }
            }
        }
        if (s_pThreshold && GM_Scene == MG2_SCENE_GAMEPLAY && GM_InsideElevator == 0)
        {
            const auto& anPixels = MG2_DoorSprites::anThresholdPixels;
            const int nWidth = static_cast<int>(std::size(anPixels));
            for (int nX = 0; nX < nWidth;)
            {
                const int nColour = anPixels[nX];
                const int nStart = nX++;
                while (nX < nWidth && anPixels[nX] == nColour)
                {
                    nX++;
                }
                const int anRectangle[] = { s_pThreshold->nX + nStart, s_pThreshold->nY, nX - nStart, 1 };
                MG2_Gamefuncs::pfnSetDrawColor(MG2_LAYER_ROOM, nColour);
                MG2_Gamefuncs::pfnLayerFillRect(MG2_LAYER_ROOM, anRectangle);
            }
        }
        s_hDrawSprites.call<void>();
    }
}

void MG2_Doors::HandleLevelTransition()
{
    s_pThreshold = nullptr;
    s_vBackings.clear();
    if (!s_hDrawSprites)
    {
        return;
    }
    for (const auto& stBacking : MG2_DoorFixes::astBackings)
    {
        if (IsRoom(stBacking.nArea, stBacking.nRoom))
        {
            s_vBackings.push_back(&stBacking);
        }
    }
    for (const auto& stThreshold : astThresholds)
    {
        if (IsRoom(stThreshold.nArea, stThreshold.nRoom))
        {
            s_pThreshold = &stThreshold;
            break;
        }
    }
}

void MG2_Doors::Initialize()
{
    if (!bEnabled)
    {
        return;
    }
    for (const auto& stDoor : MG2_DoorFixes::astDoors)
    {
        MG2_RoomPatches::AddDoor(stDoor);
    }
    if (!pScriptGlobals || !MG2_Gamefuncs::pfnDrawSprites || !MG2_Gamefuncs::pfnSetDrawColor || !MG2_Gamefuncs::pfnLayerFillRect)
    {
        spdlog::error("MG2: Door Sprites : Missing drawing functions. Skipping elevator doors and backings.");
    }
    else
    {
        s_hDrawSprites = safetyhook::create_inline(MG2_Gamefuncs::pfnDrawSprites, DrawSprites_hook);
        LOG_HOOK(s_hDrawSprites, "MG2: Door Sprites : DrawSprites()")
        if (s_hDrawSprites)
        {
            for (const auto& stDoor : MG2_DoorSprites::astElevatorDoors)
            {
                MG2_RoomPatches::AddDoorArtwork(stDoor);
            }
            spdlog::info("MG2: Door Sprites : Elevator thresholds and {} door backings initialized.", std::size(MG2_DoorFixes::astBackings));
        }
    }
    for (const auto& stDoor : MG2_DoorSprites::astA1Doors)
    {
        MG2_RoomPatches::AddDoorArtwork(stDoor);
    }
    for (const auto& stDoor : MG2_DoorSprites::astA5Doors)
    {
        MG2_RoomPatches::AddDoorArtwork(stDoor);
    }
    for (const auto& stDoor : MG2_DoorSprites::astA17Doors)
    {
        MG2_RoomPatches::AddDoorArtwork(stDoor);
    }
    for (const auto& stDoor : MG2_DoorSprites::astDoors)
    {
        MG2_RoomPatches::AddDoorArtwork(stDoor);
    }
}
