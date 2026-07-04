#include "stdafx.h"
#include "helper.hpp"
#include "common.hpp"
#include "config.hpp"

#include "logging.hpp"

#pragma comment(lib,"Version.lib")


#pragma comment(lib, "bcrypt.lib")

namespace Memory
{
    std::vector<int> PatternToBytes(const char* pattern)
    {
        std::vector<int> bytes {};
        const char* current = pattern;
        const char* end = pattern + strlen(pattern);

        while (current < end)
        {
            if (*current == ' ')
            {
                ++current;
                continue;
            }

            if (*current == '?')
            {
                ++current;
                if (current < end && *current == '?')
                {
                    ++current;
                }
                bytes.push_back(-1);
                continue;
            }

            bytes.push_back(static_cast<int>(strtoul(current, const_cast<char**>(&current), 16)));
        }

        return bytes;
    }

    bool IsReadable(const void* ptr, size_t size)
    {
        if (!ptr || size == 0)
        {
            return false;
        }

        MEMORY_BASIC_INFORMATION mbi {};
        if (!VirtualQuery(ptr, &mbi, sizeof(mbi)))
        {
            return false;
        }

        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
        {
            return false;
        }

        const uintptr_t begin = reinterpret_cast<uintptr_t>(ptr);
        const uintptr_t end = begin + size;
        const uintptr_t regionEnd = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        return end >= begin && end <= regionEnd;
    }

    bool IsWritable(const void* ptr, size_t size)
    {
        if (!ptr || size == 0)
        {
            return false;
        }

        MEMORY_BASIC_INFORMATION mbi {};
        if (!VirtualQuery(ptr, &mbi, sizeof(mbi)))
        {
            return false;
        }

        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
        {
            return false;
        }

        constexpr DWORD writable =
            PAGE_READWRITE |
            PAGE_WRITECOPY |
            PAGE_EXECUTE_READWRITE |
            PAGE_EXECUTE_WRITECOPY;

        const uintptr_t begin = reinterpret_cast<uintptr_t>(ptr);
        const uintptr_t end = begin + size;
        const uintptr_t regionEnd = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        return end >= begin && end <= regionEnd && (mbi.Protect & writable);
    }

