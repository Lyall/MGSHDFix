#include "stdafx.h"
#include "config_keys.hpp"

#include "mgs2_3rd_person_freecam.hpp"
#include "common.hpp"
#include "input_handler.hpp"
#include "logging.hpp"

namespace
{
    //TODO: 
    //      - force camera to isometric when hf blade is equipped
    //      - force camera to isometric on initial entrance of w45a (or figure out how forced angles are determined and force while in the doorway. hzx determined perhaps?)
    //      - disable camera angles when leaning against walls
    //      - block camera increase / decrease when in menus

    std::int32_t* gBP_3rdPersonCamera_Override = nullptr;

    int* gBP_3rdPersonCamera_Dist = nullptr;         // max camera distance from player

    // 3rd person camera overrides
    //FVECTOR gBP_3rdPersonCamera_Target         // Target position that camera looks at
    //FVECTOR gBP_3rdPersonCamera_Eye =      // Eye position where camera is placed
    //SVECTOR gBP_3rdPersonCamera_Rot =      // Rotation around player


    void Toggle3rdPersonCamera()
    {
        *gBP_3rdPersonCamera_Override = !*gBP_3rdPersonCamera_Override;
    }

    void IncreaseCameraDistance()
    {
        *gBP_3rdPersonCamera_Dist = std::min(*gBP_3rdPersonCamera_Dist + MGS2_ThirdPersonFreecam::iCameraDistanceStep, k3rdPersonMaxCameraDistance);
        spdlog::info("MGS2: Third Person Freecam: Increased camera distance to {}", *gBP_3rdPersonCamera_Dist);
    
    }

    void DecreaseCameraDistance()
    {
        *gBP_3rdPersonCamera_Dist = std::max(*gBP_3rdPersonCamera_Dist - MGS2_ThirdPersonFreecam::iCameraDistanceStep, k3rdPersonMinCameraDistance);

        spdlog::info("MGS2: Third Person Freecam: Decreased camera distance to {}", *gBP_3rdPersonCamera_Dist);
    }

    void ResetCameraDistance()
    {
        *gBP_3rdPersonCamera_Dist = MGS2_ThirdPersonFreecam::iMax_Camera_Distance;
        spdlog::info("MGS2: Third Person Freecam: Reset camera distance to {}", *gBP_3rdPersonCamera_Dist);
    }

}

void MGS2_ThirdPersonFreecam::HandleLevelTransition()
{
    //todo -> handling for some levels with small entrances where the freecam clips, like w45a
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



}

