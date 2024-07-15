/* CDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#include "cdkinternals.h"
#include "cdkdisplayprivate.h"
#include "cdkdndprivate.h"

#include <string.h>
#include <math.h>


/**
 * SECTION:events
 * @Short_description: Functions for handling events from the window system
 * @Title: Events
 * @See_also: [Event Structures][cdk3-Event-Structures]
 *
 * This section describes functions dealing with events from the window
 * system.
 *
 * In CTK+ applications the events are handled automatically in
 * ctk_main_do_event() and passed on to the appropriate widgets, so these
 * functions are rarely needed. Though some of the fields in the
 * [Event Structures][cdk3-Event-Structures] are useful.
 */


typedef struct _CdkIOClosure CdkIOClosure;

struct _CdkIOClosure
{
  GDestroyNotify notify;
  gpointer data;
};

/* Private variable declarations
 */

static CdkEventFunc   _cdk_event_func = NULL;    /* Callback for events */
static gpointer       _cdk_event_data = NULL;
static GDestroyNotify _cdk_event_notify = NULL;

void
_cdk_event_emit (CdkEvent *event)
{
  if (cdk_drag_context_handle_source_event (event))
    return;

  if (_cdk_event_func)
    (*_cdk_event_func) (event, _cdk_event_data);

  if (cdk_drag_context_handle_dest_event (event))
    return;
}

/*********************************************
 * Functions for maintaining the event queue *
 *********************************************/

/**
 * _cdk_event_queue_find_first:
 * @display: a #CdkDisplay
 * 
 * Find the first event on the queue that is not still
 * being filled in.
 * 
 * Returns: (nullable): Pointer to the list node for that event, or
 *   %NULL.
 **/
GList*
_cdk_event_queue_find_first (CdkDisplay *display)
{
  GList *tmp_list;
  GList *pending_motion = NULL;

  gboolean paused = display->event_pause_count > 0;

  tmp_list = display->queued_events;
  while (tmp_list)
    {
      CdkEventPrivate *event = tmp_list->data;

      if ((event->flags & CDK_EVENT_PENDING) == 0 &&
	  (!paused || (event->flags & CDK_EVENT_FLUSHED) != 0))
        {
          if (pending_motion)
            return pending_motion;

          if (event->event.type == CDK_MOTION_NOTIFY && (event->flags & CDK_EVENT_FLUSHED) == 0)
            pending_motion = tmp_list;
          else
            return tmp_list;
        }

      tmp_list = tmp_list->next;
    }

  return NULL;
}

/**
 * _cdk_event_queue_append:
 * @display: a #CdkDisplay
 * @event: Event to append.
 * 
 * Appends an event onto the tail of the event queue.
 *
 * Returns: the newly appended list node.
 **/
GList *
_cdk_event_queue_append (CdkDisplay *display,
			 CdkEvent   *event)
{
  display->queued_tail = g_list_append (display->queued_tail, event);
  
  if (!display->queued_events)
    display->queued_events = display->queued_tail;
  else
    display->queued_tail = display->queued_tail->next;

  return display->queued_tail;
}

/**
 * _cdk_event_queue_insert_after:
 * @display: a #CdkDisplay
 * @sibling: Append after this event.
 * @event: Event to append.
 *
 * Appends an event after the specified event, or if it isn’t in
 * the queue, onto the tail of the event queue.
 *
 * Returns: the newly appended list node.
 *
 * Since: 2.16
 */
GList*
_cdk_event_queue_insert_after (CdkDisplay *display,
                               CdkEvent   *sibling,
                               CdkEvent   *event)
{
  GList *prev = g_list_find (display->queued_events, sibling);
  if (prev && prev->next)
    {
      display->queued_events = g_list_insert_before (display->queued_events, prev->next, event);
      return prev->next;
    }
  else
    return _cdk_event_queue_append (display, event);
}

/**
 * _cdk_event_queue_insert_before:
 * @display: a #CdkDisplay
 * @sibling: Append before this event
 * @event: Event to prepend
 *
 * Prepends an event before the specified event, or if it isn’t in
 * the queue, onto the head of the event queue.
 *
 * Returns: the newly prepended list node.
 *
 * Since: 2.16
 */
GList*
_cdk_event_queue_insert_before (CdkDisplay *display,
				CdkEvent   *sibling,
				CdkEvent   *event)
{
  GList *next = g_list_find (display->queued_events, sibling);
  if (next)
    {
      display->queued_events = g_list_insert_before (display->queued_events, next, event);
      return next->prev;
    }
  else
    return _cdk_event_queue_append (display, event);
}


/**
 * _cdk_event_queue_remove_link:
 * @display: a #CdkDisplay
 * @node: node to remove
 * 
 * Removes a specified list node from the event queue.
 **/
void
_cdk_event_queue_remove_link (CdkDisplay *display,
			      GList      *node)
{
  if (node->prev)
    node->prev->next = node->next;
  else
    display->queued_events = node->next;
  
  if (node->next)
    node->next->prev = node->prev;
  else
    display->queued_tail = node->prev;
}

/**
 * _cdk_event_unqueue:
 * @display: a #CdkDisplay
 * 
 * Removes and returns the first event from the event
 * queue that is not still being filled in.
 * 
 * Returns: (nullable): the event, or %NULL. Ownership is transferred
 * to the caller.
 **/
CdkEvent*
_cdk_event_unqueue (CdkDisplay *display)
{
  CdkEvent *event = NULL;
  GList *tmp_list;

  tmp_list = _cdk_event_queue_find_first (display);

  if (tmp_list)
    {
      event = tmp_list->data;
      _cdk_event_queue_remove_link (display, tmp_list);
      g_list_free_1 (tmp_list);
    }

  return event;
}

void
_cdk_event_queue_handle_motion_compression (CdkDisplay *display)
{
  GList *tmp_list;
  GList *pending_motions = NULL;
  CdkWindow *pending_motion_window = NULL;
  CdkDevice *pending_motion_device = NULL;

  /* If the last N events in the event queue are motion notify
   * events for the same window, drop all but the last */

  tmp_list = display->queued_tail;

  while (tmp_list)
    {
      CdkEventPrivate *event = tmp_list->data;

      if (event->flags & CDK_EVENT_PENDING)
        break;

      if (event->event.type != CDK_MOTION_NOTIFY)
        break;

      if (pending_motion_window != NULL &&
          pending_motion_window != event->event.motion.window)
        break;

      if (pending_motion_device != NULL &&
          pending_motion_device != event->event.motion.device)
        break;

      if (!event->event.motion.window->event_compression)
        break;

      pending_motion_window = event->event.motion.window;
      pending_motion_device = event->event.motion.device;
      pending_motions = tmp_list;

      tmp_list = tmp_list->prev;
    }

  while (pending_motions && pending_motions->next != NULL)
    {
      GList *next = pending_motions->next;
      cdk_event_free (pending_motions->data);
      display->queued_events = g_list_delete_link (display->queued_events,
                                                   pending_motions);
      pending_motions = next;
    }

  if (pending_motions &&
      pending_motions == display->queued_events &&
      pending_motions == display->queued_tail)
    {
      CdkFrameClock *clock = cdk_window_get_frame_clock (pending_motion_window);
      if (clock) /* might be NULL if window was destroyed */
	cdk_frame_clock_request_phase (clock, CDK_FRAME_CLOCK_PHASE_FLUSH_EVENTS);
    }
}

void
_cdk_event_queue_flush (CdkDisplay *display)
{
  GList *tmp_list;

  for (tmp_list = display->queued_events; tmp_list; tmp_list = tmp_list->next)
    {
      CdkEventPrivate *event = tmp_list->data;
      event->flags |= CDK_EVENT_FLUSHED;
    }
}

/**
 * cdk_event_handler_set:
 * @func: the function to call to handle events from CDK.
 * @data: user data to pass to the function. 
 * @notify: the function to call when the handler function is removed, i.e. when
 *          cdk_event_handler_set() is called with another event handler.
 * 
 * Sets the function to call to handle all events from CDK.
 *
 * Note that CTK+ uses this to install its own event handler, so it is
 * usually not useful for CTK+ applications. (Although an application
 * can call this function then call ctk_main_do_event() to pass
 * events to CTK+.)
 **/
void 
cdk_event_handler_set (CdkEventFunc   func,
		       gpointer       data,
		       GDestroyNotify notify)
{
  if (_cdk_event_notify)
    (*_cdk_event_notify) (_cdk_event_data);

  _cdk_event_func = func;
  _cdk_event_data = data;
  _cdk_event_notify = notify;
}

/**
 * cdk_events_pending:
 *
 * Checks if any events are ready to be processed for any display.
 *
 * Returns: %TRUE if any events are pending.
 */
