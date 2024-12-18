/* ctktreestore.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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
#include <string.h>
#include <gobject/gvaluecollector.h>
#include "ctktreemodel.h"
#include "ctktreestore.h"
#include "ctktreedatalist.h"
#include "ctktreednd.h"
#include "ctkbuildable.h"
#include "ctkbuilderprivate.h"
#include "ctkdebug.h"
#include "ctkintl.h"


/**
 * SECTION:ctktreestore
 * @Short_description: A tree-like data structure that can be used with the CtkTreeView
 * @Title: CtkTreeStore
 * @See_also: #CtkTreeModel
 *
 * The #CtkTreeStore object is a list model for use with a #CtkTreeView
 * widget.  It implements the #CtkTreeModel interface, and consequentially,
 * can use all of the methods available there.  It also implements the
 * #CtkTreeSortable interface so it can be sorted by the view.  Finally,
 * it also implements the tree
 * [drag and drop][ctk3-CtkTreeView-drag-and-drop]
 * interfaces.
 *
 * # CtkTreeStore as CtkBuildable
 *
 * The CtkTreeStore implementation of the #CtkBuildable interface allows
 * to specify the model columns with a <columns> element that may contain
 * multiple <column> elements, each specifying one model column. The “type”
 * attribute specifies the data type for the column.
 *
 * An example of a UI Definition fragment for a tree store:
 * |[
 * <object class="CtkTreeStore">
 *   <columns>
 *     <column type="gchararray"/>
 *     <column type="gchararray"/>
 *     <column type="gint"/>
 *   </columns>
 * </object>
 * ]|
 */

struct _CtkTreeStorePrivate
{
  gint stamp;
  CtkSortType order;
  gpointer root;
  gpointer last;
  gint n_columns;
  gint sort_column_id;
  GList *sort_list;
  GType *column_headers;
  CtkTreeIterCompareFunc default_sort_func;
  gpointer default_sort_data;
  GDestroyNotify default_sort_destroy;
  guint columns_dirty : 1;
};


#define G_NODE(node) ((GNode *)node)
#define CTK_TREE_STORE_IS_SORTED(tree) (((CtkTreeStore*)(tree))->priv->sort_column_id != CTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID)
#define VALID_ITER(iter, tree_store) ((iter)!= NULL && (iter)->user_data != NULL && ((CtkTreeStore*)(tree_store))->priv->stamp == (iter)->stamp)

static void         ctk_tree_store_tree_model_init (CtkTreeModelIface *iface);
static void         ctk_tree_store_drag_source_init(CtkTreeDragSourceIface *iface);
static void         ctk_tree_store_drag_dest_init  (CtkTreeDragDestIface   *iface);
static void         ctk_tree_store_sortable_init   (CtkTreeSortableIface   *iface);
static void         ctk_tree_store_buildable_init  (CtkBuildableIface      *iface);
static void         ctk_tree_store_finalize        (GObject           *object);
static CtkTreeModelFlags ctk_tree_store_get_flags  (CtkTreeModel      *tree_model);
static gint         ctk_tree_store_get_n_columns   (CtkTreeModel      *tree_model);
static GType        ctk_tree_store_get_column_type (CtkTreeModel      *tree_model,
						    gint               index);
static gboolean     ctk_tree_store_get_iter        (CtkTreeModel      *tree_model,
						    CtkTreeIter       *iter,
						    CtkTreePath       *path);
static CtkTreePath *ctk_tree_store_get_path        (CtkTreeModel      *tree_model,
						    CtkTreeIter       *iter);
static void         ctk_tree_store_get_value       (CtkTreeModel      *tree_model,
						    CtkTreeIter       *iter,
						    gint               column,
						    GValue            *value);
static gboolean     ctk_tree_store_iter_next       (CtkTreeModel      *tree_model,
						    CtkTreeIter       *iter);
static gboolean     ctk_tree_store_iter_previous   (CtkTreeModel      *tree_model,
						    CtkTreeIter       *iter);
static gboolean     ctk_tree_store_iter_children   (CtkTreeModel      *tree_model,
						    CtkTreeIter       *iter,
						    CtkTreeIter       *parent);
static gboolean     ctk_tree_store_iter_has_child  (CtkTreeModel      *tree_model,
						    CtkTreeIter       *iter);
static gint         ctk_tree_store_iter_n_children (CtkTreeModel      *tree_model,
						    CtkTreeIter       *iter);
static gboolean     ctk_tree_store_iter_nth_child  (CtkTreeModel      *tree_model,
						    CtkTreeIter       *iter,
						    CtkTreeIter       *parent,
						    gint               n);
static gboolean     ctk_tree_store_iter_parent     (CtkTreeModel      *tree_model,
						    CtkTreeIter       *iter,
						    CtkTreeIter       *child);


static void ctk_tree_store_set_n_columns   (CtkTreeStore *tree_store,
					    gint          n_columns);
static void ctk_tree_store_set_column_type (CtkTreeStore *tree_store,
					    gint          column,
					    GType         type);

static void ctk_tree_store_increment_stamp (CtkTreeStore  *tree_store);


/* DND interfaces */
static gboolean real_ctk_tree_store_row_draggable   (CtkTreeDragSource *drag_source,
						   CtkTreePath       *path);
static gboolean ctk_tree_store_drag_data_delete   (CtkTreeDragSource *drag_source,
						   CtkTreePath       *path);
static gboolean ctk_tree_store_drag_data_get      (CtkTreeDragSource *drag_source,
						   CtkTreePath       *path,
						   CtkSelectionData  *selection_data);
static gboolean ctk_tree_store_drag_data_received (CtkTreeDragDest   *drag_dest,
						   CtkTreePath       *dest,
						   CtkSelectionData  *selection_data);
static gboolean ctk_tree_store_row_drop_possible  (CtkTreeDragDest   *drag_dest,
						   CtkTreePath       *dest_path,
						   CtkSelectionData  *selection_data);

/* Sortable Interfaces */

static void     ctk_tree_store_sort                    (CtkTreeStore           *tree_store);
static void     ctk_tree_store_sort_iter_changed       (CtkTreeStore           *tree_store,
							CtkTreeIter            *iter,
							gint                    column,
							gboolean                emit_signal);
static gboolean ctk_tree_store_get_sort_column_id      (CtkTreeSortable        *sortable,
							gint                   *sort_column_id,
							CtkSortType            *order);
static void     ctk_tree_store_set_sort_column_id      (CtkTreeSortable        *sortable,
							gint                    sort_column_id,
							CtkSortType             order);
static void     ctk_tree_store_set_sort_func           (CtkTreeSortable        *sortable,
							gint                    sort_column_id,
							CtkTreeIterCompareFunc  func,
							gpointer                data,
							GDestroyNotify          destroy);
static void     ctk_tree_store_set_default_sort_func   (CtkTreeSortable        *sortable,
							CtkTreeIterCompareFunc  func,
							gpointer                data,
							GDestroyNotify          destroy);
static gboolean ctk_tree_store_has_default_sort_func   (CtkTreeSortable        *sortable);


/* buildable */

static gboolean ctk_tree_store_buildable_custom_tag_start (CtkBuildable  *buildable,
							   CtkBuilder    *builder,
							   GObject       *child,
							   const gchar   *tagname,
							   GMarkupParser *parser,
							   gpointer      *data);
static void     ctk_tree_store_buildable_custom_finished (CtkBuildable 	 *buildable,
							  CtkBuilder   	 *builder,
							  GObject      	 *child,
							  const gchar  	 *tagname,
							  gpointer     	  user_data);

static void     ctk_tree_store_move                    (CtkTreeStore           *tree_store,
                                                        CtkTreeIter            *iter,
                                                        CtkTreeIter            *position,
                                                        gboolean                before);


#ifdef G_ENABLE_DEBUG
static void validate_gnode (GNode *node);

static inline void
validate_tree (CtkTreeStore *tree_store)
{
  if (CTK_DEBUG_CHECK (TREE))
    {
      g_assert (G_NODE (tree_store->priv->root)->parent == NULL);
      validate_gnode (G_NODE (tree_store->priv->root));
    }
}
#else
#define validate_tree(store)
#endif

G_DEFINE_TYPE_WITH_CODE (CtkTreeStore, ctk_tree_store, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (CtkTreeStore)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_TREE_MODEL,
						ctk_tree_store_tree_model_init)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_TREE_DRAG_SOURCE,
						ctk_tree_store_drag_source_init)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_TREE_DRAG_DEST,
						ctk_tree_store_drag_dest_init)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_TREE_SORTABLE,
						ctk_tree_store_sortable_init)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
						ctk_tree_store_buildable_init))

static void
ctk_tree_store_class_init (CtkTreeStoreClass *class)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *) class;

  object_class->finalize = ctk_tree_store_finalize;
}

static void
ctk_tree_store_tree_model_init (CtkTreeModelIface *iface)
{
  iface->get_flags = ctk_tree_store_get_flags;
  iface->get_n_columns = ctk_tree_store_get_n_columns;
  iface->get_column_type = ctk_tree_store_get_column_type;
  iface->get_iter = ctk_tree_store_get_iter;
  iface->get_path = ctk_tree_store_get_path;
  iface->get_value = ctk_tree_store_get_value;
  iface->iter_next = ctk_tree_store_iter_next;
  iface->iter_previous = ctk_tree_store_iter_previous;
  iface->iter_children = ctk_tree_store_iter_children;
  iface->iter_has_child = ctk_tree_store_iter_has_child;
  iface->iter_n_children = ctk_tree_store_iter_n_children;
  iface->iter_nth_child = ctk_tree_store_iter_nth_child;
  iface->iter_parent = ctk_tree_store_iter_parent;
}

static void
ctk_tree_store_drag_source_init (CtkTreeDragSourceIface *iface)
{
  iface->row_draggable = real_ctk_tree_store_row_draggable;
  iface->drag_data_delete = ctk_tree_store_drag_data_delete;
  iface->drag_data_get = ctk_tree_store_drag_data_get;
}

static void
ctk_tree_store_drag_dest_init (CtkTreeDragDestIface *iface)
{
  iface->drag_data_received = ctk_tree_store_drag_data_received;
  iface->row_drop_possible = ctk_tree_store_row_drop_possible;
}

static void
ctk_tree_store_sortable_init (CtkTreeSortableIface *iface)
{
  iface->get_sort_column_id = ctk_tree_store_get_sort_column_id;
  iface->set_sort_column_id = ctk_tree_store_set_sort_column_id;
  iface->set_sort_func = ctk_tree_store_set_sort_func;
  iface->set_default_sort_func = ctk_tree_store_set_default_sort_func;
  iface->has_default_sort_func = ctk_tree_store_has_default_sort_func;
}

void
ctk_tree_store_buildable_init (CtkBuildableIface *iface)
{
  iface->custom_tag_start = ctk_tree_store_buildable_custom_tag_start;
  iface->custom_finished = ctk_tree_store_buildable_custom_finished;
}

static void
ctk_tree_store_init (CtkTreeStore *tree_store)
{
  CtkTreeStorePrivate *priv;

  priv = ctk_tree_store_get_instance_private (tree_store);
  tree_store->priv = priv;
  priv->root = g_node_new (NULL);
  /* While the odds are against us getting 0...  */
  do
    {
      priv->stamp = g_random_int ();
    }
  while (priv->stamp == 0);

  priv->sort_list = NULL;
  priv->sort_column_id = CTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID;
  priv->columns_dirty = FALSE;
}

/**
 * ctk_tree_store_new:
 * @n_columns: number of columns in the tree store
 * @...: all #GType types for the columns, from first to last
 *
 * Creates a new tree store as with @n_columns columns each of the types passed
 * in.  Note that only types derived from standard GObject fundamental types
 * are supported.
 *
 * As an example, `ctk_tree_store_new (3, G_TYPE_INT, G_TYPE_STRING,
 * GDK_TYPE_PIXBUF);` will create a new #CtkTreeStore with three columns, of type
 * #gint, #gchararray, and #GdkPixbuf respectively.
 *
 * Returns: a new #CtkTreeStore
 **/
CtkTreeStore *
ctk_tree_store_new (gint n_columns,
			       ...)
{
  CtkTreeStore *retval;
  va_list args;
  gint i;

  g_return_val_if_fail (n_columns > 0, NULL);

  retval = g_object_new (CTK_TYPE_TREE_STORE, NULL);
  ctk_tree_store_set_n_columns (retval, n_columns);

  va_start (args, n_columns);

  for (i = 0; i < n_columns; i++)
    {
      GType type = va_arg (args, GType);
      if (! _ctk_tree_data_list_check_type (type))
	{
	  g_warning ("%s: Invalid type %s", G_STRLOC, g_type_name (type));
	  g_object_unref (retval);
          va_end (args);
	  return NULL;
	}
      ctk_tree_store_set_column_type (retval, i, type);
    }
  va_end (args);

  return retval;
}
/**
 * ctk_tree_store_newv: (rename-to ctk_tree_store_new)
 * @n_columns: number of columns in the tree store
 * @types: (array length=n_columns): an array of #GType types for the columns, from first to last
 *
 * Non vararg creation function.  Used primarily by language bindings.
 *
 * Returns: (transfer full): a new #CtkTreeStore
 **/
