#include "common.hpp"
#include "logging.hpp"
#include "submodule_initiailization.hpp"
#include "config.hpp"

///Resources
#include "d3d11_api.hpp"
#include "gamevars.hpp"
#include "steamworks_api.hpp"
#include "version_checking.hpp"

///Features
#include "effect_speeds.hpp"
#include "intro_skip.hpp"
#include "keep_aiming_after_firing.hpp"
#include "pause_on_focus_loss.hpp"
#include "stat_persistence.hpp"
#include "mgs2_sunglasses.hpp"

///Fixes
#include "aiming_after_equip.hpp"
#include "line_scaling.hpp"
#include "skyboxes.hpp"
#include "stereo_audio.hpp"
#include "texture_buffer_size.hpp"
#include "water_reflections.hpp"

//Warnings
#include "asi_loader_checks.hpp"
#include "corrupt_save_message.hpp"
#include "mute_warning.hpp"
#include "reshade_compatibility_checks.hpp"

///WIP


#include "aiming_full_tilt.hpp"
#include "color_filters.hpp"
#include "distance_culling.hpp"
#include "gamma_correction.hpp"
#include "mg1_custom_loading_screens.hpp"
#include "msaa.hpp"
#include "wireframe.hpp"


// Aspect ratio + HUD stuff
constexpr float fNativeAspect = 16.0f / 9.0f;
float fAspectMultiplier;
float fHUDWidth;
float fHUDHeight;
constexpr float fDefaultHUDWidth = 1280;
constexpr float fDefaultHUDHeight = 720;
float fHUDWidthOffset;
float fHUDHeightOffset;
float fMGS2_EffectScaleX;
float fMGS2_EffectScaleY;

