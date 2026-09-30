#include "win32/uhandle.hpp"
#include <windows.h>

void win32::UniqueHandle::reset(void *h) noexcept {
  // only close the handle if it represents a valid kernel obj
  if (h_ != nullptr &&
      h_ != reinterpret_cast<void *>(static_cast<intptr_t>(-1))) {
    ::CloseHandle(static_cast<HANDLE>(h_));
  }
  h_ = h;
}