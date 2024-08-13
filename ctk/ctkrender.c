/* CTK - The GIMP Toolkit
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#include "ctkrender.h"
#include "ctkrenderprivate.h"

#include <math.h>

#include "ctkcsscornervalueprivate.h"
#include "ctkcssimagebuiltinprivate.h"
#include "ctkcssimagevalueprivate.h"
#include "ctkcssnumbervalueprivate.h"
#include "ctkcssrgbavalueprivate.h"
#include "ctkcssshadowsvalueprivate.h"
#include "ctkcsstransformvalueprivate.h"
#include "ctkhslaprivate.h"
#include "ctkrenderbackgroundprivate.h"
#include "ctkrenderborderprivate.h"
#include "ctkrendericonprivate.h"
#include "ctkstylecontextprivate.h"

#include "fallback-c89.c"

static void
ctk_do_render_check (CtkStyleContext *context,
                     cairo_t         *cr,
                     gdouble          x,
                     gdouble          y,
                     gdouble          width,
                     gdouble          height)
{
  CtkStateFlags state;
  CtkCssImageBuiltinType image_type;

  state = ctk_style_context_get_state (context);
  if (state & CTK_STATE_FLAG_INCONSISTENT)
    image_type = CTK_CSS_IMAGE_BUILTIN_CHECK_INCONSISTENT;
  else if (state & CTK_STATE_FLAG_CHECKED)
    image_type = CTK_CSS_IMAGE_BUILTIN_CHECK;
  else
    image_type = CTK_CSS_IMAGE_BUILTIN_NONE;

  ctk_css_style_render_icon (ctk_style_context_lookup_style (context), cr, x, y, width, height, image_type);
}

/**
 * ctk_render_check:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders a checkmark (as in a #CtkCheckButton).
 *
 * The %CTK_STATE_FLAG_CHECKED state determines whether the check is
 * on or off, and %CTK_STATE_FLAG_INCONSISTENT determines whether it
 * should be marked as undefined.
 *
 * Typical checkmark rendering:
 *
 * ![](checks.png)
 *
 * Since: 3.0
 **/
void
ctk_render_check (CtkStyleContext *context,
                  cairo_t         *cr,
                  gdouble          x,
                  gdouble          y,
                  gdouble          width,
                  gdouble          height)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  ctk_do_render_check (context, cr, x, y, width, height);
}

static void
ctk_do_render_option (CtkStyleContext *context,
                      cairo_t         *cr,
                      gdouble          x,
                      gdouble          y,
                      gdouble          width,
                      gdouble          height)
{
  CtkStateFlags state;
  CtkCssImageBuiltinType image_type;

  state = ctk_style_context_get_state (context);
  if (state & CTK_STATE_FLAG_INCONSISTENT)
    image_type = CTK_CSS_IMAGE_BUILTIN_OPTION_INCONSISTENT;
  else if (state & CTK_STATE_FLAG_CHECKED)
    image_type = CTK_CSS_IMAGE_BUILTIN_OPTION;
  else
    image_type = CTK_CSS_IMAGE_BUILTIN_NONE;

  ctk_css_style_render_icon (ctk_style_context_lookup_style (context), cr, x, y, width, height, image_type);
}

/**
 * ctk_render_option:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders an option mark (as in a #CtkRadioButton), the %CTK_STATE_FLAG_CHECKED
 * state will determine whether the option is on or off, and
 * %CTK_STATE_FLAG_INCONSISTENT whether it should be marked as undefined.
 *
 * Typical option mark rendering:
 *
 * ![](options.png)
 *
 * Since: 3.0
 **/
void
ctk_render_option (CtkStyleContext *context,
                   cairo_t         *cr,
                   gdouble          x,
                   gdouble          y,
                   gdouble          width,
                   gdouble          height)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  ctk_do_render_option (context, cr, x, y, width, height);
}

