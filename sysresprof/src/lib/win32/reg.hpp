#pragma once
#include <cstdint>
#include <string>
#include <windows.h>

namespace win32::reg {

// Read
LSTATUS ReadRegValWithStatus(HKEY root, const std::string &subkey,
                             const std::string &name, uint32_t &out);
LSTATUS ReadRegValWithStatus(HKEY root, const std::string &subkey,
                             const std::string &name, uint64_t &out);
LSTATUS ReadRegValWithStatus(HKEY root, const std::string &subkey,
                             const std::string &name, std::string &out);

// Write
LSTATUS WriteRegValWithStatus(HKEY root, const std::string &subkey,
                              const std::string &name, uint32_t value);
LSTATUS WriteRegValWithStatus(HKEY root, const std::string &subkey,
                              const std::string &name, uint64_t value);
LSTATUS WriteRegValWithStatus(HKEY root, const std::string &subkey,
                              const std::string &name,
                              const std::string &value);

template <class T>
bool ReadRegVal(HKEY root, const std::string &subkey, const std::string &name,
                T &out) {
  return ReadRegValWithStatus(root, subkey, name, out) == ERROR_SUCCESS;
}

template <class T>
bool WriteRegVal(HKEY root, const std::string &subkey, const std::string &name,
                 const T &value) {
  return WriteRegValWithStatus(root, subkey, name, value) == ERROR_SUCCESS;
}

} // namespace win32::reg