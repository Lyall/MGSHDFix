#include "stdafx.h"
#include "common.hpp"
#include "logging.hpp"
#include "game_funcs.hpp"
#include "mg1_linkvarbuf.hpp"
#include "mg1_text_strings.hpp"
#include "mg2_text_strings.hpp"
#include "mg2_ending_cast.hpp"
#include "mg2_boss_name_sprites.hpp"
#include "mg2_sprite_patcher.hpp"
#include "mg_text_encoding.hpp"
#include "mg_text_overrides.hpp"
#include "translation_diffs.hpp"

namespace
{
    using TextKey = std::pair<uint32_t, int32_t>;
    using TextTable = std::map<TextKey, std::string>;
    std::map<int, TextTable> s_mMG1Text;
    std::map<int, TextTable> s_mMG2Text;
    SafetyHookInline s_hMG1ChunkData;
    SafetyHookInline s_hMG1ChunkSize;
    SafetyHookInline s_hMG2Text;

    struct TextChunk
    {
        const uint8_t* pSource = nullptr;
        int nLanguage = 0;
        std::vector<uint8_t> vOriginal;
        std::vector<uint8_t> vReplacement;
    };
    std::deque<TextChunk> s_vMG1Chunks;
    TextChunk* s_pCurrentChunk = nullptr;

    int Language(const char* pszCode)
    {
        constexpr std::pair<std::string_view, MGLanguage> astLanguages[] =
        {
            { "en", MGLanguage::English }, { "fr", MGLanguage::French },
            { "gr", MGLanguage::German }, { "it", MGLanguage::Italian },
            { "sp", MGLanguage::Spanish }, { "jp", MGLanguage::Japanese },
        };
        for (const auto& [svCode, eLanguage] : astLanguages)
        {
            if (pszCode && svCode == pszCode)
            {
                return static_cast<int>(eLanguage);
            }
        }
        return 0;
    }

