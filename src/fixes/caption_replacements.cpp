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
        const char* vanillaKeyboardOverrides = nullptr; // BP_LookupStringOveride's mouse & keyboard table - only used when no controller is detected by Steam Input
        const char* vanillaOverrides = nullptr;        // BP_LookupStringOveride's override table
    };

    using CaptionEntryMap = std::unordered_map<uint32_t, CaptionEntry>;

    struct CaptionOverrideTables
    {
        CaptionEntryMap entries;
    };

    CaptionOverrideTables g_MGS2CaptionOverrides;
    CaptionOverrideTables g_MGS3CaptionOverrides;
    SafetyHookInline BP_LookupStringOveride_hook {};

#ifdef _DUMP_INPUT_STRINGS
    SafetyHookMid BP_GetOverrideString_logging_hook {};
#endif

    struct SVanillaTableLocations
    {
        uintptr_t mouseKeyboardCountAddr;
        uintptr_t mouseKeyboardTablePtrAddr;
        uintptr_t gamepadCountAddr;
        uintptr_t gamepadTablePtrAddr;
    };

    struct SVanillaLoadResult
    {
        size_t mouseKeyboardLoaded = 0;
        size_t mouseKeyboardOverlappedWithCustom = 0;
        size_t gamepadLoaded = 0;
        size_t gamepadOverlappedWithCustom = 0;
        size_t gamepadOverlappedWithMouseKeyboard = 0;
    };

    SVanillaTableLocations g_VanillaTableLocations {};
    bool g_VanillaTableLocationsResolved = false;

    template <size_t N>
    size_t LoadForceOriginal(CaptionEntryMap& destination, const uint32_t(&hashes)[N])
    {
        size_t loaded = 0;

        for (uint32_t const hash : hashes)
        {
            CaptionEntry& entry = destination[hash];

            if (entry.action != CaptionAction::None)
            {
                spdlog::warn("Duplicate hash in force-original list: {:#010x}", hash);
                continue;
            }

            entry.action = CaptionAction::ForceOriginal;
            ++loaded;
        }

        return loaded;
    }

    template <size_t N>
    size_t LoadTypoCorrections(CaptionEntryMap& destination, const CaptionOverride(&entries)[N])
    {
        size_t loaded = 0;

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
            ++loaded;
        }

        return loaded;
    }

    template <size_t N>
    size_t LoadForcePs2Strings(CaptionEntryMap& destination, const uint32_t(&hashes)[N])
    {
        size_t loaded = 0;

        for (uint32_t const hash : hashes)
        {
            CaptionEntry& entry = destination[hash];

            if (entry.forcePs2Action != ForcePs2Action::None)
            {
                spdlog::warn("Duplicate hash in Force PS2 string list: {:#010x}", hash);
                continue;
            }

            entry.forcePs2Action = ForcePs2Action::UsePs2String;
            ++loaded;
        }

        return loaded;
    }

    template <size_t N>
    size_t LoadForcePs2Corrections(CaptionEntryMap& destination, const CaptionOverride(&entries)[N])
    {
        size_t loaded = 0;

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
            ++loaded;
        }

        return loaded;
    }

    bool ResolveVanillaTableLocations(SVanillaTableLocations& out)
    {
        uint8_t* const mouseKeyboardMatch = Memory::PatternScan(baseModule, "44 8B 0D ?? ?? ?? ?? 44 8B DF 41 83 E9 ?? 48 8B 1D ?? ?? ?? ?? 78 ?? 66 0F 1F 44 00", "MGS 2: Caption Typo Fix: BP_LookupStringOveride mouse & keyboard table");

        uint8_t* const gamepadMatch = Memory::PatternScan(baseModule, "44 8B 0D ?? ?? ?? ?? 44 8B DF 41 83 E9 ?? 48 8B 1D ?? ?? ?? ?? 78 ?? 0F 1F 80", "MGS 2: Caption Typo Fix: BP_LookupStringOveride standard override table");

        if (!mouseKeyboardMatch || !gamepadMatch)
        {
            return false;
        }

        out.mouseKeyboardCountAddr = Memory::GetRipRelativeAddress(mouseKeyboardMatch, 3, 7);
        out.mouseKeyboardTablePtrAddr = Memory::GetRipRelativeAddress(mouseKeyboardMatch + 14, 3, 7);
        out.gamepadCountAddr = Memory::GetRipRelativeAddress(gamepadMatch, 3, 7);
        out.gamepadTablePtrAddr = Memory::GetRipRelativeAddress(gamepadMatch + 14, 3, 7);
        spdlog::info("ResolveVanillaTableLocations: Resolved vanilla BP_LookupStringOverride tables in {:s}: (M&K - Count: +{:X} | Table: +{:X}) (Standard Overrides - Count: +{:X} | Table: +{:X})", sExeName.c_str(), (uintptr_t)out.mouseKeyboardCountAddr - (uintptr_t)baseModule, (uintptr_t)out.mouseKeyboardTablePtrAddr - (uintptr_t)baseModule, (uintptr_t)out.gamepadCountAddr - (uintptr_t)baseModule, (uintptr_t)out.gamepadTablePtrAddr - (uintptr_t)baseModule);

        return true;
    }

    SVanillaLoadResult LoadVanillaOverrides(CaptionEntryMap& destination, SVanillaTableLocations const& loc)
    {
        auto loadTable = [&](uintptr_t countAddr, uintptr_t tablePtrAddr, const char* CaptionEntry::* field, bool checkMouseKeyboardOverlap) -> std::tuple<size_t, size_t, size_t>
            {
                uint32_t const count = *reinterpret_cast<uint32_t*>(countAddr);
                uintptr_t const tableBase = *reinterpret_cast<uintptr_t*>(tablePtrAddr);

                size_t loaded = 0;
                size_t overlappedWithCustom = 0;
                size_t overlappedWithMouseKeyboard = 0;

                for (uint32_t i = 0; i < count; ++i)
                {
                    uint32_t const hash = *reinterpret_cast<uint32_t*>(tableBase + i * 16);
                    char const* str = *reinterpret_cast<char* const*>(tableBase + i * 16 + 8);

                    if (!str)
                    {
                        continue;
                    }

                    auto const [it, inserted] = destination.try_emplace(hash);
                    if (!inserted)
                    {
                        if (checkMouseKeyboardOverlap && it->second.vanillaKeyboardOverrides)
                        {
                            ++overlappedWithMouseKeyboard;
                        }
                        else
                        {
                            ++overlappedWithCustom;
                        }
                    }

                    it->second.*field = str;
                    ++loaded;
                }
                return { loaded, overlappedWithCustom, overlappedWithMouseKeyboard };
            };

        auto const [mouseKeyboardLoaded, mouseKeyboardOverlappedWithCustom, unusedMkMk] = loadTable(loc.mouseKeyboardCountAddr, loc.mouseKeyboardTablePtrAddr, &CaptionEntry::vanillaKeyboardOverrides, false);
        auto const [gamepadLoaded, gamepadOverlappedWithCustom, gamepadOverlappedWithMouseKeyboard] = loadTable(loc.gamepadCountAddr, loc.gamepadTablePtrAddr, &CaptionEntry::vanillaOverrides, true);

        return { mouseKeyboardLoaded, mouseKeyboardOverlappedWithCustom, gamepadLoaded, gamepadOverlappedWithCustom, gamepadOverlappedWithMouseKeyboard };
    }


    char const* __fastcall BP_LookupStringOveride_hk(uint32_t hash, char isMouseAndKeyboard)
    {
        const CaptionOverrideTables& tables = (eGameType & MGS2) ? g_MGS2CaptionOverrides : g_MGS3CaptionOverrides;

        if (auto const it = tables.entries.find(hash); it != tables.entries.end())
        {
            CaptionEntry const& entry = it->second;

            if (CaptionReplacements::bForcePS2)
            {
                if (entry.forcePs2Action == ForcePs2Action::Correction)
                {
                    //spdlog::info("Forcing PS2 caption replacement for hash {:#010x}: {}", hash, entry.forcePs2Replacement);
                    return entry.forcePs2Replacement;
                }

                if (entry.forcePs2Action == ForcePs2Action::UsePs2String)
                {
                    //spdlog::info("Forcing PS2 caption for hash {:#010x}: {}", hash, inputString);
                    return nullptr;
                }
            }

            if (entry.action == CaptionAction::ForceOriginal)
            {
                return nullptr;
            }

            if (entry.action == CaptionAction::Correction)
            {
                return entry.replacement;
            }

            if (isMouseAndKeyboard && entry.vanillaKeyboardOverrides)
            {
                return entry.vanillaKeyboardOverrides;
            }

            if (entry.vanillaOverrides)
            {
                return entry.vanillaOverrides;
            }
        }

        return nullptr;
    }

