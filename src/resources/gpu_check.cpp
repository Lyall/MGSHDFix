#include "gpu_check.hpp"

#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <algorithm>
#include <regex>

#ifndef MINIMUM_GPU_NAME
#define MINIMUM_GPU_NAME "NVIDIA GeForce GTX 970"
#endif

namespace
{
    struct GpuEntry
    {
        std::string model;
        int tier;
    };

    // GPU tier table without suffixes
    std::vector<GpuEntry> gpuTable = {
        {"GTX 750 TI", 40}, {"GTX 950", 55}, {"GTX 960", 65}, {"GTX 970", 100},
        {"GTX 980", 120}, {"GTX 980 TI", 140}, {"GTX 1050", 60}, {"GTX 1050 TI", 70},
        {"GTX 1060 3GB", 110}, {"GTX 1060 6GB", 120}, {"GTX 1070", 150}, {"GTX 1080", 170},
        {"GTX 1650", 85}, {"GTX 1650 SUPER", 95}, {"GTX 1660", 125}, {"GTX 1660 TI", 135},
        {"RTX 2060", 180}, {"RTX 2070", 200}, {"RTX 2080", 220}, {"RTX 3060", 200},
        {"RTX 3070", 260}, {"RTX 3080", 300}, {"RTX 3090", 350}, {"RTX 4060", 220},
        {"RTX 4070 TI", 310}, {"RTX 4070", 280}, {"RTX 4080", 350}, {"RTX 4090", 400},
        {"R9 270X", 55}, {"R9 280", 70}, {"R9 290", 100}, {"R9 290X", 110},
        {"RX 460", 50}, {"RX 470", 95}, {"RX 480", 100}, {"RX 570", 100},
        {"RX 580", 110}, {"RX 590", 115}, {"RX 5500 XT", 120}, {"RX 5600 XT", 140},
        {"RX 5700", 160}, {"RX 6600", 170}, {"RX 6700 XT", 220}, {"RX 6800", 270},
        {"RX 6900 XT", 310}, {"RX 7600", 190}, {"RX 7700 XT", 230}, {"RX 7800 XT", 270},
        {"RX 7900 XT", 340}, {"RX 7900 XTX", 360},
    };

    // Sort GPU table descending by model length for potential substring matching (not strictly necessary here)
    struct TableSorter
    {
        TableSorter()
        {
            std::sort(gpuTable.begin(), gpuTable.end(),
                [](const GpuEntry& a, const GpuEntry& b)
                {
                    return a.model.size() > b.model.size();
                });
        }
    } tableSorter;

