/* GTK - The GIMP Toolkit
 * Copyright (C) 2014 Red Hat, Inc.
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
 * SECTION:gtkgesturemultipress
 * @Short_description: Multipress gesture
 * @Title: GtkGestureMultiPress
 *
 * #GtkGestureMultiPress is a #GtkGesture implementation able to recognize
 * multiple clicks on a nearby zone, which can be listened for through the
 * #GtkGestureMultiPress::pressed signal. Whenever time or distance between
 * clicks exceed the GTK+ defaults, #GtkGestureMultiPress::stopped is emitted,
 * and the click counter is reset.
 *
 * Callers may also restrict the area that is considered valid for a >1
 * touch/button press through ctk_gesture_multi_press_set_area(), so any
 * click happening outside that area is considered to be a first click of
 * its own.
 */

#include "config.h"
#include "gtkgestureprivate.h"
#include "gtkgesturemultipress.h"
#include "gtkgesturemultipressprivate.h"
#include "gtkprivate.h"
#include "gtkintl.h"
#include "gtkmarshalers.h"

typedef struct _GtkGestureMultiPressPrivate GtkGestureMultiPressPrivate;

struct _GtkGestureMultiPressPrivate
{
  GdkRectangle rect;
  GdkDevice *current_device;
  gdouble initial_press_x;
  gdouble initial_press_y;
  guint double_click_timeout_id;
  guint n_presses;
  guint n_release;
  guint current_button;
  guint rect_is_set : 1;
};

enum {
  PRESSED,
  RELEASED,
  STOPPED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (GtkGestureMultiPress, ctk_gesture_multi_press, GTK_TYPE_GESTURE_SINGLE)

static void
ctk_gesture_multi_press_finalize (GObject *object)
{
  GtkGestureMultiPressPrivate *priv;
  GtkGestureMultiPress *gesture;

  gesture = GTK_GESTURE_MULTI_PRESS (object);
  priv = ctk_gesture_multi_press_get_instance_private (gesture);

  if (priv->double_click_timeout_id)
    {
      g_source_remove (priv->double_click_timeout_id);
      priv->double_click_timeout_id = 0;
    }

  G_OBJECT_CLASS (ctk_gesture_multi_press_parent_class)->finalize (object);
}

static gboolean
ctk_gesture_multi_press_check (GtkGesture *gesture)
{
  GtkGestureMultiPress *multi_press;
  GtkGestureMultiPressPrivate *priv;
  GList *sequences;
  gboolean active;

  multi_press = GTK_GESTURE_MULTI_PRESS (gesture);
  priv = ctk_gesture_multi_press_get_instance_private (multi_press);
  sequences = ctk_gesture_get_sequences (gesture);

  active = g_list_length (sequences) == 1 || priv->double_click_timeout_id;
  g_list_free (sequences);

  return active;
}

static void
_ctk_gesture_multi_press_stop (GtkGestureMultiPress *gesture)
{
  GtkGestureMultiPressPrivate *priv;

  priv = ctk_gesture_multi_press_get_instance_private (gesture);

  if (priv->n_presses == 0)
    return;

  priv->current_device = NULL;
  priv->current_button = 0;
  priv->n_presses = 0;
  g_signal_emit (gesture, signals[STOPPED], 0);
  _ctk_gesture_check (GTK_GESTURE (gesture));
}

static gboolean
_double_click_timeout_cb (gpointer user_data)
{
  GtkGestureMultiPress *gesture = user_data;
  GtkGestureMultiPressPrivate *priv;

  priv = ctk_gesture_multi_press_get_instance_private (gesture);
  priv->double_click_timeout_id = 0;
  _ctk_gesture_multi_press_stop (gesture);

  return FALSE;
}

static void
_ctk_gesture_multi_press_update_timeout (GtkGestureMultiPress *gesture)
{
  GtkGestureMultiPressPrivate *priv;
  guint double_click_time;
  GtkSettings *settings;
  GtkWidget *widget;

  priv = ctk_gesture_multi_press_get_instance_private (gesture);

  if (priv->double_click_timeout_id)
    g_source_remove (priv->double_click_timeout_id);

  widget = ctk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture));
  settings = ctk_widget_get_settings (widget);
  g_object_get (settings, "gtk-double-click-time", &double_click_time, NULL);

  priv->double_click_timeout_id =
    gdk_threads_add_timeout (double_click_time,
                             _double_click_timeout_cb,
                             gesture);
}

static gboolean
_ctk_gesture_multi_press_check_within_threshold (GtkGestureMultiPress *gesture,
                                                 gdouble               x,
                                                 gdouble               y)
{
  GtkGestureMultiPressPrivate *priv;
  guint double_click_distance;
  GtkSettings *settings;
  GtkWidget *widget;

  priv = ctk_gesture_multi_press_get_instance_private (gesture);

  if (priv->n_presses == 0)
    return TRUE;

  widget = ctk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture));
  settings = ctk_widget_get_settings (widget);
  g_object_get (settings,
                "gtk-double-click-distance", &double_click_distance,
                NULL);

  if (ABS (priv->initial_press_x - x) < double_click_distance &&
      ABS (priv->initial_press_y - y) < double_click_distance)
    {
      if (!priv->rect_is_set ||
          (x >= priv->rect.x && x < priv->rect.x + priv->rect.width &&
           y >= priv->rect.y && y < priv->rect.y + priv->rect.height))
        return TRUE;
    }

  return FALSE;
}

static void
ctk_gesture_multi_press_begin (GtkGesture       *gesture,
                               GdkEventSequence *sequence)
{
  GtkGestureMultiPress *multi_press;
  GtkGestureMultiPressPrivate *priv;
  guint n_presses, button = 1;
  GdkEventSequence *current;
  const GdkEvent *event;
  GdkDevice *device;
  gdouble x, y;

  if (!ctk_gesture_handles_sequence (gesture, sequence))
    return;

  multi_press = GTK_GESTURE_MULTI_PRESS (gesture);
  priv = ctk_gesture_multi_press_get_instance_private (multi_press);
  event = ctk_gesture_get_last_event (gesture, sequence);
  current = ctk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));
  device = gdk_event_get_source_device (event);

  if (event->type == GDK_BUTTON_PRESS)
    button = event->button.button;
  else if (event->type == GDK_TOUCH_BEGIN)
    button = 1;
  else
    return;

  /* Reset the gesture if the button number changes mid-recognition */
  if (priv->n_presses > 0 &&
      priv->current_button != button)
    _ctk_gesture_multi_press_stop (multi_press);

  /* Reset also if the device changed */
  if (priv->current_device && priv->current_device != device)
    _ctk_gesture_multi_press_stop (multi_press);

  priv->current_device = device;
  priv->current_button = button;
  _ctk_gesture_multi_press_update_timeout (multi_press);
  ctk_gesture_get_point (gesture, current, &x, &y);

  if (!_ctk_gesture_multi_press_check_within_threshold (multi_press, x, y))
    _ctk_gesture_multi_press_stop (multi_press);

  /* Increment later the real counter, just if the gesture is
   * reset on the pressed handler */
  n_presses = priv->n_release = priv->n_presses + 1;

  g_signal_emit (gesture, signals[PRESSED], 0, n_presses, x, y);

  if (priv->n_presses == 0)
    {
      priv->initial_press_x = x;
      priv->initial_press_y = y;
    }

  priv->n_presses++;
}

static void
ctk_gesture_multi_press_update (GtkGesture       *gesture,
                                GdkEventSequence *sequence)
{
  GtkGestureMultiPress *multi_press;
  GdkEventSequence *current;
  gdouble x, y;

  multi_press = GTK_GESTURE_MULTI_PRESS (gesture);
  current = ctk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));
  ctk_gesture_get_point (gesture, current, &x, &y);

  if (!_ctk_gesture_multi_press_check_within_threshold (multi_press, x, y))
    _ctk_gesture_multi_press_stop (multi_press);
}

static void
ctk_gesture_multi_press_end (GtkGesture       *gesture,
                             GdkEventSequence *sequence)
{
  GtkGestureMultiPress *multi_press;
  GtkGestureMultiPressPrivate *priv;
  GdkEventSequence *current;
  gdouble x, y;
  gboolean interpreted;
  GtkEventSequenceState state;

  multi_press = GTK_GESTURE_MULTI_PRESS (gesture);
  priv = ctk_gesture_multi_press_get_instance_private (multi_press);
  current = ctk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));
  interpreted = ctk_gesture_get_point (gesture, current, &x, &y);
  state = ctk_gesture_get_sequence_state (gesture, current);

  if (state != GTK_EVENT_SEQUENCE_DENIED && interpreted)
    g_signal_emit (gesture, signals[RELEASED], 0, priv->n_release, x, y);

  priv->n_release = 0;
}

static void
ctk_gesture_multi_press_cancel (GtkGesture       *gesture,
                                GdkEventSequence *sequence)
{
  _ctk_gesture_multi_press_stop (GTK_GESTURE_MULTI_PRESS (gesture));
  GTK_GESTURE_CLASS (ctk_gesture_multi_press_parent_class)->cancel (gesture, sequence);
}

static void
ctk_gesture_multi_press_reset (GtkEventController *controller)
{
  _ctk_gesture_multi_press_stop (GTK_GESTURE_MULTI_PRESS (controller));
  GTK_EVENT_CONTROLLER_CLASS (ctk_gesture_multi_press_parent_class)->reset (controller);
}

