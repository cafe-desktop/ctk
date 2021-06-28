/* GDK - The GIMP Drawing Kit
 * Copyright (C) 2009 Carlos Garnacho <carlosg@gnome.org>
 * Copyright (C) 2010 Kristian Rietveld <kris@ctk.org>
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

#include <cdk/cdkdeviceprivate.h>
#include <cdk/cdkdisplayprivate.h>

#import "CdkQuartzView.h"
#include "cdkquartzwindow.h"
#include "cdkquartzcursor.h"
#include "cdkprivate-quartz.h"
#include "cdkquartzdevice-core.h"
#include "cdkinternal-quartz.h"

struct _CdkQuartzDeviceCore
{
  CdkDevice parent_instance;

  gboolean active;
  NSUInteger device_id;
  unsigned long long unique_id;
};

struct _CdkQuartzDeviceCoreClass
{
  CdkDeviceClass parent_class;
};

static gboolean cdk_quartz_device_core_get_history (CdkDevice      *device,
                                                    CdkWindow      *window,
                                                    guint32         start,
                                                    guint32         stop,
                                                    CdkTimeCoord ***events,
                                                    gint           *n_events);
static void cdk_quartz_device_core_get_state (CdkDevice       *device,
                                              CdkWindow       *window,
                                              gdouble         *axes,
                                              CdkModifierType *mask);
static void cdk_quartz_device_core_set_window_cursor (CdkDevice *device,
                                                      CdkWindow *window,
                                                      CdkCursor *cursor);
static void cdk_quartz_device_core_warp (CdkDevice *device,
                                         CdkScreen *screen,
                                         gdouble    x,
                                         gdouble    y);
static void cdk_quartz_device_core_query_state (CdkDevice        *device,
                                                CdkWindow        *window,
                                                CdkWindow       **root_window,
                                                CdkWindow       **child_window,
                                                gdouble          *root_x,
                                                gdouble          *root_y,
                                                gdouble          *win_x,
                                                gdouble          *win_y,
                                                CdkModifierType  *mask);
static CdkGrabStatus cdk_quartz_device_core_grab   (CdkDevice     *device,
                                                    CdkWindow     *window,
                                                    gboolean       owner_events,
                                                    CdkEventMask   event_mask,
                                                    CdkWindow     *confine_to,
                                                    CdkCursor     *cursor,
                                                    guint32        time_);
static void          cdk_quartz_device_core_ungrab (CdkDevice     *device,
                                                    guint32        time_);
static CdkWindow * cdk_quartz_device_core_window_at_position (CdkDevice       *device,
                                                              gdouble         *win_x,
                                                              gdouble         *win_y,
                                                              CdkModifierType *mask,
                                                              gboolean         get_toplevel);
static void      cdk_quartz_device_core_select_window_events (CdkDevice       *device,
                                                              CdkWindow       *window,
                                                              CdkEventMask     event_mask);


G_DEFINE_TYPE (CdkQuartzDeviceCore, cdk_quartz_device_core, GDK_TYPE_DEVICE)

static void
cdk_quartz_device_core_class_init (CdkQuartzDeviceCoreClass *klass)
{
  CdkDeviceClass *device_class = GDK_DEVICE_CLASS (klass);

  device_class->get_history = cdk_quartz_device_core_get_history;
  device_class->get_state = cdk_quartz_device_core_get_state;
  device_class->set_window_cursor = cdk_quartz_device_core_set_window_cursor;
  device_class->warp = cdk_quartz_device_core_warp;
  device_class->query_state = cdk_quartz_device_core_query_state;
  device_class->grab = cdk_quartz_device_core_grab;
  device_class->ungrab = cdk_quartz_device_core_ungrab;
  device_class->window_at_position = cdk_quartz_device_core_window_at_position;
  device_class->select_window_events = cdk_quartz_device_core_select_window_events;
}

static void
cdk_quartz_device_core_init (CdkQuartzDeviceCore *quartz_device_core)
{
  CdkDevice *device;

  device = GDK_DEVICE (quartz_device_core);

  _cdk_device_add_axis (device, GDK_NONE, GDK_AXIS_X, 0, 0, 1);
  _cdk_device_add_axis (device, GDK_NONE, GDK_AXIS_Y, 0, 0, 1);
}

static gboolean
cdk_quartz_device_core_get_history (CdkDevice      *device,
                                    CdkWindow      *window,
                                    guint32         start,
                                    guint32         stop,
                                    CdkTimeCoord ***events,
                                    gint           *n_events)
{
  return FALSE;
}

static void
cdk_quartz_device_core_get_state (CdkDevice       *device,
                                  CdkWindow       *window,
                                  gdouble         *axes,
                                  CdkModifierType *mask)
{
  gint x_int, y_int;

  cdk_window_get_device_position (window, device, &x_int, &y_int, mask);

  if (axes)
    {
      axes[0] = x_int;
      axes[1] = y_int;
    }
}

static void
translate_coords_to_child_coords (CdkWindow *parent,
                                  CdkWindow *child,
                                  gint      *x,
                                  gint      *y)
{
  CdkWindow *current = child;

  if (child == parent)
    return;

  while (current != parent)
    {
      gint tmp_x, tmp_y;

      cdk_window_get_origin (current, &tmp_x, &tmp_y);

      *x -= tmp_x;
      *y -= tmp_y;

      current = cdk_window_get_effective_parent (current);
    }
}

static void
cdk_quartz_device_core_set_window_cursor (CdkDevice *device,
                                          CdkWindow *window,
                                          CdkCursor *cursor)
{
  NSCursor *nscursor;

  if (GDK_WINDOW_DESTROYED (window))
    return;

  nscursor = _cdk_quartz_cursor_get_ns_cursor (cursor);

  [nscursor set];
}

static void
cdk_quartz_device_core_warp (CdkDevice *device,
                             CdkScreen *screen,
                             gdouble    x,
                             gdouble    y)
{
  CGDisplayMoveCursorToPoint (CGMainDisplayID (), CGPointMake (x, y));
}

static CdkWindow *
cdk_quartz_device_core_query_state_helper (CdkWindow       *window,
                                           CdkDevice       *device,
                                           gdouble         *x,
                                           gdouble         *y,
                                           CdkModifierType *mask)
{
  CdkWindow *toplevel;
  NSPoint point;
  gint x_tmp, y_tmp;
  CdkWindow *found_window;

  g_return_val_if_fail (window == NULL || GDK_IS_WINDOW (window), NULL);

  if (GDK_WINDOW_DESTROYED (window))
    {
      *x = 0;
      *y = 0;
      *mask = 0;
      return NULL;
    }

  toplevel = cdk_window_get_effective_toplevel (window);

  if (mask)
    *mask = _cdk_quartz_events_get_current_keyboard_modifiers () |
        _cdk_quartz_events_get_current_mouse_modifiers ();

  /* Get the y coordinate, needs to be flipped. */
  if (window == _cdk_root)
    {
      point = [NSEvent mouseLocation];
      _cdk_quartz_window_nspoint_to_cdk_xy (point, &x_tmp, &y_tmp);
    }
  else
    {
      CdkWindowImplQuartz *impl;
      NSWindow *nswindow;

      impl = GDK_WINDOW_IMPL_QUARTZ (toplevel->impl);
      nswindow = impl->toplevel;

      point = [nswindow mouseLocationOutsideOfEventStream];

      x_tmp = point.x;
      y_tmp = toplevel->height - point.y;

      window = toplevel;
    }

  found_window = _cdk_quartz_window_find_child (window, x_tmp, y_tmp,
                                                FALSE);

  if (found_window == _cdk_root)
    found_window = NULL;
  else if (found_window)
    translate_coords_to_child_coords (window, found_window,
                                      &x_tmp, &y_tmp);

  if (x)
    *x = x_tmp;

  if (y)
    *y = y_tmp;

  return found_window;
}

