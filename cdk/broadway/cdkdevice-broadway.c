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
#include <stdlib.h>

#include "cdkdevice-broadway.h"

#include "cdkwindow.h"
#include "cdkprivate-broadway.h"

static gboolean cdk_broadway_device_get_history (CdkDevice      *device,
						 CdkWindow      *window,
						 guint32         start,
						 guint32         stop,
						 CdkTimeCoord ***events,
						 gint           *n_events);
static void cdk_broadway_device_get_state (CdkDevice       *device,
					   CdkWindow       *window,
					   gdouble         *axes,
					   CdkModifierType *mask);
static void cdk_broadway_device_set_window_cursor (CdkDevice *device,
						   CdkWindow *window,
						   CdkCursor *cursor);
static void cdk_broadway_device_warp (CdkDevice *device,
				      CdkScreen *screen,
				      gdouble    x,
				      gdouble    y);
static void cdk_broadway_device_query_state (CdkDevice        *device,
                                             CdkWindow        *window,
                                             CdkWindow       **root_window,
                                             CdkWindow       **child_window,
                                             gdouble          *root_x,
                                             gdouble          *root_y,
                                             gdouble          *win_x,
                                             gdouble          *win_y,
                                             CdkModifierType  *mask);
static CdkGrabStatus cdk_broadway_device_grab   (CdkDevice     *device,
						 CdkWindow     *window,
						 gboolean       owner_events,
						 CdkEventMask   event_mask,
						 CdkWindow     *confine_to,
						 CdkCursor     *cursor,
						 guint32        time_);
static void          cdk_broadway_device_ungrab (CdkDevice     *device,
						 guint32        time_);
static CdkWindow * cdk_broadway_device_window_at_position (CdkDevice       *device,
							   gdouble         *win_x,
							   gdouble         *win_y,
							   CdkModifierType *mask,
							   gboolean         get_toplevel);
static void      cdk_broadway_device_select_window_events (CdkDevice       *device,
							   CdkWindow       *window,
							   CdkEventMask     event_mask);


G_DEFINE_TYPE (CdkBroadwayDevice, cdk_broadway_device, CDK_TYPE_DEVICE)

static void
cdk_broadway_device_class_init (CdkBroadwayDeviceClass *klass)
{
  CdkDeviceClass *device_class = CDK_DEVICE_CLASS (klass);

  device_class->get_history = cdk_broadway_device_get_history;
  device_class->get_state = cdk_broadway_device_get_state;
  device_class->set_window_cursor = cdk_broadway_device_set_window_cursor;
  device_class->warp = cdk_broadway_device_warp;
  device_class->query_state = cdk_broadway_device_query_state;
  device_class->grab = cdk_broadway_device_grab;
  device_class->ungrab = cdk_broadway_device_ungrab;
  device_class->window_at_position = cdk_broadway_device_window_at_position;
  device_class->select_window_events = cdk_broadway_device_select_window_events;
}

static void
cdk_broadway_device_init (CdkBroadwayDevice *device_core)
{
  CdkDevice *device;

  device = CDK_DEVICE (device_core);

  _cdk_device_add_axis (device, CDK_NONE, CDK_AXIS_X, 0, 0, 1);
  _cdk_device_add_axis (device, CDK_NONE, CDK_AXIS_Y, 0, 0, 1);
}

static gboolean
cdk_broadway_device_get_history (CdkDevice      *device,
				 CdkWindow      *window,
				 guint32         start,
				 guint32         stop,
				 CdkTimeCoord ***events,
				 gint           *n_events)
{
  return FALSE;
}

