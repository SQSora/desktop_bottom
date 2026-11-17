import 'package:desktop_bottom/desktop_bottom.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('桌面置底插件可被调用', (WidgetTester tester) async {
    expect(DesktopBottom.isSupported, isTrue);
    expect(await DesktopBottom.isSupportedNow(), isTrue);
    expect(await DesktopBottom.isEnabled(), isFalse);
    expect(await DesktopBottom.setEnabled(false), isTrue);

    // Cover the full enable -> disable path. Restoring the Windows taskbar
    // entry is shell behavior and needs real-device UI automation.
    expect(await DesktopBottom.setEnabled(true), isTrue);
    expect(await DesktopBottom.isEnabled(), isTrue);
    expect(await DesktopBottom.setEnabled(false), isTrue);
    expect(await DesktopBottom.isEnabled(), isFalse);

    final desktopMode = await DesktopBottom.setDesktopMode(true);
    if (defaultTargetPlatform == TargetPlatform.macOS ||
        defaultTargetPlatform == TargetPlatform.linux) {
      expect(desktopMode, isTrue);
      expect(await DesktopBottom.isDesktopMode(), isTrue);
      expect(await DesktopBottom.setDesktopMode(false), isTrue);
      expect(await DesktopBottom.isDesktopMode(), isFalse);
    } else {
      expect(desktopMode, isFalse);
    }
  });
}