static void
ctk_do_render_arrow (CtkStyleContext *context,
                     cairo_t         *cr,
                     gdouble          angle,
                     gdouble          x,
                     gdouble          y,
                     gdouble          size)
{
  CtkCssImageBuiltinType image_type;

  /* map [0, 2 * pi) to [0, 4) */
  angle = round (2 * angle / G_PI);

  switch (((int) angle) & 3)
  {
  case 0:
    image_type = CTK_CSS_IMAGE_BUILTIN_ARROW_UP;
    break;
  case 1:
    image_type = CTK_CSS_IMAGE_BUILTIN_ARROW_RIGHT;
    break;
  case 2:
    image_type = CTK_CSS_IMAGE_BUILTIN_ARROW_DOWN;
    break;
  case 3:
    image_type = CTK_CSS_IMAGE_BUILTIN_ARROW_LEFT;
    break;
  default:
    g_assert_not_reached ();
    image_type = CTK_CSS_IMAGE_BUILTIN_ARROW_UP;
    break;
  }

  ctk_css_style_render_icon (ctk_style_context_lookup_style (context), cr, x, y, size, size, image_type);
}

/**
 * ctk_render_arrow:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @angle: arrow angle from 0 to 2 * %G_PI, being 0 the arrow pointing to the north
 * @x: X origin of the render area
 * @y: Y origin of the render area
 * @size: square side for render area
 *
 * Renders an arrow pointing to @angle.
 *
 * Typical arrow rendering at 0, 1⁄2 π;, π; and 3⁄2 π:
 *
 * ![](arrows.png)
 *
 * Since: 3.0
 **/
void
ctk_render_arrow (CtkStyleContext *context,
                  cairo_t         *cr,
                  gdouble          angle,
                  gdouble          x,
                  gdouble          y,
                  gdouble          size)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (size <= 0)
    return;

  ctk_do_render_arrow (context, cr, angle, x, y, size);
}

/**
 * ctk_render_background:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders the background of an element.
 *
 * Typical background rendering, showing the effect of
 * `background-image`, `border-width` and `border-radius`:
 *
 * ![](background.png)
 *
 * Since: 3.0.
 **/
void
ctk_render_background (CtkStyleContext *context,
                       cairo_t         *cr,
                       gdouble          x,
                       gdouble          y,
                       gdouble          width,
                       gdouble          height)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  ctk_css_style_render_background (ctk_style_context_lookup_style (context),
                                   cr, x, y, width, height,
                                   ctk_style_context_get_junction_sides (context));
}

/**
 * ctk_render_background_get_clip:
 * @context: a #CtkStyleContext
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 * @out_clip: (out): return location for the clip
 *
 * Returns the area that will be affected (i.e. drawn to) when
 * calling ctk_render_background() for the given @context and
 * rectangle.
 *
 * Since: 3.20
 */
void
ctk_render_background_get_clip (CtkStyleContext *context,
                                gdouble          x,
                                gdouble          y,
                                gdouble          width,
                                gdouble          height,
                                CdkRectangle    *out_clip)
{
  CtkBorder shadow;

  _ctk_css_shadows_value_get_extents (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_BOX_SHADOW), &shadow);

  out_clip->x = floor (x) - shadow.left;
  out_clip->y = floor (y) - shadow.top;
  out_clip->width = ceil (width) + shadow.left + shadow.right;
  out_clip->height = ceil (height) + shadow.top + shadow.bottom;
}

/**
 * ctk_render_frame:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders a frame around the rectangle defined by @x, @y, @width, @height.
 *
 * Examples of frame rendering, showing the effect of `border-image`,
 * `border-color`, `border-width`, `border-radius` and junctions:
 *
 * ![](frames.png)
 *
 * Since: 3.0
 **/
void
ctk_render_frame (CtkStyleContext *context,
                  cairo_t         *cr,
                  gdouble          x,
                  gdouble          y,
                  gdouble          width,
                  gdouble          height)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  ctk_css_style_render_border (ctk_style_context_lookup_style (context),
                               cr,
                               x, y, width, height,
                               0,
                               ctk_style_context_get_junction_sides (context));
}

