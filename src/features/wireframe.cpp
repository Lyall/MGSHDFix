#include "../resources/d3d11_api.hpp"
#include <spdlog/spdlog.h>


void CreateWireframeRasterizerState()
{
    // Define the rasterizer state description
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_WIREFRAME; // Set to wireframe mode
    rasterDesc.CullMode = D3D11_CULL_BACK;     // Back-face culling
    rasterDesc.FrontCounterClockwise = FALSE;  // Clockwise is front-facing
    rasterDesc.DepthClipEnable = TRUE;         // Enable depth clipping
    // Create the rasterizer state
    HRESULT hr = d3dDevice->CreateRasterizerState(&rasterDesc, &wireframeRasterizerState);
    if (FAILED(hr))
    {
        spdlog::error("Failed to create wireframe rasterizer state. HRESULT: 0x{:08X}", hr);
        return;
    }
    spdlog::info("Wireframe rasterizer state successfully created.");
}

void ApplyWireframeRasterizerState()
{
    if (d3dDeviceContext && wireframeRasterizerState)
    {
        // Set the wireframe rasterizer state
        d3dDeviceContext->RSSetState(wireframeRasterizerState);
        spdlog::info("Wireframe rasterizer state applied.");
    }
    else
    {
        spdlog::error("Failed to apply wireframe rasterizer state. Ensure it is created and initialized.");
    }
}

void Cleanup()
{
    if (wireframeRasterizerState) wireframeRasterizerState->Release();
    if (d3dDeviceContext) d3dDeviceContext->Release();
    if (d3dDevice) d3dDevice->Release();
}
