#include "stdafx.h"
#include "config_keys.hpp"

#include "mgs2_3rd_person_freecam.hpp"
#include "common.hpp"
#include "gamevars.hpp"
#include "input_handler.hpp"
#include "logging.hpp"
using namespace MGS2_StatusFlags;

namespace
{
    //TODO: 
    //      - disable camera angles when leaning against walls | partially done. see v3 known issues below
	// Make transition to static cam hold inverted input
	// Falling into ocean needs to force static

    std::int32_t* gBP_3rdPersonCamera_Override = nullptr;

    int* gBP_3rdPersonCamera_Dist = nullptr;         // max camera distance from player
    int* gBP_Camera_InheritRot = nullptr;              // Inherit rotation between cameras
                                                        // the code suggests it has something to do with elevator and locker focus, but i haven't noticed it actually do anything.

    bool bCameraForcedDisabled = false;
    bool bPreviousCameraState = false;

    // 3rd person camera overrides
    //FVECTOR gBP_3rdPersonCamera_Target         // Target position that camera looks at
    //FVECTOR gBP_3rdPersonCamera_Eye =      // Eye position where camera is placed
    //SVECTOR gBP_3rdPersonCamera_Rot =      // Rotation around player

    void ReleaseCameraState(bool enabled)
    {
        bCameraForcedDisabled = false;
        *gBP_3rdPersonCamera_Override = enabled;
    }

    void ForceCameraDisabled()
    {
        if (!bCameraForcedDisabled)
        {
            //spdlog::info("MGS2: Third Person Freecam: Forcing camera disabled. GM_Weapon value: {}, Get_GM_GameStatus value: {}", MGS2_LinkVarBuf::GM_Weapon.get(), g_GameVars.Get_GM_GameStatus());
            bCameraForcedDisabled = true;
            bPreviousCameraState = *gBP_3rdPersonCamera_Override;
        }
        *gBP_3rdPersonCamera_Override = false;
    }

    void Toggle3rdPersonCamera()
    {
        if (bCameraForcedDisabled)
        {
            return;
        }
        *gBP_3rdPersonCamera_Override = !*gBP_3rdPersonCamera_Override;
    }

    void IncreaseCameraDistance()
    {
        if (bCameraForcedDisabled) //don't increase while d-padding in menus and shit
        {
            return;
        }
        *gBP_3rdPersonCamera_Dist = std::min(*gBP_3rdPersonCamera_Dist + MGS2_ThirdPersonFreecam::iCameraDistanceStep, k3rdPersonMaxCameraDistance);
    }

    void DecreaseCameraDistance()
    {
        if (bCameraForcedDisabled) //don't decrease while d-padding in menus and shit
        {
            return;
        }
        *gBP_3rdPersonCamera_Dist = std::max(*gBP_3rdPersonCamera_Dist - MGS2_ThirdPersonFreecam::iCameraDistanceStep, k3rdPersonMinCameraDistance);
    }

    void ResetCameraDistance()
    {
        if (bCameraForcedDisabled)
        {
            return;
        }
        *gBP_3rdPersonCamera_Dist = MGS2_ThirdPersonFreecam::iMax_Camera_Distance;
        //spdlog::info("MGS2_LinkVarBuf::GM_PlayerPosX value: {}, MGS2_LinkVarBuf::GM_PlayerPosY value: {}, MGS2_LinkVarBuf::GM_PlayerPosZ value: {}", MGS2_LinkVarBuf::GM_PlayerPosX.get(), MGS2_LinkVarBuf::GM_PlayerPosY.get(), MGS2_LinkVarBuf::GM_PlayerPosZ.get());
    }

    /* v2
    safetyhook::InlineHook g_CheckBehindCamera_hook;
    safetyhook::InlineHook g_GM_ChangeCamera_hook;

    bool g_suppressBehindCameraChange = false;

    void __fastcall GM_ChangeCamera_hooked(int chanl) //straight returning CheckBehindCamera results in jumpout shots breaking
    {
        if (g_suppressBehindCameraChange)
        {
            return;
        }
        g_GM_ChangeCamera_hook.call<void>(chanl);
    }

    __int64 __fastcall CheckBehindCamera_hooked(__int64 a1) //fix wall hugging breaking third person cam
    {
        if (*gBP_3rdPersonCamera_Override)
        {
            g_suppressBehindCameraChange = true;
        }
        __int64 result = g_CheckBehindCamera_hook.call<__int64>(a1);
        g_suppressBehindCameraChange = false;
        return result;
    }
    */

    // v3 - known issue: hug wall -> enter first person -> exit first person -> wall camera angle activates -> exit wall hug -> need to tap first person to reset camera angle.
    safetyhook::InlineHook g_GM_ChangeCamera_hook;
    safetyhook::InlineHook g_GM_SetCameraInterpMode_hook;
    safetyhook::InlineHook g_PL_LeaveSubject_hook;
    int g_leavingSubjectFrames = 0;

