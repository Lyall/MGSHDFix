#pragma once

class DistanceCulling final
{
public:
    void Initialize() const;

    bool bMGS2_ForceNPCLOD = true;
    bool bAlwaysRenderShellCasings = true;

    bool bForceGrassAlways;
    int vkForceGrassAlwaysToggle = 0;

    float fGrassDistanceScalar;
};

inline DistanceCulling g_DistanceCulling;
