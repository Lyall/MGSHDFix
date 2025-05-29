#pragma once

class StereoAudioFix
{
public:
    bool isEnabled;
    void Initialize() const;
};

inline StereoAudioFix g_StereoAudioFix;