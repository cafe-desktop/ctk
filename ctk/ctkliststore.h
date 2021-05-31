/* ctkliststore.h
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

#ifndef __CTK_LIST_STORE_H__
#define __CTK_LIST_STORE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <ctk/ctktreemodel.h>
#include <ctk/ctktreesortable.h>


G_BEGIN_DECLS


#define CTK_TYPE_LIST_STORE	       (ctk_list_store_get_type ())
#define CTK_LIST_STORE(obj)	       (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_LIST_STORE, CtkListStore))
#define CTK_LIST_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_LIST_STORE, CtkListStoreClass))
#define CTK_IS_LIST_STORE(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_LIST_STORE))
#define CTK_IS_LIST_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_LIST_STORE))
#define CTK_LIST_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_LIST_STORE, CtkListStoreClass))

typedef struct _CtkListStore              CtkListStore;
typedef struct _CtkListStorePrivate       CtkListStorePrivate;
typedef struct _CtkListStoreClass         CtkListStoreClass;

struct _CtkListStore
{
  GObject parent;

  /*< private >*/
  CtkListStorePrivate *priv;
};

struct _CtkListStoreClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType         ctk_list_store_get_type         (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkListStore *ctk_list_store_new              (gint          n_columns,
					       ...);
GDK_AVAILABLE_IN_ALL
CtkListStore *ctk_list_store_newv             (gint          n_columns,
					       GType        *types);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_set_column_types (CtkListStore *list_store,
					       gint          n_columns,
					       GType        *types);

/* NOTE: use ctk_tree_model_get to get values from a CtkListStore */

GDK_AVAILABLE_IN_ALL
void          ctk_list_store_set_value        (CtkListStore *list_store,
					       CtkTreeIter  *iter,
					       gint          column,
					       GValue       *value);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_set              (CtkListStore *list_store,
					       CtkTreeIter  *iter,
					       ...);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_set_valuesv      (CtkListStore *list_store,
					       CtkTreeIter  *iter,
					       gint         *columns,
					       GValue       *values,
					       gint          n_values);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_set_valist       (CtkListStore *list_store,
					       CtkTreeIter  *iter,
					       va_list       var_args);
GDK_AVAILABLE_IN_ALL
gboolean      ctk_list_store_remove           (CtkListStore *list_store,
					       CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_insert           (CtkListStore *list_store,
					       CtkTreeIter  *iter,
					       gint          position);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_insert_before    (CtkListStore *list_store,
					       CtkTreeIter  *iter,
					       CtkTreeIter  *sibling);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_insert_after     (CtkListStore *list_store,
					       CtkTreeIter  *iter,
					       CtkTreeIter  *sibling);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_insert_with_values  (CtkListStore *list_store,
						  CtkTreeIter  *iter,
						  gint          position,
						  ...);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_insert_with_valuesv (CtkListStore *list_store,
						  CtkTreeIter  *iter,
						  gint          position,
						  gint         *columns,
						  GValue       *values,
						  gint          n_values);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_prepend          (CtkListStore *list_store,
					       CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_append           (CtkListStore *list_store,
					       CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_clear            (CtkListStore *list_store);
GDK_AVAILABLE_IN_ALL
gboolean      ctk_list_store_iter_is_valid    (CtkListStore *list_store,
                                               CtkTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_reorder          (CtkListStore *store,
                                               gint         *new_order);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_swap             (CtkListStore *store,
                                               CtkTreeIter  *a,
                                               CtkTreeIter  *b);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_move_after       (CtkListStore *store,
                                               CtkTreeIter  *iter,
                                               CtkTreeIter  *position);
GDK_AVAILABLE_IN_ALL
void          ctk_list_store_move_before      (CtkListStore *store,
                                               CtkTreeIter  *iter,
                                               CtkTreeIter  *position);


G_END_DECLS


#endif /* __CTK_LIST_STORE_H__ */
