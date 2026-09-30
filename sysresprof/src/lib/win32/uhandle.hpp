#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <winternl.h>

namespace win32 {

class UniqueHandle {
public:
  UniqueHandle() = default;
  explicit UniqueHandle(void *h) noexcept : h_(h) {}
  ~UniqueHandle() { reset(); }

  UniqueHandle(const UniqueHandle &) = delete;
  UniqueHandle &operator=(const UniqueHandle &) = delete;

  UniqueHandle(UniqueHandle &&o) noexcept : h_(std::exchange(o.h_, nullptr)) {}
  UniqueHandle &operator=(UniqueHandle &&o) noexcept {
    if (this != &o)
      reset(std::exchange(o.h_, nullptr));
    return *this;
  }

  void *get() const noexcept { return h_; }
  void *release() noexcept { return std::exchange(h_, nullptr); }
  void reset(void *h = nullptr) noexcept;

  explicit operator bool() const noexcept {
    return h_ != nullptr &&
           h_ != reinterpret_cast<void *>(static_cast<intptr_t>(-1));
  }

private:
  void *h_ = nullptr;
};

} // namespace win32