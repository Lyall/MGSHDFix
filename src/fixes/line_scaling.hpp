#pragma once

#include <d3d11.h>
#include <wrl/client.h>

class VectorScalingFix final
{
private:
    bool CompileGeometryShader();

    static constexpr int DEFAULT_LINE_SCALE = 360;

    Microsoft::WRL::ComPtr<ID3DBlob> compiledShaderBytecode;

public:
    void Initialize();
    void LoadCompiledShader();

    bool bEnableVectorLineFix = false;
    double iVectorLineScale = 360;
};

inline VectorScalingFix g_VectorScalingFix;
