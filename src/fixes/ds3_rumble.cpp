#include "stdafx.h"
#include "ds3_rumble.hpp"
#include "pressure_inputs.hpp"

#include "common.hpp"
#include "logging.hpp"

#include <hidsdi.h>
#include <hidpi.h>
#pragma comment(lib, "hid.lib")

// The MC only rumbles through ISteamInput, which cannot see a DsHidMini DualShock 3; tap the
// engine's motor values and drive the pad directly.
namespace
{
    // High byte small motor 0/1, low byte big motor 0-255.
    std::atomic<uint16_t> gMotors = 0;
    HANDLE gWake = nullptr;
    SafetyHookMid gSinkHook {};

    HANDLE gDev = nullptr;
    USHORT gReportLen = 0;
    int gMode = -1;     // kProfiles index: 0 SXS, 1 SDF, 2 native

    // The DS3's big motor stalls below ~1/3 throttle; lift nonzero levels onto the band that spins.
    constexpr int kBigMotorFloor = 96;

    uint8_t BigMotorCurve(uint8_t value)
    {
        const int scaled = std::min(255, value * Ds3Rumble::iStrength / 100);
        return scaled ? static_cast<uint8_t>(kBigMotorFloor + scaled * (255 - kBigMotorFloor) / 255) : 0;
    }

    void CloseDevice()
    {
        if (gDev != nullptr)
        {
            CloseHandle(gDev);
            gDev = nullptr;
        }
    }

    bool EnsureDevice()
    {
        std::wstring path;
        const int mode = PressureInputs::Ds3DeviceMode(path);
        if (mode < 0)
        {
            CloseDevice();
            return false;
        }
        if (gDev != nullptr && mode == gMode)
        {
            return true;
        }
        CloseDevice();
        gMode = mode;
        if (mode == 2)
        {
            static bool warned = false;
            if (!warned)
            {
                warned = true;
                spdlog::warn("DS3 Rumble - the native report mode has no rumble path; use DsHidMini SDF or SXS.");
            }
            return false;
        }

        gDev = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (gDev == INVALID_HANDLE_VALUE)
        {
            gDev = nullptr;
            return false;
        }

        // hidclass wants full-length writes; SXS caps are unreadable, so use the driver's fixed sizes.
        PHIDP_PREPARSED_DATA pp = nullptr;
        HIDP_CAPS caps {};
        USHORT length = 0;
        if (HidD_GetPreparsedData(gDev, &pp))
        {
            if (HidP_GetCaps(pp, &caps) == HIDP_STATUS_SUCCESS && caps.OutputReportByteLength <= 64)
            {
                length = caps.OutputReportByteLength;
            }
            HidD_FreePreparsedData(pp);
        }
        gReportLen = length ? length : (mode == 0 ? 49 : 19);
        spdlog::info("DS3 Rumble - write handle open, {} mode, {}-byte output reports.",
            mode == 0 ? "SXS" : "SDF", gReportLen);
        return true;
    }

    bool WriteReport(const uint8_t* src, size_t n)
    {
        uint8_t buf[64] {};
        memcpy(buf, src, n);
        DWORD wrote = 0;
        return WriteFile(gDev, buf, gReportLen, &wrote, nullptr) && wrote == gReportLen;
    }

    // SDF is PID force feedback: a constant force per motor (block 1 big, 2 small), Start flushes.
    bool SendSdf(uint8_t smallMotor, uint8_t bigMotor)
    {
        if (smallMotor == 0 && bigMotor == 0)
        {
            const uint8_t stop[2] = { 0x19, 0x03 };
            return WriteReport(stop, sizeof(stop));
        }
        const auto lg = static_cast<uint16_t>(BigMotorCurve(bigMotor) * 10000 / 255);
        const uint8_t left[4] = { 0x14, 0x01, static_cast<uint8_t>(lg & 0xFF), static_cast<uint8_t>(lg >> 8) };
        const uint16_t sm = smallMotor ? 10000 : 0;
        const uint8_t right[4] = { 0x14, 0x02, static_cast<uint8_t>(sm & 0xFF), static_cast<uint8_t>(sm >> 8) };
        const uint8_t start[4] = { 0x18, 0x01, 0x01, 0x01 };
        return WriteReport(left, sizeof(left)) && WriteReport(right, sizeof(right)) &&
               WriteReport(start, sizeof(start));
    }

