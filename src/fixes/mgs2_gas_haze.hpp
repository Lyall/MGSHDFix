#pragma once

struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;

namespace MGS2GasHaze
{
    void Initialize();
    void DrawIntoCurrentTarget();
    void DrawInto(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* depth);
    void OnPreMenuRender(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* depth);
    void InvalidateCapture();
    void OnFeedbackCapture(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* depth);
    bool IsActive();

    inline bool bEnabled = true;
};
