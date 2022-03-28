/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2009 Carlos Garnacho <carlosg@gnome.org>
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

#include "cdkeventsource.h"
#include "cdkdevicemanager-broadway.h"

#include "cdkinternals.h"

#include <stdlib.h>

static gboolean cdk_event_source_prepare  (GSource     *source,
                                           gint        *timeout);
static gboolean cdk_event_source_check    (GSource     *source);
static gboolean cdk_event_source_dispatch (GSource     *source,
                                           GSourceFunc  callback,
                                           gpointer     user_data);
static void     cdk_event_source_finalize (GSource     *source);

#define HAS_FOCUS(toplevel)                           \
  ((toplevel)->has_focus || (toplevel)->has_pointer_focus)

struct _CdkEventSource
{
  GSource source;

  CdkDisplay *display;
  GPollFD event_poll_fd;
};

static GSourceFuncs event_funcs = {
  .prepare = cdk_event_source_prepare,
  .check = cdk_event_source_check,
  .dispatch = cdk_event_source_dispatch,
  .finalize = cdk_event_source_finalize
};

static GList *event_sources = NULL;

static gboolean
cdk_event_source_prepare (GSource *source,
                          gint    *timeout)
{
  CdkDisplay *display = ((CdkEventSource*) source)->display;
  gboolean retval;

  cdk_threads_enter ();

  *timeout = -1;

  retval = (_cdk_event_queue_find_first (display) != NULL);

  cdk_threads_leave ();

  return retval;
}

static gboolean
cdk_event_source_check (GSource *source)
{
  CdkEventSource *event_source = (CdkEventSource*) source;
  gboolean retval;

  cdk_threads_enter ();

  if (event_source->display->event_pause_count > 0 ||
      event_source->event_poll_fd.revents & G_IO_IN)
    retval = (_cdk_event_queue_find_first (event_source->display) != NULL);
  else
    retval = FALSE;

  cdk_threads_leave ();

  return retval;
}

