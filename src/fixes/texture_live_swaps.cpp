#include "stdafx.h"

#include "texture_live_swaps.hpp"

#include "common.hpp"
#include "expand_bp_assets.hpp"
#include "gamevars.hpp"
#include "logging.hpp"

typedef uintptr_t* (__fastcall* FASTCALL_1IN1OUT)(long long);
typedef uintptr_t* (__fastcall* FASTCALL_2IN1OUT)(long long, long long);
typedef uintptr_t (__fastcall* FASTCALL_3IN1OUT)(long long, long long, int);

namespace
{
    constexpr unsigned int STRCODE_PDRAY_OTHER = 2151908;
    constexpr unsigned int STRCODE_HAR01 = 11685431;
    constexpr unsigned int STRCODE_W24C1 = 0x89dc98;
    constexpr unsigned int STRCODE_REV_DISP = 0x4c4dfd;
    constexpr unsigned int STRCODE_BDU_DISP = 0x20158d;

    constexpr unsigned int STRCODE_NODE_TITLE_TEX_TRI = 0xbbe697;
    constexpr unsigned int STRCODE_TITLE_LOGO = 0x07bc34;
    constexpr unsigned int STRCODE_TITLE_NUMBAH_TWO = 0xc1181b;
    /// DG_GetTexture2
    FASTCALL_2IN1OUT GetCtxrHandle;
    /// BP_ReplaceTexture
    FASTCALL_2IN1OUT CopyCtxr;
    FASTCALL_3IN1OUT AllocTexture;
    FASTCALL_1IN1OUT FreeTexture;

    std::list<std::pair<uintptr_t, uintptr_t>> SwapMap;

    bool bRestoringCtxrs = false;

    bool GetAndCopyCtxr(int tricode, int dst, int src, bool shouldSave = true)
    {
        if (!GetCtxrHandle || !CopyCtxr || !AllocTexture)
            return false;

        uintptr_t* srcCtxr = GetCtxrHandle(tricode, src);
        uintptr_t* dstCtxr = GetCtxrHandle(tricode, dst);

        if (srcCtxr == nullptr || dstCtxr == nullptr)
        {
            //spdlog::info("GetAndCopyCtxr: Failed to get ctxr handle for tricode {:x}, src {}, dst {}", tricode, src, dst);
            return false;
        }

        uintptr_t srcHandle = srcCtxr[4];
        uintptr_t dstHandle = dstCtxr[4];

        if (!srcHandle || !dstHandle)
        {
            //spdlog::info("GetAndCopyCtxr: Invalid ctxr handle for tricode {:x}, src {}, dst {}", tricode, src, dst);
            return false;
        }

        // Need to save handles to restore on stage reset
        for (auto it = SwapMap.begin(); it != SwapMap.end(); it++)
        {
            if (it->second == dstHandle)
            {
                // Texture already swapped, do not add to map
                shouldSave = false;
                break;
            }
        }

        if (shouldSave)
        {
            // Arguments are width, height, clut, but apparently don't matter?
            uintptr_t saveHandle = AllocTexture(16, 16, 0);

            if (!saveHandle)
                return false;

            CopyCtxr(dstHandle, saveHandle);
            SwapMap.push_back({ saveHandle, dstHandle });
        }

        CopyCtxr(srcHandle, dstHandle);
        return true;
    }

    void RestoreCtxrs()
    {
        if (!CopyCtxr || !FreeTexture || bRestoringCtxrs || SwapMap.empty())
            return;

        bRestoringCtxrs = true;

        struct RestoreGuard
        {
            ~RestoreGuard()
            {
                bRestoringCtxrs = false;
            }
        } guard;

        auto swaps = std::move(SwapMap);
        SwapMap.clear();

        for (const auto& [saveHandle, dstHandle] : swaps)
        {
            if (!saveHandle || !dstHandle)
                continue;

            CopyCtxr(saveHandle, dstHandle);
            FreeTexture(saveHandle);
        }
    }


    void RestoreCtxr(int tricode, int dst)
    {
        if (!GetCtxrHandle || !CopyCtxr || !FreeTexture)
            return;

        uintptr_t* dstCtxr = GetCtxrHandle(tricode, dst);

        if (dstCtxr == nullptr)
            return;

        uintptr_t dstHandle = dstCtxr[4];

        if (!dstHandle)
            return;

        for (auto it = SwapMap.begin(); it != SwapMap.end(); it++)
        {
            if (it->second != dstHandle)
                continue;

            auto swap = *it;
            SwapMap.erase(it);

            if (swap.first && swap.second)
            {
                CopyCtxr(swap.first, swap.second);
                FreeTexture(swap.first);
            }

            break;
        }
    }

}



