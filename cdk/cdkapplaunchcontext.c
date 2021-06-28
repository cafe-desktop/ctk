/* cdkapplaunchcontext.c - Ctk+ implementation for GAppLaunchContext

   Copyright (C) 2007 Red Hat, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library. If not, see <http://www.gnu.org/licenses/>.

   Author: Alexander Larsson <alexl@redhat.com>
*/

#include "config.h"

#include "cdkapplaunchcontextprivate.h"
#include "cdkscreen.h"
#include "cdkintl.h"


/**
 * SECTION:cdkapplaunchcontext
 * @Short_description: Startup notification for applications
 * @Title: Application launching
 *
 * CdkAppLaunchContext is an implementation of #GAppLaunchContext that
 * handles launching an application in a graphical context. It provides
 * startup notification and allows to launch applications on a specific
 * screen or workspace.
 *
 * ## Launching an application
 *
 * |[<!-- language="C" -->
 * CdkAppLaunchContext *context;
 *
 * context = cdk_display_get_app_launch_context (display);
 *
 * cdk_app_launch_context_set_screen (screen);
 * cdk_app_launch_context_set_timestamp (event->time);
 *
 * if (!g_app_info_launch_default_for_uri ("http://www.ctk.org", context, &error))
 *   g_warning ("Launching failed: %s\n", error->message);
 *
 * g_object_unref (context);
 * ]|
 */


static void    cdk_app_launch_context_finalize    (GObject           *object);
static gchar * cdk_app_launch_context_get_display (GAppLaunchContext *context,
                                                   GAppInfo          *info,
                                                   GList             *files);
static gchar * cdk_app_launch_context_get_startup_notify_id (GAppLaunchContext *context,
                                                             GAppInfo          *info,
                                                             GList             *files);
static void    cdk_app_launch_context_launch_failed (GAppLaunchContext *context,
                                                     const gchar       *startup_notify_id);


enum
{
  PROP_0,
  PROP_DISPLAY
};

G_DEFINE_TYPE (CdkAppLaunchContext, cdk_app_launch_context, G_TYPE_APP_LAUNCH_CONTEXT)

