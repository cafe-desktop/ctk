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

#ifndef __CDK_TYPES_H__
#define __CDK_TYPES_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

/* CDK uses "glib". (And so does CTK).
 */
#include <glib.h>
#include <pango/pango.h>
#include <glib-object.h>
#include <cairo.h>

#if !GLIB_CHECK_VERSION(2,67,3)
#if !defined g_memdup2
#define g_memdup2 g_memdup
#endif
#endif

#if !GLIB_CHECK_VERSION(2,69,0)
#if !defined g_pattern_spec_match_string
#define g_pattern_spec_match_string g_pattern_match_string
#endif
#endif

/* The system specific file cdkconfig.h contains such configuration
 * settings that are needed not only when compiling CDK (or CTK)
 * itself, but also occasionally when compiling programs that use CDK
 * (or CTK). One such setting is what windowing API backend is in use.
 */
#include <cdk/cdkconfig.h>

/* some common magic values */

/**
 * CDK_CURRENT_TIME:
 *
 * Represents the current time, and can be used anywhere a time is expected.
 */
#define CDK_CURRENT_TIME     0L

/**
 * CDK_PARENT_RELATIVE:
 *
 * A special value, indicating that the background
 * for a window should be inherited from the parent window.
 */
#define CDK_PARENT_RELATIVE  1L



G_BEGIN_DECLS


/* Type definitions for the basic structures.
 */
typedef struct _CdkPoint              CdkPoint;

/**
 * CdkRectangle:
 * @x: X coordinate of the left side of the rectangle
 * @y: Y coordinate of the the top side of the rectangle
 * @width: width of the rectangle
 * @height: height of the rectangle
 *
 * Defines the position and size of a rectangle. It is identical to
 * #cairo_rectangle_int_t.
 */
#ifdef __GI_SCANNER__
/* The introspection scanner is currently unable to lookup how
 * cairo_rectangle_int_t is actually defined. This prevents
 * introspection data for the CdkRectangle type to include fields
 * descriptions. To workaround this issue, we define it with the same
 * content as cairo_rectangle_int_t, but only under the introspection
 * define.
 */
struct _CdkRectangle
{
    int x, y;
    int width, height;
};
typedef struct _CdkRectangle          CdkRectangle;
#else
typedef cairo_rectangle_int_t         CdkRectangle;
#endif

/**
 * CdkAtom:
 *
 * An opaque type representing a string as an index into a table
 * of strings on the X server.
 */
typedef struct _CdkAtom            *CdkAtom;

/**
 * CDK_ATOM_TO_POINTER:
 * @atom: a #CdkAtom.
 *
 * Converts a #CdkAtom into a pointer type.
 */
#define CDK_ATOM_TO_POINTER(atom) (atom)

/**
 * CDK_POINTER_TO_ATOM:
 * @ptr: a pointer containing a #CdkAtom.
 *
 * Extracts a #CdkAtom from a pointer. The #CdkAtom must have been
 * stored in the pointer with CDK_ATOM_TO_POINTER().
 */
#define CDK_POINTER_TO_ATOM(ptr)  ((CdkAtom)(ptr))

#define _CDK_MAKE_ATOM(val) ((CdkAtom)GUINT_TO_POINTER(val))

/**
 * CDK_NONE:
 *
 * A null value for #CdkAtom, used in a similar way as
 * `None` in the Xlib API.
 */
#define CDK_NONE            _CDK_MAKE_ATOM (0)

/* Forward declarations of commonly used types */
typedef struct _CdkColor              CdkColor;
typedef struct _CdkRGBA               CdkRGBA;
typedef struct _CdkCursor             CdkCursor;
typedef struct _CdkVisual             CdkVisual;
typedef struct _CdkDevice             CdkDevice;
typedef struct _CdkDragContext        CdkDragContext;

typedef struct _CdkDisplayManager     CdkDisplayManager;
typedef struct _CdkDeviceManager      CdkDeviceManager;
typedef struct _CdkDisplay            CdkDisplay;
typedef struct _CdkScreen             CdkScreen;
typedef struct _CdkWindow             CdkWindow;
typedef struct _CdkKeymap             CdkKeymap;
typedef struct _CdkAppLaunchContext   CdkAppLaunchContext;
typedef struct _CdkSeat               CdkSeat;

