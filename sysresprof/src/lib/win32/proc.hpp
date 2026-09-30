#pragma once

#include "prof/sampler.hpp"

#include <boost/nowide/convert.hpp>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nw = boost::nowide;

namespace win32 {

namespace proc {

struct ProcessEntry {
  uint32_t pid;
  std::string exeName; // UTF-8 file name
  std::string key;     // should normalize
};

std::optional<uint64_t> CountProcess(std::string &pName, std::string &err);
bool EnumerateProcesses(std::vector<ProcessEntry> &out, std::string &err);
} // namespace proc

// One row per sample for the whole machine.
struct SystemSample {
  std::chrono::system_clock::time_point t;

  float cpuPct;    // 0-100, all cores combined
  uint32_t cpuMHz; // current clock, changes with boost

  uint64_t ramUsedBytes;
  uint64_t ramTotalBytes;
  float diskActivePct; // 0-100
};

// One row per sample per watched process.
struct ProcessSample {
  std::chrono::system_clock::time_point t;
  uint32_t pid;

  float cpuPct;       // 0-100, normalized like Task Manager
  float cpuCores;     // e.g. 2.3 = using about 2.3 cores' worth
  float maxThreadPct; // 0-100, busiest single thread

  uint64_t workingSetBytes; // RAM the process is actually using
  uint64_t diskReadBytesPerSec;
  uint64_t diskWriteBytesPerSec;

  struct Gpu {
    uint8_t index;
    float pct3D;
    float pctEncode;
    float pctDecode;
    uint64_t dedicatedBytes; // VRAM
    uint64_t sharedBytes;    // system RAM borrowed by the GPU
  };
  std::optional<Gpu> gpu; // empty = not using a GPU (replaces isUsingGpu)
};

} // namespace win32