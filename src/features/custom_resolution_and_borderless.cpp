#include "stdafx.h"

#include "custom_resolution_and_borderless.hpp"

#include "logging.hpp"

#include "common.hpp"
#include "config_keys.hpp"
#include "d3d11_api.hpp"

namespace
{
    float fAspectRatio;

    constexpr float fNativeAspect = 16.0f / 9.0f;
    float fAspectMultiplier;
    float fHUDWidth;
    float fHUDHeight;
    constexpr float fDefaultHUDWidth = 1280.0f;
    constexpr float fDefaultHUDHeight = 720.0f;
    float fHUDWidthOffset;
    float fHUDHeightOffset;
    float fMGS2_EffectScaleX;
    float fMGS2_EffectScaleY;

    const char* kOrigWndProcProp = "CRB_OrigWndProc";
    const char* kInitFocusProp = "CRB_InitFocusDone";

    bool IsOverlappingTaskbarArea(HWND hWnd)
    {
        if (hWnd == nullptr)
        {
            return false;
        }

        const int winW = CustomResolutionAndBorderless::iFinalWindowResolutionX;
        const int winH = CustomResolutionAndBorderless::iFinalWindowResolutionY;

        if (winW <= 0 || winH <= 0)
        {
            return false;
        }

        HMONITOR mon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
        if (mon == nullptr)
        {
            return false;
        }

        MONITORINFO mi {};
        mi.cbSize = sizeof(mi);
        if (!GetMonitorInfoA(mon, &mi))
        {
            return false;
        }

        const RECT& monRect = mi.rcMonitor;
        const RECT& work = mi.rcWork;

        const int monW = monRect.right - monRect.left;
        const int monH = monRect.bottom - monRect.top;

        // Centered in the FULL monitor area (not work area).
        const int left = monRect.left + (monW - winW) / 2;
        const int top = monRect.top + (monH - winH) / 2;
        const int right = left + winW;
        const int bottom = top + winH;

        // If any part of this centered rect lies outside work area, it overlaps taskbar/appbar space.
        if (left < work.left)
        {
            return true;
        }
        if (top < work.top)
        {
            return true;
        }
        if (right > work.right)
        {
            return true;
        }
        if (bottom > work.bottom)
        {
            return true;
        }

        return false;
    }

    ///fixes the taskbar being over the window when first created when window is smaller than max res & overlaps the taskbar.
    void EnsureInitialTopmostAndFocus(HWND hWnd)
    {
        static bool bLikeAVirgin = true;
        if (!bLikeAVirgin || hWnd == nullptr || GetPropA(hWnd, kInitFocusProp) != nullptr)
        {
            return;
        }
        bLikeAVirgin = false; //touched for the very first time

        spdlog::info("CreateWindowExA: Checking if EnsureInitialTopmostAndFocus is needed...");
        
        if (!CustomResolutionAndBorderless::bWindowedMode)
        {
            spdlog::info("CreateWindowExA: Not in windowed mode, skipping EnsureInitialTopmostAndFocus.");
            return;
        }

        if (CustomResolutionAndBorderless::bBorderlessMode && (CustomResolutionAndBorderless::bUsingAutomaticOutputY || (CustomResolutionAndBorderless::iFinalWindowResolutionY >= CustomResolutionAndBorderless::DesktopDimensions.second)))
        {
            spdlog::info("CreateWindowExA: Detected fullscreen mode, skipping EnsureInitialTopmostAndFocus.");
            return;
        }
        if (!IsOverlappingTaskbarArea(hWnd))
        {
            spdlog::info("CreateWindowExA: Window is not overlapping taskbar area, skipping EnsureInitialTopmostAndFocus.");
            return;
        }
        else
        {
            spdlog::info("CreateWindowExA: Window is overlapping taskbar area, EnsureInitialTopmostAndFocus will be applied.");
        }
        if (g_Logging.bConsoleShown)
        {
            static bool bAlreadyWarned = false;
            if (bAlreadyWarned)
            {
                return;
            }
            bAlreadyWarned = true;
            spdlog::warn("CreateWindowExA: Skipping EnsureInitialTopmostAndFocus because console is shown. This may cause focus issues. If you want to fix this, close the console and restart the game.");
            return;
        }

        SetPropA(hWnd, kInitFocusProp, (HANDLE)1);

        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);

