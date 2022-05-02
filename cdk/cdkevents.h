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

#ifndef __CDK_EVENTS_H__
#define __CDK_EVENTS_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
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
 * The event structures contain data specific to each type of event in CDK.
 *
 * > A common mistake is to forget to set the event mask of a widget so that
 * > the required events are received. See ctk_widget_set_events().
 */


#define CDK_TYPE_EVENT          (cdk_event_get_type ())
#define CDK_TYPE_EVENT_SEQUENCE (cdk_event_sequence_get_type ())

/**
 * CDK_PRIORITY_EVENTS:
 *
 * This is the priority that events from the X server are given in the
 * [GLib Main Loop][glib-The-Main-Event-Loop].
 */
#define CDK_PRIORITY_EVENTS	(G_PRIORITY_DEFAULT)

/**
 * CDK_PRIORITY_REDRAW: (value 120)
 *
 * This is the priority that the idle handler processing window updates
 * is given in the
 * [GLib Main Loop][glib-The-Main-Event-Loop].
 */
#define CDK_PRIORITY_REDRAW     (G_PRIORITY_HIGH_IDLE + 20)

/**
 * CDK_EVENT_PROPAGATE:
 *
 * Use this macro as the return value for continuing the propagation of
 * an event handler.
 *
 * Since: 3.4
 */
#define CDK_EVENT_PROPAGATE     (FALSE)

/**
 * CDK_EVENT_STOP:
 *
 * Use this macro as the return value for stopping the propagation of
 * an event handler.
 *
 * Since: 3.4
 */
#define CDK_EVENT_STOP          (TRUE)

/**
 * CDK_BUTTON_PRIMARY:
 *
 * The primary button. This is typically the left mouse button, or the
 * right button in a left-handed setup.
 *
 * Since: 3.4
 */
#define CDK_BUTTON_PRIMARY      (1)

/**
 * CDK_BUTTON_MIDDLE:
 *
 * The middle button.
 *
 * Since: 3.4
 */
#define CDK_BUTTON_MIDDLE       (2)

/**
 * CDK_BUTTON_SECONDARY:
 *
 * The secondary button. This is typically the right mouse button, or the
 * left button in a left-handed setup.
 *
 * Since: 3.4
 */
#define CDK_BUTTON_SECONDARY    (3)



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
 * handle all CDK events.
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
 * @CDK_FILTER_CONTINUE: event not handled, continue processing.
 * @CDK_FILTER_TRANSLATE: native event translated into a CDK event and stored
 *  in the `event` structure that was passed in.
 * @CDK_FILTER_REMOVE: event handled, terminate processing.
 *
 * Specifies the result of applying a #CdkFilterFunc to a native event.
 */
typedef enum {
  CDK_FILTER_CONTINUE,	  /* Event not handled, continue processesing */
  CDK_FILTER_TRANSLATE,	  /* Native event translated into a CDK event and
                             stored in the "event" structure that was
                             passed in */
  CDK_FILTER_REMOVE	  /* Terminate processing, removing event */
} CdkFilterReturn;

/**
 * CdkFilterFunc:
 * @xevent: the native event to filter.
 * @event: the CDK event to which the X event will be translated.
 * @data: (closure): user data set when the filter was installed.
 *
 * Specifies the type of function used to filter native events before they are
 * converted to CDK events.
 *
 * When a filter is called, @event is unpopulated, except for
 * `event->window`. The filter may translate the native
 * event to a CDK event and store the result in @event, or handle it without
 * translation. If the filter translates the event and processing should
 * continue, it should return %CDK_FILTER_TRANSLATE.
 *
 * Returns: a #CdkFilterReturn value.
 */
typedef CdkFilterReturn (*CdkFilterFunc) (CdkXEvent *xevent,
					  CdkEvent *event,
					  gpointer  data);