static void
ctk_do_render_expander (CtkStyleContext *context,
                        cairo_t         *cr,
                        gdouble          x,
                        gdouble          y,
                        gdouble          width,
                        gdouble          height)
{
  CtkCssImageBuiltinType image_type;
  CtkStateFlags state;

  state = ctk_style_context_get_state (context);
  if (ctk_style_context_has_class (context, "horizontal"))
    {
      if (state & CTK_STATE_FLAG_DIR_RTL)
        image_type = (state & CTK_STATE_FLAG_CHECKED)
                     ? CTK_CSS_IMAGE_BUILTIN_EXPANDER_HORIZONTAL_RIGHT_EXPANDED
                     : CTK_CSS_IMAGE_BUILTIN_EXPANDER_HORIZONTAL_RIGHT;
      else
        image_type = (state & CTK_STATE_FLAG_CHECKED)
                     ? CTK_CSS_IMAGE_BUILTIN_EXPANDER_HORIZONTAL_LEFT_EXPANDED
                     : CTK_CSS_IMAGE_BUILTIN_EXPANDER_HORIZONTAL_LEFT;
    }
  else
    {
      if (state & CTK_STATE_FLAG_DIR_RTL)
        image_type = (state & CTK_STATE_FLAG_CHECKED)
                     ? CTK_CSS_IMAGE_BUILTIN_EXPANDER_VERTICAL_RIGHT_EXPANDED
                     : CTK_CSS_IMAGE_BUILTIN_EXPANDER_VERTICAL_RIGHT;
      else
        image_type = (state & CTK_STATE_FLAG_CHECKED)
                     ? CTK_CSS_IMAGE_BUILTIN_EXPANDER_VERTICAL_LEFT_EXPANDED
                     : CTK_CSS_IMAGE_BUILTIN_EXPANDER_VERTICAL_LEFT;
    }

  ctk_css_style_render_icon (ctk_style_context_lookup_style (context), cr, x, y, width, height, image_type);
}

/**
 * ctk_render_expander:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders an expander (as used in #CtkTreeView and #CtkExpander) in the area
 * defined by @x, @y, @width, @height. The state %CTK_STATE_FLAG_CHECKED
 * determines whether the expander is collapsed or expanded.
 *
 * Typical expander rendering:
 *
 * ![](expanders.png)
 *
 * Since: 3.0
 **/
void
ctk_render_expander (CtkStyleContext *context,
                     cairo_t         *cr,
                     gdouble          x,
                     gdouble          y,
                     gdouble          width,
                     gdouble          height)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  ctk_do_render_expander (context, cr, x, y, width, height);
}

/**
 * ctk_render_focus:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders a focus indicator on the rectangle determined by @x, @y, @width, @height.
 *
 * Typical focus rendering:
 *
 * ![](focus.png)
 *
 * Since: 3.0
 **/
void
ctk_render_focus (CtkStyleContext *context,
                  cairo_t         *cr,
                  gdouble          x,
                  gdouble          y,
                  gdouble          width,
                  gdouble          height)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  ctk_css_style_render_outline (ctk_style_context_lookup_style (context),
                                cr,
                                x, y, width, height);
}

static void
prepare_context_for_layout (cairo_t *cr,
                            gdouble x,
                            gdouble y,
                            PangoLayout *layout)
{
  const PangoMatrix *matrix;

  matrix = pango_context_get_matrix (pango_layout_get_context (layout));

  cairo_move_to (cr, x, y);

  if (matrix)
    {
      cairo_matrix_t cairo_matrix;

      cairo_matrix_init (&cairo_matrix,
                         matrix->xx, matrix->yx,
                         matrix->xy, matrix->yy,
                         matrix->x0, matrix->y0);

      cairo_transform (cr, &cairo_matrix);
    }
}

static void
ctk_do_render_layout (CtkStyleContext *context,
                      cairo_t         *cr,
                      gdouble          x,
                      gdouble          y,
                      PangoLayout     *layout)
{
  const CdkRGBA *fg_color;

  cairo_save (cr);
  fg_color = _ctk_css_rgba_value_get_rgba (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_COLOR));

  prepare_context_for_layout (cr, x, y, layout);

  _ctk_css_shadows_value_paint_layout (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_TEXT_SHADOW),
                                       cr, layout);

  cdk_cairo_set_source_rgba (cr, fg_color);
  pango_cairo_show_layout (cr, layout);

  cairo_restore (cr);
}

/**
 * ctk_render_layout:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @x: X origin
 * @y: Y origin
 * @layout: the #PangoLayout to render
 *
 * Renders @layout on the coordinates @x, @y
 *
 * Since: 3.0
 **/
void
ctk_render_layout (CtkStyleContext *context,
                   cairo_t         *cr,
                   gdouble          x,
                   gdouble          y,
                   PangoLayout     *layout)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (PANGO_IS_LAYOUT (layout));
  g_return_if_fail (cr != NULL);

  ctk_do_render_layout (context, cr, x, y, layout);
}

