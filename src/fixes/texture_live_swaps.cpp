#include "stdafx.h"

#include "texture_live_swaps.hpp"

#include "common.hpp"
#include "logging.hpp"

typedef uintptr_t* (__fastcall* FASTCALL_1IN1OUT)(long long);
typedef uintptr_t* (__fastcall* FASTCALL_2IN1OUT)(long long, long long);
typedef uintptr_t (__fastcall* FASTCALL_3IN1OUT)(long long, long long, int);
#define STRCODE_PDRAY_OTHER 2151908
#define STRCODE_HAR01 11685431

typedef struct _dontknowdontcare {
    char data[0x90];
} dontknowdontcare;

namespace
{
    FASTCALL_2IN1OUT GetCtxrHandle;
    FASTCALL_2IN1OUT CopyCtxr;
    FASTCALL_3IN1OUT AllocTexture;
    FASTCALL_1IN1OUT FreeTexture;

    std::list<std::pair<uintptr_t, uintptr_t>> SwapMap;
}


static void GetAndCopyCtxr(int tricode, int dst, int src) {
    if (!GetCtxrHandle || !CopyCtxr || !AllocTexture)
        return;
    uintptr_t srcHandle = GetCtxrHandle(tricode, src)[4];
    uintptr_t dstHandle = GetCtxrHandle(tricode, dst)[4];
    // Need to save handles to restore on stage reset
    bool shouldSave = true;
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
    uint8_t* ShavedSnakeCode = Memory::PatternScan(baseModule, "4A 63 14 36 48 8B CD E8 ?? ?? ?? ?? 49 63 16 48 8B CD 48 8B D8 E8 ?? ?? ?? ?? 48 63 57 58", "Texture Swaps (Shaved Snake)");
    if (!ShavedSnakeCode) {
        spdlog::error("Texture Swaps: Failed to match Shaved Snake reference location. Aborting.");
        return;
    }
    GetCtxrHandle = (FASTCALL_2IN1OUT)Memory::GetRelativeOffset(ShavedSnakeCode + 8);
    CopyCtxr = (FASTCALL_2IN1OUT)Memory::GetRelativeOffset(ShavedSnakeCode + 79);

    // user/takabe/object/eddogtag.c -> CreateDogTagTexture() uses the same copy function, as well as necessary allocation/freeing for additional texture handles.
    // (technically Shaved Snake should have done this too, but it's a non-issue since he never needs to be reset to un-shaved within the stage)
    uint8_t* DogTagCode = Memory::PatternScan(baseModule, "48 8B CF E8 ?? ?? ?? ?? 8B 57 74 45 33 C0 8B 4F 70 E8 ?? ?? ?? ?? 4C 8B 87 80 00 00 00 48 8B C8 48 8B 57 78 48 8B D8", "Texture Swaps (Dog Tags)");
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
        MAKE_HOOK_MID(baseModule, "B9 E4 D5 20 00 E8 ?? ?? ?? ?? 48 89 84 F3 40 06 00 00 48 85 C0", "Texture Swaps (RAY Numbers)", {
            GetAndCopyCtxr(STRCODE_PDRAY_OTHER, ctx.rdx, ctx.r8);
        });
    }
    {   // user/satoyoshi/harrier/har_damage.c -> Har_damage_tex()
        MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 8B C8 48 89 84 DE 68 01 00 00 E8 ?? ?? ?? ?? 48 8B 4D 37 48 33 CC", "Texture Swaps (Harrier Damage)", {
            GetAndCopyCtxr(ctx.rcx, ctx.rdx, ctx.r8);
        });
    }
    {   // user/takabe/pdray/r_server.c -> Die()
        MAKE_HOOK_MID(baseModule, "57 48 83 EC 20 48 8D 99 10 06 00 00 BF 06 00 00 00 48 8B 4B D0 48 85 C9 74 0E", "Texture Swaps (RAY Cleanup)", {
            RestoreCtxrs();
        });
    }
    {   // user/satoyoshi/harrier/har_main.c -> Die() (invokes static function Clean_damage_tex_set() from har_damage.c)
        MAKE_HOOK_MID(baseModule, "48 8D 8E 98 05 00 00 E8 ?? ?? ?? ?? 48 8D 4E 60 E8 ?? ?? ?? ?? 33 ED 48 8D 9E 68 01 00 00 BF 0C 00 00 00", "Texture Swaps (Harrier Cleanup)", {
            RestoreCtxrs();
        });
    }

}