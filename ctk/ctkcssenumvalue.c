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

#include "gtkcssenumvalueprivate.h"

#include "gtkcssstyleprivate.h"
#include "gtkcssnumbervalueprivate.h"
#include "gtkstyleproviderprivate.h"
#include "gtksettingsprivate.h"

/* repeated API */

struct _GtkCssValue {
  CTK_CSS_VALUE_BASE
  int value;
  const char *name;
};

static void
ctk_css_value_enum_free (GtkCssValue *value)
{
  g_slice_free (GtkCssValue, value);
}

static GtkCssValue *
ctk_css_value_enum_compute (GtkCssValue             *value,
                            guint                    property_id,
                            GtkStyleProviderPrivate *provider,
                            GtkCssStyle             *style,
                            GtkCssStyle             *parent_style)
{
  return _ctk_css_value_ref (value);
}

static gboolean
ctk_css_value_enum_equal (const GtkCssValue *enum1,
                          const GtkCssValue *enum2)
{
  return enum1 == enum2;
}

static GtkCssValue *
ctk_css_value_enum_transition (GtkCssValue *start,
                               GtkCssValue *end,
                               guint        property_id,
                               double       progress)
{
  return NULL;
}

static void
ctk_css_value_enum_print (const GtkCssValue *value,
                          GString           *string)
{
  g_string_append (string, value->name);
}

/* GtkBorderStyle */

static const GtkCssValueClass CTK_CSS_VALUE_BORDER_STYLE = {
  ctk_css_value_enum_free,
  ctk_css_value_enum_compute,
  ctk_css_value_enum_equal,
  ctk_css_value_enum_transition,
  ctk_css_value_enum_print
};

static GtkCssValue border_style_values[] = {
  { &CTK_CSS_VALUE_BORDER_STYLE, 1, CTK_BORDER_STYLE_NONE, "none" },
  { &CTK_CSS_VALUE_BORDER_STYLE, 1, CTK_BORDER_STYLE_SOLID, "solid" },
  { &CTK_CSS_VALUE_BORDER_STYLE, 1, CTK_BORDER_STYLE_INSET, "inset" },
  { &CTK_CSS_VALUE_BORDER_STYLE, 1, CTK_BORDER_STYLE_OUTSET, "outset" },
  { &CTK_CSS_VALUE_BORDER_STYLE, 1, CTK_BORDER_STYLE_HIDDEN, "hidden" },
  { &CTK_CSS_VALUE_BORDER_STYLE, 1, CTK_BORDER_STYLE_DOTTED, "dotted" },
  { &CTK_CSS_VALUE_BORDER_STYLE, 1, CTK_BORDER_STYLE_DASHED, "dashed" },
  { &CTK_CSS_VALUE_BORDER_STYLE, 1, CTK_BORDER_STYLE_DOUBLE, "double" },
  { &CTK_CSS_VALUE_BORDER_STYLE, 1, CTK_BORDER_STYLE_GROOVE, "groove" },
  { &CTK_CSS_VALUE_BORDER_STYLE, 1, CTK_BORDER_STYLE_RIDGE, "ridge" }
};

GtkCssValue *
_ctk_css_border_style_value_new (GtkBorderStyle border_style)
{
  g_return_val_if_fail (border_style < G_N_ELEMENTS (border_style_values), NULL);

  return _ctk_css_value_ref (&border_style_values[border_style]);
}

GtkCssValue *
_ctk_css_border_style_value_try_parse (GtkCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (border_style_values); i++)
    {
      if (_ctk_css_parser_try (parser, border_style_values[i].name, TRUE))
        return _ctk_css_value_ref (&border_style_values[i]);
    }

  return NULL;
}

GtkBorderStyle
_ctk_css_border_style_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_BORDER_STYLE, CTK_BORDER_STYLE_NONE);

  return value->value;
}

/* GtkCssBlendMode */

static const GtkCssValueClass CTK_CSS_VALUE_BLEND_MODE = {
  ctk_css_value_enum_free,
  ctk_css_value_enum_compute,
  ctk_css_value_enum_equal,
  ctk_css_value_enum_transition,
  ctk_css_value_enum_print
};

