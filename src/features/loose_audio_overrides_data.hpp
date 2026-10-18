#pragma once
#include <dr_flac.h>
#include <dr_mp3.h>
#include <dr_wav.h>
#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>
#undef STB_VORBIS_HEADER_ONLY

namespace LooseAudioOverridesData
{
    constexpr size_t nHeaderBytes = 2048;
    constexpr size_t nUnitBytes = 272;
    constexpr size_t nFramesPerUnit = 256;
    constexpr size_t nMaxTracks = 2;
    constexpr uint32_t nMtaType = 0x110001;
    constexpr size_t nMaxAssetBytes = 64 * 1024 * 1024;
    constexpr size_t nCursorStackOffset = 0x20;
    constexpr size_t nPcmStackOffset = 0x30;
    using Header = std::array<uint8_t, nHeaderBytes>;

    enum class LoudnessMode { Off, GainOnly, Limiter };

    struct GainTags
    {
        std::optional<double> fReplayPeak;
        std::optional<double> fReplayGainDb;
    };

    inline bool TagEquals(std::string_view sLeft, std::string_view sRight)
    {
        if (sLeft.size() != sRight.size())
        {
            return false;
        }
        for (size_t nIndex = 0; nIndex < sLeft.size(); ++nIndex)
        {
            char nChar = sLeft[nIndex];
            if (nChar >= 'a' && nChar <= 'z')
            {
                nChar -= 'a' - 'A';
            }
            if (nChar != sRight[nIndex])
            {
                return false;
            }
        }
        return true;
    }

    inline void ReadGainTag(GainTags& stTags, std::string_view sComment)
    {
        const auto nEquals = sComment.find('=');
        if (nEquals == std::string_view::npos)
        {
            return;
        }
        const auto sName = sComment.substr(0, nEquals);
        const bool bPeak = TagEquals(sName, "REPLAYGAIN_TRACK_PEAK");
        if (!bPeak && !TagEquals(sName, "REPLAYGAIN_TRACK_GAIN"))
        {
            return;
        }
        auto sValue = sComment.substr(nEquals + 1);
        while (!sValue.empty() && sValue.front() == ' ')
        {
            sValue.remove_prefix(1);
        }
        if (!sValue.empty() && sValue.front() == '+')
        {
            sValue.remove_prefix(1);
        }
        if (sValue.empty())
        {
            return;
        }
        double fGainDb = 0;
        const auto stParsed = std::from_chars(sValue.data(), sValue.data() + sValue.size(), fGainDb);
        if (stParsed.ec != std::errc() || !std::isfinite(fGainDb)
            || (bPeak ? (fGainDb < 0 || fGainDb > 64) : (fGainDb < -60 || fGainDb > 30)))
        {
            return;
        }
        auto sSuffix = sValue.substr(stParsed.ptr - sValue.data());
        while (!sSuffix.empty() && sSuffix.front() == ' ')
        {
            sSuffix.remove_prefix(1);
        }
        while (!sSuffix.empty() && sSuffix.back() == ' ')
        {
            sSuffix.remove_suffix(1);
        }
        if (!sSuffix.empty() && (bPeak || !TagEquals(sSuffix, "DB")))
        {
            return;
        }
        (bPeak ? stTags.fReplayPeak : stTags.fReplayGainDb) = fGainDb;
    }

