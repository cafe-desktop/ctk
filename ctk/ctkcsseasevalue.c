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

#include "ctkcsseasevalueprivate.h"

#include <math.h>

typedef enum {
  CTK_CSS_EASE_CUBIC_BEZIER,
  CTK_CSS_EASE_STEPS
} CtkCssEaseType;

struct _CtkCssValue {
  CTK_CSS_VALUE_BASE
  CtkCssEaseType type;
  union {
    struct {
      double x1;
      double y1;
      double x2;
      double y2;
    } cubic;
    struct {
      guint steps;
      gboolean start;
    } steps;
  } u;
};

static void
ctk_css_value_ease_free (CtkCssValue *value)
{
  g_slice_free (CtkCssValue, value);
}

static CtkCssValue *
ctk_css_value_ease_compute (CtkCssValue             *value,
                            guint                    property_id,
                            CtkStyleProviderPrivate *provider,
                            CtkCssStyle             *style,
                            CtkCssStyle             *parent_style)
{
  return _ctk_css_value_ref (value);
}

static gboolean
ctk_css_value_ease_equal (const CtkCssValue *ease1,
                          const CtkCssValue *ease2)
{
  if (ease1->type != ease2->type)
    return FALSE;
  
  switch (ease1->type)
    {
    case CTK_CSS_EASE_CUBIC_BEZIER:
      return ease1->u.cubic.x1 == ease2->u.cubic.x1 &&
             ease1->u.cubic.y1 == ease2->u.cubic.y1 &&
             ease1->u.cubic.x2 == ease2->u.cubic.x2 &&
             ease1->u.cubic.y2 == ease2->u.cubic.y2;
    case CTK_CSS_EASE_STEPS:
      return ease1->u.steps.steps == ease2->u.steps.steps &&
             ease1->u.steps.start == ease2->u.steps.start;
    default:
      g_assert_not_reached ();
      return FALSE;
    }
}

static CtkCssValue *
ctk_css_value_ease_transition (CtkCssValue *start,
                               CtkCssValue *end,
                               guint        property_id,
                               double       progress)
{
  return NULL;
}

