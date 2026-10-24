#include "stdafx.h"
#include "mg2_bridge.hpp"
#include "mg2_bridge_data.hpp"

#include "common.hpp"
#include "mg2_linkvarbuf.hpp"
#include "stb_image.h"

using namespace MG2_LinkVarBuf;

namespace
{
    enum class BridgePhase
    {
        None,
        Initial,
        Madnar,
        Gustava,
        Explosion,
        SnakeRight,
        Guards,
        Close,
        Reveal,
        SnakeLeave,
    };

    constexpr int nOriginX = 2 * nRoomWidth;
    constexpr int nOriginY = nRoomHeight;
    constexpr int nBlack = 4;
    constexpr int nWhite = 2;
    constexpr int nFlashColour = 254;
    SafetyHookInline s_hScriptRun;
    SafetyHookInline s_hScriptCommand;
    SafetyHookInline s_hDrawSprites;
    SafetyHookInline s_hDrawSprite;
    bool s_bRoom = false;
    bool s_bPatched = false;
    bool s_bActive = false;
    bool s_bWarned = false;
    BridgePhase s_ePhase = BridgePhase::None;
    int s_nPhaseTick = 0;
    int s_nSequence = 0;
    unsigned int s_nFrame = 0;
    bool s_bShutter = false;
    int s_nEyeTick = 0;
    int s_nEyeFrame = 0;
    int s_nForcedEyes = -1;
    const char* s_pDyingMessage = nullptr;
    int s_nDyingMessageLength = 0;
    int s_nEyesOpenPos = -1;
    int s_nEyesClosedPos = -1;
    std::vector<MG2_BridgeData::Span> s_vSpans;
    Sprite* s_pRoomSprite = nullptr;
    Sprite* s_pSnakeSprite = nullptr;

    ObjectPosition* GetCompanion(int nIndex)
    {
        const auto* ahCompanions = static_cast<const uintptr_t*>(MG2_Gamefuncs::pfnGetHandleData(*pCompanionList));
        return ahCompanions[nIndex] ? static_cast<ObjectPosition*>(MG2_Gamefuncs::pfnGetHandleData(ahCompanions[nIndex])) : nullptr;
    }

    const MG2_BridgeData::Frame& GetFrame()
    {
        const auto& stSequence = MG2_BridgeData::astSequences[s_nSequence];
        return MG2_BridgeData::astFrames[stSequence.nFirstFrame + std::min(s_nFrame, stSequence.nFrameCount - 1)];
    }

    void PositionCompanion(ObjectPosition* pCompanion, int nX, int nY)
    {
        if (!pCompanion)
        {
            return;
        }
        pCompanion->nMovementInput = 0;
        pCompanion->nSpeed = 0;
        pCompanion->nWalkSpeed = 0;
        pCompanion->nX = nOriginX * nFixedPointOne + nX * 256;
        pCompanion->nY = nOriginY * nFixedPointOne + nY * 256;
    }

    void PositionCompanions()
    {
        const auto& stFrame = GetFrame();
        if (s_nSequence <= 2)
        {
            PositionCompanion(GetCompanion(1), stFrame.nGustavaX, stFrame.nGustavaY);
        }
        PositionCompanion(GetCompanion(2), stFrame.nMadnarX, stFrame.nMadnarY);
    }

    bool IsDyingCall()
    {
        return s_bActive && s_bRoom && s_nSequence == 7 && GM_Scene == MG2_SCENE_FACE_TO_FACE;
    }

    bool AdvanceSequence(int nSequence)
    {
        s_nSequence = nSequence;
        const auto& stSequence = MG2_BridgeData::astSequences[nSequence];
        s_nFrame = std::min(static_cast<unsigned int>(s_nPhaseTick), stSequence.nFrameCount - 1);
        s_nEyeTick = GetFrame().nEyeTick;
        s_nEyeFrame = GM_FrameCounter;
        if (nSequence <= 4)
        {
            PositionCompanions();
        }
        return s_nFrame + 1 == stSequence.nFrameCount;
    }

