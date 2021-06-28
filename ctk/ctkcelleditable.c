/* ctkcelleditable.c
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

/**
 * SECTION:ctkcelleditable
 * @Short_description: Interface for widgets that can be used for editing cells
 * @Title: CtkCellEditable
 * @See_also: #CtkCellRenderer
 *
 * The #CtkCellEditable interface must be implemented for widgets to be usable
 * to edit the contents of a #CtkTreeView cell. It provides a way to specify how
 * temporary widgets should be configured for editing, get the new value, etc.
 */

#include "config.h"
#include "ctkcelleditable.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctkintl.h"


typedef CtkCellEditableIface CtkCellEditableInterface;
G_DEFINE_INTERFACE(CtkCellEditable, ctk_cell_editable, CTK_TYPE_WIDGET)

static void
ctk_cell_editable_default_init (CtkCellEditableInterface *iface)
{
  /**
   * CtkCellEditable:editing-canceled:
   *
   * Indicates whether editing on the cell has been canceled.
   *
   * Since: 2.20
   */
  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("editing-canceled",
                                       P_("Editing Canceled"),
                                       P_("Indicates that editing has been canceled"),
                                       FALSE,
                                       CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkCellEditable::editing-done:
   * @cell_editable: the object on which the signal was emitted
   *
   * This signal is a sign for the cell renderer to update its
   * value from the @cell_editable.
   *
   * Implementations of #CtkCellEditable are responsible for
   * emitting this signal when they are done editing, e.g.
   * #CtkEntry emits this signal when the user presses Enter. Typical things to
   * do in a handler for ::editing-done are to capture the edited value,
   * disconnect the @cell_editable from signals on the #CtkCellRenderer, etc.
   *
   * ctk_cell_editable_editing_done() is a convenience method
   * for emitting #CtkCellEditable::editing-done.
   */
  g_signal_new (I_("editing-done"),
                CTK_TYPE_CELL_EDITABLE,
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (CtkCellEditableIface, editing_done),
                NULL, NULL,
                NULL,
                G_TYPE_NONE, 0);

  /**
   * CtkCellEditable::remove-widget:
   * @cell_editable: the object on which the signal was emitted
   *
   * This signal is meant to indicate that the cell is finished
   * editing, and the @cell_editable widget is being removed and may
   * subsequently be destroyed.
   *
   * Implementations of #CtkCellEditable are responsible for
   * emitting this signal when they are done editing. It must
   * be emitted after the #CtkCellEditable::editing-done signal,
   * to give the cell renderer a chance to update the cell's value
   * before the widget is removed.
   *
   * ctk_cell_editable_remove_widget() is a convenience method
   * for emitting #CtkCellEditable::remove-widget.
   */
  g_signal_new (I_("remove-widget"),
                CTK_TYPE_CELL_EDITABLE,
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (CtkCellEditableIface, remove_widget),
                NULL, NULL,
                NULL,
                G_TYPE_NONE, 0);
}

/**
 * ctk_cell_editable_start_editing:
 * @cell_editable: A #CtkCellEditable
 * @event: (nullable): The #CdkEvent that began the editing process, or
 *   %NULL if editing was initiated programmatically
 * 
 * Begins editing on a @cell_editable.
 *
 * The #CtkCellRenderer for the cell creates and returns a #CtkCellEditable from
 * ctk_cell_renderer_start_editing(), configured for the #CtkCellRenderer type.
 *
 * ctk_cell_editable_start_editing() can then set up @cell_editable suitably for
 * editing a cell, e.g. making the Esc key emit #CtkCellEditable::editing-done.
 *
 * Note that the @cell_editable is created on-demand for the current edit; its
 * lifetime is temporary and does not persist across other edits and/or cells.
 **/
void
ctk_cell_editable_start_editing (CtkCellEditable *cell_editable,
				 CdkEvent        *event)
{
  g_return_if_fail (CTK_IS_CELL_EDITABLE (cell_editable));

  (* CTK_CELL_EDITABLE_GET_IFACE (cell_editable)->start_editing) (cell_editable, event);
}

/**
 * ctk_cell_editable_editing_done:
 * @cell_editable: A #CtkCellEditable
 * 
 * Emits the #CtkCellEditable::editing-done signal. 
 **/
void
ctk_cell_editable_editing_done (CtkCellEditable *cell_editable)
{
  g_return_if_fail (CTK_IS_CELL_EDITABLE (cell_editable));

  g_signal_emit_by_name (cell_editable, "editing-done");
}

/**
 * ctk_cell_editable_remove_widget:
 * @cell_editable: A #CtkCellEditable
 * 
 * Emits the #CtkCellEditable::remove-widget signal.  
 **/
void
ctk_cell_editable_remove_widget (CtkCellEditable *cell_editable)
{
  g_return_if_fail (CTK_IS_CELL_EDITABLE (cell_editable));

  g_signal_emit_by_name (cell_editable, "remove-widget");
}
