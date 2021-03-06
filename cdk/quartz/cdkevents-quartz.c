/* cdkevents-quartz.c
 *
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-2002 Tor Lillqvist
 * Copyright (C) 2005-2008 Imendio AB
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
#include <sys/types.h>
#include <sys/sysctl.h>
#include <pthread.h>
#include <unistd.h>

#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#include <cdk/cdkdisplayprivate.h>

#include "cdkscreen.h"
#include "cdkkeysyms.h"
#include "cdkquartz.h"
#include "cdkquartzdisplay.h"
#include "cdkprivate-quartz.h"
#include "cdkinternal-quartz.h"
#include "cdkquartzdevicemanager-core.h"
#include "cdkquartzkeys.h"
#include "cdkkeys-quartz.h"

#define GRIP_WIDTH 15
#define GRIP_HEIGHT 15
#define CDK_LION_RESIZE 5
#define TABLET_AXES 5

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1060
#define NSEventTypeRotate 13
#define NSEventTypeMagnify 30
#endif

#define WINDOW_IS_TOPLEVEL(window)		     \
  (CDK_WINDOW_TYPE (window) != CDK_WINDOW_CHILD &&   \
   CDK_WINDOW_TYPE (window) != CDK_WINDOW_FOREIGN && \
   CDK_WINDOW_TYPE (window) != CDK_WINDOW_OFFSCREEN)


/* This is the window corresponding to the key window */
static CdkWindow   *current_keyboard_window;


static void append_event                        (CdkEvent  *event,
                                                 gboolean   windowing);

static CdkWindow *find_toplevel_under_pointer   (CdkDisplay *display,
                                                 NSPoint     screen_point,
                                                 gint       *x,
                                                 gint       *y);


static void
cdk_quartz_ns_notification_callback (CFNotificationCenterRef  center,
                                     void                    *observer,
                                     CFStringRef              name,
                                     const void              *object,
                                     CFDictionaryRef          userInfo)
{
  CdkEvent new_event;

  new_event.type = CDK_SETTING;
  new_event.setting.window = cdk_screen_get_root_window (_cdk_screen);
  new_event.setting.send_event = FALSE;
  new_event.setting.action = CDK_SETTING_ACTION_CHANGED;
  new_event.setting.name = NULL;

  /* Translate name */
  if (CFStringCompare (name,
                       CFSTR("AppleNoRedisplayAppearancePreferenceChanged"),
                       0) == kCFCompareEqualTo)
    new_event.setting.name = "ctk-primary-button-warps-slider";

  if (!new_event.setting.name)
    return;

  cdk_event_put (&new_event);
}

static void
cdk_quartz_events_init_notifications (void)
{
  static gboolean notifications_initialized = FALSE;

  if (notifications_initialized)
    return;
  notifications_initialized = TRUE;

  /* Initialize any handlers for notifications we want to push to CTK
   * through CdkEventSettings.
   */

  /* This is an undocumented *distributed* notification to listen for changes
   * in scrollbar jump behavior. It is used by LibreOffice and WebKit as well.
   */
  CFNotificationCenterAddObserver (CFNotificationCenterGetDistributedCenter (),
                                   NULL,
                                   &cdk_quartz_ns_notification_callback,
                                   CFSTR ("AppleNoRedisplayAppearancePreferenceChanged"),
                                   NULL,
                                   CFNotificationSuspensionBehaviorDeliverImmediately);
}

void
_cdk_quartz_events_init (void)
{
  _cdk_quartz_event_loop_init ();
  cdk_quartz_events_init_notifications ();

  current_keyboard_window = g_object_ref (_cdk_root);
}

gboolean
_cdk_quartz_display_has_pending (CdkDisplay *display)
{
  return (_cdk_event_queue_find_first (display) ||
         (_cdk_quartz_event_loop_check_pending ()));
}

void
_cdk_quartz_events_break_all_grabs (guint32 time)
{
  CdkSeat *seat = cdk_display_get_default_seat (_cdk_display);
  cdk_seat_ungrab (seat);
}

static void
fixup_event (CdkEvent *event)
{
  if (event->any.window)
    g_object_ref (event->any.window);
  if (((event->any.type == CDK_ENTER_NOTIFY) ||
       (event->any.type == CDK_LEAVE_NOTIFY)) &&
      (event->crossing.subwindow != NULL))
    g_object_ref (event->crossing.subwindow);
  event->any.send_event = FALSE;
}

static void
append_event (CdkEvent *event,
              gboolean  windowing)
{
  GList *node;

  fixup_event (event);
  node = _cdk_event_queue_append (_cdk_display, event);

  if (windowing)
    _cdk_windowing_got_event (_cdk_display, node, event, 0);
}

static gint
cdk_event_apply_filters (NSEvent *nsevent,
			 CdkEvent *event,
			 GList **filters)
{
  GList *tmp_list;
  CdkFilterReturn result;

  tmp_list = *filters;

  while (tmp_list)
    {
      CdkEventFilter *filter = (CdkEventFilter*) tmp_list->data;
      GList *node;

      if ((filter->flags & CDK_EVENT_FILTER_REMOVED) != 0)
        {
          tmp_list = tmp_list->next;
          continue;
        }

      filter->ref_count++;
      result = filter->function (nsevent, event, filter->data);

      /* get the next node after running the function since the
         function may add or remove a next node */
      node = tmp_list;
      tmp_list = tmp_list->next;

      filter->ref_count--;
      if (filter->ref_count == 0)
        {
          *filters = g_list_remove_link (*filters, node);
          g_list_free_1 (node);
          g_free (filter);
        }

      if (result !=  CDK_FILTER_CONTINUE)
	return result;
    }

  return CDK_FILTER_CONTINUE;
}

static guint32
get_time_from_ns_event (NSEvent *event)
{
  double time = [event timestamp];

  /* cast via double->uint64 conversion to make sure that it is
   * wrapped on 32-bit machines when it overflows
   */
  return (guint32) (guint64) (time * 1000.0);
}

static int
get_mouse_button_from_ns_event (NSEvent *event)
{
  NSInteger button;

  button = [event buttonNumber];

  switch (button)
    {
    case 0:
      return 1;
    case 1:
      return 3;
    case 2:
      return 2;
    default:
      return button + 1;
    }
}

static CdkModifierType
get_mouse_button_modifiers_from_ns_buttons (NSUInteger nsbuttons)
{
  CdkModifierType modifiers = 0;

  if (nsbuttons & (1 << 0))
    modifiers |= CDK_BUTTON1_MASK;
  if (nsbuttons & (1 << 1))
    modifiers |= CDK_BUTTON3_MASK;
  if (nsbuttons & (1 << 2))
    modifiers |= CDK_BUTTON2_MASK;
  if (nsbuttons & (1 << 3))
    modifiers |= CDK_BUTTON4_MASK;
  if (nsbuttons & (1 << 4))
    modifiers |= CDK_BUTTON5_MASK;

  return modifiers;
}

static CdkModifierType
get_mouse_button_modifiers_from_ns_event (NSEvent *event)
{
  int button;
  CdkModifierType state = 0;

  /* This maps buttons 1 to 5 to CDK_BUTTON[1-5]_MASK */
  button = get_mouse_button_from_ns_event (event);
  if (button >= 1 && button <= 5)
    state = (1 << (button + 7));

  return state;
}

static CdkModifierType
get_keyboard_modifiers_from_ns_flags (NSUInteger nsflags)
{
  CdkModifierType modifiers = 0;

  if (nsflags & CDK_QUARTZ_ALPHA_SHIFT_KEY_MASK)
    modifiers |= CDK_LOCK_MASK;
  if (nsflags & CDK_QUARTZ_SHIFT_KEY_MASK)
    modifiers |= CDK_SHIFT_MASK;
  if (nsflags & CDK_QUARTZ_CONTROL_KEY_MASK)
    modifiers |= CDK_CONTROL_MASK;
  if (nsflags & CDK_QUARTZ_ALTERNATE_KEY_MASK)
    modifiers |= CDK_MOD1_MASK;
  if (nsflags & CDK_QUARTZ_COMMAND_KEY_MASK)
    modifiers |= CDK_MOD2_MASK;

  return modifiers;
}

