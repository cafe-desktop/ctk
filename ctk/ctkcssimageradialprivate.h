/*
 * Copyright © 2015 Red Hat Inc.
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

#ifndef __CTK_CSS_IMAGE_RADIAL_PRIVATE_H__
#define __CTK_CSS_IMAGE_RADIAL_PRIVATE_H__

#include "ctk/ctkcssimageprivate.h"
#include "ctk/ctkcssvalueprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_IMAGE_RADIAL           (_ctk_css_image_radial_get_type ())
#define CTK_CSS_IMAGE_RADIAL(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_IMAGE_RADIAL, CtkCssImageRadial))
#define CTK_CSS_IMAGE_RADIAL_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_IMAGE_RADIAL, CtkCssImageRadialClass))
#define CTK_IS_CSS_IMAGE_RADIAL(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_IMAGE_RADIAL))
#define CTK_IS_CSS_IMAGE_RADIAL_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_IMAGE_RADIAL))
#define CTK_CSS_IMAGE_RADIAL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_IMAGE_RADIAL, CtkCssImageRadialClass))

typedef struct _CtkCssImageRadial           CtkCssImageRadial;
typedef struct _CtkCssImageRadialClass      CtkCssImageRadialClass;
typedef struct _CtkCssImageRadialColorStop  CtkCssImageRadialColorStop;

struct _CtkCssImageRadialColorStop {
  CtkCssValue        *offset;
  CtkCssValue        *color;
};

typedef enum {
  CTK_CSS_EXPLICIT_SIZE,
  CTK_CSS_CLOSEST_SIDE,
  CTK_CSS_FARTHEST_SIDE,
  CTK_CSS_CLOSEST_CORNER,
  CTK_CSS_FARTHEST_CORNER
} CtkCssRadialSize;

struct _CtkCssImageRadial
{
  CtkCssImage parent;

  CtkCssValue *position;
  CtkCssValue *sizes[2];
  GArray *stops;
  CtkCssRadialSize size;
  guint circle : 1;
  guint repeating :1;
};

struct _CtkCssImageRadialClass
{
  CtkCssImageClass parent_class;
};

GType          _ctk_css_image_radial_get_type             (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __CTK_CSS_IMAGE_RADIAL_PRIVATE_H__ */
