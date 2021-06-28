/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Copyright 2013, 2015 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 *         Carlos Soriano <csoriano@gnome.org>
 */

#include "config.h"
#include "ctkrevealer.h"
#include <cdk/cdk.h>
#include "ctktypebuiltins.h"
#include "ctkprivate.h"
#include "ctksettingsprivate.h"
#include "ctkprogresstrackerprivate.h"
#include "ctkintl.h"

#include "fallback-c89.c"

/**
 * SECTION:ctkrevealer
 * @Short_description: Hide and show with animation
 * @Title: CtkRevealer
 * @See_also: #CtkExpander
 *
 * The CtkRevealer widget is a container which animates
 * the transition of its child from invisible to visible.
 *
 * The style of transition can be controlled with
 * ctk_revealer_set_transition_type().
 *
 * These animations respect the #CtkSettings:ctk-enable-animations
 * setting.
 *
 * # CSS nodes
 *
 * CtkRevealer has a single CSS node with name revealer.
 *
 * The CtkRevealer widget was added in CTK+ 3.10.
 */

/**
 * CtkRevealerTransitionType:
 * @CTK_REVEALER_TRANSITION_TYPE_NONE: No transition
 * @CTK_REVEALER_TRANSITION_TYPE_CROSSFADE: Fade in
 * @CTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT: Slide in from the left
 * @CTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT: Slide in from the right
 * @CTK_REVEALER_TRANSITION_TYPE_SLIDE_UP: Slide in from the bottom
 * @CTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN: Slide in from the top
 *
 * These enumeration values describe the possible transitions
 * when the child of a #CtkRevealer widget is shown or hidden.
 */

enum  {
  PROP_0,
  PROP_TRANSITION_TYPE,
  PROP_TRANSITION_DURATION,
  PROP_REVEAL_CHILD,
  PROP_CHILD_REVEALED,
  LAST_PROP
};

typedef struct {
  CtkRevealerTransitionType transition_type;
  guint transition_duration;

  CdkWindow* bin_window;
  CdkWindow* view_window;

  gdouble current_pos;
  gdouble source_pos;
  gdouble target_pos;

  guint tick_id;
  CtkProgressTracker tracker;
} CtkRevealerPrivate;

static GParamSpec *props[LAST_PROP] = { NULL, };

static void     ctk_revealer_real_realize                        (CtkWidget     *widget);
static void     ctk_revealer_real_unrealize                      (CtkWidget     *widget);
static void     ctk_revealer_real_add                            (CtkContainer  *widget,
                                                                  CtkWidget     *child);
static void     ctk_revealer_real_size_allocate                  (CtkWidget     *widget,
                                                                  CtkAllocation *allocation);
static void     ctk_revealer_real_map                            (CtkWidget     *widget);
static void     ctk_revealer_real_unmap                          (CtkWidget     *widget);
static gboolean ctk_revealer_real_draw                           (CtkWidget     *widget,
                                                                  cairo_t       *cr);
static void     ctk_revealer_real_get_preferred_height           (CtkWidget     *widget,
                                                                  gint          *minimum_height,
                                                                  gint          *natural_height);
static void     ctk_revealer_real_get_preferred_height_for_width (CtkWidget     *widget,
                                                                  gint           width,
                                                                  gint          *minimum_height,
                                                                  gint          *natural_height);
static void     ctk_revealer_real_get_preferred_width            (CtkWidget     *widget,
                                                                  gint          *minimum_width,
                                                                  gint          *natural_width);
static void     ctk_revealer_real_get_preferred_width_for_height (CtkWidget     *widget,
                                                                  gint           height,
                                                                  gint          *minimum_width,
                                                                  gint          *natural_width);

G_DEFINE_TYPE_WITH_PRIVATE (CtkRevealer, ctk_revealer, CTK_TYPE_BIN)

static void
ctk_revealer_get_padding (CtkRevealer *revealer,
                          CtkBorder   *padding)
{
  CtkWidget *widget = CTK_WIDGET (revealer);
  CtkStyleContext *context;
  CtkStateFlags state;

  context = ctk_widget_get_style_context (widget);
  state = ctk_style_context_get_state (context);

  ctk_style_context_get_padding (context, state, padding);
}

