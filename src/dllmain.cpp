#include "common.hpp"
#include <spdlog/spdlog.h>
#include <safetyhook.hpp>
#include <spdlog/sinks/base_sink.h>

#include "asi_loader_checks.hpp"
#include "reshade_compatibility_checks.hpp"
#include "d3d11_api.hpp"
#include "intro_skip.hpp"
#include "gamma_correction.hpp"
#include "wireframe.hpp"
#include "effect_speeds.hpp"
#include "gamevars.hpp"
#include "line_scaling.hpp"
#include "stereo_audio.hpp"


std::chrono::time_point<std::chrono::high_resolution_clock> initStartTime;
std::string lastLoaded;


HMODULE baseModule = GetModuleHandle(NULL);
HMODULE unityPlayer;

// Version
#define VERSION_STRING "2.5.0"
std::string sFixName = "MGSHDFix";
int iConfigVersion = 2; //increment this when making config changes, along with the number at the bottom of the config file
                        //that way we can sanity check to ensure people don't have broken/disabled features due to old config files.

// Logger
std::shared_ptr<spdlog::logger> logger;
std::filesystem::path sLogFile = sFixName + ".log";
std::filesystem::path sFixPath;
std::filesystem::path sExePath;
std::string sExeName;
std::string sGameVersion;

// Ini
inipp::Ini<char> ini;
std::filesystem::path sConfigFile = sFixName + ".ini";
std::pair DesktopDimensions = { 0,0 };

// Ini Variables
bool bVerboseLogging = true;
bool bAspectFix;
bool bHUDFix;
bool bFOVFix;
bool bOutputResolution;
int iOutputResX;
int iOutputResY;
int iInternalResX;
int iInternalResY;
bool bWindowedMode;
bool bBorderlessMode;
bool bFramebufferFix;
bool bLauncherJumpStart;
int iAnisotropicFiltering;
bool bDisableTextureFiltering;
int iTextureBufferSizeMB;
bool bMouseSensitivity;
float fMouseSensitivityXMulti;
float fMouseSensitivityYMulti;
bool bDisableCursor;
bool bOutdatedReshade;

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
float fDefaultHUDWidth = (float)1280;
float fDefaultHUDHeight = (float)720;
float fHUDWidthOffset;
float fHUDHeightOffset;
float fMGS2_EffectScaleX;
float fMGS2_EffectScaleY;
int iCurrentResX;
int iCurrentResY;

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


const std::map<MgsGame, GameInfo> kGames = {
    {MGS2, {"Metal Gear Solid 2 HD", "METAL GEAR SOLID2.exe", 2131640}},
    {MGS3, {"Metal Gear Solid 3 HD", "METAL GEAR SOLID3.exe", 2131650}},
    {MG, {"Metal Gear / Metal Gear 2 (MSX)", "METAL GEAR.exe", 2131680}},
};

const GameInfo* game = nullptr;
MgsGame eGameType =  UNKNOWN;
const LPCSTR sClassName = "CSD3DWND";


