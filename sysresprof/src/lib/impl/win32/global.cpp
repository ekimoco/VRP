#include "win32/global.hpp"

#include <boost/nowide/convert.hpp>
#include <format>
#include <vector>
#include <windows.h>
#include <winternl.h>

#ifndef RRF_SUBKEY_WOW6464KEY
#define RRF_SUBKEY_WOW6464KEY 0x00010000
#endif

namespace nw = boost::nowide;

// win32
std::string win32::ErrorMessage(uint32_t code) {
  const std::string hex = std::format("0x{:08X}", code);

  wchar_t *buf = nullptr;
  const DWORD len = FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, code, 0, reinterpret_cast<LPWSTR>(&buf), 0, nullptr);

  if (len == 0 || buf == nullptr)
    return std::format("unknown error ({})", hex);

  std::wstring msg(buf, len);
  LocalFree(buf);

  while (!msg.empty() &&
         (msg.back() == L'\r' || msg.back() == L'\n' || msg.back() == L' '))
    msg.pop_back();

  return std::format("{} ({})", nw::narrow(msg), hex);
}

std::string win32::LastErrorMessage() { return ErrorMessage(GetLastError()); }