#ifdef _DUMP_OVERRIDE_STRINGS

    void DumpStringOverrides()
    {
        if (!g_VanillaTableLocationsResolved)
        {
            spdlog::error("DumpStringOverrides: vanilla tables were never resolved.");
            return;
        }

        std::ofstream out("string_overrides_dump.txt");

        auto dumpTable = [&](uintptr_t countAddr, uintptr_t tablePtrAddr, const char* label)
            {
                uint32_t const count = *reinterpret_cast<uint32_t*>(countAddr);
                uintptr_t const tableBase = *reinterpret_cast<uintptr_t*>(tablePtrAddr);

                out << "===== " << label << " (" << count << " entries) =====\n";
                for (uint32_t i = 0; i < count; ++i)
                {
                    uint32_t const hash = *reinterpret_cast<uint32_t*>(tableBase + i * 16);
                    char const* str = *reinterpret_cast<char* const*>(tableBase + i * 16 + 8);
                    out << "0x" << std::hex << std::setw(8) << std::setfill('0') << hash << std::dec
                        << " : " << (str ? str : "(null)") << "\n";
                }
            };

        dumpTable(g_VanillaTableLocations.mouseKeyboardCountAddr, g_VanillaTableLocations.mouseKeyboardTablePtrAddr, "Mouse & Keyboard Table");
        dumpTable(g_VanillaTableLocations.gamepadCountAddr, g_VanillaTableLocations.gamepadTablePtrAddr, "Gamepad Table");
    }

