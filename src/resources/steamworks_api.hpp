#pragma once

#include <optional>
#include <cstdint>

#pragma warning(push)
#pragma warning(disable:4828)
#include <isteaminput.h>
#pragma warning(pop)

#pragma comment(lib,"steam_api64.lib")

class SteamAPI final
{
private:
    static void OnSteamInitialized();
    static void FetchAndCacheSteamID();
    static void ResetAllAchievements();

public:
    bool bInitialized = false;
    void Setup() const;

    static void OnSteamInputLoaded();

    // Achievement-related wrappers
    [[nodiscard]] static bool SetAchievement(const char* achievementID);
    [[nodiscard]] static bool ClearAchievement(const char* achievementID);

    bool bIsLegitCopy = true;
    bool bIsOnline = true;
    std::optional<uint64_t> steamID;
    bool bResetAchievements = false;
    bool bDisableSteamAchievementUnlocking = false;

    int* iNumberOfControllers = nullptr;

};

inline SteamAPI g_SteamAPI;
