/* gtktreedatalist.h
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

#ifndef __CTK_TREE_DATA_LIST_H__
#define __CTK_TREE_DATA_LIST_H__

#include <gtk/gtktreemodel.h>
#include <gtk/gtktreesortable.h>

typedef struct _GtkTreeDataList GtkTreeDataList;
struct _GtkTreeDataList
{
  GtkTreeDataList *next;

  union {
    gint	   v_int;
    gint8          v_char;
    guint8         v_uchar;
    guint	   v_uint;
    glong	   v_long;
    gulong	   v_ulong;
    gint64	   v_int64;
    guint64        v_uint64;
    gfloat	   v_float;
    gdouble        v_double;
    gpointer	   v_pointer;
  } data;
};

typedef struct _GtkTreeDataSortHeader
{
  gint sort_column_id;
  GtkTreeIterCompareFunc func;
  gpointer data;
  GDestroyNotify destroy;
} GtkTreeDataSortHeader;

GtkTreeDataList *_ctk_tree_data_list_alloc          (void);
void             _ctk_tree_data_list_free           (GtkTreeDataList *list,
						     GType           *column_headers);
gboolean         _ctk_tree_data_list_check_type     (GType            type);
void             _ctk_tree_data_list_node_to_value  (GtkTreeDataList *list,
						     GType            type,
						     GValue          *value);
void             _ctk_tree_data_list_value_to_node  (GtkTreeDataList *list,
						     GValue          *value);

GtkTreeDataList *_ctk_tree_data_list_node_copy      (GtkTreeDataList *list,
                                                     GType            type);

/* Header code */
gint                   _ctk_tree_data_list_compare_func (GtkTreeModel *model,
							 GtkTreeIter  *a,
							 GtkTreeIter  *b,
							 gpointer      user_data);
GList *                _ctk_tree_data_list_header_new  (gint          n_columns,
							GType        *types);
void                   _ctk_tree_data_list_header_free (GList        *header_list);
GtkTreeDataSortHeader *_ctk_tree_data_list_get_header  (GList        *header_list,
							gint          sort_column_id);
GList                 *_ctk_tree_data_list_set_header  (GList                  *header_list,
							gint                    sort_column_id,
							GtkTreeIterCompareFunc  func,
							gpointer                data,
							GDestroyNotify          destroy);

#endif /* __CTK_TREE_DATA_LIST_H__ */