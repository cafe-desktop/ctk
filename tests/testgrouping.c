#include <gtk/gtk.h>


static GtkTreeModel *
create_model (void)
{
  GtkTreeStore *store;
  GtkTreeIter iter;
  GtkTreeIter parent;

  store = ctk_tree_store_new (1, G_TYPE_STRING);

  ctk_tree_store_insert_with_values (store, &parent, NULL, 0,
				     0, "Applications", -1);

  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "File Manager", -1);
  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "Gossip", -1);
  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "System Settings", -1);
  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "The GIMP", -1);
  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "Terminal", -1);
  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "Word Processor", -1);


  ctk_tree_store_insert_with_values (store, &parent, NULL, 1,
				     0, "Documents", -1);

  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "blaat.txt", -1);
  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "sliff.txt", -1);
  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "test.txt", -1);
  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "blaat.txt", -1);
  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "brrrr.txt", -1);
  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "hohoho.txt", -1);


  ctk_tree_store_insert_with_values (store, &parent, NULL, 2,
				     0, "Images", -1);

  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "image1.png", -1);
  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "image2.png", -1);
  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "image3.jpg", -1);

  return CTK_TREE_MODEL (store);
}

static void
set_color_func (GtkTreeViewColumn *column,
		GtkCellRenderer   *cell,
		GtkTreeModel      *model,
		GtkTreeIter       *iter,
		gpointer           data)
{
  if (ctk_tree_model_iter_has_child (model, iter))
    g_object_set (cell, "cell-background", "Grey", NULL);
  else
    g_object_set (cell, "cell-background", NULL, NULL);
}

static void
tree_view_row_activated (GtkTreeView       *tree_view,
			 GtkTreePath       *path,
			 GtkTreeViewColumn *column)
{
  if (ctk_tree_path_get_depth (path) > 1)
    return;

  if (ctk_tree_view_row_expanded (CTK_TREE_VIEW (tree_view), path))
    ctk_tree_view_collapse_row (CTK_TREE_VIEW (tree_view), path);
  else
    ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path, FALSE);
}

static gboolean
tree_view_select_func (GtkTreeSelection *selection,
		       GtkTreeModel     *model,
		       GtkTreePath      *path,
		       gboolean          path_currently_selected,
		       gpointer          data)
{
  if (ctk_tree_path_get_depth (path) > 1)
    return TRUE;

  return FALSE;
}

int
main (int argc, char **argv)
{
  GtkWidget *window, *sw, *tv;
  GtkTreeModel *model;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  ctk_init (&argc, &argv);

  model = create_model ();

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "delete_event",
		    G_CALLBACK (ctk_main_quit), NULL);
  ctk_window_set_default_size (CTK_WINDOW (window), 320, 480);

  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_container_add (CTK_CONTAINER (window), sw);

  tv = ctk_tree_view_new_with_model (model);
  ctk_container_add (CTK_CONTAINER (sw), tv);

  g_signal_connect (tv, "row-activated",
		    G_CALLBACK (tree_view_row_activated), tv);
  g_object_set (tv,
		"show-expanders", FALSE,
		"level-indentation", 10,
		NULL);

  ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (tv), FALSE);
  ctk_tree_view_expand_all (CTK_TREE_VIEW (tv));

  ctk_tree_selection_set_select_function (ctk_tree_view_get_selection (CTK_TREE_VIEW (tv)),
					  tree_view_select_func,
					  NULL,
					  NULL);

  renderer = ctk_cell_renderer_text_new ();
  column = ctk_tree_view_column_new_with_attributes ("(none)",
						     renderer,
						     "text", 0,
						     NULL);
  ctk_tree_view_column_set_cell_data_func (column,
					   renderer,
					   set_color_func,
					   NULL,
					   NULL);
  ctk_tree_view_insert_column (CTK_TREE_VIEW (tv), column, 0);

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
