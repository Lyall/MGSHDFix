#include "stdafx.h"
#include "mg2_call_sequence.hpp"
#include "mg2_portrait_overlay.hpp"

#include "common.hpp"
#include "mg2_linkvarbuf.hpp"

// Fixes outgoing transciever calls showing the right side portrait before Snake's initial outbound message.
namespace
{
    constexpr size_t nMessageLengthOnStack = 0x40;

    SafetyHookInline s_hScriptCommand {};
    SafetyHookMid s_hMessageLength {};

    struct OpeningMessage
    {
        int nBlock = 0;
        int nIndex = 0;
        int nEnd = 0;
        int* pScript = nullptr;
        int nInstruction = 0;
        bool bFinished = false;
    };

    OpeningMessage s_stOpening {};
    bool s_bDrawingOpening = false;

    int FindOpeningEnd(const char* pText, int nLength)
    {
        bool bSnake = false;
        int nPageStart = 0;
        for (int nIndex = 0; nIndex < nLength; nIndex++)
        {
            const auto nChar = static_cast<unsigned char>(pText[nIndex]);
            if ((nChar >= 0x81 && nChar <= 0x9F) || nChar >= 0xE0)
            {
                nIndex++;
                continue;
            }
            if (nChar != '\\' || nIndex + 1 >= nLength)
            {
                continue;
            }

            const char cEscape = pText[++nIndex];
            if (cEscape == 's')
            {
                bSnake = true;
            }
            else if (cEscape == 'r')
            {
                return bSnake ? nPageStart : 0;
            }
            else if (cEscape == 'w' || cEscape == 'W')
            {
                nPageStart = nIndex + 1;
            }
        }
        return 0;
    }

    bool IsRadioSprite(const int* pArgs, int nCount, int nSlot, int nAnimation, int nFrame)
    {
        return nCount == 7 && pArgs[0] == MG2_SPRITE_LIST_TRANSCEIVER && pArgs[1] == nSlot && pArgs[2] == MG2_SPRITE_SHEET_TRANSCEIVER && pArgs[3] == nAnimation && pArgs[4] == nFrame;
    }

    void PrepareOpening()
    {
        s_stOpening = {};
        using namespace MG2_LinkVarBuf;
        if (GM_Scene != MG2_SCENE_TRANSCEIVER || !pScriptGlobals || GM_ScriptedCall == 0 || GM_ContactSlot < MG2_CONTACT_SLOT_FIRST || GM_ContactSlot > MG2_CONTACT_SLOT_LAST || GM_ReplyIndex < 0)
        {
            return;
        }

        int nLength = 0;
        const int nBlock = MG2_DIALOGUE_BLOCK_CONTACT_BASE + GM_ContactSlot;
        const char* pText = MG2_Gamefuncs::pfnAVS_GetLocalizedString(nBlock, GM_ReplyIndex, &nLength);
        if (!pText || nLength <= 0)
        {
            return;
        }
        s_stOpening.nBlock = nBlock;
        s_stOpening.nIndex = GM_ReplyIndex;
        s_stOpening.nEnd = FindOpeningEnd(pText, nLength);
    }

