/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1999 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#include "config.h"

#include "ctkdnd.h"
#include "ctkdndprivate.h"
#include "ctksettingsprivate.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "cdk/cdk.h"

#ifdef CDK_WINDOWING_X11
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "cdk/x11/cdkx.h"
#ifdef XINPUT_2
#include <X11/extensions/XInput2.h>
#endif
#endif

#ifdef CDK_WINDOWING_WIN32
#include <cdk/win32/cdkwin32.h>
#endif

#ifdef CDK_WINDOWING_WAYLAND
#include <cdk/wayland/cdkwayland.h>
#endif

#include "ctkdragdest.h"
#include "ctkgesturedrag.h"
#include "ctkgesturesingle.h"
#include "ctkicontheme.h"
#include "ctkimageprivate.h"
#include "ctkintl.h"
#include "ctkmain.h"
#include "ctkplug.h"
#include "ctktooltipprivate.h"
#include "ctkwindow.h"
#include "ctkrender.h"
#include "ctkselectionprivate.h"
#include "ctkwindowgroup.h"
#include "ctkwindowprivate.h"
#include "ctkwidgetprivate.h"


/**
 * SECTION:ctkdnd
 * @Short_description: Functions for controlling drag and drop handling
 * @Title: Drag and Drop
 *
 * CTK+ has a rich set of functions for doing inter-process communication
 * via the drag-and-drop metaphor.
 *
 * As well as the functions listed here, applications may need to use some
 * facilities provided for [Selections][ctk3-Selections]. Also, the Drag and
 * Drop API makes use of signals in the #CtkWidget class.
 */


static GSList *source_widgets = NULL;

typedef struct _CtkDragSourceInfo CtkDragSourceInfo;
typedef struct _CtkDragDestInfo CtkDragDestInfo;


typedef enum 
{
  CTK_DRAG_STATUS_DRAG,
  CTK_DRAG_STATUS_WAIT,
  CTK_DRAG_STATUS_DROP
} CtkDragStatus;

struct _CtkDragSourceInfo 
{
  CtkWidget         *widget;
  CtkTargetList     *target_list; /* Targets for drag data */
  CdkDragAction      possible_actions; /* Actions allowed by source */
  CdkDragContext    *context;     /* drag context */
  CtkWidget         *icon_window; /* Window for drag */
  CtkWidget         *icon_widget; /* Widget for drag */
  CtkWidget         *ipc_widget;  /* CtkInvisible for grab, message passing */
  CdkCursor         *cursor;      /* Cursor for drag */
  gint hot_x, hot_y;              /* Hot spot for drag */
  gint button;                    /* mouse button starting drag */

  CtkDragStatus      status;      /* drag status */
  CdkEvent          *last_event;  /* pending event */

  gint               start_x, start_y; /* Initial position */
  gint               cur_x, cur_y;     /* Current Position */
  CdkScreen         *cur_screen;       /* Current screen for pointer */

  guint32            grab_time;   /* timestamp for initial grab */
  GList             *selections;  /* selections we've claimed */

  CtkDragDestInfo   *proxy_dest;  /* Set if this is a proxy drag */

  guint              update_idle;      /* Idle function to update the drag */
  guint              drop_timeout;     /* Timeout for aborting drop */
  guint              destroy_icon : 1; /* If true, destroy icon_widget */
  guint              have_grab    : 1; /* Do we still have the pointer grab */
};

struct _CtkDragDestInfo
{
  CtkWidget         *widget;              /* Widget in which drag is in */
  CdkDragContext    *context;             /* Drag context */
  CtkDragSourceInfo *proxy_source;        /* Set if this is a proxy drag */
  CtkSelectionData  *proxy_data;          /* Set while retrieving proxied data */
  guint32            proxy_drop_time;     /* Timestamp for proxied drop */
  guint              proxy_drop_wait : 1; /* Set if we are waiting for a
                                           * status reply before sending
                                           * a proxied drop on.
                                           */
  guint              dropped : 1;         /* Set after we receive a drop */
  gint               drop_x, drop_y;      /* Position of drop */
};

#define DROP_ABORT_TIME 300000

typedef gboolean (* CtkDragDestCallback) (CtkWidget      *widget,
                                          CdkDragContext *context,
                                          gint            x,
                                          gint            y,
                                          guint32         time);

/* Enumeration for some targets we handle internally */

enum {
  TARGET_DELETE = 0x40000002
};

/* Forward declarations */
static void          ctk_drag_get_event_actions (const CdkEvent  *event,
                                                 gint             button,
                                                 CdkDragAction    actions,
                                                 CdkDragAction   *suggested_action,
                                                 CdkDragAction   *possible_actions);
static CdkCursor *   ctk_drag_get_cursor         (CtkWidget      *widget,
                                                  CdkDisplay     *display,
                                                  CdkDragAction   action,
                                                  CtkDragSourceInfo *info);
static void          ctk_drag_update_cursor      (CtkDragSourceInfo *info);
static CtkWidget    *ctk_drag_get_ipc_widget            (CtkWidget *widget);
static CtkWidget    *ctk_drag_get_ipc_widget_for_screen (CdkScreen *screen);
static void          ctk_drag_release_ipc_widget (CtkWidget      *widget);

static void     ctk_drag_selection_received     (CtkWidget        *widget,
                                                 CtkSelectionData *selection_data,
                                                 guint             time,
                                                 gpointer          data);
static gboolean ctk_drag_find_widget            (CtkWidget        *widget,
                                                 CdkDragContext   *context,
                                                 CtkDragDestInfo  *info,
                                                 gint              x,
                                                 gint              y,
                                                 guint32           time,
                                                 CtkDragDestCallback callback);
static void     ctk_drag_proxy_begin            (CtkWidget        *widget,
                                                 CtkDragDestInfo  *dest_info,
                                                 guint32           time);
static void     ctk_drag_dest_leave             (CtkWidget        *widget,
                                                 CdkDragContext   *context,
                                                 guint             time);
static gboolean ctk_drag_dest_motion            (CtkWidget        *widget,
                                                 CdkDragContext   *context,
                                                 gint              x,
                                                 gint              y,
                                                 guint             time);
static gboolean ctk_drag_dest_drop              (CtkWidget        *widget,
                                                 CdkDragContext   *context,
                                                 gint              x,
                                                 gint              y,
                                                 guint             time);
static void     ctk_drag_dest_set_widget        (CtkDragDestInfo  *info,
                                                 CtkWidget        *widget);

static CtkDragDestInfo *  ctk_drag_get_dest_info     (CdkDragContext *context,
                                                      gboolean        create);
static CtkDragSourceInfo *ctk_drag_get_source_info   (CdkDragContext *context,
                                                      gboolean        create);
static void               ctk_drag_clear_source_info (CdkDragContext *context);

static void ctk_drag_source_check_selection    (CtkDragSourceInfo *info, 
                                                CdkAtom            selection,
                                                guint32            time);
static void ctk_drag_source_release_selections (CtkDragSourceInfo *info,
                                                guint32            time);
static void ctk_drag_drop                      (CtkDragSourceInfo *info,
                                                guint32            time);
static void ctk_drag_drop_finished             (CtkDragSourceInfo *info,
                                                CtkDragResult      result,
                                                guint              time);
static void ctk_drag_cancel_internal           (CtkDragSourceInfo *info,
                                                CtkDragResult      result,
                                                guint32            time);

static void ctk_drag_selection_get             (CtkWidget         *widget, 
                                                CtkSelectionData  *selection_data,
                                                guint              sel_info,
                                                guint32            time,
                                                gpointer           data);
static void ctk_drag_remove_icon               (CtkDragSourceInfo *info);
static void ctk_drag_source_info_destroy       (CtkDragSourceInfo *info);

static void ctk_drag_context_drop_performed_cb (CdkDragContext    *context,
                                                guint              time,
                                                CtkDragSourceInfo *info);
static void ctk_drag_context_cancel_cb         (CdkDragContext      *context,
                                                CdkDragCancelReason  reason,
                                                CtkDragSourceInfo   *info);
static void ctk_drag_context_action_cb         (CdkDragContext    *context,
                                                CdkDragAction      action,
                                                CtkDragSourceInfo *info);
static void ctk_drag_context_dnd_finished_cb   (CdkDragContext    *context,
                                                CtkDragSourceInfo *info);
static void ctk_drag_add_update_idle           (CtkDragSourceInfo *info);

static void ctk_drag_update                    (CtkDragSourceInfo *info,
                                                CdkScreen         *screen,
                                                gint               x_root,
                                                gint               y_root,
                                                const CdkEvent    *event);
static gboolean ctk_drag_motion_cb             (CtkWidget         *widget, 
                                                CdkEventMotion    *event, 
                                                gpointer           data);
static gboolean ctk_drag_key_cb                (CtkWidget         *widget, 
                                                CdkEventKey       *event, 
                                                gpointer           data);
static gboolean ctk_drag_grab_broken_event_cb  (CtkWidget          *widget,
                                                CdkEventGrabBroken *event,
                                                gpointer            data);
static void     ctk_drag_grab_notify_cb        (CtkWidget         *widget,
                                                gboolean           was_grabbed,
                                                gpointer           data);
static gboolean ctk_drag_button_release_cb     (CtkWidget         *widget, 
                                                CdkEventButton    *event, 
                                                gpointer           data);
static gboolean ctk_drag_abort_timeout         (gpointer           data);

static void     set_icon_helper (CdkDragContext    *context,
                                 CtkImageDefinition*def,
                                 gint               hot_x,
                                 gint               hot_y);

/************************
 * Cursor and Icon data *
 ************************/

static struct {
  CdkDragAction action;
  const gchar  *name;
  GdkPixbuf    *pixbuf;
  CdkCursor    *cursor;
} drag_cursors[] = {
  { CDK_ACTION_DEFAULT, NULL },
  { CDK_ACTION_ASK,   "dnd-ask",  NULL, NULL },
  { CDK_ACTION_COPY,  "copy", NULL, NULL },
  { CDK_ACTION_MOVE,  "move", NULL, NULL },
  { CDK_ACTION_LINK,  "alias", NULL, NULL },
  { 0              ,  "no-drop", NULL, NULL },
};

/*********************
 * Utility functions *
 *********************/

static CtkWidget *
ctk_drag_get_ipc_widget_for_screen (CdkScreen *screen)
{
  CtkWidget *result;
  GSList *drag_widgets = g_object_get_data (G_OBJECT (screen), 
                                            "ctk-dnd-ipc-widgets");
  
  if (drag_widgets)
    {
      GSList *tmp = drag_widgets;
      result = drag_widgets->data;
      drag_widgets = drag_widgets->next;
      g_object_set_data (G_OBJECT (screen),
                         I_("ctk-dnd-ipc-widgets"),
                         drag_widgets);
      g_slist_free_1 (tmp);
    }
  else
    {
      result = ctk_window_new (CTK_WINDOW_POPUP);
      ctk_window_set_screen (CTK_WINDOW (result), screen);
      ctk_window_resize (CTK_WINDOW (result), 1, 1);
      ctk_window_move (CTK_WINDOW (result), -99, -99);
      ctk_widget_show (result);
    }  

  return result;
}

static CtkWidget *
ctk_drag_get_ipc_widget (CtkWidget *widget)
{
  CtkWidget *result;
  CtkWidget *toplevel;

  result = ctk_drag_get_ipc_widget_for_screen (ctk_widget_get_screen (widget));
  
  toplevel = ctk_widget_get_toplevel (widget);
  
  if (CTK_IS_WINDOW (toplevel))
    {
      if (ctk_window_has_group (CTK_WINDOW (toplevel)))
        ctk_window_group_add_window (ctk_window_get_group (CTK_WINDOW (toplevel)),
                                     CTK_WINDOW (result));
    }

  return result;
}

#if defined (CDK_WINDOWING_X11)

/*
 * We want to handle a handful of keys during DND, e.g. Escape to abort.
 * Grabbing the keyboard has the unfortunate side-effect of preventing
 * useful things such as using Alt-Tab to cycle between windows or
 * switching workspaces. Therefore, we just grab the few keys we are
 * interested in. Note that we need to put the grabs on the root window
 * in order for them to still work when the focus is moved to another
 * app/workspace.
 *
 * CDK needs a little help to successfully deliver root key events...
 */

static CdkFilterReturn
root_key_filter (CdkXEvent *xevent,
                 CdkEvent  *event,
                 gpointer   data)
{
  XEvent *ev = (XEvent *) xevent;

  if ((ev->type == KeyPress || ev->type == KeyRelease) &&
      ev->xkey.root == ev->xkey.window)
    ev->xkey.window = (Window)data;
  else if (ev->type == GenericEvent)
    {
      XGenericEventCookie *cookie;
      XIDeviceEvent *dev;

      cookie = &ev->xcookie;
      dev = (XIDeviceEvent *) cookie->data;

      if (dev->evtype == XI_KeyPress ||
          dev->evtype == XI_KeyRelease)
        dev->event = (Window)data;
    }

  return CDK_FILTER_CONTINUE;
}

