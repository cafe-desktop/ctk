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
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "ctkcssimagelinearprivate.h"

#include <math.h>

#include "ctkcsscolorvalueprivate.h"
#include "ctkcssnumbervalueprivate.h"
#include "ctkcssrgbavalueprivate.h"
#include "ctkcssprovider.h"

G_DEFINE_TYPE (CtkCssImageLinear, _ctk_css_image_linear, CTK_TYPE_CSS_IMAGE)

static void
ctk_css_image_linear_get_start_end (CtkCssImageLinear *linear,
                                    double             length,
                                    double            *start,
                                    double            *end)
{
  CtkCssImageLinearColorStop *stop;
  double pos;
  guint i;
      
  if (linear->repeating)
    {
      stop = &g_array_index (linear->stops, CtkCssImageLinearColorStop, 0);
      if (stop->offset == NULL)
        *start = 0;
      else
        *start = _ctk_css_number_value_get (stop->offset, length) / length;

      *end = *start;

      for (i = 0; i < linear->stops->len; i++)
        {
          stop = &g_array_index (linear->stops, CtkCssImageLinearColorStop, i);
          
          if (stop->offset == NULL)
            continue;

          pos = _ctk_css_number_value_get (stop->offset, length) / length;

          *end = MAX (pos, *end);
        }
      
      if (stop->offset == NULL)
        *end = MAX (*end, 1.0);
    }
  else
    {
      *start = 0;
      *end = 1;
    }
}

static void
ctk_css_image_linear_compute_start_point (double angle_in_degrees,
                                          double width,
                                          double height,
                                          double *x,
                                          double *y)
{
  double angle, c, slope, perpendicular;
  
  angle = fmod (angle_in_degrees, 360);
  if (angle < 0)
    angle += 360;

  if (angle == 0)
    {
      *x = 0;
      *y = -height;
      return;
    }
  else if (angle == 90)
    {
      *x = width;
      *y = 0;
      return;
    }
  else if (angle == 180)
    {
      *x = 0;
      *y = height;
      return;
    }
  else if (angle == 270)
    {
      *x = -width;
      *y = 0;
      return;
    }

  /* The tan() is confusing because the angle is clockwise
   * from 'to top' */
  perpendicular = tan (angle * G_PI / 180);
  slope = -1 / perpendicular;

  if (angle > 180)
    width = - width;
  if (angle < 90 || angle > 270)
    height = - height;
  
  /* Compute c (of y = mx + c) of perpendicular */
  c = height - perpendicular * width;

  *x = c / (slope - perpendicular);
  *y = perpendicular * *x + c;
}
                                         
static void
ctk_css_image_linear_draw (CtkCssImage        *image,
                           cairo_t            *cr,
                           double              width,
                           double              height)
{
  CtkCssImageLinear *linear = CTK_CSS_IMAGE_LINEAR (image);
  cairo_pattern_t *pattern;
  double angle; /* actual angle of the gradiant line in degrees */
  double x, y; /* coordinates of start point */
  double length; /* distance in pixels for 100% */
  double start, end; /* position of first/last point on gradient line - with gradient line being [0, 1] */
  double offset;
  int i, last;

  if (linear->side)
    {
      /* special casing the regular cases here so we don't get rounding errors */
      switch (linear->side)
      {
        case 1 << CTK_CSS_RIGHT:
          angle = 90;
          break;
        case 1 << CTK_CSS_LEFT:
          angle = 270;
          break;
        case 1 << CTK_CSS_TOP:
          angle = 0;
          break;
        case 1 << CTK_CSS_BOTTOM:
          angle = 180;
          break;
        default:
          angle = atan2 (linear->side & 1 << CTK_CSS_TOP ? -width : width,
                         linear->side & 1 << CTK_CSS_LEFT ? -height : height);
          angle = 180 * angle / G_PI + 90;
          break;
      }
    }
  else
    {
      angle = _ctk_css_number_value_get (linear->angle, 100);
    }

  ctk_css_image_linear_compute_start_point (angle,
                                            width, height,
                                            &x, &y);

  length = sqrt (x * x + y * y);
  ctk_css_image_linear_get_start_end (linear, length, &start, &end);
  pattern = cairo_pattern_create_linear (x * (start - 0.5), y * (start - 0.5),
                                         x * (end - 0.5),   y * (end - 0.5));
  if (linear->repeating)
    cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
  else
    cairo_pattern_set_extend (pattern, CAIRO_EXTEND_PAD);

  offset = start;
  last = -1;
  for (i = 0; i < linear->stops->len; i++)
    {
      CtkCssImageLinearColorStop *stop;
      double pos, step;
      
      stop = &g_array_index (linear->stops, CtkCssImageLinearColorStop, i);

      if (stop->offset == NULL)
        {
          if (i == 0)
            pos = 0.0;
          else if (i + 1 == linear->stops->len)
            pos = 1.0;
          else
            continue;
        }
      else
        pos = _ctk_css_number_value_get (stop->offset, length) / length;

      pos = MAX (pos, offset);
      step = (pos - offset) / (i - last);
      for (last = last + 1; last <= i; last++)
        {
          const CdkRGBA *rgba;

          stop = &g_array_index (linear->stops, CtkCssImageLinearColorStop, last);

          rgba = _ctk_css_rgba_value_get_rgba (stop->color);
          offset += step;

          cairo_pattern_add_color_stop_rgba (pattern,
                                             (offset - start) / (end - start),
                                             rgba->red,
                                             rgba->green,
                                             rgba->blue,
                                             rgba->alpha);
        }

      offset = pos;
      last = i;
    }

  cairo_rectangle (cr, 0, 0, width, height);
  cairo_translate (cr, width / 2, height / 2);
  cairo_set_source (cr, pattern);
  cairo_fill (cr);

  cairo_pattern_destroy (pattern);
}


