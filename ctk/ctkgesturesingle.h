/* GTK - The GIMP Toolkit
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

#ifndef __CTK_GESTURE_SINGLE_H__
#define __CTK_GESTURE_SINGLE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkgesture.h>

G_BEGIN_DECLS

#define CTK_TYPE_GESTURE_SINGLE         (ctk_gesture_single_get_type ())
#define CTK_GESTURE_SINGLE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_GESTURE_SINGLE, CtkGestureSingle))
#define CTK_GESTURE_SINGLE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_TYPE_GESTURE_SINGLE, CtkGestureSingleClass))
#define CTK_IS_GESTURE_SINGLE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_GESTURE_SINGLE))
#define CTK_IS_GESTURE_SINGLE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_GESTURE_SINGLE))
#define CTK_GESTURE_SINGLE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_GESTURE_SINGLE, CtkGestureSingleClass))

typedef struct _CtkGestureSingle CtkGestureSingle;
typedef struct _CtkGestureSingleClass CtkGestureSingleClass;

GDK_AVAILABLE_IN_3_14
GType       ctk_gesture_single_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_14
gboolean    ctk_gesture_single_get_touch_only (CtkGestureSingle *gesture);

GDK_AVAILABLE_IN_3_14
void        ctk_gesture_single_set_touch_only (CtkGestureSingle *gesture,
                                               gboolean          touch_only);
GDK_AVAILABLE_IN_3_14
gboolean    ctk_gesture_single_get_exclusive  (CtkGestureSingle *gesture);

GDK_AVAILABLE_IN_3_14
void        ctk_gesture_single_set_exclusive  (CtkGestureSingle *gesture,
                                               gboolean          exclusive);

GDK_AVAILABLE_IN_3_14
guint       ctk_gesture_single_get_button     (CtkGestureSingle *gesture);

GDK_AVAILABLE_IN_3_14
void        ctk_gesture_single_set_button     (CtkGestureSingle *gesture,
                                               guint             button);

GDK_AVAILABLE_IN_3_14
guint       ctk_gesture_single_get_current_button
                                              (CtkGestureSingle *gesture);

GDK_AVAILABLE_IN_3_14
GdkEventSequence * ctk_gesture_single_get_current_sequence
                                              (CtkGestureSingle *gesture);

G_END_DECLS

#endif /* __CTK_GESTURE_SINGLE_H__ */