typedef struct _CdkGLContext          CdkGLContext;

/**
 * CdkByteOrder:
 * @CDK_LSB_FIRST: The values are stored with the least-significant byte
 *   first. For instance, the 32-bit value 0xffeecc would be stored
 *   in memory as 0xcc, 0xee, 0xff, 0x00.
 * @CDK_MSB_FIRST: The values are stored with the most-significant byte
 *   first. For instance, the 32-bit value 0xffeecc would be stored
 *   in memory as 0x00, 0xff, 0xee, 0xcc.
 *
 * A set of values describing the possible byte-orders
 * for storing pixel values in memory.
 */
typedef enum
{
  CDK_LSB_FIRST,
  CDK_MSB_FIRST
} CdkByteOrder;

/* Types of modifiers.
 */
/**
 * CdkModifierType:
 * @CDK_SHIFT_MASK: the Shift key.
 * @CDK_LOCK_MASK: a Lock key (depending on the modifier mapping of the
 *  X server this may either be CapsLock or ShiftLock).
 * @CDK_CONTROL_MASK: the Control key.
 * @CDK_MOD1_MASK: the fourth modifier key (it depends on the modifier
 *  mapping of the X server which key is interpreted as this modifier, but
 *  normally it is the Alt key).
 * @CDK_MOD2_MASK: the fifth modifier key (it depends on the modifier
 *  mapping of the X server which key is interpreted as this modifier).
 * @CDK_MOD3_MASK: the sixth modifier key (it depends on the modifier
 *  mapping of the X server which key is interpreted as this modifier).
 * @CDK_MOD4_MASK: the seventh modifier key (it depends on the modifier
 *  mapping of the X server which key is interpreted as this modifier).
 * @CDK_MOD5_MASK: the eighth modifier key (it depends on the modifier
 *  mapping of the X server which key is interpreted as this modifier).
 * @CDK_BUTTON1_MASK: the first mouse button.
 * @CDK_BUTTON2_MASK: the second mouse button.
 * @CDK_BUTTON3_MASK: the third mouse button.
 * @CDK_BUTTON4_MASK: the fourth mouse button.
 * @CDK_BUTTON5_MASK: the fifth mouse button.
 * @CDK_MODIFIER_RESERVED_13_MASK: A reserved bit flag; do not use in your own code
 * @CDK_MODIFIER_RESERVED_14_MASK: A reserved bit flag; do not use in your own code
 * @CDK_MODIFIER_RESERVED_15_MASK: A reserved bit flag; do not use in your own code
 * @CDK_MODIFIER_RESERVED_16_MASK: A reserved bit flag; do not use in your own code
 * @CDK_MODIFIER_RESERVED_17_MASK: A reserved bit flag; do not use in your own code
 * @CDK_MODIFIER_RESERVED_18_MASK: A reserved bit flag; do not use in your own code
 * @CDK_MODIFIER_RESERVED_19_MASK: A reserved bit flag; do not use in your own code
 * @CDK_MODIFIER_RESERVED_20_MASK: A reserved bit flag; do not use in your own code
 * @CDK_MODIFIER_RESERVED_21_MASK: A reserved bit flag; do not use in your own code
 * @CDK_MODIFIER_RESERVED_22_MASK: A reserved bit flag; do not use in your own code
 * @CDK_MODIFIER_RESERVED_23_MASK: A reserved bit flag; do not use in your own code
 * @CDK_MODIFIER_RESERVED_24_MASK: A reserved bit flag; do not use in your own code
 * @CDK_MODIFIER_RESERVED_25_MASK: A reserved bit flag; do not use in your own code
 * @CDK_SUPER_MASK: the Super modifier. Since 2.10
 * @CDK_HYPER_MASK: the Hyper modifier. Since 2.10
 * @CDK_META_MASK: the Meta modifier. Since 2.10
 * @CDK_MODIFIER_RESERVED_29_MASK: A reserved bit flag; do not use in your own code
 * @CDK_RELEASE_MASK: not used in CDK itself. CTK+ uses it to differentiate
 *  between (keyval, modifiers) pairs from key press and release events.
 * @CDK_MODIFIER_MASK: a mask covering all modifier types.
 *
 * A set of bit-flags to indicate the state of modifier keys and mouse buttons
 * in various event types. Typical modifier keys are Shift, Control, Meta,
 * Super, Hyper, Alt, Compose, Apple, CapsLock or ShiftLock.
 *
 * Like the X Window System, CDK supports 8 modifier keys and 5 mouse buttons.
 *
 * Since 2.10, CDK recognizes which of the Meta, Super or Hyper keys are mapped
 * to Mod2 - Mod5, and indicates this by setting %CDK_SUPER_MASK,
 * %CDK_HYPER_MASK or %CDK_META_MASK in the state field of key events.
 *
 * Note that CDK may add internal values to events which include
 * reserved values such as %CDK_MODIFIER_RESERVED_13_MASK.  Your code
 * should preserve and ignore them.  You can use %CDK_MODIFIER_MASK to
 * remove all reserved values.
 *
 * Also note that the CDK X backend interprets button press events for button
 * 4-7 as scroll events, so %CDK_BUTTON4_MASK and %CDK_BUTTON5_MASK will never
 * be set.
 */
