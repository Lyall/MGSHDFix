#include "stdafx.h"
#include "common.hpp"
#include "steamworks_api.hpp"
#include "logging.hpp"


#pragma warning(push)
#pragma warning(disable:4828)
#include "config_keys.hpp"
#include "isteamuser.h"
#include "isteamuserstats.h"
#include "isteaminput.h"
#include "swap_menu_buttons.hpp"
#include "version.h"

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

    if (bDisableSteamAchievementUnlocking)
    {
        uint8_t* SteamUnlockAchievementsScanResult = Memory::PatternScan(baseModule, eGameType & MGS2 ? "41 43 48 5F 30 30 32 5F 25 30 33 64 00" : eGameType & MGS3 ? "41 43 48 5F 30 30 33 5F 25 30 33 64 00" : "41 43 48 5F 31 41 32 5F 25 30 33 64 00", "SteamAPI: Disable Achievement Unlocking");
        if (SteamUnlockAchievementsScanResult)
        {
            Memory::PatchBytes((uintptr_t)SteamUnlockAchievementsScanResult, "\x44", 1);
            spdlog::info("SteamAPI: Disabled achievement unlocking.");
            return;
        }
    }

}

#if !defined(RELEASE_BUILD)
#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <spdlog/spdlog.h>
#include <string>
#include <map>
#include <vector>
#include <regex>
#include <algorithm>

#pragma comment(lib, "setupapi.lib")

// Helper to query registry-based device properties
std::string GetDeviceRegistryProperty(HDEVINFO hDevInfo, SP_DEVINFO_DATA& devInfoData, DWORD property)
{
    char buffer[512];
    DWORD size = 0;
    if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devInfoData, property, nullptr,
                                          (PBYTE)buffer, sizeof(buffer), &size))
    {
        return std::string(buffer);
    }
    return "unknown";
}

// Check if a device is enabled (driver started)
bool IsDeviceEnabled(SP_DEVINFO_DATA& devInfoData)
{
    ULONG status = 0, problem = 0;
    if (CM_Get_DevNode_Status(&status, &problem, devInfoData.DevInst, 0) == CR_SUCCESS)
    {
        return (status & DN_STARTED) != 0;
    }
    return false;
}

// Sanitize names like "BillyJoe's ..." -> "[CENSORED]'s ..."
std::string SanitizeOwnerName(const std::string& input)
{
    static std::regex pattern(R"(([^']+)'s )");
    return std::regex_replace(input, pattern, "[CENSORED]'s ");
}

bool IsValidProvider(const std::string& provider)
{
    if (provider.empty() || provider == "unknown")
        return false;

    std::string lower = provider;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // kill both "[Standard...]" and "(Standard...)"
    if (lower.rfind("[standard", 0) == 0 || lower.rfind("(standard", 0) == 0)
        return false;

    return true;
}