    bool ShouldSuppressCameraChange()
    {
        if (g_GameVars.InCutscene() || g_GameVars.InScriptedSequence())
        {
            return false;
        }
        return *gBP_3rdPersonCamera_Override && (g_leavingSubjectFrames <= 0) &&
            (!(g_GameVars.Get_PL_Status() & PLAYER_WATCH) &&
             (g_GameVars.Get_PL_Status() & (PLAYER_CAUTION | PLAYER_BEHIND)));
    }

    void __fastcall GM_ChangeCamera_hooked(int chanl)
    {
        if (ShouldSuppressCameraChange()) return;
        g_GM_ChangeCamera_hook.call<void>(chanl);
    }

    void __fastcall GM_SetCameraInterpMode_hooked(void* cam, int in, int out, int a, int b)
    {
        if (ShouldSuppressCameraChange()) return;
        g_GM_SetCameraInterpMode_hook.call<void>(cam, in, out, a, b);
    }

    void __fastcall PL_LeaveSubject_hooked(__int64 a1)
    {
        g_leavingSubjectFrames = 30;
        g_PL_LeaveSubject_hook.call<void>(a1);
    }

    bool isW45a = false;
    bool isMainGameOrAlternate = false;
}

void MGS2_ThirdPersonFreecam::Tick()
{
    if (!bEnabled)
    {
        return;
    }
    if (gBP_3rdPersonCamera_Override == nullptr)
    {
        return;
    }

    if (g_leavingSubjectFrames > 0)
        g_leavingSubjectFrames--;

    if (g_GameVars.InCutscene() || g_GameVars.InScriptedSequence())
    {
        ForceCameraDisabled();
        return;
    }

    if (!isMainGameOrAlternate)
    {
        ForceCameraDisabled();
        return;
    }

    //if (Get_PL_Status() & (PLAYER_CAUTION|STATE_CUT_IN))

    if (MGS2_LinkVarBuf::GM_Weapon == MGS2_WEAPON_INDEX_HIGH_FREQUENCY_BLADE || MGS2_LinkVarBuf::GM_Weapon == MGS2_WEAPON_INDEX_COOLANT)
    {
        ForceCameraDisabled();
        return;
    }


    if (isW45a)
    {
        const int playerPosX = MGS2_LinkVarBuf::GM_PlayerPosX;
        const int playerPosZ = MGS2_LinkVarBuf::GM_PlayerPosZ;
        if (playerPosX >= 1358 && playerPosX <= 3000 && playerPosZ >= -137100) //doorway to the room. freecam clips through the geometry pretty heavy when you enter.
        {
            ForceCameraDisabled();
            return;
        }
    }
    if (bCameraForcedDisabled)
    {
        ReleaseCameraState(bPreviousCameraState);
        return;
    }


}


void MGS2_ThirdPersonFreecam::HandleLevelTransition()
{
    if (!bEnabled)
    {
        return;
    }
    if (gBP_3rdPersonCamera_Override == nullptr)
    {
        return;
    }

    isMainGameOrAlternate = ((g_GameVars.MGS2_GetGameMode() == MGS2GameMode::Plant) || (g_GameVars.MGS2_GetGameMode() == MGS2GameMode::Tanker) || (g_GameVars.MGS2_GetGameMode() == MGS2GameMode::Alternate));

    isW45a = (g_GameVars.IsStage(MGS2Stages::W45A) || g_GameVars.IsStage(MGS2Stages::A45A));

}

