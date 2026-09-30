#include "win32/literals.hpp"
#include <windows.h>

std::wstring win32::literals::UpperInvariant(const std::wstring &s) {
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