static void
ctk_do_render_line (CtkStyleContext *context,
                    cairo_t         *cr,
                    gdouble          x0,
                    gdouble          y0,
                    gdouble          x1,
                    gdouble          y1)
{
  const CdkRGBA *color;

  cairo_save (cr);

  color = _ctk_css_rgba_value_get_rgba (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_COLOR));

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
  cairo_set_line_width (cr, 1);

  cairo_move_to (cr, x0 + 0.5, y0 + 0.5);
  cairo_line_to (cr, x1 + 0.5, y1 + 0.5);

  cdk_cairo_set_source_rgba (cr, color);
  cairo_stroke (cr);

  cairo_restore (cr);
}

/**
 * ctk_render_line:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @x0: X coordinate for the origin of the line
 * @y0: Y coordinate for the origin of the line
 * @x1: X coordinate for the end of the line
 * @y1: Y coordinate for the end of the line
 *
 * Renders a line from (x0, y0) to (x1, y1).
 *
 * Since: 3.0
 **/
void
ctk_render_line (CtkStyleContext *context,
                 cairo_t         *cr,
                 gdouble          x0,
                 gdouble          y0,
                 gdouble          x1,
                 gdouble          y1)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  ctk_do_render_line (context, cr, x0, y0, x1, y1);
}

static void
ctk_do_render_slider (CtkStyleContext *context,
                      cairo_t         *cr,
                      gdouble          x,
                      gdouble          y,
                      gdouble          width,
                      gdouble          height,
                      CtkOrientation   orientation G_GNUC_UNUSED)
{
  CtkCssStyle *style;
  CtkJunctionSides junction;

  style = ctk_style_context_lookup_style (context);
  junction = ctk_style_context_get_junction_sides (context);

  ctk_css_style_render_background (style,
                                   cr,
                                   x, y, width, height,
                                   junction);
  ctk_css_style_render_border (style,
                               cr,
                               x, y, width, height,
                               0,
                               junction);
}

/**
 * ctk_render_slider:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 * @orientation: orientation of the slider
 *
 * Renders a slider (as in #CtkScale) in the rectangle defined by @x, @y,
 * @width, @height. @orientation defines whether the slider is vertical
 * or horizontal.
 *
 * Typical slider rendering:
 *
 * ![](sliders.png)
 *
 * Since: 3.0
 **/
void
ctk_render_slider (CtkStyleContext *context,
                   cairo_t         *cr,
                   gdouble          x,
                   gdouble          y,
                   gdouble          width,
                   gdouble          height,
                   CtkOrientation   orientation)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  ctk_do_render_slider (context, cr, x, y, width, height, orientation);
}

