#include "common.hpp"
#include "steamworks_api.hpp"
#include <logging.hpp>


#pragma warning(push)
#pragma warning(disable:4828)
#include <isteamuser.h>
#include <isteamuserstats.h>
#include "isteaminput.h"

#pragma warning(pop)

void AfterSteamInitialized();
void AfterSteamInputInitialized();

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
    AfterSteamInitialized();
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




void SteamAPI::OnSteamInputLoaded()
{
    if (!g_SteamAPI.bInitialized)
    {
        return;
    }
    spdlog::info("SteamInput: Initializing...");
    ISteamInput* steamInput = SteamInput();
    steamInput->RunFrame();
    g_SteamAPI.iNumberOfControllers = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "89 05 ?? ?? ?? ?? 85 C0 75 ?? 33 DB", "MGS 2: SteamInput: iNumberOfControllers") + 2));
    InputHandle_t controllerHandles[STEAM_INPUT_MAX_COUNT] = {};
    *g_SteamAPI.iNumberOfControllers = steamInput->GetConnectedControllers(controllerHandles);

    spdlog::info("SteamInput: Detected {} controller{}.", *g_SteamAPI.iNumberOfControllers, *g_SteamAPI.iNumberOfControllers == 1 ? "" : "s");

    // Cache known action sets here by name (populate once, e.g. on first call)
    static std::unordered_map<InputActionSetHandle_t, std::string> actionSetNames;
    if (actionSetNames.empty())
    {
        actionSetNames[steamInput->GetActionSetHandle("CommonSet")] = "CommonSet";
    }

    std::vector<std::string> actionNames;

    if (eGameType & MGS2)
    {
        actionNames = {
            "ingame_cmn_action_btn",    //0x1
            "ingame_cmn_punch",         //0x2
            "ingame_cmn_weapon",        //0x3
            "ingame_cmn_sneaking",      //0x4
            "ingame_cmn_move_up",       //0x5
            "ingame_cmn_move_right",    //0x6
            "ingame_cmn_move_down",     //0x7
            "ingame_cmn_move_left",     //0x8
            "ingame_cmn_hold_weapon",   //0x9
            "ingame_cmn_blade_stab",    //0xA
            "ingame_cmn_lock_on",       //0xB
            "ingame_cmn_pov_cam",       //0xC
            "ingame_cmn_equip_window",  //0xD
            "ingame_cmn_weapon_window", //0xE
            "ingame_cmn_radio_menu",    //0xF
            "ingame_cmn_pause_menu",    //0x10
        };
    }
    else if (eGameType & MGS3)
    {
        actionNames = {
            "ingame_cmn_action_btn",    //0x1
            "ingame_cmn_punch",         //0x2
            "ingame_cmn_weapon",        //0x3
            "ingame_cmn_sneaking",      //0x4
            "ingame_cmn_move_up",       //0x5
            "ingame_cmn_move_right",    //0x6
            "ingame_cmn_move_down",     //0x7
            "ingame_cmn_move_left",     //0x8
            "ingame_cmn_interrogate",   //0x9
            "ingame_cmn_change_view",   //0xA
            "ingame_cmn_weapon_aim",    //0xB
            "ingame_cmn_pov_cam",       //0xC
            "ingame_cmn_equip_window",  //0xD
            "ingame_cmn_weapon_window", //0xE
            "ingame_cmn_radio_menu",    //0xF
            "ingame_cmn_survival_viwer",//0x10
        };

    }
    else if (eGameType & MG)
    {
        actionNames = {
            "ingame_cmn_action_btn",    //0x1
            "ingame_cmn_punch",         //0x2
            "ingame_cmn_weapon",        //0x3
            "ingame_cmn_sneaking",      //0x4
            "ingame_cmn_move_up",       //0x5
            "ingame_cmn_move_right",    //0x6
            "ingame_cmn_move_down",     //0x7
            "ingame_cmn_move_left",     //0x8
            "ingame_cmn_interrogate",   //0x9
            "ingame_cmn_change_view",   //0xA
            "ingame_cmn_weapon_aim",    //0xB
            "ingame_cmn_pov_cam",       //0xC
            "ingame_cmn_equip_menu",    //0xD
            "ingame_cmn_weapon_menu",   //0xE
            "ingame_cmn_radio_menu",    //0xF
            "ingame_cmn_pause_menu",    //0x10
        };
    }

    for (int i = 0; i < *g_SteamAPI.iNumberOfControllers; ++i)
    {
        InputHandle_t handle = controllerHandles[i];
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

        if ((gamepadIndex == -1) || (eGameType & MG && gamepadIndex == 4) )
        {
            spdlog::info("SteamInput: Controller #{} | Type: {} | Handle: {} | Status: Good", i + 1, typeStr, handle);
        }
        else
        {
            spdlog::error("-------------------    ERROR     ----------------------");
            spdlog::error("SteamInput: Controller #{} | Type: {} | Handle: {} | Status: ERROR: Gamepad Handler is NOT correct - {}", i + 1, typeStr, handle, gamepadIndex);
            spdlog::error("SteamInput: Gamepad detected using incorrect input handler (ie Steam Input drivers.)");
            spdlog::error("SteamInput: This indicates a Steam Input / OS level game controller driver conflict.");
            spdlog::error("SteamInput: It's been reported that deleting the \"controller_base\" folder from your main Steam directory & then restarting Steam will resolve this issue.");
            spdlog::error("-------------------    ERROR     ----------------------");
        }

        InputActionSetHandle_t activeActionSet = steamInput->GetCurrentActionSet(handle);

        auto it = actionSetNames.find(activeActionSet);
        if (it != actionSetNames.end())
        {
            spdlog::info("SteamInput: Controller #{} | Active Action Set: {}", i + 1, it->second);
        }
        else
        {
            std::stringstream ss;
            ss << "0x" << std::hex << std::uppercase << (uint64_t)activeActionSet;
            spdlog::info("SteamInput: Controller #{} | Active Action Set Handle: {}", i + 1, ss.str());
        }

        bool bHasUnboundButtons = false;
        for (const std::string& actionName : actionNames)
        {
            InputDigitalActionHandle_t actionHandle = steamInput->GetDigitalActionHandle(actionName.c_str());
            if (actionHandle == 0)
            {
                spdlog::warn("SteamInput: Action '{}' not bound for Controller #{}", actionName, i + 1);
                bHasUnboundButtons = true;
                continue;
            }

            EInputActionOrigin origins[STEAM_INPUT_MAX_ORIGINS] = {};
            int originCount = steamInput->GetDigitalActionOrigins(handle, activeActionSet, actionHandle, origins);

            for (int j = 0; j < originCount; ++j)
            {
                const char* originName = steamInput->GetStringForActionOrigin(origins[j]);
                spdlog::info(
                    "SteamInput: Controller #{} | Action (0x{:X}) '{}' bound to: {}",
                    i + 1,
                    actionHandle,
                    actionName,
                    originName ? originName : "Unknown"
                );
            }
        }

        if (bHasUnboundButtons)
        {
            spdlog::warn("-------------------    WARNING     ----------------------");
            spdlog::warn("SteamInput: One or more actions are unbound for Controller #{}. Please check your Steam Input configuration for this game.", i + 1);
            spdlog::warn("-------------------    WARNING     ----------------------");

        }
    }
    spdlog::info("SteamInput: Initialization complete.");
    AfterSteamInputInitialized();
}