static GtkCssValue blend_mode_values[] = {
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_COLOR_BURN, "color-burn" },
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_COLOR_DODGE, "color-dodge" },
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_COLOR, "color" },
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_DARKEN, "darken" },
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_DIFFERENCE, "difference" },
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_EXCLUSION, "exclusion" },
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_HARD_LIGHT, "hard-light" },
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_HUE, "hue" },
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_LIGHTEN, "lighten" },
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_LUMINOSITY, "luminosity" },
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_MULTIPLY, "multiply" },
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_NORMAL, "normal" },
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_OVERLAY, "overlay" },
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_SATURATE, "saturate" },
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_SCREEN, "screen" },
  { &CTK_CSS_VALUE_BLEND_MODE, 1, CTK_CSS_BLEND_MODE_SOFT_LIGHT, "soft-light" }
};

GtkCssValue *
_ctk_css_blend_mode_value_new (GtkCssBlendMode blend_mode)
{
  g_return_val_if_fail (blend_mode < G_N_ELEMENTS (blend_mode_values), NULL);

  return _ctk_css_value_ref (&blend_mode_values[blend_mode]);
}

GtkCssValue *
_ctk_css_blend_mode_value_try_parse (GtkCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (blend_mode_values); i++)
    {
      if (_ctk_css_parser_try (parser, blend_mode_values[i].name, TRUE))
        return _ctk_css_value_ref (&blend_mode_values[i]);
    }

  return NULL;
}

GtkCssBlendMode
_ctk_css_blend_mode_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_BLEND_MODE, CTK_CSS_BLEND_MODE_NORMAL);

  return value->value;
}

/* GtkCssFontSize */

static double
get_dpi (GtkCssStyle *style)
{
  return _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_DPI), 96);
}

/* XXX: Kinda bad to have that machinery here, nobody expects vital font
 * size code to appear in gtkcssvalueenum.c.
 */
#define DEFAULT_FONT_SIZE_PT 10

double
ctk_css_font_size_get_default_px (GtkStyleProviderPrivate *provider,
                                  GtkCssStyle             *style)
{
  GtkSettings *settings;
  int font_size;

  settings = _ctk_style_provider_private_get_settings (provider);
  if (settings == NULL)
    return DEFAULT_FONT_SIZE_PT * get_dpi (style) / 72.0;

  font_size = ctk_settings_get_font_size (settings);
  if (font_size == 0)
    return DEFAULT_FONT_SIZE_PT * get_dpi (style) / 72.0;
  else if (ctk_settings_get_font_size_is_absolute (settings))
    return (double) font_size / PANGO_SCALE;
  else
    return ((double) font_size / PANGO_SCALE) * get_dpi (style) / 72.0;
}

static GtkCssValue *
ctk_css_value_font_size_compute (GtkCssValue             *value,
                                 guint                    property_id,
                                 GtkStyleProviderPrivate *provider,
                                 GtkCssStyle             *style,
                                 GtkCssStyle             *parent_style)
{
  double font_size;

  switch (value->value)
    {
    case CTK_CSS_FONT_SIZE_XX_SMALL:
      font_size = ctk_css_font_size_get_default_px (provider, style) * 3. / 5;
      break;
    case CTK_CSS_FONT_SIZE_X_SMALL:
      font_size = ctk_css_font_size_get_default_px (provider, style) * 3. / 4;
      break;
    case CTK_CSS_FONT_SIZE_SMALL:
      font_size = ctk_css_font_size_get_default_px (provider, style) * 8. / 9;
      break;
    default:
      g_assert_not_reached ();
      /* fall thru */
    case CTK_CSS_FONT_SIZE_MEDIUM:
      font_size = ctk_css_font_size_get_default_px (provider, style);
      break;
    case CTK_CSS_FONT_SIZE_LARGE:
      font_size = ctk_css_font_size_get_default_px (provider, style) * 6. / 5;
      break;
    case CTK_CSS_FONT_SIZE_X_LARGE:
      font_size = ctk_css_font_size_get_default_px (provider, style) * 3. / 2;
      break;
    case CTK_CSS_FONT_SIZE_XX_LARGE:
      font_size = ctk_css_font_size_get_default_px (provider, style) * 2;
      break;
    case CTK_CSS_FONT_SIZE_SMALLER:
      if (parent_style)
        font_size = _ctk_css_number_value_get (ctk_css_style_get_value (parent_style, CTK_CSS_PROPERTY_FONT_SIZE), 100);
      else
        font_size = ctk_css_font_size_get_default_px (provider, style);
      /* XXX: This is what WebKit does... */
      font_size /= 1.2;
      break;
    case CTK_CSS_FONT_SIZE_LARGER:
      if (parent_style)
        font_size = _ctk_css_number_value_get (ctk_css_style_get_value (parent_style, CTK_CSS_PROPERTY_FONT_SIZE), 100);
      else
        font_size = ctk_css_font_size_get_default_px (provider, style);
      /* XXX: This is what WebKit does... */
      font_size *= 1.2;
      break;
  }

  return _ctk_css_number_value_new (font_size, CTK_CSS_PX);
}

