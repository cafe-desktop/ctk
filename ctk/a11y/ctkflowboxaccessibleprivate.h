/*
 * Copyright (C) 2013 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_FLOW_BOX_ACCESSIBLE_PRIVATE_H__
#define __CTK_FLOW_BOX_ACCESSIBLE_PRIVATE_H__

#include <ctk/a11y/ctkflowboxaccessible.h>

G_BEGIN_DECLS

void _ctk_flow_box_accessible_selection_changed (CtkWidget *box);
void _ctk_flow_box_accessible_update_cursor     (CtkWidget *box,
                                                 CtkWidget *child);
G_END_DECLS

#endif /* __CTK_FLOW_BOX_ACCESSIBLE_PRIVATE_H__ */