    // SXS takes the raw 49-byte sixaxis.sys packet; it owns the LED too, so that rides along.
    bool SendSxs(uint8_t smallMotor, uint8_t bigMotor)
    {
        uint8_t r[49] {};
        r[1] = 0x02;
        r[5] = 0xFF;
        r[6] = smallMotor ? 0x01 : 0x00;
        r[7] = 0xFF;
        r[8] = BigMotorCurve(bigMotor);
        r[13] = 0x02;
        constexpr uint8_t led[5] = { 0xFF, 0x27, 0x10, 0x00, 0x32 };
        for (int i = 0; i < 4; ++i)
        {
            memcpy(&r[14 + 5 * i], led, sizeof(led));
        }
        return WriteReport(r, sizeof(r));
    }

    // The pad holds its last state, so only changes are sent.
    DWORD WINAPI WriterThread(LPVOID)
    {
        uint16_t lastSent = 0;
        for (;;)
        {
            WaitForSingleObject(gWake, 1000);
            const uint16_t want = gMotors.load(std::memory_order_relaxed);
            if (want == lastSent)
            {
                continue;
            }
            if (!EnsureDevice())
            {
                continue;
            }
            const auto smallMotor = static_cast<uint8_t>(want >> 8);
            const auto bigMotor = static_cast<uint8_t>(want & 0xFF);
            if (gMode == 0 ? SendSxs(smallMotor, bigMotor) : SendSdf(smallMotor, bigMotor))
            {
                lastSent = want;
            }
            else
            {
                CloseDevice();
            }
        }
    }

    void Publish(uint16_t state)
    {
        if (gMotors.exchange(state, std::memory_order_relaxed) != state)
        {
            SetEvent(gWake);
        }
    }
}

void Ds3Rumble::Shutdown()
{
    // Quitting mid-rumble buzzes forever unless the exit path sends one silence.
    if (gDev != nullptr)
    {
        gMode == 0 ? SendSxs(0, 0) : SendSdf(0, 0);
    }
}

void Ds3Rumble::Initialize()
{
    if (!bEnabled || !(eGameType & (MGS2 | MGS3)))
    {
        return;
    }
    if (!PressureInputs::bEnabled)
    {
        spdlog::info("DS3 Rumble - needs Pressure Sensitive Facebuttons for the DualShock 3.");
        return;
    }
    gWake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (gWake == nullptr)
    {
        return;
    }

    if (eGameType & MGS2)
    {
        // pad_send_vibration's movzx pair before the Steam bridge call: al = small, cl = big.
        uint8_t* address = Memory::PatternScan(baseModule,
            "84 D2 75 ?? 84 C9",
            "MGS 2: DS3 Rumble - Vibration Sink | libgv\\pad.c -> pad_send_vibration()");
        if (address == nullptr)
        {
            return;
        }
        gSinkHook = safetyhook::create_mid(address + 28, [](SafetyHookContext& ctx)
        {
            Publish(static_cast<uint16_t>(((ctx.rax & 0xFF) << 8) | (ctx.rcx & 0xFF)));
        });
        LOG_HOOK(gSinkHook, "MGS 2: DS3 Rumble - Vibration Sink")
    }
    else
    {
        // The logical pad's motor bytes; the engine's consume sits behind a Steam-visibility
        // gate that never passes with the pad hidden, so read and clear them here.
        uint8_t* address = Memory::PatternScan(baseModule,
            "85 FF 74 ?? 44 39 7B",
            "MGS 3: DS3 Rumble - Vibration Sink | input poll pad loop");
        if (address == nullptr)
        {
            return;
        }
        gSinkHook = safetyhook::create_mid(address, [](SafetyHookContext& ctx)
        {
            if ((ctx.rsi & 0xFFFFFFFF) != 0)    // player pad only
            {
                return;
            }
            auto* pad = reinterpret_cast<uint8_t*>(ctx.rbx);
            const auto state = static_cast<uint16_t>((pad[0x38] ? 0x100 : 0) | pad[0x39]);
            if (PressureInputs::HavePad())
            {
                *reinterpret_cast<uint16_t*>(pad + 0x38) = 0;
            }
            Publish(state);
        });
        LOG_HOOK(gSinkHook, "MGS 3: DS3 Rumble - Vibration Sink")
    }
    if (!gSinkHook)
    {
        return;
    }

    if (HANDLE thread = CreateThread(nullptr, 0, WriterThread, nullptr, 0, nullptr))
    {
        CloseHandle(thread);
    }
}
