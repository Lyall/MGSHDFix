#include "stdafx.h"
#include "mg2_truckyard_fence.hpp"
#include "mg2_truckyard_fence_sprites.hpp"

#include "common.hpp"
#include "mg2_linkvarbuf.hpp"
#include "mg2_sprite_patcher.hpp"

using namespace MG2_LinkVarBuf;

namespace
{
    constexpr int nArea = 15;
    constexpr int nRoom = 1;
    constexpr int nFenceX = 64;
    constexpr int nFenceY = 128;
    constexpr int nFenceSize = 40;
    constexpr int nBlockedWidth = 4;
    constexpr int nBlockedHeight = 4;
    constexpr uint16_t nBlockedFlags = MG2_CELL_BLOCK_PLAYER | MG2_CELL_BLOCK_ACTORS;
    constexpr uint16_t aanOpenCells[nBlockedHeight][nBlockedWidth] =
    {
        { 4, 4, 5, 6 },
        { 4, 6, 4, 4 },
        { 6, 5, 6, 6 },
        { 4, 6, 4, 5 },
    };

    bool s_bInitialized = false;
    bool s_bFenceClosed = false;
    SafetyHookMid s_hDrawFence {};

    void DrawFence(SafetyHookContext& ctx)
    {
        if (!s_bFenceClosed)
        {
            return;
        }
        for (int nY = 0; nY < nFenceSize; nY++)
        {
            for (int nX = 0; nX < nFenceSize;)
            {
                const int nColour = MG2_TruckyardFenceSprites::aanPixels[nY][nX];
                const int nStart = nX++;
                while (nX < nFenceSize && MG2_TruckyardFenceSprites::aanPixels[nY][nX] == nColour)
                {
                    nX++;
                }
                const int anRectangle[] = { nFenceX + nStart, nFenceY + nY, nX - nStart, 1 };
                MG2_Gamefuncs::pfnSetDrawColor(MG2_LAYER_ROOM, nColour);
                MG2_Gamefuncs::pfnLayerFillRect(MG2_LAYER_ROOM, anRectangle);
            }
        }
    }
}

void MG2_TruckyardFence::HandleLevelTransition()
{
    s_bFenceClosed = false;
    if (!s_bInitialized || !IsArea(nArea) || MG2_Gamefuncs::pfnIsBossSurvival() != 0 || !*pAreaMapChunk)
    {
        return;
    }

    const uintptr_t hChunk = *pAreaMapChunk;
    const int nEntry = nRoomCellEntry + nRoom;
    if (MG2_Gamefuncs::pfnGetChunkEntrySize(hChunk, nEntry) != nRoomCellMapSize)
    {
        spdlog::warn("MG2: Truckyard Fence : Unexpected room cell map size. Skipping.");
        return;
    }
    auto* pCells = MG2_Gamefuncs::pfnGetChunkEntryData(hChunk, nEntry);
    if (!Memory::IsWritable(pCells, nRoomCellMapSize))
    {
        spdlog::warn("MG2: Truckyard Fence : Room cells are not writable. Skipping.");
        return;
    }
    for (int nY = 0; nY < nBlockedHeight; nY++)
    {
        for (int nX = 0; nX < nBlockedWidth; nX++)
        {
            const int nOffset = 2 * ((nFenceY / nRoomCellSize + nY) * nRoomCellColumns + nFenceX / nRoomCellSize + nX);
            if ((Memory::ReadBE16(pCells + nOffset) & ~nBlockedFlags) != aanOpenCells[nY][nX])
            {
                spdlog::warn("MG2: Truckyard Fence : Room cells have changed. Skipping.");
                return;
            }
        }
    }

    const bool bClosed = MG2_Gamefuncs::pfnTestFlag(MG2_FLAG_A15_R4_BRIDGE_OF_SORROW_NASTACHA_DEATH) == 0;
    for (int nY = 0; nY < nBlockedHeight; nY++)
    {
        for (int nX = 0; nX < nBlockedWidth; nX++)
        {
            const int nOffset = 2 * ((nFenceY / nRoomCellSize + nY) * nRoomCellColumns + nFenceX / nRoomCellSize + nX);
            Memory::WriteBE16(pCells + nOffset, aanOpenCells[nY][nX] | (bClosed ? nBlockedFlags : 0));
        }
    }
    s_bFenceClosed = bClosed && GM_Room == nRoom;
}

void MG2_TruckyardFence::Initialize()
{
    if (!bEnabled)
    {
        return;
    }

    //DrawRoom+22D
    uint8_t* pDrawFence = Memory::PatternScan(mg2Module, "44 39 2D ?? ?? ?? ?? 4C 8B 7C 24", "MG2: Truckyard Fence : DrawRoom()+0x22D");
    if (!pDrawFence || !pAreaMapChunk || !pScriptGlobals || !MG2_Gamefuncs::pfnTestFlag || !MG2_Gamefuncs::pfnIsBossSurvival || !MG2_Gamefuncs::pfnGetChunkEntryData || !MG2_Gamefuncs::pfnGetChunkEntrySize || !MG2_Gamefuncs::pfnSetDrawColor || !MG2_Gamefuncs::pfnLayerFillRect || !MG2_SpritePatcher::bAllowReplacements)
    {
        spdlog::error("MG2: Truckyard Fence : Missing room or drawing functions. Skipping.");
    }
    else
    {
        s_hDrawFence = safetyhook::create_mid(pDrawFence, DrawFence);
        LOG_HOOK(s_hDrawFence, "MG2: Truckyard Fence : DrawRoom()+0x22D")
        if (s_hDrawFence)
        {
            // Area 15 uses only palette slots 192 -> 201; 202 has the brown we need
            if (!MG2_SpritePatcher::AddPalettePatch(26, 10, 0x6D4800))
            {
                s_hDrawFence.reset();
                spdlog::error("MG2: Truckyard Fence : Could not register the fence colour. Skipping.");
                return;
            }
            s_bInitialized = true;
            spdlog::info("MG2: Truckyard Fence : Initialized.");
        }
    }
}
