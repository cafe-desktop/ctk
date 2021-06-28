/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __GDK_EVENTS_H__
#define __GDK_EVENTS_H__

#if !defined (__GDK_H_INSIDE__) && !defined (GDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdkversionmacros.h>
#include <cdk/cdktypes.h>
#include <cdk/cdkdnd.h>
#include <cdk/cdkdevice.h>
#include <cdk/cdkdevicetool.h>

G_BEGIN_DECLS


/**
 * SECTION:event_structs
 * @Short_description: Data structures specific to each type of event
 * @Title: Event Structures
 *
 * The event structures contain data specific to each type of event in GDK.
 *
 * > A common mistake is to forget to set the event mask of a widget so that
 * > the required events are received. See ctk_widget_set_events().
 */


#define GDK_TYPE_EVENT          (cdk_event_get_type ())
#define GDK_TYPE_EVENT_SEQUENCE (cdk_event_sequence_get_type ())

/**
 * GDK_PRIORITY_EVENTS:
 *
 * This is the priority that events from the X server are given in the
 * [GLib Main Loop][glib-The-Main-Event-Loop].
 */
#define GDK_PRIORITY_EVENTS	(G_PRIORITY_DEFAULT)

/**
 * GDK_PRIORITY_REDRAW: (value 120)
 *
 * This is the priority that the idle handler processing window updates
 * is given in the
 * [GLib Main Loop][glib-The-Main-Event-Loop].
 */
#define GDK_PRIORITY_REDRAW     (G_PRIORITY_HIGH_IDLE + 20)

/**
 * GDK_EVENT_PROPAGATE:
 *
 * Use this macro as the return value for continuing the propagation of
 * an event handler.
 *
 * Since: 3.4
 */
#define GDK_EVENT_PROPAGATE     (FALSE)

/**
 * GDK_EVENT_STOP:
 *
 * Use this macro as the return value for stopping the propagation of
 * an event handler.
 *
 * Since: 3.4
 */
#define GDK_EVENT_STOP          (TRUE)

/**
 * GDK_BUTTON_PRIMARY:
 *
 * The primary button. This is typically the left mouse button, or the
 * right button in a left-handed setup.
 *
 * Since: 3.4
 */
#define GDK_BUTTON_PRIMARY      (1)

/**
 * GDK_BUTTON_MIDDLE:
 *
 * The middle button.
 *
 * Since: 3.4
 */
#define GDK_BUTTON_MIDDLE       (2)

/**
 * GDK_BUTTON_SECONDARY:
 *
 * The secondary button. This is typically the right mouse button, or the
 * left button in a left-handed setup.
 *
 * Since: 3.4
 */
#define GDK_BUTTON_SECONDARY    (3)



typedef struct _CdkEventAny	    CdkEventAny;
typedef struct _CdkEventExpose	    CdkEventExpose;
typedef struct _CdkEventVisibility  CdkEventVisibility;
typedef struct _CdkEventMotion	    CdkEventMotion;
typedef struct _CdkEventButton	    CdkEventButton;
typedef struct _CdkEventTouch       CdkEventTouch;
typedef struct _CdkEventScroll      CdkEventScroll;  
typedef struct _CdkEventKey	    CdkEventKey;
typedef struct _CdkEventFocus	    CdkEventFocus;
typedef struct _CdkEventCrossing    CdkEventCrossing;
typedef struct _CdkEventConfigure   CdkEventConfigure;
typedef struct _CdkEventProperty    CdkEventProperty;
typedef struct _CdkEventSelection   CdkEventSelection;
typedef struct _CdkEventOwnerChange CdkEventOwnerChange;
typedef struct _CdkEventProximity   CdkEventProximity;
typedef struct _CdkEventDND         CdkEventDND;
typedef struct _CdkEventWindowState CdkEventWindowState;
typedef struct _CdkEventSetting     CdkEventSetting;
typedef struct _CdkEventGrabBroken  CdkEventGrabBroken;
typedef struct _CdkEventTouchpadSwipe CdkEventTouchpadSwipe;
typedef struct _CdkEventTouchpadPinch CdkEventTouchpadPinch;
typedef struct _CdkEventPadButton   CdkEventPadButton;
typedef struct _CdkEventPadAxis     CdkEventPadAxis;
typedef struct _CdkEventPadGroupMode CdkEventPadGroupMode;

typedef struct _CdkEventSequence    CdkEventSequence;

typedef union  _CdkEvent	    CdkEvent;

/**
 * CdkEventFunc:
 * @event: the #CdkEvent to process.
 * @data: (closure): user data set when the event handler was installed with
 *   cdk_event_handler_set().
 *
 * Specifies the type of function passed to cdk_event_handler_set() to
 * handle all GDK events.
 */
typedef void (*CdkEventFunc) (CdkEvent *event,
			      gpointer	data);

/* Event filtering */

/**
 * CdkXEvent:
 *
 * Used to represent native events (XEvents for the X11
 * backend, MSGs for Win32).
 */
typedef void CdkXEvent;	  /* Can be cast to window system specific
			   * even type, XEvent on X11, MSG on Win32.
			   */

/**
 * CdkFilterReturn:
 * @GDK_FILTER_CONTINUE: event not handled, continue processing.
 * @GDK_FILTER_TRANSLATE: native event translated into a GDK event and stored
 *  in the `event` structure that was passed in.
 * @GDK_FILTER_REMOVE: event handled, terminate processing.
 *
 * Specifies the result of applying a #CdkFilterFunc to a native event.
 */
typedef enum {
  GDK_FILTER_CONTINUE,	  /* Event not handled, continue processesing */
  GDK_FILTER_TRANSLATE,	  /* Native event translated into a GDK event and
                             stored in the "event" structure that was
                             passed in */
  GDK_FILTER_REMOVE	  /* Terminate processing, removing event */
} CdkFilterReturn;

/**
 * CdkFilterFunc:
 * @xevent: the native event to filter.
 * @event: the GDK event to which the X event will be translated.
 * @data: (closure): user data set when the filter was installed.
 *
 * Specifies the type of function used to filter native events before they are
 * converted to GDK events.
 *
 * When a filter is called, @event is unpopulated, except for
 * `event->window`. The filter may translate the native
 * event to a GDK event and store the result in @event, or handle it without
 * translation. If the filter translates the event and processing should
 * continue, it should return %GDK_FILTER_TRANSLATE.
 *
 * Returns: a #CdkFilterReturn value.
 */
typedef CdkFilterReturn (*CdkFilterFunc) (CdkXEvent *xevent,
					  CdkEvent *event,
					  gpointer  data);


