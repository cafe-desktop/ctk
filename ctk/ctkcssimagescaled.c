/*
 * Copyright © 2013 Red Hat Inc.
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
 * Authors: Alexander Larsson <alexl@gnome.org>
 */

#include "config.h"

#include "ctkcssimagescaledprivate.h"

#include "ctkstyleproviderprivate.h"

G_DEFINE_TYPE (CtkCssImageScaled, _ctk_css_image_scaled, CTK_TYPE_CSS_IMAGE)

static int
ctk_css_image_scaled_get_width (CtkCssImage *image)
{
  CtkCssImageScaled *scaled = CTK_CSS_IMAGE_SCALED (image);

  return _ctk_css_image_get_width (scaled->images[scaled->scale - 1]) / scaled->scale;
}

static int
ctk_css_image_scaled_get_height (CtkCssImage *image)
{
  CtkCssImageScaled *scaled = CTK_CSS_IMAGE_SCALED (image);

  return _ctk_css_image_get_height (scaled->images[scaled->scale - 1]) / scaled->scale;
}

static double
ctk_css_image_scaled_get_aspect_ratio (CtkCssImage *image)
{
  CtkCssImageScaled *scaled = CTK_CSS_IMAGE_SCALED (image);

  return _ctk_css_image_get_aspect_ratio (scaled->images[scaled->scale - 1]);
}

static void
ctk_css_image_scaled_draw (CtkCssImage *image,
			   cairo_t     *cr,
			   double       width,
			   double       height)
{
  CtkCssImageScaled *scaled = CTK_CSS_IMAGE_SCALED (image);

  _ctk_css_image_draw (scaled->images[scaled->scale - 1], cr, width, height);
}

static void
ctk_css_image_scaled_print (CtkCssImage *image,
                             GString     *string)
{
  CtkCssImageScaled *scaled = CTK_CSS_IMAGE_SCALED (image);
  int i;
  
  g_string_append (string, "-ctk-scaled(");
  for (i = 0; i < scaled->n_images; i++)
    {
      _ctk_css_image_print (scaled->images[i], string);
      if (i != scaled->n_images - 1)
	g_string_append (string, ",");
    }
  g_string_append (string, ")");
}

static void
ctk_css_image_scaled_dispose (GObject *object)
{
  CtkCssImageScaled *scaled = CTK_CSS_IMAGE_SCALED (object);
  int i;

  for (i = 0; i < scaled->n_images; i++)
    g_object_unref (scaled->images[i]);
  g_free (scaled->images);
  scaled->images = NULL;

  G_OBJECT_CLASS (_ctk_css_image_scaled_parent_class)->dispose (object);
}


static CtkCssImage *
ctk_css_image_scaled_compute (CtkCssImage             *image,
			      guint                    property_id,
			      CtkStyleProviderPrivate *provider,
			      CtkCssStyle             *style,
			      CtkCssStyle             *parent_style)
{
  CtkCssImageScaled *scaled = CTK_CSS_IMAGE_SCALED (image);
  int scale;

  scale = _ctk_style_provider_private_get_scale (provider);
  scale = MAX(MIN (scale, scaled->n_images), 1);

  if (scaled->scale == scale)
    return CTK_CSS_IMAGE (g_object_ref (scaled));
  else
    {
      CtkCssImageScaled *copy;
      int i;

      copy = g_object_new (_ctk_css_image_scaled_get_type (), NULL);
      copy->scale = scale;
      copy->n_images = scaled->n_images;
      copy->images = g_new (CtkCssImage *, scaled->n_images);
      for (i = 0; i < scaled->n_images; i++)
        {
          if (i == scale - 1)
            copy->images[i] = _ctk_css_image_compute (scaled->images[i],
                                                      property_id,
                                                      provider,
                                                      style,
                                                      parent_style);
          else
            copy->images[i] = g_object_ref (scaled->images[i]);
        }

      return CTK_CSS_IMAGE (copy);
    }
}

static gboolean
ctk_css_image_scaled_parse (CtkCssImage  *image,
			    CtkCssParser *parser)
{
  CtkCssImageScaled *scaled = CTK_CSS_IMAGE_SCALED (image);
  GPtrArray *images;

  if (!_ctk_css_parser_try (parser, "-ctk-scaled", TRUE))
    {
      _ctk_css_parser_error (parser, "'-ctk-scaled'");
      return FALSE;
    }

  if (!_ctk_css_parser_try (parser, "(", TRUE))
    {
      _ctk_css_parser_error (parser,
                             "Expected '(' after '-ctk-scaled'");
      return FALSE;
    }

  images = g_ptr_array_new_with_free_func (g_object_unref);

  do
    {
      CtkCssImage *child;

      child = _ctk_css_image_new_parse (parser);
      if (child == NULL)
	{
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
                             "Expected ')' at end of '-ctk-scaled'");
      return FALSE;
    }

  scaled->n_images = images->len;
  scaled->images = (CtkCssImage **) g_ptr_array_free (images, FALSE);

  return TRUE;
}

static void
_ctk_css_image_scaled_class_init (CtkCssImageScaledClass *klass)
{
  CtkCssImageClass *image_class = CTK_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->get_width = ctk_css_image_scaled_get_width;
  image_class->get_height = ctk_css_image_scaled_get_height;
  image_class->get_aspect_ratio = ctk_css_image_scaled_get_aspect_ratio;
  image_class->draw = ctk_css_image_scaled_draw;
  image_class->parse = ctk_css_image_scaled_parse;
  image_class->compute = ctk_css_image_scaled_compute;
  image_class->print = ctk_css_image_scaled_print;

  object_class->dispose = ctk_css_image_scaled_dispose;
}

static void
_ctk_css_image_scaled_init (CtkCssImageScaled *image_scaled)
{
  image_scaled->scale = 1;
}
