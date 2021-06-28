/* ctktreestore.h
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

#ifndef __CTK_TREE_STORE_H__
#define __CTK_TREE_STORE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cdk/cdk.h>
#include <ctk/ctktreemodel.h>
#include <ctk/ctktreesortable.h>
#include <stdarg.h>


G_BEGIN_DECLS


#define CTK_TYPE_TREE_STORE			(ctk_tree_store_get_type ())
#define CTK_TREE_STORE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TREE_STORE, CtkTreeStore))
#define CTK_TREE_STORE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TREE_STORE, CtkTreeStoreClass))
#define CTK_IS_TREE_STORE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TREE_STORE))
#define CTK_IS_TREE_STORE_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TREE_STORE))
#define CTK_TREE_STORE_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TREE_STORE, CtkTreeStoreClass))

typedef struct _CtkTreeStore        CtkTreeStore;
typedef struct _CtkTreeStoreClass   CtkTreeStoreClass;
typedef struct _CtkTreeStorePrivate CtkTreeStorePrivate;

struct _CtkTreeStore
{
  GObject parent;

  CtkTreeStorePrivate *priv;
};

struct _CtkTreeStoreClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


CDK_AVAILABLE_IN_ALL
GType         ctk_tree_store_get_type         (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkTreeStore *ctk_tree_store_new              (gint          n_columns,
					       ...);
CDK_AVAILABLE_IN_ALL
CtkTreeStore *ctk_tree_store_newv             (gint          n_columns,
					       GType        *types);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_set_column_types (CtkTreeStore *tree_store,
					       gint          n_columns,
					       GType        *types);

/* NOTE: use ctk_tree_model_get to get values from a CtkTreeStore */

CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_set_value        (CtkTreeStore *tree_store,
					       CtkTreeIter  *iter,
					       gint          column,
					       GValue       *value);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_set              (CtkTreeStore *tree_store,
					       CtkTreeIter  *iter,
					       ...);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_set_valuesv      (CtkTreeStore *tree_store,
					       CtkTreeIter  *iter,
					       gint         *columns,
					       GValue       *values,
					       gint          n_values);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_set_valist       (CtkTreeStore *tree_store,
					       CtkTreeIter  *iter,
					       va_list       var_args);
CDK_AVAILABLE_IN_ALL
gboolean      ctk_tree_store_remove           (CtkTreeStore *tree_store,
					       CtkTreeIter  *iter);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_insert           (CtkTreeStore *tree_store,
					       CtkTreeIter  *iter,
					       CtkTreeIter  *parent,
					       gint          position);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_insert_before    (CtkTreeStore *tree_store,
					       CtkTreeIter  *iter,
					       CtkTreeIter  *parent,
					       CtkTreeIter  *sibling);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_insert_after     (CtkTreeStore *tree_store,
					       CtkTreeIter  *iter,
					       CtkTreeIter  *parent,
					       CtkTreeIter  *sibling);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_insert_with_values (CtkTreeStore *tree_store,
						 CtkTreeIter  *iter,
						 CtkTreeIter  *parent,
						 gint          position,
						 ...);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_insert_with_valuesv (CtkTreeStore *tree_store,
						  CtkTreeIter  *iter,
						  CtkTreeIter  *parent,
						  gint          position,
						  gint         *columns,
						  GValue       *values,
						  gint          n_values);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_prepend          (CtkTreeStore *tree_store,
					       CtkTreeIter  *iter,
					       CtkTreeIter  *parent);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_append           (CtkTreeStore *tree_store,
					       CtkTreeIter  *iter,
					       CtkTreeIter  *parent);
CDK_AVAILABLE_IN_ALL
gboolean      ctk_tree_store_is_ancestor      (CtkTreeStore *tree_store,
					       CtkTreeIter  *iter,
					       CtkTreeIter  *descendant);
CDK_AVAILABLE_IN_ALL
gint          ctk_tree_store_iter_depth       (CtkTreeStore *tree_store,
					       CtkTreeIter  *iter);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_clear            (CtkTreeStore *tree_store);
CDK_AVAILABLE_IN_ALL
gboolean      ctk_tree_store_iter_is_valid    (CtkTreeStore *tree_store,
                                               CtkTreeIter  *iter);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_reorder          (CtkTreeStore *tree_store,
                                               CtkTreeIter  *parent,
                                               gint         *new_order);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_swap             (CtkTreeStore *tree_store,
                                               CtkTreeIter  *a,
                                               CtkTreeIter  *b);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_move_before      (CtkTreeStore *tree_store,
                                               CtkTreeIter  *iter,
                                               CtkTreeIter  *position);
CDK_AVAILABLE_IN_ALL
void          ctk_tree_store_move_after       (CtkTreeStore *tree_store,
                                               CtkTreeIter  *iter,
                                               CtkTreeIter  *position);


G_END_DECLS


#endif /* __CTK_TREE_STORE_H__ */
