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
    constexpr const char* VerboseLogging_Section = "Verbose Logging";
    constexpr const char* VerboseLogging_Setting = "Enabled";

    constexpr const char* ForceWindowSize_Section = "Output Resolution";
    constexpr const char* ForceWindowSize_Setting = "Enabled";
    constexpr const char* WindowWidth_Section = "Output Resolution";
    constexpr const char* WindowWidth_Setting = "Width";
    constexpr const char* WindowHeight_Section = "Output Resolution";
    constexpr const char* WindowHeight_Setting = "Height";
    constexpr const char* WindowedMode_Section = "Output Resolution";
    constexpr const char* WindowedMode_Setting = "Windowed";
    constexpr const char* BorderlessWindowed_Section = "Output Resolution";
    constexpr const char* BorderlessWindowed_Setting = "Borderless";

    constexpr const char* RenderScaleWidth_Section = "Internal Resolution";
    constexpr const char* RenderScaleWidth_Setting = "Width";
    constexpr const char* RenderScaleHeight_Section = "Internal Resolution";
    constexpr const char* RenderScaleHeight_Setting = "Height";

    constexpr const char* AnisotropicFiltering_Section = "Texture Filtering";
    constexpr const char* AnisotropicFiltering_Setting = "Anisotropic Filtering Level";

    constexpr const char* DisableTextureFiltering_Section = "Texture Filtering";
    constexpr const char* DisableTextureFiltering_Setting = "Disable Texture Filtering";

    // Ultrawide
    constexpr const char* FixAspectRatio_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FixAspectRatio_Setting = "Fix Aspect Ratio";

    constexpr const char* FixHUD_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FixHUD_Setting = "Fix HUD";

    constexpr const char* FixFOV_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FixFOV_Setting = "Fix FOV";

    constexpr const char* FramebufferFix_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FramebufferFix_Setting = "Fix Framebuffer";

    // Bugfixes
    constexpr const char* FixVectorRain_Section = "Vector Line Scaling Fix";
    constexpr const char* FixVectorRain_Setting = "Fix Rain Width";
    constexpr const char* FixVectorUI_Section = "Vector Line Scaling Fix";
    constexpr const char* FixVectorUI_Setting = "Fix UI Width";
    constexpr const char* VectorLineScale_Section = "Vector Line Scaling Fix";
    constexpr const char* VectorLineScale_Setting = "Line Scale";

    constexpr const char* EffectSpeedFixes_Section = "Bugfixes";
    constexpr const char* EffectSpeedFixes_Setting = "Fix Effect Speeds";

    constexpr const char* EnablePauseOnFocusLoss_Section = "Pause On Focus Loss";
    constexpr const char* EnablePauseOnFocusLoss_Setting = "Enabled";
    constexpr const char* PauseOnFocusLoss_SpeedrunnerBugfixOverride_Section = "Bugfixes";
    constexpr const char* PauseOnFocusLoss_SpeedrunnerBugfixOverride_Setting = "Fix Alt-Tab Loading Bugs";

    constexpr const char* FixAimingAfterEquip_Section = "Bugfixes";
    constexpr const char* FixAimingAfterEquip_Setting = "Fix Aiming After Equip";

    constexpr const char* FixAimingFullTilt_Section = "Bugfixes";
    constexpr const char* FixAimingFullTilt_Setting = "Fix Aiming On Full Tilt";

    // Gameplay
    constexpr const char* KeepAimingAfterFiring_Always_Section = "Keep Aiming After Firing";
    constexpr const char* KeepAimingAfterFiring_Always_Setting = "Always Keep Aiming After Firing";
    constexpr const char* KeepAimingAfterFiring_InFirstPerson_Section = "Keep Aiming After Firing";
    constexpr const char* KeepAimingAfterFiring_InFirstPerson_Setting = "While in First Person";
    constexpr const char* KeepAimingAfterFiring_OnLockOn_Section = "Keep Aiming After Firing";
    constexpr const char* KeepAimingAfterFiring_OnLockOn_Setting = "While Holding Lock On";

    // Tweaks
    constexpr const char* SkipIntroLogos_Section = "Skip Logo Screens";
    constexpr const char* SkipIntroLogos_Setting = "Skip In-Game Logos";
    constexpr const char* LauncherJumpStart_Section = "Skip Logo Screens";
    constexpr const char* LauncherJumpStart_Setting = "Skip Launcher Logos";
    constexpr const char* SkipLauncher_Section = "Launcher Config";
    constexpr const char* SkipLauncher_Setting = "Skip Launcher";
    constexpr const char* CtrlType_Section = "Controller Icons";
    constexpr const char* CtrlType_Setting = "Controller Type";
    constexpr const char* Language_Section = "Language Settings";
    constexpr const char* Language_Setting = "Game Language";
    constexpr const char* Region_Section = "Language Settings";
    constexpr const char* Region_Setting = "Game Region";
    constexpr const char* SkipLauncherMSXGame_Section = "Launcher Config";
    constexpr const char* SkipLauncherMSXGame_Setting = "MSXGame";
    constexpr const char* MSXWallType_Section = "Launcher Config";
    constexpr const char* MSXWallType_Setting = "MSXWallType";
    constexpr const char* MSXWallAlign_Section = "Launcher Config";
    constexpr const char* MSXWallAlign_Setting = "MSXWallAlign";

    constexpr const char* ForceStereoAudio_Section = "Various";
    constexpr const char* ForceStereoAudio_Setting = "Force Stereo Audio / Fix Codec Volume";

    constexpr const char* MuteWarning_Section = "Mute Warning";
    constexpr const char* MuteWarning_Setting = "Enabled";


    constexpr const char* DisableMouseCursor_Section = "Bugfixes";
    constexpr const char* DisableMouseCursor_Setting = "Fix Mouse Cursor Showing";

    constexpr const char* OverrideMouseSensitivity_Section = "Mouse Sensitivity";
    constexpr const char* OverrideMouseSensitivity_Setting = "Enabled";
    constexpr const char* MouseSensitivity_XMultiplier_Section = "Mouse Sensitivity";
    constexpr const char* MouseSensitivity_XMultiplier_Setting = "X Multiplier";
    constexpr const char* MouseSensitivity_YMultiplier_Section = "Mouse Sensitivity";
    constexpr const char* MouseSensitivity_YMultiplier_Setting = "Y Multiplier";

    constexpr const char* MGS2Sunglasses_Section = "MGS2 Sunglasses";
    constexpr const char* MGS2Sunglasses_Setting = "ShouldWearSunglasses";

    // Hotkeys
    constexpr const char* ToggleRainShader_Section = "Hotkeys";
    constexpr const char* ToggleRainShader_Setting = "Toggle Rain Shader";
    constexpr const char* ToggleUIShader_Section = "Hotkeys";
    constexpr const char* ToggleUIShader_Setting = "Toggle UI Shader";
    constexpr const char* CycleWireframeMode_Section = "Hotkeys";
    constexpr const char* CycleWireframeMode_Setting = "Cycle Wireframe Mode";

    // Achievements
    constexpr const char* AchievementPersistence_Section = "Bugfixes";
    constexpr const char* AchievementPersistence_Setting = "Fix Achievement Stat Tracking";

    constexpr const char* ResetAllAchievements_Section = "CAUTION - THIS BOX WILL RESET ALL ACHIEVEMENTS.";
    constexpr const char* ResetAllAchievements_Setting = "Reset All Achivements";

    // Internal
    constexpr const char* CheckForUpdates_Section = "Update Notifications";
    constexpr const char* CheckForUpdates_Setting = "CheckForUpdates";
    constexpr const char* UpdateConsoleNotifications_Section = "Update Notifications";
    constexpr const char* UpdateConsoleNotifications_Setting = "ConsoleNotifications";
}