// CreateWindowExA Hook
SafetyHookInline CreateWindowExA_hook{};
HWND WINAPI CreateWindowExA_hooked(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    if (std::string(lpClassName) == std::string(sClassName))
    {
        if (bBorderlessMode && !(eGameType & UNKNOWN))
        {
            auto hWnd = CreateWindowExA_hook.stdcall<HWND>(dwExStyle, lpClassName, lpWindowName, WS_POPUP, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
            SetWindowPos(hWnd, HWND_TOP, 0, 0, DesktopDimensions.first, DesktopDimensions.second, NULL);
            spdlog::info("CreateWindowExA: Borderless: ClassName = {}, WindowName = {}, dwStyle = {:x}, X = {}, Y = {}, nWidth = {}, nHeight = {}", lpClassName, lpWindowName, WS_POPUP, X, Y, nWidth, nHeight);
            spdlog::info("CreateWindowExA: Borderless: SetWindowPos to X = {}, Y = {}, cx = {}, cy = {}", 0, 0, (int)DesktopDimensions.first, (int)DesktopDimensions.second);
            MainHwnd = hWnd;
            return hWnd;
        }

        if (bWindowedMode && !(eGameType & UNKNOWN))
        {
            auto hWnd = CreateWindowExA_hook.stdcall<HWND>(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
            SetWindowPos(hWnd, HWND_TOP, 0, 0, iOutputResX, iOutputResY, NULL);
            spdlog::info("CreateWindowExA: Windowed: ClassName = {}, WindowName = {}, dwStyle = {:x}, X = {}, Y = {}, nWidth = {}, nHeight = {}", lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight);
            spdlog::info("CreateWindowExA: Windowed: SetWindowPos to X = {}, Y = {}, cx = {}, cy = {}", 0, 0, iOutputResX, iOutputResY);
            MainHwnd = hWnd;
            return hWnd;
        }
    }

    MainHwnd = CreateWindowExA_hook.stdcall<HWND>(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    return MainHwnd;
}

// cipherxof's Water Surface Rendering Fix
bool MGS3_UseAdjustedOffsetY = true;
SafetyHookInline MGS3_RenderWaterSurface_hook{};
SafetyHookInline MGS3_GetViewportCameraOffsetY_hook{};

int64_t __fastcall MGS3_RenderWaterSurface(int64_t work)
{
    MGS3_UseAdjustedOffsetY = false;
    auto result = MGS3_RenderWaterSurface_hook.fastcall<int64_t>(work);
    MGS3_UseAdjustedOffsetY = true;
    return result;
}

float MGS3_GetViewportCameraOffsetY()
{
    return MGS3_UseAdjustedOffsetY ? MGS3_GetViewportCameraOffsetY_hook.stdcall<float>() : 0.00f;
}

// Spdlog sink (truncate on startup, single file)
template<typename Mutex>
class size_limited_sink : public spdlog::sinks::base_sink<Mutex> {
public:
    explicit size_limited_sink(const std::string& filename, size_t max_size)
        : _filename(filename), _max_size(max_size)
    {
        truncate_log_file();

        _file.open(_filename, std::ios::app);
        if (!_file.is_open()) 
        {
            throw spdlog::spdlog_ex("Failed to open log file " + filename);
        }
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (std::filesystem::exists(_filename) && std::filesystem::file_size(_filename) >= _max_size) 
        {
            return;
        }

        spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);

        _file.write(formatted.data(), formatted.size());
        _file.flush();
    }

    void flush_() override
    {
        _file.flush();
    }

private:
    std::ofstream _file;
    std::string _filename;
    size_t _max_size;

    void truncate_log_file()
    {
        if (std::filesystem::exists(_filename))
        {
            std::ofstream ofs(_filename, std::ofstream::out | std::ofstream::trunc);
            ofs.close();
        }
    }
};

void Init_CalculateScreenSize()
{
    iCurrentResX = iInternalResX;
    iCurrentResY = iInternalResY;

    // Calculate aspect ratio
    fAspectRatio = (float)iCurrentResX / (float)iCurrentResY;
    fAspectMultiplier = fAspectRatio / fNativeAspect;

    // HUD variables
    fHUDWidth = iCurrentResY * fNativeAspect;
    fHUDHeight = (float)iCurrentResY;
    fHUDWidthOffset = (float)(iCurrentResX - fHUDWidth) / 2;
    fHUDHeightOffset = 0;
    if (fAspectRatio < fNativeAspect) 
    {
        fHUDWidth = (float)iCurrentResX;
        fHUDHeight = (float)iCurrentResX / fNativeAspect;
        fHUDWidthOffset = 0;
        fHUDHeightOffset = (float)(iCurrentResY - fHUDHeight) / 2;
    }


    // Log details about current resolution
    spdlog::info("Current Resolution: Resolution: {}x{}", iCurrentResX, iCurrentResY);
    spdlog::info("Current Resolution: fAspectRatio: {}", fAspectRatio);
    spdlog::info("Current Resolution: fAspectMultiplier: {}", fAspectMultiplier);
    spdlog::info("Current Resolution: fHUDWidth: {}", fHUDWidth);
    spdlog::info("Current Resolution: fHUDHeight: {}", fHUDHeight);
    spdlog::info("Current Resolution: fHUDWidthOffset: {}", fHUDWidthOffset);
    spdlog::info("Current Resolution: fHUDHeightOffset: {}", fHUDHeightOffset);
}

void Init_Logging()
{
    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    sExePath = sExePath.remove_filename();

   // spdlog initialisation
    {
        try {
            bool logDirExists = std::filesystem::is_directory(sExePath / "logs");
            if (!logDirExists)
            { 
                std::filesystem::create_directory(sExePath / "logs"); //create a "logs" subdirectory in the game folder to keep the main directory tidy.
            }
            // Create 10MB truncated logger
            logger = std::make_shared<spdlog::logger>(sLogFile.string(), std::make_shared<size_limited_sink<std::mutex>>((sExePath / "logs" / sLogFile).string(), 10 * 1024 * 1024));
            spdlog::set_default_logger(logger);

            spdlog::flush_on(spdlog::level::debug);
            spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            spdlog::info("---------- Logging initialization started ----------");
            if (!logDirExists)
            {
                spdlog::info("New log subdirectory created.");
            }
            spdlog::info("{} v{} loaded.", sFixName, VERSION_STRING);
            spdlog::info("ASI plugin location: {}", (sExePath / sFixPath / (sFixName + ".asi")).string());
            spdlog::info("----------");
            spdlog::info("Log file: {}", (sExePath / "logs" / sLogFile).string());
            spdlog::info("----------");

            // Log module details
            spdlog::info("Module Name: {0:s}", sExeName.c_str());
            spdlog::info("Module Path: {0:s}", sExePath.string());
            spdlog::info("Module Address: 0x{0:x}", (uintptr_t)baseModule);
            spdlog::info("Module Version: {}", Memory::GetModuleVersion(baseModule));
        }
        catch (const spdlog::spdlog_ex& ex) 
        {
            AllocConsole();
            FILE* dummy;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            std::cout << "Log initialisation failed: " << ex.what() << std::endl;
            return FreeLibraryAndExitThread(baseModule, 1);
        }
    }
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - initStartTime).count(); \
    spdlog::info("---------- Logging loaded in: {} ms ----------", duration);
}



///Prints CPU, GPU, and RAM info to the log to expedite common troubleshooting.
void Init_LogSysInfo()
{
    #ifndef _WIN32
    spdlog::info("System Details - Steam Deck/Linux");
    return;
    #endif


    std::array<int, 4> integerBuffer = {};
    constexpr size_t sizeofIntegerBuffer = sizeof(int) * integerBuffer.size();
    std::array<char, 64> charBuffer = {};
    std::array<std::uint32_t, 3> functionIds = {
        0x8000'0002, // Manufacturer  
        0x8000'0003, // Model 
        0x8000'0004  // Clock-speed
    };

    std::string cpu;
    for (int id : functionIds)
    {
        __cpuid(integerBuffer.data(), id);
        std::memcpy(charBuffer.data(), integerBuffer.data(), sizeofIntegerBuffer);
        cpu += std::string(charBuffer.data());
    }

    spdlog::info("System Details - CPU: {}", cpu);

    std::string deviceString;
    for (int i = 0; ; i++)
    {
        DISPLAY_DEVICE dd = { sizeof(dd), 0 };
        BOOL f = EnumDisplayDevices(NULL, i, &dd, EDD_GET_DEVICE_INTERFACE_NAME);
        if (!f)
        {
            break; //that's all, folks.
        }
        char deviceStringBuffer[128];
        WideCharToMultiByte(CP_UTF8, 0, dd.DeviceString, -1, deviceStringBuffer, sizeof(deviceStringBuffer), NULL, NULL);
        if (deviceString == deviceStringBuffer) //each monitor reports what gpu is driving it, lets just double check in case we're looking at a laptop with mixed usage.
        {
            continue;
        }
        deviceString = deviceStringBuffer;
        spdlog::info("System Details - GPU: {}", deviceString);
    }

    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    double totalMemory = status.ullTotalPhys / 1024 / 1024;    ///Total physical RAM in MB.
    spdlog::info("System Details - RAM: {} GB ({} MB)", ceil((totalMemory / 1024) * 100) / 100, totalMemory);
}

void Init_ReadConfig()
{
    // Initialise config
    std::ifstream iniFile((sExePath / sFixPath / sConfigFile).string());
    if (!iniFile) 
    {
        AllocConsole();
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "" << sFixName << " v" << VERSION_STRING << " loaded." << std::endl;
        std::cout << "ERROR: Could not locate config file." << std::endl;
        std::cout << "ERROR: Make sure " << sConfigFile << " is located in " << sExePath / sFixPath << std::endl;
        return FreeLibraryAndExitThread(baseModule, 1);
    }
    else {
        spdlog::info("Config file: {}", (sExePath / sFixPath / sConfigFile).string());
        ini.parse(iniFile);
    }

    int loadedConfigVersion;
    inipp::get_value(ini.sections["Config Version"], "Version", loadedConfigVersion);
    if (loadedConfigVersion != iConfigVersion) 
    {
        AllocConsole();
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "" << sFixName << " v" << VERSION_STRING << " loaded." << std::endl;
        std::cout << "MGSHDFix CONFIG ERROR: Outdated config file!" << std::endl;
        std::cout << "MGSHDFix CONFIG ERROR: Please install -all- the files from the latest release!" << std::endl;
        return FreeLibraryAndExitThread(baseModule, 1);
    }

    // Grab desktop resolution
    DesktopDimensions = Util::GetPhysicalDesktopDimensions();

    ConfigParse_Fix_LineScaling();

    // Read ini file
    //inipp::get_value(ini.sections["Verbose Logging"], "Enabled", bVerboseLogging);
    inipp::get_value(ini.sections["Output Resolution"], "Enabled", bOutputResolution);
    inipp::get_value(ini.sections["Output Resolution"], "Width", iOutputResX);
    inipp::get_value(ini.sections["Output Resolution"], "Height", iOutputResY);
    inipp::get_value(ini.sections["Output Resolution"], "Windowed", bWindowedMode);
    inipp::get_value(ini.sections["Output Resolution"], "Borderless", bBorderlessMode);
    inipp::get_value(ini.sections["Internal Resolution"], "Width", iInternalResX);
    inipp::get_value(ini.sections["Internal Resolution"], "Height", iInternalResY);
    inipp::get_value(ini.sections["Anisotropic Filtering"], "Samples", iAnisotropicFiltering);
    inipp::get_value(ini.sections["Disable Texture Filtering"], "DisableTextureFiltering", bDisableTextureFiltering);
    inipp::get_value(ini.sections["Framebuffer Fix"], "Enabled", bFramebufferFix);
    inipp::get_value(ini.sections["Launcher Config"], "LauncherJumpStart", bLauncherJumpStart);
    inipp::get_value(ini.sections["Skip Intro Logos"], "Enabled", g_IntroSkip.isEnabled);
    inipp::get_value(ini.sections["Mouse Sensitivity"], "Enabled", bMouseSensitivity);
    inipp::get_value(ini.sections["Mouse Sensitivity"], "X Multiplier", fMouseSensitivityXMulti);
    inipp::get_value(ini.sections["Mouse Sensitivity"], "Y Multiplier", fMouseSensitivityYMulti);
    inipp::get_value(ini.sections["Disable Mouse Cursor"], "Enabled", bDisableCursor);
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
    spdlog::info("Config Parse: bVerboseLogging: {}", bVerboseLogging);
    spdlog::info("Config Parse: bOutputResolution: {}", bOutputResolution);
    if (iOutputResX == 0 || iOutputResY == 0) 
    {
        iOutputResX = DesktopDimensions.first;
        iOutputResY = DesktopDimensions.second;
    }
    spdlog::info("Config Parse: iOutputResX: {}", iOutputResX);
    spdlog::info("Config Parse: iOutputResY: {}", iOutputResY);
    spdlog::info("Config Parse: bWindowedMode: {}", bWindowedMode);
    spdlog::info("Config Parse: bBorderlessMode: {}", bBorderlessMode);
    if (iInternalResX == 0 || iInternalResY == 0) 
    {
        iInternalResX = iOutputResX;
        iInternalResY = iOutputResY;
    }
    spdlog::info("Config Parse: iInternalResX: {}", iInternalResX);
    spdlog::info("Config Parse: iInternalResY: {}", iInternalResY);
    spdlog::info("Config Parse: iAnisotropicFiltering: {}", iAnisotropicFiltering);
    if (iAnisotropicFiltering < 0 || iAnisotropicFiltering > 16)
    {
        iAnisotropicFiltering = std::clamp(iAnisotropicFiltering, 0, 16);
        spdlog::info("Config Parse: iAnisotropicFiltering value invalid, clamped to {}", iAnisotropicFiltering);
    }
    spdlog::info("Config Parse: bDisableTextureFiltering: {}", bDisableTextureFiltering);
    spdlog::info("Config Parse: bFramebufferFix: {}", bFramebufferFix);
    spdlog::info("Config Parse: bSkipIntroLogos: {}", g_IntroSkip.isEnabled);
    spdlog::info("Config Parse: bLauncherJumpStart: {}", bLauncherJumpStart);
    spdlog::info("Config Parse: bMouseSensitivity: {}", bMouseSensitivity);
    spdlog::info("Config Parse: fMouseSensitivityXMulti: {}", fMouseSensitivityXMulti);
    spdlog::info("Config Parse: fMouseSensitivityYMulti: {}", fMouseSensitivityYMulti);
    spdlog::info("Config Parse: bDisableCursor: {}", bDisableCursor);
    spdlog::info("Config Parse: iTextureBufferSizeMB: {}", iTextureBufferSizeMB);
    spdlog::info("Config Parse: bAspectFix: {}", bAspectFix);
    spdlog::info("Config Parse: bHUDFix: {}", bHUDFix);
    spdlog::info("Config Parse: bFOVFix: {}", bFOVFix);
    spdlog::info("Config Parse: bLauncherConfigSkipLauncher: {}", bLauncherConfigSkipLauncher);
    spdlog::info("Config Parse: iLauncherConfigCtrlType: {}", iLauncherConfigCtrlType);
    spdlog::info("Config Parse: iLauncherConfigRegion: {}", iLauncherConfigRegion);
    spdlog::info("Config Parse: iLauncherConfigLanguage: {}", iLauncherConfigLanguage);
}

bool DetectGame()
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
                game = &info;
                return true;
            }
        }

        spdlog::error("Failed to detect supported game, unknown launcher");
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

    spdlog::error("Failed to detect supported game, {} isn't supported by MGSHDFix", sExeName.c_str());
    return false;
}

