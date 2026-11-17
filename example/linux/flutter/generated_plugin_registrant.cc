//
//  Generated file. Do not edit.
//

// clang-format off

#include "generated_plugin_registrant.h"

#include <desktop_bottom/desktop_bottom_plugin.h>

void fl_register_plugins(FlPluginRegistry* registry) {
  g_autoptr(FlPluginRegistrar) desktop_bottom_registrar =
      fl_plugin_registry_get_registrar_for_plugin(registry, "DesktopBottomPlugin");
  desktop_bottom_plugin_register_with_registrar(desktop_bottom_registrar);
}
