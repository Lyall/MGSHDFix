#pragma once
#include <d3d11.h>



extern HWND MainHwnd;
// ===================== Device and Context =====================
extern ID3D11Device* d3dDevice;
extern ID3D11DeviceContext* d3dDeviceContext;

// ===================== DXGI =====================
extern IDXGIFactory* dxgiFactory;
extern IDXGISwapChain* swapChain;
extern IDXGIAdapter* dxgiAdapter;

// ===================== Render Targets =====================
extern ID3D11Texture2D* backBuffer;
extern ID3D11RenderTargetView* backBufferView;
extern ID3D11Texture2D* backBufferTexture;
extern ID3D11RenderTargetView* backBufferRTV;
extern ID3D11Texture2D* renderTargetTexture;
extern ID3D11RenderTargetView* renderTargetView;
extern ID3D11Texture2D* renderTargetBuffer;
extern ID3D11RenderTargetView* renderTargetRTV;

// ===================== Depth/Stencil =====================
extern ID3D11Texture2D* depthStencilBuffer;
extern ID3D11Texture2D* depthStencilTexture;
extern ID3D11DepthStencilView* depthStencilView;
extern ID3D11ShaderResourceView* depthStencilSRV;
extern ID3D11DepthStencilState* depthStencilState;
extern ID3D11DepthStencilState* depthStencilStateReadOnly;
extern ID3D11DepthStencilState* depthStencilStateReadWrite;
extern ID3D11DepthStencilState* depthStencilStateReadOnlyDepth;
extern ID3D11DepthStencilState* depthStencilStateReadOnlyStencil;
extern ID3D11DepthStencilState* depthStencilStateReadWriteDepth;
extern ID3D11DepthStencilState* depthStencilStateReadWriteStencil;

// ===================== Shader Resource Views and Samplers =====================
extern ID3D11ShaderResourceView* shaderResourceView;
extern ID3D11SamplerState* samplerState;
extern ID3D11SamplerState* linearSamplerState;
extern ID3D11SamplerState* pointSamplerState;
extern ID3D11Texture2D* texture;

// ===================== Shaders =====================
extern ID3D11InputLayout* inputLayout;
extern ID3D11VertexShader* vertexShader;
extern ID3D11GeometryShader* geometryShader;
extern ID3D11HullShader* hullShader;
extern ID3D11DomainShader* domainShader;
extern ID3D11ComputeShader* computeShader;
extern ID3D11PixelShader* pixelShader;

// ===================== Shader Buffers =====================
extern ID3D11Buffer* vertexShaderBuffer;
extern ID3D11Buffer* geometryShaderBuffer;
extern ID3D11Buffer* hullShaderBuffer;
extern ID3D11Buffer* domainShaderBuffer;
extern ID3D11Buffer* computeShaderBuffer;
extern ID3D11Buffer* pixelShaderBuffer;
extern ID3D11Buffer* inputLayoutBuffer;
extern ID3D11Buffer* computeShaderInputLayoutBuffer;
extern ID3D11Buffer* pixelShaderInputLayoutBuffer;
extern ID3D11Buffer* geometryShaderInputLayoutBuffer;
extern ID3D11Buffer* hullShaderOutputLayoutBuffer;
extern ID3D11Buffer* domainShaderOutputLayoutBuffer;
extern ID3D11Buffer* computeShaderOutputLayoutBuffer;
extern ID3D11Buffer* pixelShaderOutputLayoutBuffer;
extern ID3D11Buffer* geometryShaderOutputLayoutBuffer;

// ===================== Vertex and Index Buffers =====================
extern ID3D11Buffer* vertexBuffer;
extern ID3D11Buffer* indexBuffer;

// ===================== Constant Buffers =====================
extern ID3D11Buffer* constantBufferVS;
extern ID3D11Buffer* constantBufferPS;
extern ID3D11Buffer* constantBufferGS;
extern ID3D11Buffer* constantBufferHS;
extern ID3D11Buffer* constantBufferDS;
extern ID3D11Buffer* constantBufferCS;
extern ID3D11Buffer* constantBuffer;

// ===================== Blend States =====================
extern ID3D11BlendState* blendState;
extern ID3D11BlendState* alphaBlendState;
extern ID3D11BlendState* additiveBlendState;
extern ID3D11BlendState* noBlendState;
extern ID3D11BlendState* premultipliedAlphaBlendState;
extern ID3D11BlendState* opaqueBlendState;
extern ID3D11BlendState* customBlendState;
extern ID3D11BlendState* customAlphaBlendState;
extern ID3D11BlendState* customAdditiveBlendState;
extern ID3D11BlendState* customNoBlendState;
extern ID3D11BlendState* customPremultipliedAlphaBlendState;
extern ID3D11BlendState* customOpaqueBlendState;
extern ID3D11BlendState* customBlendState1;

// ===================== Rasterizer States =====================
extern ID3D11RasterizerState* rasterizerState;
extern ID3D11RasterizerState* cullNoneRasterizerState;
extern ID3D11RasterizerState* cullFrontRasterizerState;
extern ID3D11RasterizerState* cullBackRasterizerState;
extern ID3D11RasterizerState* cullFrontWireframeRasterizerState;
extern ID3D11RasterizerState* cullBackWireframeRasterizerState;
extern ID3D11RasterizerState* cullNoneWireframeRasterizerState;
extern ID3D11RasterizerState* cullFrontSolidRasterizerState;
extern ID3D11RasterizerState* cullBackSolidRasterizerState;
extern ID3D11RasterizerState* cullNoneSolidRasterizerState;
extern ID3D11RasterizerState* wireframeRasterizerState;
extern ID3D11RasterizerState* solidRasterizerState;
extern ID3D11RasterizerState* wireframeCullNoneRasterizerState;
extern ID3D11RasterizerState* wireframeCullFrontRasterizerState;
extern ID3D11RasterizerState* wireframeCullBackRasterizerState;
extern ID3D11RasterizerState* solidCullNoneRasterizerState;
extern ID3D11RasterizerState* solidCullFrontRasterizerState;
extern ID3D11RasterizerState* solidCullBackRasterizerState;
extern ID3D11RasterizerState* wireframeCullFrontSolidRasterizerState;
extern ID3D11RasterizerState* wireframeCullBackSolidRasterizerState;
extern ID3D11RasterizerState* solidCullFrontWireframeRasterizerState;
extern ID3D11RasterizerState* solidCullBackWireframeRasterizerState;
extern ID3D11RasterizerState* solidCullNoneWireframeRasterizerState;
extern ID3D11RasterizerState* solidCullNoneSolidRasterizerState;
extern ID3D11RasterizerState* solidCullFrontSolidRasterizerState;
extern ID3D11RasterizerState* solidCullBackSolidRasterizerState;

// ===================== Queries =====================
extern ID3D11Query* query;
extern ID3D11Query* occlusionQuery;
extern ID3D11Query* timestampQuery;
extern ID3D11Query* timestampDisjointQuery;

void Init_D3D11Hooks();
