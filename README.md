# Radicc

A lightweight Radiko utility with three subcommands:

- `rec`: record a program to m4a
- `fetch`: resolve program information without recording
- `list`: list one day of schedule for a station

Recording uses FFmpeg libraries (`libavformat` / `libavcodec` / `libavutil`) for remuxing and metadata. Cover art is embedded when possible and falls back to audio-only when not.

## Dependencies

- FFmpeg libraries and pkg-config
  - Arch: `sudo pacman -S ffmpeg`
  - Ubuntu/Debian: `sudo apt-get install -y libavformat-dev libavcodec-dev libavutil-dev pkg-config`
  - macOS: `brew install ffmpeg pkg-config`

## Build

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
```

The build fails if libav* are not found (CLI fallback is removed).

## .env (optional)

Radiko login settings are optional. Search order:

1. `$XDG_CONFIG_HOME/radicc`
2. `~/.config/radicc`

```bash
RADIKO_USER=you@example.com
RADIKO_PASS=yourpassword
```

Example environment variable:

```bash
export RADICC_DIR="$HOME/Radiko/"
```

`RADICC_DIR` is the default output root when no higher-priority path is provided.

## TOML (recurring)

Minimal fields are `title` (section name) and `station`.

- Global: `base_dir` sets a common output root for all entries
- Per-entry: `dir` is a subdirectory under `base_dir`
- Per-entry: `filename` is the filename base; `-YYYYMMDD.m4a` is appended automatically
- Per-entry: `date_offset` adjusts the filename date only
- Per-entry: `img` / `pfm` are fallbacks used only if not fetched

```toml
base_dir = "$HOME/Radiko"

[example]            # select via -t example
id = "my-program"    # or select via -i my-program
title = "Program Title"
station = "JORF"
img = ""             # used only if no image fetched
pfm = ""             # used only if no pfm fetched
date_offset = 0      # adjust filename date only (days; e.g., 1)
dir = ""             # subdirectory under base_dir; only used when explicitly set
filename = ""        # defaults to <title>-YYYYMMDD.m4a (suffix always appended)
```

- Discovery: `title + station` → weekly XML → nearest past entry (to <= now)
- Fetched: `id/title/pfm/ft/to/img`. Save-time priority: pfm/img → fetched > TOML; dir/filename → TOML if present; album is always title.

## Output Path Priority

Output root/path is resolved in this order:

1. `-o` explicit path
2. TOML global `base_dir`
3. `RADICC_DIR`
4. current directory `./`

Examples:

- `-o my_pick`
  Writes under the default output root as `my_pick-YYYYMMDD.m4a`
- `-o ./`
  Writes in the current directory
- `-o ./file.m4a`
  Writes exactly to `./file.m4a`
- `-o /path/to/file.m4a`
  Writes exactly to that file path

Parent directories are created automatically. A subdirectory is created only when `dir` or a directory path in `-o` is explicitly provided.

## Subcommands

### `rec`

Record a program from TOML or a Radiko timeshift URL.

```bash
./radicc rec -t example
./radicc rec --url 'https://radiko.jp/#!/ts/FMO/20250920140000'
./radicc rec --url 'https://radiko.jp/#!/ts/FMO/20250920140000' --date-offset 1
```

### `fetch`

Resolve program info without recording. This is the old `--fetch` behavior.

```bash
./radicc fetch -t example
./radicc fetch --url 'https://radiko.jp/#!/ts/FMO/20250920140000'
./radicc fetch --url 'https://radiko.jp/#!/ts/FMO/20250920140000' --json
```

### `list`

List one day of programs for a station.

```bash
./radicc list --station-id JORF
./radicc list --station-id JORF --date 20260327
./radicc list --station-id JORF --date 20260327 --json
```

- If `--date` is omitted, `radicc` tries the current JST date first, then the previous day, then the next day
- Normal output prints a formatted list
- `--json` prints an array suitable for tools like `fzf`
- Each item includes a timefree URL in the form `https://radiko.jp/#!/ts/{station_id}/{ft}`

## Command Options

### `rec` / `fetch`

- `-t, --target <section>`: select TOML section
- `-i, --id <id>`: select by `id` in TOML
- `-u, --url <url>`: one-off URL mode
- `-o, --output <name-or-path>`: filename base, directory override, or explicit file path
- `-d, --duration <min>`: override duration in minutes
- `--date-offset <days>`: shift the filename date backward
- `--json`: print resolved result as JSON

### `list`

- `--station-id <id>`: station id
- `--date <YYYYMMDD>`: target date, max one day
- `--json`: print raw JSON array

## Notes

- Album is always the resolved title; artist is pfm (fetched when available, empty otherwise).
- Library logging is reduced to errors to avoid noisy per-segment messages.
- See `FLOW.md` and `FLOW-ja.md` for detailed flow, required/optional fields, and priorities.

## Server

`radicc-server` is a small HTTP wrapper intended for personal use.

Start:
```bash
./radicc-server --port 8080
```

Health check:
```bash
curl http://127.0.0.1:8080/health
```

Record request:
```bash
curl -X POST \
  -H 'Content-Type: application/json' \
  -d '{"url":"https://radiko.jp/#!/ts/STATION/YYYYMMDDHHMMSS","date_offset":1}' \
  http://127.0.0.1:8080/record
```

Example response:
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

Download:
```bash
curl -OJ http://127.0.0.1:8080/download/d18b6740d8fb41d2
```
