#include "common.hpp"
#include "steamworks_api.hpp"
#include "stat_persistence.hpp"
#include <logging.hpp>


#pragma warning(push)
#pragma warning(disable:4828)
#include <isteamuser.h>
#include <isteamuserstats.h>
#include "isteaminput.h"

#pragma warning(pop)

namespace
{
    SafetyHookMid SteamMidhook;
    SafetyHookMid SteamInputMidhook;

}


void SteamAPI::FetchAndCacheSteamID()
{
    CSteamID mySteamID = SteamUser()->GetSteamID();
    if (!mySteamID.IsValid())
    {
        spdlog::error("SteamAPI: Failed to fetch SteamID. Is Steam running?");
        return;
    }
    g_SteamAPI.steamID = mySteamID.ConvertToUint64();
    OnSteamInitialized();
}

void SteamAPI::OnSteamInitialized()
{
    g_SteamAPI.bInitialized = true;
    ResetAllAchievements(); //DON'T FREAK OUT READING THIS. bResetAchievements needs to be true, and there's multiple user confirmations first.
                            //This must ALWAYS be called before StatPersistence to make sure we don't wipe out the user's persistence file if they cancel the reset.
    g_StatPersistence.OnSteamInitialized();
}


bool SteamAPI::SetAchievement(const char* achievementID)
{
    // Sync any locally cached changes from the game. If a newer version is on the server already, it'll skip this.
    SteamUserStats()->StoreStats();

    // Pull current stats from server to sync cache
    if (!SteamUserStats()->RequestCurrentStats())
    {
        spdlog::error("Steam Achievements: RequestCurrentStats() before setting failed.");
    }

    if (!SteamUserStats()->SetAchievement(achievementID))
    {
        spdlog::error("Steam Achievements: Failed to set achievement '{}'.", achievementID);
    }

    if (!SteamUserStats()->StoreStats())
    {
        spdlog::error("Steam Achievements: Failed to store stats after setting achievement '{}'.", achievementID);
    }

    // Sync local cache with server again
    if (!SteamUserStats()->RequestCurrentStats())
    {
        spdlog::warn("Steam Achievements: RequestCurrentStats() after setting failed.");
    }

    return true;
}


bool SteamAPI::ClearAchievement(const char* achievementID)
{
    // Sync any locally cached changes from the game. If a newer version is on the server already, it'll skip this.
    SteamUserStats()->StoreStats();

    // Pull current stats from server to sync cache
    if (!SteamUserStats()->RequestCurrentStats())
    {
        spdlog::error("Steam Achievements: RequestCurrentStats() before clearing failed.");
    }

    if (!SteamUserStats()->ClearAchievement(achievementID))
    {
        spdlog::error("Steam Achievements: Failed to clear achievement '{}'.", achievementID);
    }

    if (!SteamUserStats()->StoreStats())
    {
        spdlog::error("Steam Achievements: Failed to store stats after clearing achievement '{}'.", achievementID);
    }

    // sync local cache with server again
    if (!SteamUserStats()->RequestCurrentStats())
    {
        spdlog::warn("Steam Achievements: RequestCurrentStats() after clearing failed.");
    }

    return true;
}

void SteamAPI::ResetAllAchievements()
{
    if (!g_SteamAPI.bResetAchievements)
    {
        return;
    }

    int result = MessageBoxA(
        nullptr,
        "You've set RESET ALL ACHIEVEMENTS to TRUE in the config file.\n"
        "This will RESET everything - there's no going back.\n"
        "Are you sure you want to do this?",
        "MGSHDFix Confirmation",
        MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2
    );
    if (result != IDYES)
    {
        g_SteamAPI.bResetAchievements = false;
        spdlog::info("Steam Achievements: User declined to reset achievements.");
        return;
    }

    spdlog::info("User confirmed to reset achievements. Doublechecking...");
    result = MessageBoxA(
        nullptr,
        "Last chance! Are you sure you want to reset all your stats/achievements?\n"
        "(Once cleared, you'll have to manually change RESET ALL ACHIEVEMENTS in the config file back to false.)\n",
        "MGSHDFix Confirmation",
        MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON2
    );

    if (result != IDYES)
    {
        g_SteamAPI.bResetAchievements = false;
        spdlog::info("Steam Achievements: User declined to reset achievements.");
        return;
    }

    spdlog::info("Steam Achievements: Setting achievements!");

    int numAchievements = static_cast<int>(SteamUserStats()->GetNumAchievements());
    if (numAchievements <= 0)
    {
        spdlog::warn("Steam Achievements: No achievements found to reset.");
        return;
    }

    SteamUserStats()->StoreStats(); // flush anything staged
    SteamUserStats()->RequestCurrentStats(); // sync

    for (int i = 0; i < numAchievements; ++i)
    {
        const char* achID = SteamUserStats()->GetAchievementName(i);
        if (!SteamUserStats()->ClearAchievement(achID))
        {
            spdlog::error("Steam Achievements: Failed to clear achievement '{}'.", achID);
        }
        else
        {
            spdlog::info("Steam Achievements: Cleared achievement '{}'.", achID);
        }
    }

    if (!SteamUserStats()->StoreStats())
    {
        spdlog::error("Steam Achievements: Failed to store stats after resetting achievements.");
    }

    if (!SteamUserStats()->RequestCurrentStats())
    {
        spdlog::warn("Steam Achievements: RequestCurrentStats() after resetting achievements failed.");
    }

    spdlog::info("Steam Achievements: All achievements reset.");
}


