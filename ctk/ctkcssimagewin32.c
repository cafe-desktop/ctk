/*
 * Copyright © 2011 Red Hat Inc.
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

#include "ctkcssimagewin32private.h"

#include "ctkcssprovider.h"

G_DEFINE_TYPE (CtkCssImageWin32, _ctk_css_image_win32, CTK_TYPE_CSS_IMAGE)

static void
ctk_css_image_win32_draw (CtkCssImage        *image,
                          cairo_t            *cr,
                          double              width,
                          double              height)
{
  CtkCssImageWin32 *wimage = CTK_CSS_IMAGE_WIN32 (image);
  cairo_surface_t *surface;
  int dx, dy;

  surface = ctk_win32_theme_create_surface (wimage->theme, wimage->part, wimage->state, wimage->margins,
				            width, height, &dx, &dy);
  
  if (wimage->state2 >= 0)
    {
      cairo_surface_t *surface2;
      cairo_t *cr2;
      int dx2, dy2;

      surface2 = ctk_win32_theme_create_surface (wimage->theme, wimage->part2, wimage->state2, wimage->margins,
						 width, height, &dx2, &dy2);

      cr2 = cairo_create (surface);

      cairo_set_source_surface (cr2, surface2, dx2 - dx, dy2-dy);
      cairo_paint_with_alpha (cr2, wimage->over_alpha);

      cairo_destroy (cr2);

      cairo_surface_destroy (surface2);
    }

  cairo_set_source_surface (cr, surface, dx, dy);
  cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_NONE);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);

  cairo_surface_destroy (surface);
}

static gboolean
ctk_css_image_win32_parse (CtkCssImage  *image,
                           CtkCssParser *parser)
{
  CtkCssImageWin32 *wimage = CTK_CSS_IMAGE_WIN32 (image);

  if (!_ctk_css_parser_try (parser, "-ctk-win32-theme-part", TRUE))
    {
      _ctk_css_parser_error (parser, "'-ctk-win32-theme-part'");
      return FALSE;
    }
  
  if (!_ctk_css_parser_try (parser, "(", TRUE))
    {
      _ctk_css_parser_error (parser,
                             "Expected '(' after '-ctk-win32-theme-part'");
      return FALSE;
    }
  
  wimage->theme = ctk_win32_theme_parse (parser);
  if (wimage->theme == NULL)
    return FALSE;

  if (! _ctk_css_parser_try (parser, ",", TRUE))
    {
      _ctk_css_parser_error (parser, "Expected ','");
      return FALSE;
    }

  if (!_ctk_css_parser_try_int (parser, &wimage->part))
    {
      _ctk_css_parser_error (parser, "Expected a valid integer value");
      return FALSE;
    }

  if (! _ctk_css_parser_try (parser, ",", TRUE))
    {
      _ctk_css_parser_error (parser, "Expected ','");
      return FALSE;
    }

  if (!_ctk_css_parser_try_int (parser, &wimage->state))
    {
      _ctk_css_parser_error (parser, "Expected a valid integer value");
      return FALSE;
    }

  while ( _ctk_css_parser_try (parser, ",", TRUE))
    {
      if ( _ctk_css_parser_try (parser, "over", TRUE))
        {
          if (!_ctk_css_parser_try (parser, "(", TRUE))
            {
              _ctk_css_parser_error (parser,
                                     "Expected '(' after 'over'");
              return FALSE;
            }

          if (!_ctk_css_parser_try_int (parser, &wimage->part2))
            {
              _ctk_css_parser_error (parser, "Expected a valid integer value");
              return FALSE;
            }

          if (! _ctk_css_parser_try (parser, ",", TRUE))
            {
              _ctk_css_parser_error (parser, "Expected ','");
              return FALSE;
            }

          if (!_ctk_css_parser_try_int (parser, &wimage->state2))
            {
              _ctk_css_parser_error (parser, "Expected a valid integer value");
              return FALSE;
            }

          if ( _ctk_css_parser_try (parser, ",", TRUE))
            {
              if (!_ctk_css_parser_try_double (parser, &wimage->over_alpha))
                {
                  _ctk_css_parser_error (parser, "Expected a valid double value");
                  return FALSE;
                }
            }

          if (!_ctk_css_parser_try (parser, ")", TRUE))
            {
              _ctk_css_parser_error (parser,
                                     "Expected ')' at end of 'over'");
              return FALSE;
            }
        }
      else if ( _ctk_css_parser_try (parser, "margins", TRUE))
        {
          guint i;

          if (!_ctk_css_parser_try (parser, "(", TRUE))
            {
              _ctk_css_parser_error (parser,
                                     "Expected '(' after 'margins'");
              return FALSE;
            }

          for (i = 0; i < 4; i++)
            {
              if (!_ctk_css_parser_try_int (parser, &wimage->margins[i]))
                break;
            }
          
          if (i == 0)
            {
              _ctk_css_parser_error (parser, "Expected valid margins");
              return FALSE;
            }

          if (i == 1)
            wimage->margins[1] = wimage->margins[0];
          if (i <= 2)
            wimage->margins[2] = wimage->margins[1];
          if (i <= 3)
            wimage->margins[3] = wimage->margins[2];
          
          if (!_ctk_css_parser_try (parser, ")", TRUE))
            {
              _ctk_css_parser_error (parser,
                                     "Expected ')' at end of 'margins'");
              return FALSE;
            }
        }
      else
        {
          _ctk_css_parser_error (parser,
                                 "Expected identifier");
          return FALSE;
        }
    }

  if (!_ctk_css_parser_try (parser, ")", TRUE))
    {
      _ctk_css_parser_error (parser,
			     "Expected ')'");
      return FALSE;
    }
  
  return TRUE;
}

static void
ctk_css_image_win32_print (CtkCssImage *image,
                           GString     *string)
{
  CtkCssImageWin32 *wimage = CTK_CSS_IMAGE_WIN32 (image);

  g_string_append (string, "-ctk-win32-theme-part(");
  ctk_win32_theme_print (wimage->theme, string);
  g_string_append_printf (string, ", %d, %d)", wimage->part, wimage->state);
}

static void
ctk_css_image_win32_finalize (GObject *object)
{
  CtkCssImageWin32 *wimage = CTK_CSS_IMAGE_WIN32 (object);

  if (wimage->theme)
    ctk_win32_theme_unref (wimage->theme);

  G_OBJECT_CLASS (_ctk_css_image_win32_parent_class)->finalize (object);
}

static void
_ctk_css_image_win32_class_init (CtkCssImageWin32Class *klass)
{
  CtkCssImageClass *image_class = CTK_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_css_image_win32_finalize;

  image_class->draw = ctk_css_image_win32_draw;
  image_class->parse = ctk_css_image_win32_parse;
  image_class->print = ctk_css_image_win32_print;
}

static void
_ctk_css_image_win32_init (CtkCssImageWin32 *wimage)
{
  wimage->over_alpha = 1.0;
  wimage->part2 = -1;
  wimage->state2 = -1;
}