CtkTreeStore *
ctk_tree_store_newv (gint   n_columns,
		     GType *types)
{
  CtkTreeStore *retval;
  gint i;

  g_return_val_if_fail (n_columns > 0, NULL);

  retval = g_object_new (CTK_TYPE_TREE_STORE, NULL);
  ctk_tree_store_set_n_columns (retval, n_columns);

   for (i = 0; i < n_columns; i++)
    {
      if (! _ctk_tree_data_list_check_type (types[i]))
	{
	  g_warning ("%s: Invalid type %s", G_STRLOC, g_type_name (types[i]));
	  g_object_unref (retval);
	  return NULL;
	}
      ctk_tree_store_set_column_type (retval, i, types[i]);
    }

  return retval;
}


/**
 * ctk_tree_store_set_column_types:
 * @tree_store: A #CtkTreeStore
 * @n_columns: Number of columns for the tree store
 * @types: (array length=n_columns): An array of #GType types, one for each column
 * 
 * This function is meant primarily for #GObjects that inherit from 
 * #CtkTreeStore, and should only be used when constructing a new 
 * #CtkTreeStore.  It will not function after a row has been added, 
 * or a method on the #CtkTreeModel interface is called.
 **/
void
ctk_tree_store_set_column_types (CtkTreeStore *tree_store,
				 gint          n_columns,
				 GType        *types)
{
  gint i;

  g_return_if_fail (CTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (tree_store->priv->columns_dirty == 0);

  ctk_tree_store_set_n_columns (tree_store, n_columns);
   for (i = 0; i < n_columns; i++)
    {
      if (! _ctk_tree_data_list_check_type (types[i]))
	{
	  g_warning ("%s: Invalid type %s", G_STRLOC, g_type_name (types[i]));
	  continue;
	}
      ctk_tree_store_set_column_type (tree_store, i, types[i]);
    }
}

static void
ctk_tree_store_set_n_columns (CtkTreeStore *tree_store,
			      gint          n_columns)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  int i;

  if (priv->n_columns == n_columns)
    return;

  priv->column_headers = g_renew (GType, priv->column_headers, n_columns);
  for (i = priv->n_columns; i < n_columns; i++)
    priv->column_headers[i] = G_TYPE_INVALID;
  priv->n_columns = n_columns;

  if (priv->sort_list)
    _ctk_tree_data_list_header_free (priv->sort_list);

  priv->sort_list = _ctk_tree_data_list_header_new (n_columns, priv->column_headers);
}

/**
 * ctk_tree_store_set_column_type:
 * @tree_store: a #CtkTreeStore
 * @column: column number
 * @type: type of the data to be stored in @column
 *
 * Supported types include: %G_TYPE_UINT, %G_TYPE_INT, %G_TYPE_UCHAR,
 * %G_TYPE_CHAR, %G_TYPE_BOOLEAN, %G_TYPE_POINTER, %G_TYPE_FLOAT,
 * %G_TYPE_DOUBLE, %G_TYPE_STRING, %G_TYPE_OBJECT, and %G_TYPE_BOXED, along with
 * subclasses of those types such as %GDK_TYPE_PIXBUF.
 *
 **/
static void
ctk_tree_store_set_column_type (CtkTreeStore *tree_store,
				gint          column,
				GType         type)
{
  CtkTreeStorePrivate *priv = tree_store->priv;

  if (!_ctk_tree_data_list_check_type (type))
    {
      g_warning ("%s: Invalid type %s", G_STRLOC, g_type_name (type));
      return;
    }
  priv->column_headers[column] = type;
}

static gboolean
node_free (GNode *node, gpointer data)
{
  if (node->data)
    _ctk_tree_data_list_free (node->data, (GType*)data);
  node->data = NULL;

  return FALSE;
}

static void
ctk_tree_store_finalize (GObject *object)
{
  CtkTreeStore *tree_store = CTK_TREE_STORE (object);
  CtkTreeStorePrivate *priv = tree_store->priv;

  g_node_traverse (priv->root, G_POST_ORDER, G_TRAVERSE_ALL, -1,
		   node_free, priv->column_headers);
  g_node_destroy (priv->root);
  _ctk_tree_data_list_header_free (priv->sort_list);
  g_free (priv->column_headers);

  if (priv->default_sort_destroy)
    {
      GDestroyNotify d = priv->default_sort_destroy;

      priv->default_sort_destroy = NULL;
      d (priv->default_sort_data);
      priv->default_sort_data = NULL;
    }

  /* must chain up */
  G_OBJECT_CLASS (ctk_tree_store_parent_class)->finalize (object);
}

/* fulfill the CtkTreeModel requirements */
/* NOTE: CtkTreeStore::root is a GNode, that acts as the parent node.  However,
 * it is not visible to the tree or to the user., and the path “0” refers to the
 * first child of CtkTreeStore::root.
 */


static CtkTreeModelFlags
ctk_tree_store_get_flags (CtkTreeModel *tree_model G_GNUC_UNUSED)
{
  return CTK_TREE_MODEL_ITERS_PERSIST;
}

static gint
ctk_tree_store_get_n_columns (CtkTreeModel *tree_model)
{
  CtkTreeStore *tree_store = (CtkTreeStore *) tree_model;
  CtkTreeStorePrivate *priv = tree_store->priv;

  priv->columns_dirty = TRUE;

  return priv->n_columns;
}

static GType
ctk_tree_store_get_column_type (CtkTreeModel *tree_model,
				gint          index)
{
  CtkTreeStore *tree_store = (CtkTreeStore *) tree_model;
  CtkTreeStorePrivate *priv = tree_store->priv;

  g_return_val_if_fail (index < priv->n_columns, G_TYPE_INVALID);

  priv->columns_dirty = TRUE;

  return priv->column_headers[index];
}

static gboolean
ctk_tree_store_get_iter (CtkTreeModel *tree_model,
			 CtkTreeIter  *iter,
			 CtkTreePath  *path)
{
  CtkTreeStore *tree_store = (CtkTreeStore *) tree_model;
  CtkTreeStorePrivate *priv = tree_store->priv;
  CtkTreeIter parent;
  gint *indices;
  gint depth, i;

  priv->columns_dirty = TRUE;

  indices = ctk_tree_path_get_indices (path);
  depth = ctk_tree_path_get_depth (path);

  g_return_val_if_fail (depth > 0, FALSE);

  parent.stamp = priv->stamp;
  parent.user_data = priv->root;

  if (!ctk_tree_store_iter_nth_child (tree_model, iter, &parent, indices[0]))
    {
      iter->stamp = 0;
      return FALSE;
    }

  for (i = 1; i < depth; i++)
    {
      parent = *iter;
      if (!ctk_tree_store_iter_nth_child (tree_model, iter, &parent, indices[i]))
        {
          iter->stamp = 0;
          return FALSE;
        }
    }

  return TRUE;
}

static CtkTreePath *
ctk_tree_store_get_path (CtkTreeModel *tree_model,
			 CtkTreeIter  *iter)
{
  CtkTreeStore *tree_store = (CtkTreeStore *) tree_model;
  CtkTreeStorePrivate *priv = tree_store->priv;
  CtkTreePath *retval;
  GNode *tmp_node;
  gint i = 0;

  g_return_val_if_fail (iter->user_data != NULL, NULL);
  g_return_val_if_fail (iter->stamp == priv->stamp, NULL);

  validate_tree (tree_store);

  if (G_NODE (iter->user_data)->parent == NULL &&
      G_NODE (iter->user_data) == priv->root)
    return ctk_tree_path_new ();
  g_assert (G_NODE (iter->user_data)->parent != NULL);

  if (G_NODE (iter->user_data)->parent == G_NODE (priv->root))
    {
      retval = ctk_tree_path_new ();
      tmp_node = G_NODE (priv->root)->children;
    }
  else
    {
      CtkTreeIter tmp_iter = *iter;

      tmp_iter.user_data = G_NODE (iter->user_data)->parent;

      retval = ctk_tree_store_get_path (tree_model, &tmp_iter);
      tmp_node = G_NODE (iter->user_data)->parent->children;
    }

  if (retval == NULL)
    return NULL;

  if (tmp_node == NULL)
    {
      ctk_tree_path_free (retval);
      return NULL;
    }

  for (; tmp_node; tmp_node = tmp_node->next)
    {
      if (tmp_node == G_NODE (iter->user_data))
	break;
      i++;
    }

  if (tmp_node == NULL)
    {
      /* We couldn't find node, meaning it's prolly not ours */
      /* Perhaps I should do a g_return_if_fail here. */
      ctk_tree_path_free (retval);
      return NULL;
    }

  ctk_tree_path_append_index (retval, i);

  return retval;
}


static void
ctk_tree_store_get_value (CtkTreeModel *tree_model,
			  CtkTreeIter  *iter,
			  gint          column,
			  GValue       *value)
{
  CtkTreeStore *tree_store = (CtkTreeStore *) tree_model;
  CtkTreeStorePrivate *priv = tree_store->priv;
  CtkTreeDataList *list;
  gint tmp_column = column;

  g_return_if_fail (column < priv->n_columns);
  g_return_if_fail (VALID_ITER (iter, tree_store));

  list = G_NODE (iter->user_data)->data;

  while (tmp_column-- > 0 && list)
    list = list->next;

  if (list)
    {
      _ctk_tree_data_list_node_to_value (list,
					 priv->column_headers[column],
					 value);
    }
  else
    {
      /* We want to return an initialized but empty (default) value */
      g_value_init (value, priv->column_headers[column]);
    }
}

static gboolean
ctk_tree_store_iter_next (CtkTreeModel  *tree_model,
			  CtkTreeIter   *iter)
{
  g_return_val_if_fail (iter->user_data != NULL, FALSE);
  g_return_val_if_fail (iter->stamp == CTK_TREE_STORE (tree_model)->priv->stamp, FALSE);

  if (G_NODE (iter->user_data)->next == NULL)
    {
      iter->stamp = 0;
      return FALSE;
    }

  iter->user_data = G_NODE (iter->user_data)->next;

  return TRUE;
}

static gboolean
ctk_tree_store_iter_previous (CtkTreeModel *tree_model,
                              CtkTreeIter  *iter)
{
  g_return_val_if_fail (iter->user_data != NULL, FALSE);
  g_return_val_if_fail (iter->stamp == CTK_TREE_STORE (tree_model)->priv->stamp, FALSE);

  if (G_NODE (iter->user_data)->prev == NULL)
    {
      iter->stamp = 0;
      return FALSE;
    }

  iter->user_data = G_NODE (iter->user_data)->prev;

  return TRUE;
}

static gboolean
ctk_tree_store_iter_children (CtkTreeModel *tree_model,
			      CtkTreeIter  *iter,
			      CtkTreeIter  *parent)
{
  CtkTreeStore *tree_store = (CtkTreeStore *) tree_model;
  CtkTreeStorePrivate *priv = tree_store->priv;
  GNode *children;

  if (parent)
    g_return_val_if_fail (VALID_ITER (parent, tree_store), FALSE);

  if (parent)
    children = G_NODE (parent->user_data)->children;
  else
    children = G_NODE (priv->root)->children;

  if (children)
    {
      iter->stamp = priv->stamp;
      iter->user_data = children;
      return TRUE;
    }
  else
    {
      iter->stamp = 0;
      return FALSE;
    }
}

static gboolean
ctk_tree_store_iter_has_child (CtkTreeModel *tree_model,
			       CtkTreeIter  *iter)
{
  g_return_val_if_fail (iter->user_data != NULL, FALSE);
  g_return_val_if_fail (VALID_ITER (iter, tree_model), FALSE);

  return G_NODE (iter->user_data)->children != NULL;
}

static gint
ctk_tree_store_iter_n_children (CtkTreeModel *tree_model,
				CtkTreeIter  *iter)
{
  GNode *node;
  gint i = 0;

  g_return_val_if_fail (iter == NULL || iter->user_data != NULL, 0);

  if (iter == NULL)
    node = G_NODE (CTK_TREE_STORE (tree_model)->priv->root)->children;
  else
    node = G_NODE (iter->user_data)->children;

  while (node)
    {
      i++;
      node = node->next;
    }

  return i;
}

