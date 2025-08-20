#include <windows.h>
#include "cpu_core_limit.hpp"
#include "logging.hpp"


void CPUCoreLimitFix::ApplyFix()
{
    if (!g_CPUCoreLimitFix.bEnabled)
    {
        return;
    }

    if (!SetProcessAffinityMask(GetCurrentProcess(), 0x3))
    {
        spdlog::error("Ryzen CPU Crash Fix: Failed to set process affinity mask. Error code: {}", GetLastError());
        return;
    }
    spdlog::info("Ryzen CPU Crash Fix: Process affinity mask set to 0x3 (CPU 0 and CPU 1). This should help prevent crashes on Ryzen CPUs.");
}
