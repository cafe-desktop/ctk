/* CTK - The GIMP Toolkit
 * Copyright (C) 2012, One Laptop Per Child.
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
#ifndef __CTK_GESTURE_ZOOM_H__
#define __CTK_GESTURE_ZOOM_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkgesture.h>
#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_GESTURE_ZOOM         (ctk_gesture_zoom_get_type ())
#define CTK_GESTURE_ZOOM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_GESTURE_ZOOM, CtkGestureZoom))
#define CTK_GESTURE_ZOOM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_TYPE_GESTURE_ZOOM, CtkGestureZoomClass))
#define CTK_IS_GESTURE_ZOOM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_GESTURE_ZOOM))
#define CTK_IS_GESTURE_ZOOM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_GESTURE_ZOOM))
#define CTK_GESTURE_ZOOM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_GESTURE_ZOOM, CtkGestureZoomClass))

typedef struct _CtkGestureZoom CtkGestureZoom;
typedef struct _CtkGestureZoomClass CtkGestureZoomClass;

CDK_AVAILABLE_IN_3_14
GType        ctk_gesture_zoom_get_type        (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_14
CtkGesture * ctk_gesture_zoom_new             (CtkWidget      *widget);

CDK_AVAILABLE_IN_3_14
gdouble      ctk_gesture_zoom_get_scale_delta (CtkGestureZoom *gesture);

G_END_DECLS

#endif /* __CTK_GESTURE_ZOOM_H__ */
