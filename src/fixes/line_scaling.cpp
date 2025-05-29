#include "common.hpp"
#include "d3d11_api.hpp"
#include "d3dcompile_api.hpp"
#include <spdlog/spdlog.h>

bool bEnableVectorLineFix;
double iVectorLineScale;
constexpr auto DEFAULT_LINE_SCALE = 360;

ID3DBlob* compiledShaderBytecode;

static SafetyHookInline MGS3_DrawIndexedPrimitive_Hook {};
uint64_t MGS3_DrawIndexedPrimitive_Hooked(void* CD3DCachedDevice, int topologyType, int BaseVertexIndex, int MinVertexIndex, int NumVertices, int startIndex, int primCount)
{ //This is called every frame, DO NOT add logging or the I/O will nuke performance.
    if(!(topologyType == 0x1 || topologyType == 0x2))
    {
        return MGS3_DrawIndexedPrimitive_Hook.call<uint64_t>(CD3DCachedDevice, topologyType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
    }
    d3dDeviceContext->GSSetShader(geometryShader, nullptr, 0);
    auto ret = MGS3_DrawIndexedPrimitive_Hook.call<uint64_t>(CD3DCachedDevice, topologyType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
    d3dDeviceContext->GSSetShader(nullptr, nullptr, 0);
    return ret;
}

void MGS23_VectorLine_InjectShader()
{
    if (!geometryShader && compiledShaderBytecode && d3dDevice)
    {
        HRESULT result = d3dDevice->CreateGeometryShader(
            compiledShaderBytecode->GetBufferPointer(),
            compiledShaderBytecode->GetBufferSize(),
            nullptr,
            &geometryShader
        );

        if (FAILED(result))
            spdlog::error("MGS 2/3: Vector Line Fix - Inject Shader: Failed to create geometry shader on device");
        else
            spdlog::info("MGS 2/3: Vector Line Fix - Inject Shader: Successfully injected geometry shader.");
    } 
}


void CompileGeometryShader()
{
    if (iVectorLineScale < 1)
    {
        spdlog::info("MGS 2/3: Vector Line Fix - CompileGeometryShader: Invalid line scale! Defaulting to 360");
        iVectorLineScale = DEFAULT_LINE_SCALE;
    }
    if(iVectorLineScale < DEFAULT_LINE_SCALE*0.5)
    {
        AllocConsole();
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "MGSHDFix Config Warning:\n"
                     "Line scale is currently set to more that double the default size of Screen Height/360 (" << iCurrentResY / 360 << " pixels wide),\n"
                     "with individual raindrops currently set to " << iVectorLineScale << " (" << iCurrentResY / iVectorLineScale << " pixels wide.)\n"
                     "If you intend for line effects to be MASSIVE like this, set \"Silence Scaling Warnings\" to true in the config";
        spdlog::warn("Config Warning");
        spdlog::warn("Line scale is currently set to more that double the default size of Screen Height / 360 (6 pixels wide,) with individual raindrops currently set to {} ({} pixels wide.)", iVectorLineScale, iCurrentResY / iVectorLineScale);
        spdlog::warn("If you intend for line effects to be MASSIVE like this, set \"Silence Scaling Warnings\" to true in the config");
    }
    else
    {
        spdlog::info("CompileGeometryShader: Line Scale before: {}", iVectorLineScale);
    }
    iVectorLineScale = round(iCurrentResY / iVectorLineScale);
    spdlog::info("CompileGeometryShader: Target Pixel Width = : {}", iVectorLineScale);
    iVectorLineScale = (iCurrentResY / iVectorLineScale);
    spdlog::info("CompileGeometryShader: Line Scale after rounding: {}", iVectorLineScale);
    std::string shaderString = R"(
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
    HRESULT hr = D3DCompile(
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
            spdlog::error("MGS 2/3: Vector Line Fix - CompileGeometryShader: Shader compile failed with error: {}",
                static_cast<const char*>(errorMsgs->GetBufferPointer()));
        }
        else
        {
            spdlog::error("MGS 2/3: Vector Line Fix - CompileGeometryShader: Shader compile failed with HRESULT: 0x{:08X}", hr);
        }
        return;
    }
    compiledShaderBytecode = compiledShader;
    spdlog::info("MGS 2/3: Vector Line Fix - CompileGeometryShader: Shader compiled successfully!");
}

void ConfigParse_Fix_LineScaling()
{
    if (!(eGameType == MgsGame::MGS3 || eGameType == MgsGame::MGS2))
    {
        bEnableVectorLineFix = false;
        return;
    }
    inipp::get_value(ini.sections["Vector Line Fix"], "Enabled", bEnableVectorLineFix);
    spdlog::info("Config Parse: bEnableVectorLineFix: {}", bEnableVectorLineFix);

    if (!bEnableVectorLineFix)
    {
        return;
    }

    inipp::get_value(ini.sections["Vector Line Fix"], "Line Scale", iVectorLineScale);
    spdlog::info("Config Parse: iVectorLineScale: {}", iVectorLineScale);
}

void Init_LineScaling()
{
    if (!bEnableVectorLineFix)
    {
        return;
    }

    CompileGeometryShader();

    if (uint8_t* MGS3_DrawIndexedPrimitive_ScanResult = Memory::PatternScan(baseModule, "48 89 5C 24 ?? 57 48 83 EC 20 FF 41 ?? 41 8B ??", "MGS 2/3: Vector Line Fix - DrawIndexedPrimitive", NULL, NULL))
    {   //Technically only needed for MGS3. MGS2 does have the function as well, but it's not used. Let's patch it anyway for futureproofing.
        MGS3_DrawIndexedPrimitive_Hook = safetyhook::create_inline(reinterpret_cast<void*>(MGS3_DrawIndexedPrimitive_ScanResult), reinterpret_cast<void*>(MGS3_DrawIndexedPrimitive_Hooked));
        LOG_HOOK(MGS3_DrawIndexedPrimitive_Hook, "MGS 2/3: Vector Line Fix - DrawIndexedPrimitive", NULL, NULL)
    }
}