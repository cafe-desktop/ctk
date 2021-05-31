/* CTK - The GIMP Toolkit
 * Copyright (C) 2017-2018, Red Hat, Inc.
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
#ifndef __CTK_GESTURE_STYLUS_H__
#define __CTK_GESTURE_STYLUS_H__

#include <ctk/ctkgesture.h>

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

G_BEGIN_DECLS

#define CTK_TYPE_GESTURE_STYLUS         (ctk_gesture_stylus_get_type ())
#define CTK_GESTURE_STYLUS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_GESTURE_STYLUS, CtkGestureStylus))
#define CTK_GESTURE_STYLUS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_TYPE_GESTURE_STYLUS, CtkGestureStylusClass))
#define CTK_IS_GESTURE_STYLUS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_GESTURE_STYLUS))
#define CTK_IS_GESTURE_STYLUS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_GESTURE_STYLUS))
#define CTK_GESTURE_STYLUS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_GESTURE_STYLUS, CtkGestureStylusClass))

typedef struct _CtkGestureStylus CtkGestureStylus;
typedef struct _CtkGestureStylusClass CtkGestureStylusClass;

GDK_AVAILABLE_IN_3_24
GType             ctk_gesture_stylus_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_24
CtkGesture *      ctk_gesture_stylus_new      (CtkWidget *widget);

GDK_AVAILABLE_IN_3_24
gboolean          ctk_gesture_stylus_get_axis (CtkGestureStylus *gesture,
					       GdkAxisUse        axis,
					       gdouble          *value);
GDK_AVAILABLE_IN_3_24
gboolean          ctk_gesture_stylus_get_axes (CtkGestureStylus  *gesture,
					       GdkAxisUse         axes[],
					       gdouble          **values);
GDK_AVAILABLE_IN_3_24
GdkDeviceTool *   ctk_gesture_stylus_get_device_tool (CtkGestureStylus *gesture);

G_END_DECLS

#endif /* __CTK_GESTURE_STYLUS_H__ */
