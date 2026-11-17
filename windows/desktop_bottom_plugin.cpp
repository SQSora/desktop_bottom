#include "desktop_bottom_plugin.h"

#include <windows.h>

#include <objbase.h>
#include <shobjidl.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <optional>
#include <variant>

namespace desktop_bottom {

namespace {

constexpr char kChannelName[] = "dev.sora.desktop_bottom";
// Win+D moves the window to -32000,-32000 and resizes it to hide it.
constexpr int kHiddenWindowPosition = -30000;
// Delay the minimize so the taskbar has time to create the entry.
constexpr UINT_PTR kMinimizeTimerId = 0xD3B0;
constexpr UINT kMinimizeDelayMs = 100;

}  // namespace

// static
void DesktopBottomPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar) {
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), kChannelName,
          &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<DesktopBottomPlugin>(registrar);

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto& call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

DesktopBottomPlugin::DesktopBottomPlugin(
    flutter::PluginRegistrarWindows* registrar)
    : registrar_(registrar) {
  window_proc_delegate_id_ =
      registrar_->RegisterTopLevelWindowProcDelegate(
          [this](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
            if (top_level_window_ == nullptr) {
              top_level_window_ = hwnd;
            }
            return HandleWindowProc(hwnd, message, wparam, lparam);
          });
}

DesktopBottomPlugin::~DesktopBottomPlugin() {
  if (enabled_) {
    SetEnabled(false);
  }
  ReleaseTaskbarList();
  if (window_proc_delegate_id_ != 0) {
    registrar_->UnregisterTopLevelWindowProcDelegate(window_proc_delegate_id_);
  }
}

void DesktopBottomPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (method_call.method_name() == "setEnabled") {
    if (method_call.arguments() == nullptr ||
        !std::holds_alternative<bool>(*method_call.arguments())) {
      result->Error("bad_arguments", "setEnabled 需要一个布尔参数");
      return;
    }
    result->Success(
        flutter::EncodableValue(SetEnabled(std::get<bool>(*method_call.arguments()))));
    return;
  }

  if (method_call.method_name() == "isEnabled") {
    result->Success(flutter::EncodableValue(enabled_));
    return;
  }

  // Desktop-widget mode is Linux X11-specific; Windows does not support it.
  if (method_call.method_name() == "setDesktopMode") {
    result->Success(flutter::EncodableValue(false));
    return;
  }

  if (method_call.method_name() == "isDesktopMode") {
    result->Success(flutter::EncodableValue(false));
    return;
  }

  if (method_call.method_name() == "isSupported") {
    result->Success(flutter::EncodableValue(GetTopLevelWindow() != nullptr &&
                                            GetShellWindow() != nullptr));
    return;
  }

  result->NotImplemented();
}

HWND DesktopBottomPlugin::GetTopLevelWindow() const {
  if (top_level_window_ != nullptr && IsWindow(top_level_window_)) {
    return top_level_window_;
  }
  if (registrar_ == nullptr) {
    return nullptr;
  }
  auto* view = registrar_->GetView();
  if (view == nullptr) {
    return nullptr;
  }
  HWND view_window = view->GetNativeWindow();
  if (view_window == nullptr) {
    return nullptr;
  }
  return GetAncestor(view_window, GA_ROOT);
}

bool DesktopBottomPlugin::EnsureTaskbarList() {
  if (taskbar_list_ != nullptr) {
    return true;
  }

  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (hr == S_OK) {
    com_initialized_ = true;
  } else if (hr != S_FALSE && hr != RPC_E_CHANGED_MODE) {
    return false;
  }

  ITaskbarList* list = nullptr;
  hr = CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(&list));
  if (FAILED(hr) || list == nullptr) {
    return false;
  }
  if (FAILED(list->HrInit())) {
    list->Release();
    return false;
  }
  taskbar_list_ = list;
  return true;
}

void DesktopBottomPlugin::ReleaseTaskbarList() {
  if (taskbar_list_ != nullptr) {
    taskbar_list_->Release();
    taskbar_list_ = nullptr;
  }
  if (com_initialized_) {
    CoUninitialize();
    com_initialized_ = false;
  }
}

void DesktopBottomPlugin::SuspendBottomForMinimize() {
  if (minimize_suspended_) {
    return;
  }

  HWND window = GetTopLevelWindow();
  if (window == nullptr || !IsWindow(window)) {
    return;
  }

  // A window owned by Progman normally has no taskbar entry. While minimized,
  // temporarily add WS_EX_APPWINDOW (so it appears in Alt+Tab) and call
  // ITaskbarList::AddTab to create the taskbar entry immediately instead of
  // waiting for shell enumeration. The owner stays unchanged, so Win+D
  // behavior is unaffected.
  saved_ex_style_ = GetWindowLongPtrW(window, GWL_EXSTYLE);
  appwindow_added_ = (saved_ex_style_ & WS_EX_APPWINDOW) == 0;
  if (appwindow_added_) {
    SetWindowLongPtrW(window, GWL_EXSTYLE,
                      saved_ex_style_ | WS_EX_APPWINDOW);
    SetWindowPos(window, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                     SWP_FRAMECHANGED);
  }
  if (EnsureTaskbarList()) {
    taskbar_list_->AddTab(window);
  }
  minimize_suspended_ = true;
}

