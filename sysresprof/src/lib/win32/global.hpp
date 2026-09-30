#pragma once

#include <cstdint>
#include <string>

namespace win32 {
std::string ErrorMessage(uint32_t code);
std::string LastErrorMessage();
}