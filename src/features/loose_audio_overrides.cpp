#include "stdafx.h"
#include "loose_audio_overrides.hpp"
#include "common.hpp"
#include "expand_bp_assets.hpp"
#include "config_keys.hpp"
#include "loose_audio_overrides_data.hpp"

#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>
#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>
#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>
#include <stb_vorbis.c>

namespace
{
    using namespace LooseAudioOverridesData;
    std::map<Header, std::filesystem::path> s_stCandidates;
    std::filesystem::path s_stOverloadRoot;
    ActiveReplacement s_stActive;
    LoudnessMode s_eLoudnessMode = LoudnessMode::Off;
    std::atomic<bool> s_bReady { false };
    SafetyHookMid s_hLoad {};
    SafetyHookMid s_hClose {};
    SafetyHookMid s_hHeader {};
    SafetyHookMid s_hPcm {};
    constexpr size_t nDriverTypeOffset = 0x50;
    constexpr size_t nDriverMovieOffset = 0x58;

    struct Variant
    {
        std::wstring_view svAsset;
        std::wstring_view svVariant;
    };
    constexpr Variant astVariants[] =
    {
        { L"mg2_bgm29_startmusen", L"before_radar" },
    };
    std::map<std::wstring, Header> s_stVariantHeaders;

    std::wstring LowerStem(const std::filesystem::path& stPath)
    {
        auto sStem = stPath.stem().wstring();
        std::transform(sStem.begin(), sStem.end(), sStem.begin(), [](wchar_t nChar) { return static_cast<wchar_t>(std::towlower(nChar)); });
        return sStem;
    }

    const Variant* FindVariant(const std::filesystem::path& stRelative)
    {
        const auto sStem = LowerStem(stRelative);
        for (const auto& stVariant : astVariants)
        {
            if (sStem == stVariant.svAsset)
            {
                return &stVariant;
            }
        }
        return nullptr;
    }

    bool HasVariantAudio(const std::filesystem::path& stRelative, const Variant* pVariant)
    {
        if (!pVariant)
        {
            return false;
        }
        for (size_t nTrack = 0; nTrack < nMaxTracks; ++nTrack)
        {
            if (!FindReplacementAudio(sExePath, s_stOverloadRoot, stRelative, nTrack, pVariant->svVariant).empty())
            {
                return true;
            }
        }
        return false;
    }

    // Use loader cursors so the next arrangement starts at frame zero.
    void UseCursorPlayback(const std::vector<int16_t>& vPcm, LoopPoints& stLoop)
    {
        if (!stLoop.nEnd && !vPcm.empty())
        {
            stLoop = {};
            stLoop.nEnd = vPcm.size() / 2;
        }
    }

