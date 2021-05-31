/* GTK - The GIMP Toolkit
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
 * SECTION:ctkgesturepan
 * @Short_description: Pan gesture
 * @Title: CtkGesturePan
 *
 * #CtkGesturePan is a #CtkGesture implementation able to recognize
 * pan gestures, those are drags that are locked to happen along one
 * axis. The axis that a #CtkGesturePan handles is defined at
 * construct time, and can be changed through
 * ctk_gesture_pan_set_orientation().
 *
 * When the gesture starts to be recognized, #CtkGesturePan will
 * attempt to determine as early as possible whether the sequence
 * is moving in the expected direction, and denying the sequence if
 * this does not happen.
 *
 * Once a panning gesture along the expected axis is recognized,
 * the #CtkGesturePan::pan signal will be emitted as input events
 * are received, containing the offset in the given axis.
 */

#include "config.h"
#include "ctkgesturepan.h"
#include "ctkgesturepanprivate.h"
#include "ctktypebuiltins.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"

typedef struct _CtkGesturePanPrivate CtkGesturePanPrivate;

struct _CtkGesturePanPrivate
{
  guint orientation : 2;
  guint panning     : 1;
};

enum {
  PROP_ORIENTATION = 1
};

enum {
  PAN,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (CtkGesturePan, ctk_gesture_pan, CTK_TYPE_GESTURE_DRAG)

static void
ctk_gesture_pan_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  CtkGesturePanPrivate *priv;