// CreateWindowExA Hook
SafetyHookInline CreateWindowExA_hook{};
HWND WINAPI CreateWindowExA_hooked(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    const LPCSTR sClassName = "CSD3DWND";
    if (std::string(lpClassName) == std::string(sClassName))
    {
        if (bBorderlessMode && !(eGameType & UNKNOWN))
        {
            auto hWnd = CreateWindowExA_hook.stdcall<HWND>(dwExStyle, lpClassName, lpWindowName, WS_POPUP, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
            SetWindowPos(hWnd, HWND_TOP, 0, 0, DesktopDimensions.first, DesktopDimensions.second, NULL);
            spdlog::info("CreateWindowExA: Borderless: ClassName = {}, WindowName = {}, dwStyle = 0x{:08X}, X = {}, Y = {}, nWidth = {}, nHeight = {}", lpClassName, lpWindowName, WS_POPUP, X, Y, nWidth, nHeight);
            spdlog::info("CreateWindowExA: Borderless: SetWindowPos to X = {}, Y = {}, cx = {}, cy = {}", 0, 0, (int)DesktopDimensions.first, (int)DesktopDimensions.second);
            g_D3D11Hooks.MainHwnd = hWnd;
            return hWnd;
        }

        if (bWindowedMode && !(eGameType & UNKNOWN))
        {
            auto hWnd = CreateWindowExA_hook.stdcall<HWND>(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
            SetWindowPos(hWnd, HWND_TOP, 0, 0, iOutputResX, iOutputResY, NULL);
            spdlog::info("CreateWindowExA: Windowed: ClassName = {}, WindowName = {}, dwStyle = 0x{:08X}, X = {}, Y = {}, nWidth = {}, nHeight = {}", lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight);
            spdlog::info("CreateWindowExA: Windowed: SetWindowPos to X = {}, Y = {}, cx = {}, cy = {}", 0, 0, iOutputResX, iOutputResY);
            g_D3D11Hooks.MainHwnd = hWnd;
            return hWnd;
        }
    }

    g_D3D11Hooks.MainHwnd = CreateWindowExA_hook.stdcall<HWND>(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    return g_D3D11Hooks.MainHwnd;
}

static void Init_CalculateScreenSize()
{
    // Calculate aspect ratio
    fAspectRatio = (float)iInternalResX / (float)iInternalResY;
    fAspectMultiplier = fAspectRatio / fNativeAspect;

    // HUD variables
    fHUDWidth = iInternalResY * fNativeAspect;
    fHUDHeight = (float)iInternalResY;
    fHUDWidthOffset = (float)(iInternalResX - fHUDWidth) / 2;
    fHUDHeightOffset = 0;
    if (fAspectRatio < fNativeAspect) 
    {
        fHUDWidth = (float)iInternalResX;
        fHUDHeight = (float)iInternalResX / fNativeAspect;
        fHUDWidthOffset = 0;
        fHUDHeightOffset = (float)(iInternalResY - fHUDHeight) / 2;
    }


    // Log details about current resolution
    spdlog::info("Current Resolution: Aspect Ratio: {}", fAspectRatio);
    spdlog::info("Current Resolution: Aspect Ratio Multiplier: {}", fAspectMultiplier);
    spdlog::info("Current Resolution: Corrected HUD Width: {}", fHUDWidth);
    spdlog::info("Current Resolution: Correct HUD Height: {}", fHUDHeight);
    spdlog::info("Current Resolution: HUD Width Offset: {}", fHUDWidthOffset);
    spdlog::info("Current Resolution: HUD Height Offset: {}", fHUDHeightOffset);
}

static void Init_FixDPIScaling()
{
    if (eGameType & (MG|MGS2|MGS3)) 
    {
        SetProcessDPIAware();
        spdlog::info("MG/MG2 | MGS 2 | MGS 3: High-DPI scaling fixed.");
    }
}

static void Init_CustomResolution()
{
    if (eGameType & (MG|MGS2|MGS3) && bOutputResolution)
    {
        // MGS 2 | MGS 3: Custom Resolution
        uint8_t* MGS2_MGS3_InternalResolutionScanResult = Memory::PatternScan(baseModule, "F2 0F ?? ?? ?? B9 05 00 00 00 E8 ?? ?? ?? ?? 85 ?? 75 ??", "MGS2_MGS3_InternalResolutionScan");
        uint8_t* MGS2_MGS3_OutputResolution1ScanResult = Memory::PatternScan(baseModule, "40 ?? ?? 74 ?? 8B ?? ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? EB ?? B9 06 00 00 00", "MGS2_MGS3_OutputResolution1Scan");
        uint8_t* MGS2_MGS3_OutputResolution2ScanResult = Memory::PatternScan(baseModule, "80 ?? ?? 00 41 ?? ?? ?? ?? ?? 48 ?? ?? ?? BA ?? ?? ?? ?? 8B ??", "MGS2_MGS3_OutputResolution2Scan");
        if (MGS2_MGS3_InternalResolutionScanResult && MGS2_MGS3_OutputResolution1ScanResult && MGS2_MGS3_OutputResolution2ScanResult)
        {
            uint8_t* MGS2_MGS3_FSR_Result = Memory::PatternScanSilent(baseModule, "83 E8 ?? 74 ?? 83 E8 ?? 74 ?? 83 F8 ?? 75 ?? C7 06");

            if (MGS2_MGS3_FSR_Result)
            {
                static SafetyHookMid FSRWarningMidHook{};
                FSRWarningMidHook = safetyhook::create_mid(MGS2_MGS3_FSR_Result,
                    [](SafetyHookContext& ctx)
                    {
                        spdlog::warn("----------");
                        spdlog::warn("WARNING: Main launcher's AMD FSR Upscaling resolution/graphical options are enabled! Unintended side effects, ie pixelization, mipmap issues, and crashing, may occur!");
                        spdlog::warn("WARNING: It's advised to set both Internal Resolution & Internal Upscaling graphical options in the game's main launcher to default/original unless ABSOLUTELY necessary!");
                        spdlog::warn("----------");
                    });
                
            }

            // Output resolution 1
            static SafetyHookMid OutputResolution1MidHook{};
            OutputResolution1MidHook = safetyhook::create_mid(MGS2_MGS3_OutputResolution1ScanResult,
                [](SafetyHookContext& ctx)
                {
                    ctx.rbx = iOutputResX;
                    ctx.rdi = iOutputResY;
                });

            // Output resolution 2
            static SafetyHookMid OutputResolution2MidHook{};
            OutputResolution2MidHook = safetyhook::create_mid(MGS2_MGS3_OutputResolution2ScanResult,
                [](SafetyHookContext& ctx)
                {
                    ctx.r8 = iOutputResX;
                    ctx.r9 = iOutputResY;
                });

            // Internal resolution
            static SafetyHookMid InternalResolutionMidHook{};
            InternalResolutionMidHook = safetyhook::create_mid(MGS2_MGS3_InternalResolutionScanResult + 0x5,
                [](SafetyHookContext& ctx)
                {
                    if (ctx.rbx + 0x4C) 
                    {
                        *reinterpret_cast<int*>(ctx.rbx + 0x4C) = iInternalResX;
                        *reinterpret_cast<int*>(ctx.rbx + 0x54) = iInternalResY;

                        *reinterpret_cast<int*>(ctx.rbx + 0x50) = iInternalResX;
                        *reinterpret_cast<int*>(ctx.rbx + 0x58) = iInternalResY;
                    }
                });
            
            // Replace loading screens with the appropriate resolutions.
            if (iOutputResY >= 1080) 
            {
                if (!Memory::PatternScanSilent(baseModule, "5F 34 6B 2E 63 74 78 72 00")) //  _4k.ctxr - Make sure the game is a version with 4k loadingscreens
                    spdlog::warn("MGS 2 | MGS 3: Custom Resolution: Splashscreens: Incompatible game version. Skipping.");
                else 
                {
                    if (uint8_t* MGS2_MGS3_SplashscreenResult = Memory::PatternScan(baseModule, "FF 15 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 4C 8D 44 24 ?? 48 8D 54 24 ?? 48 8B 08 48 8B 01 FF 50 ?? 48 8B 58", "MGS 2 | MGS 3: Custom Resolution: Splashscreens"))
                    {
                        static SafetyHookMid MGS2_MGS3_SplashScreenMidHook{};
                        MGS2_MGS3_SplashScreenMidHook = safetyhook::create_mid(MGS2_MGS3_SplashscreenResult,
                            [](SafetyHookContext& ctx)
                            {
                                std::string fileName = reinterpret_cast<char*>(ctx.rdx);
                                if (fileName.ends_with("_720.ctxr")) 
                                {
                                    fileName.replace(fileName.end() - 8, fileName.begin(), iOutputResY >= 2160 ? "4k.ctxr" : 
                                                                                           iOutputResY >= 1440 ? "wqhd.ctxr":
                                                                                         /*iOutputResY >= 1080*/ "fhd.ctxr");
                                    ctx.rdx = reinterpret_cast<uintptr_t>(fileName.c_str());
                                }
                            });
                        LOG_HOOK(MGS2_MGS3_SplashScreenMidHook, "MGS 2 | MGS 3: Custom Resolution")
                    }

                    if (uint8_t* MGS2_MGS3_LoadingScreenEngScanResult = Memory::PatternScan(baseModule, "48 8D 8C 24 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 8C 24 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 8C 24 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 8C 24", "MGS 2 | MGS 3: Custom Resolution: Loading Screen (ENG)"))
                    {
                        static SafetyHookMid MGS2_MGS3_LoadingScreenEngMidHook{};
                        MGS2_MGS3_LoadingScreenEngMidHook = safetyhook::create_mid(MGS2_MGS3_LoadingScreenEngScanResult,
                            [](SafetyHookContext& ctx)
                            {
                                ctx.rdx = iOutputResY >= 2160 ? reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_4k.ctxr") :
                                          iOutputResY >= 1440 ? reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_wqhd.ctxr") :
                                        /*iOutputResY >= 1080*/ reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_fhd.ctxr");
                            });
                        LOG_HOOK(MGS2_MGS3_LoadingScreenEngMidHook, "MGS 2 | MGS 3: Custom Resolution: Loading Screen (ENG)")
                    }

                    if (uint8_t* MGS2_MGS3_LoadingScreenJPScanResult = Memory::PatternScan(baseModule, "48 8D 4C 24 ?? FF 15 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 4C 24", "MGS 2 | MGS 3: Custom Resolution: Loading Screen (JP)")) //    /loading_jp.ctxr 
                    {
                        static SafetyHookMid MGS2_MGS3_LoadingScreenJPMidHook{};
                        MGS2_MGS3_LoadingScreenJPMidHook = safetyhook::create_mid(MGS2_MGS3_LoadingScreenJPScanResult,
                            [](SafetyHookContext& ctx)
                            {
                                ctx.rdx = iOutputResY >= 2160 ? reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_jp_4k.ctxr") :
                                          iOutputResY >= 1440 ? reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_jp_wqhd.ctxr") :
                                        /*iOutputResY >= 1080*/ reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_jp_fhd.ctxr");
                            });
                        LOG_HOOK(MGS2_MGS3_LoadingScreenJPMidHook, "MGS 2 | MGS 3: Custom Resolution: Loading Screen (JP)")
                    }
                }

            }
        }
        else if (!MGS2_MGS3_InternalResolutionScanResult || !MGS2_MGS3_OutputResolution1ScanResult || !MGS2_MGS3_OutputResolution2ScanResult)
        {
            spdlog::error("MG/MG2 | MGS 2 | MGS 3: Custom Resolution: Pattern scan failed.");
        }

        // MG 1/2 | MGS 2 | MGS 3: WindowedMode
        if (uint8_t* MGS2_MGS3_WindowedModeScanResult = Memory::PatternScan(baseModule, "48 ?? ?? E8 ?? ?? ?? ?? 84 ?? 0F 84 ?? ?? ?? ?? 48 ?? ?? ?? ?? ?? ?? 41 ?? 03 00 00 00", "WindowedMode"))
        {
            static SafetyHookMid WindowedModeMidHook{};
            WindowedModeMidHook = safetyhook::create_mid(MGS2_MGS3_WindowedModeScanResult,
                [](SafetyHookContext& ctx)
                {
                    // Force windowed mode if windowed or borderless is set.
                    if (bWindowedMode || bBorderlessMode)
                    {
                        ctx.rdx = 0;
                    }
                    else
                    {
                        ctx.rdx = 1;
                    }
                });
        }


        // MG 1/2 | MGS 2 | MGS 3: CreateWindowExA
        CreateWindowExA_hook = safetyhook::create_inline(CreateWindowExA, reinterpret_cast<void*>(CreateWindowExA_hooked));
        LOG_HOOK(CreateWindowExA_hook, "MG/MG2 | MGS 2 | MGS 3: CreateWindowExA")

        // MG 1/2 | MGS 2 | MGS 3: SetWindowPos
        if (uint8_t* MGS2_MGS3_SetWindowPosScanResult = Memory::PatternScan(baseModule, "33 ?? 48 ?? ?? ?? FF ?? ?? ?? ?? ?? 8B ?? ?? BA 02 00 00 00", "SetWindowPos"))
        {
            static SafetyHookMid SetWindowPosMidHook{};
            SetWindowPosMidHook = safetyhook::create_mid(MGS2_MGS3_SetWindowPosScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (bBorderlessMode)
                    {
                        // Set X and Y to 0 to position window at centre of screen.
                        ctx.r8 = 0;
                        ctx.r9 = 0;
                        // Set window width and height to desktop resolution.
                        *reinterpret_cast<int*>(ctx.rsp + 0x20) = (int)DesktopDimensions.first;
                        *reinterpret_cast<int*>(ctx.rsp + 0x28) = (int)DesktopDimensions.second;
                    }

                    if (bWindowedMode)
                    {
                        // Set X and Y to 0 to position window at centre of screen in case the window is off-screen.
                        ctx.r8 = 0;
                        ctx.r9 = 0;
                        // Set window width and height to custom resolution.
                        *reinterpret_cast<int*>(ctx.rsp + 0x20) = iOutputResX;
                        *reinterpret_cast<int*>(ctx.rsp + 0x28) = iOutputResY;
                    }
                });

        }


        // MGS 2 | MGS 3: Framebuffer fix, stops the framebuffer from being set to maximum display resolution.
        // Thanks emoose!
        if (bFramebufferFix)
        {
            // Need to stop hor + vert from being modified.
            for (int i = 1; i <= 2; ++i)
            {
                // Fullscreen framebuffer
                if (uint8_t* MGS2_MGS3_FullscreenFramebufferFixScanResult = Memory::PatternScanSilent(baseModule, "03 ?? 41 ?? ?? ?? C7 ?? ?? ?? ?? ?? ?? 00 00 00"))
                {
                    Memory::PatchBytes((uintptr_t)MGS2_MGS3_FullscreenFramebufferFixScanResult + 0x2, "\x90\x90\x90\x90", 4);
                    spdlog::info("MG/MG2 | MGS 2 | MGS 3: Fullscreen Framebuffer {}: Patched instruction.", i);
                }
            }

            // Windowed framebuffer
            
            if (uint8_t* MGS2_MGS3_WindowedFramebufferFixScanResult = Memory::PatternScan(baseModule, "?? ?? F3 0F ?? ?? 41 ?? ?? F3 0F ?? ?? F3 0F ?? ?? 66 0F ?? ?? 0F ?? ??", "Windowed Framebuffer"))
            {
                Memory::PatchBytes((uintptr_t)MGS2_MGS3_WindowedFramebufferFixScanResult, "\xEB", 1);
                if (eGameType & MG|MGS3)
                {
                    Memory::PatchBytes((uintptr_t)MGS2_MGS3_WindowedFramebufferFixScanResult + 0x2A, "\xEB", 1);
                }
                    
                if (eGameType & MGS2)
                {
                    Memory::PatchBytes((uintptr_t)MGS2_MGS3_WindowedFramebufferFixScanResult + 0x27, "\xEB", 1);
                }
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Windowed Framebuffer: Patched instructions.");
            }
        }
    }

    /*
    float WidescreenRes = (float)(iOutputResY * 16) / 9;
    
    if (uint8_t* MGS2_MGS3_ViewportScanResult = Memory::PatternScan(baseModule, "48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 4C 8B C2", "MGS 2 | MGS 3: CD3DCachedDevice::SetViewport"))
    {
        static SafetyHookMid Viewport_MidHook {};
        Viewport_MidHook = safetyhook::create_mid(MGS2_MGS3_ViewportScanResult,
            [](SafetyHookContext& ctx)
            {
                D3D11_VIEWPORT D3DViewport = *reinterpret_cast<D3D11_VIEWPORT*>(ctx.rdx);
                //check what scales with 1024x1024
                /*if (D3DViewport.Height == 720.0f)
                {
                    return;
                }

                if (D3DViewport.Height == 2160.0f)
                {
                    return;
                }
                if (D3DViewport.Height == 969.0f && (D3DViewport.Width == 645.0f || D3DViewport.Width == 652.0f) && ((D3DViewport.TopLeftX == 2745.0f && D3DViewport.TopLeftY == 106.0f) || (D3DViewport.TopLeftX == 427.0f && D3DViewport.TopLeftY == 106.0f)))
                {
                    return;
                }
                if (D3DViewport.Width == 384.0f && D3DViewport.Height == 288.0f)//videos
                {
                    return;
                }
                spdlog::info("MGS 2 | MGS 3: Viewport: X: {}, Y: {}, Width: {}, Height: {}, Max Depth: {}, Min Depth: {}", D3DViewport.TopLeftX, D3DViewport.TopLeftY, D3DViewport.Width, D3DViewport.Height, D3DViewport.MaxDepth, D3DViewport.MinDepth);
                /*
                if (D3DViewport.TopLeftY != 1080.0f)
                {
                    //D3DViewport.TopLeftX = (float)iOutputResX/2/2/2;
                    D3DViewport.Width = 1920.0f;
                    spdlog::info("MGS 2 | MGS 3: Viewport: New X: {}, Y: {}, Width: {}, Height: {}", D3DViewport.TopLeftX, D3DViewport.TopLeftY, D3DViewport.Width, D3DViewport.Height); 
                    *reinterpret_cast<D3D11_VIEWPORT*>(ctx.rdx) = D3DViewport;

                }*//*
            });
    }
    */
}


static void Init_ScaleEffects()
{
    if ((eGameType & (MGS2|MGS3)) && bOutputResolution)
    {
        // MGS 2 | MGS 3: Fix scaling for added volume menu in v1.4.0 patch
        if (uint8_t* MGS2_MGS3_VolumeMenuScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? 48 ?? ?? ?? 89 ?? ?? ?? 00 00 F3 0F ?? ?? 89 ?? ?? ?? 00 00", "MGS 2 | MGS 3: Volume Menu"))
        {
            static SafetyHookMid MGS2_MGS3_VolumeMenuMidHook{};
            MGS2_MGS3_VolumeMenuMidHook = safetyhook::create_mid(MGS2_MGS3_VolumeMenuScanResult,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm2.f32[0] = (float)1280;
                    ctx.xmm3.f32[0] = (float)720;
                });
            LOG_HOOK(MGS2_MGS3_VolumeMenuMidHook, "MGS 2 | MGS 3: Volume Menu")
        }
    }

    if (eGameType & MGS2 && bOutputResolution)
    {
        // MGS 2: Scale Effects
        if (uint8_t* MGS2_ScaleEffectsScanResult = Memory::PatternScan(baseModule, "48 8B ?? ?? 66 ?? ?? ?? 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? ?? ?? ??", "MGS 2: Scale Effects"))
        {
            float fMGS2_DefaultEffectScaleX = *reinterpret_cast<float*>(Memory::GetAbsolute((uintptr_t)MGS2_ScaleEffectsScanResult - 0x4));
            float fMGS2_DefaultEffectScaleY = *reinterpret_cast<float*>(Memory::GetAbsolute((uintptr_t)MGS2_ScaleEffectsScanResult + 0x28));
            spdlog::info("MGS 2: Scale Effects: Default X is {}, Y is {}", fMGS2_DefaultEffectScaleX, fMGS2_DefaultEffectScaleY);

            if (bHUDFix)
            {
                fMGS2_EffectScaleX = (float)fMGS2_DefaultEffectScaleX / (fDefaultHUDWidth / fHUDWidth);
                fMGS2_EffectScaleY = (float)fMGS2_DefaultEffectScaleY / (fDefaultHUDHeight / (float)iInternalResY);
                if (fAspectRatio < fNativeAspect)
                {
                    fMGS2_EffectScaleX = (float)fMGS2_DefaultEffectScaleX / (fDefaultHUDWidth / (float)iInternalResX);
                    fMGS2_EffectScaleY = (float)fMGS2_DefaultEffectScaleY / (fDefaultHUDWidth / (float)iInternalResX);
                }
                spdlog::info("MGS 2: Scale Effects (HUD Fix Enabled): New X is {}, Y is {}", fMGS2_EffectScaleX, fMGS2_EffectScaleY);
            }
            else
            {
                fMGS2_EffectScaleX = (float)fMGS2_DefaultEffectScaleX / (fDefaultHUDWidth / (float)iInternalResX);
                fMGS2_EffectScaleY = (float)fMGS2_DefaultEffectScaleY / (fDefaultHUDHeight / (float)iInternalResY);
                spdlog::info("MGS 2: Scale Effects: New X is {}, Y is {}", fMGS2_EffectScaleX, fMGS2_EffectScaleY);
            }

            static SafetyHookMid ScaleEffectsXMidHook{};
            ScaleEffectsXMidHook = safetyhook::create_mid(MGS2_ScaleEffectsScanResult,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm1.f32[0] = fMGS2_EffectScaleX;
                });

            static SafetyHookMid ScaleEffectsX2MidHook{};
            ScaleEffectsX2MidHook = safetyhook::create_mid(MGS2_ScaleEffectsScanResult - 0x2B,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm1.f32[0] = fMGS2_EffectScaleX;
                });

            static SafetyHookMid ScaleEffectsYMidHook{};
            ScaleEffectsYMidHook = safetyhook::create_mid(MGS2_ScaleEffectsScanResult + 0x2C,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm1.f32[0] = fMGS2_EffectScaleY;
                });
        }
    }
    
}




