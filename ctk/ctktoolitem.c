/* ctktoolitem.c
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@gnome.org>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
 * Copyright (C) 2003 Soeren Sandmann <sandmann@daimi.au.dk>
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

#include "ctktoolitem.h"

#include <string.h>

#include "ctkmarshalers.h"
#include "ctktoolshell.h"
#include "ctkseparatormenuitem.h"
#include "ctksizerequest.h"
#include "ctkactivatable.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkwidgetprivate.h"

/**
 * SECTION:ctktoolitem
 * @short_description: The base class of widgets that can be added to CtkToolShell
 * @Title: CtkToolItem
 * @see_also: #CtkToolbar, #CtkToolButton, #CtkSeparatorToolItem
 *
 * #CtkToolItems are widgets that can appear on a toolbar. To
 * create a toolbar item that contain something else than a button, use
 * ctk_tool_item_new(). Use ctk_container_add() to add a child
 * widget to the tool item.
 *
 * For toolbar items that contain buttons, see the #CtkToolButton,
 * #CtkToggleToolButton and #CtkRadioToolButton classes.
 *
 * See the #CtkToolbar class for a description of the toolbar widget, and
 * #CtkToolShell for a description of the tool shell interface.
 */

/**
 * CtkToolItem:
 *
 * The CtkToolItem struct contains only private data.
 * It should only be accessed through the functions described below.
 */

enum {
  CREATE_MENU_PROXY,
  TOOLBAR_RECONFIGURED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_VISIBLE_HORIZONTAL,
  PROP_VISIBLE_VERTICAL,
  PROP_IS_IMPORTANT,

  /* activatable properties */
  PROP_ACTIVATABLE_RELATED_ACTION,
  PROP_ACTIVATABLE_USE_ACTION_APPEARANCE
};


struct _CtkToolItemPrivate
{
  gchar *tip_text;
  gchar *tip_private;

  guint visible_horizontal    : 1;
  guint visible_vertical      : 1;
  guint homogeneous           : 1;
  guint expand                : 1;
  guint use_drag_window       : 1;
  guint is_important          : 1;
  guint use_action_appearance : 1;

  CdkWindow *drag_window;
  gchar *menu_item_id;
  CtkWidget *menu_item;

  CtkAction *action;
};

static void ctk_tool_item_finalize     (GObject         *object);
static void ctk_tool_item_dispose      (GObject         *object);
static void ctk_tool_item_parent_set   (CtkWidget       *toolitem,
				        CtkWidget       *parent);
static void ctk_tool_item_set_property (GObject         *object,
					guint            prop_id,
					const GValue    *value,
					GParamSpec      *pspec);
static void ctk_tool_item_get_property (GObject         *object,
					guint            prop_id,
					GValue          *value,
					GParamSpec      *pspec);
static void ctk_tool_item_property_notify (GObject      *object,
					   GParamSpec   *pspec);
static void ctk_tool_item_realize       (CtkWidget      *widget);
static void ctk_tool_item_unrealize     (CtkWidget      *widget);
static void ctk_tool_item_map           (CtkWidget      *widget);
static void ctk_tool_item_unmap         (CtkWidget      *widget);
static void ctk_tool_item_get_preferred_width
                                        (CtkWidget      *widget,
                                         gint           *minimum,
                                         gint           *natural);
static void ctk_tool_item_get_preferred_height
                                        (CtkWidget      *widget,
                                         gint           *minimum,
                                         gint           *natural);
static void ctk_tool_item_size_allocate (CtkWidget      *widget,
					 CtkAllocation  *allocation);

static void ctk_tool_item_activatable_interface_init (CtkActivatableIface  *iface);
static void ctk_tool_item_update                     (CtkActivatable       *activatable,
						      CtkAction            *action,
						      const gchar          *property_name);
static void ctk_tool_item_sync_action_properties     (CtkActivatable       *activatable,
						      CtkAction            *action);
static void ctk_tool_item_set_related_action         (CtkToolItem          *item, 
						      CtkAction            *action);
static void ctk_tool_item_set_use_action_appearance  (CtkToolItem          *item, 
						      gboolean              use_appearance);

static guint toolitem_signals[LAST_SIGNAL] = { 0 };

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
G_DEFINE_TYPE_WITH_CODE (CtkToolItem, ctk_tool_item, CTK_TYPE_BIN,
                         G_ADD_PRIVATE (CtkToolItem)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIVATABLE,
						ctk_tool_item_activatable_interface_init))
G_GNUC_END_IGNORE_DEPRECATIONS;

