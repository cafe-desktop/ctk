/* GTK+ - accessibility implementations
 * Copyright (C) 2002, 2004  Anders Carlsson <andersca@gnu.org>
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

#ifndef __CTK_LABEL_ACCESSIBLE_PRIVATE_H__
#define __CTK_LABEL_ACCESSIBLE_PRIVATE_H__

#include <ctk/a11y/ctklabelaccessible.h>

G_BEGIN_DECLS

void _ctk_label_accessible_text_deleted  (CtkLabel *label);
void _ctk_label_accessible_text_inserted (CtkLabel *label);
void _ctk_label_accessible_update_links  (CtkLabel *label);
void _ctk_label_accessible_focus_link_changed (CtkLabel *label);

G_END_DECLS

#endif /* __CTK_LABEL_ACCESSIBLE_PRIVATE_H__ */
