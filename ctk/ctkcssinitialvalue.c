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

#include "ctkcssinitialvalueprivate.h"

#include "ctkcssarrayvalueprivate.h"
#include "ctkcssnumbervalueprivate.h"
#include "ctkcssstringvalueprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctksettingsprivate.h"
#include "ctkstyleproviderprivate.h"

struct _GtkCssValue {
  CTK_CSS_VALUE_BASE
};

static void
ctk_css_value_initial_free (GtkCssValue *value)
{
  /* Can only happen if the unique value gets unreffed too often */
  g_assert_not_reached ();
}

static GtkCssValue *
ctk_css_value_initial_compute (GtkCssValue             *value,
                               guint                    property_id,
                               GtkStyleProviderPrivate *provider,
                               GtkCssStyle             *style,
                               GtkCssStyle             *parent_style)
{
  GtkSettings *settings;

  switch (property_id)
    {
    case CTK_CSS_PROPERTY_DPI:
      settings = _ctk_style_provider_private_get_settings (provider);
      if (settings)
        {
          GdkScreen *screen = _ctk_settings_get_screen (settings);
          double resolution = gdk_screen_get_resolution (screen);

          if (resolution > 0.0)
            return _ctk_css_number_value_new (resolution, CTK_CSS_NUMBER);
        }
      break;

    case CTK_CSS_PROPERTY_FONT_FAMILY:
      settings = _ctk_style_provider_private_get_settings (provider);
      if (settings && ctk_settings_get_font_family (settings) != NULL)
        return _ctk_css_array_value_new (_ctk_css_string_value_new (ctk_settings_get_font_family (settings)));
      break;

    default:
      break;
    }

  return _ctk_css_value_compute (_ctk_css_style_property_get_initial_value (_ctk_css_style_property_lookup_by_id (property_id)),
                                 property_id,
                                 provider,
                                 style,
                                 parent_style);
}

static gboolean
ctk_css_value_initial_equal (const GtkCssValue *value1,
                             const GtkCssValue *value2)
{
  return TRUE;
}

static GtkCssValue *
ctk_css_value_initial_transition (GtkCssValue *start,
                                  GtkCssValue *end,
                                  guint        property_id,
                                  double       progress)
{
  return NULL;
}

static void
ctk_css_value_initial_print (const GtkCssValue *value,
                             GString           *string)
{
  g_string_append (string, "initial");
}

static const GtkCssValueClass CTK_CSS_VALUE_INITIAL = {
  ctk_css_value_initial_free,
  ctk_css_value_initial_compute,
  ctk_css_value_initial_equal,
  ctk_css_value_initial_transition,
  ctk_css_value_initial_print
};

static GtkCssValue initial = { &CTK_CSS_VALUE_INITIAL, 1 };

GtkCssValue *
_ctk_css_initial_value_new (void)
{
  return _ctk_css_value_ref (&initial);
}

GtkCssValue *
_ctk_css_initial_value_get (void)
{
  return &initial;
}
