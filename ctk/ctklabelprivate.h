/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#ifndef __CTK_LABEL_PRIVATE_H__
#define __CTK_LABEL_PRIVATE_H__


#include <ctk/ctklabel.h>


G_BEGIN_DECLS

void _ctk_label_mnemonics_visible_apply_recursively (CtkWidget *widget,
                                                     gboolean   mnemonics_visible);
gint _ctk_label_get_cursor_position (CtkLabel *label);
gint _ctk_label_get_selection_bound (CtkLabel *label);

gint         _ctk_label_get_n_links     (CtkLabel *label);
gint         _ctk_label_get_link_at     (CtkLabel *label,
                                         gint      pos);
void         _ctk_label_activate_link   (CtkLabel *label, 
                                         gint      idx);
const gchar *_ctk_label_get_link_uri    (CtkLabel *label,
                                         gint      idx);
void         _ctk_label_get_link_extent (CtkLabel *label,
                                         gint      idx,
                                         gint     *start,
                                         gint     *end);
gboolean     _ctk_label_get_link_visited (CtkLabel *label,
                                          gint      idx);
gboolean     _ctk_label_get_link_focused (CtkLabel *label,
                                          gint      idx);
                             
G_END_DECLS

#endif /* __CTK_LABEL_PRIVATE_H__ */
