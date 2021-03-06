/*
 * Copyright © 2012 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Alexander Larsson <alexl@gnome.org>
 */

#ifndef __CTK_CSS_REPEAT_VALUE_PRIVATE_H__
#define __CTK_CSS_REPEAT_VALUE_PRIVATE_H__

#include "ctkcssparserprivate.h"
#include "ctkcssvalueprivate.h"

G_BEGIN_DECLS

typedef enum {
  CTK_CSS_REPEAT_STYLE_NO_REPEAT,
  CTK_CSS_REPEAT_STYLE_STRETCH = CTK_CSS_REPEAT_STYLE_NO_REPEAT,
  CTK_CSS_REPEAT_STYLE_REPEAT,
  CTK_CSS_REPEAT_STYLE_ROUND,
  CTK_CSS_REPEAT_STYLE_SPACE
} CtkCssRepeatStyle;

CtkCssValue *       _ctk_css_background_repeat_value_new        (CtkCssRepeatStyle       x,
                                                                 CtkCssRepeatStyle       y);
CtkCssValue *       _ctk_css_background_repeat_value_try_parse  (CtkCssParser           *parser);
CtkCssRepeatStyle   _ctk_css_background_repeat_value_get_x      (const CtkCssValue      *repeat);
CtkCssRepeatStyle   _ctk_css_background_repeat_value_get_y      (const CtkCssValue      *repeat);

CtkCssValue *       _ctk_css_border_repeat_value_new            (CtkCssRepeatStyle       x,
                                                                 CtkCssRepeatStyle       y);
CtkCssValue *       _ctk_css_border_repeat_value_try_parse      (CtkCssParser           *parser);
CtkCssRepeatStyle   _ctk_css_border_repeat_value_get_x          (const CtkCssValue      *repeat);
CtkCssRepeatStyle   _ctk_css_border_repeat_value_get_y          (const CtkCssValue      *repeat);

G_END_DECLS

#endif /* __CTK_CSS_REPEAT_VALUE_PRIVATE_H__ */