static const GtkCssValueClass CTK_CSS_VALUE_FONT_SIZE = {
  ctk_css_value_enum_free,
  ctk_css_value_font_size_compute,
  ctk_css_value_enum_equal,
  ctk_css_value_enum_transition,
  ctk_css_value_enum_print
};

static GtkCssValue font_size_values[] = {
  { &CTK_CSS_VALUE_FONT_SIZE, 1, CTK_CSS_FONT_SIZE_SMALLER, "smaller" },
  { &CTK_CSS_VALUE_FONT_SIZE, 1, CTK_CSS_FONT_SIZE_LARGER, "larger" },
  { &CTK_CSS_VALUE_FONT_SIZE, 1, CTK_CSS_FONT_SIZE_XX_SMALL, "xx-small" },
  { &CTK_CSS_VALUE_FONT_SIZE, 1, CTK_CSS_FONT_SIZE_X_SMALL, "x-small" },
  { &CTK_CSS_VALUE_FONT_SIZE, 1, CTK_CSS_FONT_SIZE_SMALL, "small" },
  { &CTK_CSS_VALUE_FONT_SIZE, 1, CTK_CSS_FONT_SIZE_MEDIUM, "medium" },
  { &CTK_CSS_VALUE_FONT_SIZE, 1, CTK_CSS_FONT_SIZE_LARGE, "large" },
  { &CTK_CSS_VALUE_FONT_SIZE, 1, CTK_CSS_FONT_SIZE_X_LARGE, "x-large" },
  { &CTK_CSS_VALUE_FONT_SIZE, 1, CTK_CSS_FONT_SIZE_XX_LARGE, "xx-large" }
};

GtkCssValue *
_ctk_css_font_size_value_new (GtkCssFontSize font_size)
{
  g_return_val_if_fail (font_size < G_N_ELEMENTS (font_size_values), NULL);

  return _ctk_css_value_ref (&font_size_values[font_size]);
}

GtkCssValue *
_ctk_css_font_size_value_try_parse (GtkCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (font_size_values); i++)
    {
      if (_ctk_css_parser_try (parser, font_size_values[i].name, TRUE))
        return _ctk_css_value_ref (&font_size_values[i]);
    }

  return NULL;
}

GtkCssFontSize
_ctk_css_font_size_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_FONT_SIZE, CTK_CSS_FONT_SIZE_MEDIUM);

  return value->value;
}

/* PangoStyle */

static const GtkCssValueClass CTK_CSS_VALUE_FONT_STYLE = {
  ctk_css_value_enum_free,
  ctk_css_value_enum_compute,
  ctk_css_value_enum_equal,
  ctk_css_value_enum_transition,
  ctk_css_value_enum_print
};

static GtkCssValue font_style_values[] = {
  { &CTK_CSS_VALUE_FONT_STYLE, 1, PANGO_STYLE_NORMAL, "normal" },
  { &CTK_CSS_VALUE_FONT_STYLE, 1, PANGO_STYLE_OBLIQUE, "oblique" },
  { &CTK_CSS_VALUE_FONT_STYLE, 1, PANGO_STYLE_ITALIC, "italic" }
};

GtkCssValue *
_ctk_css_font_style_value_new (PangoStyle font_style)
{
  g_return_val_if_fail (font_style < G_N_ELEMENTS (font_style_values), NULL);

  return _ctk_css_value_ref (&font_style_values[font_style]);
}

GtkCssValue *
_ctk_css_font_style_value_try_parse (GtkCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (font_style_values); i++)
    {
      if (_ctk_css_parser_try (parser, font_style_values[i].name, TRUE))
        return _ctk_css_value_ref (&font_style_values[i]);
    }

  return NULL;
}

PangoStyle
_ctk_css_font_style_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_FONT_STYLE, PANGO_STYLE_NORMAL);

  return value->value;
}

/* PangoVariant */

static const GtkCssValueClass CTK_CSS_VALUE_FONT_VARIANT = {
  ctk_css_value_enum_free,
  ctk_css_value_enum_compute,
  ctk_css_value_enum_equal,
  ctk_css_value_enum_transition,
  ctk_css_value_enum_print
};

static GtkCssValue font_variant_values[] = {
  { &CTK_CSS_VALUE_FONT_VARIANT, 1, PANGO_VARIANT_NORMAL, "normal" },
  { &CTK_CSS_VALUE_FONT_VARIANT, 1, PANGO_VARIANT_SMALL_CAPS, "small-caps" }
};

