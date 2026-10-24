#include "stdafx.h"
#include "mg2_companion_follow.hpp"

#include "common.hpp"
#include "mg2_linkvarbuf.hpp"

using namespace MG2_LinkVarBuf;

namespace
{
    constexpr int nSampleStep = 4 * nFixedPointOne;
    constexpr int nHistoryRecords = 16;
    constexpr std::array<int, 3> anFollowGaps = { 4 * nSampleStep, 4 * nSampleStep, 12 * nSampleStep };

    struct FollowPoint
    {
        double dX;
        double dY;
        double dDistance;
        int nFacing;
    };

    std::deque<FollowPoint> s_vHistory;
    FollowPoint s_stPrevious {};
    double s_dNextSample = nSampleStep;
    SafetyHookInline s_hReset;
    SafetyHookInline s_hUpdate;
    SafetyHookInline s_hScriptRun;
    SafetyHookInline s_hDrawSprite;
    bool s_bRestStopPatched = false;
    bool s_bRestStopWarned = false;

    struct ScriptPositionPatch
    {
        size_t nOffset;
        uint32_t nOriginal;
        uint32_t nRestored;
        size_t nSize = 2;
    };

    constexpr int nRestStopOriginY = 3 * nRoomHeight;
    constexpr int nMadnarExitY = nRestStopOriginY + 224;
    constexpr int nScriptedWalkStep = 2;
    constexpr int nSeatedOffsetY = -11;
    constexpr ScriptPositionPatch astRestStopPositions[] =
    {
        { 0x83, 632, nRestStopOriginY + 59 },
        { 0x8B, 632, nRestStopOriginY + 59 },
        { 0xA2, 632, nRestStopOriginY + 59 },
        // MC moves once more after setting Madnar's exit position.
        { 0x29B, 832, nMadnarExitY - nScriptedWalkStep },
        { 0x2C7, 832, nMadnarExitY - nScriptedWalkStep },
        { 0x497, 768, nRestStopOriginY + 190 },
        { 0x4F7, 672, nRestStopOriginY + 95 },
        { 0x513, 672, 2 * (nRestStopOriginY + 95) + 1 },
        { 0x516, nFixedPointOne, nFixedPointOne / 2, 4 },
        { 0x531, 672, nRestStopOriginY + 95 },
    };

    bool RestoreRestStopPositions(uint8_t* pCode)
    {
        for (const auto& stPatch : astRestStopPositions)
        {
            uint32_t nValue = 0;
            for (size_t nByte = 0; nByte < stPatch.nSize; nByte++)
            {
                nValue = (nValue << 8) | pCode[stPatch.nOffset + nByte];
            }
            if (pCode[stPatch.nOffset - 1] != stPatch.nSize || (nValue != stPatch.nOriginal && nValue != stPatch.nRestored))
            {
                return false;
            }
        }
        for (const auto& stPatch : astRestStopPositions)
        {
            for (size_t nByte = 0; nByte < stPatch.nSize; nByte++)
            {
                pCode[stPatch.nOffset + nByte] = static_cast<uint8_t>(stPatch.nRestored >> (8 * (stPatch.nSize - nByte - 1)));
            }
        }
        return true;
    }

    void __fastcall ScriptRun_hook(int* pScript)
    {
        if (IsScript(pScript, MG2_SCRIPT_REST_STOP))
        {
            auto* pCode = static_cast<uint8_t*>(MG2_Gamefuncs::pfnGetHandleData(pLoadedScripts[MG2_SCRIPT_REST_STOP]));
            s_bRestStopPatched = Memory::IsReadable(pCode, 0x533) && RestoreRestStopPositions(pCode);
            if (!s_bRestStopPatched && !s_bRestStopWarned)
            {
                spdlog::warn("MG2: Companion Follow : Unexpected rest-stop script. Keeping MC scene positions.");
                s_bRestStopWarned = true;
            }
        }
        s_hScriptRun.call<void>(pScript);
    }

