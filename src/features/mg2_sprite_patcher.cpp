#include "stdafx.h"
#include "mg2_sprite_patcher.hpp"
#include "mg2_sprite_patches.hpp"
#include "mg2_palette_patches.hpp"

#include "common.hpp"
#include "mg2_linkvarbuf.hpp"

using namespace MG2_LinkVarBuf;


namespace
{
    struct ImagePatch
    {
        int nEntry;
        uint64_t nHash;
        std::vector<uint8_t> vBlob;
    };

    std::vector<ImagePatch> s_vPatches;
    std::vector<mg2_palette_patches::Override> s_vPalettePatches;
    bool s_bInitialized = false;
    SafetyHookMid s_hLoadImage;

    constexpr size_t nMaxEntries = 128;
    constexpr size_t nMaxPixels = 4u * 1024u * 1024u;

    std::array<std::atomic<bool>, nMaxEntries> s_abApplied {};
    std::array<int, nPaletteSlotCount> s_anSlotOwner {};   // palette entry last pushed to each slot, -1 = none
    std::vector<mg2_palette_patches::SlotOverride> s_vResolvedColours;
    bool s_bPaletteAssignmentsChanged = true;

    void TrackPalette(int nEntry, size_t nSlot)
    {
        if (nSlot < nPaletteSlotCount && s_anSlotOwner[nSlot] != nEntry)
        {
            s_anSlotOwner[nSlot] = nEntry;
            s_bPaletteAssignmentsChanged = true;
        }
    }

    void ApplyPaletteOverrides(uint32_t* pBaseTable)
    {
        for (const mg2_palette_patches::SlotOverride& stSlot : mg2_palette_patches::astSlotOverrides)
        {
            if (stSlot.nSlot > 0 && stSlot.nSlot < static_cast<int>(nPaletteSlotCount))
            {
                pBaseTable[stSlot.nSlot] = stSlot.nRgb;
            }
        }

        if (s_bPaletteAssignmentsChanged)
        {
            s_vResolvedColours.clear();
            for (const mg2_palette_patches::Override& stChange : s_vPalettePatches)
            {
                for (size_t nSlot = 0; nSlot < nPaletteSlotCount; nSlot++)
                {
                    const size_t nTarget = nSlot + static_cast<size_t>(stChange.nIndex);
                    if (s_anSlotOwner[nSlot] == stChange.nEntry && nTarget < nPaletteSlotCount)
                    {
                        s_vResolvedColours.push_back({ static_cast<int>(nTarget), stChange.nRgb });
                    }
                }
            }
            s_bPaletteAssignmentsChanged = false;
        }
        for (const auto& stColour : s_vResolvedColours)
        {
            pBaseTable[stColour.nSlot] = stColour.nRgb;
        }
    }

    uint64_t Fnv1a64(const uint8_t* pData, size_t nSize)
    {
        uint64_t nHash = 0xCBF29CE484222325ull;
        for (size_t nIndex = 0; nIndex < nSize; nIndex++)
        {
            nHash = (nHash ^ pData[nIndex]) * 0x100000001B3ull;
        }
        return nHash;
    }

    void SwapBlob(SafetyHookContext& stContext)
    {
        // rdx = the blob: [u32 LE width][u32 LE height][width * height palette indices]
        const uint8_t* pBlob = reinterpret_cast<const uint8_t*>(stContext.rdx);
        const uint32_t nWidth = Memory::ReadLE32(pBlob);
        const uint32_t nHeight = Memory::ReadLE32(pBlob + nImageBlobHeightOffset);
        const size_t nPixels = static_cast<size_t>(nWidth) * nHeight;
        if (nPixels == 0 || nPixels > nMaxPixels)
        {
            return;
        }

        uint64_t nHash = 0;
        bool bHashed = false;
        for (const ImagePatch& stPatch : s_vPatches)
        {
            if (stPatch.vBlob.size() != nImageBlobHeaderSize + nPixels || Memory::ReadLE32(stPatch.vBlob.data()) != nWidth || Memory::ReadLE32(stPatch.vBlob.data() + nImageBlobHeightOffset) != nHeight)
            {
                continue;
            }
            if (!bHashed)
            {
                nHash = Fnv1a64(pBlob, nImageBlobHeaderSize + nPixels);
                bHashed = true;
            }
            if (stPatch.nHash != nHash)
            {
                continue;
            }

            stContext.rdx = reinterpret_cast<uintptr_t>(stPatch.vBlob.data());
            if (stPatch.nEntry >= 0 && stPatch.nEntry < static_cast<int>(nMaxEntries))
            {
                s_abApplied[stPatch.nEntry] = true;
            }
            return;
        }
    }
}

bool MG2_SpritePatcher::AddPatch(int nImageEntry, uint64_t nOriginalHash, const uint8_t* pBlob, size_t nSize)
{
    if (s_bInitialized)
    {
        spdlog::error("MG2: Sprite Patcher : Image {} registered after initialization.", nImageEntry);
        return false;
    }
    if (nImageEntry < 0 || nImageEntry >= static_cast<int>(nMaxEntries) || !pBlob || nSize < nImageBlobHeaderSize)
    {
        return false;
    }
    const uint64_t nPixels = uint64_t(Memory::ReadLE32(pBlob)) * Memory::ReadLE32(pBlob + nImageBlobHeightOffset);
    if (nPixels == 0 || nPixels > nMaxPixels || nSize != nImageBlobHeaderSize + nPixels)
    {
        return false;
    }
    for (const ImagePatch& stPatch : s_vPatches)
    {
        if (stPatch.nEntry == nImageEntry || stPatch.nHash == nOriginalHash)
        {
            if (stPatch.nEntry == nImageEntry && stPatch.nHash == nOriginalHash && stPatch.vBlob.size() == nSize && std::equal(stPatch.vBlob.begin(), stPatch.vBlob.end(), pBlob))
            {
                return true;
            }
            spdlog::error("MG2: Sprite Patcher : Conflicting replacement for image {}.", nImageEntry);
            return false;
        }
    }
    s_vPatches.push_back({ nImageEntry, nOriginalHash, std::vector<uint8_t>(pBlob, pBlob + nSize) });
    return true;
}

