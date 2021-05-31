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

#include "gtkcssarrayvalueprivate.h"
#include "gtkcssimagevalueprivate.h"
#include "gtkcssstylepropertyprivate.h"

#include <string.h>

struct _GtkCssValue {
  CTK_CSS_VALUE_BASE
  guint         n_values;
  GtkCssValue  *values[1];
};

static void
ctk_css_value_array_free (GtkCssValue *value)
{
  guint i;

  for (i = 0; i < value->n_values; i++)
    {
      _ctk_css_value_unref (value->values[i]);
    }

  g_slice_free1 (sizeof (GtkCssValue) + sizeof (GtkCssValue *) * (value->n_values - 1), value);
}

static GtkCssValue *
ctk_css_value_array_compute (GtkCssValue             *value,
                             guint                    property_id,
                             GtkStyleProviderPrivate *provider,
                             GtkCssStyle             *style,
                             GtkCssStyle             *parent_style)
{
  GtkCssValue *result;
  GtkCssValue *i_value;
  guint i, j;

  result = NULL;
  for (i = 0; i < value->n_values; i++)
    {
      i_value =  _ctk_css_value_compute (value->values[i], property_id, provider, style, parent_style);

      if (result == NULL &&
	  i_value != value->values[i])
	{
	  result = _ctk_css_array_value_new_from_array (value->values, value->n_values);
	  for (j = 0; j < i; j++)
	    _ctk_css_value_ref (result->values[j]);
	}

      if (result != NULL)
	result->values[i] = i_value;
      else
	_ctk_css_value_unref (i_value);
    }

  if (result == NULL)
    return _ctk_css_value_ref (value);

  return result;
}

static gboolean
ctk_css_value_array_equal (const GtkCssValue *value1,
                           const GtkCssValue *value2)
{
  guint i;

  if (value1->n_values != value2->n_values)
    return FALSE;

  for (i = 0; i < value1->n_values; i++)
    {
      if (!_ctk_css_value_equal (value1->values[i],
                                 value2->values[i]))
        return FALSE;
    }

  return TRUE;
}

static guint
gcd (guint a, guint b)
{
  while (b != 0)
    {
      guint t = b;
      b = a % b;
      a = t;
    }
  return a;
}

static guint
lcm (guint a, guint b)
{
  return a / gcd (a, b) * b;
}

static GtkCssValue *
ctk_css_value_array_transition_repeat (GtkCssValue *start,
                                       GtkCssValue *end,
                                       guint        property_id,
                                       double       progress)
{
  GtkCssValue **transitions;
  guint i, n;

  n = lcm (start->n_values, end->n_values);
  transitions = g_newa (GtkCssValue *, n);

  for (i = 0; i < n; i++)
    {
      transitions[i] = _ctk_css_value_transition (start->values[i % start->n_values],
                                                  end->values[i % end->n_values],
                                                  property_id,
                                                  progress);
      if (transitions[i] == NULL)
        {
          while (i--)
            _ctk_css_value_unref (transitions[i]);
          return NULL;
        }
    }

  return _ctk_css_array_value_new_from_array (transitions, n);
}

static GtkCssValue *
ctk_css_array_value_create_default_transition_value (guint property_id)
{
  switch (property_id)
    {
    case CTK_CSS_PROPERTY_BACKGROUND_IMAGE:
      return _ctk_css_image_value_new (NULL);
    default:
      g_return_val_if_reached (NULL);
    }
}

static GtkCssValue *
ctk_css_value_array_transition_extend (GtkCssValue *start,
                                       GtkCssValue *end,
                                       guint        property_id,
                                       double       progress)
{
  GtkCssValue **transitions;
  guint i, n;

  n = MAX (start->n_values, end->n_values);
  transitions = g_newa (GtkCssValue *, n);

  for (i = 0; i < MIN (start->n_values, end->n_values); i++)
    {
      transitions[i] = _ctk_css_value_transition (start->values[i],
                                                  end->values[i],
                                                  property_id,
                                                  progress);
      if (transitions[i] == NULL)
        {
          while (i--)
            _ctk_css_value_unref (transitions[i]);
          return NULL;
        }
    }

  if (start->n_values != end->n_values)
    {
      GtkCssValue *default_value;

      default_value = ctk_css_array_value_create_default_transition_value (property_id);

      for (; i < start->n_values; i++)
        {
          transitions[i] = _ctk_css_value_transition (start->values[i],
                                                      default_value,
                                                      property_id,
                                                      progress);
          if (transitions[i] == NULL)
            {
              while (i--)
                _ctk_css_value_unref (transitions[i]);
              return NULL;
            }
        }

      for (; i < end->n_values; i++)
        {
          transitions[i] = _ctk_css_value_transition (default_value,
                                                      end->values[i],
                                                      property_id,
                                                      progress);
          if (transitions[i] == NULL)
            {
              while (i--)
                _ctk_css_value_unref (transitions[i]);
              return NULL;
            }
        }

    }

  g_assert (i == n);

  return _ctk_css_array_value_new_from_array (transitions, n);
}

