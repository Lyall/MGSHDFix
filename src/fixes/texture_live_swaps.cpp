#include "stdafx.h"

#include "texture_live_swaps.hpp"

#include "common.hpp"
#include "logging.hpp"

typedef uintptr_t* (__fastcall* FASTCALL_2IN1OUT)(long long, long long);
#define STRCODE_PDRAY_OTHER 2151908

namespace
{
    FASTCALL_2IN1OUT GetCtxrHandle;
    FASTCALL_2IN1OUT CopyCtxr;
}


static void GetAndCopyCtxr(int tricode, int dst, int src) {
    if (!GetCtxrHandle || !CopyCtxr)
        return;
    uintptr_t* srcHandle = GetCtxrHandle(tricode, src);
    uintptr_t* dstHandle = GetCtxrHandle(tricode, dst);
    //CopyCtxr(dstHandle[4], srcHandle[4]);
    CopyCtxr(srcHandle[4], dstHandle[4]);
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
    // Because Konami has generously provided a fix to the first in the Master Collection (user/sonoyama/etc/shavedsnake.c -> MakeTexReplace()), we can copy that.
    // The new functions appear to get a struct pointer relating to the source and destination ctxrs, then pass them both into another function to swap.
    uint8_t* NewFunctionUse = Memory::PatternScan(baseModule, "4A 63 14 36 48 8B CD E8 ?? ?? ?? ?? 49 63 16 48 8B CD 48 8B D8 E8 ?? ?? ?? ?? 48 63 57 58", "Texture Swaps (Shaved Snake)");
    if (!NewFunctionUse) {
        spdlog::error("Texture Swaps: Failed to match Shaved Snake reference location. Aborting.");
        return;
    }
    GetCtxrHandle = (FASTCALL_2IN1OUT)Memory::GetRelativeOffset(NewFunctionUse + 8);
    CopyCtxr = (FASTCALL_2IN1OUT)Memory::GetRelativeOffset(NewFunctionUse + 79);

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

}