    void __fastcall DrawSprite_hook(Sprite* pSprite, int nMode)
    {
        if (!s_bRestStopPatched || pSprite->nSheet != MG2_SPRITE_SHEET_GUSTAVA || pSprite->nAnim != MG2_COMPANION_ANIMATION_SEATED || !IsRoom(8, 12))
        {
            s_hDrawSprite.call<void>(pSprite, nMode);
            return;
        }
        const int nY = pSprite->nY;
        pSprite->nY += nSeatedOffsetY;
        s_hDrawSprite.call<void>(pSprite, nMode);
        pSprite->nY = nY;
    }

    void ResetHistory(int nX, int nY, int nFacing)
    {
        s_stPrevious = { static_cast<double>(nX), static_cast<double>(nY), 0.0, nFacing };
        s_vHistory.assign(1, s_stPrevious);
        s_dNextSample = nSampleStep;
    }

    void AppendPoint(const FollowPoint& stPoint)
    {
        s_vHistory.push_back(stPoint);
        const double dOldest = stPoint.dDistance - nHistoryRecords * nSampleStep;
        while (s_vHistory.size() > 2 && s_vHistory[1].dDistance <= dOldest)
        {
            s_vHistory.pop_front();
        }
    }

    bool RecordMovement(int nX, int nY, int nFacing)
    {
        const double dX = nX - s_stPrevious.dX;
        const double dY = nY - s_stPrevious.dY;
        const double dLength = std::hypot(dX, dY);
        if (dLength == 0.0)
        {
            return false;
        }
        const double dEnd = s_stPrevious.dDistance + dLength;
        while (s_dNextSample <= dEnd)
        {
            const double dFraction = (s_dNextSample - s_stPrevious.dDistance) / dLength;
            AppendPoint({ s_stPrevious.dX + dX * dFraction, s_stPrevious.dY + dY * dFraction, s_dNextSample, nFacing });
            s_dNextSample += nSampleStep;
        }
        // Keep turn points between samples so interpolation follows corners instead of cutting across them.
        if (s_vHistory.back().dDistance < dEnd)
        {
            AppendPoint({ static_cast<double>(nX), static_cast<double>(nY), dEnd, nFacing });
        }
        s_stPrevious = { static_cast<double>(nX), static_cast<double>(nY), dEnd, nFacing };
        return true;
    }

    FollowPoint GetTarget(int nGap)
    {
        const double dTarget = s_stPrevious.dDistance - nGap;
        if (dTarget <= s_vHistory.front().dDistance)
        {
            return s_vHistory.front();
        }
        for (size_t nIndex = 1; nIndex < s_vHistory.size(); nIndex++)
        {
            const auto& stEnd = s_vHistory[nIndex];
            if (dTarget <= stEnd.dDistance)
            {
                const auto& stStart = s_vHistory[nIndex - 1];
                const double dFraction = (dTarget - stStart.dDistance) / (stEnd.dDistance - stStart.dDistance);
                return { stStart.dX + (stEnd.dX - stStart.dX) * dFraction, stStart.dY + (stEnd.dY - stStart.dY) * dFraction, dTarget, stEnd.nFacing };
            }
        }
        return s_vHistory.back();
    }

    void __fastcall ResetCompanionFollow_hook()
    {
        const auto* pSnake = MG2_Gamefuncs::pfnGetPlayer();
        const auto* pState = MG2_Gamefuncs::pfnGetEngineState();
        ResetHistory(pSnake->nX, pSnake->nY, pState->GM_Facing);
        const auto* ahCompanions = static_cast<const uintptr_t*>(MG2_Gamefuncs::pfnGetHandleData(*pCompanionList));
        for (int nIndex = 2; nIndex >= 0; nIndex--)
        {
            if (pState->anCompanionFollow[nIndex] == 0)
            {
                continue;
            }
            auto* pCompanion = static_cast<ObjectPosition*>(MG2_Gamefuncs::pfnGetHandleData(ahCompanions[nIndex]));
            pCompanion->nX = pSnake->nX;
            pCompanion->nY = pSnake->nY;
            pCompanion->nMovementInput = 0;
            pCompanion->nSpeed = 0;
        }
    }

