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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * CtkLayout: Widget for scrolling of arbitrary-sized areas.
 *
 * Copyright Owen Taylor, 1998
 */

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#include "ctklayout.h"

#include "cdk/cdk.h"

#include "ctkadjustment.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctkscrollable.h"


/**
 * SECTION:ctklayout
 * @Short_description: Infinite scrollable area containing child widgets
 *   and/or custom drawing
 * @Title: CtkLayout
 * @See_also: #CtkDrawingArea, #CtkFixed
 *
 * #CtkLayout is similar to #CtkDrawingArea in that it’s a “blank slate” and
 * doesn’t do anything except paint a blank background by default. It’s
 * different in that it supports scrolling natively due to implementing
 * #CtkScrollable, and can contain child widgets since it’s a #CtkContainer.
 *
 * If you just want to draw, a #CtkDrawingArea is a better choice since it has
 * lower overhead. If you just need to position child widgets at specific
 * points, then #CtkFixed provides that functionality on its own.
 *
 * When handling expose events on a #CtkLayout, you must draw to the #CdkWindow
 * returned by ctk_layout_get_bin_window(), rather than to the one returned by
 * ctk_widget_get_window() as you would for a #CtkDrawingArea.
 */


typedef struct _CtkLayoutChild   CtkLayoutChild;

struct _CtkLayoutPrivate
{
  /* Properties */
  guint width;
  guint height;

  CtkAdjustment *hadjustment;
  CtkAdjustment *vadjustment;

  /* CtkScrollablePolicy needs to be checked when
   * driving the scrollable adjustment values */
  guint hscroll_policy : 1;
  guint vscroll_policy : 1;

  /* Properties */

  CdkVisibilityState visibility;
  CdkWindow *bin_window;

  GList *children;

  gint scroll_x;
  gint scroll_y;

  guint freeze_count;
};

struct _CtkLayoutChild {
  CtkWidget *widget;
  gint x;
  gint y;
};

enum {
   PROP_0,
   PROP_HADJUSTMENT,
   PROP_VADJUSTMENT,
   PROP_HSCROLL_POLICY,
   PROP_VSCROLL_POLICY,
   PROP_WIDTH,
   PROP_HEIGHT
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_X,
  CHILD_PROP_Y
};

static void ctk_layout_get_property       (GObject        *object,
                                           guint           prop_id,
                                           GValue         *value,
                                           GParamSpec     *pspec);
static void ctk_layout_set_property       (GObject        *object,
                                           guint           prop_id,
                                           const GValue   *value,
                                           GParamSpec     *pspec);
static void ctk_layout_finalize           (GObject        *object);
static void ctk_layout_realize            (CtkWidget      *widget);
static void ctk_layout_unrealize          (CtkWidget      *widget);
static void ctk_layout_map                (CtkWidget      *widget);
static void ctk_layout_get_preferred_width  (CtkWidget     *widget,
                                             gint          *minimum,
                                             gint          *natural);
static void ctk_layout_get_preferred_height (CtkWidget     *widget,
                                             gint          *minimum,
                                             gint          *natural);
static void ctk_layout_size_allocate      (CtkWidget      *widget,
                                           CtkAllocation  *allocation);
static gint ctk_layout_draw               (CtkWidget      *widget,
                                           cairo_t        *cr);
static void ctk_layout_add                (CtkContainer   *container,
					   CtkWidget      *widget);
static void ctk_layout_remove             (CtkContainer   *container,
                                           CtkWidget      *widget);
static void ctk_layout_forall             (CtkContainer   *container,
                                           gboolean        include_internals,
                                           CtkCallback     callback,
                                           gpointer        callback_data);
static void ctk_layout_set_child_property (CtkContainer   *container,
                                           CtkWidget      *child,
                                           guint           property_id,
                                           const GValue   *value,
                                           GParamSpec     *pspec);