static void
ctk_css_style_render_frame_gap (CtkCssStyle     *style,
                                cairo_t         *cr,
                                gdouble          x,
                                gdouble          y,
                                gdouble          width,
                                gdouble          height,
                                CtkPositionType  gap_side,
                                gdouble          xy0_gap,
                                gdouble          xy1_gap,
                                CtkJunctionSides junction)
{
  gint border_width;
  CtkCssValue *corner[4];
  gdouble x0, y0, x1, y1, xc = 0.0, yc = 0.0, wc = 0.0, hc = 0.0;
  CtkBorder border;

  border.top = _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_TOP_WIDTH), 100);
  border.right = _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_RIGHT_WIDTH), 100);
  border.bottom = _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_BOTTOM_WIDTH), 100);
  border.left = _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_LEFT_WIDTH), 100);
  corner[CTK_CSS_TOP_LEFT] = ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_TOP_LEFT_RADIUS);
  corner[CTK_CSS_TOP_RIGHT] = ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_TOP_RIGHT_RADIUS);
  corner[CTK_CSS_BOTTOM_LEFT] = ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_BOTTOM_LEFT_RADIUS);
  corner[CTK_CSS_BOTTOM_RIGHT] = ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_BOTTOM_RIGHT_RADIUS);

  border_width = MIN (MIN (border.top, border.bottom),
                      MIN (border.left, border.right));

  cairo_save (cr);

  switch (gap_side)
    {
    case CTK_POS_TOP:
      xc = x + xy0_gap + border_width;
      yc = y;
      wc = MAX (xy1_gap - xy0_gap - 2 * border_width, 0);
      hc = border_width;

      if (xy0_gap < _ctk_css_corner_value_get_x (corner[CTK_CSS_TOP_LEFT], width))
        junction |= CTK_JUNCTION_CORNER_TOPLEFT;

      if (xy1_gap > width - _ctk_css_corner_value_get_x (corner[CTK_CSS_TOP_RIGHT], width))
        junction |= CTK_JUNCTION_CORNER_TOPRIGHT;
      break;
    case CTK_POS_BOTTOM:
      xc = x + xy0_gap + border_width;
      yc = y + height - border_width;
      wc = MAX (xy1_gap - xy0_gap - 2 * border_width, 0);
      hc = border_width;

      if (xy0_gap < _ctk_css_corner_value_get_x (corner[CTK_CSS_BOTTOM_LEFT], width))
        junction |= CTK_JUNCTION_CORNER_BOTTOMLEFT;

      if (xy1_gap > width - _ctk_css_corner_value_get_x (corner[CTK_CSS_BOTTOM_RIGHT], width))
        junction |= CTK_JUNCTION_CORNER_BOTTOMRIGHT;

      break;
    case CTK_POS_LEFT:
      xc = x;
      yc = y + xy0_gap + border_width;
      wc = border_width;
      hc = MAX (xy1_gap - xy0_gap - 2 * border_width, 0);

      if (xy0_gap < _ctk_css_corner_value_get_y (corner[CTK_CSS_TOP_LEFT], height))
        junction |= CTK_JUNCTION_CORNER_TOPLEFT;

      if (xy1_gap > height - _ctk_css_corner_value_get_y (corner[CTK_CSS_BOTTOM_LEFT], height))
        junction |= CTK_JUNCTION_CORNER_BOTTOMLEFT;

      break;
    case CTK_POS_RIGHT:
      xc = x + width - border_width;
      yc = y + xy0_gap + border_width;
      wc = border_width;
      hc = MAX (xy1_gap - xy0_gap - 2 * border_width, 0);

      if (xy0_gap < _ctk_css_corner_value_get_y (corner[CTK_CSS_TOP_RIGHT], height))
        junction |= CTK_JUNCTION_CORNER_TOPRIGHT;

      if (xy1_gap > height - _ctk_css_corner_value_get_y (corner[CTK_CSS_BOTTOM_RIGHT], height))
        junction |= CTK_JUNCTION_CORNER_BOTTOMRIGHT;

      break;
    }

  cairo_clip_extents (cr, &x0, &y0, &x1, &y1);
  cairo_rectangle (cr, x0, y0, x1 - x0, yc - y0);
  cairo_rectangle (cr, x0, yc, xc - x0, hc);
  cairo_rectangle (cr, xc + wc, yc, x1 - (xc + wc), hc);
  cairo_rectangle (cr, x0, yc + hc, x1 - x0, y1 - (yc + hc));
  cairo_clip (cr);

  ctk_css_style_render_border (style, cr,
                               x, y, width, height,
                               0, junction);

  cairo_restore (cr);
}

/**
 * ctk_render_frame_gap:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 * @gap_side: side where the gap is
 * @xy0_gap: initial coordinate (X or Y depending on @gap_side) for the gap
 * @xy1_gap: end coordinate (X or Y depending on @gap_side) for the gap
 *
 * Renders a frame around the rectangle defined by (@x, @y, @width, @height),
 * leaving a gap on one side. @xy0_gap and @xy1_gap will mean X coordinates
 * for %CTK_POS_TOP and %CTK_POS_BOTTOM gap sides, and Y coordinates for
 * %CTK_POS_LEFT and %CTK_POS_RIGHT.
 *
 * Typical rendering of a frame with a gap:
 *
 * ![](frame-gap.png)
 *
 * Since: 3.0
 *
 * Deprecated: 3.24: Use ctk_render_frame() instead. Themes can create gaps
 *     by omitting borders via CSS.
 **/
void
ctk_render_frame_gap (CtkStyleContext *context,
                      cairo_t         *cr,
                      gdouble          x,
                      gdouble          y,
                      gdouble          width,
                      gdouble          height,
                      CtkPositionType  gap_side,
                      gdouble          xy0_gap,
                      gdouble          xy1_gap)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (xy0_gap <= xy1_gap);
  g_return_if_fail (xy0_gap >= 0);

  if (width <= 0 || height <= 0)
    return;

  if (gap_side == CTK_POS_LEFT ||
      gap_side == CTK_POS_RIGHT)
    g_return_if_fail (xy1_gap <= height);
  else
    g_return_if_fail (xy1_gap <= width);

  ctk_css_style_render_frame_gap (ctk_style_context_lookup_style (context),
                                  cr,
                                  x, y, width, height, gap_side,
                                  xy0_gap, xy1_gap,
                                  ctk_style_context_get_junction_sides (context));
}

