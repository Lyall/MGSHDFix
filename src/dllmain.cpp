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
int iAnisotropicFiltering;
bool bFramebufferFix;
int iTextureBufferSizeMB;
bool bDisableBackgroundInput;
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
        cmp bNarrowAspect, 1
        // ->
        jne $+0x8
        mov eax, [iHUDHeight]
        // <-
        mov r8d, eax
        mov eax, ebx
        imul eax, [rsi + 0x0C]
        mov ecx, r8d
        imul ecx, [rsi + 0x10]
        jmp[MGS2_RadarWidthReturnJMP]
    }
}

// MGS 2: Radar Width Offset Hook
DWORD64 MGS2_RadarWidthOffsetReturnJMP;
void __declspec(naked) MGS2_RadarWidthOffset_CC()
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
        jmp[MGS2_RadarWidthOffsetReturnJMP]
    }
}

// MGS 2: Radar Width Offset Hook
DWORD64 MGS2_RadarHeightOffsetReturnJMP;
DWORD64 MGS2_RadarHeightOffsetValueAddress = 0;
void __declspec(naked) MGS2_RadarHeightOffset_CC()
{
    __asm
    {
        sar edx, 0x08
        mov eax, edx
        shr eax, 0x1F
        add edx, eax
        mov eax, [iHUDHeightOffset]
        add edx, eax
        mov rax, [MGS2_RadarHeightOffsetValueAddress]
        mov dword ptr[rax], edx
        xor eax, eax
        jmp[MGS2_RadarHeightOffsetReturnJMP]
    }
}

