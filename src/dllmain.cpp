#include "stdafx.h"
#include "helper.hpp"

using namespace std;

HMODULE baseModule = GetModuleHandle(NULL);

inipp::Ini<char> ini;

// INI Variables
bool bAspectFix;
bool bHUDFix;
bool bMovieFix;
bool bCustomResolution;
bool bWindowedMode;
bool bBorderlessMode;
bool bDisableCursor;
int iCustomResX;
int iCustomResY;
int iInjectionDelay;
int iAspectFix;
int iHUDFix;
int iMovieFix;

// Variables
float fNewX;
float fNewY;
float fNativeAspect = (float)16/9;
float fPi = 3.14159265358979323846f;
float fNewAspect;
float fAspectDivisional;
float fAspectMultiplier;
float fHUDWidth;
float fHUDOffset;
float fMGS2_DefaultHUDX = (float)1280;
float fMGS2_DefaultHUDY = (float)720;
string sExeName;
string sGameName;
string sExePath;
string sGameVersion;
string sFixVer = "0.6";

// MGS 2: Aspect Ratio Hook
DWORD64 MGS2_GameplayAspectReturnJMP;
void __declspec(naked) MGS2_GameplayAspect_CC()
{
    __asm
    {
        xorps xmm1, xmm1
        cmove rax, rcx
        movss xmm0, [rax + 0x14]
        divss xmm0, [fAspectMultiplier]
        ret
    }
}

// MGS 3: Aspect Ratio Hook
DWORD64 MGS3_GameplayAspectReturnJMP;
void __declspec(naked) MGS3_GameplayAspect_CC()
{
    __asm
    {
        xorps xmm0, xmm0
        cmove rax, rcx
        movss xmm1, [rax + 0x14]
        divss xmm1, [fAspectMultiplier]
        movaps xmm0,xmm1
        ret
    }
}

/*
// MGS 3: HUD Width Hook
DWORD64 MGS3_HUDWidthReturnJMP;
void __declspec(naked) MGS3_HUDWidth_CC()
{
    __asm
    {
        movss xmm0, [rdi + 0x0C]
        mulss xmm0, [fAspectMultiplier]
        movaps xmm1, xmm14
        subss xmm0, [rdi + 0x04]
        jmp[MGS3_HUDWidthReturnJMP]
    }
}


// MGS 2: HUD Width Hook
DWORD64 MGS2_HUDWidthReturnJMP;
float fMGS2_DefaultHUDWidth = (float)512;
void __declspec(naked) MGS2_HUDWidth_CC()
{
    __asm
    {
        movss xmm0, [rsp + 0x28]
        lea rax, [rcx + 0x18]
        movss xmm15, [fMGS2_DefaultHUDWidth]
        comiss xmm3, xmm15
        je scaleHUD
        xorps xmm15, xmm15
        xorps xmm14, xmm14
        movss[rcx + 0x14], xmm0
        jmp[MGS2_HUDWidthReturnJMP]

        scaleHUD:
            movss xmm15, xmm3
            movss xmm14, xmm3
            divss xmm14, xmm0
            mulss xmm3, xmm14
            subss xmm15, xmm3
            movss xmm1, xmm15
            xorps xmm15, xmm15
            xorps xmm14, xmm14
            movss[rcx + 0x14], xmm0
            jmp[MGS2_HUDWidthReturnJMP]
    }
}
*/

// MGS 2: Effects Scale X Hook
DWORD64 MGS2_EffectsScaleXReturnJMP;
void __declspec(naked) MGS2_EffectsScaleX_CC()
{
    __asm
    {
        mov rcx, [rbp - 0x60]
        movd xmm0, eax
        cvtdq2ps xmm0, xmm0
        movss xmm0, [fMGS2_DefaultHUDX]
        divss xmm1, xmm0
        jmp[MGS2_EffectsScaleXReturnJMP]
    }
}

