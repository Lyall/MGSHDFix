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
#include "tab_data.hpp"
#include <d3d11.h>

#include "config_keys.hpp"

const std::vector<std::pair<wxString, std::vector<Field>>> kTabs = {
    { wxString("General"), {
        { ConfigKeys::EffectSpeedFixes_Section, ConfigKeys::EffectSpeedFixes_Setting, ConfigKeys::EffectSpeedFixes_Help, ConfigKeys::EffectSpeedFixes_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { ConfigKeys::FixAimingAfterEquip_Section, ConfigKeys::FixAimingAfterEquip_Setting, ConfigKeys::FixAimingAfterEquip_Help, ConfigKeys::FixAimingAfterEquip_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { ConfigKeys::DisableMouseCursor_Section, ConfigKeys::DisableMouseCursor_Setting, ConfigKeys::DisableMouseCursor_Help, ConfigKeys::DisableMouseCursor_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { ConfigKeys::FixAimingFullTilt_Section, ConfigKeys::FixAimingFullTilt_Setting, ConfigKeys::FixAimingFullTilt_Help, ConfigKeys::FixAimingFullTilt_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Section, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Setting, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Help, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { ConfigKeys::ForceStereoAudio_Section, ConfigKeys::ForceStereoAudio_Setting, ConfigKeys::ForceStereoAudio_Help, ConfigKeys::ForceStereoAudio_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { ConfigKeys::CPUCoreLimit_Section, ConfigKeys::CPUCoreLimit_Setting, ConfigKeys::CPUCoreLimit_Help, ConfigKeys::CPUCoreLimit_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Section, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Setting, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Help, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Tooltip,
          std::make_pair(ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting), true, Field::Bool, true },

        { ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting, ConfigKeys::KeepAimingAfterFiring_Always_Help, ConfigKeys::KeepAimingAfterFiring_Always_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { ConfigKeys::KeepAimingAfterFiring_OnLockOn_Section, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Setting, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Help, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Tooltip,
          std::make_pair(ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting), true, Field::Bool, true },

        { ConfigKeys::Region_Section, ConfigKeys::Region_Setting, ConfigKeys::Region_Help, ConfigKeys::Region_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, *std::next(kLauncherConfigRegions.begin(), 0),
          { std::begin(kLauncherConfigRegions), std::end(kLauncherConfigRegions) } },

        { ConfigKeys::Language_Section, ConfigKeys::Language_Setting, ConfigKeys::Language_Help, ConfigKeys::Language_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, *std::next(kLauncherConfigLanguages.begin(), 0),
          { std::begin(kLauncherConfigLanguages), std::end(kLauncherConfigLanguages) } },

        { ConfigKeys::CtrlType_Section, ConfigKeys::CtrlType_Setting, ConfigKeys::CtrlType_Help, ConfigKeys::CtrlType_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, *std::next(kLauncherConfigCtrlTypes.begin(), 5),
          { std::begin(kLauncherConfigCtrlTypes), std::end(kLauncherConfigCtrlTypes) } },

        { ConfigKeys::CtrlType_Section, "",
          "", "",
          std::nullopt, false, Field::Spacer }
    }},
    { wxString("Graphics"), {
        { ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting, ConfigKeys::ForceWindowSize_Help, ConfigKeys::ForceWindowSize_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { ConfigKeys::WindowWidth_Section, ConfigKeys::WindowWidth_Setting, ConfigKeys::WindowWidth_Help, ConfigKeys::WindowWidth_Tooltip,
          std::make_pair(ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting), false, Field::Int, 0, 0, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION },

        { ConfigKeys::WindowedMode_Section, ConfigKeys::WindowedMode_Setting, ConfigKeys::WindowedMode_Help, ConfigKeys::WindowedMode_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { ConfigKeys::WindowHeight_Section, ConfigKeys::WindowHeight_Setting, ConfigKeys::WindowHeight_Help, ConfigKeys::WindowHeight_Tooltip,
          std::make_pair(ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting), false, Field::Int, 0, 0, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION },

        { ConfigKeys::BorderlessWindowed_Section, ConfigKeys::BorderlessWindowed_Setting, ConfigKeys::BorderlessWindowed_Help, ConfigKeys::BorderlessWindowed_Tooltip,
          std::make_pair(ConfigKeys::WindowedMode_Section, ConfigKeys::WindowedMode_Setting), false, Field::Bool, true },

        { ConfigKeys::RenderScaleWidth_Section, ConfigKeys::RenderScaleWidth_Setting, ConfigKeys::RenderScaleWidth_Help, ConfigKeys::RenderScaleWidth_Tooltip,
          std::nullopt, false, Field::Int, 0, 0, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION },

        { ConfigKeys::RenderScaleHeight_Section, ConfigKeys::RenderScaleHeight_Setting, ConfigKeys::RenderScaleHeight_Help, ConfigKeys::RenderScaleHeight_Tooltip,
          std::nullopt, false, Field::Int, 0, 0, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION },

        { ConfigKeys::AnisotropicFiltering_Section, ConfigKeys::AnisotropicFiltering_Setting, ConfigKeys::AnisotropicFiltering_Help, ConfigKeys::AnisotropicFiltering_Tooltip,
          std::make_pair(ConfigKeys::DisableTextureFiltering_Section, ConfigKeys::DisableTextureFiltering_Setting), true, Field::Int, 16, 0, D3D11_DEFAULT_MAX_ANISOTROPY},

        { ConfigKeys::DisableTextureFiltering_Section, ConfigKeys::DisableTextureFiltering_Setting, ConfigKeys::DisableTextureFiltering_Help, ConfigKeys::DisableTextureFiltering_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { ConfigKeys::FixVectorRain_Section, ConfigKeys::FixVectorRain_Setting, ConfigKeys::FixVectorRain_Help, ConfigKeys::FixVectorRain_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { ConfigKeys::VectorLineScale_Section, ConfigKeys::VectorLineScale_Setting, ConfigKeys::VectorLineScale_Help, ConfigKeys::VectorLineScale_Tooltip,
          std::nullopt, false, Field::Int, 360, 1, (1 << 12) },

        { ConfigKeys::FixVectorUI_Section, ConfigKeys::FixVectorUI_Setting, ConfigKeys::FixVectorUI_Help, ConfigKeys::FixVectorUI_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { ConfigKeys::FixAspectRatio_Section, ConfigKeys::FixAspectRatio_Setting, ConfigKeys::FixAspectRatio_Help, ConfigKeys::FixAspectRatio_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { ConfigKeys::FixHUD_Section, ConfigKeys::FixHUD_Setting, ConfigKeys::FixHUD_Help, ConfigKeys::FixHUD_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { ConfigKeys::FixFOV_Section, ConfigKeys::FixFOV_Setting, ConfigKeys::FixFOV_Help, ConfigKeys::FixFOV_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { ConfigKeys::FramebufferFix_Section, ConfigKeys::FramebufferFix_Setting, ConfigKeys::FramebufferFix_Help, ConfigKeys::FramebufferFix_Tooltip,
          std::nullopt, false, Field::Bool, true }
    }},
    { wxString("Tweaks"), {
        { ConfigKeys::LauncherJumpStart_Section, ConfigKeys::LauncherJumpStart_Setting, ConfigKeys::LauncherJumpStart_Help, ConfigKeys::LauncherJumpStart_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { ConfigKeys::SkipIntroLogos_Section, ConfigKeys::SkipIntroLogos_Setting, ConfigKeys::SkipIntroLogos_Help, ConfigKeys::SkipIntroLogos_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { ConfigKeys::SkipLauncher_Section, ConfigKeys::SkipLauncher_Setting, ConfigKeys::SkipLauncher_Help, ConfigKeys::SkipLauncher_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { ConfigKeys::SkipLauncherMSXGame_Section, ConfigKeys::SkipLauncherMSXGame_Setting, ConfigKeys::SkipLauncherMSXGame_Help, ConfigKeys::SkipLauncherMSXGame_Tooltip,
          std::make_pair(ConfigKeys::SkipLauncher_Section, ConfigKeys::SkipLauncher_Setting), false, Field::Choice, 0, 0, 0, ConfigKeys::SkipLauncherMSX_Option_MG1, {ConfigKeys::SkipLauncherMSX_Option_MG1,ConfigKeys::SkipLauncherMSX_Option_MG2}},

        { ConfigKeys::MSXWallType_Section, ConfigKeys::MSXWallType_Setting, ConfigKeys::MSXWallType_Help, ConfigKeys::MSXWallType_Tooltip,
          std::nullopt, false, Field::Int, 0 , 0, 6},

        { ConfigKeys::MSXWallAlign_Section, ConfigKeys::MSXWallAlign_Setting, ConfigKeys::MSXWallAlign_Help, ConfigKeys::MSXWallAlign_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, ConfigKeys::MSXWallAlign_Option_Center, {ConfigKeys::MSXWallAlign_Option_Center, ConfigKeys::MSXWallAlign_Option_Left, ConfigKeys::MSXWallAlign_Option_Right} },

        { ConfigKeys::EnablePauseOnFocusLoss_Section, ConfigKeys::EnablePauseOnFocusLoss_Setting, ConfigKeys::EnablePauseOnFocusLoss_Help, ConfigKeys::EnablePauseOnFocusLoss_Tooltip,
          std::nullopt, false, Field::Bool, false },


        { ConfigKeys::MGS2Sunglasses_Section, ConfigKeys::MGS2Sunglasses_Setting, ConfigKeys::MGS2Sunglasses_Help, ConfigKeys::MGS2Sunglasses_Tooltip,
          std::nullopt, false, Field::Choice, 0, 0, 0, ConfigKeys::MGS2Sunglasses_Option_Normal, {ConfigKeys::MGS2Sunglasses_Option_Normal,ConfigKeys::MGS2Sunglasses_Option_Always, ConfigKeys::MGS2Sunglasses_Option_Never } },

        { ConfigKeys::DistanceCullingGrassAlways_Section, ConfigKeys::DistanceCullingGrassAlways_Setting, ConfigKeys::DistanceCullingGrassAlways_Help, ConfigKeys::DistanceCullingGrassAlways_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { ConfigKeys::DistanceCullingGrassScalar_Section, ConfigKeys::DistanceCullingGrassScalar_Setting, ConfigKeys::DistanceCullingGrassScalar_Help, ConfigKeys::DistanceCullingGrassScalar_Tooltip,
          std::make_pair(ConfigKeys::DistanceCullingGrassAlways_Section, ConfigKeys::DistanceCullingGrassAlways_Setting), true, Field::Float, 0 , 0, 0, "", {}, 1.0, 0},
    }},
    { wxString("Controls | Hotkeys"), {
        { ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Section, ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Setting, ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Help, ConfigKeys::CaptureInputsWhileAltTabbedHotkey_Tooltip,
          std::nullopt, false, Field::Bool, false},

        { ConfigKeys::ToggleRainShader_Section, ConfigKeys::ToggleRainShader_Setting, ConfigKeys::ToggleRainShader_Help, ConfigKeys::ToggleRainShader_Tooltip,
          std::make_pair(ConfigKeys::FixVectorRain_Section, ConfigKeys::FixVectorRain_Setting), false, Field::Hotkey, 0, 0, 0, "Insert" },

        { ConfigKeys::CycleWireframeMode_Section, ConfigKeys::CycleWireframeMode_Setting, ConfigKeys::CycleWireframeMode_Help, ConfigKeys::CycleWireframeMode_Tooltip,
          std::make_pair(ConfigKeys::FixVectorRain_Section, ConfigKeys::FixVectorRain_Setting), false, Field::Hotkey, 0, 0, 0, "End"},

        { ConfigKeys::ToggleUIShader_Section, ConfigKeys::ToggleUIShader_Setting, ConfigKeys::ToggleUIShader_Help, ConfigKeys::ToggleUIShader_Tooltip,
          std::make_pair(ConfigKeys::FixVectorUI_Section, ConfigKeys::FixVectorUI_Setting), false, Field::Hotkey, 0, 0, 0, "Delete" },

        { ConfigKeys::ToggleDistanceCullingGrass_Section, ConfigKeys::ToggleDistanceCullingGrass_Setting, ConfigKeys::ToggleDistanceCullingGrass_Help, ConfigKeys::ToggleDistanceCullingGrass_Tooltip,
          std::make_pair(ConfigKeys::DistanceCullingGrassAlways_Section, ConfigKeys::DistanceCullingGrassAlways_Setting), false, Field::Hotkey, 0, 0, 0, "Page Up" },


        { ConfigKeys::OverrideMouseSensitivity_Section, ConfigKeys::OverrideMouseSensitivity_Setting, ConfigKeys::OverrideMouseSensitivity_Help, ConfigKeys::OverrideMouseSensitivity_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { ConfigKeys::MouseSensitivity_XMultiplier_Section, ConfigKeys::MouseSensitivity_XMultiplier_Setting, ConfigKeys::MouseSensitivity_XMultiplier_Help, ConfigKeys::MouseSensitivity_XMultiplier_Tooltip,
          std::make_pair(ConfigKeys::OverrideMouseSensitivity_Section, ConfigKeys::OverrideMouseSensitivity_Setting), false,
          Field::Int, 1, 1, 100 },

        { ConfigKeys::OverrideMouseSensitivity_Section, "",
          "", "",
          std::nullopt, false, Field::Spacer },

        { ConfigKeys::MouseSensitivity_YMultiplier_Section, ConfigKeys::MouseSensitivity_YMultiplier_Setting, ConfigKeys::MouseSensitivity_YMultiplier_Help, ConfigKeys::MouseSensitivity_YMultiplier_Tooltip,
          std::make_pair(ConfigKeys::OverrideMouseSensitivity_Section, ConfigKeys::OverrideMouseSensitivity_Setting), false,
          Field::Int, 1, 1, 100 }
    }},
    { wxString("Achievements"), {
        { ConfigKeys::AchievementPersistence_Section, ConfigKeys::AchievementPersistence_Setting, ConfigKeys::AchievementPersistence_Help, ConfigKeys::AchievementPersistence_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { ConfigKeys::ResetAllAchievements_Section, "Safety Switch",
          ConfigKeys::ResetAllAchievements_Help, ConfigKeys::ResetAllAchievements_Tooltip,
          std::nullopt, false, Field::Bool, false },

        { ConfigKeys::ResetAllAchievements_Section, ConfigKeys::ResetAllAchievements_Setting, ConfigKeys::ResetAllAchievements_Help, ConfigKeys::ResetAllAchievements_Tooltip,
          std::make_pair(ConfigKeys::ResetAllAchievements_Section, "Safety Switch"), false, Field::Bool, false }
    }},
    { wxString("MGSHDFix / Internal"), {
        { ConfigKeys::CheckForUpdates_Section, ConfigKeys::CheckForUpdates_Setting, ConfigKeys::CheckForUpdates_Help, ConfigKeys::CheckForUpdates_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { ConfigKeys::UpdateConsoleNotifications_Section, ConfigKeys::UpdateConsoleNotifications_Setting, ConfigKeys::UpdateConsoleNotifications_Help, ConfigKeys::UpdateConsoleNotifications_Tooltip,
          std::make_pair(ConfigKeys::CheckForUpdates_Section, ConfigKeys::CheckForUpdates_Setting), false, Field::Bool, true},

        { ConfigKeys::MuteWarning_Section, ConfigKeys::MuteWarning_Setting, ConfigKeys::MuteWarning_Help, ConfigKeys::MuteWarning_Tooltip,
          std::nullopt, false, Field::Bool, true },

        { ConfigKeys::VerboseLogging_Section, ConfigKeys::VerboseLogging_Setting, ConfigKeys::VerboseLogging_Help, ConfigKeys::VerboseLogging_Tooltip,
          std::nullopt, false, Field::Bool, false },

        {"About", "", "", "", std::nullopt, false, Field::Spacer},

    }}
};
