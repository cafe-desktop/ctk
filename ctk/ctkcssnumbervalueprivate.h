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

#ifndef __CTK_CSS_NUMBER_VALUE_PRIVATE_H__
#define __CTK_CSS_NUMBER_VALUE_PRIVATE_H__

#include "ctkcssparserprivate.h"
#include "ctkcsstypesprivate.h"
#include "ctkcssvalueprivate.h"

G_BEGIN_DECLS

typedef enum /*< skip >*/ {
  CTK_CSS_POSITIVE_ONLY = (1 << 0),
  CTK_CSS_PARSE_PERCENT = (1 << 1),
  CTK_CSS_PARSE_NUMBER = (1 << 2),
  CTK_CSS_NUMBER_AS_PIXELS = (1 << 3),
  CTK_CSS_PARSE_LENGTH = (1 << 4),
  CTK_CSS_PARSE_ANGLE = (1 << 5),
  CTK_CSS_PARSE_TIME = (1 << 6)
} CtkCssNumberParseFlags;

typedef struct _CtkCssNumberValueClass CtkCssNumberValueClass;

struct _CtkCssNumberValueClass {
  CtkCssValueClass      value_class;

  double                (* get)                     (const CtkCssValue      *value,
                                                     double                  one_hundred_percent);
  CtkCssDimension       (* get_dimension)           (const CtkCssValue      *value);
  gboolean              (* has_percent)             (const CtkCssValue      *value);
  CtkCssValue *         (* multiply)                (const CtkCssValue      *value,
                                                     double                  factor);
  CtkCssValue *         (* try_add)                 (const CtkCssValue      *value1,
                                                     const CtkCssValue      *value2);
  gint                  (* get_calc_term_order)     (const CtkCssValue      *value);
};

CtkCssValue *   _ctk_css_number_value_new           (double                  value,
                                                     CtkCssUnit              unit);
CtkCssValue *   ctk_css_number_value_transition     (CtkCssValue            *start,
                                                     CtkCssValue            *end,
                                                     guint                   property_id,
                                                     double                  progress);
gboolean        ctk_css_number_value_can_parse      (CtkCssParser           *parser);
CtkCssValue *   _ctk_css_number_value_parse         (CtkCssParser           *parser,
                                                     CtkCssNumberParseFlags  flags);

CtkCssDimension ctk_css_number_value_get_dimension  (const CtkCssValue      *value);
gboolean        ctk_css_number_value_has_percent    (const CtkCssValue      *value);
CtkCssValue *   ctk_css_number_value_multiply       (const CtkCssValue      *value,
                                                     double                  factor);
CtkCssValue *   ctk_css_number_value_add            (CtkCssValue            *value1,
                                                     CtkCssValue            *value2);
CtkCssValue *   ctk_css_number_value_try_add        (const CtkCssValue      *value1,
                                                     const CtkCssValue      *value2);
gint            ctk_css_number_value_get_calc_term_order (const CtkCssValue *value);

double          _ctk_css_number_value_get           (const CtkCssValue      *number,
                                                     double                  one_hundred_percent);


G_END_DECLS

#endif /* __CTK_CSS_NUMBER_VALUE_PRIVATE_H__ */