void LogEnabledDevicesGrouped()
{
    HDEVINFO hDevInfo = SetupDiGetClassDevsA(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        spdlog::error("SetupDiGetClassDevs failed.");
        return;
    }

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    // Nested map: Class -> Bus -> Devices
    std::map<std::string, std::map<std::string, std::vector<std::string>>> groups;

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i)
    {
        if (!IsDeviceEnabled(devInfoData))
            continue; // skip disabled devices

        std::string cls = GetDeviceRegistryProperty(hDevInfo, devInfoData, SPDRP_CLASS);
        std::string bus = GetDeviceRegistryProperty(hDevInfo, devInfoData, SPDRP_ENUMERATOR_NAME);

        std::string friendly = GetDeviceRegistryProperty(hDevInfo, devInfoData, SPDRP_FRIENDLYNAME);
        if (friendly == "unknown")
            friendly = GetDeviceRegistryProperty(hDevInfo, devInfoData, SPDRP_DEVICEDESC);

        friendly = SanitizeOwnerName(friendly);

        // Bus reported device description
        std::string busReportedDesc = GetDeviceRegistryProperty(hDevInfo, devInfoData, SPDRP_DEVICEDESC);

        // Manufacturer / provider
        std::string provider = GetDeviceRegistryProperty(hDevInfo, devInfoData, SPDRP_MFG);

        // Build final string
        std::string formatted = friendly;
        if (!busReportedDesc.empty() && busReportedDesc != "unknown" && busReportedDesc != friendly)
        {
            formatted += " - " + busReportedDesc;
        }
        if (IsValidProvider(provider))
        {
            formatted += " [" + provider + "]";
        }

        groups[cls][bus].push_back(formatted);
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    // Print grouped results (sorted, but no deduplication)
    for (auto& [cls, busMap] : groups)
    {
        spdlog::info("[{}]", cls);

        for (auto& [bus, devices] : busMap)
        {
            std::sort(devices.begin(), devices.end());
            spdlog::info("\t[{}]", bus);

            for (auto& dev : devices)
                spdlog::info("\t\t- {}", dev);
        }
    }
}
#endif

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
    bool errored = false;
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
            spdlog::info("SteamInput: Controller #{} | Type: {} | Handle: {} | Controller -> Game Connection Status: Good", i + 1, typeStr, handle);
        }
        else
        {
            spdlog::error("-------------------    ERROR     ----------------------");
            spdlog::error("SteamInput: Controller #{} | Type: {} | Handle: {} | Status: ERROR: Controller -> Game Connection Status is NOT correct - {}", i + 1, typeStr, handle, gamepadIndex);
            spdlog::error("SteamInput: Gamepad detected using incorrect input handler (ie Steam Input drivers.)");
            spdlog::error("SteamInput: This indicates a Steam Input / OS level game controller driver conflict, or that Steam Input itself is disabled.");
            spdlog::error("SteamInput: It's been reported that deleting the \"controller_base\" folder from your main Steam directory & then restarting Steam can also resolve this issue if you're having continued trouble.");
            spdlog::error("SteamInput: If you require further assistance, you can find our Discord support channel at the Metal Gear Network Discord - #HDFix: {}", DISCORD_URL);
            spdlog::error("-------------------    ERROR     ----------------------");
            errored = true;
            continue;
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
                spdlog::error("SteamInput: Controller #{} | Game Action '{}' does not exist in the Steam Input manifest", i + 1, actionName);

                bHasUnboundButtons = true;
                continue;
            }

            EInputActionOrigin origins[STEAM_INPUT_MAX_ORIGINS] = {};
            int originCount = steamInput->GetDigitalActionOrigins(handle, activeActionSet, actionHandle, origins);

            if (originCount <= 0)
            {
                spdlog::error("SteamInput: Controller #{} | Game Action (0x{:X}) '{}' is UNBOUND in the active action set", i + 1, actionHandle, actionName);

                if ((SwapMenuButtons::force_menu_buttons != ConfigKeys::MenuButton_Option_Default) && ((actionName == "ingame_cmn_punch") || (actionName == "ingame_cmn_sneaking")))
                {
                    spdlog::error("-------------------    ERROR     ----------------------");
                    spdlog::error("SteamInput: '{}' is not bound in Steam Input.", actionName);
                    spdlog::error("SteamInput: This is typically caused by using an outdated community-made controller layout/profile that swapped the binds to keyboard inputs.");
                    spdlog::error("SteamInput: MGSHDFix's swapped menu buttons may not function correctly.");
                    spdlog::error("SteamInput: Rebind this Steam Input Game Action or switch Menu Buttons back to Default for proper functionality.");
                    spdlog::error("-------------------    ERROR     ----------------------");
                }

                bHasUnboundButtons = true;
                continue;
            }

            for (int j = 0; j < originCount; ++j)
            {
                const char* originName = steamInput->GetStringForActionOrigin(origins[j]);
                spdlog::info(
                    "SteamInput: Controller #{} | Game Action (0x{:X}) '{}' bound to: {}",
                    i + 1,
                    actionHandle,
                    actionName,
                    originName ? originName : "Unknown"
                );
            }
        }

        if (bHasUnboundButtons)
        {
            spdlog::error("-------------------    ERROR     ----------------------");
            spdlog::error("SteamInput: One or more Game Actions are not bound for Controller #{}. ", i + 1);
            spdlog::error("SteamInput: This usually indicates your Steam Input controller layout/profile is using \"GAMEPAD\" buttons instead of \"GAME ACTION\" buttons.");
            spdlog::error("SteamInput: Switching to a community-made controller layout/profile and then back to the default should resolve this issue.");
            spdlog::error("SteamInput: Alternatively, you can manually rebind your controller layout/profile to use the correct \"GAME ACTION\" buttons.");
            spdlog::error("SteamInput: If you require further assistance, you can find our Discord support channel at the Metal Gear Network Discord - #HDFix: {}", DISCORD_URL);
            spdlog::error("-------------------    ERROR     ----------------------");

        }
    }
#if !defined(RELEASE_BUILD)
    if (errored)
    {
        spdlog::error("-------------------    TROUBLESHOOTING LOGGING     ----------------------");
        spdlog::error("SteamInput: One or more controllers had errors. Logging all currently enabled devices.");
        LogEnabledDevicesGrouped();
        spdlog::error("-------------------    TROUBLESHOOTING LOGGING     ----------------------");

    }
#endif
    spdlog::info("SteamInput: Initialization complete.");
    AfterSteamInputInitialized();
}
