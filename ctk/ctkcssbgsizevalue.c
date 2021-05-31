/* GTK - The GIMP Toolkit
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

#include "ctkcssbgsizevalueprivate.h"

#include "ctkcssnumbervalueprivate.h"

struct _CtkCssValue {
  CTK_CSS_VALUE_BASE
  guint cover :1;
  guint contain :1;
  CtkCssValue *x;
  CtkCssValue *y;
};

static void
ctk_css_value_bg_size_free (CtkCssValue *value)
{
  if (value->x)
    _ctk_css_value_unref (value->x);
  if (value->y)
    _ctk_css_value_unref (value->y);

  g_slice_free (CtkCssValue, value);
}

static CtkCssValue *
ctk_css_value_bg_size_compute (CtkCssValue             *value,
                               guint                    property_id,
                               CtkStyleProviderPrivate *provider,
                               CtkCssStyle             *style,
                               CtkCssStyle             *parent_style)
{
  CtkCssValue *x, *y;

  if (value->x == NULL && value->y == NULL)
    return _ctk_css_value_ref (value);

  x = y = NULL;

  if (value->x)
    x = _ctk_css_value_compute (value->x, property_id, provider, style, parent_style);

  if (value->y)
    y = _ctk_css_value_compute (value->y, property_id, provider, style, parent_style);

  if (x == value->x && y == value->y)
    {
      if (x)
        _ctk_css_value_unref (x);
      if (y)
        _ctk_css_value_unref (y);

      return _ctk_css_value_ref (value);
    }

  return _ctk_css_bg_size_value_new (value->x ? x : NULL,
                                     value->y ? y : NULL);
}

static gboolean
ctk_css_value_bg_size_equal (const CtkCssValue *value1,
                             const CtkCssValue *value2)
{
  return value1->cover == value2->cover &&
         value1->contain == value2->contain &&
         (value1->x == value2->x ||
          (value1->x != NULL && value2->x != NULL &&
           _ctk_css_value_equal (value1->x, value2->x))) &&
         (value1->y == value2->y ||
          (value1->y != NULL && value2->y != NULL &&
           _ctk_css_value_equal (value1->y, value2->y)));
}

static CtkCssValue *
ctk_css_value_bg_size_transition (CtkCssValue *start,
                                  CtkCssValue *end,
                                  guint        property_id,
                                  double       progress)
{
  CtkCssValue *x, *y;

  if (start->cover)
    return end->cover ? _ctk_css_value_ref (end) : NULL;
  if (start->contain)
    return end->contain ? _ctk_css_value_ref (end) : NULL;

  if ((start->x != NULL) ^ (end->x != NULL) ||
      (start->y != NULL) ^ (end->y != NULL))
    return NULL;

  if (start->x)
    {
      x = _ctk_css_value_transition (start->x, end->x, property_id, progress);
      if (x == NULL)
        return NULL;
    }
  else
    x = NULL;

  if (start->y)
    {
      y = _ctk_css_value_transition (start->y, end->y, property_id, progress);
      if (y == NULL)
        {
          _ctk_css_value_unref (x);
          return NULL;
        }
    }
  else
    y = NULL;

  return _ctk_css_bg_size_value_new (x, y);
}

static void
ctk_css_value_bg_size_print (const CtkCssValue *value,
                             GString           *string)
{
  if (value->cover)
    g_string_append (string, "cover");
  else if (value->contain)
    g_string_append (string, "contain");
  else
    {
      if (value->x == NULL)
        g_string_append (string, "auto");
      else
        _ctk_css_value_print (value->x, string);

      if (value->y)
        {
          g_string_append_c (string, ' ');
          _ctk_css_value_print (value->y, string);
        }
    }
}

static const CtkCssValueClass CTK_CSS_VALUE_BG_SIZE = {
  ctk_css_value_bg_size_free,
  ctk_css_value_bg_size_compute,
  ctk_css_value_bg_size_equal,
  ctk_css_value_bg_size_transition,
  ctk_css_value_bg_size_print
};

static CtkCssValue auto_singleton = { &CTK_CSS_VALUE_BG_SIZE, 1, FALSE, FALSE, NULL, NULL };
static CtkCssValue cover_singleton = { &CTK_CSS_VALUE_BG_SIZE, 1, TRUE, FALSE, NULL, NULL };
static CtkCssValue contain_singleton = { &CTK_CSS_VALUE_BG_SIZE, 1, FALSE, TRUE, NULL, NULL };

CtkCssValue *
_ctk_css_bg_size_value_new (CtkCssValue *x,
                            CtkCssValue *y)
{
  CtkCssValue *result;

  if (x == NULL && y == NULL)
    return _ctk_css_value_ref (&auto_singleton);

  result = _ctk_css_value_new (CtkCssValue, &CTK_CSS_VALUE_BG_SIZE);
  result->x = x;
  result->y = y;

  return result;
}

CtkCssValue *
_ctk_css_bg_size_value_parse (CtkCssParser *parser)
{
  CtkCssValue *x, *y;

  if (_ctk_css_parser_try (parser, "cover", TRUE))
    return _ctk_css_value_ref (&cover_singleton);
  else if (_ctk_css_parser_try (parser, "contain", TRUE))
    return _ctk_css_value_ref (&contain_singleton);

  if (_ctk_css_parser_try (parser, "auto", TRUE))
    x = NULL;
  else
    {
      x = _ctk_css_number_value_parse (parser,
                                       CTK_CSS_POSITIVE_ONLY
                                       | CTK_CSS_PARSE_PERCENT
                                       | CTK_CSS_PARSE_LENGTH);
      if (x == NULL)
        return NULL;
    }

  if (_ctk_css_parser_try (parser, "auto", TRUE))
    y = NULL;
  else if (!ctk_css_number_value_can_parse (parser))
    y = NULL;
  else
    {
      y = _ctk_css_number_value_parse (parser,
                                       CTK_CSS_POSITIVE_ONLY
                                       | CTK_CSS_PARSE_PERCENT
                                       | CTK_CSS_PARSE_LENGTH);
      if (y == NULL)
        {
          _ctk_css_value_unref (x);
          return NULL;
        }
    }

  return _ctk_css_bg_size_value_new (x, y);
}

static void
ctk_css_bg_size_compute_size_for_cover_contain (gboolean     cover,
                                                CtkCssImage *image,
                                                double       width,
                                                double       height,
                                                double      *concrete_width,
                                                double      *concrete_height)
{
  double aspect, image_aspect;
  
  image_aspect = _ctk_css_image_get_aspect_ratio (image);
  if (image_aspect == 0.0)
    {
      *concrete_width = width;
      *concrete_height = height;
      return;
    }

  aspect = width / height;

  if ((aspect >= image_aspect && cover) ||
      (aspect < image_aspect && !cover))
    {
      *concrete_width = width;
      *concrete_height = width / image_aspect;
    }
  else
    {
      *concrete_height = height;
      *concrete_width = height * image_aspect;
    }
}

void
_ctk_css_bg_size_value_compute_size (const CtkCssValue *value,
                                     CtkCssImage       *image,
                                     double             area_width,
                                     double             area_height,
                                     double            *out_width,
                                     double            *out_height)
{
  g_return_if_fail (value->class == &CTK_CSS_VALUE_BG_SIZE);

  if (value->contain || value->cover)
    {
      ctk_css_bg_size_compute_size_for_cover_contain (value->cover,
                                                      image,
                                                      area_width, area_height,
                                                      out_width, out_height);
    }
  else
    {
      double x, y;

      /* note: 0 does the right thing later for 'auto' */
      x = value->x ? _ctk_css_number_value_get (value->x, area_width) : 0;
      y = value->y ? _ctk_css_number_value_get (value->y, area_height) : 0;

      if ((x <= 0 && value->x) ||
          (y <= 0 && value->y))
        {
          *out_width = 0;
          *out_height = 0;
        }
      else
        {
          _ctk_css_image_get_concrete_size (image,
                                            x, y,
                                            area_width, area_height,
                                            out_width, out_height);
        }
    }
}

