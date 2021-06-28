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
 * SECTION:ctkgesture
 * @Short_description: Base class for gestures
 * @Title: CtkGesture
 * @See_also: #CtkEventController, #CtkGestureSingle
 *
 * #CtkGesture is the base object for gesture recognition, although this
 * object is quite generalized to serve as a base for multi-touch gestures,
 * it is suitable to implement single-touch and pointer-based gestures (using
 * the special %NULL #CdkEventSequence value for these).
 *
 * The number of touches that a #CtkGesture need to be recognized is controlled
 * by the #CtkGesture:n-points property, if a gesture is keeping track of less
 * or more than that number of sequences, it won't check wether the gesture
 * is recognized.
 *
 * As soon as the gesture has the expected number of touches, the gesture will
 * run the #CtkGesture::check signal regularly on input events until the gesture
 * is recognized, the criteria to consider a gesture as "recognized" is left to
 * #CtkGesture subclasses.
 *
 * A recognized gesture will then emit the following signals:
 * - #CtkGesture::begin when the gesture is recognized.
 * - A number of #CtkGesture::update, whenever an input event is processed.
 * - #CtkGesture::end when the gesture is no longer recognized.
 *
 * ## Event propagation
 *
 * In order to receive events, a gesture needs to either set a propagation phase
 * through ctk_event_controller_set_propagation_phase(), or feed those manually
 * through ctk_event_controller_handle_event().
 *
 * In the capture phase, events are propagated from the toplevel down to the
 * target widget, and gestures that are attached to containers above the widget
 * get a chance to interact with the event before it reaches the target.
 *
 * After the capture phase, CTK+ emits the traditional #CtkWidget::button-press-event,
 * #CtkWidget::button-release-event, #CtkWidget::touch-event, etc signals. Gestures
 * with the %CTK_PHASE_TARGET phase are fed events from the default #CtkWidget::event
 * handlers.
 *
 * In the bubble phase, events are propagated up from the target widget to the
 * toplevel, and gestures that are attached to containers above the widget get
 * a chance to interact with events that have not been handled yet.
 *
 * ## States of a sequence # {#touch-sequence-states}
 *
 * Whenever input interaction happens, a single event may trigger a cascade of
 * #CtkGestures, both across the parents of the widget receiving the event and
 * in parallel within an individual widget. It is a responsibility of the
 * widgets using those gestures to set the state of touch sequences accordingly
 * in order to enable cooperation of gestures around the #CdkEventSequences
 * triggering those.
 *
 * Within a widget, gestures can be grouped through ctk_gesture_group(),
 * grouped gestures synchronize the state of sequences, so calling
 * ctk_gesture_set_sequence_state() on one will effectively propagate
 * the state throughout the group.
 *
 * By default, all sequences start out in the #CTK_EVENT_SEQUENCE_NONE state,
 * sequences in this state trigger the gesture event handler, but event
 * propagation will continue unstopped by gestures.
 *
 * If a sequence enters into the #CTK_EVENT_SEQUENCE_DENIED state, the gesture
 * group will effectively ignore the sequence, letting events go unstopped
 * through the gesture, but the "slot" will still remain occupied while
 * the touch is active.
 *
 * If a sequence enters in the #CTK_EVENT_SEQUENCE_CLAIMED state, the gesture
 * group will grab all interaction on the sequence, by:
 * - Setting the same sequence to #CTK_EVENT_SEQUENCE_DENIED on every other gesture
 *   group within the widget, and every gesture on parent widgets in the propagation
 *   chain.
 * - calling #CtkGesture::cancel on every gesture in widgets underneath in the
 *   propagation chain.
 * - Stopping event propagation after the gesture group handles the event.
 *
 * Note: if a sequence is set early to #CTK_EVENT_SEQUENCE_CLAIMED on
 * #GDK_TOUCH_BEGIN/#GDK_BUTTON_PRESS (so those events are captured before
 * reaching the event widget, this implies #CTK_PHASE_CAPTURE), one similar
 * event will emulated if the sequence changes to #CTK_EVENT_SEQUENCE_DENIED.
 * This way event coherence is preserved before event propagation is unstopped
 * again.
 *
 * Sequence states can't be changed freely, see ctk_gesture_set_sequence_state()
 * to know about the possible lifetimes of a #CdkEventSequence.
 *
 * ## Touchpad gestures
 *
 * On the platforms that support it, #CtkGesture will handle transparently
 * touchpad gesture events. The only precautions users of #CtkGesture should do
 * to enable this support are:
 * - Enabling %GDK_TOUCHPAD_GESTURE_MASK on their #CdkWindows
 * - If the gesture has %CTK_PHASE_NONE, ensuring events of type
 *   %GDK_TOUCHPAD_SWIPE and %GDK_TOUCHPAD_PINCH are handled by the #CtkGesture
 */

#include "config.h"
#include "ctkgesture.h"
#include "ctkwidgetprivate.h"
#include "ctkeventcontrollerprivate.h"
#include "ctkgestureprivate.h"
#include "ctktypebuiltins.h"
#include "ctkprivate.h"
#include "ctkmain.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"

typedef struct _CtkGesturePrivate CtkGesturePrivate;
typedef struct _PointData PointData;

enum {
  PROP_N_POINTS = 1,
  PROP_WINDOW
};

enum {
  BEGIN,
  END,
  UPDATE,
  CANCEL,
  SEQUENCE_STATE_CHANGED,
  N_SIGNALS
};

struct _PointData
{
  CdkEvent *event;
  gdouble widget_x;
  gdouble widget_y;

  /* Acummulators for touchpad events */
  gdouble accum_dx;
  gdouble accum_dy;

  guint press_handled : 1;
  guint state : 2;
};

struct _CtkGesturePrivate
{
  GHashTable *points;
  CdkEventSequence *last_sequence;
  CdkWindow *user_window;
  CdkWindow *window;
  CdkDevice *device;
  GList *group_link;
  guint n_points;
  guint recognized : 1;
  guint touchpad : 1;
};

static guint signals[N_SIGNALS] = { 0 };

#define BUTTONS_MASK (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)

#define EVENT_IS_TOUCHPAD_GESTURE(e) ((e)->type == GDK_TOUCHPAD_SWIPE || \
                                      (e)->type == GDK_TOUCHPAD_PINCH)

GList * _ctk_gesture_get_group_link (CtkGesture *gesture);

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (CtkGesture, ctk_gesture, CTK_TYPE_EVENT_CONTROLLER)