typedef enum
{
  CDK_SHIFT_MASK    = 1 << 0,
  CDK_LOCK_MASK     = 1 << 1,
  CDK_CONTROL_MASK  = 1 << 2,
  CDK_MOD1_MASK     = 1 << 3,
  CDK_MOD2_MASK     = 1 << 4,
  CDK_MOD3_MASK     = 1 << 5,
  CDK_MOD4_MASK     = 1 << 6,
  CDK_MOD5_MASK     = 1 << 7,
  CDK_BUTTON1_MASK  = 1 << 8,
  CDK_BUTTON2_MASK  = 1 << 9,
  CDK_BUTTON3_MASK  = 1 << 10,
  CDK_BUTTON4_MASK  = 1 << 11,
  CDK_BUTTON5_MASK  = 1 << 12,

  CDK_MODIFIER_RESERVED_13_MASK  = 1 << 13,
  CDK_MODIFIER_RESERVED_14_MASK  = 1 << 14,
  CDK_MODIFIER_RESERVED_15_MASK  = 1 << 15,
  CDK_MODIFIER_RESERVED_16_MASK  = 1 << 16,
  CDK_MODIFIER_RESERVED_17_MASK  = 1 << 17,
  CDK_MODIFIER_RESERVED_18_MASK  = 1 << 18,
  CDK_MODIFIER_RESERVED_19_MASK  = 1 << 19,
  CDK_MODIFIER_RESERVED_20_MASK  = 1 << 20,
  CDK_MODIFIER_RESERVED_21_MASK  = 1 << 21,
  CDK_MODIFIER_RESERVED_22_MASK  = 1 << 22,
  CDK_MODIFIER_RESERVED_23_MASK  = 1 << 23,
  CDK_MODIFIER_RESERVED_24_MASK  = 1 << 24,
  CDK_MODIFIER_RESERVED_25_MASK  = 1 << 25,

  /* The next few modifiers are used by XKB, so we skip to the end.
   * Bits 15 - 25 are currently unused. Bit 29 is used internally.
   */
  
  CDK_SUPER_MASK    = 1 << 26,
  CDK_HYPER_MASK    = 1 << 27,
  CDK_META_MASK     = 1 << 28,
  
  CDK_MODIFIER_RESERVED_29_MASK  = 1 << 29,

  CDK_RELEASE_MASK  = 1 << 30,

  /* Combination of CDK_SHIFT_MASK..CDK_BUTTON5_MASK + CDK_SUPER_MASK
     + CDK_HYPER_MASK + CDK_META_MASK + CDK_RELEASE_MASK */
  CDK_MODIFIER_MASK = 0x5c001fff
} CdkModifierType;