    bool MoveSnake(bool bLeaving)
    {
        AdvanceSequence(bLeaving ? 8 : 7);
        auto* pSnake = MG2_Gamefuncs::pfnGetPlayer();
        auto* pState = MG2_Gamefuncs::pfnGetEngineState();
        pSnake->nSpeed = 0;
        pSnake->nWalkSpeed = 0;
        pSnake->nMovementInput = 0;
        const int nTargetX = (nOriginX + (bLeaving ? 112 : 136)) * nFixedPointOne;
        const int nTargetY = (nOriginY + (bLeaving ? 124 : 156)) * nFixedPointOne;
        const int nStep = 4 * nFixedPointOne;
        int nFacing = MG2_FACING_RIGHT;
        if (bLeaving)
        {
            const auto& stFrame = GetFrame();
            nFacing = stFrame.nSnakeX == 112 ? MG2_FACING_UP : MG2_FACING_LEFT;
            pSnake->nX = (nOriginX + stFrame.nSnakeX) * nFixedPointOne;
            pSnake->nY = (nOriginY + stFrame.nSnakeY) * nFixedPointOne;
        }
        else if (s_nPhaseTick == 0)
        {
            return false;
        }
        else if (pSnake->nX < nTargetX)
        {
            pSnake->nX = std::min(pSnake->nX + nStep, nTargetX);
        }
        else if (pSnake->nX > nTargetX)
        {
            nFacing = MG2_FACING_LEFT;
            pSnake->nX = std::max(pSnake->nX - nStep, nTargetX);
        }
        else if (pSnake->nY > nTargetY)
        {
            nFacing = MG2_FACING_UP;
            pSnake->nY = std::max(pSnake->nY - nStep, nTargetY);
        }
        pState->GM_Facing = nFacing;
        return pSnake->nX == nTargetX && pSnake->nY == nTargetY;
    }

    bool AdvancePhase(BridgePhase ePhase)
    {
        switch (ePhase)
        {
        case BridgePhase::Initial:
            s_bActive = true;
            s_pDyingMessage = nullptr;
            s_nForcedEyes = -1;
            s_bShutter = false;
            s_nSequence = 0;
            s_nFrame = 0;
            PositionCompanions();
            return true;
        case BridgePhase::Madnar:
            return AdvanceSequence(1);
        case BridgePhase::Gustava:
            return AdvanceSequence(2);
        case BridgePhase::Explosion:
            if (s_nPhaseTick == 0)
            {
                auto* pSnake = MG2_Gamefuncs::pfnGetPlayer();
                pSnake->nY = (nOriginY + 156) * nFixedPointOne;
                pSnake->nSpeed = 0;
                pSnake->nMovementInput = 0;
            }
            if (s_nPhaseTick == 12)
            {
                MG2_Gamefuncs::pfnRequestSound(MG2_SOUND_BRIDGE_EXPLOSION);
            }
            return AdvanceSequence(3);
        case BridgePhase::SnakeRight:
            return MoveSnake(false);
        case BridgePhase::Guards:
        {
            const bool bFinished = AdvanceSequence(4);
            PositionCompanion(GetCompanion(4), 104 * 256, GetFrame().nGuardY * 256);
            PositionCompanion(GetCompanion(5), 128 * 256, GetFrame().nGuardY * 256);
            return bFinished;
        }
        case BridgePhase::Close:
            s_bShutter = true;
            return AdvanceSequence(5);
        case BridgePhase::Reveal:
        {
            const int nPreviousHeight = GetFrame().nShutter;
            const bool bFinished = AdvanceSequence(6);
            const int nHeight = GetFrame().nShutter;
            if ((nPreviousHeight > 80 && nHeight <= 80) || (nPreviousHeight > 40 && nHeight <= 40))
            {
                MG2_Gamefuncs::pfnRequestSound(MG2_SOUND_METAL_GEAR_REVEAL);
            }
            if (bFinished)
            {
                s_bShutter = false;
                return true;
            }
            return false;
        }
        case BridgePhase::SnakeLeave:
            if (MoveSnake(true))
            {
                MG2_Gamefuncs::pfnGetPlayer()->nWalkSpeed = 4 * nFixedPointOne;
                return true;
            }
            return false;
        default:
            return true;
        }
    }

    int __fastcall ScriptCommand_hook(int* pScript, uint8_t nCommand, int* pArgs, char* pStrings, int nCount)
    {
        if (s_bPatched && s_bRoom && nCommand == MG2_COMMAND_YIELD && nCount == 1 && IsScript(pScript, MG2_SCRIPT_BRIDGE))
        {
            const auto ePhase = static_cast<BridgePhase>(pArgs[0]);
            if (ePhase != s_ePhase)
            {
                s_ePhase = ePhase;
                s_nPhaseTick = 0;
            }
            const bool bFinished = AdvancePhase(ePhase);
            ++s_nPhaseTick;
            if (!bFinished)
            {
                pScript[MG2_SCRIPT_PROG_COUNT] = pScript[MG2_SCRIPT_INSTRUCTION_PROG_COUNT];
                return 1;
            }
            s_ePhase = BridgePhase::None;
            return 0;
        }
        return s_hScriptCommand.call<int>(pScript, nCommand, pArgs, pStrings, nCount);
    }

