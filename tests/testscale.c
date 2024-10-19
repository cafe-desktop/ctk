/* testscale.c - scale mark demo
 * Copyright (C) 2009 Red Hat, Inc.
 * Author: Matthias Clasen
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

#include <ctk/ctk.h>


GSList *scales;
CtkWidget *flipbox;
CtkWidget *extra_scale;

static void
invert (CtkButton *button G_GNUC_UNUSED)
{
  GSList *l;

  for (l = scales; l; l = l->next)
    {
      CtkRange *range = l->data;
      ctk_range_set_inverted (range, !ctk_range_get_inverted (range));
    }
}

static void
flip (CtkButton *button G_GNUC_UNUSED)
{
  GSList *l;

  ctk_orientable_set_orientation (CTK_ORIENTABLE (flipbox), 1 - ctk_orientable_get_orientation (CTK_ORIENTABLE (flipbox)));

  for (l = scales; l; l = l->next)
    {
      CtkOrientable *o = l->data;
      ctk_orientable_set_orientation (o, 1 - ctk_orientable_get_orientation (o));
    }
}

static void
trough (CtkToggleButton *button)
{
  GSList *l;
  gboolean value;

  value = ctk_toggle_button_get_active (button);

  for (l = scales; l; l = l->next)
    {
      CtkRange *range = l->data;
      ctk_range_set_range (range, 0., value ? 100.0 : 0.);
    }
}

gdouble marks[3] = { 0.0, 50.0, 100.0 };
gdouble extra_marks[2] = { 20.0, 40.0 };

static void
extra (CtkToggleButton *button)
{
  gboolean value;

  value = ctk_toggle_button_get_active (button);

  if (value)
    {
      ctk_scale_add_mark (CTK_SCALE (extra_scale), extra_marks[0], CTK_POS_TOP, NULL);
      ctk_scale_add_mark (CTK_SCALE (extra_scale), extra_marks[1], CTK_POS_TOP, NULL);
    }
  else
    {
      ctk_scale_clear_marks (CTK_SCALE (extra_scale));
      ctk_scale_add_mark (CTK_SCALE (extra_scale), marks[0], CTK_POS_BOTTOM, NULL);
      ctk_scale_add_mark (CTK_SCALE (extra_scale), marks[1], CTK_POS_BOTTOM, NULL);
      ctk_scale_add_mark (CTK_SCALE (extra_scale), marks[2], CTK_POS_BOTTOM, NULL);
    }
}

int main (int argc, char *argv[])
{
  CtkWidget *window;
  CtkWidget *box;
  CtkWidget *box1;
  CtkWidget *box2;
  CtkWidget *button;
  CtkWidget *frame;
  CtkWidget *scale;
  const gchar *labels[3] = {
    "<small>Left</small>",
    "<small>Middle</small>",
    "<small>Right</small>"
  };

  gdouble bath_marks[4] = { 0.0, 33.3, 66.6, 100.0 };
  const gchar *bath_labels[4] = {
    "<span color='blue' size='small'>Cold</span>",
    "<span size='small'>Baby bath</span>",
    "<span size='small'>Hot tub</span>",
    "<span color='Red' size='small'>Hot</span>"
  };

  gdouble pos_marks[4] = { 0.0, 33.3, 66.6, 100.0 };
  const gchar *pos_labels[4] = { "Left", "Right", "Top", "Bottom" };

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Ranges with marks");
  box1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
  flipbox = box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
  ctk_widget_set_hexpand (flipbox, TRUE);
  ctk_widget_set_vexpand (flipbox, TRUE);
  ctk_container_add (CTK_CONTAINER (box1), box);
  ctk_container_add (CTK_CONTAINER (window), box1);

  frame = ctk_frame_new ("No marks");
  scale = ctk_scale_new_with_range (CTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  ctk_scale_set_draw_value (CTK_SCALE (scale), FALSE);
  ctk_container_add (CTK_CONTAINER (frame), scale);
  ctk_box_pack_start (CTK_BOX (box), frame, FALSE, FALSE, 0);

  frame = ctk_frame_new ("With fill level");
  scale = ctk_scale_new_with_range (CTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  ctk_scale_set_draw_value (CTK_SCALE (scale), FALSE);
  ctk_range_set_show_fill_level (CTK_RANGE (scale), TRUE);
  ctk_range_set_fill_level (CTK_RANGE (scale), 50);
  ctk_container_add (CTK_CONTAINER (frame), scale);
  ctk_box_pack_start (CTK_BOX (box), frame, FALSE, FALSE, 0);

  frame = ctk_frame_new ("Simple marks");
  extra_scale = scale = ctk_scale_new_with_range (CTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  ctk_scale_set_draw_value (CTK_SCALE (scale), FALSE);
  ctk_scale_add_mark (CTK_SCALE (scale), marks[0], CTK_POS_BOTTOM, NULL);
  ctk_scale_add_mark (CTK_SCALE (scale), marks[1], CTK_POS_BOTTOM, NULL);
  ctk_scale_add_mark (CTK_SCALE (scale), marks[2], CTK_POS_BOTTOM, NULL);
  ctk_container_add (CTK_CONTAINER (frame), scale);
  ctk_box_pack_start (CTK_BOX (box), frame, FALSE, FALSE, 0);

  frame = ctk_frame_new ("Simple marks up");
  scale = ctk_scale_new_with_range (CTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  ctk_scale_set_draw_value (CTK_SCALE (scale), FALSE);
  ctk_scale_add_mark (CTK_SCALE (scale), marks[0], CTK_POS_TOP, NULL);
  ctk_scale_add_mark (CTK_SCALE (scale), marks[1], CTK_POS_TOP, NULL);
  ctk_scale_add_mark (CTK_SCALE (scale), marks[2], CTK_POS_TOP, NULL);
  ctk_container_add (CTK_CONTAINER (frame), scale);
  ctk_box_pack_start (CTK_BOX (box), frame, FALSE, FALSE, 0);

  frame = ctk_frame_new ("Labeled marks");
  box2 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);

  scale = ctk_scale_new_with_range (CTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  ctk_scale_set_draw_value (CTK_SCALE (scale), FALSE);
  ctk_scale_add_mark (CTK_SCALE (scale), marks[0], CTK_POS_BOTTOM, labels[0]);
  ctk_scale_add_mark (CTK_SCALE (scale), marks[1], CTK_POS_BOTTOM, labels[1]);
  ctk_scale_add_mark (CTK_SCALE (scale), marks[2], CTK_POS_BOTTOM, labels[2]);
  ctk_container_add (CTK_CONTAINER (frame), scale);
  ctk_box_pack_start (CTK_BOX (box), frame, FALSE, FALSE, 0);

  frame = ctk_frame_new ("Some labels");
  scale = ctk_scale_new_with_range (CTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  ctk_scale_set_draw_value (CTK_SCALE (scale), FALSE);
  ctk_scale_add_mark (CTK_SCALE (scale), marks[0], CTK_POS_TOP, labels[0]);
  ctk_scale_add_mark (CTK_SCALE (scale), marks[1], CTK_POS_TOP, NULL);
  ctk_scale_add_mark (CTK_SCALE (scale), marks[2], CTK_POS_TOP, labels[2]);
  ctk_container_add (CTK_CONTAINER (frame), scale);
  ctk_box_pack_start (CTK_BOX (box), frame, FALSE, FALSE, 0);

  frame = ctk_frame_new ("Above and below");
  scale = ctk_scale_new_with_range (CTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  ctk_scale_set_draw_value (CTK_SCALE (scale), FALSE);
  ctk_scale_add_mark (CTK_SCALE (scale), bath_marks[0], CTK_POS_TOP, bath_labels[0]);
  ctk_scale_add_mark (CTK_SCALE (scale), bath_marks[1], CTK_POS_BOTTOM, bath_labels[1]);
  ctk_scale_add_mark (CTK_SCALE (scale), bath_marks[2], CTK_POS_BOTTOM, bath_labels[2]);
  ctk_scale_add_mark (CTK_SCALE (scale), bath_marks[3], CTK_POS_TOP, bath_labels[3]);
  ctk_container_add (CTK_CONTAINER (frame), scale);
  ctk_box_pack_start (CTK_BOX (box), frame, FALSE, FALSE, 0);

  frame = ctk_frame_new ("Positions");
  scale = ctk_scale_new_with_range (CTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  ctk_scale_set_draw_value (CTK_SCALE (scale), FALSE);
  ctk_scale_add_mark (CTK_SCALE (scale), pos_marks[0], CTK_POS_LEFT, pos_labels[0]);
  ctk_scale_add_mark (CTK_SCALE (scale), pos_marks[1], CTK_POS_RIGHT, pos_labels[1]);
  ctk_scale_add_mark (CTK_SCALE (scale), pos_marks[2], CTK_POS_TOP, pos_labels[2]);
  ctk_scale_add_mark (CTK_SCALE (scale), pos_marks[3], CTK_POS_BOTTOM, pos_labels[3]);
  ctk_container_add (CTK_CONTAINER (frame), scale);
  ctk_box_pack_start (CTK_BOX (box), frame, FALSE, FALSE, 0);

  box2 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
  ctk_container_add (CTK_CONTAINER (box1), box2);
  button = ctk_button_new_with_label ("Flip");
  g_signal_connect (button, "clicked", G_CALLBACK (flip), NULL);
  ctk_container_add (CTK_CONTAINER (box2), button);

  button = ctk_button_new_with_label ("Invert");
  g_signal_connect (button, "clicked", G_CALLBACK (invert), NULL);
  ctk_container_add (CTK_CONTAINER (box2), button);

  button = ctk_toggle_button_new_with_label ("Trough");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (button, "toggled", G_CALLBACK (trough), NULL);
  ctk_container_add (CTK_CONTAINER (box2), button);
  ctk_widget_show_all (window);

  button = ctk_toggle_button_new_with_label ("Extra");
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), FALSE);
  g_signal_connect (button, "toggled", G_CALLBACK (extra), NULL);
  ctk_container_add (CTK_CONTAINER (box2), button);
  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}