void
_cdk_broadway_events_got_input (BroadwayInputMsg *message)
{
  CdkDisplay *display;
  CdkBroadwayDisplay *display_broadway;
  CdkBroadwayDeviceManager *device_manager;
  CdkScreen *screen;
  CdkWindow *window;
  CdkEvent *event = NULL;
  GList *node;
  GSList *list, *d;

  display = NULL;

  list = cdk_display_manager_list_displays (cdk_display_manager_get ());
  for (d = list; d; d = d->next)
    {
      if (CDK_IS_BROADWAY_DISPLAY (d->data))
        {
          display = d->data;
          break;
        }
    }
  g_slist_free (list);

  g_assert (display != NULL);

  display_broadway = CDK_BROADWAY_DISPLAY (display);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  device_manager = CDK_BROADWAY_DEVICE_MANAGER (cdk_display_get_device_manager (display));
  G_GNUC_END_IGNORE_DEPRECATIONS;

  switch (message->base.type) {
  case BROADWAY_EVENT_ENTER:
    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->pointer.event_window_id));
    if (window)
      {
	event = cdk_event_new (CDK_ENTER_NOTIFY);
	event->crossing.window = g_object_ref (window);
	event->crossing.time = message->base.time;
	event->crossing.x = message->pointer.win_x;
	event->crossing.y = message->pointer.win_y;
	event->crossing.x_root = message->pointer.root_x;
	event->crossing.y_root = message->pointer.root_y;
	event->crossing.state = message->pointer.state;
	event->crossing.mode = message->crossing.mode;
	event->crossing.detail = CDK_NOTIFY_ANCESTOR;
	cdk_event_set_device (event, device_manager->core_pointer);
	cdk_event_set_seat (event, cdk_device_get_seat (device_manager->core_pointer));

	node = _cdk_event_queue_append (display, event);
	_cdk_windowing_got_event (display, node, event, message->base.serial);
      }
    break;
  case BROADWAY_EVENT_LEAVE:
    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->pointer.event_window_id));
    if (window)
      {
	event = cdk_event_new (CDK_LEAVE_NOTIFY);
	event->crossing.window = g_object_ref (window);
	event->crossing.time = message->base.time;
	event->crossing.x = message->pointer.win_x;
	event->crossing.y = message->pointer.win_y;
	event->crossing.x_root = message->pointer.root_x;
	event->crossing.y_root = message->pointer.root_y;
	event->crossing.state = message->pointer.state;
	event->crossing.mode = message->crossing.mode;
	event->crossing.detail = CDK_NOTIFY_ANCESTOR;
	cdk_event_set_device (event, device_manager->core_pointer);
	cdk_event_set_seat (event, cdk_device_get_seat (device_manager->core_pointer));

	node = _cdk_event_queue_append (display, event);
	_cdk_windowing_got_event (display, node, event, message->base.serial);
      }
    break;
  case BROADWAY_EVENT_POINTER_MOVE:
    if (_cdk_broadway_moveresize_handle_event (display, message))
      break;

    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->pointer.event_window_id));
    if (window)
      {
	event = cdk_event_new (CDK_MOTION_NOTIFY);
	event->motion.window = g_object_ref (window);
	event->motion.time = message->base.time;
	event->motion.x = message->pointer.win_x;
	event->motion.y = message->pointer.win_y;
	event->motion.x_root = message->pointer.root_x;
	event->motion.y_root = message->pointer.root_y;
	event->motion.state = message->pointer.state;
	cdk_event_set_device (event, device_manager->core_pointer);
	cdk_event_set_seat (event, cdk_device_get_seat (device_manager->core_pointer));

	node = _cdk_event_queue_append (display, event);
	_cdk_windowing_got_event (display, node, event, message->base.serial);
      }

    break;
  case BROADWAY_EVENT_BUTTON_PRESS:
  case BROADWAY_EVENT_BUTTON_RELEASE:
    if (message->base.type != 'b' &&
	_cdk_broadway_moveresize_handle_event (display, message))
      break;

    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->pointer.event_window_id));
    if (window)
      {
	event = cdk_event_new (message->base.type == 'b' ? CDK_BUTTON_PRESS : CDK_BUTTON_RELEASE);
	event->button.window = g_object_ref (window);
	event->button.time = message->base.time;
	event->button.x = message->pointer.win_x;
	event->button.y = message->pointer.win_y;
	event->button.x_root = message->pointer.root_x;
	event->button.y_root = message->pointer.root_y;
	event->button.button = message->button.button;
	event->button.state = message->pointer.state;
	cdk_event_set_device (event, device_manager->core_pointer);
	cdk_event_set_seat (event, cdk_device_get_seat (device_manager->core_pointer));

	node = _cdk_event_queue_append (display, event);
	_cdk_windowing_got_event (display, node, event, message->base.serial);
      }

    break;
  case BROADWAY_EVENT_SCROLL:
    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->pointer.event_window_id));
    if (window)
      {
	event = cdk_event_new (CDK_SCROLL);
	event->scroll.window = g_object_ref (window);
	event->scroll.time = message->base.time;
	event->scroll.x = message->pointer.win_x;
	event->scroll.y = message->pointer.win_y;
	event->scroll.x_root = message->pointer.root_x;
	event->scroll.y_root = message->pointer.root_y;
        event->scroll.state = message->pointer.state;
	event->scroll.direction = message->scroll.dir == 0 ? CDK_SCROLL_UP : CDK_SCROLL_DOWN;
	cdk_event_set_device (event, device_manager->core_pointer);
	cdk_event_set_seat (event, cdk_device_get_seat (device_manager->core_pointer));

	node = _cdk_event_queue_append (display, event);
	_cdk_windowing_got_event (display, node, event, message->base.serial);
      }

    break;
  case BROADWAY_EVENT_TOUCH:
    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->touch.event_window_id));
    if (window)
      {
        CdkEventType event_type = 0;

        switch (message->touch.touch_type) {
        case 0:
          event_type = CDK_TOUCH_BEGIN;
          break;
        case 1:
          event_type = CDK_TOUCH_UPDATE;
          break;
        case 2:
          event_type = CDK_TOUCH_END;
          break;
        default:
          g_printerr ("_cdk_broadway_events_got_input - Unknown touch type %d\n", message->touch.touch_type);
        }

        if (event_type != CDK_TOUCH_BEGIN &&
            message->touch.is_emulated && _cdk_broadway_moveresize_handle_event (display, message))
          break;

	event = cdk_event_new (event_type);
	event->touch.window = g_object_ref (window);
	event->touch.sequence = GUINT_TO_POINTER(message->touch.sequence_id);
	event->touch.emulating_pointer = message->touch.is_emulated;
	event->touch.time = message->base.time;
	event->touch.x = message->touch.win_x;
	event->touch.y = message->touch.win_y;
	event->touch.x_root = message->touch.root_x;
	event->touch.y_root = message->touch.root_y;
	event->touch.state = message->touch.state;

	cdk_event_set_device (event, device_manager->core_pointer);
	cdk_event_set_source_device (event, device_manager->touchscreen);
	cdk_event_set_seat (event, cdk_device_get_seat (device_manager->core_pointer));

        if (message->touch.is_emulated)
          cdk_event_set_pointer_emulated (event, TRUE);

        if (event_type == CDK_TOUCH_BEGIN || event_type == CDK_TOUCH_UPDATE)
          event->touch.state |= CDK_BUTTON1_MASK;

	node = _cdk_event_queue_append (display, event);
	_cdk_windowing_got_event (display, node, event, message->base.serial);
      }

    break;
  case BROADWAY_EVENT_KEY_PRESS:
  case BROADWAY_EVENT_KEY_RELEASE:
    window = g_hash_table_lookup (display_broadway->id_ht,
				  GINT_TO_POINTER (message->key.window_id));
    if (window)
      {
	event = cdk_event_new (message->base.type == 'k' ? CDK_KEY_PRESS : CDK_KEY_RELEASE);
	event->key.window = g_object_ref (window);
	event->key.time = message->base.time;
	event->key.keyval = message->key.key;
	event->key.state = message->key.state;
	event->key.hardware_keycode = message->key.key;
        cdk_event_set_scancode (event, message->key.key);
	event->key.length = 0;
	cdk_event_set_device (event, device_manager->core_keyboard);
	cdk_event_set_seat (event, cdk_device_get_seat (device_manager->core_keyboard));

	node = _cdk_event_queue_append (display, event);
	_cdk_windowing_got_event (display, node, event, message->base.serial);
      }

    break;
  case BROADWAY_EVENT_GRAB_NOTIFY:
  case BROADWAY_EVENT_UNGRAB_NOTIFY:
    _cdk_display_device_grab_update (display, device_manager->core_pointer, device_manager->core_pointer, message->base.serial);
    break;

  case BROADWAY_EVENT_CONFIGURE_NOTIFY:
    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->configure_notify.id));
    if (window)
      {
	window->x = message->configure_notify.x;
	window->y = message->configure_notify.y;

	event = cdk_event_new (CDK_CONFIGURE);
	event->configure.window = g_object_ref (window);
	event->configure.x = message->configure_notify.x;
	event->configure.y = message->configure_notify.y;
	event->configure.width = message->configure_notify.width;
	event->configure.height = message->configure_notify.height;

	node = _cdk_event_queue_append (display, event);
	_cdk_windowing_got_event (display, node, event, message->base.serial);

	if (window->resize_count >= 1)
	  {
	    window->resize_count -= 1;

	    if (window->resize_count == 0)
	      _cdk_broadway_moveresize_configure_done (display, window);
	  }
      }
    break;

  case BROADWAY_EVENT_DELETE_NOTIFY:
    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->delete_notify.id));
    if (window)
      {
	event = cdk_event_new (CDK_DELETE);
	event->any.window = g_object_ref (window);

	node = _cdk_event_queue_append (display, event);
	_cdk_windowing_got_event (display, node, event, message->base.serial);
      }
    break;

  case BROADWAY_EVENT_SCREEN_SIZE_CHANGED:
    screen = cdk_display_get_default_screen (display);
    window = cdk_screen_get_root_window (screen);
    window->width = message->screen_resize_notify.width;
    window->height = message->screen_resize_notify.height;

    _cdk_window_update_size (window);
    _cdk_broadway_screen_size_changed (screen, &message->screen_resize_notify);
    break;

  case BROADWAY_EVENT_FOCUS:
    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->focus.old_id));
    if (window)
      {
	event = cdk_event_new (CDK_FOCUS_CHANGE);
	event->focus_change.window = g_object_ref (window);
	event->focus_change.in = FALSE;
	cdk_event_set_device (event, device_manager->core_pointer);
	cdk_event_set_seat (event, cdk_device_get_seat (device_manager->core_pointer));
	node = _cdk_event_queue_append (display, event);
	_cdk_windowing_got_event (display, node, event, message->base.serial);
      }
    window = g_hash_table_lookup (display_broadway->id_ht, GINT_TO_POINTER (message->focus.new_id));
    if (window)
      {
	event = cdk_event_new (CDK_FOCUS_CHANGE);
	event->focus_change.window = g_object_ref (window);
	event->focus_change.in = TRUE;
	cdk_event_set_device (event, device_manager->core_pointer);
	cdk_event_set_seat (event, cdk_device_get_seat (device_manager->core_pointer));
	node = _cdk_event_queue_append (display, event);
	_cdk_windowing_got_event (display, node, event, message->base.serial);
      }
    break;

  default:
    g_printerr ("_cdk_broadway_events_got_input - Unknown input command %c\n", message->base.type);
    break;
  }
}