GtkCssValue *
_ctk_css_font_variant_value_new (PangoVariant font_variant)
{
  g_return_val_if_fail ((gint)font_variant < G_N_ELEMENTS (font_variant_values), NULL);

  return _ctk_css_value_ref (&font_variant_values[font_variant]);
}

GtkCssValue *
_ctk_css_font_variant_value_try_parse (GtkCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (font_variant_values); i++)
    {
      if (_ctk_css_parser_try (parser, font_variant_values[i].name, TRUE))
        return _ctk_css_value_ref (&font_variant_values[i]);
    }

  return NULL;
}

PangoVariant
_ctk_css_font_variant_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_FONT_VARIANT, PANGO_VARIANT_NORMAL);

  return value->value;
}

/* PangoWeight */

#define BOLDER -1
#define LIGHTER -2

static GtkCssValue *
ctk_css_value_font_weight_compute (GtkCssValue             *value,
                                   guint                    property_id,
                                   GtkStyleProviderPrivate *provider,
                                   GtkCssStyle             *style,
                                   GtkCssStyle             *parent_style)
{
  PangoWeight new_weight;
  int parent_value;

  if (value->value >= 0)
    return _ctk_css_value_ref (value);

  if (parent_style)
    parent_value = ctk_css_style_get_value (parent_style, property_id)->value;
  else
    parent_value = 400;

  if (value->value == BOLDER)
    {
      if (parent_value < 400)
        new_weight = PANGO_WEIGHT_NORMAL;
      else if (parent_value < 600)
        new_weight = PANGO_WEIGHT_BOLD;
      else
        new_weight = PANGO_WEIGHT_HEAVY;
    }
  else if (value->value == LIGHTER)
    {
      if (parent_value > 700)
        new_weight = PANGO_WEIGHT_BOLD;
      else if (parent_value > 500)
        new_weight = PANGO_WEIGHT_NORMAL;
      else
        new_weight = PANGO_WEIGHT_THIN;
    }
  else
    {
      g_assert_not_reached ();
      new_weight = PANGO_WEIGHT_NORMAL;
    }

  return _ctk_css_font_weight_value_new (new_weight);
}

static GtkCssValue *
ctk_css_value_font_weight_transition (GtkCssValue *start,
                                      GtkCssValue *end,
                                      guint        property_id,
                                      double       progress)
{
  PangoWeight new_weight;

  if (start->value < 0 || end->value < 0)
    return NULL;

  new_weight = (start->value + end->value + 50) / 200 * 100;

  return _ctk_css_font_weight_value_new (new_weight);
}

static const GtkCssValueClass CTK_CSS_VALUE_FONT_WEIGHT = {
  ctk_css_value_enum_free,
  ctk_css_value_font_weight_compute,
  ctk_css_value_enum_equal,
  ctk_css_value_font_weight_transition,
  ctk_css_value_enum_print
};

static GtkCssValue font_weight_values[] = {
  { &CTK_CSS_VALUE_FONT_WEIGHT, 1, BOLDER, "bolder" },
  { &CTK_CSS_VALUE_FONT_WEIGHT, 1, LIGHTER, "lighter" },
  { &CTK_CSS_VALUE_FONT_WEIGHT, 1, PANGO_WEIGHT_THIN, "100" },
  { &CTK_CSS_VALUE_FONT_WEIGHT, 1, PANGO_WEIGHT_ULTRALIGHT, "200" },
  { &CTK_CSS_VALUE_FONT_WEIGHT, 1, PANGO_WEIGHT_LIGHT, "300" },
  { &CTK_CSS_VALUE_FONT_WEIGHT, 1, PANGO_WEIGHT_NORMAL, "normal" },
  { &CTK_CSS_VALUE_FONT_WEIGHT, 1, PANGO_WEIGHT_MEDIUM, "500" },
  { &CTK_CSS_VALUE_FONT_WEIGHT, 1, PANGO_WEIGHT_SEMIBOLD, "600" },
  { &CTK_CSS_VALUE_FONT_WEIGHT, 1, PANGO_WEIGHT_BOLD, "bold" },
  { &CTK_CSS_VALUE_FONT_WEIGHT, 1, PANGO_WEIGHT_ULTRABOLD, "800" },
  { &CTK_CSS_VALUE_FONT_WEIGHT, 1, PANGO_WEIGHT_HEAVY, "900" },
  { &CTK_CSS_VALUE_FONT_WEIGHT, 1, PANGO_WEIGHT_ULTRAHEAVY, "1000" }
};