static void Init_AspectFOVFix()
{
    // Fix aspect ratio
    if (eGameType & MGS3 && bAspectFix)
    {
        // MGS 3: Fix gameplay aspect ratio
        if (uint8_t* MGS3_GameplayAspectScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? E8 ?? ?? ?? ?? 48 8D ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ??", "MGS 3: Aspect Ratio"))
        {
            DWORD64 MGS3_GameplayAspectAddress = Memory::GetAbsolute((uintptr_t)MGS3_GameplayAspectScanResult + 0x5);
            spdlog::info("MGS 3: Aspect Ratio: Function address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS3_GameplayAspectAddress - (uintptr_t)baseModule);

            static SafetyHookMid MGS3_GameplayAspectMidHook{};
            MGS3_GameplayAspectMidHook = safetyhook::create_mid(MGS3_GameplayAspectAddress + 0x38,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm1.f32[0] /= fAspectMultiplier;
                });
            LOG_HOOK(MGS3_GameplayAspectMidHook, "MGS 3: Aspect Ratio")

        }
    }
    else if (eGameType & MGS2 && bAspectFix)
    {
        // MGS 2: Fix gameplay aspect ratio
        if (uint8_t* MGS2_GameplayAspectScanResult = Memory::PatternScan(baseModule, "48 8D ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? ??", "MGS 2: Aspect Ratio"))
        {
            DWORD64 MGS2_GameplayAspectAddress = Memory::GetAbsolute((uintptr_t)MGS2_GameplayAspectScanResult + 0xB);
            spdlog::info("MGS 2: Aspect Ratio: Function address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS2_GameplayAspectAddress - (uintptr_t)baseModule);

            static SafetyHookMid MGS2_GameplayAspectMidHook{};
            MGS2_GameplayAspectMidHook = safetyhook::create_mid(MGS2_GameplayAspectAddress + 0x38,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm0.f32[0] /= fAspectMultiplier;
                });
            LOG_HOOK(MGS2_GameplayAspectMidHook, "MGS 2: Aspect Ratio")
        }
    }

    // Convert FOV to vert- to match 16:9 horizontal field of view
    if (eGameType & MGS3 && bFOVFix)
    {
        // MGS 3: FOV
        if (uint8_t* MGS3_FOVScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? 44 ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 ?? ?? ?? ?? E8 ?? ?? ?? ??", "MGS 3: FOV"))
        {
            static SafetyHookMid MGS3_FOVMidHook{};
            MGS3_FOVMidHook = safetyhook::create_mid(MGS3_FOVScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio < fNativeAspect)
                    {
                        ctx.xmm2.f32[0] *= fAspectMultiplier;
                    }
                });
            LOG_HOOK(MGS3_FOVMidHook, "MG3 2: FOV")
        }
    }
    else if (eGameType & MGS2 && bFOVFix)
    {
        // MGS 2: FOV
        if (uint8_t* MGS2_FOVScanResult = Memory::PatternScan(baseModule, "44 ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 44 ?? ?? ?? ?? 48 ?? ?? 48 ?? ?? ?? ?? 00 00", "MGS 2: FOV"))
        {
            static SafetyHookMid MGS2_FOVMidHook{};
            MGS2_FOVMidHook = safetyhook::create_mid(MGS2_FOVScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio < fNativeAspect)
                    {
                        ctx.xmm2.f32[0] *= fAspectMultiplier;
                    }
                });
            LOG_HOOK(MGS2_FOVMidHook, "MGS 2: FOV")
        }
        
    }
    
}

