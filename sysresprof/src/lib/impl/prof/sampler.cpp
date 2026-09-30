#include "prof/sampler.hpp"
#include "win32/proc.hpp"
#include "win32/hw.hpp"

#include <vector>

namespace hw = win32::hw;

namespace {

std::vector<win32::proc::ProcessEntry> gProcEntries;
std::vector<hw::snapshot::SysSnapshot> gSnapshots;

} // namespace

bool sampler::GetSystemUsage(std::vector<SampleData> &out, std::string &err) {
  // init gEntries
  if (!gProcEntries.empty())
    gProcEntries.clear();

  if (!win32::proc::EnumerateProcesses(gProcEntries, err))
    return false;

  for (auto& proc : gProcEntries) {
    // TODO: Sample
  }
  return true;
}