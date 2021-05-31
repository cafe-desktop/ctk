/*
 * Copyright © 2015 Endless Mobile, Inc.
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
 * Authors: Cosimo Cecchi <cosimoc@gnome.org>
 */

#ifndef __CTK_ICON_PRIVATE_H__
#define __CTK_ICON_PRIVATE_H__

#include "ctk/ctkwidget.h"

G_BEGIN_DECLS

#define CTK_TYPE_ICON           (ctk_icon_get_type ())
#define CTK_ICON(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_ICON, CtkIcon))
#define CTK_ICON_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_ICON, CtkIconClass))
#define CTK_IS_ICON(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_ICON))
#define CTK_IS_ICON_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_ICON))
#define CTK_ICON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ICON, CtkIconClass))

typedef struct _CtkIcon           CtkIcon;
typedef struct _CtkIconClass      CtkIconClass;

struct _CtkIcon
{
  CtkWidget parent;
};

struct _CtkIconClass
{
  CtkWidgetClass  parent_class;
};

GType        ctk_icon_get_type               (void) G_GNUC_CONST;

CtkWidget *  ctk_icon_new                    (const char *css_name);
const char * ctk_icon_get_css_name           (CtkIcon    *icon);
void         ctk_icon_set_css_name           (CtkIcon    *icon,
                                              const char *css_name);

G_END_DECLS

#endif /* __CTK_ICON_PRIVATE_H__ */
