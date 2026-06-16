#include "stdafx.h"
#include "mgs2_first_person_view_mode.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "input_handler.hpp"
#include "logging.hpp"

namespace
{
    bool skip_tick = false; ///skip processing during tick if everything's disabled.

    int* gBP_1stPersonCamera_Override = nullptr;             // enable/disable
    int* gBP_1stPersonCamera_Move = nullptr;             // is fpv movement enabled
    int* gBP_1stPersonCamera_Hold_Toggle = nullptr;             // is first person toggled on/off
    int* gBP_1stPersonCamera_Active = nullptr;             // currently in 3rd or 1st person. (cutscene detection?)

    /*

        todo:
            keep aiming after firing integration   


            Refactor speed fx for egametype & mgs3
            MGS3 rain color fix
            Scope zoom in speed
            Fix ledge jump forcing camera
            real character names / custom character name
            laser origin
        
        */

    bool bCameraForcedDisabled = false;
    bool bPreviousOverrideState = false;

    void ForceCameraDisabled()
    {
        if (!bCameraForcedDisabled)
        {
            bCameraForcedDisabled = true;
            bPreviousOverrideState = *gBP_1stPersonCamera_Override;
            //spdlog::info("MGS2: First Person View Mode: Forcing camera disabled. Override: {}, Hold Toggle: {}, Movement: {}", bPreviousOverrideState, bPreviousHoldToggleState, bPreviousMoveState);
        }
        *gBP_1stPersonCamera_Override = false;
    }

    void ReleaseCameraState()
    {
        bCameraForcedDisabled = false;
        *gBP_1stPersonCamera_Override = bPreviousOverrideState;
        //spdlog::info("MGS2: First Person View Mode: Camera state released. Override: {}, Hold Toggle: {}, Movement: {}", *gBP_1stPersonCamera_Override, *gBP_1stPersonCamera_Hold_Toggle, *gBP_1stPersonCamera_Move);
    }

    bool bLevelForcedOff = false;
}

bool MGS2_First_Person_View::IsActive()
{
    if (!bFirst_Person_View_Enabled)
    {
        return false;
    }
    if (gBP_1stPersonCamera_Override == nullptr)
    {
        return false;
    }
    //spdlog::info("did the thing");
    return *gBP_1stPersonCamera_Active;
}

void MGS2_First_Person_View::HandleLevelTransition()
{
    if (!bFirst_Person_View_Enabled)
    {
        return;
    }
    if (gBP_1stPersonCamera_Override == nullptr)
    {
        return;
    }
    if(g_GameVars.MGS2_GetGameMode() == MGS2GameMode::VRFirstPerson)
    {
        bLevelForcedOff = true;
        ForceCameraDisabled();
        return;
    }
    
    bLevelForcedOff = false;
}


void MGS2_First_Person_View::Tick()
{
    if (skip_tick)
    {
        return;
    }
    if (gBP_1stPersonCamera_Override == nullptr)
    {
        return;
    }
    if (bLevelForcedOff)
    {
        ForceCameraDisabled();
        return;
    }
    if (g_GameVars.InScriptedSequence()) //pad demos break when movement & hold toggle are enabled for obvious reasons.
    {
        ForceCameraDisabled();
        return;
    }

    auto current_weapon = MGS2_LinkVarBuf::GM_Weapon;
    if (current_weapon == MGS2_WEAPON_INDEX_STINGER || current_weapon == MGS2_WEAPON_INDEX_D_MIC)
    {
        ForceCameraDisabled();
        return;
    }

    if (bCameraForcedDisabled)
    {
        ReleaseCameraState();
        return;
    }

}


void MGS2_First_Person_View::Activate()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    skip_tick = (!bFirst_Person_View_Enabled && !bFirst_Person_View_Toggle_Hold);

    if (bFirst_Person_View_Toggle_Hold)
    {
        //spdlog::info("MGS 2: First Person View Mode: Toggle Hold enabled, applying hotkey.");
        const auto gBP_1stPersonCamera_Toggle_scan = Memory::PatternScan(baseModule, "8B 05 ?? ?? ?? ?? 89 05 ?? ?? ?? ?? 48 8B 97", "MGS2: First Person View Mode: gBP_1stPersonCamera_Hold_Toggle");
        if (!gBP_1stPersonCamera_Toggle_scan)
        {
            spdlog::error("MGS 2: First Person View Mode: gBP_1stPersonCamera_Hold_Toggle not found, skipping.");
        }
        else
        {
            spdlog::info("MGS 2: First Person View Mode: gBP_1stPersonCamera_Hold_Toggle found, applying hotkey.");
            gBP_1stPersonCamera_Hold_Toggle = reinterpret_cast<int*>(Memory::GetRelativeOffset(gBP_1stPersonCamera_Toggle_scan + 2));
            *gBP_1stPersonCamera_Hold_Toggle = true;
            g_InputHandler.RegisterHotkey(vkToggle_Hold_First_Person_View, "Toggle Hold first person view", []()
                {
                    if (bCameraForcedDisabled)
                    {
                        return;
                    }
                    *gBP_1stPersonCamera_Hold_Toggle = !*gBP_1stPersonCamera_Hold_Toggle;
                    //spdlog::info("MGS 2: First Person View Mode: gBP_1stPersonCamera_Hold_Toggle toggled to {}", *gBP_1stPersonCamera_Hold_Toggle);
                });
            //spdlog::info("MGS 2: First Person View Mode: Toggle Hold hotkey applied.");
        }
    }

    if (!bFirst_Person_View_Enabled)
    {
        spdlog::info("MGS 2: First Person View Mode: Config disabled, skipping.");
        return;
    }

