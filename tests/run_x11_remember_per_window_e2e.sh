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

for tool in qdbus xdotool xprop awk; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "Required tool is missing: $tool" >&2
    exit 2
  fi
done

CONFIG_DIR="${HOME}/.config/FlamentineSwitcher"
CONFIG_FILE="${CONFIG_DIR}/config.json"
STATE_FILE="${CONFIG_DIR}/state.json"
mkdir -p "$CONFIG_DIR"

BACKUP_CONFIG="$(mktemp)"
BACKUP_STATE="$(mktemp)"
if [ -f "$CONFIG_FILE" ]; then
  cp "$CONFIG_FILE" "$BACKUP_CONFIG"
else
  : > "$BACKUP_CONFIG"
fi
if [ -f "$STATE_FILE" ]; then
  cp "$STATE_FILE" "$BACKUP_STATE"
else
  : > "$BACKUP_STATE"
fi

APP_PID=""
TARGET_PID=""
TEMP_DIR="$(mktemp -d)"
TARGET_OUTPUT="${TEMP_DIR}/target_output.txt"
TARGET_STDOUT="${TEMP_DIR}/target_stdout.txt"
APP_LOG="${TEMP_DIR}/switcher.log"

cleanup() {
  if [ -n "$TARGET_PID" ]; then
    kill "$TARGET_PID" 2>/dev/null || true
  fi
  if [ -n "$APP_PID" ]; then
    kill "$APP_PID" 2>/dev/null || true
  fi

  if [ -s "$BACKUP_CONFIG" ]; then
    cp "$BACKUP_CONFIG" "$CONFIG_FILE"
  else
    rm -f "$CONFIG_FILE"
  fi
  if [ -s "$BACKUP_STATE" ]; then
    cp "$BACKUP_STATE" "$STATE_FILE"
  else
    rm -f "$STATE_FILE"
  fi

  rm -f "$BACKUP_CONFIG" "$BACKUP_STATE"
  rm -rf "$TEMP_DIR"
}
trap cleanup EXIT

start_switcher() {
  env DISPLAY="$DISPLAY_TO_USE" XDG_SESSION_TYPE=x11 QT_QPA_PLATFORM=xcb WAYLAND_DISPLAY= "$SWITCHER_BIN" >"$APP_LOG" 2>&1 &
  APP_PID=$!

  for _ in $(seq 1 40); do
    if qdbus org.flamentineswitcher.Control /org/flamentineswitcher/Control org.flamentineswitcher.Control.GetCurrentLayout >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.25
  done

  echo "Failed to start FlamentineSwitcher D-Bus service" >&2
  exit 1
}

cat > "$CONFIG_FILE" <<'JSON'
{
  "version": 1,
  "enabled": true,
  "layouts": ["us", "ru"],
  "defaultLayout": "us",
  "rememberLayoutPerWindow": true,
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
    "autoConvertEnabled": false,
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

start_switcher

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

DISPLAY="$DISPLAY_TO_USE" xdotool windowactivate --sync "$WINDOW_ID" >/dev/null 2>&1 || true
DISPLAY="$DISPLAY_TO_USE" xdotool mousemove --window "$WINDOW_ID" 50 30 click 1 >/dev/null 2>&1 || true
ACTIVE_WINDOW_HEX="$(DISPLAY="$DISPLAY_TO_USE" xprop -root _NET_ACTIVE_WINDOW 2>/dev/null | awk '{print $NF}' || true)"
TARGET_WINDOW_HEX="0x$(printf '%x' "$WINDOW_ID")"
if [ "$ACTIVE_WINDOW_HEX" != "$TARGET_WINDOW_HEX" ]; then
  echo "Unable to focus the target window automatically on this X server; run this e2e in a real Plasma X11 session" >&2
  exit 2
fi
sleep 0.5
qdbus org.flamentineswitcher.Control /org/flamentineswitcher/Control org.flamentineswitcher.Control.SetLayout ru >/dev/null
sleep 2.2

STATE_CONTENT="$(cat "$STATE_FILE" 2>/dev/null || true)"
if ! printf '%s' "$STATE_CONTENT" | grep -F "\"0x$(printf '%x' "$WINDOW_ID")\": \"ru\"" >/dev/null 2>&1; then
  echo "Expected persisted per-window state was not written for window $WINDOW_ID" >&2
  echo "State content: $STATE_CONTENT" >&2
  exit 1
fi

kill "$APP_PID" 2>/dev/null || true
wait "$APP_PID" || true
APP_PID=""

if command -v setxkbmap >/dev/null 2>&1; then
  DISPLAY="$DISPLAY_TO_USE" setxkbmap -layout us,ru >/dev/null 2>&1 || true
fi

start_switcher
sleep 1.6
RESTORED_LAYOUT="$(qdbus org.flamentineswitcher.Control /org/flamentineswitcher/Control org.flamentineswitcher.Control.GetCurrentLayout | tr -d '\r\n')"

if [ "$RESTORED_LAYOUT" != "ru" ]; then
  echo "Unexpected remembered layout after restart: '$RESTORED_LAYOUT'" >&2
  echo "State content: $STATE_CONTENT" >&2
  echo "Switcher log: $(tr '\n' ' ' <"$APP_LOG")" >&2
  exit 1
fi

kill "$TARGET_PID" 2>/dev/null || true
wait "$TARGET_PID" || true
TARGET_PID=""

printf "E2E_OK window=%s layout=%s\n" "$WINDOW_ID" "$RESTORED_LAYOUT"
