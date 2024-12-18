/* CDK - The GIMP Drawing Kit
 * cdkdisplay-broadway.c
 * 
 * Copyright 2001 Sun Microsystems Inc.
 * Copyright (C) 2004 Nokia Corporation
 *
 * Erwann Chenede <erwann.chenede@sun.com>
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

#include "cdkdisplay-broadway.h"

#include "cdkdisplay.h"
#include "cdkeventsource.h"
#include "cdkscreen.h"
#include "cdkscreen-broadway.h"
#include "cdkmonitor-broadway.h"
#include "cdkinternals.h"
#include "cdkdeviceprivate.h"
#include "cdkdevicemanager-broadway.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>

static void   cdk_broadway_display_dispose            (GObject            *object);
static void   cdk_broadway_display_finalize           (GObject            *object);

#if 0
#define DEBUG_WEBSOCKETS 1
#endif

G_DEFINE_TYPE (CdkBroadwayDisplay, cdk_broadway_display, CDK_TYPE_DISPLAY)

static void
cdk_broadway_display_init (CdkBroadwayDisplay *display)
{
  display->id_ht = g_hash_table_new (NULL, NULL);

  display->monitor = g_object_new (CDK_TYPE_BROADWAY_MONITOR,
                                   "display", display,
                                   NULL);
  cdk_monitor_set_manufacturer (display->monitor, "browser");
  cdk_monitor_set_model (display->monitor, "0");
}

static void
cdk_event_init (CdkDisplay *display)
{
  CdkBroadwayDisplay *broadway_display;

  broadway_display = CDK_BROADWAY_DISPLAY (display);
  broadway_display->event_source = _cdk_broadway_event_source_new (display);
}

CdkDisplay *
_cdk_broadway_display_open (const gchar *display_name)
{
  CdkDisplay *display;
  CdkBroadwayDisplay *broadway_display;
  GError *error = NULL;

  display = g_object_new (CDK_TYPE_BROADWAY_DISPLAY, NULL);
  broadway_display = CDK_BROADWAY_DISPLAY (display);

  /* initialize the display's screens */
  broadway_display->screens = g_new (CdkScreen *, 1);
  broadway_display->screens[0] = _cdk_broadway_screen_new (display, 0);

  /* We need to initialize events after we have the screen
   * structures in places
   */
  _cdk_broadway_screen_events_init (broadway_display->screens[0]);

  /*set the default screen */
  broadway_display->default_screen = broadway_display->screens[0];

  display->device_manager = _cdk_broadway_device_manager_new (display);

  cdk_event_init (display);

  _cdk_broadway_display_init_dnd (display);

  _cdk_broadway_screen_setup (broadway_display->screens[0]);

  if (display_name == NULL)
    display_name = g_getenv ("BROADWAY_DISPLAY");

  broadway_display->server = _cdk_broadway_server_new (display_name, &error);
  if (broadway_display->server == NULL)
    {
      g_printerr ("Unable to init server: %s\n", error->message);
      g_error_free (error);
      return NULL;
    }

  g_signal_emit_by_name (display, "opened");

  return display;
}

static const gchar *
cdk_broadway_display_get_name (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return (gchar *) "Broadway";
}

static CdkScreen *
cdk_broadway_display_get_default_screen (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return CDK_BROADWAY_DISPLAY (display)->default_screen;
}

static void
cdk_broadway_display_beep (CdkDisplay *display)
{
  g_return_if_fail (CDK_IS_DISPLAY (display));
}

static void
cdk_broadway_display_sync (CdkDisplay *display)
{
  CdkBroadwayDisplay *broadway_display = CDK_BROADWAY_DISPLAY (display);

  g_return_if_fail (CDK_IS_BROADWAY_DISPLAY (display));

  _cdk_broadway_server_sync (broadway_display->server);
}

static void
cdk_broadway_display_flush (CdkDisplay *display)
{
  CdkBroadwayDisplay *broadway_display = CDK_BROADWAY_DISPLAY (display);

  g_return_if_fail (CDK_IS_BROADWAY_DISPLAY (display));

  _cdk_broadway_server_flush (broadway_display->server);
}

