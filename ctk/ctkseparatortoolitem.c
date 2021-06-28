/* ctkseparatortoolitem.c
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@gnome.org>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
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

#include "config.h"
#include "ctkseparatormenuitem.h"
#include "ctkseparatortoolitem.h"
#include "ctkintl.h"
#include "ctktoolbarprivate.h"
#include "ctkprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkcsscustomgadgetprivate.h"

/**
 * SECTION:ctkseparatortoolitem
 * @Short_description: A toolbar item that separates groups of other
 *   toolbar items
 * @Title: CtkSeparatorToolItem
 * @See_also: #CtkToolbar, #CtkRadioToolButton
 *
 * A #CtkSeparatorToolItem is a #CtkToolItem that separates groups of other
 * #CtkToolItems. Depending on the theme, a #CtkSeparatorToolItem will
 * often look like a vertical line on horizontally docked toolbars.
 *
 * If the #CtkToolbar child property “expand” is %TRUE and the property
 * #CtkSeparatorToolItem:draw is %FALSE, a #CtkSeparatorToolItem will act as
 * a “spring” that forces other items to the ends of the toolbar.
 *
 * Use ctk_separator_tool_item_new() to create a new #CtkSeparatorToolItem.
 *
 * # CSS nodes
 *
 * CtkSeparatorToolItem has a single CSS node with name separator.
 */

#define MENU_ID "ctk-separator-tool-item-menu-id"

struct _CtkSeparatorToolItemPrivate
{
  CtkCssGadget *gadget;
  GdkWindow *event_window;
  guint draw : 1;
};

enum {
  PROP_0,
  PROP_DRAW
};

static gboolean ctk_separator_tool_item_create_menu_proxy (CtkToolItem               *item);
static void     ctk_separator_tool_item_set_property      (GObject                   *object,
                                                           guint                      prop_id,
                                                           const GValue              *value,
                                                           GParamSpec                *pspec);
static void     ctk_separator_tool_item_get_property      (GObject                   *object,
                                                           guint                      prop_id,
                                                           GValue                    *value,
                                                           GParamSpec                *pspec);
static void     ctk_separator_tool_item_get_preferred_width (CtkWidget               *widget,
                                                           gint                      *minimum,
                                                           gint                      *natural);
static void     ctk_separator_tool_item_get_preferred_height (CtkWidget              *widget,
                                                           gint                      *minimum,
                                                           gint                      *natural);
static void     ctk_separator_tool_item_size_allocate     (CtkWidget                 *widget,
                                                           CtkAllocation             *allocation);
static gboolean ctk_separator_tool_item_draw              (CtkWidget                 *widget,
                                                           cairo_t                   *cr);
static void     ctk_separator_tool_item_add               (CtkContainer              *container,
                                                           CtkWidget                 *child);
static void     ctk_separator_tool_item_realize           (CtkWidget                 *widget);
static void     ctk_separator_tool_item_unrealize         (CtkWidget                 *widget);
static void     ctk_separator_tool_item_map               (CtkWidget                 *widget);
static void     ctk_separator_tool_item_unmap             (CtkWidget                 *widget);
static gboolean ctk_separator_tool_item_button_event      (CtkWidget                 *widget,
                                                           GdkEventButton            *event);
static gboolean ctk_separator_tool_item_motion_event      (CtkWidget                 *widget,
                                                           GdkEventMotion            *event);

G_DEFINE_TYPE_WITH_PRIVATE (CtkSeparatorToolItem, ctk_separator_tool_item, CTK_TYPE_TOOL_ITEM)

static void
ctk_separator_tool_item_finalize (GObject *object)
{
  CtkSeparatorToolItem *item = CTK_SEPARATOR_TOOL_ITEM (object);

  g_clear_object (&item->priv->gadget);

  G_OBJECT_CLASS (ctk_separator_tool_item_parent_class)->finalize (object);
}

