#include "helper.hpp"
#include <spdlog/spdlog.h>

#pragma comment(lib,"Version.lib")

extern HMODULE baseModule;
extern std::filesystem::path sExePath;
extern std::filesystem::path sFixPath;
extern std::string sExeName;


namespace Memory
{

    void PatchBytes(uintptr_t address, const char* pattern, unsigned int numBytes)
    {
        DWORD oldProtect;
        VirtualProtect((LPVOID)address, numBytes, PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy((LPVOID)address, pattern, numBytes);
        VirtualProtect((LPVOID)address, numBytes, oldProtect, &oldProtect);
    }
   
    static HMODULE GetThisDllHandle()
    {
        MEMORY_BASIC_INFORMATION info;
        size_t len = VirtualQueryEx(GetCurrentProcess(), (void*)GetThisDllHandle, &info, sizeof(info));
        assert(len == sizeof(info));
        return len ? (HMODULE)info.AllocationBase : NULL;
    }

    std::string GetModuleVersion(HMODULE module)
    {
        if (!module)
            return "0.0.0";

        char modulePath[MAX_PATH] = { 0 };
        if (!GetModuleFileNameA(module, modulePath, MAX_PATH))
            return "0.0.0";

        DWORD handle = 0;
        DWORD size = GetFileVersionInfoSizeA(modulePath, &handle);
        if (size == 0)
            return "0.0.0";

        std::vector<BYTE> versionInfo(size);
        if (!GetFileVersionInfoA(modulePath, handle, size, versionInfo.data()))
            return "0.0.0";

        VS_FIXEDFILEINFO* fileInfo = nullptr;
        UINT fileInfoLen = 0;
        if (!VerQueryValueA(versionInfo.data(), "\\", reinterpret_cast<LPVOID*>(&fileInfo), &fileInfoLen) || !fileInfo)
            return "0.0.0";

        // Extract version numbers
        DWORD verMS = fileInfo->dwFileVersionMS;
        DWORD verLS = fileInfo->dwFileVersionLS;
        int major = HIWORD(verMS);
        int minor = LOWORD(verMS);
        int patch = HIWORD(verLS);

        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }

    // CSGOSimple's pattern scan
    // https://github.com/OneshotGH/CSGOSimple-master/blob/master/CSGOSimple/helpers/utils.cpp
    std::uint8_t* PatternScanSilent(void* module, const char* signature)
    {
        static auto pattern_to_byte = [](const char* pattern) {
            auto bytes = std::vector<int>{};
            auto start = const_cast<char*>(pattern);
            auto end = const_cast<char*>(pattern) + strlen(pattern);

            for (auto current = start; current < end; ++current) {
                if (*current == '?') {
                    ++current;
                    if (*current == '?')
                        ++current;
                    bytes.push_back(-1);
                }
                else {
                    bytes.push_back(strtoul(current, &current, 16));
                }
            }
            return bytes;
        };

        auto dosHeader = (PIMAGE_DOS_HEADER)module;
        auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)module + dosHeader->e_lfanew);

        auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
        auto patternBytes = pattern_to_byte(signature);
        auto scanBytes = reinterpret_cast<std::uint8_t*>(module);

        auto s = patternBytes.size();
        auto d = patternBytes.data();

        for (auto i = 0ul; i < sizeOfImage - s; ++i) {
            bool found = true;
            for (auto j = 0ul; j < s; ++j) {
                if (scanBytes[i + j] != d[j] && d[j] != -1) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return &scanBytes[i];
            }
        }
        return nullptr;
    }

    std::uint8_t* PatternScan(void* module, const char* signature, const char* prefix, const char* successMessage, const char* errorMessage)
    {
        std::uint8_t* foundPattern = PatternScanSilent(module, signature);
        if (foundPattern)
        {
            if (bVerboseLogging)
            {
                if (successMessage)
                {
                    spdlog::info("{} Address: {:s}+{:x}", successMessage, sExeName.c_str(), (uintptr_t)foundPattern - (uintptr_t)baseModule);

                }
                else
                {
                
                    spdlog::info("{}: Pattern scan found. Address: {:s}+{:x}", prefix, sExeName.c_str(), (uintptr_t)foundPattern - (uintptr_t)baseModule);
                }
            }
        }
        else
        {
            if (errorMessage)
            {
                spdlog::error("{}", errorMessage);

            }
            else
            {
                spdlog::error("{}: Pattern scan failed.", prefix);
            }
        }
        return foundPattern;
    }

    uintptr_t GetAbsolute(uintptr_t address) noexcept
    {
        return (address + 4 + *reinterpret_cast<std::int32_t*>(address));
    }

    uintptr_t GetRelativeOffset(uint8_t* addr) noexcept
    {
        return reinterpret_cast<uintptr_t>(addr) + 4 + *reinterpret_cast<int32_t*>(addr);
    }

    BOOL HookIAT(HMODULE callerModule, char const* targetModule, const void* targetFunction, void* detourFunction)
    {
        auto* base = (uint8_t*)callerModule;
        const auto* dos_header = (IMAGE_DOS_HEADER*)base;
        const auto nt_headers = (IMAGE_NT_HEADERS*)(base + dos_header->e_lfanew);
        const auto* imports = (IMAGE_IMPORT_DESCRIPTOR*)(base + nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

        for (int i = 0; imports[i].Characteristics; i++)
        {
            const char* name = (const char*)(base + imports[i].Name);
            if (lstrcmpiA(name, targetModule) != 0)
                continue;

            void** thunk = (void**)(base + imports[i].FirstThunk);

            for (; *thunk; thunk++)
            {
                const void* import = *thunk;

                if (import != targetFunction)
                    continue;

                DWORD oldState;
                if (!VirtualProtect(thunk, sizeof(void*), PAGE_READWRITE, &oldState))
                    return FALSE;

                *thunk = detourFunction;

                VirtualProtect(thunk, sizeof(void*), oldState, &oldState);

                return TRUE;
            }
        }
        return FALSE;
    }
}

