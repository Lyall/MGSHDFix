#include "stdafx.h"

#include "common.hpp"
#include "config.hpp"

#include "inipp/inipp.h"

#include "aiming_after_equip.hpp"
#include "aiming_full_tilt.hpp"
#include "mgs2_blood_stains.hpp"
#include "mgs2_scope_warp.hpp"
#include "mgs2_water_effects.hpp"
#include "mgs2_lens_droplets.hpp"
#include "mgs2_gas_haze.hpp"
#include "mgs2_demo_blur.hpp"
#include "mgs2_crossfade.hpp"
#include "input_handler.hpp"
#include "intro_skip.hpp"
#include "line_scaling.hpp"
#include "logging.hpp"
#include "mute_warning.hpp"
#include "optical_camo.hpp"
#include "pause_on_focus_loss.hpp"
#include "steamworks_api.hpp"
#include "stereo_audio.hpp"
//#include "texture_buffer_size.hpp"
#include "background_shuffle_warning.hpp"
#include "bugfix_mod_checks.hpp"
#include "check_gamesave_folder.hpp"
#include "cpu_core_limit.hpp"
#include "distance_culling.hpp"
#include "depth_of_field.hpp"
#include "version_checking.hpp"
#include "stat_persistence.hpp"
#include "keep_aiming_after_firing.hpp"
#include "mgs2_sunglasses.hpp"
#include "config_keys.hpp"
#include "corrupt_save_message.hpp"
#include "effect_speeds.hpp"
#include "mgs2_restore_dogtags.hpp"
#include "windows_fullscreen_optimization.hpp"
#include "custom_resolution_and_borderless.hpp"
#include "busy_loop_fix.hpp"
#include "mgs2_3rd_person_freecam.hpp"
#include "mgs2_restore_phone_jingle.hpp"
#include "mgs2_restore_action_level_selection.hpp"
#include "swap_menu_buttons.hpp"
#include "mgs2_msx_colonel.hpp"
#include "mgs2_difficulty.hpp"
#include "mgs2_hostage_type_easter_egg.hpp"
#include "mgs2_underwater_filter.hpp"
#include "original_camera_positions.hpp"
#include "adjustable_captions.hpp"
#include "color_correction.hpp"
#include "mgs2_first_person_view_mode.hpp"
#include "resolution_scaling_fixes.hpp"
#include "texture_live_swaps.hpp"
#include "mgs2_restore_sol_radar.hpp"
#include "mgs2_restore_elevator_glitch.hpp"
#include "mgs2_snake_tales_radar.hpp"
#include "mgs2_thermal_goggles.hpp"
#include "custom_player_name.hpp"
#include "game_funcs.hpp"
#include "mg1_display_scaling.hpp"
#include "mgs2_contrast_fix.hpp"
#include "mgs2_newscrconcentrateblur.hpp"
#include "mgs2_restore_dogtag_viewer.hpp"
#include "mgs2_vamp_punch_fix.hpp"
#include "mgs_smaa.hpp"
#include "mgs3_film_grain.hpp"
#include "playtime_fixes.hpp"

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


namespace
{

    std::string sReadableRegionName;
    std::string sReadableLanguageName;

    void ValidateLauncherRegionOptions()
    {
        const bool bIsMGLauncher = (game->ExeName == kGames.at(MG).ExeName);
        const bool bIsMGS2Launcher = (game->ExeName == kGames.at(MGS2).ExeName);
        const bool bIsMGS3Launcher = (game->ExeName == kGames.at(MGS3).ExeName);

        if (bIsMGLauncher)
        {
            usDatExists = true;
            jpDatExists = true;

        }
        else
        {
            usDatExists = bIsMGS2Launcher ? std::filesystem::exists(sExePath / "Misc" / "us" / "BP_SE.DAT") : std::filesystem::exists(sExePath / "fr" / "stage" / "mg2" / "cache" / "english.raw");
            jpDatExists = bIsMGS2Launcher ? std::filesystem::exists(sExePath / "Misc" / "jp" / "BP_SE.DAT") : std::filesystem::exists(sExePath / "jp" / "stage" / "mg2" / "cache" / "japanese.raw");
            spdlog::info("MGS 2 | MGS 3: Launcher Config: US/EU language pack installed: {}\t|\tJP language pack installed: {}", usDatExists ? "YES" : "NO", jpDatExists ? "YES" : "NO");
        }


        if (sSkipLauncherLanguage == "jp")
        {
            if (!jpDatExists)
            {
                std::string fallbackText = bIsMGS3Launcher ? "Region: North America & Language: English" : "Region: US/EU & Language: English";

                std::string msg = "Japanese language pack not installed (selected region: " + sSkipLauncherRegion + ", language: " + sSkipLauncherLanguage + "), defaulting to " + fallbackText + ".";
                spdlog::error("MGS 2 | MGS 3: Launcher Config: {}", msg);

                MessageBoxA(nullptr, msg.c_str(), "MGSHDFix Config Error", MB_OK | MB_ICONWARNING);

                sSkipLauncherRegion = bIsMGS3Launcher ? "us" : "eu";
                sSkipLauncherLanguage = "en";
            }
            else if (sSkipLauncherRegion != "jp")
            {
                spdlog::warn("MGS 2 | MGS 3: Launcher Config: Japanese language selected but region is set to {}, forcing region to Japan.", sSkipLauncherRegion);
                sSkipLauncherRegion = "jp";
            }
        }
        else //sSkipLauncherLanguage != "jp"
        {
            if (!usDatExists)
            {
                std::string errorMessage = "US / EU DAT language pack not installed (selected region: " + sSkipLauncherRegion + ", language: " + sSkipLauncherLanguage + "). Defaulting to Japanese.";

                spdlog::error("MGS 2 | MGS 3: Launcher Config: {}", errorMessage);
                MessageBoxA(nullptr, errorMessage.c_str(), "MGSHDFix Config Warning", MB_OK | MB_ICONWARNING);
                sSkipLauncherRegion = "jp";
                sSkipLauncherLanguage = "jp";
            }
            else
            {
                spdlog::info("MG | MG2 | MGS 2 | MGS 3: Launcher Config: Validating selected region/language pair (region: {}, language: {})", sSkipLauncherRegion, sSkipLauncherLanguage);
                if (!(bIsMGS3Launcher ? IsValidRegionLanguagePair(MGS3_LanguagePairs, sSkipLauncherRegion, sSkipLauncherLanguage) : IsValidRegionLanguagePair(MG1_MG2_MGS2_LanguagePairs, sSkipLauncherRegion, sSkipLauncherLanguage)))
                {
                    std::string errorMessage = "Invalid region/language pair selected (region: " + sSkipLauncherRegion + ", language: " + sSkipLauncherLanguage + ").";

                    if (bIsMGS3Launcher)
                    {
                        spdlog::error("MGS 3: Config Error: {}", errorMessage + " Defaulting to Region: North America & Language: English.");
                        //MessageBoxA(nullptr, errorMessage.append(" Defaulting to Region: North America & Language: English.").c_str(), "MGSHDFix Config Error", MB_OK | MB_ICONWARNING);
                        sSkipLauncherRegion = "us";
                        sSkipLauncherLanguage = "en";
                    }
                    else
                    {
                        spdlog::error("MG | MG2 | MGS2: Config Error: {}", errorMessage + " Defaulting to Region: US/EU & Language: English.");
                        //MessageBoxA(nullptr, errorMessage.append(" Defaulting to Region: US/EU & Language: English.").c_str(), "MGSHDFix Config Error", MB_OK | MB_ICONWARNING);
                        sSkipLauncherRegion = "eu";
                        sSkipLauncherLanguage = "en";
                    }

                }
                else
                {
                    spdlog::info("MG | MG2 | MGS 2 | MGS 3: Launcher Config: Valid region/language pair selected (region: {}, language: {})", sSkipLauncherRegion, sSkipLauncherLanguage);
                }
            }
        }


        if (bIsMGS3Launcher)
        {
            ResolveRegionLanguageNames(MGS3_LanguagePairs, sSkipLauncherRegion, sSkipLauncherLanguage, sReadableRegionName, sReadableLanguageName);
        }
        else
        {
            ResolveRegionLanguageNames(MG1_MG2_MGS2_LanguagePairs, sSkipLauncherRegion, sSkipLauncherLanguage, sReadableRegionName, sReadableLanguageName);

        }
    }
}

void Config::Read()
{
    std::filesystem::path sConfigFile = sFixName + ".settings";

    std::ifstream iniFile((sExePath / sFixPath / sConfigFile).string());
    if (!iniFile)
    {
        spdlog::error("CONFIG ERROR: File not found: {}", (sExePath / sFixPath / sConfigFile).string());
        spdlog::error("Make sure that you've run the {} (in your game's /plugins folder) to generate your settings file.", sFixName + " Config Tool");
        spdlog::error("and that {} is located in {}", sConfigFile.string(), (sExePath / sFixPath).string());
        Logging::ShowConsole();
        std::cout << "" << sFixName << " v" << sFixVersion << " loaded." << std::endl;
        std::cout << "ERROR: File not found: " << (sExePath / sFixPath / sConfigFile).string() << std::endl;
        std::cout << "ERROR: Make sure that you've run the " << sFixName + " Config Tool" << " (in your game's /plugins folder) to generate your settings file." << std::endl;
        std::cout << "ERROR: And that " << sConfigFile << " is located in " << sExePath / sFixPath << "\n" << std::endl;
        if (Util::IsSteamOS())
        {
            std::cout << "ERROR: When launching the MGSHDFix Config Tool.exe on SteamOS, a protontricks window will open.\n"
                "ERROR: Simply select ANY game that's in the list and hit OK.\n"
                "ERROR: The Config Tool will then open normally.\n"
                "\n"
                "ERROR: If you still experience difficulty launching the config tool on SteamOS, add it as a non-steam game and launch it once.\n"
                "ERROR: That will generate a new Wine prefix just for the config tool, allowing you to open it directly via protontricks in the future."
                "\n"
                "If you require further assistance, you can find our support channel at the Metal Gear Network Discord - #HDFix: " << DISCORD_URL << std::endl;
            spdlog::error("When launching the MGSHDFix Config Tool.exe on SteamOS, a protontricks window will open.");
            spdlog::error("Simply select ANY game that's in the list and hit OK.");
            spdlog::error("The Config Tool will then open normally.");
            spdlog::error("If you still experience difficulty launching the config tool on SteamOS, add it as a non-steam game and launch it once.");
            spdlog::error("That will generate a new Wine prefix just for the config tool, allowing you to open it directly via protontricks in the future.");
            spdlog::error("If you require further assistance, you can find our support channel at the Metal Gear Network Discord - #HDFix: {}", DISCORD_URL);
        }
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
    CustomResolutionAndBorderless::DesktopDimensions = Util::GetPhysicalDesktopDimensions();


    ConfigHelper::getValue(ini, ConfigKeys::VerboseLogging_Section, ConfigKeys::VerboseLogging_Setting, g_Logging.bVerboseLogging);
    LOG_CONFIG(ConfigKeys::VerboseLogging_Section, ConfigKeys::VerboseLogging_Setting, g_Logging.bVerboseLogging);


    ConfigHelper::getValue(ini, ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting, CustomResolutionAndBorderless::bOutputResolution);
    LOG_CONFIG(ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting, CustomResolutionAndBorderless::bOutputResolution);


    ConfigHelper::getValue(ini, ConfigKeys::MGS2_Thermal_Mode_Section, ConfigKeys::MGS2_Thermal_Mode_Setting, MGS2ThermalGoggles::bEnabled);
    LOG_CONFIG(ConfigKeys::MGS2_Thermal_Mode_Section, ConfigKeys::MGS2_Thermal_Mode_Setting, MGS2ThermalGoggles::bEnabled);
    if (MGS2ThermalGoggles::bEnabled)
    {
        InputHandler::GetKeybind(ini, ConfigKeys::MGS2_Thermal_Cycle_Hotkey_Section, ConfigKeys::MGS2_Thermal_Cycle_Hotkey_Setting, MGS2ThermalGoggles::vk_ToggleThermalGoggleColor);

        std::string sIRMode;
        ConfigHelper::getValue(ini, ConfigKeys::MGS2_Thermal_Default_Mode_Section, ConfigKeys::MGS2_Thermal_Default_Mode_Setting, sIRMode);
        if (sIRMode != ConfigKeys::MGS2_Thermal_Default_Mode_Option_Substance &&
            sIRMode != ConfigKeys::MGS2_Thermal_Default_Mode_Option_RedHot &&
            sIRMode != ConfigKeys::MGS2_Thermal_Default_Mode_Option_SplinterCell &&
            sIRMode != ConfigKeys::MGS2_Thermal_Default_Mode_Option_WhiteHot &&
            sIRMode != ConfigKeys::MGS2_Thermal_Default_Mode_Option_BlackHot)
        {
            spdlog::error("Invalid config value for {}: {}", ConfigKeys::MGS2_Thermal_Default_Mode_Setting, sIRMode);
            Logging::ShowConsole();
            std::cout << "Invalid config value for " << ConfigKeys::MGS2_Thermal_Default_Mode_Setting << ": " << sIRMode << std::endl;
            return FreeLibraryAndExitThread(baseModule, 1);
        }
        if (sIRMode == ConfigKeys::MGS2_Thermal_Default_Mode_Option_RedHot)
        {
            MGS2ThermalGoggles::g_irMode = MGS2ThermalGoggles::IRMode::RedHot;
        }
        else if (sIRMode == ConfigKeys::MGS2_Thermal_Default_Mode_Option_SplinterCell)
        {
            MGS2ThermalGoggles::g_irMode = MGS2ThermalGoggles::IRMode::SplinterCell;
        }
        else if (sIRMode == ConfigKeys::MGS2_Thermal_Default_Mode_Option_WhiteHot)
        {
            MGS2ThermalGoggles::g_irMode = MGS2ThermalGoggles::IRMode::WhiteHot;
        }
        else if (sIRMode == ConfigKeys::MGS2_Thermal_Default_Mode_Option_BlackHot)
        {
            MGS2ThermalGoggles::g_irMode = MGS2ThermalGoggles::IRMode::BlackHot;
        }
        // else Substance
        LOG_CONFIG(ConfigKeys::MGS2_Thermal_Default_Mode_Section, ConfigKeys::MGS2_Thermal_Default_Mode_Setting, sIRMode);
    }
        
    ConfigHelper::getValue(ini, ConfigKeys::WindowWidth_Section, ConfigKeys::WindowWidth_Setting, CustomResolutionAndBorderless::iOutputResX);
    ConfigHelper::getValue(ini, ConfigKeys::WindowHeight_Section, ConfigKeys::WindowHeight_Setting, CustomResolutionAndBorderless::iOutputResY);
    ConfigHelper::getValue(ini, ConfigKeys::RenderScaleWidth_Section, ConfigKeys::RenderScaleWidth_Setting, CustomResolutionAndBorderless::iInternalResX);
    ConfigHelper::getValue(ini, ConfigKeys::RenderScaleHeight_Section, ConfigKeys::RenderScaleHeight_Setting, CustomResolutionAndBorderless::iInternalResY);

    ConfigHelper::getValue(ini, ConfigKeys::WindowedMode_Section, ConfigKeys::WindowedMode_Setting, CustomResolutionAndBorderless::bWindowOrFullscreenMode);
    LOG_CONFIG(ConfigKeys::WindowedMode_Section, ConfigKeys::WindowedMode_Setting, CustomResolutionAndBorderless::bWindowOrFullscreenMode);
    CustomResolutionAndBorderless::bLimitToPrimaryMonitor = true;
    if (CustomResolutionAndBorderless::bOutputResolution)
    {
        if (CustomResolutionAndBorderless::bWindowOrFullscreenMode == ConfigKeys::BorderlessMode_Option_BorderlessFullscreen)
        {
            CustomResolutionAndBorderless::bMaximizeBorderless = true;
            CustomResolutionAndBorderless::bBorderlessMode = true;
            CustomResolutionAndBorderless::bWindowedMode = true;
            CustomResolutionAndBorderless::iOutputResX = 0;
            CustomResolutionAndBorderless::iOutputResY = 0;
        }
        else if (CustomResolutionAndBorderless::bWindowOrFullscreenMode == ConfigKeys::BorderlessMode_Option_BorderlessWindowed)
        {   
            CustomResolutionAndBorderless::bBorderlessMode = true;
            CustomResolutionAndBorderless::bWindowedMode = true;
        }
        else if (CustomResolutionAndBorderless::bWindowOrFullscreenMode == ConfigKeys::BorderlessMode_Option_Windowed)
        {
            CustomResolutionAndBorderless::bWindowedMode = true;
        }
        else if (CustomResolutionAndBorderless::bWindowOrFullscreenMode == ConfigKeys::BorderlessMode_Option_Fullscreen)
        {
            CustomResolutionAndBorderless::iOutputResX = 0;
            CustomResolutionAndBorderless::iOutputResY = 0;
        }
        else{
             spdlog::warn("Config Parse: Invalid value for {} in section {}, expected 'borderless', 'windowed', or 'fullscreen', got '{}'. Defaulting to fullscreen mode.", ConfigKeys::WindowedMode_Setting, ConfigKeys::WindowedMode_Section, CustomResolutionAndBorderless::bWindowOrFullscreenMode);
        }
    }

    if (CustomResolutionAndBorderless::iOutputResX == 0)
    {
        CustomResolutionAndBorderless::bUsingAutomaticOutputX = true;
        CustomResolutionAndBorderless::iOutputResX = CustomResolutionAndBorderless::DesktopDimensions.first;
    }
    if (CustomResolutionAndBorderless::iOutputResY == 0)
    {
        CustomResolutionAndBorderless::bUsingAutomaticOutputY = true;
        CustomResolutionAndBorderless::iOutputResY = CustomResolutionAndBorderless::DesktopDimensions.second;
    }
    CustomResolutionAndBorderless::iOriginalOutputResX = CustomResolutionAndBorderless::iOutputResX;
    CustomResolutionAndBorderless::iOriginalOutputResY = CustomResolutionAndBorderless::iOutputResY;
    LOG_CONFIG(ConfigKeys::WindowWidth_Section, ConfigKeys::WindowWidth_Setting, CustomResolutionAndBorderless::iOutputResX);
    LOG_CONFIG(ConfigKeys::WindowHeight_Section, ConfigKeys::WindowHeight_Setting, CustomResolutionAndBorderless::iOutputResY);



    if (CustomResolutionAndBorderless::iInternalResX == 0)
    {
        CustomResolutionAndBorderless::iInternalResX = CustomResolutionAndBorderless::iOutputResX;
    }
    if (CustomResolutionAndBorderless::iInternalResY == 0)
    {
        CustomResolutionAndBorderless::iInternalResY = CustomResolutionAndBorderless::iOutputResY;
    }
    if (CustomResolutionAndBorderless::bBorderlessMode || CustomResolutionAndBorderless::bWindowedMode)
    {
        CustomResolutionAndBorderless::iOutputResX = CustomResolutionAndBorderless::iInternalResX;
        CustomResolutionAndBorderless::iOutputResY = CustomResolutionAndBorderless::iInternalResY;
    }


    LOG_CONFIG(ConfigKeys::RenderScaleWidth_Section, ConfigKeys::RenderScaleWidth_Setting, CustomResolutionAndBorderless::iInternalResX);
    LOG_CONFIG(ConfigKeys::RenderScaleHeight_Section, ConfigKeys::RenderScaleHeight_Setting, CustomResolutionAndBorderless::iInternalResY);


    ConfigHelper::getValue(ini, ConfigKeys::MGS2_Increase_Shadow_Resolution_Section, ConfigKeys::MGS2_Increase_Shadow_Resolution_Setting, ResolutionScalingFixes::bIncreaseShadowResolution);
    LOG_CONFIG(ConfigKeys::MGS2_Increase_Shadow_Resolution_Section, ConfigKeys::MGS2_Increase_Shadow_Resolution_Setting, ResolutionScalingFixes::bIncreaseShadowResolution);


    ConfigHelper::getValue(ini, ConfigKeys::MGS2_LaserOriginFix_FixM9FPV_Section, ConfigKeys::MGS2_LaserOriginFix_FixM9FPV_Setting, ResolutionScalingFixes::bFixM92FPV);
    LOG_CONFIG(ConfigKeys::MGS2_LaserOriginFix_FixM9FPV_Section, ConfigKeys::MGS2_LaserOriginFix_FixM9FPV_Setting, ResolutionScalingFixes::bFixM92FPV);


    ConfigHelper::getValue(ini, ConfigKeys::AnisotropicFiltering_Section, ConfigKeys::AnisotropicFiltering_Setting, iAnisotropicFiltering);
    if (iAnisotropicFiltering < 0 || iAnisotropicFiltering > 16)
    {
        iAnisotropicFiltering = std::clamp(iAnisotropicFiltering, 0, 16);
        spdlog::warn("Config Parse: Anisotropic Filtering value invalid, clamped to {}", iAnisotropicFiltering);
    }
    LOG_CONFIG(ConfigKeys::AnisotropicFiltering_Section, ConfigKeys::AnisotropicFiltering_Setting, iAnisotropicFiltering);

    ConfigHelper::getValue(ini, ConfigKeys::DisableTextureFiltering_Section, ConfigKeys::DisableTextureFiltering_Setting, bDisableTextureFiltering);
    LOG_CONFIG(ConfigKeys::DisableTextureFiltering_Section, ConfigKeys::DisableTextureFiltering_Setting, bDisableTextureFiltering);

    ConfigHelper::getValue(ini, ConfigKeys::FramebufferFix_Section, ConfigKeys::FramebufferFix_Setting, CustomResolutionAndBorderless::bFramebufferFix);
    LOG_CONFIG(ConfigKeys::FramebufferFix_Section, ConfigKeys::FramebufferFix_Setting, CustomResolutionAndBorderless::bFramebufferFix);

    ConfigHelper::getValue(ini, ConfigKeys::LauncherJumpStart_Section, ConfigKeys::LauncherJumpStart_Setting, bLauncherJumpStart);
    LOG_CONFIG(ConfigKeys::LauncherJumpStart_Section, ConfigKeys::LauncherJumpStart_Setting, bLauncherJumpStart);

    ConfigHelper::getValue(ini, ConfigKeys::SkipIntroLogos_Section, ConfigKeys::SkipIntroLogos_Setting, g_IntroSkip.isEnabled);
    LOG_CONFIG(ConfigKeys::SkipIntroLogos_Section, ConfigKeys::SkipIntroLogos_Setting, g_IntroSkip.isEnabled);

    std::string sAudioSetting;
    ConfigHelper::getValue(ini, ConfigKeys::ForceStereoAudio_Section, ConfigKeys::ForceStereoAudio_Setting, sAudioSetting);
    if (sAudioSetting == ConfigKeys::ForceStereoAudio_Option_Stereo)
    {
        g_StereoAudioFix.isEnabled = true;
    }
    else if (sAudioSetting == ConfigKeys::ForceStereoAudio_Option_Surround)
    {
        g_StereoAudioFix.isEnabled = false;
    }
    else
    {
        spdlog::error("Invalid config value for {}: {}", ConfigKeys::ForceStereoAudio_Setting, sAudioSetting);
        spdlog::error("Defaulting to Stereo audio output.");
        spdlog::error("Please run the {} to update your settings file.", sFixName + " Config Tool");
        Logging::ShowConsole();
        std::cout << "Invalid config value for " << ConfigKeys::ForceStereoAudio_Setting << ": " << sAudioSetting << std::endl;
        std::cout << "Defaulting to Stereo audio output." << std::endl;
        std::cout << "Please run the " << sFixName << " Config Tool" << " to update your settings file." << std::endl;
        g_StereoAudioFix.isEnabled = true;
    }
    LOG_CONFIG(ConfigKeys::ForceStereoAudio_Section, ConfigKeys::ForceStereoAudio_Setting, sAudioSetting);

    ConfigHelper::getValue(ini, ConfigKeys::EnablePauseOnFocusLoss_Section, ConfigKeys::EnablePauseOnFocusLoss_Setting, g_PauseOnFocusLoss.bPauseOnFocusLoss);
    ConfigHelper::getValue(ini, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Section, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Setting, g_PauseOnFocusLoss.bFixAltTabBugs);
    LOG_CONFIG(ConfigKeys::EnablePauseOnFocusLoss_Section, ConfigKeys::EnablePauseOnFocusLoss_Setting, g_PauseOnFocusLoss.bPauseOnFocusLoss);
    LOG_CONFIG(ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Section, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Setting, g_PauseOnFocusLoss.bFixAltTabBugs);

    ConfigHelper::getValue(ini, ConfigKeys::MuteWarning_Section, ConfigKeys::MuteWarning_Setting, g_MuteWarning.bEnabled);
    LOG_CONFIG(ConfigKeys::MuteWarning_Section, ConfigKeys::MuteWarning_Setting, g_MuteWarning.bEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::FSRWarning_Section, ConfigKeys::FSRWarning_Setting, CustomResolutionAndBorderless::bEnableFSRWarning);
    LOG_CONFIG(ConfigKeys::FSRWarning_Section, ConfigKeys::FSRWarning_Setting, CustomResolutionAndBorderless::bEnableFSRWarning);

    ConfigHelper::getValue(ini, ConfigKeys::MissingBugfixModWarning_Section, ConfigKeys::MissingBugfixModWarning_Setting, BugfixMods::bEnableVisibleWarnings);
    LOG_CONFIG(ConfigKeys::MissingBugfixModWarning_Section, ConfigKeys::MissingBugfixModWarning_Setting, BugfixMods::bEnableVisibleWarnings);


    ConfigHelper::getValue(ini, ConfigKeys::WindowsSlideshowWarning_Section, ConfigKeys::WindowsSlideshowWarning_Setting, BackgroundShuffleWarning::bEnabled);
    LOG_CONFIG(ConfigKeys::WindowsSlideshowWarning_Section, ConfigKeys::WindowsSlideshowWarning_Setting, BackgroundShuffleWarning::bEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::CheckForUpdates_Section, ConfigKeys::CheckForUpdates_Setting, bShouldCheckForUpdates);
    ConfigHelper::getValue(ini, ConfigKeys::UpdateConsoleNotifications_Section, ConfigKeys::UpdateConsoleNotifications_Setting, bConsoleUpdateNotifications);
    LOG_CONFIG(ConfigKeys::CheckForUpdates_Section, ConfigKeys::CheckForUpdates_Setting, bShouldCheckForUpdates);
    LOG_CONFIG(ConfigKeys::UpdateConsoleNotifications_Section, ConfigKeys::UpdateConsoleNotifications_Setting, bConsoleUpdateNotifications);


    ConfigHelper::getValue(ini, ConfigKeys::DisableSteamAchievements_Section, ConfigKeys::DisableSteamAchievements_Setting, g_SteamAPI.bDisableSteamAchievementUnlocking);
    LOG_CONFIG(ConfigKeys::DisableSteamAchievements_Section, ConfigKeys::DisableSteamAchievements_Setting, g_SteamAPI.bDisableSteamAchievementUnlocking);
    if (g_SteamAPI.bDisableSteamAchievementUnlocking)
    {
        g_StatPersistence.bAchievementPersistenceEnabled = false;
    }
    else
    {
        ConfigHelper::getValue(ini, ConfigKeys::AchievementPersistence_Section, ConfigKeys::AchievementPersistence_Setting, g_StatPersistence.bAchievementPersistenceEnabled);
        LOG_CONFIG(ConfigKeys::AchievementPersistence_Section, ConfigKeys::AchievementPersistence_Setting, g_StatPersistence.bAchievementPersistenceEnabled);
    }

    ConfigHelper::getValue(ini, ConfigKeys::ResetAllAchievements_Section, ConfigKeys::ResetAllAchievements_Setting, g_SteamAPI.bResetAchievements);
    LOG_CONFIG(ConfigKeys::ResetAllAchievements_Section, ConfigKeys::ResetAllAchievements_Setting, g_SteamAPI.bResetAchievements);

    ConfigHelper::getValue(ini, ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting, g_KeepAimingAfterFiring.bAlwaysKeepAiming);
    ConfigHelper::getValue(ini, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Section, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Setting, g_KeepAimingAfterFiring.bKeepAimingInFirstPerson);
    ConfigHelper::getValue(ini, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Section, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Setting, g_KeepAimingAfterFiring.bKeepAimingOnLockOn);
    ConfigHelper::getValue(ini, ConfigKeys::KeepAimingAfterFiring_InFPSMode_Section, ConfigKeys::KeepAimingAfterFiring_InFPSMode_Setting, g_KeepAimingAfterFiring.bKeepAimingInFPSMode);
    LOG_CONFIG(ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting, g_KeepAimingAfterFiring.bAlwaysKeepAiming);
    LOG_CONFIG(ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Section, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Setting, g_KeepAimingAfterFiring.bKeepAimingInFirstPerson);
    LOG_CONFIG(ConfigKeys::KeepAimingAfterFiring_OnLockOn_Section, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Setting, g_KeepAimingAfterFiring.bKeepAimingOnLockOn);
    LOG_CONFIG(ConfigKeys::KeepAimingAfterFiring_InFPSMode_Section, ConfigKeys::KeepAimingAfterFiring_InFPSMode_Setting, g_KeepAimingAfterFiring.bKeepAimingInFPSMode);

    ConfigHelper::getValue(ini, ConfigKeys::FixAimingAfterEquip_Section, ConfigKeys::FixAimingAfterEquip_Setting, g_FixAimAfterEquip.bEnabled);
    LOG_CONFIG(ConfigKeys::FixAimingAfterEquip_Section, ConfigKeys::FixAimingAfterEquip_Setting, g_FixAimAfterEquip.bEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::FixAimingFullTilt_Section, ConfigKeys::FixAimingFullTilt_Setting, FixAimingFullTilt::bEnabled);
    LOG_CONFIG(ConfigKeys::FixAimingFullTilt_Section, ConfigKeys::FixAimingFullTilt_Setting, FixAimingFullTilt::bEnabled);

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

    ConfigHelper::getValue(ini, ConfigKeys::FixAspectRatio_Section, ConfigKeys::FixAspectRatio_Setting, CustomResolutionAndBorderless::bAspectFix);
    ConfigHelper::getValue(ini, ConfigKeys::FixHUD_Section, ConfigKeys::FixHUD_Setting, CustomResolutionAndBorderless::bHUDFix);
    ConfigHelper::getValue(ini, ConfigKeys::FixFOV_Section, ConfigKeys::FixFOV_Setting, CustomResolutionAndBorderless::bFOVFix);
    LOG_CONFIG(ConfigKeys::FixAspectRatio_Section, ConfigKeys::FixAspectRatio_Setting, CustomResolutionAndBorderless::bAspectFix);
    LOG_CONFIG(ConfigKeys::FixHUD_Section, ConfigKeys::FixHUD_Setting, CustomResolutionAndBorderless::bHUDFix);
    LOG_CONFIG(ConfigKeys::FixFOV_Section, ConfigKeys::FixFOV_Setting, CustomResolutionAndBorderless::bFOVFix);

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

    ConfigHelper::getValue(ini, ConfigKeys::LOD_MGS2_ShellCasings_Section, ConfigKeys::LOD_MGS2_ShellCasings_Setting, g_DistanceCulling.bAlwaysRenderShellCasings);
    LOG_CONFIG(ConfigKeys::LOD_MGS2_ShellCasings_Section, ConfigKeys::LOD_MGS2_ShellCasings_Setting, g_DistanceCulling.bAlwaysRenderShellCasings);

    ConfigHelper::getValue(ini, ConfigKeys::LOD_MGS2_NPC_Section, ConfigKeys::LOD_MGS2_NPC_Setting, g_DistanceCulling.bMGS2_ForceNPCLOD);
    LOG_CONFIG(ConfigKeys::LOD_MGS2_NPC_Section, ConfigKeys::LOD_MGS2_NPC_Setting, g_DistanceCulling.bMGS2_ForceNPCLOD);



    ConfigHelper::getValue(ini, ConfigKeys::MGS2_SnakeTales_Radar_Section, ConfigKeys::MGS2_SnakeTales_Radar_Setting, MGS2_SnakeTalesRadar::bEnabled);
    LOG_CONFIG(ConfigKeys::MGS2_SnakeTales_Radar_Section, ConfigKeys::MGS2_SnakeTales_Radar_Setting, MGS2_SnakeTalesRadar::bEnabled);


    ConfigHelper::getValue(ini, ConfigKeys::MGS2_FixDamageType_Section, ConfigKeys::MGS2_FixDamageType_Setting, MGS2VampFPVPunch::bEnabled);
    LOG_CONFIG(ConfigKeys::MGS2_FixDamageType_Section, ConfigKeys::MGS2_FixDamageType_Setting, MGS2VampFPVPunch::bEnabled);
    

    ConfigHelper::getValue(ini, ConfigKeys::Region_Section, ConfigKeys::Region_Setting, sSkipLauncherRegion);
    ConfigHelper::getValue(ini, ConfigKeys::Language_Section, ConfigKeys::Language_Setting, sSkipLauncherLanguage);
    ValidateLauncherRegionOptions();

    LOG_CONFIG(ConfigKeys::Region_Section, ConfigKeys::Region_Setting, sReadableRegionName);
    LOG_CONFIG(ConfigKeys::Language_Section, ConfigKeys::Language_Setting, sReadableLanguageName);

    // Launcher settings
    std::string sLauncherConfigCtrlType = *std::next(kLauncherConfigCtrlTypes.begin(), 5);


    ConfigHelper::getValue(ini, ConfigKeys::CtrlType_Section, ConfigKeys::CtrlType_Setting, sLauncherConfigCtrlType);
    iLauncherConfigCtrlType = Util::findStringInVector(sLauncherConfigCtrlType, kLauncherConfigCtrlTypes);

    ConfigHelper::getValue(ini, ConfigKeys::SkipLauncherMSXGame_Section, ConfigKeys::SkipLauncherMSXGame_Setting, sLauncherConfigMSXGame);

    std::string ps2Type(ConfigKeys::ControllerType_PS2);
    if (iLauncherConfigCtrlType == Util::findStringInVector(ps2Type, kLauncherConfigCtrlTypes))
    {
        bIsPS2controltype = true;
        std::string ps4Type = ConfigKeys::ControllerType_PS4;
        iLauncherConfigCtrlType = Util::findStringInVector(ps4Type, kLauncherConfigCtrlTypes);
        LOG_CONFIG(ConfigKeys::CtrlType_Section, "PS2 Alias Applied", true);

    }

    LOG_CONFIG(ConfigKeys::CtrlType_Section, ConfigKeys::CtrlType_Setting, Util::GetNameAtIndex(kLauncherConfigCtrlTypes, iLauncherConfigCtrlType));

    LOG_CONFIG(ConfigKeys::SkipLauncherMSXGame_Section, ConfigKeys::SkipLauncherMSXGame_Setting, sLauncherConfigMSXGame);

    ConfigHelper::getValue(ini, ConfigKeys::MenuButton_Section, ConfigKeys::MenuButton_Setting, SwapMenuButtons::force_menu_buttons);
    LOG_CONFIG(ConfigKeys::MenuButton_Section, ConfigKeys::MenuButton_Setting, SwapMenuButtons::force_menu_buttons);

    ConfigHelper::getValue(ini, ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Section, ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Setting, g_InputHandler.bCaptureInputsWhileAltTabbed);
    LOG_CONFIG(ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Section, ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Setting, g_InputHandler.bCaptureInputsWhileAltTabbed);

    if (eGameType & (MGS2 | MGS3))
    {

        bool bRestoreVFX = true;
        ConfigHelper::getValue(ini, ConfigKeys::MGS2_Restore_VFX_Section, ConfigKeys::MGS2_Restore_VFX_Setting, bRestoreVFX);
        LOG_CONFIG(ConfigKeys::MGS2_Restore_VFX_Section, ConfigKeys::MGS2_Restore_VFX_Setting, bRestoreVFX);
        g_VectorScalingFix.bFixRain = g_VectorScalingFix.bFixUI = g_EffectSpeedFix.isEnabled = bRestoreVFX;

        if (eGameType & MGS2)
        {
            g_MGS2UnderwaterFilterFix.bEnabled = g_OpticalCamoFix.bEnabled = MGS2BloodStains::bEnabled = MGS2ScopeWarp::bEnabled = MGS2WaterEffects::bEnabled = MGS2LensDroplets::bEnabled = MGS2GasHaze::bEnabled = MGS2_ContrastShader::bEnabled = MGS2_Crossfade::bEnabled = MGS2ConcentrateBlur::bEnabled = bRestoreVFX;

            ConfigHelper::getValue(ini, ConfigKeys::MotionBlur_Section, ConfigKeys::MotionBlur_Setting, MGS2DemoBlur::bEnabled);
            LOG_CONFIG(ConfigKeys::MotionBlur_Section, ConfigKeys::MotionBlur_Setting, MGS2DemoBlur::bEnabled);
            

            ConfigHelper::getValue(ini, ConfigKeys::FixDepthOfField_Section, ConfigKeys::FixDepthOfField_Setting, g_DepthOfFieldFixes.bEnabled);
            LOG_CONFIG(ConfigKeys::FixDepthOfField_Section, ConfigKeys::FixDepthOfField_Setting, g_DepthOfFieldFixes.bEnabled);
        }
        else if (eGameType & MGS3)
        {
            std::string sFilmGrainMode = "On";

            ConfigHelper::getValue(ini, ConfigKeys::FixDepthOfField_Section, ConfigKeys::FixDepthOfField_Setting, g_DepthOfFieldFixes.bEnabled);
            ConfigHelper::getValue(ini, ConfigKeys::MGS3DepthOfFieldBlurUvMultiplier_Section, ConfigKeys::MGS3DepthOfFieldBlurUvMultiplier_Setting, g_DepthOfFieldFixes.fBlurUvMultiplier);
            ConfigHelper::getValue(ini, ConfigKeys::MGS3_Restore_Film_Grain_Section, ConfigKeys::MGS3_Restore_Film_Grain_Setting, sFilmGrainMode);

            bool bFilmGrainEnabled = true;
            if (!ConfigHelper::TryParse(sFilmGrainMode, bFilmGrainEnabled))
            {
                if (sFilmGrainMode == "Restored" || sFilmGrainMode == "Force Always On") // retired choice values, treat as enabled
                {
                    bFilmGrainEnabled = true;
                }
                else
                {
                    ConfigHelper::FatalConfigError(ConfigKeys::MGS3_Restore_Film_Grain_Section, ConfigKeys::MGS3_Restore_Film_Grain_Setting, "Invalid option '" + sFilmGrainMode + "'");
                }
            }
            MGS3FilmGrain::mode = bFilmGrainEnabled ? MGS3FilmGrain::Mode::On : MGS3FilmGrain::Mode::Off;

            LOG_CONFIG(ConfigKeys::FixDepthOfField_Section, ConfigKeys::FixDepthOfField_Setting, g_DepthOfFieldFixes.bEnabled);
            LOG_CONFIG(ConfigKeys::MGS3DepthOfFieldBlurUvMultiplier_Section, ConfigKeys::MGS3DepthOfFieldBlurUvMultiplier_Setting, g_DepthOfFieldFixes.fBlurUvMultiplier);
            LOG_CONFIG(ConfigKeys::MGS3_Restore_Film_Grain_Section, ConfigKeys::MGS3_Restore_Film_Grain_Setting, bFilmGrainEnabled);
        }

        if (g_VectorScalingFix.bFixRain || g_VectorScalingFix.bFixUI)
        {
            InputHandler::GetKeybind(ini, ConfigKeys::ToggleRainShader_Section, ConfigKeys::ToggleRainShader_Setting, g_VectorScalingFix.vkRainShaderToggle);
            InputHandler::GetKeybind(ini, ConfigKeys::CycleWireframeMode_Section, ConfigKeys::CycleWireframeMode_Setting, g_VectorScalingFix.vkWireframeToggle);
            g_VectorScalingFix.iVectorLineScale = 360;
        }
    }

    ConfigHelper::getValue(ini, ConfigKeys::DisableFullscreenOptimization_Section, ConfigKeys::DisableFullscreenOptimization_Setting, g_FixFullscreenOptimization.enabled);
    LOG_CONFIG(ConfigKeys::DisableFullscreenOptimization_Section, ConfigKeys::DisableFullscreenOptimization_Setting, g_FixFullscreenOptimization.enabled);

    ConfigHelper::getValue(ini, ConfigKeys::RestoreDogtagNames_Section, ConfigKeys::RestoreDogtagNames_Setting, MGS2_RestoreDogtags::isEnabled);
    LOG_CONFIG(ConfigKeys::RestoreDogtagNames_Section, ConfigKeys::RestoreDogtagNames_Setting, MGS2_RestoreDogtags::isEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::MGS2_RestoreNodeDOBInfo_Section, ConfigKeys::MGS2_RestoreNodeDOBInfo_Setting, MGS2_RestoreDogtagViewer::bRestoreNodeScreen);
    LOG_CONFIG(ConfigKeys::MGS2_RestoreNodeDOBInfo_Section, ConfigKeys::MGS2_RestoreNodeDOBInfo_Setting, MGS2_RestoreDogtagViewer::bRestoreNodeScreen);



    ConfigHelper::getValue(ini, ConfigKeys::RestoreSoLRadarRotation_Section, ConfigKeys::RestoreSoLRadarRotation_Setting, MGS2_RestoreSoLRadar::bEnabled);
    LOG_CONFIG(ConfigKeys::RestoreSoLRadarRotation_Section, ConfigKeys::RestoreSoLRadarRotation_Setting, MGS2_RestoreSoLRadar::bEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::MGS2_RestoreElevatorGlitch_Section, ConfigKeys::MGS2_RestoreElevatorGlitch_Setting, MGS2_RestoreElevatorGlitch::bEnabled);
    LOG_CONFIG(ConfigKeys::MGS2_RestoreElevatorGlitch_Section, ConfigKeys::MGS2_RestoreElevatorGlitch_Setting, MGS2_RestoreElevatorGlitch::bEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::MGS2_RestoreActionLevelSelection_Section, ConfigKeys::MGS2_RestoreActionLevelSelection_Setting, MGS2_RestoreActionLevelSelection::bEnabled);
    LOG_CONFIG(ConfigKeys::MGS2_RestoreActionLevelSelection_Section, ConfigKeys::MGS2_RestoreActionLevelSelection_Setting, MGS2_RestoreActionLevelSelection::bEnabled);

    std::string sOutdatedSaveDataSetting;
    ConfigHelper::getValue(ini, ConfigKeys::RenameOrRemoveCorruptSaveData_Section, ConfigKeys::RenameOrRemoveCorruptSaveData_Setting, sOutdatedSaveDataSetting);
    if (sOutdatedSaveDataSetting == ConfigKeys::RenameOrRemoveCorruptSaveData_Option_Disable)
    {
        DamagedSaveFix::bEnabled = false;
    }
    else if(sOutdatedSaveDataSetting == ConfigKeys::RenameOrRemoveCorruptSaveData_Option_Delete){
        DamagedSaveFix::bDeleteOutdatedSaveData = true;
    }
    LOG_CONFIG(ConfigKeys::RenameOrRemoveCorruptSaveData_Section, ConfigKeys::RenameOrRemoveCorruptSaveData_Setting, sOutdatedSaveDataSetting);

    ConfigHelper::getValue(ini, ConfigKeys::CorruptSaveData_Notification_Section, ConfigKeys::CorruptSaveData_Notification_Setting, DamagedSaveFix::bEnableConsoleNotification);
    LOG_CONFIG(ConfigKeys::CorruptSaveData_Notification_Section, ConfigKeys::CorruptSaveData_Notification_Setting, DamagedSaveFix::bEnableConsoleNotification);

    ConfigHelper::getValue(ini, ConfigKeys::SaveFolderWriteWarning_Section, ConfigKeys::SaveFolderWriteWarning_Setting, CheckGamesaveFolderWritable::bVerifySameDirectoryWriteable);
    LOG_CONFIG(ConfigKeys::SaveFolderWriteWarning_Section, ConfigKeys::SaveFolderWriteWarning_Setting, CheckGamesaveFolderWritable::bVerifySameDirectoryWriteable);


    ConfigHelper::getValue(ini, ConfigKeys::SaveFileReadOnlyWarning_Section, ConfigKeys::SaveFileReadOnlyWarning_Setting, CheckGamesaveFolderWritable::bCheckSaveFilesReadOnly);
    LOG_CONFIG(ConfigKeys::SaveFileReadOnlyWarning_Section, ConfigKeys::SaveFileReadOnlyWarning_Setting, CheckGamesaveFolderWritable::bCheckSaveFilesReadOnly);

    // Busy Loop Fix
    if (eGameType & (MG | MGS2 | MGS3))
    {
        std::string sBusyLoopFix;
        ConfigHelper::getValue(ini, ConfigKeys::BusyLoopFix_Section, ConfigKeys::BusyLoopFix_Setting, sBusyLoopFix);
        if (sBusyLoopFix != ConfigKeys::BusyLoopFix_Option_Disabled && sBusyLoopFix != ConfigKeys::BusyLoopFix_Option_Half && sBusyLoopFix != ConfigKeys::BusyLoopFix_Option_Full)
        {
            spdlog::error("Invalid config value for {}: {}", ConfigKeys::BusyLoopFix_Setting, sBusyLoopFix);
            Logging::ShowConsole();
            std::cout << "Invalid config value for " << ConfigKeys::BusyLoopFix_Setting << ": " << sBusyLoopFix << std::endl;
            return FreeLibraryAndExitThread(baseModule, 1);
        }
        if (sBusyLoopFix == ConfigKeys::BusyLoopFix_Option_Disabled)
        {
            g_BusyLoopFix.iOption = 0;
        }
        else if (sBusyLoopFix == ConfigKeys::BusyLoopFix_Option_Half)
        {
            g_BusyLoopFix.iOption = 1;
        }
        else if (sBusyLoopFix == ConfigKeys::BusyLoopFix_Option_Full)
        {
            g_BusyLoopFix.iOption = 2;
        }
        LOG_CONFIG(ConfigKeys::BusyLoopFix_Section, ConfigKeys::BusyLoopFix_Setting, sBusyLoopFix);
    }


    ConfigHelper::getValue(ini, ConfigKeys::MGS2_PhoneJingle_Section, ConfigKeys::MGS2_PhoneJingle_Setting, MGS2_RestorePhoneJingle::bEnabled);
    LOG_CONFIG(ConfigKeys::MGS2_PhoneJingle_Section, ConfigKeys::MGS2_PhoneJingle_Setting, MGS2_RestorePhoneJingle::bEnabled);
   

    ConfigHelper::getValue(ini, ConfigKeys::Disable_HDC_Camera_Positions_Section, ConfigKeys::Disable_HDC_Camera_Positions_Setting, OriginalCameraPositions::bEnabled);
    LOG_CONFIG(ConfigKeys::Disable_HDC_Camera_Positions_Section, ConfigKeys::Disable_HDC_Camera_Positions_Setting, OriginalCameraPositions::bEnabled);
    if (OriginalCameraPositions::bEnabled)
    {
        InputHandler::GetKeybind(ini, ConfigKeys::Disable_HDC_Camera_Positions_ToggleKey_Section, ConfigKeys::Disable_HDC_Camera_Positions_ToggleKey_Setting, OriginalCameraPositions::vkToggle_HDC_CameraPositions);
    }



    ConfigHelper::getValue(ini, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Setting, MGS2_ThirdPersonFreecam::bEnabled);
    LOG_CONFIG(ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Setting, MGS2_ThirdPersonFreecam::bEnabled);
    if (MGS2_ThirdPersonFreecam::bEnabled)
    {
        InputHandler::GetKeybind(ini, ConfigKeys::MGS2_ThirdPersonFreecam_ToggleKey_Section, ConfigKeys::MGS2_ThirdPersonFreecam_ToggleKey_Setting, MGS2_ThirdPersonFreecam::vkToggle_Camera);
        InputHandler::GetKeybind(ini, ConfigKeys::MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_ToggleKey_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_ToggleKey_Setting, MGS2_ThirdPersonFreecam::vkToggle_Inherit_Camera_Rotation);
        InputHandler::GetKeybind(ini, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Increase_Hotkey_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Increase_Hotkey_Setting, MGS2_ThirdPersonFreecam::vkToggle_Increase_Camera_Distance);
        InputHandler::GetKeybind(ini, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Decrease_Hotkey_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Decrease_Hotkey_Setting, MGS2_ThirdPersonFreecam::vkToggle_Decrease_Camera_Distance);
        InputHandler::GetKeybind(ini, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Reset_Hotkey_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Reset_Hotkey_Setting, MGS2_ThirdPersonFreecam::vkToggle_Reset_Camera_Distance);
        
        ConfigHelper::getValue(ini, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Setting, MGS2_ThirdPersonFreecam::iCameraDistanceStep);
        LOG_CONFIG(ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Setting, MGS2_ThirdPersonFreecam::iCameraDistanceStep);

        ConfigHelper::getValue(ini, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Change_Speed_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Change_Speed_Setting, MGS2_ThirdPersonFreecam::iCameraDistanceChangeSpeed);
        LOG_CONFIG(ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Change_Speed_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Change_Speed_Setting, MGS2_ThirdPersonFreecam::iCameraDistanceChangeSpeed);

        ConfigHelper::getValue(ini, ConfigKeys::MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Setting, MGS2_ThirdPersonFreecam::bInherit_Camera_Rotation);
        LOG_CONFIG(ConfigKeys::MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Setting, MGS2_ThirdPersonFreecam::bInherit_Camera_Rotation);

        ConfigHelper::getValue(ini, ConfigKeys::MGS2_ThirdPersonFreecam_Max_Camera_Distance_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Max_Camera_Distance_Setting, MGS2_ThirdPersonFreecam::iMax_Camera_Distance);
        LOG_CONFIG(ConfigKeys::MGS2_ThirdPersonFreecam_Max_Camera_Distance_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Max_Camera_Distance_Setting, MGS2_ThirdPersonFreecam::iMax_Camera_Distance);

        ConfigHelper::getValue(ini, ConfigKeys::MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Setting, MGS2_ThirdPersonFreecam::fHorizontal_Sensitivity);
        LOG_CONFIG(ConfigKeys::MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Setting, MGS2_ThirdPersonFreecam::fHorizontal_Sensitivity);

        ConfigHelper::getValue(ini, ConfigKeys::MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Setting, MGS2_ThirdPersonFreecam::fVertical_Sensitivity);
        LOG_CONFIG(ConfigKeys::MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Setting, MGS2_ThirdPersonFreecam::fVertical_Sensitivity);

    }


    std::string sSolidusChokingRestoration;
    ConfigHelper::getValue(ini, ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Section, ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Setting, sSolidusChokingRestoration);
    if (sSolidusChokingRestoration != ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_Disabled &&
        sSolidusChokingRestoration != ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_LifeReductionOnly &&
        sSolidusChokingRestoration != ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_DurationIncreaseOnly &&
        sSolidusChokingRestoration != ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_Both)
    {
        spdlog::error("Invalid config value for {}: {}", ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Setting, sSolidusChokingRestoration);
        Logging::ShowConsole();
        std::cout << "Invalid config value for " << ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Setting << ": " << sSolidusChokingRestoration << std::endl;
        return FreeLibraryAndExitThread(baseModule, 1);
    }
    if (sSolidusChokingRestoration != ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_Disabled)
    {
        if (sSolidusChokingRestoration == ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_Both)
        {
            MGS2_RestoreOriginalDifficulty::bRestoreOriginalSolidusChokingDuration = true;
            MGS2_RestoreOriginalDifficulty::bRestoreOriginalSolidusChokingLife = true;
        }
        else if (sSolidusChokingRestoration == ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_DurationIncreaseOnly)
        {
            MGS2_RestoreOriginalDifficulty::bRestoreOriginalSolidusChokingDuration = true;
        }
        else if (sSolidusChokingRestoration == ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_LifeReductionOnly)
        {
            MGS2_RestoreOriginalDifficulty::bRestoreOriginalSolidusChokingLife = true;
        }
    }
    LOG_CONFIG(ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Section, ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Setting, sSolidusChokingRestoration);


    ConfigHelper::getValue(ini, ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Section, ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Setting, MGS2_RestoreOriginalDifficulty::bEnableGrenadeCooking);
    LOG_CONFIG(ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Section, ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Setting, MGS2_RestoreOriginalDifficulty::bEnableGrenadeCooking);
    if (MGS2_RestoreOriginalDifficulty::bEnableGrenadeCooking)
    {
        InputHandler::GetKeybind(ini, ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Toggle_Section, ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Toggle_Setting, MGS2_RestoreOriginalDifficulty::vkToggleGrenadeCooking);

    }

    std::string sHostageType;
    ConfigHelper::getValue(ini, ConfigKeys::MGS2_Hostage_Type_Section, ConfigKeys::MGS2_Hostage_Type_Setting, sHostageType);
    if (sHostageType != ConfigKeys::MGS2_Hostage_Type_Option_Normal &&
        sHostageType != ConfigKeys::MGS2_Hostage_Type_Option_OnePM &&
        sHostageType != ConfigKeys::MGS2_Hostage_Type_Option_TenPM &&
        sHostageType != ConfigKeys::MGS2_Hostage_Type_Option_Midnight)
    {
        spdlog::error("Invalid config value for {}: {}", ConfigKeys::MGS2_Hostage_Type_Setting, sHostageType);
        Logging::ShowConsole();
        std::cout << "Invalid config value for " << ConfigKeys::MGS2_Hostage_Type_Setting << ": " << sHostageType << std::endl;
        return FreeLibraryAndExitThread(baseModule, 1);
    }
    if (sHostageType != ConfigKeys::MGS2_Hostage_Type_Option_Normal)
    {
        if (sHostageType == ConfigKeys::MGS2_Hostage_Type_Option_OnePM)
        {
            MGS2_Hostage_Type_Easter_Egg::hostageMode = MGS2_Hostage_Type_Easter_Egg::HostageMode::RTC_KATOCHAN;
        }
        else if (sHostageType == ConfigKeys::MGS2_Hostage_Type_Option_TenPM)
        {
            MGS2_Hostage_Type_Easter_Egg::hostageMode = MGS2_Hostage_Type_Easter_Egg::HostageMode::RTC_CATHY;
        }
        else if (sHostageType == ConfigKeys::MGS2_Hostage_Type_Option_Midnight)
        {
            MGS2_Hostage_Type_Easter_Egg::hostageMode = MGS2_Hostage_Type_Easter_Egg::HostageMode::RTC_JENNIFER;
        }
    }
    LOG_CONFIG(ConfigKeys::MGS2_Hostage_Type_Section, ConfigKeys::MGS2_Hostage_Type_Setting, sHostageType);


    ConfigHelper::getValue(ini, ConfigKeys::Caption_Scale_Section, ConfigKeys::Caption_Scale_Setting, AdjustableCaptions::fSubtitleScale);
    LOG_CONFIG(ConfigKeys::Caption_Scale_Section, ConfigKeys::Caption_Scale_Setting, AdjustableCaptions::fSubtitleScale);

    ConfigHelper::getValue(ini, ConfigKeys::Caption_Opacity_Section, ConfigKeys::Caption_Opacity_Setting, AdjustableCaptions::iSubtitleAlpha);
    LOG_CONFIG(ConfigKeys::Caption_Opacity_Section, ConfigKeys::Caption_Opacity_Setting, AdjustableCaptions::iSubtitleAlpha);

    ConfigHelper::getValue(ini, ConfigKeys::Caption_Background_Opacity_Section, ConfigKeys::Caption_Background_Opacity_Setting, AdjustableCaptions::iOutlineOpacity);
    LOG_CONFIG(ConfigKeys::Caption_Background_Opacity_Section, ConfigKeys::Caption_Background_Opacity_Setting, AdjustableCaptions::iOutlineOpacity);

    ConfigHelper::getValue(ini, ConfigKeys::MGS2_First_Person_View_Enabled_Section, ConfigKeys::MGS2_First_Person_View_Enabled_Setting, MGS2_First_Person_View::bFirst_Person_View_Enabled);
    LOG_CONFIG(ConfigKeys::MGS2_First_Person_View_Enabled_Section, ConfigKeys::MGS2_First_Person_View_Enabled_Setting, MGS2_First_Person_View::bFirst_Person_View_Enabled);
    if (MGS2_First_Person_View::bFirst_Person_View_Enabled)
    {
        InputHandler::GetKeybind(ini, ConfigKeys::MGS2_First_Person_View_ToggleKey_Section, ConfigKeys::MGS2_First_Person_View_ToggleKey_Setting, MGS2_First_Person_View::vkToggle_First_Person_Override);
        InputHandler::GetKeybind(ini, ConfigKeys::MGS2_First_Person_View_Movement_ToggleKey_Section, ConfigKeys::MGS2_First_Person_View_Movement_ToggleKey_Setting, MGS2_First_Person_View::vkToggle_First_Person_View_Movement);

        ConfigHelper::getValue(ini, ConfigKeys::MGS2_First_Person_View_Movement_Enabled_By_Default_Section, ConfigKeys::MGS2_First_Person_View_Movement_Enabled_By_Default_Setting, MGS2_First_Person_View::bFirst_Person_View_Movement_Enabled_By_Default);
        LOG_CONFIG(ConfigKeys::MGS2_First_Person_View_Movement_Enabled_By_Default_Section, ConfigKeys::MGS2_First_Person_View_Movement_Enabled_By_Default_Setting, MGS2_First_Person_View::bFirst_Person_View_Movement_Enabled_By_Default);
    }

    ConfigHelper::getValue(ini, ConfigKeys::MGS2_First_Person_View_Hold_Button_Section, ConfigKeys::MGS2_First_Person_View_Hold_Button_Setting, MGS2_First_Person_View::bFirst_Person_View_Toggle_Hold);
    LOG_CONFIG(ConfigKeys::MGS2_First_Person_View_Hold_Button_Section, ConfigKeys::MGS2_First_Person_View_Hold_Button_Setting, MGS2_First_Person_View::bFirst_Person_View_Toggle_Hold);
    if (MGS2_First_Person_View::bFirst_Person_View_Toggle_Hold)
    {
        InputHandler::GetKeybind(ini, ConfigKeys::MGS2_First_Person_View_Hold_ToggleKey_Section, ConfigKeys::MGS2_First_Person_View_Hold_ToggleKey_Setting, MGS2_First_Person_View::vkToggle_Hold_First_Person_View);
    }

    std::string sColonelMsxSprite;
    ConfigHelper::getValue(ini, ConfigKeys::UnusedRetroColonel_Section, ConfigKeys::UnusedRetroColonel_Setting, sColonelMsxSprite);
    if (sColonelMsxSprite == ConfigKeys::UnusedRetroColonel_Option_Subsistence)
    {
        g_MGS2RetroColonel.bEnabled = true;
        g_MGS2RetroColonel.bUseNewSprite = true;
    }
    else if (sColonelMsxSprite == ConfigKeys::UnusedRetroColonel_Option_MSX)
    {
        g_MGS2RetroColonel.bEnabled = true;
        g_MGS2RetroColonel.bUseNewSprite = false;
    }
    else if (sColonelMsxSprite == ConfigKeys::UnusedRetroColonel_Option_Normal)
    {
        g_MGS2RetroColonel.bEnabled = false;
    }
    else
    {
        spdlog::error("Invalid config value for MGS2 Colonel Sprite: {}", sColonelMsxSprite);
        Logging::ShowConsole();
        std::cout << "Invalid config value for MGS2 Colonel Sprite: " << sColonelMsxSprite << std::endl;
        return FreeLibraryAndExitThread(baseModule, 1);
    }
    LOG_CONFIG(ConfigKeys::UnusedRetroColonel_Section, ConfigKeys::UnusedRetroColonel_Setting, sColonelMsxSprite);


    ConfigHelper::getValue(ini, ConfigKeys::Restore_Title_Screen_Swapping_Section, ConfigKeys::Restore_Title_Screen_Swapping_Setting, TextureLiveSwaps::bRestoreTitleScreenSwapping);
    LOG_CONFIG(ConfigKeys::Restore_Title_Screen_Swapping_Section, ConfigKeys::Restore_Title_Screen_Swapping_Setting, TextureLiveSwaps::bRestoreTitleScreenSwapping);



    ConfigHelper::getValue(ini, ConfigKeys::MGS2_Lifebar_Name_Use_Character_Names_Section, ConfigKeys::MGS2_Lifebar_Name_Use_Character_Names_Setting, CustomPlayerName::bUseCharacterNames);
    LOG_CONFIG(ConfigKeys::MGS2_Lifebar_Name_Use_Character_Names_Section, ConfigKeys::MGS2_Lifebar_Name_Use_Character_Names_Setting, CustomPlayerName::bUseCharacterNames);
    if (!CustomPlayerName::bUseCharacterNames)
    {
        ConfigHelper::getValue(ini, ConfigKeys::MGS2_Lifebar_Name_Use_Custom_Section, ConfigKeys::MGS2_Lifebar_Name_Use_Custom_Setting, CustomPlayerName::bUseCustomName);
        LOG_CONFIG(ConfigKeys::MGS2_Lifebar_Name_Use_Custom_Section, ConfigKeys::MGS2_Lifebar_Name_Use_Custom_Setting, CustomPlayerName::bUseCustomName);
        if (CustomPlayerName::bUseCustomName)
        {
            ConfigHelper::getValue(ini, ConfigKeys::MGS2_Lifebar_Name_Custom_Section, ConfigKeys::MGS2_Lifebar_Name_Custom_Setting, CustomPlayerName::sCustomName);
            LOG_CONFIG(ConfigKeys::MGS2_Lifebar_Name_Custom_Section, ConfigKeys::MGS2_Lifebar_Name_Custom_Setting, CustomPlayerName::sCustomName);
        }
    }



    ConfigHelper::getValue(ini, ConfigKeys::EnableSMAA_Section, ConfigKeys::EnableSMAA_Setting, SMAA_AA::bEnabled);
    LOG_CONFIG(ConfigKeys::EnableSMAA_Section, ConfigKeys::EnableSMAA_Setting, SMAA_AA::bEnabled);



    ConfigHelper::getValue(ini, ConfigKeys::ColorCorrection_Enabled_Section, ConfigKeys::ColorCorrection_Enabled_Setting, ColorCorrection::bEnabled);
    LOG_CONFIG(ConfigKeys::ColorCorrection_Enabled_Section, ConfigKeys::ColorCorrection_Enabled_Setting, ColorCorrection::bEnabled);

    ConfigHelper::getValue(ini, ConfigKeys::MG1_Crop_Overscan_Enabled_Section, ConfigKeys::MG1_Crop_Overscan_Enabled_Setting, MG1_DisplayScaling::bCropBorders);
    LOG_CONFIG(ConfigKeys::MG1_Crop_Overscan_Enabled_Section, ConfigKeys::MG1_Crop_Overscan_Enabled_Setting, MG1_DisplayScaling::bCropBorders);


    ConfigHelper::getValue(ini, ConfigKeys::MG1_Correct_Aspect_Ratio_Enabled_Section, ConfigKeys::MG1_Correct_Aspect_Ratio_Enabled_Setting, MG1_DisplayScaling::bCorrectTo4x3);
    LOG_CONFIG(ConfigKeys::MG1_Correct_Aspect_Ratio_Enabled_Section, ConfigKeys::MG1_Correct_Aspect_Ratio_Enabled_Setting, MG1_DisplayScaling::bCorrectTo4x3);




    ConfigHelper::getValue(ini, ConfigKeys::Debugging_Start_In_Dev_Menu_Section, ConfigKeys::Debugging_Start_In_Dev_Menu_Setting, MGS2_GameFuncs::StartInDebugMode);
    LOG_CONFIG(ConfigKeys::Debugging_Start_In_Dev_Menu_Section, ConfigKeys::Debugging_Start_In_Dev_Menu_Setting, MGS2_GameFuncs::StartInDebugMode);

    ConfigHelper::getValue(ini, ConfigKeys::FixIGTLoadingPause_Section, ConfigKeys::FixIGTLoadingPause_Setting, FixPlaytime::bEnabled);
    LOG_CONFIG(ConfigKeys::FixIGTLoadingPause_Section, ConfigKeys::FixIGTLoadingPause_Setting, FixPlaytime::bEnabled);


    ConfigLogger::Flush();
}
