#include "stdafx.h"
#include "windows_multiplane_overlay_warning.hpp"

#include "logging.hpp"

void Win11AltTabPerformanceWarning::Check()
{
	HKEY hKey;
	DWORD value = 0;
	DWORD size = sizeof(value);
	DWORD type = 0;

	const LSTATUS openStatus = RegOpenKeyExW(
		HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows\\Dwm",
		0,
		KEY_READ | KEY_WOW64_64KEY,
		&hKey
	);

	if (openStatus != ERROR_SUCCESS)
	{
		return;
	}

	const LSTATUS queryStatus = RegQueryValueExW(
		hKey,
		L"OverlayTestMode",
		nullptr,
		&type,
		reinterpret_cast<LPBYTE>(&value),
		&size
	);

	RegCloseKey(hKey);

	if (queryStatus != ERROR_SUCCESS || type != REG_DWORD || value != 5)
	{
		return;
	}

	spdlog::warn("------------------- PERFORMANCE WARNING -------------------");
	spdlog::warn("Alt-Tab Performance Warning: Windows Multi-Plane Overlay (MPO) appears to be disabled.");
	spdlog::warn("Alt-Tab Performance Warning: Disabling MPO can cause freezing or slowdowns when alt-tabbing the game.");
	spdlog::warn("Alt-Tab Performance Warning: This is a Windows display configuration settings issue and is not caused by the game.");
	spdlog::warn("Alt-Tab Performance Warning: NVIDIA provides a registry file that can be used to restore the default MPO setting:");
	spdlog::warn("Alt-Tab Performance Warning: https://nvidia.custhelp.com/app/answers/detail/a_id/5157/~/what-is-multi-plane-overlay-(mpo)-in-windows-11");
	spdlog::warn("Alt-Tab Performance Warning: This affects both NVIDIA and AMD users. If you experience the issue, try re-enabling MPO using NVIDIA's registry fix.");
	spdlog::warn("------------------- PERFORMANCE WARNING -------------------");
}
