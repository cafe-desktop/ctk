/* CTK - The GIMP Toolkit
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * Authors: Carlos Garnacho <carlosg@gnome.org>
 *          Cosimo Cecchi <cosimoc@gnome.org>
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

#include <config.h>

#include "ctkrenderborderprivate.h"

#include <cairo-gobject.h>
#include <math.h>

#include "ctkcssbordervalueprivate.h"
#include "ctkcssenumvalueprivate.h"
#include "ctkcssimagevalueprivate.h"
#include "ctkcssnumbervalueprivate.h"
#include "ctkcssrepeatvalueprivate.h"
#include "ctkcssrgbavalueprivate.h"
#include "ctkcssstyleprivate.h"
#include "ctkhslaprivate.h"
#include "ctkroundedboxprivate.h"

/* this is in case round() is not provided by the compiler, 
 * such as in the case of C89 compilers, like MSVC
 */
#include "fallback-c89.c"

typedef struct _CtkBorderImage CtkBorderImage;

struct _CtkBorderImage {
  CtkCssImage *source;

  CtkCssValue *slice;
  CtkCssValue *width;
  CtkCssValue *repeat;
};

static gboolean
ctk_border_image_init (CtkBorderImage *image,
                       CtkCssStyle    *style)
{
  image->source = _ctk_css_image_value_get_image (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_IMAGE_SOURCE));
  if (image->source == NULL)
    return FALSE;

  image->slice = ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_IMAGE_SLICE);
  image->width = ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_IMAGE_WIDTH);
  image->repeat = ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_IMAGE_REPEAT);

  return TRUE;
}

typedef struct _CtkBorderImageSliceSize CtkBorderImageSliceSize;
struct _CtkBorderImageSliceSize {
  double offset;
  double size;
};

static void
ctk_border_image_compute_border_size (CtkBorderImageSliceSize  sizes[3],
                                      double                   offset,
                                      double                   area_size,
                                      double                   start_border_width,
                                      double                   end_border_width,
                                      const CtkCssValue       *start_border,
                                      const CtkCssValue       *end_border)
{
  double start, end;

  if (ctk_css_number_value_get_dimension (start_border) == CTK_CSS_DIMENSION_NUMBER)
    start = start_border_width * _ctk_css_number_value_get (start_border, 100);
  else
    start = _ctk_css_number_value_get (start_border, area_size);
  if (ctk_css_number_value_get_dimension (end_border) == CTK_CSS_DIMENSION_NUMBER)
    end = end_border_width * _ctk_css_number_value_get (end_border, 100);
  else
    end = _ctk_css_number_value_get (end_border, area_size);

  /* XXX: reduce vertical and horizontal by the same factor */
  if (start + end > area_size)
    {
      start = start * area_size / (start + end);
      end = end * area_size / (start + end);
    }

  sizes[0].offset = offset;
  sizes[0].size = start;
  sizes[1].offset = offset + start;
  sizes[1].size = area_size - start - end;
  sizes[2].offset = offset + area_size - end;
  sizes[2].size = end;
}

