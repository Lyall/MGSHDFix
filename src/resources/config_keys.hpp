#pragma once
#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string>
#include <initializer_list>

inline const std::initializer_list<std::string> kLauncherConfigCtrlTypes = {
    "PS5",
    "PS4",
    "XBOX",
    "NX",
    "STMD",
    "KBD",
    "PS2"
};

inline const std::initializer_list<std::string> kLauncherConfigLanguages = {
    "EN",
    "JP",
    "FR",
    "GR",
    "IT",
    "PR",
    "SP",
    "DU",
    "RU"
};

inline const std::initializer_list<std::string> kLauncherConfigRegions = {
    "US",
    "JP",
    "EU"
};

namespace ConfigKeys
{
    // Graphics

    constexpr const char* ForceWindowSize_Section = "Output Resolution";
    constexpr const char* ForceWindowSize_Setting = "Enabled";
    constexpr const char* ForceWindowSize_Help = "";
    constexpr const char* ForceWindowSize_Tooltip = "";

    constexpr const char* WindowWidth_Section = "Output Resolution";
    constexpr const char* WindowWidth_Setting = "Width";
    constexpr const char* WindowWidth_Help = "";
    constexpr const char* WindowWidth_Tooltip = "";

    constexpr const char* WindowHeight_Section = "Output Resolution";
    constexpr const char* WindowHeight_Setting = "Height";
    constexpr const char* WindowHeight_Help = "";
    constexpr const char* WindowHeight_Tooltip = "";

    constexpr const char* WindowedMode_Section = "Output Resolution";
    constexpr const char* WindowedMode_Setting = "Windowed";
    constexpr const char* WindowedMode_Help = "";
    constexpr const char* WindowedMode_Tooltip = "";

    constexpr const char* BorderlessWindowed_Section = "Output Resolution";
    constexpr const char* BorderlessWindowed_Setting = "Borderless";
    constexpr const char* BorderlessWindowed_Help = "";
    constexpr const char* BorderlessWindowed_Tooltip = "";

    constexpr const char* RenderScaleWidth_Section = "Internal Resolution";
    constexpr const char* RenderScaleWidth_Setting = "Width";
    constexpr const char* RenderScaleWidth_Help = "";
    constexpr const char* RenderScaleWidth_Tooltip = "";

    constexpr const char* RenderScaleHeight_Section = "Internal Resolution";
    constexpr const char* RenderScaleHeight_Setting = "Height";
    constexpr const char* RenderScaleHeight_Help = "";
    constexpr const char* RenderScaleHeight_Tooltip = "";

    constexpr const char* AnisotropicFiltering_Section = "Texture Filtering";
    constexpr const char* AnisotropicFiltering_Setting = "Anisotropic Filtering Level";
    constexpr const char* AnisotropicFiltering_Help = "";
    constexpr const char* AnisotropicFiltering_Tooltip = "";

    constexpr const char* DisableTextureFiltering_Section = "Texture Filtering";
    constexpr const char* DisableTextureFiltering_Setting = "Disable Texture Filtering";
    constexpr const char* DisableTextureFiltering_Help = "";
    constexpr const char* DisableTextureFiltering_Tooltip = "";

    // Ultrawide
    constexpr const char* FixAspectRatio_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FixAspectRatio_Setting = "Fix Aspect Ratio";
    constexpr const char* FixAspectRatio_Help = "";
    constexpr const char* FixAspectRatio_Tooltip = "";

    constexpr const char* FixHUD_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FixHUD_Setting = "Fix HUD";
    constexpr const char* FixHUD_Help = "";
    constexpr const char* FixHUD_Tooltip = "";

    constexpr const char* FixFOV_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FixFOV_Setting = "Fix FOV";
    constexpr const char* FixFOV_Help = "";
    constexpr const char* FixFOV_Tooltip = "";

    constexpr const char* FramebufferFix_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FramebufferFix_Setting = "Fix Framebuffer";
    constexpr const char* FramebufferFix_Help = "";
    constexpr const char* FramebufferFix_Tooltip = "";

    // Bugfixes
    constexpr const char* FixVectorRain_Section = "Vector Line Scaling Fix";
    constexpr const char* FixVectorRain_Setting = "Fix Rain Width";
    constexpr const char* FixVectorRain_Help = "";
    constexpr const char* FixVectorRain_Tooltip = "";

    constexpr const char* FixVectorUI_Section = "Vector Line Scaling Fix";
    constexpr const char* FixVectorUI_Setting = "Fix UI Width";
    constexpr const char* FixVectorUI_Help = "";
    constexpr const char* FixVectorUI_Tooltip = "";

    constexpr const char* VectorLineScale_Section = "Vector Line Scaling Fix";
    constexpr const char* VectorLineScale_Setting = "Line Scale";
    constexpr const char* VectorLineScale_Help = "";
    constexpr const char* VectorLineScale_Tooltip = "";

    constexpr const char* EffectSpeedFixes_Section = "Bugfixes";
    constexpr const char* EffectSpeedFixes_Setting = "Fix Effect Speeds";
    constexpr const char* EffectSpeedFixes_Help = "";
    constexpr const char* EffectSpeedFixes_Tooltip = "";