static void
ctk_tool_item_class_init (CtkToolItemClass *klass)
{
  GObjectClass *object_class;
  CtkWidgetClass *widget_class;
  
  object_class = (GObjectClass *)klass;
  widget_class = (CtkWidgetClass *)klass;

  object_class->set_property = ctk_tool_item_set_property;
  object_class->get_property = ctk_tool_item_get_property;
  object_class->finalize     = ctk_tool_item_finalize;
  object_class->dispose      = ctk_tool_item_dispose;
  object_class->notify       = ctk_tool_item_property_notify;

  widget_class->realize       = ctk_tool_item_realize;
  widget_class->unrealize     = ctk_tool_item_unrealize;
  widget_class->map           = ctk_tool_item_map;
  widget_class->unmap         = ctk_tool_item_unmap;
  widget_class->get_preferred_width = ctk_tool_item_get_preferred_width;
  widget_class->get_preferred_height = ctk_tool_item_get_preferred_height;
  widget_class->size_allocate = ctk_tool_item_size_allocate;
  widget_class->parent_set    = ctk_tool_item_parent_set;

  ctk_container_class_handle_border_width (CTK_CONTAINER_CLASS (klass));

  klass->create_menu_proxy = _ctk_tool_item_create_menu_proxy;
  
  g_object_class_install_property (object_class,
				   PROP_VISIBLE_HORIZONTAL,
				   g_param_spec_boolean ("visible-horizontal",
							 P_("Visible when horizontal"),
							 P_("Whether the toolbar item is visible when the toolbar is in a horizontal orientation."),
							 TRUE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  g_object_class_install_property (object_class,
				   PROP_VISIBLE_VERTICAL,
				   g_param_spec_boolean ("visible-vertical",
							 P_("Visible when vertical"),
							 P_("Whether the toolbar item is visible when the toolbar is in a vertical orientation."),
							 TRUE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  g_object_class_install_property (object_class,
 				   PROP_IS_IMPORTANT,
 				   g_param_spec_boolean ("is-important",
 							 P_("Is important"),
 							 P_("Whether the toolbar item is considered important. When TRUE, toolbar buttons show text in CTK_TOOLBAR_BOTH_HORIZ mode"),
 							 FALSE,
 							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_override_property (object_class, PROP_ACTIVATABLE_RELATED_ACTION, "related-action");
  g_object_class_override_property (object_class, PROP_ACTIVATABLE_USE_ACTION_APPEARANCE, "use-action-appearance");


/**
 * CtkToolItem::create-menu-proxy:
 * @tool_item: the object the signal was emitted on
 *
 * This signal is emitted when the toolbar needs information from @tool_item
 * about whether the item should appear in the toolbar overflow menu. In
 * response the tool item should either
 * 
 * - call ctk_tool_item_set_proxy_menu_item() with a %NULL
 *   pointer and return %TRUE to indicate that the item should not appear
 *   in the overflow menu
 * 
 * - call ctk_tool_item_set_proxy_menu_item() with a new menu
 *   item and return %TRUE, or 
 *
 * - return %FALSE to indicate that the signal was not handled by the item.
 *   This means that the item will not appear in the overflow menu unless
 *   a later handler installs a menu item.
 *
 * The toolbar may cache the result of this signal. When the tool item changes
 * how it will respond to this signal it must call ctk_tool_item_rebuild_menu()
 * to invalidate the cache and ensure that the toolbar rebuilds its overflow
 * menu.
 *
 * Returns: %TRUE if the signal was handled, %FALSE if not
 **/
  toolitem_signals[CREATE_MENU_PROXY] =
    g_signal_new (I_("create-menu-proxy"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkToolItemClass, create_menu_proxy),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);

/**
 * CtkToolItem::toolbar-reconfigured:
 * @tool_item: the object the signal was emitted on
 *
 * This signal is emitted when some property of the toolbar that the
 * item is a child of changes. For custom subclasses of #CtkToolItem,
 * the default handler of this signal use the functions
 * - ctk_tool_shell_get_orientation()
 * - ctk_tool_shell_get_style()
 * - ctk_tool_shell_get_icon_size()
 * - ctk_tool_shell_get_relief_style()
 * to find out what the toolbar should look like and change
 * themselves accordingly.
 **/
  toolitem_signals[TOOLBAR_RECONFIGURED] =
    g_signal_new (I_("toolbar-reconfigured"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkToolItemClass, toolbar_reconfigured),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  ctk_widget_class_set_css_name (widget_class, "toolitem");
}

static void
ctk_tool_item_init (CtkToolItem *toolitem)
{
  ctk_widget_set_can_focus (CTK_WIDGET (toolitem), FALSE);

  toolitem->priv = ctk_tool_item_get_instance_private (toolitem);
  toolitem->priv->visible_horizontal = TRUE;
  toolitem->priv->visible_vertical = TRUE;
  toolitem->priv->homogeneous = FALSE;
  toolitem->priv->expand = FALSE;
  toolitem->priv->use_action_appearance = TRUE;
}

static void
ctk_tool_item_finalize (GObject *object)
{
  CtkToolItem *item = CTK_TOOL_ITEM (object);

  g_free (item->priv->menu_item_id);

  if (item->priv->menu_item)
    g_object_unref (item->priv->menu_item);

  G_OBJECT_CLASS (ctk_tool_item_parent_class)->finalize (object);
}

static void
ctk_tool_item_dispose (GObject *object)
{
  CtkToolItem *item = CTK_TOOL_ITEM (object);

  if (item->priv->action)
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_activatable_do_set_related_action (CTK_ACTIVATABLE (item), NULL);      
      G_GNUC_END_IGNORE_DEPRECATIONS;
      item->priv->action = NULL;
    }
  G_OBJECT_CLASS (ctk_tool_item_parent_class)->dispose (object);
}


static void
ctk_tool_item_parent_set (CtkWidget   *toolitem,
			  CtkWidget   *prev_parent)
{
  if (ctk_widget_get_parent (CTK_WIDGET (toolitem)) != NULL)
    ctk_tool_item_toolbar_reconfigured (CTK_TOOL_ITEM (toolitem));
}

static void
ctk_tool_item_set_property (GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
  CtkToolItem *toolitem = CTK_TOOL_ITEM (object);

  switch (prop_id)
    {
    case PROP_VISIBLE_HORIZONTAL:
      ctk_tool_item_set_visible_horizontal (toolitem, g_value_get_boolean (value));
      break;
    case PROP_VISIBLE_VERTICAL:
      ctk_tool_item_set_visible_vertical (toolitem, g_value_get_boolean (value));
      break;
    case PROP_IS_IMPORTANT:
      ctk_tool_item_set_is_important (toolitem, g_value_get_boolean (value));
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      ctk_tool_item_set_related_action (toolitem, g_value_get_object (value));
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
      ctk_tool_item_set_use_action_appearance (toolitem, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_tool_item_get_property (GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
  CtkToolItem *toolitem = CTK_TOOL_ITEM (object);

  switch (prop_id)
    {
    case PROP_VISIBLE_HORIZONTAL:
      g_value_set_boolean (value, toolitem->priv->visible_horizontal);
      break;
    case PROP_VISIBLE_VERTICAL:
      g_value_set_boolean (value, toolitem->priv->visible_vertical);
      break;
    case PROP_IS_IMPORTANT:
      g_value_set_boolean (value, toolitem->priv->is_important);
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      g_value_set_object (value, toolitem->priv->action);
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
      g_value_set_boolean (value, toolitem->priv->use_action_appearance);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_tool_item_property_notify (GObject    *object,
			       GParamSpec *pspec)
{
  CtkToolItem *tool_item = CTK_TOOL_ITEM (object);

  if (tool_item->priv->menu_item && strcmp (pspec->name, "sensitive") == 0)
    ctk_widget_set_sensitive (tool_item->priv->menu_item,
			      ctk_widget_get_sensitive (CTK_WIDGET (tool_item)));

  if (G_OBJECT_CLASS (ctk_tool_item_parent_class)->notify)
    G_OBJECT_CLASS (ctk_tool_item_parent_class)->notify (object, pspec);
}

static void
create_drag_window (CtkToolItem *toolitem)
{
  CtkAllocation allocation;
  CtkWidget *widget;
  CdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (toolitem->priv->use_drag_window == TRUE);

  widget = CTK_WIDGET (toolitem);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = CDK_INPUT_ONLY;
  attributes.event_mask = ctk_widget_get_events (widget);
  attributes.event_mask |= (CDK_BUTTON_PRESS_MASK | CDK_BUTTON_RELEASE_MASK);

  attributes_mask = CDK_WA_X | CDK_WA_Y;

  toolitem->priv->drag_window = cdk_window_new (ctk_widget_get_parent_window (widget),
					  &attributes, attributes_mask);
  ctk_widget_register_window (widget, toolitem->priv->drag_window);
}

static void
ctk_tool_item_realize (CtkWidget *widget)
{
  CtkToolItem *toolitem;
  CdkWindow *window;

  toolitem = CTK_TOOL_ITEM (widget);
  ctk_widget_set_realized (widget, TRUE);

  window = ctk_widget_get_parent_window (widget);
  ctk_widget_set_window (widget, window);
  g_object_ref (window);

  if (toolitem->priv->use_drag_window)
    create_drag_window(toolitem);
}

static void
destroy_drag_window (CtkToolItem *toolitem)
{
  if (toolitem->priv->drag_window)
    {
      ctk_widget_unregister_window (CTK_WIDGET (toolitem), toolitem->priv->drag_window);
      cdk_window_destroy (toolitem->priv->drag_window);
      toolitem->priv->drag_window = NULL;
    }
}

static void
ctk_tool_item_unrealize (CtkWidget *widget)
{
  CtkToolItem *toolitem;

  toolitem = CTK_TOOL_ITEM (widget);

  destroy_drag_window (toolitem);
  
  CTK_WIDGET_CLASS (ctk_tool_item_parent_class)->unrealize (widget);
}

static void
ctk_tool_item_map (CtkWidget *widget)
{
  CtkToolItem *toolitem;

  toolitem = CTK_TOOL_ITEM (widget);
  CTK_WIDGET_CLASS (ctk_tool_item_parent_class)->map (widget);
  if (toolitem->priv->drag_window)
    cdk_window_show (toolitem->priv->drag_window);
}

static void
ctk_tool_item_unmap (CtkWidget *widget)
{
  CtkToolItem *toolitem;

  toolitem = CTK_TOOL_ITEM (widget);
  if (toolitem->priv->drag_window)
    cdk_window_hide (toolitem->priv->drag_window);
  CTK_WIDGET_CLASS (ctk_tool_item_parent_class)->unmap (widget);
}

static void
ctk_tool_item_get_preferred_width (CtkWidget *widget,
                                   gint      *minimum,
                                   gint      *natural)
{
  CtkWidget *child;

  *minimum = *natural = 0;

  child = ctk_bin_get_child (CTK_BIN (widget));
  if (child && ctk_widget_get_visible (child))
    ctk_widget_get_preferred_width (child, minimum, natural);
}

static void
ctk_tool_item_get_preferred_height (CtkWidget *widget,
                                    gint      *minimum,
                                    gint      *natural)
{
  CtkWidget *child;

  *minimum = *natural = 0;

  child = ctk_bin_get_child (CTK_BIN (widget));
  if (child && ctk_widget_get_visible (child))
    ctk_widget_get_preferred_height (child, minimum, natural);
}

static void
ctk_tool_item_size_allocate (CtkWidget     *widget,
			     CtkAllocation *allocation)
{
  CtkToolItem *toolitem = CTK_TOOL_ITEM (widget);
  CtkAllocation child_allocation;
  CtkWidget *child;

  ctk_widget_set_allocation (widget, allocation);

  if (toolitem->priv->drag_window)
    cdk_window_move_resize (toolitem->priv->drag_window,
                            allocation->x,
                            allocation->y,
                            allocation->width,
                            allocation->height);

  child = ctk_bin_get_child (CTK_BIN (widget));
  if (child && ctk_widget_get_visible (child))
    {
      child_allocation.x = allocation->x;
      child_allocation.y = allocation->y;
      child_allocation.width = allocation->width;
      child_allocation.height = allocation->height;
      
      ctk_widget_size_allocate (child, &child_allocation);
    }

  _ctk_widget_set_simple_clip (widget, NULL);
}

gboolean
_ctk_tool_item_create_menu_proxy (CtkToolItem *item)
{
  CtkWidget *menu_item;
  gboolean visible_overflown;
  gboolean ret = FALSE;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;

  if (item->priv->action)
    {
      g_object_get (item->priv->action, "visible-overflown", &visible_overflown, NULL);
    
      if (visible_overflown)
	{
	  menu_item = ctk_action_create_menu_item (item->priv->action);

	  g_object_ref_sink (menu_item);
      	  ctk_tool_item_set_proxy_menu_item (item, "ctk-action-menu-item", menu_item);
	  g_object_unref (menu_item);
	}
      else
	ctk_tool_item_set_proxy_menu_item (item, "ctk-action-menu-item", NULL);

      ret = TRUE;
    }

  G_GNUC_END_IGNORE_DEPRECATIONS;

  return ret;
}

static void
ctk_tool_item_activatable_interface_init (CtkActivatableIface *iface)
{
  iface->update = ctk_tool_item_update;
  iface->sync_action_properties = ctk_tool_item_sync_action_properties;
}

static void
ctk_tool_item_update (CtkActivatable *activatable,
		      CtkAction      *action,
	     	      const gchar    *property_name)
{
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;

  if (strcmp (property_name, "visible") == 0)
    {
      if (ctk_action_is_visible (action))
	ctk_widget_show (CTK_WIDGET (activatable));
      else
	ctk_widget_hide (CTK_WIDGET (activatable));
    }
  else if (strcmp (property_name, "sensitive") == 0)
    ctk_widget_set_sensitive (CTK_WIDGET (activatable), ctk_action_is_sensitive (action));
  else if (strcmp (property_name, "tooltip") == 0)
    ctk_tool_item_set_tooltip_text (CTK_TOOL_ITEM (activatable),
				    ctk_action_get_tooltip (action));
  else if (strcmp (property_name, "visible-horizontal") == 0)
    ctk_tool_item_set_visible_horizontal (CTK_TOOL_ITEM (activatable),
					  ctk_action_get_visible_horizontal (action));
  else if (strcmp (property_name, "visible-vertical") == 0)
    ctk_tool_item_set_visible_vertical (CTK_TOOL_ITEM (activatable),
					ctk_action_get_visible_vertical (action));
  else if (strcmp (property_name, "is-important") == 0)
    ctk_tool_item_set_is_important (CTK_TOOL_ITEM (activatable),
				    ctk_action_get_is_important (action));

  G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
ctk_tool_item_sync_action_properties (CtkActivatable *activatable,
				      CtkAction      *action)
{
  if (!action)
    return;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;

  if (ctk_action_is_visible (action))
    ctk_widget_show (CTK_WIDGET (activatable));
  else
    ctk_widget_hide (CTK_WIDGET (activatable));
  
  ctk_widget_set_sensitive (CTK_WIDGET (activatable), ctk_action_is_sensitive (action));
  
  ctk_tool_item_set_tooltip_text (CTK_TOOL_ITEM (activatable),
				  ctk_action_get_tooltip (action));
  ctk_tool_item_set_visible_horizontal (CTK_TOOL_ITEM (activatable),
					ctk_action_get_visible_horizontal (action));
  ctk_tool_item_set_visible_vertical (CTK_TOOL_ITEM (activatable),
				      ctk_action_get_visible_vertical (action));
  ctk_tool_item_set_is_important (CTK_TOOL_ITEM (activatable),
				  ctk_action_get_is_important (action));

  G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
ctk_tool_item_set_related_action (CtkToolItem *item, 
				  CtkAction   *action)
{
  if (item->priv->action == action)
    return;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  ctk_activatable_do_set_related_action (CTK_ACTIVATABLE (item), action);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  item->priv->action = action;

  if (action)
    {
      ctk_tool_item_rebuild_menu (item);
    }
}

static void
ctk_tool_item_set_use_action_appearance (CtkToolItem *item,
					 gboolean     use_appearance)
{
  if (item->priv->use_action_appearance != use_appearance)
    {
      item->priv->use_action_appearance = use_appearance;

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_activatable_sync_action_properties (CTK_ACTIVATABLE (item), item->priv->action);
      G_GNUC_END_IGNORE_DEPRECATIONS;
    }
}


/**
 * ctk_tool_item_new:
 * 
 * Creates a new #CtkToolItem
 * 
 * Returns: the new #CtkToolItem
 * 
 * Since: 2.4
 **/
CtkToolItem *
ctk_tool_item_new (void)
{
  CtkToolItem *item;

  item = g_object_new (CTK_TYPE_TOOL_ITEM, NULL);

  return item;
}

/**
 * ctk_tool_item_get_ellipsize_mode:
 * @tool_item: a #CtkToolItem
 *
 * Returns the ellipsize mode used for @tool_item. Custom subclasses of
 * #CtkToolItem should call this function to find out how text should
 * be ellipsized.
 *
 * Returns: a #PangoEllipsizeMode indicating how text in @tool_item
 * should be ellipsized.
 *
 * Since: 2.20
 **/
PangoEllipsizeMode
ctk_tool_item_get_ellipsize_mode (CtkToolItem *tool_item)
{
  CtkWidget *parent;
  
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (tool_item), PANGO_ELLIPSIZE_NONE);

  parent = ctk_widget_get_parent (CTK_WIDGET (tool_item));
  if (!parent || !CTK_IS_TOOL_SHELL (parent))
    return PANGO_ELLIPSIZE_NONE;

  return ctk_tool_shell_get_ellipsize_mode (CTK_TOOL_SHELL (parent));
}

/**
 * ctk_tool_item_get_icon_size:
 * @tool_item: a #CtkToolItem
 * 
 * Returns the icon size used for @tool_item. Custom subclasses of
 * #CtkToolItem should call this function to find out what size icons
 * they should use.
 * 
 * Returns: (type int): a #CtkIconSize indicating the icon size
 * used for @tool_item
 * 
 * Since: 2.4
 **/
CtkIconSize
ctk_tool_item_get_icon_size (CtkToolItem *tool_item)
{
  CtkWidget *parent;

  g_return_val_if_fail (CTK_IS_TOOL_ITEM (tool_item), CTK_ICON_SIZE_LARGE_TOOLBAR);

  parent = ctk_widget_get_parent (CTK_WIDGET (tool_item));
  if (!parent || !CTK_IS_TOOL_SHELL (parent))
    return CTK_ICON_SIZE_LARGE_TOOLBAR;

  return ctk_tool_shell_get_icon_size (CTK_TOOL_SHELL (parent));
}

/**
 * ctk_tool_item_get_orientation:
 * @tool_item: a #CtkToolItem 
 * 
 * Returns the orientation used for @tool_item. Custom subclasses of
 * #CtkToolItem should call this function to find out what size icons
 * they should use.
 * 
 * Returns: a #CtkOrientation indicating the orientation
 * used for @tool_item
 * 
 * Since: 2.4
 **/
CtkOrientation
ctk_tool_item_get_orientation (CtkToolItem *tool_item)
{
  CtkWidget *parent;
  
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (tool_item), CTK_ORIENTATION_HORIZONTAL);

  parent = ctk_widget_get_parent (CTK_WIDGET (tool_item));
  if (!parent || !CTK_IS_TOOL_SHELL (parent))
    return CTK_ORIENTATION_HORIZONTAL;

  return ctk_tool_shell_get_orientation (CTK_TOOL_SHELL (parent));
}

/**
 * ctk_tool_item_get_toolbar_style:
 * @tool_item: a #CtkToolItem 
 * 
 * Returns the toolbar style used for @tool_item. Custom subclasses of
 * #CtkToolItem should call this function in the handler of the
 * CtkToolItem::toolbar_reconfigured signal to find out in what style
 * the toolbar is displayed and change themselves accordingly 
 *
 * Possibilities are:
 * - %CTK_TOOLBAR_BOTH, meaning the tool item should show
 *   both an icon and a label, stacked vertically
 * - %CTK_TOOLBAR_ICONS, meaning the toolbar shows only icons
 * - %CTK_TOOLBAR_TEXT, meaning the tool item should only show text
 * - %CTK_TOOLBAR_BOTH_HORIZ, meaning the tool item should show
 *   both an icon and a label, arranged horizontally
 * 
 * Returns: A #CtkToolbarStyle indicating the toolbar style used
 * for @tool_item.
 * 
 * Since: 2.4
 **/
CtkToolbarStyle
ctk_tool_item_get_toolbar_style (CtkToolItem *tool_item)
{
  CtkWidget *parent;
  
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (tool_item), CTK_TOOLBAR_ICONS);

  parent = ctk_widget_get_parent (CTK_WIDGET (tool_item));
  if (!parent || !CTK_IS_TOOL_SHELL (parent))
    return CTK_TOOLBAR_ICONS;

  return ctk_tool_shell_get_style (CTK_TOOL_SHELL (parent));
}

/**
 * ctk_tool_item_get_relief_style:
 * @tool_item: a #CtkToolItem 
 * 
 * Returns the relief style of @tool_item. See ctk_button_set_relief().
 * Custom subclasses of #CtkToolItem should call this function in the handler
 * of the #CtkToolItem::toolbar_reconfigured signal to find out the
 * relief style of buttons.
 * 
 * Returns: a #CtkReliefStyle indicating the relief style used
 * for @tool_item.
 * 
 * Since: 2.4
 **/
CtkReliefStyle 
ctk_tool_item_get_relief_style (CtkToolItem *tool_item)
{
  CtkWidget *parent;
  
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (tool_item), CTK_RELIEF_NONE);

  parent = ctk_widget_get_parent (CTK_WIDGET (tool_item));
  if (!parent || !CTK_IS_TOOL_SHELL (parent))
    return CTK_RELIEF_NONE;

  return ctk_tool_shell_get_relief_style (CTK_TOOL_SHELL (parent));
}

/**
 * ctk_tool_item_get_text_alignment:
 * @tool_item: a #CtkToolItem: 
 * 
 * Returns the text alignment used for @tool_item. Custom subclasses of
 * #CtkToolItem should call this function to find out how text should
 * be aligned.
 * 
 * Returns: a #gfloat indicating the horizontal text alignment
 * used for @tool_item
 * 
 * Since: 2.20
 **/
gfloat
ctk_tool_item_get_text_alignment (CtkToolItem *tool_item)
{
  CtkWidget *parent;
  
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (tool_item), CTK_ORIENTATION_HORIZONTAL);

  parent = ctk_widget_get_parent (CTK_WIDGET (tool_item));
  if (!parent || !CTK_IS_TOOL_SHELL (parent))
    return 0.5;

  return ctk_tool_shell_get_text_alignment (CTK_TOOL_SHELL (parent));
}

/**
 * ctk_tool_item_get_text_orientation:
 * @tool_item: a #CtkToolItem
 *
 * Returns the text orientation used for @tool_item. Custom subclasses of
 * #CtkToolItem should call this function to find out how text should
 * be orientated.
 *
 * Returns: a #CtkOrientation indicating the text orientation
 * used for @tool_item
 *
 * Since: 2.20
 */
CtkOrientation
ctk_tool_item_get_text_orientation (CtkToolItem *tool_item)
{
  CtkWidget *parent;
  
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (tool_item), CTK_ORIENTATION_HORIZONTAL);

  parent = ctk_widget_get_parent (CTK_WIDGET (tool_item));
  if (!parent || !CTK_IS_TOOL_SHELL (parent))
    return CTK_ORIENTATION_HORIZONTAL;

  return ctk_tool_shell_get_text_orientation (CTK_TOOL_SHELL (parent));
}

/**
 * ctk_tool_item_get_text_size_group:
 * @tool_item: a #CtkToolItem
 *
 * Returns the size group used for labels in @tool_item.
 * Custom subclasses of #CtkToolItem should call this function
 * and use the size group for labels.
 *
 * Returns: (transfer none): a #CtkSizeGroup
 *
 * Since: 2.20
 */
CtkSizeGroup *
ctk_tool_item_get_text_size_group (CtkToolItem *tool_item)
{
  CtkWidget *parent;
  
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (tool_item), NULL);

  parent = ctk_widget_get_parent (CTK_WIDGET (tool_item));
  if (!parent || !CTK_IS_TOOL_SHELL (parent))
    return NULL;

  return ctk_tool_shell_get_text_size_group (CTK_TOOL_SHELL (parent));
}

/**
 * ctk_tool_item_set_expand:
 * @tool_item: a #CtkToolItem
 * @expand: Whether @tool_item is allocated extra space
 *
 * Sets whether @tool_item is allocated extra space when there
 * is more room on the toolbar then needed for the items. The
 * effect is that the item gets bigger when the toolbar gets bigger
 * and smaller when the toolbar gets smaller.
 *
 * Since: 2.4
 */
void
ctk_tool_item_set_expand (CtkToolItem *tool_item,
			  gboolean     expand)
{
  g_return_if_fail (CTK_IS_TOOL_ITEM (tool_item));
    
  expand = expand != FALSE;

  if (tool_item->priv->expand != expand)
    {
      tool_item->priv->expand = expand;
      ctk_widget_child_notify (CTK_WIDGET (tool_item), "expand");
      ctk_widget_queue_resize (CTK_WIDGET (tool_item));
    }
}

/**
 * ctk_tool_item_get_expand:
 * @tool_item: a #CtkToolItem 
 * 
 * Returns whether @tool_item is allocated extra space.
 * See ctk_tool_item_set_expand().
 * 
 * Returns: %TRUE if @tool_item is allocated extra space.
 * 
 * Since: 2.4
 **/
gboolean
ctk_tool_item_get_expand (CtkToolItem *tool_item)
{
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (tool_item), FALSE);

  return tool_item->priv->expand;
}

/**
 * ctk_tool_item_set_homogeneous:
 * @tool_item: a #CtkToolItem 
 * @homogeneous: whether @tool_item is the same size as other homogeneous items
 * 
 * Sets whether @tool_item is to be allocated the same size as other
 * homogeneous items. The effect is that all homogeneous items will have
 * the same width as the widest of the items.
 * 
 * Since: 2.4
 **/
void
ctk_tool_item_set_homogeneous (CtkToolItem *tool_item,
			       gboolean     homogeneous)
{
  g_return_if_fail (CTK_IS_TOOL_ITEM (tool_item));
    
  homogeneous = homogeneous != FALSE;

  if (tool_item->priv->homogeneous != homogeneous)
    {
      tool_item->priv->homogeneous = homogeneous;
      ctk_widget_child_notify (CTK_WIDGET (tool_item), "homogeneous");
      ctk_widget_queue_resize (CTK_WIDGET (tool_item));
    }
}

/**
 * ctk_tool_item_get_homogeneous:
 * @tool_item: a #CtkToolItem 
 * 
 * Returns whether @tool_item is the same size as other homogeneous
 * items. See ctk_tool_item_set_homogeneous().
 * 
 * Returns: %TRUE if the item is the same size as other homogeneous
 * items.
 * 
 * Since: 2.4
 **/
gboolean
ctk_tool_item_get_homogeneous (CtkToolItem *tool_item)
{
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (tool_item), FALSE);

  return tool_item->priv->homogeneous;
}

/**
 * ctk_tool_item_get_is_important:
 * @tool_item: a #CtkToolItem
 * 
 * Returns whether @tool_item is considered important. See
 * ctk_tool_item_set_is_important()
 * 
 * Returns: %TRUE if @tool_item is considered important.
 * 
 * Since: 2.4
 **/
gboolean
ctk_tool_item_get_is_important (CtkToolItem *tool_item)
{
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (tool_item), FALSE);

  return tool_item->priv->is_important;
}

/**
 * ctk_tool_item_set_is_important:
 * @tool_item: a #CtkToolItem
 * @is_important: whether the tool item should be considered important
 * 
 * Sets whether @tool_item should be considered important. The #CtkToolButton
 * class uses this property to determine whether to show or hide its label
 * when the toolbar style is %CTK_TOOLBAR_BOTH_HORIZ. The result is that
 * only tool buttons with the “is_important” property set have labels, an
 * effect known as “priority text”
 * 
 * Since: 2.4
 **/
void
ctk_tool_item_set_is_important (CtkToolItem *tool_item, gboolean is_important)
{
  g_return_if_fail (CTK_IS_TOOL_ITEM (tool_item));

  is_important = is_important != FALSE;

  if (is_important != tool_item->priv->is_important)
    {
      tool_item->priv->is_important = is_important;

      ctk_widget_queue_resize (CTK_WIDGET (tool_item));

      g_object_notify (G_OBJECT (tool_item), "is-important");
    }
}

/**
 * ctk_tool_item_set_tooltip_text:
 * @tool_item: a #CtkToolItem 
 * @text: text to be used as tooltip for @tool_item
 *
 * Sets the text to be displayed as tooltip on the item.
 * See ctk_widget_set_tooltip_text().
 *
 * Since: 2.12
 **/
void
ctk_tool_item_set_tooltip_text (CtkToolItem *tool_item,
			        const gchar *text)
{
  CtkWidget *child;

  g_return_if_fail (CTK_IS_TOOL_ITEM (tool_item));

  child = ctk_bin_get_child (CTK_BIN (tool_item));
  if (child)
    ctk_widget_set_tooltip_text (child, text);
}

/**
 * ctk_tool_item_set_tooltip_markup:
 * @tool_item: a #CtkToolItem 
 * @markup: markup text to be used as tooltip for @tool_item
 *
 * Sets the markup text to be displayed as tooltip on the item.
 * See ctk_widget_set_tooltip_markup().
 *
 * Since: 2.12
 **/
void
ctk_tool_item_set_tooltip_markup (CtkToolItem *tool_item,
				  const gchar *markup)
{
  CtkWidget *child;

  g_return_if_fail (CTK_IS_TOOL_ITEM (tool_item));

  child = ctk_bin_get_child (CTK_BIN (tool_item));
  if (child)
    ctk_widget_set_tooltip_markup (child, markup);
}

/**
 * ctk_tool_item_set_use_drag_window:
 * @tool_item: a #CtkToolItem 
 * @use_drag_window: Whether @tool_item has a drag window.
 * 
 * Sets whether @tool_item has a drag window. When %TRUE the
 * toolitem can be used as a drag source through ctk_drag_source_set().
 * When @tool_item has a drag window it will intercept all events,
 * even those that would otherwise be sent to a child of @tool_item.
 * 
 * Since: 2.4
 **/
void
ctk_tool_item_set_use_drag_window (CtkToolItem *toolitem,
				   gboolean     use_drag_window)
{
  g_return_if_fail (CTK_IS_TOOL_ITEM (toolitem));

  use_drag_window = use_drag_window != FALSE;

  if (toolitem->priv->use_drag_window != use_drag_window)
    {
      toolitem->priv->use_drag_window = use_drag_window;
      
      if (use_drag_window)
	{
	  if (!toolitem->priv->drag_window &&
              ctk_widget_get_realized (CTK_WIDGET (toolitem)))
	    {
	      create_drag_window(toolitem);
	      if (ctk_widget_get_mapped (CTK_WIDGET (toolitem)))
		cdk_window_show (toolitem->priv->drag_window);
	    }
	}
      else
	{
	  destroy_drag_window (toolitem);
	}
    }
}

/**
 * ctk_tool_item_get_use_drag_window:
 * @tool_item: a #CtkToolItem 
 * 
 * Returns whether @tool_item has a drag window. See
 * ctk_tool_item_set_use_drag_window().
 * 
 * Returns: %TRUE if @tool_item uses a drag window.
 * 
 * Since: 2.4
 **/
gboolean
ctk_tool_item_get_use_drag_window (CtkToolItem *toolitem)
{
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (toolitem), FALSE);

  return toolitem->priv->use_drag_window;
}

/**
 * ctk_tool_item_set_visible_horizontal:
 * @tool_item: a #CtkToolItem
 * @visible_horizontal: Whether @tool_item is visible when in horizontal mode
 * 
 * Sets whether @tool_item is visible when the toolbar is docked horizontally.
 * 
 * Since: 2.4
 **/
void
ctk_tool_item_set_visible_horizontal (CtkToolItem *toolitem,
				      gboolean     visible_horizontal)
{
  g_return_if_fail (CTK_IS_TOOL_ITEM (toolitem));

  visible_horizontal = visible_horizontal != FALSE;

  if (toolitem->priv->visible_horizontal != visible_horizontal)
    {
      toolitem->priv->visible_horizontal = visible_horizontal;

      g_object_notify (G_OBJECT (toolitem), "visible-horizontal");

      ctk_widget_queue_resize (CTK_WIDGET (toolitem));
    }
}

/**
 * ctk_tool_item_get_visible_horizontal:
 * @tool_item: a #CtkToolItem 
 * 
 * Returns whether the @tool_item is visible on toolbars that are
 * docked horizontally.
 * 
 * Returns: %TRUE if @tool_item is visible on toolbars that are
 * docked horizontally.
 * 
 * Since: 2.4
 **/
gboolean
ctk_tool_item_get_visible_horizontal (CtkToolItem *toolitem)
{
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (toolitem), FALSE);

  return toolitem->priv->visible_horizontal;
}

/**
 * ctk_tool_item_set_visible_vertical:
 * @tool_item: a #CtkToolItem 
 * @visible_vertical: whether @tool_item is visible when the toolbar
 * is in vertical mode
 *
 * Sets whether @tool_item is visible when the toolbar is docked
 * vertically. Some tool items, such as text entries, are too wide to be
 * useful on a vertically docked toolbar. If @visible_vertical is %FALSE
 * @tool_item will not appear on toolbars that are docked vertically.
 * 
 * Since: 2.4
 **/
void
ctk_tool_item_set_visible_vertical (CtkToolItem *toolitem,
				    gboolean     visible_vertical)
{
  g_return_if_fail (CTK_IS_TOOL_ITEM (toolitem));

  visible_vertical = visible_vertical != FALSE;

  if (toolitem->priv->visible_vertical != visible_vertical)
    {
      toolitem->priv->visible_vertical = visible_vertical;

      g_object_notify (G_OBJECT (toolitem), "visible-vertical");

      ctk_widget_queue_resize (CTK_WIDGET (toolitem));
    }
}

/**
 * ctk_tool_item_get_visible_vertical:
 * @tool_item: a #CtkToolItem 
 * 
 * Returns whether @tool_item is visible when the toolbar is docked vertically.
 * See ctk_tool_item_set_visible_vertical().
 * 
 * Returns: Whether @tool_item is visible when the toolbar is docked vertically
 * 
 * Since: 2.4
 **/
gboolean
ctk_tool_item_get_visible_vertical (CtkToolItem *toolitem)
{
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (toolitem), FALSE);

  return toolitem->priv->visible_vertical;
}