static void
ctk_border_image_render_slice (cairo_t           *cr,
                               cairo_surface_t   *slice,
                               double             slice_width,
                               double             slice_height,
                               double             x,
                               double             y,
                               double             width,
                               double             height,
                               CtkCssRepeatStyle  hrepeat,
                               CtkCssRepeatStyle  vrepeat)
{
  double hscale, vscale;
  double xstep, ystep;
  cairo_extend_t extend = CAIRO_EXTEND_PAD;
  cairo_matrix_t matrix;
  cairo_pattern_t *pattern;

  /* We can't draw center tiles yet */
  g_assert (hrepeat == CTK_CSS_REPEAT_STYLE_STRETCH || vrepeat == CTK_CSS_REPEAT_STYLE_STRETCH);

  hscale = width / slice_width;
  vscale = height / slice_height;
  xstep = width;
  ystep = height;

  switch (hrepeat)
    {
    case CTK_CSS_REPEAT_STYLE_REPEAT:
      extend = CAIRO_EXTEND_REPEAT;
      hscale = vscale;
      break;
    case CTK_CSS_REPEAT_STYLE_SPACE:
      {
        double space, n;

        extend = CAIRO_EXTEND_NONE;
        hscale = vscale;

        xstep = hscale * slice_width;
        n = floor (width / xstep);
        space = (width - n * xstep) / (n + 1);
        xstep += space;
        x += space;
        width -= 2 * space;
      }
      break;
    case CTK_CSS_REPEAT_STYLE_STRETCH:
      break;
    case CTK_CSS_REPEAT_STYLE_ROUND:
      extend = CAIRO_EXTEND_REPEAT;
      hscale = width / (slice_width * MAX (round (width / (slice_width * vscale)), 1));
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  switch (vrepeat)
    {
    case CTK_CSS_REPEAT_STYLE_REPEAT:
      extend = CAIRO_EXTEND_REPEAT;
      vscale = hscale;
      break;
    case CTK_CSS_REPEAT_STYLE_SPACE:
      {
        double space, n;

        extend = CAIRO_EXTEND_NONE;
        vscale = hscale;

        ystep = vscale * slice_height;
        n = floor (height / ystep);
        space = (height - n * ystep) / (n + 1);
        ystep += space;
        y += space;
        height -= 2 * space;
      }
      break;
    case CTK_CSS_REPEAT_STYLE_STRETCH:
      break;
    case CTK_CSS_REPEAT_STYLE_ROUND:
      extend = CAIRO_EXTEND_REPEAT;
      vscale = height / (slice_height * MAX (round (height / (slice_height * hscale)), 1));
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  pattern = cairo_pattern_create_for_surface (slice);

  cairo_matrix_init_translate (&matrix,
                               hrepeat == CTK_CSS_REPEAT_STYLE_REPEAT ? slice_width / 2 : 0,
                               vrepeat == CTK_CSS_REPEAT_STYLE_REPEAT ? slice_height / 2 : 0);
  cairo_matrix_scale (&matrix, 1 / hscale, 1 / vscale);
  cairo_matrix_translate (&matrix,
                          hrepeat == CTK_CSS_REPEAT_STYLE_REPEAT ? - width / 2 : 0,
                          vrepeat == CTK_CSS_REPEAT_STYLE_REPEAT ? - height / 2 : 0);

  cairo_pattern_set_matrix (pattern, &matrix);
  cairo_pattern_set_extend (pattern, extend);

  cairo_save (cr);
  cairo_translate (cr, x, y);

  for (y = 0; y < height; y += ystep)
    {
      for (x = 0; x < width; x += xstep)
        {
          cairo_save (cr);
          cairo_translate (cr, x, y);
          cairo_set_source (cr, pattern);
          cairo_rectangle (cr, 0, 0, xstep, ystep);
          cairo_fill (cr);
          cairo_restore (cr);
        }
    }

  cairo_restore (cr);

  cairo_pattern_destroy (pattern);
}

static void
ctk_border_image_compute_slice_size (CtkBorderImageSliceSize sizes[3],
                                     int                     surface_size,
                                     int                     start_size,
                                     int                     end_size)
{
  sizes[0].size = MIN (start_size, surface_size);
  sizes[0].offset = 0;

  sizes[2].size = MIN (end_size, surface_size);
  sizes[2].offset = surface_size - sizes[2].size;

  sizes[1].size = MAX (0, surface_size - sizes[0].size - sizes[2].size);
  sizes[1].offset = sizes[0].size;
}

static void
ctk_border_image_render (CtkBorderImage   *image,
                         const double      border_width[4],
                         cairo_t          *cr,
                         gdouble           x,
                         gdouble           y,
                         gdouble           width,
                         gdouble           height)
{
  cairo_surface_t *surface, *slice;
  CtkBorderImageSliceSize vertical_slice[3], horizontal_slice[3];
  CtkBorderImageSliceSize vertical_border[3], horizontal_border[3];
  double source_width, source_height;
  int h, v;

  _ctk_css_image_get_concrete_size (image->source,
                                    0, 0,
                                    width, height,
                                    &source_width, &source_height);

  /* XXX: Optimize for (source_width == width && source_height == height) */

  surface = _ctk_css_image_get_surface (image->source,
                                        cairo_get_target (cr),
                                        source_width, source_height);

  ctk_border_image_compute_slice_size (horizontal_slice,
                                       source_width, 
                                       _ctk_css_number_value_get (_ctk_css_border_value_get_left (image->slice), source_width),
                                       _ctk_css_number_value_get (_ctk_css_border_value_get_right (image->slice), source_width));
  ctk_border_image_compute_slice_size (vertical_slice,
                                       source_height, 
                                       _ctk_css_number_value_get (_ctk_css_border_value_get_top (image->slice), source_height),
                                       _ctk_css_number_value_get (_ctk_css_border_value_get_bottom (image->slice), source_height));
  ctk_border_image_compute_border_size (horizontal_border,
                                        x,
                                        width,
                                        border_width[CTK_CSS_LEFT],
                                        border_width[CTK_CSS_RIGHT],
                                        _ctk_css_border_value_get_left (image->width),
                                        _ctk_css_border_value_get_right (image->width));
  ctk_border_image_compute_border_size (vertical_border,
                                        y,
                                        height,
                                        border_width[CTK_CSS_TOP],
                                        border_width[CTK_CSS_BOTTOM],
                                        _ctk_css_border_value_get_top (image->width),
                                        _ctk_css_border_value_get_bottom(image->width));
  
  for (v = 0; v < 3; v++)
    {
      if (vertical_slice[v].size == 0 ||
          vertical_border[v].size == 0)
        continue;

      for (h = 0; h < 3; h++)
        {
          if (horizontal_slice[h].size == 0 ||
              horizontal_border[h].size == 0)
            continue;

          if (h == 1 && v == 1)
            continue;

          slice = cairo_surface_create_for_rectangle (surface,
                                                      horizontal_slice[h].offset,
                                                      vertical_slice[v].offset,
                                                      horizontal_slice[h].size,
                                                      vertical_slice[v].size);

          ctk_border_image_render_slice (cr,
                                         slice,
                                         horizontal_slice[h].size,
                                         vertical_slice[v].size,
                                         horizontal_border[h].offset,
                                         vertical_border[v].offset,
                                         horizontal_border[h].size,
                                         vertical_border[v].size,
                                         h == 1 ? _ctk_css_border_repeat_value_get_x (image->repeat) : CTK_CSS_REPEAT_STYLE_STRETCH,
                                         v == 1 ? _ctk_css_border_repeat_value_get_y (image->repeat) : CTK_CSS_REPEAT_STYLE_STRETCH);

          cairo_surface_destroy (slice);
        }
    }

  cairo_surface_destroy (surface);
}

static void
hide_border_sides (double         border[4],
                   CtkBorderStyle border_style[4] G_GNUC_UNUSED,
                   guint          hidden_side)
{
  guint i;

  for (i = 0; i < 4; i++)
    {
      if (hidden_side & (1 << i))
        border[i] = 0;
    }
}

static void
render_frame_fill (cairo_t       *cr,
                   CtkRoundedBox *border_box,
                   const double   border_width[4],
                   CdkRGBA        colors[4],
                   guint          hidden_side)
{
  CtkRoundedBox padding_box;
  guint i, j;

  padding_box = *border_box;
  _ctk_rounded_box_shrink (&padding_box,
                           border_width[CTK_CSS_TOP],
                           border_width[CTK_CSS_RIGHT],
                           border_width[CTK_CSS_BOTTOM],
                           border_width[CTK_CSS_LEFT]);

  if (hidden_side == 0 &&
      cdk_rgba_equal (&colors[0], &colors[1]) &&
      cdk_rgba_equal (&colors[0], &colors[2]) &&
      cdk_rgba_equal (&colors[0], &colors[3]))
    {
      cdk_cairo_set_source_rgba (cr, &colors[0]);

      _ctk_rounded_box_path (border_box, cr);
      _ctk_rounded_box_path (&padding_box, cr);
      cairo_fill (cr);
    }
  else
    {
      for (i = 0; i < 4; i++) 
        {
          if (hidden_side & (1 << i))
            continue;

          for (j = 0; j < 4; j++)
            { 
              if (hidden_side & (1 << j))
                continue;

              if (i == j || 
                  (cdk_rgba_equal (&colors[i], &colors[j])))
                {
                  /* We were already painted when i == j */
                  if (i > j)
                    break;

                  if (j == 0)
                    _ctk_rounded_box_path_top (border_box, &padding_box, cr);
                  else if (j == 1)
                    _ctk_rounded_box_path_right (border_box, &padding_box, cr);
                  else if (j == 2)
                    _ctk_rounded_box_path_bottom (border_box, &padding_box, cr);
                  else if (j == 3)
                    _ctk_rounded_box_path_left (border_box, &padding_box, cr);
                }
            }
          /* We were already painted when i == j */
          if (i > j)
            continue;

          cdk_cairo_set_source_rgba (cr, &colors[i]);

          cairo_fill (cr);
        }
    }
}

static void
set_stroke_style (cairo_t        *cr,
                  double          line_width,
                  CtkBorderStyle  style,
                  double          length)
{
  double segments[2];
  double n;

  cairo_set_line_width (cr, line_width);

  if (style == CTK_BORDER_STYLE_DOTTED)
    {
      n = round (0.5 * length / line_width);

      segments[0] = 0;
      segments[1] = n ? length / n : 2;
      cairo_set_dash (cr, segments, G_N_ELEMENTS (segments), 0);

      cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
    }
  else
    {
      n = length / line_width;
      /* Optimize the common case of an integer-sized rectangle
       * Again, we care about focus rectangles.
       */
      if (n == nearbyint (n))
        {
          segments[0] = line_width;
          segments[1] = 2 * line_width;
        }
      else
        {
          n = round ((1. / 3) * n);

          segments[0] = n ? (1. / 3) * length / n : 1;
          segments[1] = 2 * segments[0];
        }
      cairo_set_dash (cr, segments, G_N_ELEMENTS (segments), 0);

      cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
      cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);
    }
}

static void
render_frame_stroke (cairo_t       *cr,
                     CtkRoundedBox *border_box,
                     const double   border_width[4],
                     CdkRGBA        colors[4],
                     guint          hidden_side,
                     CtkBorderStyle stroke_style)
{
  gboolean different_colors, different_borders;
  CtkRoundedBox stroke_box;
  guint i;

  different_colors = !cdk_rgba_equal (&colors[0], &colors[1]) ||
                     !cdk_rgba_equal (&colors[0], &colors[2]) ||
                     !cdk_rgba_equal (&colors[0], &colors[3]);
  different_borders = border_width[0] != border_width[1] ||
                      border_width[0] != border_width[2] ||
                      border_width[0] != border_width[3] ;

  stroke_box = *border_box;
  _ctk_rounded_box_shrink (&stroke_box,
                           border_width[CTK_CSS_TOP] / 2.0,
                           border_width[CTK_CSS_RIGHT] / 2.0,
                           border_width[CTK_CSS_BOTTOM] / 2.0,
                           border_width[CTK_CSS_LEFT] / 2.0);

  if (!different_colors && !different_borders && hidden_side == 0)
    {
      double length = 0;

      /* FAST PATH:
       * Mostly expected to trigger for focus rectangles */
      for (i = 0; i < 4; i++) 
        {
          length += _ctk_rounded_box_guess_length (&stroke_box, i);
        }

      _ctk_rounded_box_path (&stroke_box, cr);
      cdk_cairo_set_source_rgba (cr, &colors[0]);
      set_stroke_style (cr, border_width[0], stroke_style, length);
      cairo_stroke (cr);
    }
  else
    {
      CtkRoundedBox padding_box;

      padding_box = *border_box;
      _ctk_rounded_box_shrink (&padding_box,
                               border_width[CTK_CSS_TOP],
                               border_width[CTK_CSS_RIGHT],
                               border_width[CTK_CSS_BOTTOM],
                               border_width[CTK_CSS_LEFT]);

      for (i = 0; i < 4; i++)
        {
          if (hidden_side & (1 << i))
            continue;

          if (border_width[i] == 0)
            continue;

          cairo_save (cr);

          if (i == 0)
            _ctk_rounded_box_path_top (border_box, &padding_box, cr);
          else if (i == 1)
            _ctk_rounded_box_path_right (border_box, &padding_box, cr);
          else if (i == 2)
            _ctk_rounded_box_path_bottom (border_box, &padding_box, cr);
          else if (i == 3)
            _ctk_rounded_box_path_left (border_box, &padding_box, cr);
          cairo_clip (cr);

          _ctk_rounded_box_path_side (&stroke_box, cr, i);

          cdk_cairo_set_source_rgba (cr, &colors[i]);
          set_stroke_style (cr,
                            border_width[i],
                            stroke_style,
                            _ctk_rounded_box_guess_length (&stroke_box, i));
          cairo_stroke (cr);

          cairo_restore (cr);
        }
    }
}

static void
color_shade (const CdkRGBA *color,
             gdouble        factor,
             CdkRGBA       *color_return)
{
  CtkHSLA hsla;

  _ctk_hsla_init_from_rgba (&hsla, color);
  _ctk_hsla_shade (&hsla, &hsla, factor);
  _cdk_rgba_init_from_hsla (color_return, &hsla);
}

static void
render_border (cairo_t       *cr,
               CtkRoundedBox *border_box,
               const double   border_width[4],
               guint          hidden_side,
               CdkRGBA        colors[4],
               CtkBorderStyle border_style[4])
{
  guint i, j;

  cairo_save (cr);

  cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);

  for (i = 0; i < 4; i++)
    {
      if (hidden_side & (1 << i))
        continue;

      /* NB: code below divides by this value */
      /* a border smaller than this will not noticably modify
       * pixels on screen, and since we don't compare with 0,
       * we'll use this value */
      if (border_width[i] < 1.0 / 1024)
        continue;

      switch (border_style[i])
        {
        case CTK_BORDER_STYLE_NONE:
        case CTK_BORDER_STYLE_HIDDEN:
        case CTK_BORDER_STYLE_SOLID:
          break;
        case CTK_BORDER_STYLE_INSET:
          if (i == 1 || i == 2)
            color_shade (&colors[i], 1.8, &colors[i]);
          break;
        case CTK_BORDER_STYLE_OUTSET:
          if (i == 0 || i == 3)
            color_shade (&colors[i], 1.8, &colors[i]);
          break;
        case CTK_BORDER_STYLE_DOTTED:
        case CTK_BORDER_STYLE_DASHED:
          {
            guint dont_draw = hidden_side;

            for (j = 0; j < 4; j++)
              {
                if (border_style[j] == border_style[i])
                  hidden_side |= (1 << j);
                else
                  dont_draw |= (1 << j);
              }
            
            render_frame_stroke (cr, border_box, border_width, colors, dont_draw, border_style[i]);
          }
          break;
        case CTK_BORDER_STYLE_DOUBLE:
          {
            CtkRoundedBox other_box;
            double other_border[4];
            guint dont_draw = hidden_side;

            for (j = 0; j < 4; j++)
              {
                if (border_style[j] == CTK_BORDER_STYLE_DOUBLE)
                  hidden_side |= (1 << j);
                else
                  dont_draw |= (1 << j);
                
                other_border[j] = border_width[j] / 3;
              }
            
            render_frame_fill (cr, border_box, other_border, colors, dont_draw);
            
            other_box = *border_box;
            _ctk_rounded_box_shrink (&other_box,
                                     2 * other_border[CTK_CSS_TOP],
                                     2 * other_border[CTK_CSS_RIGHT],
                                     2 * other_border[CTK_CSS_BOTTOM],
                                     2 * other_border[CTK_CSS_LEFT]);
            render_frame_fill (cr, &other_box, other_border, colors, dont_draw);
          }
          break;
        case CTK_BORDER_STYLE_GROOVE:
        case CTK_BORDER_STYLE_RIDGE:
          {
            CtkRoundedBox other_box;
            CdkRGBA other_colors[4];
            guint dont_draw = hidden_side;
            double other_border[4];

            for (j = 0; j < 4; j++)
              {
                other_colors[j] = colors[j];
                if ((j == 0 || j == 3) ^ (border_style[j] == CTK_BORDER_STYLE_RIDGE))
                  color_shade (&other_colors[j], 1.8, &other_colors[j]);
                else
                  color_shade (&colors[j], 1.8, &colors[j]);
                if (border_style[j] == CTK_BORDER_STYLE_GROOVE ||
                    border_style[j] == CTK_BORDER_STYLE_RIDGE)
                  hidden_side |= (1 << j);
                else
                  dont_draw |= (1 << j);
                other_border[j] = border_width[j] / 2;
              }
            
            render_frame_fill (cr, border_box, other_border, colors, dont_draw);
            
            other_box = *border_box;
            _ctk_rounded_box_shrink (&other_box,
                                     other_border[CTK_CSS_TOP],
                                     other_border[CTK_CSS_RIGHT],
                                     other_border[CTK_CSS_BOTTOM],
                                     other_border[CTK_CSS_LEFT]);
            render_frame_fill (cr, &other_box, other_border, other_colors, dont_draw);
          }
          break;
        default:
          g_assert_not_reached ();
          break;
        }
    }
  
  render_frame_fill (cr, border_box, border_width, colors, hidden_side);

  cairo_restore (cr);
}

