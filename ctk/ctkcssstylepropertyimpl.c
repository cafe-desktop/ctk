/* CTK - The GIMP Toolkit
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#include "ctkstylepropertyprivate.h"

#include <gobject/gvaluecollector.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo-gobject.h>
#include <math.h>

#include "ctkcssparserprivate.h"
#include "ctkcssstylefuncsprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkcsstypesprivate.h"
#include "ctkintl.h"
#include "ctkprivatetypebuiltins.h"

/* this is in case round() is not provided by the compiler, 
 * such as in the case of C89 compilers, like MSVC
 */
#include "fallback-c89.c"

/* the actual parsers we have */
#include "ctkbindings.h"
#include "ctkcssarrayvalueprivate.h"
#include "ctkcssbgsizevalueprivate.h"
#include "ctkcssbordervalueprivate.h"
#include "ctkcsscolorvalueprivate.h"
#include "ctkcsscornervalueprivate.h"
#include "ctkcsseasevalueprivate.h"
#include "ctkcssenginevalueprivate.h"
#include "ctkcssiconthemevalueprivate.h"
#include "ctkcssimageprivate.h"
#include "ctkcssimagebuiltinprivate.h"
#include "ctkcssimagegradientprivate.h"
#include "ctkcssimagevalueprivate.h"
#include "ctkcssinitialvalueprivate.h"
#include "ctkcssenumvalueprivate.h"
#include "ctkcssnumbervalueprivate.h"
#include "ctkcsspalettevalueprivate.h"
#include "ctkcsspositionvalueprivate.h"
#include "ctkcssrepeatvalueprivate.h"
#include "ctkcssrgbavalueprivate.h"
#include "ctkcssshadowsvalueprivate.h"
#include "ctkcssstringvalueprivate.h"
#include "ctkcsstransformvalueprivate.h"
#include "ctktypebuiltins.h"

#include "deprecated/ctkthemingengine.h"

/*** REGISTRATION ***/

typedef enum {
  CTK_STYLE_PROPERTY_INHERIT = (1 << 0),
  CTK_STYLE_PROPERTY_ANIMATED = (1 << 1),
} CtkStylePropertyFlags;

static void
ctk_css_style_property_register (const char *                   name,
                                 guint                          expected_id,
                                 GType                          value_type,
                                 CtkStylePropertyFlags          flags,
                                 CtkCssAffects                  affects,
                                 CtkCssStylePropertyParseFunc   parse_value,
                                 CtkCssStylePropertyQueryFunc   query_value,
                                 CtkCssStylePropertyAssignFunc  assign_value,
                                 CtkCssValue *                  initial_value)
{
  CtkCssStyleProperty *node;

  g_assert (initial_value != NULL);
  g_assert (parse_value != NULL);
  g_assert (value_type == G_TYPE_NONE || query_value != NULL);
  g_assert (assign_value == NULL || query_value != NULL);

  node = g_object_new (CTK_TYPE_CSS_STYLE_PROPERTY,
                       "value-type", value_type,
                       "affects", affects,
                       "animated", (flags & CTK_STYLE_PROPERTY_ANIMATED) ? TRUE : FALSE,
                       "inherit", (flags & CTK_STYLE_PROPERTY_INHERIT) ? TRUE : FALSE,
                       "initial-value", initial_value,
                       "name", name,
                       NULL);

  node->parse_value = parse_value;
  node->query_value = query_value;
  node->assign_value = assign_value;

  _ctk_css_value_unref (initial_value);

  g_assert (_ctk_css_style_property_get_id (node) == expected_id);
}

/*** IMPLEMENTATIONS ***/

static void
query_length_as_int (CtkCssStyleProperty *property,
                     const CtkCssValue   *css_value,
                     GValue              *value)
{
  g_value_init (value, G_TYPE_INT);
  g_value_set_int (value, round (_ctk_css_number_value_get (css_value, 100)));
}

static CtkCssValue *
assign_length_from_int (CtkCssStyleProperty *property,
                        const GValue        *value)
{
  return _ctk_css_number_value_new (g_value_get_int (value), CTK_CSS_PX);
}

static void
query_font_size (CtkCssStyleProperty *property,
                 const CtkCssValue   *css_value,
                 GValue              *value)
{
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, _ctk_css_number_value_get (css_value, 100));
}

static CtkCssValue *
assign_font_size (CtkCssStyleProperty *property,
                  const GValue        *value)
{
  return _ctk_css_number_value_new (g_value_get_double (value), CTK_CSS_PX);
}

static void
query_border (CtkCssStyleProperty *property,
              const CtkCssValue   *css_value,
              GValue              *value)
{
  CtkBorder border;

  g_value_init (value, CTK_TYPE_BORDER);
  
  border.top = round (_ctk_css_number_value_get (_ctk_css_border_value_get_top (css_value), 100));
  border.right = round (_ctk_css_number_value_get (_ctk_css_border_value_get_right (css_value), 100));
  border.bottom = round (_ctk_css_number_value_get (_ctk_css_border_value_get_bottom (css_value), 100));
  border.left = round (_ctk_css_number_value_get (_ctk_css_border_value_get_left (css_value), 100));

  g_value_set_boxed (value, &border);
}

static CtkCssValue *
assign_border (CtkCssStyleProperty *property,
               const GValue        *value)
{
  const CtkBorder *border = g_value_get_boxed (value);

  if (border == NULL)
    return _ctk_css_initial_value_new ();
  else
    return _ctk_css_border_value_new (_ctk_css_number_value_new (border->top, CTK_CSS_PX),
                                      _ctk_css_number_value_new (border->right, CTK_CSS_PX),
                                      _ctk_css_number_value_new (border->bottom, CTK_CSS_PX),
                                      _ctk_css_number_value_new (border->left, CTK_CSS_PX));
}

static CtkCssValue *
color_parse (CtkCssStyleProperty *property,
             CtkCssParser        *parser)
{
  return _ctk_css_color_value_parse (parser);
}

static void
color_query (CtkCssStyleProperty *property,
             const CtkCssValue   *css_value,
             GValue              *value)
{
  g_value_init (value, CDK_TYPE_RGBA);
  g_value_set_boxed (value, _ctk_css_rgba_value_get_rgba (css_value));
}

static CtkCssValue *
color_assign (CtkCssStyleProperty *property,
              const GValue        *value)
{
  return _ctk_css_rgba_value_new_from_rgba (g_value_get_boxed (value));
}

static CtkCssValue *
font_family_parse_one (CtkCssParser *parser)
{
  char *name;

  name = _ctk_css_parser_try_ident (parser, TRUE);
  if (name)
    {
      GString *string = g_string_new (name);
      g_free (name);
      while ((name = _ctk_css_parser_try_ident (parser, TRUE)))
        {
          g_string_append_c (string, ' ');
          g_string_append (string, name);
          g_free (name);
        }
      name = g_string_free (string, FALSE);
    }
  else 
    {
      name = _ctk_css_parser_read_string (parser);
      if (name == NULL)
        return NULL;
    }

  return _ctk_css_string_value_new_take (name);
}

CtkCssValue *
ctk_css_font_family_value_parse (CtkCssParser *parser)
{
  return _ctk_css_array_value_parse (parser, font_family_parse_one);
}

static CtkCssValue *
font_family_parse (CtkCssStyleProperty *property,
                   CtkCssParser        *parser)
{
  return ctk_css_font_family_value_parse (parser);
}

static void
font_family_query (CtkCssStyleProperty *property,
                   const CtkCssValue   *css_value,
                   GValue              *value)
{
  GPtrArray *array;
  guint i;

  array = g_ptr_array_new ();

  for (i = 0; i < _ctk_css_array_value_get_n_values (css_value); i++)
    {
      g_ptr_array_add (array, g_strdup (_ctk_css_string_value_get (_ctk_css_array_value_get_nth (css_value, i))));
    }

  /* NULL-terminate */
  g_ptr_array_add (array, NULL);

  g_value_init (value, G_TYPE_STRV);
  g_value_set_boxed (value, g_ptr_array_free (array, FALSE));
}

