#pragma once

struct IDXGISwapChain;

// Restores the demo camera crossfade, which doesn't composite on the MC's D3D11 path.
namespace MGS2_Crossfade
{
    inline bool bEnabled = true;   // "Fix Broken PS2 Visual Effects"

    void Initialize();
    void OnPresent(IDXGISwapChain* swap);
    void OnPreMenuRender(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* depth);

    // True while a demo camera crossfade is in progress (i.e. a camera transition). Other
    // present-time effects use this to avoid feeding a stale pre-cut frame across the cut.
    bool IsFading();
}