/**
 * CdkEventType:
 * @CDK_NOTHING: a special code to indicate a null event.
 * @CDK_DELETE: the window manager has requested that the toplevel window be
 *   hidden or destroyed, usually when the user clicks on a special icon in the
 *   title bar.
 * @CDK_DESTROY: the window has been destroyed.
 * @CDK_EXPOSE: all or part of the window has become visible and needs to be
 *   redrawn.
 * @CDK_MOTION_NOTIFY: the pointer (usually a mouse) has moved.
 * @CDK_BUTTON_PRESS: a mouse button has been pressed.
 * @CDK_2BUTTON_PRESS: a mouse button has been double-clicked (clicked twice
 *   within a short period of time). Note that each click also generates a
 *   %CDK_BUTTON_PRESS event.
 * @CDK_DOUBLE_BUTTON_PRESS: alias for %CDK_2BUTTON_PRESS, added in 3.6.
 * @CDK_3BUTTON_PRESS: a mouse button has been clicked 3 times in a short period
 *   of time. Note that each click also generates a %CDK_BUTTON_PRESS event.
 * @CDK_TRIPLE_BUTTON_PRESS: alias for %CDK_3BUTTON_PRESS, added in 3.6.
 * @CDK_BUTTON_RELEASE: a mouse button has been released.
 * @CDK_KEY_PRESS: a key has been pressed.
 * @CDK_KEY_RELEASE: a key has been released.
 * @CDK_ENTER_NOTIFY: the pointer has entered the window.
 * @CDK_LEAVE_NOTIFY: the pointer has left the window.
 * @CDK_FOCUS_CHANGE: the keyboard focus has entered or left the window.
 * @CDK_CONFIGURE: the size, position or stacking order of the window has changed.
 *   Note that CTK+ discards these events for %CDK_WINDOW_CHILD windows.
 * @CDK_MAP: the window has been mapped.
 * @CDK_UNMAP: the window has been unmapped.
 * @CDK_PROPERTY_NOTIFY: a property on the window has been changed or deleted.
 * @CDK_SELECTION_CLEAR: the application has lost ownership of a selection.
 * @CDK_SELECTION_REQUEST: another application has requested a selection.
 * @CDK_SELECTION_NOTIFY: a selection has been received.
 * @CDK_PROXIMITY_IN: an input device has moved into contact with a sensing
 *   surface (e.g. a touchscreen or graphics tablet).
 * @CDK_PROXIMITY_OUT: an input device has moved out of contact with a sensing
 *   surface.
 * @CDK_DRAG_ENTER: the mouse has entered the window while a drag is in progress.
 * @CDK_DRAG_LEAVE: the mouse has left the window while a drag is in progress.
 * @CDK_DRAG_MOTION: the mouse has moved in the window while a drag is in
 *   progress.
 * @CDK_DRAG_STATUS: the status of the drag operation initiated by the window
 *   has changed.
 * @CDK_DROP_START: a drop operation onto the window has started.
 * @CDK_DROP_FINISHED: the drop operation initiated by the window has completed.
 * @CDK_CLIENT_EVENT: a message has been received from another application.
 * @CDK_VISIBILITY_NOTIFY: the window visibility status has changed.
 * @CDK_SCROLL: the scroll wheel was turned
 * @CDK_WINDOW_STATE: the state of a window has changed. See #CdkWindowState
 *   for the possible window states
 * @CDK_SETTING: a setting has been modified.
 * @CDK_OWNER_CHANGE: the owner of a selection has changed. This event type
 *   was added in 2.6
 * @CDK_GRAB_BROKEN: a pointer or keyboard grab was broken. This event type
 *   was added in 2.8.
 * @CDK_DAMAGE: the content of the window has been changed. This event type
 *   was added in 2.14.
 * @CDK_TOUCH_BEGIN: A new touch event sequence has just started. This event
 *   type was added in 3.4.
 * @CDK_TOUCH_UPDATE: A touch event sequence has been updated. This event type
 *   was added in 3.4.
 * @CDK_TOUCH_END: A touch event sequence has finished. This event type
 *   was added in 3.4.
 * @CDK_TOUCH_CANCEL: A touch event sequence has been canceled. This event type
 *   was added in 3.4.
 * @CDK_TOUCHPAD_SWIPE: A touchpad swipe gesture event, the current state
 *   is determined by its phase field. This event type was added in 3.18.
 * @CDK_TOUCHPAD_PINCH: A touchpad pinch gesture event, the current state
 *   is determined by its phase field. This event type was added in 3.18.
 * @CDK_PAD_BUTTON_PRESS: A tablet pad button press event. This event type
 *   was added in 3.22.
 * @CDK_PAD_BUTTON_RELEASE: A tablet pad button release event. This event type
 *   was added in 3.22.
 * @CDK_PAD_RING: A tablet pad axis event from a "ring". This event type was
 *   added in 3.22.
 * @CDK_PAD_STRIP: A tablet pad axis event from a "strip". This event type was
 *   added in 3.22.
 * @CDK_PAD_GROUP_MODE: A tablet pad group mode change. This event type was
 *   added in 3.22.
 * @CDK_EVENT_LAST: marks the end of the CdkEventType enumeration. Added in 2.18
 *
 * Specifies the type of the event.
 *
 * Do not confuse these events with the signals that CTK+ widgets emit.
 * Although many of these events result in corresponding signals being emitted,
 * the events are often transformed or filtered along the way.
 *
 * In some language bindings, the values %CDK_2BUTTON_PRESS and
 * %CDK_3BUTTON_PRESS would translate into something syntactically
 * invalid (eg `Cdk.EventType.2ButtonPress`, where a
 * symbol is not allowed to start with a number). In that case, the
 * aliases %CDK_DOUBLE_BUTTON_PRESS and %CDK_TRIPLE_BUTTON_PRESS can
 * be used instead.
 */
