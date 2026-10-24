#include "stdafx.h"
#include "mg2_ending_cast.hpp"

#include "common.hpp"
#include "game_funcs.hpp"
#include "mg_text_overrides.hpp"
#include "mg2_gamevar_enums.hpp"
#include "mg2_ending_cast_data.hpp"

using namespace MG2_Gamefuncs;

namespace
{
    SafetyHookInline s_hGetChunkEntryData;
    bool s_bWarned = false;

    uint8_t* __fastcall GetChunkEntryData_hook(uintptr_t hChunk, int nEntry)
    {
        auto* pData = s_hGetChunkEntryData.call<uint8_t*>(hChunk, nEntry);
        if (nEntry != MG2_SPRITE_SHEET_ENDING_CAST || !pData || pfnGetChunkEntrySize(hChunk, nEntry) != MG2_EndingCastData::nSize)
        {
            return pData;
        }
        const auto& stFirst = MG2_EndingCastData::astChanges[0];
        if (pData[stFirst.nOffset] != stFirst.nOriginal)
        {
            return pData;
        }
        uint64_t nHash = 0xCBF29CE484222325ull;
        for (size_t nIndex = 0; nIndex < MG2_EndingCastData::nSize; ++nIndex)
        {
            nHash = (nHash ^ pData[nIndex]) * 0x100000001B3ull;
        }
        if (nHash != MG2_EndingCastData::nOriginalHash)
        {
            if (!s_bWarned)
            {
                spdlog::warn("MG2: Ending Cast : Unexpected animation data. Skipping restoration.");
                s_bWarned = true;
            }
            return pData;
        }
        for (const auto& stChange : MG2_EndingCastData::astChanges)
        {
            pData[stChange.nOffset] = stChange.nRestored;
        }
        return pData;
    }
}

void MG2_EndingCast::Initialize()
{
    if (!MG_TextOverrides::bMG2RestoreOriginalScript)
    {
        return;
    }
    if (!pfnGetChunkEntryData || !pfnGetChunkEntrySize)
    {
        spdlog::error("MG2: Ending Cast : Required resource functions unavailable.");
    }
    else
    {
        s_hGetChunkEntryData = safetyhook::create_inline(pfnGetChunkEntryData, GetChunkEntryData_hook);
        LOG_HOOK(s_hGetChunkEntryData, "MG2: Ending Cast : GetChunkEntryData()")
        if (s_hGetChunkEntryData)
        {
            spdlog::info("MG2: Ending Cast : Initialized.");
        }
    }
}