    void __fastcall ScriptRun_hook(int* pScript)
    {
        if (s_bRoom && IsScript(pScript, MG2_SCRIPT_BRIDGE) && pScript[MG2_SCRIPT_PROG_COUNT] == 0)
        {
            s_bActive = false;
            auto* pCode = static_cast<uint8_t*>(MG2_Gamefuncs::pfnGetHandleData(pLoadedScripts[MG2_SCRIPT_BRIDGE]));
            constexpr auto nSize = sizeof(MG2_BridgeData::anOriginalScript);
            s_bPatched = Memory::IsReadable(pCode, nSize) && (std::memcmp(pCode, MG2_BridgeData::anOriginalScript, nSize) == 0 || std::memcmp(pCode, MG2_BridgeData::anRestoredScript, nSize) == 0);
            if (s_bPatched)
            {
                std::memcpy(pCode, MG2_BridgeData::anRestoredScript, nSize);
                s_ePhase = BridgePhase::None;
            }
            else if (!s_bWarned)
            {
                spdlog::warn("MG2: Bridge Cutscene : Unexpected script. Keeping MC sequence.");
                s_bWarned = true;
            }
        }
        s_hScriptRun.call<void>(pScript);
    }

    uint8_t* GetPixels(DrawLayer* pLayer)
    {
        return static_cast<uint8_t*>(MG2_Gamefuncs::pfnGetHandleData(pLayer->hPixels));
    }

    void DrawSpans(unsigned int nFirst, unsigned int nCount, int nOffsetX = 0, int nOffsetY = 0)
    {
        auto* pLayer = MG2_Gamefuncs::pfnGetLayerImage(MG2_LAYER_SPRITES);
        auto* pPixels = GetPixels(pLayer);
        for (unsigned int nIndex = 0; nIndex < nCount; ++nIndex)
        {
            const auto& stSpan = s_vSpans[nFirst + nIndex];
            const int nX = stSpan.nX + nOffsetX;
            const int nY = stSpan.nY + nOffsetY;
            const int nLeft = std::max(nX, 0);
            const int nRight = std::min(nX + stSpan.nWidth, nRoomWidth);
            if (nY >= 0 && nY < nRoomHeight && nLeft < nRight)
            {
                const int nColour = stSpan.nColour == nFlashColour ? (MG2_Bridge::bReducePhotosensitiveEffects ? nBlack : nWhite) : stSpan.nColour;
                std::memset(pPixels + nY * pLayer->nWidth + nLeft, nColour, nRight - nLeft);
            }
        }
    }

    void __fastcall DrawSprite_hook(Sprite* pSprite, int nMode)
    {
        if (IsDyingCall() && pSprite->nSheet == MG2_SPRITE_SHEET_PORTRAITS && pSprite->nAnim == MG2_PORTRAIT_ANIMATION_STATIC && (pSprite->nTime == MG2_PORTRAIT_GUSTAVA_EYES_OPEN || pSprite->nTime == MG2_PORTRAIT_GUSTAVA_EYES_CLOSED))
        {
            int nLength = 0;
            const char* pText = MG2_Gamefuncs::pfnAVS_GetLocalizedString(GM_MessageId, GM_MessageSub, &nLength);
            MG2_Bridge::CaptureDyingMessage(pText, nLength);
            const int nTime = pSprite->nTime;
            const int nTrackFrame = pSprite->nFirstTrackFrame;
            pSprite->nTime = MG2_Bridge::GetDyingEyeState() == 1 ? MG2_PORTRAIT_GUSTAVA_EYES_CLOSED : MG2_PORTRAIT_GUSTAVA_EYES_OPEN;
            pSprite->nFirstTrackFrame = pSprite->nTime;
            s_hDrawSprite.call<void>(pSprite, nMode);
            pSprite->nTime = nTime;
            pSprite->nFirstTrackFrame = nTrackFrame;
            return;
        }
        if (s_pRoomSprite && s_bActive)
        {
            if (pSprite == s_pSnakeSprite && (s_ePhase == BridgePhase::SnakeRight || s_ePhase == BridgePhase::SnakeLeave))
            {
                const auto& stFrame = GetFrame();
                const auto* pSnake = MG2_Gamefuncs::pfnGetPlayer();
                DrawSpans(stFrame.nFirstSnakeSpan, stFrame.nSnakeSpanCount, pSnake->nX / nFixedPointOne - nOriginX - stFrame.nSnakeX, pSnake->nY / nFixedPointOne - nOriginY - stFrame.nSnakeY);
                return;
            }
            if (pSprite->nSheet == MG2_SPRITE_SHEET_GUSTAVA || pSprite->nSheet == MG2_SPRITE_SHEET_MADNAR || pSprite->nSheet == MG2_SPRITE_SHEET_BRIDGE_GUARDS)
            {
                return;
            }
            s_hDrawSprite.call<void>(pSprite, nMode);
            if (pSprite == s_pRoomSprite && pSprite->nShown)
            {
                const auto& stFrame = GetFrame();
                DrawSpans(stFrame.nFirstSpan, stFrame.nSpanCount);
            }
            return;
        }
        s_hDrawSprite.call<void>(pSprite, nMode);
    }