static void Init_HUDFix()
{
    if (eGameType & MGS2 && bHUDFix)
    {
        // MGS 2: HUD
        uint8_t* MGS2_HUDWidthScanResult = Memory::PatternScanSilent(baseModule, "E9 ?? ?? ?? ?? F3 0F ?? ?? ?? 0F ?? ?? F3 0F ?? ?? ?? F3 0F ?? ??");
        if (MGS2_HUDWidthScanResult)
        {
            spdlog::info("MGS 2: HUD: Address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS2_HUDWidthScanResult - (uintptr_t)baseModule);

            static SafetyHookMid MGS2_HUDWidthMidHook{};
            MGS2_HUDWidthMidHook = safetyhook::create_mid(MGS2_HUDWidthScanResult + 0xD,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio > fNativeAspect)
                    {
                        ctx.xmm2.f32[0] = 1 / fAspectMultiplier;
                        ctx.xmm9.f32[0] = -2;
                        ctx.xmm10.f32[0] = -1;
                    }
                    else if (fAspectRatio < fNativeAspect)
                    {
                        ctx.xmm2.f32[0] = 1;
                        ctx.xmm9.f32[0] = -2 * fAspectMultiplier;
                        ctx.xmm10.f32[0] = -1 * fAspectMultiplier;
                    }
                });
        }
        else if (!MGS2_HUDWidthScanResult)
        {
            spdlog::error("MGS 2: HUD: Pattern scan failed.");
        }

        // MGS 2: Radar
        uint8_t* MGS2_RadarWidthScanResult = Memory::PatternScanSilent(baseModule, "44 ?? ?? 8B ?? 0F ?? ?? ?? 41 ?? ?? 0F ?? ?? ?? 44 ?? ?? ?? ?? ?? ?? 0F ?? ?? ?? 99");
        if (MGS2_RadarWidthScanResult)
        {
            // Radar width
            DWORD64 MGS2_RadarWidthAddress = (uintptr_t)MGS2_RadarWidthScanResult;
            spdlog::info("MGS 2: Radar Width: Hook address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS2_RadarWidthAddress - (uintptr_t)baseModule);

            static SafetyHookMid MGS2_RadarWidthMidHook{};
            MGS2_RadarWidthMidHook = safetyhook::create_mid(MGS2_RadarWidthScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio > fNativeAspect)
                    {
                        ctx.rbx = (int)fHUDWidth;
                    }
                    else if (fAspectRatio < fNativeAspect)
                    {
                        ctx.rax = (int)fHUDHeight;
                    }
                });

            // Radar width offset
            DWORD64 MGS2_RadarWidthOffsetAddress = (uintptr_t)MGS2_RadarWidthScanResult + 0x54;
            spdlog::info("MGS 2: Radar Width Offset: Hook address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS2_RadarWidthOffsetAddress - (uintptr_t)baseModule);

            static SafetyHookMid MGS2_RadarWidthOffsetMidHook{};
            MGS2_RadarWidthOffsetMidHook = safetyhook::create_mid(MGS2_RadarWidthOffsetAddress,
                [](SafetyHookContext& ctx)
                {
                    ctx.rax += (int)fHUDWidthOffset;
                });

            // Radar height offset
            DWORD64 MGS2_RadarHeightOffsetAddress = (uintptr_t)MGS2_RadarWidthScanResult + 0x90;
            spdlog::info("MGS 2: Radar Height Offset: Hook address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS2_RadarHeightOffsetAddress - (uintptr_t)baseModule);

            static SafetyHookMid MGS2_RadarHeightOffsetMidHook{};
            MGS2_RadarHeightOffsetMidHook = safetyhook::create_mid(MGS2_RadarHeightOffsetAddress,
                [](SafetyHookContext& ctx)
                {
                    ctx.rax = (int)fHUDHeightOffset;
                });
        }
        else if (!MGS2_RadarWidthScanResult)
        {
            spdlog::error("MGS 2: Radar Fix: Pattern scan failed.");
        }

        // MGS 2: Codec Portraits
        // TODO: Reassess this, it's not right.
        uint8_t* MGS2_CodecPortraitsScanResult = Memory::PatternScanSilent(baseModule, "F3 0F ?? ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? 66 0F ?? ?? 0F ?? ??");
        if (MGS2_CodecPortraitsScanResult)
        {
            spdlog::info("MGS 2: Codec Portraits: Hook address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS2_CodecPortraitsScanResult - (uintptr_t)baseModule);

            static SafetyHookMid MGS2_CodecPortraitsMidHook{};
            MGS2_CodecPortraitsMidHook = safetyhook::create_mid(MGS2_CodecPortraitsScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio > fNativeAspect)
                    {
                        ctx.xmm0.f32[0] /= fAspectMultiplier;
                        ctx.xmm0.f32[0] += (ctx.xmm4.f32[0] / fAspectMultiplier);
                    }
                    else if (fAspectRatio < fNativeAspect)
                    {
                        ctx.xmm5.f32[0] /= fAspectMultiplier;
                    }
                });
        }
        else if (!MGS2_CodecPortraitsScanResult)
        {
            spdlog::error("MGS 2: Codec Portraits: Pattern scan failed.");
        }

        // MGS 2: Disable motion blur. 
        uint8_t* MGS2_MotionBlurScanResult = Memory::PatternScanSilent(baseModule, "F3 48 ?? ?? ?? ?? 48 ?? ?? ?? 48 ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ??");
        if (MGS2_MotionBlurScanResult)
        {
            spdlog::info("MGS 2: Motion Blur: Address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS2_MotionBlurScanResult - (uintptr_t)baseModule);

            Memory::PatchBytes((uintptr_t)MGS2_MotionBlurScanResult, "\x48\x31\xDB\x90\x90\x90", 6);
            spdlog::info("MGS 2: Motion Blur: Patched instruction.");
        }
        else if (!MGS2_MotionBlurScanResult)
        {
            spdlog::error("MGS 2: Motion Blur: Pattern scan failed.");
        }
    }
    else if (eGameType & MGS3 && bHUDFix || eGameType & MG && fAspectRatio != fNativeAspect)
    {
        // MG1/2 | MGS 3: HUD
        uint8_t* MGS3_HUDWidthScanResult = Memory::PatternScanSilent(baseModule, "0F ?? ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? ?? 4C ?? ?? ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? ?? 41 ?? 00 02 00 00");
        if (MGS3_HUDWidthScanResult)
        {
            static SafetyHookMid MGS3_HUDWidthMidHook{};
            MGS3_HUDWidthMidHook = safetyhook::create_mid(MGS3_HUDWidthScanResult + 0x1F,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio > fNativeAspect)
                    {
                        ctx.xmm14.f32[0] = 2 / fAspectMultiplier;
                        ctx.xmm15.f32[0] = -2;
                    }
                    else if (fAspectRatio < fNativeAspect)
                    {
                        ctx.xmm14.f32[0] = 2;
                        ctx.xmm15.f32[0] = -2 * fAspectMultiplier;
                    }
                });

            spdlog::info("MG1/2 | MGS 3: HUD Width: Hook address is{:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS3_HUDWidthScanResult - (uintptr_t)baseModule);
        }
        else if (!MGS3_HUDWidthScanResult)
        {
            spdlog::error("MG1/2 | MGS 3: HUD Width: Pattern scan failed.");
        }
    }

    if ((eGameType & (MG|MGS2|MGS3)) && bHUDFix)
    {
        // MGS 2 | MGS 3: Letterboxing
        uint8_t* MGS2_MGS3_LetterboxingScanResult = Memory::PatternScanSilent(baseModule, "83 ?? 01 75 ?? ?? 01 00 00 00 44 ?? ?? ?? ?? ?? ?? 89 ?? ?? ?? ?? ??");
        if (MGS2_MGS3_LetterboxingScanResult)
        {
            DWORD64 MGS2_MGS3_LetterboxingAddress = (uintptr_t)MGS2_MGS3_LetterboxingScanResult + 0x6;
            spdlog::info("MGS 2 | MGS 3: Letterboxing: Address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_LetterboxingAddress - (uintptr_t)baseModule);

            Memory::Write(MGS2_MGS3_LetterboxingAddress, (int)0);
            spdlog::info("MGS 2 | MGS 3: Letterboxing: Disabled letterboxing.");
        }
        else if (!MGS2_MGS3_LetterboxingScanResult)
        {
            spdlog::error("MGS 2 | MGS 3: Letterboxing: Pattern scan failed.");
        }
    }
    
}