    bool IsExecutable(const void* ptr)
    {
        if (!ptr)
        {
            return false;
        }

        MEMORY_BASIC_INFORMATION mbi {};
        if (!VirtualQuery(ptr, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT ||
            (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
        {
            return false;
        }

        const DWORD protect = mbi.Protect & 0xff;
        return protect == PAGE_EXECUTE ||
            protect == PAGE_EXECUTE_READ ||
            protect == PAGE_EXECUTE_READWRITE ||
            protect == PAGE_EXECUTE_WRITECOPY;
    }

    size_t ReadableBytes(const void* ptr)
    {
        if (!ptr)
        {
            return 0;
        }

        MEMORY_BASIC_INFORMATION mbi {};
        if (!VirtualQuery(ptr, &mbi, sizeof(mbi)))
        {
            return 0;
        }

        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
        {
            return 0;
        }

        const uintptr_t begin = reinterpret_cast<uintptr_t>(ptr);
        const uintptr_t regionEnd = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        return regionEnd > begin ? regionEnd - begin : 0;
    }

    void PatchBytes(uintptr_t address, const char* pattern, unsigned int numBytes)
    {
        DWORD oldProtect;
        VirtualProtect((LPVOID)address, numBytes, PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy((LPVOID)address, pattern, numBytes);
        VirtualProtect((LPVOID)address, numBytes, oldProtect, &oldProtect);
    }

    bool PatchFloatImmediate(void* module, const char* signature, ptrdiff_t immediateOffset, uint32_t value, const char* prefix)
    {
        if (uint8_t* address = PatternScan(module, signature, prefix))
        {
            Write<uint32_t>(reinterpret_cast<uintptr_t>(address) + immediateOffset, value);
            return true;
        }

        return false;
    }
   
    static HMODULE GetThisDllHandle()
    {
        MEMORY_BASIC_INFORMATION info;
        size_t len = VirtualQueryEx(GetCurrentProcess(), (void*)GetThisDllHandle, &info, sizeof(info));
        assert(len == sizeof(info));
        return len ? (HMODULE)info.AllocationBase : NULL;
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

    std::uint8_t* PatternScan(void* module, const char* signature, const char* prefix)
    {
        std::uint8_t* foundPattern = PatternScanSilent(module, signature);
        if (foundPattern)
        {
            if (g_Logging.bVerboseLogging)
            {

                spdlog::info("{}: Pattern scan found. Address: {:s}+{:X}", prefix, sExeName.c_str(), (uintptr_t)foundPattern - (uintptr_t)baseModule);
            }
        }
        else
        {

            spdlog::error("{}: Pattern scan failed.", prefix);
        }
        return foundPattern;
    }

    std::vector<std::uint8_t*> FindMultiplePatternMatches(void* module, const char* signature)
    {
        std::vector<std::uint8_t*> matches {};
        auto* dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
        auto* ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module) + dosHeader->e_lfanew);
        const size_t sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
        const std::vector<int> patternBytes = PatternToBytes(signature);
        auto* scanBytes = reinterpret_cast<std::uint8_t*>(module);

        for (size_t i = 0; i + patternBytes.size() <= sizeOfImage; ++i)
        {
            bool found = true;
            for (size_t j = 0; j < patternBytes.size(); ++j)
            {
                if (patternBytes[j] != -1 && scanBytes[i + j] != static_cast<std::uint8_t>(patternBytes[j]))
                {
                    found = false;
                    break;
                }
            }

            if (found)
            {
                matches.push_back(scanBytes + i);
            }
        }

        return matches;
    }

    uintptr_t GetAbsolute(uintptr_t address) noexcept
    {
        return (address + 4 + *reinterpret_cast<std::int32_t*>(address));
    }

    uintptr_t GetRelativeOffset(uint8_t* addr) noexcept
    {
        return reinterpret_cast<uintptr_t>(addr) + 4 + *reinterpret_cast<int32_t*>(addr);
    }

    uintptr_t GetRipRelativeAddress(std::uint8_t* instruction, std::size_t displacementOffset, std::size_t instructionLength) noexcept
    {
        const auto displacement = *reinterpret_cast<std::int32_t*>(instruction + displacementOffset);
        return reinterpret_cast<uintptr_t>(instruction) + instructionLength + displacement;
    }

    uint8_t* ResolveCall(uint8_t* callInsn)
    {
        int32_t rel = *reinterpret_cast<int32_t*>(callInsn + 1);
        return callInsn + 5 + rel;
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
    // Read the current IAT entry (without changing it)
    void* ReadIAT(HMODULE callerModule, const char* targetModule, const char* targetFunction)
    {
        uint8_t* base = reinterpret_cast<uint8_t*>(callerModule);
        auto dos_header = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto nt_headers = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos_header->e_lfanew);
        auto imports = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
            base + nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

        for (int i = 0; imports[i].Characteristics; ++i)
        {
            const char* dllName = reinterpret_cast<const char*>(base + imports[i].Name);
            if (_stricmp(dllName, targetModule) != 0)
                continue;

            auto origFirstThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(base + imports[i].OriginalFirstThunk);
            auto firstThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(base + imports[i].FirstThunk);

            for (; origFirstThunk->u1.AddressOfData; ++origFirstThunk, ++firstThunk)
            {
                auto importByName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + origFirstThunk->u1.AddressOfData);
                if (strcmp(reinterpret_cast<const char*>(importByName->Name), targetFunction) != 0)
                    continue;

                return reinterpret_cast<void*>(firstThunk->u1.Function);
            }
        }

        return nullptr;
    }

    // Write a new pointer into the IAT entry (unconditionally)
    BOOL WriteIAT(HMODULE callerModule, const char* targetModule, const char* targetFunction, void* detourFunction)
    {
        uint8_t* base = reinterpret_cast<uint8_t*>(callerModule);
        auto dos_header = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto nt_headers = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos_header->e_lfanew);
        auto imports = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
            base + nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

        for (int i = 0; imports[i].Characteristics; ++i)
        {
            const char* dllName = reinterpret_cast<const char*>(base + imports[i].Name);
            if (_stricmp(dllName, targetModule) != 0)
                continue;

            auto origFirstThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(base + imports[i].OriginalFirstThunk);
            auto firstThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(base + imports[i].FirstThunk);

            for (; origFirstThunk->u1.AddressOfData; ++origFirstThunk, ++firstThunk)
            {
                auto importByName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + origFirstThunk->u1.AddressOfData);
                if (strcmp(reinterpret_cast<const char*>(importByName->Name), targetFunction) != 0)
                    continue;

                DWORD oldProtect;
                if (!VirtualProtect(&firstThunk->u1.Function, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect))
                    return FALSE;

                firstThunk->u1.Function = reinterpret_cast<ULONG_PTR>(detourFunction);

                VirtualProtect(&firstThunk->u1.Function, sizeof(void*), oldProtect, &oldProtect);

                return TRUE;
            }
        }

        return FALSE;
    }


