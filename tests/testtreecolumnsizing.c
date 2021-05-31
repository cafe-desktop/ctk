/* testtreecolumnsizing.c: Test case for tree view column resizing.
 *
 * Copyright (C) 2008  Kristian Rietveld  <kris@gtk.org>
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <gtk/gtk.h>
#include <string.h>

#define NO_EXPAND "No expandable columns"
#define SINGLE_EXPAND "One expandable column"
#define MULTI_EXPAND "Multiple expandable columns"
#define LAST_EXPAND "Last column is expandable"
#define BORDER_EXPAND "First and last columns are expandable"
#define ALL_EXPAND "All columns are expandable"

#define N_ROWS 10


static GtkTreeModel *
create_model (void)
{
  int i;
  GtkListStore *store;

  store = ctk_list_store_new (5,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  for (i = 0; i < N_ROWS; i++)
    {
      gchar *str;

      str = g_strdup_printf ("Row %d", i);
      ctk_list_store_insert_with_values (store, NULL, i,
                                         0, str,
                                         1, "Blah blah blah blah blah",
                                         2, "Less blah",
                                         3, "Medium length",
                                         4, "Eek",
                                         -1);
      g_free (str);
    }

  return CTK_TREE_MODEL (store);
}

static void
toggle_long_content_row (GtkToggleButton *button,
                         gpointer         user_data)
{
  GtkTreeModel *model;

  model = ctk_tree_view_get_model (CTK_TREE_VIEW (user_data));
  if (ctk_tree_model_iter_n_children (model, NULL) == N_ROWS)
    {
      ctk_list_store_insert_with_values (CTK_LIST_STORE (model), NULL, N_ROWS,
                                         0, "Very very very very longggggg",
                                         1, "Blah blah blah blah blah",
                                         2, "Less blah",
                                         3, "Medium length",
                                         4, "Eek we make the scrollbar appear",
                                         -1);
    }
  else
    {
      GtkTreeIter iter;

      ctk_tree_model_iter_nth_child (model, &iter, NULL, N_ROWS);
      ctk_list_store_remove (CTK_LIST_STORE (model), &iter);
    }
}

static void
combo_box_changed (GtkComboBox *combo_box,
                   gpointer     user_data)
{
  gchar *str;
  GList *list;
  GList *columns;

  str = ctk_combo_box_text_get_active_text (CTK_COMBO_BOX_TEXT (combo_box));
  if (!str)
    return;

  columns = ctk_tree_view_get_columns (CTK_TREE_VIEW (user_data));

  if (!strcmp (str, NO_EXPAND))
    {
      for (list = columns; list; list = list->next)
        ctk_tree_view_column_set_expand (list->data, FALSE);
    }
  else if (!strcmp (str, SINGLE_EXPAND))
    {
      for (list = columns; list; list = list->next)
        {
          if (list->prev && !list->prev->prev)
            /* This is the second column */
            ctk_tree_view_column_set_expand (list->data, TRUE);
          else
            ctk_tree_view_column_set_expand (list->data, FALSE);
        }
    }
  else if (!strcmp (str, MULTI_EXPAND))
    {
      for (list = columns; list; list = list->next)
        {
          if (list->prev && !list->prev->prev)
            /* This is the second column */
            ctk_tree_view_column_set_expand (list->data, TRUE);
          else if (list->prev && !list->prev->prev->prev)
            /* This is the third column */
            ctk_tree_view_column_set_expand (list->data, TRUE);
          else
            ctk_tree_view_column_set_expand (list->data, FALSE);
        }
    }
  else if (!strcmp (str, LAST_EXPAND))
    {
      for (list = columns; list->next; list = list->next)
        ctk_tree_view_column_set_expand (list->data, FALSE);
      /* This is the last column */
      ctk_tree_view_column_set_expand (list->data, TRUE);
    }
  else if (!strcmp (str, BORDER_EXPAND))
    {
      ctk_tree_view_column_set_expand (columns->data, TRUE);
      for (list = columns->next; list->next; list = list->next)
        ctk_tree_view_column_set_expand (list->data, FALSE);
      /* This is the last column */
      ctk_tree_view_column_set_expand (list->data, TRUE);
    }
  else if (!strcmp (str, ALL_EXPAND))
    {
      for (list = columns; list; list = list->next)
        ctk_tree_view_column_set_expand (list->data, TRUE);
    }

  g_free (str);
  g_list_free (columns);
}

int
main (int argc, char **argv)
{
  int i;
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *combo_box;
  GtkWidget *sw;
  GtkWidget *tree_view;
  GtkWidget *button;

  ctk_init (&argc, &argv);

  /* Window and box */
  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window), 640, 480);
  g_signal_connect (window, "delete-event", G_CALLBACK (ctk_main_quit), NULL);
  ctk_container_set_border_width (CTK_CONTAINER (window), 5);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  /* Option menu contents */
  combo_box = ctk_combo_box_text_new ();

  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), NO_EXPAND);
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), SINGLE_EXPAND);
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), MULTI_EXPAND);
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), LAST_EXPAND);
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), BORDER_EXPAND);
  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), ALL_EXPAND);

  ctk_box_pack_start (CTK_BOX (vbox), combo_box, FALSE, FALSE, 0);

  /* Scrolled window and tree view */
  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                  CTK_POLICY_AUTOMATIC,
                                  CTK_POLICY_AUTOMATIC);
  ctk_box_pack_start (CTK_BOX (vbox), sw, TRUE, TRUE, 0);

  tree_view = ctk_tree_view_new_with_model (create_model ());
  ctk_container_add (CTK_CONTAINER (sw), tree_view);

  for (i = 0; i < 5; i++)
    {
      GtkTreeViewColumn *column;

      ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
                                                   i, "Header",
                                                   ctk_cell_renderer_text_new (),
                                                   "text", i,
                                                   NULL);

      column = ctk_tree_view_get_column (CTK_TREE_VIEW (tree_view), i);
      ctk_tree_view_column_set_resizable (column, TRUE);
    }

  /* Toggle button for long content row */
  button = ctk_toggle_button_new_with_label ("Toggle long content row");
  g_signal_connect (button, "toggled",
                    G_CALLBACK (toggle_long_content_row), tree_view);
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);

  /* Set up option menu callback and default item */
  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (combo_box_changed), tree_view);
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo_box), 0);

  /* Done */
  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
