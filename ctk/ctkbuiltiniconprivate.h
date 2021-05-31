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
#define CTK_BUILTIN_ICON(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_BUILTIN_ICON, GtkBuiltinIcon))
#define CTK_BUILTIN_ICON_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_BUILTIN_ICON, GtkBuiltinIconClass))
#define CTK_IS_BUILTIN_ICON(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_BUILTIN_ICON))
#define CTK_IS_BUILTIN_ICON_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_BUILTIN_ICON))
#define CTK_BUILTIN_ICON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_BUILTIN_ICON, GtkBuiltinIconClass))

typedef struct _GtkBuiltinIcon           GtkBuiltinIcon;
typedef struct _GtkBuiltinIconClass      GtkBuiltinIconClass;

struct _GtkBuiltinIcon
{
  GtkCssGadget parent;
};

struct _GtkBuiltinIconClass
{
  GtkCssGadgetClass  parent_class;
};

GType                   ctk_builtin_icon_get_type               (void) G_GNUC_CONST;

GtkCssGadget *          ctk_builtin_icon_new                    (const char             *name,
                                                                 GtkWidget              *owner,
                                                                 GtkCssGadget           *parent,
                                                                 GtkCssGadget           *next_sibling);
GtkCssGadget *          ctk_builtin_icon_new_for_node           (GtkCssNode             *node,
                                                                 GtkWidget              *owner);

void                    ctk_builtin_icon_set_image              (GtkBuiltinIcon         *icon,
                                                                 GtkCssImageBuiltinType  image);
GtkCssImageBuiltinType  ctk_builtin_icon_get_image              (GtkBuiltinIcon         *icon);
void                    ctk_builtin_icon_set_default_size       (GtkBuiltinIcon         *icon,
                                                                 int                     default_size);
int                     ctk_builtin_icon_get_default_size       (GtkBuiltinIcon         *icon);
void                    ctk_builtin_icon_set_default_size_property (GtkBuiltinIcon      *icon,
                                                                 const char             *property_name);
const char *            ctk_builtin_icon_get_default_size_property (GtkBuiltinIcon      *icon);

G_END_DECLS

#endif /* __CTK_BUILTIN_ICON_PRIVATE_H__ */