typedef enum
{
  CDK_NOTHING		= -1,
  CDK_DELETE		= 0,
  CDK_DESTROY		= 1,
  CDK_EXPOSE		= 2,
  CDK_MOTION_NOTIFY	= 3,
  CDK_BUTTON_PRESS	= 4,
  CDK_2BUTTON_PRESS	= 5,
  CDK_DOUBLE_BUTTON_PRESS = CDK_2BUTTON_PRESS,
  CDK_3BUTTON_PRESS	= 6,
  CDK_TRIPLE_BUTTON_PRESS = CDK_3BUTTON_PRESS,
  CDK_BUTTON_RELEASE	= 7,
  CDK_KEY_PRESS		= 8,
  CDK_KEY_RELEASE	= 9,
  CDK_ENTER_NOTIFY	= 10,
  CDK_LEAVE_NOTIFY	= 11,
  CDK_FOCUS_CHANGE	= 12,
  CDK_CONFIGURE		= 13,
  CDK_MAP		= 14,
  CDK_UNMAP		= 15,
  CDK_PROPERTY_NOTIFY	= 16,
  CDK_SELECTION_CLEAR	= 17,
  CDK_SELECTION_REQUEST = 18,
  CDK_SELECTION_NOTIFY	= 19,
  CDK_PROXIMITY_IN	= 20,
  CDK_PROXIMITY_OUT	= 21,
  CDK_DRAG_ENTER        = 22,
  CDK_DRAG_LEAVE        = 23,
  CDK_DRAG_MOTION       = 24,
  CDK_DRAG_STATUS       = 25,
  CDK_DROP_START        = 26,
  CDK_DROP_FINISHED     = 27,
  CDK_CLIENT_EVENT	= 28,
  CDK_VISIBILITY_NOTIFY = 29,
  CDK_SCROLL            = 31,
  CDK_WINDOW_STATE      = 32,
  CDK_SETTING           = 33,
  CDK_OWNER_CHANGE      = 34,
  CDK_GRAB_BROKEN       = 35,
  CDK_DAMAGE            = 36,
  CDK_TOUCH_BEGIN       = 37,
  CDK_TOUCH_UPDATE      = 38,
  CDK_TOUCH_END         = 39,
  CDK_TOUCH_CANCEL      = 40,
  CDK_TOUCHPAD_SWIPE    = 41,
  CDK_TOUCHPAD_PINCH    = 42,
  CDK_PAD_BUTTON_PRESS  = 43,
  CDK_PAD_BUTTON_RELEASE = 44,
  CDK_PAD_RING          = 45,
  CDK_PAD_STRIP         = 46,
  CDK_PAD_GROUP_MODE    = 47,
  CDK_EVENT_LAST        /* helper variable for decls */
} CdkEventType;

/**
 * CdkVisibilityState:
 * @CDK_VISIBILITY_UNOBSCURED: the window is completely visible.
 * @CDK_VISIBILITY_PARTIAL: the window is partially visible.
 * @CDK_VISIBILITY_FULLY_OBSCURED: the window is not visible at all.
 *
 * Specifies the visiblity status of a window for a #CdkEventVisibility.
 */
typedef enum
{
  CDK_VISIBILITY_UNOBSCURED,
  CDK_VISIBILITY_PARTIAL,
  CDK_VISIBILITY_FULLY_OBSCURED
} CdkVisibilityState;

/**
 * CdkTouchpadGesturePhase:
 * @CDK_TOUCHPAD_GESTURE_PHASE_BEGIN: The gesture has begun.
 * @CDK_TOUCHPAD_GESTURE_PHASE_UPDATE: The gesture has been updated.
 * @CDK_TOUCHPAD_GESTURE_PHASE_END: The gesture was finished, changes
 *   should be permanently applied.
 * @CDK_TOUCHPAD_GESTURE_PHASE_CANCEL: The gesture was cancelled, all
 *   changes should be undone.
 *
 * Specifies the current state of a touchpad gesture. All gestures are
 * guaranteed to begin with an event with phase %CDK_TOUCHPAD_GESTURE_PHASE_BEGIN,
 * followed by 0 or several events with phase %CDK_TOUCHPAD_GESTURE_PHASE_UPDATE.
 *
 * A finished gesture may have 2 possible outcomes, an event with phase
 * %CDK_TOUCHPAD_GESTURE_PHASE_END will be emitted when the gesture is
 * considered successful, this should be used as the hint to perform any
 * permanent changes.

 * Cancelled gestures may be so for a variety of reasons, due to hardware
 * or the compositor, or due to the gesture recognition layers hinting the
 * gesture did not finish resolutely (eg. a 3rd finger being added during
 * a pinch gesture). In these cases, the last event will report the phase
 * %CDK_TOUCHPAD_GESTURE_PHASE_CANCEL, this should be used as a hint
 * to undo any visible/permanent changes that were done throughout the
 * progress of the gesture.
 *
 * See also #CdkEventTouchpadSwipe and #CdkEventTouchpadPinch.
 *
 */
typedef enum
{
  CDK_TOUCHPAD_GESTURE_PHASE_BEGIN,
  CDK_TOUCHPAD_GESTURE_PHASE_UPDATE,
  CDK_TOUCHPAD_GESTURE_PHASE_END,
  CDK_TOUCHPAD_GESTURE_PHASE_CANCEL
} CdkTouchpadGesturePhase;

/**
 * CdkScrollDirection:
 * @CDK_SCROLL_UP: the window is scrolled up.
 * @CDK_SCROLL_DOWN: the window is scrolled down.
 * @CDK_SCROLL_LEFT: the window is scrolled to the left.
 * @CDK_SCROLL_RIGHT: the window is scrolled to the right.
 * @CDK_SCROLL_SMOOTH: the scrolling is determined by the delta values
 *   in #CdkEventScroll. See cdk_event_get_scroll_deltas(). Since: 3.4
 *
 * Specifies the direction for #CdkEventScroll.
 */
