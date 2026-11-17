#include "include/desktop_bottom/desktop_bottom_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

#include <cstring>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#define DESKTOP_BOTTOM_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), desktop_bottom_plugin_get_type(), \
                              DesktopBottomPlugin))

struct _DesktopBottomPlugin {
  GObject parent_instance;

  FlView* view;
  gboolean enabled;
  gboolean desktop_mode;

  GdkWindowTypeHint original_type_hint;
  gboolean original_decorated;
  gboolean original_skip_taskbar;
  gboolean original_skip_pager;
};

G_DEFINE_TYPE(DesktopBottomPlugin, desktop_bottom_plugin, G_TYPE_OBJECT)

namespace {

GtkWindow* GetTopLevelWindow(DesktopBottomPlugin* self) {
  if (self->view == nullptr) {
    return nullptr;
  }
  GtkWidget* toplevel = gtk_widget_get_toplevel(GTK_WIDGET(self->view));
  if (!GTK_IS_WINDOW(toplevel)) {
    return nullptr;
  }
  return GTK_WINDOW(toplevel);
}

bool IsX11(GtkWindow* window) {
#ifdef GDK_WINDOWING_X11
  if (window == nullptr) {
    return false;
  }
  GdkDisplay* display = gtk_widget_get_display(GTK_WIDGET(window));
  return GDK_IS_X11_DISPLAY(display);
#else
  (void)window;
  return false;
#endif
}

gboolean SetDesktopMode(DesktopBottomPlugin* self, gboolean enabled);

gboolean SetEnabled(DesktopBottomPlugin* self, gboolean enabled) {
  GtkWindow* window = GetTopLevelWindow(self);
  if (window == nullptr || !IsX11(window)) {
    self->enabled = FALSE;
    self->desktop_mode = FALSE;
    return FALSE;
  }

  if (self->desktop_mode) {
    if (!SetDesktopMode(self, FALSE)) {
      return FALSE;
    }
  }

  if (self->enabled == enabled) {
    return TRUE;
  }

  // On X11 GDK translates this into an EWMH _NET_WM_STATE_BELOW request; the
  // window manager may ignore it.
  gtk_window_set_keep_below(window, enabled);
  if (!enabled) {
    // Bring the window to the foreground after disabling bottom mode to match
    // Windows behavior.
    gtk_window_present(window);
  }
  self->enabled = enabled;
  return TRUE;
}

gboolean SetDesktopMode(DesktopBottomPlugin* self, gboolean enabled) {
  GtkWindow* window = GetTopLevelWindow(self);
  if (window == nullptr || !IsX11(window)) {
    self->enabled = FALSE;
    self->desktop_mode = FALSE;
    return FALSE;
  }

  if (self->desktop_mode == enabled) {
    return TRUE;
  }

  if (enabled) {
    self->original_type_hint = gtk_window_get_type_hint(window);
    self->original_decorated = gtk_window_get_decorated(window);
    self->original_skip_taskbar = gtk_window_get_skip_taskbar_hint(window);
    self->original_skip_pager = gtk_window_get_skip_pager_hint(window);

    // The desktop type keeps the window on the desktop layer and visible while
    // Show Desktop is active; the trade-off is that it no longer has a normal
    // app titlebar or taskbar entry.
    gtk_window_set_type_hint(window, GDK_WINDOW_TYPE_HINT_DESKTOP);
    gtk_window_set_keep_below(window, TRUE);
    gtk_window_set_skip_taskbar_hint(window, TRUE);
    gtk_window_set_skip_pager_hint(window, TRUE);
    gtk_window_set_decorated(window, FALSE);
    gtk_window_stick(window);
    self->desktop_mode = TRUE;
    self->enabled = TRUE;
    return TRUE;
  }

  gtk_window_set_keep_below(window, FALSE);
  gtk_window_set_skip_taskbar_hint(window, self->original_skip_taskbar);
  gtk_window_set_skip_pager_hint(window, self->original_skip_pager);
  gtk_window_set_decorated(window, self->original_decorated);
  gtk_window_set_type_hint(window, self->original_type_hint);
  gtk_window_unstick(window);
  gtk_window_present(window);
  self->desktop_mode = FALSE;
  self->enabled = FALSE;
  return TRUE;
}

gboolean IsSupported(DesktopBottomPlugin* self) {
  GtkWindow* window = GetTopLevelWindow(self);
  return window != nullptr && IsX11(window);
}

void HandleMethodCall(DesktopBottomPlugin* self, FlMethodCall* method_call) {
  const gchar* method = fl_method_call_get_name(method_call);
  g_autoptr(FlMethodResponse) response = nullptr;

  if (strcmp(method, "setEnabled") == 0) {
    FlValue* args = fl_method_call_get_args(method_call);
    if (args == nullptr || fl_value_get_type(args) != FL_VALUE_TYPE_BOOL) {
      response = FL_METHOD_RESPONSE(fl_method_error_response_new(
          "bad_arguments", "setEnabled 需要一个布尔参数", nullptr));
    } else {
      gboolean applied = SetEnabled(self, fl_value_get_bool(args));
      response = FL_METHOD_RESPONSE(
          fl_method_success_response_new(fl_value_new_bool(applied)));
    }
  } else if (strcmp(method, "isEnabled") == 0) {
    response = FL_METHOD_RESPONSE(
        fl_method_success_response_new(fl_value_new_bool(self->enabled)));
  } else if (strcmp(method, "setDesktopMode") == 0) {
    FlValue* args = fl_method_call_get_args(method_call);
    if (args == nullptr || fl_value_get_type(args) != FL_VALUE_TYPE_BOOL) {
      response = FL_METHOD_RESPONSE(fl_method_error_response_new(
          "bad_arguments", "setDesktopMode 需要一个布尔参数", nullptr));
    } else {
      gboolean applied = SetDesktopMode(self, fl_value_get_bool(args));
      response = FL_METHOD_RESPONSE(
          fl_method_success_response_new(fl_value_new_bool(applied)));
    }
  } else if (strcmp(method, "isDesktopMode") == 0) {
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(
        fl_value_new_bool(self->desktop_mode)));
  } else if (strcmp(method, "isSupported") == 0) {
    response = FL_METHOD_RESPONSE(
        fl_method_success_response_new(fl_value_new_bool(IsSupported(self))));
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

}  // namespace

static void desktop_bottom_plugin_dispose(GObject* object) {
  DesktopBottomPlugin* self = DESKTOP_BOTTOM_PLUGIN(object);
  if (self->desktop_mode) {
    SetDesktopMode(self, FALSE);
  } else if (self->enabled) {
    GtkWindow* window = GetTopLevelWindow(self);
    if (window != nullptr) {
      gtk_window_set_keep_below(window, FALSE);
    }
    self->enabled = FALSE;
  }
  if (self->view != nullptr) {
    g_object_unref(self->view);
    self->view = nullptr;
  }
  G_OBJECT_CLASS(desktop_bottom_plugin_parent_class)->dispose(object);
}

static void desktop_bottom_plugin_class_init(
    DesktopBottomPluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = desktop_bottom_plugin_dispose;
}

static void desktop_bottom_plugin_init(DesktopBottomPlugin* self) {
  self->view = nullptr;
  self->enabled = FALSE;
  self->desktop_mode = FALSE;
  self->original_type_hint = GDK_WINDOW_TYPE_HINT_NORMAL;
  self->original_decorated = TRUE;
  self->original_skip_taskbar = FALSE;
  self->original_skip_pager = FALSE;
}

static void method_call_cb(FlMethodChannel* channel,
                           FlMethodCall* method_call,
                           gpointer user_data) {
  (void)channel;
  HandleMethodCall(DESKTOP_BOTTOM_PLUGIN(user_data), method_call);
}

void desktop_bottom_plugin_register_with_registrar(
    FlPluginRegistrar* registrar) {
  DesktopBottomPlugin* plugin = DESKTOP_BOTTOM_PLUGIN(
      g_object_new(desktop_bottom_plugin_get_type(), nullptr));
  // FlPluginRegistrar is only valid during the registration callback; keep a
  // reference to the view for later method calls.
  plugin->view = fl_plugin_registrar_get_view(registrar);
  if (plugin->view != nullptr) {
    g_object_ref(plugin->view);
  }

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  g_autoptr(FlMethodChannel) channel = fl_method_channel_new(
      fl_plugin_registrar_get_messenger(registrar), "dev.sora.desktop_bottom",
      FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(
      channel, method_call_cb, g_object_ref(plugin), g_object_unref);

  g_object_unref(plugin);
}
