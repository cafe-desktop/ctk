/* CTK - The GIMP Toolkit
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

#include "ctkprivate.h"
#include "ctkcssvalueprivate.h"

#include "ctkcssstyleprivate.h"
#include "ctkstyleproviderprivate.h"

struct _CtkCssValue {
  CTK_CSS_VALUE_BASE
};

G_DEFINE_BOXED_TYPE (CtkCssValue, _ctk_css_value, _ctk_css_value_ref, _ctk_css_value_unref)

CtkCssValue *
_ctk_css_value_alloc (const CtkCssValueClass *klass,
                      gsize                   size)
{
  CtkCssValue *value;

  value = g_slice_alloc0 (size);

  value->class = klass;
  value->ref_count = 1;

  return value;
}

CtkCssValue *
_ctk_css_value_ref (CtkCssValue *value)
{
  ctk_internal_return_val_if_fail (value != NULL, NULL);

  value->ref_count += 1;

  return value;
}

void
_ctk_css_value_unref (CtkCssValue *value)
{
  if (value == NULL)
    return;

  value->ref_count -= 1;
  if (value->ref_count > 0)
    return;

  value->class->free (value);
}

/**
 * _ctk_css_value_compute:
 * @value: the value to compute from
 * @property_id: the ID of the property to compute
 * @provider: Style provider for looking up extra information
 * @style: Style to compute for
 * @parent_style: parent style to use for inherited values
 *
 * Converts the specified @value into the computed value for the CSS
 * property given by @property_id using the information in @context.
 * This step is explained in detail in the
 * [CSS Documentation](http://www.w3.org/TR/css3-cascade/#computed).
 *
 * Returns: the computed value
 **/
CtkCssValue *
_ctk_css_value_compute (CtkCssValue             *value,
                        guint                    property_id,
                        CtkStyleProviderPrivate *provider,
                        CtkCssStyle             *style,
                        CtkCssStyle             *parent_style)
{

  ctk_internal_return_val_if_fail (value != NULL, NULL);
  ctk_internal_return_val_if_fail (CTK_IS_STYLE_PROVIDER_PRIVATE (provider), NULL);
  ctk_internal_return_val_if_fail (CTK_IS_CSS_STYLE (style), NULL);
  ctk_internal_return_val_if_fail (parent_style == NULL || CTK_IS_CSS_STYLE (parent_style), NULL);

  return value->class->compute (value, property_id, provider, style, parent_style);
}

gboolean
_ctk_css_value_equal (const CtkCssValue *value1,
                      const CtkCssValue *value2)
{
  ctk_internal_return_val_if_fail (value1 != NULL, FALSE);
  ctk_internal_return_val_if_fail (value2 != NULL, FALSE);

  if (value1 == value2)
    return TRUE;

  if (value1->class != value2->class)
    return FALSE;

  return value1->class->equal (value1, value2);
}

gboolean
_ctk_css_value_equal0 (const CtkCssValue *value1,
                       const CtkCssValue *value2)
{
  /* Inclues both values being NULL */
  if (value1 == value2)
    return TRUE;

  if (value1 == NULL || value2 == NULL)
    return FALSE;

  return _ctk_css_value_equal (value1, value2);
}

CtkCssValue *
_ctk_css_value_transition (CtkCssValue *start,
                           CtkCssValue *end,
                           guint        property_id,
                           double       progress)
{
  ctk_internal_return_val_if_fail (start != NULL, FALSE);
  ctk_internal_return_val_if_fail (end != NULL, FALSE);

  /* We compare functions here instead of classes so that number
   * values can all transition to each other */
  if (start->class->transition != end->class->transition)
    return NULL;

  return start->class->transition (start, end, property_id, progress);
}

char *
_ctk_css_value_to_string (const CtkCssValue *value)
{
  GString *string;

  ctk_internal_return_val_if_fail (value != NULL, NULL);

  string = g_string_new (NULL);
  _ctk_css_value_print (value, string);
  return g_string_free (string, FALSE);
}

/**
 * _ctk_css_value_print:
 * @value: the value to print
 * @string: the string to print to
 *
 * Prints @value to the given @string in CSS format. The @value must be a
 * valid specified value as parsed using the parse functions or as assigned
 * via _ctk_style_property_assign().
 **/
void
_ctk_css_value_print (const CtkCssValue *value,
                      GString           *string)
{
  ctk_internal_return_if_fail (value != NULL);
  ctk_internal_return_if_fail (string != NULL);

  value->class->print (value, string);
}

