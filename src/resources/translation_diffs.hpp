#pragma once

namespace TranslationDiffs
{
    struct Entry
    {
        uint32_t nBlock = 0;
        int nIndex = 0;
        uint32_t nHash = 0;
        int nTable = -1;
        bool bDisabled = false;
        std::string sLanguage;
        std::string sText;
    };

    std::vector<Entry> Parse(const nlohmann::json& stRoot, std::string_view svGame);
    std::vector<Entry> Load(std::string_view svGame);
}