    void __fastcall DrawSprites_hook()
    {
        const bool bDraw = s_bActive && s_bRoom && GM_Scene == MG2_SCENE_GAMEPLAY;
        if (bDraw)
        {
            const auto* ahRoomSprites = static_cast<const uintptr_t*>(MG2_Gamefuncs::pfnGetHandleData(pSpriteLists[MG2_SPRITE_LIST_ROOM]));
            s_pRoomSprite = static_cast<Sprite*>(MG2_Gamefuncs::pfnGetHandleData(ahRoomSprites[0]));
            s_pSnakeSprite = MG2_Gamefuncs::pfnGetObjectSprite(MG2_Gamefuncs::pfnGetPlayer());
            if (GetFrame().bFlash && !MG2_Bridge::bReducePhotosensitiveEffects)
            {
                auto* pLayer = MG2_Gamefuncs::pfnGetLayerImage(MG2_LAYER_SPRITES);
                auto* pPixels = GetPixels(pLayer);
                std::replace(pPixels, pPixels + pLayer->nWidth * pLayer->nHeight, static_cast<uint8_t>(nFlashColour), static_cast<uint8_t>(nWhite));
            }
        }
        s_hDrawSprites.call<void>();
        s_pRoomSprite = nullptr;
        s_pSnakeSprite = nullptr;
        if (bDraw && s_bShutter)
        {
            auto* pLayer = MG2_Gamefuncs::pfnGetLayerImage(MG2_LAYER_SPRITES);
            auto* pPixels = GetPixels(pLayer);
            for (int nY = 0; nY < GetFrame().nShutter; ++nY)
            {
                std::memset(pPixels + nY * pLayer->nWidth, nBlack, nRoomWidth);
            }
        }
    }
}

void MG2_Bridge::CaptureDyingMessage(const char* pText, int nLength)
{
    if (!IsDyingCall() || !pText || nLength <= 0)
    {
        s_pDyingMessage = nullptr;
        s_nForcedEyes = -1;
        return;
    }
    if (pText != s_pDyingMessage || nLength != s_nDyingMessageLength)
    {
        s_pDyingMessage = pText;
        s_nDyingMessageLength = nLength;
        s_nEyesOpenPos = -1;
        s_nEyesClosedPos = -1;
        int nLastPageEnd = -1;
        for (int nIndex = 0; nIndex + 1 < nLength; ++nIndex)
        {
            const auto nChar = static_cast<unsigned char>(pText[nIndex]);
            if ((nChar >= 0x81 && nChar <= 0x9F) || nChar >= 0xE0)
            {
                ++nIndex;
            }
            else if (nChar == '\\')
            {
                const char cCommand = pText[++nIndex];
                if (cCommand == 'f' && s_nEyesOpenPos < 0)
                {
                    s_nEyesOpenPos = nLastPageEnd >= 0 ? nLastPageEnd : nIndex + 1;
                }
                else if (cCommand == 'w' || cCommand == 'W')
                {
                    if (s_nEyesOpenPos >= 0)
                    {
                        s_nEyesClosedPos = nIndex + 1;
                        break;
                    }
                    nLastPageEnd = nIndex + 1;
                }
            }
        }
    }
    const int nPos = GM_MessagePos;
    if (s_nEyesClosedPos >= 0 && nPos >= s_nEyesClosedPos)
    {
        s_nForcedEyes = 1;
    }
    else if (s_nEyesOpenPos >= 0 && nPos >= s_nEyesOpenPos)
    {
        s_nForcedEyes = 0;
    }
    else
    {
        s_nForcedEyes = -1;
    }
}