gboolean
cdk_events_pending (void)
{
  GSList *list, *l;
  gboolean pending;

  pending = FALSE;
  list = cdk_display_manager_list_displays (cdk_display_manager_get ());
  for (l = list; l; l = l->next)
    {
      if (_cdk_event_queue_find_first (l->data))
        {
          pending = TRUE;
          goto out;
        }
    }

  for (l = list; l; l = l->next)
    {
      if (cdk_display_has_pending (l->data))
        {
          pending = TRUE;
          goto out;
        }
    }

 out:
  g_slist_free (list);

  return pending;
}

/**
 * cdk_event_get:
 * 
 * Checks all open displays for a #CdkEvent to process,to be processed
 * on, fetching events from the windowing system if necessary.
 * See cdk_display_get_event().
 * 
 * Returns: (nullable): the next #CdkEvent to be processed, or %NULL
 * if no events are pending. The returned #CdkEvent should be freed
 * with cdk_event_free().
 **/
CdkEvent*
cdk_event_get (void)
{
  GSList *list, *l;
  CdkEvent *event;

  event = NULL;
  list = cdk_display_manager_list_displays (cdk_display_manager_get ());
  for (l = list; l; l = l->next)
    {
      event = cdk_display_get_event (l->data);
      if (event)
        break;
    }

  g_slist_free (list);

  return event;
}

/**
 * cdk_event_peek:
 *
 * If there is an event waiting in the event queue of some open
 * display, returns a copy of it. See cdk_display_peek_event().
 * 
 * Returns: (nullable): a copy of the first #CdkEvent on some event
 * queue, or %NULL if no events are in any queues. The returned
 * #CdkEvent should be freed with cdk_event_free().
 **/
CdkEvent*
cdk_event_peek (void)
{
  GSList *list, *l;
  CdkEvent *event;

  event = NULL;
  list = cdk_display_manager_list_displays (cdk_display_manager_get ());
  for (l = list; l; l = l->next)
    {
      event = cdk_display_peek_event (l->data);
      if (event)
        break;
    }

  g_slist_free (list);

  return event;
}

static CdkDisplay *
event_get_display (const CdkEvent *event)
{
  if (event->any.window)
    return cdk_window_get_display (event->any.window);
  else
    return cdk_display_get_default ();
}

/**
 * cdk_event_put:
 * @event: a #CdkEvent.
 *
 * Appends a copy of the given event onto the front of the event
 * queue for event->any.window’s display, or the default event
 * queue if event->any.window is %NULL. See cdk_display_put_event().
 **/
void
cdk_event_put (const CdkEvent *event)
{
  CdkDisplay *display;
  
  g_return_if_fail (event != NULL);

  display = event_get_display (event);

  cdk_display_put_event (display, event);
}

static GHashTable *event_hash = NULL;

/**
 * cdk_event_new:
 * @type: a #CdkEventType 
 * 
 * Creates a new event of the given type. All fields are set to 0.
 * 
 * Returns: a newly-allocated #CdkEvent. The returned #CdkEvent 
 * should be freed with cdk_event_free().
 *
 * Since: 2.2
 **/
CdkEvent*
cdk_event_new (CdkEventType type)
{
  CdkEventPrivate *new_private;
  CdkEvent *new_event;
  
  if (!event_hash)
    event_hash = g_hash_table_new (g_direct_hash, NULL);

  new_private = g_slice_new0 (CdkEventPrivate);
  
  new_private->flags = 0;
  new_private->screen = NULL;

  g_hash_table_insert (event_hash, new_private, GUINT_TO_POINTER (1));

  new_event = (CdkEvent *) new_private;

  new_event->any.type = type;

  /*
   * Bytewise 0 initialization is reasonable for most of the 
   * current event types. Explicitely initialize double fields
   * since I trust bytewise 0 == 0. less than for integers
   * or pointers.
   */
  switch (type)
    {
    case CDK_MOTION_NOTIFY:
      new_event->motion.x = 0.;
      new_event->motion.y = 0.;
      new_event->motion.x_root = 0.;
      new_event->motion.y_root = 0.;
      break;
    case CDK_BUTTON_PRESS:
    case CDK_2BUTTON_PRESS:
    case CDK_3BUTTON_PRESS:
    case CDK_BUTTON_RELEASE:
      new_event->button.x = 0.;
      new_event->button.y = 0.;
      new_event->button.x_root = 0.;
      new_event->button.y_root = 0.;
      break;
    case CDK_TOUCH_BEGIN:
    case CDK_TOUCH_UPDATE:
    case CDK_TOUCH_END:
    case CDK_TOUCH_CANCEL:
      new_event->touch.x = 0.;
      new_event->touch.y = 0.;
      new_event->touch.x_root = 0.;
      new_event->touch.y_root = 0.;
      break;
    case CDK_SCROLL:
      new_event->scroll.x = 0.;
      new_event->scroll.y = 0.;
      new_event->scroll.x_root = 0.;
      new_event->scroll.y_root = 0.;
      new_event->scroll.delta_x = 0.;
      new_event->scroll.delta_y = 0.;
      new_event->scroll.is_stop = FALSE;
      break;
    case CDK_ENTER_NOTIFY:
    case CDK_LEAVE_NOTIFY:
      new_event->crossing.x = 0.;
      new_event->crossing.y = 0.;
      new_event->crossing.x_root = 0.;
      new_event->crossing.y_root = 0.;
      break;
    case CDK_TOUCHPAD_SWIPE:
      new_event->touchpad_swipe.x = 0;
      new_event->touchpad_swipe.y = 0;
      new_event->touchpad_swipe.dx = 0;
      new_event->touchpad_swipe.dy = 0;
      new_event->touchpad_swipe.x_root = 0;
      new_event->touchpad_swipe.y_root = 0;
      break;
    case CDK_TOUCHPAD_PINCH:
      new_event->touchpad_pinch.x = 0;
      new_event->touchpad_pinch.y = 0;
      new_event->touchpad_pinch.dx = 0;
      new_event->touchpad_pinch.dy = 0;
      new_event->touchpad_pinch.angle_delta = 0;
      new_event->touchpad_pinch.scale = 0;
      new_event->touchpad_pinch.x_root = 0;
      new_event->touchpad_pinch.y_root = 0;
      break;
    default:
      break;
    }
  
  return new_event;
}

gboolean
cdk_event_is_allocated (const CdkEvent *event)
{
  if (event_hash)
    return g_hash_table_lookup (event_hash, event) != NULL;

  return FALSE;
}

void
cdk_event_set_pointer_emulated (CdkEvent *event,
                                gboolean  emulated)
{
  if (cdk_event_is_allocated (event))
    {
      CdkEventPrivate *private = (CdkEventPrivate *) event;

      if (emulated)
        private->flags |= CDK_EVENT_POINTER_EMULATED;
      else
        private->flags &= ~(CDK_EVENT_POINTER_EMULATED);
    }
}

/**
 * cdk_event_get_pointer_emulated:
 * @event: a #CdkEvent
 *
 * Returns whether this event is an 'emulated' pointer event (typically
 * from a touch event), as opposed to a real one.
 *
 * Returns: %TRUE if this event is emulated
 *
 * Since: 3.22
 */
gboolean
cdk_event_get_pointer_emulated (CdkEvent *event)
{
  if (cdk_event_is_allocated (event))
    return (((CdkEventPrivate *) event)->flags & CDK_EVENT_POINTER_EMULATED) != 0;

  return FALSE;
}

/**
 * cdk_event_copy:
 * @event: a #CdkEvent
 * 
 * Copies a #CdkEvent, copying or incrementing the reference count of the
 * resources associated with it (e.g. #CdkWindow’s and strings).
 * 
 * Returns: a copy of @event. The returned #CdkEvent should be freed with
 * cdk_event_free().
 **/
