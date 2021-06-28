/* ctktreeviewcolumn.h
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

#ifndef __CTK_TREE_VIEW_COLUMN_H__
#define __CTK_TREE_VIEW_COLUMN_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcellrenderer.h>
#include <ctk/ctktreemodel.h>
#include <ctk/ctktreesortable.h>
#include <ctk/ctkcellarea.h>


G_BEGIN_DECLS


#define CTK_TYPE_TREE_VIEW_COLUMN	     (ctk_tree_view_column_get_type ())
#define CTK_TREE_VIEW_COLUMN(obj)	     (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TREE_VIEW_COLUMN, CtkTreeViewColumn))
#define CTK_TREE_VIEW_COLUMN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TREE_VIEW_COLUMN, CtkTreeViewColumnClass))
#define CTK_IS_TREE_VIEW_COLUMN(obj)	     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TREE_VIEW_COLUMN))
#define CTK_IS_TREE_VIEW_COLUMN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TREE_VIEW_COLUMN))
#define CTK_TREE_VIEW_COLUMN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TREE_VIEW_COLUMN, CtkTreeViewColumnClass))

typedef struct _CtkTreeViewColumn        CtkTreeViewColumn;
typedef struct _CtkTreeViewColumnClass   CtkTreeViewColumnClass;
typedef struct _CtkTreeViewColumnPrivate CtkTreeViewColumnPrivate;

/**
 * CtkTreeViewColumnSizing:
 * @CTK_TREE_VIEW_COLUMN_GROW_ONLY: Columns only get bigger in reaction to changes in the model
 * @CTK_TREE_VIEW_COLUMN_AUTOSIZE: Columns resize to be the optimal size everytime the model changes.
 * @CTK_TREE_VIEW_COLUMN_FIXED: Columns are a fixed numbers of pixels wide.
 *
 * The sizing method the column uses to determine its width.  Please note
 * that @CTK_TREE_VIEW_COLUMN_AUTOSIZE are inefficient for large views, and
 * can make columns appear choppy.
 */
typedef enum
{
  CTK_TREE_VIEW_COLUMN_GROW_ONLY,
  CTK_TREE_VIEW_COLUMN_AUTOSIZE,
  CTK_TREE_VIEW_COLUMN_FIXED
} CtkTreeViewColumnSizing;

/**
 * CtkTreeCellDataFunc:
 * @tree_column: A #CtkTreeViewColumn
 * @cell: The #CtkCellRenderer that is being rendered by @tree_column
 * @tree_model: The #CtkTreeModel being rendered
 * @iter: A #CtkTreeIter of the current row rendered
 * @data: (closure): user data
 *
 * A function to set the properties of a cell instead of just using the
 * straight mapping between the cell and the model.  This is useful for
 * customizing the cell renderer.  For example, a function might get an
 * integer from the @tree_model, and render it to the “text” attribute of
 * “cell” by converting it to its written equivalent.  This is set by
 * calling ctk_tree_view_column_set_cell_data_func()
 */
typedef void (* CtkTreeCellDataFunc) (CtkTreeViewColumn *tree_column,
				      CtkCellRenderer   *cell,
				      CtkTreeModel      *tree_model,
				      CtkTreeIter       *iter,
				      gpointer           data);


struct _CtkTreeViewColumn
{
  GInitiallyUnowned parent_instance;

  CtkTreeViewColumnPrivate *priv;
};

struct _CtkTreeViewColumnClass
{
  GInitiallyUnownedClass parent_class;

