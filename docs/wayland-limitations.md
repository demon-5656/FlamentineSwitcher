# Wayland Limitations

FlamentineSwitcher does not claim full Punto Switcher-like behavior on Wayland.

## What is intentionally not promised

- global transparent input interception
- unrestricted global key capture
- arbitrary text rewriting inside other applications
- hidden fallback hacks that break on compositor updates

## Current Wayland stance

- the application reads the active Plasma layout via `org.kde.KeyboardLayouts`
- manual layout switching works through the same Plasma D-Bus API
- the application remains usable as a tray/settings utility
- text conversion remains available as a manual clipboard-oriented workflow

## Why

Modern Wayland compositors deliberately restrict global input interception. KDE Plasma can expose safe APIs over time, but the MVP architecture assumes those APIs may be absent and degrades cleanly.

## Next realistic expansion points

- supported Plasma or KWin D-Bus interfaces
- portal-based interactions when suitable
- compositor-specific integration behind explicit backend modules