/**
 * CdkModifierIntent:
 * @CDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR: the primary modifier used to invoke
 *  menu accelerators.
 * @CDK_MODIFIER_INTENT_CONTEXT_MENU: the modifier used to invoke context menus.
 *  Note that mouse button 3 always triggers context menus. When this modifier
 *  is not 0, it additionally triggers context menus when used with mouse button 1.
 * @CDK_MODIFIER_INTENT_EXTEND_SELECTION: the modifier used to extend selections
 *  using `modifier`-click or `modifier`-cursor-key
 * @CDK_MODIFIER_INTENT_MODIFY_SELECTION: the modifier used to modify selections,
 *  which in most cases means toggling the clicked item into or out of the selection.
 * @CDK_MODIFIER_INTENT_NO_TEXT_INPUT: when any of these modifiers is pressed, the
 *  key event cannot produce a symbol directly. This is meant to be used for
 *  input methods, and for use cases like typeahead search.
 * @CDK_MODIFIER_INTENT_SHIFT_GROUP: the modifier that switches between keyboard
 *  groups (AltGr on X11/Windows and Option/Alt on OS X).
 * @CDK_MODIFIER_INTENT_DEFAULT_MOD_MASK: The set of modifier masks accepted
 * as modifiers in accelerators. Needed because Command is mapped to MOD2 on
 * OSX, which is widely used, but on X11 MOD2 is NumLock and using that for a
 * mod key is problematic at best.
 * Ref: https://bugzilla.gnome.org/show_bug.cgi?id=736125.
 *
 * This enum is used with cdk_keymap_get_modifier_mask()
 * in order to determine what modifiers the
 * currently used windowing system backend uses for particular
 * purposes. For example, on X11/Windows, the Control key is used for
 * invoking menu shortcuts (accelerators), whereas on Apple computers
 * it’s the Command key (which correspond to %CDK_CONTROL_MASK and
 * %CDK_MOD2_MASK, respectively).
 *
 * Since: 3.4
 **/
typedef enum
{
  CDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR,
  CDK_MODIFIER_INTENT_CONTEXT_MENU,
  CDK_MODIFIER_INTENT_EXTEND_SELECTION,
  CDK_MODIFIER_INTENT_MODIFY_SELECTION,
  CDK_MODIFIER_INTENT_NO_TEXT_INPUT,
  CDK_MODIFIER_INTENT_SHIFT_GROUP,
  CDK_MODIFIER_INTENT_DEFAULT_MOD_MASK,
} CdkModifierIntent;

typedef enum
{
  CDK_OK          = 0,
  CDK_ERROR       = -1,
  CDK_ERROR_PARAM = -2,
  CDK_ERROR_FILE  = -3,
  CDK_ERROR_MEM   = -4
} CdkStatus;

/**
 * CdkGrabStatus:
 * @CDK_GRAB_SUCCESS: the resource was successfully grabbed.
 * @CDK_GRAB_ALREADY_GRABBED: the resource is actively grabbed by another client.
 * @CDK_GRAB_INVALID_TIME: the resource was grabbed more recently than the
 *  specified time.
 * @CDK_GRAB_NOT_VIEWABLE: the grab window or the @confine_to window are not
 *  viewable.
 * @CDK_GRAB_FROZEN: the resource is frozen by an active grab of another client.
 * @CDK_GRAB_FAILED: the grab failed for some other reason. Since 3.16
 *
 * Returned by cdk_device_grab(), cdk_pointer_grab() and cdk_keyboard_grab() to
 * indicate success or the reason for the failure of the grab attempt.
 */
typedef enum
{
  CDK_GRAB_SUCCESS         = 0,
  CDK_GRAB_ALREADY_GRABBED = 1,
  CDK_GRAB_INVALID_TIME    = 2,
  CDK_GRAB_NOT_VIEWABLE    = 3,
  CDK_GRAB_FROZEN          = 4,
  CDK_GRAB_FAILED          = 5
} CdkGrabStatus;

/**
 * CdkGrabOwnership:
 * @CDK_OWNERSHIP_NONE: All other devices’ events are allowed.
 * @CDK_OWNERSHIP_WINDOW: Other devices’ events are blocked for the grab window.
 * @CDK_OWNERSHIP_APPLICATION: Other devices’ events are blocked for the whole application.
 *
 * Defines how device grabs interact with other devices.
 */
typedef enum
{
  CDK_OWNERSHIP_NONE,
  CDK_OWNERSHIP_WINDOW,
  CDK_OWNERSHIP_APPLICATION
} CdkGrabOwnership;