/**
 * CdkEventType:
 * @GDK_NOTHING: a special code to indicate a null event.
 * @GDK_DELETE: the window manager has requested that the toplevel window be
 *   hidden or destroyed, usually when the user clicks on a special icon in the
 *   title bar.
 * @GDK_DESTROY: the window has been destroyed.
 * @GDK_EXPOSE: all or part of the window has become visible and needs to be
 *   redrawn.
 * @GDK_MOTION_NOTIFY: the pointer (usually a mouse) has moved.
 * @GDK_BUTTON_PRESS: a mouse button has been pressed.
 * @GDK_2BUTTON_PRESS: a mouse button has been double-clicked (clicked twice
 *   within a short period of time). Note that each click also generates a
 *   %GDK_BUTTON_PRESS event.
 * @GDK_DOUBLE_BUTTON_PRESS: alias for %GDK_2BUTTON_PRESS, added in 3.6.
 * @GDK_3BUTTON_PRESS: a mouse button has been clicked 3 times in a short period
 *   of time. Note that each click also generates a %GDK_BUTTON_PRESS event.
 * @GDK_TRIPLE_BUTTON_PRESS: alias for %GDK_3BUTTON_PRESS, added in 3.6.
 * @GDK_BUTTON_RELEASE: a mouse button has been released.
 * @GDK_KEY_PRESS: a key has been pressed.
 * @GDK_KEY_RELEASE: a key has been released.
 * @GDK_ENTER_NOTIFY: the pointer has entered the window.
 * @GDK_LEAVE_NOTIFY: the pointer has left the window.
 * @GDK_FOCUS_CHANGE: the keyboard focus has entered or left the window.
 * @GDK_CONFIGURE: the size, position or stacking order of the window has changed.
 *   Note that CTK+ discards these events for %GDK_WINDOW_CHILD windows.
 * @GDK_MAP: the window has been mapped.
 * @GDK_UNMAP: the window has been unmapped.
 * @GDK_PROPERTY_NOTIFY: a property on the window has been changed or deleted.
 * @GDK_SELECTION_CLEAR: the application has lost ownership of a selection.
 * @GDK_SELECTION_REQUEST: another application has requested a selection.
 * @GDK_SELECTION_NOTIFY: a selection has been received.
 * @GDK_PROXIMITY_IN: an input device has moved into contact with a sensing
 *   surface (e.g. a touchscreen or graphics tablet).
 * @GDK_PROXIMITY_OUT: an input device has moved out of contact with a sensing
 *   surface.
 * @GDK_DRAG_ENTER: the mouse has entered the window while a drag is in progress.
 * @GDK_DRAG_LEAVE: the mouse has left the window while a drag is in progress.
 * @GDK_DRAG_MOTION: the mouse has moved in the window while a drag is in
 *   progress.
 * @GDK_DRAG_STATUS: the status of the drag operation initiated by the window
 *   has changed.
 * @GDK_DROP_START: a drop operation onto the window has started.
 * @GDK_DROP_FINISHED: the drop operation initiated by the window has completed.
 * @GDK_CLIENT_EVENT: a message has been received from another application.
 * @GDK_VISIBILITY_NOTIFY: the window visibility status has changed.
 * @GDK_SCROLL: the scroll wheel was turned
 * @GDK_WINDOW_STATE: the state of a window has changed. See #CdkWindowState
 *   for the possible window states
 * @GDK_SETTING: a setting has been modified.
 * @GDK_OWNER_CHANGE: the owner of a selection has changed. This event type
 *   was added in 2.6
 * @GDK_GRAB_BROKEN: a pointer or keyboard grab was broken. This event type
 *   was added in 2.8.
 * @GDK_DAMAGE: the content of the window has been changed. This event type
 *   was added in 2.14.
 * @GDK_TOUCH_BEGIN: A new touch event sequence has just started. This event
 *   type was added in 3.4.
 * @GDK_TOUCH_UPDATE: A touch event sequence has been updated. This event type
 *   was added in 3.4.
 * @GDK_TOUCH_END: A touch event sequence has finished. This event type
 *   was added in 3.4.
 * @GDK_TOUCH_CANCEL: A touch event sequence has been canceled. This event type
 *   was added in 3.4.
 * @GDK_TOUCHPAD_SWIPE: A touchpad swipe gesture event, the current state
 *   is determined by its phase field. This event type was added in 3.18.
 * @GDK_TOUCHPAD_PINCH: A touchpad pinch gesture event, the current state
 *   is determined by its phase field. This event type was added in 3.18.
 * @GDK_PAD_BUTTON_PRESS: A tablet pad button press event. This event type
 *   was added in 3.22.
 * @GDK_PAD_BUTTON_RELEASE: A tablet pad button release event. This event type
 *   was added in 3.22.
 * @GDK_PAD_RING: A tablet pad axis event from a "ring". This event type was
 *   added in 3.22.
 * @GDK_PAD_STRIP: A tablet pad axis event from a "strip". This event type was
 *   added in 3.22.
 * @GDK_PAD_GROUP_MODE: A tablet pad group mode change. This event type was
 *   added in 3.22.
 * @GDK_EVENT_LAST: marks the end of the CdkEventType enumeration. Added in 2.18
 *
 * Specifies the type of the event.
 *
 * Do not confuse these events with the signals that CTK+ widgets emit.
 * Although many of these events result in corresponding signals being emitted,
 * the events are often transformed or filtered along the way.
 *
 * In some language bindings, the values %GDK_2BUTTON_PRESS and
 * %GDK_3BUTTON_PRESS would translate into something syntactically
 * invalid (eg `Cdk.EventType.2ButtonPress`, where a
 * symbol is not allowed to start with a number). In that case, the
 * aliases %GDK_DOUBLE_BUTTON_PRESS and %GDK_TRIPLE_BUTTON_PRESS can
 * be used instead.
 */
typedef enum
{
  GDK_NOTHING		= -1,
  GDK_DELETE		= 0,
  GDK_DESTROY		= 1,
  GDK_EXPOSE		= 2,
  GDK_MOTION_NOTIFY	= 3,
  GDK_BUTTON_PRESS	= 4,
  GDK_2BUTTON_PRESS	= 5,
  GDK_DOUBLE_BUTTON_PRESS = GDK_2BUTTON_PRESS,
  GDK_3BUTTON_PRESS	= 6,
  GDK_TRIPLE_BUTTON_PRESS = GDK_3BUTTON_PRESS,
  GDK_BUTTON_RELEASE	= 7,
  GDK_KEY_PRESS		= 8,
  GDK_KEY_RELEASE	= 9,
  GDK_ENTER_NOTIFY	= 10,
  GDK_LEAVE_NOTIFY	= 11,
  GDK_FOCUS_CHANGE	= 12,
  GDK_CONFIGURE		= 13,
  GDK_MAP		= 14,
  GDK_UNMAP		= 15,
  GDK_PROPERTY_NOTIFY	= 16,
  GDK_SELECTION_CLEAR	= 17,
  GDK_SELECTION_REQUEST = 18,
  GDK_SELECTION_NOTIFY	= 19,
  GDK_PROXIMITY_IN	= 20,
  GDK_PROXIMITY_OUT	= 21,
  GDK_DRAG_ENTER        = 22,
  GDK_DRAG_LEAVE        = 23,
  GDK_DRAG_MOTION       = 24,
  GDK_DRAG_STATUS       = 25,
  GDK_DROP_START        = 26,
  GDK_DROP_FINISHED     = 27,
  GDK_CLIENT_EVENT	= 28,
  GDK_VISIBILITY_NOTIFY = 29,
  GDK_SCROLL            = 31,
  GDK_WINDOW_STATE      = 32,
  GDK_SETTING           = 33,
  GDK_OWNER_CHANGE      = 34,
  GDK_GRAB_BROKEN       = 35,
  GDK_DAMAGE            = 36,
  GDK_TOUCH_BEGIN       = 37,
  GDK_TOUCH_UPDATE      = 38,
  GDK_TOUCH_END         = 39,
  GDK_TOUCH_CANCEL      = 40,
  GDK_TOUCHPAD_SWIPE    = 41,
  GDK_TOUCHPAD_PINCH    = 42,
  GDK_PAD_BUTTON_PRESS  = 43,
  GDK_PAD_BUTTON_RELEASE = 44,
  GDK_PAD_RING          = 45,
  GDK_PAD_STRIP         = 46,
  GDK_PAD_GROUP_MODE    = 47,
  GDK_EVENT_LAST        /* helper variable for decls */
} CdkEventType;

