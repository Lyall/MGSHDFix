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
    constexpr const char* ForceWindowSize_Setting = "Set Window Size";
    constexpr const char* ForceWindowSize_Help = "";
    constexpr const char* ForceWindowSize_Tooltip = "If you want MGSHDFix to force the game's window to a specific size.";

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
    constexpr const char* WindowedMode_Tooltip = "Runs the game in a window instead of exclusive fullscreen.";

    constexpr const char* BorderlessWindowed_Section = "Window Settings";
    constexpr const char* BorderlessWindowed_Setting = "Borderless Window";
    constexpr const char* BorderlessWindowed_Help = "";
    constexpr const char* BorderlessWindowed_Tooltip = "Removes the title bar and borders while keeping the game windowed.";

    constexpr const char* RenderScaleWidth_Section = "Internal Resolution";
    constexpr const char* RenderScaleWidth_Setting = "Width";
    constexpr const char* RenderScaleWidth_Help = "";
    constexpr const char* RenderScaleWidth_Tooltip = "Leave 0 to use your desktop width. Otherwise sets the internal render width.";

    constexpr const char* RenderScaleHeight_Section = "Internal Resolution";
    constexpr const char* RenderScaleHeight_Setting = "Height";
    constexpr const char* RenderScaleHeight_Help = "";
    constexpr const char* RenderScaleHeight_Tooltip = "Leave 0 to use your desktop height. Otherwise sets the internal render height.";

    constexpr const char* AnisotropicFiltering_Section = "Texture Filtering";
    constexpr const char* AnisotropicFiltering_Setting = "Anisotropic Filtering Level";
    constexpr const char* AnisotropicFiltering_Help = "";
    constexpr const char* AnisotropicFiltering_Tooltip = "Controls the level of anisotropic filtering applied to textures.\n"
                                                         "\n"
                                                         "Higher values improve texture detail while far away or at oblique angles.";

    constexpr const char* DisableTextureFiltering_Section = "Texture Filtering";
    constexpr const char* DisableTextureFiltering_Setting = "Nearest Neighbor Texture Filtering";
    constexpr const char* DisableTextureFiltering_Help = "";
    constexpr const char* DisableTextureFiltering_Tooltip = "Disables all texture filtering to use nearest neighbor sampling.\n"
                                                            "\n"
                                                            "This will give the game a pixelated / Minecraft-esque appearance.";

    constexpr const char* DistanceCullingGrassAlways_Section = "Override Render Distance";
    constexpr const char* DistanceCullingGrassAlways_Setting = "Always Show Grass";
    constexpr const char* DistanceCullingGrassAlways_Help = "";
    constexpr const char* DistanceCullingGrassAlways_Tooltip = "Prevents grass from disappearing at long distances (which was originally a PS2 performance optimization).\n"
                                                               "\n"
                                                               "When enabled, grass always remains visible regardless of distance.";

    constexpr const char* DistanceCullingGrassScalar_Section = "Override Render Distance";
    constexpr const char* DistanceCullingGrassScalar_Setting = "Custom Grass Distance Multiplier";
    constexpr const char* DistanceCullingGrassScalar_Help = "";
    constexpr const char* DistanceCullingGrassScalar_Tooltip = "Multiplies the grass render distance by this factor for finer control.\n"
                                                               "\n"
                                                               "1.0 is the unmodified distance.";

    // Ultrawide
    constexpr const char* FixAspectRatio_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FixAspectRatio_Setting = "Fix Aspect Ratio";
    constexpr const char* FixAspectRatio_Help = "";
    constexpr const char* FixAspectRatio_Tooltip = "Fixes aspect ratio and removes pillarboxing in MGS2/MGS3.";

    constexpr const char* FixHUD_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FixHUD_Setting = "Lock HUD && Movies to 16:9";
    constexpr const char* FixHUD_Help = "";
    constexpr const char* FixHUD_Tooltip = "Scales menus, HUD, and in-game movies to 16:9 when using ultrawide.\n"
                                           "\n"
                                           "May cause minor visual glitches.";

    constexpr const char* FixFOV_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FixFOV_Setting = "Fix FOV";
    constexpr const char* FixFOV_Help = "";
    constexpr const char* FixFOV_Tooltip = "Converts FOV to vert- and matches 16:9 horizontal FOV at narrower aspect ratios.\n"
                                           "\n"
                                           "Only applies below 16:9. Automatically disabled if aspect ratio is wider than 16:9.";

    constexpr const char* FramebufferFix_Section = "Ultra-Wide / 16:10+";
    constexpr const char* FramebufferFix_Setting = "Fix Framebuffer";
    constexpr const char* FramebufferFix_Help = "(Fixes Pillarboxing issues)";
    constexpr const char* FramebufferFix_Tooltip = "Forces the framebuffer size to be the same as the custom resolution.\n"
                                                   "\n"
                                                   "Disable if you prefer pillarboxing or letterboxing with a custom resolution.";

    // Bugfixes
    constexpr const char* FixVectorRain_Section = "Vector Line Scaling Fix";
    constexpr const char* FixVectorRain_Setting = "Fix Rain Width";
    constexpr const char* FixVectorRain_Help = "";
    constexpr const char* FixVectorRain_Tooltip = "Fixes Rain/Lasers/Bullet Trails width, which is not scaled up properly from the original PS2 size (always appearing at only 1 pixel width regardless of game resolutions)\n"
                                                  "\n"
                                                  "When Fix Rain Width is enabled, an optional wireframe rendering mode becomes available - toggled by hotkey.";

    constexpr const char* FixVectorUI_Section = "Vector Line Scaling Fix";
    constexpr const char* FixVectorUI_Setting = "Fix UI Width";
    constexpr const char* FixVectorUI_Help = "";
    constexpr const char* FixVectorUI_Tooltip = "Fixes UI line widths that were not scaled up from the original PS2 size.";

    constexpr const char* VectorLineScale_Section = "Vector Line Scaling Fix";
    constexpr const char* VectorLineScale_Setting = "Line Scale Size";
    constexpr const char* VectorLineScale_Help = "(360 = Accurate to PCSX2)";
    constexpr const char* VectorLineScale_Tooltip = "Lower numbers increase the width of vector/line effects.\n"
                                                    "\n"
                                                    "You can calculate the scale as Screen Height / Desired Pixel Width, ie (1080 Resolution / 4 Pixel Width = 270 Scale).\n"
                                                    "\n"
                                                    "The number will be automatically adjusted to the nearest whole pixel, don't worry about decimals.\n"
                                                    "\n"
                                                    "360 scale is pixel-accurate to PCSX2's corrected line widths across all resolutions - 1 pixel @ 448/OG PS2 Res, 2 @ 720p, 3 @ 1080p, 4 @ 1440, 6 @ 2160.";

    constexpr const char* EffectSpeedFixes_Section = "Bugfixes";
    constexpr const char* EffectSpeedFixes_Setting = "Fix Effect Speeds";
    constexpr const char* EffectSpeedFixes_Help = "";
    constexpr const char* EffectSpeedFixes_Tooltip = "Fixes various effects throughout MGS2 & MGS3 which originally had their durations tuned for the PS2's FPS slowdowns during intense cutscenes, "
                                                     "resulting in them running at double (or higher) their intended speed & ending early on modern / more powerful hardware.";

    constexpr const char* EnablePauseOnFocusLoss_Section = "Various";
    constexpr const char* EnablePauseOnFocusLoss_Setting = "Pause On Focus Loss";
    constexpr const char* EnablePauseOnFocusLoss_Help = "";
    constexpr const char* EnablePauseOnFocusLoss_Tooltip = "Pauses the game when the window loses focus (alt-tabbed)";

    constexpr const char* PauseOnFocusLoss_SpeedrunnerBugfixOverride_Section = "Bugfixes";
    constexpr const char* PauseOnFocusLoss_SpeedrunnerBugfixOverride_Setting = "Fix Alt-Tab Loading Bugs";
    constexpr const char* PauseOnFocusLoss_SpeedrunnerBugfixOverride_Help = "";
    constexpr const char* PauseOnFocusLoss_SpeedrunnerBugfixOverride_Tooltip = "Ensures assets load correctly during level transitions and cutscenes when alt-tabbed.\n"
                                                                               "\n"
                                                                               "Prevents softlocks (e.g., doors not loading) and crash-prone timers.\n"
                                                                               "Note: speedrunners sometimes rely on the original bug to skip codec calls.";

    constexpr const char* FixAimingAfterEquip_Section = "Bugfixes";
    constexpr const char* FixAimingAfterEquip_Setting = "Fix Aiming After Equip";
    constexpr const char* FixAimingAfterEquip_Help = "";
    constexpr const char* FixAimingAfterEquip_Tooltip = "Prevents auto-aiming immediately after re-equipping a weapon that was holstered while drawn.";

    constexpr const char* FixAimingFullTilt_Section = "Bugfixes";
    constexpr const char* FixAimingFullTilt_Setting = "Fix Aiming On Full Tilt";
    constexpr const char* FixAimingFullTilt_Help = "";
    constexpr const char* FixAimingFullTilt_Tooltip = "In MGS2, prevents aiming from dropping when tilting the analog stick fully while holding Lock-On / L1.";

    constexpr const char* CPUCoreLimit_Section = "System Specific Fixes";
    constexpr const char* CPUCoreLimit_Setting = "Limit Game to 2 CPU Cores";
    constexpr const char* CPUCoreLimit_Help = "(Fixes cutscene crashes on some newer CPUs)";
    constexpr const char* CPUCoreLimit_Tooltip = "Workaround for uncommon driver related cutscene crashes on newer CPUs.\n"
                                                 "\n"
                                                 "This will impact performance, only enable if you're actively experiencing crashing in the middle of cutscenes.";

    // Gameplay
    constexpr const char* KeepAimingAfterFiring_Always_Section = "Keep Aiming After Firing";
    constexpr const char* KeepAimingAfterFiring_Always_Setting = "Always Keep Aiming";
    constexpr const char* KeepAimingAfterFiring_Always_Help = "";
    constexpr const char* KeepAimingAfterFiring_Always_Tooltip = "Keeps aiming after firing, always.\n"
                                                                 "\n"
                                                                 "Matches the older KeepAiming mod behavior. Can be awkward to play with.\n"
                                                                 "Suggestion: leave this off and use the First-Person and Lock-On options instead.\n"
                                                                 "\n"
                                                                 "(MGS2 support is limited to rifles. All guns supported in MGS3.)";

    constexpr const char* KeepAimingAfterFiring_InFirstPerson_Section = "Keep Aiming After Firing";
    constexpr const char* KeepAimingAfterFiring_InFirstPerson_Setting = "While in First Person";
    constexpr const char* KeepAimingAfterFiring_InFirstPerson_Help = "";
    constexpr const char* KeepAimingAfterFiring_InFirstPerson_Tooltip = "Keeps aiming after firing while in first-person view.";

    constexpr const char* KeepAimingAfterFiring_OnLockOn_Section = "Keep Aiming After Firing";
    constexpr const char* KeepAimingAfterFiring_OnLockOn_Setting = "While Holding Lock On";
    constexpr const char* KeepAimingAfterFiring_OnLockOn_Help = "";
    constexpr const char* KeepAimingAfterFiring_OnLockOn_Tooltip = "Keeps aiming after firing while holding Lock-On (L1).";

    // Tweaks
    constexpr const char* SkipIntroLogos_Section = "Skip Logo Screens";
    constexpr const char* SkipIntroLogos_Setting = "Skip In-Game Splashscreens";
    constexpr const char* SkipIntroLogos_Help = "";
    constexpr const char* SkipIntroLogos_Tooltip = "Skips the unskippable \"KONAMI\" splashscreens on startup. Skippable intro videos will still be played.\n"
                                                   "\n"
                                                   "(Only supports MGS2 & MGS3.)";

    constexpr const char* LauncherJumpStart_Section = "Skip Logo Screens";
    constexpr const char* LauncherJumpStart_Setting = "Skip Launcher Splashscreens";
    constexpr const char* LauncherJumpStart_Help = "";
    constexpr const char* LauncherJumpStart_Tooltip = "Skips launcher splash screens and menus and jumps directly to the launch game screen.";

    constexpr const char* SkipLauncher_Section = "Launcher Config";
    constexpr const char* SkipLauncher_Setting = "Skip Launcher";
    constexpr const char* SkipLauncher_Help = "";
    constexpr const char* SkipLauncher_Tooltip = "Skips the launcher app and runs the game directly.";

    constexpr const char* CtrlType_Section = "Controller Settings";
    constexpr const char* CtrlType_Setting = "Button Icons";
    constexpr const char* CtrlType_Help = "";
    constexpr const char* CtrlType_Tooltip = "Selects which controller button icons to display in-game.";

    constexpr const char* Language_Section = "Language Settings";
    constexpr const char* Language_Setting = "Game Language";
    constexpr const char* Language_Help = "";
    constexpr const char* Language_Tooltip = "Selects in-game language. Availability may depend on region.";

    constexpr const char* Region_Section = "Language Settings";
    constexpr const char* Region_Setting = "Game Region";
    constexpr const char* Region_Help = "";
    constexpr const char* Region_Tooltip = "(MGS3 Only)";

    constexpr const char* SkipLauncherMSXGame_Section = "Launcher Config";
    constexpr const char* SkipLauncherMSXGame_Setting = "MSX Skip Launcher Game";
    constexpr const char* SkipLauncherMSXGame_Help = "";
    constexpr const char* SkipLauncherMSXGame_Tooltip = "Which MSX game to start when Skip Launcher is enabled.";
    constexpr const char* SkipLauncherMSX_Option_MG1 = "Metal Gear (MSX)";
    constexpr const char* SkipLauncherMSX_Option_MG2 = "Metal Gear 2: Solid Snake";

    constexpr const char* MSXWallType_Section = "Launcher Config";
    constexpr const char* MSXWallType_Setting = "MSX Wallpaper";
    constexpr const char* MSXWallType_Help = "";
    constexpr const char* MSXWallType_Tooltip = "Which wallpaper to use while playing the MSX games.\n"
                                                "\n"
                                                "You can see which image corresponds to each number in the main launcher's options.";

    constexpr const char* MSXWallAlign_Section = "Launcher Config";
    constexpr const char* MSXWallAlign_Setting = "MSX Display Area";
    constexpr const char* MSXWallAlign_Help = "";
    constexpr const char* MSXWallAlign_Tooltip = "If you want the gameplay window of the MSX games aligned to the left, right, or center of the screen.";
    constexpr const char* MSXWallAlign_Option_Left = "Align Left";
    constexpr const char* MSXWallAlign_Option_Right = "Align Right";
    constexpr const char* MSXWallAlign_Option_Center = "Align Center";

    constexpr const char* ForceStereoAudio_Section = "System Specific Fixes";
    constexpr const char* ForceStereoAudio_Setting = "Force Stereo Audio Output";
    constexpr const char* ForceStereoAudio_Help = "(Fixes codec / radio conversation volume)";
    constexpr const char* ForceStereoAudio_Tooltip = "Forces stereo output if Windows (and SteamDeck) is incorrectly set to 5.1+, which makes the game send audio to non-existent channels."
                                                     "\n"
                                                     "For example, codec / radio conversations will be FAR QUIETER than other sound effects (such as rain), or outright missing throughout the game.\n"
                                                     "\n"
                                                     "Razer Synapse's THX Virtualization and SteelSeries Sonar's ChatMix features are known to cause this.\n"
                                                     "\n"
                                                     "This is a workaround for a system configuration issue. Other games will still be affected.\n"
                                                     "Fix your audio device setup if possible.";


    constexpr const char* MuteWarning_Section = "Mute Warning";
    constexpr const char* MuteWarning_Setting = "Enabled";
    constexpr const char* MuteWarning_Help = "";
    constexpr const char* MuteWarning_Tooltip = "When enabled, a visible warning will be displayed on startup if game audio is muted via the launcher's audio settings.";

    constexpr const char* DisableMouseCursor_Section = "Bugfixes";
    constexpr const char* DisableMouseCursor_Setting = "Fix Mouse Cursor Showing";
    constexpr const char* DisableMouseCursor_Help = "";
    constexpr const char* DisableMouseCursor_Tooltip = "Stops the mouse cursor from showing in the launcher and game.";

    constexpr const char* OverrideMouseSensitivity_Section = "Mouse Sensitivity";
    constexpr const char* OverrideMouseSensitivity_Setting = "Override Mouse Sensitivity";
    constexpr const char* OverrideMouseSensitivity_Help = "";
    constexpr const char* OverrideMouseSensitivity_Tooltip = "Multiplies reported X/Y position of the cursor to increase sensitivity.\n"
                                                             "\n"
                                                             "Higher multipliers produce more sensitivity.";

    constexpr const char* MouseSensitivity_XMultiplier_Section = "Mouse Sensitivity";
    constexpr const char* MouseSensitivity_XMultiplier_Setting = "X Multiplier";
    constexpr const char* MouseSensitivity_XMultiplier_Help = "";
    constexpr const char* MouseSensitivity_XMultiplier_Tooltip = "Multiplies reported X position of the cursor to increase sensitivity.\n"
                                                                 "\n"
                                                                 "Higher multipliers produce more sensitivity.";

    constexpr const char* MouseSensitivity_YMultiplier_Section = "Mouse Sensitivity";
    constexpr const char* MouseSensitivity_YMultiplier_Setting = "Y Multiplier";
    constexpr const char* MouseSensitivity_YMultiplier_Help = "";
    constexpr const char* MouseSensitivity_YMultiplier_Tooltip = "Multiplies reported Y position of the cursor to increase sensitivity.\n"
                                                                 "\n"
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
    constexpr const char* CaptureInputsWhileAltTabbedHotkey_Tooltip = "Capture hotkeys even when the window is not focused or is alt-tabbed.";

    constexpr const char* ToggleRainShader_Section = "Hotkeys";
    constexpr const char* ToggleRainShader_Setting = "Toggle Rain Shader";
    constexpr const char* ToggleRainShader_Help = "";
    constexpr const char* ToggleRainShader_Tooltip = "Toggles the rain/laser width fix on/off in real-time.";

    constexpr const char* ToggleUIShader_Section = "Hotkeys";
    constexpr const char* ToggleUIShader_Setting = "Toggle UI Shader";
    constexpr const char* ToggleUIShader_Help = "";
    constexpr const char* ToggleUIShader_Tooltip = "Toggles the UI line width fix on/off in real-time.";

    constexpr const char* CycleWireframeMode_Section = "Hotkeys";
    constexpr const char* CycleWireframeMode_Setting = "Cycle Wireframe Mode";
    constexpr const char* CycleWireframeMode_Help = "";
    constexpr const char* CycleWireframeMode_Tooltip = "Cycle between wireframe rendering modes (available when Rain Width Fix is enabled).";


    constexpr const char* ToggleDistanceCullingGrass_Section = "Hotkeys";
    constexpr const char* ToggleDistanceCullingGrass_Setting = "Toggle Always Show Grass";
    constexpr const char* ToggleDistanceCullingGrass_Help = "";
    constexpr const char* ToggleDistanceCullingGrass_Tooltip = "Toggles the Always Show Grass option on/off in real-time, for those who want comparison shots.\n"
                                                               "\n"
                                                               "You may have to exit and reenter an area for the change to take effect.";

    // Achievements
    constexpr const char* AchievementPersistence_Section = "Bugfixes";
    constexpr const char* AchievementPersistence_Setting = "Fix Achievement Stat Tracking";
    constexpr const char* AchievementPersistence_Help = "";
    constexpr const char* AchievementPersistence_Tooltip = "When enabled, stats that count towards unlocking achievements (e.g., necks broken, guards tranquilized, "
                                                           "hidden R1 First Person cutscenes watched, ect.) will be automatically synchronized to Steam Cloud.\n"
                                                           "\n"
                                                           "(The game normally doesn't save any of these stats when you close the game.)";

    constexpr const char* ResetAllAchievements_Section = "CAUTION - THIS WILL RESET ALL ACHIEVEMENTS";
    constexpr const char* ResetAllAchievements_Setting = "Reset All Achievements";
    constexpr const char* ResetAllAchievements_Help = "";
    constexpr const char* ResetAllAchievements_Tooltip = "Will RESET all your achievements for the specific game you launch.\n"
                                                         "\n"
                                                         "THIS IS IRREVERSIBLE. You'll have to unlock all your achievements over again, so be really super duper sure about it!";

    // Internal
    constexpr const char* CheckForUpdates_Section = "Update Notifications";
    constexpr const char* CheckForUpdates_Setting = "Check For MGSHDFix Updates";
    constexpr const char* CheckForUpdates_Help = "";
    constexpr const char* CheckForUpdates_Tooltip = "If MGSHDFix should notify you when launching the game if a new MGSHDFix update is available for download.";

    constexpr const char* UpdateConsoleNotifications_Section = "Update Notifications";
    constexpr const char* UpdateConsoleNotifications_Setting = "In-Game Update Notifications";
    constexpr const char* UpdateConsoleNotifications_Help = "";
    constexpr const char* UpdateConsoleNotifications_Tooltip = "If you want a visible notification when starting the game if an MGSHDFix update is available.\n"
                                                               "\n"
                                                               "Notifications will still be printed to the log file while disabled.";

    constexpr const char* VerboseLogging_Section = "Internal Settings";
    constexpr const char* VerboseLogging_Setting = "Debug Logging";
    constexpr const char* VerboseLogging_Help = "";
    constexpr const char* VerboseLogging_Tooltip = "Enables verbose logging for debugging purposes.";

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
