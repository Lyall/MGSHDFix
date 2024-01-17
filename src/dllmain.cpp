#include "stdafx.h"
#include "helper.hpp"
#include <inipp/inipp.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <safetyhook.hpp>

using namespace std;

HMODULE baseModule = GetModuleHandle(NULL);

// Logger and config setup
inipp::Ini<char> ini;
string sFixName = "MGSHDFix";
string sFixVer = "2.0";
string sLogFile = "MGSHDFix.log";
string sConfigFile = "MGSHDFix.ini";
string sExeName;
filesystem::path sExePath;

// Ini Variables
bool bAspectFix;
bool bHUDFix;
bool bFOVFix;
bool bCustomResolution;
int iCustomResX;
int iCustomResY;
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

// Launcher ini variables
bool bLauncherConfigSkipLauncher = false;
int iLauncherConfigCtrlType = 5;
int iLauncherConfigRegion = 0;
int iLauncherConfigLanguage = 0;
std::string sLauncherConfigMSXGame = "mg1";
int iLauncherConfigMSXWallType = 0;
std::string sLauncherConfigMSXWallAlign = "C";

// Aspect ratio + HUD stuff
float fNativeAspect = (float)16 / 9;
float fAspectRatio;
float fAspectMultiplier;
float fHUDWidth;
float fHUDHeight;
float fDefaultHUDWidth = (float)1920;
float fDefaultHUDHeight = (float)1080;
float fHUDWidthOffset;
float fHUDHeightOffset;

const std::initializer_list<std::string> kLauncherConfigCtrlTypes = {
    "ps5",
    "ps4",
    "xbox",
    "nx",
    "stmd",
    "kbd"
};

const std::initializer_list<std::string> kLauncherConfigLanguages = {
    "en",
    "jp",
    "fr",
    "gr",
    "it",
    "pr",
    "sp",
    "du",
    "ru"
};

const std::initializer_list<std::string> kLauncherConfigRegions = {
    "us",
    "jp",
    "eu"
};

struct GameInfo
{
    std::string GameTitle;
    std::string ExeName;
    int SteamAppId;
};

enum class MgsGame
{
    Unknown,
    MGS2,
    MGS3,
    MG,
    Launcher
};

const std::map<MgsGame, GameInfo> kGames = {
    {MgsGame::MGS2, {"Metal Gear Solid 2 HD", "METAL GEAR SOLID2.exe", 2131640}},
    {MgsGame::MGS3, {"Metal Gear Solid 3 HD", "METAL GEAR SOLID3.exe", 2131650}},
    {MgsGame::MG, {"Metal Gear / Metal Gear 2 (MSX)", "METAL GEAR.exe", 2131680}},
};

const GameInfo* game = nullptr;
MgsGame eGameType = MgsGame::Unknown;

void Logging()
{
    // spdlog initialisation
    {
        try
        {
            auto logger = spdlog::basic_logger_mt(sFixName.c_str(), sLogFile, true);
            spdlog::set_default_logger(logger);

        }
        catch (const spdlog::spdlog_ex& ex)
        {
            AllocConsole();
            FILE* dummy;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            std::cout << "Log initialisation failed: " << ex.what() << std::endl;
        }
    }

    spdlog::flush_on(spdlog::level::debug);
    spdlog::info("{} v{} loaded.", sFixName.c_str(), sFixVer.c_str());
    spdlog::info("----------");

    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();

    // Log module details
    spdlog::info("Module Name: {0:s}", sExeName.c_str());
    spdlog::info("Module Path: {0:s}", sExePath.string().c_str());
    spdlog::info("Module Address: 0x{0:x}", (uintptr_t)baseModule);
    spdlog::info("Module Timesstamp: {0:d}", Memory::ModuleTimestamp(baseModule));
    spdlog::info("----------");
}

