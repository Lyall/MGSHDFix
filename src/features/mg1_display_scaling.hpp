#pragma once
#include <d3d11.h>
#include <dxgi.h>

namespace MG1_DisplayScaling
{
    void Setup();
    void Init();
    void Draw(IDXGISwapChain* swap);

    inline bool bEnabled = true;


    inline float scaleX = 1.131f;
    inline float scaleY = 1.130f;
    inline float posX = 0.5f;
    inline float posY = 0.5f;

}
