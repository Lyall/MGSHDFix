#pragma once

#include <string>
#include <vector>

namespace BP_FilesysChanges
{
	void Initialize();

	// Register lines merged into EVERY gameplay stage's manifest.txt / bp_assets.txt.
	// %S% expands to the stage name, %R% to the region (eu/us/jp). Resident (r_*) stages
	// and codec caches are skipped. Call before or after Initialize; applies on stage load.
	void AddUniversalStageLines(std::vector<std::string> manifestLines,
	                            std::vector<std::string> bpAssetsLines);

	inline bool bLoaded = false;
}