void ReadConfig()
{
    // Initialise config
    std::ifstream iniFile(sConfigFile);
    if (!iniFile)
    {
        spdlog::critical("Failed to load config file.");
        spdlog::critical("Make sure {} is present in the game folder.", sConfigFile);

    }
    else
    {
        ini.parse(iniFile);
    }

    // Read ini file
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
    inipp::get_value(ini.sections["Texture Buffer"], "SizeMB", iTextureBufferSizeMB);
    inipp::get_value(ini.sections["Fix Aspect Ratio"], "Enabled", bAspectFix);
    inipp::get_value(ini.sections["Fix HUD"], "Enabled", bHUDFix);
    inipp::get_value(ini.sections["Fix FOV"], "Enabled", bFOVFix);
    inipp::get_value(ini.sections["Launcher Config"], "SkipLauncher", bLauncherConfigSkipLauncher);

    // Read launcher settings from ini
    std::string sLauncherConfigCtrlType = "kbd";
    std::string sLauncherConfigRegion = "us";
    std::string sLauncherConfigLanguage = "en";
    inipp::get_value(ini.sections["Launcher Config"], "CtrlType", sLauncherConfigCtrlType);
    inipp::get_value(ini.sections["Launcher Config"], "Region", sLauncherConfigRegion);
    inipp::get_value(ini.sections["Launcher Config"], "Language", sLauncherConfigLanguage);
    inipp::get_value(ini.sections["Launcher Config"], "MSXGame", sLauncherConfigMSXGame);
    inipp::get_value(ini.sections["Launcher Config"], "MSXWallType", iLauncherConfigMSXWallType);
    inipp::get_value(ini.sections["Launcher Config"], "MSXWallAlign", sLauncherConfigMSXWallAlign);
    iLauncherConfigCtrlType = Util::findStringInVector(sLauncherConfigCtrlType, kLauncherConfigCtrlTypes);
    iLauncherConfigRegion = Util::findStringInVector(sLauncherConfigRegion, kLauncherConfigRegions);
    iLauncherConfigLanguage = Util::findStringInVector(sLauncherConfigLanguage, kLauncherConfigLanguages);

    // Log config parse
    spdlog::info("Config Parse: bCustomResolution: {}", bCustomResolution);
    spdlog::info("Config Parse: iCustomResX: {}", iCustomResX);
    spdlog::info("Config Parse: iCustomResY: {}", iCustomResY);
    spdlog::info("Config Parse: bWindowedMode: {}", bWindowedMode);
    spdlog::info("Config Parse: bBorderlessMode: {}", bBorderlessMode);
    spdlog::info("Config Parse: iAnisotropicFiltering: {}", iAnisotropicFiltering);
    if (iAnisotropicFiltering < 0 || iAnisotropicFiltering > 16)
    {
        iAnisotropicFiltering = std::clamp(iAnisotropicFiltering, 0, 16);
        spdlog::info("Config Parse: iAnisotropicFiltering value invalid, clamped to {}", iAnisotropicFiltering);
    }
    spdlog::info("Config Parse: bFramebufferFix: {}", bFramebufferFix);
    spdlog::info("Config Parse: bSkipIntroLogos: {}", bSkipIntroLogos);
    spdlog::info("Config Parse: bDisableCursor: {}", bDisableCursor);
    spdlog::info("Config Parse: bMouseSensitivity: {}", bMouseSensitivity);
    spdlog::info("Config Parse: fMouseSensitivityXMulti: {}", fMouseSensitivityXMulti);
    spdlog::info("Config Parse: fMouseSensitivityYMulti: {}", fMouseSensitivityYMulti);
    spdlog::info("Config Parse: iTextureBufferSizeMB: {}", iTextureBufferSizeMB);
    spdlog::info("Config Parse: bAspectFix: {}", bAspectFix);
    spdlog::info("Config Parse: bHUDFix: {}", bHUDFix);
    spdlog::info("Config Parse: bFOVFix: {}", bFOVFix);
    spdlog::info("Config Parse: bLauncherConfigSkipLauncher: {}", bLauncherConfigSkipLauncher);
    spdlog::info("Config Parse: iLauncherConfigCtrlType: {}", iLauncherConfigCtrlType);
    spdlog::info("Config Parse: iLauncherConfigRegion: {}", iLauncherConfigRegion);
    spdlog::info("Config Parse: iLauncherConfigLanguage: {}", iLauncherConfigLanguage);
    spdlog::info("----------");

    // Calculate aspect ratio / use desktop res instead
    if (iCustomResX > 0 && iCustomResY > 0)
    {
        fAspectRatio = (float)iCustomResX / (float)iCustomResY;
    }
    else
    {
        RECT desktop;
        GetWindowRect(GetDesktopWindow(), &desktop);
        iCustomResX = (int)desktop.right;
        iCustomResY = (int)desktop.bottom;
        fAspectRatio = (float)desktop.right / (float)desktop.bottom;
    }
    fAspectMultiplier = fAspectRatio / fNativeAspect;
    fHUDWidth = fDefaultHUDHeight * fAspectRatio;
    fHUDWidthOffset = (float)(iCustomResX - fHUDWidth) / 2;

    // Log aspect ratio stuff
    spdlog::info("Custom Resolution: fAspectRatio: {}", fAspectRatio);
    spdlog::info("Custom Resolution: fAspectMultiplier: {}", fAspectMultiplier);
    spdlog::info("Custom Resolution: fHUDWidth: {}", fHUDWidth);
    spdlog::info("Custom Resolution: fHUDHeight: {}", fHUDHeight);
    spdlog::info("Custom Resolution: fHUDWidthOffset: {}", fHUDWidthOffset);
    spdlog::info("Custom Resolution: fHUDHeightOffset: {}", fHUDHeightOffset);
    spdlog::info("----------");
}

bool DetectGame()
{
    eGameType = MgsGame::Unknown;
    // Special handling for launcher.exe
    if (sExeName == "launcher.exe")
    {
        for (const auto& [type, info] : kGames)
        {
            auto gamePath = sExePath.parent_path() / info.ExeName;
            if (std::filesystem::exists(gamePath))
            {
                spdlog::info("Detected launcher for game: {} (app {})", info.GameTitle.c_str(), info.SteamAppId);
                eGameType = MgsGame::Launcher;
                game = &info;
                return true;
            }
        }

        spdlog::info("Failed to detect supported game, unknown launcher");
        return false;
    }

    for (const auto& [type, info] : kGames)
    {
        if (info.ExeName == sExeName)
        {
            spdlog::info("Detected game: {} (app {})", info.GameTitle.c_str(), info.SteamAppId);
            eGameType = type;
            game = &info;
            return true;
        }
    }

    spdlog::info("Failed to detect supported game, {} isn't supported by MGSHDFix", sExeName.c_str());
    return false;
}