static void
ctk_gesture_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  CtkGesturePrivate *priv = ctk_gesture_get_instance_private (CTK_GESTURE (object));

  switch (prop_id)
    {
    case PROP_N_POINTS:
      g_value_set_uint (value, priv->n_points);
      break;
    case PROP_WINDOW:
      g_value_set_object (value, priv->user_window);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_gesture_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  CtkGesturePrivate *priv = ctk_gesture_get_instance_private (CTK_GESTURE (object));

  switch (prop_id)
    {
    case PROP_N_POINTS:
      priv->n_points = g_value_get_uint (value);
      break;
    case PROP_WINDOW:
      ctk_gesture_set_window (CTK_GESTURE (object),
                              g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_gesture_finalize (GObject *object)
{
  CtkGesture *gesture = CTK_GESTURE (object);
  CtkGesturePrivate *priv = ctk_gesture_get_instance_private (gesture);

  ctk_gesture_ungroup (gesture);
  g_list_free (priv->group_link);

  g_hash_table_destroy (priv->points);

  G_OBJECT_CLASS (ctk_gesture_parent_class)->finalize (object);
}

static guint
_ctk_gesture_get_n_touchpad_points (CtkGesture *gesture,
                                    gboolean    only_active)
{
  CtkGesturePrivate *priv;
  PointData *data;

  priv = ctk_gesture_get_instance_private (gesture);

  if (!priv->touchpad)
    return 0;

  data = g_hash_table_lookup (priv->points, NULL);

  if (!data)
    return 0;

  if (only_active &&
      (data->state == CTK_EVENT_SEQUENCE_DENIED ||
       (data->event->type == GDK_TOUCHPAD_SWIPE &&
        data->event->touchpad_swipe.phase == GDK_TOUCHPAD_GESTURE_PHASE_END) ||
       (data->event->type == GDK_TOUCHPAD_PINCH &&
        data->event->touchpad_pinch.phase == GDK_TOUCHPAD_GESTURE_PHASE_END)))
    return 0;

  switch (data->event->type)
    {
    case GDK_TOUCHPAD_SWIPE:
      return data->event->touchpad_swipe.n_fingers;
    case GDK_TOUCHPAD_PINCH:
      return data->event->touchpad_pinch.n_fingers;
    default:
      return 0;
    }
}

static guint
_ctk_gesture_get_n_touch_points (CtkGesture *gesture,
                                 gboolean    only_active)
{
  CtkGesturePrivate *priv;
  GHashTableIter iter;
  guint n_points = 0;
  PointData *data;

  priv = ctk_gesture_get_instance_private (gesture);
  g_hash_table_iter_init (&iter, priv->points);

  while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &data))
    {
      if (only_active &&
          (data->state == CTK_EVENT_SEQUENCE_DENIED ||
           data->event->type == GDK_TOUCH_END ||
           data->event->type == GDK_BUTTON_RELEASE))
        continue;

      n_points++;
    }

  return n_points;
}

static guint
_ctk_gesture_get_n_physical_points (CtkGesture *gesture,
                                    gboolean    only_active)
{
  CtkGesturePrivate *priv;

  priv = ctk_gesture_get_instance_private (gesture);

  if (priv->touchpad)
    return _ctk_gesture_get_n_touchpad_points (gesture, only_active);
  else
    return _ctk_gesture_get_n_touch_points (gesture, only_active);
}

static gboolean
ctk_gesture_check_impl (CtkGesture *gesture)
{
  CtkGesturePrivate *priv;
  guint n_points;

  priv = ctk_gesture_get_instance_private (gesture);
  n_points = _ctk_gesture_get_n_physical_points (gesture, TRUE);

  return n_points == priv->n_points;
}

static void
_ctk_gesture_set_recognized (CtkGesture       *gesture,
                             gboolean          recognized,
                             CdkEventSequence *sequence)
{
  CtkGesturePrivate *priv;

  priv = ctk_gesture_get_instance_private (gesture);

  if (priv->recognized == recognized)
    return;

  priv->recognized = recognized;

  if (recognized)
    g_signal_emit (gesture, signals[BEGIN], 0, sequence);
  else
    g_signal_emit (gesture, signals[END], 0, sequence);
}

static gboolean
_ctk_gesture_do_check (CtkGesture *gesture)
{
  CtkGestureClass *gesture_class;
  gboolean retval = FALSE;

  gesture_class = CTK_GESTURE_GET_CLASS (gesture);

  if (!gesture_class->check)
    return retval;

  retval = gesture_class->check (gesture);
  return retval;
}

static gboolean
_ctk_gesture_has_matching_touchpoints (CtkGesture *gesture)
{
  CtkGesturePrivate *priv = ctk_gesture_get_instance_private (gesture);
  guint active_n_points, current_n_points;

  current_n_points = _ctk_gesture_get_n_physical_points (gesture, FALSE);
  active_n_points = _ctk_gesture_get_n_physical_points (gesture, TRUE);

  return (active_n_points == priv->n_points &&
          current_n_points == priv->n_points);
}

static gboolean
_ctk_gesture_check_recognized (CtkGesture       *gesture,
                               CdkEventSequence *sequence)
{
  CtkGesturePrivate *priv = ctk_gesture_get_instance_private (gesture);
  gboolean has_matching_touchpoints;

  has_matching_touchpoints = _ctk_gesture_has_matching_touchpoints (gesture);

  if (priv->recognized && !has_matching_touchpoints)
    _ctk_gesture_set_recognized (gesture, FALSE, sequence);
  else if (!priv->recognized && has_matching_touchpoints &&
           _ctk_gesture_do_check (gesture))
    _ctk_gesture_set_recognized (gesture, TRUE, sequence);

  return priv->recognized;
}

/* Finds the first window pertaining to the controller's widget */
static CdkWindow *
_find_widget_window (CtkGesture *gesture,
                     CdkWindow  *window)
{
  CtkWidget *widget, *window_widget;

  widget = ctk_event_controller_get_widget (CTK_EVENT_CONTROLLER (gesture));

  while (window && !cdk_window_is_destroyed (window))
    {
      cdk_window_get_user_data (window, (gpointer*) &window_widget);

      if (window_widget == widget ||
          ctk_widget_get_window (widget) == window)
        return window;

      window = cdk_window_get_effective_parent (window);
    }

  return NULL;
}

static void
_update_touchpad_deltas (PointData *data)
{
  CdkEvent *event = data->event;

  if (!event)
    return;

  if (event->type == GDK_TOUCHPAD_SWIPE)
    {
      if (event->touchpad_swipe.phase == GDK_TOUCHPAD_GESTURE_PHASE_BEGIN)
        data->accum_dx = data->accum_dy = 0;
      else if (event->touchpad_swipe.phase == GDK_TOUCHPAD_GESTURE_PHASE_UPDATE)
        {
          data->accum_dx += event->touchpad_swipe.dx;
          data->accum_dy += event->touchpad_swipe.dy;
        }
    }
  else if (event->type == GDK_TOUCHPAD_PINCH)
    {
      if (event->touchpad_pinch.phase == GDK_TOUCHPAD_GESTURE_PHASE_BEGIN)
        data->accum_dx = data->accum_dy = 0;
      else if (event->touchpad_pinch.phase == GDK_TOUCHPAD_GESTURE_PHASE_UPDATE)
        {
          data->accum_dx += event->touchpad_pinch.dx;
          data->accum_dy += event->touchpad_pinch.dy;
        }
    }
}

static void
_get_event_coordinates (PointData *data,
                        gdouble   *x,
                        gdouble   *y)
{
  gdouble event_x, event_y;

  g_assert (data->event != NULL);

  cdk_event_get_coords (data->event, &event_x, &event_y);
  event_x += data->accum_dx;
  event_y += data->accum_dy;

  if (x)
    *x = event_x;
  if (y)
    *y = event_y;
}

static void
_update_widget_coordinates (CtkGesture *gesture,
                            PointData  *data)
{
  CdkWindow *window, *event_widget_window;
  CtkWidget *event_widget, *widget;
  CtkAllocation allocation;
  gdouble event_x, event_y;
  gint wx, wy, x, y;

  event_widget = ctk_get_event_widget (data->event);

  if (!event_widget)
    return;

  widget = ctk_event_controller_get_widget (CTK_EVENT_CONTROLLER (gesture));
  event_widget_window = ctk_widget_get_window (event_widget);
  _get_event_coordinates (data, &event_x, &event_y);
  window = data->event->any.window;

  while (window && window != event_widget_window)
    {
      cdk_window_get_position (window, &wx, &wy);
      event_x += wx;
      event_y += wy;
      window = cdk_window_get_effective_parent (window);
    }

  if (!window)
    return;

  if (!ctk_widget_get_has_window (event_widget))
    {
      ctk_widget_get_allocation (event_widget, &allocation);
      event_x -= allocation.x;
      event_y -= allocation.y;
    }

  ctk_widget_translate_coordinates (event_widget, widget,
                                    event_x, event_y, &x, &y);
  data->widget_x = x;
  data->widget_y = y;
}

static CtkEventSequenceState
ctk_gesture_get_group_state (CtkGesture       *gesture,
                             CdkEventSequence *sequence)
{
  CtkEventSequenceState state = CTK_EVENT_SEQUENCE_NONE;
  GList *group_elem;

  group_elem = g_list_first (_ctk_gesture_get_group_link (gesture));

  for (; group_elem; group_elem = group_elem->next)
    {
      if (group_elem->data == gesture)
        continue;
      if (!ctk_gesture_handles_sequence (group_elem->data, sequence))
        continue;

      state = ctk_gesture_get_sequence_state (group_elem->data, sequence);
      break;
    }

  return state;
}

static gboolean
_ctk_gesture_update_point (CtkGesture     *gesture,
                           const CdkEvent *event,
                           gboolean        add)
{
  CdkEventSequence *sequence;
  CdkWindow *widget_window;
  CtkGesturePrivate *priv;
  CdkDevice *device;
  gboolean existed, touchpad;
  PointData *data;

  if (!cdk_event_get_coords (event, NULL, NULL))
    return FALSE;

  device = cdk_event_get_device (event);

  if (!device)
    return FALSE;

  priv = ctk_gesture_get_instance_private (gesture);
  widget_window = _find_widget_window (gesture, event->any.window);

  if (!widget_window)
    widget_window = event->any.window;

  touchpad = EVENT_IS_TOUCHPAD_GESTURE (event);

  if (add)
    {
      /* If the event happens with the wrong device, or
       * on the wrong window, ignore.
       */
      if (priv->device && priv->device != device)
        return FALSE;
      if (priv->window && priv->window != widget_window)
        return FALSE;
      if (priv->user_window && priv->user_window != widget_window)
        return FALSE;

      /* Make touchpad and touchscreen gestures mutually exclusive */
      if (touchpad && g_hash_table_size (priv->points) > 0)
        return FALSE;
      else if (!touchpad && priv->touchpad)
        return FALSE;
    }
  else if (!priv->device || !priv->window)
    return FALSE;

  sequence = cdk_event_get_event_sequence (event);
  existed = g_hash_table_lookup_extended (priv->points, sequence,
                                          NULL, (gpointer *) &data);
  if (!existed)
    {
      CtkEventSequenceState group_state;

      if (!add)
        return FALSE;

      if (g_hash_table_size (priv->points) == 0)
        {
          priv->window = widget_window;
          priv->device = device;
          priv->touchpad = touchpad;
        }

      data = g_new0 (PointData, 1);
      g_hash_table_insert (priv->points, sequence, data);

      group_state = ctk_gesture_get_group_state (gesture, sequence);
      ctk_gesture_set_sequence_state (gesture, sequence, group_state);
    }

  if (data->event)
    cdk_event_free (data->event);

  data->event = cdk_event_copy (event);
  _update_touchpad_deltas (data);
  _update_widget_coordinates (gesture, data);

  /* Deny the sequence right away if the expected
   * number of points is exceeded, so this sequence
   * can be tracked with ctk_gesture_handles_sequence().
   */
  if (!existed && _ctk_gesture_get_n_physical_points (gesture, FALSE) > priv->n_points)
    ctk_gesture_set_sequence_state (gesture, sequence,
                                    CTK_EVENT_SEQUENCE_DENIED);

  return TRUE;
}

static void
_ctk_gesture_check_empty (CtkGesture *gesture)
{
  CtkGesturePrivate *priv;

  priv = ctk_gesture_get_instance_private (gesture);

  if (g_hash_table_size (priv->points) == 0)
    {
      priv->window = NULL;
      priv->device = NULL;
      priv->touchpad = FALSE;
    }
}

static void
_ctk_gesture_remove_point (CtkGesture     *gesture,
                           const CdkEvent *event)
{
  CdkEventSequence *sequence;
  CtkGesturePrivate *priv;
  CdkDevice *device;

  sequence = cdk_event_get_event_sequence (event);
  device = cdk_event_get_device (event);
  priv = ctk_gesture_get_instance_private (gesture);

  if (priv->device != device)
    return;

  g_hash_table_remove (priv->points, sequence);
  _ctk_gesture_check_empty (gesture);
}

static void
_ctk_gesture_cancel_all (CtkGesture *gesture)
{
  CdkEventSequence *sequence;
  CtkGesturePrivate *priv;
  GHashTableIter iter;

  priv = ctk_gesture_get_instance_private (gesture);
  g_hash_table_iter_init (&iter, priv->points);

  while (g_hash_table_iter_next (&iter, (gpointer*) &sequence, NULL))
    {
      g_signal_emit (gesture, signals[CANCEL], 0, sequence);
      g_hash_table_iter_remove (&iter);
      _ctk_gesture_check_recognized (gesture, sequence);
    }

  _ctk_gesture_check_empty (gesture);
}

static gboolean
gesture_within_window (CtkGesture *gesture,
                       CdkWindow  *parent)
{
  CdkWindow *window;
  CtkWidget *widget;

  widget = ctk_event_controller_get_widget (CTK_EVENT_CONTROLLER (gesture));
  window = ctk_widget_get_window (widget);

  while (window)
    {
      if (window == parent)
        return TRUE;

      window = cdk_window_get_effective_parent (window);
    }

  return FALSE;
}

static gboolean
ctk_gesture_filter_event (CtkEventController *controller,
                          const CdkEvent     *event)
{
  /* Even though CtkGesture handles these events, we want
   * touchpad gestures disabled by default, it will be
   * subclasses which punch the holes in for the events
   * they can possibly handle.
   */
  return EVENT_IS_TOUCHPAD_GESTURE (event);
}

static gboolean
ctk_gesture_handle_event (CtkEventController *controller,
                          const CdkEvent     *event)
{
  CtkGesture *gesture = CTK_GESTURE (controller);
  CdkEventSequence *sequence;
  CtkGesturePrivate *priv;
  CdkDevice *source_device;
  gboolean was_recognized;

  source_device = cdk_event_get_source_device (event);

  if (!source_device)
    return FALSE;

  priv = ctk_gesture_get_instance_private (gesture);
  sequence = cdk_event_get_event_sequence (event);
  was_recognized = ctk_gesture_is_recognized (gesture);

  if (ctk_gesture_get_sequence_state (gesture, sequence) != CTK_EVENT_SEQUENCE_DENIED)
    priv->last_sequence = sequence;

  if (event->type == GDK_BUTTON_PRESS ||
      event->type == GDK_TOUCH_BEGIN ||
      (event->type == GDK_TOUCHPAD_SWIPE &&
       event->touchpad_swipe.phase == GDK_TOUCHPAD_GESTURE_PHASE_BEGIN) ||
      (event->type == GDK_TOUCHPAD_PINCH &&
       event->touchpad_pinch.phase == GDK_TOUCHPAD_GESTURE_PHASE_BEGIN))
    {
      if (_ctk_gesture_update_point (gesture, event, TRUE))
        {
          gboolean triggered_recognition;

          triggered_recognition =
            !was_recognized && _ctk_gesture_has_matching_touchpoints (gesture);

          if (_ctk_gesture_check_recognized (gesture, sequence))
            {
              PointData *data;

              data = g_hash_table_lookup (priv->points, sequence);

              /* If the sequence was claimed early, the press event will be consumed */
              if (ctk_gesture_get_sequence_state (gesture, sequence) == CTK_EVENT_SEQUENCE_CLAIMED)
                data->press_handled = TRUE;
            }
          else if (triggered_recognition && g_hash_table_size (priv->points) == 0)
            {
              /* Recognition was triggered, but the gesture reset during
               * ::begin emission. Still, recognition was strictly triggered,
               * so the event should be consumed.
               */
              return TRUE;
            }
        }
    }
  else if (event->type == GDK_BUTTON_RELEASE ||
           event->type == GDK_TOUCH_END ||
           (event->type == GDK_TOUCHPAD_SWIPE &&
            event->touchpad_swipe.phase == GDK_TOUCHPAD_GESTURE_PHASE_END) ||
           (event->type == GDK_TOUCHPAD_PINCH &&
            event->touchpad_pinch.phase == GDK_TOUCHPAD_GESTURE_PHASE_END))
    {
      if (_ctk_gesture_update_point (gesture, event, FALSE))
        {
          if (was_recognized &&
              _ctk_gesture_check_recognized (gesture, sequence))
            g_signal_emit (gesture, signals[UPDATE], 0, sequence);

          _ctk_gesture_remove_point (gesture, event);
        }
    }
  else if (event->type == GDK_MOTION_NOTIFY ||
           event->type == GDK_TOUCH_UPDATE ||
           (event->type == GDK_TOUCHPAD_SWIPE &&
            event->touchpad_swipe.phase == GDK_TOUCHPAD_GESTURE_PHASE_UPDATE) ||
           (event->type == GDK_TOUCHPAD_PINCH &&
            event->touchpad_pinch.phase == GDK_TOUCHPAD_GESTURE_PHASE_UPDATE))
    {
      if (event->type == GDK_MOTION_NOTIFY)
        {
          if ((event->motion.state & BUTTONS_MASK) == 0)
            return FALSE;

          if (event->motion.is_hint)
            cdk_event_request_motions (&event->motion);
        }

      if (_ctk_gesture_update_point (gesture, event, FALSE) &&
          _ctk_gesture_check_recognized (gesture, sequence))
        g_signal_emit (gesture, signals[UPDATE], 0, sequence);
    }
  else if (event->type == GDK_TOUCH_CANCEL)
    {
      if (!priv->touchpad)
        _ctk_gesture_cancel_sequence (gesture, sequence);
    }
  else if ((event->type == GDK_TOUCHPAD_SWIPE &&
            event->touchpad_swipe.phase == GDK_TOUCHPAD_GESTURE_PHASE_CANCEL) ||
           (event->type == GDK_TOUCHPAD_PINCH &&
            event->touchpad_pinch.phase == GDK_TOUCHPAD_GESTURE_PHASE_CANCEL))
    {
      if (priv->touchpad)
        _ctk_gesture_cancel_sequence (gesture, sequence);
    }
  else if (event->type == GDK_GRAB_BROKEN)
    {
      if (!event->grab_broken.grab_window ||
          !gesture_within_window (gesture, event->grab_broken.grab_window))
        _ctk_gesture_cancel_all (gesture);

      return FALSE;
    }
  else
    {
      /* Unhandled event */
      return FALSE;
    }

  if (ctk_gesture_get_sequence_state (gesture, sequence) != CTK_EVENT_SEQUENCE_CLAIMED)
    return FALSE;

  return priv->recognized;
}

static void
ctk_gesture_reset (CtkEventController *controller)
{
  _ctk_gesture_cancel_all (CTK_GESTURE (controller));
}

static void
ctk_gesture_class_init (CtkGestureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkEventControllerClass *controller_class = CTK_EVENT_CONTROLLER_CLASS (klass);

  object_class->get_property = ctk_gesture_get_property;
  object_class->set_property = ctk_gesture_set_property;
  object_class->finalize = ctk_gesture_finalize;

  controller_class->filter_event = ctk_gesture_filter_event;
  controller_class->handle_event = ctk_gesture_handle_event;
  controller_class->reset = ctk_gesture_reset;

  klass->check = ctk_gesture_check_impl;

  /**
   * CtkGesture:n-points:
   *
   * The number of touch points that trigger recognition on this gesture,
   * 
   *
   * Since: 3.14
   */
  g_object_class_install_property (object_class,
                                   PROP_N_POINTS,
                                   g_param_spec_uint ("n-points",
                                                      P_("Number of points"),
                                                      P_("Number of points needed "
                                                         "to trigger the gesture"),
                                                      1, G_MAXUINT, 1,
                                                      CTK_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT_ONLY));
  /**
   * CtkGesture:window:
   *
   * If non-%NULL, the gesture will only listen for events that happen on
   * this #CdkWindow, or a child of it.
   *
   * Since: 3.14
   */
  g_object_class_install_property (object_class,
                                   PROP_WINDOW,
                                   g_param_spec_object ("window",
                                                        P_("CdkWindow to receive events about"),
                                                        P_("CdkWindow to receive events about"),
                                                        GDK_TYPE_WINDOW,
                                                        CTK_PARAM_READWRITE));
  /**
   * CtkGesture::begin:
   * @gesture: the object which received the signal
   * @sequence: (nullable): the #CdkEventSequence that made the gesture to be recognized
   *
   * This signal is emitted when the gesture is recognized. This means the
   * number of touch sequences matches #CtkGesture:n-points, and the #CtkGesture::check
   * handler(s) returned #TRUE.
   *
   * Note: These conditions may also happen when an extra touch (eg. a third touch
   * on a 2-touches gesture) is lifted, in that situation @sequence won't pertain
   * to the current set of active touches, so don't rely on this being true.
   *
   * Since: 3.14
   */
  signals[BEGIN] =
    g_signal_new (I_("begin"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGestureClass, begin),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, GDK_TYPE_EVENT_SEQUENCE);
  /**
   * CtkGesture::end:
   * @gesture: the object which received the signal
   * @sequence: (nullable): the #CdkEventSequence that made gesture recognition to finish
   *
   * This signal is emitted when @gesture either stopped recognizing the event
   * sequences as something to be handled (the #CtkGesture::check handler returned
   * %FALSE), or the number of touch sequences became higher or lower than
   * #CtkGesture:n-points.
   *
   * Note: @sequence might not pertain to the group of sequences that were
   * previously triggering recognition on @gesture (ie. a just pressed touch
   * sequence that exceeds #CtkGesture:n-points). This situation may be detected
   * by checking through ctk_gesture_handles_sequence().
   *
   * Since: 3.14
   */
  signals[END] =
    g_signal_new (I_("end"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGestureClass, end),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, GDK_TYPE_EVENT_SEQUENCE);
  /**
   * CtkGesture::update:
   * @gesture: the object which received the signal
   * @sequence: (nullable): the #CdkEventSequence that was updated
   *
   * This signal is emitted whenever an event is handled while the gesture is
   * recognized. @sequence is guaranteed to pertain to the set of active touches.
   *
   * Since: 3.14
   */
  signals[UPDATE] =
    g_signal_new (I_("update"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGestureClass, update),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, GDK_TYPE_EVENT_SEQUENCE);
  /**
   * CtkGesture::cancel:
   * @gesture: the object which received the signal
   * @sequence: (nullable): the #CdkEventSequence that was cancelled
   *
   * This signal is emitted whenever a sequence is cancelled. This usually
   * happens on active touches when ctk_event_controller_reset() is called
   * on @gesture (manually, due to grabs...), or the individual @sequence
   * was claimed by parent widgets' controllers (see ctk_gesture_set_sequence_state()).
   *
   * @gesture must forget everything about @sequence as a reaction to this signal.
   *
   * Since: 3.14
   */
  signals[CANCEL] =
    g_signal_new (I_("cancel"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGestureClass, cancel),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, GDK_TYPE_EVENT_SEQUENCE);
  /**
   * CtkGesture::sequence-state-changed:
   * @gesture: the object which received the signal
   * @sequence: (nullable): the #CdkEventSequence that was cancelled
   * @state: the new sequence state
   *
   * This signal is emitted whenever a sequence state changes. See
   * ctk_gesture_set_sequence_state() to know more about the expectable
   * sequence lifetimes.
   *
   * Since: 3.14
   */
  signals[SEQUENCE_STATE_CHANGED] =
    g_signal_new (I_("sequence-state-changed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGestureClass, sequence_state_changed),
                  NULL, NULL,
                  _ctk_marshal_VOID__BOXED_ENUM,
                  G_TYPE_NONE, 2, GDK_TYPE_EVENT_SEQUENCE,
                  CTK_TYPE_EVENT_SEQUENCE_STATE);
  g_signal_set_va_marshaller (signals[SEQUENCE_STATE_CHANGED],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__BOXED_ENUMv);
}

static void
free_point_data (gpointer data)
{
  PointData *point = data;

  if (point->event)
    cdk_event_free (point->event);

  g_free (point);
}

static void
ctk_gesture_init (CtkGesture *gesture)
{
  CtkGesturePrivate *priv;

  priv = ctk_gesture_get_instance_private (gesture);
  priv->points = g_hash_table_new_full (NULL, NULL, NULL,
                                        (GDestroyNotify) free_point_data);
  ctk_event_controller_set_event_mask (CTK_EVENT_CONTROLLER (gesture),
                                       GDK_TOUCH_MASK |
                                       GDK_TOUCHPAD_GESTURE_MASK);

  priv->group_link = g_list_prepend (NULL, gesture);
}

/**
 * ctk_gesture_get_device:
 * @gesture: a #CtkGesture
 *
 * Returns the master #CdkDevice that is currently operating
 * on @gesture, or %NULL if the gesture is not being interacted.
 *
 * Returns: (nullable) (transfer none): a #CdkDevice, or %NULL
 *
 * Since: 3.14
 **/
CdkDevice *
ctk_gesture_get_device (CtkGesture *gesture)
{
  CtkGesturePrivate *priv;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), NULL);

  priv = ctk_gesture_get_instance_private (gesture);

  return priv->device;
}

