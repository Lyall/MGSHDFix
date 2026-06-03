#pragma once

namespace MGS2_ShimmerEffect
{
    void Init();
    void Draw(IDXGISwapChain* swap);
    void SetupHooks();

    inline bool bNeedsCompiler = false;
    inline bool bShaderLoaded  = false;
}