        HWND fg = GetForegroundWindow();
        DWORD fgThread = fg ? GetWindowThreadProcessId(fg, nullptr) : 0;
        DWORD wndThread = GetWindowThreadProcessId(hWnd, nullptr);

        if (fgThread && wndThread && fgThread != wndThread)
        {
            AttachThreadInput(fgThread, wndThread, TRUE);
        }

        SetForegroundWindow(hWnd);
        SetActiveWindow(hWnd);
        SetFocus(hWnd);

        if (fgThread && wndThread && fgThread != wndThread)
        {
            AttachThreadInput(fgThread, wndThread, FALSE);
        }

        spdlog::info("CreateWindowExA: EnsureInitialTopmostAndFocus applied.");
    }

    LRESULT CALLBACK FocusTopmostWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_SHOWWINDOW && wParam != 0)
        {
            EnsureInitialTopmostAndFocus(hWnd);
        }
        else if (msg == WM_ACTIVATEAPP)
        {
            const bool active = (wParam != 0);
            SetWindowPos(hWnd, active ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

            if (active)
            {
                EnsureInitialTopmostAndFocus(hWnd);
            }
        }
        else if (msg == WM_NCDESTROY || msg == WM_DESTROY)
        {
            RemovePropA(hWnd, kInitFocusProp);

            WNDPROC orig = (WNDPROC)GetPropA(hWnd, kOrigWndProcProp);
            if (orig != nullptr)
            {
                RemovePropA(hWnd, kOrigWndProcProp);
                SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR)orig);
                return CallWindowProcA(orig, hWnd, msg, wParam, lParam);
            }
        }

        WNDPROC orig = (WNDPROC)GetPropA(hWnd, kOrigWndProcProp);
        return orig ? CallWindowProcA(orig, hWnd, msg, wParam, lParam) : DefWindowProcA(hWnd, msg, wParam, lParam);
    }

    SafetyHookInline CreateWindowExA_hook {};

    void SubclassAndKick(HWND hWnd)
    {
        if (hWnd == nullptr)
        {
            return;
        }

        if (GetPropA(hWnd, kOrigWndProcProp) == nullptr)
        {
            WNDPROC orig = (WNDPROC)SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR)FocusTopmostWndProc);
            SetPropA(hWnd, kOrigWndProcProp, (HANDLE)orig);
        }

        EnsureInitialTopmostAndFocus(hWnd);
    }

    HWND WINAPI CreateWindowExA_hooked(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        const char* sClassName = "CSD3DWND";

        if (lpClassName != nullptr && std::strcmp(lpClassName, sClassName) == 0 && !(eGameType & UNKNOWN))
        {
            if (CustomResolutionAndBorderless::bBorderlessMode)
            {
                spdlog::info("CreateWindowExA: Borderless: ClassName = {}, WindowName = {}, dwStyle = 0x{:08X}, dwExStyle = 0x{:08X}, X = {}, Y = {}, nWidth = {}, nHeight = {}",
                             lpClassName ? lpClassName : "<null>",
                             lpWindowName ? lpWindowName : "<null>",
                             (uint32_t)dwStyle,
                             (uint32_t)dwExStyle,
                             X, Y, nWidth, nHeight);

                HWND hWnd = CreateWindowExA_hook.stdcall<HWND>(dwExStyle, lpClassName, lpWindowName, WS_POPUP | WS_VISIBLE, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

                {
                    LONG_PTR style = GetWindowLongPtrA(hWnd, GWL_STYLE);
                    style &= ~WS_OVERLAPPEDWINDOW;
                    style |= WS_POPUP | WS_VISIBLE;
                    SetWindowLongPtrA(hWnd, GWL_STYLE, style);

                    LONG_PTR exStyle = GetWindowLongPtrA(hWnd, GWL_EXSTYLE);
                    exStyle &= ~WS_EX_TOOLWINDOW;
                    exStyle |= WS_EX_APPWINDOW;
                    SetWindowLongPtrA(hWnd, GWL_EXSTYLE, exStyle);
                }

                HMONITOR mon = CustomResolutionAndBorderless::bLimitToPrimaryMonitor ? MonitorFromPoint(POINT { 0, 0 }, MONITOR_DEFAULTTOPRIMARY) : MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

                MONITORINFO mi {};
                mi.cbSize = sizeof(mi);
                GetMonitorInfoA(mon, &mi);

                const int monX = mi.rcMonitor.left;
                const int monY = mi.rcMonitor.top;
                const int monW = mi.rcMonitor.right - mi.rcMonitor.left;
                const int monH = mi.rcMonitor.bottom - mi.rcMonitor.top;

                spdlog::info("CreateWindowExA: Borderless: LimitToPrimaryMonitor = {}", CustomResolutionAndBorderless::bLimitToPrimaryMonitor);
                spdlog::info("CreateWindowExA: Borderless: Monitor rcMonitor = L={}, T={}, R={}, B={} ({}x{})", mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom, monW, monH);

                int finalW = CustomResolutionAndBorderless::iFinalWindowResolutionX;
                int finalH = CustomResolutionAndBorderless::iFinalWindowResolutionY;

                if (finalW <= 0 || finalH <= 0)
                {
                    finalW = CustomResolutionAndBorderless::DesktopDimensions.first > 0 ? CustomResolutionAndBorderless::DesktopDimensions.first : monW;
                    finalH = CustomResolutionAndBorderless::DesktopDimensions.second > 0 ? CustomResolutionAndBorderless::DesktopDimensions.second : monH;

                    spdlog::info("CreateWindowExA: Borderless: Final size was invalid, falling back: DesktopDimensions = {}x{}, Using = {}x{}",
                                 CustomResolutionAndBorderless::DesktopDimensions.first,
                                 CustomResolutionAndBorderless::DesktopDimensions.second,
                                 finalW,
                                 finalH);
                }

                const int posX = monX + (monW - finalW) / 2;
                const int posY = monY + (monH - finalH) / 2;

                spdlog::info("CreateWindowExA: Borderless: SetWindowPos to X = {}, Y = {}, cx = {}, cy = {}", posX, posY, finalW, finalH);

                SetWindowPos(hWnd, HWND_TOPMOST, posX, posY, finalW, finalH, SWP_FRAMECHANGED | SWP_SHOWWINDOW);

                SubclassAndKick(hWnd);

                g_D3D11Hooks.MainHwnd = hWnd;
                return hWnd;
            }

            if (CustomResolutionAndBorderless::bWindowedMode)
            {
                spdlog::info("CreateWindowExA: Windowed: ClassName = {}, WindowName = {}, dwStyle = 0x{:08X}, dwExStyle = 0x{:08X}, X = {}, Y = {}, nWidth = {}, nHeight = {}",
                             lpClassName ? lpClassName : "<null>",
                             lpWindowName ? lpWindowName : "<null>",
                             (uint32_t)dwStyle,
                             (uint32_t)dwExStyle,
                             X, Y, nWidth, nHeight);

                HWND hWnd = CreateWindowExA_hook.stdcall<HWND>(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

                spdlog::info("CreateWindowExA: Windowed: Forcing output size: iOutputResX = {}, iOutputResY = {}", CustomResolutionAndBorderless::iOutputResX, CustomResolutionAndBorderless::iOutputResY);
                spdlog::info("CreateWindowExA: Windowed: SetWindowPos to X = {}, Y = {}, cx = {}, cy = {}", 0, 0, CustomResolutionAndBorderless::iOutputResX, CustomResolutionAndBorderless::iOutputResY);

                SetWindowPos(hWnd, HWND_TOP, 0, 0, CustomResolutionAndBorderless::iOutputResX, CustomResolutionAndBorderless::iOutputResY, SWP_FRAMECHANGED | SWP_SHOWWINDOW);

                SubclassAndKick(hWnd);

                g_D3D11Hooks.MainHwnd = hWnd;
                return hWnd;
            }
        }

        spdlog::info("CreateWindowExA: Passing through original CreateWindowExA");

        g_D3D11Hooks.MainHwnd = CreateWindowExA_hook.stdcall<HWND>(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
        return g_D3D11Hooks.MainHwnd;
    }
}

namespace CustomResolutionAndBorderless
{


    void Init_FixDPIScaling()
    {
        if (eGameType & (MG | MGS2 | MGS3))
        {
            SetProcessDPIAware();
            spdlog::info("MG/MG2 | MGS 2 | MGS 3: High-DPI scaling fixed.");
        }
    }


    void Init_CalculateScreenSize()
    {
        // Calculate aspect ratio
        fHeightDeltaFrom720p = static_cast<float>(iInternalResY) / 720.0f;
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



    void Init_CustomResolution()
    {
        if (eGameType & (MG | MGS2 | MGS3) && bOutputResolution)
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
                    static SafetyHookMid FSRWarningMidHook {};
                    FSRWarningMidHook = safetyhook::create_mid(MGS2_MGS3_FSR_Result,
                                                               [](SafetyHookContext& ctx)
                                                               {



                                                                   static bool bFSRWarningShown = false;
                                                                   if (!bEnableFSRWarning || bFSRWarningShown)
                                                                   {
                                                                       return;
                                                                   }
                                                                   bFSRWarningShown = true;
                                                                   spdlog::warn("----------");
                                                                   spdlog::warn("WARNING: The game launcher's built-in AMD FSR Upscaling resolution/graphical options are enabled!");
                                                                   spdlog::warn("WARNING: MGSHDFix already handles increasing the game's resolution.");
                                                                   spdlog::warn("WARNING: Unintended side effects, e.g. pixelization, mipmap issues (oversharpening on textures), and crashing, may occur while the game's built-in settings are enabled!");
                                                                   spdlog::warn("WARNING: It's advised to set both Internal Resolution & Internal Upscaling graphical options in the game's main launcher to default/original unless ABSOLUTELY necessary! (such as some systems crashing during Stillman's tutorial)");
                                                                   spdlog::warn("----------");
                                                                   Logging::ShowConsole();
                                                                   std::cout << "MGSHDFix WARNING\n"
                                                                       "The game launcher's built-in AMD FSR Upscaling resolution/graphical options are enabled!\n"
                                                                       "\n"
                                                                       "MGSHDFix already handles increasing the game's resolution.\n"
                                                                       "Unintended side effects, e.g. pixelization, mipmap issues (oversharpening on textures), and crashing, may occur while the game's built-in settings are enabled!\n"
                                                                       "\n"
                                                                       "It's advised to set both Internal Resolution & Internal Upscaling graphical options in the game's main launcher to default/original unless ABSOLUTELY necessary! (such as some systems crashing during Stillman's tutorial)\n"
                                                                       "\n"
                                                                       "This warning can be muted on the config tool's \"MGSHDfix / Internal\" settings page." << std::endl;


                                                               });

                }

                // Output resolution 1
                static SafetyHookMid OutputResolution1MidHook {};
                OutputResolution1MidHook = safetyhook::create_mid(MGS2_MGS3_OutputResolution1ScanResult,
                                                                  [](SafetyHookContext& ctx)
                                                                  {
                                                                      ctx.rbx = iOutputResX;
                                                                      ctx.rdi = iOutputResY;
                                                                  });

                // Output resolution 2
                static SafetyHookMid OutputResolution2MidHook {};
                OutputResolution2MidHook = safetyhook::create_mid(MGS2_MGS3_OutputResolution2ScanResult,
                                                                  [](SafetyHookContext& ctx)
                                                                  {
                                                                      ctx.r8 = iOutputResX;
                                                                      ctx.r9 = iOutputResY;
                                                                  });

                // Internal resolution
                static SafetyHookMid InternalResolutionMidHook {};
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
                            static SafetyHookMid MGS2_MGS3_SplashScreenMidHook {};
                            MGS2_MGS3_SplashScreenMidHook = safetyhook::create_mid(MGS2_MGS3_SplashscreenResult,
                                                                                   [](SafetyHookContext& ctx)
                                                                                   {
                                                                                       std::string fileName = reinterpret_cast<char*>(ctx.rdx);
                                                                                       if (fileName.ends_with("_720.ctxr"))
                                                                                       {
                                                                                           fileName.replace(fileName.end() - 8, fileName.begin(), iOutputResY >= 2160 ? "4k.ctxr" :
                                                                                                            iOutputResY >= 1440 ? "wqhd.ctxr" :
                                                                                                            /*iOutputResY >= 1080*/ "fhd.ctxr");
                                                                                           ctx.rdx = reinterpret_cast<uintptr_t>(fileName.c_str());
                                                                                       }
                                                                                   });
                            LOG_HOOK(MGS2_MGS3_SplashScreenMidHook, "MGS 2 | MGS 3: Custom Resolution")
                        }

                        if (uint8_t* MGS2_MGS3_LoadingScreenEngScanResult = Memory::PatternScan(baseModule, "48 8D 8C 24 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 8C 24 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 8C 24 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 8C 24", "MGS 2 | MGS 3: Custom Resolution: Loading Screen (ENG)"))
                        {
                            static SafetyHookMid MGS2_MGS3_LoadingScreenEngMidHook {};
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
                            static SafetyHookMid MGS2_MGS3_LoadingScreenJPMidHook {};
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
                static SafetyHookMid WindowedModeMidHook {};
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
                    if (bBorderlessMode || bWindowedMode)
                    {
                        const int desktopW = DesktopDimensions.first;
                        const int desktopH = DesktopDimensions.second;

                        const float desktopAspect = (float)desktopW / (float)desktopH;

                        constexpr float EPSILON = 1e-6f;

                        const bool aspectsMatch = std::abs(fAspectRatio - desktopAspect) < EPSILON;
                        const bool clampToWidth = desktopAspect < fAspectRatio;

                        const bool internalMeetsDesktop = clampToWidth ? (iInternalResX >= desktopW) : (iInternalResY >= desktopH);
                        const bool shouldMaximize = bMaximizeBorderless || internalMeetsDesktop;

                        int finalX = 0;
                        int finalY = 0;

                        const bool manualX = !bUsingAutomaticOutputX;
                        const bool manualY = !bUsingAutomaticOutputY;

                        spdlog::info("AspectClamp: Desktop = {}x{}, Internal = {}x{}, AspectInternal = {:.6f}, AspectDesktop = {:.6f}", desktopW, desktopH, iInternalResX, iInternalResY, fAspectRatio, desktopAspect);
                        spdlog::info("AspectClamp: aspectsMatch = {}, clampToWidth = {}, internalMeetsDesktop = {}, shouldMaximize = {}", aspectsMatch, clampToWidth, internalMeetsDesktop, shouldMaximize);
                        spdlog::info("AspectClamp: manualX = {}, manualY = {}, iOriginalOutputResX = {}, iOriginalOutputResY = {}", manualX, manualY, iOriginalOutputResX, iOriginalOutputResY);

                        if (manualX || manualY)
                        {
                            if (manualX)
                            {
                                finalX = iOriginalOutputResX;
                            }
                            if (manualY)
                            {
                                finalY = iOriginalOutputResY;
                            }

                            spdlog::info("AspectClamp: Manual: start finalX = {}, finalY = {}", finalX, finalY);

                            if (!manualX || finalX <= 0)
                            {
                                if (finalY > 0)
                                {
                                    finalX = (int)std::lround((double)finalY * (double)fAspectRatio);
                                }
                            }
                            if (!manualY || finalY <= 0)
                            {
                                if (finalX > 0)
                                {
                                    finalY = (int)std::lround((double)finalX / (double)fAspectRatio);
                                }
                            }

                            spdlog::info("AspectClamp: Manual: derived finalX = {}, finalY = {}", finalX, finalY);

                            if (finalX <= 0) { finalX = iInternalResX; }
                            if (finalY <= 0) { finalY = iInternalResY; }

                            if (finalX > desktopW || finalY > desktopH)
                            {
                                spdlog::info("AspectClamp: Manual: clamping to desktop {}x{}", desktopW, desktopH);

                                finalX = std::min(finalX, desktopW);
                                finalY = (int)std::lround((double)finalX / (double)fAspectRatio);

                                if (finalY > desktopH)
                                {
                                    finalY = desktopH;
                                    finalX = (int)std::lround((double)finalY * (double)fAspectRatio);
                                }
                            }

                            spdlog::info("AspectClamp: Manual: final window size = {}x{}", finalX, finalY);
                        }
                        else if (aspectsMatch)
                        {
                            spdlog::info("AspectClamp: MATCH branch");

                            if (shouldMaximize)
                            {
                                finalX = desktopW;
                                finalY = desktopH;
                                spdlog::info("AspectClamp: MATCH maximize -> desktop {}x{}", finalX, finalY);
                            }
                            else
                            {
                                finalX = iInternalResX;
                                finalY = iInternalResY;
                                spdlog::info("AspectClamp: MATCH use internal {}x{}", finalX, finalY);

                                if (finalX > desktopW || finalY > desktopH)
                                {
                                    spdlog::info("AspectClamp: MATCH internal exceeds desktop, clamping");

                                    finalX = desktopW;
                                    finalY = (int)std::lround((double)finalX / (double)fAspectRatio);

                                    if (finalY > desktopH)
                                    {
                                        finalY = desktopH;
                                        finalX = (int)std::lround((double)finalY * (double)fAspectRatio);
                                    }
                                }
                            }
                        }
                        else
                        {
                            spdlog::info("AspectClamp: DIFFER branch");

                            if (!shouldMaximize)
                            {
                                finalX = iInternalResX;
                                finalY = iInternalResY;
                                spdlog::info("AspectClamp: DIFFER not maximizing -> internal {}x{}", finalX, finalY);

                                if (finalX > desktopW || finalY > desktopH)
                                {
                                    spdlog::info("AspectClamp: DIFFER internal exceeds desktop, clamping");

                                    finalX = desktopW;
                                    finalY = (int)std::lround((double)finalX / (double)fAspectRatio);

                                    if (finalY > desktopH)
                                    {
                                        finalY = desktopH;
                                        finalX = (int)std::lround((double)finalY * (double)fAspectRatio);
                                    }
                                }
                            }
                            else if (clampToWidth)
                            {
                                finalX = desktopW;
                                finalY = (int)std::lround((double)finalX / (double)fAspectRatio);
                                spdlog::info("AspectClamp: DIFFER clamp X -> {}x{}", finalX, finalY);

                                if (finalY > desktopH)
                                {
                                    spdlog::info("AspectClamp: DIFFER secondary clamp to Y");

                                    finalY = desktopH;
                                    finalX = (int)std::lround((double)finalY * (double)fAspectRatio);
                                }
                            }
                            else
                            {
                                finalY = desktopH;
                                finalX = (int)std::lround((double)finalY * (double)fAspectRatio);
                                spdlog::info("AspectClamp: DIFFER clamp Y -> {}x{}", finalX, finalY);

                                if (finalX > desktopW)
                                {
                                    spdlog::info("AspectClamp: DIFFER secondary clamp to X");

                                    finalX = desktopW;
                                    finalY = (int)std::lround((double)finalX / (double)fAspectRatio);
                                }
                            }
                        }

                        iFinalWindowResolutionX = finalX;
                        iFinalWindowResolutionY = finalY;

                        spdlog::info("AspectClamp: FINAL iFinalWindowResolution = {}x{}, Internal = {}x{}, Desktop = {}x{}", iFinalWindowResolutionX, iFinalWindowResolutionY, iInternalResX, iInternalResY, desktopW, desktopH);
                    }

                    static SafetyHookMid SetWindowPosMidHook {};
                    SetWindowPosMidHook = safetyhook::create_mid(MGS2_MGS3_SetWindowPosScanResult,
                                                                 [](SafetyHookContext& ctx)
                                                                 {
                                                                     if (bBorderlessMode || bWindowedMode)
                                                                     {
                                                                         const int desktopW = (int)DesktopDimensions.first;
                                                                         const int desktopH = (int)DesktopDimensions.second;

                                                                         const bool isBorderless = bBorderlessMode;

                                                                         const int outW = iFinalWindowResolutionX + (isBorderless ? 0 : 18);
                                                                         const int outH = iFinalWindowResolutionY + (isBorderless ? 0 : 47);

                                                                         int offsetX = (desktopW - outW) / 2;
                                                                         int offsetY = (desktopH - outH) / 2;

                                                                         if (isBorderless)
                                                                         {
                                                                             if (offsetX < 0) offsetX = 0;
                                                                             if (offsetY < 0) offsetY = 0;
                                                                         }

                                                                         spdlog::info("SetWindowPos: {}: Desktop = {}x{}, Final = {}x{}, Out = {}x{}, Internal = {}x{}, Offset = {},{}",
                                                                                      isBorderless ? "Borderless" : "Windowed",
                                                                                      desktopW, desktopH,
                                                                                      iFinalWindowResolutionX, iFinalWindowResolutionY,
                                                                                      outW, outH,
                                                                                      iInternalResX, iInternalResY,
                                                                                      offsetX, offsetY);

                                                                         ctx.r8 = (uint64_t)(int64_t)offsetX;
                                                                         ctx.r9 = (uint64_t)(int64_t)offsetY;

                                                                         *reinterpret_cast<int*>(ctx.rsp + 0x20) = outW;
                                                                         *reinterpret_cast<int*>(ctx.rsp + 0x28) = outH;
                                                                     }
                                                                 });

                    LOG_HOOK(SetWindowPosMidHook, "MG 1/2 | MGS 2 | MGS 3: SetWindowPos")
                }
        }
    


        // MGS 2 | MGS 3: Framebuffer fix, stops the framebuffer from being set to maximum display resolution.
        // Thanks emoose!
        if (bFramebufferFix && eGameType & (MG | MGS2 | MGS3))
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
                if (eGameType & MG | MGS3)
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


    void Init_ScaleEffects()
    {
        if ((eGameType & (MGS2 | MGS3)) && bOutputResolution)
        {
            // MGS 2 | MGS 3: Fix scaling for added volume menu in v1.4.0 patch
            if (uint8_t* MGS2_MGS3_VolumeMenuScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? 48 ?? ?? ?? 89 ?? ?? ?? 00 00 F3 0F ?? ?? 89 ?? ?? ?? 00 00", "MGS 2 | MGS 3: Volume Menu"))
            {
                static SafetyHookMid MGS2_MGS3_VolumeMenuMidHook {};
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

                static SafetyHookMid ScaleEffectsXMidHook {};
                ScaleEffectsXMidHook = safetyhook::create_mid(MGS2_ScaleEffectsScanResult,
                                                              [](SafetyHookContext& ctx)
                                                              {
                                                                  ctx.xmm1.f32[0] = fMGS2_EffectScaleX;
                                                              });

                static SafetyHookMid ScaleEffectsX2MidHook {};
                ScaleEffectsX2MidHook = safetyhook::create_mid(MGS2_ScaleEffectsScanResult - 0x2B,
                                                               [](SafetyHookContext& ctx)
                                                               {
                                                                   ctx.xmm1.f32[0] = fMGS2_EffectScaleX;
                                                               });

                static SafetyHookMid ScaleEffectsYMidHook {};
                ScaleEffectsYMidHook = safetyhook::create_mid(MGS2_ScaleEffectsScanResult + 0x2C,
                                                              [](SafetyHookContext& ctx)
                                                              {
                                                                  ctx.xmm1.f32[0] = fMGS2_EffectScaleY;
                                                              });
            }
        }

    }




    void Init_AspectFOVFix()
    {
        if (std::abs(fAspectRatio - fNativeAspect) < 0.0001f)
        {
            spdlog::info("Aspect ratio matches native aspect ratio. Skipping {} and {}.", ConfigKeys::FixAspectRatio_Setting, ConfigKeys::FixFOV_Setting);
            return;
        }
        // Fix aspect ratio
        if (eGameType & MGS3 && bAspectFix)
        {
            // MGS 3: Fix gameplay aspect ratio
            if (uint8_t* MGS3_GameplayAspectScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? E8 ?? ?? ?? ?? 48 8D ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ??", "MGS 3: Aspect Ratio"))
            {
                DWORD64 MGS3_GameplayAspectAddress = Memory::GetAbsolute((uintptr_t)MGS3_GameplayAspectScanResult + 0x5);
                spdlog::info("MGS 3: Aspect Ratio: Function address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS3_GameplayAspectAddress - (uintptr_t)baseModule);

                static SafetyHookMid MGS3_GameplayAspectMidHook {};
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

                static SafetyHookMid MGS2_GameplayAspectMidHook {};
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
                static SafetyHookMid MGS3_FOVMidHook {};
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
                static SafetyHookMid MGS2_FOVMidHook {};
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

    void Init_HUDFix()
    {
        if (std::abs(fAspectRatio - fNativeAspect) < 0.0001f)
        {
            spdlog::info("Aspect ratio matches native aspect ratio. Skipping {}.", ConfigKeys::FixHUD_Setting);
            return;
        }
        if (eGameType & MGS2 && bHUDFix)
        {
            // MGS 2: HUD
            uint8_t* MGS2_HUDWidthScanResult = Memory::PatternScanSilent(baseModule, "E9 ?? ?? ?? ?? F3 0F ?? ?? ?? 0F ?? ?? F3 0F ?? ?? ?? F3 0F ?? ??");
            if (MGS2_HUDWidthScanResult)
            {
                spdlog::info("MGS 2: HUD: Address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS2_HUDWidthScanResult - (uintptr_t)baseModule);

                static SafetyHookMid MGS2_HUDWidthMidHook {};
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

                static SafetyHookMid MGS2_RadarWidthMidHook {};
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

                static SafetyHookMid MGS2_RadarWidthOffsetMidHook {};
                MGS2_RadarWidthOffsetMidHook = safetyhook::create_mid(MGS2_RadarWidthOffsetAddress,
                                                                      [](SafetyHookContext& ctx)
                                                                      {
                                                                          ctx.rax += (int)fHUDWidthOffset;
                                                                      });

                // Radar height offset
                DWORD64 MGS2_RadarHeightOffsetAddress = (uintptr_t)MGS2_RadarWidthScanResult + 0x90;
                spdlog::info("MGS 2: Radar Height Offset: Hook address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS2_RadarHeightOffsetAddress - (uintptr_t)baseModule);

                static SafetyHookMid MGS2_RadarHeightOffsetMidHook {};
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

                static SafetyHookMid MGS2_CodecPortraitsMidHook {};
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

            if (fAspectRatio > fNativeAspect)
            {
                if (uint8_t* MGS2_MotionBlurScanResult = Memory::PatternScanSilent(baseModule, "F3 48 ?? ?? ?? ?? 48 ?? ?? ?? 48 ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ??"))
                {
                    Memory::PatchBytes((uintptr_t)MGS2_MotionBlurScanResult, "\x48\x31\xDB\x90\x90\x90", 6);
                }
            }

        }
        else if (eGameType & MGS3 && bHUDFix || eGameType & MG)
        {
            // MG1/2 | MGS 3: HUD
            uint8_t* MGS3_HUDWidthScanResult = Memory::PatternScanSilent(baseModule, "0F ?? ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? ?? 4C ?? ?? ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? ?? 41 ?? 00 02 00 00");
            if (MGS3_HUDWidthScanResult)
            {
                static SafetyHookMid MGS3_HUDWidthMidHook {};
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

        if ((eGameType & (MG | MGS2 | MGS3)) && bHUDFix)
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
}