/**
 * ctk_gesture_get_sequence_state:
 * @gesture: a #CtkGesture
 * @sequence: a #CdkEventSequence
 *
 * Returns the @sequence state, as seen by @gesture.
 *
 * Returns: The sequence state in @gesture
 *
 * Since: 3.14
 **/
CtkEventSequenceState
ctk_gesture_get_sequence_state (CtkGesture       *gesture,
                                CdkEventSequence *sequence)
{
  CtkGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture),
                        CTK_EVENT_SEQUENCE_NONE);

  priv = ctk_gesture_get_instance_private (gesture);
  data = g_hash_table_lookup (priv->points, sequence);

  if (!data)
    return CTK_EVENT_SEQUENCE_NONE;

  return data->state;
}

/**
 * ctk_gesture_set_sequence_state:
 * @gesture: a #CtkGesture
 * @sequence: a #CdkEventSequence
 * @state: the sequence state
 *
 * Sets the state of @sequence in @gesture. Sequences start
 * in state #CTK_EVENT_SEQUENCE_NONE, and whenever they change
 * state, they can never go back to that state. Likewise,
 * sequences in state #CTK_EVENT_SEQUENCE_DENIED cannot turn
 * back to a not denied state. With these rules, the lifetime
 * of an event sequence is constrained to the next four:
 *
 * * None
 * * None → Denied
 * * None → Claimed
 * * None → Claimed → Denied
 *
 * Note: Due to event handling ordering, it may be unsafe to
 * set the state on another gesture within a #CtkGesture::begin
 * signal handler, as the callback might be executed before
 * the other gesture knows about the sequence. A safe way to
 * perform this could be:
 *
 * |[
 * static void
 * first_gesture_begin_cb (CtkGesture       *first_gesture,
 *                         CdkEventSequence *sequence,
 *                         gpointer          user_data)
 * {
 *   ctk_gesture_set_sequence_state (first_gesture, sequence, CTK_EVENT_SEQUENCE_CLAIMED);
 *   ctk_gesture_set_sequence_state (second_gesture, sequence, CTK_EVENT_SEQUENCE_DENIED);
 * }
 *
 * static void
 * second_gesture_begin_cb (CtkGesture       *second_gesture,
 *                          CdkEventSequence *sequence,
 *                          gpointer          user_data)
 * {
 *   if (ctk_gesture_get_sequence_state (first_gesture, sequence) == CTK_EVENT_SEQUENCE_CLAIMED)
 *     ctk_gesture_set_sequence_state (second_gesture, sequence, CTK_EVENT_SEQUENCE_DENIED);
 * }
 * ]|
 *
 * If both gestures are in the same group, just set the state on
 * the gesture emitting the event, the sequence will be already
 * be initialized to the group's global state when the second
 * gesture processes the event.
 *
 * Returns: %TRUE if @sequence is handled by @gesture,
 *          and the state is changed successfully
 *
 * Since: 3.14
 **/
