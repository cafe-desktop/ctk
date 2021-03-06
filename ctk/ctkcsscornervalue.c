/* CTK - The GIMP Toolkit
 * Copyright (C) 2011 Red Hat, Inc.
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

#include "ctkcsscornervalueprivate.h"

#include "ctkcssnumbervalueprivate.h"

struct _CtkCssValue {
  CTK_CSS_VALUE_BASE
  CtkCssValue *x;
  CtkCssValue *y;
};

static void
ctk_css_value_corner_free (CtkCssValue *value)
{
  _ctk_css_value_unref (value->x);
  _ctk_css_value_unref (value->y);

  g_slice_free (CtkCssValue, value);
}

static CtkCssValue *
ctk_css_value_corner_compute (CtkCssValue             *corner,
                              guint                    property_id,
                              CtkStyleProviderPrivate *provider,
                              CtkCssStyle             *style,
                              CtkCssStyle             *parent_style)
{
  CtkCssValue *x, *y;

  x = _ctk_css_value_compute (corner->x, property_id, provider, style, parent_style);
  y = _ctk_css_value_compute (corner->y, property_id, provider, style, parent_style);
  if (x == corner->x && y == corner->y)
    {
      _ctk_css_value_unref (x);
      _ctk_css_value_unref (y);
      return _ctk_css_value_ref (corner);
    }

  return _ctk_css_corner_value_new (x, y);
}

static gboolean
ctk_css_value_corner_equal (const CtkCssValue *corner1,
                            const CtkCssValue *corner2)
{
  return _ctk_css_value_equal (corner1->x, corner2->x)
      && _ctk_css_value_equal (corner1->y, corner2->y);
}

static CtkCssValue *
ctk_css_value_corner_transition (CtkCssValue *start,
                                 CtkCssValue *end,
                                 guint        property_id,
                                 double       progress)
{
  CtkCssValue *x, *y;

  x = _ctk_css_value_transition (start->x, end->x, property_id, progress);
  if (x == NULL)
    return NULL;
  y = _ctk_css_value_transition (start->y, end->y, property_id, progress);
  if (y == NULL)
    {
      _ctk_css_value_unref (x);
      return NULL;
    }

  return _ctk_css_corner_value_new (x, y);
}

static void
ctk_css_value_corner_print (const CtkCssValue *corner,
                           GString           *string)
{
  _ctk_css_value_print (corner->x, string);
  if (!_ctk_css_value_equal (corner->x, corner->y))
    {
      g_string_append_c (string, ' ');
      _ctk_css_value_print (corner->y, string);
    }
}

static const CtkCssValueClass CTK_CSS_VALUE_CORNER = {
  ctk_css_value_corner_free,
  ctk_css_value_corner_compute,
  ctk_css_value_corner_equal,
  ctk_css_value_corner_transition,
  ctk_css_value_corner_print
};

CtkCssValue *
_ctk_css_corner_value_new (CtkCssValue *x,
                           CtkCssValue *y)
{
  CtkCssValue *result;

  result = _ctk_css_value_new (CtkCssValue, &CTK_CSS_VALUE_CORNER);
  result->x = x;
  result->y = y;

  return result;
}

CtkCssValue *
_ctk_css_corner_value_parse (CtkCssParser *parser)
{
  CtkCssValue *x, *y;

  x = _ctk_css_number_value_parse (parser,
                                   CTK_CSS_POSITIVE_ONLY
                                   | CTK_CSS_PARSE_PERCENT
                                   | CTK_CSS_NUMBER_AS_PIXELS
                                   | CTK_CSS_PARSE_LENGTH);
  if (x == NULL)
    return NULL;

  if (!ctk_css_number_value_can_parse (parser))
    y = _ctk_css_value_ref (x);
  else
    {
      y = _ctk_css_number_value_parse (parser,
                                       CTK_CSS_POSITIVE_ONLY
                                       | CTK_CSS_PARSE_PERCENT
                                       | CTK_CSS_NUMBER_AS_PIXELS
                                       | CTK_CSS_PARSE_LENGTH);
      if (y == NULL)
        {
          _ctk_css_value_unref (x);
          return NULL;
        }
    }

  return _ctk_css_corner_value_new (x, y);
}

double
_ctk_css_corner_value_get_x (const CtkCssValue *corner,
                             double             one_hundred_percent)
{
  g_return_val_if_fail (corner != NULL, 0.0);
  g_return_val_if_fail (corner->class == &CTK_CSS_VALUE_CORNER, 0.0);

  return _ctk_css_number_value_get (corner->x, one_hundred_percent);
}

double
_ctk_css_corner_value_get_y (const CtkCssValue *corner,
                             double             one_hundred_percent)
{
  g_return_val_if_fail (corner != NULL, 0.0);
  g_return_val_if_fail (corner->class == &CTK_CSS_VALUE_CORNER, 0.0);

  return _ctk_css_number_value_get (corner->y, one_hundred_percent);
}