static gboolean
ctk_css_image_linear_parse (CtkCssImage  *image,
                            CtkCssParser *parser)
{
  CtkCssImageLinear *linear = CTK_CSS_IMAGE_LINEAR (image);
  guint i;

  if (_ctk_css_parser_try (parser, "repeating-linear-gradient(", TRUE))
    linear->repeating = TRUE;
  else if (_ctk_css_parser_try (parser, "linear-gradient(", TRUE))
    linear->repeating = FALSE;
  else
    {
      _ctk_css_parser_error (parser, "Not a linear gradient");
      return FALSE;
    }

  if (_ctk_css_parser_try (parser, "to", TRUE))
    {
      for (i = 0; i < 2; i++)
        {
          if (_ctk_css_parser_try (parser, "left", TRUE))
            {
              if (linear->side & ((1 << CTK_CSS_LEFT) | (1 << CTK_CSS_RIGHT)))
                {
                  _ctk_css_parser_error (parser, "Expected 'top', 'bottom' or comma");
                  return FALSE;
                }
              linear->side |= (1 << CTK_CSS_LEFT);
            }
          else if (_ctk_css_parser_try (parser, "right", TRUE))
            {
              if (linear->side & ((1 << CTK_CSS_LEFT) | (1 << CTK_CSS_RIGHT)))
                {
                  _ctk_css_parser_error (parser, "Expected 'top', 'bottom' or comma");
                  return FALSE;
                }
              linear->side |= (1 << CTK_CSS_RIGHT);
            }
          else if (_ctk_css_parser_try (parser, "top", TRUE))
            {
              if (linear->side & ((1 << CTK_CSS_TOP) | (1 << CTK_CSS_BOTTOM)))
                {
                  _ctk_css_parser_error (parser, "Expected 'left', 'right' or comma");
                  return FALSE;
                }
              linear->side |= (1 << CTK_CSS_TOP);
            }
          else if (_ctk_css_parser_try (parser, "bottom", TRUE))
            {
              if (linear->side & ((1 << CTK_CSS_TOP) | (1 << CTK_CSS_BOTTOM)))
                {
                  _ctk_css_parser_error (parser, "Expected 'left', 'right' or comma");
                  return FALSE;
                }
              linear->side |= (1 << CTK_CSS_BOTTOM);
            }
          else
            break;
        }

      if (linear->side == 0)
        {
          _ctk_css_parser_error (parser, "Expected side that gradient should go to");
          return FALSE;
        }

      if (!_ctk_css_parser_try (parser, ",", TRUE))
        {
          _ctk_css_parser_error (parser, "Expected a comma");
          return FALSE;
        }
    }
  else if (ctk_css_number_value_can_parse (parser))
    {
      linear->angle = _ctk_css_number_value_parse (parser, CTK_CSS_PARSE_ANGLE);
      if (linear->angle == NULL)
        return FALSE;

      if (!_ctk_css_parser_try (parser, ",", TRUE))
        {
          _ctk_css_parser_error (parser, "Expected a comma");
          return FALSE;
        }
    }
  else
    linear->side = 1 << CTK_CSS_BOTTOM;

  do {
    CtkCssImageLinearColorStop stop;

    stop.color = _ctk_css_color_value_parse (parser);
    if (stop.color == NULL)
      return FALSE;

    if (ctk_css_number_value_can_parse (parser))
      {
        stop.offset = _ctk_css_number_value_parse (parser,
                                                   CTK_CSS_PARSE_PERCENT
                                                   | CTK_CSS_PARSE_LENGTH);
        if (stop.offset == NULL)
          {
            _ctk_css_value_unref (stop.color);
            return FALSE;
          }
      }
    else
      {
        stop.offset = NULL;
      }

    g_array_append_val (linear->stops, stop);

  } while (_ctk_css_parser_try (parser, ",", TRUE));

  if (linear->stops->len < 2)
    {
      _ctk_css_parser_error_full (parser,
                                  CTK_CSS_PROVIDER_ERROR_DEPRECATED,
                                  "Using one color stop with %s() is deprecated.",
                                  linear->repeating ? "repeating-linear-gradient" : "linear-gradient");
    }

  if (!_ctk_css_parser_try (parser, ")", TRUE))
    {
      _ctk_css_parser_error (parser, "Missing closing bracket at end of linear gradient");
      return FALSE;
    }

  return TRUE;
}

