#include "constants.hpp"
#include "lib/win32/fs.hpp"
#include "lib/win32/global.hpp"
#include "lib/win32/hw.hpp"
#include "lib/win32/literals.hpp"
#include "lib/win32/proc.hpp"
#include "lib/win32/reg.hpp"

#include <boost/nowide/args.hpp>
#include <boost/nowide/iostream.hpp>

#include <algorithm>
#include <atomic>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace nw = boost::nowide;

namespace {
constexpr uint32_t kDefaultIntervalMs = 1000;
constexpr uint32_t kMinIntervalMs = 50;

//== String Helpers ==//
std::string FormatGiB(uint64_t bytes) {
  return std::format("{:.2f} GiB", bytes / double(1ull << 30));
}

// Same format Windows uses in GPU counter names: luid_0x00000000_0x0000DDD6
std::string FormatLuid(uint64_t luid) {
  return std::format("0x{:08X}_0x{:08X}", uint32_t(luid >> 32), uint32_t(luid));
}

std::string_view VendorName(uint32_t id) {
  switch (id) {
  case 0x10DE:
    return "NVIDIA";
  case 0x1002:
    return "AMD";
  case 0x8086:
    return "Intel";
  default:
    return "不明";
  }
}

std::string_view KindLabel(win32::hw::profile::GpuKind k) {
  switch (k) {
  case win32::hw::profile::GpuKind::Discrete:
    return "ディスクリート（独立）";
  case win32::hw::profile::GpuKind::Integrated:
    return "内蔵";
  default:
    return "不明";
  }
}

//== States and constants ==//

std::atomic<bool> gRunning{false};
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
         "    --interval <ミリ秒>           サンプリングの時間間隔 "
         "（単位：ミリ秒・1000分の1秒）を指定します。 \n"
         "　　　　　　　　　　　　　（デフォルト："
      << kDefaultIntervalMs << " ms, 最小 " << kMinIntervalMs
      << " ms）\n"
         "\n"
         "    --out-dir <path>              "
         "ログのファイルのファイルパスを指定します。（デフォルト：現在位置）\n"
         "\n"
         "    --make-dir                    "
         "指定されたフォルダーが存在しない場合、新しく生成します。\n"
         "\n"
         "    --gpu <index>                 "
         "記録対象のGPU（グラフィックスカード）を指定します。\n"
         "\n"
         "    --track-disk <label>          "
         "Disk IO（ディスクに対する入出力）の対象ディスクを指定します。"
         "\n"
         "    -h, --help                    このヘルプを出力します。\n";
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
    } else {
      nw::cerr << arg << ": 処理不可のオプションがありました。\n";

      PrintUsage();
      return ParseResult::Error;
    }
  }

  std::string resolved, err;
  if (!win32::fs::ResolvePath(cfg.outDir, resolved, err)) {
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
    if (line == "y" || line == "Y" || line == "ｙ" || line == "Ｙ")
      return true;
    if (line == "n" || line == "N" || line == "ｎ" || line == "Ｎ")
      return false;
    nw::cout << "「Y」か「N」を入力してください：";
  }
  return std::nullopt;
}

int main(int argc, char *argv[]) {
  // Check if the program running elsewhere
  std::vector<win32::proc::ProcessEntry> vProc;
  std::string err;

  if (!win32::proc::EnumerateProcesses(vProc, err)) {
    nw::cout << err;
    return 1;
  }

  std::string programName = win32::fs::ToPath((argv[0])).filename().string();
  auto count = win32::proc::CountProcess(programName, err);

  if (!count) {
    nw::cout << std::format("Failed counting program '{}': {}", programName,
                            err);
    return 1;
  }

  if (count >= 2) {
    nw::cout << "すでに実行されています。";
    return 0;
  }

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

  // Setup
  std::optional<bool> yn{false};

  switch (win32::fs::GetFileType(cfg.outDir)) {
  case win32::fs::FILETYPE_NONE: {
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

    if (!win32::fs::EnsureDirectory(cfg.outDir, err)) {
      nw::cerr << err << '\n';
      return 1;
    }
    break;
  }

  case win32::fs::FILETYPE_DIR:
    break;

  case win32::fs::FILETYPE_FILE:
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

  const std::string outPath = win32::fs::FromPath(
      win32::fs::ToPath(cfg.outDir) / std::format("prof_{}.log", Timestamp()));

  nw::cout << "時間間隔　　　　：　" << cfg.intervalMs << " ms\n"
           << "ログファイル名　：　" << outPath << '\n';

  nw::cout << "\n[ハードウェアプロフィール]\n";
  auto profile = win32::hw::profile::GetProfile(err);
  nw::cout << "- CPUの名前　　　：" << profile.cpuName << "\n"
           << "- 論理コアの個数 ：" << profile.logicalCores << "\n"
           << "- GPUの個数　　　：" << profile.gpuProfiles.size() << "\n";

  if (profile.gpuProfiles.size() > 0) {
    for (const auto &g : profile.gpuProfiles) {
      nw::cout << std::format("- GPU #{} ({})\n"
                              "    - 種類：{}\n"
                              "    - LUID：\n",
                              g.index + 1, g.gpuName, KindLabel(g.kind));
      for (auto luid : g.luids)
        nw::cout << "        - " << FormatLuid(luid) << "\n";

      nw::cout << std::format(
          "    - ベンダー：{} (0x{:04X})\n"
          "    - 専用GPUメモリ：{}\n"
          "    - 共有GPUメモリ：{}\n"
          "    - 専用システムメモリ：{}\n"
          "    - 実用可能なメモリ：{}\n\n",
          VendorName(g.vendorId), g.vendorId, FormatGiB(g.capDedicatedRam),
          FormatGiB(g.capSharedRam), FormatGiB(g.capDedicatedSystemRam),
          FormatGiB(g.EffectiveCap()));
    }
  }

  nw::cout << "サンプリングを始まります。";

  return 0;
}