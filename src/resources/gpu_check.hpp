#pragma once

#include <string>

/// Checks if the detected GPU meets the minimum requirements.
/// Logs details about vendor, estimated performance, and warnings if below minimum.
///
/// Minimum GPU can be overridden by defining MINIMUM_GPU_NAME before including this header.
/// Default: NVIDIA GeForce GTX 970
void CheckMinimumGPU(const std::string& gpuName);
