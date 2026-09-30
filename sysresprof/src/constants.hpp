#pragma once

#include <string>
#include <string_view>
#include <windows.h>

struct ProgramDetails {
  std::string_view name;
  std::string_view description;
};

namespace ProgramExeNames {
// clang-format off
inline constexpr ProgramDetails kVRChat                   = { .name ="VRChat.exe",                  .description = "VRChatのゲーム本体(クライアント)" };
inline constexpr ProgramDetails kVirtualDesktop_Service   = { .name ="VirtualDesktop.Service.exe",  .description = "Virtual Desktop - ヘッドセットからの接続を待ち受けるWindowsのバックグラウンドプロセス" };
inline constexpr ProgramDetails kVirtualDesktop_Streamer  = { .name ="VirtualDesktop.Streamer.exe", .description = "Virtual Desktop Streamer - PC側のストリーマー本体" };
inline constexpr ProgramDetails kSteamVR_vrserver         = { .name ="vrserver.exe",                .description = "SteamVR - デバイスのドライバを読み込み、トラッキングデータの処理やアプリとの通信を担当" };
inline constexpr ProgramDetails kSteamVR_vrmonitor        = { .name ="vrmonitor.exe",               .description = "SteamVR - vrserver.exeを起動" };
inline constexpr ProgramDetails kSteamVR_vrwebhelper      = { .name ="vrwebhelper.exe",             .description = "SteamVR - 設定画面やダッシュボートなどのUI表示訳" };
inline constexpr ProgramDetails kStreamVR_compositor      = { .name ="vrcompositer.exe",            .description = "SteamVR - VRアプリの映像を合成してヘッドセットへ出力" };
inline constexpr ProgramDetails kMeta_OVRServer_x64       = { .name ="OVRServer_x64.exe",           .description = "Meta Quest - ヘッドセットの管理、映像の合成" };
inline constexpr ProgramDetails kMeta_OVRServiceLauncher  = { .name ="OVRServiceLauncher.exe",      .description = "Meta Quest - OVRserver_x64.exeを起動する役割" };
inline constexpr ProgramDetails kMeta_OVRRedir            = { .name ="OVRRedir.exe",                .description = "Meta Quest - アプリの起動や描画をOculus側のランタイムへ振り向ける（仕様不明）" };
// clang-format on
} // namespace ProgramExeNames

namespace Disclaimer {
inline static const HKEY kWakattaRegRoot = HKEY_CURRENT_USER;
inline static const std::string kWakattaRegSubkey =
    "Software\\EKIMOCO\\sysresprof";
inline static const std::string kWakattaRegKeyName = "wakatta";

inline static const char *kWakattaPrompt =
    "\x1b[33m╔══\x1b[1;30;43m 注意 "
    "\x1b[0m\x1b["
    "33m══════════════════════════════════════════════════════════╗\x1b[0m\n"
    "\x1b[33m║\x1b[0m                                                          "
    "        \x1b[33m║\x1b[0m\n"
    "\x1b[33m║\x1b[0m  "
    "このプログラムは、\x1b[1;91m全てのプロセス\x1b["
    "0mのリソース使用率を記録します。  \x1b[33m║\x1b[0m\n"
    "\x1b[33m║\x1b[0m  そのため、記録されるログファイルに                      "
    "        \x1b[33m║\x1b[0m\n"
    "\x1b[33m║\x1b[0m  "
    "\x1b[1;91m他人に見られたくないプロセス名が含まれる\x1b[0mことがあります。 "
    "       \x1b[33m║\x1b[0m\n"
    "\x1b[33m║\x1b[0m                                                          "
    "        \x1b[33m║\x1b[0m\n"
    "\x1b[33m║\x1b[0m  これは、GPUやRAMの使用率が高くなり、                    "
    "        \x1b[33m║\x1b[0m\n"
    "\x1b[33m║\x1b[0m  万が一VRChatがクラッシュした場合に、                    "
    "        \x1b[33m║\x1b[0m\n"
    "\x1b[33m║\x1b[0m  その原因がゲームにあるかどうかを判断するためです。      "
    "        \x1b[33m║\x1b[0m\n"
    "\x1b[33m║\x1b[0m                                                          "
    "        \x1b[33m║\x1b[0m\n"
    "\x1b[33m║\x1b[0m  "
    "生成されたログファイルは、なるべく\x1b[1;91m共有しないでください。\x1b[0m "
    "       \x1b[33m║\x1b[0m\n"
    "\x1b[33m║\x1b[0m                                                          "
    "        \x1b[33m║\x1b[0m\n"
    "\x1b["
    "33m╚══════════════════════════════════════════════════════════════════╝"
    "\x1b[0m\n"
    "(承認：Y, 拒絶：N)：";

} // namespace Disclaimer