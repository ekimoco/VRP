#include "prof/sampler.hpp"
#include "win32/proc.hpp"
#include "win32/hw.hpp"

#include <vector>

namespace hw = win32::hw;

namespace {
// Hypothetic
std::vector<win32::proc::ProcessEntry> gProcEntries;
std::vector<hw::SysSnapshot> gSnapshots;
} // namespace

bool sampler::GetSystemUsage(std::span<SampleData> &out, std::string &err) {
  // init gEntries
  if (!gProcEntries.empty())
    gProcEntries.clear();

  if (!win32::proc::EnumerateProcesses(gProcEntries, err))
    return false;

  for (auto& proc : gProcEntries) {
    SampleData sample;
  }
}