static GtkCssValue *
ctk_css_value_array_transition (GtkCssValue *start,
                                GtkCssValue *end,
                                guint        property_id,
                                double       progress)
{
  switch (property_id)
    {
    case CTK_CSS_PROPERTY_BACKGROUND_CLIP:
    case CTK_CSS_PROPERTY_BACKGROUND_ORIGIN:
    case CTK_CSS_PROPERTY_BACKGROUND_SIZE:
    case CTK_CSS_PROPERTY_BACKGROUND_POSITION:
    case CTK_CSS_PROPERTY_BACKGROUND_REPEAT:
      return ctk_css_value_array_transition_repeat (start, end, property_id, progress);
    case CTK_CSS_PROPERTY_BACKGROUND_IMAGE:
      return ctk_css_value_array_transition_extend (start, end, property_id, progress);
    case CTK_CSS_PROPERTY_COLOR:
    case CTK_CSS_PROPERTY_FONT_SIZE:
    case CTK_CSS_PROPERTY_BACKGROUND_COLOR:
    case CTK_CSS_PROPERTY_FONT_FAMILY:
    case CTK_CSS_PROPERTY_FONT_STYLE:
    case CTK_CSS_PROPERTY_FONT_VARIANT:
    case CTK_CSS_PROPERTY_FONT_WEIGHT:
    case CTK_CSS_PROPERTY_TEXT_SHADOW:
    case CTK_CSS_PROPERTY_ICON_SHADOW:
    case CTK_CSS_PROPERTY_BOX_SHADOW:
    case CTK_CSS_PROPERTY_MARGIN_TOP:
    case CTK_CSS_PROPERTY_MARGIN_LEFT:
    case CTK_CSS_PROPERTY_MARGIN_BOTTOM:
    case CTK_CSS_PROPERTY_MARGIN_RIGHT:
    case CTK_CSS_PROPERTY_PADDING_TOP:
    case CTK_CSS_PROPERTY_PADDING_LEFT:
    case CTK_CSS_PROPERTY_PADDING_BOTTOM:
    case CTK_CSS_PROPERTY_PADDING_RIGHT:
    case CTK_CSS_PROPERTY_BORDER_TOP_STYLE:
    case CTK_CSS_PROPERTY_BORDER_TOP_WIDTH:
    case CTK_CSS_PROPERTY_BORDER_LEFT_STYLE:
    case CTK_CSS_PROPERTY_BORDER_LEFT_WIDTH:
    case CTK_CSS_PROPERTY_BORDER_BOTTOM_STYLE:
    case CTK_CSS_PROPERTY_BORDER_BOTTOM_WIDTH:
    case CTK_CSS_PROPERTY_BORDER_RIGHT_STYLE:
    case CTK_CSS_PROPERTY_BORDER_RIGHT_WIDTH:
    case CTK_CSS_PROPERTY_BORDER_TOP_LEFT_RADIUS:
    case CTK_CSS_PROPERTY_BORDER_TOP_RIGHT_RADIUS:
    case CTK_CSS_PROPERTY_BORDER_BOTTOM_RIGHT_RADIUS:
    case CTK_CSS_PROPERTY_BORDER_BOTTOM_LEFT_RADIUS:
    case CTK_CSS_PROPERTY_OUTLINE_STYLE:
    case CTK_CSS_PROPERTY_OUTLINE_WIDTH:
    case CTK_CSS_PROPERTY_OUTLINE_OFFSET:
    case CTK_CSS_PROPERTY_OUTLINE_TOP_LEFT_RADIUS:
    case CTK_CSS_PROPERTY_OUTLINE_TOP_RIGHT_RADIUS:
    case CTK_CSS_PROPERTY_OUTLINE_BOTTOM_RIGHT_RADIUS:
    case CTK_CSS_PROPERTY_OUTLINE_BOTTOM_LEFT_RADIUS:
    case CTK_CSS_PROPERTY_BORDER_TOP_COLOR:
    case CTK_CSS_PROPERTY_BORDER_RIGHT_COLOR:
    case CTK_CSS_PROPERTY_BORDER_BOTTOM_COLOR:
    case CTK_CSS_PROPERTY_BORDER_LEFT_COLOR:
    case CTK_CSS_PROPERTY_OUTLINE_COLOR:
    case CTK_CSS_PROPERTY_BORDER_IMAGE_SOURCE:
    case CTK_CSS_PROPERTY_BORDER_IMAGE_REPEAT:
    case CTK_CSS_PROPERTY_BORDER_IMAGE_SLICE:
    case CTK_CSS_PROPERTY_BORDER_IMAGE_WIDTH:
    case CTK_CSS_PROPERTY_ENGINE:
    default:
      /* keep all values that are not arrays here, so we get a warning if we ever turn them
       * into arrays and start animating them. */
      g_warning ("Don't know how to transition arrays for property '%s'", 
                 _ctk_style_property_get_name (CTK_STYLE_PROPERTY (_ctk_css_style_property_lookup_by_id (property_id))));
    case CTK_CSS_PROPERTY_TRANSITION_PROPERTY:
    case CTK_CSS_PROPERTY_TRANSITION_DURATION:
    case CTK_CSS_PROPERTY_TRANSITION_TIMING_FUNCTION:
    case CTK_CSS_PROPERTY_TRANSITION_DELAY:
    case CTK_CSS_PROPERTY_CTK_KEY_BINDINGS:
      return NULL;
    }
}

