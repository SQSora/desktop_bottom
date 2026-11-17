import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'desktop_bottom_method_channel.dart';

/// Platform interface; tests can replace it with a fake.
abstract class DesktopBottomPlatform extends PlatformInterface {
  DesktopBottomPlatform() : super(token: _token);

  static final Object _token = Object();

  static DesktopBottomPlatform _instance = MethodChannelDesktopBottom();

  static DesktopBottomPlatform get instance => _instance;

  static set instance(DesktopBottomPlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  /// Whether the current platform has an implementation.
  ///
  /// This is a synchronous platform check and does not touch the native window.
  /// On Linux Wayland it is still true, but the native side rejects requests;
  /// use [isSupportedNow] to check the current session.
  bool get isSupported;

  /// Asks the native side whether the current session is usable.
  ///
  /// Linux requires an X11 session; macOS requires a window to exist. Other
  /// unsupported conditions return false.
  Future<bool> isSupportedNow();

  /// Enables or disables the mode; returns whether the native side applied it.
  Future<bool> setEnabled(bool enabled);

  /// Whether the native side is currently in bottom mode.
  Future<bool> isEnabled();

  /// Enables or disables Linux X11 desktop-widget mode.
  Future<bool> setDesktopMode(bool enabled);

  /// Whether the native side is currently in desktop-widget mode.
  Future<bool> isDesktopMode();
}
