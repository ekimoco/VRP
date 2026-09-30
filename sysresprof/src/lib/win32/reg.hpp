#pragma once

#include <winternl.h>
#include <string>
#include <cstdint>

namespace win32 {

namespace reg {

bool ReadRegVal(HKEY root, const std::string &subkey, const std::string &name,
                uint32_t &out);
bool ReadRegVal(HKEY root, const std::string &subkey, const std::string &name,
                uint64_t &out);
bool ReadRegVal(HKEY root, const std::string &subkey, const std::string &name,
                std::string &out);

bool WriteRegVal(HKEY root, const std::string &subkey, const std::string &name,
                 uint32_t value);
bool WriteRegVal(HKEY root, const std::string &subkey, const std::string &name,
                 uint64_t value);
bool WriteRegVal(HKEY root, const std::string &subkey, const std::string &name,
                 const std::string &value);

} // namespace reg

} // namespace win32