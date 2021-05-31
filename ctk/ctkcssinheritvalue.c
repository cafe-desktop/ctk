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

#include "ctkcssinheritvalueprivate.h"

#include "ctkcssinitialvalueprivate.h"
#include "ctkstylecontextprivate.h"

struct _CtkCssValue {
  CTK_CSS_VALUE_BASE
};

static void
ctk_css_value_inherit_free (CtkCssValue *value)
{
  /* Can only happen if the unique value gets unreffed too often */
  g_assert_not_reached ();
}

static CtkCssValue *
ctk_css_value_inherit_compute (CtkCssValue             *value,
                               guint                    property_id,
                               CtkStyleProviderPrivate *provider,
                               CtkCssStyle             *style,
                               CtkCssStyle             *parent_style)
{
  if (parent_style)
    {
      return _ctk_css_value_ref (ctk_css_style_get_value (parent_style, property_id));
    }
  else
    {
      return _ctk_css_value_compute (_ctk_css_initial_value_get (),
                                     property_id,
                                     provider,
                                     style,
                                     parent_style);
    }
}

static gboolean
ctk_css_value_inherit_equal (const CtkCssValue *value1,
                             const CtkCssValue *value2)
{
  return TRUE;
}

static CtkCssValue *
ctk_css_value_inherit_transition (CtkCssValue *start,
                                  CtkCssValue *end,
                                  guint        property_id,
                                  double       progress)
{
  return NULL;
}

static void
ctk_css_value_inherit_print (const CtkCssValue *value,
                             GString           *string)
{
  g_string_append (string, "inherit");
}

static const CtkCssValueClass CTK_CSS_VALUE_INHERIT = {
  ctk_css_value_inherit_free,
  ctk_css_value_inherit_compute,
  ctk_css_value_inherit_equal,
  ctk_css_value_inherit_transition,
  ctk_css_value_inherit_print
};

static CtkCssValue inherit = { &CTK_CSS_VALUE_INHERIT, 1 };

CtkCssValue *
_ctk_css_inherit_value_new (void)
{
  return _ctk_css_value_ref (&inherit);
}

CtkCssValue *
_ctk_css_inherit_value_get (void)
{
  return &inherit;
}
