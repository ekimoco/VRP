#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <windows.h>

namespace win32 {

std::string ErrorMessage(uint32_t code);
std::string LastErrorMessage();

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