/* ctktreemodelsort.h
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

#ifndef __CTK_TREE_MODEL_SORT_H__
#define __CTK_TREE_MODEL_SORT_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <ctk/ctktreemodel.h>
#include <ctk/ctktreesortable.h>

G_BEGIN_DECLS

#define CTK_TYPE_TREE_MODEL_SORT			(ctk_tree_model_sort_get_type ())
#define CTK_TREE_MODEL_SORT(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TREE_MODEL_SORT, GtkTreeModelSort))
#define CTK_TREE_MODEL_SORT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TREE_MODEL_SORT, GtkTreeModelSortClass))
#define CTK_IS_TREE_MODEL_SORT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TREE_MODEL_SORT))
#define CTK_IS_TREE_MODEL_SORT_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TREE_MODEL_SORT))
#define CTK_TREE_MODEL_SORT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TREE_MODEL_SORT, GtkTreeModelSortClass))

typedef struct _GtkTreeModelSort        GtkTreeModelSort;
typedef struct _GtkTreeModelSortClass   GtkTreeModelSortClass;
typedef struct _GtkTreeModelSortPrivate GtkTreeModelSortPrivate;

struct _GtkTreeModelSort
{
  GObject parent;

  /* < private > */
  GtkTreeModelSortPrivate *priv;
};

struct _GtkTreeModelSortClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType         ctk_tree_model_sort_get_type                   (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkTreeModel *ctk_tree_model_sort_new_with_model             (GtkTreeModel     *child_model);

GDK_AVAILABLE_IN_ALL
GtkTreeModel *ctk_tree_model_sort_get_model                  (GtkTreeModelSort *tree_model);
GDK_AVAILABLE_IN_ALL
GtkTreePath  *ctk_tree_model_sort_convert_child_path_to_path (GtkTreeModelSort *tree_model_sort,
							      GtkTreePath      *child_path);
GDK_AVAILABLE_IN_ALL
gboolean      ctk_tree_model_sort_convert_child_iter_to_iter (GtkTreeModelSort *tree_model_sort,
							      GtkTreeIter      *sort_iter,
							      GtkTreeIter      *child_iter);
GDK_AVAILABLE_IN_ALL
GtkTreePath  *ctk_tree_model_sort_convert_path_to_child_path (GtkTreeModelSort *tree_model_sort,
							      GtkTreePath      *sorted_path);
GDK_AVAILABLE_IN_ALL
void          ctk_tree_model_sort_convert_iter_to_child_iter (GtkTreeModelSort *tree_model_sort,
							      GtkTreeIter      *child_iter,
							      GtkTreeIter      *sorted_iter);
GDK_AVAILABLE_IN_ALL
void          ctk_tree_model_sort_reset_default_sort_func    (GtkTreeModelSort *tree_model_sort);
GDK_AVAILABLE_IN_ALL
void          ctk_tree_model_sort_clear_cache                (GtkTreeModelSort *tree_model_sort);
GDK_AVAILABLE_IN_ALL
gboolean      ctk_tree_model_sort_iter_is_valid              (GtkTreeModelSort *tree_model_sort,
                                                              GtkTreeIter      *iter);


G_END_DECLS

#endif /* __CTK_TREE_MODEL_SORT_H__ */
