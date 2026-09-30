#pragma once

#include "win32/proc.hpp"
#include <chrono>
#include <cstdint>
#include <powrprof.h>
#include <string_view>
#include <vector>
#include <windows.h>


namespace sampler {

struct SampleData {
  std::chrono::_V2::system_clock::time_point t;
  win32::proc::ProcessEntry processEntry;

  std::string_view cpuName;
  uint8_t cpuUtil; // 0-100
  uint64_t cpuMHz; // MHz

  uint8_t ramUtil;  // 0-100
  uint8_t diskUtil; // 0-100

  uint8_t gpuIndex;
  uint8_t gpu3DUtil;
  uint8_t gpuVEncUtil;
  uint8_t gpuVDecUtil;

  uint32_t gpuVramUsage;
  uint32_t gpuSramUsage;
};

bool GetSystemUsage(std::vector<SampleData> &out, std::string &err);

} // namespace sample