static void
ctk_css_style_render_extension (CtkCssStyle     *style,
                                cairo_t         *cr,
                                gdouble          x,
                                gdouble          y,
                                gdouble          width,
                                gdouble          height,
                                CtkPositionType  gap_side)
{
  CtkJunctionSides junction = 0;
  guint hidden_side = 0;

  switch (gap_side)
    {
    case CTK_POS_LEFT:
      junction = CTK_JUNCTION_LEFT;
      hidden_side = (1 << CTK_CSS_LEFT);
      break;
    case CTK_POS_RIGHT:
      junction = CTK_JUNCTION_RIGHT;
      hidden_side = (1 << CTK_CSS_RIGHT);
      break;
    case CTK_POS_TOP:
      junction = CTK_JUNCTION_TOP;
      hidden_side = (1 << CTK_CSS_TOP);
      break;
    case CTK_POS_BOTTOM:
      junction = CTK_JUNCTION_BOTTOM;
      hidden_side = (1 << CTK_CSS_BOTTOM);
      break;
    }

  ctk_css_style_render_background (style,
                                   cr,
                                   x, y,
                                   width, height,
                                   junction);

  ctk_css_style_render_border (style, cr,
                               x, y, width, height,
                               hidden_side, junction);
}

/**
 * ctk_render_extension:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 * @gap_side: side where the gap is
 *
 * Renders a extension (as in a #CtkNotebook tab) in the rectangle
 * defined by @x, @y, @width, @height. The side where the extension
 * connects to is defined by @gap_side.
 *
 * Typical extension rendering:
 *
 * ![](extensions.png)
 *
 * Since: 3.0
 **/
void
ctk_render_extension (CtkStyleContext *context,
                      cairo_t         *cr,
                      gdouble          x,
                      gdouble          y,
                      gdouble          width,
                      gdouble          height,
                      CtkPositionType  gap_side)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  ctk_css_style_render_extension (ctk_style_context_lookup_style (context),
                                  cr,
                                  x, y, width, height,
                                  gap_side);
}

static void
ctk_do_render_handle (CtkStyleContext *context,
                      cairo_t         *cr,
                      gdouble          x,
                      gdouble          y,
                      gdouble          width,
                      gdouble          height)
{
  CtkCssImageBuiltinType type;

  ctk_render_background (context, cr, x, y, width, height);
  ctk_render_frame (context, cr, x, y, width, height);

  if (ctk_style_context_has_class (context, CTK_STYLE_CLASS_GRIP))
    {
      CtkJunctionSides sides = ctk_style_context_get_junction_sides (context);

      /* order is important here for when too many (or too few) sides are set */
      if ((sides & CTK_JUNCTION_CORNER_BOTTOMRIGHT) == CTK_JUNCTION_CORNER_BOTTOMRIGHT)
        type = CTK_CSS_IMAGE_BUILTIN_GRIP_BOTTOMRIGHT;
      else if ((sides & CTK_JUNCTION_CORNER_TOPRIGHT) == CTK_JUNCTION_CORNER_TOPRIGHT)
        type = CTK_CSS_IMAGE_BUILTIN_GRIP_TOPRIGHT;
      else if ((sides & CTK_JUNCTION_CORNER_BOTTOMLEFT) == CTK_JUNCTION_CORNER_BOTTOMLEFT)
        type = CTK_CSS_IMAGE_BUILTIN_GRIP_BOTTOMLEFT;
      else if ((sides & CTK_JUNCTION_CORNER_TOPLEFT) == CTK_JUNCTION_CORNER_TOPLEFT)
        type = CTK_CSS_IMAGE_BUILTIN_GRIP_TOPLEFT;
      else if (sides & CTK_JUNCTION_RIGHT)
        type = CTK_CSS_IMAGE_BUILTIN_GRIP_RIGHT;
      else if (sides & CTK_JUNCTION_BOTTOM)
        type = CTK_CSS_IMAGE_BUILTIN_GRIP_BOTTOM;
      else if (sides & CTK_JUNCTION_TOP)
        type = CTK_CSS_IMAGE_BUILTIN_GRIP_TOP;
      else if (sides & CTK_JUNCTION_LEFT)
        type = CTK_CSS_IMAGE_BUILTIN_GRIP_LEFT;
      else
        type = CTK_CSS_IMAGE_BUILTIN_GRIP_BOTTOMRIGHT;
    }
  else if (ctk_style_context_has_class (context, CTK_STYLE_CLASS_PANE_SEPARATOR))
    {
      type = CTK_CSS_IMAGE_BUILTIN_PANE_SEPARATOR;
    }
  else
    {
      type = CTK_CSS_IMAGE_BUILTIN_HANDLE;
    }

  ctk_css_style_render_icon (ctk_style_context_lookup_style (context), cr, x, y, width, height, type);
}

