// ============================================================================
// Project:   Universal Config Tool
// File:      tab_data.cpp
//
// Copyright (c) 2025 Afevis
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// ============================================================================
// ReSharper disable CppClangTidyClangDiagnosticMissingFieldInitializers
#include "pch.h"
#include "tab_data.hpp"
#include <d3d11.h>

#include "config_keys.hpp"

const std::vector<std::pair<wxString, std::vector<Field>>> kTabs = {
    { wxString("General"), {

        { (MGS2|MGS3), ConfigKeys::FixAimingAfterEquip_Section, ConfigKeys::FixAimingAfterEquip_Setting, ConfigKeys::FixAimingAfterEquip_Help, ConfigKeys::FixAimingAfterEquip_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MGS2|MGS3), ConfigKeys::FixAimingFullTilt_Section, ConfigKeys::FixAimingFullTilt_Setting, ConfigKeys::FixAimingFullTilt_Help, ConfigKeys::FixAimingFullTilt_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MGS2|MGS3), ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Section, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Setting, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Help, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MG|MGS2|MGS3), ConfigKeys::DisableMouseCursor_Section, ConfigKeys::DisableMouseCursor_Setting, ConfigKeys::DisableMouseCursor_Help, ConfigKeys::DisableMouseCursor_Tooltip,
          std::nullopt, false, Field::Bool, true },


        { (MGS2), ConfigKeys::MGS2_LaserOriginFix_FixM9FPV_Section, ConfigKeys::MGS2_LaserOriginFix_FixM9FPV_Setting, ConfigKeys::MGS2_LaserOriginFix_FixM9FPV_Help, ConfigKeys::MGS2_LaserOriginFix_FixM9FPV_Tooltip,
          std::nullopt, false, Field::Bool, true },


        { (MGS3), ConfigKeys::BusyLoopFix_Section, "",
          "", "",
          std::nullopt, false, Field::Spacer },


        { (MG|MGS2|MGS3), ConfigKeys::BusyLoopFix_Section, ConfigKeys::BusyLoopFix_Setting, ConfigKeys::BusyLoopFix_Help, ConfigKeys::BusyLoopFix_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, ConfigKeys::BusyLoopFix_Option_Full, {ConfigKeys::BusyLoopFix_Option_Full, ConfigKeys::BusyLoopFix_Option_Half, ConfigKeys::BusyLoopFix_Option_Disabled} },

        { (MG|MGS2|MGS3), ConfigKeys::ForceStereoAudio_Section, ConfigKeys::ForceStereoAudio_Setting, ConfigKeys::ForceStereoAudio_Help, ConfigKeys::ForceStereoAudio_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, "", {ConfigKeys::ForceStereoAudio_Option_Stereo, ConfigKeys::ForceStereoAudio_Option_Surround} },

        { (MGS2|MGS3), ConfigKeys::CPUCoreLimit_Section, ConfigKeys::CPUCoreLimit_Setting, ConfigKeys::CPUCoreLimit_Help, ConfigKeys::CPUCoreLimit_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { (MG), ConfigKeys::ForceStereoAudio_Section, "",
          "", "",
          std::nullopt, false, Field::Spacer },


        { (MG | MGS2 | MGS3), ConfigKeys::ForceDedicatedGPU_Section, ConfigKeys::ForceDedicatedGPU_Setting, ConfigKeys::ForceDedicatedGPU_Help, ConfigKeys::ForceDedicatedGPU_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MG|MGS2|MGS3), ConfigKeys::DisableFullscreenOptimization_Section, ConfigKeys::DisableFullscreenOptimization_Setting, ConfigKeys::DisableFullscreenOptimization_Help, ConfigKeys::DisableFullscreenOptimization_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { (MG|MGS2|MGS3), ConfigKeys::RenameOrRemoveCorruptSaveData_Section, ConfigKeys::RenameOrRemoveCorruptSaveData_Setting, ConfigKeys::RenameOrRemoveCorruptSaveData_Help, ConfigKeys::RenameOrRemoveCorruptSaveData_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, "", {ConfigKeys::RenameOrRemoveCorruptSaveData_Option_Move, ConfigKeys::RenameOrRemoveCorruptSaveData_Option_Delete, ConfigKeys::RenameOrRemoveCorruptSaveData_Option_Disable} },

        { (MG|MGS2|MGS3), ConfigKeys::CorruptSaveData_Notification_Section, ConfigKeys::CorruptSaveData_Notification_Setting, ConfigKeys::CorruptSaveData_Notification_Help, ConfigKeys::CorruptSaveData_Notification_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MG|MGS2|MGS3), ConfigKeys::Region_Section, ConfigKeys::Region_Setting, ConfigKeys::Region_Help, ConfigKeys::Region_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, "", {} },

        { (MG|MGS2|MGS3), ConfigKeys::Language_Section, ConfigKeys::Language_Setting, ConfigKeys::Language_Help, ConfigKeys::Language_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, "", {} },

        { (MG|MGS2|MGS3), ConfigKeys::CtrlType_Section, ConfigKeys::CtrlType_Setting, ConfigKeys::CtrlType_Help, ConfigKeys::CtrlType_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, *std::next(kLauncherConfigCtrlTypes.begin(), 5),
          { std::begin(kLauncherConfigCtrlTypes), std::end(kLauncherConfigCtrlTypes) } },

        { (MGS2), ConfigKeys::MenuButton_Section, ConfigKeys::MenuButton_Setting, ConfigKeys::MenuButton_Help, ConfigKeys::MenuButton_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, "", {ConfigKeys::MenuButton_Option_Default, ConfigKeys::MenuButton_Option_EastForOK, ConfigKeys::MenuButton_Option_SouthForOK} },


        { (MG|MGS2|MGS3), ConfigKeys::CtrlType_Section, "",
          "", "",
          std::nullopt, false, Field::Spacer }
    }},
    { wxString("Graphics"), {
        


                { (MG|MGS2|MGS3), ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting, ConfigKeys::ForceWindowSize_Help, ConfigKeys::ForceWindowSize_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MG|MGS2|MGS3), ConfigKeys::WindowWidth_Section, ConfigKeys::WindowWidth_Setting, ConfigKeys::WindowWidth_Help, ConfigKeys::WindowWidth_Tooltip,
          std::make_pair(ConfigKeys::WindowedMode_Section, ConfigKeys::WindowedMode_Setting),
          false,
          Field::Int, 0, 0, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION,
          "", {}, 0.0, 0.0, 0.0,
          { ConfigKeys::BorderlessMode_Option_BorderlessWindowed, ConfigKeys::BorderlessMode_Option_Windowed } },


        { (MG|MGS2|MGS3), ConfigKeys::WindowedMode_Section, ConfigKeys::WindowedMode_Setting, ConfigKeys::WindowedMode_Help, ConfigKeys::WindowedMode_Tooltip,
          std::make_pair(ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting), false, Field::Choice, 0, 0, 0, ConfigKeys::BorderlessMode_Option_BorderlessFullscreen, {ConfigKeys::BorderlessMode_Option_Fullscreen, ConfigKeys::BorderlessMode_Option_BorderlessFullscreen, ConfigKeys::BorderlessMode_Option_BorderlessWindowed, ConfigKeys::BorderlessMode_Option_Windowed, } },


        { (MG|MGS2|MGS3), ConfigKeys::WindowHeight_Section, ConfigKeys::WindowHeight_Setting, ConfigKeys::WindowHeight_Help, ConfigKeys::WindowHeight_Tooltip,
          std::make_pair(ConfigKeys::WindowedMode_Section, ConfigKeys::WindowedMode_Setting),
          false,
          Field::Int, 0, 0, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION,
          "", {}, 0.0, 0.0, 0.0,
          { ConfigKeys::BorderlessMode_Option_BorderlessWindowed, ConfigKeys::BorderlessMode_Option_Windowed } },

        { (MG|MGS2|MGS3), ConfigKeys::RenderScaleWidth_Section, ConfigKeys::RenderScaleWidth_Setting, ConfigKeys::RenderScaleWidth_Help, ConfigKeys::RenderScaleWidth_Tooltip,
          std::make_pair(ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting), false, Field::Int, 0, 0, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION },

        { (MG|MGS2|MGS3), ConfigKeys::RenderScaleHeight_Section, ConfigKeys::RenderScaleHeight_Setting, ConfigKeys::RenderScaleHeight_Help, ConfigKeys::RenderScaleHeight_Tooltip,
          std::make_pair(ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting), false, Field::Int, 0, 0, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION },

        { (MG|MGS2|MGS3), ConfigKeys::ColorCorrection_Enabled_Section, ConfigKeys::ColorCorrection_Enabled_Setting, ConfigKeys::ColorCorrection_Enabled_Help, ConfigKeys::ColorCorrection_Enabled_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MGS2 | MGS3), ConfigKeys::AnisotropicFiltering_Section, ConfigKeys::AnisotropicFiltering_Setting, ConfigKeys::AnisotropicFiltering_Help, ConfigKeys::AnisotropicFiltering_Tooltip,
          std::make_pair(ConfigKeys::DisableTextureFiltering_Section, ConfigKeys::DisableTextureFiltering_Setting), true, Field::Int, 16, 0, D3D11_DEFAULT_MAX_ANISOTROPY},


        { (MG), ConfigKeys::MG1_Crop_Overscan_Enabled_Section, ConfigKeys::MG1_Crop_Overscan_Enabled_Setting, ConfigKeys::MG1_Crop_Overscan_Enabled_Help, ConfigKeys::MG1_Crop_Overscan_Enabled_Tooltip,
          std::nullopt, false, Field::Bool, true },

        /*
                  { (MG), ConfigKeys::MG1_Correct_Aspect_Ratio_Enabled_Section, "",
          "", "",
          std::nullopt, false, Field::Spacer },
          */
        { (MG), ConfigKeys::MG1_Correct_Aspect_Ratio_Enabled_Section, ConfigKeys::MG1_Correct_Aspect_Ratio_Enabled_Setting, ConfigKeys::MG1_Correct_Aspect_Ratio_Enabled_Help, ConfigKeys::MG1_Correct_Aspect_Ratio_Enabled_Tooltip,
          std::nullopt, false, Field::Bool, true },



        { (MGS2 | MGS3), ConfigKeys::EnableSMAA_Section, ConfigKeys::EnableSMAA_Setting, ConfigKeys::EnableSMAA_Help, ConfigKeys::EnableSMAA_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MGS2|MGS3), ConfigKeys::DisableTextureFiltering_Section, ConfigKeys::DisableTextureFiltering_Setting, ConfigKeys::DisableTextureFiltering_Help, ConfigKeys::DisableTextureFiltering_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { (MGS2), ConfigKeys::LOD_MGS2_NPC_Section, ConfigKeys::LOD_MGS2_NPC_Setting, ConfigKeys::LOD_MGS2_NPC_Help, ConfigKeys::LOD_MGS2_NPC_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MGS2), ConfigKeys::LOD_MGS2_ShellCasings_Section, ConfigKeys::LOD_MGS2_ShellCasings_Setting, ConfigKeys::LOD_MGS2_ShellCasings_Help, ConfigKeys::LOD_MGS2_ShellCasings_Tooltip,
          std::nullopt, false, Field::Bool, true },


        { (MGS2), ConfigKeys::MGS2_Increase_Shadow_Resolution_Section, ConfigKeys::MGS2_Increase_Shadow_Resolution_Setting, ConfigKeys::MGS2_Increase_Shadow_Resolution_Help, ConfigKeys::MGS2_Increase_Shadow_Resolution_Tooltip,
          std::nullopt, false, Field::Bool, true },



        { (MGS3), ConfigKeys::DistanceCullingGrassAlways_Section, ConfigKeys::DistanceCullingGrassAlways_Setting, ConfigKeys::DistanceCullingGrassAlways_Help, ConfigKeys::DistanceCullingGrassAlways_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MGS3), ConfigKeys::DistanceCullingGrassScalar_Section, ConfigKeys::DistanceCullingGrassScalar_Setting, ConfigKeys::DistanceCullingGrassScalar_Help, ConfigKeys::DistanceCullingGrassScalar_Tooltip,
          std::make_pair(ConfigKeys::DistanceCullingGrassAlways_Section, ConfigKeys::DistanceCullingGrassAlways_Setting), true, Field::Float, 0 , 0, 0, "", {}, 1.0, 0},


        { (MGS3), ConfigKeys::ToggleDistanceCullingGrass_Section, ConfigKeys::ToggleDistanceCullingGrass_Setting, ConfigKeys::ToggleDistanceCullingGrass_Help, ConfigKeys::ToggleDistanceCullingGrass_Tooltip,
          std::make_pair(ConfigKeys::DistanceCullingGrassAlways_Section, ConfigKeys::DistanceCullingGrassAlways_Setting), false, Field::Hotkey, 0, 0, 0, "Page Up" },


        { (MGS2|MGS3), ConfigKeys::FixAspectRatio_Section, ConfigKeys::FixAspectRatio_Setting, ConfigKeys::FixAspectRatio_Help, ConfigKeys::FixAspectRatio_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MG|MGS2|MGS3), ConfigKeys::FixHUD_Section, ConfigKeys::FixHUD_Setting, ConfigKeys::FixHUD_Help, ConfigKeys::FixHUD_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { (MGS2|MGS3), ConfigKeys::FixFOV_Section, ConfigKeys::FixFOV_Setting, ConfigKeys::FixFOV_Help, ConfigKeys::FixFOV_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MG|MGS2|MGS3), ConfigKeys::FramebufferFix_Section, ConfigKeys::FramebufferFix_Setting, ConfigKeys::FramebufferFix_Help, ConfigKeys::FramebufferFix_Tooltip,
          std::nullopt, false, Field::Bool, true },


    }},
    { wxString("Tweaks"), {

        { (MG|MGS2|MGS3), ConfigKeys::SkipLauncher_Section, ConfigKeys::SkipLauncher_Setting, ConfigKeys::SkipLauncher_Help, ConfigKeys::SkipLauncher_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { (MG), ConfigKeys::SkipLauncherMSXGame_Section, ConfigKeys::SkipLauncherMSXGame_Setting, ConfigKeys::SkipLauncherMSXGame_Help, ConfigKeys::SkipLauncherMSXGame_Tooltip,
          std::make_pair(ConfigKeys::SkipLauncher_Section, ConfigKeys::SkipLauncher_Setting), false, Field::Choice, 0, 0, 0, ConfigKeys::SkipLauncherMSX_Option_MG1, {ConfigKeys::SkipLauncherMSX_Option_MG1,ConfigKeys::SkipLauncherMSX_Option_MG2}},

        { (MGS2 | MGS3), ConfigKeys::SkipLauncher_Section,     "" ,  "", "",
          std::nullopt, false, Field::Spacer },


        { (MGS2 | MGS3), ConfigKeys::LauncherJumpStart_Section, ConfigKeys::LauncherJumpStart_Setting, ConfigKeys::LauncherJumpStart_Help, ConfigKeys::LauncherJumpStart_Tooltip,
          std::make_pair(ConfigKeys::SkipLauncher_Section, ConfigKeys::SkipLauncher_Setting), true, Field::Bool, false},

        { (MGS2 | MGS3), ConfigKeys::SkipIntroLogos_Section, ConfigKeys::SkipIntroLogos_Setting, ConfigKeys::SkipIntroLogos_Help, ConfigKeys::SkipIntroLogos_Tooltip,
          std::nullopt, false, Field::Bool, false },


        { (MG|MGS2|MGS3), ConfigKeys::EnablePauseOnFocusLoss_Section, ConfigKeys::EnablePauseOnFocusLoss_Setting, ConfigKeys::EnablePauseOnFocusLoss_Help, ConfigKeys::EnablePauseOnFocusLoss_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { (MGS2), ConfigKeys::MGS2Sunglasses_Section, ConfigKeys::MGS2Sunglasses_Setting, ConfigKeys::MGS2Sunglasses_Help, ConfigKeys::MGS2Sunglasses_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, ConfigKeys::MGS2Sunglasses_Option_Normal, {ConfigKeys::MGS2Sunglasses_Option_Normal,ConfigKeys::MGS2Sunglasses_Option_Always, ConfigKeys::MGS2Sunglasses_Option_Never } },


        { (MGS2), ConfigKeys::MGS2_SnakeTales_Radar_Section, ConfigKeys::MGS2_SnakeTales_Radar_Setting, ConfigKeys::MGS2_SnakeTales_Radar_Help, ConfigKeys::MGS2_SnakeTales_Radar_Tooltip,
            std::nullopt, false, Field::Bool, false},

        { (MGS2), ConfigKeys::MGS2_FixDamageType_Section, ConfigKeys::MGS2_FixDamageType_Setting, ConfigKeys::MGS2_FixDamageType_Help, ConfigKeys::MGS2_FixDamageType_Tooltip,
            std::nullopt, false, Field::Bool, false},


            {(MGS2), ConfigKeys::MGS2_Lifebar_Name_Custom_Section, "", "", "", std::nullopt, false, Field::Spacer },


        { (MGS2), ConfigKeys::MGS2_Lifebar_Name_Use_Custom_Section, ConfigKeys::MGS2_Lifebar_Name_Use_Custom_Setting, ConfigKeys::MGS2_Lifebar_Name_Use_Custom_Help, ConfigKeys::MGS2_Lifebar_Name_Use_Custom_Tooltip,
          std::make_pair(ConfigKeys::MGS2_Lifebar_Name_Use_Character_Names_Section, ConfigKeys::MGS2_Lifebar_Name_Use_Character_Names_Setting), true, Field::Bool, false },


                  { (MGS2), ConfigKeys::MGS2_Lifebar_Name_Use_Character_Names_Section, ConfigKeys::MGS2_Lifebar_Name_Use_Character_Names_Setting, ConfigKeys::MGS2_Lifebar_Name_Use_Character_Names_Help, ConfigKeys::MGS2_Lifebar_Name_Use_Character_Names_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { (MGS2), ConfigKeys::MGS2_Lifebar_Name_Custom_Section, ConfigKeys::MGS2_Lifebar_Name_Custom_Setting, ConfigKeys::MGS2_Lifebar_Name_Custom_Help, ConfigKeys::MGS2_Lifebar_Name_Custom_Tooltip,
            std::make_pair(ConfigKeys::MGS2_Lifebar_Name_Use_Custom_Section, ConfigKeys::MGS2_Lifebar_Name_Use_Custom_Setting), false, Field::Str, 0, 0, 0, "LIFE" },

        { (MGS2|MGS3), ConfigKeys::Disable_HDC_Camera_Positions_Section, ConfigKeys::Disable_HDC_Camera_Positions_Setting, ConfigKeys::Disable_HDC_Camera_Positions_Help, ConfigKeys::Disable_HDC_Camera_Positions_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { (MGS2|MGS3), ConfigKeys::Disable_HDC_Camera_Positions_ToggleKey_Section, ConfigKeys::Disable_HDC_Camera_Positions_ToggleKey_Setting, ConfigKeys::Disable_HDC_Camera_Positions_ToggleKey_Help, ConfigKeys::Disable_HDC_Camera_Positions_ToggleKey_Tooltip,
            std::make_pair(ConfigKeys::Disable_HDC_Camera_Positions_Section, ConfigKeys::Disable_HDC_Camera_Positions_Setting), false, Field::Hotkey, 0, 0, 0, "F6" },



        { (MGS2 | MGS3), ConfigKeys::Caption_Scale_Section, ConfigKeys::Caption_Scale_Setting, ConfigKeys::Caption_Scale_Help, ConfigKeys::Caption_Scale_Tooltip,
std::nullopt, false, Field::Int, 100, 1, 100},


        { (MGS2 | MGS3), ConfigKeys::Caption_Opacity_Section, ConfigKeys::Caption_Opacity_Setting, ConfigKeys::Caption_Opacity_Help, ConfigKeys::Caption_Opacity_Tooltip,
          std::nullopt, false, Field::Int, 100, 0, 100},

                { (MGS2 | MGS3), ConfigKeys::Caption_Opacity_Section, "",
          "", "",
          std::nullopt, false, Field::Spacer },


        { (MGS2 | MGS3), ConfigKeys::Caption_Background_Opacity_Section, ConfigKeys::Caption_Background_Opacity_Setting, ConfigKeys::Caption_Background_Opacity_Help, ConfigKeys::Caption_Background_Opacity_Tooltip,
          std::nullopt, false, Field::Int, 100, 0, 100 },

        { (MGS2 | MGS3), ConfigKeys::ShowSpeedrunnerOverlay_Section, ConfigKeys::ShowSpeedrunnerOverlay_Setting, ConfigKeys::ShowSpeedrunnerOverlay_Help, ConfigKeys::ShowSpeedrunnerOverlay_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, ConfigKeys::ShowSpeedrunnerOverlay_Option_Disabled, {ConfigKeys::ShowSpeedrunnerOverlay_Option_Disabled, ConfigKeys::ShowSpeedrunnerOverlay_Option_TopLeft, ConfigKeys::ShowSpeedrunnerOverlay_Option_TopRight, ConfigKeys::ShowSpeedrunnerOverlay_Option_BottomLeft, ConfigKeys::ShowSpeedrunnerOverlay_Option_BottomRight} },

        { (MGS2 | MGS3), ConfigKeys::FixIGTLoadingPause_Section, ConfigKeys::FixIGTLoadingPause_Setting, ConfigKeys::FixIGTLoadingPause_Help, ConfigKeys::FixIGTLoadingPause_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MGS2), ConfigKeys::MGS2_RestoreElevatorGlitch_Section, ConfigKeys::MGS2_RestoreElevatorGlitch_Setting, ConfigKeys::MGS2_RestoreElevatorGlitch_Help, ConfigKeys::MGS2_RestoreElevatorGlitch_Tooltip,
          std::nullopt, false, Field::Bool, false },


                    { (MGS2), ConfigKeys::MGS2_Hostage_Type_Section, ConfigKeys::MGS2_Hostage_Type_Setting, ConfigKeys::MGS2_Hostage_Type_Help, ConfigKeys::MGS2_Hostage_Type_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, ConfigKeys::MGS2_Hostage_Type_Option_Normal, {ConfigKeys::MGS2_Hostage_Type_Option_Normal, ConfigKeys::MGS2_Hostage_Type_Option_OnePM, ConfigKeys::MGS2_Hostage_Type_Option_TenPM, ConfigKeys::MGS2_Hostage_Type_Option_Midnight,} },

    }},
    { wxString("Restoration"), {


        { (MGS2), ConfigKeys::MGS2_Restore_VFX_Section, ConfigKeys::MGS2_Restore_VFX_Setting, ConfigKeys::MGS2_Restore_VFX_Help, ConfigKeys::MGS2_Restore_VFX_Tooltip,
          std::nullopt, false, Field::Bool, true },

        {(MGS2), ConfigKeys::MotionBlur_Section, ConfigKeys::MotionBlur_Setting, ConfigKeys::MotionBlur_Help, ConfigKeys::MotionBlur_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MGS2|MGS3), ConfigKeys::FixDepthOfField_Section, ConfigKeys::FixDepthOfField_Setting, ConfigKeys::FixDepthOfField_Help, ConfigKeys::FixDepthOfField_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MGS3), ConfigKeys::MGS3DepthOfFieldBlurUvMultiplier_Section, ConfigKeys::MGS3DepthOfFieldBlurUvMultiplier_Setting, ConfigKeys::MGS3DepthOfFieldBlurUvMultiplier_Help, ConfigKeys::MGS3DepthOfFieldBlurUvMultiplier_Tooltip,
          std::make_pair(ConfigKeys::MGS3DepthOfFieldBlurUvMultiplier_Section, ConfigKeys::MGS3DepthOfFieldBlurUvMultiplier_Setting), false, Field::Float, 0, 0, 0, "", {}, 10.0, 0.0, 20.0 },

        { (MGS3), ConfigKeys::MGS3_Restore_Film_Grain_Section, ConfigKeys::MGS3_Restore_Film_Grain_Setting, ConfigKeys::MGS3_Restore_Film_Grain_Help, ConfigKeys::MGS3_Restore_Film_Grain_Tooltip,
          std::nullopt, false, Field::Bool, true },


        { (MGS2|MGS3), ConfigKeys::Restore_Reverb_Level_Section, "", "", "",
        std::nullopt, false, Field::Spacer },

        { (MGS2|MGS3), ConfigKeys::Restore_Reverb_Level_Section, ConfigKeys::Restore_Reverb_Level_Setting, ConfigKeys::Restore_Reverb_Level_Help, ConfigKeys::Restore_Reverb_Level_Tooltip,
          std::nullopt, false,  Field::Bool, true},

        { (MGS2|MGS3), ConfigKeys::Restore_Reverb_Level_Scale_Section, ConfigKeys::Restore_Reverb_Level_Scale_Setting, ConfigKeys::Restore_Reverb_Level_Scale_Help, ConfigKeys::Restore_Reverb_Level_Scale_Tooltip,
          std::make_pair(ConfigKeys::Restore_Reverb_Level_Section, ConfigKeys::Restore_Reverb_Level_Setting), false,  Field::Float, 0 , 0, 0, "", {}, 1.4, 1, 100},

        { (MGS2), ConfigKeys::MGS2_RestoreActionLevelSelection_Section, ConfigKeys::MGS2_RestoreActionLevelSelection_Setting, ConfigKeys::MGS2_RestoreActionLevelSelection_Help, ConfigKeys::MGS2_RestoreActionLevelSelection_Tooltip,
          std::nullopt, false, Field::Bool, true },


        { (MGS2), ConfigKeys::RestoreDogtagNames_Section, ConfigKeys::RestoreDogtagNames_Setting, ConfigKeys::RestoreDogtagNames_Help, ConfigKeys::RestoreDogtagNames_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MGS2), ConfigKeys::MGS2_RestoreNodeDOBInfo_Section, ConfigKeys::MGS2_RestoreNodeDOBInfo_Setting, ConfigKeys::MGS2_RestoreNodeDOBInfo_Help, ConfigKeys::MGS2_RestoreNodeDOBInfo_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MGS2), ConfigKeys::MGS2_PhoneJingle_Section, ConfigKeys::MGS2_PhoneJingle_Setting, ConfigKeys::MGS2_PhoneJingle_Help, ConfigKeys::MGS2_PhoneJingle_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MGS2|MGS3), ConfigKeys::RestorePS2MemoryCardStrings_Section, ConfigKeys::RestorePS2MemoryCardStrings_Setting, ConfigKeys::RestorePS2MemoryCardStrings_Help, ConfigKeys::RestorePS2MemoryCardStrings_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { (MGS2), ConfigKeys::RestoreSoLRadarRotation_Section, ConfigKeys::RestoreSoLRadarRotation_Setting, ConfigKeys::RestoreSoLRadarRotation_Help, ConfigKeys::RestoreSoLRadarRotation_Tooltip,
          std::nullopt, false, Field::Bool, false },



        { (MGS2), ConfigKeys::MGS2_Thermal_Mode_Section, ConfigKeys::MGS2_Thermal_Mode_Setting, ConfigKeys::MGS2_Thermal_Mode_Help, ConfigKeys::MGS2_Thermal_Mode_Tooltip,
        std::nullopt, false, Field::Bool, false},

        { (MGS2), ConfigKeys::MGS2_Thermal_Cycle_Hotkey_Section, ConfigKeys::MGS2_Thermal_Cycle_Hotkey_Setting, ConfigKeys::MGS2_Thermal_Cycle_Hotkey_Help, ConfigKeys::MGS2_Thermal_Cycle_Hotkey_Tooltip,
        std::make_pair(ConfigKeys::MGS2_Thermal_Mode_Section, ConfigKeys::MGS2_Thermal_Mode_Setting), false, Field::Hotkey, 0, 0, 0, "Num7"},




        { (MGS2), ConfigKeys::MGS2_Thermal_Default_Mode_Section, ConfigKeys::MGS2_Thermal_Default_Mode_Setting, ConfigKeys::MGS2_Thermal_Default_Mode_Help, ConfigKeys::MGS2_Thermal_Default_Mode_Tooltip,
        std::make_pair(ConfigKeys::MGS2_Thermal_Mode_Section, ConfigKeys::MGS2_Thermal_Mode_Setting), false, Field::Choice, 0, 0, 0, ConfigKeys::MGS2_Thermal_Default_Mode_Option_Substance, { ConfigKeys::MGS2_Thermal_Default_Mode_Option_Substance, ConfigKeys::MGS2_Thermal_Default_Mode_Option_RedHot, ConfigKeys::MGS2_Thermal_Default_Mode_Option_SplinterCell, ConfigKeys::MGS2_Thermal_Default_Mode_Option_WhiteHot, ConfigKeys::MGS2_Thermal_Default_Mode_Option_BlackHot } },


        { (MGS2), ConfigKeys::Restore_Title_Screen_Swapping_Section, ConfigKeys::Restore_Title_Screen_Swapping_Setting, ConfigKeys::Restore_Title_Screen_Swapping_Help, ConfigKeys::Restore_Title_Screen_Swapping_Tooltip,
          std::nullopt, false, Field::Bool, true },


        { (MGS2), ConfigKeys::UnusedRetroColonel_Section, ConfigKeys::UnusedRetroColonel_Setting, ConfigKeys::UnusedRetroColonel_Help, ConfigKeys::UnusedRetroColonel_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, ConfigKeys::UnusedRetroColonel_Option_Normal, { ConfigKeys::UnusedRetroColonel_Option_Normal, ConfigKeys::UnusedRetroColonel_Option_MSX, ConfigKeys::UnusedRetroColonel_Option_Subsistence } },



      { (MGS2), ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Section, ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Setting, ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Help, ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Tooltip,
          std::nullopt, false, Field::Bool, false },


        { (MGS2), ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Toggle_Section, ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Toggle_Setting, ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Toggle_Help, ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Toggle_Tooltip,
        std::make_pair(ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Section, ConfigKeys::MGS2_RestoreOriginalDifficulty_EnableGrenadeCooking_Setting), false, Field::Hotkey, 0, 0, 0, "Num9" },


        { (MGS2), ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Section, ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Setting, ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Help, ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Tooltip,
        std::nullopt, false, Field::Choice, 0, 0, 0, ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_Disabled, {ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_Disabled,  ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_DurationIncreaseOnly, ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_LifeReductionOnly, ConfigKeys::MGS2_RestoreOriginalDifficulty_Solidus_Choking_Option_Both, } },


    } },


    { wxString("FPS Mode | 3rd Person"), {

        { (MGS2), ConfigKeys::MGS2_First_Person_View_Hold_Button_Section, ConfigKeys::MGS2_First_Person_View_Hold_Button_Setting, ConfigKeys::MGS2_First_Person_View_Hold_Button_Help, ConfigKeys::MGS2_First_Person_View_Hold_Button_Tooltip,
          std::nullopt, false, Field::Bool, false },


        { (MGS2), ConfigKeys::MGS2_First_Person_View_Hold_ToggleKey_Section, ConfigKeys::MGS2_First_Person_View_Hold_ToggleKey_Setting, ConfigKeys::MGS2_First_Person_View_Hold_ToggleKey_Help, ConfigKeys::MGS2_First_Person_View_Hold_ToggleKey_Tooltip,
        std::make_pair(ConfigKeys::MGS2_First_Person_View_Hold_Button_Section, ConfigKeys::MGS2_First_Person_View_Hold_Button_Setting), false, Field::Hotkey, 0, 0, 0, "Up" },



      { (MGS2), ConfigKeys::MGS2_First_Person_View_Enabled_Section, ConfigKeys::MGS2_First_Person_View_Enabled_Setting, ConfigKeys::MGS2_First_Person_View_Enabled_Help, ConfigKeys::MGS2_First_Person_View_Enabled_Tooltip,
            std::nullopt, false, Field::Bool, false },

        { (MGS2), ConfigKeys::MGS2_First_Person_View_ToggleKey_Section, ConfigKeys::MGS2_First_Person_View_ToggleKey_Setting, ConfigKeys::MGS2_First_Person_View_ToggleKey_Help, ConfigKeys::MGS2_First_Person_View_ToggleKey_Tooltip,
            std::make_pair(ConfigKeys::MGS2_First_Person_View_Enabled_Section, ConfigKeys::MGS2_First_Person_View_Enabled_Setting), false, Field::Hotkey, 0, 0, 0, "Right" },


        { (MGS2), ConfigKeys::MGS2_First_Person_View_Movement_Enabled_By_Default_Section, ConfigKeys::MGS2_First_Person_View_Movement_Enabled_By_Default_Setting, ConfigKeys::MGS2_First_Person_View_Movement_Enabled_By_Default_Help, ConfigKeys::MGS2_First_Person_View_Movement_Enabled_By_Default_Tooltip,
            std::make_pair(ConfigKeys::MGS2_First_Person_View_Enabled_Section, ConfigKeys::MGS2_First_Person_View_Enabled_Setting), false, Field::Bool, true },

      { (MGS2), ConfigKeys::MGS2_First_Person_View_Movement_ToggleKey_Section, ConfigKeys::MGS2_First_Person_View_Movement_ToggleKey_Setting, ConfigKeys::MGS2_First_Person_View_Movement_ToggleKey_Help, ConfigKeys::MGS2_First_Person_View_Movement_ToggleKey_Tooltip,
            std::make_pair(ConfigKeys::MGS2_First_Person_View_Enabled_Section, ConfigKeys::MGS2_First_Person_View_Enabled_Setting), false, Field::Hotkey, 0, 0, 0, "Down" },



      { (MGS2), ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Setting, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Help, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Tooltip,
            std::nullopt, false, Field::Bool, false },

      { (MGS2), ConfigKeys::MGS2_ThirdPersonFreecam_ToggleKey_Section, ConfigKeys::MGS2_ThirdPersonFreecam_ToggleKey_Setting, ConfigKeys::MGS2_ThirdPersonFreecam_ToggleKey_Help, ConfigKeys::MGS2_ThirdPersonFreecam_ToggleKey_Tooltip,
                std::make_pair(ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Setting), false, Field::Hotkey, 0, 0, 0, "Mouse5" },

      { (MGS2), ConfigKeys::MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Setting, ConfigKeys::MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Help, ConfigKeys::MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_Tooltip,
            std::make_pair(ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Setting), false, Field::Bool, false },

      { (MGS2), ConfigKeys::MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_ToggleKey_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_ToggleKey_Setting, ConfigKeys::MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_ToggleKey_Help, ConfigKeys::MGS2_ThirdPersonFreecam_Inherit_Camera_Rotation_ToggleKey_Tooltip,
                std::make_pair(ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Setting), false, Field::Hotkey, 0, 0, 0, "NumMultiply" },

      { (MGS2), ConfigKeys::MGS2_ThirdPersonFreecam_Max_Camera_Distance_Section, "",
          "", "",
          std::nullopt, false, Field::Spacer },

    { (MGS2), ConfigKeys::MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Setting, ConfigKeys::MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Help, ConfigKeys::MGS2_ThirdPersonFreecam_Horizontal_Sensitivity_Tooltip,
                std::make_pair(ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Setting), false, Field::Float, 0 , 0, 0, "", {}, k3rdPersonFreecamDefaultHorizontalSensitivity, 0.1, 10.0},



      { (MGS2), ConfigKeys::MGS2_ThirdPersonFreecam_Max_Camera_Distance_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Max_Camera_Distance_Setting, ConfigKeys::MGS2_ThirdPersonFreecam_Max_Camera_Distance_Help, ConfigKeys::MGS2_ThirdPersonFreecam_Max_Camera_Distance_Tooltip,
                std::make_pair(ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Setting), false, Field::Int, k3rdPersonFreecamDefaultMaxCameraDistance, k3rdPersonMinCameraDistance, k3rdPersonMaxCameraDistance },

      { (MGS2), ConfigKeys::MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Setting, ConfigKeys::MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Help, ConfigKeys::MGS2_ThirdPersonFreecam_Vertical_Sensitivity_Tooltip,
                std::make_pair(ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Setting), false, Field::Float, 0 , 0, 0, "", {}, k3rdPersonFreecamDefaultVerticalSensitivity, 0.1, 10.0},


      { (MGS2), ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Setting, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Help, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Step_Amount_Tooltip,
                std::make_pair(ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Setting), false, Field::Int, 250, 1, k3rdPersonMaxCameraDistance },

      { (MGS2), ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Decrease_Hotkey_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Decrease_Hotkey_Setting, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Decrease_Hotkey_Help, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Decrease_Hotkey_Tooltip,
                std::make_pair(ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Setting), false, Field::Hotkey, 0, 0, 0, "WheelUp" },

      { (MGS2), ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Reset_Hotkey_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Reset_Hotkey_Setting, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Reset_Hotkey_Help, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Reset_Hotkey_Tooltip,
                std::make_pair(ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Setting), false, Field::Hotkey, 0, 0, 0, "Mouse4" },

      { (MGS2), ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Increase_Hotkey_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Increase_Hotkey_Setting, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Increase_Hotkey_Help, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Increase_Hotkey_Tooltip,
                std::make_pair(ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Setting), false, Field::Hotkey, 0, 0, 0, "WheelDown" },


                      { (MGS2), ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Decrease_Hotkey_Section, "",
          "", "",
          std::nullopt, false, Field::Spacer },


      { (MGS2), ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Change_Speed_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Change_Speed_Setting, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Change_Speed_Help, ConfigKeys::MGS2_ThirdPersonFreecam_Camera_Distance_Change_Speed_Tooltip,
                std::make_pair(ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Section, ConfigKeys::MGS2_ThirdPersonFreecam_Enabled_Setting), false, Field::Int, 25, 1, 500 },



  } },

    { wxString("Controls | Hotkeys"), {
        { (MG|MGS2|MGS3), ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Section, ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Setting, ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Help, ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Tooltip,
          std::nullopt, false, Field::Bool, true},

        { (MGS2|MGS3), ConfigKeys::ToggleRainShader_Section, ConfigKeys::ToggleRainShader_Setting, ConfigKeys::ToggleRainShader_Help, ConfigKeys::ToggleRainShader_Tooltip,
          std::make_pair(ConfigKeys::MGS2_Restore_VFX_Section, ConfigKeys::MGS2_Restore_VFX_Setting), false, Field::Hotkey, 0, 0, 0, "Insert" },

        { (MGS2|MGS3), ConfigKeys::CycleWireframeMode_Section, ConfigKeys::CycleWireframeMode_Setting, ConfigKeys::CycleWireframeMode_Help, ConfigKeys::CycleWireframeMode_Tooltip,
          std::make_pair(ConfigKeys::MGS2_Restore_VFX_Section, ConfigKeys::MGS2_Restore_VFX_Setting), false, Field::Hotkey, 0, 0, 0, "End"},

        { (MGS3), ConfigKeys::OverrideMouseSensitivity_Section, ConfigKeys::OverrideMouseSensitivity_Setting, ConfigKeys::OverrideMouseSensitivity_Help, ConfigKeys::OverrideMouseSensitivity_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { (MGS3), ConfigKeys::MouseSensitivity_XMultiplier_Section, ConfigKeys::MouseSensitivity_XMultiplier_Setting, ConfigKeys::MouseSensitivity_XMultiplier_Help, ConfigKeys::MouseSensitivity_XMultiplier_Tooltip,
          std::make_pair(ConfigKeys::OverrideMouseSensitivity_Section, ConfigKeys::OverrideMouseSensitivity_Setting), false,
          Field::Int, 1, 1, 100 },

        { (MGS3), ConfigKeys::OverrideMouseSensitivity_Section, "",
          "", "",
          std::nullopt, false, Field::Spacer },

        { (MGS3), ConfigKeys::MouseSensitivity_YMultiplier_Section, ConfigKeys::MouseSensitivity_YMultiplier_Setting, ConfigKeys::MouseSensitivity_YMultiplier_Help, ConfigKeys::MouseSensitivity_YMultiplier_Tooltip,
          std::make_pair(ConfigKeys::OverrideMouseSensitivity_Section, ConfigKeys::OverrideMouseSensitivity_Setting), false,
          Field::Int, 1, 1, 100 },

        {
         (MGS2|MGS3), ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Section, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Setting, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Help, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Tooltip,
        std::make_pair(ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting), true, Field::Bool, true
           },

        { (MGS2|MGS3), ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting, ConfigKeys::KeepAimingAfterFiring_Always_Help, ConfigKeys::KeepAimingAfterFiring_Always_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { (MGS2), ConfigKeys::KeepAimingAfterFiring_InFPSMode_Section, ConfigKeys::KeepAimingAfterFiring_InFPSMode_Setting, ConfigKeys::KeepAimingAfterFiring_InFPSMode_Help, ConfigKeys::KeepAimingAfterFiring_InFPSMode_Tooltip,
          std::make_pair(ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting), true, Field::Bool, false },


        { (MGS2|MGS3), ConfigKeys::KeepAimingAfterFiring_OnLockOn_Section, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Setting, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Help, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Tooltip,
          std::make_pair(ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting), true, Field::Bool, true },

      } },

    { wxString("Achievements"), {
        { (MGS2|MGS3), ConfigKeys::AchievementPersistence_Section, ConfigKeys::AchievementPersistence_Setting, ConfigKeys::AchievementPersistence_Help, ConfigKeys::AchievementPersistence_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MG|MGS2|MGS3), ConfigKeys::DisableSteamAchievements_Section, ConfigKeys::DisableSteamAchievements_Setting, ConfigKeys::DisableSteamAchievements_Help, ConfigKeys::DisableSteamAchievements_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { (MG|MGS2|MGS3), ConfigKeys::ResetAllAchievements_Section, "Safety Switch",
          ConfigKeys::ResetAllAchievements_Help, ConfigKeys::ResetAllAchievements_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { (MG|MGS2|MGS3), ConfigKeys::ResetAllAchievements_Section, ConfigKeys::ResetAllAchievements_Setting, ConfigKeys::ResetAllAchievements_Help, ConfigKeys::ResetAllAchievements_Tooltip,
          std::make_pair(ConfigKeys::ResetAllAchievements_Section, "Safety Switch"), false, Field::Bool, false },

    }},
    { wxString("MGSHDFix / Internal"), {
        { (MG|MGS2|MGS3), ConfigKeys::CheckForUpdates_Section, ConfigKeys::CheckForUpdates_Setting, ConfigKeys::CheckForUpdates_Help, ConfigKeys::CheckForUpdates_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MG|MGS2|MGS3), ConfigKeys::UpdateConsoleNotifications_Section, ConfigKeys::UpdateConsoleNotifications_Setting, ConfigKeys::UpdateConsoleNotifications_Help, ConfigKeys::UpdateConsoleNotifications_Tooltip,
          std::make_pair(ConfigKeys::CheckForUpdates_Section, ConfigKeys::CheckForUpdates_Setting), false, Field::Bool, true},

        { (MG|MGS2|MGS3), ConfigKeys::MuteWarning_Section, ConfigKeys::MuteWarning_Setting, ConfigKeys::MuteWarning_Help, ConfigKeys::MuteWarning_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MG|MGS2|MGS3), ConfigKeys::FSRWarning_Section, ConfigKeys::FSRWarning_Setting, ConfigKeys::FSRWarning_Help, ConfigKeys::FSRWarning_Tooltip,
          std::nullopt, false, Field::Bool, true },


        { (MG|MGS2|MGS3), ConfigKeys::MissingBugfixModWarning_Section, ConfigKeys::MissingBugfixModWarning_Setting, ConfigKeys::MissingBugfixModWarning_Help, ConfigKeys::MissingBugfixModWarning_Tooltip,
          std::nullopt, false, Field::Bool, true },


        { (MG|MGS2|MGS3), ConfigKeys::SaveFolderWriteWarning_Section, ConfigKeys::SaveFolderWriteWarning_Setting, ConfigKeys::SaveFolderWriteWarning_Help, ConfigKeys::SaveFolderWriteWarning_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MG|MGS2|MGS3), ConfigKeys::WindowsSlideshowWarning_Section, ConfigKeys::WindowsSlideshowWarning_Setting, ConfigKeys::WindowsSlideshowWarning_Help, ConfigKeys::WindowsSlideshowWarning_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MG|MGS2|MGS3), ConfigKeys::SaveFileReadOnlyWarning_Section, ConfigKeys::SaveFileReadOnlyWarning_Setting, ConfigKeys::SaveFileReadOnlyWarning_Help, ConfigKeys::SaveFileReadOnlyWarning_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { (MG|MGS2|MGS3), ConfigKeys::VerboseLogging_Section, ConfigKeys::VerboseLogging_Setting, ConfigKeys::VerboseLogging_Help, ConfigKeys::VerboseLogging_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { (MGS2|MGS3), ConfigKeys::Debugging_Start_In_Dev_Menu_Section, ConfigKeys::Debugging_Start_In_Dev_Menu_Setting, ConfigKeys::Debugging_Start_In_Dev_Menu_Help, ConfigKeys::Debugging_Start_In_Dev_Menu_Tooltip,
          std::nullopt, false, Field::Bool, false },

        {(MG|MGS2|MGS3), "About", "", "", "", std::nullopt, false, Field::Spacer},

    }}
};
