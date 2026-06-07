#include "stdafx.h"
#include <filesystem>
#include <string>

#include "expand_bp_assets.hpp"

#include "common.hpp"
#include "logging.hpp"

typedef long long (__fastcall* FASTCALL_1IN1OUT)(void*);
typedef void* (__fastcall* FASTCALL_3IN1OUT)(void*, void*, size_t);

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
	FASTCALL_3IN1OUT BPReadFile;
	FASTCALL_1IN1OUT BPCloseFile;
	FASTCALL_1IN1OUT MGSMalloc;
	FASTCALL_1IN1OUT MGSFree;
}

// Helper for optimized file buffer allocation
static inline size_t RoundUp(size_t size) {
	int ret = 1;
	while (ret < size)
		ret <<= 1;
	return ret;
}

// realloc() implementation using the game's allocation lists (coooould replace with a pattern match I guess)
static void* MGSRealloc(void* old, size_t size, size_t old_size) {
	void* ret = (void*)MGSMalloc((void*)size);
	memcpy(ret, old, old_size);
	MGSFree(old);
	return ret;
}

static inline void* LoadSimilarFiles(BPLoadFileState* state, bool isBPAssets) {
#define match_substr (isBPAssets ? "bp_assets_" : "manifest_")

	// Find files similar to the currently used path.
	const char* filePath = isBPAssets ? state->bpAssetsPath : state->manifestPath;
	std::filesystem::path directory = std::filesystem::path(filePath).parent_path();
	char** buffer = isBPAssets ? &state->bpAssetsBuffer : &state->manifestBuffer;

	// Avoid realloc when reasonable
	size_t prevBufferSize = state->currentFileSize + 1;

	for (auto const& dir_entry : std::filesystem::directory_iterator(directory)) {
		if (!dir_entry.is_regular_file()) {
			continue;
		}

		std::filesystem::path entry_path = dir_entry.path();
		if (entry_path.stem().string().starts_with(match_substr)
			&& entry_path.extension() == ".txt") {
			// Match found, load it
			// Need to update: buffer, file size, file handle
			BPCloseFile(state->currentFileHandle);
			state->currentFileHandle = (void*)BPOpenFile((void*)entry_path.string().c_str());
			size_t newFileSize = BPGetFileSize(state->currentFileHandle);
			size_t newBufferSize = RoundUp(state->currentFileSize + newFileSize + 1);
			if (newBufferSize > prevBufferSize) {
				*buffer = (char*)MGSRealloc(*buffer, newBufferSize, prevBufferSize);
				(*buffer)[state->currentFileSize + newFileSize] = '\0';
			}
			prevBufferSize = newBufferSize;
			// Oh, and read the new file, of course.
			state->currentFileHandle = BPReadFile(state->currentFileHandle, &(*buffer)[state->currentFileSize], newFileSize);
			state->currentFileSize += newFileSize;
		}
	}

	return state->currentFileHandle;
}

void BP_FilesysChanges::Initialize() {

	if (!(eGameType & MGS2)) // TODO: MGS3 support, likely requires different patterns
	{
		return;
	}

	// The HD Collection (hence, the Master Collection) have a different file system to the original games.
	// Files are stored with their proper names, but loaded into a cache with their strcode names as needed.
	// This cache is defined by manifest.txt and bp_assets.txt files for each stage and each codec character.
	// To increase compatibility, we hook the cache loader and allow loading multiple such manifests.
	// Specifically, the part of the loader that initializes a buffer and reads the file into memory.
	//uint8_t* ManifestLoader = Memory::PatternScan(baseModule, "40 53 48 83 EC ?? 48 8B D9 48 8B 89 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 4B", "manifest.txt loader");
	uint8_t* BPAssetsLoader = Memory::PatternScan(baseModule, "40 53 55 56 57 41 54 41 56 41 57 48 81 EC 70 03 00 00", "bp_assets.txt loader");
	if (!BPAssetsLoader) {
		spdlog::error("Failed to match BP_LoadFlatFSSync.");
		return;
	}

	BPCloseFile = (FASTCALL_1IN1OUT)Memory::GetRelativeOffset(BPAssetsLoader + 0x67);
	BPOpenFile = (FASTCALL_1IN1OUT)Memory::GetRelativeOffset(BPAssetsLoader + 0x91);
	BPGetFileSize = (FASTCALL_1IN1OUT)Memory::GetRelativeOffset(BPAssetsLoader + 0xa6);
	//auto wut = (long long)Memory::GetRelativeOffset(BPAssetsLoader + 0xe3);
	//spdlog::info("{}", wut);
	//MGSMalloc = (FASTCALL_1IN1OUT)wut;
	//spdlog::info("not dead?");
	MGSMalloc = *(FASTCALL_1IN1OUT*)Memory::GetRelativeOffset(BPAssetsLoader + 0xe3);
	BPReadFile = (FASTCALL_3IN1OUT)Memory::GetRelativeOffset(BPAssetsLoader + 0x10f);
	MGSFree = (FASTCALL_1IN1OUT)Memory::GetRelativeOffset(BPAssetsLoader + 0x1e1);


	// Both these injections are immediately after the file is read in; we can close the handle and open new ones as needed.
	// system/libfs/loader_flatfs.cpp -> BP_BeginLoadFlatFS()
	{
		MAKE_HOOK_MID(baseModule, "48 89 83 50 05 00 00 48 83 C4 20 5B C3", "manifest.txt Union", {
			// rax = file handle
			// rbx = load state struct
			ctx.rax = (uintptr_t)LoadSimilarFiles((BPLoadFileState*)ctx.rbx, false);
		});
	}
	// system/libfs/loader_flatfs.cpp -> BP_LoadFlatFSSync()
	{
		MAKE_HOOK_MID(baseModule, "48 89 87 50 05 00 00 C7 07 03 00 00 00", "bp_assets.txt Union", {
			// rax = file handle
			// rdi = load state struct
			ctx.rax = (uintptr_t)LoadSimilarFiles((BPLoadFileState*)ctx.rdi, true);
		});
	}
}