CdkEvent*
cdk_event_copy (const CdkEvent *event)
{
  CdkEventPrivate *new_private;
  CdkEvent *new_event;

  g_return_val_if_fail (event != NULL, NULL);

  new_event = cdk_event_new (CDK_NOTHING);
  new_private = (CdkEventPrivate *)new_event;

  *new_event = *event;
  if (new_event->any.window)
    g_object_ref (new_event->any.window);

  if (cdk_event_is_allocated (event))
    {
      CdkEventPrivate *private = (CdkEventPrivate *)event;

      new_private->screen = private->screen;
      new_private->device = private->device ? g_object_ref (private->device) : NULL;
      new_private->source_device = private->source_device ? g_object_ref (private->source_device) : NULL;
      new_private->seat = private->seat;
      new_private->tool = private->tool;

#ifdef CDK_WINDOWING_WIN32
      new_private->translation_len = private->translation_len;
      new_private->translation = g_memdup2 (private->translation, private->translation_len * sizeof (private->translation[0]));
#endif
    }

  switch (event->any.type)
    {
    case CDK_KEY_PRESS:
    case CDK_KEY_RELEASE:
      new_event->key.string = g_strdup (event->key.string);
      break;

    case CDK_ENTER_NOTIFY:
    case CDK_LEAVE_NOTIFY:
      if (event->crossing.subwindow != NULL)
        g_object_ref (event->crossing.subwindow);
      break;

    case CDK_DRAG_ENTER:
    case CDK_DRAG_LEAVE:
    case CDK_DRAG_MOTION:
    case CDK_DRAG_STATUS:
    case CDK_DROP_START:
    case CDK_DROP_FINISHED:
      g_object_ref (event->dnd.context);
      break;

    case CDK_EXPOSE:
    case CDK_DAMAGE:
      if (event->expose.region)
        new_event->expose.region = cairo_region_copy (event->expose.region);
      break;

    case CDK_SETTING:
      new_event->setting.name = g_strdup (new_event->setting.name);
      break;

    case CDK_BUTTON_PRESS:
    case CDK_2BUTTON_PRESS:
    case CDK_3BUTTON_PRESS:
    case CDK_BUTTON_RELEASE:
      if (event->button.axes)
        new_event->button.axes = g_memdup2 (event->button.axes,
                                            sizeof (gdouble) * cdk_device_get_n_axes (event->button.device));
      break;

    case CDK_TOUCH_BEGIN:
    case CDK_TOUCH_UPDATE:
    case CDK_TOUCH_END:
    case CDK_TOUCH_CANCEL:
      if (event->touch.axes)
        new_event->touch.axes = g_memdup2 (event->touch.axes,
                                           sizeof (gdouble) * cdk_device_get_n_axes (event->touch.device));
      break;

    case CDK_MOTION_NOTIFY:
      if (event->motion.axes)
        new_event->motion.axes = g_memdup2 (event->motion.axes,
                                            sizeof (gdouble) * cdk_device_get_n_axes (event->motion.device));
      break;

    case CDK_OWNER_CHANGE:
      new_event->owner_change.owner = event->owner_change.owner;
      if (new_event->owner_change.owner)
        g_object_ref (new_event->owner_change.owner);
      break;

    case CDK_SELECTION_CLEAR:
    case CDK_SELECTION_NOTIFY:
    case CDK_SELECTION_REQUEST:
      new_event->selection.requestor = event->selection.requestor;
      if (new_event->selection.requestor)
        g_object_ref (new_event->selection.requestor);
      break;

    default:
      break;
    }

  if (cdk_event_is_allocated (event))
    _cdk_display_event_data_copy (event_get_display (event), event, new_event);

  return new_event;
}

/**
 * cdk_event_free:
 * @event:  a #CdkEvent.
 * 
 * Frees a #CdkEvent, freeing or decrementing any resources associated with it.
 * Note that this function should only be called with events returned from
 * functions such as cdk_event_peek(), cdk_event_get(), cdk_event_copy()
 * and cdk_event_new().
 **/
void
cdk_event_free (CdkEvent *event)
{
  CdkEventPrivate *private;
  CdkDisplay *display;

  g_return_if_fail (event != NULL);

  if (cdk_event_is_allocated (event))
    {
      private = (CdkEventPrivate *) event;
      g_clear_object (&private->device);
      g_clear_object (&private->source_device);
#ifdef CDK_WINDOWING_WIN32
      g_free (private->translation);
#endif
    }

  switch (event->any.type)
    {
    case CDK_KEY_PRESS:
    case CDK_KEY_RELEASE:
      g_free (event->key.string);
      break;
      
    case CDK_ENTER_NOTIFY:
    case CDK_LEAVE_NOTIFY:
      if (event->crossing.subwindow != NULL)
	g_object_unref (event->crossing.subwindow);
      break;
      
    case CDK_DRAG_ENTER:
    case CDK_DRAG_LEAVE:
    case CDK_DRAG_MOTION:
    case CDK_DRAG_STATUS:
    case CDK_DROP_START:
    case CDK_DROP_FINISHED:
      if (event->dnd.context != NULL)
        g_object_unref (event->dnd.context);
      break;

    case CDK_BUTTON_PRESS:
    case CDK_2BUTTON_PRESS:
    case CDK_3BUTTON_PRESS:
    case CDK_BUTTON_RELEASE:
      g_free (event->button.axes);
      break;

    case CDK_TOUCH_BEGIN:
    case CDK_TOUCH_UPDATE:
    case CDK_TOUCH_END:
    case CDK_TOUCH_CANCEL:
      g_free (event->touch.axes);
      break;

    case CDK_EXPOSE:
    case CDK_DAMAGE:
      if (event->expose.region)
	cairo_region_destroy (event->expose.region);
      break;
      
    case CDK_MOTION_NOTIFY:
      g_free (event->motion.axes);
      break;
      
    case CDK_SETTING:
      g_free (event->setting.name);
      break;
      
    case CDK_OWNER_CHANGE:
      if (event->owner_change.owner)
        g_object_unref (event->owner_change.owner);
      break;

    case CDK_SELECTION_CLEAR:
    case CDK_SELECTION_NOTIFY:
    case CDK_SELECTION_REQUEST:
      if (event->selection.requestor)
        g_object_unref (event->selection.requestor);
      break;

    default:
      break;
    }

  display = event_get_display (event);
  if (display)
    _cdk_display_event_data_free (display, event);

  if (event->any.window)
    g_object_unref (event->any.window);

  g_hash_table_remove (event_hash, event);
  g_slice_free (CdkEventPrivate, (CdkEventPrivate*) event);
}

/**
 * cdk_event_get_window:
 * @event: a #CdkEvent
 *
 * Extracts the #CdkWindow associated with an event.
 *
 * Returns: (transfer none): The #CdkWindow associated with the event
 *
 * Since: 3.10
 */
CdkWindow *
cdk_event_get_window (const CdkEvent *event)
{
  g_return_val_if_fail (event != NULL, NULL);

  return event->any.window;
}

/**
 * cdk_event_get_time:
 * @event: a #CdkEvent
 * 
 * Returns the time stamp from @event, if there is one; otherwise
 * returns #CDK_CURRENT_TIME. If @event is %NULL, returns #CDK_CURRENT_TIME.
 * 
 * Returns: time stamp field from @event
 **/
guint32
cdk_event_get_time (const CdkEvent *event)
{
  if (event)
    switch (event->type)
      {
      case CDK_MOTION_NOTIFY:
	return event->motion.time;
      case CDK_BUTTON_PRESS:
      case CDK_2BUTTON_PRESS:
      case CDK_3BUTTON_PRESS:
      case CDK_BUTTON_RELEASE:
	return event->button.time;
      case CDK_TOUCH_BEGIN:
      case CDK_TOUCH_UPDATE:
      case CDK_TOUCH_END:
      case CDK_TOUCH_CANCEL:
        return event->touch.time;
      case CDK_TOUCHPAD_SWIPE:
        return event->touchpad_swipe.time;
      case CDK_TOUCHPAD_PINCH:
        return event->touchpad_pinch.time;
      case CDK_SCROLL:
        return event->scroll.time;
      case CDK_KEY_PRESS:
      case CDK_KEY_RELEASE:
	return event->key.time;
      case CDK_ENTER_NOTIFY:
      case CDK_LEAVE_NOTIFY:
	return event->crossing.time;
      case CDK_PROPERTY_NOTIFY:
	return event->property.time;
      case CDK_SELECTION_CLEAR:
      case CDK_SELECTION_REQUEST:
      case CDK_SELECTION_NOTIFY:
	return event->selection.time;
      case CDK_PROXIMITY_IN:
      case CDK_PROXIMITY_OUT:
	return event->proximity.time;
      case CDK_DRAG_ENTER:
      case CDK_DRAG_LEAVE:
      case CDK_DRAG_MOTION:
      case CDK_DRAG_STATUS:
      case CDK_DROP_START:
      case CDK_DROP_FINISHED:
	return event->dnd.time;
      case CDK_PAD_BUTTON_PRESS:
      case CDK_PAD_BUTTON_RELEASE:
        return event->pad_button.time;
      case CDK_PAD_RING:
      case CDK_PAD_STRIP:
        return event->pad_axis.time;
      case CDK_PAD_GROUP_MODE:
        return event->pad_group_mode.time;
      case CDK_CLIENT_EVENT:
      case CDK_VISIBILITY_NOTIFY:
      case CDK_CONFIGURE:
      case CDK_FOCUS_CHANGE:
      case CDK_NOTHING:
      case CDK_DAMAGE:
      case CDK_DELETE:
      case CDK_DESTROY:
      case CDK_EXPOSE:
      case CDK_MAP:
      case CDK_UNMAP:
      case CDK_WINDOW_STATE:
      case CDK_SETTING:
      case CDK_OWNER_CHANGE:
      case CDK_GRAB_BROKEN:
      case CDK_EVENT_LAST:
        /* return current time */
        break;
      }
  
  return CDK_CURRENT_TIME;
}

/**
 * cdk_event_get_state:
 * @event: (allow-none): a #CdkEvent or %NULL
 * @state: (out): return location for state
 * 
 * If the event contains a “state” field, puts that field in @state. Otherwise
 * stores an empty state (0). Returns %TRUE if there was a state field
 * in the event. @event may be %NULL, in which case it’s treated
 * as if the event had no state field.
 * 
 * Returns: %TRUE if there was a state field in the event 
 **/
