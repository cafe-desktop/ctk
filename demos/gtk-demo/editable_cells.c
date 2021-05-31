/* Tree View/Editable Cells
 *
 * This demo demonstrates the use of editable cells in a GtkTreeView. If
 * you're new to the GtkTreeView widgets and associates, look into
 * the GtkListStore example first. It also shows how to use the
 * GtkCellRenderer::editing-started signal to do custom setup of the
 * editable widget.
 *
 * The cell renderers used in this demo are GtkCellRendererText,
 * GtkCellRendererCombo and GtkCellRendererProgress.
 */

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

typedef struct
{
  gint   number;
  gchar *product;
  gint   yummy;
}
Item;

enum
{
  COLUMN_ITEM_NUMBER,
  COLUMN_ITEM_PRODUCT,
  COLUMN_ITEM_YUMMY,
  NUM_ITEM_COLUMNS
};

enum
{
  COLUMN_NUMBER_TEXT,
  NUM_NUMBER_COLUMNS
};

static GArray *articles = NULL;

static void
add_items (void)
{
  Item foo;

  g_return_if_fail (articles != NULL);

  foo.number = 3;
  foo.product = g_strdup ("bottles of coke");
  foo.yummy = 20;
  g_array_append_vals (articles, &foo, 1);

  foo.number = 5;
  foo.product = g_strdup ("packages of noodles");
  foo.yummy = 50;
  g_array_append_vals (articles, &foo, 1);

  foo.number = 2;
  foo.product = g_strdup ("packages of chocolate chip cookies");
  foo.yummy = 90;
  g_array_append_vals (articles, &foo, 1);

  foo.number = 1;
  foo.product = g_strdup ("can vanilla ice cream");
  foo.yummy = 60;
  g_array_append_vals (articles, &foo, 1);

  foo.number = 6;
  foo.product = g_strdup ("eggs");
  foo.yummy = 10;
  g_array_append_vals (articles, &foo, 1);
}

static GtkTreeModel *
create_items_model (void)
{
  gint i = 0;
  GtkListStore *model;
  GtkTreeIter iter;

  /* create array */
  articles = g_array_sized_new (FALSE, FALSE, sizeof (Item), 1);

  add_items ();

  /* create list store */
  model = ctk_list_store_new (NUM_ITEM_COLUMNS, G_TYPE_INT, G_TYPE_STRING,
                              G_TYPE_INT, G_TYPE_BOOLEAN);

  /* add items */
  for (i = 0; i < articles->len; i++)
    {
      ctk_list_store_append (model, &iter);

      ctk_list_store_set (model, &iter,
                          COLUMN_ITEM_NUMBER,
                          g_array_index (articles, Item, i).number,
                          COLUMN_ITEM_PRODUCT,
                          g_array_index (articles, Item, i).product,
                          COLUMN_ITEM_YUMMY,
                          g_array_index (articles, Item, i).yummy,
                          -1);
    }

  return CTK_TREE_MODEL (model);
}

static GtkTreeModel *
create_numbers_model (void)
{
#define N_NUMBERS 10
  gint i = 0;
  GtkListStore *model;
  GtkTreeIter iter;

  /* create list store */
  model = ctk_list_store_new (NUM_NUMBER_COLUMNS, G_TYPE_STRING, G_TYPE_INT);

  /* add numbers */
  for (i = 0; i < N_NUMBERS; i++)
    {
      char str[2];

      str[0] = '0' + i;
      str[1] = '\0';

      ctk_list_store_append (model, &iter);

      ctk_list_store_set (model, &iter,
                          COLUMN_NUMBER_TEXT, str,
                          -1);
    }

  return CTK_TREE_MODEL (model);

#undef N_NUMBERS
}

static void
add_item (GtkWidget *button, gpointer data)
{
  Item foo;
  GtkTreeIter current, iter;
  GtkTreePath *path;
  GtkTreeModel *model;
  GtkTreeViewColumn *column;
  GtkTreeView *treeview = (GtkTreeView *)data;

  g_return_if_fail (articles != NULL);

  foo.number = 0;
  foo.product = g_strdup ("Description here");
  foo.yummy = 50;
  g_array_append_vals (articles, &foo, 1);

  /* Insert a new row below the current one */
  ctk_tree_view_get_cursor (treeview, &path, NULL);
  model = ctk_tree_view_get_model (treeview);
  if (path)
    {
      ctk_tree_model_get_iter (model, &current, path);
      ctk_tree_path_free (path);
      ctk_list_store_insert_after (CTK_LIST_STORE (model), &iter, &current);
    }
  else
    {
      ctk_list_store_insert (CTK_LIST_STORE (model), &iter, -1);
    }

  /* Set the data for the new row */
  ctk_list_store_set (CTK_LIST_STORE (model), &iter,
                      COLUMN_ITEM_NUMBER, foo.number,
                      COLUMN_ITEM_PRODUCT, foo.product,
                      COLUMN_ITEM_YUMMY, foo.yummy,
                      -1);

  /* Move focus to the new row */
  path = ctk_tree_model_get_path (model, &iter);
  column = ctk_tree_view_get_column (treeview, 0);
  ctk_tree_view_set_cursor (treeview, path, column, FALSE);

  ctk_tree_path_free (path);
}

static void
remove_item (GtkWidget *widget, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeView *treeview = (GtkTreeView *)data;
  GtkTreeModel *model = ctk_tree_view_get_model (treeview);
  GtkTreeSelection *selection = ctk_tree_view_get_selection (treeview);

  if (ctk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gint i;
      GtkTreePath *path;

      path = ctk_tree_model_get_path (model, &iter);
      i = ctk_tree_path_get_indices (path)[0];
      ctk_list_store_remove (CTK_LIST_STORE (model), &iter);

      g_array_remove_index (articles, i);

      ctk_tree_path_free (path);
    }
}

