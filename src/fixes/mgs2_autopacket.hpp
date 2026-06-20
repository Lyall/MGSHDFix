#pragma once

// Shared DG_SetDmapack autopacket tag builders, used by the MGS2 effects that rebuild a PS2
// crack_pack into an autopacket the D3D backend draws (scope warp, lens droplets).
namespace MGS2Autopacket
{
    constexpr uint8_t kOpTrBuffer   = 0x16;
    constexpr uint8_t kOpDrawOffset = 0x1b;
    constexpr uint8_t kOpTriStrip   = 0x0e;
    constexpr uint8_t kOpEnd        = 0x1e;
    constexpr int     kVertexSize   = 0x18;

    constexpr ptrdiff_t kDmap_Packet0    = 0x08;
    constexpr ptrdiff_t kDmap_Packet1    = 0x10;
    constexpr ptrdiff_t kDmap_Autopacket = 0x38;

    constexpr float kDrawW   = 512.0f;
    constexpr float kDrawH   = 448.0f;
    constexpr float kUVScale = 16384.0f;
    constexpr int   kUVMin   = 8;
    constexpr int   kUVMax   = 16376;

    using InitMdlDrawFn = void*(__fastcall*)(void* prim, uint64_t test, uint64_t alpha, uint64_t primReg);

    inline constexpr const char* kInitMdlDrawSig = "40 53 48 83 EC ?? F3 0F 10 05 ?? ?? ?? ?? 0F 57 D2";

    inline uint8_t* WriteTrBuffer(uint8_t* p)
    {
        std::memset(p, 0, 0x18);
        p[0] = kOpTrBuffer;
        *reinterpret_cast<float*>(p + 0x10) = kDrawW;
        *reinterpret_cast<float*>(p + 0x14) = kDrawH;
        return p + 0x18;
    }

    inline uint8_t* WriteDrawOffset(uint8_t* p)
    {
        std::memset(p, 0, 0x10);
        p[0] = kOpDrawOffset;
        return p + 0x10;
    }

    inline uint8_t* WriteStripHeader(uint8_t* p, uint32_t** countFieldOut)
    {
        std::memset(p, 0, 0x10);
        p[0] = kOpTriStrip;
        *countFieldOut = reinterpret_cast<uint32_t*>(p + 8);
        **countFieldOut = 0;
        return p + 0x10;
    }

    inline uint8_t* WriteVertex(uint8_t* p, float sx, float sy, float un, float vn, uint32_t color)
    {
        std::memset(p, 0, kVertexSize);
        *reinterpret_cast<uint32_t*>(p + 8) = color;
        int ut = static_cast<int>(un * kUVScale);
        int vt = static_cast<int>(vn * kUVScale);
        if (ut < kUVMin) ut = kUVMin; else if (ut > kUVMax) ut = kUVMax;
        if (vt < kUVMin) vt = kUVMin; else if (vt > kUVMax) vt = kUVMax;
        *reinterpret_cast<int16_t*>(p + 0x0c) = static_cast<int16_t>(ut);
        *reinterpret_cast<int16_t*>(p + 0x0e) = static_cast<int16_t>(vt);
        *reinterpret_cast<int16_t*>(p + 0x10) = static_cast<int16_t>(static_cast<int>(sx));
        *reinterpret_cast<int16_t*>(p + 0x12) = static_cast<int16_t>(static_cast<int>(sy));
        return p + kVertexSize;
    }

    inline uint8_t* WriteEnd(uint8_t* p)
    {
        std::memset(p, 0, 8);
        p[0] = kOpEnd;
        return p + 8;
    }

    inline void Commit(uintptr_t dmapack, uint8_t* buffer)
    {
        *reinterpret_cast<uintptr_t*>(dmapack + kDmap_Autopacket) = reinterpret_cast<uintptr_t>(buffer);
        *reinterpret_cast<uintptr_t*>(dmapack + kDmap_Packet0)    = 0;
        *reinterpret_cast<uintptr_t*>(dmapack + kDmap_Packet1)    = 0;
    }
}
