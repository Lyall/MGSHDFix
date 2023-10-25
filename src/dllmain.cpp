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
string sExeName;
string sGameName;
string sExePath;
string sGameVersion;
string sFixVer = "0.4";

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

// MGS 3: HUD Width
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

// MGS 2: Fades
DWORD64 MGS2_FadesReturnJMP;
void __declspec(naked) MGS2_Fades_CC()
{
    __asm
    {
        movss xmm0, [fNewY]
        xorps xmm2, xmm2
        movss xmm3, [fNewX]
        jmp[MGS2_FadesReturnJMP]
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
    LOG_F(INFO, "Config Parse: bAspectFix: %d", bAspectFix);
    LOG_F(INFO, "Config Parse: bHUDFix: %d", bHUDFix);
    LOG_F(INFO, "Config Parse: bMovieFix: %d", bMovieFix);

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
        // MGS 3: Custom Resolution
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
        // MGS 2: Scale fades to span screen
        // TODO: Sig is bad, need better way of getting here.
        uint8_t* MGS2_FadesScanResult = Memory::PatternScan(baseModule, "E8 BF ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ?? 89 ?? ?? ?? 48 ?? ??");
        if (MGS2_FadesScanResult)
        {
            DWORD64 MGS2_FadesAddress = (uintptr_t)MGS2_FadesScanResult + 0x5;
            int MGS2_FadesHookLength = Memory::GetHookLength((char*)MGS2_FadesAddress, 13);
            MGS2_FadesReturnJMP = MGS2_FadesAddress + MGS2_FadesHookLength;
            Memory::DetourFunction64((void*)MGS2_FadesAddress, MGS2_Fades_CC, MGS2_FadesHookLength);

            LOG_F(INFO, "MGS 2: Fades: Hook length is %d bytes", MGS2_FadesHookLength);
            LOG_F(INFO, "MGS 2: Fades: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_FadesAddress);
        }
        else if (!MGS2_FadesScanResult)
        {
            LOG_F(INFO, "MGS 2: Fades: Pattern scan failed.");
        }

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

void Letterboxing()
{
    if (sExeName == "METAL GEAR SOLID2.exe" && bAspectFix || sExeName == "METAL GEAR SOLID3.exe" && bAspectFix)
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

void HUDFix()
{
    if (sExeName == "METAL GEAR SOLID2.exe" && bHUDFix)
    {
        // TODO
    }
    else if (sExeName == "METAL GEAR SOLID3.exe" && bHUDFix)
    {
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
}

void MovieFix()
{
    // Currently the HUD modification affects FMVs too.
}

DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    DetectGame();
    CustomResolution();
    Sleep(iInjectionDelay);
    AspectFOVFix();
    Letterboxing();
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

