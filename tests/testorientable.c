/* testorientable.c
 * Copyright (C) 2004  Red Hat, Inc.
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

#include <ctk/ctk.h>

static void
orient_toggled (CtkToggleButton *button, gpointer user_data)
{
  GList *orientables = (GList *) user_data, *ptr;
  gboolean state = ctk_toggle_button_get_active (button);
  CtkOrientation orientation;

  if (state)
    {
      orientation = CTK_ORIENTATION_VERTICAL;
      ctk_button_set_label (CTK_BUTTON (button), "Vertical");
    }
  else
    {
      orientation = CTK_ORIENTATION_HORIZONTAL;
      ctk_button_set_label (CTK_BUTTON (button), "Horizontal");
    }

  for (ptr = orientables; ptr; ptr = ptr->next)
    {
      CtkOrientable *orientable = CTK_ORIENTABLE (ptr->data);

      ctk_orientable_set_orientation (orientable, orientation);
    }
}

int
main (int argc, char **argv)
{
  CtkWidget *window;
  CtkWidget *grid;
  CtkWidget *box, *button;
  GList *orientables = NULL;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  grid= ctk_grid_new ();
  ctk_grid_set_row_spacing (CTK_GRID (grid), 12);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 12);

  /* CtkBox */
  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
  orientables = g_list_prepend (orientables, box);
  ctk_grid_attach (CTK_GRID (grid), box, 0, 1, 1, 1);
  ctk_box_pack_start (CTK_BOX (box),
                  ctk_button_new_with_label ("CtkBox 1"),
                  TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (box),
                  ctk_button_new_with_label ("CtkBox 2"),
                  TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (box),
                  ctk_button_new_with_label ("CtkBox 3"),
                  TRUE, TRUE, 0);

  /* CtkButtonBox */
  box = ctk_button_box_new (CTK_ORIENTATION_HORIZONTAL);
  orientables = g_list_prepend (orientables, box);
  ctk_grid_attach (CTK_GRID (grid), box, 1, 1, 1, 1);
  ctk_box_pack_start (CTK_BOX (box),
                  ctk_button_new_with_label ("CtkButtonBox 1"),
                  TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (box),
                  ctk_button_new_with_label ("CtkButtonBox 2"),
                  TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (box),
                  ctk_button_new_with_label ("CtkButtonBox 3"),
                  TRUE, TRUE, 0);

  /* CtkSeparator */
  box = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
  orientables = g_list_prepend (orientables, box);
  ctk_grid_attach (CTK_GRID (grid), box, 2, 1, 1, 1);

  button = ctk_toggle_button_new_with_label ("Horizontal");
  ctk_grid_attach (CTK_GRID (grid), button, 0, 0, 1, 1);
  g_signal_connect (button, "toggled",
                  G_CALLBACK (orient_toggled), orientables);

  ctk_container_add (CTK_CONTAINER (window), grid);
  ctk_widget_show_all (window);

  g_signal_connect (window, "destroy",
                  G_CALLBACK (ctk_main_quit), NULL);

  ctk_main ();

  return 0;
}