static void ctk_layout_get_child_property (CtkContainer   *container,
                                           CtkWidget      *child,
                                           guint           property_id,
                                           GValue         *value,
                                           GParamSpec     *pspec);
static void ctk_layout_allocate_child     (CtkLayout      *layout,
                                           CtkLayoutChild *child);
static void ctk_layout_adjustment_changed (CtkAdjustment  *adjustment,
                                           CtkLayout      *layout);
static void ctk_layout_style_updated      (CtkWidget      *widget);

static void ctk_layout_set_hadjustment_values (CtkLayout      *layout);
static void ctk_layout_set_vadjustment_values (CtkLayout      *layout);

G_DEFINE_TYPE_WITH_CODE (CtkLayout, ctk_layout, CTK_TYPE_CONTAINER,
                         G_ADD_PRIVATE (CtkLayout)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_SCROLLABLE, NULL))

/* Public interface
 */
/**
 * ctk_layout_new:
 * @hadjustment: (allow-none): horizontal scroll adjustment, or %NULL
 * @vadjustment: (allow-none): vertical scroll adjustment, or %NULL
 * 
 * Creates a new #CtkLayout. Unless you have a specific adjustment
 * you’d like the layout to use for scrolling, pass %NULL for
 * @hadjustment and @vadjustment.
 * 
 * Returns: a new #CtkLayout
 **/
  
CtkWidget*    
ctk_layout_new (CtkAdjustment *hadjustment,
		CtkAdjustment *vadjustment)
{
  CtkLayout *layout;

  layout = g_object_new (CTK_TYPE_LAYOUT,
			 "hadjustment", hadjustment,
			 "vadjustment", vadjustment,
			 NULL);

  return CTK_WIDGET (layout);
}

/**
 * ctk_layout_get_bin_window:
 * @layout: a #CtkLayout
 *
 * Retrieve the bin window of the layout used for drawing operations.
 *
 * Returns: (transfer none): a #CdkWindow
 *
 * Since: 2.14
 **/
CdkWindow*
ctk_layout_get_bin_window (CtkLayout *layout)
{
  g_return_val_if_fail (CTK_IS_LAYOUT (layout), NULL);

  return layout->priv->bin_window;
}

/**
 * ctk_layout_get_hadjustment:
 * @layout: a #CtkLayout
 *
 * This function should only be called after the layout has been
 * placed in a #CtkScrolledWindow or otherwise configured for
 * scrolling. It returns the #CtkAdjustment used for communication
 * between the horizontal scrollbar and @layout.
 *
 * See #CtkScrolledWindow, #CtkScrollbar, #CtkAdjustment for details.
 *
 * Returns: (transfer none): horizontal scroll adjustment
 *
 * Deprecated: 3.0: Use ctk_scrollable_get_hadjustment()
 **/
CtkAdjustment*
ctk_layout_get_hadjustment (CtkLayout *layout)
{
  g_return_val_if_fail (CTK_IS_LAYOUT (layout), NULL);

  return layout->priv->hadjustment;
}
/**
 * ctk_layout_get_vadjustment:
 * @layout: a #CtkLayout
 *
 * This function should only be called after the layout has been
 * placed in a #CtkScrolledWindow or otherwise configured for
 * scrolling. It returns the #CtkAdjustment used for communication
 * between the vertical scrollbar and @layout.
 *
 * See #CtkScrolledWindow, #CtkScrollbar, #CtkAdjustment for details.
 *
 * Returns: (transfer none): vertical scroll adjustment
 *
 * Deprecated: 3.0: Use ctk_scrollable_get_vadjustment()
 **/
CtkAdjustment*
ctk_layout_get_vadjustment (CtkLayout *layout)
{
  g_return_val_if_fail (CTK_IS_LAYOUT (layout), NULL);

  return layout->priv->vadjustment;
}

