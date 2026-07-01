#include "stdafx.h"

#include"resolution_scaling_fixes.hpp"

#include "common.hpp"
#include "custom_resolution_and_borderless.hpp"
#include "gamevars.hpp"
//#include "input_handler.hpp"
#include "game_funcs.hpp"
#include "logging.hpp"
#include "mgs2_linkvarbuf.hpp"

namespace
{
    double scaleX_fromPs2 = 1.0;
    double scaleX_fromPs2_4by3 = 1.0;
    double scaleY_fromPs2 = 1.0;


    FVECTOR* g_laserPoints = nullptr;
    //int g_laserEditIndex = 2;
    //float g_laserStep = 0.5f;
    //FVECTOR g_laserPointsBackup[20];

    FVECTOR raiden_laserpoints[20] = {
        { 16.5F,     -259.0F,   28.70F, 0.0F },     //  [0]  m92
        { 17.5F,     -370.0F,   69.7F,  0.0F },     //  [1]  m92_sub    //fpv
        { 17.0F,     -271.0F,   8.0F,   0.0F },     //  [2]  usp
        { 17.0F,     -250.0F,   67.0F,  0.0F },     //  [3]  usp_sub    //fpv
        { 24.5F,     -267.5F,   50.2F,  0.0F },     //  [4]  scm
        { 17.5F,     -274.0F,   78.7F,  0.0F },     //  [5]  scm_sub    //fpv
        { 17.0F,     -500.0F,   57.0F,  0.0F },     //  [6]  fms
        { 17.0F,     -500.0F,   17.0F,  0.0F },     //  [7]  fms_sub    //fpv
        { 17.5F,     -280.0F,   44.7F,  0.0F },     //  [8]  spp
        { 17.5F,     -280.0F,   70.7F,  0.0F },     //  [9]  spp_sub    //fpv
        { 20.0F,     -438.0F,   70.5F,  0.0F },     //  [10] aks_sp
        { 20.0F,     -538.0F,   57.0F,  0.0F },     //  [11] aks_sp_sub //fpv
        { 46.5F,     -538.0F,   92.0F,  0.0F },     //  [12] m4
        { 47.0F,     -538.0F,   92.0F,  0.0F },     //  [13] m4_sub     //fpv
        { 19.0F,     -719.0F,   41.0F,  0.0F },     //  [14] aks_sp
        { 20.0F,     -538.0F,   57.0F,  0.0F },     //  [15] aks_sp_sub //fpv
        { 24.5F,     -269.5F,   49.2F,  0.0F },     //  [16] scm_sp
        { 17.5F,     -481.0F,   78.7F,  0.0F },     //  [17] scm_sp_sub //fpv
        { 17.0F,     -271.0F,   8.0F,   0.0F },     //  [18] usp_sp
        { 17.0F,     -250.0F,   67.0F,  0.0F },     //  [19] usp_sp_sub //fpv
    };

    constexpr FVECTOR m92_snake_override = { 17.5F, -291.5F, 29.7F, 0.0F }; // m92 3rd person w/ snake
    constexpr FVECTOR m92_sub_corrected = { 19.5F, -222.0F, 31.7F, 0.0F }; //  m92 fpv

    void UpdateCharacterLaserPoints()
    {
        if (!g_laserPoints)
        {
            return;
        }
        g_laserPoints[0] = MGS2_Characters::IsRaiden() ? raiden_laserpoints[0] : m92_snake_override;
    }

    constexpr float RAYEYE_TAILSSHIFT_Y_OFFSET = 900.0f;
    constexpr float RAYEYE_TAILSSHIFT_X_OFFSET = 3800.0f;

    constexpr FVECTOR rayEyeTailsShift_Original[8] = {
        {  511.0F, 1039.0F, 2485.0F, 1.0F },
        {  571.0F, 1073.0F, 2335.0F, 1.0F },
        {  571.0F, 1073.0F, 2335.0F, 1.0F },
        {  713.0F, 1152.0F, 1970.0F, 1.0F },
        { -511.0F, 1039.0F, 2485.0F, 1.0F },
        { -571.0F, 1073.0F, 2335.0F, 1.0F },
        { -571.0F, 1073.0F, 2335.0F, 1.0F },
        { -713.0F, 1152.0F, 1970.0F, 1.0F },
    };

