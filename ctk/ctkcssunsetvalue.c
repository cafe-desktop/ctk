/* CTK - The GIMP Toolkit
 * Copyright (C) 2014 Red Hat, Inc.
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

#include "ctkcssunsetvalueprivate.h"

#include "ctkcssinheritvalueprivate.h"
#include "ctkcssinitialvalueprivate.h"
#include "ctkcssstylepropertyprivate.h"

struct _CtkCssValue {
  CTK_CSS_VALUE_BASE
};

static void
ctk_css_value_unset_free (CtkCssValue *value)
{
  /* Can only happen if the unique value gets unreffed too often */
  g_assert_not_reached ();
}

static CtkCssValue *
ctk_css_value_unset_compute (CtkCssValue             *value,
                             guint                    property_id,
                             CtkStyleProviderPrivate *provider,
                             CtkCssStyle             *style,
                             CtkCssStyle             *parent_style)
{
  CtkCssStyleProperty *property;
  CtkCssValue *unset_value;
  
  property = _ctk_css_style_property_lookup_by_id (property_id);

  if (_ctk_css_style_property_is_inherit (property))
    unset_value = _ctk_css_inherit_value_get ();
  else
    unset_value = _ctk_css_initial_value_get ();

  return _ctk_css_value_compute (unset_value,
                                 property_id,
                                 provider,
                                 style,
                                 parent_style);
}

static gboolean
ctk_css_value_unset_equal (const CtkCssValue *value1,
                           const CtkCssValue *value2)
{
  return TRUE;
}

static CtkCssValue *
ctk_css_value_unset_transition (CtkCssValue *start,
                                CtkCssValue *end,
                                guint        property_id,
                                double       progress)
{
  return NULL;
}

static void
ctk_css_value_unset_print (const CtkCssValue *value,
                             GString           *string)
{
  g_string_append (string, "unset");
}

static const CtkCssValueClass CTK_CSS_VALUE_UNSET = {
  ctk_css_value_unset_free,
  ctk_css_value_unset_compute,
  ctk_css_value_unset_equal,
  ctk_css_value_unset_transition,
  ctk_css_value_unset_print
};

static CtkCssValue unset = { &CTK_CSS_VALUE_UNSET, 1 };

CtkCssValue *
_ctk_css_unset_value_new (void)
{
  return _ctk_css_value_ref (&unset);
}
