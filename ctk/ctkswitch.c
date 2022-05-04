/* CTK - The GIMP Toolkit
 *
 * Copyright (C) 2010  Intel Corporation
 * Copyright (C) 2010  RedHat, Inc.
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
 * Author:
 *      Emmanuele Bassi <ebassi@linux.intel.com>
 *      Matthias Clasen <mclasen@redhat.com>
 *
 * Based on similar code from Mx.
 */

/**
 * SECTION:ctkswitch
 * @Short_Description: A “light switch” style toggle
 * @Title: CtkSwitch
 * @See_Also: #CtkToggleButton
 *
 * #CtkSwitch is a widget that has two states: on or off. The user can control
 * which state should be active by clicking the empty area, or by dragging the
 * handle.
 *
 * CtkSwitch can also handle situations where the underlying state changes with
 * a delay. See #CtkSwitch::state-set for details.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * switch
 * ╰── slider
 * ]|
 *
 * CtkSwitch has two css nodes, the main node with the name switch and a subnode
 * named slider. Neither of them is using any style classes.
 */

#include "config.h"

#include "ctkswitch.h"

#include "ctkactivatable.h"
#include "ctktoggleaction.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkwidget.h"
#include "ctkmarshalers.h"
#include "ctkapplicationprivate.h"
#include "ctkactionable.h"
#include "a11y/ctkswitchaccessible.h"
#include "ctkactionhelper.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkcssgadgetprivate.h"
#include "ctkiconhelperprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkcssshadowsvalueprivate.h"
#include "ctkcssnumbervalueprivate.h"
#include "ctkprogresstrackerprivate.h"
#include "ctksettingsprivate.h"

#include "fallback-c89.c"

#define DEFAULT_SLIDER_WIDTH    (36)
#define DEFAULT_SLIDER_HEIGHT   (22)

struct _CtkSwitchPrivate
{
  CdkWindow *event_window;
  CtkAction *action;
  CtkActionHelper *action_helper;

  CtkGesture *pan_gesture;
  CtkGesture *multipress_gesture;

  CtkCssGadget *gadget;
  CtkCssGadget *slider_gadget;
  CtkCssGadget *on_gadget;
  CtkCssGadget *off_gadget;

  double handle_pos;
  guint tick_id;
  CtkProgressTracker tracker;

  guint state                 : 1;
  guint is_active             : 1;
  guint in_switch             : 1;
  guint use_action_appearance : 1;
};

enum
{
  PROP_0,
  PROP_ACTIVE,
  PROP_STATE,
  PROP_RELATED_ACTION,
  PROP_USE_ACTION_APPEARANCE,
  LAST_PROP,
  PROP_ACTION_NAME,
  PROP_ACTION_TARGET
};

enum
{
  ACTIVATE,
  STATE_SET,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static GParamSpec *switch_props[LAST_PROP] = { NULL, };

static void ctk_switch_actionable_iface_init (CtkActionableInterface *iface);
static void ctk_switch_activatable_interface_init (CtkActivatableIface *iface);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
G_DEFINE_TYPE_WITH_CODE (CtkSwitch, ctk_switch, CTK_TYPE_WIDGET,
                         G_ADD_PRIVATE (CtkSwitch)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIONABLE,
                                                ctk_switch_actionable_iface_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIVATABLE,
                                                ctk_switch_activatable_interface_init));
G_GNUC_END_IGNORE_DEPRECATIONS;

static void
ctk_switch_end_toggle_animation (CtkSwitch *sw)
{
  CtkSwitchPrivate *priv = sw->priv;

  if (priv->tick_id != 0)
    {
      ctk_widget_remove_tick_callback (CTK_WIDGET (sw), priv->tick_id);
      priv->tick_id = 0;
    }
}

static gboolean
ctk_switch_on_frame_clock_update (CtkWidget     *widget,
                                  CdkFrameClock *clock,
                                  gpointer       user_data)
{
  CtkSwitch *sw = CTK_SWITCH (widget);
  CtkSwitchPrivate *priv = sw->priv;

  ctk_progress_tracker_advance_frame (&priv->tracker,
                                      cdk_frame_clock_get_frame_time (clock));

  if (ctk_progress_tracker_get_state (&priv->tracker) != CTK_PROGRESS_STATE_AFTER)
    {
      if (priv->is_active)
        priv->handle_pos = 1.0 - ctk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE);
      else
        priv->handle_pos = ctk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE);
    }
  else
    {
      ctk_switch_set_active (sw, !priv->is_active);
    }

  ctk_widget_queue_allocate (CTK_WIDGET (sw));

  return G_SOURCE_CONTINUE;
}

#define ANIMATION_DURATION 100

