#pragma once

namespace MGS2_ContrastShader
{
    void Setup();
    void Init();
    void Draw(IDXGISwapChain* swap, int keepR, int keepG, int keepB, int keepA, int negaFlag);

    inline bool bShaderLoaded = false;
    inline bool bEnabled = true;

    struct ContrastWork {
        uint8_t _pad[152];
        int keep_r_plus;
        int keep_g_plus;
        int keep_b_plus;
        int keep_a_plus;
        uint8_t _pad2[8];
        int nega_posi_flag;
    };

    ContrastWork* GetActiveWork();

    inline ContrastWork** pContrastWork = nullptr;

    void SetOverride(int keepR, int keepG, int keepB, int keepA, int negaFlag);
    void ClearOverride();

};