/**
 * ctk_tool_item_retrieve_proxy_menu_item:
 * @tool_item: a #CtkToolItem 
 * 
 * Returns the #CtkMenuItem that was last set by
 * ctk_tool_item_set_proxy_menu_item(), ie. the #CtkMenuItem
 * that is going to appear in the overflow menu.
 *
 * Returns: (transfer none): The #CtkMenuItem that is going to appear in the
 * overflow menu for @tool_item.
 *
 * Since: 2.4
 **/
CtkWidget *
ctk_tool_item_retrieve_proxy_menu_item (CtkToolItem *tool_item)
{
  gboolean retval;
  
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (tool_item), NULL);

  g_signal_emit (tool_item, toolitem_signals[CREATE_MENU_PROXY], 0,
		 &retval);
  
  return tool_item->priv->menu_item;
}

/**
 * ctk_tool_item_get_proxy_menu_item:
 * @tool_item: a #CtkToolItem
 * @menu_item_id: a string used to identify the menu item
 *
 * If @menu_item_id matches the string passed to
 * ctk_tool_item_set_proxy_menu_item() return the corresponding #CtkMenuItem.
 *
 * Custom subclasses of #CtkToolItem should use this function to
 * update their menu item when the #CtkToolItem changes. That the
 * @menu_item_ids must match ensures that a #CtkToolItem
 * will not inadvertently change a menu item that they did not create.
 *
 * Returns: (transfer none) (nullable): The #CtkMenuItem passed to
 *     ctk_tool_item_set_proxy_menu_item(), if the @menu_item_ids
 *     match.
 *
 * Since: 2.4
 **/