static void
cdk_quartz_device_core_query_state (CdkDevice        *device,
                                    CdkWindow        *window,
                                    CdkWindow       **root_window,
                                    CdkWindow       **child_window,
                                    gdouble          *root_x,
                                    gdouble          *root_y,
                                    gdouble          *win_x,
                                    gdouble          *win_y,
                                    CdkModifierType  *mask)
{
  CdkWindow *found_window;
  NSPoint point;
  gint x_tmp, y_tmp;

  found_window = cdk_quartz_device_core_query_state_helper (window, device,
                                                            win_x, win_y,
                                                            mask);

  if (root_window)
    *root_window = _cdk_root;

  if (child_window)
    *child_window = found_window;

  point = [NSEvent mouseLocation];
  _cdk_quartz_window_nspoint_to_cdk_xy (point, &x_tmp, &y_tmp);

  if (root_x)
    *root_x = x_tmp;

  if (root_y)
    *root_y = y_tmp;
}

static CdkGrabStatus
cdk_quartz_device_core_grab (CdkDevice    *device,
                             CdkWindow    *window,
                             gboolean      owner_events,
                             CdkEventMask  event_mask,
                             CdkWindow    *confine_to,
                             CdkCursor    *cursor,
                             guint32       time_)
{
  /* Should remain empty */
  return GDK_GRAB_SUCCESS;
}

