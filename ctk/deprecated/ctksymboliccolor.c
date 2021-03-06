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

#include "ctkcsscolorvalueprivate.h"
#include "ctkcssrgbavalueprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkhslaprivate.h"
#include "ctkstylepropertyprivate.h"
#include "ctksymboliccolorprivate.h"
#include "ctkstyleproperties.h"
#include "ctkintl.h"
#include "ctkwin32themeprivate.h"

/**
 * SECTION:ctksymboliccolor
 * @Short_description: Symbolic colors
 * @Title: CtkSymbolicColor
 *
 * CtkSymbolicColor is a boxed type that represents a symbolic color.
 * It is the result of parsing a
 * [color expression][ctkcssprovider-symbolic-colors].
 * To obtain the color represented by a CtkSymbolicColor, it has to
 * be resolved with ctk_symbolic_color_resolve(), which replaces all
 * symbolic color references by the colors they refer to (in a given
 * context) and evaluates mix, shade and other expressions, resulting
 * in a #CdkRGBA value.
 *
 * It is not normally necessary to deal directly with #CtkSymbolicColors,
 * since they are mostly used behind the scenes by #CtkStyleContext and
 * #CtkCssProvider.
 *
 * #CtkSymbolicColor is deprecated. Symbolic colors are considered an
 * implementation detail of CTK+.
 */

G_DEFINE_BOXED_TYPE (CtkSymbolicColor, ctk_symbolic_color,
                     ctk_symbolic_color_ref, ctk_symbolic_color_unref)

struct _CtkSymbolicColor
{
  CtkCssValue *value;
  gint ref_count;
};

static CtkSymbolicColor *
ctk_symbolic_color_new (CtkCssValue *value)
{
  CtkSymbolicColor *symbolic;

  symbolic = g_slice_new0 (CtkSymbolicColor);
  symbolic->value = value;
  symbolic->ref_count = 1;

  return symbolic;
}

/**
 * ctk_symbolic_color_new_literal:
 * @color: a #CdkRGBA
 *
 * Creates a symbolic color pointing to a literal color.
 *
 * Returns: a newly created #CtkSymbolicColor
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: #CtkSymbolicColor is deprecated.
 **/
CtkSymbolicColor *
ctk_symbolic_color_new_literal (const CdkRGBA *color)
{
  g_return_val_if_fail (color != NULL, NULL);

  return ctk_symbolic_color_new (_ctk_css_color_value_new_literal (color));
}

/**
 * ctk_symbolic_color_new_name:
 * @name: color name
 *
 * Creates a symbolic color pointing to an unresolved named
 * color. See ctk_style_context_lookup_color() and
 * ctk_style_properties_lookup_color().
 *
 * Returns: a newly created #CtkSymbolicColor
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: #CtkSymbolicColor is deprecated.
 **/
CtkSymbolicColor *
ctk_symbolic_color_new_name (const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);

  return ctk_symbolic_color_new (_ctk_css_color_value_new_name (name));
}

/**
 * ctk_symbolic_color_new_shade: (constructor)
 * @color: another #CtkSymbolicColor
 * @factor: shading factor to apply to @color
 *
 * Creates a symbolic color defined as a shade of
 * another color. A factor > 1.0 would resolve to
 * a brighter color, while < 1.0 would resolve to
 * a darker color.
 *
 * Returns: A newly created #CtkSymbolicColor
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: #CtkSymbolicColor is deprecated.
 **/
CtkSymbolicColor *
ctk_symbolic_color_new_shade (CtkSymbolicColor *color,
                              gdouble           factor)
{
  g_return_val_if_fail (color != NULL, NULL);

  return ctk_symbolic_color_new (_ctk_css_color_value_new_shade (color->value,
                                                                 factor));
}

/**
 * ctk_symbolic_color_new_alpha: (constructor)
 * @color: another #CtkSymbolicColor
 * @factor: factor to apply to @color alpha
 *
 * Creates a symbolic color by modifying the relative alpha
 * value of @color. A factor < 1.0 would resolve to a more
 * transparent color, while > 1.0 would resolve to a more
 * opaque color.
 *
 * Returns: A newly created #CtkSymbolicColor
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: #CtkSymbolicColor is deprecated.
 **/
CtkSymbolicColor *
ctk_symbolic_color_new_alpha (CtkSymbolicColor *color,
                              gdouble           factor)
{
  g_return_val_if_fail (color != NULL, NULL);

  return ctk_symbolic_color_new (_ctk_css_color_value_new_alpha (color->value,
                                                                 factor));
}

/**
 * ctk_symbolic_color_new_mix: (constructor)
 * @color1: color to mix
 * @color2: another color to mix
 * @factor: mix factor
 *
 * Creates a symbolic color defined as a mix of another
 * two colors. a mix factor of 0 would resolve to @color1,
 * while a factor of 1 would resolve to @color2.
 *
 * Returns: A newly created #CtkSymbolicColor
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: #CtkSymbolicColor is deprecated.
 **/
