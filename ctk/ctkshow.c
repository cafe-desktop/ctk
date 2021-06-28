/*
 * CTK - The GIMP Toolkit
 * Copyright (C) 2008  Jaap Haitsma <jaap@haitsma.org>
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cdk/cdk.h>

#include "ctkshow.h"
#include "ctkwindowprivate.h"

/**
 * ctk_show_uri:
 * @screen: (allow-none): screen to show the uri on
 *     or %NULL for the default screen
 * @uri: the uri to show
 * @timestamp: a timestamp to prevent focus stealing
 * @error: a #GError that is returned in case of errors
 *
 * A convenience function for launching the default application
 * to show the uri. Like ctk_show_uri_on_window(), but takes a screen
 * as transient parent instead of a window.
 *
 * Note that this function is deprecated as it does not pass the necessary
 * information for helpers to parent their dialog properly, when run from
 * sandboxed applications for example.
 *
 * Returns: %TRUE on success, %FALSE on error
 *
 * Since: 2.14
 *
 * Deprecated: 3.22: Use ctk_show_uri_on_window() instead.
 */
gboolean
ctk_show_uri (CdkScreen    *screen,
              const gchar  *uri,
              guint32       timestamp,
              GError      **error)
{
  CdkAppLaunchContext *context;
  gboolean ret;
  CdkDisplay *display;

  g_return_val_if_fail (uri != NULL, FALSE);

  if (screen != NULL)
    display = cdk_screen_get_display (screen);
  else
    display = cdk_display_get_default ();

  context = cdk_display_get_app_launch_context (display);
  cdk_app_launch_context_set_screen (context, screen);
  cdk_app_launch_context_set_timestamp (context, timestamp);

  ret = g_app_info_launch_default_for_uri (uri, G_APP_LAUNCH_CONTEXT (context), error);
  g_object_unref (context);

  return ret;
}

static void
launch_uri_done (GObject      *source,
                 GAsyncResult *result,
                 gpointer      data)
{
  CtkWindow *window = data;

  g_app_info_launch_default_for_uri_finish (result, NULL);

  if (window)
    ctk_window_unexport_handle (window);
}

static void
window_handle_exported (CtkWindow  *window,
                        const char *handle_str,
                        gpointer    user_data)
{
  GAppLaunchContext *context = user_data;
  const char *uri;

  uri = (const char *)g_object_get_data (G_OBJECT (context), "uri");

  g_app_launch_context_setenv (context, "PARENT_WINDOW_ID", handle_str);

  g_app_info_launch_default_for_uri_async (uri, G_APP_LAUNCH_CONTEXT (context), NULL, launch_uri_done, window);

  g_object_unref (context);
}

/**
 * ctk_show_uri_on_window:
 * @parent: (allow-none): parent window
 * @uri: the uri to show
 * @timestamp: a timestamp to prevent focus stealing
 * @error: a #GError that is returned in case of errors
 *
 * This is a convenience function for launching the default application
 * to show the uri. The uri must be of a form understood by GIO (i.e. you
 * need to install gvfs to get support for uri schemes such as http://
 * or ftp://, as only local files are handled by GIO itself).
 * Typical examples are
 * - `file:///home/gnome/pict.jpg`
 * - `http://www.gnome.org`
 * - `mailto:me@gnome.org`
 *
 * Ideally the timestamp is taken from the event triggering
 * the ctk_show_uri() call. If timestamp is not known you can take
 * %GDK_CURRENT_TIME.
 *
 * This is the recommended call to be used as it passes information
 * necessary for sandbox helpers to parent their dialogs properly.
 *
 * Returns: %TRUE on success, %FALSE on error
 *
 * Since: 3.22
 */
gboolean
ctk_show_uri_on_window (CtkWindow   *parent,
                        const char  *uri,
                        guint32      timestamp,
                        GError     **error)
{
  CdkAppLaunchContext *context;
  gboolean ret;
  CdkDisplay *display;

  g_return_val_if_fail (uri != NULL, FALSE);

  if (parent)
    display = ctk_widget_get_display (CTK_WIDGET (parent));
  else
    display = cdk_display_get_default ();

  context = cdk_display_get_app_launch_context (display);
  cdk_app_launch_context_set_timestamp (context, timestamp);

  g_object_set_data_full (G_OBJECT (context), "uri", g_strdup (uri), g_free);

  if (parent && ctk_window_export_handle (parent, window_handle_exported, context))
    return TRUE;

  ret = g_app_info_launch_default_for_uri (uri, G_APP_LAUNCH_CONTEXT (context), error);
  g_object_unref (context);

  return ret;
}
