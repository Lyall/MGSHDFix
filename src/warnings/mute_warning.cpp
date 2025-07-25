#include "common.hpp"
#include "mute_warning.hpp"

#include "logging.hpp"

void MuteWarning::CheckStatus() const
{
    if (!(eGameType & (MG | MGS2 | MGS3)))
    {
        return;
    }

    if (*muteWarningAddress)
    {
        spdlog::warn("------ GAME AUDIO OUTPUT MUTED ------");

        spdlog::warn("Game audio output is currently muted via the main launcher.");
        if (bEnabled)
        {
            spdlog::warn("Set \"Mute Warning\" to disabled in the config file to disable console warnings (this warning will still appear in the log.)");
            Logging::ShowConsole();
            std::cout << "MGSHDFix - Muted Audio Output Warning\nGame audio output is currently muted via the main launcher.\nIf this is intentional, set \"Mute Warning\" to \"false\" in the MGSHDFix config file to disable console warnings.\n(This warning will still appear in the log file while disabled.)" << std::endl;
        }
        spdlog::warn("------ GAME AUDIO OUTPUT MUTED ------");

    }
}

void MuteWarning::Setup()
{
    if (!(eGameType & (MG | MGS2 | MGS3)))
    {
        return;
    }

    if (uint8_t* muteWarningResult = Memory::PatternScan(baseModule, "66 0F 7F 0D ?? ?? ?? ?? 66 0F 6F 0D ?? ?? ?? ?? 66 0F 7F 05 ?? ?? ?? ?? 66 0F 7F 0D", "MG-MG2 | MGS 2 | MGS 3: Mute Warning"))
    {
        muteWarningAddress = reinterpret_cast<int*>(Memory::GetAbsolute(reinterpret_cast<uintptr_t>(muteWarningResult) + 0x4) + (eGameType & MGS2 ? 0x4 : 0xC));
        if (muteWarningAddress == nullptr)
        {
            spdlog::error("MG-MG2 | MGS 2 | MGS3: Mute Warning: Settings strut not found.");
            return;
        }
        if (g_Logging.bVerboseLogging)
        {
            spdlog::info("MG-MG2 | MGS 2 | MGS3: Mute Warning: Settings strut is at {:s}+{:X}", sExeName.c_str(), reinterpret_cast<uintptr_t>(muteWarningAddress) - reinterpret_cast<uintptr_t>(baseModule));
        }
    }

}
