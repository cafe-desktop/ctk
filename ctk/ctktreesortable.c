/* ctktreesortable.c
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
#include "ctktreesortable.h"
#include "ctkmarshalers.h"
#include "ctkintl.h"


/**
 * SECTION:ctktreesortable
 * @Short_description: The interface for sortable models used by CtkTreeView
 * @Title: CtkTreeSortable
 * @See_also:#CtkTreeModel, #CtkTreeView
 *
 * #CtkTreeSortable is an interface to be implemented by tree models which
 * support sorting. The #CtkTreeView uses the methods provided by this interface
 * to sort the model.
 */


static void ctk_tree_sortable_base_init (gpointer g_class);

GType
ctk_tree_sortable_get_type (void)
{
  static GType tree_sortable_type = 0;

  if (! tree_sortable_type)
    {
      const GTypeInfo tree_sortable_info =
      {
	.class_size = sizeof (CtkTreeSortableIface),
	.base_init = ctk_tree_sortable_base_init,
	.instance_size = 0,
	.n_preallocs = 0
      };

      tree_sortable_type =
	g_type_register_static (G_TYPE_INTERFACE, I_("CtkTreeSortable"),
				&tree_sortable_info, 0);

      g_type_interface_add_prerequisite (tree_sortable_type, CTK_TYPE_TREE_MODEL);
    }

  return tree_sortable_type;
}

static void
ctk_tree_sortable_base_init (gpointer g_class G_GNUC_UNUSED)
{
  static gboolean initialized = FALSE;

  if (! initialized)
    {
      /**
       * CtkTreeSortable::sort-column-changed:
       * @sortable: the object on which the signal is emitted
       *
       * The ::sort-column-changed signal is emitted when the sort column
       * or sort order of @sortable is changed. The signal is emitted before
       * the contents of @sortable are resorted.
       */
      g_signal_new (I_("sort-column-changed"),
                    CTK_TYPE_TREE_SORTABLE,
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (CtkTreeSortableIface, sort_column_changed),
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 0);
      initialized = TRUE;
    }
}

/**
 * ctk_tree_sortable_sort_column_changed:
 * @sortable: A #CtkTreeSortable
 * 
 * Emits a #CtkTreeSortable::sort-column-changed signal on @sortable.
 */
void
ctk_tree_sortable_sort_column_changed (CtkTreeSortable *sortable)
{
  g_return_if_fail (CTK_IS_TREE_SORTABLE (sortable));

  g_signal_emit_by_name (sortable, "sort-column-changed");
}

/**
 * ctk_tree_sortable_get_sort_column_id:
 * @sortable: A #CtkTreeSortable
 * @sort_column_id: (out): The sort column id to be filled in
 * @order: (out): The #CtkSortType to be filled in
 * 
 * Fills in @sort_column_id and @order with the current sort column and the
 * order. It returns %TRUE unless the @sort_column_id is 
 * %CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID or 
 * %CTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID.
 * 
 * Returns: %TRUE if the sort column is not one of the special sort
 *   column ids.
 **/
gboolean
ctk_tree_sortable_get_sort_column_id (CtkTreeSortable  *sortable,
				      gint             *sort_column_id,
				      CtkSortType      *order)
{
  CtkTreeSortableIface *iface;

  g_return_val_if_fail (CTK_IS_TREE_SORTABLE (sortable), FALSE);

  iface = CTK_TREE_SORTABLE_GET_IFACE (sortable);

  g_return_val_if_fail (iface != NULL, FALSE);
  g_return_val_if_fail (iface->get_sort_column_id != NULL, FALSE);

  return (* iface->get_sort_column_id) (sortable, sort_column_id, order);
}

/**
 * ctk_tree_sortable_set_sort_column_id:
 * @sortable: A #CtkTreeSortable
 * @sort_column_id: the sort column id to set
 * @order: The sort order of the column
 * 
 * Sets the current sort column to be @sort_column_id. The @sortable will
 * resort itself to reflect this change, after emitting a
 * #CtkTreeSortable::sort-column-changed signal. @sort_column_id may either be
 * a regular column id, or one of the following special values:
 * 
 * - %CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID: the default sort function
 *   will be used, if it is set
 * 
 * - %CTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID: no sorting will occur
 */