    inline void ReadMp3GainMetadata(void* pUserData, const drmp3_metadata* pMetadata)
    {
        if (pMetadata->type != DRMP3_METADATA_TYPE_ID3V2 || pMetadata->rawDataSize < 10)
        {
            return;
        }
        const auto* pData = static_cast<const uint8_t*>(pMetadata->pRawData);
        const auto ReadSize = [](const uint8_t* pBytes, bool bSyncSafe) -> std::optional<size_t>
        {
            size_t nValue = 0;
            for (size_t nIndex = 0; nIndex < 4; ++nIndex)
            {
                if (bSyncSafe && (pBytes[nIndex] & 0x80))
                {
                    return {};
                }
                nValue = (nValue << (bSyncSafe ? 7 : 8)) | pBytes[nIndex];
            }
            return nValue;
        };
        const uint8_t nVersion = pData[3];

        if (memcmp(pData, "ID3", 3) != 0 || (nVersion != 3 && nVersion != 4) || (pData[5] & 0x80))
        {
            return;
        }
        const auto nSize = ReadSize(pData + 6, true);
        if (!nSize || *nSize > pMetadata->rawDataSize - 10)
        {
            return;
        }
        const size_t nEnd = 10 + *nSize;
        size_t nOffset = 10;
        if (pData[5] & 0x40)
        {
            if (nEnd - nOffset < 4)
            {
                return;
            }
            const auto nExtended = ReadSize(pData + nOffset, nVersion == 4);
            if (!nExtended || *nExtended < 6)
            {
                return;
            }
            const size_t nSkip = *nExtended + (nVersion == 3 ? 4 : 0);
            if (nSkip > nEnd - nOffset)
            {
                return;
            }
            nOffset += nSkip;
        }
        auto stTags = *static_cast<GainTags*>(pUserData);
        while (nEnd - nOffset >= 10 && pData[nOffset] != 0)
        {
            const auto* pFrame = pData + nOffset;
            const auto nFrameSize = ReadSize(pFrame + 4, nVersion == 4);
            nOffset += 10;
            if (!nFrameSize || *nFrameSize > nEnd - nOffset)
            {
                return;
            }

            // Skip compressed/encrypted frames.
            if (memcmp(pFrame, "TXXX", 4) == 0 && pFrame[9] == 0 && *nFrameSize > 1 && *nFrameSize <= 4096)
            {
                const auto* pText = pData + nOffset;
                const uint8_t nEncoding = *pText++;
                size_t nRemaining = *nFrameSize - 1;
                std::string sText;
                bool bValid = nEncoding <= 3;
                bool bLittleEndian = false;
                while (bValid && nRemaining > 0)
                {
                    uint16_t nChar = 0;
                    if (nEncoding == 1 || nEncoding == 2)
                    {
                        if (nRemaining < 2)
                        {
                            bValid = false;
                            break;
                        }
                        if (nEncoding == 1 && ((pText[0] == 0xFF && pText[1] == 0xFE) || (pText[0] == 0xFE && pText[1] == 0xFF)))
                        {
                            bLittleEndian = pText[0] == 0xFF;
                            pText += 2;
                            nRemaining -= 2;
                            continue;
                        }
                        nChar = bLittleEndian ? pText[0] | (uint16_t(pText[1]) << 8) : (uint16_t(pText[0]) << 8) | pText[1];
                        pText += 2;
                        nRemaining -= 2;
                    }
                    else
                    {
                        nChar = *pText++;
                        --nRemaining;
                    }

                    bValid = nChar < 128;
                    sText.push_back(static_cast<char>(nChar));
                }
                const auto nSeparator = sText.find('\0');
                if (bValid && nSeparator != std::string::npos)
                {
                    sText[nSeparator] = '=';
                    while (!sText.empty() && sText.back() == '\0')
                    {
                        sText.pop_back();
                    }
                    ReadGainTag(stTags, sText);
                }
            }
            nOffset += *nFrameSize;
        }
        *static_cast<GainTags*>(pUserData) = stTags;
    }

    inline void ReadGainMetadata(void* pUserData, drflac_metadata* pMetadata)
    {
        if (pMetadata->type != DRFLAC_METADATA_BLOCK_TYPE_VORBIS_COMMENT)
        {
            return;
        }
        auto& stTags = *static_cast<GainTags*>(pUserData);
        drflac_vorbis_comment_iterator stIterator {};
        const auto& stComments = pMetadata->data.vorbis_comment;
        drflac_init_vorbis_comment_iterator(&stIterator, stComments.commentCount, stComments.pComments);
        drflac_uint32 nLength = 0;
        while (const char* pszComment = drflac_next_vorbis_comment(&stIterator, &nLength))
        {
            ReadGainTag(stTags, std::string_view(pszComment, nLength));
        }
    }

    inline void ApplyLoudness(const std::vector<float>& vSamples, std::vector<int16_t>& vPcm, LoudnessMode eMode, double fGainDb, double fTaggedPeak = 0)
    {
        constexpr size_t nLookaheadFrames = 240; // 5 ms at 48 kHz; output timing is the same
        const double fCeiling = std::floor(32767.0 * std::pow(10.0, -1.0 / 20));
        double fGain = eMode == LoudnessMode::Off ? 1 : std::pow(10.0, fGainDb / 20);
        if (eMode == LoudnessMode::GainOnly)
        {
            double fPeak = fTaggedPeak * 32768.0;
            for (const float fSample : vSamples)
            {
                fPeak = std::max(fPeak, std::abs(double(fSample)));
            }
            if (fPeak > 0)
            {
                fGain = std::min(fGain, fCeiling / fPeak);
            }
        }
        const size_t nFrames = vSamples.size() / 2;
        std::deque<size_t> stPeaks;
        size_t nNext = 0;
        double fEnvelope = 1;
        const double fRelease = std::exp(-1.0 / (48000 * 0.060));
        const auto Peak = [&](size_t nFrame)
        {
            return std::max(std::abs(double(vSamples[nFrame * 2])), std::abs(double(vSamples[nFrame * 2 + 1]))) * fGain;
        };
        vPcm.resize(vSamples.size());
        for (size_t nFrame = 0; nFrame < nFrames; ++nFrame)
        {
            if (eMode == LoudnessMode::Limiter)
            {
                while (!stPeaks.empty() && stPeaks.front() < nFrame)
                {
                    stPeaks.pop_front();
                }
                while (nNext < nFrames && nNext <= nFrame + nLookaheadFrames)
                {
                    while (!stPeaks.empty() && Peak(stPeaks.back()) <= Peak(nNext))
                    {
                        stPeaks.pop_back();
                    }
                    stPeaks.push_back(nNext++);
                }
                const double fPeak = Peak(stPeaks.front());
                const double fRequired = fPeak > fCeiling ? fCeiling / fPeak : 1;
                fEnvelope = std::min(fRequired, 1 - (1 - fEnvelope) * fRelease);
            }
            for (size_t nChannel = 0; nChannel < 2; ++nChannel)
            {
                const size_t nIndex = nFrame * 2 + nChannel;
                vPcm[nIndex] = static_cast<int16_t>(std::clamp(std::lround(vSamples[nIndex] * fGain * fEnvelope), -32768L, 32767L));
            }
        }
    }