/**
 * ctk_render_handle:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders a handle (as in #CtkHandleBox, #CtkPaned and
 * #CtkWindow’s resize grip), in the rectangle
 * determined by @x, @y, @width, @height.
 *
 * Handles rendered for the paned and grip classes:
 *
 * ![](handles.png)
 *
 * Since: 3.0
 **/
void
ctk_render_handle (CtkStyleContext *context,
                   cairo_t         *cr,
                   gdouble          x,
                   gdouble          y,
                   gdouble          width,
                   gdouble          height)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  ctk_do_render_handle (context, cr, x, y, width, height);
}

/**
 * ctk_render_activity:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @x: X origin of the rectangle
 * @y: Y origin of the rectangle
 * @width: rectangle width
 * @height: rectangle height
 *
 * Renders an activity indicator (such as in #CtkSpinner).
 * The state %CTK_STATE_FLAG_CHECKED determines whether there is
 * activity going on.
 *
 * Since: 3.0
 **/
void
ctk_render_activity (CtkStyleContext *context,
                     cairo_t         *cr,
                     gdouble          x,
                     gdouble          y,
                     gdouble          width,
                     gdouble          height)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  if (width <= 0 || height <= 0)
    return;

  ctk_css_style_render_icon (ctk_style_context_lookup_style (context), cr, x, y, width, height, CTK_CSS_IMAGE_BUILTIN_SPINNER);
}

static GdkPixbuf *
scale_or_ref (GdkPixbuf *src,
              gint       width,
              gint       height)
{
  if (width == gdk_pixbuf_get_width (src) &&
      height == gdk_pixbuf_get_height (src))
    return g_object_ref (src);
  else
    return gdk_pixbuf_scale_simple (src,
                                    width, height,
                                    GDK_INTERP_BILINEAR);
}

GdkPixbuf *
ctk_render_icon_pixbuf_unpacked (GdkPixbuf           *base_pixbuf,
                                 CtkIconSize          size,
                                 CtkCssIconEffect     icon_effect)
{
  GdkPixbuf *scaled;
  GdkPixbuf *stated;
  cairo_surface_t *surface;

  g_return_val_if_fail (base_pixbuf != NULL, NULL);

  /* If the size was wildcarded, and we're allowed to scale, then scale; otherwise,
   * leave it alone.
   */
  if (size != (CtkIconSize) -1)
    {
      int width = 1;
      int height = 1;

      if (!ctk_icon_size_lookup (size, &width, &height))
        {
          g_warning (G_STRLOC ": invalid icon size '%d'", size);
          return NULL;
        }

      scaled = scale_or_ref (base_pixbuf, width, height);
    }
  else
    {
      scaled = g_object_ref (base_pixbuf);
    }

  if (icon_effect != CTK_CSS_ICON_EFFECT_NONE)
    {
      surface = cdk_cairo_surface_create_from_pixbuf (scaled, 1, NULL);
      ctk_css_icon_effect_apply (icon_effect, surface);
      stated = gdk_pixbuf_get_from_surface (surface, 0, 0,
					    cairo_image_surface_get_width (surface),
					    cairo_image_surface_get_height (surface));
      cairo_surface_destroy (surface);
    }
  else
    {
      stated = scaled;
    }

  return stated;
}

/**
 * ctk_render_icon_pixbuf:
 * @context: a #CtkStyleContext
 * @source: the #CtkIconSource specifying the icon to render
 * @size: (type int): the size (#CtkIconSize) to render the icon at.
 *        A size of `(CtkIconSize) -1` means render at the size of the source
 *        and don’t scale.
 *
 * Renders the icon specified by @source at the given @size, returning the result
 * in a pixbuf.
 *
 * Returns: (transfer full): a newly-created #GdkPixbuf containing the rendered icon
 *
 * Since: 3.0
 *
 * Deprecated: 3.10: Use ctk_icon_theme_load_icon() instead.
 **/