gboolean
cdk_event_get_state (const CdkEvent        *event,
                     CdkModifierType       *state)
{
  g_return_val_if_fail (state != NULL, FALSE);
  
  if (event)
    switch (event->type)
      {
      case CDK_MOTION_NOTIFY:
	*state = event->motion.state;
        return TRUE;
      case CDK_BUTTON_PRESS:
      case CDK_2BUTTON_PRESS:
      case CDK_3BUTTON_PRESS:
      case CDK_BUTTON_RELEASE:
        *state = event->button.state;
        return TRUE;
      case CDK_TOUCH_BEGIN:
      case CDK_TOUCH_UPDATE:
      case CDK_TOUCH_END:
      case CDK_TOUCH_CANCEL:
        *state = event->touch.state;
        return TRUE;
      case CDK_TOUCHPAD_SWIPE:
        *state = event->touchpad_swipe.state;
        return TRUE;
      case CDK_TOUCHPAD_PINCH:
        *state = event->touchpad_pinch.state;
        return TRUE;
      case CDK_SCROLL:
	*state =  event->scroll.state;
        return TRUE;
      case CDK_KEY_PRESS:
      case CDK_KEY_RELEASE:
	*state =  event->key.state;
        return TRUE;
      case CDK_ENTER_NOTIFY:
      case CDK_LEAVE_NOTIFY:
	*state =  event->crossing.state;
        return TRUE;
      case CDK_PROPERTY_NOTIFY:
      case CDK_VISIBILITY_NOTIFY:
      case CDK_CLIENT_EVENT:
      case CDK_CONFIGURE:
      case CDK_FOCUS_CHANGE:
      case CDK_SELECTION_CLEAR:
      case CDK_SELECTION_REQUEST:
      case CDK_SELECTION_NOTIFY:
      case CDK_PROXIMITY_IN:
      case CDK_PROXIMITY_OUT:
      case CDK_DAMAGE:
      case CDK_DRAG_ENTER:
      case CDK_DRAG_LEAVE:
      case CDK_DRAG_MOTION:
      case CDK_DRAG_STATUS:
      case CDK_DROP_START:
      case CDK_DROP_FINISHED:
      case CDK_NOTHING:
      case CDK_DELETE:
      case CDK_DESTROY:
      case CDK_EXPOSE:
      case CDK_MAP:
      case CDK_UNMAP:
      case CDK_WINDOW_STATE:
      case CDK_SETTING:
      case CDK_OWNER_CHANGE:
      case CDK_GRAB_BROKEN:
      case CDK_PAD_BUTTON_PRESS:
      case CDK_PAD_BUTTON_RELEASE:
      case CDK_PAD_RING:
      case CDK_PAD_STRIP:
      case CDK_PAD_GROUP_MODE:
      case CDK_EVENT_LAST:
        /* no state field */
        break;
      }

  *state = 0;
  return FALSE;
}

/**
 * cdk_event_get_coords:
 * @event: a #CdkEvent
 * @x_win: (out) (optional): location to put event window x coordinate
 * @y_win: (out) (optional): location to put event window y coordinate
 * 
 * Extract the event window relative x/y coordinates from an event.
 * 
 * Returns: %TRUE if the event delivered event window coordinates
 **/
gboolean
cdk_event_get_coords (const CdkEvent *event,
		      gdouble        *x_win,
		      gdouble        *y_win)
{
  gdouble x = 0, y = 0;
  gboolean fetched = TRUE;
  
  g_return_val_if_fail (event != NULL, FALSE);

  switch (event->type)
    {
    case CDK_CONFIGURE:
      x = event->configure.x;
      y = event->configure.y;
      break;
    case CDK_ENTER_NOTIFY:
    case CDK_LEAVE_NOTIFY:
      x = event->crossing.x;
      y = event->crossing.y;
      break;
    case CDK_SCROLL:
      x = event->scroll.x;
      y = event->scroll.y;
      break;
    case CDK_BUTTON_PRESS:
    case CDK_2BUTTON_PRESS:
    case CDK_3BUTTON_PRESS:
    case CDK_BUTTON_RELEASE:
      x = event->button.x;
      y = event->button.y;
      break;
    case CDK_TOUCH_BEGIN:
    case CDK_TOUCH_UPDATE:
    case CDK_TOUCH_END:
    case CDK_TOUCH_CANCEL:
      x = event->touch.x;
      y = event->touch.y;
      break;
    case CDK_MOTION_NOTIFY:
      x = event->motion.x;
      y = event->motion.y;
      break;
    case CDK_TOUCHPAD_SWIPE:
      x = event->touchpad_swipe.x;
      y = event->touchpad_swipe.y;
      break;
    case CDK_TOUCHPAD_PINCH:
      x = event->touchpad_pinch.x;
      y = event->touchpad_pinch.y;
      break;
    default:
      fetched = FALSE;
      break;
    }

  if (x_win)
    *x_win = x;
  if (y_win)
    *y_win = y;

  return fetched;
}

/**
 * cdk_event_get_root_coords:
 * @event: a #CdkEvent
 * @x_root: (out) (optional): location to put root window x coordinate
 * @y_root: (out) (optional): location to put root window y coordinate
 * 
 * Extract the root window relative x/y coordinates from an event.
 * 
 * Returns: %TRUE if the event delivered root window coordinates
 **/
gboolean
cdk_event_get_root_coords (const CdkEvent *event,
			   gdouble        *x_root,
			   gdouble        *y_root)
{
  gdouble x = 0, y = 0;
  gboolean fetched = TRUE;
  
  g_return_val_if_fail (event != NULL, FALSE);

  switch (event->type)
    {
    case CDK_MOTION_NOTIFY:
      x = event->motion.x_root;
      y = event->motion.y_root;
      break;
    case CDK_SCROLL:
      x = event->scroll.x_root;
      y = event->scroll.y_root;
      break;
    case CDK_BUTTON_PRESS:
    case CDK_2BUTTON_PRESS:
    case CDK_3BUTTON_PRESS:
    case CDK_BUTTON_RELEASE:
      x = event->button.x_root;
      y = event->button.y_root;
      break;
    case CDK_TOUCH_BEGIN:
    case CDK_TOUCH_UPDATE:
    case CDK_TOUCH_END:
    case CDK_TOUCH_CANCEL:
      x = event->touch.x_root;
      y = event->touch.y_root;
      break;
    case CDK_ENTER_NOTIFY:
    case CDK_LEAVE_NOTIFY:
      x = event->crossing.x_root;
      y = event->crossing.y_root;
      break;
    case CDK_DRAG_ENTER:
    case CDK_DRAG_LEAVE:
    case CDK_DRAG_MOTION:
    case CDK_DRAG_STATUS:
    case CDK_DROP_START:
    case CDK_DROP_FINISHED:
      x = event->dnd.x_root;
      y = event->dnd.y_root;
      break;
    case CDK_TOUCHPAD_SWIPE:
      x = event->touchpad_swipe.x_root;
      y = event->touchpad_swipe.y_root;
      break;
    case CDK_TOUCHPAD_PINCH:
      x = event->touchpad_pinch.x_root;
      y = event->touchpad_pinch.y_root;
      break;
    default:
      fetched = FALSE;
      break;
    }

  if (x_root)
    *x_root = x;
  if (y_root)
    *y_root = y;

  return fetched;
}

/**
 * cdk_event_get_button:
 * @event: a #CdkEvent
 * @button: (out): location to store mouse button number
 *
 * Extract the button number from an event.
 *
 * Returns: %TRUE if the event delivered a button number
 *
 * Since: 3.2
 **/
gboolean
cdk_event_get_button (const CdkEvent *event,
                      guint *button)
{
  gboolean fetched = TRUE;
  guint number = 0;

  g_return_val_if_fail (event != NULL, FALSE);
  
  switch (event->type)
    {
    case CDK_BUTTON_PRESS:
    case CDK_2BUTTON_PRESS:
    case CDK_3BUTTON_PRESS:
    case CDK_BUTTON_RELEASE:
      number = event->button.button;
      break;
    case CDK_PAD_BUTTON_PRESS:
    case CDK_PAD_BUTTON_RELEASE:
      number = event->pad_button.button;
      break;
    default:
      fetched = FALSE;
      break;
    }

  if (button)
    *button = number;

  return fetched;
}

/**
 * cdk_event_get_click_count:
 * @event: a #CdkEvent
 * @click_count: (out): location to store click count
 *
 * Extracts the click count from an event.
 *
 * Returns: %TRUE if the event delivered a click count
 *
 * Since: 3.2
 */
gboolean
cdk_event_get_click_count (const CdkEvent *event,
                           guint *click_count)
{
  gboolean fetched = TRUE;
  guint number = 0;

  g_return_val_if_fail (event != NULL, FALSE);

  switch (event->type)
    {
    case CDK_BUTTON_PRESS:
    case CDK_BUTTON_RELEASE:
      number = 1;
      break;
    case CDK_2BUTTON_PRESS:
      number = 2;
      break;
    case CDK_3BUTTON_PRESS:
      number = 3;
      break;
    default:
      fetched = FALSE;
      break;
    }

  if (click_count)
    *click_count = number;

  return fetched;
}