typedef enum
{
  CDK_SCROLL_UP,
  CDK_SCROLL_DOWN,
  CDK_SCROLL_LEFT,
  CDK_SCROLL_RIGHT,
  CDK_SCROLL_SMOOTH
} CdkScrollDirection;

/**
 * CdkNotifyType:
 * @CDK_NOTIFY_ANCESTOR: the window is entered from an ancestor or
 *   left towards an ancestor.
 * @CDK_NOTIFY_VIRTUAL: the pointer moves between an ancestor and an
 *   inferior of the window.
 * @CDK_NOTIFY_INFERIOR: the window is entered from an inferior or
 *   left towards an inferior.
 * @CDK_NOTIFY_NONLINEAR: the window is entered from or left towards
 *   a window which is neither an ancestor nor an inferior.
 * @CDK_NOTIFY_NONLINEAR_VIRTUAL: the pointer moves between two windows
 *   which are not ancestors of each other and the window is part of
 *   the ancestor chain between one of these windows and their least
 *   common ancestor.
 * @CDK_NOTIFY_UNKNOWN: an unknown type of enter/leave event occurred.
 *
 * Specifies the kind of crossing for #CdkEventCrossing.
 *
 * See the X11 protocol specification of LeaveNotify for
 * full details of crossing event generation.
 */
typedef enum
{
  CDK_NOTIFY_ANCESTOR		= 0,
  CDK_NOTIFY_VIRTUAL		= 1,
  CDK_NOTIFY_INFERIOR		= 2,
  CDK_NOTIFY_NONLINEAR		= 3,
  CDK_NOTIFY_NONLINEAR_VIRTUAL	= 4,
  CDK_NOTIFY_UNKNOWN		= 5
} CdkNotifyType;

/**
 * CdkCrossingMode:
 * @CDK_CROSSING_NORMAL: crossing because of pointer motion.
 * @CDK_CROSSING_GRAB: crossing because a grab is activated.
 * @CDK_CROSSING_UNGRAB: crossing because a grab is deactivated.
 * @CDK_CROSSING_CTK_GRAB: crossing because a CTK+ grab is activated.
 * @CDK_CROSSING_CTK_UNGRAB: crossing because a CTK+ grab is deactivated.
 * @CDK_CROSSING_STATE_CHANGED: crossing because a CTK+ widget changed
 *   state (e.g. sensitivity).
 * @CDK_CROSSING_TOUCH_BEGIN: crossing because a touch sequence has begun,
 *   this event is synthetic as the pointer might have not left the window.
 * @CDK_CROSSING_TOUCH_END: crossing because a touch sequence has ended,
 *   this event is synthetic as the pointer might have not left the window.
 * @CDK_CROSSING_DEVICE_SWITCH: crossing because of a device switch (i.e.
 *   a mouse taking control of the pointer after a touch device), this event
 *   is synthetic as the pointer didn’t leave the window.
 *
 * Specifies the crossing mode for #CdkEventCrossing.
 */
typedef enum
{
  CDK_CROSSING_NORMAL,
  CDK_CROSSING_GRAB,
  CDK_CROSSING_UNGRAB,
  CDK_CROSSING_CTK_GRAB,
  CDK_CROSSING_CTK_UNGRAB,
  CDK_CROSSING_STATE_CHANGED,
  CDK_CROSSING_TOUCH_BEGIN,
  CDK_CROSSING_TOUCH_END,
  CDK_CROSSING_DEVICE_SWITCH
} CdkCrossingMode;

/**
 * CdkPropertyState:
 * @CDK_PROPERTY_NEW_VALUE: the property value was changed.
 * @CDK_PROPERTY_DELETE: the property was deleted.
 *
 * Specifies the type of a property change for a #CdkEventProperty.
 */
typedef enum
{
  CDK_PROPERTY_NEW_VALUE,
  CDK_PROPERTY_DELETE
} CdkPropertyState;

/**
 * CdkWindowState:
 * @CDK_WINDOW_STATE_WITHDRAWN: the window is not shown.
 * @CDK_WINDOW_STATE_ICONIFIED: the window is minimized.
 * @CDK_WINDOW_STATE_MAXIMIZED: the window is maximized.
 * @CDK_WINDOW_STATE_STICKY: the window is sticky.
 * @CDK_WINDOW_STATE_FULLSCREEN: the window is maximized without
 *   decorations.
 * @CDK_WINDOW_STATE_ABOVE: the window is kept above other windows.
 * @CDK_WINDOW_STATE_BELOW: the window is kept below other windows.
 * @CDK_WINDOW_STATE_FOCUSED: the window is presented as focused (with active decorations).
 * @CDK_WINDOW_STATE_TILED: the window is in a tiled state, Since 3.10. Since 3.22.23, this
 *                          is deprecated in favor of per-edge information.
 * @CDK_WINDOW_STATE_TOP_TILED: whether the top edge is tiled, Since 3.22.23
 * @CDK_WINDOW_STATE_TOP_RESIZABLE: whether the top edge is resizable, Since 3.22.23
 * @CDK_WINDOW_STATE_RIGHT_TILED: whether the right edge is tiled, Since 3.22.23
 * @CDK_WINDOW_STATE_RIGHT_RESIZABLE: whether the right edge is resizable, Since 3.22.23
 * @CDK_WINDOW_STATE_BOTTOM_TILED: whether the bottom edge is tiled, Since 3.22.23
 * @CDK_WINDOW_STATE_BOTTOM_RESIZABLE: whether the bottom edge is resizable, Since 3.22.23
 * @CDK_WINDOW_STATE_LEFT_TILED: whether the left edge is tiled, Since 3.22.23
 * @CDK_WINDOW_STATE_LEFT_RESIZABLE: whether the left edge is resizable, Since 3.22.23
 *
 * Specifies the state of a toplevel window.
 */
