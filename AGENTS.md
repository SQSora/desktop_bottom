# AGENTS.md — desktop_bottom

## 项目定位

- Flutter 桌面插件：Windows、Linux X11、macOS；没有 Android/iOS/Web 实现。
- 插件只提供原生窗口开关，**不持久化设置、不做单实例**。调用方负责保存状态并在启动时重放。
- 只操作 implicit view；多窗口（Flutter windowing）不支持。

## 工具链与常用命令

使用 `fvm`（`.fvmrc` 固定 Flutter 3.44.3），不要直接用裸 `flutter`。

根目录：

```powershell
fvm flutter pub get
fvm flutter analyze
fvm flutter test
fvm flutter test test/desktop_bottom_test.dart --plain-name "转发 isSupported"
```

example：

```powershell
cd example
fvm flutter pub get
fvm flutter analyze
fvm flutter test
fvm flutter build windows --debug
```

Windows 构建产物：
`example/build/windows/x64/runner/Debug/desktop_bottom_example.exe`

集成测试会真实开窗，**不能跑在无桌面的 headless 环境**：

```powershell
cd example
fvm flutter test integration_test -d windows
fvm flutter test integration_test -d linux   # 仅 X11
```

- Linux 通过 SSH 运行时需显式设置 `DISPLAY` 和 `XAUTHORITY` 到当前 X11 会话。
- 集成测试假设新进程初始 `isEnabled == false`；期间不要复用同一应用实例。
- 仓库没有 CI。`example/windows/CMakeLists.txt` 把 `include_desktop_bottom_tests` 设为 `FALSE`，`windows/test/desktop_bottom_plugin_test.cpp` 也只是占位；原生行为必须真机/真桌面验证。
- 根目录 `pubspec.lock` 被 `.gitignore` 忽略（库项目约定）；`example/pubspec.lock` 才是跟踪的锁文件。

## 结构与数据流

- `lib/desktop_bottom.dart`：`DesktopBottom` 静态公开 API。
- `lib/desktop_bottom_platform_interface.dart`：`DesktopBottomPlatform` 抽象，`PlatformInterface` token；测试用 fake 替换。
- `lib/desktop_bottom_method_channel.dart`：`MethodChannel('dev.sora.desktop_bottom')`。
- 原生实现：
  - `windows/desktop_bottom_plugin.cpp`
  - `linux/desktop_bottom_plugin.cc`
  - `macos/desktop_bottom/Sources/desktop_bottom/DesktopBottomPlugin.swift`
- 通道方法名固定为 `setEnabled`、`isEnabled`、`setDesktopMode`、`isDesktopMode`、`isSupported`；Dart、三端原生和测试必须同步修改。
- `isSupported` 是同步平台判断（Linux Wayland 下仍为 true）；`isSupportedNow()` 才查询原生当前会话（Wayland/macOS 窗口未创建时为 false）。不要用前者判断运行时可用性。
- `setDesktopMode` 在 Windows 恒返回 false；Linux 仅 X11、macOS 需真实 `NSWindow`。
- 自定义 `DesktopBottomPlatform` 实现必须补 `isSupportedNow`；这是 0.1.1 的接口变更。
- 未支持平台/不可用会话的 setter 返回 false，不应改成抛异常。

## 平台约束与坑

### Windows