void
_cdk_broadway_display_queue_events (CdkDisplay *display)
{
}

static gboolean
cdk_event_source_dispatch (GSource     *source,
                           GSourceFunc  callback,
                           gpointer     user_data)
{
  CdkDisplay *display = ((CdkEventSource*) source)->display;
  CdkEvent *event;

  cdk_threads_enter ();

  event = cdk_display_get_event (display);

  if (event)
    {
      _cdk_event_emit (event);

      cdk_event_free (event);
    }

  cdk_threads_leave ();

  return TRUE;
}

static void
cdk_event_source_finalize (GSource *source)
{
  CdkEventSource *event_source = (CdkEventSource *)source;

  event_sources = g_list_remove (event_sources, event_source);
}

GSource *
_cdk_broadway_event_source_new (CdkDisplay *display)
{
  GSource *source;
  CdkEventSource *event_source;
  char *name;

  source = g_source_new (&event_funcs, sizeof (CdkEventSource));
  name = g_strdup_printf ("CDK Broadway Event source (%s)",
			  cdk_display_get_name (display));
  g_source_set_name (source, name);
  g_free (name);
  event_source = (CdkEventSource *) source;
  event_source->display = display;

  g_source_set_priority (source, CDK_PRIORITY_EVENTS);
  g_source_set_can_recurse (source, TRUE);
  g_source_attach (source, NULL);

  event_sources = g_list_prepend (event_sources, source);

  return source;
}
