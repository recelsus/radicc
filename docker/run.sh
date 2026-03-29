#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
compose_file="${script_dir}/docker-compose.yml"
if [ ! -f "${compose_file}" ]; then
  echo "Error: ${compose_file} not found." >&2
  echo "Create it from docker-compose.yml.example before running this script." >&2
  exit 1
fi

confirm() {
  local prompt="$1"
  printf '%s [y/N]: ' "${prompt}"
  read -r reply
  case "${reply}" in
    y|Y) return 0 ;;
    *) echo "Cancelled."; return 1 ;;
  esac
}

compose() {
  (
    cd "${script_dir}"
    HOST_UID="$(id -u)" HOST_GID="$(id -g)" \
      docker compose -f "${compose_file}" "$@"
  )
}

is_running() {
  local state
  state="$(compose ps --status running --services 2>/dev/null || true)"
  [ "${state}" = "radicc-server" ]
}

do_up() {
  mkdir -p "${script_dir}/recordings"
  compose up -d
}

do_down() {
  compose down
}

do_build_up() {
  mkdir -p "${script_dir}/recordings"
  compose down
  compose up -d --build
}

main() {
  if [ "${1:-}" = "--build" ]; then
    confirm "Stop, rebuild, and start radicc-server?" || exit 0
    do_build_up
    exit 0
  fi

  if is_running; then
    confirm "radicc-server is running. Stop it?" || exit 0
    do_down
    exit 0
  fi

  confirm "radicc-server is stopped. Start it?" || exit 0
  do_up
}

main "$@"
