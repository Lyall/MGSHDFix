#pragma once
struct IDXGISwapChain;

class SMAA_AA
{
public:
    static void Init();
    static void Draw(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* depth);
    static bool CompileShaders();
    static inline bool bInitialized   = false;
    static inline bool bEnabled = false;
};
