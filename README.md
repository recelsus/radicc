# Radicc

A lightweight Radiko recorder that generates m4a from either a scheduled TOML entry or a one-off URL. Uses FFmpeg libraries (libavformat/libavcodec/libavutil) by default for remuxing and metadata; embeds cover art (attached_pic) when possible, and falls back to audio-only if not.

- TOML (recurring): title + station, with optional dir/filename/date_offset overrides
- URL (one-off): station + ft parsed from URL (TOML is not consulted)
- Library-based pipeline (default): HLS + Radiko headers → AAC copy + aac_adtstoasc → MP4/M4A, artist/album metadata, best-effort cover art

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

Searched in `$XDG_CONFIG_HOME/radicc` or `~/.config/radicc`.

```bash
RADIKO_USER=you@example.com
RADIKO_PASS=yourpassword
OUTPUT_DIR="$HOME/Radiko"
```

## TOML (recurring)

Minimal fields: `title` (section name) and `station`.
Optional overrides: `dir` (folder), `filename` (base; `-YYYYMMDD.m4a` is always appended), `date_offset` (filename date only), and fallbacks `img`/`pfm` (used only if not fetched).

```toml
[example]            # select via -t example (section name acts as title)
id = "my-program"    # or select via -i my-program (id acts as title)
title = "Program Title"
station = "JORF"
img = ""             # used only if no image fetched
pfm = ""             # used only if no pfm fetched
date_offset = 0      # adjust filename date only (days; e.g., 1)
dir = ""             # defaults to title
filename = ""        # defaults to <title>-YYYYMMDD.m4a (suffix always appended)
```

- Discovery: `title + station` → weekly XML → nearest past entry (to <= now)
- Fetched: `id/title/pfm/ft/to/img`. Save-time priority: pfm/img → fetched > TOML; dir/filename → TOML if present; album is always title.

## URL mode (one-off)

TOML is ignored entirely. The URL provides `station` and `ft`; we match the exact entry in the weekly XML and record it.

```bash
./radicc --url 'https://radiko.jp/#!/ts/FMO/20250920140000' -o clips/my_pick
```

- `-o` accepts a base name or relative path; `-YYYYMMDD.m4a` is always appended. Parent directories are created automatically.
- Cover art is embedded when possible; otherwise the file is written as audio-only.

## CLI Options

- `-t, --target <section>`: select TOML section (preferred)
- `-i, --id <id>`: select by `id` in TOML (fallback)
- `-u, --url <url>`: URL mode (ignores TOML)
- `-o, --output <name-or-path>`: base name or relative path (`-YYYYMMDD.m4a` appended)
- `-d, --duration <min>`: override duration in minutes
- `--dry-run`: resolve and print only; do not record

## Notes

- Album is always the resolved title; artist is pfm (fetched when available, empty otherwise).
- Library logging is reduced to errors to avoid noisy per-segment messages.
- See `FLOW.md` and `FLOW-ja.md` for detailed flow, required/optional fields, and priorities.

