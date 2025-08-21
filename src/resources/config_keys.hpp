#pragma once
#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string>
#include <initializer_list>

namespace ConfigKeys
{
    // Graphics
    constexpr const char* ControllerType_PS5 = "PlayStation 5";
    constexpr const char* ControllerType_PS4 = "PlayStation 4";
    constexpr const char* ControllerType_XboxOne = "Xbox One";
    constexpr const char* ControllerType_NintendoSwitch = "Nintendo Switch";
    constexpr const char* ControllerType_SteamDeck = "Steam Deck";
    constexpr const char* ControllerType_KeyboardMouse = "Keyboard / Mouse";
    constexpr const char* ControllerType_PS2 = "PlayStation 2";

    constexpr const char* ForceWindowSize_Section = "Window Settings";
    constexpr const char* ForceWindowSize_Setting = "Force Window Size";
    constexpr const char* ForceWindowSize_Help = "";
    constexpr const char* ForceWindowSize_Tooltip = "";

    constexpr const char* WindowWidth_Section = "Window Settings";
    constexpr const char* WindowWidth_Setting = "Window Width";
    constexpr const char* WindowWidth_Help = "";
    constexpr const char* WindowWidth_Tooltip = "Leave this set to 0 to default to your desktop resolution.";

    constexpr const char* WindowHeight_Section = "Window Settings";
    constexpr const char* WindowHeight_Setting = "Window Height";
    constexpr const char* WindowHeight_Help = "";
    constexpr const char* WindowHeight_Tooltip = "Leave this set to 0 to default to your desktop resolution.";

    constexpr const char* WindowedMode_Section = "Window Settings";
    constexpr const char* WindowedMode_Setting = "Windowed Mode";
    constexpr const char* WindowedMode_Help = "";
    constexpr const char* WindowedMode_Tooltip = "";

    constexpr const char* BorderlessWindowed_Section = "Window Settings";
    constexpr const char* BorderlessWindowed_Setting = "Borderless Window";
    constexpr const char* BorderlessWindowed_Help = "";
    constexpr const char* BorderlessWindowed_Tooltip = "";

    constexpr const char* RenderScaleWidth_Section = "Internal Resolution";
    constexpr const char* RenderScaleWidth_Setting = "Width";
    constexpr const char* RenderScaleWidth_Help = "";
    constexpr const char* RenderScaleWidth_Tooltip = "Leave this set to 0 to default to your desktop resolution.";

    constexpr const char* RenderScaleHeight_Section = "Internal Resolution";
    constexpr const char* RenderScaleHeight_Setting = "Height";
    constexpr const char* RenderScaleHeight_Help = "";
    constexpr const char* RenderScaleHeight_Tooltip = "Leave this set to 0 to default to your desktop resolution.";

    constexpr const char* AnisotropicFiltering_Section = "Texture Filtering";
    constexpr const char* AnisotropicFiltering_Setting = "Anisotropic Filtering Level";
    constexpr const char* AnisotropicFiltering_Help = "";
    constexpr const char* AnisotropicFiltering_Tooltip = "";

    constexpr const char* DisableTextureFiltering_Section = "Texture Filtering";
    constexpr const char* DisableTextureFiltering_Setting = "Disable Texture Filtering";
    constexpr const char* DisableTextureFiltering_Help = "";
    constexpr const char* DisableTextureFiltering_Tooltip = "";

    constexpr const char* DistanceCullingGrass_Section = "Override Render Distance";
    constexpr const char* DistanceCullingGrass_Setting = "Always Show Grass";
    constexpr const char* DistanceCullingGrass_Help = "";
    constexpr const char* DistanceCullingGrass_Tooltip = "Stops grass from vanishing at long distances (a PS2 optimization.)\n"
                                                         "When enabled, grass stays visible no matter how far away you are.";



    // Ultrawide
    constexpr const char* FixAspectRatio_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FixAspectRatio_Setting = "Fix Aspect Ratio";
    constexpr const char* FixAspectRatio_Help = "";
    constexpr const char* FixAspectRatio_Tooltip = "";

    constexpr const char* FixHUD_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FixHUD_Setting = "Lock HUD && Movies to 16:9";
    constexpr const char* FixHUD_Help = "";
    constexpr const char* FixHUD_Tooltip = "Scales menus, HUD, and in-game movies to 16:9 when playing in Ultra-wide. May cause some visual glitches.";

    constexpr const char* FixFOV_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FixFOV_Setting = "Fix FOV";
    constexpr const char* FixFOV_Help = "";
    constexpr const char* FixFOV_Tooltip = "";

    constexpr const char* FramebufferFix_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FramebufferFix_Setting = "Fix Framebuffer";
    constexpr const char* FramebufferFix_Help = "(Fixes Pillarboxing issues)";
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

    constexpr const char* EnablePauseOnFocusLoss_Section = "Various";
    constexpr const char* EnablePauseOnFocusLoss_Setting = "Pause On Focus Loss";
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
    constexpr const char* CPUCoreLimit_Setting = "Limit Game to 2 CPU Cores";
    constexpr const char* CPUCoreLimit_Help = "(Fixes cutscene crashes on some newer CPUs)";
    constexpr const char* CPUCoreLimit_Tooltip = "";

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
    constexpr const char* SkipLauncherMSXGame_Setting = "MSX Skip Launcher Game";
    constexpr const char* SkipLauncherMSXGame_Help = "";
    constexpr const char* SkipLauncherMSXGame_Tooltip = "Which MSX game to launch when skip launcher is enabled.";
    constexpr const char* SkipLauncherMSX_Option_MG1 = "Metal Gear (MSX)";
    constexpr const char* SkipLauncherMSX_Option_MG2 = "Metal Gear 2: Solid Snake";

