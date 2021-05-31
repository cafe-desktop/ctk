/*
 * Copyright (c) 2014 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CTK_INSPECTOR_MENU_H_
#define _CTK_INSPECTOR_MENU_H_

#include <ctk/ctkbox.h>

#define CTK_TYPE_INSPECTOR_MENU            (ctk_inspector_menu_get_type())
#define CTK_INSPECTOR_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), CTK_TYPE_INSPECTOR_MENU, CtkInspectorMenu))
#define CTK_INSPECTOR_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CTK_TYPE_INSPECTOR_MENU, CtkInspectorMenuClass))
#define CTK_INSPECTOR_IS_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), CTK_TYPE_INSPECTOR_MENU))
#define CTK_INSPECTOR_IS_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), CTK_TYPE_INSPECTOR_MENU))
#define CTK_INSPECTOR_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_INSPECTOR_MENU, CtkInspectorMenuClass))


typedef struct _CtkInspectorMenuPrivate CtkInspectorMenuPrivate;

typedef struct _CtkInspectorMenu
{
  CtkBox parent;
  CtkInspectorMenuPrivate *priv;
} CtkInspectorMenu;

typedef struct _CtkInspectorMenuClass
{
  CtkBoxClass parent;
} CtkInspectorMenuClass;

G_BEGIN_DECLS

GType      ctk_inspector_menu_get_type   (void);
void       ctk_inspector_menu_set_object (CtkInspectorMenu *sl,
                                          GObject          *object);

G_END_DECLS

#endif // _CTK_INSPECTOR_MENU_H_

// vim: set et sw=2 ts=2:
