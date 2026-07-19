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



    constexpr const char* ColorCorrection_Enabled_Section = "Enhancements and Tweaks";
    constexpr const char* ColorCorrection_Enabled_Setting = "Correct Gamma Levels";
    constexpr const char* ColorCorrection_Enabled_Help = "";
    constexpr const char* ColorCorrection_Enabled_Tooltip = "Corrects gamma levels to more closely match the original presentation on a CRT.";


    constexpr const char* DisableTextureFiltering_Section = "Enhancements and Tweaks";
    constexpr const char* DisableTextureFiltering_Setting = "Nearest Neighbor Texture Filtering";
    constexpr const char* DisableTextureFiltering_Help = "";
    constexpr const char* DisableTextureFiltering_Tooltip = "Disables all texture filtering to use nearest neighbor sampling.\n"
                                                            "\n"
                                                            "This will give the game a pixelated / Minecraft-esque appearance.";


    constexpr const char* EnableSMAA_Section = "Enhancements and Tweaks";
    constexpr const char* EnableSMAA_Setting = "Enable SMAA Anti-Aliasing";
    constexpr const char* EnableSMAA_Help = "";
    constexpr const char* EnableSMAA_Tooltip = "Subpixel Morphological Anti-Aliasing.";


    constexpr const char* AnisotropicFiltering_Section = "Enhancements and Tweaks";
    constexpr const char* AnisotropicFiltering_Setting = "Anisotropic Filtering Level";
    constexpr const char* AnisotropicFiltering_Help = "";
    constexpr const char* AnisotropicFiltering_Tooltip = "Controls the level of anisotropic filtering applied to textures.\n"
        "\n"
        "Higher values improve texture detail while far away or at oblique angles.";

    constexpr const char* MG1_Crop_Overscan_Enabled_Section = "Enhancements and Tweaks";
    constexpr const char* MG1_Crop_Overscan_Enabled_Setting = "Crop Overscan Area";
    constexpr const char* MG1_Crop_Overscan_Enabled_Help = "";
    constexpr const char* MG1_Crop_Overscan_Enabled_Tooltip = "Enables cropping of overscan areas in MG1.";


    constexpr const char* MG1_Correct_Aspect_Ratio_Enabled_Section = "Enhancements and Tweaks";
    constexpr const char* MG1_Correct_Aspect_Ratio_Enabled_Setting = "Correct Aspect Ratio to 4:3";
    constexpr const char* MG1_Correct_Aspect_Ratio_Enabled_Help = "";
    constexpr const char* MG1_Correct_Aspect_Ratio_Enabled_Tooltip = "Corrects the viewport's dimensions from the MSX2's raw internal aspect ratio (64:53 / 256x212) to the intended CRT aspect ratio of 4:3.";


    constexpr const char* Caption_Scale_Section = "Caption Settings";
    constexpr const char* Caption_Scale_Setting = "Caption Size (%)";
    constexpr const char* Caption_Scale_Help = "";
    constexpr const char* Caption_Scale_Tooltip = "";

    constexpr const char* Caption_Opacity_Section = "Caption Settings";
    constexpr const char* Caption_Opacity_Setting = "Caption Opacity (%)";
    constexpr const char* Caption_Opacity_Help = "";
    constexpr const char* Caption_Opacity_Tooltip = "";

    constexpr const char* Caption_Background_Opacity_Section = "Caption Settings";
    constexpr const char* Caption_Background_Opacity_Setting = "Caption Outline Opacity (%)";
    constexpr const char* Caption_Background_Opacity_Help = "30% Recommended";
    constexpr const char* Caption_Background_Opacity_Tooltip = "";



    constexpr const char* MGS2_SnakeTales_Radar_Section = "Various";
    constexpr const char* MGS2_SnakeTales_Radar_Setting = "Enable Radar in Snake Tales";
    constexpr const char* MGS2_SnakeTales_Radar_Help = "";
    constexpr const char* MGS2_SnakeTales_Radar_Tooltip = "Enables Radar in Snake Tales / Alternative missions.";


    constexpr const char* LOD_MGS2_ShellCasings_Section = "Model Quality && Level of Detail Enhancements";
    constexpr const char* LOD_MGS2_ShellCasings_Setting = "Always Show Weapon Shell Casings";
    constexpr const char* LOD_MGS2_ShellCasings_Help = "";
    constexpr const char* LOD_MGS2_ShellCasings_Tooltip = "Shell casings normally get culled if height is > 3500 units from the camera.\n"
                                                          "\n"
                                                          "When enabled, shell casings will be rendered at all distances.\n";


    constexpr const char* LOD_MGS2_NPC_Section = "Model Quality && Level of Detail Enhancements";
    constexpr const char* LOD_MGS2_NPC_Setting = "Force High Quality Characters";
    constexpr const char* LOD_MGS2_NPC_Help = "";
    constexpr const char* LOD_MGS2_NPC_Tooltip = "When enabled, all character models always use their high polygon model.";

    constexpr const char* MGS2_Hostage_Type_Section = "Speedrunner Settings";
    constexpr const char* MGS2_Hostage_Type_Setting = "Force RTC Hostage Type";
    constexpr const char* MGS2_Hostage_Type_Help = "";
    constexpr const char* MGS2_Hostage_Type_Tooltip = "The game swaps which hostages are in Shell 1 core based off your system's real time clock in New Game+ playthroughs.\n"
                                                      "\n"
                                                      "Normal = use normal RTC hostages.\n"
                                                      "\n"
                                                      "1:00 PM = All hostages are Kato-chan (Japanese comedian.)\n"
                                                      "\n"
                                                      "10:00 PM = All hostages are Cathy.\n"
                                                      "\n"
                                                      "Midnight = All hostages are Jennifer Love Hewitt.\n";
    constexpr const char* MGS2_Hostage_Type_Option_Normal = "Normal";
    constexpr const char* MGS2_Hostage_Type_Option_OnePM = "Kato-chan";
    constexpr const char* MGS2_Hostage_Type_Option_TenPM = "Old Beauties";
    constexpr const char* MGS2_Hostage_Type_Option_Midnight = "Jennifer";

    constexpr const char* MGS2_Thermal_Mode_Section = "Various";
    constexpr const char* MGS2_Thermal_Mode_Setting = "Thermal Goggle Palette Swapping";
    constexpr const char* MGS2_Thermal_Mode_Help = "";
    constexpr const char* MGS2_Thermal_Mode_Tooltip = "Enable swapping what color palettes are used by the thermal goggles. Cycle in-game with the hotkey.";

    constexpr const char* MGS2_Thermal_Default_Mode_Section = "Various";
    constexpr const char* MGS2_Thermal_Default_Mode_Setting = "Thermal Goggle Default Palette";
    constexpr const char* MGS2_Thermal_Default_Mode_Help = "";
    constexpr const char* MGS2_Thermal_Default_Mode_Tooltip = "Default color palette used by the thermal goggles when the game starts.\n"
                                                                "\n"
                                                                "Substance = Vanilla / Unmodified Substance Colors\n"
                                                                "Sons of Liberty = Red Hot / Terminator Vision\n"
                                                                "Splinter Cell (Blacklist) = Modernized Red Hot -> Purple Cold";
    constexpr const char* MGS2_Thermal_Default_Mode_Option_Substance = "Substance";
    constexpr const char* MGS2_Thermal_Default_Mode_Option_RedHot = "Sons of Liberty";
    constexpr const char* MGS2_Thermal_Default_Mode_Option_SplinterCell = "Splinter Cell";
    constexpr const char* MGS2_Thermal_Default_Mode_Option_WhiteHot = "White Hot";
    constexpr const char* MGS2_Thermal_Default_Mode_Option_BlackHot = "Black Hot";

    constexpr const char* MGS2_Thermal_Cycle_Hotkey_Section = "Various";
    constexpr const char* MGS2_Thermal_Cycle_Hotkey_Setting = "T.Goggle Color Cycle Hotkey";
    constexpr const char* MGS2_Thermal_Cycle_Hotkey_Help = "";
    constexpr const char* MGS2_Thermal_Cycle_Hotkey_Tooltip = "Hotkey to cycle through color palettes in realtime.";


    constexpr const char* DistanceCullingGrassAlways_Section = "Model Quality && Level of Detail Enhancements";
    constexpr const char* DistanceCullingGrassAlways_Setting = "Always Show Grass";
    constexpr const char* DistanceCullingGrassAlways_Help = "";
    constexpr const char* DistanceCullingGrassAlways_Tooltip = "Prevents grass from disappearing at long distances (which was originally a PS2 performance optimization).\n"
                                                               "\n"
                                                               "When enabled, grass always remains visible regardless of distance.";

    constexpr const char* DistanceCullingGrassScalar_Section = DistanceCullingGrassAlways_Section;
    constexpr const char* DistanceCullingGrassScalar_Setting = "Custom Grass Distance Multiplier";
    constexpr const char* DistanceCullingGrassScalar_Help = "";
    constexpr const char* DistanceCullingGrassScalar_Tooltip = "Multiplies the grass render distance by this factor for finer control.\n"
                                                               "\n"
                                                               "1.0 is the unmodified distance.";

    constexpr const char* ToggleDistanceCullingGrass_Section = DistanceCullingGrassAlways_Section;
    constexpr const char* ToggleDistanceCullingGrass_Setting = "Toggle Always Show Grass";
    constexpr const char* ToggleDistanceCullingGrass_Help = "";
    constexpr const char* ToggleDistanceCullingGrass_Tooltip = "Toggles the Always Show Grass option on/off in real-time, for those who want comparison shots.\n"
        "\n"
        "You may have to exit and reenter an area for the change to take effect.";

    constexpr const char* MGS2_Lifebar_Name_Use_Custom_Section = "Various";
    constexpr const char* MGS2_Lifebar_Name_Use_Custom_Setting = "Use Custom Lifebar Name";
    constexpr const char* MGS2_Lifebar_Name_Use_Custom_Help = "";
    constexpr const char* MGS2_Lifebar_Name_Use_Custom_Tooltip = "When enabled, the game will use the custom name below for the lifebar";

    constexpr const char* MGS2_Lifebar_Name_Custom_Section = "Various";
    constexpr const char* MGS2_Lifebar_Name_Custom_Setting = "Custom Lifebar Name";
    constexpr const char* MGS2_Lifebar_Name_Custom_Help = "";
    constexpr const char* MGS2_Lifebar_Name_Custom_Tooltip = "Custom name to use for the lifebar.";
    
    constexpr const char* MGS2_Lifebar_Name_Use_Character_Names_Section = "Various";
    constexpr const char* MGS2_Lifebar_Name_Use_Character_Names_Setting = "Use Character Names for Lifebar";
    constexpr const char* MGS2_Lifebar_Name_Use_Character_Names_Help = "";
    constexpr const char* MGS2_Lifebar_Name_Use_Character_Names_Tooltip = "When enabled, the game will use actual character names (ie \"Raiden\" / \"Snake\") for the lifebar instead of saying \"LIFE\", matching later entries in the MGS series.";

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
    constexpr const char* ToggleRainShader_Section = "Hotkeys";
    constexpr const char* ToggleRainShader_Setting = "Toggle Vector Line Fixes";
    constexpr const char* ToggleRainShader_Help = "";
    constexpr const char* ToggleRainShader_Tooltip = "Toggles the rain/laser width fix on/off in real-time for comparison shots.";


    constexpr const char* FixDepthOfField_Section = "Bugfixes";
    constexpr const char* FixDepthOfField_Setting = "Fix Depth of Field";
    constexpr const char* FixDepthOfField_Help = "(May Impact Performance)";
    constexpr const char* FixDepthOfField_Tooltip = "Restores depth of field blur at higher resolutions.\n"
                                                    "\n"
                                                    "Also restores close-up depth of field / camera blur, which was outright disabled/broken by the HD Collection.";


    constexpr const char* MotionBlur_Section = "Bugfixes";
    constexpr const char* MotionBlur_Setting = "Fix Motion Trails";
    constexpr const char* MotionBlur_Help = "";
    constexpr const char* MotionBlur_Tooltip = "Restores the game's built-in motion blur/trails, which was broken by the HD Collection.\n"
                                               "\n"
                                               "This effect can seem a little strong / distracting at higher resolutions.";


    constexpr const char* MGS3DepthOfFieldBlurUvMultiplier_Section = FixDepthOfField_Section;
    constexpr const char* MGS3DepthOfFieldBlurUvMultiplier_Setting = "Depth of Field Blur Strength";
    constexpr const char* MGS3DepthOfFieldBlurUvMultiplier_Help = "";
    constexpr const char* MGS3DepthOfFieldBlurUvMultiplier_Tooltip = "Scales the UV radius used by MGS3's depth of field blur.\n"
                                                                     "\n"
                                                                     "Higher values increase blur radius but can expose sampling artifacts.";

    constexpr const char* Restore_Reverb_Level_Section = "Bugfixes";
    constexpr const char* Restore_Reverb_Level_Setting = "Boost Reverb Volume";
    constexpr const char* Restore_Reverb_Level_Help = "";
    constexpr const char* Restore_Reverb_Level_Tooltip = "Increases the reverb volume.\n"
                                                        "\n"
                                                        "In the HD Collection and Master Collection, reverb is effectively silent, causing indoor areas to sound much drier than intended.";

    constexpr const char* Restore_Reverb_Level_Scale_Section = "Bugfixes";
    constexpr const char* Restore_Reverb_Level_Scale_Setting = "Reverb Volume Multiplier";
    constexpr const char* Restore_Reverb_Level_Scale_Help = "(1.40 Recommended)";
    constexpr const char* Restore_Reverb_Level_Scale_Tooltip = "Increases the reverb volume by this multiplier.\n"
                                                              "\n"
                                                              "Vanilla / Disabled = 1.0";

    constexpr const char* MGS3_Restore_Film_Grain_Section = "Bugfixes";
    constexpr const char* MGS3_Restore_Film_Grain_Setting = "Fix Film Grain";
    constexpr const char* MGS3_Restore_Film_Grain_Help = "(May Impact Performance)";
    constexpr const char* MGS3_Restore_Film_Grain_Tooltip = "Restores the film grain effect used during dark cutscenes, which was broken by the HD Collection.";

    constexpr const char* MGS2_RestoreActionLevelSelection_Section = "Various";
    constexpr const char* MGS2_RestoreActionLevelSelection_Setting = "Restore Main Menu Voiceovers";
    constexpr const char* MGS2_RestoreActionLevelSelection_Help = "";
    constexpr const char* MGS2_RestoreActionLevelSelection_Tooltip = "Restores the original first-time action level questionnaire before difficulty selection.";

    constexpr const char* MGS2_Increase_Shadow_Resolution_Section = "Model Quality && Level of Detail Enhancements";
    constexpr const char* MGS2_Increase_Shadow_Resolution_Setting = "Increase Shadow Resolution";
    constexpr const char* MGS2_Increase_Shadow_Resolution_Help = "";
    constexpr const char* MGS2_Increase_Shadow_Resolution_Tooltip = "The game's realtime shadows are hardcoded to 256x256 resolution.\n"
        "\n"
        "This option makes shadow resolution scale dynamically with the game's internal resolution.";

    constexpr const char* MGS2_LaserOriginFix_FixM9FPV_Section = "Bugfixes";
    constexpr const char* MGS2_LaserOriginFix_FixM9FPV_Setting = "Fix M92 Laser Origin in FPV";
    constexpr const char* MGS2_LaserOriginFix_FixM9FPV_Help = "";
    constexpr const char* MGS2_LaserOriginFix_FixM9FPV_Tooltip = "Fixes the M92's laser sight origin point when aiming in first-person view, which is aligned with the gun's barrel instead of the attached laser aiming module.";

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

    constexpr const char* MGS2_Restore_VFX_Section = "Bugfixes";
    constexpr const char* MGS2_Restore_VFX_Setting = "Fix Broken PS2 Visual Effects";
    constexpr const char* MGS2_Restore_VFX_Help = "";
    constexpr const char* MGS2_Restore_VFX_Tooltip = "Restores numerous broken visual effects that were broken by the HD Collection / Master Collection.\n"
                                                     "\n"
                                                     "These effects range from vector effect scaling (ie lasers, rain, UI line elements), water distortion, stealth camoflauge refraction, water droplets on the camera, blood stains on enemy clothing, underwater distortion, and many more.";

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
    constexpr const char* BusyLoopFix_Setting = "Fix  High  CPU  Usage";
    constexpr const char* BusyLoopFix_Help = "(Also fixes TWD usage on handheld.)";
    constexpr const char* BusyLoopFix_Tooltip = "Fixes High CPU & TWD usage.\n"
                                             "\n"
                                             "Patch 1.4 moved window message handling to a separate thread, but it continuously polls instead of sleeping when idle.\n"
                                             "\n"
                                             "This effectively doubles CPU usage, and on Steam Deck, also doubles total power draw.\n"
                                             "\n"
                                             "Fixing this restores normal behavior and improves GPU utilization.";

    constexpr const char* BusyLoopFix_Option_Disabled = "Disabled";
    constexpr const char* BusyLoopFix_Option_Half = "Half";
    constexpr const char* BusyLoopFix_Option_Full = "Full";

    // Gameplay
    constexpr const char* KeepAimingAfterFiring_Always_Section = "Keep Aiming After Firing";
    constexpr const char* KeepAimingAfterFiring_Always_Setting = "Always Keep Aiming";
    constexpr const char* KeepAimingAfterFiring_Always_Help = "(MGS2 support is limited to rifles.)\n"
                                                              "    (All guns supported in MGS3.)";
    constexpr const char* KeepAimingAfterFiring_Always_Tooltip = "Keeps aiming after firing, always.\n"
                                                                 "\n"
                                                                 "Matches the older KeepAiming mod behavior. Can be awkward to play with.\n"
                                                                 "Suggestion: leave this off and use the First-Person and Lock-On options instead.\n";

    constexpr const char* KeepAimingAfterFiring_InFirstPerson_Section = "Keep Aiming After Firing";
    constexpr const char* KeepAimingAfterFiring_InFirstPerson_Setting = "While in First Person";
    constexpr const char* KeepAimingAfterFiring_InFirstPerson_Help = "(Excludes FPS mode)";
    constexpr const char* KeepAimingAfterFiring_InFirstPerson_Tooltip = "Keeps aiming after firing while in first-person view.\n"
                                                                        "\n"
                                                                        "This does not force aiming when the joystick is at full tilt, matching PS2 behavior.\n"
                                                                        "\n"
                                                                        "For full tilt, enable \"Fix Aiming On Full Tilt\" under the bugfixes panel.";

    constexpr const char* KeepAimingAfterFiring_InFPSMode_Section = "Keep Aiming After Firing";
    constexpr const char* KeepAimingAfterFiring_InFPSMode_Setting = "While in FPS Mode";
    constexpr const char* KeepAimingAfterFiring_InFPSMode_Help = "";
    constexpr const char* KeepAimingAfterFiring_InFPSMode_Tooltip = "Keeps aiming after firing while in FPS mode."
                                                                    "\n"
                                                                    "This does not force aiming when the joystick is at full tilt, matching PS2 behavior.\n"
                                                                    "\n"
                                                                    "For full tilt, enable \"Fix Aiming On Full Tilt\" under the bugfixes panel.";

    constexpr const char* KeepAimingAfterFiring_OnLockOn_Section = "Keep Aiming After Firing";
    constexpr const char* KeepAimingAfterFiring_OnLockOn_Setting = "While Holding Lock On";
    constexpr const char* KeepAimingAfterFiring_OnLockOn_Help = "";
    constexpr const char* KeepAimingAfterFiring_OnLockOn_Tooltip = "Keeps aiming after firing while holding Lock-On (L1).";

    constexpr const char* MGS2_FixDamageType_Section = "Various";
    constexpr const char* MGS2_FixDamageType_Setting = "Fix Vamp Punch Damage Type";
    constexpr const char* MGS2_FixDamageType_Help = "";
    constexpr const char* MGS2_FixDamageType_Tooltip = "Punching Vamp while in first-person-view erroneously does lethal damage.\n"
                                                       "Fixes to deal non-lethal damage like normal hand-to-hand attacks.";



    constexpr const char* MGS2_ThirdPersonFreecam_Enabled_Section = "Third Person Freecam";
    constexpr const char* MGS2_ThirdPersonFreecam_Enabled_Setting = "Enable Third Person Freecam";
    constexpr const char* MGS2_ThirdPersonFreecam_Enabled_Help = "(EXPERIMENTAL - SEE TOOLTIP)\n"
                                                                 "(MOUSE SUPPORT STILL W.I.P.)\n"
                                                                 "(!!! MAY CAUSE CRASHING !!!)\n";
    constexpr const char* MGS2_ThirdPersonFreecam_Enabled_Tooltip = "Enables the third person freecam.\n"
                                                                    "\n"
                                                                    "This was a cut-feature originally developed by Bluepoint for the 2011 HD Collection.\n"
                                                                    "\n"
                                                                    "This feature was cut around 7/7/2011 per a leftover note found in the source code.\n"
                                                                    "\n"
                                                                    "Functionality is a bit glitchy, as this feature was not fully completed.\n"
                                                                    "Things are still being patched up by hand with love. <3\n"
                                                                    "\n"
                                                                    "If the camera gets stuck at a weird angle; tap First Person View, or toggle the third person camera a few times to reset it.";

    constexpr const char* MGS2_ThirdPersonFreecam_ToggleKey_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_ToggleKey_Setting = "Third Person View Toggle";
    constexpr const char* MGS2_ThirdPersonFreecam_ToggleKey_Help = "(Controller inputs accepted)\n(Utilizes your Steam Input binds)";
    constexpr const char* MGS2_ThirdPersonFreecam_ToggleKey_Tooltip = "Toggles the third person view on/off.\n"
                                                                      "\n"
                                                                      "R3 / Right Analog Stick Click suggested.";

    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Setting = "Inherit Camera Rotation";
    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Tooltip = "Functionality unknown - seems related to lockers & elevators.";

    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_ToggleKey_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_ToggleKey_Setting = "Inherit Camera Rotation Toggle";
    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_ToggleKey_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_ToggleKey_Tooltip = "";

    constexpr const char* MGS2_ThirdPersonFreecam_Max_Camera_Distance_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Max_Camera_Distance_Setting = "Max Camera Distance";
    constexpr const char* MGS2_ThirdPersonFreecam_Max_Camera_Distance_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Max_Camera_Distance_Tooltip = "How far the camera should zoom out from the player.";

    constexpr const char* MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Setting = "Horizontal Camera Sensitivity";
    constexpr const char* MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Tooltip = "";

    constexpr const char* MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Setting = "Vertical Camera Sensitivity";
    constexpr const char* MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Tooltip = "";

    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Decrease_Hotkey_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Decrease_Hotkey_Setting = "Camera - Zoom In Hotkey";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Decrease_Hotkey_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Decrease_Hotkey_Tooltip = "Zooms the camera towards the player.\n"
                                                                                            "\n"
                                                                                            "D-Pad Up suggested.";

    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Increase_Hotkey_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Increase_Hotkey_Setting = "Camera - Zoom Out Hotkey";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Increase_Hotkey_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Increase_Hotkey_Tooltip = "Zooms the camera away from the player.\n"
                                                                                            "\n"
                                                                                            "D-Pad Down suggested.";

    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Reset_Hotkey_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Reset_Hotkey_Setting = "Camera - Zoom Reset Hotkey";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Reset_Hotkey_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Reset_Hotkey_Tooltip = "Resets the camera zoom to the default distance.\n"
                                                                                            "\n"
                                                                                            "D-Pad Left suggested.";

    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Setting = "Camera - Zoom Step Amount";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Help = "";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Tooltip = "How much the camera should move when zooming.";

    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Change_Speed_Section = MGS2_ThirdPersonFreecam_Enabled_Section;
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Change_Speed_Setting = "Camera - Zoom Speed";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Change_Speed_Help = "(In microseconds)";
    constexpr const char* MGS2_ThirdPersonFreecam_Camera_Distance_Change_Speed_Tooltip = "How quickly the camera should zoom.";



    constexpr const char* MGS2_First_Person_View_Enabled_Section = "First Person Shooter Mode";
    constexpr const char* MGS2_First_Person_View_Enabled_Setting = "Enable First Person Shooter Mode";
    constexpr const char* MGS2_First_Person_View_Enabled_Help = "(EXPERIMENTAL - SEE TOOLTIP)\n"
                                                                "(SOME EFFECTS WILL BE MISSING)\n"
                                                                "(!!! MAY CAUSE CRASHING !!!)";
    constexpr const char* MGS2_First_Person_View_Enabled_Tooltip = "Enables the First Person Shooter mode.\n"
                                                                    "\n"
                                                                    "This was a cut-feature originally developed by Bluepoint for the 2011 HD Collection.\n"
                                                                    "\n"
                                                                    "This feature was cut around 7/7/2011 per a leftover note found in the source code.\n"
                                                                    "\n"
                                                                    "Functionality is a bit glitchy, as this feature was not fully completed.\n"
                                                                    "Some effects and enemy logic -will- be different/missing compared to regular first person view.\n"
                                                                    "\n"
                                                                    "Things are still being patched up by hand with love. <3";

    constexpr const char* MGS2_First_Person_View_Movement_Enabled_By_Default_Section = MGS2_First_Person_View_Enabled_Section;
    constexpr const char* MGS2_First_Person_View_Movement_Enabled_By_Default_Setting = "First Person Shooter - Movement Enabled By Default";
    constexpr const char* MGS2_First_Person_View_Movement_Enabled_By_Default_Help = "";
    constexpr const char* MGS2_First_Person_View_Movement_Enabled_By_Default_Tooltip = "";

    constexpr const char* MGS2_First_Person_View_Movement_ToggleKey_Section = MGS2_First_Person_View_Enabled_Section;
    constexpr const char* MGS2_First_Person_View_Movement_ToggleKey_Setting = "Toggle First Person Shooter Movement";
    constexpr const char* MGS2_First_Person_View_Movement_ToggleKey_Help = "";
    constexpr const char* MGS2_First_Person_View_Movement_ToggleKey_Tooltip = "";

    constexpr const char* MGS2_First_Person_View_ToggleKey_Section = MGS2_First_Person_View_Enabled_Section;
    constexpr const char* MGS2_First_Person_View_ToggleKey_Setting = "Toggle First Person Shooter Mode";
    constexpr const char* MGS2_First_Person_View_ToggleKey_Help = "";
    constexpr const char* MGS2_First_Person_View_ToggleKey_Tooltip = "";

    constexpr const char* MGS2_First_Person_View_Hold_Button_Section = MGS2_First_Person_View_Enabled_Section;
    constexpr const char* MGS2_First_Person_View_Hold_Button_Setting = "Tap to Keep First Person View Active";
    constexpr const char* MGS2_First_Person_View_Hold_Button_Help = "";
    constexpr const char* MGS2_First_Person_View_Hold_Button_Tooltip = "";



    constexpr const char* MGS2_First_Person_View_Hold_ToggleKey_Section = MGS2_First_Person_View_Enabled_Section;
    constexpr const char* MGS2_First_Person_View_Hold_ToggleKey_Setting = "Toggle Tap for First Person View";
    constexpr const char* MGS2_First_Person_View_Hold_ToggleKey_Help = "";
    constexpr const char* MGS2_First_Person_View_Hold_ToggleKey_Tooltip = "";


    constexpr const char* MGS2_RestoreOriginalDifficulty_Solidus_Choking_Section = "Difficulty Restoration";
    constexpr const char* MGS2_RestoreOriginalDifficulty_Solidus_Choking_Setting = "Restore PS2 Solidus Choking Difficulty";
    constexpr const char* MGS2_RestoreOriginalDifficulty_Solidus_Choking_Help = "";
    constexpr const char* MGS2_RestoreOriginalDifficulty_Solidus_Choking_Tooltip = "Restores the original harder PS2 durations and life reductions for Solidus's choking sequence, which were rebalanced/made easier with the HD Collection.\n"
                                                                                         "\n"
                                                                                         "HDC Duration:\n"
                                                                                         "Very Easy: 600, Easy: 650, Normal: 700, Hard: 750, Extreme: 800, European Extreme: 850\n"
                                                                                         "PS2 Duration:\n"
                                                                                         "Very Easy: 600, Easy: 635, Normal: 900, Hard: 1200, Extreme: 1500, European Extreme: 3000\n"
                                                                                         "\n"
                                                                                         "HDC Life Amount\n"
                                                                                         "Very Easy: 200, Easy: 184, Normal: 168, Hard: 152, Extreme: 136, European Extreme: 120\n"
                                                                                         "PS2 Life Amount\n"
                                                                                         "Very Easy: 200, Easy: 120, Normal: 100, Hard: 75, Extreme: 50, European Extreme: 30\n";
    constexpr const char* MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_Disabled = "Disabled";
    constexpr const char* MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_LifeReductionOnly = "Life Reduction Only";
    constexpr const char* MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_DurationIncreaseOnly = "Duration Increase Only";
    constexpr const char* MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_Both = "Full Restoration";


    constexpr const char* MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Section = "Difficulty Restoration";
    constexpr const char* MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Setting = "Enable Grenade Cooking";
    constexpr const char* MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Help = "";
    constexpr const char* MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Tooltip = "Restores the original PS2 grenade cooking behavior, where time spent holding an armed grenade counts toward its detonation timer.";
    

    constexpr const char* MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Toggle_Section = "Difficulty Restoration";
    constexpr const char* MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Toggle_Setting = "Toggle Grenade Cooking";
    constexpr const char* MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Toggle_Help = "";
    constexpr const char* MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Toggle_Tooltip = "Toggles grenade cooking on/off.";



    // Tweaks
    constexpr const char* SkipIntroLogos_Section = "Launcher and Splashscreens";
    constexpr const char* SkipIntroLogos_Setting = "Skip In-Game Splashscreens";
    constexpr const char* SkipIntroLogos_Help = "";
    constexpr const char* SkipIntroLogos_Tooltip = "Skips the unskippable \"KONAMI\" splashscreens on startup. Skippable intro videos will still be played.";

    constexpr const char* LauncherJumpStart_Section = SkipIntroLogos_Section;
    constexpr const char* LauncherJumpStart_Setting = "Skip Launcher Splashscreens";
    constexpr const char* LauncherJumpStart_Help = "";
    constexpr const char* LauncherJumpStart_Tooltip = "Skips launcher splash screens and menus and jumps directly to the launch game screen.";

    constexpr const char* SkipLauncher_Section = SkipIntroLogos_Section;
    constexpr const char* SkipLauncher_Setting = "Skip Launcher";
    constexpr const char* SkipLauncher_Help = "";
    constexpr const char* SkipLauncher_Tooltip = "Skips the launcher app and runs the game directly.";

    constexpr const char* CtrlType_Section = "Controller Settings";
    constexpr const char* CtrlType_Setting = "Button Icons";
    constexpr const char* CtrlType_Help = "";
    constexpr const char* CtrlType_Tooltip = "Selects which controller button icons to display in-game.";

    constexpr const char* MenuButton_Section = "Controller Settings";
    constexpr const char* MenuButton_Setting = "Set Menu OK && Cancel Button";
    constexpr const char* MenuButton_Help = "";
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

    constexpr const char* SkipLauncherMSXGame_Section = SkipIntroLogos_Section;
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
    constexpr const char* MGS2Sunglasses_Setting = "Force Sunglasses";
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
    constexpr const char* RestoreDogtagNames_Setting = "Restore Original Dogtag Names";
    constexpr const char* RestoreDogtagNames_Help = "";
    constexpr const char* RestoreDogtagNames_Tooltip = "Restores the names of dogtags that were edited for copyright/legal reasons (ie Gackt -> Gekko.)";

    constexpr const char* MGS2_RestoreNodeDOBInfo_Section = "Various";
    constexpr const char* MGS2_RestoreNodeDOBInfo_Setting = "Restore Node DoB && Bloodtype Entry";
    constexpr const char* MGS2_RestoreNodeDOBInfo_Help = "";
    constexpr const char* MGS2_RestoreNodeDOBInfo_Tooltip = "Restores DoB / Bloodtype information entry at the start of the Plant chapter.";

    constexpr const char* RestoreSoLRadarRotation_Section = "Various";
    constexpr const char* RestoreSoLRadarRotation_Setting = "Restore SoL Radar Rotation";
    constexpr const char* RestoreSoLRadarRotation_Help = "";
    constexpr const char* RestoreSoLRadarRotation_Tooltip = "The radar in Sons of Liberty had a flag East -> West rotation during plant.\n"
                                                            "\n"
                                                            "Substance modified the rotation to align closer to the actual orientation of the struts.";

    constexpr const char* MGS2_RestoreElevatorGlitch_Section = "Speedrunner Settings";
    constexpr const char* MGS2_RestoreElevatorGlitch_Setting = "Restore SoL Elevator Glitch";
    constexpr const char* MGS2_RestoreElevatorGlitch_Help = "";
    constexpr const char* MGS2_RestoreElevatorGlitch_Tooltip = "Speedrun opt-in. Re-enables the Sons of Liberty floor-clip: going prone at an elevator call button and operating it forces a stand-up with the player's origin left on the floor, clipping through it.\n"
                                                               "\n"
                                                               "Substance gated elevator operation to standing/squat to patch this out. Off by default.";

    constexpr const char* ShowSpeedrunnerOverlay_Section = "Speedrunner Settings";
    constexpr const char* ShowSpeedrunnerOverlay_Setting = "Gameplay Stats Overlay";
    constexpr const char* ShowSpeedrunnerOverlay_Help = "";
    constexpr const char* ShowSpeedrunnerOverlay_Tooltip = "Displays an overlay with current stage time / in-game timer (IGT) / elasped time.\n"
                                                           "\n"
                                                           "Extended stats are also shown while the game is paused.";
    constexpr const char* ShowSpeedrunnerOverlay_Option_Disabled = "Disabled";
    constexpr const char* ShowSpeedrunnerOverlay_Option_TopLeft = "Top Left";
    constexpr const char* ShowSpeedrunnerOverlay_Option_TopRight = "Top Right";
    constexpr const char* ShowSpeedrunnerOverlay_Option_BottomLeft = "Bottom Left";
    constexpr const char* ShowSpeedrunnerOverlay_Option_BottomRight = "Bottom Right";


    constexpr const char* FixIGTLoadingPause_Section = "Speedrunner Settings";
    constexpr const char* FixIGTLoadingPause_Setting = "Fix In-Game Timer Loading Pause";
    constexpr const char* FixIGTLoadingPause_Help = "";
    constexpr const char* FixIGTLoadingPause_Tooltip = "Fixes the In-Game Timer (IGT) continuing to run during loading screens / level transitions.\n"
        "\n"
        "In earlier versions of the game, the IGT was paused during loading screens. When Bluepoint refactored the loading system for the HD Collection, "
        "the unique flag responsible for pausing the timer was no longer applied when loading began, even though it was still cleared when loading ended.\n"
        "\n"
        "This restores the original pre-HD Collection IGT behavior.\n"
        "\n"
        "Fixing this has reportedly reduced final IGT by up to 5 minutes compared to the bugged HDC/MC IGT behavior.";

    constexpr const char* MGS2_PhoneJingle_Section = "Various";
    constexpr const char* MGS2_PhoneJingle_Setting = "Restore Japanese Phone Ringtone";
    constexpr const char* MGS2_PhoneJingle_Help = "";
    constexpr const char* MGS2_PhoneJingle_Tooltip = "Restores the Japanese Sons of Liberty exclusive MGS theme-song phone ringtone that was missing from all future versions of the game.\n"
                                                     "\n"
                                                     "This will enable the ringtone for all versions of the game.";

    constexpr const char* UnusedRetroColonel_Section = "MGS2 Community Bugfix Compilation Integration";
    constexpr const char* UnusedRetroColonel_Setting = "Retro MSX Colonel Sprite";
    constexpr const char* UnusedRetroColonel_Help = "";
    constexpr const char* UnusedRetroColonel_Tooltip = "The Colonel's glitching sometimes matches his sprites from MGS1 and from Ghost Babel. "
                                                       "Restores a removed use of his sprite from Metal Gear 2: Solid Snake.\n"
                                                       "\n"
                                                       "Disabled = The vanilla behavior, MGS1 and Ghost Babel sprites only.\n"
                                                       "MSX2 = MGS1, Ghost Babel, and the original MSX2 sprite (resembling Richard Crenna as the colonel from Rambo).\n"
                                                       "Subsistence = MGS1, Ghost Babel, and the redrawn MG2 sprite used from the Subsistence re-release onward.";
    constexpr const char* UnusedRetroColonel_Option_Normal = "Disabled";
    constexpr const char* UnusedRetroColonel_Option_MSX = "MSX2";
    constexpr const char* UnusedRetroColonel_Option_Subsistence = "Subsistence";

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
    constexpr const char* CaptureInputsWhileAltTabbedHotkey_Setting = "Capture Hotkeys While Alt Tabbed";
    constexpr const char* CaptureInputsWhileAltTabbedHotkey_Help = "";
    constexpr const char* CaptureInputsWhileAltTabbedHotkey_Tooltip = "Capture hotkeys even when the window is not focused or is alt-tabbed.";

    constexpr const char* CycleWireframeMode_Section = "Hotkeys";
    constexpr const char* CycleWireframeMode_Setting = "Cycle Wireframe Mode";
    constexpr const char* CycleWireframeMode_Help = "";
    constexpr const char* CycleWireframeMode_Tooltip = "Cycle between wireframe rendering modes (available when Rain Width Fix is enabled).";


    // Achievements

    constexpr const char* DisableSteamAchievements_Section = "DISABLE STEAM ACHIEVEMENTS";
    constexpr const char* DisableSteamAchievements_Setting = "Disable Unlocking Steam Achievements";
    constexpr const char* DisableSteamAchievements_Help = "";
    constexpr const char* DisableSteamAchievements_Tooltip = "Disables Steam achievements for the game.";

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

    constexpr const char* VerboseLogging_Section = "Debugging";
    constexpr const char* VerboseLogging_Setting = "Debug Logging";
    constexpr const char* VerboseLogging_Help = "";
    constexpr const char* VerboseLogging_Tooltip = "Enables verbose logging for debugging purposes.";

    constexpr const char* Debugging_Start_In_Dev_Menu_Section = "Debugging";
    constexpr const char* Debugging_Start_In_Dev_Menu_Setting = "Start Game in Developer Menu";
    constexpr const char* Debugging_Start_In_Dev_Menu_Help = "";
    constexpr const char* Debugging_Start_In_Dev_Menu_Tooltip = "Starts the game in the developer menu for debugging.";

    constexpr const char* Restore_Title_Screen_Swapping_Section = "MGS2 Community Bugfix Compilation Integration";
    constexpr const char* Restore_Title_Screen_Swapping_Setting = "Restore Title Screen 2 Color Swapping";
    constexpr const char* Restore_Title_Screen_Swapping_Help = "";
    constexpr const char* Restore_Title_Screen_Swapping_Tooltip = "Makes the title screen's 2 change color upon each game completion, which was removed in the HD Collection.\n"
                                                                  "\n"
                                                                  "Afevis's Alternative Titlecards and loading screens mod is also supported.";

    
        



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
