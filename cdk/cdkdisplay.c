/* CDK - The GIMP Drawing Kit
 * cdkdisplay.c
 * 
 * Copyright 2001 Sun Microsystems Inc. 
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

#include "cdkdisplay.h"
#include "cdkdisplayprivate.h"

#include "cdk-private.h"

#include "cdkdeviceprivate.h"
#include "cdkdisplaymanagerprivate.h"
#include "cdkevents.h"
#include "cdkwindowimpl.h"
#include "cdkinternals.h"
#include "cdkmarshalers.h"
#include "cdkscreen.h"
#include "cdkmonitorprivate.h"

#include <math.h>
#include <glib.h>

/* for the use of round() */
#include "fallback-c89.c"

/**
 * SECTION:cdkdisplay
 * @Short_description: Controls a set of CdkScreens and their associated input devices
 * @Title: CdkDisplay
 *
 * #CdkDisplay objects purpose are two fold:
 *
 * - To manage and provide information about input devices (pointers and keyboards)
 *
 * - To manage and provide information about the available #CdkScreens
 *
 * CdkDisplay objects are the CDK representation of an X Display,
 * which can be described as a workstation consisting of
 * a keyboard, a pointing device (such as a mouse) and one or more
 * screens.
 * It is used to open and keep track of various CdkScreen objects
 * currently instantiated by the application. It is also used to
 * access the keyboard(s) and mouse pointer(s) of the display.
 *
 * Most of the input device handling has been factored out into
 * the separate #CdkDeviceManager object. Every display has a
 * device manager, which you can obtain using
 * cdk_display_get_device_manager().
 */


enum {
  OPENED,
  CLOSED,
  SEAT_ADDED,
  SEAT_REMOVED,
  MONITOR_ADDED,
  MONITOR_REMOVED,
  LAST_SIGNAL
};

static void cdk_display_dispose     (GObject         *object);
static void cdk_display_finalize    (GObject         *object);
static void cdk_display_put_event_nocopy (CdkDisplay *display,
                                          CdkEvent   *event);


static CdkAppLaunchContext *cdk_display_real_get_app_launch_context (CdkDisplay *display);

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (CdkDisplay, cdk_display, G_TYPE_OBJECT)

static void
cdk_display_real_make_default (CdkDisplay *display)
{
}

static void
device_removed_cb (CdkDeviceManager *device_manager,
                   CdkDevice        *device,
                   CdkDisplay       *display)
{
  g_hash_table_remove (display->multiple_click_info, device);
  g_hash_table_remove (display->device_grabs, device);
  g_hash_table_remove (display->pointers_info, device);

  /* FIXME: change core pointer and remove from device list */
}

static void
cdk_display_real_opened (CdkDisplay *display)
{
  CdkDeviceManager *device_manager;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  device_manager = cdk_display_get_device_manager (display);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  g_signal_connect (device_manager, "device-removed",
                    G_CALLBACK (device_removed_cb), display);

  _cdk_display_manager_add_display (cdk_display_manager_get (), display);
}

static void
cdk_display_real_event_data_copy (CdkDisplay     *display,
                                  const CdkEvent *src,
                                  CdkEvent       *dst)
{
}

static void
cdk_display_real_event_data_free (CdkDisplay     *display,
                                  CdkEvent       *dst)
{
}

static CdkSeat *
cdk_display_real_get_default_seat (CdkDisplay *display)
{
  if (!display->seats)
    return NULL;

  return display->seats->data;
}

