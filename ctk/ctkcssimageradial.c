/*
 * Copyright © 2015 Red Hat Inc.
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
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include "ctkcssimageradialprivate.h"

#include <math.h>

#include "ctkcsscolorvalueprivate.h"
#include "ctkcssnumbervalueprivate.h"
#include "ctkcsspositionvalueprivate.h"
#include "ctkcssrgbavalueprivate.h"
#include "ctkcssprovider.h"

G_DEFINE_TYPE (CtkCssImageRadial, _ctk_css_image_radial, CTK_TYPE_CSS_IMAGE)

static void
ctk_css_image_radial_get_start_end (CtkCssImageRadial *radial,
                                    double             radius,
                                    double            *start,
                                    double            *end)
{
  CtkCssImageRadialColorStop *stop;
  double pos;
  guint i;

  if (radial->repeating)
    {
      stop = &g_array_index (radial->stops, CtkCssImageRadialColorStop, 0);
      if (stop->offset == NULL)
        *start = 0;
      else
        *start = _ctk_css_number_value_get (stop->offset, radius) / radius;

      *end = *start;

      for (i = 0; i < radial->stops->len; i++)
        {
          stop = &g_array_index (radial->stops, CtkCssImageRadialColorStop, i);

          if (stop->offset == NULL)
            continue;

          pos = _ctk_css_number_value_get (stop->offset, radius) / radius;

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
ctk_css_image_radial_draw (CtkCssImage *image,
                           cairo_t     *cr,
                           double       width,
                           double       height)
{
  CtkCssImageRadial *radial = CTK_CSS_IMAGE_RADIAL (image);
  cairo_pattern_t *pattern;
  cairo_matrix_t matrix;
  double x, y;
  double radius, yscale;
  double start, end;
  double r1, r2, r3, r4, r;
  double offset;
  int i, last;

  x = _ctk_css_position_value_get_x (radial->position, width);
  y = _ctk_css_position_value_get_y (radial->position, height);

  if (radial->circle)
    {
      switch (radial->size)
        {
        case CTK_CSS_EXPLICIT_SIZE:
          radius = _ctk_css_number_value_get (radial->sizes[0], width);
          break;
        case CTK_CSS_CLOSEST_SIDE:
          radius = MIN (MIN (x, width - x), MIN (y, height - y));
          break;
        case CTK_CSS_FARTHEST_SIDE:
          radius = MAX (MAX (x, width - x), MAX (y, height - y));
          break;
        case CTK_CSS_CLOSEST_CORNER:
        case CTK_CSS_FARTHEST_CORNER:
          r1 = x*x + y*y;
          r2 = x*x + (height - y)*(height - y);
          r3 = (width - x)*(width - x) + y*y;
          r4 = (width - x)*(width - x) + (height - y)*(height - y);
          if (radial->size == CTK_CSS_CLOSEST_CORNER)
            r = MIN ( MIN (r1, r2), MIN (r3, r4));
          else
            r = MAX ( MAX (r1, r2), MAX (r3, r4));
          radius = sqrt (r);
          break;
        default:
          g_assert_not_reached ();
        }

      radius = MAX (1.0, radius);
      yscale = 1.0;
    }
  else
    {
      double hradius, vradius;

      switch (radial->size)
        {
        case CTK_CSS_EXPLICIT_SIZE:
          hradius = _ctk_css_number_value_get (radial->sizes[0], width);
          vradius = _ctk_css_number_value_get (radial->sizes[1], height);
          break;
        case CTK_CSS_CLOSEST_SIDE:
          hradius = MIN (x, width - x);
          vradius = MIN (y, height - y);
          break;
        case CTK_CSS_FARTHEST_SIDE:
          hradius = MAX (x, width - x);
          vradius = MAX (y, height - y);
          break;
        case CTK_CSS_CLOSEST_CORNER:
          hradius = M_SQRT2 * MIN (x, width - x);
          vradius = M_SQRT2 * MIN (y, height - y);
          break;
        case CTK_CSS_FARTHEST_CORNER:
          hradius = M_SQRT2 * MAX (x, width - x);
          vradius = M_SQRT2 * MAX (y, height - y);
          break;
        default:
          g_assert_not_reached ();
        }

      hradius = MAX (1.0, hradius);
      vradius = MAX (1.0, vradius);

      radius = hradius;
      yscale = vradius / hradius;
    }

  ctk_css_image_radial_get_start_end (radial, radius, &start, &end);

  pattern = cairo_pattern_create_radial (0, 0, 0, 0, 0, radius);
  if (yscale != 1.0)
    {
      cairo_matrix_init_scale (&matrix, 1.0, 1.0 / yscale);
      cairo_pattern_set_matrix (pattern, &matrix);
    }

 if (radial->repeating)
    cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
  else
    cairo_pattern_set_extend (pattern, CAIRO_EXTEND_PAD);

  offset = start;
  last = -1;
  for (i = 0; i < radial->stops->len; i++)
    {
      CtkCssImageRadialColorStop *stop;
      double pos, step;

      stop = &g_array_index (radial->stops, CtkCssImageRadialColorStop, i);

      if (stop->offset == NULL)
        {
          if (i == 0)
            pos = 0.0;
          else if (i + 1 == radial->stops->len)
            pos = 1.0;
          else
            continue;
        }
      else
        pos = _ctk_css_number_value_get (stop->offset, radius) / radius;

      pos = MAX (pos, 0);
      step = pos / (i - last);
      for (last = last + 1; last <= i; last++)
        {
          const CdkRGBA *rgba;

          stop = &g_array_index (radial->stops, CtkCssImageRadialColorStop, last);

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
  cairo_translate (cr, x, y);
  cairo_set_source (cr, pattern);
  cairo_fill (cr);

  cairo_pattern_destroy (pattern);
}

static gboolean
ctk_css_image_radial_parse (CtkCssImage  *image,
                            CtkCssParser *parser)
{
  CtkCssImageRadial *radial = CTK_CSS_IMAGE_RADIAL (image);
  gboolean has_shape = FALSE;
  gboolean has_size = FALSE;
  gboolean found_one = FALSE;
  guint i;
  static struct {
    const char *name;
    guint       value;
  } names[] = {
    { "closest-side", CTK_CSS_CLOSEST_SIDE },
    { "farthest-side", CTK_CSS_FARTHEST_SIDE },
    { "closest-corner", CTK_CSS_CLOSEST_CORNER },
    { "farthest-corner", CTK_CSS_FARTHEST_CORNER }
  };

  if (_ctk_css_parser_try (parser, "repeating-radial-gradient(", TRUE))
    radial->repeating = TRUE;
  else if (_ctk_css_parser_try (parser, "radial-gradient(", TRUE))
    radial->repeating = FALSE;
  else
    {
      _ctk_css_parser_error (parser, "Not a radial gradient");
      return FALSE;
    }

  do {
    found_one = FALSE;
    if (!has_shape && _ctk_css_parser_try (parser, "circle", TRUE))
      {
        radial->circle = TRUE;
        found_one = has_shape = TRUE;
      }
    else if (!has_shape && _ctk_css_parser_try (parser, "ellipse", TRUE))
      {
        radial->circle = FALSE;
        found_one = has_shape = TRUE;
      }
    else if (!has_size)
      {
        for (i = 0; i < G_N_ELEMENTS (names); i++)
          {
            if (_ctk_css_parser_try (parser, names[i].name, TRUE))
              {
                found_one = has_size = TRUE;
                radial->size = names[i].value;
                break;
              }
          }

        if (!has_size)
          {
            if (ctk_css_number_value_can_parse (parser))
              radial->sizes[0] = _ctk_css_number_value_parse (parser, CTK_CSS_PARSE_LENGTH | CTK_CSS_PARSE_PERCENT);
            if (ctk_css_number_value_can_parse (parser))
              radial->sizes[1] = _ctk_css_number_value_parse (parser, CTK_CSS_PARSE_LENGTH | CTK_CSS_PARSE_PERCENT);
            found_one = has_size = radial->sizes[0] != NULL;
          }
      }

  } while (found_one && !(has_shape && has_size));

  if (_ctk_css_parser_try (parser, "at", TRUE))
    {
      radial->position = _ctk_css_position_value_parse (parser);
      if (!radial->position)
        return FALSE;
      if (!_ctk_css_parser_try (parser, ",", TRUE))
        {
          _ctk_css_parser_error (parser, "Expected a comma here");
          return FALSE;
        }
    }
  else
    {
      radial->position = _ctk_css_position_value_new (_ctk_css_number_value_new (50, CTK_CSS_PERCENT),
                                                      _ctk_css_number_value_new (50, CTK_CSS_PERCENT));

      if ((has_shape || has_size) &&
          !_ctk_css_parser_try (parser, ",", TRUE))
        {
          _ctk_css_parser_error (parser, "Expected a comma here");
          return FALSE;
        }
    }

  if (!has_size)
    {
      radial->size = CTK_CSS_FARTHEST_CORNER;
    }

  if (!has_shape)
    {
      if (radial->sizes[0] && radial->sizes[1])
        radial->circle = FALSE;
      else
        radial->circle = TRUE;
    }

  if (has_shape && radial->circle)
    {
      if (radial->sizes[0] && radial->sizes[1])
        {
          _ctk_css_parser_error (parser, "Circular gradient can only have one size");
          return FALSE;
        }

      if (radial->sizes[0] && ctk_css_number_value_has_percent (radial->sizes[0]))
        {
          _ctk_css_parser_error (parser, "Circular gradient cannot have percentage as size");
          return FALSE;
        }
    }

  if (has_size && !radial->circle)
    {
      if (!radial->sizes[1])
        radial->sizes[1] = _ctk_css_value_ref (radial->sizes[0]);
    }

  do {
    CtkCssImageRadialColorStop stop;

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

    g_array_append_val (radial->stops, stop);

  } while (_ctk_css_parser_try (parser, ",", TRUE));

  if (radial->stops->len < 2)
    {
      _ctk_css_parser_error_full (parser,
                                  CTK_CSS_PROVIDER_ERROR_DEPRECATED,
                                  "Using one color stop with %s() is deprecated.",
                                  radial->repeating ? "repeating-radial-gradient" : "radial-gradient");
    }

  if (!_ctk_css_parser_try (parser, ")", TRUE))
    {
      _ctk_css_parser_error (parser, "Missing closing bracket at end of radial gradient");
      return FALSE;
    }

  return TRUE;
}

static void
ctk_css_image_radial_print (CtkCssImage *image,
                            GString     *string)
{
  CtkCssImageRadial *radial = CTK_CSS_IMAGE_RADIAL (image);
  guint i;
  const gchar *names[] = {
    NULL,
    "closest-side",
    "farthest-side",
    "closest-corner",
    "farthest-corner"
  };

  if (radial->repeating)
    g_string_append (string, "repeating-radial-gradient(");
  else
    g_string_append (string, "radial-gradient(");

  if (radial->circle)
    g_string_append (string, "circle ");
  else
    g_string_append (string, "ellipse ");

  if (radial->size != 0)
    g_string_append (string, names[radial->size]);
  else
    {
      if (radial->sizes[0])
        _ctk_css_value_print (radial->sizes[0], string);
      if (radial->sizes[1])
        {
          g_string_append (string, " ");
          _ctk_css_value_print (radial->sizes[1], string);
        }
    }

  g_string_append (string, " at ");
  _ctk_css_value_print (radial->position, string);

  g_string_append (string, ", ");

  for (i = 0; i < radial->stops->len; i++)
    {
      CtkCssImageRadialColorStop *stop;

      if (i > 0)
        g_string_append (string, ", ");

      stop = &g_array_index (radial->stops, CtkCssImageRadialColorStop, i);

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
ctk_css_image_radial_compute (CtkCssImage             *image,
                              guint                    property_id,
                              CtkStyleProviderPrivate *provider,
                              CtkCssStyle             *style,
                              CtkCssStyle             *parent_style)
{
  CtkCssImageRadial *radial = CTK_CSS_IMAGE_RADIAL (image);
  CtkCssImageRadial *copy;
  guint i;

  copy = g_object_new (CTK_TYPE_CSS_IMAGE_RADIAL, NULL);
  copy->repeating = radial->repeating;
  copy->circle = radial->circle;
  copy->size = radial->size;

  copy->position = _ctk_css_value_compute (radial->position, property_id, provider, style, parent_style);

  if (radial->sizes[0])
    copy->sizes[0] = _ctk_css_value_compute (radial->sizes[0], property_id, provider, style, parent_style);

  if (radial->sizes[1])
    copy->sizes[1] = _ctk_css_value_compute (radial->sizes[1], property_id, provider, style, parent_style);

  g_array_set_size (copy->stops, radial->stops->len);
  for (i = 0; i < radial->stops->len; i++)
    {
      CtkCssImageRadialColorStop *stop, *scopy;

      stop = &g_array_index (radial->stops, CtkCssImageRadialColorStop, i);
      scopy = &g_array_index (copy->stops, CtkCssImageRadialColorStop, i);

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
ctk_css_image_radial_transition (CtkCssImage *start_image,
                                 CtkCssImage *end_image,
                                 guint        property_id,
                                 double       progress)
{
  CtkCssImageRadial *start, *end, *result;
  guint i;

  start = CTK_CSS_IMAGE_RADIAL (start_image);

  if (end_image == NULL)
    return CTK_CSS_IMAGE_CLASS (_ctk_css_image_radial_parent_class)->transition (start_image, end_image, property_id, progress);

  if (!CTK_IS_CSS_IMAGE_RADIAL (end_image))
    return CTK_CSS_IMAGE_CLASS (_ctk_css_image_radial_parent_class)->transition (start_image, end_image, property_id, progress);

  end = CTK_CSS_IMAGE_RADIAL (end_image);

  if (start->repeating != end->repeating ||
      start->stops->len != end->stops->len ||
      start->size != end->size ||
      start->circle != end->circle)
    return CTK_CSS_IMAGE_CLASS (_ctk_css_image_radial_parent_class)->transition (start_image, end_image, property_id, progress);

  result = g_object_new (CTK_TYPE_CSS_IMAGE_RADIAL, NULL);
  result->repeating = start->repeating;
  result->circle = start->circle;
  result->size = start->size;

  result->position = _ctk_css_value_transition (start->position, end->position, property_id, progress);
  if (result->position == NULL)
    goto fail;

  if (start->sizes[0] && end->sizes[0])
    {
      result->sizes[0] = _ctk_css_value_transition (start->sizes[0], end->sizes[0], property_id, progress);
      if (result->sizes[0] == NULL)
        goto fail;
    }
  else
    result->sizes[0] = 0;

  if (start->sizes[1] && end->sizes[1])
    {
      result->sizes[1] = _ctk_css_value_transition (start->sizes[1], end->sizes[1], property_id, progress);
      if (result->sizes[1] == NULL)
        goto fail;
    }
  else
    result->sizes[1] = 0;

  for (i = 0; i < start->stops->len; i++)
    {
      CtkCssImageRadialColorStop stop, *start_stop, *end_stop;

      start_stop = &g_array_index (start->stops, CtkCssImageRadialColorStop, i);
      end_stop = &g_array_index (end->stops, CtkCssImageRadialColorStop, i);

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
  return CTK_CSS_IMAGE_CLASS (_ctk_css_image_radial_parent_class)->transition (start_image, end_image, property_id, progress);
}

static gboolean
ctk_css_image_radial_equal (CtkCssImage *image1,
                            CtkCssImage *image2)
{
  CtkCssImageRadial *radial1 = CTK_CSS_IMAGE_RADIAL (image1);
  CtkCssImageRadial *radial2 = CTK_CSS_IMAGE_RADIAL (image2);
  guint i;

  if (radial1->repeating != radial2->repeating ||
      radial1->size != radial2->size ||
      !_ctk_css_value_equal (radial1->position, radial2->position) ||
      ((radial1->sizes[0] == NULL) != (radial2->sizes[0] == NULL)) ||
      (radial1->sizes[0] && radial2->sizes[0] && !_ctk_css_value_equal (radial1->sizes[0], radial2->sizes[0])) ||
      ((radial1->sizes[1] == NULL) != (radial2->sizes[1] == NULL)) ||
      (radial1->sizes[1] && radial2->sizes[1] && !_ctk_css_value_equal (radial1->sizes[1], radial2->sizes[1])) ||
      radial1->stops->len != radial2->stops->len)
    return FALSE;

  for (i = 0; i < radial1->stops->len; i++)
    {
      CtkCssImageRadialColorStop *stop1, *stop2;

      stop1 = &g_array_index (radial1->stops, CtkCssImageRadialColorStop, i);
      stop2 = &g_array_index (radial2->stops, CtkCssImageRadialColorStop, i);

      if (!_ctk_css_value_equal0 (stop1->offset, stop2->offset) ||
          !_ctk_css_value_equal (stop1->color, stop2->color))
        return FALSE;
    }

  return TRUE;
}

static void
ctk_css_image_radial_dispose (GObject *object)
{
  CtkCssImageRadial *radial = CTK_CSS_IMAGE_RADIAL (object);
  int i;

  if (radial->stops)
    {
      g_array_free (radial->stops, TRUE);
      radial->stops = NULL;
    }

  if (radial->position)
    {
      _ctk_css_value_unref (radial->position);
      radial->position = NULL;
    }

  for (i = 0; i < 2; i++)
    if (radial->sizes[i])
      {
        _ctk_css_value_unref (radial->sizes[i]);
        radial->sizes[i] = NULL;
      }

  G_OBJECT_CLASS (_ctk_css_image_radial_parent_class)->dispose (object);
}

static void
_ctk_css_image_radial_class_init (CtkCssImageRadialClass *klass)
{
  CtkCssImageClass *image_class = CTK_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->draw = ctk_css_image_radial_draw;
  image_class->parse = ctk_css_image_radial_parse;
  image_class->print = ctk_css_image_radial_print;
  image_class->compute = ctk_css_image_radial_compute;
  image_class->transition = ctk_css_image_radial_transition;
  image_class->equal = ctk_css_image_radial_equal;

  object_class->dispose = ctk_css_image_radial_dispose;
}

static void
ctk_css_image_clear_color_stop (gpointer color_stop)
{
  CtkCssImageRadialColorStop *stop = color_stop;

  _ctk_css_value_unref (stop->color);
  if (stop->offset)
    _ctk_css_value_unref (stop->offset);
}

static void
_ctk_css_image_radial_init (CtkCssImageRadial *radial)
{
  radial->stops = g_array_new (FALSE, FALSE, sizeof (CtkCssImageRadialColorStop));
  g_array_set_clear_func (radial->stops, ctk_css_image_clear_color_stop);
}

