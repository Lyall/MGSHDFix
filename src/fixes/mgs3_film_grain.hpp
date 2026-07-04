#pragma once

#include <cstdint>

struct IDXGISwapChain;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;

namespace MGS3FilmGrain
{
    enum class Mode
    {
        Off,
        On
    };

    inline Mode mode = Mode::On;

    void Initialize();
    void BeginPresent();
    void EndPresent();
    void OnAfterGameDraw(ID3D11DeviceContext* context, unsigned int vertexCount);
    void OnPreMenuRender(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* depth);
    bool HasActiveGrain();
}