    constexpr const char* MSXWallType_Section = "Launcher Config";
    constexpr const char* MSXWallType_Setting = "MSX Wallpaper";
    constexpr const char* MSXWallType_Help = "";
    constexpr const char* MSXWallType_Tooltip = "";

    constexpr const char* MSXWallAlign_Section = "Launcher Config";
    constexpr const char* MSXWallAlign_Setting = "MSX Display Area";
    constexpr const char* MSXWallAlign_Help = "";
    constexpr const char* MSXWallAlign_Tooltip = "";
    constexpr const char* MSXWallAlign_Option_Left = "Align Left";
    constexpr const char* MSXWallAlign_Option_Right = "Align Right";
    constexpr const char* MSXWallAlign_Option_Center = "Align Center";

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
    constexpr const char* OverrideMouseSensitivity_Setting = "Override Mouse Sensitivity";
    constexpr const char* OverrideMouseSensitivity_Help = "";
    constexpr const char* OverrideMouseSensitivity_Tooltip = "Multiplies reported X/Y position of the cursor to increase sensitivity.\n"
                                                             "Higher multipliers produce more sensitivity.";

    constexpr const char* MouseSensitivity_XMultiplier_Section = "Mouse Sensitivity";
    constexpr const char* MouseSensitivity_XMultiplier_Setting = "X Multiplier";
    constexpr const char* MouseSensitivity_XMultiplier_Help = "";
    constexpr const char* MouseSensitivity_XMultiplier_Tooltip = "Multiplies reported X position of the cursor to increase sensitivity.\n"
                                                                 "Higher multipliers produce more sensitivity.";

    constexpr const char* MouseSensitivity_YMultiplier_Section = "Mouse Sensitivity";
    constexpr const char* MouseSensitivity_YMultiplier_Setting = "Y Multiplier";
    constexpr const char* MouseSensitivity_YMultiplier_Help = "";
    constexpr const char* MouseSensitivity_YMultiplier_Tooltip = "Multiplies reported Y position of the cursor to increase sensitivity.\n"
                                                                 "Higher multipliers produce more sensitivity.";

    constexpr const char* MGS2Sunglasses_Section = "Various";
    constexpr const char* MGS2Sunglasses_Setting = "MGS2 - Force Sunglasses";
    constexpr const char* MGS2Sunglasses_Help = "";
    constexpr const char* MGS2Sunglasses_Tooltip = "Forces Snake/Raiden to always wear their New Game+ Sunglasses.\n"
                                                   "\n"
                                                   "Normal = The vanilla behavior, sunglasses only worn on third playthroughs.\n"
                                                   "Always = Always force Snake/Raiden to wear sunglasses.\n"
                                                   "Never = Snake/Raiden will never wear sunglasses.";
    constexpr const char* MGS2Sunglasses_Option_Normal = "Normal";
    constexpr const char* MGS2Sunglasses_Option_Always = "Always";
    constexpr const char* MGS2Sunglasses_Option_Never = "Never";


    // Hotkeys
    constexpr const char* CaptureInputsWhileAltTabbedHotkey_Section = "Hotkeys";
    constexpr const char* CaptureInputsWhileAltTabbedHotkey_Setting = "Capture Hotkeys While Alt-Tabbed";
    constexpr const char* CaptureInputsWhileAltTabbedHotkey_Help = "";
    constexpr const char* CaptureInputsWhileAltTabbedHotkey_Tooltip = "If hotkey inputs should be captured while the window\n"
                                                                      "does NOT have focused / is alt-tabbed.";

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

    constexpr const char* ResetAllAchievements_Section = "CAUTION - THIS WILL RESET ALL ACHIEVEMENTS";
    constexpr const char* ResetAllAchievements_Setting = "Reset All Achivements";
    constexpr const char* ResetAllAchievements_Help = "";
    constexpr const char* ResetAllAchievements_Tooltip = "";

    // Internal
    constexpr const char* CheckForUpdates_Section = "Update Notifications";
    constexpr const char* CheckForUpdates_Setting = "Check For MGSHDFix Updates";
    constexpr const char* CheckForUpdates_Help = "";
    constexpr const char* CheckForUpdates_Tooltip = "";

    constexpr const char* UpdateConsoleNotifications_Section = "Update Notifications";
    constexpr const char* UpdateConsoleNotifications_Setting = "Visible Notifications";
    constexpr const char* UpdateConsoleNotifications_Help = "";
    constexpr const char* UpdateConsoleNotifications_Tooltip = "If you want a visible notification when starting the game if an MGSHDFix update is available.";

    constexpr const char* VerboseLogging_Section = "Verbose Logging";
    constexpr const char* VerboseLogging_Setting = "Enabled";
    constexpr const char* VerboseLogging_Help = "";
    constexpr const char* VerboseLogging_Tooltip = "";



}



inline const std::initializer_list<std::string> kLauncherConfigCtrlTypes = { //THESE ARE ORDER SENSITIVE.
    ConfigKeys::ControllerType_PS5,          //0
    ConfigKeys::ControllerType_PS4,             //1
    ConfigKeys::ControllerType_XboxOne,         //2
    ConfigKeys::ControllerType_NintendoSwitch,  //3
    ConfigKeys::ControllerType_SteamDeck,       //4
    ConfigKeys::ControllerType_KeyboardMouse,   //5
    ConfigKeys::ControllerType_PS2,             //6
};

inline const std::initializer_list<std::string> kLauncherConfigLanguages = {
    "English",
    "Japanese",
    "French",
    "German",
    "Italian",
    "Portuguese",
    "Spanish",
    "Dutch",
    "Russian"
};

inline const std::initializer_list<std::string> kLauncherConfigRegions = {
    "United States",
    "Japan",
    "Europe"
};