static CdkModifierType
get_keyboard_modifiers_from_ns_event (NSEvent *nsevent)
{
  return get_keyboard_modifiers_from_ns_flags ([nsevent modifierFlags]);
}

/* Return an event mask from an NSEvent */
static CdkEventMask
get_event_mask_from_ns_event (NSEvent *nsevent)
{
  switch ([nsevent type])
    {
    case CDK_QUARTZ_LEFT_MOUSE_DOWN:
    case CDK_QUARTZ_RIGHT_MOUSE_DOWN:
    case CDK_QUARTZ_OTHER_MOUSE_DOWN:
      return CDK_BUTTON_PRESS_MASK;
    case CDK_QUARTZ_LEFT_MOUSE_UP:
    case CDK_QUARTZ_RIGHT_MOUSE_UP:
    case CDK_QUARTZ_OTHER_MOUSE_UP:
      return CDK_BUTTON_RELEASE_MASK;
    case CDK_QUARTZ_MOUSE_MOVED:
      return CDK_POINTER_MOTION_MASK | CDK_POINTER_MOTION_HINT_MASK;
    case CDK_QUARTZ_SCROLL_WHEEL:
      /* Since applications that want button press events can get
       * scroll events on X11 (since scroll wheel events are really
       * button press events there), we need to use CDK_BUTTON_PRESS_MASK too.
       */
      return CDK_SCROLL_MASK | CDK_BUTTON_PRESS_MASK;
    case CDK_QUARTZ_LEFT_MOUSE_DRAGGED:
      return (CDK_POINTER_MOTION_MASK | CDK_POINTER_MOTION_HINT_MASK |
	      CDK_BUTTON_MOTION_MASK | CDK_BUTTON1_MOTION_MASK | 
	      CDK_BUTTON1_MASK);
    case CDK_QUARTZ_RIGHT_MOUSE_DRAGGED:
      return (CDK_POINTER_MOTION_MASK | CDK_POINTER_MOTION_HINT_MASK |
	      CDK_BUTTON_MOTION_MASK | CDK_BUTTON3_MOTION_MASK | 
	      CDK_BUTTON3_MASK);
    case CDK_QUARTZ_OTHER_MOUSE_DRAGGED:
      {
	CdkEventMask mask;

	mask = (CDK_POINTER_MOTION_MASK |
		CDK_POINTER_MOTION_HINT_MASK |
		CDK_BUTTON_MOTION_MASK);

	if (get_mouse_button_from_ns_event (nsevent) == 2)
	  mask |= (CDK_BUTTON2_MOTION_MASK | CDK_BUTTON2_MOTION_MASK | 
		   CDK_BUTTON2_MASK);

	return mask;
      }
    case NSEventTypeMagnify:
    case NSEventTypeRotate:
      return CDK_TOUCHPAD_GESTURE_MASK;
    case CDK_QUARTZ_KEY_DOWN:
    case CDK_QUARTZ_KEY_UP:
    case CDK_QUARTZ_FLAGS_CHANGED:
      {
        switch (_cdk_quartz_keys_event_type (nsevent))
	  {
	  case CDK_KEY_PRESS:
	    return CDK_KEY_PRESS_MASK;
	  case CDK_KEY_RELEASE:
	    return CDK_KEY_RELEASE_MASK;
	  case CDK_NOTHING:
	    return 0;
	  default:
	    g_assert_not_reached ();
	  }
      }
      break;

    case CDK_QUARTZ_MOUSE_ENTERED:
      return CDK_ENTER_NOTIFY_MASK;

    case CDK_QUARTZ_MOUSE_EXITED:
      return CDK_LEAVE_NOTIFY_MASK;

    default:
      g_assert_not_reached ();
    }

  return 0;
}

static void
get_window_point_from_screen_point (CdkWindow *window,
                                    NSPoint    screen_point,
                                    gint      *x,
                                    gint      *y)
{
  NSPoint point;
  CdkQuartzNSWindow *nswindow;

  nswindow = (CdkQuartzNSWindow*)(((CdkWindowImplQuartz *)window->impl)->toplevel);
  point = [nswindow convertPointFromScreen:screen_point];
  *x = point.x;
  *y = window->height - point.y;
}

static gboolean
is_mouse_button_press_event (NSEventType type)
{
  switch (type)
    {
      case CDK_QUARTZ_LEFT_MOUSE_DOWN:
      case CDK_QUARTZ_RIGHT_MOUSE_DOWN:
      case CDK_QUARTZ_OTHER_MOUSE_DOWN:
        return TRUE;
    default:
      return FALSE;
    }

  return FALSE;
}

static CdkWindow *
get_toplevel_from_ns_event (NSEvent *nsevent,
                            NSPoint *screen_point,
                            gint    *x,
                            gint    *y)
{
  CdkWindow *toplevel = NULL;

  if ([nsevent window])
    {
      CdkQuartzView *view;
      NSPoint point, view_point;
      NSRect view_frame;

      view = (CdkQuartzView *)[[nsevent window] contentView];

      toplevel = [view cdkWindow];

      point = [nsevent locationInWindow];
      view_point = [view convertPoint:point fromView:nil];
      view_frame = [view frame];

      /* NSEvents come in with a window set, but with window coordinates
       * out of window bounds. For e.g. moved events this is fine, we use
       * this information to properly handle enter/leave notify and motion
       * events. For mouse button press/release, we want to avoid forwarding
       * these events however, because the window they relate to is not the
       * window set in the event. This situation appears to occur when button
       * presses come in just before (or just after?) a window is resized and
       * also when a button press occurs on the OS X window titlebar.
       *
       * By setting toplevel to NULL, we do another attempt to get the right
       * toplevel window below.
       */
      if (is_mouse_button_press_event ([nsevent type]) &&
          (view_point.x < view_frame.origin.x ||
           view_point.x >= view_frame.origin.x + view_frame.size.width ||
           view_point.y < view_frame.origin.y ||
           view_point.y >= view_frame.origin.y + view_frame.size.height))
        {
          toplevel = NULL;

          /* This is a hack for button presses to break all grabs. E.g. if
           * a menu is open and one clicks on the title bar (or anywhere
           * out of window bounds), we really want to pop down the menu (by
           * breaking the grabs) before OS X handles the action of the title
           * bar button.
           *
           * Because we cannot ingest this event into CDK, we have to do it
           * here, not very nice.
           */
          _cdk_quartz_events_break_all_grabs (get_time_from_ns_event (nsevent));

          /* Check if the event occurred on the titlebar. If it did,
           * explicitly return NULL to prevent going through the
           * fallback path, which could match the window that is
           * directly under the titlebar.
           */
          if (view_point.y < 0 &&
              view_point.x >= view_frame.origin.x &&
              view_point.x < view_frame.origin.x + view_frame.size.width)
            {
              NSView *superview = [view superview];
              if (superview)
                {
                  NSRect superview_frame = [superview frame];
                  int titlebar_height = superview_frame.size.height -
                                        view_frame.size.height;

                  if (titlebar_height > 0 && view_point.y >= -titlebar_height)
                    {
                      return NULL;
                    }
                }
            }
        }
      else
        {
	  *screen_point = [(CdkQuartzNSWindow*)[nsevent window] convertPointToScreen:point];
          *x = point.x;
          *y = toplevel->height - point.y;
        }
    }

  if (!toplevel)
    {
      /* Fallback used when no NSWindow set.  This happens e.g. when
       * we allow motion events without a window set in cdk_event_translate()
       * that occur immediately after the main menu bar was clicked/used.
       * This fallback will not return coordinates contained in a window's
       * titlebar.
       */
      *screen_point = [NSEvent mouseLocation];
      toplevel = find_toplevel_under_pointer (_cdk_display,
                                              *screen_point,
                                              x, y);
    }

  return toplevel;
}

