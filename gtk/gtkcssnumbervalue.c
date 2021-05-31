/* GTK - The GIMP Toolkit
 * Copyright (C) 2016 Benjamin Otte <otte@gnome.org>
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

#include "config.h"

#include "gtkcssnumbervalueprivate.h"

#include "gtkcsscalcvalueprivate.h"
#include "gtkcssdimensionvalueprivate.h"
#include "gtkcsswin32sizevalueprivate.h"

struct _GtkCssValue {
  GTK_CSS_VALUE_BASE
};

GtkCssDimension
ctk_css_number_value_get_dimension (const GtkCssValue *value)
{
  GtkCssNumberValueClass *number_value_class = (GtkCssNumberValueClass *) value->class;

  return number_value_class->get_dimension (value);
}

gboolean
ctk_css_number_value_has_percent (const GtkCssValue *value)
{
  GtkCssNumberValueClass *number_value_class = (GtkCssNumberValueClass *) value->class;

  return number_value_class->has_percent (value);
}

GtkCssValue *
ctk_css_number_value_multiply (const GtkCssValue *value,
                               double             factor)
{
  GtkCssNumberValueClass *number_value_class = (GtkCssNumberValueClass *) value->class;

  return number_value_class->multiply (value, factor);
}

GtkCssValue *
ctk_css_number_value_add (GtkCssValue *value1,
                          GtkCssValue *value2)
{
  GtkCssValue *sum;

  sum = ctk_css_number_value_try_add (value1, value2);
  if (sum == NULL)
    sum = ctk_css_calc_value_new_sum (value1, value2);

  return sum;
}

GtkCssValue *
ctk_css_number_value_try_add (const GtkCssValue *value1,
                              const GtkCssValue *value2)
{
  GtkCssNumberValueClass *number_value_class;
  
  if (value1->class != value2->class)
    return NULL;

  number_value_class = (GtkCssNumberValueClass *) value1->class;

  return number_value_class->try_add (value1, value2);
}

/*
 * ctk_css_number_value_get_calc_term_order:
 * @value: Value to compute order for
 *
 * Determines the position of @value when printed as part of a calc()
 * expression. Values with lower numbers are printed first. Note that
 * these numbers are arbitrary, so when adding new types of values to
 * print, feel free to change them in implementations so that they
 * match.
 *
 * Returns: Magic value determining placement when printing calc()
 *     expression.
 */
gint
ctk_css_number_value_get_calc_term_order (const GtkCssValue *value)
{
  GtkCssNumberValueClass *number_value_class = (GtkCssNumberValueClass *) value->class;

  return number_value_class->get_calc_term_order (value);
}

GtkCssValue *
_ctk_css_number_value_new (double     value,
                           GtkCssUnit unit)
{
  return ctk_css_dimension_value_new (value, unit);
}

GtkCssValue *
ctk_css_number_value_transition (GtkCssValue *start,
                                 GtkCssValue *end,
                                 guint        property_id,
                                 double       progress)
{
  GtkCssValue *result, *mul_start, *mul_end;

  mul_start = ctk_css_number_value_multiply (start, 1 - progress);
  mul_end = ctk_css_number_value_multiply (end, progress);

  result = ctk_css_number_value_add (mul_start, mul_end);

  _ctk_css_value_unref (mul_start);
  _ctk_css_value_unref (mul_end);

  return result;
}

gboolean
ctk_css_number_value_can_parse (GtkCssParser *parser)
{
  return _ctk_css_parser_has_number (parser)
      || _ctk_css_parser_has_prefix (parser, "calc")
      || _ctk_css_parser_has_prefix (parser, "-gtk-win32-size")
      || _ctk_css_parser_has_prefix (parser, "-gtk-win32-part-width")
      || _ctk_css_parser_has_prefix (parser, "-gtk-win32-part-height")
      || _ctk_css_parser_has_prefix (parser, "-gtk-win32-part-border-top")
      || _ctk_css_parser_has_prefix (parser, "-gtk-win32-part-border-left")
      || _ctk_css_parser_has_prefix (parser, "-gtk-win32-part-border-bottom")
      || _ctk_css_parser_has_prefix (parser, "-gtk-win32-part-border-right");
}

GtkCssValue *
_ctk_css_number_value_parse (GtkCssParser           *parser,
                             GtkCssNumberParseFlags  flags)
{
  if (_ctk_css_parser_has_prefix (parser, "calc"))
    return ctk_css_calc_value_parse (parser, flags);
  if (_ctk_css_parser_has_prefix (parser, "-gtk-win32-size") ||
      _ctk_css_parser_has_prefix (parser, "-gtk-win32-part-width") ||
      _ctk_css_parser_has_prefix (parser, "-gtk-win32-part-height") ||
      _ctk_css_parser_has_prefix (parser, "-gtk-win32-part-border-top") ||
      _ctk_css_parser_has_prefix (parser, "-gtk-win32-part-border-left") ||
      _ctk_css_parser_has_prefix (parser, "-gtk-win32-part-border-bottom") ||
      _ctk_css_parser_has_prefix (parser, "-gtk-win32-part-border-right"))
    return ctk_css_win32_size_value_parse (parser, flags);

  return ctk_css_dimension_value_parse (parser, flags);
}

double
_ctk_css_number_value_get (const GtkCssValue *number,
                           double             one_hundred_percent)
{
  GtkCssNumberValueClass *number_value_class;

  g_return_val_if_fail (number != NULL, 0.0);

  number_value_class = (GtkCssNumberValueClass *) number->class;

  return number_value_class->get (number, one_hundred_percent);
}