static gboolean
ctk_tree_store_iter_nth_child (CtkTreeModel *tree_model,
			       CtkTreeIter  *iter,
			       CtkTreeIter  *parent,
			       gint          n)
{
  CtkTreeStore *tree_store = (CtkTreeStore *) tree_model;
  CtkTreeStorePrivate *priv = tree_store->priv;
  GNode *parent_node;
  GNode *child;

  g_return_val_if_fail (parent == NULL || parent->user_data != NULL, FALSE);

  if (parent == NULL)
    parent_node = priv->root;
  else
    parent_node = parent->user_data;

  child = g_node_nth_child (parent_node, n);

  if (child)
    {
      iter->user_data = child;
      iter->stamp = priv->stamp;
      return TRUE;
    }
  else
    {
      iter->stamp = 0;
      return FALSE;
    }
}

static gboolean
ctk_tree_store_iter_parent (CtkTreeModel *tree_model,
			    CtkTreeIter  *iter,
			    CtkTreeIter  *child)
{
  CtkTreeStore *tree_store = (CtkTreeStore *) tree_model;
  CtkTreeStorePrivate *priv = tree_store->priv;
  GNode *parent;

  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (VALID_ITER (child, tree_store), FALSE);

  parent = G_NODE (child->user_data)->parent;

  g_assert (parent != NULL);

  if (parent != priv->root)
    {
      iter->user_data = parent;
      iter->stamp = priv->stamp;
      return TRUE;
    }
  else
    {
      iter->stamp = 0;
      return FALSE;
    }
}


/* Does not emit a signal */
static gboolean
ctk_tree_store_real_set_value (CtkTreeStore *tree_store,
			       CtkTreeIter  *iter,
			       gint          column,
			       GValue       *value,
			       gboolean      sort)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  CtkTreeDataList *list;
  CtkTreeDataList *prev;
  gint old_column = column;
  GValue real_value = G_VALUE_INIT;
  gboolean converted = FALSE;
  gboolean retval = FALSE;

  if (! g_type_is_a (G_VALUE_TYPE (value), priv->column_headers[column]))
    {
      if (! (g_value_type_transformable (G_VALUE_TYPE (value), priv->column_headers[column])))
	{
	  g_warning ("%s: Unable to convert from %s to %s",
		     G_STRLOC,
		     g_type_name (G_VALUE_TYPE (value)),
		     g_type_name (priv->column_headers[column]));
	  return retval;
	}

      g_value_init (&real_value, priv->column_headers[column]);
      if (!g_value_transform (value, &real_value))
	{
	  g_warning ("%s: Unable to make conversion from %s to %s",
		     G_STRLOC,
		     g_type_name (G_VALUE_TYPE (value)),
		     g_type_name (priv->column_headers[column]));
	  g_value_unset (&real_value);
	  return retval;
	}
      converted = TRUE;
    }

  prev = list = G_NODE (iter->user_data)->data;

  while (list != NULL)
    {
      if (column == 0)
	{
	  if (converted)
	    _ctk_tree_data_list_value_to_node (list, &real_value);
	  else
	    _ctk_tree_data_list_value_to_node (list, value);
	  retval = TRUE;
	  if (converted)
	    g_value_unset (&real_value);
          if (sort && CTK_TREE_STORE_IS_SORTED (tree_store))
            ctk_tree_store_sort_iter_changed (tree_store, iter, old_column, TRUE);
	  return retval;
	}

      column--;
      prev = list;
      list = list->next;
    }

  if (G_NODE (iter->user_data)->data == NULL)
    {
      G_NODE (iter->user_data)->data = list = _ctk_tree_data_list_alloc ();
      list->next = NULL;
    }
  else
    {
      list = prev->next = _ctk_tree_data_list_alloc ();
      list->next = NULL;
    }

  while (column != 0)
    {
      list->next = _ctk_tree_data_list_alloc ();
      list = list->next;
      list->next = NULL;
      column --;
    }

  if (converted)
    _ctk_tree_data_list_value_to_node (list, &real_value);
  else
    _ctk_tree_data_list_value_to_node (list, value);
  
  retval = TRUE;
  if (converted)
    g_value_unset (&real_value);

  if (sort && CTK_TREE_STORE_IS_SORTED (tree_store))
    ctk_tree_store_sort_iter_changed (tree_store, iter, old_column, TRUE);

  return retval;
}

/**
 * ctk_tree_store_set_value:
 * @tree_store: a #CtkTreeStore
 * @iter: A valid #CtkTreeIter for the row being modified
 * @column: column number to modify
 * @value: new value for the cell
 *
 * Sets the data in the cell specified by @iter and @column.
 * The type of @value must be convertible to the type of the
 * column.
 *
 **/
void
ctk_tree_store_set_value (CtkTreeStore *tree_store,
			  CtkTreeIter  *iter,
			  gint          column,
			  GValue       *value)
{
  g_return_if_fail (CTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (VALID_ITER (iter, tree_store));
  g_return_if_fail (column >= 0 && column < tree_store->priv->n_columns);
  g_return_if_fail (G_IS_VALUE (value));

  if (ctk_tree_store_real_set_value (tree_store, iter, column, value, TRUE))
    {
      CtkTreePath *path;

      path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), iter);
      ctk_tree_model_row_changed (CTK_TREE_MODEL (tree_store), path, iter);
      ctk_tree_path_free (path);
    }
}

static CtkTreeIterCompareFunc
ctk_tree_store_get_compare_func (CtkTreeStore *tree_store)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  CtkTreeIterCompareFunc func = NULL;

  if (CTK_TREE_STORE_IS_SORTED (tree_store))
    {
      if (priv->sort_column_id != -1)
	{
	  CtkTreeDataSortHeader *header;
	  header = _ctk_tree_data_list_get_header (priv->sort_list,
						   priv->sort_column_id);
	  g_return_val_if_fail (header != NULL, NULL);
	  g_return_val_if_fail (header->func != NULL, NULL);
	  func = header->func;
	}
      else
	{
	  func = priv->default_sort_func;
	}
    }

  return func;
}

static void
ctk_tree_store_set_vector_internal (CtkTreeStore *tree_store,
				    CtkTreeIter  *iter,
				    gboolean     *emit_signal,
				    gboolean     *maybe_need_sort,
				    gint         *columns,
				    GValue       *values,
				    gint          n_values)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  gint i;
  CtkTreeIterCompareFunc func = NULL;

  func = ctk_tree_store_get_compare_func (tree_store);
  if (func != _ctk_tree_data_list_compare_func)
    *maybe_need_sort = TRUE;

  for (i = 0; i < n_values; i++)
    {
      *emit_signal = ctk_tree_store_real_set_value (tree_store, iter,
						    columns[i], &values[i],
						    FALSE) || *emit_signal;

      if (func == _ctk_tree_data_list_compare_func &&
	  columns[i] == priv->sort_column_id)
	*maybe_need_sort = TRUE;
    }
}

static void
ctk_tree_store_set_valist_internal (CtkTreeStore *tree_store,
                                    CtkTreeIter  *iter,
                                    gboolean     *emit_signal,
                                    gboolean     *maybe_need_sort,
                                    va_list       var_args)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  gint column;
  CtkTreeIterCompareFunc func = NULL;

  column = va_arg (var_args, gint);

  func = ctk_tree_store_get_compare_func (tree_store);
  if (func != _ctk_tree_data_list_compare_func)
    *maybe_need_sort = TRUE;

  while (column != -1)
    {
      GValue value = G_VALUE_INIT;
      gchar *error = NULL;

      if (column < 0 || column >= priv->n_columns)
	{
	  g_warning ("%s: Invalid column number %d added to iter (remember to end your list of columns with a -1)", G_STRLOC, column);
	  break;
	}

      G_VALUE_COLLECT_INIT (&value, priv->column_headers[column],
                            var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);

 	  /* we purposely leak the value here, it might not be
	   * in a sane state if an error condition occoured
	   */
	  break;
	}

      *emit_signal = ctk_tree_store_real_set_value (tree_store,
						    iter,
						    column,
						    &value,
						    FALSE) || *emit_signal;

      if (func == _ctk_tree_data_list_compare_func &&
	  column == priv->sort_column_id)
	*maybe_need_sort = TRUE;

      g_value_unset (&value);

      column = va_arg (var_args, gint);
    }
}

/**
 * ctk_tree_store_set_valuesv: (rename-to ctk_tree_store_set)
 * @tree_store: A #CtkTreeStore
 * @iter: A valid #CtkTreeIter for the row being modified
 * @columns: (array length=n_values): an array of column numbers
 * @values: (array length=n_values): an array of GValues
 * @n_values: the length of the @columns and @values arrays
 *
 * A variant of ctk_tree_store_set_valist() which takes
 * the columns and values as two arrays, instead of varargs.  This
 * function is mainly intended for language bindings or in case
 * the number of columns to change is not known until run-time.
 *
 * Since: 2.12
 **/
void
ctk_tree_store_set_valuesv (CtkTreeStore *tree_store,
			    CtkTreeIter  *iter,
			    gint         *columns,
			    GValue       *values,
			    gint          n_values)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  gboolean emit_signal = FALSE;
  gboolean maybe_need_sort = FALSE;

  g_return_if_fail (CTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (VALID_ITER (iter, tree_store));

  ctk_tree_store_set_vector_internal (tree_store, iter,
				      &emit_signal,
				      &maybe_need_sort,
				      columns, values, n_values);

  if (maybe_need_sort && CTK_TREE_STORE_IS_SORTED (tree_store))
    ctk_tree_store_sort_iter_changed (tree_store, iter, priv->sort_column_id, TRUE);

  if (emit_signal)
    {
      CtkTreePath *path;

      path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), iter);
      ctk_tree_model_row_changed (CTK_TREE_MODEL (tree_store), path, iter);
      ctk_tree_path_free (path);
    }
}

/**
 * ctk_tree_store_set_valist:
 * @tree_store: A #CtkTreeStore
 * @iter: A valid #CtkTreeIter for the row being modified
 * @var_args: va_list of column/value pairs
 *
 * See ctk_tree_store_set(); this version takes a va_list for
 * use by language bindings.
 *
 **/
void
ctk_tree_store_set_valist (CtkTreeStore *tree_store,
                           CtkTreeIter  *iter,
                           va_list       var_args)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  gboolean emit_signal = FALSE;
  gboolean maybe_need_sort = FALSE;

  g_return_if_fail (CTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (VALID_ITER (iter, tree_store));

  ctk_tree_store_set_valist_internal (tree_store, iter,
				      &emit_signal,
				      &maybe_need_sort,
				      var_args);

  if (maybe_need_sort && CTK_TREE_STORE_IS_SORTED (tree_store))
    ctk_tree_store_sort_iter_changed (tree_store, iter, priv->sort_column_id, TRUE);

  if (emit_signal)
    {
      CtkTreePath *path;

      path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), iter);
      ctk_tree_model_row_changed (CTK_TREE_MODEL (tree_store), path, iter);
      ctk_tree_path_free (path);
    }
}

/**
 * ctk_tree_store_set:
 * @tree_store: A #CtkTreeStore
 * @iter: A valid #CtkTreeIter for the row being modified
 * @...: pairs of column number and value, terminated with -1
 *
 * Sets the value of one or more cells in the row referenced by @iter.
 * The variable argument list should contain integer column numbers,
 * each column number followed by the value to be set.
 * The list is terminated by a -1. For example, to set column 0 with type
 * %G_TYPE_STRING to “Foo”, you would write
 * `ctk_tree_store_set (store, iter, 0, "Foo", -1)`.
 *
 * The value will be referenced by the store if it is a %G_TYPE_OBJECT, and it
 * will be copied if it is a %G_TYPE_STRING or %G_TYPE_BOXED.
 **/
void
ctk_tree_store_set (CtkTreeStore *tree_store,
		    CtkTreeIter  *iter,
		    ...)
{
  va_list var_args;

  va_start (var_args, iter);
  ctk_tree_store_set_valist (tree_store, iter, var_args);
  va_end (var_args);
}

/**
 * ctk_tree_store_remove:
 * @tree_store: A #CtkTreeStore
 * @iter: A valid #CtkTreeIter
 * 
 * Removes @iter from @tree_store.  After being removed, @iter is set to the
 * next valid row at that level, or invalidated if it previously pointed to the
 * last one.
 *
 * Returns: %TRUE if @iter is still valid, %FALSE if not.
 **/
