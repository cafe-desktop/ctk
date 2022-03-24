/* ctkcelllayout.h
 * Copyright (C) 2003  Kristian Rietveld  <kris@gtk.org>
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

#ifndef __CTK_CELL_LAYOUT_H__
#define __CTK_CELL_LAYOUT_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcellrenderer.h>
#include <ctk/ctkcellarea.h>
#include <ctk/ctkbuildable.h>
#include <ctk/ctkbuilder.h>

G_BEGIN_DECLS

#define CTK_TYPE_CELL_LAYOUT            (ctk_cell_layout_get_type ())
#define CTK_CELL_LAYOUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_LAYOUT, CtkCellLayout))
#define CTK_IS_CELL_LAYOUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_LAYOUT))
#define CTK_CELL_LAYOUT_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CTK_TYPE_CELL_LAYOUT, CtkCellLayoutIface))

typedef struct _CtkCellLayout           CtkCellLayout; /* dummy typedef */
typedef struct _CtkCellLayoutIface      CtkCellLayoutIface;

/* keep in sync with CtkTreeCellDataFunc */
/**
 * CtkCellLayoutDataFunc:
 * @cell_layout: a #CtkCellLayout
 * @cell: the cell renderer whose value is to be set
 * @tree_model: the model
 * @iter: a #CtkTreeIter indicating the row to set the value for
 * @data: (closure): user data passed to ctk_cell_layout_set_cell_data_func()
 *
 * A function which should set the value of @cell_layout’s cell renderer(s)
 * as appropriate. 
 */
typedef void (* CtkCellLayoutDataFunc) (CtkCellLayout   *cell_layout,
                                        CtkCellRenderer *cell,
                                        CtkTreeModel    *tree_model,
                                        CtkTreeIter     *iter,
                                        gpointer         data);

/**
 * CtkCellLayoutIface:
 * @pack_start: Packs the cell into the beginning of cell_layout.
 * @pack_end: Adds the cell to the end of cell_layout.
 * @clear: Unsets all the mappings on all renderers on cell_layout and
 *    removes all renderers from cell_layout.
 * @add_attribute: Adds an attribute mapping to the list in
 *    cell_layout.
 * @set_cell_data_func: Sets the #CtkCellLayoutDataFunc to use for
 *    cell_layout.
 * @clear_attributes: Clears all existing attributes previously set
 *    with ctk_cell_layout_set_attributes().
 * @reorder: Re-inserts cell at position.
 * @get_cells: Get the cell renderers which have been added to
 *    cell_layout.
 * @get_area: Get the underlying #CtkCellArea which might be
 *    cell_layout if called on a #CtkCellArea or might be NULL if no
 *    #CtkCellArea is used by cell_layout.
 */
struct _CtkCellLayoutIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  /* Virtual Table */
  void (* pack_start)         (CtkCellLayout         *cell_layout,
                               CtkCellRenderer       *cell,
                               gboolean               expand);
  void (* pack_end)           (CtkCellLayout         *cell_layout,
                               CtkCellRenderer       *cell,
                               gboolean               expand);
  void (* clear)              (CtkCellLayout         *cell_layout);
  void (* add_attribute)      (CtkCellLayout         *cell_layout,
                               CtkCellRenderer       *cell,
                               const gchar           *attribute,
                               gint                   column);
  void (* set_cell_data_func) (CtkCellLayout         *cell_layout,
                               CtkCellRenderer       *cell,
                               CtkCellLayoutDataFunc  func,
                               gpointer               func_data,
                               GDestroyNotify         destroy);
  void (* clear_attributes)   (CtkCellLayout         *cell_layout,
                               CtkCellRenderer       *cell);
  void (* reorder)            (CtkCellLayout         *cell_layout,
                               CtkCellRenderer       *cell,
                               gint                   position);
  GList* (* get_cells)        (CtkCellLayout         *cell_layout);

  CtkCellArea *(* get_area)   (CtkCellLayout         *cell_layout);
};

CDK_AVAILABLE_IN_ALL
GType ctk_cell_layout_get_type           (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
void  ctk_cell_layout_pack_start         (CtkCellLayout         *cell_layout,
                                          CtkCellRenderer       *cell,
                                          gboolean               expand);
CDK_AVAILABLE_IN_ALL
void  ctk_cell_layout_pack_end           (CtkCellLayout         *cell_layout,
                                          CtkCellRenderer       *cell,
                                          gboolean               expand);
CDK_AVAILABLE_IN_ALL
GList *ctk_cell_layout_get_cells         (CtkCellLayout         *cell_layout);
CDK_AVAILABLE_IN_ALL
void  ctk_cell_layout_clear              (CtkCellLayout         *cell_layout);
CDK_AVAILABLE_IN_ALL
void  ctk_cell_layout_set_attributes     (CtkCellLayout         *cell_layout,
                                          CtkCellRenderer       *cell,
                                          ...) G_GNUC_NULL_TERMINATED;
CDK_AVAILABLE_IN_ALL
void  ctk_cell_layout_add_attribute      (CtkCellLayout         *cell_layout,
                                          CtkCellRenderer       *cell,
                                          const gchar           *attribute,
                                          gint                   column);
CDK_AVAILABLE_IN_ALL
void  ctk_cell_layout_set_cell_data_func (CtkCellLayout         *cell_layout,
                                          CtkCellRenderer       *cell,
                                          CtkCellLayoutDataFunc  func,
                                          gpointer               func_data,
                                          GDestroyNotify         destroy);
CDK_AVAILABLE_IN_ALL
void  ctk_cell_layout_clear_attributes   (CtkCellLayout         *cell_layout,
                                          CtkCellRenderer       *cell);
CDK_AVAILABLE_IN_ALL
void  ctk_cell_layout_reorder            (CtkCellLayout         *cell_layout,
                                          CtkCellRenderer       *cell,
                                          gint                   position);
CDK_AVAILABLE_IN_ALL
CtkCellArea *ctk_cell_layout_get_area    (CtkCellLayout         *cell_layout);

gboolean _ctk_cell_layout_buildable_custom_tag_start (CtkBuildable  *buildable,
						      CtkBuilder    *builder,
						      GObject       *child,
						      const gchar   *tagname,
						      GMarkupParser *parser,
						      gpointer      *data);
gboolean _ctk_cell_layout_buildable_custom_tag_end   (CtkBuildable  *buildable,
						      CtkBuilder    *builder,
						      GObject       *child,
						      const gchar   *tagname,
						      gpointer      *data);
void _ctk_cell_layout_buildable_add_child            (CtkBuildable  *buildable,
						      CtkBuilder    *builder,
						      GObject       *child,
						      const gchar   *type);

G_END_DECLS

#endif /* __CTK_CELL_LAYOUT_H__ */
