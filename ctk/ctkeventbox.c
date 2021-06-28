/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 */

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include "ctkeventbox.h"

#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkrender.h"
#include "ctksizerequest.h"


/**
 * SECTION:ctkeventbox
 * @Short_description: A widget used to catch events for widgets which
 *     do not have their own window
 * @Title: CtkEventBox
 *
 * The #CtkEventBox widget is a subclass of #CtkBin which also has its
 * own window. It is useful since it allows you to catch events for widgets
 * which do not have their own window.
 */

struct _CtkEventBoxPrivate
{
  gboolean above_child;
  CdkWindow *event_window;
};

enum {
  PROP_0,
  PROP_VISIBLE_WINDOW,
  PROP_ABOVE_CHILD
};

static void     ctk_event_box_realize       (CtkWidget        *widget);
static void     ctk_event_box_unrealize     (CtkWidget        *widget);
static void     ctk_event_box_map           (CtkWidget        *widget);
static void     ctk_event_box_unmap         (CtkWidget        *widget);
static void     ctk_event_box_get_preferred_width  (CtkWidget *widget,
                                                    gint      *minimum,
                                                    gint      *natural);
static void     ctk_event_box_get_preferred_height (CtkWidget *widget,
                                                    gint      *minimum,
                                                    gint      *natural);
static void     ctk_event_box_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
									   gint       width,
									   gint      *minimum,
									   gint      *natural,
									   gint      *minimum_baseline,
									   gint      *natural_baseline);
static void     ctk_event_box_size_allocate (CtkWidget        *widget,
                                             CtkAllocation    *allocation);
static gboolean ctk_event_box_draw          (CtkWidget        *widget,
                                             cairo_t          *cr);
static void     ctk_event_box_set_property  (GObject          *object,
                                             guint             prop_id,
                                             const GValue     *value,
                                             GParamSpec       *pspec);
static void     ctk_event_box_get_property  (GObject          *object,
                                             guint             prop_id,
                                             GValue           *value,
                                             GParamSpec       *pspec);

G_DEFINE_TYPE_WITH_PRIVATE (CtkEventBox, ctk_event_box, CTK_TYPE_BIN)