static void Init_Miscellaneous()
{
    if (eGameType & (MG|MGS2|MGS3|LAUNCHER))
    {
        if (bDisableCursor)
        {
            // Launcher | MG/MG2 | MGS 2 | MGS 3: Disable mouse cursor
            // Thanks again emoose!
            if (uint8_t* MGS2_MGS3_MouseCursorScanResult = Memory::PatternScan(eGameType & LAUNCHER ? unityPlayer : baseModule, "BA 00 7F 00 00 33 ?? FF ?? ?? ?? ?? ?? 48 ?? ??", "Launcher | MG/MG2 | MGS 2 | MGS 3: Mouse Cursor"))
            {
                // The game enters 32512 in the RDX register for the function USER32.LoadCursorA to load IDC_ARROW (normal select arrow in windows)
                // Set this to 0 and no cursor icon is loaded
                Memory::PatchBytes((uintptr_t)MGS2_MGS3_MouseCursorScanResult + 0x2, "\x00", 1);
                spdlog::info("Launcher | MG/MG2 | MGS 2 | MGS 3: Mouse Cursor: Patched instruction.");
            }
        }
    }

    if ((bDisableTextureFiltering || iAnisotropicFiltering > 0) && (eGameType & (MGS2|MGS3)))
    {
        if (uint8_t* MGS3_SetSamplerStateInsnScanResult = Memory::PatternScan(baseModule, "48 8B ?? ?? ?? ?? ?? 44 39 ?? ?? 38 ?? ?? ?? 74 ?? 44 89 ?? ?? ?? ?? ?? ?? EB ?? 48 ?? ??", "MGS 2 | MGS 3: Texture Filtering"))
        {
            static SafetyHookMid SetSamplerStateInsnXMidHook{};
            SetSamplerStateInsnXMidHook = safetyhook::create_mid(MGS3_SetSamplerStateInsnScanResult + 0x7,
                [](SafetyHookContext& ctx)
                {
                    // [rcx+rax+0x438] = D3D11_SAMPLER_DESC, +0x14 = MaxAnisotropy
                    *reinterpret_cast<int*>(ctx.rcx + ctx.rax + 0x438 + 0x14) = iAnisotropicFiltering;

                    // Override filter mode in r9d with aniso value and run compare from orig game code
                    // Game code will then copy in r9d & update D3D etc when r9d is different to existing value
                    //0x1 = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR (Linear mips is essentially perspective correction.) 0x55 = D3D11_FILTER_ANISOTROPIC
                    ctx.r9 = bDisableTextureFiltering ? 0x1 : 0x55;
                });
            LOG_HOOK(SetSamplerStateInsnXMidHook, "MGS 2 | MGS 3: Texture Filtering")
        }

    }

    if (eGameType & MGS3 && bMouseSensitivity)
    {
        // MG 1/2 | MGS 2 | MGS 3: MouseSensitivity
        uint8_t* MGS3_MouseSensitivityScanResult = Memory::PatternScanSilent(baseModule, "F3 0F ?? ?? ?? F3 0F ?? ?? 66 0F ?? ?? ?? 0F ?? ?? 66 0F ?? ?? 8B ?? ??");
        if (MGS3_MouseSensitivityScanResult)
        {
            spdlog::info("MGS 3: Mouse Sensitivity: Address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS3_MouseSensitivityScanResult - (uintptr_t)baseModule);

            static SafetyHookMid MouseSensitivityXMidHook{};
            MouseSensitivityXMidHook = safetyhook::create_mid(MGS3_MouseSensitivityScanResult,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm0.f32[0] *= fMouseSensitivityXMulti;
                });

            static SafetyHookMid MouseSensitivityYMidHook{};
            MouseSensitivityYMidHook = safetyhook::create_mid(MGS3_MouseSensitivityScanResult + 0x2E,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm0.f32[0] *= fMouseSensitivityYMulti;
                });
        }
        else if (!MGS3_MouseSensitivityScanResult)
        {
            spdlog::error("MGS 3: Mouse Sensitivity: Pattern scan failed.");
        }
    }
}