/**
 * cdk_event_get_keyval:
 * @event: a #CdkEvent
 * @keyval: (out): location to store the keyval
 *
 * Extracts the keyval from an event.
 *
 * Returns: %TRUE if the event delivered a key symbol
 *
 * Since: 3.2
 */
gboolean
cdk_event_get_keyval (const CdkEvent *event,
                      guint *keyval)
{
  gboolean fetched = TRUE;
  guint number = 0;

  switch (event->type)
    {
    case CDK_KEY_PRESS:
    case CDK_KEY_RELEASE:
      number = event->key.keyval;
      break;
    default:
      fetched = FALSE;
      break;
    }

  if (keyval)
    *keyval = number;

  return fetched;
}

/**
 * cdk_event_get_keycode:
 * @event: a #CdkEvent
 * @keycode: (out): location to store the keycode
 *
 * Extracts the hardware keycode from an event.
 *
 * Also see cdk_event_get_scancode().
 *
 * Returns: %TRUE if the event delivered a hardware keycode
 *
 * Since: 3.2
 */
gboolean
cdk_event_get_keycode (const CdkEvent *event,
                       guint16 *keycode)
{
  gboolean fetched = TRUE;
  guint16 number = 0;

  switch (event->type)
    {
    case CDK_KEY_PRESS:
    case CDK_KEY_RELEASE:
      number = event->key.hardware_keycode;
      break;
    default:
      fetched = FALSE;
      break;
    }

  if (keycode)
    *keycode = number;

  return fetched;
}

/**
 * cdk_event_get_scroll_direction:
 * @event: a #CdkEvent
 * @direction: (out): location to store the scroll direction
 *
 * Extracts the scroll direction from an event.
 *
 * If @event is not of type %CDK_SCROLL, the contents of @direction
 * are undefined.
 *
 * If you wish to handle both discrete and smooth scrolling, you
 * should check the return value of this function, or of
 * cdk_event_get_scroll_deltas(); for instance:
 *
 * |[<!-- language="C" -->
 *   CdkScrollDirection direction;
 *   double vscroll_factor = 0.0;
 *   double x_scroll, y_scroll;
 *
 *   if (cdk_event_get_scroll_direction (event, &direction))
 *     {
 *       // Handle discrete scrolling with a known constant delta;
 *       const double delta = 12.0;
 *
 *       switch (direction)
 *         {
 *         case CDK_SCROLL_UP:
 *           vscroll_factor = -delta;
 *           break;
 *         case CDK_SCROLL_DOWN:
 *           vscroll_factor = delta;
 *           break;
 *         default:
 *           // no scrolling
 *           break;
 *         }
 *     }
 *   else if (cdk_event_get_scroll_deltas (event, &x_scroll, &y_scroll))
 *     {
 *       // Handle smooth scrolling directly
 *       vscroll_factor = y_scroll;
 *     }
 * ]|
 *
 * Returns: %TRUE if the event delivered a scroll direction
 *   and %FALSE otherwise
 *
 * Since: 3.2
 */
gboolean
cdk_event_get_scroll_direction (const CdkEvent *event,
                                CdkScrollDirection *direction)
{
  gboolean fetched = TRUE;
  CdkScrollDirection dir = 0;

  switch (event->type)
    {
    case CDK_SCROLL:
      if (event->scroll.direction == CDK_SCROLL_SMOOTH)
        fetched = FALSE;
      else
        dir = event->scroll.direction;
      break;
    default:
      fetched = FALSE;
      break;
    }

  if (direction)
    *direction = dir;

  return fetched;
}

/**
 * cdk_event_get_scroll_deltas:
 * @event: a #CdkEvent
 * @delta_x: (out): return location for X delta
 * @delta_y: (out): return location for Y delta
 *
 * Retrieves the scroll deltas from a #CdkEvent
 *
 * See also: cdk_event_get_scroll_direction()
 *
 * Returns: %TRUE if the event contains smooth scroll information
 *   and %FALSE otherwise
 *
 * Since: 3.4
 **/
gboolean
cdk_event_get_scroll_deltas (const CdkEvent *event,
                             gdouble        *delta_x,
                             gdouble        *delta_y)
{
  gboolean fetched = TRUE;
  gdouble dx = 0.0;
  gdouble dy = 0.0;

  switch (event->type)
    {
    case CDK_SCROLL:
      if (event->scroll.direction == CDK_SCROLL_SMOOTH)
        {
          dx = event->scroll.delta_x;
          dy = event->scroll.delta_y;
        }
      else
        fetched = FALSE;
      break;
    default:
      fetched = FALSE;
      break;
    }

  if (delta_x)
    *delta_x = dx;

  if (delta_y)
    *delta_y = dy;

  return fetched;
}

/**
 * cdk_event_is_scroll_stop_event
 * @event: a #CdkEvent
 *
 * Check whether a scroll event is a stop scroll event. Scroll sequences
 * with smooth scroll information may provide a stop scroll event once the
 * interaction with the device finishes, e.g. by lifting a finger. This
 * stop scroll event is the signal that a widget may trigger kinetic
 * scrolling based on the current velocity.
 *
 * Stop scroll events always have a a delta of 0/0.
 *
 * Returns: %TRUE if the event is a scroll stop event
 *
 * Since: 3.20
 */
gboolean
cdk_event_is_scroll_stop_event (const CdkEvent *event)
{
  return event->scroll.is_stop;
}

/**
 * cdk_event_get_axis:
 * @event: a #CdkEvent
 * @axis_use: the axis use to look for
 * @value: (out): location to store the value found
 * 
 * Extract the axis value for a particular axis use from
 * an event structure.
 * 
 * Returns: %TRUE if the specified axis was found, otherwise %FALSE
 **/
gboolean
cdk_event_get_axis (const CdkEvent *event,
		    CdkAxisUse      axis_use,
		    gdouble        *value)
{
  gdouble *axes;
  CdkDevice *device;
  
  g_return_val_if_fail (event != NULL, FALSE);
  
  if (axis_use == CDK_AXIS_X || axis_use == CDK_AXIS_Y)
    {
      gdouble x, y;
      
      switch (event->type)
	{
        case CDK_MOTION_NOTIFY:
	  x = event->motion.x;
	  y = event->motion.y;
	  break;
	case CDK_SCROLL:
	  x = event->scroll.x;
	  y = event->scroll.y;
	  break;
	case CDK_BUTTON_PRESS:
	case CDK_BUTTON_RELEASE:
	  x = event->button.x;
	  y = event->button.y;
	  break;
        case CDK_TOUCH_BEGIN:
        case CDK_TOUCH_UPDATE:
        case CDK_TOUCH_END:
        case CDK_TOUCH_CANCEL:
	  x = event->touch.x;
	  y = event->touch.y;
	  break;
	case CDK_ENTER_NOTIFY:
	case CDK_LEAVE_NOTIFY:
	  x = event->crossing.x;
	  y = event->crossing.y;
	  break;
	  
	default:
	  return FALSE;
	}

      if (axis_use == CDK_AXIS_X && value)
	*value = x;
      if (axis_use == CDK_AXIS_Y && value)
	*value = y;

      return TRUE;
    }
  else if (event->type == CDK_BUTTON_PRESS ||
	   event->type == CDK_BUTTON_RELEASE)
    {
      device = event->button.device;
      axes = event->button.axes;
    }
  else if (event->type == CDK_TOUCH_BEGIN ||
           event->type == CDK_TOUCH_UPDATE ||
           event->type == CDK_TOUCH_END ||
           event->type == CDK_TOUCH_CANCEL)
    {
      device = event->touch.device;
      axes = event->touch.axes;
    }
  else if (event->type == CDK_MOTION_NOTIFY)
    {
      device = event->motion.device;
      axes = event->motion.axes;
    }
  else
    return FALSE;

  return cdk_device_get_axis (device, axes, axis_use, value);
}

/**
 * cdk_event_set_device:
 * @event: a #CdkEvent
 * @device: a #CdkDevice
 *
 * Sets the device for @event to @device. The event must
 * have been allocated by CTK+, for instance, by
 * cdk_event_copy().
 *
 * Since: 3.0
 **/
void
cdk_event_set_device (CdkEvent  *event,
                      CdkDevice *device)
{
  CdkEventPrivate *private;

  g_return_if_fail (cdk_event_is_allocated (event));

  private = (CdkEventPrivate *) event;

  g_set_object (&private->device, device);

  switch (event->type)
    {
    case CDK_MOTION_NOTIFY:
      event->motion.device = device;
      break;
    case CDK_BUTTON_PRESS:
    case CDK_2BUTTON_PRESS:
    case CDK_3BUTTON_PRESS:
    case CDK_BUTTON_RELEASE:
      event->button.device = device;
      break;
    case CDK_TOUCH_BEGIN:
    case CDK_TOUCH_UPDATE:
    case CDK_TOUCH_END:
    case CDK_TOUCH_CANCEL:
      event->touch.device = device;
      break;
    case CDK_SCROLL:
      event->scroll.device = device;
      break;
    case CDK_PROXIMITY_IN:
    case CDK_PROXIMITY_OUT:
      event->proximity.device = device;
      break;
    default:
      break;
    }
}

