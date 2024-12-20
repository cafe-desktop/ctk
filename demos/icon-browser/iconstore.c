#include "iconstore.h"
#include <ctk/ctk.h>

struct _IconStore
{
  CtkListStore parent;

  gint text_column;
};

struct _IconStoreClass
{
  CtkListStoreClass parent_class;
};

static void icon_store_drag_source_init (CtkTreeDragSourceIface *iface);

G_DEFINE_TYPE_WITH_CODE (IconStore, icon_store, CTK_TYPE_LIST_STORE,
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_TREE_DRAG_SOURCE,
                                                icon_store_drag_source_init))


static void
icon_store_init (IconStore *store)
{
  GType types[4] = { G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING };

  ctk_list_store_set_column_types (CTK_LIST_STORE (store), 4, types);

  store->text_column = ICON_STORE_NAME_COLUMN;
}

static void
icon_store_class_init (IconStoreClass *class G_GNUC_UNUSED)
{
}

static gboolean
row_draggable (CtkTreeDragSource *drag_source G_GNUC_UNUSED,
               CtkTreePath       *path G_GNUC_UNUSED)
{
  return TRUE;
}

static gboolean
drag_data_delete (CtkTreeDragSource *drag_source,
                  CtkTreePath       *path)
{
  CtkTreeIter iter;

  if (ctk_tree_model_get_iter (CTK_TREE_MODEL (drag_source), &iter, path))
    return ctk_list_store_remove (CTK_LIST_STORE (drag_source), &iter);
  return FALSE;
}

static gboolean
drag_data_get (CtkTreeDragSource *drag_source,
               CtkTreePath       *path,
               CtkSelectionData  *selection)
{
  CtkTreeIter iter;
  gchar *text;

  if (!ctk_tree_model_get_iter (CTK_TREE_MODEL (drag_source), &iter, path))
    return FALSE;

  ctk_tree_model_get (CTK_TREE_MODEL (drag_source), &iter,
                      ICON_STORE (drag_source)->text_column, &text,
                      -1);

  ctk_selection_data_set_text (selection, text, -1);

  g_free (text);

  return TRUE;
}


static void
icon_store_drag_source_init (CtkTreeDragSourceIface *iface)
{
  iface->row_draggable = row_draggable;
  iface->drag_data_delete = drag_data_delete;
  iface->drag_data_get = drag_data_get;
}

void
icon_store_set_text_column (IconStore *store, gint text_column)
{
  store->text_column = text_column;
}
