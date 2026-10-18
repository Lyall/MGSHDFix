#include "stdafx.h"
#include "mg_vector_font_renderer.hpp"

#include "d3d11_state_cache.hpp"

using namespace MG_VectorFontRenderer;
using Microsoft::WRL::ComPtr;

namespace
{
    constexpr size_t nMaximumAtlasPages = 16;
    constexpr size_t nMaximumVertices = 128 * 4096 * 6;

    struct Constants
    {
        float fWidth;
        float fHeight;
        float fFade;
        UINT nBGRA;
    };

    constexpr const char* pszShader = R"(
cbuffer Settings : register(b0)
{
    float2 size;
    float fade;
    uint bgra;
};
Texture2D image : register(t0);
struct Input
{
    float2 position : POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};
struct Pixel
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};
Pixel Quad(Input input)
{
    Pixel output;
    output.position = float4(input.position / size * float2(2, -2) + float2(-1, 1), 0, 1);
    output.uv = input.uv;
    output.color = bgra != 0 ? input.color.bgra : input.color;
    return output;
}
Pixel Screen(uint id : SV_VertexID)
{
    Pixel output;
    float2 uv = float2((id << 1) & 2, id & 2);
    output.position = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    output.uv = uv;
    output.color = 1;
    return output;
}
float4 Glyph(Pixel input) : SV_Target
{
    float coverage = image.Load(int3(input.uv, 0)).r;
    return float4(input.color.rgb * coverage, coverage);
}
float4 Base(Pixel input) : SV_Target
{
    float4 pixel = image.Load(int3(input.uv * float2(512, 424), 0));
    pixel.a = 1 - pixel.a;
    return float4(pixel.rgb * pixel.a, pixel.a);
}
float4 Resolve(Pixel input) : SV_Target
{
    float4 pixel = image.Load(int3(input.position.xy, 0));
    return float4(pixel.a > 0 ? pixel.rgb / pixel.a * fade : 0, pixel.a);
}
)";
}

bool Renderer::IsDevice(ID3D11Device* pDevice) const
{
    return m_pDevice.Get() == pDevice && m_pResolveShader;
}

