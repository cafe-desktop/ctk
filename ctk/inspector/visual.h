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

#ifndef _CTK_INSPECTOR_VISUAL_H_
#define _CTK_INSPECTOR_VISUAL_H_

#include <ctk/ctkscrolledwindow.h>

#define CTK_TYPE_INSPECTOR_VISUAL            (ctk_inspector_visual_get_type())
#define CTK_INSPECTOR_VISUAL(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), CTK_TYPE_INSPECTOR_VISUAL, CtkInspectorVisual))
#define CTK_INSPECTOR_VISUAL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CTK_TYPE_INSPECTOR_VISUAL, CtkInspectorVisualClass))
#define CTK_INSPECTOR_IS_VISUAL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), CTK_TYPE_INSPECTOR_VISUAL))
#define CTK_INSPECTOR_IS_VISUAL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), CTK_TYPE_INSPECTOR_VISUAL))
#define CTK_INSPECTOR_VISUAL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_INSPECTOR_VISUAL, CtkInspectorVisualClass))


typedef struct _CtkInspectorVisualPrivate CtkInspectorVisualPrivate;

typedef struct _CtkInspectorVisual
{
  CtkScrolledWindow parent;
  CtkInspectorVisualPrivate *priv;
} CtkInspectorVisual;

typedef struct _CtkInspectorVisualClass
{
  CtkScrolledWindowClass parent;
} CtkInspectorVisualClass;

G_BEGIN_DECLS

GType      ctk_inspector_visual_get_type   (void);

G_END_DECLS

#endif // _CTK_INSPECTOR_VISUAL_H_

// vim: set et sw=2 ts=2:
