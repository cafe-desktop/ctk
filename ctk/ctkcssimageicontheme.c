/*
 * Copyright © 2012 Red Hat Inc.
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
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "ctkcssimageiconthemeprivate.h"

#include <math.h>

#include "ctkcssiconthemevalueprivate.h"
#include "ctkcssrgbavalueprivate.h"
#include "ctksettingsprivate.h"
#include "ctkstyleproviderprivate.h"
#include "ctkiconthemeprivate.h"

G_DEFINE_TYPE (CtkCssImageIconTheme, _ctk_css_image_icon_theme, CTK_TYPE_CSS_IMAGE)

static double
ctk_css_image_icon_theme_get_aspect_ratio (CtkCssImage *image)
{
  /* icon theme icons only take a single size when requesting, so we insist on being square */
  return 1.0;
}

static void
ctk_css_image_icon_theme_draw (CtkCssImage        *image,
                               cairo_t            *cr,
                               double              width,
                               double              height)
{
  CtkCssImageIconTheme *icon_theme = CTK_CSS_IMAGE_ICON_THEME (image);
  GError *error = NULL;
  CtkIconInfo *icon_info;
  GdkPixbuf *pixbuf;
  gint size;

  size = floor (MIN (width, height));
  if (size <= 0)
    return;

  icon_info = ctk_icon_theme_lookup_icon_for_scale (icon_theme->icon_theme,
                                                    icon_theme->name,
                                                    size,
                                                    icon_theme->scale,
                                                    CTK_ICON_LOOKUP_USE_BUILTIN);
  if (icon_info == NULL)
    {
      /* XXX: render missing icon image here? */
      return;
    }

  pixbuf = ctk_icon_info_load_symbolic (icon_info,
                                        &icon_theme->color,
                                        &icon_theme->success,
                                        &icon_theme->warning,
                                        &icon_theme->error,
                                        NULL,
                                        &error);
  if (pixbuf == NULL)
    {
      /* XXX: render missing icon image here? */
      g_error_free (error);
      return;
    }

  cairo_translate (cr, width / 2.0, height / 2.0);
  cairo_scale (cr, 1.0 / icon_theme->scale, 1.0 / icon_theme->scale);
  gdk_cairo_set_source_pixbuf (cr,
                               pixbuf,
                               - gdk_pixbuf_get_width (pixbuf) / 2.0,
                               - gdk_pixbuf_get_height (pixbuf) / 2.0);
  cairo_paint (cr);

  g_object_unref (pixbuf);
  g_object_unref (icon_info);
}


static gboolean
ctk_css_image_icon_theme_parse (CtkCssImage  *image,
                                CtkCssParser *parser)
{
  CtkCssImageIconTheme *icon_theme = CTK_CSS_IMAGE_ICON_THEME (image);

  if (!_ctk_css_parser_try (parser, "-ctk-icontheme(", TRUE))
    {
      _ctk_css_parser_error (parser, "Expected '-ctk-icontheme('");
      return FALSE;
    }

  icon_theme->name = _ctk_css_parser_read_string (parser);
  if (icon_theme->name == NULL)
    return FALSE;

  if (!_ctk_css_parser_try (parser, ")", TRUE))
    {
      _ctk_css_parser_error (parser, "Missing closing bracket at end of '-ctk-icontheme'");
      return FALSE;
    }

  return TRUE;
}

static void
ctk_css_image_icon_theme_print (CtkCssImage *image,
                                GString     *string)
{
  CtkCssImageIconTheme *icon_theme = CTK_CSS_IMAGE_ICON_THEME (image);

  g_string_append (string, "-ctk-icontheme(");
  _ctk_css_print_string (string, icon_theme->name);
  g_string_append (string, ")");
}

static CtkCssImage *
ctk_css_image_icon_theme_compute (CtkCssImage             *image,
                                  guint                    property_id,
                                  CtkStyleProviderPrivate *provider,
                                  CtkCssStyle             *style,
                                  CtkCssStyle             *parent_style)
{
  CtkCssImageIconTheme *icon_theme = CTK_CSS_IMAGE_ICON_THEME (image);
  CtkCssImageIconTheme *copy;

  copy = g_object_new (CTK_TYPE_CSS_IMAGE_ICON_THEME, NULL);
  copy->name = g_strdup (icon_theme->name);
  copy->icon_theme = ctk_css_icon_theme_value_get_icon_theme (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_ICON_THEME));
  copy->scale = _ctk_style_provider_private_get_scale (provider);
  ctk_icon_theme_lookup_symbolic_colors (style, &copy->color, &copy->success, &copy->warning, &copy->error);

  return CTK_CSS_IMAGE (copy);
}

static gboolean
ctk_css_image_icon_theme_equal (CtkCssImage *image1,
                                CtkCssImage *image2)
{
  CtkCssImageIconTheme *icon_theme1 = CTK_CSS_IMAGE_ICON_THEME (image1);
  CtkCssImageIconTheme *icon_theme2 = CTK_CSS_IMAGE_ICON_THEME (image2);

  return g_str_equal (icon_theme1->name, icon_theme2->name);
}

static void
ctk_css_image_icon_theme_dispose (GObject *object)
{
  CtkCssImageIconTheme *icon_theme = CTK_CSS_IMAGE_ICON_THEME (object);

  g_free (icon_theme->name);
  icon_theme->name = NULL;

  G_OBJECT_CLASS (_ctk_css_image_icon_theme_parent_class)->dispose (object);
}

static void
_ctk_css_image_icon_theme_class_init (CtkCssImageIconThemeClass *klass)
{
  CtkCssImageClass *image_class = CTK_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->get_aspect_ratio = ctk_css_image_icon_theme_get_aspect_ratio;
  image_class->draw = ctk_css_image_icon_theme_draw;
  image_class->parse = ctk_css_image_icon_theme_parse;
  image_class->print = ctk_css_image_icon_theme_print;
  image_class->compute = ctk_css_image_icon_theme_compute;
  image_class->equal = ctk_css_image_icon_theme_equal;

  object_class->dispose = ctk_css_image_icon_theme_dispose;
}

static void
_ctk_css_image_icon_theme_init (CtkCssImageIconTheme *icon_theme)
{
  icon_theme->icon_theme = ctk_icon_theme_get_default ();
  icon_theme->scale = 1;
}

