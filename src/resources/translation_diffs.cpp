#include "stdafx.h"
#include "common.hpp"
#include "translation_diffs.hpp"
#include "expand_bp_assets.hpp"
#include "game_funcs.hpp"

namespace
{
    uint32_t Hash(const nlohmann::json& stValue)
    {
        std::string sHash = stValue.get<std::string>();
        std::string_view svHash = sHash;
        if (svHash.starts_with("0x") || svHash.starts_with("0X"))
        {
            svHash.remove_prefix(2);
        }
        uint32_t nHash = 0;
        const auto [pEnd, eError] = std::from_chars(svHash.data(), svHash.data() + svHash.size(), nHash, 16);
        if (svHash.empty() || svHash.size() > 8 || eError != std::errc() || pEnd != svHash.data() + svHash.size())
        {
            throw std::runtime_error("Invalid CRC32");
        }
        return nHash;
    }

    int Identity(const nlohmann::json& stValue)
    {
        if (!stValue.is_number_integer())
        {
            throw std::runtime_error("Invalid entry index");
        }
        const int64_t nValue = stValue.get<int64_t>();
        if (nValue < 0 || nValue > INT_MAX)
        {
            throw std::runtime_error("Entry index out of range");
        }
        return static_cast<int>(nValue);
    }

    std::string Text(const nlohmann::json& stValue)
    {
        std::string sText = stValue.get<std::string>();
        if (sText.find('\0') != std::string::npos)
        {
            throw std::runtime_error("Text contains a null character");
        }
        return sText;
    }

    void ReadCaptions(const nlohmann::json& stRoot, bool bOverrides, std::vector<TranslationDiffs::Entry>& vEntries)
    {
        const int nVersion = stRoot.at("version").get<int>();
        if (nVersion != 1 || stRoot.at("type") != (bOverrides ? "mgs2-string-overrides-dif" : "mgs2-stringdb-dif"))
        {
            throw std::runtime_error("Unsupported caption diff format");
        }
        auto ReadEntries = [&](const nlohmann::json& stChanges, bool bDisabled)
        {
            if (!stChanges.is_array())
            {
                throw std::runtime_error("Expected an array of changes");
            }
            for (const auto& stChange : stChanges)
            {
                TranslationDiffs::Entry stEntry;
                stEntry.nHash = Hash(stChange.at(bOverrides ? "hash" : "crc"));
                stEntry.bDisabled = bDisabled;
                stEntry.sLanguage = stChange.value("lang", "-1");
                if (stEntry.sLanguage != "-1" && (stEntry.sLanguage.size() != 1 || stEntry.sLanguage[0] < '0' || stEntry.sLanguage[0] > '7'))
                {
                    throw std::runtime_error("Invalid caption language");
                }
                if (bOverrides)
                {
                    stEntry.nTable = Identity(stChange.at("table"));
                    if (stEntry.nTable > 1)
                    {
                        throw std::runtime_error("Invalid caption table");
                    }
                }
                if (!bDisabled)
                {
                    stEntry.sText = Text(stChange.at("after"));
                }
                vEntries.push_back(std::move(stEntry));
            }
        };
        ReadEntries(stRoot.at("changes"), false);
        if (bOverrides)
        {
            ReadEntries(stRoot.at("disabled"), true);
        }
    }
}

std::vector<TranslationDiffs::Entry> TranslationDiffs::Parse(const nlohmann::json& stRoot, std::string_view svGame)
{
    std::vector<Entry> vEntries;
    if (stRoot.at("game").get<std::string>() != svGame)
    {
        return vEntries;
    }
    if (svGame == "MG1" || svGame == "MG2")
    {
        const int nVersion = stRoot.at("version").get<int>();
        if (stRoot.at("type") != "mg-block-dif" || nVersion != 1 || !stRoot.at("changes").is_array())
        {
            throw std::runtime_error("Unsupported MG diff format");
        }
        for (const auto& stChange : stRoot.at("changes"))
        {
            Entry stEntry;
            stEntry.nBlock = Identity(stChange.at("block"));
            stEntry.nIndex = Identity(stChange.at("index"));
            stEntry.sLanguage = stChange.at("lang").get<std::string>();
            constexpr std::string_view asvLanguages[] = { "en", "fr", "gr", "it", "sp", "jp" };
            if (std::ranges::find(asvLanguages, stEntry.sLanguage) == std::end(asvLanguages))
            {
                throw std::runtime_error("Unknown MG language");
            }
            Hash(stChange.at("beforeCrc32"));
            stEntry.sText = Text(stChange.at("after"));
            vEntries.push_back(std::move(stEntry));
        }
    }
    else
    {
        if (stRoot.at("type") == "mgs-unified-dif" && stRoot.at("version") == 1)
        {
            ReadCaptions(stRoot.at("strings"), false, vEntries);
            ReadCaptions(stRoot.at("overrides"), true, vEntries);
        }
        else
        {
            ReadCaptions(stRoot, stRoot.at("type") == "mgs2-string-overrides-dif", vEntries);
        }
    }
    return vEntries;
}

