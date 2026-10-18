#include "stdafx.h"
#include "mg1_portrait_overlay.hpp"
#include "mg1_portrait_sprites.hpp"

#include "common.hpp"
#include "mg1_linkvarbuf.hpp"

using namespace MG1_LinkVarBuf;

namespace
{
    constexpr int nPortraitX = 208;
    constexpr int nPortraitY = 48;
    constexpr int anTalkingFrames[] = { 1, 2, 0, 2, 0, 2, 0, 2 };

    SafetyHookMid s_hShowMessage;
    SafetyHookMid s_hPrintCharacter;
    SafetyHookMid s_hDrawPortrait;
    bool s_bInitialized = false;
    bool s_bSnakeSpeaking = false;
    size_t s_nPhase = 0;
    int s_nFrame = 0;

    void BeginMessage(SafetyHookContext& ctx)
    {
        const int nChunk = static_cast<int>(ctx.rcx);
        const int nRecord = static_cast<int>(ctx.rdx);
        s_bSnakeSpeaking = (nChunk == MG1_DIALOGUE_RADIO && nRecord == MG1_SNAKE_RADIO_SEND) || (nChunk == MG1_DIALOGUE_ENDING && nRecord == MG1_SNAKE_FINAL_REPORT);
        s_nPhase = 0;
        s_nFrame = 0;
    }

    void PrintCharacter(SafetyHookContext&)
    {
        if (!s_bInitialized || !s_bSnakeSpeaking)
        {
            return;
        }

        // One phase per character: MSX prints every four 60 Hz ticks; MC prints at 15 Hz.
        s_nFrame = anTalkingFrames[s_nPhase];
        s_nPhase = (s_nPhase + 1) % std::size(anTalkingFrames);
    }

    void DrawPortrait(SafetyHookContext&)
    {
        if (!s_bInitialized)
        {
            return;
        }

        if (!s_bSnakeSpeaking || !GM_MessageActive || GM_MessageDone || GM_MessageInstant || GM_MessagePos >= *pMessageLength)
        {
            s_nFrame = 0;
        }
        const uint8_t* pPixels = MG1_PortraitSprites::aanFrames[s_nFrame];
        const DrawLayer stImage = { reinterpret_cast<uintptr_t>(&pPixels), MG1_PortraitSprites::nWidth, MG1_PortraitSprites::nHeight };
        const int anSourceRect[] = { 0, 0, MG1_PortraitSprites::nWidth, MG1_PortraitSprites::nHeight };
        MG1_Gamefuncs::pfnBlitImage(GM_CurrentDrawLayer, &stImage, anSourceRect, nPortraitX, nPortraitY);
    }
}

void MG1_PortraitOverlay::Initialize()
{
    if (!bEnabled)
    {
        return;
    }

    //MessageNextLine+187
    uint8_t* pPrintCharacter = Memory::PatternScan(mg1Module, "83 3D ?? ?? ?? ?? 00 8B 05", "MG1: Snake Portrait : MessageNextLine()+0x187 : printed character");
    //DrawTransceiver+165
    uint8_t* pDrawPortrait = Memory::PatternScan(mg1Module, "B8 ?? ?? ?? ?? F7 2D ?? ?? ?? ?? 8B CA C1 F9 ?? 8B D1", "MG1: Snake Portrait : DrawTransceiver()+0x165 : after Snake portrait");
    if (!pPrintCharacter || !pDrawPortrait || !pScriptGlobals || !pMessageLength || !MG1_Gamefuncs::pfnShowMessage || !MG1_Gamefuncs::pfnBlitImage)
    {
        spdlog::error("MG1: Snake Portrait : Missing message or drawing addresses. Skipping.");
    }
    else
    {
        s_hShowMessage = safetyhook::create_mid(reinterpret_cast<void*>(MG1_Gamefuncs::pfnShowMessage), BeginMessage);
        LOG_HOOK(s_hShowMessage, "MG1: Snake Portrait : ShowMessage()")
        s_hPrintCharacter = safetyhook::create_mid(pPrintCharacter, PrintCharacter);
        LOG_HOOK(s_hPrintCharacter, "MG1: Snake Portrait : MessageNextLine()+0x187")
        s_hDrawPortrait = safetyhook::create_mid(pDrawPortrait, DrawPortrait);
        LOG_HOOK(s_hDrawPortrait, "MG1: Snake Portrait : DrawTransceiver()+0x165")
        if (!s_hShowMessage || !s_hPrintCharacter || !s_hDrawPortrait)
        {
            s_hDrawPortrait.reset();
            s_hPrintCharacter.reset();
            s_hShowMessage.reset();
            spdlog::error("MG1: Snake Portrait : Incomplete hook setup. Disabled portrait restoration.");
        }
        else
        {
            s_bInitialized = true;
            spdlog::info("MG1: Snake Portrait : Initialized.");
        }
    }
}
