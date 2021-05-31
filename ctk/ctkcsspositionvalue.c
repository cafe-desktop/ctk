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

#include "ctkcsspositionvalueprivate.h"

#include "ctkcssnumbervalueprivate.h"

struct _CtkCssValue {
  CTK_CSS_VALUE_BASE
  CtkCssValue *x;
  CtkCssValue *y;
};

static void
ctk_css_value_position_free (CtkCssValue *value)
{
  _ctk_css_value_unref (value->x);
  _ctk_css_value_unref (value->y);

  g_slice_free (CtkCssValue, value);
}

static CtkCssValue *
ctk_css_value_position_compute (CtkCssValue             *position,
                                guint                    property_id,
                                CtkStyleProviderPrivate *provider,
                                CtkCssStyle             *style,
                                CtkCssStyle             *parent_style)
{
  CtkCssValue *x, *y;

  x = _ctk_css_value_compute (position->x, property_id, provider, style, parent_style);
  y = _ctk_css_value_compute (position->y, property_id, provider, style, parent_style);
  if (x == position->x && y == position->y)
    {
      _ctk_css_value_unref (x);
      _ctk_css_value_unref (y);
      return _ctk_css_value_ref (position);
    }

  return _ctk_css_position_value_new (x, y);
}

static gboolean
ctk_css_value_position_equal (const CtkCssValue *position1,
                              const CtkCssValue *position2)
{
  return _ctk_css_value_equal (position1->x, position2->x)
      && _ctk_css_value_equal (position1->y, position2->y);
}

static CtkCssValue *
ctk_css_value_position_transition (CtkCssValue *start,
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

  return _ctk_css_position_value_new (x, y);
}

static void
ctk_css_value_position_print (const CtkCssValue *position,
                              GString           *string)
{
  struct {
    const char *x_name;
    const char *y_name;
    CtkCssValue *number;
  } values[] = { 
    { "left",   "top",    _ctk_css_number_value_new (0, CTK_CSS_PERCENT) },
    { "right",  "bottom", _ctk_css_number_value_new (100, CTK_CSS_PERCENT) }
  };
  CtkCssValue *center = _ctk_css_number_value_new (50, CTK_CSS_PERCENT);
  guint i;

  if (_ctk_css_value_equal (position->x, center))
    {
      if (_ctk_css_value_equal (position->y, center))
        {
          g_string_append (string, "center");
          goto done;
        }
    }
  else
    {
      for (i = 0; i < G_N_ELEMENTS (values); i++)
        {
          if (_ctk_css_value_equal (position->x, values[i].number))
            {
              g_string_append (string, values[i].x_name);
              break;
            }
        }
      if (i == G_N_ELEMENTS (values))
        _ctk_css_value_print (position->x, string);

      if (_ctk_css_value_equal (position->y, center))
        goto done;

      g_string_append_c (string, ' ');
    }

  for (i = 0; i < G_N_ELEMENTS (values); i++)
    {
      if (_ctk_css_value_equal (position->y, values[i].number))
        {
          g_string_append (string, values[i].y_name);
          goto done;
        }
    }
  if (i == G_N_ELEMENTS (values))
    {
      if (_ctk_css_value_equal (position->x, center))
        g_string_append (string, "center ");
      _ctk_css_value_print (position->y, string);
    }

done:
  for (i = 0; i < G_N_ELEMENTS (values); i++)
    _ctk_css_value_unref (values[i].number);
  _ctk_css_value_unref (center);
}

static const CtkCssValueClass CTK_CSS_VALUE_POSITION = {
  ctk_css_value_position_free,
  ctk_css_value_position_compute,
  ctk_css_value_position_equal,
  ctk_css_value_position_transition,
  ctk_css_value_position_print
};

CtkCssValue *
_ctk_css_position_value_new (CtkCssValue *x,
                             CtkCssValue *y)
{
  CtkCssValue *result;

  result = _ctk_css_value_new (CtkCssValue, &CTK_CSS_VALUE_POSITION);
  result->x = x;
  result->y = y;

  return result;
}