static CtkCssValue *
font_family_assign (CtkCssStyleProperty *property,
                    const GValue        *value)
{
  const char **names;
  CtkCssValue *result;
  GPtrArray *array;

  array = g_ptr_array_new ();

  for (names = g_value_get_boxed (value); *names; names++)
    {
      g_ptr_array_add (array, _ctk_css_string_value_new (*names));
    }

  result = _ctk_css_array_value_new_from_array ((CtkCssValue **) array->pdata, array->len);
  g_ptr_array_free (array, TRUE);
  return result;
}

static CtkCssValue *
font_style_parse (CtkCssStyleProperty *property,
                  CtkCssParser        *parser)
{
  CtkCssValue *value = _ctk_css_font_style_value_try_parse (parser);
  
  if (value == NULL)
    _ctk_css_parser_error (parser, "unknown value for property");

  return value;
}

static void
font_style_query (CtkCssStyleProperty *property,
                  const CtkCssValue   *css_value,
                  GValue              *value)
{
  g_value_init (value, PANGO_TYPE_STYLE);
  g_value_set_enum (value, _ctk_css_font_style_value_get (css_value));
}

static CtkCssValue *
font_style_assign (CtkCssStyleProperty *property,
                   const GValue        *value)
{
  return _ctk_css_font_style_value_new (g_value_get_enum (value));
}

static CtkCssValue *
font_weight_parse (CtkCssStyleProperty *property,
                   CtkCssParser        *parser)
{
  CtkCssValue *value = _ctk_css_font_weight_value_try_parse (parser);
  
  if (value == NULL)
    _ctk_css_parser_error (parser, "unknown value for property");

  return value;
}

static void
font_weight_query (CtkCssStyleProperty *property,
                   const CtkCssValue   *css_value,
                   GValue              *value)
{
  g_value_init (value, PANGO_TYPE_WEIGHT);
  g_value_set_enum (value, _ctk_css_font_weight_value_get (css_value));
}

static CtkCssValue *
font_weight_assign (CtkCssStyleProperty *property,
                    const GValue        *value)
{
  return _ctk_css_font_weight_value_new (g_value_get_enum (value));
}

static CtkCssValue *
font_variant_parse (CtkCssStyleProperty *property,
                    CtkCssParser        *parser)
{
  CtkCssValue *value = _ctk_css_font_variant_value_try_parse (parser);
  
  if (value == NULL)
    _ctk_css_parser_error (parser, "unknown value for property");

  return value;
}

static void
font_variant_query (CtkCssStyleProperty *property,
                    const CtkCssValue   *css_value,
                     GValue              *value)
{
  g_value_init (value, PANGO_TYPE_VARIANT);
  g_value_set_enum (value, _ctk_css_font_variant_value_get (css_value));
}

static CtkCssValue *
font_variant_assign (CtkCssStyleProperty *property,
                     const GValue        *value)
{
  return _ctk_css_font_variant_value_new (g_value_get_enum (value));
}

static CtkCssValue *
font_stretch_parse (CtkCssStyleProperty *property,
                    CtkCssParser        *parser)
{
  CtkCssValue *value = _ctk_css_font_stretch_value_try_parse (parser);

  if (value == NULL)
    _ctk_css_parser_error (parser, "unknown value for property");

  return value;
}

static void
font_stretch_query (CtkCssStyleProperty *property,
                    const CtkCssValue   *css_value,
                    GValue              *value)
{
  g_value_init (value, PANGO_TYPE_STRETCH);
  g_value_set_enum (value, _ctk_css_font_stretch_value_get (css_value));
}

static CtkCssValue *
font_stretch_assign (CtkCssStyleProperty *property,
                     const GValue        *value)
{
  return _ctk_css_font_stretch_value_new (g_value_get_enum (value));
}

static CtkCssValue *
parse_border_style (CtkCssStyleProperty *property,
                    CtkCssParser        *parser)
{
  CtkCssValue *value = _ctk_css_border_style_value_try_parse (parser);
  
  if (value == NULL)
    _ctk_css_parser_error (parser, "unknown value for property");

  return value;
}

static void
query_border_style (CtkCssStyleProperty *property,
                    const CtkCssValue   *css_value,
                    GValue              *value)
{
  g_value_init (value, CTK_TYPE_BORDER_STYLE);
  g_value_set_enum (value, _ctk_css_border_style_value_get (css_value));
}

static CtkCssValue *
assign_border_style (CtkCssStyleProperty *property,
                     const GValue        *value)
{
  return _ctk_css_border_style_value_new (g_value_get_enum (value));
}

static CtkCssValue *
parse_css_area_one (CtkCssParser *parser)
{
  CtkCssValue *value = _ctk_css_area_value_try_parse (parser);
  
  if (value == NULL)
    _ctk_css_parser_error (parser, "unknown value for property");

  return value;
}

static CtkCssValue *
parse_css_area (CtkCssStyleProperty *property,
                CtkCssParser        *parser)
{
  return _ctk_css_array_value_parse (parser, parse_css_area_one);
}

static CtkCssValue *
parse_one_css_direction (CtkCssParser *parser)
{
  CtkCssValue *value = _ctk_css_direction_value_try_parse (parser);
  
  if (value == NULL)
    _ctk_css_parser_error (parser, "unknown value for property");

  return value;
}

static CtkCssValue *
parse_css_direction (CtkCssStyleProperty *property,
                     CtkCssParser        *parser)
{
  return _ctk_css_array_value_parse (parser, parse_one_css_direction);
}

static CtkCssValue *
opacity_parse (CtkCssStyleProperty *property,
	       CtkCssParser        *parser)
{
  return _ctk_css_number_value_parse (parser, CTK_CSS_PARSE_NUMBER);
}

static void
opacity_query (CtkCssStyleProperty *property,
               const CtkCssValue   *css_value,
               GValue              *value)
{
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, _ctk_css_number_value_get (css_value, 100));
}

static CtkCssValue *
parse_font_feature_settings (CtkCssStyleProperty *property,
                      CtkCssParser        *parser)
{
  return _ctk_css_string_value_parse (parser);
}

static CtkCssValue *
parse_one_css_play_state (CtkCssParser *parser)
{
  CtkCssValue *value = _ctk_css_play_state_value_try_parse (parser);
  
  if (value == NULL)
    _ctk_css_parser_error (parser, "unknown value for property");

  return value;
}

static CtkCssValue *
parse_css_play_state (CtkCssStyleProperty *property,
                      CtkCssParser        *parser)
{
  return _ctk_css_array_value_parse (parser, parse_one_css_play_state);
}

static CtkCssValue *
parse_one_css_fill_mode (CtkCssParser *parser)
{
  CtkCssValue *value = _ctk_css_fill_mode_value_try_parse (parser);
  
  if (value == NULL)
    _ctk_css_parser_error (parser, "unknown value for property");

  return value;
}

static CtkCssValue *
parse_css_fill_mode (CtkCssStyleProperty *property,
                     CtkCssParser        *parser)
{
  return _ctk_css_array_value_parse (parser, parse_one_css_fill_mode);
}

static CtkCssValue *
image_effect_parse (CtkCssStyleProperty *property,
		    CtkCssParser        *parser)
{
  CtkCssValue *value = _ctk_css_icon_effect_value_try_parse (parser);

  if (value == NULL)
    _ctk_css_parser_error (parser, "unknown value for property");

  return value;
}

static CtkCssValue *
icon_palette_parse (CtkCssStyleProperty *property,
		    CtkCssParser        *parser)
{
  return ctk_css_palette_value_parse (parser);
}

static CtkCssValue *
icon_style_parse (CtkCssStyleProperty *property,
		  CtkCssParser        *parser)
{
  CtkCssValue *value = _ctk_css_icon_style_value_try_parse (parser);

  if (value == NULL)
    _ctk_css_parser_error (parser, "unknown value for property");

  return value;
}

static CtkCssValue *
bindings_value_parse_one (CtkCssParser *parser)
{
  char *name;

  name = _ctk_css_parser_try_ident (parser, TRUE);
  if (name == NULL)
    {
      _ctk_css_parser_error (parser, "Not a valid binding name");
      return NULL;
    }

  if (g_ascii_strcasecmp (name, "none") == 0)
    {
      name = NULL;
    }
  else if (!ctk_binding_set_find (name))
    {
      _ctk_css_parser_error (parser, "No binding set named '%s'", name);
      g_free (name);
      return NULL;
    }

  return _ctk_css_string_value_new_take (name);
}

