#pragma once

namespace ColorCorrection
{
    const char* GetShaderSource();
    void ResetRenderTarget();
    void Setup();
    void Init();
    void Draw(IDXGISwapChain* swap);

    inline bool bEnabled = false;
    inline bool bShaderLoaded = false;
}
