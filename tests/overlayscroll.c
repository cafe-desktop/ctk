/* simple.c
 * Copyright (C) 1997  Red Hat, Inc
 * Author: Elliot Lee
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
#include <string.h>

static gchar *
get_content (void)
{
  GString *s;
  gint i;

  s = g_string_new ("");
  for (i = 1; i <= 150; i++)
    g_string_append_printf (s, "Line %d\n", i);

  return g_string_free (s, FALSE);
}

static void
mode_changed (CtkComboBox *combo, CtkScrolledWindow *sw)
{
  gint active = ctk_combo_box_get_active (combo);

  ctk_scrolled_window_set_overlay_scrolling (sw, active == 1);
}

int
main (int argc, char *argv[])
{
  CtkWidget *window;
  gchar *content;
  CtkWidget *box;
  CtkWidget *sw;
  CtkWidget *tv;
  CtkWidget *sb2;
  CtkWidget *combo;
  CtkAdjustment *adj;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window), 640, 480);
  g_signal_connect (window, "delete-event", G_CALLBACK (ctk_main_quit), NULL);

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 20);
  ctk_container_add (CTK_CONTAINER (window), box);

  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                  CTK_POLICY_NEVER,
                                  CTK_POLICY_AUTOMATIC);

  ctk_box_pack_start (CTK_BOX (box), sw, TRUE, TRUE, 0);

  content = get_content ();

  tv = ctk_text_view_new ();
  ctk_text_view_set_wrap_mode (CTK_TEXT_VIEW (tv), CTK_WRAP_WORD);
  ctk_container_add (CTK_CONTAINER (sw), tv);
  ctk_text_buffer_set_text (ctk_text_view_get_buffer (CTK_TEXT_VIEW (tv)),
                            content, -1);
  g_free (content);

  adj = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (tv));

  combo = ctk_combo_box_text_new ();
  ctk_widget_set_valign (combo, CTK_ALIGN_START);
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Traditional");
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo), "Overlay");
  g_signal_connect (combo, "changed", G_CALLBACK (mode_changed), sw);
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo), 1);

  ctk_container_add (CTK_CONTAINER (box), combo);

  sb2 = ctk_scrollbar_new (CTK_ORIENTATION_VERTICAL, adj);
  ctk_container_add (CTK_CONTAINER (box), sb2);

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