static gboolean
separator_row (GtkTreeModel *model,
               GtkTreeIter  *iter,
               gpointer      data)
{
  GtkTreePath *path;
  gint idx;

  path = ctk_tree_model_get_path (model, iter);
  idx = ctk_tree_path_get_indices (path)[0];

  ctk_tree_path_free (path);

  return idx == 5;
}

static void
editing_started (GtkCellRenderer *cell,
                 GtkCellEditable *editable,
                 const gchar     *path,
                 gpointer         data)
{
  ctk_combo_box_set_row_separator_func (CTK_COMBO_BOX (editable),
                                        separator_row, NULL, NULL);
}

static void
cell_edited (GtkCellRendererText *cell,
             const gchar         *path_string,
             const gchar         *new_text,
             gpointer             data)
{
  GtkTreeModel *model = (GtkTreeModel *)data;
  GtkTreePath *path = ctk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;

  gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

  ctk_tree_model_get_iter (model, &iter, path);

  switch (column)
    {
    case COLUMN_ITEM_NUMBER:
      {
        gint i;

        i = ctk_tree_path_get_indices (path)[0];
        g_array_index (articles, Item, i).number = atoi (new_text);

        ctk_list_store_set (CTK_LIST_STORE (model), &iter, column,
                            g_array_index (articles, Item, i).number, -1);
      }
      break;

    case COLUMN_ITEM_PRODUCT:
      {
        gint i;
        gchar *old_text;

        ctk_tree_model_get (model, &iter, column, &old_text, -1);
        g_free (old_text);

        i = ctk_tree_path_get_indices (path)[0];
        g_free (g_array_index (articles, Item, i).product);
        g_array_index (articles, Item, i).product = g_strdup (new_text);

        ctk_list_store_set (CTK_LIST_STORE (model), &iter, column,
                            g_array_index (articles, Item, i).product, -1);
      }
      break;
    }

  ctk_tree_path_free (path);
}

static void
add_columns (GtkTreeView  *treeview,
             GtkTreeModel *items_model,
             GtkTreeModel *numbers_model)
{
  GtkCellRenderer *renderer;

  /* number column */
  renderer = ctk_cell_renderer_combo_new ();
  g_object_set (renderer,
                "model", numbers_model,
                "text-column", COLUMN_NUMBER_TEXT,
                "has-entry", FALSE,
                "editable", TRUE,
                NULL);
  g_signal_connect (renderer, "edited",
                    G_CALLBACK (cell_edited), items_model);
  g_signal_connect (renderer, "editing-started",
                    G_CALLBACK (editing_started), NULL);
  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_ITEM_NUMBER));

  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (treeview),
                                               -1, "Number", renderer,
                                               "text", COLUMN_ITEM_NUMBER,
                                               NULL);

  /* product column */
  renderer = ctk_cell_renderer_text_new ();
  g_object_set (renderer,
                "editable", TRUE,
                NULL);
  g_signal_connect (renderer, "edited",
                    G_CALLBACK (cell_edited), items_model);
  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_ITEM_PRODUCT));

  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (treeview),
                                               -1, "Product", renderer,
                                               "text", COLUMN_ITEM_PRODUCT,
                                               NULL);

  /* yummy column */
  renderer = ctk_cell_renderer_progress_new ();
  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_ITEM_YUMMY));

  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (treeview),
                                               -1, "Yummy", renderer,
                                               "value", COLUMN_ITEM_YUMMY,
                                               NULL);
}

GtkWidget *
do_editable_cells (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      GtkWidget *vbox;
      GtkWidget *hbox;
      GtkWidget *sw;
      GtkWidget *treeview;
      GtkWidget *button;
      GtkTreeModel *items_model;
      GtkTreeModel *numbers_model;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Editable Cells");
      ctk_container_set_border_width (CTK_CONTAINER (window), 5);
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
      ctk_container_add (CTK_CONTAINER (window), vbox);

      ctk_box_pack_start (CTK_BOX (vbox),
                          ctk_label_new ("Shopping list (you can edit the cells!)"),
                          FALSE, FALSE, 0);

      sw = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (sw),
                                           CTK_SHADOW_ETCHED_IN);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                      CTK_POLICY_AUTOMATIC,
                                      CTK_POLICY_AUTOMATIC);
      ctk_box_pack_start (CTK_BOX (vbox), sw, TRUE, TRUE, 0);

      /* create models */
      items_model = create_items_model ();
      numbers_model = create_numbers_model ();

      /* create tree view */
      treeview = ctk_tree_view_new_with_model (items_model);
      ctk_tree_selection_set_mode (ctk_tree_view_get_selection (CTK_TREE_VIEW (treeview)),
                                   CTK_SELECTION_SINGLE);

      add_columns (CTK_TREE_VIEW (treeview), items_model, numbers_model);

      g_object_unref (numbers_model);
      g_object_unref (items_model);

      ctk_container_add (CTK_CONTAINER (sw), treeview);

      /* some buttons */
      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 4);
      ctk_box_set_homogeneous (CTK_BOX (hbox), TRUE);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      button = ctk_button_new_with_label ("Add item");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (add_item), treeview);
      ctk_box_pack_start (CTK_BOX (hbox), button, TRUE, TRUE, 0);

      button = ctk_button_new_with_label ("Remove item");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (remove_item), treeview);
      ctk_box_pack_start (CTK_BOX (hbox), button, TRUE, TRUE, 0);

      ctk_window_set_default_size (CTK_WINDOW (window), 320, 200);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