GtkCssValue *
_ctk_css_font_weight_value_new (PangoWeight font_weight)
{
  guint i;
  gint w;

  w = ((font_weight + 50) / 100) * 100;

  for (i = 2; i < G_N_ELEMENTS (font_weight_values); i++)
    {
      if (font_weight_values[i].value == w)
        return _ctk_css_value_ref (&font_weight_values[i]);
    }

  g_return_val_if_reached (NULL);
}

GtkCssValue *
_ctk_css_font_weight_value_try_parse (GtkCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (font_weight_values); i++)
    {
      if (_ctk_css_parser_try (parser, font_weight_values[i].name, TRUE))
        return _ctk_css_value_ref (&font_weight_values[i]);
    }
  /* special cases go here */
  if (_ctk_css_parser_try (parser, "400", TRUE))
    return _ctk_css_value_ref (&font_weight_values[5]);
  if (_ctk_css_parser_try (parser, "700", TRUE))
    return _ctk_css_value_ref (&font_weight_values[8]);

  return NULL;
}

PangoWeight
_ctk_css_font_weight_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_FONT_WEIGHT, PANGO_WEIGHT_NORMAL);

  return value->value;
}

#undef BOLDER
#undef LIGHTER

/* PangoStretch */

static const GtkCssValueClass CTK_CSS_VALUE_FONT_STRETCH = {
  ctk_css_value_enum_free,
  ctk_css_value_enum_compute,
  ctk_css_value_enum_equal,
  ctk_css_value_enum_transition,
  ctk_css_value_enum_print
};

static GtkCssValue font_stretch_values[] = {
  { &CTK_CSS_VALUE_FONT_STRETCH, 1, PANGO_STRETCH_ULTRA_CONDENSED, "ultra-condensed" },
  { &CTK_CSS_VALUE_FONT_STRETCH, 1, PANGO_STRETCH_EXTRA_CONDENSED, "extra-condensed" },
  { &CTK_CSS_VALUE_FONT_STRETCH, 1, PANGO_STRETCH_CONDENSED, "condensed" },
  { &CTK_CSS_VALUE_FONT_STRETCH, 1, PANGO_STRETCH_SEMI_CONDENSED, "semi-condensed" },
  { &CTK_CSS_VALUE_FONT_STRETCH, 1, PANGO_STRETCH_NORMAL, "normal" },
  { &CTK_CSS_VALUE_FONT_STRETCH, 1, PANGO_STRETCH_SEMI_EXPANDED, "semi-expanded" },
  { &CTK_CSS_VALUE_FONT_STRETCH, 1, PANGO_STRETCH_EXPANDED, "expanded" },
  { &CTK_CSS_VALUE_FONT_STRETCH, 1, PANGO_STRETCH_EXTRA_EXPANDED, "extra-expanded" },
  { &CTK_CSS_VALUE_FONT_STRETCH, 1, PANGO_STRETCH_ULTRA_EXPANDED, "ultra-expanded" },
};

GtkCssValue *
_ctk_css_font_stretch_value_new (PangoStretch font_stretch)
{
  g_return_val_if_fail (font_stretch < G_N_ELEMENTS (font_stretch_values), NULL);

  return _ctk_css_value_ref (&font_stretch_values[font_stretch]);
}

GtkCssValue *
_ctk_css_font_stretch_value_try_parse (GtkCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (font_stretch_values); i++)
    {
      if (_ctk_css_parser_try (parser, font_stretch_values[i].name, TRUE))
        return _ctk_css_value_ref (&font_stretch_values[i]);
    }

  return NULL;
}

PangoStretch
_ctk_css_font_stretch_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_FONT_STRETCH, PANGO_STRETCH_NORMAL);

  return value->value;
}

/* GtkTextDecorationLine */

static const GtkCssValueClass CTK_CSS_VALUE_TEXT_DECORATION_LINE = {
  ctk_css_value_enum_free,
  ctk_css_value_enum_compute,
  ctk_css_value_enum_equal,
  ctk_css_value_enum_transition,
  ctk_css_value_enum_print
};

static GtkCssValue text_decoration_line_values[] = {
  { &CTK_CSS_VALUE_TEXT_DECORATION_LINE, 1, CTK_CSS_TEXT_DECORATION_LINE_NONE, "none" },
  { &CTK_CSS_VALUE_TEXT_DECORATION_LINE, 1, CTK_CSS_TEXT_DECORATION_LINE_UNDERLINE, "underline" },
  { &CTK_CSS_VALUE_TEXT_DECORATION_LINE, 1, CTK_CSS_TEXT_DECORATION_LINE_LINE_THROUGH, "line-through" },
};