gboolean
ctk_gesture_set_sequence_state (CtkGesture            *gesture,
                                CdkEventSequence      *sequence,
                                CtkEventSequenceState  state)
{
  CtkGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), FALSE);
  g_return_val_if_fail (state >= CTK_EVENT_SEQUENCE_NONE &&
                        state <= CTK_EVENT_SEQUENCE_DENIED, FALSE);

  priv = ctk_gesture_get_instance_private (gesture);
  data = g_hash_table_lookup (priv->points, sequence);

  if (!data)
    return FALSE;

  if (data->state == state)
    return FALSE;

  /* denied sequences remain denied */
  if (data->state == CTK_EVENT_SEQUENCE_DENIED)
    return FALSE;

  /* Sequences can't go from claimed/denied to none */
  if (state == CTK_EVENT_SEQUENCE_NONE &&
      data->state != CTK_EVENT_SEQUENCE_NONE)
    return FALSE;

  data->state = state;
  g_signal_emit (gesture, signals[SEQUENCE_STATE_CHANGED], 0,
                 sequence, state);

  if (state == CTK_EVENT_SEQUENCE_DENIED)
    _ctk_gesture_check_recognized (gesture, sequence);

  return TRUE;
}

/**
 * ctk_gesture_set_state:
 * @gesture: a #CtkGesture
 * @state: the sequence state
 *
 * Sets the state of all sequences that @gesture is currently
 * interacting with. See ctk_gesture_set_sequence_state()
 * for more details on sequence states.
 *
 * Returns: %TRUE if the state of at least one sequence
 *     was changed successfully
 *
 * Since: 3.14
 */