static void
ctk_css_value_ease_print (const CtkCssValue *ease,
                          GString           *string)
{
  switch (ease->type)
    {
    case CTK_CSS_EASE_CUBIC_BEZIER:
      if (ease->u.cubic.x1 == 0.25 && ease->u.cubic.y1 == 0.1 &&
          ease->u.cubic.x2 == 0.25 && ease->u.cubic.y2 == 1.0)
        g_string_append (string, "ease");
      else if (ease->u.cubic.x1 == 0.0 && ease->u.cubic.y1 == 0.0 &&
               ease->u.cubic.x2 == 1.0 && ease->u.cubic.y2 == 1.0)
        g_string_append (string, "linear");
      else if (ease->u.cubic.x1 == 0.42 && ease->u.cubic.y1 == 0.0 &&
               ease->u.cubic.x2 == 1.0  && ease->u.cubic.y2 == 1.0)
        g_string_append (string, "ease-in");
      else if (ease->u.cubic.x1 == 0.0  && ease->u.cubic.y1 == 0.0 &&
               ease->u.cubic.x2 == 0.58 && ease->u.cubic.y2 == 1.0)
        g_string_append (string, "ease-out");
      else if (ease->u.cubic.x1 == 0.42 && ease->u.cubic.y1 == 0.0 &&
               ease->u.cubic.x2 == 0.58 && ease->u.cubic.y2 == 1.0)
        g_string_append (string, "ease-in-out");
      else
        g_string_append_printf (string, "cubic-bezier(%g,%g,%g,%g)", 
                                ease->u.cubic.x1, ease->u.cubic.y1,
                                ease->u.cubic.x2, ease->u.cubic.y2);
      break;
    case CTK_CSS_EASE_STEPS:
      if (ease->u.steps.steps == 1)
        {
          g_string_append (string, ease->u.steps.start ? "step-start" : "step-end");
        }
      else
        {
          g_string_append_printf (string, "steps(%u%s)", ease->u.steps.steps, ease->u.steps.start ? ",start" : "");
        }
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

static const CtkCssValueClass CTK_CSS_VALUE_EASE = {
  ctk_css_value_ease_free,
  ctk_css_value_ease_compute,
  ctk_css_value_ease_equal,
  ctk_css_value_ease_transition,
  ctk_css_value_ease_print
};

CtkCssValue *
_ctk_css_ease_value_new_cubic_bezier (double x1,
                                      double y1,
                                      double x2,
                                      double y2)
{
  CtkCssValue *value;

  g_return_val_if_fail (x1 >= 0.0, NULL);
  g_return_val_if_fail (x1 <= 1.0, NULL);
  g_return_val_if_fail (x2 >= 0.0, NULL);
  g_return_val_if_fail (x2 <= 1.0, NULL);

  value = _ctk_css_value_new (CtkCssValue, &CTK_CSS_VALUE_EASE);
  
  value->type = CTK_CSS_EASE_CUBIC_BEZIER;
  value->u.cubic.x1 = x1;
  value->u.cubic.y1 = y1;
  value->u.cubic.x2 = x2;
  value->u.cubic.y2 = y2;

  return value;
}

static CtkCssValue *
_ctk_css_ease_value_new_steps (guint n_steps,
                               gboolean start)
{
  CtkCssValue *value;

  g_return_val_if_fail (n_steps > 0, NULL);

  value = _ctk_css_value_new (CtkCssValue, &CTK_CSS_VALUE_EASE);
  
  value->type = CTK_CSS_EASE_STEPS;
  value->u.steps.steps = n_steps;
  value->u.steps.start = start;

  return value;
}

static const struct {
  const char *name;
  guint is_bezier :1;
  guint needs_custom :1;
  double values[4];
} parser_values[] = {
  { "linear",       TRUE,  FALSE, { 0.0,  0.0, 1.0,  1.0 } },
  { "ease-in-out",  TRUE,  FALSE, { 0.42, 0.0, 0.58, 1.0 } },
  { "ease-in",      TRUE,  FALSE, { 0.42, 0.0, 1.0,  1.0 } },
  { "ease-out",     TRUE,  FALSE, { 0.0,  0.0, 0.58, 1.0 } },
  { "ease",         TRUE,  FALSE, { 0.25, 0.1, 0.25, 1.0 } },
  { "step-start",   FALSE, FALSE, { 1.0,  1.0, 0.0,  0.0 } },
  { "step-end",     FALSE, FALSE, { 1.0,  0.0, 0.0,  0.0 } },
  { "steps",        FALSE, TRUE,  { 0.0,  0.0, 0.0,  0.0 } },
  { "cubic-bezier", TRUE,  TRUE,  { 0.0,  0.0, 0.0,  0.0 } }
};

gboolean
_ctk_css_ease_value_can_parse (CtkCssParser *parser)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (parser_values); i++)
    {
      if (_ctk_css_parser_has_prefix (parser, parser_values[i].name))
        return TRUE;
    }

  return FALSE;
}

static CtkCssValue *
ctk_css_ease_value_parse_cubic_bezier (CtkCssParser *parser)
{
  double values[4];
  guint i;

  for (i = 0; i < 4; i++)
    {
      if (!_ctk_css_parser_try (parser, i ? "," : "(", TRUE))
        {
          _ctk_css_parser_error (parser, "Expected '%s'", i ? "," : "(");
          return NULL;
        }
      if (!_ctk_css_parser_try_double (parser, &values[i]))
        {
          _ctk_css_parser_error (parser, "Expected a number");
          return NULL;
        }
      if ((i == 0 || i == 2) &&
          (values[i] < 0 || values[i] > 1.0))
        {
          _ctk_css_parser_error (parser, "value %g out of range. Must be from 0.0 to 1.0", values[i]);
          return NULL;
        }
    }

  if (!_ctk_css_parser_try (parser, ")", TRUE))
    {
      _ctk_css_parser_error (parser, "Missing closing ')' for cubic-bezier");
      return NULL;
    }

  return _ctk_css_ease_value_new_cubic_bezier (values[0], values[1], values[2], values[3]);
}