    // Convert string to uppercase for case-insensitive matching
    std::string ToUpper(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c)
            {
                return std::toupper(c);
            });
        return str;
    }

    // Extract vendor from GPU name
    std::string GetVendor(const std::string& name)
    {
        if (name.find("NVIDIA") != std::string::npos) return "NVIDIA";
        if (name.find("AMD") != std::string::npos || name.find("RADEON") != std::string::npos) return "AMD";
        if (name.find("INTEL") != std::string::npos) return "INTEL";
        return "UNKNOWN";
    }

    // Estimate NVIDIA GPU tier based on model number
    int EstimateNVidia(int model)
    {
        if (model >= 1650 && model < 1660) return 85;
        if (model >= 1050 && model < 1060) return 70;
        if (model >= 4090) return 400;
        if (model >= 4080) return 350;
        if (model >= 4070) return 280;
        if (model >= 4060) return 220;
        if (model >= 3090) return 350;
        if (model >= 3080) return 300;
        if (model >= 3070) return 260;
        if (model >= 3060) return 200;
        if (model >= 2080) return 230;
        if (model >= 2070) return 200;
        if (model >= 2060) return 180;
        if (model >= 1660) return 125;
        if (model >= 1080) return 170;
        if (model >= 1070) return 150;
        if (model >= 1060) return 120;
        if (model >= 970)  return 100;
        if (model >= 960)  return 65;
        if (model >= 750)  return 40;
        return 20;
    }

    // Estimate AMD GPU tier based on model number
    int EstimateAMD(int model)
    {
        if (model >= 7900) return 350;
        if (model >= 7800) return 270;
        if (model >= 7700) return 230;
        if (model >= 7600) return 190;
        if (model >= 6900) return 310;
        if (model >= 6800) return 270;
        if (model >= 6700) return 220;
        if (model >= 6600) return 170;
        if (model >= 580)  return 110;
        if (model >= 570)  return 100;
        if (model >= 470)  return 95;
        return 20;
    }

    // Parse GPU string and estimate tier including suffix multipliers
    int ParseAndEstimate(const std::string& name)
    {
        // Regex extracts: prefix (GTX/RTX/RX/etc), model number, optional suffix (TI/SUPER/ULTRA)
        std::regex re(R"((GTX|RTX|RX|ARC|HD|IRIS)\s*([0-9]{3,4})(?:\s*(TI|SUPER|ULTRA))?)");
        std::smatch match;
        if (std::regex_search(name, match, re))
        {
            std::string prefix = match[1];
            int modelNum = std::stoi(match[2]);
            std::string suffix = (match.size() >= 4 && match[3].matched) ? match[3].str() : "";

            int baseTier = 0;
            if (prefix == "GTX" || prefix == "RTX")
            {
                baseTier = EstimateNVidia(modelNum);
            }
            else if (prefix == "RX")
            {
                baseTier = EstimateAMD(modelNum);
            }
            else if (prefix == "ARC")
            {
                baseTier = (modelNum >= 770) ? 160 : (modelNum >= 750 ? 140 : 85);
            }
            else if (prefix == "IRIS")
            {
                baseTier = 35;
            }
            else if (prefix == "HD")
            {
                baseTier = 10;
            }

            if (!suffix.empty())
            {
                if (suffix == "TI")
                {
                    baseTier = static_cast<int>(baseTier * 1.1);
                }
                else if (suffix == "SUPER")
                {
                    baseTier = static_cast<int>(baseTier * 1.15);
                }
                else if (suffix == "ULTRA")
                {
                    baseTier = static_cast<int>(baseTier * 1.2);
                }
            }

            return baseTier;
        }

        return 0;
    }

    // Get the tier for the GPU name:
    // 1) Try exact match in gpuTable
    // 2) If no exact match, parse and estimate tier with suffix
    int GetTier(const std::string& upperName)
    {
        for (const auto& entry : gpuTable)
        {
            if (upperName == entry.model)  // Exact match only!
            {
                return entry.tier;
            }
        }
        return ParseAndEstimate(upperName);
    }

    // Compute minimum tier once, at startup, from MINIMUM_GPU_NAME
    const int kMinimumTier = []
        {
            std::string minUpper = ToUpper(MINIMUM_GPU_NAME);
            int tier = GetTier(minUpper);
            if (tier == 0) tier = ParseAndEstimate(minUpper);
            if (tier == 0)
            {
                spdlog::warn("Minimum GPU \"{}\" could not be recognized - defaulting to tier 100.", MINIMUM_GPU_NAME);
                return 100;
            }
            return tier;
        }();

} // namespace

void CheckMinimumGPU(const std::string& gpuName)
{
    std::string upper = ToUpper(gpuName);
    std::string vendor = GetVendor(upper);

    int tier = GetTier(upper);

    if (tier == 0)
    {
        spdlog::warn("System Details - GPU: {} ({}) was not recognized.", gpuName, vendor);
        spdlog::warn("System Details - GPU: The game requires a minimum of a {} or equivalent.", MINIMUM_GPU_NAME);
        spdlog::warn("System Details - GPU: Degraded performance (ie \"Snake moving in slow motion\") and crashing likely to occur.");
        return;
    }

    if (tier < kMinimumTier)
    {
        spdlog::info("System Details - GPU: {}", gpuName);
        spdlog::warn("System Details - GPU: This GPU is below the minimum system requirements of a {} or equivalent.", MINIMUM_GPU_NAME);
        int percent = tier * 100 / kMinimumTier;
        spdlog::warn("System Details - GPU: Estimated performance compared to a {}: {}%", MINIMUM_GPU_NAME, percent);
        spdlog::warn("System Details - GPU: Degraded performance (ie \"Snake moving in slow motion\") and crashing likely to occur.");
    }
    else
    {
        spdlog::info("System Details - GPU: {}", gpuName);
    }
}
