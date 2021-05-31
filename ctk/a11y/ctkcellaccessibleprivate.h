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

#ifndef __CTK_CELL_ACCESSIBLE_PRIVATE_H__
#define __CTK_CELL_ACCESSIBLE_PRIVATE_H__

#include <ctk/a11y/ctkcellaccessible.h>

G_BEGIN_DECLS

void     _ctk_cell_accessible_state_changed (CtkCellAccessible *cell,
                                             CtkCellRendererState added,
                                             CtkCellRendererState removed);
void     _ctk_cell_accessible_update_cache  (CtkCellAccessible *cell,
                                             gboolean           emit_signal);
void     _ctk_cell_accessible_initialize    (CtkCellAccessible *cell,
                                             CtkWidget         *widget,
                                             AtkObject         *parent);
gboolean _ctk_cell_accessible_add_state     (CtkCellAccessible *cell,
                                             AtkStateType       state_type,
                                             gboolean           emit_signal);
gboolean _ctk_cell_accessible_remove_state  (CtkCellAccessible *cell,
                                             AtkStateType       state_type,
                                             gboolean           emit_signal);

G_END_DECLS

#endif /* __CTK_CELL_ACCESSIBLE_PRIVATE_H__ */