static CdkEvent *
create_focus_event (CdkWindow *window,
		    gboolean   in)
{
  CdkEvent *event;
  CdkDisplay *display = cdk_window_get_display (window);
  CdkSeat *seat = cdk_display_get_default_seat (display);

  event = cdk_event_new (CDK_FOCUS_CHANGE);
  event->focus_change.window = window;
  event->focus_change.in = in;

  cdk_event_set_device (event, cdk_seat_get_keyboard (seat));
  cdk_event_set_seat (event, seat);

  return event;
}


static void
generate_motion_event (CdkWindow *window)
{
  NSPoint screen_point;
  CdkEvent *event;
  gint x, y, x_root, y_root;
  CdkDisplay *display = cdk_window_get_display (window);
  CdkSeat *seat = cdk_display_get_default_seat (display);

  event = cdk_event_new (CDK_MOTION_NOTIFY);
  event->any.window = NULL;
  event->any.send_event = TRUE;

  screen_point = [NSEvent mouseLocation];

  _cdk_quartz_window_nspoint_to_cdk_xy (screen_point, &x_root, &y_root);
  get_window_point_from_screen_point (window, screen_point, &x, &y);

  event->any.type = CDK_MOTION_NOTIFY;
  event->motion.window = window;
  event->motion.time = get_time_from_ns_event ([NSApp currentEvent]);
  event->motion.x = x;
  event->motion.y = y;
  event->motion.x_root = x_root;
  event->motion.y_root = y_root;
  /* FIXME event->axes */
  event->motion.state = _cdk_quartz_events_get_current_keyboard_modifiers () |
                        _cdk_quartz_events_get_current_mouse_modifiers ();
  event->motion.is_hint = FALSE;
  cdk_event_set_device (event, cdk_seat_get_pointer (seat));
  cdk_event_set_seat (event, seat);

  append_event (event, TRUE);
}

/* Note: Used to both set a new focus window and to unset the old one. */
void
_cdk_quartz_events_update_focus_window (CdkWindow *window,
					gboolean   got_focus)
{
  CdkEvent *event;

  if (got_focus && window == current_keyboard_window)
    return;

  /* FIXME: Don't do this when grabbed? Or make CdkQuartzNSWindow
   * disallow it in the first place instead?
   */
  
  if (!got_focus && window == current_keyboard_window)
    {
      event = create_focus_event (current_keyboard_window, FALSE);
      append_event (event, FALSE);
      g_object_unref (current_keyboard_window);
      current_keyboard_window = NULL;
    }

  if (got_focus)
    {
      if (current_keyboard_window)
	{
	  event = create_focus_event (current_keyboard_window, FALSE);
	  append_event (event, FALSE);
	  g_object_unref (current_keyboard_window);
	  current_keyboard_window = NULL;
	}
      
      event = create_focus_event (window, TRUE);
      append_event (event, FALSE);
      current_keyboard_window = g_object_ref (window);

      /* We just became the active window.  Unlike X11, Mac OS X does
       * not send us motion events while the window does not have focus
       * ("is not key").  We send a dummy motion notify event now, so that
       * everything in the window is set to correct state.
       */
      generate_motion_event (window);
    }
}

void
_cdk_quartz_events_send_map_event (CdkWindow *window)
{
  CdkWindowImplQuartz *impl = CDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (!impl->toplevel)
    return;

  if (window->event_mask & CDK_STRUCTURE_MASK)
    {
      CdkEvent event;

      event.any.type = CDK_MAP;
      event.any.window = window;
  
      cdk_event_put (&event);
    }
}

static CdkWindow *
find_toplevel_under_pointer (CdkDisplay *display,
                             NSPoint     screen_point,
                             gint       *x,
                             gint       *y)
{
  CdkWindow *toplevel;
  CdkPointerWindowInfo *info;
  CdkSeat *seat = cdk_display_get_default_seat (display);

  info = _cdk_display_get_pointer_info (display, cdk_seat_get_pointer (seat));
  toplevel = info->toplevel_under_pointer;

  if (!(toplevel && WINDOW_IS_TOPLEVEL (toplevel)))
    {
      gint cdk_x = 0, cdk_y = 0;
      CdkDevice *pointer = cdk_seat_get_pointer(seat);
      _cdk_quartz_window_nspoint_to_cdk_xy (screen_point, &cdk_x, &cdk_y);
      toplevel = cdk_device_get_window_at_position (pointer, &cdk_x, &cdk_y);

      if (toplevel && ! WINDOW_IS_TOPLEVEL (toplevel))
        toplevel = cdk_window_get_toplevel (toplevel);

      if (toplevel)
        info->toplevel_under_pointer = g_object_ref (toplevel);
      else
        info->toplevel_under_pointer = NULL;

    }

  if (toplevel)
    {
      get_window_point_from_screen_point (toplevel, screen_point, x, y);
      /* If the coordinates are out of window bounds, this toplevel is not
       * under the pointer and we thus return NULL. This can occur when
       * toplevel under pointer has not yet been updated due to a very recent
       * window resize. Alternatively, we should no longer be relying on
       * the toplevel_under_pointer value which is maintained in cdkwindow.c.
       */
      if (*x < 0 || *y < 0 || *x >= toplevel->width || *y >= toplevel->height)
        return NULL;
    }

  return toplevel;
}

static CdkWindow *
find_toplevel_for_keyboard_event (NSEvent *nsevent)
{
  CdkQuartzView *view = (CdkQuartzView *)[[nsevent window] contentView];
  CdkWindow *window  = [view cdkWindow];
  CdkDisplay *display = cdk_window_get_display (window);
  CdkSeat *seat = cdk_display_get_default_seat (display);
  CdkDevice *device = cdk_seat_get_keyboard (seat);
  CdkDeviceGrabInfo *grab = _cdk_display_get_last_device_grab (display, device);

  if (grab && grab->window && !grab->owner_events)
    window = cdk_window_get_effective_toplevel (grab->window);

  return window;
}

static CdkWindow *
find_toplevel_for_mouse_event (NSEvent    *nsevent,
                               gint       *x,
                               gint       *y)
{
  NSPoint screen_point;
  NSEventType event_type;
  CdkWindow *toplevel;
  CdkDisplay *display;
  CdkDeviceGrabInfo *grab;
  CdkSeat *seat;

  toplevel = get_toplevel_from_ns_event (nsevent, &screen_point, x, y);

  display = cdk_window_get_display (toplevel);
  seat = cdk_display_get_default_seat (_cdk_display);
  
  event_type = [nsevent type];

  /* From the docs for XGrabPointer:
   *
   * If owner_events is True and if a generated pointer event
   * would normally be reported to this client, it is reported
   * as usual. Otherwise, the event is reported with respect to
   * the grab_window and is reported only if selected by
   * event_mask. For either value of owner_events, unreported
   * events are discarded.
   */
  grab = _cdk_display_get_last_device_grab (display,
                                            cdk_seat_get_pointer (seat));
  if (WINDOW_IS_TOPLEVEL (toplevel) && grab)
    {
      /* Implicit grabs do not go through XGrabPointer and thus the
       * event mask should not be checked.
       */
      if (!grab->implicit
          && (grab->event_mask & get_event_mask_from_ns_event (nsevent)) == 0)
        return NULL;

      if (grab->owner_events)
        {
          /* For owner events, we need to use the toplevel under the
           * pointer, not the window from the NSEvent, since that is
           * reported with respect to the key window, which could be
           * wrong.
           */
          CdkWindow *toplevel_under_pointer;
          gint x_tmp, y_tmp;

          toplevel_under_pointer = find_toplevel_under_pointer (display,
                                                                screen_point,
                                                                &x_tmp, &y_tmp);
          if (toplevel_under_pointer)
            {
              toplevel = toplevel_under_pointer;
              *x = x_tmp;
              *y = y_tmp;
            }

          return toplevel;
        }
      else
        {
          /* Finally check the grab window. */
          CdkWindow *grab_toplevel;

          grab_toplevel = cdk_window_get_effective_toplevel (grab->window);
          get_window_point_from_screen_point (grab_toplevel, screen_point,
                                              x, y);

          return grab_toplevel;
        }

      return NULL;
    }
  else 
    {
      /* The non-grabbed case. */
      CdkWindow *toplevel_under_pointer;
      gint x_tmp, y_tmp;

      /* Ignore all events but mouse moved that might be on the title
       * bar (above the content view). The reason is that otherwise
       * cdk gets confused about getting e.g. button presses with no
       * window (the title bar is not known to it).
       */
      if (event_type != CDK_QUARTZ_MOUSE_MOVED)
        if (*y < 0)
          return NULL;

      /* As for owner events, we need to use the toplevel under the
       * pointer, not the window from the NSEvent.
       */
      toplevel_under_pointer = find_toplevel_under_pointer (display,
                                                            screen_point,
                                                            &x_tmp, &y_tmp);
      if (toplevel_under_pointer
          && WINDOW_IS_TOPLEVEL (toplevel_under_pointer))
        {
          CdkWindowImplQuartz *toplevel_impl;

          toplevel = toplevel_under_pointer;

          toplevel_impl = (CdkWindowImplQuartz *)toplevel->impl;

          *x = x_tmp;
          *y = y_tmp;
        }

      return toplevel;
    }

  return NULL;
}