static void
cdk_display_class_init (CdkDisplayClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = cdk_display_finalize;
  object_class->dispose = cdk_display_dispose;

  class->get_app_launch_context = cdk_display_real_get_app_launch_context;
  class->window_type = CDK_TYPE_WINDOW;

  class->opened = cdk_display_real_opened;
  class->make_default = cdk_display_real_make_default;
  class->event_data_copy = cdk_display_real_event_data_copy;
  class->event_data_free = cdk_display_real_event_data_free;
  class->get_default_seat = cdk_display_real_get_default_seat;

  /**
   * CdkDisplay::opened:
   * @display: the object on which the signal is emitted
   *
   * The ::opened signal is emitted when the connection to the windowing
   * system for @display is opened.
   */
  signals[OPENED] =
    g_signal_new (g_intern_static_string ("opened"),
		  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CdkDisplayClass, opened),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CdkDisplay::closed:
   * @display: the object on which the signal is emitted
   * @is_error: %TRUE if the display was closed due to an error
   *
   * The ::closed signal is emitted when the connection to the windowing
   * system for @display is closed.
   *
   * Since: 2.2
   */   
  signals[CLOSED] =
    g_signal_new (g_intern_static_string ("closed"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CdkDisplayClass, closed),
		  NULL, NULL,
		  _cdk_marshal_VOID__BOOLEAN,
		  G_TYPE_NONE,
		  1,
		  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (signals[CLOSED],
                              G_OBJECT_CLASS_TYPE (object_class),
                              _cdk_marshal_VOID__BOOLEANv);

  /**
   * CdkDisplay::seat-added:
   * @display: the object on which the signal is emitted
   * @seat: the seat that was just added
   *
   * The ::seat-added signal is emitted whenever a new seat is made
   * known to the windowing system.
   *
   * Since: 3.20
   */
  signals[SEAT_ADDED] =
    g_signal_new (g_intern_static_string ("seat-added"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  0, NULL, NULL,
                  NULL,
		  G_TYPE_NONE, 1, CDK_TYPE_SEAT);

  /**
   * CdkDisplay::seat-removed:
   * @display: the object on which the signal is emitted
   * @seat: the seat that was just removed
   *
   * The ::seat-removed signal is emitted whenever a seat is removed
   * by the windowing system.
   *
   * Since: 3.20
   */
  signals[SEAT_REMOVED] =
    g_signal_new (g_intern_static_string ("seat-removed"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  0, NULL, NULL,
                  NULL,
		  G_TYPE_NONE, 1, CDK_TYPE_SEAT);

  /**
   * CdkDisplay::monitor-added:
   * @display: the objedct on which the signal is emitted
   * @monitor: the monitor that was just added
   *
   * The ::monitor-added signal is emitted whenever a monitor is
   * added.
   *
   * Since: 3.22
   */
  signals[MONITOR_ADDED] =
    g_signal_new (g_intern_static_string ("monitor-added"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  0, NULL, NULL,
                  NULL,
		  G_TYPE_NONE, 1, CDK_TYPE_MONITOR);

  /**
   * CdkDisplay::monitor-removed:
   * @display: the object on which the signal is emitted
   * @monitor: the monitor that was just removed
   *
   * The ::monitor-removed signal is emitted whenever a monitor is
   * removed.
   *
   * Since: 3.22
   */
  signals[MONITOR_REMOVED] =
    g_signal_new (g_intern_static_string ("monitor-removed"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  0, NULL, NULL,
                  NULL,
		  G_TYPE_NONE, 1, CDK_TYPE_MONITOR);
}

static void
free_pointer_info (CdkPointerWindowInfo *info)
{
  if (info->toplevel_under_pointer)
    g_object_unref (info->toplevel_under_pointer);
  g_clear_object (&info->last_slave);
  g_slice_free (CdkPointerWindowInfo, info);
}

static void
free_device_grab (CdkDeviceGrabInfo *info)
{
  g_object_unref (info->window);
  g_object_unref (info->native_window);
  g_free (info);
}

static gboolean
free_device_grabs_foreach (gpointer key,
                           gpointer value,
                           gpointer user_data)
{
  GList *list = value;

  g_list_free_full (list, (GDestroyNotify) free_device_grab);

  return TRUE;
}

static void
cdk_display_init (CdkDisplay *display)
{
  display->double_click_time = 250;
  display->double_click_distance = 5;

  display->touch_implicit_grabs = g_array_new (FALSE, FALSE, sizeof (CdkTouchGrabInfo));
  display->device_grabs = g_hash_table_new (NULL, NULL);
  display->motion_hint_info = g_hash_table_new_full (NULL, NULL, NULL,
                                                     (GDestroyNotify) g_free);

  display->pointers_info = g_hash_table_new_full (NULL, NULL, NULL,
                                                  (GDestroyNotify) free_pointer_info);

  display->multiple_click_info = g_hash_table_new_full (NULL, NULL, NULL,
                                                        (GDestroyNotify) g_free);

  display->rendering_mode = _cdk_rendering_mode;
}

static void
cdk_display_dispose (GObject *object)
{
  CdkDisplay *display = CDK_DISPLAY (object);
  CdkDeviceManager *device_manager;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  device_manager = cdk_display_get_device_manager (CDK_DISPLAY (object));
  G_GNUC_END_IGNORE_DEPRECATIONS;

  _cdk_display_manager_remove_display (cdk_display_manager_get (), display);

  g_list_free_full (display->queued_events, (GDestroyNotify) cdk_event_free);
  display->queued_events = NULL;
  display->queued_tail = NULL;

  g_list_foreach (display->input_devices, (GFunc) g_object_run_dispose, NULL);

  if (device_manager)
    {
      /* this is to make it drop devices which may require using the X
       * display and therefore can't be cleaned up in finalize.
       * It will also disconnect device_removed_cb
       */
      g_object_run_dispose (G_OBJECT (display->device_manager));
    }

  G_OBJECT_CLASS (cdk_display_parent_class)->dispose (object);
}

static void
cdk_display_finalize (GObject *object)
{
  CdkDisplay *display = CDK_DISPLAY (object);

  g_hash_table_foreach_remove (display->device_grabs,
                               free_device_grabs_foreach,
                               NULL);
  g_hash_table_destroy (display->device_grabs);

  g_array_free (display->touch_implicit_grabs, TRUE);

  g_hash_table_destroy (display->motion_hint_info);
  g_hash_table_destroy (display->pointers_info);
  g_hash_table_destroy (display->multiple_click_info);

  g_list_free_full (display->input_devices, g_object_unref);
  g_list_free_full (display->seats, g_object_unref);

  if (display->device_manager)
    g_object_unref (display->device_manager);

  G_OBJECT_CLASS (cdk_display_parent_class)->finalize (object);
}

/**
 * cdk_display_close:
 * @display: a #CdkDisplay
 *
 * Closes the connection to the windowing system for the given display,
 * and cleans up associated resources.
 *
 * Since: 2.2
 */
void
cdk_display_close (CdkDisplay *display)
{
  g_return_if_fail (CDK_IS_DISPLAY (display));

  if (!display->closed)
    {
      display->closed = TRUE;
      
      g_signal_emit (display, signals[CLOSED], 0, FALSE);
      g_object_run_dispose (G_OBJECT (display));
      
      g_object_unref (display);
    }
}

/**
 * cdk_display_is_closed:
 * @display: a #CdkDisplay
 *
 * Finds out if the display has been closed.
 *
 * Returns: %TRUE if the display is closed.
 *
 * Since: 2.22
 */
gboolean
cdk_display_is_closed  (CdkDisplay  *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  return display->closed;
}

/**
 * cdk_display_get_event:
 * @display: a #CdkDisplay
 * 
 * Gets the next #CdkEvent to be processed for @display, fetching events from the
 * windowing system if necessary.
 * 
 * Returns: (nullable): the next #CdkEvent to be processed, or %NULL
 * if no events are pending. The returned #CdkEvent should be freed
 * with cdk_event_free().
 *
 * Since: 2.2
 **/
CdkEvent*
cdk_display_get_event (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  if (display->event_pause_count == 0)
    CDK_DISPLAY_GET_CLASS (display)->queue_events (display);

  return _cdk_event_unqueue (display);
}

/**
 * cdk_display_peek_event:
 * @display: a #CdkDisplay 
 * 
 * Gets a copy of the first #CdkEvent in the @display’s event queue, without
 * removing the event from the queue.  (Note that this function will
 * not get more events from the windowing system.  It only checks the events
 * that have already been moved to the CDK event queue.)
 * 
 * Returns: (nullable): a copy of the first #CdkEvent on the event
 * queue, or %NULL if no events are in the queue. The returned
 * #CdkEvent should be freed with cdk_event_free().
 *
 * Since: 2.2
 **/
CdkEvent*
cdk_display_peek_event (CdkDisplay *display)
{
  GList *tmp_list;

  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  tmp_list = _cdk_event_queue_find_first (display);
  
  if (tmp_list)
    return cdk_event_copy (tmp_list->data);
  else
    return NULL;
}

static void
cdk_display_put_event_nocopy (CdkDisplay *display,
                              CdkEvent   *event)
{
  _cdk_event_queue_append (display, event);
  /* If the main loop is blocking in a different thread, wake it up */
  g_main_context_wakeup (NULL);
}

/**
 * cdk_display_put_event:
 * @display: a #CdkDisplay
 * @event: a #CdkEvent.
 *
 * Appends a copy of the given event onto the front of the event
 * queue for @display.
 *
 * Since: 2.2
 **/
void
cdk_display_put_event (CdkDisplay     *display,
		       const CdkEvent *event)
{
  g_return_if_fail (CDK_IS_DISPLAY (display));
  g_return_if_fail (event != NULL);

  cdk_display_put_event_nocopy (display, cdk_event_copy (event));
}

/**
 * cdk_display_pointer_ungrab:
 * @display: a #CdkDisplay.
 * @time_: a timestap (e.g. %CDK_CURRENT_TIME).
 *
 * Release any pointer grab.
 *
 * Since: 2.2
 */
void
cdk_display_pointer_ungrab (CdkDisplay *display,
			    guint32     time_)
{
  GList *seats, *s;
  CdkDevice *device;

  g_return_if_fail (CDK_IS_DISPLAY (display));

  seats = cdk_display_list_seats (display);

  for (s = seats; s; s = s->next)
    {
      device = cdk_seat_get_pointer (s->data);
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      cdk_device_ungrab (device, time_);
      G_GNUC_END_IGNORE_DEPRECATIONS;
    }

  g_list_free (seats);
}

/**
 * cdk_pointer_ungrab:
 * @time_: a timestamp from a #CdkEvent, or %CDK_CURRENT_TIME if no 
 *  timestamp is available.
 *
 * Ungrabs the pointer on the default display, if it is grabbed by this 
 * application.
 **/
void
cdk_pointer_ungrab (guint32 time)
{
  cdk_display_pointer_ungrab (cdk_display_get_default (), time);
}

/**
 * cdk_pointer_is_grabbed:
 * 
 * Returns %TRUE if the pointer on the default display is currently 
 * grabbed by this application.
 *
 * Note that this does not take the inmplicit pointer grab on button
 * presses into account.
 *
 * Returns: %TRUE if the pointer is currently grabbed by this application.
 **/
gboolean
cdk_pointer_is_grabbed (void)
{
  return cdk_display_pointer_is_grabbed (cdk_display_get_default ());
}

/**
 * cdk_display_keyboard_ungrab:
 * @display: a #CdkDisplay.
 * @time_: a timestap (e.g #CDK_CURRENT_TIME).
 *
 * Release any keyboard grab
 *
 * Since: 2.2
 */
void
cdk_display_keyboard_ungrab (CdkDisplay *display,
			     guint32     time)
{
  GList *seats, *s;
  CdkDevice *device;

  g_return_if_fail (CDK_IS_DISPLAY (display));

  seats = cdk_display_list_seats (display);

  for (s = seats; s; s = s->next)
    {
      device = cdk_seat_get_keyboard (s->data);
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      cdk_device_ungrab (device, time);
      G_GNUC_END_IGNORE_DEPRECATIONS;
    }

  g_list_free (seats);
}


/**
 * cdk_keyboard_ungrab:
 * @time_: a timestamp from a #CdkEvent, or %CDK_CURRENT_TIME if no
 *        timestamp is available.
 * 
 * Ungrabs the keyboard on the default display, if it is grabbed by this 
 * application.
 **/
void
cdk_keyboard_ungrab (guint32 time)
{
  cdk_display_keyboard_ungrab (cdk_display_get_default (), time);
}

/**
 * cdk_beep:
 * 
 * Emits a short beep on the default display.
 **/
void
cdk_beep (void)
{
  cdk_display_beep (cdk_display_get_default ());
}

/**
 * cdk_flush:
 *
 * Flushes the output buffers of all display connections and waits
 * until all requests have been processed.
 * This is rarely needed by applications.
 */
void
cdk_flush (void)
{
  GSList *list, *l;

  list = cdk_display_manager_list_displays (cdk_display_manager_get ());
  for (l = list; l; l = l->next)
    {
      CdkDisplay *display = l->data;

      CDK_DISPLAY_GET_CLASS (display)->sync (display);
    }

  g_slist_free (list);
}

void
_cdk_display_enable_motion_hints (CdkDisplay *display,
                                  CdkDevice  *device)
{
  gulong *device_serial, serial;

  device_serial = g_hash_table_lookup (display->motion_hint_info, device);

  if (!device_serial)
    {
      device_serial = g_new0 (gulong, 1);
      *device_serial = G_MAXULONG;
      g_hash_table_insert (display->motion_hint_info, device, device_serial);
    }

  if (*device_serial != 0)
    {
      serial = _cdk_display_get_next_serial (display);
      /* We might not actually generate the next request, so
	 make sure this triggers always, this may cause it to
	 trigger slightly too early, but this is just a hint
	 anyway. */
      if (serial > 0)
	serial--;
      if (serial < *device_serial)
	*device_serial = serial;
    }
}

/**
 * cdk_display_get_pointer:
 * @display: a #CdkDisplay
 * @screen: (out) (allow-none) (transfer none): location to store the screen that the
 *          cursor is on, or %NULL.
 * @x: (out) (allow-none): location to store root window X coordinate of pointer, or %NULL.
 * @y: (out) (allow-none): location to store root window Y coordinate of pointer, or %NULL.
 * @mask: (out) (allow-none): location to store current modifier mask, or %NULL
 *
 * Gets the current location of the pointer and the current modifier
 * mask for a given display.
 *
 * Since: 2.2
 **/
void
cdk_display_get_pointer (CdkDisplay      *display,
			 CdkScreen      **screen,
			 gint            *x,
			 gint            *y,
			 CdkModifierType *mask)
{
  CdkScreen *default_screen;
  CdkSeat *default_seat;
  CdkWindow *root;
  gdouble tmp_x, tmp_y;
  CdkModifierType tmp_mask;

  g_return_if_fail (CDK_IS_DISPLAY (display));

  if (cdk_display_is_closed (display))
    return;

  default_screen = cdk_display_get_default_screen (display);
  default_seat = cdk_display_get_default_seat (display);

  /* We call _cdk_device_query_state() here manually instead of
   * cdk_device_get_position() because we care about the modifier mask */

  _cdk_device_query_state (cdk_seat_get_pointer (default_seat),
                           cdk_screen_get_root_window (default_screen),
                           &root, NULL,
                           &tmp_x, &tmp_y,
                           NULL, NULL,
                           &tmp_mask);

  if (screen)
    *screen = cdk_window_get_screen (root);
  if (x)
    *x = round (tmp_x);
  if (y)
    *y = round (tmp_y);
  if (mask)
    *mask = tmp_mask;
}

/**
 * cdk_display_get_window_at_pointer:
 * @display: a #CdkDisplay
 * @win_x: (out) (allow-none): return location for x coordinate of the pointer location relative
 *    to the window origin, or %NULL
 * @win_y: (out) (allow-none): return location for y coordinate of the pointer location relative
 &    to the window origin, or %NULL
 *
 * Obtains the window underneath the mouse pointer, returning the location
 * of the pointer in that window in @win_x, @win_y for @screen. Returns %NULL
 * if the window under the mouse pointer is not known to CDK (for example, 
 * belongs to another application).
 *
 * Returns: (nullable) (transfer none): the window under the mouse
 *   pointer, or %NULL
 *
 * Since: 2.2
 **/
CdkWindow *
cdk_display_get_window_at_pointer (CdkDisplay *display,
				   gint       *win_x,
				   gint       *win_y)
{
  CdkDevice *pointer;

  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  pointer = cdk_seat_get_pointer (cdk_display_get_default_seat (display));

  return cdk_device_get_window_at_position (pointer, win_x, win_y);
}

static void
generate_grab_broken_event (CdkDisplay *display,
                            CdkWindow  *window,
                            CdkDevice  *device,
			    gboolean    implicit,
			    CdkWindow  *grab_window)
{
  g_return_if_fail (window != NULL);

  if (!CDK_WINDOW_DESTROYED (window))
    {
      CdkEvent *event;

      event = cdk_event_new (CDK_GRAB_BROKEN);
      event->grab_broken.window = g_object_ref (window);
      event->grab_broken.send_event = FALSE;
      event->grab_broken.implicit = implicit;
      event->grab_broken.grab_window = grab_window;
      cdk_event_set_device (event, device);
      event->grab_broken.keyboard = (cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD) ? TRUE : FALSE;

      cdk_display_put_event_nocopy (display, event);
    }
}

CdkDeviceGrabInfo *
_cdk_display_get_last_device_grab (CdkDisplay *display,
                                   CdkDevice  *device)
{
  GList *l;

  l = g_hash_table_lookup (display->device_grabs, device);

  if (l)
    {
      l = g_list_last (l);
      return l->data;
    }

  return NULL;
}

CdkDeviceGrabInfo *
_cdk_display_add_device_grab (CdkDisplay       *display,
                              CdkDevice        *device,
                              CdkWindow        *window,
                              CdkWindow        *native_window,
                              CdkGrabOwnership  grab_ownership,
                              gboolean          owner_events,
                              CdkEventMask      event_mask,
                              unsigned long     serial_start,
                              guint32           time,
                              gboolean          implicit)
{
  CdkDeviceGrabInfo *info, *other_info;
  GList *grabs, *l;

  info = g_new0 (CdkDeviceGrabInfo, 1);

  info->window = g_object_ref (window);
  info->native_window = g_object_ref (native_window);
  info->serial_start = serial_start;
  info->serial_end = G_MAXULONG;
  info->owner_events = owner_events;
  info->event_mask = event_mask;
  info->time = time;
  info->implicit = implicit;
  info->ownership = grab_ownership;

  grabs = g_hash_table_lookup (display->device_grabs, device);

  /* Find the first grab that has a larger start time (if any) and insert
   * before that. I.E we insert after already existing grabs with same
   * start time */
  for (l = grabs; l != NULL; l = l->next)
    {
      other_info = l->data;

      if (info->serial_start < other_info->serial_start)
	break;
    }

  grabs = g_list_insert_before (grabs, l, info);

  /* Make sure the new grab end before next grab */
  if (l)
    {
      other_info = l->data;
      info->serial_end = other_info->serial_start;
    }

  /* Find any previous grab and update its end time */
  l = g_list_find (grabs, info);
  l = l->prev;
  if (l)
    {
      other_info = l->data;
      other_info->serial_end = serial_start;
    }

  g_hash_table_insert (display->device_grabs, device, grabs);

  return info;
}

static void
_cdk_display_break_touch_grabs (CdkDisplay *display,
                                CdkDevice  *device,
                                CdkWindow  *new_grab_window)
{
  guint i;

  for (i = 0; i < display->touch_implicit_grabs->len; i++)
    {
      CdkTouchGrabInfo *info;

      info = &g_array_index (display->touch_implicit_grabs,
                             CdkTouchGrabInfo, i);

      if (info->device == device && info->window != new_grab_window)
        generate_grab_broken_event (display, CDK_WINDOW (info->window),
                                    device, TRUE, new_grab_window);
    }
}

void
_cdk_display_add_touch_grab (CdkDisplay       *display,
                             CdkDevice        *device,
                             CdkEventSequence *sequence,
                             CdkWindow        *window,
                             CdkWindow        *native_window,
                             CdkEventMask      event_mask,
                             unsigned long     serial,
                             guint32           time)
{
  CdkTouchGrabInfo info;

  info.device = device;
  info.sequence = sequence;
  info.window = g_object_ref (window);
  info.native_window = g_object_ref (native_window);
  info.serial = serial;
  info.event_mask = event_mask;
  info.time = time;

  g_array_append_val (display->touch_implicit_grabs, info);
}

gboolean
_cdk_display_end_touch_grab (CdkDisplay       *display,
                             CdkDevice        *device,
                             CdkEventSequence *sequence)
{
  guint i;

  for (i = 0; i < display->touch_implicit_grabs->len; i++)
    {
      CdkTouchGrabInfo *info;

      info = &g_array_index (display->touch_implicit_grabs,
                             CdkTouchGrabInfo, i);

      if (info->device == device && info->sequence == sequence)
        {
          g_array_remove_index_fast (display->touch_implicit_grabs, i);
          return TRUE;
        }
    }

  return FALSE;
}

/* _cdk_synthesize_crossing_events only works inside one toplevel.
   This function splits things into two calls if needed, converting the
   coordinates to the right toplevel */
static void
synthesize_crossing_events (CdkDisplay      *display,
                            CdkDevice       *device,
                            CdkDevice       *source_device,
			    CdkWindow       *src_window,
			    CdkWindow       *dest_window,
			    CdkCrossingMode  crossing_mode,
			    guint32          time,
			    gulong           serial)
{
  CdkWindow *src_toplevel, *dest_toplevel;
  CdkModifierType state;
  double x, y;

  if (src_window)
    src_toplevel = cdk_window_get_toplevel (src_window);
  else
    src_toplevel = NULL;
  if (dest_window)
    dest_toplevel = cdk_window_get_toplevel (dest_window);
  else
    dest_toplevel = NULL;

  if (src_toplevel == NULL && dest_toplevel == NULL)
    return;
  
  if (src_toplevel == NULL ||
      src_toplevel == dest_toplevel)
    {
      /* Same toplevels */
      cdk_window_get_device_position_double (dest_toplevel,
                                             device,
                                             &x, &y, &state);
      _cdk_synthesize_crossing_events (display,
				       src_window,
				       dest_window,
                                       device, source_device,
				       crossing_mode,
				       x, y, state,
				       time,
				       NULL,
				       serial, FALSE);
    }
  else if (dest_toplevel == NULL)
    {
      cdk_window_get_device_position_double (src_toplevel,
                                             device,
                                             &x, &y, &state);
      _cdk_synthesize_crossing_events (display,
                                       src_window,
                                       NULL,
                                       device, source_device,
                                       crossing_mode,
                                       x, y, state,
                                       time,
                                       NULL,
                                       serial, FALSE);
    }
  else
    {
      /* Different toplevels */
      cdk_window_get_device_position_double (src_toplevel,
                                             device,
                                             &x, &y, &state);
      _cdk_synthesize_crossing_events (display,
				       src_window,
				       NULL,
                                       device, source_device,
				       crossing_mode,
				       x, y, state,
				       time,
				       NULL,
				       serial, FALSE);
      cdk_window_get_device_position_double (dest_toplevel,
                                             device,
                                             &x, &y, &state);
      _cdk_synthesize_crossing_events (display,
				       NULL,
				       dest_window,
                                       device, source_device,
				       crossing_mode,
				       x, y, state,
				       time,
				       NULL,
				       serial, FALSE);
    }
}

static CdkWindow *
get_current_toplevel (CdkDisplay      *display,
                      CdkDevice       *device,
                      int             *x_out,
                      int             *y_out,
		      CdkModifierType *state_out)
{
  CdkWindow *pointer_window;
  gdouble x, y;
  CdkModifierType state;

  pointer_window = _cdk_device_window_at_position (device, &x, &y, &state, TRUE);

  if (pointer_window != NULL &&
      (CDK_WINDOW_DESTROYED (pointer_window) ||
       CDK_WINDOW_TYPE (pointer_window) == CDK_WINDOW_ROOT ||
       CDK_WINDOW_TYPE (pointer_window) == CDK_WINDOW_FOREIGN))
    pointer_window = NULL;

  *x_out = round (x);
  *y_out = round (y);
  *state_out = state;

  return pointer_window;
}

static void
switch_to_pointer_grab (CdkDisplay        *display,
                        CdkDevice         *device,
                        CdkDevice         *source_device,
			CdkDeviceGrabInfo *grab,
			CdkDeviceGrabInfo *last_grab,
			guint32            time,
			gulong             serial)
{
  CdkWindow *src_window, *pointer_window, *new_toplevel;
  CdkPointerWindowInfo *info;
  GList *old_grabs;
  CdkModifierType state;
  int x = 0, y = 0;

  /* Temporarily unset pointer to make sure we send the crossing events below */
  old_grabs = g_hash_table_lookup (display->device_grabs, device);
  g_hash_table_steal (display->device_grabs, device);
  info = _cdk_display_get_pointer_info (display, device);

  if (grab)
    {
      /* New grab is in effect */

      /* We need to generate crossing events for the grab.
       * However, there are never any crossing events for implicit grabs
       * TODO: ... Actually, this could happen if the pointer window
       *           doesn't have button mask so a parent gets the event...
       */
      if (!grab->implicit)
	{
	  /* We send GRAB crossing events from the window under the pointer to the
	     grab window. Except if there is an old grab then we start from that */
	  if (last_grab)
	    src_window = last_grab->window;
	  else
	    src_window = info->window_under_pointer;

	  if (src_window != grab->window)
            synthesize_crossing_events (display, device, source_device,
                                        src_window, grab->window,
                                        CDK_CROSSING_GRAB, time, serial);

	  /* !owner_event Grabbing a window that we're not inside, current status is
	     now NULL (i.e. outside grabbed window) */
	  if (!grab->owner_events && info->window_under_pointer != grab->window)
	    _cdk_display_set_window_under_pointer (display, device, NULL);
	}

      grab->activated = TRUE;
    }

  if (last_grab)
    {
      new_toplevel = NULL;

      if (grab == NULL /* ungrab */ ||
	  (!last_grab->owner_events && grab->owner_events) /* switched to owner_events */ )
	{
	  /* We force check what window we're in, and update the toplevel_under_pointer info,
	   * as that won't get told of this change with toplevel enter events.
	   */
	  if (info->toplevel_under_pointer)
	    g_object_unref (info->toplevel_under_pointer);
	  info->toplevel_under_pointer = NULL;

          /* Ungrabbed slave devices don't have a position by
           * itself, rather depend on its master pointer, so
           * it doesn't make sense to track any position for
           * these after the grab
           */
          if (grab || cdk_device_get_device_type (device) != CDK_DEVICE_TYPE_SLAVE)
            new_toplevel = get_current_toplevel (display, device, &x, &y, &state);

	  if (new_toplevel)
	    {
	      /* w is now toplevel and x,y in toplevel coords */
	      info->toplevel_under_pointer = g_object_ref (new_toplevel);
	      info->toplevel_x = x;
	      info->toplevel_y = y;
	      info->state = state;
	    }
	}

      if (grab == NULL) /* Ungrabbed, send events */
	{
          /* If the source device is a touch device, do not
           * propagate any enter event yet, until one is
           * synthesized when needed.
           */
          if (source_device &&
              (cdk_device_get_source (source_device) == CDK_SOURCE_TOUCHSCREEN))
            info->need_touch_press_enter = TRUE;

          pointer_window = NULL;

          if (new_toplevel &&
              !info->need_touch_press_enter)
            {
              /* Find (possibly virtual) child window */
              pointer_window =
                _cdk_window_find_descendant_at (new_toplevel,
                                                x, y,
                                                NULL, NULL);
            }

	  if (!info->need_touch_press_enter &&
	      pointer_window != last_grab->window)
            synthesize_crossing_events (display, device, source_device,
                                        last_grab->window, pointer_window,
                                        CDK_CROSSING_UNGRAB, time, serial);

	  /* We're now ungrabbed, update the window_under_pointer */
	  _cdk_display_set_window_under_pointer (display, device, pointer_window);
	}
    }

  g_hash_table_insert (display->device_grabs, device, old_grabs);
}

void
_cdk_display_update_last_event (CdkDisplay     *display,
                                const CdkEvent *event)
{
  if (cdk_event_get_time (event) != CDK_CURRENT_TIME)
    display->last_event_time = cdk_event_get_time (event);
}

void
_cdk_display_device_grab_update (CdkDisplay *display,
                                 CdkDevice  *device,
                                 CdkDevice  *source_device,
                                 gulong      current_serial)
{
  CdkDeviceGrabInfo *current_grab, *next_grab;
  GList *grabs;
  guint32 time;

  time = display->last_event_time;
  grabs = g_hash_table_lookup (display->device_grabs, device);

  while (grabs != NULL)
    {
      current_grab = grabs->data;

      if (current_grab->serial_start > current_serial)
	return; /* Hasn't started yet */

      if (current_grab->serial_end > current_serial)
	{
	  /* This one hasn't ended yet.
	     its the currently active one or scheduled to be active */

	  if (!current_grab->activated)
            {
              if (cdk_device_get_source (device) != CDK_SOURCE_KEYBOARD)
                switch_to_pointer_grab (display, device, source_device, current_grab, NULL, time, current_serial);
            }

	  break;
	}

      next_grab = NULL;
      if (grabs->next)
	{
	  /* This is the next active grab */
	  next_grab = grabs->next->data;

	  if (next_grab->serial_start > current_serial)
	    next_grab = NULL; /* Actually its not yet active */
	}

      if (next_grab)
        _cdk_display_break_touch_grabs (display, device, next_grab->window);

      if ((next_grab == NULL && current_grab->implicit_ungrab) ||
          (next_grab != NULL && current_grab->window != next_grab->window))
        generate_grab_broken_event (display, CDK_WINDOW (current_grab->window),
                                    device,
                                    current_grab->implicit,
                                    next_grab? next_grab->window : NULL);

      /* Remove old grab */
      grabs = g_list_delete_link (grabs, grabs);
      g_hash_table_insert (display->device_grabs, device, grabs);

      if (cdk_device_get_source (device) != CDK_SOURCE_KEYBOARD)
        switch_to_pointer_grab (display, device, source_device,
                                next_grab, current_grab,
                                time, current_serial);

      free_device_grab (current_grab);
    }
}

static GList *
grab_list_find (GList  *grabs,
                gulong  serial)
{
  CdkDeviceGrabInfo *grab;

  while (grabs)
    {
      grab = grabs->data;

      if (serial >= grab->serial_start && serial < grab->serial_end)
	return grabs;

      grabs = grabs->next;
    }

  return NULL;
}

static GList *
find_device_grab (CdkDisplay *display,
                   CdkDevice  *device,
                   gulong      serial)
{
  GList *l;

  l = g_hash_table_lookup (display->device_grabs, device);
  return grab_list_find (l, serial);
}

CdkDeviceGrabInfo *
_cdk_display_has_device_grab (CdkDisplay *display,
                              CdkDevice  *device,
                              gulong      serial)
{
  GList *l;

  l = find_device_grab (display, device, serial);
  if (l)
    return l->data;

  return NULL;
}

CdkTouchGrabInfo *
_cdk_display_has_touch_grab (CdkDisplay       *display,
                             CdkDevice        *device,
                             CdkEventSequence *sequence,
                             gulong            serial)
{
  guint i;

  g_return_val_if_fail (display, NULL);

  if (!display->touch_implicit_grabs)
    return NULL;

  for (i = 0; i < display->touch_implicit_grabs->len; i++)
    {
      CdkTouchGrabInfo *info;

      info = &g_array_index (display->touch_implicit_grabs,
                             CdkTouchGrabInfo, i);

      if (info->device == device && info->sequence == sequence)
        {
          if (serial >= info->serial)
            return info;
          else
            return NULL;
        }
    }

  return NULL;
}

/* Returns true if last grab was ended
 * If if_child is non-NULL, end the grab only if the grabbed
 * window is the same as if_child or a descendant of it */
gboolean
_cdk_display_end_device_grab (CdkDisplay *display,
                              CdkDevice  *device,
                              gulong      serial,
                              CdkWindow  *if_child,
                              gboolean    implicit)
{
  CdkDeviceGrabInfo *grab;
  GList *l;

  l = find_device_grab (display, device, serial);

  if (l == NULL)
    return FALSE;

  grab = l->data;
  if (grab &&
      (if_child == NULL ||
       _cdk_window_event_parent_of (if_child, grab->window)))
    {
      grab->serial_end = serial;
      grab->implicit_ungrab = implicit;
      return l->next == NULL;
    }
  
  return FALSE;
}

/* Returns TRUE if device events are not blocked by any grab */
gboolean
_cdk_display_check_grab_ownership (CdkDisplay *display,
                                   CdkDevice  *device,
                                   gulong      serial)
{
  GHashTableIter iter;
  gpointer key, value;
  CdkGrabOwnership higher_ownership, device_ownership;
  gboolean device_is_keyboard;

  g_return_val_if_fail (display, TRUE);

  if (!display->device_grabs)
    return TRUE; /* No hash table, no grabs. */

  g_hash_table_iter_init (&iter, display->device_grabs);
  higher_ownership = device_ownership = CDK_OWNERSHIP_NONE;
  device_is_keyboard = (cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD);

  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      CdkDeviceGrabInfo *grab;
      CdkDevice *dev;
      GList *grabs;

      dev = key;
      grabs = value;
      grabs = grab_list_find (grabs, serial);

      if (!grabs)
        continue;

      /* Discard device if it's not of the same type */
      if ((device_is_keyboard && cdk_device_get_source (dev) != CDK_SOURCE_KEYBOARD) ||
          (!device_is_keyboard && cdk_device_get_source (dev) == CDK_SOURCE_KEYBOARD))
        continue;

      grab = grabs->data;

      if (dev == device)
        device_ownership = grab->ownership;
      else
        {
          if (grab->ownership > higher_ownership)
            higher_ownership = grab->ownership;
        }
    }

  if (higher_ownership > device_ownership)
    {
      /* There's a higher priority ownership
       * going on for other device(s)
       */
      return FALSE;
    }

  return TRUE;
}

CdkPointerWindowInfo *
_cdk_display_get_pointer_info (CdkDisplay *display,
                               CdkDevice  *device)
{
  CdkPointerWindowInfo *info;

  if (device && cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD)
    device = cdk_device_get_associated_device (device);

  if (G_UNLIKELY (!device))
    return NULL;

  info = g_hash_table_lookup (display->pointers_info, device);

  if (G_UNLIKELY (!info))
    {
      info = g_slice_new0 (CdkPointerWindowInfo);
      g_hash_table_insert (display->pointers_info, device, info);
    }

  return info;
}

void
_cdk_display_pointer_info_foreach (CdkDisplay                   *display,
                                   CdkDisplayPointerInfoForeach  func,
                                   gpointer                      user_data)
{
  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, display->pointers_info);

  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      CdkPointerWindowInfo *info = value;
      CdkDevice *device = key;

      (func) (display, device, info, user_data);
    }
}

/*< private >
 * cdk_device_grab_info:
 * @display: the display for which to get the grab information
 * @device: device to get the grab information from
 * @grab_window: (out) (transfer none): location to store current grab window
 * @owner_events: (out): location to store boolean indicating whether
 *   the @owner_events flag to cdk_keyboard_grab() or
 *   cdk_pointer_grab() was %TRUE.
 *
 * Determines information about the current keyboard grab.
 * This is not public API and must not be used by applications.
 *
 * Returns: %TRUE if this application currently has the
 *  keyboard grabbed.
 */
gboolean
cdk_device_grab_info (CdkDisplay  *display,
                      CdkDevice   *device,
                      CdkWindow  **grab_window,
                      gboolean    *owner_events)
{
  CdkDeviceGrabInfo *info;

  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (CDK_IS_DEVICE (device), FALSE);

  info = _cdk_display_get_last_device_grab (display, device);

  if (info)
    {
      if (grab_window)
        *grab_window = info->window;
      if (owner_events)
        *owner_events = info->owner_events;

      return TRUE;
    }
  else
    return FALSE;
}

/**
 * cdk_device_grab_info_libctk_only:
 * @display: the display for which to get the grab information
 * @device: device to get the grab information from
 * @grab_window: (out) (transfer none): location to store current grab window
 * @owner_events: (out): location to store boolean indicating whether
 *   the @owner_events flag to cdk_keyboard_grab() or
 *   cdk_pointer_grab() was %TRUE.
 *
 * Determines information about the current keyboard grab.
 * This is not public API and must not be used by applications.
 *
 * Returns: %TRUE if this application currently has the
 *  keyboard grabbed.
 *
 * Deprecated: 3.16: The symbol was never meant to be used outside
 *   of CTK+
 */
gboolean
cdk_device_grab_info_libctk_only (CdkDisplay  *display,
                                  CdkDevice   *device,
                                  CdkWindow  **grab_window,
                                  gboolean    *owner_events)
{
  return cdk_device_grab_info (display, device, grab_window, owner_events);
}

/**
 * cdk_display_pointer_is_grabbed:
 * @display: a #CdkDisplay
 *
 * Test if the pointer is grabbed.
 *
 * Returns: %TRUE if an active X pointer grab is in effect
 *
 * Since: 2.2
 */
gboolean
cdk_display_pointer_is_grabbed (CdkDisplay *display)
{
  GList *seats, *s;
  CdkDevice *device;

  g_return_val_if_fail (CDK_IS_DISPLAY (display), TRUE);

  seats = cdk_display_list_seats (display);

  for (s = seats; s; s = s->next)
    {
      device = cdk_seat_get_pointer (s->data);

      if (cdk_display_device_is_grabbed (display, device))
        {
          g_list_free (seats);
          return TRUE;
        }
    }

  g_list_free (seats);

  return FALSE;
}

/**
 * cdk_display_device_is_grabbed:
 * @display: a #CdkDisplay
 * @device: a #CdkDevice
 *
 * Returns %TRUE if there is an ongoing grab on @device for @display.
 *
 * Returns: %TRUE if there is a grab in effect for @device.
 **/
gboolean
cdk_display_device_is_grabbed (CdkDisplay *display,
                               CdkDevice  *device)
{
  CdkDeviceGrabInfo *info;

  g_return_val_if_fail (CDK_IS_DISPLAY (display), TRUE);
  g_return_val_if_fail (CDK_IS_DEVICE (device), TRUE);

  /* What we're interested in is the steady state (ie last grab),
     because we're interested e.g. if we grabbed so that we
     can ungrab, even if our grab is not active just yet. */
  info = _cdk_display_get_last_device_grab (display, device);

  return (info && !info->implicit);
}

/**
 * cdk_display_get_device_manager:
 * @display: a #CdkDisplay.
 *
 * Returns the #CdkDeviceManager associated to @display.
 *
 * Returns: (nullable) (transfer none): A #CdkDeviceManager, or
 *          %NULL. This memory is owned by CDK and must not be freed
 *          or unreferenced.
 *
 * Since: 3.0
 *
 * Deprecated: 3.20. Use cdk_display_get_default_seat() and #CdkSeat operations.
 **/
CdkDeviceManager *
cdk_display_get_device_manager (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return display->device_manager;
}

/**
 * cdk_display_get_name:
 * @display: a #CdkDisplay
 *
 * Gets the name of the display.
 *
 * Returns: a string representing the display name. This string is owned
 * by CDK and should not be modified or freed.
 *
 * Since: 2.2
 */
const gchar *
cdk_display_get_name (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return CDK_DISPLAY_GET_CLASS (display)->get_name (display);
}

/**
 * cdk_display_get_n_screens:
 * @display: a #CdkDisplay
 *
 * Gets the number of screen managed by the @display.
 *
 * Returns: number of screens.
 *
 * Since: 2.2
 *
 * Deprecated: 3.10: The number of screens is always 1.
 */
gint
cdk_display_get_n_screens (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), 0);

  return 1;
}