static CtkCssValue *
bindings_value_parse (CtkCssStyleProperty *property,
                      CtkCssParser        *parser)
{
  return _ctk_css_array_value_parse (parser, bindings_value_parse_one);
}

static void
bindings_value_query (CtkCssStyleProperty *property,
                      const CtkCssValue   *css_value,
                      GValue              *value)
{
  GPtrArray *array;
  guint i;

  g_value_init (value, G_TYPE_PTR_ARRAY);

  if (_ctk_css_array_value_get_n_values (css_value) == 0)
    return;

  array = NULL;

  for (i = 0; i < _ctk_css_array_value_get_n_values (css_value); i++)
    {
      const char *name;
      CtkBindingSet *binding_set;
      
      name = _ctk_css_string_value_get (_ctk_css_array_value_get_nth (css_value, i));
      if (name == NULL)
        continue;

      binding_set = ctk_binding_set_find (name);
      if (binding_set == NULL)
        continue;
      
      if (array == NULL)
        array = g_ptr_array_new ();
      g_ptr_array_add (array, binding_set);
    }

  g_value_take_boxed (value, array);
}

static CtkCssValue *
bindings_value_assign (CtkCssStyleProperty *property,
                       const GValue        *value)
{
  GPtrArray *binding_sets = g_value_get_boxed (value);
  CtkCssValue **values, *result;
  guint i;

  if (binding_sets == NULL || binding_sets->len == 0)
    return _ctk_css_array_value_new (_ctk_css_string_value_new (NULL));

  values = g_new (CtkCssValue *, binding_sets->len);

  for (i = 0; i < binding_sets->len; i++)
    {
      CtkBindingSet *binding_set = g_ptr_array_index (binding_sets, i);
      values[i] = _ctk_css_string_value_new (binding_set->set_name);
    }

  result = _ctk_css_array_value_new_from_array (values, binding_sets->len);
  g_free (values);
  return result;
}

static CtkCssValue *
parse_letter_spacing (CtkCssStyleProperty *property,
                      CtkCssParser        *parser)
{
  return _ctk_css_number_value_parse (parser, CTK_CSS_PARSE_LENGTH);
}

static CtkCssValue *
parse_text_decoration_line (CtkCssStyleProperty *property,
                            CtkCssParser        *parser)
{
  CtkCssValue *value = _ctk_css_text_decoration_line_value_try_parse (parser);

  if (value == NULL)
    _ctk_css_parser_error (parser, "unknown value for property");

  return value;
}

static CtkCssValue *
parse_text_decoration_style (CtkCssStyleProperty *property,
                             CtkCssParser        *parser)
{
  CtkCssValue *value = _ctk_css_text_decoration_style_value_try_parse (parser);

  if (value == NULL)
    _ctk_css_parser_error (parser, "unknown value for property");

  return value;
}

static CtkCssValue *
box_shadow_value_parse (CtkCssStyleProperty *property,
                        CtkCssParser        *parser)
{
  return _ctk_css_shadows_value_parse (parser, TRUE);
}

static CtkCssValue *
shadow_value_parse (CtkCssStyleProperty *property,
                    CtkCssParser        *parser)
{
  return _ctk_css_shadows_value_parse (parser, FALSE);
}

static CtkCssValue *
transform_value_parse (CtkCssStyleProperty *property,
                       CtkCssParser        *parser)
{
  return _ctk_css_transform_value_parse (parser);
}

static CtkCssValue *
border_corner_radius_value_parse (CtkCssStyleProperty *property,
                                  CtkCssParser        *parser)
{
  return _ctk_css_corner_value_parse (parser);
}

static CtkCssValue *
css_image_value_parse (CtkCssStyleProperty *property,
                       CtkCssParser        *parser)
{
  CtkCssImage *image;

  if (_ctk_css_parser_try (parser, "none", TRUE))
    image = NULL;
  else
    {
      image = _ctk_css_image_new_parse (parser);
      if (image == NULL)
        return NULL;
    }

  return _ctk_css_image_value_new (image);
}

static CtkCssValue *
css_image_value_parse_with_builtin (CtkCssStyleProperty *property,
                                    CtkCssParser        *parser)
{
  if (_ctk_css_parser_try (parser, "builtin", TRUE))
    return _ctk_css_image_value_new (ctk_css_image_builtin_new ());

  return css_image_value_parse (property, parser);
}

static void
css_image_value_query (CtkCssStyleProperty *property,
                       const CtkCssValue   *css_value,
                       GValue              *value)
{
  CtkCssImage *image = _ctk_css_image_value_get_image (css_value);
  cairo_pattern_t *pattern;
  cairo_surface_t *surface;
  cairo_matrix_t matrix;
  
  g_value_init (value, CAIRO_GOBJECT_TYPE_PATTERN);

  if (CTK_IS_CSS_IMAGE_GRADIENT (image))
    g_value_set_boxed (value, CTK_CSS_IMAGE_GRADIENT (image)->pattern);
  else if (image != NULL)
    {
      double width, height;

      /* the 100, 100 is rather random */
      _ctk_css_image_get_concrete_size (image, 0, 0, 100, 100, &width, &height);
      surface = _ctk_css_image_get_surface (image, NULL, width, height);
      pattern = cairo_pattern_create_for_surface (surface);
      cairo_matrix_init_scale (&matrix, width, height);
      cairo_pattern_set_matrix (pattern, &matrix);
      cairo_surface_destroy (surface);
      g_value_take_boxed (value, pattern);
    }
}

static CtkCssValue *
css_image_value_assign (CtkCssStyleProperty *property,
                        const GValue        *value)
{
  g_warning ("FIXME: assigning images is not implemented");
  return _ctk_css_image_value_new (NULL);
}

static CtkCssValue *
background_image_value_parse_one (CtkCssParser *parser)
{
  return css_image_value_parse (NULL, parser);
}

static CtkCssValue *
background_image_value_parse (CtkCssStyleProperty *property,
                              CtkCssParser        *parser)
{
  return _ctk_css_array_value_parse (parser, background_image_value_parse_one);
}

static void
background_image_value_query (CtkCssStyleProperty *property,
                              const CtkCssValue   *css_value,
                              GValue              *value)
{
  css_image_value_query (property, _ctk_css_array_value_get_nth (css_value, 0), value);
}

static CtkCssValue *
background_image_value_assign (CtkCssStyleProperty *property,
                               const GValue        *value)
{
  return _ctk_css_array_value_new (css_image_value_assign (property, value));
}

static CtkCssValue *
dpi_parse (CtkCssStyleProperty *property,
	   CtkCssParser        *parser)
{
  return _ctk_css_number_value_parse (parser, CTK_CSS_PARSE_NUMBER);
}

CtkCssValue *
ctk_css_font_size_value_parse (CtkCssParser *parser)
{
  CtkCssValue *value;

  value = _ctk_css_font_size_value_try_parse (parser);
  if (value)
    return value;

  return _ctk_css_number_value_parse (parser,
                                      CTK_CSS_PARSE_LENGTH
                                      | CTK_CSS_PARSE_PERCENT
                                      | CTK_CSS_POSITIVE_ONLY
                                      | CTK_CSS_NUMBER_AS_PIXELS);
}

static CtkCssValue *
font_size_parse (CtkCssStyleProperty *property,
                 CtkCssParser        *parser)
{
  return ctk_css_font_size_value_parse (parser);
}

static CtkCssValue *
outline_parse (CtkCssStyleProperty *property,
               CtkCssParser        *parser)
{
  return _ctk_css_number_value_parse (parser,
                                      CTK_CSS_NUMBER_AS_PIXELS
                                      | CTK_CSS_PARSE_LENGTH);
}

static CtkCssValue *
border_image_repeat_parse (CtkCssStyleProperty *property,
                           CtkCssParser        *parser)
{
  CtkCssValue *value = _ctk_css_border_repeat_value_try_parse (parser);

  if (value == NULL)
    {
      _ctk_css_parser_error (parser, "Not a valid value");
      return NULL;
    }

  return value;
}