static CtkCssValue *
ctk_css_ease_value_parse_steps (CtkCssParser *parser)
{
  guint n_steps;
  gboolean start;

  if (!_ctk_css_parser_try (parser, "(", TRUE))
    {
      _ctk_css_parser_error (parser, "Expected '('");
      return NULL;
    }

  if (!_ctk_css_parser_try_uint (parser, &n_steps))
    {
      _ctk_css_parser_error (parser, "Expected number of steps");
      return NULL;
    }

  if (_ctk_css_parser_try (parser, ",", TRUE))
    {
      if (_ctk_css_parser_try (parser, "start", TRUE))
        start = TRUE;
      else if (_ctk_css_parser_try (parser, "end", TRUE))
        start = FALSE;
      else
        {
          _ctk_css_parser_error (parser, "Only allowed values are 'start' and 'end'");
          return NULL;
        }
    }
  else
    start = FALSE;

  if (!_ctk_css_parser_try (parser, ")", TRUE))
    {
      _ctk_css_parser_error (parser, "Missing closing ')' for steps");
      return NULL;
    }

  return _ctk_css_ease_value_new_steps (n_steps, start);
}

CtkCssValue *
_ctk_css_ease_value_parse (CtkCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (parser_values); i++)
    {
      if (_ctk_css_parser_try (parser, parser_values[i].name, FALSE))
        {
          if (parser_values[i].needs_custom)
            {
              if (parser_values[i].is_bezier)
                return ctk_css_ease_value_parse_cubic_bezier (parser);
              else
                return ctk_css_ease_value_parse_steps (parser);
            }

          _ctk_css_parser_skip_whitespace (parser);

          if (parser_values[i].is_bezier)
            return _ctk_css_ease_value_new_cubic_bezier (parser_values[i].values[0],
                                                         parser_values[i].values[1],
                                                         parser_values[i].values[2],
                                                         parser_values[i].values[3]);
          else
            return _ctk_css_ease_value_new_steps (parser_values[i].values[0],
                                                  parser_values[i].values[1] != 0.0);
        }
    }

  _ctk_css_parser_error (parser, "Unknown value");
  return NULL;
}

double
_ctk_css_ease_value_transform (const CtkCssValue *ease,
                               double             progress)
{
  g_return_val_if_fail (ease->class == &CTK_CSS_VALUE_EASE, 1.0);

  if (progress <= 0)
    return 0;
  if (progress >= 1)
    return 1;

  switch (ease->type)
    {
    case CTK_CSS_EASE_CUBIC_BEZIER:
      {
        static const double epsilon = 0.00001;
        double tmin, t, tmax;

        tmin = 0.0;
        tmax = 1.0;
        t = progress;

        while (tmin < tmax)
          {
             double sample;
             sample = (((1.0 + 3 * ease->u.cubic.x1 - 3 * ease->u.cubic.x2) * t
                       +      -6 * ease->u.cubic.x1 + 3 * ease->u.cubic.x2) * t
                       +       3 * ease->u.cubic.x1                       ) * t;
             if (fabs(sample - progress) < epsilon)
               break;

             if (progress > sample)
               tmin = t;
             else
               tmax = t;
             t = (tmax + tmin) * .5;
          }

        return (((1.0 + 3 * ease->u.cubic.y1 - 3 * ease->u.cubic.y2) * t
                +      -6 * ease->u.cubic.y1 + 3 * ease->u.cubic.y2) * t
                +       3 * ease->u.cubic.y1                       ) * t;
      }
    case CTK_CSS_EASE_STEPS:
      progress *= ease->u.steps.steps;
      progress = floor (progress) + (ease->u.steps.start ? 0 : 1);
      return progress / ease->u.steps.steps;
    default:
      g_assert_not_reached ();
      return 1.0;
    }
}