    void AddStackInt32(uint64_t rsp, ptrdiff_t offset, int32_t amount)
    {
        auto* value = reinterpret_cast<int32_t*>(rsp + offset);
        if (Memory::IsWritable(value, sizeof(*value)))
        {
            *value += amount;
        }
    }

    void HookMidAtOffset(uint8_t* address, ptrdiff_t offset, SafetyHookMid& hook, const char* name, void (*callback)(SafetyHookContext&))
    {
        hook = safetyhook::create_mid(address + offset, callback);
        LOG_HOOK(hook, name)
    }

}

namespace Util
{
#if defined(ENABLE_DUMP_CONTEXT)
    void DumpContext(const safetyhook::Context& ctx)
    {
        spdlog::info("\n"
#if defined(_M_X64) || defined(__x86_64__)
            // General-purpose 64-bit registers
            "RAX = 0x{:X}\t| RBX = 0x{:X}\t| RCX = 0x{:X}\t| RDX = 0x{:X}\n"
            "RSI = 0x{:X}\t| RDI = 0x{:X}\t| RBP = 0x{:X}\t| RSP = 0x{:X}\n"
            "R8  = 0x{:X}\t| R9  = 0x{:X}\t| R10 = 0x{:X}\t| R11 = 0x{:X}\n"
            "R12 = 0x{:X}\t| R13 = 0x{:X}\t| R14 = 0x{:X}\t| R15 = 0x{:X}\n"
            "RIP = 0x{:X}\n"
            // XMM floats
            "XMM0 = {:g}\t| XMM1 = {:g}\t| XMM2 = {:g}\t| XMM3 = {:g}\n"
            "XMM4 = {:g}\t| XMM5 = {:g}\t| XMM6 = {:g}\t| XMM7 = {:g}\n"
            "XMM8 = {:g}\t| XMM9 = {:g}\t| XMM10 = {:g}\t| XMM11 = {:g}\n"
            "XMM12 = {:g}\t| XMM13 = {:g}\t| XMM14 = {:g}\t| XMM15 = {:g}",
            ctx.rax, ctx.rbx, ctx.rcx, ctx.rdx,
            ctx.rsi, ctx.rdi, ctx.rbp, ctx.rsp,
            ctx.r8, ctx.r9, ctx.r10, ctx.r11,
            ctx.r12, ctx.r13, ctx.r14, ctx.r15,
            ctx.rip,
            ctx.xmm0.f32[0], ctx.xmm1.f32[0], ctx.xmm2.f32[0], ctx.xmm3.f32[0],
            ctx.xmm4.f32[0], ctx.xmm5.f32[0], ctx.xmm6.f32[0], ctx.xmm7.f32[0],
            ctx.xmm8.f32[0], ctx.xmm9.f32[0], ctx.xmm10.f32[0], ctx.xmm11.f32[0],
            ctx.xmm12.f32[0], ctx.xmm13.f32[0], ctx.xmm14.f32[0], ctx.xmm15.f32[0]
#else       
            // General-purpose 32-bit registers
             "EAX = 0x{:X}\t| EBX = 0x{:X}\t| ECX = 0x{:X}\t| EDX = 0x{:X}\n"
             "ESI = 0x{:X}\t| EDI = 0x{:X}\t| EBP = 0x{:X}\t| ESP = 0x{:X}\n"
             "EIP = 0x{:X}\n"
             // XMM floats
             "XMM0 = {:g}\t| XMM1 = {:g}\t| XMM2 = {:g}\t| XMM3 = {:g}\n"
             "XMM4 = {:g}\t| XMM5 = {:g}\t| XMM6 = {:g}\t| XMM7 = {:g}\n",
             ctx.eax, ctx.ebx, ctx.ecx, ctx.edx,
             ctx.esi, ctx.edi, ctx.ebp, ctx.esp,
             ctx.eip,
             ctx.xmm0.f32[0], ctx.xmm1.f32[0], ctx.xmm2.f32[0], ctx.xmm3.f32[0],
             ctx.xmm4.f32[0], ctx.xmm5.f32[0], ctx.xmm6.f32[0], ctx.xmm7.f32[0]
#endif
        );
    }

