#pragma once

namespace D3D11StateCache
{
    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void Invalidate(ID3D11DeviceContext* context);
}
