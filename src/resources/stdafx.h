#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <SDKDDKVer.h>
#include <cassert>
#include <Windows.h>
#include <winsock2.h>
#include <Shlobj.h>
#include <cstdint>
#include <sstream>
#include <fstream>
#include <string>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <inttypes.h>
#include <filesystem>
#include <codecvt>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <regex>
#include <tlhelp32.h>
#include <psapi.h>
#include <map>
#include <winhttp.h>
#include <random>
#include <d3dcompiler.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <initializer_list>
#include <functional>
#include <optional>
#include <array>
#include <unordered_set>
#include <unordered_map>
#include <numbers>
#include <bcrypt.h> //sha256
#include <limits>


#include <shellapi.h> //ShellExecuteA

#include <future>
#include <subauth.h>
#include <string_view>
