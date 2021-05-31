/* GTK - The GIMP Toolkit
 * Copyright (C) 2014 Red Hat, Inc.
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
 *
 * Author(s): Carlos Garnacho <carlosg@gnome.org>
 */
#ifndef __CTK_GESTURE_MULTI_PRESS_H__
#define __CTK_GESTURE_MULTI_PRESS_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>
#include <ctk/ctkgesturesingle.h>

G_BEGIN_DECLS

#define CTK_TYPE_GESTURE_MULTI_PRESS         (ctk_gesture_multi_press_get_type ())
#define CTK_GESTURE_MULTI_PRESS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_GESTURE_MULTI_PRESS, GtkGestureMultiPress))
#define CTK_GESTURE_MULTI_PRESS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_TYPE_GESTURE_MULTI_PRESS, GtkGestureMultiPressClass))
#define CTK_IS_GESTURE_MULTI_PRESS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_GESTURE_MULTI_PRESS))
#define CTK_IS_GESTURE_MULTI_PRESS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_GESTURE_MULTI_PRESS))
#define CTK_GESTURE_MULTI_PRESS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_GESTURE_MULTI_PRESS, GtkGestureMultiPressClass))

typedef struct _GtkGestureMultiPress GtkGestureMultiPress;
typedef struct _GtkGestureMultiPressClass GtkGestureMultiPressClass;

GDK_AVAILABLE_IN_3_14
GType        ctk_gesture_multi_press_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_14
GtkGesture * ctk_gesture_multi_press_new      (GtkWidget            *widget);

GDK_AVAILABLE_IN_3_14
void         ctk_gesture_multi_press_set_area (GtkGestureMultiPress *gesture,
					       const GdkRectangle   *rect);
GDK_AVAILABLE_IN_3_14
gboolean     ctk_gesture_multi_press_get_area (GtkGestureMultiPress *gesture,
                                               GdkRectangle         *rect);

G_END_DECLS

#endif /* __CTK_GESTURE_MULTI_PRESS_H__ */
