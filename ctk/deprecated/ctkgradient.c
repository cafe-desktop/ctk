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

#define CDK_DISABLE_DEPRECATION_WARNINGS

#include "ctkgradientprivate.h"
#include "ctkcsscolorvalueprivate.h"
#include "ctkcssrgbavalueprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkstyleproperties.h"
#include "ctksymboliccolorprivate.h"

/**
 * SECTION:ctkgradient
 * @Short_description: Gradients
 * @Title: CtkGradient
 *
 * CtkGradient is a boxed type that represents a gradient.
 * It is the result of parsing a
 * [gradient expression][ctkcssprovider-gradients].
 * To obtain the gradient represented by a CtkGradient, it has to
 * be resolved with ctk_gradient_resolve(), which replaces all
 * symbolic color references by the colors they refer to (in a given
 * context) and constructs a #cairo_pattern_t value.
 *
 * It is not normally necessary to deal directly with #CtkGradients,
 * since they are mostly used behind the scenes by #CtkStyleContext and
 * #CtkCssProvider.
 *
 * #CtkGradient is deprecated. It was used internally by CTK’s CSS engine
 * to represent gradients. As its handling is not conforming to modern
 * web standards, it is not used anymore. If you want to use gradients in
 * your own code, please use Cairo directly.
 */

G_DEFINE_BOXED_TYPE (CtkGradient, ctk_gradient,
                     ctk_gradient_ref, ctk_gradient_unref)

typedef struct ColorStop ColorStop;

struct ColorStop
{
  gdouble offset;
  CtkSymbolicColor *color;
};

struct _CtkGradient
{
  gdouble x0;
  gdouble y0;
  gdouble x1;
  gdouble y1;
  gdouble radius0;
  gdouble radius1;

  GArray *stops;

  guint ref_count;
};

/**
 * ctk_gradient_new_linear:
 * @x0: X coordinate of the starting point
 * @y0: Y coordinate of the starting point
 * @x1: X coordinate of the end point
 * @y1: Y coordinate of the end point
 *
 * Creates a new linear gradient along the line defined by (x0, y0) and (x1, y1). Before using the gradient
 * a number of stop colors must be added through ctk_gradient_add_color_stop().
 *
 * Returns: A newly created #CtkGradient
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: #CtkGradient is deprecated.
 **/
CtkGradient *
ctk_gradient_new_linear (gdouble x0,
                         gdouble y0,
                         gdouble x1,
                         gdouble y1)
{
  CtkGradient *gradient;

  gradient = g_slice_new (CtkGradient);
  gradient->stops = g_array_new (FALSE, FALSE, sizeof (ColorStop));

  gradient->x0 = x0;
  gradient->y0 = y0;
  gradient->x1 = x1;
  gradient->y1 = y1;
  gradient->radius0 = 0;
  gradient->radius1 = 0;

  gradient->ref_count = 1;

  return gradient;
}

/**
 * ctk_gradient_new_radial:
 * @x0: X coordinate of the start circle
 * @y0: Y coordinate of the start circle
 * @radius0: radius of the start circle
 * @x1: X coordinate of the end circle
 * @y1: Y coordinate of the end circle
 * @radius1: radius of the end circle
 *
 * Creates a new radial gradient along the two circles defined by (x0, y0, radius0) and
 * (x1, y1, radius1). Before using the gradient a number of stop colors must be added
 * through ctk_gradient_add_color_stop().
 *
 * Returns: A newly created #CtkGradient
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: #CtkGradient is deprecated.
 **/
CtkGradient *
ctk_gradient_new_radial (gdouble x0,
			 gdouble y0,
			 gdouble radius0,
			 gdouble x1,
			 gdouble y1,
			 gdouble radius1)
{
  CtkGradient *gradient;

  gradient = g_slice_new (CtkGradient);
  gradient->stops = g_array_new (FALSE, FALSE, sizeof (ColorStop));

  gradient->x0 = x0;
  gradient->y0 = y0;
  gradient->x1 = x1;
  gradient->y1 = y1;
  gradient->radius0 = radius0;
  gradient->radius1 = radius1;

  gradient->ref_count = 1;

  return gradient;
}

