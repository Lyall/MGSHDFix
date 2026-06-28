#pragma once

namespace MGS2_ShimmerEffect
{
    void Init();
    void Draw(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* /*depth*/);
    void SetupHooks();

    inline bool bShaderLoaded  = false;
}
