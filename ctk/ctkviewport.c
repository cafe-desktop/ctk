/* GTK - The GIMP Toolkit
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#include "ctkviewport.h"

#include "ctkadjustment.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctkpixelcacheprivate.h"
#include "ctkprivate.h"
#include "ctkscrollable.h"
#include "ctkrenderbackgroundprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctktypebuiltins.h"
#include "ctkwidgetprivate.h"


/**
 * SECTION:ctkviewport
 * @Short_description: An adapter which makes widgets scrollable
 * @Title: GtkViewport
 * @See_also:#GtkScrolledWindow, #GtkAdjustment
 *
 * The #GtkViewport widget acts as an adaptor class, implementing
 * scrollability for child widgets that lack their own scrolling
 * capabilities. Use GtkViewport to scroll child widgets such as
 * #GtkGrid, #GtkBox, and so on.
 *
 * If a widget has native scrolling abilities, such as #GtkTextView,
 * #GtkTreeView or #GtkIconView, it can be added to a #GtkScrolledWindow
 * with ctk_container_add(). If a widget does not, you must first add the
 * widget to a #GtkViewport, then add the viewport to the scrolled window.
 * ctk_container_add() does this automatically if a child that does not
 * implement #GtkScrollable is added to a #GtkScrolledWindow, so you can
 * ignore the presence of the viewport.
 *
 * The GtkViewport will start scrolling content only if allocated less
 * than the child widget’s minimum size in a given orientation.
 *
 * # CSS nodes
 *
 * GtkViewport has a single CSS node with name viewport.
 */

struct _GtkViewportPrivate
{
  GtkAdjustment  *hadjustment;
  GtkAdjustment  *vadjustment;
  GtkShadowType   shadow_type;

  GdkWindow      *bin_window;
  GdkWindow      *view_window;

  GtkCssGadget *gadget;

  GtkPixelCache *pixel_cache;

  /* GtkScrollablePolicy needs to be checked when
   * driving the scrollable adjustment values */
  guint hscroll_policy : 1;
  guint vscroll_policy : 1;
};

enum {
  PROP_0,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_HSCROLL_POLICY,
  PROP_VSCROLL_POLICY,
  PROP_SHADOW_TYPE
};


static void ctk_viewport_set_property             (GObject         *object,
						   guint            prop_id,
						   const GValue    *value,
						   GParamSpec      *pspec);
static void ctk_viewport_get_property             (GObject         *object,
						   guint            prop_id,
						   GValue          *value,
						   GParamSpec      *pspec);
static void ctk_viewport_finalize                 (GObject         *object);
static void ctk_viewport_destroy                  (GtkWidget        *widget);
static void ctk_viewport_realize                  (GtkWidget        *widget);
static void ctk_viewport_unrealize                (GtkWidget        *widget);
static void ctk_viewport_map                      (GtkWidget        *widget);
static void ctk_viewport_unmap                    (GtkWidget        *widget);
static gint ctk_viewport_draw                     (GtkWidget        *widget,
						   cairo_t          *cr);
static void ctk_viewport_remove                   (GtkContainer     *container,
						   GtkWidget        *widget);
static void ctk_viewport_add                      (GtkContainer     *container,
						   GtkWidget        *widget);
static void ctk_viewport_size_allocate            (GtkWidget        *widget,
						   GtkAllocation    *allocation);
static void ctk_viewport_adjustment_value_changed (GtkAdjustment    *adjustment,
						   gpointer          data);

static void ctk_viewport_get_preferred_width      (GtkWidget        *widget,
						   gint             *minimum_size,
						   gint             *natural_size);
static void ctk_viewport_get_preferred_height     (GtkWidget        *widget,
						   gint             *minimum_size,
						   gint             *natural_size);
static void ctk_viewport_get_preferred_width_for_height (GtkWidget  *widget,
                                                   gint              height,
						   gint             *minimum_size,
						   gint             *natural_size);
static void ctk_viewport_get_preferred_height_for_width (GtkWidget  *widget,
                                                   gint              width,
						   gint             *minimum_size,
						   gint             *natural_size);

static void viewport_set_adjustment               (GtkViewport      *viewport,
                                                   GtkOrientation    orientation,
                                                   GtkAdjustment    *adjustment);
static void ctk_viewport_queue_draw_region        (GtkWidget        *widget,
						   const cairo_region_t *region);