/**
 * CdkVisibilityState:
 * @GDK_VISIBILITY_UNOBSCURED: the window is completely visible.
 * @GDK_VISIBILITY_PARTIAL: the window is partially visible.
 * @GDK_VISIBILITY_FULLY_OBSCURED: the window is not visible at all.
 *
 * Specifies the visiblity status of a window for a #CdkEventVisibility.
 */
typedef enum
{
  GDK_VISIBILITY_UNOBSCURED,
  GDK_VISIBILITY_PARTIAL,
  GDK_VISIBILITY_FULLY_OBSCURED
} CdkVisibilityState;

/**
 * CdkTouchpadGesturePhase:
 * @GDK_TOUCHPAD_GESTURE_PHASE_BEGIN: The gesture has begun.
 * @GDK_TOUCHPAD_GESTURE_PHASE_UPDATE: The gesture has been updated.
 * @GDK_TOUCHPAD_GESTURE_PHASE_END: The gesture was finished, changes
 *   should be permanently applied.
 * @GDK_TOUCHPAD_GESTURE_PHASE_CANCEL: The gesture was cancelled, all
 *   changes should be undone.
 *
 * Specifies the current state of a touchpad gesture. All gestures are
 * guaranteed to begin with an event with phase %GDK_TOUCHPAD_GESTURE_PHASE_BEGIN,
 * followed by 0 or several events with phase %GDK_TOUCHPAD_GESTURE_PHASE_UPDATE.
 *
 * A finished gesture may have 2 possible outcomes, an event with phase
 * %GDK_TOUCHPAD_GESTURE_PHASE_END will be emitted when the gesture is
 * considered successful, this should be used as the hint to perform any
 * permanent changes.

 * Cancelled gestures may be so for a variety of reasons, due to hardware
 * or the compositor, or due to the gesture recognition layers hinting the
 * gesture did not finish resolutely (eg. a 3rd finger being added during
 * a pinch gesture). In these cases, the last event will report the phase
 * %GDK_TOUCHPAD_GESTURE_PHASE_CANCEL, this should be used as a hint
 * to undo any visible/permanent changes that were done throughout the
 * progress of the gesture.
 *
 * See also #CdkEventTouchpadSwipe and #CdkEventTouchpadPinch.
 *
 */
typedef enum
{
  GDK_TOUCHPAD_GESTURE_PHASE_BEGIN,
  GDK_TOUCHPAD_GESTURE_PHASE_UPDATE,
  GDK_TOUCHPAD_GESTURE_PHASE_END,
  GDK_TOUCHPAD_GESTURE_PHASE_CANCEL
} CdkTouchpadGesturePhase;

/**
 * CdkScrollDirection:
 * @GDK_SCROLL_UP: the window is scrolled up.
 * @GDK_SCROLL_DOWN: the window is scrolled down.
 * @GDK_SCROLL_LEFT: the window is scrolled to the left.
 * @GDK_SCROLL_RIGHT: the window is scrolled to the right.
 * @GDK_SCROLL_SMOOTH: the scrolling is determined by the delta values
 *   in #CdkEventScroll. See cdk_event_get_scroll_deltas(). Since: 3.4
 *
 * Specifies the direction for #CdkEventScroll.
 */
typedef enum
{
  GDK_SCROLL_UP,
  GDK_SCROLL_DOWN,
  GDK_SCROLL_LEFT,
  GDK_SCROLL_RIGHT,
  GDK_SCROLL_SMOOTH
} CdkScrollDirection;

/**
 * CdkNotifyType:
 * @GDK_NOTIFY_ANCESTOR: the window is entered from an ancestor or
 *   left towards an ancestor.
 * @GDK_NOTIFY_VIRTUAL: the pointer moves between an ancestor and an
 *   inferior of the window.
 * @GDK_NOTIFY_INFERIOR: the window is entered from an inferior or
 *   left towards an inferior.
 * @GDK_NOTIFY_NONLINEAR: the window is entered from or left towards
 *   a window which is neither an ancestor nor an inferior.
 * @GDK_NOTIFY_NONLINEAR_VIRTUAL: the pointer moves between two windows
 *   which are not ancestors of each other and the window is part of
 *   the ancestor chain between one of these windows and their least
 *   common ancestor.
 * @GDK_NOTIFY_UNKNOWN: an unknown type of enter/leave event occurred.
 *
 * Specifies the kind of crossing for #CdkEventCrossing.
 *
 * See the X11 protocol specification of LeaveNotify for
 * full details of crossing event generation.
 */
typedef enum
{
  GDK_NOTIFY_ANCESTOR		= 0,
  GDK_NOTIFY_VIRTUAL		= 1,
  GDK_NOTIFY_INFERIOR		= 2,
  GDK_NOTIFY_NONLINEAR		= 3,
  GDK_NOTIFY_NONLINEAR_VIRTUAL	= 4,
  GDK_NOTIFY_UNKNOWN		= 5
} CdkNotifyType;

/**
 * CdkCrossingMode:
 * @GDK_CROSSING_NORMAL: crossing because of pointer motion.
 * @GDK_CROSSING_GRAB: crossing because a grab is activated.
 * @GDK_CROSSING_UNGRAB: crossing because a grab is deactivated.
 * @GDK_CROSSING_CTK_GRAB: crossing because a CTK+ grab is activated.
 * @GDK_CROSSING_CTK_UNGRAB: crossing because a CTK+ grab is deactivated.
 * @GDK_CROSSING_STATE_CHANGED: crossing because a CTK+ widget changed
 *   state (e.g. sensitivity).
 * @GDK_CROSSING_TOUCH_BEGIN: crossing because a touch sequence has begun,
 *   this event is synthetic as the pointer might have not left the window.
 * @GDK_CROSSING_TOUCH_END: crossing because a touch sequence has ended,
 *   this event is synthetic as the pointer might have not left the window.
 * @GDK_CROSSING_DEVICE_SWITCH: crossing because of a device switch (i.e.
 *   a mouse taking control of the pointer after a touch device), this event
 *   is synthetic as the pointer didn’t leave the window.
 *
 * Specifies the crossing mode for #CdkEventCrossing.
 */
typedef enum
{
  GDK_CROSSING_NORMAL,
  GDK_CROSSING_GRAB,
  GDK_CROSSING_UNGRAB,
  GDK_CROSSING_CTK_GRAB,
  GDK_CROSSING_CTK_UNGRAB,
  GDK_CROSSING_STATE_CHANGED,
  GDK_CROSSING_TOUCH_BEGIN,
  GDK_CROSSING_TOUCH_END,
  GDK_CROSSING_DEVICE_SWITCH
} CdkCrossingMode;

/**
 * CdkPropertyState:
 * @GDK_PROPERTY_NEW_VALUE: the property value was changed.
 * @GDK_PROPERTY_DELETE: the property was deleted.
 *
 * Specifies the type of a property change for a #CdkEventProperty.
 */
typedef enum
{
  GDK_PROPERTY_NEW_VALUE,
  GDK_PROPERTY_DELETE
} CdkPropertyState;