/**
 * CdkEventMask:
 * @CDK_EXPOSURE_MASK: receive expose events
 * @CDK_POINTER_MOTION_MASK: receive all pointer motion events
 * @CDK_POINTER_MOTION_HINT_MASK: deprecated. see the explanation above
 * @CDK_BUTTON_MOTION_MASK: receive pointer motion events while any button is pressed
 * @CDK_BUTTON1_MOTION_MASK: receive pointer motion events while 1 button is pressed
 * @CDK_BUTTON2_MOTION_MASK: receive pointer motion events while 2 button is pressed
 * @CDK_BUTTON3_MOTION_MASK: receive pointer motion events while 3 button is pressed
 * @CDK_BUTTON_PRESS_MASK: receive button press events
 * @CDK_BUTTON_RELEASE_MASK: receive button release events
 * @CDK_KEY_PRESS_MASK: receive key press events
 * @CDK_KEY_RELEASE_MASK: receive key release events
 * @CDK_ENTER_NOTIFY_MASK: receive window enter events
 * @CDK_LEAVE_NOTIFY_MASK: receive window leave events
 * @CDK_FOCUS_CHANGE_MASK: receive focus change events
 * @CDK_STRUCTURE_MASK: receive events about window configuration change
 * @CDK_PROPERTY_CHANGE_MASK: receive property change events
 * @CDK_VISIBILITY_NOTIFY_MASK: receive visibility change events
 * @CDK_PROXIMITY_IN_MASK: receive proximity in events
 * @CDK_PROXIMITY_OUT_MASK: receive proximity out events
 * @CDK_SUBSTRUCTURE_MASK: receive events about window configuration changes of
 *   child windows
 * @CDK_SCROLL_MASK: receive scroll events
 * @CDK_TOUCH_MASK: receive touch events. Since 3.4
 * @CDK_SMOOTH_SCROLL_MASK: receive smooth scrolling events. Since 3.4
   @CDK_TOUCHPAD_GESTURE_MASK: receive touchpad gesture events. Since 3.18
 * @CDK_TABLET_PAD_MASK: receive tablet pad events. Since 3.22
 * @CDK_ALL_EVENTS_MASK: the combination of all the above event masks.
 *
 * A set of bit-flags to indicate which events a window is to receive.
 * Most of these masks map onto one or more of the #CdkEventType event types
 * above.
 *
 * See the [input handling overview][chap-input-handling] for details of
 * [event masks][event-masks] and [event propagation][event-propagation].
 *
 * %CDK_POINTER_MOTION_HINT_MASK is deprecated. It is a special mask
 * to reduce the number of %CDK_MOTION_NOTIFY events received. When using
 * %CDK_POINTER_MOTION_HINT_MASK, fewer %CDK_MOTION_NOTIFY events will
 * be sent, some of which are marked as a hint (the is_hint member is
 * %TRUE). To receive more motion events after a motion hint event,
 * the application needs to asks for more, by calling
 * cdk_event_request_motions().
 * 
 * Since CTK 3.8, motion events are already compressed by default, independent
 * of this mechanism. This compression can be disabled with
 * cdk_window_set_event_compression(). See the documentation of that function
 * for details.
 *
 * If %CDK_TOUCH_MASK is enabled, the window will receive touch events
 * from touch-enabled devices. Those will come as sequences of #CdkEventTouch
 * with type %CDK_TOUCH_UPDATE, enclosed by two events with
 * type %CDK_TOUCH_BEGIN and %CDK_TOUCH_END (or %CDK_TOUCH_CANCEL).
 * cdk_event_get_event_sequence() returns the event sequence for these
 * events, so different sequences may be distinguished.
 */
typedef enum
{
  CDK_EXPOSURE_MASK             = 1 << 1,
  CDK_POINTER_MOTION_MASK       = 1 << 2,
  CDK_POINTER_MOTION_HINT_MASK  = 1 << 3,
  CDK_BUTTON_MOTION_MASK        = 1 << 4,
  CDK_BUTTON1_MOTION_MASK       = 1 << 5,
  CDK_BUTTON2_MOTION_MASK       = 1 << 6,
  CDK_BUTTON3_MOTION_MASK       = 1 << 7,
  CDK_BUTTON_PRESS_MASK         = 1 << 8,
  CDK_BUTTON_RELEASE_MASK       = 1 << 9,
  CDK_KEY_PRESS_MASK            = 1 << 10,
  CDK_KEY_RELEASE_MASK          = 1 << 11,
  CDK_ENTER_NOTIFY_MASK         = 1 << 12,
  CDK_LEAVE_NOTIFY_MASK         = 1 << 13,
  CDK_FOCUS_CHANGE_MASK         = 1 << 14,
  CDK_STRUCTURE_MASK            = 1 << 15,
  CDK_PROPERTY_CHANGE_MASK      = 1 << 16,
  CDK_VISIBILITY_NOTIFY_MASK    = 1 << 17,
  CDK_PROXIMITY_IN_MASK         = 1 << 18,
  CDK_PROXIMITY_OUT_MASK        = 1 << 19,
  CDK_SUBSTRUCTURE_MASK         = 1 << 20,
  CDK_SCROLL_MASK               = 1 << 21,
  CDK_TOUCH_MASK                = 1 << 22,
  CDK_SMOOTH_SCROLL_MASK        = 1 << 23,
  CDK_TOUCHPAD_GESTURE_MASK     = 1 << 24,
  CDK_TABLET_PAD_MASK           = 1 << 25,
  CDK_ALL_EVENTS_MASK           = 0x3FFFFFE
} CdkEventMask;

