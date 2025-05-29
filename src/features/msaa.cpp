
/*
void MSAAswapchain()
{
    // Check MSAA support for 8x
    UINT msaaQuality = 8;
    HRESULT hr = d3dDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 8, &msaaQuality);
    spdlog::info("{}", msaaQuality);
    // Release the existing swapchain if it exists
    if (swapChain)
    {
        swapChain->Release();
        swapChain = nullptr;
    }
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = 800; // Set your desired width
    swapChainDesc.BufferDesc.Height = 600; // Set your desired height
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 120;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = MainHwnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    // Set MSAA level
    swapChainDesc.SampleDesc.Count = 8; // Adjust as needed
    swapChainDesc.SampleDesc.Quality = 8 - 1; // Use the highest quality level
    // Create the swap chain
    HRESULT swapchain = dxgiFactory->CreateSwapChain(d3dDevice, &swapChainDesc, &swapChain);
    if (FAILED(swapchain))
    {
        spdlog::error("Failed to create swap chain. HRESULT: 0x{:08X}", swapchain);
        return;
    }
    // Log the device creation
}
*/