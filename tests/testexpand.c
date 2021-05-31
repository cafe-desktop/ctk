/* testexpand.c
 * Copyright (C) 2010 Havoc Pennington
 *
 * Author:
 *      Havoc Pennington <hp@pobox.com>
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
on_toggle_hexpand (CtkToggleButton *toggle,
                   void            *data)
{
  g_object_set (toggle,
                "hexpand", ctk_toggle_button_get_active (toggle),
                NULL);
}

static void
on_toggle_vexpand (CtkToggleButton *toggle,
                   void            *data)
{
  g_object_set (toggle,
                "vexpand", ctk_toggle_button_get_active (toggle),
                NULL);
}

static void
create_box_window (void)
{
  CtkWidget *window;
  CtkWidget *box1, *box2, *box3;
  CtkWidget *toggle;
  CtkWidget *colorbox;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Boxes");

  box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  box2 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  box3 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

  ctk_box_pack_start (CTK_BOX (box1),
                      ctk_label_new ("VBox 1 Top"),
                      FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (box1),
                      box2,
                      FALSE, TRUE, 0);
  ctk_box_pack_end (CTK_BOX (box1),
                    ctk_label_new ("VBox 1 Bottom"),
                    FALSE, FALSE, 0);

  ctk_box_pack_start (CTK_BOX (box2),
                      ctk_label_new ("HBox 2 Left"),
                      FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (box2),
                      box3,
                      FALSE, TRUE, 0);
  ctk_box_pack_end (CTK_BOX (box2),
                    ctk_label_new ("HBox 2 Right"),
                    FALSE, FALSE, 0);

  ctk_box_pack_start (CTK_BOX (box3),
                      ctk_label_new ("VBox 3 Top"),
                      FALSE, FALSE, 0);
  ctk_box_pack_end (CTK_BOX (box3),
                    ctk_label_new ("VBox 3 Bottom"),
                    FALSE, FALSE, 0);

  colorbox = ctk_frame_new (NULL);

  toggle = ctk_toggle_button_new_with_label ("H Expand");
  ctk_widget_set_halign (toggle, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (toggle, CTK_ALIGN_CENTER);
  g_object_set (toggle, "margin", 5, NULL);
  g_signal_connect (G_OBJECT (toggle), "toggled",
                    G_CALLBACK (on_toggle_hexpand), NULL);
  ctk_container_add (CTK_CONTAINER (colorbox), toggle);

  ctk_box_pack_start (CTK_BOX (box3), colorbox, FALSE, TRUE, 0);

  colorbox = ctk_frame_new (NULL);

  toggle = ctk_toggle_button_new_with_label ("V Expand");
  ctk_widget_set_halign (toggle, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (toggle, CTK_ALIGN_CENTER);
  g_object_set (toggle, "margin", 5, NULL);
  g_signal_connect (G_OBJECT (toggle), "toggled",
                    G_CALLBACK (on_toggle_vexpand), NULL);
  ctk_container_add (CTK_CONTAINER (colorbox), toggle);
  ctk_box_pack_start (CTK_BOX (box3), colorbox, FALSE, TRUE, 0);

  ctk_container_add (CTK_CONTAINER (window), box1);
  ctk_widget_show_all (window);
}

static void
create_grid_window (void)
{
  CtkWidget *window;
  CtkWidget *grid;
  CtkWidget *toggle;
  CtkWidget *colorbox;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Grid");

  grid = ctk_grid_new ();

  ctk_grid_attach (CTK_GRID (grid), ctk_label_new ("Top"), 1, 0, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), ctk_label_new ("Bottom"), 1, 3, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), ctk_label_new ("Left"), 0, 1, 1, 2);
  ctk_grid_attach (CTK_GRID (grid), ctk_label_new ("Right"), 2, 1, 1, 2);

  colorbox = ctk_frame_new (NULL);

  toggle = ctk_toggle_button_new_with_label ("H Expand");
  ctk_widget_set_halign (toggle, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (toggle, CTK_ALIGN_CENTER);
  g_object_set (toggle, "margin", 5, NULL);
  g_signal_connect (G_OBJECT (toggle), "toggled",
                    G_CALLBACK (on_toggle_hexpand), NULL);
  ctk_container_add (CTK_CONTAINER (colorbox), toggle);

  ctk_grid_attach (CTK_GRID (grid), colorbox, 1, 1, 1, 1);

  colorbox = ctk_frame_new (NULL);

  toggle = ctk_toggle_button_new_with_label ("V Expand");
  ctk_widget_set_halign (toggle, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (toggle, CTK_ALIGN_CENTER);
  g_object_set (toggle, "margin", 5, NULL);
  g_signal_connect (G_OBJECT (toggle), "toggled",
                    G_CALLBACK (on_toggle_vexpand), NULL);
  ctk_container_add (CTK_CONTAINER (colorbox), toggle);

  ctk_grid_attach (CTK_GRID (grid), colorbox, 1, 2, 1, 1); 

  ctk_container_add (CTK_CONTAINER (window), grid);
  ctk_widget_show_all (window);
}

int
main (int argc, char *argv[])
{
  ctk_init (&argc, &argv);

  if (g_getenv ("RTL"))
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);

  create_box_window ();
  create_grid_window ();

  ctk_main ();

  return 0;
}
