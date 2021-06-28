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

/**
 * SECTION:ctkgesturestylus
 * @Short_description: Gesture for stylus input
 * @Title: CtkGestureStylus
 * @See_also: #CtkGesture, #CtkGestureSingle
 *
 * #CtkGestureStylus is a #CtkGesture implementation specific to stylus
 * input. The provided signals just provide the basic information
 */

#include "config.h"
#include "ctkgesturestylus.h"
#include "ctkgesturestylusprivate.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctkmain.h"

G_DEFINE_TYPE (CtkGestureStylus, ctk_gesture_stylus, CTK_TYPE_GESTURE_SINGLE)

enum {
  PROXIMITY,
  DOWN,
  MOTION,
  UP,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0, };

static gboolean
ctk_gesture_stylus_handle_event (CtkEventController *controller,
                                 const GdkEvent     *event)
{
  GdkModifierType modifiers;
  guint n_signal;
  gdouble x, y;

  CTK_EVENT_CONTROLLER_CLASS (ctk_gesture_stylus_parent_class)->handle_event (controller, event);

  if (!cdk_event_get_device_tool (event))
    return FALSE;
  if (!cdk_event_get_coords (event, &x, &y))
    return FALSE;

  switch ((guint) cdk_event_get_event_type (event))
    {
    case GDK_BUTTON_PRESS:
      n_signal = DOWN;
      break;
    case GDK_BUTTON_RELEASE:
      n_signal = UP;
      break;
    case GDK_MOTION_NOTIFY:
      cdk_event_get_state (event, &modifiers);

      if (modifiers & GDK_BUTTON1_MASK)
        n_signal = MOTION;
      else
        n_signal = PROXIMITY;
      break;
    default:
      return FALSE;
    }

  g_signal_emit (controller, signals[n_signal], 0, x, y);

  return TRUE;
}

static void
ctk_gesture_stylus_class_init (CtkGestureStylusClass *klass)
{
  CtkEventControllerClass *event_controller_class;

  event_controller_class = CTK_EVENT_CONTROLLER_CLASS (klass);
  event_controller_class->handle_event = ctk_gesture_stylus_handle_event;

  signals[PROXIMITY] =
    g_signal_new (I_("proximity"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGestureStylusClass, proximity),
                  NULL, NULL,
                  _ctk_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[PROXIMITY],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__DOUBLE_DOUBLEv);

  signals[DOWN] =
    g_signal_new (I_("down"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGestureStylusClass, down),
                  NULL, NULL,
                  _ctk_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[DOWN],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__DOUBLE_DOUBLEv);

  signals[MOTION] =
    g_signal_new (I_("motion"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGestureStylusClass, motion),
                  NULL, NULL,
                  _ctk_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[MOTION],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__DOUBLE_DOUBLEv);

  signals[UP] =
    g_signal_new (I_("up"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGestureStylusClass, up),
                  NULL, NULL,
                  _ctk_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[UP],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__DOUBLE_DOUBLEv);
}

static void
ctk_gesture_stylus_init (CtkGestureStylus *gesture)
{
}

/**
 * ctk_gesture_stylus_new:
 * @widget: a #CtkWidget
 *
 * Creates a new #CtkGestureStylus.
 *
 * Returns: a newly created stylus gesture
 *
 * Since: 3.24
 **/
CtkGesture *
ctk_gesture_stylus_new (CtkWidget *widget)
{
  return g_object_new (CTK_TYPE_GESTURE_STYLUS,
                       "widget", widget,
                       NULL);
}

static const GdkEvent *
gesture_get_current_event (CtkGestureStylus *gesture)
{
  GdkEventSequence *sequence;

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));

  return ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);
}

/**
 * ctk_gesture_stylus_get_axis:
 * @gesture: a #CtkGestureStylus
 * @axis: requested device axis
 * @value: (out): return location for the axis value
 *
 * Returns the current value for the requested @axis. This function
 * must be called from either the #CtkGestureStylus:down,
 * #CtkGestureStylus:motion, #CtkGestureStylus:up or #CtkGestureStylus:proximity
 * signals.
 *
 * Returns: #TRUE if there is a current value for the axis
 *
 * Since: 3.24
 **/
gboolean
ctk_gesture_stylus_get_axis (CtkGestureStylus *gesture,
			     GdkAxisUse        axis,
			     gdouble          *value)
{
  const GdkEvent *event;

  g_return_val_if_fail (CTK_IS_GESTURE_STYLUS (gesture), FALSE);
  g_return_val_if_fail (axis < GDK_AXIS_LAST, FALSE);
  g_return_val_if_fail (value != NULL, FALSE);

  event = gesture_get_current_event (gesture);
  if (!event)
    return FALSE;

  return cdk_event_get_axis (event, axis, value);
}

/**
 * ctk_gesture_stylus_get_axes:
 * @gesture: a CtkGestureStylus
 * @axes: (array): array of requested axes, terminated with #GDK_AXIS_IGNORE
 * @values: (out) (array): return location for the axis values
 *
 * Returns the current values for the requested @axes. This function
 * must be called from either the #CtkGestureStylus:down,
 * #CtkGestureStylus:motion, #CtkGestureStylus:up or #CtkGestureStylus:proximity
 * signals.
 *
 * Returns: #TRUE if there is a current value for the axes
 *
 * Since: 3.24
 **/
gboolean
ctk_gesture_stylus_get_axes (CtkGestureStylus  *gesture,
			     GdkAxisUse         axes[],
			     gdouble          **values)
{
  const GdkEvent *event;
  GArray *array;
  gint i = 0;

  g_return_val_if_fail (CTK_IS_GESTURE_STYLUS (gesture), FALSE);
  g_return_val_if_fail (values != NULL, FALSE);

  event = gesture_get_current_event (gesture);
  if (!event)
    return FALSE;

  array = g_array_new (TRUE, FALSE, sizeof (gdouble));

  while (axes[i] != GDK_AXIS_IGNORE)
    {
      gdouble value;

      if (axes[i] >= GDK_AXIS_LAST)
        {
          g_warning ("Requesting unknown axis %d, did you "
                     "forget to add a last GDK_AXIS_IGNORE axis?",
                     axes[i]);
          g_array_free (array, TRUE);
          return FALSE;
        }

      cdk_event_get_axis (event, axes[i], &value);
      g_array_append_val (array, value);
      i++;
    }

  *values = (gdouble *) g_array_free (array, FALSE);
  return TRUE;
}

/**
 * ctk_gesture_stylus_get_device_tool:
 * @gesture: a #CtkGestureStylus
 *
 * Returns the #GdkDeviceTool currently driving input through this gesture.
 * This function must be called from either the #CtkGestureStylus::down,
 * #CtkGestureStylus::motion, #CtkGestureStylus::up or #CtkGestureStylus::proximity
 * signal handlers.
 *
 * Returns: (nullable) (transfer none): The current stylus tool
 *
 * Since: 3.24
 **/
GdkDeviceTool *
ctk_gesture_stylus_get_device_tool (CtkGestureStylus *gesture)
{
  const GdkEvent *event;

  g_return_val_if_fail (CTK_IS_GESTURE_STYLUS (gesture), FALSE);

  event = gesture_get_current_event (gesture);
  if (!event)
    return NULL;

  return cdk_event_get_device_tool (event);
}
