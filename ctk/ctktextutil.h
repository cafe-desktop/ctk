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

/*
 * Modified by the CTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_TEXT_UTIL_H__
#define __CTK_TEXT_UTIL_H__

G_BEGIN_DECLS

/* This is a private uninstalled header shared between
 * CtkTextView and CtkEntry
 */

typedef void (* CtkTextUtilCharChosenFunc) (const char *text,
                                            gpointer    data);

void _ctk_text_util_append_special_char_menuitems (CtkMenuShell              *menushell,
                                                   CtkTextUtilCharChosenFunc  func,
                                                   gpointer                   data);

cairo_surface_t * _ctk_text_util_create_drag_icon (CtkWidget     *widget,
                                                   gchar         *text,
                                                   gsize          len);
cairo_surface_t * _ctk_text_util_create_rich_drag_icon (CtkWidget     *widget,
                                                   CtkTextBuffer *buffer,
                                                   CtkTextIter   *start,
                                                   CtkTextIter   *end);

gboolean _ctk_text_util_get_block_cursor_location (PangoLayout    *layout,
						   gint            index_,
						   PangoRectangle *rectangle,
						   gboolean       *at_line_end);

G_END_DECLS

#endif /* __CTK_TEXT_UTIL_H__ */
