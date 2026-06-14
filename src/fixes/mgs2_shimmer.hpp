#pragma once

namespace MGS2_ShimmerEffect
{
    void Init();
    void Draw(IDXGISwapChain* swap);
    void SetupHooks();

    inline bool bNeedsCompiler = true;
    inline bool bShaderLoaded  = false;
}
