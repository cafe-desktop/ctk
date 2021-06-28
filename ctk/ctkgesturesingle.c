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
 * SECTION:ctkgesturesingle
 * @Short_description: Base class for mouse/single-touch gestures
 * @Title: CtkGestureSingle
 *
 * #CtkGestureSingle is a subclass of #CtkGesture, optimized (although
 * not restricted) for dealing with mouse and single-touch gestures. Under
 * interaction, these gestures stick to the first interacting sequence, which
 * is accessible through ctk_gesture_single_get_current_sequence() while the
 * gesture is being interacted with.
 *
 * By default gestures react to both %GDK_BUTTON_PRIMARY and touch
 * events, ctk_gesture_single_set_touch_only() can be used to change the
 * touch behavior. Callers may also specify a different mouse button number
 * to interact with through ctk_gesture_single_set_button(), or react to any
 * mouse button by setting 0. While the gesture is active, the button being
 * currently pressed can be known through ctk_gesture_single_get_current_button().
 */

#include "config.h"
#include "ctkgesturesingle.h"
#include "ctkgesturesingleprivate.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctkdebug.h"

typedef struct _CtkGestureSinglePrivate CtkGestureSinglePrivate;

struct _CtkGestureSinglePrivate
{
  CdkEventSequence *current_sequence;
  guint button;
  guint current_button;
  guint touch_only : 1;
  guint exclusive  : 1;
};

enum {
  PROP_TOUCH_ONLY = 1,
  PROP_EXCLUSIVE,
  PROP_BUTTON,
  LAST_PROP
};

static GParamSpec *properties[LAST_PROP] = { NULL, };

G_DEFINE_TYPE_WITH_PRIVATE (CtkGestureSingle, ctk_gesture_single, CTK_TYPE_GESTURE)