int MG2_Bridge::GetDyingEyeState()
{
    if (!IsDyingCall())
    {
        return -1;
    }
    if (s_nForcedEyes >= 0)
    {
        return s_nForcedEyes;
    }
    // MSX dialogue ticks at 30 Hz; MC ticks at 15 Hz.
    const unsigned int nTick = s_nEyeTick + (static_cast<unsigned int>(GM_FrameCounter) - s_nEyeFrame) * 2;
    const unsigned int nIndex = ((nTick >> 3) + 0xB0) & 0xFF;
    return (MG2_BridgeData::anDyingEyePattern[nIndex >> 2] >> (6 - 2 * (nIndex & 3))) & 3;
}

void MG2_Bridge::HandleLevelTransition()
{
    const bool bRoom = IsRoom(15, 4);
    if (!s_bRoom || !bRoom)
    {
        s_bActive = false;
        s_bPatched = false;
        s_ePhase = BridgePhase::None;
        s_pRoomSprite = nullptr;
        s_pSnakeSprite = nullptr;
    }
    s_bRoom = bRoom;
}

void MG2_Bridge::Initialize()
{
    if (!bEnabled)
    {
        return;
    }
    if (!pScriptGlobals || !pLoadedScripts || !pRunningScriptCode || !pCompanionList || !pSpriteLists || !MG2_Gamefuncs::pfnGetHandleData || !MG2_Gamefuncs::pfnGetPlayer || !MG2_Gamefuncs::pfnGetObjectSprite || !MG2_Gamefuncs::pfnGetEngineState || !MG2_Gamefuncs::pfnScriptRun || !MG2_Gamefuncs::pfnScriptCommand || !MG2_Gamefuncs::pfnDrawSprite || !MG2_Gamefuncs::pfnDrawSprites || !MG2_Gamefuncs::pfnGetLayerImage || !MG2_Gamefuncs::pfnRequestSound || !MG2_Gamefuncs::pfnAVS_GetLocalizedString)
    {
        spdlog::error("MG2: Bridge Cutscene : Missing script or drawing functions. Skipping.");
    }
    else
    {
        s_vSpans.resize(MG2_BridgeData::nSpanCount);
        const auto nBytes = static_cast<int>(s_vSpans.size() * sizeof(s_vSpans[0]));
        if (stbi_zlib_decode_buffer(reinterpret_cast<char*>(s_vSpans.data()), nBytes, reinterpret_cast<const char*>(MG2_BridgeData::anCompressedSpans), sizeof(MG2_BridgeData::anCompressedSpans)) != nBytes)
        {
            spdlog::error("MG2: Bridge Cutscene : Could not load artwork. Skipping.");
            return;
        }
        s_hScriptCommand = safetyhook::create_inline(MG2_Gamefuncs::pfnScriptCommand, ScriptCommand_hook);
        LOG_HOOK(s_hScriptCommand, "MG2: Bridge Cutscene : ScriptCommand()")
        s_hDrawSprite = safetyhook::create_inline(MG2_Gamefuncs::pfnDrawSprite, DrawSprite_hook);
        LOG_HOOK(s_hDrawSprite, "MG2: Bridge Cutscene : DrawSprite()")
        s_hDrawSprites = safetyhook::create_inline(MG2_Gamefuncs::pfnDrawSprites, DrawSprites_hook);
        LOG_HOOK(s_hDrawSprites, "MG2: Bridge Cutscene : DrawSprites()")
        s_hScriptRun = safetyhook::create_inline(MG2_Gamefuncs::pfnScriptRun, ScriptRun_hook);
        LOG_HOOK(s_hScriptRun, "MG2: Bridge Cutscene : ScriptRun()")
        if (!s_hScriptRun || !s_hScriptCommand || !s_hDrawSprites || !s_hDrawSprite)
        {
            s_hScriptRun.reset();
            s_hDrawSprites.reset();
            s_hDrawSprite.reset();
            s_hScriptCommand.reset();
            spdlog::error("MG2: Bridge Cutscene : Incomplete hook setup. Disabled restoration.");
        }
        else
        {
            spdlog::info("MG2: Bridge Cutscene : Initialized.");
        }
    }
}
