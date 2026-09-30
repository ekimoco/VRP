#pragma once

#include <filesystem>
#include <string>

namespace win32::fs {

inline constexpr const int FILETYPE_NONE = 0;
inline constexpr const int FILETYPE_FILE = 1;
inline constexpr const int FILETYPE_DIR = 2;

std::filesystem::path ToPath(const std::string &utf8);
std::string FromPath(const std::filesystem::path &p);

bool ResolvePath(const std::string &in, std::string &out, std::string &err);

int GetFileType(const std::string &dir) noexcept;
int GetFileType(const std::filesystem::path &dir) noexcept;

bool EnsureDirectory(const std::string &dir, std::string &err);
bool EnsureFile(const std::string &dir, std::string &err);

} // namespace win32::fs