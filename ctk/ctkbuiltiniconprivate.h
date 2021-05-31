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
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#ifndef __CTK_BUILTIN_ICON_PRIVATE_H__
#define __CTK_BUILTIN_ICON_PRIVATE_H__

#include "ctk/ctkcssgadgetprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_BUILTIN_ICON           (ctk_builtin_icon_get_type ())
#define CTK_BUILTIN_ICON(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_BUILTIN_ICON, CtkBuiltinIcon))
#define CTK_BUILTIN_ICON_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_BUILTIN_ICON, CtkBuiltinIconClass))
#define CTK_IS_BUILTIN_ICON(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_BUILTIN_ICON))
#define CTK_IS_BUILTIN_ICON_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_BUILTIN_ICON))
#define CTK_BUILTIN_ICON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_BUILTIN_ICON, CtkBuiltinIconClass))

typedef struct _CtkBuiltinIcon           CtkBuiltinIcon;
typedef struct _CtkBuiltinIconClass      CtkBuiltinIconClass;

struct _CtkBuiltinIcon
{
  CtkCssGadget parent;
};

struct _CtkBuiltinIconClass
{
  CtkCssGadgetClass  parent_class;
};

GType                   ctk_builtin_icon_get_type               (void) G_GNUC_CONST;

CtkCssGadget *          ctk_builtin_icon_new                    (const char             *name,
                                                                 CtkWidget              *owner,
                                                                 CtkCssGadget           *parent,
                                                                 CtkCssGadget           *next_sibling);
CtkCssGadget *          ctk_builtin_icon_new_for_node           (CtkCssNode             *node,
                                                                 CtkWidget              *owner);

void                    ctk_builtin_icon_set_image              (CtkBuiltinIcon         *icon,
                                                                 CtkCssImageBuiltinType  image);
CtkCssImageBuiltinType  ctk_builtin_icon_get_image              (CtkBuiltinIcon         *icon);
void                    ctk_builtin_icon_set_default_size       (CtkBuiltinIcon         *icon,
                                                                 int                     default_size);
int                     ctk_builtin_icon_get_default_size       (CtkBuiltinIcon         *icon);
void                    ctk_builtin_icon_set_default_size_property (CtkBuiltinIcon      *icon,
                                                                 const char             *property_name);
const char *            ctk_builtin_icon_get_default_size_property (CtkBuiltinIcon      *icon);

G_END_DECLS

#endif /* __CTK_BUILTIN_ICON_PRIVATE_H__ */