static void
ctk_switch_begin_toggle_animation (CtkSwitch *sw)
{
  CtkSwitchPrivate *priv = sw->priv;

  if (ctk_settings_get_enable_animations (ctk_widget_get_settings (CTK_WIDGET (sw))))
    {
      ctk_progress_tracker_start (&priv->tracker, 1000 * ANIMATION_DURATION, 0, 1.0);
      if (priv->tick_id == 0)
        priv->tick_id = ctk_widget_add_tick_callback (CTK_WIDGET (sw),
                                                      ctk_switch_on_frame_clock_update,
                                                      NULL, NULL);
    }
  else
    {
      ctk_switch_set_active (sw, !priv->is_active);
    }
}

static void
ctk_switch_multipress_gesture_pressed (CtkGestureMultiPress *gesture,
                                       gint                  n_press,
                                       gdouble               x,
                                       gdouble               y,
                                       CtkSwitch            *sw)
{
  CtkSwitchPrivate *priv = sw->priv;
  CtkAllocation allocation;

  ctk_widget_get_allocation (CTK_WIDGET (sw), &allocation);
  ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_CLAIMED);
  priv->in_switch = TRUE;

  /* If the press didn't happen in the draggable handle,
   * cancel the pan gesture right away
   */
  if ((priv->is_active && x <= allocation.width / 2.0) ||
      (!priv->is_active && x > allocation.width / 2.0))
    ctk_gesture_set_state (priv->pan_gesture, CTK_EVENT_SEQUENCE_DENIED);
}

static void
ctk_switch_multipress_gesture_released (CtkGestureMultiPress *gesture,
                                        gint                  n_press,
                                        gdouble               x,
                                        gdouble               y,
                                        CtkSwitch            *sw)
{
  CtkSwitchPrivate *priv = sw->priv;
  CdkEventSequence *sequence;

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));

  if (priv->in_switch &&
      ctk_gesture_handles_sequence (CTK_GESTURE (gesture), sequence))
    ctk_switch_begin_toggle_animation (sw);

  priv->in_switch = FALSE;
}

static void
ctk_switch_pan_gesture_pan (CtkGesturePan   *gesture,
                            CtkPanDirection  direction,
                            gdouble          offset,
                            CtkSwitch       *sw)
{
  CtkWidget *widget = CTK_WIDGET (sw);
  CtkSwitchPrivate *priv = sw->priv;
  gint width;

  if (direction == CTK_PAN_DIRECTION_LEFT)
    offset = -offset;

  ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_CLAIMED);

  width = ctk_widget_get_allocated_width (widget);

  if (priv->is_active)
    offset += width / 2;
  
  offset /= width / 2;
  /* constrain the handle within the trough width */
  priv->handle_pos = CLAMP (offset, 0, 1.0);

  /* we need to redraw the handle */
  ctk_widget_queue_allocate (widget);
}

static void
ctk_switch_pan_gesture_drag_end (CtkGestureDrag *gesture,
                                 gdouble         x,
                                 gdouble         y,
                                 CtkSwitch      *sw)
{
  CtkSwitchPrivate *priv = sw->priv;
  CdkEventSequence *sequence;
  CtkAllocation allocation;
  gboolean active;

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));

  if (ctk_gesture_get_sequence_state (CTK_GESTURE (gesture), sequence) == CTK_EVENT_SEQUENCE_CLAIMED)
    {
      ctk_widget_get_allocation (CTK_WIDGET (sw), &allocation);

      /* if half the handle passed the middle of the switch, then we
       * consider it to be on
       */
      active = priv->handle_pos >= 0.5;
    }
  else if (!ctk_gesture_handles_sequence (priv->multipress_gesture, sequence))
    active = priv->is_active;
  else
    return;

  priv->handle_pos = active ? 1.0 : 0.0;
  ctk_switch_set_active (sw, active);
  ctk_widget_queue_allocate (CTK_WIDGET (sw));
}

static gboolean
ctk_switch_enter (CtkWidget        *widget,
                  CdkEventCrossing *event)
{
  CtkSwitchPrivate *priv = CTK_SWITCH (widget)->priv;

  if (event->window == priv->event_window)
    {
      priv->in_switch = TRUE;
      ctk_widget_set_state_flags (widget, CTK_STATE_FLAG_PRELIGHT, FALSE);
    }

  return FALSE;
}

static gboolean
ctk_switch_leave (CtkWidget        *widget,
                  CdkEventCrossing *event)
{
  CtkSwitchPrivate *priv = CTK_SWITCH (widget)->priv;

  if (event->window == priv->event_window)
    {
      priv->in_switch = FALSE;
      ctk_widget_unset_state_flags (widget, CTK_STATE_FLAG_PRELIGHT);
    }

  return FALSE;
}

