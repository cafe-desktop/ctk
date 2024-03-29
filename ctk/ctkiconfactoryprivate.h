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

#ifndef __CTK_ICON_FACTORY_PRIVATE_H__
#define __CTK_ICON_FACTORY_PRIVATE_H__

#include <ctk/ctkiconfactory.h>

GList *     _ctk_icon_factory_list_ids                  (void);
void        _ctk_icon_factory_ensure_default_icons      (void);

GdkPixbuf * ctk_icon_set_render_icon_pixbuf_for_scale   (CtkIconSet             *icon_set,
                                                         CtkCssStyle            *style,
                                                         CtkTextDirection        direction,
                                                         CtkIconSize             size,
                                                         gint                    scale);

#endif /* __CTK_ICON_FACTORY_PRIVATE_H__ */
