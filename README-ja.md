※ このファイルは日本語版のREADMEです。英語版は `README.md` を参照してください。

# Radicc

## 概要

- TOML（定期予約）または URL（単発録音）から、Radiko の番組を録音して m4a を生成します。
- デフォルトで FFmpeg ライブラリ（libavformat/libavcodec/libavutil）を利用してリマックスします。
- 画像（attached_pic）は可能な限り埋め込み（取得できない/不整合の場合は音声のみで継続）。

## 依存

- FFmpeg ライブラリ（libavformat, libavcodec, libavutil）と pkg-config
  - Arch: `sudo pacman -S ffmpeg`
  - Ubuntu/Debian: `sudo apt-get install -y libavformat-dev libavcodec-dev libavutil-dev pkg-config`
  - macOS: `brew install ffmpeg pkg-config`

## ビルド

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
```

## 設定（.env）

Radikoログイン情報など（任意）を `.env` または `env` に記述します。探索場所:

1. `$XDG_CONFIG_HOME/radicc`
2. `~/.config/radicc`

例:
```bash
RADIKO_USER=you@example.com
RADIKO_PASS=yourpassword
OUTPUT_DIR="$HOME/Radiko"
```

## 設定（radicc.toml: 定期予約）

最小構成は `title`（セクション名）と `station`。オプションで `dir`（ディレクトリ名）と `filename`（ファイル名ベース）、`date_offset`（ファイル名日付の補正）を指定可能です。

```toml
[example]            # -t example で指定（セクション名=title相当）
id = "my-program"    # -i my-program （id=title相当）
title = "Program Title"
station = "JORF"
img = ""             # 画像URL（取得できなかった場合のみ使用）
pfm = ""             # パーソナリティ（取得できなかった場合のみ使用）
date_offset = 0      # ファイル名の日付のみ補正（日数; 例: 1）
dir = ""             # 省略時は title
filename = ""        # 省略時は <title>-YYYYMMDD.m4a（必ず -YYYYMMDD.m4a が付与）
```

- 検索: `title + station` で weekly XML から最も近い過去回（to <= now）を特定。
- 取得: `id/title/pfm/ft/to/img`。保存時は `pfm/img` は取得 > TOML フォールバック、`dir/filename` はTOML優先、albumはtitle固定。

## URLモード（単発録音）

TOMLは一切参照しません。URLから `station` と `ft` を抽出し、weekly XML で完全一致の番組を特定して録音します。

```bash
./radicc --url 'https://radiko.jp/#!/ts/FMO/20250920140000' -o clips/my_pick
```

- `-o` はベース名または相対パスを指定可（`clips/my_pick-YYYYMMDD.m4a`）。親ディレクトリは自動作成。
- attached_pic は可能な場合に埋め込み。失敗時は音声のみで継続。

## CLI オプション

- `-t, --target <section>`: TOMLセクション名で指定（優先）
- `-i, --id <id>`: TOMLの `id` で指定（フォールバック）
- `-u, --url <url>`: URLモード（TOML非参照）
- `-o, --output <name-or-path>`: 出力のベース名または相対パス（`-YYYYMMDD.m4a` 付与）
- `-d, --duration <min>`: 録音尺の上書き（必要に応じて）
- `--dry-run`: 取得・解決のみ（録音なし）

## メモ

- ライブラリ経路は音声リマックス/メタ埋め込み/カバー画像（可能な限り）を行います。
- 画像・pfmが無い場合でも動作は継続します（音声のみ/artistなし）。

