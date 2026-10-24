#include "stdafx.h"
#include "mg2_outdoor_tiles.hpp"
#include "mg2_outdoor_tiles_data.hpp"

#include "common.hpp"
#include "mg2_linkvarbuf.hpp"

using namespace MG2_LinkVarBuf;

namespace
{
    struct ReplacementImage
    {
        const uint8_t* pPixels = nullptr;
        DrawLayer stImage{};

        void Initialize(const uint8_t* pData, int nWidth, int nHeight)
        {
            pPixels = pData;
            stImage.hPixels = reinterpret_cast<uintptr_t>(&pPixels);
            stImage.nWidth = nWidth;
            stImage.nHeight = nHeight;
        }
    };

    constexpr int nDoorX = 112;
    constexpr int nDoorY = 62;
    constexpr int nDoorWidth = 32;
    constexpr int nDoorHeight = 2;
    constexpr int anTileRectangle[] = { 0, 0, nRoomCellSize, nRoomCellSize };
    ReplacementImage s_astTiles[std::size(MG2_OutdoorTilesData::astTiles)]{};
    ReplacementImage s_stDoor{};
    int s_anRoomTiles[nRoomCellColumns * nRoomCellColumns]{};
    bool s_bInitialized = false;
    bool s_bRoomActive = false;
    bool s_bDoorActive = false;
    SafetyHookMid s_hDrawTile{};
    SafetyHookMid s_hDrawDoor{};

    bool MatchesImage(const DrawLayer* pstImage, const int* pnRect, const uint8_t* pnExpected, int nWidth, int nHeight, int nOffsetX, int nOffsetY)
    {
        const int nX = pnRect[0] - nOffsetX;
        const int nY = pnRect[1] - nOffsetY;
        if (nX < 0 || nY < 0 || pnRect[2] <= 0 || pnRect[3] <= 0 || pnRect[2] > nWidth - nX || pnRect[3] > nHeight - nY)
        {
            return false;
        }
        if (pnRect[0] < 0 || pnRect[1] < 0 || pnRect[2] > pstImage->nWidth - pnRect[0] || pnRect[3] > pstImage->nHeight - pnRect[1] || !Memory::IsReadable(reinterpret_cast<void*>(pstImage->hPixels), sizeof(uintptr_t)))
        {
            return false;
        }
        const auto* pnPixels = *reinterpret_cast<const uint8_t* const*>(pstImage->hPixels);
        for (int nRow = 0; nRow < pnRect[3]; nRow++)
        {
            const auto* pnSource = pnPixels + static_cast<size_t>(pnRect[1] + nRow) * pstImage->nWidth + pnRect[0];
            if (!Memory::IsReadable(pnSource, pnRect[2]) || std::memcmp(pnSource, pnExpected + (nY + nRow) * nWidth + nX, pnRect[2]) != 0)
            {
                return false;
            }
        }
        return true;
    }

    void DrawTile(SafetyHookContext& ctx)
    {
        if (!s_bRoomActive)
        {
            return;
        }
        const int nX = static_cast<int>(ctx.r9);
        const int nY = static_cast<int>(ctx.r14);
        if (nX < 0 || nY < 0 || nX >= nRoomWidth || nY >= nRoomHeight || nX % nRoomCellSize || nY % nRoomCellSize)
        {
            return;
        }
        const int nIndex = s_anRoomTiles[nY / nRoomCellSize * nRoomCellColumns + nX / nRoomCellSize] - 1;
        if (nIndex < 0)
        {
            return;
        }
        const auto* pnRect = reinterpret_cast<const int*>(ctx.r8);
        const auto* pstImage = reinterpret_cast<const DrawLayer*>(ctx.rdx);
        if (pnRect[2] != nRoomCellSize || pnRect[3] != nRoomCellSize || !MatchesImage(pstImage, pnRect, MG2_OutdoorTilesData::astTiles[nIndex].anOriginal, nRoomCellSize, nRoomCellSize, pnRect[0], pnRect[1]))
        {
            return;
        }
        ctx.rdx = reinterpret_cast<uintptr_t>(&s_astTiles[nIndex].stImage);
        ctx.r8 = reinterpret_cast<uintptr_t>(anTileRectangle);
    }

