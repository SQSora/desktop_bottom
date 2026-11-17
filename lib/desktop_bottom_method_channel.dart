import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'desktop_bottom_platform_interface.dart';

/// Calls the Windows / Linux / macOS native implementations through
/// [MethodChannel].
class MethodChannelDesktopBottom extends DesktopBottomPlatform {
  @visibleForTesting
  final MethodChannel methodChannel = const MethodChannel(
    'dev.sora.desktop_bottom',
  );

  @override
  bool get isSupported {
    if (kIsWeb) return false;
    return defaultTargetPlatform == TargetPlatform.windows ||
        defaultTargetPlatform == TargetPlatform.linux ||
        defaultTargetPlatform == TargetPlatform.macOS;
  }

  @override
  Future<bool> isSupportedNow() async {
    if (!isSupported) return false;
    final result = await methodChannel.invokeMethod<bool>('isSupported');
    return result ?? false;
  }

  @override
  Future<bool> setEnabled(bool enabled) async {
    if (!isSupported) return false;
    final result = await methodChannel.invokeMethod<bool>(
      'setEnabled',
      enabled,
    );
    return result ?? false;
  }

  @override
  Future<bool> isEnabled() async {
    if (!isSupported) return false;
    final result = await methodChannel.invokeMethod<bool>('isEnabled');
    return result ?? false;
  }

  @override
  Future<bool> setDesktopMode(bool enabled) async {
    if (!isSupported) return false;
    final result = await methodChannel.invokeMethod<bool>(
      'setDesktopMode',
      enabled,
    );
    return result ?? false;
  }

  @override
  Future<bool> isDesktopMode() async {
    if (!isSupported) return false;
    final result = await methodChannel.invokeMethod<bool>('isDesktopMode');
    return result ?? false;
  }
}
