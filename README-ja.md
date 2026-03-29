# Radicc

## Description

- `rec`: 録音
- `fetch`: 録音せず番組情報だけ解決
- `list`: 局IDと日付で1日分の番組表を取得

- TOML(定期予約)または URL(単発録音)から、Radikoタイムフリーの番組を録音してm4aを生成します。
- 画像(attached_pic)は可能な限り埋め込み(取得できない/不整合の場合は音声のみで継続)。

## Dependencies

- FFmpeg ライブラリ（libavformat, libavcodec, libavutil）と pkg-config
  - Arch: `sudo pacman -S ffmpeg`
  - Ubuntu/Debian: `sudo apt-get install -y libavformat-dev libavcodec-dev libavutil-dev pkg-config`
  - macOS: `brew install ffmpeg pkg-config`

## Build

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
```

## Config（.env）

Radikoログイン情報など(任意)を `.env` または `env` に記述します。探索場所:

1. `$XDG_CONFIG_HOME/radicc`
2. `~/.config/radicc`

例: .env
```bash
RADIKO_USER=you@example.com
RADIKO_PASS=yourpassword
```

例: 環境変数
```
export RADICC_DIR=$HOME/Radiko/
```

`RADICC_DIR` は、より高い優先順位の保存先指定がない場合の既定保存先です。

## Config(radicc.toml: 定期予約)

最小構成は `title`（セクション名）と `station` です。

- グローバル `base_dir`: 全番組共通の保存先ルート
- 各番組の `dir`: `base_dir` 配下のサブディレクトリ
- 各番組の `filename`: ファイル名ベース
- 各番組の `date_offset`: ファイル名の日付だけ補正
- 各番組の `img` / `pfm`: 取得できなかったときのフォールバック

```toml
base_dir = "$HOME/Radiko" # optional 未設定時は実行ディレクトリ直下

[example]            # -t example で指定（セクション名=title相当）
id = "my-program"    # -i my-program （id=title相当）
title = "Program Title"
station = "JORF"
img = ""             # 画像URL(自動取得できなかった場合のみ使用)
pfm = ""             # パーソナリティ(取得できなかった場合のみ使用)
date_offset = 0      # ファイル名の日付のみ補正(1月1日の25時放送の場合そのままでは1月2日のファイル名になってしまうため1とすることで1日前の日付に補正)
dir = ""             # base_dir 配下のサブディレクトリ。
filename = ""        # 省略時は <title>-YYYYMMDD.m4a（必ず -YYYYMMDD.m4a が付与）
```

- 検索: `title + station` で weekly XML から最も近い過去回（to <= now）を特定。
- 取得: `id/title/pfm/ft/to/img`。保存時は `pfm/img` は取得 > TOML フォールバック、`dir/filename` はTOML優先、albumはtitle固定。

## Usage

### `rec`

通常

```bash
./radicc rec -t example # tomlに設定したもの
./radicc rec --url 'https://radiko.jp/#!/ts/FMO/20250920140000' # URLから直接
./radicc rec --url 'https://radiko.jp/#!/ts/FMO/20250920140000' --date-offset 1 # URLから直接取得と日付補正
```

### `fetch`

録音せず、番組情報だけ解決します。

```bash
./radicc fetch -t example
./radicc fetch --url 'https://radiko.jp/#!/ts/FMO/20250920140000'
./radicc fetch --url 'https://radiko.jp/#!/ts/FMO/20250920140000' --json
```

### `list`

局IDと日付で1日分の番組表を取得します。

```bash
./radicc list --station-id JORF
./radicc list --station-id JORF --date 20260327
./radicc list --station-id JORF --date 20260327 --json
```

- `--date` 省略時は実行時の JST 当日
- 通常出力は整形リスト
- `--json` は `fzf` などに渡しやすい配列 JSON
- 各要素には `https://radiko.jp/#!/ts/{station_id}/{ft}` 形式のURLを作成しています

## Option

### `rec` / `fetch`

- `-t, --target <section>`: TOMLセクション名で指定(優先)
- `-i, --id <id>`: TOMLの `id` で指定(フォールバック)
- `-u, --url <url>`: URLモード(TOML非参照)
- `-o, --output <name-or-path>`: ベース名、ディレクトリ、または明示ファイルパス
- `-d, --duration <min>`: 録音尺の上書き(必要に応じて)
- `--date-offset <days>`: ファイル名の日付を過去側へ補正
- `--json`: 解決結果を JSON 出力

### `list`

- `--station-id <id>`: 局ID
- `--date <YYYYMMDD>`: 対象日付
- `--json`: JSON 配列で出力

## Server

`radicc-server` は個人利用向けの簡易 HTTP サーバーです。

起動例:
```bash
./radicc-server --port 8080
```

health check:
```bash
curl http://127.0.0.1:8080/health
```

録音リクエスト:
```bash
curl -X POST \
  -H 'Content-Type: application/json' \
  -d '{"url":"https://radiko.jp/#!/ts/STATION/YYYYMMDDHHMMSS","date_offset":1}' \
  http://127.0.0.1:8080/record
```

レスポンス例:
```json
{
  "status": "done",
  "job_id": "d18b6740d8fb41d2",
  "download_url": "/download/d18b6740d8fb41d2",
  "filepath": "/path/to/file.m4a",
  "output_file": "file.m4a",
  "title": "Program Title",
  "start_time": "20260329000000",
  "end_time": "20260329003000"
}
```

ダウンロード例:
```bash
curl -OJ http://127.0.0.1:8080/download/d18b6740d8fb41d2
```
