// ReSharper disable CommentTypo
#include "stdafx.h"

#include "d3d11_text_overlay.hpp"
#include "d3d11_api.hpp"

#include "logging.hpp"

#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"
#include "version.h"
#include "common.hpp"
#include "gamevars.hpp"
#include "mg1_display_scaling.hpp"
#include "mgs2_linkvarbuf.hpp"
#include "mgs2_status_flags.hpp"
#include "mgs3_linkvarbuf.hpp"
#include "mgs3_status_flags.hpp"
#include "version_checking.hpp"

namespace
{
    float fMGCropOffset = 0.0f;

    const char* kTextShader = R"(
    cbuffer CB : register(b0)
    {
        float2 screenSize;
        float2 padding;
    };

    struct VSInput
    {
        float3 position : POSITION;
        float4 color : COLOR;
    };

    struct PSInput
    {
        float4 position : SV_Position;
        float4 color : COLOR;
    };

    PSInput VS(VSInput input)
    {
        PSInput output;

        output.position.x = input.position.x / screenSize.x * 2.0 - 1.0;
        output.position.y = 1.0 - input.position.y / screenSize.y * 2.0;
        output.position.z = 0.0;
        output.position.w = 1.0;
        output.color = input.color;

        return output;
    }

    float4 PS(PSInput input) : SV_Target
    {
        return input.color;
    }
    )";

    struct EasyFontVertex
    {
        float x;
        float y;
        float z;
        uint8_t color[4];
    };

    struct TextVertex
    {
        float x;
        float y;
        float z;
        uint8_t color[4];
    };

    struct TextConstants
    {
        float screenWidth;
        float screenHeight;
        float padding[2];
    };

    struct TextColor
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    struct FadeState
    {
        std::chrono::steady_clock::time_point fadeStart;
        std::chrono::steady_clock::time_point lastCall;
        bool bStarted = false;
    };

    enum class TextHorizontalAlignment
    {
        Left,
        Center,
        Right
    };

    enum class TextVerticalAlignment
    {
        Top,
        Center,
        Bottom
    };

    constexpr UINT MaxTextVertices = 24576;
    constexpr size_t EasyFontBufferSize = 1024 * 64;

    ComPtr<ID3DBlob>                vsBlob;
    ComPtr<ID3DBlob>                psBlob;
    ComPtr<ID3D11VertexShader>      vs;
    ComPtr<ID3D11PixelShader>       ps;
    ComPtr<ID3D11InputLayout>       inputLayout;
    ComPtr<ID3D11Buffer>            vertexBuffer;
    ComPtr<ID3D11Buffer>            constantBuffer;
    ComPtr<ID3D11BlendState>        blend;
    ComPtr<ID3D11RasterizerState>   rs;
    ComPtr<ID3D11DepthStencilState> dss;

    std::array<uint8_t, EasyFontBufferSize> easyFontBuffer;
    std::vector<TextVertex> textVertices;

    void AddTextGeometry(const char* text, float x, float y, float scale, const TextColor& color)
    {
        unsigned char stbColor[4] = { color.r, color.g, color.b, color.a };

        int quadCount = stb_easy_font_print(0.0f, 0.0f, const_cast<char*>(text), stbColor, easyFontBuffer.data(), static_cast<int>(easyFontBuffer.size()));
        if (quadCount <= 0)
        {
            return;
        }

        const auto* sourceVertices = reinterpret_cast<const EasyFontVertex*>(easyFontBuffer.data());

        const auto addVertex = [&](const EasyFontVertex& source)
            {
                if (textVertices.size() >= MaxTextVertices)
                {
                    return;
                }

                TextVertex vertex = {};
                vertex.x = x + source.x * scale;
                vertex.y = y + source.y * scale;
                vertex.z = 0.0f;
                vertex.color[0] = color.r;
                vertex.color[1] = color.g;
                vertex.color[2] = color.b;
                vertex.color[3] = color.a;
                textVertices.push_back(vertex);
            };

        for (int i = 0; i < quadCount; i++)
        {
            const EasyFontVertex* quad = &sourceVertices[i * 4];

            addVertex(quad[0]);
            addVertex(quad[1]);
            addVertex(quad[2]);

            addVertex(quad[0]);
            addVertex(quad[2]);
            addVertex(quad[3]);
        }
    }

    void Draw(const char* text, float x, float y, float scale, float borderSize, const TextColor& textColor, const TextColor& borderColor, TextHorizontalAlignment horizontalAlignment = TextHorizontalAlignment::Left, TextVerticalAlignment verticalAlignment = TextVerticalAlignment::Top)
    {
        if (!D3D11TextOverlay::bShaderLoaded || !text || !text[0])
        {
            return;
        }

        auto* swap = g_D3D11Hooks.swapChain.Get();
        auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
        auto* dev = g_D3D11Hooks.d3dDevice.Get();

        if (!swap || !ctx || !dev)
        {
            return;
        }

        ComPtr<ID3D11Texture2D> backbuffer;
        HRESULT hr = swap->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backbuffer.GetAddressOf()));
        if (FAILED(hr))
        {
            return;
        }

        D3D11_TEXTURE2D_DESC backbufferDesc = {};
        backbuffer->GetDesc(&backbufferDesc);

        const float resolutionScale = static_cast<float>(backbufferDesc.Height) / 2160.0f;
        const float anchorX = x * resolutionScale;
        float scaledY = y * resolutionScale;
        const float scaledFont = scale * resolutionScale;
        const float scaledBorder = borderSize * resolutionScale;
        const float textHeight = static_cast<float>(stb_easy_font_height(const_cast<char*>(text))) * scaledFont;


        switch (verticalAlignment)
        {
        case TextVerticalAlignment::Center:
            scaledY -= textHeight * 0.5f;
            break;
        case TextVerticalAlignment::Bottom:
            scaledY -= textHeight;
            break;
        case TextVerticalAlignment::Top:
        default:
            break;
        }

        textVertices.clear();

        const size_t estimatedVertices = strlen(text) * 6 * 9;
        textVertices.reserve(std::min(estimatedVertices, static_cast<size_t>(MaxTextVertices)));

        float lineY = scaledY;
        const char* lineStart = text;

        while (true)
        {
            const char* lineEnd = strchr(lineStart, '\n');
            const std::string line = lineEnd ? std::string(lineStart, lineEnd) : std::string(lineStart);
            const float lineWidth = static_cast<float>(stb_easy_font_width(const_cast<char*>(line.c_str()))) * scaledFont;
            const float lineHeight = static_cast<float>(stb_easy_font_height(const_cast<char*>(line.c_str()))) * scaledFont;
            float lineX = anchorX;

            switch (horizontalAlignment)
            {
            case TextHorizontalAlignment::Center:
                lineX -= lineWidth * 0.5f;
                break;
            case TextHorizontalAlignment::Right:
                lineX -= lineWidth;
                break;
            case TextHorizontalAlignment::Left:
            default:
                break;
            }

            if (!line.empty())
            {
                if (borderSize > 0.0f && borderColor.a > 0)
                {
                    AddTextGeometry(line.c_str(), lineX - scaledBorder, lineY - scaledBorder, scaledFont, borderColor);
                    AddTextGeometry(line.c_str(), lineX, lineY - scaledBorder, scaledFont, borderColor);
                    AddTextGeometry(line.c_str(), lineX + scaledBorder, lineY - scaledBorder, scaledFont, borderColor);
                    AddTextGeometry(line.c_str(), lineX - scaledBorder, lineY, scaledFont, borderColor);
                    AddTextGeometry(line.c_str(), lineX + scaledBorder, lineY, scaledFont, borderColor);
                    AddTextGeometry(line.c_str(), lineX - scaledBorder, lineY + scaledBorder, scaledFont, borderColor);
                    AddTextGeometry(line.c_str(), lineX, lineY + scaledBorder, scaledFont, borderColor);
                    AddTextGeometry(line.c_str(), lineX + scaledBorder, lineY + scaledBorder, scaledFont, borderColor);
                }

                AddTextGeometry(line.c_str(), lineX, lineY, scaledFont, textColor);
            }

            if (!lineEnd)
                break;

            lineY += lineHeight;
            lineStart = lineEnd + 1;
        }


        if (textVertices.empty())
        {
            return;
        }

        ComPtr<ID3D11RenderTargetView> rtv;
        hr = dev->CreateRenderTargetView(backbuffer.Get(), nullptr, rtv.GetAddressOf());
        if (FAILED(hr))
        {
            return;
        }

        D3D11_MAPPED_SUBRESOURCE mapped = {};

        hr = ctx->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr))
        {
            return;
        }

        memcpy(mapped.pData, textVertices.data(), textVertices.size() * sizeof(TextVertex));
        ctx->Unmap(vertexBuffer.Get(), 0);

        hr = ctx->Map(constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr))
        {
            return;
        }

        auto* constants = static_cast<TextConstants*>(mapped.pData);
        constants->screenWidth = static_cast<float>(backbufferDesc.Width);
        constants->screenHeight = static_cast<float>(backbufferDesc.Height);
        constants->padding[0] = 0.0f;
        constants->padding[1] = 0.0f;

        ctx->Unmap(constantBuffer.Get(), 0);

        ID3D11RenderTargetView* oldRTV[8] = {};
        ID3D11DepthStencilView* oldDSV = nullptr;
        ID3D11BlendState* oldBlend = nullptr;
        ID3D11DepthStencilState* oldDSS = nullptr;
        ID3D11RasterizerState* oldRS = nullptr;
        ID3D11VertexShader* oldVS = nullptr;
        ID3D11PixelShader* oldPS = nullptr;
        ID3D11GeometryShader* oldGS = nullptr;
        ID3D11InputLayout* oldIL = nullptr;
        ID3D11Buffer* oldVSCB[1] = {};
        ID3D11Buffer* oldVB[1] = {};
        D3D11_PRIMITIVE_TOPOLOGY oldTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        D3D11_VIEWPORT oldVP[1] = {};
        UINT oldBlendMask = 0;
        UINT oldStencilRef = 0;
        UINT oldStride = 0;
        UINT oldOffset = 0;
        UINT numVP = 1;
        float oldBlendFactor[4] = {};

        ctx->OMGetRenderTargets(8, oldRTV, &oldDSV);
        ctx->OMGetBlendState(&oldBlend, oldBlendFactor, &oldBlendMask);
        ctx->OMGetDepthStencilState(&oldDSS, &oldStencilRef);
        ctx->RSGetState(&oldRS);
        ctx->RSGetViewports(&numVP, oldVP);
        ctx->VSGetShader(&oldVS, nullptr, nullptr);
        ctx->PSGetShader(&oldPS, nullptr, nullptr);
        ctx->GSGetShader(&oldGS, nullptr, nullptr);
        ctx->IAGetInputLayout(&oldIL);
        ctx->IAGetPrimitiveTopology(&oldTopology);
        ctx->IAGetVertexBuffers(0, 1, oldVB, &oldStride, &oldOffset);
        ctx->VSGetConstantBuffers(0, 1, oldVSCB);

        D3D11_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(backbufferDesc.Width), static_cast<float>(backbufferDesc.Height), 0.0f, 1.0f };
        UINT stride = sizeof(TextVertex);
        UINT offset = 0;
        ID3D11Buffer* buffer = vertexBuffer.Get();

        ctx->IASetInputLayout(inputLayout.Get());
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
        ctx->VSSetShader(vs.Get(), nullptr, 0);
        ctx->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
        ctx->PSSetShader(ps.Get(), nullptr, 0);
        ctx->GSSetShader(nullptr, nullptr, 0);
        ctx->RSSetState(rs.Get());
        ctx->RSSetViewports(1, &viewport);
        ctx->OMSetBlendState(blend.Get(), nullptr, 0xFFFFFFFF);
        ctx->OMSetDepthStencilState(dss.Get(), 0);
        ctx->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);

        ctx->Draw(static_cast<UINT>(textVertices.size()), 0);

        ctx->OMSetRenderTargets(8, oldRTV, oldDSV);
        ctx->OMSetBlendState(oldBlend, oldBlendFactor, oldBlendMask);
        ctx->OMSetDepthStencilState(oldDSS, oldStencilRef);
        ctx->RSSetState(oldRS);
        ctx->RSSetViewports(numVP, oldVP);
        ctx->VSSetShader(oldVS, nullptr, 0);
        ctx->VSSetConstantBuffers(0, 1, oldVSCB);
        ctx->PSSetShader(oldPS, nullptr, 0);
        ctx->GSSetShader(oldGS, nullptr, 0);
        ctx->IASetInputLayout(oldIL);
        ctx->IASetPrimitiveTopology(oldTopology);
        ctx->IASetVertexBuffers(0, 1, oldVB, &oldStride, &oldOffset);

        for (auto* renderTarget : oldRTV)
        {
            if (renderTarget)
            {
                renderTarget->Release();
            }
        }

        if (oldDSV) oldDSV->Release();
        if (oldBlend) oldBlend->Release();
        if (oldDSS) oldDSS->Release();
        if (oldRS) oldRS->Release();
        if (oldVS) oldVS->Release();
        if (oldPS) oldPS->Release();
        if (oldGS) oldGS->Release();
        if (oldIL) oldIL->Release();
        if (oldVSCB[0]) oldVSCB[0]->Release();
        if (oldVB[0]) oldVB[0]->Release();
    }

    void DrawFading(const char* text, float x, float y, float scale, float borderSize, const TextColor& textColor, const TextColor& borderColor, float maxAlpha, float fadeInSeconds, float fadeOutSeconds, FadeState& state, TextHorizontalAlignment horizontalAlignment = TextHorizontalAlignment::Left, TextVerticalAlignment verticalAlignment = TextVerticalAlignment::Top)
    {
        using Clock = std::chrono::steady_clock;

        constexpr float resetAfterSeconds = 0.25f;

        const auto currentTime = Clock::now();

        if (!state.bStarted || std::chrono::duration<float>(currentTime - state.lastCall).count() >= resetAfterSeconds)
        {
            state.fadeStart = currentTime;
            state.bStarted = true;
        }

        state.lastCall = currentTime;

        const float totalDuration = fadeInSeconds + fadeOutSeconds;
        if (totalDuration <= 0.0f)
        {
            return;
        }

        const float elapsedSeconds = std::chrono::duration<float>(currentTime - state.fadeStart).count();
        const float cycleTime = std::fmod(elapsedSeconds, totalDuration);

        float alpha;

        if (cycleTime < fadeInSeconds)
        {
            alpha = fadeInSeconds > 0.0f ? cycleTime / fadeInSeconds : 1.0f;
        }
        else
        {
            alpha = fadeOutSeconds > 0.0f ? 1.0f - ((cycleTime - fadeInSeconds) / fadeOutSeconds) : 0.0f;
        }

        alpha = std::clamp(alpha, 0.0f, 1.0f);
        alpha = alpha * alpha * (3.0f - 2.0f * alpha);

        maxAlpha = std::clamp(maxAlpha, 0.0f, 1.0f);

        TextColor fadedTextColor = textColor;
        TextColor fadedBorderColor = borderColor;

        fadedTextColor.a = static_cast<uint8_t>(std::round(static_cast<float>(textColor.a) * maxAlpha * alpha));
        fadedBorderColor.a = static_cast<uint8_t>(std::round(static_cast<float>(borderColor.a) * maxAlpha * alpha));

        Draw(text, x, y, scale, borderSize, fadedTextColor, fadedBorderColor, horizontalAlignment, verticalAlignment);
    }


    bool bShowVersionNumber = true;
    bool bShowUpdateNotification = false;
    std::string FormatFrameTime(const int frameCount, const int frameRate = 60)
    {
        const int64_t totalMilliseconds = (static_cast<int64_t>(frameCount) * 1000) / frameRate;
        const int hours = static_cast<int>(totalMilliseconds / 3600000);
        const int minutes = static_cast<int>((totalMilliseconds / 60000) % 60);
        const int seconds = static_cast<int>((totalMilliseconds / 1000) % 60);
        const int milliseconds = static_cast<int>(totalMilliseconds % 1000);
        if (hours > 0)
            return std::format("{}:{:02}:{:02}.{:03}", hours, minutes, seconds, milliseconds);
        if (minutes > 0)
            return std::format("{}:{:02}.{:03}", minutes, seconds, milliseconds);
        if (seconds > 0)
            return std::format("{}.{:03}", seconds, milliseconds);
        return std::to_string(milliseconds);
    }

    using StatClock = std::chrono::steady_clock;
    inline StatClock::time_point g_RealElapsedStartTime = StatClock::now();
    inline StatClock::time_point g_RealElapsedPauseTime;
    inline bool g_RealElapsedPaused = false;

    void ResetRealElapsedTime()
    {
        g_RealElapsedStartTime = StatClock::now();
        g_RealElapsedPaused = false;
    }

    void PauseRealElapsedTime()
    {
        if (!g_RealElapsedPaused)
        {
            g_RealElapsedPauseTime = StatClock::now();
            g_RealElapsedPaused = true;
        }
    }

    std::string GetRealElapsedTime()
    {
        const auto currentTime = g_RealElapsedPaused ? g_RealElapsedPauseTime : StatClock::now();
        const auto totalMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - g_RealElapsedStartTime).count();
        const auto hours = totalMilliseconds / 3600000;
        const auto minutes = (totalMilliseconds / 60000) % 60;
        const auto seconds = (totalMilliseconds / 1000) % 60;
        const auto milliseconds = totalMilliseconds % 1000;
        char buffer[32];
        if (hours > 0)
            std::snprintf(buffer, sizeof(buffer), "%lld:%02lld:%02lld.%03lld", hours, minutes, seconds, milliseconds);
        else if (minutes > 0)
            std::snprintf(buffer, sizeof(buffer), "%lld:%02lld.%03lld", minutes, seconds, milliseconds);
        else
            std::snprintf(buffer, sizeof(buffer), "%lld.%03lld", seconds, milliseconds);
        return buffer;
    }

    void ReverseStatLines(std::string& text)
    {
        std::vector<std::string> lines;
        size_t start = 0;

        while (start <= text.size())
        {
            const size_t end = text.find('\n', start);
            lines.emplace_back(text.substr(start, end - start));

            if (end == std::string::npos)
            {
                break;
            }

            start = end + 1;
        }

        while (!lines.empty() && lines.back().empty())
        {
            lines.pop_back();
        }

        std::reverse(lines.begin(), lines.end());
        text.clear();

        for (size_t i = 0; i < lines.size(); ++i)
        {
            if (i > 0)
            {
                text += '\n';
            }

            text += lines[i];
        }
    }

    const char* GetStatOverlay()
    {
        static std::string text;
        if (eGameType & MGS2)
        {
            using namespace MGS2_StatusFlags;
            using namespace MGS2_LinkVarBuf;
            text = FormatFrameTime(GM_StagePlayTime) + " / " + FormatFrameTime(GM_PlayTime) + " / " + GetRealElapsedTime();
            if (g_GameVars.GV_PauseLevel() & MGS2_GV_PAUSE_PAUSE)
            {
                text += "\n"
                    "Alerts: " + std::to_string(GM_AlertCount) + "\n"
                    "Continues: " + std::to_string(GM_ContinueCount) + "\n"
                    "Kills: " + std::to_string(GM_KillCount) + "\n"
                    "Saves: " + std::to_string(GM_SaveCount) + "\n"
                    "Shots: " + std::to_string(GM_ShootCount) + "\n"
                    "Rations: " + std::to_string(GM_RationUseCount) + "\n"
                    + (GM_ClearCodeFlag & GM_CLEAR_RADAR_USED ? "Radar used: Yes\n" : "")
                    + (GM_ClearCodeFlag & GM_CLEAR_SPECIAL_ITEM_USED ? "Special item used: Yes\n" : "")
                ;
                if (D3D11TextOverlay::iStatsPosition == D3D11TextOverlay::BottomLeft || D3D11TextOverlay::iStatsPosition == D3D11TextOverlay::BottomRight)
                {
                    ReverseStatLines(text);
                }
            }

            return text.c_str();
        }
        if (eGameType & MGS3)
        {
            using namespace MGS3_StatusFlags;
            using namespace MGS3_LinkVarBuf;
            text = FormatFrameTime(GM_StagePlayTime, 300) + " / " + FormatFrameTime(GM_PlayTime) + " / " + GetRealElapsedTime();
            if (g_GameVars.GV_PauseLevel() & MGS3_GV_PAUSE_PAUSE && !g_GameVars.InCutscene())
            {
                text += "\n"
                   "Alerts: " + std::to_string(GM_AlertCount) + "\n"
                   "Continues: " + std::to_string(GM_ContinueCount) + "\n"
                   "Kills: " + std::to_string(GM_KillCount) + "\n"
                   "Injuries: " + std::to_string(GM_InjuryCount) + "\n"
                   "Lifebars: " + std::to_string(GM_LifebarDamageCount) + "\n"
                   "Life Meds: " + std::to_string(GM_LifeMedicineUseCount) + "\n"
                   "Meals: " + std::to_string(GM_MealCount) + "\n"
                   "Saves: " + std::to_string(GM_SaveCount) + "\n"
                   ;
                if (D3D11TextOverlay::iStatsPosition == D3D11TextOverlay::BottomLeft || D3D11TextOverlay::iStatsPosition == D3D11TextOverlay::BottomRight)
                {
                    ReverseStatLines(text);
                }
            }
            return text.c_str();
        }
        return "null";
    }

    bool isCurrentlyMenu = true;
}

