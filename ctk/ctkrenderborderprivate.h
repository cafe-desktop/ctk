/* CTK - The GIMP Toolkit
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * Authors: Carlos Garnacho <carlosg@gnome.org>
 *          Cosimo Cecchi <cosimoc@gnome.org>
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

#ifndef __CTK_RENDER_BORDER_H__
#define __CTK_RENDER_BORDER_H__

#include "ctkborder.h"
#include "ctkcssimageprivate.h"
#include "ctkcssvalueprivate.h"

G_BEGIN_DECLS

gboolean        ctk_css_style_render_has_border         (CtkCssStyle            *style);
void            ctk_css_style_render_border             (CtkCssStyle            *style,
                                                         cairo_t                *cr,
                                                         gdouble                 x,
                                                         gdouble                 y,
                                                         gdouble                 width,
                                                         gdouble                 height,
                                                         guint                   hidden_side,
                                                         CtkJunctionSides        junction);
gboolean        ctk_css_style_render_border_get_clip    (CtkCssStyle            *style,
                                                         gdouble                 x,
                                                         gdouble                 y,
                                                         gdouble                 width,
                                                         gdouble                 height,
                                                         GdkRectangle           *out_clip) G_GNUC_WARN_UNUSED_RESULT;

gboolean        ctk_css_style_render_has_outline        (CtkCssStyle            *style);
void            ctk_css_style_render_outline            (CtkCssStyle            *style,
                                                         cairo_t                *cr,
                                                         gdouble                 x,
                                                         gdouble                 y,
                                                         gdouble                 width,
                                                         gdouble                 height);
gboolean        ctk_css_style_render_outline_get_clip   (CtkCssStyle            *style,
                                                         gdouble                 x,
                                                         gdouble                 y,
                                                         gdouble                 width,
                                                         gdouble                 height,
                                                         GdkRectangle           *out_clip) G_GNUC_WARN_UNUSED_RESULT;


G_END_DECLS

#endif /* __CTK_RENDER_BORDER_H__ */