static void
cdk_broadway_device_get_state (CdkDevice       *device,
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
cdk_broadway_device_set_window_cursor (CdkDevice *device,
				       CdkWindow *window,
				       CdkCursor *cursor)
{
}

static void
cdk_broadway_device_warp (CdkDevice *device,
			  CdkScreen *screen,
			  gdouble    x,
			  gdouble    y)
{
}

static void
cdk_broadway_device_query_state (CdkDevice        *device,
				 CdkWindow        *window,
				 CdkWindow       **root_window,
				 CdkWindow       **child_window,
				 gdouble          *root_x,
				 gdouble          *root_y,
				 gdouble          *win_x,
				 gdouble          *win_y,
				 CdkModifierType  *mask)
{
  CdkWindow *toplevel;
  CdkWindowImplBroadway *impl;
  CdkDisplay *display;
  CdkBroadwayDisplay *broadway_display;
  CdkScreen *screen;
  gint32 device_root_x, device_root_y;
  guint32 mouse_toplevel_id;
  CdkWindow *mouse_toplevel;
  guint32 mask32;

  if (cdk_device_get_source (device) != CDK_SOURCE_MOUSE)
    return;

  display = cdk_device_get_display (device);
  broadway_display = CDK_BROADWAY_DISPLAY (display);

  impl = CDK_WINDOW_IMPL_BROADWAY (window->impl);
  toplevel = impl->wrapper;

  if (root_window)
    {
      screen = cdk_window_get_screen (window);
      *root_window = cdk_screen_get_root_window (screen);
    }

  _cdk_broadway_server_query_mouse (broadway_display->server,
				    &mouse_toplevel_id,
				    &device_root_x,
				    &device_root_y,
				    &mask32);
  mouse_toplevel = g_hash_table_lookup (broadway_display->id_ht, GUINT_TO_POINTER (mouse_toplevel_id));

  if (root_x)
    *root_x = device_root_x;
  if (root_y)
    *root_y = device_root_y;
  if (win_x)
    *win_x = device_root_x - toplevel->x;
  if (win_y)
    *win_y = device_root_y - toplevel->y;
  if (mask)
    *mask = mask32;
  if (child_window)
    {
      if (cdk_window_get_window_type (toplevel) == CDK_WINDOW_ROOT)
	{
	  *child_window = mouse_toplevel;
	  if (*child_window == NULL)
	    *child_window = toplevel;
	}
      else
	{
	  /* No native children */
	  *child_window = toplevel;
	}
    }

  return;
}

void
_cdk_broadway_window_grab_check_unmap (CdkWindow *window,
				       gulong     serial)
{
  CdkDisplay *display = cdk_window_get_display (window);
  CdkDeviceManager *device_manager;
  GList *devices, *d;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  device_manager = cdk_display_get_device_manager (display);

  /* Get all devices */
  devices = cdk_device_manager_list_devices (device_manager, CDK_DEVICE_TYPE_MASTER);
  devices = g_list_concat (devices, cdk_device_manager_list_devices (device_manager, CDK_DEVICE_TYPE_SLAVE));
  devices = g_list_concat (devices, cdk_device_manager_list_devices (device_manager, CDK_DEVICE_TYPE_FLOATING));
  G_GNUC_END_IGNORE_DEPRECATIONS;

  /* End all grabs on the newly hidden window */
  for (d = devices; d; d = d->next)
    _cdk_display_end_device_grab (display, d->data, serial, window, TRUE);

  g_list_free (devices);
}


void
_cdk_broadway_window_grab_check_destroy (CdkWindow *window)
{
  CdkDisplay *display = cdk_window_get_display (window);
  CdkDeviceManager *device_manager;
  CdkDeviceGrabInfo *grab;
  GList *devices, *d;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  device_manager = cdk_display_get_device_manager (display);

  /* Get all devices */
  devices = cdk_device_manager_list_devices (device_manager, CDK_DEVICE_TYPE_MASTER);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  for (d = devices; d; d = d->next)
    {
      /* Make sure there is no lasting grab in this native window */
      grab = _cdk_display_get_last_device_grab (display, d->data);

      if (grab && grab->native_window == window)
	{
	  grab->serial_end = grab->serial_start;
	  grab->implicit_ungrab = TRUE;
	}

    }

  g_list_free (devices);
}


static CdkGrabStatus
cdk_broadway_device_grab (CdkDevice    *device,
			  CdkWindow    *window,
			  gboolean      owner_events,
			  CdkEventMask  event_mask,
			  CdkWindow    *confine_to,
			  CdkCursor    *cursor,
			  guint32       time_)
{
  CdkDisplay *display;
  CdkBroadwayDisplay *broadway_display;

  display = cdk_device_get_display (device);
  broadway_display = CDK_BROADWAY_DISPLAY (display);

  if (cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD)
    {
      /* Device is a keyboard */
      return CDK_GRAB_SUCCESS;
    }
  else
    {
      /* Device is a pointer */
      return _cdk_broadway_server_grab_pointer (broadway_display->server,
						CDK_WINDOW_IMPL_BROADWAY (window->impl)->id,
						owner_events,
						event_mask,
						time_);
    }
}

#define TIME_IS_LATER(time1, time2)                        \
  ( (( time1 > time2 ) && ( time1 - time2 < ((guint32)-1)/2 )) ||  \
    (( time1 < time2 ) && ( time2 - time1 > ((guint32)-1)/2 ))     \
  )

static void
cdk_broadway_device_ungrab (CdkDevice *device,
			    guint32    time_)
{
  CdkDisplay *display;
  CdkBroadwayDisplay *broadway_display;
  CdkDeviceGrabInfo *grab;
  guint32 serial;

  display = cdk_device_get_display (device);
  broadway_display = CDK_BROADWAY_DISPLAY (display);

  if (cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD)
    {
      /* Device is a keyboard */
    }
  else
    {
      /* Device is a pointer */
      serial = _cdk_broadway_server_ungrab_pointer (broadway_display->server, time_);

      if (serial != 0)
	{
	  grab = _cdk_display_get_last_device_grab (display, device);
	  if (grab &&
	      (time_ == CDK_CURRENT_TIME ||
	       grab->time == CDK_CURRENT_TIME ||
	       !TIME_IS_LATER (grab->time, time_)))
	    grab->serial_end = serial;
	}
    }
}

static CdkWindow *
cdk_broadway_device_window_at_position (CdkDevice       *device,
					gdouble         *win_x,
					gdouble         *win_y,
					CdkModifierType *mask,
					gboolean         get_toplevel)
{
  CdkScreen *screen;
  CdkWindow *root_window;
  CdkWindow *window;

  screen = cdk_display_get_default_screen (cdk_device_get_display (device));
  root_window = cdk_screen_get_root_window (screen);

  cdk_broadway_device_query_state (device, root_window, NULL, &window, NULL, NULL, win_x, win_y, mask);

  return window;
}

static void
cdk_broadway_device_select_window_events (CdkDevice    *device,
					  CdkWindow    *window,
					  CdkEventMask  event_mask)
{
}
