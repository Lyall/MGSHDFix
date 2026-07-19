#include "stdafx.h"

#include "common.hpp"
#include "keep_aiming_after_firing.hpp"

#include "gamevars.hpp"
#include "logging.hpp"
#include "mgs2_first_person_view_mode.hpp"

/// Originally made by Zenf as part of the Keep Aiming mod for MGS3.

namespace
{
    bool disabled = false;
    bool is_first_person_vr = false;
}

void KeepAimingAfterFiring::HandleLevelTransition()
{
    if (disabled)
    {
        return;
    }
    if (!(eGameType & MGS2))
    {
        return;
    }
    is_first_person_vr = (g_GameVars.MGS2_GetGameMode() == MGS2GameMode::VRFirstPerson);
}

void KeepAimingAfterFiring::Initialize()
{
    if (!(eGameType & (MGS2|MGS3)))
    {
        return;
    }

    if (!(g_KeepAimingAfterFiring.bAlwaysKeepAiming || g_KeepAimingAfterFiring.bKeepAimingInFirstPerson || g_KeepAimingAfterFiring.bKeepAimingOnLockOn || g_KeepAimingAfterFiring.bKeepAimingInFPSMode))
    {
        disabled = true;
        return;
    }

    if (eGameType & MGS2)
    {
        using namespace MGS2_StatusFlags;

        MAKE_HOOK_MID(baseModule, "4C 89 25 ?? ?? ?? ?? EB", "MGS 2: Keep Aiming After Firing", {
            switch (ctx.rsi)
            {
                case MGS2_WEAPON_INDEX_AKS74U:
                case MGS2_WEAPON_INDEX_M4:
                    break;
                default:
                    return;
            }
            if (g_KeepAimingAfterFiring.bAlwaysKeepAiming)
            {
                ctx.r12 = g_GameVars.GetAimingState();
                return;
            }
            //spdlog::info("MGS 2: Keep Aiming After Firing: is_first_person_vr {}, bKeepAimingInFirstPerson {}, PL_Status {:X}", is_first_person_vr, g_KeepAimingAfterFiring.bKeepAimingInFirstPerson, g_GameVars.Get_PL_Status());
            if (g_KeepAimingAfterFiring.bKeepAimingInFirstPerson && (g_GameVars.Get_PL_Status() & (PLAYER_INTRUDE | PLAYER_WATCH)))
            {
                ctx.r12 = g_GameVars.GetAimingState();
                return;
            }
            if (g_KeepAimingAfterFiring.bKeepAimingInFPSMode && (MGS2_First_Person_View::IsActive() || is_first_person_vr))
            {
                ctx.r12 = g_GameVars.GetAimingState();
                //spdlog::info("MGS 2: Keep Aiming After Firing: keeping aiming in FPS mode");
                return;
            }
            if (g_KeepAimingAfterFiring.bKeepAimingOnLockOn && g_GameVars.MGS2IsHoldingLockOn())
            {
                ctx.r12 = g_GameVars.GetAimingState();
                return;
            }
        });
        /*
            MAKE_HOOK_MID(baseModule, "66 44 89 B8 ?? ?? ?? ?? 8B 15", "crouch fix", {
                if (ctx.r15 == 2 && ctx.rdx == 0xDC)
                {
                    ctx.r15 = 1;
                    spdlog::info("MGS 2: Keep Aiming After Firing - forced");
                }
            });
            */
    }
    else if (eGameType & MGS3)
    {
        MAKE_HOOK_MID(baseModule, "48 89 1D ?? ?? ?? ?? 4C 8D 15", "MGS 3: Keep Aiming After Firing", {
            switch (ctx.r9)
            {
                case MGS3_WEAPON_INDEX_MK22:
                case MGS3_WEAPON_INDEX_M1911A1:
                case MGS3_WEAPON_INDEX_EzGun:
                case MGS3_WEAPON_INDEX_SAA:
                case MGS3_WEAPON_INDEX_Patriot:
                case MGS3_WEAPON_INDEX_Scorpion:
                case MGS3_WEAPON_INDEX_XM16E1:
                case MGS3_WEAPON_INDEX_AK47:
                case MGS3_WEAPON_INDEX_M63:
                case MGS3_WEAPON_INDEX_M37:
                    break;
                default:
                    return;
            }
            if (g_KeepAimingAfterFiring.bAlwaysKeepAiming){
                ctx.rbx = g_GameVars.GetAimingState();
                g_KeepAimingAfterFiring.bOverrodeState = true;
                return;
            }
            if (g_KeepAimingAfterFiring.bKeepAimingInFirstPerson && g_GameVars.MGS3IsHoldingFirstPerson())
            {
                ctx.rbx = g_GameVars.GetAimingState();
                return;
            }
            if (g_KeepAimingAfterFiring.bKeepAimingOnLockOn && g_GameVars.MGS3IsHoldingLockOn())
            {
                ctx.rbx = g_GameVars.GetAimingState();
                return;
            }
            });

        if (g_KeepAimingAfterFiring.bAlwaysKeepAiming)
        {
            MAKE_HOOK_MID(baseModule, "4C 8D 15 ?? ?? ?? ?? 4C 8B A4 24", "MGS 3: Keep Aiming After Firing 2", {
                if (!g_KeepAimingAfterFiring.bOverrodeState)
                {
                    return;
                }
                ctx.rbx = 0LL;
                g_KeepAimingAfterFiring.bOverrodeState = false;
                });
        }
    }
}