static void
ctk_gesture_single_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  CtkGestureSinglePrivate *priv;

  priv = ctk_gesture_single_get_instance_private (CTK_GESTURE_SINGLE (object));

  switch (prop_id)
    {
    case PROP_TOUCH_ONLY:
      g_value_set_boolean (value, priv->touch_only);
      break;
    case PROP_EXCLUSIVE:
      g_value_set_boolean (value, priv->exclusive);
      break;
    case PROP_BUTTON:
      g_value_set_uint (value, priv->button);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_gesture_single_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_TOUCH_ONLY:
      ctk_gesture_single_set_touch_only (CTK_GESTURE_SINGLE (object),
                                         g_value_get_boolean (value));
      break;
    case PROP_EXCLUSIVE:
      ctk_gesture_single_set_exclusive (CTK_GESTURE_SINGLE (object),
                                        g_value_get_boolean (value));
      break;
    case PROP_BUTTON:
      ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (object),
                                     g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_gesture_single_cancel (CtkGesture       *gesture,
                           CdkEventSequence *sequence)
{
  CtkGestureSinglePrivate *priv;

  priv = ctk_gesture_single_get_instance_private (CTK_GESTURE_SINGLE (gesture));

  if (sequence == priv->current_sequence)
    priv->current_button = 0;
}

static gboolean
ctk_gesture_single_handle_event (CtkEventController *controller,
                                 const CdkEvent     *event)
{
  CdkEventSequence *sequence = NULL;
  CtkGestureSinglePrivate *priv;
  CdkDevice *source_device;
  CdkInputSource source;
  guint button = 0, i;
  gboolean retval, test_touchscreen = FALSE;

  source_device = cdk_event_get_source_device (event);

  if (!source_device)
    return FALSE;

  priv = ctk_gesture_single_get_instance_private (CTK_GESTURE_SINGLE (controller));
  source = cdk_device_get_source (source_device);

  if (source != GDK_SOURCE_TOUCHSCREEN)
    test_touchscreen = ctk_simulate_touchscreen ();

  switch (event->type)
    {
    case GDK_TOUCH_BEGIN:
    case GDK_TOUCH_END:
    case GDK_TOUCH_UPDATE:
      if (priv->exclusive && !event->touch.emulating_pointer)
        return FALSE;

      sequence = event->touch.sequence;
      button = 1;
      break;
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      if (priv->touch_only && !test_touchscreen && source != GDK_SOURCE_TOUCHSCREEN)
        return FALSE;

      button = event->button.button;
      break;
    case GDK_MOTION_NOTIFY:
      if (!ctk_gesture_handles_sequence (CTK_GESTURE (controller), sequence))
        return FALSE;
      if (priv->touch_only && !test_touchscreen && source != GDK_SOURCE_TOUCHSCREEN)
        return FALSE;

      if (priv->current_button > 0 && priv->current_button <= 5 &&
          (event->motion.state & (GDK_BUTTON1_MASK << (priv->current_button - 1))))
        button = priv->current_button;
      else if (priv->current_button == 0)
        {
          /* No current button, find out from the mask */
          for (i = 0; i < 3; i++)
            {
              if ((event->motion.state & (GDK_BUTTON1_MASK << i)) == 0)
                continue;
              button = i + 1;
              break;
            }
        }

      break;
    case GDK_TOUCH_CANCEL:
    case GDK_GRAB_BROKEN:
    case GDK_TOUCHPAD_SWIPE:
      return CTK_EVENT_CONTROLLER_CLASS (ctk_gesture_single_parent_class)->handle_event (controller,
                                                                                         event);
      break;
    default:
      return FALSE;
    }

  if (button == 0 ||
      (priv->button != 0 && priv->button != button) ||
      (priv->current_button != 0 && priv->current_button != button))
    {
      if (ctk_gesture_is_active (CTK_GESTURE (controller)))
        ctk_event_controller_reset (controller);
      return FALSE;
    }

  if (event->type == GDK_BUTTON_PRESS || event->type == GDK_TOUCH_BEGIN ||
      event->type == GDK_MOTION_NOTIFY || event->type == GDK_TOUCH_UPDATE)
    {
      if (!ctk_gesture_is_active (CTK_GESTURE (controller)))
        priv->current_sequence = sequence;

      priv->current_button = button;
    }

  retval = CTK_EVENT_CONTROLLER_CLASS (ctk_gesture_single_parent_class)->handle_event (controller, event);

  if (sequence == priv->current_sequence &&
      (event->type == GDK_BUTTON_RELEASE || event->type == GDK_TOUCH_END))
    priv->current_button = 0;
  else if (priv->current_sequence == sequence &&
           !ctk_gesture_handles_sequence (CTK_GESTURE (controller), sequence))
    {
      if (button == priv->current_button && event->type == GDK_BUTTON_PRESS)
        priv->current_button = 0;
      else if (sequence == priv->current_sequence && event->type == GDK_TOUCH_BEGIN)
        priv->current_sequence = NULL;
    }

  return retval;
}

static void
ctk_gesture_single_class_init (CtkGestureSingleClass *klass)
{
  CtkEventControllerClass *controller_class = CTK_EVENT_CONTROLLER_CLASS (klass);
  CtkGestureClass *gesture_class = CTK_GESTURE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ctk_gesture_single_get_property;
  object_class->set_property = ctk_gesture_single_set_property;

  controller_class->handle_event = ctk_gesture_single_handle_event;

  gesture_class->cancel = ctk_gesture_single_cancel;

  /**
   * CtkGestureSingle:touch-only:
   *
   * Whether the gesture handles only touch events.
   *
   * Since: 3.14
   */
  properties[PROP_TOUCH_ONLY] =
      g_param_spec_boolean ("touch-only",
                            P_("Handle only touch events"),
                            P_("Whether the gesture handles only touch events"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkGestureSingle:exclusive:
   *
   * Whether the gesture is exclusive. Exclusive gestures only listen to pointer
   * and pointer emulated events.
   *
   * Since: 3.14
   */
  properties[PROP_EXCLUSIVE] =
      g_param_spec_boolean ("exclusive",
                            P_("Whether the gesture is exclusive"),
                            P_("Whether the gesture is exclusive"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkGestureSingle:button:
   *
   * Mouse button number to listen to, or 0 to listen for any button.
   *
   * Since: 3.14
   */
  properties[PROP_BUTTON] =
      g_param_spec_uint ("button",
                         P_("Button number"),
                         P_("Button number to listen to"),
                         0, G_MAXUINT,
                         GDK_BUTTON_PRIMARY,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
_ctk_gesture_single_update_evmask (CtkGestureSingle *gesture)
{
  CtkGestureSinglePrivate *priv;
  CdkEventMask evmask;

  priv = ctk_gesture_single_get_instance_private (gesture);
  evmask = GDK_TOUCH_MASK;

  if (!priv->touch_only || ctk_simulate_touchscreen ())
    evmask |= GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
      GDK_BUTTON_MOTION_MASK;

  ctk_event_controller_set_event_mask (CTK_EVENT_CONTROLLER (gesture), evmask);
}

static void
ctk_gesture_single_init (CtkGestureSingle *gesture)
{
  CtkGestureSinglePrivate *priv;

  priv = ctk_gesture_single_get_instance_private (gesture);
  priv->touch_only = FALSE;
  priv->button = GDK_BUTTON_PRIMARY;
  _ctk_gesture_single_update_evmask (gesture);
}

/**
 * ctk_gesture_single_get_touch_only:
 * @gesture: a #CtkGestureSingle
 *
 * Returns %TRUE if the gesture is only triggered by touch events.
 *
 * Returns: %TRUE if the gesture only handles touch events
 *
 * Since: 3.14
 **/
gboolean
ctk_gesture_single_get_touch_only (CtkGestureSingle *gesture)
{
  CtkGestureSinglePrivate *priv;

  g_return_val_if_fail (CTK_IS_GESTURE_SINGLE (gesture), FALSE);

  priv = ctk_gesture_single_get_instance_private (gesture);

  return priv->touch_only;
}

/**
 * ctk_gesture_single_set_touch_only:
 * @gesture: a #CtkGestureSingle
 * @touch_only: whether @gesture handles only touch events
 *
 * If @touch_only is %TRUE, @gesture will only handle events of type
 * #GDK_TOUCH_BEGIN, #GDK_TOUCH_UPDATE or #GDK_TOUCH_END. If %FALSE,
 * mouse events will be handled too.
 *
 * Since: 3.14
 **/
void
ctk_gesture_single_set_touch_only (CtkGestureSingle *gesture,
                                   gboolean          touch_only)
{
  CtkGestureSinglePrivate *priv;

  g_return_if_fail (CTK_IS_GESTURE_SINGLE (gesture));

  touch_only = touch_only != FALSE;
  priv = ctk_gesture_single_get_instance_private (gesture);

  if (priv->touch_only == touch_only)
    return;

  priv->touch_only = touch_only;
  _ctk_gesture_single_update_evmask (gesture);
  g_object_notify_by_pspec (G_OBJECT (gesture), properties[PROP_TOUCH_ONLY]);
}

/**
 * ctk_gesture_single_get_exclusive:
 * @gesture: a #CtkGestureSingle
 *
 * Gets whether a gesture is exclusive. For more information, see
 * ctk_gesture_single_set_exclusive().
 *
 * Returns: Whether the gesture is exclusive
 *
 * Since: 3.14
 **/
gboolean
ctk_gesture_single_get_exclusive (CtkGestureSingle *gesture)
{
  CtkGestureSinglePrivate *priv;

  g_return_val_if_fail (CTK_IS_GESTURE_SINGLE (gesture), FALSE);

  priv = ctk_gesture_single_get_instance_private (gesture);

  return priv->exclusive;
}

/**
 * ctk_gesture_single_set_exclusive:
 * @gesture: a #CtkGestureSingle
 * @exclusive: %TRUE to make @gesture exclusive
 *
 * Sets whether @gesture is exclusive. An exclusive gesture will
 * only handle pointer and "pointer emulated" touch events, so at
 * any given time, there is only one sequence able to interact with
 * those.
 *
 * Since: 3.14
 **/
void
ctk_gesture_single_set_exclusive (CtkGestureSingle *gesture,
                                  gboolean          exclusive)
{
  CtkGestureSinglePrivate *priv;

  g_return_if_fail (CTK_IS_GESTURE_SINGLE (gesture));

  exclusive = exclusive != FALSE;
  priv = ctk_gesture_single_get_instance_private (gesture);

  if (priv->exclusive == exclusive)
    return;

  priv->exclusive = exclusive;
  _ctk_gesture_single_update_evmask (gesture);
  g_object_notify_by_pspec (G_OBJECT (gesture), properties[PROP_EXCLUSIVE]);
}

/**
 * ctk_gesture_single_get_button:
 * @gesture: a #CtkGestureSingle
 *
 * Returns the button number @gesture listens for, or 0 if @gesture
 * reacts to any button press.
 *
 * Returns: The button number, or 0 for any button
 *
 * Since: 3.14
 **/
guint
ctk_gesture_single_get_button (CtkGestureSingle *gesture)
{
  CtkGestureSinglePrivate *priv;

  g_return_val_if_fail (CTK_IS_GESTURE_SINGLE (gesture), 0);

  priv = ctk_gesture_single_get_instance_private (gesture);

  return priv->button;
}

/**
 * ctk_gesture_single_set_button:
 * @gesture: a #CtkGestureSingle
 * @button: button number to listen to, or 0 for any button
 *
 * Sets the button number @gesture listens to. If non-0, every
 * button press from a different button number will be ignored.
 * Touch events implicitly match with button 1.
 *
 * Since: 3.14
 **/
void
ctk_gesture_single_set_button (CtkGestureSingle *gesture,
                               guint             button)
{
  CtkGestureSinglePrivate *priv;

  g_return_if_fail (CTK_IS_GESTURE_SINGLE (gesture));

  priv = ctk_gesture_single_get_instance_private (gesture);

  if (priv->button == button)
    return;

  priv->button = button;
  g_object_notify_by_pspec (G_OBJECT (gesture), properties[PROP_BUTTON]);
}

/**
 * ctk_gesture_single_get_current_button:
 * @gesture: a #CtkGestureSingle
 *
 * Returns the button number currently interacting with @gesture, or 0 if there
 * is none.
 *
 * Returns: The current button number
 *
 * Since: 3.14
 **/
guint
ctk_gesture_single_get_current_button (CtkGestureSingle *gesture)
{
  CtkGestureSinglePrivate *priv;

  g_return_val_if_fail (CTK_IS_GESTURE_SINGLE (gesture), 0);

  priv = ctk_gesture_single_get_instance_private (gesture);

  return priv->current_button;
}

/**
 * ctk_gesture_single_get_current_sequence:
 * @gesture: a #CtkGestureSingle
 *
 * Returns the event sequence currently interacting with @gesture.
 * This is only meaningful if ctk_gesture_is_active() returns %TRUE.
 *
 * Returns: (nullable): the current sequence
 *
 * Since: 3.14
 **/
CdkEventSequence *
ctk_gesture_single_get_current_sequence (CtkGestureSingle *gesture)
{
  CtkGestureSinglePrivate *priv;

  g_return_val_if_fail (CTK_IS_GESTURE_SINGLE (gesture), NULL);

  priv = ctk_gesture_single_get_instance_private (gesture);

  return priv->current_sequence;
}
