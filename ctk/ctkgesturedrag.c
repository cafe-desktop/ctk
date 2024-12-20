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

/**
 * SECTION:ctkgesturedrag
 * @Short_description: Drag gesture
 * @Title: CtkGestureDrag
 * @See_also: #CtkGestureSwipe
 *
 * #CtkGestureDrag is a #CtkGesture implementation that recognizes drag
 * operations. The drag operation itself can be tracked throught the
 * #CtkGestureDrag::drag-begin, #CtkGestureDrag::drag-update and
 * #CtkGestureDrag::drag-end signals, or the relevant coordinates be
 * extracted through ctk_gesture_drag_get_offset() and
 * ctk_gesture_drag_get_start_point().
 */
#include "config.h"
#include "ctkgesturedrag.h"
#include "ctkgesturedragprivate.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"

typedef struct _CtkGestureDragPrivate CtkGestureDragPrivate;
typedef struct _EventData EventData;

struct _CtkGestureDragPrivate
{
  gdouble start_x;
  gdouble start_y;
  gdouble last_x;
  gdouble last_y;
};

enum {
  DRAG_BEGIN,
  DRAG_UPDATE,
  DRAG_END,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (CtkGestureDrag, ctk_gesture_drag, CTK_TYPE_GESTURE_SINGLE)

static gboolean
ctk_gesture_drag_filter_event (CtkEventController *controller,
                               const CdkEvent     *event)
{
  /* Let touchpad swipe events go through, only if they match n-points  */
  if (event->type == CDK_TOUCHPAD_SWIPE)
    {
      guint n_points;

      g_object_get (G_OBJECT (controller), "n-points", &n_points, NULL);

      if (event->touchpad_swipe.n_fingers == n_points)
        return FALSE;
      else
        return TRUE;
    }

  return CTK_EVENT_CONTROLLER_CLASS (ctk_gesture_drag_parent_class)->filter_event (controller, event);
}

static void
ctk_gesture_drag_begin (CtkGesture       *gesture,
                        CdkEventSequence *sequence G_GNUC_UNUSED)
{
  CtkGestureDragPrivate *priv;
  CdkEventSequence *current;

  current = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));

  priv = ctk_gesture_drag_get_instance_private (CTK_GESTURE_DRAG (gesture));
  ctk_gesture_get_point (gesture, current, &priv->start_x, &priv->start_y);
  priv->last_x = priv->start_x;
  priv->last_y = priv->start_y;

  g_signal_emit (gesture, signals[DRAG_BEGIN], 0, priv->start_x, priv->start_y);
}

static void
ctk_gesture_drag_update (CtkGesture       *gesture,
                         CdkEventSequence *sequence)
{
  CtkGestureDragPrivate *priv;
  gdouble x, y;

  priv = ctk_gesture_drag_get_instance_private (CTK_GESTURE_DRAG (gesture));
  ctk_gesture_get_point (gesture, sequence, &priv->last_x, &priv->last_y);
  x = priv->last_x - priv->start_x;
  y = priv->last_y - priv->start_y;

  g_signal_emit (gesture, signals[DRAG_UPDATE], 0, x, y);
}

static void
ctk_gesture_drag_end (CtkGesture       *gesture,
                      CdkEventSequence *sequence G_GNUC_UNUSED)
{
  CtkGestureDragPrivate *priv;
  CdkEventSequence *current;
  gdouble x, y;

  current = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));

  priv = ctk_gesture_drag_get_instance_private (CTK_GESTURE_DRAG (gesture));
  ctk_gesture_get_point (gesture, current, &priv->last_x, &priv->last_y);
  x = priv->last_x - priv->start_x;
  y = priv->last_y - priv->start_y;

  g_signal_emit (gesture, signals[DRAG_END], 0, x, y);
}