G_DEFINE_TYPE_WITH_CODE (GtkViewport, ctk_viewport, CTK_TYPE_BIN,
                         G_ADD_PRIVATE (GtkViewport)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_SCROLLABLE, NULL))

static void
ctk_viewport_measure (GtkCssGadget   *gadget,
                      GtkOrientation  orientation,
                      int             for_size,
                      int            *minimum,
                      int            *natural,
                      int            *minimum_baseline,
                      int            *natural_baseline,
                      gpointer        data)
{
  GtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  GtkWidget *child;

  *minimum = *natural = 0;

  child = ctk_bin_get_child (CTK_BIN (widget));
  if (child && ctk_widget_get_visible (child))
    _ctk_widget_get_preferred_size_for_size (child,
                                             orientation,
                                             for_size,
                                             minimum, natural,
                                             NULL, NULL);
}

static void
viewport_set_hadjustment_values (GtkViewport *viewport)
{
  GtkBin *bin = CTK_BIN (viewport);
  GtkAllocation view_allocation;
  GtkAdjustment *hadjustment = viewport->priv->hadjustment;
  GtkWidget *child;
  gdouble upper, value;

  ctk_css_gadget_get_content_allocation (viewport->priv->gadget,
                                         &view_allocation, NULL);

  child = ctk_bin_get_child (bin);
  if (child && ctk_widget_get_visible (child))
    {
      gint minimum_width, natural_width;
      gint scroll_height;

      if (viewport->priv->vscroll_policy == CTK_SCROLL_MINIMUM)
	ctk_widget_get_preferred_height (child, &scroll_height, NULL);
      else
	ctk_widget_get_preferred_height (child, NULL, &scroll_height);

      ctk_widget_get_preferred_width_for_height (child,
                                                 MAX (view_allocation.height, scroll_height),
                                                 &minimum_width,
                                                 &natural_width);

      if (viewport->priv->hscroll_policy == CTK_SCROLL_MINIMUM)
	upper = MAX (minimum_width, view_allocation.width);
      else
	upper = MAX (natural_width, view_allocation.width);
    }
  else
    upper = view_allocation.width;

  value = ctk_adjustment_get_value (hadjustment);
  /* We clamp to the left in RTL mode */
  if (ctk_widget_get_direction (CTK_WIDGET (viewport)) == CTK_TEXT_DIR_RTL)
    {
      gdouble dist = ctk_adjustment_get_upper (hadjustment)
                     - value
                     - ctk_adjustment_get_page_size (hadjustment);
      value = upper - dist - view_allocation.width;
    }

  ctk_adjustment_configure (hadjustment,
                            value,
                            0,
                            upper,
                            view_allocation.width * 0.1,
                            view_allocation.width * 0.9,
                            view_allocation.width);
}

static void
viewport_set_vadjustment_values (GtkViewport *viewport)
{
  GtkBin *bin = CTK_BIN (viewport);
  GtkAllocation view_allocation;
  GtkAdjustment *vadjustment = viewport->priv->vadjustment;
  GtkWidget *child;
  gdouble upper;

  ctk_css_gadget_get_content_allocation (viewport->priv->gadget,
                                         &view_allocation, NULL);

  child = ctk_bin_get_child (bin);
  if (child && ctk_widget_get_visible (child))
    {
      gint minimum_height, natural_height;
      gint scroll_width;

      if (viewport->priv->hscroll_policy == CTK_SCROLL_MINIMUM)
	ctk_widget_get_preferred_width (child, &scroll_width, NULL);
      else
	ctk_widget_get_preferred_width (child, NULL, &scroll_width);

      ctk_widget_get_preferred_height_for_width (child,
                                                 MAX (view_allocation.width, scroll_width),
                                                 &minimum_height,
                                                 &natural_height);

      if (viewport->priv->vscroll_policy == CTK_SCROLL_MINIMUM)
	upper = MAX (minimum_height, view_allocation.height);
      else
	upper = MAX (natural_height, view_allocation.height);
    }
  else
    upper = view_allocation.height;

  ctk_adjustment_configure (vadjustment,
                            ctk_adjustment_get_value (vadjustment),
                            0,
                            upper,
                            view_allocation.height * 0.1,
                            view_allocation.height * 0.9,
                            view_allocation.height);
}

