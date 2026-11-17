# desktop_bottom

[English](README.md) | 简体中文

让 Flutter 桌面窗口始终沉在普通窗口层级的最底部，并且在显示桌面时仍然可见。

`desktop_bottom` 是一个面向 Windows、Linux（X11）和 macOS 的小型插件。它让
当前顶层窗口表现得像桌面层，同时不接管 Flutter 的渲染。

> 若中文版本的`README`与英文内容不一致时，以本中文版为准。

## 特性

- **始终置底**：普通窗口会盖在应用窗口之上。
- **感知“显示桌面”**：Windows 上按 Win+D 仍然可见；Linux X11 和 macOS 的
  桌面组件模式在“显示桌面”时仍然可见。
- **可选的桌面组件模式**：在 Linux X11 和 macOS 上，把窗口变成没有标题栏的
  桌面组件；Linux 上还会移除任务栏入口。
- **保持正常 Flutter 渲染**：窗口仍是普通的 Flutter 顶层窗口，不会被
  `SetParent` 到 Progman/WorkerW。
- **明确的能力检查**：`isSupported` 检查平台，`isSupportedNow()` 检查当前原生
  会话是否真的可用。
- **不持久化**：插件不会保存设置；何时以及如何重新应用由调用方决定。

## 平台支持

| 平台 | 置底模式 | 桌面组件模式 | 说明 |
|---|---|---|---|
| Windows | 支持 | 不支持 | Win+D 时仍可见；最小化期间会出现任务栏入口 |
| Linux X11 | 支持 | 支持 | 不支持 Wayland |
| macOS | 支持 | 支持 | 需要真实的 `NSWindow`；全屏时不应用桌面组件模式 |

`isSupported` 是同步的平台判断；在 Linux Wayland 下它仍然为 `true`，因此在
依赖原生行为前应先调用 `isSupportedNow()`。

## 安装

```sh
flutter pub add desktop_bottom
```

或手动添加：

```yaml
dependencies:
  desktop_bottom: ^0.1.1
```

包要求 Dart 3.10.3 或更高版本；确切约束见 `pubspec.yaml`。

## 快速开始

```dart
import 'package:desktop_bottom/desktop_bottom.dart';
import 'package:flutter/foundation.dart';

Future<void> setUpDesktopBottom() async {
  if (!DesktopBottom.isSupported) return;
  if (!await DesktopBottom.isSupportedNow()) return;

  // 让窗口沉在普通窗口之后。
  await DesktopBottom.setEnabled(true);

  // 可选：在 Linux X11 和 macOS 上变成桌面组件。
  if (defaultTargetPlatform == TargetPlatform.linux ||
      defaultTargetPlatform == TargetPlatform.macOS) {
    await DesktopBottom.setDesktopMode(true);
  }
}
```

在 macOS 上，等待窗口创建完成后再调用 `isSupportedNow()`；没有 `NSWindow` 时
它会返回 `false`。对于桌面组件模式，在窗口首次显示前应用通常最可靠。在 Linux
上，运行时修改窗口类型可能会被某些窗口管理器忽略。

插件不会持久化这些设置。调用方应把配置保存在自己的设置中，并在应用启动时
重新应用。

## API

| 成员 | 返回 | 说明 |
|---|---|---|
| `DesktopBottom.isSupported` | `bool` | 同步平台判断；不访问原生窗口。 |
| `DesktopBottom.isSupportedNow()` | `Future<bool>` | 向原生查询当前会话是否可用。 |
| `DesktopBottom.setEnabled(bool)` | `Future<bool>` | 开启或关闭置底模式。 |
| `DesktopBottom.isEnabled()` | `Future<bool>` | 当前是否已应用置底模式。 |
| `DesktopBottom.setDesktopMode(bool)` | `Future<bool>` | 开启或关闭桌面组件模式；Windows 返回 `false`。 |
| `DesktopBottom.isDesktopMode()` | `Future<bool>` | 当前是否已应用桌面组件模式。 |