    void DumpBytes(uintptr_t address, size_t count = 16)
    {
        if (address == 0)
        {
            spdlog::error("DumpBytes: null address");
            return;
        }

        MEMORY_BASIC_INFORMATION mbi {};
        if (!VirtualQuery(reinterpret_cast<void*>(address), &mbi, sizeof(mbi)))
        {
            spdlog::error("DumpBytes: VirtualQuery failed for 0x{:X}", address);
            return;
        }

        const bool readable =
            mbi.State == MEM_COMMIT &&
            !(mbi.Protect & PAGE_NOACCESS) &&
            !(mbi.Protect & PAGE_GUARD);

        if (!readable)
        {
            spdlog::error("DumpBytes: unreadable address 0x{:X}, protect=0x{:X}", address, mbi.Protect);
            return;
        }

        auto* bytes = reinterpret_cast<const uint8_t*>(address);

        spdlog::info("Bytes at 0x{:X}:", address);

        for (size_t i = 0; i < count; ++i)
        {
            spdlog::info("  +0x{:02X}: 0x{:02X}", i, bytes[i]);
        }
    }
#endif



    std::string GetCommandLineArgs()
    {
        const wchar_t* cmdLineW = ::GetCommandLineW();
        if (!cmdLineW)
        {
            return {};
        }

        return WideToUTF8(cmdLineW);
    }

    bool IsProcessRunning(const std::filesystem::path& fullPath)
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        PROCESSENTRY32W entry {};
        entry.dwSize = sizeof(entry);

        bool found = false;

