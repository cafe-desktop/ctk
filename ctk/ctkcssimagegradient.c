/*
 * Copyright © 2011 Red Hat Inc.
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

#define CDK_DISABLE_DEPRECATION_WARNINGS

#include "ctkcssimagegradientprivate.h"

#include "ctkcssprovider.h"

#include "deprecated/ctkgradientprivate.h"
#include "deprecated/ctksymboliccolorprivate.h"

G_DEFINE_TYPE (CtkCssImageGradient, _ctk_css_image_gradient, CTK_TYPE_CSS_IMAGE)

static CtkCssImage *
ctk_css_image_gradient_compute (CtkCssImage             *image,
                                guint                    property_id,
                                CtkStyleProviderPrivate *provider,
                                CtkCssStyle             *style,
                                CtkCssStyle             *parent_style)
{
  CtkCssImageGradient *gradient = CTK_CSS_IMAGE_GRADIENT (image);
  CtkCssImageGradient *copy;

  if (gradient->pattern)
    return CTK_CSS_IMAGE (g_object_ref (gradient));

  copy = g_object_new (CTK_TYPE_CSS_IMAGE_GRADIENT, NULL);
  copy->gradient = ctk_gradient_ref (gradient->gradient);
  copy->pattern = _ctk_gradient_resolve_full (copy->gradient, provider, style, parent_style);

  return CTK_CSS_IMAGE (copy);
}

static cairo_pattern_t *
fade_pattern (cairo_pattern_t *pattern,
              double           opacity)
{
  double x0, y0, x1, y1, r0, r1;
  cairo_pattern_t *result;
  int i, n;

  switch (cairo_pattern_get_type (pattern))
    {
    case CAIRO_PATTERN_TYPE_LINEAR:
      cairo_pattern_get_linear_points (pattern, &x0, &y0, &x1, &y1);
      result = cairo_pattern_create_linear (x0, y0, x1, y1);
      break;
    case CAIRO_PATTERN_TYPE_RADIAL:
      cairo_pattern_get_radial_circles (pattern, &x0, &y0, &r0, &x1, &y1, &r1);
      result = cairo_pattern_create_radial (x0, y0, r0, x1, y1, r1);
      break;
    default:
      g_return_val_if_reached (NULL);
    }

  cairo_pattern_get_color_stop_count (pattern, &n);
  for (i = 0; i < n; i++)
    {
      double o, r, g, b, a;

      cairo_pattern_get_color_stop_rgba (pattern, i, &o, &r, &g, &b, &a);
      cairo_pattern_add_color_stop_rgba (result, o, r, g, b, a * opacity);
    }

  return result;
}

static cairo_pattern_t *
transition_pattern (cairo_pattern_t *start,
                    cairo_pattern_t *end,
                    double           progress)
{
  double sx0, sy0, sx1, sy1, sr0, sr1, ex0, ey0, ex1, ey1, er0, er1;
  cairo_pattern_t *result;
  int i, n;

  progress = CLAMP (progress, 0.0, 1.0);

  if (end == NULL)
    return fade_pattern (start, 1.0 - progress);

  g_assert (cairo_pattern_get_type (start) == cairo_pattern_get_type (end));

  switch (cairo_pattern_get_type (start))
    {
    case CAIRO_PATTERN_TYPE_LINEAR:
      cairo_pattern_get_linear_points (start, &sx0, &sy0, &sx1, &sy1);
      cairo_pattern_get_linear_points (end, &ex0, &ey0, &ex1, &ey1);
      result = cairo_pattern_create_linear ((1 - progress) * sx0 + progress * ex0,
                                            (1 - progress) * sx1 + progress * ex1,
                                            (1 - progress) * sy0 + progress * ey0,
                                            (1 - progress) * sy1 + progress * ey1);
      break;
    case CAIRO_PATTERN_TYPE_RADIAL:
      cairo_pattern_get_radial_circles (start, &sx0, &sy0, &sr0, &sx1, &sy1, &sr1);
      cairo_pattern_get_radial_circles (end, &ex0, &ey0, &er0, &ex1, &ey1, &er1);
      result = cairo_pattern_create_radial ((1 - progress) * sx0 + progress * ex0,
                                            (1 - progress) * sy0 + progress * ey0,
                                            (1 - progress) * sr0 + progress * er0,
                                            (1 - progress) * sx1 + progress * ex1,
                                            (1 - progress) * sy1 + progress * ey1,
                                            (1 - progress) * sr1 + progress * er1);
      break;
    default:
      g_return_val_if_reached (NULL);
    }

  cairo_pattern_get_color_stop_count (start, &n);
  for (i = 0; i < n; i++)
    {
      double so, sr, sg, sb, sa, eo, er, eg, eb, ea;

      cairo_pattern_get_color_stop_rgba (start, i, &so, &sr, &sg, &sb, &sa);
      cairo_pattern_get_color_stop_rgba (end, i, &eo, &er, &eg, &eb, &ea);

      cairo_pattern_add_color_stop_rgba (result,
                                         (1 - progress) * so + progress * eo,
                                         (1 - progress) * sr + progress * er,
                                         (1 - progress) * sg + progress * eg,
                                         (1 - progress) * sb + progress * eb,
                                         (1 - progress) * sa + progress * ea);
    }

  return result;
}

static CtkCssImage *
ctk_css_image_gradient_transition (CtkCssImage *start_image,
                                   CtkCssImage *end_image,
                                   guint        property_id,
                                   double       progress)
{
  CtkGradient *start_gradient, *end_gradient, *gradient;
  cairo_pattern_t *start_pattern, *end_pattern;
  CtkCssImageGradient *result;

  start_gradient = CTK_CSS_IMAGE_GRADIENT (start_image)->gradient;
  start_pattern = CTK_CSS_IMAGE_GRADIENT (start_image)->pattern;
  if (end_image == NULL)
    {
      end_gradient = NULL;
      end_pattern = NULL;
    }
  else
    {
      if (!CTK_IS_CSS_IMAGE_GRADIENT (end_image))
        return CTK_CSS_IMAGE_CLASS (_ctk_css_image_gradient_parent_class)->transition (start_image, end_image, property_id, progress);

      end_gradient = CTK_CSS_IMAGE_GRADIENT (end_image)->gradient;
      end_pattern = CTK_CSS_IMAGE_GRADIENT (end_image)->pattern;
    }

  gradient = _ctk_gradient_transition (start_gradient, end_gradient, property_id, progress);
  if (gradient == NULL)
    return CTK_CSS_IMAGE_CLASS (_ctk_css_image_gradient_parent_class)->transition (start_image, end_image, property_id, progress);

  result = g_object_new (CTK_TYPE_CSS_IMAGE_GRADIENT, NULL);
  result->gradient = gradient;
  result->pattern = transition_pattern (start_pattern, end_pattern, progress);

  return CTK_CSS_IMAGE (result);
}

static gboolean
ctk_css_image_gradient_draw_circle (CtkCssImageGradient *image,
                                    cairo_t              *cr,
                                    double               width,
                                    double               height)
{
  cairo_pattern_t *pattern = image->pattern;
  double x0, y0, x1, y1, r0, r1;
  CdkRGBA color0, color1;
  double offset0, offset1;
  int n_stops;

  if (cairo_pattern_get_type (pattern) != CAIRO_PATTERN_TYPE_RADIAL)
    return FALSE;
  if (cairo_pattern_get_extend (pattern) != CAIRO_EXTEND_PAD)
    return FALSE;

  cairo_pattern_get_radial_circles (pattern, &x0, &y0, &r0, &x1, &y1, &r1);

  if (x0 != x1 ||
      y0 != y1 ||
      r0 != 0.0)
    return FALSE;

  cairo_pattern_get_color_stop_count (pattern, &n_stops);
  if (n_stops != 2)
    return FALSE;

  cairo_pattern_get_color_stop_rgba (pattern, 0, &offset0, &color0.red, &color0.green, &color0.blue, &color0.alpha);
  cairo_pattern_get_color_stop_rgba (pattern, 1, &offset1, &color1.red, &color1.green, &color1.blue, &color1.alpha);
  if (offset0 != offset1)
    return FALSE;

  cairo_scale (cr, width, height);

  cairo_rectangle (cr, 0, 0, 1, 1);
  cairo_clip (cr);

  cdk_cairo_set_source_rgba (cr, &color1);
  cairo_paint (cr);

  cdk_cairo_set_source_rgba (cr, &color0);
  cairo_arc (cr, x1, y1, r1 * offset1, 0, 2 * G_PI);
  cairo_fill (cr);

  return TRUE;
}

static void
ctk_css_image_gradient_draw (CtkCssImage        *image,
                             cairo_t            *cr,
                             double              width,
                             double              height)
{
  CtkCssImageGradient *gradient = CTK_CSS_IMAGE_GRADIENT (image);

  if (!gradient->pattern)
    {
      g_warning ("trying to paint unresolved gradient");
      return;
    }

  if (ctk_css_image_gradient_draw_circle (gradient, cr, width, height))
    return;

  cairo_scale (cr, width, height);

  cairo_rectangle (cr, 0, 0, 1, 1);
  cairo_set_source (cr, gradient->pattern);
  cairo_fill (cr);
}

static gboolean
ctk_css_image_gradient_parse (CtkCssImage  *image,
                              CtkCssParser *parser)
{
  CtkCssImageGradient *gradient = CTK_CSS_IMAGE_GRADIENT (image);

  gradient->gradient = _ctk_gradient_parse (parser);

  return gradient->gradient != NULL;
}

static void
ctk_css_image_gradient_print (CtkCssImage *image,
                              GString     *string)
{
  CtkCssImageGradient *gradient = CTK_CSS_IMAGE_GRADIENT (image);
  char *s;

  s = ctk_gradient_to_string (gradient->gradient);
  g_string_append (string, s);
  g_free (s);
}

static void
ctk_css_image_gradient_dispose (GObject *object)
{
  CtkCssImageGradient *gradient = CTK_CSS_IMAGE_GRADIENT (object);

  if (gradient->gradient)
    {
      ctk_gradient_unref (gradient->gradient);
      gradient->gradient = NULL;
    }
  if (gradient->pattern)
    {
      cairo_pattern_destroy (gradient->pattern);
      gradient->pattern = NULL;
    }

  G_OBJECT_CLASS (_ctk_css_image_gradient_parent_class)->dispose (object);
}

static void
_ctk_css_image_gradient_class_init (CtkCssImageGradientClass *klass)
{
  CtkCssImageClass *image_class = CTK_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->compute = ctk_css_image_gradient_compute;
  image_class->transition = ctk_css_image_gradient_transition;
  image_class->draw = ctk_css_image_gradient_draw;
  image_class->parse = ctk_css_image_gradient_parse;
  image_class->print = ctk_css_image_gradient_print;

  object_class->dispose = ctk_css_image_gradient_dispose;
}

static void
_ctk_css_image_gradient_init (CtkCssImageGradient *image_gradient)
{
}

CtkGradient *
_ctk_gradient_parse (CtkCssParser *parser)
{
  CtkGradient *gradient;
  cairo_pattern_type_t type;
  gdouble coords[6];
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  if (!_ctk_css_parser_try (parser, "-ctk-gradient", TRUE))
    {
      _ctk_css_parser_error (parser,
                             "Expected '-ctk-gradient'");
      return NULL;
    }

  if (!_ctk_css_parser_try (parser, "(", TRUE))
    {
      _ctk_css_parser_error (parser,
                             "Expected '(' after '-ctk-gradient'");
      return NULL;
    }

  /* Parse gradient type */
  if (_ctk_css_parser_try (parser, "linear", TRUE))
    type = CAIRO_PATTERN_TYPE_LINEAR;
  else if (_ctk_css_parser_try (parser, "radial", TRUE))
    type = CAIRO_PATTERN_TYPE_RADIAL;
  else
    {
      _ctk_css_parser_error (parser,
                             "Gradient type must be 'radial' or 'linear'");
      return NULL;
    }

  /* Parse start/stop position parameters */
  for (i = 0; i < 2; i++)
    {
      if (! _ctk_css_parser_try (parser, ",", TRUE))
        {
          _ctk_css_parser_error (parser,
                                 "Expected ','");
          return NULL;
        }

      if (_ctk_css_parser_try (parser, "left", TRUE))
        coords[i * 3] = 0;
      else if (_ctk_css_parser_try (parser, "right", TRUE))
        coords[i * 3] = 1;
      else if (_ctk_css_parser_try (parser, "center", TRUE))
        coords[i * 3] = 0.5;
      else if (!_ctk_css_parser_try_double (parser, &coords[i * 3]))
        {
          _ctk_css_parser_error (parser,
                                 "Expected a valid X coordinate");
          return NULL;
        }

      if (_ctk_css_parser_try (parser, "top", TRUE))
        coords[i * 3 + 1] = 0;
      else if (_ctk_css_parser_try (parser, "bottom", TRUE))
        coords[i * 3 + 1] = 1;
      else if (_ctk_css_parser_try (parser, "center", TRUE))
        coords[i * 3 + 1] = 0.5;
      else if (!_ctk_css_parser_try_double (parser, &coords[i * 3 + 1]))
        {
          _ctk_css_parser_error (parser,
                                 "Expected a valid Y coordinate");
          return NULL;
        }

      if (type == CAIRO_PATTERN_TYPE_RADIAL)
        {
          /* Parse radius */
          if (! _ctk_css_parser_try (parser, ",", TRUE))
            {
              _ctk_css_parser_error (parser,
                                     "Expected ','");
              return NULL;
            }

          if (! _ctk_css_parser_try_double (parser, &coords[(i * 3) + 2]))
            {
              _ctk_css_parser_error (parser,
                                     "Expected a number for the radius");
              return NULL;
            }
        }
    }

  if (type == CAIRO_PATTERN_TYPE_LINEAR)
    gradient = ctk_gradient_new_linear (coords[0], coords[1], coords[3], coords[4]);
  else
    gradient = ctk_gradient_new_radial (coords[0], coords[1], coords[2],
                                        coords[3], coords[4], coords[5]);

  while (_ctk_css_parser_try (parser, ",", TRUE))
    {
      CtkSymbolicColor *color;
      gdouble position;

      if (_ctk_css_parser_try (parser, "from", TRUE))
        {
          position = 0;

          if (!_ctk_css_parser_try (parser, "(", TRUE))
            {
              ctk_gradient_unref (gradient);
              _ctk_css_parser_error (parser,
                                     "Expected '('");
              return NULL;
            }

        }
      else if (_ctk_css_parser_try (parser, "to", TRUE))
        {
          position = 1;

          if (!_ctk_css_parser_try (parser, "(", TRUE))
            {
              ctk_gradient_unref (gradient);
              _ctk_css_parser_error (parser,
                                     "Expected '('");
              return NULL;
            }

        }
      else if (_ctk_css_parser_try (parser, "color-stop", TRUE))
        {
          if (!_ctk_css_parser_try (parser, "(", TRUE))
            {
              ctk_gradient_unref (gradient);
              _ctk_css_parser_error (parser,
                                     "Expected '('");
              return NULL;
            }

          if (!_ctk_css_parser_try_double (parser, &position))
            {
              ctk_gradient_unref (gradient);
              _ctk_css_parser_error (parser,
                                     "Expected a valid number");
              return NULL;
            }

          if (!_ctk_css_parser_try (parser, ",", TRUE))
            {
              ctk_gradient_unref (gradient);
              _ctk_css_parser_error (parser,
                                     "Expected a comma");
              return NULL;
            }
        }
      else
        {
          ctk_gradient_unref (gradient);
          _ctk_css_parser_error (parser,
                                 "Not a valid color-stop definition");
          return NULL;
        }

      color = _ctk_css_symbolic_value_new (parser);
      if (color == NULL)
        {
          ctk_gradient_unref (gradient);
          return NULL;
        }

      ctk_gradient_add_color_stop (gradient, position, color);
      ctk_symbolic_color_unref (color);

      if (!_ctk_css_parser_try (parser, ")", TRUE))
        {
          ctk_gradient_unref (gradient);
          _ctk_css_parser_error (parser,
                                 "Expected ')'");
          return NULL;
        }
    }

  if (!_ctk_css_parser_try (parser, ")", TRUE))
    {
      ctk_gradient_unref (gradient);
      _ctk_css_parser_error (parser,
                             "Expected ')'");
      return NULL;
    }

  return gradient;
}
