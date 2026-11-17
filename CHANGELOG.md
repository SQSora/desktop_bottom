# Changelog

## 0.1.1 - 2026-11-17

Initial release.

- Windows: keep the top-level Flutter window below normal windows, remain
  visible on Win+D, and restore the taskbar entry while minimized.
- Linux X11: add normal keep-below mode and desktop-widget mode.
- macOS: add normal bottom mode and desktop-widget mode.
- Add `DesktopBottom.isSupportedNow()` for checking the current native session.
- Keep settings application explicit: the plugin does not persist state.
