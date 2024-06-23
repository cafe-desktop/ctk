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


#include "cdkx11device-core.h"
#include "cdkdeviceprivate.h"

#include "cdkinternals.h"
#include "cdkwindow.h"
#include "cdkprivate-x11.h"
#include "cdkasync.h"

#include <math.h>

/* for the use of round() */
#include "fallback-c89.c"

struct _CdkX11DeviceCore
{
  CdkDevice parent_instance;
};

struct _CdkX11DeviceCoreClass
{
  CdkDeviceClass parent_class;
};

static gboolean cdk_x11_device_core_get_history (CdkDevice       *device,
                                                 CdkWindow       *window,
                                                 guint32          start,
                                                 guint32          stop,
                                                 CdkTimeCoord  ***events,
                                                 gint            *n_events);
static void     cdk_x11_device_core_get_state   (CdkDevice       *device,
                                                 CdkWindow       *window,
                                                 gdouble         *axes,
                                                 CdkModifierType *mask);
static void     cdk_x11_device_core_set_window_cursor (CdkDevice *device,
                                                       CdkWindow *window,
                                                       CdkCursor *cursor);
static void     cdk_x11_device_core_warp (CdkDevice *device,
                                          CdkScreen *screen,
                                          gdouble    x,
                                          gdouble    y);
static void cdk_x11_device_core_query_state (CdkDevice        *device,
                                             CdkWindow        *window,
                                             CdkWindow       **root_window,
                                             CdkWindow       **child_window,
                                             gdouble          *root_x,
                                             gdouble          *root_y,
                                             gdouble          *win_x,
                                             gdouble          *win_y,
                                             CdkModifierType  *mask);
static CdkGrabStatus cdk_x11_device_core_grab   (CdkDevice     *device,
                                                 CdkWindow     *window,
                                                 gboolean       owner_events,
                                                 CdkEventMask   event_mask,
                                                 CdkWindow     *confine_to,
                                                 CdkCursor     *cursor,
                                                 guint32        time_);
static void          cdk_x11_device_core_ungrab (CdkDevice     *device,
                                                 guint32        time_);
static CdkWindow * cdk_x11_device_core_window_at_position (CdkDevice       *device,
                                                           gdouble         *win_x,
                                                           gdouble         *win_y,
                                                           CdkModifierType *mask,
                                                           gboolean         get_toplevel);
static void      cdk_x11_device_core_select_window_events (CdkDevice       *device,
                                                           CdkWindow       *window,
                                                           CdkEventMask     event_mask);

G_DEFINE_TYPE (CdkX11DeviceCore, cdk_x11_device_core, CDK_TYPE_DEVICE)

static void
cdk_x11_device_core_class_init (CdkX11DeviceCoreClass *klass)
{
  CdkDeviceClass *device_class = CDK_DEVICE_CLASS (klass);

  device_class->get_history = cdk_x11_device_core_get_history;
  device_class->get_state = cdk_x11_device_core_get_state;
  device_class->set_window_cursor = cdk_x11_device_core_set_window_cursor;
  device_class->warp = cdk_x11_device_core_warp;
  device_class->query_state = cdk_x11_device_core_query_state;
  device_class->grab = cdk_x11_device_core_grab;
  device_class->ungrab = cdk_x11_device_core_ungrab;
  device_class->window_at_position = cdk_x11_device_core_window_at_position;
  device_class->select_window_events = cdk_x11_device_core_select_window_events;
}

static void
cdk_x11_device_core_init (CdkX11DeviceCore *device_core)
{
  CdkDevice *device;

  device = CDK_DEVICE (device_core);

  _cdk_device_add_axis (device, CDK_NONE, CDK_AXIS_X, 0, 0, 1);
  _cdk_device_add_axis (device, CDK_NONE, CDK_AXIS_Y, 0, 0, 1);
}

static gboolean
impl_coord_in_window (CdkWindow *window,
		      int        impl_x,
		      int        impl_y)
{
  if (impl_x < window->abs_x ||
      impl_x >= window->abs_x + window->width)
    return FALSE;

  if (impl_y < window->abs_y ||
      impl_y >= window->abs_y + window->height)
    return FALSE;

  return TRUE;
}