// MGS 2: Effects Scale Y Hook
DWORD64 MGS2_EffectsScaleYReturnJMP;
void __declspec(naked) MGS2_EffectsScaleY_CC()
{
    __asm
    {
        movaps [rsp + 0x00000490], xmm6
        cvtdq2ps xmm0, xmm0
        movss xmm0, [fMGS2_DefaultHUDY]
        movaps [rsp + 0x00000470], xmm8
        jmp[MGS2_EffectsScaleYReturnJMP]
    }
}

// MGS 2: Borderless Hook
DWORD64 MGS2_CreateWindowExAReturnJMP;
void __declspec(naked) MGS2_CreateWindowExA_CC()
{
    __asm
    {
        mov ecx, [r13 + 0x08]
        mov rax, [rbp - 0x68]
        mov r9d, [rbp + 0x000000B0]
        mov r9d, 0x90000000                 // WS_VISIBLE + WS_POPUP
        jmp[MGS2_CreateWindowExAReturnJMP]
    }
}

// MGS 3: Borderless Hook
DWORD64 MGS3_CreateWindowExAReturnJMP;
void __declspec(naked) MGS3_CreateWindowExA_CC()
{
    __asm
    {
        mov rax, [rbp + 0x000000C8]
        mov [rsp + 0x48], r12
        mov [rsp + 0x40], rax
        mov r9d, 0x90000000                 // WS_VISIBLE + WS_POPUP
        jmp[MGS3_CreateWindowExAReturnJMP]
    }
}

/*
// MGS 2: Movie Hook
DWORD64 MGS2_MovieReturnJMP;
float MGS2_fMovieOffset;
float MGS2_fMovieWidth;
void __declspec(naked) MGS2_Movie_CC()
{
    __asm
    {
        movss xmm1, [MGS2_fMovieOffset]
        movss xmm3, [MGS2_fMovieWidth]
        addss xmm3, xmm1
        mov rcx, rdi
        movss[rsp + 0x20], xmm0
        mov rbx, rax
        jmp[MGS2_MovieReturnJMP]
    }
}

// MGS 3: Movie Hook
DWORD64 MGS3_MovieReturnJMP;
float MGS3_fMovieOffset;
float MGS3_fMovieWidth;
void __declspec(naked) MGS3_Movie_CC()
{
    __asm
    {
        xorps xmm2, xmm2
        xorps xmm1, xmm1
        movss xmm3, [MGS3_fMovieWidth]
        movss xmm1, [MGS3_fMovieOffset]
        addss xmm3, [MGS3_fMovieOffset]
        jmp[MGS3_MovieReturnJMP]
    }
}
*/

void Logging()
{
    loguru::add_file("MGSHDFix.log", loguru::Truncate, loguru::Verbosity_MAX);
    loguru::set_thread_name("Main");

    LOG_F(INFO, "MGSHDFix v%s loaded", sFixVer.c_str());
}

