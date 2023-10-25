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
string sFixVer = "0.8";

// MGS 3: Aspect Ratio Hook
DWORD64 MGS3_GameplayAspectReturnJMP;
DWORD64 MGS3_GameplayAspectCallAddress;
void __declspec(naked) MGS3_GameplayAspect_CC()
{
    __asm
    {
        movss xmm1, [fAspectMultiplier]
        movaps xmm0, xmm2
        call MGS3_GameplayAspectCallAddress
        jmp[MGS3_GameplayAspectReturnJMP]
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

    // Log config parse
    LOG_F(INFO, "Config Parse: iInjectionDelay: %dms", iInjectionDelay);
    LOG_F(INFO, "Config Parse: bCustomResolution: %d", bCustomResolution);
    LOG_F(INFO, "Config Parse: iCustomResX: %d", iCustomResX);
    LOG_F(INFO, "Config Parse: iCustomResY: %d", iCustomResY);
    LOG_F(INFO, "Config Parse: bAspectFix: %d", bAspectFix);
    LOG_F(INFO, "Config Parse: bHUDFix: %d", bHUDFix);
    LOG_F(INFO, "Config Parse: bMovieFix: %d", bMovieFix);
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
    if (sExeName == "METAL GEAR SOLID3.exe" && bCustomResolution)
    {
        // MGS 3: Custom Resolution
        uint8_t* MGS3_ResolutionScanResult = Memory::PatternScan(baseModule, "C7 45 ?? 00 05 00 00 C7 ?? ?? D0 02 00 00 C7 ?? ?? 00 05 00 00 C7 ?? ?? D0 02 00 00");
        if (MGS3_ResolutionScanResult)
        {
            DWORD64 MGS3_ResolutionAddress = (uintptr_t)MGS3_ResolutionScanResult + 0x3;
            LOG_F(INFO, "MGS 3: Custom Resolution: Address is 0x%" PRIxPTR, (uintptr_t)MGS3_ResolutionAddress);

            Memory::Write(MGS3_ResolutionAddress, iCustomResX);
            Memory::Write((MGS3_ResolutionAddress + 0x7), iCustomResY);
            Memory::Write((MGS3_ResolutionAddress + 0xE), iCustomResX);
            Memory::Write((MGS3_ResolutionAddress + 0x15), iCustomResY);
            LOG_F(INFO, "MGS 3: Custom Resolution: New Custom Resolution = %dx%d", iCustomResX, iCustomResY);
        }
        else if (!MGS3_ResolutionScanResult)
        {
            LOG_F(INFO, "MGS 3: Custom Resolution: Pattern scan failed.");
        }
    }
}

void AspectFOVFix()
{
    if (sExeName == "METAL GEAR SOLID3.exe" && bAspectFix)
    {
        // MGS 3: Fix gameplay aspect ratio
        uint8_t* MGS3_GameplayAspectScanResult = Memory::PatternScan(baseModule, "0F ?? ?? EB ?? F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ?? E8 ?? ?? ?? ?? 0F ?? ?? 0F ?? ?? 0F ?? ?? F3 ?? ?? ?? ??");
        if (MGS3_GameplayAspectScanResult)
        {
            MGS3_GameplayAspectCallAddress = Memory::GetAbsolute((uintptr_t)MGS3_GameplayAspectScanResult + 0x11);
            LOG_F(INFO, "MGS 3: Aspect Ratio: Call address is 0x%" PRIxPTR, (uintptr_t)MGS3_GameplayAspectCallAddress);

            DWORD64 MGS3_GameplayAspectAddress = (uintptr_t)MGS3_GameplayAspectScanResult + 0x5;
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
}

void HUDFix()
{
    if (sExeName == "METAL GEAR SOLID3.exe" && bHUDFix)
    {
        // MGS3: HUD width
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
    
}

DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    DetectGame();
    CustomResolution();
    Sleep(iInjectionDelay);
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

