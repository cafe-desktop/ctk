/* testtreeflow.c
 * Copyright (C) 2001 Red Hat, Inc
 * Author: Jonathan Blandford
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

CtkTreeModel *model = NULL;
static GRand *grand = NULL;
CtkTreeSelection *selection = NULL;
enum
{
  TEXT_COLUMN,
  NUM_COLUMNS
};

static char *words[] =
{
  "Boom",
  "Borp",
  "Multiline\ntext",
  "Bingo",
  "Veni\nVedi\nVici",
  NULL
};


#define NUM_WORDS 5
#define NUM_ROWS 100


static void
initialize_model (void)
{
  gint i;
  CtkTreeIter iter;

  model = (CtkTreeModel *) ctk_list_store_new (NUM_COLUMNS, G_TYPE_STRING);
  grand = g_rand_new ();
  for (i = 0; i < NUM_ROWS; i++)
    {
      ctk_list_store_append (CTK_LIST_STORE (model), &iter);
      ctk_list_store_set (CTK_LIST_STORE (model), &iter,
			  TEXT_COLUMN, words[g_rand_int_range (grand, 0, NUM_WORDS)],
			  -1);
    }
}

static void
futz_row (void)
{
  gint i;
  CtkTreePath *path;
  CtkTreeIter iter;
  CtkTreeIter iter2;

  i = g_rand_int_range (grand, 0,
			ctk_tree_model_iter_n_children (model, NULL));
  path = ctk_tree_path_new ();
  ctk_tree_path_append_index (path, i);
  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_path_free (path);

  if (ctk_tree_selection_iter_is_selected (selection, &iter))
    return;
  switch (g_rand_int_range (grand, 0, 3))
    {
    case 0:
      /* insert */
            ctk_list_store_insert_after (CTK_LIST_STORE (model),
            				   &iter2, &iter);
            ctk_list_store_set (CTK_LIST_STORE (model), &iter2,
            			  TEXT_COLUMN, words[g_rand_int_range (grand, 0, NUM_WORDS)],
            			  -1);
      break;
    case 1:
      /* delete */
      if (ctk_tree_model_iter_n_children (model, NULL) == 0)
	return;
      ctk_list_store_remove (CTK_LIST_STORE (model), &iter);
      break;
    case 2:
      /* modify */
      return;
      if (ctk_tree_model_iter_n_children (model, NULL) == 0)
	return;
      ctk_list_store_set (CTK_LIST_STORE (model), &iter,
      			  TEXT_COLUMN, words[g_rand_int_range (grand, 0, NUM_WORDS)],
      			  -1);
      break;
    }
}

static gboolean
futz (void)
{
  gint i;

  for (i = 0; i < 15; i++)
    futz_row ();
  g_print ("Number of rows: %d\n", ctk_tree_model_iter_n_children (model, NULL));
  return TRUE;
}

int
main (int argc, char *argv[])
{
  CtkWidget *window;
  CtkWidget *vbox;
  CtkWidget *scrolled_window;
  CtkWidget *tree_view;
  CtkWidget *hbox;
  CtkWidget *button;
  CtkTreePath *path;

  ctk_init (&argc, &argv);

  path = ctk_tree_path_new_from_string ("80");
  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "Reflow test");
  g_signal_connect (window, "destroy", ctk_main_quit, NULL);
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
  ctk_container_set_border_width (CTK_CONTAINER (vbox), 8);
  ctk_box_pack_start (CTK_BOX (vbox), ctk_label_new ("Incremental Reflow Test"), FALSE, FALSE, 0);
  ctk_container_add (CTK_CONTAINER (window), vbox);
  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window),
				  CTK_POLICY_AUTOMATIC,
				  CTK_POLICY_AUTOMATIC);
  ctk_box_pack_start (CTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  
  initialize_model ();
  tree_view = ctk_tree_view_new_with_model (model);
  ctk_tree_view_scroll_to_cell (CTK_TREE_VIEW (tree_view), path, NULL, TRUE, 0.5, 0.0);
  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (tree_view));
  ctk_tree_selection_select_path (selection, path);
  ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (tree_view), FALSE);
  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
					       -1,
					       NULL,
					       ctk_cell_renderer_text_new (),
					       "text", TEXT_COLUMN,
					       NULL);
  ctk_container_add (CTK_CONTAINER (scrolled_window), tree_view);
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  button = ctk_button_new_with_mnemonic ("<b>_Futz!!</b>");
  ctk_box_pack_start (CTK_BOX (hbox), button, FALSE, FALSE, 0);
  ctk_label_set_use_markup (CTK_LABEL (ctk_bin_get_child (CTK_BIN (button))), TRUE);
  g_signal_connect (button, "clicked", G_CALLBACK (futz), NULL);
  g_signal_connect (button, "realize", G_CALLBACK (ctk_widget_grab_focus), NULL);
  ctk_window_set_default_size (CTK_WINDOW (window), 300, 400);
  ctk_widget_show_all (window);
  cdk_threads_add_timeout (1000, (GSourceFunc) futz, NULL);
  ctk_main ();
  return 0;
}
