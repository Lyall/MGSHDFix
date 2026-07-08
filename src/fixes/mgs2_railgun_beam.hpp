#pragma once

// Untextured prims bind an all-white stand-in texture and render through the colour-doubling
// shader path, twice as bright as the GS. Substitutes a mid-grey twin for effect draws.
namespace MGS2RailgunBeam
{
    void InitializeEarly();   // hooks D3D11 device creation - call at dll load, before the game boots

    inline bool bEnabled = true;

}
