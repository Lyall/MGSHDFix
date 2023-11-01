#include "stdafx.h"
#include "helper.hpp"

using namespace std;

HMODULE baseModule = GetModuleHandle(NULL);

inipp::Ini<char> ini;

// INI Variables
bool bAspectFix;
bool bHUDFix;
bool bCustomResolution;
bool bSkipIntroLogos;
bool bWindowedMode;
bool bBorderlessMode;
bool bFramebufferFix;
bool bDisableCursor;
bool bMouseSensitivity;
float fMouseSensitivityXMulti;
float fMouseSensitivityYMulti;
int iCustomResX;
int iCustomResY;
int iInjectionDelay;

// Variables
float fNewX;
float fNewY;
float fNativeAspect = (float)16/9;
float fPi = 3.14159265358979323846f;
float fNewAspect;
bool bNarrowAspect = false;
float fAspectDivisional;
float fAspectMultiplier;
float fHUDWidth;
float fHUDHeight;
float fHUDWidthOffset;
float fHUDHeightOffset;
int iHUDWidth;
int iHUDHeight;
int iHUDWidthOffset;
int iHUDHeightOffset;
float fMGS2_DefaultHUDX = (float)1280;
float fMGS2_DefaultHUDY = (float)720;
float fMGS3_DefaultHUDWidth = (float)2;
float fMGS3_DefaultHUDHeight = (float)-2;
float fMGS2_DefaultHUDWidth = (float)1;
float fMGS2_DefaultHUDHeight = (float)-2;
float fMGS2_DefaultHUDHeight2 = (float)-1;
string sExeName;
string sGameName;
string sExePath;
string sGameVersion;
string sFixVer = "0.8";

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

// MGS 3: HUD Width Hook
DWORD64 MGS3_HUDWidthReturnJMP;
float fMGS3_NewHUDWidth;
float fMGS3_NewHUDHeight;
void __declspec(naked) MGS3_HUDWidth_CC()
{
    __asm
    {
        movss xmm14, [fMGS3_NewHUDWidth]
        movss xmm15, [fMGS3_NewHUDHeight]
        mov r12d, 0x00000200
        jmp[MGS3_HUDWidthReturnJMP]
    }
}

// MGS 2: HUD Width Hook
DWORD64 MGS2_HUDWidthReturnJMP;
float fMGS2_NewHUDWidth;
float fMGS2_NewHUDHeight;
float fMGS2_NewHUDHeight2;
void __declspec(naked) MGS2_HUDWidth_CC()
{
    __asm
    {
        movss xmm0, [rdi + 0x10]
        movaps xmm2, xmm7
        movss xmm2, [fMGS2_NewHUDWidth]
        subss xmm0, [rdi + 0x08]
        divss xmm2, xmm0
        movss xmm9, [fMGS2_NewHUDHeight]
        movss xmm10, [fMGS2_NewHUDHeight2]
        jmp[MGS2_HUDWidthReturnJMP]
    }
}

// MGS 2: Radar Width Hook
DWORD64 MGS2_RadarWidthReturnJMP;
void __declspec(naked) MGS2_RadarWidth_CC()
{
    __asm
    {
        mov ebx, [iHUDWidth]
        mov r8d, eax
        mov eax, ebx
        imul eax, [rsi + 0x0C]
        mov ecx, r8d
        imul ecx, [rsi + 0x10]
        jmp[MGS2_RadarWidthReturnJMP]
    }
}

// MGS 2: Radar Offset Hook
DWORD64 MGS2_RadarOffsetReturnJMP;
void __declspec(naked) MGS2_RadarOffset_CC()
{
    __asm
    {
        sar r9d, 8
        add eax, edx
        mov ecx, r9d
        sar eax, 9
        shr ecx, 31
        add r9d, ecx
        mov ecx, [iHUDWidthOffset]
        add eax, ecx
        jmp[MGS2_RadarOffsetReturnJMP]
    }
}