static void
ctk_switch_activate (CtkSwitch *sw)
{
  ctk_switch_begin_toggle_animation (sw);
}

static void
ctk_switch_get_slider_size (CtkCssGadget   *gadget,
                            CtkOrientation  orientation,
                            gint            for_size,
                            gint           *minimum,
                            gint           *natural,
                            gint           *minimum_baseline,
                            gint           *natural_baseline,
                            gpointer        unused)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  gdouble min_size;

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      min_size = _ctk_css_number_value_get (ctk_css_style_get_value (ctk_css_gadget_get_style (gadget), CTK_CSS_PROPERTY_MIN_WIDTH), 100);

      if (min_size > 0.0)
        *minimum = 0;
      else
        ctk_widget_style_get (widget, "slider-width", minimum, NULL);
    }
  else
    {
      min_size = _ctk_css_number_value_get (ctk_css_style_get_value (ctk_css_gadget_get_style (gadget), CTK_CSS_PROPERTY_MIN_HEIGHT), 100);

      if (min_size > 0.0)
        *minimum = 0;
      else
        ctk_widget_style_get (widget, "slider-height", minimum, NULL);
    }

  *natural = *minimum;
}

static void
ctk_switch_get_content_size (CtkCssGadget   *gadget,
                             CtkOrientation  orientation,
                             gint            for_size,
                             gint           *minimum,
                             gint           *natural,
                             gint           *minimum_baseline,
                             gint           *natural_baseline,
                             gpointer        unused)
{
  CtkWidget *widget;
  CtkSwitch *self;
  CtkSwitchPrivate *priv;
  gint slider_minimum, slider_natural;
  gint on_minimum, on_natural;
  gint off_minimum, off_natural;

  widget = ctk_css_gadget_get_owner (gadget);
  self = CTK_SWITCH (widget);
  priv = self->priv;

  ctk_css_gadget_get_preferred_size (priv->slider_gadget,
                                     orientation,
                                     -1,
                                     &slider_minimum, &slider_natural,
                                     NULL, NULL);

  ctk_css_gadget_get_preferred_size (priv->on_gadget,
                                     orientation,
                                     -1,
                                     &on_minimum, &on_natural,
                                     NULL, NULL);
  ctk_css_gadget_get_preferred_size (priv->off_gadget,
                                     orientation,
                                     -1,
                                     &off_minimum, &off_natural,
                                     NULL, NULL);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      *minimum = 2 * MAX (slider_minimum, MAX (on_minimum, off_minimum));
      *natural = 2 * MAX (slider_natural, MAX (on_natural, off_natural));
    }
  else
    {
      *minimum = MAX (slider_minimum, MAX (on_minimum, off_minimum));
      *natural = MAX (slider_natural, MAX (on_natural, off_natural));
    }
}

