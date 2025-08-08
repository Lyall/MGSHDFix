#include "postfx.hpp"
#include <d3dcompiler.h>
#include <spdlog/spdlog.h>

// Bright magenta test shader
static const char* SolidColorPS = R"(
float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_Target
{
    return float4(1, 0, 1, 1); // magenta
}
)";

bool PostFX::CompileShader(const std::string& source, const std::string& entry, const std::string& target, ID3DBlob** blob)
{
    HMODULE d3dcompiler = LoadLibraryA("d3dcompiler_47.dll");
    if (!d3dcompiler)
    {
        spdlog::error("PostFX: Failed to load d3dcompiler_47.dll");
        return false;
    }

    using pD3DCompile = HRESULT(WINAPI*)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);
    auto D3DCompileFunc = reinterpret_cast<pD3DCompile>(GetProcAddress(d3dcompiler, "D3DCompile"));
    if (!D3DCompileFunc)
    {
        spdlog::error("PostFX: Failed to get D3DCompile address");
        return false;
    }

    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompileFunc(
        source.c_str(),
        source.size(),
        nullptr,
        nullptr,
        nullptr,
        entry.c_str(),
        target.c_str(),
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        blob,
        errorBlob.GetAddressOf()
    );

    if (FAILED(hr))
    {
        if (errorBlob)
            spdlog::error("Shader compile error: {}", static_cast<const char*>(errorBlob->GetBufferPointer()));
        return false;
    }
    return true;
}

bool PostFX::Initialize(ID3D11Device* device, IDXGISwapChain* swapChain)
{
    spdlog::info("PostFX: Initializing magenta test...");

    // Fullscreen triangle vertex shader
    const char* vsCode = R"(
    struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
    VSOut main(uint id : SV_VertexID)
    {
        VSOut o;
        o.uv = float2((id << 1) & 2, id & 2);
        o.pos = float4(o.uv * float2(2, -2) + float2(-1, 1), 0, 1);
        return o;
    }
    )";

    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    if (!CompileShader(vsCode, "main", "vs_5_0", vsBlob.GetAddressOf()))
        return false;
    if (FAILED(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, m_vertexShader.GetAddressOf())))
        return false;

    // Compile magenta PS
    Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
    if (!CompileShader(SolidColorPS, "main", "ps_5_0", psBlob.GetAddressOf()))
        return false;
    if (FAILED(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, m_pixelShader.GetAddressOf())))
        return false;

    // Sampler
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = sampDesc.AddressV = sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    device->CreateSamplerState(&sampDesc, m_samLinear.GetAddressOf());

    // Copy texture & SRV to match backbuffer
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    if (SUCCEEDED(swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()))))
    {
        D3D11_TEXTURE2D_DESC desc;
        backBuffer->GetDesc(&desc);
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        if (FAILED(device->CreateTexture2D(&desc, nullptr, m_copyTex.GetAddressOf())))
            return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        if (FAILED(device->CreateShaderResourceView(m_copyTex.Get(), &srvDesc, m_copySRV.GetAddressOf())))
            return false;
    }

    spdlog::info("PostFX: Magenta test initialized.");
    return true;
}

void PostFX::Render(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv)
{
    if (!context || !rtv)
        return;

    // Bind RTV
    context->OMSetRenderTargets(1, &rtv, nullptr);

    // Fullscreen tri
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->PSSetSamplers(0, 1, m_samLinear.GetAddressOf());

    context->Draw(3, 0);
}

void PostFX::Shutdown()
{
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_samLinear.Reset();
    m_copyTex.Reset();
    m_copySRV.Reset();
}