bool MG2_SpritePatcher::AddPalettePatch(int nPaletteEntry, int nColourIndex, uint32_t nRgb)
{
    if (s_bInitialized || nPaletteEntry < 0 || nColourIndex < 0 || nColourIndex >= static_cast<int>(nPaletteSlotCount) || nRgb > 0xFFFFFF)
    {
        spdlog::error("MG2: Sprite Patcher : Invalid or late palette registration for entry {}.", nPaletteEntry);
        return false;
    }
    for (const mg2_palette_patches::Override& stChange : s_vPalettePatches)
    {
        if (stChange.nEntry == nPaletteEntry && stChange.nIndex == nColourIndex)
        {
            if (stChange.nRgb == nRgb)
            {
                return true;
            }
            spdlog::error("MG2: Sprite Patcher : Conflicting replacement for palette {} colour {}.", nPaletteEntry, nColourIndex);
            return false;
        }
    }
    s_vPalettePatches.push_back({ nPaletteEntry, nColourIndex, nRgb });
    return true;
}

bool MG2_SpritePatcher::IsPatched(int nImageEntry)
{
    return nImageEntry >= 0 && nImageEntry < static_cast<int>(nMaxEntries) && s_abApplied[nImageEntry];
}

void MG2_SpritePatcher::Initialize()
{
    if (!bAllowReplacements || s_bInitialized)
    {
        return;
    }
    for (const mg2_sprite_patches::Patch& stPatch : mg2_sprite_patches::astPatches)
    {
        AddPatch(stPatch.nEntry, stPatch.nHash, stPatch.pBlob, stPatch.nSize);
    }
    for (const mg2_palette_patches::Override& stChange : mg2_palette_patches::astOverrides)
    {
        AddPalettePatch(stChange.nEntry, stChange.nIndex, stChange.nRgb);
    }
    s_bInitialized = true;

    if (!s_vPatches.empty())
    {
        if (!MG2_Gamefuncs::pfnLoadImageBlob)
        {
            spdlog::error("MG2: Sprite Patcher : Missing LoadImageBlob address. Skipping image patches.");
        }
        else
        {
            //LoadImageBlob
            s_hLoadImage = safetyhook::create_mid(reinterpret_cast<void*>(MG2_Gamefuncs::pfnLoadImageBlob), SwapBlob);
            LOG_HOOK(s_hLoadImage, "MG2: Sprite Patcher : LoadImageBlob() : replace matching sprite images")
            if (s_hLoadImage)
            {
                spdlog::info("MG2: Sprite Patcher : Image loader initialized with {} registered patches.", s_vPatches.size());
            }
        }
    }

    if (!s_vPalettePatches.empty() || !mg2_palette_patches::astSlotOverrides.empty())
    {
        s_anSlotOwner.fill(-1);

        //PushPalette
        uint8_t* pPushPalette = Memory::PatternScan(mg2Module, "4C 63 05 ?? ?? ?? ?? 4C 8D 0D", "MG2: Sprite Patcher : PushPalette() : palette slots");
        //UpdatePalette+39F
        uint8_t* pUpdatePalette = Memory::PatternScan(mg2Module, "89 2D ?? ?? ?? ?? 48 8B 6C 24 ?? 48 83 C4 ?? 41 5F", "MG2: Sprite Patcher : UpdatePalette()+0x39F : base colours");
        if (!pPushPalette || !pUpdatePalette)
        {
            spdlog::error("MG2: Sprite Patcher : Missing palette addresses. Skipping palette patches.");
        }
        else
        {
            static SafetyHookMid s_hPushPalette {};
            static SafetyHookMid s_hUpdatePalette {};
            s_hPushPalette = safetyhook::create_mid(pPushPalette, [](SafetyHookContext& stContext)
            {
                TrackPalette(static_cast<int>(static_cast<uint32_t>(stContext.rcx)), static_cast<uint32_t>(stContext.rdx));
            });
            LOG_HOOK(s_hPushPalette, "MG2: Sprite Patcher : PushPalette() : palette slots")
            s_hUpdatePalette = safetyhook::create_mid(pUpdatePalette, [](SafetyHookContext& stContext)
            {
                ApplyPaletteOverrides(reinterpret_cast<uint32_t*>(stContext.r15));
            });
            LOG_HOOK(s_hUpdatePalette, "MG2: Sprite Patcher : UpdatePalette()+0x39F : base colours")
            if (!s_hPushPalette || !s_hUpdatePalette)
            {
                s_hPushPalette.reset();
                s_hUpdatePalette.reset();
                spdlog::error("MG2: Sprite Patcher : Incomplete palette hook setup. Disabled palette patches.");
            }
            else
            {
                spdlog::info("MG2: Sprite Patcher : Palette hooks initialized with {} registered colours and {} slot overrides.", s_vPalettePatches.size(), mg2_palette_patches::astSlotOverrides.size());
            }
        }
    }
}
