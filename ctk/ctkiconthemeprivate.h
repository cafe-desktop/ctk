/* CTK - The GIMP Toolkit
 * Copyright (C) 2015 Benjamin Otte <otte@gnome.org>
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

#ifndef __CTK_ICON_THEME_PRIVATE_H__
#define __CTK_ICON_THEME_PRIVATE_H__

#include <ctk/ctkicontheme.h>
#include <ctk/ctkcssstyleprivate.h>

void        ctk_icon_theme_lookup_symbolic_colors       (CtkCssStyle    *style,
                                                         CdkRGBA        *color_out,
                                                         CdkRGBA        *success_out,
                                                         CdkRGBA        *warning_out,
                                                         CdkRGBA        *error_out);

CtkIconInfo *ctk_icon_info_new_for_file (GFile *file,
                                         gint   size,
                                         gint   scale);

CdkPixbuf * ctk_icon_theme_color_symbolic_pixbuf (CdkPixbuf     *symbolic,
                                                  const CdkRGBA *fg_color,
                                                  const CdkRGBA *success_color,
                                                  const CdkRGBA *warning_color,
                                                  const CdkRGBA *error_color);


#endif /* __CTK_ICON_THEME_PRIVATE_H__ */
