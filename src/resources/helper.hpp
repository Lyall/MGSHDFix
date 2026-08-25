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

    uint8_t* ResolveCall(uint8_t* callInsn);

    uintptr_t GetRipRelativeAddress(std::uint8_t* instruction, std::size_t displacementOffset, std::size_t instructionLength) noexcept;

    BOOL HookIAT(HMODULE callerModule, char const* targetModule, const void* targetFunction, void* detourFunction);

    void* ReadIAT(HMODULE callerModule, const char* targetModule, const char* targetFunction);
    BOOL WriteIAT(HMODULE callerModule, const char* targetModule, const char* targetFunction, void* detourFunction);

    void AddStackInt32(uint64_t rsp, ptrdiff_t offset, int32_t amount);

    void HookMidAtOffset(uint8_t* address, ptrdiff_t offset, SafetyHookMid& hook, const char* name, void (*callback)(SafetyHookContext&));
}

namespace Util
{
#if !defined(RELEASE_BUILD)
#ifndef ENABLE_DUMP_CONTEXT
#define ENABLE_DUMP_CONTEXT
#endif
    void DumpContext(const safetyhook::Context& ctx);

    void DumpBytes(uintptr_t address, size_t count = 16)
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

    std::string GetParentProcessName(bool returnFullPath = false);

    bool IsProcessParent(const std::string& exeName);


    std::string GetFileProductName(const std::filesystem::path& path);

    using SHA1Hash = std::array<std::uint8_t, 20>;

    std::optional<SHA1Hash> ComputeSHA1Bytes(const std::filesystem::path& filePath);

    bool SHA1Equals(const SHA1Hash& actual, std::string_view expected);

    bool SHA1Check(const std::filesystem::path& filePath, std::string_view expected);

    void ShutdownSHA1Provider();

    bool IsFileReadOnly(const std::filesystem::path& path);

    // FNV-1a over the texel rows of an RGBA8 upload.
    inline uint64_t HashTexels(const void* data, size_t rowPitch, unsigned width, unsigned height)
    {
        uint64_t hash = 0xcbf29ce484222325ull;
        for (unsigned y = 0; y < height; y++)
        {
            const auto* row = static_cast<const uint8_t*>(data) + static_cast<size_t>(y) * rowPitch;
            for (unsigned b = 0; b < width * 4; b++)
            {
                hash = (hash ^ row[b]) * 0x100000001b3ull;
            }
        }
        return hash;
    }

    bool IsJapanese();


    //from zlib
    constexpr uint32_t kCRC32Poly = 0xFFFFFFFFu;
    inline constexpr uint32_t kCRC32Table[256] =
    {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
        0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
        0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
        0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
        0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
        0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
        0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
        0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
        0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
        0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
        0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
        0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
        0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
        0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
        0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
        0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
        0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
        0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
        0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
        0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
        0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
        0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
        0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
        0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
        0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
        0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
        0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
        0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
        0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
        0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
        0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
        0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
        0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
        0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
        0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
        0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
        0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
        0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
        0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
        0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
        0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
        0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
        0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
        0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
        0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
        0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
        0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
        0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
        0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
        0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
        0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
        0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
        0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
        0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };

    constexpr uint32_t StringToCRC32(const char* str)
    {
        uint32_t crc = kCRC32Poly;
        for (const char* p = str; *p != '\0'; ++p)
            crc = (crc >> 8) ^ kCRC32Table[(crc & 0xFF) ^ static_cast<uint8_t>(*p)];
        return crc ^ kCRC32Poly;
    }

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
 * MAKE_HOOK_MID(baseModule, "74 ?? B9 ?? ?? ?? ?? C7", "completion check", {
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
   * MAKE_HOOK_TRAMPOLINE(baseModule, "E8 ?? ?? ?? ?? C7", "feature toggle", bool, {
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
