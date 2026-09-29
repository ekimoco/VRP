#include "constants.hpp"
#include "lib/process.hpp"
#include "lib/win32.hpp"
#include "lib/winfs.hpp"

#include <boost/nowide/args.hpp>
#include <boost/nowide/iostream.hpp>

#include <algorithm>
#include <atomic>
#include <charconv>
#include <cstdint>
#include <ctime>
#include <format>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <windows.h>

namespace nw = boost::nowide;

namespace {
constexpr uint32_t kDefaultIntervalMs = 1000;
constexpr uint32_t kMinIntervalMs = 50;
bool gMakeDir = false;
bool gDismissWarning = false;

struct Target {
  std::string name;
  std::string key;
};

struct Config {
  // clang-format off
    uint32_t                    intervalMs = kDefaultIntervalMs;
    std::string                 outDir     = ".";
    std::vector<Target>         targets;
    bool                        toStdout = false;
    bool                        isUsingDefaultTarget = false;
  // clang-format on
};

enum class ParseResult { Ok, Exit, Error };

constexpr ProgramDetails kDefaultTargets[] = {
    ProgramExeNames::kVRChat,
    ProgramExeNames::kVirtualDesktop_Service,
    ProgramExeNames::kVirtualDesktop_Streamer,
    ProgramExeNames::kSteamVR_vrmonitor,
    ProgramExeNames::kSteamVR_vrserver,
    ProgramExeNames::kSteamVR_vrwebhelper,
    ProgramExeNames::kMeta_OVRServer_x64,
    ProgramExeNames::kMeta_OVRServiceLauncher,
    ProgramExeNames::kMeta_OVRRedir};

void AddTarget(std::vector<Target> &targets, std::string_view name) {
  std::string key = process::NormalizeName(name);
  for (const auto &t : targets)
    if (t.key == key)
      return;

  targets.push_back(
      {.name = std::string(name.data(), name.size()), .key = std::move(key)});
}

std::string Timestamp() {
  const std::time_t t = std::time(nullptr);
  std::tm tm{};
  localtime_s(&tm, &t);

  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm);
  return buf;
}

bool ParseUInt(std::string_view s, uint32_t &out) {
  if (s.empty())
    return false;

  uint32_t value = 0;
  const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);

  if (ec != std::errc{} || ptr != s.data() + s.size())
    return false;

  out = value;
  return true;
}

void PrintUsage() {
  nw::cout
      << "使用方法: gpumon [options]\n"
         "\n"
         "オプション:\n"
         "    --interval <ミリ秒>   サンプリングの時間間隔 "
         "（単位：ミリ秒・1000分の1秒）を指定します。 \n"
         "　　　　　　　　　　　　　（デフォルト："
      << kDefaultIntervalMs << " ms, 最小 " << kMinIntervalMs
      << " ms）\n"
         "\n"
         "    --out-dir <path>     "
         "ログのファイルのファイルパスを指定します。（デフォルト：現在位置）\n"
         "\n"
         "    --make-dir           "
         "指定されたフォルダーが存在しない場合、新しく生成します。\n"
         "\n"
         "    --track <name>       追跡するプログラムを指定します。\n"
         "　　　　　　　　　　　　　（例：gpumon --track vrchat.exe "
         "nantokavr.exe）\n"
         "　　　　　　　　　　　　　（デフォルト：VR関連のプログラム名）\n"
         "\n"
         "    --stdout             "
         "追跡途中でデータをこのコンソールに出力します。\n"
         "\n"
         "    -h, --help           このヘルプを出力します。\n"
         "\n"
         "例：\n"
         "   ・gpumon --interval 500\n"
         "       500 ms (0.5秒)ずつ、GPUの負荷を記録します。\n"
         "   ・gpumon --interval 500 --track steamwebhelper.exe vrchat.exe\n"
         "       500 "
         "msずつ、steamwebhelper.exeとvrchat.exeのGPU負荷を記録します。\n"
         "   ・gpumon --interval 500 --track vrchat.exe --stdout\n"
         "       500 "
         "msずつ、vrchat.exeのGPU負荷を記録しながら、コンソールに出力します。\n"
         "\n"
         "次は、\"--track\" "
         "が入力されない場合に追跡するプログラム名のリストです：\n";

  for (auto &p : kDefaultTargets) {
    nw::cout << "　・" << p.name << ": " << p.description << "\n";
  }
}

