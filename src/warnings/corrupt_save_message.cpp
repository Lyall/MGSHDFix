#include "common.hpp"
#include "corrupt_save_message.hpp"

#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

#include "logging.hpp"

void DamagedSaveFix::Initialize()
{
    if (!(eGameType & (MG|MGS2|MGS3)))
    {
        return;
    }

    bool bWarnedOnce = false;
    std::filesystem::path gamePath = sGameSavePath;
    spdlog::info("Checking for damaged save files in: {}", gamePath.string());
    for (const auto& firstLevelEntry : std::filesystem::directory_iterator(gamePath))
    {
        if (!firstLevelEntry.is_directory())
        {
            continue;
        }
        for (const auto& secondLevelEntry : std::filesystem::directory_iterator(firstLevelEntry))
        {
            auto filename = secondLevelEntry.path().filename();
            std::string filenameLower = filename.string();
            std::transform(filenameLower.begin(), filenameLower.end(), filenameLower.begin(), ::tolower);
            if (!secondLevelEntry.is_directory() || filenameLower == "launcher" || filenameLower == "outdated saves")
            {
                continue;
            }

            std::string dirName = filename.string();
            int fileCount = 0;
            for (const auto& fileEntry : std::filesystem::directory_iterator(secondLevelEntry))
            {
                if (fileEntry.is_regular_file())
                {
                    ++fileCount;
                    if (fileCount > 2)
                    {
                        break;
                    }
                }
            }
            if (fileCount <= 2)
            {
                continue;
            }

            if (!bWarnedOnce)
            {
                spdlog::error("Damaged save file detected.");
                spdlog::error("This issue is typically caused by closing the game too quickly after saving, resulting in Steam Cloud syncing old data improperly.");
                Logging::ShowConsole();
                std::cout << "MGSHDFix Bugfix: Damaged save file detected.\nThis issue is typically caused by closing the game too quickly after saving, resulting in Steam Cloud syncing old data improperly." << std::endl;
                bWarnedOnce = true;
            }
            std::cout << "Outdated save data detected in folder: " << "\n" << secondLevelEntry.path().string() << "\n\nFound Saves:" << std::endl;
            spdlog::error("Outdated save data detected in folder:");
            spdlog::error("{}", secondLevelEntry.path().string());
            spdlog::error("Found Saves:");
            std::vector<std::filesystem::directory_entry> candidateFiles;
            for (const auto& fileEntry : std::filesystem::directory_iterator(secondLevelEntry))
            {
                if (!fileEntry.is_regular_file())
                {
                    continue;
                }
                std::string fileName = fileEntry.path().filename().string();
                if (fileName == dirName)
                {
                    continue;
                }
                // Get last write time (portable conversion)
                auto ftime = std::filesystem::last_write_time(fileEntry);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - std::filesystem::file_time_type::clock::now()
                    + std::chrono::system_clock::now()
                );
                std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
                std::tm tm_buf;
                #ifdef _WIN32
                localtime_s(&tm_buf, &cftime);
                #else
                localtime_r(&cftime, &tm_buf);
                #endif
                std::ostringstream timeStream;
                timeStream << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
                std::cout << "  " << fileName << " (Date Created: " << timeStream.str() << ")" << std::endl;
                spdlog::error("  {} (Date Created: {})", fileName, timeStream.str());
                candidateFiles.push_back(fileEntry);
            }

            // Create "Outdated Saves" directory one level up from secondLevelEntry
            auto parentPath = secondLevelEntry.path().parent_path();
            auto outdatedSavesPath = parentPath / "Outdated Saves";
            std::error_code ec;
            std::filesystem::create_directories(outdatedSavesPath, ec);
            if (ec)
            {
                std::cout << "Failed to create 'Outdated Saves' directory: " << ec.message() << std::endl;
                spdlog::error("Failed to create 'Outdated Saves' directory: {}", ec.message());
                continue;
            }

            // Get current date as YYYY-MM-DD
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm tm_buf;
            #ifdef _WIN32
            localtime_s(&tm_buf, &now_c);
            #else
            localtime_r(&now_c, &tm_buf);
            #endif
            std::ostringstream dateStream;
            dateStream << std::put_time(&tm_buf, "%Y-%m-%d");
            std::string datedDirName = dirName + "_" + dateStream.str();

            // Find a unique datedOutdatedSavePath (append -2, -3, etc. if needed)
            auto datedOutdatedSavePath = outdatedSavesPath / datedDirName;
            int suffix = 2;
            while (std::filesystem::exists(datedOutdatedSavePath))
            {
                datedOutdatedSavePath = outdatedSavesPath / (datedDirName + "-" + std::to_string(suffix));
                ++suffix;
            }
            std::filesystem::create_directories(datedOutdatedSavePath, ec);
            if (ec)
            {
                std::cout << "Failed to create dated outdated save directory: " << ec.message() << std::endl;
                spdlog::error("Failed to create dated outdated save directory: {}", ec.message());
                continue;
            }

            // Move the oldest file (by last_write_time) to the new folder
            if (candidateFiles.empty())
            {
                continue;
            }
            auto oldest = std::min_element(candidateFiles.begin(), candidateFiles.end(),
                [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b)
                {
                    return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
                });
            if (oldest == candidateFiles.end())
            {
                continue;
            }
            std::filesystem::path targetPath = datedOutdatedSavePath / oldest->path().filename();
            std::error_code moveEc;
            std::filesystem::rename(oldest->path(), targetPath, moveEc);
            if (moveEc)
            {
                std::cout << "Failed to move save from " << oldest->path().parent_path().filename().string() << " to " << targetPath.parent_path().string() << ": " << moveEc.message() << std::endl;
                spdlog::error("Failed to move save from {} to {}: {}", oldest->path().parent_path().filename().string(), targetPath.parent_path().string(), moveEc.message());
                continue;
            }
            std::cout << "Moved oldest save from '" << oldest->path().parent_path().filename().string() << "' to '" << targetPath.parent_path().string() << "'" << std::endl;
            spdlog::error("Moved oldest save from '{}' to '{}'", oldest->path().parent_path().filename().string(), targetPath.parent_path().string());
        }
    }
    if (!bWarnedOnce)
    {
        spdlog::info("Damaged save file check completed.");
    }
}
                