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
 * SECTION:ctkgestureswipe
 * @Short_description: Swipe gesture
 * @Title: CtkGestureSwipe
 *
 * #CtkGestureSwipe is a #CtkGesture implementation able to recognize
 * swipes, after a press/move/.../move/release sequence happens, the
 * #CtkGestureSwipe::swipe signal will be emitted, providing the velocity
 * and directionality of the sequence at the time it was lifted.
 *
 * If the velocity is desired in intermediate points,
 * ctk_gesture_swipe_get_velocity() can be called on eg. a
 * #CtkGesture::update handler.
 *
 * All velocities are reported in pixels/sec units.
 */

#include "config.h"
#include "ctkgestureswipe.h"
#include "ctkgestureswipeprivate.h"
#include "ctkgestureprivate.h"
#include "ctkmarshalers.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"

#define CAPTURE_THRESHOLD_MS 150

typedef struct _CtkGestureSwipePrivate CtkGestureSwipePrivate;
typedef struct _EventData EventData;

struct _EventData
{
  guint32 evtime;
  GdkPoint point;
};

struct _CtkGestureSwipePrivate
{
  GArray *events;
};

enum {
  SWIPE,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (CtkGestureSwipe, ctk_gesture_swipe, CTK_TYPE_GESTURE_SINGLE)

static void
ctk_gesture_swipe_finalize (GObject *object)
{
  CtkGestureSwipePrivate *priv;

  priv = ctk_gesture_swipe_get_instance_private (CTK_GESTURE_SWIPE (object));
  g_array_free (priv->events, TRUE);

  G_OBJECT_CLASS (ctk_gesture_swipe_parent_class)->finalize (object);
}

static gboolean
ctk_gesture_swipe_filter_event (CtkEventController *controller,
                                const GdkEvent     *event)
{
  /* Let touchpad swipe events go through, only if they match n-points  */
  if (event->type == GDK_TOUCHPAD_SWIPE)
    {
      guint n_points;

      g_object_get (G_OBJECT (controller), "n-points", &n_points, NULL);

      if (event->touchpad_swipe.n_fingers == n_points)
        return FALSE;
      else
        return TRUE;
    }

  return CTK_EVENT_CONTROLLER_CLASS (ctk_gesture_swipe_parent_class)->filter_event (controller, event);
}

static void
_ctk_gesture_swipe_clear_backlog (CtkGestureSwipe *gesture,
                                  guint32          evtime)
{
  CtkGestureSwipePrivate *priv;
  gint i, length = 0;

  priv = ctk_gesture_swipe_get_instance_private (gesture);

  for (i = 0; i < (gint) priv->events->len; i++)
    {
      EventData *data;

      data = &g_array_index (priv->events, EventData, i);

      if (data->evtime >= evtime - CAPTURE_THRESHOLD_MS)
        {
          length = i - 1;
          break;
        }
    }

  if (length > 0)
    g_array_remove_range (priv->events, 0, length);
}

static void
ctk_gesture_swipe_append_event (CtkGestureSwipe  *swipe,
                                GdkEventSequence *sequence)
{
  CtkGestureSwipePrivate *priv;
  EventData new;
  gdouble x, y;

  priv = ctk_gesture_swipe_get_instance_private (swipe);
  _ctk_gesture_get_last_update_time (CTK_GESTURE (swipe), sequence, &new.evtime);
  ctk_gesture_get_point (CTK_GESTURE (swipe), sequence, &x, &y);

  new.point.x = x;
  new.point.y = y;

  _ctk_gesture_swipe_clear_backlog (swipe, new.evtime);
  g_array_append_val (priv->events, new);
}

static void
ctk_gesture_swipe_update (CtkGesture       *gesture,
                          GdkEventSequence *sequence)
{
  CtkGestureSwipe *swipe = CTK_GESTURE_SWIPE (gesture);

  ctk_gesture_swipe_append_event (swipe, sequence);
}

static void
_ctk_gesture_swipe_calculate_velocity (CtkGestureSwipe *gesture,
                                       gdouble         *velocity_x,
                                       gdouble         *velocity_y)
{
  CtkGestureSwipePrivate *priv;
  GdkEventSequence *sequence;
  guint32 evtime, diff_time;
  EventData *start, *end;
  gdouble diff_x, diff_y;

  priv = ctk_gesture_swipe_get_instance_private (gesture);
  *velocity_x = *velocity_y = 0;

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  _ctk_gesture_get_last_update_time (CTK_GESTURE (gesture), sequence, &evtime);
  _ctk_gesture_swipe_clear_backlog (gesture, evtime);

  if (priv->events->len == 0)
    return;

  start = &g_array_index (priv->events, EventData, 0);
  end = &g_array_index (priv->events, EventData, priv->events->len - 1);

  diff_time = end->evtime - start->evtime;
  diff_x = end->point.x - start->point.x;
  diff_y = end->point.y - start->point.y;

  if (diff_time == 0)
    return;

  /* Velocity in pixels/sec */
  *velocity_x = diff_x * 1000 / diff_time;
  *velocity_y = diff_y * 1000 / diff_time;
}

static void
ctk_gesture_swipe_end (CtkGesture       *gesture,
                       GdkEventSequence *sequence)
{
  CtkGestureSwipe *swipe = CTK_GESTURE_SWIPE (gesture);
  CtkGestureSwipePrivate *priv;
  gdouble velocity_x, velocity_y;
  GdkEventSequence *seq;

  seq = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));

  if (ctk_gesture_get_sequence_state (gesture, seq) == CTK_EVENT_SEQUENCE_DENIED)
    return;

  if (ctk_gesture_is_active (gesture))
    return;

  ctk_gesture_swipe_append_event (swipe, sequence);

  priv = ctk_gesture_swipe_get_instance_private (swipe);
  _ctk_gesture_swipe_calculate_velocity (swipe, &velocity_x, &velocity_y);
  g_signal_emit (gesture, signals[SWIPE], 0, velocity_x, velocity_y);

  if (priv->events->len > 0)
    g_array_remove_range (priv->events, 0, priv->events->len);
}

