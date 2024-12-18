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

#include "ctkcsswin32sizevalueprivate.h"

#include "ctkwin32drawprivate.h"
#include "ctkwin32themeprivate.h"

typedef enum {
  CTK_WIN32_SIZE,
  CTK_WIN32_PART_WIDTH,
  CTK_WIN32_PART_HEIGHT,
  CTK_WIN32_PART_BORDER_TOP,
  CTK_WIN32_PART_BORDER_RIGHT,
  CTK_WIN32_PART_BORDER_BOTTOM,
  CTK_WIN32_PART_BORDER_LEFT
} CtkWin32SizeType;

static const char *css_value_names[] = {
  "-ctk-win32-size(",
  "-ctk-win32-part-width(",
  "-ctk-win32-part-height(",
  "-ctk-win32-part-border-top(",
  "-ctk-win32-part-border-right(",
  "-ctk-win32-part-border-bottom(",
  "-ctk-win32-part-border-left("
};

struct _CtkCssValue {
  CTK_CSS_VALUE_BASE
  double                 scale;         /* needed for calc() math */
  CtkWin32Theme         *theme;
  CtkWin32SizeType       type;
  union {
    struct {
      gint               id;
    } size;
    struct {
      gint               part;
      gint               state;
    } part;
  }                      val;
};

static CtkCssValue *    ctk_css_win32_size_value_new (double            scale,
                                                      CtkWin32Theme    *theme,
                                                      CtkWin32SizeType  type);

static void
ctk_css_value_win32_size_free (CtkCssValue *value)
{
  ctk_win32_theme_unref (value->theme);
  g_slice_free (CtkCssValue, value);
}

static int
ctk_css_value_win32_compute_size (const CtkCssValue *value)
{
  CtkBorder border;
  int size;

  switch (value->type)
    {
    case CTK_WIN32_SIZE:
      size = ctk_win32_theme_get_size (value->theme, value->val.size.id);
      break;

    case CTK_WIN32_PART_WIDTH:
      ctk_win32_theme_get_part_size (value->theme, value->val.part.part, value->val.part.state, &size, NULL);
      break;

    case CTK_WIN32_PART_HEIGHT:
      ctk_win32_theme_get_part_size (value->theme, value->val.part.part, value->val.part.state, NULL, &size);
      break;

    case CTK_WIN32_PART_BORDER_TOP:
      ctk_win32_theme_get_part_border (value->theme, value->val.part.part, value->val.part.state, &border);
      size = border.top;
      break;

    case CTK_WIN32_PART_BORDER_RIGHT:
      ctk_win32_theme_get_part_border (value->theme, value->val.part.part, value->val.part.state, &border);
      size = border.right;
      break;

    case CTK_WIN32_PART_BORDER_BOTTOM:
      ctk_win32_theme_get_part_border (value->theme, value->val.part.part, value->val.part.state, &border);
      size = border.bottom;
      break;

    case CTK_WIN32_PART_BORDER_LEFT:
      ctk_win32_theme_get_part_border (value->theme, value->val.part.part, value->val.part.state, &border);
      size = border.left;
      break;

    default:
      g_assert_not_reached ();
      return 0;
    }

  return size;
}

static CtkCssValue *
ctk_css_value_win32_size_compute (CtkCssValue             *value,
                                  guint                    property_id G_GNUC_UNUSED,
                                  CtkStyleProviderPrivate *provider G_GNUC_UNUSED,
                                  CtkCssStyle             *style G_GNUC_UNUSED,
                                  CtkCssStyle             *parent_style G_GNUC_UNUSED)
{
  return _ctk_css_number_value_new (value->scale * ctk_css_value_win32_compute_size (value), CTK_CSS_PX);
}