/**
 * ctk_gradient_add_color_stop:
 * @gradient: a #CtkGradient
 * @offset: offset for the color stop
 * @color: color to use
 *
 * Adds a stop color to @gradient.
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: #CtkGradient is deprecated.
 **/
void
ctk_gradient_add_color_stop (CtkGradient      *gradient,
                             gdouble           offset,
                             CtkSymbolicColor *color)
{
  ColorStop stop;

  g_return_if_fail (gradient != NULL);

  stop.offset = offset;
  stop.color = ctk_symbolic_color_ref (color);

  g_array_append_val (gradient->stops, stop);
}

/**
 * ctk_gradient_ref:
 * @gradient: a #CtkGradient
 *
 * Increases the reference count of @gradient.
 *
 * Returns: The same @gradient
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: #CtkGradient is deprecated.
 **/
CtkGradient *
ctk_gradient_ref (CtkGradient *gradient)
{
  g_return_val_if_fail (gradient != NULL, NULL);

  gradient->ref_count++;

  return gradient;
}

/**
 * ctk_gradient_unref:
 * @gradient: a #CtkGradient
 *
 * Decreases the reference count of @gradient, freeing its memory
 * if the reference count reaches 0.
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: #CtkGradient is deprecated.
 **/
void
ctk_gradient_unref (CtkGradient *gradient)
{
  g_return_if_fail (gradient != NULL);

  gradient->ref_count--;

  if (gradient->ref_count == 0)
    {
      guint i;

      for (i = 0; i < gradient->stops->len; i++)
        {
          ColorStop *stop;

          stop = &g_array_index (gradient->stops, ColorStop, i);
          ctk_symbolic_color_unref (stop->color);
        }

      g_array_free (gradient->stops, TRUE);
      g_slice_free (CtkGradient, gradient);
    }
}

/**
 * ctk_gradient_resolve:
 * @gradient: a #CtkGradient
 * @props: #CtkStyleProperties to use when resolving named colors
 * @resolved_gradient: (out): return location for the resolved pattern
 *
 * If @gradient is resolvable, @resolved_gradient will be filled in
 * with the resolved gradient as a cairo_pattern_t, and %TRUE will
 * be returned. Generally, if @gradient can’t be resolved, it is
 * due to it being defined on top of a named color that doesn't
 * exist in @props.
 *
 * Returns: %TRUE if the gradient has been resolved
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: #CtkGradient is deprecated.
 **/
gboolean
ctk_gradient_resolve (CtkGradient         *gradient,
                      CtkStyleProperties  *props,
                      cairo_pattern_t    **resolved_gradient)
{
  cairo_pattern_t *pattern;
  guint i;

  g_return_val_if_fail (gradient != NULL, FALSE);
  g_return_val_if_fail (CTK_IS_STYLE_PROPERTIES (props), FALSE);
  g_return_val_if_fail (resolved_gradient != NULL, FALSE);

  if (gradient->radius0 == 0 && gradient->radius1 == 0)
    pattern = cairo_pattern_create_linear (gradient->x0, gradient->y0,
                                           gradient->x1, gradient->y1);
  else
    pattern = cairo_pattern_create_radial (gradient->x0, gradient->y0,
                                           gradient->radius0,
                                           gradient->x1, gradient->y1,
                                           gradient->radius1);

  for (i = 0; i < gradient->stops->len; i++)
    {
      ColorStop *stop;
      CdkRGBA color;

      stop = &g_array_index (gradient->stops, ColorStop, i);

      if (!ctk_symbolic_color_resolve (stop->color, props, &color))
        {
          cairo_pattern_destroy (pattern);
          return FALSE;
        }

      cairo_pattern_add_color_stop_rgba (pattern, stop->offset,
                                         color.red, color.green,
                                         color.blue, color.alpha);
    }

  *resolved_gradient = pattern;
  return TRUE;
}