void Init_FixDPIScaling()
{
    if (eGameType & (MG|MGS2|MGS3)) 
    {
        SetProcessDPIAware();
        spdlog::info("MG/MG2 | MGS 2 | MGS 3: High-DPI scaling fixed.");
    }
}

void Init_CustomResolution()
{
    if (eGameType & (MG|MGS2|MGS3) && bOutputResolution)
    {
        // MGS 2 | MGS 3: Custom Resolution
        uint8_t* MGS2_MGS3_InternalResolutionScanResult = Memory::PatternScanSilent(baseModule, "F2 0F ?? ?? ?? B9 05 00 00 00 E8 ?? ?? ?? ?? 85 ?? 75 ??");
        uint8_t* MGS2_MGS3_OutputResolution1ScanResult = Memory::PatternScanSilent(baseModule, "40 ?? ?? 74 ?? 8B ?? ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? EB ?? B9 06 00 00 00");
        uint8_t* MGS2_MGS3_OutputResolution2ScanResult = Memory::PatternScanSilent(baseModule, "80 ?? ?? 00 41 ?? ?? ?? ?? ?? 48 ?? ?? ?? BA ?? ?? ?? ?? 8B ??");
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
            spdlog::info("MG/MG2 | MGS 2 | MGS 3: Custom Resolution: Output 1: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_OutputResolution1ScanResult - (uintptr_t)baseModule);
            static SafetyHookMid OutputResolution1MidHook{};
            OutputResolution1MidHook = safetyhook::create_mid(MGS2_MGS3_OutputResolution1ScanResult,
                [](SafetyHookContext& ctx)
                {
                    ctx.rbx = iOutputResX;
                    ctx.rdi = iOutputResY;
                });

            // Output resolution 2
            spdlog::info("MG/MG2 | MGS 2 | MGS 3: Custom Resolution: Output 2: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_OutputResolution2ScanResult - (uintptr_t)baseModule);
            static SafetyHookMid OutputResolution2MidHook{};
            OutputResolution2MidHook = safetyhook::create_mid(MGS2_MGS3_OutputResolution2ScanResult,
                [](SafetyHookContext& ctx)
                {
                    ctx.r8 = iOutputResX;
                    ctx.r9 = iOutputResY;
                });

            // Internal resolution
            spdlog::info("MG/MG2 | MGS 2 | MGS 3: Custom Resolution: Internal: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_InternalResolutionScanResult - (uintptr_t)baseModule);
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
                    spdlog::warn("MGS 2 | MGS 3: Custom Resolution: Splashscreens {}: Incompatible game version. Skipping.");
                else 
                {
                    uint8_t* MGS2_MGS3_SplashscreenResult = Memory::PatternScanSilent(baseModule, "FF 15 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 4C 8D 44 24 ?? 48 8D 54 24 ?? 48 8B 08 48 8B 01 FF 50 ?? 48 8B 58");
                    if (!MGS2_MGS3_SplashscreenResult)
                    {
                        spdlog::error("MGS 2 | MGS 3: Custom Resolution: Splashscreens {}: Pattern scan failed.");
                    }
                    else
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
                        spdlog::info("MGS 2 | MGS 3: Custom Resolution: Splashscreens patched at {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_SplashscreenResult - (uintptr_t)baseModule);
                    }

                    uint8_t* MGS2_MGS3_LoadingScreenEngScanResult = Memory::PatternScanSilent(baseModule, "48 8D 8C 24 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 8C 24 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 8C 24 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 8C 24"); //    /loading.ctxr 
                    if (!MGS2_MGS3_LoadingScreenEngScanResult)
                    {
                        spdlog::error("MGS 2 | MGS 3: Custom Resolution: Loading Screen (ENG) {}: Pattern scan failed.");
                    }
                    else
                    {
                        static SafetyHookMid MGS2_MGS3_LoadingScreenEngMidHook{};
                        MGS2_MGS3_LoadingScreenEngMidHook = safetyhook::create_mid(MGS2_MGS3_LoadingScreenEngScanResult,
                            [](SafetyHookContext& ctx)
                            {
                                ctx.rdx = iOutputResY >= 2160 ? reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_4k.ctxr") :
                                          iOutputResY >= 1440 ? reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_wqhd.ctxr") :
                                        /*iOutputResY >= 1080*/ reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_fhd.ctxr");
                            });
                        spdlog::info("MGS 2 | MGS 3: Custom Resolution: Loading Screen (ENG) patched at {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_LoadingScreenEngScanResult - (uintptr_t)baseModule);
                    }

                    uint8_t* MGS2_MGS3_LoadingScreenJPScanResult = Memory::PatternScanSilent(baseModule, "48 8D 4C 24 ?? FF 15 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 4C 24"); //    /loading_jp.ctxr 
                    if (!MGS2_MGS3_LoadingScreenJPScanResult)
                    {
                        spdlog::error("MGS 2 | MGS 3: Custom Resolution: Loading Screen (JPN) {}: Pattern scan failed.");
                    }
                    else
                    {
                        static SafetyHookMid MGS2_MGS3_LoadingScreenJPMidHook{};
                        MGS2_MGS3_LoadingScreenJPMidHook = safetyhook::create_mid(MGS2_MGS3_LoadingScreenJPScanResult,
                            [](SafetyHookContext& ctx)
                            {
                                ctx.rdx = iOutputResY >= 2160 ? reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_jp_4k.ctxr") :
                                          iOutputResY >= 1440 ? reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_jp_wqhd.ctxr") :
                                        /*iOutputResY >= 1080*/ reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_jp_fhd.ctxr");
                            });
                        spdlog::info("MGS 2 | MGS 3: Custom Resolution: Loading Screen (JP) patched at {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_LoadingScreenJPScanResult - (uintptr_t)baseModule);
                    }
                }

            }
        }
        else if (!MGS2_MGS3_InternalResolutionScanResult || !MGS2_MGS3_OutputResolution1ScanResult || !MGS2_MGS3_OutputResolution2ScanResult)
        {
            spdlog::error("MG/MG2 | MGS 2 | MGS 3: Custom Resolution: Pattern scan failed.");
        }

        // MG 1/2 | MGS 2 | MGS 3: WindowedMode
        uint8_t* MGS2_MGS3_WindowedModeScanResult = Memory::PatternScanSilent(baseModule, "48 ?? ?? E8 ?? ?? ?? ?? 84 ?? 0F 84 ?? ?? ?? ?? 48 ?? ?? ?? ?? ?? ?? 41 ?? 03 00 00 00");
        if (MGS2_MGS3_WindowedModeScanResult)
        {
            spdlog::info("MG/MG2 | MGS 2 | MGS 3: WindowedMode: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_WindowedModeScanResult - (uintptr_t)baseModule);
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
        else if (!MGS2_MGS3_WindowedModeScanResult)
        {
            spdlog::error("MG/MG2 | MGS 2 | MGS 3: WindowedMode: Pattern scan failed.");
        }

        // MG 1/2 | MGS 2 | MGS 3: CreateWindowExA
        CreateWindowExA_hook = safetyhook::create_inline(CreateWindowExA, reinterpret_cast<void*>(CreateWindowExA_hooked));
        spdlog::info("MG/MG2 | MGS 2 | MGS 3: CreateWindowExA: Hooked function.");

        // MG 1/2 | MGS 2 | MGS 3: SetWindowPos
        uint8_t* MGS2_MGS3_SetWindowPosScanResult = Memory::PatternScanSilent(baseModule, "33 ?? 48 ?? ?? ?? FF ?? ?? ?? ?? ?? 8B ?? ?? BA 02 00 00 00");
        if (MGS2_MGS3_SetWindowPosScanResult)
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

            spdlog::info("MG/MG2 | MGS 2 | MGS 3: SetWindowPos: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_SetWindowPosScanResult - (uintptr_t)baseModule);
        }
        else if (!MGS2_MGS3_SetWindowPosScanResult)
        {
            spdlog::error("MG/MG2 | MGS 2 | MGS 3: SetWindowPos: Pattern scan failed.");
        }

        // MGS 2 | MGS 3: Framebuffer fix, stops the framebuffer from being set to maximum display resolution.
        // Thanks emoose!
        if (bFramebufferFix)
        {
            // Need to stop hor + vert from being modified.
            for (int i = 1; i <= 2; ++i)
            {
                // Fullscreen framebuffer
                uint8_t* MGS2_MGS3_FullscreenFramebufferFixScanResult = Memory::PatternScanSilent(baseModule, "03 ?? 41 ?? ?? ?? C7 ?? ?? ?? ?? ?? ?? 00 00 00");
                if (MGS2_MGS3_FullscreenFramebufferFixScanResult)
                {
                    spdlog::info("MG/MG2 | MGS 2 | MGS 3: Fullscreen Framebuffer {}: Address is {:s}+{:x}", i, sExeName.c_str(), (uintptr_t)MGS2_MGS3_FullscreenFramebufferFixScanResult - (uintptr_t)baseModule);
                    Memory::PatchBytes((uintptr_t)MGS2_MGS3_FullscreenFramebufferFixScanResult + 0x2, "\x90\x90\x90\x90", 4);
                    spdlog::info("MG/MG2 | MGS 2 | MGS 3: Fullscreen Framebuffer {}: Patched instruction.", i);
                }
                else if (!MGS2_MGS3_FullscreenFramebufferFixScanResult)
                {
                    spdlog::error("MG/MG2 | MGS 2 | MGS 3: Fullscreen Framebuffer {}: Pattern scan failed.", i);
                }
            }

            // Windowed framebuffer
            uint8_t* MGS2_MGS3_WindowedFramebufferFixScanResult = Memory::PatternScanSilent(baseModule, "?? ?? F3 0F ?? ?? 41 ?? ?? F3 0F ?? ?? F3 0F ?? ?? 66 0F ?? ?? 0F ?? ??");
            if (MGS2_MGS3_WindowedFramebufferFixScanResult)
            {
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Windowed Framebuffer: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_WindowedFramebufferFixScanResult - (uintptr_t)baseModule);
                Memory::PatchBytes((uintptr_t)MGS2_MGS3_WindowedFramebufferFixScanResult, "\xEB", 1);
                if (eGameType & MG|MGS3)
                    Memory::PatchBytes((uintptr_t)MGS2_MGS3_WindowedFramebufferFixScanResult + 0x2A, "\xEB", 1);
                if (eGameType & MGS2)
                    Memory::PatchBytes((uintptr_t)MGS2_MGS3_WindowedFramebufferFixScanResult + 0x27, "\xEB", 1);
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Windowed Framebuffer: Patched instructions.");
            }
            else if (!MGS2_MGS3_WindowedFramebufferFixScanResult)
            {
                spdlog::error("MG/MG2 | MGS 2 | MGS 3: Windowed Framebuffer: Pattern scan failed.");
            }
        }
    }
    
}



void Init_ScaleEffects()
{
    if ((eGameType & (MG|MGS2|MGS3)) && bOutputResolution)
    {
        // MGS 2 | MGS 3: Fix scaling for added volume menu in v1.4.0 patch
        uint8_t* MGS2_MGS3_VolumeMenuScanResult = Memory::PatternScanSilent(baseModule, "F3 0F ?? ?? 48 ?? ?? ?? 89 ?? ?? ?? 00 00 F3 0F ?? ?? 89 ?? ?? ?? 00 00");
        if (MGS2_MGS3_VolumeMenuScanResult)
        {
            spdlog::info("MGS 2 | MGS 3: Volume Menu: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_VolumeMenuScanResult - (uintptr_t)baseModule);

            static SafetyHookMid MGS2_MGS3_VolumeMenuMidHook{};
            MGS2_MGS3_VolumeMenuMidHook = safetyhook::create_mid(MGS2_MGS3_VolumeMenuScanResult,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm2.f32[0] = (float)1280;
                    ctx.xmm3.f32[0] = (float)720;
                });
        }
        else if (!MGS2_MGS3_VolumeMenuScanResult)
        {
            spdlog::error("MGS 2 | MGS 3: Volume Menu: Pattern scan failed.");
        }
    }

    if (eGameType & MGS2 && bOutputResolution)
    {
        // MGS 2: Scale Effects
        uint8_t* MGS2_ScaleEffectsScanResult = Memory::PatternScanSilent(baseModule, "48 8B ?? ?? 66 ?? ?? ?? 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? ?? ?? ??");
        if (MGS2_ScaleEffectsScanResult)
        {
            spdlog::info("MGS 2: Scale Effects: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_ScaleEffectsScanResult - (uintptr_t)baseModule);

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
        else if (!MGS2_ScaleEffectsScanResult)
        {
            spdlog::error("MGS 2: Scale Effects: Pattern scan failed.");
        }
    }
    
}




void Init_AspectFOVFix()
{
    // Fix aspect ratio
    if (eGameType & MGS3 && bAspectFix)
    {
        // MGS 3: Fix gameplay aspect ratio
        uint8_t* MGS3_GameplayAspectScanResult = Memory::PatternScanSilent(baseModule, "F3 0F ?? ?? E8 ?? ?? ?? ?? 48 8D ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ??");
        if (MGS3_GameplayAspectScanResult)
        {
            spdlog::info("MGS 3: Aspect Ratio: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS3_GameplayAspectScanResult - (uintptr_t)baseModule);
            DWORD64 MGS3_GameplayAspectAddress = Memory::GetAbsolute((uintptr_t)MGS3_GameplayAspectScanResult + 0x5);
            spdlog::info("MGS 3: Aspect Ratio: Function address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS3_GameplayAspectAddress - (uintptr_t)baseModule);

            static SafetyHookMid MGS3_GameplayAspectMidHook{};
            MGS3_GameplayAspectMidHook = safetyhook::create_mid(MGS3_GameplayAspectAddress + 0x38,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm1.f32[0] /= fAspectMultiplier;
                });
        }
        else if (!MGS3_GameplayAspectScanResult)
        {
            spdlog::error("MG/MG2 | MGS 3: Aspect Ratio: Pattern scan failed.");
        }
    }
    else if (eGameType & MGS2 && bAspectFix)
    {
        // MGS 2: Fix gameplay aspect ratio
        uint8_t* MGS2_GameplayAspectScanResult = Memory::PatternScanSilent(baseModule, "48 8D ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? ??");
        if (MGS2_GameplayAspectScanResult)
        {
            spdlog::info("MGS 2: Aspect Ratio: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_GameplayAspectScanResult - (uintptr_t)baseModule);
            DWORD64 MGS2_GameplayAspectAddress = Memory::GetAbsolute((uintptr_t)MGS2_GameplayAspectScanResult + 0xB);
            spdlog::info("MGS 2: Aspect Ratio: Function address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_GameplayAspectAddress - (uintptr_t)baseModule);

            static SafetyHookMid MGS2_GameplayAspectMidHook{};
            MGS2_GameplayAspectMidHook = safetyhook::create_mid(MGS2_GameplayAspectAddress + 0x38,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm0.f32[0] /= fAspectMultiplier;
                });
        }
        else if (!MGS2_GameplayAspectScanResult)
        {
            spdlog::error("MGS 2: Aspect Ratio: Pattern scan failed.");
        }
    }

    // Convert FOV to vert- to match 16:9 horizontal field of view
    if (eGameType & MGS3 && bFOVFix)
    {
        // MGS 3: FOV
        uint8_t* MGS3_FOVScanResult = Memory::PatternScanSilent(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? 44 ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 ?? ?? ?? ?? E8 ?? ?? ?? ??");
        if (MGS3_FOVScanResult)
        {
            spdlog::info("MGS 3: FOV: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS3_FOVScanResult - (uintptr_t)baseModule);

            static SafetyHookMid MGS3_FOVMidHook{};
            MGS3_FOVMidHook = safetyhook::create_mid(MGS3_FOVScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio < fNativeAspect)
                    {
                        ctx.xmm2.f32[0] *= fAspectMultiplier;
                    }
                });
        }
        else if (!MGS3_FOVScanResult)
        {
            spdlog::error("MGS 3: FOV: Pattern scan failed.");
        }
    }
    else if (eGameType & MGS2 && bFOVFix)
    {
        // MGS 2: FOV
        uint8_t* MGS2_FOVScanResult = Memory::PatternScanSilent(baseModule, "44 ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 44 ?? ?? ?? ?? 48 ?? ?? 48 ?? ?? ?? ?? 00 00");
        if (MGS2_FOVScanResult)
        {
            spdlog::info("MGS 2: FOV: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_FOVScanResult - (uintptr_t)baseModule);

            static SafetyHookMid MGS2_FOVMidHook{};
            MGS2_FOVMidHook = safetyhook::create_mid(MGS2_FOVScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio < fNativeAspect)
                    {
                        ctx.xmm2.f32[0] *= fAspectMultiplier;
                    }
                });
        }
        else if (!MGS2_FOVScanResult)
        {
            spdlog::error("MGS 2: FOV: Pattern scan failed.");
        }
    }
    
}

void Init_HUDFix()
{
    if (eGameType & MGS2 && bHUDFix)
    {
        // MGS 2: HUD
        uint8_t* MGS2_HUDWidthScanResult = Memory::PatternScanSilent(baseModule, "E9 ?? ?? ?? ?? F3 0F ?? ?? ?? 0F ?? ?? F3 0F ?? ?? ?? F3 0F ?? ??");
        if (MGS2_HUDWidthScanResult)
        {
            spdlog::info("MGS 2: HUD: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_HUDWidthScanResult - (uintptr_t)baseModule);

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
            spdlog::info("MGS 2: Radar Width: Hook address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_RadarWidthAddress - (uintptr_t)baseModule);

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
            spdlog::info("MGS 2: Radar Width Offset: Hook address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_RadarWidthOffsetAddress - (uintptr_t)baseModule);

            static SafetyHookMid MGS2_RadarWidthOffsetMidHook{};
            MGS2_RadarWidthOffsetMidHook = safetyhook::create_mid(MGS2_RadarWidthOffsetAddress,
                [](SafetyHookContext& ctx)
                {
                    ctx.rax += (int)fHUDWidthOffset;
                });

            // Radar height offset
            DWORD64 MGS2_RadarHeightOffsetAddress = (uintptr_t)MGS2_RadarWidthScanResult + 0x90;
            spdlog::info("MGS 2: Radar Height Offset: Hook address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_RadarHeightOffsetAddress - (uintptr_t)baseModule);

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
            spdlog::info("MGS 2: Codec Portraits: Hook address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_CodecPortraitsScanResult - (uintptr_t)baseModule);

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
            spdlog::info("MGS 2: Motion Blur: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MotionBlurScanResult - (uintptr_t)baseModule);

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

            spdlog::info("MG1/2 | MGS 3: HUD Width: Hook address is{:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS3_HUDWidthScanResult - (uintptr_t)baseModule);
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
            spdlog::info("MGS 2 | MGS 3: Letterboxing: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_LetterboxingAddress - (uintptr_t)baseModule);

            Memory::Write(MGS2_MGS3_LetterboxingAddress, (int)0);
            spdlog::info("MGS 2 | MGS 3: Letterboxing: Disabled letterboxing.");
        }
        else if (!MGS2_MGS3_LetterboxingScanResult)
        {
            spdlog::error("MGS 2 | MGS 3: Letterboxing: Pattern scan failed.");
        }
    }
    
}

void Init_Miscellaneous()
{
    if (eGameType & (MG|MGS2|MGS3|LAUNCHER))
    {
        if (bDisableCursor)
        {
            // Launcher | MG/MG2 | MGS 2 | MGS 3: Disable mouse cursor
            // Thanks again emoose!
            uint8_t* MGS2_MGS3_MouseCursorScanResult = Memory::PatternScanSilent(baseModule, "BA 00 7F 00 00 33 ?? FF ?? ?? ?? ?? ?? 48 ?? ??");
            if (eGameType & LAUNCHER)
            {
                unityPlayer = GetModuleHandleA("UnityPlayer.dll");
                MGS2_MGS3_MouseCursorScanResult = Memory::PatternScanSilent(unityPlayer, "BA 00 7F 00 00 33 ?? FF ?? ?? ?? ?? ?? 48 ?? ??");
            }

            if (MGS2_MGS3_MouseCursorScanResult)
            {
                if (eGameType & LAUNCHER)
                {
                    spdlog::info("Launcher | MG/MG2 | MGS 2 | MGS 3: Mouse Cursor: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_MouseCursorScanResult - (uintptr_t)unityPlayer);
                }
                else
                {
                    spdlog::info("Launcher | MG/MG2 | MGS 2 | MGS 3: Mouse Cursor: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS2_MGS3_MouseCursorScanResult - (uintptr_t)baseModule);
                }
                // The game enters 32512 in the RDX register for the function USER32.LoadCursorA to load IDC_ARROW (normal select arrow in windows)
                // Set this to 0 and no cursor icon is loaded
                Memory::PatchBytes((uintptr_t)MGS2_MGS3_MouseCursorScanResult + 0x2, "\x00", 1);
                spdlog::info("Launcher | MG/MG2 | MGS 2 | MGS 3: Mouse Cursor: Patched instruction.");
            }
            else if (!MGS2_MGS3_MouseCursorScanResult)
            {
                spdlog::error("Launcher | MG/MG2 | MGS 2 | MGS 3: Mouse Cursor: Pattern scan failed.");
            }
        }
    }

    if ((bDisableTextureFiltering || iAnisotropicFiltering > 0) && (eGameType & (MG|MGS2|MGS3)))
    {
        uint8_t* MGS3_SetSamplerStateInsnScanResult = Memory::PatternScanSilent(baseModule, "48 8B ?? ?? ?? ?? ?? 44 39 ?? ?? 38 ?? ?? ?? 74 ?? 44 89 ?? ?? ?? ?? ?? ?? EB ?? 48 ?? ??");
        if (MGS3_SetSamplerStateInsnScanResult)
        {
            spdlog::info("MGS 2 | MGS 3: Texture Filtering: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MGS3_SetSamplerStateInsnScanResult - (uintptr_t)baseModule);

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

        }
        else if (!MGS3_SetSamplerStateInsnScanResult)
        {
            spdlog::error("MGS 2 | MGS 3: Texture Filtering: Pattern scan failed.");
        }
    }

    if (eGameType & MGS3 && bMouseSensitivity)
    {
        // MG 1/2 | MGS 2 | MGS 3: MouseSensitivity
        uint8_t* MGS3_MouseSensitivityScanResult = Memory::PatternScanSilent(baseModule, "F3 0F ?? ?? ?? F3 0F ?? ?? 66 0F ?? ?? ?? 0F ?? ?? 66 0F ?? ?? 8B ?? ??");
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

    if (iTextureBufferSizeMB > 128 && (eGameType & (MG|MGS3)))
    {
        // MG/MG2 | MGS3: texture buffer size extension
        uint32_t NewSize = iTextureBufferSizeMB * 1024 * 1024;

        // Scan for the 9 mallocs which set buffer inside CTextureBuffer::sInstance
        bool failure = false;
        for (int i = 0; i < 9; i++)
        {
            uint8_t* MGS3_CTextureBufferMallocResult = Memory::PatternScanSilent(baseModule, "75 ?? B9 00 00 00 08 FF");
            if (MGS3_CTextureBufferMallocResult)
            {
                uint32_t* bufferAmount = (uint32_t*)(MGS3_CTextureBufferMallocResult + 3);
                spdlog::info("MG/MG2 | MGS 3: Texture Buffer Size: #{} ({:s}+{:x}) old buffer size: {}", i, sExeName.c_str(), (uintptr_t)MGS3_CTextureBufferMallocResult - (uintptr_t)baseModule, (uintptr_t)*bufferAmount);
                Memory::Write((uintptr_t)bufferAmount, NewSize);
                spdlog::info("MG/MG2 | MGS 3: Texture Buffer Size: #{} ({:s}+{:x}) new buffer size: {}", i, sExeName.c_str(), (uintptr_t)MGS3_CTextureBufferMallocResult - (uintptr_t)baseModule, (uintptr_t)*bufferAmount);
            }
            else
            {
                spdlog::error("MG/MG2 | MGS 3: Texture Buffer Size: #{}: Pattern scan failed.", i);
                failure = true;
                break;
            }
        }

        if (!failure)
        {
            // CBaseTexture::Create seems to contain code that mallocs buffers based on 16MiB shifted by index of the mip being loaded
            // (ie: size = 16MiB >> mipIndex)
            // We'll make sure to increase the base 16MiB size it uses too
            uint8_t* MGS3_CBaseTextureMallocScanResult = Memory::PatternScanSilent(baseModule, "75 ?? 00 00 00 08 8B ??");
            if (MGS3_CBaseTextureMallocScanResult)
            {
                uint32_t* bufferAmount = (uint32_t*)(MGS3_CBaseTextureMallocScanResult + 3);
                spdlog::info("MG/MG2 | MGS 3: Texture Buffer Size: #{} ({:s}+{:x}) old buffer size: {}", 9, sExeName.c_str(), (uintptr_t)MGS3_CBaseTextureMallocScanResult - (uintptr_t)baseModule, (uintptr_t)*bufferAmount);
                Memory::Write((uintptr_t)bufferAmount, NewSize);
                spdlog::info("MG/MG2 | MGS 3: Texture Buffer Size: #{} ({:s}+{:x}) new buffer size: {}", 9, sExeName.c_str(), (uintptr_t)MGS3_CBaseTextureMallocScanResult - (uintptr_t)baseModule, (uintptr_t)*bufferAmount);
            }
            else
            {
                spdlog::error("MG/MG2 | MGS 3: Texture Buffer Size: #{}: Pattern scan failed.", 9);
            }
        }
    }
}

void Init_ViewportFix()
{
    if (eGameType & MGS3)
    {
        uint8_t* MGS3_RenderWaterSurfaceScanResult = Memory::PatternScanSilent(baseModule, "0F 57 ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B ?? ?? ?? 48 89 ?? ?? ?? ?? ??");
        uintptr_t MGS3_RenderWaterSurfaceScanAddress = Memory::GetAbsolute((uintptr_t)MGS3_RenderWaterSurfaceScanResult + 0x10);
        if (MGS3_RenderWaterSurfaceScanResult && MGS3_RenderWaterSurfaceScanAddress)
        {
            MGS3_RenderWaterSurface_hook = safetyhook::create_inline(reinterpret_cast<void*>(MGS3_RenderWaterSurfaceScanAddress), reinterpret_cast<void*>(MGS3_RenderWaterSurface));
            if (!MGS3_RenderWaterSurface_hook)
            {
                spdlog::info("MGS 3: Render Water Surface: Hook failed.");
                return;
            }
            spdlog::info("MGS 3: Render Water Surface: Hook successful. Address is {:s}+{:x}", sExeName.c_str(), MGS3_RenderWaterSurfaceScanAddress - (uintptr_t)baseModule);
        }
        else
        {
            spdlog::error("MGS 3:  Render Water Surface: Pattern scan failed.");
        }

        uint8_t* MGS3_GetViewportCameraOffsetYScanResult = Memory::PatternScanSilent(baseModule, "E8 ?? ?? ?? ?? F3 44 ?? ?? ?? E8 ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? 00 00");
        uintptr_t MGS3_GetViewportCameraOffsetYScanAddress = Memory::GetAbsolute((uintptr_t)MGS3_GetViewportCameraOffsetYScanResult + 0xB);
        if (MGS3_GetViewportCameraOffsetYScanResult && MGS3_GetViewportCameraOffsetYScanAddress)
        {
            MGS3_GetViewportCameraOffsetY_hook = safetyhook::create_inline(reinterpret_cast<void*>(MGS3_GetViewportCameraOffsetYScanAddress), reinterpret_cast<void*>(MGS3_GetViewportCameraOffsetY));
            if (!MGS3_GetViewportCameraOffsetY_hook)
            {
                spdlog::info("MGS 3: Get Viewport Camera Offset: Hook failed.");
                return;
            }
            spdlog::info("MGS 3: Get Viewport Camera Offset: Hook successful. Address is {:s}+{:x}", sExeName.c_str(), MGS3_GetViewportCameraOffsetYScanAddress - (uintptr_t)baseModule);
        }
        else
        {
            spdlog::error("MGS 3: Get Viewport Camera Offset: Pattern scan failed.");
        }
    }
    
}



// cipherxof's Skybox Rendering Fix
void Init_SkyboxFix()
{
    if (!(eGameType & MGS2))
    {
        return;
    }
    if (uintptr_t MGS2_CreateSkyUtilScanResult = (uintptr_t)Memory::PatternScan(baseModule, "81 4F ?? ?? 30 00 00 4D 85 FF", "MGS 2: Skybox", NULL, NULL))
    {
        Memory::PatchBytes(MGS2_CreateSkyUtilScanResult, "\x90\x90\x90\x90\x90\x90\x90", 7);
        spdlog::info("MGS 2: Skybox: Patch successful.");
    }
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

void Init_LauncherConfigOverride()
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

            std::string sCommandLine(commandLine.begin(), commandLine.end());
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
    HMODULE engineModule = GetModuleHandleA("Engine.dll");
    if (!engineModule)
    {
        spdlog::error("MG/MG2 | MGS 2 | MGS3: Launcher Config: Failed to get Engine.dll module handle");
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
                    spdlog::info("MGS 2: Launcher Config: Hooked COsContext::InitializeSKUandLang, overriding with Language setting from INI");
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
                spdlog::error("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Failed to apply NHT_COsContext_SetControllerID IAT hook");
            }
        }
        else
        {
            spdlog::error("MG/MG2 | MGS 2 | MGS3: Launcher Config: Failed to locate NHT_COsContext_SetControllerID export");
        }
    }
    else
    {
        spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: -ctrltype specified on command-line, skipping INI override");
    }

}



void preCreateDXGIFactory()
{

    
}

void afterCreateDXGIFactory()
{

}

void preD3D11CreateDevice()
{

}

void afterD3D11CreateDevice()
{
    MGS23_VectorLine_InjectShader();
    //createGammaShader();

    //SetGamma(1.0);
}


#define INITIALIZE(func) \
    do { \
        std::chrono::time_point<std::chrono::high_resolution_clock> currentInitPhaseStartTime;\
        if(strcmp(#func,"InitializeSubsystems()") == 0) \
        {\
            spdlog::info("---------- Subsystem initialization started ----------", #func); \
            currentInitPhaseStartTime = initStartTime;\
        }\
        else if(!lastLoaded.empty())\
        {\
            spdlog::info("---------- {}\tNow loading: {} ----------", lastLoaded, #func); \
            currentInitPhaseStartTime = std::chrono::high_resolution_clock::now();\
        }\
        else\
        {\
            spdlog::info("---------- Loading: {} ----------", #func); \
            currentInitPhaseStartTime = std::chrono::high_resolution_clock::now();\
        }\
        (func); \
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - currentInitPhaseStartTime).count();\
        if(strcmp(#func,"InitializeSubsystems()") == 0) \
        {\
            if(!lastLoaded.empty())\
            {\
                spdlog::info("---------- {} ----------", lastLoaded); \
            }\
            spdlog::info("---------- All systems completed loading in: {} ms. ----------", duration); \
        }\
        else\
        {\
            lastLoaded = std::string(#func) + " loaded in: " + std::to_string(duration) + " ms."; \
        }\
    } while (0)

std::mutex mainThreadFinishedMutex;
std::condition_variable mainThreadFinishedVar;
bool mainThreadFinished = false;

void InitializeSubsystems()
{
    INITIALIZE(Init_LogSysInfo());
    INITIALIZE(Init_ASILoaderSanityChecks());
    if (DetectGame())
    {
        //Initialization order (these systems initialize vars used by following ones.)
        INITIALIZE(g_GameVars.Initialize());           //1
        INITIALIZE(Init_D3D11Hooks());                 //2 Caches the D3DDevice, DXGIFactory, and D3DContext from D3DCreateDevice/DXGICreateFactory
        INITIALIZE(Init_ReadConfig());                 //3
        INITIALIZE(Init_ReshadeCompatibilityChecks()); //4 Dependent on ReadConfig, must also be before LauncherConfigOverride
        INITIALIZE(Init_CalculateScreenSize());        //4
        INITIALIZE(Init_LauncherConfigOverride());     //5
        INITIALIZE(Init_FixDPIScaling());              //6 Needs to be anywhere before the window is created in CustomResolution.
        INITIALIZE(Init_CustomResolution());           //7
        INITIALIZE(g_IntroSkip.Initialize());
        INITIALIZE(g_StereoAudioFix.Initialize());
        INITIALIZE(Init_ScaleEffects());
        INITIALIZE(Init_AspectFOVFix());
        INITIALIZE(Init_HUDFix());
        INITIALIZE(Init_Miscellaneous());
        INITIALIZE(Init_ViewportFix());
        INITIALIZE(Init_LineScaling());
        INITIALIZE(Init_SkyboxFix());
        INITIALIZE(g_EffectSpeedFix.Initialize());

        //INITIALIZE(Init_GammaShader());
        //MGS2_MGS3_Aiming_Fix();
        //ReplaceSaveErrorMessages();
    }
}

DWORD __stdcall Main(void*)
{
    initStartTime = std::chrono::high_resolution_clock::now();
    Init_Logging();
    INITIALIZE(InitializeSubsystems());

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

BOOL APIENTRY DllMain(HMODULE hModule,
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
        SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED); //fixes the monitor going to sleep during cutscenes.
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