/**
 * cdk_event_get_device:
 * @event: a #CdkEvent.
 *
 * If the event contains a “device” field, this function will return
 * it, else it will return %NULL.
 *
 * Returns: (nullable) (transfer none): a #CdkDevice, or %NULL.
 *
 * Since: 3.0
 **/
CdkDevice *
cdk_event_get_device (const CdkEvent *event)
{
  g_return_val_if_fail (event != NULL, NULL);

  if (cdk_event_is_allocated (event))
    {
      CdkEventPrivate *private = (CdkEventPrivate *) event;

      if (private->device)
        return private->device;
    }

  switch (event->type)
    {
    case CDK_MOTION_NOTIFY:
      return event->motion.device;
    case CDK_BUTTON_PRESS:
    case CDK_2BUTTON_PRESS:
    case CDK_3BUTTON_PRESS:
    case CDK_BUTTON_RELEASE:
      return event->button.device;
    case CDK_TOUCH_BEGIN:
    case CDK_TOUCH_UPDATE:
    case CDK_TOUCH_END:
    case CDK_TOUCH_CANCEL:
      return event->touch.device;
    case CDK_SCROLL:
      return event->scroll.device;
    case CDK_PROXIMITY_IN:
    case CDK_PROXIMITY_OUT:
      return event->proximity.device;
    default:
      break;
    }

  /* Fallback if event has no device set */
  switch (event->type)
    {
    case CDK_MOTION_NOTIFY:
    case CDK_BUTTON_PRESS:
    case CDK_2BUTTON_PRESS:
    case CDK_3BUTTON_PRESS:
    case CDK_BUTTON_RELEASE:
    case CDK_TOUCH_BEGIN:
    case CDK_TOUCH_UPDATE:
    case CDK_TOUCH_END:
    case CDK_TOUCH_CANCEL:
    case CDK_ENTER_NOTIFY:
    case CDK_LEAVE_NOTIFY:
    case CDK_FOCUS_CHANGE:
    case CDK_PROXIMITY_IN:
    case CDK_PROXIMITY_OUT:
    case CDK_DRAG_ENTER:
    case CDK_DRAG_LEAVE:
    case CDK_DRAG_MOTION:
    case CDK_DRAG_STATUS:
    case CDK_DROP_START:
    case CDK_DROP_FINISHED:
    case CDK_SCROLL:
    case CDK_GRAB_BROKEN:
    case CDK_KEY_PRESS:
    case CDK_KEY_RELEASE:
      {
        CdkDisplay *display;
        CdkSeat *seat;

        g_warning ("Event with type %d not holding a CdkDevice. "
                   "It is most likely synthesized outside Cdk/CTK+",
                   event->type);

        display = cdk_window_get_display (event->any.window);
        seat = cdk_display_get_default_seat (display);

        if (event->type == CDK_KEY_PRESS ||
            event->type == CDK_KEY_RELEASE)
          return cdk_seat_get_keyboard (seat);
        else
          return cdk_seat_get_pointer (seat);
      }
      break;
    default:
      return NULL;
    }
}

/**
 * cdk_event_set_source_device:
 * @event: a #CdkEvent
 * @device: a #CdkDevice
 *
 * Sets the slave device for @event to @device.
 *
 * The event must have been allocated by CTK+,
 * for instance by cdk_event_copy().
 *
 * Since: 3.0
 **/
void
cdk_event_set_source_device (CdkEvent  *event,
                             CdkDevice *device)
{
  CdkEventPrivate *private;

  g_return_if_fail (cdk_event_is_allocated (event));
  g_return_if_fail (CDK_IS_DEVICE (device));

  private = (CdkEventPrivate *) event;

  g_set_object (&private->source_device, device);
}

/**
 * cdk_event_get_source_device:
 * @event: a #CdkEvent
 *
 * This function returns the hardware (slave) #CdkDevice that has
 * triggered the event, falling back to the virtual (master) device
 * (as in cdk_event_get_device()) if the event wasn’t caused by
 * interaction with a hardware device. This may happen for example
 * in synthesized crossing events after a #CdkWindow updates its
 * geometry or a grab is acquired/released.
 *
 * If the event does not contain a device field, this function will
 * return %NULL.
 *
 * Returns: (nullable) (transfer none): a #CdkDevice, or %NULL.
 *
 * Since: 3.0
 **/
CdkDevice *
cdk_event_get_source_device (const CdkEvent *event)
{
  CdkEventPrivate *private;

  g_return_val_if_fail (event != NULL, NULL);

  if (!cdk_event_is_allocated (event))
    return NULL;

  private = (CdkEventPrivate *) event;

  if (private->source_device)
    return private->source_device;

  /* Fallback to event device */
  return cdk_event_get_device (event);
}

/**
 * cdk_event_request_motions:
 * @event: a valid #CdkEvent
 *
 * Request more motion notifies if @event is a motion notify hint event.
 *
 * This function should be used instead of cdk_window_get_pointer() to
 * request further motion notifies, because it also works for extension
 * events where motion notifies are provided for devices other than the
 * core pointer. Coordinate extraction, processing and requesting more
 * motion events from a %CDK_MOTION_NOTIFY event usually works like this:
 *
 * |[<!-- language="C" -->
 * {
 *   // motion_event handler
 *   x = motion_event->x;
 *   y = motion_event->y;
 *   // handle (x,y) motion
 *   cdk_event_request_motions (motion_event); // handles is_hint events
 * }
 * ]|
 *
 * Since: 2.12
 **/
void
cdk_event_request_motions (const CdkEventMotion *event)
{
  CdkDisplay *display;
  
  g_return_if_fail (event != NULL);
  
  if (event->type == CDK_MOTION_NOTIFY && event->is_hint)
    {
      cdk_device_get_state (event->device, event->window, NULL, NULL);
      
      display = cdk_window_get_display (event->window);
      _cdk_display_enable_motion_hints (display, event->device);
    }
}

/**
 * cdk_event_triggers_context_menu:
 * @event: a #CdkEvent, currently only button events are meaningful values
 *
 * This function returns whether a #CdkEventButton should trigger a
 * context menu, according to platform conventions. The right mouse
 * button always triggers context menus. Additionally, if
 * cdk_keymap_get_modifier_mask() returns a non-0 mask for
 * %CDK_MODIFIER_INTENT_CONTEXT_MENU, then the left mouse button will
 * also trigger a context menu if this modifier is pressed.
 *
 * This function should always be used instead of simply checking for
 * event->button == %CDK_BUTTON_SECONDARY.
 *
 * Returns: %TRUE if the event should trigger a context menu.
 *
 * Since: 3.4
 **/
gboolean
cdk_event_triggers_context_menu (const CdkEvent *event)
{
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->type == CDK_BUTTON_PRESS)
    {
      const CdkEventButton *bevent = (const CdkEventButton *) event;
      CdkDisplay *display;
      CdkModifierType modifier;

      g_return_val_if_fail (CDK_IS_WINDOW (bevent->window), FALSE);

      if (bevent->button == CDK_BUTTON_SECONDARY &&
          ! (bevent->state & (CDK_BUTTON1_MASK | CDK_BUTTON2_MASK)))
        return TRUE;

      display = cdk_window_get_display (bevent->window);

      modifier = cdk_keymap_get_modifier_mask (cdk_keymap_get_for_display (display),
                                               CDK_MODIFIER_INTENT_CONTEXT_MENU);

      if (modifier != 0 &&
          bevent->button == CDK_BUTTON_PRIMARY &&
          ! (bevent->state & (CDK_BUTTON2_MASK | CDK_BUTTON3_MASK)) &&
          (bevent->state & modifier))
        return TRUE;
    }

  return FALSE;
}

static gboolean
cdk_events_get_axis_distances (CdkEvent *event1,
                               CdkEvent *event2,
                               gdouble  *x_distance,
                               gdouble  *y_distance,
                               gdouble  *distance)
{
  gdouble x1, x2, y1, y2;
  gdouble xd, yd;

  if (!cdk_event_get_coords (event1, &x1, &y1) ||
      !cdk_event_get_coords (event2, &x2, &y2))
    return FALSE;

  xd = x2 - x1;
  yd = y2 - y1;

  if (x_distance)
    *x_distance = xd;

  if (y_distance)
    *y_distance = yd;

  if (distance)
    *distance = sqrt ((xd * xd) + (yd * yd));

  return TRUE;
}

