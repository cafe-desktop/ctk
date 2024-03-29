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

#ifndef __CTK_SYMBOLIC_COLOR_H__
#define __CTK_SYMBOLIC_COLOR_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cdk/cdk.h>
#include <ctk/ctkstyleproperties.h>

G_BEGIN_DECLS

#define CTK_TYPE_SYMBOLIC_COLOR (ctk_symbolic_color_get_type ())

CDK_DEPRECATED_IN_3_8
GType              ctk_symbolic_color_get_type    (void) G_GNUC_CONST;

CDK_DEPRECATED_IN_3_8
CtkSymbolicColor * ctk_symbolic_color_new_literal (const CdkRGBA      *color);
CDK_DEPRECATED_IN_3_8
CtkSymbolicColor * ctk_symbolic_color_new_name    (const gchar        *name);
CDK_DEPRECATED_IN_3_8
CtkSymbolicColor * ctk_symbolic_color_new_shade   (CtkSymbolicColor   *color,
                                                   gdouble             factor);
CDK_DEPRECATED_IN_3_8
CtkSymbolicColor * ctk_symbolic_color_new_alpha   (CtkSymbolicColor   *color,
                                                   gdouble             factor);
CDK_DEPRECATED_IN_3_8
CtkSymbolicColor * ctk_symbolic_color_new_mix     (CtkSymbolicColor   *color1,
                                                   CtkSymbolicColor   *color2,
                                                   gdouble             factor);
CDK_DEPRECATED_IN_3_8
CtkSymbolicColor * ctk_symbolic_color_new_win32   (const gchar        *theme_class,
                                                   gint                id);

CDK_DEPRECATED_IN_3_8
CtkSymbolicColor * ctk_symbolic_color_ref         (CtkSymbolicColor   *color);
CDK_DEPRECATED_IN_3_8
void               ctk_symbolic_color_unref       (CtkSymbolicColor   *color);

CDK_DEPRECATED_IN_3_8
char *             ctk_symbolic_color_to_string   (CtkSymbolicColor   *color);

CDK_DEPRECATED_IN_3_8
gboolean           ctk_symbolic_color_resolve     (CtkSymbolicColor   *color,
                                                   CtkStyleProperties *props,
                                                   CdkRGBA            *resolved_color);

G_END_DECLS

#endif /* __CTK_SYMBOLIC_COLOR_H__ */