void D3D11TextOverlay::Setup()
{

    if (!g_D3D11Hooks.D3DCompileFunc)
    {
        spdlog::error("D3D11TextOverlay: Failed to get D3DCompile");
        return;
    }

    ComPtr<ID3DBlob> err;

    HRESULT hr = g_D3D11Hooks.D3DCompileFunc(kTextShader, strlen(kTextShader), nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0, vsBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("D3D11TextOverlay: Failed to compile vertex shader: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "Unknown error");
        return;
    }

    err.Reset();

    hr = g_D3D11Hooks.D3DCompileFunc(kTextShader, strlen(kTextShader), nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, psBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("D3D11TextOverlay: Failed to compile pixel shader: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "Unknown error");
        vsBlob.Reset();
        return;
    }

    spdlog::info("D3D11TextOverlay: Compiled text shader successfully");

}

void D3D11TextOverlay::Init()
{
    if (bShaderLoaded)
    {
        return;
    }

    ID3D11Device* dev = g_D3D11Hooks.d3dDevice.Get();
    if (!dev)
    {
        spdlog::error("D3D11TextOverlay: D3D11 device is not initialized");
        return;
    }

    if (!vsBlob || !psBlob)
    {
        spdlog::error("D3D11TextOverlay: Shader bytecode was not compiled");
        return;
    }

    HRESULT hr = dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, vs.GetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("D3D11TextOverlay: Failed to create vertex shader. HRESULT: 0x{:08X}", static_cast<uint32_t>(hr));
        return;
    }

    hr = dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, ps.GetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("D3D11TextOverlay: Failed to create pixel shader. HRESULT: 0x{:08X}", static_cast<uint32_t>(hr));
        return;
    }

    D3D11_INPUT_ELEMENT_DESC inputElements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(TextVertex, x), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, offsetof(TextVertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = dev->CreateInputLayout(inputElements, ARRAYSIZE(inputElements), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), inputLayout.GetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("D3D11TextOverlay: Failed to create input layout. HRESULT: 0x{:08X}", static_cast<uint32_t>(hr));
        return;
    }

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.ByteWidth = MaxTextVertices * sizeof(TextVertex);
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = dev->CreateBuffer(&vertexBufferDesc, nullptr, vertexBuffer.GetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("D3D11TextOverlay: Failed to create vertex buffer. HRESULT: 0x{:08X}", static_cast<uint32_t>(hr));
        return;
    }

    D3D11_BUFFER_DESC constantBufferDesc = {};
    constantBufferDesc.ByteWidth = sizeof(TextConstants);
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = dev->CreateBuffer(&constantBufferDesc, nullptr, constantBuffer.GetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("D3D11TextOverlay: Failed to create constant buffer. HRESULT: 0x{:08X}", static_cast<uint32_t>(hr));
        return;
    }

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = dev->CreateBlendState(&blendDesc, blend.GetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("D3D11TextOverlay: Failed to create blend state. HRESULT: 0x{:08X}", static_cast<uint32_t>(hr));
        return;
    }

    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.DepthClipEnable = TRUE;

    hr = dev->CreateRasterizerState(&rasterizerDesc, rs.GetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("D3D11TextOverlay: Failed to create rasterizer state. HRESULT: 0x{:08X}", static_cast<uint32_t>(hr));
        return;
    }

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = FALSE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

    hr = dev->CreateDepthStencilState(&depthStencilDesc, dss.GetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("D3D11TextOverlay: Failed to create depth stencil state. HRESULT: 0x{:08X}", static_cast<uint32_t>(hr));
        return;
    }

    textVertices.reserve(MaxTextVertices);

    vsBlob.Reset();
    psBlob.Reset();

    bShaderLoaded = true;
    spdlog::info("D3D11TextOverlay initialized.");

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 33 D2 E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 41 B9 ?? ?? ?? ?? B2 ?? 45 8D 41 ?? E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 33 D2 E8 ?? ?? ?? ?? 48 8B 0D", "BP_RenderLoadingSpinner -> l1549", {
        bShowVersionNumber = true;
                  });


    if (eGameType & MGS2)
    {
        /*
        //die isn't really needed, it's just here as a redundant cleanup.
        MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 33 D2 E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 41 B9 ?? ?? ?? ?? B2 ?? 45 8D 41 ?? E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 33 D2 E8 ?? ?? ?? ?? 48 8B 0D", "NewTitleScrMan -> Die()", {
        bShowUpdateNotification = false;
                      });
                      */
        MAKE_HOOK_MID(baseModule, "B8 ?? ?? ?? ?? BA ?? ?? ?? ?? 0F 45 C2 89 41", "NewTitleScrMan -> SignalFunc() -> CODE_FACECLOSE", {
        bShowUpdateNotification = false;
                      });

        MAKE_HOOK_MID(baseModule, "B8 ?? ?? ?? ?? BA ?? ?? ?? ?? 0F 44 C2 EB ?? 83 B9 ?? ?? ?? ?? 00", "NewTitleScrMan -> SignalFunc() -> CODE_FACEOPEN", {
        bShowUpdateNotification = true;
                      });
    }

    
    if ((eGameType & MG) && MG1_DisplayScaling::bCropBorders)
    {
        fMGCropOffset = -300.0f;
    }
}

