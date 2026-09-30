#pragma once

#include <boost/nowide/convert.hpp>
#include <cstdint>
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

bool EnumerateProcesses(std::vector<ProcessEntry> &out, std::string &err);
} // namespace proc
} // namespace win32