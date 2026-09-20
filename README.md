# KURUI (TRIMUI Model S 向け超軽量カスタムランチャー)

TRIMUI Model S (Powkiddy A66) 向けに、C言語と SDL 1.2 のみで動作する超軽量・単機能なミニマル・インダストリアルランチャーです。

Teenage Engineering 製品のようなソリッドなデザインと、ゲームボーイ風の起動シーケンス（矩形波ピコーンシンセ音同期）を搭載しています。

---

## 主な特徴

- **超軽量＆ゼロ依存**: 外部フォントライブラリ（SDL_ttf）やオーディオライブラリ（SDL_mixer）、OpenGLは一切使わず、SDL 1.2 コアのみで動作。
- **メモリ安全設計**: メインループ内での動的メモリ確保（`malloc`/`free`）を完全排除。
- **GB風ブート演出 ＆ 矩形波シンセ**:
  - 画面上部からドットロゴが落下・停止。
  - SDL標準オーディオコールバック（`AUDIO_U8`, 22050Hz, バッファ256サンプル）によるリアルタイム矩形波合成（C6 → C7 減衰音）を遅延ゼロで発音。
- **美咲フォント準拠 8x8 ビットマップ**:
  - ASCII 95文字＋日本語（ひらがな、カタカナ、主要漢字）を 8x8 ドットでピクセルパーフェクト描画。
- **実機(16bit RGB565) / PC(32bit) 両対応**:
  - `SDL_MapRGB` とサーフェス深度判定により環境差を吸収。
  - FPS制御（`SDL_Delay`）によりCPU負荷を適正化。
- **クリーンなプロセス起動**:
  - ゲーム起動時は `SDL_Quit()` でSDLリソースを完全破棄してからシェルコマンドを実行し、復帰時に安全に再初期化。

---

## ディレクトリ構成

```text
kurui/
├── Makefile                     # PCビルド (make) / クロスコンパイル対応
├── .github/workflows/build.yml   # GitHub Actions 自動クロスビルド＆ZIP生成
├── src/
│   ├── main.c                   # エントリポイント、メインループ、ゲーム起動
│   ├── boot.c / boot.h          # GB風ブートアニメーション演出
│   ├── sound.c / sound.h        # リアルタイム矩形波シンセ（ピコーン音）
│   ├── font.c / font.h          # 8x8 ビットマップフォント描画・描画プリミティブ
│   └── font_data.h              # ASCIIおよび日本語 8x8 ビットマップ配列
├── scripts/
│   ├── trimui_init.sh           # SDカード直下配置用、起動フックスクリプト
│   └── update.sh                # 公式アップデート検知用インストーラースクリプト
└── README.md                    # 本書
```

---

## 操作方法

| 操作 | 実機 (TRIMUI Model S) | PC テスト環境 |
| :--- | :--- | :--- |
| **項目選択 (上下)** | 十字キー 上 / 下 | `↑` / `↓` 矢印キー |
| **Aボタン (ゲーム起動)** | `A (LCTRL)` | `Z` キー / `Enter` キー |
| **Bボタン (電源OFF / 終了)** | `B (LALT)` | `X` キー / `ESC` キー |
| **強制終了 (安全終了)** | `START + SELECT` 同時押し | `Enter + Space` または `Enter + 右Shift` |
| **ブートスキップ** | 任意のボタン | 任意のキー |

---

## ビルド方法

### 1. PC環境でのビルド & テスト (Linux / macOS / WSL)

SDL 1.2 開発パッケージが必要です。

```bash
# Ubuntu / Debian / WSL
sudo apt-get update
sudo apt-get install build-essential libsdl1.2-dev

# ビルド
make

# 実行 (320x240 ウィンドウで起動)
./kurui
```

### 2. TRIMUI実機向けクロスコンパイル (ARMv5TE)

```bash
# ツールチェーンとarmel版SDL1.2を指定してビルド
make CC=arm-linux-gnueabi-gcc \
     SDL_CFLAGS="-I/usr/include/SDL -I/usr/include/arm-linux-gnueabi/SDL -D_GNU_SOURCE=1 -D_REENTRANT" \
     SDL_LIBS="-L/usr/lib/arm-linux-gnueabi -lSDL -lpthread -lm"

# バイナリサイズを最小化
make strip CROSS_COMPILE=arm-linux-gnueabi-
```

### 3. GitHub Actions による自動ビルド & バージョニング

リポジトリの `main` ブランチにプッシュすると、`.github/workflows/build.yml` が自動実行されます。

- **バージョン規則**:
  - リポジトリ直下の `VERSION` ファイルに第1・第2オクテット（例: `0.0`）が定義されています。
  - GitHub Actions が既存の Git タグ（`v0.0.*`）を検索し、**第3オクテット（パッチ番号）を自動で +1 してインクリメント**します（初回は `v0.0.1`）。
  - **メジャー・マイナーバージョンを上げる場合**: `VERSION` ファイルの内容を手動で `0.1` や `1.0` に変更してプッシュしてください。次回リリースは自動的に `v0.1.1` や `v1.0.1` からインクリメントされます。
- **自動リリース**:
  - Git タグが自動付与され、GitHub Releases に実機用バイナリとインストーラーを含んだ `trimui_kurui.zip` が自動公開されます。

---

## 実機への導入手順

### 方法A: 自動インストーラーを使う場合（推奨）
1. 配布パッケージ（`trimui_kurui.zip`）を解凍します。
2. 解凍されたファイル群（`updater`, `trimui_init.sh`, `apps/`）を MicroSD カードの**最上位（ルート）**にコピーします。
   ※ 公式アップデート検知（`updater`）により、本体起動時に自動実行されます。
3. MicroSD カードを TRIMUI Model S に挿入し、電源を入れます。
4. アップデート画面が走り、自動的に再起動して KURUI が起動します。

### 方法B: 手動で配置する場合
1. MicroSD カード内に以下のパスで配置します:
   - `/mnt/SDCARD/apps/kurui/kurui` (実行バイナリ)
   - `/mnt/SDCARD/trimui_init.sh` (起動フックスクリプト)
2. 本体を起動すると、内蔵メニューの代わりに KURUI が優先起動します。
   （元の公式メニューに戻したい場合は、SDカード直下の `trimui_init.sh` を削除またはリネームしてください）

---

## ゲームリストのカスタマイズ

[src/main.c](file:///src/main.c) 内の `g_games` 配列を編集することで、表示するゲームや起動コマンドを自由に変更できます。

```c
static const GameEntry g_games[] = {
    {"[GB]",   "テトリス",           "/mnt/SDCARD/Emus/gb/launch.sh \"/mnt/SDCARD/Roms/gb/tetris.gb\"", "echo [LAUNCH] GB Tetris"},
    {"[FC]",   "スーパーマリオ",     "/mnt/SDCARD/Emus/fc/launch.sh \"/mnt/SDCARD/Roms/fc/mario.nes\"", "echo [LAUNCH] FC Super Mario"},
    // お好みのエミュレータとROMパスを追加
};
```