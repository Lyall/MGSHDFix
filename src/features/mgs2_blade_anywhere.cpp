#include "stdafx.h"
#include "mgs2_blade_anywhere.hpp"

#include "common.hpp"
#include "logging.hpp"
#include "gamevars.hpp"
#include "expand_bp_assets.hpp"
#include "mgs2_linkvarbuf.hpp"


namespace
{
    // The ctor reads -m as the motion archive's strcode.
    constexpr int kRaiBladeMotion = GameVars::GV_StrCode("rai_blade");
    constexpr int kWpBlade = 13;   // g_define.h WP_Blade
    constexpr int kWpNone = 0;     // g_define.h WP_None

    using NewPluginBladeFn = int (*)();
    NewPluginBladeFn g_newPluginBlade = nullptr;
    int* g_bladeMngAlive = nullptr;
    int* g_pullOutFlag = nullptr;
    int* g_visFlag = nullptr;
    void** g_checkAttackFunc = nullptr;
    void* g_bladeCheckAttack = nullptr;
    safetyhook::InlineHook g_setWeapon_hook;

    using PL_ChangeMotionArcFn = void(__fastcall*)(uintptr_t work, int arc);
    PL_ChangeMotionArcFn g_PL_ChangeMotionArc = nullptr;
    ptrdiff_t g_curMar = 0;
    ptrdiff_t g_orgMotion = 0;

    // raiden\raiden.h PlayerWork: the two fields event.c SetWeapon() commits.
    constexpr ptrdiff_t kPlWeapon = 0xB90;
    constexpr ptrdiff_t kPlWpAct = 0xB98;

    // Everything NewPluginBlade() reads out of GM_PlayerStateFlag, and nothing else reads it.
    constexpr short kBladeLatch = MGS2_LinkVarBuf::PL_START_STATE_BLADE_OUT
        | MGS2_LinkVarBuf::PL_START_STATE_BLADE_INV | MGS2_LinkVarBuf::PL_BLADE_MODE_MINEUCHI;

    // raiden\motion.h act names, as GV_StrCode hashes.
    constexpr uint64_t kStandStill = GameVars::GV_StrCode("StandStill");
    constexpr uint64_t kSquatStill = GameVars::GV_StrCode("SquatStill");
    constexpr uint64_t kStandRun = GameVars::GV_StrCode("StandRun");

    // GCL bytecode for `-m <int>`: an option block then the end tag (libgcl/parse.c).
    constexpr uint8_t kLine[] = {
        0x50 | 0x0E, 0x06, 0x00, 'm',
        0x09,
        static_cast<uint8_t>(kRaiBladeMotion & 0xFF),
        static_cast<uint8_t>((kRaiBladeMotion >> 8) & 0xFF),
        static_cast<uint8_t>((kRaiBladeMotion >> 16) & 0xFF),
        0x00,
        0x00,   // GCL_END
    };

    char g_stage[16] = {};

    // g_define.h: the blade's three ways of hitting, and the two Fatman already answers to.
    constexpr uint64_t kWpCuts = (1ull << 13) | (1ull << 25);   // WP_BLADE, WP_BLADESTAB
    constexpr uint64_t kWpStuns = 1ull << 24;                   // WP_BLADEFAINT, the blunt side
    constexpr uint64_t kWpUspBit = 1ull << 2;                   // WP_USP - takes life
    constexpr uint64_t kWpNikitaBit = 1ull << 6;                // WP_NIKITA - Fortune's railgun
    constexpr uint64_t kWpStunBit = 1ull << 11;                 // WP_STUNGRENADE - fells him, no blood
    constexpr uint64_t kVmpStun = 3;   // vamp.h VMP_WPDM_STUN, the kind that owns his stun reaction

    // bladeply.c BladeReverse() keeps this in step with its own InvFlag, so it is live mid-fight.
    bool BladeIsBlunt()
    {
        return (MGS2_LinkVarBuf::GM_PlayerStateFlag & MGS2_LinkVarBuf::PL_BLADE_MODE_MINEUCHI) != 0;
    }

    // Fatman's chain has no blade arm at all, so hand each hit the branch it deserves - the edge takes
    // the bullet's, the flat takes the stun grenade's, which fells him with stars and draws no blood.
    uint64_t FatmanWeapon(uint64_t weapon)
    {
        if (weapon & (kWpCuts | kWpStuns))
        {
            weapon |= BladeIsBlunt() ? kWpStunBit : kWpUspBit;
        }
        return weapon;
    }