    void __fastcall UpdateCompanionFollow_hook()
    {
        const auto* pSnake = MG2_Gamefuncs::pfnGetPlayer();
        const auto* pState = MG2_Gamefuncs::pfnGetEngineState();
        if (s_vHistory.empty())
        {
            ResetHistory(pSnake->nX, pSnake->nY, pState->GM_Facing);
        }
        const bool bFacingOnly = pState->nMovementBlocked != 0;
        // Scripted stops can reposition Snake after clearing his movement input.
        const int nFacing = pState->nMovementDirection != -1 ? pState->nMovementDirection : s_stPrevious.nFacing;
        const bool bMoved = !bFacingOnly && pState->nTurnState == 0 && RecordMovement(pSnake->nX, pSnake->nY, nFacing);
        const auto* ahCompanions = static_cast<const uintptr_t*>(MG2_Gamefuncs::pfnGetHandleData(*pCompanionList));
        for (int nIndex = 2; nIndex >= 0; nIndex--)
        {
            if (pState->anCompanionFollow[nIndex] == 0)
            {
                continue;
            }
            auto* pCompanion = static_cast<ObjectPosition*>(MG2_Gamefuncs::pfnGetHandleData(ahCompanions[nIndex]));
            if (bFacingOnly)
            {
                pCompanion->nMovementInput = pState->nMovementDirection + 1;
                continue;
            }
            pCompanion->nSpeed = 0;
            pCompanion->nMovementInput = 0;
            if (!bMoved)
            {
                continue;
            }
            const auto stTarget = GetTarget(anFollowGaps[nIndex]);
            const int nX = static_cast<int>(std::llround(stTarget.dX));
            const int nY = static_cast<int>(std::llround(stTarget.dY));
            if (pCompanion->nX != nX || pCompanion->nY != nY)
            {
                pCompanion->nX = nX;
                pCompanion->nY = nY;
                pCompanion->nMovementInput = stTarget.nFacing + 1;
            }
        }
    }
}

void MG2_CompanionFollow::Initialize()
{
    if (!bEnabled)
    {
        return;
    }
    if (!MG2_Gamefuncs::pfnResetCompanionFollow || !MG2_Gamefuncs::pfnUpdateCompanionFollow || !MG2_Gamefuncs::pfnGetPlayer || !MG2_Gamefuncs::pfnGetEngineState || !MG2_Gamefuncs::pfnGetHandleData || !pCompanionList || !pLoadedScripts || !pRunningScriptCode || !pScriptGlobals || !MG2_Gamefuncs::pfnScriptRun || !MG2_Gamefuncs::pfnDrawSprite)
    {
        spdlog::error("MG2: Companion Follow : Missing companion functions. Skipping.");
    }
    else
    {
        s_hReset = safetyhook::create_inline(MG2_Gamefuncs::pfnResetCompanionFollow, ResetCompanionFollow_hook);
        LOG_HOOK(s_hReset, "MG2: Companion Follow : ResetCompanionFollow()")
        s_hUpdate = safetyhook::create_inline(MG2_Gamefuncs::pfnUpdateCompanionFollow, UpdateCompanionFollow_hook);
        LOG_HOOK(s_hUpdate, "MG2: Companion Follow : UpdateCompanionFollow()")
        s_hScriptRun = safetyhook::create_inline(MG2_Gamefuncs::pfnScriptRun, ScriptRun_hook);
        LOG_HOOK(s_hScriptRun, "MG2: Companion Follow : ScriptRun()")
        s_hDrawSprite = safetyhook::create_inline(MG2_Gamefuncs::pfnDrawSprite, DrawSprite_hook);
        LOG_HOOK(s_hDrawSprite, "MG2: Companion Follow : DrawSprite()")
        if (!s_hReset || !s_hUpdate || !s_hScriptRun || !s_hDrawSprite)
        {
            s_hDrawSprite.reset();
            s_hScriptRun.reset();
            s_hUpdate.reset();
            s_hReset.reset();
            spdlog::error("MG2: Companion Follow : Incomplete hook setup. Disabled restoration.");
        }
        else
        {
            spdlog::info("MG2: Companion Follow : Initialized.");
        }
    }
}
