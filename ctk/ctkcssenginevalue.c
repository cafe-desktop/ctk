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

#define GDK_DISABLE_DEPRECATION_WARNINGS
#include "ctkcssenginevalueprivate.h"

#include "ctkstylepropertyprivate.h"

struct _CtkCssValue {
  CTK_CSS_VALUE_BASE
  CtkThemingEngine *engine;
};

static void
ctk_css_value_engine_free (CtkCssValue *value)
{
  g_object_unref (value->engine);

  g_slice_free (CtkCssValue, value);
}

static CtkCssValue *
ctk_css_value_engine_compute (CtkCssValue             *value,
                              guint                    property_id,
                              CtkStyleProviderPrivate *provider,
                              CtkCssStyle             *style,
                              CtkCssStyle             *parent_style)
{
  return _ctk_css_value_ref (value);
}

static gboolean
ctk_css_value_engine_equal (const CtkCssValue *value1,
                            const CtkCssValue *value2)
{
  return value1->engine == value2->engine;
}

static CtkCssValue *
ctk_css_value_engine_transition (CtkCssValue *start,
                                 CtkCssValue *end,
                                 guint        property_id,
                                 double       progress)
{
  return NULL;
}

static void
ctk_css_value_engine_print (const CtkCssValue *value,
                            GString           *string)
{
  char *name;

  g_object_get (value->engine, "name", &name, NULL);

  if (name)
    g_string_append (string, name);
  else
    g_string_append (string, "none");

  g_free (name);
}

static const CtkCssValueClass CTK_CSS_VALUE_ENGINE = {
  ctk_css_value_engine_free,
  ctk_css_value_engine_compute,
  ctk_css_value_engine_equal,
  ctk_css_value_engine_transition,
  ctk_css_value_engine_print
};

CtkCssValue *
_ctk_css_engine_value_new (CtkThemingEngine *engine)
{
  CtkCssValue *result;

  g_return_val_if_fail (CTK_IS_THEMING_ENGINE (engine), NULL);

  result = _ctk_css_value_new (CtkCssValue, &CTK_CSS_VALUE_ENGINE);
  result->engine = g_object_ref (engine);

  return result;
}

CtkCssValue *
_ctk_css_engine_value_parse (CtkCssParser *parser)
{
  CtkThemingEngine *engine;
  char *str;

  g_return_val_if_fail (parser != NULL, NULL);

  if (_ctk_css_parser_try (parser, "none", TRUE))
    return _ctk_css_engine_value_new (ctk_theming_engine_load (NULL));

  str = _ctk_css_parser_try_ident (parser, TRUE);
  if (str == NULL)
    {
      _ctk_css_parser_error (parser, "Expected a valid theme name");
      return NULL;
    }

  engine = ctk_theming_engine_load (str);

  if (engine == NULL)
    {
      _ctk_css_parser_error (parser, "Theming engine '%s' not found", str);
      g_free (str);
      return NULL;
    }

  g_free (str);

  return _ctk_css_engine_value_new (engine);
}

CtkThemingEngine *
_ctk_css_engine_value_get_engine (const CtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_ENGINE, NULL);

  return value->engine;
}

