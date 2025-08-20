#pragma once
#include "config_keys.hpp"
#include <vector>
#include <wx/string.h>

struct Field
{
    wxString section;
    wxString key;
    enum Type
    {
        Bool,
        Int,
        Str,
        Choice,
        Hotkey // key/mouse capture
    } type;

    wxString defaultString;
    int defaultInt = 0;
    std::vector<wxString> choices;

    wxString help;     // subtitle shown under the label
    wxString tooltip;  // hover text shown on the actual control

    // NEW: optional prerequisite, defaults to none
    std::optional<std::pair<wxString, wxString>> prerequisite = std::nullopt;
    bool prerequisiteNegate = false;
};
// ----------------- FULL SCHEMA -----------------
static const std::vector<std::pair<wxString, std::vector<Field>>> kTabs = {
    { wxString("General"), {
        { ConfigKeys::EffectSpeedFixes_Section, ConfigKeys::EffectSpeedFixes_Setting, Field::Bool, "", 1, {}, ConfigKeys::EffectSpeedFixes_Help, ConfigKeys::EffectSpeedFixes_Tooltip },
        { ConfigKeys::FixAimingAfterEquip_Section, ConfigKeys::FixAimingAfterEquip_Setting, Field::Bool, "", 1, {}, ConfigKeys::FixAimingAfterEquip_Help, ConfigKeys::FixAimingAfterEquip_Tooltip },
        { ConfigKeys::DisableMouseCursor_Section, ConfigKeys::DisableMouseCursor_Setting, Field::Bool, "", 1, {}, ConfigKeys::DisableMouseCursor_Help, ConfigKeys::DisableMouseCursor_Tooltip },
        { ConfigKeys::FixAimingFullTilt_Section, ConfigKeys::FixAimingFullTilt_Setting, Field::Bool, "", 1, {}, ConfigKeys::FixAimingFullTilt_Help, ConfigKeys::FixAimingFullTilt_Tooltip },
        { ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Section, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Setting, Field::Bool, "", 1, {}, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Help, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Tooltip },
        { ConfigKeys::ForceStereoAudio_Section, ConfigKeys::ForceStereoAudio_Setting, Field::Bool, "", 0, {}, ConfigKeys::ForceStereoAudio_Help, ConfigKeys::ForceStereoAudio_Tooltip },
        { ConfigKeys::CPUCoreLimit_Section, ConfigKeys::CPUCoreLimit_Setting, Field::Bool, "", 0, {}, ConfigKeys::CPUCoreLimit_Help, ConfigKeys::CPUCoreLimit_Tooltip },
        { ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Section, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Setting, Field::Bool, "", 1, {}, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Help, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Tooltip,
                std::make_pair(ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting), true},
        { ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting, Field::Bool, "", 0, {}, ConfigKeys::KeepAimingAfterFiring_Always_Help, ConfigKeys::KeepAimingAfterFiring_Always_Tooltip },
        { ConfigKeys::KeepAimingAfterFiring_OnLockOn_Section, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Setting, Field::Bool, "", 1, {}, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Help, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Tooltip,
                std::make_pair(ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting), true},
        { ConfigKeys::EnablePauseOnFocusLoss_Section, ConfigKeys::EnablePauseOnFocusLoss_Setting, Field::Bool, "", 0, {}, ConfigKeys::EnablePauseOnFocusLoss_Help, ConfigKeys::EnablePauseOnFocusLoss_Tooltip },
        { ConfigKeys::Region_Section, ConfigKeys::Region_Setting, Field::Choice, "US", 0, { std::begin(kLauncherConfigRegions), std::end(kLauncherConfigRegions) }, ConfigKeys::Region_Help, ConfigKeys::Region_Tooltip },
        { ConfigKeys::Language_Section, ConfigKeys::Language_Setting, Field::Choice, "EN", 0, { std::begin(kLauncherConfigLanguages), std::end(kLauncherConfigLanguages) }, ConfigKeys::Language_Help, ConfigKeys::Language_Tooltip },
        { ConfigKeys::CtrlType_Section, ConfigKeys::CtrlType_Setting, Field::Choice, "XBOX", 0, { std::begin(kLauncherConfigCtrlTypes), std::end(kLauncherConfigCtrlTypes) }, ConfigKeys::CtrlType_Help, ConfigKeys::CtrlType_Tooltip },
    }},
    { wxString("Graphics"), {
        { ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting, Field::Bool, "", 1, {}, ConfigKeys::ForceWindowSize_Help, ConfigKeys::ForceWindowSize_Tooltip },
        { ConfigKeys::WindowWidth_Section, ConfigKeys::WindowWidth_Setting, Field::Int, "", 0, {}, ConfigKeys::WindowWidth_Help, ConfigKeys::WindowWidth_Tooltip,
            std::make_pair(ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting) },
        { ConfigKeys::WindowedMode_Section, ConfigKeys::WindowedMode_Setting, Field::Bool, "", 1, {}, ConfigKeys::WindowedMode_Help, ConfigKeys::WindowedMode_Tooltip },
        { ConfigKeys::WindowHeight_Section, ConfigKeys::WindowHeight_Setting, Field::Int, "", 0, {}, ConfigKeys::WindowHeight_Help, ConfigKeys::WindowHeight_Tooltip,
            std::make_pair(ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting) },
        { ConfigKeys::BorderlessWindowed_Section, ConfigKeys::BorderlessWindowed_Setting, Field::Bool, "", 1, {}, ConfigKeys::BorderlessWindowed_Help, ConfigKeys::BorderlessWindowed_Tooltip,
                std::make_pair(ConfigKeys::WindowedMode_Section, ConfigKeys::WindowedMode_Setting)},
        { ConfigKeys::RenderScaleWidth_Section, ConfigKeys::RenderScaleWidth_Setting, Field::Int, "", 0, {}, ConfigKeys::RenderScaleWidth_Help, ConfigKeys::RenderScaleWidth_Tooltip },
        { ConfigKeys::RenderScaleHeight_Section, ConfigKeys::RenderScaleHeight_Setting, Field::Int, "", 0, {}, ConfigKeys::RenderScaleHeight_Help, ConfigKeys::RenderScaleHeight_Tooltip },
        { ConfigKeys::AnisotropicFiltering_Section, ConfigKeys::AnisotropicFiltering_Setting, Field::Int, "", 16, {}, ConfigKeys::AnisotropicFiltering_Help, ConfigKeys::AnisotropicFiltering_Tooltip },
        { ConfigKeys::DisableTextureFiltering_Section, ConfigKeys::DisableTextureFiltering_Setting, Field::Bool, "", 0, {}, ConfigKeys::DisableTextureFiltering_Help, ConfigKeys::DisableTextureFiltering_Tooltip },
        { ConfigKeys::FixVectorRain_Section, ConfigKeys::FixVectorRain_Setting, Field::Bool, "", 1, {}, ConfigKeys::FixVectorRain_Help, ConfigKeys::FixVectorRain_Tooltip },
        { ConfigKeys::VectorLineScale_Section, ConfigKeys::VectorLineScale_Setting, Field::Int, "", 360, {}, ConfigKeys::VectorLineScale_Help, ConfigKeys::VectorLineScale_Tooltip },
        { ConfigKeys::FixVectorUI_Section, ConfigKeys::FixVectorUI_Setting, Field::Bool, "", 1, {}, ConfigKeys::FixVectorUI_Help, ConfigKeys::FixVectorUI_Tooltip },
        { ConfigKeys::FixAspectRatio_Section, ConfigKeys::FixAspectRatio_Setting, Field::Bool, "", 1, {}, ConfigKeys::FixAspectRatio_Help, ConfigKeys::FixAspectRatio_Tooltip },
        { ConfigKeys::FixHUD_Section, ConfigKeys::FixHUD_Setting, Field::Bool, "", 0, {}, ConfigKeys::FixHUD_Help, ConfigKeys::FixHUD_Tooltip },
        { ConfigKeys::FixFOV_Section, ConfigKeys::FixFOV_Setting, Field::Bool, "", 1, {}, ConfigKeys::FixFOV_Help, ConfigKeys::FixFOV_Tooltip },
        { ConfigKeys::FramebufferFix_Section, ConfigKeys::FramebufferFix_Setting, Field::Bool, "", 1, {}, ConfigKeys::FramebufferFix_Help, ConfigKeys::FramebufferFix_Tooltip },
    }},
    { wxString("Tweaks"), {
        { ConfigKeys::LauncherJumpStart_Section, ConfigKeys::LauncherJumpStart_Setting, Field::Bool, "", 0, {}, ConfigKeys::LauncherJumpStart_Help, ConfigKeys::LauncherJumpStart_Tooltip },
        { ConfigKeys::SkipIntroLogos_Section, ConfigKeys::SkipIntroLogos_Setting, Field::Bool, "", 0, {}, ConfigKeys::SkipIntroLogos_Help, ConfigKeys::SkipIntroLogos_Tooltip },
        { ConfigKeys::SkipLauncher_Section, ConfigKeys::SkipLauncher_Setting, Field::Bool, "", 0, {}, ConfigKeys::SkipLauncher_Help, ConfigKeys::SkipLauncher_Tooltip },
        { ConfigKeys::SkipLauncherMSXGame_Section, ConfigKeys::SkipLauncherMSXGame_Setting, Field::Choice, "MG1", 0, {"MG1","MG2"}, ConfigKeys::SkipLauncherMSXGame_Help, ConfigKeys::SkipLauncherMSXGame_Tooltip },
        { ConfigKeys::MSXWallType_Section, ConfigKeys::MSXWallType_Setting, Field::Int, "", 0, {}, ConfigKeys::MSXWallType_Help, ConfigKeys::MSXWallType_Tooltip },
        { ConfigKeys::MSXWallAlign_Section, ConfigKeys::MSXWallAlign_Setting, Field::Choice, "Center", 0, {"Left","Right","Center"}, ConfigKeys::MSXWallAlign_Help, ConfigKeys::MSXWallAlign_Tooltip },
        { ConfigKeys::MuteWarning_Section, ConfigKeys::MuteWarning_Setting, Field::Bool, "", 1, {}, ConfigKeys::MuteWarning_Help, ConfigKeys::MuteWarning_Tooltip },
        { ConfigKeys::MGS2Sunglasses_Section, ConfigKeys::MGS2Sunglasses_Setting, Field::Choice, "Normal", 0, {"Normal","Always","Never"}, ConfigKeys::MGS2Sunglasses_Help, ConfigKeys::MGS2Sunglasses_Tooltip },
    }},
    { wxString("Controls | Hotkeys"), {
        { ConfigKeys::ToggleRainShader_Section, ConfigKeys::ToggleRainShader_Setting, Field::Hotkey, "Insert", 0, {}, ConfigKeys::ToggleRainShader_Help, ConfigKeys::ToggleRainShader_Tooltip },
        { ConfigKeys::ToggleUIShader_Section, ConfigKeys::ToggleUIShader_Setting, Field::Hotkey, "Delete", 0, {}, ConfigKeys::ToggleUIShader_Help, ConfigKeys::ToggleUIShader_Tooltip },
        { ConfigKeys::CycleWireframeMode_Section, ConfigKeys::CycleWireframeMode_Setting, Field::Hotkey, "End", 0, {}, ConfigKeys::CycleWireframeMode_Help, ConfigKeys::CycleWireframeMode_Tooltip },
        { ConfigKeys::OverrideMouseSensitivity_Section, ConfigKeys::OverrideMouseSensitivity_Setting, Field::Bool, "", 0, {}, ConfigKeys::OverrideMouseSensitivity_Help, ConfigKeys::OverrideMouseSensitivity_Tooltip },
        { ConfigKeys::MouseSensitivity_XMultiplier_Section, ConfigKeys::MouseSensitivity_XMultiplier_Setting, Field::Int, "", 1, {}, ConfigKeys::MouseSensitivity_XMultiplier_Help, ConfigKeys::MouseSensitivity_XMultiplier_Tooltip },
        { ConfigKeys::MouseSensitivity_YMultiplier_Section, ConfigKeys::MouseSensitivity_YMultiplier_Setting, Field::Int, "", 1, {}, ConfigKeys::MouseSensitivity_YMultiplier_Help, ConfigKeys::MouseSensitivity_YMultiplier_Tooltip },
    }},
    { wxString("Achievements"), {
        { ConfigKeys::AchievementPersistence_Section, ConfigKeys::AchievementPersistence_Setting, Field::Bool, "", 1, {}, ConfigKeys::AchievementPersistence_Help, ConfigKeys::AchievementPersistence_Tooltip },
        { ConfigKeys::ResetAllAchievements_Section, "Safety Switch", Field::Bool, "", 0, {}, ConfigKeys::ResetAllAchievements_Help, ConfigKeys::ResetAllAchievements_Tooltip },
        { ConfigKeys::ResetAllAchievements_Section, ConfigKeys::ResetAllAchievements_Setting, Field::Bool, "", 0, {}, ConfigKeys::ResetAllAchievements_Help, ConfigKeys::ResetAllAchievements_Tooltip,
            std::make_pair(ConfigKeys::ResetAllAchievements_Section, "Safety Switch")},
    }},
    { wxString("MGSHDFix / Internal"), {
        { ConfigKeys::CheckForUpdates_Section, ConfigKeys::CheckForUpdates_Setting, Field::Bool, "", 1, {}, ConfigKeys::CheckForUpdates_Help, ConfigKeys::CheckForUpdates_Tooltip },
        { ConfigKeys::UpdateConsoleNotifications_Section, ConfigKeys::UpdateConsoleNotifications_Setting, Field::Bool, "", 1, {}, ConfigKeys::UpdateConsoleNotifications_Help, ConfigKeys::UpdateConsoleNotifications_Tooltip },
        { ConfigKeys::VerboseLogging_Section, ConfigKeys::VerboseLogging_Setting, Field::Bool, "", 0, {}, ConfigKeys::VerboseLogging_Help, ConfigKeys::VerboseLogging_Tooltip },
    }},
};
