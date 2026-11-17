import 'package:desktop_bottom/desktop_bottom_method_channel.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  const MethodChannel channel = MethodChannel('dev.sora.desktop_bottom');
  late bool nativeSupported;

  setUp(() {
    nativeSupported = true;
    debugDefaultTargetPlatformOverride = TargetPlatform.windows;
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, (MethodCall methodCall) async {
          if (methodCall.method == 'isSupported') {
            return nativeSupported;
          }
          if (methodCall.method == 'setEnabled') {
            return methodCall.arguments as bool;
          }
          if (methodCall.method == 'isEnabled') {
            return true;
          }
          if (methodCall.method == 'setDesktopMode') {
            return methodCall.arguments as bool;
          }
          if (methodCall.method == 'isDesktopMode') {
            return true;
          }
          return null;
        });
  });

  tearDown(() {
    debugDefaultTargetPlatformOverride = null;
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, null);
  });

  test('isSupportedNow 查询原生 isSupported', () async {
    final platform = MethodChannelDesktopBottom();
    expect(await platform.isSupportedNow(), isTrue);

    nativeSupported = false;
    expect(await platform.isSupportedNow(), isFalse);
  });

  test('非桌面平台 isSupportedNow 返回 false', () async {
    debugDefaultTargetPlatformOverride = TargetPlatform.android;
    final platform = MethodChannelDesktopBottom();
    expect(platform.isSupported, isFalse);
    expect(await platform.isSupportedNow(), isFalse);
  });

  test('setEnabled / isEnabled 走平台通道', () async {
    final platform = MethodChannelDesktopBottom();
    expect(await platform.setEnabled(true), isTrue);
    expect(await platform.setEnabled(false), isFalse);
    expect(await platform.isEnabled(), isTrue);
  });

  test('Linux 平台同样走平台通道', () async {
    debugDefaultTargetPlatformOverride = TargetPlatform.linux;
    final platform = MethodChannelDesktopBottom();
    expect(platform.isSupported, isTrue);
    expect(await platform.setEnabled(true), isTrue);
    expect(await platform.isEnabled(), isTrue);
  });

  test('macOS 平台同样走平台通道', () async {
    debugDefaultTargetPlatformOverride = TargetPlatform.macOS;
    final platform = MethodChannelDesktopBottom();
    expect(platform.isSupported, isTrue);
    expect(await platform.setEnabled(true), isTrue);
    expect(await platform.setDesktopMode(true), isTrue);
    expect(await platform.isDesktopMode(), isTrue);
  });

  test('setDesktopMode / isDesktopMode 走平台通道', () async {
    final platform = MethodChannelDesktopBottom();
    expect(await platform.setDesktopMode(true), isTrue);
    expect(await platform.isDesktopMode(), isTrue);
    expect(await platform.setDesktopMode(false), isFalse);
  });
}