static void
ctk_gesture_swipe_class_init (CtkGestureSwipeClass *klass)
{
  CtkGestureClass *gesture_class = CTK_GESTURE_CLASS (klass);
  CtkEventControllerClass *event_controller_class = CTK_EVENT_CONTROLLER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_gesture_swipe_finalize;

  event_controller_class->filter_event = ctk_gesture_swipe_filter_event;

  gesture_class->update = ctk_gesture_swipe_update;
  gesture_class->end = ctk_gesture_swipe_end;

  /**
   * CtkGestureSwipe::swipe:
   * @gesture: object which received the signal
   * @velocity_x: velocity in the X axis, in pixels/sec
   * @velocity_y: velocity in the Y axis, in pixels/sec
   *
   * This signal is emitted when the recognized gesture is finished, velocity
   * and direction are a product of previously recorded events.
   *
   * Since: 3.14
   */
  signals[SWIPE] =
    g_signal_new (I_("swipe"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGestureSwipeClass, swipe),
                  NULL, NULL,
                  _ctk_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[SWIPE],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__DOUBLE_DOUBLEv);
}

static void
ctk_gesture_swipe_init (CtkGestureSwipe *gesture)
{
  CtkGestureSwipePrivate *priv;

  priv = ctk_gesture_swipe_get_instance_private (gesture);
  priv->events = g_array_new (FALSE, FALSE, sizeof (EventData));
}

/**
 * ctk_gesture_swipe_new:
 * @widget: a #CtkWidget
 *
 * Returns a newly created #CtkGesture that recognizes swipes.
 *
 * Returns: a newly created #CtkGestureSwipe
 *
 * Since: 3.14
 **/
CtkGesture *
ctk_gesture_swipe_new (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return g_object_new (CTK_TYPE_GESTURE_SWIPE,
                       "widget", widget,
                       NULL);
}

/**
 * ctk_gesture_swipe_get_velocity:
 * @gesture: a #CtkGestureSwipe
 * @velocity_x: (out): return value for the velocity in the X axis, in pixels/sec
 * @velocity_y: (out): return value for the velocity in the Y axis, in pixels/sec
 *
 * If the gesture is recognized, this function returns %TRUE and fill in
 * @velocity_x and @velocity_y with the recorded velocity, as per the
 * last event(s) processed.
 *
 * Returns: whether velocity could be calculated
 *
 * Since: 3.14
 **/
gboolean
ctk_gesture_swipe_get_velocity (CtkGestureSwipe *gesture,
                                gdouble         *velocity_x,
                                gdouble         *velocity_y)
{
  gdouble vel_x, vel_y;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), FALSE);

  if (!ctk_gesture_is_recognized (CTK_GESTURE (gesture)))
    return FALSE;

  _ctk_gesture_swipe_calculate_velocity (gesture, &vel_x, &vel_y);

  if (velocity_x)
    *velocity_x = vel_x;
  if (velocity_y)
    *velocity_y = vel_y;

  return TRUE;
}
