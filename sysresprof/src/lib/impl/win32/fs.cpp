#include "win32/fs.hpp"

#include <boost/nowide/convert.hpp>
#include <format>
#include <fstream>
#include <system_error>

namespace stdfs = std::filesystem;
namespace nw = boost::nowide;

stdfs::path win32::fs::ToPath(const std::string &utf8) {
  return stdfs::path(nw::widen(utf8));
}

std::string win32::fs::FromPath(const stdfs::path &p) {
  return nw::narrow(p.wstring());
}

bool win32::fs::ResolvePath(const std::string &in, std::string &out,
                        std::string &err) {
  const stdfs::path input = ToPath(in);
  if (input.has_root_name() && !input.has_root_directory()) {
    err = std::format(
        "'{}' はドライブ相対パスです。'{}\\...' の形式で指定してください。", in,
        FromPath(input.root_name()));
    return false;
  }

  std::error_code ec;
  stdfs::path p = stdfs::absolute(input, ec);
  if (!ec)
    p = stdfs::weakly_canonical(p, ec);

  if (ec) {
    err = std::format("cannot resolve '{}': {}", in, ec.message());
    return false;
  }

  out = FromPath(p);
  return true;
}

int win32::fs::GetFileType(const std::string &dir) noexcept {
  std::error_code ec;
  const stdfs::path p = ToPath(dir);
  return GetFileType(p);
}

int win32::fs::GetFileType(const stdfs::path &path) noexcept {
  std::error_code ec;

  if (stdfs::is_directory(path, ec))
    return FILETYPE_DIR;
  if (stdfs::exists(path, ec))
    return FILETYPE_FILE;
  else
    return FILETYPE_NONE;
}

bool win32::fs::EnsureDirectory(const std::string &dir, std::string &err) {
  switch (GetFileType(dir)) {
  case FILETYPE_NONE: {
    const stdfs::path p = ToPath(dir);
    std::error_code createEc;
    stdfs::create_directories(p, createEc);

    if (GetFileType(p) == FILETYPE_NONE) {
      err = std::format("フォルダー '{}'の生成に失敗しました: {}", dir,
                        createEc.message());
      return false;
    }

    return true;
  }
  case FILETYPE_DIR:
    return true;
  case FILETYPE_FILE:
    err =
        std::format("'{}'はファイルです。フォルダーを指定してください。", dir);
    return false;
  }
  return false;
}

bool win32::fs::EnsureFile(const std::string &path, std::string &err) {
  switch (GetFileType(path)) {
  case FILETYPE_NONE: {
    const auto fPath = ToPath(path);
    const auto fParentPath = fPath.parent_path();

    std::error_code createEc;
    std::filesystem::create_directories(fParentPath, createEc);
    std::ofstream{fPath};

    if (GetFileType(fPath) == FILETYPE_NONE) {
      err = std::format("ファイル '{}'の生成に失敗しました: {}", path,
                        createEc.message());
      return false;
    }

    return true;
  }
  case FILETYPE_DIR:
    err =
        std::format("'{}'は既に存在するフォルダーです。フォルダーを削除するか、名前を変更してください。", path);
    return false;
  case FILETYPE_FILE:
    return true;
  }
  return false;
}