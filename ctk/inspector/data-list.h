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

#ifndef _CTK_INSPECTOR_DATA_LIST_H_
#define _CTK_INSPECTOR_DATA_LIST_H_

#include <ctk/ctkbox.h>

#define CTK_TYPE_INSPECTOR_DATA_LIST            (ctk_inspector_data_list_get_type())
#define CTK_INSPECTOR_DATA_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), CTK_TYPE_INSPECTOR_DATA_LIST, CtkInspectorDataList))
#define CTK_INSPECTOR_DATA_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CTK_TYPE_INSPECTOR_DATA_LIST, CtkInspectorDataListClass))
#define CTK_INSPECTOR_IS_DATA_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), CTK_TYPE_INSPECTOR_DATA_LIST))
#define CTK_INSPECTOR_IS_DATA_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), CTK_TYPE_INSPECTOR_DATA_LIST))
#define CTK_INSPECTOR_DATA_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_INSPECTOR_DATA_LIST, CtkInspectorDataListClass))


typedef struct _CtkInspectorDataListPrivate CtkInspectorDataListPrivate;

typedef struct _CtkInspectorDataList
{
  CtkBox parent;
  CtkInspectorDataListPrivate *priv;
} CtkInspectorDataList;

typedef struct _CtkInspectorDataListClass
{
  CtkBoxClass parent;
} CtkInspectorDataListClass;

G_BEGIN_DECLS

GType      ctk_inspector_data_list_get_type   (void);
void       ctk_inspector_data_list_set_object (CtkInspectorDataList *sl,
                                               GObject              *object);

G_END_DECLS

#endif // _CTK_INSPECTOR_DATA_LIST_H_

// vim: set et sw=2 ts=2:
