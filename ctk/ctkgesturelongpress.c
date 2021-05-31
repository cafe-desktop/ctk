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

/**
 * SECTION:ctkgesturelongpress
 * @Short_description: "Press and Hold" gesture
 * @Title: CtkGestureLongPress
 *
 * #CtkGestureLongPress is a #CtkGesture implementation able to recognize
 * long presses, triggering the #CtkGestureLongPress::pressed after the
 * timeout is exceeded.
 *
 * If the touchpoint is lifted before the timeout passes, or if it drifts
 * too far of the initial press point, the #CtkGestureLongPress::cancelled
 * signal will be emitted.
 */

#include "config.h"
#include "ctkgesturelongpress.h"
#include "ctkgesturelongpressprivate.h"
#include "ctkgestureprivate.h"
#include "ctkmarshalers.h"
#include "ctkdnd.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"

typedef struct _CtkGestureLongPressPrivate CtkGestureLongPressPrivate;

enum {
  PRESSED,
  CANCELLED,
  N_SIGNALS
};

enum {
  PROP_DELAY_FACTOR = 1
};

struct _CtkGestureLongPressPrivate
{
  gdouble initial_x;
  gdouble initial_y;

  gdouble delay_factor;
  guint timeout_id;
  guint delay;
  guint cancelled : 1;
  guint triggered : 1;
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (CtkGestureLongPress, ctk_gesture_long_press, CTK_TYPE_GESTURE_SINGLE)

static void
ctk_gesture_long_press_init (CtkGestureLongPress *gesture)
{
  CtkGestureLongPressPrivate *priv;

  priv = ctk_gesture_long_press_get_instance_private (CTK_GESTURE_LONG_PRESS (gesture));
  priv->delay_factor = 1.0;
}

static gboolean
ctk_gesture_long_press_check (CtkGesture *gesture)
{
  CtkGestureLongPressPrivate *priv;

  priv = ctk_gesture_long_press_get_instance_private (CTK_GESTURE_LONG_PRESS (gesture));

  if (priv->cancelled)
    return FALSE;

  return CTK_GESTURE_CLASS (ctk_gesture_long_press_parent_class)->check (gesture);
}

static gboolean
_ctk_gesture_long_press_timeout (gpointer user_data)
{
  CtkGestureLongPress *gesture = user_data;
  CtkGestureLongPressPrivate *priv;
  GdkEventSequence *sequence;
  gdouble x, y;

  priv = ctk_gesture_long_press_get_instance_private (gesture);
  sequence = ctk_gesture_get_last_updated_sequence (CTK_GESTURE (gesture));
  ctk_gesture_get_point (CTK_GESTURE (gesture), sequence, &x, &y);

  priv->timeout_id = 0;
  priv->triggered = TRUE;
  g_signal_emit (gesture, signals[PRESSED], 0, x, y);

  return G_SOURCE_REMOVE;
}

static void
ctk_gesture_long_press_begin (CtkGesture       *gesture,
                              GdkEventSequence *sequence)
{
  CtkGestureLongPressPrivate *priv;
  const GdkEvent *event;
  CtkWidget *widget;
  gint delay;

  priv = ctk_gesture_long_press_get_instance_private (CTK_GESTURE_LONG_PRESS (gesture));
  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (gesture, sequence);

  if (!event ||
      (event->type != GDK_BUTTON_PRESS &&
       event->type != GDK_TOUCH_BEGIN))
    return;

  widget = ctk_event_controller_get_widget (CTK_EVENT_CONTROLLER (gesture));
  g_object_get (ctk_widget_get_settings (widget),
                "ctk-long-press-time", &delay,
                NULL);

  delay = (gint)(priv->delay_factor * delay);

  ctk_gesture_get_point (gesture, sequence,
                         &priv->initial_x, &priv->initial_y);
  priv->timeout_id =
    gdk_threads_add_timeout (delay,
                             _ctk_gesture_long_press_timeout,
                             gesture);
}

static void
ctk_gesture_long_press_update (CtkGesture       *gesture,
                               GdkEventSequence *sequence)
{
  CtkGestureLongPressPrivate *priv;
  CtkWidget *widget;
  gdouble x, y;

  widget = ctk_event_controller_get_widget (CTK_EVENT_CONTROLLER (gesture));
  priv = ctk_gesture_long_press_get_instance_private (CTK_GESTURE_LONG_PRESS (gesture));
  ctk_gesture_get_point (gesture, sequence, &x, &y);

  if (ctk_drag_check_threshold (widget, priv->initial_x, priv->initial_y, x, y))
    {
      if (priv->timeout_id)
        {
          g_source_remove (priv->timeout_id);
          priv->timeout_id = 0;
          g_signal_emit (gesture, signals[CANCELLED], 0);
        }

      priv->cancelled = TRUE;
      _ctk_gesture_check (gesture);
    }
}

static void
ctk_gesture_long_press_end (CtkGesture       *gesture,
                            GdkEventSequence *sequence)
{
  CtkGestureLongPressPrivate *priv;

  priv = ctk_gesture_long_press_get_instance_private (CTK_GESTURE_LONG_PRESS (gesture));

  if (priv->timeout_id)
    {
      g_source_remove (priv->timeout_id);
      priv->timeout_id = 0;
      g_signal_emit (gesture, signals[CANCELLED], 0);
    }

  priv->cancelled = priv->triggered = FALSE;
}

static void
ctk_gesture_long_press_cancel (CtkGesture       *gesture,
                               GdkEventSequence *sequence)
{
  ctk_gesture_long_press_end (gesture, sequence);
  CTK_GESTURE_CLASS (ctk_gesture_long_press_parent_class)->cancel (gesture, sequence);
}

static void
ctk_gesture_long_press_sequence_state_changed (CtkGesture            *gesture,
                                               GdkEventSequence      *sequence,
                                               CtkEventSequenceState  state)
{
  if (state == CTK_EVENT_SEQUENCE_DENIED)
    ctk_gesture_long_press_end (gesture, sequence);
}

static void
ctk_gesture_long_press_finalize (GObject *object)
{
  CtkGestureLongPressPrivate *priv;

  priv = ctk_gesture_long_press_get_instance_private (CTK_GESTURE_LONG_PRESS (object));

  if (priv->timeout_id)
    g_source_remove (priv->timeout_id);

  G_OBJECT_CLASS (ctk_gesture_long_press_parent_class)->finalize (object);
}

static void
ctk_gesture_long_press_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  CtkGestureLongPressPrivate *priv;

