#pragma once

// Needs to import windows.h first
#include "win32/proc.hpp"
#include <chrono>
#include <cstdint>
// clang-format off
#include <windows.h>
// clang-format on
#include <powrprof.h>
#include <string_view>
#include <vector>

namespace sampler {

struct SampleData {};

bool GetSystemUsage(std::vector<SampleData> &out, std::string &err);

} // namespace sample