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

#ifndef _CTK_INSPECTOR_MISC_INFO_H_
#define _CTK_INSPECTOR_MISC_INFO_H_

#include "ctkscrolledwindow.h"

#define CTK_TYPE_INSPECTOR_MISC_INFO            (ctk_inspector_misc_info_get_type())
#define CTK_INSPECTOR_MISC_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), CTK_TYPE_INSPECTOR_MISC_INFO, GtkInspectorMiscInfo))
#define CTK_INSPECTOR_MISC_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CTK_TYPE_INSPECTOR_MISC_INFO, GtkInspectorMiscInfoClass))
#define CTK_INSPECTOR_IS_MISC_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), CTK_TYPE_INSPECTOR_MISC_INFO))
#define CTK_INSPECTOR_IS_MISC_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), CTK_TYPE_INSPECTOR_MISC_INFO))
#define CTK_INSPECTOR_MISC_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_INSPECTOR_MISC_INFO, GtkInspectorMiscInfoClass))


typedef struct _GtkInspectorMiscInfoPrivate GtkInspectorMiscInfoPrivate;

typedef struct _GtkInspectorMiscInfo
{
  GtkScrolledWindow parent;
  GtkInspectorMiscInfoPrivate *priv;
} GtkInspectorMiscInfo;

typedef struct _GtkInspectorMiscInfoClass
{
  GtkScrolledWindowClass parent;
} GtkInspectorMiscInfoClass;

G_BEGIN_DECLS

GType ctk_inspector_misc_info_get_type   (void);
void  ctk_inspector_misc_info_set_object (GtkInspectorMiscInfo *sl,
                                          GObject              *object);

G_END_DECLS

#endif // _CTK_INSPECTOR_MISC_INFO_H_

// vim: set et sw=2 ts=2:
