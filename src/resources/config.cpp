#include "common.hpp"
#include "config.hpp"

#include <inipp/inipp.h>

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

    int loadedConfigVersion;
    inipp::get_value(ini.sections["Config Version"], "Version", loadedConfigVersion);
    if (loadedConfigVersion != iConfigVersion)
    {
        spdlog::error("CONFIG ERROR: Config file version mismatch! Expected version {}, but found version {}.", iConfigVersion, loadedConfigVersion);
        Logging::ShowConsole();
        std::cout << "" << sFixName << " v" << sFixVersion << " loaded." << std::endl;
        std::cout << "MGSHDFix CONFIG ERROR: Outdated config file!" << std::endl;
        std::cout << "MGSHDFix CONFIG ERROR: Please install -all- the files from the latest release!" << std::endl;
        return FreeLibraryAndExitThread(baseModule, 1);
    }

    // Grab desktop resolution
    DesktopDimensions = Util::GetPhysicalDesktopDimensions();

    // Read ini file
    g_Logging.bVerboseLogging = Util::stringToBool(ini.sections["Verbose Logging"]["Enabled"]);
    bOutputResolution = Util::stringToBool(ini.sections["Output Resolution"]["Enabled"]);
    inipp::get_value(ini.sections["Output Resolution"], "Width", iOutputResX);
    inipp::get_value(ini.sections["Output Resolution"], "Height", iOutputResY);
    bWindowedMode = Util::stringToBool(ini.sections["Output Resolution"]["Windowed"]);
    bBorderlessMode = Util::stringToBool(ini.sections["Output Resolution"]["Borderless"]);
    inipp::get_value(ini.sections["Internal Resolution"], "Width", iInternalResX);
    inipp::get_value(ini.sections["Internal Resolution"], "Height", iInternalResY);
    inipp::get_value(ini.sections["Anisotropic Filtering"], "Samples", iAnisotropicFiltering);
    bDisableTextureFiltering = Util::stringToBool(ini.sections["Disable Texture Filtering"]["DisableTextureFiltering"]);
    bFramebufferFix = Util::stringToBool(ini.sections["Framebuffer Fix"]["Enabled"]);
    bLauncherJumpStart = Util::stringToBool(ini.sections["Launcher Config"]["LauncherJumpStart"]);
    g_IntroSkip.isEnabled = Util::stringToBool(ini.sections["Skip Intro Logos"]["Enabled"]);
    g_StereoAudioFix.isEnabled = Util::stringToBool(ini.sections["Force Stereo Audio"]["Enabled"]);
    g_PauseOnFocusLoss.bPauseOnFocusLoss = Util::stringToBool(ini.sections["Pause On Focus Loss"]["Enabled"]);
    g_PauseOnFocusLoss.bSpeedrunnerBugfixOverride = Util::stringToBool(ini.sections["Pause On Focus Loss"]["SpeedrunnerBugfixOverride"]);
    g_MuteWarning.bEnabled = Util::stringToBool(ini.sections["Mute Warning"]["Enabled"]);

    bShouldCheckForUpdates = Util::stringToBool(ini.sections["Update Notifications"]["CheckForUpdates"]);
    bConsoleUpdateNotifications = Util::stringToBool(ini.sections["Update Notifications"]["ConsoleNotifications"]);
    g_StatPersistence.bAchievementPersistenceEnabled = Util::stringToBool(ini.sections["Achievement Persistence"]["Enabled"]);
    g_SteamAPI.bResetAchievements = Util::stringToBool(ini.sections["Reset All Achievements"]["Reset_All_Achievements"]);

    /*//INITIALIZE(Init_GammaShader());
    //INITIALIZE(g_DistanceCulling.Initialize());
    //INITIALIZE(g_MultiSampleAntiAliasing.Initialize());
    //INITIALIZE(g_Wireframe.Initialize());

    //INITIALIZE(g_AimAfterEquipFix.Initialize());
    //INITIALIZE(g_ColorFilterFix.Initialize());*/

    //inipp::get_value(ini.sections["MG1 Custom Loading Screens"], "Enabled", g_MG1CustomLoadingScreens.isEnabled);
    bMouseSensitivity = Util::stringToBool(ini.sections["Mouse Sensitivity"]["Enabled"]);
    inipp::get_value(ini.sections["Mouse Sensitivity"], "X Multiplier", fMouseSensitivityXMulti);
    inipp::get_value(ini.sections["Mouse Sensitivity"], "Y Multiplier", fMouseSensitivityYMulti);
    bDisableCursor = Util::stringToBool(ini.sections["Disable Mouse Cursor"]["Enabled"]);
    inipp::get_value(ini.sections["Texture Buffer"], "SizeMB", g_TextureBufferSize.iTextureBufferSizeMB);
    bAspectFix = Util::stringToBool(ini.sections["Fix Aspect Ratio"]["Enabled"]);
    bHUDFix = Util::stringToBool(ini.sections["Fix HUD"]["Enabled"]);
    bFOVFix = Util::stringToBool(ini.sections["Fix FOV"]["Enabled"]);
    bLauncherConfigSkipLauncher = Util::stringToBool(ini.sections["Launcher Config"]["SkipLauncher"]);

    // Read launcher settings from ini
    std::string sLauncherConfigCtrlType = "kbd";
    std::string sLauncherConfigRegion = "us";
    std::string sLauncherConfigLanguage = "en";
    inipp::get_value(ini.sections["Launcher Config"], "CtrlType", sLauncherConfigCtrlType);
    inipp::get_value(ini.sections["Launcher Config"], "Region", sLauncherConfigRegion);
    inipp::get_value(ini.sections["Launcher Config"], "Language", sLauncherConfigLanguage);
    inipp::get_value(ini.sections["Launcher Config"], "MSXGame", sLauncherConfigMSXGame);
    inipp::get_value(ini.sections["Launcher Config"], "MSXWallType", iLauncherConfigMSXWallType);
    inipp::get_value(ini.sections["Launcher Config"], "MSXWallAlign", sLauncherConfigMSXWallAlign);
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
        g_VectorScalingFix.bEnableVectorLineFix = Util::stringToBool(ini.sections["Vector Line Fix"]["Enabled"]);
        spdlog::info("Config Parse: Fix Vector Effect (Rain) Scaling: {}", g_VectorScalingFix.bEnableVectorLineFix);
        if (g_VectorScalingFix.bEnableVectorLineFix)
        {
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
}