static void
ctk_css_value_array_print (const GtkCssValue *value,
                           GString           *string)
{
  guint i;

  if (value->n_values == 0)
    {
      g_string_append (string, "none");
      return;
    }

  for (i = 0; i < value->n_values; i++)
    {
      if (i > 0)
        g_string_append (string, ", ");
      _ctk_css_value_print (value->values[i], string);
    }
}

static const GtkCssValueClass CTK_CSS_VALUE_ARRAY = {
  ctk_css_value_array_free,
  ctk_css_value_array_compute,
  ctk_css_value_array_equal,
  ctk_css_value_array_transition,
  ctk_css_value_array_print
};

GtkCssValue *
_ctk_css_array_value_new (GtkCssValue *content)
{
  g_return_val_if_fail (content != NULL, NULL);

  return _ctk_css_array_value_new_from_array (&content, 1);
}

GtkCssValue *
_ctk_css_array_value_new_from_array (GtkCssValue **values,
                                     guint         n_values)
{
  GtkCssValue *result;
           
  g_return_val_if_fail (values != NULL, NULL);
  g_return_val_if_fail (n_values > 0, NULL);
         
  result = _ctk_css_value_alloc (&CTK_CSS_VALUE_ARRAY, sizeof (GtkCssValue) + sizeof (GtkCssValue *) * (n_values - 1));
  result->n_values = n_values;
  memcpy (&result->values[0], values, sizeof (GtkCssValue *) * n_values);
            
  return result;
}

GtkCssValue *
_ctk_css_array_value_parse (GtkCssParser *parser,
                            GtkCssValue  *(* parse_func) (GtkCssParser *parser))
{
  GtkCssValue *value, *result;
  GPtrArray *values;

  values = g_ptr_array_new ();

  do {
    value = parse_func (parser);

    if (value == NULL)
      {
        g_ptr_array_set_free_func (values, (GDestroyNotify) _ctk_css_value_unref);
        g_ptr_array_free (values, TRUE);
        return NULL;
      }

    g_ptr_array_add (values, value);
  } while (_ctk_css_parser_try (parser, ",", TRUE));

  result = _ctk_css_array_value_new_from_array ((GtkCssValue **) values->pdata, values->len);
  g_ptr_array_free (values, TRUE);
  return result;
}

GtkCssValue *
_ctk_css_array_value_get_nth (const GtkCssValue *value,
                              guint              i)
{
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_ARRAY, NULL);
  g_return_val_if_fail (value->n_values > 0, NULL);

  return value->values[i % value->n_values];
}

guint
_ctk_css_array_value_get_n_values (const GtkCssValue *value)
{
  g_return_val_if_fail (value != NULL, 0);
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_ARRAY, 0);

  return value->n_values;
}