  priv = ctk_gesture_pan_get_instance_private (CTK_GESTURE_PAN (object));

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_gesture_pan_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_ORIENTATION:
      ctk_gesture_pan_set_orientation (CTK_GESTURE_PAN (object),
                                       g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
direction_from_offset (gdouble          offset_x,
                       gdouble          offset_y,
                       CtkOrientation   orientation,
                       CtkPanDirection *direction)
{
  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (offset_x > 0)
        *direction = CTK_PAN_DIRECTION_RIGHT;
      else
        *direction = CTK_PAN_DIRECTION_LEFT;
    }
  else if (orientation == CTK_ORIENTATION_VERTICAL)
    {
      if (offset_y > 0)
        *direction = CTK_PAN_DIRECTION_DOWN;
      else
        *direction = CTK_PAN_DIRECTION_UP;
    }
  else
    g_assert_not_reached ();
}

static gboolean
guess_direction (CtkGesturePan   *gesture,
                 gdouble          offset_x,
                 gdouble          offset_y,
                 CtkPanDirection *direction)
{
  gdouble abs_x, abs_y;

  abs_x = ABS (offset_x);
  abs_y = ABS (offset_y);

#define FACTOR 2
  if (abs_x > abs_y * FACTOR)
    direction_from_offset (offset_x, offset_y,
                           CTK_ORIENTATION_HORIZONTAL, direction);
  else if (abs_y > abs_x * FACTOR)
    direction_from_offset (offset_x, offset_y,
                           CTK_ORIENTATION_VERTICAL, direction);
  else
    return FALSE;

  return TRUE;
#undef FACTOR
}

static gboolean
check_orientation_matches (CtkGesturePan   *gesture,
                           CtkPanDirection  direction)
{
  CtkGesturePanPrivate *priv = ctk_gesture_pan_get_instance_private (gesture);

  return (((direction == CTK_PAN_DIRECTION_LEFT ||
            direction == CTK_PAN_DIRECTION_RIGHT) &&
           priv->orientation == CTK_ORIENTATION_HORIZONTAL) ||
          ((direction == CTK_PAN_DIRECTION_UP ||
            direction == CTK_PAN_DIRECTION_DOWN) &&
           priv->orientation == CTK_ORIENTATION_VERTICAL));
}

static void
ctk_gesture_pan_drag_update (CtkGestureDrag *gesture,
                             gdouble         offset_x,
                             gdouble         offset_y)
{
  CtkGesturePanPrivate *priv;
  CtkPanDirection direction;
  CtkGesturePan *pan;
  gdouble offset;

  pan = CTK_GESTURE_PAN (gesture);
  priv = ctk_gesture_pan_get_instance_private (pan);

  if (!priv->panning)
    {
      if (!guess_direction (pan, offset_x, offset_y, &direction))
        return;

      if (!check_orientation_matches (pan, direction))
        {
          ctk_gesture_set_state (CTK_GESTURE (gesture),
                                 CTK_EVENT_SEQUENCE_DENIED);
          return;
        }

      priv->panning = TRUE;
    }
  else
    direction_from_offset (offset_x, offset_y, priv->orientation, &direction);

  offset = (priv->orientation == CTK_ORIENTATION_VERTICAL) ?
    ABS (offset_y) : ABS (offset_x);
  g_signal_emit (gesture, signals[PAN], 0, direction, offset);
}

static void
ctk_gesture_pan_drag_end (CtkGestureDrag *gesture,
                          gdouble         offset_x,
                          gdouble         offset_y)
{
  CtkGesturePanPrivate *priv;

  priv = ctk_gesture_pan_get_instance_private (CTK_GESTURE_PAN (gesture));
  priv->panning = FALSE;
}

static void
ctk_gesture_pan_class_init (CtkGesturePanClass *klass)
{
  CtkGestureDragClass *drag_gesture_class = CTK_GESTURE_DRAG_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ctk_gesture_pan_get_property;
  object_class->set_property = ctk_gesture_pan_set_property;

  drag_gesture_class->drag_update = ctk_gesture_pan_drag_update;
  drag_gesture_class->drag_end = ctk_gesture_pan_drag_end;

  /**
   * CtkGesturePan:orientation:
   *
   * The expected orientation of pan gestures.
   *
   * Since: 3.14
   */
  g_object_class_install_property (object_class,
                                   PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation",
                                                      P_("Orientation"),
                                                      P_("Allowed orientations"),
                                                      CTK_TYPE_ORIENTATION,
                                                      CTK_ORIENTATION_HORIZONTAL,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkGesturePan::pan:
   * @gesture: The object which received the signal
   * @direction: current direction of the pan gesture
   * @offset: Offset along the gesture orientation
   *
   * This signal is emitted once a panning gesture along the
   * expected axis is detected.
   *
   * Since: 3.14
   */
  signals[PAN] =
    g_signal_new (I_("pan"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGesturePanClass, pan),
                  NULL, NULL,
                  _ctk_marshal_VOID__ENUM_DOUBLE,
                  G_TYPE_NONE, 2, CTK_TYPE_PAN_DIRECTION,
                  G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[PAN],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__ENUM_DOUBLEv);
}

static void
ctk_gesture_pan_init (CtkGesturePan *gesture)
{
  CtkGesturePanPrivate *priv;

  priv = ctk_gesture_pan_get_instance_private (gesture);
  priv->orientation = CTK_ORIENTATION_HORIZONTAL;
}

/**
 * ctk_gesture_pan_new:
 * @widget: a #CtkWidget
 * @orientation: expected orientation
 *
 * Returns a newly created #CtkGesture that recognizes pan gestures.
 *
 * Returns: a newly created #CtkGesturePan
 *
 * Since: 3.14
 **/
CtkGesture *
ctk_gesture_pan_new (CtkWidget      *widget,
                     CtkOrientation  orientation)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return g_object_new (CTK_TYPE_GESTURE_PAN,
                       "widget", widget,
                       "orientation", orientation,
                       NULL);
}

/**
 * ctk_gesture_pan_get_orientation:
 * @gesture: A #CtkGesturePan
 *
 * Returns the orientation of the pan gestures that this @gesture expects.
 *
 * Returns: the expected orientation for pan gestures
 *
 * Since: 3.14
 */
CtkOrientation
ctk_gesture_pan_get_orientation (CtkGesturePan *gesture)
{
  CtkGesturePanPrivate *priv;

  g_return_val_if_fail (CTK_IS_GESTURE_PAN (gesture), 0);

  priv = ctk_gesture_pan_get_instance_private (gesture);

  return priv->orientation;
}

/**
 * ctk_gesture_pan_set_orientation:
 * @gesture: A #CtkGesturePan
 * @orientation: expected orientation
 *
 * Sets the orientation to be expected on pan gestures.
 *
 * Since: 3.14
 */
void
ctk_gesture_pan_set_orientation (CtkGesturePan  *gesture,
                                 CtkOrientation  orientation)
{
  CtkGesturePanPrivate *priv;

  g_return_if_fail (CTK_IS_GESTURE_PAN (gesture));
  g_return_if_fail (orientation == CTK_ORIENTATION_HORIZONTAL ||
                    orientation == CTK_ORIENTATION_VERTICAL);

  priv = ctk_gesture_pan_get_instance_private (gesture);

  if (priv->orientation == orientation)
    return;

  priv->orientation = orientation;
  g_object_notify (G_OBJECT (gesture), "orientation");
}
