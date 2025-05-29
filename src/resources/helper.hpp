#pragma once

#include "stdafx.h"
#include "safetyhook.hpp"

extern bool bVerboseLogging;

namespace Memory
{
    template<typename T>
    void Write(uintptr_t writeAddress, T value)
    {
        DWORD oldProtect;
        VirtualProtect((LPVOID)(writeAddress), sizeof(T), PAGE_EXECUTE_WRITECOPY, &oldProtect);
        *(reinterpret_cast<T*>(writeAddress)) = value;
        VirtualProtect((LPVOID)(writeAddress), sizeof(T), oldProtect, &oldProtect);
    }

    void PatchBytes(uintptr_t address, const char* pattern, unsigned int numBytes);

    static HMODULE GetThisDllHandle();

    std::string GetModuleVersion(HMODULE module);

    std::uint8_t* PatternScanSilent(void* module, const char* signature);

    std::uint8_t* PatternScan(void* module, const char* signature, const char* prefix, const char* successMessage, const char* errorMessage);

    uintptr_t GetAbsolute(uintptr_t address) noexcept;

    uintptr_t GetRelativeOffset(uint8_t* addr) noexcept;

    BOOL HookIAT(HMODULE callerModule, char const* targetModule, const void* targetFunction, void* detourFunction);
}

namespace Util
{
    extern int findStringInVector(std::string& str, const std::initializer_list<std::string>& search);

    // Convert an UTF8 string to a wide Unicode String
    std::wstring utf8_decode(const std::string& str);

    std::pair<int, int> GetPhysicalDesktopDimensions();

    std::string GetFileDescription(const std::string& filePath);

    bool CheckForASIFiles(std::string fileName, bool checkForDuplicates, bool setFixPath);

}


///Input: SafetyHookMid, const char* Prefix, const char* successMessage (or NULL), const char* errorMessage (or NULL)
#define LOG_HOOK(hook, prefix, successMessage, errorMessage)\
{\
    if (hook)\
    {\
        if (bVerboseLogging)\
        {\
            if (successMessage)\
            {\
                spdlog::info("{}", successMessage);\
            }\
            else\
            {\
                spdlog::info("{}: Hook installed.", prefix);\
            }\
        }\
    }\
    else if (errorMessage)\
    {\
        spdlog::error("{}", errorMessage);\
    }\
    else\
    {\
        spdlog::error("{}: Hook failed.", prefix);\
    }\
}\
