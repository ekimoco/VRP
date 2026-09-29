# **resprof**
### VRChat想定GPU負荷測定プログラム

VRChatやVR関係プログラムで生じるGPUやRAMの負荷を測定し、それぞれの数値をログに書き込みます。

<details>
  <summary>測定要素</summary>
  <ul>
    <li>3D使用率</li>
    <li>専用GPUメモリ (VRAM)</li>
    <li>共有GPUメモリ（Shared Memory）</li>
    <li>GPUの温度</li>
  </ul>
</details>

### プロジェクト構造
`resprof/`<br>
│<br>
├─`src/`：プロゴラムのソースコード <br>
│　├─　`constants.hpp`：VR関連プログラム名のまとめ<br>
│　├─　`main.cpp`：実引数の処理や、サンプリングの開始を担当<br>
│　└─　`lib/`：コード整理<br>
│　　　　├─　`gpu.hpp`：GPUへの負荷を測定 (未完成)<br>
│　　　　├─　`process.hpp`：プロセスの名前やステータスをリアルタイムで読み込む<br>
│　　　　├─　`win32.hpp`：WIN32API関連コード<br>
│　　　　├─　`winfs.hpp`：Windowsでのファイルシステム管理コード<br>
│　　　　└─　`impl/...`：上の `.hpp`ファイルそれぞれの実際の実行コード<br>
│<br>
├─　`.clang-format`：コード書きの形式を指定（インデント、括弧の置き場、等）<br>
├─　`.clang-tidy`：静的解釈ルールやコーディング規約を設定<br>
├─　`.gitattributes`：gitでのプロジェクトファイルの扱い方を設定<br>
├─　`.gitignore`：Gitでアップロードしないファイルやフォルダーを指定<br>
│<br>
├─　`CMakeLists.txt`：`CMake`向けビルド設定ファイル<br>
├─　`CMakePresets.json`：ビルド時に使用する環境設定ファイル<br>
├─　`README.md`：_今読んでるよ！_<br>
│<br>
├─　`run.bat`：ビルドを行い、プログラムを実行する。与えられた引数を注入する<br>
├─　`run.sh`：`run.bat`と機能的に同じ。Bashやシェルターミナルでの実行用<br>
└─　`vcpkg.json`：パッケージマネージャの`vcpkg`の設定ファイル<br>

[!IMPORTANT]
> 2026/9/30の時点、Windowsロケール（言語）が**英語に設定**されたことを想定して作成しました。後で対応言語を増やすつもりなので、できるまで少々お待ちください。

