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

#ifndef __CTK_CSS_IMAGE_SURFACE_PRIVATE_H__
#define __CTK_CSS_IMAGE_SURFACE_PRIVATE_H__

#include "ctk/ctkcssimageprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_IMAGE_SURFACE           (_ctk_css_image_surface_get_type ())
#define CTK_CSS_IMAGE_SURFACE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_IMAGE_SURFACE, CtkCssImageSurface))
#define CTK_CSS_IMAGE_SURFACE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_IMAGE_SURFACE, CtkCssImageSurfaceClass))
#define CTK_IS_CSS_IMAGE_SURFACE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_IMAGE_SURFACE))
#define CTK_IS_CSS_IMAGE_SURFACE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_IMAGE_SURFACE))
#define CTK_CSS_IMAGE_SURFACE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_IMAGE_SURFACE, CtkCssImageSurfaceClass))

typedef struct _CtkCssImageSurface           CtkCssImageSurface;
typedef struct _CtkCssImageSurfaceClass      CtkCssImageSurfaceClass;

struct _CtkCssImageSurface
{
  CtkCssImage parent;

  cairo_surface_t *surface;             /* the surface we render - guaranteed to be an image surface */
  cairo_surface_t *cache;               /* the scaled surface - to avoid scaling every time we need to draw */
  double width;                         /* original cache width */
  double height;                        /* original cache height */
};

struct _CtkCssImageSurfaceClass
{
  CtkCssImageClass parent_class;
};

GType          _ctk_css_image_surface_get_type             (void) G_GNUC_CONST;

CtkCssImage *  _ctk_css_image_surface_new                  (cairo_surface_t *surface);
CtkCssImage *  _ctk_css_image_surface_new_for_pixbuf       (GdkPixbuf       *pixbuf);

G_END_DECLS

#endif /* __CTK_CSS_IMAGE_SURFACE_PRIVATE_H__ */
