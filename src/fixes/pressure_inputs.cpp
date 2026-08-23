#include "stdafx.h"
#include "pressure_inputs.hpp"

#include "common.hpp"
#include "logging.hpp"
#include "gamevars.hpp"

#include <hidsdi.h>
#include <limits>
#pragma comment(lib, "hid.lib")

// libgv\pad.c -> setup_pressure() flattens GV_PAD.pressure[12] to 0xFF/0x00 for any pad the engine
// does not recognise. Refill it from a DualShock 3, via DsHidMini in SDF or SXS mode.
namespace
{
    constexpr size_t kSlots = 12;
    constexpr size_t kCircle = 5;
    constexpr size_t kTriangle = 4;
    constexpr size_t kL1 = 8;           // libgv.h PAD_PRESS_L1
    constexpr size_t kR1 = 9;           // libgv.h PAD_PRESS_R1
    constexpr size_t kR2 = 11;          // libgv.h PAD_PRESS_R2
    constexpr size_t kSquare = 7;

    // pressure[] slot -> index within the report's pressure block, for each block order.
    // Our slots run R L U D TRI CIR CRO SQU L1 R1 L2 R2.
    constexpr int kSdfOrder[kSlots] = { 3, 5, 2, 4, 8, 9, 10, 11, 6, 7, 0, 1 };
    constexpr int kNativeOrder[kSlots] = { 1, 3, 0, 2, 8, 9, 10, 11, 6, 7, 4, 5 };

    // SXS carries all twelve in its feature report, SDF in a 39-byte input report with a different
    // order, and Wine's hidraw passthrough in the native 49-byte report.
    struct Profile
    {
        const char* name;
        bool feature;           // poll HidD_GetFeature rather than read input reports
        size_t length;
        size_t base;            // first pressure byte
        const int* order;
    };

    constexpr Profile kProfiles[] = {
        { "SXS",    true,  50, 15, kNativeOrder },
        { "SDF",    false, 39,  8, kSdfOrder },
        { "native", false, 49, 14, kNativeOrder },
    };
    const Profile* gProfile = nullptr;

    std::atomic<bool> gHavePad = false;
    std::atomic<uint8_t> gCirclePressure = 0;
    uint8_t gPressure[kSlots] {};
    std::mutex gPressureLock;

    // The open device's path and kProfiles index, for rumble's own write handle.
    std::wstring gDevicePath;
    int gDeviceMode = -1;

    void PublishDevice(const std::wstring& path, int mode)
    {
        const std::lock_guard<std::mutex> guard(gPressureLock);
        gDevicePath = path;
        gDeviceMode = mode;
    }

    constexpr DWORD kReadGone = ~0u;    // disconnected, as opposed to merely quiet

    // Reconnect backoff. Discovery walks the whole HID class key, so without this a session with
    // no pad attached rescans every couple of seconds until the game closes.
    constexpr DWORD kRetryFirst = 2000;
    constexpr DWORD kRetryMax = 16000;

    // Overlapped: a silent interface must not wedge discovery or the reader.
    DWORD ReadReport(HANDLE h, HANDLE ev, uint8_t* buf, DWORD len, DWORD timeoutMs)
    {
        OVERLAPPED ov {};
        ov.hEvent = ev;
        ResetEvent(ev);

        if (!ReadFile(h, buf, len, nullptr, &ov) && GetLastError() != ERROR_IO_PENDING)
        {
            return kReadGone;
        }
        if (WaitForSingleObject(ev, timeoutMs) != WAIT_OBJECT_0)
        {
            CancelIo(h);
            WaitForSingleObject(ev, 200);
            return 0;
        }

        DWORD read = 0;
        return GetOverlappedResult(h, &ov, &read, FALSE) ? read : kReadGone;
    }

    // DsHidMini stamps 00 3F over the first two bytes of the feature report. That pair is SXS.
    bool ReadFeature(HANDLE h, uint8_t* buf, size_t len)
    {
        memset(buf, 0, len);
        return HidD_GetFeature(h, buf, static_cast<ULONG>(len)) != FALSE;
    }

    // Match on exact length: a 49-byte report would otherwise parse as SDF at the wrong offsets.
    const Profile* ProbeInputProfile(HANDLE h)
    {
        uint8_t buf[96];
        HANDLE ev = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (ev == nullptr)
        {
            return nullptr;
        }
        const DWORD read = ReadReport(h, ev, buf, sizeof(buf), 500);
        CloseHandle(ev);

        for (const Profile& p : kProfiles)
        {
            if (!p.feature && read == p.length && buf[0] == 0x01)
            {
                return &p;
            }
        }
        return nullptr;
    }

