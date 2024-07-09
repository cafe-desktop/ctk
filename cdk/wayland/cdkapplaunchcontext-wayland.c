/*
 * Copyright © 2010 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <gio/gdesktopappinfo.h>

#include "cdkwayland.h"
#include "cdkprivate-wayland.h"
#include "cdkapplaunchcontextprivate.h"
#include "cdkscreen.h"
#include "cdkinternals.h"
#include "cdkintl.h"

static char *
cdk_wayland_app_launch_context_get_startup_notify_id (GAppLaunchContext *context,
                                                      GAppInfo          *info G_GNUC_UNUSED,
                                                      GList             *files G_GNUC_UNUSED)
{
  CdkWaylandDisplay *display;
  gchar *id = NULL;

  g_object_get (context, "display", &display, NULL);

  if (display->ctk_shell_version >= 3)
    {
      id = g_uuid_string_random ();
      ctk_shell1_notify_launch (display->ctk_shell, id);
    }

  g_object_unref (display);

  return id;
}

static void
cdk_wayland_app_launch_context_launch_failed (GAppLaunchContext *context G_GNUC_UNUSED,
                                              const char        *startup_notify_id G_GNUC_UNUSED)
{
}

typedef struct _CdkWaylandAppLaunchContext CdkWaylandAppLaunchContext;
typedef struct _CdkWaylandAppLaunchContextClass CdkWaylandAppLaunchContextClass;

struct _CdkWaylandAppLaunchContext
{
  CdkAppLaunchContext base;
  gchar *name;
  guint serial;
};

struct _CdkWaylandAppLaunchContextClass
{
  CdkAppLaunchContextClass base_class;
};

GType cdk_wayland_app_launch_context_get_type (void);

G_DEFINE_TYPE (CdkWaylandAppLaunchContext, cdk_wayland_app_launch_context, CDK_TYPE_APP_LAUNCH_CONTEXT)

static void
cdk_wayland_app_launch_context_class_init (CdkWaylandAppLaunchContextClass *klass)
{
  GAppLaunchContextClass *ctx_class = G_APP_LAUNCH_CONTEXT_CLASS (klass);

  ctx_class->get_startup_notify_id = cdk_wayland_app_launch_context_get_startup_notify_id;
  ctx_class->launch_failed = cdk_wayland_app_launch_context_launch_failed;
}

static void
cdk_wayland_app_launch_context_init (CdkWaylandAppLaunchContext *ctx G_GNUC_UNUSED)
{
}

CdkAppLaunchContext *
_cdk_wayland_display_get_app_launch_context (CdkDisplay *display)
{
  CdkAppLaunchContext *ctx;

  ctx = g_object_new (cdk_wayland_app_launch_context_get_type (),
                      "display", display,
                      NULL);

  return ctx;
}