CtkWidget *
ctk_tool_item_get_proxy_menu_item (CtkToolItem *tool_item,
				   const gchar *menu_item_id)
{
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (tool_item), NULL);
  g_return_val_if_fail (menu_item_id != NULL, NULL);

  if (tool_item->priv->menu_item_id && strcmp (tool_item->priv->menu_item_id, menu_item_id) == 0)
    return tool_item->priv->menu_item;

  return NULL;
}

/**
 * ctk_tool_item_rebuild_menu:
 * @tool_item: a #CtkToolItem
 *
 * Calling this function signals to the toolbar that the
 * overflow menu item for @tool_item has changed. If the
 * overflow menu is visible when this function it called,
 * the menu will be rebuilt.
 *
 * The function must be called when the tool item changes what it
 * will do in response to the #CtkToolItem::create-menu-proxy signal.
 *
 * Since: 2.6
 */
void
ctk_tool_item_rebuild_menu (CtkToolItem *tool_item)
{
  CtkWidget *parent;
  CtkWidget *widget;

  g_return_if_fail (CTK_IS_TOOL_ITEM (tool_item));

  widget = CTK_WIDGET (tool_item);

  parent = ctk_widget_get_parent (widget);
  if (CTK_IS_TOOL_SHELL (parent))
    ctk_tool_shell_rebuild_menu (CTK_TOOL_SHELL (parent));
}

