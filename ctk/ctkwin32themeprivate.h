/* GTK - The GIMP Toolkit
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * Authors: Alexander Larsson <alexl@gnome.org>
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

#ifndef __CTK_WIN32_THEME_PART_H__
#define __CTK_WIN32_THEME_PART_H__

#include "ctkcssparserprivate.h"

G_BEGIN_DECLS

typedef struct _CtkWin32Theme CtkWin32Theme;

#define CTK_WIN32_THEME_SYMBOLIC_COLOR_NAME "-ctk-win32-color"

CtkWin32Theme *         ctk_win32_theme_lookup          (const char     *class_name);
CtkWin32Theme *         ctk_win32_theme_parse           (CtkCssParser   *parser);

CtkWin32Theme *         ctk_win32_theme_ref             (CtkWin32Theme  *theme);
void                    ctk_win32_theme_unref           (CtkWin32Theme  *theme);

gboolean                ctk_win32_theme_equal           (CtkWin32Theme  *theme1,
                                                         CtkWin32Theme  *theme2);

void                    ctk_win32_theme_print           (CtkWin32Theme  *theme,
                                                         GString        *string);

cairo_surface_t *       ctk_win32_theme_create_surface  (CtkWin32Theme *theme,
                                                         int            xp_part,
                                                         int            state,
                                                         int            margins[4],
                                                         int            width,
                                                         int            height,
							 int           *x_offs_out,
							 int           *y_offs_out);

void                    ctk_win32_theme_get_part_border (CtkWin32Theme  *theme,
                                                         int             part,
                                                         int             state,
                                                         CtkBorder      *out_border);
void                    ctk_win32_theme_get_part_size   (CtkWin32Theme  *theme,
                                                         int             part,
                                                         int             state,
                                                         int            *width,
                                                         int            *height);
int                     ctk_win32_theme_get_size        (CtkWin32Theme  *theme,
			                                 int             id);
void                    ctk_win32_theme_get_color       (CtkWin32Theme  *theme,
                                                         gint            id,
                                                         GdkRGBA        *color);

G_END_DECLS

#endif /* __CTK_WIN32_THEME_PART_H__ */