    void Load_hook(SafetyHookContext& ctx)
    {
        const auto* pDriver = reinterpret_cast<const uint8_t*>(ctx.rbx);
        if (!s_bReady.load(std::memory_order_acquire) || Read32(pDriver + nDriverTypeOffset) != nMtaType || *reinterpret_cast<const uintptr_t*>(pDriver + nDriverMovieOffset) != 0)
        {
            return; // Only replace BGM. (if someone needs mgs2/mgs3's cutscene support, i do have that worked out already, but there's not much of the point atm as they already have better audio mod. poke afevis if you need it implemented the rest of the way)
        }
        s_stActive.Replace(nullptr);
        if (!ctx.rax)
        {
            return;
        }
        try
        {
            Header aHeader {};
            memcpy(aHeader.data(), reinterpret_cast<const void*>(ctx.rax), aHeader.size());
            const auto stCandidate = s_stCandidates.find(aHeader);
            if (stCandidate == s_stCandidates.end())
            {
                return;
            }
            const auto& stPath = stCandidate->second;
            auto pAudio = std::make_unique<Replacement>();
            if (!ReadOriginal(ResolveAsset(sExePath, s_stOverloadRoot, stPath), *pAudio) || pAudio->aHeader != aHeader)
            {
                spdlog::warn("MG1/2: Loose Audio : Unsupported or changed asset: {}", stPath.string());
                return;
            }
            bool bAnyTrack = false;
            for (size_t nTrack = 0; nTrack < pAudio->nTracks; ++nTrack)
            {
                const auto stAudioPath = FindReplacementAudio(sExePath, s_stOverloadRoot, stPath, nTrack);
                if (stAudioPath.empty())
                {
                    continue;
                }
                ClipLength stLength;
                if (!DecodeAudio(stAudioPath, *pAudio, nTrack, s_eLoudnessMode, &stLength))
                {
                    spdlog::warn("MG1/2: Loose Audio : Unsupported audio, retaining native track: {}", stAudioPath.string());
                    continue;
                }
                bAnyTrack = true;
                if (!stLength.sLoopError.empty())
                {
                    spdlog::warn("MG1/2: Loose Audio : Ignoring loop JSON for {}: {}; using original duration and loops", stAudioPath.string(), stLength.sLoopError);
                }
                if (LooseAudioOverrides::sDebugLogging == ConfigKeys::LooseAudioDebugLogging_Disabled)
                {
                    continue;
                }
                const auto& stLoop = pAudio->astLoops[nTrack];
                if (stLoop.nEnd)
                {
                    if (LooseAudioOverrides::sDebugLogging == ConfigKeys::LooseAudioDebugLogging_Full)
                    {
                        spdlog::info("MG1/2: Loose Audio : Loaded on demand {} (file: {:.6f}s; custom loop: {:.6f}s -> {:.6f}s; intro plays once)", stAudioPath.string(), stLength.fFileSeconds, double(stLoop.nStart) / 48000, double(stLoop.nEnd) / 48000);
                    }
                    continue;
                }
                const double fDifference = stLength.fFileSeconds - stLength.fExpectedSeconds;
                if (fDifference > 0)
                {
                    spdlog::info("MG1/2: Loose Audio : Loaded on demand {} (file: {:.6f}s, expected: {:.6f}s; longer by {:.6f}s, trimmed)", stAudioPath.string(), stLength.fFileSeconds, stLength.fExpectedSeconds, fDifference);
                }
                else if (fDifference < 0)
                {
                    spdlog::info("MG1/2: Loose Audio : Loaded on demand {} (file: {:.6f}s, expected: {:.6f}s; shorter by {:.6f}s, padded with silence)", stAudioPath.string(), stLength.fFileSeconds, stLength.fExpectedSeconds, -fDifference);
                }
                else if (LooseAudioOverrides::sDebugLogging == ConfigKeys::LooseAudioDebugLogging_Full)
                {
                    spdlog::info("MG1/2: Loose Audio : Loaded on demand {} ({:.6f}s)", stAudioPath.string(), stLength.fExpectedSeconds);
                }
            }
            const auto* pVariant = FindVariant(stPath);
            if (HasVariantAudio(stPath, pVariant))
            {
                for (size_t nTrack = 0; nTrack < pAudio->nTracks; ++nTrack)
                {
                    const auto stVariantPath = FindReplacementAudio(sExePath, s_stOverloadRoot, stPath, nTrack, pVariant->svVariant);
                    if (stVariantPath.empty())
                    {
                        continue;
                    }
                    pAudio->avAfterPcm[nTrack] = std::move(pAudio->avPcm[nTrack]);
                    pAudio->astAfterLoops[nTrack] = pAudio->astLoops[nTrack];
                    ClipLength stLength;
                    if (!DecodeAudio(stVariantPath, *pAudio, nTrack, s_eLoudnessMode, &stLength))
                    {
                        pAudio->avPcm[nTrack] = std::move(pAudio->avAfterPcm[nTrack]);
                        pAudio->astLoops[nTrack] = pAudio->astAfterLoops[nTrack];
                        spdlog::warn("MG1/2: Loose Audio : Unsupported variant audio, retaining normal track: {}", stVariantPath.string());
                        continue;
                    }
                    UseCursorPlayback(pAudio->avAfterPcm[nTrack], pAudio->astAfterLoops[nTrack]);
                    UseCursorPlayback(pAudio->avPcm[nTrack], pAudio->astLoops[nTrack]);
                    pAudio->abVariantTracks[nTrack] = true;
                    pAudio->bVariant = true;
                    bAnyTrack = true;
                    if (LooseAudioOverrides::sDebugLogging != ConfigKeys::LooseAudioDebugLogging_Disabled)
                    {
                        spdlog::info("MG1/2: Loose Audio : Loaded variant {} ({:.6f}s)", stVariantPath.string(), stLength.fFileSeconds);
                    }
                }
            }
            if (bAnyTrack)
            {
                s_stActive.Replace(std::move(pAudio));
            }
        }
        catch (const std::exception& stError)
        {
            spdlog::error("MG1/2: Loose Audio : On-demand load failed, retaining native audio: {}", stError.what());
        }
    }

