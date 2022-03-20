/* testscrolledge.c
 *
 * Copyright (C) 2014 Matthias Clasen <mclasen@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctk/ctk.h>

static guint add_rows_id = 0;

static void
populate_list (CtkListBox *list)
{
  gint i;
  gint n;
  GList *l;

  l = ctk_container_get_children (CTK_CONTAINER (list));
  n = g_list_length (l);
  g_list_free (l);

  for (i = 1; i <= 50; i++)
    {
      gchar *text;
      CtkWidget *row, *label;

      row = ctk_list_box_row_new ();
      text = g_strdup_printf ("List row %d", i + n);
      label = ctk_label_new (text);
      g_free (text);

      g_object_set (label, "margin", 10, NULL);
      ctk_widget_set_halign (label, CTK_ALIGN_START);
      ctk_container_add (CTK_CONTAINER (row), label);
      ctk_widget_show_all (row);
      ctk_container_add (CTK_CONTAINER (list), row);
    }
}

static CtkWidget *popup;
static CtkWidget *spinner;

static gboolean
add_rows (gpointer data)
{
  CtkListBox *list = data;

  ctk_widget_hide (popup);
  ctk_spinner_stop (CTK_SPINNER (spinner));

  populate_list (list);
  add_rows_id = 0;

  return G_SOURCE_REMOVE;
}

static void
edge_overshot (CtkScrolledWindow *sw,
               CtkPositionType    pos,
               CtkListBox        *list)
{
  if (pos == CTK_POS_BOTTOM)
    {
      ctk_spinner_start (CTK_SPINNER (spinner));
      ctk_widget_show (popup);

      if (add_rows_id == 0)
        add_rows_id = g_timeout_add (2000, add_rows, list);
    }
}

static void
edge_reached (CtkScrolledWindow *sw,
	      CtkPositionType    pos,
	      CtkListBox        *list)
{
  g_print ("Reached the edge at pos %d!\n", pos);
}

int
main (int argc, char *argv[])
{
  CtkWidget *win;
  CtkWidget *sw;
  CtkWidget *list;
  CtkWidget *overlay;
  CtkWidget *label;

  ctk_init (NULL, NULL);

  win = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (win), 600, 400);

  overlay = ctk_overlay_new ();
  popup = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
  ctk_widget_set_halign (popup, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (popup, CTK_ALIGN_END);
  g_object_set (popup, "margin", 40, NULL);
  label = ctk_label_new ("Getting more rows...");
  spinner = ctk_spinner_new ();
  ctk_widget_show (spinner);
  ctk_widget_show (label);
  ctk_container_add (CTK_CONTAINER (popup), label);
  ctk_container_add (CTK_CONTAINER (popup), spinner);

  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), popup);
  ctk_widget_set_no_show_all (popup, TRUE);

  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw), CTK_POLICY_NEVER, CTK_POLICY_AUTOMATIC);
  list = ctk_list_box_new ();
  ctk_list_box_set_selection_mode (CTK_LIST_BOX (list), CTK_SELECTION_NONE);

  ctk_container_add (CTK_CONTAINER (win), overlay);
  ctk_container_add (CTK_CONTAINER (overlay), sw);
  ctk_container_add (CTK_CONTAINER (sw), list);
  populate_list (CTK_LIST_BOX (list));

  g_signal_connect (sw, "edge-overshot", G_CALLBACK (edge_overshot), list);
  g_signal_connect (sw, "edge-reached", G_CALLBACK (edge_reached), list);

  ctk_widget_show_all (win);

  ctk_main ();

  return 0;
}