    inline bool IsEnabledAsset(const std::wstring& sStem, bool bEnabled)
    {
        return bEnabled && (sStem.starts_with(L"mg1_") || sStem.starts_with(L"mg2_"));
    }

    inline bool MatchesEncodedLength(size_t nActualBytes, size_t nDeclaredBytes, size_t nGroupBytes, size_t nEndGroup)
    {
        // hacky vanilla fix: MG1's underground sdt incorrectly reports 1 extra block than is actually encoded, lets not pass beyond the real data stream size
        return nActualBytes == nDeclaredBytes || (nDeclaredBytes == nActualBytes + nGroupBytes
            && nGroupBytes != 0 && nActualBytes / nGroupBytes == nEndGroup && nActualBytes % nGroupBytes == 0);
    }

    inline std::filesystem::path ResolveAsset(const std::filesystem::path& stGameRoot, const std::filesystem::path& stOverloadRoot, const std::filesystem::path& stRelative)
    {
        if (!stOverloadRoot.empty() && std::filesystem::is_regular_file(stOverloadRoot / stRelative))
        {
            return stOverloadRoot / stRelative;
        }
        return stGameRoot / stRelative;
    }

    inline std::filesystem::path FindReplacementAudio(const std::filesystem::path& stGameRoot, const std::filesystem::path& stOverloadRoot, const std::filesystem::path& stRelative, size_t nTrack, std::wstring_view svVariant = {})
    {
        const std::wstring sVariant = svVariant.empty() ? std::wstring() : L"." + std::wstring(svVariant);
        for (const auto& stRoot : { stOverloadRoot, stGameRoot })
        {
            if (stRoot.empty())
            {
                continue;
            }
            for (const auto* pszExtension : { L".flac", L".wav", L".ogg", L".mp3" })
            {
                const auto stPath = stRoot / stRelative.parent_path() / (stRelative.stem().wstring() + sVariant + L".track" + std::to_wstring(nTrack) + pszExtension);
                if (std::filesystem::is_regular_file(stPath))
                {
                    return stPath;
                }
            }
        }
        return {};
    }

    inline std::vector<std::filesystem::path> FindBgmAssets(const std::filesystem::path& stGameRoot, const std::filesystem::path& stOverloadRoot)
    {
        std::map<std::wstring, std::filesystem::path> stRelativePaths;
        for (const auto& stRoot : { stGameRoot, stOverloadRoot })
        {
            if (stRoot.empty() || !std::filesystem::is_directory(stRoot))
            {
                continue;
            }
            for (const auto& stRegion : std::filesystem::directory_iterator(stRoot))
            {
                if (!stRegion.is_directory())
                {
                    continue;
                }
                const auto stBgm = stRegion.path() / "bgm_2";
                if (!std::filesystem::is_directory(stBgm))
                {
                    continue;
                }
                for (const auto& stFile : std::filesystem::directory_iterator(stBgm))
                {
                    if (!stFile.is_regular_file())
                    {
                        continue;
                    }
                    const auto stRelative = stRegion.path().filename() / "bgm_2" / stFile.path().filename();
                    auto sKey = stRelative.generic_wstring();
                    std::transform(sKey.begin(), sKey.end(), sKey.begin(), [](wchar_t nChar) { return static_cast<wchar_t>(std::towlower(nChar)); });
                    if (std::filesystem::path(sKey).extension() == L".sdt")
                    {
                        stRelativePaths[sKey] = stRelative;
                    }
                }
            }
        }
        std::vector<std::filesystem::path> vResult;
        for (const auto& [sKey, stRelative] : stRelativePaths)
        {
            vResult.push_back(stRelative);
        }
        return vResult;
    }

    struct LoopPoints
    {
        uint64_t nStart = 0;
        uint64_t nEnd = 0;
        uint64_t nSampleRate = 0;
        uint64_t nSourceFrames = 0;
    };