typedef struct {
  gint keysym;
  gint modifiers;
} GrabKey;

static GrabKey grab_keys[] = {
  { XK_Escape, 0 },
  { XK_space, 0 },
  { XK_KP_Space, 0 },
  { XK_Return, 0 },
  { XK_KP_Enter, 0 },
  { XK_Up, 0 },
  { XK_Up, Mod1Mask },
  { XK_Down, 0 },
  { XK_Down, Mod1Mask },
  { XK_Left, 0 },
  { XK_Left, Mod1Mask },
  { XK_Right, 0 },
  { XK_Right, Mod1Mask },
  { XK_KP_Up, 0 },
  { XK_KP_Up, Mod1Mask },
  { XK_KP_Down, 0 },
  { XK_KP_Down, Mod1Mask },
  { XK_KP_Left, 0 },
  { XK_KP_Left, Mod1Mask },
  { XK_KP_Right, 0 },
  { XK_KP_Right, Mod1Mask }
};

static void
grab_dnd_keys (CtkWidget *widget,
               CdkDevice *device,
               guint32    time)
{
  guint i;
  CdkDisplay *display;
  CdkWindow *window, *root;
  gint keycode;
#ifdef XINPUT_2
  gint deviceid;
  XIGrabModifiers mods;
  gint num_mods;
  XIEventMask evmask;
  unsigned char mask[(XI_LASTEVENT + 7)/8];
  gboolean using_xi2;

  window = ctk_widget_get_window (widget);
  if (!CDK_IS_X11_WINDOW (window))
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      cdk_device_grab (device,
                       ctk_widget_get_window (widget),
                       CDK_OWNERSHIP_APPLICATION, FALSE,
                       CDK_KEY_PRESS_MASK | CDK_KEY_RELEASE_MASK,
                       NULL, time);
      G_GNUC_END_IGNORE_DEPRECATIONS;
      return;
    }

  deviceid = cdk_x11_device_get_id (device);

  if (CDK_IS_X11_DEVICE_XI2 (device))
    using_xi2 = TRUE;
  else
    using_xi2 = FALSE;
#endif

  display = ctk_widget_get_display (widget);
  root = cdk_screen_get_root_window (ctk_widget_get_screen (widget));

  cdk_x11_display_error_trap_push (display);

  for (i = 0; i < G_N_ELEMENTS (grab_keys); ++i)
    {
      keycode = XKeysymToKeycode (CDK_WINDOW_XDISPLAY (window), grab_keys[i].keysym);
      if (keycode == NoSymbol)
        continue;

#ifdef XINPUT_2
      if (using_xi2)
        {
          memset (mask, 0, sizeof (mask));
          XISetMask (mask, XI_KeyPress);
          XISetMask (mask, XI_KeyRelease);

          evmask.deviceid = deviceid;
          evmask.mask_len = sizeof (mask);
          evmask.mask = mask;

          num_mods = 1;
          mods.modifiers = grab_keys[i].modifiers;

          XIGrabKeycode (CDK_WINDOW_XDISPLAY (window),
                         deviceid,
                         keycode,
                         CDK_WINDOW_XID (root),
                         GrabModeAsync,
                         GrabModeAsync,
                         False,
                         &evmask,
                         num_mods,
                         &mods);
        }
      else
#endif
        XGrabKey (CDK_WINDOW_XDISPLAY (window),
                  keycode, grab_keys[i].modifiers,
                  CDK_WINDOW_XID (root),
                  FALSE,
                  GrabModeAsync,
                  GrabModeAsync);
    }

  cdk_display_flush (display);
  cdk_x11_display_error_trap_pop_ignored (display);

  cdk_window_add_filter (NULL, root_key_filter, (gpointer) CDK_WINDOW_XID (window));
}

static void
ungrab_dnd_keys (CtkWidget *widget,
                 CdkDevice *device,
                 guint32    time)
{
  guint i;
  CdkWindow *window, *root;
  CdkDisplay *display;
  gint keycode;
#ifdef XINPUT_2
  XIGrabModifiers mods;
  gint num_mods;
  gint deviceid;
  gboolean using_xi2;

  window = ctk_widget_get_window (widget);
  if (!CDK_IS_X11_WINDOW (window))
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      cdk_device_ungrab (device, time);
      G_GNUC_END_IGNORE_DEPRECATIONS;
      return;
    }

  deviceid = cdk_x11_device_get_id (device);

  if (CDK_IS_X11_DEVICE_XI2 (device))
    using_xi2 = TRUE;
  else
    using_xi2 = FALSE;
#endif

  display = ctk_widget_get_display (widget);
  root = cdk_screen_get_root_window (ctk_widget_get_screen (widget));

  cdk_window_remove_filter (NULL, root_key_filter, (gpointer) CDK_WINDOW_XID (window));

  cdk_x11_display_error_trap_push (display);

  for (i = 0; i < G_N_ELEMENTS (grab_keys); ++i)
    {
      keycode = XKeysymToKeycode (CDK_WINDOW_XDISPLAY (window), grab_keys[i].keysym);
      if (keycode == NoSymbol)
        continue;

#ifdef XINPUT_2
      if (using_xi2)
        {
          num_mods = 1;
          mods.modifiers = grab_keys[i].modifiers;

          XIUngrabKeycode (CDK_WINDOW_XDISPLAY (window),
                           deviceid,
                           keycode,
                           CDK_WINDOW_XID (root),
                           num_mods,
                           &mods);
        }
      else
#endif
        XUngrabKey (CDK_WINDOW_XDISPLAY (window),
                    keycode, grab_keys[i].modifiers,
                    CDK_WINDOW_XID (root));
    }

  cdk_display_flush (display);
  cdk_x11_display_error_trap_pop_ignored (display);
}

#else /* !CDK_WINDOWING_X11 */

static void
grab_dnd_keys (CtkWidget *widget,
               CdkDevice *device,
               guint32    time)
{
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  cdk_device_grab (device,
                   ctk_widget_get_window (widget),
                   CDK_OWNERSHIP_APPLICATION, FALSE,
                   CDK_KEY_PRESS_MASK | CDK_KEY_RELEASE_MASK,
                   NULL, time);
  G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
ungrab_dnd_keys (CtkWidget *widget,
                 CdkDevice *device,
                 guint32    time)
{
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  cdk_device_ungrab (device, time);
  G_GNUC_END_IGNORE_DEPRECATIONS;
}

#endif /* CDK_WINDOWING_X11 */

/*
 * ctk_drag_release_ipc_widget:
 * @widget: the widget to release
 *
 * Releases widget retrieved with ctk_drag_get_ipc_widget().
 */
static void
ctk_drag_release_ipc_widget (CtkWidget *widget)
{
  CtkWindow *window = CTK_WINDOW (widget);
  CdkScreen *screen = ctk_widget_get_screen (widget);
  CdkDragContext *context = g_object_get_data (G_OBJECT (widget), "drag-context");
  GSList *drag_widgets = g_object_get_data (G_OBJECT (screen),
                                            "ctk-dnd-ipc-widgets");
  CdkDevice *pointer, *keyboard;

  if (context)
    {
      pointer = cdk_drag_context_get_device (context);
      keyboard = cdk_device_get_associated_device (pointer);

      if (keyboard)
        ungrab_dnd_keys (widget, keyboard, CDK_CURRENT_TIME);
    }

  if (ctk_window_has_group (window))
    ctk_window_group_remove_window (ctk_window_get_group (window),
                                    window);
  drag_widgets = g_slist_prepend (drag_widgets, widget);
  g_object_set_data (G_OBJECT (screen),
                     I_("ctk-dnd-ipc-widgets"),
                     drag_widgets);
}

static guint32
ctk_drag_get_event_time (CdkEvent *event)
{
  guint32 tm = CDK_CURRENT_TIME;
  
  if (event)
    switch (event->type)
      {
      case CDK_MOTION_NOTIFY:
        tm = event->motion.time; break;
      case CDK_BUTTON_PRESS:
      case CDK_2BUTTON_PRESS:
      case CDK_3BUTTON_PRESS:
      case CDK_BUTTON_RELEASE:
        tm = event->button.time; break;
      case CDK_KEY_PRESS:
      case CDK_KEY_RELEASE:
        tm = event->key.time; break;
      case CDK_ENTER_NOTIFY:
      case CDK_LEAVE_NOTIFY:
        tm = event->crossing.time; break;
      case CDK_PROPERTY_NOTIFY:
        tm = event->property.time; break;
      case CDK_SELECTION_CLEAR:
      case CDK_SELECTION_REQUEST:
      case CDK_SELECTION_NOTIFY:
        tm = event->selection.time; break;
      case CDK_PROXIMITY_IN:
      case CDK_PROXIMITY_OUT:
        tm = event->proximity.time; break;
      default:                  /* use current time */
        break;
      }
  
  return tm;
}

static void
ctk_drag_get_event_actions (const CdkEvent *event,
                            gint            button,
                            CdkDragAction   actions,
                            CdkDragAction  *suggested_action,
                            CdkDragAction  *possible_actions)
{
  *suggested_action = 0;
  *possible_actions = 0;

  if (event)
    {
      CdkModifierType state = 0;
      
      switch (event->type)
        {
        case CDK_MOTION_NOTIFY:
          state = event->motion.state;
          break;
        case CDK_BUTTON_PRESS:
        case CDK_2BUTTON_PRESS:
        case CDK_3BUTTON_PRESS:
        case CDK_BUTTON_RELEASE:
          state = event->button.state;
          break;
        case CDK_KEY_PRESS:
        case CDK_KEY_RELEASE:
          state = event->key.state;
          break;
        case CDK_ENTER_NOTIFY:
        case CDK_LEAVE_NOTIFY:
          state = event->crossing.state;
          break;
        default:
          break;
        }

      if ((button == CDK_BUTTON_MIDDLE || button == CDK_BUTTON_SECONDARY) && (actions & CDK_ACTION_ASK))
        {
          *suggested_action = CDK_ACTION_ASK;
          *possible_actions = actions;
        }
      else if (state & (CDK_SHIFT_MASK | CDK_CONTROL_MASK))
        {
          if ((state & CDK_SHIFT_MASK) && (state & CDK_CONTROL_MASK))
            {
              if (actions & CDK_ACTION_LINK)
                {
                  *suggested_action = CDK_ACTION_LINK;
                  *possible_actions = CDK_ACTION_LINK;
                }
            }
          else if (state & CDK_CONTROL_MASK)
            {
              if (actions & CDK_ACTION_COPY)
                {
                  *suggested_action = CDK_ACTION_COPY;
                  *possible_actions = CDK_ACTION_COPY;
                }
            }
          else
            {
              if (actions & CDK_ACTION_MOVE)
                {
                  *suggested_action = CDK_ACTION_MOVE;
                  *possible_actions = CDK_ACTION_MOVE;
                }
            }
        }
      else
        {
          *possible_actions = actions;

          if ((state & (CDK_MOD1_MASK)) && (actions & CDK_ACTION_ASK))
            *suggested_action = CDK_ACTION_ASK;
          else if (actions & CDK_ACTION_COPY)
            *suggested_action =  CDK_ACTION_COPY;
          else if (actions & CDK_ACTION_MOVE)
            *suggested_action = CDK_ACTION_MOVE;
          else if (actions & CDK_ACTION_LINK)
            *suggested_action = CDK_ACTION_LINK;
        }
    }
  else
    {
      *possible_actions = actions;
      
      if (actions & CDK_ACTION_COPY)
        *suggested_action =  CDK_ACTION_COPY;
      else if (actions & CDK_ACTION_MOVE)
        *suggested_action = CDK_ACTION_MOVE;
      else if (actions & CDK_ACTION_LINK)
        *suggested_action = CDK_ACTION_LINK;
    }
}

static void
ensure_drag_cursor_pixbuf (int i)
{
  if (drag_cursors[i].pixbuf == NULL)
    {
      char *path = g_strconcat ("/org/ctk/libctk/cursor/",  drag_cursors[i].name, ".png", NULL);
      GInputStream *stream = g_resources_open_stream (path, 0, NULL);
      if (stream != NULL)
        {
          drag_cursors[i].pixbuf = gdk_pixbuf_new_from_stream (stream, NULL, NULL);
          g_object_unref (stream);
        }
      g_free (path);
    }
}

static CdkCursor *
ctk_drag_get_cursor (CtkWidget         *widget,
                     CdkDisplay        *display,
                     CdkDragAction      action,
                     CtkDragSourceInfo *info)
{
  gint i;

  /* reconstruct the cursors for each new drag (thus !info),
   * to catch cursor theme changes
   */
  if (!info)
    {
      for (i = 0 ; i < G_N_ELEMENTS (drag_cursors) - 1; i++)
        g_clear_object (&drag_cursors[i].cursor);
    }

  for (i = 0 ; i < G_N_ELEMENTS (drag_cursors) - 1; i++)
    if (drag_cursors[i].action == action)
      break;

  if (drag_cursors[i].cursor != NULL)
    {
      if (display != cdk_cursor_get_display (drag_cursors[i].cursor))
        g_clear_object (&drag_cursors[i].cursor);
    }

  if (drag_cursors[i].cursor == NULL)
    drag_cursors[i].cursor = cdk_cursor_new_from_name (display, drag_cursors[i].name);

  if (drag_cursors[i].cursor == NULL)
    {
      ensure_drag_cursor_pixbuf (i);
      drag_cursors[i].cursor = cdk_cursor_new_from_pixbuf (display, drag_cursors[i].pixbuf, 0, 0);
    }

  return drag_cursors[i].cursor;
}

static void
ctk_drag_update_cursor (CtkDragSourceInfo *info)
{
  CdkCursor *cursor;
  gint i;

  if (!info->have_grab)
    return;

  for (i = 0 ; i < G_N_ELEMENTS (drag_cursors) - 1; i++)
    if (info->cursor == drag_cursors[i].cursor)
      break;

  if (i == G_N_ELEMENTS (drag_cursors))
    return;

  cursor = ctk_drag_get_cursor (info->widget,
                                cdk_cursor_get_display (info->cursor),
                                drag_cursors[i].action, info);

  if (cursor != info->cursor)
    {
      CdkDevice *pointer;

      pointer = cdk_drag_context_get_device (info->context);
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      cdk_device_grab (pointer,
                       ctk_widget_get_window (info->ipc_widget),
                       CDK_OWNERSHIP_APPLICATION, FALSE,
                       CDK_POINTER_MOTION_MASK | CDK_BUTTON_RELEASE_MASK,
                       cursor, info->grab_time);
      G_GNUC_END_IGNORE_DEPRECATIONS;
      info->cursor = cursor;
    }
}

/********************
 * Destination side *
 ********************/

/**
 * ctk_drag_get_data: (method)
 * @widget: the widget that will receive the
 *   #CtkWidget::drag-data-received signal
 * @context: the drag context
 * @target: the target (form of the data) to retrieve
 * @time_: a timestamp for retrieving the data. This will
 *   generally be the time received in a #CtkWidget::drag-motion
 *   or #CtkWidget::drag-drop signal
 *
 * Gets the data associated with a drag. When the data
 * is received or the retrieval fails, CTK+ will emit a
 * #CtkWidget::drag-data-received signal. Failure of the retrieval
 * is indicated by the length field of the @selection_data
 * signal parameter being negative. However, when ctk_drag_get_data()
 * is called implicitely because the %CTK_DEST_DEFAULT_DROP was set,
 * then the widget will not receive notification of failed
 * drops.
 */
void
ctk_drag_get_data (CtkWidget      *widget,
                   CdkDragContext *context,
                   CdkAtom         target,
                   guint32         time_)
{
  CtkWidget *selection_widget;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CDK_IS_DRAG_CONTEXT (context));

  selection_widget = ctk_drag_get_ipc_widget (widget);

  g_object_ref (context);
  g_object_ref (widget);

  g_signal_connect (selection_widget, "selection-received",
                    G_CALLBACK (ctk_drag_selection_received), widget);

  g_object_set_data (G_OBJECT (selection_widget), I_("drag-context"), context);

  ctk_selection_convert (selection_widget,
                         cdk_drag_get_selection (context),
                         target,
                         time_);
}