/**
 * cdk_display_get_screen:
 * @display: a #CdkDisplay
 * @screen_num: the screen number
 *
 * Returns a screen object for one of the screens of the display.
 *
 * Returns: (transfer none): the #CdkScreen object
 *
 * Since: 2.2
 * Deprecated: 3.20: There is only one screen; use cdk_display_get_default_screen() to get it.
 */
CdkScreen *
cdk_display_get_screen (CdkDisplay *display,
			gint        screen_num)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (screen_num == 0, NULL);

  return cdk_display_get_default_screen (display);
}

/**
 * cdk_display_get_default_screen:
 * @display: a #CdkDisplay
 *
 * Get the default #CdkScreen for @display.
 *
 * Returns: (transfer none): the default #CdkScreen object for @display
 *
 * Since: 2.2
 */
CdkScreen *
cdk_display_get_default_screen (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return CDK_DISPLAY_GET_CLASS (display)->get_default_screen (display);
}

/**
 * cdk_display_beep:
 * @display: a #CdkDisplay
 *
 * Emits a short beep on @display
 *
 * Since: 2.2
 */
void
cdk_display_beep (CdkDisplay *display)
{
  g_return_if_fail (CDK_IS_DISPLAY (display));

  CDK_DISPLAY_GET_CLASS (display)->beep (display);
}

