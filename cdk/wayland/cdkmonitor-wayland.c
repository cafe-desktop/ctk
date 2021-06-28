/*
 * Copyright © 2016 Red Hat, Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib.h>
#include <gio/gio.h>
#include "cdkprivate-wayland.h"

#include "cdkmonitor-wayland.h"

G_DEFINE_TYPE (CdkWaylandMonitor, cdk_wayland_monitor, GDK_TYPE_MONITOR)

static void
cdk_wayland_monitor_init (CdkWaylandMonitor *monitor)
{
}

static void
cdk_wayland_monitor_finalize (GObject *object)
{
  CdkWaylandMonitor *monitor = (CdkWaylandMonitor *)object;

  g_free (monitor->name);

  wl_output_destroy (monitor->output);

  G_OBJECT_CLASS (cdk_wayland_monitor_parent_class)->finalize (object);
}

static void
cdk_wayland_monitor_class_init (CdkWaylandMonitorClass *class)
{
  G_OBJECT_CLASS (class)->finalize = cdk_wayland_monitor_finalize;
}

/**
 * cdk_wayland_monitor_get_wl_output:
 * @monitor: (type CdkWaylandMonitor): a #CdkMonitor
 *
 * Returns the Wayland wl_output of a #CdkMonitor.
 *
 * Returns: (transfer none): a Wayland wl_output
 * Since: 3.22
 */
struct wl_output *
cdk_wayland_monitor_get_wl_output (CdkMonitor *monitor)
{
  g_return_val_if_fail (GDK_IS_WAYLAND_MONITOR (monitor), NULL);

  return GDK_WAYLAND_MONITOR (monitor)->output;
}