/**
 * ctk_drag_get_source_widget:
 * @context: a (destination side) drag context
 *
 * Determines the source widget for a drag.
 *
 * Returns: (nullable) (transfer none): if the drag is occurring
 *     within a single application, a pointer to the source widget.
 *     Otherwise, %NULL.
 */
CtkWidget *
ctk_drag_get_source_widget (CdkDragContext *context)
{
  GSList *tmp_list;

  g_return_val_if_fail (CDK_IS_DRAG_CONTEXT (context), NULL);
  
  tmp_list = source_widgets;
  while (tmp_list)
    {
      CtkWidget *ipc_widget = tmp_list->data;

      if (ctk_widget_get_window (ipc_widget) == cdk_drag_context_get_source_window (context))
        {
          CtkDragSourceInfo *info;
          info = g_object_get_data (G_OBJECT (ipc_widget), "ctk-info");

          return info ? info->widget : NULL;
        }

      tmp_list = tmp_list->next;
    }

  return NULL;
}

/**
 * ctk_drag_finish:
 * @context: the drag context
 * @success: a flag indicating whether the drop was successful
 * @del: a flag indicating whether the source should delete the
 *   original data. (This should be %TRUE for a move)
 * @time_: the timestamp from the #CtkWidget::drag-drop signal
 *
 * Informs the drag source that the drop is finished, and
 * that the data of the drag will no longer be required.
 */
void 
ctk_drag_finish (CdkDragContext *context,
                 gboolean        success,
                 gboolean        del,
                 guint32         time)
{
  CdkAtom target = CDK_NONE;

  g_return_if_fail (CDK_IS_DRAG_CONTEXT (context));

  if (success && del)
    {
      target = cdk_atom_intern_static_string ("DELETE");
    }

  if (target != CDK_NONE)
    {
      CtkWidget *selection_widget = ctk_drag_get_ipc_widget_for_screen (cdk_window_get_screen (cdk_drag_context_get_source_window (context)));

      g_object_ref (context);
      
      g_object_set_data (G_OBJECT (selection_widget), I_("drag-context"), context);
      g_signal_connect (selection_widget, "selection-received",
                        G_CALLBACK (ctk_drag_selection_received),
                        NULL);
      
      ctk_selection_convert (selection_widget,
                             cdk_drag_get_selection (context),
                             target,
                             time);
    }
  
  if (!(success && del))
    cdk_drop_finish (context, success, time);
}

/**
 * ctk_drag_highlight: (method)
 * @widget: a widget to highlight
 *
 * Highlights a widget as a currently hovered drop target.
 * To end the highlight, call ctk_drag_unhighlight().
 * CTK+ calls this automatically if %CTK_DEST_DEFAULT_HIGHLIGHT is set.
 */
void
ctk_drag_highlight (CtkWidget  *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_widget_set_state_flags (widget, CTK_STATE_FLAG_DROP_ACTIVE, FALSE);
}

/**
 * ctk_drag_unhighlight: (method)
 * @widget: a widget to remove the highlight from
 *
 * Removes a highlight set by ctk_drag_highlight() from
 * a widget.
 */
void
ctk_drag_unhighlight (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_widget_unset_state_flags (widget, CTK_STATE_FLAG_DROP_ACTIVE);
}

/*
 * _ctk_drag_dest_handle_event:
 * @toplevel: Toplevel widget that received the event
 * @event: the event to handle
 *
 * Called from widget event handling code on Drag events
 * for destinations.
 */
