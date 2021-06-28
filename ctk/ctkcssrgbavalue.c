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

#include "ctkcssrgbavalueprivate.h"

#include "ctkcssstylepropertyprivate.h"
#include "ctkstylecontextprivate.h"

struct _CtkCssValue {
  CTK_CSS_VALUE_BASE
  GdkRGBA rgba;
};

static void
ctk_css_value_rgba_free (CtkCssValue *value)
{
  g_slice_free (CtkCssValue, value);
}

static CtkCssValue *
ctk_css_value_rgba_compute (CtkCssValue             *value,
                            guint                    property_id,
                            CtkStyleProviderPrivate *provider,
                            CtkCssStyle             *style,
                            CtkCssStyle             *parent_style)
{
  return _ctk_css_value_ref (value);
}

static gboolean
ctk_css_value_rgba_equal (const CtkCssValue *rgba1,
                          const CtkCssValue *rgba2)
{
  return cdk_rgba_equal (&rgba1->rgba, &rgba2->rgba);
}

static inline double
transition (double start,
            double end,
            double progress)
{
  return start + (end - start) * progress;
}

static CtkCssValue *
ctk_css_value_rgba_transition (CtkCssValue *start,
                               CtkCssValue *end,
                               guint        property_id,
                               double       progress)
{
  GdkRGBA result;

  progress = CLAMP (progress, 0, 1);
  result.alpha = transition (start->rgba.alpha, end->rgba.alpha, progress);
  if (result.alpha <= 0.0)
    {
      result.red = result.green = result.blue = 0.0;
    }
  else
    {
      result.red = transition (start->rgba.red * start->rgba.alpha,
                               end->rgba.red * end->rgba.alpha,
                               progress) / result.alpha;
      result.green = transition (start->rgba.green * start->rgba.alpha,
                                 end->rgba.green * end->rgba.alpha,
                                 progress) / result.alpha;
      result.blue = transition (start->rgba.blue * start->rgba.alpha,
                                end->rgba.blue * end->rgba.alpha,
                                progress) / result.alpha;
    }

  return _ctk_css_rgba_value_new_from_rgba (&result);
}

static void
ctk_css_value_rgba_print (const CtkCssValue *rgba,
                          GString           *string)
{
  char *s = cdk_rgba_to_string (&rgba->rgba);
  g_string_append (string, s);
  g_free (s);
}

static const CtkCssValueClass CTK_CSS_VALUE_RGBA = {
  ctk_css_value_rgba_free,
  ctk_css_value_rgba_compute,
  ctk_css_value_rgba_equal,
  ctk_css_value_rgba_transition,
  ctk_css_value_rgba_print
};

CtkCssValue *
_ctk_css_rgba_value_new_from_rgba (const GdkRGBA *rgba)
{
  CtkCssValue *value;

  g_return_val_if_fail (rgba != NULL, NULL);

  value = _ctk_css_value_new (CtkCssValue, &CTK_CSS_VALUE_RGBA);
  value->rgba = *rgba;

  return value;
}

const GdkRGBA *
_ctk_css_rgba_value_get_rgba (const CtkCssValue *rgba)
{
  g_return_val_if_fail (rgba->class == &CTK_CSS_VALUE_RGBA, NULL);

  return &rgba->rgba;
}