/**
 * CdkPoint:
 * @x: the x coordinate of the point.
 * @y: the y coordinate of the point.
 *
 * Defines the x and y coordinates of a point.
 */
struct _CdkPoint
{
  gint x;
  gint y;
};

/**
 * CdkGLError:
 * @CDK_GL_ERROR_NOT_AVAILABLE: OpenGL support is not available
 * @CDK_GL_ERROR_UNSUPPORTED_FORMAT: The requested visual format is not supported
 * @CDK_GL_ERROR_UNSUPPORTED_PROFILE: The requested profile is not supported
 *
 * Error enumeration for #CdkGLContext.
 *
 * Since: 3.16
 */
typedef enum {
  CDK_GL_ERROR_NOT_AVAILABLE,
  CDK_GL_ERROR_UNSUPPORTED_FORMAT,
  CDK_GL_ERROR_UNSUPPORTED_PROFILE
} CdkGLError;

/**
 * CdkWindowTypeHint:
 * @CDK_WINDOW_TYPE_HINT_NORMAL: Normal toplevel window.
 * @CDK_WINDOW_TYPE_HINT_DIALOG: Dialog window.
 * @CDK_WINDOW_TYPE_HINT_MENU: Window used to implement a menu; CTK+ uses
 *  this hint only for torn-off menus, see #CtkTearoffMenuItem.
 * @CDK_WINDOW_TYPE_HINT_TOOLBAR: Window used to implement toolbars.
 * @CDK_WINDOW_TYPE_HINT_SPLASHSCREEN: Window used to display a splash
 *  screen during application startup.
 * @CDK_WINDOW_TYPE_HINT_UTILITY: Utility windows which are not detached
 *  toolbars or dialogs.
 * @CDK_WINDOW_TYPE_HINT_DOCK: Used for creating dock or panel windows.
 * @CDK_WINDOW_TYPE_HINT_DESKTOP: Used for creating the desktop background
 *  window.
 * @CDK_WINDOW_TYPE_HINT_DROPDOWN_MENU: A menu that belongs to a menubar.
 * @CDK_WINDOW_TYPE_HINT_POPUP_MENU: A menu that does not belong to a menubar,
 *  e.g. a context menu.
 * @CDK_WINDOW_TYPE_HINT_TOOLTIP: A tooltip.
 * @CDK_WINDOW_TYPE_HINT_NOTIFICATION: A notification - typically a “bubble”
 *  that belongs to a status icon.
 * @CDK_WINDOW_TYPE_HINT_COMBO: A popup from a combo box.
 * @CDK_WINDOW_TYPE_HINT_DND: A window that is used to implement a DND cursor.
 *
 * These are hints for the window manager that indicate what type of function
 * the window has. The window manager can use this when determining decoration
 * and behaviour of the window. The hint must be set before mapping the window.
 *
 * See the [Extended Window Manager Hints](http://www.freedesktop.org/Standards/wm-spec)
 * specification for more details about window types.
 */