GtkCssValue *
_ctk_css_text_decoration_line_value_new (GtkTextDecorationLine line)
{
  g_return_val_if_fail (line < G_N_ELEMENTS (text_decoration_line_values), NULL);

  return _ctk_css_value_ref (&text_decoration_line_values[line]);
}

GtkCssValue *
_ctk_css_text_decoration_line_value_try_parse (GtkCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (text_decoration_line_values); i++)
    {
      if (_ctk_css_parser_try (parser, text_decoration_line_values[i].name, TRUE))
        return _ctk_css_value_ref (&text_decoration_line_values[i]);
    }

  return NULL;
}

GtkTextDecorationLine
_ctk_css_text_decoration_line_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_TEXT_DECORATION_LINE, CTK_CSS_TEXT_DECORATION_LINE_NONE);

  return value->value;
}

/* GtkTextDecorationStyle */

static const GtkCssValueClass CTK_CSS_VALUE_TEXT_DECORATION_STYLE = {
  ctk_css_value_enum_free,
  ctk_css_value_enum_compute,
  ctk_css_value_enum_equal,
  ctk_css_value_enum_transition,
  ctk_css_value_enum_print
};

static GtkCssValue text_decoration_style_values[] = {
  { &CTK_CSS_VALUE_TEXT_DECORATION_STYLE, 1, CTK_CSS_TEXT_DECORATION_STYLE_SOLID, "solid" },
  { &CTK_CSS_VALUE_TEXT_DECORATION_STYLE, 1, CTK_CSS_TEXT_DECORATION_STYLE_DOUBLE, "double" },
  { &CTK_CSS_VALUE_TEXT_DECORATION_STYLE, 1, CTK_CSS_TEXT_DECORATION_STYLE_WAVY, "wavy" },
};

GtkCssValue *
_ctk_css_text_decoration_style_value_new (GtkTextDecorationStyle style)
{
  g_return_val_if_fail (style < G_N_ELEMENTS (text_decoration_style_values), NULL);

  return _ctk_css_value_ref (&text_decoration_style_values[style]);
}

GtkCssValue *
_ctk_css_text_decoration_style_value_try_parse (GtkCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (text_decoration_style_values); i++)
    {
      if (_ctk_css_parser_try (parser, text_decoration_style_values[i].name, TRUE))
        return _ctk_css_value_ref (&text_decoration_style_values[i]);
    }

  return NULL;
}

GtkTextDecorationStyle
_ctk_css_text_decoration_style_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_TEXT_DECORATION_STYLE, CTK_CSS_TEXT_DECORATION_STYLE_SOLID);

  return value->value;
}

/* GtkCssArea */

static const GtkCssValueClass CTK_CSS_VALUE_AREA = {
  ctk_css_value_enum_free,
  ctk_css_value_enum_compute,
  ctk_css_value_enum_equal,
  ctk_css_value_enum_transition,
  ctk_css_value_enum_print
};

static GtkCssValue area_values[] = {
  { &CTK_CSS_VALUE_AREA, 1, CTK_CSS_AREA_BORDER_BOX, "border-box" },
  { &CTK_CSS_VALUE_AREA, 1, CTK_CSS_AREA_PADDING_BOX, "padding-box" },
  { &CTK_CSS_VALUE_AREA, 1, CTK_CSS_AREA_CONTENT_BOX, "content-box" }
};

GtkCssValue *
_ctk_css_area_value_new (GtkCssArea area)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (area_values); i++)
    {
      if (area_values[i].value == area)
        return _ctk_css_value_ref (&area_values[i]);
    }

  g_return_val_if_reached (NULL);
}

GtkCssValue *
_ctk_css_area_value_try_parse (GtkCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (area_values); i++)
    {
      if (_ctk_css_parser_try (parser, area_values[i].name, TRUE))
        return _ctk_css_value_ref (&area_values[i]);
    }

  return NULL;
}

GtkCssArea
_ctk_css_area_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_AREA, CTK_CSS_AREA_BORDER_BOX);

  return value->value;
}

/* GtkCssDirection */

static const GtkCssValueClass CTK_CSS_VALUE_DIRECTION = {
  ctk_css_value_enum_free,
  ctk_css_value_enum_compute,
  ctk_css_value_enum_equal,
  ctk_css_value_enum_transition,
  ctk_css_value_enum_print
};

