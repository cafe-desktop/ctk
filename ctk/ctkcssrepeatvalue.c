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

#include "ctkcssrepeatvalueprivate.h"

#include "ctkcssnumbervalueprivate.h"

struct _CtkCssValue {
  CTK_CSS_VALUE_BASE
  CtkCssRepeatStyle x;
  CtkCssRepeatStyle y;
};

static void
ctk_css_value_repeat_free (CtkCssValue *value)
{
  g_slice_free (CtkCssValue, value);
}

static CtkCssValue *
ctk_css_value_repeat_compute (CtkCssValue             *value,
                              guint                    property_id G_GNUC_UNUSED,
                              CtkStyleProviderPrivate *provider G_GNUC_UNUSED,
                              CtkCssStyle             *style G_GNUC_UNUSED,
                              CtkCssStyle             *parent_style G_GNUC_UNUSED)
{
  return _ctk_css_value_ref (value);
}

static gboolean
ctk_css_value_repeat_equal (const CtkCssValue *repeat1,
                            const CtkCssValue *repeat2)
{
  return repeat1->x == repeat2->x
      && repeat1->y == repeat2->y;
}

static CtkCssValue *
ctk_css_value_repeat_transition (CtkCssValue *start G_GNUC_UNUSED,
                                 CtkCssValue *end G_GNUC_UNUSED,
                                 guint        property_id G_GNUC_UNUSED,
                                 double       progress G_GNUC_UNUSED)
{
  return NULL;
}

static void
ctk_css_value_background_repeat_print (const CtkCssValue *repeat,
                                       GString           *string)
{
  static const char *names[] = {
    "no-repeat",
    "repeat",
    "round",
    "space"
  };

  if (repeat->x == repeat->y)
    {
      g_string_append (string, names[repeat->x]);
    }
  else if (repeat->x == CTK_CSS_REPEAT_STYLE_REPEAT &&
           repeat->y == CTK_CSS_REPEAT_STYLE_NO_REPEAT)
    {
      g_string_append (string, "repeat-x");
    }
  else if (repeat->x == CTK_CSS_REPEAT_STYLE_NO_REPEAT &&
           repeat->y == CTK_CSS_REPEAT_STYLE_REPEAT)
    {
      g_string_append (string, "repeat-y");
    }
  else
    {
      g_string_append (string, names[repeat->x]);
      g_string_append_c (string, ' ');
      g_string_append (string, names[repeat->y]);
    }
}

static void
ctk_css_value_border_repeat_print (const CtkCssValue *repeat,
                                   GString           *string)
{
  static const char *names[] = {
    "stretch",
    "repeat",
    "round",
    "space"
  };

  g_string_append (string, names[repeat->x]);
  if (repeat->x != repeat->y)
    {
      g_string_append_c (string, ' ');
      g_string_append (string, names[repeat->y]);
    }
}

static const CtkCssValueClass CTK_CSS_VALUE_BACKGROUND_REPEAT = {
  ctk_css_value_repeat_free,
  ctk_css_value_repeat_compute,
  ctk_css_value_repeat_equal,
  ctk_css_value_repeat_transition,
  ctk_css_value_background_repeat_print
};

static const CtkCssValueClass CTK_CSS_VALUE_BORDER_REPEAT = {
  ctk_css_value_repeat_free,
  ctk_css_value_repeat_compute,
  ctk_css_value_repeat_equal,
  ctk_css_value_repeat_transition,
  ctk_css_value_border_repeat_print
};
/* BACKGROUND REPEAT */

static struct {
  const char *name;
  CtkCssValue values[4];
} background_repeat_values[4] = {
  { "no-repeat",
  { { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_NO_REPEAT, CTK_CSS_REPEAT_STYLE_NO_REPEAT },
    { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_NO_REPEAT, CTK_CSS_REPEAT_STYLE_REPEAT    },
    { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_NO_REPEAT, CTK_CSS_REPEAT_STYLE_ROUND     },
    { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_NO_REPEAT, CTK_CSS_REPEAT_STYLE_SPACE     }
  } },
  { "repeat",
  { { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_REPEAT,    CTK_CSS_REPEAT_STYLE_NO_REPEAT },
    { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_REPEAT,    CTK_CSS_REPEAT_STYLE_REPEAT    },
    { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_REPEAT,    CTK_CSS_REPEAT_STYLE_ROUND     },
    { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_REPEAT,    CTK_CSS_REPEAT_STYLE_SPACE     }
  } }, 
  { "round",
  { { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_ROUND,     CTK_CSS_REPEAT_STYLE_NO_REPEAT },
    { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_ROUND,     CTK_CSS_REPEAT_STYLE_REPEAT    },
    { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_ROUND,     CTK_CSS_REPEAT_STYLE_ROUND     },
    { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_ROUND,     CTK_CSS_REPEAT_STYLE_SPACE     }
  } }, 
  { "space",
  { { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_SPACE,     CTK_CSS_REPEAT_STYLE_NO_REPEAT },
    { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_SPACE,     CTK_CSS_REPEAT_STYLE_REPEAT    },
    { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_SPACE,     CTK_CSS_REPEAT_STYLE_ROUND     },
    { &CTK_CSS_VALUE_BACKGROUND_REPEAT, 1, CTK_CSS_REPEAT_STYLE_SPACE,     CTK_CSS_REPEAT_STYLE_SPACE     }
  } }
};

CtkCssValue *
_ctk_css_background_repeat_value_new (CtkCssRepeatStyle x,
                                      CtkCssRepeatStyle y)
{
  return _ctk_css_value_ref (&background_repeat_values[x].values[y]);
}