/**
 * cdk_events_get_distance:
 * @event1: first #CdkEvent
 * @event2: second #CdkEvent
 * @distance: (out): return location for the distance
 *
 * If both events have X/Y information, the distance between both coordinates
 * (as in a straight line going from @event1 to @event2) will be returned.
 *
 * Returns: %TRUE if the distance could be calculated.
 *
 * Since: 3.0
 **/
gboolean
cdk_events_get_distance (CdkEvent *event1,
                         CdkEvent *event2,
                         gdouble  *distance)
{
  return cdk_events_get_axis_distances (event1, event2,
                                        NULL, NULL,
                                        distance);
}

/**
 * cdk_events_get_angle:
 * @event1: first #CdkEvent
 * @event2: second #CdkEvent
 * @angle: (out): return location for the relative angle between both events
 *
 * If both events contain X/Y information, this function will return %TRUE
 * and return in @angle the relative angle from @event1 to @event2. The rotation
 * direction for positive angles is from the positive X axis towards the positive
 * Y axis.
 *
 * Returns: %TRUE if the angle could be calculated.
 *
 * Since: 3.0
 **/
gboolean
cdk_events_get_angle (CdkEvent *event1,
                      CdkEvent *event2,
                      gdouble  *angle)
{
  gdouble x_distance, y_distance, distance;

  if (!cdk_events_get_axis_distances (event1, event2,
                                      &x_distance, &y_distance,
                                      &distance))
    return FALSE;

  if (angle)
    {
      *angle = atan2 (x_distance, y_distance);

      /* Invert angle */
      *angle = (2 * G_PI) - *angle;

      /* Shift it 90° */
      *angle += G_PI / 2;

      /* And constraint it to 0°-360° */
      *angle = fmod (*angle, 2 * G_PI);
    }

  return TRUE;
}

/**
 * cdk_events_get_center:
 * @event1: first #CdkEvent
 * @event2: second #CdkEvent
 * @x: (out): return location for the X coordinate of the center
 * @y: (out): return location for the Y coordinate of the center
 *
 * If both events contain X/Y information, the center of both coordinates
 * will be returned in @x and @y.
 *
 * Returns: %TRUE if the center could be calculated.
 *
 * Since: 3.0
 **/
gboolean
cdk_events_get_center (CdkEvent *event1,
                       CdkEvent *event2,
                       gdouble  *x,
                       gdouble  *y)
{
  gdouble x1, x2, y1, y2;

  if (!cdk_event_get_coords (event1, &x1, &y1) ||
      !cdk_event_get_coords (event2, &x2, &y2))
    return FALSE;

  if (x)
    *x = (x2 + x1) / 2;

  if (y)
    *y = (y2 + y1) / 2;

  return TRUE;
}

/**
 * cdk_event_set_screen:
 * @event: a #CdkEvent
 * @screen: a #CdkScreen
 * 
 * Sets the screen for @event to @screen. The event must
 * have been allocated by CTK+, for instance, by
 * cdk_event_copy().
 *
 * Since: 2.2
 **/
void
cdk_event_set_screen (CdkEvent  *event,
		      CdkScreen *screen)
{
  CdkEventPrivate *private;
  
  g_return_if_fail (cdk_event_is_allocated (event));

  private = (CdkEventPrivate *)event;
  
  private->screen = screen;
}

/**
 * cdk_event_get_screen:
 * @event: a #CdkEvent
 * 
 * Returns the screen for the event. The screen is
 * typically the screen for `event->any.window`, but
 * for events such as mouse events, it is the screen
 * where the pointer was when the event occurs -
 * that is, the screen which has the root window 
 * to which `event->motion.x_root` and
 * `event->motion.y_root` are relative.
 * 
 * Returns: (transfer none): the screen for the event
 *
 * Since: 2.2
 **/
CdkScreen *
cdk_event_get_screen (const CdkEvent *event)
{
  if (cdk_event_is_allocated (event))
    {
      CdkEventPrivate *private = (CdkEventPrivate *)event;

      if (private->screen)
	return private->screen;
    }

  if (event->any.window)
    return cdk_window_get_screen (event->any.window);

  return NULL;
}

/**
 * cdk_event_get_event_sequence:
 * @event: a #CdkEvent
 *
 * If @event if of type %CDK_TOUCH_BEGIN, %CDK_TOUCH_UPDATE,
 * %CDK_TOUCH_END or %CDK_TOUCH_CANCEL, returns the #CdkEventSequence
 * to which the event belongs. Otherwise, return %NULL.
 *
 * Returns: (transfer none): the event sequence that the event belongs to
 *
 * Since: 3.4
 */
CdkEventSequence *
cdk_event_get_event_sequence (const CdkEvent *event)
{
  if (!event)
    return NULL;

  if (event->type == CDK_TOUCH_BEGIN ||
      event->type == CDK_TOUCH_UPDATE ||
      event->type == CDK_TOUCH_END ||
      event->type == CDK_TOUCH_CANCEL)
    return event->touch.sequence;

  return NULL;
}

/**
 * cdk_set_show_events:
 * @show_events:  %TRUE to output event debugging information.
 * 
 * Sets whether a trace of received events is output.
 * Note that CTK+ must be compiled with debugging (that is,
 * configured using the `--enable-debug` option)
 * to use this option.
 **/
void
cdk_set_show_events (gboolean show_events)
{
  if (show_events)
    _cdk_debug_flags |= CDK_DEBUG_EVENTS;
  else
    _cdk_debug_flags &= ~CDK_DEBUG_EVENTS;
}

/**
 * cdk_get_show_events:
 * 
 * Gets whether event debugging output is enabled.
 * 
 * Returns: %TRUE if event debugging output is enabled.
 **/
gboolean
cdk_get_show_events (void)
{
  return (_cdk_debug_flags & CDK_DEBUG_EVENTS) != 0;
}

static void
cdk_synthesize_click (CdkDisplay *display,
                      CdkEvent   *event,
                      gint        nclicks)
{
  CdkEvent *event_copy;

  event_copy = cdk_event_copy (event);
  event_copy->type = (nclicks == 2) ? CDK_2BUTTON_PRESS : CDK_3BUTTON_PRESS;

  _cdk_event_queue_append (display, event_copy);
}

void
_cdk_event_button_generate (CdkDisplay *display,
			    CdkEvent   *event)
{
  CdkMultipleClickInfo *info;
  CdkDevice *source_device;

  g_return_if_fail (event->type == CDK_BUTTON_PRESS);

  source_device = cdk_event_get_source_device (event);
  info = g_hash_table_lookup (display->multiple_click_info, event->button.device);

  if (G_UNLIKELY (!info))
    {
      info = g_new0 (CdkMultipleClickInfo, 1);
      info->button_number[0] = info->button_number[1] = -1;

      g_hash_table_insert (display->multiple_click_info,
                           event->button.device, info);
    }

  if ((event->button.time < (info->button_click_time[1] + 2 * display->double_click_time)) &&
      (event->button.window == info->button_window[1]) &&
      (event->button.button == info->button_number[1]) &&
      (source_device == info->last_slave) &&
      (ABS (event->button.x - info->button_x[1]) <= display->double_click_distance) &&
      (ABS (event->button.y - info->button_y[1]) <= display->double_click_distance))
    {
      cdk_synthesize_click (display, event, 3);

      info->button_click_time[1] = 0;
      info->button_click_time[0] = 0;
      info->button_window[1] = NULL;
      info->button_window[0] = NULL;
      info->button_number[1] = -1;
      info->button_number[0] = -1;
      info->button_x[0] = info->button_x[1] = 0;
      info->button_y[0] = info->button_y[1] = 0;
      info->last_slave = NULL;
    }
  else if ((event->button.time < (info->button_click_time[0] + display->double_click_time)) &&
	   (event->button.window == info->button_window[0]) &&
	   (event->button.button == info->button_number[0]) &&
           (source_device == info->last_slave) &&
	   (ABS (event->button.x - info->button_x[0]) <= display->double_click_distance) &&
	   (ABS (event->button.y - info->button_y[0]) <= display->double_click_distance))
    {
      cdk_synthesize_click (display, event, 2);
      
      info->button_click_time[1] = info->button_click_time[0];
      info->button_click_time[0] = event->button.time;
      info->button_window[1] = info->button_window[0];
      info->button_window[0] = event->button.window;
      info->button_number[1] = info->button_number[0];
      info->button_number[0] = event->button.button;
      info->button_x[1] = info->button_x[0];
      info->button_x[0] = event->button.x;
      info->button_y[1] = info->button_y[0];
      info->button_y[0] = event->button.y;
      info->last_slave = source_device;
    }
  else
    {
      info->button_click_time[1] = 0;
      info->button_click_time[0] = event->button.time;
      info->button_window[1] = NULL;
      info->button_window[0] = event->button.window;
      info->button_number[1] = -1;
      info->button_number[0] = event->button.button;
      info->button_x[1] = 0;
      info->button_x[0] = event->button.x;
      info->button_y[1] = 0;
      info->button_y[0] = event->button.y;
      info->last_slave = source_device;
    }
}