CtkSymbolicColor *
ctk_symbolic_color_new_mix (CtkSymbolicColor *color1,
                            CtkSymbolicColor *color2,
                            gdouble           factor)
{
  g_return_val_if_fail (color1 != NULL, NULL);
  g_return_val_if_fail (color1 != NULL, NULL);

  return ctk_symbolic_color_new (_ctk_css_color_value_new_mix (color1->value,
                                                               color2->value,
                                                               factor));
}

/**
 * ctk_symbolic_color_new_win32: (constructor)
 * @theme_class: The theme class to pull color from
 * @id: The color id
 *
 * Creates a symbolic color based on the current win32
 * theme.
 *
 * Note that while this call is available on all platforms
 * the actual value returned is not reliable on non-win32
 * platforms.
 *
 * Returns: A newly created #CtkSymbolicColor
 *
 * Since: 3.4
 *
 * Deprecated: 3.8: #CtkSymbolicColor is deprecated.
 */
CtkSymbolicColor *
ctk_symbolic_color_new_win32 (const gchar *theme_class,
                              gint         id)
{
  g_return_val_if_fail (theme_class != NULL, NULL);

  return ctk_symbolic_color_new (_ctk_css_color_value_new_win32 (theme_class, id));
}

/**
 * ctk_symbolic_color_ref:
 * @color: a #CtkSymbolicColor
 *
 * Increases the reference count of @color
 *
 * Returns: the same @color
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: #CtkSymbolicColor is deprecated.
 **/
CtkSymbolicColor *
ctk_symbolic_color_ref (CtkSymbolicColor *color)
{
  g_return_val_if_fail (color != NULL, NULL);

  color->ref_count++;

  return color;
}

/**
 * ctk_symbolic_color_unref:
 * @color: a #CtkSymbolicColor
 *
 * Decreases the reference count of @color, freeing its memory if the
 * reference count reaches 0.
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: #CtkSymbolicColor is deprecated.
 **/
void
ctk_symbolic_color_unref (CtkSymbolicColor *color)
{
  g_return_if_fail (color != NULL);

  if (--color->ref_count)
    return;

  _ctk_css_value_unref (color->value);

  g_slice_free (CtkSymbolicColor, color);
}

/**
 * ctk_symbolic_color_resolve:
 * @color: a #CtkSymbolicColor
 * @props: (allow-none): #CtkStyleProperties to use when resolving
 *    named colors, or %NULL
 * @resolved_color: (out): return location for the resolved color
 *
 * If @color is resolvable, @resolved_color will be filled in
 * with the resolved color, and %TRUE will be returned. Generally,
 * if @color can’t be resolved, it is due to it being defined on
 * top of a named color that doesn’t exist in @props.
 *
 * When @props is %NULL, resolving of named colors will fail, so if
 * your @color is or references such a color, this function will
 * return %FALSE.
 *
 * Returns: %TRUE if the color has been resolved
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: #CtkSymbolicColor is deprecated.
 **/
gboolean
ctk_symbolic_color_resolve (CtkSymbolicColor   *color,
			    CtkStyleProperties *props,
			    CdkRGBA            *resolved_color)
{
  CdkRGBA pink = { 1.0, 0.5, 0.5, 1.0 };
  CtkCssValue *v, *current;

  g_return_val_if_fail (color != NULL, FALSE);
  g_return_val_if_fail (resolved_color != NULL, FALSE);
  g_return_val_if_fail (props == NULL || CTK_IS_STYLE_PROPERTIES (props), FALSE);

  current = _ctk_css_rgba_value_new_from_rgba (&pink);
  v = _ctk_css_color_value_resolve (color->value,
                                    CTK_STYLE_PROVIDER_PRIVATE (props),
                                    current,
                                    NULL);
  _ctk_css_value_unref (current);
  if (v == NULL)
    return FALSE;

  *resolved_color = *_ctk_css_rgba_value_get_rgba (v);
  _ctk_css_value_unref (v);
  return TRUE;
}

/**
 * ctk_symbolic_color_to_string:
 * @color: color to convert to a string
 *
 * Converts the given @color to a string representation. This is useful
 * both for debugging and for serialization of strings. The format of
 * the string may change between different versions of CTK, but it is
 * guaranteed that the CTK css parser is able to read the string and
 * create the same symbolic color from it.
 *
 * Returns: a new string representing @color
 *
 * Deprecated: 3.8: #CtkSymbolicColor is deprecated.
 **/
char *
ctk_symbolic_color_to_string (CtkSymbolicColor *color)
{
  g_return_val_if_fail (color != NULL, NULL);

  return _ctk_css_value_to_string (color->value);
}

CtkSymbolicColor *
_ctk_css_symbolic_value_new (CtkCssParser *parser)
{
  CtkCssValue *value;

  value = _ctk_css_color_value_parse (parser);
  if (value == NULL)
    return NULL;

  return ctk_symbolic_color_new (value);
}

CtkCssValue *
_ctk_symbolic_color_get_css_value (CtkSymbolicColor *symbolic)
{
  return symbolic->value;
}