static CtkCssValue *
border_image_slice_parse (CtkCssStyleProperty *property,
                          CtkCssParser        *parser)
{
  return _ctk_css_border_value_parse (parser,
                                      CTK_CSS_PARSE_PERCENT
                                      | CTK_CSS_PARSE_NUMBER
                                      | CTK_CSS_POSITIVE_ONLY,
                                      FALSE,
                                      TRUE);
}

static CtkCssValue *
border_image_width_parse (CtkCssStyleProperty *property,
                          CtkCssParser        *parser)
{
  return _ctk_css_border_value_parse (parser,
                                      CTK_CSS_PARSE_PERCENT
                                      | CTK_CSS_PARSE_LENGTH
                                      | CTK_CSS_PARSE_NUMBER
                                      | CTK_CSS_POSITIVE_ONLY,
                                      TRUE,
                                      FALSE);
}

static CtkCssValue *
minmax_parse (CtkCssStyleProperty *property,
              CtkCssParser        *parser)
{
  return _ctk_css_number_value_parse (parser,
                                      CTK_CSS_PARSE_LENGTH
                                      | CTK_CSS_POSITIVE_ONLY);
}

static CtkCssValue *
transition_property_parse_one (CtkCssParser *parser)
{
  CtkCssValue *value;

  value = _ctk_css_ident_value_try_parse (parser);

  if (value == NULL)
    {
      _ctk_css_parser_error (parser, "Expected an identifier");
      return NULL;
    }

  return value;
}

static CtkCssValue *
transition_property_parse (CtkCssStyleProperty *property,
                           CtkCssParser        *parser)
{
  return _ctk_css_array_value_parse (parser, transition_property_parse_one);
}

static CtkCssValue *
transition_time_parse_one (CtkCssParser *parser)
{
  return _ctk_css_number_value_parse (parser, CTK_CSS_PARSE_TIME);
}

static CtkCssValue *
transition_time_parse (CtkCssStyleProperty *property,
                       CtkCssParser        *parser)
{
  return _ctk_css_array_value_parse (parser, transition_time_parse_one);
}

static CtkCssValue *
transition_timing_function_parse (CtkCssStyleProperty *property,
                                  CtkCssParser        *parser)
{
  return _ctk_css_array_value_parse (parser, _ctk_css_ease_value_parse);
}

static CtkCssValue *
iteration_count_parse_one (CtkCssParser *parser)
{
  if (_ctk_css_parser_try (parser, "infinite", TRUE))
    return _ctk_css_number_value_new (HUGE_VAL, CTK_CSS_NUMBER);

  return _ctk_css_number_value_parse (parser, CTK_CSS_PARSE_NUMBER | CTK_CSS_POSITIVE_ONLY);
}

static CtkCssValue *
iteration_count_parse (CtkCssStyleProperty *property,
                       CtkCssParser        *parser)
{
  return _ctk_css_array_value_parse (parser, iteration_count_parse_one);
}

static CtkCssValue *
engine_parse (CtkCssStyleProperty *property,
              CtkCssParser        *parser)
{
  return _ctk_css_engine_value_parse (parser);
}

static void
engine_query (CtkCssStyleProperty *property,
              const CtkCssValue   *css_value,
              GValue              *value)
{
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_value_init (value, CTK_TYPE_THEMING_ENGINE);
  g_value_set_object (value, _ctk_css_engine_value_get_engine (css_value));
G_GNUC_END_IGNORE_DEPRECATIONS
}

static CtkCssValue *
engine_assign (CtkCssStyleProperty *property,
               const GValue        *value)
{
  return _ctk_css_engine_value_new (g_value_get_object (value));
}

static CtkCssValue *
parse_margin (CtkCssStyleProperty *property,
              CtkCssParser        *parser)
{
  return _ctk_css_number_value_parse (parser,
                                      CTK_CSS_NUMBER_AS_PIXELS
                                      | CTK_CSS_PARSE_LENGTH);
}

static CtkCssValue *
parse_padding (CtkCssStyleProperty *property,
               CtkCssParser        *parser)
{
  return _ctk_css_number_value_parse (parser,
                                      CTK_CSS_POSITIVE_ONLY
                                      | CTK_CSS_NUMBER_AS_PIXELS
                                      | CTK_CSS_PARSE_LENGTH);
}

static CtkCssValue *
parse_border_width (CtkCssStyleProperty *property,
                    CtkCssParser        *parser)
{
  return _ctk_css_number_value_parse (parser,
                                      CTK_CSS_POSITIVE_ONLY
                                      | CTK_CSS_NUMBER_AS_PIXELS
                                      | CTK_CSS_PARSE_LENGTH);
}

static CtkCssValue *
blend_mode_value_parse_one (CtkCssParser        *parser)
{
  CtkCssValue *value = _ctk_css_blend_mode_value_try_parse (parser);

  if (value == NULL)
    _ctk_css_parser_error (parser, "unknown value for property");

  return value;
}

static CtkCssValue *
blend_mode_value_parse (CtkCssStyleProperty *property,
                        CtkCssParser        *parser)
{
  return _ctk_css_array_value_parse (parser, blend_mode_value_parse_one);
}

static CtkCssValue *
background_repeat_value_parse_one (CtkCssParser *parser)
{
  CtkCssValue *value = _ctk_css_background_repeat_value_try_parse (parser);

  if (value == NULL)
    {
      _ctk_css_parser_error (parser, "Not a valid value");
      return NULL;
    }

  return value;
}

static CtkCssValue *
background_repeat_value_parse (CtkCssStyleProperty *property,
                               CtkCssParser        *parser)
{
  return _ctk_css_array_value_parse (parser, background_repeat_value_parse_one);
}

static CtkCssValue *
background_size_parse (CtkCssStyleProperty *property,
                       CtkCssParser        *parser)
{
  return _ctk_css_array_value_parse (parser, _ctk_css_bg_size_value_parse);
}

static CtkCssValue *
background_position_parse (CtkCssStyleProperty *property,
			   CtkCssParser        *parser)
{
  return _ctk_css_array_value_parse (parser, _ctk_css_position_value_parse);
}

static CtkCssValue *
icon_theme_value_parse (CtkCssStyleProperty *property,
		        CtkCssParser        *parser)
{
  return ctk_css_icon_theme_value_parse (parser);
}

/*** REGISTRATION ***/

