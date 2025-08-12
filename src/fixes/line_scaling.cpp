#include "common.hpp"
#include "line_scaling.hpp"
#include "d3d11_api.hpp"
#include <d3dcompiler.h>

#include "config.hpp"
#include "input_handler.hpp"
#include "logging.hpp"

namespace
{
    // Shader objects
    ComPtr<ID3D11GeometryShader> geometryShader;
    ComPtr<ID3DBlob> compiledShaderBytecode;

    // CB for pixel-accurate line expansion
    ComPtr<ID3D11Buffer> gsParamsCB;

    struct GSParamsCPU
    {
        float invViewport[2];   // 1/width, 1/height for current viewport
        float pixelThickness;   // desired line thickness in viewport pixels
        float _pad0;
    };

    // Hooks
    SafetyHookInline MGS3_DrawIndexedPrimitive_Hook {};
    SafetyHookInline D3D11_DrawInstanced_Hook {};
    SafetyHookInline D3D11_Draw_Hook {};

    using DrawFn = void(__stdcall*)(ID3D11DeviceContext* context, UINT VertexCount, UINT StartVertexLocation);

    // Fill and bind GS constant buffer for the current viewport.
    // Needed to scale the radar, which is handled in a separate UI pass with its own smaller viewport.
    inline void UpdateGSParamsCB(ID3D11DeviceContext* ctx, bool isUI)
    {
        if (!ctx || !gsParamsCB)
            return;
        D3D11_VIEWPORT vp {};
        UINT num = 1;
        ctx->RSGetViewports(&num, &vp);
        if (num == 0 || vp.Width <= 0.0f || vp.Height <= 0.0f)
            return;

        D3D11_MAPPED_SUBRESOURCE mapped {};
        if (SUCCEEDED(ctx->Map(gsParamsCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* p = reinterpret_cast<GSParamsCPU*>(mapped.pData);
            p->invViewport[0] = 1.0f / vp.Width;
            p->invViewport[1] = 1.0f / vp.Height;
            p->_pad0 = 0.0f;
            ctx->Unmap(gsParamsCB.Get(), 0);
        }

        ID3D11Buffer* cb = gsParamsCB.Get();
        ctx->GSSetConstantBuffers(0, 1, &cb);
    }


    uint64_t MGS3_DrawIndexedPrimitive_Hooked(void* CD3DCachedDevice, int topologyType, int BaseVertexIndex, int MinVertexIndex, int NumVertices, int startIndex, int primCount)
    { // This is called every frame. Do not add logging or it will nuke I/O.
        // Wireframe toggle: expand triangles as lines via GS
        if (g_VectorScalingFix.bToggleWireframe && (topologyType == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST || (g_VectorScalingFix.bToggleWireframe > 1 && topologyType == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP)))
        {
            UpdateGSParamsCB(g_D3D11Hooks.d3dDeviceContext.Get(), false);
            g_D3D11Hooks.d3dDeviceContext->GSSetShader(geometryShader.Get(), nullptr, 0);
            const auto ret = MGS3_DrawIndexedPrimitive_Hook.call<uint64_t>(CD3DCachedDevice, topologyType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
            g_D3D11Hooks.d3dDeviceContext->GSSetShader(nullptr, nullptr, 0);
            return ret;
        }

        // World-space line expansion (rain/laser/bullet): points and lines
        if (g_VectorScalingFix.bToggleRainShader && (topologyType == D3D11_PRIMITIVE_TOPOLOGY_POINTLIST || topologyType == D3D11_PRIMITIVE_TOPOLOGY_LINELIST))
        {
            UpdateGSParamsCB(g_D3D11Hooks.d3dDeviceContext.Get(), false);
            g_D3D11Hooks.d3dDeviceContext->GSSetShader(geometryShader.Get(), nullptr, 0);
            const auto ret = MGS3_DrawIndexedPrimitive_Hook.call<uint64_t>(CD3DCachedDevice, topologyType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
            g_D3D11Hooks.d3dDeviceContext->GSSetShader(nullptr, nullptr, 0);
            return ret;
        }

        return MGS3_DrawIndexedPrimitive_Hook.call<uint64_t>(CD3DCachedDevice, topologyType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
    }

    void __stdcall HookedDraw(ID3D11DeviceContext* context, UINT VertexCount, UINT StartVertexLocation)
    {
        D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        context->IAGetPrimitiveTopology(&topology);

        if (g_VectorScalingFix.bToggleUIShader && (topology == D3D11_PRIMITIVE_TOPOLOGY_LINELIST || topology == D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP))
        {
            ID3D11GeometryShader* currentGS = nullptr;
            context->GSGetShader(&currentGS, nullptr, nullptr);

            if (!currentGS && geometryShader)
            {
                UpdateGSParamsCB(context, true);
                context->GSSetShader(geometryShader.Get(), nullptr, 0);
            }

            D3D11_Draw_Hook.call<void>(context, VertexCount, StartVertexLocation);

            if (!currentGS && geometryShader)
            {
                context->GSSetShader(nullptr, nullptr, 0);
            }

            if (currentGS)
            {
                currentGS->Release();
            }
        }
        else
        {
            D3D11_Draw_Hook.call<void>(context, VertexCount, StartVertexLocation);
        }
    }

}


void VectorScalingFix::LoadCompiledShader() const
{
    if (!(eGameType & (MGS2 | MGS3)))
    {
        return;
    }
    spdlog::info("MGS 2 | MGS 3: Vector Line Fix - Load Shader: Attempting to load compiled geometry shader...");

    if (geometryShader || !compiledShaderBytecode)
    {
        spdlog::error("MGS 2 | MGS 3: Vector Line Fix - Load Shader: Geometry shader or compiled shader bytecode already exists.");
        return;
    }

    if (!g_D3D11Hooks.d3dDevice)
    {
        spdlog::error("MGS 2 | MGS 3: Vector Line Fix - Load Shader: D3D11 device is not initialized.");
        return;
    }

    HRESULT result = g_D3D11Hooks.d3dDevice->CreateGeometryShader(
        compiledShaderBytecode->GetBufferPointer(),
        compiledShaderBytecode->GetBufferSize(),
        nullptr,
        geometryShader.GetAddressOf()
    );

    if (FAILED(result))
    {
        spdlog::error("MGS 2 | MGS 3: Vector Line Fix - Load Shader: Failed to create geometry shader on device");
        return;
    }

    // Create dynamic CB once
    if (!gsParamsCB)
    {
        D3D11_BUFFER_DESC cbd {};
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.ByteWidth = sizeof(GSParamsCPU);
        cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbd.Usage = D3D11_USAGE_DYNAMIC;

        if (HRESULT cbhr = g_D3D11Hooks.d3dDevice->CreateBuffer(&cbd, nullptr, gsParamsCB.GetAddressOf()); FAILED(cbhr))
        {
            spdlog::error("MGS 2 | MGS 3: Vector Line Fix - Load Shader: Failed to create GS params constant buffer. hr=0x{:08X}", cbhr);
            return;
        }
    }

    if (!bFixUI)
    {
        spdlog::info("MGS 2 | MGS 3: Vector Line Fix - Load Shader: UI Fix is disabled, skipping hooking Draw calls for UI Fix geometry shader...");
        spdlog::info("MGS 2 | MGS 3: Vector Line Fix - Load Shader: Successfully loaded geometry shader on device.");
        return;
    }

    if (!g_D3D11Hooks.d3dDeviceContext)
    {
        spdlog::error("MGS 2 | MGS 3: Vector Line Fix - Load Shader: D3D11 device context is not initialized.");
        spdlog::error("MGS 2 | MGS 3: Vector Line Fix - Load Shader: Failed to hook Draw calls for UI Fix geometry shader.");
        return;
    }


    spdlog::info("MGS 2 | MGS 3: Vector Line Fix - Load Shader: Hooking Draw calls to apply UI Fix geometry shader...");
    void** vtable = *reinterpret_cast<void***>(g_D3D11Hooks.d3dDeviceContext.Get());
    D3D11_Draw_Hook = safetyhook::create_inline(vtable[13], reinterpret_cast<void*>(HookedDraw));
    LOG_HOOK(D3D11_Draw_Hook, "ID3D11DeviceContext::Draw");

    spdlog::info("MGS 2 | MGS 3: Vector Line Fix - Load Shader: Successfully loaded geometry shader on device.");

    g_InputHandler.RegisterHotkey(vkUIShaderToggle, "UI Shader Toggle", []()
        {
            g_VectorScalingFix.bToggleUIShader = !g_VectorScalingFix.bToggleUIShader;
        });
}

bool VectorScalingFix::CompileGeometryShader()
{
    HMODULE d3dcompiler = LoadLibraryA("d3dcompiler_43.dll");
    if (!d3dcompiler)
    {
        spdlog::error("MGS 2 | MGS 3: Vector Line Fix - CompileGeometryShader: Failed to load d3dcompiler_43.dll");
        return false;
    }

    pD3DCompile D3DCompileFunc = reinterpret_cast<pD3DCompile>(GetProcAddress(d3dcompiler, "D3DCompile"));
    if (!D3DCompileFunc)
    {
        spdlog::error("MGS 2 | MGS 3: Vector Line Fix - CompileGeometryShader: Failed to get address for D3DCompile");
        return false;
    }

    if (iVectorLineScale < 1)
    {
        spdlog::info("MGS 2 | MGS 3: Vector Line Fix - CompileGeometryShader: Invalid line scale! Defaulting to 360");
        iVectorLineScale = DEFAULT_LINE_SCALE;
    }
    if(iVectorLineScale < DEFAULT_LINE_SCALE*0.5)
    {
        spdlog::warn("Config Warning");
        spdlog::warn("Line scale is currently set to more that double the default size of Screen Height / 360 (6 pixels wide,) with individual raindrops currently set to {} ({} pixels wide.)", iVectorLineScale, iInternalResY / iVectorLineScale);
        spdlog::warn("If you intend for line effects to be MASSIVE like this, set \"Silence Scaling Warnings\" to true in the config");
        Logging::ShowConsole();
        std::cout << "MGSHDFix Config Warning:\n"
            "Line scale is currently set to more that double the default size of Screen Height/360 (" << iInternalResY / 360 << " pixels wide),\n"
            "with individual raindrops currently set to " << iVectorLineScale << " (" << iInternalResY / iVectorLineScale << " pixels wide.)\n"
            "If you intend for line effects to be MASSIVE like this, set \"Silence Scaling Warnings\" to true in the config";
    }
    else
    {
        spdlog::info("MGS 2 | MGS 3: Vector Line Fix - CompileGeometryShader: Line Scale before: {}", iVectorLineScale);
    }
    iVectorLineScale = round(iInternalResY / iVectorLineScale);
    spdlog::info("MGS 2 | MGS 3: Vector Line Fix - CompileGeometryShader: Target Pixel Width = : {}", iVectorLineScale);
    iVectorLineScale = (iInternalResY / iVectorLineScale);
    spdlog::info("MGS 2 | MGS 3: Vector Line Fix - CompileGeometryShader: Line Scale after rounding: {}", iVectorLineScale);
    const std::string shaderString = R"(
        cbuffer GSParams : register(b0)
        {
            float2 invViewport;     // 1/width, 1/height
            float  pixelThickness;  // desired pixels in the current viewport
            float  _pad0;
        };

        struct VS_OUTPUT {
            float4 Position : SV_Position;
            float4 param1   : TEXCOORD0;
            float4 param2   : TEXCOORD1;
        };
        struct GS_OUTPUT {
            float4 Position : SV_Position;
            float4 param1   : TEXCOORD0;
            float4 param2   : TEXCOORD1;
        };

        [maxvertexcount(4)]
        void GS_LineToQuad(line VS_OUTPUT input[2], inout TriangleStream<GS_OUTPUT> OutputStream)
        {
            float4 p0_clip = input[0].Position;
            float4 p1_clip = input[1].Position;

            float2 p0_ndc = p0_clip.xy / p0_clip.w;
            float2 p1_ndc = p1_clip.xy / p1_clip.w;

            float2 dir_ndc  = normalize(p1_ndc - p0_ndc);
            float2 perp_ndc = float2(-dir_ndc.y, dir_ndc.x);

            // NDC per pixel for the current viewport
            float2 ndc_per_pixel = 2.0 * invViewport;

            float targetPixelWidth = )" + std::to_string(static_cast<float>(iInternalResY) / static_cast<float>(std::max(1.0, g_VectorScalingFix.iVectorLineScale))) + R"(;
            // Offset half on each side
            float2 offset = perp_ndc * (0.5 * targetPixelWidth) * ndc_per_pixel;

            float2 v0_ndc = p0_ndc - offset;
            float2 v1_ndc = p0_ndc + offset;
            float2 v2_ndc = p1_ndc + offset;
            float2 v3_ndc = p1_ndc - offset;

            GS_OUTPUT v0, v1, v2, v3;
            v0.Position = float4(v0_ndc * p0_clip.w, p0_clip.z, p0_clip.w);
            v1.Position = float4(v1_ndc * p0_clip.w, p0_clip.z, p0_clip.w);
            v2.Position = float4(v2_ndc * p1_clip.w, p1_clip.z, p1_clip.w);
            v3.Position = float4(v3_ndc * p1_clip.w, p1_clip.z, p1_clip.w);

            v0.param1 = input[0].param1; v0.param2 = input[0].param2;
            v1.param1 = input[0].param1; v1.param2 = input[0].param2;
            v2.param1 = input[1].param1; v2.param2 = input[1].param2;
            v3.param1 = input[1].param1; v3.param2 = input[1].param2;

            OutputStream.Append(v0);
            OutputStream.Append(v1);
            OutputStream.Append(v3);
            OutputStream.Append(v2);
            OutputStream.RestartStrip();
        }
    )";

    ComPtr<ID3DBlob> compiledShader;
    ComPtr<ID3DBlob> errorMsgs;
    HRESULT hr = D3DCompileFunc(
        shaderString.c_str(),
        shaderString.size(),
        "geometry_shader",
        nullptr,
        nullptr,
        "GS_LineToQuad",
        "gs_4_0",
        0,
        0,
        compiledShader.GetAddressOf(),
        errorMsgs.GetAddressOf()
    );

    bNeedsCompiler = false;
    D3D11Hooks::UnloadCompiler(d3dcompiler);

    if (FAILED(hr))
    {
        if (errorMsgs)
        {
            spdlog::error("MGS 2 | MGS 3: Vector Line Fix - CompileGeometryShader: Shader compile failed with error: {}",
                static_cast<const char*>(errorMsgs->GetBufferPointer()));
        }
        else
        {
            spdlog::error("MGS 2 | MGS 3: Vector Line Fix - CompileGeometryShader: Shader compile failed with HRESULT: 0x{:08X}", hr);
        }
        return false;
    }

    compiledShaderBytecode = compiledShader;
    spdlog::info("MGS 2 | MGS 3: Vector Line Fix - CompileGeometryShader: Shader compiled successfully!");
    return true;
}

void VectorScalingFix::Initialize()
{
    if (!(eGameType & (MGS2 | MGS3)))
    {
        return;
    }

    if (!bFixRain && !bFixUI)
    {
        spdlog::info("MGS 2 | MGS 3: Vector Line Fix: Config disabled. Skipping");
        return;
    }

    if (!CompileGeometryShader())
    {
        return;
    }

    if (!bFixRain)
    {
        spdlog::info("MGS 2 | MGS 3: Vector Line Fix - Initialize: Rain Fix is disabled, not hooking Rain/Laser/Bullet effects.");
        return;
    }

    if (uint8_t* MGS3_DrawIndexedPrimitive_ScanResult = Memory::PatternScan(baseModule, "48 89 5C 24 ?? 57 48 83 EC 20 FF 41 ?? 41 8B ??", "MGS 2 | MGS 3: Vector Line Fix - DrawIndexedPrimitive"))
    {
        MGS3_DrawIndexedPrimitive_Hook = safetyhook::create_inline(reinterpret_cast<void*>(MGS3_DrawIndexedPrimitive_ScanResult), reinterpret_cast<void*>(MGS3_DrawIndexedPrimitive_Hooked));
        LOG_HOOK(MGS3_DrawIndexedPrimitive_Hook, "MGS 2 | MGS 3: Vector Line Fix - DrawIndexedPrimitive")

        g_InputHandler.RegisterHotkey(vkRainShaderToggle, "Rain Shader Toggle", []()
            {
                g_VectorScalingFix.bToggleRainShader = !g_VectorScalingFix.bToggleRainShader;
            });

        g_InputHandler.RegisterHotkey(vkWireframeToggle, "Wireframe Cycle", []()
            {
                if (g_VectorScalingFix.bToggleWireframe > 1)
                {
                    g_VectorScalingFix.bToggleWireframe = 0;
                }
                else
                {
                    g_VectorScalingFix.bToggleWireframe++;
                }
            });

    }
}