    // The blast suit ricochets everything that is not a head hit, so let the edge count as one. The
    // hit position is already resolved by here, so the blood still lands where the blade struck.
    // The stun arm never asks which part it hit, so the flat keeps its own reaction and i-frames.
    constexpr uint64_t kFatHead = 4;   // fatman.h FAT_TRG_CHILD_HEAD
    bool FatmanForceHead(uint64_t weapon)
    {
        return (weapon & (kWpCuts | kWpStuns)) != 0 && !BladeIsBlunt();
    }

    // Once he is down every arm is head-gated and none of them is a blunt one, so deal the faint
    // ourselves rather than borrow the tranq's and leave a dart in him. FAT_GetDamageValue() is
    // inlined at every site as the same pair, so match it: Very Easy 60, otherwise 40.
    constexpr ptrdiff_t kFatFaint = 0x2330;   // fatman.h Work.m9_faint

    bool FatmanBluntHit(uint64_t weapon)
    {
        return (weapon & (kWpCuts | kWpStuns)) != 0 && BladeIsBlunt();
    }

    void FatmanBluntFaint(uintptr_t work)
    {
        if (work == 0) { return; }
        const int dealt = (MGS2_LinkVarBuf::GM_GameLevel
            == static_cast<short>(MGS2_LinkVarBuf::GameLevel::VeryEasy)) ? 60 : 40;
        *reinterpret_cast<int*>(work + kFatFaint) -= dealt;
    }

    std::vector<SafetyHookMid> g_weaponReadHooks;

    // Every pattern opens on the 7-byte TARGET.weapon_type read, so the hook lands just past it. The
    // bodies name their registers, so a match count we did not expect means the code moved under us -
    // hook nothing rather than write the wrong register somewhere.
    void HookWeaponRead(const char* pattern, const char* name, size_t expected,
                        void (*body)(SafetyHookContext&))
    {
        const std::vector<uint8_t*> sites = Memory::FindMultiplePatternMatches(baseModule, pattern);
        if (sites.size() != expected)
        {
            spdlog::error("{}: Pattern scan failed, expected {} site(s), found {}.",
                name, expected, sites.size());
            return;
        }
        for (uint8_t* site : sites)
        {
            g_weaponReadHooks.push_back(safetyhook::create_mid(site + 7, body));
            LOG_HOOK(g_weaponReadHooks.back(), name)
        }
    }

    safetyhook::InlineHook g_makeObjs_hook;

    // libdg.h DG_MakeObjsD(DG_DEF *def, int flag, int chanl, char *fname); fname is the caller's __FILE__.
    void* __fastcall MakeObjs_hooked(void* def, int flag, int chanl, const char* fname)
    {
        if (def == nullptr)
        {
            static int told = 0;
            if (++told <= 20)
            {
                const char* tail = fname ? strrchr(fname, '\\') : nullptr;
                spdlog::error("MGS2: Blade Anywhere: {} has no model on stage {} (flag {:#x}) - skipped.",
                    tail ? tail + 1 : "an effect", g_stage, static_cast<unsigned>(flag));
            }
            return nullptr;
        }
        return g_makeObjs_hook.fastcall<void*>(def, flag, chanl, fname);
    }

    // bladeply.c reads its motion archive off the live GCL line, so lend it one. The line stack and
    // the parse cursor both belong to the game thread and this runs inside its call.
    void ArmBlade()
    {
        static char* s_frame[2] = {};
        char** const savedLine = *g_GameVars.GCL_CommandLine();
        char* const savedNext = *g_GameVars.GCL_NextStrPtr();
        s_frame[0] = reinterpret_cast<char*>(const_cast<uint8_t*>(kLine));
        *g_GameVars.GCL_CommandLine() = &s_frame[1];
        g_newPluginBlade();   // self-guarded: a stage that already ran it is a no-op
        *g_GameVars.GCL_CommandLine() = savedLine;
        *g_GameVars.GCL_NextStrPtr() = savedNext;
    }

