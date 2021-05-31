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
#ifndef __CTK_GESTURE_PAN_H__
#define __CTK_GESTURE_PAN_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkenums.h>
#include <gtk/gtkgesturedrag.h>

G_BEGIN_DECLS

#define CTK_TYPE_GESTURE_PAN         (ctk_gesture_pan_get_type ())
#define CTK_GESTURE_PAN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_GESTURE_PAN, GtkGesturePan))
#define CTK_GESTURE_PAN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_TYPE_GESTURE_PAN, GtkGesturePanClass))
#define CTK_IS_GESTURE_PAN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_GESTURE_PAN))
#define CTK_IS_GESTURE_PAN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_GESTURE_PAN))
#define CTK_GESTURE_PAN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_GESTURE_PAN, GtkGesturePanClass))

typedef struct _GtkGesturePan GtkGesturePan;
typedef struct _GtkGesturePanClass GtkGesturePanClass;

GDK_AVAILABLE_IN_3_14
GType             ctk_gesture_pan_get_type        (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_14
GtkGesture *      ctk_gesture_pan_new             (GtkWidget      *widget,
                                                   GtkOrientation  orientation);

GDK_AVAILABLE_IN_3_14
GtkOrientation    ctk_gesture_pan_get_orientation (GtkGesturePan  *gesture);

GDK_AVAILABLE_IN_3_14
void              ctk_gesture_pan_set_orientation (GtkGesturePan  *gesture,
                                                   GtkOrientation  orientation);


G_END_DECLS

#endif /* __CTK_GESTURE_PAN_H__ */