// MGS 2: Effects Scale X Hook
DWORD64 MGS2_EffectsScaleXReturnJMP;
float fMGS2_EffectScaleX;
void __declspec(naked) MGS2_EffectsScaleX_CC()
{
    __asm
    {
        movss xmm1, [fMGS2_EffectScaleX]
        mov rcx, [rbp - 0x60]
        movd xmm0, eax
        cvtdq2ps xmm0, xmm0
        divss xmm1, xmm0
        jmp[MGS2_EffectsScaleXReturnJMP]
    }
}

// MGS 2: Effects Scale X 2 Hook
DWORD64 MGS2_EffectsScaleX2ReturnJMP;
void __declspec(naked) MGS2_EffectsScaleX2_CC()
{
    __asm
    {
        movss xmm1, [fMGS2_EffectScaleX]
        cvtdq2ps xmm0, xmm0
        divss xmm1, xmm0
        movd xmm0, [rbp - 0x04]
        addss xmm1, xmm1
        jmp[MGS2_EffectsScaleX2ReturnJMP]
    }
}

// MGS 2: Effects Scale Y Hook
DWORD64 MGS2_EffectsScaleYReturnJMP;
float fMGS2_EffectScaleY;
void __declspec(naked) MGS2_EffectsScaleY_CC()
{
    __asm
    {
        movss xmm1, [fMGS2_EffectScaleY]
        mov [rsp + 0x000004F8], rbx
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

// MGS 3: Mouse Sensitivity X Hook
DWORD64 MGS3_MouseSensitivityXReturnJMP;
void __declspec(naked) MGS3_MouseSensitivityX_CC()
{
    __asm
    {
        mulss xmm0, [fMouseSensitivityXMulti]
        cvttss2si eax, xmm0
        movd xmm0, ecx
        mov ecx, [rbx + 0x24]
        jmp[MGS3_MouseSensitivityXReturnJMP]
    }
}

// MGS 3: Mouse Sensitivity Y Hook
DWORD64 MGS3_MouseSensitivityYReturnJMP;
void __declspec(naked) MGS3_MouseSensitivityY_CC()
{
    __asm
    {
        mulss xmm0, [fMouseSensitivityYMulti]
        mov [rbx + 0x50], edx
        cvttss2si eax, xmm0
        movd xmm0, ecx
        jmp[MGS3_MouseSensitivityYReturnJMP]
    }
}

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
    inipp::get_value(ini.sections["Framebuffer Fix"], "Enabled", bFramebufferFix);
    inipp::get_value(ini.sections["Skip Intro Logos"], "Enabled", bSkipIntroLogos);
    inipp::get_value(ini.sections["Disable Mouse Cursor"], "Enabled", bDisableCursor);
    inipp::get_value(ini.sections["Mouse Sensitivity"], "Enabled", bMouseSensitivity);
    inipp::get_value(ini.sections["Mouse Sensitivity"], "X Multiplier", fMouseSensitivityXMulti);
    inipp::get_value(ini.sections["Mouse Sensitivity"], "Y Multiplier", fMouseSensitivityYMulti);
    inipp::get_value(ini.sections["Fix Aspect Ratio"], "Enabled", bAspectFix);
    inipp::get_value(ini.sections["Fix HUD"], "Enabled", bHUDFix);

    // Log config parse
    LOG_F(INFO, "Config Parse: iInjectionDelay: %dms", iInjectionDelay);
    LOG_F(INFO, "Config Parse: bCustomResolution: %d", bCustomResolution);
    LOG_F(INFO, "Config Parse: iCustomResX: %d", iCustomResX);
    LOG_F(INFO, "Config Parse: iCustomResY: %d", iCustomResY);
    LOG_F(INFO, "Config Parse: bWindowedMode: %d", bWindowedMode);
    LOG_F(INFO, "Config Parse: bBorderlessMode: %d", bBorderlessMode);
    LOG_F(INFO, "Config Parse: bFramebufferFix: %d", bFramebufferFix);
    LOG_F(INFO, "Config Parse: bSkipIntroLogos: %d", bSkipIntroLogos);
    LOG_F(INFO, "Config Parse: bDisableCursor: %d", bDisableCursor);
    LOG_F(INFO, "Config Parse: bMouseSensitivity: %d", bMouseSensitivity);
    LOG_F(INFO, "Config Parse: fMouseSensitivityXMulti: %.2f", fMouseSensitivityXMulti);
    LOG_F(INFO, "Config Parse: fMouseSensitivityYMulti: %.2f", fMouseSensitivityYMulti);
    LOG_F(INFO, "Config Parse: bAspectFix: %d", bAspectFix);
    LOG_F(INFO, "Config Parse: bHUDFix: %d", bHUDFix);

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

    // Check if <16:9
    if (fNewAspect < fNativeAspect)
    {
        bNarrowAspect = true;
    }

    // Disable ultrawide fixes at 16:9
    if (fNewAspect == fNativeAspect)
    {
        bAspectFix = false;
        bHUDFix = false;
        LOG_F(INFO, "Config Parse: Aspect ratio is native, disabling ultrawide fixes.");
    }

    // HUD variables
    fAspectMultiplier = (float)fNewAspect / fNativeAspect;
    fHUDWidth = (float)fNewY * fNativeAspect;
    fHUDHeight = (float)fNewY;
    fHUDWidthOffset = (float)(fNewX - fHUDWidth) / 2;
    fHUDHeightOffset = 0;
    if (bNarrowAspect) 
    { 
        fHUDWidth = fNewX; 
        fHUDHeight = (float)fNewX / fNativeAspect;

        fHUDWidthOffset = 0;
        fHUDHeightOffset = (float)(fNewY - fHUDHeight) / 2;

    }
    iHUDWidth = (int)fHUDWidth;
    iHUDHeight = (int)fHUDHeight;
    iHUDWidthOffset = (int)fHUDWidthOffset;
    iHUDHeightOffset = (int)fHUDHeightOffset;

    LOG_F(INFO, "Custom Resolution: fNewAspect: %.4f", fNewAspect);
    LOG_F(INFO, "Custom Resolution: fAspectMultiplier: %.4f", fAspectMultiplier);
    LOG_F(INFO, "Custom Resolution: fHUDWidth: %.4f", fHUDWidth);
    LOG_F(INFO, "Custom Resolution: fHUDHeight: %.4f", fHUDHeight);
    LOG_F(INFO, "Custom Resolution: fHUDWidthOffset: %.4f", fHUDWidthOffset);
    LOG_F(INFO, "Custom Resolution: fHUDHeightOffset: %.4f", fHUDHeightOffset);
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
    else if (sExeName == "METAL GEAR.exe")
    {
        LOG_F(INFO, "Detected game is: Metal Gear / Metal Gear 2 (MSX)");
    }
}

void CustomResolution()
{
    if ((sExeName == "METAL GEAR SOLID2.exe" || sExeName == "METAL GEAR SOLID3.exe" || sExeName == "METAL GEAR.exe") && bCustomResolution)
    {
        // MGS 2 | MGS 3: Custom Resolution
        uint8_t* MGS2_MGS3_ResolutionScanResult = Memory::PatternScan(baseModule, "C7 45 ?? 00 05 00 00 C7 ?? ?? D0 02 00 00 C7 ?? ?? 00 05 00 00 C7 ?? ?? D0 02 00 00");
        if (MGS2_MGS3_ResolutionScanResult)
        {
            DWORD64 MGS2_MGS3_ResolutionAddress = (uintptr_t)MGS2_MGS3_ResolutionScanResult + 0x3;
            LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Custom Resolution: Address is 0x%" PRIxPTR, (uintptr_t)MGS2_MGS3_ResolutionAddress);

            Memory::Write(MGS2_MGS3_ResolutionAddress, iCustomResX);
            Memory::Write((MGS2_MGS3_ResolutionAddress + 0x7), iCustomResY);
            Memory::Write((MGS2_MGS3_ResolutionAddress + 0xE), iCustomResX);
            Memory::Write((MGS2_MGS3_ResolutionAddress + 0x15), iCustomResY);
            LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Custom Resolution: New Custom Resolution = %dx%d", iCustomResX, iCustomResY);
        }
        else if (!MGS2_MGS3_ResolutionScanResult)
        {
            LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Custom Resolution: Pattern scan failed.");
        }

        // MGS 2 | MGS 3: Framebuffer fix, stops the framebuffer from being set to maximum display resolution.
        // Thanks emoose!
        if (bFramebufferFix)
        {
            for (int i = 1; i <= 2; ++i) // Two results to change, unsure if first result is actually used but we NOP it anyway.
            {
                uint8_t* MGS2_MGS3_FramebufferFixScanResult = Memory::PatternScan(baseModule, "8B ?? ?? 48 ?? ?? ?? 03 C2 89 ?? ??");
                if (MGS2_MGS3_FramebufferFixScanResult)
                {
                    DWORD64 MGS2_MGS3_FramebufferFixAddress = (uintptr_t)MGS2_MGS3_FramebufferFixScanResult + 0x9;
                    LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Framebuffer %d: Address is 0x%" PRIxPTR, i, (uintptr_t)MGS2_MGS3_FramebufferFixAddress);

                    Memory::PatchBytes(MGS2_MGS3_FramebufferFixAddress, "\x90\x90\x90", 3);
                    LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Framebuffer %d: Patched instruction.", i);
                }
                else if (!MGS2_MGS3_FramebufferFixScanResult)
                {
                    LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Framebuffer %d: Pattern scan failed.", i);
                }
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
                LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Window Mode: Address is 0x%" PRIxPTR, (uintptr_t)MGS2_MGS3_WindowModeAddress);

                Memory::PatchBytes(MGS2_MGS3_WindowModeAddress, "\x00", 1);
                LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Window Mode: Patched instruction.");
            }
            else if (!MGS2_MGS3_WindowModeScanResult)
            {
                LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Window Mode: Pattern scan failed.");
            }
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
    else if ((sExeName == "METAL GEAR SOLID3.exe" || sExeName == "METAL GEAR.exe") && bBorderlessMode)
    {
        uint8_t* MGS3_CreateWindowExAScanResult = Memory::PatternScan(baseModule, "48 ?? ?? ?? ?? 00 00 4C ?? ?? ?? ?? 48 ?? ?? ?? ?? 44 ?? ?? ?? ??");
        if (MGS3_CreateWindowExAScanResult)
        {
            DWORD64 MGS3_CreateWindowExAAddress = (uintptr_t)MGS3_CreateWindowExAScanResult;
            int MGS3_CreateWindowExAHookLength = Memory::GetHookLength((char*)MGS3_CreateWindowExAAddress, 13);
            MGS3_CreateWindowExAReturnJMP = MGS3_CreateWindowExAAddress + MGS3_CreateWindowExAHookLength;
            Memory::DetourFunction64((void*)MGS3_CreateWindowExAAddress, MGS3_CreateWindowExA_CC, MGS3_CreateWindowExAHookLength);

            LOG_F(INFO, "MG/MG2 | MGS 3: Borderless: Hook length is %d bytes", MGS3_CreateWindowExAHookLength);
            LOG_F(INFO, "MG/MG2 | MGS 3: Borderless: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS3_CreateWindowExAAddress);
        }
        else if (!MGS3_CreateWindowExAScanResult)
        {
            LOG_F(INFO, "MG/MG2 | MGS 3: Borderless: Pattern scan failed.");
        }
    }   
}

void IntroSkip()
{
    if (!bSkipIntroLogos)
        return;
    if (sExeName != "METAL GEAR SOLID2.exe" && sExeName != "METAL GEAR SOLID3.exe")
        return;

    uint8_t* MGS2_MGS3_InitialIntroStateScanResult = Memory::PatternScan(baseModule, "75 ? C7 05 ? ? ? ? 01 00 00 00 C3");
    if (!MGS2_MGS3_InitialIntroStateScanResult)
    {
        LOG_F(INFO, "MGS 2 | MGS 3: Skip Intro Logos: Pattern scan failed.");
        return;
    }

    uint32_t* MGS2_MGS3_InitialIntroStatePtr = (uint32_t*)(MGS2_MGS3_InitialIntroStateScanResult + 8);
    LOG_F(INFO, "MGS 2 | MGS 3: Skip Intro Logos: Initial state: %x", *MGS2_MGS3_InitialIntroStatePtr);

    uint32_t NewState = 3;
    Memory::PatchBytes((uintptr_t)MGS2_MGS3_InitialIntroStatePtr, (const char*)&NewState, sizeof(NewState));
    LOG_F(INFO, "MGS 2 | MGS 3: Skip Intro Logos: Patched state: %x", *MGS2_MGS3_InitialIntroStatePtr);
}

void ScaleEffects()
{
    if (sExeName == "METAL GEAR SOLID2.exe" && bCustomResolution)
    {
        // MGS 2: Scale effects correctly. (text, overlays, fades etc)
        uint8_t* MGS2_EffectsScaleScanResult = Memory::PatternScan(baseModule, "48 8B ?? ?? 66 ?? ?? ?? 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? ?? ?? ??");
        if (MGS2_EffectsScaleScanResult)
        {
            // X scale
            DWORD64 MGS2_EffectsScaleXAddress = (uintptr_t)MGS2_EffectsScaleScanResult;
            int MGS2_EffectsScaleXHookLength = Memory::GetHookLength((char*)MGS2_EffectsScaleXAddress, 13);
            MGS2_EffectsScaleXReturnJMP = MGS2_EffectsScaleXAddress + MGS2_EffectsScaleXHookLength;
            Memory::DetourFunction64((void*)MGS2_EffectsScaleXAddress, MGS2_EffectsScaleX_CC, MGS2_EffectsScaleXHookLength);

            float fMGS2_DefaultEffectScaleX = *reinterpret_cast<float*>(Memory::GetAbsolute(MGS2_EffectsScaleXAddress - 0x4));
            fMGS2_EffectScaleX = (float)fMGS2_DefaultEffectScaleX / (fMGS2_DefaultHUDX / fNewX);       
            if (bHUDFix)
            {
                fMGS2_EffectScaleX = (float)fMGS2_DefaultEffectScaleX / (fMGS2_DefaultHUDX / fHUDWidth);
            }

            LOG_F(INFO, "MGS 2: Scale Effects X: Hook length is %d bytes", MGS2_EffectsScaleXHookLength);
            LOG_F(INFO, "MGS 2: Scale Effects X: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_EffectsScaleXAddress);

            DWORD64 MGS2_EffectsScaleX2Address = (uintptr_t)MGS2_EffectsScaleScanResult - 0x2B;
            int MGS2_EffectsScaleX2HookLength = Memory::GetHookLength((char*)MGS2_EffectsScaleX2Address, 13);
            MGS2_EffectsScaleX2ReturnJMP = MGS2_EffectsScaleX2Address + MGS2_EffectsScaleX2HookLength;
            Memory::DetourFunction64((void*)MGS2_EffectsScaleX2Address, MGS2_EffectsScaleX2_CC, MGS2_EffectsScaleX2HookLength);

            LOG_F(INFO, "MGS 2: Scale Effects X 2: Hook length is %d bytes", MGS2_EffectsScaleX2HookLength);
            LOG_F(INFO, "MGS 2: Scale Effects X 2: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_EffectsScaleX2Address);

            // Y scale
            DWORD64 MGS2_EffectsScaleYAddress = (uintptr_t)MGS2_EffectsScaleScanResult + 0x24;
            int MGS2_EffectsScaleYHookLength = Memory::GetHookLength((char*)MGS2_EffectsScaleYAddress, 13);
            MGS2_EffectsScaleYReturnJMP = MGS2_EffectsScaleYAddress + MGS2_EffectsScaleYHookLength;

            float fMGS2_DefaultEffectScaleY = *reinterpret_cast<float*>(Memory::GetAbsolute(MGS2_EffectsScaleYAddress + 0x4));
            fMGS2_EffectScaleY = (float)fMGS2_DefaultEffectScaleY / (fMGS2_DefaultHUDY / fNewY);
            if (bHUDFix && bNarrowAspect)
            {
                fMGS2_EffectScaleY = (float)fMGS2_DefaultEffectScaleY / (fMGS2_DefaultHUDX / fHUDHeight);
            }

            Memory::DetourFunction64((void*)MGS2_EffectsScaleYAddress, MGS2_EffectsScaleY_CC, MGS2_EffectsScaleYHookLength);

            LOG_F(INFO, "MGS 2: Scale Effects Y: Hook length is %d bytes", MGS2_EffectsScaleYHookLength);
            LOG_F(INFO, "MGS 2: Scale Effects Y: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_EffectsScaleYAddress);
        }
        else if (!MGS2_EffectsScaleScanResult)
        {
            LOG_F(INFO, "MGS 2: Scale Effects: Pattern scan failed.");
        }
    }
}

void AspectFOVFix()
{
    if ((sExeName == "METAL GEAR SOLID3.exe" || sExeName == "METAL GEAR.exe") && bAspectFix)
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

            LOG_F(INFO, "MG/MG2 | MGS 3: Aspect Ratio: Hook length is %d bytes", MGS3_GameplayAspectHookLength);
            LOG_F(INFO, "MG/MG2 | MGS 3: Aspect Ratio: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS3_GameplayAspectAddress);
        }
        else if (!MGS3_GameplayAspectScanResult)
        {
            LOG_F(INFO, "MG/MG2 | MGS 3: Aspect Ratio: Pattern scan failed.");
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
    if (sExeName == "METAL GEAR SOLID2.exe" && bHUDFix)
    {
        // MGS 2: HUD
        uint8_t* MGS2_HUDWidthScanResult = Memory::PatternScan(baseModule, "E9 ?? ?? ?? ?? F3 0F ?? ?? ?? 0F ?? ?? F3 0F ?? ?? ?? F3 0F ?? ??");
        if (MGS2_HUDWidthScanResult)
        {
            fMGS2_NewHUDWidth = fMGS2_DefaultHUDWidth / fAspectMultiplier;
            fMGS2_NewHUDHeight = fMGS2_DefaultHUDHeight;
            fMGS2_NewHUDHeight2 = fMGS2_DefaultHUDHeight2;

            if (bNarrowAspect)
            {
                fMGS2_NewHUDWidth = fMGS2_DefaultHUDWidth;
                fMGS2_NewHUDHeight = fMGS2_DefaultHUDHeight * fAspectMultiplier;
                fMGS2_NewHUDHeight2 = fMGS2_DefaultHUDHeight2 * fAspectMultiplier;
            }

            DWORD64 MGS2_HUDWidthAddress = (uintptr_t)MGS2_HUDWidthScanResult + 0x5;
            int MGS2_HUDWidthHookLength = 17;
            MGS2_HUDWidthReturnJMP = MGS2_HUDWidthAddress + MGS2_HUDWidthHookLength;
            Memory::DetourFunction64((void*)MGS2_HUDWidthAddress, MGS2_HUDWidth_CC, MGS2_HUDWidthHookLength);

            LOG_F(INFO, "MGS 2: HUD Width: Hook length is %d bytes", MGS2_HUDWidthHookLength);
            LOG_F(INFO, "MGS 2: HUD Width: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_HUDWidthAddress);
        }
        else if (!MGS2_HUDWidthScanResult)
        {
            LOG_F(INFO, "MGS 2: HUD Fix: Pattern scan failed.");
        }

        // MGS 2: Radar
        uint8_t* MGS2_RadarWidthScanResult = Memory::PatternScan(baseModule, "44 ?? ?? 8B ?? 0F ?? ?? ?? 41 ?? ?? 0F ?? ?? ?? 44 ?? ?? ?? ?? ?? ?? 0F ?? ?? ?? 99");
        if (MGS2_RadarWidthScanResult)
        {
            // Dont scale width if <16:9
            if (!bNarrowAspect)
            {
                DWORD64 MGS2_RadarWidthAddress = (uintptr_t)MGS2_RadarWidthScanResult;
                int MGS2_RadarWidthHookLength = Memory::GetHookLength((char*)MGS2_RadarWidthAddress, 13);
                MGS2_RadarWidthReturnJMP = MGS2_RadarWidthAddress + MGS2_RadarWidthHookLength;
                Memory::DetourFunction64((void*)MGS2_RadarWidthAddress, MGS2_RadarWidth_CC, MGS2_RadarWidthHookLength);

                LOG_F(INFO, "MGS 2: Radar Width: Hook length is %d bytes", MGS2_RadarWidthHookLength);
                LOG_F(INFO, "MGS 2: Radar Width: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_RadarWidthAddress);
            }

            DWORD64 MGS2_RadarOffsetAddress = (uintptr_t)MGS2_RadarWidthScanResult + 0x42;
            int MGS2_RadarOffsetHookLength = 18; // length disassembler causes crashes for some reason?
            MGS2_RadarOffsetReturnJMP = MGS2_RadarOffsetAddress + MGS2_RadarOffsetHookLength;
            //Memory::DetourFunction64((void*)MGS2_RadarOffsetAddress, MGS2_RadarOffset_CC, MGS2_RadarOffsetHookLength);

            LOG_F(INFO, "MGS 2: Radar Offset: Hook length is %d bytes", MGS2_RadarOffsetHookLength);
            LOG_F(INFO, "MGS 2: Radar Offset: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_RadarOffsetAddress);
        }
        else if (!MGS2_RadarWidthScanResult)
        {
            LOG_F(INFO, "MGS 2: Radar Fix: Pattern scan failed.");
        }

        // MGS 2: Disable motion blur. 
        uint8_t* MGS2_MGS3_MotionBlurScanResult = Memory::PatternScan(baseModule, "F3 48 ?? ?? ?? ?? 48 ?? ?? ?? 48 ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ??");
        if (MGS2_MGS3_MotionBlurScanResult && bWindowedMode)
        {
            DWORD64 MGS2_MGS3_MotionBlurAddress = (uintptr_t)MGS2_MGS3_MotionBlurScanResult;
            LOG_F(INFO, "MGS 2: Motion Blur: Address is 0x%" PRIxPTR, (uintptr_t)MGS2_MGS3_MotionBlurAddress);

            Memory::PatchBytes(MGS2_MGS3_MotionBlurAddress, "\x48\x31\xDB\x90\x90\x90", 6);
            LOG_F(INFO, "MGS 2: Motion Blur: Patched instruction.");
        }
        else if (!MGS2_MGS3_MotionBlurScanResult)
        {
            LOG_F(INFO, "MGS 2: Motion Blur: Pattern scan failed.");
        }
    }
    else if (sExeName == "METAL GEAR SOLID3.exe" && bHUDFix)
    {
        // MGS 3: HUD
        uint8_t* MGS3_HUDWidthScanResult = Memory::PatternScan(baseModule, "0F ?? ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? ?? 4C ?? ?? ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? ?? 41 ?? 00 02 00 00");
        if (MGS3_HUDWidthScanResult)
        {
            fMGS3_NewHUDWidth = fMGS3_DefaultHUDWidth / fAspectMultiplier;
            fMGS3_NewHUDHeight = fMGS3_DefaultHUDHeight;

            if (bNarrowAspect)
            {
                fMGS3_NewHUDWidth = fMGS3_DefaultHUDWidth;
                fMGS3_NewHUDHeight = fMGS3_DefaultHUDHeight * fAspectMultiplier;
            }

            DWORD64 MGS3_HUDWidthAddress = (uintptr_t)MGS3_HUDWidthScanResult + 0x16;
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

    if ((sExeName == "METAL GEAR SOLID2.exe" || sExeName == "METAL GEAR SOLID3.exe") && bHUDFix)
    {
        // MGS 2 | MGS 3: Letterboxing
        uint8_t* MGS2_MGS3_LetterboxingScanResult = Memory::PatternScan(baseModule, "83 ?? 01 75 ?? ?? 01 00 00 00 44 ?? ?? ?? ?? ?? ?? 89 ?? ?? ?? ?? ??");
        if (MGS2_MGS3_LetterboxingScanResult)
        {
            DWORD64 MGS2_MGS3_LetterboxingAddress = (uintptr_t)MGS2_MGS3_LetterboxingScanResult + 0x6;
            LOG_F(INFO, "MGS 2 | MGS 3: Letterboxing: Address is 0x%" PRIxPTR, (uintptr_t)MGS2_MGS3_LetterboxingAddress);

            Memory::Write(MGS2_MGS3_LetterboxingAddress, (int)0);
            LOG_F(INFO, "MGS 2 | MGS 3: Letterboxing: Disabled letterboxing.");
        }
        else if (!MGS2_MGS3_LetterboxingScanResult)
        {
            LOG_F(INFO, "MGS 2 | MGS 3: Letterboxing: Pattern scan failed.");
        }
    }
}

void Miscellaneous()
{

    if ((sExeName == "METAL GEAR SOLID2.exe" || sExeName == "METAL GEAR SOLID3.exe" || sExeName == "METAL GEAR.exe") && bDisableCursor)
    {
        // MGS 2 | MGS 3: Disable mouse cursor
        // Thanks again emoose!
        uint8_t* MGS2_MGS3_MouseCursorScanResult = Memory::PatternScan(baseModule, "?? ?? BA ?? ?? 00 00 FF ?? ?? ?? ?? ?? 48 ?? ??");
        if (MGS2_MGS3_MouseCursorScanResult && bWindowedMode)
        {
            DWORD64 MGS2_MGS3_MouseCursorAddress = (uintptr_t)MGS2_MGS3_MouseCursorScanResult;
            LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Mouse Cursor: Address is 0x%" PRIxPTR, (uintptr_t)MGS2_MGS3_MouseCursorAddress);

            Memory::PatchBytes(MGS2_MGS3_MouseCursorAddress, "\xEB", 1);
            LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Mouse Cursor: Patched instruction.");
        }
        else if (!MGS2_MGS3_MouseCursorScanResult)
        {
            LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Mouse Cursor: Pattern scan failed.");
        }
    }

    if (sExeName == "METAL GEAR SOLID3.exe" && bMouseSensitivity)
    {
        // MGS 3: Mouse sensitivity
        uint8_t* MGS3_MouseSensitivityScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? F3 0F ?? ?? 66 0F ?? ?? 8B ?? ??");
        if (MGS3_MouseSensitivityScanResult)
        {
            DWORD64 MGS3_MouseSensitivityXAddress = (uintptr_t)MGS3_MouseSensitivityScanResult;
            int MGS3_MouseSensitivityXHookLength = Memory::GetHookLength((char*)MGS3_MouseSensitivityXAddress, 13);
            MGS3_MouseSensitivityXReturnJMP = MGS3_MouseSensitivityXAddress + MGS3_MouseSensitivityXHookLength;
            Memory::DetourFunction64((void*)MGS3_MouseSensitivityXAddress, MGS3_MouseSensitivityX_CC, MGS3_MouseSensitivityXHookLength);

            LOG_F(INFO, "MGS 3: Mouse Sensitivity X: Hook length is %d bytes", MGS3_MouseSensitivityXHookLength);
            LOG_F(INFO, "MGS 3: Mouse Sensitivity X: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS3_MouseSensitivityXAddress);

            DWORD64 MGS3_MouseSensitivityYAddress = (uintptr_t)MGS3_MouseSensitivityScanResult + 0x33;
            int MGS3_MouseSensitivityYHookLength = Memory::GetHookLength((char*)MGS3_MouseSensitivityYAddress, 13);
            MGS3_MouseSensitivityYReturnJMP = MGS3_MouseSensitivityYAddress + MGS3_MouseSensitivityYHookLength;
            Memory::DetourFunction64((void*)MGS3_MouseSensitivityYAddress, MGS3_MouseSensitivityY_CC, MGS3_MouseSensitivityYHookLength);

            LOG_F(INFO, "MGS 3: Mouse Sensitivity Y: Hook length is %d bytes", MGS3_MouseSensitivityYHookLength);
            LOG_F(INFO, "MGS 3: Mouse Sensitivity Y: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS3_MouseSensitivityYAddress);
        }
        else if (!MGS3_MouseSensitivityScanResult)
        {
            LOG_F(INFO, "MGS 3: Mouse Sensitivity: Pattern scan failed.");
        }
    }  
}

DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    DetectGame();
    CustomResolution();
    IntroSkip();
    Sleep(iInjectionDelay);
    ScaleEffects();
    AspectFOVFix();
    HUDFix();
    Miscellaneous();
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