static GList *
cdk_get_pending_window_state_event_link (CdkWindow *window)
{
  CdkDisplay *display = cdk_window_get_display (window);
  GList *tmp_list;

  for (tmp_list = display->queued_events; tmp_list; tmp_list = tmp_list->next)
    {
      CdkEventPrivate *event = tmp_list->data;

      if (event->event.type == CDK_WINDOW_STATE &&
          event->event.window_state.window == window)
        return tmp_list;
    }

  return NULL;
}

void
_cdk_set_window_state (CdkWindow      *window,
                       CdkWindowState  new_state)
{
  CdkDisplay *display = cdk_window_get_display (window);
  CdkEvent temp_event;
  CdkWindowState old;
  GList *pending_event_link;

  g_return_if_fail (window != NULL);

  temp_event.window_state.window = window;
  temp_event.window_state.type = CDK_WINDOW_STATE;
  temp_event.window_state.send_event = FALSE;
  temp_event.window_state.new_window_state = new_state;

  if (temp_event.window_state.new_window_state == window->state)
    return; /* No actual work to do, nothing changed. */

  pending_event_link = cdk_get_pending_window_state_event_link (window);
  if (pending_event_link)
    {
      old = window->old_state;
      _cdk_event_queue_remove_link (display, pending_event_link);
      cdk_event_free (pending_event_link->data);
      g_list_free_1 (pending_event_link);
    }
  else
    {
      old = window->state;
      window->old_state = old;
    }

  temp_event.window_state.changed_mask = new_state ^ old;

  /* Actually update the field in CdkWindow, this is sort of an odd
   * place to do it, but seems like the safest since it ensures we expose no
   * inconsistent state to the user.
   */

  window->state = new_state;

  if (temp_event.window_state.changed_mask & CDK_WINDOW_STATE_WITHDRAWN)
    _cdk_window_update_viewable (window);

  /* We only really send the event to toplevels, since
   * all the window states don't apply to non-toplevels.
   * Non-toplevels do use the CDK_WINDOW_STATE_WITHDRAWN flag
   * internally so we needed to update window->state.
   */
  switch (window->window_type)
    {
    case CDK_WINDOW_TOPLEVEL:
    case CDK_WINDOW_TEMP: /* ? */
      cdk_display_put_event (display, &temp_event);
      break;
    case CDK_WINDOW_FOREIGN:
    case CDK_WINDOW_ROOT:
    case CDK_WINDOW_CHILD:
      break;
    }
}

void
cdk_synthesize_window_state (CdkWindow     *window,
                             CdkWindowState unset_flags,
                             CdkWindowState set_flags)
{
  g_return_if_fail (window != NULL);

  _cdk_set_window_state (window, (window->state | set_flags) & ~unset_flags);
}

/**
 * cdk_display_set_double_click_time:
 * @display: a #CdkDisplay
 * @msec: double click time in milliseconds (thousandths of a second) 
 * 
 * Sets the double click time (two clicks within this time interval
 * count as a double click and result in a #CDK_2BUTTON_PRESS event).
 * Applications should not set this, it is a global 
 * user-configured setting.
 *
 * Since: 2.2
 **/
void
cdk_display_set_double_click_time (CdkDisplay *display,
				   guint       msec)
{
  display->double_click_time = msec;
}

/**
 * cdk_set_double_click_time:
 * @msec: double click time in milliseconds (thousandths of a second)
 *
 * Set the double click time for the default display. See
 * cdk_display_set_double_click_time(). 
 * See also cdk_display_set_double_click_distance().
 * Applications should not set this, it is a 
 * global user-configured setting.
 **/
void
cdk_set_double_click_time (guint msec)
{
  cdk_display_set_double_click_time (cdk_display_get_default (), msec);
}

/**
 * cdk_display_set_double_click_distance:
 * @display: a #CdkDisplay
 * @distance: distance in pixels
 * 
 * Sets the double click distance (two clicks within this distance
 * count as a double click and result in a #CDK_2BUTTON_PRESS event).
 * See also cdk_display_set_double_click_time().
 * Applications should not set this, it is a global 
 * user-configured setting.
 *
 * Since: 2.4
 **/
void
cdk_display_set_double_click_distance (CdkDisplay *display,
				       guint       distance)
{
  display->double_click_distance = distance;
}

G_DEFINE_BOXED_TYPE (CdkEvent, cdk_event,
                     cdk_event_copy,
                     cdk_event_free)

static CdkEventSequence *
cdk_event_sequence_copy (CdkEventSequence *sequence)
{
  return sequence;
}

static void
cdk_event_sequence_free (CdkEventSequence *sequence G_GNUC_UNUSED)
{
  /* Nothing to free here */
}

G_DEFINE_BOXED_TYPE (CdkEventSequence, cdk_event_sequence,
                     cdk_event_sequence_copy,
                     cdk_event_sequence_free)

/**
 * cdk_setting_get:
 * @name: the name of the setting.
 * @value: location to store the value of the setting.
 *
 * Obtains a desktop-wide setting, such as the double-click time,
 * for the default screen. See cdk_screen_get_setting().
 *
 * Returns: %TRUE if the setting existed and a value was stored
 *   in @value, %FALSE otherwise.
 **/
gboolean
cdk_setting_get (const gchar *name,
		 GValue      *value)
{
  return cdk_screen_get_setting (cdk_screen_get_default (), name, value);
}

/**
 * cdk_event_get_event_type:
 * @event: a #CdkEvent
 *
 * Retrieves the type of the event.
 *
 * Returns: a #CdkEventType
 *
 * Since: 3.10
 */
CdkEventType
cdk_event_get_event_type (const CdkEvent *event)
{
  g_return_val_if_fail (event != NULL, CDK_NOTHING);

  return event->type;
}

/**
 * cdk_event_get_seat:
 * @event: a #CdkEvent
 *
 * Returns the #CdkSeat this event was generated for.
 *
 * Returns: (transfer none): The #CdkSeat of this event
 *
 * Since: 3.20
 **/
CdkSeat *
cdk_event_get_seat (const CdkEvent *event)
{
  const CdkEventPrivate *priv;

  if (!cdk_event_is_allocated (event))
    return NULL;

  priv = (const CdkEventPrivate *) event;

  if (!priv->seat)
    {
      CdkDevice *device;

      g_warning ("Event with type %d not holding a CdkSeat. "
                 "It is most likely synthesized outside Cdk/CTK+",
                 event->type);

      device = cdk_event_get_device (event);

      return device ? cdk_device_get_seat (device) : NULL;
    }

  return priv->seat;
}

void
cdk_event_set_seat (CdkEvent *event,
                    CdkSeat  *seat)
{
  CdkEventPrivate *priv;

  if (cdk_event_is_allocated (event))
    {
      priv = (CdkEventPrivate *) event;
      priv->seat = seat;
    }
}

/**
 * cdk_event_get_device_tool:
 * @event: a #CdkEvent
 *
 * If the event was generated by a device that supports
 * different tools (eg. a tablet), this function will
 * return a #CdkDeviceTool representing the tool that
 * caused the event. Otherwise, %NULL will be returned.
 *
 * Note: the #CdkDeviceTool<!-- -->s will be constant during
 * the application lifetime, if settings must be stored
 * persistently across runs, see cdk_device_tool_get_serial()
 *
 * Returns: (transfer none): The current device tool, or %NULL
 *
 * Since: 3.22
 **/
CdkDeviceTool *
cdk_event_get_device_tool (const CdkEvent *event)
{
  CdkEventPrivate *private;

  if (!cdk_event_is_allocated (event))
    return NULL;

  private = (CdkEventPrivate *) event;
  return private->tool;
}

/**
 * cdk_event_set_device_tool:
 * @event: a #CdkEvent
 * @tool: (nullable): tool to set on the event, or %NULL
 *
 * Sets the device tool for this event, should be rarely used.
 *
 * Since: 3.22
 **/
void
cdk_event_set_device_tool (CdkEvent      *event,
                           CdkDeviceTool *tool)
{
  CdkEventPrivate *private;

  if (!cdk_event_is_allocated (event))
    return;

  private = (CdkEventPrivate *) event;
  private->tool = tool;
}

void
cdk_event_set_scancode (CdkEvent *event,
                        guint16 scancode)
{
  CdkEventPrivate *private = (CdkEventPrivate *) event;

  private->key_scancode = scancode;
}

/**
 * cdk_event_get_scancode:
 * @event: a #CdkEvent
 *
 * Gets the keyboard low-level scancode of a key event.
 *
 * This is usually hardware_keycode. On Windows this is the high
 * word of WM_KEY{DOWN,UP} lParam which contains the scancode and
 * some extended flags.
 *
 * Returns: The associated keyboard scancode or 0
 *
 * Since: 3.22
 **/
int
cdk_event_get_scancode (CdkEvent *event)
{
  CdkEventPrivate *private;

  if (!cdk_event_is_allocated (event))
    return 0;

  private = (CdkEventPrivate *) event;
  return private->key_scancode;
}
