#pragma once

// MGS2: the game correctly drives the PS2 reverb (HALL mode, per-room depth) into an XAudio2
// reverb submix, but at the PS2 depth values the XAudio2 reverb is much quieter than the
// PS2's SPU2 hall, so rooms sound nearly dry. Scale the wet volume to compensate.
namespace FixReverbWetLevel
{
    inline bool bEnabled = true;

    // Multiplier on the reverb submix wet volume. 1.0 = vanilla. Chosen by A/B against PS2.
    inline float fWetVolumeScale = 1.4f;

    void Initialize();
}

