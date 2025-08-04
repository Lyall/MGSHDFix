#include "wireframe.hpp"
#include "d3d11_api.hpp"
#include "logging.hpp"

/*
void CreateWireframeRasterizerState()
{
    // Define the rasterizer state description
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_WIREFRAME; // Set to wireframe mode
    rasterDesc.CullMode = D3D11_CULL_BACK;     // Back-face culling
    rasterDesc.FrontCounterClockwise = FALSE;  // Clockwise is front-facing
    rasterDesc.DepthClipEnable = TRUE;         // Enable depth clipping
    // Create the rasterizer state
    HRESULT hr = g_D3D11Hooks.d3dDevice->CreateRasterizerState(&rasterDesc, &g_D3D11Hooks.wireframeRasterizerState);
    if (FAILED(hr))
    {
        spdlog::error("Failed to create wireframe rasterizer state. HRESULT: 0x{:08X}", hr);
        return;
    }
    spdlog::info("Wireframe rasterizer state successfully created.");
}

void ApplyWireframeRasterizerState()
{
    if (g_D3D11Hooks.d3dDeviceContext && g_D3D11Hooks.wireframeRasterizerState)
    {
        // Set the wireframe rasterizer state
        g_D3D11Hooks.d3dDeviceContext->RSSetState(g_D3D11Hooks.wireframeRasterizerState);
        spdlog::info("Wireframe rasterizer state applied.");
    }
    else
    {
        spdlog::error("Failed to apply wireframe rasterizer state. Ensure it is created and initialized.");
    }
}

void Cleanup()
{
    if (g_D3D11Hooks.wireframeRasterizerState) g_D3D11Hooks.wireframeRasterizerState->Release();
    if (g_D3D11Hooks.d3dDeviceContext) g_D3D11Hooks.d3dDeviceContext->Release();
    if (g_D3D11Hooks.d3dDevice) g_D3D11Hooks.d3dDevice->Release();
}
*/