void
_ctk_drag_dest_handle_event (CtkWidget *toplevel,
                             CdkEvent  *event)
{
  CtkDragDestInfo *info;
  CdkDragContext *context;

  g_return_if_fail (toplevel != NULL);
  g_return_if_fail (event != NULL);

  context = event->dnd.context;

  info = ctk_drag_get_dest_info (context, TRUE);

  /* Find the widget for the event */
  switch (event->type)
    {
    case CDK_DRAG_ENTER:
      break;
      
    case CDK_DRAG_LEAVE:
      if (info->widget)
        {
          ctk_drag_dest_leave (info->widget, context, event->dnd.time);
          ctk_drag_dest_set_widget (info, NULL);
        }
      break;
      
    case CDK_DRAG_MOTION:
    case CDK_DROP_START:
      {
        CdkWindow *window;
        gint tx, ty;
        gboolean found;

        if (event->type == CDK_DROP_START)
          {
            info->dropped = TRUE;
            /* We send a leave here so that the widget unhighlights
             * properly.
             */
            if (info->widget)
              {
                ctk_drag_dest_leave (info->widget, context, event->dnd.time);
                ctk_drag_dest_set_widget (info, NULL);
              }
          }

        window = ctk_widget_get_window (toplevel);

#ifdef CDK_WINDOWING_X11
        /* Hackaround for: http://bugzilla.gnome.org/show_bug.cgi?id=136112
         *
         * Currently cdk_window_get_position doesn't provide reliable
         * information for embedded windows, so we call the much more
         * expensive cdk_window_get_origin().
         */
        if (CTK_IS_PLUG (toplevel))
          cdk_window_get_origin (window, &tx, &ty);
        else
#endif /* CDK_WINDOWING_X11 */
          cdk_window_get_position (window, &tx, &ty);

        found = ctk_drag_find_widget (toplevel,
                                      context,
                                      info,
                                      event->dnd.x_root - tx,
                                      event->dnd.y_root - ty,
                                      event->dnd.time,
                                      (event->type == CDK_DRAG_MOTION) ?
                                      ctk_drag_dest_motion :
                                      ctk_drag_dest_drop);

        if (info->widget && !found)
          {
            ctk_drag_dest_leave (info->widget, context, event->dnd.time);
            ctk_drag_dest_set_widget (info, NULL);
          }
        
        /* Send a reply.
         */
        if (event->type == CDK_DRAG_MOTION)
          {
            if (!found)
              cdk_drag_status (context, 0, event->dnd.time);
          }
        else if (event->type == CDK_DROP_START && !info->proxy_source)
          {
            cdk_drop_reply (context, found, event->dnd.time);
          }
      }
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
ctk_drag_selection_received (CtkWidget        *widget,
                             CtkSelectionData *selection_data,
                             guint             time,
                             gpointer          data)
{
  CdkDragContext *context;
  CtkDragDestInfo *info;
  CtkWidget *drop_widget;
  CdkAtom target;

  drop_widget = data;

  context = g_object_get_data (G_OBJECT (widget), "drag-context");
  info = ctk_drag_get_dest_info (context, FALSE);

  if (info->proxy_data && 
      ctk_selection_data_get_target (info->proxy_data) == ctk_selection_data_get_target (selection_data))
    {
      ctk_selection_data_set (info->proxy_data,
                              ctk_selection_data_get_data_type (selection_data),
                              ctk_selection_data_get_format (selection_data),
                              ctk_selection_data_get_data (selection_data),
                              ctk_selection_data_get_length (selection_data));
      ctk_main_quit ();
      return;
    }

  target = ctk_selection_data_get_target (selection_data);
  if (target == cdk_atom_intern_static_string ("DELETE"))
    {
      ctk_drag_finish (context, TRUE, FALSE, time);
    }
  else if ((target == cdk_atom_intern_static_string ("XmTRANSFER_SUCCESS")) ||
           (target == cdk_atom_intern_static_string ("XmTRANSFER_FAILURE")))
    {
      /* Do nothing */
    }
  else
    {
      CtkDragDestSite *site;

      site = g_object_get_data (G_OBJECT (drop_widget), "ctk-drag-dest");

      if (site && site->target_list)
        {
          guint target_info;

          if (ctk_target_list_find (site->target_list, 
                                    target,
                                    &target_info))
            {
              if (!(site->flags & CTK_DEST_DEFAULT_DROP) ||
                  ctk_selection_data_get_length (selection_data) >= 0)
                g_signal_emit_by_name (drop_widget,
                                       "drag-data-received",
                                       context, info->drop_x, info->drop_y,
                                       selection_data,
                                       target_info, time);
            }
        }
      else
        {
          g_signal_emit_by_name (drop_widget,
                                 "drag-data-received",
                                 context, info->drop_x, info->drop_y,
                                 selection_data,
                                 0, time);
        }
      
      if (site && site->flags & CTK_DEST_DEFAULT_DROP)
        {

          ctk_drag_finish (context, 
                           (ctk_selection_data_get_length (selection_data) >= 0),
                           (cdk_drag_context_get_selected_action (context) == CDK_ACTION_MOVE),
                           time);
        }
      
      g_object_unref (drop_widget);
    }

  g_signal_handlers_disconnect_by_func (widget,
                                        ctk_drag_selection_received,
                                        data);
  
  g_object_set_data (G_OBJECT (widget), I_("drag-context"), NULL);
  g_object_unref (context);

  ctk_drag_release_ipc_widget (widget);
}

static gboolean
ctk_drag_find_widget (CtkWidget           *widget,
                      CdkDragContext      *context,
                      CtkDragDestInfo     *info,
                      gint                 x,
                      gint                 y,
                      guint32              time,
                      CtkDragDestCallback  callback)
{
  if (!ctk_widget_get_mapped (widget) ||
      !ctk_widget_get_sensitive (widget))
    return FALSE;

  /* Get the widget at the pointer coordinates and travel up
   * the widget hierarchy from there.
   */
  widget = _ctk_widget_find_at_coords (ctk_widget_get_window (widget),
                                       x, y, &x, &y);
  if (!widget)
    return FALSE;

  while (widget)
    {
      CtkWidget *parent;
      GList *hierarchy = NULL;
      gboolean found = FALSE;

      if (!ctk_widget_get_mapped (widget))
        return FALSE;

      if (ctk_widget_get_state_flags (widget) & CTK_STATE_FLAG_INSENSITIVE)
        {
          widget = ctk_widget_get_parent (widget);
          continue;
        }

      /* need to reference the entire hierarchy temporarily in case the
       * ::drag-motion/::drag-drop callbacks change the widget hierarchy.
       */
      for (parent = widget;
           parent;
           parent = ctk_widget_get_parent (parent))
        {
          hierarchy = g_list_prepend (hierarchy, g_object_ref (parent));
        }

      /* If the current widget is registered as a drop site, check to
       * emit "drag-motion" to check if we are actually in a drop
       * site.
       */
      if (g_object_get_data (G_OBJECT (widget), "ctk-drag-dest"))
        {
          found = callback (widget, context, x, y, time);

          /* If so, send a "drag-leave" to the last widget */
          if (found && info->widget != widget)
            {
              if (info->widget)
                ctk_drag_dest_leave (info->widget, context, time);

              ctk_drag_dest_set_widget (info, widget);
            }
        }

      if (!found)
        {
          /* Get the parent before unreffing the hierarchy because
           * invoking the callback might have destroyed the widget
           */
          parent = ctk_widget_get_parent (widget);

          /* The parent might be going away when unreffing the
           * hierarchy, so also protect againt that
           */
          if (parent)
            g_object_add_weak_pointer (G_OBJECT (parent), (gpointer *) &parent);
        }

      g_list_free_full (hierarchy, g_object_unref);

      if (found)
        return TRUE;

      if (parent)
        g_object_remove_weak_pointer (G_OBJECT (parent), (gpointer *) &parent);
      else
        return FALSE;

      if (!ctk_widget_translate_coordinates (widget, parent, x, y, &x, &y))
        return FALSE;

      widget = parent;
    }

  return FALSE;
}

static void
ctk_drag_proxy_begin (CtkWidget       *widget,
                      CtkDragDestInfo *dest_info,
                      guint32          time)
{
  CtkDragSourceInfo *source_info;
  GList *tmp_list;
  CdkDragContext *context;
  CtkWidget *ipc_widget;

  if (dest_info->proxy_source)
    {
      cdk_drag_abort (dest_info->proxy_source->context, time);
      ctk_drag_source_info_destroy (dest_info->proxy_source);
      dest_info->proxy_source = NULL;
    }
  
  ipc_widget = ctk_drag_get_ipc_widget (widget);
  context = cdk_drag_begin (ctk_widget_get_window (ipc_widget),
                            cdk_drag_context_list_targets (dest_info->context));

  source_info = ctk_drag_get_source_info (context, TRUE);

  source_info->ipc_widget = ipc_widget;
  source_info->widget = g_object_ref (widget);

  source_info->target_list = ctk_target_list_new (NULL, 0);
  tmp_list = cdk_drag_context_list_targets (dest_info->context);
  while (tmp_list)
    {
      ctk_target_list_add (source_info->target_list,
                           CDK_POINTER_TO_ATOM (tmp_list->data), 0, 0);
      tmp_list = tmp_list->next;
    }

  source_info->proxy_dest = dest_info;
  
  g_signal_connect (ipc_widget,
                    "selection-get",
                    G_CALLBACK (ctk_drag_selection_get),
                    source_info);
  
  dest_info->proxy_source = source_info;
}

static void
ctk_drag_dest_set_widget (CtkDragDestInfo *info,
                          CtkWidget       *widget)
{
  if (info->widget)
    g_object_remove_weak_pointer (G_OBJECT (info->widget), (gpointer *) &info->widget);

  info->widget = widget;

  if (info->widget)
    g_object_add_weak_pointer (G_OBJECT (info->widget), (gpointer *) &info->widget);
}

static void
ctk_drag_dest_info_destroy (gpointer data)
{
  CtkDragDestInfo *info = (CtkDragDestInfo *)data;

  ctk_drag_dest_set_widget (info, NULL);

  g_slice_free (CtkDragDestInfo, data);
}

static CtkDragDestInfo *
ctk_drag_get_dest_info (CdkDragContext *context,
                        gboolean        create)
{
  CtkDragDestInfo *info;
  static GQuark info_quark = 0;
  if (!info_quark)
    info_quark = g_quark_from_static_string ("ctk-dest-info");
  
  info = g_object_get_qdata (G_OBJECT (context), info_quark);
  if (!info && create)
    {
      info = g_slice_new0 (CtkDragDestInfo);
      info->context = context;
      g_object_set_qdata_full (G_OBJECT (context), info_quark,
                               info, ctk_drag_dest_info_destroy);
    }

  return info;
}

static GQuark dest_info_quark = 0;

static CtkDragSourceInfo *
ctk_drag_get_source_info (CdkDragContext *context,
                          gboolean        create)
{
  CtkDragSourceInfo *info;
  if (!dest_info_quark)
    dest_info_quark = g_quark_from_static_string ("ctk-source-info");
  
  info = g_object_get_qdata (G_OBJECT (context), dest_info_quark);
  if (!info && create)
    {
      info = g_new0 (CtkDragSourceInfo, 1);
      info->context = context;
      g_object_set_qdata (G_OBJECT (context), dest_info_quark, info);
    }

  return info;
}

static void
ctk_drag_clear_source_info (CdkDragContext *context)
{
  g_object_set_qdata (G_OBJECT (context), dest_info_quark, NULL);
}

/*
 * Default drag handlers
 */
static void  
ctk_drag_dest_leave (CtkWidget      *widget,
                     CdkDragContext *context,
                     guint           time)
{
  CtkDragDestSite *site;

  site = g_object_get_data (G_OBJECT (widget), "ctk-drag-dest");
  g_return_if_fail (site != NULL);

  if (site->do_proxy)
    {
      CtkDragDestInfo *info = ctk_drag_get_dest_info (context, FALSE);

      if (info->proxy_source && info->proxy_source->widget == widget && !info->dropped)
        {
          cdk_drag_abort (info->proxy_source->context, time);
          ctk_drag_source_info_destroy (info->proxy_source);
          info->proxy_source = NULL;
        }
      
      return;
    }
  else
    {
      if ((site->flags & CTK_DEST_DEFAULT_HIGHLIGHT) && site->have_drag)
        ctk_drag_unhighlight (widget);

      if (!(site->flags & CTK_DEST_DEFAULT_MOTION) || site->have_drag ||
          site->track_motion)
        g_signal_emit_by_name (widget, "drag-leave", context, time);
      
      site->have_drag = FALSE;
    }
}

static gboolean
ctk_drag_dest_motion (CtkWidget      *widget,
                      CdkDragContext *context,
                      gint            x,
                      gint            y,
                      guint           time)
{
  CtkDragDestSite *site;
  CdkDragAction action = 0;
  gboolean retval;

  site = g_object_get_data (G_OBJECT (widget), "ctk-drag-dest");
  g_return_val_if_fail (site != NULL, FALSE);

  if (site->do_proxy)
    {
      CdkAtom selection;
      CdkEvent *current_event;
      CdkWindow *dest_window;
      CdkDragProtocol proto;
        
      CtkDragDestInfo *info = ctk_drag_get_dest_info (context, FALSE);

      if (!info->proxy_source || info->proxy_source->widget != widget)
        ctk_drag_proxy_begin (widget, info, time);

      current_event = ctk_get_current_event ();

      if (site->proxy_window)
        {
          dest_window = site->proxy_window;
          proto = site->proxy_protocol;
        }
      else
        {
          cdk_drag_find_window_for_screen (info->proxy_source->context,
                                           NULL,
                                           cdk_window_get_screen (current_event->dnd.window),
                                           current_event->dnd.x_root, 
                                           current_event->dnd.y_root,
                                           &dest_window, &proto);
        }
      
      cdk_drag_motion (info->proxy_source->context, 
                       dest_window, proto,
                       current_event->dnd.x_root, 
                       current_event->dnd.y_root, 
                       cdk_drag_context_get_suggested_action (context),
                       cdk_drag_context_get_actions (context),
                       time);

      if (!site->proxy_window && dest_window)
        g_object_unref (dest_window);

      selection = cdk_drag_get_selection (info->proxy_source->context);
      if (selection && 
          selection != cdk_drag_get_selection (info->context))
        ctk_drag_source_check_selection (info->proxy_source, selection, time);

      cdk_event_free (current_event);
      
      return TRUE;
    }

  if (site->track_motion || site->flags & CTK_DEST_DEFAULT_MOTION)
    {
      if (cdk_drag_context_get_suggested_action (context) & site->actions)
        action = cdk_drag_context_get_suggested_action (context);
      else
        {
          gint i;
          
          for (i = 0; i < 8; i++)
            {
              if ((site->actions & (1 << i)) &&
                  (cdk_drag_context_get_actions (context) & (1 << i)))
                {
                  action = (1 << i);
                  break;
                }
            }
        }

      if (action && ctk_drag_dest_find_target (widget, context, NULL))
        {
          if (!site->have_drag)
            {
              site->have_drag = TRUE;
              if (site->flags & CTK_DEST_DEFAULT_HIGHLIGHT)
                ctk_drag_highlight (widget);
            }

          cdk_drag_status (context, action, time);
        }
      else
        {
          cdk_drag_status (context, 0, time);
          if (!site->track_motion)
            return TRUE;
        }
    }

  g_signal_emit_by_name (widget, "drag-motion",
                         context, x, y, time, &retval);

  return (site->flags & CTK_DEST_DEFAULT_MOTION) ? TRUE : retval;
}

static gboolean
ctk_drag_dest_drop (CtkWidget      *widget,
                    CdkDragContext *context,
                    gint            x,
                    gint            y,
                    guint           time)
{
  CtkDragDestSite *site;
  CtkDragDestInfo *info;

  site = g_object_get_data (G_OBJECT (widget), "ctk-drag-dest");
  g_return_val_if_fail (site != NULL, FALSE);

  info = ctk_drag_get_dest_info (context, FALSE);
  g_return_val_if_fail (info != NULL, FALSE);

  info->drop_x = x;
  info->drop_y = y;

  if (site->do_proxy)
    {
      if (info->proxy_source || 
          (cdk_drag_context_get_protocol (info->context) == CDK_DRAG_PROTO_ROOTWIN))
        {
          ctk_drag_drop (info->proxy_source, time);
        }
      else
        {
          /* We need to synthesize a motion event, wait for a status,
           * and, if we get a good one, do a drop.
           */
          
          CdkEvent *current_event;
          CdkAtom selection;
          CdkWindow *dest_window;
          CdkDragProtocol proto;
          
          ctk_drag_proxy_begin (widget, info, time);
          info->proxy_drop_wait = TRUE;
          info->proxy_drop_time = time;
          
          current_event = ctk_get_current_event ();

          if (site->proxy_window)
            {
              dest_window = site->proxy_window;
              proto = site->proxy_protocol;
            }
          else
            {
              cdk_drag_find_window_for_screen (info->proxy_source->context,
                                               NULL,
                                               cdk_window_get_screen (current_event->dnd.window),
                                               current_event->dnd.x_root, 
                                               current_event->dnd.y_root,
                                               &dest_window, &proto);
            }

          cdk_drag_motion (info->proxy_source->context, 
                           dest_window, proto,
                           current_event->dnd.x_root, 
                           current_event->dnd.y_root, 
                           cdk_drag_context_get_suggested_action (context),
                           cdk_drag_context_get_actions (context),
                           time);

          if (!site->proxy_window && dest_window)
            g_object_unref (dest_window);

          selection = cdk_drag_get_selection (info->proxy_source->context);
          if (selection && 
              selection != cdk_drag_get_selection (info->context))
            ctk_drag_source_check_selection (info->proxy_source, selection, time);

          cdk_event_free (current_event);
        }

      return TRUE;
    }
  else
    {
      gboolean retval;

      if (site->flags & CTK_DEST_DEFAULT_DROP)
        {
          CdkAtom target = ctk_drag_dest_find_target (widget, context, NULL);

          if (target == CDK_NONE)
            {
              ctk_drag_finish (context, FALSE, FALSE, time);
              return TRUE;
            }
          else 
            ctk_drag_get_data (widget, context, target, time);
        }

      g_signal_emit_by_name (widget, "drag-drop",
                             context, x, y, time, &retval);

      return (site->flags & CTK_DEST_DEFAULT_DROP) ? TRUE : retval;
    }
}

/***************
 * Source side *
 ***************/


static gboolean
ctk_drag_is_managed (CtkWidget *source_widget)
{
  return
#ifdef CDK_WINDOWING_X11
    CDK_IS_X11_DISPLAY (ctk_widget_get_display (source_widget)) ||
#endif
#ifdef CDK_WINDOWING_WAYLAND
    CDK_IS_WAYLAND_DISPLAY (ctk_widget_get_display (source_widget)) ||
#endif
#ifdef CDK_WINDOWING_WIN32
    CDK_IS_WIN32_DISPLAY (ctk_widget_get_display (source_widget)) ||
#endif
    FALSE;
}

/* Like ctk_drag_begin(), but also communicates the need to
 * create an icon for the drag operation back to the caller.
 * If the caller passes out_needs_icon == NULL, it means that
 * the caller does not care.
 */
CdkDragContext *
ctk_drag_begin_internal (CtkWidget           *widget,
                         gboolean            *out_needs_icon,
                         CtkTargetList       *target_list,
                         CdkDragAction        actions,
                         gint                 button,
                         const CdkEvent      *event,
                         int                  x,
                         int                  y)
{
  CtkDragSourceInfo *info;
  GList *targets = NULL;
  GList *tmp_list;
  guint32 time = CDK_CURRENT_TIME;
  CdkDragAction possible_actions, suggested_action;
  CdkDragContext *context;
  CtkWidget *ipc_widget;
  CdkCursor *cursor;
  CdkDevice *pointer, *keyboard;
  CdkWindow *ipc_window;
  gint start_x, start_y;
  CdkAtom selection;
  gboolean managed;

  managed = ctk_drag_is_managed (widget);

  pointer = keyboard = NULL;
  ipc_widget = ctk_drag_get_ipc_widget (widget);
  
  ctk_drag_get_event_actions (event, button, actions,
                              &suggested_action, &possible_actions);
  
  cursor = ctk_drag_get_cursor (widget,
                                ctk_widget_get_display (widget), 
                                suggested_action,
                                NULL);
  
  if (event)
    {
      time = cdk_event_get_time (event);
      if (time == CDK_CURRENT_TIME)
        time = ctk_get_current_event_time ();

      pointer = cdk_event_get_device (event);

      if (cdk_device_get_source (pointer) == CDK_SOURCE_KEYBOARD)
        {
          keyboard = pointer;
          pointer = cdk_device_get_associated_device (keyboard);
        }
      else
        keyboard = cdk_device_get_associated_device (pointer);
    }
  else
    {
      CdkSeat *seat;

      seat = cdk_display_get_default_seat (ctk_widget_get_display (widget));
      pointer = cdk_seat_get_pointer (seat);
      keyboard = cdk_seat_get_keyboard (seat);
    }

  if (!pointer)
    return NULL;

  ipc_window = ctk_widget_get_window (ipc_widget);

  if (!managed)
    {
      gboolean grabbed;

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      grabbed = cdk_device_grab (pointer, ipc_window,
                                 CDK_OWNERSHIP_APPLICATION, FALSE,
                                 CDK_POINTER_MOTION_MASK |
                                 CDK_BUTTON_RELEASE_MASK,
                                 cursor, time) == CDK_GRAB_SUCCESS;
      G_GNUC_END_IGNORE_DEPRECATIONS;

      if (!grabbed)
        {
          ctk_drag_release_ipc_widget (ipc_widget);
          return NULL;
        }

      if (keyboard)
        grab_dnd_keys (ipc_widget, keyboard, time);

      /* We use a CTK grab here to override any grabs that the widget
       * we are dragging from might have held
       */
      ctk_device_grab_add (ipc_widget, pointer, FALSE);
    }

  tmp_list = g_list_last (target_list->list);
  while (tmp_list)
    {
      CtkTargetPair *pair = tmp_list->data;
      targets = g_list_prepend (targets, 
                                GINT_TO_POINTER (pair->target));
      tmp_list = tmp_list->prev;
    }

  source_widgets = g_slist_prepend (source_widgets, ipc_widget);

  if (x != -1 && y != -1)
    {
      CtkWidget *toplevel = ctk_widget_get_toplevel (widget);
      ctk_widget_translate_coordinates (widget, toplevel,
                                        x, y, &x, &y);
      cdk_window_get_root_coords (ctk_widget_get_window (toplevel),
                                  x, y, &start_x, &start_y);
    }
  else if (event && event->type == CDK_MOTION_NOTIFY)
    {
      start_x = event->motion.x_root;
      start_y = event->motion.y_root;
    }
  else
    cdk_device_get_position (pointer, NULL, &start_x, &start_y);

  context = cdk_drag_begin_from_point (ipc_window, pointer, targets, start_x, start_y);

  cdk_drag_context_set_device (context, pointer);
  g_list_free (targets);

  if (managed &&
      !cdk_drag_context_manage_dnd (context, ipc_window, actions))
    {
      ctk_drag_release_ipc_widget (ipc_widget);
      g_object_unref (context);
      return NULL;
    }
  
  info = ctk_drag_get_source_info (context, TRUE);
  
  info->ipc_widget = ipc_widget;
  g_object_set_data (G_OBJECT (info->ipc_widget), I_("ctk-info"), info);

  info->widget = g_object_ref (widget);
  
  info->button = button;
  info->cursor = cursor;
  info->target_list = target_list;
  ctk_target_list_ref (target_list);

  info->possible_actions = actions;

  info->status = CTK_DRAG_STATUS_DRAG;
  info->last_event = NULL;
  info->selections = NULL;
  info->icon_window = NULL;
  info->icon_widget = NULL;
  info->destroy_icon = FALSE;

  if (event)
    info->cur_screen = cdk_event_get_screen (event);
  else
    cdk_device_get_position (pointer, &info->cur_screen, NULL, NULL);

  info->start_x = start_x;
  info->start_y = start_y;

  ctk_widget_reset_controllers (widget);

  g_signal_emit_by_name (widget, "drag-begin", info->context);

  /* Ensure that we have an icon before we start the drag; the
   * application may have set one in ::drag_begin, or it may
   * not have set one.
   */
  if (!info->icon_widget && out_needs_icon == NULL)
    {
      CtkImageDefinition *icon = ctk_image_definition_new_icon_name ("text-x-generic");
      set_icon_helper (info->context, icon, 0, 0);
      ctk_image_definition_unref (icon);
    }

  if (out_needs_icon != NULL)
    *out_needs_icon = (info->icon_widget == NULL);

  if (managed)
    {
      g_signal_connect (context, "drop-performed",
                        G_CALLBACK (ctk_drag_context_drop_performed_cb), info);
      g_signal_connect (context, "dnd-finished",
                        G_CALLBACK (ctk_drag_context_dnd_finished_cb), info);
      g_signal_connect (context, "cancel",
                        G_CALLBACK (ctk_drag_context_cancel_cb), info);
      g_signal_connect (context, "action-changed",
                        G_CALLBACK (ctk_drag_context_action_cb), info);

      selection = cdk_drag_get_selection (context);
      if (selection)
        ctk_drag_source_check_selection (info, selection, time);
    }
  else
    {
      info->cur_x = info->start_x;
      info->cur_y = info->start_y;

      if (event && event->type == CDK_MOTION_NOTIFY)
        ctk_drag_motion_cb (info->ipc_widget, (CdkEventMotion *)event, info);
      else
        ctk_drag_update (info, info->cur_screen, info->cur_x, info->cur_y, event);

      g_signal_connect (info->ipc_widget, "grab-broken-event",
                        G_CALLBACK (ctk_drag_grab_broken_event_cb), info);
      g_signal_connect (info->ipc_widget, "grab-notify",
                        G_CALLBACK (ctk_drag_grab_notify_cb), info);
      g_signal_connect (info->ipc_widget, "button-release-event",
                        G_CALLBACK (ctk_drag_button_release_cb), info);
      g_signal_connect (info->ipc_widget, "motion-notify-event",
                        G_CALLBACK (ctk_drag_motion_cb), info);
      g_signal_connect (info->ipc_widget, "key-press-event",
                        G_CALLBACK (ctk_drag_key_cb), info);
      g_signal_connect (info->ipc_widget, "key-release-event",
                        G_CALLBACK (ctk_drag_key_cb), info);
    }

  g_signal_connect (info->ipc_widget, "selection-get",
                    G_CALLBACK (ctk_drag_selection_get), info);

  info->have_grab = TRUE;
  info->grab_time = time;

  return info->context;
}

/**
 * ctk_drag_begin_with_coordinates: (method)
 * @widget: the source widget
 * @targets: The targets (data formats) in which the
 *    source can provide the data
 * @actions: A bitmask of the allowed drag actions for this drag
 * @button: The button the user clicked to start the drag
 * @event: (nullable): The event that triggered the start of the drag,
 *    or %NULL if none can be obtained.
 * @x: The initial x coordinate to start dragging from, in the coordinate space
 *    of @widget. If -1 is passed, the coordinates are retrieved from @event or
 *    the current pointer position
 * @y: The initial y coordinate to start dragging from, in the coordinate space
 *    of @widget. If -1 is passed, the coordinates are retrieved from @event or
 *    the current pointer position
 *
 * Initiates a drag on the source side. The function only needs to be used
 * when the application is starting drags itself, and is not needed when
 * ctk_drag_source_set() is used.
 *
 * The @event is used to retrieve the timestamp that will be used internally to
 * grab the pointer.  If @event is %NULL, then %CDK_CURRENT_TIME will be used.
 * However, you should try to pass a real event in all cases, since that can be
 * used to get information about the drag.
 *
 * Generally there are three cases when you want to start a drag by hand by
 * calling this function:
 *
 * 1. During a #CtkWidget::button-press-event handler, if you want to start a drag
 * immediately when the user presses the mouse button.  Pass the @event
 * that you have in your #CtkWidget::button-press-event handler.
 *
 * 2. During a #CtkWidget::motion-notify-event handler, if you want to start a drag
 * when the mouse moves past a certain threshold distance after a button-press.
 * Pass the @event that you have in your #CtkWidget::motion-notify-event handler.
 *
 * 3. During a timeout handler, if you want to start a drag after the mouse
 * button is held down for some time.  Try to save the last event that you got
 * from the mouse, using cdk_event_copy(), and pass it to this function
 * (remember to free the event with cdk_event_free() when you are done).
 * If you really cannot pass a real event, pass %NULL instead.
 *
 * Returns: (transfer none): the context for this drag
 *
 * Since: 3.10
 */
CdkDragContext *
ctk_drag_begin_with_coordinates (CtkWidget     *widget,
                                 CtkTargetList *targets,
                                 CdkDragAction  actions,
                                 gint           button,
                                 CdkEvent      *event,
                                 gint           x,
                                 gint           y)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (ctk_widget_get_realized (widget), NULL);
  g_return_val_if_fail (targets != NULL, NULL);

  return ctk_drag_begin_internal (widget, NULL, targets,
                                  actions, button, event, x, y);
}

/**
 * ctk_drag_begin: (method)
 * @widget: the source widget
 * @targets: The targets (data formats) in which the
 *    source can provide the data
 * @actions: A bitmask of the allowed drag actions for this drag
 * @button: The button the user clicked to start the drag
 * @event: (nullable): The event that triggered the start of the drag,
 *    or %NULL if none can be obtained.
 *
 * This function is equivalent to ctk_drag_begin_with_coordinates(),
 * passing -1, -1 as coordinates.
 *
 * Returns: (transfer none): the context for this drag
 *
 * Deprecated: 3.10: Use ctk_drag_begin_with_coordinates() instead
 */
CdkDragContext *
ctk_drag_begin (CtkWidget     *widget,
                CtkTargetList *targets,
                CdkDragAction  actions,
                gint           button,
                CdkEvent      *event)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (ctk_widget_get_realized (widget), NULL);
  g_return_val_if_fail (targets != NULL, NULL);

  return ctk_drag_begin_internal (widget, NULL, targets,
                                  actions, button, event, -1, -1);
}

