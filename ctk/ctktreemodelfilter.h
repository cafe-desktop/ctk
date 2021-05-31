/* ctktreemodelfilter.h
 * Copyright (C) 2000,2001  Red Hat, Inc., Jonathan Blandford <jrb@redhat.com>
 * Copyright (C) 2001-2003  Kristian Rietveld <kris@ctk.org>
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

#ifndef __CTK_TREE_MODEL_FILTER_H__
#define __CTK_TREE_MODEL_FILTER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctktreemodel.h>

G_BEGIN_DECLS

#define CTK_TYPE_TREE_MODEL_FILTER              (ctk_tree_model_filter_get_type ())
#define CTK_TREE_MODEL_FILTER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TREE_MODEL_FILTER, CtkTreeModelFilter))
#define CTK_TREE_MODEL_FILTER_CLASS(vtable)     (G_TYPE_CHECK_CLASS_CAST ((vtable), CTK_TYPE_TREE_MODEL_FILTER, CtkTreeModelFilterClass))
#define CTK_IS_TREE_MODEL_FILTER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TREE_MODEL_FILTER))
#define CTK_IS_TREE_MODEL_FILTER_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), CTK_TYPE_TREE_MODEL_FILTER))
#define CTK_TREE_MODEL_FILTER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TREE_MODEL_FILTER, CtkTreeModelFilterClass))

/**
 * CtkTreeModelFilterVisibleFunc:
 * @model: the child model of the #CtkTreeModelFilter
 * @iter: a #CtkTreeIter pointing to the row in @model whose visibility
 *   is determined
 * @data: (closure): user data given to ctk_tree_model_filter_set_visible_func()
 *
 * A function which decides whether the row indicated by @iter is visible.
 *
 * Returns: Whether the row indicated by @iter is visible.
 */
typedef gboolean (* CtkTreeModelFilterVisibleFunc) (CtkTreeModel *model,
                                                    CtkTreeIter  *iter,
                                                    gpointer      data);

/**
 * CtkTreeModelFilterModifyFunc:
 * @model: the #CtkTreeModelFilter
 * @iter: a #CtkTreeIter pointing to the row whose display values are determined
 * @value: (out caller-allocates): A #GValue which is already initialized for
 *  with the correct type for the column @column.
 * @column: the column whose display value is determined
 * @data: (closure): user data given to ctk_tree_model_filter_set_modify_func()
 *
 * A function which calculates display values from raw values in the model.
 * It must fill @value with the display value for the column @column in the
 * row indicated by @iter.
 *
 * Since this function is called for each data access, it’s not a
 * particularly efficient operation.
 */

typedef void (* CtkTreeModelFilterModifyFunc) (CtkTreeModel *model,
                                               CtkTreeIter  *iter,
                                               GValue       *value,
                                               gint          column,
                                               gpointer      data);

typedef struct _CtkTreeModelFilter          CtkTreeModelFilter;
typedef struct _CtkTreeModelFilterClass     CtkTreeModelFilterClass;
typedef struct _CtkTreeModelFilterPrivate   CtkTreeModelFilterPrivate;

struct _CtkTreeModelFilter
{
  GObject parent;

  /*< private >*/
  CtkTreeModelFilterPrivate *priv;
};

struct _CtkTreeModelFilterClass
{
  GObjectClass parent_class;

  gboolean (* visible) (CtkTreeModelFilter *self,
                        CtkTreeModel       *child_model,
                        CtkTreeIter        *iter);
  void (* modify) (CtkTreeModelFilter *self,
                   CtkTreeModel       *child_model,
                   CtkTreeIter        *iter,
                   GValue             *value,
                   gint                column);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

/* base */
GDK_AVAILABLE_IN_ALL
GType         ctk_tree_model_filter_get_type                   (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkTreeModel *ctk_tree_model_filter_new                        (CtkTreeModel                 *child_model,
                                                                CtkTreePath                  *root);
GDK_AVAILABLE_IN_ALL
void          ctk_tree_model_filter_set_visible_func           (CtkTreeModelFilter           *filter,
                                                                CtkTreeModelFilterVisibleFunc func,
                                                                gpointer                      data,
                                                                GDestroyNotify                destroy);
GDK_AVAILABLE_IN_ALL
void          ctk_tree_model_filter_set_modify_func            (CtkTreeModelFilter           *filter,
                                                                gint                          n_columns,
                                                                GType                        *types,
                                                                CtkTreeModelFilterModifyFunc  func,
                                                                gpointer                      data,
                                                                GDestroyNotify                destroy);
GDK_AVAILABLE_IN_ALL
void          ctk_tree_model_filter_set_visible_column         (CtkTreeModelFilter           *filter,
                                                                gint                          column);

GDK_AVAILABLE_IN_ALL
CtkTreeModel *ctk_tree_model_filter_get_model                  (CtkTreeModelFilter           *filter);

/* conversion */
GDK_AVAILABLE_IN_ALL
gboolean      ctk_tree_model_filter_convert_child_iter_to_iter (CtkTreeModelFilter           *filter,
                                                                CtkTreeIter                  *filter_iter,
                                                                CtkTreeIter                  *child_iter);
GDK_AVAILABLE_IN_ALL
void          ctk_tree_model_filter_convert_iter_to_child_iter (CtkTreeModelFilter           *filter,
                                                                CtkTreeIter                  *child_iter,
                                                                CtkTreeIter                  *filter_iter);
GDK_AVAILABLE_IN_ALL
CtkTreePath  *ctk_tree_model_filter_convert_child_path_to_path (CtkTreeModelFilter           *filter,
                                                                CtkTreePath                  *child_path);
GDK_AVAILABLE_IN_ALL
CtkTreePath  *ctk_tree_model_filter_convert_path_to_child_path (CtkTreeModelFilter           *filter,
                                                                CtkTreePath                  *filter_path);

/* extras */
GDK_AVAILABLE_IN_ALL
void          ctk_tree_model_filter_refilter                   (CtkTreeModelFilter           *filter);
GDK_AVAILABLE_IN_ALL
void          ctk_tree_model_filter_clear_cache                (CtkTreeModelFilter           *filter);

G_END_DECLS

#endif /* __CTK_TREE_MODEL_FILTER_H__ */