static void
ctk_event_box_class_init (CtkEventBoxClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (class);

  gobject_class->set_property = ctk_event_box_set_property;
  gobject_class->get_property = ctk_event_box_get_property;

  widget_class->realize = ctk_event_box_realize;
  widget_class->unrealize = ctk_event_box_unrealize;
  widget_class->map = ctk_event_box_map;
  widget_class->unmap = ctk_event_box_unmap;
  widget_class->get_preferred_width = ctk_event_box_get_preferred_width;
  widget_class->get_preferred_height = ctk_event_box_get_preferred_height;
  widget_class->get_preferred_height_and_baseline_for_width = ctk_event_box_get_preferred_height_and_baseline_for_width;
  widget_class->size_allocate = ctk_event_box_size_allocate;
  widget_class->draw = ctk_event_box_draw;

  ctk_container_class_handle_border_width (container_class);

  g_object_class_install_property (gobject_class,
                                   PROP_VISIBLE_WINDOW,
                                   g_param_spec_boolean ("visible-window",
                                                        P_("Visible Window"),
                                                        P_("Whether the event box is visible, as opposed to invisible and only used to trap events."),
                                                        TRUE,
                                                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  g_object_class_install_property (gobject_class,
                                   PROP_ABOVE_CHILD,
                                   g_param_spec_boolean ("above-child",
                                                        P_("Above child"),
                                                        P_("Whether the event-trapping window of the eventbox is above the window of the child widget as opposed to below it."),
                                                        FALSE,
                                                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
}

static void
ctk_event_box_init (CtkEventBox *event_box)
{
  ctk_widget_set_has_window (CTK_WIDGET (event_box), TRUE);

  event_box->priv = ctk_event_box_get_instance_private (event_box);
  event_box->priv->above_child = FALSE;
}

/**
 * ctk_event_box_new:
 *
 * Creates a new #CtkEventBox.
 *
 * Returns: a new #CtkEventBox
 */
CtkWidget*
ctk_event_box_new (void)
{
  return g_object_new (CTK_TYPE_EVENT_BOX, NULL);
}

static void
ctk_event_box_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  CtkEventBox *event_box;

  event_box = CTK_EVENT_BOX (object);

  switch (prop_id)
    {
    case PROP_VISIBLE_WINDOW:
      ctk_event_box_set_visible_window (event_box, g_value_get_boolean (value));
      break;

    case PROP_ABOVE_CHILD:
      ctk_event_box_set_above_child (event_box, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_event_box_get_property (GObject     *object,
                            guint        prop_id,
                            GValue      *value,
                            GParamSpec  *pspec)
{
  CtkEventBox *event_box;

  event_box = CTK_EVENT_BOX (object);

  switch (prop_id)
    {
    case PROP_VISIBLE_WINDOW:
      g_value_set_boolean (value, ctk_event_box_get_visible_window (event_box));
      break;

    case PROP_ABOVE_CHILD:
      g_value_set_boolean (value, ctk_event_box_get_above_child (event_box));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * ctk_event_box_get_visible_window:
 * @event_box: a #CtkEventBox
 *
 * Returns whether the event box has a visible window.
 * See ctk_event_box_set_visible_window() for details.
 *
 * Returns: %TRUE if the event box window is visible
 *
 * Since: 2.4
 */
gboolean
ctk_event_box_get_visible_window (CtkEventBox *event_box)
{
  g_return_val_if_fail (CTK_IS_EVENT_BOX (event_box), FALSE);

  return ctk_widget_get_has_window (CTK_WIDGET (event_box));
}

/**
 * ctk_event_box_set_visible_window:
 * @event_box: a #CtkEventBox
 * @visible_window: %TRUE to make the event box have a visible window
 *
 * Set whether the event box uses a visible or invisible child
 * window. The default is to use visible windows.
 *
 * In an invisible window event box, the window that the
 * event box creates is a %GDK_INPUT_ONLY window, which
 * means that it is invisible and only serves to receive
 * events.
 *
 * A visible window event box creates a visible (%GDK_INPUT_OUTPUT)
 * window that acts as the parent window for all the widgets
 * contained in the event box.
 *
 * You should generally make your event box invisible if
 * you just want to trap events. Creating a visible window
 * may cause artifacts that are visible to the user, especially
 * if the user is using a theme with gradients or pixmaps.
 *
 * The main reason to create a non input-only event box is if
 * you want to set the background to a different color or
 * draw on it.
 *
 * There is one unexpected issue for an invisible event box that has its
 * window below the child. (See ctk_event_box_set_above_child().)
 * Since the input-only window is not an ancestor window of any windows
 * that descendent widgets of the event box create, events on these
 * windows aren’t propagated up by the windowing system, but only by CTK+.
 * The practical effect of this is if an event isn’t in the event
 * mask for the descendant window (see ctk_widget_add_events()),
 * it won’t be received by the event box.
 * 
 * This problem doesn’t occur for visible event boxes, because in
 * that case, the event box window is actually the ancestor of the
 * descendant windows, not just at the same place on the screen.
 *
 * Since: 2.4
 */
void
ctk_event_box_set_visible_window (CtkEventBox *event_box,
                                  gboolean     visible_window)
{
  CtkWidget *widget;

  g_return_if_fail (CTK_IS_EVENT_BOX (event_box));

  widget = CTK_WIDGET (event_box);

  visible_window = visible_window != FALSE;

  if (visible_window != ctk_widget_get_has_window (widget))
    {
      if (ctk_widget_get_realized (widget))
        {
          gboolean visible = ctk_widget_get_visible (widget);

          if (visible)
            ctk_widget_hide (widget);

          ctk_widget_unrealize (widget);

          ctk_widget_set_has_window (widget, visible_window);

          ctk_widget_realize (widget);

          if (visible)
            ctk_widget_show (widget);
        }
      else
        {
          ctk_widget_set_has_window (widget, visible_window);
        }

      if (ctk_widget_get_visible (widget))
        ctk_widget_queue_resize (widget);

      g_object_notify (G_OBJECT (event_box), "visible-window");
    }
}

/**
 * ctk_event_box_get_above_child:
 * @event_box: a #CtkEventBox
 *
 * Returns whether the event box window is above or below the
 * windows of its child. See ctk_event_box_set_above_child()
 * for details.
 *
 * Returns: %TRUE if the event box window is above the
 *     window of its child
 *
 * Since: 2.4
 */
gboolean
ctk_event_box_get_above_child (CtkEventBox *event_box)
{
  CtkEventBoxPrivate *priv = event_box->priv;

  g_return_val_if_fail (CTK_IS_EVENT_BOX (event_box), FALSE);

  return priv->above_child;
}

/**
 * ctk_event_box_set_above_child:
 * @event_box: a #CtkEventBox
 * @above_child: %TRUE if the event box window is above its child
 *
 * Set whether the event box window is positioned above the windows
 * of its child, as opposed to below it. If the window is above, all
 * events inside the event box will go to the event box. If the window
 * is below, events in windows of child widgets will first got to that
 * widget, and then to its parents.
 *
 * The default is to keep the window below the child.
 *
 * Since: 2.4
 */
void
ctk_event_box_set_above_child (CtkEventBox *event_box,
                               gboolean     above_child)
{
  CtkEventBoxPrivate *priv = event_box->priv;
  CtkWidget *widget;

  g_return_if_fail (CTK_IS_EVENT_BOX (event_box));

  widget = CTK_WIDGET (event_box);

  above_child = above_child != FALSE;

  if (priv->above_child != above_child)
    {
      priv->above_child = above_child;

      if (ctk_widget_get_realized (widget))
        {
          if (!ctk_widget_get_has_window (widget))
            {
              if (above_child)
                cdk_window_raise (priv->event_window);
              else
                cdk_window_lower (priv->event_window);
            }
          else
            {
              gboolean visible = ctk_widget_get_visible (widget);

              if (visible)
                ctk_widget_hide (widget);

              ctk_widget_unrealize (widget);
              ctk_widget_realize (widget);

              if (visible)
                ctk_widget_show (widget);
            }
        }

      if (ctk_widget_get_visible (widget))
        ctk_widget_queue_resize (widget);

      g_object_notify (G_OBJECT (event_box), "above-child");
    }
}

static void
ctk_event_box_realize (CtkWidget *widget)
{
  CtkEventBoxPrivate *priv;
  CtkAllocation allocation;
  CdkWindow *window;
  CdkWindowAttr attributes;
  gint attributes_mask;
  gboolean visible_window;

  ctk_widget_get_allocation (widget, &allocation);

  ctk_widget_set_realized (widget, TRUE);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = ctk_widget_get_events (widget)
                        | GDK_BUTTON_MOTION_MASK
                        | GDK_BUTTON_PRESS_MASK
                        | GDK_BUTTON_RELEASE_MASK
                        | GDK_EXPOSURE_MASK
                        | GDK_ENTER_NOTIFY_MASK
                        | GDK_LEAVE_NOTIFY_MASK;

  priv = CTK_EVENT_BOX (widget)->priv;

  visible_window = ctk_widget_get_has_window (widget);
  if (visible_window)
    {
      attributes.visual = ctk_widget_get_visual (widget);
      attributes.wclass = GDK_INPUT_OUTPUT;

      attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

      window = cdk_window_new (ctk_widget_get_parent_window (widget),
                               &attributes, attributes_mask);
      ctk_widget_set_window (widget, window);
      ctk_widget_register_window (widget, window);
    }
  else
    {
      window = ctk_widget_get_parent_window (widget);
      ctk_widget_set_window (widget, window);
      g_object_ref (window);
    }

  if (!visible_window || priv->above_child)
    {
      attributes.wclass = GDK_INPUT_ONLY;
      if (!visible_window)
        attributes_mask = GDK_WA_X | GDK_WA_Y;
      else
        attributes_mask = 0;

      priv->event_window = cdk_window_new (window,
                                           &attributes, attributes_mask);
      ctk_widget_register_window (widget, priv->event_window);
    }
}

static void
ctk_event_box_unrealize (CtkWidget *widget)
{
  CtkEventBoxPrivate *priv = CTK_EVENT_BOX (widget)->priv;

  if (priv->event_window != NULL)
    {
      ctk_widget_unregister_window (widget, priv->event_window);
      cdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  CTK_WIDGET_CLASS (ctk_event_box_parent_class)->unrealize (widget);
}

static void
ctk_event_box_map (CtkWidget *widget)
{
  CtkEventBoxPrivate *priv = CTK_EVENT_BOX (widget)->priv;

  if (priv->event_window != NULL && !priv->above_child)
    cdk_window_show (priv->event_window);

  CTK_WIDGET_CLASS (ctk_event_box_parent_class)->map (widget);

  if (priv->event_window != NULL && priv->above_child)
    cdk_window_show (priv->event_window);
}

static void
ctk_event_box_unmap (CtkWidget *widget)
{
  CtkEventBoxPrivate *priv = CTK_EVENT_BOX (widget)->priv;

  if (priv->event_window != NULL)
    cdk_window_hide (priv->event_window);

  CTK_WIDGET_CLASS (ctk_event_box_parent_class)->unmap (widget);
}

static void
ctk_event_box_get_preferred_width (CtkWidget *widget,
                                   gint      *minimum,
                                   gint      *natural)
{
  CtkBin *bin = CTK_BIN (widget);
  CtkWidget *child;

  *minimum = 0;
  *natural = 0;

  child = ctk_bin_get_child (bin);
  if (child && ctk_widget_get_visible (child))
    ctk_widget_get_preferred_width (child, minimum, natural);
}

static void
ctk_event_box_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
							   gint       width,
							   gint      *minimum,
							   gint      *natural,
							   gint      *minimum_baseline,
							   gint      *natural_baseline)
{
  CtkBin *bin = CTK_BIN (widget);
  CtkWidget *child;

  *minimum = 0;
  *natural = 0;

  if (minimum_baseline)
    *minimum_baseline = -1;

  if (natural_baseline)
    *natural_baseline = -1;

  child = ctk_bin_get_child (bin);
  if (child && ctk_widget_get_visible (child))
    ctk_widget_get_preferred_height_and_baseline_for_width (child,
							    width,
							    minimum,
							    natural,
							    minimum_baseline,
							    natural_baseline);
}

static void
ctk_event_box_get_preferred_height (CtkWidget *widget,
                                    gint      *minimum,
                                    gint      *natural)
{
  ctk_event_box_get_preferred_height_and_baseline_for_width (widget, -1,
							     minimum, natural,
							     NULL, NULL);
}

static void
ctk_event_box_size_allocate (CtkWidget     *widget,
                             CtkAllocation *allocation)
{
  CtkBin *bin;
  CtkAllocation child_allocation;
  gint baseline;
  CtkEventBoxPrivate *priv;
  CtkWidget *child;

  bin = CTK_BIN (widget);

  ctk_widget_set_allocation (widget, allocation);

  if (!ctk_widget_get_has_window (widget))
    {
      child_allocation.x = allocation->x;
      child_allocation.y = allocation->y;
    }
  else
    {
      child_allocation.x = 0;
      child_allocation.y = 0;
    }
  child_allocation.width = allocation->width;
  child_allocation.height = allocation->height;

  if (ctk_widget_get_realized (widget))
    {
      priv = CTK_EVENT_BOX (widget)->priv;

      if (priv->event_window != NULL)
        cdk_window_move_resize (priv->event_window,
                                child_allocation.x,
                                child_allocation.y,
                                child_allocation.width,
                                child_allocation.height);

      if (ctk_widget_get_has_window (widget))
        cdk_window_move_resize (ctk_widget_get_window (widget),
                                allocation->x,
                                allocation->y,
                                child_allocation.width,
                                child_allocation.height);
    }

  baseline = ctk_widget_get_allocated_baseline (widget);
  child = ctk_bin_get_child (bin);
  if (child)
    ctk_widget_size_allocate_with_baseline (child, &child_allocation, baseline);
}

static gboolean
ctk_event_box_draw (CtkWidget *widget,
                    cairo_t   *cr)
{
  if (ctk_widget_get_has_window (widget) &&
      !ctk_widget_get_app_paintable (widget))
    {
      CtkStyleContext *context;

      context = ctk_widget_get_style_context (widget);

      ctk_render_background (context, cr, 0, 0,
                             ctk_widget_get_allocated_width (widget),
                             ctk_widget_get_allocated_height (widget));
      ctk_render_frame (context, cr, 0, 0,
                        ctk_widget_get_allocated_width (widget),
                        ctk_widget_get_allocated_height (widget));
    }

  CTK_WIDGET_CLASS (ctk_event_box_parent_class)->draw (widget, cr);

  return FALSE;
}
