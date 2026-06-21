#pragma once

struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;

namespace MGS2GasHaze
{
    void Initialize();
    // D3D11 smoke pass; called at the end of the 3D pass (before UI) via SceneDepth's callback,
    // drawing into the scene colour RT with the live depth for occlusion.
    void DrawInto(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* depth);

    inline bool bEnabled = true;
};