void
_ctk_css_style_property_init_properties (void)
{
  /* Initialize "color", "-ctk-dpi" and "font-size" first,
   * so that when computing values later they are
   * done first. That way, 'currentColor' and font
   * sizes in em can be looked up properly */
  ctk_css_style_property_register        ("color",
                                          CTK_CSS_PROPERTY_COLOR,
                                          CDK_TYPE_RGBA,
                                          CTK_STYLE_PROPERTY_INHERIT | CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_FOREGROUND | CTK_CSS_AFFECTS_TEXT | CTK_CSS_AFFECTS_SYMBOLIC_ICON,
                                          color_parse,
                                          color_query,
                                          color_assign,
                                          _ctk_css_color_value_new_rgba (1, 1, 1, 1));
  ctk_css_style_property_register        ("-ctk-dpi",
                                          CTK_CSS_PROPERTY_DPI,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_INHERIT | CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_FONT | CTK_CSS_AFFECTS_TEXT | CTK_CSS_AFFECTS_SIZE,
                                          dpi_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_number_value_new (96.0, CTK_CSS_NUMBER));
  ctk_css_style_property_register        ("font-size",
                                          CTK_CSS_PROPERTY_FONT_SIZE,
                                          G_TYPE_DOUBLE,
                                          CTK_STYLE_PROPERTY_INHERIT | CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_FONT | CTK_CSS_AFFECTS_TEXT | CTK_CSS_AFFECTS_SIZE,
                                          font_size_parse,
                                          query_font_size,
                                          assign_font_size,
                                          _ctk_css_font_size_value_new (CTK_CSS_FONT_SIZE_MEDIUM));
  ctk_css_style_property_register        ("-ctk-icon-theme",
                                          CTK_CSS_PROPERTY_ICON_THEME,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_INHERIT,
                                          CTK_CSS_AFFECTS_ICON | CTK_CSS_AFFECTS_SYMBOLIC_ICON,
                                          icon_theme_value_parse,
                                          NULL,
                                          NULL,
                                          ctk_css_icon_theme_value_new (NULL));
  ctk_css_style_property_register        ("-ctk-icon-palette",
					  CTK_CSS_PROPERTY_ICON_PALETTE,
					  G_TYPE_NONE,
					  CTK_STYLE_PROPERTY_ANIMATED | CTK_STYLE_PROPERTY_INHERIT,
                                          CTK_CSS_AFFECTS_SYMBOLIC_ICON,
					  icon_palette_parse,
					  NULL,
					  NULL,
					  ctk_css_palette_value_new_default ());


  /* properties that aren't referenced when computing values
   * start here */
  ctk_css_style_property_register        ("background-color",
                                          CTK_CSS_PROPERTY_BACKGROUND_COLOR,
                                          CDK_TYPE_RGBA,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BACKGROUND,
                                          color_parse,
                                          color_query,
                                          color_assign,
                                          _ctk_css_color_value_new_rgba (0, 0, 0, 0));

  ctk_css_style_property_register        ("font-family",
                                          CTK_CSS_PROPERTY_FONT_FAMILY,
                                          G_TYPE_STRV,
                                          CTK_STYLE_PROPERTY_INHERIT,
                                          CTK_CSS_AFFECTS_FONT | CTK_CSS_AFFECTS_TEXT,
                                          font_family_parse,
                                          font_family_query,
                                          font_family_assign,
                                          _ctk_css_array_value_new (_ctk_css_string_value_new ("Sans")));
  ctk_css_style_property_register        ("font-style",
                                          CTK_CSS_PROPERTY_FONT_STYLE,
                                          PANGO_TYPE_STYLE,
                                          CTK_STYLE_PROPERTY_INHERIT,
                                          CTK_CSS_AFFECTS_FONT | CTK_CSS_AFFECTS_TEXT,
                                          font_style_parse,
                                          font_style_query,
                                          font_style_assign,
                                          _ctk_css_font_style_value_new (PANGO_STYLE_NORMAL));
  ctk_css_style_property_register        ("font-variant",
                                          CTK_CSS_PROPERTY_FONT_VARIANT,
                                          PANGO_TYPE_VARIANT,
                                          CTK_STYLE_PROPERTY_INHERIT,
                                          CTK_CSS_AFFECTS_FONT | CTK_CSS_AFFECTS_TEXT,
                                          font_variant_parse,
                                          font_variant_query,
                                          font_variant_assign,
                                          _ctk_css_font_variant_value_new (PANGO_VARIANT_NORMAL));
  ctk_css_style_property_register        ("font-weight",
                                          CTK_CSS_PROPERTY_FONT_WEIGHT,
                                          PANGO_TYPE_WEIGHT,
                                          CTK_STYLE_PROPERTY_INHERIT | CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_FONT | CTK_CSS_AFFECTS_TEXT,
                                          font_weight_parse,
                                          font_weight_query,
                                          font_weight_assign,
                                          _ctk_css_font_weight_value_new (PANGO_WEIGHT_NORMAL));
  ctk_css_style_property_register        ("font-stretch",
                                          CTK_CSS_PROPERTY_FONT_STRETCH,
                                          PANGO_TYPE_STRETCH,
                                          CTK_STYLE_PROPERTY_INHERIT,
                                          CTK_CSS_AFFECTS_FONT | CTK_CSS_AFFECTS_TEXT,
                                          font_stretch_parse,
                                          font_stretch_query,
                                          font_stretch_assign,
                                          _ctk_css_font_stretch_value_new (PANGO_STRETCH_NORMAL));

  ctk_css_style_property_register        ("letter-spacing",
                                          CTK_CSS_PROPERTY_LETTER_SPACING,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_INHERIT | CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_TEXT | CTK_CSS_AFFECTS_TEXT_ATTRS,
                                          parse_letter_spacing,
                                          NULL,
                                          NULL,
                                          _ctk_css_number_value_new (0.0, CTK_CSS_PX));

  ctk_css_style_property_register        ("text-decoration-line",
                                          CTK_CSS_PROPERTY_TEXT_DECORATION_LINE,
                                          G_TYPE_NONE,
                                          0,
                                          CTK_CSS_AFFECTS_TEXT | CTK_CSS_AFFECTS_TEXT_ATTRS,
                                          parse_text_decoration_line,
                                          NULL,
                                          NULL,
                                          _ctk_css_text_decoration_line_value_new (CTK_CSS_TEXT_DECORATION_LINE_NONE));
  ctk_css_style_property_register        ("text-decoration-color",
                                          CTK_CSS_PROPERTY_TEXT_DECORATION_COLOR,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_TEXT | CTK_CSS_AFFECTS_TEXT_ATTRS,
                                          color_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_color_value_new_current_color ());
  ctk_css_style_property_register        ("text-decoration-style",
                                          CTK_CSS_PROPERTY_TEXT_DECORATION_STYLE,
                                          G_TYPE_NONE,
                                          0,
                                          CTK_CSS_AFFECTS_TEXT | CTK_CSS_AFFECTS_TEXT_ATTRS,
                                          parse_text_decoration_style,
                                          NULL,
                                          NULL,
                                          _ctk_css_text_decoration_style_value_new (CTK_CSS_TEXT_DECORATION_STYLE_SOLID));

  ctk_css_style_property_register        ("text-shadow",
                                          CTK_CSS_PROPERTY_TEXT_SHADOW,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_INHERIT | CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_TEXT | CTK_CSS_AFFECTS_CLIP,
                                          shadow_value_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_shadows_value_new_none ());

  ctk_css_style_property_register        ("box-shadow",
                                          CTK_CSS_PROPERTY_BOX_SHADOW,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BACKGROUND | CTK_CSS_AFFECTS_CLIP,
                                          box_shadow_value_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_shadows_value_new_none ());

  ctk_css_style_property_register        ("margin-top",
                                          CTK_CSS_PROPERTY_MARGIN_TOP,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_SIZE,
                                          parse_margin,
                                          query_length_as_int,
                                          assign_length_from_int,
                                          _ctk_css_number_value_new (0.0, CTK_CSS_PX));
  ctk_css_style_property_register        ("margin-left",
                                          CTK_CSS_PROPERTY_MARGIN_LEFT,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_SIZE,
                                          parse_margin,
                                          query_length_as_int,
                                          assign_length_from_int,
                                          _ctk_css_number_value_new (0.0, CTK_CSS_PX));
  ctk_css_style_property_register        ("margin-bottom",
                                          CTK_CSS_PROPERTY_MARGIN_BOTTOM,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_SIZE,
                                          parse_margin,
                                          query_length_as_int,
                                          assign_length_from_int,
                                          _ctk_css_number_value_new (0.0, CTK_CSS_PX));
  ctk_css_style_property_register        ("margin-right",
                                          CTK_CSS_PROPERTY_MARGIN_RIGHT,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_SIZE,
                                          parse_margin,
                                          query_length_as_int,
                                          assign_length_from_int,
                                          _ctk_css_number_value_new (0.0, CTK_CSS_PX));
  ctk_css_style_property_register        ("padding-top",
                                          CTK_CSS_PROPERTY_PADDING_TOP,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_SIZE,
                                          parse_padding,
                                          query_length_as_int,
                                          assign_length_from_int,
                                          _ctk_css_number_value_new (0.0, CTK_CSS_PX));
  ctk_css_style_property_register        ("padding-left",
                                          CTK_CSS_PROPERTY_PADDING_LEFT,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_SIZE,
                                          parse_padding,
                                          query_length_as_int,
                                          assign_length_from_int,
                                          _ctk_css_number_value_new (0.0, CTK_CSS_PX));
  ctk_css_style_property_register        ("padding-bottom",
                                          CTK_CSS_PROPERTY_PADDING_BOTTOM,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_SIZE,
                                          parse_padding,
                                          query_length_as_int,
                                          assign_length_from_int,
                                          _ctk_css_number_value_new (0.0, CTK_CSS_PX));
  ctk_css_style_property_register        ("padding-right",
                                          CTK_CSS_PROPERTY_PADDING_RIGHT,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_SIZE,
                                          parse_padding,
                                          query_length_as_int,
                                          assign_length_from_int,
                                          _ctk_css_number_value_new (0.0, CTK_CSS_PX));
  /* IMPORTANT: the border-width properties must come after border-style properties,
   * they depend on them for their value computation.
   */
  ctk_css_style_property_register        ("border-top-style",
                                          CTK_CSS_PROPERTY_BORDER_TOP_STYLE,
                                          CTK_TYPE_BORDER_STYLE,
                                          0,
                                          CTK_CSS_AFFECTS_BORDER,
                                          parse_border_style,
                                          query_border_style,
                                          assign_border_style,
                                          _ctk_css_border_style_value_new (CTK_BORDER_STYLE_NONE));
  ctk_css_style_property_register        ("border-top-width",
                                          CTK_CSS_PROPERTY_BORDER_TOP_WIDTH,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BORDER | CTK_CSS_AFFECTS_SIZE,
                                          parse_border_width,
                                          query_length_as_int,
                                          assign_length_from_int,
                                          _ctk_css_number_value_new (0.0, CTK_CSS_PX));
  ctk_css_style_property_register        ("border-left-style",
                                          CTK_CSS_PROPERTY_BORDER_LEFT_STYLE,
                                          CTK_TYPE_BORDER_STYLE,
                                          0,
                                          CTK_CSS_AFFECTS_BORDER,
                                          parse_border_style,
                                          query_border_style,
                                          assign_border_style,
                                          _ctk_css_border_style_value_new (CTK_BORDER_STYLE_NONE));
  ctk_css_style_property_register        ("border-left-width",
                                          CTK_CSS_PROPERTY_BORDER_LEFT_WIDTH,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BORDER | CTK_CSS_AFFECTS_SIZE,
                                          parse_border_width,
                                          query_length_as_int,
                                          assign_length_from_int,
                                          _ctk_css_number_value_new (0.0, CTK_CSS_PX));
  ctk_css_style_property_register        ("border-bottom-style",
                                          CTK_CSS_PROPERTY_BORDER_BOTTOM_STYLE,
                                          CTK_TYPE_BORDER_STYLE,
                                          0,
                                          CTK_CSS_AFFECTS_BORDER,
                                          parse_border_style,
                                          query_border_style,
                                          assign_border_style,
                                          _ctk_css_border_style_value_new (CTK_BORDER_STYLE_NONE));
  ctk_css_style_property_register        ("border-bottom-width",
                                          CTK_CSS_PROPERTY_BORDER_BOTTOM_WIDTH,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BORDER | CTK_CSS_AFFECTS_SIZE,
                                          parse_border_width,
                                          query_length_as_int,
                                          assign_length_from_int,
                                          _ctk_css_number_value_new (0.0, CTK_CSS_PX));
  ctk_css_style_property_register        ("border-right-style",
                                          CTK_CSS_PROPERTY_BORDER_RIGHT_STYLE,
                                          CTK_TYPE_BORDER_STYLE,
                                          0,
                                          CTK_CSS_AFFECTS_BORDER,
                                          parse_border_style,
                                          query_border_style,
                                          assign_border_style,
                                          _ctk_css_border_style_value_new (CTK_BORDER_STYLE_NONE));
  ctk_css_style_property_register        ("border-right-width",
                                          CTK_CSS_PROPERTY_BORDER_RIGHT_WIDTH,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BORDER | CTK_CSS_AFFECTS_SIZE,
                                          parse_border_width,
                                          query_length_as_int,
                                          assign_length_from_int,
                                          _ctk_css_number_value_new (0.0, CTK_CSS_PX));

  ctk_css_style_property_register        ("border-top-left-radius",
                                          CTK_CSS_PROPERTY_BORDER_TOP_LEFT_RADIUS,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BACKGROUND | CTK_CSS_AFFECTS_BORDER,
                                          border_corner_radius_value_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_corner_value_new (_ctk_css_number_value_new (0, CTK_CSS_PX),
                                                                     _ctk_css_number_value_new (0, CTK_CSS_PX)));
  ctk_css_style_property_register        ("border-top-right-radius",
                                          CTK_CSS_PROPERTY_BORDER_TOP_RIGHT_RADIUS,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BACKGROUND | CTK_CSS_AFFECTS_BORDER,
                                          border_corner_radius_value_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_corner_value_new (_ctk_css_number_value_new (0, CTK_CSS_PX),
                                                                     _ctk_css_number_value_new (0, CTK_CSS_PX)));
  ctk_css_style_property_register        ("border-bottom-right-radius",
                                          CTK_CSS_PROPERTY_BORDER_BOTTOM_RIGHT_RADIUS,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BACKGROUND | CTK_CSS_AFFECTS_BORDER,
                                          border_corner_radius_value_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_corner_value_new (_ctk_css_number_value_new (0, CTK_CSS_PX),
                                                                     _ctk_css_number_value_new (0, CTK_CSS_PX)));
  ctk_css_style_property_register        ("border-bottom-left-radius",
                                          CTK_CSS_PROPERTY_BORDER_BOTTOM_LEFT_RADIUS,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BACKGROUND | CTK_CSS_AFFECTS_BORDER,
                                          border_corner_radius_value_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_corner_value_new (_ctk_css_number_value_new (0, CTK_CSS_PX),
                                                                     _ctk_css_number_value_new (0, CTK_CSS_PX)));

  ctk_css_style_property_register        ("outline-style",
                                          CTK_CSS_PROPERTY_OUTLINE_STYLE,
                                          CTK_TYPE_BORDER_STYLE,
                                          0,
                                          CTK_CSS_AFFECTS_OUTLINE | CTK_CSS_AFFECTS_CLIP,
                                          parse_border_style,
                                          query_border_style,
                                          assign_border_style,
                                          _ctk_css_border_style_value_new (CTK_BORDER_STYLE_NONE));
  ctk_css_style_property_register        ("outline-width",
                                          CTK_CSS_PROPERTY_OUTLINE_WIDTH,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_OUTLINE | CTK_CSS_AFFECTS_CLIP,
                                          parse_border_width,
                                          query_length_as_int,
                                          assign_length_from_int,
                                          _ctk_css_number_value_new (0.0, CTK_CSS_PX));
  ctk_css_style_property_register        ("outline-offset",
                                          CTK_CSS_PROPERTY_OUTLINE_OFFSET,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_OUTLINE | CTK_CSS_AFFECTS_CLIP,
                                          outline_parse,
                                          query_length_as_int,
                                          assign_length_from_int,
                                          _ctk_css_number_value_new (0.0, CTK_CSS_PX));

  ctk_css_style_property_register        ("-ctk-outline-top-left-radius",
                                          CTK_CSS_PROPERTY_OUTLINE_TOP_LEFT_RADIUS,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_OUTLINE,
                                          border_corner_radius_value_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_corner_value_new (_ctk_css_number_value_new (0, CTK_CSS_PX),
                                                                     _ctk_css_number_value_new (0, CTK_CSS_PX)));
  _ctk_style_property_add_alias ("-ctk-outline-top-left-radius", "outline-top-left-radius");
  ctk_css_style_property_register        ("-ctk-outline-top-right-radius",
                                          CTK_CSS_PROPERTY_OUTLINE_TOP_RIGHT_RADIUS,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_OUTLINE,
                                          border_corner_radius_value_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_corner_value_new (_ctk_css_number_value_new (0, CTK_CSS_PX),
                                                                     _ctk_css_number_value_new (0, CTK_CSS_PX)));
  _ctk_style_property_add_alias ("-ctk-outline-top-right-radius", "outline-top-right-radius");
  ctk_css_style_property_register        ("-ctk-outline-bottom-right-radius",
                                          CTK_CSS_PROPERTY_OUTLINE_BOTTOM_RIGHT_RADIUS,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_OUTLINE,
                                          border_corner_radius_value_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_corner_value_new (_ctk_css_number_value_new (0, CTK_CSS_PX),
                                                                     _ctk_css_number_value_new (0, CTK_CSS_PX)));
  _ctk_style_property_add_alias ("-ctk-outline-bottom-right-radius", "outline-bottom-right-radius");
  ctk_css_style_property_register        ("-ctk-outline-bottom-left-radius",
                                          CTK_CSS_PROPERTY_OUTLINE_BOTTOM_LEFT_RADIUS,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_OUTLINE,
                                          border_corner_radius_value_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_corner_value_new (_ctk_css_number_value_new (0, CTK_CSS_PX),
                                                                     _ctk_css_number_value_new (0, CTK_CSS_PX)));
  _ctk_style_property_add_alias ("-ctk-outline-bottom-left-radius", "outline-bottom-left-radius");

  ctk_css_style_property_register        ("background-clip",
                                          CTK_CSS_PROPERTY_BACKGROUND_CLIP,
                                          G_TYPE_NONE,
                                          0,
                                          CTK_CSS_AFFECTS_BACKGROUND,
                                          parse_css_area,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_area_value_new (CTK_CSS_AREA_BORDER_BOX)));
  ctk_css_style_property_register        ("background-origin",
                                          CTK_CSS_PROPERTY_BACKGROUND_ORIGIN,
                                          G_TYPE_NONE,
                                          0,
                                          CTK_CSS_AFFECTS_BACKGROUND,
                                          parse_css_area,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_area_value_new (CTK_CSS_AREA_PADDING_BOX)));
  ctk_css_style_property_register        ("background-size",
                                          CTK_CSS_PROPERTY_BACKGROUND_SIZE,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BACKGROUND,
                                          background_size_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_bg_size_value_new (NULL, NULL)));
  ctk_css_style_property_register        ("background-position",
                                          CTK_CSS_PROPERTY_BACKGROUND_POSITION,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BACKGROUND,
                                          background_position_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_position_value_new (_ctk_css_number_value_new (0, CTK_CSS_PERCENT),
                                                                                                 _ctk_css_number_value_new (0, CTK_CSS_PERCENT))));

  ctk_css_style_property_register        ("border-top-color",
                                          CTK_CSS_PROPERTY_BORDER_TOP_COLOR,
                                          CDK_TYPE_RGBA,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BORDER,
                                          color_parse,
                                          color_query,
                                          color_assign,
                                          _ctk_css_color_value_new_current_color ());
  ctk_css_style_property_register        ("border-right-color",
                                          CTK_CSS_PROPERTY_BORDER_RIGHT_COLOR,
                                          CDK_TYPE_RGBA,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BORDER,
                                          color_parse,
                                          color_query,
                                          color_assign,
                                          _ctk_css_color_value_new_current_color ());
  ctk_css_style_property_register        ("border-bottom-color",
                                          CTK_CSS_PROPERTY_BORDER_BOTTOM_COLOR,
                                          CDK_TYPE_RGBA,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BORDER,
                                          color_parse,
                                          color_query,
                                          color_assign,
                                          _ctk_css_color_value_new_current_color ());
  ctk_css_style_property_register        ("border-left-color",
                                          CTK_CSS_PROPERTY_BORDER_LEFT_COLOR,
                                          CDK_TYPE_RGBA,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BORDER,
                                          color_parse,
                                          color_query,
                                          color_assign,
                                          _ctk_css_color_value_new_current_color ());
  ctk_css_style_property_register        ("outline-color",
                                          CTK_CSS_PROPERTY_OUTLINE_COLOR,
                                          CDK_TYPE_RGBA,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_OUTLINE,
                                          color_parse,
                                          color_query,
                                          color_assign,
                                          _ctk_css_color_value_new_current_color ());

  ctk_css_style_property_register        ("background-repeat",
                                          CTK_CSS_PROPERTY_BACKGROUND_REPEAT,
                                          G_TYPE_NONE,
                                          0,
                                          CTK_CSS_AFFECTS_BACKGROUND,
                                          background_repeat_value_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_background_repeat_value_new (CTK_CSS_REPEAT_STYLE_REPEAT,
                                                                                                          CTK_CSS_REPEAT_STYLE_REPEAT)));
  ctk_css_style_property_register        ("background-image",
                                          CTK_CSS_PROPERTY_BACKGROUND_IMAGE,
                                          CAIRO_GOBJECT_TYPE_PATTERN,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BACKGROUND,
                                          background_image_value_parse,
                                          background_image_value_query,
                                          background_image_value_assign,
                                          _ctk_css_array_value_new (_ctk_css_image_value_new (NULL)));

  ctk_css_style_property_register        ("background-blend-mode",
                                          CTK_CSS_PROPERTY_BACKGROUND_BLEND_MODE,
                                          G_TYPE_NONE,
                                          0,
                                          CTK_CSS_AFFECTS_BACKGROUND,
                                          blend_mode_value_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_blend_mode_value_new (CTK_CSS_BLEND_MODE_NORMAL)));

  ctk_css_style_property_register        ("border-image-source",
                                          CTK_CSS_PROPERTY_BORDER_IMAGE_SOURCE,
                                          CAIRO_GOBJECT_TYPE_PATTERN,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_BORDER,
                                          css_image_value_parse,
                                          css_image_value_query,
                                          css_image_value_assign,
                                          _ctk_css_image_value_new (NULL));
  ctk_css_style_property_register        ("border-image-repeat",
                                          CTK_CSS_PROPERTY_BORDER_IMAGE_REPEAT,
                                          G_TYPE_NONE,
                                          0,
                                          CTK_CSS_AFFECTS_BORDER,
                                          border_image_repeat_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_border_repeat_value_new (CTK_CSS_REPEAT_STYLE_STRETCH,
                                                                            CTK_CSS_REPEAT_STYLE_STRETCH));

  ctk_css_style_property_register        ("border-image-slice",
                                          CTK_CSS_PROPERTY_BORDER_IMAGE_SLICE,
                                          CTK_TYPE_BORDER,
                                          0,
                                          CTK_CSS_AFFECTS_BORDER,
                                          border_image_slice_parse,
                                          query_border,
                                          assign_border,
                                          _ctk_css_border_value_new (_ctk_css_number_value_new (100, CTK_CSS_PERCENT),
                                                                     _ctk_css_number_value_new (100, CTK_CSS_PERCENT),
                                                                     _ctk_css_number_value_new (100, CTK_CSS_PERCENT),
                                                                     _ctk_css_number_value_new (100, CTK_CSS_PERCENT)));
  ctk_css_style_property_register        ("border-image-width",
                                          CTK_CSS_PROPERTY_BORDER_IMAGE_WIDTH,
                                          CTK_TYPE_BORDER,
                                          0,
                                          CTK_CSS_AFFECTS_BORDER,
                                          border_image_width_parse,
                                          query_border,
                                          assign_border,
                                          _ctk_css_border_value_new (_ctk_css_number_value_new (1, CTK_CSS_NUMBER),
                                                                     _ctk_css_number_value_new (1, CTK_CSS_NUMBER),
                                                                     _ctk_css_number_value_new (1, CTK_CSS_NUMBER),
                                                                     _ctk_css_number_value_new (1, CTK_CSS_NUMBER)));

  ctk_css_style_property_register        ("-ctk-icon-source",
                                          CTK_CSS_PROPERTY_ICON_SOURCE,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_ICON | CTK_CSS_AFFECTS_SYMBOLIC_ICON,
                                          css_image_value_parse_with_builtin,
                                          NULL,
                                          NULL,
                                          _ctk_css_image_value_new (ctk_css_image_builtin_new ()));
  ctk_css_style_property_register        ("-ctk-icon-shadow",
                                          CTK_CSS_PROPERTY_ICON_SHADOW,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_INHERIT | CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_ICON | CTK_CSS_AFFECTS_SYMBOLIC_ICON | CTK_CSS_AFFECTS_CLIP,
                                          shadow_value_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_shadows_value_new_none ());
  _ctk_style_property_add_alias ("-ctk-icon-shadow", "icon-shadow");
  ctk_css_style_property_register        ("-ctk-icon-style",
                                          CTK_CSS_PROPERTY_ICON_STYLE,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_INHERIT,
                                          CTK_CSS_AFFECTS_ICON | CTK_CSS_AFFECTS_SYMBOLIC_ICON,
                                          icon_style_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_icon_style_value_new (CTK_CSS_ICON_STYLE_REQUESTED));
  ctk_css_style_property_register        ("-ctk-icon-transform",
                                          CTK_CSS_PROPERTY_ICON_TRANSFORM,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_ICON | CTK_CSS_AFFECTS_SYMBOLIC_ICON | CTK_CSS_AFFECTS_CLIP,
                                          transform_value_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_transform_value_new_none ());

  ctk_css_style_property_register        ("min-width",
                                          CTK_CSS_PROPERTY_MIN_WIDTH,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_SIZE,
                                          minmax_parse,
                                          query_length_as_int,
                                          NULL,
                                          _ctk_css_number_value_new (0, CTK_CSS_PX));
  ctk_css_style_property_register        ("min-height",
                                          CTK_CSS_PROPERTY_MIN_HEIGHT,
                                          G_TYPE_INT,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_SIZE,
                                          minmax_parse,
                                          query_length_as_int,
                                          NULL,
                                          _ctk_css_number_value_new (0, CTK_CSS_PX));

  ctk_css_style_property_register        ("transition-property",
                                          CTK_CSS_PROPERTY_TRANSITION_PROPERTY,
                                          G_TYPE_NONE,
                                          0,
                                          0,
                                          transition_property_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_ident_value_new ("all")));
  ctk_css_style_property_register        ("transition-duration",
                                          CTK_CSS_PROPERTY_TRANSITION_DURATION,
                                          G_TYPE_NONE,
                                          0,
                                          0,
                                          transition_time_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_number_value_new (0, CTK_CSS_S)));
  ctk_css_style_property_register        ("transition-timing-function",
                                          CTK_CSS_PROPERTY_TRANSITION_TIMING_FUNCTION,
                                          G_TYPE_NONE,
                                          0,
                                          0,
                                          transition_timing_function_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (
                                            _ctk_css_ease_value_new_cubic_bezier (0.25, 0.1, 0.25, 1.0)));
  ctk_css_style_property_register        ("transition-delay",
                                          CTK_CSS_PROPERTY_TRANSITION_DELAY,
                                          G_TYPE_NONE,
                                          0,
                                          0,
                                          transition_time_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_number_value_new (0, CTK_CSS_S)));

  ctk_css_style_property_register        ("animation-name",
                                          CTK_CSS_PROPERTY_ANIMATION_NAME,
                                          G_TYPE_NONE,
                                          0,
                                          0,
                                          transition_property_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_ident_value_new ("none")));
  ctk_css_style_property_register        ("animation-duration",
                                          CTK_CSS_PROPERTY_ANIMATION_DURATION,
                                          G_TYPE_NONE,
                                          0,
                                          0,
                                          transition_time_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_number_value_new (0, CTK_CSS_S)));
  ctk_css_style_property_register        ("animation-timing-function",
                                          CTK_CSS_PROPERTY_ANIMATION_TIMING_FUNCTION,
                                          G_TYPE_NONE,
                                          0,
                                          0,
                                          transition_timing_function_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (
                                            _ctk_css_ease_value_new_cubic_bezier (0.25, 0.1, 0.25, 1.0)));
  ctk_css_style_property_register        ("animation-iteration-count",
                                          CTK_CSS_PROPERTY_ANIMATION_ITERATION_COUNT,
                                          G_TYPE_NONE,
                                          0,
                                          0,
                                          iteration_count_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_number_value_new (1, CTK_CSS_NUMBER)));
  ctk_css_style_property_register        ("animation-direction",
                                          CTK_CSS_PROPERTY_ANIMATION_DIRECTION,
                                          G_TYPE_NONE,
                                          0,
                                          0,
                                          parse_css_direction,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_direction_value_new (CTK_CSS_DIRECTION_NORMAL)));
  ctk_css_style_property_register        ("animation-play-state",
                                          CTK_CSS_PROPERTY_ANIMATION_PLAY_STATE,
                                          G_TYPE_NONE,
                                          0,
                                          0,
                                          parse_css_play_state,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_play_state_value_new (CTK_CSS_PLAY_STATE_RUNNING)));
  ctk_css_style_property_register        ("animation-delay",
                                          CTK_CSS_PROPERTY_ANIMATION_DELAY,
                                          G_TYPE_NONE,
                                          0,
                                          0,
                                          transition_time_parse,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_number_value_new (0, CTK_CSS_S)));
  ctk_css_style_property_register        ("animation-fill-mode",
                                          CTK_CSS_PROPERTY_ANIMATION_FILL_MODE,
                                          G_TYPE_NONE,
                                          0,
                                          0,
                                          parse_css_fill_mode,
                                          NULL,
                                          NULL,
                                          _ctk_css_array_value_new (_ctk_css_fill_mode_value_new (CTK_CSS_FILL_NONE)));

  ctk_css_style_property_register        ("opacity",
                                          CTK_CSS_PROPERTY_OPACITY,
                                          G_TYPE_DOUBLE,
                                          CTK_STYLE_PROPERTY_ANIMATED,
                                          0,
                                          opacity_parse,
                                          opacity_query,
                                          NULL,
                                          _ctk_css_number_value_new (1, CTK_CSS_NUMBER));
  ctk_css_style_property_register        ("-ctk-icon-effect",
					  CTK_CSS_PROPERTY_ICON_EFFECT,
					  G_TYPE_NONE,
					  CTK_STYLE_PROPERTY_INHERIT,
                                          CTK_CSS_AFFECTS_ICON,
					  image_effect_parse,
					  NULL,
					  NULL,
					  _ctk_css_icon_effect_value_new (CTK_CSS_ICON_EFFECT_NONE));
  _ctk_style_property_add_alias ("-ctk-icon-effect", "-ctk-image-effect");

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_css_style_property_register        ("engine",
                                          CTK_CSS_PROPERTY_ENGINE,
                                          CTK_TYPE_THEMING_ENGINE,
                                          0,
                                          0,
                                          engine_parse,
                                          engine_query,
                                          engine_assign,
                                          _ctk_css_engine_value_new (ctk_theming_engine_load (NULL)));