static void
cdk_quartz_device_core_ungrab (CdkDevice *device,
                               guint32    time_)
{
  CdkDeviceGrabInfo *grab;

  grab = _cdk_display_get_last_device_grab (_cdk_display, device);
  if (grab)
    grab->serial_end = 0;

  _cdk_display_device_grab_update (_cdk_display, device, NULL, 0);
}

static CdkWindow *
cdk_quartz_device_core_window_at_position (CdkDevice       *device,
                                           gdouble         *win_x,
                                           gdouble         *win_y,
                                           CdkModifierType *mask,
                                           gboolean         get_toplevel)
{
  CdkDisplay *display;
  CdkScreen *screen;
  CdkWindow *found_window;
  NSPoint point;
  gint x_tmp, y_tmp;

  display = cdk_device_get_display (device);
  screen = cdk_display_get_default_screen (display);

  /* Get mouse coordinates, find window under the mouse pointer */
  point = [NSEvent mouseLocation];
  _cdk_quartz_window_nspoint_to_cdk_xy (point, &x_tmp, &y_tmp);

  found_window = _cdk_quartz_window_find_child (_cdk_root, x_tmp, y_tmp,
                                                get_toplevel);

  if (found_window)
    translate_coords_to_child_coords (_cdk_root, found_window,
                                      &x_tmp, &y_tmp);

  if (win_x)
    *win_x = found_window ? x_tmp : -1;

  if (win_y)
    *win_y = found_window ? y_tmp : -1;

  if (mask)
    *mask = _cdk_quartz_events_get_current_keyboard_modifiers () |
        _cdk_quartz_events_get_current_mouse_modifiers ();

  return found_window;
}

static void
cdk_quartz_device_core_select_window_events (CdkDevice    *device,
                                             CdkWindow    *window,
                                             CdkEventMask  event_mask)
{
  /* The mask is set in the common code. */
}

void
_cdk_quartz_device_core_set_active (CdkDevice  *device,
                                    gboolean    active,
                                    NSUInteger  device_id)
{
  CdkQuartzDeviceCore *self = GDK_QUARTZ_DEVICE_CORE (device);

  self->active = active;
  self->device_id = device_id;
}

gboolean
_cdk_quartz_device_core_is_active (CdkDevice  *device,
                                   NSUInteger  device_id)
{
  CdkQuartzDeviceCore *self = GDK_QUARTZ_DEVICE_CORE (device);

  return (self->active && self->device_id == device_id);
}

void
_cdk_quartz_device_core_set_unique (CdkDevice          *device,
                                    unsigned long long  unique_id)
{
  GDK_QUARTZ_DEVICE_CORE (device)->unique_id = unique_id;
}

unsigned long long
_cdk_quartz_device_core_get_unique (CdkDevice *device)
{
  return GDK_QUARTZ_DEVICE_CORE (device)->unique_id;
}