这些 setter 会返回原生侧是否成功应用。平台不支持或会话不可用时返回 `false`，
而不是抛异常。如果支持平台上的插件没有注册，MethodChannel 仍可能抛出
`MissingPluginException`。

## 工作原理

### Windows

- 把顶层窗口的 **所有者（owner）** 设为 `GetShellWindow()`（Progman），而不是
  父窗口。窗口仍然是普通的 Flutter 顶层窗口，渲染不受影响；当 Win+D 抬高桌面
  时，这个 owned 窗口会跟着桌面留在可见层。
- 在 `WM_WINDOWPOSCHANGING` 中强制 `HWND_BOTTOM`，让普通窗口可以盖住它。
- 拦截 Win+D 的 `(-32000, -32000)` 隐藏移动。
- 最小化时临时加上 `WS_EX_APPWINDOW` 并调用 `ITaskbarList::AddTab`，让窗口立即
  获得任务栏入口；恢复时调用 `DeleteTab`，并重新应用 owner=Progman 和
  `HWND_BOTTOM`。

### Linux X11

- 普通模式调用 `gtk_window_set_keep_below()`，请求窗口管理器设置
  `_NET_WM_STATE_BELOW`；窗口管理器可能忽略该请求。
- 桌面组件模式设置 `_NET_WM_WINDOW_TYPE_DESKTOP`，并设置 below、sticky、
  skip-taskbar/pager 状态，同时移除窗口装饰。这样窗口在“显示桌面”时仍然可见。
- Wayland 下没有让普通客户端控制窗口层级的协议；两种模式都返回 `false`。

### macOS

- 置底模式把 `NSWindow.level` 设为略低于 `normalWindow`，并向 collection
  behavior 加入 `.stationary` / `.canJoinAllSpaces`。
- 桌面组件模式把 level 设为高于 `desktopIconWindow`，使用保留 titled 样式、
  但标题栏隐藏且透明、红黄绿按钮隐藏的窗口，并加入 `.stationary`、
  `.canJoinAllSpaces` 和 `.ignoresCycle`。保留 titled 样式可以让窗口继续成为
  key/main window，键盘输入仍然可用。

## 注意事项

- **Windows Explorer 重启/崩溃：** owner 是 Explorer 的 `Progman`。重启或崩溃
  Explorer 会销毁窗口并导致应用退出。下次启动时重新应用设置。
- **固定到任务栏：** 置底期间窗口没有任务栏 tab。如果应用被固定，点击 pinned
  图标会启动新进程，而不是激活已有窗口。需要的话应在应用层实现单实例处理。
- **Win+M / Minimize All：** 置底窗口会保持可见。要最小化，请使用 Win+↓ 或
  最小化按钮。
- **Linux：** 仅支持 X11。Wayland 不支持。普通置底模式会被“显示桌面”隐藏；
  桌面组件模式才会保持可见。桌面组件模式还会移除任务栏/Alt-Tab 入口。
- **macOS：** 桌面组件模式会移除标题栏并保留应用的 Dock 图标；窗口循环切换会
  被禁用。窗口处于全屏时返回 `false`。
- **App Store：** macOS 桌面组件模式使用非标准窗口层级；App Store 审核可能
  要求说明理由。
- **单视图：** 插件只针对隐式 Flutter 视图。多窗口应用需要单独设计。
- **自定义 Windows runner：** 必须通过
  `FlutterViewController::HandleTopLevelWindowProc` 转发顶层窗口消息，否则插件
  收不到所需消息。
- **窗口管理类插件：** 可能与其他修改同一个顶层窗口的插件冲突，例如
  `window_manager`。
- **桌面图标：** 桌面图标或其他桌面窗口可能会绘制在组件之上。
- **某些窗口管理器** 可能忽略运行时对窗口类型的修改；建议在窗口首次显示前
  应用桌面组件模式。

## 示例

`example/` 目录包含一个可运行示例：

```sh
cd example
# Windows
flutter run -d windows
# Linux：确保当前是 X11 会话
flutter run -d linux
# macOS
flutter run -d macos
```