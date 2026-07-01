#pragma once
#include <dxgi.h>

namespace MG1_DisplayScaling
{
    void Setup();
    void Init();
    void Draw(IDXGISwapChain* swap);

    inline bool bCropBorders = true;
    inline bool bCorrectTo4x3 = true;

}
