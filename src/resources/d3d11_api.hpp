#pragma once
#include "helper.hpp"
#include <d3d11.h>


class D3D11Hooks final
{
    
public:
    static void Initialize();

    static void UnloadCompiler(HMODULE d3dcompiler);

    HWND MainHwnd = nullptr;

    // ===================== Device and Context =====================
    ComPtrLite<ID3D11Device> d3dDevice;

    ComPtrLite<ID3D11DeviceContext> d3dDeviceContext;

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
};

inline D3D11Hooks g_D3D11Hooks;
