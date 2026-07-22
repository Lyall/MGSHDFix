// ReSharper disable CppClangTidyClangDiagnosticInvalidUtf8
// ReSharper disable CommentTypo
#include "stdafx.h"

#include "common.hpp"
#include "logging.hpp"

#include "caption_replacements.hpp"

#include "game_strings.hpp"

#include "input_handler.hpp"

//#define _DUMP_INPUT_STRINGS
namespace
{

    enum class CaptionAction : uint8_t
    {
        Correction,
        ForceOriginal,
    };

    struct CaptionEntry
    {
        CaptionAction action;
        const char* replacement; // valid only when action == Correction
    };

    using CaptionEntryMap = std::unordered_map<uint32_t, CaptionEntry>;
    using HashSet = std::unordered_set<uint32_t>;

    struct CaptionOverrideTables
    {
        CaptionEntryMap entries;
        HashSet forcePs2StringList;
    };

    CaptionOverrideTables g_MGS2CaptionOverrides;
    CaptionOverrideTables g_MGS3CaptionOverrides;
    SafetyHookInline BP_GetOverrideString_hook {};

    template <size_t N>
    void LoadForceOriginal(CaptionEntryMap& destination, const uint32_t(&hashes)[N])
    {
        for (uint32_t const hash : hashes)
        {
            if (!destination.try_emplace(hash, CaptionEntry { CaptionAction::ForceOriginal, nullptr }).second)
            {
                spdlog::warn("Duplicate hash in force-original list: {:#010x}", hash);
            }
        }
    }

    template <size_t N>
    void LoadBuiltInCorrections(CaptionEntryMap& destination, const CaptionOverride(&entries)[N])
    {
        for (CaptionOverride const& entry : entries)
        {
            if (!destination.try_emplace(entry.ps2_crc32, CaptionEntry { CaptionAction::Correction, entry.replacement }).second)
            {
                spdlog::warn("Duplicate correction hash (already present as force-original or duplicate entry): {:#010x}", entry.ps2_crc32);
            }
        }
    }

    template <size_t N>
    void LoadHashSet(HashSet& destination, const uint32_t(&hashes)[N])
    {
        destination.max_load_factor(0.8f);
        destination.reserve(N);
        for (uint32_t const hash : hashes)
        {
            if (!destination.emplace(hash).second)
            {
                spdlog::warn("Duplicate hash in caption override list: {:#010x}", hash);
            }
        }
    }

    template <size_t N>
    void LoadHashSet(HashSet& destination, const std::array<uint32_t, N>& hashes)
    {
        if constexpr (N > 0)
        {
            destination.max_load_factor(0.8f);
            destination.reserve(N);
            for (uint32_t const hash : hashes)
            {
                if (!destination.emplace(hash).second)
                {
                    spdlog::warn("Duplicate hash in caption override list: {:#010x}", hash);
                }
            }
        }
    }

    void InitializeCaptionOverrides()
    {
        CaptionOverrideTables& tables = (eGameType & MGS2) ? g_MGS2CaptionOverrides : g_MGS3CaptionOverrides;

        tables.entries.max_load_factor(0.8f);

        if (eGameType & MGS2)
        {
            tables.entries.reserve(std::size(kMGS2ForceOriginalHashes) + std::size(kMGS2CaptionTypoFixes));
            LoadForceOriginal(tables.entries, kMGS2ForceOriginalHashes);
            LoadBuiltInCorrections(tables.entries, kMGS2CaptionTypoFixes);
            LoadHashSet(tables.forcePs2StringList, kMGS2ForcePs2StringHashes);
        }
        else
        {
            tables.entries.reserve(std::size(kMGS3CaptionTypoFixes));
            //LoadForceOriginal(tables.entries, kMGS3ForceOriginalHashes);
            LoadBuiltInCorrections(tables.entries, kMGS3CaptionTypoFixes);
            LoadHashSet(tables.forcePs2StringList, kMGS3ForcePs2StringHashes);
        }
    }