static gboolean
cdk_x11_device_core_get_history (CdkDevice      *device,
                                 CdkWindow      *window,
                                 guint32         start,
                                 guint32         stop,
                                 CdkTimeCoord ***events,
                                 gint           *n_events)
{
  XTimeCoord *xcoords;
  CdkTimeCoord **coords;
  CdkWindow *impl_window;
  CdkWindowImplX11 *impl;
  int tmp_n_events;
  int i, j;

  impl_window = _cdk_window_get_impl_window (window);
  impl =  CDK_WINDOW_IMPL_X11 (impl_window->impl);
  xcoords = XGetMotionEvents (CDK_WINDOW_XDISPLAY (window),
                              CDK_WINDOW_XID (impl_window),
                              start, stop, &tmp_n_events);
  if (!xcoords)
    return FALSE;

  coords = _cdk_device_allocate_history (device, tmp_n_events);

  for (i = 0, j = 0; i < tmp_n_events; i++)
    {
      if (impl_coord_in_window (window,
                                xcoords[i].x / impl->window_scale,
                                xcoords[i].y / impl->window_scale))
        {
          coords[j]->time = xcoords[i].time;
          coords[j]->axes[0] = (double)xcoords[i].x / impl->window_scale - window->abs_x;
          coords[j]->axes[1] = (double)xcoords[i].y / impl->window_scale - window->abs_y;
          j++;
        }
    }

  XFree (xcoords);

  /* free the events we allocated too much */
  for (i = j; i < tmp_n_events; i++)
    {
      g_free (coords[i]);
      coords[i] = NULL;
    }

  tmp_n_events = j;

  if (tmp_n_events == 0)
    {
      cdk_device_free_history (coords, tmp_n_events);
      return FALSE;
    }

  if (n_events)
    *n_events = tmp_n_events;

  if (events)
    *events = coords;
  else if (coords)
    cdk_device_free_history (coords, tmp_n_events);

  return TRUE;
}

static void
cdk_x11_device_core_get_state (CdkDevice       *device,
                               CdkWindow       *window,
                               gdouble         *axes,
                               CdkModifierType *mask)
{
  gdouble x, y;

  cdk_window_get_device_position_double (window, device, &x, &y, mask);

  if (axes)
    {
      axes[0] = x;
      axes[1] = y;
    }
}

static void
cdk_x11_device_core_set_window_cursor (CdkDevice *device G_GNUC_UNUSED,
                                       CdkWindow *window,
                                       CdkCursor *cursor)
{
  Cursor xcursor;

  if (!cursor)
    xcursor = None;
  else
    xcursor = cdk_x11_cursor_get_xcursor (cursor);

  XDefineCursor (CDK_WINDOW_XDISPLAY (window),
                 CDK_WINDOW_XID (window),
                 xcursor);
}

static void
cdk_x11_device_core_warp (CdkDevice *device,
                          CdkScreen *screen,
                          gdouble    x,
                          gdouble    y)
{
  Display *xdisplay;
  Window dest;

  xdisplay = CDK_DISPLAY_XDISPLAY (cdk_device_get_display (device));
  dest = CDK_WINDOW_XID (cdk_screen_get_root_window (screen));

  XWarpPointer (xdisplay, None, dest, 0, 0, 0, 0,
                round (x * CDK_X11_SCREEN (screen)->window_scale),
                round (y * CDK_X11_SCREEN (screen)->window_scale));
}

static void
cdk_x11_device_core_query_state (CdkDevice        *device G_GNUC_UNUSED,
                                 CdkWindow        *window,
                                 CdkWindow       **root_window,
                                 CdkWindow       **child_window,
                                 gdouble          *root_x,
                                 gdouble          *root_y,
                                 gdouble          *win_x,
                                 gdouble          *win_y,
                                 CdkModifierType  *mask)
{
  CdkWindowImplX11 *impl = CDK_WINDOW_IMPL_X11 (window->impl);
  CdkDisplay *display;
  CdkScreen *default_screen;
  Window xroot_window, xchild_window;
  int xroot_x, xroot_y, xwin_x, xwin_y;
  unsigned int xmask;

  display = cdk_window_get_display (window);
  default_screen = cdk_display_get_default_screen (display);

  if (!CDK_X11_DISPLAY (display)->trusted_client ||
      !XQueryPointer (CDK_WINDOW_XDISPLAY (window),
                      CDK_WINDOW_XID (window),
                      &xroot_window,
                      &xchild_window,
                      &xroot_x, &xroot_y,
                      &xwin_x, &xwin_y,
                      &xmask))
    {
      XSetWindowAttributes attributes;
      Display *xdisplay;
      Window xwindow, w;

      /* FIXME: untrusted clients not multidevice-safe */
      xdisplay = CDK_SCREEN_XDISPLAY (default_screen);
      xwindow = CDK_SCREEN_XROOTWIN (default_screen);

      w = XCreateWindow (xdisplay, xwindow, 0, 0, 1, 1, 0,
                         CopyFromParent, InputOnly, CopyFromParent,
                         0, &attributes);
      XQueryPointer (xdisplay, w,
                     &xroot_window,
                     &xchild_window,
                     &xroot_x, &xroot_y,
                     &xwin_x, &xwin_y,
                     &xmask);
      XDestroyWindow (xdisplay, w);
    }

  if (root_window)
    *root_window = cdk_x11_window_lookup_for_display (display, xroot_window);

  if (child_window)
    *child_window = cdk_x11_window_lookup_for_display (display, xchild_window);

  if (root_x)
    *root_x = (double)xroot_x / impl->window_scale;

  if (root_y)
    *root_y = (double)xroot_y / impl->window_scale;

  if (win_x)
    *win_x = (double)xwin_x / impl->window_scale;

  if (win_y)
    *win_y = (double)xwin_y / impl->window_scale;

  if (mask)
    *mask = xmask;
}

