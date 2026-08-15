#pragma once

struct ID3D11Device;

namespace MGS3GlowOverbright
{
    inline bool bEnabled = false;

    void Initialize();
    void OnDeviceReady(ID3D11Device* device);
}