static void
ctk_revealer_init (CtkRevealer *revealer)
{
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);

  priv->transition_type = CTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN;
  priv->transition_duration = 250;
  priv->current_pos = 0.0;
  priv->target_pos = 0.0;

  ctk_widget_set_has_window ((CtkWidget*) revealer, TRUE);
}

static void
ctk_revealer_finalize (GObject *obj)
{
  CtkRevealer *revealer = CTK_REVEALER (obj);
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);

  if (priv->tick_id != 0)
    ctk_widget_remove_tick_callback (CTK_WIDGET (revealer), priv->tick_id);
  priv->tick_id = 0;

  G_OBJECT_CLASS (ctk_revealer_parent_class)->finalize (obj);
}

static void
ctk_revealer_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  CtkRevealer *revealer = CTK_REVEALER (object);

  switch (property_id)
   {
    case PROP_TRANSITION_TYPE:
      g_value_set_enum (value, ctk_revealer_get_transition_type (revealer));
      break;
    case PROP_TRANSITION_DURATION:
      g_value_set_uint (value, ctk_revealer_get_transition_duration (revealer));
      break;
    case PROP_REVEAL_CHILD:
      g_value_set_boolean (value, ctk_revealer_get_reveal_child (revealer));
      break;
    case PROP_CHILD_REVEALED:
      g_value_set_boolean (value, ctk_revealer_get_child_revealed (revealer));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ctk_revealer_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  CtkRevealer *revealer = CTK_REVEALER (object);

  switch (property_id)
    {
    case PROP_TRANSITION_TYPE:
      ctk_revealer_set_transition_type (revealer, g_value_get_enum (value));
      break;
    case PROP_TRANSITION_DURATION:
      ctk_revealer_set_transition_duration (revealer, g_value_get_uint (value));
      break;
    case PROP_REVEAL_CHILD:
      ctk_revealer_set_reveal_child (revealer, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ctk_revealer_class_init (CtkRevealerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS(klass);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);

  object_class->get_property = ctk_revealer_get_property;
  object_class->set_property = ctk_revealer_set_property;
  object_class->finalize = ctk_revealer_finalize;

  widget_class->realize = ctk_revealer_real_realize;
  widget_class->unrealize = ctk_revealer_real_unrealize;
  widget_class->size_allocate = ctk_revealer_real_size_allocate;
  widget_class->map = ctk_revealer_real_map;
  widget_class->unmap = ctk_revealer_real_unmap;
  widget_class->draw = ctk_revealer_real_draw;
  widget_class->get_preferred_height = ctk_revealer_real_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_revealer_real_get_preferred_height_for_width;
  widget_class->get_preferred_width = ctk_revealer_real_get_preferred_width;
  widget_class->get_preferred_width_for_height = ctk_revealer_real_get_preferred_width_for_height;

  container_class->add = ctk_revealer_real_add;

  props[PROP_TRANSITION_TYPE] =
    g_param_spec_enum ("transition-type",
                       P_("Transition type"),
                       P_("The type of animation used to transition"),
                       CTK_TYPE_REVEALER_TRANSITION_TYPE,
                       CTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN,
                       CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_TRANSITION_DURATION] =
    g_param_spec_uint ("transition-duration",
                       P_("Transition duration"),
                       P_("The animation duration, in milliseconds"),
                       0, G_MAXUINT, 250,
                       CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_REVEAL_CHILD] =
    g_param_spec_boolean ("reveal-child",
                          P_("Reveal Child"),
                          P_("Whether the container should reveal the child"),
                          FALSE,
                          CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_CHILD_REVEALED] =
    g_param_spec_boolean ("child-revealed",
                          P_("Child Revealed"),
                          P_("Whether the child is revealed and the animation target reached"),
                          FALSE,
                          G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  ctk_widget_class_set_css_name (widget_class, "revealer");
}

/**
 * ctk_revealer_new:
 *
 * Creates a new #CtkRevealer.
 *
 * Returns: a newly created #CtkRevealer
 *
 * Since: 3.10
 */
CtkWidget *
ctk_revealer_new (void)
{
  return g_object_new (CTK_TYPE_REVEALER, NULL);
}

static CtkRevealerTransitionType
effective_transition (CtkRevealer *revealer)
{
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);

  if (ctk_widget_get_direction (CTK_WIDGET (revealer)) == CTK_TEXT_DIR_RTL)
    {
      if (priv->transition_type == CTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT)
        return CTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT;
      else if (priv->transition_type == CTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
        return CTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT;
    }

  return priv->transition_type;
}

static void
ctk_revealer_get_child_allocation (CtkRevealer   *revealer,
                                   CtkAllocation *allocation,
                                   CtkAllocation *child_allocation)
{
  CtkWidget *child;
  CtkRevealerTransitionType transition;
  CtkBorder padding;
  gint vertical_padding, horizontal_padding;

  g_return_if_fail (revealer != NULL);
  g_return_if_fail (allocation != NULL);

  /* See explanation on ctk_revealer_real_size_allocate */
  ctk_revealer_get_padding (revealer, &padding);
  vertical_padding = padding.top + padding.bottom;
  horizontal_padding = padding.left + padding.right;

  child_allocation->x = 0;
  child_allocation->y = 0;
  child_allocation->width = 0;
  child_allocation->height = 0;

  child = ctk_bin_get_child (CTK_BIN (revealer));
  if (child != NULL && ctk_widget_get_visible (child))
    {
      transition = effective_transition (revealer);
      if (transition == CTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT ||
          transition == CTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
        ctk_widget_get_preferred_width_for_height (child, MAX (0, allocation->height - vertical_padding), NULL,
                                                   &child_allocation->width);
      else
        ctk_widget_get_preferred_height_for_width (child, MAX (0, allocation->width - horizontal_padding), NULL,
                                                   &child_allocation->height);
    }

  child_allocation->width = MAX (child_allocation->width, allocation->width - horizontal_padding);
  child_allocation->height = MAX (child_allocation->height, allocation->height - vertical_padding);
}

static void
ctk_revealer_real_realize (CtkWidget *widget)
{
  CtkRevealer *revealer = CTK_REVEALER (widget);
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);
  CtkAllocation allocation;
  CdkWindowAttr attributes = { 0 };
  CdkWindowAttributesType attributes_mask;
  CtkAllocation child_allocation;
  CtkWidget *child;
  CtkRevealerTransitionType transition;
  CtkBorder padding;

  ctk_widget_set_realized (widget, TRUE);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.wclass = CDK_INPUT_OUTPUT;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.event_mask =
    ctk_widget_get_events (widget);
  attributes_mask = (CDK_WA_X | CDK_WA_Y) | CDK_WA_VISUAL;

  priv->view_window =
    cdk_window_new (ctk_widget_get_parent_window ((CtkWidget*) revealer),
                    &attributes, attributes_mask);
  ctk_widget_set_window (widget, priv->view_window);
  ctk_widget_register_window (widget, priv->view_window);

  ctk_revealer_get_child_allocation (revealer, &allocation, &child_allocation);

  ctk_revealer_get_padding (revealer, &padding);
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = child_allocation.width;
  attributes.height = child_allocation.height;

  /* See explanation on ctk_revealer_real_size_allocate */
  transition = effective_transition (revealer);
  if (transition == CTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN)
    {
      attributes.y = allocation.height - child_allocation.height - padding.bottom;
      attributes.x = padding.left;
    }
  else if (transition == CTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
    {
      attributes.y = padding.top;
      attributes.x = allocation.width - child_allocation.width - padding.right;
    }
 else
   {
     attributes.y = padding.top;
     attributes.x = padding.left;
   }

  priv->bin_window =
    cdk_window_new (priv->view_window, &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->bin_window);

  child = ctk_bin_get_child (CTK_BIN (revealer));
  if (child != NULL)
    ctk_widget_set_parent_window (child, priv->bin_window);

  cdk_window_show (priv->bin_window);
}

static void
ctk_revealer_real_unrealize (CtkWidget *widget)
{
  CtkRevealer *revealer = CTK_REVEALER (widget);
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);

  ctk_widget_unregister_window (widget, priv->bin_window);
  cdk_window_destroy (priv->bin_window);
  priv->view_window = NULL;

  CTK_WIDGET_CLASS (ctk_revealer_parent_class)->unrealize (widget);
}

static void
ctk_revealer_real_add (CtkContainer *container,
                       CtkWidget    *child)
{
  CtkRevealer *revealer = CTK_REVEALER (container);
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);

  g_return_if_fail (child != NULL);

  ctk_widget_set_parent_window (child, priv->bin_window);
  ctk_widget_set_child_visible (child, priv->current_pos != 0.0);

  CTK_CONTAINER_CLASS (ctk_revealer_parent_class)->add (container, child);
}

static void
ctk_revealer_real_size_allocate (CtkWidget     *widget,
                                 CtkAllocation *allocation)
{
  CtkRevealer *revealer = CTK_REVEALER (widget);
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);
  CtkAllocation child_allocation;
  CtkWidget *child;
  gboolean window_visible;
  int bin_x, bin_y;
  CtkRevealerTransitionType transition;
  CtkBorder padding;

  g_return_if_fail (allocation != NULL);

  ctk_widget_set_allocation (widget, allocation);
  ctk_revealer_get_child_allocation (revealer, allocation, &child_allocation);

  child = ctk_bin_get_child (CTK_BIN (revealer));
  if (child != NULL && ctk_widget_get_visible (child))
    ctk_widget_size_allocate (child, &child_allocation);

  if (ctk_widget_get_realized (widget))
    {
      if (ctk_widget_get_mapped (widget))
        {
          window_visible = allocation->width > 0 && allocation->height > 0;

          if (!window_visible && cdk_window_is_visible (priv->view_window))
            cdk_window_hide (priv->view_window);

          if (window_visible && !cdk_window_is_visible (priv->view_window))
            cdk_window_show (priv->view_window);
        }

      /* The view window will follow the revealer allocation, which is modified
       * along the animation */
      cdk_window_move_resize (priv->view_window,
                              allocation->x, allocation->y,
                              allocation->width, allocation->height);

      ctk_revealer_get_padding (revealer, &padding);
      bin_x = 0;
      bin_y = 0;

      transition = effective_transition (revealer);
      /* The child allocation is fixed (it is not modified by the animation),
       * and it's origin is relative to the bin_window.
       * The bin_window has the same allocation as the child, and then the bin_window
       * deals with the relative positioning with respect to the revealer taking
       * into account the paddings of the revealer.
       *
       * For most of transitions, the bin_window moves along with the revealer,
       * as its allocation changes.
       * However for CTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN
       * we need to first move the bin_window upwards and then slide it down in
       * the revealer.
       * Otherwise the child would appear as static and the revealer will allocate
       * following the animation, clipping the child.
       * To calculate the correct y position for this case:
       * allocation->height - child_allocation.height is the relative position
       * towards the revealer taking into account the animation progress with
       * both vertical paddings added, therefore we need to substract the part
       * that we don't want to take into account for the y position, which
       * in this case is the bottom padding.
       *
       * The same special treatment is needed for CTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT.
       */
      if (transition == CTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN)
        {
          bin_y = allocation->height - child_allocation.height - padding.bottom;
          bin_x = padding.left;
        }
      else if (transition == CTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
        {
          bin_y = padding.top;
          bin_x = allocation->width - child_allocation.width - padding.right;
        }
     else
       {
         bin_x = padding.left;
         bin_y = padding.top;
       }

      cdk_window_move_resize (priv->bin_window,
                              bin_x, bin_y,
                              child_allocation.width, child_allocation.height);
    }
}

static void
ctk_revealer_set_position (CtkRevealer *revealer,
                           gdouble      pos)
{
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);
  gboolean new_visible;
  CtkWidget *child;
  CtkRevealerTransitionType transition;

  priv->current_pos = pos;

  /* We check target_pos here too, because we want to ensure we set
   * child_visible immediately when starting a reveal operation
   * otherwise the child widgets will not be properly realized
   * after the reveal returns.
   */
  new_visible = priv->current_pos != 0.0 || priv->target_pos != 0.0;

  child = ctk_bin_get_child (CTK_BIN (revealer));
  if (child != NULL &&
      new_visible != ctk_widget_get_child_visible (child))
    ctk_widget_set_child_visible (child, new_visible);

  transition = effective_transition (revealer);
  if (transition == CTK_REVEALER_TRANSITION_TYPE_CROSSFADE)
    {
      ctk_widget_set_opacity (CTK_WIDGET (revealer), priv->current_pos);
      ctk_widget_queue_draw (CTK_WIDGET (revealer));
    }
  else
    {
      ctk_widget_queue_resize (CTK_WIDGET (revealer));
    }

  if (priv->current_pos == priv->target_pos)
    g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_CHILD_REVEALED]);
}