    constexpr FVECTOR rayEyeTailsShift_Corrected[8] = {
        {  511.0F-RAYEYE_TAILSSHIFT_X_OFFSET, 1039.0F + RAYEYE_TAILSSHIFT_Y_OFFSET, 2485.0F, 1.0F },
        {  571.0F-RAYEYE_TAILSSHIFT_X_OFFSET, 1073.0F + RAYEYE_TAILSSHIFT_Y_OFFSET, 2335.0F, 1.0F },
        {  571.0F-RAYEYE_TAILSSHIFT_X_OFFSET, 1073.0F + RAYEYE_TAILSSHIFT_Y_OFFSET, 2335.0F, 1.0F },
        {  713.0F-RAYEYE_TAILSSHIFT_X_OFFSET, 1152.0F + RAYEYE_TAILSSHIFT_Y_OFFSET, 1970.0F, 1.0F },
        { -511.0F-RAYEYE_TAILSSHIFT_X_OFFSET, 1039.0F + RAYEYE_TAILSSHIFT_Y_OFFSET, 2485.0F, 1.0F },
        { -571.0F-RAYEYE_TAILSSHIFT_X_OFFSET, 1073.0F + RAYEYE_TAILSSHIFT_Y_OFFSET, 2335.0F, 1.0F },
        { -571.0F-RAYEYE_TAILSSHIFT_X_OFFSET, 1073.0F + RAYEYE_TAILSSHIFT_Y_OFFSET, 2335.0F, 1.0F },
        { -713.0F-RAYEYE_TAILSSHIFT_X_OFFSET, 1152.0F + RAYEYE_TAILSSHIFT_Y_OFFSET, 1970.0F, 1.0F },
    };

    FVECTOR* g_rayEyeTailsShift = nullptr;

}


