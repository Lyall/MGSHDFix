#pragma once

#include <d3d11.h>
#include <wrl/client.h>

// Shared read access to the scene depth buffer, for fixes that need scene distance (occlusion, soft
// particles, depth fog). The game's depth is DSV-only (not shader-readable), so we copy whatever depth
// it binds for the 3D pass into a sampleable texture - the game's own resources are left untouched.
//
// Two uses:
//   - GetSRV(): sample the depth in your shader. Raw 0..1, reversed-Z (far ~0, near ~1). May be null.
//   - SetEndOf3DCallback(cb): to draw under the UI. cb fires after the 3D pass with the scene colour
//     RT and depth SRV (this is how the smk_blur smoke draws + gets occluded).
//
// Initialize() once the device exists; CaptureForFrame() once per frame from Present.
namespace SceneDepth
{
    void Initialize();
    void CaptureForFrame();
    ID3D11ShaderResourceView* GetSRV();
    bool IsAvailable();

    // Fires at the end of the 3D pass (after the scene, before UI). Gives the scene colour RT to draw
    // into and the depth SRV for occlusion.
    using EndOf3DCallback = void(*)(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* depth);
    void SetEndOf3DCallback(EndOf3DCallback cb);
}
