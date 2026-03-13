# X11 Smoke Testing

FlamentineSwitcher ships environment-sensitive runtime smoke scripts for the X11 path:

- `tests/run_x11_delayed_replacement_e2e.sh`
- `tests/run_x11_remember_per_window_e2e.sh`
- `tests/run_x11_remember_per_app_e2e.sh`

## Existing Plasma X11 Session

If you already have a real Plasma X11 session, run the suite against that display:

```bash
tests/run_x11_runtime_smoke_suite.sh ./build/flamentine-switcher ./build/x11_text_target "$DISPLAY"
```

This is the most reliable path because the focus rules match the intended deployment target.

## Nested Xephyr or Xvfb

The suite can also run against a clean nested X11 server:

```bash
tests/run_x11_runtime_smoke_suite.sh ./build/flamentine-switcher ./build/x11_text_target --xephyr
tests/run_x11_runtime_smoke_suite.sh ./build/flamentine-switcher ./build/x11_text_target --xvfb
```

Requirements for nested mode:

- `Xephyr` or `Xvfb`
- `kwin_x11` or `openbox`

The window manager requirement is intentional. The X11 window backend depends on `_NET_ACTIVE_WINDOW`, so a bare X server is not enough for meaningful focus-sensitive tests.

If the required binaries are missing, the suite exits with code `2` and a clear message.