  priv = ctk_gesture_long_press_get_instance_private (CTK_GESTURE_LONG_PRESS (object));

  switch (property_id)
    {
    case PROP_DELAY_FACTOR:
      g_value_set_double (value, priv->delay_factor);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ctk_gesture_long_press_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  CtkGestureLongPressPrivate *priv;

  priv = ctk_gesture_long_press_get_instance_private (CTK_GESTURE_LONG_PRESS (object));

  switch (property_id)
    {
    case PROP_DELAY_FACTOR:
      priv->delay_factor = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ctk_gesture_long_press_class_init (CtkGestureLongPressClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkGestureClass *gesture_class = CTK_GESTURE_CLASS (klass);

  object_class->finalize = ctk_gesture_long_press_finalize;
  object_class->get_property = ctk_gesture_long_press_get_property;
  object_class->set_property = ctk_gesture_long_press_set_property;

  gesture_class->check = ctk_gesture_long_press_check;
  gesture_class->begin = ctk_gesture_long_press_begin;
  gesture_class->update = ctk_gesture_long_press_update;
  gesture_class->end = ctk_gesture_long_press_end;
  gesture_class->cancel = ctk_gesture_long_press_cancel;
  gesture_class->sequence_state_changed = ctk_gesture_long_press_sequence_state_changed;

  g_object_class_install_property (object_class,
                                   PROP_DELAY_FACTOR,
                                   g_param_spec_double ("delay-factor",
                                                        P_("Delay factor"),
                                                        P_("Factor by which to modify the default timeout"),
                                                        0.5, 2.0, 1.0,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * CtkGestureLongPress::pressed:
   * @gesture: the object which received the signal
   * @x: the X coordinate where the press happened, relative to the widget allocation
   * @y: the Y coordinate where the press happened, relative to the widget allocation
   *
   * This signal is emitted whenever a press goes unmoved/unreleased longer than
   * what the GTK+ defaults tell.
   *
   * Since: 3.14
   */
  signals[PRESSED] =
    g_signal_new (I_("pressed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGestureLongPressClass, pressed),
                  NULL, NULL,
                  _ctk_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[PRESSED],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__DOUBLE_DOUBLEv);
  /**
   * CtkGestureLongPress::cancelled:
   * @gesture: the object which received the signal
   *
   * This signal is emitted whenever a press moved too far, or was released
   * before #CtkGestureLongPress::pressed happened.
   *
   * Since: 3.14
   */
  signals[CANCELLED] =
    g_signal_new (I_("cancelled"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGestureLongPressClass, cancelled),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

/**
 * ctk_gesture_long_press_new:
 * @widget: a #CtkWidget
 *
 * Returns a newly created #CtkGesture that recognizes long presses.
 *
 * Returns: a newly created #CtkGestureLongPress
 *
 * Since: 3.14
 **/
CtkGesture *
ctk_gesture_long_press_new (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return g_object_new (CTK_TYPE_GESTURE_LONG_PRESS,
                       "widget", widget,
                       NULL);
}
