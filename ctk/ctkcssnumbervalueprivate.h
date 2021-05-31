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

#include "gtkcssparserprivate.h"
#include "gtkcsstypesprivate.h"
#include "gtkcssvalueprivate.h"

G_BEGIN_DECLS

typedef enum /*< skip >*/ {
  CTK_CSS_POSITIVE_ONLY = (1 << 0),
  CTK_CSS_PARSE_PERCENT = (1 << 1),
  CTK_CSS_PARSE_NUMBER = (1 << 2),
  CTK_CSS_NUMBER_AS_PIXELS = (1 << 3),
  CTK_CSS_PARSE_LENGTH = (1 << 4),
  CTK_CSS_PARSE_ANGLE = (1 << 5),
  CTK_CSS_PARSE_TIME = (1 << 6)
} GtkCssNumberParseFlags;

typedef struct _GtkCssNumberValueClass GtkCssNumberValueClass;

struct _GtkCssNumberValueClass {
  GtkCssValueClass      value_class;

  double                (* get)                     (const GtkCssValue      *value,
                                                     double                  one_hundred_percent);
  GtkCssDimension       (* get_dimension)           (const GtkCssValue      *value);
  gboolean              (* has_percent)             (const GtkCssValue      *value);
  GtkCssValue *         (* multiply)                (const GtkCssValue      *value,
                                                     double                  factor);
  GtkCssValue *         (* try_add)                 (const GtkCssValue      *value1,
                                                     const GtkCssValue      *value2);
  gint                  (* get_calc_term_order)     (const GtkCssValue      *value);
};

GtkCssValue *   _ctk_css_number_value_new           (double                  value,
                                                     GtkCssUnit              unit);
GtkCssValue *   ctk_css_number_value_transition     (GtkCssValue            *start,
                                                     GtkCssValue            *end,
                                                     guint                   property_id,
                                                     double                  progress);
gboolean        ctk_css_number_value_can_parse      (GtkCssParser           *parser);
GtkCssValue *   _ctk_css_number_value_parse         (GtkCssParser           *parser,
                                                     GtkCssNumberParseFlags  flags);

GtkCssDimension ctk_css_number_value_get_dimension  (const GtkCssValue      *value);
gboolean        ctk_css_number_value_has_percent    (const GtkCssValue      *value);
GtkCssValue *   ctk_css_number_value_multiply       (const GtkCssValue      *value,
                                                     double                  factor);
GtkCssValue *   ctk_css_number_value_add            (GtkCssValue            *value1,
                                                     GtkCssValue            *value2);
GtkCssValue *   ctk_css_number_value_try_add        (const GtkCssValue      *value1,
                                                     const GtkCssValue      *value2);
gint            ctk_css_number_value_get_calc_term_order (const GtkCssValue *value);

double          _ctk_css_number_value_get           (const GtkCssValue      *number,
                                                     double                  one_hundred_percent);


G_END_DECLS

#endif /* __CTK_CSS_NUMBER_VALUE_PRIVATE_H__ */