/**
 * CdkWindowState:
 * @GDK_WINDOW_STATE_WITHDRAWN: the window is not shown.
 * @GDK_WINDOW_STATE_ICONIFIED: the window is minimized.
 * @GDK_WINDOW_STATE_MAXIMIZED: the window is maximized.
 * @GDK_WINDOW_STATE_STICKY: the window is sticky.
 * @GDK_WINDOW_STATE_FULLSCREEN: the window is maximized without
 *   decorations.
 * @GDK_WINDOW_STATE_ABOVE: the window is kept above other windows.
 * @GDK_WINDOW_STATE_BELOW: the window is kept below other windows.
 * @GDK_WINDOW_STATE_FOCUSED: the window is presented as focused (with active decorations).
 * @GDK_WINDOW_STATE_TILED: the window is in a tiled state, Since 3.10. Since 3.22.23, this
 *                          is deprecated in favor of per-edge information.
 * @GDK_WINDOW_STATE_TOP_TILED: whether the top edge is tiled, Since 3.22.23
 * @GDK_WINDOW_STATE_TOP_RESIZABLE: whether the top edge is resizable, Since 3.22.23
 * @GDK_WINDOW_STATE_RIGHT_TILED: whether the right edge is tiled, Since 3.22.23
 * @GDK_WINDOW_STATE_RIGHT_RESIZABLE: whether the right edge is resizable, Since 3.22.23
 * @GDK_WINDOW_STATE_BOTTOM_TILED: whether the bottom edge is tiled, Since 3.22.23
 * @GDK_WINDOW_STATE_BOTTOM_RESIZABLE: whether the bottom edge is resizable, Since 3.22.23
 * @GDK_WINDOW_STATE_LEFT_TILED: whether the left edge is tiled, Since 3.22.23
 * @GDK_WINDOW_STATE_LEFT_RESIZABLE: whether the left edge is resizable, Since 3.22.23
 *
 * Specifies the state of a toplevel window.
 */
typedef enum
{
  GDK_WINDOW_STATE_WITHDRAWN        = 1 << 0,
  GDK_WINDOW_STATE_ICONIFIED        = 1 << 1,
  GDK_WINDOW_STATE_MAXIMIZED        = 1 << 2,
  GDK_WINDOW_STATE_STICKY           = 1 << 3,
  GDK_WINDOW_STATE_FULLSCREEN       = 1 << 4,
  GDK_WINDOW_STATE_ABOVE            = 1 << 5,
  GDK_WINDOW_STATE_BELOW            = 1 << 6,
  GDK_WINDOW_STATE_FOCUSED          = 1 << 7,
  GDK_WINDOW_STATE_TILED            = 1 << 8,
  GDK_WINDOW_STATE_TOP_TILED        = 1 << 9,
  GDK_WINDOW_STATE_TOP_RESIZABLE    = 1 << 10,
  GDK_WINDOW_STATE_RIGHT_TILED      = 1 << 11,
  GDK_WINDOW_STATE_RIGHT_RESIZABLE  = 1 << 12,
  GDK_WINDOW_STATE_BOTTOM_TILED     = 1 << 13,
  GDK_WINDOW_STATE_BOTTOM_RESIZABLE = 1 << 14,
  GDK_WINDOW_STATE_LEFT_TILED       = 1 << 15,
  GDK_WINDOW_STATE_LEFT_RESIZABLE   = 1 << 16
} CdkWindowState;

/**
 * CdkSettingAction:
 * @GDK_SETTING_ACTION_NEW: a setting was added.
 * @GDK_SETTING_ACTION_CHANGED: a setting was changed.
 * @GDK_SETTING_ACTION_DELETED: a setting was deleted.
 *
 * Specifies the kind of modification applied to a setting in a
 * #CdkEventSetting.
 */
typedef enum
{
  GDK_SETTING_ACTION_NEW,
  GDK_SETTING_ACTION_CHANGED,
  GDK_SETTING_ACTION_DELETED
} CdkSettingAction;

/**
 * CdkOwnerChange:
 * @GDK_OWNER_CHANGE_NEW_OWNER: some other app claimed the ownership
 * @GDK_OWNER_CHANGE_DESTROY: the window was destroyed
 * @GDK_OWNER_CHANGE_CLOSE: the client was closed
 *
 * Specifies why a selection ownership was changed.
 */
typedef enum
{
  GDK_OWNER_CHANGE_NEW_OWNER,
  GDK_OWNER_CHANGE_DESTROY,
  GDK_OWNER_CHANGE_CLOSE
} CdkOwnerChange;

/**
 * CdkEventAny:
 * @type: the type of the event.
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 *
 * Contains the fields which are common to all event structs.
 * Any event pointer can safely be cast to a pointer to a #CdkEventAny to
 * access these fields.
 */
struct _CdkEventAny
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
};

/**
 * CdkEventExpose:
 * @type: the type of the event (%GDK_EXPOSE or %GDK_DAMAGE).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @area: bounding box of @region.
 * @region: the region that needs to be redrawn.
 * @count: the number of contiguous %GDK_EXPOSE events following this one.
 *   The only use for this is “exposure compression”, i.e. handling all
 *   contiguous %GDK_EXPOSE events in one go, though GDK performs some
 *   exposure compression so this is not normally needed.
 *
 * Generated when all or part of a window becomes visible and needs to be
 * redrawn.
 */
struct _CdkEventExpose
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  CdkRectangle area;
  cairo_region_t *region;
  gint count; /* If non-zero, how many more events follow. */
};

/**
 * CdkEventVisibility:
 * @type: the type of the event (%GDK_VISIBILITY_NOTIFY).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @state: the new visibility state (%GDK_VISIBILITY_FULLY_OBSCURED,
 *   %GDK_VISIBILITY_PARTIAL or %GDK_VISIBILITY_UNOBSCURED).
 *
 * Generated when the window visibility status has changed.
 *
 * Deprecated: 3.12: Modern composited windowing systems with pervasive
 *     transparency make it impossible to track the visibility of a window
 *     reliably, so this event can not be guaranteed to provide useful
 *     information.
 */
struct _CdkEventVisibility
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  CdkVisibilityState state;
};

/**
 * CdkEventMotion:
 * @type: the type of the event.
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @time: the time of the event in milliseconds.
 * @x: the x coordinate of the pointer relative to the window.
 * @y: the y coordinate of the pointer relative to the window.
 * @axes: @x, @y translated to the axes of @device, or %NULL if @device is
 *   the mouse.
 * @state: (type CdkModifierType): a bit-mask representing the state of
 *   the modifier keys (e.g. Control, Shift and Alt) and the pointer
 *   buttons. See #CdkModifierType.
 * @is_hint: set to 1 if this event is just a hint, see the
 *   %GDK_POINTER_MOTION_HINT_MASK value of #CdkEventMask.
 * @device: the master device that the event originated from. Use
 * cdk_event_get_source_device() to get the slave device.
 * @x_root: the x coordinate of the pointer relative to the root of the
 *   screen.
 * @y_root: the y coordinate of the pointer relative to the root of the
 *   screen.
 *
 * Generated when the pointer moves.
 */
struct _CdkEventMotion
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  guint32 time;
  gdouble x;
  gdouble y;
  gdouble *axes;
  guint state;
  gint16 is_hint;
  CdkDevice *device;
  gdouble x_root, y_root;
};

