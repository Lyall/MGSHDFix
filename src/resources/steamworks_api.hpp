#pragma once

#include <optional>
#include <cstdint>
#pragma comment(lib,"steam_api64.lib")

class SteamAPI final
{
private:
    static void OnSteamInitialized();
    static void FetchAndCacheSteamID();
    static void ResetAllAchievements();

public:
    void Setup() const;


    // Achievement-related wrappers
    [[nodiscard]] static bool SetAchievement(const char* achievementID);
    [[nodiscard]] static bool ClearAchievement(const char* achievementID);

    bool bIsLegitCopy = true;
    bool bIsOnline = true;
    std::optional<uint64_t> steamID;
    bool bResetAchievements = false;

};

inline SteamAPI g_SteamAPI;
