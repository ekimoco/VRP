#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace process {

struct ProcessEntry {
  uint32_t pid;
  std::string exeName; // UTF-8 file name
  std::string key;     // should normalize
};

bool EnumerateProcesses(std::vector<ProcessEntry> &out, std::string &err);

std::string NormalizeName(std::string_view name);

} // namespace process