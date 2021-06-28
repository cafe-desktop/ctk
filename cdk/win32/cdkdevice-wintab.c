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

#include <cdk/cdkwindow.h>

#include <windowsx.h>
#include <objbase.h>

#include "cdkwin32.h"
#include "cdkdevice-wintab.h"

G_DEFINE_TYPE (CdkDeviceWintab, cdk_device_wintab, CDK_TYPE_DEVICE)

static gboolean
cdk_device_wintab_get_history (CdkDevice      *device,
                               CdkWindow      *window,
                               guint32         start,
                               guint32         stop,
                               CdkTimeCoord ***events,
                               gint           *n_events)
{
  return FALSE;
}

static CdkModifierType
get_current_mask (void)
{
  CdkModifierType mask;
  BYTE kbd[256];

  GetKeyboardState (kbd);
  mask = 0;
  if (kbd[VK_SHIFT] & 0x80)
    mask |= CDK_SHIFT_MASK;
  if (kbd[VK_CAPITAL] & 0x80)
    mask |= CDK_LOCK_MASK;
  if (kbd[VK_CONTROL] & 0x80)
    mask |= CDK_CONTROL_MASK;
  if (kbd[VK_MENU] & 0x80)
    mask |= CDK_MOD1_MASK;
  if (kbd[VK_LBUTTON] & 0x80)
    mask |= CDK_BUTTON1_MASK;
  if (kbd[VK_MBUTTON] & 0x80)
    mask |= CDK_BUTTON2_MASK;
  if (kbd[VK_RBUTTON] & 0x80)
    mask |= CDK_BUTTON3_MASK;

  return mask;
}

static void
cdk_device_wintab_get_state (CdkDevice       *device,
                             CdkWindow       *window,
                             gdouble         *axes,
                             CdkModifierType *mask)
{
  CdkDeviceWintab *device_wintab;

  device_wintab = CDK_DEVICE_WINTAB (device);

  /* For now just use the last known button and axis state of the device.
   * Since graphical tablets send an insane amount of motion events each
   * second, the info should be fairly up to date */
  if (mask)
    {
      *mask = get_current_mask ();
      *mask &= 0xFF; /* Mask away core pointer buttons */
      *mask |= ((device_wintab->button_state << 8)
                & (CDK_BUTTON1_MASK | CDK_BUTTON2_MASK
                   | CDK_BUTTON3_MASK | CDK_BUTTON4_MASK
                   | CDK_BUTTON5_MASK));
    }

  if (axes && device_wintab->last_axis_data)
    _cdk_device_wintab_translate_axes (device_wintab, window, axes, NULL, NULL);
}

static void
cdk_device_wintab_set_window_cursor (CdkDevice *device,
                                     CdkWindow *window,
                                     CdkCursor *cursor)
{
}

static void
cdk_device_wintab_warp (CdkDevice *device,
                        CdkScreen *screen,
                        gdouble   x,
                        gdouble   y)
{
}

static void
cdk_device_wintab_query_state (CdkDevice        *device,
                               CdkWindow        *window,
                               CdkWindow       **root_window,
                               CdkWindow       **child_window,
                               gdouble          *root_x,
                               gdouble          *root_y,
                               gdouble          *win_x,
                               gdouble          *win_y,
                               CdkModifierType  *mask)
{
  CdkDeviceWintab *device_wintab;
  CdkScreen *screen;
  POINT point;
  HWND hwnd, hwndc;
  CdkWindowImplWin32 *impl;

  device_wintab = CDK_DEVICE_WINTAB (device);
  screen = cdk_window_get_screen (window);
  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  hwnd = CDK_WINDOW_HWND (window);
  GetCursorPos (&point);

  if (root_x)
    *root_x = point.x / impl->window_scale;

  if (root_y)
    *root_y = point.y / impl->window_scale;

  ScreenToClient (hwnd, &point);

  if (win_x)
    *win_x = point.x / impl->window_scale;

  if (win_y)
    *win_y = point.y / impl->window_scale;

  if (window == cdk_get_default_root_window ())
    {
      if (win_x)
        *win_x += _cdk_offset_x;

      if (win_y)
        *win_y += _cdk_offset_y;
    }

  if (child_window)
    {
      hwndc = ChildWindowFromPoint (hwnd, point);

      if (hwndc && hwndc != hwnd)
        *child_window = cdk_win32_handle_table_lookup (hwndc);
      else
        *child_window = NULL; /* Direct child unknown to cdk */
    }

  if (root_window)
    *root_window = cdk_screen_get_root_window (screen);

  if (mask)
    {
      *mask = get_current_mask ();
      *mask &= 0xFF; /* Mask away core pointer buttons */
      *mask |= ((device_wintab->button_state << 8)
                & (CDK_BUTTON1_MASK | CDK_BUTTON2_MASK
                   | CDK_BUTTON3_MASK | CDK_BUTTON4_MASK
                   | CDK_BUTTON5_MASK));

    }
}

