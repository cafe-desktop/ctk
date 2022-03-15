/* Tree View/Filter Model
 *
 * This example demonstrates how CtkTreeModelFilter can be used not
 * just to show a subset of the rows, but also to compute columns
 * that are not actually present in the underlying model.
 */

#include <ctk/ctk.h>
#include <stdlib.h>

enum {
  WIDTH_COLUMN,
  HEIGHT_COLUMN,
  AREA_COLUMN,
  SQUARE_COLUMN
};

static void
format_number (CtkTreeViewColumn *col,
               CtkCellRenderer   *cell,
               CtkTreeModel      *model,
               CtkTreeIter       *iter,
               gpointer           data)
{
  gint num;
  gchar *text;

  ctk_tree_model_get (model, iter, GPOINTER_TO_INT (data), &num, -1);
  text = g_strdup_printf ("%d", num);
  g_object_set (cell, "text", text, NULL);
  g_free (text);
}

static void
filter_modify_func (CtkTreeModel *model,
                    CtkTreeIter  *iter,
                    GValue       *value,
                    gint          column,
                    gpointer      data)
{
  CtkTreeModelFilter *filter_model = CTK_TREE_MODEL_FILTER (model);
  gint width, height;
  CtkTreeModel *child_model;
  CtkTreeIter child_iter;

  child_model = ctk_tree_model_filter_get_model (filter_model);
  ctk_tree_model_filter_convert_iter_to_child_iter (filter_model, &child_iter, iter);

  ctk_tree_model_get (child_model, &child_iter,
                      WIDTH_COLUMN, &width,
                      HEIGHT_COLUMN, &height,
                      -1);

  switch (column)
    {
    case WIDTH_COLUMN:
      g_value_set_int (value, width);
      break;
    case HEIGHT_COLUMN:
      g_value_set_int (value, height);
      break;
    case AREA_COLUMN:
      g_value_set_int (value, width * height);
      break;
    case SQUARE_COLUMN:
      g_value_set_boolean (value, width == height);
      break;
    default:
      g_assert_not_reached ();
    }
}

static gboolean
visible_func (CtkTreeModel *model,
              CtkTreeIter  *iter,
              gpointer      data)
{
  gint width;

  ctk_tree_model_get (model, iter,
                      WIDTH_COLUMN, &width,
                      -1);

  return width < 10;
}

static void
cell_edited (CtkCellRendererSpin *cell,
             const char          *path_string,
             const char          *new_text,
             CtkListStore        *store)
{
  int val;
  CtkTreePath *path;
  CtkTreeIter iter;
  int column;

  path = ctk_tree_path_new_from_string (path_string);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (store), &iter, path);
  ctk_tree_path_free (path);

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

  val = atoi (new_text);

  ctk_list_store_set (store, &iter, column, val, -1);
}