cairo_pattern_t *
_ctk_gradient_resolve_full (CtkGradient             *gradient,
                            CtkStyleProviderPrivate *provider,
                            CtkCssStyle             *style,
                            CtkCssStyle             *parent_style)
{
  cairo_pattern_t *pattern;
  guint i;

  g_return_val_if_fail (gradient != NULL, NULL);
  g_return_val_if_fail (CTK_IS_STYLE_PROVIDER (provider), NULL);
  g_return_val_if_fail (CTK_IS_CSS_STYLE (style), NULL);
  g_return_val_if_fail (parent_style == NULL || CTK_IS_CSS_STYLE (parent_style), NULL);

  if (gradient->radius0 == 0 && gradient->radius1 == 0)
    pattern = cairo_pattern_create_linear (gradient->x0, gradient->y0,
                                           gradient->x1, gradient->y1);
  else
    pattern = cairo_pattern_create_radial (gradient->x0, gradient->y0,
                                           gradient->radius0,
                                           gradient->x1, gradient->y1,
                                           gradient->radius1);

  for (i = 0; i < gradient->stops->len; i++)
    {
      ColorStop *stop;
      CtkCssValue *val;
      CdkRGBA rgba;

      stop = &g_array_index (gradient->stops, ColorStop, i);

      /* if color resolving fails, assume transparency */
      val = _ctk_css_color_value_resolve (_ctk_symbolic_color_get_css_value (stop->color),
                                          provider,
                                          ctk_css_style_get_value (style, CTK_CSS_PROPERTY_COLOR),
                                          NULL);
      if (val)
        {
          rgba = *_ctk_css_rgba_value_get_rgba (val);
          _ctk_css_value_unref (val);
        }
      else
        {
          rgba.red = rgba.green = rgba.blue = rgba.alpha = 0.0;
        }

      cairo_pattern_add_color_stop_rgba (pattern, stop->offset,
                                         rgba.red, rgba.green,
                                         rgba.blue, rgba.alpha);
    }

  return pattern;
}

static void
append_number (GString    *str,
               double      d,
               const char *zero,
               const char *half,
               const char *one)
{
  if (zero && d == 0.0)
    g_string_append (str, zero);
  else if (half && d == 0.5)
    g_string_append (str, half);
  else if (one && d == 1.0)
    g_string_append (str, one);
  else
    {
      char buf[G_ASCII_DTOSTR_BUF_SIZE];

      g_ascii_dtostr (buf, sizeof (buf), d);
      g_string_append (str, buf);
    }
}

/**
 * ctk_gradient_to_string:
 * @gradient: the gradient to print
 *
 * Creates a string representation for @gradient that is suitable
 * for using in CTK CSS files.
 *
 * Returns: A string representation for @gradient
 *
 * Deprecated: 3.8: #CtkGradient is deprecated.
 **/
char *
ctk_gradient_to_string (CtkGradient *gradient)
{
  GString *str;
  guint i;

  g_return_val_if_fail (gradient != NULL, NULL);

  str = g_string_new ("-ctk-gradient (");

  if (gradient->radius0 == 0 && gradient->radius1 == 0)
    {
      g_string_append (str, "linear, ");
      append_number (str, gradient->x0, "left", "center", "right");
      g_string_append_c (str, ' ');
      append_number (str, gradient->y0, "top", "center", "bottom");
      g_string_append (str, ", ");
      append_number (str, gradient->x1, "left", "center", "right");
      g_string_append_c (str, ' ');
      append_number (str, gradient->y1, "top", "center", "bottom");
    }
  else
    {
      g_string_append (str, "radial, ");
      append_number (str, gradient->x0, "left", "center", "right");
      g_string_append_c (str, ' ');
      append_number (str, gradient->y0, "top", "center", "bottom");
      g_string_append (str, ", ");
      append_number (str, gradient->radius0, NULL, NULL, NULL);
      g_string_append (str, ", ");
      append_number (str, gradient->x1, "left", "center", "right");
      g_string_append_c (str, ' ');
      append_number (str, gradient->y1, "top", "center", "bottom");
      g_string_append (str, ", ");
      append_number (str, gradient->radius1, NULL, NULL, NULL);
    }
  
  for (i = 0; i < gradient->stops->len; i++)
    {
      ColorStop *stop;
      char *s;

      stop = &g_array_index (gradient->stops, ColorStop, i);

      g_string_append (str, ", ");

      if (stop->offset == 0.0)
        g_string_append (str, "from (");
      else if (stop->offset == 1.0)
        g_string_append (str, "to (");
      else
        {
          g_string_append (str, "color-stop (");
          append_number (str, stop->offset, NULL, NULL, NULL);
          g_string_append (str, ", ");
        }

      s = ctk_symbolic_color_to_string (stop->color);
      g_string_append (str, s);
      g_free (s);

      g_string_append (str, ")");
    }

  g_string_append (str, ")");

  return g_string_free (str, FALSE);
}