    inline bool ParseLoopPoints(const std::string& sJson, LoopPoints& stLoop)
    {
        stLoop = {};
        if (sJson.size() > 4096)
        {
            return false;
        }
        const auto nFirst = sJson.find_first_not_of(" \t\r\n");
        const auto nLast = sJson.find_last_not_of(" \t\r\n");
        if (nFirst == std::string::npos || sJson[nFirst] != '{' || sJson[nLast] != '}')
        {
            return false;
        }
        static const std::regex stField(R"json([ \t\r\n]*"([a-z_]+)"[ \t\r\n]*:[ \t\r\n]*(true|false|-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)[ \t\r\n]*)json");
        std::map<std::string, std::string> stFields;
        auto stPosition = sJson.begin() + nFirst + 1;
        const auto stEnd = sJson.begin() + nLast;
        while (stPosition != stEnd)
        {
            std::smatch stMatch;
            if (!std::regex_search(stPosition, stEnd, stMatch, stField, std::regex_constants::match_continuous) || !stFields.emplace(stMatch[1].str(), stMatch[2].str()).second)
            {
                return false;
            }
            stPosition = stMatch[0].second;
            if (stPosition == stEnd)
            {
                break;
            }
            if (*stPosition++ != ',' || stPosition == stEnd)
            {
                return false;
            }
        }
        const auto ReadInteger = [&](const char* pszKey, uint64_t& nValue)
        {
            const auto stFound = stFields.find(pszKey);
            if (stFound == stFields.end())
            {
                return false;
            }
            const auto& sValue = stFound->second;
            const auto stResult = std::from_chars(sValue.data(), sValue.data() + sValue.size(), nValue);
            return stResult.ec == std::errc() && stResult.ptr == sValue.data() + sValue.size();
        };
        LoopPoints stParsed;
        if (!stFields.contains("schema"))
        {
            if (stFields.size() != 2 || !ReadInteger("loop_start", stParsed.nStart) || !ReadInteger("loop_end", stParsed.nEnd) || stParsed.nStart >= stParsed.nEnd)
            {
                return false;
            }
        }
        else
        {
            uint64_t nSchema = 0;
            uint64_t nStartFrame = 0;
            if (!ReadInteger("schema", nSchema) || nSchema != 1 || !ReadInteger("sample_rate", stParsed.nSampleRate) || !ReadInteger("source_frames", stParsed.nSourceFrames) || !ReadInteger("start_frame", nStartFrame) || nStartFrame > stParsed.nSourceFrames || stParsed.nSourceFrames == 0 || (stParsed.nSampleRate != 44100 && stParsed.nSampleRate != 48000))
            {
                return false;
            }
            const auto stOneShot = stFields.find("one_shot");
            if (stOneShot == stFields.end() || (stOneShot->second != "true" && stOneShot->second != "false"))
            {
                return false;
            }
            if (stFields.contains("loop_start") || stFields.contains("loop_end") || stOneShot->second == "false")
            {
                if (!ReadInteger("loop_start", stParsed.nStart) || !ReadInteger("loop_end", stParsed.nEnd) || stParsed.nStart >= stParsed.nEnd || stParsed.nEnd > stParsed.nSourceFrames)
                {
                    return false;
                }
            }
            if (stOneShot->second == "true")
            {
                stParsed.nStart = stParsed.nEnd = 0;
            }
            for (const auto& [sKey, sValue] : stFields)
            {
                if (sKey == "peak" || sKey == "peak_dbfs" || sKey == "loudness_lufs")
                {
                    double fValue = 0;
                    const auto stResult = std::from_chars(sValue.data(), sValue.data() + sValue.size(), fValue);
                    if (stResult.ec != std::errc() || stResult.ptr != sValue.data() + sValue.size() || !std::isfinite(fValue) || (sKey == "peak" && fValue < 0))
                    {
                        return false;
                    }
                }
                else if (sKey != "schema" && sKey != "sample_rate" && sKey != "source_frames" && sKey != "start_frame" && sKey != "one_shot" && sKey != "loop_start" && sKey != "loop_end")
                {
                    return false;
                }
            }
        }
        stLoop = stParsed;
        return true;
    }

    inline LoopPoints ReadLoopPoints(std::filesystem::path stPath, std::string& sError)
    {
        sError.clear();
        stPath.replace_extension(L".json");
        std::error_code stError;
        if (!std::filesystem::exists(stPath, stError) && !stError)
        {
            return {};
        }
        std::ifstream stFile(stPath, std::ios::binary | std::ios::ate);
        const auto nSize = stFile.tellg();
        if (nSize <= 0 || nSize > 4096)
        {
            sError = "JSON is unreadable, empty or larger than 4096 bytes";
            return {};
        }
        std::string sJson(static_cast<size_t>(nSize), '\0');
        stFile.seekg(0);
        if (!stFile.read(sJson.data(), sJson.size()))
        {
            sError = "could not read JSON";
            return {};
        }
        if (sJson.starts_with("\xEF\xBB\xBF"))
        {
            sJson.erase(0, 3);
        }
        LoopPoints stLoop;
        if (!ParseLoopPoints(sJson, stLoop))
        {
            sError = "invalid loop JSON or unsupported schema";
        }
        return stLoop;
    }

    struct Replacement
    {
        Header aHeader {};
        std::vector<uint8_t> vEncoded;
        size_t nTracks = 0;
        std::array<std::vector<int16_t>, nMaxTracks> avPcm;
        std::array<LoopPoints, nMaxTracks> astLoops {};
        bool bStreamLoops = false;
        // Audio to play after the variant; empty tracks resume MC audio.
        bool bVariant = false;
        std::array<bool, nMaxTracks> abVariantTracks {};
        std::array<std::vector<int16_t>, nMaxTracks> avAfterPcm;
        std::array<LoopPoints, nMaxTracks> astAfterLoops {};
        std::array<std::vector<int16_t>, nMaxTracks> avFadePcm;
        std::array<LoopPoints, nMaxTracks> astFadeLoops {};
        std::array<size_t, nMaxTracks> anFadeCursors {};
        std::array<size_t, nMaxTracks> anFadeLeft {};
    };

    constexpr size_t nVariantCrossfadeFrames = 720; // 15 ms at 48 kHz

