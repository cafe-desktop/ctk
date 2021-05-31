/* CTK - The GIMP Toolkit
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
#ifndef __CTK_GESTURE_PRIVATE_H__
#define __CTK_GESTURE_PRIVATE_H__

#include "ctkeventcontrollerprivate.h"
#include "ctkgesture.h"

struct _CtkGesture
{
  CtkEventController parent_instance;
};

struct _CtkGestureClass
{
  CtkEventControllerClass parent_class;

  gboolean (* check)  (CtkGesture       *gesture);

  void     (* begin)  (CtkGesture       *gesture,
                       GdkEventSequence *sequence);
  void     (* update) (CtkGesture       *gesture,
                       GdkEventSequence *sequence);
  void     (* end)    (CtkGesture       *gesture,
                       GdkEventSequence *sequence);

  void     (* cancel) (CtkGesture       *gesture,
                       GdkEventSequence *sequence);

  void     (* sequence_state_changed) (CtkGesture            *gesture,
                                       GdkEventSequence      *sequence,
                                       CtkEventSequenceState  state);

  /*< private >*/
  gpointer padding[10];
};


G_BEGIN_DECLS

gboolean _ctk_gesture_check                  (CtkGesture       *gesture);

gboolean _ctk_gesture_handled_sequence_press (CtkGesture       *gesture,
                                              GdkEventSequence *sequence);

gboolean _ctk_gesture_get_pointer_emulating_sequence
                                                (CtkGesture        *gesture,
                                                 GdkEventSequence **sequence);

gboolean _ctk_gesture_cancel_sequence        (CtkGesture       *gesture,
                                              GdkEventSequence *sequence);

gboolean _ctk_gesture_get_last_update_time   (CtkGesture       *gesture,
                                              GdkEventSequence *sequence,
                                              guint32          *evtime);

G_END_DECLS

#endif /* __CTK_GESTURE_PRIVATE_H__ */
