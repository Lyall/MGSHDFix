#include "common.hpp"
#include "logging.hpp"

#include <spdlog/sinks/base_sink.h>

#include "gpu_check.hpp"
#include "steamworks_api.hpp"
#include "version.h"


// Spdlog sink (truncate on startup, single file)
template<typename Mutex>
class size_limited_sink : public spdlog::sinks::base_sink<Mutex>
{
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
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
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

void Logging::ShowConsole()
{
    if (g_Logging.bConsoleShown)
    {
        return;
    }
    g_Logging.bConsoleShown = true;
    AllocConsole();
    FILE* dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
}


void Logging::Initialize()
{
    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    sExePath = sExePath.remove_filename();

    // spdlog initialisation
    {
        try
        {
            bool logDirExists = std::filesystem::is_directory(sExePath / "logs");
            if (!logDirExists)
            {
                std::filesystem::create_directory(sExePath / "logs"); //create a "logs" subdirectory in the game folder to keep the main directory tidy.
            }
            g_Logging.bLoaded = true;
            // Create 10MB truncated logger
            std::filesystem::path sLogFile = sFixName + ".log";
            std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>(sLogFile.string(), std::make_shared<size_limited_sink<std::mutex>>((sExePath / "logs" / sLogFile).string(), 10 * 1024 * 1024));
            spdlog::set_default_logger(logger);

            spdlog::flush_on(spdlog::level::debug);
            spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            spdlog::info("---------- Logging initialization started ----------");
            if (!logDirExists)
            {
                spdlog::info("New log subdirectory created.");
            }
            spdlog::info("Checking for duplicate intallations of {}.", sFixName);
            Util::CheckForASIFiles(sFixName, true, true, nullptr); //Sets sFixPath. Exit thread & warn the user if multiple copies of MGSHDFix are trying to initialize.
            spdlog::info("{} v{} loaded.", sFixName, sFixVersion);
            spdlog::info("ASI plugin location: {}", (sExePath / sFixPath / (sFixName + ".asi")).string());
            spdlog::info("----------");
            spdlog::info("Log file: {}", (sExePath / "logs" / sLogFile).string());
            spdlog::info("----------");

            // Log module details
            spdlog::info("Module Name: {0:s}", sExeName.c_str());
            spdlog::info("Module Path: {0:s}", sExePath.string());
            spdlog::info("Module Address: 0x{0:X}", (uintptr_t)baseModule);
            spdlog::info("Module First Segment: 0x{0:X}", (uintptr_t)baseModule+0x1000);
            spdlog::info("Module Version: {}", Memory::GetModuleVersion(baseModule));
            if (std::filesystem::exists(sExePath / "steamclient64.dll") || std::filesystem::exists(sExePath / "steamclient.dll") || std::filesystem::exists(sExePath / "GameOverlayRenderer64.dll") || std::filesystem::exists(sExePath / "GameOverlayRenderer.dll"))
            {
                g_SteamAPI.bIsLegitCopy = false;
                spdlog::warn("Piracy Warning: This has been detected as a pirated copy of the game. Crashing issues are VERY likely to occur due to missing memory patterns.");
            }
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            ShowConsole();
            std::cout << "Log initialisation failed: " << ex.what() << std::endl;
            return FreeLibraryAndExitThread(baseModule, 1);
        }
    }
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - g_Logging.initStartTime).count();
    spdlog::info("---------- Logging loaded in: {} ms ----------", duration);
}

std::string Logging::GetSteamOSVersion()
{
    std::ifstream os_release("/etc/os-release");
    std::string line;
    while (std::getline(os_release, line))
    {
        if (line.find("PRETTY_NAME=") == 0)
        {
            // Remove quotes if present
            size_t first_quote = line.find('"');
            size_t last_quote = line.rfind('"');
            if (first_quote != std::string::npos && last_quote != std::string::npos && last_quote > first_quote)
            {
                return line.substr(first_quote + 1, last_quote - first_quote - 1);
            }
            return line.substr(13); // fallback
        }
    }
    return "SteamOS (Unknown Version)";
}

///Prints CPU, GPU, and RAM info to the log to expedite common troubleshooting.
void Logging::LogSysInfo()
{
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

    if (Util::IsSteamOS())
    {
        spdlog::info("System Details - Detected Steam Deck (SteamOS / Proton).");
    }
    else
    {
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
            CheckMinimumGPU(deviceString);
        }
    }


    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    double totalMemory = static_cast<double>(status.ullTotalPhys) / 1024.0 / 1024.0;
    spdlog::info("System Details - RAM: {} GB ({:.0f} MB)", ceil((totalMemory / 1024) * 100) / 100, totalMemory);


    std::string os;

    if (Util::IsSteamOS())
    {
        os = GetSteamOSVersion();
    }
    else
    {
        HKEY key;
        LSTATUS versionResult = RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
            0, KEY_READ | KEY_WOW64_64KEY, &key
        );

        if (versionResult == ERROR_SUCCESS)
        {
            char buffer[256]; DWORD size = sizeof(buffer);
            LSTATUS nameResult = RegQueryValueExA(
                key, "ProductName",
                nullptr, nullptr,
                reinterpret_cast<LPBYTE>(buffer), &size
            );
            if (nameResult == ERROR_SUCCESS)
            {
                os = buffer;
            }
        }

        // Read UBR (Update Build Revision) value from registry
        DWORD ubr = 0;
        if (key != nullptr)
        {
            DWORD ubrSize = sizeof(ubr);
            RegQueryValueExA(key, "UBR", nullptr, nullptr, reinterpret_cast<LPBYTE>(&ubr), &ubrSize);
        }

        RegCloseKey(key);

        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (ntdll)
        {
            typedef LONG(WINAPI* RtlGetVersion_t)(PRTL_OSVERSIONINFOW);
            RtlGetVersion_t RtlGetVersion =
                reinterpret_cast<RtlGetVersion_t>(GetProcAddress(ntdll, "RtlGetVersion"));
            if (RtlGetVersion)
            {
                RTL_OSVERSIONINFOW info = {};
                info.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);

                if (RtlGetVersion(&info) == 0)
                {
                    // Append build number and UBR (e.g. " (26100.4652)")
                    os += " (" + std::to_string(info.dwBuildNumber) + "." + std::to_string(ubr) + ")";

                    // Replace "Windows 10" with "Windows 11" if build number is 22000 or greater
                    if (info.dwBuildNumber >= 22000)
                    {
                        std::size_t pos = os.find("Windows 10");
                        if (pos != std::string::npos)
                        {
                            os.replace(pos, 10, "Windows 11");
                        }
                    }
                }
            }
        }
    }
    if (!os.empty()) spdlog::info("System Details - OS:  {}", os);

}