/* This function finds the correct window to send an event to, taking
 * into account grabs, event propagation, and event masks.
 */
static CdkWindow *
find_window_for_ns_event (NSEvent *nsevent, 
                          gint    *x, 
                          gint    *y,
                          gint    *x_root,
                          gint    *y_root)
{
  CdkQuartzView *view;
  CdkWindow *toplevel;
  NSPoint screen_point;
  NSEventType event_type;

  view = (CdkQuartzView *)[[nsevent window] contentView];

  toplevel = get_toplevel_from_ns_event (nsevent, &screen_point, x, y);
  if (!toplevel)
    return NULL;
  _cdk_quartz_window_nspoint_to_cdk_xy (screen_point, x_root, y_root);

  event_type = [nsevent type];

  switch (event_type)
    {
    case CDK_QUARTZ_LEFT_MOUSE_DOWN:
    case CDK_QUARTZ_RIGHT_MOUSE_DOWN:
    case CDK_QUARTZ_OTHER_MOUSE_DOWN:
    case CDK_QUARTZ_LEFT_MOUSE_UP:
    case CDK_QUARTZ_RIGHT_MOUSE_UP:
    case CDK_QUARTZ_OTHER_MOUSE_UP:
    case CDK_QUARTZ_MOUSE_MOVED:
    case CDK_QUARTZ_SCROLL_WHEEL:
    case CDK_QUARTZ_LEFT_MOUSE_DRAGGED:
    case CDK_QUARTZ_RIGHT_MOUSE_DRAGGED:
    case CDK_QUARTZ_OTHER_MOUSE_DRAGGED:
    case NSEventTypeMagnify:
    case NSEventTypeRotate:
      return find_toplevel_for_mouse_event (nsevent, x, y);

    case CDK_QUARTZ_MOUSE_ENTERED:
    case CDK_QUARTZ_MOUSE_EXITED:
      /* Only handle our own entered/exited events, not the ones for the
       * titlebar buttons.
       */
      if ([view trackingRect] == [nsevent trackingNumber])
        return toplevel;
      else
        return NULL;

    case CDK_QUARTZ_KEY_DOWN:
    case CDK_QUARTZ_KEY_UP:
    case CDK_QUARTZ_FLAGS_CHANGED:
      return find_toplevel_for_keyboard_event (nsevent);

    default:
      /* Ignore everything else. */
      break;
    }

  return NULL;
}

static void
fill_crossing_event (CdkWindow       *toplevel,
                     CdkEvent        *event,
                     NSEvent         *nsevent,
                     gint             x,
                     gint             y,
                     gint             x_root,
                     gint             y_root,
                     CdkEventType     event_type,
                     CdkCrossingMode  mode,
                     CdkNotifyType    detail)
{
  CdkSeat *seat = cdk_display_get_default_seat (_cdk_display);

  event->any.type = event_type;
  event->crossing.window = toplevel;
  event->crossing.subwindow = NULL;
  event->crossing.time = get_time_from_ns_event (nsevent);
  event->crossing.x = x;
  event->crossing.y = y;
  event->crossing.x_root = x_root;
  event->crossing.y_root = y_root;
  event->crossing.mode = mode;
  event->crossing.detail = detail;
  event->crossing.state = get_keyboard_modifiers_from_ns_event (nsevent) |
                         _cdk_quartz_events_get_current_mouse_modifiers ();

  cdk_event_set_device (event, cdk_seat_get_pointer (seat));
  cdk_event_set_seat (event, seat);

  /* FIXME: Focus and button state? */
}