void CustomResolution()
{
    if ((eGameType == MgsGame::MGS2 || eGameType == MgsGame::MGS3 || eGameType == MgsGame::MG) && bCustomResolution)
    {
        // MGS 2 | MGS 3: Custom Resolution
        uint8_t* MGS2_MGS3_ResolutionScanResult = Memory::PatternScan(baseModule, "C7 45 ?? 00 05 00 00 C7 ?? ?? D0 02 00 00 C7 ?? ?? 00 05 00 00 C7 ?? ?? D0 02 00 00");
        if (MGS2_MGS3_ResolutionScanResult)
        {
            DWORD64 MGS2_MGS3_ResolutionAddress = (uintptr_t)MGS2_MGS3_ResolutionScanResult + 0x3;
            spdlog::info("MG/MG2 | MGS 2 | MGS 3: Custom Resolution: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_ResolutionAddress - (uintptr_t)baseModule);

            Memory::Write(MGS2_MGS3_ResolutionAddress, iCustomResX);
            Memory::Write((MGS2_MGS3_ResolutionAddress + 0x7), iCustomResY);
            Memory::Write((MGS2_MGS3_ResolutionAddress + 0xE), iCustomResX);
            Memory::Write((MGS2_MGS3_ResolutionAddress + 0x15), iCustomResY);
            spdlog::info("MG/MG2 | MGS 2 | MGS 3: Custom Resolution: New Custom Resolution = {}x{}", iCustomResX, iCustomResY);
        }
        else if (!MGS2_MGS3_ResolutionScanResult)
        {
            spdlog::info("MG/MG2 | MGS 2 | MGS 3: Custom Resolution: Pattern scan failed.");
        }

        // MG 1/2 | MGS 2 | MGS 3: WindowedMode
        uint8_t* MGS2_MGS3_WindowedModeScanResult = Memory::PatternScan(baseModule, "49 ?? ?? 01 75 ?? 0F ?? ?? 41 ?? ?? 0F ?? ?? ?? ?? ??");
        if (MGS2_MGS3_WindowedModeScanResult)
        {
            spdlog::info("MG/MG2 | MGS 2 | MGS 3: WindowedMode: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_WindowedModeScanResult - (uintptr_t)baseModule);
            
            // Set windowed mode
            static SafetyHookMid WindowedModeMidHook{};
            WindowedModeMidHook = safetyhook::create_mid(MGS2_MGS3_WindowedModeScanResult + 0x54,
                [](SafetyHookContext& ctx)
                {
                    if (bWindowedMode || bBorderlessMode) 
                    { 
                        ctx.rax = 1; 
                    }
                    else 
                    { 
                        ctx.rax = 0;
                    }
                });

            // Set window size X
            static SafetyHookMid WindowedModeXMidHook{};
            WindowedModeXMidHook = safetyhook::create_mid(MGS2_MGS3_WindowedModeScanResult + 0x60,
                [](SafetyHookContext& ctx)
                {
                    ctx.rax = iCustomResX;
                });

            // Set window size Y
            static SafetyHookMid WindowedModeYMidHook{};
            WindowedModeYMidHook = safetyhook::create_mid(MGS2_MGS3_WindowedModeScanResult + 0x6C,
                [](SafetyHookContext& ctx)
                {
                    ctx.rax = iCustomResY;
                }); 
        }
        else if (!MGS2_MGS3_WindowedModeScanResult)
        {
            spdlog::error("MG/MG2 | MGS 2 | MGS 3: WindowedMode: Pattern scan failed.");
        }
        
        // MG 1/2 | MGS 2 | MGS 3: CreateWindowExA
        // TODO: Make a proper hook for this.
        uint8_t* MGS2_MGS3_CreateWindowExAScanResult = Memory::PatternScan(baseModule, "FF ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 48 ?? ?? 48 ?? ?? ?? 48 ?? ?? ?? ?? 48 ?? ?? ?? ?? ?? ?? 48 ?? ??");
        if (MGS2_MGS3_CreateWindowExAScanResult)
        {
            spdlog::info("MG/MG2 | MGS 2 | MGS 3: CreateWindowExA: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_CreateWindowExAScanResult - (uintptr_t)baseModule);
            
            static SafetyHookMid CreateWindowExAMidHook{};
            CreateWindowExAMidHook = safetyhook::create_mid(MGS2_MGS3_CreateWindowExAScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (bBorderlessMode)
                    {
                        ctx.r9 = 0x90000008;
                    }
                });
        }
        else if (!MGS2_MGS3_CreateWindowExAScanResult)
        {
            spdlog::error("MG/MG2 | MGS 2 | MGS 3: CreateWindowExA: Pattern scan failed.");
        }

        // MGS 2 | MGS 3: Framebuffer fix, stops the framebuffer from being set to maximum display resolution.
        // Thanks emoose!
        if (bFramebufferFix)
        {
            // Need to stop hor + vert from being modified.
            for (int i = 1; i <= 2; ++i)
            {
                uint8_t* MGS2_MGS3_FramebufferFixScanResult = Memory::PatternScan(baseModule, "03 ?? 41 ?? ?? ?? C7 ?? ?? ?? ?? ?? ?? 00 00 00");
                if (MGS2_MGS3_FramebufferFixScanResult)
                {
                    DWORD64 MGS2_MGS3_FramebufferFixAddress = (uintptr_t)MGS2_MGS3_FramebufferFixScanResult + 0x2;
                    spdlog::info("MG/MG2 | MGS 2 | MGS 3: Framebuffer {}: Address is {:s}+{:x}", i, sExeName.c_str(), (uintptr_t)MGS2_MGS3_FramebufferFixAddress - (uintptr_t)baseModule);

                    Memory::PatchBytes(MGS2_MGS3_FramebufferFixAddress, "\x90\x90\x90\x90", 4);
                    spdlog::info("MG/MG2 | MGS 2 | MGS 3: Framebuffer {}: Patched instruction.", i);
                }
                else if (!MGS2_MGS3_FramebufferFixScanResult)
                {
                    spdlog::info("MG/MG2 | MGS 2 | MGS 3: Framebuffer {}: Pattern scan failed.", i);
                }
            }
        }      
    }   
}

void IntroSkip()
{
    if (!bSkipIntroLogos)
        return;
    if (eGameType != MgsGame::MGS2 && eGameType != MgsGame::MGS3)
        return;

    uint8_t* MGS2_MGS3_InitialIntroStateScanResult = Memory::PatternScan(baseModule, "75 ? C7 05 ? ? ? ? 01 00 00 00 C3");
    if (!MGS2_MGS3_InitialIntroStateScanResult)
    {
        spdlog::info("MGS 2 | MGS 3: Skip Intro Logos: Pattern scan failed.");
        return;
    }

    uint32_t* MGS2_MGS3_InitialIntroStatePtr = (uint32_t*)(MGS2_MGS3_InitialIntroStateScanResult + 8);
    spdlog::info("MGS 2 | MGS 3: Skip Intro Logos: Initial state: {}", *MGS2_MGS3_InitialIntroStatePtr);

    uint32_t NewState = 3;
    Memory::PatchBytes((uintptr_t)MGS2_MGS3_InitialIntroStatePtr, (const char*)&NewState, sizeof(NewState));
    spdlog::info("MGS 2 | MGS 3: Skip Intro Logos: Patched state: {}", *MGS2_MGS3_InitialIntroStatePtr);
}

void ScaleEffects()
{
    /*
    if (eGameType == MgsGame::MGS2 && bCustomResolution)
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

            spdlog::info("MGS 2: Scale Effects X: Hook length is {} bytes", MGS2_EffectsScaleXHookLength);
            spdlog::info("MGS 2: Scale Effects X: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_EffectsScaleXAddress);

            DWORD64 MGS2_EffectsScaleX2Address = (uintptr_t)MGS2_EffectsScaleScanResult - 0x2B;
            int MGS2_EffectsScaleX2HookLength = Memory::GetHookLength((char*)MGS2_EffectsScaleX2Address, 13);
            MGS2_EffectsScaleX2ReturnJMP = MGS2_EffectsScaleX2Address + MGS2_EffectsScaleX2HookLength;
            Memory::DetourFunction64((void*)MGS2_EffectsScaleX2Address, MGS2_EffectsScaleX2_CC, MGS2_EffectsScaleX2HookLength);

            spdlog::info("MGS 2: Scale Effects X 2: Hook length is {} bytes", MGS2_EffectsScaleX2HookLength);
            spdlog::info("MGS 2: Scale Effects X 2: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_EffectsScaleX2Address);

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

            spdlog::info("MGS 2: Scale Effects Y: Hook length is {} bytes", MGS2_EffectsScaleYHookLength);
            spdlog::info("MGS 2: Scale Effects Y: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_EffectsScaleYAddress);
        }
        else if (!MGS2_EffectsScaleScanResult)
        {
            spdlog::info("MGS 2: Scale Effects: Pattern scan failed.");
        }
    }
    */
}

void AspectFOVFix()
{
    /*
    if ((eGameType == MgsGame::MGS3 || eGameType == MgsGame::MG) && bAspectFix)
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

            spdlog::info("MG/MG2 | MGS 3: Aspect Ratio: Hook length is {} bytes", MGS3_GameplayAspectHookLength);
            spdlog::info("MG/MG2 | MGS 3: Aspect Ratio: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS3_GameplayAspectAddress);
        }
        else if (!MGS3_GameplayAspectScanResult)
        {
            spdlog::info("MG/MG2 | MGS 3: Aspect Ratio: Pattern scan failed.");
        }
    }  
    else if (eGameType == MgsGame::MGS2 && bAspectFix)
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

            spdlog::info("MGS 2: Aspect Ratio: Hook length is {} bytes", MGS2_GameplayAspectHookLength);
            spdlog::info("MGS 2: Aspect Ratio: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_GameplayAspectAddress);
        }
        else if (!MGS2_GameplayAspectScanResult)
        {
            spdlog::info("MGS 2: Aspect Ratio: Pattern scan failed.");
        }
    }
    
    // Convert FOV to vert- to match 16:9 horizontal field of view
    if (eGameType == MgsGame::MGS3 && bNarrowAspect && bFOVFix)
    {
        // MGS 3: FOV
        uint8_t* MGS3_FOVScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? 44 ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 ?? ?? ?? ?? E8 ?? ?? ?? ??");
        if (MGS3_FOVScanResult)
        {
            DWORD64 MGS3_FOVAddress = (uintptr_t)MGS3_FOVScanResult;
            int MGS3_FOVHookLength = Memory::GetHookLength((char*)MGS3_FOVAddress, 13);
            MGS3_FOVReturnJMP = MGS3_FOVAddress + MGS3_FOVHookLength;
            Memory::DetourFunction64((void*)MGS3_FOVAddress, MGS3_FOV_CC, MGS3_FOVHookLength);

            spdlog::info("MGS 3: FOV: Hook length is {} bytes", MGS3_FOVHookLength);
            spdlog::info("MGS 3: FOV: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS3_FOVAddress);
        }
        else if (!MGS3_FOVScanResult)
        {
            spdlog::info("MGS 3: FOV: Pattern scan failed.");
        }
    }
    else if (eGameType == MgsGame::MGS2 && bNarrowAspect && bFOVFix)
    {
        // MGS 2: FOV
        uint8_t* MGS2_FOVScanResult = Memory::PatternScan(baseModule, "44 ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 44 ?? ?? ?? ?? 48 ?? ?? 48 ?? ?? ?? ?? 00 00");
        if (MGS2_FOVScanResult)
        {
            DWORD64 MGS2_FOVAddress = (uintptr_t)MGS2_FOVScanResult;
            int MGS2_FOVHookLength = 18;
            MGS2_FOVReturnJMP = MGS2_FOVAddress + MGS2_FOVHookLength;
            Memory::DetourFunction64((void*)MGS2_FOVAddress, MGS2_FOV_CC, MGS2_FOVHookLength);

            spdlog::info("MGS 2: FOV: Hook length is {} bytes", MGS2_FOVHookLength);
            spdlog::info("MGS 2: FOV: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_FOVAddress);
        }
        else if (!MGS2_FOVScanResult)
        {
            spdlog::info("MGS 2: FOV: Pattern scan failed.");
        }
    }
    */
}

void HUDFix()
{
    /*
    if (eGameType == MgsGame::MGS2 && bHUDFix)
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

            spdlog::info("MGS 2: HUD Width: Hook length is {} bytes", MGS2_HUDWidthHookLength);
            spdlog::info("MGS 2: HUD Width: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_HUDWidthAddress);
        }
        else if (!MGS2_HUDWidthScanResult)
        {
            spdlog::info("MGS 2: HUD Fix: Pattern scan failed.");
        }

        // MGS 2: Radar
        uint8_t* MGS2_RadarWidthScanResult = Memory::PatternScan(baseModule, "44 ?? ?? 8B ?? 0F ?? ?? ?? 41 ?? ?? 0F ?? ?? ?? 44 ?? ?? ?? ?? ?? ?? 0F ?? ?? ?? 99");
        if (MGS2_RadarWidthScanResult)
        {
            DWORD64 MGS2_RadarWidthAddress = (uintptr_t)MGS2_RadarWidthScanResult;
            int MGS2_RadarWidthHookLength = Memory::GetHookLength((char*)MGS2_RadarWidthAddress, 13);
            MGS2_RadarWidthReturnJMP = MGS2_RadarWidthAddress + MGS2_RadarWidthHookLength;
            Memory::DetourFunction64((void*)MGS2_RadarWidthAddress, MGS2_RadarWidth_CC, MGS2_RadarWidthHookLength);

            spdlog::info("MGS 2: Radar Width: Hook length is {} bytes", MGS2_RadarWidthHookLength);
            spdlog::info("MGS 2: Radar Width: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_RadarWidthAddress);

            DWORD64 MGS2_RadarWidthOffsetAddress = (uintptr_t)MGS2_RadarWidthScanResult + 0x42;
            int MGS2_RadarWidthOffsetHookLength = 18; // length disassembler causes crashes for some reason?
            MGS2_RadarWidthOffsetReturnJMP = MGS2_RadarWidthOffsetAddress + MGS2_RadarWidthOffsetHookLength;
            Memory::DetourFunction64((void*)MGS2_RadarWidthOffsetAddress, MGS2_RadarWidthOffset_CC, MGS2_RadarWidthOffsetHookLength);

            spdlog::info("MGS 2: Radar Width Offset: Hook length is {} bytes", MGS2_RadarWidthOffsetHookLength);
            spdlog::info("MGS 2: Radar Width Offset: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_RadarWidthOffsetAddress);

            DWORD64 MGS2_RadarHeightOffsetAddress = (uintptr_t)MGS2_RadarWidthScanResult + 0x88;
            int MGS2_RadarHeightOffsetHookLength = 16;
            MGS2_RadarHeightOffsetReturnJMP = MGS2_RadarHeightOffsetAddress + MGS2_RadarHeightOffsetHookLength;
            MGS2_RadarHeightOffsetValueAddress = *(int*)(MGS2_RadarHeightOffsetAddress + 0xC) + (MGS2_RadarHeightOffsetAddress + 0xC) + 0x4;
            Memory::DetourFunction64((void*)MGS2_RadarHeightOffsetAddress, MGS2_RadarHeightOffset_CC, MGS2_RadarHeightOffsetHookLength);

            spdlog::info("MGS 2: Radar Height Offset: Hook length is {} bytes", MGS2_RadarHeightOffsetHookLength);
            spdlog::info("MGS 2: Radar Height Offset: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_RadarHeightOffsetAddress);
        }
        else if (!MGS2_RadarWidthScanResult)
        {
            spdlog::info("MGS 2: Radar Fix: Pattern scan failed.");
        }

         // MGS 2: Codec Portraits
        uint8_t* MGS2_CodecPortraitsScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? 66 0F ?? ?? 0F ?? ??");
        if (MGS2_CodecPortraitsScanResult)
        {
            DWORD64 MGS2_CodecPortraitsAddress = (uintptr_t)MGS2_CodecPortraitsScanResult;
            int MGS2_CodecPortraitsHookLength = Memory::GetHookLength((char*)MGS2_CodecPortraitsAddress, 13);
            MGS2_CodecPortraitsReturnJMP = MGS2_CodecPortraitsAddress + MGS2_CodecPortraitsHookLength;
            Memory::DetourFunction64((void*)MGS2_CodecPortraitsAddress, MGS2_CodecPortraits_CC, MGS2_CodecPortraitsHookLength);

            spdlog::info("MGS 2: Codec Portraits: Hook length is {} bytes", MGS2_CodecPortraitsHookLength);
            spdlog::info("MGS 2: Codec Portraits: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_CodecPortraitsAddress);
        }
        else if (!MGS2_CodecPortraitsScanResult)
        {
            spdlog::info("MGS 2: Codec Portraits: Pattern scan failed.");
        }

        // MGS 2: Disable motion blur. 
        uint8_t* MGS2_MotionBlurScanResult = Memory::PatternScan(baseModule, "F3 48 ?? ?? ?? ?? 48 ?? ?? ?? 48 ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ??");
        if (MGS2_MotionBlurScanResult && bWindowedMode)
        {
            DWORD64 MGS2_MotionBlurAddress = (uintptr_t)MGS2_MotionBlurScanResult;
            spdlog::info("MGS 2: Motion Blur: Address is 0x%" PRIxPTR, (uintptr_t)MGS2_MotionBlurAddress);

            Memory::PatchBytes(MGS2_MotionBlurAddress, "\x48\x31\xDB\x90\x90\x90", 6);
            spdlog::info("MGS 2: Motion Blur: Patched instruction.");
        }
        else if (!MGS2_MotionBlurScanResult)
        {
            spdlog::info("MGS 2: Motion Blur: Pattern scan failed.");
        }
    }
    else if (eGameType == MgsGame::MGS3 && bHUDFix)
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

            spdlog::info("MGS 3: HUD Width: Hook length is {} bytes", MGS3_HUDWidthHookLength);
            spdlog::info("MGS 3: HUD Width: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS3_HUDWidthAddress);
        }
        else if (!MGS3_HUDWidthScanResult)
        {
            spdlog::info("MGS 3: HUD Width: Pattern scan failed.");
        }
    }
    else if ((eGameType == MgsGame::MG && fNewAspect > fNativeAspect) || (eGameType == MgsGame::MG && fNewAspect < fNativeAspect))
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

            spdlog::info("MG1/MG2: HUD Width: Hook length is {} bytes", MGS3_HUDWidthHookLength);
            spdlog::info("MG1/MG2: HUD Width: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS3_HUDWidthAddress);
        }
        else if (!MGS3_HUDWidthScanResult)
        {
            spdlog::info("MG1/MG2: HUD Width: Pattern scan failed.");
        }
    }

    if ((eGameType == MgsGame::MGS2 || eGameType == MgsGame::MGS3) && bHUDFix)
    {
        // MGS 2 | MGS 3: Letterboxing
        uint8_t* MGS2_MGS3_LetterboxingScanResult = Memory::PatternScan(baseModule, "83 ?? 01 75 ?? ?? 01 00 00 00 44 ?? ?? ?? ?? ?? ?? 89 ?? ?? ?? ?? ??");
        if (MGS2_MGS3_LetterboxingScanResult)
        {
            DWORD64 MGS2_MGS3_LetterboxingAddress = (uintptr_t)MGS2_MGS3_LetterboxingScanResult + 0x6;
            spdlog::info("MGS 2 | MGS 3: Letterboxing: Address is 0x%" PRIxPTR, (uintptr_t)MGS2_MGS3_LetterboxingAddress);

            Memory::Write(MGS2_MGS3_LetterboxingAddress, (int)0);
            spdlog::info("MGS 2 | MGS 3: Letterboxing: Disabled letterboxing.");
        }
        else if (!MGS2_MGS3_LetterboxingScanResult)
        {
            spdlog::info("MGS 2 | MGS 3: Letterboxing: Pattern scan failed.");
        }
    }
    */
}

void Miscellaneous()
{
    /*
    if (eGameType == MgsGame::MGS2 || eGameType == MgsGame::MGS3 || eGameType == MgsGame::MG)
    {
        if (bDisableCursor)
        {
            // MGS 2 | MGS 3: Disable mouse cursor
            // Thanks again emoose!
            uint8_t* MGS2_MGS3_MouseCursorScanResult = Memory::PatternScan(baseModule, "?? ?? BA ?? ?? 00 00 FF ?? ?? ?? ?? ?? 48 ?? ??");
            if (MGS2_MGS3_MouseCursorScanResult && bWindowedMode)
            {
                DWORD64 MGS2_MGS3_MouseCursorAddress = (uintptr_t)MGS2_MGS3_MouseCursorScanResult;
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Mouse Cursor: Address is 0x%" PRIxPTR, (uintptr_t)MGS2_MGS3_MouseCursorAddress);

                Memory::PatchBytes(MGS2_MGS3_MouseCursorAddress, "\xEB", 1);
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Mouse Cursor: Patched instruction.");
            }
            else if (!MGS2_MGS3_MouseCursorScanResult)
            {
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Mouse Cursor: Pattern scan failed.");
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
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Disable Background Input: IsWindowedCheck at 0x%" PRIxPTR, (uintptr_t)MGS_WndProc_IsWindowedCheck);
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Disable Background Input: ShowWindowCall at 0x%" PRIxPTR, (uintptr_t)MGS_WndProc_ShowWindowCall);
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Disable Background Input: SetFullscreenEnd at 0x%" PRIxPTR, (uintptr_t)MGS_WndProc_SetFullscreenEnd);

                // Patch out the jnz after the windowed check
                Memory::PatchBytes((uintptr_t)(MGS_WndProc_IsWindowedCheck + 7), "\x90\x90\x90\x90\x90\x90", 6);

                // We included 2 more bytes in MGS_WndProc_ShowWindowCall sig to reduce matches, but we want to keep those
                MGS_WndProc_ShowWindowCall += 2;

                // Skip the ShowWindow & SetFullscreenState block by figuring out how many bytes to skip over
                uint8_t jmper[] = { 0xEB, 0x00 };
                jmper[1] = (uint8_t)((uintptr_t)MGS_WndProc_SetFullscreenEnd - (uintptr_t)(MGS_WndProc_ShowWindowCall + 2));

                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Disable Background Input: ShowWindowCall jmp skipping %x bytes", jmper[1]);
                Memory::PatchBytes((uintptr_t)MGS_WndProc_ShowWindowCall, (const char*)jmper, 2);
            }
            else
            {
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Disable Background Input: Pattern scan failed.");
            }
        }
    }
    */

   
    if (iAnisotropicFiltering > 0 && (eGameType == MgsGame::MGS3 || eGameType == MgsGame::MGS2))
    {
        /*
        uint8_t* MGS2_MGS3_SetSamplerStateInsnResult = Memory::PatternScan(baseModule, "48 8B 05 ?? ?? ?? ?? 44 39 8C 01 38 04 00 00");
        if (MGS2_MGS3_SetSamplerStateInsnResult)
        {
            DWORD64 MGS2_MGS3_SetSamplerStateInsnAddress = (uintptr_t)MGS2_MGS3_SetSamplerStateInsnResult;
            DWORD64 gpRenderBackendPtrAddr = MGS2_MGS3_SetSamplerStateInsnAddress + 3;

            gpRenderBackend = *(int*)gpRenderBackendPtrAddr + gpRenderBackendPtrAddr + 4;
            spdlog::info("MGS 2 | MGS 3: Anisotropic Filtering: gpRenderBackend = 0x%" PRIxPTR, (uintptr_t)gpRenderBackend);

            int MGS2_MGS3_SetSamplerStateInsnHookLength = Memory::GetHookLength((char*)MGS2_MGS3_SetSamplerStateInsnAddress, 14);

            spdlog::info("MGS 2 | MGS 3: Anisotropic Filtering: Hook length is {} bytes", MGS2_MGS3_SetSamplerStateInsnHookLength);
            spdlog::info("MGS 2 | MGS 3: Anisotropic Filtering: Hook address is 0x%" PRIxPTR, (uintptr_t)MGS2_MGS3_SetSamplerStateInsnAddress);

            MGS2_MGS3_SetSamplerStateAnisoReturnJMP = MGS2_MGS3_SetSamplerStateInsnAddress + MGS2_MGS3_SetSamplerStateInsnHookLength;
            Memory::DetourFunction64((void*)MGS2_MGS3_SetSamplerStateInsnAddress, MGS2_MGS3_SetSamplerStateAniso_CC, MGS2_MGS3_SetSamplerStateInsnHookLength);
        }
        else
        {
            spdlog::info("MGS 2 | MGS 3: Anisotropic Filtering: Sampler state pattern scan failed.");
        }
        */

        uint8_t* MGS3_SetSamplerStateInsnScanResult = Memory::PatternScan(baseModule, "48 8B ?? ?? ?? ?? ?? 44 39 ?? ?? 38 ?? ?? ?? 74 ?? 44 89 ?? ?? ?? ?? ?? ?? EB ?? 48 ?? ??");
        if (MGS3_SetSamplerStateInsnScanResult)
        {
            spdlog::info("MGS 2 | MGS 3: Anisotropic Filtering: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS3_SetSamplerStateInsnScanResult - (uintptr_t)baseModule);

            static SafetyHookMid SetSamplerStateInsnXMidHook{};
            SetSamplerStateInsnXMidHook = safetyhook::create_mid(MGS3_SetSamplerStateInsnScanResult + 0x7,
                [](SafetyHookContext& ctx)
                {
                    *reinterpret_cast<int*>(ctx.rcx + ctx.rax + 0x438 + 0x14) = iAnisotropicFiltering;
                    ctx.r9 = 0x55;
                });

        }
        else if (!MGS3_SetSamplerStateInsnScanResult)
        {
            spdlog::error("MGS 3: Mouse Sensitivity: Pattern scan failed.");
        }
    }
    

    if (eGameType == MgsGame::MGS3 && bMouseSensitivity)
    {
        // MG 1/2 | MGS 2 | MGS 3: MouseSensitivity
        uint8_t* MGS3_MouseSensitivityScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? F3 0F ?? ?? 66 0F ?? ?? ?? 0F ?? ?? 66 0F ?? ?? 8B ?? ??");
        if (MGS3_MouseSensitivityScanResult)
        {
            spdlog::info("MGS 3: Mouse Sensitivity: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS3_MouseSensitivityScanResult - (uintptr_t)baseModule);

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
   
    if (iTextureBufferSizeMB > 16 && (eGameType == MgsGame::MGS3 || eGameType == MgsGame::MG))
    {
        // MG/MG2 | MGS3: texture buffer size extension
        uint32_t NewSize = iTextureBufferSizeMB * 1024 * 1024;

        // Scan for the 9 mallocs which set buffer inside CTextureBuffer::sInstance
        bool failure = false;
        for (int i = 0; i < 9; i++)
        {
            uint8_t* MGS3_CTextureBufferMallocResult = Memory::PatternScan(baseModule, "75 ?? B9 00 00 00 01 FF");
            if (MGS3_CTextureBufferMallocResult)
            {
                uint32_t* bufferAmount = (uint32_t*)(MGS3_CTextureBufferMallocResult + 3);
                spdlog::info("MG/MG2 | MGS 3: Texture Buffer Size: #{} ({:s}+{:x}) old buffer size: {}", i, sExeName.c_str(), (uintptr_t)MGS3_CTextureBufferMallocResult - (uintptr_t)baseModule, (uintptr_t)*bufferAmount);
                Memory::Write((uintptr_t)bufferAmount, NewSize);
                spdlog::info("MG/MG2 | MGS 3: Texture Buffer Size: #{} ({:s}+{:x}) new buffer size: {}", i, sExeName.c_str(), (uintptr_t)MGS3_CTextureBufferMallocResult - (uintptr_t)baseModule, (uintptr_t)*bufferAmount);
            }
            else
            {
                spdlog::info("MG/MG2 | MGS 3: Texture Buffer Size: #{}: Pattern scan failed.", i);
                failure = true;
                break;
            }
        }

        if (!failure)
        {
            // CBaseTexture::Create seems to contain code that mallocs buffers based on 16MiB shifted by index of the mip being loaded
            // (ie: size = 16MiB >> mipIndex)
            // We'll make sure to increase the base 16MiB size it uses too
            uint8_t* MGS3_CBaseTextureMallocScanResult = Memory::PatternScan(baseModule, "75 ?? B8 00 00 00 01");
            if (MGS3_CBaseTextureMallocScanResult)
            {
                uint32_t* bufferAmount = (uint32_t*)(MGS3_CBaseTextureMallocScanResult + 3);
                spdlog::info("MG/MG2 | MGS 3: Texture Buffer Size: #{} ({:s}+{:x}) old buffer size: {}", 9, sExeName.c_str(), (uintptr_t)MGS3_CBaseTextureMallocScanResult - (uintptr_t)baseModule, (uintptr_t)*bufferAmount);
                Memory::Write((uintptr_t)bufferAmount, NewSize);
                spdlog::info("MG/MG2 | MGS 3: Texture Buffer Size: #{} ({:s}+{:x}) new buffer size: {}", 9, sExeName.c_str(), (uintptr_t)MGS3_CBaseTextureMallocScanResult - (uintptr_t)baseModule, (uintptr_t)*bufferAmount);
            }
            else
            {
                spdlog::info("MG/MG2 | MGS 3: Texture Buffer Size: #{}: Pattern scan failed.", 9);
            }
        }
    }
    
}

void ViewportFix()
{
    /*
    if (eGameType == MgsGame::MGS3)
    {
        MH_STATUS status;

        uint8_t* MGS3_RenderWaterSurfaceScanResult = Memory::PatternScan(baseModule, "0F 57 ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B ?? ?? ?? 48 89 ?? ?? ?? ?? ??");
        uintptr_t MGS3_RenderWaterSurfaceScanAddress = Memory::GetAbsolute((uintptr_t)MGS3_RenderWaterSurfaceScanResult + 0x10);
        if (MGS3_RenderWaterSurfaceScanResult && MGS3_RenderWaterSurfaceScanAddress)
        {
            status = Memory::HookFunction((uint8_t*)MGS3_RenderWaterSurfaceScanAddress, MGS3_RenderWaterSurface_Hook, (LPVOID*)&MGS3_RenderWaterSurface);

            if (status != MH_OK)
            {
                spdlog::info("MGS 3: Render Water Surface: Hook failed.");
            }
            else if (status == MH_OK)
            {
                spdlog::info("MGS 3: Render Water Surface: Hook successful. Hook address is 0x%" PRIxPTR, MGS3_RenderWaterSurfaceScanAddress);
            }
        }
        else
        {
            spdlog::info("MGS 3:  Render Water Surface: Pattern scan failed.");
        }

        uint8_t* MGS3_GetViewportCameraOffsetYScanResult = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? F3 44 ?? ?? ?? E8 ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? 00 00");
        uintptr_t MGS3_GetViewportCameraOffsetYScanAddress = Memory::GetAbsolute((uintptr_t)MGS3_GetViewportCameraOffsetYScanResult + 0xB);
        if (MGS3_GetViewportCameraOffsetYScanResult && MGS3_GetViewportCameraOffsetYScanAddress)
        {
            status = Memory::HookFunction((uint8_t*)MGS3_GetViewportCameraOffsetYScanAddress, MGS3_GetViewportCameraOffsetY_Hook, (LPVOID*)&MGS3_GetViewportCameraOffsetY);

            if (status != MH_OK)
            {
                spdlog::info("MGS 3: Get Viewport Camera Offset: Hook failed.");
            }
            else if (status == MH_OK)
            {
                spdlog::info("MGS 3: Get Viewport Camera Offset: Hook successful. Hook address is 0x%" PRIxPTR, MGS3_GetViewportCameraOffsetYScanAddress);
            }
        }
        else
        {
            spdlog::info("MGS 3: Get Viewport Camera Offset: Pattern scan failed.");
        }
    }
    */
}

using NHT_COsContext_SetControllerID_Fn = void (*)(int controllerType);
NHT_COsContext_SetControllerID_Fn NHT_COsContext_SetControllerID = nullptr;
void NHT_COsContext_SetControllerID_Hook(int controllerType)
{
    spdlog::info("NHT_COsContext_SetControllerID_Hook: controltype {} -> {}", controllerType, iLauncherConfigCtrlType);
    NHT_COsContext_SetControllerID(iLauncherConfigCtrlType);
}

using MGS3_COsContext_InitializeSKUandLang_Fn = void(__fastcall*)(void*, int, int);
MGS3_COsContext_InitializeSKUandLang_Fn MGS3_COsContext_InitializeSKUandLang = nullptr;
void __fastcall MGS3_COsContext_InitializeSKUandLang_Hook(void* thisptr, int lang, int sku)
{
    spdlog::info("MGS3_COsContext_InitializeSKUandLang: lang {} -> {}, sku {} -> {}", sku, iLauncherConfigRegion, lang, iLauncherConfigLanguage);
    MGS3_COsContext_InitializeSKUandLang(thisptr, iLauncherConfigLanguage, iLauncherConfigRegion);
}

using MGS2_COsContext_InitializeSKUandLang_Fn = void(__fastcall*)(void*, int);
MGS2_COsContext_InitializeSKUandLang_Fn MGS2_COsContext_InitializeSKUandLang = nullptr;
void __fastcall MGS2_COsContext_InitializeSKUandLang_Hook(void* thisptr, int lang)
{
    spdlog::info("MGS2_COsContext_InitializeSKUandLang: lang {} -> {}", lang, iLauncherConfigLanguage);
    MGS2_COsContext_InitializeSKUandLang(thisptr, iLauncherConfigLanguage);
}

void LauncherConfigOverride()
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
            spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Launcher Config: steam_appid.txt creation failed (exception: %s)", ex.what());
        }
    }

    // If SkipLauncher is enabled & we're running inside launcher process, we'll just start the game immediately and exit this launcher
    if (eGameType == MgsGame::Launcher)
    {
        if (bLauncherConfigSkipLauncher)
        {
            auto gameExePath = sExePath.parent_path() / game->ExeName;

            spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: SkipLauncher set, attempting game launch");

            PROCESS_INFORMATION processInfo = {};
            STARTUPINFO startupInfo = {};
            startupInfo.cb = sizeof(STARTUPINFO);

            std::wstring commandLine = L"\"" + gameExePath.wstring() + L"\"";


            if (game->ExeName == "METAL GEAR.exe")
            {
                // Add launch parameters for MG MSX
                auto transformString = [](const std::string& input, int (*transformation)(int)) -> std::wstring {
                    // Apply the transformation function to each character
                    std::string transformedString = input;
                    std::transform(transformedString.begin(), transformedString.end(), transformedString.begin(), transformation);

                    // Convert the transformed string to std::wstring
                    std::wstring wideString = Util::utf8_decode(transformedString);
                    return wideString;
                    };

                commandLine += L" -mgst " + transformString(sLauncherConfigMSXGame, ::tolower); // -mgst must be lowercase
                commandLine += L" -walltype " + std::to_wstring(iLauncherConfigMSXWallType);
                commandLine += L" -wallalign " + transformString(sLauncherConfigMSXWallAlign, ::toupper); // -wallalign must be uppercase
            }

            string sCommandLine(commandLine.begin(), commandLine.end());
            spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Launch command line: {}", sCommandLine.c_str());


            // Call CreateProcess to start the game process
            if (CreateProcess(nullptr, (LPWSTR)commandLine.c_str(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo))
            {
                // Successfully started the process
                CloseHandle(processInfo.hProcess);
                CloseHandle(processInfo.hThread);

                // Force launcher to exit
                ExitProcess(0);
            }
            else
            {
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: SkipLauncher failed to create game EXE process");
            }
        }
        return;
    }

    // Certain config such as language/button style is normally passed from launcher to game via arguments
    // When game EXE gets ran directly this config is left at default (english game, xbox buttons)
    // If launcher argument isn't detected we'll allow defaults to be changed by hooking the engine functions responsible for them
    HMODULE engineModule = GetModuleHandleA("Engine.dll");
    if (!engineModule)
    {
        spdlog::info("MG/MG2 | MGS 2 | MGS3: Launcher Config: Failed to get Engine.dll module handle");
        return;
    }

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
                spdlog::info("MG/MG2 | MGS 3: Launcher Config: Hooked COsContext::InitializeSKUandLang, overriding with Region/Language settings from INI");
            }
            else
            {
                spdlog::info("MG/MG2 | MGS 3: Launcher Config: Failed to apply COsContext::InitializeSKUandLang IAT hook");
            }
        }
        else
        {
            MGS2_COsContext_InitializeSKUandLang = decltype(MGS2_COsContext_InitializeSKUandLang)(GetProcAddress(engineModule, "?InitializeSKUandLang@COsContext@@QEAAXH@Z"));
            if (MGS2_COsContext_InitializeSKUandLang)
            {
                if (Memory::HookIAT(baseModule, "Engine.dll", MGS2_COsContext_InitializeSKUandLang, MGS2_COsContext_InitializeSKUandLang_Hook))
                {
                    spdlog::info("MGS 2: Launcher Config: Hooked COsContext::InitializeSKUandLang, overriding with Language setting from INI");
                }
                else
                {
                    spdlog::info("MGS 2: Launcher Config: Failed to apply COsContext::InitializeSKUandLang IAT hook");
                }
            }
            else
            {
                spdlog::info("MG/MG2 | MGS 2 | MGS3: Launcher Config: Failed to locate COsContext::InitializeSKUandLang export");
            }
        }
    }
    else
    {
        spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: -region/-lan specified on command-line, skipping INI override");
    }

    if (!hasCtrltype)
    {
        NHT_COsContext_SetControllerID = decltype(NHT_COsContext_SetControllerID)(GetProcAddress(engineModule, "NHT_COsContext_SetControllerID"));
        if (NHT_COsContext_SetControllerID)
        {
            if (Memory::HookIAT(baseModule, "Engine.dll", NHT_COsContext_SetControllerID, NHT_COsContext_SetControllerID_Hook))
            {
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Hooked NHT_COsContext_SetControllerID, overriding with CtrlType setting from INI");
            }
            else
            {
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Failed to apply NHT_COsContext_SetControllerID IAT hook");
            }
        }
        else
        {
            spdlog::info("MG/MG2 | MGS 2 | MGS3: Launcher Config: Failed to locate NHT_COsContext_SetControllerID export");
        }
    }
    else
    {
        spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: -ctrltype specified on command-line, skipping INI override");
    }
}

std::mutex mainThreadFinishedMutex;
std::condition_variable mainThreadFinishedVar;
bool mainThreadFinished = false;

DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    if (DetectGame())
    {
        LauncherConfigOverride();
        CustomResolution();
        IntroSkip();
        ScaleEffects();
        AspectFOVFix();
        HUDFix();
        Miscellaneous();
        ViewportFix();
    }

    // Signal any threads which might be waiting for us before continuing
    {
        std::lock_guard lock(mainThreadFinishedMutex);
        mainThreadFinished = true;
        mainThreadFinishedVar.notify_all();
    }

    return true;
}

