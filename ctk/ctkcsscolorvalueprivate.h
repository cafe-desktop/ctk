/* GTK - The GIMP Toolkit
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

#ifndef __CTK_CSS_COLOR_VALUE_PRIVATE_H__
#define __CTK_CSS_COLOR_VALUE_PRIVATE_H__

#include "ctkcssparserprivate.h"
#include "ctkcssvalueprivate.h"

G_BEGIN_DECLS


CtkCssValue *   _ctk_css_color_value_new_literal        (const GdkRGBA  *color);
CtkCssValue *   _ctk_css_color_value_new_rgba           (double          red,
                                                         double          green,
                                                         double          blue,
                                                         double          alpha);
CtkCssValue *   _ctk_css_color_value_new_name           (const gchar    *name);
CtkCssValue *   _ctk_css_color_value_new_shade          (CtkCssValue    *color,
                                                         gdouble         factor);
CtkCssValue *   _ctk_css_color_value_new_alpha          (CtkCssValue    *color,
                                                         gdouble         factor);
CtkCssValue *   _ctk_css_color_value_new_mix            (CtkCssValue    *color1,
                                                         CtkCssValue    *color2,
                                                         gdouble         factor);
CtkCssValue *   _ctk_css_color_value_new_win32          (const gchar    *theme_class,
                                                         gint            id);
CtkCssValue *   _ctk_css_color_value_new_current_color  (void);

CtkCssValue *   _ctk_css_color_value_parse              (CtkCssParser   *parser);

CtkCssValue *   _ctk_css_color_value_resolve            (CtkCssValue             *color,
                                                         CtkStyleProviderPrivate *provider,
                                                         CtkCssValue             *current,
                                                         GSList                  *cycle_list);


G_END_DECLS

#endif /* __CTK_CSS_COLOR_VALUE_PRIVATE_H__ */