/* fill_pinch_event handles the conversion from the two OSX gesture events
   NSEventTypeMagnfiy and NSEventTypeRotate to the CDK_TOUCHPAD_PINCH event.
   The normal behavior of the OSX events is that they produce as sequence of
     1 x NSEventPhaseBegan,
     n x NSEventPhaseChanged,
     1 x NSEventPhaseEnded
   This can happen for both the Magnify and the Rotate events independently.
   As both events are summarized in one CDK_TOUCHPAD_PINCH event sequence, a
   little state machine handles the case of two NSEventPhaseBegan events in
   a sequence, e.g. Magnify(Began), Magnify(Changed)..., Rotate(Began)...
   such that PINCH(STARTED), PINCH(UPDATE).... will not show a second
   PINCH(STARTED) event.
*/
#ifdef AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
static void
fill_pinch_event (CdkWindow *window,
                  CdkEvent  *event,
                  NSEvent   *nsevent,
                  gint       x,
                  gint       y,
                  gint       x_root,
                  gint       y_root)
{
  static double last_scale = 1.0;
  static enum {
    FP_STATE_IDLE,
    FP_STATE_UPDATE
  } last_state = FP_STATE_IDLE;
  CdkSeat *seat = cdk_display_get_default_seat (_cdk_display);

  event->any.type = CDK_TOUCHPAD_PINCH;
  event->touchpad_pinch.window = window;
  event->touchpad_pinch.time = get_time_from_ns_event (nsevent);
  event->touchpad_pinch.x = x;
  event->touchpad_pinch.y = y;
  event->touchpad_pinch.x_root = x_root;
  event->touchpad_pinch.y_root = y_root;
  event->touchpad_pinch.state = get_keyboard_modifiers_from_ns_event (nsevent);
  event->touchpad_pinch.n_fingers = 2;
  event->touchpad_pinch.dx = 0.0;
  event->touchpad_pinch.dy = 0.0;
  cdk_event_set_device (event, cdk_seat_get_pointer (seat));

  switch ([nsevent phase])
    {
    case NSEventPhaseBegan:
      switch (last_state)
        {
        case FP_STATE_IDLE:
          event->touchpad_pinch.phase = CDK_TOUCHPAD_GESTURE_PHASE_BEGIN;
          last_state = FP_STATE_UPDATE;
          last_scale = 1.0;
          break;
        case FP_STATE_UPDATE:
          /* We have already received a PhaseBegan event but no PhaseEnded
             event. This can happen, e.g. Magnify(Began), Magnify(Change)...
             Rotate(Began), Rotate (Change),...., Magnify(End) Rotate(End)
          */
          event->touchpad_pinch.phase = CDK_TOUCHPAD_GESTURE_PHASE_UPDATE;
          break;
        }
      break;
    case NSEventPhaseChanged:
      event->touchpad_pinch.phase = CDK_TOUCHPAD_GESTURE_PHASE_UPDATE;
      break;
    case NSEventPhaseEnded:
      event->touchpad_pinch.phase = CDK_TOUCHPAD_GESTURE_PHASE_END;
      switch (last_state)
        {
        case FP_STATE_IDLE:
          /* We are idle but have received a second PhaseEnded event.
             This can happen because we have Magnify and Rotate OSX
             event sequences. We just send a second end CDK_PHASE_END.
          */
          break;
        case FP_STATE_UPDATE:
          last_state = FP_STATE_IDLE;
          break;
        }
      break;
    case NSEventPhaseCancelled:
      event->touchpad_pinch.phase = CDK_TOUCHPAD_GESTURE_PHASE_CANCEL;
      last_state = FP_STATE_IDLE;
      break;
    case NSEventPhaseMayBegin:
    case NSEventPhaseStationary:
      event->touchpad_pinch.phase = CDK_TOUCHPAD_GESTURE_PHASE_CANCEL;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  switch ([nsevent type])
    {
    case NSEventTypeMagnify:
      last_scale *= [nsevent magnification] + 1.0;
      event->touchpad_pinch.angle_delta = 0.0;
      break;
    case NSEventTypeRotate:
      event->touchpad_pinch.angle_delta = - [nsevent rotation] * G_PI / 180.0;
      break;
    default:
      g_assert_not_reached ();
    }
  event->touchpad_pinch.scale = last_scale;
}
#endif /* OSX Version >= 10.8 */

static void
fill_button_event (CdkWindow *window,
                   CdkEvent  *event,
                   NSEvent   *nsevent,
                   gint       x,
                   gint       y,
                   gint       x_root,
                   gint       y_root)
{
  CdkEventType type;
  CdkDevice *event_device = NULL;
  gdouble *axes = NULL;
  gint state;
  CdkSeat *seat = cdk_display_get_default_seat (_cdk_display);

  state = get_keyboard_modifiers_from_ns_event (nsevent) |
         _cdk_quartz_events_get_current_mouse_modifiers ();

  switch ([nsevent type])
    {
    case CDK_QUARTZ_LEFT_MOUSE_DOWN:
    case CDK_QUARTZ_RIGHT_MOUSE_DOWN:
    case CDK_QUARTZ_OTHER_MOUSE_DOWN:
      type = CDK_BUTTON_PRESS;
      state &= ~get_mouse_button_modifiers_from_ns_event (nsevent);
      break;

    case CDK_QUARTZ_LEFT_MOUSE_UP:
    case CDK_QUARTZ_RIGHT_MOUSE_UP:
    case CDK_QUARTZ_OTHER_MOUSE_UP:
      type = CDK_BUTTON_RELEASE;
      state |= get_mouse_button_modifiers_from_ns_event (nsevent);
      break;

    default:
      g_assert_not_reached ();
    }

  event_device = _cdk_quartz_device_manager_core_device_for_ns_event (cdk_display_get_device_manager (_cdk_display),
                                                                      nsevent);

  if ([nsevent subtype] == CDK_QUARTZ_EVENT_SUBTYPE_TABLET_POINT)
    {
      axes = g_new (gdouble, TABLET_AXES);

      axes[0] = x;
      axes[1] = y;
      axes[2] = [nsevent pressure];
      axes[3] = [nsevent tilt].x;
      axes[4] = [nsevent tilt].y;
    }

  event->any.type = type;
  event->button.window = window;
  event->button.time = get_time_from_ns_event (nsevent);
  event->button.x = x;
  event->button.y = y;
  event->button.x_root = x_root;
  event->button.y_root = y_root;
  event->button.axes = axes;
  event->button.state = state;
  event->button.button = get_mouse_button_from_ns_event (nsevent);

  cdk_event_set_device (event, cdk_seat_get_pointer (seat));
  cdk_event_set_source_device (event, event_device);
  cdk_event_set_seat (event, seat);
}

static void
fill_motion_event (CdkWindow *window,
                   CdkEvent  *event,
                   NSEvent   *nsevent,
                   gint       x,
                   gint       y,
                   gint       x_root,
                   gint       y_root)
{
  CdkSeat *seat = cdk_display_get_default_seat (_cdk_display);
  CdkDevice *event_device = NULL;
  gdouble *axes = NULL;

  event_device = _cdk_quartz_device_manager_core_device_for_ns_event (cdk_display_get_device_manager (_cdk_display),
                                                                      nsevent);

  if ([nsevent subtype] == CDK_QUARTZ_EVENT_SUBTYPE_TABLET_POINT)
    {
      axes = g_new (gdouble, TABLET_AXES);

      axes[0] = x;
      axes[1] = y;
      axes[2] = [nsevent pressure];
      axes[3] = [nsevent tilt].x;
      axes[4] = [nsevent tilt].y;
    }

  event->any.type = CDK_MOTION_NOTIFY;
  event->motion.window = window;
  event->motion.time = get_time_from_ns_event (nsevent);
  event->motion.x = x;
  event->motion.y = y;
  event->motion.x_root = x_root;
  event->motion.y_root = y_root;
  event->motion.axes = axes;
  event->motion.state = get_keyboard_modifiers_from_ns_event (nsevent) |
                        _cdk_quartz_events_get_current_mouse_modifiers ();
  event->motion.is_hint = FALSE;
  cdk_event_set_device (event, cdk_seat_get_pointer (seat));
  cdk_event_set_source_device (event, event_device);

  cdk_event_set_seat (event, seat);
}

static void
fill_scroll_event (CdkWindow          *window,
                   CdkEvent           *event,
                   NSEvent            *nsevent,
                   gint                x,
                   gint                y,
                   gint                x_root,
                   gint                y_root,
                   gdouble             delta_x,
                   gdouble             delta_y,
                   CdkScrollDirection  direction)
{
  CdkSeat *seat = cdk_display_get_default_seat (_cdk_display);
  NSPoint point;

  point = [nsevent locationInWindow];

  event->any.type = CDK_SCROLL;
  event->scroll.window = window;
  event->scroll.time = get_time_from_ns_event (nsevent);
  event->scroll.x = x;
  event->scroll.y = y;
  event->scroll.x_root = x_root;
  event->scroll.y_root = y_root;
  event->scroll.state = get_keyboard_modifiers_from_ns_event (nsevent);
  event->scroll.direction = direction;
  event->scroll.delta_x = delta_x;
  event->scroll.delta_y = delta_y;
  cdk_event_set_device (event, cdk_seat_get_pointer (seat));
  cdk_event_set_seat (event, seat);
}

static void
fill_key_event (CdkWindow    *window,
                CdkEvent     *event,
                NSEvent      *nsevent,
                CdkEventType  type)
{
  CdkEventPrivate *priv;
  CdkSeat *seat = cdk_display_get_default_seat (_cdk_display);
  gchar buf[7];
  gunichar c = 0;

  priv = (CdkEventPrivate *) event;
  priv->windowing_data = [nsevent retain];

  event->any.type = type;
  event->key.window = window;
  event->key.time = get_time_from_ns_event (nsevent);
  event->key.state = get_keyboard_modifiers_from_ns_event (nsevent);
  event->key.hardware_keycode = [nsevent keyCode];
  cdk_event_set_scancode (event, [nsevent keyCode]);
  event->key.group = ([nsevent modifierFlags] & CDK_QUARTZ_ALTERNATE_KEY_MASK) ? 1 : 0;
  event->key.keyval = CDK_KEY_VoidSymbol;

  cdk_event_set_device (event, cdk_seat_get_keyboard (seat));
  cdk_event_set_seat (event, seat);

  cdk_keymap_translate_keyboard_state (cdk_keymap_get_for_display (_cdk_display),
				       event->key.hardware_keycode,
				       event->key.state,
				       event->key.group,
				       &event->key.keyval,
				       NULL, NULL, NULL);

  event->key.is_modifier = _cdk_quartz_keys_is_modifier (event->key.hardware_keycode);

  /* If the key press is a modifier, the state should include the mask
   * for that modifier but only for releases, not presses. This
   * matches the X11 backend behavior.
   */
  if (event->key.is_modifier)
    {
      int mask = 0;

      switch (event->key.keyval)
        {
        case CDK_KEY_Meta_R:
        case CDK_KEY_Meta_L:
          mask = CDK_MOD2_MASK;
          break;
        case CDK_KEY_Shift_R:
        case CDK_KEY_Shift_L:
          mask = CDK_SHIFT_MASK;
          break;
        case CDK_KEY_Caps_Lock:
          mask = CDK_LOCK_MASK;
          break;
        case CDK_KEY_Alt_R:
        case CDK_KEY_Alt_L:
          mask = CDK_MOD1_MASK;
          break;
        case CDK_KEY_Control_R:
        case CDK_KEY_Control_L:
          mask = CDK_CONTROL_MASK;
          break;
        default:
          mask = 0;
        }

      if (type == CDK_KEY_PRESS)
        event->key.state &= ~mask;
      else if (type == CDK_KEY_RELEASE)
        event->key.state |= mask;
    }

  event->key.state |= _cdk_quartz_events_get_current_mouse_modifiers ();

  /* The X11 backend adds the first virtual modifier MOD2..MOD5 are
   * mapped to. Since we only have one virtual modifier in the quartz
   * backend, calling the standard function will do.
   */
  cdk_keymap_add_virtual_modifiers (cdk_keymap_get_for_display (_cdk_display),
                                    &event->key.state);

  event->key.string = NULL;

  /* Fill in ->string since apps depend on it, taken from the x11 backend. */
  if (event->key.keyval != CDK_KEY_VoidSymbol)
    c = cdk_keyval_to_unicode (event->key.keyval);

  if (c)
    {
      gsize bytes_written;
      gint len;

      len = g_unichar_to_utf8 (c, buf);
      buf[len] = '\0';
      
      event->key.string = g_locale_from_utf8 (buf, len,
					      NULL, &bytes_written,
					      NULL);
      if (event->key.string)
	event->key.length = bytes_written;
    }
  else if (event->key.keyval == CDK_KEY_Escape)
    {
      event->key.length = 1;
      event->key.string = g_strdup ("\033");
    }
  else if (event->key.keyval == CDK_KEY_Return ||
	  event->key.keyval == CDK_KEY_KP_Enter)
    {
      event->key.length = 1;
      event->key.string = g_strdup ("\r");
    }

  if (!event->key.string)
    {
      event->key.length = 0;
      event->key.string = g_strdup ("");
    }

  CDK_NOTE(EVENTS,
    g_message ("key %s:\t\twindow: %p  key: %12s  %d",
	  type == CDK_KEY_PRESS ? "press" : "release",
	  event->key.window,
	  event->key.keyval ? cdk_keyval_name (event->key.keyval) : "(none)",
	  event->key.keyval));
}

static gboolean
synthesize_crossing_event (CdkWindow *window,
                           CdkEvent  *event,
                           NSEvent   *nsevent,
                           gint       x,
                           gint       y,
                           gint       x_root,
                           gint       y_root)
{
  switch ([nsevent type])
    {
    case CDK_QUARTZ_MOUSE_ENTERED:
      /* Enter events are considered always to be from another toplevel
       * window, this shouldn't negatively affect any app or ctk code,
       * and is the only way to make CtkMenu work. EEK EEK EEK.
       */
      if (!(window->event_mask & CDK_ENTER_NOTIFY_MASK))
        return FALSE;

      fill_crossing_event (window, event, nsevent,
                           x, y,
                           x_root, y_root,
                           CDK_ENTER_NOTIFY,
                           CDK_CROSSING_NORMAL,
                           CDK_NOTIFY_NONLINEAR);
      return TRUE;

    case CDK_QUARTZ_MOUSE_EXITED:
      /* See above */
      if (!(window->event_mask & CDK_LEAVE_NOTIFY_MASK))
        return FALSE;

      fill_crossing_event (window, event, nsevent,
                           x, y,
                           x_root, y_root,
                           CDK_LEAVE_NOTIFY,
                           CDK_CROSSING_NORMAL,
                           CDK_NOTIFY_NONLINEAR);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

void
_cdk_quartz_synthesize_null_key_event (CdkWindow *window)
{
  CdkEvent *event;
  CdkSeat *seat = cdk_display_get_default_seat (_cdk_display);

  event = cdk_event_new (CDK_KEY_PRESS);
  event->any.type = CDK_KEY_PRESS;
  event->key.window = window;
  event->key.state = 0;
  event->key.hardware_keycode = 0;
  event->key.group = 0;
  event->key.keyval = CDK_KEY_VoidSymbol;

  cdk_event_set_device (event, cdk_seat_get_keyboard (seat));
  cdk_event_set_seat (event, seat);
  append_event(event, FALSE);
}

CdkModifierType
_cdk_quartz_events_get_current_keyboard_modifiers (void)
{
  if (cdk_quartz_osx_version () >= CDK_OSX_SNOW_LEOPARD)
    {
      return get_keyboard_modifiers_from_ns_flags ([NSClassFromString(@"NSEvent") modifierFlags]);
    }
  else
    {
      guint carbon_modifiers = GetCurrentKeyModifiers ();
      CdkModifierType modifiers = 0;

      if (carbon_modifiers & alphaLock)
        modifiers |= CDK_LOCK_MASK;
      if (carbon_modifiers & shiftKey)
        modifiers |= CDK_SHIFT_MASK;
      if (carbon_modifiers & controlKey)
        modifiers |= CDK_CONTROL_MASK;
      if (carbon_modifiers & optionKey)
        modifiers |= CDK_MOD1_MASK;
      if (carbon_modifiers & cmdKey)
        modifiers |= CDK_MOD2_MASK;

      return modifiers;
    }
}

CdkModifierType
_cdk_quartz_events_get_current_mouse_modifiers (void)
{
  NSUInteger buttons = 0;
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
  if (cdk_quartz_osx_version () >= CDK_OSX_SNOW_LEOPARD)
    buttons = [NSClassFromString(@"NSEvent") pressedMouseButtons];
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
  else
#endif
#endif
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
    buttons = GetCurrentButtonState ();
#endif
  return get_mouse_button_modifiers_from_ns_buttons (buttons);
}

/* Detect window resizing */

static gboolean
test_resize (NSEvent *event, CdkWindow *toplevel, gint x, gint y)
{
  CdkWindowImplQuartz *toplevel_impl;
  gboolean lion;

  /* Resizing from the resize indicator only begins if an CDK_QUARTZ_LEFT_MOUSE_BUTTON
   * event is received in the resizing area.
   */
  toplevel_impl = (CdkWindowImplQuartz *)toplevel->impl;
  if ([toplevel_impl->toplevel showsResizeIndicator])
  if ([event type] == CDK_QUARTZ_LEFT_MOUSE_DOWN &&
      [toplevel_impl->toplevel showsResizeIndicator])
    {
      NSRect frame;

      /* If the resize indicator is visible and the event
       * is in the lower right 15x15 corner, we leave these
       * events to Cocoa as to be handled as resize events.
       * Applications may have widgets in this area.  These
       * will most likely be larger than 15x15 and for
       * scroll bars there are also other means to move
       * the scroll bar.  Since the resize indicator is
       * the only way of resizing windows on Mac OS, it
       * is too important to not make functional.
       */
      frame = [toplevel_impl->view bounds];
      if (x > frame.size.width - GRIP_WIDTH &&
          x < frame.size.width &&
          y > frame.size.height - GRIP_HEIGHT &&
          y < frame.size.height)
        return TRUE;
     }

  /* If we're on Lion and within 5 pixels of an edge,
   * then assume that the user wants to resize, and
   * return NULL to let Quartz get on with it. We check
   * the selector isRestorable to see if we're on 10.7.
   * This extra check is in case the user starts
   * dragging before CDK recognizes the grab.
   *
   * We perform this check for a button press of all buttons, because we
   * do receive, for instance, a right mouse down event for a CDK window
   * for x-coordinate range [-3, 0], but we do not want to forward this
   * into CDK. Forwarding such events into CDK will confuse the pointer
   * window finding code, because there are no CdkWindows present in
   * the range [-3, 0].
   */
  lion = cdk_quartz_osx_version () >= CDK_OSX_LION;
  if (lion &&
      ([event type] == CDK_QUARTZ_LEFT_MOUSE_DOWN ||
       [event type] == CDK_QUARTZ_RIGHT_MOUSE_DOWN ||
       [event type] == CDK_QUARTZ_OTHER_MOUSE_DOWN))
    {
      if (x < CDK_LION_RESIZE ||
          x > toplevel->width - CDK_LION_RESIZE ||
          y > toplevel->height - CDK_LION_RESIZE)
        return TRUE;
    }

  return FALSE;
}

#if MAC_OS_X_VERSION_MIN_REQUIRED < 101200
#define CDK_QUARTZ_APP_KIT_DEFINED NSAppKitDefined
#define CDK_QUARTZ_APPLICATION_DEACTIVATED NSApplicationDeactivatedEventType
#else
#define CDK_QUARTZ_APP_KIT_DEFINED NSEventTypeAppKitDefined
#define CDK_QUARTZ_APPLICATION_DEACTIVATED NSEventSubtypeApplicationDeactivated
#endif

static gboolean
cdk_event_translate (CdkEvent *event,
                     NSEvent  *nsevent)
{
  NSEventType event_type;
  NSWindow *nswindow;
  CdkWindow *window;
  int x, y;
  int x_root, y_root;
  gboolean return_val;

  /* There is no support for real desktop wide grabs, so we break
   * grabs when the application loses focus (gets deactivated).
   */
  event_type = [nsevent type];
  if (event_type == CDK_QUARTZ_APP_KIT_DEFINED)
    {
      if ([nsevent subtype] ==  CDK_QUARTZ_APPLICATION_DEACTIVATED)
        _cdk_quartz_events_break_all_grabs (get_time_from_ns_event (nsevent));

      /* This could potentially be used to break grabs when clicking
       * on the title. The subtype 20 is undocumented so it's probably
       * not a good idea: else if (subtype == 20) break_all_grabs ();
       */

      /* Leave all AppKit events to AppKit. */
      return FALSE;
    }

  if (_cdk_default_filters)
    {
      /* Apply global filters */
      CdkFilterReturn result;

      result = cdk_event_apply_filters (nsevent, event, &_cdk_default_filters);
      if (result != CDK_FILTER_CONTINUE)
        {
          return_val = (result == CDK_FILTER_TRANSLATE) ? TRUE : FALSE;
          goto done;
        }
    }

  /* We need to register the proximity event from any point on the screen
   * to properly register the devices
   */
  if (event_type == CDK_QUARTZ_EVENT_TABLET_PROXIMITY)
    {
      _cdk_quartz_device_manager_register_device_for_ns_event (cdk_display_get_device_manager (_cdk_display),
                                                               nsevent);
    }

  nswindow = [nsevent window];

  /* Ignore events for windows not created by CDK. */
  if (nswindow && ![[nswindow contentView] isKindOfClass:[CdkQuartzView class]])
    return FALSE;

  /* Ignore events for ones with no windows */
  if (!nswindow)
    {
      CdkWindow *toplevel = NULL;

      if (event_type == CDK_QUARTZ_MOUSE_MOVED)
        {
          /* Motion events received after clicking the menu bar do not have the
           * window field set.  Instead of giving up on the event immediately,
           * we first check whether this event is within our window bounds.
           */
          NSPoint screen_point = [NSEvent mouseLocation];
          gint x_tmp, y_tmp;

          toplevel = find_toplevel_under_pointer (_cdk_display,
                                                  screen_point,
                                                  &x_tmp, &y_tmp);
        }

      if (!toplevel)
        return FALSE;
    }

  /* Ignore events and break grabs while the window is being
   * dragged. This is a workaround for the window getting events for
   * the window title.
   */
  if ([(CdkQuartzNSWindow *)nswindow isInMove])
    {
      _cdk_quartz_events_break_all_grabs (get_time_from_ns_event (nsevent));
      return FALSE;
    }

  /* Also when in a manual resize or move , we ignore events so that
   * these are pushed to CdkQuartzNSWindow's sendEvent handler.
   */
  if ([(CdkQuartzNSWindow *)nswindow isInManualResizeOrMove])
    return FALSE;

  /* Find the right CDK window to send the event to, taking grabs and
   * event masks into consideration.
   */
  window = find_window_for_ns_event (nsevent, &x, &y, &x_root, &y_root);
  if (!window)
    return FALSE;

  /* Quartz handles resizing on its own, so we want to stay out of the way. */
  if (test_resize (nsevent, window, x, y))
    return FALSE;

  /* Apply any window filters. */
  if (CDK_IS_WINDOW (window))
    {
      CdkFilterReturn result;

      if (window->filters)
	{
	  g_object_ref (window);

	  result = cdk_event_apply_filters (nsevent, event, &window->filters);

	  g_object_unref (window);

	  if (result != CDK_FILTER_CONTINUE)
	    {
	      return_val = (result == CDK_FILTER_TRANSLATE) ? TRUE : FALSE;
	      goto done;
	    }
	}
    }

  /* If the app is not active leave the event to AppKit so the window gets
   * focused correctly and don't do click-through (so we behave like most
   * native apps). If the app is active, we focus the window and then handle
   * the event, also to match native apps.
   */
  if ((event_type == CDK_QUARTZ_RIGHT_MOUSE_DOWN ||
       event_type == CDK_QUARTZ_OTHER_MOUSE_DOWN ||
       event_type == CDK_QUARTZ_LEFT_MOUSE_DOWN))
    {
      CdkWindowImplQuartz *impl = CDK_WINDOW_IMPL_QUARTZ (window->impl);

      if (![NSApp isActive])
        {
          [NSApp activateIgnoringOtherApps:YES];
          return FALSE;
        }
      else if (![impl->toplevel isKeyWindow])
        {
          CdkDeviceGrabInfo *grab;
          CdkSeat *seat = cdk_display_get_default_seat (_cdk_display);

          grab = _cdk_display_get_last_device_grab (_cdk_display,
                                                    cdk_seat_get_pointer (seat));
          if (!grab)
            [impl->toplevel makeKeyWindow];

        }
    }

  return_val = TRUE;

  switch (event_type)
    {
    case CDK_QUARTZ_LEFT_MOUSE_DOWN:
    case CDK_QUARTZ_RIGHT_MOUSE_DOWN:
    case CDK_QUARTZ_OTHER_MOUSE_DOWN:
    case CDK_QUARTZ_LEFT_MOUSE_UP:
    case CDK_QUARTZ_RIGHT_MOUSE_UP:
    case CDK_QUARTZ_OTHER_MOUSE_UP:
      fill_button_event (window, event, nsevent, x, y, x_root, y_root);
      break;

    case CDK_QUARTZ_LEFT_MOUSE_DRAGGED:
    case CDK_QUARTZ_RIGHT_MOUSE_DRAGGED:
    case CDK_QUARTZ_OTHER_MOUSE_DRAGGED:
    case CDK_QUARTZ_MOUSE_MOVED:
      fill_motion_event (window, event, nsevent, x, y, x_root, y_root);
      break;

    case CDK_QUARTZ_SCROLL_WHEEL:
      {
        CdkScrollDirection direction;
	float dx;
	float dy;
#ifdef AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
	if (cdk_quartz_osx_version() >= CDK_OSX_LION &&
	    [nsevent hasPreciseScrollingDeltas])
	  {
	    dx = [nsevent scrollingDeltaX];
	    dy = [nsevent scrollingDeltaY];
            direction = CDK_SCROLL_SMOOTH;

            fill_scroll_event (window, event, nsevent, x, y, x_root, y_root,
                               -dx, -dy, direction);

            /* Fall through for scroll buttons emulation */
	  }
#endif
        dx = [nsevent deltaX];
        dy = [nsevent deltaY];

        if (dy != 0.0)
          {
            if (dy < 0.0)
              direction = CDK_SCROLL_DOWN;
            else
              direction = CDK_SCROLL_UP;

            dy = fabs (dy);
            dx = 0.0;
          }
        else if (dx != 0.0)
          {
            if (dx < 0.0)
              direction = CDK_SCROLL_RIGHT;
            else
              direction = CDK_SCROLL_LEFT;

            dx = fabs (dx);
            dy = 0.0;
          }

        if (dx != 0.0 || dy != 0.0)
          {
#ifdef AVAILABLE_MAC_OS_X_VERSION_10_7_AND_LATER
	    if (cdk_quartz_osx_version() >= CDK_OSX_LION &&
		[nsevent hasPreciseScrollingDeltas])
              {
                CdkEvent *emulated_event;

                emulated_event = cdk_event_new (CDK_SCROLL);
                cdk_event_set_pointer_emulated (emulated_event, TRUE);
                fill_scroll_event (window, emulated_event, nsevent,
                                   x, y, x_root, y_root,
                                   dx, dy, direction);
                append_event (emulated_event, TRUE);
              }
            else
#endif
              fill_scroll_event (window, event, nsevent,
                                 x, y, x_root, y_root,
                                 dx, dy, direction);
          }
      }
      break;
#ifdef AVAILABLE_MAC_OS_X_VERSION_10_8_AND_LATER
    case NSEventTypeMagnify:
    case NSEventTypeRotate:
      /* Event handling requires [NSEvent phase] which was introduced in 10.7 */
      /* However - Tests on 10.7 showed that phase property does not work     */
      if (cdk_quartz_osx_version () >= CDK_OSX_MOUNTAIN_LION)
        fill_pinch_event (window, event, nsevent, x, y, x_root, y_root);
      else
        return_val = FALSE;
      break;
#endif
    case CDK_QUARTZ_MOUSE_EXITED:
      if (WINDOW_IS_TOPLEVEL (window))
          [[NSCursor arrowCursor] set];
      /* fall through */
    case CDK_QUARTZ_MOUSE_ENTERED:
      return_val = synthesize_crossing_event (window, event, nsevent, x, y, x_root, y_root);
      break;

    case CDK_QUARTZ_KEY_DOWN:
    case CDK_QUARTZ_KEY_UP:
    case CDK_QUARTZ_FLAGS_CHANGED:
      {
        CdkEventType type;

        type = _cdk_quartz_keys_event_type (nsevent);
        if (type == CDK_NOTHING)
          return_val = FALSE;
        else
          fill_key_event (window, event, nsevent, type);
      }
      break;

    default:
      /* Ignore everything elsee. */
      return_val = FALSE;
      break;
    }

 done:
  if (return_val)
    {
      if (event->any.window)
	g_object_ref (event->any.window);
      if (((event->any.type == CDK_ENTER_NOTIFY) ||
	   (event->any.type == CDK_LEAVE_NOTIFY)) &&
	  (event->crossing.subwindow != NULL))
	g_object_ref (event->crossing.subwindow);
    }
  else
    {
      /* Mark this event as having no resources to be freed */
      event->any.window = NULL;
      event->any.type = CDK_NOTHING;
    }

  return return_val;
}

void
_cdk_quartz_display_queue_events (CdkDisplay *display)
{  
  NSEvent *nsevent;

  nsevent = _cdk_quartz_event_loop_get_pending ();
  if (nsevent)
    {
      CdkEvent *event;
      GList *node;

      event = cdk_event_new (CDK_NOTHING);

      event->any.window = NULL;
      event->any.send_event = FALSE;

      ((CdkEventPrivate *)event)->flags |= CDK_EVENT_PENDING;

      node = _cdk_event_queue_append (display, event);

      if (cdk_event_translate (event, nsevent))
        {
	  ((CdkEventPrivate *)event)->flags &= ~CDK_EVENT_PENDING;
          _cdk_windowing_got_event (display, node, event, 0);
        }
      else
        {
	  _cdk_event_queue_remove_link (display, node);
	  g_list_free_1 (node);
	  cdk_event_free (event);

          cdk_threads_leave ();
          [NSApp sendEvent:nsevent];
          cdk_threads_enter ();
        }

      _cdk_quartz_event_loop_release_event (nsevent);
    }
}

void
_cdk_quartz_screen_broadcast_client_message (CdkScreen *screen,
                                             CdkEvent  *event)
{
  /* Not supported. */
}

gboolean
_cdk_quartz_screen_get_setting (CdkScreen   *screen,
                                const gchar *name,
                                GValue      *value)
{
  if (strcmp (name, "ctk-double-click-time") == 0)
    {
      NSUserDefaults *defaults;
      float t;

      CDK_QUARTZ_ALLOC_POOL;

      defaults = [NSUserDefaults standardUserDefaults];
            
      t = [defaults floatForKey:@"com.apple.mouse.doubleClickThreshold"];
      if (t == 0.0)
	{
	  /* No user setting, use the default in OS X. */
	  t = 0.5;
	}

      CDK_QUARTZ_RELEASE_POOL;

      g_value_set_int (value, t * 1000);

      return TRUE;
    }
  else if (strcmp (name, "ctk-font-name") == 0)
    {
      NSString *name;
      char *str;
      gint size;

      CDK_QUARTZ_ALLOC_POOL;

      name = [[NSFont systemFontOfSize:0] familyName];
      size = (gint)[[NSFont userFontOfSize:0] pointSize];

      /* Let's try to use the "views" font size (12pt) by default. This is
       * used for lists/text/other "content" which is the largest parts of
       * apps, using the "regular control" size (13pt) looks a bit out of
       * place. We might have to tweak this.
       */

      /* The size has to be hardcoded as there doesn't seem to be a way to
       * get the views font size programmatically.
       */
      str = g_strdup_printf ("%s %d", [name UTF8String], size);
      g_value_set_string (value, str);
      g_free (str);

      CDK_QUARTZ_RELEASE_POOL;

      return TRUE;
    }
  else if (strcmp (name, "ctk-primary-button-warps-slider") == 0)
    {
      CDK_QUARTZ_ALLOC_POOL;

      BOOL setting = [[NSUserDefaults standardUserDefaults] boolForKey:@"AppleScrollerPagingBehavior"];

      /* If the Apple property is YES, it means "warp" */
      g_value_set_boolean (value, setting == YES);

      CDK_QUARTZ_RELEASE_POOL;

      return TRUE;
    }
  else if (strcmp (name, "ctk-shell-shows-desktop") == 0)
    {
      CDK_QUARTZ_ALLOC_POOL;

      g_value_set_boolean (value, TRUE);

      CDK_QUARTZ_RELEASE_POOL;

      return TRUE;
    }
  
  /* FIXME: Add more settings */

  return FALSE;
}

void
_cdk_quartz_display_event_data_copy (CdkDisplay     *display,
                                     const CdkEvent *src,
                                     CdkEvent       *dst)
{
  CdkEventPrivate *priv_src = (CdkEventPrivate *) src;
  CdkEventPrivate *priv_dst = (CdkEventPrivate *) dst;

  if (priv_src->windowing_data)
    {
      priv_dst->windowing_data = priv_src->windowing_data;
      [(NSEvent *)priv_dst->windowing_data retain];
    }
}

void
_cdk_quartz_display_event_data_free (CdkDisplay *display,
                                     CdkEvent   *event)
{
  CdkEventPrivate *priv = (CdkEventPrivate *) event;

  if (priv->windowing_data)
    {
      [(NSEvent *)priv->windowing_data release];
      priv->windowing_data = NULL;
    }
}
