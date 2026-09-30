#pragma once

#include "win32/proc.hpp"
#include <cstdint>
#include <vector>

namespace win32 {
namespace hw {

struct SysSnapshot {
  std::vector<win32::proc::ProcessEntry> processes;
};

bool CaptureSnapshot(SysSnapshot &out, uint32_t err);

} // namespace hw
} // namespace win32