static void
ctk_separator_tool_item_class_init (CtkSeparatorToolItemClass *class)
{
  GObjectClass *object_class;
  CtkContainerClass *container_class;
  CtkToolItemClass *toolitem_class;
  CtkWidgetClass *widget_class;
  
  object_class = (GObjectClass *)class;
  container_class = (CtkContainerClass *)class;
  toolitem_class = (CtkToolItemClass *)class;
  widget_class = (CtkWidgetClass *)class;

  object_class->set_property = ctk_separator_tool_item_set_property;
  object_class->get_property = ctk_separator_tool_item_get_property;
  object_class->finalize = ctk_separator_tool_item_finalize;

  widget_class->get_preferred_width = ctk_separator_tool_item_get_preferred_width;
  widget_class->get_preferred_height = ctk_separator_tool_item_get_preferred_height;
  widget_class->size_allocate = ctk_separator_tool_item_size_allocate;
  widget_class->draw = ctk_separator_tool_item_draw;
  widget_class->realize = ctk_separator_tool_item_realize;
  widget_class->unrealize = ctk_separator_tool_item_unrealize;
  widget_class->map = ctk_separator_tool_item_map;
  widget_class->unmap = ctk_separator_tool_item_unmap;
  widget_class->button_press_event = ctk_separator_tool_item_button_event;
  widget_class->button_release_event = ctk_separator_tool_item_button_event;
  widget_class->motion_notify_event = ctk_separator_tool_item_motion_event;

  toolitem_class->create_menu_proxy = ctk_separator_tool_item_create_menu_proxy;
  
  container_class->add = ctk_separator_tool_item_add;
  
  g_object_class_install_property (object_class,
                                   PROP_DRAW,
                                   g_param_spec_boolean ("draw",
                                                         P_("Draw"),
                                                         P_("Whether the separator is drawn, or just blank"),
                                                         TRUE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  ctk_widget_class_set_css_name (widget_class, "separator");
}

static void
ctk_separator_tool_item_init (CtkSeparatorToolItem *separator_item)
{
  CtkSeparatorToolItemPrivate *priv;
  CtkWidget *widget;
  CtkCssNode *widget_node;

  widget = CTK_WIDGET (separator_item);
  priv = separator_item->priv = ctk_separator_tool_item_get_instance_private (separator_item);
  priv->draw = TRUE;

  ctk_widget_set_has_window (widget, FALSE);

  widget_node = ctk_widget_get_css_node (widget);
  separator_item->priv->gadget =
    ctk_css_custom_gadget_new_for_node (widget_node,
                                        widget,
                                        NULL, NULL, NULL,
                                        NULL, NULL);
}

static void
ctk_separator_tool_item_add (CtkContainer *container,
                             CtkWidget    *child)
{
  g_warning ("attempt to add a child to an CtkSeparatorToolItem");
}

static gboolean
ctk_separator_tool_item_create_menu_proxy (CtkToolItem *item)
{
  CtkWidget *menu_item = NULL;
  
  menu_item = ctk_separator_menu_item_new ();
  
  ctk_tool_item_set_proxy_menu_item (item, MENU_ID, menu_item);
  
  return TRUE;
}

static void
ctk_separator_tool_item_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  CtkSeparatorToolItem *item = CTK_SEPARATOR_TOOL_ITEM (object);
  
  switch (prop_id)
    {
    case PROP_DRAW:
      ctk_separator_tool_item_set_draw (item, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_separator_tool_item_get_property (GObject      *object,
                                      guint         prop_id,
                                      GValue       *value,
                                      GParamSpec   *pspec)
{
  CtkSeparatorToolItem *item = CTK_SEPARATOR_TOOL_ITEM (object);
  
  switch (prop_id)
    {
    case PROP_DRAW:
      g_value_set_boolean (value, ctk_separator_tool_item_get_draw (item));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_separator_tool_item_get_preferred_width (CtkWidget *widget,
                                             gint      *minimum,
                                             gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_SEPARATOR_TOOL_ITEM (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_separator_tool_item_get_preferred_height (CtkWidget *widget,
                                              gint      *minimum,
                                              gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_SEPARATOR_TOOL_ITEM (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_separator_tool_item_size_allocate (CtkWidget     *widget,
                                       CtkAllocation *allocation)
{
  CtkSeparatorToolItem *separator = CTK_SEPARATOR_TOOL_ITEM (widget);
  CtkSeparatorToolItemPrivate *priv = separator->priv;
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  if (ctk_widget_get_realized (widget))
    cdk_window_move_resize (priv->event_window,
                            allocation->x,
                            allocation->y,
                            allocation->width,
                            allocation->height);

  ctk_css_gadget_allocate (priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_separator_tool_item_realize (CtkWidget *widget)
{
  CtkAllocation allocation;
  CtkSeparatorToolItem *separator = CTK_SEPARATOR_TOOL_ITEM (widget);
  CtkSeparatorToolItemPrivate *priv = separator->priv;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;

  ctk_widget_set_realized (widget, TRUE);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.event_mask = ctk_widget_get_events (widget) |
                          GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK;
  attributes_mask = GDK_WA_X | GDK_WA_Y;

  window = ctk_widget_get_parent_window (widget);
  ctk_widget_set_window (widget, window);
  g_object_ref (window);

  priv->event_window = cdk_window_new (ctk_widget_get_parent_window (widget),
                                       &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->event_window);
}

static void
ctk_separator_tool_item_unrealize (CtkWidget *widget)
{
  CtkSeparatorToolItem *separator = CTK_SEPARATOR_TOOL_ITEM (widget);
  CtkSeparatorToolItemPrivate *priv = separator->priv;

  if (priv->event_window)
    {
      ctk_widget_unregister_window (widget, priv->event_window);
      cdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  CTK_WIDGET_CLASS (ctk_separator_tool_item_parent_class)->unrealize (widget);
}

static void
ctk_separator_tool_item_map (CtkWidget *widget)
{
  CtkSeparatorToolItem *separator = CTK_SEPARATOR_TOOL_ITEM (widget);
  CtkSeparatorToolItemPrivate *priv = separator->priv;

  CTK_WIDGET_CLASS (ctk_separator_tool_item_parent_class)->map (widget);

  if (priv->event_window)
    cdk_window_show (priv->event_window);
}

static void
ctk_separator_tool_item_unmap (CtkWidget *widget)
{
  CtkSeparatorToolItem *separator = CTK_SEPARATOR_TOOL_ITEM (widget);
  CtkSeparatorToolItemPrivate *priv = separator->priv;

  if (priv->event_window)
    cdk_window_hide (priv->event_window);

  CTK_WIDGET_CLASS (ctk_separator_tool_item_parent_class)->unmap (widget);
}

static gboolean
ctk_separator_tool_item_motion_event (CtkWidget      *widget,
                                      GdkEventMotion *event)
{
  CtkSeparatorToolItem *separator = CTK_SEPARATOR_TOOL_ITEM (widget);
  CtkSeparatorToolItemPrivate *priv = separator->priv;

  /* We want window dragging to work on empty toolbar areas,
   * so we only eat button events on visible separators
   */
  return priv->draw;
}

static gboolean
ctk_separator_tool_item_button_event (CtkWidget      *widget,
                                      GdkEventButton *event)
{
  CtkSeparatorToolItem *separator = CTK_SEPARATOR_TOOL_ITEM (widget);
  CtkSeparatorToolItemPrivate *priv = separator->priv;

  /* We want window dragging to work on empty toolbar areas,
   * so we only eat button events on visible separators
   */
  return priv->draw;
}

static gboolean
ctk_separator_tool_item_draw (CtkWidget *widget,
                              cairo_t   *cr)
{
  if (CTK_SEPARATOR_TOOL_ITEM (widget)->priv->draw)
    ctk_css_gadget_draw (CTK_SEPARATOR_TOOL_ITEM (widget)->priv->gadget, cr);

  return FALSE;
}

/**
 * ctk_separator_tool_item_new:
 * 
 * Create a new #CtkSeparatorToolItem
 * 
 * Returns: the new #CtkSeparatorToolItem
 * 
 * Since: 2.4
 */
CtkToolItem *
ctk_separator_tool_item_new (void)
{
  CtkToolItem *self;
  
  self = g_object_new (CTK_TYPE_SEPARATOR_TOOL_ITEM,
                       NULL);
  
  return self;
}

/**
 * ctk_separator_tool_item_get_draw:
 * @item: a #CtkSeparatorToolItem 
 * 
 * Returns whether @item is drawn as a line, or just blank. 
 * See ctk_separator_tool_item_set_draw().
 * 
 * Returns: %TRUE if @item is drawn as a line, or just blank.
 * 
 * Since: 2.4
 */
gboolean
ctk_separator_tool_item_get_draw (CtkSeparatorToolItem *item)
{
  g_return_val_if_fail (CTK_IS_SEPARATOR_TOOL_ITEM (item), FALSE);
  
  return item->priv->draw;
}

/**
 * ctk_separator_tool_item_set_draw:
 * @item: a #CtkSeparatorToolItem
 * @draw: whether @item is drawn as a vertical line
 * 
 * Whether @item is drawn as a vertical line, or just blank.
 * Setting this to %FALSE along with ctk_tool_item_set_expand() is useful
 * to create an item that forces following items to the end of the toolbar.
 * 
 * Since: 2.4
 */
void
ctk_separator_tool_item_set_draw (CtkSeparatorToolItem *item,
                                  gboolean              draw)
{
  g_return_if_fail (CTK_IS_SEPARATOR_TOOL_ITEM (item));

  draw = draw != FALSE;

  if (draw != item->priv->draw)
    {
      item->priv->draw = draw;
      if (draw)
        ctk_css_gadget_remove_class (item->priv->gadget, "invisible");
      else
        ctk_css_gadget_add_class (item->priv->gadget, "invisible");

      ctk_widget_queue_draw (CTK_WIDGET (item));

      g_object_notify (G_OBJECT (item), "draw");
    }
}
