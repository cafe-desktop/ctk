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

#include <cdk/wayland/cdkwayland.h>

typedef CtkApplicationImplDBusClass CtkApplicationImplWaylandClass;

typedef struct
{
  CtkApplicationImplDBus dbus;

} CtkApplicationImplWayland;

G_DEFINE_TYPE (CtkApplicationImplWayland, ctk_application_impl_wayland, CTK_TYPE_APPLICATION_IMPL_DBUS)

static void
ctk_application_impl_wayland_handle_window_realize (CtkApplicationImpl *impl,
                                                    CtkWindow          *window)
{
  CtkApplicationImplClass *impl_class =
    CTK_APPLICATION_IMPL_CLASS (ctk_application_impl_wayland_parent_class);
  CtkApplicationImplDBus *dbus = (CtkApplicationImplDBus *) impl;
  CdkWindow *cdk_window;
  gchar *window_path;

  cdk_window = ctk_widget_get_window (CTK_WIDGET (window));

  if (!CDK_IS_WAYLAND_WINDOW (cdk_window))
    return;

  window_path = ctk_application_impl_dbus_get_window_path (dbus, window);

  cdk_wayland_window_set_dbus_properties_libctk_only (cdk_window,
                                                      dbus->application_id, dbus->app_menu_path, dbus->menubar_path,
                                                      window_path, dbus->object_path, dbus->unique_name);

  g_free (window_path);

  impl_class->handle_window_realize (impl, window);
}

static void
ctk_application_impl_wayland_before_emit (CtkApplicationImpl *impl G_GNUC_UNUSED,
                                          GVariant           *platform_data)
{
  const char *startup_notification_id = NULL;

  g_variant_lookup (platform_data, "desktop-startup-id", "&s", &startup_notification_id);

  cdk_wayland_display_set_startup_notification_id (cdk_display_get_default (), startup_notification_id);
}

static void
ctk_application_impl_wayland_init (CtkApplicationImplWayland *wayland G_GNUC_UNUSED)
{
}

static void
ctk_application_impl_wayland_class_init (CtkApplicationImplWaylandClass *class)
{
  CtkApplicationImplClass *impl_class = CTK_APPLICATION_IMPL_CLASS (class);

  impl_class->handle_window_realize =
    ctk_application_impl_wayland_handle_window_realize;
  impl_class->before_emit =
    ctk_application_impl_wayland_before_emit;
}