static gboolean
ctk_css_value_win32_size_equal (const CtkCssValue *value1,
                                const CtkCssValue *value2)
{
  if (value1->type != value2->type ||
      !ctk_win32_theme_equal (value1->theme, value2->theme) )
    return FALSE;

  switch (value1->type)
    {
    case CTK_WIN32_SIZE:
      return value1->val.size.id == value2->val.size.id;

    case CTK_WIN32_PART_WIDTH:
    case CTK_WIN32_PART_HEIGHT:
      return value1->val.part.part == value2->val.part.part
          && value1->val.part.state == value2->val.part.state;

    default:
      g_assert_not_reached ();
      return FALSE;
    }
}

static void
ctk_css_value_win32_size_print (const CtkCssValue *value,
                                GString           *string)
{
  if (value->scale != 1.0)
    {
      g_string_append_printf (string, "%g * ", value->scale);
    }
  g_string_append (string, css_value_names[value->type]);
  ctk_win32_theme_print (value->theme, string);

  switch (value->type)
    {
    case CTK_WIN32_SIZE:
      {
        const char *name = ctk_win32_get_sys_metric_name_for_id (value->val.size.id);
        if (name)
          g_string_append (string, name);
        else
          g_string_append_printf (string, ", %d", value->val.size.id);
      }
      break;

    case CTK_WIN32_PART_WIDTH:
    case CTK_WIN32_PART_HEIGHT:
    case CTK_WIN32_PART_BORDER_TOP:
    case CTK_WIN32_PART_BORDER_RIGHT:
    case CTK_WIN32_PART_BORDER_BOTTOM:
    case CTK_WIN32_PART_BORDER_LEFT:
      g_string_append_printf (string, ", %d, %d", value->val.part.part, value->val.part.state);
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  g_string_append (string, ")");
}

static double
ctk_css_value_win32_size_get (const CtkCssValue *value,
                              double             one_hundred_percent G_GNUC_UNUSED)
{
  return value->scale * ctk_css_value_win32_compute_size (value);
}

static CtkCssDimension
ctk_css_value_win32_size_get_dimension (const CtkCssValue *value G_GNUC_UNUSED)
{
  return CTK_CSS_DIMENSION_LENGTH;
}

static gboolean
ctk_css_value_win32_size_has_percent (const CtkCssValue *value G_GNUC_UNUSED)
{
  return FALSE;
}

static CtkCssValue *
ctk_css_value_win32_size_multiply (const CtkCssValue *value,
                                   double             factor)
{
  CtkCssValue *result;

  result = ctk_css_win32_size_value_new (value->scale * factor, value->theme, value->type);
  result->val = value->val;

  return result;
}

static CtkCssValue *
ctk_css_value_win32_size_try_add (const CtkCssValue *value1,
                                  const CtkCssValue *value2)
{
  CtkCssValue *result;

  if (!ctk_css_value_win32_size_equal (value1, value2))
    return NULL;

  result = ctk_css_win32_size_value_new (value1->scale + value2->scale, value1->theme, value1->type);
  result->val = value1->val;

  return result;
}

static gint
ctk_css_value_win32_size_get_calc_term_order (const CtkCssValue *value)
{
  return 2000 + 100 * value->type;
}

static const CtkCssNumberValueClass CTK_CSS_VALUE_WIN32_SIZE = {
  {
    ctk_css_value_win32_size_free,
    ctk_css_value_win32_size_compute,
    ctk_css_value_win32_size_equal,
    ctk_css_number_value_transition,
    ctk_css_value_win32_size_print
  },
  ctk_css_value_win32_size_get,
  ctk_css_value_win32_size_get_dimension,
  ctk_css_value_win32_size_has_percent,
  ctk_css_value_win32_size_multiply,
  ctk_css_value_win32_size_try_add,
  ctk_css_value_win32_size_get_calc_term_order
};

static CtkCssValue *
ctk_css_win32_size_value_new (double            scale,
                              CtkWin32Theme    *theme,
                              CtkWin32SizeType  type)
{
  CtkCssValue *result;

  result = _ctk_css_value_new (CtkCssValue, &CTK_CSS_VALUE_WIN32_SIZE.value_class);
  result->scale = scale;
  result->theme = ctk_win32_theme_ref (theme);
  result->type = type;

  return result;
}

static CtkCssValue *
ctk_css_win32_size_value_parse_size (CtkCssValue *value,
                                     CtkCssParser *parser)
{
  char *name;

  name = _ctk_css_parser_try_ident (parser, TRUE);
  if (name)
    {
      value->val.size.id = ctk_win32_get_sys_metric_id_for_name (name);
      if (value->val.size.id == -1)
        {
          _ctk_css_parser_error (parser, "'%s' is not a name for a win32 metric.", name);
          _ctk_css_value_unref (value);
          g_free (name);
          return NULL;
        }
      g_free (name);
    }
  else if (!_ctk_css_parser_try_int (parser, &value->val.size.id))
    {
      _ctk_css_value_unref (value);
      _ctk_css_parser_error (parser, "Expected an integer ID");
      return NULL;
    }

  return value;
}

static CtkCssValue *
ctk_css_win32_size_value_parse_part_size (CtkCssValue *value,
                                          CtkCssParser *parser)
{
  if (!_ctk_css_parser_try_int (parser, &value->val.part.part))
    {
      _ctk_css_value_unref (value);
      _ctk_css_parser_error (parser, "Expected an integer part ID");
      return NULL;
    }

  if (! _ctk_css_parser_try (parser, ",", TRUE))
    {
      _ctk_css_value_unref (value);
      _ctk_css_parser_error (parser, "Expected ','");
      return NULL;
    }

  if (!_ctk_css_parser_try_int (parser, &value->val.part.state))
    {
      _ctk_css_value_unref (value);
      _ctk_css_parser_error (parser, "Expected an integer state ID");
      return NULL;
    }

  return value;
}

CtkCssValue *
ctk_css_win32_size_value_parse (CtkCssParser           *parser,
                                CtkCssNumberParseFlags  flags G_GNUC_UNUSED)
{
  CtkWin32Theme *theme;
  CtkCssValue *result;
  guint type;

  for (type = 0; type < G_N_ELEMENTS(css_value_names); type++)
    {
      if (_ctk_css_parser_try (parser, css_value_names[type], TRUE))
        break;
    }

  if (type >= G_N_ELEMENTS(css_value_names))
    {
      _ctk_css_parser_error (parser, "Not a win32 size value");
      return NULL;
    }

  theme = ctk_win32_theme_parse (parser);
  if (theme == NULL)
    return NULL;

  result = ctk_css_win32_size_value_new (1.0, theme, type);
  ctk_win32_theme_unref (theme);

  if (! _ctk_css_parser_try (parser, ",", TRUE))
    {
      _ctk_css_value_unref (result);
      _ctk_css_parser_error (parser, "Expected ','");
      return NULL;
    }

  switch (result->type)
    {
    case CTK_WIN32_SIZE:
      result = ctk_css_win32_size_value_parse_size (result, parser);
      break;

    case CTK_WIN32_PART_WIDTH:
    case CTK_WIN32_PART_HEIGHT:
    case CTK_WIN32_PART_BORDER_TOP:
    case CTK_WIN32_PART_BORDER_RIGHT:
    case CTK_WIN32_PART_BORDER_BOTTOM:
    case CTK_WIN32_PART_BORDER_LEFT:
      result = ctk_css_win32_size_value_parse_part_size (result, parser);
      break;

    default:
      g_assert_not_reached ();
      _ctk_css_value_unref (result);
      result = NULL;
      break;
    }

  if (result == NULL)
    return NULL;

  if (!_ctk_css_parser_try (parser, ")", TRUE))
    {
      _ctk_css_value_unref (result);
      _ctk_css_parser_error (parser, "Expected ')'");
      return NULL;
    }

  return result;
}