static void
ctk_viewport_allocate (GtkCssGadget        *gadget,
                       const GtkAllocation *allocation,
                       int                  baseline,
                       GtkAllocation       *out_clip,
                       gpointer             data)
{
  GtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  GtkViewport *viewport = CTK_VIEWPORT (widget);
  GtkViewportPrivate *priv = viewport->priv;
  GtkAdjustment *hadjustment = priv->hadjustment;
  GtkAdjustment *vadjustment = priv->vadjustment;
  GtkWidget *child;

  g_object_freeze_notify (G_OBJECT (hadjustment));
  g_object_freeze_notify (G_OBJECT (vadjustment));

  viewport_set_hadjustment_values (viewport);
  viewport_set_vadjustment_values (viewport);

  if (ctk_widget_get_realized (widget))
    {
      gdk_window_move_resize (priv->view_window,
			      allocation->x,
			      allocation->y,
			      allocation->width,
			      allocation->height);
      gdk_window_move_resize (priv->bin_window,
                              - ctk_adjustment_get_value (hadjustment),
                              - ctk_adjustment_get_value (vadjustment),
                              ctk_adjustment_get_upper (hadjustment),
                              ctk_adjustment_get_upper (vadjustment));
    }

  child = ctk_bin_get_child (CTK_BIN (widget));
  if (child && ctk_widget_get_visible (child))
    {
      GtkAllocation child_allocation;

      child_allocation.x = 0;
      child_allocation.y = 0;
      child_allocation.width = ctk_adjustment_get_upper (hadjustment);
      child_allocation.height = ctk_adjustment_get_upper (vadjustment);

      ctk_widget_size_allocate (child, &child_allocation);
    }

  g_object_thaw_notify (G_OBJECT (hadjustment));
  g_object_thaw_notify (G_OBJECT (vadjustment));
}

static void
draw_bin (cairo_t *cr,
	  gpointer user_data)
{
  GtkWidget *widget = CTK_WIDGET (user_data);
  CTK_WIDGET_CLASS (ctk_viewport_parent_class)->draw (widget, cr);
}

static gboolean
ctk_viewport_render (GtkCssGadget *gadget,
                     cairo_t      *cr,
                     int           x,
                     int           y,
                     int           width,
                     int           height,
                     gpointer      data)
{
  GtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  GtkViewport *viewport = CTK_VIEWPORT (widget);
  GtkViewportPrivate *priv = viewport->priv;

  if (ctk_cairo_should_draw_window (cr, priv->bin_window))
    {
      cairo_rectangle_int_t view_rect;
      cairo_rectangle_int_t canvas_rect;

      gdk_window_get_position (priv->view_window, &view_rect.x, &view_rect.y);
      view_rect.width = gdk_window_get_width (priv->view_window);
      view_rect.height = gdk_window_get_height (priv->view_window);

      gdk_window_get_position (priv->bin_window, &canvas_rect.x, &canvas_rect.y);
      canvas_rect.width = gdk_window_get_width (priv->bin_window);
      canvas_rect.height = gdk_window_get_height (priv->bin_window);

      _ctk_pixel_cache_draw (priv->pixel_cache, cr, priv->bin_window,
			     &view_rect, &canvas_rect,
			     draw_bin, widget);
    }

  return FALSE;
}