    bool Encode(const char* pszText, int nLanguage, bool bMG2, std::string& sEncoded)
    {
        if (!pszText)
        {
            return false;
        }
        const int nLength = static_cast<int>(strlen(pszText));
        if (nLength == 0)
        {
            return true;
        }
        const int nWideLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pszText, nLength, nullptr, 0);
        if (nWideLength == 0)
        {
            return false;
        }
        std::wstring sWide(nWideLength, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pszText, nLength, sWide.data(), nWideLength);
        if (nLanguage == static_cast<int>(MGLanguage::Japanese))
        {
            for (size_t nIndex = 0; nIndex < sWide.size(); ++nIndex)
            {
                if (bMG2)
                {
                    const auto itCharacter = std::ranges::find(astMG2JapaneseCharacters, sWide[nIndex], &MGTextCharacter::nUnicode);
                    if (itCharacter != std::end(astMG2JapaneseCharacters))
                    {
                        sEncoded.push_back(static_cast<char>(itCharacter->nByte));
                        continue;
                    }
                }
                const int nCharacters = sWide[nIndex] >= 0xD800 && sWide[nIndex] <= 0xDBFF && nIndex + 1 < sWide.size() ? 2 : 1;
                char achEncoded[4] {};
                BOOL bDefault = FALSE;
                const int nBytes = WideCharToMultiByte(932, WC_NO_BEST_FIT_CHARS, sWide.data() + nIndex, nCharacters, achEncoded, sizeof(achEncoded), nullptr, &bDefault);
                if (nBytes <= 0 || bDefault)
                {
                    return false;
                }
                sEncoded.append(achEncoded, nBytes);
                nIndex += nCharacters - 1;
            }
            return true;
        }
        const std::span<const MGTextCharacter> stCharacters = bMG2 ? std::span<const MGTextCharacter>(astMG2WesternCharacters) : std::span<const MGTextCharacter>(astMG1WesternCharacters);
        for (wchar_t nCharacter : sWide)
        {
            if (nCharacter < 0x80)
            {
                sEncoded.push_back(static_cast<char>(nCharacter));
                continue;
            }
            const auto itCharacter = std::ranges::find(stCharacters, nCharacter, &MGTextCharacter::nUnicode);
            if (itCharacter == stCharacters.end())
            {
                return false;
            }
            sEncoded.push_back(static_cast<char>(itCharacter->nByte));
        }
        return true;
    }

    void Load(std::span<const MGTextOverride> vEntries, bool bMG2, std::map<int, TextTable>& mTables)
    {
        for (const MGTextOverride& stEntry : vEntries)
        {
            const int nLanguage = Language(stEntry.pszLanguage);
            std::string sText;
            if (!nLanguage || stEntry.nIndex < 0 || !Encode(stEntry.pszReplacementUtf8, nLanguage, bMG2, sText) || sText.size() > (bMG2 ? 32767u : 2047u))
            {
                spdlog::error("MG{}: Text Overrides : Invalid replacement for block {}, index {}.", bMG2 ? 2 : 1, stEntry.nBlock, stEntry.nIndex);
                continue;
            }
            mTables[nLanguage][{ stEntry.nBlock, stEntry.nIndex }] = std::move(sText);
        }
    }

    uint32_t ReadSize(const uint8_t* pData)
    {
        return (uint32_t(pData[0]) << 24) | (uint32_t(pData[1]) << 16) | (uint32_t(pData[2]) << 8) | pData[3];
    }

    bool RebuildChunk(std::span<const uint8_t> stSource, const TextTable& mText, std::vector<uint8_t>& vOutput)
    {
        size_t nOffset = 0;
        uint32_t nBlock = 0;
        while (nOffset < stSource.size())
        {
            if (stSource.size() - nOffset < 4)
            {
                return false;
            }
            const uint32_t nHeader = ReadSize(stSource.data() + nOffset);
            const size_t nLength = nHeader & 0x3FFFFFFF;
            if (nLength > stSource.size() - nOffset - 4)
            {
                return false;
            }
            const size_t nStart = nOffset + 4;
            const size_t nEnd = nStart + nLength;
            const auto itBlock = mText.lower_bound({ nBlock, 0 });
            if (itBlock == mText.end() || itBlock->first.first != nBlock)
            {
                vOutput.insert(vOutput.end(), stSource.begin() + nOffset, stSource.begin() + nEnd);
            }
            else
            {
                const size_t nHeaderOffset = vOutput.size();
                vOutput.resize(nHeaderOffset + 4);
                int32_t nIndex = 0;
                size_t nRecord = nStart;
                while (nRecord < nEnd)
                {
                    if (nEnd - nRecord < 3)
                    {
                        return false;
                    }
                    const size_t nTextLength = (size_t(stSource[nRecord + 1]) << 8) | stSource[nRecord + 2];
                    if (nTextLength > nEnd - nRecord - 3)
                    {
                        return false;
                    }
                    const auto itText = mText.find({ nBlock, nIndex });
                    if (itText == mText.end())
                    {
                        vOutput.insert(vOutput.end(), stSource.begin() + nRecord, stSource.begin() + nRecord + 3 + nTextLength);
                    }
                    else
                    {
                        const std::string& sText = itText->second;
                        vOutput.push_back(stSource[nRecord]);
                        vOutput.push_back(static_cast<uint8_t>(sText.size() >> 8));
                        vOutput.push_back(static_cast<uint8_t>(sText.size()));
                        vOutput.insert(vOutput.end(), sText.begin(), sText.end());
                    }
                    nRecord += 3 + nTextLength;
                    ++nIndex;
                }
                const uint32_t nNewHeader = (nHeader & 0xC0000000) | static_cast<uint32_t>(vOutput.size() - nHeaderOffset - 4);
                for (int nByte = 0; nByte < 4; ++nByte)
                {
                    vOutput[nHeaderOffset + nByte] = static_cast<uint8_t>(nNewHeader >> (24 - 8 * nByte));
                }
            }
            nOffset = nEnd;
            ++nBlock;
        }
        return true;
    }

    TextChunk* GetTextChunk(const uint8_t* pSource, uint32_t nSize)
    {
        const int nLanguage = Shared_Gamefuncs::pfnGetLanguage();
        const auto itTable = s_mMG1Text.find(nLanguage);
        if (itTable == s_mMG1Text.end() || !pSource || nSize > 16 * 1024 * 1024)
        {
            return nullptr;
        }
        if (s_pCurrentChunk && s_pCurrentChunk->pSource == pSource && s_pCurrentChunk->nLanguage == nLanguage && s_pCurrentChunk->vOriginal.size() == nSize && memcmp(pSource, s_pCurrentChunk->vOriginal.data(), nSize) == 0)
        {
            return s_pCurrentChunk;
        }
        TextChunk stChunk;
        stChunk.pSource = pSource;
        stChunk.nLanguage = nLanguage;
        stChunk.vOriginal.assign(pSource, pSource + nSize);
        if (!RebuildChunk(stChunk.vOriginal, itTable->second, stChunk.vReplacement))
        {
            spdlog::error("MG1: Text Overrides : Invalid text chunk.");
            return nullptr;
        }
        s_vMG1Chunks.push_back(std::move(stChunk)); // Earlier text pointers may still be in use.
        s_pCurrentChunk = &s_vMG1Chunks.back();
        return s_pCurrentChunk;
    }

    const uint8_t* __fastcall GetChunkData(uintptr_t hChunk, int nEntry)
    {
        const uint8_t* pData = s_hMG1ChunkData.call<const uint8_t*>(hChunk, nEntry);
        if (nEntry != 0 || hChunk != *MG1_LinkVarBuf::pTextChunkHandle)
        {
            return pData;
        }
        const uint32_t nSize = s_hMG1ChunkSize.call<uint32_t>(hChunk, nEntry);
        TextChunk* pChunk = GetTextChunk(pData, nSize);
        return pChunk ? pChunk->vReplacement.data() : pData;
    }

    uint32_t __fastcall GetChunkSize(uintptr_t hChunk, int nEntry)
    {
        const uint32_t nSize = s_hMG1ChunkSize.call<uint32_t>(hChunk, nEntry);
        if (nEntry != 0 || hChunk != *MG1_LinkVarBuf::pTextChunkHandle)
        {
            return nSize;
        }
        TextChunk* pChunk = GetTextChunk(s_hMG1ChunkData.call<const uint8_t*>(hChunk, nEntry), nSize);
        return pChunk ? static_cast<uint32_t>(pChunk->vReplacement.size()) : nSize;
    }

    const char* __fastcall GetLocalizedString(uint32_t nBlock, int32_t nIndex, int* pLength)
    {
        const auto itLanguage = s_mMG2Text.find(Shared_Gamefuncs::pfnGetLanguage());
        if (itLanguage != s_mMG2Text.end())
        {
            const auto itText = itLanguage->second.find({ nBlock, nIndex });
            if (itText != itLanguage->second.end())
            {
                if (pLength)
                {
                    *pLength = static_cast<int>(itText->second.size());
                }
                return itText->second.c_str();
            }
        }
        return s_hMG2Text.call<const char*>(nBlock, nIndex, pLength);
    }
}

