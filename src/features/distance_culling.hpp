#pragma once

class DistanceCulling final
{
public:
    void Initialize() const;

    bool bForceGrassAlways;
    int vkForceGrassAlwaysToggle = 0;

    float fGrassDistanceScalar;
};

inline DistanceCulling g_DistanceCulling;
