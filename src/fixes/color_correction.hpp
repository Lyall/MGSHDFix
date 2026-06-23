#pragma once

namespace ColorCorrection
{
    void Setup();
    void Init();
    void Draw(IDXGISwapChain* swap);

    inline bool bEnabled = false;
    inline bool bNeedsCompiler = true;
    inline bool bShaderLoaded = false;
}
