/*
 * Copyright © 2016 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include "ctkcssimagefallbackprivate.h"
#include "ctkcssimagesurfaceprivate.h"
#include "ctkcsscolorvalueprivate.h"
#include "ctkcssrgbavalueprivate.h"

#include "ctkstyleproviderprivate.h"

G_DEFINE_TYPE (CtkCssImageFallback, _ctk_css_image_fallback, CTK_TYPE_CSS_IMAGE)

static int
ctk_css_image_fallback_get_width (CtkCssImage *image)
{
  CtkCssImageFallback *fallback = CTK_CSS_IMAGE_FALLBACK (image);

  if (fallback->used < 0)
    return 0;

  return _ctk_css_image_get_width (fallback->images[fallback->used]);
}

static int
ctk_css_image_fallback_get_height (CtkCssImage *image)
{
  CtkCssImageFallback *fallback = CTK_CSS_IMAGE_FALLBACK (image);

  if (fallback->used < 0)
    return 0;

  return _ctk_css_image_get_height (fallback->images[fallback->used]);
}

static double
ctk_css_image_fallback_get_aspect_ratio (CtkCssImage *image)
{
  CtkCssImageFallback *fallback = CTK_CSS_IMAGE_FALLBACK (image);

  if (fallback->used < 0)
    return 0;

  return _ctk_css_image_get_aspect_ratio (fallback->images[fallback->used]);
}

static void
ctk_css_image_fallback_draw (CtkCssImage *image,
                             cairo_t     *cr,
                             double       width,
                             double       height)
{
  CtkCssImageFallback *fallback = CTK_CSS_IMAGE_FALLBACK (image);

  if (fallback->used < 0)
    {
      if (fallback->color)
        gdk_cairo_set_source_rgba (cr, _ctk_css_rgba_value_get_rgba (fallback->color));
      else
        cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);

      cairo_rectangle (cr, 0, 0, width, height);
      cairo_fill (cr);
    }
  else
    _ctk_css_image_draw (fallback->images[fallback->used], cr, width, height);
}

static void
ctk_css_image_fallback_print (CtkCssImage *image,
                              GString     *string)
{
  CtkCssImageFallback *fallback = CTK_CSS_IMAGE_FALLBACK (image);
  int i;

  g_string_append (string, "image(");
  for (i = 0; i < fallback->n_images; i++)
    {
      if (i > 0)
        g_string_append (string, ",");
      _ctk_css_image_print (fallback->images[i], string);
    }
  if (fallback->color)
    {
      if (fallback->n_images > 0)
        g_string_append (string, ",");
      _ctk_css_value_print (fallback->color, string);
    }

  g_string_append (string, ")");
}

static void
ctk_css_image_fallback_dispose (GObject *object)
{
  CtkCssImageFallback *fallback = CTK_CSS_IMAGE_FALLBACK (object);
  int i;

  for (i = 0; i < fallback->n_images; i++)
    g_object_unref (fallback->images[i]);
  g_free (fallback->images);
  fallback->images = NULL;

  if (fallback->color)
    {
      _ctk_css_value_unref (fallback->color);
      fallback->color = NULL;
    }

  G_OBJECT_CLASS (_ctk_css_image_fallback_parent_class)->dispose (object);
}


static CtkCssImage *
ctk_css_image_fallback_compute (CtkCssImage             *image,
                                guint                    property_id,
                                CtkStyleProviderPrivate *provider,
                                CtkCssStyle             *style,
                                CtkCssStyle             *parent_style)
{
  CtkCssImageFallback *fallback = CTK_CSS_IMAGE_FALLBACK (image);
  CtkCssImageFallback *copy;
  int i;

  if (fallback->used < 0)
    {
      copy = g_object_new (_ctk_css_image_fallback_get_type (), NULL);
      copy->n_images = fallback->n_images;
      copy->images = g_new (CtkCssImage *, fallback->n_images);
      for (i = 0; i < fallback->n_images; i++)
        {
          copy->images[i] = _ctk_css_image_compute (fallback->images[i],
                                                    property_id,
                                                    provider,
                                                    style,
                                                    parent_style);

          /* Assume that failing to load an image leaves a 0x0 surface image */
          if (CTK_IS_CSS_IMAGE_SURFACE (copy->images[i]) &&
              _ctk_css_image_get_width (copy->images[i]) == 0 &&
              _ctk_css_image_get_height (copy->images[i]) == 0)
            continue;

          if (copy->used < 0)
            copy->used = i;
        }

      if (fallback->color)
        copy->color = _ctk_css_value_compute (fallback->color,
                                              property_id,
                                              provider,
                                              style,
                                              parent_style);
      else
        copy->color = NULL;

      return CTK_CSS_IMAGE (copy);
    }
  else
    return CTK_CSS_IMAGE (g_object_ref (fallback));
}

static gboolean
ctk_css_image_fallback_parse (CtkCssImage  *image,
                              CtkCssParser *parser)
{
  CtkCssImageFallback *fallback = CTK_CSS_IMAGE_FALLBACK (image);
  GPtrArray *images;
  CtkCssImage *child;

  if (!_ctk_css_parser_try (parser, "image", TRUE))
    {
      _ctk_css_parser_error (parser, "'image'");
      return FALSE;
    }

  if (!_ctk_css_parser_try (parser, "(", TRUE))
    {
      _ctk_css_parser_error (parser,
                             "Expected '(' after 'image'");
      return FALSE;
    }

  images = g_ptr_array_new_with_free_func (g_object_unref);

  do
    {
      child = NULL;
      if (_ctk_css_image_can_parse (parser))
        child = _ctk_css_image_new_parse (parser);
      if (child == NULL)
        {
          fallback->color = _ctk_css_color_value_parse (parser);
          if (fallback->color)
            break;

          g_ptr_array_free (images, TRUE);
          return FALSE;
        }
      g_ptr_array_add (images, child);
    }
  while ( _ctk_css_parser_try (parser, ",", TRUE));

  if (!_ctk_css_parser_try (parser, ")", TRUE))
    {
      g_ptr_array_free (images, TRUE);
      _ctk_css_parser_error (parser,
                             "Expected ')' at end of 'image'");
      return FALSE;
    }

  fallback->n_images = images->len;
  fallback->images = (CtkCssImage **) g_ptr_array_free (images, FALSE);

  return TRUE;
}

static void
_ctk_css_image_fallback_class_init (CtkCssImageFallbackClass *klass)
{
  CtkCssImageClass *image_class = CTK_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->get_width = ctk_css_image_fallback_get_width;
  image_class->get_height = ctk_css_image_fallback_get_height;
  image_class->get_aspect_ratio = ctk_css_image_fallback_get_aspect_ratio;
  image_class->draw = ctk_css_image_fallback_draw;
  image_class->parse = ctk_css_image_fallback_parse;
  image_class->compute = ctk_css_image_fallback_compute;
  image_class->print = ctk_css_image_fallback_print;

  object_class->dispose = ctk_css_image_fallback_dispose;
}

static void
_ctk_css_image_fallback_init (CtkCssImageFallback *image_fallback)
{
  image_fallback->used = -1;
}
