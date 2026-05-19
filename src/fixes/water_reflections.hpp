#pragma once

class WaterReflectionFix final
{
public:
    void Initialize() const;
    bool MGS3_UseAdjustedOffsetY = true;

};

inline WaterReflectionFix g_WaterReflectionFix;
