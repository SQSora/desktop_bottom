#ifndef FLUTTER_PLUGIN_DESKTOP_BOTTOM_PLUGIN_H_
#define FLUTTER_PLUGIN_DESKTOP_BOTTOM_PLUGIN_H_

#include <flutter/encodable_value.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>
#include <optional>

struct ITaskbarList;

namespace desktop_bottom {

class DesktopBottomPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

  explicit DesktopBottomPlugin(flutter::PluginRegistrarWindows* registrar);

  ~DesktopBottomPlugin() override;

  DesktopBottomPlugin(const DesktopBottomPlugin&) = delete;
  DesktopBottomPlugin& operator=(const DesktopBottomPlugin&) = delete;

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

 private:
  bool SetEnabled(bool enabled);
  void SuspendBottomForMinimize();
  void ResumeBottomAfterMinimize();
  bool EnsureTaskbarList();
  void ReleaseTaskbarList();
  HWND GetTopLevelWindow() const;
  std::optional<LRESULT> HandleWindowProc(HWND hwnd, UINT message,
                                          WPARAM wparam, LPARAM lparam);

  flutter::PluginRegistrarWindows* registrar_;
  int window_proc_delegate_id_ = 0;
  HWND top_level_window_ = nullptr;
  bool enabled_ = false;
  bool minimize_suspended_ = false;
  bool minimize_pending_ = false;
  bool appwindow_added_ = false;
  bool com_initialized_ = false;
  ITaskbarList* taskbar_list_ = nullptr;
  LONG_PTR saved_owner_ = 0;
  LONG_PTR saved_ex_style_ = 0;
};

}  // namespace desktop_bottom

#endif  // FLUTTER_PLUGIN_DESKTOP_BOTTOM_PLUGIN_H_