/**
 * ctk_tool_item_set_proxy_menu_item:
 * @tool_item: a #CtkToolItem
 * @menu_item_id: a string used to identify @menu_item
 * @menu_item: (nullable): a #CtkMenuItem to use in the overflow menu, or %NULL
 * 
 * Sets the #CtkMenuItem used in the toolbar overflow menu. The
 * @menu_item_id is used to identify the caller of this function and
 * should also be used with ctk_tool_item_get_proxy_menu_item().
 *
 * See also #CtkToolItem::create-menu-proxy.
 * 
 * Since: 2.4
 **/
void
ctk_tool_item_set_proxy_menu_item (CtkToolItem *tool_item,
				   const gchar *menu_item_id,
				   CtkWidget   *menu_item)
{
  g_return_if_fail (CTK_IS_TOOL_ITEM (tool_item));
  g_return_if_fail (menu_item == NULL || CTK_IS_MENU_ITEM (menu_item));
  g_return_if_fail (menu_item_id != NULL);

  g_free (tool_item->priv->menu_item_id);
      
  tool_item->priv->menu_item_id = g_strdup (menu_item_id);

  if (tool_item->priv->menu_item != menu_item)
    {
      if (tool_item->priv->menu_item)
	g_object_unref (tool_item->priv->menu_item);
      
      if (menu_item)
	{
	  g_object_ref_sink (menu_item);

	  ctk_widget_set_sensitive (menu_item,
				    ctk_widget_get_sensitive (CTK_WIDGET (tool_item)));
	}
      
      tool_item->priv->menu_item = menu_item;
    }
}

/**
 * ctk_tool_item_toolbar_reconfigured:
 * @tool_item: a #CtkToolItem
 *
 * Emits the signal #CtkToolItem::toolbar_reconfigured on @tool_item.
 * #CtkToolbar and other #CtkToolShell implementations use this function
 * to notify children, when some aspect of their configuration changes.
 *
 * Since: 2.14
 **/
void
ctk_tool_item_toolbar_reconfigured (CtkToolItem *tool_item)
{
  /* The slightely inaccurate name "ctk_tool_item_toolbar_reconfigured" was
   * choosen over "ctk_tool_item_tool_shell_reconfigured", since the function
   * emits the "toolbar-reconfigured" signal, not "tool-shell-reconfigured".
   * It's not possible to rename the signal, and emitting another name than
   * indicated by the function name would be quite confusing. That's the
   * price of providing stable APIs.
   */
  g_return_if_fail (CTK_IS_TOOL_ITEM (tool_item));

  g_signal_emit (tool_item, toolitem_signals[TOOLBAR_RECONFIGURED], 0);
  
  if (tool_item->priv->drag_window)
    cdk_window_raise (tool_item->priv->drag_window);

  ctk_widget_queue_resize (CTK_WIDGET (tool_item));
}