static gboolean
ctk_revealer_animate_cb (CtkWidget     *widget,
                         CdkFrameClock *frame_clock,
                         gpointer       user_data)
{
  CtkRevealer *revealer = CTK_REVEALER (widget);
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);
  gdouble ease;

  ctk_progress_tracker_advance_frame (&priv->tracker,
                                      cdk_frame_clock_get_frame_time (frame_clock));
  ease = ctk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE);
  ctk_revealer_set_position (revealer,
                             priv->source_pos + (ease * (priv->target_pos - priv->source_pos)));

  if (ctk_progress_tracker_get_state (&priv->tracker) == CTK_PROGRESS_STATE_AFTER)
    {
      priv->tick_id = 0;
      return FALSE;
    }

  return TRUE;
}

static void
ctk_revealer_start_animation (CtkRevealer *revealer,
                              gdouble      target)
{
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);
  CtkWidget *widget = CTK_WIDGET (revealer);
  CtkRevealerTransitionType transition;

  if (priv->target_pos == target)
    return;

  priv->target_pos = target;
  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_REVEAL_CHILD]);

  transition = effective_transition (revealer);
  if (ctk_widget_get_mapped (widget) &&
      priv->transition_duration != 0 &&
      transition != CTK_REVEALER_TRANSITION_TYPE_NONE &&
      ctk_settings_get_enable_animations (ctk_widget_get_settings (widget)))
    {
      priv->source_pos = priv->current_pos;
      if (priv->tick_id == 0)
        priv->tick_id =
          ctk_widget_add_tick_callback (widget, ctk_revealer_animate_cb, revealer, NULL);
      ctk_progress_tracker_start (&priv->tracker,
                                  priv->transition_duration * 1000,
                                  0,
                                  1.0);
    }
  else
    {
      ctk_revealer_set_position (revealer, target);
    }
}

