#include <d3d11.h>
#include <spdlog/spdlog.h>
#include <safetyhook.hpp>
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib, "dxgi.lib")

HWND MainHwnd = nullptr;
// ===================== Device and Context =====================
ID3D11Device* d3dDevice = nullptr;
ID3D11DeviceContext* d3dDeviceContext = nullptr;

// ===================== DXGI =====================
IDXGIAdapter* dxgiAdapter = nullptr;
IDXGIFactory* dxgiFactory = nullptr;
IDXGISwapChain* swapChain = nullptr;

// ===================== Render Targets =====================
ID3D11Texture2D* backBuffer = nullptr;
ID3D11RenderTargetView* backBufferView = nullptr;
ID3D11Texture2D* backBufferTexture = nullptr;
ID3D11RenderTargetView* backBufferRTV = nullptr;
ID3D11Texture2D* renderTargetTexture = nullptr;
ID3D11RenderTargetView* renderTargetView = nullptr;
ID3D11Texture2D* renderTargetBuffer = nullptr;
ID3D11RenderTargetView* renderTargetRTV = nullptr;

// ===================== Depth/Stencil =====================
ID3D11Texture2D* depthStencilBuffer = nullptr;
ID3D11Texture2D* depthStencilTexture = nullptr;
ID3D11DepthStencilView* depthStencilView = nullptr;
ID3D11ShaderResourceView* depthStencilSRV = nullptr;
ID3D11DepthStencilState* depthStencilState = nullptr;
ID3D11DepthStencilState* depthStencilStateReadOnly = nullptr;
ID3D11DepthStencilState* depthStencilStateReadWrite = nullptr;
ID3D11DepthStencilState* depthStencilStateReadOnlyDepth = nullptr;
ID3D11DepthStencilState* depthStencilStateReadOnlyStencil = nullptr;
ID3D11DepthStencilState* depthStencilStateReadWriteDepth = nullptr;
ID3D11DepthStencilState* depthStencilStateReadWriteStencil = nullptr;

// ===================== Shader Resource Views and Samplers =====================
ID3D11ShaderResourceView* shaderResourceView = nullptr;
ID3D11SamplerState* samplerState = nullptr;
ID3D11SamplerState* linearSamplerState = nullptr;
ID3D11SamplerState* pointSamplerState = nullptr;
ID3D11Texture2D* texture = nullptr;

// ===================== Shaders =====================
ID3D11InputLayout* inputLayout = nullptr;
ID3D11VertexShader* vertexShader = nullptr;
ID3D11GeometryShader* geometryShader = nullptr;
ID3D11HullShader* hullShader = nullptr;
ID3D11DomainShader* domainShader = nullptr;
ID3D11ComputeShader* computeShader = nullptr;
ID3D11PixelShader* pixelShader = nullptr;

// ===================== Shader Buffers =====================
ID3D11Buffer* vertexShaderBuffer = nullptr;
ID3D11Buffer* geometryShaderBuffer = nullptr;
ID3D11Buffer* hullShaderBuffer = nullptr;
ID3D11Buffer* domainShaderBuffer = nullptr;
ID3D11Buffer* computeShaderBuffer = nullptr;
ID3D11Buffer* pixelShaderBuffer = nullptr;
ID3D11Buffer* inputLayoutBuffer = nullptr;
ID3D11Buffer* computeShaderInputLayoutBuffer = nullptr;
ID3D11Buffer* pixelShaderInputLayoutBuffer = nullptr;
ID3D11Buffer* geometryShaderInputLayoutBuffer = nullptr;
ID3D11Buffer* hullShaderOutputLayoutBuffer = nullptr;
ID3D11Buffer* domainShaderOutputLayoutBuffer = nullptr;
ID3D11Buffer* computeShaderOutputLayoutBuffer = nullptr;
ID3D11Buffer* pixelShaderOutputLayoutBuffer = nullptr;
ID3D11Buffer* geometryShaderOutputLayoutBuffer = nullptr;

// ===================== Vertex and Index Buffers =====================
ID3D11Buffer* vertexBuffer = nullptr;
ID3D11Buffer* indexBuffer = nullptr;

// ===================== Constant Buffers =====================
ID3D11Buffer* constantBufferVS = nullptr;
ID3D11Buffer* constantBufferPS = nullptr;
ID3D11Buffer* constantBufferGS = nullptr;
ID3D11Buffer* constantBufferHS = nullptr;
ID3D11Buffer* constantBufferDS = nullptr;
ID3D11Buffer* constantBufferCS = nullptr;
ID3D11Buffer* constantBuffer = nullptr;

// ===================== Blend States =====================
ID3D11BlendState* blendState = nullptr;
ID3D11BlendState* alphaBlendState = nullptr;
ID3D11BlendState* additiveBlendState = nullptr;
ID3D11BlendState* noBlendState = nullptr;
ID3D11BlendState* premultipliedAlphaBlendState = nullptr;
ID3D11BlendState* opaqueBlendState = nullptr;
ID3D11BlendState* customBlendState = nullptr;
ID3D11BlendState* customAlphaBlendState = nullptr;
ID3D11BlendState* customAdditiveBlendState = nullptr;
ID3D11BlendState* customNoBlendState = nullptr;
ID3D11BlendState* customPremultipliedAlphaBlendState = nullptr;
ID3D11BlendState* customOpaqueBlendState = nullptr;
ID3D11BlendState* customBlendState1 = nullptr;

