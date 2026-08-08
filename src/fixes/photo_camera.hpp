#pragma once

struct IDXGISwapChain;

// The in-game camera's shutter also lands a full-resolution shot in the Steam screenshot
// library, instead of only the tiny save-file photo the ports inherited from the PS2.
namespace PhotoCamera
{
    inline bool bEnabled = false;

    void Initialize();
    void OnPresent(IDXGISwapChain* swap);
}
