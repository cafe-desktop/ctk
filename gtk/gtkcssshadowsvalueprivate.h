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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_CSS_SHADOWS_VALUE_H__
#define __CTK_CSS_SHADOWS_VALUE_H__

#include <cairo.h>
#include <pango/pango.h>

#include "gtktypes.h"
#include "gtkcssparserprivate.h"
#include "gtkcssvalueprivate.h"
#include "gtkroundedboxprivate.h"

G_BEGIN_DECLS

GtkCssValue *   _ctk_css_shadows_value_new_none       (void);
GtkCssValue *   _ctk_css_shadows_value_parse          (GtkCssParser             *parser,
                                                       gboolean                  box_shadow_mode);

gboolean        _ctk_css_shadows_value_is_none        (const GtkCssValue        *shadows);

void            _ctk_css_shadows_value_paint_layout   (const GtkCssValue        *shadows,
                                                       cairo_t                  *cr,
                                                       PangoLayout              *layout);

void            _ctk_css_shadows_value_paint_icon     (const GtkCssValue        *shadows,
					               cairo_t                  *cr);

void            _ctk_css_shadows_value_paint_box      (const GtkCssValue        *shadows,
                                                       cairo_t                  *cr,
                                                       const GtkRoundedBox      *padding_box,
                                                       gboolean                  inset);

void            _ctk_css_shadows_value_get_extents    (const GtkCssValue        *shadows,
                                                       GtkBorder                *border);

G_END_DECLS

#endif /* __CTK_CSS_SHADOWS_VALUE_H__ */