void D3D11TextOverlay::HandleLevelTransition()
{
    if (!bShowSpeedrunnerStats)
    {
        return;
    }
    if (!(eGameType & (MGS2 | MGS3)))
    {
        return;
    }
    static bool wasMenu = false;
    isCurrentlyMenu = eGameType & MGS2 ? (g_GameVars.MGS2_GetGameMode() == MGS2GameMode::Menu) : (g_GameVars.MGS3_GetGameMode() == MGS3GameMode::Menu);
    if (isCurrentlyMenu)
    {
        PauseRealElapsedTime();
    }
    else if (wasMenu)
    {
        ResetRealElapsedTime();
    }
    wasMenu = isCurrentlyMenu;
}



void D3D11TextOverlay::Tick()
{
    if (!bShaderLoaded || !g_D3D11Hooks.swapChain)
    {
        return;
    }



    if (bShowVersionNumber)
    {
        Draw(sFixNameAndVersion.c_str(), 3167.0f, 2005.0f + fMGCropOffset, 5.0f, 1.0f, {199, 199, 199, 255 }, {0,0,0,0}, TextHorizontalAlignment::Center);
        bShowVersionNumber = false;
		if(bDebugBuild)
		{
			Draw("Nightly Build", 3167.0f, 2090.0f + fMGCropOffset, 5.0f, 1.0f, {199, 199, 199, 255 }, {0,0,0,0}, TextHorizontalAlignment::Center, TextVerticalAlignment::Center);
		}
    }

    if (bShowUpdateNotification && (bNewUpdateAvailable || bDebugBuild))
    {
        constexpr float x_pos = 3840.0f * 0.5;
        constexpr float y_pos = 2020.0f;
        if (bNewUpdateAvailable)
        {
            static FadeState versionFade;
            DrawFading("A new version of MGSHDFix is available for download!", x_pos, y_pos, 6.0f, 1.0f, {160, 160, 160, 255}, {0, 0, 0, 255}, 1.0f, 0.5, 1.5f, versionFade, TextHorizontalAlignment::Center, TextVerticalAlignment::Center);
        }
        else
        {
            Draw(("MGSHDFix Nightly Build v" + sFixVersion).c_str(), x_pos, y_pos, 6.0f, 1.0f, {160, 160, 160, 255}, {0,0,0,0}, TextHorizontalAlignment::Center, TextVerticalAlignment::Center);
        }
    }

    if (bShowSpeedrunnerStats && !isCurrentlyMenu)
    {
        float x = 3830.0f;
        float y = 20.0f;
        TextHorizontalAlignment horizontalAlignment = TextHorizontalAlignment::Right;
        TextVerticalAlignment verticalAlignment = TextVerticalAlignment::Top;

        switch (iStatsPosition)
        {
            case TopLeft:
            {
                x = 10.0f;
                horizontalAlignment = TextHorizontalAlignment::Left;
                break;
            }
            case TopRight:
            {
                break;
            }
            case BottomLeft:
            {
                x = 10.0f;
                y = 2140.0f;
                horizontalAlignment = TextHorizontalAlignment::Left;
                verticalAlignment = TextVerticalAlignment::Bottom;
                break;
            }
            case BottomRight:
            {
                y = 2140.0f;
                verticalAlignment = TextVerticalAlignment::Bottom;
                break;
            }
        }

        Draw(GetStatOverlay(), x, y, 4.0f, 3.0f, { 199, 199, 199, 255 }, { 0, 0, 0, 128 }, horizontalAlignment, verticalAlignment);
    }
}
