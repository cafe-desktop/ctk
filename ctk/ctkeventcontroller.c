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
 * SECTION:ctkeventcontroller
 * @Short_description: Self-contained handler of series of events
 * @Title: CtkEventController
 * @See_also: #CtkGesture
 *
 * #CtkEventController is a base, low-level implementation for event
 * controllers. Those react to a series of #CdkEvents, and possibly trigger
 * actions as a consequence of those.
 */

#include "config.h"
#include "ctkeventcontroller.h"
#include "ctkeventcontrollerprivate.h"
#include "ctkwidgetprivate.h"
#include "ctktypebuiltins.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctkintl.h"

typedef struct _CtkEventControllerPrivate CtkEventControllerPrivate;

enum {
  PROP_WIDGET = 1,
  PROP_PROPAGATION_PHASE,
  LAST_PROP
};

static GParamSpec *properties[LAST_PROP] = { NULL, };

struct _CtkEventControllerPrivate
{
  CtkWidget *widget;
  guint evmask;
  CtkPropagationPhase phase;
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (CtkEventController, ctk_event_controller, G_TYPE_OBJECT)

static gboolean
ctk_event_controller_handle_event_default (CtkEventController *controller,
                                           const CdkEvent     *event)
{
  return FALSE;
}

static void
ctk_event_controller_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  CtkEventControllerPrivate *priv;

  priv = ctk_event_controller_get_instance_private (CTK_EVENT_CONTROLLER (object));