/**
 * cdk_display_sync:
 * @display: a #CdkDisplay
 *
 * Flushes any requests queued for the windowing system and waits until all
 * requests have been handled. This is often used for making sure that the
 * display is synchronized with the current state of the program. Calling
 * cdk_display_sync() before cdk_error_trap_pop() makes sure that any errors
 * generated from earlier requests are handled before the error trap is
 * removed.
 *
 * This is most useful for X11. On windowing systems where requests are
 * handled synchronously, this function will do nothing.
 *
 * Since: 2.2
 */
void
cdk_display_sync (CdkDisplay *display)
{
  g_return_if_fail (CDK_IS_DISPLAY (display));

  CDK_DISPLAY_GET_CLASS (display)->sync (display);
}

/**
 * cdk_display_flush:
 * @display: a #CdkDisplay
 *
 * Flushes any requests queued for the windowing system; this happens automatically
 * when the main loop blocks waiting for new events, but if your application
 * is drawing without returning control to the main loop, you may need
 * to call this function explicitly. A common case where this function
 * needs to be called is when an application is executing drawing commands
 * from a thread other than the thread where the main loop is running.
 *
 * This is most useful for X11. On windowing systems where requests are
 * handled synchronously, this function will do nothing.
 *
 * Since: 2.4
 */