static gboolean
_ctk_css_background_repeat_style_try (CtkCssParser      *parser,
                                      CtkCssRepeatStyle *result)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (background_repeat_values); i++)
    {
      if (_ctk_css_parser_try (parser, background_repeat_values[i].name, TRUE))
        {
          *result = i;
          return TRUE;
        }
    }

  return FALSE;
}

CtkCssValue *
_ctk_css_background_repeat_value_try_parse (CtkCssParser *parser)
{
  CtkCssRepeatStyle x, y;

  g_return_val_if_fail (parser != NULL, NULL);

  if (_ctk_css_parser_try (parser, "repeat-x", TRUE))
    return _ctk_css_background_repeat_value_new (CTK_CSS_REPEAT_STYLE_REPEAT, CTK_CSS_REPEAT_STYLE_NO_REPEAT);
  if (_ctk_css_parser_try (parser, "repeat-y", TRUE))
    return _ctk_css_background_repeat_value_new (CTK_CSS_REPEAT_STYLE_NO_REPEAT, CTK_CSS_REPEAT_STYLE_REPEAT);

  if (!_ctk_css_background_repeat_style_try (parser, &x))
    return NULL;

  if (!_ctk_css_background_repeat_style_try (parser, &y))
    y = x;

  return _ctk_css_background_repeat_value_new (x, y);
}

CtkCssRepeatStyle
_ctk_css_background_repeat_value_get_x (const CtkCssValue *repeat)
{
  g_return_val_if_fail (repeat->class == &CTK_CSS_VALUE_BACKGROUND_REPEAT, CTK_CSS_REPEAT_STYLE_NO_REPEAT);

  return repeat->x;
}

CtkCssRepeatStyle
_ctk_css_background_repeat_value_get_y (const CtkCssValue *repeat)
{
  g_return_val_if_fail (repeat->class == &CTK_CSS_VALUE_BACKGROUND_REPEAT, CTK_CSS_REPEAT_STYLE_NO_REPEAT);

  return repeat->y;
}

/* BORDER IMAGE REPEAT */

static struct {
  const char *name;
  CtkCssValue values[4];
} border_repeat_values[4] = {
  { "stretch",
  { { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_STRETCH, CTK_CSS_REPEAT_STYLE_STRETCH },
    { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_STRETCH, CTK_CSS_REPEAT_STYLE_REPEAT  },
    { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_STRETCH, CTK_CSS_REPEAT_STYLE_ROUND   },
    { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_STRETCH, CTK_CSS_REPEAT_STYLE_SPACE   }
  } },
  { "repeat",
  { { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_REPEAT,  CTK_CSS_REPEAT_STYLE_STRETCH },
    { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_REPEAT,  CTK_CSS_REPEAT_STYLE_REPEAT  },
    { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_REPEAT,  CTK_CSS_REPEAT_STYLE_ROUND   },
    { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_REPEAT,  CTK_CSS_REPEAT_STYLE_SPACE   }
  } }, 
  { "round",
  { { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_ROUND,   CTK_CSS_REPEAT_STYLE_STRETCH },
    { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_ROUND,   CTK_CSS_REPEAT_STYLE_REPEAT  },
    { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_ROUND,   CTK_CSS_REPEAT_STYLE_ROUND   },
    { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_ROUND,   CTK_CSS_REPEAT_STYLE_SPACE   }
  } }, 
  { "space",
  { { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_SPACE,   CTK_CSS_REPEAT_STYLE_STRETCH },
    { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_SPACE,   CTK_CSS_REPEAT_STYLE_REPEAT  },
    { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_SPACE,   CTK_CSS_REPEAT_STYLE_ROUND   },
    { &CTK_CSS_VALUE_BORDER_REPEAT, 1, CTK_CSS_REPEAT_STYLE_SPACE,   CTK_CSS_REPEAT_STYLE_SPACE   }
  } }
};

CtkCssValue *
_ctk_css_border_repeat_value_new (CtkCssRepeatStyle x,
                                  CtkCssRepeatStyle y)
{
  return _ctk_css_value_ref (&border_repeat_values[x].values[y]);
}

static gboolean
_ctk_css_border_repeat_style_try (CtkCssParser      *parser,
                                  CtkCssRepeatStyle *result)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (border_repeat_values); i++)
    {
      if (_ctk_css_parser_try (parser, border_repeat_values[i].name, TRUE))
        {
          *result = i;
          return TRUE;
        }
    }

  return FALSE;
}

CtkCssValue *
_ctk_css_border_repeat_value_try_parse (CtkCssParser *parser)
{
  CtkCssRepeatStyle x, y;

  g_return_val_if_fail (parser != NULL, NULL);

  if (!_ctk_css_border_repeat_style_try (parser, &x))
    return NULL;

  if (!_ctk_css_border_repeat_style_try (parser, &y))
    y = x;

  return _ctk_css_border_repeat_value_new (x, y);
}

CtkCssRepeatStyle
_ctk_css_border_repeat_value_get_x (const CtkCssValue *repeat)
{
  g_return_val_if_fail (repeat->class == &CTK_CSS_VALUE_BORDER_REPEAT, CTK_CSS_REPEAT_STYLE_STRETCH);

  return repeat->x;
}

CtkCssRepeatStyle
_ctk_css_border_repeat_value_get_y (const CtkCssValue *repeat)
{
  g_return_val_if_fail (repeat->class == &CTK_CSS_VALUE_BORDER_REPEAT, CTK_CSS_REPEAT_STYLE_STRETCH);

  return repeat->y;
}

