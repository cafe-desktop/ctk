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

#include "ctkcssiconthemevalueprivate.h"

#include "ctkicontheme.h"
#include "ctksettingsprivate.h"
#include "ctkstyleproviderprivate.h"

/*
 * The idea behind this value (and the '-ctk-icon-theme' CSS property) is
 * to track changes to the icon theme.
 *
 * We create a new instance of this value whenever the icon theme changes
 * (via emitting the changed signal). So as long as the icon theme does
 * not change, we will compute the same value. We can then compare values
 * by pointer to see if the icon theme changed.
 */

struct _CtkCssValue {
  CTK_CSS_VALUE_BASE
  CtkIconTheme *icontheme;
  guint changed_id;
};

static void
ctk_css_value_icon_theme_disconnect_handler (CtkCssValue *value)
{
  if (value->changed_id == 0)
    return;

  g_object_set_data (G_OBJECT (value->icontheme), "-ctk-css-value", NULL);

  g_signal_handler_disconnect (value->icontheme, value->changed_id);
  value->changed_id = 0;
}

static void
ctk_css_value_icon_theme_changed_cb (CtkIconTheme *icontheme G_GNUC_UNUSED,
                                     CtkCssValue  *value)
{
  ctk_css_value_icon_theme_disconnect_handler (value);
}

static void
ctk_css_value_icon_theme_free (CtkCssValue *value)
{
  ctk_css_value_icon_theme_disconnect_handler (value);

  if (value->icontheme)
    g_object_unref (value->icontheme);

  g_slice_free (CtkCssValue, value);
}

static CtkCssValue *
ctk_css_value_icon_theme_compute (CtkCssValue             *icon_theme,
                                  guint                    property_id G_GNUC_UNUSED,
                                  CtkStyleProviderPrivate *provider,
                                  CtkCssStyle             *style G_GNUC_UNUSED,
                                  CtkCssStyle             *parent_style G_GNUC_UNUSED)
{
  CtkIconTheme *icontheme;

  if (icon_theme->icontheme)
    icontheme = icon_theme->icontheme;
  else
    icontheme = ctk_icon_theme_get_for_screen (_ctk_settings_get_screen (_ctk_style_provider_private_get_settings (provider)));

  return ctk_css_icon_theme_value_new (icontheme);
}

static gboolean
ctk_css_value_icon_theme_equal (const CtkCssValue *value1 G_GNUC_UNUSED,
                                const CtkCssValue *value2 G_GNUC_UNUSED)
{
  return FALSE;
}

static CtkCssValue *
ctk_css_value_icon_theme_transition (CtkCssValue *start G_GNUC_UNUSED,
                                     CtkCssValue *end G_GNUC_UNUSED,
                                     guint        property_id G_GNUC_UNUSED,
                                     double       progress G_GNUC_UNUSED)
{
  return NULL;
}

static void
ctk_css_value_icon_theme_print (const CtkCssValue *icon_theme G_GNUC_UNUSED,
                                GString           *string)
{
  g_string_append (string, "initial");
}

static const CtkCssValueClass CTK_CSS_VALUE_ICON_THEME = {
  ctk_css_value_icon_theme_free,
  ctk_css_value_icon_theme_compute,
  ctk_css_value_icon_theme_equal,
  ctk_css_value_icon_theme_transition,
  ctk_css_value_icon_theme_print
};

static CtkCssValue default_icon_theme_value = { &CTK_CSS_VALUE_ICON_THEME, 1, NULL, 0 };

CtkCssValue *
ctk_css_icon_theme_value_new (CtkIconTheme *icontheme)
{
  CtkCssValue *result;

  if (icontheme == NULL)
    return _ctk_css_value_ref (&default_icon_theme_value);

  result = g_object_get_data (G_OBJECT (icontheme), "-ctk-css-value");
  if (result)
    return _ctk_css_value_ref (result);

  result = _ctk_css_value_new (CtkCssValue, &CTK_CSS_VALUE_ICON_THEME);
  result->icontheme = g_object_ref (icontheme);

  g_object_set_data (G_OBJECT (icontheme), "-ctk-css-value", result);
  result->changed_id = g_signal_connect (icontheme, "changed", G_CALLBACK (ctk_css_value_icon_theme_changed_cb), result);

  return result;
}

CtkCssValue *
ctk_css_icon_theme_value_parse (CtkCssParser *parser)
{
  CtkIconTheme *icontheme;
  CtkCssValue *result;
  char *s;

  s = _ctk_css_parser_read_string (parser);
  if (s == NULL)
    return NULL;

  icontheme = ctk_icon_theme_new ();
  ctk_icon_theme_set_custom_theme (icontheme, s);

  result = ctk_css_icon_theme_value_new (icontheme);

  g_object_unref (icontheme);
  g_free (s);

  return result;
}

CtkIconTheme *
ctk_css_icon_theme_value_get_icon_theme (CtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_ICON_THEME, NULL);

  return value->icontheme;
}
