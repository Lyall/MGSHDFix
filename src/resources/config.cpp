#include "common.hpp"
#include "config.hpp"

#include <inipp/inipp.h>

#include "aiming_after_equip.hpp"
#include "aiming_full_tilt.hpp"
#include "input_handler.hpp"
#include "intro_skip.hpp"
#include "line_scaling.hpp"
#include "logging.hpp"
#include "mute_warning.hpp"
#include "pause_on_focus_loss.hpp"
#include "steamworks_api.hpp"
#include "stereo_audio.hpp"
//#include "texture_buffer_size.hpp"
#include "cpu_core_limit.hpp"
#include "distance_culling.hpp"
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


#include "config_keys.hpp"
void Config::Read()
{
    std::filesystem::path sConfigFile = sFixName + ".settings";

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
    ConfigHelper::getValue(ini, ConfigKeys::VerboseLogging_Section, ConfigKeys::VerboseLogging_Setting, g_Logging.bVerboseLogging);

    ConfigHelper::getValue(ini, ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting, bOutputResolution);
    ConfigHelper::getValue(ini, ConfigKeys::WindowWidth_Section, ConfigKeys::WindowWidth_Setting, iOutputResX);
    ConfigHelper::getValue(ini, ConfigKeys::WindowHeight_Section, ConfigKeys::WindowHeight_Setting, iOutputResY);
    ConfigHelper::getValue(ini, ConfigKeys::WindowedMode_Section, ConfigKeys::WindowedMode_Setting, bWindowedMode);
    ConfigHelper::getValue(ini, ConfigKeys::BorderlessWindowed_Section, ConfigKeys::BorderlessWindowed_Setting, bBorderlessMode);

    ConfigHelper::getValue(ini, ConfigKeys::RenderScaleWidth_Section, ConfigKeys::RenderScaleWidth_Setting, iInternalResX);
    ConfigHelper::getValue(ini, ConfigKeys::RenderScaleHeight_Section, ConfigKeys::RenderScaleHeight_Setting, iInternalResY);

    ConfigHelper::getValue(ini, ConfigKeys::AnisotropicFiltering_Section, ConfigKeys::AnisotropicFiltering_Setting, iAnisotropicFiltering);

    ConfigHelper::getValue(ini, ConfigKeys::DisableTextureFiltering_Section, ConfigKeys::DisableTextureFiltering_Setting, bDisableTextureFiltering);

    ConfigHelper::getValue(ini, ConfigKeys::FramebufferFix_Section, ConfigKeys::FramebufferFix_Setting, bFramebufferFix);

    ConfigHelper::getValue(ini, ConfigKeys::LauncherJumpStart_Section, ConfigKeys::LauncherJumpStart_Setting, bLauncherJumpStart);

    ConfigHelper::getValue(ini, ConfigKeys::SkipIntroLogos_Section, ConfigKeys::SkipIntroLogos_Setting, g_IntroSkip.isEnabled);
    ConfigHelper::getValue(ini, ConfigKeys::ForceStereoAudio_Section, ConfigKeys::ForceStereoAudio_Setting, g_StereoAudioFix.isEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::EnablePauseOnFocusLoss_Section, ConfigKeys::EnablePauseOnFocusLoss_Setting, g_PauseOnFocusLoss.bPauseOnFocusLoss);
    ConfigHelper::getValue(ini, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Section, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Setting, g_PauseOnFocusLoss.bFixAltTabBugs);

    ConfigHelper::getValue(ini, ConfigKeys::MuteWarning_Section, ConfigKeys::MuteWarning_Setting, g_MuteWarning.bEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::CheckForUpdates_Section, ConfigKeys::CheckForUpdates_Setting, bShouldCheckForUpdates);
    ConfigHelper::getValue(ini, ConfigKeys::UpdateConsoleNotifications_Section, ConfigKeys::UpdateConsoleNotifications_Setting, bConsoleUpdateNotifications);

    ConfigHelper::getValue(ini, ConfigKeys::AchievementPersistence_Section, ConfigKeys::AchievementPersistence_Setting, g_StatPersistence.bAchievementPersistenceEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::ResetAllAchievements_Section, ConfigKeys::ResetAllAchievements_Setting, g_SteamAPI.bResetAchievements);

    ConfigHelper::getValue(ini, ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting, g_KeepAimingAfterFiring.bAlwaysKeepAiming);
    ConfigHelper::getValue(ini, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Section, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Setting, g_KeepAimingAfterFiring.bKeepAimingInFirstPerson);
    ConfigHelper::getValue(ini, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Section, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Setting, g_KeepAimingAfterFiring.bKeepAimingOnLockOn);

    ConfigHelper::getValue(ini, ConfigKeys::FixAimingAfterEquip_Section, ConfigKeys::FixAimingAfterEquip_Setting, g_FixAimAfterEquip.bEnabled);
    ConfigHelper::getValue(ini, ConfigKeys::FixAimingFullTilt_Section, ConfigKeys::FixAimingFullTilt_Setting, g_FixAimingFullTilt.bEnabled);

    std::string sShouldWearSunglasses;
    ConfigHelper::getValue(ini, ConfigKeys::MGS2Sunglasses_Section, ConfigKeys::MGS2Sunglasses_Setting, sShouldWearSunglasses);
    if (sShouldWearSunglasses != ConfigKeys::MGS2Sunglasses_Option_Normal && sShouldWearSunglasses != ConfigKeys::MGS2Sunglasses_Option_Always && sShouldWearSunglasses != ConfigKeys::MGS2Sunglasses_Option_Never)
    {
        spdlog::error("Invalid config value for MGS2 Sunglasses: {}", sShouldWearSunglasses);
        Logging::ShowConsole();
        std::cout << "Invalid config value for MGS2 Sunglasses: " << sShouldWearSunglasses << std::endl;
        return FreeLibraryAndExitThread(baseModule, 1);
    }
    if (sShouldWearSunglasses != ConfigKeys::MGS2Sunglasses_Option_Normal)
    {
        g_MGS2Sunglasses.bEnabled = true;
        if (sShouldWearSunglasses == ConfigKeys::MGS2Sunglasses_Option_Always)
        {
            g_MGS2Sunglasses.bAlwaysWearingSunglasses = true;
        }
    }

    ConfigHelper::getValue(ini, ConfigKeys::OverrideMouseSensitivity_Section, ConfigKeys::OverrideMouseSensitivity_Setting, bMouseSensitivity);
    ConfigHelper::getValue(ini, ConfigKeys::MouseSensitivity_XMultiplier_Section, ConfigKeys::MouseSensitivity_XMultiplier_Setting, fMouseSensitivityXMulti);
    ConfigHelper::getValue(ini, ConfigKeys::MouseSensitivity_YMultiplier_Section, ConfigKeys::MouseSensitivity_YMultiplier_Setting, fMouseSensitivityYMulti);

    ConfigHelper::getValue(ini, ConfigKeys::DisableMouseCursor_Section, ConfigKeys::DisableMouseCursor_Setting, bDisableCursor);

    ConfigHelper::getValue(ini, ConfigKeys::FixAspectRatio_Section, ConfigKeys::FixAspectRatio_Setting, bAspectFix);
    ConfigHelper::getValue(ini, ConfigKeys::FixHUD_Section, ConfigKeys::FixHUD_Setting, bHUDFix);
    ConfigHelper::getValue(ini, ConfigKeys::FixFOV_Section, ConfigKeys::FixFOV_Setting, bFOVFix);

    ConfigHelper::getValue(ini, ConfigKeys::SkipLauncher_Section, ConfigKeys::SkipLauncher_Setting, bLauncherConfigSkipLauncher);
    ConfigHelper::getValue(ini, ConfigKeys::CPUCoreLimit_Section, ConfigKeys::CPUCoreLimit_Setting, g_CPUCoreLimitFix.bEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::DistanceCullingGrass_Section, ConfigKeys::DistanceCullingGrass_Setting, g_DistanceCulling.bOverrideGrass);

    // Read launcher settings from ini
    std::string sLauncherConfigCtrlType = *std::next(kLauncherConfigCtrlTypes.begin(), 5);
    std::string sLauncherConfigRegion = *std::next(kLauncherConfigRegions.begin(), 0);
    std::string sLauncherConfigLanguage = *std::next(kLauncherConfigLanguages.begin(), 0);
    ConfigHelper::getValue(ini, ConfigKeys::CtrlType_Section, ConfigKeys::CtrlType_Setting, sLauncherConfigCtrlType);
    ConfigHelper::getValue(ini, ConfigKeys::Region_Section, ConfigKeys::Region_Setting, sLauncherConfigRegion);
    ConfigHelper::getValue(ini, ConfigKeys::Language_Section, ConfigKeys::Language_Setting, sLauncherConfigLanguage);
    ConfigHelper::getValue(ini, ConfigKeys::SkipLauncherMSXGame_Section, ConfigKeys::SkipLauncherMSXGame_Setting, sLauncherConfigMSXGame);
    ConfigHelper::getValue(ini, ConfigKeys::MSXWallType_Section, ConfigKeys::MSXWallType_Setting, iLauncherConfigMSXWallType);
    ConfigHelper::getValue(ini, ConfigKeys::MSXWallAlign_Section, ConfigKeys::MSXWallAlign_Setting, sLauncherConfigMSXWallAlign);
    iLauncherConfigCtrlType = Util::findStringInVector(sLauncherConfigCtrlType, kLauncherConfigCtrlTypes);
    iLauncherConfigRegion = Util::findStringInVector(sLauncherConfigRegion, kLauncherConfigRegions);
    iLauncherConfigLanguage = Util::findStringInVector(sLauncherConfigLanguage, kLauncherConfigLanguages);

    ConfigHelper::getValue(ini, ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Section, ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Setting, g_InputHandler.bCaptureInputsWhileAltTabbed);
    // Vector Line Fix
    if (eGameType & (MGS2 | MGS3))
    {
        ConfigHelper::getValue(ini, ConfigKeys::FixVectorRain_Section, ConfigKeys::FixVectorRain_Setting, g_VectorScalingFix.bFixRain);
        ConfigHelper::getValue(ini, ConfigKeys::FixVectorUI_Section, ConfigKeys::FixVectorUI_Setting, g_VectorScalingFix.bFixUI);
        if (g_VectorScalingFix.bFixRain || g_VectorScalingFix.bFixUI)
        {
            InputHandler::GetKeybind(ini, ConfigKeys::ToggleRainShader_Section, ConfigKeys::ToggleRainShader_Setting, g_VectorScalingFix.vkRainShaderToggle);
            InputHandler::GetKeybind(ini, ConfigKeys::ToggleUIShader_Section, ConfigKeys::ToggleUIShader_Setting, g_VectorScalingFix.vkUIShaderToggle);
            InputHandler::GetKeybind(ini, ConfigKeys::CycleWireframeMode_Section, ConfigKeys::CycleWireframeMode_Setting, g_VectorScalingFix.vkWireframeToggle);

            g_VectorScalingFix.bNeedsCompiler = true;
            inipp::get_value(ini.sections[ConfigKeys::VectorLineScale_Section], ConfigKeys::VectorLineScale_Setting, g_VectorScalingFix.iVectorLineScale);
            spdlog::info("Config Parse: Vector Effect Width: {} / {} pixels wide.", g_VectorScalingFix.iVectorLineScale, iInternalResY / g_VectorScalingFix.iVectorLineScale);
        }
    }

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
    spdlog::info("Config Parse: Launcher Jump Start: {}", bLauncherJumpStart);
    spdlog::info("Config Parse: Launcher - Skip Launcher: {}", bLauncherConfigSkipLauncher);
    spdlog::info("Config Parse: Launcher - Controller Glyphs: {} ( {} )", iLauncherConfigCtrlType, Util::GetUppercaseNameAtIndex(kLauncherConfigCtrlTypes, iLauncherConfigCtrlType));
    spdlog::info("Config Parse: Launcher - MSX Game: {}", sLauncherConfigMSXGame);
    spdlog::info("Config Parse: Launcher - Region: {} ({})", iLauncherConfigRegion, Util::GetUppercaseNameAtIndex(kLauncherConfigRegions, iLauncherConfigRegion));
    spdlog::info("Config Parse: Launcher - Language: {} ({})", iLauncherConfigLanguage, Util::GetUppercaseNameAtIndex(kLauncherConfigLanguages, iLauncherConfigLanguage));
    if (iLauncherConfigCtrlType == 6)
    {
        bIsPS2controltype = true;
        iLauncherConfigCtrlType = 1;
    }
    spdlog::info("Config Parse: Skip Intro Videos: {}", g_IntroSkip.isEnabled);
    spdlog::info("Config Parse: Pause On Focus Loss: {}", g_PauseOnFocusLoss.bPauseOnFocusLoss);
    spdlog::info("Config Parse: Cutscene Asset Loading Fix: {}", g_PauseOnFocusLoss.bFixAltTabBugs);
    spdlog::info("Config Parse: Force Stereo Audio: {}", g_StereoAudioFix.isEnabled);
    spdlog::info("Config Parse: Muted Audio Console Warnings: {}", g_MuteWarning.bEnabled);
    if (eGameType & (MGS2 | MGS3))
    {
        spdlog::info("Config Parse: Fix Vector Effect (Rain/Laser/Bullet Trail) Scaling: {}", g_VectorScalingFix.bFixRain);
        spdlog::info("Config Parse: Fix Vector Effect (UI / HUD) Scaling: {}", g_VectorScalingFix.bFixUI);
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
    spdlog::info("Config Parse: MGS2 Sunglasses - {}", (sShouldWearSunglasses == ConfigKeys::MGS2Sunglasses_Option_Normal ? "Normal" : (sShouldWearSunglasses == ConfigKeys::MGS2Sunglasses_Option_Always ? "Always Wearing Sunglasses" : "Never Wearing Sunglasses")));
    spdlog::info("Config Parse: Capture Inputs While Alt-Tabbed: {}", g_InputHandler.bCaptureInputsWhileAltTabbed);
    spdlog::info("Config Parse: CPU Core Limit Fix: {}", g_CPUCoreLimitFix.bEnabled);
    spdlog::info("Config Parse: Distance Culling Grass: {}", g_DistanceCulling.bOverrideGrass);

}