GdkPixbuf *
ctk_render_icon_pixbuf (CtkStyleContext     *context,
                        const CtkIconSource *source,
                        CtkIconSize          size)
{
  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), NULL);
  g_return_val_if_fail (size > CTK_ICON_SIZE_INVALID || size == (CtkIconSize)-1, NULL);
  g_return_val_if_fail (source != NULL, NULL);

  return ctk_render_icon_pixbuf_unpacked (ctk_icon_source_get_pixbuf (source),
                                          ctk_icon_source_get_size_wildcarded (source) ? size : -1,
                                          ctk_icon_source_get_state_wildcarded (source)
                                          ? _ctk_css_icon_effect_value_get (
                                             _ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_ICON_EFFECT))
                                          : CTK_CSS_ICON_EFFECT_NONE);
}

/**
 * ctk_render_icon:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @pixbuf: a #GdkPixbuf containing the icon to draw
 * @x: X position for the @pixbuf
 * @y: Y position for the @pixbuf
 *
 * Renders the icon in @pixbuf at the specified @x and @y coordinates.
 *
 * This function will render the icon in @pixbuf at exactly its size,
 * regardless of scaling factors, which may not be appropriate when
 * drawing on displays with high pixel densities.
 *
 * You probably want to use ctk_render_icon_surface() instead, if you
 * already have a Cairo surface.
 *
 * Since: 3.2
 **/
void
ctk_render_icon (CtkStyleContext *context,
                 cairo_t         *cr,
                 GdkPixbuf       *pixbuf,
                 gdouble          x,
                 gdouble          y)
{
  cairo_surface_t *surface;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  surface = cdk_cairo_surface_create_from_pixbuf (pixbuf, 1, NULL);

  ctk_css_style_render_icon_surface (ctk_style_context_lookup_style (context),
                                     cr,
                                     surface,
                                     x, y);

  cairo_surface_destroy (surface);
}

/**
 * ctk_render_icon_surface:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @surface: a #cairo_surface_t containing the icon to draw
 * @x: X position for the @icon
 * @y: Y position for the @incon
 *
 * Renders the icon in @surface at the specified @x and @y coordinates.
 *
 * Since: 3.10
 **/
void
ctk_render_icon_surface (CtkStyleContext *context,
			 cairo_t         *cr,
			 cairo_surface_t *surface,
			 gdouble          x,
			 gdouble          y)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  ctk_css_style_render_icon_surface (ctk_style_context_lookup_style (context),
                                     cr,
                                     surface,
                                     x, y);
}

/*
 * ctk_render_content_path:
 * @context: style context to get style information from
 * @cr: cairo context to add path to
 * @x: x coordinate of CSS box
 * @y: y coordinate of CSS box
 * @width: width of CSS box
 * @height: height of CSS box
 *
 * Adds the path of the content box to @cr for a given border box.
 * This function respects rounded corners.
 *
 * This is useful if you are drawing content that is supposed to
 * fill the whole content area, like the color buttons in
 * #CtkColorChooserDialog.
 **/
void
ctk_render_content_path (CtkStyleContext *context,
                         cairo_t         *cr,
                         double           x,
                         double           y,
                         double           width,
                         double           height)
{
  CtkRoundedBox box;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);

  _ctk_rounded_box_init_rect (&box, x, y, width, height);
  _ctk_rounded_box_apply_border_radius_for_style (&box, ctk_style_context_lookup_style (context), 0);

  _ctk_rounded_box_shrink (&box,
                           _ctk_css_number_value_get (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_BORDER_TOP_WIDTH), 100)
                           + _ctk_css_number_value_get (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_PADDING_TOP), 100),
                           _ctk_css_number_value_get (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_BORDER_RIGHT_WIDTH), 100)
                           + _ctk_css_number_value_get (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_PADDING_RIGHT), 100),
                           _ctk_css_number_value_get (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_BORDER_BOTTOM_WIDTH), 100)
                           + _ctk_css_number_value_get (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_PADDING_BOTTOM), 100),
                           _ctk_css_number_value_get (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_BORDER_LEFT_WIDTH), 100)
                           + _ctk_css_number_value_get (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_PADDING_LEFT), 100));

  _ctk_rounded_box_path (&box, cr);
}