static GtkCssValue direction_values[] = {
  { &CTK_CSS_VALUE_DIRECTION, 1, CTK_CSS_DIRECTION_NORMAL, "normal" },
  { &CTK_CSS_VALUE_DIRECTION, 1, CTK_CSS_DIRECTION_REVERSE, "reverse" },
  { &CTK_CSS_VALUE_DIRECTION, 1, CTK_CSS_DIRECTION_ALTERNATE, "alternate" },
  { &CTK_CSS_VALUE_DIRECTION, 1, CTK_CSS_DIRECTION_ALTERNATE_REVERSE, "alternate-reverse" }
};

GtkCssValue *
_ctk_css_direction_value_new (GtkCssDirection direction)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (direction_values); i++)
    {
      if (direction_values[i].value == direction)
        return _ctk_css_value_ref (&direction_values[i]);
    }

  g_return_val_if_reached (NULL);
}

GtkCssValue *
_ctk_css_direction_value_try_parse (GtkCssParser *parser)
{
  int i;

  g_return_val_if_fail (parser != NULL, NULL);

  /* need to parse backwards here, otherwise "alternate" will also match "alternate-reverse".
   * Our parser rocks!
   */
  for (i = G_N_ELEMENTS (direction_values) - 1; i >= 0; i--)
    {
      if (_ctk_css_parser_try (parser, direction_values[i].name, TRUE))
        return _ctk_css_value_ref (&direction_values[i]);
    }

  return NULL;
}

GtkCssDirection
_ctk_css_direction_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_DIRECTION, CTK_CSS_DIRECTION_NORMAL);

  return value->value;
}

/* GtkCssPlayState */

static const GtkCssValueClass CTK_CSS_VALUE_PLAY_STATE = {
  ctk_css_value_enum_free,
  ctk_css_value_enum_compute,
  ctk_css_value_enum_equal,
  ctk_css_value_enum_transition,
  ctk_css_value_enum_print
};

static GtkCssValue play_state_values[] = {
  { &CTK_CSS_VALUE_PLAY_STATE, 1, CTK_CSS_PLAY_STATE_RUNNING, "running" },
  { &CTK_CSS_VALUE_PLAY_STATE, 1, CTK_CSS_PLAY_STATE_PAUSED, "paused" }
};

GtkCssValue *
_ctk_css_play_state_value_new (GtkCssPlayState play_state)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (play_state_values); i++)
    {
      if (play_state_values[i].value == play_state)
        return _ctk_css_value_ref (&play_state_values[i]);
    }

  g_return_val_if_reached (NULL);
}

GtkCssValue *
_ctk_css_play_state_value_try_parse (GtkCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (play_state_values); i++)
    {
      if (_ctk_css_parser_try (parser, play_state_values[i].name, TRUE))
        return _ctk_css_value_ref (&play_state_values[i]);
    }

  return NULL;
}

GtkCssPlayState
_ctk_css_play_state_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_PLAY_STATE, CTK_CSS_PLAY_STATE_RUNNING);

  return value->value;
}

/* GtkCssFillMode */

static const GtkCssValueClass CTK_CSS_VALUE_FILL_MODE = {
  ctk_css_value_enum_free,
  ctk_css_value_enum_compute,
  ctk_css_value_enum_equal,
  ctk_css_value_enum_transition,
  ctk_css_value_enum_print
};

static GtkCssValue fill_mode_values[] = {
  { &CTK_CSS_VALUE_FILL_MODE, 1, CTK_CSS_FILL_NONE, "none" },
  { &CTK_CSS_VALUE_FILL_MODE, 1, CTK_CSS_FILL_FORWARDS, "forwards" },
  { &CTK_CSS_VALUE_FILL_MODE, 1, CTK_CSS_FILL_BACKWARDS, "backwards" },
  { &CTK_CSS_VALUE_FILL_MODE, 1, CTK_CSS_FILL_BOTH, "both" }
};

GtkCssValue *
_ctk_css_fill_mode_value_new (GtkCssFillMode fill_mode)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (fill_mode_values); i++)
    {
      if (fill_mode_values[i].value == fill_mode)
        return _ctk_css_value_ref (&fill_mode_values[i]);
    }

  g_return_val_if_reached (NULL);
}

GtkCssValue *
_ctk_css_fill_mode_value_try_parse (GtkCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (fill_mode_values); i++)
    {
      if (_ctk_css_parser_try (parser, fill_mode_values[i].name, TRUE))
        return _ctk_css_value_ref (&fill_mode_values[i]);
    }

  return NULL;
}

