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

#ifndef __GTK_CSS_IMAGE_GRADIENT_PRIVATE_H__
#define __GTK_CSS_IMAGE_GRADIENT_PRIVATE_H__

#include "gtk/gtkcssimageprivate.h"

#include <gtk/deprecated/gtkgradient.h>

G_BEGIN_DECLS

#define GTK_TYPE_CSS_IMAGE_GRADIENT           (_ctk_css_image_gradient_get_type ())
#define GTK_CSS_IMAGE_GRADIENT(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, GTK_TYPE_CSS_IMAGE_GRADIENT, GtkCssImageGradient))
#define GTK_CSS_IMAGE_GRADIENT_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, GTK_TYPE_CSS_IMAGE_GRADIENT, GtkCssImageGradientClass))
#define GTK_IS_CSS_IMAGE_GRADIENT(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, GTK_TYPE_CSS_IMAGE_GRADIENT))
#define GTK_IS_CSS_IMAGE_GRADIENT_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, GTK_TYPE_CSS_IMAGE_GRADIENT))
#define GTK_CSS_IMAGE_GRADIENT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_CSS_IMAGE_GRADIENT, GtkCssImageGradientClass))

typedef struct _GtkCssImageGradient           GtkCssImageGradient;
typedef struct _GtkCssImageGradientClass      GtkCssImageGradientClass;

struct _GtkCssImageGradient
{
  GtkCssImage parent;

  GtkGradient *gradient;
  cairo_pattern_t *pattern;             /* the resolved gradient */
};

struct _GtkCssImageGradientClass
{
  GtkCssImageClass parent_class;
};

GType          _ctk_css_image_gradient_get_type             (void) G_GNUC_CONST;

/* for lack of a better place to put it */
GtkGradient *  _ctk_gradient_parse                          (GtkCssParser *parser);

G_END_DECLS

#endif /* __GTK_CSS_IMAGE_GRADIENT_PRIVATE_H__ */
