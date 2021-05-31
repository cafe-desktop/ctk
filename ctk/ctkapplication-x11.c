/*
 * Copyright © 2010 Codethink Limited
 * Copyright © 2013 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "ctkapplicationprivate.h"

#include <gdk/gdkx.h>

typedef CtkApplicationImplDBusClass CtkApplicationImplX11Class;

typedef struct
{
  CtkApplicationImplDBus dbus;

} CtkApplicationImplX11;

G_DEFINE_TYPE (CtkApplicationImplX11, ctk_application_impl_x11, CTK_TYPE_APPLICATION_IMPL_DBUS)

static void
ctk_application_impl_x11_handle_window_realize (CtkApplicationImpl *impl,
                                                CtkWindow          *window)
{
  CtkApplicationImplDBus *dbus = (CtkApplicationImplDBus *) impl;
  GdkWindow *gdk_window;
  gchar *window_path;

  gdk_window = ctk_widget_get_window (CTK_WIDGET (window));

  if (!GDK_IS_X11_WINDOW (gdk_window))
    return;

  window_path = ctk_application_impl_dbus_get_window_path (dbus, window);

  gdk_x11_window_set_utf8_property (gdk_window, "_CTK_APPLICATION_ID", dbus->application_id);
  gdk_x11_window_set_utf8_property (gdk_window, "_CTK_UNIQUE_BUS_NAME", dbus->unique_name);
  gdk_x11_window_set_utf8_property (gdk_window, "_CTK_APPLICATION_OBJECT_PATH", dbus->object_path);
  gdk_x11_window_set_utf8_property (gdk_window, "_CTK_WINDOW_OBJECT_PATH", window_path);
  gdk_x11_window_set_utf8_property (gdk_window, "_CTK_APP_MENU_OBJECT_PATH", dbus->app_menu_path);
  gdk_x11_window_set_utf8_property (gdk_window, "_CTK_MENUBAR_OBJECT_PATH", dbus->menubar_path);

  g_free (window_path);
}

static GVariant *
ctk_application_impl_x11_get_window_system_id (CtkApplicationImplDBus *dbus,
                                               CtkWindow              *window)
{
  GdkWindow *gdk_window;

  gdk_window = ctk_widget_get_window (CTK_WIDGET (window));

  if (GDK_IS_X11_WINDOW (gdk_window))
    return g_variant_new_uint32 (GDK_WINDOW_XID (gdk_window));

  return CTK_APPLICATION_IMPL_DBUS_CLASS (ctk_application_impl_x11_parent_class)->get_window_system_id (dbus, window);
}

static void
ctk_application_impl_x11_init (CtkApplicationImplX11 *x11)
{
}

static void
ctk_application_impl_x11_before_emit (CtkApplicationImpl *impl,
                                      GVariant           *platform_data)
{
  const char *startup_notification_id = NULL;

  g_variant_lookup (platform_data, "desktop-startup-id", "&s", &startup_notification_id);

  gdk_x11_display_set_startup_notification_id (gdk_display_get_default (), startup_notification_id);
}

static void
ctk_application_impl_x11_class_init (CtkApplicationImplX11Class *class)
{
  CtkApplicationImplDBusClass *dbus_class = CTK_APPLICATION_IMPL_DBUS_CLASS (class);
  CtkApplicationImplClass *impl_class = CTK_APPLICATION_IMPL_CLASS (class);

  impl_class->handle_window_realize = ctk_application_impl_x11_handle_window_realize;
  dbus_class->get_window_system_id = ctk_application_impl_x11_get_window_system_id;
  impl_class->before_emit = ctk_application_impl_x11_before_emit;
}
