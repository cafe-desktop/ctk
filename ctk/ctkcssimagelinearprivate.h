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

#ifndef __CTK_CSS_IMAGE_LINEAR_PRIVATE_H__
#define __CTK_CSS_IMAGE_LINEAR_PRIVATE_H__

#include "ctk/ctkcssimageprivate.h"
#include "ctk/ctkcssvalueprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_IMAGE_LINEAR           (_ctk_css_image_linear_get_type ())
#define CTK_CSS_IMAGE_LINEAR(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_IMAGE_LINEAR, GtkCssImageLinear))
#define CTK_CSS_IMAGE_LINEAR_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_IMAGE_LINEAR, GtkCssImageLinearClass))
#define CTK_IS_CSS_IMAGE_LINEAR(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_IMAGE_LINEAR))
#define CTK_IS_CSS_IMAGE_LINEAR_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_IMAGE_LINEAR))
#define CTK_CSS_IMAGE_LINEAR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_IMAGE_LINEAR, GtkCssImageLinearClass))

typedef struct _GtkCssImageLinear           GtkCssImageLinear;
typedef struct _GtkCssImageLinearClass      GtkCssImageLinearClass;
typedef struct _GtkCssImageLinearColorStop  GtkCssImageLinearColorStop;

struct _GtkCssImageLinearColorStop {
  GtkCssValue        *offset;
  GtkCssValue        *color;
};

struct _GtkCssImageLinear
{
  GtkCssImage parent;

  guint        side;  /* side the gradient should go to or 0 for angle */
  GtkCssValue *angle;
  GArray *stops;
  guint repeating :1;
};

struct _GtkCssImageLinearClass
{
  GtkCssImageClass parent_class;
};

GType          _ctk_css_image_linear_get_type             (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __CTK_CSS_IMAGE_LINEAR_PRIVATE_H__ */
