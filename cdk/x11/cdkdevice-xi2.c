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

#include "cdkx11device-xi2.h"
#include "cdkdeviceprivate.h"
#include "cdkdisplayprivate.h"

#include "cdkintl.h"
#include "cdkasync.h"
#include "cdkprivate-x11.h"

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>

#include <math.h>

/* for the use of round() */
#include "fallback-c89.c"

typedef struct _ScrollValuator ScrollValuator;

struct _ScrollValuator
{
  guint n_valuator       : 4;
  guint direction        : 4;
  guint last_value_valid : 1;
  gdouble last_value;
  gdouble increment;
};

struct _CdkX11DeviceXI2
{
  CdkDevice parent_instance;

  gint device_id;
  GArray *scroll_valuators;
  gdouble *last_axes;
};

struct _CdkX11DeviceXI2Class
{
  CdkDeviceClass parent_class;
};

G_DEFINE_TYPE (CdkX11DeviceXI2, cdk_x11_device_xi2, CDK_TYPE_DEVICE)


static void cdk_x11_device_xi2_finalize     (GObject      *object);
static void cdk_x11_device_xi2_get_property (GObject      *object,
                                             guint         prop_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);
static void cdk_x11_device_xi2_set_property (GObject      *object,
                                             guint         prop_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);

static void cdk_x11_device_xi2_get_state (CdkDevice       *device,
                                          CdkWindow       *window,
                                          gdouble         *axes,
                                          CdkModifierType *mask);
static void cdk_x11_device_xi2_set_window_cursor (CdkDevice *device,
                                                  CdkWindow *window,
                                                  CdkCursor *cursor);
static void cdk_x11_device_xi2_warp (CdkDevice *device,
                                     CdkScreen *screen,
                                     gdouble    x,
                                     gdouble    y);
static void cdk_x11_device_xi2_query_state (CdkDevice        *device,
                                            CdkWindow        *window,
                                            CdkWindow       **root_window,
                                            CdkWindow       **child_window,
                                            gdouble          *root_x,
                                            gdouble          *root_y,
                                            gdouble          *win_x,
                                            gdouble          *win_y,
                                            CdkModifierType  *mask);

static CdkGrabStatus cdk_x11_device_xi2_grab   (CdkDevice     *device,
                                                CdkWindow     *window,
                                                gboolean       owner_events,
                                                CdkEventMask   event_mask,
                                                CdkWindow     *confine_to,
                                                CdkCursor     *cursor,
                                                guint32        time_);
static void          cdk_x11_device_xi2_ungrab (CdkDevice     *device,
                                                guint32        time_);

static CdkWindow * cdk_x11_device_xi2_window_at_position (CdkDevice       *device,
                                                          gdouble         *win_x,
                                                          gdouble         *win_y,
                                                          CdkModifierType *mask,
                                                          gboolean         get_toplevel);
static void  cdk_x11_device_xi2_select_window_events (CdkDevice    *device,
                                                      CdkWindow    *window,
                                                      CdkEventMask  event_mask);


enum {
  PROP_0,
  PROP_DEVICE_ID
};

