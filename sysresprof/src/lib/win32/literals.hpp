#pragma once

#include <boost/nowide/convert.hpp>
#include <string>
#include <string_view>
#include <windows.h>

namespace nw = boost::nowide;

namespace win32::literals {

  std::wstring UpperInvariant(const std::wstring &s);

  inline std::string NormalizeName(const char *str, size_t size) {
    return nw::narrow(UpperInvariant(nw::widen(str, size)));
  }

  inline std::string NormalizeName(std::string_view str) {
    return NormalizeName(str.data(), str.size());
  }

  inline std::string operator""_norm(const char *str, size_t size) {
    return NormalizeName(str, size);
  }

} // namespace win32::literals