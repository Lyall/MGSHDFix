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
#include "config_keys.hpp"

// -----------------------------------------------------------------------------
// ConfigHelper: A type-safe, case-insensitive, error-checked INI config reader.
// Automatically logs missing/invalid values and exits the thread immediately.
// By Afevis/ShizCalev, 2025.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Config Logger: caches config values for sorted flush at the end
// -----------------------------------------------------------------------------
namespace ConfigLogger
{
    inline std::map<std::string, std::map<std::string, std::string>> cache;

    template <typename T>
    void Cache(const char* section, const char* setting, const T& value)
    {
        cache[section][setting] = fmt::format("{}", value);
    }

    void Flush()
    {
        spdlog::info("---------- Config Parse Results ----------");
        for (auto& sec : cache)
        {
            spdlog::info("[{}]", sec.first);
            for (auto& kv : sec.second)
            {
                spdlog::info("    {} = {}", kv.first, kv.second);
            }
        }
        spdlog::info("---------- End Config Parse ----------");
    }
}

#define LOG_CONFIG(section, setting, value) \
    ConfigLogger::Cache(section, setting, value)

namespace ConfigHelper
{
    inline void FatalConfigError(const std::string& section, const std::string& key, const std::string& reason)
    {
        std::string message = "[" + sFixName + " Config Helper] Failed to read config key '" + key +
            "' in section '" + section + "': " + reason;

        spdlog::error(message);
        spdlog::error("Please run the {} to update your settings file.", sFixName + " Config Tool");
        Logging::ShowConsole();
        std::cout << message << std::endl;
        std::cout << "Please run the " << sFixName + " Config Tool" << " to update your settings file." << std::endl;

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

    /// Specialization for std::string values (handles quotes)
    template <>
    inline void getValue<std::string>(const inipp::Ini<char>& ini, const std::string& section, const std::string& key, std::string& out)
    {
        auto secIt = ini.sections.find(section);
        if (secIt == ini.sections.end())
            FatalConfigError(section, key, "Section not found");

        const auto& keyvals = secIt->second;
        auto keyIt = keyvals.find(key);
        if (keyIt == keyvals.end())
            FatalConfigError(section, key, "Key not found");

        out = Util::StripQuotes(keyIt->second);
    }
}


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
        spdlog::error("Error parsing ini file, encountered {} errors:", ini.errors.size());
        Logging::ShowConsole();
        for (auto err : ini.errors)
        {
            spdlog::error(err);
            std::cout << err << std::endl;
        }
    }

    // Grab desktop resolution
    DesktopDimensions = Util::GetPhysicalDesktopDimensions();


    ConfigHelper::getValue(ini, ConfigKeys::VerboseLogging_Section, ConfigKeys::VerboseLogging_Setting, g_Logging.bVerboseLogging);
    LOG_CONFIG(ConfigKeys::VerboseLogging_Section, ConfigKeys::VerboseLogging_Setting, g_Logging.bVerboseLogging);

    ConfigHelper::getValue(ini, ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting, bOutputResolution);
    LOG_CONFIG(ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting, bOutputResolution);

    ConfigHelper::getValue(ini, ConfigKeys::WindowWidth_Section, ConfigKeys::WindowWidth_Setting, iOutputResX);
    ConfigHelper::getValue(ini, ConfigKeys::WindowHeight_Section, ConfigKeys::WindowHeight_Setting, iOutputResY);
    if (iOutputResX == 0 || iOutputResY == 0)
    {
        iOutputResX = DesktopDimensions.first;
        iOutputResY = DesktopDimensions.second;
    }
    LOG_CONFIG(ConfigKeys::WindowWidth_Section, ConfigKeys::WindowWidth_Setting, iOutputResX);
    LOG_CONFIG(ConfigKeys::WindowHeight_Section, ConfigKeys::WindowHeight_Setting, iOutputResY);

    ConfigHelper::getValue(ini, ConfigKeys::WindowedMode_Section, ConfigKeys::WindowedMode_Setting, bWindowedMode);
    ConfigHelper::getValue(ini, ConfigKeys::BorderlessWindowed_Section, ConfigKeys::BorderlessWindowed_Setting, bBorderlessMode);
    LOG_CONFIG(ConfigKeys::WindowedMode_Section, ConfigKeys::WindowedMode_Setting, bWindowedMode);
    LOG_CONFIG(ConfigKeys::BorderlessWindowed_Section, ConfigKeys::BorderlessWindowed_Setting, bBorderlessMode);

    ConfigHelper::getValue(ini, ConfigKeys::RenderScaleWidth_Section, ConfigKeys::RenderScaleWidth_Setting, iInternalResX);
    ConfigHelper::getValue(ini, ConfigKeys::RenderScaleHeight_Section, ConfigKeys::RenderScaleHeight_Setting, iInternalResY);
    if (iInternalResX == 0 || iInternalResY == 0)
    {
        iInternalResX = iOutputResX;
        iInternalResY = iOutputResY;
    }
    LOG_CONFIG(ConfigKeys::RenderScaleWidth_Section, ConfigKeys::RenderScaleWidth_Setting, iInternalResX);
    LOG_CONFIG(ConfigKeys::RenderScaleHeight_Section, ConfigKeys::RenderScaleHeight_Setting, iInternalResY);

    ConfigHelper::getValue(ini, ConfigKeys::AnisotropicFiltering_Section, ConfigKeys::AnisotropicFiltering_Setting, iAnisotropicFiltering);
    if (iAnisotropicFiltering < 0 || iAnisotropicFiltering > 16)
    {
        iAnisotropicFiltering = std::clamp(iAnisotropicFiltering, 0, 16);
        spdlog::warn("Config Parse: Anisotropic Filtering value invalid, clamped to {}", iAnisotropicFiltering);
    }
    LOG_CONFIG(ConfigKeys::AnisotropicFiltering_Section, ConfigKeys::AnisotropicFiltering_Setting, iAnisotropicFiltering);

    ConfigHelper::getValue(ini, ConfigKeys::DisableTextureFiltering_Section, ConfigKeys::DisableTextureFiltering_Setting, bDisableTextureFiltering);
    LOG_CONFIG(ConfigKeys::DisableTextureFiltering_Section, ConfigKeys::DisableTextureFiltering_Setting, bDisableTextureFiltering);

    ConfigHelper::getValue(ini, ConfigKeys::FramebufferFix_Section, ConfigKeys::FramebufferFix_Setting, bFramebufferFix);
    LOG_CONFIG(ConfigKeys::FramebufferFix_Section, ConfigKeys::FramebufferFix_Setting, bFramebufferFix);

    ConfigHelper::getValue(ini, ConfigKeys::LauncherJumpStart_Section, ConfigKeys::LauncherJumpStart_Setting, bLauncherJumpStart);
    LOG_CONFIG(ConfigKeys::LauncherJumpStart_Section, ConfigKeys::LauncherJumpStart_Setting, bLauncherJumpStart);

    ConfigHelper::getValue(ini, ConfigKeys::SkipIntroLogos_Section, ConfigKeys::SkipIntroLogos_Setting, g_IntroSkip.isEnabled);
    LOG_CONFIG(ConfigKeys::SkipIntroLogos_Section, ConfigKeys::SkipIntroLogos_Setting, g_IntroSkip.isEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::ForceStereoAudio_Section, ConfigKeys::ForceStereoAudio_Setting, g_StereoAudioFix.isEnabled);
    LOG_CONFIG(ConfigKeys::ForceStereoAudio_Section, ConfigKeys::ForceStereoAudio_Setting, g_StereoAudioFix.isEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::EnablePauseOnFocusLoss_Section, ConfigKeys::EnablePauseOnFocusLoss_Setting, g_PauseOnFocusLoss.bPauseOnFocusLoss);
    ConfigHelper::getValue(ini, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Section, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Setting, g_PauseOnFocusLoss.bFixAltTabBugs);
    LOG_CONFIG(ConfigKeys::EnablePauseOnFocusLoss_Section, ConfigKeys::EnablePauseOnFocusLoss_Setting, g_PauseOnFocusLoss.bPauseOnFocusLoss);
    LOG_CONFIG(ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Section, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Setting, g_PauseOnFocusLoss.bFixAltTabBugs);

    ConfigHelper::getValue(ini, ConfigKeys::MuteWarning_Section, ConfigKeys::MuteWarning_Setting, g_MuteWarning.bEnabled);
    LOG_CONFIG(ConfigKeys::MuteWarning_Section, ConfigKeys::MuteWarning_Setting, g_MuteWarning.bEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::CheckForUpdates_Section, ConfigKeys::CheckForUpdates_Setting, bShouldCheckForUpdates);
    ConfigHelper::getValue(ini, ConfigKeys::UpdateConsoleNotifications_Section, ConfigKeys::UpdateConsoleNotifications_Setting, bConsoleUpdateNotifications);
    LOG_CONFIG(ConfigKeys::CheckForUpdates_Section, ConfigKeys::CheckForUpdates_Setting, bShouldCheckForUpdates);
    LOG_CONFIG(ConfigKeys::UpdateConsoleNotifications_Section, ConfigKeys::UpdateConsoleNotifications_Setting, bConsoleUpdateNotifications);

    ConfigHelper::getValue(ini, ConfigKeys::AchievementPersistence_Section, ConfigKeys::AchievementPersistence_Setting, g_StatPersistence.bAchievementPersistenceEnabled);
    LOG_CONFIG(ConfigKeys::AchievementPersistence_Section, ConfigKeys::AchievementPersistence_Setting, g_StatPersistence.bAchievementPersistenceEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::ResetAllAchievements_Section, ConfigKeys::ResetAllAchievements_Setting, g_SteamAPI.bResetAchievements);
    LOG_CONFIG(ConfigKeys::ResetAllAchievements_Section, ConfigKeys::ResetAllAchievements_Setting, g_SteamAPI.bResetAchievements);

    ConfigHelper::getValue(ini, ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting, g_KeepAimingAfterFiring.bAlwaysKeepAiming);
    ConfigHelper::getValue(ini, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Section, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Setting, g_KeepAimingAfterFiring.bKeepAimingInFirstPerson);
    ConfigHelper::getValue(ini, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Section, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Setting, g_KeepAimingAfterFiring.bKeepAimingOnLockOn);
    LOG_CONFIG(ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting, g_KeepAimingAfterFiring.bAlwaysKeepAiming);
    LOG_CONFIG(ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Section, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Setting, g_KeepAimingAfterFiring.bKeepAimingInFirstPerson);
    LOG_CONFIG(ConfigKeys::KeepAimingAfterFiring_OnLockOn_Section, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Setting, g_KeepAimingAfterFiring.bKeepAimingOnLockOn);

    ConfigHelper::getValue(ini, ConfigKeys::FixAimingAfterEquip_Section, ConfigKeys::FixAimingAfterEquip_Setting, g_FixAimAfterEquip.bEnabled);
    ConfigHelper::getValue(ini, ConfigKeys::FixAimingFullTilt_Section, ConfigKeys::FixAimingFullTilt_Setting, g_FixAimingFullTilt.bEnabled);
    LOG_CONFIG(ConfigKeys::FixAimingAfterEquip_Section, ConfigKeys::FixAimingAfterEquip_Setting, g_FixAimAfterEquip.bEnabled);
    LOG_CONFIG(ConfigKeys::FixAimingFullTilt_Section, ConfigKeys::FixAimingFullTilt_Setting, g_FixAimingFullTilt.bEnabled);

    std::string sShouldWearSunglasses;
    ConfigHelper::getValue(ini, ConfigKeys::MGS2Sunglasses_Section, ConfigKeys::MGS2Sunglasses_Setting, sShouldWearSunglasses);
    if (sShouldWearSunglasses != ConfigKeys::MGS2Sunglasses_Option_Normal &&
        sShouldWearSunglasses != ConfigKeys::MGS2Sunglasses_Option_Always &&
        sShouldWearSunglasses != ConfigKeys::MGS2Sunglasses_Option_Never)
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
    LOG_CONFIG(ConfigKeys::MGS2Sunglasses_Section, ConfigKeys::MGS2Sunglasses_Setting, sShouldWearSunglasses);

    ConfigHelper::getValue(ini, ConfigKeys::OverrideMouseSensitivity_Section, ConfigKeys::OverrideMouseSensitivity_Setting, bMouseSensitivity);
    ConfigHelper::getValue(ini, ConfigKeys::MouseSensitivity_XMultiplier_Section, ConfigKeys::MouseSensitivity_XMultiplier_Setting, fMouseSensitivityXMulti);
    ConfigHelper::getValue(ini, ConfigKeys::MouseSensitivity_YMultiplier_Section, ConfigKeys::MouseSensitivity_YMultiplier_Setting, fMouseSensitivityYMulti);
    LOG_CONFIG(ConfigKeys::OverrideMouseSensitivity_Section, ConfigKeys::OverrideMouseSensitivity_Setting, bMouseSensitivity);
    LOG_CONFIG(ConfigKeys::MouseSensitivity_XMultiplier_Section, ConfigKeys::MouseSensitivity_XMultiplier_Setting, fMouseSensitivityXMulti);
    LOG_CONFIG(ConfigKeys::MouseSensitivity_YMultiplier_Section, ConfigKeys::MouseSensitivity_YMultiplier_Setting, fMouseSensitivityYMulti);

    ConfigHelper::getValue(ini, ConfigKeys::DisableMouseCursor_Section, ConfigKeys::DisableMouseCursor_Setting, bDisableCursor);
    LOG_CONFIG(ConfigKeys::DisableMouseCursor_Section, ConfigKeys::DisableMouseCursor_Setting, bDisableCursor);

    ConfigHelper::getValue(ini, ConfigKeys::FixAspectRatio_Section, ConfigKeys::FixAspectRatio_Setting, bAspectFix);
    ConfigHelper::getValue(ini, ConfigKeys::FixHUD_Section, ConfigKeys::FixHUD_Setting, bHUDFix);
    ConfigHelper::getValue(ini, ConfigKeys::FixFOV_Section, ConfigKeys::FixFOV_Setting, bFOVFix);
    LOG_CONFIG(ConfigKeys::FixAspectRatio_Section, ConfigKeys::FixAspectRatio_Setting, bAspectFix);
    LOG_CONFIG(ConfigKeys::FixHUD_Section, ConfigKeys::FixHUD_Setting, bHUDFix);
    LOG_CONFIG(ConfigKeys::FixFOV_Section, ConfigKeys::FixFOV_Setting, bFOVFix);

    ConfigHelper::getValue(ini, ConfigKeys::SkipLauncher_Section, ConfigKeys::SkipLauncher_Setting, bLauncherConfigSkipLauncher);
    ConfigHelper::getValue(ini, ConfigKeys::CPUCoreLimit_Section, ConfigKeys::CPUCoreLimit_Setting, g_CPUCoreLimitFix.bEnabled);
    LOG_CONFIG(ConfigKeys::SkipLauncher_Section, ConfigKeys::SkipLauncher_Setting, bLauncherConfigSkipLauncher);
    LOG_CONFIG(ConfigKeys::CPUCoreLimit_Section, ConfigKeys::CPUCoreLimit_Setting, g_CPUCoreLimitFix.bEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::DistanceCullingGrassAlways_Section, ConfigKeys::DistanceCullingGrassAlways_Setting, g_DistanceCulling.bForceGrassAlways);
    LOG_CONFIG(ConfigKeys::DistanceCullingGrassAlways_Section, ConfigKeys::DistanceCullingGrassAlways_Setting, g_DistanceCulling.bForceGrassAlways);
    if (g_DistanceCulling.bForceGrassAlways)
    {
        InputHandler::GetKeybind(ini, ConfigKeys::ToggleDistanceCullingGrass_Section, ConfigKeys::ToggleDistanceCullingGrass_Setting, g_DistanceCulling.vkForceGrassAlwaysToggle);
    }

    ConfigHelper::getValue(ini, ConfigKeys::DistanceCullingGrassScalar_Section, ConfigKeys::DistanceCullingGrassScalar_Setting, g_DistanceCulling.fGrassDistanceScalar);
    LOG_CONFIG(ConfigKeys::DistanceCullingGrassScalar_Section, ConfigKeys::DistanceCullingGrassScalar_Setting, g_DistanceCulling.fGrassDistanceScalar);

    // Launcher settings
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

    std::string ps2Type(ConfigKeys::ControllerType_PS2);
    if (iLauncherConfigCtrlType == Util::findStringInVector(ps2Type, kLauncherConfigCtrlTypes))
    {
        bIsPS2controltype = true;
        std::string ps4Type = ConfigKeys::ControllerType_PS4;
        iLauncherConfigCtrlType = Util::findStringInVector(ps4Type, kLauncherConfigCtrlTypes);
        LOG_CONFIG(ConfigKeys::CtrlType_Section, "PS2 Alias Applied", true);

    }

    LOG_CONFIG(ConfigKeys::CtrlType_Section, ConfigKeys::CtrlType_Setting, Util::GetNameAtIndex(kLauncherConfigCtrlTypes, iLauncherConfigCtrlType));
    LOG_CONFIG(ConfigKeys::Region_Section, ConfigKeys::Region_Setting, Util::GetNameAtIndex(kLauncherConfigRegions, iLauncherConfigRegion));
    LOG_CONFIG(ConfigKeys::Language_Section, ConfigKeys::Language_Setting, Util::GetNameAtIndex(kLauncherConfigLanguages, iLauncherConfigLanguage));
    LOG_CONFIG(ConfigKeys::SkipLauncherMSXGame_Section, ConfigKeys::SkipLauncherMSXGame_Setting, sLauncherConfigMSXGame);
    LOG_CONFIG(ConfigKeys::MSXWallType_Section, ConfigKeys::MSXWallType_Setting, iLauncherConfigMSXWallType);
    LOG_CONFIG(ConfigKeys::MSXWallAlign_Section, ConfigKeys::MSXWallAlign_Setting, sLauncherConfigMSXWallAlign);


    ConfigHelper::getValue(ini, ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Section, ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Setting, g_InputHandler.bCaptureInputsWhileAltTabbed);
    LOG_CONFIG(ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Section, ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Setting, g_InputHandler.bCaptureInputsWhileAltTabbed);

    // Vector Line Fix
    if (eGameType & (MGS2 | MGS3))
    {
        ConfigHelper::getValue(ini, ConfigKeys::FixVectorRain_Section, ConfigKeys::FixVectorRain_Setting, g_VectorScalingFix.bFixRain);
        ConfigHelper::getValue(ini, ConfigKeys::FixVectorUI_Section, ConfigKeys::FixVectorUI_Setting, g_VectorScalingFix.bFixUI);

        LOG_CONFIG(ConfigKeys::FixVectorRain_Section, ConfigKeys::FixVectorRain_Setting, g_VectorScalingFix.bFixRain);
        LOG_CONFIG(ConfigKeys::FixVectorUI_Section, ConfigKeys::FixVectorUI_Setting, g_VectorScalingFix.bFixUI);

        if (g_VectorScalingFix.bFixRain || g_VectorScalingFix.bFixUI)
        {
            InputHandler::GetKeybind(ini, ConfigKeys::ToggleRainShader_Section, ConfigKeys::ToggleRainShader_Setting, g_VectorScalingFix.vkRainShaderToggle);
            InputHandler::GetKeybind(ini, ConfigKeys::ToggleUIShader_Section, ConfigKeys::ToggleUIShader_Setting, g_VectorScalingFix.vkUIShaderToggle);
            InputHandler::GetKeybind(ini, ConfigKeys::CycleWireframeMode_Section, ConfigKeys::CycleWireframeMode_Setting, g_VectorScalingFix.vkWireframeToggle);

            g_VectorScalingFix.bNeedsCompiler = true;
            inipp::get_value(ini.sections[ConfigKeys::VectorLineScale_Section], ConfigKeys::VectorLineScale_Setting, g_VectorScalingFix.iVectorLineScale);
            LOG_CONFIG(ConfigKeys::VectorLineScale_Section, ConfigKeys::VectorLineScale_Setting, g_VectorScalingFix.iVectorLineScale);
        }
    }



    ConfigLogger::Flush();
}