void TextureLiveSwaps::ApplyFixes()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    // system/libdg/xtext.c -> DG_SetMoveReplaceTexture() is called to swap texture IDs within a tri at runtime in three contexts:
    // - Shaved Snake
    // - Harrier Damage
    // - RAY Numbers
    // All three of these are broken in the HD Collection, but Shaved Snake was fixed in the Master Collection...
    // with a function Bluepoint already developed for wig textures (thanks Gaming With Portals for the find).
    uint8_t* ShavedSnakeCode = Memory::PatternScan(baseModule, "?? ?? ?? ?? 48 8B CD E8 ?? ?? ?? ?? ?? ?? ?? 48 8B CD 48 8B D8", "Texture Swaps (Shaved Snake)");
    if (!ShavedSnakeCode) 
    {
        spdlog::error("Texture Swaps: Failed to match Shaved Snake reference location. Aborting.");
        return;
    }
    GetCtxrHandle = (FASTCALL_2IN1OUT)Memory::GetRelativeOffset(ShavedSnakeCode + 8);
    CopyCtxr = (FASTCALL_2IN1OUT)Memory::GetRelativeOffset(ShavedSnakeCode + 79);

    // user/takabe/object/eddogtag.c -> CreateDogTagTexture() uses the same copy function, as well as necessary allocation/freeing for additional texture handles.
    // (technically Shaved Snake should have done this too, but it's a non-issue since he never needs to be reset to un-shaved within the stage)
    uint8_t* DogTagCode = Memory::PatternScan(baseModule, "48 8B CF E8 ?? ?? ?? ?? 8B 57 ?? 45 33 C0", "Texture Swaps (Dog Tags)");
    if (!DogTagCode) 
    {
        spdlog::error("Texture Swaps: Failed to match dog tag reference location. Aborting.");
        return;
    }
    AllocTexture = (FASTCALL_3IN1OUT)Memory::GetRelativeOffset(DogTagCode + 18);
    FreeTexture = (FASTCALL_1IN1OUT)Memory::GetRelativeOffset(DogTagCode + 64);
    if ((uintptr_t)CopyCtxr != Memory::GetRelativeOffset(DogTagCode + 56)) 
    {
        spdlog::warn("Texture Swaps: Mismatched texture copy function - false positive match?");
    }

    // Hook problems

    {   // user/takabe/pdray/r_server.c -> RAYSERVER_GetNumberModel()
        MAKE_HOOK_MID(baseModule, "B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 89 84 F3", "Texture Swaps (RAY Numbers)", {
            GetAndCopyCtxr(STRCODE_PDRAY_OTHER, ctx.rdx, ctx.r8);
        });
    }
    {   // user/satoyoshi/harrier/har_damage.c -> Har_damage_tex()
        MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 8B C8 48 89 84 DE", "Texture Swaps (Harrier Damage)", {
            GetAndCopyCtxr(ctx.rcx, ctx.rdx, ctx.r8);
        });
    }
    {   // user/takabe/pdray/r_server.c -> Die()
        MAKE_HOOK_MID(baseModule, "57 48 83 EC ?? 48 8D 99 ?? ?? ?? ?? BF ?? ?? ?? ?? 48 8B 4B", "Texture Swaps (RAY Cleanup)", {
            RestoreCtxrs();
        });
    }
    {   // user/satoyoshi/harrier/har_main.c -> Die() (invokes static function Clean_damage_tex_set() from har_damage.c)
        MAKE_HOOK_MID(baseModule, "48 8D 8E ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 4E ?? E8 ?? ?? ?? ?? 33 ED", "Texture Swaps (Harrier Cleanup)", {
            RestoreCtxrs();
        });
    }


    // There is another texture to swap, this one not a restoration.
    // In the Ames cutscene, Ocelot can be seen watching Raiden and Ames talk on a camera.
    // However, this texture always shows Raiden in his Sneaking Suit. This should change.

    // Requires a custom texture (should be bundled with community bugfix compilation)
#define EU_JP(X) (exists(sExePath / "eu" / X) && exists(sExePath / "jp" / X))