bool Renderer::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, decltype(&D3DCompile) pfnCompile)
{
    if (IsDevice(pDevice))
    {
        return true;
    }
    *this = Renderer {};
    ComPtr<ID3D11Device1> pDevice1;
    if (!pDevice || !pContext || !pfnCompile || FAILED(pDevice->QueryInterface(IID_PPV_ARGS(&pDevice1))) || FAILED(pContext->QueryInterface(IID_PPV_ARGS(&m_pContext))))
    {
        return false;
    }
    m_pDevice = pDevice;
    const D3D_FEATURE_LEVEL eLevel = pDevice->GetFeatureLevel();
    const UINT nFlags = (pDevice->GetCreationFlags() & D3D11_CREATE_DEVICE_SINGLETHREADED) ? D3D11_1_CREATE_DEVICE_CONTEXT_STATE_SINGLETHREADED : 0;
    if (FAILED(pDevice1->CreateDeviceContextState(nFlags, &eLevel, 1, D3D11_SDK_VERSION, __uuidof(ID3D11Device), nullptr, &m_pState)))
    {
        return false;
    }
    const auto Compile = [&](const char* pszEntry, const char* pszProfile, ComPtr<ID3DBlob>& pBlob)
    {
        ComPtr<ID3DBlob> pErrors;
        return SUCCEEDED(pfnCompile(pszShader, strlen(pszShader), nullptr, nullptr, nullptr, pszEntry, pszProfile, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pBlob, &pErrors));
    };
    ComPtr<ID3DBlob> pQuad, pScreen, pGlyph, pBase, pResolve;
    if (!Compile("Quad", "vs_5_0", pQuad) || !Compile("Screen", "vs_5_0", pScreen) || !Compile("Glyph", "ps_5_0", pGlyph) || !Compile("Base", "ps_5_0", pBase) || !Compile("Resolve", "ps_5_0", pResolve))
    {
        return false;
    }
    const D3D11_INPUT_ELEMENT_DESC astLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    D3D11_BUFFER_DESC stConstants {};
    stConstants.ByteWidth = sizeof(Constants);
    stConstants.Usage = D3D11_USAGE_DEFAULT;
    stConstants.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_BLEND_DESC stBlend {};
    auto& stTarget = stBlend.RenderTarget[0];
    stTarget.BlendEnable = TRUE;
    stTarget.SrcBlend = stTarget.SrcBlendAlpha = D3D11_BLEND_ONE;
    stTarget.DestBlend = stTarget.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    stTarget.BlendOp = stTarget.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    stTarget.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    D3D11_RASTERIZER_DESC stRasterizer {};
    stRasterizer.FillMode = D3D11_FILL_SOLID;
    stRasterizer.CullMode = D3D11_CULL_NONE;
    stRasterizer.DepthClipEnable = TRUE;
    if (FAILED(pDevice->CreateVertexShader(pQuad->GetBufferPointer(), pQuad->GetBufferSize(), nullptr, &m_pQuadShader))
        || FAILED(pDevice->CreateVertexShader(pScreen->GetBufferPointer(), pScreen->GetBufferSize(), nullptr, &m_pScreenShader))
        || FAILED(pDevice->CreatePixelShader(pGlyph->GetBufferPointer(), pGlyph->GetBufferSize(), nullptr, &m_pGlyphShader))
        || FAILED(pDevice->CreatePixelShader(pBase->GetBufferPointer(), pBase->GetBufferSize(), nullptr, &m_pBaseShader))
        || FAILED(pDevice->CreateInputLayout(astLayout, static_cast<UINT>(std::size(astLayout)), pQuad->GetBufferPointer(), pQuad->GetBufferSize(), &m_pLayout))
        || FAILED(pDevice->CreateBuffer(&stConstants, nullptr, &m_pConstants))
        || FAILED(pDevice->CreateBlendState(&stBlend, &m_pBlend))
        || FAILED(pDevice->CreateRasterizerState(&stRasterizer, &m_pRasterizer)))
    {
        return false;
    }
    return SUCCEEDED(pDevice->CreatePixelShader(pResolve->GetBufferPointer(), pResolve->GetBufferSize(), nullptr, &m_pResolveShader));
}

bool Renderer::CreateSurface(Surface& stSurface, UINT nWidth, UINT nHeight, DXGI_FORMAT eFormat, bool bTarget)
{
    stSurface = {};
    D3D11_TEXTURE2D_DESC stDesc {};
    stDesc.Width = nWidth;
    stDesc.Height = nHeight;
    stDesc.MipLevels = stDesc.ArraySize = stDesc.SampleDesc.Count = 1;
    stDesc.Format = eFormat;
    stDesc.Usage = D3D11_USAGE_DEFAULT;
    stDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | (bTarget ? D3D11_BIND_RENDER_TARGET : 0);
    return SUCCEEDED(m_pDevice->CreateTexture2D(&stDesc, nullptr, &stSurface.pTexture))
        && SUCCEEDED(m_pDevice->CreateShaderResourceView(stSurface.pTexture.Get(), nullptr, &stSurface.pView))
        && (!bTarget || SUCCEEDED(m_pDevice->CreateRenderTargetView(stSurface.pTexture.Get(), nullptr, &stSurface.pTarget)));
}

void Renderer::ResetGlyphs()
{
    m_vAtlases.clear();
    m_nRevision = 0;
    m_nFade = -1;
}