gboolean
ctk_gesture_set_state (CtkGesture            *gesture,
                       CtkEventSequenceState  state)
{
  gboolean handled = FALSE;
  CtkGesturePrivate *priv;
  GList *sequences, *l;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), FALSE);
  g_return_val_if_fail (state >= CTK_EVENT_SEQUENCE_NONE &&
                        state <= CTK_EVENT_SEQUENCE_DENIED, FALSE);

  priv = ctk_gesture_get_instance_private (gesture);
  sequences = g_hash_table_get_keys (priv->points);

  for (l = sequences; l; l = l->next)
    handled |= ctk_gesture_set_sequence_state (gesture, l->data, state);

  g_list_free (sequences);

  return handled;
}

/**
 * ctk_gesture_get_sequences:
 * @gesture: a #CtkGesture
 *
 * Returns the list of #CdkEventSequences currently being interpreted
 * by @gesture.
 *
 * Returns: (transfer container) (element-type CdkEventSequence): A list
 *          of #CdkEventSequences, the list elements are owned by CTK+
 *          and must not be freed or modified, the list itself must be deleted
 *          through g_list_free()
 *
 * Since: 3.14
 **/
GList *
ctk_gesture_get_sequences (CtkGesture *gesture)
{
  CdkEventSequence *sequence;
  CtkGesturePrivate *priv;
  GList *sequences = NULL;
  GHashTableIter iter;
  PointData *data;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), NULL);

  priv = ctk_gesture_get_instance_private (gesture);
  g_hash_table_iter_init (&iter, priv->points);

  while (g_hash_table_iter_next (&iter, (gpointer *) &sequence, (gpointer *) &data))
    {
      if (data->state == CTK_EVENT_SEQUENCE_DENIED)
        continue;
      if (data->event->type == GDK_TOUCH_END ||
          data->event->type == GDK_BUTTON_RELEASE)
        continue;

      sequences = g_list_prepend (sequences, sequence);
    }

  return sequences;
}

