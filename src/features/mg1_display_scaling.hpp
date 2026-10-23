#pragma once

namespace MG1_DisplayScaling
{
    void Setup();
    void Init();
    void OnLoadingSpinnerDraw();
    void ResetRenderTarget();
    [[nodiscard]] bool Draw(IDXGISwapChain* swap);

    inline bool bCropBorders = true;
    inline bool bCorrectTo4x3 = true;

}