static void
ctk_viewport_class_init (GtkViewportClass *class)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  gobject_class = G_OBJECT_CLASS (class);
  widget_class = (GtkWidgetClass*) class;
  container_class = (GtkContainerClass*) class;

  gobject_class->set_property = ctk_viewport_set_property;
  gobject_class->get_property = ctk_viewport_get_property;
  gobject_class->finalize = ctk_viewport_finalize;

  widget_class->destroy = ctk_viewport_destroy;
  widget_class->realize = ctk_viewport_realize;
  widget_class->unrealize = ctk_viewport_unrealize;
  widget_class->map = ctk_viewport_map;
  widget_class->unmap = ctk_viewport_unmap;
  widget_class->draw = ctk_viewport_draw;
  widget_class->size_allocate = ctk_viewport_size_allocate;
  widget_class->get_preferred_width = ctk_viewport_get_preferred_width;
  widget_class->get_preferred_height = ctk_viewport_get_preferred_height;
  widget_class->get_preferred_width_for_height = ctk_viewport_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = ctk_viewport_get_preferred_height_for_width;
  widget_class->queue_draw_region = ctk_viewport_queue_draw_region;
  
  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_VIEWPORT);

  container_class->remove = ctk_viewport_remove;
  container_class->add = ctk_viewport_add;
  ctk_container_class_handle_border_width (container_class);

  /* GtkScrollable implementation */
  g_object_class_override_property (gobject_class, PROP_HADJUSTMENT,    "hadjustment");
  g_object_class_override_property (gobject_class, PROP_VADJUSTMENT,    "vadjustment");
  g_object_class_override_property (gobject_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (gobject_class, PROP_VSCROLL_POLICY, "vscroll-policy");

  g_object_class_install_property (gobject_class,
                                   PROP_SHADOW_TYPE,
                                   g_param_spec_enum ("shadow-type",
						      P_("Shadow type"),
						      P_("Determines how the shadowed box around the viewport is drawn"),
						      CTK_TYPE_SHADOW_TYPE,
						      CTK_SHADOW_IN,
						      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  ctk_widget_class_set_css_name (widget_class, "viewport");
}

static void
ctk_viewport_set_property (GObject         *object,
			   guint            prop_id,
			   const GValue    *value,
			   GParamSpec      *pspec)
{
  GtkViewport *viewport;

  viewport = CTK_VIEWPORT (object);

  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      viewport_set_adjustment (viewport, CTK_ORIENTATION_HORIZONTAL, g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      viewport_set_adjustment (viewport, CTK_ORIENTATION_VERTICAL, g_value_get_object (value));
      break;
    case PROP_HSCROLL_POLICY:
      if (viewport->priv->hscroll_policy != g_value_get_enum (value))
        {
          viewport->priv->hscroll_policy = g_value_get_enum (value);
          ctk_widget_queue_resize (CTK_WIDGET (viewport));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_VSCROLL_POLICY:
      if (viewport->priv->vscroll_policy != g_value_get_enum (value))
        {
          viewport->priv->vscroll_policy = g_value_get_enum (value);
          ctk_widget_queue_resize (CTK_WIDGET (viewport));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_SHADOW_TYPE:
      ctk_viewport_set_shadow_type (viewport, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_viewport_get_property (GObject         *object,
			   guint            prop_id,
			   GValue          *value,
			   GParamSpec      *pspec)
{
  GtkViewport *viewport = CTK_VIEWPORT (object);
  GtkViewportPrivate *priv = viewport->priv;

  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      g_value_set_object (value, priv->hadjustment);
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value, priv->vadjustment);
      break;
    case PROP_HSCROLL_POLICY:
      g_value_set_enum (value, priv->hscroll_policy);
      break;
    case PROP_VSCROLL_POLICY:
      g_value_set_enum (value, priv->vscroll_policy);
      break;
    case PROP_SHADOW_TYPE:
      g_value_set_enum (value, priv->shadow_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_viewport_init (GtkViewport *viewport)
{
  GtkWidget *widget;
  GtkViewportPrivate *priv;
  GtkCssNode *widget_node;

  viewport->priv = ctk_viewport_get_instance_private (viewport);
  priv = viewport->priv;
  widget = CTK_WIDGET (viewport);

  ctk_widget_set_has_window (widget, TRUE);

  priv->shadow_type = CTK_SHADOW_IN;
  priv->view_window = NULL;
  priv->bin_window = NULL;
  priv->hadjustment = NULL;
  priv->vadjustment = NULL;

  priv->pixel_cache = _ctk_pixel_cache_new ();

  widget_node = ctk_widget_get_css_node (widget);
  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     widget,
                                                     ctk_viewport_measure,
                                                     ctk_viewport_allocate,
                                                     ctk_viewport_render,
                                                     NULL, NULL);

  ctk_css_gadget_add_class (priv->gadget, CTK_STYLE_CLASS_FRAME);
  viewport_set_adjustment (viewport, CTK_ORIENTATION_HORIZONTAL, NULL);
  viewport_set_adjustment (viewport, CTK_ORIENTATION_VERTICAL, NULL);
}

/**
 * ctk_viewport_new:
 * @hadjustment: (allow-none): horizontal adjustment
 * @vadjustment: (allow-none): vertical adjustment
 *
 * Creates a new #GtkViewport with the given adjustments, or with default
 * adjustments if none are given.
 *
 * Returns: a new #GtkViewport
 */
GtkWidget*
ctk_viewport_new (GtkAdjustment *hadjustment,
		  GtkAdjustment *vadjustment)
{
  GtkWidget *viewport;

  viewport = g_object_new (CTK_TYPE_VIEWPORT,
                           "hadjustment", hadjustment,
                           "vadjustment", vadjustment,
                           NULL);

  return viewport;
}

#define ADJUSTMENT_POINTER(viewport, orientation)         \
  (((orientation) == CTK_ORIENTATION_HORIZONTAL) ?        \
     &(viewport)->priv->hadjustment : &(viewport)->priv->vadjustment)

static void
viewport_disconnect_adjustment (GtkViewport    *viewport,
				GtkOrientation  orientation)
{
  GtkAdjustment **adjustmentp = ADJUSTMENT_POINTER (viewport, orientation);

  if (*adjustmentp)
    {
      g_signal_handlers_disconnect_by_func (*adjustmentp,
					    ctk_viewport_adjustment_value_changed,
					    viewport);
      g_object_unref (*adjustmentp);
      *adjustmentp = NULL;
    }
}

static void
ctk_viewport_destroy (GtkWidget *widget)
{
  GtkViewport *viewport = CTK_VIEWPORT (widget);
  GtkViewportPrivate *priv = viewport->priv;

  viewport_disconnect_adjustment (viewport, CTK_ORIENTATION_HORIZONTAL);
  viewport_disconnect_adjustment (viewport, CTK_ORIENTATION_VERTICAL);

  CTK_WIDGET_CLASS (ctk_viewport_parent_class)->destroy (widget);

  g_clear_pointer (&priv->pixel_cache, _ctk_pixel_cache_free);
}

static void
ctk_viewport_finalize (GObject *object)
{
  GtkViewport *viewport = CTK_VIEWPORT (object);
  GtkViewportPrivate *priv = viewport->priv;

  g_clear_object (&priv->gadget);

  G_OBJECT_CLASS (ctk_viewport_parent_class)->finalize (object);
}

/**
 * ctk_viewport_get_hadjustment:
 * @viewport: a #GtkViewport.
 *
 * Returns the horizontal adjustment of the viewport.
 *
 * Returns: (transfer none): the horizontal adjustment of @viewport.
 *
 * Deprecated: 3.0: Use ctk_scrollable_get_hadjustment()
 **/
GtkAdjustment*
ctk_viewport_get_hadjustment (GtkViewport *viewport)
{
  g_return_val_if_fail (CTK_IS_VIEWPORT (viewport), NULL);

  return viewport->priv->hadjustment;
}

/**
 * ctk_viewport_get_vadjustment:
 * @viewport: a #GtkViewport.
 * 
 * Returns the vertical adjustment of the viewport.
 *
 * Returns: (transfer none): the vertical adjustment of @viewport.
 *
 * Deprecated: 3.0: Use ctk_scrollable_get_vadjustment()
 **/
GtkAdjustment*
ctk_viewport_get_vadjustment (GtkViewport *viewport)
{
  g_return_val_if_fail (CTK_IS_VIEWPORT (viewport), NULL);

  return viewport->priv->vadjustment;
}

static void
viewport_set_adjustment (GtkViewport    *viewport,
			 GtkOrientation  orientation,
			 GtkAdjustment  *adjustment)
{
  GtkAdjustment **adjustmentp = ADJUSTMENT_POINTER (viewport, orientation);

  if (adjustment && adjustment == *adjustmentp)
    return;

  if (!adjustment)
    adjustment = ctk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  viewport_disconnect_adjustment (viewport, orientation);
  *adjustmentp = adjustment;
  g_object_ref_sink (adjustment);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    viewport_set_hadjustment_values (viewport);
  else
    viewport_set_vadjustment_values (viewport);

  g_signal_connect (adjustment, "value-changed",
		    G_CALLBACK (ctk_viewport_adjustment_value_changed),
		    viewport);

  ctk_viewport_adjustment_value_changed (adjustment, viewport);
}

/**
 * ctk_viewport_set_hadjustment:
 * @viewport: a #GtkViewport.
 * @adjustment: (allow-none): a #GtkAdjustment.
 *
 * Sets the horizontal adjustment of the viewport.
 *
 * Deprecated: 3.0: Use ctk_scrollable_set_hadjustment()
 **/
void
ctk_viewport_set_hadjustment (GtkViewport   *viewport,
			      GtkAdjustment *adjustment)
{
  g_return_if_fail (CTK_IS_VIEWPORT (viewport));
  if (adjustment)
    g_return_if_fail (CTK_IS_ADJUSTMENT (adjustment));

  viewport_set_adjustment (viewport, CTK_ORIENTATION_HORIZONTAL, adjustment);

  g_object_notify (G_OBJECT (viewport), "hadjustment");
}

/**
 * ctk_viewport_set_vadjustment:
 * @viewport: a #GtkViewport.
 * @adjustment: (allow-none): a #GtkAdjustment.
 *
 * Sets the vertical adjustment of the viewport.
 *
 * Deprecated: 3.0: Use ctk_scrollable_set_vadjustment()
 **/
void
ctk_viewport_set_vadjustment (GtkViewport   *viewport,
			      GtkAdjustment *adjustment)
{
  g_return_if_fail (CTK_IS_VIEWPORT (viewport));
  if (adjustment)
    g_return_if_fail (CTK_IS_ADJUSTMENT (adjustment));

  viewport_set_adjustment (viewport, CTK_ORIENTATION_VERTICAL, adjustment);

  g_object_notify (G_OBJECT (viewport), "vadjustment");
}

/** 
 * ctk_viewport_set_shadow_type:
 * @viewport: a #GtkViewport.
 * @type: the new shadow type.
 *
 * Sets the shadow type of the viewport.
 **/ 
void
ctk_viewport_set_shadow_type (GtkViewport   *viewport,
			      GtkShadowType  type)
{
  GtkViewportPrivate *priv;
  GtkWidget *widget;
  GtkStyleContext *context;

  g_return_if_fail (CTK_IS_VIEWPORT (viewport));

  widget = CTK_WIDGET (viewport);
  priv = viewport->priv;

  if ((GtkShadowType) priv->shadow_type != type)
    {
      priv->shadow_type = type;

      context = ctk_widget_get_style_context (widget);
      if (type != CTK_SHADOW_NONE)
        ctk_style_context_add_class (context, CTK_STYLE_CLASS_FRAME);
      else
        ctk_style_context_remove_class (context, CTK_STYLE_CLASS_FRAME);
 
      ctk_widget_queue_resize (widget);

      g_object_notify (G_OBJECT (viewport), "shadow-type");
    }
}

/**
 * ctk_viewport_get_shadow_type:
 * @viewport: a #GtkViewport
 *
 * Gets the shadow type of the #GtkViewport. See
 * ctk_viewport_set_shadow_type().
 *
 * Returns: the shadow type 
 **/
GtkShadowType
ctk_viewport_get_shadow_type (GtkViewport *viewport)
{
  g_return_val_if_fail (CTK_IS_VIEWPORT (viewport), CTK_SHADOW_NONE);

  return viewport->priv->shadow_type;
}

/**
 * ctk_viewport_get_bin_window:
 * @viewport: a #GtkViewport
 *
 * Gets the bin window of the #GtkViewport.
 *
 * Returns: (transfer none): a #GdkWindow
 *
 * Since: 2.20
 **/
GdkWindow*
ctk_viewport_get_bin_window (GtkViewport *viewport)
{
  g_return_val_if_fail (CTK_IS_VIEWPORT (viewport), NULL);

  return viewport->priv->bin_window;
}

/**
 * ctk_viewport_get_view_window:
 * @viewport: a #GtkViewport
 *
 * Gets the view window of the #GtkViewport.
 *
 * Returns: (transfer none): a #GdkWindow
 *
 * Since: 2.22
 **/
GdkWindow*
ctk_viewport_get_view_window (GtkViewport *viewport)
{
  g_return_val_if_fail (CTK_IS_VIEWPORT (viewport), NULL);

  return viewport->priv->view_window;
}

static void
ctk_viewport_bin_window_invalidate_handler (GdkWindow *window,
					    cairo_region_t *region)
{
  gpointer widget;
  GtkViewport *viewport;
  GtkViewportPrivate *priv;

  gdk_window_get_user_data (window, &widget);
  viewport = CTK_VIEWPORT (widget);
  priv = viewport->priv;

  _ctk_pixel_cache_invalidate (priv->pixel_cache, region);
}

static void
ctk_viewport_queue_draw_region (GtkWidget *widget,
				const cairo_region_t *region)
{
  GtkViewport *viewport = CTK_VIEWPORT (widget);
  GtkViewportPrivate *priv = viewport->priv;

  /* There is no way we can know if a region targets the
     not-currently-visible but in pixel cache region, so we
     always just invalidate the whole thing whenever the
     tree view gets a queue draw. This doesn't normally happen
     in normal scrolling cases anyway. */
  _ctk_pixel_cache_invalidate (priv->pixel_cache, NULL);

  CTK_WIDGET_CLASS (ctk_viewport_parent_class)->queue_draw_region (widget,
								   region);
}


static void
ctk_viewport_realize (GtkWidget *widget)
{
  GtkViewport *viewport = CTK_VIEWPORT (widget);
  GtkViewportPrivate *priv = viewport->priv;
  GtkBin *bin = CTK_BIN (widget);
  GtkAdjustment *hadjustment = priv->hadjustment;
  GtkAdjustment *vadjustment = priv->vadjustment;
  GtkAllocation allocation;
  GtkAllocation view_allocation;
  GtkWidget *child;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;
  gint event_mask;

  ctk_widget_set_realized (widget, TRUE);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = ctk_widget_get_visual (widget);

  event_mask = ctk_widget_get_events (widget);

  attributes.event_mask = event_mask | GDK_SCROLL_MASK | GDK_TOUCH_MASK | GDK_SMOOTH_SCROLL_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

  window = gdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_set_window (widget, window);
  ctk_widget_register_window (widget, window);

  ctk_css_gadget_get_content_allocation (priv->gadget,
                                         &view_allocation, NULL);

  attributes.x = view_allocation.x;
  attributes.y = view_allocation.y;
  attributes.width = view_allocation.width;
  attributes.height = view_allocation.height;
  attributes.event_mask = 0;

  priv->view_window = gdk_window_new (window,
                                      &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->view_window);

  attributes.x = - ctk_adjustment_get_value (hadjustment);
  attributes.y = - ctk_adjustment_get_value (vadjustment);
  attributes.width = ctk_adjustment_get_upper (hadjustment);
  attributes.height = ctk_adjustment_get_upper (vadjustment);
  
  attributes.event_mask = event_mask;

  priv->bin_window = gdk_window_new (priv->view_window, &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->bin_window);
  gdk_window_set_invalidate_handler (priv->bin_window,
				     ctk_viewport_bin_window_invalidate_handler);

  child = ctk_bin_get_child (bin);
  if (child)
    ctk_widget_set_parent_window (child, priv->bin_window);

  gdk_window_show (priv->bin_window);
  gdk_window_show (priv->view_window);
}

static void
ctk_viewport_unrealize (GtkWidget *widget)
{
  GtkViewport *viewport = CTK_VIEWPORT (widget);
  GtkViewportPrivate *priv = viewport->priv;

  ctk_widget_unregister_window (widget, priv->view_window);
  gdk_window_destroy (priv->view_window);
  priv->view_window = NULL;

  ctk_widget_unregister_window (widget, priv->bin_window);
  gdk_window_destroy (priv->bin_window);
  priv->bin_window = NULL;

  CTK_WIDGET_CLASS (ctk_viewport_parent_class)->unrealize (widget);
}

static void
ctk_viewport_map (GtkWidget *widget)
{
  GtkViewport *viewport = CTK_VIEWPORT (widget);
  GtkViewportPrivate *priv = viewport->priv;

  _ctk_pixel_cache_map (priv->pixel_cache);

  CTK_WIDGET_CLASS (ctk_viewport_parent_class)->map (widget);
}

static void
ctk_viewport_unmap (GtkWidget *widget)
{
  GtkViewport *viewport = CTK_VIEWPORT (widget);
  GtkViewportPrivate *priv = viewport->priv;

  CTK_WIDGET_CLASS (ctk_viewport_parent_class)->unmap (widget);

  _ctk_pixel_cache_unmap (priv->pixel_cache);
}

static gint
ctk_viewport_draw (GtkWidget *widget,
                   cairo_t   *cr)
{
  GtkViewport *viewport = CTK_VIEWPORT (widget);
  GtkViewportPrivate *priv = viewport->priv;

  if (ctk_cairo_should_draw_window (cr, ctk_widget_get_window (widget)) ||
      ctk_cairo_should_draw_window (cr, priv->bin_window))
    ctk_css_gadget_draw (priv->gadget, cr);

  return FALSE;
}

static void
ctk_viewport_update_pixelcache_opacity (GtkWidget   *child,
                                        GtkViewport *viewport)
{
  GtkViewportPrivate *priv = viewport->priv;

  ctk_pixel_cache_set_is_opaque (priv->pixel_cache,
                                 ctk_css_style_render_background_is_opaque (
                                   ctk_style_context_lookup_style (
                                     ctk_widget_get_style_context (child))));
}

static void
ctk_viewport_remove (GtkContainer *container,
		     GtkWidget    *child)
{
  GtkViewport *viewport = CTK_VIEWPORT (container);
  GtkViewportPrivate *priv = viewport->priv;

  if (g_signal_handlers_disconnect_by_func (child, ctk_viewport_update_pixelcache_opacity, viewport) != 1)
    {
      g_assert_not_reached ();
    }

  CTK_CONTAINER_CLASS (ctk_viewport_parent_class)->remove (container, child);

  ctk_pixel_cache_set_is_opaque (priv->pixel_cache, FALSE);
}

static void
ctk_viewport_add (GtkContainer *container,
		  GtkWidget    *child)
{
  GtkBin *bin = CTK_BIN (container);
  GtkViewport *viewport = CTK_VIEWPORT (bin);
  GtkViewportPrivate *priv = viewport->priv;

  g_return_if_fail (ctk_bin_get_child (bin) == NULL);

  ctk_widget_set_parent_window (child, priv->bin_window);
  
  CTK_CONTAINER_CLASS (ctk_viewport_parent_class)->add (container, child);

  g_signal_connect (child, "style-updated", G_CALLBACK (ctk_viewport_update_pixelcache_opacity), viewport);
  ctk_viewport_update_pixelcache_opacity (child, viewport);
}

static void
ctk_viewport_size_allocate (GtkWidget     *widget,
			    GtkAllocation *allocation)
{
  GtkViewport *viewport = CTK_VIEWPORT (widget);
  GtkViewportPrivate *priv = viewport->priv;
  GtkAllocation clip, content_allocation, widget_allocation;

  /* If our size changed, and we have a shadow, queue a redraw on widget->window to
   * redraw the shadow correctly.
   */
  ctk_widget_get_allocation (widget, &widget_allocation);
  if (ctk_widget_get_mapped (widget) &&
      priv->shadow_type != CTK_SHADOW_NONE &&
      (widget_allocation.width != allocation->width ||
       widget_allocation.height != allocation->height))
    gdk_window_invalidate_rect (ctk_widget_get_window (widget), NULL, FALSE);

  ctk_widget_set_allocation (widget, allocation);

  if (ctk_widget_get_realized (widget))
    gdk_window_move_resize (ctk_widget_get_window (widget),
                            allocation->x,
                            allocation->y,
                            allocation->width,
                            allocation->height);

  content_allocation = *allocation;
  content_allocation.x = content_allocation.y = 0;
  ctk_css_gadget_allocate (priv->gadget,
                           &content_allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  clip.x += allocation->x;
  clip.y += allocation->y;
  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_viewport_adjustment_value_changed (GtkAdjustment *adjustment,
				       gpointer       data)
{
  GtkViewport *viewport = CTK_VIEWPORT (data);
  GtkViewportPrivate *priv = viewport->priv;
  GtkBin *bin = CTK_BIN (data);
  GtkWidget *child;

  child = ctk_bin_get_child (bin);
  if (child && ctk_widget_get_visible (child) &&
      ctk_widget_get_realized (CTK_WIDGET (viewport)))
    {
      GtkAdjustment *hadjustment = priv->hadjustment;
      GtkAdjustment *vadjustment = priv->vadjustment;
      gint old_x, old_y;
      gint new_x, new_y;

      gdk_window_get_position (priv->bin_window, &old_x, &old_y);
      new_x = - ctk_adjustment_get_value (hadjustment);
      new_y = - ctk_adjustment_get_value (vadjustment);

      if (new_x != old_x || new_y != old_y)
	gdk_window_move (priv->bin_window, new_x, new_y);
    }
}

static void
ctk_viewport_get_preferred_width (GtkWidget *widget,
                                  gint      *minimum_size,
                                  gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_VIEWPORT (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_viewport_get_preferred_height (GtkWidget *widget,
                                   gint      *minimum_size,
                                   gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_VIEWPORT (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_viewport_get_preferred_width_for_height (GtkWidget *widget,
                                             gint       height,
                                             gint      *minimum_size,
                                             gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_VIEWPORT (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_viewport_get_preferred_height_for_width (GtkWidget *widget,
                                             gint       width,
                                             gint      *minimum_size,
                                             gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_VIEWPORT (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}
