/* CTK - The GIMP Toolkit
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

#include "ctkcssnumbervalueprivate.h"

#include "ctkcsscalcvalueprivate.h"
#include "ctkcssdimensionvalueprivate.h"
#include "ctkcsswin32sizevalueprivate.h"

struct _CtkCssValue {
  CTK_CSS_VALUE_BASE
};

CtkCssDimension
ctk_css_number_value_get_dimension (const CtkCssValue *value)
{
  CtkCssNumberValueClass *number_value_class = (CtkCssNumberValueClass *) value->class;

  return number_value_class->get_dimension (value);
}

gboolean
ctk_css_number_value_has_percent (const CtkCssValue *value)
{
  CtkCssNumberValueClass *number_value_class = (CtkCssNumberValueClass *) value->class;

  return number_value_class->has_percent (value);
}

CtkCssValue *
ctk_css_number_value_multiply (const CtkCssValue *value,
                               double             factor)
{
  CtkCssNumberValueClass *number_value_class = (CtkCssNumberValueClass *) value->class;

  return number_value_class->multiply (value, factor);
}

CtkCssValue *
ctk_css_number_value_add (CtkCssValue *value1,
                          CtkCssValue *value2)
{
  CtkCssValue *sum;

  sum = ctk_css_number_value_try_add (value1, value2);
  if (sum == NULL)
    sum = ctk_css_calc_value_new_sum (value1, value2);

  return sum;
}

CtkCssValue *
ctk_css_number_value_try_add (const CtkCssValue *value1,
                              const CtkCssValue *value2)
{
  CtkCssNumberValueClass *number_value_class;
  
  if (value1->class != value2->class)
    return NULL;

  number_value_class = (CtkCssNumberValueClass *) value1->class;

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
ctk_css_number_value_get_calc_term_order (const CtkCssValue *value)
{
  CtkCssNumberValueClass *number_value_class = (CtkCssNumberValueClass *) value->class;

  return number_value_class->get_calc_term_order (value);
}

CtkCssValue *
_ctk_css_number_value_new (double     value,
                           CtkCssUnit unit)
{
  return ctk_css_dimension_value_new (value, unit);
}

CtkCssValue *
ctk_css_number_value_transition (CtkCssValue *start,
                                 CtkCssValue *end,
                                 guint        property_id,
                                 double       progress)
{
  CtkCssValue *result, *mul_start, *mul_end;

  mul_start = ctk_css_number_value_multiply (start, 1 - progress);
  mul_end = ctk_css_number_value_multiply (end, progress);

  result = ctk_css_number_value_add (mul_start, mul_end);

  _ctk_css_value_unref (mul_start);
  _ctk_css_value_unref (mul_end);

  return result;
}

gboolean
ctk_css_number_value_can_parse (CtkCssParser *parser)
{
  return _ctk_css_parser_has_number (parser)
      || _ctk_css_parser_has_prefix (parser, "calc")
      || _ctk_css_parser_has_prefix (parser, "-ctk-win32-size")
      || _ctk_css_parser_has_prefix (parser, "-ctk-win32-part-width")
      || _ctk_css_parser_has_prefix (parser, "-ctk-win32-part-height")
      || _ctk_css_parser_has_prefix (parser, "-ctk-win32-part-border-top")
      || _ctk_css_parser_has_prefix (parser, "-ctk-win32-part-border-left")
      || _ctk_css_parser_has_prefix (parser, "-ctk-win32-part-border-bottom")
      || _ctk_css_parser_has_prefix (parser, "-ctk-win32-part-border-right");
}

CtkCssValue *
_ctk_css_number_value_parse (CtkCssParser           *parser,
                             CtkCssNumberParseFlags  flags)
{
  if (_ctk_css_parser_has_prefix (parser, "calc"))
    return ctk_css_calc_value_parse (parser, flags);
  if (_ctk_css_parser_has_prefix (parser, "-ctk-win32-size") ||
      _ctk_css_parser_has_prefix (parser, "-ctk-win32-part-width") ||
      _ctk_css_parser_has_prefix (parser, "-ctk-win32-part-height") ||
      _ctk_css_parser_has_prefix (parser, "-ctk-win32-part-border-top") ||
      _ctk_css_parser_has_prefix (parser, "-ctk-win32-part-border-left") ||
      _ctk_css_parser_has_prefix (parser, "-ctk-win32-part-border-bottom") ||
      _ctk_css_parser_has_prefix (parser, "-ctk-win32-part-border-right"))
    return ctk_css_win32_size_value_parse (parser, flags);

  return ctk_css_dimension_value_parse (parser, flags);
}

double
_ctk_css_number_value_get (const CtkCssValue *number,
                           double             one_hundred_percent)
{
  CtkCssNumberValueClass *number_value_class;

  g_return_val_if_fail (number != NULL, 0.0);

  number_value_class = (CtkCssNumberValueClass *) number->class;

  return number_value_class->get (number, one_hundred_percent);
}

