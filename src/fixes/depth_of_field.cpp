#include "depth_of_field.hpp"

#include "common.hpp"
#include "config.hpp"
#include "logging.hpp"


void DepthOfFieldFixes::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? F6 C3 ?? 74 ?? 41 0F 28 D9 41 0F 28 CA EB ?? 0F 28 DF 41 0F 28 C8 F6 C3 ?? 74 ?? 41 0F 28 D3 41 0F 28 C4 EB ?? 0F 28 D7 41 0F 28 C0 C7 44 24 ?? ?? ?? ?? ?? F3 0F 58 C2 F3 0F 58 CB 48 8D 4F ?? F3 0F 11 44 24 ?? F3 0F 11 4C 24 ?? 0F 28 CF F3 44 0F 11 6C 24 ?? F3 44 0F 11 74 24 ?? F3 0F 11 54 24 ?? 0F 28 D7 E8 ?? ?? ?? ?? 48 81 C7", "dof test near", {
        spdlog::info("ctx.xmm1.f32[0] before = {:.6g}", ctx.xmm1.f32[0]);
        ctx.xmm1.f32[0] *= (float)iInternalResY / 448.0f;
        spdlog::info("ctx.xmm1.f32[0] after = {:.6g}", ctx.xmm1.f32[0]);
        })

        MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? F6 C3 ?? 74 ?? 41 0F 28 D9 41 0F 28 CA EB ?? 0F 28 DF 41 0F 28 C8 F6 C3 ?? 74 ?? 41 0F 28 D3 41 0F 28 C4 EB ?? 0F 28 D7 41 0F 28 C0 C7 44 24 ?? ?? ?? ?? ?? F3 0F 58 C2 F3 0F 58 CB 48 8D 4F ?? F3 0F 11 44 24 ?? F3 0F 11 4C 24 ?? 0F 28 CF F3 44 0F 11 6C 24 ?? F3 44 0F 11 74 24 ?? F3 0F 11 54 24 ?? 0F 28 D7 E8 ?? ?? ?? ?? 48 83 C7", "dof test far", {
        ctx.xmm1.f32[0] *= (float)iInternalResY / 448.0f;
            })

   /* uintptr_t testXres = Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "F3 0F 10 35 ?? ?? ?? ?? 0F 57 D2 F3 0F 10 3D ?? ?? ?? ?? 0F 28 DE C1 E3", "MGS 2: GameVars: 1280 resolution") + 4);
    Memory::PatchBytes(testXres, "\x00\x00\x70\x45", sizeof(float));
    uintptr_t testYrex = Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "F3 0F 10 3D ?? ?? ?? ?? 0F 28 DE C1 E3", "MGS 2: GameVars: 720 resolution") + 4);
    Memory::PatchBytes(testYrex, "\x00\x00\x07\x45", sizeof(float));
    spdlog::info("testXres = {}", *reinterpret_cast<float*>(testXres));
    spdlog::info("testYrex = {}", *reinterpret_cast<float*>(testYrex));*/

    /*
    MAKE_HOOK_MID(baseModule, "66 0F 1F 84 00 ?? ?? ?? ?? 8B 4E ?? 41 0F 28 F0 F3 0F 10 5E ?? 66 0F 6E C3 F3 0F 5C 5E ?? 8D 41 ?? 66 0F 6E C8 0F 5B C0 0F 5B C9 F3 0F 59 D8 0F 28 C7 F3 0F 5E D9 F3 0F 58 5E ?? F3 0F 5F C3 F3 0F 5D F0 3B D9 0F 8D ?? ?? ?? ?? 41 0F 2E F0 7A ?? 0F 84 ?? ?? ?? ?? 0F 2E F7 7A ?? 0F 84 ?? ?? ?? ?? BA", "dof test near", {
        ctx.xmm9.f32[0] = 1.0f / static_cast<float>(iInternalResX);
        //ctx.xmm10.f32[0] = 1.0f + (1.0f / static_cast<float>(iInternalResX));
        ctx.xmm11.f32[0] = 1.0f / static_cast<float>(iInternalResY);
        //ctx.xmm12.f32[0] = 1.0f + (1.0f / static_cast<float>(iInternalResY));
        //ctx.xmm13.f32[0] = static_cast<float>(iInternalResY);
        //ctx.xmm14.f32[0] = static_cast<float>(iInternalResX);
    })

    MAKE_HOOK_MID(baseModule, "66 0F 1F 84 00 ?? ?? ?? ?? 8B 4E ?? 41 0F 28 F0 F3 0F 10 5E ?? 66 0F 6E C3 F3 0F 5C 5E ?? 8D 41 ?? 66 0F 6E C8 0F 5B C0 0F 5B C9 F3 0F 59 D8 0F 28 C7 F3 0F 5E D9 F3 0F 58 5E ?? F3 0F 5F C3 F3 0F 5D F0 3B D9 0F 8D ?? ?? ?? ?? 41 0F 2E F0 7A ?? 0F 84 ?? ?? ?? ?? 0F 2E F7 7A ?? 0F 84 ?? ?? ?? ?? 48 8D 4F", "dof test far", {
        ctx.xmm9.f32[0] = 1.0f / static_cast<float>(iInternalResX);
        //ctx.xmm10.f32[0] = 1.0f + (1.0f / static_cast<float>(iInternalResX));
        ctx.xmm11.f32[0] = 1.0f / static_cast<float>(iInternalResY);
        //ctx.xmm12.f32[0] = 1.0f + (1.0f / static_cast<float>(iInternalResY));
        //ctx.xmm13.f32[0] = static_cast<float>(iInternalResY);
        //ctx.xmm14.f32[0] = static_cast<float>(iInternalResX);
        })
    /*
    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 83 C7 ?? FF C3", "MGS2: Depth of Field loc 1", {
        spdlog::info("dof loc 1");
        ctx.xmm14.f32[0] = 3840.0f; // 3840.0f
        ctx.xmm13.f32[0] = 2160.0f; // 2160.0f
    });
    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 81 C7 ?? ?? ?? ?? FF C3 83 FB", "MGS2: Depth of Field loc 2", {
        spdlog::info("dof loc 2");
        ctx.xmm14.f32[0] = 3840.0f; // 3840.0f
        ctx.xmm13.f32[0] = 2160.0f; // 2160.0f
        });

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? BA ?? ?? ?? ?? 48 8B C8 E8 ?? ?? ?? ?? 48 8B C8", "MGS2: blur 1", {
    //spdlog::info("blur 1");
        ctx.xmm6.f32[0] = 3840.0f; // 3840.0f
        ctx.xmm7.f32[0] = 2160.0f; // 2160.0f
        });


    MAKE_HOOK_MID(baseModule, "76 ?? 89 93 ?? ?? ?? ?? EB ?? 0F 2F F0", "MGS2: MGS2_Resolution_Conversion", {
        spdlog::info("MGS2_Resolution_Conversion");
        Util::DumpContext(ctx);
        });
        */
    /*
    MAKE_HOOK_MID(baseModule, "66 41 89 42 ?? F3 0F 2C 44 24 ?? F3 0F 59 DA 66 41 89 42 ?? F3 0F 59 CA F3 0F 2C C3 66 41 89 42 ?? F3 0F 2C C0 F3 0F 10 44 24 ?? 66 41 89 42 ?? F3 0F 2C C1 F3 0F 59 C2 66 41 89 42 ?? F3 0F 2C C0 66 41 89 42 ?? 49 8D 42 ?? C3 CC 8B 44 24 ?? 4C 8B D1 F3 0F 10 44 24 ?? 8B D0 C1 EA", "MGS3: Depth of Field loc 1", {
       // spdlog::info("x axis {}", ctx.rax);
        if (ctx.rax == 512)// || ctx.rax == 1280)
        {
            ctx.rax = iInternalResX;
            spdlog::info("MGS3: Depth of Field - X axis set to {}", iInternalResX);
        }

    });
    MAKE_HOOK_MID(baseModule, "66 41 89 42 ?? F3 0F 59 CA F3 0F 2C C3 66 41 89 42 ?? F3 0F 2C C0 F3 0F 10 44 24 ?? 66 41 89 42 ?? F3 0F 2C C1 F3 0F 59 C2 66 41 89 42 ?? F3 0F 2C C0 66 41 89 42 ?? 49 8D 42 ?? C3 CC 8B 44 24 ?? 4C 8B D1 F3 0F 10 44 24 ?? 8B D0 C1 EA", "MGS3: Depth of Field loc 1", {
        if (ctx.rax == 448)// || ctx.rax == 720)
        {
            ctx.rax = iInternalResY;
            spdlog::info("MGS3: Depth of Field - Y axis set to {}", iInternalResY);
        }

        });
        */
}