    // Hand the pick back as bare hands; CheckChangeWeapon() finishes the swap by itself next tick.
    void SurrenderBlade()
    {
        MGS2_LinkVarBuf::GM_Weapon = kWpNone;
        *g_pullOutFlag = 0;
        *g_visFlag = 0;
        if (*g_checkAttackFunc == g_bladeCheckAttack)
        {
            *g_checkAttackFunc = nullptr;
        }
        short& state = MGS2_LinkVarBuf::GM_PlayerStateFlag.get();
        state = static_cast<short>(state & ~kBladeLatch);
        spdlog::warn("MGS2: Blade Anywhere: No blade to hold on stage {} - unarmed instead.", g_stage);
    }

    // The model is only ever built here, and there is no second chance: event.c commits work->weapon
    // even with no constructor, and picking the same weapon again is a no-op. So register first.
    void __fastcall SetWeapon_hooked(uintptr_t work, int no_change)
    {
        if (MGS2_LinkVarBuf::GM_Weapon == kWpBlade && *g_bladeMngAlive == 0)
        {
            ArmBlade();
        }

        g_setWeapon_hook.fastcall<void>(work, no_change);

        // Asked for the blade and no actor came back - a stage short of its models, or an arm that did
        // not take. Either way he cannot hold it, so do not leave him posed around an empty hand.
        if (MGS2_LinkVarBuf::GM_Weapon == kWpBlade && Memory::ReadField<uintptr_t>(work, kPlWpAct) == 0)
        {
            SurrenderBlade();
        }
    }

    void GameTick()
    {
        if (!g_GameVars.MGS2_IsPlayableStage())
        {
            return;
        }
        strncpy_s(g_stage, g_GameVars.GetCurrentStage(), _TRUNCATE);

        const uintptr_t work = g_GameVars.GM_PlayerWork();
        const bool picked = MGS2_LinkVarBuf::GM_Weapon == kWpBlade
            || Memory::ReadField<int>(work, kPlWeapon, kWpNone) == kWpBlade;

        // bladeply.c drops PL_TargetCallbackFunc when the blade goes away but never PL_CheckAttackFunc,
        // so a scripted weapon swap leaves CQC running blade moves with a gun in hand.
        if (!picked && *g_checkAttackFunc == g_bladeCheckAttack)
        {
            *g_checkAttackFunc = nullptr;
        }

        // Only NewPluginBlade() ever reads the latch, and it does not ask what is equipped. Drop it
        // while the plugin is down, or a much later arm puts a drawn blade in a hand that never asked.
        if (!picked && *g_bladeMngAlive == 0)
        {
            short& state = MGS2_LinkVarBuf::GM_PlayerStateFlag.get();
            state = static_cast<short>(state & ~kBladeLatch);
        }

        // Snake's set and Raiden's; getitem.c GM_SetCurrentItemSet() repoints gm_weapons by protagonist.
        int16_t* const weapons[] = { MGS2_LinkVarBuf::GM_Weapons, MGS2_LinkVarBuf::GM_WeaponsR };
        int16_t* const maxes[] = { MGS2_LinkVarBuf::GM_WeaponsMax, MGS2_LinkVarBuf::GM_WeaponsMaxR };
        if (weapons[0] == nullptr || maxes[0] == nullptr)
        {
            return;
        }

        // Kept topped up every tick: a room restores the loadout from the save, the blade is not in it,
        // and an unowned weapon is an unselectable one.
        for (size_t set = 0; set < std::size(weapons); set++)
        {
            if (maxes[set][kWpBlade] < 1)
            {
                maxes[set][kWpBlade] = 1;
            }
            if (weapons[set][kWpBlade] < 1)
            {
                weapons[set][kWpBlade] = 1;
            }
        }
    }
}

