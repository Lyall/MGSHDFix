#pragma once

namespace D3D11TextOverlay
{
    void Setup();
    void Init();
    void Tick();
    void HandleLevelTransition();

    inline bool bShaderLoaded = false;
    inline bool bShowSpeedrunnerStats = false;
    enum StatsPosition
    {
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
    };
    inline StatsPosition iStatsPosition = TopRight;
}