bool Renderer::AddGlyph(const uint8_t* pCoverage, int nWidth, int nHeight, GlyphLocation& stLocation)
{
    if (!pCoverage || nWidth <= 0 || nHeight <= 0 || nWidth > nAtlasSize || nHeight > nAtlasSize)
    {
        return false;
    }
    if (!m_vAtlases.empty())
    {
        Atlas& stAtlas = m_vAtlases.back();
        if (stAtlas.nX + nWidth > nAtlasSize)
        {
            stAtlas.nX = 0;
            stAtlas.nY += stAtlas.nRowHeight;
            stAtlas.nRowHeight = 0;
        }
    }
    if (m_vAtlases.empty() || m_vAtlases.back().nY + nHeight > nAtlasSize)
    {
        if (m_vAtlases.size() >= nMaximumAtlasPages)
        {
            return false;
        }
        Atlas stAtlas;
        if (!CreateSurface(stAtlas.stSurface, nAtlasSize, nAtlasSize, DXGI_FORMAT_R8_UNORM, false))
        {
            return false;
        }
        m_vAtlases.push_back(std::move(stAtlas));
    }
    Atlas& stAtlas = m_vAtlases.back();
    stLocation = { static_cast<UINT>(m_vAtlases.size() - 1), stAtlas.nX, stAtlas.nY };
    const D3D11_BOX stBox { UINT(stAtlas.nX), UINT(stAtlas.nY), 0, UINT(stAtlas.nX + nWidth), UINT(stAtlas.nY + nHeight), 1 };
    m_pContext->UpdateSubresource(stAtlas.stSurface.pTexture.Get(), 0, &stBox, pCoverage, nWidth, 0);
    stAtlas.nX += nWidth;
    stAtlas.nRowHeight = (std::max)(stAtlas.nRowHeight, nHeight);
    return true;
}