void
cdk_display_flush (CdkDisplay *display)
{
  g_return_if_fail (CDK_IS_DISPLAY (display));

  CDK_DISPLAY_GET_CLASS (display)->flush (display);
}

/**
 * cdk_display_get_default_group:
 * @display: a #CdkDisplay
 *
 * Returns the default group leader window for all toplevel windows
 * on @display. This window is implicitly created by CDK.
 * See cdk_window_set_group().
 *
 * Returns: (transfer none): The default group leader window
 * for @display
 *
 * Since: 2.4
 **/
CdkWindow *
cdk_display_get_default_group (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return CDK_DISPLAY_GET_CLASS (display)->get_default_group (display);
}

/**
 * cdk_display_supports_selection_notification:
 * @display: a #CdkDisplay
 *
 * Returns whether #CdkEventOwnerChange events will be
 * sent when the owner of a selection changes.
 *
 * Returns: whether #CdkEventOwnerChange events will
 *               be sent.
 *
 * Since: 2.6
 **/
gboolean
cdk_display_supports_selection_notification (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  return CDK_DISPLAY_GET_CLASS (display)->supports_selection_notification (display);
}

/**
 * cdk_display_request_selection_notification:
 * @display: a #CdkDisplay
 * @selection: the #CdkAtom naming the selection for which
 *             ownership change notification is requested
 *
 * Request #CdkEventOwnerChange events for ownership changes
 * of the selection named by the given atom.
 *
 * Returns: whether #CdkEventOwnerChange events will
 *               be sent.
 *
 * Since: 2.6
 **/
