/* testfullscreen.c
 * Copyright (C) 2013 Red Hat
 * Author: Olivier Fourdan <ofourdan@redhat.com>
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
#include <ctk/ctk.h>

static void
set_fullscreen_monitor_cb (CtkWidget *widget, gpointer user_data)
{
  GdkFullscreenMode mode = (GdkFullscreenMode) GPOINTER_TO_INT (user_data);
  GdkWindow  *window;

  window = ctk_widget_get_parent_window (widget);
  cdk_window_set_fullscreen_mode (window, mode);
  cdk_window_fullscreen (window);
}

static void
remove_fullscreen_cb (CtkWidget *widget, gpointer user_data)
{
  GdkWindow  *window;

  window = ctk_widget_get_parent_window (widget);
  cdk_window_unfullscreen (window);
}

int
main (int argc, char *argv[])
{
  CtkWidget *window, *vbox, *button;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
  ctk_widget_set_valign (vbox, CTK_ALIGN_CENTER);
  ctk_widget_set_halign (vbox, CTK_ALIGN_CENTER);
  ctk_box_set_homogeneous (CTK_BOX (vbox), TRUE);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  button = ctk_button_new_with_label ("Fullscreen on current monitor");
  g_signal_connect (button, "clicked", G_CALLBACK (set_fullscreen_monitor_cb), GINT_TO_POINTER (GDK_FULLSCREEN_ON_CURRENT_MONITOR));
  ctk_container_add (CTK_CONTAINER (vbox), button);

  button = ctk_button_new_with_label ("Fullscreen on all monitors");
  g_signal_connect (button, "clicked", G_CALLBACK (set_fullscreen_monitor_cb), GINT_TO_POINTER (GDK_FULLSCREEN_ON_ALL_MONITORS));
  ctk_container_add (CTK_CONTAINER (vbox), button);

  button = ctk_button_new_with_label ("Un-fullscreen");
  g_signal_connect (button, "clicked", G_CALLBACK (remove_fullscreen_cb), NULL);
  ctk_container_add (CTK_CONTAINER (vbox), button);

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
