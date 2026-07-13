# Third-Party Notices

Radicc is licensed under the MIT License. See `LICENSE`.

## FFmpeg

Radicc links to FFmpeg libraries provided by the operating system or package manager:

- `libavformat`
- `libavcodec`
- `libavutil`

FFmpeg is not vendored, bundled, or statically linked by this project. Homebrew and
Arch Linux packages should declare FFmpeg as an external dependency.

FFmpeg is distributed under its own licence terms. See:

- https://ffmpeg.org/legal.html
- https://ffmpeg.org/

## toml++

Radicc includes the single-header distribution of toml++ in `third_party/tomlplusplus/toml.hpp`.

toml++ is licensed under the MIT License.

Project:

- https://github.com/marzer/tomlplusplus
