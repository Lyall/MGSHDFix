#pragma once

namespace MG_VectorFontRenderer
{
    constexpr int nLayerWidth = 512;
    constexpr int nLayerHeight = 424;
    constexpr int nAtlasSize = 2048;

    struct GlyphLocation
    {
        UINT nPage = 0;
        int nX = 0;
        int nY = 0;
    };

    struct Vertex
    {
        float fX;
        float fY;
        float fU;
        float fV;
        uint32_t nColor;
    };

    struct Batch
    {
        UINT nPage;
        UINT nStart;
        UINT nCount;
    };

    class Renderer
    {
    public:
        bool Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, decltype(&D3DCompile) pfnCompile);
        bool IsDevice(ID3D11Device* pDevice) const;
        void ResetGlyphs();
        bool AddGlyph(const uint8_t* pCoverage, int nWidth, int nHeight, GlyphLocation& stLocation);
        ID3D11ShaderResourceView* Render(const std::vector<uint32_t>& vBasePixels, const std::vector<Vertex>& vVertices, const std::vector<Batch>& vBatches, uint64_t nRevision, int nScale, int nFade, DXGI_FORMAT eFormat);

    private:
        struct Surface
        {
            Microsoft::WRL::ComPtr<ID3D11Texture2D> pTexture;
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pView;
            Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pTarget;
        };

        struct Atlas
        {
            Surface stSurface;
            int nX = 0;
            int nY = 0;
            int nRowHeight = 0;
        };

        bool CreateSurface(Surface& stSurface, UINT nWidth, UINT nHeight, DXGI_FORMAT eFormat, bool bTarget);
        Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext1> m_pContext;
        Microsoft::WRL::ComPtr<ID3DDeviceContextState> m_pState;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pQuadShader;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pScreenShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pGlyphShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pBaseShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pResolveShader;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pLayout;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_pVertices;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_pConstants;
        Microsoft::WRL::ComPtr<ID3D11BlendState> m_pBlend;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pRasterizer;
        std::vector<Atlas> m_vAtlases;
        std::vector<uint32_t> m_vBasePixels;
        Surface m_stBase;
        Surface m_stComposite;
        Surface m_stOutput;
        UINT m_nVertexCapacity = 0;
        uint64_t m_nRevision = 0;
        int m_nScale = 0;
        int m_nFade = -1;
        DXGI_FORMAT m_eFormat = DXGI_FORMAT_UNKNOWN;
    };
}
