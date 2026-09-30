#pragma once

#include <windows.h>
#include <boost/nowide/convert.hpp>

namespace nw = boost::nowide;

namespace win32 {
namespace literals {

std::wstring UpperInvariant(const std::wstring &s);

std::string NormalizeName(const char *str, size_t size) {
  return nw::narrow(UpperInvariant(nw::widen(str, size)));
}

std::string NormalizeName(std::string_view str) {
  return NormalizeName(str.data(), str.size());
}

std::string operator""_norm(const char *str, size_t size) {
  return NormalizeName(str, size);
}

} // namespace literals
} // namespace win32