/**
 * ctk_gesture_get_last_updated_sequence:
 * @gesture: a #CtkGesture
 *
 * Returns the #CdkEventSequence that was last updated on @gesture.
 *
 * Returns: (transfer none) (nullable): The last updated sequence
 *
 * Since: 3.14
 **/
CdkEventSequence *
ctk_gesture_get_last_updated_sequence (CtkGesture *gesture)
{
  CtkGesturePrivate *priv;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), NULL);

  priv = ctk_gesture_get_instance_private (gesture);

  return priv->last_sequence;
}

/**
 * ctk_gesture_get_last_event:
 * @gesture: a #CtkGesture
 * @sequence: (nullable): a #CdkEventSequence
 *
 * Returns the last event that was processed for @sequence.
 *
 * Note that the returned pointer is only valid as long as the @sequence
 * is still interpreted by the @gesture. If in doubt, you should make
 * a copy of the event.
 *
 * Returns: (transfer none) (nullable): The last event from @sequence
 **/
const CdkEvent *
ctk_gesture_get_last_event (CtkGesture       *gesture,
                            CdkEventSequence *sequence)
{
  CtkGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), NULL);

  priv = ctk_gesture_get_instance_private (gesture);
  data = g_hash_table_lookup (priv->points, sequence);

  if (!data)
    return NULL;

  return data->event;
}

/**
 * ctk_gesture_get_point:
 * @gesture: a #CtkGesture
 * @sequence: (allow-none): a #CdkEventSequence, or %NULL for pointer events
 * @x: (out) (allow-none): return location for X axis of the sequence coordinates
 * @y: (out) (allow-none): return location for Y axis of the sequence coordinates
 *
 * If @sequence is currently being interpreted by @gesture, this
 * function returns %TRUE and fills in @x and @y with the last coordinates
 * stored for that event sequence. The coordinates are always relative to the
 * widget allocation.
 *
 * Returns: %TRUE if @sequence is currently interpreted
 *
 * Since: 3.14
 **/
gboolean
ctk_gesture_get_point (CtkGesture       *gesture,
                       CdkEventSequence *sequence,
                       gdouble          *x,
                       gdouble          *y)
{
  CtkGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), FALSE);

  priv = ctk_gesture_get_instance_private (gesture);

  if (!g_hash_table_lookup_extended (priv->points, sequence,
                                     NULL, (gpointer *) &data))
    return FALSE;

  if (x)
    *x = data->widget_x;
  if (y)
    *y = data->widget_y;

  return TRUE;
}

