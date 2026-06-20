#include "stdafx.h"
#include "background_shuffle_warning.hpp"

#include "helper.hpp"
#include "logging.hpp"

void BackgroundShuffleWarning::Check()
{
	if (!bEnabled)
	{
        spdlog::info("Background Shuffle Warning: Disabled via config, skipping check.");
        return;
	}
	if (Util::IsSteamOS())
	{
		return;
	}

	spdlog::info("Background Shuffle Warning: Checking wallpaper settings...");
	HKEY hKey;
	DWORD value = 0;
	DWORD size = sizeof(value);

	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Wallpapers", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
        spdlog::info("Background Shuffle Warning: Wallpaper registry key not found, skipping check.");
		return; // key doesn't exist, bail
	}

	LSTATUS status = RegQueryValueExW(hKey, L"BackgroundType", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size);
	RegCloseKey(hKey);

	if (status == ERROR_SUCCESS && value > 1)
	{
		Logging::ShowConsole();
		const char* message =
			"MGSHDFix Warning:\n\n"
			"Having Windows wallpaper set to Slideshow / Window Spotlight mode is known to cause stuttering while in DirectX games.\n"
			"\n"
			"If you experience intermittent stuttering, change your wallpaper to a static picture in your personalization settings.";

		std::cout << message << std::endl;
	}
	else
	{
        spdlog::info("Windows background shuffle is not enabled. (Correct)");
	}
}