void ResolutionScalingFixes::HandleLevelTransition()
{
    UpdateCharacterLaserPoints();
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


    MAKE_HOOK_MID(baseModule, "66 0F 6E F1 48 8B 4E", "MGS2: user\\shibata\\effect\\2d_sprt.c -> New2dSprite() -> GetResources() @l252 | tanker MGS2 logo", {
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

    if (uint8_t* scan = Memory::PatternScan(baseModule, "41 0F 28 84 CC ?? ?? ?? ?? F3 0F 10 2D", "LaserPoints"))
    {
        uint32_t rva = *reinterpret_cast<uint32_t*>(scan + 5);
        g_laserPoints = reinterpret_cast<FVECTOR*>((uintptr_t)baseModule + rva);
        spdlog::info("MGS2_LaserPoints: Weapon LaserPoints table found at {:s}+{:X}", sExeName.c_str(), rva);
        memcpy(g_laserPoints, raiden_laserpoints, sizeof(raiden_laserpoints));
        spdlog::info("MGS2_LaserPoints: Weapon laser point origins corrected.");
        if (bFixM92FPV)
        {
            spdlog::info("MGS2_LaserPoints: M92 FPV laser point fix enabled in config - applying.");
            g_laserPoints[1] = m92_sub_corrected;
        }
        //memcpy(g_laserPointsBackup, g_laserPoints, sizeof(g_laserPointsBackup));
        //spdlog::info("MGS2_LaserPoints: Backed up {} entries.", 20);
    }
    else
    {
        spdlog::error("MGS2_LaserPoints: Weapon LaserPoints table not found.");
    }

    if (uint8_t* scan = Memory::PatternScan(baseModule, "00 80 FF 43 00 E0 81 44 00 50 1B 45 00 00 80 3F 00 C0 0E 44 00 20 86 44 00 F0 11 45 00 00 80 3F", "RayEyeTailsShift"))
    {
        g_rayEyeTailsShift = reinterpret_cast<FVECTOR*>(scan);
        spdlog::info("MGS2_RayEyeTailsShift: TailsShift table found at {:s}+{:X}", sExeName.c_str(), scan - (uint8_t*)baseModule);

        MAKE_HOOK_MID(baseModule, "48 8B C4 48 89 58 ?? 55 56 57 41 54 41 55 41 56 41 57 48 8D 68 ?? 48 81 EC ?? ?? ?? ?? 0F 29 70 ?? 0F 29 78 ?? 44 0F 29 40 ?? 44 0F 29 48 ?? 44 0F 29 54 24", "MGS2: ray_eye.c -> InitTailsData()", {
            if (!(MGS2_LinkVarBuf::GM_Configuration & GM_CONFIG_CUTSCENES_LETTERBOXED))
            {
                memcpy(g_rayEyeTailsShift, rayEyeTailsShift_Corrected, sizeof(rayEyeTailsShift_Corrected));
                //spdlog::info("MGS2_RayEyeTailsShift: Ray eye tail height corrected.");
            }
            else
            {
                memcpy(g_rayEyeTailsShift, rayEyeTailsShift_Original, sizeof(rayEyeTailsShift_Original));
                //spdlog::info("MGS2_RayEyeTailsShift: Ray eye tail height restored.");
            }
                      });
    }
    else
    {
        spdlog::error("MGS2_RayEyeTailsShift: TailsShift table not found. Ray cutscene eye fix not applied.");
    }


    struct RouteVoiceEntry {
        short route;
        short point;
        int strm_code;
        int strm_time;
    };


    MAKE_HOOK_MID(baseModule, "0F 8D ?? ?? ?? ?? 0F 57 C0 48 63 C6", "GetRouteVoice()", {
        if (!(g_GameVars.IsStage(MGS2Stages::W03A) || g_GameVars.IsStage(MGS2Stages::A03A)))
        {
            return;
        }
        auto* arr = reinterpret_cast<RouteVoiceEntry*>(ctx.r14 - 4);
        int count = static_cast<int>(ctx.rsi);

        for (int i = 0; i < count; i++)
        {
            if (arr[i].route == 15 && arr[i].point == 3)
            {
                //spdlog::info("corrected COM_CallRouteVoice: Changing route 15 point 3 (vc045003) to point 4");
                arr[i].point = 4;
            }
        }
                  });

    //todo: NewUSPLight -> light_offset: convert to per-weapon offset.
        #define	SHIFT3_X			(17.5f)
        #define	SHIFT4_X			(0.0f)

    MAKE_HOOK_MID(baseModule, "75 ?? 81 4A ?? ?? ?? ?? ?? EB ?? 83 BB", "sonoyama\\raiden\\rai_equip.c -> RaiHolsterAct() -> @ l160, Raiden M9 holster fix", {
            if (ctx.rsi == MGS2_WEAPON_INDEX_M9)
            {
                reghelpers::SetZF(ctx, true);
            }
        });

}



#pragma region UnusedLaserDebuggingCode

    /*
    const char* kLaserPointNames[20] = {
    "m92",
    "m92_sub",
    "usp",  //x=17.00 y=-271.00 z=8.00
    "usp_sub", //fpv
    "scm", //[Laser][4 | scm] x=24.50 y=-267.50 z=50.20
    "scm_sub", //fpv
    "fms",
    "fms_sub", //fpv
    "spp",
    "spp_sub", //fpv
    "aks_sp", //[10 | aks_sp] -> rai unsup | x=20.00 y=-438.00 z=70.50
    "aks_sp_sub", //fpv
    "m4", //x=46.50 y=-538.00 z=92.00
    "m4_sub", //fpv
    "aks_sp", //[14 | aks_sp] rai sub x=19.00 y=-719.00 z=41.00
    "aks_sp_sub", //fpv
    "scm_sp",  //[16 | scm_sp] x=24.50 y=-269.50 z=49.20
    "scm_sp_sub", //fpv
    "usp_sp",                       ///todo
    "usp_sp_sub", //fpv
    };
    */



        /*
    auto logLaser = []()
        {
            if (!g_laserPoints) return;
            auto& e = g_laserPoints[g_laserEditIndex];
            const char* name = (g_laserEditIndex >= 0 && g_laserEditIndex < 20) ? kLaserPointNames[g_laserEditIndex] : "???";
            spdlog::info("[Laser][{} | {}] x={:.2f} y={:.2f} z={:.2f}", g_laserEditIndex, name, e.x, e.y, e.z);
        };

    g_InputHandler.RegisterHeldHotkey(VK_UP, "laser z+", [logLaser]() { if (g_laserPoints) { g_laserPoints[g_laserEditIndex].z += g_laserStep; logLaser(); } });
    g_InputHandler.RegisterHeldHotkey(VK_DOWN, "laser z-", [logLaser]() { if (g_laserPoints) { g_laserPoints[g_laserEditIndex].z -= g_laserStep; logLaser(); } });
    g_InputHandler.RegisterHeldHotkey(VK_NUMPAD1, "laser y+", [logLaser]() { if (g_laserPoints) { g_laserPoints[g_laserEditIndex].y += g_laserStep; logLaser(); } });
    g_InputHandler.RegisterHeldHotkey(VK_NUMPAD2, "laser y-", [logLaser]() { if (g_laserPoints) { g_laserPoints[g_laserEditIndex].y -= g_laserStep; logLaser(); } });
    g_InputHandler.RegisterHeldHotkey(VK_RIGHT, "laser x+", [logLaser]() { if (g_laserPoints) { g_laserPoints[g_laserEditIndex].x += g_laserStep; logLaser(); } });
    g_InputHandler.RegisterHeldHotkey(VK_LEFT, "laser x-", [logLaser]() { if (g_laserPoints) { g_laserPoints[g_laserEditIndex].x -= g_laserStep; logLaser(); } });
    g_InputHandler.RegisterHotkey(VK_NUMPAD3, "laser idx+", [logLaser]() { ++g_laserEditIndex; logLaser(); });
    g_InputHandler.RegisterHotkey(VK_NUMPAD4, "laser idx-", [logLaser]() { --g_laserEditIndex; logLaser(); });
    g_InputHandler.RegisterHotkey(VK_NUMPAD5, "laser reset", [logLaser]() {
        if (g_laserPoints) { g_laserPoints[g_laserEditIndex] = g_laserPointsBackup[g_laserEditIndex]; logLaser(); }
                                  });
                                  */
#pragma endregion

