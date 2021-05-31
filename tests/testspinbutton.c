/* testspinbutton.c
 * Copyright (C) 2004 Morten Welinder
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

static gint num_windows = 0;

static gboolean
on_delete_event (GtkWidget *w,
                 GdkEvent *event,
                 gpointer user_data)
{
  num_windows--;
  if (num_windows == 0)
    ctk_main_quit ();

  return FALSE;
}

static void
prepare_window_for_orientation (GtkOrientation orientation)
{
  GtkWidget *window, *mainbox, *wrap_button;
  int max;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "delete_event", G_CALLBACK (on_delete_event), NULL);

  mainbox = ctk_box_new (CTK_ORIENTATION_VERTICAL ^ orientation, 2);
  ctk_container_add (CTK_CONTAINER (window), mainbox);

  wrap_button = ctk_toggle_button_new_with_label ("Wrap");
  ctk_container_add (CTK_CONTAINER (mainbox), wrap_button);

  for (max = 9; max <= 999999999; max = max * 10 + 9)
    {
      GtkAdjustment *adj = ctk_adjustment_new (max,
                                               1, max,
                                               1,
                                               (max + 1) / 10,
                                               0.0);

      GtkWidget *spin = ctk_spin_button_new (adj, 1.0, 0);
      GtkWidget *hbox;
      ctk_orientable_set_orientation (CTK_ORIENTABLE (spin), orientation);
      ctk_widget_set_halign (CTK_WIDGET (spin), CTK_ALIGN_CENTER);

      g_object_bind_property (wrap_button, "active", spin, "wrap", G_BINDING_SYNC_CREATE);

      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
      ctk_box_pack_start (CTK_BOX (hbox), spin, FALSE, FALSE, 2);
      ctk_container_add (CTK_CONTAINER (mainbox), hbox);
    }

  ctk_widget_show_all (window);
  num_windows++;
}

int
main (int argc, char **argv)
{
  ctk_init (&argc, &argv);

  prepare_window_for_orientation (CTK_ORIENTATION_HORIZONTAL);
  prepare_window_for_orientation (CTK_ORIENTATION_VERTICAL);

  ctk_main ();

  return 0;
}
