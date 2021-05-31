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

#include "gtkcsstypedvalueprivate.h"

#include "gtkcsscustompropertyprivate.h"
#include "gtkcssstylefuncsprivate.h"

struct _GtkCssValue {
  CTK_CSS_VALUE_BASE
  GValue value;
};

static void
ctk_css_value_typed_free (GtkCssValue *value)
{
  g_value_unset (&value->value);
  g_slice_free (GtkCssValue, value);
}

static GtkCssValue *
ctk_css_value_typed_compute (GtkCssValue             *value,
                             guint                    property_id,
                             GtkStyleProviderPrivate *provider,
                             GtkCssStyle             *style,
                             GtkCssStyle             *parent_style)
{
  GtkCssCustomProperty *custom = CTK_CSS_CUSTOM_PROPERTY (_ctk_css_style_property_lookup_by_id (property_id));

  return _ctk_css_style_funcs_compute_value (provider, style, parent_style, custom->pspec->value_type, value);
}

static gboolean
ctk_css_value_typed_equal (const GtkCssValue *value1,
                           const GtkCssValue *value2)
{
  return FALSE;
}

static GtkCssValue *
ctk_css_value_typed_transition (GtkCssValue *start,
                                GtkCssValue *end,
                                guint        property_id,
                                double       progress)
{
  return NULL;
}

static void
ctk_css_value_typed_print (const GtkCssValue *value,
                           GString           *string)
{
  _ctk_css_style_funcs_print_value (&value->value, string);
}

static const GtkCssValueClass CTK_CSS_VALUE_TYPED = {
  ctk_css_value_typed_free,
  ctk_css_value_typed_compute,
  ctk_css_value_typed_equal,
  ctk_css_value_typed_transition,
  ctk_css_value_typed_print
};

static GtkCssValue *
ctk_css_typed_value_new_for_type (GType type)
{
  GtkCssValue *result;

  result = _ctk_css_value_new (GtkCssValue, &CTK_CSS_VALUE_TYPED);

  g_value_init (&result->value, type);

  return result;
}

GtkCssValue *
_ctk_css_typed_value_new (const GValue *value)
{
  GtkCssValue *result;

  g_return_val_if_fail (G_IS_VALUE (value), NULL);

  result = ctk_css_typed_value_new_for_type (G_VALUE_TYPE (value));

  g_value_copy (value, &result->value);

  return result;
}

GtkCssValue *
_ctk_css_typed_value_new_take (GValue *value)
{
  GtkCssValue *result;

  g_return_val_if_fail (G_IS_VALUE (value), NULL);

  result = _ctk_css_typed_value_new (value);
  g_value_unset (value);

  return result;
}

gboolean
_ctk_is_css_typed_value_of_type (const GtkCssValue *value,
                                 GType              type)
{
  if (value->class != &CTK_CSS_VALUE_TYPED)
    return FALSE;

  return G_VALUE_HOLDS (&value->value, type);
}

const GValue *
_ctk_css_typed_value_get (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_TYPED, NULL);

  return &value->value;
}
