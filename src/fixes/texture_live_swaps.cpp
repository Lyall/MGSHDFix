#include "stdafx.h"

#include "texture_live_swaps.hpp"

#include "common.hpp"
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

    FASTCALL_2IN1OUT GetCtxrHandle;
    FASTCALL_2IN1OUT CopyCtxr;
    FASTCALL_3IN1OUT AllocTexture;
    FASTCALL_1IN1OUT FreeTexture;

    std::list<std::pair<uintptr_t, uintptr_t>> SwapMap;


    void GetAndCopyCtxr(int tricode, int dst, int src, bool shouldSave = true)
    {
        if (!GetCtxrHandle || !CopyCtxr || !AllocTexture)
            return;
        uintptr_t srcHandle = GetCtxrHandle(tricode, src)[4];
        uintptr_t dstHandle = GetCtxrHandle(tricode, dst)[4];
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
            CopyCtxr(dstHandle, saveHandle);
            SwapMap.push_back({ saveHandle, dstHandle });
        }
        CopyCtxr(srcHandle, dstHandle);
    }

    void RestoreCtxrs()
    {
        if (!CopyCtxr || !FreeTexture)
            return;
        for (auto it = SwapMap.begin(); it != SwapMap.end(); it++)
        {
            CopyCtxr(it->first, it->second);
            FreeTexture(it->first);
        }
        SwapMap.clear();
    }


    int menu_view_count = 0;
}




void TextureLiveSwaps::HandleLevelTransition()
{
    if (!g_GameVars.IsStage(MGS2Stages::N_TITLE))
    {
        menu_view_count = 0;
        //spdlog::info("reset count");
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
    // TODO: manifest check (similar to mgs2_hostage_model and mgs2_msx_colonel)
    if (exists(sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_eu" / "_win" / "spacecore_w24c1_rev_disp_02.bmp.ctxr") 
        && exists(sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_jp" / "_win" / "spacecore_w24c1_rev_disp_02.bmp.ctxr")) 
    {
        spdlog::info("MGS 2 - Texture Swaps: w24c screen texture fix enabled.");
        // user/mode/demo/demod.c -> StartDemo()
        MAKE_HOOK_MID(baseModule, "48 83 EC 28 48 8B 15 ?? ?? ?? ?? 48 85 D2 74 53", "Texture Swaps (Ocelot Spying)", {
            if (g_GameVars.IsStage(MGS2Stages::D036P03) && MGS2_LinkVarBuf::GM_Item == MGS2_ITEM_INDEX_UNIFORM) {
                GetAndCopyCtxr(STRCODE_W24C1, STRCODE_REV_DISP, STRCODE_BDU_DISP, false);
            }
        });

    }
    
    if (bRestoreTitleScreenSwapping
        && (exists(sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_eu" / "_win" / "00c1181b.ctxr") && exists(sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_jp" / "_win" / "00c1181b.ctxr")) //blue 2
        && (exists(sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_eu" / "_win" / "0007bc34.ctxr") && exists(sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_jp" / "_win" / "0007bc34.ctxr"))) //red 2
    {
        spdlog::info("MGS 2 - Texture Swaps: Title screen fix enabled.");
        MAKE_HOOK_MID(baseModule, "48 89 5C 24 ?? 57 48 83 EC ?? 45 33 C9 8B F9 BA ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 41 8D 49 ?? E8 ?? ?? ?? ?? 48 8B D8 48 85 C0 0F 84 ?? ?? ?? ?? 48 89 74 24 ?? 4C 8D 05 ?? ?? ?? ?? 33 F6 89 78 ?? 48 8D 15 ?? ?? ?? ?? 89 B0", "NewTitleScrMan", {
            if (((MGS2_LinkVarBuf::GM_GameClearCount.get() & 1) != 0) && !(menu_view_count & 1))
            {
                GetAndCopyCtxr(STRCODE_NODE_TITLE_TEX_TRI, STRCODE_TITLE_LOGO, STRCODE_TITLE_NUMBAH_TWO);
                //spdlog::info("Title screen texture swap applied. Menu view count: {}", menu_view_count);
            }
            else
            {
                RestoreCtxrs();
            }
            ++menu_view_count;
            });

    }

}