static CdkGrabStatus
cdk_x11_device_core_grab (CdkDevice    *device,
                          CdkWindow    *window,
                          gboolean      owner_events,
                          CdkEventMask  event_mask,
                          CdkWindow    *confine_to,
                          CdkCursor    *cursor,
                          guint32       time_)
{
  CdkDisplay *display;
  Window xwindow, xconfine_to;
  gint status;

  display = cdk_device_get_display (device);

  xwindow = CDK_WINDOW_XID (window);

  if (confine_to)
    confine_to = _cdk_window_get_impl_window (confine_to);

  if (!confine_to || CDK_WINDOW_DESTROYED (confine_to))
    xconfine_to = None;
  else
    xconfine_to = CDK_WINDOW_XID (confine_to);

#ifdef G_ENABLE_DEBUG
  if (CDK_DEBUG_CHECK (NOGRABS))
    status = GrabSuccess;
  else
#endif
  if (cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD)
    {
      /* Device is a keyboard */
      status = XGrabKeyboard (CDK_DISPLAY_XDISPLAY (display),
                              xwindow,
                              owner_events,
                              GrabModeAsync, GrabModeAsync,
                              time_);
    }
  else
    {
      Cursor xcursor;
      guint xevent_mask;
      gint i;

      /* Device is a pointer */
      if (!cursor)
        xcursor = None;
      else
        {
          _cdk_x11_cursor_update_theme (cursor);
          xcursor = cdk_x11_cursor_get_xcursor (cursor);
        }

      xevent_mask = 0;

      for (i = 0; i < _cdk_x11_event_mask_table_size; i++)
        {
          if (event_mask & (1 << (i + 1)))
            xevent_mask |= _cdk_x11_event_mask_table[i];
        }

      /* We don't want to set a native motion hint mask, as we're emulating motion
       * hints. If we set a native one we just wouldn't get any events.
       */
      xevent_mask &= ~PointerMotionHintMask;

      status = XGrabPointer (CDK_DISPLAY_XDISPLAY (display),
                             xwindow,
                             owner_events,
                             xevent_mask,
                             GrabModeAsync, GrabModeAsync,
                             xconfine_to,
                             xcursor,
                             time_);
    }

  _cdk_x11_display_update_grab_info (display, device, status);

  return _cdk_x11_convert_grab_status (status);
}

static void
cdk_x11_device_core_ungrab (CdkDevice *device,
                            guint32    time_)
{
  CdkDisplay *display;
  gulong serial;

  display = cdk_device_get_display (device);
  serial = NextRequest (CDK_DISPLAY_XDISPLAY (display));

  if (cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD)
    XUngrabKeyboard (CDK_DISPLAY_XDISPLAY (display), time_);
  else
    XUngrabPointer (CDK_DISPLAY_XDISPLAY (display), time_);

  _cdk_x11_display_update_grab_info_ungrab (display, device, time_, serial);
}