    // Steam Input hides the pad from SetupDi, so take the interface paths from the registry.
    HANDLE OpenDs3()
    {
        GUID hid {};
        HidD_GetHidGuid(&hid);

        wchar_t guid[40];
        swprintf_s(guid, L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            hid.Data1, hid.Data2, hid.Data3, hid.Data4[0], hid.Data4[1], hid.Data4[2],
            hid.Data4[3], hid.Data4[4], hid.Data4[5], hid.Data4[6], hid.Data4[7]);

        HKEY cls = nullptr;
        const std::wstring devices =
            L"SYSTEM\\CurrentControlSet\\Control\\DeviceClasses\\" + std::wstring(guid);
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, devices.c_str(), 0, KEY_READ, &cls) != ERROR_SUCCESS)
        {
            return nullptr;
        }

        HANDLE found = nullptr;
        size_t candidates = 0;
        DWORD lastError = 0;
        wchar_t name[512];
        for (DWORD i = 0; found == nullptr; ++i)
        {
            DWORD len = static_cast<DWORD>(std::size(name));
            if (RegEnumKeyExW(cls, i, name, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
            {
                break;
            }
            std::wstring key = name;
            std::wstring lower = key;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
            if (lower.find(L"vid_054c") == std::wstring::npos
                || lower.find(L"pid_0268") == std::wstring::npos
                || lower.rfind(L"##?#", 0) != 0
                || lower.find(L"ig_00") != std::wstring::npos)   // XInput node, no pressure
            {
                continue;
            }
            ++candidates;
            // the registry spells the symbolic link with '#' for the leading separators only.
            // No access first: SXS only needs feature reports, and GENERIC_READ adds a reader.
            const std::wstring path = L"\\\\?\\" + key.substr(4);
            HANDLE h = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
            if (h == INVALID_HANDLE_VALUE)
            {
                lastError = GetLastError();     // 2 = stale entry, 5 = something is hiding it
                continue;
            }

            uint8_t probe[96] {};
            if (ReadFeature(h, probe, sizeof(probe)) && probe[1] == 0x00 && probe[2] == 0x3F)
            {
                found = h;
                gProfile = &kProfiles[0];
                PublishDevice(path, 0);
                continue;
            }

            // SDF and native need input reports, so reopen for read.
            CloseHandle(h);
            h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
            if (h == INVALID_HANDLE_VALUE)
            {
                lastError = GetLastError();
                continue;
            }
            // A dead node would block the reader and hide a working pad behind it.
            if (const Profile* profile = ProbeInputProfile(h))
            {
                found = h;
                gProfile = profile;
                PublishDevice(path, static_cast<int>(profile - kProfiles));
            }
            else
            {
                CloseHandle(h);
            }
        }
        RegCloseKey(cls);

        static bool reported = false;
        if (found == nullptr && !reported)
        {
            reported = true;
            if (candidates == 0)
            {
                spdlog::info("Pressure Inputs - No DualShock 3 registered.");
            }
            else if (lastError == ERROR_ACCESS_DENIED)
            {
                spdlog::warn("Pressure Inputs - {} DualShock 3 interface(s) found but access was "
                    "denied. HidHide must allow the game.", candidates);
            }
            else
            {
                spdlog::warn("Pressure Inputs - {} DualShock 3 interface(s) found, none reporting "
                    "pressure. DsHidMini must be in SDF or SXS mode.", candidates);
            }
        }
        return found;
    }

    DWORD WINAPI ReaderThread(LPVOID)
    {
        uint8_t report[96];
        HANDLE pad = nullptr;
        DWORD retry = kRetryFirst;
        HANDLE ev = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        for (;;)
        {
            if (pad == nullptr)
            {
                pad = OpenDs3();
                if (pad == nullptr)
                {
                    gHavePad = false;
                    Sleep(retry);
                    retry = std::min<DWORD>(retry * 2, kRetryMax);
                    continue;
                }
                retry = kRetryFirst;
                spdlog::info("Pressure Inputs - DualShock 3 opened, {} mode, {}-byte reports.",
                    gProfile->name, gProfile->length);
            }

            bool lost = false;
            if (gProfile->feature)
            {
                // Synchronous IOCTL, so keep it off the render thread and poll once a frame.
                lost = !ReadFeature(pad, report, gProfile->length);
                if (!lost)
                {
                    Sleep(8);
                }
            }
            else
            {
                const DWORD read = ReadReport(pad, ev, report, sizeof(report), 5000);
                if (read == 0)
                {
                    continue;      // quiet pad, not a lost one
                }
                lost = read == kReadGone || read < gProfile->length;
            }

            if (lost)
            {
                CloseHandle(pad);
                pad = nullptr;
                gProfile = nullptr;
                gHavePad = false;
                PublishDevice({}, -1);
                Sleep(retry);
                retry = std::min<DWORD>(retry * 2, kRetryMax);
                continue;
            }

            const std::lock_guard<std::mutex> guard(gPressureLock);
            for (size_t s = 0; s < kSlots; ++s)
            {
                gPressure[s] = report[gProfile->base + gProfile->order[s]];
            }
            gCirclePressure = gPressure[kCircle];
            gHavePad = true;
        }
    }

    void ReadPad(uint8_t (&out)[kSlots])
    {
        const std::lock_guard<std::mutex> guard(gPressureLock);
        memcpy(out, gPressure, sizeof(out));
    }

    // ---- MGS 2 ------------------------------------------------------------------------------
    // The negative inner thought is L1 on PS2, R2 in MC. Rebound only while a pad is present.
    // The read is redirected here so the game loads our value.
    uint8_t gThoughtScratch = 0;

    uintptr_t gThoughtGate = 0;
    uintptr_t gThoughtPick = 0;
    uintptr_t gThoughtHold = 0;
    std::atomic<bool> gThoughtRebound = false;

    std::atomic<bool> gThoughtOnlyL1 = false;

    void RebindNegativeThought(bool on)
    {
        if (on == gThoughtRebound.load()
            && (!on || PressureInputs::bSuppressAlternates == gThoughtOnlyL1.load()))
        {
            return;
        }
        gThoughtRebound = on;
        gThoughtOnlyL1 = PressureInputs::bSuppressAlternates;

        if (!on)                                                      // back to MC's own R2
        {
            if (gThoughtGate != 0)
            {
                Memory::PatchBytes(gThoughtGate + 7, "\x0A", 1);
            }
            if (gThoughtPick != 0)
            {
                Memory::PatchBytes(gThoughtPick, "\x45\x84\xC9", 3);
            }
            if (gThoughtHold != 0)
            {
                Memory::PatchBytes(gThoughtHold + 11, "\x23", 1);
            }
            return;
        }

        // Suppressed: L1 alone, as PS2. Otherwise MC's R2 stays live and L1 joins it.
        const bool only = PressureInputs::bSuppressAlternates;
        if (gThoughtGate != 0)
        {
            Memory::PatchBytes(gThoughtGate + 7, only ? "\x0C" : "\x0E", 1);   // R1|L1 (|R2)
        }
        if (gThoughtPick != 0)
        {
            // Only the button test: +9 is the last byte of the instruction the hook sits on.
            Memory::PatchBytes(gThoughtPick, only ? "\xF6\xC1\x04" : "\xF6\xC1\x06", 3);
        }
        if (gThoughtHold != 0)
        {
            Memory::PatchBytes(gThoughtHold + 11, only ? "\x20" : "\x23", 1);
        }
        spdlog::info("MGS 2: Pressure Inputs - Negative inner thought bound to {}.",
            only ? "L1" : "L1 and R2");
    }

    // On PS2, pressure[PL_PAD_PRESS_LOCKER] > 128 snaps the camera in and overshoots into the
    // poster kiss or the head bang. Bluepoint eases off the right stick, so it never can.
    // MC takes the locker's zoom speed from the right stick, so make its deflection out of R1's
    // pressure instead.
    std::atomic<uint8_t> gLockerPressure = 0;

    uint8_t LockerDeflection(uint8_t stick)
    {
        const float lean = gLockerPressure.load() * (0.55f / 128.0f);
        const int pressed = std::clamp(static_cast<int>(127.0f - lean * 128.0f), 0, 255);
        return static_cast<uint8_t>(PressureInputs::bSuppressAlternates
            ? pressed : std::min<int>(stick, pressed));
    }

    // MC's stand-ins for pressure, and the controls they borrow.
    uintptr_t gWeaponOverride = 0;   // raiden.c  -> the whole synthesised weapon pad
    uintptr_t gSprayPitch = 0;       // subject.c -> right-stick pitch, killed while spraying

    void SetAlternatesSuppressed(bool on)
    {
        if (gWeaponOverride == 0 || gSprayPitch == 0)
        {
            return;
        }
        static bool patched = false;
        if (on != patched)
        {
            patched = on;
            // je rel32 -> nop + jmp rel32: the displacement still ends where it did
            Memory::PatchBytes(gWeaponOverride, on ? "\x90\xE9" : "\x0F\x84", 2);
            Memory::PatchBytes(gSprayPitch, on ? "\xEB" : "\x74", 1);
        }
    }


    // MGS 3's stick clicks. Interrogate needs both its entry and its exit; L3's holster is the
    // emulator faking SQUARE, one site per edge.
    uintptr_t gCqcInterrogateEntry = 0;
    uintptr_t gCqcInterrogateExit = 0;
    uintptr_t gHolsterEdge = 0;
    uintptr_t gDrawEdge = 0;

    void SetStickActionsRestored(bool on)
    {
        if (gCqcInterrogateEntry == 0 || gCqcInterrogateExit == 0
            || gHolsterEdge == 0 || gDrawEdge == 0)
        {
            return;
        }
        static bool patched = false;
        if (on == patched)
        {
            return;
        }
        patched = on;

        // Eleven bytes either way so the je never moves; si keeps 0x100 for the store below.
        Memory::PatchBytes(gCqcInterrogateEntry,
            on ? "\x66\xBE\x00\x01\xF6\x87\xF0\x07\x00\x00\x02"
               : "\xBE\x00\x01\x00\x00\x85\xB7\xF0\x07\x00\x00", 11);
        Memory::PatchBytes(gCqcInterrogateExit, on ? "\x02\x00\x00\x00" : "\x00\x01\x00\x00", 4);

        Memory::PatchBytes(gHolsterEdge, on ? "\x00" : "\x02", 1);
        Memory::PatchBytes(gDrawEdge, on ? "\x00" : "\x02", 1);
    }



    // The scope's zoom rate, flat in MC and pressure/60 on PS2. In and out differ by register.
    uintptr_t gScopeZoomIn = 0;
    uintptr_t gScopeZoomOut = 0;

    void SetScopeAnalogue(bool on)
    {
        if (gScopeZoomIn == 0 || gScopeZoomOut == 0)
        {
            return;
        }
        static bool patched = false;
        if (on != patched)
        {
            patched = on;
            Memory::PatchBytes(gScopeZoomIn, on ? "\xEB" : "\x74", 1);    // je -> jmp
            Memory::PatchBytes(gScopeZoomOut, on ? "\xEB" : "\x74", 1);
        }
    }

    // The cutscene zoom, R1 on PS2 and R2 in MC. Both of MC's branches meet on one instruction
    // with the pressure in eax.
    uint8_t* gDirectPressure = nullptr;   // GV_PadDataDirect's array, the pad the demo camera reads

    void ApplyPressure(uint8_t* pressure)
    {
        if (!gHavePad.load())
        {
            SetScopeAnalogue(false);
            SetAlternatesSuppressed(false);
            RebindNegativeThought(false);
            return;
        }
        SetScopeAnalogue(true);
        SetAlternatesSuppressed(PressureInputs::bSuppressAlternates);
        RebindNegativeThought(true);

        uint8_t now[kSlots];
        ReadPad(now);
        gLockerPressure = now[kR1];
        for (size_t s = 0; s < kSlots; ++s)
        {
            // Write the zero too. The button bit outlives the pressure, so skipping it leaves
            // setup_pressure's 0xFF and every release reads as a full press.
            pressure[s] = now[s];
        }
    }

    // ---- MGS 3 ------------------------------------------------------------------------------
    // Same twelve slots in the same order, but uint16 at pad+0x14 with a 0x2C stride.
    uint16_t* gMgs3Pressure = nullptr;

    // Skipping the launcher leaves the action -> button tables zeroed, so everything resolves to
    // CROSS. Run the game's own init rather than seeding them by hand.
    using ControlConfigInit = void (*)();
    ControlConfigInit gControlConfigInit = nullptr;
    const int32_t* gControlMap = nullptr;

    void RepairControlMap()
    {
        static int attempts = 0;
        if (gControlConfigInit == nullptr || gControlMap == nullptr || attempts >= 8)
        {
            return;
        }
        const int32_t* map = gControlMap;
        bool allZero = true;
        for (size_t i = 0; i < 10; ++i)
        {
            allZero = allZero && map[i] == 0;
        }
        if (!allZero)
        {
            return;
        }
        ++attempts;
        gControlConfigInit();

        static bool told = false;
        if (!told)
        {
            told = true;
            spdlog::info("MGS 3: Pressure Inputs - Control map was unset; ran the game's own init.");
        }
    }

    // The gameplay pad's own bytes. The eight the mechanics read start at +0x0C and run
    // L2 R2 L1 R1 TRI CIR CRO SQU, in the pad's units, so no scaling.
    constexpr ptrdiff_t kPadPressureOffset = 0x0C;
    constexpr size_t kPadSlots = 8;
    // MGS3 builds its triggers from floats, but SXS reads them as fully held at rest.
    constexpr int kPadPressureSrc[kPadSlots] = { 10, 11, 8, 9, 4, 5, 6, 7 };
    uint8_t* gPadPressure = nullptr;
    uint8_t* gMgs3DemoPad = nullptr;    // GV_PadDataDirect, what the demo camera reads

    void SetKnifeAnalogue(bool on);

    void FillPadPressure()
    {
        if (!gHavePad.load())
        {
            SetKnifeAnalogue(false);
            return;
        }
        SetKnifeAnalogue(true);
        SetStickActionsRestored(PressureInputs::bSuppressAlternates);

        if (gPadPressure == nullptr)
        {
            return;
        }
        uint8_t now[kSlots];
        ReadPad(now);
        for (size_t i = 0; i < kPadSlots; ++i)
        {
            gPadPressure[i] = now[kPadPressureSrc[i]];
        }
    }

    // ---- CQC hard-squeeze slit ---------------------------------------------------------------
    // The slit test wants timer[CIRCLE] >= 1.0f and TRIANGLE, but that float is a hold timer the
    // player tick fills at 1.0f/fps. Feed it pressure and drop TRIANGLE.
    constexpr uint8_t kSlitPressure = 200;      // the PS2 value, its own `mov eax, 0xC8`
    uintptr_t gTriangleBranch = 0;
    const uint32_t* gAnalogStatus = nullptr;
    float* gHoldTimers = nullptr;               // the whole array; the game hands us the index

    void FeedSlitTest(SafetyHookContext& ctx)
    {
        const bool live = gHavePad.load();

        // Only drop TRIANGLE with a pad attached; without one the slot is still a hold timer.
        if (gTriangleBranch != 0)
        {
            static bool patched = false;
            if (live != patched)
            {
                patched = live;
                Memory::PatchBytes(gTriangleBranch, live ? "\x66\x90" : "\x74\x04", 2);
            }
        }
        if (!live || gHoldTimers == nullptr || gAnalogStatus == nullptr)
        {
            return;
        }

        // Accidental slits were a common complaint about the PS2 game. If it should be harder to
        // trigger, that goes here: raise kSlitPressure, or require a minimum before feeding at all.
        float squeeze = static_cast<float>(gCirclePressure.load()) / kSlitPressure;
        // Keep MC's TRIANGLE kill: hand it the full scale its timer would have reached. It is one
        // of the stand-ins, so it goes when they do and the squeeze is the only way in.
        if (!PressureInputs::bSuppressAlternates && (*gAnalogStatus & 0x1000) != 0)
        {
            squeeze = 1.0f;
        }
        // rcx is the slot the game resolved for this test.
        gHoldTimers[ctx.rcx] = std::min(squeeze, 1.0f);
    }

    // ---- Knife hard stab ---------------------------------------------------------------------
    // The PS2 test survives but is unreachable: with the analog flag clear it wants SQUARE >= 0xAA
    // held five frames. MC leaves the flag set, so flipping the je runs the game's own test.
    uintptr_t gKnifeBranch = 0;

    void SetKnifeAnalogue(bool on)
    {
        if (gKnifeBranch == 0)
        {
            return;
        }
        static bool patched = false;
        if (on != patched)
        {
            patched = on;
            Memory::PatchBytes(gKnifeBranch, on ? "\xEB" : "\x74", 1);   // je -> jmp
        }
    }

    // ---- Grenade throws ----------------------------------------------------------------------
    // ThrowGrenade() takes five levels off the weapon button, clamp((pressure - 140) / 28, 0, 4),
    // peak-held across the wind-up. MGS 3 throws at a fixed speed instead.
    std::atomic<uint8_t> gThrowPeak = 0;

    int ThrowLevel()
    {
        return std::clamp((static_cast<int>(gThrowPeak.load()) - 140) / 28, 0, 4);
    }

    // The release motion is picked after the button is released, so sampling there reads 0.
    void TrackThrowPeak(uintptr_t work)
    {
        const uint8_t state = *reinterpret_cast<uint8_t*>(work + 0x82);
        uint8_t now[kSlots];
        ReadPad(now);
        const uint8_t squeeze = now[kSquare];
        if (state == 25 || state == 26)         // winding up
        {
            if (squeeze > gThrowPeak.load())
            {
                gThrowPeak = squeeze;
            }
        }
        else if (state == 0xFF)
        {
            gThrowPeak = 0;
        }
    }

    // The projectile is known only by its contents, so a touch can land on another actor.
    __declspec(noinline) bool ReadFloat(uintptr_t address, float* out)
    {
        __try
        {
            *out = *reinterpret_cast<const float*>(address);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    __declspec(noinline) bool ReadTriple(uintptr_t address, float* out)
    {
        __try
        {
            const float* f = reinterpret_cast<const float*>(address);
            out[0] = f[0];
            out[1] = f[1];
            out[2] = f[2];
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    __declspec(noinline) bool WriteFloat(uintptr_t address, float value)
    {
        __try
        {
            *reinterpret_cast<float*>(address) = value;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // Velocity +0x70, speed +0x7C, duplicated at +0x220. Scaled once on the first tick, normalised
    // so level 4 matches stock distance.
    constexpr size_t kBullets = 8;
    std::atomic<uintptr_t> gBullets[kBullets] {};
    std::atomic<size_t> gBulletCount = 0;

    void ScaleThrownGrenade(uintptr_t work)
    {
        const size_t seen = gBulletCount.load();
        for (size_t i = 0; i < seen && i < kBullets; ++i)
        {
            if (gBullets[i].load() == work)
            {
                return;
            }
        }
        if (seen >= kBullets)
        {
            return;
        }

        float velocity[3] {};
        float speed = 0.0f;
        if (!ReadTriple(work + 0x70, velocity) || !ReadFloat(work + 0x7C, &speed))
        {
            return;
        }
        const float mag = std::sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1]
            + velocity[2] * velocity[2]);
        if (!(speed > 1.0f) || !(mag > 1.0f) || !std::isfinite(mag))
        {
            return;         // not a projectile, just another actor on this Act
        }
        gBullets[seen] = work;
        gBulletCount = seen + 1;

        const float level = static_cast<float>(ThrowLevel());
        const float horizontal = (1.0f + level) / 5.0f;
        const float vertical = (1.0f + 0.13f * level) / 1.52f;    // keeps the arc, shortens the fall

        // Only write the second copy if it matches the first, in case the actor is smaller.
        float second[3] {};
        bool haveSecond = ReadTriple(work + 0x220, second);
        if (haveSecond)
        {
            haveSecond = std::fabs(second[0] - velocity[0]) < 0.01f
                && std::fabs(second[1] - velocity[1]) < 0.01f
                && std::fabs(second[2] - velocity[2]) < 0.01f;
        }

        WriteFloat(work + 0x70, velocity[0] * horizontal);
        WriteFloat(work + 0x74, velocity[1] * vertical);
        WriteFloat(work + 0x78, velocity[2] * horizontal);
        WriteFloat(work + 0x7C, speed * horizontal);
        if (haveSecond)
        {
            WriteFloat(work + 0x220, second[0] * horizontal);
            WriteFloat(work + 0x224, second[1] * vertical);
            WriteFloat(work + 0x228, second[2] * horizontal);

            float secondSpeed = 0.0f;
            if (ReadFloat(work + 0x22C, &secondSpeed) && secondSpeed > 1.0f)
            {
                WriteFloat(work + 0x22C, secondSpeed * horizontal);
            }
        }
    }

    void InitializeMGS3()
    {
        // The last store gives the array via its own displacement. Land past the memcmp after it.
        if (uint8_t* address = Memory::PatternScan(baseModule, "0F B6 C3 66 89 05 ?? ?? ?? ?? E8",
            "MGS 3: Pressure Inputs - Pad Writer | libgv\\pad.c -> setup_pressure()"))
        {
            const int32_t displacement = *reinterpret_cast<int32_t*>(address + 6);
            gMgs3Pressure = reinterpret_cast<uint16_t*>(address + 10 + displacement) - 11;

            static SafetyHookMid padHook {};
            padHook = safetyhook::create_mid(address + 15, [](SafetyHookContext&)
            {
                if (!gHavePad.load() || gMgs3Pressure == nullptr)
                {
                    return;
                }
                uint8_t now[kSlots];
                ReadPad(now);
                for (size_t s = 0; s < kSlots; ++s)
                {
                    gMgs3Pressure[s] = now[s];
                }
            });
            LOG_HOOK(padHook, "MGS 3: Pressure Inputs - Pad Writer")

            if (g_Logging.bVerboseLogging)
            {
                spdlog::info("MGS 3: Pressure Inputs - GV_PAD.pressure at {:s}+{:X}",
                    sExeName.c_str(), reinterpret_cast<uintptr_t>(gMgs3Pressure)
                        - reinterpret_cast<uintptr_t>(baseModule));
            }
        }

        // Anchored on the prologue, so the entry point is the match and not a back-offset.
        gControlConfigInit = reinterpret_cast<ControlConfigInit>(Memory::PatternScan(baseModule,
            "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 54 41 55 41 56 41 57 48 83 EC 30 "
            "45 33 F6 41 8B FE 41 8B DE 8D 4B 2B",
            "MGS 3: Pressure Inputs - Control Config Init"));

        // The copy is unrolled and repeats for a second table; ours reads .data and writes it back
        // less than 0x1000 higher.
        for (uint8_t* match : Memory::FindMultiplePatternMatches(baseModule,
            "CC CC CC CC CC 8B 05 ?? ?? ?? ?? 89 05 ?? ?? ?? ?? 8B 05 ?? ?? ?? ?? 89 05"))
        {
            uint8_t* copy = match + 5;
            const uint8_t* source = copy + 6 + *reinterpret_cast<int32_t*>(copy + 2);
            uint8_t* destination = copy + 12 + *reinterpret_cast<int32_t*>(copy + 8);
            const ptrdiff_t delta = destination - source;
            if (delta > 0 && delta <= 0x1000)
            {
                gControlMap = reinterpret_cast<const int32_t*>(destination);
                break;
            }
        }
        if (gControlMap == nullptr)
        {
            spdlog::error("MGS 3: Pressure Inputs - Control map not resolved; skipping the repair.");
        }

        // `test [pad], eax` on the gameplay pad - the pressure bytes sit a fixed 0x0C into it.
        if (uint8_t* address = Memory::PatternScan(baseModule,
            "8B 87 C4 00 00 00 85 05 ?? ?? ?? ?? 74 32 33 C9",
            "MGS 3: Pressure Inputs - Gameplay Pad"))
        {
            gPadPressure = address + 12 + *reinterpret_cast<int32_t*>(address + 8)
                + kPadPressureOffset;
            if (g_Logging.bVerboseLogging)
            {
                spdlog::info("MGS 3: Pressure Inputs - gameplay pad at {:s}+{:X}",
                    sExeName.c_str(), reinterpret_cast<uintptr_t>(gPadPressure)
                        - kPadPressureOffset - reinterpret_cast<uintptr_t>(baseModule));
            }
        }

        // End of the input tick, past the builder's own copy, so the bytes we fill are the ones
        // gameplay goes on to read.
        if (uint8_t* address = Memory::PatternScan(baseModule,
            "85 C0 74 11 8B CB E8 ?? ?? ?? ?? 48 83 C4 20 5B",
            "MGS 3: Pressure Inputs - Input Tick"))
        {
            static SafetyHookMid tickHook {};
            tickHook = safetyhook::create_mid(address + 11, [](SafetyHookContext&)
            {
                RepairControlMap();
                FillPadPressure();
            });
            LOG_HOOK(tickHook, "MGS 3: Pressure Inputs - Input Tick")
        }

        // The cutscene zoom: TRIANGLE on PS2, R2 in MC, chosen by the cmov above. It reads a pad
        // we do not fill.
        if (uint8_t* address = Memory::PatternScan(baseModule,
            "48 8D 0D ?? ?? ?? ?? 45 0F 57 E4 E8",
            "MGS 3: Pressure Inputs - Demo Camera Pad | demo\\cam_act.c -> Act()"))
        {
            gMgs3DemoPad = address + 7 + *reinterpret_cast<int32_t*>(address + 3);
        }

        MAKE_HOOK_MID(baseModule, "33 DB 83 F8 1E 7E ?? 83 C0 E2 69 C8 FF 00 00 00",
            "MGS 3: Pressure Inputs - Demo Zoom Amount | demo\\cam_act.c -> Act()",
        {
            if (gHavePad.load() && gMgs3DemoPad != nullptr)
            {
                uint8_t now[kSlots];
                ReadPad(now);
                // The getter only reports a button that is held, so mirror that.
                const uint32_t status = *reinterpret_cast<const uint32_t*>(gMgs3DemoPad);
                const uint8_t tri = (status & 0x1000) ? now[kTriangle] : 0;
                const uint8_t r2 = (status & 0x0200) ? now[kR2] : 0;
                ctx.rax = PressureInputs::bSuppressAlternates ? tri : std::max(tri, r2);
            }
        });

        // Both sit in the one input handler; the mask is the imm32 the press is tested against.
        // Wildcarded imm so the scan still matches once we have patched it.
        if (uint8_t* address = Memory::PatternScan(baseModule,
            "BE ?? 01 00 00 85 B7 F0 07 00 00",
            "MGS 3: Pressure Inputs - Interrogate Entry"))
        {
            gCqcInterrogateEntry = reinterpret_cast<uintptr_t>(address);
        }

        if (uint8_t* address = Memory::PatternScan(baseModule,
            "F7 87 EC 07 00 00 ?? 01 00 00",
            "MGS 3: Pressure Inputs - Interrogate Exit"))
        {
            gCqcInterrogateExit = reinterpret_cast<uintptr_t>(address) + 6;
        }

        // L3 faking a SQUARE press: one site per edge, both needed.
        if (uint8_t* address = Memory::PatternScan(baseModule,
            "F6 05 ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? F7 05 ?? ?? ?? ?? 00 80 00 00",
            "MGS 3: Pressure Inputs - Holster Edge"))
        {
            gHolsterEdge = reinterpret_cast<uintptr_t>(address) + 6;
        }

        if (uint8_t* address = Memory::PatternScan(baseModule,
            "B8 01 00 00 00 83 E1 ?? 41 BF 07 00 00 00 0F 45 F0",
            "MGS 3: Pressure Inputs - Draw Edge"))
        {
            gDrawEdge = reinterpret_cast<uintptr_t>(address) + 7;
        }

        // Anchored on the comiss: -0x0A is the movss that loads the timer, +19 the TRIANGLE je.
        if (uint8_t* address = Memory::PatternScan(baseModule,
            "0F 2F 05 ?? ?? ?? ?? 72 10 F7 05 ?? ?? ?? ?? 00 10 00 00 74 04 4C 89 63 58",
            "MGS 3: Pressure Inputs - CQC Slit Test"))
        {
            gTriangleBranch = reinterpret_cast<uintptr_t>(address) + 19;

            // The movss is [r13 + rcx*4 + timers], so its disp32 gives the array.
            uint8_t* load = address - 0x0A;
            gHoldTimers = reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(baseModule)
                + *reinterpret_cast<uint32_t*>(load + 6));
            gAnalogStatus = reinterpret_cast<const uint32_t*>(
                address + 19 + *reinterpret_cast<int32_t*>(address + 11));

            static SafetyHookMid slitHook {};
            slitHook = safetyhook::create_mid(load,
                [](SafetyHookContext& ctx) { FeedSlitTest(ctx); });
            LOG_HOOK(slitHook, "MGS 3: Pressure Inputs - CQC Slit Test")
        }

        if (uint8_t* address = Memory::PatternScan(baseModule,
            "83 3D ?? ?? ?? ?? 00 48 8B DA 48 8B F9 74 12 83 3D ?? ?? ?? ?? 00",
            "MGS 3: Pressure Inputs - Knife Hard Stab"))
        {
            gKnifeBranch = reinterpret_cast<uintptr_t>(address) + 13;
        }

        // Throwables get a made-up SQUARE pressure: 69, or 70 once held for 0.7 s. Keep the
        // timer from ever getting there, and put the real pressure where the 69 goes.
        if (uint8_t* address = Memory::PatternScan(baseModule,
            "0F 2F 84 82 ?? ?? ?? ?? 76 ?? B8",
            "MGS 3: Pressure Inputs - Throwable Hold Timer | player pad, the 0.7 s hold that stamps 70 | (NewPlayer() -> PL_PluginStart() -> PL_PLG_PadPlugin() -> Act()"))
        {
            static SafetyHookMid timerHook {};
            timerHook = safetyhook::create_mid(address, [](SafetyHookContext& ctx)
            {
                if (gHavePad.load())
                {
                    ctx.xmm0.f32[0] = std::numeric_limits<float>::max();
                }
            });
            LOG_HOOK(timerHook, "MGS 3: Pressure Inputs - Throwable Hold Timer | player pad, the 0.7 s hold that stamps 70")
        }
        if (uint8_t* address = Memory::PatternScan(baseModule,
            "89 05 ?? ?? ?? ?? 44 8D 78",
            "MGS 3: Pressure Inputs - Throwable Pressure | player pad, the SQUARE stamp for throwables | (NewPlayer() -> PL_PluginStart() -> PL_PLG_PadPlugin() -> Act()"))
        {
            static SafetyHookMid stampHook {};
            stampHook = safetyhook::create_mid(address, [](SafetyHookContext& ctx)
            {
                if (gHavePad.load())
                {
                    uint8_t now[kSlots];
                    ReadPad(now);
                    ctx.rdi = now[kSquare];
                }
            });
            LOG_HOOK(stampHook, "MGS 3: Pressure Inputs - Throwable Pressure | player pad, the SQUARE stamp for throwables")
        }

        // The weapon state machine, for the wind-up peak.
        if (uint8_t* address = Memory::PatternScan(baseModule,
            "56 57 41 55 41 57 48 81 EC A8 00 00 00 48 8B FA 48 8B F1",
            "MGS 3: Pressure Inputs - Weapon State"))
        {
            static SafetyHookMid weaponHook {};
            weaponHook = safetyhook::create_mid(address - 1,
                [](SafetyHookContext& ctx) { TrackThrowPeak(ctx.rdx); });
            LOG_HOOK(weaponHook, "MGS 3: Pressure Inputs - Weapon State")
        }

        // One of two Acts a throw registers under. It has a byte-identical twin, so the pattern
        // runs past the branch that separates them.
        MAKE_HOOK_MID(baseModule,
            "48 89 5C 24 20 57 48 81 EC 80 00 00 00 0F 29 74 24 70 48 8B 05 ?? ?? ?? ?? "
            "48 33 C4 48 89 44 24 60 81 A1 A8 03 00 00 FF 7F FF FF "
            "BA FF FF FF FF 48 8B D9 8D 4A 08 E8 ?? ?? ?? ?? 8B 93 A8 03 00 00 8B CA 83 C9 02 "
            "83 E2 FD 85 C0 0F 45 D1 89 93 A8 03 00 00 F6 C2 04 0F 84 29 01 00 00",
            "MGS 3: Pressure Inputs - Projectile Act",
        {
            ScaleThrownGrenade(ctx.rcx);
        });

        MAKE_HOOK_MID(baseModule, "48 89 5C 24 08 57 48 83 EC 30 B8 FF 7F 00 00 48 8B D9 66 21",
            "MGS 3: Pressure Inputs - Projectile Act 2",
        {
            ScaleThrownGrenade(ctx.rcx);
        });
    }

    void InitializeMGS2()
    {
        // Straight after the pad writer calls setup_pressure(), pad in r9.
        MAKE_HOOK_MID(baseModule, "41 F7 41 24 2F 01 00 00 74 0C 49",
            "MGS 2: Pressure Inputs - Pad Writer | libgv\\pad.c -> setup_pressure()",
        {
            gDirectPressure = reinterpret_cast<uint8_t*>(ctx.r9 + 0x18);
            ApplyPressure(gDirectPressure);
        });

        // MSVC inlined pad.c's second UpdatePad, so the hook above only sees GV_PadDataDirect,
        // which only the demo camera reads. Pad in rbx here.
        MAKE_HOOK_MID(baseModule, "41 83 E2 DF 44 89 15",
            "MGS 2: Pressure Inputs - Gameplay Pad | libgv\\pad.c -> GV_PadData",
        {
            // A pad demo replays its own recorded pressure into this pad, and the controller it was
            // recorded on is not the one on the desk. Leave the recording alone.
            if (g_GameVars.Get_GM_GameStatus() & MGS2_StatusFlags::STATE_PAD_DEMO)
            {
                return;
            }
            ApplyPressure(reinterpret_cast<uint8_t*>(ctx.rbx + 0x18));
        });

        // The right stick's deflection, on its way to the locker's speed choice.
        MAKE_HOOK_MID(baseModule,
            "66 0F 6E C0 0F 5B C0 F3 0F 5C C8 F3 0F 5C 0D ?? ?? ?? ?? F3 0F 59 0D",
            "MGS 2: Pressure Inputs - Locker Lean | plugin\\locker2.c",
        {
            if (gHavePad.load())
            {
                ctx.rax = LockerDeflection(static_cast<uint8_t>(ctx.rax));
            }
        });

        if (uint8_t* address = Memory::PatternScan(baseModule,
            "44 39 25 ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 0F 29 BC 24 10 01 00 00 48 8B CF",
            "MGS 2: Pressure Inputs - Weapon Pad Override | raiden\\raiden.c"))
        {
            gWeaponOverride = reinterpret_cast<uintptr_t>(address) + 7;
        }

        // The je is a few instructions after its cmp; the flags survive the stick reads.
        if (uint8_t* address = Memory::PatternScan(baseModule,
            "F3 0F 5C F9 F3 0F 5C F1 74 0C 83 BD ?? ?? ?? ?? 0E 75 03 0F 57 F6",
            "MGS 2: Pressure Inputs - Spray Camera Pitch | raiden\\subject.c"))
        {
            gSprayPitch = reinterpret_cast<uintptr_t>(address) + 8;
        }

        // PressureToZoomIn/OutSpeed, inlined into the zoom camera's Act(). Identical but for the
        // register the speed lands in, so the pattern runs to the mulss that separates them.
        {
            constexpr const char* kZoom =
                "F3 0F 2C C8 74 13 85 C9 74 0A F3 0F 10 0D ?? ?? ?? ?? EB 25 0F 57 C9 EB 20 "
                "83 F9 3C 7C 14 B8 89 88 88 88 F7 E9 8D 3C 11 C1 FF 05 8B C7 C1 E8 1F 03 F8 "
                "66 0F 6E CF 0F 5B C9 F3 41 0F 59 ";
            if (uint8_t* address = Memory::PatternScan(baseModule, (std::string(kZoom) + "C9").c_str(),
                "MGS 2: Pressure Inputs - Scope Zoom In | etc\\zoomcam.c -> Act()"))
            {
                gScopeZoomIn = reinterpret_cast<uintptr_t>(address) + 4;
            }
            if (uint8_t* address = Memory::PatternScan(baseModule, (std::string(kZoom) + "C8").c_str(),
                "MGS 2: Pressure Inputs - Scope Zoom Out | etc\\zoomcam.c -> Act()"))
            {
                gScopeZoomOut = reinterpret_cast<uintptr_t>(address) + 4;
            }
        }

        // Both of Bluepoint's zoom reads, the switch case and the R1-gated one. The byte we move
        // is the low half of the RIP displacement, so scan to the instruction and step in 3.
        // Where both of Bluepoint's zoom branches meet, with the pressure they chose in eax.
        MAKE_HOOK_MID(baseModule, "66 44 0F 6E C0 49 8B 46 58 45 0F 5B C0",
            "MGS 2: Pressure Inputs - Demo Zoom Amount | demo\\cam_act.c -> Act()",
        {
            if (gHavePad.load() && gDirectPressure != nullptr)
            {
                const uint8_t r1 = gDirectPressure[kR1];
                const uint8_t r2 = gDirectPressure[kR2];
                ctx.rax = PressureInputs::bSuppressAlternates ? r1 : std::max(r1, r2);
            }
        });

        gThoughtGate = reinterpret_cast<uintptr_t>(Memory::PatternScan(baseModule,
            "48 8B 43 58 F6 40 08 0A",
            "MGS 2: Pressure Inputs - Thought Gate | codec\\cdc_mind.c"));
        gThoughtPick = reinterpret_cast<uintptr_t>(Memory::PatternScan(baseModule,
            "41 84 C9 74 ?? 44 0F B6 40 23",
            "MGS 2: Pressure Inputs - Thought Select | codec\\cdc_mind.c"));
        gThoughtHold = reinterpret_cast<uintptr_t>(Memory::PatternScan(baseModule,
            "74 ?? 0F B6 40 21 EB ?? 0F B6 40 23",
            "MGS 2: Pressure Inputs - Thought Hold | codec\\cdc_mind.c"));

        // The mask, adjusted at the store: the register that builds it is also the codec state.
        MAKE_HOOK_MID(baseModule, "48 89 83 ?? ?? ?? ?? 89 8B ?? ?? ?? ?? EB",
            "MGS 2: Pressure Inputs - Thought Held Mask | codec\\cdc_mind.c",
        {
            if (gHavePad.load() && gThoughtRebound.load() && ctx.rax == 2)
            {
                ctx.rax = PressureInputs::bSuppressAlternates ? 4 : 6;    // L1, or L1|R2
            }
        });

        // The peak tracker reads the slot the mask implies, so feed it the same value.
        MAKE_HOOK_MID(baseModule, "3B 8B ?? ?? ?? ?? 7E ?? 89 8B",
            "MGS 2: Pressure Inputs - Thought Peak | codec\\cdc_mind.c",
        {
            if (gHavePad.load() && gDirectPressure != nullptr && gThoughtRebound.load()
                && (*reinterpret_cast<const uint32_t*>(ctx.rbx + 0xb0) & 8) == 0)
            {
                ctx.rcx = PressureInputs::bSuppressAlternates
                    ? gDirectPressure[kL1]
                    : std::max(gDirectPressure[kL1], gDirectPressure[kR2]);
            }
        });

        // With both buttons live the game still reads one slot, so give it the harder press.
        MAKE_HOOK_MID(baseModule, "44 0F B6 40 23 41 3B D0",
            "MGS 2: Pressure Inputs - Thought Pressure | codec\\cdc_mind.c",
        {
            // The load would overwrite r8, so redirect what it reads. rax dies two on.
            if (gHavePad.load() && gDirectPressure != nullptr && gThoughtRebound.load())
            {
                gThoughtScratch = PressureInputs::bSuppressAlternates
                    ? gDirectPressure[kL1]
                    : std::max(gDirectPressure[kL1], gDirectPressure[kR2]);
                ctx.rax = reinterpret_cast<uintptr_t>(&gThoughtScratch) - 0x23;
            }
        });


        // WP_ColdSpray clamps the right stick to 0..1 and uses that one float for both the fire
        // gate (0 strips PL_PAD_WEAPON) and the jet thickness, so the button does nothing. Take
        // whichever is further on so the stick keeps working - speedruns are built around it.
        MAKE_HOOK_MID(baseModule,
            "8B 15 ?? ?? ?? ?? 85 C0 74 ?? 0F BA E2",
            "MGS 2: Pressure Inputs - Coolant Spray | skoba\\weapon\\spray.c",
        {
            if (gHavePad.load())
            {
                uint8_t now[kSlots];
                ReadPad(now);
                const float squeeze = static_cast<float>(now[kSquare]) / 255.0f;
                ctx.xmm7.f32[0] = std::max(ctx.xmm7.f32[0], squeeze);
            }
        });

        // The chosen button shares a register with the next step value, so fix it at the store.
        MAKE_HOOK_MID(baseModule, "48 89 83 B0 00 00 00 89 8B A4 00 00 00",
            "MGS 2: Pressure Inputs - Thought Button | codec\\cdc_mind.c",
        {
            if (gThoughtRebound.load() && ctx.rax == 2)
            {
                ctx.rax = 4;
            }
        });
    }
}

void PressureInputs::ReadPad(uint8_t (&out)[kPadSlots])
{
    static_assert(kPadSlots == kSlots, "pad slot count out of step with libgv");
    const std::lock_guard<std::mutex> guard(gPressureLock);
    memcpy(out, gPressure, sizeof(out));
}

bool PressureInputs::HavePad()
{
    return gHavePad.load();
}

int PressureInputs::Ds3DeviceMode(std::wstring& path)
{
    if (!gHavePad.load())
    {
        return -1;
    }
    const std::lock_guard<std::mutex> guard(gPressureLock);
    if (gDeviceMode < 0)
    {
        return -1;
    }
    path = gDevicePath;
    return gDeviceMode;
}

void PressureInputs::Initialize()
{
    if (!bEnabled)
    {
        return;
    }

    if (eGameType & MGS3)
    {
        InitializeMGS3();
    }
    else if (eGameType & MGS2)
    {
        InitializeMGS2();
    }
    else
    {
        return;
    }

    if (HANDLE reader = CreateThread(nullptr, 0, ReaderThread, nullptr, 0, nullptr))
    {
        CloseHandle(reader);
    }
}