gboolean
_ctk_gesture_get_last_update_time (CtkGesture       *gesture,
                                   CdkEventSequence *sequence,
                                   guint32          *evtime)
{
  CtkGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), FALSE);

  priv = ctk_gesture_get_instance_private (gesture);

  if (!g_hash_table_lookup_extended (priv->points, sequence,
                                     NULL, (gpointer *) &data))
    return FALSE;

  if (evtime)
    *evtime = cdk_event_get_time (data->event);

  return TRUE;
};

/**
 * ctk_gesture_get_bounding_box:
 * @gesture: a #CtkGesture
 * @rect: (out): bounding box containing all active touches.
 *
 * If there are touch sequences being currently handled by @gesture,
 * this function returns %TRUE and fills in @rect with the bounding
 * box containing all active touches. Otherwise, %FALSE will be
 * returned.
 *
 * Note: This function will yield unexpected results on touchpad
 * gestures. Since there is no correlation between physical and
 * pixel distances, these will look as if constrained in an
 * infinitely small area, @rect width and height will thus be 0
 * regardless of the number of touchpoints.
 *
 * Returns: %TRUE if there are active touches, %FALSE otherwise
 *
 * Since: 3.14
 **/
gboolean
ctk_gesture_get_bounding_box (CtkGesture   *gesture,
                              CdkRectangle *rect)
{
  CtkGesturePrivate *priv;
  gdouble x1, y1, x2, y2;
  GHashTableIter iter;
  guint n_points = 0;
  PointData *data;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), FALSE);
  g_return_val_if_fail (rect != NULL, FALSE);

  priv = ctk_gesture_get_instance_private (gesture);
  x1 = y1 = G_MAXDOUBLE;
  x2 = y2 = -G_MAXDOUBLE;

  g_hash_table_iter_init (&iter, priv->points);

  while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &data))
    {
      gdouble x, y;

      if (data->state == CTK_EVENT_SEQUENCE_DENIED)
        continue;
      if (data->event->type == GDK_TOUCH_END ||
          data->event->type == GDK_BUTTON_RELEASE)
        continue;

      cdk_event_get_coords (data->event, &x, &y);
      n_points++;
      x1 = MIN (x1, x);
      y1 = MIN (y1, y);
      x2 = MAX (x2, x);
      y2 = MAX (y2, y);
    }

  if (n_points == 0)
    return FALSE;

  rect->x = x1;
  rect->y = y1;
  rect->width = x2 - x1;
  rect->height = y2 - y1;

  return TRUE;
}


/**
 * ctk_gesture_get_bounding_box_center:
 * @gesture: a #CtkGesture
 * @x: (out): X coordinate for the bounding box center
 * @y: (out): Y coordinate for the bounding box center
 *
 * If there are touch sequences being currently handled by @gesture,
 * this function returns %TRUE and fills in @x and @y with the center
 * of the bounding box containing all active touches. Otherwise, %FALSE
 * will be returned.
 *
 * Returns: %FALSE if no active touches are present, %TRUE otherwise
 *
 * Since: 3.14
 **/
gboolean
ctk_gesture_get_bounding_box_center (CtkGesture *gesture,
                                     gdouble    *x,
                                     gdouble    *y)
{
  const CdkEvent *last_event;
  CdkRectangle rect;
  CdkEventSequence *sequence;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), FALSE);
  g_return_val_if_fail (x != NULL && y != NULL, FALSE);

  sequence = ctk_gesture_get_last_updated_sequence (gesture);
  last_event = ctk_gesture_get_last_event (gesture, sequence);

  if (EVENT_IS_TOUCHPAD_GESTURE (last_event))
    return ctk_gesture_get_point (gesture, sequence, x, y);
  else if (!ctk_gesture_get_bounding_box (gesture, &rect))
    return FALSE;

  *x = rect.x + rect.width / 2;
  *y = rect.y + rect.height / 2;
  return TRUE;
}

/**
 * ctk_gesture_is_active:
 * @gesture: a #CtkGesture
 *
 * Returns %TRUE if the gesture is currently active.
 * A gesture is active meanwhile there are touch sequences
 * interacting with it.
 *
 * Returns: %TRUE if gesture is active
 *
 * Since: 3.14
 **/
gboolean
ctk_gesture_is_active (CtkGesture *gesture)
{
  g_return_val_if_fail (CTK_IS_GESTURE (gesture), FALSE);

  return _ctk_gesture_get_n_physical_points (gesture, TRUE) != 0;
}

/**
 * ctk_gesture_is_recognized:
 * @gesture: a #CtkGesture
 *
 * Returns %TRUE if the gesture is currently recognized.
 * A gesture is recognized if there are as many interacting
 * touch sequences as required by @gesture, and #CtkGesture::check
 * returned %TRUE for the sequences being currently interpreted.
 *
 * Returns: %TRUE if gesture is recognized
 *
 * Since: 3.14
 **/
gboolean
ctk_gesture_is_recognized (CtkGesture *gesture)
{
  CtkGesturePrivate *priv;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), FALSE);

  priv = ctk_gesture_get_instance_private (gesture);

  return priv->recognized;
}

gboolean
_ctk_gesture_check (CtkGesture *gesture)
{
  CtkGesturePrivate *priv;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), FALSE);

  priv = ctk_gesture_get_instance_private (gesture);

  return _ctk_gesture_check_recognized (gesture, priv->last_sequence);
}

/**
 * ctk_gesture_handles_sequence:
 * @gesture: a #CtkGesture
 * @sequence: (nullable): a #CdkEventSequence or %NULL
 *
 * Returns %TRUE if @gesture is currently handling events corresponding to
 * @sequence.
 *
 * Returns: %TRUE if @gesture is handling @sequence, %FALSE otherwise
 *
 * Since: 3.14
 **/
gboolean
ctk_gesture_handles_sequence (CtkGesture       *gesture,
                              CdkEventSequence *sequence)
{
  CtkGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), FALSE);

  priv = ctk_gesture_get_instance_private (gesture);
  data = g_hash_table_lookup (priv->points, sequence);

  if (!data)
    return FALSE;

  if (data->state == CTK_EVENT_SEQUENCE_DENIED)
    return FALSE;

  return TRUE;
}

gboolean
_ctk_gesture_cancel_sequence (CtkGesture       *gesture,
                              CdkEventSequence *sequence)
{
  CtkGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), FALSE);

  priv = ctk_gesture_get_instance_private (gesture);
  data = g_hash_table_lookup (priv->points, sequence);

  if (!data)
    return FALSE;

  g_signal_emit (gesture, signals[CANCEL], 0, sequence);
  _ctk_gesture_remove_point (gesture, data->event);
  _ctk_gesture_check_recognized (gesture, sequence);

  return TRUE;
}

