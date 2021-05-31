/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_TEXT_TAG_TABLE_PRIVATE_H__
#define __CTK_TEXT_TAG_TABLE_PRIVATE_H__

#include <ctk/ctktexttagtable.h>

G_BEGIN_DECLS

void     _ctk_text_tag_table_add_buffer         (CtkTextTagTable *table,
                                                 gpointer         buffer);
void     _ctk_text_tag_table_remove_buffer      (CtkTextTagTable *table,
                                                 gpointer         buffer);
void     _ctk_text_tag_table_tag_changed        (CtkTextTagTable *table,
                                                 CtkTextTag      *tag,
                                                 gboolean         size_changed);
gboolean _ctk_text_tag_table_affects_visibility (CtkTextTagTable *table);

G_END_DECLS

#endif