static void
cdk_app_launch_context_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  CdkAppLaunchContext *context = CDK_APP_LAUNCH_CONTEXT (object);

  switch (prop_id)
    {
    case PROP_DISPLAY:
      g_value_set_object (value, context->display);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
cdk_app_launch_context_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  CdkAppLaunchContext *context = CDK_APP_LAUNCH_CONTEXT (object);

  switch (prop_id)
    {
    case PROP_DISPLAY:
      context->display = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
cdk_app_launch_context_class_init (CdkAppLaunchContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GAppLaunchContextClass *context_class = G_APP_LAUNCH_CONTEXT_CLASS (klass);

  gobject_class->set_property = cdk_app_launch_context_set_property,
  gobject_class->get_property = cdk_app_launch_context_get_property;

  gobject_class->finalize = cdk_app_launch_context_finalize;

  context_class->get_display = cdk_app_launch_context_get_display;
  context_class->get_startup_notify_id = cdk_app_launch_context_get_startup_notify_id;
  context_class->launch_failed = cdk_app_launch_context_launch_failed;

  g_object_class_install_property (gobject_class, PROP_DISPLAY,
    g_param_spec_object ("display", P_("Display"), P_("Display"),
                         CDK_TYPE_DISPLAY,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
}

static void
cdk_app_launch_context_init (CdkAppLaunchContext *context)
{
  context->workspace = -1;
}

static void
cdk_app_launch_context_finalize (GObject *object)
{
  CdkAppLaunchContext *context = CDK_APP_LAUNCH_CONTEXT (object);

  if (context->display)
    g_object_unref (context->display);

  if (context->screen)
    g_object_unref (context->screen);

  if (context->icon)
    g_object_unref (context->icon);

  g_free (context->icon_name);

  G_OBJECT_CLASS (cdk_app_launch_context_parent_class)->finalize (object);
}

static gchar *
cdk_app_launch_context_get_display (GAppLaunchContext *context,
                                    GAppInfo          *info,
                                    GList             *files)
{
  CdkAppLaunchContext *ctx = CDK_APP_LAUNCH_CONTEXT (context);
  CdkDisplay *display;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (ctx->screen)
    return cdk_screen_make_display_name (ctx->screen);
G_GNUC_END_IGNORE_DEPRECATIONS

  if (ctx->display)
    display = ctx->display;
  else
    display = cdk_display_get_default ();

  return g_strdup (cdk_display_get_name (display));
}

/**
 * cdk_app_launch_context_set_display:
 * @context: a #CdkAppLaunchContext
 * @display: a #CdkDisplay
 *
 * Sets the display on which applications will be launched when
 * using this context. See also cdk_app_launch_context_set_screen().
 *
 * Since: 2.14
 *
 * Deprecated: 3.0: Use cdk_display_get_app_launch_context() instead
 */
void
cdk_app_launch_context_set_display (CdkAppLaunchContext *context,
                                    CdkDisplay          *display)
{
  g_return_if_fail (CDK_IS_APP_LAUNCH_CONTEXT (context));
  g_return_if_fail (display == NULL || CDK_IS_DISPLAY (display));

  g_warn_if_fail (display == NULL || display == context->display);
}

/**
 * cdk_app_launch_context_set_screen:
 * @context: a #CdkAppLaunchContext
 * @screen: a #CdkScreen
 *
 * Sets the screen on which applications will be launched when
 * using this context. See also cdk_app_launch_context_set_display().
 *
 * If both @screen and @display are set, the @screen takes priority.
 * If neither @screen or @display are set, the default screen and
 * display are used.
 *
 * Since: 2.14
 */
void
cdk_app_launch_context_set_screen (CdkAppLaunchContext *context,
                                   CdkScreen           *screen)
{
  g_return_if_fail (CDK_IS_APP_LAUNCH_CONTEXT (context));
  g_return_if_fail (screen == NULL || CDK_IS_SCREEN (screen));

  g_return_if_fail (screen == NULL || cdk_screen_get_display (screen) == context->display);

  if (context->screen)
    {
      g_object_unref (context->screen);
      context->screen = NULL;
    }

  if (screen)
    context->screen = g_object_ref (screen);
}

/**
 * cdk_app_launch_context_set_desktop:
 * @context: a #CdkAppLaunchContext
 * @desktop: the number of a workspace, or -1
 *
 * Sets the workspace on which applications will be launched when
 * using this context when running under a window manager that
 * supports multiple workspaces, as described in the
 * [Extended Window Manager Hints](http://www.freedesktop.org/Standards/wm-spec).
 *
 * When the workspace is not specified or @desktop is set to -1,
 * it is up to the window manager to pick one, typically it will
 * be the current workspace.
 *
 * Since: 2.14
 */
void
cdk_app_launch_context_set_desktop (CdkAppLaunchContext *context,
                                    gint                 desktop)
{
  g_return_if_fail (CDK_IS_APP_LAUNCH_CONTEXT (context));

  context->workspace = desktop;
}

/**
 * cdk_app_launch_context_set_timestamp:
 * @context: a #CdkAppLaunchContext
 * @timestamp: a timestamp
 *
 * Sets the timestamp of @context. The timestamp should ideally
 * be taken from the event that triggered the launch.
 *
 * Window managers can use this information to avoid moving the
 * focus to the newly launched application when the user is busy
 * typing in another window. This is also known as 'focus stealing
 * prevention'.
 *
 * Since: 2.14
 */
void
cdk_app_launch_context_set_timestamp (CdkAppLaunchContext *context,
                                      guint32              timestamp)
{
  g_return_if_fail (CDK_IS_APP_LAUNCH_CONTEXT (context));

  context->timestamp = timestamp;
}

/**
 * cdk_app_launch_context_set_icon:
 * @context: a #CdkAppLaunchContext
 * @icon: (allow-none): a #GIcon, or %NULL
 *
 * Sets the icon for applications that are launched with this
 * context.
 *
 * Window Managers can use this information when displaying startup
 * notification.
 *
 * See also cdk_app_launch_context_set_icon_name().
 *
 * Since: 2.14
 */
void
cdk_app_launch_context_set_icon (CdkAppLaunchContext *context,
                                 GIcon               *icon)
{
  g_return_if_fail (CDK_IS_APP_LAUNCH_CONTEXT (context));
  g_return_if_fail (icon == NULL || G_IS_ICON (icon));

  if (context->icon)
    {
      g_object_unref (context->icon);
      context->icon = NULL;
    }

  if (icon)
    context->icon = g_object_ref (icon);
}

/**
 * cdk_app_launch_context_set_icon_name:
 * @context: a #CdkAppLaunchContext
 * @icon_name: (allow-none): an icon name, or %NULL
 *
 * Sets the icon for applications that are launched with this context.
 * The @icon_name will be interpreted in the same way as the Icon field
 * in desktop files. See also cdk_app_launch_context_set_icon().
 *
 * If both @icon and @icon_name are set, the @icon_name takes priority.
 * If neither @icon or @icon_name is set, the icon is taken from either
 * the file that is passed to launched application or from the #GAppInfo
 * for the launched application itself.
 *
 * Since: 2.14
 */
void
cdk_app_launch_context_set_icon_name (CdkAppLaunchContext *context,
                                      const char          *icon_name)
{
  g_return_if_fail (CDK_IS_APP_LAUNCH_CONTEXT (context));

  g_free (context->icon_name);
  context->icon_name = g_strdup (icon_name);
}

/**
 * cdk_app_launch_context_new:
 *
 * Creates a new #CdkAppLaunchContext.
 *
 * Returns: a new #CdkAppLaunchContext
 *
 * Since: 2.14
 *
 * Deprecated: 3.0: Use cdk_display_get_app_launch_context() instead
 */
CdkAppLaunchContext *
cdk_app_launch_context_new (void)
{
  return cdk_display_get_app_launch_context (cdk_display_get_default ());
}

static char *
cdk_app_launch_context_get_startup_notify_id (GAppLaunchContext *context,
                                              GAppInfo          *info,
                                              GList             *files)
{
 return NULL;
}

static void
cdk_app_launch_context_launch_failed (GAppLaunchContext *context,
                                      const gchar       *startup_notify_id)
{
}
