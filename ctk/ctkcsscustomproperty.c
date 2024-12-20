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

#include "ctkcsscustompropertyprivate.h"

#include <string.h>

#include "ctkcssstylefuncsprivate.h"
#include "ctkcsstypedvalueprivate.h"
#include "ctkstylepropertiesprivate.h"

#include "deprecated/ctkthemingengine.h"

#include "deprecated/ctksymboliccolor.h"

G_DEFINE_TYPE (CtkCssCustomProperty, _ctk_css_custom_property, CTK_TYPE_CSS_STYLE_PROPERTY)

static CtkCssValue *
ctk_css_custom_property_parse_value (CtkStyleProperty *property G_GNUC_UNUSED,
                                     CtkCssParser     *parser)
{
  _ctk_css_parser_error_full (parser,
                              CTK_CSS_PROVIDER_ERROR_NAME,
                              "Custom CSS properties are no longer supported.");
  return NULL;
}

static void
ctk_css_custom_property_query (CtkStyleProperty   *property,
                               GValue             *value,
                               CtkStyleQueryFunc   query_func,
                               gpointer            query_data)
{
  CtkCssStyleProperty *style = CTK_CSS_STYLE_PROPERTY (property);
  CtkCssCustomProperty *custom = CTK_CSS_CUSTOM_PROPERTY (property);
  CtkCssValue *css_value;
  
  css_value = (* query_func) (_ctk_css_style_property_get_id (style), query_data);
  if (css_value == NULL)
    css_value = _ctk_css_style_property_get_initial_value (style);

  g_value_init (value, custom->pspec->value_type);
  g_value_copy (_ctk_css_typed_value_get (css_value), value);
}

static void
ctk_css_custom_property_assign (CtkStyleProperty   *property,
                                CtkStyleProperties *props,
                                CtkStateFlags       state,
                                const GValue       *value)
{
  CtkCssValue *css_value = _ctk_css_typed_value_new (value);
  _ctk_style_properties_set_property_by_property (props,
                                                  CTK_CSS_STYLE_PROPERTY (property),
                                                  state,
                                                  css_value);
  _ctk_css_value_unref (css_value);
}

static void
_ctk_css_custom_property_class_init (CtkCssCustomPropertyClass *klass)
{
  CtkStylePropertyClass *property_class = CTK_STYLE_PROPERTY_CLASS (klass);

  property_class->parse_value = ctk_css_custom_property_parse_value;
  property_class->query = ctk_css_custom_property_query;
  property_class->assign = ctk_css_custom_property_assign;
}

static void
_ctk_css_custom_property_init (CtkCssCustomProperty *custom G_GNUC_UNUSED)
{
}

static CtkCssValue *
ctk_css_custom_property_create_initial_value (GParamSpec *pspec)
{
  GValue value = G_VALUE_INIT;
  CtkCssValue *result;

  g_value_init (&value, pspec->value_type);


G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (pspec->value_type == CTK_TYPE_THEMING_ENGINE)
    g_value_set_object (&value, ctk_theming_engine_load (NULL));
  else if (pspec->value_type == PANGO_TYPE_FONT_DESCRIPTION)
    g_value_take_boxed (&value, pango_font_description_from_string ("Sans 10"));
  else if (pspec->value_type == CDK_TYPE_RGBA)
    {
      CdkRGBA color;
      cdk_rgba_parse (&color, "pink");
      g_value_set_boxed (&value, &color);
    }
  else if (pspec->value_type == g_type_from_name ("CdkColor"))
    {
      CdkColor color;
      cdk_color_parse ("pink", &color);
      g_value_set_boxed (&value, &color);
    }
  else if (pspec->value_type == CTK_TYPE_BORDER)
    {
      g_value_take_boxed (&value, ctk_border_new ());
    }
  else
    g_param_value_set_default (pspec, &value);
G_GNUC_END_IGNORE_DEPRECATIONS

  result = _ctk_css_typed_value_new (&value);
  g_value_unset (&value);

  return result;
}

/* Property registration functions */