void MG_TextOverrides::InitializeMG1()
{
    Load(kMG1TypoFixes, false, s_mMG1Text);
    if (bMG1RestoreBossNames)
    {
        Load(kMG1RestoredBossNames, false, s_mMG1Text);
    }
    //if (bMG1RestoreOriginalScript)
    //{
    //    Load(kMG1OriginalScript, false, s_mMG1Text);
    //}
    for (const auto& stEntry : TranslationDiffs::Load("MG1"))
    {
        const MGTextOverride stOverride { stEntry.nBlock, stEntry.nIndex, stEntry.sLanguage.c_str(), stEntry.sText.c_str() };
        Load(std::span<const MGTextOverride>(&stOverride, 1), false, s_mMG1Text);
    }
    if (s_mMG1Text.empty())
    {
        spdlog::info("MG1: Text Overrides : No replacements configured.");
        return;
    }
    if (!MG1_Gamefuncs::pfnGetChunkEntryData || !MG1_Gamefuncs::pfnGetChunkEntrySize || !MG1_LinkVarBuf::pTextChunkHandle || !Shared_Gamefuncs::pfnGetLanguage)
    {
        spdlog::error("MG1: Text Overrides : Required functions or text chunk unavailable.");
        return;
    }
    s_hMG1ChunkData = safetyhook::create_inline(MG1_Gamefuncs::pfnGetChunkEntryData, GetChunkData);
    s_hMG1ChunkSize = safetyhook::create_inline(MG1_Gamefuncs::pfnGetChunkEntrySize, GetChunkSize);
    LOG_HOOK(s_hMG1ChunkData, "MG1: Text Overrides : GetChunkEntryData()")
    LOG_HOOK(s_hMG1ChunkSize, "MG1: Text Overrides : GetChunkEntrySize()")
    if (!s_hMG1ChunkData || !s_hMG1ChunkSize)
    {
        s_hMG1ChunkData.reset();
        s_hMG1ChunkSize.reset();
        return;
    }
    spdlog::info("MG1: Text Overrides : Initialized.");
}

void MG_TextOverrides::InitializeMG2()
{
    MG2_EndingCast::Initialize();
    Load(kMG2TypoFixes, true, s_mMG2Text);
    if (bMG2RestoreBossNames)
    {
        Load(kMG2RestoredBossNames, true, s_mMG2Text);
        bool bRegistered = true;
        for (const auto& stPatch : MG2_BossNameSprites::astPatches)
        {
            bRegistered &= MG2_SpritePatcher::AddPatch(70, stPatch.nOriginalHash, stPatch.aBlob.data(), stPatch.aBlob.size());
        }
        if (bRegistered)
        {
            spdlog::info("MG2: Boss Names : Sprite replacements registered.");
        }
    }
    if (bMG2RestoreOriginalScript)
    {
        Load(kMG2OriginalScript, true, s_mMG2Text);
    }
    for (const auto& stEntry : TranslationDiffs::Load("MG2"))
    {
        const MGTextOverride stOverride { stEntry.nBlock, stEntry.nIndex, stEntry.sLanguage.c_str(), stEntry.sText.c_str() };
        Load(std::span<const MGTextOverride>(&stOverride, 1), true, s_mMG2Text);
    }
    if (s_mMG2Text.empty())
    {
        spdlog::info("MG2: Text Overrides : No replacements configured.");
        return;
    }
    if (!MG2_Gamefuncs::pfnAVS_GetLocalizedString || !Shared_Gamefuncs::pfnGetLanguage)
    {
        spdlog::error("MG2: Text Overrides : Required functions unavailable.");
        return;
    }
    s_hMG2Text = safetyhook::create_inline(MG2_Gamefuncs::pfnAVS_GetLocalizedString, GetLocalizedString);
    LOG_HOOK(s_hMG2Text, "MG2: Text Overrides : AVS_GetLocalizedString()")
    if (s_hMG2Text)
    {
        spdlog::info("MG2: Text Overrides : Initialized.");
    }
}
