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

#include "ctkprivate.h"
#include "ctkcssstyleprivate.h"

#include "ctkcssanimationprivate.h"
#include "ctkcssarrayvalueprivate.h"
#include "ctkcssenumvalueprivate.h"
#include "ctkcssinheritvalueprivate.h"
#include "ctkcssinitialvalueprivate.h"
#include "ctkcssnumbervalueprivate.h"
#include "ctkcssrgbavalueprivate.h"
#include "ctkcsssectionprivate.h"
#include "ctkcssshorthandpropertyprivate.h"
#include "ctkcssstringvalueprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkcsstransitionprivate.h"
#include "ctkstyleanimationprivate.h"
#include "ctkstylepropertyprivate.h"
#include "ctkstyleproviderprivate.h"

G_DEFINE_ABSTRACT_TYPE (CtkCssStyle, ctk_css_style, G_TYPE_OBJECT)

static CtkCssSection *
ctk_css_style_real_get_section (CtkCssStyle *style,
                                guint        id)
{
  return NULL;
}

static gboolean
ctk_css_style_real_is_static (CtkCssStyle *style)
{
  return TRUE;
}

static void
ctk_css_style_class_init (CtkCssStyleClass *klass)
{
  klass->get_section = ctk_css_style_real_get_section;
  klass->is_static = ctk_css_style_real_is_static;
}

static void
ctk_css_style_init (CtkCssStyle *style)
{
}

CtkCssValue *
ctk_css_style_get_value (CtkCssStyle *style,
                          guint        id)
{
  ctk_internal_return_val_if_fail (CTK_IS_CSS_STYLE (style), NULL);

  return CTK_CSS_STYLE_GET_CLASS (style)->get_value (style, id);
}

CtkCssSection *
ctk_css_style_get_section (CtkCssStyle *style,
                           guint        id)
{
  ctk_internal_return_val_if_fail (CTK_IS_CSS_STYLE (style), NULL);

  return CTK_CSS_STYLE_GET_CLASS (style)->get_section (style, id);
}

CtkBitmask *
ctk_css_style_add_difference (CtkBitmask  *accumulated,
                              CtkCssStyle *style,
                              CtkCssStyle *other)
{
  gint len, i;

  if (style == other)
    return accumulated;

  len = _ctk_css_style_property_get_n_properties ();
  for (i = 0; i < len; i++)
    {
      if (_ctk_bitmask_get (accumulated, i))
        continue;

      if (!_ctk_css_value_equal (ctk_css_style_get_value (style, i),
                                 ctk_css_style_get_value (other, i)))
        accumulated = _ctk_bitmask_set (accumulated, i, TRUE);
    }

  return accumulated;
}

gboolean
ctk_css_style_is_static (CtkCssStyle *style)
{
  ctk_internal_return_val_if_fail (CTK_IS_CSS_STYLE (style), TRUE);

  return CTK_CSS_STYLE_GET_CLASS (style)->is_static (style);
}

/*
 * ctk_css_style_print:
 * @style: a #CtkCssStyle
 * @string: the #GString to print to
 * @indent: level of indentation to use
 * @skip_initial: %TRUE to skip properties that have their initial value
 *
 * Print the @style to @string, in CSS format. Every property is printed
 * on a line by itself, indented by @indent spaces. If @skip_initial is
 * %TRUE, properties are only printed if their value in @style is different
 * from the initial value of the property.
 *
 * Returns: %TRUE is any properties have been printed
 */
gboolean
ctk_css_style_print (CtkCssStyle *style,
                     GString     *string,
                     guint        indent,
                     gboolean     skip_initial)
{
  guint i;
  gboolean retval = FALSE;

  g_return_val_if_fail (CTK_IS_CSS_STYLE (style), FALSE);
  g_return_val_if_fail (string != NULL, FALSE);

  for (i = 0; i < _ctk_css_style_property_get_n_properties (); i++)
    {
      CtkCssSection *section;
      CtkCssStyleProperty *prop;
      CtkCssValue *value;
      const char *name;

      section = ctk_css_style_get_section (style, i);
      if (!section && skip_initial)
        continue;

      prop = _ctk_css_style_property_lookup_by_id (i);
      name = _ctk_style_property_get_name (CTK_STYLE_PROPERTY (prop));
      value = ctk_css_style_get_value (style, i);

      g_string_append_printf (string, "%*s%s: ", indent, "", name);
      _ctk_css_value_print (value, string);
      g_string_append_c (string, ';');

      if (section)
        {
          g_string_append (string, " /* ");
          _ctk_css_section_print (section, string);
          g_string_append (string, " */");
        }

      g_string_append_c (string, '\n');

      retval = TRUE;
    }

  return retval;
}