static void
cdk_x11_device_xi2_class_init (CdkX11DeviceXI2Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkDeviceClass *device_class = CDK_DEVICE_CLASS (klass);

  object_class->finalize = cdk_x11_device_xi2_finalize;
  object_class->get_property = cdk_x11_device_xi2_get_property;
  object_class->set_property = cdk_x11_device_xi2_set_property;

  device_class->get_state = cdk_x11_device_xi2_get_state;
  device_class->set_window_cursor = cdk_x11_device_xi2_set_window_cursor;
  device_class->warp = cdk_x11_device_xi2_warp;
  device_class->query_state = cdk_x11_device_xi2_query_state;
  device_class->grab = cdk_x11_device_xi2_grab;
  device_class->ungrab = cdk_x11_device_xi2_ungrab;
  device_class->window_at_position = cdk_x11_device_xi2_window_at_position;
  device_class->select_window_events = cdk_x11_device_xi2_select_window_events;

  g_object_class_install_property (object_class,
                                   PROP_DEVICE_ID,
                                   g_param_spec_int ("device-id",
                                                     P_("Device ID"),
                                                     P_("Device identifier"),
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
cdk_x11_device_xi2_init (CdkX11DeviceXI2 *device)
{
  device->scroll_valuators = g_array_new (FALSE, FALSE, sizeof (ScrollValuator));
}

static void
cdk_x11_device_xi2_finalize (GObject *object)
{
  CdkX11DeviceXI2 *device = CDK_X11_DEVICE_XI2 (object);

  g_array_free (device->scroll_valuators, TRUE);
  g_free (device->last_axes);

  G_OBJECT_CLASS (cdk_x11_device_xi2_parent_class)->finalize (object);
}

static void
cdk_x11_device_xi2_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  CdkX11DeviceXI2 *device_xi2 = CDK_X11_DEVICE_XI2 (object);

  switch (prop_id)
    {
    case PROP_DEVICE_ID:
      g_value_set_int (value, device_xi2->device_id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
cdk_x11_device_xi2_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  CdkX11DeviceXI2 *device_xi2 = CDK_X11_DEVICE_XI2 (object);

  switch (prop_id)
    {
    case PROP_DEVICE_ID:
      device_xi2->device_id = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
cdk_x11_device_xi2_get_state (CdkDevice       *device,
                              CdkWindow       *window,
                              gdouble         *axes,
                              CdkModifierType *mask)
{
  CdkX11DeviceXI2 *device_xi2 = CDK_X11_DEVICE_XI2 (device);

  if (axes)
    {
      CdkDisplay *display;
      XIDeviceInfo *info;
      gint i, j, ndevices;

      display = cdk_device_get_display (device);

      cdk_x11_display_error_trap_push (display);
      info = XIQueryDevice (CDK_DISPLAY_XDISPLAY (display),
                            device_xi2->device_id, &ndevices);
      cdk_x11_display_error_trap_pop_ignored (display);

      for (i = 0, j = 0; info && i < info->num_classes; i++)
        {
          XIAnyClassInfo *class_info = info->classes[i];
          CdkAxisUse use;
          gdouble value;

          if (class_info->type != XIValuatorClass)
            continue;

          value = ((XIValuatorClassInfo *) class_info)->value;
          use = cdk_device_get_axis_use (device, j);

          switch (use)
            {
            case CDK_AXIS_X:
            case CDK_AXIS_Y:
            case CDK_AXIS_IGNORE:
              if (cdk_device_get_mode (device) == CDK_MODE_WINDOW)
                _cdk_device_translate_window_coord (device, window, j, value, &axes[j]);
              else
                {
                  gint root_x, root_y;

                  /* FIXME: Maybe root coords chaching should happen here */
                  cdk_window_get_origin (window, &root_x, &root_y);
                  _cdk_device_translate_screen_coord (device, window,
                                                      root_x, root_y,
                                                      j, value,
                                                      &axes[j]);
                }
              break;
            default:
              _cdk_device_translate_axis (device, j, value, &axes[j]);
              break;
            }

          j++;
        }

      if (info)
        XIFreeDeviceInfo (info);
    }

  if (mask)
    cdk_x11_device_xi2_query_state (device, window,
                                    NULL, NULL,
                                    NULL, NULL,
                                    NULL, NULL,
                                    mask);
}

static void
cdk_x11_device_xi2_set_window_cursor (CdkDevice *device,
                                      CdkWindow *window,
                                      CdkCursor *cursor)
{
  CdkX11DeviceXI2 *device_xi2 = CDK_X11_DEVICE_XI2 (device);

  /* Non-master devices don't have a cursor */
  if (cdk_device_get_device_type (device) != CDK_DEVICE_TYPE_MASTER)
    return;

  if (cursor)
    XIDefineCursor (CDK_WINDOW_XDISPLAY (window),
                    device_xi2->device_id,
                    CDK_WINDOW_XID (window),
                    cdk_x11_cursor_get_xcursor (cursor));
  else
    {
      /* While a grab is active, a NULL cursor on the grab window itself
       * must not strip the cursor the grab defined there: update_cursor()
       * sends it when the pointer hovers cursor-less windows of the
       * grabbed hierarchy, and undefining would make the server walk past
       * the grab window up to its ancestors, where the window manager
       * frame windows can carry a stale cursor.  Grabs taken with an
       * explicit cursor show that cursor regardless of the window
       * attributes, so their grab window is left untouched as well.  On
       * any other window a NULL applies the cursor the grab was taken
       * with: cursor-less windows applying their cursor while the grab
       * lasts keep that image instead of walking up the window ancestors,
       * where the window manager frame windows can carry a stale cursor. */
      CdkDisplay *display = cdk_device_get_display (device);
      CdkDeviceGrabInfo *grab;
      Cursor grab_cursor = None;

      grab = _cdk_display_get_last_device_grab (display, device);
      if (grab != NULL)
        {
          if (window == grab->window)
            return;

          grab_cursor = _cdk_x11_display_get_grab_current_cursor (display);
        }

      if (grab_cursor != None)
        XIDefineCursor (CDK_WINDOW_XDISPLAY (window),
                        device_xi2->device_id,
                        CDK_WINDOW_XID (window),
                        grab_cursor);
      else if (cdk_window_get_window_type (window) == CDK_WINDOW_TOPLEVEL)
        {
          /* Outside of grabs, leaving one of our own toplevels bare hands
           * the cursor lookup to the server, which resolves it walking up
           * the window's ancestors: the first window beyond our toplevel
           * is the window manager frame, carrying whatever cursor the WM
           * applied last on the side the pointer crossed.  The WM stops
           * receiving pointer events for as long as one of our grabs
           * lasts, so that image is frozen meanwhile and can be stale by
           * the time the grab ends: with the pointer parked on a resize
           * border, opening a menu with the keyboard and clicking off it
           * into the window makes update_cursor() strip the remembered
           * image the grab applied on the following enter, and the
           * frame's stale resize cursor then shows up all over the client
           * area until the pointer reaches the title bar or a corner and
           * the WM redefines its frame cursor.  Define the theme's default
           * arrow instead: it is the image the server would show walking
           * past the bare toplevel anyway, and it keeps the frame windows
           * out of the lookup for as long as the pointer is over our
           * windows, whatever the frame carries.  A theme that fails to
           * provide the arrow keeps the old behavior. */
          CdkCursor *arrow = _cdk_x11_display_get_default_arrow_cursor (display);

          if (arrow != NULL)
            XIDefineCursor (CDK_WINDOW_XDISPLAY (window),
                            device_xi2->device_id,
                            CDK_WINDOW_XID (window),
                            cdk_x11_cursor_get_xcursor (arrow));
          else
            XIUndefineCursor (CDK_WINDOW_XDISPLAY (window),
                              device_xi2->device_id,
                              CDK_WINDOW_XID (window));
        }
      else
        XIUndefineCursor (CDK_WINDOW_XDISPLAY (window),
                          device_xi2->device_id,
                          CDK_WINDOW_XID (window));
    }
}

static void
cdk_x11_device_xi2_warp (CdkDevice *device,
                         CdkScreen *screen,
                         gdouble    x,
                         gdouble    y)
{
  CdkX11DeviceXI2 *device_xi2 = CDK_X11_DEVICE_XI2 (device);
  Window dest;

  dest = CDK_WINDOW_XID (cdk_screen_get_root_window (screen));

  XIWarpPointer (CDK_SCREEN_XDISPLAY (screen),
                 device_xi2->device_id,
                 None, dest,
                 0, 0, 0, 0,
                 round (x * CDK_X11_SCREEN(screen)->window_scale),
                 round (y * CDK_X11_SCREEN(screen)->window_scale));
}

static void
cdk_x11_device_xi2_query_state (CdkDevice        *device,
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
  CdkX11DeviceXI2 *device_xi2 = CDK_X11_DEVICE_XI2 (device);
  CdkDisplay *display;
  CdkScreen *default_screen;
  Window xroot_window, xchild_window;
  gdouble xroot_x, xroot_y, xwin_x, xwin_y;
  XIButtonState button_state;
  XIModifierState mod_state;
  XIGroupState group_state;

  display = cdk_window_get_display (window);
  default_screen = cdk_display_get_default_screen (display);

  if (cdk_device_get_device_type (device) == CDK_DEVICE_TYPE_SLAVE)
    {
      CdkDevice *master = cdk_device_get_associated_device (device);

      if (master)
        _cdk_device_query_state (master, window, root_window, child_window,
                                 root_x, root_y, win_x, win_y, mask);
      return;
    }

  if (!CDK_X11_DISPLAY (display)->trusted_client ||
      !XIQueryPointer (CDK_WINDOW_XDISPLAY (window),
                       device_xi2->device_id,
                       CDK_WINDOW_XID (window),
                       &xroot_window,
                       &xchild_window,
                       &xroot_x, &xroot_y,
                       &xwin_x, &xwin_y,
                       &button_state,
                       &mod_state,
                       &group_state))
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
      XIQueryPointer (xdisplay, device_xi2->device_id,
                      w,
                      &xroot_window,
                      &xchild_window,
                      &xroot_x, &xroot_y,
                      &xwin_x, &xwin_y,
                      &button_state,
                      &mod_state,
                      &group_state);
      XDestroyWindow (xdisplay, w);
    }

  if (root_window)
    *root_window = cdk_x11_window_lookup_for_display (display, xroot_window);

  if (child_window)
    *child_window = cdk_x11_window_lookup_for_display (display, xchild_window);

  if (root_x)
    *root_x = xroot_x / impl->window_scale;

  if (root_y)
    *root_y = xroot_y / impl->window_scale;

  if (win_x)
    *win_x = xwin_x / impl->window_scale;

  if (win_y)
    *win_y = xwin_y / impl->window_scale;

  if (mask)
    *mask = _cdk_x11_device_xi2_translate_state (&mod_state, &button_state, &group_state);

  free (button_state.mask);
}

static CdkGrabStatus
cdk_x11_device_xi2_grab (CdkDevice    *device,
                         CdkWindow    *window,
                         gboolean      owner_events,
                         CdkEventMask  event_mask,
                         CdkWindow    *confine_to G_GNUC_UNUSED,
                         CdkCursor    *cursor,
                         guint32       time_)
{
  CdkX11DeviceXI2 *device_xi2 = CDK_X11_DEVICE_XI2 (device);
  CdkX11DeviceManagerXI2 *device_manager_xi2;
  CdkDisplay *display;
  XIEventMask mask;
  Window xwindow;
  Cursor xcursor;
  Cursor grab_cursor = None;
  CdkCursor *arrow;
  gint status;

  display = cdk_device_get_display (device);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  device_manager_xi2 = CDK_X11_DEVICE_MANAGER_XI2 (cdk_display_get_device_manager (display));
  G_GNUC_END_IGNORE_DEPRECATIONS;

  /* FIXME: confine_to is actually unused */

  xwindow = CDK_WINDOW_XID (window);

  if (!cursor)
    xcursor = None;
  else
    {
      _cdk_x11_cursor_update_theme (cursor);
      xcursor = cdk_x11_cursor_get_xcursor (cursor);
    }

  if (xcursor == None &&
      cdk_device_get_source (device) != CDK_SOURCE_KEYBOARD)
    {
      /* A grab without an explicit cursor makes the server display, for
       * every window outside the grabbed window, the cursor of the grab
       * window walking up its ancestors, and a grab window with no cursor
       * of its own keeps walking up past it.  The grab window here is the
       * native toplevel of the application, and its ancestors are the
       * window manager frame windows, whose cursor is whatever arrow the
       * WM applied last on the frame side the pointer crossed; a stale
       * cursor there then shows up all over the screen while the grab is
       * active (e.g. menus opened with the keyboard while the pointer is
       * parked outside the window).  Define the theme's default arrow on
       * the grab window so that does not happen, whatever the pointer is
       * showing: an application cursor of this client, one of another
       * client, or a cursor of the frame must not be pinned over the
       * whole screen by a keyboard-driven grab.  The arrow also matches
       * what a click does under the grab, the first click on a window of
       * another client only dismisses the menu.  Inside the application
       * the per-window cursors keep working through update_cursor(),
       * because the cursor argument passed to XIGrabDevice stays None. */
      arrow = _cdk_x11_display_get_default_arrow_cursor (display);

      if (arrow != NULL)
        grab_cursor = cdk_x11_cursor_get_xcursor (arrow);

      /* A theme that fails to provide the default arrow falls back to
       * freezing the exact image the pointer shows: the visible cursor
       * does not change at all, the stale frame cursor does not come
       * back, and the frozen image stays for as long as the grab
       * lasts. */
      if (grab_cursor == None)
        grab_cursor = _cdk_x11_display_get_current_cursor (display);

      if (grab_cursor != None)
        XIDefineCursor (CDK_DISPLAY_XDISPLAY (display),
                        device_xi2->device_id,
                        xwindow,
                        grab_cursor);
    }

  mask.deviceid = device_xi2->device_id;
  mask.mask = _cdk_x11_device_xi2_translate_event_mask (device_manager_xi2,
                                                        event_mask,
                                                        &mask.mask_len);

#ifdef G_ENABLE_DEBUG
  if (CDK_DEBUG_CHECK (NOGRABS))
    status = GrabSuccess;
  else
#endif
  status = XIGrabDevice (CDK_DISPLAY_XDISPLAY (display),
                         device_xi2->device_id,
                         xwindow,
                         time_,
                         xcursor,
                         GrabModeAsync, GrabModeAsync,
                         owner_events,
                         &mask);

  if (status == GrabSuccess &&
      cdk_device_get_source (device) != CDK_SOURCE_KEYBOARD)
    {
      /* Remember the cursor the grab was taken with: cursor-less windows
       * applying their cursor while the grab is active keep that image
       * instead of walking up the window ancestors, where the window
       * manager frame windows can carry a stale cursor.  A grab taken
       * with an explicit cursor remembers None instead: the server shows
       * that cursor regardless of the window attributes, so the windows
       * applying their NULL cursor while the grab lasts are just left
       * undefined. */
      _cdk_x11_display_set_grab_current_cursor (display, grab_cursor);
    }

  g_free (mask.mask);

  _cdk_x11_display_update_grab_info (display, device, status);

  return _cdk_x11_convert_grab_status (status);
}

static void
cdk_x11_device_xi2_ungrab (CdkDevice *device,
                           guint32    time_)
{
  CdkX11DeviceXI2 *device_xi2 = CDK_X11_DEVICE_XI2 (device);
  CdkDisplay *display;
  gulong serial;

  display = cdk_device_get_display (device);
  serial = NextRequest (CDK_DISPLAY_XDISPLAY (display));

  XIUngrabDevice (CDK_DISPLAY_XDISPLAY (display), device_xi2->device_id, time_);

  /* The device being ungrabbed can be a keyboard: releasing a keyboard grab
   * while the pointer grab of the device pair is still active must not erase
   * the image the pointer grab remembered, or the cursor-less windows of the
   * grabbed hierarchy applying their cursor for the rest of the grab are
   * undefined instead and walk up the window ancestors, where the window
   * manager frame windows can carry a stale cursor.  The core backend only
   * clears on pointer ungrabs too, and the XI2 grab only remembers the
   * cursor on non-keyboard devices. */
  if (cdk_device_get_source (device) != CDK_SOURCE_KEYBOARD)
    _cdk_x11_display_set_grab_current_cursor (display, None);

  _cdk_x11_display_update_grab_info_ungrab (display, device, time_, serial);
}

static CdkWindow *
cdk_x11_device_xi2_window_at_position (CdkDevice       *device,
                                       gdouble         *win_x,
                                       gdouble         *win_y,
                                       CdkModifierType *mask,
                                       gboolean         get_toplevel)
{
  CdkWindowImplX11 *impl;
  CdkX11DeviceXI2 *device_xi2 = CDK_X11_DEVICE_XI2 (device);
  CdkDisplay *display;
  CdkScreen *screen;
  Display *xdisplay;
  CdkWindow *window;
  Window xwindow, root, child, last = None;
  gdouble xroot_x, xroot_y, xwin_x, xwin_y;
  XIButtonState button_state = { 0 };
  XIModifierState mod_state;
  XIGroupState group_state;
  Bool retval;

  display = cdk_device_get_display (device);
  screen = cdk_display_get_default_screen (display);

  cdk_x11_display_error_trap_push (display);

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
      XIQueryPointer (xdisplay,
                      device_xi2->device_id,
                      xwindow,
                      &root, &child,
                      &xroot_x, &xroot_y,
                      &xwin_x, &xwin_y,
                      &button_state,
                      &mod_state,
                      &group_state);

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

      /* FIXME: untrusted clients case not multidevice-safe */
      pointer_window = None;

      screen = cdk_display_get_default_screen (display);
      toplevels = cdk_screen_get_toplevel_windows (screen);
      for (list = toplevels; list != NULL; list = list->next)
        {
          window = CDK_WINDOW (list->data);
          xwindow = CDK_WINDOW_XID (window);

          /* Free previous button mask, if any */
          g_free (button_state.mask);

          retval = XIQueryPointer (xdisplay,
                                   device_xi2->device_id,
                                   xwindow,
                                   &root, &child,
                                   &xroot_x, &xroot_y,
                                   &xwin_x, &xwin_y,
                                   &button_state,
                                   &mod_state,
                                   &group_state);
          if (!retval)
            continue;

          if (child != None)
            {
              pointer_window = child;
              break;
            }
          cdk_window_get_geometry (window, NULL, NULL, &width, &height);
          if (xwin_x >= 0 && xwin_y >= 0 && xwin_x < width && xwin_y < height)
            {
              /* A childless toplevel, or below another window? */
              XSetWindowAttributes attributes;
              Window w;

              free (button_state.mask);

              w = XCreateWindow (xdisplay, xwindow, (int)xwin_x, (int)xwin_y, 1, 1, 0,
                                 CopyFromParent, InputOnly, CopyFromParent,
                                 0, &attributes);
              XMapWindow (xdisplay, w);
              XIQueryPointer (xdisplay,
                              device_xi2->device_id,
                              xwindow,
                              &root, &child,
                              &xroot_x, &xroot_y,
                              &xwin_x, &xwin_y,
                              &button_state,
                              &mod_state,
                              &group_state);
              XDestroyWindow (xdisplay, w);
              if (child == w)
                {
                  pointer_window = xwindow;
                  break;
                }
            }

          g_list_free (toplevels);
          if (pointer_window != None)
            break;
        }

      xwindow = pointer_window;
    }

  while (xwindow)
    {
      last = xwindow;
      free (button_state.mask);

      retval = XIQueryPointer (xdisplay,
                               device_xi2->device_id,
                               xwindow,
                               &root, &xwindow,
                               &xroot_x, &xroot_y,
                               &xwin_x, &xwin_y,
                               &button_state,
                               &mod_state,
                               &group_state);
      if (!retval)
        break;

      if (get_toplevel && last != root &&
          (window = cdk_x11_window_lookup_for_display (display, last)) != NULL &&
          CDK_WINDOW_TYPE (window) != CDK_WINDOW_FOREIGN)
        {
          xwindow = last;
          break;
        }
    }

  cdk_x11_display_ungrab (display);

  if (cdk_x11_display_error_trap_pop (display) == 0)
    {
      window = cdk_x11_window_lookup_for_display (display, last);
      impl = NULL;
      if (window)
        impl = CDK_WINDOW_IMPL_X11 (window->impl);

      if (mask)
        *mask = _cdk_x11_device_xi2_translate_state (&mod_state, &button_state, &group_state);

      free (button_state.mask);
    }
  else
    {
      window = NULL;

      if (mask)
        *mask = 0;
    }

  if (win_x)
    *win_x = (window) ? (xwin_x / impl->window_scale) : -1;

  if (win_y)
    *win_y = (window) ? (xwin_y / impl->window_scale) : -1;


  return window;
}

static void
cdk_x11_device_xi2_select_window_events (CdkDevice    *device,
                                         CdkWindow    *window,
                                         CdkEventMask  event_mask)
{
  CdkX11DeviceXI2 *device_xi2 = CDK_X11_DEVICE_XI2 (device);
  CdkX11DeviceManagerXI2 *device_manager_xi2;
  CdkDisplay *display;
  XIEventMask evmask;

  display = cdk_device_get_display (device);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  device_manager_xi2 = CDK_X11_DEVICE_MANAGER_XI2 (cdk_display_get_device_manager (display));
  G_GNUC_END_IGNORE_DEPRECATIONS;

  evmask.deviceid = device_xi2->device_id;
  evmask.mask = _cdk_x11_device_xi2_translate_event_mask (device_manager_xi2,
                                                          event_mask,
                                                          &evmask.mask_len);

  XISelectEvents (CDK_WINDOW_XDISPLAY (window),
                  CDK_WINDOW_XID (window),
                  &evmask, 1);

  g_free (evmask.mask);
}

guchar *
_cdk_x11_device_xi2_translate_event_mask (CdkX11DeviceManagerXI2 *device_manager_xi2,
                                          CdkEventMask            event_mask,
                                          gint                   *len)
{
  guchar *mask;
  gint minor;

  g_object_get (device_manager_xi2, "minor", &minor, NULL);

  *len = XIMaskLen (XI_LASTEVENT);
  mask = g_new0 (guchar, *len);

  if (event_mask & CDK_POINTER_MOTION_MASK ||
      event_mask & CDK_POINTER_MOTION_HINT_MASK)
    XISetMask (mask, XI_Motion);

  if (event_mask & CDK_BUTTON_MOTION_MASK ||
      event_mask & CDK_BUTTON1_MOTION_MASK ||
      event_mask & CDK_BUTTON2_MOTION_MASK ||
      event_mask & CDK_BUTTON3_MOTION_MASK)
    {
      XISetMask (mask, XI_ButtonPress);
      XISetMask (mask, XI_ButtonRelease);
      XISetMask (mask, XI_Motion);
    }

  if (event_mask & CDK_SCROLL_MASK)
    {
      XISetMask (mask, XI_ButtonPress);
      XISetMask (mask, XI_ButtonRelease);
    }

  if (event_mask & CDK_BUTTON_PRESS_MASK)
    XISetMask (mask, XI_ButtonPress);

  if (event_mask & CDK_BUTTON_RELEASE_MASK)
    XISetMask (mask, XI_ButtonRelease);

  if (event_mask & CDK_KEY_PRESS_MASK)
    XISetMask (mask, XI_KeyPress);

  if (event_mask & CDK_KEY_RELEASE_MASK)
    XISetMask (mask, XI_KeyRelease);

  if (event_mask & CDK_ENTER_NOTIFY_MASK)
    XISetMask (mask, XI_Enter);

  if (event_mask & CDK_LEAVE_NOTIFY_MASK)
    XISetMask (mask, XI_Leave);

  if (event_mask & CDK_FOCUS_CHANGE_MASK)
    {
      XISetMask (mask, XI_FocusIn);
      XISetMask (mask, XI_FocusOut);
    }

#ifdef XINPUT_2_2
  /* XInput 2.2 includes multitouch support */
  if (minor >= 2 &&
      event_mask & CDK_TOUCH_MASK)
    {
      XISetMask (mask, XI_TouchBegin);
      XISetMask (mask, XI_TouchUpdate);
      XISetMask (mask, XI_TouchEnd);
    }
#endif /* XINPUT_2_2 */

  return mask;
}

guint
_cdk_x11_device_xi2_translate_state (XIModifierState *mods_state,
                                     XIButtonState   *buttons_state,
                                     XIGroupState    *group_state)
{
  guint state = 0;

  if (mods_state)
    state = mods_state->effective;

  if (buttons_state)
    {
      gint len, i;

      /* We're only interested in the first 3 buttons */
      len = MIN (3, buttons_state->mask_len * 8);

      for (i = 1; i <= len; i++)
        {
          if (!XIMaskIsSet (buttons_state->mask, i))
            continue;

          switch (i)
            {
            case 1:
              state |= CDK_BUTTON1_MASK;
              break;
            case 2:
              state |= CDK_BUTTON2_MASK;
              break;
            case 3:
              state |= CDK_BUTTON3_MASK;
              break;
            default:
              break;
            }
        }
    }

  if (group_state)
    state |= (group_state->effective) << 13;

  return state;
}

void
_cdk_x11_device_xi2_add_scroll_valuator (CdkX11DeviceXI2    *device,
                                         guint               n_valuator,
                                         CdkScrollDirection  direction,
                                         gdouble             increment)
{
  ScrollValuator scroll;

  g_return_if_fail (CDK_IS_X11_DEVICE_XI2 (device));
  g_return_if_fail (n_valuator < cdk_device_get_n_axes (CDK_DEVICE (device)));

  scroll.n_valuator = n_valuator;
  scroll.direction = direction;
  scroll.last_value_valid = FALSE;
  scroll.increment = increment;

  g_array_append_val (device->scroll_valuators, scroll);
}

gboolean
_cdk_x11_device_xi2_get_scroll_delta (CdkX11DeviceXI2    *device,
                                      guint               n_valuator,
                                      gdouble             valuator_value,
                                      CdkScrollDirection *direction_ret,
                                      gdouble            *delta_ret)
{
  guint i;

  g_return_val_if_fail (CDK_IS_X11_DEVICE_XI2 (device), FALSE);
  g_return_val_if_fail (n_valuator < cdk_device_get_n_axes (CDK_DEVICE (device)), FALSE);

  for (i = 0; i < device->scroll_valuators->len; i++)
    {
      ScrollValuator *scroll;

      scroll = &g_array_index (device->scroll_valuators, ScrollValuator, i);

      if (scroll->n_valuator == n_valuator)
        {
          if (direction_ret)
            *direction_ret = scroll->direction;

          if (delta_ret)
            *delta_ret = 0;

          if (scroll->last_value_valid)
            {
              if (delta_ret)
                *delta_ret = (valuator_value - scroll->last_value) / scroll->increment;

              scroll->last_value = valuator_value;
            }
          else
            {
              scroll->last_value = valuator_value;
              scroll->last_value_valid = TRUE;
            }

          return TRUE;
        }
    }

  return FALSE;
}

void
_cdk_device_xi2_reset_scroll_valuators (CdkX11DeviceXI2 *device)
{
  guint i;

  for (i = 0; i < device->scroll_valuators->len; i++)
    {
      ScrollValuator *scroll;

      scroll = &g_array_index (device->scroll_valuators, ScrollValuator, i);
      scroll->last_value_valid = FALSE;
    }
}

void
_cdk_device_xi2_unset_scroll_valuators (CdkX11DeviceXI2 *device)
{
  if (device->scroll_valuators->len > 0)
    g_array_remove_range (device->scroll_valuators, 0,
                          device->scroll_valuators->len);
}

gint
_cdk_x11_device_xi2_get_id (CdkX11DeviceXI2 *device)
{
  g_return_val_if_fail (CDK_IS_X11_DEVICE_XI2 (device), 0);

  return device->device_id;
}

gdouble
cdk_x11_device_xi2_get_last_axis_value (CdkX11DeviceXI2 *device,
                                        gint             n_axis)
{
  if (n_axis >= cdk_device_get_n_axes (CDK_DEVICE (device)))
    return 0;

  if (!device->last_axes)
    return 0;

  return device->last_axes[n_axis];
}

void
cdk_x11_device_xi2_store_axes (CdkX11DeviceXI2 *device,
                               gdouble         *axes,
                               gint             n_axes)
{
  g_free (device->last_axes);

  if (axes && n_axes)
    device->last_axes = g_memdup2 (axes, sizeof (gdouble) * n_axes);
  else
    device->last_axes = NULL;
}
