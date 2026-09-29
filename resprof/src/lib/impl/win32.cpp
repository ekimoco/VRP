#include "win32.hpp"

#include <boost/nowide/convert.hpp>
#include <format>
#include <windows.h>

#ifndef RRF_SUBKEY_WOW6464KEY
#define RRF_SUBKEY_WOW6464KEY 0x00010000
#endif

namespace nw = boost::nowide;
static_assert(sizeof(uint32_t) == sizeof(DWORD), "REG_DWORD must be 32-bit");

namespace {
struct RegKey {
  HKEY h = nullptr;
  ~RegKey() {
    if (h)
      RegCloseKey(h);
  }
};

LONG GetRaw(HKEY root, const std::string &subkey, const std::string &name,
            DWORD flags, void *data, DWORD *size) {
  return RegGetValueW(root, nw::widen(subkey).c_str(), nw::widen(name).c_str(),
                      flags | RRF_SUBKEY_WOW6464KEY, nullptr, data, size);
}

bool SetRaw(HKEY root, const std::string &subkey, const std::string &name,
            DWORD type, const void *data, DWORD size) {
  RegKey key;
  if (RegCreateKeyExW(root, nw::widen(subkey).c_str(), 0, nullptr, 0,
                      KEY_SET_VALUE | KEY_WOW64_64KEY, nullptr, &key.h,
                      nullptr) != ERROR_SUCCESS)
    return false;
  return RegSetValueExW(key.h, nw::widen(name).c_str(), 0, type,
                        static_cast<const BYTE *>(data), size) == ERROR_SUCCESS;
}
} // namespace

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

// win32::reg
bool win32::reg::ReadRegVal(HKEY root, const std::string &subkey,
                            const std::string &name, uint32_t &out) {
  DWORD size = sizeof(out);
  return GetRaw(root, subkey, name, RRF_RT_REG_DWORD, &out, &size) ==
         ERROR_SUCCESS;
}

bool win32::reg::ReadRegVal(HKEY root, const std::string &subkey,
                            const std::string &name, uint64_t &out) {
  DWORD size = sizeof(out);
  return GetRaw(root, subkey, name, RRF_RT_REG_QWORD, &out, &size) ==
         ERROR_SUCCESS;
}

bool win32::reg::ReadRegVal(HKEY root, const std::string &subkey,
                       const std::string &name, std::string &out) {
  DWORD size = 0;
  for (;;) {
    if (GetRaw(root, subkey, name, RRF_RT_REG_SZ, nullptr, &size) !=
        ERROR_SUCCESS)
      return false;
    std::wstring buf(size / sizeof(wchar_t), L'\0');
    LONG s = GetRaw(root, subkey, name, RRF_RT_REG_SZ, buf.data(), &size);
    if (s == ERROR_MORE_DATA)
      continue; // value grew between calls; retry
    if (s != ERROR_SUCCESS)
      return false;
    buf.resize(size / sizeof(wchar_t));
    if (!buf.empty() && buf.back() == L'\0')
      buf.pop_back();
    out = nw::narrow(buf);
    return true;
  }
}

bool win32::reg::WriteRegVal(HKEY root, const std::string &subkey,
                             const std::string &name, uint32_t value) {
  return SetRaw(root, subkey, name, REG_DWORD, &value, sizeof(value));
}

bool win32::reg::WriteRegVal(HKEY root, const std::string &subkey,
                             const std::string &name, uint64_t value) {
  return SetRaw(root, subkey, name, REG_QWORD, &value, sizeof(value));
}

bool win32::reg::WriteRegVal(HKEY root, const std::string &subkey,
                             const std::string &name,
                             const std::string &value) {
  const std::wstring w = nw::widen(value);
  const DWORD bytes =
      static_cast<DWORD>((w.size() + 1) * sizeof(wchar_t)); // include null
  return SetRaw(root, subkey, name, REG_SZ, w.c_str(), bytes);
}


void win32::UniqueHandle::reset(void *h) noexcept {
  if (h == h_)
    return;
  if (*this)
    CloseHandle(h_);
  h_ = h;
}