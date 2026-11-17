import 'package:desktop_bottom/desktop_bottom.dart';
import 'package:desktop_bottom/desktop_bottom_platform_interface.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class _FakePlatform extends DesktopBottomPlatform
    with MockPlatformInterfaceMixin {
  bool supported = true;
  bool supportedNow = true;
  bool enabled = false;
  bool desktopMode = false;
  final List<bool> calls = [];

  @override
  bool get isSupported => supported;

  @override
  Future<bool> isSupportedNow() async => supportedNow;

  @override
  Future<bool> setEnabled(bool value) async {
    calls.add(value);
    enabled = value;
    if (value) desktopMode = false;
    return true;
  }

  @override
  Future<bool> isEnabled() async => enabled;

  @override
  Future<bool> setDesktopMode(bool value) async {
    desktopMode = value;
    enabled = value;
    return true;
  }

  @override
  Future<bool> isDesktopMode() async => desktopMode;
}

void main() {
  late _FakePlatform fake;
  late DesktopBottomPlatform original;

  setUp(() {
    original = DesktopBottomPlatform.instance;
    fake = _FakePlatform();
    DesktopBottomPlatform.instance = fake;
  });

  tearDown(() {
    DesktopBottomPlatform.instance = original;
  });

  test('转发 isSupported / setEnabled / isEnabled', () async {
    expect(DesktopBottom.isSupported, isTrue);

    expect(await DesktopBottom.setEnabled(true), isTrue);
    expect(fake.calls, [true]);
    expect(await DesktopBottom.isEnabled(), isTrue);

    fake.supported = false;
    expect(DesktopBottom.isSupported, isFalse);
  });

  test('转发 isSupportedNow', () async {
    expect(await DesktopBottom.isSupportedNow(), isTrue);

    fake.supportedNow = false;
    expect(await DesktopBottom.isSupportedNow(), isFalse);
  });

  test('转发 setDesktopMode / isDesktopMode', () async {
    expect(await DesktopBottom.setDesktopMode(true), isTrue);
    expect(await DesktopBottom.isDesktopMode(), isTrue);
    expect(await DesktopBottom.isEnabled(), isTrue);

    expect(await DesktopBottom.setDesktopMode(false), isTrue);
    expect(await DesktopBottom.isDesktopMode(), isFalse);
    expect(await DesktopBottom.isEnabled(), isFalse);
  });
}
