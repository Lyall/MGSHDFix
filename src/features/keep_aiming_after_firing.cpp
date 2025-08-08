#include "common.hpp"
#include "keep_aiming_after_firing.hpp"

#include "gamevars.hpp"
#include "logging.hpp"
#include "steamworks_api.hpp"

/// Originally made by Zenf as part of the Keep Aiming mod for MGS3.


void KeepAimingAfterFiring::Initialize()
{
    if (!(eGameType & (MGS2|MGS3)) || !(g_KeepAimingAfterFiring.bAlwaysKeepAiming || g_KeepAimingAfterFiring.bKeepAimingInFirstPerson || g_KeepAimingAfterFiring.bKeepAimingOnLockOn))
    {
        return;
    }

    if (eGameType & MGS2)
    {
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
            if (g_KeepAimingAfterFiring.bKeepAimingInFirstPerson && g_GameVars.MGS2IsHoldingFirstPerson())
            {
                ctx.r12 = g_GameVars.GetAimingState();
                return;
            }
            if (g_KeepAimingAfterFiring.bKeepAimingOnLockOn && g_GameVars.MGS2IsHoldingLockOn())
            {
                ctx.r12 = g_GameVars.GetAimingState();
                return;
            }
        });
            MAKE_HOOK_MID(baseModule, "66 44 89 B8 ?? ?? ?? ?? 8B 15", "crouch fix", {
                if (ctx.r15 == 2 && ctx.rdx == 0xDC)
                {
                    ctx.r15 = 1;
                    spdlog::info("MGS 2: Keep Aiming After Firing - forced");
                }
            });
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
            if (g_KeepAimingAfterFiring.bAlwaysKeepAiming)
            {
                ctx.rbx = g_GameVars.GetAimingState();
                return;
            }
            if (g_KeepAimingAfterFiring.bKeepAimingInFirstPerson & g_GameVars.MGS3IsHoldingFirstPerson())
            {
                ctx.rbx = g_GameVars.GetAimingState();
                return;
            }
            if (g_KeepAimingAfterFiring.bKeepAimingOnLockOn & g_GameVars.MGS3IsHoldingLockOn())
            {
                ctx.rbx = g_GameVars.GetAimingState();
                return;
            }
        });
    }
}