std::vector<TranslationDiffs::Entry> TranslationDiffs::Load(std::string_view svGame)
{
    std::vector<Entry> vEntries;
    std::vector<std::filesystem::path> vDirectories { sExePath / "translations" };
    if (!sFixPath.empty())
    {
        vDirectories.push_back(sExePath / sFixPath / "translations");
    }
    if (Shared_Gamefuncs::BP_GetAssetLoadFullPath)
    {
        const auto stActiveDirectory = BP_FileSys::GetActiveAssetPath("translations/");
        if (!stActiveDirectory.empty())
        {
            vDirectories.push_back(stActiveDirectory);
        }
    }

    std::map<std::wstring, std::filesystem::path> mFiles;
    auto ScanDirectory = [&](const std::filesystem::path& stDirectory)
    {
        std::error_code stError;
        if (!std::filesystem::is_directory(stDirectory, stError))
        {
            return;
        }
        for (std::filesystem::directory_iterator itFile(stDirectory, stError), itEnd; !stError && itFile != itEnd; itFile.increment(stError))
        {
            if (itFile->is_regular_file(stError))
            {
                auto sName = itFile->path().filename().wstring();
                std::ranges::transform(sName, sName.begin(), [](wchar_t nChar) { return static_cast<wchar_t>(std::towlower(nChar)); });
                if (sName.ends_with(L".dif.json") || sName == L"dif.json")
                {
                    mFiles[sName] = itFile->path();
                }
            }
        }
        if (stError)
        {
            spdlog::error("{}: Translation : Cannot read {}: {}", svGame, stDirectory.string(), stError.message());
        }
    };
    for (const auto& stDirectory : vDirectories)
    {
        ScanDirectory(stDirectory);
    }
    if (Shared_Gamefuncs::BP_GetAssetLoadFullPath)
    {
        for (auto& [sName, stFile] : mFiles)
        {
            const auto stActiveFile = BP_FileSys::GetActiveAssetPath((std::filesystem::path("translations") / stFile.filename()).generic_string());
            std::error_code stError;
            if (!stActiveFile.empty() && std::filesystem::is_regular_file(stActiveFile, stError))
            {
                stFile = stActiveFile;
            }
        }
    }

    auto stOverloadRoot = BP_FileSys::LoaderOverloadRoot();
    if (!stOverloadRoot.empty())
    {
        if (stOverloadRoot.is_relative())
        {
            stOverloadRoot = sExePath / stOverloadRoot;
        }
        for (const auto& stDirectory : vDirectories)
        {
            const auto stRelative = stDirectory.lexically_normal().lexically_relative(sExePath.lexically_normal());
            if (!stRelative.empty() && *stRelative.begin() != "..")
            {
                ScanDirectory(stOverloadRoot / stRelative);
            }
        }
    }
    for (const auto& [sName, stFile] : mFiles)
    {
        try
        {
            std::ifstream stStream(stFile, std::ios::binary);
            auto stRoot = nlohmann::json::parse(stStream);
            auto vFileEntries = Parse(stRoot, svGame);
            if (!vFileEntries.empty())
            {
                spdlog::info("{}: Translation : Loaded {} changes from {}.", svGame, vFileEntries.size(), stFile.string());
                vEntries.insert(vEntries.end(), std::make_move_iterator(vFileEntries.begin()), std::make_move_iterator(vFileEntries.end()));
            }
        }
        catch (const std::exception& stException)
        {
            spdlog::error("{}: Translation : Skipped {}: {}", svGame, stFile.string(), stException.what());
        }
    }
    return vEntries;
}