static void
ctk_revealer_stop_animation (CtkRevealer *revealer)
{
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);
  if (priv->current_pos != priv->target_pos)
    ctk_revealer_set_position (revealer, priv->target_pos);
  if (priv->tick_id != 0)
    {
      ctk_widget_remove_tick_callback (CTK_WIDGET (revealer), priv->tick_id);
      priv->tick_id = 0;
    }
}

static void
ctk_revealer_real_map (CtkWidget *widget)
{
  CtkRevealer *revealer = CTK_REVEALER (widget);
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);
  CtkAllocation allocation;

  if (!ctk_widget_get_mapped (widget))
    {
      ctk_widget_get_allocation (widget, &allocation);

      if (allocation.width > 0 && allocation.height > 0)
        cdk_window_show (priv->view_window);
    }

  CTK_WIDGET_CLASS (ctk_revealer_parent_class)->map (widget);
}

static void
ctk_revealer_real_unmap (CtkWidget *widget)
{
  CtkRevealer *revealer = CTK_REVEALER (widget);

  CTK_WIDGET_CLASS (ctk_revealer_parent_class)->unmap (widget);

  ctk_revealer_stop_animation (revealer);
}

static gboolean
ctk_revealer_real_draw (CtkWidget *widget,
                        cairo_t   *cr)
{
  CtkRevealer *revealer = CTK_REVEALER (widget);
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);

  if (ctk_cairo_should_draw_window (cr, priv->bin_window))
    CTK_WIDGET_CLASS (ctk_revealer_parent_class)->draw (widget, cr);

  return CDK_EVENT_PROPAGATE;
}