void DesktopBottomPlugin::ResumeBottomAfterMinimize() {
  if (!minimize_suspended_) {
    return;
  }
  minimize_suspended_ = false;

  HWND window = GetTopLevelWindow();
  if (window == nullptr || !IsWindow(window)) {
    return;
  }

  // Remove the taskbar entry after restore so the window returns to pure
  // desktop-bottom mode. If bottom mode was disabled while minimized, the
  // window must return as a normal window, so the entry must stay.
  if (enabled_ && taskbar_list_ != nullptr) {
    taskbar_list_->DeleteTab(window);
  }

  if (appwindow_added_) {
    SetWindowLongPtrW(window, GWL_EXSTYLE, saved_ex_style_);
    SetWindowPos(window, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                     SWP_FRAMECHANGED);
    appwindow_added_ = false;
  }

  if (!enabled_) {
    return;
  }

  HWND shell = GetShellWindow();
  if (shell == nullptr) {
    enabled_ = false;
    return;
  }

  SetWindowLongPtrW(window, GWLP_HWNDPARENT,
                    reinterpret_cast<LONG_PTR>(shell));
  SetWindowPos(window, HWND_BOTTOM, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

bool DesktopBottomPlugin::SetEnabled(bool enabled) {
  HWND window = GetTopLevelWindow();
  if (window == nullptr || !IsWindow(window)) {
    enabled_ = false;
    minimize_suspended_ = false;
    return false;
  }

  if (minimize_suspended_) {
    if (enabled) {
      enabled_ = true;
      return true;
    }

    // Disable bottom mode while minimized: restore the owner but keep
    // WS_EX_APPWINDOW until the window is restored, so the taskbar entry does
    // not disappear.
    SetWindowLongPtrW(window, GWLP_HWNDPARENT, saved_owner_);
    enabled_ = false;
    return true;
  }

  if (enabled_ == enabled) {
    return true;
  }

  if (enabled) {
    HWND shell = GetShellWindow();
    if (shell == nullptr) {
      return false;
    }

    saved_owner_ = GetWindowLongPtrW(window, GWLP_HWNDPARENT);
    // Use the desktop as owner (not parent): when show-desktop raises Progman,
    // the owned window is carried above the desktop; the window is still a
    // normal top-level window, so Flutter rendering is unaffected.
    SetWindowLongPtrW(window, GWLP_HWNDPARENT,
                      reinterpret_cast<LONG_PTR>(shell));
    if (GetWindowLongPtrW(window, GWLP_HWNDPARENT) !=
        reinterpret_cast<LONG_PTR>(shell)) {
      SetWindowLongPtrW(window, GWLP_HWNDPARENT, saved_owner_);
      return false;
    }

    if (!SetWindowPos(window, HWND_BOTTOM, 0, 0, 0, 0,
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                          SWP_FRAMECHANGED)) {
      SetWindowLongPtrW(window, GWLP_HWNDPARENT, saved_owner_);
      return false;
    }

    // The window should have no taskbar entry while bottom mode is enabled. If
    // it was previously registered as a normal window, remove it. Ensure the
    // taskbar list exists first so AddTab is not skipped on the first disable.
    if (EnsureTaskbarList()) {
      taskbar_list_->DeleteTab(window);
    }

    enabled_ = true;
    return true;
  }

  enabled_ = false;
  SetWindowLongPtrW(window, GWLP_HWNDPARENT, saved_owner_);
  SetWindowPos(window, HWND_TOP, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
  SetForegroundWindow(window);
  saved_owner_ = 0;
  // After DeleteTab the shell does not automatically re-add the window to the
  // taskbar; explicitly AddTab after disabling bottom mode, otherwise a later
  // minimize would have no taskbar entry. On the first enable/disable,
  // taskbar_list_ may not exist yet (only the minimize path creates it), so
  // ensure it is initialized here.
  if (EnsureTaskbarList()) {
    taskbar_list_->AddTab(window);
  }
  return true;
}

std::optional<LRESULT> DesktopBottomPlugin::HandleWindowProc(
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  if (message == WM_TIMER && wparam == kMinimizeTimerId) {
    KillTimer(hwnd, kMinimizeTimerId);
    if (minimize_pending_) {
      minimize_pending_ = false;
      ShowWindow(hwnd, SW_MINIMIZE);
    }
    return 0;
  }

  if (minimize_suspended_ && message == WM_SIZE) {
    if (wparam != SIZE_MINIMIZED) {
      ResumeBottomAfterMinimize();
    }
    return std::nullopt;
  }

  // During the minimize transition, allow hide/position changes; otherwise the
  // window remains visible (IsIconic is true but the frame stays on screen).
  if (minimize_suspended_) {
    return std::nullopt;
  }

  if (!enabled_) {
    return std::nullopt;
  }

  switch (message) {
    case WM_WINDOWPOSCHANGING: {
      auto* position = reinterpret_cast<WINDOWPOS*>(lparam);
      if (position == nullptr) {
        return std::nullopt;
      }

      // Block Win+D's hide: keep the current position and size, and clear the
      // hide flag.
      if (position->x <= kHiddenWindowPosition ||
          position->y <= kHiddenWindowPosition) {
        position->flags |= SWP_NOMOVE | SWP_NOSIZE;
        position->flags &= ~SWP_HIDEWINDOW;
      }

      // Otherwise force the window to the bottom so clicking it or the taskbar
      // cannot raise it above normal windows.
      position->hwndInsertAfter = HWND_BOTTOM;
      position->flags &= ~SWP_NOZORDER;
      return std::nullopt;
    }
    case WM_SYSCOMMAND:
      if ((wparam & 0xFFF0) == SC_MINIMIZE) {
        if (minimize_suspended_) {
          return 0;
        }
        SuspendBottomForMinimize();
        if (!minimize_suspended_) {
          return std::nullopt;
        }
        // Let the taskbar create the entry first, then actually minimize.
        minimize_pending_ = true;
        SetTimer(hwnd, kMinimizeTimerId, kMinimizeDelayMs, nullptr);
        return 0;
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

}  // namespace desktop_bottom