    void Close_hook(SafetyHookContext&)
    {
        if (s_bReady.load(std::memory_order_acquire))
        {
            s_stActive.Replace(nullptr);
        }
    }

    void Header_hook(SafetyHookContext& ctx)
    {
        if (!s_bReady.load(std::memory_order_acquire))
        {
            return;
        }
        s_stActive.Select(reinterpret_cast<const uint8_t*>(ctx.r8));
    }

    void Pcm_hook(SafetyHookContext& ctx)
    {
        const size_t nTrack = static_cast<uint32_t>(ctx.rsi);
        if (!s_bReady.load(std::memory_order_acquire))
        {
            return;
        }

        const auto nCursor = *reinterpret_cast<const uintptr_t*>(ctx.rsp + nCursorStackOffset);
        if (nCursor < nUnitBytes)
        {
            return;
        }
        const auto* pUnit = reinterpret_cast<const uint8_t*>(nCursor - nUnitBytes);
        auto* pPcm = reinterpret_cast<int16_t*>(ctx.rsp + nPcmStackOffset);
        s_stActive.Copy(pUnit, pPcm, nTrack);
    }
}

void LooseAudioOverrides::Initialize()
{
    if (!(eGameType & MG) || !bEnabled || s_hLoad || s_hClose || s_hHeader || s_hPcm)
    {
        return;
    }
    try
    {
        std::map<Header, size_t> stHeaderCounts;
        s_eLoudnessMode = sLoudnessMode == ConfigKeys::LooseAudioLoudness_Gain ? LoudnessMode::GainOnly
            : sLoudnessMode == ConfigKeys::LooseAudioLoudness_Limiter ? LoudnessMode::Limiter : LoudnessMode::Off;
        std::map<Header, std::filesystem::path> stCandidates;
        s_stOverloadRoot = BP_FileSys::LoaderOverloadRoot();
        if (!s_stOverloadRoot.empty() && s_stOverloadRoot.is_relative())
        {
            s_stOverloadRoot = sExePath / s_stOverloadRoot;
        }

        for (const auto& stRelative : FindBgmAssets(sExePath, s_stOverloadRoot))
        {
            Header aHeader {};
            const auto stOriginal = ResolveAsset(sExePath, s_stOverloadRoot, stRelative);
            if (!ReadHeader(stOriginal, aHeader))
            {
                continue;
            }
            ++stHeaderCounts[aHeader];
            auto sStem = stRelative.stem().wstring();
            std::transform(sStem.begin(), sStem.end(), sStem.begin(), [](wchar_t nChar) { return static_cast<wchar_t>(std::towlower(nChar)); });
            if (!IsEnabledAsset(sStem, bEnabled))
            {
                continue;
            }
            for (size_t nTrack = 0; nTrack < nMaxTracks; ++nTrack)
            {
                if (!FindReplacementAudio(sExePath, s_stOverloadRoot, stRelative, nTrack).empty())
                {
                    stCandidates[aHeader] = stRelative;
                    break;
                }
            }
            if (const auto* pVariant = FindVariant(stRelative); HasVariantAudio(stRelative, pVariant))
            {
                stCandidates[aHeader] = stRelative;
                s_stVariantHeaders[std::wstring(pVariant->svAsset)] = aHeader;
            }
        }
        for (const auto& [aHeader, stPath] : stCandidates)
        {
            if (stHeaderCounts[aHeader] != 1)
            {
                spdlog::warn("MG1/2: Loose Audio : Ambiguous asset: {}", stPath.string());
                continue;
            }
            s_stCandidates.emplace(aHeader, stPath);
        }
    }
    catch (const std::exception& stError)
    {
        spdlog::error("MG1/2: Loose Audio : Asset indexing failed: {}", stError.what());
        s_stCandidates.clear();
        return;
    }
    if (s_stCandidates.empty())
    {
        if (sDebugLogging == ConfigKeys::LooseAudioDebugLogging_Full)
        {
            spdlog::info("MG1/2: Loose Audio : No replacement audio files could be matched to a unique game asset.");
        }
        return;
    }

    if (sDebugLogging == ConfigKeys::LooseAudioDebugLogging_Full)
    {
        spdlog::info("MG1/2: Loose Audio : Indexed {} assets for on-demand loading.", s_stCandidates.size());
    }

    //NewStreamSoundDriver__Init+2F
    uint8_t* pLoad = Memory::PatternScan(baseModule, "48 8B 4B ?? 48 8B F8 48 85 C9 74", "MG1/2: Loose Audio : NewStreamSoundDriver__Init()+0x2F : prepare BGM replacement");
    //tune_thread+EF
    uint8_t* pClose = Memory::PatternScan(baseModule, "48 89 73 ?? 89 73 ?? E9", "MG1/2: Loose Audio : tune_thread()+0xEF : release closed BGM replacement");
    //sub_1400635D0+156
    uint8_t* pHeader = Memory::PatternScan(baseModule, "41 8B 48 ?? 4C 8D 2D ?? ?? ?? ?? 8B 05", "MG1/2: Loose Audio : CStreamDriver_MTA::ProcessPacket() | roughly @l181: MTA header selection");
    //sub_140064340+2C0
    uint8_t* pPcm = Memory::PatternScan(baseModule, "8B 0D ?? ?? ?? ?? 8B C5 99 F7 3D ?? ?? ?? ?? 8B D8 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 0F AF CE 4C 63 C9", "MG1/2: Loose Audio : CStreamDriver_MTA::ProcessPacket() | roughly @l346: decoded stereo block");
    if (!pLoad || !pClose || !pHeader || !pPcm)
    {
        spdlog::error("MG1/2: Loose Audio : Missing hook addresses.");
    }
    else
    {
        s_hLoad = safetyhook::create_mid(pLoad, Load_hook);
        s_hClose = safetyhook::create_mid(pClose, Close_hook);
        s_hHeader = safetyhook::create_mid(pHeader, Header_hook);
        s_hPcm = safetyhook::create_mid(pPcm, Pcm_hook);
        LOG_HOOK(s_hLoad, "MG1/2: Loose Audio : prepare BGM replacement");
        LOG_HOOK(s_hClose, "MG1/2: Loose Audio : release closed BGM replacement");
        LOG_HOOK(s_hHeader, "MG1/2: Loose Audio : MTA header selection");
        LOG_HOOK(s_hPcm, "MG1/2: Loose Audio : decoded stereo block");
        if (!s_hLoad || !s_hClose || !s_hHeader || !s_hPcm)
        {
            s_hLoad.reset();
            s_hClose.reset();
            s_hHeader.reset();
            s_hPcm.reset();
            return;
        }
        s_bReady.store(true, std::memory_order_release);
    }
}

bool LooseAudioOverrides::HasVariant(std::wstring_view svAsset)
{
    if (!s_bReady.load(std::memory_order_acquire))
    {
        return false;
    }
    const auto stHeader = s_stVariantHeaders.find(std::wstring(svAsset));
    return stHeader != s_stVariantHeaders.end() && s_stCandidates.contains(stHeader->second);
}

bool LooseAudioOverrides::EndVariant(std::wstring_view svAsset)
{
    if (!s_bReady.load(std::memory_order_acquire))
    {
        return false;
    }
    const auto stHeader = s_stVariantHeaders.find(std::wstring(svAsset));
    return stHeader != s_stVariantHeaders.end() && s_stActive.EndVariant(stHeader->second);
}