    inline void SwitchToAfter(Replacement& stAudio, size_t nTrack, bool bCrossfade = false, size_t nCursor = 0)
    {
        stAudio.avFadePcm[nTrack].clear();
        stAudio.anFadeLeft[nTrack] = 0;
        if (bCrossfade)
        {
            stAudio.avFadePcm[nTrack] = std::move(stAudio.avPcm[nTrack]);
            stAudio.astFadeLoops[nTrack] = stAudio.astLoops[nTrack];
            stAudio.anFadeCursors[nTrack] = nCursor;
            stAudio.anFadeLeft[nTrack] = nVariantCrossfadeFrames;
        }
        stAudio.avPcm[nTrack] = std::move(stAudio.avAfterPcm[nTrack]);
        stAudio.avAfterPcm[nTrack].clear();
        stAudio.astLoops[nTrack] = stAudio.astAfterLoops[nTrack];
    }

    inline uint32_t Read32(const uint8_t* pData)
    {
        uint32_t nValue;
        memcpy(&nValue, pData, sizeof(nValue));
        return nValue;
    }

    inline bool ReadHeader(const std::filesystem::path& stPath, Header& aHeader)
    {
        std::ifstream stFile(stPath, std::ios::binary);
        std::array<uint8_t, 32> aTags {};
        return stFile.read(reinterpret_cast<char*>(aTags.data()), aTags.size())
            && Read32(aTags.data()) == 0x10 && Read32(aTags.data() + 4) == 16
            && Read32(aTags.data() + 12) == nMtaType
            && Read32(aTags.data() + 16) == nMtaType
            && Read32(aTags.data() + 20) == nHeaderBytes + 16
            && stFile.read(reinterpret_cast<char*>(aHeader.data()), aHeader.size())
            && Read32(aHeader.data()) == 0x4641544D;
    }

    inline bool ReadOriginal(const std::filesystem::path& stPath, Replacement& stAudio)
    {
        stAudio = {};
        std::ifstream stFile(stPath, std::ios::binary | std::ios::ate);
        const auto nLength = stFile.tellg();
        if (nLength < 0 || static_cast<uint64_t>(nLength) > nMaxAssetBytes)
        {
            return false;
        }
        std::vector<uint8_t> vFile(static_cast<size_t>(nLength));
        stFile.seekg(0);
        if (!stFile.read(reinterpret_cast<char*>(vFile.data()), vFile.size()) || !ReadHeader(stPath, stAudio.aHeader))
        {
            return false;
        }
        const uint32_t nChannels = Read32(stAudio.aHeader.data() + 0x4C);
        if (nChannels == 0 || nChannels % 2 != 0 || nChannels / 2 > nMaxTracks)
        {
            return false;
        }
        stAudio.nTracks = nChannels / 2;
        const size_t nGroupBytes = stAudio.nTracks * nUnitBytes;
        if (Read32(stAudio.aHeader.data() + 0x60) != nGroupBytes)
        {
            return false;
        }

        // Contiguous mode-0 stereo pairs only
        for (size_t nChannel = 0; nChannel < 16; ++nChannel)
        {
            const auto* pChannel = stAudio.aHeader.data() + 0x100 + nChannel * 112;
            if (Read32(pChannel) != (nChannel < nChannels ? 0 : 0xFFFFFFFF)
                || (nChannel < nChannels && (Read32(pChannel + 0x24) & 2) != 0))
            {
                return false;
            }
        }
        size_t nOffset = 32 + nHeaderBytes;
        bool bEnd = false;
        while (nOffset + 16 <= vFile.size())
        {
            const auto* pTag = vFile.data() + nOffset;
            const uint32_t nType = Read32(pTag);
            const uint32_t nSize = Read32(pTag + 4);
            const uint32_t nCount = Read32(pTag + 12);
            if (nSize < 16 || nSize > vFile.size() - nOffset)
            {
                return false;
            }
            if (nType == 0xF0)
            {
                bEnd = nSize == 16 && nOffset + nSize == vFile.size();
                stAudio.bStreamLoops = nCount != 0 && (Read32(stAudio.aHeader.data() + 0x70) & 1) != 0;
                break;
            }
            if (nType != nMtaType || Read32(pTag + 8) != 0 || uint64_t(nCount) * nGroupBytes > nSize - 16)
            {
                return false;
            }
            stAudio.vEncoded.insert(stAudio.vEncoded.end(), pTag + 16, pTag + 16 + nCount * nGroupBytes);
            nOffset += nSize;
        }
        if (!bEnd || stAudio.vEncoded.empty() || stAudio.vEncoded.size() % nGroupBytes != 0
            || !MatchesEncodedLength(stAudio.vEncoded.size(), Read32(stAudio.aHeader.data() + 0x7FC), nGroupBytes, Read32(stAudio.aHeader.data() + 0x68)))
        {
            return false;
        }
        for (size_t nUnit = 0; nUnit < stAudio.vEncoded.size() / nUnitBytes; ++nUnit)
        {
            const uint32_t nId = Read32(stAudio.vEncoded.data() + nUnit * nUnitBytes);
            if ((nId >> 8) != nUnit + 1 || (nId & 0xF) != nUnit % stAudio.nTracks)
            {
                return false;
            }
        }
        return true;
    }

    struct ClipLength
    {
        double fFileSeconds = 0;
        double fExpectedSeconds = 0;
        std::string sLoopError;
    };

