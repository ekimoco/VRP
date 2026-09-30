#include "win32/proc.hpp"
#include "win32/global.hpp"
#include "win32/uhandle.hpp"
#include "win32/literals.hpp"
#include "win32/hw.hpp"

#include <boost/nowide/convert.hpp>
#include <dxgi.h>
#include <tlhelp32.h>
#include <windows.h>

namespace nw = boost::nowide;

bool win32::proc::EnumerateProcesses(std::vector<ProcessEntry> &out,
                                 std::string &err) {
  out.clear();
  win32::UniqueHandle snap(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
  if (!snap) {
    err = "CreateToolhelp32Snapshotの実行が失敗しました： " +
          win32::LastErrorMessage();
    return false;
  }

  PROCESSENTRY32W pe{};
  pe.dwSize = sizeof(pe);

  if (!Process32FirstW(snap.get(), &pe)) {
    err = "Process32FirstWの実行が失敗しました：" + win32::LastErrorMessage();
    return false;
  }

  do {
    const std::wstring wname = pe.szExeFile;
    // though DWORD and uint32_t are the same,
    // i'd wanted to assert my intention that the struct is using
    // cstdint in replacement of windows.h to avoid type confusion
    out.push_back({.pid = static_cast<uint32_t>(pe.th32ProcessID),
                   .exeName = nw::narrow(wname),
                   .key = nw::narrow(win32::literals::UpperInvariant(wname))});

  } while (Process32NextW(snap.get(), &pe));

  return true;
}