  switch (prop_id)
    {
    case PROP_WIDGET:
      priv->widget = g_value_get_object (value);
      if (priv->widget)
        g_object_add_weak_pointer (G_OBJECT (priv->widget), (gpointer *) &priv->widget);
      break;
    case PROP_PROPAGATION_PHASE:
      ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (object),
                                                  g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_event_controller_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  CtkEventControllerPrivate *priv;

  priv = ctk_event_controller_get_instance_private (CTK_EVENT_CONTROLLER (object));

  switch (prop_id)
    {
    case PROP_WIDGET:
      g_value_set_object (value, priv->widget);
      break;
    case PROP_PROPAGATION_PHASE:
      g_value_set_enum (value, priv->phase);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_event_controller_constructed (GObject *object)
{
  CtkEventController *controller = CTK_EVENT_CONTROLLER (object);
  CtkEventControllerPrivate *priv;

  G_OBJECT_CLASS (ctk_event_controller_parent_class)->constructed (object);

  priv = ctk_event_controller_get_instance_private (controller);
  if (priv->widget)
    _ctk_widget_add_controller (priv->widget, controller);
}

static void
ctk_event_controller_dispose (GObject *object)
{
  CtkEventController *controller = CTK_EVENT_CONTROLLER (object);
  CtkEventControllerPrivate *priv;

  priv = ctk_event_controller_get_instance_private (controller);
  if (priv->widget)
    {
      _ctk_widget_remove_controller (priv->widget, controller);
      g_object_remove_weak_pointer (G_OBJECT (priv->widget), (gpointer *) &priv->widget);
      priv->widget = NULL;
    }

  G_OBJECT_CLASS (ctk_event_controller_parent_class)->dispose (object);
}

static void
ctk_event_controller_class_init (CtkEventControllerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  klass->filter_event = ctk_event_controller_handle_event_default;
  klass->handle_event = ctk_event_controller_handle_event_default;

  object_class->set_property = ctk_event_controller_set_property;
  object_class->get_property = ctk_event_controller_get_property;
  object_class->constructed = ctk_event_controller_constructed;
  object_class->dispose = ctk_event_controller_dispose;

  /**
   * CtkEventController:widget:
   *
   * The widget receiving the #CdkEvents that the controller will handle.
   *
   * Since: 3.14
   */
  properties[PROP_WIDGET] =
      g_param_spec_object ("widget",
                           P_("Widget"),
                           P_("Widget the gesture relates to"),
                           CTK_TYPE_WIDGET,
                           CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);
  /**
   * CtkEventController:propagation-phase:
   *
   * The propagation phase at which this controller will handle events.
   *
   * Since: 3.14
   */
  properties[PROP_PROPAGATION_PHASE] =
      g_param_spec_enum ("propagation-phase",
                         P_("Propagation phase"),
                         P_("Propagation phase at which this controller is run"),
                         CTK_TYPE_PROPAGATION_PHASE,
                         CTK_PHASE_BUBBLE,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
ctk_event_controller_init (CtkEventController *controller)
{
  CtkEventControllerPrivate *priv;

  priv = ctk_event_controller_get_instance_private (controller);
  priv->phase = CTK_PHASE_BUBBLE;
}

/**
 * ctk_event_controller_handle_event:
 * @controller: a #CtkEventController
 * @event: a #CdkEvent
 *
 * Feeds an events into @controller, so it can be interpreted
 * and the controller actions triggered.
 *
 * Returns: %TRUE if the event was potentially useful to trigger the
 *          controller action
 *
 * Since: 3.14
 **/
gboolean
ctk_event_controller_handle_event (CtkEventController *controller,
                                   const CdkEvent     *event)
{
  CtkEventControllerClass *controller_class;
  gboolean retval = FALSE;

  g_return_val_if_fail (CTK_IS_EVENT_CONTROLLER (controller), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  controller_class = CTK_EVENT_CONTROLLER_GET_CLASS (controller);

  if (controller_class->filter_event (controller, event))
    return retval;

  if (controller_class->handle_event)
    {
      g_object_ref (controller);
      retval = controller_class->handle_event (controller, event);
      g_object_unref (controller);
    }

  return retval;
}

void
ctk_event_controller_set_event_mask (CtkEventController *controller,
                                     CdkEventMask        event_mask)
{
  CtkEventControllerPrivate *priv;

  g_return_if_fail (CTK_IS_EVENT_CONTROLLER (controller));

  priv = ctk_event_controller_get_instance_private (controller);

  if (priv->evmask == event_mask)
    return;

  priv->evmask = event_mask;
}

CdkEventMask
ctk_event_controller_get_event_mask (CtkEventController *controller)
{
  CtkEventControllerPrivate *priv;

  g_return_val_if_fail (CTK_IS_EVENT_CONTROLLER (controller), 0);

  priv = ctk_event_controller_get_instance_private (controller);

  return priv->evmask;
}

/**
 * ctk_event_controller_get_widget:
 * @controller: a #CtkEventController
 *
 * Returns the #CtkWidget this controller relates to.
 *
 * Returns: (transfer none): a #CtkWidget
 *
 * Since: 3.14
 **/
CtkWidget *
ctk_event_controller_get_widget (CtkEventController *controller)
{
  CtkEventControllerPrivate *priv;

  g_return_val_if_fail (CTK_IS_EVENT_CONTROLLER (controller), 0);

  priv = ctk_event_controller_get_instance_private (controller);

  return priv->widget;
}

/**
 * ctk_event_controller_reset:
 * @controller: a #CtkEventController
 *
 * Resets the @controller to a clean state. Every interaction
 * the controller did through #CtkEventController::handle-event
 * will be dropped at this point.
 *
 * Since: 3.14
 **/
void
ctk_event_controller_reset (CtkEventController *controller)
{
  CtkEventControllerClass *controller_class;

  g_return_if_fail (CTK_IS_EVENT_CONTROLLER (controller));

  controller_class = CTK_EVENT_CONTROLLER_GET_CLASS (controller);

  if (controller_class->reset)
    controller_class->reset (controller);
}

/**
 * ctk_event_controller_get_propagation_phase:
 * @controller: a #CtkEventController
 *
 * Gets the propagation phase at which @controller handles events.
 *
 * Returns: the propagation phase
 *
 * Since: 3.14
 **/
CtkPropagationPhase
ctk_event_controller_get_propagation_phase (CtkEventController *controller)
{
  CtkEventControllerPrivate *priv;

  g_return_val_if_fail (CTK_IS_EVENT_CONTROLLER (controller), CTK_PHASE_NONE);

  priv = ctk_event_controller_get_instance_private (controller);

  return priv->phase;
}

/**
 * ctk_event_controller_set_propagation_phase:
 * @controller: a #CtkEventController
 * @phase: a propagation phase
 *
 * Sets the propagation phase at which a controller handles events.
 *
 * If @phase is %CTK_PHASE_NONE, no automatic event handling will be
 * performed, but other additional gesture maintenance will. In that phase,
 * the events can be managed by calling ctk_event_controller_handle_event().
 *
 * Since: 3.14
 **/
void
ctk_event_controller_set_propagation_phase (CtkEventController  *controller,
                                            CtkPropagationPhase  phase)
{
  CtkEventControllerPrivate *priv;

  g_return_if_fail (CTK_IS_EVENT_CONTROLLER (controller));
  g_return_if_fail (phase >= CTK_PHASE_NONE && phase <= CTK_PHASE_TARGET);

  priv = ctk_event_controller_get_instance_private (controller);

  if (priv->phase == phase)
    return;

  priv->phase = phase;

  if (phase == CTK_PHASE_NONE)
    ctk_event_controller_reset (controller);

  g_object_notify_by_pspec (G_OBJECT (controller), properties[PROP_PROPAGATION_PHASE]);
}
