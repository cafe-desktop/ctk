/* CtkToolPalette -- A tool palette with categories and DnD support
 * Copyright (C) 2008  Openismus GmbH
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Mathias Hasselmann
 */

#ifndef __CTK_TOOL_PALETTE_PRIVATE_H__
#define __CTK_TOOL_PALETTE_PRIVATE_H__

#include <ctk/ctk.h>

G_BEGIN_DECLS

void _ctk_tool_palette_get_item_size           (CtkToolPalette   *palette,
                                                CtkRequisition   *item_size,
                                                gboolean          homogeneous_only,
                                                gint             *requested_rows);
void _ctk_tool_palette_child_set_drag_source   (CtkWidget        *widget,
                                                gpointer          data);
void _ctk_tool_palette_set_expanding_child     (CtkToolPalette   *palette,
                                                CtkWidget        *widget);

void _ctk_tool_item_group_palette_reconfigured (CtkToolItemGroup *group);
void _ctk_tool_item_group_item_size_request    (CtkToolItemGroup *group,
                                                CtkRequisition   *item_size,
                                                gboolean          homogeneous_only,
                                                gint             *requested_rows);
gint _ctk_tool_item_group_get_height_for_width (CtkToolItemGroup *group,
                                                gint              width);
gint _ctk_tool_item_group_get_width_for_height (CtkToolItemGroup *group,
                                                gint              height);
gint _ctk_tool_item_group_get_size_for_limit   (CtkToolItemGroup *group,
                                                gint              limit,
                                                gboolean          vertical,
                                                gboolean          animation);


CtkSizeGroup *_ctk_tool_palette_get_size_group (CtkToolPalette   *palette);

G_END_DECLS

#endif /* __CTK_TOOL_PALETTE_PRIVATE_H__ */
