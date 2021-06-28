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

/**
 * SECTION:ctkgesturerotate
 * @Short_description: Rotate gesture
 * @Title: CtkGestureRotate
 * @See_also: #CtkGestureZoom
 *
 * #CtkGestureRotate is a #CtkGesture implementation able to recognize
 * 2-finger rotations, whenever the angle between both handled sequences
 * changes, the #CtkGestureRotate::angle-changed signal is emitted.
 */

#include "config.h"
#include <math.h>
#include "ctkgesturerotate.h"
#include "ctkgesturerotateprivate.h"
#include "ctkmarshalers.h"
#include "ctkintl.h"

typedef struct _CtkGestureRotatePrivate CtkGestureRotatePrivate;

enum {
  ANGLE_CHANGED,
  LAST_SIGNAL
};

struct _CtkGestureRotatePrivate
{
  gdouble initial_angle;
  gdouble accum_touchpad_angle;
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (CtkGestureRotate, ctk_gesture_rotate, CTK_TYPE_GESTURE)

static void
ctk_gesture_rotate_init (CtkGestureRotate *gesture)
{
}

static GObject *
ctk_gesture_rotate_constructor (GType                  type,
                                guint                  n_construct_properties,
                                GObjectConstructParam *construct_properties)
{
  GObject *object;

  object = G_OBJECT_CLASS (ctk_gesture_rotate_parent_class)->constructor (type,
                                                                          n_construct_properties,
                                                                          construct_properties);
  g_object_set (object, "n-points", 2, NULL);

  return object;
}

static gboolean
_ctk_gesture_rotate_get_angle (CtkGestureRotate *rotate,
                               gdouble          *angle)
{
  CtkGestureRotatePrivate *priv;
  const CdkEvent *last_event;
  gdouble x1, y1, x2, y2;
  CtkGesture *gesture;
  gdouble dx, dy;
  GList *sequences = NULL;
  gboolean retval = FALSE;

  gesture = CTK_GESTURE (rotate);
  priv = ctk_gesture_rotate_get_instance_private (rotate);

  if (!ctk_gesture_is_recognized (gesture))
    goto out;

  sequences = ctk_gesture_get_sequences (gesture);
  if (!sequences)
    goto out;

  last_event = ctk_gesture_get_last_event (gesture, sequences->data);
  if (last_event->type == GDK_TOUCHPAD_PINCH &&
      (last_event->touchpad_pinch.phase == GDK_TOUCHPAD_GESTURE_PHASE_BEGIN ||
       last_event->touchpad_pinch.phase == GDK_TOUCHPAD_GESTURE_PHASE_UPDATE ||
       last_event->touchpad_pinch.phase == GDK_TOUCHPAD_GESTURE_PHASE_END))
    {
      *angle = priv->accum_touchpad_angle;
    }
  else
    {
      if (!sequences->next)
        goto out;

      ctk_gesture_get_point (gesture, sequences->data, &x1, &y1);
      ctk_gesture_get_point (gesture, sequences->next->data, &x2, &y2);

      dx = x1 - x2;
      dy = y1 - y2;

      *angle = atan2 (dx, dy);

      /* Invert angle */
      *angle = (2 * G_PI) - *angle;

      /* And constraint it to 0°-360° */
      *angle = fmod (*angle, 2 * G_PI);
    }

  retval = TRUE;

 out:
  g_list_free (sequences);
  return retval;
}

static gboolean
_ctk_gesture_rotate_check_emit (CtkGestureRotate *gesture)
{
  CtkGestureRotatePrivate *priv;
  gdouble angle, delta;

  if (!_ctk_gesture_rotate_get_angle (gesture, &angle))
    return FALSE;

  priv = ctk_gesture_rotate_get_instance_private (gesture);
  delta = angle - priv->initial_angle;

  if (delta < 0)
    delta += 2 * G_PI;

  g_signal_emit (gesture, signals[ANGLE_CHANGED], 0, angle, delta);
  return TRUE;
}

static void
ctk_gesture_rotate_begin (CtkGesture       *gesture,
                          CdkEventSequence *sequence)
{
  CtkGestureRotate *rotate = CTK_GESTURE_ROTATE (gesture);
  CtkGestureRotatePrivate *priv;

  priv = ctk_gesture_rotate_get_instance_private (rotate);
  _ctk_gesture_rotate_get_angle (rotate, &priv->initial_angle);
}

static void
ctk_gesture_rotate_update (CtkGesture       *gesture,
                           CdkEventSequence *sequence)
{
  _ctk_gesture_rotate_check_emit (CTK_GESTURE_ROTATE (gesture));
}

static gboolean
ctk_gesture_rotate_filter_event (CtkEventController *controller,
                                 const CdkEvent     *event)
{
  /* Let 2-finger touchpad pinch events go through */
  if (event->type == GDK_TOUCHPAD_PINCH)
    {
      if (event->touchpad_pinch.n_fingers == 2)
        return FALSE;
      else
        return TRUE;
    }

  return CTK_EVENT_CONTROLLER_CLASS (ctk_gesture_rotate_parent_class)->filter_event (controller, event);
}

static gboolean
ctk_gesture_rotate_handle_event (CtkEventController *controller,
                                 const CdkEvent     *event)
{
  CtkGestureRotate *rotate = CTK_GESTURE_ROTATE (controller);
  CtkGestureRotatePrivate *priv;

  priv = ctk_gesture_rotate_get_instance_private (rotate);

  if (event->type == GDK_TOUCHPAD_PINCH)
    {
      if (event->touchpad_pinch.phase == GDK_TOUCHPAD_GESTURE_PHASE_BEGIN ||
          event->touchpad_pinch.phase == GDK_TOUCHPAD_GESTURE_PHASE_END)
        priv->accum_touchpad_angle = 0;
      else if (event->touchpad_pinch.phase == GDK_TOUCHPAD_GESTURE_PHASE_UPDATE)
        priv->accum_touchpad_angle += event->touchpad_pinch.angle_delta;
    }

  return CTK_EVENT_CONTROLLER_CLASS (ctk_gesture_rotate_parent_class)->handle_event (controller, event);
}

static void
ctk_gesture_rotate_class_init (CtkGestureRotateClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkEventControllerClass *event_controller_class = CTK_EVENT_CONTROLLER_CLASS (klass);
  CtkGestureClass *gesture_class = CTK_GESTURE_CLASS (klass);

  object_class->constructor = ctk_gesture_rotate_constructor;

  event_controller_class->filter_event = ctk_gesture_rotate_filter_event;
  event_controller_class->handle_event = ctk_gesture_rotate_handle_event;

  gesture_class->begin = ctk_gesture_rotate_begin;
  gesture_class->update = ctk_gesture_rotate_update;

  /**
   * CtkGestureRotate::angle-changed:
   * @gesture: the object on which the signal is emitted
   * @angle: Current angle in radians
   * @angle_delta: Difference with the starting angle, in radians
   *
   * This signal is emitted when the angle between both tracked points
   * changes.
   *
   * Since: 3.14
   */
  signals[ANGLE_CHANGED] =
    g_signal_new (I_("angle-changed"),
                  CTK_TYPE_GESTURE_ROTATE,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkGestureRotateClass, angle_changed),
                  NULL, NULL,
                  _ctk_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[ANGLE_CHANGED],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__DOUBLE_DOUBLEv);
}