gboolean
cdk_display_request_selection_notification (CdkDisplay *display,
					    CdkAtom     selection)

{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  return CDK_DISPLAY_GET_CLASS (display)->request_selection_notification (display, selection);
}

/**
 * cdk_display_supports_clipboard_persistence:
 * @display: a #CdkDisplay
 *
 * Returns whether the speicifed display supports clipboard
 * persistance; i.e. if it’s possible to store the clipboard data after an
 * application has quit. On X11 this checks if a clipboard daemon is
 * running.
 *
 * Returns: %TRUE if the display supports clipboard persistance.
 *
 * Since: 2.6
 */
gboolean
cdk_display_supports_clipboard_persistence (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  return CDK_DISPLAY_GET_CLASS (display)->supports_clipboard_persistence (display);
}

/**
 * cdk_display_store_clipboard:
 * @display:          a #CdkDisplay
 * @clipboard_window: a #CdkWindow belonging to the clipboard owner
 * @time_:            a timestamp
 * @targets:          (array length=n_targets) (nullable): an array of targets
 *                    that should be saved, or %NULL
 *                    if all available targets should be saved.
 * @n_targets:        length of the @targets array
 *
 * Issues a request to the clipboard manager to store the
 * clipboard data. On X11, this is a special program that works
 * according to the
 * [FreeDesktop Clipboard Specification](http://www.freedesktop.org/Standards/clipboard-manager-spec).
 *
 * Since: 2.6
 */
void
cdk_display_store_clipboard (CdkDisplay    *display,
			     CdkWindow     *clipboard_window,
			     guint32        time_,
			     const CdkAtom *targets,
			     gint           n_targets)
{
  g_return_if_fail (CDK_IS_DISPLAY (display));

  CDK_DISPLAY_GET_CLASS (display)->store_clipboard (display, clipboard_window, time_, targets, n_targets);
}

/**
 * cdk_display_supports_shapes:
 * @display: a #CdkDisplay
 *
 * Returns %TRUE if cdk_window_shape_combine_mask() can
 * be used to create shaped windows on @display.
 *
 * Returns: %TRUE if shaped windows are supported
 *
 * Since: 2.10
 */
gboolean
cdk_display_supports_shapes (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  return CDK_DISPLAY_GET_CLASS (display)->supports_shapes (display);
}

/**
 * cdk_display_supports_input_shapes:
 * @display: a #CdkDisplay
 *
 * Returns %TRUE if cdk_window_input_shape_combine_mask() can
 * be used to modify the input shape of windows on @display.
 *
 * Returns: %TRUE if windows with modified input shape are supported
 *
 * Since: 2.10
 */
gboolean
cdk_display_supports_input_shapes (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  return CDK_DISPLAY_GET_CLASS (display)->supports_input_shapes (display);
}

/**
 * cdk_display_supports_composite:
 * @display: a #CdkDisplay
 *
 * Returns %TRUE if cdk_window_set_composited() can be used
 * to redirect drawing on the window using compositing.
 *
 * Currently this only works on X11 with XComposite and
 * XDamage extensions available.
 *
 * Returns: %TRUE if windows may be composited.
 *
 * Since: 2.12
 *
 * Deprecated: 3.16: Compositing is an outdated technology that
 *   only ever worked on X11.
 */
gboolean
cdk_display_supports_composite (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  return CDK_DISPLAY_GET_CLASS (display)->supports_composite (display);
}

/**
 * cdk_display_list_devices:
 * @display: a #CdkDisplay
 *
 * Returns the list of available input devices attached to @display.
 * The list is statically allocated and should not be freed.
 *
 * Returns: (transfer none) (element-type CdkDevice):
 *     a list of #CdkDevice
 *
 * Since: 2.2
 *
 * Deprecated: 3.0: Use cdk_device_manager_list_devices() instead.
 **/
GList *
cdk_display_list_devices (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  if (!display->input_devices)
    {
      CdkSeat *seat;

      seat = cdk_display_get_default_seat (display);

      /* For backwards compatibility we only include pointing
       * devices (the core pointer and the slaves).
       * We store the list since this deprecated function does
       * not transfer the list ownership.
       */
      display->input_devices = cdk_seat_get_slaves (seat, CDK_SEAT_CAPABILITY_ALL_POINTING);
      display->input_devices = g_list_prepend (display->input_devices, cdk_seat_get_pointer (seat));
      g_list_foreach (display->input_devices, (GFunc) g_object_ref, NULL);
    }

  return display->input_devices;
}

static CdkAppLaunchContext *
cdk_display_real_get_app_launch_context (CdkDisplay *display)
{
  CdkAppLaunchContext *ctx;

  ctx = g_object_new (CDK_TYPE_APP_LAUNCH_CONTEXT,
                      "display", display,
                      NULL);

  return ctx;
}

/**
 * cdk_display_get_app_launch_context:
 * @display: a #CdkDisplay
 *
 * Returns a #CdkAppLaunchContext suitable for launching
 * applications on the given display.
 *
 * Returns: (transfer full): a new #CdkAppLaunchContext for @display.
 *     Free with g_object_unref() when done
 *
 * Since: 3.0
 */
CdkAppLaunchContext *
cdk_display_get_app_launch_context (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return CDK_DISPLAY_GET_CLASS (display)->get_app_launch_context (display);
}

/**
 * cdk_display_open:
 * @display_name: the name of the display to open
 *
 * Opens a display.
 *
 * Returns: (nullable) (transfer none): a #CdkDisplay, or %NULL if the
 *     display could not be opened
 *
 * Since: 2.2
 */
CdkDisplay *
cdk_display_open (const gchar *display_name)
{
  return cdk_display_manager_open_display (cdk_display_manager_get (),
                                           display_name);
}

/**
 * cdk_display_has_pending:
 * @display: a #CdkDisplay
 *
 * Returns whether the display has events that are waiting
 * to be processed.
 *
 * Returns: %TRUE if there are events ready to be processed.
 *
 * Since: 3.0
 */
gboolean
cdk_display_has_pending (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  return CDK_DISPLAY_GET_CLASS (display)->has_pending (display);
}

/**
 * cdk_display_supports_cursor_alpha:
 * @display: a #CdkDisplay
 *
 * Returns %TRUE if cursors can use an 8bit alpha channel
 * on @display. Otherwise, cursors are restricted to bilevel
 * alpha (i.e. a mask).
 *
 * Returns: whether cursors can have alpha channels.
 *
 * Since: 2.4
 */
gboolean
cdk_display_supports_cursor_alpha (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  return CDK_DISPLAY_GET_CLASS (display)->supports_cursor_alpha (display);
}

/**
 * cdk_display_supports_cursor_color:
 * @display: a #CdkDisplay
 *
 * Returns %TRUE if multicolored cursors are supported
 * on @display. Otherwise, cursors have only a forground
 * and a background color.
 *
 * Returns: whether cursors can have multiple colors.
 *
 * Since: 2.4
 */
gboolean
cdk_display_supports_cursor_color (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  return CDK_DISPLAY_GET_CLASS (display)->supports_cursor_color (display);
}

/**
 * cdk_display_get_default_cursor_size:
 * @display: a #CdkDisplay
 *
 * Returns the default size to use for cursors on @display.
 *
 * Returns: the default cursor size.
 *
 * Since: 2.4
 */
guint
cdk_display_get_default_cursor_size (CdkDisplay *display)
{
  guint width, height;

  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  CDK_DISPLAY_GET_CLASS (display)->get_default_cursor_size (display,
                                                            &width,
                                                            &height);

  return MIN (width, height);
}

/**
 * cdk_display_get_maximal_cursor_size:
 * @display: a #CdkDisplay
 * @width: (out): the return location for the maximal cursor width
 * @height: (out): the return location for the maximal cursor height
 *
 * Gets the maximal size to use for cursors on @display.
 *
 * Since: 2.4
 */
void
cdk_display_get_maximal_cursor_size (CdkDisplay *display,
                                     guint       *width,
                                     guint       *height)
{
  g_return_if_fail (CDK_IS_DISPLAY (display));

  CDK_DISPLAY_GET_CLASS (display)->get_maximal_cursor_size (display,
                                                            width,
                                                            height);
}

