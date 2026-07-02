#include "stdafx.h"

#include "mgs2_ray_photo_voice.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "logging.hpp"

namespace
{
    constexpr uint32_t kSnakeYes01 = 0x287;
    constexpr uint32_t kSnakeYes02 = 0x288;

    constexpr std::array<uint8_t, 25> kRayPhotoVoiceLine = {
        0x8D, 0x32, 0x6D, 0x15, 0xC9, 0x2B, 0x08, 0x04, 0x06, 0xF1, 0xCB, 0x88,
        0x54, 0x73, 0x01, 0x88, 0x02, 0x53, 0x76, 0x02, 0x3F, 0x52, 0x70, 0xE1, 0x00
    };

    char*** g_GclCommandLineP = nullptr;

    bool MatchesBytes(const uint8_t* address, const std::array<uint8_t, kRayPhotoVoiceLine.size()>& bytes)
    {
        return Memory::IsReadable(address, bytes.size()) && std::memcmp(address, bytes.data(), bytes.size()) == 0;
    }

    char* GetCurrentCommandLine()
    {
        if (!g_GclCommandLineP || !Memory::IsReadable(g_GclCommandLineP, sizeof(*g_GclCommandLineP)))
        {
            return nullptr;
        }

        char** commandLineP = *g_GclCommandLineP;
        if (!commandLineP)
        {
            return nullptr;
        }

        char** currentCommandLine = commandLineP - 1;
        if (!Memory::IsReadable(currentCommandLine, sizeof(*currentCommandLine)))
        {
            return nullptr;
        }

        return *currentCommandLine;
    }

    bool IsRayPhotoVoiceLine()
    {
        const char* commandLine = GetCurrentCommandLine();
        if (!commandLine)
        {
            return false;
        }

        const auto* commandLineBytes = reinterpret_cast<const uint8_t*>(commandLine);
        return MatchesBytes(commandLineBytes - 12, kRayPhotoVoiceLine) || MatchesBytes(commandLineBytes - 8, kRayPhotoVoiceLine);
    }

    uint32_t PickSnakeYesVoice()
    {
        static std::mt19937 rng(std::random_device {}());
        std::uniform_int_distribution<uint32_t> dist(0, 1);
        return dist(rng) == 0 ? kSnakeYes01 : kSnakeYes02;
    }
}

void MGS2RayPhotoVoice::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    constexpr char prefix[] = "MGS2: RAY Photo Voice";

    uint8_t* gclGetOption = Memory::PatternScan(baseModule,
        "40 53 48 83 EC 20 48 8B 05 ?? ?? ?? ?? 4C 8D 44 24 ?? 0F BE D9 48 8D 54 24 ?? 48 8B 48 F8 E8",
        prefix);
    if (!gclGetOption)
    {
        return;
    }

    g_GclCommandLineP = reinterpret_cast<char***>(Memory::GetRipRelativeAddress(gclGetOption + 6, 3, 7));

    uint8_t* seSetVolPan = Memory::PatternScan(baseModule,
        "E8 ?? ?? ?? ?? BE 3F 00 00 00 B1 76 8B D6 8B F8 E8 ?? ?? ?? ?? 8D 56 E1 B1 70 48 8B D8 E8",
        prefix);
    if (!seSetVolPan)
    {
        return;
    }

    static SafetyHookMid rayPhotoVoiceHook {};
    rayPhotoVoiceHook = safetyhook::create_mid(seSetVolPan + 0x10,
        [](SafetyHookContext& ctx)
        {
            if (ctx.rdi == kSnakeYes02 && g_GameVars.IsStage(MGS2Stages::W04C) && IsRayPhotoVoiceLine())
            {
                ctx.rdi = PickSnakeYesVoice();
            }
        });
    LOG_HOOK(rayPhotoVoiceHook, prefix)
}
