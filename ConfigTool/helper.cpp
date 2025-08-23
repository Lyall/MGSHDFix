#include "helper.hpp"
#include <array>
#include <wx/filefn.h>
#include <wx/log.h>


namespace Helper
{
    std::filesystem::path FindASILocation(std::string fileName)
    {
        static const std::array<std::string, 4> paths = { "", "plugins", "scripts", "update" };

        std::filesystem::path base = wxGetCwd().ToStdString();

        for (const auto& p : paths)
        {
            std::filesystem::path filePath = base / p / (fileName + ".asi");
            if (std::filesystem::exists(filePath))
            {
                return filePath.parent_path();
            }
        }
        wxLogError("Unable to locate %s.asi in %s and the other supported ASI directories.", fileName, wxGetCwd());
        return {};
    }

}
