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

#include "ctkstylepropertyprivate.h"

#include "ctkcssprovider.h"
#include "ctkcssparserprivate.h"
#include "ctkcssshorthandpropertyprivate.h"
#include "ctkcssstylefuncsprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkcsstypesprivate.h"
#include "ctkintl.h"
#include "ctkprivatetypebuiltins.h"

enum {
  PROP_0,
  PROP_NAME,
  PROP_VALUE_TYPE
};

G_DEFINE_ABSTRACT_TYPE (CtkStyleProperty, _ctk_style_property, G_TYPE_OBJECT)

static void
ctk_style_property_finalize (GObject *object)
{
  CtkStyleProperty *property = CTK_STYLE_PROPERTY (object);

  g_warning ("finalizing %s '%s', how could this happen?", G_OBJECT_TYPE_NAME (object), property->name);

  G_OBJECT_CLASS (_ctk_style_property_parent_class)->finalize (object);
}

static void
ctk_style_property_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  CtkStyleProperty *property = CTK_STYLE_PROPERTY (object);
  CtkStylePropertyClass *klass = CTK_STYLE_PROPERTY_GET_CLASS (property);

  switch (prop_id)
    {
    case PROP_NAME:
      property->name = g_value_dup_string (value);
      g_assert (property->name);
      g_assert (g_hash_table_lookup (klass->properties, property->name) == NULL);
      g_hash_table_insert (klass->properties, property->name, property);
      break;
    case PROP_VALUE_TYPE:
      property->value_type = g_value_get_gtype (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_style_property_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  CtkStyleProperty *property = CTK_STYLE_PROPERTY (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, property->name);
      break;
    case PROP_VALUE_TYPE:
      g_value_set_gtype (value, property->value_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_ctk_style_property_class_init (CtkStylePropertyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_style_property_finalize;
  object_class->set_property = ctk_style_property_set_property;
  object_class->get_property = ctk_style_property_get_property;

  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        P_("Property name"),
                                                        P_("The name of the property"),
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   PROP_VALUE_TYPE,
                                   g_param_spec_gtype ("value-type",
                                                       P_("Value type"),
                                                       P_("The value type returned by CtkStyleContext"),
                                                       G_TYPE_NONE,
                                                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  klass->properties = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
_ctk_style_property_init (CtkStyleProperty *property)
{
  property->value_type = G_TYPE_NONE;
}

/**
 * _ctk_style_property_parse_value:
 * @property: the property
 * @parser: the parser to parse from
 *
 * Tries to parse the given @property from the given @parser into
 * @value. The type that @value will be assigned is dependant on
 * the parser and no assumptions must be made about it. If the
 * parsing fails, %FALSE will be returned and @value will be
 * left uninitialized.
 *
 * Only if @property is a #CtkCssShorthandProperty, the @value will
 * always be a #CtkCssValue whose values can be queried with
 * _ctk_css_array_value_get_nth().
 *
 * Returns: %NULL on failure or the parsed #CtkCssValue
 **/
CtkCssValue *
_ctk_style_property_parse_value (CtkStyleProperty *property,
                                 CtkCssParser     *parser)
{
  CtkStylePropertyClass *klass;

  g_return_val_if_fail (CTK_IS_STYLE_PROPERTY (property), NULL);
  g_return_val_if_fail (parser != NULL, NULL);

  klass = CTK_STYLE_PROPERTY_GET_CLASS (property);

  return klass->parse_value (property, parser);
}

/**
 * _ctk_style_property_assign:
 * @property: the property
 * @props: The properties to assign to
 * @state: The state to assign
 * @value: (out): the #GValue with the value to be
 *     assigned
 *
 * This function is called by ctk_style_properties_set() and in
 * turn ctk_style_context_set() and similar functions to set the
 * value from code using old APIs.
 **/
void
_ctk_style_property_assign (CtkStyleProperty   *property,
                            CtkStyleProperties *props,
                            CtkStateFlags       state,
                            const GValue       *value)
{
  CtkStylePropertyClass *klass;

  g_return_if_fail (CTK_IS_STYLE_PROPERTY (property));
  g_return_if_fail (CTK_IS_STYLE_PROPERTIES (props));
  g_return_if_fail (value != NULL);

  klass = CTK_STYLE_PROPERTY_GET_CLASS (property);

  klass->assign (property, props, state, value);
}

/**
 * _ctk_style_property_query:
 * @property: the property
 * @value: (out): an uninitialized #GValue to be filled with the
 *   contents of the lookup
 * @query_func: The function to use to query properties
 * @query_data: The data to pass to @query_func
 *
 * This function is called by ctk_style_properties_get() and in
 * turn ctk_style_context_get() and similar functions to get the
 * value to return to code using old APIs.
 **/
void
_ctk_style_property_query (CtkStyleProperty  *property,
                           GValue            *value,
                           CtkStyleQueryFunc  query_func,
                           gpointer           query_data)
{
  CtkStylePropertyClass *klass;

  g_return_if_fail (value != NULL);
  g_return_if_fail (CTK_IS_STYLE_PROPERTY (property));
  g_return_if_fail (query_func != NULL);

  klass = CTK_STYLE_PROPERTY_GET_CLASS (property);

  klass->query (property, value, query_func, query_data);
}

void
_ctk_style_property_init_properties (void)
{
  static gboolean initialized = FALSE;

  if (G_LIKELY (initialized))
    return;

  initialized = TRUE;

  _ctk_css_style_property_init_properties ();
  /* initialize shorthands last, they depend on the real properties existing */
  _ctk_css_shorthand_property_init_properties ();
}

void
_ctk_style_property_add_alias (const gchar *name,
                               const gchar *alias)
{
  CtkStylePropertyClass *klass;
  CtkStyleProperty *property;

  g_return_if_fail (name != NULL);
  g_return_if_fail (alias != NULL);

  klass = g_type_class_peek (CTK_TYPE_STYLE_PROPERTY);

  property = g_hash_table_lookup (klass->properties, name);

  g_assert (property != NULL);
  g_assert (g_hash_table_lookup (klass->properties, alias) == NULL);

  g_hash_table_insert (klass->properties, (gpointer)alias, property);
}

/**
 * _ctk_style_property_lookup:
 * @name: name of the property to lookup
 *
 * Looks up the CSS property with the given @name. If no such
 * property exists, %NULL is returned.
 *
 * Returns: (nullable) (transfer none): The property or %NULL if no
 *     property with the given name exists.
 **/
CtkStyleProperty *
_ctk_style_property_lookup (const char *name)
{
  CtkStylePropertyClass *klass;

  g_return_val_if_fail (name != NULL, NULL);

  _ctk_style_property_init_properties ();

  klass = g_type_class_peek (CTK_TYPE_STYLE_PROPERTY);

  return g_hash_table_lookup (klass->properties, name);
}

/**
 * _ctk_style_property_get_name:
 * @property: the property to query
 *
 * Gets the name of the given property.
 *
 * Returns: the name of the property
 **/
const char *
_ctk_style_property_get_name (CtkStyleProperty *property)
{
  g_return_val_if_fail (CTK_IS_STYLE_PROPERTY (property), NULL);

  return property->name;
}

/**
 * _ctk_style_property_get_value_type:
 * @property: the property to query
 *
 * Gets the value type of the @property, if the property is usable
 * in public API via _ctk_style_property_assign() and
 * _ctk_style_property_query(). If the @property is not usable in that
 * way, %G_TYPE_NONE is returned.
 *
 * Returns: the value type in use or %G_TYPE_NONE if none.
 **/
GType
_ctk_style_property_get_value_type (CtkStyleProperty *property)
{
  g_return_val_if_fail (CTK_IS_STYLE_PROPERTY (property), G_TYPE_NONE);

  return property->value_type;
}
