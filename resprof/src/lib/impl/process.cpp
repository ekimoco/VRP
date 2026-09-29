#include "process.hpp"
#include "win32.hpp"
#include <boost/nowide/convert.hpp>
#include <dxgi.h>
#include <tlhelp32.h>
#include <windows.h>

namespace nw = boost::nowide;
namespace {

std::wstring UpperInvariant(const std::wstring &s) {
  if (s.empty())
    return s;

  const int n = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_UPPERCASE, s.c_str(),
                              static_cast<int>(s.size()), nullptr, 0, nullptr,
                              nullptr, 0);

  std::wstring out(n, L'\0');
  LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_UPPERCASE, s.c_str(),
                static_cast<int>(s.size()), out.data(), n, nullptr, nullptr, 0);
  return out;
}

} // namespace

bool process::EnumerateProcesses(std::vector<ProcessEntry> &out,
                                 std::string &err) {
  out.clear();
  win32::UniqueHandle snap(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
  if (!snap) {
    err = "CreateToolhelp32Snapshot failed: " + win32::LastErrorMessage();
    return false;
  }

  PROCESSENTRY32W pe{};
  pe.dwSize = sizeof(pe);

  if (!Process32FirstW(snap.get(), &pe)) {
    err = "Process32FirstW failed:  " + win32::LastErrorMessage();
    return false;
  }

  do {
    const std::wstring wname = pe.szExeFile;
    // though DWORD and uint32_t are the same, i'd wanted to assert intention
    // that the struct is using cstdint instead of windows.h to avoid type
    // confusion
    out.push_back({.pid = static_cast<uint32_t>(pe.th32ProcessID),
                   .exeName = nw::narrow(wname),
                   .key = nw::narrow(UpperInvariant(wname))});

  } while (Process32NextW(snap.get(), &pe));

  return true;
}

// string_view isn't guaranteed to have the null terminator
std::string process::NormalizeName(std::string_view name) {
  return nw::narrow(UpperInvariant(nw::widen(name.data(), name.size())));
}