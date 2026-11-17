# desktop_bottom

[简体中文](https://github.com/SQSora/desktop_bottom/blob/master/README_zh.md) | English

Keep a Flutter desktop window at the bottom of the normal window stack — and
keep it visible when the desktop is shown.

`desktop_bottom` is a small plugin for Windows, Linux (X11), and macOS. It makes
the current top-level window behave like a desktop layer without taking over
Flutter's rendering.

## Features

- **Always at the bottom** — normal windows stay above the app window.
- **Show Desktop aware** — stays visible on Win+D on Windows; desktop mode
  stays visible on Show Desktop on Linux X11 and macOS.
- **Optional desktop-widget mode** — on Linux X11 and macOS, turn the window
  into a desktop-level widget with no titlebar; on Linux it also has no taskbar
  entry.
- **Normal Flutter rendering** — the window remains a regular top-level Flutter
  window; it is not reparented into Progman/WorkerW.
- **Explicit capability checks** — `isSupported` checks the platform, while
  `isSupportedNow()` checks whether the current native session is usable.
- **No persistence** — the plugin never writes settings; your app decides when
  and how to re-apply the mode.

## Platform support

| Platform | Bottom mode | Desktop-widget mode | Notes |
|---|---|---|---|
| Windows | Yes | No | Visible on Win+D; a taskbar entry appears while minimized |
| Linux X11 | Yes | Yes | Wayland is not supported |
| macOS | Yes | Yes | Requires a real `NSWindow`; desktop mode is not applied in full screen |

`isSupported` is a synchronous platform check. It is still `true` on Linux
Wayland, so call `isSupportedNow()` before relying on the native behavior.

## Installation

```sh
flutter pub add desktop_bottom
```

Or add it manually:

```yaml
dependencies:
  desktop_bottom: ^0.1.1
```

The package requires Dart 3.10.3 or later; see `pubspec.yaml` for the exact
constraints.

## Quick start

```dart
import 'package:desktop_bottom/desktop_bottom.dart';
import 'package:flutter/foundation.dart';

Future<void> setUpDesktopBottom() async {
  if (!DesktopBottom.isSupported) return;
  if (!await DesktopBottom.isSupportedNow()) return;

  // Keep the window below normal windows.
  await DesktopBottom.setEnabled(true);

  // Optional: on Linux X11 and macOS, turn it into a desktop widget.
  if (defaultTargetPlatform == TargetPlatform.linux ||
      defaultTargetPlatform == TargetPlatform.macOS) {
    await DesktopBottom.setDesktopMode(true);
  }
}
```

On macOS, wait until the window exists before calling `isSupportedNow()`; it
returns `false` while there is no `NSWindow`. For desktop-widget mode, applying
it before the window is first shown gives the most reliable result. On Linux,
changing the window type at runtime may be ignored by some window managers.

The plugin does not persist these settings. Store the preference in your own
settings and re-apply it when the app starts.

## API

| Member | Returns | Description |
|---|---|---|
| `DesktopBottom.isSupported` | `bool` | Synchronous platform check; does not touch the native window. |
| `DesktopBottom.isSupportedNow()` | `Future<bool>` | Asks the native side whether the current session is usable. |
| `DesktopBottom.setEnabled(bool)` | `Future<bool>` | Enables or disables bottom mode. |
| `DesktopBottom.isEnabled()` | `Future<bool>` | Whether bottom mode is currently applied. |
| `DesktopBottom.setDesktopMode(bool)` | `Future<bool>` | Enables or disables desktop-widget mode; returns `false` on Windows. |
| `DesktopBottom.isDesktopMode()` | `Future<bool>` | Whether desktop-widget mode is currently applied. |

The setter methods return whether the native side applied the request. On an
unsupported platform or an unusable session they return `false` rather than
throwing. If the plugin is not registered on a supported platform, the method
channel can still throw `MissingPluginException`.

## How it works

### Windows

- Sets the top-level window's **owner** to `GetShellWindow()` (Progman), not its
  parent. The window stays a normal top-level Flutter window, so rendering is
  unaffected; when Win+D raises the desktop, the owned window follows it.
- Forces `HWND_BOTTOM` in `WM_WINDOWPOSCHANGING` so normal windows cover it.
- Blocks Win+D's `(-32000, -32000)` hide move.
- When minimized, temporarily adds `WS_EX_APPWINDOW` and calls
  `ITaskbarList::AddTab` so the window gets a taskbar entry immediately. When
  restored, `DeleteTab` removes the entry and owner=Progman plus `HWND_BOTTOM`
  are re-applied.

### Linux X11

- Normal mode calls `gtk_window_set_keep_below()`, which asks the window manager
  to set `_NET_WM_STATE_BELOW`. The window manager may ignore the request.
- Desktop-widget mode sets `_NET_WM_WINDOW_TYPE_DESKTOP` plus below, sticky, and
  skip-taskbar/pager states, and removes window decorations. This keeps the
  window visible while the desktop is shown.
- Wayland has no protocol for a normal client to control stacking; both modes
  return `false` there.

### macOS

- Bottom mode sets `NSWindow.level` just below `normalWindow` and adds
  `.stationary` / `.canJoinAllSpaces` to the collection behavior.
- Desktop-widget mode sets the level above `desktopIconWindow`, uses a titled
  window with a hidden/transparent titlebar and hidden traffic lights, and adds
  `.stationary`, `.canJoinAllSpaces`, and `.ignoresCycle`. The titled style
  keeps the window key/main capable, so keyboard input keeps working.

## Caveats

- **Windows Explorer restart/crash:** the owner is Explorer's `Progman`.
  Restarting or crashing Explorer destroys the window and exits the app.
  Re-apply the setting on the next launch.
- **Pinned taskbar icon:** while bottom mode is enabled the window has no
  taskbar tab. If the app is pinned, clicking the pinned icon starts a new
  process instead of activating the existing window. Add single-instance
  handling in the app if that matters.
- **Win+M / Minimize All:** the bottom window stays visible. Use Win+Down or the
  minimize button to minimize it.
- **Linux:** X11 only. Wayland is unsupported. Normal bottom mode is hidden by
  Show Desktop; desktop-widget mode is the mode that remains visible. Desktop
  mode also removes the taskbar/Alt-Tab entry.
- **macOS:** desktop mode removes the titlebar and keeps the app's Dock icon;
  window cycling is disabled. It returns `false` while the window is in full
  screen.
- **App Store:** macOS desktop mode uses a non-standard window level; App Store
  review may require justification for desktop-level windows.
- **Single view:** the plugin targets the implicit Flutter view. Multi-window
  apps need a separate design.
- **Custom Windows runners:** forward top-level window messages through
  `FlutterViewController::HandleTopLevelWindowProc`, or the plugin will not
  receive the messages it needs.
- **Window-management plugins:** may conflict with plugins that manipulate the
  same top-level window, such as `window_manager`.
- **Desktop icons:** desktop icons or other desktop windows may be drawn above
  the widget.
- **Some window managers** may ignore runtime changes to the window type.
  Applying desktop-widget mode before the window is first shown is recommended.

## Example

A runnable example is included in the `example/` directory:

```sh
cd example
flutter run -d windows
# Linux: make sure you are in an X11 session
flutter run -d linux
# macOS
flutter run -d macos
```

