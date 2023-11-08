#pragma once

#define WIN32_LEAN_AND_MEAN

#include <cassert>
#include <Windows.h>
#include <Shlobj.h>
#include <cstdint>
#include <sstream>
#include <fstream>
#include <string>
#include <iostream>
#include <inttypes.h>
#include <mutex>
#include <filesystem>

#include "external/loguru/loguru.hpp"
#include "external/inipp/inipp/inipp.h"
#include "external/length-disassembler/headerOnly/ldisasm.h"