/**
 * cdk_display_warp_pointer:
 * @display: a #CdkDisplay
 * @screen: the screen of @display to warp the pointer to
 * @x: the x coordinate of the destination
 * @y: the y coordinate of the destination
 *
 * Warps the pointer of @display to the point @x,@y on
 * the screen @screen, unless the pointer is confined
 * to a window by a grab, in which case it will be moved
 * as far as allowed by the grab. Warping the pointer
 * creates events as if the user had moved the mouse
 * instantaneously to the destination.
 *
 * Note that the pointer should normally be under the
 * control of the user. This function was added to cover
 * some rare use cases like keyboard navigation support
 * for the color picker in the #CtkColorSelectionDialog.
 *
 * Since: 2.8
 */
void
cdk_display_warp_pointer (CdkDisplay *display,
                          CdkScreen  *screen,
                          gint        x,
                          gint        y)
{
  CdkDevice *pointer;

  g_return_if_fail (CDK_IS_DISPLAY (display));

  pointer = cdk_seat_get_pointer (cdk_display_get_default_seat (display));
  cdk_device_warp (pointer, screen, x, y);
}

gulong
_cdk_display_get_next_serial (CdkDisplay *display)
{
  return CDK_DISPLAY_GET_CLASS (display)->get_next_serial (display);
}


/**
 * cdk_notify_startup_complete:
 *
 * Indicates to the GUI environment that the application has finished
 * loading. If the applications opens windows, this function is
 * normally called after opening the application’s initial set of
 * windows.
 *
 * CTK+ will call this function automatically after opening the first
 * #CtkWindow unless ctk_window_set_auto_startup_notification() is called
 * to disable that feature.
 *
 * Since: 2.2
 **/
void
cdk_notify_startup_complete (void)
{
  cdk_notify_startup_complete_with_id (NULL);
}

/**
 * cdk_notify_startup_complete_with_id:
 * @startup_id: a startup-notification identifier, for which
 *     notification process should be completed
 *
 * Indicates to the GUI environment that the application has
 * finished loading, using a given identifier.
 *
 * CTK+ will call this function automatically for #CtkWindow
 * with custom startup-notification identifier unless
 * ctk_window_set_auto_startup_notification() is called to
 * disable that feature.
 *
 * Since: 2.12
 */
void
cdk_notify_startup_complete_with_id (const gchar* startup_id)
{
  CdkDisplay *display;

  display = cdk_display_get_default ();
  if (display)
    cdk_display_notify_startup_complete (display, startup_id);
}

/**
 * cdk_display_notify_startup_complete:
 * @display: a #CdkDisplay
 * @startup_id: a startup-notification identifier, for which
 *     notification process should be completed
 *
 * Indicates to the GUI environment that the application has
 * finished loading, using a given identifier.
 *
 * CTK+ will call this function automatically for #CtkWindow
 * with custom startup-notification identifier unless
 * ctk_window_set_auto_startup_notification() is called to
 * disable that feature.
 *
 * Since: 3.0
 */
void
cdk_display_notify_startup_complete (CdkDisplay  *display,
                                     const gchar *startup_id)
{
  g_return_if_fail (CDK_IS_DISPLAY (display));

  CDK_DISPLAY_GET_CLASS (display)->notify_startup_complete (display, startup_id);
}

void
_cdk_display_pause_events (CdkDisplay *display)
{
  display->event_pause_count++;
}

void
_cdk_display_unpause_events (CdkDisplay *display)
{
  g_return_if_fail (display->event_pause_count > 0);

  display->event_pause_count--;
}

void
_cdk_display_event_data_copy (CdkDisplay     *display,
                              const CdkEvent *event,
                              CdkEvent       *new_event)
{
  CDK_DISPLAY_GET_CLASS (display)->event_data_copy (display, event, new_event);
}

void
_cdk_display_event_data_free (CdkDisplay *display,
                              CdkEvent   *event)
{
  CDK_DISPLAY_GET_CLASS (display)->event_data_free (display, event);
}

void
_cdk_display_create_window_impl (CdkDisplay       *display,
                                 CdkWindow        *window,
                                 CdkWindow        *real_parent,
                                 CdkScreen        *screen,
                                 CdkEventMask      event_mask,
                                 CdkWindowAttr    *attributes,
                                 gint              attributes_mask)
{
  CDK_DISPLAY_GET_CLASS (display)->create_window_impl (display,
                                                       window,
                                                       real_parent,
                                                       screen,
                                                       event_mask,
                                                       attributes,
                                                       attributes_mask);
}

CdkWindow *
_cdk_display_create_window (CdkDisplay *display)
{
  return g_object_new (CDK_DISPLAY_GET_CLASS (display)->window_type, NULL);
}

/**
 * cdk_keymap_get_for_display:
 * @display: the #CdkDisplay.
 *
 * Returns the #CdkKeymap attached to @display.
 *
 * Returns: (transfer none): the #CdkKeymap attached to @display.
 *
 * Since: 2.2
 */
CdkKeymap*
cdk_keymap_get_for_display (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return CDK_DISPLAY_GET_CLASS (display)->get_keymap (display);
}

typedef struct _CdkGlobalErrorTrap  CdkGlobalErrorTrap;

struct _CdkGlobalErrorTrap
{
  GSList *displays;
};

static GQueue cdk_error_traps = G_QUEUE_INIT;

/**
 * cdk_error_trap_push:
 *
 * This function allows X errors to be trapped instead of the normal
 * behavior of exiting the application. It should only be used if it
 * is not possible to avoid the X error in any other way. Errors are
 * ignored on all #CdkDisplay currently known to the
 * #CdkDisplayManager. If you don’t care which error happens and just
 * want to ignore everything, pop with cdk_error_trap_pop_ignored().
 * If you need the error code, use cdk_error_trap_pop() which may have
 * to block and wait for the error to arrive from the X server.
 *
 * This API exists on all platforms but only does anything on X.
 *
 * You can use cdk_x11_display_error_trap_push() to ignore errors
 * on only a single display.
 *
 * ## Trapping an X error
 *
 * |[<!-- language="C" -->
 * cdk_error_trap_push ();
 *
 *  // ... Call the X function which may cause an error here ...
 *
 *
 * if (cdk_error_trap_pop ())
 *  {
 *    // ... Handle the error here ...
 *  }
 * ]|
 */
void
cdk_error_trap_push (void)
{
  CdkDisplayManager *manager;
  CdkGlobalErrorTrap *trap;
  GSList *displays;
  GSList *l;

  manager = cdk_display_manager_get ();
  displays = cdk_display_manager_list_displays (manager);

  trap = g_slice_new0 (CdkGlobalErrorTrap);
  for (l = displays; l != NULL; l = l->next)
    {
      CdkDisplay *display = l->data;
      CdkDisplayClass *class = CDK_DISPLAY_GET_CLASS (display);

      if (class->push_error_trap != NULL)
        {
          class->push_error_trap (display);
          trap->displays = g_slist_prepend (trap->displays, g_object_ref (display));
        }
    }

  g_queue_push_head (&cdk_error_traps, trap);

  g_slist_free (displays);
}

static gint
cdk_error_trap_pop_internal (gboolean need_code)
{
  CdkGlobalErrorTrap *trap;
  gint result;
  GSList *l;

  trap = g_queue_pop_head (&cdk_error_traps);

  g_return_val_if_fail (trap != NULL, 0);

  result = 0;
  for (l = trap->displays; l != NULL; l = l->next)
    {
      CdkDisplay *display = l->data;
      CdkDisplayClass *class = CDK_DISPLAY_GET_CLASS (display);
      gint code = class->pop_error_trap (display, !need_code);

      /* we use the error on the last display listed, why not. */
      if (code != 0)
        result = code;
    }

  g_slist_free_full (trap->displays, g_object_unref);
  g_slice_free (CdkGlobalErrorTrap, trap);

  return result;
}

/**
 * cdk_error_trap_pop_ignored:
 *
 * Removes an error trap pushed with cdk_error_trap_push(), but
 * without bothering to wait and see whether an error occurred.  If an
 * error arrives later asynchronously that was triggered while the
 * trap was pushed, that error will be ignored.
 *
 * Since: 3.0
 */
void
cdk_error_trap_pop_ignored (void)
{
  cdk_error_trap_pop_internal (FALSE);
}

/**
 * cdk_error_trap_pop:
 *
 * Removes an error trap pushed with cdk_error_trap_push().
 * May block until an error has been definitively received
 * or not received from the X server. cdk_error_trap_pop_ignored()
 * is preferred if you don’t need to know whether an error
 * occurred, because it never has to block. If you don't
 * need the return value of cdk_error_trap_pop(), use
 * cdk_error_trap_pop_ignored().
 *
 * Prior to CDK 3.0, this function would not automatically
 * sync for you, so you had to cdk_flush() if your last
 * call to Xlib was not a blocking round trip.
 *
 * Returns: X error code or 0 on success
 */
gint
cdk_error_trap_pop (void)
{
  return cdk_error_trap_pop_internal (TRUE);
}

/*< private >
 * cdk_display_make_gl_context_current:
 * @display: a #CdkDisplay
 * @context: (optional): a #CdkGLContext, or %NULL
 *
 * Makes the given @context the current GL context, or unsets
 * the current GL context if @context is %NULL.
 */
