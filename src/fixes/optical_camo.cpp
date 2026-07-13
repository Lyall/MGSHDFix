#include "stdafx.h"

#include "common.hpp"
#include "helper.hpp"
#include "logging.hpp"
#include "optical_camo.hpp"


namespace
{
    // Hooked at the game's own upload calls, so which upload is which is structural, not guessed.
    SafetyHookMid ExtentsUpload_hook {};
    SafetyHookMid RefractionUpload_hook {};
    SafetyHookMid EyeInvUpload_hook {};
    SafetyHookMid RotMatCapture_hook {};

    // Trims the warp toward the base extents. BP shipped 0.75 (1.15 on Vita's third pass) but never
    // ran the blend at all, so 1.0 is the deviation optcmfbr actually authored.
    constexpr float kOptCmfIntensity = 1.0f;

    // The PS2 draw space the camo rows were authored in. Never derive it from the rows - the break
    // animates m[1][1].
    constexpr float kDrawWidth = 512.0f;
    constexpr float kDrawHeight = 448.0f;

    constexpr uint32_t kLightColRegister = 6;
    constexpr uint32_t kLightColRegisterCount = 4;

    float gTargetWidth = kDrawWidth;
    float gTargetHeight = kDrawHeight;

    // The break's spin. It lives in the scrpad, which is scratch and is long recycled by the time the
    // deferred render runs - so grab it in ChainObjs while it is still alive.
    float gCamoRotMat[16] {};
    bool gHaveRotMat = false;

    float gScaledRows[16] {};
    float gSpunEyeInv[16] {};

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

    // ChainObjs, about to copy objs->extend_data[1] into the scrpad. That is optcmfbr's spin matrix,
    // and this is the only place it is reliably alive.
    void RotMatCapture(SafetyHookContext& ctx)
    {
        const uint8_t* objs = reinterpret_cast<const uint8_t*>(ctx.r15);
        if (!objs)
        {
            return;
        }

        const float* extendData = *reinterpret_cast<const float* const*>(objs + 0xA8);
        if (!extendData || !IsFiniteBlock(extendData + 16, 16))
        {
            return;
        }

        std::copy(extendData + 16, extendData + 32, gCamoRotMat);
        gHaveRotMat = true;
    }

    // The camo source texture's half-extents, straight from the upload that seeds rows 0,1.
    void ExtentsUpload(SafetyHookContext& ctx)
    {
        const float* regs = reinterpret_cast<const float*>(ctx.r9);
        if (!regs || !IsFiniteBlock(regs, 8) || regs[0] <= 0.0f || regs[5] <= 0.0f)
        {
            return;
        }

        gTargetWidth = regs[0] * 2.0f;
        gTargetHeight = regs[5] * 2.0f;
    }

    // The game uploads rows 2,3 raw. Rescale the whole matrix to the target and send all four.
    void RefractionUpload(SafetyHookContext& ctx)
    {
        const float* sourceRows = reinterpret_cast<const float*>(ctx.r9) - 8;
        if (!ctx.r9 || !IsFiniteBlock(sourceRows, 16))
        {
            return;
        }

        std::copy(sourceRows, sourceRows + 16, gScaledRows);

        const float xScale = gTargetWidth / kDrawWidth;
        const float yScale = gTargetHeight / kDrawHeight;
        for (int row = 0; row < 3; ++row)
        {
            gScaledRows[row * 4 + 0] *= xScale;
            gScaledRows[row * 4 + 1] *= yScale;
        }

        gScaledRows[1] = -gScaledRows[1];

        const float baseScaleX = gTargetWidth * 0.5f;
        const float baseScaleY = -gTargetHeight * 0.5f;
        const float baseOffsetX = gTargetWidth * 0.5f;
        const float baseOffsetY = gTargetHeight * 0.5f;

        gScaledRows[0] = baseScaleX + (gScaledRows[0] - baseScaleX) * kOptCmfIntensity;
        gScaledRows[1] = baseScaleY + (gScaledRows[1] - baseScaleY) * kOptCmfIntensity;
        gScaledRows[4] = baseOffsetX + (gScaledRows[4] - baseOffsetX) * kOptCmfIntensity;
        gScaledRows[5] = baseOffsetY + (gScaledRows[5] - baseOffsetY) * kOptCmfIntensity;
        gScaledRows[8] *= kOptCmfIntensity;
        gScaledRows[9] *= kOptCmfIntensity;

        ctx.rdx = kLightColRegister;
        ctx.r8 = kLightColRegisterCount;
        ctx.r9 = reinterpret_cast<uintptr_t>(gScaledRows);
    }

