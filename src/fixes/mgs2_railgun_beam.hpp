#pragma once

// Untextured prims bind an all-white stand-in texture and render through the colour-doubling
// shader path, twice as bright as the GS. Substitutes a mid-grey twin for effect draws.
namespace MGS2RailgunBeam
{
    void OnDeviceCreated(ID3D11Device* dev);   // d3d11_api calls this as the game creates its device

    inline bool bEnabled = true;

}
