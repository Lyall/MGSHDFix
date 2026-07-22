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
        None,
        Correction,
        ForceOriginal,
    };

    enum class ForcePs2Action : uint8_t
    {
        None,
        UsePs2String,
        Correction,
    };

    struct CaptionEntry
    {
        CaptionAction action = CaptionAction::None;
        const char* replacement = nullptr; // valid only when action == Correction
        ForcePs2Action forcePs2Action = ForcePs2Action::None;
        const char* forcePs2Replacement = nullptr; // valid only when forcePs2Action == Correction
    };

    using CaptionEntryMap = std::unordered_map<uint32_t, CaptionEntry>;

    struct CaptionOverrideTables
    {
        CaptionEntryMap entries;
    };

    CaptionOverrideTables g_MGS2CaptionOverrides;
    CaptionOverrideTables g_MGS3CaptionOverrides;
    SafetyHookInline BP_GetOverrideString_hook {};

    template <size_t N>
    void LoadForceOriginal(CaptionEntryMap& destination, const uint32_t(&hashes)[N])
    {
        for (uint32_t const hash : hashes)
        {
            CaptionEntry& entry = destination[hash];

            if (entry.action != CaptionAction::None)
            {
                spdlog::warn("Duplicate hash in force-original list: {:#010x}", hash);
                continue;
            }

            entry.action = CaptionAction::ForceOriginal;
        }
    }

    template <size_t N>
    void LoadBuiltInCorrections(CaptionEntryMap& destination, const CaptionOverride(&entries)[N])
    {
        for (CaptionOverride const& correction : entries)
        {
            CaptionEntry& entry = destination[correction.ps2_crc32];

            if (entry.action != CaptionAction::None)
            {
                spdlog::warn("Duplicate correction hash (already present as force-original or duplicate entry): {:#010x}", correction.ps2_crc32);
                continue;
            }

            entry.action = CaptionAction::Correction;
            entry.replacement = correction.replacement;
        }
    }

    template <size_t N>
    void LoadForcePs2Strings(CaptionEntryMap& destination, const uint32_t(&hashes)[N])
    {
        for (uint32_t const hash : hashes)
        {
            CaptionEntry& entry = destination[hash];

            if (entry.forcePs2Action != ForcePs2Action::None)
            {
                spdlog::warn("Duplicate hash in Force PS2 string list: {:#010x}", hash);
                continue;
            }

            entry.forcePs2Action = ForcePs2Action::UsePs2String;
        }
    }

    template <size_t N>
    void LoadForcePs2Corrections(CaptionEntryMap& destination, const CaptionOverride(&entries)[N])
    {
        for (CaptionOverride const& correction : entries)
        {
            CaptionEntry& entry = destination[correction.ps2_crc32];

            if (entry.forcePs2Action == ForcePs2Action::Correction)
            {
                spdlog::warn("Duplicate Force PS2 correction hash: {:#010x}", correction.ps2_crc32);
                continue;
            }

            entry.forcePs2Action = ForcePs2Action::Correction;
            entry.forcePs2Replacement = correction.replacement;
        }
    }

    void InitializeCaptionOverrides()
    {
        CaptionOverrideTables& tables = (eGameType & MGS2) ? g_MGS2CaptionOverrides : g_MGS3CaptionOverrides;

        tables.entries.max_load_factor(0.8f);

        if (eGameType & MGS2)
        {
            size_t reserveCount = std::size(kMGS2ForceOriginalHashes) + std::size(kMGS2CaptionTypoFixes);

            if (CaptionReplacements::bForcePS2)
            {
                reserveCount += std::size(kMGS2ForcePs2StringHashes);
                //reserveCount += std::size(kMGS2ForcePs2Corrections);
            }

            tables.entries.reserve(reserveCount);
            LoadForceOriginal(tables.entries, kMGS2ForceOriginalHashes);
            LoadBuiltInCorrections(tables.entries, kMGS2CaptionTypoFixes);

            if (CaptionReplacements::bForcePS2)
            {
                LoadForcePs2Strings(tables.entries, kMGS2ForcePs2StringHashes);
                //LoadForcePs2Corrections(tables.entries, kMGS2ForcePs2Corrections);
            }
        }
        else
        {
            size_t reserveCount = std::size(kMGS3CaptionTypoFixes);

            if (CaptionReplacements::bForcePS2)
            {
                reserveCount += std::size(kMGS3ForcePs2StringHashes) + std::size(kMGS3ForcePs2Corrections);
            }

            tables.entries.reserve(reserveCount);
            //LoadForceOriginal(tables.entries, kMGS3ForceOriginalHashes);
            LoadBuiltInCorrections(tables.entries, kMGS3CaptionTypoFixes);

            if (CaptionReplacements::bForcePS2)
            {
                LoadForcePs2Strings(tables.entries, kMGS3ForcePs2StringHashes);
                LoadForcePs2Corrections(tables.entries, kMGS3ForcePs2Corrections);
            }
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

        if (auto const it = tables.entries.find(hash); it != tables.entries.end())
        {
            CaptionEntry const& entry = it->second;

            if (CaptionReplacements::bForcePS2)
            {
                if (entry.forcePs2Action == ForcePs2Action::Correction)
                {
                    //spdlog::info("Forcing PS2 caption replacement for hash {:#010x}: {}", hash, entry.forcePs2Replacement);
                    return const_cast<char*>(entry.forcePs2Replacement);
                }

                if (entry.forcePs2Action == ForcePs2Action::UsePs2String)
                {
                    //spdlog::info("Forcing PS2 caption for hash {:#010x}: {}", hash, inputString);
                    return inputString;
                }
            }

            if (entry.action == CaptionAction::ForceOriginal)
            {
                return inputString;
            }

            if (entry.action == CaptionAction::Correction)
            {
                return const_cast<char*>(entry.replacement);
            }
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