#endif

}

void CaptionReplacements::InitializeCaptionOverrides()
{
    spdlog::info("InitializeCaptionOverrides: Initializing string overrides/fixes.");
    CaptionOverrideTables& tables = (eGameType & MGS2) ? g_MGS2CaptionOverrides : g_MGS3CaptionOverrides;

    size_t reserveCount = 0;

    if (g_VanillaTableLocationsResolved)
    {
        reserveCount += *reinterpret_cast<uint32_t*>(g_VanillaTableLocations.mouseKeyboardCountAddr);
        reserveCount += *reinterpret_cast<uint32_t*>(g_VanillaTableLocations.gamepadCountAddr);
    }

    if (eGameType & MGS2)
    {
        reserveCount += std::size(kMGS2ForceOriginalHashes) + std::size(kMGS2CaptionTypoFixes);

        if (CaptionReplacements::bForcePS2)
        {
            reserveCount += std::size(kMGS2ForcePs2StringHashes);
            reserveCount += std::size(kMGS2ForcePs2Corrections);
        }
    }
    else
    {
        reserveCount += std::size(kMGS3CaptionTypoFixes);

        if (CaptionReplacements::bForcePS2)
        {
            reserveCount += std::size(kMGS3ForcePs2StringHashes) + std::size(kMGS3ForcePs2Corrections);
        }
    }

    tables.entries.max_load_factor(0.8f);
    tables.entries.reserve(reserveCount);

    size_t forceOriginalLoaded = 0;
    size_t typoCorrectionsLoaded = 0;
    size_t forcePs2StringsLoaded = 0;
    size_t forcePs2CorrectionsLoaded = 0;

    if (eGameType & MGS2)
    {
        forceOriginalLoaded = LoadForceOriginal(tables.entries, kMGS2ForceOriginalHashes);
        typoCorrectionsLoaded = LoadTypoCorrections(tables.entries, kMGS2CaptionTypoFixes);

        if (CaptionReplacements::bForcePS2)
        {
            forcePs2StringsLoaded = LoadForcePs2Strings(tables.entries, kMGS2ForcePs2StringHashes);
            forcePs2CorrectionsLoaded = LoadForcePs2Corrections(tables.entries, kMGS2ForcePs2Corrections);
        }
    }
    else
    {
        typoCorrectionsLoaded = LoadTypoCorrections(tables.entries, kMGS3CaptionTypoFixes);

        if (CaptionReplacements::bForcePS2)
        {
            forcePs2StringsLoaded = LoadForcePs2Strings(tables.entries, kMGS3ForcePs2StringHashes);
            forcePs2CorrectionsLoaded = LoadForcePs2Corrections(tables.entries, kMGS3ForcePs2Corrections);
        }
    }

    SVanillaLoadResult vanillaResult;
    if (g_VanillaTableLocationsResolved)
    {
        vanillaResult = LoadVanillaOverrides(tables.entries, g_VanillaTableLocations);
    }
    else
    {
        spdlog::error("InitializeCaptionOverrides: vanilla tables were never resolved - skipping vanilla load.");
    }

    size_t const totalProcessed = forceOriginalLoaded + typoCorrectionsLoaded + forcePs2StringsLoaded + forcePs2CorrectionsLoaded
        + vanillaResult.mouseKeyboardLoaded + vanillaResult.gamepadLoaded;

    std::string overlapPart;
    if (vanillaResult.mouseKeyboardOverlappedWithCustom > 0 && vanillaResult.gamepadOverlappedWithCustom > 0)
    {
        overlapPart = fmt::format(" ({} / {} skipped by previously loaded overrides.)", vanillaResult.mouseKeyboardOverlappedWithCustom, vanillaResult.gamepadOverlappedWithCustom);
    }
    else if (vanillaResult.mouseKeyboardOverlappedWithCustom > 0)
    {
        overlapPart = fmt::format(" ({} M&K skipped by previously loaded overrides.)", vanillaResult.mouseKeyboardOverlappedWithCustom);
    }
    else if (vanillaResult.gamepadOverlappedWithCustom > 0)
    {
        overlapPart = fmt::format(" ({} standard skipped by previously loaded overrides.)", vanillaResult.gamepadOverlappedWithCustom);
    }

    spdlog::info("InitializeCaptionOverrides: {} unique override strings loaded ({} entries across all sources, {} with multiple overrides.)", tables.entries.size(), totalProcessed, totalProcessed - tables.entries.size());

    if (forceOriginalLoaded > 0)
    {
        spdlog::info("InitializeCaptionOverrides: {} decensorsed strings loaded.", forceOriginalLoaded);
    }
    if (typoCorrectionsLoaded > 0)
    {
        spdlog::info("InitializeCaptionOverrides: {} typo corrections loaded.", typoCorrectionsLoaded);
    }
    if (forcePs2StringsLoaded > 0)
    {
        spdlog::info("InitializeCaptionOverrides: {} forced PS2 strings loaded.", forcePs2StringsLoaded);
    }
    if (forcePs2CorrectionsLoaded > 0)
    {
        spdlog::info("InitializeCaptionOverrides: {} HDC strings changed to match PS2.", forcePs2CorrectionsLoaded);
    }
    if (vanillaResult.mouseKeyboardLoaded > 0 || vanillaResult.gamepadLoaded > 0)
    {
        spdlog::info("InitializeCaptionOverrides: Vanilla BP_LookupStringOveride entries loaded from the game: {} mouse & keyboard + {} standard{}", vanillaResult.mouseKeyboardLoaded, vanillaResult.gamepadLoaded, overlapPart);
    }
}