typedef enum
{
  CDK_WINDOW_STATE_WITHDRAWN        = 1 << 0,
  CDK_WINDOW_STATE_ICONIFIED        = 1 << 1,
  CDK_WINDOW_STATE_MAXIMIZED        = 1 << 2,
  CDK_WINDOW_STATE_STICKY           = 1 << 3,
  CDK_WINDOW_STATE_FULLSCREEN       = 1 << 4,
  CDK_WINDOW_STATE_ABOVE            = 1 << 5,
  CDK_WINDOW_STATE_BELOW            = 1 << 6,
  CDK_WINDOW_STATE_FOCUSED          = 1 << 7,
  CDK_WINDOW_STATE_TILED            = 1 << 8,
  CDK_WINDOW_STATE_TOP_TILED        = 1 << 9,
  CDK_WINDOW_STATE_TOP_RESIZABLE    = 1 << 10,
  CDK_WINDOW_STATE_RIGHT_TILED      = 1 << 11,
  CDK_WINDOW_STATE_RIGHT_RESIZABLE  = 1 << 12,
  CDK_WINDOW_STATE_BOTTOM_TILED     = 1 << 13,
  CDK_WINDOW_STATE_BOTTOM_RESIZABLE = 1 << 14,
  CDK_WINDOW_STATE_LEFT_TILED       = 1 << 15,
  CDK_WINDOW_STATE_LEFT_RESIZABLE   = 1 << 16
} CdkWindowState;

/**
 * CdkSettingAction:
 * @CDK_SETTING_ACTION_NEW: a setting was added.
 * @CDK_SETTING_ACTION_CHANGED: a setting was changed.
 * @CDK_SETTING_ACTION_DELETED: a setting was deleted.
 *
 * Specifies the kind of modification applied to a setting in a
 * #CdkEventSetting.
 */
typedef enum
{
  CDK_SETTING_ACTION_NEW,
  CDK_SETTING_ACTION_CHANGED,
  CDK_SETTING_ACTION_DELETED
} CdkSettingAction;

/**
 * CdkOwnerChange:
 * @CDK_OWNER_CHANGE_NEW_OWNER: some other app claimed the ownership
 * @CDK_OWNER_CHANGE_DESTROY: the window was destroyed
 * @CDK_OWNER_CHANGE_CLOSE: the client was closed
 *
 * Specifies why a selection ownership was changed.
 */