CtkWidget *
do_filtermodel (CtkWidget *do_widget)
{
  static CtkWidget *window;

  if (!window)
    {
      CtkWidget *tree;
      CtkListStore *store;
      CtkTreeModel *model;
      CtkTreeViewColumn *column;
      CtkCellRenderer *cell;
      GType types[4];
      CtkBuilder *builder;

      builder = ctk_builder_new_from_resource ("/filtermodel/filtermodel.ui");
      ctk_builder_connect_signals (builder, NULL);
      window = CTK_WIDGET (ctk_builder_get_object (builder, "window1"));
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      store = (CtkListStore*)ctk_builder_get_object (builder, "liststore1");

      column = (CtkTreeViewColumn*)ctk_builder_get_object (builder, "treeviewcolumn1");
      cell = (CtkCellRenderer*)ctk_builder_get_object (builder, "cellrenderertext1");
      ctk_tree_view_column_set_cell_data_func (column, cell,
                                               format_number, GINT_TO_POINTER (WIDTH_COLUMN), NULL);
      g_object_set_data (G_OBJECT (cell), "column", GINT_TO_POINTER (WIDTH_COLUMN));
      g_signal_connect (cell, "edited", G_CALLBACK (cell_edited), store);

      column = (CtkTreeViewColumn*)ctk_builder_get_object (builder, "treeviewcolumn2");
      cell = (CtkCellRenderer*)ctk_builder_get_object (builder, "cellrenderertext2");
      ctk_tree_view_column_set_cell_data_func (column, cell,
                                               format_number, GINT_TO_POINTER (HEIGHT_COLUMN), NULL);
      g_object_set_data (G_OBJECT (cell), "column", GINT_TO_POINTER (HEIGHT_COLUMN));
      g_signal_connect (cell, "edited", G_CALLBACK (cell_edited), store);

      column = (CtkTreeViewColumn*)ctk_builder_get_object (builder, "treeviewcolumn3");
      cell = (CtkCellRenderer*)ctk_builder_get_object (builder, "cellrenderertext3");
      ctk_tree_view_column_set_cell_data_func (column, cell,
                                               format_number, GINT_TO_POINTER (WIDTH_COLUMN), NULL);

      column = (CtkTreeViewColumn*)ctk_builder_get_object (builder, "treeviewcolumn4");
      cell = (CtkCellRenderer*)ctk_builder_get_object (builder, "cellrenderertext4");
      ctk_tree_view_column_set_cell_data_func (column, cell,
                                               format_number, GINT_TO_POINTER (HEIGHT_COLUMN), NULL);

      column = (CtkTreeViewColumn*)ctk_builder_get_object (builder, "treeviewcolumn5");
      cell = (CtkCellRenderer*)ctk_builder_get_object (builder, "cellrenderertext5");
      ctk_tree_view_column_set_cell_data_func (column, cell,
                                               format_number, GINT_TO_POINTER (AREA_COLUMN), NULL);

      column = (CtkTreeViewColumn*)ctk_builder_get_object (builder, "treeviewcolumn6");
      cell = (CtkCellRenderer*)ctk_builder_get_object (builder, "cellrendererpixbuf1");
      ctk_tree_view_column_add_attribute (column, cell, "visible", SQUARE_COLUMN);

      tree = (CtkWidget*)ctk_builder_get_object (builder, "treeview2");

      types[WIDTH_COLUMN] = G_TYPE_INT;
      types[HEIGHT_COLUMN] = G_TYPE_INT;
      types[AREA_COLUMN] = G_TYPE_INT;
      types[SQUARE_COLUMN] = G_TYPE_BOOLEAN;
      model = ctk_tree_model_filter_new (CTK_TREE_MODEL (store), NULL);
      ctk_tree_model_filter_set_modify_func (CTK_TREE_MODEL_FILTER (model),
                                             G_N_ELEMENTS (types), types,
                                             filter_modify_func, NULL, NULL);

      ctk_tree_view_set_model (CTK_TREE_VIEW (tree), model);

      column = (CtkTreeViewColumn*)ctk_builder_get_object (builder, "treeviewcolumn7");
      cell = (CtkCellRenderer*)ctk_builder_get_object (builder, "cellrenderertext6");
      ctk_tree_view_column_set_cell_data_func (column, cell,
                                               format_number, GINT_TO_POINTER (WIDTH_COLUMN), NULL);

      column = (CtkTreeViewColumn*)ctk_builder_get_object (builder, "treeviewcolumn8");
      cell = (CtkCellRenderer*)ctk_builder_get_object (builder, "cellrenderertext7");
      ctk_tree_view_column_set_cell_data_func (column, cell,
                                               format_number, GINT_TO_POINTER (HEIGHT_COLUMN), NULL);

      tree = (CtkWidget*)ctk_builder_get_object (builder, "treeview3");

      model = ctk_tree_model_filter_new (CTK_TREE_MODEL (store), NULL);
      ctk_tree_model_filter_set_visible_func (CTK_TREE_MODEL_FILTER (model),
                                              visible_func, NULL, NULL);
      ctk_tree_view_set_model (CTK_TREE_VIEW (tree), model);

      g_object_unref (builder);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