static CtkCssValue *
position_value_parse (CtkCssParser *parser, gboolean try)
{
  static const struct {
    const char *name;
    guint       percentage;
    gboolean    horizontal;
    gboolean    vertical;
  } names[] = {
    { "left",     0, TRUE,  FALSE },
    { "right",  100, TRUE,  FALSE },
    { "center",  50, TRUE,  TRUE  },
    { "top",      0, FALSE, TRUE  },
    { "bottom", 100, FALSE, TRUE  },
    { NULL    ,   0, TRUE,  FALSE }, /* used for numbers */
    { NULL    ,  50, TRUE,  TRUE  }  /* used for no value */
  };
  CtkCssValue *x, *y;
  CtkCssValue **missing;
  guint first, second;

  for (first = 0; names[first].name != NULL; first++)
    {
      if (_ctk_css_parser_try (parser, names[first].name, TRUE))
        {
          if (names[first].horizontal)
            {
	      x = _ctk_css_number_value_new (names[first].percentage, CTK_CSS_PERCENT);
              missing = &y;
            }
          else
            {
	      y = _ctk_css_number_value_new (names[first].percentage, CTK_CSS_PERCENT);
              missing = &x;
            }
          break;
        }
    }
  if (names[first].name == NULL)
    {
      if (ctk_css_number_value_can_parse (parser))
        {
          missing = &y;
          x = _ctk_css_number_value_parse (parser,
                                           CTK_CSS_PARSE_PERCENT
                                           | CTK_CSS_PARSE_LENGTH);

          if (x == NULL)
            return NULL;
        }
      else
        {
          if (!try)
            _ctk_css_parser_error (parser, "Unrecognized position value");
          return NULL;
        }
    }

  for (second = 0; names[second].name != NULL; second++)
    {
      if (_ctk_css_parser_try (parser, names[second].name, TRUE))
        {
	  *missing = _ctk_css_number_value_new (names[second].percentage, CTK_CSS_PERCENT);
          break;
        }
    }

  if (names[second].name == NULL)
    {
      if (ctk_css_number_value_can_parse (parser))
        {
          if (missing != &y)
            {
              if (!try)
                _ctk_css_parser_error (parser, "Invalid combination of values");
              _ctk_css_value_unref (y);
              return NULL;
            }
          y = _ctk_css_number_value_parse (parser,
                                           CTK_CSS_PARSE_PERCENT
                                           | CTK_CSS_PARSE_LENGTH);
          if (y == NULL)
            {
              _ctk_css_value_unref (x);
	      return NULL;
            }
        }
      else
        {
          second++;
          *missing = _ctk_css_number_value_new (50, CTK_CSS_PERCENT);
        }
    }
  else
    {
      if ((names[first].horizontal && !names[second].vertical) ||
          (!names[first].horizontal && !names[second].horizontal))
        {
          if (!try)
            _ctk_css_parser_error (parser, "Invalid combination of values");
          _ctk_css_value_unref (x);
          _ctk_css_value_unref (y);
          return NULL;
        }
    }

  return _ctk_css_position_value_new (x, y);
}

CtkCssValue *
_ctk_css_position_value_parse (CtkCssParser *parser)
{
  return position_value_parse (parser, FALSE);
}

CtkCssValue *
_ctk_css_position_value_try_parse (CtkCssParser *parser)
{
  return position_value_parse (parser, TRUE);
}

double
_ctk_css_position_value_get_x (const CtkCssValue *position,
                               double             one_hundred_percent)
{
  g_return_val_if_fail (position != NULL, 0.0);
  g_return_val_if_fail (position->class == &CTK_CSS_VALUE_POSITION, 0.0);

  return _ctk_css_number_value_get (position->x, one_hundred_percent);
}

double
_ctk_css_position_value_get_y (const CtkCssValue *position,
                               double             one_hundred_percent)
{
  g_return_val_if_fail (position != NULL, 0.0);
  g_return_val_if_fail (position->class == &CTK_CSS_VALUE_POSITION, 0.0);

  return _ctk_css_number_value_get (position->y, one_hundred_percent);
}

