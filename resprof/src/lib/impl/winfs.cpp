#include "winfs.hpp"

#include <boost/nowide/convert.hpp>
#include <format>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;
namespace nw = boost::nowide;

fs::path winfs::ToPath(const std::string &utf8) {
  return fs::path(nw::widen(utf8));
}

std::string winfs::FromPath(const fs::path &p) {
  return nw::narrow(p.wstring());
}

bool winfs::ResolvePath(const std::string &in, std::string &out,
                        std::string &err) {
  const fs::path input = ToPath(in);
  if (input.has_root_name() && !input.has_root_directory()) {
    err = std::format(
        "'{}' はドライブ相対パスです。'{}\\...' の形式で指定してください。", in,
        FromPath(input.root_name()));
    return false;
  }

  std::error_code ec;
  fs::path p = fs::absolute(input, ec);
  if (!ec)
    p = fs::weakly_canonical(p, ec);

  if (ec) {
    err = std::format("cannot resolve '{}': {}", in, ec.message());
    return false;
  }

  out = FromPath(p);
  return true;
}

int winfs::GetFileType(const std::string &dir) noexcept {
  std::error_code ec;
  const fs::path p = ToPath(dir);
  return GetFileType(p);
}

int winfs::GetFileType(const fs::path &path) noexcept {
  std::error_code ec;

  if (fs::is_directory(path, ec))
    return FILETYPE_DIR;
  if (fs::exists(path, ec))
    return FILETYPE_FILE;
  else
    return FILETYPE_NONE;
}

bool winfs::EnsureDirectory(const std::string &dir, std::string &err) {
  switch (GetFileType(dir)) {
  case FILETYPE_NONE: {
    const fs::path p = ToPath(dir);
    std::error_code createEc;
    fs::create_directories(p, createEc);

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

bool winfs::EnsureFile(const std::string &path, std::string &err) {
  switch (GetFileType(path)) {
  case FILETYPE_NONE: {
    const fs::path fPath = ToPath(path);
    const fs::path fParentPath = fPath.parent_path();

    std::error_code createEc;
    fs::create_directories(fParentPath, createEc);
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