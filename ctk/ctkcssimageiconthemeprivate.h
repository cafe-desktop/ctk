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

#ifndef __CTK_CSS_IMAGE_ICON_THEME_PRIVATE_H__
#define __CTK_CSS_IMAGE_ICON_THEME_PRIVATE_H__

#include "ctk/ctkcssimageprivate.h"
#include "ctk/ctkicontheme.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_IMAGE_ICON_THEME           (_ctk_css_image_icon_theme_get_type ())
#define CTK_CSS_IMAGE_ICON_THEME(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_IMAGE_ICON_THEME, CtkCssImageIconTheme))
#define CTK_CSS_IMAGE_ICON_THEME_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_IMAGE_ICON_THEME, CtkCssImageIconThemeClass))
#define CTK_IS_CSS_IMAGE_ICON_THEME(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_IMAGE_ICON_THEME))
#define CTK_IS_CSS_IMAGE_ICON_THEME_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_IMAGE_ICON_THEME))
#define CTK_CSS_IMAGE_ICON_THEME_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_IMAGE_ICON_THEME, CtkCssImageIconThemeClass))

typedef struct _CtkCssImageIconTheme           CtkCssImageIconTheme;
typedef struct _CtkCssImageIconThemeClass      CtkCssImageIconThemeClass;

struct _CtkCssImageIconTheme
{
  CtkCssImage parent;

  CtkIconTheme *icon_theme;
  CdkRGBA color;
  CdkRGBA success;
  CdkRGBA warning;
  CdkRGBA error;
  gint scale;
  char *name;
};

struct _CtkCssImageIconThemeClass
{
  CtkCssImageClass parent_class;
};

GType          _ctk_css_image_icon_theme_get_type             (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __CTK_CSS_IMAGE_ICON_THEME_PRIVATE_H__ */