static gboolean
cdk_broadway_display_has_pending (CdkDisplay *display G_GNUC_UNUSED)
{
  return FALSE;
}

static CdkWindow *
cdk_broadway_display_get_default_group (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return NULL;
}

static void
cdk_broadway_display_dispose (GObject *object)
{
  CdkBroadwayDisplay *broadway_display = CDK_BROADWAY_DISPLAY (object);

  _cdk_screen_close (broadway_display->screens[0]);

  if (broadway_display->event_source)
    {
      g_source_destroy (broadway_display->event_source);
      g_source_unref (broadway_display->event_source);
      broadway_display->event_source = NULL;
    }

  G_OBJECT_CLASS (cdk_broadway_display_parent_class)->dispose (object);
}

static void
cdk_broadway_display_finalize (GObject *object)
{
  CdkBroadwayDisplay *broadway_display = CDK_BROADWAY_DISPLAY (object);

  /* Keymap */
  if (broadway_display->keymap)
    g_object_unref (broadway_display->keymap);

  _cdk_broadway_cursor_display_finalize (CDK_DISPLAY_OBJECT(broadway_display));

  /* Free all CdkScreens */
  g_object_unref (broadway_display->screens[0]);
  g_free (broadway_display->screens);

  g_object_unref (broadway_display->monitor);

  G_OBJECT_CLASS (cdk_broadway_display_parent_class)->finalize (object);
}

static void
cdk_broadway_display_notify_startup_complete (CdkDisplay  *display G_GNUC_UNUSED,
					      const gchar *startup_id G_GNUC_UNUSED)
{
}

static gboolean
cdk_broadway_display_supports_selection_notification (CdkDisplay *display G_GNUC_UNUSED)
{
  return FALSE;
}

static gboolean
cdk_broadway_display_request_selection_notification (CdkDisplay *display G_GNUC_UNUSED,
						     CdkAtom     selection G_GNUC_UNUSED)

{
    return FALSE;
}

static gboolean
cdk_broadway_display_supports_clipboard_persistence (CdkDisplay *display G_GNUC_UNUSED)
{
  return FALSE;
}

static void
cdk_broadway_display_store_clipboard (CdkDisplay    *display G_GNUC_UNUSED,
				      CdkWindow     *clipboard_window G_GNUC_UNUSED,
				      guint32        time_ G_GNUC_UNUSED,
				      const CdkAtom *targets G_GNUC_UNUSED,
				      gint           n_targets G_GNUC_UNUSED)
{
}

static gboolean
cdk_broadway_display_supports_shapes (CdkDisplay *display G_GNUC_UNUSED)
{
  return FALSE;
}

static gboolean
cdk_broadway_display_supports_input_shapes (CdkDisplay *display G_GNUC_UNUSED)
{
  return FALSE;
}

static gboolean
cdk_broadway_display_supports_composite (CdkDisplay *display G_GNUC_UNUSED)
{
  return FALSE;
}

static gulong
cdk_broadway_display_get_next_serial (CdkDisplay *display)
{
  CdkBroadwayDisplay *broadway_display;
  broadway_display = CDK_BROADWAY_DISPLAY (display);

  return _cdk_broadway_server_get_next_serial (broadway_display->server);
}

void
cdk_broadway_display_show_keyboard (CdkBroadwayDisplay *display)
{
  g_return_if_fail (CDK_IS_BROADWAY_DISPLAY (display));

  _cdk_broadway_server_set_show_keyboard (display->server, TRUE);
}

void
cdk_broadway_display_hide_keyboard (CdkBroadwayDisplay *display)
{
  g_return_if_fail (CDK_IS_BROADWAY_DISPLAY (display));

  _cdk_broadway_server_set_show_keyboard (display->server, FALSE);
}

static int
cdk_broadway_display_get_n_monitors (CdkDisplay *display G_GNUC_UNUSED)
{
  return 1;
}

