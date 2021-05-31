/* GTK - The GIMP Toolkit
 * Copyright (C) 2011 Red Hat, Inc.
 * 
 * Author: Cosimo Cecchi <cosimoc@gnome.org>
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

#ifndef __CTK_RENDER_BACKGROUND_PRIVATE_H__
#define __CTK_RENDER_BACKGROUND_PRIVATE_H__

#include <glib-object.h>
#include <cairo.h>

#include "ctkcsstypesprivate.h"
#include "ctktypes.h"

G_BEGIN_DECLS

void            ctk_css_style_render_background                 (GtkCssStyle          *style,
                                                                 cairo_t              *cr,
                                                                 gdouble               x,
                                                                 gdouble               y,
                                                                 gdouble               width,
                                                                 gdouble               height,
                                                                 GtkJunctionSides      junction);
gboolean        ctk_css_style_render_background_is_opaque       (GtkCssStyle          *style);


G_END_DECLS

#endif /* __CTK_RENDER_BACKGROUND_PRIVATE_H__ */