- 核心是 **owner = `GetShellWindow()`（Progman）**，不是 parent。不要改回 `SetParent`/子窗口方案：Flutter 的 DXGI/ANGLE 渲染会黑屏。
- 不要加 `WS_EX_TOOLWINDOW`；它会变成工具窗口并隐藏最小化/最大化按钮。
- `WM_WINDOWPOSCHANGING` 强制 `HWND_BOTTOM`，并拦截 Win+D 的 `-32000` 隐藏移动；这两段不要随手移除。
- 最小化依赖 `WM_SYSCOMMAND SC_MINIMIZE` 拦截、100ms 定时器、临时 `WS_EX_APPWINDOW` 和 `ITaskbarList::AddTab`；恢复时 `DeleteTab` 并重新应用 owner/bottom。改这段必须重测任务栏入口。
- Explorer 重启/崩溃会销毁 owned 窗口并结束应用（设计代价，README 有说明）。
- 固定到任务栏后：置底时 pinned 图标保留但“运行中”状态消失；最小化时 `AddTab` 会与 pinned 合并成单个按钮；**置底期间点击 pinned 图标会启动第二个实例**，因为原窗口没有 taskbar tab。example 不是单实例；生产应用需自行做单实例激活，不要通过恢复置底时的 taskbar tab 来“修”它。
- `Win+M` / `Shell.MinimizeAll()` 不会最小化置底窗口；`Win+↓` 和最小化按钮会。写测试时不要假设 Win+M 走挂起路径。
- 自定义 Windows runner 必须把顶层窗口消息转发到 `FlutterViewController::HandleTopLevelWindowProc`，否则插件收不到消息。
- `windows/CMakeLists.txt` 的 `/utf-8` 是防止非 ASCII 源码在代码页 936 下触发 C4819（再被 `/WX` 提升为错误）的必要设置，不要删除。

### Linux

- 仅 X11。Wayland 下 `isSupportedNow`、`setEnabled`、`setDesktopMode` 都返回 false，不要试图用普通客户端协议实现置底。
- 普通模式用 `gtk_window_set_keep_below`；桌面模式额外设置 `_NET_WM_WINDOW_TYPE_DESKTOP`、`_NET_WM_STATE_BELOW/STICKY/SKIP_TASKBAR/SKIP_PAGER` 和 undecorated。
- `setEnabled()` 会先退出桌面模式；`setDesktopMode(false)` 恢复到普通非置底窗口。
- 运行时改变 window type 可能被某些 WM 忽略；README 建议在窗口首次显示前应用。Mutter X11 实测可运行时切换。
- 已知边界：`SetDesktopMode(false)` 无条件 `gtk_window_unstick`，不会恢复进入桌面模式前已有的 sticky 状态；修这块时要补原状态保存。
- example Linux runner 使用 `G_APPLICATION_NON_UNIQUE`，本身允许多实例；与 pinned 第二实例现象相关。

### macOS

- 所有 `NSWindow` 操作在主线程；`isSupportedNow` 在窗口不存在时为 false。
- 普通模式只改 level/collectionBehavior，不恢复 frame（置底期间的移动/缩放要保留）。
- 桌面模式改 styleMask/frame/标题栏/交通灯；全屏时返回 false；恢复时还原保存的状态。
- 普通模式与桌面模式共用一份 `savedState`：先普通置底再进桌面模式，会复用普通置底时的 frame 快照。改状态机前先确认这个语义。
- macOS 行为只能在 macOS 上编译/运行验证；Windows/Linux 测试不能替代。

## 测试与证据

- `fvm flutter test` 覆盖 Dart API 转发和 MethodChannel，不覆盖原生行为。
- `example/test/widget_test.dart` 只检查示例页文案；`fvm flutter test` 在 example 下应通过。
- 原生行为需要真实桌面：owner、Win+D、任务栏入口、最小化、Show Desktop、窗口类型等。
- 本机历史验证用的探针脚本和截图在 `build/probe/`（`build/` 被忽略，不属于包）；clean clone 中不要假设它们存在，也不要提交 build 产物。
- 验证平台时至少覆盖：
  - Windows：enable → 普通窗口遮挡 → Win+D → 最小化/任务栏恢复 → disable → 再最小化。
  - Linux X11：`_NET_WM_STATE_BELOW` → Show Desktop（普通模式应隐藏）→ desktop mode 四类 state/type → Show Desktop（应保持可见）→ disable。
  - macOS：普通模式、桌面模式、全屏边界、键盘输入。

## 文档与发布

- `README_zh.md` 是当前行为的权威依据；`README.md` 是供 pub.dev 展示的英文翻译。不要以历史聊天记录作为现状。
- 版本号需同步：`pubspec.yaml`、`macos/desktop_bottom.podspec` 与 `CHANGELOG.md`。
- `publish_to` 已移除；`repository`/`issue_tracker` 指向 GitHub，podspec 的 homepage 已同步。
- `example/**/generated_plugin_registrant.*`、`generated_plugins.cmake` 是生成文件，不要手改。