static void
ctk_layout_set_hadjustment_values (CtkLayout *layout)
{
  CtkLayoutPrivate *priv = layout->priv;
  CtkAllocation  allocation;
  CtkAdjustment *adj = priv->hadjustment;
  gdouble old_value;
  gdouble new_value;
  gdouble new_upper;

  ctk_widget_get_allocation (CTK_WIDGET (layout), &allocation);

  old_value = ctk_adjustment_get_value (adj);
  new_upper = MAX (allocation.width, priv->width);

  g_object_set (adj,
                "lower", 0.0,
                "upper", new_upper,
                "page-size", (gdouble)allocation.width,
                "step-increment", allocation.width * 0.1,
                "page-increment", allocation.width * 0.9,
                NULL);

  new_value = CLAMP (old_value, 0, new_upper - allocation.width);
  if (new_value != old_value)
    ctk_adjustment_set_value (adj, new_value);
}

static void
ctk_layout_set_vadjustment_values (CtkLayout *layout)
{
  CtkAllocation  allocation;
  CtkAdjustment *adj = layout->priv->vadjustment;
  gdouble old_value;
  gdouble new_value;
  gdouble new_upper;

  ctk_widget_get_allocation (CTK_WIDGET (layout), &allocation);

  old_value = ctk_adjustment_get_value (adj);
  new_upper = MAX (allocation.height, layout->priv->height);

  g_object_set (adj,
                "lower", 0.0,
                "upper", new_upper,
                "page-size", (gdouble)allocation.height,
                "step-increment", allocation.height * 0.1,
                "page-increment", allocation.height * 0.9,
                NULL);

  new_value = CLAMP (old_value, 0, new_upper - allocation.height);
  if (new_value != old_value)
    ctk_adjustment_set_value (adj, new_value);
}

static void
ctk_layout_finalize (GObject *object)
{
  CtkLayout *layout = CTK_LAYOUT (object);
  CtkLayoutPrivate *priv = layout->priv;

  g_object_unref (priv->hadjustment);
  g_object_unref (priv->vadjustment);

  G_OBJECT_CLASS (ctk_layout_parent_class)->finalize (object);
}

static void
ctk_layout_do_set_hadjustment (CtkLayout     *layout,
                               CtkAdjustment *adjustment)
{
  CtkLayoutPrivate *priv;

  priv = layout->priv;

  if (adjustment && priv->hadjustment == adjustment)
        return;

  if (priv->hadjustment != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->hadjustment,
                                            ctk_layout_adjustment_changed,
                                            layout);
      g_object_unref (priv->hadjustment);
    }

  if (adjustment == NULL)
    adjustment = ctk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (ctk_layout_adjustment_changed), layout);
  priv->hadjustment = g_object_ref_sink (adjustment);
  ctk_layout_set_hadjustment_values (layout);

  g_object_notify (G_OBJECT (layout), "hadjustment");
}

/**
 * ctk_layout_set_hadjustment:
 * @layout: a #CtkLayout
 * @adjustment: (allow-none): new scroll adjustment
 *
 * Sets the horizontal scroll adjustment for the layout.
 *
 * See #CtkScrolledWindow, #CtkScrollbar, #CtkAdjustment for details.
 *
 * Deprecated: 3.0: Use ctk_scrollable_set_hadjustment()
 **/
void
ctk_layout_set_hadjustment (CtkLayout     *layout,
                            CtkAdjustment *adjustment)
{
  g_return_if_fail (CTK_IS_LAYOUT (layout));
  g_return_if_fail (adjustment == NULL || CTK_IS_ADJUSTMENT (adjustment));

  ctk_layout_do_set_hadjustment (layout, adjustment);
}