static void
ctk_css_image_linear_print (CtkCssImage *image,
                            GString     *string)
{
  CtkCssImageLinear *linear = CTK_CSS_IMAGE_LINEAR (image);
  guint i;

  if (linear->repeating)
    g_string_append (string, "repeating-linear-gradient(");
  else
    g_string_append (string, "linear-gradient(");

  if (linear->side)
    {
      if (linear->side != (1 << CTK_CSS_BOTTOM))
        {
          g_string_append (string, "to");

          if (linear->side & (1 << CTK_CSS_TOP))
            g_string_append (string, " top");
          else if (linear->side & (1 << CTK_CSS_BOTTOM))
            g_string_append (string, " bottom");

          if (linear->side & (1 << CTK_CSS_LEFT))
            g_string_append (string, " left");
          else if (linear->side & (1 << CTK_CSS_RIGHT))
            g_string_append (string, " right");

          g_string_append (string, ", ");
        }
    }
  else
    {
      _ctk_css_value_print (linear->angle, string);
      g_string_append (string, ", ");
    }

  for (i = 0; i < linear->stops->len; i++)
    {
      CtkCssImageLinearColorStop *stop;
      
      if (i > 0)
        g_string_append (string, ", ");

      stop = &g_array_index (linear->stops, CtkCssImageLinearColorStop, i);

      _ctk_css_value_print (stop->color, string);

      if (stop->offset)
        {
          g_string_append (string, " ");
          _ctk_css_value_print (stop->offset, string);
        }
    }
  
  g_string_append (string, ")");
}

static CtkCssImage *
ctk_css_image_linear_compute (CtkCssImage             *image,
                              guint                    property_id,
                              CtkStyleProviderPrivate *provider,
                              CtkCssStyle             *style,
                              CtkCssStyle             *parent_style)
{
  CtkCssImageLinear *linear = CTK_CSS_IMAGE_LINEAR (image);
  CtkCssImageLinear *copy;
  guint i;

  copy = g_object_new (CTK_TYPE_CSS_IMAGE_LINEAR, NULL);
  copy->repeating = linear->repeating;
  copy->side = linear->side;

  if (linear->angle)
    copy->angle = _ctk_css_value_compute (linear->angle, property_id, provider, style, parent_style);
  
  g_array_set_size (copy->stops, linear->stops->len);
  for (i = 0; i < linear->stops->len; i++)
    {
      CtkCssImageLinearColorStop *stop, *scopy;

      stop = &g_array_index (linear->stops, CtkCssImageLinearColorStop, i);
      scopy = &g_array_index (copy->stops, CtkCssImageLinearColorStop, i);
              
      scopy->color = _ctk_css_value_compute (stop->color, property_id, provider, style, parent_style);
      
      if (stop->offset)
        {
          scopy->offset = _ctk_css_value_compute (stop->offset, property_id, provider, style, parent_style);
        }
      else
        {
          scopy->offset = NULL;
        }
    }

  return CTK_CSS_IMAGE (copy);
}