void ReadConfig()
{
    // Initialise config
    std::ifstream iniFile(".\\MGSHDFix.ini");
    if (!iniFile)
    {
        LOG_F(ERROR, "Failed to load config file.");
    }
    else
    {
        ini.parse(iniFile);
    }

    inipp::get_value(ini.sections["MGSHDFix Parameters"], "InjectionDelay", iInjectionDelay);
    inipp::get_value(ini.sections["Custom Resolution"], "Enabled", bCustomResolution);
    inipp::get_value(ini.sections["Custom Resolution"], "Width", iCustomResX);
    inipp::get_value(ini.sections["Custom Resolution"], "Height", iCustomResY);
    inipp::get_value(ini.sections["Custom Resolution"], "Windowed", bWindowedMode);
    inipp::get_value(ini.sections["Custom Resolution"], "Borderless", bBorderlessMode);
    inipp::get_value(ini.sections["Disable Mouse Cursor"], "Enabled", bDisableCursor);
    inipp::get_value(ini.sections["Fix Aspect Ratio"], "Enabled", bAspectFix);
    iAspectFix = (int)bAspectFix;
    inipp::get_value(ini.sections["Fix HUD"], "Enabled", bHUDFix);
    iHUDFix = (int)bHUDFix;
    inipp::get_value(ini.sections["Fix FMVs"], "Enabled", bMovieFix);
    iMovieFix = (int)bMovieFix;

    // Log config parse
    LOG_F(INFO, "Config Parse: iInjectionDelay: %dms", iInjectionDelay);
    LOG_F(INFO, "Config Parse: bCustomResolution: %d", bCustomResolution);
    LOG_F(INFO, "Config Parse: iCustomResX: %d", iCustomResX);
    LOG_F(INFO, "Config Parse: iCustomResY: %d", iCustomResY);
    LOG_F(INFO, "Config Parse: bWindowedMode: %d", bWindowedMode);
    LOG_F(INFO, "Config Parse: bBorderlessMode: %d", bBorderlessMode);
    LOG_F(INFO, "Config Parse: bAspectFix: %d", bAspectFix);
    //LOG_F(INFO, "Config Parse: bHUDFix: %d", bHUDFix);
    //LOG_F(INFO, "Config Parse: bMovieFix: %d", bMovieFix);

    // Force windowed mode if borderless is enabled but windowed is not. There is undoubtedly a more elegant way to handle this.
    if (bBorderlessMode)
    {
        bWindowedMode = true;
        LOG_F(INFO, "Config Parse: Borderless mode enabled.");
    }

    // Custom resolution
    if (iCustomResX > 0 && iCustomResY > 0)
    {
        fNewX = (float)iCustomResX;
        fNewY = (float)iCustomResY;
        fNewAspect = (float)iCustomResX / (float)iCustomResY;
    }
    else
    {
        // Grab desktop resolution
        RECT desktop;
        GetWindowRect(GetDesktopWindow(), &desktop);
        fNewX = (float)desktop.right;
        fNewY = (float)desktop.bottom;
        iCustomResX = (int)desktop.right;
        iCustomResY = (int)desktop.bottom;
        fNewAspect = (float)desktop.right / (float)desktop.bottom;
    }

    fAspectMultiplier = (float)fNewAspect / fNativeAspect;
    fHUDWidth = (float)fNewY * fNativeAspect;
    fHUDOffset = (float)(fNewX - fHUDWidth) / 2;
    LOG_F(INFO, "Custom Resolution: fNewAspect: %.4f", fNewAspect);
    LOG_F(INFO, "Custom Resolution: fAspectMultiplier: %.4f", fAspectMultiplier);
    LOG_F(INFO, "Custom Resolution: fHUDWidth: %.4f", fHUDWidth);
    LOG_F(INFO, "Custom Resolution: fHUDOffset: %.4f", fHUDOffset);

    // Disable ultrawide fixes at 16:9
    if (fNewAspect == fNativeAspect)
    {
        bAspectFix = false;
        bMovieFix = false;
        LOG_F(INFO, "Config Parse: Aspect ratio is native, disabling ultrawide fixes.");
    }
}

void DetectGame()
{
    // Get game name and exe path
    LPWSTR exePath = new WCHAR[_MAX_PATH];
    GetModuleFileName(baseModule, exePath, MAX_PATH);
    wstring exePathWString(exePath);
    sExePath = string(exePathWString.begin(), exePathWString.end());
    sExeName = sExePath.substr(sExePath.find_last_of("/\\") + 1);

    LOG_F(INFO, "Game Name: %s", sExeName.c_str());
    LOG_F(INFO, "Game Path: %s", sExePath.c_str());

    if (sExeName == "METAL GEAR SOLID2.exe")
    {
        LOG_F(INFO, "Detected game is: Metal Gear Solid 2 HD");
    }
    else if (sExeName == "METAL GEAR SOLID3.exe")
    {
        LOG_F(INFO, "Detected game is: Metal Gear Solid 3 HD");
    }
}