/**
 * CdkEventButton:
 * @type: the type of the event (%GDK_BUTTON_PRESS, %GDK_2BUTTON_PRESS,
 *   %GDK_3BUTTON_PRESS or %GDK_BUTTON_RELEASE).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @time: the time of the event in milliseconds.
 * @x: the x coordinate of the pointer relative to the window.
 * @y: the y coordinate of the pointer relative to the window.
 * @axes: @x, @y translated to the axes of @device, or %NULL if @device is
 *   the mouse.
 * @state: (type CdkModifierType): a bit-mask representing the state of
 *   the modifier keys (e.g. Control, Shift and Alt) and the pointer
 *   buttons. See #CdkModifierType.
 * @button: the button which was pressed or released, numbered from 1 to 5.
 *   Normally button 1 is the left mouse button, 2 is the middle button,
 *   and 3 is the right button. On 2-button mice, the middle button can
 *   often be simulated by pressing both mouse buttons together.
 * @device: the master device that the event originated from. Use
 * cdk_event_get_source_device() to get the slave device.
 * @x_root: the x coordinate of the pointer relative to the root of the
 *   screen.
 * @y_root: the y coordinate of the pointer relative to the root of the
 *   screen.
 *
 * Used for button press and button release events. The
 * @type field will be one of %GDK_BUTTON_PRESS,
 * %GDK_2BUTTON_PRESS, %GDK_3BUTTON_PRESS or %GDK_BUTTON_RELEASE,
 *
 * Double and triple-clicks result in a sequence of events being received.
 * For double-clicks the order of events will be:
 *
 * - %GDK_BUTTON_PRESS
 * - %GDK_BUTTON_RELEASE
 * - %GDK_BUTTON_PRESS
 * - %GDK_2BUTTON_PRESS
 * - %GDK_BUTTON_RELEASE
 *
 * Note that the first click is received just like a normal
 * button press, while the second click results in a %GDK_2BUTTON_PRESS
 * being received just after the %GDK_BUTTON_PRESS.
 *
 * Triple-clicks are very similar to double-clicks, except that
 * %GDK_3BUTTON_PRESS is inserted after the third click. The order of the
 * events is:
 *
 * - %GDK_BUTTON_PRESS
 * - %GDK_BUTTON_RELEASE
 * - %GDK_BUTTON_PRESS
 * - %GDK_2BUTTON_PRESS
 * - %GDK_BUTTON_RELEASE
 * - %GDK_BUTTON_PRESS
 * - %GDK_3BUTTON_PRESS
 * - %GDK_BUTTON_RELEASE
 *
 * For a double click to occur, the second button press must occur within
 * 1/4 of a second of the first. For a triple click to occur, the third
 * button press must also occur within 1/2 second of the first button press.
 */
struct _CdkEventButton
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  guint32 time;
  gdouble x;
  gdouble y;
  gdouble *axes;
  guint state;
  guint button;
  CdkDevice *device;
  gdouble x_root, y_root;
};

/**
 * CdkEventTouch:
 * @type: the type of the event (%GDK_TOUCH_BEGIN, %GDK_TOUCH_UPDATE,
 *   %GDK_TOUCH_END, %GDK_TOUCH_CANCEL)
 * @window: the window which received the event
 * @send_event: %TRUE if the event was sent explicitly.
 * @time: the time of the event in milliseconds.
 * @x: the x coordinate of the pointer relative to the window
 * @y: the y coordinate of the pointer relative to the window
 * @axes: @x, @y translated to the axes of @device, or %NULL if @device is
 *   the mouse
 * @state: (type CdkModifierType): a bit-mask representing the state of
 *   the modifier keys (e.g. Control, Shift and Alt) and the pointer
 *   buttons. See #CdkModifierType
 * @sequence: the event sequence that the event belongs to
 * @emulating_pointer: whether the event should be used for emulating
 *   pointer event
 * @device: the master device that the event originated from. Use
 * cdk_event_get_source_device() to get the slave device.
 * @x_root: the x coordinate of the pointer relative to the root of the
 *   screen
 * @y_root: the y coordinate of the pointer relative to the root of the
 *   screen
 *
 * Used for touch events.
 * @type field will be one of %GDK_TOUCH_BEGIN, %GDK_TOUCH_UPDATE,
 * %GDK_TOUCH_END or %GDK_TOUCH_CANCEL.
 *
 * Touch events are grouped into sequences by means of the @sequence
 * field, which can also be obtained with cdk_event_get_event_sequence().
 * Each sequence begins with a %GDK_TOUCH_BEGIN event, followed by
 * any number of %GDK_TOUCH_UPDATE events, and ends with a %GDK_TOUCH_END
 * (or %GDK_TOUCH_CANCEL) event. With multitouch devices, there may be
 * several active sequences at the same time.
 */
struct _CdkEventTouch
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  guint32 time;
  gdouble x;
  gdouble y;
  gdouble *axes;
  guint state;
  CdkEventSequence *sequence;
  gboolean emulating_pointer;
  CdkDevice *device;
  gdouble x_root, y_root;
};

/**
 * CdkEventScroll:
 * @type: the type of the event (%GDK_SCROLL).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @time: the time of the event in milliseconds.
 * @x: the x coordinate of the pointer relative to the window.
 * @y: the y coordinate of the pointer relative to the window.
 * @state: (type CdkModifierType): a bit-mask representing the state of
 *   the modifier keys (e.g. Control, Shift and Alt) and the pointer
 *   buttons. See #CdkModifierType.
 * @direction: the direction to scroll to (one of %GDK_SCROLL_UP,
 *   %GDK_SCROLL_DOWN, %GDK_SCROLL_LEFT, %GDK_SCROLL_RIGHT or
 *   %GDK_SCROLL_SMOOTH).
 * @device: the master device that the event originated from. Use
 * cdk_event_get_source_device() to get the slave device.
 * @x_root: the x coordinate of the pointer relative to the root of the
 *   screen.
 * @y_root: the y coordinate of the pointer relative to the root of the
 *   screen.
 * @delta_x: the x coordinate of the scroll delta
 * @delta_y: the y coordinate of the scroll delta
 *
 * Generated from button presses for the buttons 4 to 7. Wheel mice are
 * usually configured to generate button press events for buttons 4 and 5
 * when the wheel is turned.
 *
 * Some GDK backends can also generate “smooth” scroll events, which
 * can be recognized by the %GDK_SCROLL_SMOOTH scroll direction. For
 * these, the scroll deltas can be obtained with
 * cdk_event_get_scroll_deltas().
 */
struct _CdkEventScroll
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  guint32 time;
  gdouble x;
  gdouble y;
  guint state;
  CdkScrollDirection direction;
  CdkDevice *device;
  gdouble x_root, y_root;
  gdouble delta_x;
  gdouble delta_y;
  guint is_stop : 1;
};

/**
 * CdkEventKey:
 * @type: the type of the event (%GDK_KEY_PRESS or %GDK_KEY_RELEASE).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @time: the time of the event in milliseconds.
 * @state: (type CdkModifierType): a bit-mask representing the state of
 *   the modifier keys (e.g. Control, Shift and Alt) and the pointer
 *   buttons. See #CdkModifierType.
 * @keyval: the key that was pressed or released. See the
 *   `cdk/cdkkeysyms.h` header file for a
 *   complete list of GDK key codes.
 * @length: the length of @string.
 * @string: a string containing an approximation of the text that
 *   would result from this keypress. The only correct way to handle text
 *   input of text is using input methods (see #CtkIMContext), so this
 *   field is deprecated and should never be used.
 *   (cdk_unicode_to_keyval() provides a non-deprecated way of getting
 *   an approximate translation for a key.) The string is encoded in the
 *   encoding of the current locale (Note: this for backwards compatibility:
 *   strings in CTK+ and GDK are typically in UTF-8.) and NUL-terminated.
 *   In some cases, the translation of the key code will be a single
 *   NUL byte, in which case looking at @length is necessary to distinguish
 *   it from the an empty translation.
 * @hardware_keycode: the raw code of the key that was pressed or released.
 * @group: the keyboard group.
 * @is_modifier: a flag that indicates if @hardware_keycode is mapped to a
 *   modifier. Since 2.10
 *
 * Describes a key press or key release event.
 */
struct _CdkEventKey
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  guint32 time;
  guint state;
  guint keyval;
  gint length;
  gchar *string;
  guint16 hardware_keycode;
  guint8 group;
  guint is_modifier : 1;
};

