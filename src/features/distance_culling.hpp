#pragma once

class DistanceCulling final
{
public:
    void Initialize() const;

    bool bMGS2_MarineForceLOD = true;
    bool bMGS2_ForcePlayerLOD = true;
    bool bMGS2_ForceHostageLOD = true;


    bool bForceGrassAlways;
    int vkForceGrassAlwaysToggle = 0;

    float fGrassDistanceScalar;
};

inline DistanceCulling g_DistanceCulling;
