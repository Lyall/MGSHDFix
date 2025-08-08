#pragma once


class VectorScalingFix final
{
private:
    bool CompileGeometryShader();
    static constexpr int DEFAULT_LINE_SCALE = 360;

public:
    void Initialize();
    void LoadCompiledShader() const;

    bool bFixRain = true;
    bool bFixUI = true;

    bool bToggleRainShader = true;
    bool bToggleUIShader = true;
    int bToggleWireframe = false;

    int vkRainShaderToggle = 0;
    int vkUIShaderToggle = 0;
    int vkWireframeToggle = 0;

    bool bNeedsCompiler = false;
    double iVectorLineScale = 360;
};

inline VectorScalingFix g_VectorScalingFix;