GtkCssFillMode
_ctk_css_fill_mode_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_FILL_MODE, CTK_CSS_FILL_NONE);

  return value->value;
}

/* GtkCssIconEffect */

static const GtkCssValueClass CTK_CSS_VALUE_ICON_EFFECT = {
  ctk_css_value_enum_free,
  ctk_css_value_enum_compute,
  ctk_css_value_enum_equal,
  ctk_css_value_enum_transition,
  ctk_css_value_enum_print
};

static GtkCssValue image_effect_values[] = {
  { &CTK_CSS_VALUE_ICON_EFFECT, 1, CTK_CSS_ICON_EFFECT_NONE, "none" },
  { &CTK_CSS_VALUE_ICON_EFFECT, 1, CTK_CSS_ICON_EFFECT_HIGHLIGHT, "highlight" },
  { &CTK_CSS_VALUE_ICON_EFFECT, 1, CTK_CSS_ICON_EFFECT_DIM, "dim" }
};

GtkCssValue *
_ctk_css_icon_effect_value_new (GtkCssIconEffect image_effect)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (image_effect_values); i++)
    {
      if (image_effect_values[i].value == image_effect)
        return _ctk_css_value_ref (&image_effect_values[i]);
    }

  g_return_val_if_reached (NULL);
}

GtkCssValue *
_ctk_css_icon_effect_value_try_parse (GtkCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (image_effect_values); i++)
    {
      if (_ctk_css_parser_try (parser, image_effect_values[i].name, TRUE))
        return _ctk_css_value_ref (&image_effect_values[i]);
    }

  return NULL;
}

GtkCssIconEffect
_ctk_css_icon_effect_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_ICON_EFFECT, CTK_CSS_ICON_EFFECT_NONE);

  return value->value;
}

void
ctk_css_icon_effect_apply (GtkCssIconEffect  icon_effect,
                           cairo_surface_t  *surface)
{
  cairo_t *cr;

  switch (icon_effect)
    {
    case CTK_CSS_ICON_EFFECT_DIM:
      cr = cairo_create (surface);
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_rgba (cr, 0, 0, 0, 0); /* transparent */
      cairo_paint_with_alpha (cr, 0.5);
      cairo_destroy (cr);
      break;

    case CTK_CSS_ICON_EFFECT_HIGHLIGHT:
      cr = cairo_create (surface);
      cairo_set_source_rgb (cr, 0.1, 0.1, 0.1);
      cairo_set_operator (cr, CAIRO_OPERATOR_COLOR_DODGE);
      /* DANGER: We mask with ourself - that works for images, but... */
      cairo_mask_surface (cr, surface, 0, 0);
      cairo_destroy (cr);
      break;

    default:
      g_warn_if_reached ();
      /* fall through */
    case CTK_CSS_ICON_EFFECT_NONE:
      break;
    }
}

/* GtkCssIconStyle */

static const GtkCssValueClass CTK_CSS_VALUE_ICON_STYLE = {
  ctk_css_value_enum_free,
  ctk_css_value_enum_compute,
  ctk_css_value_enum_equal,
  ctk_css_value_enum_transition,
  ctk_css_value_enum_print
};

static GtkCssValue icon_style_values[] = {
  { &CTK_CSS_VALUE_ICON_STYLE, 1, CTK_CSS_ICON_STYLE_REQUESTED, "requested" },
  { &CTK_CSS_VALUE_ICON_STYLE, 1, CTK_CSS_ICON_STYLE_REGULAR, "regular" },
  { &CTK_CSS_VALUE_ICON_STYLE, 1, CTK_CSS_ICON_STYLE_SYMBOLIC, "symbolic" }
};

GtkCssValue *
_ctk_css_icon_style_value_new (GtkCssIconStyle icon_style)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (icon_style_values); i++)
    {
      if (icon_style_values[i].value == icon_style)
        return _ctk_css_value_ref (&icon_style_values[i]);
    }

  g_return_val_if_reached (NULL);
}

GtkCssValue *
_ctk_css_icon_style_value_try_parse (GtkCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (icon_style_values); i++)
    {
      if (_ctk_css_parser_try (parser, icon_style_values[i].name, TRUE))
        return _ctk_css_value_ref (&icon_style_values[i]);
    }

  return NULL;
}

GtkCssIconStyle
_ctk_css_icon_style_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_ICON_STYLE, CTK_CSS_ICON_STYLE_REQUESTED);

  return value->value;
}
