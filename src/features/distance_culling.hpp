#pragma once

class DistanceCulling final
{
public:
    void Initialize() const;

    bool bOverrideGrass;
};

inline DistanceCulling g_DistanceCulling;
