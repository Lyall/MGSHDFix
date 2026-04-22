#include "stdafx.h"
#include "bugfix_mod_checks.hpp"

#include "common.hpp"
#include "config_keys.hpp"
#include "d3d11_api.hpp"
#include "logging.hpp"


namespace
{
    // ==========================================================
    // POLICY
    // ==========================================================
    struct WarningPolicy
    {
        uint32_t initialWarningCount = 3; // initial warning phase budget
        uint32_t cooldownDays = 30;       // after initial phase, allow 1 warning per cooldownDays
    };

    // ==========================================================
    // CACHE
    // ==========================================================
    constexpr uint32_t kCacheMagic = 'MWC1'; // Mod Warning Cache File
    constexpr uint32_t kCacheVersion = 9;

    struct WarningEntry
    {
        uint32_t shownCount = 0;           // total times shown (monotonic)
        uint64_t lastShownUnix = 0;        // seconds since epoch (when we actually showed it)
        bool initialPhaseComplete = false; // true once initial warning phase is exhausted
    };

    struct ModWarningCache
    {
        std::wstring installPath;
        std::wstring installDate;
        std::unordered_map<std::string, WarningEntry> warn;
    };

    uint64_t NowUnixSeconds()
    {
        using namespace std::chrono;
        return static_cast<uint64_t>(duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
    }

    uint64_t DaysToSeconds(uint32_t days)
    {
        return static_cast<uint64_t>(days) * 24ull * 60ull * 60ull;
    }

    std::wstring GetWindowsInstallDate()
    {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        {
            return L"";
        }

        DWORD installDate = 0;
        DWORD size = sizeof(installDate);

        if (RegQueryValueExW(hKey, L"InstallDate", nullptr, nullptr, reinterpret_cast<LPBYTE>(&installDate), &size) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return L"";
        }

        RegCloseKey(hKey);

        wchar_t buf[64];
        swprintf_s(buf, L"%u", installDate);
        return buf;
    }

    // ==========================================================
    // LOAD/SAVE
    // ==========================================================
    ModWarningCache LoadCache(const std::filesystem::path& file)
    {
        ModWarningCache data;

        if (!std::filesystem::exists(file))
            return data;

        std::ifstream f(file, std::ios::binary);
        if (!f)
            return data;

        uint32_t magic = 0;
        uint32_t version = 0;

        f.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        f.read(reinterpret_cast<char*>(&version), sizeof(version));

        if (!f || magic != kCacheMagic || version != kCacheVersion)
        {
            // Wrong file or incompatible version: start clean.
            return ModWarningCache {};
        }

        uint32_t pathLen = 0;
        uint32_t dateLen = 0;
        uint32_t entryCount = 0;

        f.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen));
        if (pathLen)
        {
            std::vector<wchar_t> buf(pathLen);
            f.read(reinterpret_cast<char*>(buf.data()), pathLen * sizeof(wchar_t));
            data.installPath.assign(buf.begin(), buf.end());
        }

        f.read(reinterpret_cast<char*>(&dateLen), sizeof(dateLen));
        if (dateLen)
        {
            std::vector<wchar_t> buf(dateLen);
            f.read(reinterpret_cast<char*>(buf.data()), dateLen * sizeof(wchar_t));
            data.installDate.assign(buf.begin(), buf.end());
        }