static CdkMonitor *
cdk_broadway_display_get_monitor (CdkDisplay *display,
                                  int         monitor_num)
{
  CdkBroadwayDisplay *broadway_display = CDK_BROADWAY_DISPLAY (display);

  if (monitor_num == 0)
    return broadway_display->monitor;

  return NULL;
}

static CdkMonitor *
cdk_broadway_display_get_primary_monitor (CdkDisplay *display)
{
  CdkBroadwayDisplay *broadway_display = CDK_BROADWAY_DISPLAY (display);

  return broadway_display->monitor;
}

static void
cdk_broadway_display_class_init (CdkBroadwayDisplayClass * class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CdkDisplayClass *display_class = CDK_DISPLAY_CLASS (class);

  object_class->dispose = cdk_broadway_display_dispose;
  object_class->finalize = cdk_broadway_display_finalize;

  display_class->window_type = CDK_TYPE_BROADWAY_WINDOW;

  display_class->get_name = cdk_broadway_display_get_name;
  display_class->get_default_screen = cdk_broadway_display_get_default_screen;
  display_class->beep = cdk_broadway_display_beep;
  display_class->sync = cdk_broadway_display_sync;
  display_class->flush = cdk_broadway_display_flush;
  display_class->has_pending = cdk_broadway_display_has_pending;
  display_class->queue_events = _cdk_broadway_display_queue_events;
  display_class->get_default_group = cdk_broadway_display_get_default_group;
  display_class->supports_selection_notification = cdk_broadway_display_supports_selection_notification;
  display_class->request_selection_notification = cdk_broadway_display_request_selection_notification;
  display_class->supports_clipboard_persistence = cdk_broadway_display_supports_clipboard_persistence;
  display_class->store_clipboard = cdk_broadway_display_store_clipboard;
  display_class->supports_shapes = cdk_broadway_display_supports_shapes;
  display_class->supports_input_shapes = cdk_broadway_display_supports_input_shapes;
  display_class->supports_composite = cdk_broadway_display_supports_composite;
  display_class->get_cursor_for_type = _cdk_broadway_display_get_cursor_for_type;
  display_class->get_cursor_for_name = _cdk_broadway_display_get_cursor_for_name;
  display_class->get_cursor_for_surface = _cdk_broadway_display_get_cursor_for_surface;
  display_class->get_default_cursor_size = _cdk_broadway_display_get_default_cursor_size;
  display_class->get_maximal_cursor_size = _cdk_broadway_display_get_maximal_cursor_size;
  display_class->supports_cursor_alpha = _cdk_broadway_display_supports_cursor_alpha;
  display_class->supports_cursor_color = _cdk_broadway_display_supports_cursor_color;

  display_class->before_process_all_updates = _cdk_broadway_display_before_process_all_updates;
  display_class->after_process_all_updates = _cdk_broadway_display_after_process_all_updates;
  display_class->get_next_serial = cdk_broadway_display_get_next_serial;
  display_class->notify_startup_complete = cdk_broadway_display_notify_startup_complete;
  display_class->create_window_impl = _cdk_broadway_display_create_window_impl;
  display_class->get_keymap = _cdk_broadway_display_get_keymap;
  display_class->get_selection_owner = _cdk_broadway_display_get_selection_owner;
  display_class->set_selection_owner = _cdk_broadway_display_set_selection_owner;
  display_class->send_selection_notify = _cdk_broadway_display_send_selection_notify;
  display_class->get_selection_property = _cdk_broadway_display_get_selection_property;
  display_class->convert_selection = _cdk_broadway_display_convert_selection;
  display_class->text_property_to_utf8_list = _cdk_broadway_display_text_property_to_utf8_list;
  display_class->utf8_to_string_target = _cdk_broadway_display_utf8_to_string_target;

  display_class->get_n_monitors = cdk_broadway_display_get_n_monitors;
  display_class->get_monitor = cdk_broadway_display_get_monitor;
  display_class->get_primary_monitor = cdk_broadway_display_get_primary_monitor;
}

