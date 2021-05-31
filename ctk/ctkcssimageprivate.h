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

#ifndef __CTK_CSS_IMAGE_PRIVATE_H__
#define __CTK_CSS_IMAGE_PRIVATE_H__

#include <cairo.h>
#include <glib-object.h>

#include "ctk/ctkcssparserprivate.h"
#include "ctk/ctkcsstypesprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_IMAGE           (_ctk_css_image_get_type ())
#define CTK_CSS_IMAGE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_IMAGE, CtkCssImage))
#define CTK_CSS_IMAGE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_IMAGE, CtkCssImageClass))
#define CTK_IS_CSS_IMAGE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_IMAGE))
#define CTK_IS_CSS_IMAGE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_IMAGE))
#define CTK_CSS_IMAGE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_IMAGE, CtkCssImageClass))

typedef struct _CtkCssImage           CtkCssImage;
typedef struct _CtkCssImageClass      CtkCssImageClass;

struct _CtkCssImage
{
  GObject parent;
};

struct _CtkCssImageClass
{
  GObjectClass parent_class;

  /* width of image or 0 if it has no width (optional) */
  int          (* get_width)                       (CtkCssImage                *image);
  /* height of image or 0 if it has no height (optional) */
  int          (* get_height)                      (CtkCssImage                *image);
  /* aspect ratio (width / height) of image or 0 if it has no aspect ratio (optional) */
  double       (* get_aspect_ratio)                (CtkCssImage                *image);

  /* create "computed value" in CSS terms, returns a new reference */
  CtkCssImage *(* compute)                         (CtkCssImage                *image,
                                                    guint                       property_id,
                                                    CtkStyleProviderPrivate    *provider,
                                                    CtkCssStyle                *style,
                                                    CtkCssStyle                *parent_style);
  /* compare two images for equality */
  gboolean     (* equal)                           (CtkCssImage                *image1,
                                                    CtkCssImage                *image2);
  /* transition between start and end image (end may be NULL), returns new reference (optional) */
  CtkCssImage *(* transition)                      (CtkCssImage                *start,
                                                    CtkCssImage                *end,
                                                    guint                       property_id,
                                                    double                      progress);

  /* draw to 0,0 with the given width and height */
  void         (* draw)                            (CtkCssImage                *image,
                                                    cairo_t                    *cr,
                                                    double                      width,
                                                    double                      height);
  /* parse CSS, return TRUE on success */
  gboolean     (* parse)                           (CtkCssImage                *image,
                                                    CtkCssParser               *parser);
  /* print to CSS */
  void         (* print)                           (CtkCssImage                *image,
                                                    GString                    *string);
};

GType          _ctk_css_image_get_type             (void) G_GNUC_CONST;

gboolean       _ctk_css_image_can_parse            (CtkCssParser               *parser);
CtkCssImage *  _ctk_css_image_new_parse            (CtkCssParser               *parser);

int            _ctk_css_image_get_width            (CtkCssImage                *image);
int            _ctk_css_image_get_height           (CtkCssImage                *image);
double         _ctk_css_image_get_aspect_ratio     (CtkCssImage                *image);

CtkCssImage *  _ctk_css_image_compute              (CtkCssImage                *image,
                                                    guint                       property_id,
                                                    CtkStyleProviderPrivate    *provider,
                                                    CtkCssStyle                *style,
                                                    CtkCssStyle                *parent_style);
gboolean       _ctk_css_image_equal                (CtkCssImage                *image1,
                                                    CtkCssImage                *image2);
CtkCssImage *  _ctk_css_image_transition           (CtkCssImage                *start,
                                                    CtkCssImage                *end,
                                                    guint                       property_id,
                                                    double                      progress);

void           _ctk_css_image_draw                 (CtkCssImage                *image,
                                                    cairo_t                    *cr,
                                                    double                      width,
                                                    double                      height);
void           _ctk_css_image_print                (CtkCssImage                *image,
                                                    GString                    *string);

void           _ctk_css_image_get_concrete_size    (CtkCssImage                *image,
                                                    double                      specified_width,
                                                    double                      specified_height,
                                                    double                      default_width,
                                                    double                      default_height,
                                                    double                     *concrete_width,
                                                    double                     *concrete_height);
cairo_surface_t *
               _ctk_css_image_get_surface          (CtkCssImage                *image,
                                                    cairo_surface_t            *target,
                                                    int                         surface_width,
                                                    int                         surface_height);

G_END_DECLS

#endif /* __CTK_CSS_IMAGE_PRIVATE_H__ */