/**
 * ctk_gesture_get_window:
 * @gesture: a #CtkGesture
 *
 * Returns the user-defined window that receives the events
 * handled by @gesture. See ctk_gesture_set_window() for more
 * information.
 *
 * Returns: (nullable) (transfer none): the user defined window, or %NULL if none
 *
 * Since: 3.14
 **/
CdkWindow *
ctk_gesture_get_window (CtkGesture *gesture)
{
  CtkGesturePrivate *priv;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), NULL);

  priv = ctk_gesture_get_instance_private (gesture);

  return priv->user_window;
}

/**
 * ctk_gesture_set_window:
 * @gesture: a #CtkGesture
 * @window: (allow-none): a #CdkWindow, or %NULL
 *
 * Sets a specific window to receive events about, so @gesture
 * will effectively handle only events targeting @window, or
 * a child of it. @window must pertain to ctk_event_controller_get_widget().
 *
 * Since: 3.14
 **/
void
ctk_gesture_set_window (CtkGesture *gesture,
                        CdkWindow  *window)
{
  CtkGesturePrivate *priv;

  g_return_if_fail (CTK_IS_GESTURE (gesture));
  g_return_if_fail (!window || GDK_IS_WINDOW (window));

  priv = ctk_gesture_get_instance_private (gesture);

  if (window)
    {
      CtkWidget *window_widget;

      cdk_window_get_user_data (window, (gpointer*) &window_widget);
      g_return_if_fail (window_widget ==
                        ctk_event_controller_get_widget (CTK_EVENT_CONTROLLER (gesture)));
    }

  if (priv->user_window == window)
    return;

  priv->user_window = window;
  g_object_notify (G_OBJECT (gesture), "window");
}

GList *
_ctk_gesture_get_group_link (CtkGesture *gesture)
{
  CtkGesturePrivate *priv;

  priv = ctk_gesture_get_instance_private (gesture);

  return priv->group_link;
}

/**
 * ctk_gesture_group:
 * @gesture: a #CtkGesture
 * @group_gesture: #CtkGesture to group @gesture with
 *
 * Adds @gesture to the same group than @group_gesture. Gestures
 * are by default isolated in their own groups.
 *
 * When gestures are grouped, the state of #CdkEventSequences
 * is kept in sync for all of those, so calling ctk_gesture_set_sequence_state(),
 * on one will transfer the same value to the others.
 *
 * Groups also perform an "implicit grabbing" of sequences, if a
 * #CdkEventSequence state is set to #CTK_EVENT_SEQUENCE_CLAIMED on one group,
 * every other gesture group attached to the same #CtkWidget will switch the
 * state for that sequence to #CTK_EVENT_SEQUENCE_DENIED.
 *
 * Since: 3.14
 **/
void
ctk_gesture_group (CtkGesture *gesture,
                   CtkGesture *group_gesture)
{
  GList *link, *group_link, *next;

  g_return_if_fail (CTK_IS_GESTURE (gesture));
  g_return_if_fail (CTK_IS_GESTURE (group_gesture));
  g_return_if_fail (ctk_event_controller_get_widget (CTK_EVENT_CONTROLLER (group_gesture)) ==
                    ctk_event_controller_get_widget (CTK_EVENT_CONTROLLER (gesture)));

  link = _ctk_gesture_get_group_link (gesture);

  if (link->prev || link->next)
    {
      if (ctk_gesture_is_grouped_with (gesture, group_gesture))
        return;
      ctk_gesture_ungroup (gesture);
    }

  group_link = _ctk_gesture_get_group_link (group_gesture);
  next = group_link->next;

  /* Rewire link so it's inserted after the group_gesture elem */
  link->prev = group_link;
  link->next = next;
  group_link->next = link;
  if (next)
    next->prev = link;
}

/**
 * ctk_gesture_ungroup:
 * @gesture: a #CtkGesture
 *
 * Separates @gesture into an isolated group.
 *
 * Since: 3.14
 **/
void
ctk_gesture_ungroup (CtkGesture *gesture)
{
  GList *link, *prev, *next;

  g_return_if_fail (CTK_IS_GESTURE (gesture));

  link = _ctk_gesture_get_group_link (gesture);
  prev = link->prev;
  next = link->next;

  /* Detach link from the group chain */
  if (prev)
    prev->next = next;
  if (next)
    next->prev = prev;

  link->next = link->prev = NULL;
}

/**
 * ctk_gesture_get_group:
 * @gesture: a #CtkGesture
 *
 * Returns all gestures in the group of @gesture
 *
 * Returns: (element-type CtkGesture) (transfer container): The list
 *   of #CtkGestures, free with g_list_free()
 *
 * Since: 3.14
 **/
GList *
ctk_gesture_get_group (CtkGesture *gesture)
{
  GList *link;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), NULL);

  link = _ctk_gesture_get_group_link (gesture);

  return g_list_copy (g_list_first (link));
}

/**
 * ctk_gesture_is_grouped_with:
 * @gesture: a #CtkGesture
 * @other: another #CtkGesture
 *
 * Returns %TRUE if both gestures pertain to the same group.
 *
 * Returns: whether the gestures are grouped
 *
 * Since: 3.14
 **/
gboolean
ctk_gesture_is_grouped_with (CtkGesture *gesture,
                             CtkGesture *other)
{
  GList *link;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), FALSE);
  g_return_val_if_fail (CTK_IS_GESTURE (other), FALSE);

  link = _ctk_gesture_get_group_link (gesture);
  link = g_list_first (link);

  return g_list_find (link, other) != NULL;
}

gboolean
_ctk_gesture_handled_sequence_press (CtkGesture       *gesture,
                                     CdkEventSequence *sequence)
{
  CtkGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), FALSE);

  priv = ctk_gesture_get_instance_private (gesture);
  data = g_hash_table_lookup (priv->points, sequence);

  if (!data)
    return FALSE;

  return data->press_handled;
}

gboolean
_ctk_gesture_get_pointer_emulating_sequence (CtkGesture        *gesture,
                                             CdkEventSequence **sequence)
{
  CtkGesturePrivate *priv;
  CdkEventSequence *seq;
  GHashTableIter iter;
  PointData *data;

  g_return_val_if_fail (CTK_IS_GESTURE (gesture), FALSE);

  priv = ctk_gesture_get_instance_private (gesture);
  g_hash_table_iter_init (&iter, priv->points);

  while (g_hash_table_iter_next (&iter, (gpointer*) &seq, (gpointer*) &data))
    {
      switch (data->event->type)
        {
        case GDK_TOUCH_BEGIN:
        case GDK_TOUCH_UPDATE:
        case GDK_TOUCH_END:
          if (!data->event->touch.emulating_pointer)
            continue;
          /* Fall through */
        case GDK_BUTTON_PRESS:
        case GDK_BUTTON_RELEASE:
        case GDK_MOTION_NOTIFY:
          *sequence = seq;
          return TRUE;
        default:
          break;
        }
    }

  return FALSE;
}