static CdkGrabStatus
cdk_device_wintab_grab (CdkDevice    *device,
                        CdkWindow    *window,
                        gboolean      owner_events,
                        CdkEventMask  event_mask,
                        CdkWindow    *confine_to,
                        CdkCursor    *cursor,
                        guint32       time_)
{
  return CDK_GRAB_SUCCESS;
}

static void
cdk_device_wintab_ungrab (CdkDevice *device,
                          guint32    time_)
{
}

static CdkWindow *
cdk_device_wintab_window_at_position (CdkDevice       *device,
                                      gdouble         *win_x,
                                      gdouble         *win_y,
                                      CdkModifierType *mask,
                                      gboolean         get_toplevel)
{
  return NULL;
}

static void
cdk_device_wintab_select_window_events (CdkDevice    *device,
                                        CdkWindow    *window,
                                        CdkEventMask  event_mask)
{
}

void
_cdk_device_wintab_translate_axes (CdkDeviceWintab *device_wintab,
                                   CdkWindow       *window,
                                   gdouble         *axes,
                                   gdouble         *x,
                                   gdouble         *y)
{
  CdkDevice *device;
  CdkWindow *impl_window;
  gint root_x, root_y;
  gdouble temp_x, temp_y;
  gint i;

  device = CDK_DEVICE (device_wintab);
  impl_window = _cdk_window_get_impl_window (window);
  temp_x = temp_y = 0;

  cdk_window_get_origin (impl_window, &root_x, &root_y);

  for (i = 0; i < cdk_device_get_n_axes (device); i++)
    {
      CdkAxisUse use;

      use = cdk_device_get_axis_use (device, i);

      switch (use)
        {
        case CDK_AXIS_X:
        case CDK_AXIS_Y:
          if (cdk_device_get_mode (device) == CDK_MODE_WINDOW)
            _cdk_device_translate_window_coord (device, window, i,
                                                device_wintab->last_axis_data[i],
                                                &axes[i]);
          else
            _cdk_device_translate_screen_coord (device, window,
                                                root_x, root_y, i,
                                                device_wintab->last_axis_data[i],
                                                &axes[i]);
          if (use == CDK_AXIS_X)
            temp_x = axes[i];
          else if (use == CDK_AXIS_Y)
            temp_y = axes[i];

          break;
        default:
          _cdk_device_translate_axis (device, i,
                                      device_wintab->last_axis_data[i],
                                      &axes[i]);
          break;
        }
    }

  if (x)
    *x = temp_x;

  if (y)
    *y = temp_y;
}

static void
cdk_device_wintab_class_init (CdkDeviceWintabClass *klass)
{
  CdkDeviceClass *device_class = CDK_DEVICE_CLASS (klass);

  device_class->get_history = cdk_device_wintab_get_history;
  device_class->get_state = cdk_device_wintab_get_state;
  device_class->set_window_cursor = cdk_device_wintab_set_window_cursor;
  device_class->warp = cdk_device_wintab_warp;
  device_class->query_state = cdk_device_wintab_query_state;
  device_class->grab = cdk_device_wintab_grab;
  device_class->ungrab = cdk_device_wintab_ungrab;
  device_class->window_at_position = cdk_device_wintab_window_at_position;
  device_class->select_window_events = cdk_device_wintab_select_window_events;
}

static void
cdk_device_wintab_init (CdkDeviceWintab *device_wintab)
{
}
