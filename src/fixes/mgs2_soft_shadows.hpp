#pragma once

namespace MGS2SoftShadows
{
    inline bool bEnabled = true;

    void NoteSquareTarget(ID3D11DeviceContext* ctx, ID3D11Texture2D* tex, UINT dim);
    void Reset();
}