/**
 * CdkEventCrossing:
 * @type: the type of the event (%GDK_ENTER_NOTIFY or %GDK_LEAVE_NOTIFY).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @subwindow: the window that was entered or left.
 * @time: the time of the event in milliseconds.
 * @x: the x coordinate of the pointer relative to the window.
 * @y: the y coordinate of the pointer relative to the window.
 * @x_root: the x coordinate of the pointer relative to the root of the screen.
 * @y_root: the y coordinate of the pointer relative to the root of the screen.
 * @mode: the crossing mode (%GDK_CROSSING_NORMAL, %GDK_CROSSING_GRAB,
 *  %GDK_CROSSING_UNGRAB, %GDK_CROSSING_CTK_GRAB, %GDK_CROSSING_CTK_UNGRAB or
 *  %GDK_CROSSING_STATE_CHANGED).  %GDK_CROSSING_CTK_GRAB, %GDK_CROSSING_CTK_UNGRAB,
 *  and %GDK_CROSSING_STATE_CHANGED were added in 2.14 and are always synthesized,
 *  never native.
 * @detail: the kind of crossing that happened (%GDK_NOTIFY_INFERIOR,
 *  %GDK_NOTIFY_ANCESTOR, %GDK_NOTIFY_VIRTUAL, %GDK_NOTIFY_NONLINEAR or
 *  %GDK_NOTIFY_NONLINEAR_VIRTUAL).
 * @focus: %TRUE if @window is the focus window or an inferior.
 * @state: (type CdkModifierType): a bit-mask representing the state of
 *   the modifier keys (e.g. Control, Shift and Alt) and the pointer
 *   buttons. See #CdkModifierType.
 *
 * Generated when the pointer enters or leaves a window.
 */
struct _CdkEventCrossing
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  CdkWindow *subwindow;
  guint32 time;
  gdouble x;
  gdouble y;
  gdouble x_root;
  gdouble y_root;
  CdkCrossingMode mode;
  CdkNotifyType detail;
  gboolean focus;
  guint state;
};

/**
 * CdkEventFocus:
 * @type: the type of the event (%GDK_FOCUS_CHANGE).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @in: %TRUE if the window has gained the keyboard focus, %FALSE if
 *   it has lost the focus.
 *
 * Describes a change of keyboard focus.
 */
struct _CdkEventFocus
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  gint16 in;
};

/**
 * CdkEventConfigure:
 * @type: the type of the event (%GDK_CONFIGURE).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @x: the new x coordinate of the window, relative to its parent.
 * @y: the new y coordinate of the window, relative to its parent.
 * @width: the new width of the window.
 * @height: the new height of the window.
 *
 * Generated when a window size or position has changed.
 */
struct _CdkEventConfigure
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  gint x, y;
  gint width;
  gint height;
};

/**
 * CdkEventProperty:
 * @type: the type of the event (%GDK_PROPERTY_NOTIFY).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @atom: the property that was changed.
 * @time: the time of the event in milliseconds.
 * @state: (type CdkPropertyState): whether the property was changed
 *   (%GDK_PROPERTY_NEW_VALUE) or deleted (%GDK_PROPERTY_DELETE).
 *
 * Describes a property change on a window.
 */
struct _CdkEventProperty
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  CdkAtom atom;
  guint32 time;
  guint state;
};

/**
 * CdkEventSelection:
 * @type: the type of the event (%GDK_SELECTION_CLEAR,
 *   %GDK_SELECTION_NOTIFY or %GDK_SELECTION_REQUEST).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @selection: the selection.
 * @target: the target to which the selection should be converted.
 * @property: the property in which to place the result of the conversion.
 * @time: the time of the event in milliseconds.
 * @requestor: the window on which to place @property or %NULL if none.
 *
 * Generated when a selection is requested or ownership of a selection
 * is taken over by another client application.
 */
struct _CdkEventSelection
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  CdkAtom selection;
  CdkAtom target;
  CdkAtom property;
  guint32 time;
  CdkWindow *requestor;
};

/**
 * CdkEventOwnerChange:
 * @type: the type of the event (%GDK_OWNER_CHANGE).
 * @window: the window which received the event
 * @send_event: %TRUE if the event was sent explicitly.
 * @owner: the new owner of the selection, or %NULL if there is none
 * @reason: the reason for the ownership change as a #CdkOwnerChange value
 * @selection: the atom identifying the selection
 * @time: the timestamp of the event
 * @selection_time: the time at which the selection ownership was taken
 *   over
 *
 * Generated when the owner of a selection changes. On X11, this
 * information is only available if the X server supports the XFIXES
 * extension.
 *
 * Since: 2.6
 */
struct _CdkEventOwnerChange
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  CdkWindow *owner;
  CdkOwnerChange reason;
  CdkAtom selection;
  guint32 time;
  guint32 selection_time;
};

/**
 * CdkEventProximity:
 * @type: the type of the event (%GDK_PROXIMITY_IN or %GDK_PROXIMITY_OUT).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @time: the time of the event in milliseconds.
 * @device: the master device that the event originated from. Use
 * cdk_event_get_source_device() to get the slave device.
 *
 * Proximity events are generated when using GDK’s wrapper for the
 * XInput extension. The XInput extension is an add-on for standard X
 * that allows you to use nonstandard devices such as graphics tablets.
 * A proximity event indicates that the stylus has moved in or out of
 * contact with the tablet, or perhaps that the user’s finger has moved
 * in or out of contact with a touch screen.
 *
 * This event type will be used pretty rarely. It only is important for
 * XInput aware programs that are drawing their own cursor.
 */
struct _CdkEventProximity
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  guint32 time;
  CdkDevice *device;
};

/**
 * CdkEventSetting:
 * @type: the type of the event (%GDK_SETTING).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @action: what happened to the setting (%GDK_SETTING_ACTION_NEW,
 *   %GDK_SETTING_ACTION_CHANGED or %GDK_SETTING_ACTION_DELETED).
 * @name: the name of the setting.
 *
 * Generated when a setting is modified.
 */
struct _CdkEventSetting
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  CdkSettingAction action;
  char *name;
};

/**
 * CdkEventWindowState:
 * @type: the type of the event (%GDK_WINDOW_STATE).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @changed_mask: mask specifying what flags have changed.
 * @new_window_state: the new window state, a combination of
 *   #CdkWindowState bits.
 *
 * Generated when the state of a toplevel window changes.
 */
struct _CdkEventWindowState
{
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  CdkWindowState changed_mask;
  CdkWindowState new_window_state;
};

/**
 * CdkEventGrabBroken:
 * @type: the type of the event (%GDK_GRAB_BROKEN)
 * @window: the window which received the event, i.e. the window
 *   that previously owned the grab
 * @send_event: %TRUE if the event was sent explicitly.
 * @keyboard: %TRUE if a keyboard grab was broken, %FALSE if a pointer
 *   grab was broken
 * @implicit: %TRUE if the broken grab was implicit
 * @grab_window: If this event is caused by another grab in the same
 *   application, @grab_window contains the new grab window. Otherwise
 *   @grab_window is %NULL.
 *
 * Generated when a pointer or keyboard grab is broken. On X11, this happens
 * when the grab window becomes unviewable (i.e. it or one of its ancestors
 * is unmapped), or if the same application grabs the pointer or keyboard
 * again. Note that implicit grabs (which are initiated by button presses)
 * can also cause #CdkEventGrabBroken events.
 *
 * Since: 2.8
 */
struct _CdkEventGrabBroken {
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  gboolean keyboard;
  gboolean implicit;
  CdkWindow *grab_window;
};

