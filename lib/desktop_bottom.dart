import 'desktop_bottom_platform_interface.dart';

/// Desktop-bottom switch.
///
/// On Windows the window remains a normal Flutter top-level window (rendering
/// is unaffected), stays below all normal windows, and remains visible when the
/// desktop is shown with Win+D.
///
/// Linux supports X11 sessions only: it uses the EWMH `_NET_WM_STATE_BELOW`
/// state to keep the window at the bottom. Wayland has no way for a normal
/// client to control stacking, so `setEnabled` returns false. "Show desktop"
/// has no equivalent keep-visible behavior. On Linux X11, [setDesktopMode] can
/// turn the window into a desktop widget so it stays visible when the desktop
/// is shown.
///
/// macOS uses `NSWindow.level` and
/// `NSWindowCollectionBehavior.stationary` for bottom mode and show-desktop
/// visibility; [setDesktopMode] makes the window a borderless desktop widget.
class DesktopBottom {
  const DesktopBottom._();

  /// Whether the current platform has an implementation; true on Windows,
  /// Linux, and macOS.
  ///
  /// This is a synchronous platform check and does not touch the native window:
  /// it is still true on Linux Wayland, and on macOS before the window exists.
  /// Use [isSupportedNow] to check whether the current session is usable.
  static bool get isSupported => DesktopBottomPlatform.instance.isSupported;

  /// Asks the native side whether the current session is usable.
  ///
  /// Linux requires an X11 session; macOS requires a window to exist. Other
  /// platforms, or a native refusal, return false. Unlike [isSupported], this
  /// call accesses the real native window.
  static Future<bool> isSupportedNow() =>
      DesktopBottomPlatform.instance.isSupportedNow();

  /// Enables or disables desktop-bottom mode; returns whether the native side
  /// applied it successfully.
  static Future<bool> setEnabled(bool enabled) =>
      DesktopBottomPlatform.instance.setEnabled(enabled);

  /// Whether the native side is currently in bottom mode.
  static Future<bool> isEnabled() => DesktopBottomPlatform.instance.isEnabled();

  /// Enables or disables desktop-widget mode.
  ///
  /// On Linux X11 this sets the window as an EWMH desktop window; on macOS it
  /// sets a desktop-level window and removes the titlebar. Both keep the window
  /// visible when the desktop is shown.
  ///
  /// Linux requires an X11 session; macOS requires a window to exist. Wayland,
  /// Windows, and other platforms return false. Applying it before the window
  /// is first shown is recommended.
  static Future<bool> setDesktopMode(bool enabled) =>
      DesktopBottomPlatform.instance.setDesktopMode(enabled);

  /// Whether the native side is currently in desktop-widget mode.
  static Future<bool> isDesktopMode() =>
      DesktopBottomPlatform.instance.isDesktopMode();
}
