#include "stdafx.h"

#include "texture_live_swaps.hpp"

#include "common.hpp"
#include "logging.hpp"

typedef uintptr_t* (__fastcall* FASTCALL_1IN1OUT)(long long);
typedef uintptr_t* (__fastcall* FASTCALL_2IN1OUT)(long long, long long);
typedef uintptr_t (__fastcall* FASTCALL_3IN1OUT)(long long, long long, int);
#define STRCODE_PDRAY_OTHER 2151908
#define STRCODE_HAR01 11685431

namespace
{
    FASTCALL_2IN1OUT GetCtxrHandle;
    FASTCALL_2IN1OUT CopyCtxr;
    FASTCALL_3IN1OUT AllocTexture;
    FASTCALL_1IN1OUT FreeTexture;

    std::list<std::pair<uintptr_t, uintptr_t>> SwapMap;
    short** GM_Item;
    char** CurArea;
}

static void GetAndCopyCtxr(int tricode, int dst, int src, bool shouldSave = true) {
    if (!GetCtxrHandle || !CopyCtxr || !AllocTexture)
        return;
    uintptr_t srcHandle = GetCtxrHandle(tricode, src)[4];
    uintptr_t dstHandle = GetCtxrHandle(tricode, dst)[4];
    // Need to save handles to restore on stage reset
    for (auto it = SwapMap.begin(); it != SwapMap.end(); it++) {
        if (it->second == dstHandle) {
            // Texture already swapped, do not add to map
            shouldSave = false;
            break;
        }
    }
    if (shouldSave) {
        // Arguments are width, height, clut, but apparently don't matter?
        uintptr_t saveHandle = AllocTexture(16, 16, 0);
        CopyCtxr(dstHandle, saveHandle);
        SwapMap.push_back({ saveHandle, dstHandle });
    }
    CopyCtxr(srcHandle, dstHandle);
}

static void RestoreCtxrs() {
    if (!CopyCtxr || !FreeTexture)
        return;
    for (auto it = SwapMap.begin(); it != SwapMap.end(); it++) {
        CopyCtxr(it->first, it->second);
        FreeTexture(it->first);
    }
    SwapMap.clear();
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
    if (!ShavedSnakeCode) {
        spdlog::error("Texture Swaps: Failed to match Shaved Snake reference location. Aborting.");
        return;
    }
    GetCtxrHandle = (FASTCALL_2IN1OUT)Memory::GetRelativeOffset(ShavedSnakeCode + 8);
    CopyCtxr = (FASTCALL_2IN1OUT)Memory::GetRelativeOffset(ShavedSnakeCode + 79);

    // user/takabe/object/eddogtag.c -> CreateDogTagTexture() uses the same copy function, as well as necessary allocation/freeing for additional texture handles.
    // (technically Shaved Snake should have done this too, but it's a non-issue since he never needs to be reset to un-shaved within the stage)
    uint8_t* DogTagCode = Memory::PatternScan(baseModule, "48 8B CF E8 ?? ?? ?? ?? 8B 57 ?? 45 33 C0", "Texture Swaps (Dog Tags)");
    if (!DogTagCode) {
        spdlog::error("Texture Swaps: Failed to match dog tag reference location. Aborting.");
        return;
    }
    AllocTexture = (FASTCALL_3IN1OUT)Memory::GetRelativeOffset(DogTagCode + 18);
    FreeTexture = (FASTCALL_1IN1OUT)Memory::GetRelativeOffset(DogTagCode + 64);
    if ((uintptr_t)CopyCtxr != Memory::GetRelativeOffset(DogTagCode + 56)) {
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
    // However, this texture shows Raiden in his Sneaking Suit.
    
    // Get GM_Item the same as with CoolantMirrorFix (user/morita/orga/orga_dsp.c -> ORG_DispMouthAnim())
    uint8_t* OlgaMouthDisp = Memory::PatternScan(baseModule, "F7 81 E0 13 00 00 00 00 00 08 75 3A 48 8B 05", "Texture Swaps (Item Check)");
    // Get the current stage (compare to d036p03) from game/area.c -> GM_GetArea()
    uint8_t* GetArea = Memory::PatternScan(baseModule, "83 3D 11 ?? ?? ?? ?? 48 8B 05", "Texture Swaps (Current Area)");
    // Also, requires a custom texture (should be bundled with community bugfix compilation)
    if (exists(sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_eu" / "_win" / "spacecore_w24c1_rev_disp_02.bmp.ctxr")
        && OlgaMouthDisp && GetArea) {

        GM_Item = (short**)Memory::GetRelativeOffset(OlgaMouthDisp + 15);
        CurArea = (char**)Memory::GetRelativeOffset(GetArea + 10);

        // user/mode/demo/demod.c -> StartDemo()
        MAKE_HOOK_MID(baseModule, "48 83 EC 28 48 8B 15 ?? ?? ?? ?? 48 85 D2 74 53", "Texture Swaps (Ocelot Spying)", {
            if (!strcmp("d036p03", CurArea[0] + 0x2c) && GM_Item[0][0x83] == 6) {
                GetAndCopyCtxr(0x89dc98, 0x4c4dfd, 0x20158d, false);
            }
        });

    }

}