static CtkCssImage *
ctk_css_image_linear_transition (CtkCssImage *start_image,
                                 CtkCssImage *end_image,
                                 guint        property_id,
                                 double       progress)
{
  CtkCssImageLinear *start, *end, *result;
  guint i;

  start = CTK_CSS_IMAGE_LINEAR (start_image);

  if (end_image == NULL)
    return CTK_CSS_IMAGE_CLASS (_ctk_css_image_linear_parent_class)->transition (start_image, end_image, property_id, progress);

  if (!CTK_IS_CSS_IMAGE_LINEAR (end_image))
    return CTK_CSS_IMAGE_CLASS (_ctk_css_image_linear_parent_class)->transition (start_image, end_image, property_id, progress);

  end = CTK_CSS_IMAGE_LINEAR (end_image);

  if ((start->repeating != end->repeating)
      || (start->stops->len != end->stops->len))
    return CTK_CSS_IMAGE_CLASS (_ctk_css_image_linear_parent_class)->transition (start_image, end_image, property_id, progress);

  result = g_object_new (CTK_TYPE_CSS_IMAGE_LINEAR, NULL);
  result->repeating = start->repeating;

  if (start->side != end->side)
    goto fail;

  result->side = start->side;
  if (result->side == 0)
    result->angle = _ctk_css_value_transition (start->angle, end->angle, property_id, progress);
  if (result->angle == NULL)
    goto fail;
  
  for (i = 0; i < start->stops->len; i++)
    {
      CtkCssImageLinearColorStop stop, *start_stop, *end_stop;

      start_stop = &g_array_index (start->stops, CtkCssImageLinearColorStop, i);
      end_stop = &g_array_index (end->stops, CtkCssImageLinearColorStop, i);

      if ((start_stop->offset != NULL) != (end_stop->offset != NULL))
        goto fail;

      if (start_stop->offset == NULL)
        {
          stop.offset = NULL;
        }
      else
        {
          stop.offset = _ctk_css_value_transition (start_stop->offset,
                                                   end_stop->offset,
                                                   property_id,
                                                   progress);
          if (stop.offset == NULL)
            goto fail;
        }

      stop.color = _ctk_css_value_transition (start_stop->color,
                                              end_stop->color,
                                              property_id,
                                              progress);
      if (stop.color == NULL)
        {
          if (stop.offset)
            _ctk_css_value_unref (stop.offset);
          goto fail;
        }

      g_array_append_val (result->stops, stop);
    }

  return CTK_CSS_IMAGE (result);

fail:
  g_object_unref (result);
  return CTK_CSS_IMAGE_CLASS (_ctk_css_image_linear_parent_class)->transition (start_image, end_image, property_id, progress);
}

static gboolean
ctk_css_image_linear_equal (CtkCssImage *image1,
                            CtkCssImage *image2)
{
  CtkCssImageLinear *linear1 = CTK_CSS_IMAGE_LINEAR (image1);
  CtkCssImageLinear *linear2 = CTK_CSS_IMAGE_LINEAR (image2);
  guint i;

  if (linear1->repeating != linear2->repeating ||
      linear1->side != linear2->side ||
      (linear1->side == 0 && !_ctk_css_value_equal (linear1->angle, linear2->angle)) ||
      linear1->stops->len != linear2->stops->len)
    return FALSE;

  for (i = 0; i < linear1->stops->len; i++)
    {
      CtkCssImageLinearColorStop *stop1, *stop2;

      stop1 = &g_array_index (linear1->stops, CtkCssImageLinearColorStop, i);
      stop2 = &g_array_index (linear2->stops, CtkCssImageLinearColorStop, i);

      if (!_ctk_css_value_equal0 (stop1->offset, stop2->offset) ||
          !_ctk_css_value_equal (stop1->color, stop2->color))
        return FALSE;
    }

  return TRUE;
}

static void
ctk_css_image_linear_dispose (GObject *object)
{
  CtkCssImageLinear *linear = CTK_CSS_IMAGE_LINEAR (object);

  if (linear->stops)
    {
      g_array_free (linear->stops, TRUE);
      linear->stops = NULL;
    }

  linear->side = 0;
  if (linear->angle)
    {
      _ctk_css_value_unref (linear->angle);
      linear->angle = NULL;
    }

  G_OBJECT_CLASS (_ctk_css_image_linear_parent_class)->dispose (object);
}

static void
_ctk_css_image_linear_class_init (CtkCssImageLinearClass *klass)
{
  CtkCssImageClass *image_class = CTK_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->draw = ctk_css_image_linear_draw;
  image_class->parse = ctk_css_image_linear_parse;
  image_class->print = ctk_css_image_linear_print;
  image_class->compute = ctk_css_image_linear_compute;
  image_class->equal = ctk_css_image_linear_equal;
  image_class->transition = ctk_css_image_linear_transition;

  object_class->dispose = ctk_css_image_linear_dispose;
}

static void
ctk_css_image_clear_color_stop (gpointer color_stop)
{
  CtkCssImageLinearColorStop *stop = color_stop;

  _ctk_css_value_unref (stop->color);
  if (stop->offset)
    _ctk_css_value_unref (stop->offset);
}

static void
_ctk_css_image_linear_init (CtkCssImageLinear *linear)
{
  linear->stops = g_array_new (FALSE, FALSE, sizeof (CtkCssImageLinearColorStop));
  g_array_set_clear_func (linear->stops, ctk_css_image_clear_color_stop);
}

