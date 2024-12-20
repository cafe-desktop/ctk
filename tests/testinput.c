/* CTK - The GIMP Toolkit
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

#include "config.h"
#include <stdio.h>
#include "ctk/ctk.h"
#include <math.h>

/* Backing surface for drawing area */

static cairo_surface_t *surface = NULL;

/* Information about cursor */

static gint cursor_proximity = TRUE;
static gdouble cursor_x;
static gdouble cursor_y;

/* Unique ID of current device */
static CdkDevice *current_device;

/* Erase the old cursor, and/or draw a new one, if necessary */
static void
update_cursor (CtkWidget *widget,  gdouble x, gdouble y)
{
  static gint cursor_present = 0;
  gint state = !cdk_device_get_has_cursor (current_device) && cursor_proximity;

  x = floor (x);
  y = floor (y);

  if (surface != NULL)
    {
      cairo_t *cr = cdk_cairo_create (ctk_widget_get_window (widget));

      if (cursor_present && (cursor_present != state ||
			     x != cursor_x || y != cursor_y))
	{
          cairo_set_source_surface (cr, surface, 0, 0);
          cairo_rectangle (cr, cursor_x - 5, cursor_y - 5, 10, 10);
          cairo_fill (cr);
	}

      cursor_present = state;
      cursor_x = x;
      cursor_y = y;

      if (cursor_present)
	{
          cairo_set_source_rgb (cr, 0, 0, 0);
          cairo_rectangle (cr, 
                           cursor_x - 5, cursor_y -5,
			   10, 10);
          cairo_fill (cr);
	}

      cairo_destroy (cr);
    }
}

/* Create a new backing surface of the appropriate size */
static gint
configure_event (CtkWidget         *widget,
		 CdkEventConfigure *event G_GNUC_UNUSED)
{
  CtkAllocation allocation;
  cairo_t *cr;

  if (surface)
    cairo_surface_destroy (surface);

  ctk_widget_get_allocation (widget, &allocation);

  surface = cdk_window_create_similar_surface (ctk_widget_get_window (widget),
                                               CAIRO_CONTENT_COLOR,
                                               allocation.width,
                                               allocation.height);
  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  cairo_destroy (cr);

  return TRUE;
}

/* Refill the screen from the backing surface */
static gboolean
draw (CtkWidget *widget G_GNUC_UNUSED,
      cairo_t   *cr)
{
  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_paint (cr);

  return FALSE;
}

/* Draw a rectangle on the screen, size depending on pressure,
   and color on the type of device */
static void
draw_brush (CtkWidget *widget, CdkInputSource source,
	    gdouble x, gdouble y, gdouble pressure)
{
  CdkRGBA color;
  CdkRectangle update_rect;
  cairo_t *cr;

  color.alpha = 1.0;

  switch (source)
    {
    case CDK_SOURCE_MOUSE:
      color.red = color.green = 0.0;
      color.blue = 1.0;
      break;
    case CDK_SOURCE_PEN:
      color.red = color.green = color.blue = 0.0;
      break;
    case CDK_SOURCE_ERASER:
      color.red = color.green = color.blue = 1.0;
      break;
    default:
      color.red = color.blue = 0.0;
      color.green = 1.0;
    }

  update_rect.x = x - 10 * pressure;
  update_rect.y = y - 10 * pressure;
  update_rect.width = 20 * pressure;
  update_rect.height = 20 * pressure;

  cr = cairo_create (surface);
  cdk_cairo_set_source_rgba (cr, &color);
  cdk_cairo_rectangle (cr, &update_rect);
  cairo_fill (cr);
  cairo_destroy (cr);

  ctk_widget_queue_draw_area (widget,
			      update_rect.x, update_rect.y,
			      update_rect.width, update_rect.height);
  cdk_window_process_updates (ctk_widget_get_window (widget), TRUE);
}

static guint32 motion_time;

static void
print_axes (CdkDevice *device, gdouble *axes)
{
  if (axes)
    {
      int i;

      g_print ("%s ", cdk_device_get_name (device));

      for (i = 0; i < cdk_device_get_n_axes (device); i++)
	g_print ("%g ", axes[i]);

      g_print ("\n");
    }
}

static gint
button_press_event (CtkWidget *widget, CdkEventButton *event)
{
  current_device = event->device;
  cursor_proximity = TRUE;

  if (event->button == CDK_BUTTON_PRIMARY &&
      surface != NULL)
    {
      gdouble pressure = 0.5;

      print_axes (event->device, event->axes);
      cdk_event_get_axis ((CdkEvent *)event, CDK_AXIS_PRESSURE, &pressure);
      draw_brush (widget, cdk_device_get_source (cdk_event_get_source_device ((CdkEvent *) event)),
                  event->x, event->y, pressure);

      motion_time = event->time;
    }

  update_cursor (widget, event->x, event->y);

  return TRUE;
}

static gint
key_press_event (CtkWidget   *widget G_GNUC_UNUSED,
		 CdkEventKey *event)
{
  if ((event->keyval >= 0x20) && (event->keyval <= 0xFF))
    printf("I got a %c\n", event->keyval);
  else
    printf("I got some other key\n");

  return TRUE;
}