    inline bool DecodeAudio(const std::filesystem::path& stPath, Replacement& stAudio, size_t nTrack = 0, LoudnessMode eMode = LoudnessMode::Off, ClipLength* pLength = nullptr)
    {
        if (pLength)
        {
            *pLength = {};
        }
        if (nTrack >= stAudio.nTracks || stAudio.nTracks > nMaxTracks)
        {
            return false;
        }
        auto& vPcm = stAudio.avPcm[nTrack];
        vPcm.clear();
        stAudio.astLoops[nTrack] = {};
        std::string sLoopError;
        auto stLoop = ReadLoopPoints(stPath, sLoopError);
        if (stLoop.nEnd && (!stAudio.bStreamLoops || (Read32(stAudio.aHeader.data() + 0x124 + nTrack * 224) & 1) == 0))
        {
            sLoopError = "original track does not loop";
            stLoop = {};
        }
        const size_t nExpectedFrames = stAudio.vEncoded.size() / (nUnitBytes * stAudio.nTracks) * nFramesPerUnit;
        size_t nFrames = nExpectedFrames;
        if (nFrames > nMaxAssetBytes / (sizeof(int16_t) * 2))
        {
            return false;
        }
        GainTags stTags;
        uint32_t nRate = 0;
        uint32_t nChannels = 0;
        size_t nSourceFrames = 0;
        uint64_t nTotalSourceFrames = 0;
        std::vector<int16_t> vSource;
        const auto Prepare = [&](uint64_t nTotal)
        {
            if (nChannels < 1 || nChannels > 2 || (nRate != 44100 && nRate != 48000) || nTotal == 0)
            {
                return false;
            }
            nTotalSourceFrames = nTotal;
            if (stLoop.nSampleRate && (stLoop.nSampleRate != nRate || stLoop.nSourceFrames != nTotal))
            {
                sLoopError = "JSON sample_rate/source_frames does not match the audio";
                stLoop = {};
            }
            if (stLoop.nEnd)
            {
                if (stLoop.nEnd > nTotal || stLoop.nEnd > uint64_t(nMaxAssetBytes / 4) * nRate / 48000)
                {
                    sLoopError = "loop_end exceeds the audio length or the 64 MiB decoded-track limit";
                    stLoop = {};
                }
                else
                {
                    stLoop.nStart = (stLoop.nStart * 48000 + nRate / 2) / nRate;
                    stLoop.nEnd = (stLoop.nEnd * 48000 + nRate / 2) / nRate;
                    nFrames = static_cast<size_t>(stLoop.nEnd);
                }
            }
            nSourceFrames = static_cast<size_t>(std::min<uint64_t>(nTotal, (uint64_t(nFrames) * nRate + 47999) / 48000 + 64));
            vSource.resize(nSourceFrames * nChannels);
            return true;
        };
        auto sExtension = stPath.extension().wstring();
        std::transform(sExtension.begin(), sExtension.end(), sExtension.begin(), [](wchar_t nChar) { return static_cast<wchar_t>(std::towlower(nChar)); });
        if (sExtension == L".flac")
        {
            std::unique_ptr<drflac, void(*)(drflac*)> pFlac(drflac_open_file_with_metadata_w(stPath.c_str(), ReadGainMetadata, &stTags, nullptr), drflac_close);
            if (!pFlac || pFlac->bitsPerSample > 16)
            {
                return false;
            }
            nRate = pFlac->sampleRate;
            nChannels = pFlac->channels;
            if (!Prepare(pFlac->totalPCMFrameCount) || drflac_read_pcm_frames_s16(pFlac.get(), nSourceFrames, vSource.data()) != nSourceFrames)
            {
                return false;
            }
        }
        else if (sExtension == L".wav")
        {
            drwav stWav {};
            if (!drwav_init_file_w(&stWav, stPath.c_str(), nullptr))
            {
                return false;
            }
            const auto Close = [](drwav* pWav) { drwav_uninit(pWav); };
            std::unique_ptr<drwav, decltype(Close)> pWav(&stWav, Close);
            nRate = stWav.sampleRate;
            nChannels = stWav.channels;
            if (stWav.translatedFormatTag != DR_WAVE_FORMAT_PCM || stWav.bitsPerSample != 16 || !Prepare(stWav.totalPCMFrameCount)
                || drwav_read_pcm_frames_s16(&stWav, nSourceFrames, vSource.data()) != nSourceFrames)
            {
                return false;
            }
        }
        else if (sExtension == L".mp3")
        {
            drmp3 stMp3 {};
            if (!drmp3_init_file_with_metadata_w(&stMp3, stPath.c_str(), ReadMp3GainMetadata, &stTags, nullptr))
            {
                return false;
            }
            const auto Close = [](drmp3* pMp3) { drmp3_uninit(pMp3); };
            std::unique_ptr<drmp3, decltype(Close)> pMp3(&stMp3, Close);
            nRate = stMp3.sampleRate;
            nChannels = stMp3.channels;
            if (!Prepare(drmp3_get_pcm_frame_count(&stMp3)) || !drmp3_seek_to_pcm_frame(&stMp3, 0)
                || drmp3_read_pcm_frames_s16(&stMp3, nSourceFrames, vSource.data()) != nSourceFrames)
            {
                return false;
            }
        }
        else if (sExtension == L".ogg")
        {
            FILE* pFile = _wfopen(stPath.c_str(), L"rb");
            if (!pFile)
            {
                return false;
            }
            int nError = 0;
            const auto Close = [](FILE* pInput) { fclose(pInput); };
            std::unique_ptr<FILE, decltype(Close)> pInput(pFile, Close);
            std::unique_ptr<stb_vorbis, void(*)(stb_vorbis*)> pVorbis(stb_vorbis_open_file(pFile, 0, &nError, nullptr), stb_vorbis_close);
            if (!pVorbis)
            {
                return false;
            }
            const auto stInfo = stb_vorbis_get_info(pVorbis.get());
            nRate = stInfo.sample_rate;
            nChannels = stInfo.channels;
            const auto stComments = stb_vorbis_get_comment(pVorbis.get());
            for (int nIndex = 0; nIndex < stComments.comment_list_length; ++nIndex)
            {
                ReadGainTag(stTags, stComments.comment_list[nIndex]);
            }
            if (!Prepare(stb_vorbis_stream_length_in_samples(pVorbis.get()))
                || stb_vorbis_get_samples_short_interleaved(pVorbis.get(), nChannels, vSource.data(), static_cast<int>(vSource.size())) != nSourceFrames)
            {
                return false;
            }
        }
        else
        {
            return false;
        }
        vPcm.assign(nFrames * 2, 0);
        const auto fTaggedGain = stTags.fReplayGainDb;
        const bool bProcess = eMode != LoudnessMode::Off && fTaggedGain.has_value();
        std::vector<float> vSamples;
        if (bProcess)
        {
            vSamples.assign(nFrames * 2, 0);
        }

        // Resample on the stream-loading thread, before audio command is queued.
        constexpr int nTaps = 64;
        constexpr int nPhases = 1024;
        std::vector<double> vFilter;
        if (nRate != 48000)
        {
            vFilter.resize(nTaps * nPhases);
            for (int nPhase = 0; nPhase < nPhases; ++nPhase)
            {
                double fSum = 0;
                for (int nTap = 0; nTap < nTaps; ++nTap)
                {
                    const double fX = nTap - 31 - double(nPhase) / nPhases;
                    const double fWindow = 0.42 + 0.5 * std::cos(std::numbers::pi * fX / 32)
                        + 0.08 * std::cos(2 * std::numbers::pi * fX / 32);
                    const double fSinc = std::abs(fX) < 1e-12 ? 1 : std::sin(std::numbers::pi * fX) / (std::numbers::pi * fX);
                    fSum += vFilter[nPhase * nTaps + nTap] = fSinc * fWindow;
                }
                for (int nTap = 0; nTap < nTaps; ++nTap)
                {
                    vFilter[nPhase * nTaps + nTap] /= fSum;
                }
            }
        }
        const size_t nAudibleFrames = static_cast<size_t>(std::min<uint64_t>(nFrames,
            (uint64_t(nSourceFrames) * 48000 + nRate - 1) / nRate));
        for (size_t nFrame = 0; nFrame < nAudibleFrames; ++nFrame)
        {
            const uint64_t nPosition = uint64_t(nFrame) * nRate;
            const size_t nBase = static_cast<size_t>(nPosition / 48000);
            const size_t nPhase = (nPosition % 48000) * nPhases / 48000;
            for (size_t nChannel = 0; nChannel < 2; ++nChannel)
            {
                const size_t nSourceChannel = nChannels == 1 ? 0 : nChannel;
                double fSample = 0;
                if (nRate == 48000)
                {
                    fSample = vSource[nBase * nChannels + nSourceChannel];
                }
                else
                {
                    for (int nTap = 0; nTap < nTaps; ++nTap)
                    {
                        const int64_t nIndex = int64_t(nBase) + nTap - 31;
                        if (nIndex >= 0 && uint64_t(nIndex) < nSourceFrames)
                        {
                            fSample += vSource[size_t(nIndex) * nChannels + nSourceChannel] * vFilter[nPhase * nTaps + nTap];
                        }
                    }
                }
                if (bProcess)
                {
                    vSamples[nFrame * 2 + nChannel] = static_cast<float>(fSample);
                }
                else
                {
                    vPcm[nFrame * 2 + nChannel] = static_cast<int16_t>(std::clamp(std::lround(fSample), -32768L, 32767L));
                }
            }
        }
        if (bProcess)
        {
            ApplyLoudness(vSamples, vPcm, eMode, *fTaggedGain, stTags.fReplayPeak.value_or(0));
        }
        if (pLength)
        {
            pLength->fFileSeconds = double(nTotalSourceFrames) / nRate;
            pLength->fExpectedSeconds = double(nExpectedFrames) / 48000;
            pLength->sLoopError = sLoopError;
        }
        stAudio.astLoops[nTrack] = stLoop;
        return true;
    }