void CustomResolution()
{
    if (sExeName == "METAL GEAR SOLID2.exe" && bCustomResolution || sExeName == "METAL GEAR SOLID3.exe" && bCustomResolution)
    {
        // MGS 2 | MGS 3: Custom Resolution
        uint8_t* MGS2_MGS3_ResolutionScanResult = Memory::PatternScan(baseModule, "C7 45 ?? 00 05 00 00 C7 ?? ?? D0 02 00 00 C7 ?? ?? 00 05 00 00 C7 ?? ?? D0 02 00 00");
        if (MGS2_MGS3_ResolutionScanResult)
        {
            DWORD64 MGS2_MGS3_ResolutionAddress = (uintptr_t)MGS2_MGS3_ResolutionScanResult + 0x3;
            LOG_F(INFO, "MGS 2 | MGS 3: Custom Resolution: Address is 0x%" PRIxPTR, (uintptr_t)MGS2_MGS3_ResolutionAddress);

            Memory::Write(MGS2_MGS3_ResolutionAddress, iCustomResX);
            Memory::Write((MGS2_MGS3_ResolutionAddress + 0x7), iCustomResY);
            Memory::Write((MGS2_MGS3_ResolutionAddress + 0xE), iCustomResX);
            Memory::Write((MGS2_MGS3_ResolutionAddress + 0x15), iCustomResY);
            LOG_F(INFO, "MGS 2 | MGS 3: Custom Resolution: New Custom Resolution = %dx%d", iCustomResX, iCustomResY);
        }
        else if (!MGS2_MGS3_ResolutionScanResult)
        {
            LOG_F(INFO, "MGS 2 | MGS 3: Custom Resolution: Pattern scan failed.");
        }

        // MGS 2 | MGS 3: Framebuffer fix, stops the framebuffer from being set to maximum display resolution.
        // Thanks emoose!
        for (int i = 1; i <= 2; ++i) // Two results to change, unsure if first result is actually used but we NOP it anyway.
        {
            uint8_t* MGS2_MGS3_FramebufferFixScanResult = Memory::PatternScan(baseModule, "8B ?? ?? 48 ?? ?? ?? 03 C2 89 ?? ??");
            if (MGS2_MGS3_FramebufferFixScanResult)
            {
                DWORD64 MGS2_MGS3_FramebufferFixAddress = (uintptr_t)MGS2_MGS3_FramebufferFixScanResult + 0x9;
                LOG_F(INFO, "MGS 2 | MGS 3: Framebuffer %d: Address is 0x%" PRIxPTR, i, (uintptr_t)MGS2_MGS3_FramebufferFixAddress);

                Memory::PatchBytes(MGS2_MGS3_FramebufferFixAddress, "\x90\x90\x90", 3);
                LOG_F(INFO, "MGS 2 | MGS 3: Framebuffer %d: Patched instruction.", i);
            }
            else if (!MGS2_MGS3_FramebufferFixScanResult)
            {
                LOG_F(INFO, "MGS 2 | MGS 3: Framebuffer %d: Pattern scan failed.", i);
            }
        }

        // MGS 2 | MGS 3: Windowed mode
        // Thanks emoose!
        if (bWindowedMode)
        {
            uint8_t* MGS2_MGS3_WindowModeScanResult = Memory::PatternScan(baseModule, "0F 84 ?? ?? ?? ?? ?? 01 48 ?? ?? E8 ?? ?? ?? ??");
            if (MGS2_MGS3_WindowModeScanResult)
            {
                DWORD64 MGS2_MGS3_WindowModeAddress = (uintptr_t)MGS2_MGS3_WindowModeScanResult + 0x7;
                LOG_F(INFO, "MGS 2 | MGS 3: Window Mode: Address is 0x%" PRIxPTR, (uintptr_t)MGS2_MGS3_WindowModeAddress);

                Memory::PatchBytes(MGS2_MGS3_WindowModeAddress, "\x00", 1);
                LOG_F(INFO, "MGS 2 | MGS 3: Window Mode: Patched instruction.");
            }
            else if (!MGS2_MGS3_WindowModeScanResult)
            {
                LOG_F(INFO, "MGS 2 | MGS 3: Window Mode: Pattern scan failed.");
            }
        }
    }

    if (sExeName == "METAL GEAR SOLID2.exe" && bDisableCursor || sExeName == "METAL GEAR SOLID3.exe" && bDisableCursor)
    {
        // MGS 2 | MGS 3: Disable mouse cursor
        // Thanks again emoose!
        uint8_t* MGS2_MGS3_MouseCursorScanResult = Memory::PatternScan(baseModule, "?? ?? BA ?? ?? 00 00 FF ?? ?? ?? ?? ?? 48 ?? ??");
        if (MGS2_MGS3_MouseCursorScanResult && bWindowedMode)
        {
            DWORD64 MGS2_MGS3_MouseCursorAddress = (uintptr_t)MGS2_MGS3_MouseCursorScanResult;
            LOG_F(INFO, "MGS 2 | MGS 3: Mouse Cursor: Address is 0x%" PRIxPTR, (uintptr_t)MGS2_MGS3_MouseCursorAddress);

            Memory::PatchBytes(MGS2_MGS3_MouseCursorAddress, "\xEB", 1);
            LOG_F(INFO, "MGS 2 | MGS 3: Mouse Cursor: Patched instruction.");
        }
        else if (!MGS2_MGS3_MouseCursorScanResult)
        {
            LOG_F(INFO, "MGS 2 | MGS 3: Mouse Cursor: Pattern scan failed.");
        }
    }

    // MGS 2: Borderless mode
    if (sExeName == "METAL GEAR SOLID2.exe" && bBorderlessMode)
    {
        uint8_t* MGS2_CreateWindowExAScanResult = Memory::PatternScan(baseModule, "41 ?? ?? ?? 48 ?? ?? ?? 44 ?? ?? ?? ?? 00 00 4C ?? ?? ?? ?? 48 ?? ?? ?? ?? 4C ?? ?? ?? ??");
        if (MGS2_CreateWindowExAScanResult)
        {
            DWORD64 MGS2_CreateWindowExAAddress = (uintptr_t)MGS2_CreateWindowExAScanResult + 0x5;
            int MGS2_CreateWindowExAHookLength = Memory::GetHookLength((char*)MGS2_CreateWindowExAAddress, 13);
            MGS2_CreateWindowExAReturnJMP = MGS2_CreateWindowExAAddress + MGS2_CreateWindowExAHookLength;
            Memory::DetourFunction64((void*)MGS2_CreateWindowExAAddress, MGS2_CreateWindowExA_CC, MGS2_CreateWindowExAHookLength);

            LOG_F(INFO, "MGS 2: Borderless: Hook length is %d bytes", MGS2_CreateWindowExAHookLength);
            LOG_F(INFO, "MGS 2: Borderless: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_CreateWindowExAAddress);
        }
        else if (!MGS2_CreateWindowExAScanResult)
        {
            LOG_F(INFO, "MGS 2: Borderless: Pattern scan failed.");
        }
    }
    else if (sExeName == "METAL GEAR SOLID3.exe" && bBorderlessMode)
    {
        uint8_t* MGS3_CreateWindowExAScanResult = Memory::PatternScan(baseModule, "48 ?? ?? ?? ?? 00 00 4C ?? ?? ?? ?? 48 ?? ?? ?? ?? 44 ?? ?? ?? ??");
        if (MGS3_CreateWindowExAScanResult)
        {
            DWORD64 MGS3_CreateWindowExAAddress = (uintptr_t)MGS3_CreateWindowExAScanResult;
            int MGS3_CreateWindowExAHookLength = Memory::GetHookLength((char*)MGS3_CreateWindowExAAddress, 13);
            MGS3_CreateWindowExAReturnJMP = MGS3_CreateWindowExAAddress + MGS3_CreateWindowExAHookLength;
            Memory::DetourFunction64((void*)MGS3_CreateWindowExAAddress, MGS3_CreateWindowExA_CC, MGS3_CreateWindowExAHookLength);

            LOG_F(INFO, "MGS 3: Borderless: Hook length is %d bytes", MGS3_CreateWindowExAHookLength);
            LOG_F(INFO, "MGS 3: Borderless: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS3_CreateWindowExAAddress);
        }
        else if (!MGS3_CreateWindowExAScanResult)
        {
            LOG_F(INFO, "MGS 3: Borderless: Pattern scan failed.");
        }
    }
    
}

void ScaleEffects()
{
    if (sExeName == "METAL GEAR SOLID2.exe" && bCustomResolution)
    {
        // MGS 2: Scale effects correctly. (text, overlays, fades etc)
        uint8_t* MGS2_EffectsScaleXScanResult = Memory::PatternScan(baseModule, "48 8B ?? ?? 66 ?? ?? ?? 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? ?? ?? ??");
        if (MGS2_EffectsScaleXScanResult)
        {
            DWORD64 MGS2_EffectsScaleXAddress = (uintptr_t)MGS2_EffectsScaleXScanResult;
            int MGS2_EffectsScaleXHookLength = Memory::GetHookLength((char*)MGS2_EffectsScaleXAddress, 13);
            MGS2_EffectsScaleXReturnJMP = MGS2_EffectsScaleXAddress + MGS2_EffectsScaleXHookLength;
            Memory::DetourFunction64((void*)MGS2_EffectsScaleXAddress, MGS2_EffectsScaleX_CC, MGS2_EffectsScaleXHookLength);

            LOG_F(INFO, "MGS 2: Scale Effects: Hook length is %d bytes", MGS2_EffectsScaleXHookLength);
            LOG_F(INFO, "MGS 2: Scale Effects: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_EffectsScaleXAddress);

            DWORD64 MGS2_EffectsScaleYAddress = (uintptr_t)MGS2_EffectsScaleXScanResult + 0x54; // Long gap, maybe do two different sigs?
            int MGS2_EffectsScaleYHookLength = Memory::GetHookLength((char*)MGS2_EffectsScaleYAddress, 13);
            MGS2_EffectsScaleYReturnJMP = MGS2_EffectsScaleYAddress + MGS2_EffectsScaleYHookLength;
            Memory::DetourFunction64((void*)MGS2_EffectsScaleYAddress, MGS2_EffectsScaleY_CC, MGS2_EffectsScaleYHookLength);

            LOG_F(INFO, "MGS 2: Scale Effects: Hook length is %d bytes", MGS2_EffectsScaleYHookLength);
            LOG_F(INFO, "MGS 2: Scale Effects: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_EffectsScaleYAddress);
        }
        else if (!MGS2_EffectsScaleXScanResult)
        {
            LOG_F(INFO, "MGS 2: Scale Effects: Pattern scan failed.");
        }
    }
}

void AspectFOVFix()
{
    if (sExeName == "METAL GEAR SOLID3.exe" && bAspectFix)
    {
        // MGS 3: Fix gameplay aspect ratio
        // TODO: Signature is not unique (2 results)
        uint8_t* MGS3_GameplayAspectScanResult = Memory::PatternScan(baseModule, "0F ?? ?? 48 ?? ?? ?? F3 0F ?? ?? ?? 0F ?? ?? 7A ?? 75 ?? F3 0F ?? ?? ?? ?? ?? ?? C3");
        if (MGS3_GameplayAspectScanResult)
        {
            DWORD64 MGS3_GameplayAspectAddress = (uintptr_t)MGS3_GameplayAspectScanResult;
            int MGS3_GameplayAspectHookLength = Memory::GetHookLength((char*)MGS3_GameplayAspectAddress, 13);
            MGS3_GameplayAspectReturnJMP = MGS3_GameplayAspectAddress + MGS3_GameplayAspectHookLength;
            Memory::DetourFunction64((void*)MGS3_GameplayAspectAddress, MGS3_GameplayAspect_CC, MGS3_GameplayAspectHookLength);

            LOG_F(INFO, "MGS 3: Aspect Ratio: Hook length is %d bytes", MGS3_GameplayAspectHookLength);
            LOG_F(INFO, "MGS 3: Aspect Ratio: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS3_GameplayAspectAddress);
        }
        else if (!MGS3_GameplayAspectScanResult)
        {
            LOG_F(INFO, "MGS 3: Aspect Ratio: Pattern scan failed.");
        }
    }  
    else if (sExeName == "METAL GEAR SOLID2.exe" && bAspectFix)
    {
        // MGS 2: Fix gameplay aspect ratio
        // TODO: Signature is not unique (2 results)
        uint8_t* MGS2_GameplayAspectScanResult = Memory::PatternScan(baseModule, "0F ?? ?? 48 ?? ?? ?? F3 0F ?? ?? ?? 0F ?? ?? 75 ?? F3 0F ?? ?? ?? ?? ?? ?? C3");
        if (MGS2_GameplayAspectScanResult)
        {
            DWORD64 MGS2_GameplayAspectAddress = (uintptr_t)MGS2_GameplayAspectScanResult;
            int MGS2_GameplayAspectHookLength = Memory::GetHookLength((char*)MGS2_GameplayAspectAddress, 13);
            MGS2_GameplayAspectReturnJMP = MGS2_GameplayAspectAddress + MGS2_GameplayAspectHookLength;
            Memory::DetourFunction64((void*)MGS2_GameplayAspectAddress, MGS2_GameplayAspect_CC, MGS2_GameplayAspectHookLength);

            LOG_F(INFO, "MGS 2: Aspect Ratio: Hook length is %d bytes", MGS2_GameplayAspectHookLength);
            LOG_F(INFO, "MGS 2: Aspect Ratio: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_GameplayAspectAddress);
        }
        else if (!MGS2_GameplayAspectScanResult)
        {
            LOG_F(INFO, "MGS 2: Aspect Ratio: Pattern scan failed.");
        }
    }
}

void HUDFix()
{
    /*
    bHUDFix = true;
    if (sExeName == "METAL GEAR SOLID2.exe" && bHUDFix)
    {
   
        // TODO: FIX THIS
        // MGS 2: HUD width
        uint8_t* MGS2_HUDWidthScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? 48 ?? ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? ?? ?? ?? 16");
        if (MGS2_HUDWidthScanResult)
        {
            DWORD64 MGS2_HUDWidthAddress = (uintptr_t)MGS2_HUDWidthScanResult;
            int MGS2_HUDWidthHookLength = Memory::GetHookLength((char*)MGS2_HUDWidthAddress, 13);
            MGS2_HUDWidthReturnJMP = MGS2_HUDWidthAddress + MGS2_HUDWidthHookLength;
            Memory::DetourFunction64((void*)MGS2_HUDWidthAddress, MGS2_HUDWidth_CC, MGS2_HUDWidthHookLength);

            LOG_F(INFO, "MGS 2: HUD Width: Hook length is %d bytes", MGS2_HUDWidthHookLength);
            LOG_F(INFO, "MGS 2: HUD Width: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_HUDWidthAddress);
        }
        else if (!MGS2_HUDWidthScanResult)
        {
            LOG_F(INFO, "MGS 2: HUD Width: Pattern scan failed.");
        }
    }
    else if (sExeName == "METAL GEAR SOLID3.exe" && bHUDFix)
    {
        // TODO: FIX THIS
        // MGS 3: HUD width
        uint8_t* MGS3_HUDWidthScanResult = Memory::PatternScan(baseModule, "48 ?? ?? E9 ?? ?? ?? ?? F3 0F ?? ?? ?? 41 ?? ?? ?? F3 0F ?? ?? ?? 41 ?? ?? ??");
        if (MGS3_HUDWidthScanResult)
        {
            DWORD64 MGS3_HUDWidthAddress = (uintptr_t)MGS3_HUDWidthScanResult + 0x8;
            int MGS3_HUDWidthHookLength = Memory::GetHookLength((char*)MGS3_HUDWidthAddress, 13);
            MGS3_HUDWidthReturnJMP = MGS3_HUDWidthAddress + MGS3_HUDWidthHookLength;
            Memory::DetourFunction64((void*)MGS3_HUDWidthAddress, MGS3_HUDWidth_CC, MGS3_HUDWidthHookLength);

            LOG_F(INFO, "MGS 3: HUD Width: Hook length is %d bytes", MGS3_HUDWidthHookLength);
            LOG_F(INFO, "MGS 3: HUD Width: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS3_HUDWidthAddress);
        }
        else if (!MGS3_HUDWidthScanResult)
        {
            LOG_F(INFO, "MGS 3: HUD Width: Pattern scan failed.");
        }
    }
    */
}

void MovieFix()
{
    /*
    if (sExeName == "METAL GEAR SOLID2.exe" && bMovieFix)
    {
        // MGS 2: Movie fix
        uint8_t* MGS2_MovieScanResult = Memory::PatternScan(baseModule, "C7 ?? 00 00 00 00 48 ?? ?? ?? ?? C7 ?? 00 00 A0 44 48 ?? ?? ?? ?? C7 ?? 00 00 00 00");
        if (MGS2_MovieScanResult)
        {
            float bob = fNewX / fNativeAspect;
            MGS2_fMovieOffset = (float)bob - (1280 * fAspectMultiplier);
            MGS2_fMovieWidth = (float)1280 - MGS2_fMovieOffset;

            DWORD64 MGS2_MovieAddress = (uintptr_t)MGS2_MovieScanResult + 0x2;
            Memory::Write(MGS2_MovieAddress, MGS2_fMovieOffset);
            Memory::Write(MGS2_MovieAddress + 0xB, MGS2_fMovieWidth);

            LOG_F(INFO, "MGS 2: Movie: Address is 0x%" PRIxPTR, (uintptr_t)MGS2_MovieAddress);
        }
        else if (!MGS2_MovieScanResult)
        {
            LOG_F(INFO, "MGS 2: Movie: Pattern scan failed.");
        }
    }
    else if (sExeName == "METAL GEAR SOLID3.exe" && bMovieFix)
    {
        // MGS 3: Movie fix
        uint8_t* MGS3_MovieScanResult = Memory::PatternScan(baseModule, "48 ?? ?? E8 ?? ?? ?? ?? 41 8B ?? ?? 48 8D ??");
        if (MGS3_MovieScanResult)
        {
            MGS3_fMovieOffset = -((fHUDOffset) / 2);
            MGS3_fMovieWidth = (float)720 * fNewAspect;

            DWORD64 MGS3_MovieAddress = (Memory::GetAbsolute((uintptr_t)MGS3_MovieScanResult + 0x4) + 0x44); // This is bad but it gives us a more unique sig otherwise it's >100 bytes.
            int MGS3_MovieHookLength = Memory::GetHookLength((char*)MGS3_MovieAddress, 13);
            MGS3_MovieReturnJMP = MGS3_MovieAddress + MGS3_MovieHookLength;
            Memory::DetourFunction64((void*)MGS3_MovieAddress, MGS3_Movie_CC, MGS3_MovieHookLength);

            LOG_F(INFO, "MGS 3: Movie: Hook length is %d bytes", MGS3_MovieHookLength);
            LOG_F(INFO, "MGS 3: Movie: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS3_MovieAddress);
        }
        else if (!MGS3_MovieScanResult)
        {
            LOG_F(INFO, "MGS 3: Movie: Pattern scan failed.");
        }
    }
    */
}

DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    DetectGame();
    CustomResolution();
    Sleep(iInjectionDelay);
    ScaleEffects();
    AspectFOVFix();
    HUDFix();
    MovieFix();
    return true; // end thread
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);

        if (mainHandle)
        {
            CloseHandle(mainHandle);
        }
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

