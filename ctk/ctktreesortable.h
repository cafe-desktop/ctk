/* ctktreesortable.h
 * Copyright (C) 2001  Red Hat, Inc.
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

#ifndef __CTK_TREE_SORTABLE_H__
#define __CTK_TREE_SORTABLE_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkenums.h>
#include <ctk/ctktreemodel.h>


G_BEGIN_DECLS

#define CTK_TYPE_TREE_SORTABLE            (ctk_tree_sortable_get_type ())
#define CTK_TREE_SORTABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TREE_SORTABLE, CtkTreeSortable))
#define CTK_TREE_SORTABLE_CLASS(obj)      (G_TYPE_CHECK_CLASS_CAST ((obj), CTK_TYPE_TREE_SORTABLE, CtkTreeSortableIface))
#define CTK_IS_TREE_SORTABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TREE_SORTABLE))
#define CTK_TREE_SORTABLE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CTK_TYPE_TREE_SORTABLE, CtkTreeSortableIface))

/**
 * CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID:
 *
 * The CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID can be used to make a
 * #CtkTreeSortable use the default sort function.
 *
 * See also ctk_tree_sortable_set_sort_column_id()
 */
#define CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID (-1)

/**
 * CTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID:
 *
 * The CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID can be used to make a
 * #CtkTreeSortable use no sorting.
 *
 * See also ctk_tree_sortable_set_sort_column_id()
 */
#define CTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID (-2)

typedef struct _CtkTreeSortable      CtkTreeSortable; /* Dummy typedef */
typedef struct _CtkTreeSortableIface CtkTreeSortableIface;

/**
 * CtkTreeIterCompareFunc:
 * @model: The #CtkTreeModel the comparison is within
 * @a: A #CtkTreeIter in @model
 * @b: Another #CtkTreeIter in @model
 * @user_data: Data passed when the compare func is assigned e.g. by
 *  ctk_tree_sortable_set_sort_func()
 *
 * A CtkTreeIterCompareFunc should return a negative integer, zero, or a positive
 * integer if @a sorts before @b, @a sorts with @b, or @a sorts after @b
 * respectively. If two iters compare as equal, their order in the sorted model
 * is undefined. In order to ensure that the #CtkTreeSortable behaves as
 * expected, the CtkTreeIterCompareFunc must define a partial order on
 * the model, i.e. it must be reflexive, antisymmetric and transitive.
 *
 * For example, if @model is a product catalogue, then a compare function
 * for the “price” column could be one which returns
 * `price_of(@a) - price_of(@b)`.
 *
 * Returns: a negative integer, zero or a positive integer depending on whether
 *   @a sorts before, with or after @b
 */
typedef gint (* CtkTreeIterCompareFunc) (CtkTreeModel *model,
					 CtkTreeIter  *a,
					 CtkTreeIter  *b,
					 gpointer      user_data);


/**
 * CtkTreeSortableIface:
 * @sort_column_changed: Signal emitted when the sort column or sort
 *    order of sortable is changed.
 * @get_sort_column_id: Fills in sort_column_id and order with the
 *    current sort column and the order.
 * @set_sort_column_id: Sets the current sort column to be
 *    sort_column_id.
 * @set_sort_func: Sets the comparison function used when sorting to
 *    be sort_func.
 * @set_default_sort_func: Sets the default comparison function used
 *    when sorting to be sort_func.
 * @has_default_sort_func: %TRUE if the model has a default sort
 * function.
 */
struct _CtkTreeSortableIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  /* signals */
  void     (* sort_column_changed)   (CtkTreeSortable        *sortable);

  /* virtual table */
  gboolean (* get_sort_column_id)    (CtkTreeSortable        *sortable,
				      gint                   *sort_column_id,
				      CtkSortType            *order);
  void     (* set_sort_column_id)    (CtkTreeSortable        *sortable,
				      gint                    sort_column_id,
				      CtkSortType             order);
  void     (* set_sort_func)         (CtkTreeSortable        *sortable,
				      gint                    sort_column_id,
				      CtkTreeIterCompareFunc  sort_func,
				      gpointer                user_data,
				      GDestroyNotify          destroy);
  void     (* set_default_sort_func) (CtkTreeSortable        *sortable,
				      CtkTreeIterCompareFunc  sort_func,
				      gpointer                user_data,
				      GDestroyNotify          destroy);
  gboolean (* has_default_sort_func) (CtkTreeSortable        *sortable);
};


GDK_AVAILABLE_IN_ALL
GType    ctk_tree_sortable_get_type              (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
void     ctk_tree_sortable_sort_column_changed   (CtkTreeSortable        *sortable);
GDK_AVAILABLE_IN_ALL
gboolean ctk_tree_sortable_get_sort_column_id    (CtkTreeSortable        *sortable,
						  gint                   *sort_column_id,
						  CtkSortType            *order);
GDK_AVAILABLE_IN_ALL
void     ctk_tree_sortable_set_sort_column_id    (CtkTreeSortable        *sortable,
						  gint                    sort_column_id,
						  CtkSortType             order);
GDK_AVAILABLE_IN_ALL
void     ctk_tree_sortable_set_sort_func         (CtkTreeSortable        *sortable,
						  gint                    sort_column_id,
						  CtkTreeIterCompareFunc  sort_func,
						  gpointer                user_data,
						  GDestroyNotify          destroy);
GDK_AVAILABLE_IN_ALL
void     ctk_tree_sortable_set_default_sort_func (CtkTreeSortable        *sortable,
						  CtkTreeIterCompareFunc  sort_func,
						  gpointer                user_data,
						  GDestroyNotify          destroy);
GDK_AVAILABLE_IN_ALL
gboolean ctk_tree_sortable_has_default_sort_func (CtkTreeSortable        *sortable);

G_END_DECLS

#endif /* __CTK_TREE_SORTABLE_H__ */