/**
 * ctk_revealer_set_reveal_child:
 * @revealer: a #CtkRevealer
 * @reveal_child: %TRUE to reveal the child
 *
 * Tells the #CtkRevealer to reveal or conceal its child.
 *
 * The transition will be animated with the current
 * transition type of @revealer.
 *
 * Since: 3.10
 */
void
ctk_revealer_set_reveal_child (CtkRevealer *revealer,
                               gboolean     reveal_child)
{
  g_return_if_fail (CTK_IS_REVEALER (revealer));

  if (reveal_child)
    ctk_revealer_start_animation (revealer, 1.0);
  else
    ctk_revealer_start_animation (revealer, 0.0);
}

/**
 * ctk_revealer_get_reveal_child:
 * @revealer: a #CtkRevealer
 *
 * Returns whether the child is currently
 * revealed. See ctk_revealer_set_reveal_child().
 *
 * This function returns %TRUE as soon as the transition
 * is to the revealed state is started. To learn whether
 * the child is fully revealed (ie the transition is completed),
 * use ctk_revealer_get_child_revealed().
 *
 * Returns: %TRUE if the child is revealed.
 *
 * Since: 3.10
 */
gboolean
ctk_revealer_get_reveal_child (CtkRevealer *revealer)
{
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);

  g_return_val_if_fail (CTK_IS_REVEALER (revealer), FALSE);

  return priv->target_pos != 0.0;
}