#if !defined(RELEASE_BUILD)
    g_InputHandler.RegisterHotkey(VK_LEFT, "report state", []()
                                  {
                                      spdlog::info("MGS 2: First Person View Mode: Current state - Override: {}, Hold Toggle: {}, Movement: {}, Active: {}", *gBP_1stPersonCamera_Override, *gBP_1stPersonCamera_Hold_Toggle, *gBP_1stPersonCamera_Move, *gBP_1stPersonCamera_Active);
                                  });
#endif

    const auto gBP_1stPersonCamera_Override_scan  = Memory::PatternScan(baseModule, "8B 15 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 85 D2", "MGS2: First Person View Mode: gBP_1stPersonCamera_Override");
    if (!gBP_1stPersonCamera_Override_scan)
    {
        spdlog::error("MGS 2: First Person View Mode: gBP_1stPersonCamera_Override not found.");
        return;
    }
    const auto gBP_1stPersonCamera_Active_scan = Memory::PatternScan(baseModule, "8B 0D ?? ?? ?? ?? 85 D2 0F 85", "MGS2: First Person View Mode: gBP_1stPersonCamera_Active");
    if (!gBP_1stPersonCamera_Active_scan)
    {
        spdlog::error("MGS 2: First Person View Mode: gBP_1stPersonCamera_Active not found.");
        return;
    }

    const auto gBP_1stPersonCamera_Move_scan = Memory::PatternScan(baseModule, "8B 05 ?? ?? ?? ?? 89 05 ?? ?? ?? ?? 85 C0", "MGS2: First Person View Mode: gBP_1stPersonCamera_Move");
    if (!gBP_1stPersonCamera_Move_scan)
    {
        spdlog::error("MGS 2: First Person View Mode: gBP_1stPersonCamera_Move not found.");
        return;
    }

    gBP_1stPersonCamera_Override = reinterpret_cast<int*>(Memory::GetRelativeOffset(gBP_1stPersonCamera_Override_scan + 2));
    *gBP_1stPersonCamera_Override = true;
    g_InputHandler.RegisterHotkey(vkToggle_First_Person_Override, "Toggle override", []()
    {
        *gBP_1stPersonCamera_Override = !*gBP_1stPersonCamera_Override;
        //spdlog::info("MGS 2: First Person View Mode: gBP_1stPersonCamera_Override toggled to {}", *gBP_1stPersonCamera_Override);
    });



    gBP_1stPersonCamera_Move     = reinterpret_cast<int*>(Memory::GetRelativeOffset(gBP_1stPersonCamera_Move_scan + 2));
    if (bFirst_Person_View_Movement_Enabled_By_Default)
    {
        *gBP_1stPersonCamera_Move = true;
    }
    g_InputHandler.RegisterHotkey(vkToggle_First_Person_View_Movement, "Toggle first person view movement", []()
    {
        if (bCameraForcedDisabled)
        {
            return;
        }
        *gBP_1stPersonCamera_Move = !*gBP_1stPersonCamera_Move;
        //spdlog::info("MGS 2: First Person View Mode: gBP_1stPersonCamera_Move toggled to {}", *gBP_1stPersonCamera_Move);
    });

    gBP_1stPersonCamera_Active = reinterpret_cast<int*>(Memory::GetRelativeOffset(gBP_1stPersonCamera_Active_scan + 2));



    MAKE_HOOK_MID(baseModule, "0F 84 ?? ?? ?? ?? 33 C9 E8 ?? ?? ?? ?? 39 6F", "NewDivingGoggles -> act()", {
        if (*gBP_1stPersonCamera_Active)
        {
            reghelpers::SetZF(ctx, false);
        }
        });

    MAKE_HOOK_MID(baseModule, "F3 0F 11 2D ?? ?? ?? ?? F7 83", "sonoyama\\raiden\\routine.c -> CheckSubjectMoveCamera() @ l933/937: Fix FPV aimpoint height", {
        if (*gBP_1stPersonCamera_Active)
        {
            auto bodyObjs = *reinterpret_cast<uintptr_t*>(ctx.rbx + 0x290); // normally calcs camera height as a fixed 1500/1000 offset from the bottom of the player's hitbox (hzx_base). 
            ctx.xmm5.f32[0] = *reinterpret_cast<float*>(bodyObjs + 0x1344);         // swap it to use player's head bone y instead, matching pl_status & PLAYER_WATCH behavior.
        }
            });

    /*
    MAKE_HOOK_MID(baseModule, "75 ?? 48 BA ?? ?? ?? ?? ?? ?? ?? ?? 49 8B CE", "mgs2x\\source\\user\\sonoyama\\raiden\\attack.c -> Combo() | +666", {
        if (*gBP_1stPersonCamera_Active)
        {
            reghelpers::SetZF(ctx, false);
        }
        });

    MAKE_HOOK_MID(baseModule, "0F 85 ?? ?? ?? ?? 48 BA ?? ?? ?? ?? ?? ?? ?? ?? 49 8B CE", "mgs2x\\source\\user\\sonoyama\\raiden\\attack.c -> Combo() | +761", {
    if (*gBP_1stPersonCamera_Active)
    {
        reghelpers::SetZF(ctx, false);
    }
                  });
                  */

}