    inline bool CopyBlock(Replacement& stAudio, const uint8_t* pUnit, int16_t* pPcm, size_t nTrack = 0, size_t* pCursor = nullptr)
    {
        if (nTrack >= stAudio.nTracks || stAudio.nTracks > nMaxTracks || stAudio.avPcm[nTrack].empty())
        {
            return false;
        }
        const uint32_t nSequence = Read32(pUnit) >> 8;
        if (nSequence == 0 || nSequence > stAudio.vEncoded.size() / nUnitBytes)
        {
            return false;
        }
        const size_t nUnit = nSequence - 1;
        const size_t nGroup = nUnit / stAudio.nTracks;
        const auto& vPcm = stAudio.avPcm[nTrack];
        if (nUnit % stAudio.nTracks != nTrack || (Read32(pUnit) & 0xF) != nTrack
            || memcmp(pUnit, stAudio.vEncoded.data() + nUnit * nUnitBytes, nUnitBytes) != 0)
        {
            return false;
        }
        const auto& stLoop = stAudio.astLoops[nTrack];
        if (stLoop.nEnd)
        {
            if (!pCursor || stLoop.nStart >= stLoop.nEnd || stLoop.nEnd > vPcm.size() / 2 || *pCursor >= stLoop.nEnd)
            {
                return false;
            }
            for (size_t nFrame = 0; nFrame < nFramesPerUnit; ++nFrame)
            {
                pPcm[nFrame] = vPcm[*pCursor * 2];
                pPcm[nFramesPerUnit + nFrame] = vPcm[*pCursor * 2 + 1];
                if (++*pCursor == stLoop.nEnd)
                {
                    *pCursor = static_cast<size_t>(stLoop.nStart);
                }
                if (stAudio.anFadeLeft[nTrack])
                {
                    const auto& vFade = stAudio.avFadePcm[nTrack];
                    const auto& stFadeLoop = stAudio.astFadeLoops[nTrack];
                    auto& nFadeCursor = stAudio.anFadeCursors[nTrack];
                    const double fPhase = double(nVariantCrossfadeFrames - stAudio.anFadeLeft[nTrack]) / nVariantCrossfadeFrames * std::numbers::pi / 2;
                    for (size_t nChannel = 0; nChannel < 2; ++nChannel)
                    {
                        auto& nSample = pPcm[nChannel * nFramesPerUnit + nFrame];
                        const double fMixed = vFade[nFadeCursor * 2 + nChannel] * std::cos(fPhase) + nSample * std::sin(fPhase);
                        nSample = static_cast<int16_t>(std::clamp(std::lround(fMixed), -32768L, 32767L));
                    }
                    if (++nFadeCursor == stFadeLoop.nEnd)
                    {
                        nFadeCursor = static_cast<size_t>(stFadeLoop.nStart);
                    }
                    if (--stAudio.anFadeLeft[nTrack] == 0)
                    {
                        stAudio.avFadePcm[nTrack].clear();
                    }
                }
            }
            return true;
        }
        if (vPcm.size() / (nFramesPerUnit * 2) <= nGroup)
        {
            return false;
        }
        const auto* pSource = vPcm.data() + nGroup * nFramesPerUnit * 2;
        for (size_t nFrame = 0; nFrame < nFramesPerUnit; ++nFrame)
        {
            pPcm[nFrame] = pSource[2 * nFrame];
            pPcm[nFramesPerUnit + nFrame] = pSource[2 * nFrame + 1];
        }
        return true;
    }

