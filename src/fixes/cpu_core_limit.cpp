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
        spdlog::error("CPU affinity limit: Failed to set process affinity mask. Error code: {}", GetLastError());
        return;
    }
    spdlog::info("CPU affinity limit: Process affinity mask set to 0x3 (CPU 0 and CPU 1). This should help prevent crashes on some newer CPUs.");
    spdlog::warn("CPU affinity limit: If you performance experience issues, such as slowdowns or audio glitches while streaming, disable this fix in the config file.");
}
