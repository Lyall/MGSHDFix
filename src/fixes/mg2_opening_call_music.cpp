#include "stdafx.h"
#include "mg2_opening_call_music.hpp"

#include "common.hpp"
#include "config_keys.hpp"
#include "loose_audio_overrides.hpp"
#include "mg2_linkvarbuf.hpp"

namespace
{
    constexpr size_t nPagesAfterRadarSwitch = 2; // Radar activation, then Campbell's explanation.
    constexpr std::wstring_view svStartMusen = L"mg2_bgm29_startmusen";

    SafetyHookInline s_hDrawMessageText {};
    int s_nSwitchOffset = 0;
    bool s_bTracking = false;
    bool s_bSwitched = false;

    int FindSwitchOffset(const char* pText, int nLength)
    {
        bool bRadarOn = false;
        size_t nPages = 0;
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
            if (cEscape == 'e')
            {
                bRadarOn = true;
            }
            else if (bRadarOn && (cEscape == 'w' || cEscape == 'W') && ++nPages == nPagesAfterRadarSwitch)
            {
                return nIndex + 1;
            }
        }
        return 0;
    }

    void __fastcall DrawMessageText_hook(unsigned int* pArgs, int nMode)
    {
        using namespace MG2_LinkVarBuf;
        // Check for a new call before DrawMessageText overwrites the IDs.
        if (GM_MessageId != MG2_DIALOGUE_BLOCK_CAMPBELL || GM_MessageSub != MG2_CAMPBELL_DIALOGUE_OPENING)
        {
            s_bTracking = false;
        }
        s_hDrawMessageText.call(pArgs, nMode);

        if (GM_MessageId != MG2_DIALOGUE_BLOCK_CAMPBELL || GM_MessageSub != MG2_CAMPBELL_DIALOGUE_OPENING)
        {
            s_bTracking = false;
            return;
        }
        if (!s_bTracking)
        {
            s_bTracking = true;
            s_bSwitched = false;
            int nLength = 0;
            const char* pText = MG2_Gamefuncs::pfnAVS_GetLocalizedString(MG2_DIALOGUE_BLOCK_CAMPBELL, MG2_CAMPBELL_DIALOGUE_OPENING, &nLength);
            s_nSwitchOffset = pText && nLength > 0 ? FindSwitchOffset(pText, nLength) : 0;
        }
        if (s_bSwitched || s_nSwitchOffset <= 0 || GM_MessagePos < s_nSwitchOffset)
        {
            return;
        }
        if (LooseAudioOverrides::EndVariant(svStartMusen))
        {
            s_bSwitched = true;
            if (LooseAudioOverrides::sDebugLogging != ConfigKeys::LooseAudioDebugLogging_Disabled)
            {
                spdlog::info("MG2: Opening Call Music : Radar explained, switched to the full arrangement.");
            }
        }
    }
}

void MG2_OpeningCallMusic::Initialize()
{
    if (!LooseAudioOverrides::HasVariant(svStartMusen))
    {
        return;
    }
    if (!MG2_Gamefuncs::pfnDrawMessageText || !MG2_Gamefuncs::pfnAVS_GetLocalizedString || !MG2_LinkVarBuf::pScriptGlobals)
    {
        spdlog::error("MG2: Opening Call Music : Missing DrawMessageText, string lookup or script globals. Skipping.");
        return;
    }
    s_hDrawMessageText = safetyhook::create_inline(MG2_Gamefuncs::pfnDrawMessageText, DrawMessageText_hook);
    LOG_HOOK(s_hDrawMessageText, "MG2: Opening Call Music : DrawMessageText()")
}