G_GNUC_END_IGNORE_DEPRECATIONS

  /* Private property holding the binding sets */
  ctk_css_style_property_register        ("-ctk-key-bindings",
                                          CTK_CSS_PROPERTY_CTK_KEY_BINDINGS,
                                          G_TYPE_PTR_ARRAY,
                                          0,
                                          0,
                                          bindings_value_parse,
                                          bindings_value_query,
                                          bindings_value_assign,
                                          _ctk_css_array_value_new (_ctk_css_string_value_new (NULL)));
  _ctk_style_property_add_alias ("-ctk-key-bindings", "ctk-key-bindings");

  ctk_css_style_property_register        ("caret-color",
                                          CTK_CSS_PROPERTY_CARET_COLOR,
                                          CDK_TYPE_RGBA,
                                          CTK_STYLE_PROPERTY_INHERIT | CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_TEXT,
                                          color_parse,
                                          color_query,
                                          color_assign,
                                          _ctk_css_color_value_new_current_color ());
  ctk_css_style_property_register        ("-ctk-secondary-caret-color",
                                          CTK_CSS_PROPERTY_SECONDARY_CARET_COLOR,
                                          CDK_TYPE_RGBA,
                                          CTK_STYLE_PROPERTY_INHERIT | CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_TEXT,
                                          color_parse,
                                          color_query,
                                          color_assign,
                                          _ctk_css_color_value_new_current_color ());
  ctk_css_style_property_register        ("font-feature-settings",
                                          CTK_CSS_PROPERTY_FONT_FEATURE_SETTINGS,
                                          G_TYPE_NONE,
                                          CTK_STYLE_PROPERTY_INHERIT | CTK_STYLE_PROPERTY_ANIMATED,
                                          CTK_CSS_AFFECTS_TEXT | CTK_CSS_AFFECTS_TEXT_ATTRS,
                                          parse_font_feature_settings,
                                          NULL,
                                          NULL,
                                          _ctk_css_string_value_new (""));
}
