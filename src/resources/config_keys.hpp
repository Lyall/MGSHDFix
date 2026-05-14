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

    constexpr const char* WindowWidth_Section = "Window Settings";
    constexpr const char* WindowWidth_Setting = "Window Width";
    constexpr const char* WindowWidth_Help = "";
    constexpr const char* WindowWidth_Tooltip = "Leave this set to 0 to default to your desktop resolution.";

    constexpr const char* ForceWindowSize_Section = "Window Settings";
    constexpr const char* ForceWindowSize_Setting = "Enable Resolution Overrides";
    constexpr const char* ForceWindowSize_Help = "";
    constexpr const char* ForceWindowSize_Tooltip = "If you want MGSHDFix to force the game's window to a specific size, force its internal resolution, or enable borderless / windowed mode..";

    constexpr const char* WindowHeight_Section = "Window Settings";
    constexpr const char* WindowHeight_Setting = "Window Height";
    constexpr const char* WindowHeight_Help = "";
    constexpr const char* WindowHeight_Tooltip = "Leave this set to 0 to default to your desktop resolution.";

    constexpr const char* WindowedMode_Section = "Window Settings";
    constexpr const char* WindowedMode_Setting = "Fullscreen, Borderless, and Windowed";
    constexpr const char* WindowedMode_Help = "";
    constexpr const char* WindowedMode_Tooltip = "If the game should run:\n"
                                                 "\n"
                                                 "In Exclusive Fullscreen (Vanilla behavior)\n"
                                                 "In Borderless Fullscreen\n"
                                                 "In Borderless Windowed Mode\n"
                                                 "In Windowed Mode\n"
                                                 "\n"
                                                 "All options will maintain the internal resolution's aspect ratio.\n"
                                                 "\n"
                                                 "If you are having issues with fullscreen opening on a secondary monitor:\n"
                                                 "   1) Open the main launcher\n"
                                                 "   2) Options -> Screen -> Windowed Mode -> Set to ON\n"
                                                 "   3) Reposition the launcher window\n"
                                                 "   4) Launch the game once";
    constexpr const char* BorderlessMode_Option_BorderlessWindowed = "Borderless Windowed";
    constexpr const char* BorderlessMode_Option_BorderlessFullscreen = "Borderless Fullscreen";
    constexpr const char* BorderlessMode_Option_Windowed = "Windowed (with borders)";
    constexpr const char* BorderlessMode_Option_Fullscreen = "Exclusive Fullscreen";


    constexpr const char* ConstraintBorderlessToMonitor_Section = "Borderless Window Settings";
    constexpr const char* ConstraintBorderlessToMonitor_Setting = "Multi-Monitor Window Spanning";
    constexpr const char* ConstraintBorderlessToMonitor_Help = "";
    constexpr const char* ConstraintBorderlessToMonitor_Tooltip = "Removes the title bar and borders while keeping the game windowed.";
    constexpr const char* ConstraintBorderlessToMonitor_Option_Single_Monitor = "Constraint Window to Primary Monitor";
    constexpr const char* ConstraintBorderlessToMonitor_Option_Multi_Monitor = "Allow Window to Span Across Multiple Monitors";

    constexpr const char* RenderScaleWidth_Section = "Internal Resolution / Render Scale (+ Downsampling / Supersampling / 21:9+ and 4:3 Support)";
    constexpr const char* RenderScaleWidth_Setting = "Render Width";
    constexpr const char* RenderScaleWidth_Help = "";
    constexpr const char* RenderScaleWidth_Tooltip = "Leave 0 to use your desktop width. Otherwise, sets the internal render width.\n"
                                                     "\n"
                                                     "Any aspect ratio can be set here.\n"
                                                     "E.g. for 4:3 - 2880x2160 @ 4K, 1920x1440 @ 2K, 1440x1080 @ 1080p.\n"
                                                     "\n"
                                                     "If set wider than your monitor, the window will be clamped to maintain the internal aspect ratio.\n"
                                                     "\n"
                                                     "Original PS2 4:3 resolution was 512x448 for reference.";

    constexpr const char* RenderScaleHeight_Section = RenderScaleWidth_Section;
    constexpr const char* RenderScaleHeight_Setting = "Render Height";
    constexpr const char* RenderScaleHeight_Help = "";
    constexpr const char* RenderScaleHeight_Tooltip = "Leave 0 to use your desktop height. Otherwise sets the internal render height.\n"
                                                       "\n"
                                                       "Original PS2 4:3 resolution was 512x448 for reference.";

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


    constexpr const char* LOD_MGS2_Marines_Section = "MGS2 - Override Render Distance";
    constexpr const char* LOD_MGS2_Marines_Setting = "Force High Quality Marines";
    constexpr const char* LOD_MGS2_Marines_Help = "";
    constexpr const char* LOD_MGS2_Marines_Tooltip = "When enabled, Marines always use their high polygon model.";


    constexpr const char* LOD_MGS2_Player_Section = "MGS2 - Override Render Distance";
    constexpr const char* LOD_MGS2_Player_Setting = "Force High Quality Player Models";
    constexpr const char* LOD_MGS2_Player_Help = "";
    constexpr const char* LOD_MGS2_Player_Tooltip = "When enabled, Player models always use their high polygon model.";


    constexpr const char* DistanceCullingGrassAlways_Section = "MGS3 - Override Render Distance";
    constexpr const char* DistanceCullingGrassAlways_Setting = "Always Show Grass";
    constexpr const char* DistanceCullingGrassAlways_Help = "";
    constexpr const char* DistanceCullingGrassAlways_Tooltip = "Prevents grass from disappearing at long distances (which was originally a PS2 performance optimization).\n"
                                                               "\n"
                                                               "When enabled, grass always remains visible regardless of distance.";

    constexpr const char* DistanceCullingGrassScalar_Section = "MGS3 - Override Render Distance";
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
    constexpr const char* FixVectorRain_Section = "Bugfixes";
    constexpr const char* FixVectorRain_Setting = "Fix Rain Width";
    constexpr const char* FixVectorRain_Help = "";
    constexpr const char* FixVectorRain_Tooltip = "Fixes Rain/Lasers/Bullet Trails width, which is not scaled up properly from the original PS2 size (always appearing at only 1 pixel width regardless of game resolutions)\n"
        "\n"
        "When Fix Rain Width is enabled, an optional wireframe rendering mode becomes available - toggled by hotkey.";

    constexpr const char* FixOpticalCamo_Section = "Bugfixes";
    constexpr const char* FixOpticalCamo_Setting = "MGS2 - Fix Optical Camo Refraction";
    constexpr const char* FixOpticalCamo_Help = "";
    constexpr const char* FixOpticalCamo_Tooltip = "Improves MGS2 optical camouflage refraction and camo-failure visibility at higher resolutions.";

    constexpr const char* FixVectorUI_Section = "Bugfixes";
    constexpr const char* FixVectorUI_Setting = "Fix UI Width";
    constexpr const char* FixVectorUI_Help = "";
    constexpr const char* FixVectorUI_Tooltip = "Fixes UI line widths that were not scaled up from the original PS2 size.";

    constexpr const char* VectorLineScale_Section = "Bugfixes";
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

    constexpr const char* DisableFullscreenOptimization_Section = "System Specific Fixes";
    constexpr const char* DisableFullscreenOptimization_Setting = "Disable Windows Fullscreen Optimization";
    constexpr const char* DisableFullscreenOptimization_Help =    "(Fixes brief freezes when alt-tabbing on some systems.)";
    constexpr const char* DisableFullscreenOptimization_Tooltip = "Sets Windows compatibility settings to disable Fullscreen Optimization for the game process.\n"
                                                                  "\n"
                                                                  "Performance impact is unknown.\n"
                                                                  "Leave this OFF unless you are experiencing framerate issues when alt-tabbing and want to experiment.\n"
                                                                  "\n"
                                                                  "This writes to:\n"
                                                                  "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers\n"
                                                                  "\n"
                                                                  "Equivalent to: Right-click the game's .exe -> Properties -> Compatibility -> check \"Disable Fullscreen Optimizations\"";
    constexpr const char* BusyLoopFix_Section = "Bugfixes";
    constexpr const char* BusyLoopFix_Enabled = "Fix High CPU Usage";
    constexpr const char* BusyLoopFix_Help = "(Also fixes TWD usage on handheld.)";
    constexpr const char* BusyLoopFix_Tooltip = "Fixes High CPU & TWD usage.\n"
                                             "\n"
                                             "Patch 1.4 moved window message handling to a separate thread, but it continuously polls instead of sleeping when idle.\n"
                                             "\n"
                                             "This effectively doubles CPU usage, and on Steam Deck, also doubles total power draw.\n"
                                             "\n"
                                             "Fixing this restores normal behavior and improves GPU utilization.";


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



    constexpr const char* MGS2_ThirdPersonFreecam_Enabled_Section = "MGS2 - Third Person Freecam";
    constexpr const char* MGS2_ThirdPersonFreecam_Enabled_Setting = "Enable Third Person Freecam";
    constexpr const char* MGS2_ThirdPersonFreecam_Enabled_Help = "(MOUSE SUPPORT STILL W.I.P.)";
    constexpr const char* MGS2_ThirdPersonFreecam_Enabled_Tooltip = "Keeps aiming after firing while in first-person view.";

    constexpr const char* MGS2_ThirdPersonFreecam_ToggleKey_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_ToggleKey_Setting = "Third Person View Toggle";
    constexpr const char* MGS2_ThirdPersonFreecam_ToggleKey_Help = "(Utilizes your Steam Input binds)";
    constexpr const char* MGS2_ThirdPersonFreecam_ToggleKey_Tooltip = "Toggles the third person view on/off.";

    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Setting = "Inherit Camera Rotation";
    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Tooltip = "";

    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_ToggleKey_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_ToggleKey_Setting = "Inherit Camera Rotation Toggle";
    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_ToggleKey_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_ToggleKey_Tooltip = "";

    constexpr const char* MGS2_ThirdPersonFreecam_Max_Camera_Distance_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Max_Camera_Distance_Setting = "Max Camera Distance";
    constexpr const char* MGS2_ThirdPersonFreecam_Max_Camera_Distance_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Max_Camera_Distance_Tooltip = "";

    constexpr const char* MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Setting = "Horizontal Camera Sensitivity";
    constexpr const char* MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Tooltip = "";

    constexpr const char* MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Setting = "Vertical Camera Sensitivity";
    constexpr const char* MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Tooltip = "";

    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Increase_Hotkey_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Increase_Hotkey_Setting = "Camera Distance Increase Hotkey";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Increase_Hotkey_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Increase_Hotkey_Tooltip = "";

    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Decrease_Hotkey_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Decrease_Hotkey_Setting = "Camera Distance Decrease Hotkey";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Decrease_Hotkey_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Decrease_Hotkey_Tooltip = "";

    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Reset_Hotkey_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Reset_Hotkey_Setting = "Camera Distance Reset Hotkey";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Reset_Hotkey_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Reset_Hotkey_Tooltip = "";

    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Setting = "Camera Distance Step Amount";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Tooltip = "";



    constexpr const char* MGS2_First_Person_View_Enabled_Section = "MGS2 - First Person View Mode";
    constexpr const char* MGS2_First_Person_View_Enabled_Setting = "First Person View Mode Enabled";
    constexpr const char* MGS2_First_Person_View_Enabled_Help = "";
    constexpr const char* MGS2_First_Person_View_Enabled_Tooltip = "";

    constexpr const char* MGS2_First_Person_View_Movement_Enabled_By_Default_Section = MGS2_First_Person_View_Enabled_Section;
    constexpr const char* MGS2_First_Person_View_Movement_Enabled_By_Default_Setting = "First Person View Movement Enabled By Default";
    constexpr const char* MGS2_First_Person_View_Movement_Enabled_By_Default_Help = "";
    constexpr const char* MGS2_First_Person_View_Movement_Enabled_By_Default_Tooltip = "";

    constexpr const char* MGS2_First_Person_View_Movement_ToggleKey_Section = MGS2_First_Person_View_Enabled_Section;
    constexpr const char* MGS2_First_Person_View_Movement_ToggleKey_Setting = "First Person View Movement Toggle";
    constexpr const char* MGS2_First_Person_View_Movement_ToggleKey_Help = "";
    constexpr const char* MGS2_First_Person_View_Movement_ToggleKey_Tooltip = "";

    constexpr const char* MGS2_First_Person_View_ToggleKey_Section = MGS2_First_Person_View_Enabled_Section;
    constexpr const char* MGS2_First_Person_View_ToggleKey_Setting = "First Person View Toggle";
    constexpr const char* MGS2_First_Person_View_ToggleKey_Help = "";
    constexpr const char* MGS2_First_Person_View_ToggleKey_Tooltip = "";




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

    constexpr const char* MenuButton_Section = "Controller Settings";
    constexpr const char* MenuButton_Setting = "Set Menu OK && Cancel Button";
    constexpr const char* MenuButton_Help = "(MGS2 Only)";
    constexpr const char* MenuButton_Tooltip = "Sets which button is used for the OK action in menus.\n"
                                               "\n"
                                               "East for OK = PS2 Circle Button for Accept, X Button for Cancel\n"
                                               "\n"
                                               "South for OK = PS2 X Button for Accept, Circle Button for Cancel";
    constexpr const char* MenuButton_Option_Default = "Default";
    constexpr const char* MenuButton_Option_EastForOK = "East for OK";
    constexpr const char* MenuButton_Option_SouthForOK = "South for OK";


    constexpr const char* Language_Section = "Language Settings";
    constexpr const char* Language_Setting = "Game Language";
    constexpr const char* Language_Help = "";
    constexpr const char* Language_Tooltip = "Selects in-game language.";

    constexpr const char* Region_Section = "Language Settings";
    constexpr const char* Region_Setting = "Game Region";
    constexpr const char* Region_Help = "";
    constexpr const char* Region_Tooltip = "Selects game region.\n"
                                           "\n"
                                           "For MGS3: Europe has additional censorship VS North America.";

    constexpr const char* SkipLauncherMSXGame_Section = "Launcher Config";
    constexpr const char* SkipLauncherMSXGame_Setting = "MSX Skip Launcher Game";
    constexpr const char* SkipLauncherMSXGame_Help = "";
    constexpr const char* SkipLauncherMSXGame_Tooltip = "Which MSX game to start when Skip Launcher is enabled.";
    constexpr const char* SkipLauncherMSX_Option_MG1 = "Metal Gear (MSX)";
    constexpr const char* SkipLauncherMSX_Option_MG2 = "Metal Gear 2: Solid Snake";

    constexpr const char* ForceStereoAudio_Section = "System Specific Fixes";
    constexpr const char* ForceStereoAudio_Setting = "Audio Output Mode";
    constexpr const char* ForceStereoAudio_Help = "";
    constexpr const char* ForceStereoAudio_Tooltip = "Fixes a bug where the game always outputs 5.1 surround sound, even on stereo systems.\n"
                                                     "\n"
                                                     "When this occurs, audio may be routed to channels that do not exist, making some sounds extremely quiet or silent.\n"
                                                     "\n"
                                                     "Symptoms include quiet codec conversations, rain overpowering other audio, quiet gunshots from behind, or missing punching sounds.\n"
                                                     "\n"
                                                     "Virtual surround software (such as Razer THX or SteelSeries Sonar) is not supported.\n"
                                                     "Only systems with physical 5.1 speakers should use Surround Sound (5.1).";
    constexpr const char* ForceStereoAudio_Option_Stereo = "Stereo (2.0)";
    constexpr const char* ForceStereoAudio_Option_Surround = "Surround Sound (5.1)";

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

    constexpr const char* RestoreDogtagNames_Section = "Various";
    constexpr const char* RestoreDogtagNames_Setting = "MGS2 - Restore Original Dogtag Names";
    constexpr const char* RestoreDogtagNames_Help = "";
    constexpr const char* RestoreDogtagNames_Tooltip = "Restores the names of dogtags that were edited for copyright/legal reasons (ie Gackt -> Gekko.)";

    constexpr const char* MGS2_PhoneJingle_Section = "Various";
    constexpr const char* MGS2_PhoneJingle_Setting = "MGS2 - Restore Japanese Phone Ringtone";
    constexpr const char* MGS2_PhoneJingle_Help = "";
    constexpr const char* MGS2_PhoneJingle_Tooltip = "Restores the Japanese Sons of Liberty exclusive MGS theme-song phone ringtone that was missing from all future versions of the game.\n"
                                                     "\n"
                                                       "This will enable the ringtone for all versions of the game.";

    constexpr const char* Disable_HDC_Camera_Positions_Section = "Camera Positioning";
    constexpr const char* Disable_HDC_Camera_Positions_Setting = "Disable HD Collection Camera Positioning";
    constexpr const char* Disable_HDC_Camera_Positions_Help = "";
    constexpr const char* Disable_HDC_Camera_Positions_Tooltip = "Disables the 16:9 expanded camera positioning introduced in the HD Collection, which zoomed in the camera on most levels to prevent the sides of screen from seeing outside the bounds of the map.\n"
                                                                 "\n"
                                                                 "This results in a loss of information at the top and bottom of the screen in many areas.\n"
                                                                 "\n"
                                                                 "Toggling this option ON will return the game back to the original PS2 framing during gameplay.\n"
                                                                 "\n"
                                                                 "You -will- see outside of the map at times with this enabled.";

    constexpr const char* Disable_HDC_Camera_Positions_ToggleKey_Section = Disable_HDC_Camera_Positions_Section;
    constexpr const char* Disable_HDC_Camera_Positions_ToggleKey_Setting = "HD Collection Camera Toggle";
    constexpr const char* Disable_HDC_Camera_Positions_ToggleKey_Help = "";
    constexpr const char* Disable_HDC_Camera_Positions_ToggleKey_Tooltip = "Toggles the HD Collection Camera Positioning option on/off in real-time.\n"
                                                                        "\n"
                                                                        "In most cases, you will have to exit and re-enter the area for camera positions to update.";

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

    constexpr const char* MuteWarning_Section = "Enable Game Warnings";
    constexpr const char* MuteWarning_Setting = "Warn When Game is Muted";
    constexpr const char* MuteWarning_Help = "";
    constexpr const char* MuteWarning_Tooltip = "When enabled, a visible warning will be displayed on startup if game audio is muted via the launcher's audio settings.\n"
        "\n"
        "This is a very common troubleshooting problem.";

    constexpr const char* FSRWarning_Section = "Enable Game Warnings";
    constexpr const char* FSRWarning_Setting = "Warn When FSR Upscaling is Enabled";
    constexpr const char* FSRWarning_Help = "";
    constexpr const char* FSRWarning_Tooltip = "When enabled, a visible warning will be displayed on startup if FSR upscaling is enabled via the launcher's graphics settings.\n"
        "\n"
        "MGSHDFix already handles increasing the game's resolution.\n"
        "\n"
        "Unintended side effects, ie pixelization, mipmap issues (oversharpening on textures), and crashing, may occur while the game's built-in settings are enabled!\n"
        "\n"
        "It's advised to set both Internal Resolution & Internal Upscaling graphical options in the game's main launcher to default/original unless ABSOLUTELY necessary!";

    constexpr const char* MissingBugfixModWarning_Section = "Enable Game Warnings";
    constexpr const char* MissingBugfixModWarning_Setting = "Warn When Missing Major Bugfix Mods";
    constexpr const char* MissingBugfixModWarning_Help = "";
    constexpr const char* MissingBugfixModWarning_Tooltip = "When enabled, a visible warning will be displayed on startup if MGSHDFix is unable to locate major bugfix mods.\n"
        "\n"
        "Current Warned Mods Include:\n"
        "MGS2 Better Audio Mod (fixes a show-stopping crash during a late-game cutscene)\n"
        "\n"
        "MGS2 Community Bugfix Mod (restores missing audio, fixes thousands of textures bugs, holes in models, and localization / typo errors.)";

    constexpr const char* WindowsSlideshowWarning_Section = "Enable Game Warnings";
    constexpr const char* WindowsSlideshowWarning_Setting = "Warn When Windows Slideshow Enabled";
    constexpr const char* WindowsSlideshowWarning_Help = "";
    constexpr const char* WindowsSlideshowWarning_Tooltip = "Having Windows wallpaper set to Slideshow / Window Spotlight mode is known to cause stuttering while in DirectX games.\n"
                                                            "\n"
                                                            "This will provide a warning when the Windows setting is enabled.";

    constexpr const char* RenameOrRemoveCorruptSaveData_Section = "Damaged Steam Cloud Save Data Fix";
    constexpr const char* RenameOrRemoveCorruptSaveData_Setting = "Fix Mode";
    constexpr const char* RenameOrRemoveCorruptSaveData_Help = "";
    constexpr const char* RenameOrRemoveCorruptSaveData_Tooltip = "When fixing damaged save data (caused by Steam Cloud syncing issues), should MGSHDFix:\n"
                                                                  "\n"
                                                                  "Move outdated save data to an \"Outdated Saves\" folder as a backup copy.\n"
                                                                  "or\n"
                                                                  "Delete outdated save data.\n"
                                                                  "or\n"
                                                                  "Do nothing, and let the damaged save file remain damaged.";
    constexpr const char* RenameOrRemoveCorruptSaveData_Option_Move = "Move Outdated Save Data to Backup Folder";
    constexpr const char* RenameOrRemoveCorruptSaveData_Option_Delete = "Delete Outdated Save Data";
    constexpr const char* RenameOrRemoveCorruptSaveData_Option_Disable = "Disable Damaged Save Data Fix";

    constexpr const char* CorruptSaveData_Notification_Section = RenameOrRemoveCorruptSaveData_Section;
    constexpr const char* CorruptSaveData_Notification_Setting = "Enable Console Notification When Fixed";
    constexpr const char* CorruptSaveData_Notification_Help = "";
    constexpr const char* CorruptSaveData_Notification_Tooltip = "If a Console Notification should be shown when a save file is fixed.";


    constexpr const char* SaveFolderWriteWarning_Section = "Enable Game Warnings";
    constexpr const char* SaveFolderWriteWarning_Setting = "Warn When Save Folders Not Writable";
    constexpr const char* SaveFolderWriteWarning_Help = "";
    constexpr const char* SaveFolderWriteWarning_Tooltip = "Warn the user when the save folder is not writable by the game, which breaks the game's ability to save.";

    constexpr const char* SaveFileReadOnlyWarning_Section = "Enable Game Warnings";
    constexpr const char* SaveFileReadOnlyWarning_Setting = "Warn When Save Files Are Read-Only";
    constexpr const char* SaveFileReadOnlyWarning_Help = "";
    constexpr const char* SaveFileReadOnlyWarning_Tooltip = "Warn the user when individual save files are set to read only, which breaks the game's ability to save.";

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
    ConfigKeys::ControllerType_PS2,             //6 - custom, use ovr_ps2 folder.
};

