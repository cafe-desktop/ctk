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

#include "ctkcssstringvalueprivate.h"

#include <string.h>

struct _CtkCssValue {
  CTK_CSS_VALUE_BASE
  char *string;
};

static void
ctk_css_value_string_free (CtkCssValue *value)
{
  g_free (value->string);
  g_slice_free (CtkCssValue, value);
}

static CtkCssValue *
ctk_css_value_string_compute (CtkCssValue             *value,
                              guint                    property_id G_GNUC_UNUSED,
                              CtkStyleProviderPrivate *provider G_GNUC_UNUSED,
                              CtkCssStyle             *style G_GNUC_UNUSED,
                              CtkCssStyle             *parent_style G_GNUC_UNUSED)
{
  return _ctk_css_value_ref (value);
}

static gboolean
ctk_css_value_string_equal (const CtkCssValue *value1,
                            const CtkCssValue *value2)
{
  return g_strcmp0 (value1->string, value2->string) == 0;
}

static CtkCssValue *
ctk_css_value_string_transition (CtkCssValue *start G_GNUC_UNUSED,
                                 CtkCssValue *end G_GNUC_UNUSED,
                                 guint        property_id G_GNUC_UNUSED,
                                 double       progress G_GNUC_UNUSED)
{
  return NULL;
}

static void
ctk_css_value_string_print (const CtkCssValue *value,
                            GString           *str)
{
  if (value->string == NULL)
    {
      g_string_append (str, "none");
      return;
    }

  _ctk_css_print_string (str, value->string);
}

static void
ctk_css_value_ident_print (const CtkCssValue *value,
                            GString           *str)
{
  char *string = value->string;
  gsize len;

  do {
    len = strcspn (string, "\"\n\r\f");
    g_string_append_len (str, string, len);
    string += len;
    switch (*string)
      {
      case '\0':
        goto out;
      case '\n':
        g_string_append (str, "\\A ");
        break;
      case '\r':
        g_string_append (str, "\\D ");
        break;
      case '\f':
        g_string_append (str, "\\C ");
        break;
      case '\"':
        g_string_append (str, "\\\"");
        break;
      case '\'':
        g_string_append (str, "\\'");
        break;
      case '\\':
        g_string_append (str, "\\\\");
        break;
      default:
        g_assert_not_reached ();
        break;
      }
    string++;
  } while (*string);

out:
  ;
}

static const CtkCssValueClass CTK_CSS_VALUE_STRING = {
  ctk_css_value_string_free,
  ctk_css_value_string_compute,
  ctk_css_value_string_equal,
  ctk_css_value_string_transition,
  ctk_css_value_string_print
};

static const CtkCssValueClass CTK_CSS_VALUE_IDENT = {
  ctk_css_value_string_free,
  ctk_css_value_string_compute,
  ctk_css_value_string_equal,
  ctk_css_value_string_transition,
  ctk_css_value_ident_print
};

CtkCssValue *
_ctk_css_string_value_new (const char *string)
{
  return _ctk_css_string_value_new_take (g_strdup (string));
}

CtkCssValue *
_ctk_css_string_value_new_take (char *string)
{
  CtkCssValue *result;

  result = _ctk_css_value_new (CtkCssValue, &CTK_CSS_VALUE_STRING);
  result->string = string;

  return result;
}

CtkCssValue *
_ctk_css_string_value_parse (CtkCssParser *parser)
{
  char *s;

  g_return_val_if_fail (parser != NULL, NULL);

  s = _ctk_css_parser_read_string (parser);
  if (s == NULL)
    return NULL;
  
  return _ctk_css_string_value_new_take (s);
}

const char *
_ctk_css_string_value_get (const CtkCssValue *value)
{
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_STRING, NULL);

  return value->string;
}

CtkCssValue *
_ctk_css_ident_value_new (const char *ident)
{
  return _ctk_css_ident_value_new_take (g_strdup (ident));
}

CtkCssValue *
_ctk_css_ident_value_new_take (char *ident)
{
  CtkCssValue *result;

  result = _ctk_css_value_new (CtkCssValue, &CTK_CSS_VALUE_IDENT);
  result->string = ident;

  return result;
}

CtkCssValue *
_ctk_css_ident_value_try_parse (CtkCssParser *parser)
{
  char *ident;

  g_return_val_if_fail (parser != NULL, NULL);

  ident = _ctk_css_parser_try_ident (parser, TRUE);
  if (ident == NULL)
    return NULL;
  
  return _ctk_css_ident_value_new_take (ident);
}

const char *
_ctk_css_ident_value_get (const CtkCssValue *value)
{
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_IDENT, NULL);

  return value->string;
}

