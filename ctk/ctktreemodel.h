/* ctktreemodel.h
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

#ifndef __CTK_TREE_MODEL_H__
#define __CTK_TREE_MODEL_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <glib-object.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define CTK_TYPE_TREE_MODEL            (ctk_tree_model_get_type ())
#define CTK_TREE_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TREE_MODEL, CtkTreeModel))
#define CTK_IS_TREE_MODEL(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TREE_MODEL))
#define CTK_TREE_MODEL_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CTK_TYPE_TREE_MODEL, CtkTreeModelIface))

#define CTK_TYPE_TREE_ITER             (ctk_tree_iter_get_type ())
#define CTK_TYPE_TREE_PATH             (ctk_tree_path_get_type ())
#define CTK_TYPE_TREE_ROW_REFERENCE    (ctk_tree_row_reference_get_type ())

typedef struct _CtkTreeIter         CtkTreeIter;
typedef struct _CtkTreePath         CtkTreePath;
typedef struct _CtkTreeRowReference CtkTreeRowReference;
typedef struct _CtkTreeModel        CtkTreeModel; /* Dummy typedef */
typedef struct _CtkTreeModelIface   CtkTreeModelIface;

/**
 * CtkTreeModelForeachFunc:
 * @model: the #CtkTreeModel being iterated
 * @path: the current #CtkTreePath
 * @iter: the current #CtkTreeIter
 * @data: (closure): The user data passed to ctk_tree_model_foreach()
 *
 * Type of the callback passed to ctk_tree_model_foreach() to
 * iterate over the rows in a tree model.
 *
 * Returns: %TRUE to stop iterating, %FALSE to continue
 *
 */
typedef gboolean (* CtkTreeModelForeachFunc) (CtkTreeModel *model, CtkTreePath *path, CtkTreeIter *iter, gpointer data);

/**
 * CtkTreeModelFlags:
 * @CTK_TREE_MODEL_ITERS_PERSIST: iterators survive all signals
 *     emitted by the tree
 * @CTK_TREE_MODEL_LIST_ONLY: the model is a list only, and never
 *     has children
 *
 * These flags indicate various properties of a #CtkTreeModel.
 *
 * They are returned by ctk_tree_model_get_flags(), and must be
 * static for the lifetime of the object. A more complete description
 * of #CTK_TREE_MODEL_ITERS_PERSIST can be found in the overview of
 * this section.
 */
typedef enum
{
  CTK_TREE_MODEL_ITERS_PERSIST = 1 << 0,
  CTK_TREE_MODEL_LIST_ONLY = 1 << 1
} CtkTreeModelFlags;

/**
 * CtkTreeIter:
 * @stamp: a unique stamp to catch invalid iterators
 * @user_data: model-specific data
 * @user_data2: model-specific data
 * @user_data3: model-specific data
 *
 * The #CtkTreeIter is the primary structure
 * for accessing a #CtkTreeModel. Models are expected to put a unique
 * integer in the @stamp member, and put
 * model-specific data in the three @user_data
 * members.
 */
struct _CtkTreeIter
{
  gint stamp;
  gpointer user_data;
  gpointer user_data2;
  gpointer user_data3;
};

/**
 * CtkTreeModelIface:
 * @row_changed: Signal emitted when a row in the model has changed.
 * @row_inserted: Signal emitted when a new row has been inserted in
 *    the model.
 * @row_has_child_toggled: Signal emitted when a row has gotten the
 *    first child row or lost its last child row.
 * @row_deleted: Signal emitted when a row has been deleted.
 * @rows_reordered: Signal emitted when the children of a node in the
 *    CtkTreeModel have been reordered.
 * @get_flags: Get #CtkTreeModelFlags supported by this interface.
 * @get_n_columns: Get the number of columns supported by the model.
 * @get_column_type: Get the type of the column.
 * @get_iter: Sets iter to a valid iterator pointing to path.
 * @get_path: Gets a newly-created #CtkTreePath referenced by iter.
 * @get_value: Initializes and sets value to that at column.
 * @iter_next: Sets iter to point to the node following it at the
 *    current level.
 * @iter_previous: Sets iter to point to the previous node at the
 *    current level.
 * @iter_children: Sets iter to point to the first child of parent.
 * @iter_has_child: %TRUE if iter has children, %FALSE otherwise.
 * @iter_n_children: Gets the number of children that iter has.
 * @iter_nth_child: Sets iter to be the child of parent, using the
 *    given index.
 * @iter_parent: Sets iter to be the parent of child.
 * @ref_node: Lets the tree ref the node.
 * @unref_node: Lets the tree unref the node.
 */
