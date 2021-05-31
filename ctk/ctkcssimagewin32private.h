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

#ifndef __CTK_CSS_IMAGE_WIN32_PRIVATE_H__
#define __CTK_CSS_IMAGE_WIN32_PRIVATE_H__

#include "ctk/ctkcssimageprivate.h"
#include "ctk/ctkwin32themeprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_IMAGE_WIN32           (_ctk_css_image_win32_get_type ())
#define CTK_CSS_IMAGE_WIN32(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_IMAGE_WIN32, CtkCssImageWin32))
#define CTK_CSS_IMAGE_WIN32_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_IMAGE_WIN32, CtkCssImageWin32Class))
#define CTK_IS_CSS_IMAGE_WIN32(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_IMAGE_WIN32))
#define CTK_IS_CSS_IMAGE_WIN32_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_IMAGE_WIN32))
#define CTK_CSS_IMAGE_WIN32_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_IMAGE_WIN32, CtkCssImageWin32Class))

typedef struct _CtkCssImageWin32           CtkCssImageWin32;
typedef struct _CtkCssImageWin32Class      CtkCssImageWin32Class;

struct _CtkCssImageWin32
{
  CtkCssImage parent;

  int part;
  int state;

  double over_alpha;
  int part2;
  int state2;

  gint margins[4];

  CtkWin32Theme *theme;
};

struct _CtkCssImageWin32Class
{
  CtkCssImageClass parent_class;
};

GType          _ctk_css_image_win32_get_type             (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __CTK_CSS_IMAGE_WIN32_PRIVATE_H__ */