/**
 * CdkEventDND:
 * @type: the type of the event (%GDK_DRAG_ENTER, %GDK_DRAG_LEAVE,
 *   %GDK_DRAG_MOTION, %GDK_DRAG_STATUS, %GDK_DROP_START or
 *   %GDK_DROP_FINISHED).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @context: the #CdkDragContext for the current DND operation.
 * @time: the time of the event in milliseconds.
 * @x_root: the x coordinate of the pointer relative to the root of the
 *   screen, only set for %GDK_DRAG_MOTION and %GDK_DROP_START.
 * @y_root: the y coordinate of the pointer relative to the root of the
 *   screen, only set for %GDK_DRAG_MOTION and %GDK_DROP_START.
 *
 * Generated during DND operations.
 */
struct _CdkEventDND {
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  CdkDragContext *context;

  guint32 time;
  gshort x_root, y_root;
};

/**
 * CdkEventTouchpadSwipe:
 * @type: the type of the event (%GDK_TOUCHPAD_SWIPE)
 * @window: the window which received the event
 * @send_event: %TRUE if the event was sent explicitly
 * @phase: the current phase of the gesture
 * @n_fingers: The number of fingers triggering the swipe
 * @time: the time of the event in milliseconds
 * @x: The X coordinate of the pointer
 * @y: The Y coordinate of the pointer
 * @dx: Movement delta in the X axis of the swipe focal point
 * @dy: Movement delta in the Y axis of the swipe focal point
 * @x_root: The X coordinate of the pointer, relative to the
 *   root of the screen.
 * @y_root: The Y coordinate of the pointer, relative to the
 *   root of the screen.
 * @state: (type CdkModifierType): a bit-mask representing the state of
 *   the modifier keys (e.g. Control, Shift and Alt) and the pointer
 *   buttons. See #CdkModifierType.
 *
 * Generated during touchpad swipe gestures.
 */
struct _CdkEventTouchpadSwipe {
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  gint8 phase;
  gint8 n_fingers;
  guint32 time;
  gdouble x;
  gdouble y;
  gdouble dx;
  gdouble dy;
  gdouble x_root, y_root;
  guint state;
};

/**
 * CdkEventTouchpadPinch:
 * @type: the type of the event (%GDK_TOUCHPAD_PINCH)
 * @window: the window which received the event
 * @send_event: %TRUE if the event was sent explicitly
 * @phase: the current phase of the gesture
 * @n_fingers: The number of fingers triggering the pinch
 * @time: the time of the event in milliseconds
 * @x: The X coordinate of the pointer
 * @y: The Y coordinate of the pointer
 * @dx: Movement delta in the X axis of the swipe focal point
 * @dy: Movement delta in the Y axis of the swipe focal point
 * @angle_delta: The angle change in radians, negative angles
 *   denote counter-clockwise movements
 * @scale: The current scale, relative to that at the time of
 *   the corresponding %GDK_TOUCHPAD_GESTURE_PHASE_BEGIN event
 * @x_root: The X coordinate of the pointer, relative to the
 *   root of the screen.
 * @y_root: The Y coordinate of the pointer, relative to the
 *   root of the screen.
 * @state: (type CdkModifierType): a bit-mask representing the state of
 *   the modifier keys (e.g. Control, Shift and Alt) and the pointer
 *   buttons. See #CdkModifierType.
 *
 * Generated during touchpad swipe gestures.
 */
struct _CdkEventTouchpadPinch {
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  gint8 phase;
  gint8 n_fingers;
  guint32 time;
  gdouble x;
  gdouble y;
  gdouble dx;
  gdouble dy;
  gdouble angle_delta;
  gdouble scale;
  gdouble x_root, y_root;
  guint state;
};

/**
 * CdkEventPadButton:
 * @type: the type of the event (%GDK_PAD_BUTTON_PRESS or %GDK_PAD_BUTTON_RELEASE).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @time: the time of the event in milliseconds.
 * @group: the pad group the button belongs to. A %GDK_SOURCE_TABLET_PAD device
 *   may have one or more groups containing a set of buttons/rings/strips each.
 * @button: The pad button that was pressed.
 * @mode: The current mode of @group. Different groups in a %GDK_SOURCE_TABLET_PAD
 *   device may have different current modes.
 *
 * Generated during %GDK_SOURCE_TABLET_PAD button presses and releases.
 *
 * Since: 3.22
 */
struct _CdkEventPadButton {
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  guint32 time;
  guint group;
  guint button;
  guint mode;
};

/**
 * CdkEventPadAxis:
 * @type: the type of the event (%GDK_PAD_RING or %GDK_PAD_STRIP).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @time: the time of the event in milliseconds.
 * @group: the pad group the ring/strip belongs to. A %GDK_SOURCE_TABLET_PAD
 *   device may have one or more groups containing a set of buttons/rings/strips
 *   each.
 * @index: number of strip/ring that was interacted. This number is 0-indexed.
 * @mode: The current mode of @group. Different groups in a %GDK_SOURCE_TABLET_PAD
 *   device may have different current modes.
 * @value: The current value for the given axis.
 *
 * Generated during %GDK_SOURCE_TABLET_PAD interaction with tactile sensors.
 *
 * Since: 3.22
 */
struct _CdkEventPadAxis {
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  guint32 time;
  guint group;
  guint index;
  guint mode;
  gdouble value;
};

/**
 * CdkEventPadGroupMode:
 * @type: the type of the event (%GDK_PAD_GROUP_MODE).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @time: the time of the event in milliseconds.
 * @group: the pad group that is switching mode. A %GDK_SOURCE_TABLET_PAD
 *   device may have one or more groups containing a set of buttons/rings/strips
 *   each.
 * @mode: The new mode of @group. Different groups in a %GDK_SOURCE_TABLET_PAD
 *   device may have different current modes.
 *
 * Generated during %GDK_SOURCE_TABLET_PAD mode switches in a group.
 *
 * Since: 3.22
 */
struct _CdkEventPadGroupMode {
  CdkEventType type;
  CdkWindow *window;
  gint8 send_event;
  guint32 time;
  guint group;
  guint mode;
};

/**
 * CdkEvent:
 * @type: the #CdkEventType
 * @any: a #CdkEventAny
 * @expose: a #CdkEventExpose
 * @visibility: a #CdkEventVisibility
 * @motion: a #CdkEventMotion
 * @button: a #CdkEventButton
 * @touch: a #CdkEventTouch
 * @scroll: a #CdkEventScroll
 * @key: a #CdkEventKey
 * @crossing: a #CdkEventCrossing
 * @focus_change: a #CdkEventFocus
 * @configure: a #CdkEventConfigure
 * @property: a #CdkEventProperty
 * @selection: a #CdkEventSelection
 * @owner_change: a #CdkEventOwnerChange
 * @proximity: a #CdkEventProximity
 * @dnd: a #CdkEventDND
 * @window_state: a #CdkEventWindowState
 * @setting: a #CdkEventSetting
 * @grab_broken: a #CdkEventGrabBroken
 * @touchpad_swipe: a #CdkEventTouchpadSwipe
 * @touchpad_pinch: a #CdkEventTouchpadPinch
 * @pad_button: a #CdkEventPadButton
 * @pad_axis: a #CdkEventPadAxis
 * @pad_group_mode: a #CdkEventPadGroupMode
 *
 * A #CdkEvent contains a union of all of the event types,
 * and allows access to the data fields in a number of ways.
 *
 * The event type is always the first field in all of the event types, and
 * can always be accessed with the following code, no matter what type of
 * event it is:
 * |[<!-- language="C" -->
 *   CdkEvent *event;
 *   CdkEventType type;
 *
 *   type = event->type;
 * ]|
 *
 * To access other fields of the event, the pointer to the event
 * can be cast to the appropriate event type, or the union member
 * name can be used. For example if the event type is %GDK_BUTTON_PRESS
 * then the x coordinate of the button press can be accessed with:
 * |[<!-- language="C" -->
 *   CdkEvent *event;
 *   gdouble x;
 *
 *   x = ((CdkEventButton*)event)->x;
 * ]|
 * or:
 * |[<!-- language="C" -->
 *   CdkEvent *event;
 *   gdouble x;
 *
 *   x = event->button.x;
 * ]|
 */