using NHT_COsContext_SetControllerID_Fn = void (*)(int controllerType);
NHT_COsContext_SetControllerID_Fn NHT_COsContext_SetControllerID = nullptr;
void NHT_COsContext_SetControllerID_Hook(int controllerType)
{
    spdlog::info("NHT_COsContext_SetControllerID_Hook: controltype {} -> {}", Util::GetUppercaseNameAtIndex(kLauncherConfigCtrlTypes, controllerType), bIsPS2controltype ? "PS2" : Util::GetUppercaseNameAtIndex(kLauncherConfigCtrlTypes, iLauncherConfigCtrlType));
    NHT_COsContext_SetControllerID(iLauncherConfigCtrlType);
}

using MGS3_COsContext_InitializeSKUandLang_Fn = void(__fastcall*)(void*, int, int);
MGS3_COsContext_InitializeSKUandLang_Fn MGS3_COsContext_InitializeSKUandLang = nullptr;
void __fastcall MGS3_COsContext_InitializeSKUandLang_Hook(void* thisptr, int lang, int sku)
{
    spdlog::info("MGS3_COsContext_InitializeSKUandLang: lang {} -> {}, sku {} -> {}", Util::GetUppercaseNameAtIndex(kLauncherConfigLanguages, lang), Util::GetUppercaseNameAtIndex(kLauncherConfigLanguages, iLauncherConfigLanguage), sku, iLauncherConfigRegion);
    MGS3_COsContext_InitializeSKUandLang(thisptr, iLauncherConfigLanguage, iLauncherConfigRegion);
}

using MGS2_COsContext_InitializeSKUandLang_Fn = void(__fastcall*)(void*, int);
MGS2_COsContext_InitializeSKUandLang_Fn MGS2_COsContext_InitializeSKUandLang = nullptr;
void __fastcall MGS2_COsContext_InitializeSKUandLang_Hook(void* thisptr, int lang)
{
    spdlog::info("MGS2_COsContext_InitializeSKUandLang: lang {} -> {}", Util::GetUppercaseNameAtIndex(kLauncherConfigLanguages, lang), Util::GetUppercaseNameAtIndex(kLauncherConfigLanguages, iLauncherConfigLanguage));
    MGS2_COsContext_InitializeSKUandLang(thisptr, iLauncherConfigLanguage);
}