gboolean
ctk_tree_store_remove (CtkTreeStore *tree_store,
		       CtkTreeIter  *iter)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  CtkTreePath *path;
  CtkTreeIter new_iter = {0,};
  GNode *parent;
  GNode *next_node;

  g_return_val_if_fail (CTK_IS_TREE_STORE (tree_store), FALSE);
  g_return_val_if_fail (VALID_ITER (iter, tree_store), FALSE);

  parent = G_NODE (iter->user_data)->parent;

  g_assert (parent != NULL);
  next_node = G_NODE (iter->user_data)->next;

  if (G_NODE (iter->user_data)->data)
    g_node_traverse (G_NODE (iter->user_data), G_POST_ORDER, G_TRAVERSE_ALL,
		     -1, node_free, priv->column_headers);

  path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), iter);
  g_node_destroy (G_NODE (iter->user_data));

  ctk_tree_model_row_deleted (CTK_TREE_MODEL (tree_store), path);

  if (parent != G_NODE (priv->root))
    {
      /* child_toggled */
      if (parent->children == NULL)
	{
	  ctk_tree_path_up (path);

	  new_iter.stamp = priv->stamp;
	  new_iter.user_data = parent;
	  ctk_tree_model_row_has_child_toggled (CTK_TREE_MODEL (tree_store), path, &new_iter);
	}
    }
  ctk_tree_path_free (path);

  /* revalidate iter */
  if (next_node != NULL)
    {
      iter->stamp = priv->stamp;
      iter->user_data = next_node;
      return TRUE;
    }
  else
    {
      iter->stamp = 0;
      iter->user_data = NULL;
    }

  return FALSE;
}

/**
 * ctk_tree_store_insert:
 * @tree_store: A #CtkTreeStore
 * @iter: (out): An unset #CtkTreeIter to set to the new row
 * @parent: (allow-none): A valid #CtkTreeIter, or %NULL
 * @position: position to insert the new row, or -1 for last
 *
 * Creates a new row at @position.  If parent is non-%NULL, then the row will be
 * made a child of @parent.  Otherwise, the row will be created at the toplevel.
 * If @position is -1 or is larger than the number of rows at that level, then
 * the new row will be inserted to the end of the list.  @iter will be changed
 * to point to this new row.  The row will be empty after this function is
 * called.  To fill in values, you need to call ctk_tree_store_set() or
 * ctk_tree_store_set_value().
 *
 **/
void
ctk_tree_store_insert (CtkTreeStore *tree_store,
		       CtkTreeIter  *iter,
		       CtkTreeIter  *parent,
		       gint          position)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  CtkTreePath *path;
  GNode *parent_node;
  GNode *new_node;

  g_return_if_fail (CTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (iter != NULL);
  if (parent)
    g_return_if_fail (VALID_ITER (parent, tree_store));

  if (parent)
    parent_node = parent->user_data;
  else
    parent_node = priv->root;

  priv->columns_dirty = TRUE;

  new_node = g_node_new (NULL);

  iter->stamp = priv->stamp;
  iter->user_data = new_node;
  g_node_insert (parent_node, position, new_node);

  path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), iter);
  ctk_tree_model_row_inserted (CTK_TREE_MODEL (tree_store), path, iter);

  if (parent_node != priv->root)
    {
      if (new_node->prev == NULL && new_node->next == NULL)
        {
          ctk_tree_path_up (path);
          ctk_tree_model_row_has_child_toggled (CTK_TREE_MODEL (tree_store), path, parent);
        }
    }

  ctk_tree_path_free (path);

  validate_tree ((CtkTreeStore*)tree_store);
}

/**
 * ctk_tree_store_insert_before:
 * @tree_store: A #CtkTreeStore
 * @iter: (out): An unset #CtkTreeIter to set to the new row
 * @parent: (allow-none): A valid #CtkTreeIter, or %NULL
 * @sibling: (allow-none): A valid #CtkTreeIter, or %NULL
 *
 * Inserts a new row before @sibling.  If @sibling is %NULL, then the row will
 * be appended to @parent ’s children.  If @parent and @sibling are %NULL, then
 * the row will be appended to the toplevel.  If both @sibling and @parent are
 * set, then @parent must be the parent of @sibling.  When @sibling is set,
 * @parent is optional.
 *
 * @iter will be changed to point to this new row.  The row will be empty after
 * this function is called.  To fill in values, you need to call
 * ctk_tree_store_set() or ctk_tree_store_set_value().
 *
 **/
void
ctk_tree_store_insert_before (CtkTreeStore *tree_store,
			      CtkTreeIter  *iter,
			      CtkTreeIter  *parent,
			      CtkTreeIter  *sibling)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  CtkTreePath *path;
  GNode *parent_node = NULL;
  GNode *new_node;

  g_return_if_fail (CTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (iter != NULL);
  if (parent != NULL)
    g_return_if_fail (VALID_ITER (parent, tree_store));
  if (sibling != NULL)
    g_return_if_fail (VALID_ITER (sibling, tree_store));

  if (parent == NULL && sibling == NULL)
    parent_node = priv->root;
  else if (parent == NULL)
    parent_node = G_NODE (sibling->user_data)->parent;
  else if (sibling == NULL)
    parent_node = G_NODE (parent->user_data);
  else
    {
      g_return_if_fail (G_NODE (sibling->user_data)->parent == G_NODE (parent->user_data));
      parent_node = G_NODE (parent->user_data);
    }

  priv->columns_dirty = TRUE;

  new_node = g_node_new (NULL);

  g_node_insert_before (parent_node,
			sibling ? G_NODE (sibling->user_data) : NULL,
                        new_node);

  iter->stamp = priv->stamp;
  iter->user_data = new_node;

  path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), iter);
  ctk_tree_model_row_inserted (CTK_TREE_MODEL (tree_store), path, iter);

  if (parent_node != priv->root)
    {
      if (new_node->prev == NULL && new_node->next == NULL)
        {
          CtkTreeIter parent_iter;

          parent_iter.stamp = priv->stamp;
          parent_iter.user_data = parent_node;

          ctk_tree_path_up (path);
          ctk_tree_model_row_has_child_toggled (CTK_TREE_MODEL (tree_store), path, &parent_iter);
        }
    }

  ctk_tree_path_free (path);

  validate_tree (tree_store);
}

/**
 * ctk_tree_store_insert_after:
 * @tree_store: A #CtkTreeStore
 * @iter: (out): An unset #CtkTreeIter to set to the new row
 * @parent: (allow-none): A valid #CtkTreeIter, or %NULL
 * @sibling: (allow-none): A valid #CtkTreeIter, or %NULL
 *
 * Inserts a new row after @sibling.  If @sibling is %NULL, then the row will be
 * prepended to @parent ’s children.  If @parent and @sibling are %NULL, then
 * the row will be prepended to the toplevel.  If both @sibling and @parent are
 * set, then @parent must be the parent of @sibling.  When @sibling is set,
 * @parent is optional.
 *
 * @iter will be changed to point to this new row.  The row will be empty after
 * this function is called.  To fill in values, you need to call
 * ctk_tree_store_set() or ctk_tree_store_set_value().
 *
 **/
void
ctk_tree_store_insert_after (CtkTreeStore *tree_store,
			     CtkTreeIter  *iter,
			     CtkTreeIter  *parent,
			     CtkTreeIter  *sibling)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  CtkTreePath *path;
  GNode *parent_node;
  GNode *new_node;

  g_return_if_fail (CTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (iter != NULL);
  if (parent != NULL)
    g_return_if_fail (VALID_ITER (parent, tree_store));
  if (sibling != NULL)
    g_return_if_fail (VALID_ITER (sibling, tree_store));

  if (parent == NULL && sibling == NULL)
    parent_node = priv->root;
  else if (parent == NULL)
    parent_node = G_NODE (sibling->user_data)->parent;
  else if (sibling == NULL)
    parent_node = G_NODE (parent->user_data);
  else
    {
      g_return_if_fail (G_NODE (sibling->user_data)->parent ==
                        G_NODE (parent->user_data));
      parent_node = G_NODE (parent->user_data);
    }

  priv->columns_dirty = TRUE;

  new_node = g_node_new (NULL);

  g_node_insert_after (parent_node,
		       sibling ? G_NODE (sibling->user_data) : NULL,
                       new_node);

  iter->stamp = priv->stamp;
  iter->user_data = new_node;

  path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), iter);
  ctk_tree_model_row_inserted (CTK_TREE_MODEL (tree_store), path, iter);

  if (parent_node != priv->root)
    {
      if (new_node->prev == NULL && new_node->next == NULL)
        {
          CtkTreeIter parent_iter;

          parent_iter.stamp = priv->stamp;
          parent_iter.user_data = parent_node;

          ctk_tree_path_up (path);
          ctk_tree_model_row_has_child_toggled (CTK_TREE_MODEL (tree_store), path, &parent_iter);
        }
    }

  ctk_tree_path_free (path);

  validate_tree (tree_store);
}

/**
 * ctk_tree_store_insert_with_values:
 * @tree_store: A #CtkTreeStore
 * @iter: (out) (allow-none): An unset #CtkTreeIter to set the new row, or %NULL.
 * @parent: (allow-none): A valid #CtkTreeIter, or %NULL
 * @position: position to insert the new row, or -1 to append after existing rows
 * @...: pairs of column number and value, terminated with -1
 *
 * Creates a new row at @position. @iter will be changed to point to this
 * new row. If @position is -1, or larger than the number of rows on the list, then
 * the new row will be appended to the list. The row will be filled with
 * the values given to this function.
 *
 * Calling
 * `ctk_tree_store_insert_with_values (tree_store, iter, position, ...)`
 * has the same effect as calling
 * |[<!-- language="C" -->
 * ctk_tree_store_insert (tree_store, iter, position);
 * ctk_tree_store_set (tree_store, iter, ...);
 * ]|
 * with the different that the former will only emit a row_inserted signal,
 * while the latter will emit row_inserted, row_changed and if the tree store
 * is sorted, rows_reordered.  Since emitting the rows_reordered signal
 * repeatedly can affect the performance of the program,
 * ctk_tree_store_insert_with_values() should generally be preferred when
 * inserting rows in a sorted tree store.
 *
 * Since: 2.10
 */
void
ctk_tree_store_insert_with_values (CtkTreeStore *tree_store,
				   CtkTreeIter  *iter,
				   CtkTreeIter  *parent,
				   gint          position,
				   ...)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  CtkTreePath *path;
  GNode *parent_node;
  GNode *new_node;
  CtkTreeIter tmp_iter;
  va_list var_args;
  gboolean changed = FALSE;
  gboolean maybe_need_sort = FALSE;

  g_return_if_fail (CTK_IS_TREE_STORE (tree_store));

  if (!iter)
    iter = &tmp_iter;

  if (parent)
    g_return_if_fail (VALID_ITER (parent, tree_store));

  if (parent)
    parent_node = parent->user_data;
  else
    parent_node = priv->root;

  priv->columns_dirty = TRUE;

  new_node = g_node_new (NULL);

  iter->stamp = priv->stamp;
  iter->user_data = new_node;
  g_node_insert (parent_node, position, new_node);

  va_start (var_args, position);
  ctk_tree_store_set_valist_internal (tree_store, iter,
				      &changed, &maybe_need_sort,
				      var_args);
  va_end (var_args);

  if (maybe_need_sort && CTK_TREE_STORE_IS_SORTED (tree_store))
    ctk_tree_store_sort_iter_changed (tree_store, iter, priv->sort_column_id, FALSE);

  path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), iter);
  ctk_tree_model_row_inserted (CTK_TREE_MODEL (tree_store), path, iter);

  if (parent_node != priv->root)
    {
      if (new_node->prev == NULL && new_node->next == NULL)
        {
	  ctk_tree_path_up (path);
	  ctk_tree_model_row_has_child_toggled (CTK_TREE_MODEL (tree_store), path, parent);
	}
    }

  ctk_tree_path_free (path);

  validate_tree ((CtkTreeStore *)tree_store);
}

/**
 * ctk_tree_store_insert_with_valuesv: (rename-to ctk_tree_store_insert_with_values)
 * @tree_store: A #CtkTreeStore
 * @iter: (out) (allow-none): An unset #CtkTreeIter to set the new row, or %NULL.
 * @parent: (allow-none): A valid #CtkTreeIter, or %NULL
 * @position: position to insert the new row, or -1 for last
 * @columns: (array length=n_values): an array of column numbers
 * @values: (array length=n_values): an array of GValues
 * @n_values: the length of the @columns and @values arrays
 *
 * A variant of ctk_tree_store_insert_with_values() which takes
 * the columns and values as two arrays, instead of varargs.  This
 * function is mainly intended for language bindings.
 *
 * Since: 2.10
 */