typedef enum
{
  CDK_OWNER_CHANGE_NEW_OWNER,
  CDK_OWNER_CHANGE_DESTROY,
  CDK_OWNER_CHANGE_CLOSE
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
 * @type: the type of the event (%CDK_EXPOSE or %CDK_DAMAGE).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @area: bounding box of @region.
 * @region: the region that needs to be redrawn.
 * @count: the number of contiguous %CDK_EXPOSE events following this one.
 *   The only use for this is “exposure compression”, i.e. handling all
 *   contiguous %CDK_EXPOSE events in one go, though CDK performs some
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
 * @type: the type of the event (%CDK_VISIBILITY_NOTIFY).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @state: the new visibility state (%CDK_VISIBILITY_FULLY_OBSCURED,
 *   %CDK_VISIBILITY_PARTIAL or %CDK_VISIBILITY_UNOBSCURED).
 *
 * Generated when the window visibility status has changed.
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
 *   %CDK_POINTER_MOTION_HINT_MASK value of #CdkEventMask.
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
 * @type: the type of the event (%CDK_BUTTON_PRESS, %CDK_2BUTTON_PRESS,
 *   %CDK_3BUTTON_PRESS or %CDK_BUTTON_RELEASE).
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
 * @type field will be one of %CDK_BUTTON_PRESS,
 * %CDK_2BUTTON_PRESS, %CDK_3BUTTON_PRESS or %CDK_BUTTON_RELEASE,
 *
 * Double and triple-clicks result in a sequence of events being received.
 * For double-clicks the order of events will be:
 *
 * - %CDK_BUTTON_PRESS
 * - %CDK_BUTTON_RELEASE
 * - %CDK_BUTTON_PRESS
 * - %CDK_2BUTTON_PRESS
 * - %CDK_BUTTON_RELEASE
 *
 * Note that the first click is received just like a normal
 * button press, while the second click results in a %CDK_2BUTTON_PRESS
 * being received just after the %CDK_BUTTON_PRESS.
 *
 * Triple-clicks are very similar to double-clicks, except that
 * %CDK_3BUTTON_PRESS is inserted after the third click. The order of the
 * events is:
 *
 * - %CDK_BUTTON_PRESS
 * - %CDK_BUTTON_RELEASE
 * - %CDK_BUTTON_PRESS
 * - %CDK_2BUTTON_PRESS
 * - %CDK_BUTTON_RELEASE
 * - %CDK_BUTTON_PRESS
 * - %CDK_3BUTTON_PRESS
 * - %CDK_BUTTON_RELEASE
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
 * @type: the type of the event (%CDK_TOUCH_BEGIN, %CDK_TOUCH_UPDATE,
 *   %CDK_TOUCH_END, %CDK_TOUCH_CANCEL)
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
 * @type field will be one of %CDK_TOUCH_BEGIN, %CDK_TOUCH_UPDATE,
 * %CDK_TOUCH_END or %CDK_TOUCH_CANCEL.
 *
 * Touch events are grouped into sequences by means of the @sequence
 * field, which can also be obtained with cdk_event_get_event_sequence().
 * Each sequence begins with a %CDK_TOUCH_BEGIN event, followed by
 * any number of %CDK_TOUCH_UPDATE events, and ends with a %CDK_TOUCH_END
 * (or %CDK_TOUCH_CANCEL) event. With multitouch devices, there may be
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
 * @type: the type of the event (%CDK_SCROLL).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @time: the time of the event in milliseconds.
 * @x: the x coordinate of the pointer relative to the window.
 * @y: the y coordinate of the pointer relative to the window.
 * @state: (type CdkModifierType): a bit-mask representing the state of
 *   the modifier keys (e.g. Control, Shift and Alt) and the pointer
 *   buttons. See #CdkModifierType.
 * @direction: the direction to scroll to (one of %CDK_SCROLL_UP,
 *   %CDK_SCROLL_DOWN, %CDK_SCROLL_LEFT, %CDK_SCROLL_RIGHT or
 *   %CDK_SCROLL_SMOOTH).
 * @device: the master device that the event originated from. Use
 * cdk_event_get_source_device() to get the slave device.
 * @x_root: the x coordinate of the pointer relative to the root of the
 *   screen.
 * @y_root: the y coordinate of the pointer relative to the root of the
 *   screen.
 * @delta_x: the x coordinate of the scroll delta
 * @delta_y: the y coordinate of the scroll delta
 * @is_stop: %TRUE if the event is a scroll stop event
 *
 * Generated from button presses for the buttons 4 to 7. Wheel mice are
 * usually configured to generate button press events for buttons 4 and 5
 * when the wheel is turned.
 *
 * Some CDK backends can also generate “smooth” scroll events, which
 * can be recognized by the %CDK_SCROLL_SMOOTH scroll direction. For
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
 * @type: the type of the event (%CDK_KEY_PRESS or %CDK_KEY_RELEASE).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @time: the time of the event in milliseconds.
 * @state: (type CdkModifierType): a bit-mask representing the state of
 *   the modifier keys (e.g. Control, Shift and Alt) and the pointer
 *   buttons. See #CdkModifierType.
 * @keyval: the key that was pressed or released. See the
 *   `cdk/cdkkeysyms.h` header file for a
 *   complete list of CDK key codes.
 * @length: the length of @string.
 * @string: a string containing an approximation of the text that
 *   would result from this keypress. The only correct way to handle text
 *   input of text is using input methods (see #CtkIMContext), so this
 *   field is deprecated and should never be used.
 *   (cdk_unicode_to_keyval() provides a non-deprecated way of getting
 *   an approximate translation for a key.) The string is encoded in the
 *   encoding of the current locale (Note: this for backwards compatibility:
 *   strings in CTK+ and CDK are typically in UTF-8.) and NUL-terminated.
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
 * @type: the type of the event (%CDK_ENTER_NOTIFY or %CDK_LEAVE_NOTIFY).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @subwindow: the window that was entered or left.
 * @time: the time of the event in milliseconds.
 * @x: the x coordinate of the pointer relative to the window.
 * @y: the y coordinate of the pointer relative to the window.
 * @x_root: the x coordinate of the pointer relative to the root of the screen.
 * @y_root: the y coordinate of the pointer relative to the root of the screen.
 * @mode: the crossing mode (%CDK_CROSSING_NORMAL, %CDK_CROSSING_GRAB,
 *  %CDK_CROSSING_UNGRAB, %CDK_CROSSING_CTK_GRAB, %CDK_CROSSING_CTK_UNGRAB or
 *  %CDK_CROSSING_STATE_CHANGED).  %CDK_CROSSING_CTK_GRAB, %CDK_CROSSING_CTK_UNGRAB,
 *  and %CDK_CROSSING_STATE_CHANGED were added in 2.14 and are always synthesized,
 *  never native.
 * @detail: the kind of crossing that happened (%CDK_NOTIFY_INFERIOR,
 *  %CDK_NOTIFY_ANCESTOR, %CDK_NOTIFY_VIRTUAL, %CDK_NOTIFY_NONLINEAR or
 *  %CDK_NOTIFY_NONLINEAR_VIRTUAL).
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
 * @type: the type of the event (%CDK_FOCUS_CHANGE).
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
 * @type: the type of the event (%CDK_CONFIGURE).
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
 * @type: the type of the event (%CDK_PROPERTY_NOTIFY).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @atom: the property that was changed.
 * @time: the time of the event in milliseconds.
 * @state: (type CdkPropertyState): whether the property was changed
 *   (%CDK_PROPERTY_NEW_VALUE) or deleted (%CDK_PROPERTY_DELETE).
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
 * @type: the type of the event (%CDK_SELECTION_CLEAR,
 *   %CDK_SELECTION_NOTIFY or %CDK_SELECTION_REQUEST).
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
 * @type: the type of the event (%CDK_OWNER_CHANGE).
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
 * @type: the type of the event (%CDK_PROXIMITY_IN or %CDK_PROXIMITY_OUT).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @time: the time of the event in milliseconds.
 * @device: the master device that the event originated from. Use
 * cdk_event_get_source_device() to get the slave device.
 *
 * Proximity events are generated when using CDK’s wrapper for the
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
 * @type: the type of the event (%CDK_SETTING).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @action: what happened to the setting (%CDK_SETTING_ACTION_NEW,
 *   %CDK_SETTING_ACTION_CHANGED or %CDK_SETTING_ACTION_DELETED).
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
 * @type: the type of the event (%CDK_WINDOW_STATE).
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
 * @type: the type of the event (%CDK_GRAB_BROKEN)
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
 * @type: the type of the event (%CDK_DRAG_ENTER, %CDK_DRAG_LEAVE,
 *   %CDK_DRAG_MOTION, %CDK_DRAG_STATUS, %CDK_DROP_START or
 *   %CDK_DROP_FINISHED).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @context: the #CdkDragContext for the current DND operation.
 * @time: the time of the event in milliseconds.
 * @x_root: the x coordinate of the pointer relative to the root of the
 *   screen, only set for %CDK_DRAG_MOTION and %CDK_DROP_START.
 * @y_root: the y coordinate of the pointer relative to the root of the
 *   screen, only set for %CDK_DRAG_MOTION and %CDK_DROP_START.
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
 * @type: the type of the event (%CDK_TOUCHPAD_SWIPE)
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
 * @type: the type of the event (%CDK_TOUCHPAD_PINCH)
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
 *   the corresponding %CDK_TOUCHPAD_GESTURE_PHASE_BEGIN event
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
 * @type: the type of the event (%CDK_PAD_BUTTON_PRESS or %CDK_PAD_BUTTON_RELEASE).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @time: the time of the event in milliseconds.
 * @group: the pad group the button belongs to. A %CDK_SOURCE_TABLET_PAD device
 *   may have one or more groups containing a set of buttons/rings/strips each.
 * @button: The pad button that was pressed.
 * @mode: The current mode of @group. Different groups in a %CDK_SOURCE_TABLET_PAD
 *   device may have different current modes.
 *
 * Generated during %CDK_SOURCE_TABLET_PAD button presses and releases.
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
 * @type: the type of the event (%CDK_PAD_RING or %CDK_PAD_STRIP).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @time: the time of the event in milliseconds.
 * @group: the pad group the ring/strip belongs to. A %CDK_SOURCE_TABLET_PAD
 *   device may have one or more groups containing a set of buttons/rings/strips
 *   each.
 * @index: number of strip/ring that was interacted. This number is 0-indexed.
 * @mode: The current mode of @group. Different groups in a %CDK_SOURCE_TABLET_PAD
 *   device may have different current modes.
 * @value: The current value for the given axis.
 *
 * Generated during %CDK_SOURCE_TABLET_PAD interaction with tactile sensors.
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
 * @type: the type of the event (%CDK_PAD_GROUP_MODE).
 * @window: the window which received the event.
 * @send_event: %TRUE if the event was sent explicitly.
 * @time: the time of the event in milliseconds.
 * @group: the pad group that is switching mode. A %CDK_SOURCE_TABLET_PAD
 *   device may have one or more groups containing a set of buttons/rings/strips
 *   each.
 * @mode: The new mode of @group. Different groups in a %CDK_SOURCE_TABLET_PAD
 *   device may have different current modes.
 *
 * Generated during %CDK_SOURCE_TABLET_PAD mode switches in a group.
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
 * name can be used. For example if the event type is %CDK_BUTTON_PRESS
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

CDK_AVAILABLE_IN_ALL
GType     cdk_event_get_type            (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_14
GType     cdk_event_sequence_get_type   (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
gboolean  cdk_events_pending	 	(void);
CDK_AVAILABLE_IN_ALL
CdkEvent* cdk_event_get			(void);

CDK_AVAILABLE_IN_ALL
CdkEvent* cdk_event_peek                (void);
CDK_AVAILABLE_IN_ALL
void      cdk_event_put	 		(const CdkEvent *event);

CDK_AVAILABLE_IN_ALL
CdkEvent* cdk_event_new                 (CdkEventType    type);
CDK_AVAILABLE_IN_ALL
CdkEvent* cdk_event_copy     		(const CdkEvent *event);
CDK_AVAILABLE_IN_ALL
void	  cdk_event_free     		(CdkEvent 	*event);

CDK_AVAILABLE_IN_3_10
CdkWindow *cdk_event_get_window         (const CdkEvent *event);

CDK_AVAILABLE_IN_ALL
guint32   cdk_event_get_time            (const CdkEvent  *event);
CDK_AVAILABLE_IN_ALL
gboolean  cdk_event_get_state           (const CdkEvent  *event,
                                         CdkModifierType *state);
CDK_AVAILABLE_IN_ALL
gboolean  cdk_event_get_coords		(const CdkEvent  *event,
					 gdouble	 *x_win,
					 gdouble	 *y_win);
CDK_AVAILABLE_IN_ALL
gboolean  cdk_event_get_root_coords	(const CdkEvent *event,
					 gdouble	*x_root,
					 gdouble	*y_root);
CDK_AVAILABLE_IN_3_2
gboolean  cdk_event_get_button          (const CdkEvent *event,
                                         guint          *button);
CDK_AVAILABLE_IN_3_2
gboolean  cdk_event_get_click_count     (const CdkEvent *event,
                                         guint          *click_count);
CDK_AVAILABLE_IN_3_2
gboolean  cdk_event_get_keyval          (const CdkEvent *event,
                                         guint          *keyval);
CDK_AVAILABLE_IN_3_2
gboolean  cdk_event_get_keycode         (const CdkEvent *event,
                                         guint16        *keycode);
CDK_AVAILABLE_IN_3_2
gboolean cdk_event_get_scroll_direction (const CdkEvent *event,
                                         CdkScrollDirection *direction);
CDK_AVAILABLE_IN_3_4
gboolean  cdk_event_get_scroll_deltas   (const CdkEvent *event,
                                         gdouble         *delta_x,
                                         gdouble         *delta_y);

CDK_AVAILABLE_IN_3_20
gboolean  cdk_event_is_scroll_stop_event (const CdkEvent *event);

CDK_AVAILABLE_IN_ALL
gboolean  cdk_event_get_axis            (const CdkEvent  *event,
                                         CdkAxisUse       axis_use,
                                         gdouble         *value);
CDK_AVAILABLE_IN_ALL
void       cdk_event_set_device         (CdkEvent        *event,
                                         CdkDevice       *device);
CDK_AVAILABLE_IN_ALL
CdkDevice* cdk_event_get_device         (const CdkEvent  *event);
CDK_AVAILABLE_IN_ALL
void       cdk_event_set_source_device  (CdkEvent        *event,
                                         CdkDevice       *device);
CDK_AVAILABLE_IN_ALL
CdkDevice* cdk_event_get_source_device  (const CdkEvent  *event);
CDK_AVAILABLE_IN_ALL
void       cdk_event_request_motions    (const CdkEventMotion *event);
CDK_AVAILABLE_IN_3_4
gboolean   cdk_event_triggers_context_menu (const CdkEvent *event);

CDK_AVAILABLE_IN_ALL
gboolean  cdk_events_get_distance       (CdkEvent        *event1,
                                         CdkEvent        *event2,
                                         gdouble         *distance);
CDK_AVAILABLE_IN_ALL
gboolean  cdk_events_get_angle          (CdkEvent        *event1,
                                         CdkEvent        *event2,
                                         gdouble         *angle);
CDK_AVAILABLE_IN_ALL
gboolean  cdk_events_get_center         (CdkEvent        *event1,
                                         CdkEvent        *event2,
                                         gdouble         *x,
                                         gdouble         *y);

CDK_AVAILABLE_IN_ALL
void	  cdk_event_handler_set 	(CdkEventFunc    func,
					 gpointer        data,
					 GDestroyNotify  notify);

CDK_AVAILABLE_IN_ALL
void       cdk_event_set_screen         (CdkEvent        *event,
                                         CdkScreen       *screen);
CDK_AVAILABLE_IN_ALL
CdkScreen *cdk_event_get_screen         (const CdkEvent  *event);

CDK_AVAILABLE_IN_3_4
CdkEventSequence *cdk_event_get_event_sequence (const CdkEvent *event);

CDK_AVAILABLE_IN_3_10
CdkEventType cdk_event_get_event_type   (const CdkEvent *event);

CDK_AVAILABLE_IN_3_20
CdkSeat  *cdk_event_get_seat            (const CdkEvent *event);

CDK_AVAILABLE_IN_ALL
void	  cdk_set_show_events		(gboolean	 show_events);
CDK_AVAILABLE_IN_ALL
gboolean  cdk_get_show_events		(void);

CDK_AVAILABLE_IN_ALL
gboolean cdk_setting_get                (const gchar    *name,
                                         GValue         *value);

CDK_AVAILABLE_IN_3_22
CdkDeviceTool *cdk_event_get_device_tool (const CdkEvent *event);

CDK_AVAILABLE_IN_3_22
void           cdk_event_set_device_tool (CdkEvent       *event,
                                          CdkDeviceTool  *tool);

CDK_AVAILABLE_IN_3_22
int            cdk_event_get_scancode    (CdkEvent *event);

CDK_AVAILABLE_IN_3_22
gboolean       cdk_event_get_pointer_emulated (CdkEvent *event);

G_END_DECLS

#endif /* __CDK_EVENTS_H__ */