gboolean
ctk_css_style_render_has_border (CtkCssStyle *style)
{
  if (_ctk_css_image_value_get_image (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_IMAGE_SOURCE)))
    return TRUE;

  return _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_TOP_WIDTH), 100) > 0
      || _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_RIGHT_WIDTH), 100) > 0
      || _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_BOTTOM_WIDTH), 100) > 0
      || _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_LEFT_WIDTH), 100) > 0;
}

void
ctk_css_style_render_border (CtkCssStyle      *style,
                             cairo_t          *cr,
                             gdouble           x,
                             gdouble           y,
                             gdouble           width,
                             gdouble           height,
                             guint             hidden_side,
                             CtkJunctionSides  junction)
{
  CtkBorderImage border_image;
  double border_width[4];

  border_width[0] = _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_TOP_WIDTH), 100);
  border_width[1] = _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_RIGHT_WIDTH), 100);
  border_width[2] = _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_BOTTOM_WIDTH), 100);
  border_width[3] = _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_LEFT_WIDTH), 100);

  if (ctk_border_image_init (&border_image, style))
    {
      ctk_border_image_render (&border_image, border_width, cr, x, y, width, height);
    }
  else
    {
      CtkBorderStyle border_style[4];
      CtkRoundedBox border_box;
      CdkRGBA colors[4];

      /* Optimize the most common case of "This widget has no border" */
      if (border_width[0] == 0 &&
          border_width[1] == 0 &&
          border_width[2] == 0 &&
          border_width[3] == 0)
        return;

      border_style[0] = _ctk_css_border_style_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_TOP_STYLE));
      border_style[1] = _ctk_css_border_style_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_RIGHT_STYLE));
      border_style[2] = _ctk_css_border_style_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_BOTTOM_STYLE));
      border_style[3] = _ctk_css_border_style_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_LEFT_STYLE));

      hide_border_sides (border_width, border_style, hidden_side);

      colors[0] = *_ctk_css_rgba_value_get_rgba (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_TOP_COLOR));
      colors[1] = *_ctk_css_rgba_value_get_rgba (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_RIGHT_COLOR));
      colors[2] = *_ctk_css_rgba_value_get_rgba (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_BOTTOM_COLOR));
      colors[3] = *_ctk_css_rgba_value_get_rgba (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_LEFT_COLOR));

      _ctk_rounded_box_init_rect (&border_box, x, y, width, height);
      _ctk_rounded_box_apply_border_radius_for_style (&border_box, style, junction);

      render_border (cr, &border_box, border_width, hidden_side, colors, border_style);
    }
}