/**
 * ctk_revealer_get_child_revealed:
 * @revealer: a #CtkRevealer
 *
 * Returns whether the child is fully revealed, in other words whether
 * the transition to the revealed state is completed.
 *
 * Returns: %TRUE if the child is fully revealed
 *
 * Since: 3.10
 */
gboolean
ctk_revealer_get_child_revealed (CtkRevealer *revealer)
{
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);
  gboolean animation_finished = (priv->target_pos == priv->current_pos);
  gboolean reveal_child = ctk_revealer_get_reveal_child (revealer);

  if (animation_finished)
    return reveal_child;
  else
    return !reveal_child;
}

/* These all report only the natural size, ignoring the minimal size,
 * because its not really possible to allocate the right size during
 * animation if the child size can change (without the child
 * re-arranging itself during the animation).
 */

static void
set_height_with_paddings (CtkRevealer *revealer,
                          gint         preferred_minimum_height,
                          gint         preferred_natural_height,
                          gint        *minimum_height_out,
                          gint        *natural_height_out)
{
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);
  gint minimum_height;
  gint natural_height;
  CtkRevealerTransitionType transition;
  CtkBorder padding;
  gint vertical_padding;

  ctk_revealer_get_padding (revealer, &padding);
  vertical_padding = padding.top + padding.bottom;
  minimum_height = preferred_minimum_height + vertical_padding;
  natural_height = preferred_natural_height + vertical_padding;

  transition = effective_transition (revealer);
  if (transition == CTK_REVEALER_TRANSITION_TYPE_NONE ||
      transition == CTK_REVEALER_TRANSITION_TYPE_SLIDE_UP ||
      transition == CTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN)
    {
      /* Padding are included in the animation */
      minimum_height = round (minimum_height * priv->current_pos);
      natural_height = round (natural_height * priv->current_pos);
    }

  *minimum_height_out = MIN (minimum_height, natural_height);
  *natural_height_out = natural_height;
}

static void
ctk_revealer_real_get_preferred_height (CtkWidget *widget,
                                        gint      *minimum_height_out,
                                        gint      *natural_height_out)
{
  CtkRevealer *revealer = CTK_REVEALER (widget);
  gint minimum_height;
  gint natural_height;

  CTK_WIDGET_CLASS (ctk_revealer_parent_class)->get_preferred_height (widget, &minimum_height, &natural_height);

  set_height_with_paddings (revealer, minimum_height, natural_height,
                            minimum_height_out, natural_height_out);
}

static void
ctk_revealer_real_get_preferred_height_for_width (CtkWidget *widget,
                                                  gint       width,
                                                  gint      *minimum_height_out,
                                                  gint      *natural_height_out)
{
  CtkRevealer *revealer = CTK_REVEALER (widget);
  gint minimum_height;
  gint natural_height;

  CTK_WIDGET_CLASS (ctk_revealer_parent_class)->get_preferred_height_for_width (widget, width, &minimum_height, &natural_height);

  set_height_with_paddings (revealer, minimum_height, natural_height,
                            minimum_height_out, natural_height_out);
}

