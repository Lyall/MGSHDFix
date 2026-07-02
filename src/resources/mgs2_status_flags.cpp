#include "stdafx.h"

#include "mgs2_status_flags.hpp"

#include "mgs2_linkvarbuf.hpp"

namespace MGS2_Characters
{
    [[nodiscard]] PlayerCharacter GetCurrentPlayerCharacter()
    {
        const char* residentDirectory = MGS2_LinkVarBuf::GM_SaveResidentDir;

        if (!residentDirectory || !*residentDirectory)
        {
            return PlayerCharacter::Unknown;
        }

        const std::string_view currentResidentDirectory { residentDirectory };

        for (const auto& entry : PlayerResidentEntries)
        {
            if (entry.residentDirectory == currentResidentDirectory)
            {
                return entry.character;
            }
        }

        return PlayerCharacter::Unknown;
    }

    [[nodiscard]] constexpr PlayerCharacterFamily GetPlayerCharacterFamily(const PlayerCharacter character)
    {
        switch (character)
        {
        case PlayerCharacter::NormalRaiden:
        case PlayerCharacter::NakedRaiden:
        case PlayerCharacter::NinjaRaiden:
            return PlayerCharacterFamily::Raiden;

        case PlayerCharacter::NormalSnake:
        case PlayerCharacter::SpecialSnake:
        case PlayerCharacter::TuxedoSnake:
        case PlayerCharacter::MGS1Snake:
            return PlayerCharacterFamily::Snake;

        case PlayerCharacter::Pliskin:
            return PlayerCharacterFamily::Pliskin;

        default:
            return PlayerCharacterFamily::Unknown;
        }
    }

    [[nodiscard]] PlayerCharacterFamily GetCurrentCharacterFamily()
    {
        return GetPlayerCharacterFamily(GetCurrentPlayerCharacter());
    }

    [[nodiscard]] bool IsCurrentlyCharacter(const PlayerCharacter character)
    {
        return GetCurrentPlayerCharacter() == character;
    }

    [[nodiscard]] bool IsRaiden()
    {
        return GetCurrentCharacterFamily() == PlayerCharacterFamily::Raiden;
    }

    [[nodiscard]] bool IsSnake()
    {
        return GetCurrentCharacterFamily() == PlayerCharacterFamily::Snake;
    }

    [[nodiscard]] bool IsPliskin()
    {
        return GetCurrentCharacterFamily() == PlayerCharacterFamily::Pliskin;
    }

    [[nodiscard]] constexpr std::string_view GetPlayerCharacterName(const PlayerCharacter character)
    {
        switch (character)
        {
        case PlayerCharacter::NormalRaiden:
        case PlayerCharacter::NakedRaiden:
            return "RAIDEN";

        case PlayerCharacter::NinjaRaiden:
            return "NINJA RAIDEN";

        case PlayerCharacter::NormalSnake:
            return "SNAKE";

        case PlayerCharacter::TuxedoSnake:
            return "TUXEDO SNAKE";

        case PlayerCharacter::MGS1Snake:
            return "SOLID SNAKE";

        case PlayerCharacter::Pliskin:
            return "PLISKIN";

        default:
            return "Unknown";
        }
    }

    [[nodiscard]] std::string_view GetCurrentCharacterName()
    {
        return GetPlayerCharacterName(GetCurrentPlayerCharacter());
    }

    [[nodiscard]] bool IsCurrentResident(const std::string_view residentDirectory)
    {
        const char* currentResidentDirectory = MGS2_LinkVarBuf::GM_SaveResidentDir;
        return currentResidentDirectory && std::string_view { currentResidentDirectory } == residentDirectory;
    }

}
