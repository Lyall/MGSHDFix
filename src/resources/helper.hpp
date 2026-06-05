#pragma once
#include "RegStateHelpers.hpp"

inline std::filesystem::path sFixPath;

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

    bool PatchFloatImmediate(void* module, const char* signature, ptrdiff_t immediateOffset, uint32_t value, const char* prefix);

    static HMODULE GetThisDllHandle();

    std::uint8_t* PatternScanSilent(void* module, const char* signature);

    std::uint8_t* PatternScan(void* module, const char* signature, const char* prefix);

    std::vector<std::uint8_t*> FindMultiplePatternMatches(void* module, const char* signature);

    bool IsReadable(const void* ptr, size_t size);

    bool IsWritable(const void* ptr, size_t size);

    bool IsExecutable(const void* ptr);

    size_t ReadableBytes(const void* ptr);

    template<typename T>
    T ReadField(uintptr_t base, ptrdiff_t offset, T fallback = {})
    {
        const auto* field = reinterpret_cast<const T*>(base + offset);
        if (!IsReadable(field, sizeof(*field)))
        {
            return fallback;
        }

        return *field;
    }

    uintptr_t GetAbsolute(uintptr_t address) noexcept;

    uintptr_t GetRelativeOffset(uint8_t* addr) noexcept;

    uintptr_t GetRipRelativeAddress(std::uint8_t* instruction, std::size_t displacementOffset, std::size_t instructionLength) noexcept;

    BOOL HookIAT(HMODULE callerModule, char const* targetModule, const void* targetFunction, void* detourFunction);

    void* ReadIAT(HMODULE callerModule, const char* targetModule, const char* targetFunction);
    BOOL WriteIAT(HMODULE callerModule, const char* targetModule, const char* targetFunction, void* detourFunction);
}

namespace Util
{
#if !defined(RELEASE_BUILD)
#ifndef ENABLE_DUMP_CONTEXT
#define ENABLE_DUMP_CONTEXT
#endif
    void DumpContext(const safetyhook::Context& ctx);

    void DumpBytes(uintptr_t address, size_t count);
#endif
    
    std::string GetCommandLineArgs();

    bool IsProcessRunning(const std::filesystem::path& fullPath);

    int findStringInVector(const std::string& str, const std::initializer_list<std::string>& search);

    std::wstring UTF8toWide(const std::string& str);

    std::string WideToUTF8(const std::wstring& wstr);

    std::pair<int, int> GetPhysicalDesktopDimensions();

    std::string GetFileDescription(const std::string& filePath);

    bool IsRunningUnderWine();

    bool CheckForASIFiles(std::string fileName, bool checkForDuplicates, bool setFixPath, const char* checkCreationDate);

    std::string GetNameAtIndex(const std::initializer_list<std::string>& list, int index);

    std::string GetUppercaseNameAtIndex(const std::initializer_list<std::string>& list, int index);

    [[nodiscard]] bool IsSteamOS();

    std::string StripQuotes(const std::string& value);

    std::string GetParentProcessName(bool returnFullPath);

    bool IsProcessParent(const std::string& exeName);


    std::string GetFileProductName(const std::filesystem::path& path);

    bool SHA1Check(const std::filesystem::path& filePath, const std::string& expected);

    bool IsFileReadOnly(const std::filesystem::path& path);

}


///Input: SafetyHookMid, const char* Prefix, const char* successMessage (or NULL), const char* errorMessage (or NULL)
#define LOG_HOOK(hook, prefix)\
{\
    if (hook)\
    {\
        if (g_Logging.bVerboseLogging)\
        {\
            spdlog::info("{}: Hook installed.", prefix);\
        }\
    }\
    else\
    {\
        spdlog::error("{}: Hook failed.", prefix);\
    }\
}

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define UNIQUE_NAME(base) CONCAT(base, __COUNTER__)

/**
 * Usage:
 * MAKE_HOOK_MID(module, pattern, name, {
 *     // Your code here using ctx
 * });
 *
 * Example:
 * MAKE_HOOK_MID(baseModule, "74 ?? B9 ?? ?? ?? ??", "completion check", {
 *     ctx.rax = 0;
 *     reghelpers::SetZF(ctx, false);
 * });
 */
#define MAKE_HOOK_MID_IMPL(module, pattern, name, body, uniq)                       \
    if (uint8_t* CONCAT(_addr_, uniq) = Memory::PatternScan(module, pattern, name)) {\
        static SafetyHookMid CONCAT(hook_, uniq) {};                               \
        CONCAT(hook_, uniq) = safetyhook::create_mid(CONCAT(_addr_, uniq),         \
            [](SafetyHookContext& ctx) { body });                                  \
        LOG_HOOK(CONCAT(hook_, uniq), name)                                        \
    }

#define MAKE_HOOK_MID(module, pattern, name, body)                                 \
    MAKE_HOOK_MID_IMPL(module, pattern, name, body, UNIQUE_NAME(_unique))

 /**
  * Usage:
  * MAKE_HOOK_INLINE(module, pattern, name, {
  *     // Your code here using ctx
  * });
  *
  * Example:
  * MAKE_HOOK_INLINE(baseModule, "83 F8 01 75 ?? 48 8B", "force always true", {
  *     ctx.rax = 1;
  * });
  */

#define MAKE_HOOK_INLINE_IMPL(module, pattern, name, body, uniq)                    \
    if (uint8_t* CONCAT(_addr_, uniq) = Memory::PatternScan(module, pattern, name)) {\
        static SafetyHookInline CONCAT(hook_, uniq) {};                            \
        CONCAT(hook_, uniq) = safetyhook::create_inline(CONCAT(_addr_, uniq),      \
            [](SafetyHookContext& ctx) { body });                                  \
        LOG_HOOK(CONCAT(hook_, uniq), name)                                        \
    }

#define MAKE_HOOK_INLINE(module, pattern, name, body)                              \
    MAKE_HOOK_INLINE_IMPL(module, pattern, name, body, UNIQUE_NAME(_unique))

  /**
   * Usage:
   * MAKE_HOOK_TRAMPOLINE(module, pattern, name, ReturnType, {
   *     // Your code here using ctx and trampoline
   *     // Must return ReturnType
   * });
   *
   * Example:
   * MAKE_HOOK_TRAMPOLINE(baseModule, "E8 ?? ?? ?? ??", "feature toggle", bool, {
   *     if (shouldEnableFeature())
   *         return true;
   *     return trampoline(ctx);
   * });
   */

#define MAKE_HOOK_TRAMPOLINE_IMPL(module, pattern, name, retType, body, uniq)       \
    if (uint8_t* CONCAT(_addr_, uniq) = Memory::PatternScan(module, pattern, name)) {\
        static SafetyHookTrampoline<retType> CONCAT(hook_, uniq) {};               \
        CONCAT(hook_, uniq) = safetyhook::create_trampoline<retType>(              \
            CONCAT(_addr_, uniq), [](SafetyHookContext& ctx, auto& trampoline) {body});\
        LOG_HOOK(CONCAT(hook_, uniq), name)                                        \
    }

#define MAKE_HOOK_TRAMPOLINE(module, pattern, name, retType, body)                 \
    MAKE_HOOK_TRAMPOLINE_IMPL(module, pattern, name, retType, body, UNIQUE_NAME(_unique))

struct HookGuard
{
    bool& flag;
    HookGuard(bool& f) : flag(f)
    {
        flag = true;
    }
    ~HookGuard()
    {
        flag = false;
    }
};