void SteamAPI::Setup() const
{
    if (!bIsLegitCopy)
    {
        spdlog::warn("SteamAPI: Steam achievement/stat tracking fixes are disabled due to non-legitimate copy.");
        return;
    }

    if (!(eGameType & (MG | MGS2 | MGS3)))
    {
        return;
    }

    if (uint8_t* AfterSteamSetupResult = Memory::PatternScan(baseModule, eGameType & MGS2 ? "48 8B 05 ?? ?? ?? ?? 8B CB" : (eGameType & MGS3 ? "48 8B 05 ?? ?? ?? ?? 8B CF 83 78" : "48 8B 05 ?? ?? ?? ?? 8B CF"), "Steam API Initialization"))
    {
        SteamMidhook = safetyhook::create_mid(AfterSteamSetupResult,
            [](SafetyHookContext& ctx)
            {
                FetchAndCacheSteamID();
            });
        LOG_HOOK(SteamMidhook, "SteamAPI Initialization")
    }

}

void SteamAPI::LogControllers()
{
    if (!g_SteamAPI.bInitialized)
    {
        return;
    }
    ISteamInput* steamInput = SteamInput();
    InputHandle_t handles[STEAM_INPUT_MAX_COUNT] = {};
    steamInput->RunFrame();
    int count = steamInput->GetConnectedControllers(handles);

    spdlog::info("SteamInput: Detected {} controller{}.", count, count == 1 ? "" : "s");

    for (int i = 0; i < count; ++i)
    {
        InputHandle_t handle = handles[i];
        ESteamInputType type = steamInput->GetInputTypeForHandle(handle);
        int gamepadIndex = steamInput->GetGamepadIndexForController(handle);

        const char* typeStr = "Unknown";
        switch (type)
        {
        case k_ESteamInputType_SteamController: typeStr = "Steam Controller"; break;
        case k_ESteamInputType_XBox360Controller: typeStr = "Xbox 360"; break;
        case k_ESteamInputType_XBoxOneController: typeStr = "Xbox One"; break;
        case k_ESteamInputType_PS4Controller: typeStr = "PS4"; break;
        case k_ESteamInputType_PS5Controller: typeStr = "PS5"; break;
        case k_ESteamInputType_SwitchProController: typeStr = "Switch Pro"; break;
        case k_ESteamInputType_GenericGamepad: typeStr = "Generic Gamepad"; break;
        case k_ESteamInputType_AppleMFiController: typeStr = "Apple MFi"; break;
        case k_ESteamInputType_AndroidController: typeStr = "Android Controller"; break;
        case k_ESteamInputType_SwitchJoyConPair: typeStr = "Switch Joy-Con Pair"; break;
        case k_ESteamInputType_SwitchJoyConSingle: typeStr = "Switch Joy-Con Single"; break;
        case k_ESteamInputType_MobileTouch: typeStr = "Mobile Touch"; break;
        case k_ESteamInputType_PS3Controller: typeStr = "PS3"; break;
        case k_ESteamInputType_SteamDeckController: typeStr = "Steam Deck"; break;
        default: break;
        }

        spdlog::info("SteamInput: Controller #{} | Type: {} | Handle: {} | Input Handler: {}", i+1, typeStr, handle, gamepadIndex == -1 ? "Steam Input" : std::to_string(gamepadIndex));

    }

}