// ===================== Rasterizer States =====================
ID3D11RasterizerState* rasterizerState = nullptr;
ID3D11RasterizerState* cullNoneRasterizerState = nullptr;
ID3D11RasterizerState* cullFrontRasterizerState = nullptr;
ID3D11RasterizerState* cullBackRasterizerState = nullptr;
ID3D11RasterizerState* cullFrontWireframeRasterizerState = nullptr;
ID3D11RasterizerState* cullBackWireframeRasterizerState = nullptr;
ID3D11RasterizerState* cullNoneWireframeRasterizerState = nullptr;
ID3D11RasterizerState* cullFrontSolidRasterizerState = nullptr;
ID3D11RasterizerState* cullBackSolidRasterizerState = nullptr;
ID3D11RasterizerState* cullNoneSolidRasterizerState = nullptr;
ID3D11RasterizerState* wireframeRasterizerState = nullptr;
ID3D11RasterizerState* solidRasterizerState = nullptr;
ID3D11RasterizerState* wireframeCullNoneRasterizerState = nullptr;
ID3D11RasterizerState* wireframeCullFrontRasterizerState = nullptr;
ID3D11RasterizerState* wireframeCullBackRasterizerState = nullptr;
ID3D11RasterizerState* solidCullNoneRasterizerState = nullptr;
ID3D11RasterizerState* solidCullFrontRasterizerState = nullptr;
ID3D11RasterizerState* solidCullBackRasterizerState = nullptr;
ID3D11RasterizerState* wireframeCullFrontSolidRasterizerState = nullptr;
ID3D11RasterizerState* wireframeCullBackSolidRasterizerState = nullptr;
ID3D11RasterizerState* solidCullFrontWireframeRasterizerState = nullptr;
ID3D11RasterizerState* solidCullBackWireframeRasterizerState = nullptr;
ID3D11RasterizerState* solidCullNoneWireframeRasterizerState = nullptr;
ID3D11RasterizerState* solidCullNoneSolidRasterizerState = nullptr;
ID3D11RasterizerState* solidCullFrontSolidRasterizerState = nullptr;
ID3D11RasterizerState* solidCullBackSolidRasterizerState = nullptr;

// ===================== Queries =====================
ID3D11Query* query = nullptr;
ID3D11Query* occlusionQuery = nullptr;
ID3D11Query* timestampQuery = nullptr;
ID3D11Query* timestampDisjointQuery = nullptr;

extern void preD3D11CreateDevice();

extern void afterD3D11CreateDevice();

// D3D11CreateDevice Hook
SafetyHookInline D3D11CreateDevice_hook {};
HRESULT WINAPI D3D11CreateDevice_hooked(
    _In_opt_ IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    _In_reads_opt_(FeatureLevels) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    _COM_Outptr_opt_ ID3D11Device** ppDevice,
    _Out_opt_ D3D_FEATURE_LEVEL* pFeatureLevel,
    _COM_Outptr_opt_ ID3D11DeviceContext** ppImmediateContext)
{
    preD3D11CreateDevice();
    HRESULT result = D3D11CreateDevice_hook.stdcall<HRESULT>(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);
    dxgiAdapter = pAdapter;
    d3dDevice = *ppDevice;
    d3dDeviceContext = *ppImmediateContext;
    if (SUCCEEDED(result))
    {
        if (d3dDevice)
        {
            spdlog::info("D3D11CreateDevice successful.");
            afterD3D11CreateDevice();
        }
        else
        {
            spdlog::error("d3dDevice is nullptr after D3D11CreateDevice.");
        }
    }
    else
    {
        spdlog::error("Failed to create D3D11 Device. HRESULT: 0x{:08X}", result);
    }
    return result;
}

extern void preCreateDXGIFactory();
extern void afterCreateDXGIFactory();


// D3D11CreateDevice Hook
SafetyHookInline CreateDXGIFactory_hook {};
HRESULT WINAPI CreateDXGIFactory_hooked(REFIID riid, _COM_Outptr_ void** ppFactory)
{
    preCreateDXGIFactory();
    auto result = CreateDXGIFactory_hook.stdcall<HRESULT>(riid, ppFactory);
    if (SUCCEEDED(result))
    {
        dxgiFactory = static_cast<IDXGIFactory*>(*ppFactory);
        if (dxgiFactory) 
        {
            spdlog::info("CreateDXGIFactory successful.");
            afterCreateDXGIFactory();
        }
        else
        {
            spdlog::error("DXGI Factory is nullptr after CreateDXGIFactory.");
        }
    }
    else
    {
        spdlog::error("Failed to create DXGI Factory.");
    }
    return result;
}

void Init_D3D11Hooks()
{
    D3D11CreateDevice_hook = safetyhook::create_inline(D3D11CreateDevice, reinterpret_cast<void*>(D3D11CreateDevice_hooked));
    spdlog::info("MG/MG2 | MGS 2 | MGS 3: D3D11CreateDevice: Hooked function.");
    CreateDXGIFactory_hook = safetyhook::create_inline(CreateDXGIFactory, reinterpret_cast<void*>(CreateDXGIFactory_hooked));
    spdlog::info("MG/MG2 | MGS 2 | MGS 3: CreateDXGIFactory: Hooked function.");
}