gboolean
ctk_css_style_render_border_get_clip (CtkCssStyle  *style,
                                      gdouble       x,
                                      gdouble       y,
                                      gdouble       width,
                                      gdouble       height,
                                      CdkRectangle *out_clip)
{
  if (!ctk_css_style_render_has_border (style))
    return FALSE;

  out_clip->x = floor (x);
  out_clip->y = floor (y);
  out_clip->width = ceil (x + width) - out_clip->x;
  out_clip->height = ceil (y + height) - out_clip->y;

  return TRUE;
}

gboolean
ctk_css_style_render_has_outline (CtkCssStyle *style)
{
  return _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_OUTLINE_WIDTH), 100) > 0;
}

static void
compute_outline_rect (CtkCssStyle       *style,
                      gdouble            x,
                      gdouble            y,
                      gdouble            width,
                      gdouble            height,
                      cairo_rectangle_t *out_rect)
{
  double offset, owidth;

  owidth = _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_OUTLINE_WIDTH), 100);
  offset = _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_OUTLINE_OFFSET), 100);

  if (width <= -2 * offset)
    {
      x += width / 2;
      out_rect->x = x - owidth;
      out_rect->width = 2 * owidth;
    }
  else
    {
      out_rect->x = x - offset - owidth;
      out_rect->width = width + 2 * (offset + owidth);
    }

  if (height <= -2 * offset)
    {
      y += height / 2;
      out_rect->y = y - owidth;
      out_rect->height = 2 * owidth;
    }
  else
    {
      out_rect->y = y - offset - owidth;
      out_rect->height = height + 2 * (offset + owidth);
    }

}
                      
