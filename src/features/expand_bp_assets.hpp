#pragma once

namespace BP_FilesysChanges
{
	void Initialize();

	inline bool bLoaded = false;
}


namespace BP_FileSys
{
    std::filesystem::path GetActiveAssetPath(const std::string& relativePath);

    // CTXR header, https://github.com/316austin316/CTXR-Converter/blob/main/ctxr_utils.py
    struct CTXRHeader
    {
        uint32_t version = 7;
        uint16_t width = 0;
        uint16_t height = 0;
        uint16_t depth = 1;
        uint32_t format = 0;
        bool hasAlpha = false;
        uint32_t additionalFlags = 0;
        uint32_t minRGBA = 0;
        uint32_t maxRGBA = 0xFFFFFFFF;
        int8_t filterHint = -1;
        uint8_t alphaRefValue = 0;
        int8_t maxLODOffset = 0;
        uint32_t type = 0;
        uint8_t numLevels = 1;
    };

    std::optional<CTXRHeader> ReadCTXRHeader(const std::filesystem::path& path);

    // Util::HashTexels() over mip level 0. Only supports uncompressed RGBA8; nullopt on read failure.
    std::optional<uint64_t> HashCTXRTexture(const std::filesystem::path& path);

	// Register lines merged into EVERY gameplay stage's manifest.txt / bp_assets.txt.
	// %S% expands to the stage name, %R% to the region (eu/us/jp). Resident (r_*) stages
	// and codec caches are skipped. Call before or after Initialize; applies on stage load.
	void AddUniversalStageLines(std::vector<std::string> manifestLines, std::vector<std::string> bpAssetsLines);


}