void CaptionReplacements::Setup()
{
    if (!(eGameType & (MGS2 | MGS3)))
    {
        return;
    }

    g_VanillaTableLocationsResolved = ResolveVanillaTableLocations(g_VanillaTableLocations);

    if (uint8_t* const BP_LookupStringOverideScanResult = Memory::PatternScan(baseModule, "48 89 5C 24 ?? 48 89 7C 24 ?? 33 FF 44 8B C1", "MGS 2: Caption Typo Fix: bp/shared/BP_Misc.cpp -> BP_LookupStringOveride()"))
    {
        BP_LookupStringOveride_hook = safetyhook::create_inline(reinterpret_cast<void*>(BP_LookupStringOverideScanResult), reinterpret_cast<void*>(BP_LookupStringOveride_hk));
        LOG_HOOK(BP_LookupStringOveride_hook, "MGS 2: Caption Typo Fix: bp/shared/BP_Misc.cpp -> BP_LookupStringOveride()")
    }

#ifdef _DUMP_INPUT_STRINGS
    MAKE_HOOK_MID(baseModule, "40 57 48 83 EC ?? 48 8B F9 48 85 C9 74 ?? 48 89 5C 24", "MGS 2: Caption Typo Fix: bp/shared/BP_Misc.cpp -> BP_GetOverrideString() (logging only)", {
            char const* const inputString = reinterpret_cast<char const*>(ctx.rcx);
            if (inputString)
            {
            spdlog::info("Input Hash:{:#010x}\n{}", Util::StringToCRC32(inputString), inputString);
            }
                  });

#endif

#ifdef _DUMP_OVERRIDE_STRINGS
    g_InputHandler.RegisterHotkey(VK_NUMPAD1, "dump strings", []()
                                  {
                                      DumpStringOverrides();
                                  });
#endif
}
