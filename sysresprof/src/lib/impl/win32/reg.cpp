#include "win32/reg.hpp"
#include <boost/nowide/convert.hpp>

namespace nw = boost::nowide;

namespace {
struct RegKey {
  HKEY h = nullptr;
  RegKey() = default;
  RegKey(const RegKey &) = delete;
  RegKey &operator=(const RegKey &) = delete;
  ~RegKey() {
    if (h)
      RegCloseKey(h);
  }
};

LSTATUS GetRaw(HKEY root, const std::string &subkey, const std::string &name,
               DWORD flags, void *data, DWORD *size) {
  return RegGetValueW(root, nw::widen(subkey).c_str(), nw::widen(name).c_str(),
                      flags | RRF_SUBKEY_WOW6464KEY, nullptr, data, size);
}

LSTATUS SetRaw(HKEY root, const std::string &subkey, const std::string &name,
               DWORD type, const void *data, DWORD size) {
  RegKey key;
  LSTATUS s = RegCreateKeyExW(root, nw::widen(subkey).c_str(), 0, nullptr, 0,
                              KEY_SET_VALUE | KEY_WOW64_64KEY, nullptr, &key.h,
                              nullptr);
  if (s != ERROR_SUCCESS)
    return s;

  return RegSetValueExW(key.h, nw::widen(name).c_str(), 0, type,
                        static_cast<const BYTE *>(data), size);
}

// Scalar type -> registry type mapping
template <class T> struct Scalar;
template <> struct Scalar<uint32_t> {
  static constexpr DWORD rrf = RRF_RT_REG_DWORD;
  static constexpr DWORD type = REG_DWORD;
};
template <> struct Scalar<uint64_t> {
  static constexpr DWORD rrf = RRF_RT_REG_QWORD;
  static constexpr DWORD type = REG_QWORD;
};

template <class T>
LSTATUS ReadScalar(HKEY root, const std::string &subkey,
                   const std::string &name, T &out) {
  T tmp{};
  DWORD size = sizeof(tmp);
  LSTATUS s = GetRaw(root, subkey, name, Scalar<T>::rrf, &tmp, &size);
  if (s == ERROR_SUCCESS)
    out = tmp; // don't touch `out` on failure
  return s;
}

template <class T>
LSTATUS WriteScalar(HKEY root, const std::string &subkey,
                    const std::string &name, T value) {
  return SetRaw(root, subkey, name, Scalar<T>::type, &value, sizeof(value));
}
} // namespace

LSTATUS win32::reg::ReadRegValWithStatus(HKEY root, const std::string &subkey,
                                         const std::string &name,
                                         uint32_t &out) {
  return ReadScalar(root, subkey, name, out);
}

LSTATUS win32::reg::ReadRegValWithStatus(HKEY root, const std::string &subkey,
                                         const std::string &name,
                                         uint64_t &out) {
  return ReadScalar(root, subkey, name, out);
}

LSTATUS win32::reg::ReadRegValWithStatus(HKEY root, const std::string &subkey,
                                         const std::string &name,
                                         std::string &out) {
  DWORD size = 0;

  for (;;) {
    LSTATUS s = GetRaw(root, subkey, name, RRF_RT_REG_SZ, nullptr, &size);
    if (s != ERROR_SUCCESS)
      return s;

    std::wstring buf(size / sizeof(wchar_t), L'\0');
    s = GetRaw(root, subkey, name, RRF_RT_REG_SZ, buf.data(), &size);

    if (s == ERROR_MORE_DATA)
      continue; // value grew between calls; retry
    if (s != ERROR_SUCCESS)
      return s;

    buf.resize(size / sizeof(wchar_t));
    if (!buf.empty() && buf.back() == L'\0')
      buf.pop_back();

    out = nw::narrow(buf);
    return ERROR_SUCCESS;
  }
}

LSTATUS win32::reg::WriteRegValWithStatus(HKEY root, const std::string &subkey,
                                          const std::string &name,
                                          uint32_t value) {
  return WriteScalar(root, subkey, name, value);
}

LSTATUS win32::reg::WriteRegValWithStatus(HKEY root, const std::string &subkey,
                                          const std::string &name,
                                          uint64_t value) {
  return WriteScalar(root, subkey, name, value);
}

LSTATUS win32::reg::WriteRegValWithStatus(HKEY root, const std::string &subkey,
                                          const std::string &name,
                                          const std::string &value) {
  const std::wstring w = nw::widen(value);
  if (w.size() >= MAXDWORD / sizeof(wchar_t))
    return ERROR_INVALID_PARAMETER;

  const DWORD bytes =
      static_cast<DWORD>((w.size() + 1) * sizeof(wchar_t)); // include null
  return SetRaw(root, subkey, name, REG_SZ, w.c_str(), bytes);
}