    char* __fastcall BP_GetOverrideString_hk(char* inputString)
    {
        if (!inputString)
        {
            return BP_GetOverrideString_hook.fastcall<char*>(inputString);
        }

        uint32_t const hash = Util::StringToCRC32(inputString);
        const CaptionOverrideTables& tables = (eGameType & MGS2) ? g_MGS2CaptionOverrides : g_MGS3CaptionOverrides;

        if (CaptionReplacements::bForcePS2 && tables.forcePs2StringList.contains(hash))
        {
            return inputString;
        }

        if (auto const it = tables.entries.find(hash); it != tables.entries.end())
        {
            return it->second.action == CaptionAction::ForceOriginal ? inputString : const_cast<char*>(it->second.replacement);
        }

#ifdef _DUMP_INPUT_STRINGS
        spdlog::info("Input Hash:{:#010x}\n{}", hash, inputString);
#endif

        return BP_GetOverrideString_hook.fastcall<char*>(inputString);
    }

#ifdef _DUMP_OVERRIDE_STRINGS

    void DumpStringOverrides(uint8_t* BP_GetOverrideStringScanResult)
    {
        uint8_t* const callInsn = BP_GetOverrideStringScanResult + 0x29;
        uint8_t* const lookupFuncAddr = Memory::ResolveCall(callInsn);

        struct STableLoc { size_t countOffset; size_t tableOffset; };
        constexpr STableLoc kTables[2] = { { 0x13, 0x21 }, { 0x72, 0x80 } };

        std::ofstream out("string_overrides_dump.txt");

        for (int tableIndex = 0; tableIndex < 2; ++tableIndex)
        {
            uintptr_t const countAddr = Memory::GetRipRelativeAddress(lookupFuncAddr + kTables[tableIndex].countOffset, 3, 7);
            uintptr_t const tablePtrAddr = Memory::GetRipRelativeAddress(lookupFuncAddr + kTables[tableIndex].tableOffset, 3, 7);

            uint32_t const count = *reinterpret_cast<uint32_t*>(countAddr);
            uintptr_t const tableBase = *reinterpret_cast<uintptr_t*>(tablePtrAddr);

            out << "===== Table " << tableIndex << " (" << count << " entries) =====\n";

            for (uint32_t i = 0; i < count; ++i)
            {
                uint32_t const hash = *reinterpret_cast<uint32_t*>(tableBase + i * 16);
                char const* str = *reinterpret_cast<char* const*>(tableBase + i * 16 + 8);

                out << "0x" << std::hex << std::setw(8) << std::setfill('0') << hash << std::dec
                    << " : " << (str ? str : "(null)") << "\n";
            }
        }
    }

#endif


}



void CaptionReplacements::Setup()
{
    if (!(eGameType & (MGS2 | MGS3)))
    {
        return;
    }



    InitializeCaptionOverrides();

    if (uint8_t* BP_GetOverrideStringScanResult = Memory::PatternScan(baseModule, "40 57 48 83 EC ?? 48 8B F9 48 85 C9 74 ?? 48 89 5C 24", "MGS 2: Caption Typo Fix: bp/shared/BP_Misc.cpp -> BP_GetOverrideString()"))
    {
#ifdef _DUMP_OVERRIDE_STRINGS
        g_InputHandler.RegisterHotkey(VK_NUMPAD1, "dump strings", []()
                                      {

                                          DumpStringOverrides(BP_GetOverrideStringScanResult);

                                      });
#endif 
        BP_GetOverrideString_hook = safetyhook::create_inline(reinterpret_cast<void*>(BP_GetOverrideStringScanResult), reinterpret_cast<void*>(BP_GetOverrideString_hk));
        LOG_HOOK(BP_GetOverrideString_hook, "MGS 2: Caption Typo Fix: bp/shared/BP_Misc.cpp -> BP_GetOverrideString()")
    }
}
