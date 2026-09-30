#pragma once
#include "win32/proc.hpp"
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace win32::hw {

namespace profile {

enum class GpuKind : uint8_t {
  Discrete = 0x00,
  Integrated = 0x01,
  Unknown = 0x02
};
struct GpuProfile {
  uint8_t index;
  std::vector<uint64_t> luids; // every LUID this physical GPU shows up under

  uint32_t vendorId;
  uint32_t deviceId; // used to spot duplicates
  uint32_t subSysId;
  uint32_t revision;

  std::string gpuName;
  GpuKind kind;

  uint64_t capDedicatedRam;
  uint64_t capDedicatedSystemRam;
  uint64_t capSharedRam;

  bool HasLuid(uint64_t luid) const {
    return std::find(luids.begin(), luids.end(), luid) != luids.end();
  }

  uint64_t EffectiveCap() const {
    return kind == GpuKind::Integrated
               ? capDedicatedRam + capDedicatedSystemRam + capSharedRam
               : capDedicatedRam;
  }
};

struct SystemProfile {
  std::string cpuName;
  uint32_t logicalCores;
  std::vector<GpuProfile> gpuProfiles;

  const GpuProfile *FindGpu(uint64_t luid) const {
    for (auto &g : gpuProfiles)
      if (g.HasLuid(luid))
        return &g;
    return nullptr;
  }
};

const SystemProfile &GetProfile(std::string &err);

} // namespace profile

namespace snapshot {
struct SysSnapshot {
  std::vector<win32::proc::ProcessEntry> processes;
};

bool CaptureSnapshot(SysSnapshot &out, uint32_t err);

} // namespace snapshot

} // namespace win32::hw