static CtkGradient *
ctk_gradient_fade (CtkGradient *gradient,
                   double       opacity)
{
  CtkGradient *faded;
  guint i;

  faded = g_slice_new (CtkGradient);
  faded->stops = g_array_new (FALSE, FALSE, sizeof (ColorStop));

  faded->x0 = gradient->x0;
  faded->y0 = gradient->y0;
  faded->x1 = gradient->x1;
  faded->y1 = gradient->y1;
  faded->radius0 = gradient->radius0;
  faded->radius1 = gradient->radius1;

  faded->ref_count = 1;

  for (i = 0; i < gradient->stops->len; i++)
    {
      CtkSymbolicColor *color;
      ColorStop *stop;

      stop = &g_array_index (gradient->stops, ColorStop, i);
      color = ctk_symbolic_color_new_alpha (stop->color, opacity);
      ctk_gradient_add_color_stop (faded, stop->offset, color);
      ctk_symbolic_color_unref (color);
    }

  return faded;
}

CtkGradient *
_ctk_gradient_transition (CtkGradient *start,
                          CtkGradient *end,
                          guint        property_id G_GNUC_UNUSED,
                          double       progress)
{
  CtkGradient *gradient;
  guint i;

  g_return_val_if_fail (start != NULL, NULL);

  if (end == NULL)
    return ctk_gradient_fade (start, 1.0 - CLAMP (progress, 0.0, 1.0));

  if (start->stops->len != end->stops->len)
    return NULL;

  /* check both are radial/linear */
  if ((start->radius0 == 0 && start->radius1 == 0) != (end->radius0 == 0 && end->radius1 == 0))
    return NULL;

  gradient = g_slice_new (CtkGradient);
  gradient->stops = g_array_new (FALSE, FALSE, sizeof (ColorStop));

  gradient->x0 = (1 - progress) * start->x0 + progress * end->x0;
  gradient->y0 = (1 - progress) * start->y0 + progress * end->y0;
  gradient->x1 = (1 - progress) * start->x1 + progress * end->x1;
  gradient->y1 = (1 - progress) * start->y1 + progress * end->y1;
  gradient->radius0 = (1 - progress) * start->radius0 + progress * end->radius0;
  gradient->radius1 = (1 - progress) * start->radius1 + progress * end->radius1;

  gradient->ref_count = 1;

  for (i = 0; i < start->stops->len; i++)
    {
      ColorStop *start_stop, *end_stop;
      CtkSymbolicColor *color;
      double offset;

      start_stop = &g_array_index (start->stops, ColorStop, i);
      end_stop = &g_array_index (end->stops, ColorStop, i);

      offset = (1 - progress) * start_stop->offset + progress * end_stop->offset;
      color = ctk_symbolic_color_new_mix (start_stop->color,
                                          end_stop->color,
                                          progress);
      ctk_gradient_add_color_stop (gradient, offset, color);
      ctk_symbolic_color_unref (color);
    }

  return gradient;
}
