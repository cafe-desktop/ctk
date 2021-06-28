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

#ifndef __CTK_CSS_IMAGE_BUILTIN_PRIVATE_H__
#define __CTK_CSS_IMAGE_BUILTIN_PRIVATE_H__

#include "ctk/ctkcssimageprivate.h"
#include "ctk/ctkicontheme.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_IMAGE_BUILTIN           (ctk_css_image_builtin_get_type ())
#define CTK_CSS_IMAGE_BUILTIN(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_IMAGE_BUILTIN, CtkCssImageBuiltin))
#define CTK_CSS_IMAGE_BUILTIN_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_IMAGE_BUILTIN, CtkCssImageBuiltinClass))
#define CTK_IS_CSS_IMAGE_BUILTIN(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_IMAGE_BUILTIN))
#define CTK_IS_CSS_IMAGE_BUILTIN_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_IMAGE_BUILTIN))
#define CTK_CSS_IMAGE_BUILTIN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_IMAGE_BUILTIN, CtkCssImageBuiltinClass))

typedef struct _CtkCssImageBuiltin           CtkCssImageBuiltin;
typedef struct _CtkCssImageBuiltinClass      CtkCssImageBuiltinClass;

struct _CtkCssImageBuiltin
{
  CtkCssImage   parent;

  CdkRGBA       fg_color;
  CdkRGBA       bg_color;
};

struct _CtkCssImageBuiltinClass
{
  CtkCssImageClass parent_class;
};

GType          ctk_css_image_builtin_get_type              (void) G_GNUC_CONST;

CtkCssImage *  ctk_css_image_builtin_new                   (void);

void           ctk_css_image_builtin_draw                  (CtkCssImage                 *image,
                                                            cairo_t                     *cr,
                                                            double                       width,
                                                            double                       height,
                                                            CtkCssImageBuiltinType       image_type);

G_END_DECLS

#endif /* __CTK_CSS_IMAGE_BUILTIN_PRIVATE_H__ */
