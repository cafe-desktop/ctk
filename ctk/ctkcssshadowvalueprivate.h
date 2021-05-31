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

#ifndef __CTK_SHADOW_H__
#define __CTK_SHADOW_H__

#include <cairo.h>
#include <pango/pango.h>

#include "ctktypes.h"
#include "ctkcssparserprivate.h"
#include "ctkcssvalueprivate.h"
#include "ctkroundedboxprivate.h"

G_BEGIN_DECLS

CtkCssValue *   _ctk_css_shadow_value_new_for_transition (CtkCssValue           *target);

CtkCssValue *   _ctk_css_shadow_value_parse           (CtkCssParser             *parser,
                                                       gboolean                  box_shadow_mode);

gboolean        _ctk_css_shadow_value_get_inset       (const CtkCssValue        *shadow);

void            _ctk_css_shadow_value_get_geometry    (const CtkCssValue        *shadow,
                                                       gdouble                  *hoffset,
                                                       gdouble                  *voffset,
                                                       gdouble                  *radius,
                                                       gdouble                  *spread);

void            _ctk_css_shadow_value_paint_layout    (const CtkCssValue        *shadow,
                                                       cairo_t                  *cr,
                                                       PangoLayout              *layout);

void            _ctk_css_shadow_value_paint_icon      (const CtkCssValue        *shadow,
					               cairo_t                  *cr);

void            _ctk_css_shadow_value_paint_box       (const CtkCssValue        *shadow,
                                                       cairo_t                  *cr,
                                                       const CtkRoundedBox      *padding_box);

G_END_DECLS

#endif /* __CTK_SHADOW_H__ */
