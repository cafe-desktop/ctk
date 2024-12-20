/* testmultidisplay.c
 * Copyright (C) 2001 Sun Microsystems Inc.
 * Author: Erwann Chenede <erwann.chenede@sun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <stdlib.h>
#include <ctk/ctk.h>

static gint num_monitors;
static gint primary_monitor;

static void
request (CtkWidget      *widget,
	 gpointer        user_data)
{
  gchar *str;
  CdkScreen *screen = ctk_widget_get_screen (widget);
  gint i = cdk_screen_get_monitor_at_window (screen,
                                             ctk_widget_get_window (widget));

  if (i < 0)
    str = g_strdup ("<big><span foreground='white' background='black'>Not on a monitor </span></big>");
  else
    {
      CdkRectangle monitor;

      cdk_screen_get_monitor_geometry (screen,
                                       i, &monitor);
      primary_monitor = cdk_screen_get_primary_monitor (screen);

      str = g_strdup_printf ("<big><span foreground='white' background='black'>"
			     "Monitor %d of %d</span></big>\n"
			     "<i>Width - Height       </i>: (%d,%d)\n"
			     "<i>Top left coordinate </i>: (%d,%d)\n"
                             "<i>Primary monitor: %d</i>",
                             i + 1, num_monitors,
			     monitor.width, monitor.height,
                             monitor.x, monitor.y,
                             primary_monitor);
    }

  ctk_label_set_markup (CTK_LABEL (user_data), str);
  g_free (str);
}

static void
monitors_changed_cb (CdkScreen *screen G_GNUC_UNUSED,
                     gpointer   data)
{
  CtkWidget *label = (CtkWidget *)data;

  request (label, label);
}

int
main (int argc, char *argv[])
{
  CdkScreen *screen;
  gint i;

  ctk_init (&argc, &argv);

  screen = cdk_screen_get_default ();

  num_monitors = cdk_screen_get_n_monitors (screen);
  if (num_monitors == 1)
    g_warning ("The default screen of the current display only has one monitor.");

  primary_monitor = cdk_screen_get_primary_monitor (screen);

  for (i = 0; i < num_monitors; i++)
    {
      CtkWidget *window, *label, *vbox, *button;

      CdkRectangle monitor; 
      gchar *str;
      
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      
      cdk_screen_get_monitor_geometry (screen, i, &monitor);
      ctk_window_set_default_size (CTK_WINDOW (window), 200, 200);
      ctk_window_move (CTK_WINDOW (window), (monitor.width - 200) / 2 + monitor.x,
		       (monitor.height - 200) / 2 + monitor.y);
      
      label = ctk_label_new (NULL);
      str = g_strdup_printf ("<big><span foreground='white' background='black'>"
			     "Monitor %d of %d</span></big>\n"
			     "<i>Width - Height       </i>: (%d,%d)\n"
			     "<i>Top left coordinate </i>: (%d,%d)\n"
                             "<i>Primary monitor: %d</i>",
                             i + 1, num_monitors,
			     monitor.width, monitor.height,
                             monitor.x, monitor.y,
                             primary_monitor);
      ctk_label_set_markup (CTK_LABEL (label), str);
      g_free (str);
      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 1);
      ctk_box_set_homogeneous (CTK_BOX (vbox), TRUE);
      ctk_container_add (CTK_CONTAINER (window), vbox);
      ctk_container_add (CTK_CONTAINER (vbox), label);
      button = ctk_button_new_with_label ("Query current monitor");
      g_signal_connect (button, "clicked", G_CALLBACK (request), label);
      ctk_container_add (CTK_CONTAINER (vbox), button);
      button = ctk_button_new_with_label ("Close");
      g_signal_connect (button, "clicked", G_CALLBACK (ctk_main_quit), NULL);
      ctk_container_add (CTK_CONTAINER (vbox), button);
      ctk_widget_show_all (window);

      g_signal_connect (screen, "monitors-changed",
                        G_CALLBACK (monitors_changed_cb), label);
    }

  ctk_main ();

  return 0;
}
