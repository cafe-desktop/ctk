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
#ifndef __CTK_GESTURE_H__
#define __CTK_GESTURE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkeventcontroller.h>
#include <ctk/ctkenums.h>

G_BEGIN_DECLS

#define CTK_TYPE_GESTURE         (ctk_gesture_get_type ())
#define CTK_GESTURE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_GESTURE, CtkGesture))
#define CTK_GESTURE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_TYPE_GESTURE, CtkGestureClass))
#define CTK_IS_GESTURE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_GESTURE))
#define CTK_IS_GESTURE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_GESTURE))
#define CTK_GESTURE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_GESTURE, CtkGestureClass))

typedef struct _CtkGesture CtkGesture;
typedef struct _CtkGestureClass CtkGestureClass;

GDK_AVAILABLE_IN_3_14
GType       ctk_gesture_get_type             (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_14
GdkDevice * ctk_gesture_get_device           (CtkGesture       *gesture);

GDK_AVAILABLE_IN_3_14
gboolean    ctk_gesture_set_state            (CtkGesture            *gesture,
                                              CtkEventSequenceState  state);
GDK_AVAILABLE_IN_3_14
CtkEventSequenceState
            ctk_gesture_get_sequence_state   (CtkGesture            *gesture,
                                              GdkEventSequence      *sequence);
GDK_AVAILABLE_IN_3_14
gboolean    ctk_gesture_set_sequence_state   (CtkGesture            *gesture,
                                              GdkEventSequence      *sequence,
                                              CtkEventSequenceState  state);
GDK_AVAILABLE_IN_3_14
GList     * ctk_gesture_get_sequences        (CtkGesture       *gesture);

GDK_AVAILABLE_IN_3_14
GdkEventSequence * ctk_gesture_get_last_updated_sequence
                                             (CtkGesture       *gesture);

GDK_AVAILABLE_IN_3_14
gboolean    ctk_gesture_handles_sequence     (CtkGesture       *gesture,
                                              GdkEventSequence *sequence);
GDK_AVAILABLE_IN_3_14
const GdkEvent *
            ctk_gesture_get_last_event       (CtkGesture       *gesture,
                                              GdkEventSequence *sequence);
GDK_AVAILABLE_IN_3_14
gboolean    ctk_gesture_get_point            (CtkGesture       *gesture,
                                              GdkEventSequence *sequence,
                                              gdouble          *x,
                                              gdouble          *y);
GDK_AVAILABLE_IN_3_14
gboolean    ctk_gesture_get_bounding_box     (CtkGesture       *gesture,
                                              GdkRectangle     *rect);
GDK_AVAILABLE_IN_3_14
gboolean    ctk_gesture_get_bounding_box_center
                                             (CtkGesture       *gesture,
                                              gdouble          *x,
                                              gdouble          *y);
GDK_AVAILABLE_IN_3_14
gboolean    ctk_gesture_is_active            (CtkGesture       *gesture);

GDK_AVAILABLE_IN_3_14
gboolean    ctk_gesture_is_recognized        (CtkGesture       *gesture);

GDK_AVAILABLE_IN_3_14
GdkWindow * ctk_gesture_get_window           (CtkGesture       *gesture);

GDK_AVAILABLE_IN_3_14
void        ctk_gesture_set_window           (CtkGesture       *gesture,
                                              GdkWindow        *window);

GDK_AVAILABLE_IN_3_14
void        ctk_gesture_group                (CtkGesture       *group_gesture,
                                              CtkGesture       *gesture);
GDK_AVAILABLE_IN_3_14
void        ctk_gesture_ungroup              (CtkGesture       *gesture);

GDK_AVAILABLE_IN_3_14
GList *     ctk_gesture_get_group            (CtkGesture       *gesture);

GDK_AVAILABLE_IN_3_14
gboolean    ctk_gesture_is_grouped_with      (CtkGesture       *gesture,
                                              CtkGesture       *other);

G_END_DECLS

#endif /* __CTK_GESTURE_H__ */