static void
set_width_with_paddings (CtkRevealer *revealer,
                         gint         preferred_minimum_width,
                         gint         preferred_natural_width,
                         gint        *minimum_width_out,
                         gint        *natural_width_out)
{
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);
  gint minimum_width;
  gint natural_width;
  CtkRevealerTransitionType transition;
  CtkBorder padding;
  gint horizontal_padding;

  ctk_revealer_get_padding (revealer, &padding);
  horizontal_padding = padding.left + padding.right;
  minimum_width = preferred_minimum_width + horizontal_padding;
  natural_width = preferred_natural_width + horizontal_padding;

  transition = effective_transition (revealer);
  if (transition == CTK_REVEALER_TRANSITION_TYPE_NONE ||
      transition == CTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT ||
      transition == CTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
    {
      /* Paddings are included in the animation */
      minimum_width = round (minimum_width * priv->current_pos);
      natural_width = round (natural_width * priv->current_pos);
    }

  *minimum_width_out = MIN (minimum_width, natural_width);
  *natural_width_out = natural_width;
}

static void
ctk_revealer_real_get_preferred_width (CtkWidget *widget,
                                       gint      *minimum_width_out,
                                       gint      *natural_width_out)
{
  CtkRevealer *revealer = CTK_REVEALER (widget);
  gint minimum_width;
  gint natural_width;

  CTK_WIDGET_CLASS (ctk_revealer_parent_class)->get_preferred_width (widget, &minimum_width, &natural_width);
  set_width_with_paddings (revealer, minimum_width, natural_width,
                           minimum_width_out, natural_width_out);
}

static void
ctk_revealer_real_get_preferred_width_for_height (CtkWidget *widget,
                                                  gint       height,
                                                  gint      *minimum_width_out,
                                                  gint      *natural_width_out)
{
  CtkRevealer *revealer = CTK_REVEALER (widget);
  gint minimum_width;
  gint natural_width;

  CTK_WIDGET_CLASS (ctk_revealer_parent_class)->get_preferred_width_for_height (widget, height, &minimum_width, &natural_width);

  set_width_with_paddings (revealer, minimum_width, natural_width,
                           minimum_width_out, natural_width_out);
}

/**
 * ctk_revealer_get_transition_duration:
 * @revealer: a #CtkRevealer
 *
 * Returns the amount of time (in milliseconds) that
 * transitions will take.
 *
 * Returns: the transition duration
 *
 * Since: 3.10
 */
guint
ctk_revealer_get_transition_duration (CtkRevealer *revealer)
{
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);

  g_return_val_if_fail (CTK_IS_REVEALER (revealer), 0);

  return priv->transition_duration;
}

/**
 * ctk_revealer_set_transition_duration:
 * @revealer: a #CtkRevealer
 * @duration: the new duration, in milliseconds
 *
 * Sets the duration that transitions will take.
 *
 * Since: 3.10
 */
void
ctk_revealer_set_transition_duration (CtkRevealer *revealer,
                                      guint        value)
{
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);

  g_return_if_fail (CTK_IS_REVEALER (revealer));

  if (priv->transition_duration == value)
    return;

  priv->transition_duration = value;
  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_TRANSITION_DURATION]);
}

/**
 * ctk_revealer_get_transition_type:
 * @revealer: a #CtkRevealer
 *
 * Gets the type of animation that will be used
 * for transitions in @revealer.
 *
 * Returns: the current transition type of @revealer
 *
 * Since: 3.10
 */
CtkRevealerTransitionType
ctk_revealer_get_transition_type (CtkRevealer *revealer)
{
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);

  g_return_val_if_fail (CTK_IS_REVEALER (revealer), CTK_REVEALER_TRANSITION_TYPE_NONE);

  return priv->transition_type;
}

/**
 * ctk_revealer_set_transition_type:
 * @revealer: a #CtkRevealer
 * @transition: the new transition type
 *
 * Sets the type of animation that will be used for
 * transitions in @revealer. Available types include
 * various kinds of fades and slides.
 *
 * Since: 3.10
 */
void
ctk_revealer_set_transition_type (CtkRevealer               *revealer,
                                  CtkRevealerTransitionType  transition)
{
  CtkRevealerPrivate *priv = ctk_revealer_get_instance_private (revealer);

  g_return_if_fail (CTK_IS_REVEALER (revealer));

  if (priv->transition_type == transition)
    return;

  priv->transition_type = transition;
  ctk_widget_queue_resize (CTK_WIDGET (revealer));
  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_TRANSITION_TYPE]);
}