struct _CtkTreeModelIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  /* Signals */
  void         (* row_changed)           (CtkTreeModel *tree_model,
					  CtkTreePath  *path,
					  CtkTreeIter  *iter);
  void         (* row_inserted)          (CtkTreeModel *tree_model,
					  CtkTreePath  *path,
					  CtkTreeIter  *iter);
  void         (* row_has_child_toggled) (CtkTreeModel *tree_model,
					  CtkTreePath  *path,
					  CtkTreeIter  *iter);
  void         (* row_deleted)           (CtkTreeModel *tree_model,
					  CtkTreePath  *path);
  void         (* rows_reordered)        (CtkTreeModel *tree_model,
					  CtkTreePath  *path,
					  CtkTreeIter  *iter,
					  gint         *new_order);

  /* Virtual Table */
  CtkTreeModelFlags (* get_flags)  (CtkTreeModel *tree_model);

  gint         (* get_n_columns)   (CtkTreeModel *tree_model);
  GType        (* get_column_type) (CtkTreeModel *tree_model,
				    gint          index_);
  gboolean     (* get_iter)        (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter,
				    CtkTreePath  *path);
  CtkTreePath *(* get_path)        (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter);
  void         (* get_value)       (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter,
				    gint          column,
				    GValue       *value);
  gboolean     (* iter_next)       (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter);
  gboolean     (* iter_previous)   (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter);
  gboolean     (* iter_children)   (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter,
				    CtkTreeIter  *parent);
  gboolean     (* iter_has_child)  (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter);
  gint         (* iter_n_children) (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter);
  gboolean     (* iter_nth_child)  (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter,
				    CtkTreeIter  *parent,
				    gint          n);
  gboolean     (* iter_parent)     (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter,
				    CtkTreeIter  *child);
  void         (* ref_node)        (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter);
  void         (* unref_node)      (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter);
};


/* CtkTreePath operations */
GDK_AVAILABLE_IN_ALL
CtkTreePath *ctk_tree_path_new              (void);
GDK_AVAILABLE_IN_ALL
CtkTreePath *ctk_tree_path_new_from_string  (const gchar       *path);
GDK_AVAILABLE_IN_ALL
CtkTreePath *ctk_tree_path_new_from_indices (gint               first_index,
					     ...);
GDK_AVAILABLE_IN_3_12
CtkTreePath *ctk_tree_path_new_from_indicesv (gint             *indices,
					      gsize             length);
GDK_AVAILABLE_IN_ALL
gchar       *ctk_tree_path_to_string        (CtkTreePath       *path);
GDK_AVAILABLE_IN_ALL
CtkTreePath *ctk_tree_path_new_first        (void);
GDK_AVAILABLE_IN_ALL
void         ctk_tree_path_append_index     (CtkTreePath       *path,
					     gint               index_);
GDK_AVAILABLE_IN_ALL
void         ctk_tree_path_prepend_index    (CtkTreePath       *path,
					     gint               index_);
GDK_AVAILABLE_IN_ALL
gint         ctk_tree_path_get_depth        (CtkTreePath       *path);
GDK_AVAILABLE_IN_ALL
gint        *ctk_tree_path_get_indices      (CtkTreePath       *path);