static void
icon_widget_destroyed (CtkWidget         *widget,
                       CtkDragSourceInfo *info)
{
  g_clear_object (&info->icon_widget);
}

static void
ctk_drag_update_icon_window (CtkDragSourceInfo *info)
{
  if (!ctk_drag_is_managed (info->widget) && info->icon_window)
    {
      ctk_window_move (CTK_WINDOW (info->icon_window),
                       info->cur_x - info->hot_x,
                       info->cur_y - info->hot_y);

      if (ctk_widget_get_visible (info->icon_window))
        cdk_window_raise (ctk_widget_get_window (info->icon_window));
      else
        ctk_widget_show (info->icon_window);
    }
}

static void
ctk_drag_set_icon_widget_internal (CdkDragContext *context,
                                   CtkWidget      *widget,
                                   gint            hot_x,
                                   gint            hot_y,
                                   gboolean        destroy_on_release)
{
  CtkDragSourceInfo *info;

  info = ctk_drag_get_source_info (context, FALSE);
  if (info == NULL)
    {
      if (destroy_on_release)
        ctk_widget_destroy (widget);
      return;
    }

  ctk_drag_remove_icon (info);

  if (widget)
    g_object_ref (widget);

  info->icon_widget = widget;
  info->hot_x = hot_x;
  info->hot_y = hot_y;
  info->destroy_icon = destroy_on_release;

  if (!widget)
    goto out;

  g_signal_connect (widget, "destroy", G_CALLBACK (icon_widget_destroyed), info);

  cdk_drag_context_set_hotspot (context, hot_x, hot_y);

  if (!info->icon_window)
    {
      CdkScreen *screen;
      CdkVisual *visual;
      gboolean has_rgba;

      screen = cdk_window_get_screen (cdk_drag_context_get_source_window (context));
      visual = cdk_screen_get_rgba_visual (screen);
      has_rgba = visual != NULL && cdk_screen_is_composited (screen);

      info->icon_window = ctk_window_new (CTK_WINDOW_POPUP);
      ctk_window_set_type_hint (CTK_WINDOW (info->icon_window), CDK_WINDOW_TYPE_HINT_DND);
      ctk_window_set_screen (CTK_WINDOW (info->icon_window), screen);
      ctk_widget_set_size_request (info->icon_window, 24, 24);
      if (visual)
        ctk_widget_set_visual (info->icon_window, visual);
      ctk_widget_set_events (info->icon_window, CDK_BUTTON_PRESS_MASK | CDK_BUTTON_RELEASE_MASK);

      if (has_rgba)
        ctk_widget_set_app_paintable (info->icon_window, TRUE);

      ctk_window_set_hardcoded_window (CTK_WINDOW (info->icon_window),
                                       cdk_drag_context_get_drag_window (context));
      ctk_widget_show (info->icon_window);
    }

  if (CTK_IS_WINDOW (widget))
    {
      ctk_widget_hide (widget);
      ctk_widget_unrealize (widget);
      ctk_widget_set_parent_window (widget, ctk_widget_get_window (info->icon_window));
      ctk_widget_show (widget);
    }

  if (ctk_bin_get_child (CTK_BIN (info->icon_window)))
    ctk_container_remove (CTK_CONTAINER (info->icon_window), ctk_bin_get_child (CTK_BIN (info->icon_window)));
  ctk_container_add (CTK_CONTAINER (info->icon_window), widget);

out:
  ctk_drag_update_cursor (info);
  ctk_drag_update_icon_window (info);
}

