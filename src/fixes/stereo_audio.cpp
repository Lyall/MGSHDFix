#include "common.hpp"
#include "spdlog/spdlog.h"
#include "stereo_audio.hpp"


void StereoAudioFix::Initialize() const
{
    if (!isEnabled || !(eGameType & (MGS2 | MGS3 | MG)))
    {
        return;
    }
    uint8_t* MGS2_MGS3_StereoAudioScanResult = Memory::PatternScan(baseModule, (eGameType & MGS2) ? "C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ?? 48 8D 8E" : "C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ?? 49 8D 8E", "MGS 2 | MGS 3: Stereo Audio", NULL, NULL);
    if (!MGS2_MGS3_StereoAudioScanResult)
    {
        return;
    }

    Memory::PatchBytes((uintptr_t)MGS2_MGS3_StereoAudioScanResult, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 20);
    spdlog::info("MGS 2 | MGS 3: Stereo Audio: Audio output forced to stereo.");
}