GDK_AVAILABLE_IN_ALL
gint        *ctk_tree_path_get_indices_with_depth (CtkTreePath *path,
						   gint        *depth);

GDK_AVAILABLE_IN_ALL
void         ctk_tree_path_free             (CtkTreePath       *path);
GDK_AVAILABLE_IN_ALL
CtkTreePath *ctk_tree_path_copy             (const CtkTreePath *path);
GDK_AVAILABLE_IN_ALL
GType        ctk_tree_path_get_type         (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
gint         ctk_tree_path_compare          (const CtkTreePath *a,
					     const CtkTreePath *b);
GDK_AVAILABLE_IN_ALL
void         ctk_tree_path_next             (CtkTreePath       *path);
GDK_AVAILABLE_IN_ALL
gboolean     ctk_tree_path_prev             (CtkTreePath       *path);
GDK_AVAILABLE_IN_ALL
gboolean     ctk_tree_path_up               (CtkTreePath       *path);
GDK_AVAILABLE_IN_ALL
void         ctk_tree_path_down             (CtkTreePath       *path);

GDK_AVAILABLE_IN_ALL
gboolean     ctk_tree_path_is_ancestor      (CtkTreePath       *path,
                                             CtkTreePath       *descendant);
GDK_AVAILABLE_IN_ALL
gboolean     ctk_tree_path_is_descendant    (CtkTreePath       *path,
                                             CtkTreePath       *ancestor);

/**
 * CtkTreeRowReference:
 *
 * A CtkTreeRowReference tracks model changes so that it always refers to the
 * same row (a #CtkTreePath refers to a position, not a fixed row). Create a
 * new CtkTreeRowReference with ctk_tree_row_reference_new().
 */

GDK_AVAILABLE_IN_ALL
GType                ctk_tree_row_reference_get_type (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkTreeRowReference *ctk_tree_row_reference_new       (CtkTreeModel        *model,
						       CtkTreePath         *path);
GDK_AVAILABLE_IN_ALL
CtkTreeRowReference *ctk_tree_row_reference_new_proxy (GObject             *proxy,
						       CtkTreeModel        *model,
						       CtkTreePath         *path);
GDK_AVAILABLE_IN_ALL
CtkTreePath         *ctk_tree_row_reference_get_path  (CtkTreeRowReference *reference);
GDK_AVAILABLE_IN_ALL
CtkTreeModel        *ctk_tree_row_reference_get_model (CtkTreeRowReference *reference);
GDK_AVAILABLE_IN_ALL
gboolean             ctk_tree_row_reference_valid     (CtkTreeRowReference *reference);
GDK_AVAILABLE_IN_ALL
CtkTreeRowReference *ctk_tree_row_reference_copy      (CtkTreeRowReference *reference);
GDK_AVAILABLE_IN_ALL
void                 ctk_tree_row_reference_free      (CtkTreeRowReference *reference);
/* These two functions are only needed if you created the row reference with a
 * proxy object */
GDK_AVAILABLE_IN_ALL
void                 ctk_tree_row_reference_inserted  (GObject     *proxy,
						       CtkTreePath *path);
GDK_AVAILABLE_IN_ALL
void                 ctk_tree_row_reference_deleted   (GObject     *proxy,
						       CtkTreePath *path);
GDK_AVAILABLE_IN_ALL
void                 ctk_tree_row_reference_reordered (GObject     *proxy,
						       CtkTreePath *path,
						       CtkTreeIter *iter,
						       gint        *new_order);

/* CtkTreeIter operations */
GDK_AVAILABLE_IN_ALL
CtkTreeIter *     ctk_tree_iter_copy             (CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
void              ctk_tree_iter_free             (CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
GType             ctk_tree_iter_get_type         (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GType             ctk_tree_model_get_type        (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkTreeModelFlags ctk_tree_model_get_flags       (CtkTreeModel *tree_model);
GDK_AVAILABLE_IN_ALL
gint              ctk_tree_model_get_n_columns   (CtkTreeModel *tree_model);
GDK_AVAILABLE_IN_ALL
GType             ctk_tree_model_get_column_type (CtkTreeModel *tree_model,
						  gint          index_);


/* Iterator movement */
GDK_AVAILABLE_IN_ALL
gboolean          ctk_tree_model_get_iter        (CtkTreeModel *tree_model,
						  CtkTreeIter  *iter,
						  CtkTreePath  *path);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_tree_model_get_iter_from_string (CtkTreeModel *tree_model,
						       CtkTreeIter  *iter,
						       const gchar  *path_string);
GDK_AVAILABLE_IN_ALL
gchar *           ctk_tree_model_get_string_from_iter (CtkTreeModel *tree_model,
                                                       CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_tree_model_get_iter_first  (CtkTreeModel *tree_model,
						  CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
CtkTreePath *     ctk_tree_model_get_path        (CtkTreeModel *tree_model,
						  CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
void              ctk_tree_model_get_value       (CtkTreeModel *tree_model,
						  CtkTreeIter  *iter,
						  gint          column,
						  GValue       *value);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_tree_model_iter_previous   (CtkTreeModel *tree_model,
						  CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_tree_model_iter_next       (CtkTreeModel *tree_model,
						  CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_tree_model_iter_children   (CtkTreeModel *tree_model,
						  CtkTreeIter  *iter,
						  CtkTreeIter  *parent);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_tree_model_iter_has_child  (CtkTreeModel *tree_model,
						  CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
gint              ctk_tree_model_iter_n_children (CtkTreeModel *tree_model,
						  CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_tree_model_iter_nth_child  (CtkTreeModel *tree_model,
						  CtkTreeIter  *iter,
						  CtkTreeIter  *parent,
						  gint          n);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_tree_model_iter_parent     (CtkTreeModel *tree_model,
						  CtkTreeIter  *iter,
						  CtkTreeIter  *child);
GDK_AVAILABLE_IN_ALL
void              ctk_tree_model_ref_node        (CtkTreeModel *tree_model,
						  CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
void              ctk_tree_model_unref_node      (CtkTreeModel *tree_model,
						  CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
void              ctk_tree_model_get             (CtkTreeModel *tree_model,
						  CtkTreeIter  *iter,
						  ...);
GDK_AVAILABLE_IN_ALL
void              ctk_tree_model_get_valist      (CtkTreeModel *tree_model,
						  CtkTreeIter  *iter,
						  va_list       var_args);


GDK_AVAILABLE_IN_ALL
void              ctk_tree_model_foreach         (CtkTreeModel            *model,
						  CtkTreeModelForeachFunc  func,
						  gpointer                 user_data);

/* Signals */
GDK_AVAILABLE_IN_ALL
void ctk_tree_model_row_changed           (CtkTreeModel *tree_model,
					   CtkTreePath  *path,
					   CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
void ctk_tree_model_row_inserted          (CtkTreeModel *tree_model,
					   CtkTreePath  *path,
					   CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
void ctk_tree_model_row_has_child_toggled (CtkTreeModel *tree_model,
					   CtkTreePath  *path,
					   CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
void ctk_tree_model_row_deleted           (CtkTreeModel *tree_model,
					   CtkTreePath  *path);
GDK_AVAILABLE_IN_ALL
void ctk_tree_model_rows_reordered        (CtkTreeModel *tree_model,
					   CtkTreePath  *path,
					   CtkTreeIter  *iter,
					   gint         *new_order);
GDK_AVAILABLE_IN_3_10
void ctk_tree_model_rows_reordered_with_length (CtkTreeModel *tree_model,
						CtkTreePath  *path,
						CtkTreeIter  *iter,
						gint         *new_order,
						gint          length);

G_END_DECLS

#endif /* __CTK_TREE_MODEL_H__ */