static gint
motion_notify_event (CtkWidget *widget, CdkEventMotion *event)
{
  CdkTimeCoord **events;
  gint n_events;

  current_device = event->device;
  cursor_proximity = TRUE;

  if (event->state & CDK_BUTTON1_MASK && surface != NULL)
    {
      if (cdk_device_get_history (event->device, event->window, 
				  motion_time, event->time,
				  &events, &n_events))
	{
	  int i;

	  for (i=0; i<n_events; i++)
	    {
	      double x = 0, y = 0, pressure = 0.5;

	      cdk_device_get_axis (event->device, events[i]->axes, CDK_AXIS_X, &x);
	      cdk_device_get_axis (event->device, events[i]->axes, CDK_AXIS_Y, &y);
	      cdk_device_get_axis (event->device, events[i]->axes, CDK_AXIS_PRESSURE, &pressure);
	      draw_brush (widget, cdk_device_get_source (cdk_event_get_source_device ((CdkEvent *) event)),
                          x, y, pressure);

	      print_axes (cdk_event_get_source_device ((CdkEvent *) event), events[i]->axes);
	    }
	  cdk_device_free_history (events, n_events);
	}
      else
	{
	  double pressure = 0.5;

	  cdk_event_get_axis ((CdkEvent *)event, CDK_AXIS_PRESSURE, &pressure);

	  draw_brush (widget, cdk_device_get_source (cdk_event_get_source_device ((CdkEvent *) event)),
                      event->x, event->y, pressure);
	}
      motion_time = event->time;
    }


  print_axes (event->device, event->axes);
  update_cursor (widget, event->x, event->y);

  return TRUE;
}

/* We track the next two events to know when we need to draw a
   cursor */

static gint
proximity_out_event (CtkWidget         *widget,
		     CdkEventProximity *event G_GNUC_UNUSED)
{
  cursor_proximity = FALSE;
  update_cursor (widget, cursor_x, cursor_y);
  return TRUE;
}

static gint
leave_notify_event (CtkWidget        *widget,
		    CdkEventCrossing *event G_GNUC_UNUSED)
{
  cursor_proximity = FALSE;
  update_cursor (widget, cursor_x, cursor_y);
  return TRUE;
}

void
quit (void)
{
  ctk_main_quit ();
}

int
main (int argc, char *argv[])
{
  GList *devices, *d;
  CdkEventMask event_mask;
  CtkWidget *window;
  CtkWidget *drawing_area;
  CtkWidget *vbox;
  CtkWidget *button;
  CdkWindow *cdk_win;
  CdkSeat *seat;

  ctk_init (&argc, &argv);

  seat = cdk_display_get_default_seat (cdk_display_get_default ());
  current_device = cdk_seat_get_pointer (seat);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_name (window, "Test Input");

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), vbox);
  ctk_widget_show (vbox);

  g_signal_connect (window, "destroy",
		    G_CALLBACK (quit), NULL);

  /* Create the drawing area */

  drawing_area = ctk_drawing_area_new ();
  ctk_widget_set_size_request (drawing_area, 200, 200);
  ctk_box_pack_start (CTK_BOX (vbox), drawing_area, TRUE, TRUE, 0);

  ctk_widget_show (drawing_area);

  /* Signals used to handle backing surface */

  g_signal_connect (drawing_area, "draw",
		    G_CALLBACK (draw), NULL);
  g_signal_connect (drawing_area, "configure_event",
		    G_CALLBACK (configure_event), NULL);

  /* Event signals */

  g_signal_connect (drawing_area, "motion_notify_event",
		    G_CALLBACK (motion_notify_event), NULL);
  g_signal_connect (drawing_area, "button_press_event",
		    G_CALLBACK (button_press_event), NULL);
  g_signal_connect (drawing_area, "key_press_event",
		    G_CALLBACK (key_press_event), NULL);

  g_signal_connect (drawing_area, "leave_notify_event",
		    G_CALLBACK (leave_notify_event), NULL);
  g_signal_connect (drawing_area, "proximity_out_event",
		    G_CALLBACK (proximity_out_event), NULL);

  event_mask = CDK_EXPOSURE_MASK |
    CDK_LEAVE_NOTIFY_MASK |
    CDK_BUTTON_PRESS_MASK |
    CDK_KEY_PRESS_MASK |
    CDK_POINTER_MOTION_MASK |
    CDK_PROXIMITY_OUT_MASK;

  ctk_widget_set_events (drawing_area, event_mask);

  devices = cdk_seat_get_slaves (seat, CDK_SEAT_CAPABILITY_ALL_POINTING);

  for (d = devices; d; d = d->next)
    {
      CdkDevice *device;

      device = d->data;
      ctk_widget_set_device_events (drawing_area, device, event_mask);
      cdk_device_set_mode (device, CDK_MODE_SCREEN);
    }

  g_list_free (devices);

  ctk_widget_set_can_focus (drawing_area, TRUE);
  ctk_widget_grab_focus (drawing_area);

  /* .. And create some buttons */
  button = ctk_button_new_with_label ("Quit");
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (ctk_widget_destroy),
			    window);
  ctk_widget_show (button);

  ctk_widget_show (window);

  /* request all motion events */
  cdk_win = ctk_widget_get_window (drawing_area);
  cdk_window_set_event_compression (cdk_win, FALSE);

  ctk_main ();

  return 0;
}
