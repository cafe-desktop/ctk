/* GTK - The GIMP Toolkit
 * Copyright (C) 2014, Red Hat, Inc.
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
#ifndef __CTK_GESTURE_DRAG_H__
#define __CTK_GESTURE_DRAG_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>
#include <ctk/ctkgesturesingle.h>

G_BEGIN_DECLS

#define CTK_TYPE_GESTURE_DRAG         (ctk_gesture_drag_get_type ())
#define CTK_GESTURE_DRAG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_GESTURE_DRAG, CtkGestureDrag))
#define CTK_GESTURE_DRAG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_TYPE_GESTURE_DRAG, CtkGestureDragClass))
#define CTK_IS_GESTURE_DRAG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_GESTURE_DRAG))
#define CTK_IS_GESTURE_DRAG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_GESTURE_DRAG))
#define CTK_GESTURE_DRAG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_GESTURE_DRAG, CtkGestureDragClass))

typedef struct _CtkGestureDrag CtkGestureDrag;
typedef struct _CtkGestureDragClass CtkGestureDragClass;

GDK_AVAILABLE_IN_3_14
GType        ctk_gesture_drag_get_type          (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_14
CtkGesture * ctk_gesture_drag_new               (CtkWidget      *widget);

GDK_AVAILABLE_IN_3_14
gboolean     ctk_gesture_drag_get_start_point   (CtkGestureDrag *gesture,
                                                 gdouble        *x,
                                                 gdouble        *y);
GDK_AVAILABLE_IN_3_14
gboolean     ctk_gesture_drag_get_offset        (CtkGestureDrag *gesture,
                                                 gdouble        *x,
                                                 gdouble        *y);

G_END_DECLS

#endif /* __CTK_GESTURE_DRAG_H__ */