union _CdkEvent
{
  CdkEventType		    type;
  CdkEventAny		    any;
  CdkEventExpose	    expose;
  CdkEventVisibility	    visibility;
  CdkEventMotion	    motion;
  CdkEventButton	    button;
  CdkEventTouch             touch;
  CdkEventScroll            scroll;
  CdkEventKey		    key;
  CdkEventCrossing	    crossing;
  CdkEventFocus		    focus_change;
  CdkEventConfigure	    configure;
  CdkEventProperty	    property;
  CdkEventSelection	    selection;
  CdkEventOwnerChange  	    owner_change;
  CdkEventProximity	    proximity;
  CdkEventDND               dnd;
  CdkEventWindowState       window_state;
  CdkEventSetting           setting;
  CdkEventGrabBroken        grab_broken;
  CdkEventTouchpadSwipe     touchpad_swipe;
  CdkEventTouchpadPinch     touchpad_pinch;
  CdkEventPadButton         pad_button;
  CdkEventPadAxis           pad_axis;
  CdkEventPadGroupMode      pad_group_mode;
};

GDK_AVAILABLE_IN_ALL
GType     cdk_event_get_type            (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_14
GType     cdk_event_sequence_get_type   (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
gboolean  cdk_events_pending	 	(void);
GDK_AVAILABLE_IN_ALL
CdkEvent* cdk_event_get			(void);

GDK_AVAILABLE_IN_ALL
CdkEvent* cdk_event_peek                (void);
GDK_AVAILABLE_IN_ALL
void      cdk_event_put	 		(const CdkEvent *event);

GDK_AVAILABLE_IN_ALL
CdkEvent* cdk_event_new                 (CdkEventType    type);
GDK_AVAILABLE_IN_ALL
CdkEvent* cdk_event_copy     		(const CdkEvent *event);
GDK_AVAILABLE_IN_ALL
void	  cdk_event_free     		(CdkEvent 	*event);

GDK_AVAILABLE_IN_3_10
CdkWindow *cdk_event_get_window         (const CdkEvent *event);

GDK_AVAILABLE_IN_ALL
guint32   cdk_event_get_time            (const CdkEvent  *event);
GDK_AVAILABLE_IN_ALL
gboolean  cdk_event_get_state           (const CdkEvent  *event,
                                         CdkModifierType *state);
GDK_AVAILABLE_IN_ALL
gboolean  cdk_event_get_coords		(const CdkEvent  *event,
					 gdouble	 *x_win,
					 gdouble	 *y_win);
GDK_AVAILABLE_IN_ALL
gboolean  cdk_event_get_root_coords	(const CdkEvent *event,
					 gdouble	*x_root,
					 gdouble	*y_root);
GDK_AVAILABLE_IN_3_2
gboolean  cdk_event_get_button          (const CdkEvent *event,
                                         guint          *button);
GDK_AVAILABLE_IN_3_2
gboolean  cdk_event_get_click_count     (const CdkEvent *event,
                                         guint          *click_count);
GDK_AVAILABLE_IN_3_2
gboolean  cdk_event_get_keyval          (const CdkEvent *event,
                                         guint          *keyval);
GDK_AVAILABLE_IN_3_2
gboolean  cdk_event_get_keycode         (const CdkEvent *event,
                                         guint16        *keycode);
GDK_AVAILABLE_IN_3_2
gboolean cdk_event_get_scroll_direction (const CdkEvent *event,
                                         CdkScrollDirection *direction);
GDK_AVAILABLE_IN_3_4
gboolean  cdk_event_get_scroll_deltas   (const CdkEvent *event,
                                         gdouble         *delta_x,
                                         gdouble         *delta_y);

GDK_AVAILABLE_IN_3_20
gboolean  cdk_event_is_scroll_stop_event (const CdkEvent *event);

GDK_AVAILABLE_IN_ALL
gboolean  cdk_event_get_axis            (const CdkEvent  *event,
                                         CdkAxisUse       axis_use,
                                         gdouble         *value);
GDK_AVAILABLE_IN_ALL
void       cdk_event_set_device         (CdkEvent        *event,
                                         CdkDevice       *device);
GDK_AVAILABLE_IN_ALL
CdkDevice* cdk_event_get_device         (const CdkEvent  *event);
GDK_AVAILABLE_IN_ALL
void       cdk_event_set_source_device  (CdkEvent        *event,
                                         CdkDevice       *device);
GDK_AVAILABLE_IN_ALL
CdkDevice* cdk_event_get_source_device  (const CdkEvent  *event);
GDK_AVAILABLE_IN_ALL
void       cdk_event_request_motions    (const CdkEventMotion *event);
GDK_AVAILABLE_IN_3_4
gboolean   cdk_event_triggers_context_menu (const CdkEvent *event);

GDK_AVAILABLE_IN_ALL
gboolean  cdk_events_get_distance       (CdkEvent        *event1,
                                         CdkEvent        *event2,
                                         gdouble         *distance);
GDK_AVAILABLE_IN_ALL
gboolean  cdk_events_get_angle          (CdkEvent        *event1,
                                         CdkEvent        *event2,
                                         gdouble         *angle);
GDK_AVAILABLE_IN_ALL
gboolean  cdk_events_get_center         (CdkEvent        *event1,
                                         CdkEvent        *event2,
                                         gdouble         *x,
                                         gdouble         *y);

GDK_AVAILABLE_IN_ALL
void	  cdk_event_handler_set 	(CdkEventFunc    func,
					 gpointer        data,
					 GDestroyNotify  notify);

GDK_AVAILABLE_IN_ALL
void       cdk_event_set_screen         (CdkEvent        *event,
                                         CdkScreen       *screen);
GDK_AVAILABLE_IN_ALL
CdkScreen *cdk_event_get_screen         (const CdkEvent  *event);

GDK_AVAILABLE_IN_3_4
CdkEventSequence *cdk_event_get_event_sequence (const CdkEvent *event);

GDK_AVAILABLE_IN_3_10
CdkEventType cdk_event_get_event_type   (const CdkEvent *event);

GDK_AVAILABLE_IN_3_20
CdkSeat  *cdk_event_get_seat            (const CdkEvent *event);

GDK_AVAILABLE_IN_ALL
void	  cdk_set_show_events		(gboolean	 show_events);
GDK_AVAILABLE_IN_ALL
gboolean  cdk_get_show_events		(void);

GDK_AVAILABLE_IN_ALL
gboolean cdk_setting_get                (const gchar    *name,
                                         GValue         *value);

GDK_AVAILABLE_IN_3_22
CdkDeviceTool *cdk_event_get_device_tool (const CdkEvent *event);

GDK_AVAILABLE_IN_3_22
void           cdk_event_set_device_tool (CdkEvent       *event,
                                          CdkDeviceTool  *tool);

GDK_AVAILABLE_IN_3_22
int            cdk_event_get_scancode    (CdkEvent *event);

GDK_AVAILABLE_IN_3_22
gboolean       cdk_event_get_pointer_emulated (CdkEvent *event);

G_END_DECLS

#endif /* __GDK_EVENTS_H__ */