/**
 * ctk_theming_engine_register_property: (skip)
 * @name_space: namespace for the property name
 * @parse_func: (nullable): parsing function to use, or %NULL
 * @pspec: the #GParamSpec for the new property
 *
 * Registers a property so it can be used in the CSS file format,
 * on the CSS file the property will look like
 * "-${@name_space}-${property_name}". being
 * ${property_name} the given to @pspec. @name_space will usually
 * be the theme engine name.
 *
 * For any type a @parse_func may be provided, being this function
 * used for turning any property value (between “:” and “;”) in
 * CSS to the #GValue needed. For basic types there is already
 * builtin parsing support, so %NULL may be provided for these
 * cases.
 *
 * Engines must ensure property registration happens exactly once,
 * usually CTK+ deals with theming engines as singletons, so this
 * should be guaranteed to happen once, but bear this in mind
 * when creating #CtkThemeEngines yourself.
 *
 * In order to make use of the custom registered properties in
 * the CSS file, make sure the engine is loaded first by specifying
 * the engine property, either in a previous rule or within the same
 * one.
 * |[
 * * {
 *     engine: someengine;
 *     -SomeEngine-custom-property: 2;
 * }
 * ]|
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: Code should use the default properties provided by CSS.
 **/
void
ctk_theming_engine_register_property (const gchar            *name_space,
                                      CtkStylePropertyParser  parse_func,
                                      GParamSpec             *pspec)
{
  CtkCssCustomProperty *node;
  CtkCssValue *initial;
  gchar *name;

  g_return_if_fail (name_space != NULL);
  g_return_if_fail (strchr (name_space, ' ') == NULL);
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  name = g_strdup_printf ("-%s-%s", name_space, pspec->name);

  /* This also initializes the default properties */
  if (_ctk_style_property_lookup (pspec->name))
    {
      g_warning ("a property with name '%s' already exists", name);
      g_free (name);
      return;
    }
  
  initial = ctk_css_custom_property_create_initial_value (pspec);

  node = g_object_new (CTK_TYPE_CSS_CUSTOM_PROPERTY,
                       "initial-value", initial,
                       "name", name,
                       "value-type", pspec->value_type,
                       NULL);
  node->pspec = pspec;
  node->property_parse_func = parse_func;

  _ctk_css_value_unref (initial);
  g_free (name);
}

/**
 * ctk_style_properties_register_property: (skip)
 * @parse_func: (nullable): parsing function to use, or %NULL
 * @pspec: the #GParamSpec for the new property
 *
 * Registers a property so it can be used in the CSS file format.
 * This function is the low-level equivalent of
 * ctk_theming_engine_register_property(), if you are implementing
 * a theming engine, you want to use that function instead.
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: Code should use the default properties provided by CSS.
 **/
void
ctk_style_properties_register_property (CtkStylePropertyParser  parse_func,
                                        GParamSpec             *pspec)
{
  CtkCssCustomProperty *node;
  CtkCssValue *initial;

  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  /* This also initializes the default properties */
  if (_ctk_style_property_lookup (pspec->name))
    {
      g_warning ("a property with name '%s' already exists", pspec->name);
      return;
    }
  
  initial = ctk_css_custom_property_create_initial_value (pspec);

  node = g_object_new (CTK_TYPE_CSS_CUSTOM_PROPERTY,
                       "initial-value", initial,
                       "name", pspec->name,
                       "value-type", pspec->value_type,
                       NULL);
  node->pspec = pspec;
  node->property_parse_func = parse_func;

  _ctk_css_value_unref (initial);
}

/**
 * ctk_style_properties_lookup_property: (skip)
 * @property_name: property name to look up
 * @parse_func: (out): return location for the parse function
 * @pspec: (out) (transfer none): return location for the #GParamSpec
 *
 * Returns %TRUE if a property has been registered, if @pspec or
 * @parse_func are not %NULL, the #GParamSpec and parsing function
 * will be respectively returned.
 *
 * Returns: %TRUE if the property is registered, %FALSE otherwise
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: This code could only look up custom properties and
 *     those are deprecated.
 **/
gboolean
ctk_style_properties_lookup_property (const gchar             *property_name,
                                      CtkStylePropertyParser  *parse_func,
                                      GParamSpec             **pspec)
{
  CtkStyleProperty *node;
  gboolean found = FALSE;

  g_return_val_if_fail (property_name != NULL, FALSE);

  node = _ctk_style_property_lookup (property_name);

  if (CTK_IS_CSS_CUSTOM_PROPERTY (node))
    {
      CtkCssCustomProperty *custom = CTK_CSS_CUSTOM_PROPERTY (node);

      if (pspec)
        *pspec = custom->pspec;

      if (parse_func)
        *parse_func = custom->property_parse_func;

      found = TRUE;
    }

  return found;
}

