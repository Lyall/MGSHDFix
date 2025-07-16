#include "common.hpp"
#include "line_scaling.hpp"
#include "d3d11_api.hpp"
#include <d3dcompiler.h>
#include "logging.hpp"


static SafetyHookInline MGS3_DrawIndexedPrimitive_Hook {};
uint64_t MGS3_DrawIndexedPrimitive_Hooked(void* CD3DCachedDevice, int topologyType, int BaseVertexIndex, int MinVertexIndex, int NumVertices, int startIndex, int primCount)
{ //This is called every frame, DO NOT add logging or the I/O will nuke performance.
    if(!(topologyType == 0x1 || topologyType == 0x2))
    {
        return MGS3_DrawIndexedPrimitive_Hook.call<uint64_t>(CD3DCachedDevice, topologyType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
    }
    g_D3D11Hooks.d3dDeviceContext->GSSetShader(g_D3D11Hooks.geometryShader, nullptr, 0);
    auto ret = MGS3_DrawIndexedPrimitive_Hook.call<uint64_t>(CD3DCachedDevice, topologyType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
    g_D3D11Hooks.d3dDeviceContext->GSSetShader(nullptr, nullptr, 0);
    return ret;
}

void VectorScalingFix::LoadCompiledShader()
{
    if (!g_D3D11Hooks.geometryShader && compiledShaderBytecode && g_D3D11Hooks.d3dDevice)
    {
        HRESULT result = g_D3D11Hooks.d3dDevice->CreateGeometryShader(
            compiledShaderBytecode->GetBufferPointer(),
            compiledShaderBytecode->GetBufferSize(),
            nullptr,
            &g_D3D11Hooks.geometryShader
        );

        if (FAILED(result))
            spdlog::error("MGS 2 | MGS 3: Vector Line Fix - Load Shader: Failed to create geometry shader on device");
        else
            spdlog::info("MGS 2 | MGS 3: Vector Line Fix - Load Shader: Successfully loaded geometry shader.");
    }
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
        struct VS_OUTPUT {
            float4 Position : SV_Position; 
            float4 param1 : TEXCOORD0;     
            float4 param2 : TEXCOORD1;    
        };
        struct GS_OUTPUT {
            float4 Position : SV_Position;
            float4 param1 : TEXCOORD0;
            float4 param2 : TEXCOORD1;
        };
        [maxvertexcount(4)]
        void GS_LineToQuad(line VS_OUTPUT input[2], inout TriangleStream<GS_OUTPUT> OutputStream)
        {
            float aspect = )" + std::to_string(fAspectRatio) + R"(;
            float thicknessFraction = 1.0 / )" + std::to_string(static_cast<float>(iVectorLineScale)) + R"(;
            float4 p0_clip = input[0].Position;
            float4 p1_clip = input[1].Position;
            float thicknessNDC = thicknessFraction * 2.0f;
            float2 p0_ndc = p0_clip.xy / p0_clip.w;
            float2 p1_ndc = p1_clip.xy / p1_clip.w;
            float2 dir_ndc = normalize(p1_ndc - p0_ndc);
            float2 perp_ndc = float2(-dir_ndc.y, dir_ndc.x);
            float2 offset = perp_ndc * (0.5f * thicknessNDC) * float2(1.0/aspect, 1.0);
            float2 v0_ndc = p0_ndc - offset;
            float2 v1_ndc = p0_ndc + offset;
            float2 v2_ndc = p1_ndc + offset;
            float2 v3_ndc = p1_ndc - offset;
            GS_OUTPUT v0, v1, v2, v3;
            v0.Position = float4(v0_ndc * p0_clip.w, p0_clip.z, p0_clip.w);
            v1.Position = float4(v1_ndc * p0_clip.w, p0_clip.z, p0_clip.w);
            v2.Position = float4(v2_ndc * p1_clip.w, p1_clip.z, p1_clip.w);
            v3.Position = float4(v3_ndc * p1_clip.w, p1_clip.z, p1_clip.w);
            v0.param1 = input[0].param1;
            v0.param2 = input[0].param2;
            v1.param1 = input[0].param1;
            v1.param2 = input[0].param2;
            v2.param1 = input[1].param1;
            v2.param2 = input[1].param2;
            v3.param1 = input[1].param1;
            v3.param2 = input[1].param2;
            OutputStream.Append(v0);
            OutputStream.Append(v1);
            OutputStream.Append(v3);
            OutputStream.Append(v2);
            OutputStream.RestartStrip();
        }
    )";
    const char* shaderCode = shaderString.c_str();
    ID3DBlob* compiledShader;
    ID3DBlob* errorMsgs;
    HRESULT hr = D3DCompileFunc(
        shaderCode,
        strlen(shaderCode),
        "geometry_shader",
        nullptr,
        nullptr,
        "GS_LineToQuad",
        "gs_4_0",
        0,
        0,
        &compiledShader,
        &errorMsgs
    );
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
    if (!bEnableVectorLineFix)
    {
        spdlog::info("MGS 2 | MGS 3: Vector Line Fix: Config disabled. Skipping");

        return;
    }

    if (!CompileGeometryShader())
    {
        return;
    }

    if (uint8_t* MGS3_DrawIndexedPrimitive_ScanResult = Memory::PatternScan(baseModule, "48 89 5C 24 ?? 57 48 83 EC 20 FF 41 ?? 41 8B ??", "MGS 2 | MGS 3: Vector Line Fix - DrawIndexedPrimitive"))
    {   //Technically only needed for MGS3. MGS2 does have the function as well, but it's not used. Let's patch it anyway for futureproofing.
        MGS3_DrawIndexedPrimitive_Hook = safetyhook::create_inline(reinterpret_cast<void*>(MGS3_DrawIndexedPrimitive_ScanResult), reinterpret_cast<void*>(MGS3_DrawIndexedPrimitive_Hooked));
        LOG_HOOK(MGS3_DrawIndexedPrimitive_Hook, "MGS 2 | MGS 3: Vector Line Fix - DrawIndexedPrimitive")
    }
    
}