/**
 * ctk_drag_set_icon_widget:
 * @context: the context for a drag. (This must be called 
          with a context for the source side of a drag)
 * @widget: a widget to use as an icon
 * @hot_x: the X offset within @widget of the hotspot
 * @hot_y: the Y offset within @widget of the hotspot
 * 
 * Changes the icon for drag operation to a given widget.
 * CTK+ will not destroy the widget, so if you don’t want
 * it to persist, you should connect to the “drag-end” 
 * signal and destroy it yourself.
 */
void 
ctk_drag_set_icon_widget (CdkDragContext *context,
                          CtkWidget      *widget,
                          gint            hot_x,
                          gint            hot_y)
{
  g_return_if_fail (CDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_drag_set_icon_widget_internal (context, widget, hot_x, hot_y, FALSE);
}

static void
ctk_drag_draw_icon_pattern (CtkWidget *window,
                            cairo_t   *cr,
                            gpointer   pattern)
{
  cairo_set_source (cr, pattern);
  cairo_paint (cr);
}

static void
ctk_drag_draw_icon_pattern_and_background (CtkWidget *window,
                                           cairo_t   *cr,
                                           gpointer   pattern)
{
  CtkStyleContext *context;
  int width, height;

  context = ctk_widget_get_style_context (window);
  width = ctk_widget_get_allocated_width (window);
  height = ctk_widget_get_allocated_height (window);

  ctk_render_background (context, cr, 0, 0, width, height);
  ctk_render_frame (context, cr, 0, 0, width, height);

  cairo_set_source (cr, pattern);
  cairo_paint (cr);
}

static void
set_icon_helper (CdkDragContext     *context,
                 CtkImageDefinition *def,
                 gint                hot_x,
                 gint                hot_y)
{
  CtkWidget *widget;

  widget = ctk_image_new ();
  ctk_widget_show (widget);

  ctk_image_set_from_definition (CTK_IMAGE (widget), def, CTK_ICON_SIZE_DND);

  ctk_drag_set_icon_widget_internal (context, widget, hot_x, hot_y, TRUE);
}

void 
ctk_drag_set_icon_definition (CdkDragContext     *context,
                              CtkImageDefinition *def,
                              gint                hot_x,
                              gint                hot_y)
{
  g_return_if_fail (CDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (def != NULL);

  set_icon_helper (context, def, hot_x, hot_y);
}

/**
 * ctk_drag_set_icon_pixbuf:
 * @context: the context for a drag (This must be called 
 *            with a  context for the source side of a drag)
 * @pixbuf: the #GdkPixbuf to use as the drag icon
 * @hot_x: the X offset within @widget of the hotspot
 * @hot_y: the Y offset within @widget of the hotspot
 * 
 * Sets @pixbuf as the icon for a given drag.
 */
void 
ctk_drag_set_icon_pixbuf (CdkDragContext *context,
                          GdkPixbuf      *pixbuf,
                          gint            hot_x,
                          gint            hot_y)
{
  CtkImageDefinition *def;

  g_return_if_fail (CDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));

  def = ctk_image_definition_new_pixbuf (pixbuf, 1);
  set_icon_helper (context, def, hot_x, hot_y);

  ctk_image_definition_unref (def);
}

/**
 * ctk_drag_set_icon_stock:
 * @context: the context for a drag (This must be called 
 *            with a  context for the source side of a drag)
 * @stock_id: the ID of the stock icon to use for the drag
 * @hot_x: the X offset within the icon of the hotspot
 * @hot_y: the Y offset within the icon of the hotspot
 * 
 * Sets the icon for a given drag from a stock ID.
 *
 * Deprecated: 3.10: Use ctk_drag_set_icon_name() instead.
 */
void 
ctk_drag_set_icon_stock (CdkDragContext *context,
                         const gchar    *stock_id,
                         gint            hot_x,
                         gint            hot_y)
{
  CtkImageDefinition *def;

  g_return_if_fail (CDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (stock_id != NULL);

  def = ctk_image_definition_new_stock (stock_id);
  set_icon_helper (context, def, hot_x, hot_y);

  ctk_image_definition_unref (def);
}

/* XXX: This function is in cdk, too. Should it be in Cairo? */
static gboolean
_ctk_cairo_surface_extents (cairo_surface_t *surface,
                            CdkRectangle    *extents)
{
  double x1, x2, y1, y2;
  cairo_t *cr;

  g_return_val_if_fail (surface != NULL, FALSE);
  g_return_val_if_fail (extents != NULL, FALSE);

  cr = cairo_create (surface);
  cairo_clip_extents (cr, &x1, &y1, &x2, &y2);
  cairo_destroy (cr);

  x1 = floor (x1);
  y1 = floor (y1);
  x2 = ceil (x2);
  y2 = ceil (y2);
  x2 -= x1;
  y2 -= y1;
  
  if (x1 < G_MININT || x1 > G_MAXINT ||
      y1 < G_MININT || y1 > G_MAXINT ||
      x2 > G_MAXINT || y2 > G_MAXINT)
    {
      extents->x = extents->y = extents->width = extents->height = 0;
      return FALSE;
    }

  extents->x = x1;
  extents->y = y1;
  extents->width = x2;
  extents->height = y2;

  return TRUE;
}

/**
 * ctk_drag_set_icon_surface:
 * @context: the context for a drag (This must be called
 *     with a context for the source side of a drag)
 * @surface: the surface to use as icon
 *
 * Sets @surface as the icon for a given drag. CTK+ retains
 * references for the arguments, and will release them when
 * they are no longer needed.
 *
 * To position the surface relative to the mouse, use
 * cairo_surface_set_device_offset() on @surface. The mouse
 * cursor will be positioned at the (0,0) coordinate of the
 * surface.
 */
void
ctk_drag_set_icon_surface (CdkDragContext  *context,
                           cairo_surface_t *surface)
{
  CtkWidget *window;
  CdkScreen *screen;
  CdkRectangle extents;
  cairo_pattern_t *pattern;
  CdkVisual *rgba_visual;
  gboolean has_rgba;
  cairo_matrix_t matrix;

  g_return_if_fail (CDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (surface != NULL);

  _ctk_cairo_surface_extents (surface, &extents);

  screen = cdk_window_get_screen (cdk_drag_context_get_source_window (context));
  rgba_visual = cdk_screen_get_rgba_visual (screen);

  window = ctk_window_new (CTK_WINDOW_POPUP);
  has_rgba = rgba_visual != NULL && cdk_screen_is_composited (screen);

  ctk_window_set_screen (CTK_WINDOW (window), screen);

  if (has_rgba)
    ctk_widget_set_visual (CTK_WIDGET (window), rgba_visual);

  ctk_window_set_type_hint (CTK_WINDOW (window), CDK_WINDOW_TYPE_HINT_DND);

  ctk_widget_set_events (window, CDK_BUTTON_PRESS_MASK | CDK_BUTTON_RELEASE_MASK);
  ctk_widget_set_app_paintable (window, TRUE);

  ctk_widget_set_size_request (window, extents.width, extents.height);
  ctk_widget_realize (window);

  pattern = cairo_pattern_create_for_surface (surface);
  cairo_matrix_init_translate (&matrix, extents.x, extents.y);
  cairo_pattern_set_matrix (pattern, &matrix);

  g_signal_connect_data (window,
                         "draw",
                         has_rgba ? G_CALLBACK (ctk_drag_draw_icon_pattern)
                                  : G_CALLBACK (ctk_drag_draw_icon_pattern_and_background),
                         pattern,
                         (GClosureNotify) cairo_pattern_destroy,
                         G_CONNECT_AFTER);

  ctk_drag_set_icon_widget_internal (context, window, extents.x, extents.y, TRUE);
}

/**
 * ctk_drag_set_icon_name:
 * @context: the context for a drag (This must be called 
 *     with a context for the source side of a drag)
 * @icon_name: name of icon to use
 * @hot_x: the X offset of the hotspot within the icon
 * @hot_y: the Y offset of the hotspot within the icon
 * 
 * Sets the icon for a given drag from a named themed icon. See
 * the docs for #CtkIconTheme for more details. Note that the
 * size of the icon depends on the icon theme (the icon is
 * loaded at the symbolic size #CTK_ICON_SIZE_DND), thus 
 * @hot_x and @hot_y have to be used with care.
 *
 * Since: 2.8
 */
void 
ctk_drag_set_icon_name (CdkDragContext *context,
                        const gchar    *icon_name,
                        gint            hot_x,
                        gint            hot_y)
{
  CtkImageDefinition *def;

  g_return_if_fail (CDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (icon_name != NULL && icon_name[0] != '\0');

  def = ctk_image_definition_new_icon_name (icon_name);
  set_icon_helper (context, def, hot_x, hot_y);

  ctk_image_definition_unref (def);
}

/**
 * ctk_drag_set_icon_gicon:
 * @context: the context for a drag (This must be called 
 *     with a context for the source side of a drag)
 * @icon: a #GIcon
 * @hot_x: the X offset of the hotspot within the icon
 * @hot_y: the Y offset of the hotspot within the icon
 * 
 * Sets the icon for a given drag from the given @icon.
 * See the documentation for ctk_drag_set_icon_name()
 * for more details about using icons in drag and drop.
 *
 * Since: 3.2
 */
void 
ctk_drag_set_icon_gicon (CdkDragContext *context,
                         GIcon          *icon,
                         gint            hot_x,
                         gint            hot_y)
{
  CtkImageDefinition *def;

  g_return_if_fail (CDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (icon != NULL);

  def = ctk_image_definition_new_gicon (icon);
  set_icon_helper (context, def, hot_x, hot_y);

  ctk_image_definition_unref (def);
}

/**
 * ctk_drag_set_icon_default:
 * @context: the context for a drag (This must be called 
 *     with a  context for the source side of a drag)
 * 
 * Sets the icon for a particular drag to the default
 * icon.
 */
void 
ctk_drag_set_icon_default (CdkDragContext *context)
{
  g_return_if_fail (CDK_IS_DRAG_CONTEXT (context));

  ctk_drag_set_icon_name (context, "text-x-generic", -2, -2);
}

/*
 * _ctk_drag_source_handle_event:
 * @toplevel: Toplevel widget that received the event
 * @event: the event to handle
 *
 * Called from widget event handling code on Drag events
 * for drag sources.
 */
void
_ctk_drag_source_handle_event (CtkWidget *widget,
                               CdkEvent  *event)
{
  CtkDragSourceInfo *info;
  CdkDragContext *context;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (event != NULL);

  context = event->dnd.context;
  info = ctk_drag_get_source_info (context, FALSE);
  if (!info)
    return;

  switch (event->type)
    {
    case CDK_DRAG_STATUS:
      {
        CdkCursor *cursor;
        if (info->proxy_dest)
          {
            if (!event->dnd.send_event)
              {
                if (info->proxy_dest->proxy_drop_wait)
                  {
                    gboolean result = cdk_drag_context_get_selected_action (context) != 0;
                    
                    /* Aha - we can finally pass the DROP on... */
                    cdk_drop_reply (info->proxy_dest->context, result, info->proxy_dest->proxy_drop_time);
                    if (result)
                      cdk_drag_drop (info->context, info->proxy_dest->proxy_drop_time);
                    else
                      ctk_drag_finish (info->proxy_dest->context, FALSE, FALSE, info->proxy_dest->proxy_drop_time);
                  }
                else
                  {
                    cdk_drag_status (info->proxy_dest->context,
                                     cdk_drag_context_get_selected_action (event->dnd.context),
                                     event->dnd.time);
                  }
              }
          }
        else if (info->have_grab)
          {
            cursor = ctk_drag_get_cursor (widget, 
                                          ctk_widget_get_display (widget),
                                          cdk_drag_context_get_selected_action (event->dnd.context),
                                          info);
            if (info->cursor != cursor)
              {
                CdkDevice *pointer;

                pointer = cdk_drag_context_get_device (context);
                G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
                cdk_device_grab (pointer, ctk_widget_get_window (widget),
                                 CDK_OWNERSHIP_APPLICATION, FALSE,
                                 CDK_POINTER_MOTION_MASK | CDK_BUTTON_RELEASE_MASK,
                                 cursor, info->grab_time);
                G_GNUC_END_IGNORE_DEPRECATIONS;
                info->cursor = cursor;
              }
            
            ctk_drag_add_update_idle (info);
          }
      }
      break;
      
    case CDK_DROP_FINISHED:
      ctk_drag_drop_finished (info, CTK_DRAG_RESULT_SUCCESS, event->dnd.time);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
ctk_drag_source_check_selection (CtkDragSourceInfo *info, 
                                 CdkAtom            selection,
                                 guint32            time)
{
  GList *tmp_list;

  tmp_list = info->selections;
  while (tmp_list)
    {
      if (CDK_POINTER_TO_ATOM (tmp_list->data) == selection)
        return;
      tmp_list = tmp_list->next;
    }

  ctk_selection_owner_set_for_display (ctk_widget_get_display (info->widget),
                                       info->ipc_widget,
                                       selection,
                                       time);
  info->selections = g_list_prepend (info->selections,
                                     GUINT_TO_POINTER (selection));

  tmp_list = info->target_list->list;
  while (tmp_list)
    {
      CtkTargetPair *pair = tmp_list->data;

      ctk_selection_add_target (info->ipc_widget,
                                selection,
                                pair->target,
                                pair->info);
      tmp_list = tmp_list->next;
    }

  ctk_selection_add_target (info->ipc_widget,
                            selection,
                            cdk_atom_intern_static_string ("DELETE"),
                            TARGET_DELETE);
}


/* Clean up from the drag, and display snapback, if necessary. */
static void
ctk_drag_drop_finished (CtkDragSourceInfo *info,
                        CtkDragResult      result,
                        guint              time)
{
  gboolean success;

  success = (result == CTK_DRAG_RESULT_SUCCESS);
  ctk_drag_source_release_selections (info, time); 

  if (info->proxy_dest)
    {
      /* The time from the event isn't reliable for Xdnd drags */
      ctk_drag_finish (info->proxy_dest->context, success, FALSE, 
                       info->proxy_dest->proxy_drop_time);
      ctk_drag_source_info_destroy (info);
    }
  else
    {
      if (!success)
        g_signal_emit_by_name (info->widget, "drag-failed",
                               info->context, result, &success);

      cdk_drag_drop_done (info->context, success);
      ctk_drag_source_info_destroy (info);
    }
}

static void
ctk_drag_source_release_selections (CtkDragSourceInfo *info,
                                    guint32            time)
{
  CdkDisplay *display = ctk_widget_get_display (info->widget);
  GList *tmp_list = info->selections;
  
  while (tmp_list)
    {
      CdkAtom selection = CDK_POINTER_TO_ATOM (tmp_list->data);
      if (cdk_selection_owner_get_for_display (display, selection) == ctk_widget_get_window (info->ipc_widget))
        ctk_selection_owner_set_for_display (display, NULL, selection, time);

      tmp_list = tmp_list->next;
    }

  g_list_free (info->selections);
  info->selections = NULL;
}

static void
ctk_drag_drop (CtkDragSourceInfo *info, 
               guint32            time)
{
  if (cdk_drag_context_get_protocol (info->context) == CDK_DRAG_PROTO_ROOTWIN)
    {
      CtkSelectionData selection_data;
      GList *tmp_list;
      /* CTK+ traditionally has used application/x-rootwin-drop, but the
       * XDND spec specifies x-rootwindow-drop.
       */
      CdkAtom target1 = cdk_atom_intern_static_string ("application/x-rootwindow-drop");
      CdkAtom target2 = cdk_atom_intern_static_string ("application/x-rootwin-drop");
      
      tmp_list = info->target_list->list;
      while (tmp_list)
        {
          CtkTargetPair *pair = tmp_list->data;
          
          if (pair->target == target1 || pair->target == target2)
            {
              selection_data.selection = CDK_NONE;
              selection_data.target = pair->target;
              selection_data.data = NULL;
              selection_data.length = -1;

              g_signal_emit_by_name (info->widget, "drag-data-get",
                                     info->context, &selection_data,
                                     pair->info,
                                     time);

              /* FIXME: Should we check for length >= 0 here? */
              ctk_drag_drop_finished (info, CTK_DRAG_RESULT_SUCCESS, time);
              return;
            }
          tmp_list = tmp_list->next;
        }
      ctk_drag_drop_finished (info, CTK_DRAG_RESULT_NO_TARGET, time);
    }
  else
    {
      if (info->icon_window)
        ctk_widget_hide (info->icon_window);

      cdk_drag_drop (info->context, time);
      info->drop_timeout = cdk_threads_add_timeout (DROP_ABORT_TIME,
                                          ctk_drag_abort_timeout,
                                          info);
      g_source_set_name_by_id (info->drop_timeout, "[ctk+] ctk_drag_abort_timeout");
    }
}

/*
 * Source side callbacks.
 */
static void
ctk_drag_selection_get (CtkWidget        *widget, 
                        CtkSelectionData *selection_data,
                        guint             sel_info,
                        guint32           time,
                        gpointer          data)
{
  CtkDragSourceInfo *info = data;
  static CdkAtom null_atom = CDK_NONE;
  guint target_info;

  if (!null_atom)
    null_atom = cdk_atom_intern_static_string ("NULL");

  switch (sel_info)
    {
    case TARGET_DELETE:
      g_signal_emit_by_name (info->widget,
                             "drag-data-delete", 
                             info->context);
      ctk_selection_data_set (selection_data, null_atom, 8, NULL, 0);
      break;
    default:
      if (info->proxy_dest)
        {
          /* This is sort of dangerous and needs to be thought
           * through better
           */
          info->proxy_dest->proxy_data = selection_data;
          ctk_drag_get_data (info->widget,
                             info->proxy_dest->context,
                             ctk_selection_data_get_target (selection_data),
                             time);
          ctk_main ();
          info->proxy_dest->proxy_data = NULL;
        }
      else
        {
          if (ctk_target_list_find (info->target_list, 
                                    ctk_selection_data_get_target (selection_data),
                                    &target_info))
            {
              g_signal_emit_by_name (info->widget, "drag-data-get",
                                     info->context,
                                     selection_data,
                                     target_info,
                                     time);
            }
        }
      break;
    }
}

static void
ctk_drag_remove_icon (CtkDragSourceInfo *info)
{
  if (info->icon_widget)
    {
      CtkWidget *widget;

      widget = info->icon_widget;
      info->icon_widget = NULL;

      g_signal_handlers_disconnect_by_func (widget, icon_widget_destroyed, info);

      ctk_widget_hide (widget);
      ctk_widget_set_opacity (widget, 1.0);

      if (info->destroy_icon)
        ctk_widget_destroy (widget);
      else
        ctk_container_remove (CTK_CONTAINER (info->icon_window), widget);

      g_object_unref (widget);
    }
}

static void
ctk_drag_source_info_free (CtkDragSourceInfo *info)
{
  ctk_drag_remove_icon (info);
  ctk_widget_destroy (info->icon_window);
  g_free (info);
}

static void
ctk_drag_source_info_destroy (CtkDragSourceInfo *info)
{
  CdkDragContext *context;
  CdkEvent       *last_event;

  g_signal_handlers_disconnect_by_func (info->context,
                                        ctk_drag_context_drop_performed_cb,
                                        info);
  g_signal_handlers_disconnect_by_func (info->context,
                                        ctk_drag_context_dnd_finished_cb,
                                        info);
  g_signal_handlers_disconnect_by_func (info->context,
                                        ctk_drag_context_cancel_cb,
                                        info);
  g_signal_handlers_disconnect_by_func (info->context,
                                        ctk_drag_context_action_cb,
                                        info);

  g_signal_handlers_disconnect_by_func (info->ipc_widget,
                                        ctk_drag_grab_broken_event_cb,
                                        info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
                                        ctk_drag_grab_notify_cb,
                                        info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
                                        ctk_drag_button_release_cb,
                                        info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
                                        ctk_drag_motion_cb,
                                        info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
                                        ctk_drag_key_cb,
                                        info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
                                        ctk_drag_selection_get,
                                        info);

  if (!info->proxy_dest)
    g_signal_emit_by_name (info->widget, "drag-end", info->context);

  g_clear_object (&info->widget);

  ctk_selection_remove_all (info->ipc_widget);
  g_object_set_data (G_OBJECT (info->ipc_widget), I_("ctk-info"), NULL);
  source_widgets = g_slist_remove (source_widgets, info->ipc_widget);
  ctk_drag_release_ipc_widget (info->ipc_widget);

  ctk_target_list_unref (info->target_list);

  if (info->drop_timeout)
    g_source_remove (info->drop_timeout);

  if (info->update_idle)
    g_source_remove (info->update_idle);

  /* keep the icon_window alive until the (possible) drag cancel animation is done */
  g_object_set_data_full (G_OBJECT (info->context), "former-ctk-source-info", info, (GDestroyNotify)ctk_drag_source_info_free);
  context = info->context;
  last_event = info->last_event;

  ctk_drag_clear_source_info (context);

  if (last_event)
    cdk_event_free (last_event);

  g_object_unref (context);
}

static gboolean
ctk_drag_update_idle (gpointer data)
{
  CtkDragSourceInfo *info = data;
  CdkWindow *dest_window;
  CdkDragProtocol protocol;
  CdkAtom selection;

  CdkDragAction action;
  CdkDragAction possible_actions;
  guint32 time;

  info->update_idle = 0;
    
  if (info->last_event)
    {
      time = ctk_drag_get_event_time (info->last_event);
      ctk_drag_get_event_actions (info->last_event,
                                  info->button, 
                                  info->possible_actions,
                                  &action, &possible_actions);

      ctk_drag_update_icon_window (info);
      cdk_drag_find_window_for_screen (info->context,
                                       info->icon_window ? ctk_widget_get_window (info->icon_window) : NULL,
                                       info->cur_screen, info->cur_x, info->cur_y,
                                       &dest_window, &protocol);
      
      if (!cdk_drag_motion (info->context, dest_window, protocol,
                            info->cur_x, info->cur_y, action, 
                            possible_actions,
                            time))
        {
          cdk_event_free ((CdkEvent *)info->last_event);
          info->last_event = NULL;
        }
  
      if (dest_window)
        g_object_unref (dest_window);
      
      selection = cdk_drag_get_selection (info->context);
      if (selection)
        ctk_drag_source_check_selection (info, selection, time);

    }

  return FALSE;
}

static void
ctk_drag_add_update_idle (CtkDragSourceInfo *info)
{
  /* We use an idle lower than CDK_PRIORITY_REDRAW so that exposes
   * from the last move can catch up before we move again.
   */
  if (!info->update_idle)
    {
      info->update_idle = cdk_threads_add_idle_full (CDK_PRIORITY_REDRAW + 5,
                                                     ctk_drag_update_idle,
                                                     info,
                                                     NULL);
      g_source_set_name_by_id (info->update_idle, "[ctk+] ctk_drag_update_idle");
    }
}

/*
 * ctk_drag_update:
 * @info: DragSourceInfo for the drag
 * @screen: new screen
 * @x_root: new X position 
 * @y_root: new y position
 * @event: event received requiring update
 * 
 * Updates the status of the drag; called when the
 * cursor moves or the modifier changes
 */
static void
ctk_drag_update (CtkDragSourceInfo *info,
                 CdkScreen         *screen,
                 gint               x_root,
                 gint               y_root,
                 const CdkEvent    *event)
{
  info->cur_screen = screen;
  info->cur_x = x_root;
  info->cur_y = y_root;
  if (info->last_event)
    {
      cdk_event_free ((CdkEvent *)info->last_event);
      info->last_event = NULL;
    }
  if (event)
    info->last_event = cdk_event_copy ((CdkEvent *)event);

  ctk_drag_add_update_idle (info);
}

/* Called when the user finishes to drag, either by
 * releasing the mouse, or by pressing Esc.
 */
static void
ctk_drag_end (CtkDragSourceInfo *info,
              guint32            time)
{
  CdkDevice *pointer, *keyboard;

  pointer = cdk_drag_context_get_device (info->context);
  keyboard = cdk_device_get_associated_device (pointer);

  /* Prevent ungrab before grab (see bug 623865) */
  if (info->grab_time == CDK_CURRENT_TIME)
    time = CDK_CURRENT_TIME;

  if (info->update_idle)
    {
      g_source_remove (info->update_idle);
      info->update_idle = 0;
    }
  
  if (info->last_event)
    {
      cdk_event_free (info->last_event);
      info->last_event = NULL;
    }
  
  info->have_grab = FALSE;
  
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
                                        ctk_drag_grab_broken_event_cb,
                                        info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
                                        ctk_drag_grab_notify_cb,
                                        info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
                                        ctk_drag_button_release_cb,
                                        info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
                                        ctk_drag_motion_cb,
                                        info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
                                        ctk_drag_key_cb,
                                        info);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  cdk_device_ungrab (pointer, time);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  ungrab_dnd_keys (info->ipc_widget, keyboard, time);
  ctk_device_grab_remove (info->ipc_widget, pointer);
}

/* Called on cancellation of a drag, either by the user
 * or programmatically.
 */
static void
ctk_drag_cancel_internal (CtkDragSourceInfo *info,
                          CtkDragResult      result,
                          guint32            time)
{
  ctk_drag_end (info, time);
  cdk_drag_abort (info->context, time);
  ctk_drag_drop_finished (info, result, time);
}

static void
ctk_drag_context_drop_performed_cb (CdkDragContext    *context,
                                    guint32            time_,
                                    CtkDragSourceInfo *info)
{
  ctk_drag_end (info, time_);
  ctk_drag_drop (info, time_);
}

static void
ctk_drag_context_cancel_cb (CdkDragContext      *context,
                            CdkDragCancelReason  reason,
                            CtkDragSourceInfo   *info)
{
  CtkDragResult result;

  switch (reason)
    {
    case CDK_DRAG_CANCEL_NO_TARGET:
      result = CTK_DRAG_RESULT_NO_TARGET;
      break;
    case CDK_DRAG_CANCEL_USER_CANCELLED:
      result = CTK_DRAG_RESULT_USER_CANCELLED;
      break;
    case CDK_DRAG_CANCEL_ERROR:
    default:
      result = CTK_DRAG_RESULT_ERROR;
      break;
    }
  ctk_drag_cancel_internal (info, result, CDK_CURRENT_TIME);
}

static void
ctk_drag_context_action_cb (CdkDragContext    *context,
                            CdkDragAction      action,
                            CtkDragSourceInfo *info)
{
  if (info->proxy_dest)
    {
      if (info->proxy_dest->proxy_drop_wait)
        {
          gboolean result = cdk_drag_context_get_selected_action (context) != 0;

          /* Aha - we can finally pass the DROP on... */
          cdk_drop_reply (info->proxy_dest->context, result, info->proxy_dest->proxy_drop_time);
          if (result)
            cdk_drag_drop (info->context, info->proxy_dest->proxy_drop_time);
          else
            ctk_drag_finish (info->proxy_dest->context, FALSE, FALSE, info->proxy_dest->proxy_drop_time);
        }
      else
        {
          cdk_drag_status (info->proxy_dest->context,
                           cdk_drag_context_get_selected_action (context),
                           CDK_CURRENT_TIME);
        }

      g_signal_stop_emission_by_name (context, "action");
    }
}

static void
ctk_drag_context_dnd_finished_cb (CdkDragContext    *context,
                                  CtkDragSourceInfo *info)
{
  ctk_drag_source_release_selections (info, CDK_CURRENT_TIME);

  if (info->proxy_dest)
    {
      /* The time from the event isn't reliable for Xdnd drags */
      ctk_drag_finish (info->proxy_dest->context, TRUE, FALSE,
                       info->proxy_dest->proxy_drop_time);
    }

  ctk_drag_source_info_destroy (info);
}

/* “motion-notify-event” callback during drag. */
static gboolean
ctk_drag_motion_cb (CtkWidget      *widget,
                    CdkEventMotion *event,
                    gpointer        data)
{
  CtkDragSourceInfo *info = (CtkDragSourceInfo *)data;
  CdkScreen *screen;
  gint x_root, y_root;

  if (event->is_hint)
    {
      cdk_device_get_position (event->device, &screen, &x_root, &y_root);
      event->x_root = x_root;
      event->y_root = y_root;
    }
  else
    screen = cdk_event_get_screen ((CdkEvent *)event);

  x_root = (gint)(event->x_root + 0.5);
  y_root = (gint)(event->y_root + 0.5);
  ctk_drag_update (info, screen, x_root, y_root, (CdkEvent *) event);

  return TRUE;
}

#define BIG_STEP 20
#define SMALL_STEP 1

/* “key-press/release-event” callback during drag */
static gboolean
ctk_drag_key_cb (CtkWidget   *widget, 
                 CdkEventKey *event, 
                 gpointer     data)
{
  CtkDragSourceInfo *info = (CtkDragSourceInfo *)data;
  CdkModifierType state;
  CdkWindow *root_window;
  CdkDevice *pointer;
  gint dx, dy;

  dx = dy = 0;
  state = event->state & ctk_accelerator_get_default_mod_mask ();
  pointer = cdk_device_get_associated_device (cdk_event_get_device ((CdkEvent *) event));

  if (event->type == CDK_KEY_PRESS)
    {
      switch (event->keyval)
        {
        case CDK_KEY_Escape:
          ctk_drag_cancel_internal (info, CTK_DRAG_RESULT_USER_CANCELLED, event->time);
          return TRUE;

        case CDK_KEY_space:
        case CDK_KEY_Return:
        case CDK_KEY_ISO_Enter:
        case CDK_KEY_KP_Enter:
        case CDK_KEY_KP_Space:
          if ((cdk_drag_context_get_selected_action (info->context) != 0) &&
              (cdk_drag_context_get_dest_window (info->context) != NULL))
            {
              ctk_drag_end (info, event->time);
              ctk_drag_drop (info, event->time);
            }
          else
            {
              ctk_drag_cancel_internal (info, CTK_DRAG_RESULT_NO_TARGET, event->time);
            }

          return TRUE;

        case CDK_KEY_Up:
        case CDK_KEY_KP_Up:
          dy = (state & CDK_MOD1_MASK) ? -BIG_STEP : -SMALL_STEP;
          break;

        case CDK_KEY_Down:
        case CDK_KEY_KP_Down:
          dy = (state & CDK_MOD1_MASK) ? BIG_STEP : SMALL_STEP;
          break;

        case CDK_KEY_Left:
        case CDK_KEY_KP_Left:
          dx = (state & CDK_MOD1_MASK) ? -BIG_STEP : -SMALL_STEP;
          break;

        case CDK_KEY_Right:
        case CDK_KEY_KP_Right:
          dx = (state & CDK_MOD1_MASK) ? BIG_STEP : SMALL_STEP;
          break;
        }
    }

  /* Now send a "motion" so that the modifier state is updated */

  /* The state is not yet updated in the event, so we need
   * to query it here. We could use XGetModifierMapping, but
   * that would be overkill.
   */
  root_window = cdk_screen_get_root_window (ctk_widget_get_screen (widget));
  cdk_window_get_device_position (root_window, pointer, NULL, NULL, &state);
  event->state = state;

  if (dx != 0 || dy != 0)
    {
      info->cur_x += dx;
      info->cur_y += dy;
      cdk_device_warp (pointer,
                       ctk_widget_get_screen (widget),
                       info->cur_x, info->cur_y);
    }

  ctk_drag_update (info, info->cur_screen, info->cur_x, info->cur_y, (CdkEvent *)event);

  return TRUE;
}

static gboolean
ctk_drag_grab_broken_event_cb (CtkWidget          *widget,
                               CdkEventGrabBroken *event,
                               gpointer            data)
{
  CtkDragSourceInfo *info = (CtkDragSourceInfo *)data;

  /* Don't cancel if we break the implicit grab from the initial button_press.
   * Also, don't cancel if we re-grab on the widget or on our IPC window, for
   * example, when changing the drag cursor.
   */
  if (event->implicit
      || event->grab_window == ctk_widget_get_window (info->widget)
      || event->grab_window == ctk_widget_get_window (info->ipc_widget))
    return FALSE;

  ctk_drag_cancel_internal (info, CTK_DRAG_RESULT_GRAB_BROKEN, ctk_get_current_event_time ());
  return TRUE;
}

static void
ctk_drag_grab_notify_cb (CtkWidget *widget,
                         gboolean   was_grabbed,
                         gpointer   data)
{
  CtkDragSourceInfo *info = (CtkDragSourceInfo *)data;
  CdkDevice *pointer;

  pointer = cdk_drag_context_get_device (info->context);

  if (ctk_widget_device_is_shadowed (widget, pointer))
    {
      /* We have to block callbacks to avoid recursion here, because
       * ctk_drag_cancel_internal calls ctk_grab_remove (via ctk_drag_end)
       */
      g_signal_handlers_block_by_func (widget, ctk_drag_grab_notify_cb, data);
      ctk_drag_cancel_internal (info, CTK_DRAG_RESULT_GRAB_BROKEN, ctk_get_current_event_time ());
      g_signal_handlers_unblock_by_func (widget, ctk_drag_grab_notify_cb, data);
    }
}

/* “button-release-event” callback during drag */
static gboolean
ctk_drag_button_release_cb (CtkWidget      *widget, 
                            CdkEventButton *event, 
                            gpointer        data)
{
  CtkDragSourceInfo *info = (CtkDragSourceInfo *)data;

  if (event->button != info->button)
    return FALSE;

  if ((cdk_drag_context_get_selected_action (info->context) != 0) &&
      (cdk_drag_context_get_dest_window (info->context) != NULL))
    {
      ctk_drag_end (info, event->time);
      ctk_drag_drop (info, event->time);
    }
  else
    {
      ctk_drag_cancel_internal (info, CTK_DRAG_RESULT_NO_TARGET, event->time);
    }

  return TRUE;
}

static gboolean
ctk_drag_abort_timeout (gpointer data)
{
  CtkDragSourceInfo *info = data;
  guint32 time = CDK_CURRENT_TIME;

  if (info->proxy_dest)
    time = info->proxy_dest->proxy_drop_time;

  info->drop_timeout = 0;
  ctk_drag_drop_finished (info, CTK_DRAG_RESULT_TIMEOUT_EXPIRED, time);
  
  return FALSE;
}

/**
 * ctk_drag_check_threshold: (method)
 * @widget: a #CtkWidget
 * @start_x: X coordinate of start of drag
 * @start_y: Y coordinate of start of drag
 * @current_x: current X coordinate
 * @current_y: current Y coordinate
 * 
 * Checks to see if a mouse drag starting at (@start_x, @start_y) and ending
 * at (@current_x, @current_y) has passed the CTK+ drag threshold, and thus
 * should trigger the beginning of a drag-and-drop operation.
 *
 * Returns: %TRUE if the drag threshold has been passed.
 */
gboolean
ctk_drag_check_threshold (CtkWidget *widget,
                          gint       start_x,
                          gint       start_y,
                          gint       current_x,
                          gint       current_y)
{
  gint drag_threshold;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  drag_threshold = ctk_settings_get_dnd_drag_threshold (ctk_widget_get_settings (widget));

  return (ABS (current_x - start_x) > drag_threshold ||
          ABS (current_y - start_y) > drag_threshold);
}

/**
 * ctk_drag_cancel: (method)
 * @context: a #CdkDragContext, as e.g. returned by ctk_drag_begin_with_coordinates()
 *
 * Cancels an ongoing drag operation on the source side.
 *
 * If you want to be able to cancel a drag operation in this way,
 * you need to keep a pointer to the drag context, either from an
 * explicit call to ctk_drag_begin_with_coordinates(), or by
 * connecting to #CtkWidget::drag-begin.
 *
 * If @context does not refer to an ongoing drag operation, this
 * function does nothing.
 *
 * If a drag is cancelled in this way, the @result argument of
 * #CtkWidget::drag-failed is set to @CTK_DRAG_RESULT_ERROR.
 *
 * Since: 3.16
 */
void
ctk_drag_cancel (CdkDragContext *context)
{
  CtkDragSourceInfo *info;

  g_return_if_fail (CDK_IS_DRAG_CONTEXT (context));

  info = ctk_drag_get_source_info (context, FALSE);
  if (info != NULL)
    ctk_drag_cancel_internal (info, CTK_DRAG_RESULT_ERROR, ctk_get_current_event_time ());
}
