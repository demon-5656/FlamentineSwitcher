#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 <flamentine-switcher-bin> <x11-text-target-bin> [display|--xephyr|--xvfb|--auto]" >&2
  exit 2
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SWITCHER_BIN="$1"
TARGET_BIN="$2"
MODE="${3:---auto}"

SERVER_PID=""
WM_PID=""
NESTED_DISPLAY=""

cleanup() {
  if [ -n "$WM_PID" ]; then
    kill "$WM_PID" 2>/dev/null || true
  fi
  if [ -n "$SERVER_PID" ]; then
    kill "$SERVER_PID" 2>/dev/null || true
  fi
}
trap cleanup EXIT

find_free_display() {
  for candidate in $(seq 120 140); do
    if ! DISPLAY=":${candidate}" xprop -root >/dev/null 2>&1; then
      printf ':%s\n' "$candidate"
      return 0
    fi
  done

  echo "Failed to find a free nested X11 display" >&2
  exit 2
}

wait_for_x_server() {
  local display_to_wait="$1"
  for _ in $(seq 1 40); do
    if DISPLAY="$display_to_wait" xprop -root >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.25
  done

  echo "Nested X11 server on $display_to_wait did not become ready" >&2
  exit 1
}

start_window_manager() {
  local display_to_use="$1"

  if command -v kwin_x11 >/dev/null 2>&1; then
    env DISPLAY="$display_to_use" QT_QPA_PLATFORM=xcb WAYLAND_DISPLAY= kwin_x11 --replace >/tmp/flamentine-switcher-kwin.log 2>&1 &
    WM_PID=$!
    return 0
  fi

  if command -v openbox >/dev/null 2>&1; then
    env DISPLAY="$display_to_use" openbox >/tmp/flamentine-switcher-openbox.log 2>&1 &
    WM_PID=$!
    return 0
  fi

  echo "A nested X11 smoke run requires kwin_x11 or openbox for _NET_ACTIVE_WINDOW support" >&2
  exit 2
}

start_nested_server() {
  local nested_mode="$1"
  NESTED_DISPLAY="$(find_free_display)"

  case "$nested_mode" in
    --xephyr)
      if ! command -v Xephyr >/dev/null 2>&1; then
        echo "Xephyr is not installed" >&2
        exit 2
      fi
      Xephyr "$NESTED_DISPLAY" -screen 1280x800 -ac -nolisten tcp >/tmp/flamentine-switcher-xephyr.log 2>&1 &
      SERVER_PID=$!
      ;;
    --xvfb)
      if ! command -v Xvfb >/dev/null 2>&1; then
        echo "Xvfb is not installed" >&2
        exit 2
      fi
      Xvfb "$NESTED_DISPLAY" -screen 0 1280x800x24 -nolisten tcp >/tmp/flamentine-switcher-xvfb.log 2>&1 &
      SERVER_PID=$!
      ;;
    *)
      if command -v Xephyr >/dev/null 2>&1; then
        Xephyr "$NESTED_DISPLAY" -screen 1280x800 -ac -nolisten tcp >/tmp/flamentine-switcher-xephyr.log 2>&1 &
        SERVER_PID=$!
      elif command -v Xvfb >/dev/null 2>&1; then
        Xvfb "$NESTED_DISPLAY" -screen 0 1280x800x24 -nolisten tcp >/tmp/flamentine-switcher-xvfb.log 2>&1 &
        SERVER_PID=$!
      else
        echo "Neither Xephyr nor Xvfb is installed, and no existing X11 display was provided" >&2
        exit 2
      fi
      ;;
  esac

  wait_for_x_server "$NESTED_DISPLAY"
  start_window_manager "$NESTED_DISPLAY"
  sleep 1.0
}

run_suite() {
  local display_to_use="$1"

  printf 'Running delayed replacement smoke on %s\n' "$display_to_use"
  "$SCRIPT_DIR/run_x11_delayed_replacement_e2e.sh" "$SWITCHER_BIN" "$TARGET_BIN" "$display_to_use"

  printf 'Running remember-per-window smoke on %s\n' "$display_to_use"
  "$SCRIPT_DIR/run_x11_remember_per_window_e2e.sh" "$SWITCHER_BIN" "$TARGET_BIN" "$display_to_use"

  printf 'Running remember-per-app smoke on %s\n' "$display_to_use"
  "$SCRIPT_DIR/run_x11_remember_per_app_e2e.sh" "$SWITCHER_BIN" "$TARGET_BIN" "$display_to_use"
}

case "$MODE" in
  --xephyr|--xvfb|--auto)
    start_nested_server "$MODE"
    run_suite "$NESTED_DISPLAY"
    ;;
  *)
    run_suite "$MODE"
    ;;
esac