// MGS 2: Codec Portraits Hook
DWORD64 MGS2_CodecPortraitsReturnJMP;
void __declspec(naked) MGS2_CodecPortraits_CC()
{
    __asm
    {
        cmp bNarrowAspect, 1
        jne resizeHor
        divss xmm5, [fAspectMultiplier]
        addss xmm0, [rax + 0x60]
        cvttss2si eax, xmm4
        movss xmm4, [rbx + 0x64]
        jmp[MGS2_CodecPortraitsReturnJMP]

        resizeHor:
            divss xmm0, [fAspectMultiplier]
            movss xmm15, xmm4
            divss xmm15, [fAspectMultiplier]
            addss xmm0, xmm15
            xorps xmm15, xmm15
            cvttss2si eax, xmm4
            movss xmm4, [rbx + 0x64]
            jmp[MGS2_CodecPortraitsReturnJMP]
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

#define D3D11_FILTER_ANISOTROPIC 0x55

// MGS 2 / MGS 3: Forced anisotropy via CD3DCachedDevice::SetSamplerState hook
DWORD64 MGS2_MGS3_SetSamplerStateAnisoReturnJMP;
DWORD64 gpRenderBackend = 0;
void __declspec(naked) MGS2_MGS3_SetSamplerStateAniso_CC()
{
    __asm
    {
        mov rax, [gpRenderBackend]
        mov rax, [rax]

        mov r9d, [iAnisotropicFiltering]

        // [rcx+rax+0x438] = D3D11_SAMPLER_DESC, +0x14 = MaxAnisotropy
        mov [rcx+rax+0x438 + 0x14], r9d

        // Override filter mode in r9d with aniso value and run compare from orig game code
        // Game code will then copy in r9d & update D3D etc when r9d is different to existing value
        mov r9d, D3D11_FILTER_ANISOTROPIC
        cmp [rcx+rax+0x438], r9d
        jmp [MGS2_MGS3_SetSamplerStateAnisoReturnJMP]
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
    inipp::get_value(ini.sections["Anisotropic Filtering"], "Samples", iAnisotropicFiltering);
    inipp::get_value(ini.sections["Framebuffer Fix"], "Enabled", bFramebufferFix);
    inipp::get_value(ini.sections["Skip Intro Logos"], "Enabled", bSkipIntroLogos);
    inipp::get_value(ini.sections["Disable Background Input"], "Enabled", bDisableBackgroundInput);
    inipp::get_value(ini.sections["Disable Mouse Cursor"], "Enabled", bDisableCursor);
    inipp::get_value(ini.sections["Mouse Sensitivity"], "Enabled", bMouseSensitivity);
    inipp::get_value(ini.sections["Mouse Sensitivity"], "X Multiplier", fMouseSensitivityXMulti);
    inipp::get_value(ini.sections["Mouse Sensitivity"], "Y Multiplier", fMouseSensitivityYMulti);
    inipp::get_value(ini.sections["Fix Aspect Ratio"], "Enabled", bAspectFix);
    inipp::get_value(ini.sections["Fix HUD"], "Enabled", bHUDFix);
    inipp::get_value(ini.sections["Texture Buffer"], "SizeMB", iTextureBufferSizeMB);

    // Log config parse
    LOG_F(INFO, "Config Parse: iInjectionDelay: %dms", iInjectionDelay);
    LOG_F(INFO, "Config Parse: bCustomResolution: %d", bCustomResolution);
    LOG_F(INFO, "Config Parse: iCustomResX: %d", iCustomResX);
    LOG_F(INFO, "Config Parse: iCustomResY: %d", iCustomResY);
    LOG_F(INFO, "Config Parse: bWindowedMode: %d", bWindowedMode);
    LOG_F(INFO, "Config Parse: bBorderlessMode: %d", bBorderlessMode);
    LOG_F(INFO, "Config Parse: iAnisotropicFiltering: %d", iAnisotropicFiltering);
    if (iAnisotropicFiltering < 0 || iAnisotropicFiltering > 16)
    {
        iAnisotropicFiltering = std::clamp(iAnisotropicFiltering, 0, 16);
        LOG_F(INFO, "Config Parse: iAnisotropicFiltering value invalid, clamped to %d", iAnisotropicFiltering);
    }
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
                fMGS2_EffectScaleY = (float)fMGS2_DefaultEffectScaleY / (fMGS2_DefaultHUDX / fNewX);
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
            DWORD64 MGS2_RadarWidthAddress = (uintptr_t)MGS2_RadarWidthScanResult;
            int MGS2_RadarWidthHookLength = Memory::GetHookLength((char*)MGS2_RadarWidthAddress, 13);
            MGS2_RadarWidthReturnJMP = MGS2_RadarWidthAddress + MGS2_RadarWidthHookLength;
            Memory::DetourFunction64((void*)MGS2_RadarWidthAddress, MGS2_RadarWidth_CC, MGS2_RadarWidthHookLength);

            LOG_F(INFO, "MGS 2: Radar Width: Hook length is %d bytes", MGS2_RadarWidthHookLength);
            LOG_F(INFO, "MGS 2: Radar Width: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_RadarWidthAddress);

            DWORD64 MGS2_RadarWidthOffsetAddress = (uintptr_t)MGS2_RadarWidthScanResult + 0x42;
            int MGS2_RadarWidthOffsetHookLength = 18; // length disassembler causes crashes for some reason?
            MGS2_RadarWidthOffsetReturnJMP = MGS2_RadarWidthOffsetAddress + MGS2_RadarWidthOffsetHookLength;
            Memory::DetourFunction64((void*)MGS2_RadarWidthOffsetAddress, MGS2_RadarWidthOffset_CC, MGS2_RadarWidthOffsetHookLength);

            LOG_F(INFO, "MGS 2: Radar Width Offset: Hook length is %d bytes", MGS2_RadarWidthOffsetHookLength);
            LOG_F(INFO, "MGS 2: Radar Width Offset: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_RadarWidthOffsetAddress);

            DWORD64 MGS2_RadarHeightOffsetAddress = (uintptr_t)MGS2_RadarWidthScanResult + 0x88;
            int MGS2_RadarHeightOffsetHookLength = 16;
            MGS2_RadarHeightOffsetReturnJMP = MGS2_RadarHeightOffsetAddress + MGS2_RadarHeightOffsetHookLength;
            MGS2_RadarHeightOffsetValueAddress = *(int*)(MGS2_RadarHeightOffsetAddress + 0xC) + (MGS2_RadarHeightOffsetAddress + 0xC) + 0x4;
            Memory::DetourFunction64((void*)MGS2_RadarHeightOffsetAddress, MGS2_RadarHeightOffset_CC, MGS2_RadarHeightOffsetHookLength);

            LOG_F(INFO, "MGS 2: Radar Height Offset: Hook length is %d bytes", MGS2_RadarHeightOffsetHookLength);
            LOG_F(INFO, "MGS 2: Radar Height Offset: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_RadarHeightOffsetAddress);
        }
        else if (!MGS2_RadarWidthScanResult)
        {
            LOG_F(INFO, "MGS 2: Radar Fix: Pattern scan failed.");
        }

         // MGS 2: Codec Portraits
        uint8_t* MGS2_CodecPortraitsScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? 66 0F ?? ?? 0F ?? ??");
        if (MGS2_CodecPortraitsScanResult)
        {
            DWORD64 MGS2_CodecPortraitsAddress = (uintptr_t)MGS2_CodecPortraitsScanResult;
            int MGS2_CodecPortraitsHookLength = Memory::GetHookLength((char*)MGS2_CodecPortraitsAddress, 13);
            MGS2_CodecPortraitsReturnJMP = MGS2_CodecPortraitsAddress + MGS2_CodecPortraitsHookLength;
            Memory::DetourFunction64((void*)MGS2_CodecPortraitsAddress, MGS2_CodecPortraits_CC, MGS2_CodecPortraitsHookLength);

            LOG_F(INFO, "MGS 2: Codec Portraits: Hook length is %d bytes", MGS2_CodecPortraitsHookLength);
            LOG_F(INFO, "MGS 2: Codec Portraits: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_CodecPortraitsAddress);
        }
        else if (!MGS2_CodecPortraitsScanResult)
        {
            LOG_F(INFO, "MGS 2: Codec Portraits: Pattern scan failed.");
        }

        // MGS 2: Disable motion blur. 
        uint8_t* MGS2_MotionBlurScanResult = Memory::PatternScan(baseModule, "F3 48 ?? ?? ?? ?? 48 ?? ?? ?? 48 ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ??");
        if (MGS2_MotionBlurScanResult && bWindowedMode)
        {
            DWORD64 MGS2_MotionBlurAddress = (uintptr_t)MGS2_MotionBlurScanResult;
            LOG_F(INFO, "MGS 2: Motion Blur: Address is 0x%" PRIxPTR, (uintptr_t)MGS2_MotionBlurAddress);

            Memory::PatchBytes(MGS2_MotionBlurAddress, "\x48\x31\xDB\x90\x90\x90", 6);
            LOG_F(INFO, "MGS 2: Motion Blur: Patched instruction.");
        }
        else if (!MGS2_MotionBlurScanResult)
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
    else if ((sExeName == "METAL GEAR.exe" && fNewAspect > fNativeAspect) || (sExeName == "METAL GEAR.exe" && fNewAspect < fNativeAspect))
    {
        // MG1/MG2: HUD
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
            int MGS3_HUDWidthHookLength = 15;
            MGS3_HUDWidthReturnJMP = MGS3_HUDWidthAddress + MGS3_HUDWidthHookLength;
            Memory::DetourFunction64((void*)MGS3_HUDWidthAddress, MGS3_HUDWidth_CC, MGS3_HUDWidthHookLength);

            LOG_F(INFO, "MG1/MG2: HUD Width: Hook length is %d bytes", MGS3_HUDWidthHookLength);
            LOG_F(INFO, "MG1/MG2: HUD Width: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS3_HUDWidthAddress);
        }
        else if (!MGS3_HUDWidthScanResult)
        {
            LOG_F(INFO, "MG1/MG2: HUD Width: Pattern scan failed.");
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
    if (sExeName == "METAL GEAR SOLID2.exe" || sExeName == "METAL GEAR SOLID3.exe" || sExeName == "METAL GEAR.exe")
    {
        if (bDisableCursor)
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

        if (bWindowedMode && bDisableBackgroundInput)
        {
            // MG/MG2 | MGS 2 | MGS 3: Disable Background Input
            uint8_t* MGS_WndProc_IsWindowedCheck = Memory::PatternScan(baseModule, "83 BF 80 02 00 00 00 0F");
            uint8_t* MGS_WndProc_ShowWindowCall = Memory::PatternScan(baseModule, "8B D5 FF 15 ?? ?? ?? ?? 48 8B 8F B0 02 00 00");
            uint8_t* MGS_WndProc_SetFullscreenEnd = Memory::PatternScan(baseModule, "39 AF 48 09 00 00");
            if (MGS_WndProc_IsWindowedCheck && MGS_WndProc_ShowWindowCall && MGS_WndProc_SetFullscreenEnd)
            {
                LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Disable Background Input: IsWindowedCheck at 0x%" PRIxPTR, (uintptr_t)MGS_WndProc_IsWindowedCheck);
                LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Disable Background Input: ShowWindowCall at 0x%" PRIxPTR, (uintptr_t)MGS_WndProc_ShowWindowCall);
                LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Disable Background Input: SetFullscreenEnd at 0x%" PRIxPTR, (uintptr_t)MGS_WndProc_SetFullscreenEnd);

                // Patch out the jnz after the windowed check
                Memory::PatchBytes((uintptr_t)(MGS_WndProc_IsWindowedCheck + 7), "\x90\x90\x90\x90\x90\x90", 6);

                // We included 2 more bytes in MGS_WndProc_ShowWindowCall sig to reduce matches, but we want to keep those
                MGS_WndProc_ShowWindowCall += 2;

                // Skip the ShowWindow & SetFullscreenState block by figuring out how many bytes to skip over
                uint8_t jmper[] = { 0xEB, 0x00 };
                jmper[1] = (uint8_t)((uintptr_t)MGS_WndProc_SetFullscreenEnd - (uintptr_t)(MGS_WndProc_ShowWindowCall + 2));

                LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Disable Background Input: ShowWindowCall jmp skipping %x bytes", jmper[1]);
                Memory::PatchBytes((uintptr_t)MGS_WndProc_ShowWindowCall, (const char*)jmper, 2);
            }
            else
            {
                LOG_F(INFO, "MG/MG2 | MGS 2 | MGS 3: Disable Background Input: Pattern scan failed.");
            }
        }
    }

    if (iAnisotropicFiltering > 0 && (sExeName == "METAL GEAR SOLID3.exe" || sExeName == "METAL GEAR SOLID2.exe"))
    {
        uint8_t* MGS2_MGS3_SetSamplerStateInsnResult = Memory::PatternScan(baseModule, "48 8B 05 ?? ?? ?? ?? 44 39 8C 01 38 04 00 00");
        if (MGS2_MGS3_SetSamplerStateInsnResult)
        {
            DWORD64 MGS2_MGS3_SetSamplerStateInsnAddress = (uintptr_t)MGS2_MGS3_SetSamplerStateInsnResult;
            DWORD64 gpRenderBackendPtrAddr = MGS2_MGS3_SetSamplerStateInsnAddress + 3;

            gpRenderBackend = *(int*)gpRenderBackendPtrAddr + gpRenderBackendPtrAddr + 4;
            LOG_F(INFO, "MGS 2 | MGS 3: Anisotropic Filtering: gpRenderBackend = 0x%" PRIxPTR, (uintptr_t)gpRenderBackend);

            int MGS2_MGS3_SetSamplerStateInsnHookLength = Memory::GetHookLength((char*)MGS2_MGS3_SetSamplerStateInsnAddress, 14);

            LOG_F(INFO, "MGS 2 | MGS 3: Anisotropic Filtering: Hook length is %d bytes", MGS2_MGS3_SetSamplerStateInsnHookLength);
            LOG_F(INFO, "MGS 2 | MGS 3: Anisotropic Filtering: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_MGS3_SetSamplerStateInsnAddress);

            MGS2_MGS3_SetSamplerStateAnisoReturnJMP = MGS2_MGS3_SetSamplerStateInsnAddress + MGS2_MGS3_SetSamplerStateInsnHookLength;
            Memory::DetourFunction64((void*)MGS2_MGS3_SetSamplerStateInsnAddress, MGS2_MGS3_SetSamplerStateAniso_CC, MGS2_MGS3_SetSamplerStateInsnHookLength);
        }
        else
        {
            LOG_F(INFO, "MGS 2 | MGS 3: Anisotropic Filtering: Sampler state pattern scan failed.");
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

    if (iTextureBufferSizeMB > 16 && (sExeName == "METAL GEAR SOLID3.exe" || sExeName == "METAL GEAR.exe"))
    {
        // MG/MG2 | MGS3: texture buffer size extension
        uint32_t NewSize = iTextureBufferSizeMB * 1024 * 1024;

        // Scan for the 9 mallocs which set buffer inside CTextureBuffer::sInstance
        bool failure = false;
        for(int i = 0; i < 9; i++)
        {
            uint8_t* MGS3_CTextureBufferMallocResult = Memory::PatternScan(baseModule, "75 ?? B9 00 00 00 01 FF");
            if(MGS3_CTextureBufferMallocResult)
            {
                uint32_t* bufferAmount = (uint32_t*)(MGS3_CTextureBufferMallocResult + 3);
                LOG_F(INFO, "MG/MG2 | MGS 3: Texture Buffer Size: #%d (0x%" PRIxPTR ") old buffer size: %d", i, MGS3_CTextureBufferMallocResult, *bufferAmount);
                Memory::Write((uintptr_t)bufferAmount, NewSize);
                LOG_F(INFO, "MG/MG2 | MGS 3: Texture Buffer Size: #%d (0x%" PRIxPTR ") new buffer size: %d", i, MGS3_CTextureBufferMallocResult, *bufferAmount);
            }
            else
            {
                LOG_F(INFO, "MG/MG2 | MGS 3: Texture Buffer Size: #%d: Pattern scan failed.", i);
                failure = true;
                break;
            }
        }

        if (!failure)
        {
            // CBaseTexture::Create seems to contain code that mallocs buffers based on 16MiB shifted by index of the mip being loaded
            // (ie: size = 16MiB >> mipIndex)
            // We'll make sure to increase the base 16MiB size it uses too
            uint8_t* MGS3_CBaseTextureMallocResult = Memory::PatternScan(baseModule, "75 ?? B8 00 00 00 01");
            if (MGS3_CBaseTextureMallocResult)
            {
                uint32_t* bufferAmount = (uint32_t*)(MGS3_CBaseTextureMallocResult + 3);
                LOG_F(INFO, "MG/MG2 | MGS 3: Texture Buffer Size: #%d (0x%" PRIxPTR ") old buffer size: %d", 9, MGS3_CBaseTextureMallocResult, *bufferAmount);
                Memory::Write((uintptr_t)bufferAmount, NewSize);
                LOG_F(INFO, "MG/MG2 | MGS 3: Texture Buffer Size: #%d (0x%" PRIxPTR ") new buffer size: %d", 9, MGS3_CBaseTextureMallocResult, *bufferAmount);
            }
            else
            {
                LOG_F(INFO, "MG/MG2 | MGS 3: Texture Buffer Size: #%d: Pattern scan failed.", 9);
            }
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

