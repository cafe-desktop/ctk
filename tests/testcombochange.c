/* testcombochange.c
 * Copyright (C) 2004  Red Hat, Inc.
 * Author: Owen Taylor
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
#include <stdarg.h>

CtkWidget *text_view;
CtkListStore *model;
GArray *contents;

static char next_value = 'A';

static void
test_init (void)
{
  if (g_file_test ("../modules/input/immodules.cache", G_FILE_TEST_EXISTS))
    g_setenv ("CTK_IM_MODULE_FILE", "../modules/input/immodules.cache", TRUE);
}

static void
combochange_log (const char *fmt,
     ...)
{
  CtkTextBuffer *buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (text_view));
  CtkTextIter iter;
  va_list vap;
  char *msg;
  GString *order_string;
  CtkTextMark *tmp_mark;
  int i;

  va_start (vap, fmt);
  
  msg = g_strdup_vprintf (fmt, vap);

  ctk_text_buffer_get_end_iter (buffer, &iter);
  ctk_text_buffer_insert (buffer, &iter, msg, -1);

  order_string = g_string_new ("\n  ");
  for (i = 0; i < contents->len; i++)
    {
      if (i)
	g_string_append_c (order_string, ' ');
      g_string_append_c (order_string, g_array_index (contents, char, i));
    }
  g_string_append_c (order_string, '\n');
  ctk_text_buffer_insert (buffer, &iter, order_string->str, -1);
  g_string_free (order_string, TRUE);

  tmp_mark = ctk_text_buffer_create_mark (buffer, NULL, &iter, FALSE);
  ctk_text_view_scroll_mark_onscreen (CTK_TEXT_VIEW (text_view), tmp_mark);
  ctk_text_buffer_delete_mark (buffer, tmp_mark);

  g_free (msg);
}

static CtkWidget *
create_combo (const char *name,
	      gboolean is_list)
{
  CtkCellRenderer *cell_renderer;
  CtkWidget *combo;
  CtkCssProvider *provider;
  CtkStyleContext *context;
  gchar *css_data;

  combo = ctk_combo_box_new_with_model (CTK_TREE_MODEL (model));
  cell_renderer = ctk_cell_renderer_text_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combo), cell_renderer, TRUE);
  ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combo), cell_renderer,
				  "text", 0, NULL);

  ctk_widget_set_name (combo, name);

  context = ctk_widget_get_style_context (combo);

  provider = ctk_css_provider_new ();
  css_data = g_strdup_printf ("#%s { -CtkComboBox-appears-as-list: %s }",
                              name, is_list ? "true" : "false");
  ctk_css_provider_load_from_data (provider, css_data, -1, NULL);
  g_free (css_data);

  ctk_style_context_add_provider (context,
                                  CTK_STYLE_PROVIDER (provider),
                                  CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  return combo;
}

static void
on_insert (void)
{
  CtkTreeIter iter;
  
  int insert_pos;
  char new_value[2];

  new_value[0] = next_value++;
  new_value[1] = '\0';

  if (next_value > 'Z')
    next_value = 'A';
  
  if (contents->len)
    insert_pos = g_random_int_range (0, contents->len + 1);
  else
    insert_pos = 0;
  
  ctk_list_store_insert (model, &iter, insert_pos);
  ctk_list_store_set (model, &iter, 0, new_value, -1);

  g_array_insert_val (contents, insert_pos, new_value);

  combochange_log ("Inserted '%c' at position %d", new_value[0], insert_pos);
}

static void
on_delete (void)
{
  CtkTreeIter iter;
  
  int delete_pos;
  char old_val;

  if (!contents->len)
    return;
  
  delete_pos = g_random_int_range (0, contents->len);
  ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (model), &iter, NULL, delete_pos);
  
  ctk_list_store_remove (model, &iter);

  old_val = g_array_index (contents, char, delete_pos);
  g_array_remove_index (contents, delete_pos);
  combochange_log ("Deleted '%c' from position %d", old_val, delete_pos);
}

static void
on_reorder (void)
{
  GArray *new_contents;
  gint *shuffle_array;
  gint i;

  shuffle_array = g_new (int, contents->len);
  
  for (i = 0; i < contents->len; i++)
    shuffle_array[i] = i;

  for (i = 0; i + 1 < contents->len; i++)
    {
      gint pos = g_random_int_range (i, contents->len);
      gint tmp;

      tmp = shuffle_array[i];
      shuffle_array[i] = shuffle_array[pos];
      shuffle_array[pos] = tmp;
    }

  ctk_list_store_reorder (model, shuffle_array);

  new_contents = g_array_new (FALSE, FALSE, sizeof (char));
  for (i = 0; i < contents->len; i++)
    g_array_append_val (new_contents,
			g_array_index (contents, char, shuffle_array[i]));
  g_array_free (contents, TRUE);
  contents = new_contents;

  combochange_log ("Reordered array");
    
  g_free (shuffle_array);
}

static int n_animations = 0;
static int timer = 0;

static gint
animation_timer (gpointer data)
{
  switch (g_random_int_range (0, 3)) 
    {
    case 0: 
      on_insert ();
      break;
    case 1:
      on_delete ();
      break;
    case 2:
      on_reorder ();
      break;
    }

  n_animations--;
  return n_animations > 0;
}

static void
on_animate (void)
{
  n_animations += 20;
 
  timer = gdk_threads_add_timeout (1000, (GSourceFunc) animation_timer, NULL);
}

int
main (int argc, char **argv)
{
  CtkWidget *content_area;
  CtkWidget *dialog;
  CtkWidget *scrolled_window;
  CtkWidget *hbox;
  CtkWidget *button_vbox;
  CtkWidget *combo_vbox;
  CtkWidget *button;
  CtkWidget *menu_combo;
  CtkWidget *label;
  CtkWidget *list_combo;
  
  test_init ();

  ctk_init (&argc, &argv);

  model = ctk_list_store_new (1, G_TYPE_STRING);
  contents = g_array_new (FALSE, FALSE, sizeof (char));
  
  dialog = ctk_dialog_new_with_buttons ("CtkComboBox model changes",
					NULL, 0,
					"_Close", CTK_RESPONSE_CLOSE,
					NULL);

  content_area = ctk_dialog_get_content_area (CTK_DIALOG (dialog));

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
  ctk_container_set_border_width (CTK_CONTAINER (hbox), 12);
  ctk_box_pack_start (CTK_BOX (content_area), hbox, TRUE, TRUE, 0);

  combo_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
  ctk_box_pack_start (CTK_BOX (hbox), combo_vbox, FALSE, FALSE, 0);

  label = ctk_label_new (NULL);
  ctk_label_set_markup (CTK_LABEL (label), "<b>Menu mode</b>");
  ctk_box_pack_start (CTK_BOX (combo_vbox), label, FALSE, FALSE, 0);

  menu_combo = create_combo ("menu-combo", FALSE);
  ctk_widget_set_margin_start (menu_combo, 12);
  ctk_box_pack_start (CTK_BOX (combo_vbox), menu_combo, FALSE, FALSE, 0);

  label = ctk_label_new (NULL);
  ctk_label_set_markup (CTK_LABEL (label), "<b>List mode</b>");
  ctk_box_pack_start (CTK_BOX (combo_vbox), label, FALSE, FALSE, 0);

  list_combo = create_combo ("list-combo", TRUE);
  ctk_widget_set_margin_start (list_combo, 12);
  ctk_box_pack_start (CTK_BOX (combo_vbox), list_combo, FALSE, FALSE, 0);

  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  ctk_box_pack_start (CTK_BOX (hbox), scrolled_window, TRUE, TRUE, 0);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window),
				  CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);

  text_view = ctk_text_view_new ();
  ctk_text_view_set_editable (CTK_TEXT_VIEW (text_view), FALSE);
  ctk_text_view_set_cursor_visible (CTK_TEXT_VIEW (text_view), FALSE);

  ctk_container_add (CTK_CONTAINER (scrolled_window), text_view);

  button_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
  ctk_box_pack_start (CTK_BOX (hbox), button_vbox, FALSE, FALSE, 0);
  
  ctk_window_set_default_size (CTK_WINDOW (dialog), 500, 300);

  button = ctk_button_new_with_label ("Insert");
  ctk_box_pack_start (CTK_BOX (button_vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (on_insert), NULL);
  
  button = ctk_button_new_with_label ("Delete");
  ctk_box_pack_start (CTK_BOX (button_vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (on_delete), NULL);

  button = ctk_button_new_with_label ("Reorder");
  ctk_box_pack_start (CTK_BOX (button_vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (on_reorder), NULL);

  button = ctk_button_new_with_label ("Animate");
  ctk_box_pack_start (CTK_BOX (button_vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (on_animate), NULL);

  ctk_widget_show_all (dialog);
  ctk_dialog_run (CTK_DIALOG (dialog));

  return 0;
}
