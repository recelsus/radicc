#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF' >&2
Usage:
  radicc-fzf.sh --station-id JORF [--date 0] [--date-offset 1]

Options:
  -s, --station-id <id>   Station ID passed to `radicc list`
  -d, --date <n|YYYYMMDD>  0=today, 1=yesterday, 2=2 days ago, or explicit YYYYMMDD
      --date-offset <days>  Passed through to `radicc rec --date-offset`
  -h, --help              Show this help

The script runs `radicc list --json` internally, opens fzf, asks for confirmation,
and starts `radicc rec --url ...` for the selected program.
EOF
}

require_command() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Error: $1 is required." >&2
    exit 1
  fi
}

resolve_schedule_date() {
  local value="$1"
  if [[ "${value}" =~ ^[0-9]{8}$ ]]; then
    printf '%s\n' "${value}"
    return
  fi

  if [[ ! "${value}" =~ ^[0-9]+$ ]]; then
    echo "Error: --date must be a non-negative offset or YYYYMMDD." >&2
    exit 1
  fi

  TZ=Asia/Tokyo date -d "${value} days ago" +%Y%m%d
}

require_command jq
require_command fzf
require_command radicc
require_command date

station_id=""
date_arg="0"
date_offset=""

while [ $# -gt 0 ]; do
  case "$1" in
    -s|--station-id)
      [ $# -ge 2 ] || { echo "Error: --station-id requires a value." >&2; usage; exit 1; }
      station_id="$2"
      shift 2
      ;;
    -d|--date)
      [ $# -ge 2 ] || { echo "Error: --date requires a value." >&2; usage; exit 1; }
      date_arg="$2"
      shift 2
      ;;
    --date-offset)
      [ $# -ge 2 ] || { echo "Error: --date-offset requires a value." >&2; usage; exit 1; }
      date_offset="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Error: Unknown argument: $1" >&2
      usage
      exit 1
      ;;
  esac
done

if [ -z "${station_id}" ]; then
  echo "Error: --station-id is required." >&2
  usage
  exit 1
fi

if [ -n "${date_offset}" ] && [[ ! "${date_offset}" =~ ^[0-9]+$ ]]; then
  echo "Error: --date-offset must be a non-negative integer." >&2
  exit 1
fi

schedule_date="$(resolve_schedule_date "${date_arg}")"
programs_json="$(radicc list --station-id "${station_id}" --date "${schedule_date}" --json)"

selection="$(
  printf '%s\n' "${programs_json}" \
    | jq -r '
      reverse[]
      | [
          (.ft[8:10] + ":" + .ft[10:12] + "-" + .to[8:10] + ":" + .to[10:12]),
          .title,
          (.pfm // ""),
          (.url // ""),
          (@base64)
        ]
      | @tsv
    ' \
    | fzf \
        --delimiter=$'\t' \
        --with-nth=1,2,3,4 \
        --preview 'printf "time: %s\n\ntitle: %s\n\npfm: %s\n\nurl: %s\n" {1} {2} {3} {4}' \
        --preview-window='down,4,wrap'
)"

[ -n "${selection}" ] || exit 130

selected_json="$(
  printf '%s\n' "${selection}" \
    | awk -F '\t' '{print $5}' \
    | base64 --decode
)"

title="$(printf '%s\n' "${selected_json}" | jq -r '.title // ""')"
time_range="$(printf '%s\n' "${selected_json}" | jq -r '(.ft[8:10] + ":" + .ft[10:12] + "-" + .to[8:10] + ":" + .to[10:12])')"
url="$(printf '%s\n' "${selected_json}" | jq -r '.url // empty')"

if [ -z "${url}" ]; then
  echo "Error: selected item does not contain a url." >&2
  exit 1
fi

printf 'Date: %s\n' "${schedule_date}" > /dev/tty
if [ -n "${date_offset}" ]; then
  printf 'Date Offset: %s\n' "${date_offset}" > /dev/tty
fi
printf 'Selected: %s | %s\n' "${time_range}" "${title}" > /dev/tty
printf 'URL: %s\n' "${url}" > /dev/tty
printf 'Start recording? [y/N]: ' > /dev/tty
read -r confirm < /dev/tty

case "${confirm}" in
  y|Y)
    if [ -n "${date_offset}" ]; then
      exec radicc rec --url "${url}" --date-offset "${date_offset}"
    fi
    exec radicc rec --url "${url}"
    ;;
  *)
    echo "Cancelled." > /dev/tty
    exit 0
    ;;
esac