/**
 * ctk_gesture_rotate_new:
 * @widget: a #CtkWidget
 *
 * Returns a newly created #CtkGesture that recognizes 2-touch
 * rotation gestures.
 *
 * Returns: a newly created #CtkGestureRotate
 *
 * Since: 3.14
 **/
CtkGesture *
ctk_gesture_rotate_new (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return g_object_new (CTK_TYPE_GESTURE_ROTATE,
                       "widget", widget,
                       NULL);
}

/**
 * ctk_gesture_rotate_get_angle_delta:
 * @gesture: a #CtkGestureRotate
 *
 * If @gesture is active, this function returns the angle difference
 * in radians since the gesture was first recognized. If @gesture is
 * not active, 0 is returned.
 *
 * Returns: the angle delta in radians
 *
 * Since: 3.14
 **/
gdouble
ctk_gesture_rotate_get_angle_delta (CtkGestureRotate *gesture)
{
  CtkGestureRotatePrivate *priv;
  gdouble angle;

  g_return_val_if_fail (CTK_IS_GESTURE_ROTATE (gesture), 0.0);

  if (!_ctk_gesture_rotate_get_angle (gesture, &angle))
    return 0.0;

  priv = ctk_gesture_rotate_get_instance_private (gesture);

  return angle - priv->initial_angle;
}
