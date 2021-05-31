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

#ifndef __CTK_CSS_IMAGE_FALLBACK_PRIVATE_H__
#define __CTK_CSS_IMAGE_FALLBACK_PRIVATE_H__

#include "ctk/ctkcssimageprivate.h"
#include "ctk/ctkcssvalueprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_IMAGE_FALLBACK           (_ctk_css_image_fallback_get_type ())
#define CTK_CSS_IMAGE_FALLBACK(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_IMAGE_FALLBACK, GtkCssImageFallback))
#define CTK_CSS_IMAGE_FALLBACK_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_IMAGE_FALLBACK, GtkCssImageFallbackClass))
#define CTK_IS_CSS_IMAGE_FALLBACK(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_IMAGE_FALLBACK))
#define CTK_IS_CSS_IMAGE_FALLBACK_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_IMAGE_FALLBACK))
#define CTK_CSS_IMAGE_FALLBACK_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_IMAGE_FALLBACK, GtkCssImageFallbackClass))

typedef struct _GtkCssImageFallback           GtkCssImageFallback;
typedef struct _GtkCssImageFallbackClass      GtkCssImageFallbackClass;

struct _GtkCssImageFallback
{
  GtkCssImage parent;

  GtkCssImage **images;
  int          n_images;

  int used;

  GtkCssValue *color;
};

struct _GtkCssImageFallbackClass
{
  GtkCssImageClass parent_class;
};

GType          _ctk_css_image_fallback_get_type             (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __CTK_CSS_IMAGE_FALLBACK_PRIVATE_H__ */