typedef enum
{
  CDK_WINDOW_TYPE_HINT_NORMAL,
  CDK_WINDOW_TYPE_HINT_DIALOG,
  CDK_WINDOW_TYPE_HINT_MENU,		/* Torn off menu */
  CDK_WINDOW_TYPE_HINT_TOOLBAR,
  CDK_WINDOW_TYPE_HINT_SPLASHSCREEN,
  CDK_WINDOW_TYPE_HINT_UTILITY,
  CDK_WINDOW_TYPE_HINT_DOCK,
  CDK_WINDOW_TYPE_HINT_DESKTOP,
  CDK_WINDOW_TYPE_HINT_DROPDOWN_MENU,	/* A drop down menu (from a menubar) */
  CDK_WINDOW_TYPE_HINT_POPUP_MENU,	/* A popup menu (from right-click) */
  CDK_WINDOW_TYPE_HINT_TOOLTIP,
  CDK_WINDOW_TYPE_HINT_NOTIFICATION,
  CDK_WINDOW_TYPE_HINT_COMBO,
  CDK_WINDOW_TYPE_HINT_DND
} CdkWindowTypeHint;

/**
 * CdkAxisUse:
 * @CDK_AXIS_IGNORE: the axis is ignored.
 * @CDK_AXIS_X: the axis is used as the x axis.
 * @CDK_AXIS_Y: the axis is used as the y axis.
 * @CDK_AXIS_PRESSURE: the axis is used for pressure information.
 * @CDK_AXIS_XTILT: the axis is used for x tilt information.
 * @CDK_AXIS_YTILT: the axis is used for y tilt information.
 * @CDK_AXIS_WHEEL: the axis is used for wheel information.
 * @CDK_AXIS_DISTANCE: the axis is used for pen/tablet distance information. (Since: 3.22)
 * @CDK_AXIS_ROTATION: the axis is used for pen rotation information. (Since: 3.22)
 * @CDK_AXIS_SLIDER: the axis is used for pen slider information. (Since: 3.22)
 * @CDK_AXIS_LAST: a constant equal to the numerically highest axis value.
 *
 * An enumeration describing the way in which a device
 * axis (valuator) maps onto the predefined valuator
 * types that CTK+ understands.
 *
 * Note that the X and Y axes are not really needed; pointer devices
 * report their location via the x/y members of events regardless. Whether
 * X and Y are present as axes depends on the CDK backend.
 */
typedef enum
{
  CDK_AXIS_IGNORE,
  CDK_AXIS_X,
  CDK_AXIS_Y,
  CDK_AXIS_PRESSURE,
  CDK_AXIS_XTILT,
  CDK_AXIS_YTILT,
  CDK_AXIS_WHEEL,
  CDK_AXIS_DISTANCE,
  CDK_AXIS_ROTATION,
  CDK_AXIS_SLIDER,
  CDK_AXIS_LAST
} CdkAxisUse;

/**
 * CdkAxisFlags:
 * @CDK_AXIS_FLAG_X: X axis is present
 * @CDK_AXIS_FLAG_Y: Y axis is present
 * @CDK_AXIS_FLAG_PRESSURE: Pressure axis is present
 * @CDK_AXIS_FLAG_XTILT: X tilt axis is present
 * @CDK_AXIS_FLAG_YTILT: Y tilt axis is present
 * @CDK_AXIS_FLAG_WHEEL: Wheel axis is present
 * @CDK_AXIS_FLAG_DISTANCE: Distance axis is present
 * @CDK_AXIS_FLAG_ROTATION: Z-axis rotation is present
 * @CDK_AXIS_FLAG_SLIDER: Slider axis is present
 *
 * Flags describing the current capabilities of a device/tool.
 *
 * Since: 3.22
 */
typedef enum
{
  CDK_AXIS_FLAG_X        = 1 << CDK_AXIS_X,
  CDK_AXIS_FLAG_Y        = 1 << CDK_AXIS_Y,
  CDK_AXIS_FLAG_PRESSURE = 1 << CDK_AXIS_PRESSURE,
  CDK_AXIS_FLAG_XTILT    = 1 << CDK_AXIS_XTILT,
  CDK_AXIS_FLAG_YTILT    = 1 << CDK_AXIS_YTILT,
  CDK_AXIS_FLAG_WHEEL    = 1 << CDK_AXIS_WHEEL,
  CDK_AXIS_FLAG_DISTANCE = 1 << CDK_AXIS_DISTANCE,
  CDK_AXIS_FLAG_ROTATION = 1 << CDK_AXIS_ROTATION,
  CDK_AXIS_FLAG_SLIDER   = 1 << CDK_AXIS_SLIDER,
} CdkAxisFlags;

G_END_DECLS

#endif /* __CDK_TYPES_H__ */