void
ctk_tree_sortable_set_sort_column_id (CtkTreeSortable  *sortable,
				      gint              sort_column_id,
				      CtkSortType       order)
{
  CtkTreeSortableIface *iface;

  g_return_if_fail (CTK_IS_TREE_SORTABLE (sortable));

  iface = CTK_TREE_SORTABLE_GET_IFACE (sortable);

  g_return_if_fail (iface != NULL);
  g_return_if_fail (iface->set_sort_column_id != NULL);
  
  (* iface->set_sort_column_id) (sortable, sort_column_id, order);
}

/**
 * ctk_tree_sortable_set_sort_func:
 * @sortable: A #CtkTreeSortable
 * @sort_column_id: the sort column id to set the function for
 * @sort_func: The comparison function
 * @user_data: (allow-none): User data to pass to @sort_func, or %NULL
 * @destroy: (allow-none): Destroy notifier of @user_data, or %NULL
 * 
 * Sets the comparison function used when sorting to be @sort_func. If the
 * current sort column id of @sortable is the same as @sort_column_id, then 
 * the model will sort using this function.
 */
void
ctk_tree_sortable_set_sort_func (CtkTreeSortable        *sortable,
				 gint                    sort_column_id,
				 CtkTreeIterCompareFunc  sort_func,
				 gpointer                user_data,
				 GDestroyNotify          destroy)
{
  CtkTreeSortableIface *iface;

  g_return_if_fail (CTK_IS_TREE_SORTABLE (sortable));
  g_return_if_fail (sort_func != NULL);

  iface = CTK_TREE_SORTABLE_GET_IFACE (sortable);

  g_return_if_fail (iface != NULL);
  g_return_if_fail (iface->set_sort_func != NULL);
  g_return_if_fail (sort_column_id >= 0);

  (* iface->set_sort_func) (sortable, sort_column_id, sort_func, user_data, destroy);
}

/**
 * ctk_tree_sortable_set_default_sort_func:
 * @sortable: A #CtkTreeSortable
 * @sort_func: The comparison function
 * @user_data: (allow-none): User data to pass to @sort_func, or %NULL
 * @destroy: (allow-none): Destroy notifier of @user_data, or %NULL
 * 
 * Sets the default comparison function used when sorting to be @sort_func.  
 * If the current sort column id of @sortable is
 * %CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, then the model will sort using 
 * this function.
 *
 * If @sort_func is %NULL, then there will be no default comparison function.
 * This means that once the model  has been sorted, it can’t go back to the
 * default state. In this case, when the current sort column id of @sortable 
 * is %CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, the model will be unsorted.
 */
void
ctk_tree_sortable_set_default_sort_func (CtkTreeSortable        *sortable,
					 CtkTreeIterCompareFunc  sort_func,
					 gpointer                user_data,
					 GDestroyNotify          destroy)
{
  CtkTreeSortableIface *iface;

  g_return_if_fail (CTK_IS_TREE_SORTABLE (sortable));

  iface = CTK_TREE_SORTABLE_GET_IFACE (sortable);

  g_return_if_fail (iface != NULL);
  g_return_if_fail (iface->set_default_sort_func != NULL);
  
  (* iface->set_default_sort_func) (sortable, sort_func, user_data, destroy);
}

/**
 * ctk_tree_sortable_has_default_sort_func:
 * @sortable: A #CtkTreeSortable
 * 
 * Returns %TRUE if the model has a default sort function. This is used
 * primarily by CtkTreeViewColumns in order to determine if a model can 
 * go back to the default state, or not.
 * 
 * Returns: %TRUE, if the model has a default sort function
 */
gboolean
ctk_tree_sortable_has_default_sort_func (CtkTreeSortable *sortable)
{
  CtkTreeSortableIface *iface;

  g_return_val_if_fail (CTK_IS_TREE_SORTABLE (sortable), FALSE);

  iface = CTK_TREE_SORTABLE_GET_IFACE (sortable);

  g_return_val_if_fail (iface != NULL, FALSE);
  g_return_val_if_fail (iface->has_default_sort_func != NULL, FALSE);
  
  return (* iface->has_default_sort_func) (sortable);
}
