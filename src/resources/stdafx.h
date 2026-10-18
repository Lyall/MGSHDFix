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
#include <d3d11_1.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <initializer_list>
#include <functional>
#include <optional>
#include <array>
#include <atomic>
#include <unordered_set>
#include <unordered_map>
#include <numbers>
#include <bcrypt.h> //sha256
#include <limits>
#include <atomic>
#include <memory>
#include <cmath>
#include <cwctype>
#include <thread>
#include <charconv>
#include <deque>


#include <shellapi.h> //ShellExecuteA

#include <future>
#include <subauth.h>
#include <string_view>


#include <hidusage.h>
#include <Xinput.h> //not actually using xinput, as steam input blocks it out - just using the VK defs for the input handler.

#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <set>
#include <thread>

#include <hidsdi.h>
#include <hidpi.h>
#include <timeapi.h>
#include <smmintrin.h>