static void Init_LauncherConfigOverride()
{
    // If we know games steam appid, try creating steam_appid.txt file, so that game EXE can be launched directly in future runs
    if (game)
    {
        const std::filesystem::path steamAppidPath = sExePath.parent_path() / "steam_appid.txt";

        try
        {
            if (!std::filesystem::exists(steamAppidPath))
            {
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Creating steam_appid.txt to allow direct EXE launches.");
                std::ofstream steamAppidOut(steamAppidPath);
                if (steamAppidOut.is_open())
                {
                    steamAppidOut << game->SteamAppId;
                    steamAppidOut.close();
                }
                if (std::filesystem::exists(steamAppidPath))
                {
                    spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: steam_appid.txt created successfully.");
                }
                else
                {
                    spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: steam_appid.txt creation failed.");
                }
            }
        }
        catch (const std::exception& ex)
        {
            spdlog::error("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Launcher Config: steam_appid.txt creation failed (exception: %s)", ex.what());
        }
    }

    // If SkipLauncher is enabled & we're running inside launcher process, we'll just start the game immediately and exit this launcher
    if (eGameType & LAUNCHER)
    {
        if (!bLauncherConfigSkipLauncher)
        {
            if (bLauncherJumpStart)
            {
                LPWSTR commandLine = GetCommandLineW();
                bool hasJumpstart = wcsstr(commandLine, L"-jump gamestart");
                if (hasJumpstart)
                {
                    spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher jumpstart already on commandline.");
                }
                else
                {
                    std::filesystem::path gameExePath = sExePath.parent_path() / "launcher.exe";

                    PROCESS_INFORMATION processInfo = {};
                    STARTUPINFO startupInfo = {};
                    startupInfo.cb = sizeof(STARTUPINFO);
                    std::wstring commandLine = L"\"" + gameExePath.wstring() + L"\"";
                    commandLine += L" -jump gamestart";
                    if (CreateProcess(nullptr, (LPWSTR)commandLine.c_str(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo))
                    {
                        // Successfully started the process
                        CloseHandle(processInfo.hProcess);
                        CloseHandle(processInfo.hThread);

                        // Force launcher to exit
                        ExitProcess(0);
                    }
                }
            }
        }
        else
        {
            auto gameExePath = sExePath.parent_path() / game->ExeName;

            spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: SkipLauncher set, attempting game launch");

            PROCESS_INFORMATION processInfo {};
            STARTUPINFO startupInfo {};
            startupInfo.cb = sizeof(STARTUPINFO);

            std::wstring commandLine = L"\"" + gameExePath.wstring() + L"\"";


            if (game->ExeName == "METAL GEAR.exe")
            {
                // Add launch parameters for MG MSX
                auto transformString = [](const std::string& input, int (*transformation)(int)) -> std::wstring
                    {
                        std::string transformedString = input;
                        std::transform(transformedString.begin(), transformedString.end(), transformedString.begin(), transformation);
                        return Util::UTF8toWide(transformedString);
                    };

                commandLine += L" -mgst " + transformString(sLauncherConfigMSXGame, ::tolower); // -mgst must be lowercase
                commandLine += L" -walltype " + std::to_wstring(iLauncherConfigMSXWallType);
                commandLine += L" -wallalign " + transformString(sLauncherConfigMSXWallAlign, ::toupper); // -wallalign must be uppercase
            }

            std::string sCommandLine = Util::WideToUTF8(commandLine);
            spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Launch command line: {}", sCommandLine);

            // Call CreateProcess to start the game process
            if (CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo))
            {
                // Successfully started the process
                CloseHandle(processInfo.hProcess);
                CloseHandle(processInfo.hThread);

                // Force launcher to exit
                ExitProcess(0);
            }
            else
            {
                spdlog::error("MG/MG2 | MGS 2 | MGS 3: Launcher Config: SkipLauncher failed to create game EXE process");
            }
        }
        return;
    }
    //Fixes a windows crash error message that sometimes appears when exiting through the main menu (which normally reopens the launcher.)
    else if ((bLauncherConfigSkipLauncher || bOutdatedReshade) && (eGameType & (MG | MGS2 | MGS3)))
    {
        uint8_t* ShouldStartLauncher_mbResult = Memory::PatternScanSilent(baseModule, "85 DB 74 ?? 48 83 C4");
        if (ShouldStartLauncher_mbResult)
        {
            static SafetyHookMid ShouldStartLauncher_mbHook{};
            ShouldStartLauncher_mbHook = safetyhook::create_mid(ShouldStartLauncher_mbResult,
                [](SafetyHookContext& ctx)
                {
                    spdlog::info("MG/MG2 | MGS 2 | MGS 3: Exit crash fixed.");
                    ctx.rbx = 0; //ebx -> rbx
                });
        }
        else
        {
            spdlog::error("MG/MG2 | MGS 2 | MGS 3: Launcher Config: SkipLauncher - exit crashfix patternscan failed!");
        }
    }
    


    // Certain config such as language/button style is normally passed from launcher to game via arguments
    // When game EXE gets ran directly this config is left at default (english game, xbox buttons)
    // If launcher argument isn't detected we'll allow defaults to be changed by hooking the engine functions responsible for them
    LPWSTR commandLine = GetCommandLineW();

    bool hasCtrltype = wcsstr(commandLine, L"-ctrltype") != nullptr;
    bool hasRegion = wcsstr(commandLine, L"-region") != nullptr;
    bool hasLang = wcsstr(commandLine, L"-lan") != nullptr;

    if (!hasRegion && !hasLang)
    {
        MGS3_COsContext_InitializeSKUandLang = decltype(MGS3_COsContext_InitializeSKUandLang)(GetProcAddress(engineModule, "?InitializeSKUandLang@COsContext@@QEAAXHH@Z"));
        if (MGS3_COsContext_InitializeSKUandLang)
        {
            if (Memory::HookIAT(baseModule, "Engine.dll", MGS3_COsContext_InitializeSKUandLang, MGS3_COsContext_InitializeSKUandLang_Hook))
            {
                spdlog::info("MG/MG2 | MGS 3: Launcher Config: Overriding Region/Language with: {} / {}", Util::GetUppercaseNameAtIndex(kLauncherConfigRegions, iLauncherConfigRegion), Util::GetUppercaseNameAtIndex(kLauncherConfigLanguages, iLauncherConfigLanguage));
            }
            else
            {
                spdlog::error("MG/MG2 | MGS 3: Launcher Config: Failed to apply COsContext::InitializeSKUandLang IAT hook");
            }
        }
        else
        {
            MGS2_COsContext_InitializeSKUandLang = decltype(MGS2_COsContext_InitializeSKUandLang)(GetProcAddress(engineModule, "?InitializeSKUandLang@COsContext@@QEAAXH@Z"));
            if (MGS2_COsContext_InitializeSKUandLang)
            {
                if (Memory::HookIAT(baseModule, "Engine.dll", MGS2_COsContext_InitializeSKUandLang, MGS2_COsContext_InitializeSKUandLang_Hook))
                {
                    spdlog::info("MGS 2: Launcher Config: Overriding game Region/Language with: {} / {}", Util::GetUppercaseNameAtIndex(kLauncherConfigRegions, iLauncherConfigRegion), Util::GetUppercaseNameAtIndex(kLauncherConfigLanguages, iLauncherConfigLanguage));
                }
                else
                {
                    spdlog::error("MGS 2: Launcher Config: Failed to apply COsContext::InitializeSKUandLang IAT hook");
                }
            }
            else
            {
                spdlog::error("MG/MG2 | MGS 2 | MGS3: Launcher Config: Failed to locate COsContext::InitializeSKUandLang export");
            }
        }
    }
    else
    {
        spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: -region/-lan specified on command-line, skipping INI override");
    }

    if (!hasCtrltype || bIsPS2controltype)
    {
        NHT_COsContext_SetControllerID = decltype(NHT_COsContext_SetControllerID)(GetProcAddress(engineModule, "NHT_COsContext_SetControllerID"));
        if (NHT_COsContext_SetControllerID)
        {
            if (Memory::HookIAT(baseModule, "Engine.dll", NHT_COsContext_SetControllerID, NHT_COsContext_SetControllerID_Hook))
            {
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Overriding controller glyphs with: {} icons.", bIsPS2controltype ? "PS2" : Util::GetUppercaseNameAtIndex(kLauncherConfigCtrlTypes, iLauncherConfigCtrlType));
            }
            else
            {
                spdlog::error("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Failed to apply NHT_COsContext_SetControllerID IAT hook");
            }
        }
        else
        {
            spdlog::error("MG/MG2 | MGS 2 | MGS3: Launcher Config: Failed to locate NHT_COsContext_SetControllerID export");
        }
        if (bIsPS2controltype)
        {
            if (uint8_t* PS4ControllerScan = Memory::PatternScan(baseModule, "6F 76 72 5F 73 74 6D 2F 63 74 72 6C 74 79 70 65 5F 70 73 34 2F", "PS4 Controller Glyphs"))
            {
                Memory::PatchBytes((uintptr_t)PS4ControllerScan, "\x6F\x76\x72\x5F\x73\x74\x6D\x2F\x63\x74\x72\x6C\x74\x79\x70\x65\x5F\x70\x73\x32\x2F", 21);
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Patched PS4 controller glyphs to PS2 glyphs.");
            }
        }
    }
    else
    {
        spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: -ctrltype specified on command-line, skipping INI override");
    }

}


static bool DetectGame()
{
    eGameType = UNKNOWN;
    // Special handling for launcher.exe
    if (sExeName == "launcher.exe")
    {
        for (const auto& [type, info] : kGames)
        {
            auto gamePath = sExePath.parent_path() / info.ExeName;
            if (std::filesystem::exists(gamePath))
            {
                spdlog::info("Detected launcher for game: {} (app {})", info.GameTitle.c_str(), info.SteamAppId);
                eGameType = LAUNCHER;
                unityPlayer = GetModuleHandleA("UnityPlayer.dll");
                game = &info;
                return true;
            }
        }

        spdlog::error("Failed to detect supported game, unknown launcher");
        FreeLibraryAndExitThread(baseModule, 1);
    }

    for (const auto& [type, info] : kGames)
    {
        if (info.ExeName == sExeName)
        {
            spdlog::info("Detected game: {} (app {})", info.GameTitle.c_str(), info.SteamAppId);
            eGameType = type;
            game = &info;

            sGameSavePath = sExePath / (eGameType & MG ? "mg12_savedata_win" : eGameType & MGS2 ? "mgs2_savedata_win" : "mgs3_savedata_win");
            spdlog::info("Game Save Path: {}", sGameSavePath.string());
            if (engineModule = GetModuleHandleA("Engine.dll"); !engineModule)
            {
                spdlog::error("Failed to get Engine.dll module handle");
            }
            return true;
        }
    }

    spdlog::error("Failed to detect supported game, {} isn't supported by MGSHDFix", sExeName.c_str());
    FreeLibraryAndExitThread(baseModule, 1);
}

