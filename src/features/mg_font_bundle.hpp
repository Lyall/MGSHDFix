#pragma once

// Unity bundles are BE & LZ4
namespace MG_FontData
{
    constexpr size_t nMaximumBytes = 64 * 1024 * 1024;

    struct Reader
    {
        const std::vector<uint8_t>& vBytes;
        size_t nPosition = 0;
        bool bValid = true;

        uint64_t Read(size_t nCount)
        {
            if (nPosition > vBytes.size() || nCount > vBytes.size() - nPosition)
            {
                bValid = false;
                return 0;
            }
            uint64_t nValue = 0;
            for (size_t nIndex = 0; nIndex < nCount; nIndex++)
            {
                nValue = (nValue << 8) | vBytes[nPosition++];
            }
            return nValue;
        }

        void SkipString()
        {
            while (bValid && Read(1) != 0)
            {
            }
        }

        void Align()
        {
            nPosition = (nPosition + 15) & ~size_t(15);
        }
    };

    bool Decode(const std::vector<uint8_t>& vInput, size_t nOffset, size_t nCompressed, size_t nSize, unsigned int nCompression, std::vector<uint8_t>& vOutput)
    {
        if (nOffset > vInput.size() || nCompressed > vInput.size() - nOffset || nSize > nMaximumBytes)
        {
            return false;
        }
        vOutput.clear();
        if (nCompression == 0)
        {
            if (nCompressed != nSize)
            {
                return false;
            }
            vOutput.assign(vInput.begin() + nOffset, vInput.begin() + nOffset + nSize);
            return true;
        }
        if (nCompression != 2 && nCompression != 3)
        {
            return false;
        }
        const size_t nEnd = nOffset + nCompressed;
        vOutput.reserve(nSize);
        auto ReadLength = [&](size_t& nLength)
        {
            if (nLength == 15)
            {
                unsigned int nExtra = 255;
                while (nExtra == 255)
                {
                    if (nOffset == nEnd)
                    {
                        return false;
                    }
                    nExtra = vInput[nOffset++];
                    nLength += nExtra;
                    if (nLength > nSize)
                    {
                        return false;
                    }
                }
            }
            return true;
        };
        while (nOffset < nEnd)
        {
            const unsigned int nToken = vInput[nOffset++];
            size_t nLength = nToken >> 4;
            if (!ReadLength(nLength) || nLength > nEnd - nOffset || nLength > nSize - vOutput.size())
            {
                return false;
            }
            vOutput.insert(vOutput.end(), vInput.begin() + nOffset, vInput.begin() + nOffset + nLength);
            nOffset += nLength;
            if (nOffset == nEnd)
            {
                break;
            }
            if (nEnd - nOffset < 2)
            {
                return false;
            }
            const size_t nDistance = vInput[nOffset] | (size_t(vInput[nOffset + 1]) << 8);
            nOffset += 2;
            nLength = nToken & 15;
            if (!ReadLength(nLength) || nDistance == 0 || nDistance > vOutput.size() || nLength + 4 > nSize - vOutput.size())
            {
                return false;
            }
            nLength += 4;
            for (size_t nIndex = 0; nIndex < nLength; nIndex++)
            {
                vOutput.push_back(vOutput[vOutput.size() - nDistance]);
            }
        }
        return vOutput.size() == nSize;
    }

    bool Unpack(const std::vector<uint8_t>& vBundle, std::vector<uint8_t>& vFont)
    {
        vFont.clear();
        Reader stHeader { vBundle };
        if (stHeader.Read(8) != 0x556E697479465300ull)
        {
            return false;
        }
        const auto nVersion = stHeader.Read(4);
        if (nVersion < 7 || nVersion > 8)
        {
            return false;
        }
        stHeader.SkipString();
        stHeader.SkipString();
        if (stHeader.Read(8) != vBundle.size())
        {
            return false;
        }
        const size_t nCompressed = stHeader.Read(4);
        const size_t nSize = stHeader.Read(4);
        const auto nFlags = stHeader.Read(4);
        stHeader.Align();
        if (!stHeader.bValid || nCompressed > vBundle.size())
        {
            return false;
        }
        const size_t nInfoOffset = (nFlags & 0x80) ? vBundle.size() - nCompressed : stHeader.nPosition;
        std::vector<uint8_t> vInfo;
        if (!Decode(vBundle, nInfoOffset, nCompressed, nSize, nFlags & 0x3F, vInfo))
        {
            return false;
        }
        if (!(nFlags & 0x80))
        {
            stHeader.nPosition += nCompressed;
        }
        if (nFlags & 0x200)
        {
            stHeader.Align();
        }
        Reader stInfo { vInfo, 16 };
        const size_t nBlocks = stInfo.Read(4);
        if (!stInfo.bValid || nBlocks > vInfo.size() / 10)
        {
            return false;
        }
        std::vector<uint8_t> vData;
        std::vector<uint8_t> vBlock;
        for (size_t nIndex = 0; nIndex < nBlocks; nIndex++)
        {
            const size_t nBlockSize = stInfo.Read(4);
            const size_t nBlockCompressed = stInfo.Read(4);
            const auto nBlockFlags = stInfo.Read(2);
            if (!stInfo.bValid || nBlockSize > nMaximumBytes - vData.size()
                || !Decode(vBundle, stHeader.nPosition, nBlockCompressed, nBlockSize, nBlockFlags & 0x3F, vBlock))
            {
                return false;
            }
            stHeader.nPosition += nBlockCompressed;
            vData.insert(vData.end(), vBlock.begin(), vBlock.end());
        }

        for (size_t nStart = 0; nStart + 12 <= vData.size(); nStart++)
        {
            Reader stFont { vData, nStart };
            if (stFont.Read(4) != 0x4F54544Fu)
            {
                continue;
            }
            const size_t nTables = stFont.Read(2);
            stFont.Read(6);
            size_t nLength = 12 + nTables * 16;
            unsigned int nRequired = 0;
            if (nTables == 0 || nTables > 128 || nLength > vData.size() - nStart)
            {
                continue;
            }
            for (size_t nIndex = 0; nIndex < nTables; nIndex++)
            {
                const auto nTag = stFont.Read(4);
                stFont.Read(4);
                const size_t nOffset = stFont.Read(4);
                const size_t nTableSize = stFont.Read(4);
                if (nOffset > vData.size() - nStart || nTableSize > vData.size() - nStart - nOffset)
                {
                    stFont.bValid = false;
                    break;
                }
                nLength = (std::max)(nLength, nOffset + nTableSize);
                nRequired |= nTag == 0x636D6170 ? 1 : nTag == 0x68656164 ? 2 : nTag == 0x43464620 ? 4 : 0;
            }
            if (stFont.bValid && nRequired == 7)
            {
                vFont.assign(vData.begin() + nStart, vData.begin() + nStart + nLength);
                return true;
            }
        }
        return false;
    }
}
