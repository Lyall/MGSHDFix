#pragma once

class DistanceCulling final
{
public:
    void Initialize() const;

    bool bForceGrassAlways;
    float fGrassDistanceScalar;
};

inline DistanceCulling g_DistanceCulling;
