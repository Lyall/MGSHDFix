#pragma once

struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;

// The demo-driven fullscreen blur. Each demo tick the PS2 lerps the frame toward the previous frame by
// intense/128, sampling it half a texel offset so bilinear spreads it a little per iteration - a soft
// accumulation glow whose strength the demo keyframes. The port's version of the draw samples the current
// frame into itself, which does nothing, so the whole layer is missing. The actor still runs and animates
// intense; read it and composite the blend ourselves after the overlay pass, like the original.
namespace MGS2DemoBlur
{
    void Initialize();
    void DrawInto(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* depth);

    inline bool bEnabled = true;

    inline bool bCutscenesOnly = false;
}
