#include "stdafx.h"

#include "common.hpp"
#include "helper.hpp"
#include "logging.hpp"
#include "optical_camo.hpp"


namespace
{
    SafetyHookInline ShaderRegisterUpload_hook {};

    constexpr float kActiveWarpScale = 0.75f;
    constexpr float kBreakWarpScale = 1.15f;
    constexpr float kFallbackSourceWidth = 640.0f;
    constexpr float kFallbackSourceHeight = 480.0f;
    constexpr uint32_t kRefractionRegister = 6;
    constexpr uint32_t kRefractionRegisterCount = 4;
    constexpr uint32_t kLateRefractionRegister = kRefractionRegister + 2;

    float gRefractionTargetWidth = kFallbackSourceWidth;
    float gRefractionTargetHeight = kFallbackSourceHeight;

    bool IsFiniteBlock(const float* regs, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            if (!std::isfinite(regs[i]))
            {
                return false;
            }
        }

        return true;
    }

    bool LooksLikeFramebufferExtentRows(const float* regs)
    {
        if (!IsFiniteBlock(regs, 8))
        {
            return false;
        }

        const float halfWidth = regs[0];
        const float negHalfHeight = regs[1];
        const float offsetX = regs[4];
        const float offsetY = regs[5];

        return halfWidth >= 160.0f && halfWidth <= 8192.0f &&
               negHalfHeight <= -120.0f && negHalfHeight >= -8192.0f &&
               std::abs(offsetX - halfWidth) <= 0.5f &&
               std::abs(offsetY + negHalfHeight) <= 0.5f;
    }

    bool LooksLikeSourceRefractionRows(const float* rows)
    {
        if (!IsFiniteBlock(rows, 16))
        {
            return false;
        }

        const float scaleX = rows[0];
        const float scaleY = rows[1];
        const float offsetX = rows[4];
        const float offsetY = rows[5];
        const float normalX = rows[8];
        const float normalY = rows[9];

        return offsetX >= 128.0f && offsetX <= 1024.0f &&
               offsetY >= 112.0f && offsetY <= 1024.0f &&
               scaleX >= 64.0f && scaleX <= offsetX &&
               scaleY >= 0.0f && scaleY <= offsetY &&
               std::abs(normalX) <= 64.0f &&
               std::abs(normalY) <= 64.0f;
    }

    bool LooksLikeBreakTransition(const float* rows)
    {
        const float red = std::abs(rows[12]);
        const float green = std::abs(rows[13]);
        const float blue = std::abs(rows[14]);
        const float alpha = std::abs(rows[15]);

        const bool animatedBreakAlpha = alpha > 96.5f;
        const bool paleBlueBreakColor = blue >= 220.0f && green >= 200.0f && red >= 180.0f && blue >= green && green >= red;
        const bool stronglyBlue = blue > 24.0f && blue > red * 1.15f && blue > green * 1.05f;
        return animatedBreakAlpha || paleBlueBreakColor || stronglyBlue;
    }

    void BuildResolutionScaledRows(const float* sourceRows, float* outRows)
    {
        std::copy(sourceRows, sourceRows + 16, outRows);

        const float sourceWidth = std::max(sourceRows[4] * 2.0f, 1.0f);
        const float sourceHeight = std::max(sourceRows[5] * 2.0f, 1.0f);
        const float xScale = gRefractionTargetWidth / sourceWidth;
        const float yScale = gRefractionTargetHeight / sourceHeight;
        for (int row = 0; row < 3; ++row)
        {
            outRows[row * 4 + 0] *= xScale;
            outRows[row * 4 + 1] *= yScale;
        }

        outRows[1] = -outRows[1];

        const bool breakTransition = LooksLikeBreakTransition(sourceRows);
        const float warpScale = breakTransition ? kBreakWarpScale : kActiveWarpScale;
        const float baseScaleX = gRefractionTargetWidth * 0.5f;
        const float baseScaleY = -gRefractionTargetHeight * 0.5f;
        const float baseOffsetX = gRefractionTargetWidth * 0.5f;
        const float baseOffsetY = gRefractionTargetHeight * 0.5f;

        outRows[0] = baseScaleX + (outRows[0] - baseScaleX) * warpScale;
        outRows[1] = baseScaleY + (outRows[1] - baseScaleY) * warpScale;
        outRows[4] = baseOffsetX + (outRows[4] - baseOffsetX) * warpScale;
        outRows[5] = baseOffsetY + (outRows[5] - baseOffsetY) * warpScale;
        outRows[8] *= warpScale;
        outRows[9] *= warpScale;
    }

    void CacheFramebufferExtentRows(const float* regs)
    {
        if (!LooksLikeFramebufferExtentRows(regs))
        {
            return;
        }

        gRefractionTargetWidth = regs[0] * 2.0f;
        gRefractionTargetHeight = regs[5] * 2.0f;
    }

    bool TryBuildScaledRegisterUpload(uint32_t startReg, uint32_t count, const float* regs, float* patchedRows)
    {
        if (startReg != kLateRefractionRegister || count != 2 || !regs)
        {
            return false;
        }

        const float* rows = regs - 8;
        if (!LooksLikeSourceRefractionRows(rows))
        {
            return false;
        }

        BuildResolutionScaledRows(rows, patchedRows);
        return true;
    }

    void __fastcall ShaderRegisterUpload_Hook(void* backend, uint32_t startReg, uint32_t count, const float* regs)
    {
        thread_local float patchedRows[16];
        if (g_OpticalCamoFix.bEnabled && regs)
        {
            if (startReg == kRefractionRegister && count == 2)
            {
                CacheFramebufferExtentRows(regs);
            }
            else if (TryBuildScaledRegisterUpload(startReg, count, regs, patchedRows))
            {
                ShaderRegisterUpload_hook.fastcall<void>(backend, kRefractionRegister, kRefractionRegisterCount, patchedRows);
                return;
            }
        }

        ShaderRegisterUpload_hook.fastcall<void>(backend, startReg, count, regs);
    }
}

void OpticalCamoFix::Initialize() const
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS 2: Optical Camo Fix: Config disabled. Skipping");
        return;
    }

    constexpr ptrdiff_t kRegisterUploadCallOffset = 0x118;
    uint8_t* refractionRegisterUploadBlock = Memory::PatternScan(
        baseModule,
        "F3 41 0F 10 87 ?? ?? ?? ?? F3 0F 11 45 ?? F3 41 0F 10 8F ?? ?? ?? ?? F3 0F 11 4D ?? F3 41 0F 10 87 ?? ?? ?? ?? F3 0F 11 45",
        "MGS 2: Optical Camo Fix: refraction register upload");

    if (!refractionRegisterUploadBlock)
    {
        spdlog::error("MGS 2: Optical Camo Fix: failed to locate refraction register upload; fix disabled.");
        return;
    }

    uint8_t* shaderRegisterUploadCall = refractionRegisterUploadBlock + kRegisterUploadCallOffset;
    if (*shaderRegisterUploadCall != 0xE8)
    {
        spdlog::error("MGS 2: Optical Camo Fix: expected shader register upload call was not found; fix disabled.");
        return;
    }

    const uintptr_t shaderRegisterUpload = Memory::GetRelativeOffset(shaderRegisterUploadCall + 1);

    ShaderRegisterUpload_hook = safetyhook::create_inline(reinterpret_cast<void*>(shaderRegisterUpload), reinterpret_cast<void*>(ShaderRegisterUpload_Hook));
    LOG_HOOK(ShaderRegisterUpload_hook, "MGS 2: Optical Camo Fix: shader register upload");
}