        f.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));

        for (uint32_t i = 0; i < entryCount; ++i)
        {
            uint32_t keyLen = 0;
            f.read(reinterpret_cast<char*>(&keyLen), sizeof(keyLen));

            std::string key(keyLen, '\0');
            f.read(key.data(), keyLen);

            WarningEntry e;
            f.read(reinterpret_cast<char*>(&e.shownCount), sizeof(e.shownCount));
            f.read(reinterpret_cast<char*>(&e.lastShownUnix), sizeof(e.lastShownUnix));

            uint8_t phaseComplete = 0;
            f.read(reinterpret_cast<char*>(&phaseComplete), sizeof(phaseComplete));
            e.initialPhaseComplete = (phaseComplete != 0);

            data.warn[key] = e;
        }

        return data;
    }

    void SaveCache(const std::filesystem::path& file, const ModWarningCache& data)
    {
        std::ofstream f(file, std::ios::binary | std::ios::trunc);
        if (!f)
            return;

        f.write(reinterpret_cast<const char*>(&kCacheMagic), sizeof(kCacheMagic));
        f.write(reinterpret_cast<const char*>(&kCacheVersion), sizeof(kCacheVersion));

        const uint32_t pathLen = static_cast<uint32_t>(data.installPath.size());
        const uint32_t dateLen = static_cast<uint32_t>(data.installDate.size());
        const uint32_t entryCount = static_cast<uint32_t>(data.warn.size());

        f.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));
        if (pathLen)
            f.write(reinterpret_cast<const char*>(data.installPath.data()), pathLen * sizeof(wchar_t));

        f.write(reinterpret_cast<const char*>(&dateLen), sizeof(dateLen));
        if (dateLen)
            f.write(reinterpret_cast<const char*>(data.installDate.data()), dateLen * sizeof(wchar_t));

        f.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));

        for (const auto& kv : data.warn)
        {
            const std::string& key = kv.first;
            const WarningEntry& e = kv.second;

            const uint32_t keyLen = static_cast<uint32_t>(key.size());
            f.write(reinterpret_cast<const char*>(&keyLen), sizeof(keyLen));
            f.write(key.data(), keyLen);

            f.write(reinterpret_cast<const char*>(&e.shownCount), sizeof(e.shownCount));
            f.write(reinterpret_cast<const char*>(&e.lastShownUnix), sizeof(e.lastShownUnix));

            const uint8_t phaseComplete = e.initialPhaseComplete ? 1u : 0u;
            f.write(reinterpret_cast<const char*>(&phaseComplete), sizeof(phaseComplete));
        }
    }

    // ==========================================================
    // MESSAGE HELPERS
    // ==========================================================
    std::string BuildInitialPhaseTail(uint32_t remainingAfterThis, uint32_t cooldownDays)
    {
        if (remainingAfterThis > 0)
        {
            return "----------------\n"
                   "\n"
                   "This warning will be hidden after " + std::to_string(remainingAfterThis) + " more launch" + (remainingAfterThis == 1 ? "" : "es") + ".\n"
                                                                                                                                                        "\n"
                                                                                                                                                         "It can also be disabled in the config tool.\n"
                                                                                                                                                         "Internal -> " + ConfigKeys::MissingBugfixModWarning_Setting;
        }

        return "----------------\n"
                "\n"
                "This warning will be hidden for the next " + std::to_string(cooldownDays) + " days.\n"
                                                                                            "\n" 
                                                                                            "It can also be disabled in the config tool.\n"
                                                                                            "Internal -> " + ConfigKeys::MissingBugfixModWarning_Setting;
    }

    std::string BuildCooldownTail(uint32_t cooldownDays)
    {
        return "----------------\n"
                "\n"
                "This reminder will appear once every " + std::to_string(cooldownDays) + " days.\n"
                                                                                         "\n"
                                                                                         "It can also be disabled in the config tool.\n"
                                                                                         "Internal -> " + ConfigKeys::MissingBugfixModWarning_Setting;
    }

    // ==========================================================
    // WARNING LOGIC
    // ==========================================================
    uint32_t GetWarningsRemaining(ModWarningCache& cache, const std::string& key, const WarningPolicy& policy)
    {
        const auto it = cache.warn.find(key);
        if (it == cache.warn.end())
        {
            return policy.initialWarningCount;
        }

        WarningEntry& e = it->second;

        if (!e.initialPhaseComplete)
        {
            const uint32_t used = (e.shownCount >= policy.initialWarningCount) ? policy.initialWarningCount : e.shownCount;
            return policy.initialWarningCount - used;
        }

        const uint64_t now = NowUnixSeconds();
        return (e.lastShownUnix == 0 || now >= e.lastShownUnix + DaysToSeconds(policy.cooldownDays)) ? 1u : 0u;
    }

    bool ShouldWarn(ModWarningCache& cache, const std::string& key, const WarningPolicy& policy)
    {
        return GetWarningsRemaining(cache, key, policy) > 0;
    }

    void RecordWarning(ModWarningCache& cache, const std::filesystem::path& file, const std::string& key, const WarningPolicy& policy)
    {
        WarningEntry& e = cache.warn[key];

        if (!e.initialPhaseComplete)
        {
            const uint32_t used = (e.shownCount >= policy.initialWarningCount) ? policy.initialWarningCount : e.shownCount;

            if (used + 1 >= policy.initialWarningCount)
            {
                e.initialPhaseComplete = true;
            }
        }

        e.shownCount++;
        e.lastShownUnix = NowUnixSeconds();

        SaveCache(file, cache);
    }

    void HardResetIfEnvironmentChanged(ModWarningCache& cache, const std::filesystem::path& file)
    {
        const std::wstring curPath = std::filesystem::current_path().wstring();
        const std::wstring curDate = GetWindowsInstallDate();

        if (cache.installPath == curPath && cache.installDate == curDate)
        {
            return;
        }

        spdlog::info("Resetting mod warning cache (environment changed)");

        cache.warn.clear();
        cache.installPath = curPath;
        cache.installDate = curDate;

        SaveCache(file, cache);
    }
}