std::mutex memsetHookMutex;
bool memsetHookCalled = false;
void* (__cdecl* memset_Fn)(void* Dst, int Val, size_t Size);
void* __cdecl memset_Hook(void* Dst, int Val, size_t Size)
{
    // memset is one of the first imports called by game (not the very first though, since ASI loader still has those hooked during our DllMain...)
    std::lock_guard lock(memsetHookMutex);
    if (!memsetHookCalled)
    {
        memsetHookCalled = true;

        // First we'll unhook the IAT for this function as early as we can
        Memory::HookIAT(baseModule, "VCRUNTIME140.dll", memset_Hook, memset_Fn);

        // Wait for our main thread to finish before we return to the game
        if (!mainThreadFinished)
        {
            std::unique_lock finishedLock(mainThreadFinishedMutex);
            mainThreadFinishedVar.wait(finishedLock, [] { return mainThreadFinished; });
        }
    }

    return memset_Fn(Dst, Val, Size);
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
        // Try hooking IAT of one of the imports game calls early on, so we can make it wait for our Main thread to complete before returning back to game
        // This will only hook the main game modules usage of memset, other modules calling it won't be affected
        HMODULE vcruntime140 = GetModuleHandleA("VCRUNTIME140.dll");
        if (vcruntime140)
        {
            memset_Fn = decltype(memset_Fn)(GetProcAddress(vcruntime140, "memset"));
            Memory::HookIAT(baseModule, "VCRUNTIME140.dll", memset_Fn, memset_Hook);
        }

        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);
        if (mainHandle)
        {
            SetThreadPriority(mainHandle, THREAD_PRIORITY_HIGHEST); // set our Main thread priority higher than the games thread
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