    // optcmfbr turns the normal-projection matrix every frame; the port ships a plain eye_inv and
    // drops it, so the break stops moving once the squeeze is spent. The uploaded block is the
    // transpose of the game's matrix, so transpose(rot * eye_inv) is transpose(eye_inv) * transpose(rot).
    void EyeInvUpload(SafetyHookContext& ctx)
    {
        const float* regs = reinterpret_cast<const float*>(ctx.r9);
        if (!regs || !gHaveRotMat || !IsFiniteBlock(regs, 16))
        {
            return;
        }

        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k)
                {
                    sum += regs[row * 4 + k] * gCamoRotMat[col * 4 + k];
                }

                gSpunEyeInv[row * 4 + col] = sum;
            }
        }

        ctx.r9 = reinterpret_cast<uintptr_t>(gSpunEyeInv);
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

    constexpr ptrdiff_t kExtentsCallOffset = 0x16;
    uint8_t* extentsUpload = Memory::PatternScan(baseModule,
        "BA 06 00 00 00 F3 0F 11 ?? ?? 48 C7 45 ?? ?? ?? ?? ?? 44 8D 42 FC E8 ?? ?? ?? ??",
        "MGS 2: Optical Camo Fix: BP_Obj_OptCmf_InitPacket() -> source extents");

    constexpr ptrdiff_t kRefractionCallOffset = 0x17;
    uint8_t* refractionUpload = Memory::PatternScan(baseModule,
        "4C 8D 8E E0 00 00 00 BA 08 00 00 00 44 8D 42 FA 48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ??",
        "MGS 2: Optical Camo Fix: BP_Obj_OptCmf_Render() -> camo rows");

    constexpr ptrdiff_t kEyeInvCallOffset = 0x39;
    uint8_t* eyeInvUpload = Memory::PatternScan(baseModule,
        "F3 0F 10 47 4C F3 0F 11 4D ?? F3 0F 10 4F 5C F3 0F 11 85 ?? ?? ?? ?? F3 0F 10 47 6C F3 0F 11 8D ?? ?? ?? ?? F3 0F 10 4F 7C F3 0F 11 85 ?? ?? ?? ?? F3 0F 11 8D ?? ?? ?? ?? E8 ?? ?? ?? ??",
        "MGS 2: Optical Camo Fix: BP_Obj_OptCmf_InitPacket() -> eye_inv");

    uint8_t* rotMatCapture = Memory::PatternScan(baseModule,
        "49 8B 87 A8 00 00 00 48 89 5C ?? ?? 49 8D 9F 10 01 00 00 4C 89 74 ?? ?? 45 8B F4 0F 28 40 40 0F 29 05 ?? ?? ?? ??",
        "MGS 2: Optical Camo Fix: ChainObjs() -> objs->extend_data[1]");

    if (!extentsUpload || !refractionUpload || !eyeInvUpload || !rotMatCapture)
    {
        spdlog::error("MGS 2: Optical Camo Fix: camo upload sites not found; fix disabled.");
        return;
    }

    if (extentsUpload[kExtentsCallOffset] != 0xE8 || refractionUpload[kRefractionCallOffset] != 0xE8 || eyeInvUpload[kEyeInvCallOffset] != 0xE8)
    {
        spdlog::error("MGS 2: Optical Camo Fix: camo upload calls moved; fix disabled.");
        return;
    }

    RotMatCapture_hook = safetyhook::create_mid(rotMatCapture, RotMatCapture);
    LOG_HOOK(RotMatCapture_hook, "MGS 2: Optical Camo Fix: rot_mat capture");

    ExtentsUpload_hook = safetyhook::create_mid(extentsUpload + kExtentsCallOffset, ExtentsUpload);
    LOG_HOOK(ExtentsUpload_hook, "MGS 2: Optical Camo Fix: source extents upload");

    RefractionUpload_hook = safetyhook::create_mid(refractionUpload + kRefractionCallOffset, RefractionUpload);
    LOG_HOOK(RefractionUpload_hook, "MGS 2: Optical Camo Fix: camo rows upload");

    EyeInvUpload_hook = safetyhook::create_mid(eyeInvUpload + kEyeInvCallOffset, EyeInvUpload);
    LOG_HOOK(EyeInvUpload_hook, "MGS 2: Optical Camo Fix: eye_inv upload");
}