static void
ctk_layout_do_set_vadjustment (CtkLayout     *layout,
                               CtkAdjustment *adjustment)
{
  CtkLayoutPrivate *priv;

  priv = layout->priv;

  if (adjustment && priv->vadjustment == adjustment)
        return;

  if (priv->vadjustment != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->vadjustment,
                                            ctk_layout_adjustment_changed,
                                            layout);
      g_object_unref (priv->vadjustment);
    }

  if (adjustment == NULL)
    adjustment = ctk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (ctk_layout_adjustment_changed), layout);
  priv->vadjustment = g_object_ref_sink (adjustment);
  ctk_layout_set_vadjustment_values (layout);

  g_object_notify (G_OBJECT (layout), "vadjustment");
}

/**
 * ctk_layout_set_vadjustment:
 * @layout: a #CtkLayout
 * @adjustment: (allow-none): new scroll adjustment
 *
 * Sets the vertical scroll adjustment for the layout.
 *
 * See #CtkScrolledWindow, #CtkScrollbar, #CtkAdjustment for details.
 *
 * Deprecated: 3.0: Use ctk_scrollable_set_vadjustment()
 **/
void
ctk_layout_set_vadjustment (CtkLayout     *layout,
                            CtkAdjustment *adjustment)
{
  g_return_if_fail (CTK_IS_LAYOUT (layout));
  g_return_if_fail (adjustment == NULL || CTK_IS_ADJUSTMENT (adjustment));

  ctk_layout_do_set_vadjustment (layout, adjustment);
}

static CtkLayoutChild*
get_child (CtkLayout  *layout,
           CtkWidget  *widget)
{
  CtkLayoutPrivate *priv = layout->priv;
  GList *children;

  children = priv->children;
  while (children)
    {
      CtkLayoutChild *child;
      
      child = children->data;
      children = children->next;

      if (child->widget == widget)
        return child;
    }

  return NULL;
}

/**
 * ctk_layout_put:
 * @layout: a #CtkLayout
 * @child_widget: child widget
 * @x: X position of child widget
 * @y: Y position of child widget
 *
 * Adds @child_widget to @layout, at position (@x,@y).
 * @layout becomes the new parent container of @child_widget.
 * 
 **/
void           
ctk_layout_put (CtkLayout     *layout, 
		CtkWidget     *child_widget, 
		gint           x, 
		gint           y)
{
  CtkLayoutPrivate *priv;
  CtkLayoutChild *child;

  g_return_if_fail (CTK_IS_LAYOUT (layout));
  g_return_if_fail (CTK_IS_WIDGET (child_widget));

  priv = layout->priv;

  child = g_new (CtkLayoutChild, 1);

  child->widget = child_widget;
  child->x = x;
  child->y = y;

  priv->children = g_list_append (priv->children, child);

  if (ctk_widget_get_realized (CTK_WIDGET (layout)))
    ctk_widget_set_parent_window (child->widget, priv->bin_window);

  ctk_widget_set_parent (child_widget, CTK_WIDGET (layout));
}

static void
ctk_layout_move_internal (CtkLayout       *layout,
                          CtkWidget       *widget,
                          gboolean         change_x,
                          gint             x,
                          gboolean         change_y,
                          gint             y)
{
  CtkLayoutChild *child;

  child = get_child (layout, widget);

  g_assert (child);

  ctk_widget_freeze_child_notify (widget);
  
  if (change_x)
    {
      child->x = x;
      ctk_widget_child_notify (widget, "x");
    }

  if (change_y)
    {
      child->y = y;
      ctk_widget_child_notify (widget, "y");
    }

  ctk_widget_thaw_child_notify (widget);
  
  if (ctk_widget_get_visible (widget) &&
      ctk_widget_get_visible (CTK_WIDGET (layout)))
    ctk_widget_queue_resize (widget);
}

/**
 * ctk_layout_move:
 * @layout: a #CtkLayout
 * @child_widget: a current child of @layout
 * @x: X position to move to
 * @y: Y position to move to
 *
 * Moves a current child of @layout to a new position.
 * 
 **/