void AfterSteamInitialized()
{
    static bool bInitialized = false;
    if (bInitialized)
    {
        spdlog::warn("AfterSteamInitialized() called multiple times, skipping initialization.");
        return;
    }
    bInitialized = true;

    spdlog::info("AfterSteamInitialized() started");
    g_StatPersistence.OnSteamInitialized();
    spdlog::info("AfterSteamInitialized() completed");
}

void AfterSteamInputInitialized()
{
    static bool bInitialized = false;
    if (bInitialized)
    {
        spdlog::warn("AfterSteamInputInitialized() called multiple times, skipping initialization.");
        return;
    }
    bInitialized = true;

}


void afterPresent()
{
    static bool bInitialized = false;
    if (bInitialized)
    {
        spdlog::warn("afterPresent() called multiple times, skipping initialization.");
        return;
    }
    bInitialized = true;
    spdlog::info("afterPresent() started");
    g_VectorScalingFix.LoadCompiledShader();
    g_MuteWarning.CheckStatus();
    g_SteamAPI.OnSteamInputLoaded();


    spdlog::info("afterPresent() completed");
}

static void InitializeSubsystems()
{
    //Initialization order (these systems initialize vars used by following ones.)
    INITIALIZE(g_Logging.LogSysInfo());            //0
    INITIALIZE(DetectGame());                      //1
    INITIALIZE(ASILoaderCompatibility::Check());   //2
    INITIALIZE(Config::Read());                    //3
    INITIALIZE(g_GameVars.Initialize());           //4
    INITIALIZE(g_D3D11Hooks.Initialize());         //5 Caches the D3DDevice, DXGIFactory, and D3DContext from D3DCreateDevice/DXGICreateFactory
    INITIALIZE(ReshadeCompatibility::Check());     //6 Dependent on ReadConfig, must also be before LauncherConfigOverride to warn the user before a crash.
    INITIALIZE(Init_CalculateScreenSize());        //7
    INITIALIZE(Init_LauncherConfigOverride());     //8
    INITIALIZE(Init_FixDPIScaling());              //9 Needs to be anywhere before the window is created in CustomResolution.
    INITIALIZE(Init_CustomResolution());           //10
    INITIALIZE(Init_ScaleEffects());               
    INITIALIZE(Init_AspectFOVFix());
    INITIALIZE(Init_HUDFix());
    INITIALIZE(Init_Miscellaneous());

        //Features
    INITIALIZE(g_TextureBufferSize.Initialize());
    INITIALIZE(g_PauseOnFocusLoss.Initialize());
    INITIALIZE(g_IntroSkip.Initialize());
    INITIALIZE(g_KeepAimingAfterFiring.Initialize());
    INITIALIZE(g_MGS2Sunglasses.Initialize());


    //INITIALIZE(g_DistanceCulling.Initialize());
    //INITIALIZE(g_MG1CustomLoadingScreens.Initialize());
    //INITIALIZE(g_MultiSampleAntiAliasing.Initialize());
    //INITIALIZE(g_Wireframe.Initialize());

        //Fixes
    INITIALIZE(g_VectorScalingFix.Initialize());
    INITIALIZE(g_WaterReflectionFix.Initialize());
    INITIALIZE(SkyboxFix::Initialize());
    INITIALIZE(g_EffectSpeedFix.Initialize());
    INITIALIZE(g_StereoAudioFix.Initialize());
    INITIALIZE(DamagedSaveFix::Initialize());
    INITIALIZE(g_FixAimAfterEquip.Initialize());
    INITIALIZE(g_FixAimingFullTilt.Initialize());
    //INITIALIZE(g_ColorFilterFix.Initialize());

        //Warnings
    INITIALIZE(g_MuteWarning.Setup()); 

    if (!(eGameType & LAUNCHER))
    {
        INITIALIZE(g_SteamAPI.Setup());
        INITIALIZE(g_StatPersistence.Setup());
        INITIALIZE(CheckForUpdates());
    }
}

std::mutex mainThreadFinishedMutex;
std::condition_variable mainThreadFinishedVar;
bool mainThreadFinished = false;

DWORD __stdcall Main(void*)
{
    g_Logging.initStartTime = std::chrono::high_resolution_clock::now();
    g_Logging.Initialize();

    INITIALIZE(InitializeSubsystems());

    // Signal any threads (e.g., the memset hook) that are waiting for initialization to finish.
    {
        std::lock_guard lock(mainThreadFinishedMutex);
        mainThreadFinished = true;
    }
    mainThreadFinishedVar.notify_all();

    return TRUE;
}

std::mutex memsetHookMutex;
bool memsetHookCalled = false;
static void* (__cdecl* memset_Fn)(void* Dst, int Val, size_t Size); // Pointer to the next function in the memset chain (could be another hook or the real CRT memset).
static void* __cdecl memset_Hook(void* Dst, int Val, size_t Size) // Our memset hook, which blocks the game's main thread until initialization is complete.
{
    std::lock_guard lock(memsetHookMutex);

    if (!memsetHookCalled)
    {
        memsetHookCalled = true;

        // Restore the original (or previously-hooked) memset in the IAT.
        // This ensures future memset calls bypass our hook and run at full speed.
        Memory::WriteIAT(baseModule, "VCRUNTIME140.dll", "memset", memset_Fn);

        // Block the current thread here until our main initialization is complete.
        std::unique_lock finishedLock(mainThreadFinishedMutex);
        mainThreadFinishedVar.wait(finishedLock, []
            {
                return mainThreadFinished;
            });
    }

    // Forward the memset call to the next function (another hook or the real memset).
    return reinterpret_cast<decltype(memset_Fn)>(memset_Fn)(Dst, Val, Size);
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        HMODULE vcruntime140 = GetModuleHandleA("VCRUNTIME140.dll");
        if (vcruntime140)
        {
            // Read the current IAT entry for memset in the base module.
            // Note: it may already point to another mod's hook if they loaded first.
            void* currentIATMemset = Memory::ReadIAT(baseModule, "VCRUNTIME140.dll", "memset");

            // Save the current pointer so we can call it later (chaining to the next hook or real memset).
            memset_Fn = reinterpret_cast<decltype(memset_Fn)>(currentIATMemset);

            // Overwrite the IAT entry with our memset_Hook, so our code intercepts memset calls.
            // We always overwrite unconditionally to ensure our hook is active.
            // This will prevent other mods that also hook memset from unpausing the main thread before our Main() has finished.
            Memory::WriteIAT(baseModule, "VCRUNTIME140.dll", "memset", &memset_Hook);
        }

        // Create our main thread, which runs the initialization logic.
        if (const HANDLE mainHandle = CreateThread(nullptr, 0, Main, nullptr, CREATE_SUSPENDED, nullptr))
        {
            SetThreadPriority(mainHandle, THREAD_PRIORITY_TIME_CRITICAL); // Give our thread higher priority than the game's.
            ResumeThread(mainHandle);
            CloseHandle(mainHandle);
        }

        // Prevent monitor or system sleep while the game is running.
        SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
    }
    return TRUE;
}