    class ActiveReplacement
    {
        std::atomic_flag stGate = ATOMIC_FLAG_INIT;
        std::unique_ptr<Replacement> pAudio;
        bool bSelected = false;
        std::array<size_t, nMaxTracks> anCursors {};

    public:
        // only the loading thread waits
        void Replace(std::unique_ptr<Replacement> pNext)
        {
            while (stGate.test_and_set(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }
            pAudio.swap(pNext);
            bSelected = false;
            anCursors = {};
            stGate.clear(std::memory_order_release);
        }

        bool Select(const uint8_t* pHeader)
        {
            if (stGate.test_and_set(std::memory_order_acquire))
            {
                return false;
            }
            bSelected = pAudio && pHeader && memcmp(pHeader, pAudio->aHeader.data(), nHeaderBytes) == 0;
            const bool bResult = bSelected;
            stGate.clear(std::memory_order_release);
            return bResult;
        }

        bool EndVariant(const Header& aHeader)
        {
            while (stGate.test_and_set(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }
            const bool bResult = pAudio && pAudio->bVariant && pAudio->aHeader == aHeader;
            if (bResult)
            {
                pAudio->bVariant = false;
                for (size_t nTrack = 0; nTrack < pAudio->nTracks; ++nTrack)
                {
                    if (!pAudio->abVariantTracks[nTrack])
                    {
                        continue;
                    }
                    pAudio->abVariantTracks[nTrack] = false;
                    const bool bCrossfade = !pAudio->avPcm[nTrack].empty() && pAudio->astLoops[nTrack].nEnd && !pAudio->avAfterPcm[nTrack].empty();
                    SwitchToAfter(*pAudio, nTrack, bCrossfade, anCursors[nTrack]);
                    anCursors[nTrack] = 0;
                }
            }
            stGate.clear(std::memory_order_release);
            return bResult;
        }

        bool Copy(const uint8_t* pUnit, int16_t* pPcm, size_t nTrack)
        {
            if (stGate.test_and_set(std::memory_order_acquire))
            {
                return false;
            }
            bool bResult = false;
            if (bSelected && pAudio && nTrack < pAudio->nTracks && !pAudio->avPcm[nTrack].empty())
            {
                bResult = CopyBlock(*pAudio, pUnit, pPcm, nTrack, &anCursors[nTrack]);
                if (!bResult)
                {
                    bSelected = false;
                }
            }
            stGate.clear(std::memory_order_release);
            return bResult;
        }
    };
}
