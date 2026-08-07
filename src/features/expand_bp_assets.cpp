#include "stdafx.h"
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>

#include "expand_bp_assets.hpp"

#include "common.hpp"
#include "logging.hpp"

typedef long long (__fastcall* FASTCALL_1IN1OUT)(void*);
typedef void* (__fastcall* FASTCALL_2IN1OUT)(void*, size_t);
typedef void* (__fastcall* FASTCALL_3IN1OUT)(void*, void*, size_t);

#define debug(...) do { if (g_Logging.bVerboseLogging) { spdlog::info("BPFilesysChanges: " __VA_ARGS__); } } while (0)

// core components of struct; original definition in system/libfs/loader_flatfs.h
typedef struct {
	unsigned int loadState;
	char manifestPath[260];
	char* manifestBuffer;
	char field_110[8]; // irrelevant
	char bpAssetsPath[260];
	char field_21C[4]; // padding
	char* bpAssetsBuffer;
	char field_228[16 + 260 * 3 + 4]; // irrelevant
	// begin sub-struct "pManifestFileState"/"pBPAssetsFileState"
	void* currentFileHandle;
	void* operatingFileHandle; // duplicate?
	char* currentFileBuffer; // only used for individual cmdl/ctxr files
	size_t currentFileSize;
} BPLoadFileState;

namespace {
	FASTCALL_1IN1OUT BPOpenFile;
	FASTCALL_1IN1OUT BPGetFileSize;
	//FASTCALL_3IN1OUT BPReadFile;
	FASTCALL_1IN1OUT BPCloseFile;
	//FASTCALL_1IN1OUT BPIsReadDone;
	FASTCALL_1IN1OUT* MGSMalloc;
	//FASTCALL_1IN1OUT* MGSFree;
}

// Helper for optimized file buffer allocation
static inline size_t RoundUp(size_t size) {
	int ret = 1;
	while (ret < size)
		ret <<= 1;
	return ret;
}

static inline void* MGSRealloc(void* oldBuf, size_t newSize) {
	//void* newBuf = (void*)(*MGSMalloc)((void*)newSize);
	//memcpy(newBuf, oldBuf, oldSize);
	//(*MGSFree)(oldBuf);
	//return newBuf;
	// 
	// Get game realloc function relatively from the table that holds malloc.
	return ((FASTCALL_2IN1OUT)(MGSMalloc[2]))(oldBuf, newSize);
}

// Key on the cache fields, not the source path. One texture can be registered against
// several tri groups, and those lines differ only in the last field.
static inline std::string_view CacheKey(const std::string& line) {
	const size_t comma = line.find(',');
	return comma == std::string::npos ? std::string_view(line) : std::string_view(line).substr(comma + 1);
}

static inline std::string_view GetExtension(const std::string& line) {
	const size_t period = line.find_last_of('.');
	if (period == std::string::npos) {
		return std::string_view(line);
	}
	else {
		return std::string_view(line).substr(period + 1);
	}
}

static void SplitLines(const char* data, size_t size, std::vector<std::string>& out) {
	size_t start = 0;
	for (size_t i = 0; i <= size; i++) {
		if (i != size && data[i] != '\n') {
			continue;
		}
		size_t end = i;
		while (end > start && (data[end - 1] == '\r' || data[end - 1] == '\n')) {
			end--;
		}
		if (end > start) {
			out.emplace_back(data + start, end - start);
		}
		start = i + 1;
	}
}

// Ultimate ASI Loader redirects opens into its overload folder, but these lists are found
// by scanning a directory rather than opened by name, so a mod's copies there are never
// seen. Ask the loader where that folder is and scan it too. No loader, no change.
typedef bool (WINAPI* GetOverloadPathWFn)(wchar_t*, size_t);

static const std::filesystem::path& OverloadRoot() {
	static std::filesystem::path root;
	static bool resolved = false;
	if (resolved) {
		return root;
	}
	resolved = true;

	HMODULE mods[256];
	DWORD needed = 0;
	if (K32EnumProcessModules(GetCurrentProcess(), mods, sizeof(mods), &needed)) {
		const size_t count = std::min<size_t>(needed / sizeof(HMODULE), std::size(mods));
		for (size_t i = 0; i < count; i++) {
			auto fn = (GetOverloadPathWFn)GetProcAddress(mods[i], "GetOverloadPathW");
			if (!fn) {
				continue;
			}
			wchar_t buf[MAX_PATH] {};
			if (fn(buf, std::size(buf)) && buf[0]) {
				root = buf;
				spdlog::info("BP_FilesysChanges: loader overload folder is {}", root.string());
			}
			break;
		}
	}
	return root;
}