static void
ctk_gesture_multi_press_class_init (GtkGestureMultiPressClass *klass)
{
  GtkEventControllerClass *controller_class = GTK_EVENT_CONTROLLER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkGestureClass *gesture_class = GTK_GESTURE_CLASS (klass);

  object_class->finalize = ctk_gesture_multi_press_finalize;

  gesture_class->check = ctk_gesture_multi_press_check;
  gesture_class->begin = ctk_gesture_multi_press_begin;
  gesture_class->update = ctk_gesture_multi_press_update;
  gesture_class->end = ctk_gesture_multi_press_end;
  gesture_class->cancel = ctk_gesture_multi_press_cancel;

  controller_class->reset = ctk_gesture_multi_press_reset;

  /**
   * GtkGestureMultiPress::pressed:
   * @gesture: the object which received the signal
   * @n_press: how many touch/button presses happened with this one
   * @x: The X coordinate, in widget allocation coordinates
   * @y: The Y coordinate, in widget allocation coordinates
   *
   * This signal is emitted whenever a button or touch press happens.
   *
   * Since: 3.14
   */
  signals[PRESSED] =
    g_signal_new (I_("pressed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkGestureMultiPressClass, pressed),
                  NULL, NULL,
                  _ctk_marshal_VOID__INT_DOUBLE_DOUBLE,
                  G_TYPE_NONE, 3, G_TYPE_INT,
                  G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[PRESSED],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__INT_DOUBLE_DOUBLEv);

  /**
   * GtkGestureMultiPress::released:
   * @gesture: the object which received the signal
   * @n_press: number of press that is paired with this release
   * @x: The X coordinate, in widget allocation coordinates
   * @y: The Y coordinate, in widget allocation coordinates
   *
   * This signal is emitted when a button or touch is released. @n_press
   * will report the number of press that is paired to this event, note
   * that #GtkGestureMultiPress::stopped may have been emitted between the
   * press and its release, @n_press will only start over at the next press.
   *
   * Since: 3.14
   */
  signals[RELEASED] =
    g_signal_new (I_("released"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkGestureMultiPressClass, released),
                  NULL, NULL,
                  _ctk_marshal_VOID__INT_DOUBLE_DOUBLE,
                  G_TYPE_NONE, 3, G_TYPE_INT,
                  G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[RELEASED],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__INT_DOUBLE_DOUBLEv);
  /**
   * GtkGestureMultiPress::stopped:
   * @gesture: the object which received the signal
   *
   * This signal is emitted whenever any time/distance threshold has
   * been exceeded.
   *
   * Since: 3.14
   */
  signals[STOPPED] =
    g_signal_new (I_("stopped"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkGestureMultiPressClass, stopped),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

static void
ctk_gesture_multi_press_init (GtkGestureMultiPress *gesture)
{
}

/**
 * ctk_gesture_multi_press_new:
 * @widget: a #GtkWidget
 *
 * Returns a newly created #GtkGesture that recognizes single and multiple
 * presses.
 *
 * Returns: a newly created #GtkGestureMultiPress
 *
 * Since: 3.14
 **/
GtkGesture *
ctk_gesture_multi_press_new (GtkWidget *widget)
{
  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  return g_object_new (GTK_TYPE_GESTURE_MULTI_PRESS,
                       "widget", widget,
                       NULL);
}

/**
 * ctk_gesture_multi_press_set_area:
 * @gesture: a #GtkGestureMultiPress
 * @rect: (allow-none): rectangle to receive coordinates on
 *
 * If @rect is non-%NULL, the press area will be checked to be
 * confined within the rectangle, otherwise the button count
 * will be reset so the press is seen as being the first one.
 * If @rect is %NULL, the area will be reset to an unrestricted
 * state.
 *
 * Note: The rectangle is only used to determine whether any
 * non-first click falls within the expected area. This is not
 * akin to an input shape.
 *
 * Since: 3.14
 **/
void
ctk_gesture_multi_press_set_area (GtkGestureMultiPress *gesture,
                                  const GdkRectangle   *rect)
{
  GtkGestureMultiPressPrivate *priv;

  g_return_if_fail (GTK_IS_GESTURE_MULTI_PRESS (gesture));

  priv = ctk_gesture_multi_press_get_instance_private (gesture);

  if (!rect)
    priv->rect_is_set = FALSE;
  else
    {
      priv->rect_is_set = TRUE;
      priv->rect = *rect;
    }
}

/**
 * ctk_gesture_multi_press_get_area:
 * @gesture: a #GtkGestureMultiPress
 * @rect: (out): return location for the press area
 *
 * If an area was set through ctk_gesture_multi_press_set_area(),
 * this function will return %TRUE and fill in @rect with the
 * press area. See ctk_gesture_multi_press_set_area() for more
 * details on what the press area represents.
 *
 * Returns: %TRUE if @rect was filled with the press area
 *
 * Since: 3.14
 **/
gboolean
ctk_gesture_multi_press_get_area (GtkGestureMultiPress *gesture,
                                  GdkRectangle         *rect)
{
  GtkGestureMultiPressPrivate *priv;

  g_return_val_if_fail (GTK_IS_GESTURE_MULTI_PRESS (gesture), FALSE);

  priv = ctk_gesture_multi_press_get_instance_private (gesture);

  if (rect)
    {
      if (priv->rect_is_set)
        *rect = priv->rect;
      else
        {
          rect->x = rect->y = G_MININT;
          rect->width = rect->height = G_MAXINT;
        }
    }

  return priv->rect_is_set;
}