static CdkWindow *
cdk_x11_device_core_window_at_position (CdkDevice       *device,
                                        gdouble         *win_x,
                                        gdouble         *win_y,
                                        CdkModifierType *mask,
                                        gboolean         get_toplevel)
{
  CdkWindowImplX11 *impl;
  CdkDisplay *display;
  CdkScreen *screen;
  Display *xdisplay;
  CdkWindow *window;
  Window xwindow, root, child, last;
  int xroot_x, xroot_y, xwin_x, xwin_y;
  unsigned int xmask;

  last = None;
  display = cdk_device_get_display (device);
  screen = cdk_display_get_default_screen (display);

  /* This function really only works if the mouse pointer is held still
   * during its operation. If it moves from one leaf window to another
   * than we'll end up with inaccurate values for win_x, win_y
   * and the result.
   */
  cdk_x11_display_grab (display);

  xdisplay = CDK_SCREEN_XDISPLAY (screen);
  xwindow = CDK_SCREEN_XROOTWIN (screen);

  if (G_LIKELY (CDK_X11_DISPLAY (display)->trusted_client))
    {
      XQueryPointer (xdisplay, xwindow,
                     &root, &child,
                     &xroot_x, &xroot_y,
                     &xwin_x, &xwin_y,
                     &xmask);

      if (root == xwindow)
        xwindow = child;
      else
       xwindow = root;
    }
  else
    {
      gint width, height;
      GList *toplevels, *list;
      Window pointer_window;
      int rootx = -1, rooty = -1;
      int winx, winy;

      /* FIXME: untrusted clients case not multidevice-safe */
      pointer_window = None;
      screen = cdk_display_get_default_screen (display);
      toplevels = cdk_screen_get_toplevel_windows (screen);
      for (list = toplevels; list != NULL; list = list->next)
        {
          window = CDK_WINDOW (list->data);
          impl = CDK_WINDOW_IMPL_X11 (window->impl);
          xwindow = CDK_WINDOW_XID (window);
          cdk_x11_display_error_trap_push (display);
          XQueryPointer (xdisplay, xwindow,
                         &root, &child,
                         &rootx, &rooty,
                         &winx, &winy,
                         &xmask);
          if (cdk_x11_display_error_trap_pop (display))
            continue;
          if (child != None)
            {
              pointer_window = child;
              break;
            }
          cdk_window_get_geometry (window, NULL, NULL, &width, &height);
          if (winx >= 0 && winy >= 0 && winx < width * impl->window_scale && winy < height * impl->window_scale)
            {
              /* A childless toplevel, or below another window? */
              XSetWindowAttributes attributes;
              Window w;

              w = XCreateWindow (xdisplay, xwindow, winx, winy, 1, 1, 0,
                                 CopyFromParent, InputOnly, CopyFromParent,
                                 0, &attributes);
              XMapWindow (xdisplay, w);
              XQueryPointer (xdisplay, xwindow,
                             &root, &child,
                             &rootx, &rooty,
                             &winx, &winy,
                             &xmask);
              XDestroyWindow (xdisplay, w);
              if (child == w)
                {
                  pointer_window = xwindow;
                  break;
                }
            }
        }

      g_list_free (toplevels);

      xwindow = pointer_window;
    }

  while (xwindow)
    {
      last = xwindow;
      cdk_x11_display_error_trap_push (display);
      XQueryPointer (xdisplay, xwindow,
                     &root, &xwindow,
                     &xroot_x, &xroot_y,
                     &xwin_x, &xwin_y,
                     &xmask);
      if (cdk_x11_display_error_trap_pop (display))
        break;

      if (get_toplevel && last != root &&
          (window = cdk_x11_window_lookup_for_display (display, last)) != NULL &&
          window->window_type != CDK_WINDOW_FOREIGN)
        {
          xwindow = last;
          break;
        }
    }

  cdk_x11_display_ungrab (display);

  window = cdk_x11_window_lookup_for_display (display, last);
  impl = NULL;
  if (window)
    impl = CDK_WINDOW_IMPL_X11 (window->impl);

  if (win_x)
    *win_x = (window) ? (double)xwin_x / impl->window_scale : -1;

  if (win_y)
    *win_y = (window) ? (double)xwin_y / impl->window_scale : -1;

  if (mask)
    *mask = xmask;

  return window;
}

static void
cdk_x11_device_core_select_window_events (CdkDevice    *device G_GNUC_UNUSED,
                                          CdkWindow    *window,
                                          CdkEventMask  event_mask)
{
  CdkEventMask filter_mask, window_mask;
  guint xmask = 0;
  gint i;

  window_mask = cdk_window_get_events (window);
  filter_mask = CDK_POINTER_MOTION_MASK
                | CDK_POINTER_MOTION_HINT_MASK
                | CDK_BUTTON_MOTION_MASK
                | CDK_BUTTON1_MOTION_MASK
                | CDK_BUTTON2_MOTION_MASK
                | CDK_BUTTON3_MOTION_MASK
                | CDK_BUTTON_PRESS_MASK
                | CDK_BUTTON_RELEASE_MASK
                | CDK_KEY_PRESS_MASK
                | CDK_KEY_RELEASE_MASK
                | CDK_ENTER_NOTIFY_MASK
                | CDK_LEAVE_NOTIFY_MASK
                | CDK_FOCUS_CHANGE_MASK
                | CDK_PROXIMITY_IN_MASK
                | CDK_PROXIMITY_OUT_MASK
                | CDK_SCROLL_MASK;

  /* Filter out non-device events */
  event_mask &= filter_mask;

  /* Unset device events on window mask */
  window_mask &= ~filter_mask;

  /* Combine masks */
  event_mask |= window_mask;

  for (i = 0; i < _cdk_x11_event_mask_table_size; i++)
    {
      if (event_mask & (1 << (i + 1)))
        xmask |= _cdk_x11_event_mask_table[i];
    }

  if (CDK_WINDOW_XID (window) != CDK_WINDOW_XROOTWIN (window))
    xmask |= StructureNotifyMask | PropertyChangeMask;

  XSelectInput (CDK_WINDOW_XDISPLAY (window),
                CDK_WINDOW_XID (window),
                xmask);
}