namespace Util
{

    int findStringInVector(std::string& str, const std::initializer_list<std::string>& search)
    {
        std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c)
            {
                return std::tolower(c);
            });
        auto it = std::find(search.begin(), search.end(), str);
        if (it != search.end())
            return (int)std::distance(search.begin(), it);
        return 0;
    }

    // Convert an UTF8 string to a wide Unicode String
    std::wstring utf8_decode(const std::string& str)
    {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }

    std::pair<int, int> GetPhysicalDesktopDimensions()
    {
        if (DEVMODE devMode { .dmSize = sizeof(DEVMODE) }; EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode))
            return { devMode.dmPelsWidth, devMode.dmPelsHeight };
        return {};
    }

    std::string GetFileDescription(const std::string& filePath)
    {
        DWORD handle = 0;
        DWORD size = GetFileVersionInfoSizeA(filePath.c_str(), &handle);
        if (size > 0)
        {
            std::vector<BYTE> versionInfo(size);
            if (GetFileVersionInfoA(filePath.c_str(), handle, size, versionInfo.data()))
            {
                void* buffer = nullptr;
                UINT sizeBuffer = 0;
                if (VerQueryValueA(versionInfo.data(), R"(\VarFileInfo\Translation)", &buffer, &sizeBuffer))
                {
                    auto translations = static_cast<WORD*>(buffer);
                    size_t translationCount = sizeBuffer / sizeof(WORD) / 2; // Each translation is two WORDs (language and code page)
                    for (size_t i = 0; i < translationCount; ++i)
                    {
                        WORD language = translations[i * 2];
                        WORD codePage = translations[i * 2 + 1];
                        // Construct the query string for the file description
                        std::ostringstream subBlock;
                        subBlock << R"(\StringFileInfo\)" << std::hex << std::setw(4) << std::setfill('0') << language
                            << std::setw(4) << std::setfill('0') << codePage << R"(\ProductName)";
                        if (VerQueryValueA(versionInfo.data(), subBlock.str().c_str(), &buffer, &sizeBuffer))
                        {
                            return std::string(static_cast<char*>(buffer), sizeBuffer - 1);
                        }
                    }
                }
            }
        }
        return "File description not found.";
    }

    ///Scans all valid ASI directories for any .asi files matching the fileName.
    bool CheckForASIFiles(std::string fileName, bool checkForDuplicates, bool setFixPath, const char* checkCreationDate)
    {
        std::array<std::string, 4> paths = { "", "plugins", "scripts", "update" };
        std::filesystem::path foundPath;
        for (const auto& path : paths)
        {
            auto filePath = sExePath / path / (fileName + ".asi");
            if (std::filesystem::exists(filePath))
            {
                if (checkCreationDate)
                {
                    auto fileTime = std::filesystem::last_write_time(filePath);
                    auto fileTimeChrono = std::chrono::system_clock::to_time_t(std::chrono::clock_cast<std::chrono::system_clock>(fileTime));
                    std::tm fileCreationTime = *std::localtime(&fileTimeChrono);
                    std::tm checkDate = {};
                    std::istringstream ss(checkCreationDate);
                    ss >> std::get_time(&checkDate, "%Y-%m-%d");
                    if (ss.fail() || std::mktime(&fileCreationTime) >= std::mktime(&checkDate))
                    {
                        continue; // Skip this file if it doesn't meet the creation date requirement
                    }
                }
                if (!foundPath.empty()) // multiple versions found
                {
                    AllocConsole();
                    FILE* dummy;
                    freopen_s(&dummy, "CONOUT$", "w", stdout);
                    std::string errorMessage = "DUPLICATE FILE ERROR: Duplicate " + fileName + ".asi installations found! Please make sure to delete any old versions!\n";
                    errorMessage.append("DUPLICATE FILE ERROR - Installation 1: ").append((sExePath / foundPath / (fileName + ".asi\n")).string());
                    errorMessage.append("DUPLICATE FILE ERROR - Installation 2: ").append((sExePath / path / (fileName + ".asi\n")).string());
                    std::cout << errorMessage;
                    spdlog::error("{}", errorMessage);
                    FreeLibraryAndExitThread(baseModule, 1);
                }
                foundPath = path;
                if (setFixPath)
                {
                    sFixPath = foundPath;
                }
                if (!checkForDuplicates)
                {
                    return TRUE;
                }
            }
        }
        return FALSE;
    }

    
}