#include "common.hpp"
#include "texture_buffer_size.hpp"
#include "logging.hpp"

void TextureBufferSize::Initialize() const
{
    if (iTextureBufferSizeMB > 128 && (eGameType & (MG | MGS3)))
    {
        // MG/MG2 | MGS3: texture buffer size extension
        const uint32_t NewSize = iTextureBufferSizeMB * 1024 * 1024;

        // Scan for the 9 mallocs which set buffer inside CTextureBuffer::sInstance
        bool failure = false;
        for (int i = 0; i < 9; i++)
        {
            if (uint8_t* MGS3_CTextureBufferMallocResult = Memory::PatternScanSilent(baseModule, "75 ?? B9 00 00 00 08 FF"))
            {
                uint32_t* bufferAmount = (uint32_t*)(MGS3_CTextureBufferMallocResult + 3);
                spdlog::info("MG/MG2 | MGS 3: Texture Buffer Size: #{} ({:s}+{:X}) old buffer size: {}", i, sExeName.c_str(), reinterpret_cast<uintptr_t>(MGS3_CTextureBufferMallocResult) - reinterpret_cast<uintptr_t>(baseModule), static_cast<uintptr_t>(*bufferAmount));
                Memory::Write((uintptr_t)bufferAmount, NewSize);
                spdlog::info("MG/MG2 | MGS 3: Texture Buffer Size: #{} ({:s}+{:X}) new buffer size: {}", i, sExeName.c_str(), reinterpret_cast<uintptr_t>(MGS3_CTextureBufferMallocResult) - reinterpret_cast<uintptr_t>(baseModule), static_cast<uintptr_t>(*bufferAmount));
            }
            else
            {
                spdlog::error("MG/MG2 | MGS 3: Texture Buffer Size: #{}: Pattern scan failed.", i);
                failure = true;
                break;
            }
        }

        if (!failure)
        {
            // CBaseTexture::Create seems to contain code that mallocs buffers based on 16MiB shifted by index of the mip being loaded
            // (ie: size = 16MiB >> mipIndex)
            // We'll make sure to increase the base 16MiB size it uses too
            if (uint8_t* MGS3_CBaseTextureMallocScanResult = Memory::PatternScanSilent(baseModule, "75 ?? 00 00 00 08 8B ??"))
            {
                uint32_t* bufferAmount = reinterpret_cast<uint32_t*>(MGS3_CBaseTextureMallocScanResult + 3);
                spdlog::info("MG/MG2 | MGS 3: Texture Buffer Size: #{} ({:s}+{:X}) old buffer size: {}", 9, sExeName.c_str(), reinterpret_cast<uintptr_t>(MGS3_CBaseTextureMallocScanResult) - reinterpret_cast<uintptr_t>(baseModule), static_cast<uintptr_t>(*bufferAmount));
                Memory::Write(reinterpret_cast<uintptr_t>(bufferAmount), NewSize);
                spdlog::info("MG/MG2 | MGS 3: Texture Buffer Size: #{} ({:s}+{:X}) new buffer size: {}", 9, sExeName.c_str(), reinterpret_cast<uintptr_t>(MGS3_CBaseTextureMallocScanResult) - reinterpret_cast<uintptr_t>(baseModule), static_cast<uintptr_t>(*bufferAmount));
            }
            else
            {
                spdlog::error("MG/MG2 | MGS 3: Texture Buffer Size: #{}: Pattern scan failed.", 9);
            }
        }
    }
}

