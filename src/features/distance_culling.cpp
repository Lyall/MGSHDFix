#include "stdafx.h"

#include "common.hpp"
#include "distance_culling.hpp"

#include "gamevars.hpp"
#include "input_handler.hpp"
#include "logging.hpp"

namespace
{
    constexpr uint32_t PARROT_MODEL_STRCODE = GameVars::GV_StrCode("par_def_mh");
#if defined(RELEASE_BUILD)
    bool log_bird = false;
#else
    bool log_bird = true;
#endif
}

void DistanceCulling::Initialize() const
{
    if (eGameType & MGS2)
    {
        //todo - test birds nLod in mgs2x\source\user\okuta\kamome\kmtest.c. 
        //  disabling their LOD will enable their animation skeletion, need to verify no performance impact.

        if (bMGS2_ForceNPCLOD)
        {
            //if (bMGS2_MarineForceLOD)
            {
                MAKE_HOOK_MID(baseModule, "44 0F 44 C9 41 3B C1", "MGS2: Marine LOD | korekado/hold/holdene.c -> SetLOD() @ L2484", {
                    ctx.rcx = 0;
                              });

                MAKE_HOOK_MID(baseModule, "0F 44 E9 8B 4A", "MGS2: Marine LOD | korekado/hold/holdene.c -> ChangeActive() @ L766", {
                    ctx.rcx = 0;
                              });
            }

           // if (bMGS2_ForcePlayerLOD)
            {
                MAKE_HOOK_MID(baseModule, "8B 8E ?? ?? ?? ?? 8B EF", "MGS2: NewLoDControl()", {
                    ctx.xmm0.f32[0] = 0.0f;
                              });
            }

            //hostages
            MAKE_HOOK_MID(baseModule, "8B D0 3D ?? ?? ?? ?? 7E", "MGS2: LodHostage()", {
                ctx.rax = 0.0f;
                          });

            //evm models, ie snake, pres, parrot, vamp, emma
            /*
            MAKE_HOOK_MID(baseModule, "3B C1 7E ?? 41 81 E0 ?? ?? ?? ?? 45 89 41 ?? 81 4F ?? ?? ?? ?? ?? EB ?? 41 81 C8 ?? ?? ?? ?? 45 89 41 ?? 81 67 ?? ?? ?? ?? ?? 8B 8B ?? ?? ?? ?? 85 C9 74 ?? 48 8B D7 E8 ?? ?? ?? ?? 48 8B 5C 24", "MGS2: NPC_AfterProcess()", {
                    ctx.rax = 0.0f;
                          });
                          */
            if (uint8_t* NPC_AfterProcessscan = Memory::PatternScan(baseModule, "7E ?? 41 81 E0 ?? ?? ?? ?? 45 89 41 ?? 81 4F ?? ?? ?? ?? ?? EB ?? 41 81 C8 ?? ?? ?? ?? 45 89 41 ?? 81 67 ?? ?? ?? ?? ?? 8B 8B ?? ?? ?? ?? 85 C9 74 ?? 48 8B D7 E8 ?? ?? ?? ?? 48 8B 5C 24", "MGS2: NPC_AfterProcess()"))
            {
                Memory::PatchBytes((uintptr_t)NPC_AfterProcessscan, "\xEB", 1);
            }
        }

        if (bAlwaysRenderShellCasings)
        {
            MAKE_HOOK_MID(baseModule, "76 ?? 80 E1 ?? 88 4B ?? 41 FF 8F", "MGS2: skoba\\weapon_old\\emb_control.c -> NewEnbControl() -> Act() @ L406", {
                reghelpers::SetCF(ctx, true);
                        });
        }
        else
        {
        //Fixes rogue distance check that was causing shell casing to not appear in some cases, ie Harrier/Solidus intro cutscene.
        //GM_PlayerPosition.vy was being checked instead of GM_CameraY. Appears to have been a Substance regression, as the old version properly checked against DG_Chanls->eye.m[3] (cam_pos.vy).
            MAKE_HOOK_MID(baseModule, "F3 0F 58 0D ?? ?? ?? ?? F3 0F 10 43", "MGS2: skoba\\weapon_old\\emb_control.c -> NewEnbControl() -> Act() @ L406 #1", {
               ctx.xmm1.f32[0] = (float)MGS2_LinkVarBuf::GM_CameraY.get();
                        });

            //Small behavior change: while the bug is ultimately fixed by the hook above, we're at higher resolution now, and memory alloc isn't a concern anymore.
            //force shell casing to always appear during cutscenes regardless of height difference/distance from camera.
            MAKE_HOOK_MID(baseModule, "76 ?? 80 E1 ?? 88 4B ?? 41 FF 8F", "MGS2: skoba\\weapon_old\\emb_control.c -> NewEnbControl() -> Act()  @ L406 #2", {
                if (g_GameVars.InCutscene())
                {
                    reghelpers::SetCF(ctx, true);
                }
                        });
        }

    }
    else if (eGameType & MGS3)
    {
        if (bForceGrassAlways || fGrassDistanceScalar != 1.0f)
        {
            //todo - disable automatically if Util::IsSteamOS() in the boss's arena.
            MAKE_HOOK_MID(baseModule, "F3 0F 11 83 ?? ?? ?? ?? 41 8B FC", "MGS3: Grass Farclip", {
               ctx.xmm0.f32[0] = g_DistanceCulling.bForceGrassAlways ? std::numeric_limits<float>::max() : ctx.xmm0.f32[0] * g_DistanceCulling.fGrassDistanceScalar;
                })

            if (bForceGrassAlways)
            {
                g_InputHandler.RegisterHotkey(vkForceGrassAlwaysToggle, "Force Grass Distance", []()
                    {
                        g_DistanceCulling.bForceGrassAlways = !g_DistanceCulling.bForceGrassAlways;
                    });
            }
        }


        /*
        if (uint8_t* akkf_low_Scan = Memory::PatternScan(baseModule, "61 6B 6B 66 5F 6C 6F 77 00", "akkf_low"))
        {
            Memory::PatchBytes((uintptr_t)akkf_low_Scan, "\x61\x6B\x6B\x66\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* akmr_amo_low_Scan = Memory::PatternScan(baseModule, "61 6B 6D 72 5F 61 6D 6F 5F 6C 6F 77 00", "akmr_amo_low"))
        {
            Memory::PatchBytes((uintptr_t)akmr_amo_low_Scan, "\x61\x6B\x6D\x72\x5F\x61\x6D\x6F\x00\x00\x00\x00\x00", 13);
        }

        if (uint8_t* akmr_low_Scan = Memory::PatternScan(baseModule, "61 6B 6D 72 5F 6C 6F 77 00", "akmr_low"))
        {
            Memory::PatchBytes((uintptr_t)akmr_low_Scan, "\x61\x6B\x6D\x72\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* amd_amo_low_Scan = Memory::PatternScan(baseModule, "61 6D 64 5F 61 6D 6F 5F 6C 6F 77 00", "amd_amo_low"))
        {
            Memory::PatchBytes((uintptr_t)amd_amo_low_Scan, "\x61\x6D\x64\x5F\x61\x6D\x6F\x00\x00\x00\x00\x00", 12);
        }

        if (uint8_t* amdr_low_Scan = Memory::PatternScan(baseModule, "61 6D 64 72 5F 6C 6F 77 00", "amdr_low"))
        {
            Memory::PatchBytes((uintptr_t)amdr_low_Scan, "\x61\x6D\x64\x72\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* dragnov_low_Scan = Memory::PatternScan(baseModule, "64 72 61 67 6E 6F 76 5F 6C 6F 77 00", "dragnov_low"))
        {
            Memory::PatchBytes((uintptr_t)dragnov_low_Scan, "\x64\x72\x61\x67\x6E\x6F\x76\x00\x00\x00\x00\x00", 12);
        }

        if (uint8_t* egun_low_Scan = Memory::PatternScan(baseModule, "65 67 75 6E 5F 6C 6F 77 00", "egun_low"))
        {
            Memory::PatchBytes((uintptr_t)egun_low_Scan, "\x65\x67\x75\x6E\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* fork_c_low_Scan = Memory::PatternScan(baseModule, "66 6F 72 6B 2E 63 5F 6C 6F 77 00", "fork.c_low"))
        {
            Memory::PatchBytes((uintptr_t)fork_c_low_Scan, "\x66\x6F\x72\x6B\x2E\x63\x00\x00\x00\x00\x00", 11);
        }

        if (uint8_t* gave_amo_low_Scan = Memory::PatternScan(baseModule, "67 61 76 65 5F 61 6D 6F 5F 6C 6F 77 00", "gave_amo_low"))
        {
            Memory::PatchBytes((uintptr_t)gave_amo_low_Scan, "\x67\x61\x76\x65\x5F\x61\x6D\x6F\x00\x00\x00\x00\x00", 13);
        }

        if (uint8_t* gave_low_Scan = Memory::PatternScan(baseModule, "67 61 76 65 5F 6C 6F 77 00", "gave_low"))
        {
            Memory::PatchBytes((uintptr_t)gave_low_Scan, "\x67\x61\x76\x65\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* gmic_low_Scan = Memory::PatternScan(baseModule, "67 6D 69 63 5F 6C 6F 77 00", "gmic_low"))
        {
            Memory::PatchBytes((uintptr_t)gmic_low_Scan, "\x67\x6D\x69\x63\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* itha_amo_low_Scan = Memory::PatternScan(baseModule, "69 74 68 61 5F 61 6D 6F 5F 6C 6F 77 00", "itha_amo_low"))
        {
            Memory::PatchBytes((uintptr_t)itha_amo_low_Scan, "\x69\x74\x68\x61\x5F\x61\x6D\x6F\x00\x00\x00\x00\x00", 13);
        }

        if (uint8_t* itha_low_Scan = Memory::PatternScan(baseModule, "69 74 68 61 5F 6C 6F 77 00", "itha_low"))
        {
            Memory::PatchBytes((uintptr_t)itha_low_Scan, "\x69\x74\x68\x61\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* ithaca_low_Scan = Memory::PatternScan(baseModule, "69 74 68 61 63 61 5F 6C 6F 77 00", "ithaca_low"))
        {
            Memory::PatchBytes((uintptr_t)ithaca_low_Scan, "\x69\x74\x68\x61\x63\x61\x00\x00\x00\x00\x00", 11);
        }

        if (uint8_t* m16a_amo_low_Scan = Memory::PatternScan(baseModule, "6D 31 36 61 5F 61 6D 6F 5F 6C 6F 77 00", "m16a_amo_low"))
        {
            Memory::PatchBytes((uintptr_t)m16a_amo_low_Scan, "\x6D\x31\x36\x61\x5F\x61\x6D\x6F\x00\x00\x00\x00\x00", 13);
        }

        if (uint8_t* m16a_low_Scan = Memory::PatternScan(baseModule, "6D 31 36 61 5F 6C 6F 77 00", "m16a_low"))
        {
            Memory::PatchBytes((uintptr_t)m16a_low_Scan, "\x6D\x31\x36\x61\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* m63_amo_low_Scan = Memory::PatternScan(baseModule, "6D 36 33 5F 61 6D 6F 5F 6C 6F 77 00", "m63_amo_low"))
        {
            Memory::PatchBytes((uintptr_t)m63_amo_low_Scan, "\x6D\x36\x33\x5F\x61\x6D\x6F\x00\x00\x00\x00\x00", 12);
        }

        if (uint8_t* m63_low_Scan = Memory::PatternScan(baseModule, "6D 36 33 5F 6C 6F 77 00", "m63_low"))
        {
            Memory::PatchBytes((uintptr_t)m63_low_Scan, "\x6D\x36\x33\x00\x00\x00\x00\x00", 8);
        }

        if (uint8_t* m63a_amo_low_Scan = Memory::PatternScan(baseModule, "6D 36 33 61 5F 61 6D 6F 5F 6C 6F 77 00", "m63a_amo_low"))
        {
            Memory::PatchBytes((uintptr_t)m63a_amo_low_Scan, "\x6D\x36\x33\x61\x5F\x61\x6D\x6F\x00\x00\x00\x00\x00", 13);
        }

        if (uint8_t* m63a_low_Scan = Memory::PatternScan(baseModule, "6D 36 33 61 5F 6C 6F 77 00", "m63a_low"))
        {
            Memory::PatchBytes((uintptr_t)m63a_low_Scan, "\x6D\x36\x33\x61\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* maka_amo_low_Scan = Memory::PatternScan(baseModule, "6D 61 6B 61 5F 61 6D 6F 5F 6C 6F 77 00", "maka_amo_low"))
        {
            Memory::PatchBytes((uintptr_t)maka_amo_low_Scan, "\x6D\x61\x6B\x61\x5F\x61\x6D\x6F\x00\x00\x00\x00\x00", 13);
        }

        if (uint8_t* maka_low_Scan = Memory::PatternScan(baseModule, "6D 61 6B 61 5F 6C 6F 77 00", "maka_low"))
        {
            Memory::PatchBytes((uintptr_t)maka_low_Scan, "\x6D\x61\x6B\x61\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* mk22_amo_low_Scan = Memory::PatternScan(baseModule, "6D 6B 32 32 5F 61 6D 6F 5F 6C 6F 77 00", "mk22_amo_low"))
        {
            Memory::PatchBytes((uintptr_t)mk22_amo_low_Scan, "\x6D\x6B\x32\x32\x5F\x61\x6D\x6F\x00\x00\x00\x00\x00", 13);
        }

        if (uint8_t* mk22_low_Scan = Memory::PatternScan(baseModule, "6D 6B 32 32 5F 6C 6F 77 00", "mk22_low"))
        {
            Memory::PatchBytes((uintptr_t)mk22_low_Scan, "\x6D\x6B\x32\x32\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* mosinnagant_low_Scan = Memory::PatternScan(baseModule, "6D 6F 73 69 6E 6E 61 67 61 6E 74 5F 6C 6F 77 00", "mosinnagant_low"))
        {
            Memory::PatchBytes((uintptr_t)mosinnagant_low_Scan, "\x6D\x6F\x73\x69\x6E\x6E\x61\x67\x61\x6E\x74\x00\x00\x00\x00\x00", 16);
        }

        if (uint8_t* msnn_low_Scan = Memory::PatternScan(baseModule, "6D 73 6E 6E 5F 6C 6F 77 00", "msnn_low"))
        {
            Memory::PatchBytes((uintptr_t)msnn_low_Scan, "\x6D\x73\x6E\x6E\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* pato_low_Scan = Memory::PatternScan(baseModule, "70 61 74 6F 5F 6C 6F 77 00", "pato_low"))
        {
            Memory::PatchBytes((uintptr_t)pato_low_Scan, "\x70\x61\x74\x6F\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* rpg7_low_Scan = Memory::PatternScan(baseModule, "72 70 67 37 5F 6C 6F 77 00", "rpg7_low"))
        {
            Memory::PatchBytes((uintptr_t)rpg7_low_Scan, "\x72\x70\x67\x37\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* rpg7_rocket_low_Scan = Memory::PatternScan(baseModule, "72 70 67 37 5F 72 6F 63 6B 65 74 5F 6C 6F 77 00", "rpg7_rocket_low"))
        {
            Memory::PatchBytes((uintptr_t)rpg7_rocket_low_Scan, "\x72\x70\x67\x37\x5F\x72\x6F\x63\x6B\x65\x74\x00\x00\x00\x00\x00", 16);
        }

        if (uint8_t* saaa_low_Scan = Memory::PatternScan(baseModule, "73 61 61 61 5F 6C 6F 77 00", "saaa_low"))
        {
            Memory::PatchBytes((uintptr_t)saaa_low_Scan, "\x73\x61\x61\x61\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* svdd_amo_low_Scan = Memory::PatternScan(baseModule, "73 76 64 64 5F 61 6D 6F 5F 6C 6F 77 00", "svdd_amo_low"))
        {
            Memory::PatchBytes((uintptr_t)svdd_amo_low_Scan, "\x73\x76\x64\x64\x5F\x61\x6D\x6F\x00\x00\x00\x00\x00", 13);
        }

        if (uint8_t* svdd_low_Scan = Memory::PatternScan(baseModule, "73 76 64 64 5F 6C 6F 77 00", "svdd_low"))
        {
            Memory::PatchBytes((uintptr_t)svdd_low_Scan, "\x73\x76\x64\x64\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* svkn_low_Scan = Memory::PatternScan(baseModule, "73 76 6B 6E 5F 6C 6F 77 00", "svkn_low"))
        {
            Memory::PatchBytes((uintptr_t)svkn_low_Scan, "\x73\x76\x6B\x6E\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* tonp_amo_low_Scan = Memory::PatternScan(baseModule, "74 6F 6E 70 5F 61 6D 6F 5F 6C 6F 77 00", "tonp_amo_low"))
        {
            Memory::PatchBytes((uintptr_t)tonp_amo_low_Scan, "\x74\x6F\x6E\x70\x5F\x61\x6D\x6F\x00\x00\x00\x00\x00", 13);
        }

        if (uint8_t* tonp_low_Scan = Memory::PatternScan(baseModule, "74 6F 6E 70 5F 6C 6F 77 00", "tonp_low"))
        {
            Memory::PatchBytes((uintptr_t)tonp_low_Scan, "\x74\x6F\x6E\x70\x00\x00\x00\x00\x00", 9);
        }

        if (uint8_t* vz61_amo_low_Scan = Memory::PatternScan(baseModule, "76 7A 36 31 5F 61 6D 6F 5F 6C 6F 77 00", "vz61_amo_low"))
        {
            Memory::PatchBytes((uintptr_t)vz61_amo_low_Scan, "\x76\x7A\x36\x31\x5F\x61\x6D\x6F\x00\x00\x00\x00\x00", 13);
        }

        if (uint8_t* vz61_low_Scan = Memory::PatternScan(baseModule, "76 7A 36 31 5F 6C 6F 77 00", "vz61_low"))
        {
            Memory::PatchBytes((uintptr_t)vz61_low_Scan, "\x76\x7A\x36\x31\x00\x00\x00\x00\x00", 9);
        }
        /*
        if (uint8_t* saa_eva_Scan = Memory::PatternScan(baseModule, "73 61 61 5F 65 76 61 00", "saa_eva"))
        {
            Memory::PatchBytes((uintptr_t)saa_eva_Scan, "\x73\x61\x61\x00\x00\x00\x00\x00", 8);
        }

        if (uint8_t* saa_oce23a_Scan = Memory::PatternScan(baseModule, "73 61 61 5F 6F 63 65 32 33 61 00", "saa_oce23a"))
        {
            Memory::PatchBytes((uintptr_t)saa_oce23a_Scan, "\x73\x61\x61\x00\x00\x00\x00\x00\x00\x00\x00", 11);
        }*/
    }
}