    constexpr const char* EnablePauseOnFocusLoss_Section = "Pause On Focus Loss";
    constexpr const char* EnablePauseOnFocusLoss_Setting = "Enabled";
    constexpr const char* EnablePauseOnFocusLoss_Help = "";
    constexpr const char* EnablePauseOnFocusLoss_Tooltip = "";

    constexpr const char* PauseOnFocusLoss_SpeedrunnerBugfixOverride_Section = "Bugfixes";
    constexpr const char* PauseOnFocusLoss_SpeedrunnerBugfixOverride_Setting = "Fix Alt-Tab Loading Bugs";
    constexpr const char* PauseOnFocusLoss_SpeedrunnerBugfixOverride_Help = "";
    constexpr const char* PauseOnFocusLoss_SpeedrunnerBugfixOverride_Tooltip = "";

    constexpr const char* FixAimingAfterEquip_Section = "Bugfixes";
    constexpr const char* FixAimingAfterEquip_Setting = "Fix Aiming After Equip";
    constexpr const char* FixAimingAfterEquip_Help = "";
    constexpr const char* FixAimingAfterEquip_Tooltip = "";

    constexpr const char* FixAimingFullTilt_Section = "Bugfixes";
    constexpr const char* FixAimingFullTilt_Setting = "Fix Aiming On Full Tilt";
    constexpr const char* FixAimingFullTilt_Help = "";
    constexpr const char* FixAimingFullTilt_Tooltip = "";

    constexpr const char* CPUCoreLimit_Section = "System Specific Fixes";
    constexpr const char* CPUCoreLimit_Setting = "Limit Games to 2 CPU Cores";
    constexpr const char* CPUCoreLimit_Help = "(Fixes cutscene crashes on Ryzen 9800X3D CPUs)";
    constexpr const char* CPUCoreLimit_Tooltip = "i'm a dog :3";

    // Gameplay
    constexpr const char* KeepAimingAfterFiring_Always_Section = "Keep Aiming After Firing";
    constexpr const char* KeepAimingAfterFiring_Always_Setting = "Always Keep Aiming";
    constexpr const char* KeepAimingAfterFiring_Always_Help = "";
    constexpr const char* KeepAimingAfterFiring_Always_Tooltip = "";

    constexpr const char* KeepAimingAfterFiring_InFirstPerson_Section = "Keep Aiming After Firing";
    constexpr const char* KeepAimingAfterFiring_InFirstPerson_Setting = "While in First Person";
    constexpr const char* KeepAimingAfterFiring_InFirstPerson_Help = "";
    constexpr const char* KeepAimingAfterFiring_InFirstPerson_Tooltip = "";

    constexpr const char* KeepAimingAfterFiring_OnLockOn_Section = "Keep Aiming After Firing";
    constexpr const char* KeepAimingAfterFiring_OnLockOn_Setting = "While Holding Lock On";
    constexpr const char* KeepAimingAfterFiring_OnLockOn_Help = "";
    constexpr const char* KeepAimingAfterFiring_OnLockOn_Tooltip = "";

    // Tweaks
    constexpr const char* SkipIntroLogos_Section = "Skip Logo Screens";
    constexpr const char* SkipIntroLogos_Setting = "Skip In-Game Logos";
    constexpr const char* SkipIntroLogos_Help = "";
    constexpr const char* SkipIntroLogos_Tooltip = "";

    constexpr const char* LauncherJumpStart_Section = "Skip Logo Screens";
    constexpr const char* LauncherJumpStart_Setting = "Skip Launcher Logos";
    constexpr const char* LauncherJumpStart_Help = "";
    constexpr const char* LauncherJumpStart_Tooltip = "";

    constexpr const char* SkipLauncher_Section = "Launcher Config";
    constexpr const char* SkipLauncher_Setting = "Skip Launcher";
    constexpr const char* SkipLauncher_Help = "";
    constexpr const char* SkipLauncher_Tooltip = "";

    constexpr const char* CtrlType_Section = "Controller Icons";
    constexpr const char* CtrlType_Setting = "Controller Type";
    constexpr const char* CtrlType_Help = "";
    constexpr const char* CtrlType_Tooltip = "";

    constexpr const char* Language_Section = "Language Settings";
    constexpr const char* Language_Setting = "Game Language";
    constexpr const char* Language_Help = "";
    constexpr const char* Language_Tooltip = "";

    constexpr const char* Region_Section = "Language Settings";
    constexpr const char* Region_Setting = "Game Region";
    constexpr const char* Region_Help = "";
    constexpr const char* Region_Tooltip = "";

    constexpr const char* SkipLauncherMSXGame_Section = "Launcher Config";
    constexpr const char* SkipLauncherMSXGame_Setting = "MSXGame";
    constexpr const char* SkipLauncherMSXGame_Help = "";
    constexpr const char* SkipLauncherMSXGame_Tooltip = "";