inline const std::initializer_list<std::string> kLauncherConfigCtrlTypesInternal = { // !!! KEEP IN SYNC WITH THE LIST ABOVE !!!
    "PS5",
    "PS4",
    "XBOX",
    "NX",
    "STMD",
    "KBD", 
    "PS4" //intentional for PS2, we override the ovr_ps4 folder to search for ovr_ps2 instead.
};

struct Game_Language_Pair_View
{
    std::string_view Region_Name;
    std::string_view Language_Name;
    std::string_view Game_Region;
    std::string_view Game_Language;
};

//Config Tool -> iTargetGame = TARGET_GAME_MGS3;
inline constexpr std::array<Game_Language_Pair_View, 9> MGS3_LanguagePairs =
{ {
    { "North America", "English",   "us", "en" },
    { "North America", "French",    "us", "fr" },
    { "North America", "Spanish",   "us", "sp" },
    { "Europe",        "English",   "eu", "en" },
    { "Europe",        "French",    "eu", "fr" },
    { "Europe",        "Italian",   "eu", "it" },
    { "Europe",        "German",    "eu", "gr" },
    { "Europe",        "Spanish",   "eu", "sp" },
    { "Japan",         "Japanese",  "jp", "jp" }
} };

//Config Tool -> iTargetGame = TARGET_GAME_MG1 
//Config Tool -> iTargetGame = TARGET_GAME_MGS2
inline constexpr std::array<Game_Language_Pair_View, 6> MG1_MG2_MGS2_LanguagePairs =
{ {
    { "US / EU", "English",  "eu", "en" },
    { "US / EU", "French",   "eu", "fr" },
    { "US / EU", "Italian",  "eu", "it" },
    { "US / EU", "German",   "eu", "gr" },
    { "US / EU", "Spanish",  "eu", "sp" },
    { "Japan",   "Japanese", "jp", "jp" }
} };

template <size_t N>
static bool IsValidRegionLanguagePair(const std::array<Game_Language_Pair_View, N>& pairs, std::string_view region, std::string_view language)
{
    for (const auto& p : pairs)
    {
        if (p.Game_Region == region && p.Game_Language == language) return true;
    }
    return false;
}

template <size_t N>
static bool ResolveRegionLanguageNames(const std::array<Game_Language_Pair_View, N>& pairs, std::string_view game_region, std::string_view game_language, std::string& out_region_name, std::string& out_language_name)
{
    for (const auto& p : pairs)
    {
        if (p.Game_Region != game_region)
        {
            continue;
        }

        if (p.Game_Language != game_language)
        {
            continue;
        }

        out_region_name.assign(p.Region_Name);
        out_language_name.assign(p.Language_Name);
        return true;
    }

    return false;
}

constexpr int k3rdPersonMaxCameraDistance = 10000;
constexpr int k3rdPersonMinCameraDistance = 100;
constexpr int k3rdPersonFreecamDefaultMaxCameraDistance = 4000;
constexpr float k3rdPersonFreecamDefaultHorizontalSensitivity = 0.6f;
constexpr float k3rdPersonFreecamDefaultVerticalSensitivity = 0.4f;
