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

#include "ctkcsscolorvalueprivate.h"
#include "ctkcssrgbavalueprivate.h"

struct _CtkCssValue {
  CTK_CSS_VALUE_BASE
  GHashTable *colors;
};

static CtkCssValue *default_palette;

static CtkCssValue *ctk_css_palette_value_new_empty (void);

static void
ctk_css_palette_value_add_color (CtkCssValue *value,
                                 const char  *name,
                                 CtkCssValue *color)
{
  g_hash_table_insert (value->colors, g_strdup (name), color);
}

static void
ctk_css_value_palette_free (CtkCssValue *value)
{
  g_hash_table_unref (value->colors);

  g_slice_free (CtkCssValue, value);
}

static CtkCssValue *
ctk_css_value_palette_compute (CtkCssValue             *specified,
                               guint                    property_id,
                               CtkStyleProviderPrivate *provider,
                               CtkCssStyle             *style,
                               CtkCssStyle             *parent_style)
{
  GHashTableIter iter;
  gpointer name, value;
  CtkCssValue *computed_color;
  CtkCssValue *result;
  gboolean changes = FALSE;

  result = ctk_css_palette_value_new_empty ();

  g_hash_table_iter_init (&iter, specified->colors);
  while (g_hash_table_iter_next (&iter, &name, &value))
    {
      computed_color = _ctk_css_value_compute (value, property_id, provider, style, parent_style);
      changes |= computed_color != value;
      ctk_css_palette_value_add_color (result, name, computed_color);
    }

  if (!changes)
    {
      _ctk_css_value_unref (result);
      result = _ctk_css_value_ref (specified);
    }

  return result;
}

static gboolean
ctk_css_value_palette_equal (const CtkCssValue *value1,
                             const CtkCssValue *value2)
{
  gpointer name, color1, color2;
  GHashTableIter iter;

  if (g_hash_table_size (value1->colors) != g_hash_table_size (value2->colors))
    return FALSE;

  g_hash_table_iter_init (&iter, value1->colors);
  while (g_hash_table_iter_next (&iter, &name, &color1))
    {
      color2 = g_hash_table_lookup (value2->colors, name);
      if (color2 == NULL)
        return FALSE;

      if (!_ctk_css_value_equal (color1, color2))
        return FALSE;
    }

  return TRUE;
}

static CtkCssValue *
ctk_css_value_palette_transition (CtkCssValue *start,
                                  CtkCssValue *end,
                                  guint        property_id,
                                  double       progress)
{
  gpointer name, start_color, end_color;
  GHashTableIter iter;
  CtkCssValue *result, *transition;

  /* XXX: For colors that are only in start or end but not both,
   * we don't transition but just keep the value.
   * That causes an abrupt transition to currentColor at the end.
   */

  result = ctk_css_palette_value_new_empty ();

  g_hash_table_iter_init (&iter, start->colors);
  while (g_hash_table_iter_next (&iter, &name, &start_color))
    {
      end_color = g_hash_table_lookup (end->colors, name);
      if (end_color == NULL)
        transition = _ctk_css_value_ref (start_color);
      else
        transition = _ctk_css_value_transition (start_color, end_color, property_id, progress);

      ctk_css_palette_value_add_color (result, name, transition);
    }

  g_hash_table_iter_init (&iter, end->colors);
  while (g_hash_table_iter_next (&iter, &name, &end_color))
    {
      start_color = g_hash_table_lookup (start->colors, name);
      if (start_color != NULL)
        continue;

      ctk_css_palette_value_add_color (result, name, _ctk_css_value_ref (end_color));
    }

  return result;
}

static void
ctk_css_value_palette_print (const CtkCssValue *value,
                             GString           *string)
{
  GHashTableIter iter;
  gpointer name, color;
  gboolean first = TRUE;

  if (value == default_palette)
    {
      g_string_append (string, "default");
      return;
    }

  g_hash_table_iter_init (&iter, value->colors);
  while (g_hash_table_iter_next (&iter, &name, &color))
    {
      if (first)
        first = FALSE;
      else
        g_string_append (string, ", ");
      g_string_append (string, name);
      g_string_append_c (string, ' ');
      _ctk_css_value_print (color, string);
    }
}

static const CtkCssValueClass CTK_CSS_VALUE_PALETTE = {
  ctk_css_value_palette_free,
  ctk_css_value_palette_compute,
  ctk_css_value_palette_equal,
  ctk_css_value_palette_transition,
  ctk_css_value_palette_print
};

static CtkCssValue *
ctk_css_palette_value_new_empty (void)
{
  CtkCssValue *result;

  result = _ctk_css_value_new (CtkCssValue, &CTK_CSS_VALUE_PALETTE);
  result->colors = g_hash_table_new_full (g_str_hash, g_str_equal,
                                          g_free,
                                          (GDestroyNotify) _ctk_css_value_unref);

  return result;
}

CtkCssValue *
ctk_css_palette_value_new_default (void)
{
  if (default_palette == NULL)
    {
      default_palette = ctk_css_palette_value_new_empty ();
      ctk_css_palette_value_add_color (default_palette, "error", _ctk_css_color_value_new_name ("error_color"));
      ctk_css_palette_value_add_color (default_palette, "warning", _ctk_css_color_value_new_name ("warning_color"));
      ctk_css_palette_value_add_color (default_palette, "success", _ctk_css_color_value_new_name ("success_color"));
    }

  return _ctk_css_value_ref (default_palette);
}

CtkCssValue *
ctk_css_palette_value_parse (CtkCssParser *parser)
{
  CtkCssValue *result, *color;
  char *ident;

  if (_ctk_css_parser_try (parser, "default", TRUE))
    return ctk_css_palette_value_new_default ();
  
  result = ctk_css_palette_value_new_empty ();

  do {
    ident = _ctk_css_parser_try_ident (parser, TRUE);
    if (ident == NULL)
      {
        _ctk_css_parser_error (parser, "expected color name");
        _ctk_css_value_unref (result);
        return NULL;
      }
    
    color = _ctk_css_color_value_parse (parser);
    if (color == NULL)
      {
        g_free (ident);
        _ctk_css_value_unref (result);
        return NULL;
      }

    ctk_css_palette_value_add_color (result, ident, color);
    g_free (ident);
  } while (_ctk_css_parser_try (parser, ",", TRUE));

  return result;
}

const GdkRGBA *
ctk_css_palette_value_get_color (CtkCssValue *value,
                                 const char  *name)
{
  CtkCssValue *color;

  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_PALETTE, NULL);

  color = g_hash_table_lookup (value->colors, name);
  if (color == NULL)
    return NULL;

  return _ctk_css_rgba_value_get_rgba (color);
}
