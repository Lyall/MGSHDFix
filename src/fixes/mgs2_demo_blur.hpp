#pragma once

struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;

// Restores the demo framebuffer trail.
namespace MGS2DemoBlur
{
    void Initialize();
    void DrawInto(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* depth);
    void InvalidateCapture();
    void CaptureFrame(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* depth);
    bool IsFeedbackActive();

    inline bool bEnabled = true;

    inline bool bCutscenesOnly = false;
}
