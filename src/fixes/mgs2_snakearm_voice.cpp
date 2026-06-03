#include "stdafx.h"

#include "mgs2_snakearm_voice.hpp"

#include "common.hpp"
#include "custom_resolution_and_borderless.hpp"
#include "logging.hpp"

namespace
{
    uintptr_t* SolControl = NULL;
}

void SnakeArmFixes::ApplyFixes() {
    if (!(eGameType & MGS2))
    {
        return;
    }

    // user/sonoyama/solidus/init_sol.c -> InitControl
    // Get SOL_SolControl (unsure how to properly optimize the pattern, more context attached)
    //   4B 8B CD 45 8D 41 EC E8 ?? ?? ?? ?? 44 8B 8D D8 00 00 00 
    //   41 B8 0F 00 00 00 8B 8D C4 00 00 00 48 8B D5 E8 ?? ?? ?? ?? A8 01 74 07 E8 ?? ?? ?? ?? 
    uint8_t* Solidus_InitControl = Memory::PatternScan(baseModule, "EB ?? F3 0F 10 47 ?? F3 0F 10 0D ?? ?? ?? ?? 0F 28 D0", "Solidus Voice Position");
    SolControl = (uintptr_t*)Memory::GetRelativeOffset(Solidus_InitControl + 31);

    // user/sonoyama/solidus/snakearm.c -> ClawAttackHit - uses GM_PlayerPosition instead of SOL_SolControl->mov
    // Same for OnlineHit
    // This is probably not the correct way to change an offset from one global to another.
    uint8_t* ClawAttackHit = Memory::PatternScan(baseModule, "2B C8 48 8D 15 ?? ?? ?? ?? 8B C1 48 8D 0D ?? ?? ?? ?? ?? ?? ?? 48 8B 5C 24", "Solidus Voice Position Fix");
    uint8_t* OnlineHit = Memory::PatternScan(baseModule, "2B C8 48 8D 15 ?? ?? ?? ?? 8B C1 48 8D 0D ?? ?? ?? ?? ?? ?? ?? E8", "Solidus Voice Position Fix 2");

    if (Solidus_InitControl && ClawAttackHit && OnlineHit) {
        // int32_t SolControl_Relative = (int32_t)(/*(long)&SolControl*/ (uintptr_t)SolControl + 0x40 - (long)ClawAttackHit - 9);
        // Memory::Write((uintptr_t)(ClawAttackHit + 5), SolControl_Relative);
        // SPDLOG_INFO("Solidus Voice Position: Write value {}", SolControl_Relative);
        
        Memory::PatchBytes((uintptr_t)ClawAttackHit + 2, "\x90\x90\x90\x90\x90\x90\x90", 7);
        Memory::PatchBytes((uintptr_t)OnlineHit + 2, "\x90\x90\x90\x90\x90\x90\x90", 7);
        
        {
            static SafetyHookMid ClawAttackHook{};
            ClawAttackHook = safetyhook::create_mid(ClawAttackHit + 2, [](SafetyHookContext& ctx) { {
                    ctx.rdx = *SolControl;
                } });

            static SafetyHookMid OnlineHitHook{};
            OnlineHitHook = safetyhook::create_mid(OnlineHit + 2, [](SafetyHookContext& ctx) { {
                    ctx.rdx = *SolControl;
                } });

            if (ClawAttackHook && OnlineHitHook) {
                if (g_Logging.bVerboseLogging) {
                    spdlog::info("Solidus Voice Position: Hooks installed.");
                }
            }
            else if (ClawAttackHook) {
                spdlog::error("Solidus Voice Position: Hook 2 failed.");
            }
            else if (OnlineHitHook) {
                spdlog::error("Solidus Voice Position: Hook 1 failed.");
            }
            else {
                spdlog::error("Solidus Voice Position: Hooks failed.");
            }
            
        }
    }
    else {
        spdlog::error("Solidus Voice Position: Failed to match one or more hook locations.");
    }
}