void MGS2BladeAnywhere::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    uint8_t* ctor = Memory::PatternScan(baseModule, "48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 ?? 83 3D ?? ?? ?? ?? 00 0F 85", "MGS2: Blade Anywhere: sonoyama\\plugin\\bladeply.c -> NewPluginBlade()");
    uint8_t* alive = Memory::PatternScan(baseModule, "83 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 48 8D 15", "MGS2: Blade Anywhere: sonoyama\\plugin\\bladeply.c -> NewPluginBlade() | BladeMngAlive");
    uint8_t* flags = Memory::PatternScan(baseModule, "C7 05 ?? ?? ?? ?? ?? ?? ?? ?? 48 8D 54 24 ?? C7 05", "MGS2: Blade Anywhere: sonoyama\\plugin\\bladeply.c -> NewPluginBlade() | PullOutFlag, VisFlag");
    uint8_t* attack = Memory::PatternScan(baseModule, "48 8B 0D ?? ?? ?? ?? 48 8D 05", "MGS2: Blade Anywhere: sonoyama\\plugin\\bladeply.c -> NewPluginBlade() | PL_CheckAttackFunc");
    uint8_t* setWeapon = Memory::PatternScan(baseModule, "40 53 57 41 54 41 56 41 57 48 83 EC ?? 48 8B 05", "MGS2: Blade Anywhere: sonoyama\\raiden\\event.c -> SetWeapon()");
    if (ctor == nullptr || alive == nullptr || flags == nullptr || attack == nullptr
        || setWeapon == nullptr)
    {
        spdlog::info("MGS2: Blade Anywhere: Pattern scan failed, skipping.");
        return;
    }

    g_newPluginBlade = reinterpret_cast<NewPluginBladeFn>(ctor);
    g_bladeMngAlive = reinterpret_cast<int*>(Memory::GetRipRelativeAddress(alive, 2, 7));
    g_pullOutFlag = reinterpret_cast<int*>(Memory::GetRipRelativeAddress(flags, 2, 10));
    g_visFlag = reinterpret_cast<int*>(Memory::GetRipRelativeAddress(flags + 15, 2, 10));
    g_bladeCheckAttack = reinterpret_cast<void*>(Memory::GetRipRelativeAddress(attack + 7, 3, 7));
    g_checkAttackFunc = reinterpret_cast<void**>(Memory::GetRipRelativeAddress(attack + 14, 3, 7));
    if (g_bladeMngAlive == nullptr || g_pullOutFlag == nullptr || g_visFlag == nullptr
        || g_bladeCheckAttack == nullptr || g_checkAttackFunc == nullptr
        || g_GameVars.GCL_CommandLine() == nullptr || g_GameVars.GCL_NextStrPtr() == nullptr)
    {
        spdlog::info("MGS2: Blade Anywhere: Globals not resolved, skipping.");
        return;
    }

    g_setWeapon_hook = safetyhook::create_inline(reinterpret_cast<void*>(setWeapon),
        reinterpret_cast<void*>(SetWeapon_hooked));
    LOG_HOOK(g_setWeapon_hook, "MGS2: Blade Anywhere: event.c SetWeapon() arm and reconcile")

    // A model the blade wants can be absent on a stage that never had it, and DG_MakeObjs derefs the
    // null def rather than checking. Its own callers cope with null, so hand them one and name the file.
    if (uint8_t* makeObjs = Memory::PatternScan(baseModule, "48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 54 41 56 41 57 48 83 EC ?? 41 8B E8", "MGS2: Blade Anywhere: system\\libdg\\objs.c -> DG_MakeObjs() | missing-model guard"))
    {
        g_makeObjs_hook = safetyhook::create_inline(reinterpret_cast<void*>(makeObjs),
            reinterpret_cast<void*>(MakeObjs_hooked));
        LOG_HOOK(g_makeObjs_hook, "MGS2: Blade Anywhere: DG_MakeObjsD() missing-model guard")
    }

    // CheckBladePullOut() picks its draw animation off the stance, and the fallthrough just shows the
    // blade. Miss on the two upright stances and the draw is instant, so a pad demo survives it.
    // Crouch and prone keep theirs: those animations end in PL_StillMode[STAND] and are the only thing
    // standing the player back up, so skipping them leaves him on the floor.
    MAKE_HOOK_MID(baseModule, "C7 05 ?? ?? ?? ?? 01 00 00 00 3D 44 D6 42 00",
        "MGS2: Blade Anywhere: sonoyama\\plugin\\bladeply.c -> CheckBladePullOut() | draw stance",
    {
        if (ctx.rax == kStandStill || ctx.rax == kStandRun) { ctx.rax = 0; }
    });

    // Same on the way back in, and a crouch joins them: putting it away has no reason to stand him up,
    // which is what BMputback ends on. A slash keeps its animation - that one is matched against
    // work->action rather than the stance, so it is out of reach here and better left alone.
    MAKE_HOOK_MID(baseModule, "41 81 F8 44 D6 42 00",
        "MGS2: Blade Anywhere: sonoyama\\plugin\\bladeply.c -> CheckBladePullOut() | sheathe stance",
    {
        if (ctx.r8 == kStandStill || ctx.r8 == kSquatStill || ctx.r8 == kStandRun) { ctx.r8 = 0; }
    });

    // bladeply.c CheckBladePullOut(), where it puts his own arc back: work->current_mar, then
    // work->org_motion into PL_ChangeMotionArc.
    if (uint8_t* arc = Memory::PatternScan(baseModule,
        "39 83 ?? ?? ?? ?? 75 ?? 8B 93 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ??",
        "MGS2: Blade Anywhere: sonoyama\\plugin\\bladeply.c -> CheckBladePullOut() | motion arc restore"))
    {
        g_curMar = *reinterpret_cast<int32_t*>(arc + 2);
        g_orgMotion = *reinterpret_cast<int32_t*>(arc + 10);
        g_PL_ChangeMotionArc = reinterpret_cast<PL_ChangeMotionArcFn>(Memory::ResolveCall(arc + 17));
    }
    // water.c SetWaterAct(): walking into the water picks its stair animation out of whatever set is
    // loaded, and the blade's set has no stair animation - so he gets the unsheathe, which never walks
    // him anywhere, and the stairs only end once he has. Leaving the water loads his own set first.
    if (uint8_t* stair = Memory::PatternScan(baseModule,
        "45 33 C9 45 33 C0 48 8B CB 41 8D 51 ?? E8 ?? ?? ?? ?? 85 FF 78 ?? 66 89 BB ?? ?? ?? ?? 66 89 BB ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ?? B9",
        "MGS2: Blade Anywhere: sonoyama\\plugin\\water.c -> SetWaterAct() | stair entry motion arc"))
    {
        static SafetyHookMid stairHook {};
        stairHook = safetyhook::create_mid(stair, [](SafetyHookContext& ctx)
        {
            const uintptr_t work = ctx.rbx;
            if (work == 0 || !g_PL_ChangeMotionArc)
            {
                return;
            }
            if (*reinterpret_cast<int*>(work + g_curMar) == kRaiBladeMotion)
            {
                g_PL_ChangeMotionArc(work, *reinterpret_cast<int*>(work + g_orgMotion));
            }
        });
        LOG_HOOK(stairHook, "MGS2: Blade Anywhere: water stairs start on his own motion arc")
    }

    g_weaponReadHooks.reserve(8);

    // bladeply.c Hitted() only sparks off WP_BULLET and answers WP_SHOTGUN_NEAR, so Fortune's railgun
    // rounds - fort_bullet.c fires them as WP_NIKITA - are swallowed with no parry at all. The guard
    // had already eaten the damage before this test, so lending them a bullet's bit only adds the
    // spark, the ring and the guard reaction.
    HookWeaponRead(
        "48 8B 87 ?? ?? ?? ?? 48 B9 1C 80 04 00 00 00 08 00 48 85 C1 0F 84 ?? ?? ?? ?? F3 0F 10 47",
        "MGS2: Blade Anywhere: sonoyama\\plugin\\bladeply.c -> Hitted() | guarded weapon", 1,
        [](SafetyHookContext& ctx) { if (ctx.rax & kWpNikitaBit) { ctx.rax |= kWpUspBit; } });

    // SlashHoming() derefs homing->body unchecked, and a stage that never had the blade can offer a
    // homing target without one, so answer the hazard test for it and let its own early-out run.
    MAKE_HOOK_MID(baseModule, "85 C0 0F 85 ?? ?? ?? ?? 48 8B 8E ?? ?? ?? ?? 48 8D 54 24",
        "MGS2: Blade Anywhere: sonoyama\\plugin\\bladeply.c -> SlashHoming() | bodyless target",
    {
        if (Memory::ReadField<void*>(ctx.rdi, 8) == nullptr
            && !(g_GameVars.Get_GM_GameStatus() & MGS2_StatusFlags::STATE_VR_ANOTHER))
        {
            ctx.rax = 1;   // "blocked", which is the one answer that makes it give up
        }
    });

    // fatdamage.c gives each of Fatman's five damage handlers its own dispatch and inlines
    // FAT_SetWepDmgWork's narc and stun timers right after, so translate at every weapon read.
    // Patching one point mid-chain only ever reached the state he was standing in.
    HookWeaponRead("4D 8B B8 ?? ?? ?? ?? 49 8B EF",
        "MGS2: Blade Anywhere: okuta\\fatman\\fatdamage.c -> weapon read | downed, part r14, work rdi", 1,
        [](SafetyHookContext& ctx)
        {
            if (FatmanBluntHit(ctx.r15)) { ctx.r14 = kFatHead; FatmanBluntFaint(ctx.rdi); return; }
            if (FatmanForceHead(ctx.r15)) { ctx.r14 = kFatHead; }
            ctx.r15 = FatmanWeapon(ctx.r15);
        });
    HookWeaponRead("4D 8B B8 ?? ?? ?? ?? 4D 8B F7 41 83 E6 ?? 49 0F BA E7 ?? 73 ?? 89 B7",
        "MGS2: Blade Anywhere: okuta\\fatman\\fatdamage.c -> weapon read | downed, part rbp, work rdi", 1,
        [](SafetyHookContext& ctx)
        {
            if (FatmanBluntHit(ctx.r15)) { ctx.rbp = kFatHead; FatmanBluntFaint(ctx.rdi); return; }
            if (FatmanForceHead(ctx.r15)) { ctx.rbp = kFatHead; }
            ctx.r15 = FatmanWeapon(ctx.r15);
        });
    HookWeaponRead("4D 8B B8 A0 00 00 00 4D 8B F7 41 83 E6 02 49 0F BA E7 0B 73 ?? 89 B3",
        "MGS2: Blade Anywhere: okuta\\fatman\\fatdamage.c -> weapon read | downed, part rbp, work rbx", 1,
        [](SafetyHookContext& ctx)
        {
            if (FatmanBluntHit(ctx.r15)) { ctx.rbp = kFatHead; FatmanBluntFaint(ctx.rbx); return; }
            if (FatmanForceHead(ctx.r15)) { ctx.rbp = kFatHead; }
            ctx.r15 = FatmanWeapon(ctx.r15);
        });
    HookWeaponRead("4D 8B B8 ?? ?? ?? ?? 49 8B DF",
        "MGS2: Blade Anywhere: okuta\\fatman\\fatdamage.c -> weapon read | r15, part r13", 1,
        [](SafetyHookContext& ctx)
        {
            if (FatmanForceHead(ctx.r15)) { ctx.r13 = kFatHead; }
            ctx.r15 = FatmanWeapon(ctx.r15);
        });
    HookWeaponRead("49 8B A8 ?? ?? ?? ?? E8",
        "MGS2: Blade Anywhere: okuta\\fatman\\fatdamage.c -> weapon read | rbp, part r14", 1,
        [](SafetyHookContext& ctx)
        {
            if (FatmanForceHead(ctx.rbp)) { ctx.r14 = kFatHead; }
            ctx.rbp = FatmanWeapon(ctx.rbp);
        });
    HookWeaponRead("49 8B AF ?? ?? ?? ?? 33 DB",
        "MGS2: Blade Anywhere: okuta\\fatman\\fatdamage.c -> weapon read | rbp, r12d stun", 1,
        [](SafetyHookContext& ctx) { ctx.rbp = FatmanWeapon(ctx.rbp); });
    HookWeaponRead("49 8B BF ?? ?? ?? ?? 33 ED",
        "MGS2: Blade Anywhere: okuta\\fatman\\fatdamage.c -> weapon read | rdi, r12d stun", 1,
        [](SafetyHookContext& ctx) { ctx.rdi = FatmanWeapon(ctx.rdi); });

    // Vamp has an arm for the edge and none for the flat, so let the flat take the edge's.
    MAKE_HOOK_MID(baseModule, "49 0F BA E7 ?? 73 ?? 8B 8D",
        "MGS2: Blade Anywhere: shibata\\vamp\\vamp.c -> ChildDmgCallBack() | blade arm",
    {
        if (ctx.r15 & kWpStuns)
        {
            ctx.r15 |= kWpCuts;
        }
    });

    // Then move its damage into the faint field - VMP_SET_DMFLAG packs those at bits 9 and 15.
    MAKE_HOOK_MID(baseModule, "89 93 ?? ?? ?? ?? EB ?? 49 0F BA E7",
        "MGS2: Blade Anywhere: shibata\\vamp\\vamp.c -> ChildDmgCallBack() | blade damage",
    {
        if (ctx.r15 & kWpStuns)
        {
            const uint32_t dealt = (static_cast<uint32_t>(ctx.rdx) >> 9) & 0x3F;
            ctx.rdx = (ctx.rdx & ~(0x3Full << 9)) | (static_cast<uint64_t>(dealt) << 15);
            // The kind picks the reaction motion, and the edge's has nothing to play for a faint.
            ctx.rdx = (ctx.rdx & ~0xFull) | kVmpStun;
        }
    });

    // The blade's whole per-stage kit, as w45a registers it; the merge dedups against stages with it.
    BP_FilesysChanges::AddUniversalStageLines(
        {
            "assets/mar/us/rai_blade.mar,us/stage/%S%/cache/00ccb3e9.mar,cache/00ccb3e9.mar",
            "assets/sar/us/rai_blade.sar,us/stage/%S%/cache/00ccb3e9.sar,cache/00ccb3e9.sar",
            "assets/tri/us/hfb_mt.tri,us/stage/%S%/cache/00928aea.tri,cache/00928aea.tri",
            "assets/kms/us/hfb_mt.kms,us/stage/%S%/cache/00928aea.kms,cache/00928aea.kms",
            "assets/kms/us/hfb_sub_mt.kms,us/stage/%S%/cache/00ebb2ce.kms,cache/00ebb2ce.kms",
            "assets/kms/us/hfb_mineuchi_mt.kms,us/stage/%S%/cache/0019a17d.kms,cache/0019a17d.kms",
            "assets/kms/us/hfb_mineuchi_sub_mt.kms,us/stage/%S%/cache/00142438.kms,cache/00142438.kms",
        },
        {
            "assets/kms/us/hfb_mt.cmdl,us/stage/%S%/cache/00928aea.cmdl,%R%/stage/%S%/cache/00928aea.cmdl",
            "assets/kms/us/hfb_sub_mt.cmdl,us/stage/%S%/cache/00ebb2ce.cmdl,%R%/stage/%S%/cache/00ebb2ce.cmdl",
            "assets/kms/us/hfb_mineuchi_mt.cmdl,us/stage/%S%/cache/0019a17d.cmdl,%R%/stage/%S%/cache/0019a17d.cmdl",
            "assets/kms/us/hfb_mineuchi_sub_mt.cmdl,us/stage/%S%/cache/00142438.cmdl,%R%/stage/%S%/cache/00142438.cmdl",
            "textures/flatlist/hfb_ovl_mod1120_alp_emap.bmp.ctxr,stage/%S%/cache/hfb_ovl_mod1120_alp_emap.bmp.ctxr,%R%/stage/%S%/cache/00928aea/00d1ba75.ctxr",
            "textures/flatlist/hfb_sb_gr2.bmp.ctxr,stage/%S%/cache/hfb_sb_gr2.bmp.ctxr,%R%/stage/%S%/cache/00928aea/00ba532b.ctxr",
            "textures/flatlist/hfb_sb_gr_mk.bmp.ctxr,stage/%S%/cache/hfb_sb_gr_mk.bmp.ctxr,%R%/stage/%S%/cache/00928aea/004d70f4.ctxr",
            "textures/flatlist/hfb_sori_gr2.bmp.ctxr,stage/%S%/cache/hfb_sori_gr2.bmp.ctxr,%R%/stage/%S%/cache/00928aea/006414c1.ctxr",
            "textures/flatlist/hfb_sub_gr2_alp_ovl.bmp.ctxr,stage/%S%/cache/hfb_sub_gr2_alp_ovl.bmp.ctxr,%R%/stage/%S%/cache/00928aea/0003d28d.ctxr",
            "textures/flatlist/hfb_tuba.bmp.ctxr,stage/%S%/cache/hfb_tuba.bmp.ctxr,%R%/stage/%S%/cache/00928aea/002f3aeb.ctxr",
            "textures/flatlist/hfbs3_sb.bmp.ctxr,stage/%S%/cache/hfbs3_sb.bmp.ctxr,%R%/stage/%S%/cache/00928aea/004e650d.ctxr",
            "textures/flatlist/hfbs3_sub_alp_ovl.bmp.ctxr,stage/%S%/cache/hfbs3_sub_alp_ovl.bmp.ctxr,%R%/stage/%S%/cache/00928aea/00a028ef.ctxr",
        });

    MAKE_HOOK_MID(baseModule,
        "48 89 5C 24 ?? E8 ?? ?? ?? ?? 44 8B 0D",
        "MGS2: Blade Anywhere: system\\libgv\\pad.c -> GV_UpdatePadSystem() body | game tick", {
        GameTick();
    });
}