ParseResult ParseArgs(int argc, char *argv[], Config &cfg) {
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];

    auto takeArgValue = [&]() -> const char * {
      if (i + 1 >= argc) {
        return nullptr;
      }
      // increment i
      return argv[++i];
    };

    // Read and exec by given arg
    if (arg == "-h" || arg == "--help") {
      PrintUsage();
      return ParseResult::Exit;
    } else if (arg == "--interval") {
      const char *v = takeArgValue();
      if (!v) {
        nw::cerr << "--interval: 数値を入力してください。（最低限："
                 << kMinIntervalMs << "）\n";
        return ParseResult::Error;
      }

      if (!ParseUInt(v, cfg.intervalMs) || cfg.intervalMs < kMinIntervalMs) {
        nw::cerr << "--interval: 有効な数値を入力してください。（最低限："
                 << kMinIntervalMs << "、入力された数値：" << v << "）\n";
        return ParseResult::Error;
      }
    } else if (arg == "--out-dir") {
      const char *v = takeArgValue();
      if (!v || (*v == '-' && *(v + 1) == '-')) {
        nw::cerr << "--out-dir: ファイルパスを設定してください。\n";
        return ParseResult::Error;
      }

      cfg.outDir = v;
    } else if (arg == "--make-dir") {
      gMakeDir = true;
    } else if (arg == "--track") {
      int taken = 0;
      while (i + 1 < argc && argv[i + 1][0] != '-') {
        const char *v = argv[++i];
        // if it's the last str, throw error
        if (*v == '\0') {
          nw::cerr << "--track: 追跡するプログラム名を記入してください。\n";
          return ParseResult::Error;
        }
        AddTarget(cfg.targets, v);
        ++taken;
      }

      if (taken == 0) {
        nw::cerr << "--track: 追跡するプログラム名を記入してください。\n";
        return ParseResult::Error;
      }
    } else if (arg == "--stdout") {
      cfg.toStdout = true;
    } else {
      nw::cerr << arg << ": 処理不可のオプションがありました。\n";

      PrintUsage();
      return ParseResult::Error;
    }
  }

  std::string resolved, err;
  if (!winfs::ResolvePath(cfg.outDir, resolved, err)) {
    nw::cerr << "--out-dir: " << err << '\n';
    return ParseResult::Error;
  }
  cfg.outDir = std::move(resolved);
  return ParseResult::Ok;
}

} // namespace

// true = Y, false = N, nullopt = input stream closed
std::optional<bool> Ask(const std::string &prompt) {
  nw::cout << prompt;
  std::string line;
  while (std::getline(nw::cin, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (line == "y" || line == "Y")
      return true;
    if (line == "n" || line == "N")
      return false;
    nw::cout << "'Y'か'N'を入力してください：";
  }
  return std::nullopt;
}

int main(int argc, char *argv[]) {
  // Validate args and put to cfg
  nw::args utf8Args(argc, argv);
  Config cfg;
  switch (ParseArgs(argc, argv, cfg)) {
  case ParseResult::Exit:
    return 0;
  case ParseResult::Error:
    return 1;
  case ParseResult::Ok:
    break;
  }

  if (cfg.targets.empty()) {
    for (const auto pDetails : kDefaultTargets)
      AddTarget(cfg.targets, pDetails.name);
    cfg.isUsingDefaultTarget = true;
  }

  // Setup
  std::optional<bool> yn{false};
  std::string err;

  switch (winfs::GetFileType(cfg.outDir)) {
  case winfs::FILETYPE_NONE: {
    if (!gMakeDir) {
      yn = Ask(std::format("'{}'"
                           "は存在しない経路です。この名前でフォルダーを新"
                           "しく生成しますか？\n（はい：y / いいえ：n）：",
                           cfg.outDir));
      if (!yn.has_value())
        return 1;
      if (!yn.value()) {
        nw::cout << "プログラムを終了します。";
        return 0;
      }
    }

    if (!winfs::EnsureDirectory(cfg.outDir, err)) {
      nw::cerr << err << '\n';
      return 1;
    }
    break;
  }

  case winfs::FILETYPE_DIR:
    break;

  case winfs::FILETYPE_FILE:
    nw::cerr << std::format(
        "'{}'はファイルです。フォルダーを指定してください。\n", cfg.outDir);
    return 1;
  }

  // wakatta
  uint32_t regVal;
  gDismissWarning = win32::reg::ReadRegVal(
      Disclaimer::kWakattaRegRoot, Disclaimer::kWakattaRegSubkey,
      Disclaimer::kWakattaRegKeyName, regVal);

  if (!gDismissWarning || regVal == 0U) {
    yn = Ask(Disclaimer::kWakattaPrompt);
    if (!yn.has_value())
      return 1;
    if (!yn.value()) {
      nw::cout << "プログラムを終了します。\n";
      return 0;
    }
    nw::cout << "承認されました。プログラムを続行します。\n";
    bool written = win32::reg::WriteRegVal(
        Disclaimer::kWakattaRegRoot, Disclaimer::kWakattaRegSubkey,
        Disclaimer::kWakattaRegKeyName, (uint32_t)1);
    if (!written) {
      nw::cerr << "レジストリ値の生成を失敗しました。\n";
      return 1;
    }
  }

  const std::string outPath = winfs::FromPath(
      winfs::ToPath(cfg.outDir) / std::format("prof_{}.log", Timestamp()));

  nw::cout << "時間間隔　　　　：　" << cfg.intervalMs << " ms\n"
           << "ファイルパス　　：　" << cfg.outDir << '\n'
           << "ログファイル名　：　" << outPath << '\n'
           << "コンソール出力　：　" << (cfg.toStdout ? "on" : "off") << '\n'
           << "追跡するプログラムのリスト"
           << (cfg.isUsingDefaultTarget ? "（デフォルト）" : "") << "：\n";

  for (const auto &t : cfg.targets)
    nw::cout << "   - " << t.name << '\n';

  return 0;
}