# TODO
## プロファイラー
### 初期設定
- [X] vspck設定
- [X] 引数読み込み及び処理
- [X] 初期実行時、警告文句を表示
  - [X] 承認したら、レジストリに登録 (`HKEY_CURRENT_USER\Software\EKIMOCO\prof\wakatta\`, `DWORD`)

### サンプリング
- [ ] GPUとRAMの負荷のサンプリング及びログ記録
- [ ] VRChat起動の時、EACに触らないかチェック

### VRChatと同期化
- [ ] VRChat起動中に`%LocalAppDataLow&\VRChat\VRChat\output_log_yyyy-MM-dd_HH-mm-ss.txt`（以下「VRCログ」）で出力されるログをtail
  - [ ] ユーザーIDを取得
  - [ ] ユーザーのフレンドたちのIDを取得（アバター表示条件の参照のため）
- [ ] VRかデスクトップか把握
- [ ] レジストリのキーと値の読み込み：
  - [ ] FOV（視野角）: `FIELD_OF_VIEW_h{hash}` - Little-Endian IEEE 754 Float (`QWORD`)
  - [ ] 「アバターの最適化」→ <最適化されてにないアバターのブロック>：`VRC_AVATAR_PERFORMANCE_RATING_MINIMUM_TO_DISPLAY`:
    - 3：Poor以下
    - 4：Very Poor
    - 5：ブロックしない
  - [ ] 「アバターの最適化」→ <最大ダウンロードサイズ>：`VRC_AVATAR_MAXIMUM_DOWMLOAD_SIZE_h{hash}` - `DWORD`（単位：Bytes）
  - [ ] 「アバターの最適化」→ <最大非圧縮サイズ>：`VRC_AVATAR_MAXIMUM_UNCOMPRESSED_SIZE_h{hash}` - `DWORD`（単位：Bytes）
  - [ ] 「アバターのカリング」→ <アバターを表示する距離> ON/OFF：`{userId}_avatarProxyShowAtRangeToggle_h{hash}` - `0`, `1` (`DWORD`)
  - [ ] 「アバターのカリング」→ <アバターを表示する距離>　数値：`{userId}_avatarProxyShowAtRange_{hash}` - Little-Endian IEEE 754 Float (`QWORD`)
  - [ ] 「アバターのカリング」→ <アバターを表示する距離> ON/OFF：`{userId}_currentShowMaxNumberOfAvatarsEnabled_h{hash}`
  - [ ] 「アバターのカリング」→ <表示するアバターの数>　数値：`{userId}_avatarProxyShowMaxNumber_h{hash}` - 64-bit 定数 (`QWORD`)
  - [ ] 「アバターのカリング」→ <フレンドのアバターを常に表示>：`{userId}_avatarProxyAlwaysShowFriends_h{hash}`
  - [ ] 「アバターのカリング」→ <個別に表示したアバターを常に表示>：`{userId}_avatarProxyAlwaysShowExplicit_h{hash}`
  - [ ] 「シールドレベル」：`VRC_SAFETY_LEVEL_h{hash}`
    - 0: Maximum
    - 2: Normal
    - 4: None
    - 5: Custom
  - [ ] （内部設定・UI上にて操作不可）アバターとの距離の程度ででダウンロードを優先 ON/OFF：`VRC_DOWNLOAD_PRIORITIZE_DISTANCE_ENABLED` - `0`, `1` (`DWORD`)
  - [ ] （内部設定・UI上にて操作不可）アバターとの距離の程度ででダウンロードを優先 距離数値：`VRC_PRIORITIZE_DOWNLOAD_DISTANCE_h{hash}` - Little-Endian IEEE 754 Float  
  - [ ] （内部設定・UI上にて操作不可）フレンドのアバターのダウンロードを優先：`VRC_PRIORITIZE_FRIEND_DOWNLOAD_h{hash}` - `0`, `1` (`DWORD`)
  - [ ] （内部設定・UI上にて操作不可）個別に表示したアバターのダウンロードを優先：`VRC_PRIORITIZE_MANUAL_DOWNLOADS_h{hash}` - `0`, `1` (`DWORD`)
  - [ ] （他に見探したら追加する）
- [ ] ログの解釈ロジック、VRChatでのステータス追跡 (VRCXはこうやってるからセーフかも)
- [ ] ワールド指定パラメータ作成 (`--wrld-id`)
- [ ] 下のイベンドの開始時、記録する
  - 指定されたワールドジョイン
  - アバター解凍
  - 他ユーザーのジョイン
  - 他ユーザーの視認性 ()

## テレメトリー
- [ ] `Debug.Log`での出力が、VRCログまで届くか確認
- [ ] ユーザーの位置を追いかける透明シリンダーオブジェクト「`RemoteUserPos`」スクリプト作成
- [ ] ローカルユーザーのカリングなどのアバター表示条件を確認
- [ ] 下のイベンドの開始時、記録する
  - [ ] ローカルユーザーの頭のPitchとYawをFOVに代入して、いずれかの`RemoteUserPos`が、いつからいつまで画面に映してるのか
    - `RemoteUserPos`の`isVisible`値のを参照
  - [ ] いずれかのワールドオブジェクトが、いつからいつまで画面に映してるのか

# Python
- [ ] ログファイルを読み込む
- [ ] ログファイルと、各イベントに時間帯を連関さえる。
- [ ] GUIやグラッフで見やすくまとめる。

> このリストは未だ企画中です

# 履歴
記録予定