void           
ctk_layout_move (CtkLayout     *layout, 
		 CtkWidget     *child_widget, 
		 gint           x, 
		 gint           y)
{
  g_return_if_fail (CTK_IS_LAYOUT (layout));
  g_return_if_fail (CTK_IS_WIDGET (child_widget));
  g_return_if_fail (ctk_widget_get_parent (child_widget) == CTK_WIDGET (layout));

  ctk_layout_move_internal (layout, child_widget, TRUE, x, TRUE, y);
}

/**
 * ctk_layout_set_size:
 * @layout: a #CtkLayout
 * @width: width of entire scrollable area
 * @height: height of entire scrollable area
 *
 * Sets the size of the scrollable area of the layout.
 * 
 **/
void
ctk_layout_set_size (CtkLayout     *layout, 
		     guint          width,
		     guint          height)
{
  CtkLayoutPrivate *priv;
  CtkWidget *widget;

  g_return_if_fail (CTK_IS_LAYOUT (layout));

  priv = layout->priv;
  widget = CTK_WIDGET (layout);

  g_object_freeze_notify (G_OBJECT (layout));
  if (width != priv->width)
     {
	priv->width = width;
	g_object_notify (G_OBJECT (layout), "width");
     }
  if (height != priv->height)
     {
	priv->height = height;
	g_object_notify (G_OBJECT (layout), "height");
     }
  g_object_thaw_notify (G_OBJECT (layout));

  if (ctk_widget_get_realized (widget))
    {
      CtkAllocation allocation;

      ctk_widget_get_allocation (widget, &allocation);
      width = MAX (width, allocation.width);
      height = MAX (height, allocation.height);
      cdk_window_resize (priv->bin_window, width, height);
    }

  ctk_layout_set_hadjustment_values (layout);
  ctk_layout_set_vadjustment_values (layout);
}

/**
 * ctk_layout_get_size:
 * @layout: a #CtkLayout
 * @width: (out) (allow-none): location to store the width set on
 *     @layout, or %NULL
 * @height: (out) (allow-none): location to store the height set on
 *     @layout, or %NULL
 *
 * Gets the size that has been set on the layout, and that determines
 * the total extents of the layout’s scrollbar area. See
 * ctk_layout_set_size ().
 **/
void
ctk_layout_get_size (CtkLayout *layout,
		     guint     *width,
		     guint     *height)
{
  CtkLayoutPrivate *priv;

  g_return_if_fail (CTK_IS_LAYOUT (layout));

  priv = layout->priv;

  if (width)
    *width = priv->width;
  if (height)
    *height = priv->height;
}

/* Basic Object handling procedures
 */