char *
ctk_css_style_to_string (CtkCssStyle *style)
{
  GString *string;

  g_return_val_if_fail (CTK_IS_CSS_STYLE (style), NULL);

  string = g_string_new ("");

  ctk_css_style_print (style, string, 0, FALSE);

  return g_string_free (string, FALSE);
}

static PangoUnderline
get_pango_underline_from_style (CtkTextDecorationStyle style)
{
  switch (style)
    {
    case CTK_CSS_TEXT_DECORATION_STYLE_DOUBLE:
      return PANGO_UNDERLINE_DOUBLE;
    case CTK_CSS_TEXT_DECORATION_STYLE_WAVY:
      return PANGO_UNDERLINE_ERROR;
    case CTK_CSS_TEXT_DECORATION_STYLE_SOLID:
    default:
      return PANGO_UNDERLINE_SINGLE;
    }

  g_return_val_if_reached (PANGO_UNDERLINE_SINGLE);
}

static PangoAttrList *
add_pango_attr (PangoAttrList  *attrs,
                PangoAttribute *attr)
{
  if (attrs == NULL)
    attrs = pango_attr_list_new ();

  pango_attr_list_insert (attrs, attr);

  return attrs;
}

PangoAttrList *
ctk_css_style_get_pango_attributes (CtkCssStyle *style)
{
  PangoAttrList *attrs = NULL;
  CtkTextDecorationLine decoration_line;
  CtkTextDecorationStyle decoration_style;
  const CdkRGBA *color;
  const CdkRGBA *decoration_color;
  gint letter_spacing;
  const char *font_feature_settings;

  /* text-decoration */
  decoration_line = _ctk_css_text_decoration_line_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_TEXT_DECORATION_LINE));
  decoration_style = _ctk_css_text_decoration_style_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_TEXT_DECORATION_STYLE));
  color = _ctk_css_rgba_value_get_rgba (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_COLOR));
  decoration_color = _ctk_css_rgba_value_get_rgba (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_TEXT_DECORATION_COLOR));

  switch (decoration_line)
    {
    case CTK_CSS_TEXT_DECORATION_LINE_UNDERLINE:
      attrs = add_pango_attr (attrs, pango_attr_underline_new (get_pango_underline_from_style (decoration_style)));
      if (!cdk_rgba_equal (color, decoration_color))
        attrs = add_pango_attr (attrs, pango_attr_underline_color_new (decoration_color->red * 65535. + 0.5,
                                                                       decoration_color->green * 65535. + 0.5,
                                                                       decoration_color->blue * 65535. + 0.5));
      break;
    case CTK_CSS_TEXT_DECORATION_LINE_LINE_THROUGH:
      attrs = add_pango_attr (attrs, pango_attr_strikethrough_new (TRUE));
      if (!cdk_rgba_equal (color, decoration_color))
        attrs = add_pango_attr (attrs, pango_attr_strikethrough_color_new (decoration_color->red * 65535. + 0.5,
                                                                           decoration_color->green * 65535. + 0.5,
                                                                           decoration_color->blue * 65535. + 0.5));
      break;
    case CTK_CSS_TEXT_DECORATION_LINE_NONE:
    default:
      break;
    }

  /* letter-spacing */
  letter_spacing = _ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_LETTER_SPACING), 100);
  if (letter_spacing != 0)
    {
      attrs = add_pango_attr (attrs, pango_attr_letter_spacing_new (letter_spacing * PANGO_SCALE));
    }

  /* font-feature-settings */
  font_feature_settings = _ctk_css_string_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_FONT_FEATURE_SETTINGS));
  if (font_feature_settings != NULL)
    {
      attrs = add_pango_attr (attrs, pango_attr_font_features_new (font_feature_settings));
    }

  return attrs;
}

static CtkCssValue *
query_func (guint    id,
            gpointer values)
{
  return ctk_css_style_get_value (values, id);
}

PangoFontDescription *
ctk_css_style_get_pango_font (CtkCssStyle *style)
{
  CtkStyleProperty *prop;
  GValue value = { 0, };

  prop = _ctk_style_property_lookup ("font");
  _ctk_style_property_query (prop, &value, query_func, style);

  return (PangoFontDescription *)g_value_get_boxed (&value);
}
