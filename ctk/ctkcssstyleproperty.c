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

#include "ctkcssstylepropertyprivate.h"

#include "ctkcssenumvalueprivate.h"
#include "ctkcssinheritvalueprivate.h"
#include "ctkcssinitialvalueprivate.h"
#include "ctkcssstylefuncsprivate.h"
#include "ctkcsstypesprivate.h"
#include "ctkcssunsetvalueprivate.h"
#include "ctkintl.h"
#include "ctkprivatetypebuiltins.h"
#include "ctkstylepropertiesprivate.h"
#include "ctkprivate.h"

/* this is in case round() is not provided by the compiler, 
 * such as in the case of C89 compilers, like MSVC
 */
#include "fallback-c89.c"

enum {
  PROP_0,
  PROP_ANIMATED,
  PROP_AFFECTS,
  PROP_ID,
  PROP_INHERIT,
  PROP_INITIAL
};

G_DEFINE_TYPE (CtkCssStyleProperty, _ctk_css_style_property, CTK_TYPE_STYLE_PROPERTY)

static CtkCssStylePropertyClass *ctk_css_style_property_class = NULL;

static void
ctk_css_style_property_constructed (GObject *object)
{
  CtkCssStyleProperty *property = CTK_CSS_STYLE_PROPERTY (object);
  CtkCssStylePropertyClass *klass = CTK_CSS_STYLE_PROPERTY_GET_CLASS (property);

  property->id = klass->style_properties->len;
  g_ptr_array_add (klass->style_properties, property);

  G_OBJECT_CLASS (_ctk_css_style_property_parent_class)->constructed (object);
}