void
ctk_tree_store_insert_with_valuesv (CtkTreeStore *tree_store,
				    CtkTreeIter  *iter,
				    CtkTreeIter  *parent,
				    gint          position,
				    gint         *columns,
				    GValue       *values,
				    gint          n_values)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  CtkTreePath *path;
  GNode *parent_node;
  GNode *new_node;
  CtkTreeIter tmp_iter;
  gboolean changed = FALSE;
  gboolean maybe_need_sort = FALSE;

  g_return_if_fail (CTK_IS_TREE_STORE (tree_store));

  if (!iter)
    iter = &tmp_iter;

  if (parent)
    g_return_if_fail (VALID_ITER (parent, tree_store));

  if (parent)
    parent_node = parent->user_data;
  else
    parent_node = priv->root;

  priv->columns_dirty = TRUE;

  new_node = g_node_new (NULL);

  iter->stamp = priv->stamp;
  iter->user_data = new_node;
  g_node_insert (parent_node, position, new_node);

  ctk_tree_store_set_vector_internal (tree_store, iter,
				      &changed, &maybe_need_sort,
				      columns, values, n_values);

  if (maybe_need_sort && CTK_TREE_STORE_IS_SORTED (tree_store))
    ctk_tree_store_sort_iter_changed (tree_store, iter, priv->sort_column_id, FALSE);

  path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), iter);
  ctk_tree_model_row_inserted (CTK_TREE_MODEL (tree_store), path, iter);

  if (parent_node != priv->root)
    {
      if (new_node->prev == NULL && new_node->next == NULL)
        {
	  ctk_tree_path_up (path);
	  ctk_tree_model_row_has_child_toggled (CTK_TREE_MODEL (tree_store), path, parent);
	}
    }

  ctk_tree_path_free (path);

  validate_tree ((CtkTreeStore *)tree_store);
}

/**
 * ctk_tree_store_prepend:
 * @tree_store: A #CtkTreeStore
 * @iter: (out): An unset #CtkTreeIter to set to the prepended row
 * @parent: (allow-none): A valid #CtkTreeIter, or %NULL
 * 
 * Prepends a new row to @tree_store.  If @parent is non-%NULL, then it will prepend
 * the new row before the first child of @parent, otherwise it will prepend a row
 * to the top level.  @iter will be changed to point to this new row.  The row
 * will be empty after this function is called.  To fill in values, you need to
 * call ctk_tree_store_set() or ctk_tree_store_set_value().
 **/
void
ctk_tree_store_prepend (CtkTreeStore *tree_store,
			CtkTreeIter  *iter,
			CtkTreeIter  *parent)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  GNode *parent_node;

  g_return_if_fail (CTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (iter != NULL);
  if (parent != NULL)
    g_return_if_fail (VALID_ITER (parent, tree_store));

  priv->columns_dirty = TRUE;

  if (parent == NULL)
    parent_node = priv->root;
  else
    parent_node = parent->user_data;

  if (parent_node->children == NULL)
    {
      CtkTreePath *path;
      
      iter->stamp = priv->stamp;
      iter->user_data = g_node_new (NULL);

      g_node_prepend (parent_node, G_NODE (iter->user_data));

      path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), iter);
      ctk_tree_model_row_inserted (CTK_TREE_MODEL (tree_store), path, iter);

      if (parent_node != priv->root)
	{
	  ctk_tree_path_up (path);
	  ctk_tree_model_row_has_child_toggled (CTK_TREE_MODEL (tree_store), path, parent);
	}
      ctk_tree_path_free (path);
    }
  else
    {
      ctk_tree_store_insert_after (tree_store, iter, parent, NULL);
    }

  validate_tree (tree_store);
}

/**
 * ctk_tree_store_append:
 * @tree_store: A #CtkTreeStore
 * @iter: (out): An unset #CtkTreeIter to set to the appended row
 * @parent: (allow-none): A valid #CtkTreeIter, or %NULL
 * 
 * Appends a new row to @tree_store.  If @parent is non-%NULL, then it will append the
 * new row after the last child of @parent, otherwise it will append a row to
 * the top level.  @iter will be changed to point to this new row.  The row will
 * be empty after this function is called.  To fill in values, you need to call
 * ctk_tree_store_set() or ctk_tree_store_set_value().
 **/
void
ctk_tree_store_append (CtkTreeStore *tree_store,
		       CtkTreeIter  *iter,
		       CtkTreeIter  *parent)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  GNode *parent_node;

  g_return_if_fail (CTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (iter != NULL);
  if (parent != NULL)
    g_return_if_fail (VALID_ITER (parent, tree_store));

  if (parent == NULL)
    parent_node = priv->root;
  else
    parent_node = parent->user_data;

  priv->columns_dirty = TRUE;

  if (parent_node->children == NULL)
    {
      CtkTreePath *path;

      iter->stamp = priv->stamp;
      iter->user_data = g_node_new (NULL);

      g_node_append (parent_node, G_NODE (iter->user_data));

      path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), iter);
      ctk_tree_model_row_inserted (CTK_TREE_MODEL (tree_store), path, iter);

      if (parent_node != priv->root)
	{
	  ctk_tree_path_up (path);
	  ctk_tree_model_row_has_child_toggled (CTK_TREE_MODEL (tree_store), path, parent);
	}
      ctk_tree_path_free (path);
    }
  else
    {
      ctk_tree_store_insert_before (tree_store, iter, parent, NULL);
    }

  validate_tree (tree_store);
}

/**
 * ctk_tree_store_is_ancestor:
 * @tree_store: A #CtkTreeStore
 * @iter: A valid #CtkTreeIter
 * @descendant: A valid #CtkTreeIter
 * 
 * Returns %TRUE if @iter is an ancestor of @descendant.  That is, @iter is the
 * parent (or grandparent or great-grandparent) of @descendant.
 * 
 * Returns: %TRUE, if @iter is an ancestor of @descendant
 **/
gboolean
ctk_tree_store_is_ancestor (CtkTreeStore *tree_store,
			    CtkTreeIter  *iter,
			    CtkTreeIter  *descendant)
{
  g_return_val_if_fail (CTK_IS_TREE_STORE (tree_store), FALSE);
  g_return_val_if_fail (VALID_ITER (iter, tree_store), FALSE);
  g_return_val_if_fail (VALID_ITER (descendant, tree_store), FALSE);

  return g_node_is_ancestor (G_NODE (iter->user_data),
			     G_NODE (descendant->user_data));
}


/**
 * ctk_tree_store_iter_depth:
 * @tree_store: A #CtkTreeStore
 * @iter: A valid #CtkTreeIter
 * 
 * Returns the depth of @iter.  This will be 0 for anything on the root level, 1
 * for anything down a level, etc.
 * 
 * Returns: The depth of @iter
 **/
gint
ctk_tree_store_iter_depth (CtkTreeStore *tree_store,
			   CtkTreeIter  *iter)
{
  g_return_val_if_fail (CTK_IS_TREE_STORE (tree_store), 0);
  g_return_val_if_fail (VALID_ITER (iter, tree_store), 0);

  return g_node_depth (G_NODE (iter->user_data)) - 2;
}

/* simple ripoff from g_node_traverse_post_order */
static gboolean
ctk_tree_store_clear_traverse (GNode        *node,
			       CtkTreeStore *store)
{
  CtkTreeIter iter;

  if (node->children)
    {
      GNode *child;

      child = node->children;
      while (child)
        {
	  register GNode *current;

	  current = child;
	  child = current->next;
	  if (ctk_tree_store_clear_traverse (current, store))
	    return TRUE;
	}

      if (node->parent)
        {
	  iter.stamp = store->priv->stamp;
	  iter.user_data = node;

	  ctk_tree_store_remove (store, &iter);
	}
    }
  else if (node->parent)
    {
      iter.stamp = store->priv->stamp;
      iter.user_data = node;

      ctk_tree_store_remove (store, &iter);
    }

  return FALSE;
}

static void
ctk_tree_store_increment_stamp (CtkTreeStore *tree_store)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  do
    {
      priv->stamp++;
    }
  while (priv->stamp == 0);
}

/**
 * ctk_tree_store_clear:
 * @tree_store: a #CtkTreeStore
 * 
 * Removes all rows from @tree_store
 **/
void
ctk_tree_store_clear (CtkTreeStore *tree_store)
{
  g_return_if_fail (CTK_IS_TREE_STORE (tree_store));

  ctk_tree_store_clear_traverse (tree_store->priv->root, tree_store);
  ctk_tree_store_increment_stamp (tree_store);
}

static gboolean
ctk_tree_store_iter_is_valid_helper (CtkTreeIter *iter,
				     GNode       *first)
{
  GNode *node;

  node = first;

  do
    {
      if (node == iter->user_data)
	return TRUE;

      if (node->children)
	if (ctk_tree_store_iter_is_valid_helper (iter, node->children))
	  return TRUE;

      node = node->next;
    }
  while (node);

  return FALSE;
}

/**
 * ctk_tree_store_iter_is_valid:
 * @tree_store: A #CtkTreeStore.
 * @iter: A #CtkTreeIter.
 *
 * WARNING: This function is slow. Only use it for debugging and/or testing
 * purposes.
 *
 * Checks if the given iter is a valid iter for this #CtkTreeStore.
 *
 * Returns: %TRUE if the iter is valid, %FALSE if the iter is invalid.
 *
 * Since: 2.2
 **/