    int __fastcall ScriptCommand_hook(int* pScript, uint8_t nCommand, int* pArgs, char* pStrings, int nCount)
    {
        using namespace MG2_LinkVarBuf;
        if (GM_Scene != MG2_SCENE_TRANSCEIVER || nCommand == MG2_COMMAND_SCENE)
        {
            s_stOpening = {};
        }

        if (nCommand == MG2_COMMAND_SPRITE_SET && IsRadioSprite(pArgs, nCount, MG2_TRANSCEIVER_SLOT_BACKGROUND, MG2_TRANSCEIVER_ANIMATION_STATIC, MG2_TRANSCEIVER_FRAME_BACKGROUND))
        {
            PrepareOpening();
        }
        if (s_stOpening.nEnd > 0 && nCommand == MG2_COMMAND_SPRITE_SET)
        {
            if (!s_stOpening.pScript && IsRadioSprite(pArgs, nCount, MG2_TRANSCEIVER_SLOT_STATUS, MG2_TRANSCEIVER_ANIMATION_STATIC, MG2_TRANSCEIVER_FRAME_RECEIVE))
            {
                pArgs[4] = MG2_TRANSCEIVER_FRAME_SEND;
            }

            if (IsRadioSprite(pArgs, nCount, MG2_TRANSCEIVER_SLOT_CONNECTION, MG2_TRANSCEIVER_ANIMATION_CONNECT, 0))
            {
                if (!s_stOpening.pScript)
                {
                    s_stOpening.pScript = pScript;
                    s_stOpening.nInstruction = pScript[MG2_SCRIPT_INSTRUCTION_PROG_COUNT];
                    GM_MessageDone = 0;
                }
                if (s_stOpening.pScript == pScript && s_stOpening.nInstruction == pScript[MG2_SCRIPT_INSTRUCTION_PROG_COUNT])
                {
                    if (!s_stOpening.bFinished)
                    {
                        int anMessage[] = { s_stOpening.nBlock, s_stOpening.nIndex };
                        s_bDrawingOpening = true;
                        s_hScriptCommand.call<int>(pScript, MG2_COMMAND_MESSAGE_TEXT, anMessage, pStrings, 2);
                        s_bDrawingOpening = false;
                        s_stOpening.bFinished = GM_MessageDone != 0;
                        pScript[MG2_SCRIPT_PROG_COUNT] = pScript[MG2_SCRIPT_INSTRUCTION_PROG_COUNT];
                        return 1;
                    }


                    s_stOpening = {};
                    GM_SpeakerIsSnake = 0;
                    int anReceive[] = { MG2_SPRITE_LIST_TRANSCEIVER, MG2_TRANSCEIVER_SLOT_STATUS, MG2_SPRITE_SHEET_TRANSCEIVER, MG2_TRANSCEIVER_ANIMATION_STATIC, MG2_TRANSCEIVER_FRAME_RECEIVE, 0, 2 };
                    s_hScriptCommand.call<int>(pScript, MG2_COMMAND_SPRITE_SET, anReceive, pStrings, 7);
                }
            }
        }
        if (GM_Scene == MG2_SCENE_TRANSCEIVER && nCommand == MG2_COMMAND_SPRITE_SET && IsRadioSprite(pArgs, nCount, MG2_TRANSCEIVER_SLOT_CONNECTION, MG2_TRANSCEIVER_ANIMATION_CONNECT, 0))
        {
            int anContact[] = { MG2_SPRITE_LIST_TRANSCEIVER, MG2_TRANSCEIVER_SLOT_CONTACT, MG2_SPRITE_SHEET_PORTRAITS, MG2_PORTRAIT_ANIMATION_STATIC, GM_ContactPortraitFrame, 0, 1 };
            s_hScriptCommand.call<int>(pScript, MG2_COMMAND_SPRITE_SET, anContact, pStrings, 7);
        }


        if (GM_Scene == MG2_SCENE_TRANSCEIVER && nCommand == MG2_COMMAND_MESSAGE_TEXT && nCount == 2 && pArgs[0] == MG2_DIALOGUE_BLOCK_CONTACT_BASE + GM_ContactSlot && pArgs[1] == GM_ReplyIndex && MG2_PortraitOverlay::IsContactRevealPending())
        {
            pScript[MG2_SCRIPT_PROG_COUNT] = pScript[MG2_SCRIPT_INSTRUCTION_PROG_COUNT];
            return 1;
        }

        return s_hScriptCommand.call<int>(pScript, nCommand, pArgs, pStrings, nCount);
    }
}

void MG2_CallSequence::Initialize()
{
    if (!bEnabled)
    {
        return;
    }

    //DrawMessageText+DE
    uint8_t* pMessageLength = Memory::PatternScan(mg2Module, "83 05 ?? ?? ?? ?? ?? 4C 8D 2D", "MG2: Call Sequence : DrawMessageText()+0xDE : opening page boundary");
    if (!MG2_Gamefuncs::pfnScriptCommand || !MG2_Gamefuncs::pfnAVS_GetLocalizedString || !pMessageLength || !MG2_LinkVarBuf::pScriptGlobals)
    {
        spdlog::error("MG2: Call Sequence : Missing command, text or globals. Skipping call sequence hooks.");
    }
    else
    {
        s_hMessageLength = safetyhook::create_mid(pMessageLength, [](SafetyHookContext& stContext)
        {
            if (s_bDrawingOpening)
            {

                int& nLength = *reinterpret_cast<int*>(stContext.rsp + nMessageLengthOnStack);
                if (s_stOpening.nEnd > 0 && s_stOpening.nEnd < nLength)
                {
                    nLength = s_stOpening.nEnd;
                }
            }
        });
        LOG_HOOK(s_hMessageLength, "MG2: Call Sequence : DrawMessageText()+0xDE : opening page boundary")
        if (s_hMessageLength)
        {
            s_hScriptCommand = safetyhook::create_inline(MG2_Gamefuncs::pfnScriptCommand, ScriptCommand_hook);
            LOG_HOOK(s_hScriptCommand, "MG2: Call Sequence : ScriptCommand()")
        }
        if (!s_hMessageLength || !s_hScriptCommand)
        {
            s_hScriptCommand.reset();
            s_hMessageLength.reset();
            spdlog::error("MG2: Call Sequence : Incomplete hook setup. Disabled call sequence hooks.");
        }
        else
        {
            spdlog::info("MG2: Call Sequence : Initialized.");
        }
    }
}