static void
ctk_css_style_property_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  CtkCssStyleProperty *property = CTK_CSS_STYLE_PROPERTY (object);

  switch (prop_id)
    {
    case PROP_ANIMATED:
      property->animated = g_value_get_boolean (value);
      break;
    case PROP_AFFECTS:
      property->affects = g_value_get_flags (value);
      break;
    case PROP_INHERIT:
      property->inherit = g_value_get_boolean (value);
      break;
    case PROP_INITIAL:
      property->initial_value = g_value_dup_boxed (value);
      g_assert (property->initial_value != NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_css_style_property_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  CtkCssStyleProperty *property = CTK_CSS_STYLE_PROPERTY (object);

  switch (prop_id)
    {
    case PROP_ANIMATED:
      g_value_set_boolean (value, property->animated);
      break;
    case PROP_AFFECTS:
      g_value_set_flags (value, property->affects);
      break;
    case PROP_ID:
      g_value_set_boolean (value, property->id);
      break;
    case PROP_INHERIT:
      g_value_set_boolean (value, property->inherit);
      break;
    case PROP_INITIAL:
      g_value_set_boxed (value, property->initial_value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_ctk_css_style_property_assign (CtkStyleProperty   *property,
                                CtkStyleProperties *props,
                                CtkStateFlags       state,
                                const GValue       *value)
{
  CtkCssStyleProperty *style;
  CtkCssValue *css_value;
  
  style = CTK_CSS_STYLE_PROPERTY (property);
  css_value = style->assign_value (style, value);

  _ctk_style_properties_set_property_by_property (props,
                                                  style,
                                                  state,
                                                  css_value);
  _ctk_css_value_unref (css_value);
}

static void
_ctk_css_style_property_query (CtkStyleProperty   *property,
                               GValue             *value,
                               CtkStyleQueryFunc   query_func,
                               gpointer            query_data)
{
  CtkCssStyleProperty *style_property = CTK_CSS_STYLE_PROPERTY (property);
  CtkCssValue *css_value;

  css_value = (* query_func) (CTK_CSS_STYLE_PROPERTY (property)->id, query_data);
  if (css_value == NULL)
    css_value =_ctk_css_style_property_get_initial_value (style_property);

  style_property->query_value (style_property, css_value, value);
}

static CtkCssValue *
ctk_css_style_property_parse_value (CtkStyleProperty *property,
                                    CtkCssParser     *parser)
{
  CtkCssStyleProperty *style_property = CTK_CSS_STYLE_PROPERTY (property);

  if (_ctk_css_parser_try (parser, "initial", TRUE))
    {
      /* the initial value can be explicitly specified with the
       * ‘initial’ keyword which all properties accept.
       */
      return _ctk_css_initial_value_new ();
    }
  else if (_ctk_css_parser_try (parser, "inherit", TRUE))
    {
      /* All properties accept the ‘inherit’ value which
       * explicitly specifies that the value will be determined
       * by inheritance. The ‘inherit’ value can be used to
       * strengthen inherited values in the cascade, and it can
       * also be used on properties that are not normally inherited.
       */
      return _ctk_css_inherit_value_new ();
    }
  else if (_ctk_css_parser_try (parser, "unset", TRUE))
    {
      /* If the cascaded value of a property is the unset keyword,
       * then if it is an inherited property, this is treated as
       * inherit, and if it is not, this is treated as initial.
       */
      return _ctk_css_unset_value_new ();
    }

  return (* style_property->parse_value) (style_property, parser);
}

static void
_ctk_css_style_property_class_init (CtkCssStylePropertyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkStylePropertyClass *property_class = CTK_STYLE_PROPERTY_CLASS (klass);

  object_class->constructed = ctk_css_style_property_constructed;
  object_class->set_property = ctk_css_style_property_set_property;
  object_class->get_property = ctk_css_style_property_get_property;

  g_object_class_install_property (object_class,
                                   PROP_ANIMATED,
                                   g_param_spec_boolean ("animated",
                                                         P_("Animated"),
                                                         P_("Set if the value can be animated"),
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   PROP_AFFECTS,
                                   g_param_spec_flags ("affects",
                                                       P_("Affects"),
                                                       P_("Set if the value affects the sizing of elements"),
                                                       CTK_TYPE_CSS_AFFECTS,
                                                       0,
                                                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   PROP_ID,
                                   g_param_spec_uint ("id",
                                                      P_("ID"),
                                                      P_("The numeric id for quick access"),
                                                      0, G_MAXUINT, 0,
                                                      G_PARAM_READABLE));
  g_object_class_install_property (object_class,
                                   PROP_INHERIT,
                                   g_param_spec_boolean ("inherit",
                                                         P_("Inherit"),
                                                         P_("Set if the value is inherited by default"),
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   PROP_INITIAL,
                                   g_param_spec_boxed ("initial-value",
                                                       P_("Initial value"),
                                                       P_("The initial specified value used for this property"),
                                                       CTK_TYPE_CSS_VALUE,
                                                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  property_class->assign = _ctk_css_style_property_assign;
  property_class->query = _ctk_css_style_property_query;
  property_class->parse_value = ctk_css_style_property_parse_value;

  klass->style_properties = g_ptr_array_new ();

  ctk_css_style_property_class = klass;
}

static CtkCssValue *
ctk_css_style_property_real_parse_value (CtkCssStyleProperty *property,
                                         CtkCssParser        *parser)
{
  g_assert_not_reached ();
  return NULL;
}

static void
_ctk_css_style_property_init (CtkCssStyleProperty *property)
{
  property->parse_value = ctk_css_style_property_real_parse_value;
}

/**
 * _ctk_css_style_property_get_n_properties:
 *
 * Gets the number of style properties. This number can increase when new
 * theme engines are loaded. Shorthand properties are not included here.
 *
 * Returns: The number of style properties.
 **/
guint
_ctk_css_style_property_get_n_properties (void)
{
  if (G_UNLIKELY (ctk_css_style_property_class == NULL))
    {
      _ctk_style_property_init_properties ();
      g_assert (ctk_css_style_property_class);
    }

  return ctk_css_style_property_class->style_properties->len;
}

/**
 * _ctk_css_style_property_lookup_by_id:
 * @id: the id of the property
 *
 * Gets the style property with the given id. All style properties (but not
 * shorthand properties) are indexable by id so that it’s easy to use arrays
 * when doing style lookups.
 *
 * Returns: (transfer none): The style property with the given id
 **/
CtkCssStyleProperty *
_ctk_css_style_property_lookup_by_id (guint id)
{

  if (G_UNLIKELY (ctk_css_style_property_class == NULL))
    {
      _ctk_style_property_init_properties ();
      g_assert (ctk_css_style_property_class);
    }

  ctk_internal_return_val_if_fail (id < ctk_css_style_property_class->style_properties->len, NULL);

  return g_ptr_array_index (ctk_css_style_property_class->style_properties, id);
}

/**
 * _ctk_css_style_property_is_inherit:
 * @property: the property
 *
 * Queries if the given @property is inherited. See the
 * [CSS Documentation](http://www.w3.org/TR/css3-cascade/#inheritance)
 * for an explanation of this concept.
 *
 * Returns: %TRUE if the property is inherited by default.
 **/
gboolean
_ctk_css_style_property_is_inherit (CtkCssStyleProperty *property)
{
  ctk_internal_return_val_if_fail (CTK_IS_CSS_STYLE_PROPERTY (property), FALSE);

  return property->inherit;
}

/**
 * _ctk_css_style_property_is_animated:
 * @property: the property
 *
 * Queries if the given @property can be is animated. See the
 * [CSS Documentation](http://www.w3.org/TR/css3-transitions/#animatable-css)
 * for animatable properties.
 *
 * Returns: %TRUE if the property can be animated.
 **/
gboolean
_ctk_css_style_property_is_animated (CtkCssStyleProperty *property)
{
  ctk_internal_return_val_if_fail (CTK_IS_CSS_STYLE_PROPERTY (property), FALSE);

  return property->animated;
}

/**
 * _ctk_css_style_property_get_affects:
 * @property: the property
 *
 * Returns all the things this property affects. See @CtkCssAffects for what
 * the flags mean.
 *
 * Returns: The things this property affects.
 **/
CtkCssAffects
_ctk_css_style_property_get_affects (CtkCssStyleProperty *property)
{
  ctk_internal_return_val_if_fail (CTK_IS_CSS_STYLE_PROPERTY (property), 0);

  return property->affects;
}

/**
 * _ctk_css_style_property_get_id:
 * @property: the property
 *
 * Gets the id for the given property. IDs are used to allow using arrays
 * for style lookups.
 *
 * Returns: The id of the property
 **/
guint
_ctk_css_style_property_get_id (CtkCssStyleProperty *property)
{
  ctk_internal_return_val_if_fail (CTK_IS_CSS_STYLE_PROPERTY (property), 0);

  return property->id;
}

/**
 * _ctk_css_style_property_get_initial_value:
 * @property: the property
 *
 * Queries the initial value of the given @property. See the
 * [CSS Documentation](http://www.w3.org/TR/css3-cascade/#intial)
 * for an explanation of this concept.
 *
 * Returns: (transfer none): the initial value. The value will never change.
 **/
CtkCssValue *
_ctk_css_style_property_get_initial_value (CtkCssStyleProperty *property)
{
  ctk_internal_return_val_if_fail (CTK_IS_CSS_STYLE_PROPERTY (property), NULL);

  return property->initial_value;
}

/**
 * _ctk_css_style_property_get_mask_affecting:
 * @flags: the flags that are affected
 *
 * Computes a bitmask for all properties that have at least one of @flags
 * set.
 *
 * Returns: (transfer full): A #CtkBitmask with the bit set for every
 *          property that has at least one of @flags set.
 */
CtkBitmask *
_ctk_css_style_property_get_mask_affecting (CtkCssAffects affects)
{
  CtkBitmask *result;
  guint i;

  result = _ctk_bitmask_new ();

  for (i = 0; i < _ctk_css_style_property_get_n_properties (); i++)
    {
      CtkCssStyleProperty *prop = _ctk_css_style_property_lookup_by_id (i);

      if (_ctk_css_style_property_get_affects (prop) & affects)
        result = _ctk_bitmask_set (result, i, TRUE);
    }

  return result;
}