static void
ctk_gesture_drag_class_init (CtkGestureDragClass *klass)
{
  CtkGestureClass *gesture_class = CTK_GESTURE_CLASS (klass);
  CtkEventControllerClass *event_controller_class = CTK_EVENT_CONTROLLER_CLASS (klass);

  event_controller_class->filter_event = ctk_gesture_drag_filter_event;

  gesture_class->begin = ctk_gesture_drag_begin;
  gesture_class->update = ctk_gesture_drag_update;
  gesture_class->end = ctk_gesture_drag_end;

  /**
   * CtkGestureDrag::drag-begin:
   * @gesture: the object which received the signal
   * @start_x: X coordinate, relative to the widget allocation
   * @start_y: Y coordinate, relative to the widget allocation
   *
   * This signal is emitted whenever dragging starts.
   *
   * Since: 3.14
   */
  signals[DRAG_BEGIN] =
    g_signal_new (I_("drag-begin"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGestureDragClass, drag_begin),
                  NULL, NULL,
                  _ctk_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[DRAG_BEGIN],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__DOUBLE_DOUBLEv);
  /**
   * CtkGestureDrag::drag-update:
   * @gesture: the object which received the signal
   * @offset_x: X offset, relative to the start point
   * @offset_y: Y offset, relative to the start point
   *
   * This signal is emitted whenever the dragging point moves.
   *
   * Since: 3.14
   */
  signals[DRAG_UPDATE] =
    g_signal_new (I_("drag-update"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGestureDragClass, drag_update),
                  NULL, NULL,
                  _ctk_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[DRAG_UPDATE],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__DOUBLE_DOUBLEv);
  /**
   * CtkGestureDrag::drag-end:
   * @gesture: the object which received the signal
   * @offset_x: X offset, relative to the start point
   * @offset_y: Y offset, relative to the start point
   *
   * This signal is emitted whenever the dragging is finished.
   *
   * Since: 3.14
   */
  signals[DRAG_END] =
    g_signal_new (I_("drag-end"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGestureDragClass, drag_end),
                  NULL, NULL,
                  _ctk_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[DRAG_END],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__DOUBLE_DOUBLEv);
}

static void
ctk_gesture_drag_init (CtkGestureDrag *gesture G_GNUC_UNUSED)
{
}

/**
 * ctk_gesture_drag_new:
 * @widget: a #CtkWidget
 *
 * Returns a newly created #CtkGesture that recognizes drags.
 *
 * Returns: a newly created #CtkGestureDrag
 *
 * Since: 3.14
 **/
CtkGesture *
ctk_gesture_drag_new (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return g_object_new (CTK_TYPE_GESTURE_DRAG,
                       "widget", widget,
                       NULL);
}

/**
 * ctk_gesture_drag_get_start_point:
 * @gesture: a #CtkGesture
 * @x: (out) (nullable): X coordinate for the drag start point
 * @y: (out) (nullable): Y coordinate for the drag start point
 *
 * If the @gesture is active, this function returns %TRUE
 * and fills in @x and @y with the drag start coordinates,
 * in window-relative coordinates.
 *
 * Returns: %TRUE if the gesture is active
 *
 * Since: 3.14
 **/
gboolean
ctk_gesture_drag_get_start_point (CtkGestureDrag *gesture,
                                  gdouble        *x,
                                  gdouble        *y)
{
  CtkGestureDragPrivate *priv;
  CdkEventSequence *sequence;

  g_return_val_if_fail (CTK_IS_GESTURE_DRAG (gesture), FALSE);

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));

  if (!ctk_gesture_handles_sequence (CTK_GESTURE (gesture), sequence))
    return FALSE;

  priv = ctk_gesture_drag_get_instance_private (gesture);

  if (x)
    *x = priv->start_x;

  if (y)
    *y = priv->start_y;

  return TRUE;
}

/**
 * ctk_gesture_drag_get_offset:
 * @gesture: a #CtkGesture
 * @x: (out) (nullable): X offset for the current point
 * @y: (out) (nullable): Y offset for the current point
 *
 * If the @gesture is active, this function returns %TRUE and
 * fills in @x and @y with the coordinates of the current point,
 * as an offset to the starting drag point.
 *
 * Returns: %TRUE if the gesture is active
 *
 * Since: 3.14
 **/
gboolean
ctk_gesture_drag_get_offset (CtkGestureDrag *gesture,
                             gdouble        *x,
                             gdouble        *y)
{
  CtkGestureDragPrivate *priv;
  CdkEventSequence *sequence;

  g_return_val_if_fail (CTK_IS_GESTURE_DRAG (gesture), FALSE);

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));

  if (!ctk_gesture_handles_sequence (CTK_GESTURE (gesture), sequence))
    return FALSE;

  priv = ctk_gesture_drag_get_instance_private (gesture);

  if (x)
    *x = priv->last_x - priv->start_x;

  if (y)
    *y = priv->last_y - priv->start_y;

  return TRUE;
}