    void DrawDoor(SafetyHookContext& ctx)
    {
        if (!s_bDoorActive)
        {
            return;
        }
        const auto* pstSprite = reinterpret_cast<const Sprite*>(ctx.rsi);
        if (pstSprite->nSheet != MG2_SPRITE_SHEET_DOORS || (pstSprite->nAnim != MG2_DOOR_ANIM_OUTDOOR_OPEN && pstSprite->nAnim != MG2_DOOR_ANIM_OUTDOOR_CLOSE))
        {
            return;
        }
        auto* pnRect = reinterpret_cast<int*>(ctx.r8);
        const auto* pstImage = reinterpret_cast<const DrawLayer*>(ctx.rdx);
        if (!MatchesImage(pstImage, pnRect, MG2_OutdoorTilesData::anDoorOriginal, nDoorWidth, nDoorHeight, nDoorX, nDoorY))
        {
            return;
        }

        pnRect[0] -= nDoorX;
        pnRect[1] -= nDoorY;
        ctx.rdx = reinterpret_cast<uintptr_t>(&s_stDoor.stImage);
    }
}

void MG2_OutdoorTiles::HandleLevelTransition()
{
    s_bRoomActive = false;
    s_bDoorActive = false;
    std::fill(std::begin(s_anRoomTiles), std::end(s_anRoomTiles), 0);
    if (!s_bInitialized || !IsArea(0) || MG2_Gamefuncs::pfnIsBossSurvival() != 0)
    {
        return;
    }
    for (size_t nIndex = 0; nIndex < std::size(MG2_OutdoorTilesData::astTiles); nIndex++)
    {
        const auto& stTile = MG2_OutdoorTilesData::astTiles[nIndex];
        if (GM_Room == stTile.nRoom)
        {
            s_anRoomTiles[stTile.nY / nRoomCellSize * nRoomCellColumns + stTile.nX / nRoomCellSize] = static_cast<int>(nIndex) + 1;
            s_bRoomActive = true;
        }
    }
    s_bDoorActive = GM_Room == 8;
}

void MG2_OutdoorTiles::Initialize()
{
    if (!bEnabled)
    {
        return;
    }

    //DrawRoom+20F
    uint8_t* pDrawTile = Memory::PatternScan(mg2Module, "E8 ?? ?? ?? ?? 41 2B DD", "MG2: Outdoor Artwork : DrawRoom()+0x20F");
    //DrawSprite+40D
    uint8_t* pDrawDoor = Memory::PatternScan(mg2Module, "E8 ?? ?? ?? ?? 41 2B 7D ?? ?? ?? ?? ?? ?? ?? ?? EB", "MG2: Outdoor Artwork : DrawSprite()+0x40D");
    if (!pDrawTile || !pDrawDoor || !pScriptGlobals || !MG2_Gamefuncs::pfnIsBossSurvival)
    {
        spdlog::error("MG2: Outdoor Artwork : Missing room or drawing addresses. Skipping.");
    }
    else
    {
        for (size_t nIndex = 0; nIndex < std::size(s_astTiles); nIndex++)
        {
            s_astTiles[nIndex].Initialize(MG2_OutdoorTilesData::astTiles[nIndex].anRestored, nRoomCellSize, nRoomCellSize);
        }
        s_stDoor.Initialize(MG2_OutdoorTilesData::anDoorRestored, nDoorWidth, nDoorHeight);
        s_hDrawTile = safetyhook::create_mid(pDrawTile, DrawTile);
        LOG_HOOK(s_hDrawTile, "MG2: Outdoor Artwork : DrawRoom()+0x20F")
        s_hDrawDoor = safetyhook::create_mid(pDrawDoor, DrawDoor);
        LOG_HOOK(s_hDrawDoor, "MG2: Outdoor Artwork : DrawSprite()+0x40D")
        if (!s_hDrawTile || !s_hDrawDoor)
        {
            s_hDrawTile.reset();
            s_hDrawDoor.reset();
            return;
        }
        s_bInitialized = true;
        spdlog::info("MG2: Outdoor Artwork : Initialized.");
    }
}
