#pragma once

namespace D3D11TextOverlay
{
    void Setup();
    void Init();
    void Tick();
    void HandleLevelTransition();

    inline bool bNeedsCompiler = true;
    inline bool bShaderLoaded = false;
}
