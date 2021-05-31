/* GTK+ - accessibility implementations
 * Copyright 2001 Sun Microsystems Inc.
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

#ifndef __CTK_CELL_ACCESSIBLE_PARENT_H__
#define __CTK_CELL_ACCESSIBLE_PARENT_H__

#if !defined (__CTK_A11Y_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk-a11y.h> can be included directly."
#endif

#include <atk/atk.h>
#include <ctk/a11y/ctkcellaccessible.h>

G_BEGIN_DECLS

/*
 * The CtkCellAccessibleParent interface should be supported by any object
 * which contains children which are flyweights, i.e. do not have corresponding
 * widgets and the children need help from their parent to provide
 * functionality. One example is CtkTreeViewAccessible where the children
 * CtkCellAccessible need help from the CtkTreeViewAccessible in order to
 * implement atk_component_get_extents().
 */

#define CTK_TYPE_CELL_ACCESSIBLE_PARENT            (ctk_cell_accessible_parent_get_type ())
#define CTK_IS_CELL_ACCESSIBLE_PARENT(obj)         G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_ACCESSIBLE_PARENT)
#define CTK_CELL_ACCESSIBLE_PARENT(obj)            G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_ACCESSIBLE_PARENT, CtkCellAccessibleParent)
#define CTK_CELL_ACCESSIBLE_PARENT_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CTK_TYPE_CELL_ACCESSIBLE_PARENT, CtkCellAccessibleParentIface))

typedef struct _CtkCellAccessibleParent CtkCellAccessibleParent;
typedef struct _CtkCellAccessibleParentIface CtkCellAccessibleParentIface;

struct _CtkCellAccessibleParentIface
{
  GTypeInterface parent;
  void     ( *get_cell_extents) (CtkCellAccessibleParent *parent,
                                 CtkCellAccessible       *cell,
                                 gint                    *x,
                                 gint                    *y,
                                 gint                    *width,
                                 gint                    *height,
                                 AtkCoordType             coord_type);
  void     ( *get_cell_area)    (CtkCellAccessibleParent *parent,
                                 CtkCellAccessible       *cell,
                                 GdkRectangle            *cell_rect);
  gboolean ( *grab_focus)       (CtkCellAccessibleParent *parent,
                                 CtkCellAccessible       *cell);
  int      ( *get_child_index)  (CtkCellAccessibleParent *parent,
                                 CtkCellAccessible       *cell);
  CtkCellRendererState
           ( *get_renderer_state) (CtkCellAccessibleParent *parent,
                                 CtkCellAccessible       *cell);
  /* actions */
  void     ( *expand_collapse)  (CtkCellAccessibleParent *parent,
                                 CtkCellAccessible       *cell);
  void     ( *activate)         (CtkCellAccessibleParent *parent,
                                 CtkCellAccessible       *cell);
  void     ( *edit)             (CtkCellAccessibleParent *parent,
                                 CtkCellAccessible       *cell);
  /* end of actions */
  void     ( *update_relationset) (CtkCellAccessibleParent *parent,
                                 CtkCellAccessible       *cell,
                                 AtkRelationSet          *relationset);
  void     ( *get_cell_position) (CtkCellAccessibleParent *parent,
                                  CtkCellAccessible       *cell,
                                  gint                    *row,
                                  gint                    *column);
  GPtrArray *   ( *get_column_header_cells) (CtkCellAccessibleParent *parent,
                                             CtkCellAccessible       *cell);
  GPtrArray *   ( *get_row_header_cells)    (CtkCellAccessibleParent *parent,
                                             CtkCellAccessible       *cell);
};

GDK_AVAILABLE_IN_ALL
GType    ctk_cell_accessible_parent_get_type         (void);

GDK_AVAILABLE_IN_ALL
void     ctk_cell_accessible_parent_get_cell_extents (CtkCellAccessibleParent *parent,
                                                      CtkCellAccessible       *cell,
                                                      gint                    *x,
                                                      gint                    *y,
                                                      gint                    *width,
                                                      gint                    *height,
                                                      AtkCoordType             coord_type);
GDK_AVAILABLE_IN_ALL
void     ctk_cell_accessible_parent_get_cell_area    (CtkCellAccessibleParent *parent,
                                                      CtkCellAccessible       *cell,
                                                      GdkRectangle            *cell_rect);
GDK_AVAILABLE_IN_ALL
gboolean ctk_cell_accessible_parent_grab_focus       (CtkCellAccessibleParent *parent,
                                                      CtkCellAccessible       *cell);
GDK_AVAILABLE_IN_ALL
int      ctk_cell_accessible_parent_get_child_index  (CtkCellAccessibleParent *parent,
                                                      CtkCellAccessible       *cell);
GDK_AVAILABLE_IN_ALL
CtkCellRendererState
         ctk_cell_accessible_parent_get_renderer_state(CtkCellAccessibleParent *parent,
                                                       CtkCellAccessible       *cell);
GDK_AVAILABLE_IN_ALL
void     ctk_cell_accessible_parent_expand_collapse  (CtkCellAccessibleParent *parent,
                                                      CtkCellAccessible       *cell);
GDK_AVAILABLE_IN_ALL
void     ctk_cell_accessible_parent_activate         (CtkCellAccessibleParent *parent,
                                                      CtkCellAccessible       *cell);
GDK_AVAILABLE_IN_ALL
void     ctk_cell_accessible_parent_edit             (CtkCellAccessibleParent *parent,
                                                      CtkCellAccessible       *cell);
GDK_AVAILABLE_IN_3_12
void     ctk_cell_accessible_parent_update_relationset (CtkCellAccessibleParent *parent,
                                                      CtkCellAccessible       *cell,
                                                      AtkRelationSet          *relationset);
GDK_AVAILABLE_IN_ALL
void     ctk_cell_accessible_parent_get_cell_position(CtkCellAccessibleParent *parent,
                                                      CtkCellAccessible       *cell,
                                                      gint                    *row,
                                                      gint                    *column);
GDK_AVAILABLE_IN_ALL
GPtrArray   *ctk_cell_accessible_parent_get_column_header_cells (CtkCellAccessibleParent *parent,
                                                                 CtkCellAccessible       *cell);
GDK_AVAILABLE_IN_ALL
GPtrArray   *ctk_cell_accessible_parent_get_row_header_cells    (CtkCellAccessibleParent *parent,
                                                                 CtkCellAccessible       *cell);

G_END_DECLS

#endif /* __CTK_CELL_ACCESSIBLE_PARENT_H__ */