void MGS2_ThirdPersonFreecam::Activate()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS2: Third Person Freecam: Config disabled, skipping.");
        return;
    }

    const auto NewCamera_Act_sub_14006BC70  = Memory::PatternScan(baseModule,"83 3D ?? ?? ?? ?? 00 0F 28 74 24","MGS2: Third Person Freecam: gBP_3rdPersonCamera_Override");
    if (NewCamera_Act_sub_14006BC70 == nullptr)
    {
        spdlog::error("MGS2: Third Person Freecam: Failed to find g_BP3rdPersonCameraOverride.");
        return;
    }

    gBP_3rdPersonCamera_Override = reinterpret_cast<std::int32_t*>(Memory::GetRipRelativeAddress(NewCamera_Act_sub_14006BC70, 2, 7));

    Toggle3rdPersonCamera();
    if (!*gBP_3rdPersonCamera_Override)
    {
        spdlog::error("MGS2: Third Person Freecam: Failed to enable third person camera.");
        return;
    }
    g_InputHandler.RegisterHotkey(vkToggle_Camera, "Third Person Camera Toggle", []()
                                  {
                                      Toggle3rdPersonCamera();
                                  });

    if (fHorizontal_Sensitivity != k3rdPersonFreecamDefaultHorizontalSensitivity)
    {
        if (const auto gBP_3rdPersonCamera_HSpeed = reinterpret_cast<float*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "F3 0F 59 05 ?? ?? ?? ?? F3 0F 2C F8 66 29 3D", "MGS 2: Third Person Freecam: gBP_3rdPersonCamera_HSpeed") + 4)); gBP_3rdPersonCamera_HSpeed != nullptr)
        {
            *gBP_3rdPersonCamera_HSpeed = fHorizontal_Sensitivity;
            spdlog::info("MGS2: Third Person Freecam: Set horizontal sensitivity to {}", fHorizontal_Sensitivity);
        }
    }
    if (fVertical_Sensitivity != k3rdPersonFreecamDefaultVerticalSensitivity)
    {
        if (const auto gBP_3rdPersonCamera_VSpeed = reinterpret_cast<float*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "F3 0F 59 05 ?? ?? ?? ?? F3 0F 2C C0 EB ?? 8B C7", "MGS 2: Third Person Freecam: gBP_3rdPersonCamera_VSpeed") + 4)); gBP_3rdPersonCamera_VSpeed != nullptr)
        {
            *gBP_3rdPersonCamera_VSpeed = fVertical_Sensitivity;
            spdlog::info("MGS2: Third Person Freecam: Set vertical sensitivity to {}", fVertical_Sensitivity);
        }
    }

    gBP_3rdPersonCamera_Dist = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "4C 8D 0D ?? ?? ?? ?? F3 0F 11 05", "MGS 2: Third Person Freecam: gBP_3rdPersonCamera_Dist") + 3));
    if (gBP_3rdPersonCamera_Dist != nullptr)
    {
        if (iMax_Camera_Distance != k3rdPersonFreecamDefaultMaxCameraDistance)
        {
            ResetCameraDistance();
        }
        
        g_InputHandler.RegisterHeldHotkey(vkToggle_Increase_Camera_Distance, "Third Person Camera - Increase Distance", []()
                                      {
                                          IncreaseCameraDistance();
                                      }, iCameraDistanceChangeSpeed);
        g_InputHandler.RegisterHeldHotkey(vkToggle_Decrease_Camera_Distance, "Third Person Camera - Decrease Distance", []()
                                      {
                                          DecreaseCameraDistance();
                                      }, iCameraDistanceChangeSpeed);
                                      
        g_InputHandler.RegisterHotkey(vkToggle_Reset_Camera_Distance, "Third Person Camera - Reset Distance", []()
                                      {
                                          ResetCameraDistance();
                                      });


    }


    
    if (const auto PL_IntoSubject = Memory::PatternScan(baseModule, "83 3D ?? ?? ?? ?? 00", "MGS2: Third Person Freecam: gBP_Camera_InheritRot"); PL_IntoSubject != nullptr)
    {
        gBP_Camera_InheritRot = reinterpret_cast<int*>(Memory::GetRelativeOffset(PL_IntoSubject + 2));
        *gBP_Camera_InheritRot = bInherit_Camera_Rotation;
        spdlog::info("MGS2: Third Person Freecam: Set inherit camera rotation to {}", *gBP_Camera_InheritRot ? "true" : "false");
        g_InputHandler.RegisterHotkey(vkToggle_Inherit_Camera_Rotation, "Third Person Camera - Inherit Rotation Toggle", []()
                                      {
                                          if (gBP_Camera_InheritRot != nullptr)
                                          {
                                              *gBP_Camera_InheritRot = !*gBP_Camera_InheritRot;
                                              //spdlog::info("MGS2: Third Person Freecam: Toggled inherit camera rotation to {}", *gBP_Camera_InheritRot);
                                          }
                                      });
    }

    //g_CheckBehindCamera_hook = safetyhook::create_inline( reinterpret_cast<void*>(Memory::PatternScan(baseModule, "40 55 53 57 41 56 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 ?? 4C 8B F1", "MGS 2: Third Person Freecam: CheckBehindCamera")),reinterpret_cast<void*>(CheckBehindCamera_hooked));
    g_GM_ChangeCamera_hook = safetyhook::create_inline(reinterpret_cast<void*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 B8", "MGS 2: Third Person Freecam: GM_ChangeCamera")+1)), reinterpret_cast<void*>(GM_ChangeCamera_hooked));
    g_GM_SetCameraInterpMode_hook = safetyhook::create_inline(reinterpret_cast<void*>(Memory::PatternScan(baseModule, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC ?? 33 FF 4C 8D 35", "MGS 2: Third Person Freecam: GM_SetCameraInterpMode")), reinterpret_cast<void*>(GM_SetCameraInterpMode_hooked));
    g_PL_LeaveSubject_hook = safetyhook::create_inline(reinterpret_cast<void*>(Memory::PatternScan(baseModule, "40 53 48 83 EC ?? 83 3D ?? ?? ?? ?? 00 48 8B D9 74 ?? 0F BF 89", "MGS 2: Third Person Freecam: PL_LeaveSubject")), reinterpret_cast<void*>(PL_LeaveSubject_hooked));
}

