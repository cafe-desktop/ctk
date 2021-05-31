/* GTK - The GIMP Toolkit
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

#ifndef __CTK_IMAGE_DEFINITION_H__
#define __CTK_IMAGE_DEFINITION_H__

#include "ctk/ctkimage.h"
#include "ctk/ctktypes.h"

G_BEGIN_DECLS

typedef union _CtkImageDefinition CtkImageDefinition;

CtkImageDefinition *    ctk_image_definition_new_empty          (void);
CtkImageDefinition *    ctk_image_definition_new_pixbuf         (GdkPixbuf                      *pixbuf,
                                                                 int                             scale);
CtkImageDefinition *    ctk_image_definition_new_stock          (const char                     *stock_id);
CtkImageDefinition *    ctk_image_definition_new_icon_set       (CtkIconSet                     *icon_set);
CtkImageDefinition *    ctk_image_definition_new_animation      (GdkPixbufAnimation             *animation,
                                                                 int                             scale);
CtkImageDefinition *    ctk_image_definition_new_icon_name      (const char                     *icon_name);
CtkImageDefinition *    ctk_image_definition_new_gicon          (GIcon                          *gicon);
CtkImageDefinition *    ctk_image_definition_new_surface        (cairo_surface_t                *surface);

CtkImageDefinition *    ctk_image_definition_ref                (CtkImageDefinition             *def);
void                    ctk_image_definition_unref              (CtkImageDefinition             *def);

CtkImageType            ctk_image_definition_get_storage_type   (const CtkImageDefinition       *def);
gint                    ctk_image_definition_get_scale          (const CtkImageDefinition       *def);
GdkPixbuf *             ctk_image_definition_get_pixbuf         (const CtkImageDefinition       *def);
const gchar *           ctk_image_definition_get_stock          (const CtkImageDefinition       *def);
CtkIconSet *            ctk_image_definition_get_icon_set       (const CtkImageDefinition       *def);
GdkPixbufAnimation *    ctk_image_definition_get_animation      (const CtkImageDefinition       *def);
const gchar *           ctk_image_definition_get_icon_name      (const CtkImageDefinition       *def);
GIcon *                 ctk_image_definition_get_gicon          (const CtkImageDefinition       *def);
cairo_surface_t *       ctk_image_definition_get_surface        (const CtkImageDefinition       *def);


G_END_DECLS

#endif /* __CTK_IMAGE_DEFINITION_H__ */