gboolean
ctk_tree_store_iter_is_valid (CtkTreeStore *tree_store,
                              CtkTreeIter  *iter)
{
  g_return_val_if_fail (CTK_IS_TREE_STORE (tree_store), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  if (!VALID_ITER (iter, tree_store))
    return FALSE;

  return ctk_tree_store_iter_is_valid_helper (iter, tree_store->priv->root);
}

/* DND */


static gboolean real_ctk_tree_store_row_draggable (CtkTreeDragSource *drag_source G_GNUC_UNUSED,
                                                   CtkTreePath       *path G_GNUC_UNUSED)
{
  return TRUE;
}
               
static gboolean
ctk_tree_store_drag_data_delete (CtkTreeDragSource *drag_source,
                                 CtkTreePath       *path)
{
  CtkTreeIter iter;

  if (ctk_tree_store_get_iter (CTK_TREE_MODEL (drag_source),
                               &iter,
                               path))
    {
      ctk_tree_store_remove (CTK_TREE_STORE (drag_source),
                             &iter);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

static gboolean
ctk_tree_store_drag_data_get (CtkTreeDragSource *drag_source,
                              CtkTreePath       *path,
                              CtkSelectionData  *selection_data)
{
  /* Note that we don't need to handle the CTK_TREE_MODEL_ROW
   * target, because the default handler does it for us, but
   * we do anyway for the convenience of someone maybe overriding the
   * default handler.
   */

  if (ctk_tree_set_row_drag_data (selection_data,
				  CTK_TREE_MODEL (drag_source),
				  path))
    {
      return TRUE;
    }
  else
    {
      /* FIXME handle text targets at least. */
    }

  return FALSE;
}

static void
copy_node_data (CtkTreeStore *tree_store,
                CtkTreeIter  *src_iter,
                CtkTreeIter  *dest_iter)
{
  CtkTreeDataList *dl = G_NODE (src_iter->user_data)->data;
  CtkTreeDataList *copy_head = NULL;
  CtkTreeDataList *copy_prev = NULL;
  CtkTreeDataList *copy_iter = NULL;
  CtkTreePath *path;
  gint col;

  col = 0;
  while (dl)
    {
      copy_iter = _ctk_tree_data_list_node_copy (dl, tree_store->priv->column_headers[col]);

      if (copy_head == NULL)
        copy_head = copy_iter;

      if (copy_prev)
        copy_prev->next = copy_iter;

      copy_prev = copy_iter;

      dl = dl->next;
      ++col;
    }

  G_NODE (dest_iter->user_data)->data = copy_head;

  path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), dest_iter);
  ctk_tree_model_row_changed (CTK_TREE_MODEL (tree_store), path, dest_iter);
  ctk_tree_path_free (path);
}

static void
recursive_node_copy (CtkTreeStore *tree_store,
                     CtkTreeIter  *src_iter,
                     CtkTreeIter  *dest_iter)
{
  CtkTreeIter child;
  CtkTreeModel *model;

  model = CTK_TREE_MODEL (tree_store);

  copy_node_data (tree_store, src_iter, dest_iter);

  if (ctk_tree_store_iter_children (model,
                                    &child,
                                    src_iter))
    {
      /* Need to create children and recurse. Note our
       * dependence on persistent iterators here.
       */
      do
        {
          CtkTreeIter copy;

          /* Gee, a really slow algorithm... ;-) FIXME */
          ctk_tree_store_append (tree_store,
                                 &copy,
                                 dest_iter);

          recursive_node_copy (tree_store, &child, &copy);
        }
      while (ctk_tree_store_iter_next (model, &child));
    }
}

static gboolean
ctk_tree_store_drag_data_received (CtkTreeDragDest   *drag_dest,
                                   CtkTreePath       *dest,
                                   CtkSelectionData  *selection_data)
{
  CtkTreeModel *tree_model;
  CtkTreeStore *tree_store;
  CtkTreeModel *src_model = NULL;
  CtkTreePath *src_path = NULL;
  gboolean retval = FALSE;

  tree_model = CTK_TREE_MODEL (drag_dest);
  tree_store = CTK_TREE_STORE (drag_dest);

  validate_tree (tree_store);

  if (ctk_tree_get_row_drag_data (selection_data,
				  &src_model,
				  &src_path) &&
      src_model == tree_model)
    {
      /* Copy the given row to a new position */
      CtkTreeIter src_iter;
      CtkTreeIter dest_iter;
      CtkTreePath *prev;

      if (!ctk_tree_store_get_iter (src_model,
                                    &src_iter,
                                    src_path))
        {
          goto out;
        }

      /* Get the path to insert _after_ (dest is the path to insert _before_) */
      prev = ctk_tree_path_copy (dest);

      if (!ctk_tree_path_prev (prev))
        {
          CtkTreeIter dest_parent;
          CtkTreePath *parent;
          CtkTreeIter *dest_parent_p;

          /* dest was the first spot at the current depth; which means
           * we are supposed to prepend.
           */

          /* Get the parent, NULL if parent is the root */
          dest_parent_p = NULL;
          parent = ctk_tree_path_copy (dest);
          if (ctk_tree_path_up (parent) &&
	      ctk_tree_path_get_depth (parent) > 0)
            {
              ctk_tree_store_get_iter (tree_model,
                                       &dest_parent,
                                       parent);
              dest_parent_p = &dest_parent;
            }
          ctk_tree_path_free (parent);
          parent = NULL;

          ctk_tree_store_prepend (tree_store,
                                  &dest_iter,
                                  dest_parent_p);

          retval = TRUE;
        }
      else
        {
          if (ctk_tree_store_get_iter (tree_model, &dest_iter, prev))
            {
              CtkTreeIter tmp_iter = dest_iter;

              ctk_tree_store_insert_after (tree_store, &dest_iter, NULL,
                                           &tmp_iter);

              retval = TRUE;
            }
        }

      ctk_tree_path_free (prev);

      /* If we succeeded in creating dest_iter, walk src_iter tree branch,
       * duplicating it below dest_iter.
       */

      if (retval)
        {
          recursive_node_copy (tree_store,
                               &src_iter,
                               &dest_iter);
        }
    }
  else
    {
      /* FIXME maybe add some data targets eventually, or handle text
       * targets in the simple case.
       */

    }

 out:

  if (src_path)
    ctk_tree_path_free (src_path);

  return retval;
}

static gboolean
ctk_tree_store_row_drop_possible (CtkTreeDragDest  *drag_dest,
                                  CtkTreePath      *dest_path,
				  CtkSelectionData *selection_data)
{
  CtkTreeModel *src_model = NULL;
  CtkTreePath *src_path = NULL;
  CtkTreePath *tmp = NULL;
  gboolean retval = FALSE;
  
  /* don't accept drops if the tree has been sorted */
  if (CTK_TREE_STORE_IS_SORTED (drag_dest))
    return FALSE;

  if (!ctk_tree_get_row_drag_data (selection_data,
				   &src_model,
				   &src_path))
    goto out;
    
  /* can only drag to ourselves */
  if (src_model != CTK_TREE_MODEL (drag_dest))
    goto out;

  /* Can't drop into ourself. */
  if (ctk_tree_path_is_ancestor (src_path,
                                 dest_path))
    goto out;

  /* Can't drop if dest_path's parent doesn't exist */
  {
    CtkTreeIter iter;

    if (ctk_tree_path_get_depth (dest_path) > 1)
      {
	tmp = ctk_tree_path_copy (dest_path);
	ctk_tree_path_up (tmp);
	
	if (!ctk_tree_store_get_iter (CTK_TREE_MODEL (drag_dest),
				      &iter, tmp))
	  goto out;
      }
  }
  
  /* Can otherwise drop anywhere. */
  retval = TRUE;

 out:

  if (src_path)
    ctk_tree_path_free (src_path);
  if (tmp)
    ctk_tree_path_free (tmp);

  return retval;
}

/* Sorting and reordering */
typedef struct _SortTuple
{
  gint offset;
  GNode *node;
} SortTuple;

/* Reordering */
static gint
ctk_tree_store_reorder_func (gconstpointer a,
			     gconstpointer b,
			     gpointer      user_data G_GNUC_UNUSED)
{
  SortTuple *a_reorder;
  SortTuple *b_reorder;

  a_reorder = (SortTuple *)a;
  b_reorder = (SortTuple *)b;

  if (a_reorder->offset < b_reorder->offset)
    return -1;
  if (a_reorder->offset > b_reorder->offset)
    return 1;

  return 0;
}

/**
 * ctk_tree_store_reorder: (skip)
 * @tree_store: A #CtkTreeStore
 * @parent: (nullable): A #CtkTreeIter, or %NULL
 * @new_order: (array): an array of integers mapping the new position of each child
 *      to its old position before the re-ordering,
 *      i.e. @new_order`[newpos] = oldpos`.
 *
 * Reorders the children of @parent in @tree_store to follow the order
 * indicated by @new_order. Note that this function only works with
 * unsorted stores.
 *
 * Since: 2.2
 **/
void
ctk_tree_store_reorder (CtkTreeStore *tree_store,
			CtkTreeIter  *parent,
			gint         *new_order)
{
  gint i, length = 0;
  GNode *level, *node;
  CtkTreePath *path;
  SortTuple *sort_array;

  g_return_if_fail (CTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (!CTK_TREE_STORE_IS_SORTED (tree_store));
  g_return_if_fail (parent == NULL || VALID_ITER (parent, tree_store));
  g_return_if_fail (new_order != NULL);

  if (!parent)
    level = G_NODE (tree_store->priv->root)->children;
  else
    level = G_NODE (parent->user_data)->children;

  if (G_UNLIKELY (!level))
    {
      g_warning ("%s: Cannot reorder, parent has no children", G_STRLOC);
      return;
    }

  /* count nodes */
  node = level;
  while (node)
    {
      length++;
      node = node->next;
    }

  /* set up sortarray */
  sort_array = g_new (SortTuple, length);

  node = level;
  for (i = 0; i < length; i++)
    {
      sort_array[new_order[i]].offset = i;
      sort_array[i].node = node;

      node = node->next;
    }

  g_qsort_with_data (sort_array,
		     length,
		     sizeof (SortTuple),
		     ctk_tree_store_reorder_func,
		     NULL);

  /* fix up level */
  for (i = 0; i < length - 1; i++)
    {
      sort_array[i].node->next = sort_array[i+1].node;
      sort_array[i+1].node->prev = sort_array[i].node;
    }

  sort_array[length-1].node->next = NULL;
  sort_array[0].node->prev = NULL;
  if (parent)
    G_NODE (parent->user_data)->children = sort_array[0].node;
  else
    G_NODE (tree_store->priv->root)->children = sort_array[0].node;

  /* emit signal */
  if (parent)
    path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), parent);
  else
    path = ctk_tree_path_new ();
  ctk_tree_model_rows_reordered (CTK_TREE_MODEL (tree_store), path,
				 parent, new_order);
  ctk_tree_path_free (path);
  g_free (sort_array);
}

/**
 * ctk_tree_store_swap:
 * @tree_store: A #CtkTreeStore.
 * @a: A #CtkTreeIter.
 * @b: Another #CtkTreeIter.
 *
 * Swaps @a and @b in the same level of @tree_store. Note that this function
 * only works with unsorted stores.
 *
 * Since: 2.2
 **/
void
ctk_tree_store_swap (CtkTreeStore *tree_store,
		     CtkTreeIter  *a,
		     CtkTreeIter  *b)
{
  GNode *tmp, *node_a, *node_b, *parent_node;
  GNode *a_prev, *a_next, *b_prev, *b_next;
  gint i, a_count, b_count, length, *order;
  CtkTreePath *path_a, *path_b;
  CtkTreeIter parent;

  g_return_if_fail (CTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (VALID_ITER (a, tree_store));
  g_return_if_fail (VALID_ITER (b, tree_store));

  node_a = G_NODE (a->user_data);
  node_b = G_NODE (b->user_data);

  /* basic sanity checking */
  if (node_a == node_b)
    return;

  path_a = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), a);
  path_b = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), b);

  g_return_if_fail (path_a && path_b);

  ctk_tree_path_up (path_a);
  ctk_tree_path_up (path_b);

  if (ctk_tree_path_get_depth (path_a) == 0
      || ctk_tree_path_get_depth (path_b) == 0)
    {
      if (ctk_tree_path_get_depth (path_a) != ctk_tree_path_get_depth (path_b))
        {
          ctk_tree_path_free (path_a);
          ctk_tree_path_free (path_b);

          g_warning ("Given children are not in the same level\n");
          return;
        }
      parent_node = G_NODE (tree_store->priv->root);
    }
  else
    {
      if (ctk_tree_path_compare (path_a, path_b))
        {
          ctk_tree_path_free (path_a);
          ctk_tree_path_free (path_b);

          g_warning ("Given children don't have a common parent\n");
          return;
        }
      ctk_tree_store_get_iter (CTK_TREE_MODEL (tree_store), &parent,
                               path_a);
      parent_node = G_NODE (parent.user_data);
    }
  ctk_tree_path_free (path_b);

  /* old links which we have to keep around */
  a_prev = node_a->prev;
  a_next = node_a->next;

  b_prev = node_b->prev;
  b_next = node_b->next;

  /* fix up links if the nodes are next to eachother */
  if (a_prev == node_b)
    a_prev = node_a;
  if (a_next == node_b)
    a_next = node_a;

  if (b_prev == node_a)
    b_prev = node_b;
  if (b_next == node_a)
    b_next = node_b;

  /* counting nodes */
  tmp = parent_node->children;
  i = a_count = b_count = 0;
  while (tmp)
    {
      if (tmp == node_a)
	a_count = i;
      if (tmp == node_b)
	b_count = i;

      tmp = tmp->next;
      i++;
    }
  length = i;

  /* hacking the tree */
  if (!a_prev)
    parent_node->children = node_b;
  else
    a_prev->next = node_b;

  if (a_next)
    a_next->prev = node_b;

  if (!b_prev)
    parent_node->children = node_a;
  else
    b_prev->next = node_a;

  if (b_next)
    b_next->prev = node_a;

  node_a->prev = b_prev;
  node_a->next = b_next;

  node_b->prev = a_prev;
  node_b->next = a_next;

  /* emit signal */
  order = g_new (gint, length);
  for (i = 0; i < length; i++)
    if (i == a_count)
      order[i] = b_count;
    else if (i == b_count)
      order[i] = a_count;
    else
      order[i] = i;

  ctk_tree_model_rows_reordered (CTK_TREE_MODEL (tree_store), path_a,
				 parent_node == tree_store->priv->root
				 ? NULL : &parent, order);
  ctk_tree_path_free (path_a);
  g_free (order);
}

/* WARNING: this function is *incredibly* fragile. Please smashtest after
 * making changes here.
 *	-Kris
 */