void
ctk_css_style_render_outline (CtkCssStyle *style,
                              cairo_t     *cr,
                              gdouble      x,
                              gdouble      y,
                              gdouble      width,
                              gdouble      height)
{
  CtkBorderStyle border_style[4];
  CtkRoundedBox border_box;
  double border_width[4];
  CdkRGBA colors[4];

  border_style[0] = _ctk_css_border_style_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_OUTLINE_STYLE));
  if (border_style[0] != CTK_BORDER_STYLE_NONE)
    {
      cairo_rectangle_t rect;

      compute_outline_rect (style, x, y, width, height, &rect);

      border_style[1] = border_style[2] = border_style[3] = border_style[0];
      border_width[0] = _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_OUTLINE_WIDTH), 100);
      border_width[3] = border_width[2] = border_width[1] = border_width[0];
      colors[0] = *_ctk_css_rgba_value_get_rgba (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_OUTLINE_COLOR));
      colors[3] = colors[2] = colors[1] = colors[0];

      _ctk_rounded_box_init_rect (&border_box, rect.x, rect.y, rect.width, rect.height);
      _ctk_rounded_box_apply_outline_radius_for_style (&border_box, style, CTK_JUNCTION_NONE);

      render_border (cr, &border_box, border_width, 0, colors, border_style);
    }
}

gboolean
ctk_css_style_render_outline_get_clip (CtkCssStyle  *style,
                                       gdouble       x,
                                       gdouble       y,
                                       gdouble       width,
                                       gdouble       height,
                                       CdkRectangle *out_clip)
{
  cairo_rectangle_t rect;

  if (!ctk_css_style_render_has_outline (style))
    return FALSE;

  compute_outline_rect (style, x, y, width, height, &rect);

  out_clip->x = floor (rect.x);
  out_clip->y = floor (rect.y);
  out_clip->width = ceil (rect.x + rect.width) - out_clip->x;
  out_clip->height = ceil (rect.y + rect.height) - out_clip->y;

  return TRUE;
}