#define TEXTURE_EU_JP(X) (exists(sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_eu" / X) \
                          && exists(sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_jp" / X))

    if (BP_FilesysChanges::bLoaded && //linux filesystems are stupid and shit reliant on expand_bp_assets can randomly cause crashing.
        exists(sExePath / "assets" / "tri" / "us" / "spacecore_w24c1.tri")
        && TEXTURE_EU_JP("_win" / "spacecore_w24c1_rev_disp_02.bmp.ctxr")
        && EU_JP("stage" / "d036p03" / "manifest_cbfc_hostage_fixes.txt")
        && EU_JP("stage" / "d036p03" / "bp_assets_cbfc_hostage_fixes.txt"))
    {
        spdlog::info("MGS 2 - Texture Swaps: w24c screen texture fix enabled.");
        // user/mode/demo/demod.c -> StartDemo()
        MAKE_HOOK_MID(baseModule, "48 83 EC 28 48 8B 15 ?? ?? ?? ?? 48 85 D2 74 53", "Texture Swaps (Ocelot Spying)", {
            if (g_GameVars.IsStage(MGS2Stages::D036P03) && MGS2_LinkVarBuf::GM_Item == MGS2_ITEM_INDEX_UNIFORM) {
                GetAndCopyCtxr(STRCODE_W24C1, STRCODE_REV_DISP, STRCODE_BDU_DISP, false);
            }
        });

    }
    else
    {
        spdlog::warn("MGS 2 - Texture Swaps: MGS2 Community Bugfix Compilation files missing. Skipping w24c screen texture fix.");
    }
    
    if (bRestoreTitleScreenSwapping
        && TEXTURE_EU_JP("_win" / "00c1181b.ctxr") //blue 2
        && TEXTURE_EU_JP("_win" / "0007bc34.ctxr")) //red 2
    {
        spdlog::info("MGS 2 - Texture Swaps: Title screen fix enabled.");
        MAKE_HOOK_MID(baseModule, "89 83 ?? ?? ?? ?? 8B 4B ?? 45 33 C9 45 33 C0 41 8D 51 ?? E8 ?? ?? ?? ?? 89 43 ?? 48 8D 3D", "NewTitleScrMan", {
            if(ctx.rax)
            {
                GetAndCopyCtxr(STRCODE_NODE_TITLE_TEX_TRI, STRCODE_TITLE_LOGO, STRCODE_TITLE_NUMBAH_TWO);
                return;
            }
            RestoreCtxr(STRCODE_NODE_TITLE_TEX_TRI, STRCODE_TITLE_LOGO);
            });

    }
    else
    {
        if (bRestoreTitleScreenSwapping)
        {
            spdlog::warn("MGS 2 - Texture Swaps: Title screen fix enabled but MGS2 Community Bugfix Compilation not installed. Skipping title screen fix.");
        }
        else
        {
            spdlog::warn("MGS 2 - Texture Swaps: MGS2 Community Bugfix Compilation files missing. Skipping title screen fix.");
        }
    }


    if (BP_FilesysChanges::bLoaded && //linux filesystems are stupid and shit reliant on expand_bp_assets can randomly cause crashing.
        EU_JP("stage" / "r_sna_b" / "bp_assets_holster_fix.txt") &&
        EU_JP("stage" / "r_tnk0" / "bp_assets_holster_fix.txt") &&
        EU_JP("stage" / "r_plt_s" / "bp_assets_holster_fix.txt") &&
        EU_JP("stage" / "r_vr_s" / "bp_assets_holster_fix.txt") &&
        EU_JP("stage" / "r_vr_sp" / "bp_assets_holster_fix.txt"))
    {
        using namespace MGS2_Characters;


        constexpr uint32_t NULL_MSK_STRCODE = GameVars::GV_StrCode("null_msk.bmp");
        constexpr uint32_t SNA_DEF_TRI_STRCODE = 0x55aab1;
        constexpr uint32_t SNA_M9_GLIP = 0x495322;

        constexpr int M92_EQUIPPED = 0;
        constexpr int USP_EQUIPPED = 1;
        constexpr int USP_SP_EQUIPPED = 9;

        static bool bSnakeHolsterManaged = false;
        MAKE_HOOK_MID(baseModule, "48 21 05 ?? ?? ?? ?? 48 8B 86", "mgs2x\\source\\user\\skoba\\weapon\\usp.c -> Act() -> @ l747", {
                static bool failed = false;
                if (failed)
                {
                    return;
                }
                if (!IsCurrentlyCharacter(PlayerCharacter::NormalSnake))
                {
                    return;
                }
                if (bSnakeHolsterManaged)
                {
                    if (g_GameVars.InCutscene())
                    {
                        bSnakeHolsterManaged = false;
                        RestoreCtxr(SNA_DEF_TRI_STRCODE, SNA_M9_GLIP);
                    }
                    return;
                }
                if (ctx.r15 == M92_EQUIPPED || ctx.r15 == USP_EQUIPPED || ctx.r15 == USP_SP_EQUIPPED) // && (MGS2_GameFuncs::GM_WeaponNum(MGS2_WEAPON_INDEX_USP) < 1)))
                {
                    if (g_GameVars.InCutscene())
                    {
                        return;
                    }
                    if (!GetAndCopyCtxr(SNA_DEF_TRI_STRCODE, SNA_M9_GLIP, NULL_MSK_STRCODE))
                    {
                        //spdlog::error("MGS2 - Texture Swaps: Failed to swap Snake holster texture.");
                        failed = true;
                        return;
                    }
                    bSnakeHolsterManaged = true;
                }
                      });

        MAKE_HOOK_MID(baseModule, "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 48 8B D9 48 8B 0D", "mgs2x\\source\\user\\skoba\\weapon\\usp.c -> Die()", {
                if (!bSnakeHolsterManaged)
                {
                    return;
                }
                if (!IsCurrentlyCharacter(PlayerCharacter::NormalSnake))
                {
                    //Snake holster texture swap active but player is not Snake???????????
                    return;
                }
                bSnakeHolsterManaged = false;
                RestoreCtxr(SNA_DEF_TRI_STRCODE, SNA_M9_GLIP);
                      });
    }


}
