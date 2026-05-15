#pragma once

class MGS2RetroColonel final
{
public:
    static void Initialize();


    bool bEnabled = false;
    bool bUseNewSprite = false;
};

inline MGS2RetroColonel g_MGS2RetroColonel;