ID3D11ShaderResourceView* Renderer::Render(const std::vector<uint32_t>& vBasePixels, const std::vector<Vertex>& vVertices, const std::vector<Batch>& vBatches, uint64_t nRevision, int nScale, int nFade, DXGI_FORMAT eFormat)
{
    if (!m_pResolveShader || vBasePixels.size() != nLayerWidth * nLayerHeight || nScale < 2 || nScale > 8 || vVertices.size() > nMaximumVertices
        || (eFormat != DXGI_FORMAT_R8G8B8A8_UNORM && eFormat != DXGI_FORMAT_B8G8R8A8_UNORM))
    {
        return nullptr;
    }
    for (const Batch& stBatch : vBatches)
    {
        if (stBatch.nPage >= m_vAtlases.size() || size_t(stBatch.nStart) + stBatch.nCount > vVertices.size())
        {
            return nullptr;
        }
    }
    const bool bResize = m_nScale != nScale || m_eFormat != eFormat;
    if (bResize)
    {
        m_nScale = 0;
        if (!CreateSurface(m_stBase, nLayerWidth, nLayerHeight, eFormat, false)
            || !CreateSurface(m_stComposite, nLayerWidth * nScale, nLayerHeight * nScale, DXGI_FORMAT_R16G16B16A16_FLOAT, true)
            || !CreateSurface(m_stOutput, nLayerWidth * nScale, nLayerHeight * nScale, eFormat, true))
        {
            return nullptr;
        }
        m_nScale = nScale;
        m_eFormat = eFormat;
        m_nFade = -1;
        m_nRevision = 0;
        m_vBasePixels.clear();
    }
    const bool bBaseChanged = m_vBasePixels != vBasePixels;
    const bool bTextChanged = m_nRevision != nRevision;
    const bool bCompose = bBaseChanged || bTextChanged || bResize;
    if (!bCompose && m_nFade == nFade)
    {
        return m_stOutput.pView.Get();
    }
    if (bTextChanged && !vVertices.empty())
    {
        if (m_nVertexCapacity < vVertices.size())
        {
            const UINT nCapacity = (std::min)(static_cast<UINT>(nMaximumVertices), (std::max)(static_cast<UINT>(vVertices.size()), (std::max)(UINT(256), m_nVertexCapacity * 2)));
            m_pVertices.Reset();
            m_nVertexCapacity = 0;
            D3D11_BUFFER_DESC stDesc {};
            stDesc.ByteWidth = nCapacity * sizeof(Vertex);
            stDesc.Usage = D3D11_USAGE_DYNAMIC;
            stDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            stDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            if (FAILED(m_pDevice->CreateBuffer(&stDesc, nullptr, &m_pVertices)))
            {
                return nullptr;
            }
            m_nVertexCapacity = nCapacity;
        }
        D3D11_MAPPED_SUBRESOURCE stMapped {};
        if (FAILED(m_pContext->Map(m_pVertices.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &stMapped)))
        {
            return nullptr;
        }
        memcpy(stMapped.pData, vVertices.data(), vVertices.size() * sizeof(Vertex));
        m_pContext->Unmap(m_pVertices.Get(), 0);
    }
    if (bBaseChanged)
    {
        m_pContext->UpdateSubresource(m_stBase.pTexture.Get(), 0, nullptr, vBasePixels.data(), nLayerWidth * sizeof(uint32_t), 0);
    }
    const Constants stConstants { float(nLayerWidth * nScale), float(nLayerHeight * nScale), float(nFade) / 255.0f, eFormat == DXGI_FORMAT_B8G8R8A8_UNORM };
    m_pContext->UpdateSubresource(m_pConstants.Get(), 0, nullptr, &stConstants, 0, 0);
    ComPtr<ID3DDeviceContextState> pPrevious;
    m_pContext->SwapDeviceContextState(m_pState.Get(), &pPrevious);
    D3D11StateCache::Invalidate(m_pContext.Get());
    m_pContext->ClearState();
    const D3D11_VIEWPORT stViewport { 0, 0, stConstants.fWidth, stConstants.fHeight, 0, 1 };
    m_pContext->RSSetViewports(1, &stViewport);
    m_pContext->RSSetState(m_pRasterizer.Get());
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pContext->VSSetConstantBuffers(0, 1, m_pConstants.GetAddressOf());
    m_pContext->PSSetConstantBuffers(0, 1, m_pConstants.GetAddressOf());
    if (bCompose)
    {
        m_pContext->OMSetRenderTargets(1, m_stComposite.pTarget.GetAddressOf(), nullptr);
        m_pContext->VSSetShader(m_pScreenShader.Get(), nullptr, 0);
        m_pContext->PSSetShader(m_pBaseShader.Get(), nullptr, 0);
        m_pContext->PSSetShaderResources(0, 1, m_stBase.pView.GetAddressOf());
        m_pContext->Draw(3, 0);
        if (!vVertices.empty())
        {
            const UINT nStride = sizeof(Vertex), nOffset = 0;
            m_pContext->IASetInputLayout(m_pLayout.Get());
            m_pContext->IASetVertexBuffers(0, 1, m_pVertices.GetAddressOf(), &nStride, &nOffset);
            m_pContext->VSSetShader(m_pQuadShader.Get(), nullptr, 0);
            m_pContext->PSSetShader(m_pGlyphShader.Get(), nullptr, 0);
            m_pContext->OMSetBlendState(m_pBlend.Get(), nullptr, UINT_MAX);
            for (const Batch& stBatch : vBatches)
            {
                m_pContext->PSSetShaderResources(0, 1, m_vAtlases[stBatch.nPage].stSurface.pView.GetAddressOf());
                m_pContext->Draw(stBatch.nCount, stBatch.nStart);
            }
        }
    }
    m_pContext->OMSetRenderTargets(1, m_stOutput.pTarget.GetAddressOf(), nullptr);
    m_pContext->OMSetBlendState(nullptr, nullptr, UINT_MAX);
    m_pContext->IASetInputLayout(nullptr);
    m_pContext->VSSetShader(m_pScreenShader.Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pResolveShader.Get(), nullptr, 0);
    m_pContext->PSSetShaderResources(0, 1, m_stComposite.pView.GetAddressOf());
    m_pContext->Draw(3, 0);
    m_pContext->ClearState();
    m_pContext->SwapDeviceContextState(pPrevious.Get(), nullptr);
    D3D11StateCache::Invalidate(m_pContext.Get());
    if (bBaseChanged)
    {
        m_vBasePixels = vBasePixels;
    }
    m_nRevision = nRevision;
    m_nFade = nFade;
    return m_stOutput.pView.Get();
}
