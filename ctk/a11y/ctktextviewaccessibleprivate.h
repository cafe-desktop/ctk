/* CTK+ - accessibility implementations
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

#ifndef __CTK_TEXT_VIEW_ACCESSIBLE_PRIVATE_H__
#define __CTK_TEXT_VIEW_ACCESSIBLE_PRIVATE_H__

#include <ctk/a11y/ctktextviewaccessible.h>

G_BEGIN_DECLS

void _ctk_text_view_accessible_set_buffer (CtkTextView   *textview,
                                           CtkTextBuffer *old_buffer);

G_END_DECLS

#endif /* __CTK_TEXT_VIEW_ACCESSIBLE_PRIVATE_H__ */
