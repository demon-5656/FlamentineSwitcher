#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 <flamentine-switcher-bin> <x11-text-target-bin> [display]" >&2
  exit 2
fi

SWITCHER_BIN="$1"
TARGET_BIN="$2"
DISPLAY_TO_USE="${3:-${DISPLAY:-}}"

if [ -z "$DISPLAY_TO_USE" ]; then
  echo "DISPLAY is not set and no explicit display argument was provided" >&2
  exit 2
fi

for tool in qdbus xdotool; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "Required tool is missing: $tool" >&2
    exit 2
  fi
done

CONFIG_DIR="${HOME}/.config/FlamentineSwitcher"
CONFIG_FILE="${CONFIG_DIR}/config.json"
STATE_FILE="${CONFIG_DIR}/state.json"
mkdir -p "$CONFIG_DIR"

BACKUP_FILE="$(mktemp)"
BACKUP_STATE_FILE="$(mktemp)"
if [ -f "$CONFIG_FILE" ]; then
  cp "$CONFIG_FILE" "$BACKUP_FILE"
else
  : > "$BACKUP_FILE"
fi
if [ -f "$STATE_FILE" ]; then
  cp "$STATE_FILE" "$BACKUP_STATE_FILE"
else
  : > "$BACKUP_STATE_FILE"
fi

APP_PID=""
TARGET_PID=""
TEMP_DIR="$(mktemp -d)"
TARGET_OUTPUT="${TEMP_DIR}/target_output.txt"
TARGET_STDOUT="${TEMP_DIR}/target_stdout.txt"
APP_LOG="${TEMP_DIR}/switcher.log"

wait_for_switcher_service() {
  local expected_pid="$1"
  for _ in $(seq 1 60); do
    local owner_pid=""
    owner_pid="$(qdbus org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.GetConnectionUnixProcessID org.flamentineswitcher.Control 2>/dev/null | tr -d '\r\n' || true)"
    if [ "$owner_pid" = "$expected_pid" ] && qdbus org.flamentineswitcher.Control /org/flamentineswitcher/Control org.flamentineswitcher.Control.GetCurrentLayout >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.25
  done

  echo "Failed to start FlamentineSwitcher D-Bus service for PID $expected_pid" >&2
  return 1
}

cleanup() {
  if [ -n "$TARGET_PID" ]; then
    kill "$TARGET_PID" 2>/dev/null || true
  fi
  if [ -n "$APP_PID" ]; then
    kill "$APP_PID" 2>/dev/null || true
  fi

  if [ -s "$BACKUP_FILE" ]; then
    cp "$BACKUP_FILE" "$CONFIG_FILE"
  else
    rm -f "$CONFIG_FILE"
  fi
  if [ -s "$BACKUP_STATE_FILE" ]; then
    cp "$BACKUP_STATE_FILE" "$STATE_FILE"
  else
    rm -f "$STATE_FILE"
  fi

  rm -f "$BACKUP_FILE" "$BACKUP_STATE_FILE"
  rm -rf "$TEMP_DIR"
}
trap cleanup EXIT

cat > "$CONFIG_FILE" <<'JSON'
{
  "version": 1,
  "enabled": true,
  "layouts": ["us", "ru"],
  "defaultLayout": "us",
  "rememberLayoutPerWindow": false,
  "rememberLayoutPerApp": false,
  "notificationsEnabled": false,
  "autoStart": false,
  "requireAllowedTargets": true,
  "allowedApps": ["x11_text_target"],
  "allowedWindowClasses": ["x11_text_target"],
  "excludedApps": [],
  "excludedWindowClasses": [],
  "excludeTerminals": false,
  "excludeFullscreen": false,
  "hotkeys": {
    "toggleLayout": "Meta+Space",
    "convertLastWord": "Pause",
    "convertSelection": "Meta+Pause",
    "openSettings": "Meta+Alt+K",
    "toggleEnabled": "Meta+Alt+S"
  },
  "conversion": {
    "preserveCase": true,
    "preservePunctuation": true,
    "autoConvertEnabled": true,
    "heuristicsEnabled": true,
    "autoConvertDelayMs": 250
  },
  "logging": {
    "level": "info",
    "fileEnabled": false
  }
}
JSON
rm -f "$STATE_FILE"

if command -v setxkbmap >/dev/null 2>&1; then
  DISPLAY="$DISPLAY_TO_USE" setxkbmap -layout us,ru >/dev/null 2>&1 || true
fi

env DISPLAY="$DISPLAY_TO_USE" XDG_SESSION_TYPE=x11 QT_QPA_PLATFORM=xcb WAYLAND_DISPLAY= "$SWITCHER_BIN" >"$APP_LOG" 2>&1 &
APP_PID=$!
wait_for_switcher_service "$APP_PID"

qdbus org.flamentineswitcher.Control /org/flamentineswitcher/Control org.flamentineswitcher.Control.SetLayout us >/dev/null

env DISPLAY="$DISPLAY_TO_USE" QT_QPA_PLATFORM=xcb WAYLAND_DISPLAY= "$TARGET_BIN" "$TARGET_OUTPUT" >"$TARGET_STDOUT" 2>/dev/null &
TARGET_PID=$!

WINDOW_ID=""
for _ in $(seq 1 40); do
  if [ -s "$TARGET_STDOUT" ]; then
    WINDOW_ID="$(tail -n 1 "$TARGET_STDOUT" | tr -d '\r\n' || true)"
    if [ -n "$WINDOW_ID" ]; then
      break
    fi
  fi
  sleep 0.25
done

if [ -z "$WINDOW_ID" ]; then
  echo "Failed to obtain target window id" >&2
  exit 1
fi

DISPLAY="$DISPLAY_TO_USE" xdotool windowactivate --sync "$WINDOW_ID" >/dev/null 2>&1
sleep 0.8
DISPLAY="$DISPLAY_TO_USE" xdotool mousemove --window "$WINDOW_ID" 50 30 click 1 >/dev/null 2>&1
sleep 0.2
DISPLAY="$DISPLAY_TO_USE" xdotool key ctrl+a >/dev/null 2>&1
sleep 0.1
DISPLAY="$DISPLAY_TO_USE" xdotool type --delay 120 ghbdtn\  >/dev/null 2>&1
sleep 2.1

CURRENT_LAYOUT="$(qdbus org.flamentineswitcher.Control /org/flamentineswitcher/Control org.flamentineswitcher.Control.GetCurrentLayout | tr -d '\r\n')"

DISPLAY="$DISPLAY_TO_USE" xdotool key Return >/dev/null 2>&1
wait "$TARGET_PID"

RESULT="$(cat "$TARGET_OUTPUT" 2>/dev/null || true)"
if [ "$RESULT" != "привет " ]; then
  echo "Unexpected e2e result: '$RESULT'" >&2
  echo "Switcher log: $(tr '\n' ' ' <"$APP_LOG")" >&2
  exit 1
fi

if [ "$CURRENT_LAYOUT" != "ru" ]; then
  echo "Unexpected layout after delayed replacement: '$CURRENT_LAYOUT'" >&2
  exit 1
fi

printf "E2E_OK result=%q layout=%s\n" "$RESULT" "$CURRENT_LAYOUT"