        if (Process32FirstW(snapshot, &entry))
        {
            do
            {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
                if (hProcess)
                {
                    wchar_t buf[MAX_PATH];
                    DWORD size = MAX_PATH;
                    if (QueryFullProcessImageNameW(hProcess, 0, buf, &size))
                    {
                        if (_wcsicmp(buf, fullPath.c_str()) == 0)
                        {
                            found = true;
                        }
                    }
                    CloseHandle(hProcess);
                    if (found) break;
                }
            } while (Process32NextW(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return found;
    }


    int findStringInVector(const std::string& str, const std::initializer_list<std::string>& search)
    {
        std::string lowerStr = str;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);

        for (auto it = search.begin(); it != search.end(); ++it)
        {
            std::string lower = *it;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            if (lowerStr == lower)
                return static_cast<int>(std::distance(search.begin(), it));
        }
        return 0;
    }



    // Convert an UTF8 string to a wide Unicode String
    std::wstring UTF8toWide(const std::string& str)
    {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }

    std::string WideToUTF8(const std::wstring& wstr)
    {
        if (wstr.empty()) return {};

        int sizeNeeded = WideCharToMultiByte(
            CP_UTF8, 0,
            wstr.data(), (int)wstr.size(),
            nullptr, 0, nullptr, nullptr
        );

        std::string result(sizeNeeded, 0);
        WideCharToMultiByte(
            CP_UTF8, 0,
            wstr.data(), (int)wstr.size(),
            result.data(), sizeNeeded,
            nullptr, nullptr
        );

        return result;
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

    bool IsRunningUnderWine()
    {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (!ntdll) return false;
        return GetProcAddress(ntdll, "wine_get_version") != nullptr;
    }

    ///Scans all valid ASI directories for any .asi files matching the fileName.
    bool CheckForASIFiles(std::string fileName, bool checkForDuplicates, bool setFixPath, const char* checkCreationDate)
    {
        std::array<std::string, 4> paths = { "", "plugins", "scripts", "update" };
        std::filesystem::path foundPath;
        bool bFoundOnce = false;
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
                        continue;
                    }
                }
                if (bFoundOnce)
                {
                    std::string errorMessage = "DUPLICATE FILE ERROR: Duplicate " + fileName + ".asi installations found! Please make sure to delete any old versions!\n";
                    errorMessage.append("DUPLICATE FILE ERROR - Installation 1: ").append((sExePath / foundPath / (fileName + ".asi")).string().append("\n"));
                    errorMessage.append("DUPLICATE FILE ERROR - Installation 2: ").append(filePath.string());
                    spdlog::error("{}", errorMessage);
                    Logging::ShowConsole();
                    std::cout << errorMessage << std::endl;
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
                bFoundOnce = true;
            }
        }
        return FALSE;
    }

    std::string GetNameAtIndex(const std::initializer_list<std::string>& list, int index)
    {
        if (index >= 0 && index < static_cast<int>(list.size()))
        {
            auto it = list.begin();
            std::advance(it, index);
            return *it;
        }
        return "Unknown";
    }

    std::string GetUppercaseNameAtIndex(const std::initializer_list<std::string>& list, int index)
    {
        if (index >= 0 && index < static_cast<int>(list.size()))
        {
            auto it = list.begin();
            std::advance(it, index);
            std::string name = *it;
            std::transform(name.begin(), name.end(), name.begin(), ::toupper);
            return name;
        }
        return "UNKNOWN";
    }

    bool IsSteamOS()
    {
        static bool bCheckedSteamDeck = false;
        static bool bIsSteamDeck = false;
        if (bCheckedSteamDeck)
        {
            return bIsSteamDeck;
        }
        bCheckedSteamDeck = true;
        // Check for Proton/Steam Deck environment variables
        if (std::getenv("STEAM_COMPAT_CLIENT_INSTALL_PATH") || std::getenv("STEAM_COMPAT_DATA_PATH") || std::getenv("XDG_SESSION_TYPE"))
        {
            bIsSteamDeck = true;
        }
        return bIsSteamDeck;
    }

    std::string StripQuotes(const std::string& value)
    {
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
        {
            std::string s = value.substr(1, value.size() - 2);
            // Handle escaped quotes
            size_t pos = 0;
            while ((pos = s.find("\\\"", pos)) != std::string::npos)
            {
                s.replace(pos, 2, "\"");
                pos += 1;
            }
            return s;
        }
        return value;
    }


    std::string GetParentProcessName(const bool returnFullPath = false)
    {
        DWORD currentPid = GetCurrentProcessId();
        DWORD parentPid = 0;

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
        {
            return {};
        }

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(snapshot, &pe))
        {
            do
            {
                if (pe.th32ProcessID == currentPid)
                {
                    parentPid = pe.th32ParentProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &pe));
        }
        CloseHandle(snapshot);

        if (parentPid == 0)
        {
            return {};
        }

        HANDLE hParent = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, parentPid);
        if (!hParent)
        {
            return {};
        }

        char exePath[MAX_PATH] = {};
        DWORD size = sizeof(exePath);
        if (!QueryFullProcessImageNameA(hParent, 0, exePath, &size))
        {
            CloseHandle(hParent);
            return {};
        }
        CloseHandle(hParent);

        std::string name = exePath;
        if (returnFullPath)
        {
            return name;
        }
        size_t pos = name.find_last_of("\\/");
        if (pos != std::string::npos)
        {
            name = name.substr(pos + 1);
        }

        // lowercase normalize
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        return name;
    }

    bool IsProcessParent(const std::string& exeName)
    {
        std::string parent = GetParentProcessName(false);
        if (parent.empty())
        {
            return false;
        }

        std::string target = exeName;
        std::transform(target.begin(), target.end(), target.begin(), ::tolower);
        return parent == target;
    }


    std::string GetFileProductName(const std::filesystem::path& path)
    {
        DWORD handle = 0;
        DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
        if (size == 0)
        {
            return {};
        }

        std::vector<char> buffer(size);
        if (!GetFileVersionInfoW(path.c_str(), handle, size, buffer.data()))
        {
            return {};
        }

        struct LANGANDCODEPAGE
        {
            WORD wLanguage; WORD wCodePage;
        };
        LANGANDCODEPAGE* lpTranslate = nullptr;
        UINT cbTranslate = 0;

        if (!VerQueryValueW(buffer.data(), L"\\VarFileInfo\\Translation",
                            reinterpret_cast<LPVOID*>(&lpTranslate), &cbTranslate))
        {
            return {};
        }

        // Just take the first translation entry
        wchar_t subBlock[50];
        swprintf_s(subBlock, L"\\StringFileInfo\\%04x%04x\\ProductName",
                   lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);

        LPVOID lpBuffer = nullptr;
        UINT dwBytes = 0;
        if (VerQueryValueW(buffer.data(), subBlock, &lpBuffer, &dwBytes) && dwBytes > 0)
        {
            std::wstring ws(static_cast<wchar_t*>(lpBuffer), dwBytes);
            return std::string(ws.begin(), ws.end());
        }

        return {};
    }


    bool SHA1Check(const std::filesystem::path& filePath, const std::string& expected)
    {
        BCRYPT_ALG_HANDLE hAlg = nullptr;
        BCRYPT_HASH_HANDLE hHash = nullptr;
        DWORD hashObjectSize = 0;
        DWORD cbData = 0;
        BYTE* hashObject = nullptr;
        BYTE hash[20]; // SHA-1 = 20 bytes

        if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA1_ALGORITHM, nullptr, 0))
        {
            spdlog::error("SHA1Check: BCryptOpenAlgorithmProvider failed for '{}'", filePath.string());
            return false;
        }

        if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&hashObjectSize),
                              sizeof(DWORD), &cbData, 0))
        {
            spdlog::error("SHA1Check: BCryptGetProperty failed for '{}'", filePath.string());
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        hashObject = new BYTE[hashObjectSize];
        if (BCryptCreateHash(hAlg, &hHash, hashObject, hashObjectSize, nullptr, 0, 0))
        {
            spdlog::error("SHA1Check: BCryptCreateHash failed for '{}'", filePath.string());
            delete[] hashObject;
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        std::ifstream file(filePath, std::ios::binary);
        if (!file)
        {
            spdlog::error("SHA1Check: Failed to open file '{}'", filePath.string());
            delete[] hashObject;
            BCryptDestroyHash(hHash);
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        std::vector<char> buffer(1 << 16);
        while (file.good())
        {
            file.read(buffer.data(), buffer.size());
            if (BCryptHashData(hHash, reinterpret_cast<PUCHAR>(buffer.data()), static_cast<ULONG>(file.gcount()), 0))
            {
                spdlog::error("SHA1Check: BCryptHashData failed for '{}'", filePath.string());
                delete[] hashObject;
                BCryptDestroyHash(hHash);
                BCryptCloseAlgorithmProvider(hAlg, 0);
                return false;
            }
        }

        if (BCryptFinishHash(hHash, hash, sizeof(hash), 0))
        {
            spdlog::error("SHA1Check: BCryptFinishHash failed for '{}'", filePath.string());
            delete[] hashObject;
            BCryptDestroyHash(hHash);
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        std::ostringstream oss;
        for (auto b : hash)
        {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
        }

        delete[] hashObject;
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);

        std::string computed = oss.str();

        if (computed.size() != expected.size())
        {
            spdlog::error("SHA1Check: Mismatch length for '{}' (expected {} chars, got {})",
                          filePath.string(), expected.size(), computed.size());
            return false;
        }

        bool match = std::equal(computed.begin(), computed.end(), expected.begin(),
                                [](char a, char b)
                                {
                                    return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
                                });
        return match;
    }


    bool IsFileReadOnly(const std::filesystem::path& path)
    {
        DWORD attrs = GetFileAttributesW(path.wstring().c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES)
        {
            std::wcerr << L"[ERROR] Failed to get attributes for: " << path << std::endl;
            spdlog::error("Failed to get attributes for file: {}", path.string());
            return false;
        }

        return (attrs & FILE_ATTRIBUTE_READONLY) != 0;
    }


    bool IsJapanese()
    {
        return sSkipLauncherLanguage == "jp" || sSkipLauncherRegion == "jp";
    }
}