  void (*clicked) (CtkTreeViewColumn *tree_column);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType                   ctk_tree_view_column_get_type            (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkTreeViewColumn      *ctk_tree_view_column_new                 (void);
GDK_AVAILABLE_IN_ALL
CtkTreeViewColumn      *ctk_tree_view_column_new_with_area       (CtkCellArea             *area);
GDK_AVAILABLE_IN_ALL
CtkTreeViewColumn      *ctk_tree_view_column_new_with_attributes (const gchar             *title,
								  CtkCellRenderer         *cell,
								  ...) G_GNUC_NULL_TERMINATED;
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_pack_start          (CtkTreeViewColumn       *tree_column,
								  CtkCellRenderer         *cell,
								  gboolean                 expand);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_pack_end            (CtkTreeViewColumn       *tree_column,
								  CtkCellRenderer         *cell,
								  gboolean                 expand);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_clear               (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_add_attribute       (CtkTreeViewColumn       *tree_column,
								  CtkCellRenderer         *cell_renderer,
								  const gchar             *attribute,
								  gint                     column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_attributes      (CtkTreeViewColumn       *tree_column,
								  CtkCellRenderer         *cell_renderer,
								  ...) G_GNUC_NULL_TERMINATED;
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_cell_data_func  (CtkTreeViewColumn       *tree_column,
								  CtkCellRenderer         *cell_renderer,
								  CtkTreeCellDataFunc      func,
								  gpointer                 func_data,
								  GDestroyNotify           destroy);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_clear_attributes    (CtkTreeViewColumn       *tree_column,
								  CtkCellRenderer         *cell_renderer);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_spacing         (CtkTreeViewColumn       *tree_column,
								  gint                     spacing);
GDK_AVAILABLE_IN_ALL
gint                    ctk_tree_view_column_get_spacing         (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_visible         (CtkTreeViewColumn       *tree_column,
								  gboolean                 visible);
GDK_AVAILABLE_IN_ALL
gboolean                ctk_tree_view_column_get_visible         (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_resizable       (CtkTreeViewColumn       *tree_column,
								  gboolean                 resizable);
GDK_AVAILABLE_IN_ALL
gboolean                ctk_tree_view_column_get_resizable       (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_sizing          (CtkTreeViewColumn       *tree_column,
								  CtkTreeViewColumnSizing  type);
GDK_AVAILABLE_IN_ALL
CtkTreeViewColumnSizing ctk_tree_view_column_get_sizing          (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_3_2
gint                    ctk_tree_view_column_get_x_offset        (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
gint                    ctk_tree_view_column_get_width           (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
gint                    ctk_tree_view_column_get_fixed_width     (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_fixed_width     (CtkTreeViewColumn       *tree_column,
								  gint                     fixed_width);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_min_width       (CtkTreeViewColumn       *tree_column,
								  gint                     min_width);
GDK_AVAILABLE_IN_ALL
gint                    ctk_tree_view_column_get_min_width       (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_max_width       (CtkTreeViewColumn       *tree_column,
								  gint                     max_width);
GDK_AVAILABLE_IN_ALL
gint                    ctk_tree_view_column_get_max_width       (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_clicked             (CtkTreeViewColumn       *tree_column);



/* Options for manipulating the column headers
 */
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_title           (CtkTreeViewColumn       *tree_column,
								  const gchar             *title);
GDK_AVAILABLE_IN_ALL
const gchar *           ctk_tree_view_column_get_title           (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_expand          (CtkTreeViewColumn       *tree_column,
								  gboolean                 expand);
GDK_AVAILABLE_IN_ALL
gboolean                ctk_tree_view_column_get_expand          (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_clickable       (CtkTreeViewColumn       *tree_column,
								  gboolean                 clickable);
GDK_AVAILABLE_IN_ALL
gboolean                ctk_tree_view_column_get_clickable       (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_widget          (CtkTreeViewColumn       *tree_column,
								  CtkWidget               *widget);
GDK_AVAILABLE_IN_ALL
CtkWidget              *ctk_tree_view_column_get_widget          (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_alignment       (CtkTreeViewColumn       *tree_column,
								  gfloat                   xalign);
GDK_AVAILABLE_IN_ALL
gfloat                  ctk_tree_view_column_get_alignment       (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_reorderable     (CtkTreeViewColumn       *tree_column,
								  gboolean                 reorderable);
GDK_AVAILABLE_IN_ALL
gboolean                ctk_tree_view_column_get_reorderable     (CtkTreeViewColumn       *tree_column);



/* You probably only want to use ctk_tree_view_column_set_sort_column_id.  The
 * other sorting functions exist primarily to let others do their own custom sorting.
 */
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_sort_column_id  (CtkTreeViewColumn       *tree_column,
								  gint                     sort_column_id);
GDK_AVAILABLE_IN_ALL
gint                    ctk_tree_view_column_get_sort_column_id  (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_sort_indicator  (CtkTreeViewColumn       *tree_column,
								  gboolean                 setting);
GDK_AVAILABLE_IN_ALL
gboolean                ctk_tree_view_column_get_sort_indicator  (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_set_sort_order      (CtkTreeViewColumn       *tree_column,
								  CtkSortType              order);
GDK_AVAILABLE_IN_ALL
CtkSortType             ctk_tree_view_column_get_sort_order      (CtkTreeViewColumn       *tree_column);


/* These functions are meant primarily for interaction between the CtkTreeView and the column.
 */
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_cell_set_cell_data  (CtkTreeViewColumn       *tree_column,
								  CtkTreeModel            *tree_model,
								  CtkTreeIter             *iter,
								  gboolean                 is_expander,
								  gboolean                 is_expanded);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_cell_get_size       (CtkTreeViewColumn       *tree_column,
								  const CdkRectangle      *cell_area,
								  gint                    *x_offset,
								  gint                    *y_offset,
								  gint                    *width,
								  gint                    *height);
GDK_AVAILABLE_IN_ALL
gboolean                ctk_tree_view_column_cell_is_visible     (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_focus_cell          (CtkTreeViewColumn       *tree_column,
								  CtkCellRenderer         *cell);
GDK_AVAILABLE_IN_ALL
gboolean                ctk_tree_view_column_cell_get_position   (CtkTreeViewColumn       *tree_column,
					                          CtkCellRenderer         *cell_renderer,
					                          gint                    *x_offset,
					                          gint                    *width);
GDK_AVAILABLE_IN_ALL
void                    ctk_tree_view_column_queue_resize        (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
CtkWidget              *ctk_tree_view_column_get_tree_view       (CtkTreeViewColumn       *tree_column);
GDK_AVAILABLE_IN_ALL
CtkWidget              *ctk_tree_view_column_get_button          (CtkTreeViewColumn       *tree_column);


G_END_DECLS


#endif /* __CTK_TREE_VIEW_COLUMN_H__ */
