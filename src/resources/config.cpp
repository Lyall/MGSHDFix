#include "common.hpp"
#include "config.hpp"

#include <inipp/inipp.h>

#include "aiming_after_equip.hpp"
#include "aiming_full_tilt.hpp"
#include "intro_skip.hpp"
#include "line_scaling.hpp"
#include "logging.hpp"
#include "mute_warning.hpp"
#include "pause_on_focus_loss.hpp"
#include "steamworks_api.hpp"
#include "stereo_audio.hpp"
#include "texture_buffer_size.hpp"
#include "version_checking.hpp"
#include "stat_persistence.hpp"
#include "keep_aiming_after_firing.hpp"
#include "mgs2_sunglasses.hpp"


// -----------------------------------------------------------------------------
// ConfigHelper: A type-safe, case-insensitive, error-checked INI config reader.
// Automatically logs missing/invalid values and exits the thread immediately.
// By Afevis/ShizCalev, 2025.
// -----------------------------------------------------------------------------

namespace ConfigHelper
{
    /// Terminates execution with a fatal INI error
    inline void FatalConfigError(const std::string& section, const std::string& key, const std::string& reason)
    {
        std::string message = "[" + sFixName +  " Config Helper] Failed to read config key '" + key +
            "' in section '" + section + "': " + reason;

        spdlog::error(message);
        spdlog::error("Please check that you're using the latest version's config file, and that there are no typos in it.");
        Logging::ShowConsole();
        std::cout << message << std::endl;
        std::cout << "Please check that you're using the latest version's config file, and that there are no typos in it." << std::endl;

        FreeLibraryAndExitThread(baseModule, 1);
    }

    /// Internal parsing helper
    template <typename T>
    bool TryParse(const std::string& str, T& out)
    {
        std::istringstream iss(str);
        return (iss >> std::boolalpha >> out) ? true : false;
    }

    /// Parses bool values with case-insensitivity and common boolean strings
    template <>
    inline bool TryParse<bool>(const std::string& str, bool& out)
    {
        std::string val = str;
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);
        if (val == "1" || val == "true" || val == "yes" || val == "on")
        {
            out = true;
            return true;
        }
        if (val == "0" || val == "false" || val == "no" || val == "off")
        {
            out = false;
            return true;
        }
        return false;
    }

    /// Generic value loader from INI with hard error on failure
    template <typename T>
    void getValue(const inipp::Ini<char>& ini, const std::string& section, const std::string& key, T& out)
    {
        auto secIt = ini.sections.find(section);
        if (secIt == ini.sections.end())
            FatalConfigError(section, key, "Section not found");

        const auto& keyvals = secIt->second;
        auto keyIt = keyvals.find(key);
        if (keyIt == keyvals.end())
            FatalConfigError(section, key, "Key not found");

        if (!TryParse<T>(keyIt->second, out))
            FatalConfigError(section, key, "Failed to parse value '" + keyIt->second + "'");
    }
}


