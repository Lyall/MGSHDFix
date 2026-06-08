#pragma once


namespace MGS2ThermalGoggles 
{
    
    void Setup();
    void Tick();

    enum class IRMode : int 
    { 
        Substance = 0, 
        SplinterCell, 
        RedHot, 
        WhiteHot, 
        BlackHot, 
        Count 
    };

    inline IRMode g_irMode = IRMode::Substance;
    inline int vk_ToggleThermalGoggleColor = 0;
    inline bool bEnabled = false;

}
