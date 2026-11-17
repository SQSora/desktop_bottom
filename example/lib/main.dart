import 'package:desktop_bottom/desktop_bottom.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  bool _enabled = false;
  bool _desktopMode = false;
  bool _nativeSupported = true;

  @override
  void initState() {
    super.initState();
    _checkNativeSupport();
  }

  Future<void> _checkNativeSupport() async {
    if (!DesktopBottom.isSupported) return;
    bool supported;
    try {
      supported = await DesktopBottom.isSupportedNow();
    } catch (_) {
      // Treat a missing plugin or failed native query as unsupported so the
      // switches do not appear to do nothing.
      supported = false;
    }
    if (!mounted || supported == _nativeSupported) return;
    setState(() => _nativeSupported = supported);
  }

  Future<void> _toggle(bool value) async {
    final applied = await DesktopBottom.setEnabled(value);
    if (!mounted) return;
    setState(() {
      if (applied) {
        _enabled = value;
        _desktopMode = false;
      }
    });
  }

  Future<void> _toggleDesktopMode(bool value) async {
    final applied = await DesktopBottom.setDesktopMode(value);
    if (!mounted) return;
    setState(() {
      if (applied) {
        _desktopMode = value;
        _enabled = value;
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    final supported = DesktopBottom.isSupported && _nativeSupported;
    final String hint;
    if (!supported) {
      hint = DesktopBottom.isSupported
          ? '当前会话不支持（Linux 需要 X11，macOS 需要窗口已创建）'
          : '仅支持 Windows、Linux X11 与 macOS';
    } else if (defaultTargetPlatform == TargetPlatform.macOS) {
      hint = '窗口会沉到所有窗口之后，按 F11 显示桌面时仍可见';
    } else if (defaultTargetPlatform == TargetPlatform.linux) {
      hint = '窗口会沉到所有窗口之后（仅支持 X11）';
    } else {
      hint = '窗口会沉到所有窗口之后，按 Win+D 仍然可见';
    }
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(title: const Text('desktop_bottom example')),
        body: ListView(
          children: [
            SwitchListTile(
              title: const Text('桌面置底'),
              subtitle: Text(hint),
              value: _enabled,
              onChanged: supported ? (value) => _toggle(value) : null,
            ),
            if (supported &&
                (defaultTargetPlatform == TargetPlatform.linux ||
                    defaultTargetPlatform == TargetPlatform.macOS))
              SwitchListTile(
                title: const Text('桌面组件模式'),
                subtitle: Text(
                  defaultTargetPlatform == TargetPlatform.macOS
                      ? '无标题栏，显示桌面时仍可见'
                      : '仅 X11：无标题栏，显示桌面时仍可见',
                ),
                value: _desktopMode,
                onChanged: _toggleDesktopMode,
              ),
          ],
        ),
      ),
    );
  }
}
