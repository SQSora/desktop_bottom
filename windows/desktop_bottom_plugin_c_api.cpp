#include "include/desktop_bottom/desktop_bottom_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "desktop_bottom_plugin.h"

void DesktopBottomPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  desktop_bottom::DesktopBottomPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
