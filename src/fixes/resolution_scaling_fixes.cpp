#include "stdafx.h"

#include"resolution_scaling_fixes.hpp"

#include "common.hpp"
#include "custom_resolution_and_borderless.hpp"
#include "gamevars.hpp"
#include "logging.hpp"
#include "mgs2_linkvarbuf.hpp"

namespace
{
    double scaleX_fromPs2 = 1.0;
    double scaleX_fromPs2_4by3 = 1.0;

    double scaleY_fromPs2 = 1.0;
}



void ResolutionScalingFixes::ApplyFixes()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    scaleX_fromPs2 = CustomResolutionAndBorderless::iInternalResX / 512.0;
    scaleX_fromPs2_4by3 = ((double)CustomResolutionAndBorderless::iInternalResX) / (double)(CustomResolutionAndBorderless::iInternalResY) / (4.0 / 3.0);
    scaleY_fromPs2 = CustomResolutionAndBorderless::iInternalResY / 448.0;

    SPDLOG_INFO("Resolution Scaling Fixes: Internal Width = {}, Internal Height = {}", CustomResolutionAndBorderless::iInternalResX, CustomResolutionAndBorderless::iInternalResY);
    SPDLOG_INFO("Resolution Scaling Fixes: PS2 Height Delta = {}, PS2 Width Delta = {}, PS2 Width 4:3 Delta = {}", scaleY_fromPs2, scaleX_fromPs2, scaleX_fromPs2_4by3);

    {   // user/mode/codec/face_bug.c -> NewBugFace() - not affected by resolution, might've just been the layout change which caused it.

        MAKE_HOOK_MID(baseModule, "48 89 83 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 8B ?? ?? ?? ?? BA", "NewBugFace", {
            ctx.xmm1.f32[0] = 3018.0f; //width | 512.0f original
            ctx.xmm2.f32[0] = 860.4896f;    //height | 384.0f original | 872.0f was a little over in case there's some effect i missed at the bottom of the sprite. 860.4896f = pixel perfect in photoshop
        });
    }

    MAKE_HOOK_MID(baseModule, "F3 0F 11 74 24 ?? E8 ?? ?? ?? ?? 81 67", "MGS2: Resolution Scaling Fixes : user\\kira\\radar\\bomb_sensor.c -> setup_bomb_object() - Scale y", {
        ctx.xmm6.f32[0] *= 0.75f;

                  });

    MAKE_HOOK_MID(baseModule, "81 E7 ?? ?? ?? ?? BE ?? ?? ?? ?? BA", "NewLinerGunPlasma demo check", {
        ctx.rdi = 0;  // force demo check false for more nodes
                  });


    
    MAKE_HOOK_MID(baseModule, "?? ?? ?? ?? F3 0F 11 48 ?? E8 ?? ?? ?? ?? 8B C6 8B D3 2B C3 83 F8 ?? 7F ?? 0F 28 74 24 ?? 48 8B 5C 24 ?? 48 8B 74 24 ?? 48 83 C4 ?? 5F C3 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? F3 0F 10 15", "MGS2: Resolution Scaling Fixes : user\\okajima\\effect2\\liner_gun_plasma.c -> CalcNextNode()", {
            if (g_GameVars.InCutscene())
            {
                ctx.xmm0.f32[0] *= (float)(2.0 * scaleX_fromPs2_4by3);
                ctx.xmm1.f32[0] *= 2.0f;
            }

                  });


    MAKE_HOOK_MID(baseModule, "66 0F 6E F1 48 8B 4E", "MGS2: user\\shibata\\effect\\2d_sprt.c -> New2dSprt() -> GetResources() @l252 | tanker MGS2 logo", {
            if (ctx.rdi != 0xA57131 || ctx.rcx != 519 || ctx.rax != 125)
            {
                //spdlog::info("rdi {}, rcx {}, rax {}", ctx.rdi, ctx.rcx, ctx.rax);
                return;
            }

            if (!(MGS2_LinkVarBuf::GM_Configuration & GM_CONFIG_CUTSCENES_LETTERBOXED))
            {
                return;
            }

            //FULLSCREEN SETTINGS
            //ctx.rcx = 519;                                        // w
            //ctx.rax = 125;                                        // h
            //*reinterpret_cast<float*>(ctx.rsp + 0x30) = 0.0f;    // pos.x
            //*reinterpret_cast<float*>(ctx.rsp + 0x34) = 20.0f;   // pos.y

            ctx.rcx = 346; // width
            ctx.rax = 83; // height
            *reinterpret_cast<float*>(ctx.rsp + 0x30) = 83.0f;    // pos.x
            *reinterpret_cast<float*>(ctx.rsp + 0x34) = 40.0f;   // pos.y
                          
                  });

    if (bIncreaseShadowResolution)
    {
        MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 89 84 FE ?? ?? ?? ?? 48 FF C7", "MGS2: shadow res", {
                if (ctx.rdi == 0x16 || ctx.rdi == 0x17 || ctx.rdi == 0x18)
                {
                    int shadowRes;
                    int resY = CustomResolutionAndBorderless::iInternalResY;
                    if (resY > 1440)
                    {
                        shadowRes = 4096;
                    }
                    else if (resY > 1080)
                    {
                        shadowRes = 2048;
                    }
                    else if (resY > 720)
                    {
                        shadowRes = 1024;
                    }
                    else
                    {
                        shadowRes = 512;
                    }

                    ctx.rcx = shadowRes;
                    ctx.rdx = shadowRes;
                }

                      });
    }



}