    constexpr const char* MSXWallType_Section = "Launcher Config";
    constexpr const char* MSXWallType_Setting = "MSXWallType";
    constexpr const char* MSXWallType_Help = "";
    constexpr const char* MSXWallType_Tooltip = "";

    constexpr const char* MSXWallAlign_Section = "Launcher Config";
    constexpr const char* MSXWallAlign_Setting = "MSXWallAlign";
    constexpr const char* MSXWallAlign_Help = "";
    constexpr const char* MSXWallAlign_Tooltip = "";

    constexpr const char* ForceStereoAudio_Section = "System Specific Fixes";
    constexpr const char* ForceStereoAudio_Setting = "Force Stereo Audio Output";
    constexpr const char* ForceStereoAudio_Help = "(Fixes codec / radio conversation volume)";
    constexpr const char* ForceStereoAudio_Tooltip = "";

    constexpr const char* MuteWarning_Section = "Mute Warning";
    constexpr const char* MuteWarning_Setting = "Enabled";
    constexpr const char* MuteWarning_Help = "";
    constexpr const char* MuteWarning_Tooltip = "";

    constexpr const char* DisableMouseCursor_Section = "Bugfixes";
    constexpr const char* DisableMouseCursor_Setting = "Fix Mouse Cursor Showing";
    constexpr const char* DisableMouseCursor_Help = "";
    constexpr const char* DisableMouseCursor_Tooltip = "";

    constexpr const char* OverrideMouseSensitivity_Section = "Mouse Sensitivity";
    constexpr const char* OverrideMouseSensitivity_Setting = "Enabled";
    constexpr const char* OverrideMouseSensitivity_Help = "";
    constexpr const char* OverrideMouseSensitivity_Tooltip = "";

    constexpr const char* MouseSensitivity_XMultiplier_Section = "Mouse Sensitivity";
    constexpr const char* MouseSensitivity_XMultiplier_Setting = "X Multiplier";
    constexpr const char* MouseSensitivity_XMultiplier_Help = "";
    constexpr const char* MouseSensitivity_XMultiplier_Tooltip = "";

    constexpr const char* MouseSensitivity_YMultiplier_Section = "Mouse Sensitivity";
    constexpr const char* MouseSensitivity_YMultiplier_Setting = "Y Multiplier";
    constexpr const char* MouseSensitivity_YMultiplier_Help = "";
    constexpr const char* MouseSensitivity_YMultiplier_Tooltip = "";

    constexpr const char* MGS2Sunglasses_Section = "MGS2 Sunglasses";
    constexpr const char* MGS2Sunglasses_Setting = "ShouldWearSunglasses";
    constexpr const char* MGS2Sunglasses_Help = "";
    constexpr const char* MGS2Sunglasses_Tooltip = "";

    // Hotkeys
    constexpr const char* ToggleRainShader_Section = "Hotkeys";
    constexpr const char* ToggleRainShader_Setting = "Toggle Rain Shader";
    constexpr const char* ToggleRainShader_Help = "";
    constexpr const char* ToggleRainShader_Tooltip = "";

    constexpr const char* ToggleUIShader_Section = "Hotkeys";
    constexpr const char* ToggleUIShader_Setting = "Toggle UI Shader";
    constexpr const char* ToggleUIShader_Help = "";
    constexpr const char* ToggleUIShader_Tooltip = "";

    constexpr const char* CycleWireframeMode_Section = "Hotkeys";
    constexpr const char* CycleWireframeMode_Setting = "Cycle Wireframe Mode";
    constexpr const char* CycleWireframeMode_Help = "";
    constexpr const char* CycleWireframeMode_Tooltip = "";

    // Achievements
    constexpr const char* AchievementPersistence_Section = "Bugfixes";
    constexpr const char* AchievementPersistence_Setting = "Fix Achievement Stat Tracking";
    constexpr const char* AchievementPersistence_Help = "";
    constexpr const char* AchievementPersistence_Tooltip = "";

    constexpr const char* ResetAllAchievements_Section = "CAUTION - THIS BOX WILL RESET ALL ACHIEVEMENTS.";
    constexpr const char* ResetAllAchievements_Setting = "Reset All Achivements";
    constexpr const char* ResetAllAchievements_Help = "";
    constexpr const char* ResetAllAchievements_Tooltip = "";

    // Internal
    constexpr const char* CheckForUpdates_Section = "Update Notifications";
    constexpr const char* CheckForUpdates_Setting = "CheckForUpdates";
    constexpr const char* CheckForUpdates_Help = "";
    constexpr const char* CheckForUpdates_Tooltip = "";

    constexpr const char* UpdateConsoleNotifications_Section = "Update Notifications";
    constexpr const char* UpdateConsoleNotifications_Setting = "ConsoleNotifications";
    constexpr const char* UpdateConsoleNotifications_Help = "";
    constexpr const char* UpdateConsoleNotifications_Tooltip = "";

    constexpr const char* VerboseLogging_Section = "Verbose Logging";
    constexpr const char* VerboseLogging_Setting = "Enabled";
    constexpr const char* VerboseLogging_Help = "";
    constexpr const char* VerboseLogging_Tooltip = "";


}