void Config::Read()
{
    std::filesystem::path sConfigFile = sFixName + ".ini";

    std::ifstream iniFile((sExePath / sFixPath / sConfigFile).string());
    if (!iniFile)
    {
        spdlog::error("CONFIG ERROR: File not found: {}", (sExePath / sFixPath / sConfigFile).string());
        Logging::ShowConsole();
        std::cout << "" << sFixName << " v" << sFixVersion << " loaded." << std::endl;
        std::cout << "ERROR: Could not locate config file." << std::endl;
        std::cout << "ERROR: Make sure " << sConfigFile << " is located in " << sExePath / sFixPath << std::endl;
        return FreeLibraryAndExitThread(baseModule, 1);
    }

    spdlog::info("Config file: {}", (sExePath / sFixPath / sConfigFile).string());

    inipp::Ini<char> ini;
    ini.parse(iniFile);
    if (!ini.errors.empty())
    {
        spdlog::error("Error parsing ini file, encountered {} errors at these lines:", ini.errors.size());
        Logging::ShowConsole();
        std::cout << "Error parsing ini file, encountered " << ini.errors.size() << " errors at these lines:" << std::endl;
        for (auto err : ini.errors)
        {
            spdlog::error(err);
            std::cout << err << std::endl;
        }
    }

    // Grab desktop resolution
    DesktopDimensions = Util::GetPhysicalDesktopDimensions();

    // Read ini file
    ConfigHelper::getValue(ini, "Verbose Logging", "Enabled", g_Logging.bVerboseLogging);

    ConfigHelper::getValue(ini, "Output Resolution", "Enabled", bOutputResolution);
    ConfigHelper::getValue(ini, "Output Resolution", "Width", iOutputResX);
    ConfigHelper::getValue(ini, "Output Resolution", "Height", iOutputResY);
    ConfigHelper::getValue(ini, "Output Resolution", "Windowed", bWindowedMode);
    ConfigHelper::getValue(ini, "Output Resolution", "Borderless", bBorderlessMode);

    ConfigHelper::getValue(ini, "Internal Resolution", "Width", iInternalResX);
    ConfigHelper::getValue(ini, "Internal Resolution", "Height", iInternalResY);

    ConfigHelper::getValue(ini, "Anisotropic Filtering", "Samples", iAnisotropicFiltering);

    ConfigHelper::getValue(ini, "Disable Texture Filtering", "DisableTextureFiltering", bDisableTextureFiltering);

    ConfigHelper::getValue(ini, "Framebuffer Fix", "Enabled", bFramebufferFix);

    ConfigHelper::getValue(ini, "Launcher Config", "LauncherJumpStart", bLauncherJumpStart);

    ConfigHelper::getValue(ini, "Skip Intro Logos", "Enabled", g_IntroSkip.isEnabled);
    ConfigHelper::getValue(ini, "Force Stereo Audio", "Enabled", g_StereoAudioFix.isEnabled);

    ConfigHelper::getValue(ini, "Pause On Focus Loss", "Enabled", g_PauseOnFocusLoss.bPauseOnFocusLoss);
    ConfigHelper::getValue(ini, "Pause On Focus Loss", "SpeedrunnerBugfixOverride", g_PauseOnFocusLoss.bSpeedrunnerBugfixOverride);

    ConfigHelper::getValue(ini, "Mute Warning", "Enabled", g_MuteWarning.bEnabled);

    ConfigHelper::getValue(ini, "Update Notifications", "CheckForUpdates", bShouldCheckForUpdates);
    ConfigHelper::getValue(ini, "Update Notifications", "ConsoleNotifications", bConsoleUpdateNotifications);

    ConfigHelper::getValue(ini, "Achievement Persistence", "Enabled", g_StatPersistence.bAchievementPersistenceEnabled);

    ConfigHelper::getValue(ini, "Reset All Achievements", "Reset_All_Achievements", g_SteamAPI.bResetAchievements);

    ConfigHelper::getValue(ini, "Keep Aiming After Firing", "AlwaysKeepAiming", g_KeepAimingAfterFiring.bAlwaysKeepAiming);
    ConfigHelper::getValue(ini, "Keep Aiming After Firing", "While in First Person", g_KeepAimingAfterFiring.bKeepAimingInFirstPerson);
    ConfigHelper::getValue(ini, "Keep Aiming After Firing", "While Holding Lock On", g_KeepAimingAfterFiring.bKeepAimingOnLockOn);

    ConfigHelper::getValue(ini, "Fix Aiming After Equip", "Enabled", g_FixAimAfterEquip.bEnabled);
    ConfigHelper::getValue(ini, "Fix Aiming On Full Tilt", "Enabled", g_FixAimingFullTilt.bEnabled);


    std::string sShouldWearSunglasses;
    ConfigHelper::getValue(ini, "MGS2 Sunglasses", "ShouldWearSunglasses", sShouldWearSunglasses);
    std::transform(sShouldWearSunglasses.begin(), sShouldWearSunglasses.end(), sShouldWearSunglasses.begin(), ::tolower);
    if (sShouldWearSunglasses != "normal" && sShouldWearSunglasses != "always" && sShouldWearSunglasses != "never")
    {
        spdlog::error("Invalid config value for MGS2 Sunglasses: {}", sShouldWearSunglasses);
        Logging::ShowConsole();
        std::cout << "Invalid config value for MGS2 Sunglasses: " << sShouldWearSunglasses << std::endl;
        return FreeLibraryAndExitThread(baseModule, 1);
    }
    if (sShouldWearSunglasses != "normal")
    {
        g_MGS2Sunglasses.bEnabled = true;
        if (sShouldWearSunglasses == "always")
        {
            g_MGS2Sunglasses.bAlwaysWearingSunglasses = true;
        }
    }

    /*//INITIALIZE(Init_GammaShader());
    //INITIALIZE(g_DistanceCulling.Initialize());
    //INITIALIZE(g_MultiSampleAntiAliasing.Initialize());
    //INITIALIZE(g_Wireframe.Initialize());

    //INITIALIZE(g_ColorFilterFix.Initialize());*/

    //inipp::get_value(ini.sections["MG1 Custom Loading Screens"], "Enabled", g_MG1CustomLoadingScreens.isEnabled);
    ConfigHelper::getValue(ini, "Mouse Sensitivity", "Enabled", bMouseSensitivity);
    ConfigHelper::getValue(ini, "Mouse Sensitivity", "X Multiplier", fMouseSensitivityXMulti);
    ConfigHelper::getValue(ini, "Mouse Sensitivity", "Y Multiplier", fMouseSensitivityYMulti);

    ConfigHelper::getValue(ini, "Disable Mouse Cursor", "Enabled", bDisableCursor);

    ConfigHelper::getValue(ini, "Texture Buffer", "SizeMB", g_TextureBufferSize.iTextureBufferSizeMB);

    ConfigHelper::getValue(ini, "Fix Aspect Ratio", "Enabled", bAspectFix);
    ConfigHelper::getValue(ini, "Fix HUD", "Enabled", bHUDFix);
    ConfigHelper::getValue(ini, "Fix FOV", "Enabled", bFOVFix);

    ConfigHelper::getValue(ini, "Launcher Config", "SkipLauncher", bLauncherConfigSkipLauncher);


    // Read launcher settings from ini
    std::string sLauncherConfigCtrlType = "kbd";
    std::string sLauncherConfigRegion = "us";
    std::string sLauncherConfigLanguage = "en";
    ConfigHelper::getValue(ini, "Launcher Config", "CtrlType", sLauncherConfigCtrlType);
    ConfigHelper::getValue(ini, "Launcher Config", "Region", sLauncherConfigRegion);
    ConfigHelper::getValue(ini, "Launcher Config", "Language", sLauncherConfigLanguage);
    ConfigHelper::getValue(ini, "Launcher Config", "MSXGame", sLauncherConfigMSXGame);
    ConfigHelper::getValue(ini, "Launcher Config", "MSXWallType", iLauncherConfigMSXWallType);
    ConfigHelper::getValue(ini, "Launcher Config", "MSXWallAlign", sLauncherConfigMSXWallAlign);
    iLauncherConfigCtrlType = Util::findStringInVector(sLauncherConfigCtrlType, kLauncherConfigCtrlTypes);
    iLauncherConfigRegion = Util::findStringInVector(sLauncherConfigRegion, kLauncherConfigRegions);
    iLauncherConfigLanguage = Util::findStringInVector(sLauncherConfigLanguage, kLauncherConfigLanguages);



    // Log config parse
    spdlog::info("Config Parse: Verbose Logging: {}", g_Logging.bVerboseLogging);
    spdlog::info("Config Parse: Custom Output Resolution: {}", bOutputResolution);
    if (iOutputResX == 0 || iOutputResY == 0)
    {
        iOutputResX = DesktopDimensions.first;
        iOutputResY = DesktopDimensions.second;
    }
    spdlog::info("Config Parse: Output Resolution (X): {}", iOutputResX);
    spdlog::info("Config Parse: Output Resolution (Y): {}", iOutputResY);
    if (iInternalResX == 0 || iInternalResY == 0)
    {
        iInternalResX = iOutputResX;
        iInternalResY = iOutputResY;
    }
    spdlog::info("Config Parse: Internal Resolution (X): {}", iInternalResX);
    spdlog::info("Config Parse: Internal Resolution (Y): {}", iInternalResY);
    spdlog::info("Config Parse: Windowed Mode: {}", bWindowedMode);
    spdlog::info("Config Parse: Borderless Mode: {}", bBorderlessMode);
    spdlog::info("Config Parse: Fix Ultrawide Framebuffer: {}", bFramebufferFix);
    spdlog::info("Config Parse: Fix Ultrawide Aspect Ratio: {}", bAspectFix);
    spdlog::info("Config Parse: Fix Ultrawide HUD: {}", bHUDFix);
    spdlog::info("Config Parse: Fix Ultrawide FOV: {}", bFOVFix);
    spdlog::info("Config Parse: Texture Buffer Size (PER TEXTURE): {}MB", g_TextureBufferSize.iTextureBufferSizeMB); //g_TextureBufferSize
    spdlog::info("Config Parse: Anisotropic Filtering Level: {}", iAnisotropicFiltering);
    if (iAnisotropicFiltering < 0 || iAnisotropicFiltering > 16)
    {
        iAnisotropicFiltering = std::clamp(iAnisotropicFiltering, 0, 16);
        spdlog::info("Config Parse: Anisotropic Filtering value invalid, clamped to {}", iAnisotropicFiltering);
    }
    spdlog::info("Config Parse: Disable Texture Filtering: {}", bDisableTextureFiltering);
    spdlog::info("Config Parse: Disable Cursor Icon: {}", bDisableCursor);
    spdlog::info("Config Parse: Mouse Sensitivity: {}", bMouseSensitivity);
    spdlog::info("Config Parse: Mouse Sensitivity X Multiplier: {}", fMouseSensitivityXMulti);
    spdlog::info("Config Parse: Mouse Sensitivity Y Multiplier: {}", fMouseSensitivityYMulti);


    //spdlog::info("Config Parse: bMG1CustomLoadingScreens: {}", g_MG1CustomLoadingScreens.isEnabled);

    spdlog::info("Config Parse: Launcher Jump Start: {}", bLauncherJumpStart);

    spdlog::info("Config Parse: Launcher - Skip Launcher: {}", bLauncherConfigSkipLauncher);
    spdlog::info("Config Parse: Launcher - Controller Glyphs: {} ( {} )", iLauncherConfigCtrlType, Util::GetUppercaseNameAtIndex(kLauncherConfigCtrlTypes, iLauncherConfigCtrlType));
    spdlog::info("Config Parse: Launcher - MSX Game: {}", sLauncherConfigMSXGame);
    spdlog::info("Config Parse: Launcher - Region: {} ({})", iLauncherConfigRegion, Util::GetUppercaseNameAtIndex(kLauncherConfigRegions, iLauncherConfigRegion));
    spdlog::info("Config Parse: Launcher - Language: {} ({})", iLauncherConfigLanguage, Util::GetUppercaseNameAtIndex(kLauncherConfigLanguages, iLauncherConfigLanguage));
    if (std::string ps2Str = "ps2"; (iLauncherConfigCtrlType == Util::findStringInVector(ps2Str, kLauncherConfigCtrlTypes)))
    {
        bIsPS2controltype = true;
        ps2Str = "ps4";
        iLauncherConfigCtrlType = Util::findStringInVector(ps2Str, kLauncherConfigCtrlTypes);
    }
    spdlog::info("Config Parse: Skip Intro Videos: {}", g_IntroSkip.isEnabled);
    spdlog::info("Config Parse: Pause On Focus Loss: {}", g_PauseOnFocusLoss.bPauseOnFocusLoss);
    spdlog::info("Config Parse: Cutscene Asset Loading Fix - Speedrunner Override: {}", g_PauseOnFocusLoss.bSpeedrunnerBugfixOverride);

    spdlog::info("Config Parse: Force Stereo Audio: {}", g_StereoAudioFix.isEnabled);
    spdlog::info("Config Parse: Muted Audio Console Warnings: {}", g_MuteWarning.bEnabled);
    if (eGameType & (MGS2 | MGS3))
    {
        ConfigHelper::getValue(ini, "Vector Line Fix", "Enabled", g_VectorScalingFix.bEnableVectorLineFix);
        spdlog::info("Config Parse: Fix Vector Effect (Rain) Scaling: {}", g_VectorScalingFix.bEnableVectorLineFix);
        if (g_VectorScalingFix.bEnableVectorLineFix)
        {
            g_VectorScalingFix.bNeedsCompiler = true; // Set this during config so the compiler can be released via D3D11Hooks::UnloadCompilerCheck once it's no longer needed.
            inipp::get_value(ini.sections["Vector Line Fix"], "Line Scale", g_VectorScalingFix.iVectorLineScale);
            spdlog::info("Config Parse: Vector Effect Width: {} / {} pixels wide.", g_VectorScalingFix.iVectorLineScale, iInternalResY / g_VectorScalingFix.iVectorLineScale);
        }
    }

    spdlog::info("Cofig Parse: Check for mod updates: {}", bShouldCheckForUpdates);
    if (bShouldCheckForUpdates)
    {
        spdlog::info("Cofig Parse: Mod update console notifications: {}", bConsoleUpdateNotifications);
    }

    spdlog::info("Config Parse: Achievement Persistence: {}", g_StatPersistence.bAchievementPersistenceEnabled);
    spdlog::info("Config Parse: Reset Achievements: {}", g_SteamAPI.bResetAchievements);

    if (g_KeepAimingAfterFiring.bAlwaysKeepAiming)
    {
        spdlog::info("Config Parse: Keep Aiming After Firing - Always Keep Aiming: Enabled");
    }
    else if (g_KeepAimingAfterFiring.bKeepAimingInFirstPerson || g_KeepAimingAfterFiring.bKeepAimingOnLockOn)
    {
        spdlog::info("Config Parse: Keep Aiming After Firing - While Holding R1: {}", g_KeepAimingAfterFiring.bKeepAimingInFirstPerson);
        spdlog::info("Config Parse: Keep Aiming After Firing - While Holding L1: {}", g_KeepAimingAfterFiring.bKeepAimingOnLockOn);
    }
    else
    {
        spdlog::info("Config Parse: Keep Aiming After Firing - Always Keep Aiming: Disabled");
    }



    spdlog::info("Config Parse: Fix Aiming After Equip: {}", g_FixAimAfterEquip.bEnabled);
    spdlog::info("Config Parse: Fix Aiming Full Tilt: {}", g_FixAimingFullTilt.bEnabled);

    spdlog::info("Config Parse: MGS2 Sunglasses - {}", (sShouldWearSunglasses == "normal" ? "Normal" : (sShouldWearSunglasses == "always" ? "Always Wearing Sunglasses" : "Never Wearing Sunglasses")));

}
