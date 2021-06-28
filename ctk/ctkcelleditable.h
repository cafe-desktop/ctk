/* ctkcelleditable.h
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

#ifndef __CTK_CELL_EDITABLE_H__
#define __CTK_CELL_EDITABLE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_CELL_EDITABLE            (ctk_cell_editable_get_type ())
#define CTK_CELL_EDITABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_EDITABLE, CtkCellEditable))
#define CTK_CELL_EDITABLE_CLASS(obj)      (G_TYPE_CHECK_CLASS_CAST ((obj), CTK_TYPE_CELL_EDITABLE, CtkCellEditableIface))
#define CTK_IS_CELL_EDITABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_EDITABLE))
#define CTK_CELL_EDITABLE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CTK_TYPE_CELL_EDITABLE, CtkCellEditableIface))

typedef struct _CtkCellEditable      CtkCellEditable; /* Dummy typedef */
typedef struct _CtkCellEditableIface CtkCellEditableIface;

/**
 * CtkCellEditableIface:
 * @editing_done: Signal is a sign for the cell renderer to update its
 *    value from the cell_editable.
 * @remove_widget: Signal is meant to indicate that the cell is
 *    finished editing, and the widget may now be destroyed.
 * @start_editing: Begins editing on a cell_editable.
 */
struct _CtkCellEditableIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  /* signals */
  void (* editing_done)  (CtkCellEditable *cell_editable);
  void (* remove_widget) (CtkCellEditable *cell_editable);

  /* virtual table */
  void (* start_editing) (CtkCellEditable *cell_editable,
			  CdkEvent        *event);
};


CDK_AVAILABLE_IN_ALL
GType ctk_cell_editable_get_type      (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
void  ctk_cell_editable_start_editing (CtkCellEditable *cell_editable,
				       CdkEvent        *event);
CDK_AVAILABLE_IN_ALL
void  ctk_cell_editable_editing_done  (CtkCellEditable *cell_editable);
CDK_AVAILABLE_IN_ALL
void  ctk_cell_editable_remove_widget (CtkCellEditable *cell_editable);


G_END_DECLS

#endif /* __CTK_CELL_EDITABLE_H__ */