// The same stage folder inside the overload root, or empty when there is no loader.
static std::filesystem::path OverloadMirror(const std::filesystem::path& directory) {
	const std::filesystem::path& root = OverloadRoot();
	if (root.empty()) {
		return {};
	}
	std::error_code ec;
	std::filesystem::path rel = std::filesystem::relative(directory, sExePath.parent_path(), ec);
	if (ec || rel.empty() || *rel.begin() == "..") {
		return {};
	}
	return root / rel;
}

static inline void* LoadSimilarFiles(BPLoadFileState* state, bool isBPAssets) {
#define match_substr (isBPAssets ? "bp_assets_" : "manifest_")
	debug("loading a {}", isBPAssets ? "bp_assets" : "manifest");

	// Find files similar to the currently used path.
	const char* filePath = isBPAssets ? state->bpAssetsPath : state->manifestPath;
	std::filesystem::path directory = std::filesystem::path(filePath).parent_path();
	char** buffer = isBPAssets ? &state->bpAssetsBuffer : &state->manifestBuffer;

	// Avoid realloc when reasonable
	size_t prevBufferSize = state->currentFileSize + 1;
	// For after the loop
	size_t originalFileSize = state->currentFileSize;

	std::vector<std::filesystem::path> entries;
	std::error_code ec;
	for (auto const& d : { directory, OverloadMirror(directory) }) {
		if (d.empty() || !std::filesystem::is_directory(d, ec)) {
			continue;
		}
		for (auto const& dir_entry : std::filesystem::directory_iterator(d, ec)) {
			if (dir_entry.is_regular_file(ec)) {
				entries.push_back(dir_entry.path());
			}
		}
	}

	for (auto const& entry_path : entries) {
		if (entry_path.stem().string().starts_with(match_substr)
			&& entry_path.extension() == ".txt") {
			// Match found, load it
			// Need to update: buffer, file size, NOT file handle
			//BPCloseFile(state->currentFileHandle);
			//state->currentFileHandle = (void*)BPOpenFile((void*)entry_path.string().c_str());
			// The Master Collection file reader is asynchronous. We need more consistency than that.
			FILE* fp = fopen(entry_path.string().c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			size_t newFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			size_t newBufferSize = RoundUp(state->currentFileSize + newFileSize + 1);
			if (newBufferSize > prevBufferSize) {
				debug("reallocing");
				char* oldBuf = *buffer;
				*buffer = (char*)MGSRealloc(oldBuf, newBufferSize);
				debug("buffer for {} was at {}, now {}", filePath, (void*)oldBuf, (void*)*buffer);
				(*buffer)[state->currentFileSize + newFileSize] = '\0';
			}
			prevBufferSize = newBufferSize;
			// And read the new file, of course.
			fread(&(*buffer)[state->currentFileSize], newFileSize, 1, fp);
			fclose(fp);
			if ((*buffer)[state->currentFileSize] == '\0')
			{
				// ??? mission failed? what?
				spdlog::warn("Failed to load a text file ({} supplementing {}), the simplest file load possible?", entry_path.string(), filePath);
				continue;
			}
			state->currentFileSize += newFileSize;
		}
	}
	
	// Merge instead of prepending. A cache path the stage already lists replaces it in
	// place, since loading an asset twice starves the streamer and runs the stage in slow
	// motion. New entries go on the end: the list is in dependency order.
	size_t newFilesSize = state->currentFileSize - originalFileSize;
	if (newFilesSize) {
		std::vector<std::string> lines;
		SplitLines(*buffer, state->currentFileSize, lines);

		// Sort lines by asset type and remove duplicates (later entry prioritized)
		typedef std::pair<std::string, std::vector<std::string>> LineGroup;
		std::vector<LineGroup> sortedLines = {
			{"tri", {}},
			{"hzx", {}},
			{"var", {}},
			{"sar", {}},
			{"row", {}},
			{"mar", {}},
			{"lt2", {}},
			{"kms", {}},
			{"evm", {}},
			{"cv2", {}},
			{"gcx", {}},
			{"ctxr", {}},
			{"cmdl", {}},
			{"other", {}} // Extension-match iterator defaults to this
		};

		size_t replaced = 0, added = 0;
		for (const std::string& line : lines) {
			// Get appropriate vector 
			const std::string_view extension = GetExtension(line);
			// std::prev causes fallback case to be "other" rather than sortedLines.end()
			auto matchGroup = std::find_if(sortedLines.begin(), std::prev(sortedLines.end()),
				[&](const LineGroup& v) { return v.first == extension; });
			std::vector<std::string>& lineVec = (*matchGroup).second;

			// Insert into vector, with duplicate cleanup
			const std::string_view key = CacheKey(line);
			auto match = std::find_if(lineVec.begin(), lineVec.end(),
				[&](const std::string& v) { return CacheKey(v) == key; });
			if (match != lineVec.end()) {
				*match = line;
				replaced++;
			}
			else {
				added++;
				lineVec.push_back(line);
			}
		}

		std::string merged;
		merged.reserve(state->currentFileSize + lines.size() + 1);
		for (const LineGroup& group : sortedLines) {
			for (const std::string& line : group.second) {
				merged += line;
				// Unix newline is slightly more optimal as the game's parser discards CR.
				merged += "\n";
			}
		}

		const size_t needed = RoundUp(merged.size() + 1);
		if (needed > prevBufferSize) {
			spdlog::warn("BPFilesysChanges: Unexpected reallocation (should have had space already?)");
			*buffer = (char*)MGSRealloc(*buffer, needed);
		}
		memcpy(*buffer, merged.data(), merged.size());
		(*buffer)[merged.size()] = '\0';
		state->currentFileSize = merged.size();
		spdlog::info("{}: merged {} supplementary entries ({} replaced, {} added)",
			std::filesystem::path(filePath).filename().string(), replaced + added, replaced, added);
	}

	return state->currentFileHandle;
}

void BP_FilesysChanges::Initialize() {

	if (!(eGameType & MGS2)) // TODO: MGS3 support, likely requires different patterns
	{
		return;
	}

	if (Util::IsSteamOS()) // temporary. linux filesystems are stupid and shit reliant on expand_bp_assets can randomly cause crashing.
	{
        spdlog::warn("BP_FilesysChanges: Temporarily disabled on SteamOS due to crashing issues with Linux filesystems.");
        spdlog::warn("BP_FilesysChanges: Features reliant on expand_bp_assets will not be available.");
        spdlog::warn("BP_FilesysChanges: This includes: Snake Holster Fix, Hostage Arm Fix, Shell 1 Core Camera Screen fix, Alternative Colonel MSX Sprite.");
		return;
	}

	bLoaded = true;

	// The HD Collection (hence, the Master Collection) have a different file system to the original games.
	// Files are stored with their proper names, but loaded into a cache with their strcode names as needed.
	// This cache is defined by manifest.txt and bp_assets.txt files for each stage and each codec character.
	// To increase compatibility, we hook the cache loader and allow loading multiple such manifests.
	// Specifically, the part of the loader that initializes a buffer and reads the file into memory.
	//uint8_t* ManifestLoader = Memory::PatternScan(baseModule, "40 53 48 83 EC ?? 48 8B D9 48 8B 89 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 4B", "manifest.txt loader");
	uint8_t* BPAssetsLoader = Memory::PatternScan(baseModule, "40 53 55 56 57 41 54 41 56 41 57 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 84 24", "bp_assets.txt loader");
	if (!BPAssetsLoader) {
		spdlog::error("Failed to match BP_LoadFlatFSSync.");
		return;
	}

	//BPIsReadDone = (FASTCALL_1IN1OUT)Memory::GetRelativeOffset(BPAssetsLoader + 0x57);
	BPCloseFile = (FASTCALL_1IN1OUT)Memory::GetRelativeOffset(BPAssetsLoader + 0x67);
	BPOpenFile = (FASTCALL_1IN1OUT)Memory::GetRelativeOffset(BPAssetsLoader + 0x91);
	BPGetFileSize = (FASTCALL_1IN1OUT)Memory::GetRelativeOffset(BPAssetsLoader + 0xa6);
	//BPReadFile = (FASTCALL_3IN1OUT)Memory::GetRelativeOffset(BPAssetsLoader + 0x10f);
	MGSMalloc = (FASTCALL_1IN1OUT*)Memory::GetRelativeOffset(BPAssetsLoader + 0xe3);
	//MGSFree = (FASTCALL_1IN1OUT*)Memory::GetRelativeOffset(BPAssetsLoader + 0x1e2);


	// Both these injections are immediately before the file is closed; we realloc and add data to the buffer.
	// system/libfs/loader_flatfs.cpp -> BP_LoadFlatFSSync()
	{
		MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 8B 87 08 01 00 00 48 8D 8F 18 01 00 00", "manifest.txt Union", {
			// rdi = load state struct
			LoadSimilarFiles((BPLoadFileState*)ctx.rdi, false);
		});

		MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 8B 87 20 02 00 00 B9 10 57 00 00", "bp_assets.txt Union", {
			// rdi = load state struct
			LoadSimilarFiles((BPLoadFileState*)ctx.rdi, true);
		});
	}
}