static void
ctk_tree_store_move (CtkTreeStore *tree_store,
                     CtkTreeIter  *iter,
		     CtkTreeIter  *position,
		     gboolean      before)
{
  GNode *parent, *node, *a, *b, *tmp, *tmp_a, *tmp_b;
  gint old_pos, new_pos, length, i, *order;
  CtkTreePath *path = NULL, *tmppath, *pos_path = NULL;
  CtkTreeIter parent_iter, dst_a, dst_b;
  gint depth = 0;
  gboolean handle_b = TRUE;

  g_return_if_fail (CTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (!CTK_TREE_STORE_IS_SORTED (tree_store));
  g_return_if_fail (VALID_ITER (iter, tree_store));
  if (position)
    g_return_if_fail (VALID_ITER (position, tree_store));

  a = b = NULL;

  /* sanity checks */
  if (position)
    {
      path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), iter);
      pos_path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store),
	                                  position);

      /* if before:
       *   moving the iter before path or "path + 1" doesn't make sense
       * else
       *   moving the iter before path or "path - 1" doesn't make sense
       */
      if (!ctk_tree_path_compare (path, pos_path))
	goto free_paths_and_out;

      if (before)
        ctk_tree_path_next (path);
      else
        ctk_tree_path_prev (path);

      if (!ctk_tree_path_compare (path, pos_path))
	goto free_paths_and_out;

      if (before)
        ctk_tree_path_prev (path);
      else
        ctk_tree_path_next (path);

      if (ctk_tree_path_get_depth (path) != ctk_tree_path_get_depth (pos_path))
        {
          g_warning ("Given children are not in the same level\n");

	  goto free_paths_and_out;
        }

      tmppath = ctk_tree_path_copy (pos_path);
      ctk_tree_path_up (path);
      ctk_tree_path_up (tmppath);

      if (ctk_tree_path_get_depth (path) > 0 &&
	  ctk_tree_path_compare (path, tmppath))
        {
          g_warning ("Given children are not in the same level\n");

          ctk_tree_path_free (tmppath);
	  goto free_paths_and_out;
        }

      ctk_tree_path_free (tmppath);
    }

  if (!path)
    {
      path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), iter);
      ctk_tree_path_up (path);
    }

  depth = ctk_tree_path_get_depth (path);

  if (depth)
    {
      ctk_tree_store_get_iter (CTK_TREE_MODEL (tree_store), 
			       &parent_iter, path);

      parent = G_NODE (parent_iter.user_data);
    }
  else
    parent = G_NODE (tree_store->priv->root);

  /* yes, I know that this can be done shorter, but I'm doing it this way
   * so the code is also maintainable
   */

  if (before && position)
    {
      b = G_NODE (position->user_data);

      if (ctk_tree_path_get_indices (pos_path)[ctk_tree_path_get_depth (pos_path) - 1] > 0)
        {
          ctk_tree_path_prev (pos_path);
          if (ctk_tree_store_get_iter (CTK_TREE_MODEL (tree_store), 
				       &dst_a, pos_path))
            a = G_NODE (dst_a.user_data);
          else
            a = NULL;
          ctk_tree_path_next (pos_path);
	}

      /* if b is NULL, a is NULL too -- we are at the beginning of the list
       * yes and we leak memory here ...
       */
      g_return_if_fail (b);
    }
  else if (before && !position)
    {
      /* move before without position is appending */
      a = NULL;
      b = NULL;
    }
  else /* !before */
    {
      if (position)
        a = G_NODE (position->user_data);
      else
        a = NULL;

      if (position)
        {
          ctk_tree_path_next (pos_path);
          if (ctk_tree_store_get_iter (CTK_TREE_MODEL (tree_store), &dst_b, pos_path))
             b = G_NODE (dst_b.user_data);
          else
             b = NULL;
          ctk_tree_path_prev (pos_path);
	}
      else
        {
	  /* move after without position is prepending */
	  if (depth)
	    ctk_tree_store_iter_children (CTK_TREE_MODEL (tree_store), &dst_b,
	                                  &parent_iter);
	  else
	    ctk_tree_store_iter_children (CTK_TREE_MODEL (tree_store), &dst_b,
		                          NULL);

	  b = G_NODE (dst_b.user_data);
	}

      /* if a is NULL, b is NULL too -- we are at the end of the list
       * yes and we leak memory here ...
       */
      if (position)
        g_return_if_fail (a);
    }

  /* counting nodes */
  tmp = parent->children;

  length = old_pos = 0;
  while (tmp)
    {
      if (tmp == iter->user_data)
	old_pos = length;

      tmp = tmp->next;
      length++;
    }

  /* remove node from list */
  node = G_NODE (iter->user_data);
  tmp_a = node->prev;
  tmp_b = node->next;

  if (tmp_a)
    tmp_a->next = tmp_b;
  else
    parent->children = tmp_b;

  if (tmp_b)
    tmp_b->prev = tmp_a;

  /* and reinsert the node */
  if (a)
    {
      tmp = a->next;

      a->next = node;
      node->next = tmp;
      node->prev = a;
    }
  else if (!a && !before)
    {
      tmp = parent->children;

      node->prev = NULL;
      parent->children = node;

      node->next = tmp;
      if (tmp) 
	tmp->prev = node;

      handle_b = FALSE;
    }
  else if (!a && before)
    {
      if (!position)
        {
          node->parent = NULL;
          node->next = node->prev = NULL;

          /* before with sibling = NULL appends */
          g_node_insert_before (parent, NULL, node);
	}
      else
        {
	  node->parent = NULL;
	  node->next = node->prev = NULL;

	  /* after with sibling = NULL prepends */
	  g_node_insert_after (parent, NULL, node);
	}

      handle_b = FALSE;
    }

  if (handle_b)
    {
      if (b)
        {
          tmp = b->prev;

          b->prev = node;
          node->prev = tmp;
          node->next = b;
        }
      else if (!(!a && before)) /* !a && before is completely handled above */
        node->next = NULL;
    }

  /* emit signal */
  if (position)
    new_pos = ctk_tree_path_get_indices (pos_path)[ctk_tree_path_get_depth (pos_path)-1];
  else if (before)
    {
      if (depth)
        new_pos = ctk_tree_store_iter_n_children (CTK_TREE_MODEL (tree_store),
	                                          &parent_iter) - 1;
      else
	new_pos = ctk_tree_store_iter_n_children (CTK_TREE_MODEL (tree_store),
	                                          NULL) - 1;
    }
  else
    new_pos = 0;

  if (new_pos > old_pos)
    {
      if (before && position)
        new_pos--;
    }
  else
    {
      if (!before && position)
        new_pos++;
    }

  order = g_new (gint, length);
  if (new_pos > old_pos)
    {
      for (i = 0; i < length; i++)
        if (i < old_pos)
          order[i] = i;
        else if (i >= old_pos && i < new_pos)
          order[i] = i + 1;
        else if (i == new_pos)
          order[i] = old_pos;
        else
	  order[i] = i;
    }
  else
    {
      for (i = 0; i < length; i++)
        if (i == new_pos)
	  order[i] = old_pos;
        else if (i > new_pos && i <= old_pos)
	  order[i] = i - 1;
	else
	  order[i] = i;
    }

  if (depth)
    {
      tmppath = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), 
					 &parent_iter);
      ctk_tree_model_rows_reordered (CTK_TREE_MODEL (tree_store),
				     tmppath, &parent_iter, order);
    }
  else
    {
      tmppath = ctk_tree_path_new ();
      ctk_tree_model_rows_reordered (CTK_TREE_MODEL (tree_store),
				     tmppath, NULL, order);
    }

  ctk_tree_path_free (tmppath);
  ctk_tree_path_free (path);
  if (position)
    ctk_tree_path_free (pos_path);
  g_free (order);

  return;

free_paths_and_out:
  ctk_tree_path_free (path);
  ctk_tree_path_free (pos_path);
}

/**
 * ctk_tree_store_move_before:
 * @tree_store: A #CtkTreeStore.
 * @iter: A #CtkTreeIter.
 * @position: (allow-none): A #CtkTreeIter or %NULL.
 *
 * Moves @iter in @tree_store to the position before @position. @iter and
 * @position should be in the same level. Note that this function only
 * works with unsorted stores. If @position is %NULL, @iter will be
 * moved to the end of the level.
 *
 * Since: 2.2
 **/
void
ctk_tree_store_move_before (CtkTreeStore *tree_store,
                            CtkTreeIter  *iter,
			    CtkTreeIter  *position)
{
  ctk_tree_store_move (tree_store, iter, position, TRUE);
}

/**
 * ctk_tree_store_move_after:
 * @tree_store: A #CtkTreeStore.
 * @iter: A #CtkTreeIter.
 * @position: (allow-none): A #CtkTreeIter.
 *
 * Moves @iter in @tree_store to the position after @position. @iter and
 * @position should be in the same level. Note that this function only
 * works with unsorted stores. If @position is %NULL, @iter will be moved
 * to the start of the level.
 *
 * Since: 2.2
 **/
void
ctk_tree_store_move_after (CtkTreeStore *tree_store,
                           CtkTreeIter  *iter,
			   CtkTreeIter  *position)
{
  ctk_tree_store_move (tree_store, iter, position, FALSE);
}

/* Sorting */
static gint
ctk_tree_store_compare_func (gconstpointer a,
			     gconstpointer b,
			     gpointer      user_data)
{
  CtkTreeStore *tree_store = user_data;
  CtkTreeStorePrivate *priv = tree_store->priv;
  GNode *node_a;
  GNode *node_b;
  CtkTreeIterCompareFunc func;
  gpointer data;

  CtkTreeIter iter_a;
  CtkTreeIter iter_b;
  gint retval;

  if (priv->sort_column_id != -1)
    {
      CtkTreeDataSortHeader *header;

      header = _ctk_tree_data_list_get_header (priv->sort_list,
					       priv->sort_column_id);
      g_return_val_if_fail (header != NULL, 0);
      g_return_val_if_fail (header->func != NULL, 0);

      func = header->func;
      data = header->data;
    }
  else
    {
      g_return_val_if_fail (priv->default_sort_func != NULL, 0);
      func = priv->default_sort_func;
      data = priv->default_sort_data;
    }

  node_a = ((SortTuple *) a)->node;
  node_b = ((SortTuple *) b)->node;

  iter_a.stamp = priv->stamp;
  iter_a.user_data = node_a;
  iter_b.stamp = priv->stamp;
  iter_b.user_data = node_b;

  retval = (* func) (CTK_TREE_MODEL (user_data), &iter_a, &iter_b, data);

  if (priv->order == CTK_SORT_DESCENDING)
    {
      if (retval > 0)
	retval = -1;
      else if (retval < 0)
	retval = 1;
    }
  return retval;
}

static void
ctk_tree_store_sort_helper (CtkTreeStore *tree_store,
			    GNode        *parent,
			    gboolean      recurse)
{
  CtkTreeIter iter;
  GArray *sort_array;
  GNode *node;
  GNode *tmp_node;
  gint list_length;
  gint i;
  gint *new_order;
  CtkTreePath *path;

  node = parent->children;
  if (node == NULL || node->next == NULL)
    {
      if (recurse && node && node->children)
        ctk_tree_store_sort_helper (tree_store, node, TRUE);

      return;
    }

  list_length = 0;
  for (tmp_node = node; tmp_node; tmp_node = tmp_node->next)
    list_length++;

  sort_array = g_array_sized_new (FALSE, FALSE, sizeof (SortTuple), list_length);

  i = 0;
  for (tmp_node = node; tmp_node; tmp_node = tmp_node->next)
    {
      SortTuple tuple;

      tuple.offset = i;
      tuple.node = tmp_node;
      g_array_append_val (sort_array, tuple);
      i++;
    }

  /* Sort the array */
  g_array_sort_with_data (sort_array, ctk_tree_store_compare_func, tree_store);

  for (i = 0; i < list_length - 1; i++)
    {
      g_array_index (sort_array, SortTuple, i).node->next =
	g_array_index (sort_array, SortTuple, i + 1).node;
      g_array_index (sort_array, SortTuple, i + 1).node->prev =
	g_array_index (sort_array, SortTuple, i).node;
    }
  g_array_index (sort_array, SortTuple, list_length - 1).node->next = NULL;
  g_array_index (sort_array, SortTuple, 0).node->prev = NULL;
  parent->children = g_array_index (sort_array, SortTuple, 0).node;

  /* Let the world know about our new order */
  new_order = g_new (gint, list_length);
  for (i = 0; i < list_length; i++)
    new_order[i] = g_array_index (sort_array, SortTuple, i).offset;

  iter.stamp = tree_store->priv->stamp;
  iter.user_data = parent;
  path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), &iter);
  ctk_tree_model_rows_reordered (CTK_TREE_MODEL (tree_store),
				 path, &iter, new_order);
  ctk_tree_path_free (path);
  g_free (new_order);
  g_array_free (sort_array, TRUE);

  if (recurse)
    {
      for (tmp_node = parent->children; tmp_node; tmp_node = tmp_node->next)
	{
	  if (tmp_node->children)
	    ctk_tree_store_sort_helper (tree_store, tmp_node, TRUE);
	}
    }
}

static void
ctk_tree_store_sort (CtkTreeStore *tree_store)
{
  CtkTreeStorePrivate *priv = tree_store->priv;

  if (!CTK_TREE_STORE_IS_SORTED (tree_store))
    return;

  if (priv->sort_column_id != -1)
    {
      CtkTreeDataSortHeader *header = NULL;

      header = _ctk_tree_data_list_get_header (priv->sort_list,
					       priv->sort_column_id);

      /* We want to make sure that we have a function */
      g_return_if_fail (header != NULL);
      g_return_if_fail (header->func != NULL);
    }
  else
    {
      g_return_if_fail (priv->default_sort_func != NULL);
    }

  ctk_tree_store_sort_helper (tree_store, G_NODE (priv->root), TRUE);
}