void BugfixMods::Check()
{
    const WarningPolicy policy { 3, 30 };

    const auto cacheFile = sGameSavePath / "MGSHDFix_Mod_Warnings.bin";
    ModWarningCache cache = LoadCache(cacheFile);

    HardResetIfEnvironmentChanged(cache, cacheFile);


    if (eGameType & (MGS2 | MGS3))
    {
        // ------------------------------------------------------
        // MGS2 & MGS3 Community Bugfix Compilation
        // ------------------------------------------------------

        {
            std::filesystem::path CommunityBugfixCompilationASI = sExePath / "plugins" / ((eGameType & MGS2) ? "MGS2-Community-Bugfix-Compilation.asi" : "MGS3-Community-Bugfix-Compilation.asi");

            spdlog::info("Bugfix Mod Checks: Checking for {} Community Bugfix Compilation.", (eGameType & MGS2) ? "MGS2" : "MGS3", CommunityBugfixCompilationASI.string());
            if (!std::filesystem::exists(CommunityBugfixCompilationASI))
            {
                spdlog::info("");
                spdlog::info("=====================================    Bugfix Mod Checks     =====================================");
                spdlog::info("{} Community Bugfix Compilation mod is not currently installed.", (eGameType & MGS2) ? "MGS2" : "MGS3");
                if (eGameType & MGS2)
                {
                    spdlog::info("This mod fixes nearly 14,000 texture issues, hundreds of transparent textures / models, missing audio / music, and countless localization errors / typos introduced by the 2011 Bluepoint HD remaster.");
                }
                else
                {
                    spdlog::info("This mod fixes nearly 4000 texture issues, over 500 transparent textures / models, restores missing regional content, and corrects countless localization errors / typos introduced by the 2011 Bluepoint HD remaster.");
                }
                spdlog::info("=====================================    Bugfix Mod Checks     =====================================");
                spdlog::info("");

            }
            else
            {
                spdlog::info("Bugfix Mod Checks: {} Community Bugfix Compilation Check: Mod is installed.", (eGameType & MGS2) ? "MGS2" : "MGS3");
            }
        }
    }



    if (eGameType & MGS2)
    {
        spdlog::info("Bugfix Mod Checks: Checking for missing MGS2 Bugfix Mods...");

        if (usDatExists) //don't bother warning people who only have japanese audio installed since the crash only affects US/EU audio.
        {
            // ------------------------------------------------------
            // MGS2: Better Audio Mod Crash Fix Check
            // ------------------------------------------------------
            spdlog::info("Bugfix Mod Checks: US/EU DAT language pack detected, checking for MGS2 Better Audio Mod / Crash Fix.");
            {
                const std::string key = "MGS2_BetterAudioMod";
                const auto sdtPath = sExePath / "us" / "demo" / "_bp" / "p070_01_p01.sdt";

                spdlog::info("Bugfix Mod Checks: MGS2 Better Audio Mod Crash Fix Check: Checking for file: {}", sdtPath.string());
                if (std::filesystem::exists(sdtPath) && Util::SHA1Check(sdtPath, "ae7497c2a59bc0c1597f49b3e4a26543eb4888a3"))
                {
                    const uint32_t remaining = GetWarningsRemaining(cache, key, policy);

                    if (ShouldWarn(cache, key, policy))
                    {
                        const bool inInitialPhase = !cache.warn[key].initialPhaseComplete;

                        std::string message;
                        if (inInitialPhase)
                        {
                            const uint32_t remainingAfterThis = (remaining > 0) ? (remaining - 1) : 0;
                            message =
                                "Warning: The MGS2 Better Audio mod is not currently installed.\n"
                                "\n"
                                "The Better Audio mod fixes a critical hang/crash that occurs very late into the game.\n"
                                "It is HIGHLY recommended to install the mod, otherwise you will most likely be unable to finish the game.\n"
                                "\n"
                                "Would you like to open the mod page now?\n"
                                "\n" +
                                BuildInitialPhaseTail(remainingAfterThis, policy.cooldownDays);
                        }
                        else
                        {
                            message =
                                "Reminder: The MGS2 Better Audio mod is not currently installed.\n"
                                "\n"
                                "This mod fixes a critical hang/crash that occurs very late into the game.\n"
                                "It is HIGHLY recommended to install the mod, otherwise you will most likely be unable to finish the game.\n"
                                "\n"
                                "Would you like to open the mod page now?\n"
                                "\n" +
                                BuildCooldownTail(policy.cooldownDays);
                        }

                        spdlog::warn(message);

                        if (bEnableVisibleWarnings)
                        {
                            if (Util::IsSteamOS())
                            {
                                std::cout
                                    << "\n================ MGSHDFix WARNING ================\n\n"
                                    << message
                                    << "\nMod page:\n"
                                    << "https://www.nexusmods.com/metalgearsolid2mc/mods/3\n"
                                    << "\n=================================================\n\n";
                            }
                            else
                            {
                                if (int result = MessageBoxA(
                                    g_D3D11Hooks.MainHwnd,
                                    message.c_str(),
                                    "MGSHDFix - Bugfix Warning",
                                    MB_ICONWARNING | MB_YESNO);
                                    result == IDYES)
                                {
                                    ShellExecuteA(
                                        nullptr,
                                        "open",
                                        "https://www.nexusmods.com/metalgearsolid2mc/mods/3",
                                        nullptr,
                                        nullptr,
                                        SW_SHOWNORMAL
                                    );
                                }
                            }
                        }

                        RecordWarning(cache, cacheFile, key, policy);
                    }
                    else
                    {
                        spdlog::info("Bugfix Mod Checks: MGS2 Better Audio Mod Crash Fix: Not currently installed.");
                    }
                }
                else
                {
                    spdlog::info("Bugfix Mod Checks: MGS2 Better Audio Mod Crash Fix Check: SDT is not vanilla. Assuming mod is installed!");
                }
            }
        }

    }

}
