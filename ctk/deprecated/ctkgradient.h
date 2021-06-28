/* CTK - The GIMP Toolkit
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#ifndef __CTK_GRADIENT_H__
#define __CTK_GRADIENT_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cdk/cdk.h>
#include <ctk/ctkstylecontext.h>
#include <ctk/deprecated/ctkstyleproperties.h>
#include <ctk/deprecated/ctksymboliccolor.h>

G_BEGIN_DECLS

#define CTK_TYPE_GRADIENT (ctk_gradient_get_type ())

GDK_DEPRECATED_IN_3_8
GType         ctk_gradient_get_type       (void) G_GNUC_CONST;

GDK_DEPRECATED_IN_3_8
CtkGradient * ctk_gradient_new_linear     (gdouble              x0,
                                           gdouble              y0,
                                           gdouble              x1,
                                           gdouble              y1);
GDK_DEPRECATED_IN_3_8
CtkGradient * ctk_gradient_new_radial     (gdouble              x0,
                                           gdouble              y0,
                                           gdouble              radius0,
                                           gdouble              x1,
                                           gdouble              y1,
                                           gdouble              radius1);

GDK_DEPRECATED_IN_3_8
void          ctk_gradient_add_color_stop (CtkGradient         *gradient,
                                           gdouble              offset,
                                           CtkSymbolicColor    *color);

GDK_DEPRECATED_IN_3_8
CtkGradient * ctk_gradient_ref            (CtkGradient         *gradient);
GDK_DEPRECATED_IN_3_8
void          ctk_gradient_unref          (CtkGradient         *gradient);

GDK_DEPRECATED_IN_3_8
gboolean      ctk_gradient_resolve        (CtkGradient         *gradient,
                                           CtkStyleProperties  *props,
                                           cairo_pattern_t    **resolved_gradient);
GDK_DEPRECATED_IN_3_8
cairo_pattern_t *
              ctk_gradient_resolve_for_context
                                          (CtkGradient         *gradient,
                                           CtkStyleContext     *context);

GDK_DEPRECATED_IN_3_8
char *        ctk_gradient_to_string      (CtkGradient         *gradient);

G_END_DECLS

#endif /* __CTK_GRADIENT_H__ */
