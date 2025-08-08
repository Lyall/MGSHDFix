#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>

class PostFX final
{
public:
    bool Initialize(ID3D11Device* device, IDXGISwapChain* swapChain);
    void Render(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv);
    void Shutdown();

private:
    bool CompileShader(const std::string& source, const std::string& entry, const std::string& target, ID3DBlob** blob);

    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samLinear;

    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_copyTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_copySRV;
};