static void
ctk_switch_get_preferred_width (CtkWidget *widget,
                                gint      *minimum,
                                gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_SWITCH (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_switch_get_preferred_height (CtkWidget *widget,
                                 gint      *minimum,
                                 gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_SWITCH (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_switch_allocate_contents (CtkCssGadget        *gadget,
                              const CtkAllocation *allocation,
                              int                  baseline,
                              CtkAllocation       *out_clip,
                              gpointer             unused)
{
  CtkSwitch *self = CTK_SWITCH (ctk_css_gadget_get_owner (gadget));
  CtkSwitchPrivate *priv = self->priv;
  CtkAllocation child_alloc;
  CtkAllocation on_clip, off_clip;

  child_alloc.x = allocation->x + round (priv->handle_pos * (allocation->width - allocation->width / 2));
  child_alloc.y = allocation->y;
  child_alloc.width = allocation->width / 2;
  child_alloc.height = allocation->height;

  ctk_css_gadget_allocate (priv->slider_gadget,
                           &child_alloc,
                           baseline,
                           out_clip);

  child_alloc.x = allocation->x;

  ctk_css_gadget_allocate (priv->on_gadget,
                           &child_alloc,
                           baseline,
                           &on_clip);

  cdk_rectangle_union (out_clip, &on_clip, out_clip);

  child_alloc.x = allocation->x + allocation->width - child_alloc.width;
  ctk_css_gadget_allocate (priv->off_gadget,
                           &child_alloc,
                           baseline,
                           &off_clip);

  cdk_rectangle_union (out_clip, &off_clip, out_clip);

  if (ctk_widget_get_realized (CTK_WIDGET (self)))
    {
      CtkAllocation border_allocation;
      ctk_css_gadget_get_border_allocation (gadget, &border_allocation, NULL);
      cdk_window_move_resize (priv->event_window,
                              border_allocation.x,
                              border_allocation.y,
                              border_allocation.width,
                              border_allocation.height);
    }
}

static void
ctk_switch_size_allocate (CtkWidget     *widget,
                          CtkAllocation *allocation)
{
  CtkSwitchPrivate *priv = CTK_SWITCH (widget)->priv;
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);
  ctk_css_gadget_allocate (priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_switch_realize (CtkWidget *widget)
{
  CtkSwitchPrivate *priv = CTK_SWITCH (widget)->priv;
  CdkWindow *parent_window;
  CdkWindowAttr attributes;
  gint attributes_mask;
  CtkAllocation allocation;

  ctk_widget_set_realized (widget, TRUE);
  parent_window = ctk_widget_get_parent_window (widget);
  ctk_widget_set_window (widget, parent_window);
  g_object_ref (parent_window);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.wclass = CDK_INPUT_ONLY;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.event_mask = ctk_widget_get_events (widget);
  attributes.event_mask |= (CDK_BUTTON_PRESS_MASK |
                            CDK_BUTTON_RELEASE_MASK |
                            CDK_BUTTON1_MOTION_MASK |
                            CDK_POINTER_MOTION_MASK |
                            CDK_ENTER_NOTIFY_MASK |
                            CDK_LEAVE_NOTIFY_MASK);
  attributes_mask = CDK_WA_X | CDK_WA_Y;

  priv->event_window = cdk_window_new (parent_window,
                                       &attributes,
                                       attributes_mask);
  ctk_widget_register_window (widget, priv->event_window);
}

static void
ctk_switch_unrealize (CtkWidget *widget)
{
  CtkSwitchPrivate *priv = CTK_SWITCH (widget)->priv;

  if (priv->event_window != NULL)
    {
      ctk_widget_unregister_window (widget, priv->event_window);
      cdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  CTK_WIDGET_CLASS (ctk_switch_parent_class)->unrealize (widget);
}

static void
ctk_switch_map (CtkWidget *widget)
{
  CtkSwitchPrivate *priv = CTK_SWITCH (widget)->priv;

  CTK_WIDGET_CLASS (ctk_switch_parent_class)->map (widget);

  if (priv->event_window)
    cdk_window_show (priv->event_window);
}

static void
ctk_switch_unmap (CtkWidget *widget)
{
  CtkSwitchPrivate *priv = CTK_SWITCH (widget)->priv;

  if (priv->event_window)
    cdk_window_hide (priv->event_window);

  CTK_WIDGET_CLASS (ctk_switch_parent_class)->unmap (widget);
}

static gboolean
ctk_switch_render_slider (CtkCssGadget *gadget,
                          cairo_t      *cr,
                          int           x,
                          int           y,
                          int           width,
                          int           height,
                          gpointer      data)
{
  return ctk_widget_has_visible_focus (ctk_css_gadget_get_owner (gadget));
}

static gboolean
ctk_switch_render_trough (CtkCssGadget *gadget,
                          cairo_t      *cr,
                          int           x,
                          int           y,
                          int           width,
                          int           height,
                          gpointer      data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkSwitchPrivate *priv = CTK_SWITCH (widget)->priv;

  ctk_css_gadget_draw (priv->on_gadget, cr);
  ctk_css_gadget_draw (priv->off_gadget, cr);
  ctk_css_gadget_draw (priv->slider_gadget, cr);

  return FALSE;
}

static gboolean
ctk_switch_draw (CtkWidget *widget,
                 cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_SWITCH (widget)->priv->gadget, cr);

  return FALSE;
}

static void
ctk_switch_state_flags_changed (CtkWidget     *widget,
                                CtkStateFlags  previous_state_flags)
{
  CtkSwitchPrivate *priv = CTK_SWITCH (widget)->priv;
  CtkStateFlags state = ctk_widget_get_state_flags (widget);

  ctk_css_gadget_set_state (priv->gadget, state);
  ctk_css_gadget_set_state (priv->slider_gadget, state);
  ctk_css_gadget_set_state (priv->on_gadget, state);
  ctk_css_gadget_set_state (priv->off_gadget, state);

  CTK_WIDGET_CLASS (ctk_switch_parent_class)->state_flags_changed (widget, previous_state_flags);
}

static void
ctk_switch_set_related_action (CtkSwitch *sw,
                               CtkAction *action)
{
  CtkSwitchPrivate *priv = sw->priv;

  if (priv->action == action)
    return;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  ctk_activatable_do_set_related_action (CTK_ACTIVATABLE (sw), action);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  priv->action = action;
}

static void
ctk_switch_set_use_action_appearance (CtkSwitch *sw,
                                      gboolean   use_appearance)
{
  CtkSwitchPrivate *priv = sw->priv;

  if (priv->use_action_appearance != use_appearance)
    {
      priv->use_action_appearance = use_appearance;

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_activatable_sync_action_properties (CTK_ACTIVATABLE (sw), priv->action);
      G_GNUC_END_IGNORE_DEPRECATIONS;
    }
}

static void
ctk_switch_set_action_name (CtkActionable *actionable,
                            const gchar   *action_name)
{
  CtkSwitch *sw = CTK_SWITCH (actionable);

  if (!sw->priv->action_helper)
    sw->priv->action_helper = ctk_action_helper_new (actionable);

  ctk_action_helper_set_action_name (sw->priv->action_helper, action_name);
}

static void
ctk_switch_set_action_target_value (CtkActionable *actionable,
                                    GVariant      *action_target)
{
  CtkSwitch *sw = CTK_SWITCH (actionable);

  if (!sw->priv->action_helper)
    sw->priv->action_helper = ctk_action_helper_new (actionable);

  ctk_action_helper_set_action_target_value (sw->priv->action_helper, action_target);
}

static const gchar *
ctk_switch_get_action_name (CtkActionable *actionable)
{
  CtkSwitch *sw = CTK_SWITCH (actionable);

  return ctk_action_helper_get_action_name (sw->priv->action_helper);
}

static GVariant *
ctk_switch_get_action_target_value (CtkActionable *actionable)
{
  CtkSwitch *sw = CTK_SWITCH (actionable);

  return ctk_action_helper_get_action_target_value (sw->priv->action_helper);
}

static void
ctk_switch_actionable_iface_init (CtkActionableInterface *iface)
{
  iface->get_action_name = ctk_switch_get_action_name;
  iface->set_action_name = ctk_switch_set_action_name;
  iface->get_action_target_value = ctk_switch_get_action_target_value;
  iface->set_action_target_value = ctk_switch_set_action_target_value;
}

static void
ctk_switch_set_property (GObject      *gobject,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  CtkSwitch *sw = CTK_SWITCH (gobject);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      ctk_switch_set_active (sw, g_value_get_boolean (value));
      break;

    case PROP_STATE:
      ctk_switch_set_state (sw, g_value_get_boolean (value));
      break;

    case PROP_RELATED_ACTION:
      ctk_switch_set_related_action (sw, g_value_get_object (value));
      break;

    case PROP_USE_ACTION_APPEARANCE:
      ctk_switch_set_use_action_appearance (sw, g_value_get_boolean (value));
      break;

    case PROP_ACTION_NAME:
      ctk_switch_set_action_name (CTK_ACTIONABLE (sw), g_value_get_string (value));
      break;

    case PROP_ACTION_TARGET:
      ctk_switch_set_action_target_value (CTK_ACTIONABLE (sw), g_value_get_variant (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
ctk_switch_get_property (GObject    *gobject,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  CtkSwitchPrivate *priv = CTK_SWITCH (gobject)->priv;

  switch (prop_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, priv->is_active);
      break;

    case PROP_STATE:
      g_value_set_boolean (value, priv->state);
      break;

    case PROP_RELATED_ACTION:
      g_value_set_object (value, priv->action);
      break;

    case PROP_USE_ACTION_APPEARANCE:
      g_value_set_boolean (value, priv->use_action_appearance);
      break;

    case PROP_ACTION_NAME:
      g_value_set_string (value, ctk_action_helper_get_action_name (priv->action_helper));
      break;

    case PROP_ACTION_TARGET:
      g_value_set_variant (value, ctk_action_helper_get_action_target_value (priv->action_helper));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
ctk_switch_dispose (GObject *object)
{
  CtkSwitchPrivate *priv = CTK_SWITCH (object)->priv;

  g_clear_object (&priv->action_helper);

  if (priv->action)
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_activatable_do_set_related_action (CTK_ACTIVATABLE (object), NULL);
      G_GNUC_END_IGNORE_DEPRECATIONS;
      priv->action = NULL;
    }

  g_clear_object (&priv->gadget);
  g_clear_object (&priv->slider_gadget);
  g_clear_object (&priv->on_gadget);
  g_clear_object (&priv->off_gadget);

  g_clear_object (&priv->pan_gesture);
  g_clear_object (&priv->multipress_gesture);

  G_OBJECT_CLASS (ctk_switch_parent_class)->dispose (object);
}

static void
ctk_switch_finalize (GObject *object)
{
  ctk_switch_end_toggle_animation (CTK_SWITCH (object));

  G_OBJECT_CLASS (ctk_switch_parent_class)->finalize (object);
}

static gboolean
state_set (CtkSwitch *sw, gboolean state)
{
  if (sw->priv->action_helper)
    ctk_action_helper_activate (sw->priv->action_helper);

  if (sw->priv->action)
    ctk_action_activate (sw->priv->action);

  ctk_switch_set_state (sw, state);

  return TRUE;
}

static void
ctk_switch_class_init (CtkSwitchClass *klass)
{
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  gpointer activatable_iface;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  activatable_iface = g_type_default_interface_peek (CTK_TYPE_ACTIVATABLE);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  switch_props[PROP_RELATED_ACTION] =
    g_param_spec_override ("related-action",
                           g_object_interface_find_property (activatable_iface,
                                                             "related-action"));

  switch_props[PROP_USE_ACTION_APPEARANCE] =
    g_param_spec_override ("use-action-appearance",
                           g_object_interface_find_property (activatable_iface,
                                                             "use-action-appearance"));

  /**
   * CtkSwitch:active:
   *
   * Whether the #CtkSwitch widget is in its on or off state.
   */
  switch_props[PROP_ACTIVE] =
    g_param_spec_boolean ("active",
                          P_("Active"),
                          P_("Whether the switch is on or off"),
                          FALSE,
                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkSwitch:state:
   *
   * The backend state that is controlled by the switch. 
   * See #CtkSwitch::state-set for details.
   *
   * Since: 3.14
   */
  switch_props[PROP_STATE] =
    g_param_spec_boolean ("state",
                          P_("State"),
                          P_("The backend state"),
                          FALSE,
                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  gobject_class->set_property = ctk_switch_set_property;
  gobject_class->get_property = ctk_switch_get_property;
  gobject_class->dispose = ctk_switch_dispose;
  gobject_class->finalize = ctk_switch_finalize;

  g_object_class_install_properties (gobject_class, LAST_PROP, switch_props);

  widget_class->get_preferred_width = ctk_switch_get_preferred_width;
  widget_class->get_preferred_height = ctk_switch_get_preferred_height;
  widget_class->size_allocate = ctk_switch_size_allocate;
  widget_class->realize = ctk_switch_realize;
  widget_class->unrealize = ctk_switch_unrealize;
  widget_class->map = ctk_switch_map;
  widget_class->unmap = ctk_switch_unmap;
  widget_class->draw = ctk_switch_draw;
  widget_class->enter_notify_event = ctk_switch_enter;
  widget_class->leave_notify_event = ctk_switch_leave;
  widget_class->state_flags_changed = ctk_switch_state_flags_changed;

  klass->activate = ctk_switch_activate;
  klass->state_set = state_set;

  /**
   * CtkSwitch:slider-width:
   *
   * The minimum width of the #CtkSwitch handle, in pixels.
   *
   * Deprecated: 3.20: Use the CSS min-width property instead.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("slider-width",
                                                             P_("Slider Width"),
                                                             P_("The minimum width of the handle"),
                                                             DEFAULT_SLIDER_WIDTH, G_MAXINT,
                                                             DEFAULT_SLIDER_WIDTH,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkSwitch:slider-height:
   *
   * The minimum height of the #CtkSwitch handle, in pixels.
   *
   * Since: 3.18
   *
   * Deprecated: 3.20: Use the CSS min-height property instead.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("slider-height",
                                                             P_("Slider Height"),
                                                             P_("The minimum height of the handle"),
                                                             DEFAULT_SLIDER_HEIGHT, G_MAXINT,
                                                             DEFAULT_SLIDER_HEIGHT,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkSwitch::activate:
   * @widget: the object which received the signal.
   *
   * The ::activate signal on CtkSwitch is an action signal and
   * emitting it causes the switch to animate.
   * Applications should never connect to this signal, but use the
   * notify::active signal.
   */
  signals[ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkSwitchClass, activate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
  widget_class->activate_signal = signals[ACTIVATE];

  /**
   * CtkSwitch::state-set:
   * @widget: the object on which the signal was emitted
   * @state: the new state of the switch
   *
   * The ::state-set signal on CtkSwitch is emitted to change the underlying
   * state. It is emitted when the user changes the switch position. The
   * default handler keeps the state in sync with the #CtkSwitch:active
   * property.
   *
   * To implement delayed state change, applications can connect to this signal,
   * initiate the change of the underlying state, and call ctk_switch_set_state()
   * when the underlying state change is complete. The signal handler should
   * return %TRUE to prevent the default handler from running.
   *
   * Visually, the underlying state is represented by the trough color of
   * the switch, while the #CtkSwitch:active property is represented by the
   * position of the switch.
   *
   * Returns: %TRUE to stop the signal emission
   *
   * Since: 3.14
   */
  signals[STATE_SET] =
    g_signal_new (I_("state-set"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkSwitchClass, state_set),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__BOOLEAN,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (signals[STATE_SET],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _ctk_marshal_BOOLEAN__BOOLEANv);

  g_object_class_override_property (gobject_class, PROP_ACTION_NAME, "action-name");
  g_object_class_override_property (gobject_class, PROP_ACTION_TARGET, "action-target");

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_SWITCH_ACCESSIBLE);
  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_TOGGLE_BUTTON);

  ctk_widget_class_set_css_name (widget_class, "switch");
}

static void
ctk_switch_init (CtkSwitch *self)
{
  CtkSwitchPrivate *priv;
  CtkGesture *gesture;
  CtkCssNode *widget_node;

  priv = self->priv = ctk_switch_get_instance_private (self);

  priv->use_action_appearance = TRUE;
  ctk_widget_set_has_window (CTK_WIDGET (self), FALSE);
  ctk_widget_set_can_focus (CTK_WIDGET (self), TRUE);

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (self));
  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     CTK_WIDGET (self),
                                                     ctk_switch_get_content_size,
                                                     ctk_switch_allocate_contents,
                                                     ctk_switch_render_trough,
                                                     NULL,
                                                     NULL);

  priv->slider_gadget = ctk_css_custom_gadget_new ("slider",
                                                   CTK_WIDGET (self),
                                                   priv->gadget,
                                                   NULL,
                                                   ctk_switch_get_slider_size,
                                                   NULL,
                                                   ctk_switch_render_slider,
                                                   NULL,
                                                   NULL);

  priv->on_gadget = ctk_icon_helper_new_named ("image", CTK_WIDGET (self));
  _ctk_icon_helper_set_icon_name (CTK_ICON_HELPER (priv->on_gadget), "switch-on-symbolic", CTK_ICON_SIZE_MENU);
  ctk_css_node_set_parent (ctk_css_gadget_get_node (priv->on_gadget), widget_node);
  ctk_css_node_set_state (ctk_css_gadget_get_node (priv->on_gadget), ctk_css_node_get_state (widget_node));

  priv->off_gadget = ctk_icon_helper_new_named ("image", CTK_WIDGET (self));
  _ctk_icon_helper_set_icon_name (CTK_ICON_HELPER (priv->off_gadget), "switch-off-symbolic", CTK_ICON_SIZE_MENU);
  ctk_css_node_set_parent (ctk_css_gadget_get_node (priv->off_gadget), widget_node);
  ctk_css_node_set_state (ctk_css_gadget_get_node (priv->off_gadget), ctk_css_node_get_state (widget_node));

  gesture = ctk_gesture_multi_press_new (CTK_WIDGET (self));
  ctk_gesture_single_set_touch_only (CTK_GESTURE_SINGLE (gesture), FALSE);
  ctk_gesture_single_set_exclusive (CTK_GESTURE_SINGLE (gesture), TRUE);
  g_signal_connect (gesture, "pressed",
                    G_CALLBACK (ctk_switch_multipress_gesture_pressed), self);
  g_signal_connect (gesture, "released",
                    G_CALLBACK (ctk_switch_multipress_gesture_released), self);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (gesture),
                                              CTK_PHASE_BUBBLE);
  priv->multipress_gesture = gesture;

  gesture = ctk_gesture_pan_new (CTK_WIDGET (self),
                                 CTK_ORIENTATION_HORIZONTAL);
  ctk_gesture_single_set_touch_only (CTK_GESTURE_SINGLE (gesture), FALSE);
  ctk_gesture_single_set_exclusive (CTK_GESTURE_SINGLE (gesture), TRUE);
  g_signal_connect (gesture, "pan",
                    G_CALLBACK (ctk_switch_pan_gesture_pan), self);
  g_signal_connect (gesture, "drag-end",
                    G_CALLBACK (ctk_switch_pan_gesture_drag_end), self);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (gesture),
                                              CTK_PHASE_BUBBLE);
  priv->pan_gesture = gesture;
}

/**
 * ctk_switch_new:
 *
 * Creates a new #CtkSwitch widget.
 *
 * Returns: the newly created #CtkSwitch instance
 *
 * Since: 3.0
 */
CtkWidget *
ctk_switch_new (void)
{
  return g_object_new (CTK_TYPE_SWITCH, NULL);
}

/**
 * ctk_switch_set_active:
 * @sw: a #CtkSwitch
 * @is_active: %TRUE if @sw should be active, and %FALSE otherwise
 *
 * Changes the state of @sw to the desired one.
 *
 * Since: 3.0
 */
void
ctk_switch_set_active (CtkSwitch *sw,
                       gboolean   is_active)
{
  CtkSwitchPrivate *priv;

  g_return_if_fail (CTK_IS_SWITCH (sw));

  ctk_switch_end_toggle_animation (sw);

  is_active = !!is_active;

  priv = sw->priv;

  if (priv->is_active != is_active)
    {
      AtkObject *accessible;
      gboolean handled;

      priv->is_active = is_active;

      if (priv->is_active)
        priv->handle_pos = 1.0;
      else
        priv->handle_pos = 0.0;

      g_signal_emit (sw, signals[STATE_SET], 0, is_active, &handled);

      g_object_notify_by_pspec (G_OBJECT (sw), switch_props[PROP_ACTIVE]);

      accessible = ctk_widget_get_accessible (CTK_WIDGET (sw));
      atk_object_notify_state_change (accessible, ATK_STATE_CHECKED, priv->is_active);

      ctk_widget_queue_allocate (CTK_WIDGET (sw));
    }
}

/**
 * ctk_switch_get_active:
 * @sw: a #CtkSwitch
 *
 * Gets whether the #CtkSwitch is in its “on” or “off” state.
 *
 * Returns: %TRUE if the #CtkSwitch is active, and %FALSE otherwise
 *
 * Since: 3.0
 */
gboolean
ctk_switch_get_active (CtkSwitch *sw)
{
  g_return_val_if_fail (CTK_IS_SWITCH (sw), FALSE);

  return sw->priv->is_active;
}

/**
 * ctk_switch_set_state:
 * @sw: a #CtkSwitch
 * @state: the new state
 *
 * Sets the underlying state of the #CtkSwitch.
 *
 * Normally, this is the same as #CtkSwitch:active, unless the switch
 * is set up for delayed state changes. This function is typically
 * called from a #CtkSwitch::state-set signal handler.
 *
 * See #CtkSwitch::state-set for details.
 *
 * Since: 3.14
 */
void
ctk_switch_set_state (CtkSwitch *sw,
                      gboolean   state)
{
  g_return_if_fail (CTK_IS_SWITCH (sw));

  state = state != FALSE;

  if (sw->priv->state == state)
    return;

  sw->priv->state = state;

  /* This will be a no-op if we're switching the state in response
   * to a UI change. We're setting active anyway, to catch 'spontaneous'
   * state changes.
   */
  ctk_switch_set_active (sw, state);

  if (state)
    ctk_widget_set_state_flags (CTK_WIDGET (sw), CTK_STATE_FLAG_CHECKED, FALSE);
  else
    ctk_widget_unset_state_flags (CTK_WIDGET (sw), CTK_STATE_FLAG_CHECKED);

  g_object_notify (G_OBJECT (sw), "state");
}

/**
 * ctk_switch_get_state:
 * @sw: a #CtkSwitch
 *
 * Gets the underlying state of the #CtkSwitch.
 *
 * Returns: the underlying state
 *
 * Since: 3.14
 */
gboolean
ctk_switch_get_state (CtkSwitch *sw)
{
  g_return_val_if_fail (CTK_IS_SWITCH (sw), FALSE);

  return sw->priv->state;
}

static void
ctk_switch_update (CtkActivatable *activatable,
                   CtkAction      *action,
                   const gchar    *property_name)
{
  if (strcmp (property_name, "visible") == 0)
    {
      if (ctk_action_is_visible (action))
        ctk_widget_show (CTK_WIDGET (activatable));
      else
        ctk_widget_hide (CTK_WIDGET (activatable));
    }
  else if (strcmp (property_name, "sensitive") == 0)
    {
      ctk_widget_set_sensitive (CTK_WIDGET (activatable), ctk_action_is_sensitive (action));
    }
  else if (strcmp (property_name, "active") == 0)
    {
      ctk_action_block_activate (action);
      ctk_switch_set_active (CTK_SWITCH (activatable), ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)));
      ctk_action_unblock_activate (action);
    }
}

static void
ctk_switch_sync_action_properties (CtkActivatable *activatable,
                                   CtkAction      *action)
{
  if (!action)
    return;

  if (ctk_action_is_visible (action))
    ctk_widget_show (CTK_WIDGET (activatable));
  else
    ctk_widget_hide (CTK_WIDGET (activatable));

  ctk_widget_set_sensitive (CTK_WIDGET (activatable), ctk_action_is_sensitive (action));

  ctk_action_block_activate (action);
  ctk_switch_set_active (CTK_SWITCH (activatable), ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)));
  ctk_action_unblock_activate (action);
}

static void
ctk_switch_activatable_interface_init (CtkActivatableIface *iface)
{
  iface->update = ctk_switch_update;
  iface->sync_action_properties = ctk_switch_sync_action_properties;
}
