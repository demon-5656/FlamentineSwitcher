# KDE Plasma: one-click transliteration

Start `flamentine-switcher` once (the supplied autostart entry can do this).  On Wayland, a `ydotoold` virtual-input daemon must already be running.  Verify it with:

```text
ydotool debug
```

The installation registers a dedicated KDE launcher for **Transliterate selected text**.  Its global shortcut is `Meta+End`.  If Plasma does not pick it up after installation, log out and back in, then assign that shortcut in **System Settings → Keyboard → Shortcuts**.

```text
flamentine-switcher --transliterate-selection
```

Bind it to one shortcut, for example `Meta+End`.

The command only contacts the running FlamentineSwitcher service over the session bus; it does not launch another resident copy.  Select text and press the shortcut.

On both Plasma X11 and Wayland, the selection is replaced immediately.  The Wayland implementation intentionally uses `ydotool` to generate `Ctrl+C`, reads the result through Plasma Klipper, and then generates `Ctrl+V`.  It therefore works in applications that accept normal keyboard copy and paste.  Do not start a second `ydotoold` if one is already using the same socket.