static void
ctk_layout_class_init (CtkLayoutClass *class)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;

  gobject_class = (GObjectClass*) class;
  widget_class = (CtkWidgetClass*) class;
  container_class = (CtkContainerClass*) class;

  gobject_class->set_property = ctk_layout_set_property;
  gobject_class->get_property = ctk_layout_get_property;
  gobject_class->finalize = ctk_layout_finalize;

  container_class->set_child_property = ctk_layout_set_child_property;
  container_class->get_child_property = ctk_layout_get_child_property;

  ctk_container_class_install_child_property (container_class,
					      CHILD_PROP_X,
					      g_param_spec_int ("x",
                                                                P_("X position"),
                                                                P_("X position of child widget"),
                                                                G_MININT,
                                                                G_MAXINT,
                                                                0,
                                                                CTK_PARAM_READWRITE));

  ctk_container_class_install_child_property (container_class,
					      CHILD_PROP_Y,
					      g_param_spec_int ("y",
                                                                P_("Y position"),
                                                                P_("Y position of child widget"),
                                                                G_MININT,
                                                                G_MAXINT,
                                                                0,
                                                                CTK_PARAM_READWRITE));
  
  /* Scrollable interface */
  g_object_class_override_property (gobject_class, PROP_HADJUSTMENT,    "hadjustment");
  g_object_class_override_property (gobject_class, PROP_VADJUSTMENT,    "vadjustment");
  g_object_class_override_property (gobject_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (gobject_class, PROP_VSCROLL_POLICY, "vscroll-policy");

  g_object_class_install_property (gobject_class,
				   PROP_WIDTH,
				   g_param_spec_uint ("width",
						     P_("Width"),
						     P_("The width of the layout"),
						     0,
						     G_MAXINT,
						     100,
						     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  g_object_class_install_property (gobject_class,
				   PROP_HEIGHT,
				   g_param_spec_uint ("height",
						     P_("Height"),
						     P_("The height of the layout"),
						     0,
						     G_MAXINT,
						     100,
						     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  widget_class->realize = ctk_layout_realize;
  widget_class->unrealize = ctk_layout_unrealize;
  widget_class->map = ctk_layout_map;
  widget_class->get_preferred_width = ctk_layout_get_preferred_width;
  widget_class->get_preferred_height = ctk_layout_get_preferred_height;
  widget_class->size_allocate = ctk_layout_size_allocate;
  widget_class->draw = ctk_layout_draw;
  widget_class->style_updated = ctk_layout_style_updated;

  container_class->add = ctk_layout_add;
  container_class->remove = ctk_layout_remove;
  container_class->forall = ctk_layout_forall;
}

static void
ctk_layout_get_property (GObject     *object,
			 guint        prop_id,
			 GValue      *value,
			 GParamSpec  *pspec)
{
  CtkLayout *layout = CTK_LAYOUT (object);
  CtkLayoutPrivate *priv = layout->priv;

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
    case PROP_WIDTH:
      g_value_set_uint (value, priv->width);
      break;
    case PROP_HEIGHT:
      g_value_set_uint (value, priv->height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_layout_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  CtkLayout *layout = CTK_LAYOUT (object);
  CtkLayoutPrivate *priv = layout->priv;

  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      ctk_layout_do_set_hadjustment (layout, g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      ctk_layout_do_set_vadjustment (layout, g_value_get_object (value));
      break;
    case PROP_HSCROLL_POLICY:
      if (priv->hscroll_policy != g_value_get_enum (value))
        {
          priv->hscroll_policy = g_value_get_enum (value);
          ctk_widget_queue_resize (CTK_WIDGET (layout));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_VSCROLL_POLICY:
      if (priv->vscroll_policy != g_value_get_enum (value))
        {
          priv->vscroll_policy = g_value_get_enum (value);
          ctk_widget_queue_resize (CTK_WIDGET (layout));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_WIDTH:
      ctk_layout_set_size (layout, g_value_get_uint (value), priv->height);
      break;
    case PROP_HEIGHT:
      ctk_layout_set_size (layout, priv->width, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_layout_set_child_property (CtkContainer    *container,
                               CtkWidget       *child,
                               guint            property_id,
                               const GValue    *value,
                               GParamSpec      *pspec)
{
  switch (property_id)
    {
    case CHILD_PROP_X:
      ctk_layout_move_internal (CTK_LAYOUT (container),
                                child,
                                TRUE, g_value_get_int (value),
                                FALSE, 0);
      break;
    case CHILD_PROP_Y:
      ctk_layout_move_internal (CTK_LAYOUT (container),
                                child,
                                FALSE, 0,
                                TRUE, g_value_get_int (value));
      break;
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_layout_get_child_property (CtkContainer *container,
                               CtkWidget    *child,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  CtkLayoutChild *layout_child;

  layout_child = get_child (CTK_LAYOUT (container), child);
  
  switch (property_id)
    {
    case CHILD_PROP_X:
      g_value_set_int (value, layout_child->x);
      break;
    case CHILD_PROP_Y:
      g_value_set_int (value, layout_child->y);
      break;
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_layout_init (CtkLayout *layout)
{
  CtkLayoutPrivate *priv;

  layout->priv = ctk_layout_get_instance_private (layout);
  priv = layout->priv;

  priv->children = NULL;

  priv->width = 100;
  priv->height = 100;

  priv->hadjustment = NULL;
  priv->vadjustment = NULL;

  priv->bin_window = NULL;

  priv->scroll_x = 0;
  priv->scroll_y = 0;
  priv->visibility = CDK_VISIBILITY_PARTIAL;

  priv->freeze_count = 0;
}

/* Widget methods
 */
static void
set_background (CtkWidget *widget)
{
  CtkLayoutPrivate *priv;

  if (ctk_widget_get_realized (widget))
    {
      priv = CTK_LAYOUT (widget)->priv;

      /* We still need to call ctk_style_context_set_background() here for
       * CtkLayout, since subclasses like EelCanvas depend on a background to
       * be set since the beginning of the draw() implementation.
       * This should be revisited next time we have a major API break.
       */
      ctk_style_context_set_background (ctk_widget_get_style_context (widget), priv->bin_window);
    }
}

static void 
ctk_layout_realize (CtkWidget *widget)
{
  CtkLayout *layout = CTK_LAYOUT (widget);
  CtkLayoutPrivate *priv = layout->priv;
  CtkAllocation allocation;
  CdkWindow *window;
  CdkWindowAttr attributes;
  GList *tmp_list;
  gint attributes_mask;

  ctk_widget_set_realized (widget, TRUE);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = CDK_INPUT_OUTPUT;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.event_mask = CDK_VISIBILITY_NOTIFY_MASK;

  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;

  window = cdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_set_window (widget, window);
  ctk_widget_register_window (widget, window);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.x = - ctk_adjustment_get_value (priv->hadjustment),
  attributes.y = - ctk_adjustment_get_value (priv->vadjustment);
  attributes.width = MAX (priv->width, allocation.width);
  attributes.height = MAX (priv->height, allocation.height);
  attributes.event_mask = CDK_EXPOSURE_MASK | CDK_SCROLL_MASK |
                          CDK_SMOOTH_SCROLL_MASK | 
                          ctk_widget_get_events (widget);

  priv->bin_window = cdk_window_new (window,
                                     &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->bin_window);
  set_background (widget);

  tmp_list = priv->children;
  while (tmp_list)
    {
      CtkLayoutChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      ctk_widget_set_parent_window (child->widget, priv->bin_window);
    }
}

static void
ctk_layout_style_updated (CtkWidget *widget)
{
  CTK_WIDGET_CLASS (ctk_layout_parent_class)->style_updated (widget);

  set_background (widget);
}

static void
ctk_layout_map (CtkWidget *widget)
{
  CtkLayout *layout = CTK_LAYOUT (widget);
  CtkLayoutPrivate *priv = layout->priv;
  GList *tmp_list;

  ctk_widget_set_mapped (widget, TRUE);

  tmp_list = priv->children;
  while (tmp_list)
    {
      CtkLayoutChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      if (ctk_widget_get_visible (child->widget))
	{
	  if (!ctk_widget_get_mapped (child->widget))
	    ctk_widget_map (child->widget);
	}
    }

  cdk_window_show (priv->bin_window);
  cdk_window_show (ctk_widget_get_window (widget));
}

static void 
ctk_layout_unrealize (CtkWidget *widget)
{
  CtkLayout *layout = CTK_LAYOUT (widget);
  CtkLayoutPrivate *priv = layout->priv;

  ctk_widget_unregister_window (widget, priv->bin_window);
  cdk_window_destroy (priv->bin_window);
  priv->bin_window = NULL;

  CTK_WIDGET_CLASS (ctk_layout_parent_class)->unrealize (widget);
}

static void
ctk_layout_get_preferred_width (CtkWidget *widget G_GNUC_UNUSED,
                                gint      *minimum,
                                gint      *natural)
{
  *minimum = *natural = 0;
}

static void
ctk_layout_get_preferred_height (CtkWidget *widget G_GNUC_UNUSED,
                                 gint      *minimum,
                                 gint      *natural)
{
  *minimum = *natural = 0;
}

static void
ctk_layout_size_allocate (CtkWidget     *widget,
			  CtkAllocation *allocation)
{
  CtkLayout *layout = CTK_LAYOUT (widget);
  CtkLayoutPrivate *priv = layout->priv;
  GList *tmp_list;

  ctk_widget_set_allocation (widget, allocation);

  tmp_list = priv->children;

  while (tmp_list)
    {
      CtkLayoutChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      ctk_layout_allocate_child (layout, child);
    }

  if (ctk_widget_get_realized (widget))
    {
      cdk_window_move_resize (ctk_widget_get_window (widget),
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

      cdk_window_resize (priv->bin_window,
			 MAX (priv->width, allocation->width),
			 MAX (priv->height, allocation->height));
    }

  ctk_layout_set_hadjustment_values (layout);
  ctk_layout_set_vadjustment_values (layout);
}

static gboolean
ctk_layout_draw (CtkWidget *widget,
                 cairo_t   *cr)
{
  CtkLayout *layout = CTK_LAYOUT (widget);
  CtkLayoutPrivate *priv = layout->priv;

  if (ctk_cairo_should_draw_window (cr, priv->bin_window))
    CTK_WIDGET_CLASS (ctk_layout_parent_class)->draw (widget, cr);

  return FALSE;
}

/* Container methods
 */
static void
ctk_layout_add (CtkContainer *container,
		CtkWidget    *widget)
{
  ctk_layout_put (CTK_LAYOUT (container), widget, 0, 0);
}

static void
ctk_layout_remove (CtkContainer *container, 
		   CtkWidget    *widget)
{
  CtkLayout *layout = CTK_LAYOUT (container);
  CtkLayoutPrivate *priv = layout->priv;
  GList *tmp_list;
  CtkLayoutChild *child = NULL;

  tmp_list = priv->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      if (child->widget == widget)
	break;
      tmp_list = tmp_list->next;
    }

  if (tmp_list)
    {
      ctk_widget_unparent (widget);

      priv->children = g_list_remove_link (priv->children, tmp_list);
      g_list_free_1 (tmp_list);
      g_free (child);
    }
}

static void
ctk_layout_forall (CtkContainer *container,
		   gboolean      include_internals G_GNUC_UNUSED,
		   CtkCallback   callback,
		   gpointer      callback_data)
{
  CtkLayout *layout = CTK_LAYOUT (container);
  CtkLayoutPrivate *priv = layout->priv;
  CtkLayoutChild *child;
  GList *tmp_list;

  tmp_list = priv->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      tmp_list = tmp_list->next;

      (* callback) (child->widget, callback_data);
    }
}

/* Operations on children
 */

static void
ctk_layout_allocate_child (CtkLayout      *layout G_GNUC_UNUSED,
			   CtkLayoutChild *child)
{
  CtkAllocation allocation;
  CtkRequisition requisition;

  allocation.x = child->x;
  allocation.y = child->y;

  ctk_widget_get_preferred_size (child->widget, &requisition, NULL);
  allocation.width = requisition.width;
  allocation.height = requisition.height;
  
  ctk_widget_size_allocate (child->widget, &allocation);
}

/* Callbacks */

static void
ctk_layout_adjustment_changed (CtkAdjustment *adjustment G_GNUC_UNUSED,
			       CtkLayout     *layout)
{
  CtkLayoutPrivate *priv = layout->priv;

  if (priv->freeze_count)
    return;

  if (ctk_widget_get_realized (CTK_WIDGET (layout)))
    {
      cdk_window_move (priv->bin_window,
		       - ctk_adjustment_get_value (priv->hadjustment),
		       - ctk_adjustment_get_value (priv->vadjustment));
    }
}
