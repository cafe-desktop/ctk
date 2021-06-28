#include <ctk/ctk.h>

typedef CtkListStore MyModel;
typedef CtkListStoreClass MyModelClass;

static void my_model_drag_source_init (CtkTreeDragSourceIface *iface);

G_DEFINE_TYPE_WITH_CODE (MyModel, my_model, CTK_TYPE_LIST_STORE,
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_TREE_DRAG_SOURCE,
                                                my_model_drag_source_init))

static void
my_model_class_init (MyModelClass *class)
{
}

static void
my_model_init (MyModel *object)
{
  GType types[1] = { G_TYPE_STRING };

  ctk_list_store_set_column_types (CTK_LIST_STORE (object), G_N_ELEMENTS (types), types);
}

static gboolean
my_model_drag_data_get (CtkTreeDragSource *source,
                        CtkTreePath       *path,
                        CtkSelectionData  *data)
{
  CtkTreeIter iter;
  gchar *text;

  ctk_tree_model_get_iter (CTK_TREE_MODEL (source), &iter, path);
  ctk_tree_model_get (CTK_TREE_MODEL (source), &iter, 0, &text, -1);
  ctk_selection_data_set_text (data, text, -1);
  g_free (text);

  return TRUE;
}

static void
my_model_drag_source_init (CtkTreeDragSourceIface *iface)
{
  static CtkTreeDragSourceIface *parent;

  parent = g_type_interface_peek_parent (iface);

  iface->row_draggable = parent->row_draggable;
  iface->drag_data_delete = parent->drag_data_delete;
  iface->drag_data_get = my_model_drag_data_get;
}

static CtkTreeModel *
get_model (void)
{
  MyModel *model;

  model = g_object_new (my_model_get_type (), NULL);
  ctk_list_store_insert_with_values (CTK_LIST_STORE (model), NULL, -1, 0, "Item 1", -1);
  ctk_list_store_insert_with_values (CTK_LIST_STORE (model), NULL, -1, 0, "Item 2", -1);
  ctk_list_store_insert_with_values (CTK_LIST_STORE (model), NULL, -1, 0, "Item 3", -1);

  return CTK_TREE_MODEL (model);
}

static CtkTargetEntry entries[] = {
  { "text/plain", 0, 0 }
};

static CtkWidget *
get_dragsource (void)
{
  CtkTreeView *tv;
  CtkCellRenderer *renderer;
  CtkTreeViewColumn *column;

  tv = (CtkTreeView*) ctk_tree_view_new ();
  renderer = ctk_cell_renderer_text_new ();
  column = ctk_tree_view_column_new_with_attributes ("Text", renderer, "text", 0, NULL);
  ctk_tree_view_append_column (tv, column);

  ctk_tree_view_set_model (tv, get_model ());
  ctk_tree_view_enable_model_drag_source (tv, CDK_BUTTON1_MASK, entries, G_N_ELEMENTS (entries), CDK_ACTION_COPY);

  return CTK_WIDGET (tv);
}

static void
data_received (CtkWidget *widget,
               CdkDragContext *context,
               gint x, gint y,
               CtkSelectionData *selda,
               guint info, guint time,
               gpointer dada)
{
  gchar *text;

  text = (gchar*) ctk_selection_data_get_text (selda);
  ctk_label_set_label (CTK_LABEL (widget), text);
  g_free (text);
}

static CtkWidget *
get_droptarget (void)
{
  CtkWidget *label;

  label = ctk_label_new ("Drop here");
  ctk_drag_dest_set (label, CTK_DEST_DEFAULT_ALL, entries, G_N_ELEMENTS (entries), CDK_ACTION_COPY);
  g_signal_connect (label, "drag-data-received", G_CALLBACK (data_received), NULL);

  return label;
}

int
main (int argc, char *argv[])
{
  CtkWidget *window;
  CtkWidget *box;

  ctk_init (NULL, NULL);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_container_add (CTK_CONTAINER (window), box);
  ctk_container_add (CTK_CONTAINER (box), get_dragsource ());
  ctk_container_add (CTK_CONTAINER (box), get_droptarget ());

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