gboolean
cdk_display_make_gl_context_current (CdkDisplay   *display,
                                     CdkGLContext *context)
{
  return CDK_DISPLAY_GET_CLASS (display)->make_gl_context_current (display, context);
}

CdkRenderingMode
cdk_display_get_rendering_mode (CdkDisplay *display)
{
  return display->rendering_mode;
}

void
cdk_display_set_rendering_mode (CdkDisplay       *display,
                                CdkRenderingMode  mode)
{
  display->rendering_mode = mode;
}

void
cdk_display_set_debug_updates (CdkDisplay *display,
                               gboolean    debug_updates)
{
  display->debug_updates = debug_updates;
  display->debug_updates_set = TRUE;
}

gboolean
cdk_display_get_debug_updates (CdkDisplay *display)
{
  if (display->debug_updates_set)
    return display->debug_updates;
  else
    return _cdk_debug_updates;
}

void
cdk_display_add_seat (CdkDisplay *display,
                      CdkSeat    *seat)
{
  g_return_if_fail (CDK_IS_DISPLAY (display));
  g_return_if_fail (CDK_IS_SEAT (seat));

  display->seats = g_list_append (display->seats, g_object_ref (seat));
  g_signal_emit (display, signals[SEAT_ADDED], 0, seat);
}

void
cdk_display_remove_seat (CdkDisplay *display,
                         CdkSeat    *seat)
{
  GList *link;

  g_return_if_fail (CDK_IS_DISPLAY (display));
  g_return_if_fail (CDK_IS_SEAT (seat));

  link = g_list_find (display->seats, seat);

  if (link)
    {
      display->seats = g_list_remove_link (display->seats, link);
      g_signal_emit (display, signals[SEAT_REMOVED], 0, seat);
      g_object_unref (link->data);
      g_list_free (link);
    }
}

/**
 * cdk_display_get_default_seat:
 * @display: a #CdkDisplay
 *
 * Returns the default #CdkSeat for this display.
 *
 * Returns: (transfer none): the default seat.
 *
 * Since: 3.20
 **/
CdkSeat *
cdk_display_get_default_seat (CdkDisplay *display)
{
  CdkDisplayClass *display_class;

  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  display_class = CDK_DISPLAY_GET_CLASS (display);

  return display_class->get_default_seat (display);
}

/**
 * cdk_display_list_seats:
 * @display: a #CdkDisplay
 *
 * Returns the list of seats known to @display.
 *
 * Returns: (transfer container) (element-type CdkSeat): the
 *          list of seats known to the #CdkDisplay
 *
 * Since: 3.20
 **/
GList *
cdk_display_list_seats (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return g_list_copy (display->seats);
}

/**
 * cdk_display_get_n_monitors:
 * @display: a #CdkDisplay
 *
 * Gets the number of monitors that belong to @display.
 *
 * The returned number is valid until the next emission of the
 * #CdkDisplay::monitor-added or #CdkDisplay::monitor-removed signal.
 *
 * Returns: the number of monitors
 * Since: 3.22
 */
int
cdk_display_get_n_monitors (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), 0);

  if (CDK_DISPLAY_GET_CLASS (display)->get_n_monitors == NULL)
    return 1;

  return CDK_DISPLAY_GET_CLASS (display)->get_n_monitors (display);
}

static CdkMonitor *
get_fallback_monitor (CdkDisplay *display)
{
  static CdkMonitor *monitor = NULL;
  CdkScreen *screen;

  if (monitor == NULL)
    {
      g_warning ("%s does not implement the monitor vfuncs", G_OBJECT_TYPE_NAME (display));
      monitor = cdk_monitor_new (display);
      cdk_monitor_set_manufacturer (monitor, "fallback");
      cdk_monitor_set_position (monitor, 0, 0);
      cdk_monitor_set_scale_factor (monitor, 1);
    }

  screen = cdk_display_get_default_screen (display);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  cdk_monitor_set_size (monitor,
                        cdk_screen_get_width (screen),
                        cdk_screen_get_height (screen));
  cdk_monitor_set_physical_size (monitor,
                                 cdk_screen_get_width_mm (screen),
                                 cdk_screen_get_height_mm (screen));
G_GNUC_END_IGNORE_DEPRECATIONS

  return monitor;
}

/**
 * cdk_display_get_monitor:
 * @display: a #CdkDisplay
 * @monitor_num: number of the monitor
 *
 * Gets a monitor associated with this display.
 *
 * Returns: (nullable) (transfer none): the #CdkMonitor, or %NULL if
 *    @monitor_num is not a valid monitor number
 * Since: 3.22
 */
CdkMonitor *
cdk_display_get_monitor (CdkDisplay *display,
                         gint        monitor_num)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  if (CDK_DISPLAY_GET_CLASS (display)->get_monitor == NULL)
    return get_fallback_monitor (display);

  return CDK_DISPLAY_GET_CLASS (display)->get_monitor (display, monitor_num);
}

/**
 * cdk_display_get_primary_monitor:
 * @display: a #CdkDisplay
 *
 * Gets the primary monitor for the display.
 *
 * The primary monitor is considered the monitor where the “main desktop”
 * lives. While normal application windows typically allow the window
 * manager to place the windows, specialized desktop applications
 * such as panels should place themselves on the primary monitor.
 *
 * Returns: (nullable) (transfer none): the primary monitor, or %NULL if no primary
 *     monitor is configured by the user
 * Since: 3.22
 */
CdkMonitor *
cdk_display_get_primary_monitor (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  if (CDK_DISPLAY_GET_CLASS (display)->get_primary_monitor)
    return CDK_DISPLAY_GET_CLASS (display)->get_primary_monitor (display);

  return NULL;
}

/**
 * cdk_display_get_monitor_at_point:
 * @display: a #CdkDisplay
 * @x: the x coordinate of the point
 * @y: the y coordinate of the point
 *
 * Gets the monitor in which the point (@x, @y) is located,
 * or a nearby monitor if the point is not in any monitor.
 *
 * Returns: (transfer none): the monitor containing the point
 * Since: 3.22
 */
CdkMonitor *
cdk_display_get_monitor_at_point (CdkDisplay *display,
                                  int         x,
                                  int         y)
{
  CdkMonitor *nearest = NULL;
  int nearest_dist = G_MAXINT;
  int n_monitors, i;

  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  n_monitors = cdk_display_get_n_monitors (display);
  for (i = 0; i < n_monitors; i++)
    {
      CdkMonitor *monitor;
      CdkRectangle geometry;
      int dist_x, dist_y, dist;

      monitor = cdk_display_get_monitor (display, i);
      cdk_monitor_get_geometry (monitor, &geometry);

      if (x < geometry.x)
        dist_x = geometry.x - x;
      else if (geometry.x + geometry.width <= x)
        dist_x = x - (geometry.x + geometry.width) + 1;
      else
        dist_x = 0;

      if (y < geometry.y)
        dist_y = geometry.y - y;
      else if (geometry.y + geometry.height <= y)
        dist_y = y - (geometry.y + geometry.height) + 1;
      else
        dist_y = 0;

      dist = dist_x + dist_y;
      if (dist < nearest_dist)
        {
          nearest_dist = dist;
          nearest = monitor;
        }

      if (nearest_dist == 0)
        break;
    }

  return nearest;
}

/**
 * cdk_display_get_monitor_at_window:
 * @display: a #CdkDisplay
 * @window: a #CdkWindow
 *
 * Gets the monitor in which the largest area of @window
 * resides, or a monitor close to @window if it is outside
 * of all monitors.
 *
 * Returns: (transfer none): the monitor with the largest overlap with @window
 * Since: 3.22
 */
CdkMonitor *
cdk_display_get_monitor_at_window (CdkDisplay *display,
                                   CdkWindow  *window)
{
  CdkRectangle win;
  int n_monitors, i;
  int area = 0;
  CdkMonitor *best = NULL;
  CdkDisplayClass *class;

  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  class = CDK_DISPLAY_GET_CLASS (display);
  if (class->get_monitor_at_window)
    {
      best = class->get_monitor_at_window (display, window);

      if (best)
        return best;
    }

  cdk_window_get_geometry (window, &win.x, &win.y, &win.width, &win.height);
  cdk_window_get_origin (window, &win.x, &win.y);

  n_monitors = cdk_display_get_n_monitors (display);
  for (i = 0; i < n_monitors; i++)
    {
      CdkMonitor *monitor;
      CdkRectangle mon, intersect;
      int overlap;

      monitor = cdk_display_get_monitor (display, i);
      cdk_monitor_get_geometry (monitor, &mon);
      cdk_rectangle_intersect (&win, &mon, &intersect);
      overlap = intersect.width *intersect.height;
      if (overlap > area)
        {
          area = overlap;
          best = monitor;
        }
    }

  if (best)
    return best;

  return cdk_display_get_monitor_at_point (display,
                                           win.x + win.width / 2,
                                           win.y + win.height / 2);
}

void
cdk_display_monitor_added (CdkDisplay *display,
                           CdkMonitor *monitor)
{
  g_signal_emit (display, signals[MONITOR_ADDED], 0, monitor);
}

void
cdk_display_monitor_removed (CdkDisplay *display,
                             CdkMonitor *monitor)
{
  g_signal_emit (display, signals[MONITOR_REMOVED], 0, monitor);
  cdk_monitor_invalidate (monitor);
}