static void
ctk_tree_store_sort_iter_changed (CtkTreeStore *tree_store,
				  CtkTreeIter  *iter,
				  gint          column,
				  gboolean      emit_signal)
{
  CtkTreeStorePrivate *priv = tree_store->priv;
  GNode *prev = NULL;
  GNode *next = NULL;
  GNode *node;
  CtkTreePath *tmp_path;
  CtkTreeIter tmp_iter;
  gint cmp_a = 0;
  gint cmp_b = 0;
  gint i;
  gint old_location;
  gint new_location;
  gint *new_order;
  gint length;
  CtkTreeIterCompareFunc func;
  gpointer data;

  g_return_if_fail (G_NODE (iter->user_data)->parent != NULL);

  tmp_iter.stamp = priv->stamp;
  if (priv->sort_column_id != -1)
    {
      CtkTreeDataSortHeader *header;
      header = _ctk_tree_data_list_get_header (priv->sort_list,
					       priv->sort_column_id);
      g_return_if_fail (header != NULL);
      g_return_if_fail (header->func != NULL);
      func = header->func;
      data = header->data;
    }
  else
    {
      g_return_if_fail (priv->default_sort_func != NULL);
      func = priv->default_sort_func;
      data = priv->default_sort_data;
    }

  /* If it's the built in function, we don't sort. */
  if (func == _ctk_tree_data_list_compare_func &&
      priv->sort_column_id != column)
    return;

  old_location = 0;
  node = G_NODE (iter->user_data)->parent->children;
  /* First we find the iter, its prev, and its next */
  while (node)
    {
      if (node == G_NODE (iter->user_data))
	break;
      old_location++;
      node = node->next;
    }
  g_assert (node != NULL);

  prev = node->prev;
  next = node->next;

  /* Check the common case, where we don't need to sort it moved. */
  if (prev != NULL)
    {
      tmp_iter.user_data = prev;
      cmp_a = (* func) (CTK_TREE_MODEL (tree_store), &tmp_iter, iter, data);
    }

  if (next != NULL)
    {
      tmp_iter.user_data = next;
      cmp_b = (* func) (CTK_TREE_MODEL (tree_store), iter, &tmp_iter, data);
    }

  if (priv->order == CTK_SORT_DESCENDING)
    {
      if (cmp_a < 0)
	cmp_a = 1;
      else if (cmp_a > 0)
	cmp_a = -1;

      if (cmp_b < 0)
	cmp_b = 1;
      else if (cmp_b > 0)
	cmp_b = -1;
    }

  if (prev == NULL && cmp_b <= 0)
    return;
  else if (next == NULL && cmp_a <= 0)
    return;
  else if (prev != NULL && next != NULL &&
	   cmp_a <= 0 && cmp_b <= 0)
    return;

  /* We actually need to sort it */
  /* First, remove the old link. */

  if (prev)
    prev->next = next;
  else
    node->parent->children = next;

  if (next)
    next->prev = prev;

  node->prev = NULL;
  node->next = NULL;

  /* FIXME: as an optimization, we can potentially start at next */
  prev = NULL;
  node = node->parent->children;
  new_location = 0;
  tmp_iter.user_data = node;
  if (priv->order == CTK_SORT_DESCENDING)
    cmp_a = (* func) (CTK_TREE_MODEL (tree_store), &tmp_iter, iter, data);
  else
    cmp_a = (* func) (CTK_TREE_MODEL (tree_store), iter, &tmp_iter, data);

  while ((node->next) && (cmp_a > 0))
    {
      prev = node;
      node = node->next;
      new_location++;
      tmp_iter.user_data = node;
      if (priv->order == CTK_SORT_DESCENDING)
	cmp_a = (* func) (CTK_TREE_MODEL (tree_store), &tmp_iter, iter, data);
      else
	cmp_a = (* func) (CTK_TREE_MODEL (tree_store), iter, &tmp_iter, data);
    }

  if ((!node->next) && (cmp_a > 0))
    {
      new_location++;
      node->next = G_NODE (iter->user_data);
      node->next->prev = node;
    }
  else if (prev)
    {
      prev->next = G_NODE (iter->user_data);
      prev->next->prev = prev;
      G_NODE (iter->user_data)->next = node;
      G_NODE (iter->user_data)->next->prev = G_NODE (iter->user_data);
    }
  else
    {
      G_NODE (iter->user_data)->next = G_NODE (iter->user_data)->parent->children;
      G_NODE (iter->user_data)->next->prev = G_NODE (iter->user_data);
      G_NODE (iter->user_data)->parent->children = G_NODE (iter->user_data);
    }

  if (!emit_signal)
    return;

  /* Emit the reordered signal. */
  length = g_node_n_children (node->parent);
  new_order = g_new (int, length);
  if (old_location < new_location)
    for (i = 0; i < length; i++)
      {
	if (i < old_location ||
	    i > new_location)
	  new_order[i] = i;
	else if (i >= old_location &&
		 i < new_location)
	  new_order[i] = i + 1;
	else if (i == new_location)
	  new_order[i] = old_location;
      }
  else
    for (i = 0; i < length; i++)
      {
	if (i < new_location ||
	    i > old_location)
	  new_order[i] = i;
	else if (i > new_location &&
		 i <= old_location)
	  new_order[i] = i - 1;
	else if (i == new_location)
	  new_order[i] = old_location;
      }

  tmp_iter.user_data = node->parent;
  tmp_path = ctk_tree_store_get_path (CTK_TREE_MODEL (tree_store), &tmp_iter);

  ctk_tree_model_rows_reordered (CTK_TREE_MODEL (tree_store),
				 tmp_path, &tmp_iter,
				 new_order);

  ctk_tree_path_free (tmp_path);
  g_free (new_order);
}


static gboolean
ctk_tree_store_get_sort_column_id (CtkTreeSortable  *sortable,
				   gint             *sort_column_id,
				   CtkSortType      *order)
{
  CtkTreeStore *tree_store = (CtkTreeStore *) sortable;
  CtkTreeStorePrivate *priv = tree_store->priv;

  if (sort_column_id)
    * sort_column_id = priv->sort_column_id;
  if (order)
    * order = priv->order;

  if (priv->sort_column_id == CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID ||
      priv->sort_column_id == CTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID)
    return FALSE;

  return TRUE;
}

static void
ctk_tree_store_set_sort_column_id (CtkTreeSortable  *sortable,
				   gint              sort_column_id,
				   CtkSortType       order)
{
  CtkTreeStore *tree_store = (CtkTreeStore *) sortable;
  CtkTreeStorePrivate *priv = tree_store->priv;

  if ((priv->sort_column_id == sort_column_id) &&
      (priv->order == order))
    return;

  if (sort_column_id != CTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID)
    {
      if (sort_column_id != CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID)
	{
	  CtkTreeDataSortHeader *header = NULL;

	  header = _ctk_tree_data_list_get_header (priv->sort_list,
						   sort_column_id);

	  /* We want to make sure that we have a function */
	  g_return_if_fail (header != NULL);
	  g_return_if_fail (header->func != NULL);
	}
      else
	{
	  g_return_if_fail (priv->default_sort_func != NULL);
	}
    }

  priv->sort_column_id = sort_column_id;
  priv->order = order;

  ctk_tree_sortable_sort_column_changed (sortable);

  ctk_tree_store_sort (tree_store);
}

static void
ctk_tree_store_set_sort_func (CtkTreeSortable        *sortable,
			      gint                    sort_column_id,
			      CtkTreeIterCompareFunc  func,
			      gpointer                data,
			      GDestroyNotify          destroy)
{
  CtkTreeStore *tree_store = (CtkTreeStore *) sortable;
  CtkTreeStorePrivate *priv = tree_store->priv;

  priv->sort_list = _ctk_tree_data_list_set_header (priv->sort_list,
						    sort_column_id,
						    func, data, destroy);

  if (priv->sort_column_id == sort_column_id)
    ctk_tree_store_sort (tree_store);
}

static void
ctk_tree_store_set_default_sort_func (CtkTreeSortable        *sortable,
				      CtkTreeIterCompareFunc  func,
				      gpointer                data,
				      GDestroyNotify          destroy)
{
  CtkTreeStore *tree_store = (CtkTreeStore *) sortable;
  CtkTreeStorePrivate *priv = tree_store->priv;

  if (priv->default_sort_destroy)
    {
      GDestroyNotify d = priv->default_sort_destroy;

      priv->default_sort_destroy = NULL;
      d (priv->default_sort_data);
    }

  priv->default_sort_func = func;
  priv->default_sort_data = data;
  priv->default_sort_destroy = destroy;

  if (priv->sort_column_id == CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID)
    ctk_tree_store_sort (tree_store);
}

static gboolean
ctk_tree_store_has_default_sort_func (CtkTreeSortable *sortable)
{
  CtkTreeStore *tree_store = (CtkTreeStore *) sortable;

  return (tree_store->priv->default_sort_func != NULL);
}

#ifdef G_ENABLE_DEBUG
static void
validate_gnode (GNode* node)
{
  GNode *iter;

  iter = node->children;
  while (iter != NULL)
    {
      g_assert (iter->parent == node);
      if (iter->prev)
        g_assert (iter->prev->next == iter);
      validate_gnode (iter);
      iter = iter->next;
    }
}
#endif

/* CtkBuildable custom tag implementation
 *
 * <columns>
 *   <column type="..."/>
 *   <column type="..."/>
 * </columns>
 */
typedef struct {
  CtkBuilder *builder;
  GObject *object;
  GSList *items;
} GSListSubParserData;

static void
tree_model_start_element (GMarkupParseContext  *context,
                          const gchar          *element_name,
                          const gchar         **names,
                          const gchar         **values,
                          gpointer              user_data,
                          GError              **error)
{
  GSListSubParserData *data = (GSListSubParserData*)user_data;

  if (strcmp (element_name, "columns") == 0)
    {
      if (!_ctk_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _ctk_builder_prefix_error (data->builder, context, error);

    }
  else if (strcmp (element_name, "column") == 0)
    {
      const gchar *type;

      if (!_ctk_builder_check_parent (data->builder, context, "columns", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "type", &type,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      data->items = g_slist_prepend (data->items, g_strdup (type));
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkTreeStore", element_name,
                                        error);
    }
}

static void
tree_model_end_element (GMarkupParseContext  *context G_GNUC_UNUSED,
                        const gchar          *element_name,
                        gpointer              user_data,
                        GError              **error G_GNUC_UNUSED)
{
  GSListSubParserData *data = (GSListSubParserData*)user_data;

  g_assert(data->builder);

  if (strcmp (element_name, "columns") == 0)
    {
      GSList *l;
      GType *types;
      int i;
      GType type;

      data = (GSListSubParserData*)user_data;
      data->items = g_slist_reverse (data->items);
      types = g_new0 (GType, g_slist_length (data->items));

      for (l = data->items, i = 0; l; l = l->next, i++)
        {
          type = ctk_builder_get_type_from_name (data->builder, l->data);
          if (type == G_TYPE_INVALID)
            {
              g_warning ("Unknown type %s specified in treemodel %s",
                         (const gchar*)l->data,
                         ctk_buildable_get_name (CTK_BUILDABLE (data->object)));
              continue;
            }
          types[i] = type;

          g_free (l->data);
        }

      ctk_tree_store_set_column_types (CTK_TREE_STORE (data->object), i, types);

      g_free (types);
    }
}

static const GMarkupParser tree_model_parser =
  {
    .start_element = tree_model_start_element,
    .end_element = tree_model_end_element
  };


static gboolean
ctk_tree_store_buildable_custom_tag_start (CtkBuildable  *buildable,
                                           CtkBuilder    *builder,
                                           GObject       *child,
                                           const gchar   *tagname,
                                           GMarkupParser *parser,
                                           gpointer      *parser_data)
{
  GSListSubParserData *data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "columns") == 0)
    {
      data = g_slice_new0 (GSListSubParserData);
      data->builder = builder;
      data->items = NULL;
      data->object = G_OBJECT (buildable);

      *parser = tree_model_parser;
      *parser_data = data;

      return TRUE;
    }

  return FALSE;
}

static void
ctk_tree_store_buildable_custom_finished (CtkBuildable *buildable G_GNUC_UNUSED,
                                          CtkBuilder   *builder G_GNUC_UNUSED,
                                          GObject      *child G_GNUC_UNUSED,
                                          const gchar  *tagname,
                                          gpointer      user_data)
{
  GSListSubParserData *data;

  if (strcmp (tagname, "columns"))
    return;

  data = (GSListSubParserData*)user_data;

  g_slist_free (data->items);
  g_slice_free (GSListSubParserData, data);
}
