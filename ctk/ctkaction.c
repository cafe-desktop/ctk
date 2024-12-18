/*
 * CTK - The GIMP Toolkit
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Author: James Henstridge <james@daa.com.au>
 *
 * Modified by the CTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

/**
 * SECTION:ctkaction
 * @Short_description: An action which can be triggered by a menu or toolbar item
 * @Title: CtkAction
 * @See_also: #CtkActionGroup, #CtkUIManager, #CtkActivatable
 *
 * Actions represent operations that the user can be perform, along with
 * some information how it should be presented in the interface. Each action
 * provides methods to create icons, menu items and toolbar items
 * representing itself.
 *
 * As well as the callback that is called when the action gets activated,
 * the following also gets associated with the action:
 *
 * - a name (not translated, for path lookup)
 *
 * - a label (translated, for display)
 *
 * - an accelerator
 *
 * - whether label indicates a stock id
 *
 * - a tooltip (optional, translated)
 *
 * - a toolbar label (optional, shorter than label)
 *
 *
 * The action will also have some state information:
 *
 * - visible (shown/hidden)
 *
 * - sensitive (enabled/disabled)
 *
 * Apart from regular actions, there are [toggle actions][CtkToggleAction],
 * which can be toggled between two states and
 * [radio actions][CtkRadioAction], of which only one in a group
 * can be in the “active” state. Other actions can be implemented as #CtkAction
 * subclasses.
 *
 * Each action can have one or more proxy widgets. To act as an action proxy,
 * widget needs to implement #CtkActivatable interface. Proxies mirror the state
 * of the action and should change when the action’s state changes. Properties
 * that are always mirrored by proxies are #CtkAction:sensitive and
 * #CtkAction:visible. #CtkAction:gicon, #CtkAction:icon-name, #CtkAction:label,
 * #CtkAction:short-label and #CtkAction:stock-id properties are only mirorred
 * if proxy widget has #CtkActivatable:use-action-appearance property set to
 * %TRUE.
 *
 * When the proxy is activated, it should activate its action.
 */

#include "config.h"

#include "ctkaction.h"
#include "ctkactiongroup.h"
#include "ctkaccellabel.h"
#include "ctkbutton.h"
#include "ctkiconfactory.h"
#include "ctkimage.h"
#include "ctkimagemenuitem.h"
#include "ctkintl.h"
#include "ctklabel.h"
#include "ctkmarshalers.h"
#include "ctkmenuitem.h"
#include "ctkstock.h"
#include "ctktearoffmenuitem.h"
#include "ctktoolbutton.h"
#include "ctktoolbar.h"
#include "ctkprivate.h"
#include "ctkbuildable.h"
#include "ctkactivatable.h"


struct _CtkActionPrivate 
{
  const gchar *name; /* interned */
  gchar *label;
  gchar *short_label;
  gchar *tooltip;
  gchar *stock_id; /* stock icon */
  gchar *icon_name; /* themed icon */
  GIcon *gicon;

  guint sensitive          : 1;
  guint visible            : 1;
  guint label_set          : 1; /* these two used so we can set label */
  guint short_label_set    : 1; /* based on stock id */
  guint visible_horizontal : 1;
  guint visible_vertical   : 1;
  guint is_important       : 1;
  guint hide_if_empty      : 1;
  guint visible_overflown  : 1;
  guint always_show_image  : 1;
  guint recursion_guard    : 1;
  guint activate_blocked   : 1;

  /* accelerator */
  guint          accel_count;
  CtkAccelGroup *accel_group;
  GClosure      *accel_closure;
  GQuark         accel_quark;

  CtkActionGroup *action_group;

  /* list of proxy widgets */
  GSList *proxies;
};

enum 
{
  ACTIVATE,
  LAST_SIGNAL
};

enum 
{
  PROP_0,
  PROP_NAME,
  PROP_LABEL,
  PROP_SHORT_LABEL,
  PROP_TOOLTIP,
  PROP_STOCK_ID,
  PROP_ICON_NAME,
  PROP_GICON,
  PROP_VISIBLE_HORIZONTAL,
  PROP_VISIBLE_VERTICAL,
  PROP_VISIBLE_OVERFLOWN,
  PROP_IS_IMPORTANT,
  PROP_HIDE_IF_EMPTY,
  PROP_SENSITIVE,
  PROP_VISIBLE,
  PROP_ACTION_GROUP,
  PROP_ALWAYS_SHOW_IMAGE
};

/* CtkBuildable */
static void ctk_action_buildable_init             (CtkBuildableIface *iface);
static void ctk_action_buildable_set_name         (CtkBuildable *buildable,
						   const gchar  *name);
static const gchar* ctk_action_buildable_get_name (CtkBuildable *buildable);

G_DEFINE_TYPE_WITH_CODE (CtkAction, ctk_action, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (CtkAction)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
						ctk_action_buildable_init))

static void ctk_action_finalize     (GObject *object);
static void ctk_action_set_property (GObject         *object,
				     guint            prop_id,
				     const GValue    *value,
				     GParamSpec      *pspec);
static void ctk_action_get_property (GObject         *object,
				     guint            prop_id,
				     GValue          *value,
				     GParamSpec      *pspec);
static void ctk_action_set_action_group (CtkAction	*action,
					 CtkActionGroup *action_group);

static CtkWidget *create_menu_item    (CtkAction *action);
static CtkWidget *create_tool_item    (CtkAction *action);
static void       connect_proxy       (CtkAction *action,
				       CtkWidget *proxy);
static void       disconnect_proxy    (CtkAction *action,
				       CtkWidget *proxy);
 
static void       closure_accel_activate (GClosure     *closure,
					  GValue       *return_value,
					  guint         n_param_values,
					  const GValue *param_values,
					  gpointer      invocation_hint,
					  gpointer      marshal_data);

static guint         action_signals[LAST_SIGNAL] = { 0 };


static void
ctk_action_class_init (CtkActionClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize     = ctk_action_finalize;
  gobject_class->set_property = ctk_action_set_property;
  gobject_class->get_property = ctk_action_get_property;

  klass->activate = NULL;

  klass->create_menu_item  = create_menu_item;
  klass->create_tool_item  = create_tool_item;
  klass->create_menu       = NULL;
  klass->menu_item_type    = CTK_TYPE_IMAGE_MENU_ITEM;
  klass->toolbar_item_type = CTK_TYPE_TOOL_BUTTON;
  klass->connect_proxy    = connect_proxy;
  klass->disconnect_proxy = disconnect_proxy;

  /**
   * CtkAction:name:
   *
   * A unique name for the action.
   */
  g_object_class_install_property (gobject_class,
				   PROP_NAME,
				   g_param_spec_string ("name",
							P_("Name"),
							P_("A unique name for the action."),
							NULL,
							CTK_PARAM_READWRITE | 
							G_PARAM_CONSTRUCT_ONLY));

  /**
   * CtkAction:label:
   *
   * The label used for menu items and buttons that activate
   * this action. If the label is %NULL, CTK+ uses the stock 
   * label specified via the stock-id property.
   *
   * This is an appearance property and thus only applies if 
   * #CtkActivatable:use-action-appearance is %TRUE.
   */
  g_object_class_install_property (gobject_class,
				   PROP_LABEL,
				   g_param_spec_string ("label",
							P_("Label"),
							P_("The label used for menu items and buttons "
							   "that activate this action."),
							NULL,
							CTK_PARAM_READWRITE));

  /**
   * CtkAction:short-label:
   *
   * A shorter label that may be used on toolbar buttons.
   *
   * This is an appearance property and thus only applies if 
   * #CtkActivatable:use-action-appearance is %TRUE.
   */
  g_object_class_install_property (gobject_class,
				   PROP_SHORT_LABEL,
				   g_param_spec_string ("short-label",
							P_("Short label"),
							P_("A shorter label that may be used on toolbar buttons."),
							NULL,
							CTK_PARAM_READWRITE));


  /**
   * CtkAction:tooltip:
   *
   * A tooltip for this action.
   */
  g_object_class_install_property (gobject_class,
				   PROP_TOOLTIP,
				   g_param_spec_string ("tooltip",
							P_("Tooltip"),
							P_("A tooltip for this action."),
							NULL,
							CTK_PARAM_READWRITE));

  /**
   * CtkAction:stock-id:
   *
   * The stock icon displayed in widgets representing this action.
   *
   * This is an appearance property and thus only applies if 
   * #CtkActivatable:use-action-appearance is %TRUE.
   */
  g_object_class_install_property (gobject_class,
				   PROP_STOCK_ID,
				   g_param_spec_string ("stock-id",
							P_("Stock Icon"),
							P_("The stock icon displayed in widgets representing "
							   "this action."),
							NULL,
							CTK_PARAM_READWRITE));
  /**
   * CtkAction:gicon:
   *
   * The #GIcon displayed in the #CtkAction.
   *
   * Note that the stock icon is preferred, if the #CtkAction:stock-id 
   * property holds the id of an existing stock icon.
   *
   * This is an appearance property and thus only applies if 
   * #CtkActivatable:use-action-appearance is %TRUE.
   *
   * Since: 2.16
   */
  g_object_class_install_property (gobject_class,
				   PROP_GICON,
				   g_param_spec_object ("gicon",
							P_("GIcon"),
							P_("The GIcon being displayed"),
							G_TYPE_ICON,
 							CTK_PARAM_READWRITE));							
  /**
   * CtkAction:icon-name:
   *
   * The name of the icon from the icon theme. 
   * 
   * Note that the stock icon is preferred, if the #CtkAction:stock-id 
   * property holds the id of an existing stock icon, and the #GIcon is
   * preferred if the #CtkAction:gicon property is set. 
   *
   * This is an appearance property and thus only applies if 
   * #CtkActivatable:use-action-appearance is %TRUE.
   *
   * Since: 2.10
   */
  g_object_class_install_property (gobject_class,
				   PROP_ICON_NAME,
				   g_param_spec_string ("icon-name",
							P_("Icon Name"),
							P_("The name of the icon from the icon theme"),
							NULL,
 							CTK_PARAM_READWRITE));

  /**
   * CtkAction:visible-horizontal:
   *
   * Whether the toolbar item is visible when the toolbar is in a horizontal orientation.
   */
  g_object_class_install_property (gobject_class,
				   PROP_VISIBLE_HORIZONTAL,
				   g_param_spec_boolean ("visible-horizontal",
							 P_("Visible when horizontal"),
							 P_("Whether the toolbar item is visible when the toolbar "
							    "is in a horizontal orientation."),
							 TRUE,
							 CTK_PARAM_READWRITE));
  /**
   * CtkAction:visible-overflown:
   *
   * When %TRUE, toolitem proxies for this action are represented in the 
   * toolbar overflow menu.
   *
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class,
				   PROP_VISIBLE_OVERFLOWN,
				   g_param_spec_boolean ("visible-overflown",
							 P_("Visible when overflown"),
							 P_("When TRUE, toolitem proxies for this action "
							    "are represented in the toolbar overflow menu."),
							 TRUE,
							 CTK_PARAM_READWRITE));

  /**
   * CtkAction:visible-vertical:
   *
   * Whether the toolbar item is visible when the toolbar is in a vertical orientation.
   */
  g_object_class_install_property (gobject_class,
				   PROP_VISIBLE_VERTICAL,
				   g_param_spec_boolean ("visible-vertical",
							 P_("Visible when vertical"),
							 P_("Whether the toolbar item is visible when the toolbar "
							    "is in a vertical orientation."),
							 TRUE,
							 CTK_PARAM_READWRITE));
  /**
   * CtkAction:is-important:
   *
   * Whether the action is considered important. When TRUE, toolitem
   * proxies for this action show text in CTK_TOOLBAR_BOTH_HORIZ mode.
   */
  g_object_class_install_property (gobject_class,
				   PROP_IS_IMPORTANT,
				   g_param_spec_boolean ("is-important",
							 P_("Is important"),
							 P_("Whether the action is considered important. "
							    "When TRUE, toolitem proxies for this action "
							    "show text in CTK_TOOLBAR_BOTH_HORIZ mode."),
							 FALSE,
							 CTK_PARAM_READWRITE));
  /**
   * CtkAction:hide-if-empty:
   *
   * When TRUE, empty menu proxies for this action are hidden.
   */
  g_object_class_install_property (gobject_class,
				   PROP_HIDE_IF_EMPTY,
				   g_param_spec_boolean ("hide-if-empty",
							 P_("Hide if empty"),
							 P_("When TRUE, empty menu proxies for this action are hidden."),
							 TRUE,
							 CTK_PARAM_READWRITE));
  /**
   * CtkAction:sensitive:
   *
   * Whether the action is enabled.
   */
  g_object_class_install_property (gobject_class,
				   PROP_SENSITIVE,
				   g_param_spec_boolean ("sensitive",
							 P_("Sensitive"),
							 P_("Whether the action is enabled."),
							 TRUE,
							 CTK_PARAM_READWRITE));
  /**
   * CtkAction:visible:
   *
   * Whether the action is visible.
   */
  g_object_class_install_property (gobject_class,
				   PROP_VISIBLE,
				   g_param_spec_boolean ("visible",
							 P_("Visible"),
							 P_("Whether the action is visible."),
							 TRUE,
							 CTK_PARAM_READWRITE));
  /**
   * CtkAction:action-group:
   *
   * The CtkActionGroup this CtkAction is associated with, or NULL
   * (for internal use).
   */
  g_object_class_install_property (gobject_class,
				   PROP_ACTION_GROUP,
				   g_param_spec_object ("action-group",
							 P_("Action Group"),
							 P_("The CtkActionGroup this CtkAction is associated with, or NULL (for internal use)."),
							 CTK_TYPE_ACTION_GROUP,
							 CTK_PARAM_READWRITE));

  /**
   * CtkAction:always-show-image:
   *
   * If %TRUE, the action's menu item proxies will ignore the #CtkSettings:ctk-menu-images 
   * setting and always show their image, if available.
   *
   * Use this property if the menu item would be useless or hard to use
   * without their image. 
   *
   * Since: 2.20
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ALWAYS_SHOW_IMAGE,
                                   g_param_spec_boolean ("always-show-image",
                                                         P_("Always show image"),
                                                         P_("Whether the image will always be shown"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  /**
   * CtkAction::activate:
   * @action: the #CtkAction
   *
   * The "activate" signal is emitted when the action is activated.
   *
   * Since: 2.4
   */
  action_signals[ACTIVATE] =
    g_signal_new (I_("activate"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		  G_STRUCT_OFFSET (CtkActionClass, activate),  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
}


static void
ctk_action_init (CtkAction *action)
{
  action->private_data = ctk_action_get_instance_private (action);

  action->private_data->name = NULL;
  action->private_data->label = NULL;
  action->private_data->short_label = NULL;
  action->private_data->tooltip = NULL;
  action->private_data->stock_id = NULL;
  action->private_data->icon_name = NULL;
  action->private_data->visible_horizontal = TRUE;
  action->private_data->visible_vertical   = TRUE;
  action->private_data->visible_overflown  = TRUE;
  action->private_data->is_important = FALSE;
  action->private_data->hide_if_empty = TRUE;
  action->private_data->always_show_image = FALSE;
  action->private_data->activate_blocked = FALSE;

  action->private_data->sensitive = TRUE;
  action->private_data->visible = TRUE;

  action->private_data->label_set = FALSE;
  action->private_data->short_label_set = FALSE;

  action->private_data->accel_count = 0;
  action->private_data->accel_group = NULL;
  action->private_data->accel_quark = 0;
  action->private_data->accel_closure = 
    g_closure_new_object (sizeof (GClosure), G_OBJECT (action));
  g_closure_set_marshal (action->private_data->accel_closure, 
			 closure_accel_activate);
  g_closure_ref (action->private_data->accel_closure);
  g_closure_sink (action->private_data->accel_closure);

  action->private_data->action_group = NULL;

  action->private_data->proxies = NULL;
  action->private_data->gicon = NULL;  
}

static void
ctk_action_buildable_init (CtkBuildableIface *iface)
{
  iface->set_name = ctk_action_buildable_set_name;
  iface->get_name = ctk_action_buildable_get_name;
}

static void
ctk_action_buildable_set_name (CtkBuildable *buildable,
			       const gchar  *name)
{
  CtkAction *action = CTK_ACTION (buildable);

  action->private_data->name = g_intern_string (name);
}

static const gchar *
ctk_action_buildable_get_name (CtkBuildable *buildable)
{
  CtkAction *action = CTK_ACTION (buildable);

  return action->private_data->name;
}

/**
 * ctk_action_new:
 * @name: A unique name for the action
 * @label: (allow-none): the label displayed in menu items and on buttons,
 *         or %NULL
 * @tooltip: (allow-none): a tooltip for the action, or %NULL
 * @stock_id: (allow-none): the stock icon to display in widgets representing
 *            the action, or %NULL
 *
 * Creates a new #CtkAction object. To add the action to a
 * #CtkActionGroup and set the accelerator for the action,
 * call ctk_action_group_add_action_with_accel().
 * See the [UI Definition section][XML-UI] for information on allowed action
 * names.
 *
 * Returns: a new #CtkAction
 *
 * Since: 2.4
 */
CtkAction *
ctk_action_new (const gchar *name,
		const gchar *label,
		const gchar *tooltip,
		const gchar *stock_id)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (CTK_TYPE_ACTION,
                       "name", name,
		       "label", label,
		       "tooltip", tooltip,
		       "stock-id", stock_id,
		       NULL);
}

static void
ctk_action_finalize (GObject *object)
{
  CtkAction *action;
  action = CTK_ACTION (object);

  g_free (action->private_data->label);
  g_free (action->private_data->short_label);
  g_free (action->private_data->tooltip);
  g_free (action->private_data->stock_id);
  g_free (action->private_data->icon_name);
  
  if (action->private_data->gicon)
    g_object_unref (action->private_data->gicon);

  g_closure_unref (action->private_data->accel_closure);
  if (action->private_data->accel_group)
    g_object_unref (action->private_data->accel_group);

  G_OBJECT_CLASS (ctk_action_parent_class)->finalize (object);  
}

static void
ctk_action_set_property (GObject         *object,
			 guint            prop_id,
			 const GValue    *value,
			 GParamSpec      *pspec)
{
  CtkAction *action;
  
  action = CTK_ACTION (object);

  switch (prop_id)
    {
    case PROP_NAME:
      action->private_data->name = g_intern_string (g_value_get_string (value));
      break;
    case PROP_LABEL:
      ctk_action_set_label (action, g_value_get_string (value));
      break;
    case PROP_SHORT_LABEL:
      ctk_action_set_short_label (action, g_value_get_string (value));
      break;
    case PROP_TOOLTIP:
      ctk_action_set_tooltip (action, g_value_get_string (value));
      break;
    case PROP_STOCK_ID:
      ctk_action_set_stock_id (action, g_value_get_string (value));
      break;
    case PROP_GICON:
      ctk_action_set_gicon (action, g_value_get_object (value));
      break;
    case PROP_ICON_NAME:
      ctk_action_set_icon_name (action, g_value_get_string (value));
      break;
    case PROP_VISIBLE_HORIZONTAL:
      ctk_action_set_visible_horizontal (action, g_value_get_boolean (value));
      break;
    case PROP_VISIBLE_VERTICAL:
      ctk_action_set_visible_vertical (action, g_value_get_boolean (value));
      break;
    case PROP_VISIBLE_OVERFLOWN:
      action->private_data->visible_overflown = g_value_get_boolean (value);
      break;
    case PROP_IS_IMPORTANT:
      ctk_action_set_is_important (action, g_value_get_boolean (value));
      break;
    case PROP_HIDE_IF_EMPTY:
      action->private_data->hide_if_empty = g_value_get_boolean (value);
      break;
    case PROP_SENSITIVE:
      ctk_action_set_sensitive (action, g_value_get_boolean (value));
      break;
    case PROP_VISIBLE:
      ctk_action_set_visible (action, g_value_get_boolean (value));
      break;
    case PROP_ACTION_GROUP:
      ctk_action_set_action_group (action, g_value_get_object (value));
      break;
    case PROP_ALWAYS_SHOW_IMAGE:
      ctk_action_set_always_show_image (action, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_action_get_property (GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
  CtkAction *action;

  action = CTK_ACTION (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_static_string (value, action->private_data->name);
      break;
    case PROP_LABEL:
      g_value_set_string (value, action->private_data->label);
      break;
    case PROP_SHORT_LABEL:
      g_value_set_string (value, action->private_data->short_label);
      break;
    case PROP_TOOLTIP:
      g_value_set_string (value, action->private_data->tooltip);
      break;
    case PROP_STOCK_ID:
      g_value_set_string (value, action->private_data->stock_id);
      break;
    case PROP_ICON_NAME:
      g_value_set_string (value, action->private_data->icon_name);
      break;
    case PROP_GICON:
      g_value_set_object (value, action->private_data->gicon);
      break;
    case PROP_VISIBLE_HORIZONTAL:
      g_value_set_boolean (value, action->private_data->visible_horizontal);
      break;
    case PROP_VISIBLE_VERTICAL:
      g_value_set_boolean (value, action->private_data->visible_vertical);
      break;
    case PROP_VISIBLE_OVERFLOWN:
      g_value_set_boolean (value, action->private_data->visible_overflown);
      break;
    case PROP_IS_IMPORTANT:
      g_value_set_boolean (value, action->private_data->is_important);
      break;
    case PROP_HIDE_IF_EMPTY:
      g_value_set_boolean (value, action->private_data->hide_if_empty);
      break;
    case PROP_SENSITIVE:
      g_value_set_boolean (value, action->private_data->sensitive);
      break;
    case PROP_VISIBLE:
      g_value_set_boolean (value, action->private_data->visible);
      break;
    case PROP_ACTION_GROUP:
      g_value_set_object (value, action->private_data->action_group);
      break;
    case PROP_ALWAYS_SHOW_IMAGE:
      g_value_set_boolean (value, action->private_data->always_show_image);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static CtkWidget *
create_menu_item (CtkAction *action)
{
  GType menu_item_type;

  menu_item_type = CTK_ACTION_GET_CLASS (action)->menu_item_type;

  return g_object_new (menu_item_type, NULL);
}

static CtkWidget *
create_tool_item (CtkAction *action)
{
  GType toolbar_item_type;

  toolbar_item_type = CTK_ACTION_GET_CLASS (action)->toolbar_item_type;

  return g_object_new (toolbar_item_type, NULL);
}

static void
remove_proxy (CtkAction *action,
	      CtkWidget *proxy)
{
  g_object_unref (proxy);
  action->private_data->proxies = g_slist_remove (action->private_data->proxies, proxy);
}

static void
connect_proxy (CtkAction *action,
	       CtkWidget *proxy)
{
  action->private_data->proxies = g_slist_prepend (action->private_data->proxies, proxy);

  g_object_ref_sink (proxy);

  if (action->private_data->action_group)
    _ctk_action_group_emit_connect_proxy (action->private_data->action_group, action, proxy);

}

static void
disconnect_proxy (CtkAction *action,
		  CtkWidget *proxy)
{
  remove_proxy (action, proxy);

  if (action->private_data->action_group)
    _ctk_action_group_emit_disconnect_proxy (action->private_data->action_group, action, proxy);
}

/**
 * _ctk_action_sync_menu_visible:
 * @action: (allow-none): a #CtkAction, or %NULL to determine the action from @proxy
 * @proxy: a proxy menu item
 * @empty: whether the submenu attached to @proxy is empty
 * 
 * Updates the visibility of @proxy from the visibility of @action
 * according to the following rules:

 * - if @action is invisible, @proxy is too
 *
 * - if @empty is %TRUE, hide @proxy unless the “hide-if-empty”
 *   property of @action indicates otherwise
 *
 * This function is used in the implementation of #CtkUIManager.
 **/
void
_ctk_action_sync_menu_visible (CtkAction *action,
			       CtkWidget *proxy,
			       gboolean   empty)
{
  gboolean visible = TRUE;
  gboolean hide_if_empty = TRUE;

  g_return_if_fail (CTK_IS_MENU_ITEM (proxy));
  g_return_if_fail (action == NULL || CTK_IS_ACTION (action));

  if (action == NULL)
    action = ctk_activatable_get_related_action (CTK_ACTIVATABLE (proxy));

  if (action)
    {
      /* a CtkMenu for a <popup/> doesn't have to have an action */
      visible = ctk_action_is_visible (action);
      hide_if_empty = action->private_data->hide_if_empty;
    }

  if (visible && !(empty && hide_if_empty))
    ctk_widget_show (proxy);
  else
    ctk_widget_hide (proxy);
}

void
_ctk_action_emit_activate (CtkAction *action)
{
  CtkActionGroup *group = action->private_data->action_group;

  if (group != NULL)
    {
      g_object_ref (action);
      g_object_ref (group);
      _ctk_action_group_emit_pre_activate (group, action);
    }

  g_signal_emit (action, action_signals[ACTIVATE], 0);

  if (group != NULL)
    {
      _ctk_action_group_emit_post_activate (group, action);
      g_object_unref (group);
      g_object_unref (action);
    }
}

/**
 * ctk_action_activate:
 * @action: the action object
 *
 * Emits the “activate” signal on the specified action, if it isn't
 * insensitive. This gets called by the proxy widgets when they get 
 * activated.
 *
 * It can also be used to manually activate an action.
 *
 * Since: 2.4
 */
void
ctk_action_activate (CtkAction *action)
{
  g_return_if_fail (CTK_IS_ACTION (action));
  
  if (action->private_data->activate_blocked)
    return;

  if (ctk_action_is_sensitive (action))
    _ctk_action_emit_activate (action);
}

/**
 * ctk_action_block_activate:
 * @action: a #CtkAction
 *
 * Disable activation signals from the action 
 *
 * This is needed when updating the state of your proxy
 * #CtkActivatable widget could result in calling ctk_action_activate(),
 * this is a convenience function to avoid recursing in those
 * cases (updating toggle state for instance).
 *
 * Since: 2.16
 */
void
ctk_action_block_activate (CtkAction *action)
{
  g_return_if_fail (CTK_IS_ACTION (action));

  action->private_data->activate_blocked = TRUE;
}

/**
 * ctk_action_unblock_activate:
 * @action: a #CtkAction
 *
 * Reenable activation signals from the action 
 *
 * Since: 2.16
 */
void
ctk_action_unblock_activate (CtkAction *action)
{
  g_return_if_fail (CTK_IS_ACTION (action));

  action->private_data->activate_blocked = FALSE;
}

/**
 * ctk_action_create_icon:
 * @action: the action object
 * @icon_size: (type int): the size of the icon (#CtkIconSize) that should
 *      be created.
 *
 * This function is intended for use by action implementations to
 * create icons displayed in the proxy widgets.
 *
 * Returns: (transfer none): a widget that displays the icon for this action.
 *
 * Since: 2.4
 */
CtkWidget *
ctk_action_create_icon (CtkAction *action, CtkIconSize icon_size)
{
  CtkWidget *widget = NULL;

  g_return_val_if_fail (CTK_IS_ACTION (action), NULL);

  if (action->private_data->stock_id &&
      ctk_icon_factory_lookup_default (action->private_data->stock_id))
    widget = ctk_image_new_from_stock (action->private_data->stock_id, icon_size);
  else if (action->private_data->gicon)
    widget = ctk_image_new_from_gicon (action->private_data->gicon, icon_size);
  else if (action->private_data->icon_name)
    widget = ctk_image_new_from_icon_name (action->private_data->icon_name, icon_size);

  return widget;
}

/**
 * ctk_action_create_menu_item:
 * @action: the action object
 *
 * Creates a menu item widget that proxies for the given action.
 *
 * Returns: (transfer none): a menu item connected to the action.
 *
 * Since: 2.4
 */
CtkWidget *
ctk_action_create_menu_item (CtkAction *action)
{
  CtkWidget *menu_item;

  g_return_val_if_fail (CTK_IS_ACTION (action), NULL);

  menu_item = CTK_ACTION_GET_CLASS (action)->create_menu_item (action);

  ctk_activatable_set_use_action_appearance (CTK_ACTIVATABLE (menu_item), TRUE);
  ctk_activatable_set_related_action (CTK_ACTIVATABLE (menu_item), action);

  return menu_item;
}

/**
 * ctk_action_create_tool_item:
 * @action: the action object
 *
 * Creates a toolbar item widget that proxies for the given action.
 *
 * Returns: (transfer none): a toolbar item connected to the action.
 *
 * Since: 2.4
 */
CtkWidget *
ctk_action_create_tool_item (CtkAction *action)
{
  CtkWidget *button;

  g_return_val_if_fail (CTK_IS_ACTION (action), NULL);

  button = CTK_ACTION_GET_CLASS (action)->create_tool_item (action);

  ctk_activatable_set_use_action_appearance (CTK_ACTIVATABLE (button), TRUE);
  ctk_activatable_set_related_action (CTK_ACTIVATABLE (button), action);

  return button;
}

void
_ctk_action_add_to_proxy_list (CtkAction     *action,
			       CtkWidget     *proxy)
{
  g_return_if_fail (CTK_IS_ACTION (action));
  g_return_if_fail (CTK_IS_WIDGET (proxy));
 
  CTK_ACTION_GET_CLASS (action)->connect_proxy (action, proxy);
}

void
_ctk_action_remove_from_proxy_list (CtkAction     *action,
				    CtkWidget     *proxy)
{
  g_return_if_fail (CTK_IS_ACTION (action));
  g_return_if_fail (CTK_IS_WIDGET (proxy));

  CTK_ACTION_GET_CLASS (action)->disconnect_proxy (action, proxy);
}

/**
 * ctk_action_get_proxies:
 * @action: the action object
 * 
 * Returns the proxy widgets for an action.
 * See also ctk_activatable_get_related_action().
 *
 * Returns: (element-type CtkWidget) (transfer none): a #GSList of proxy widgets. The list is owned by CTK+
 * and must not be modified.
 *
 * Since: 2.4
 **/
GSList*
ctk_action_get_proxies (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), NULL);

  return action->private_data->proxies;
}

/**
 * ctk_action_get_name:
 * @action: the action object
 * 
 * Returns the name of the action.
 * 
 * Returns: the name of the action. The string belongs to CTK+ and should not
 *   be freed.
 *
 * Since: 2.4
 **/
const gchar *
ctk_action_get_name (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), NULL);

  return action->private_data->name;
}

/**
 * ctk_action_is_sensitive:
 * @action: the action object
 * 
 * Returns whether the action is effectively sensitive.
 *
 * Returns: %TRUE if the action and its associated action group 
 * are both sensitive.
 *
 * Since: 2.4
 **/
gboolean
ctk_action_is_sensitive (CtkAction *action)
{
  CtkActionPrivate *priv;
  g_return_val_if_fail (CTK_IS_ACTION (action), FALSE);

  priv = action->private_data;
  return priv->sensitive &&
    (priv->action_group == NULL ||
     ctk_action_group_get_sensitive (priv->action_group));
}

/**
 * ctk_action_get_sensitive:
 * @action: the action object
 * 
 * Returns whether the action itself is sensitive. Note that this doesn’t 
 * necessarily mean effective sensitivity. See ctk_action_is_sensitive() 
 * for that.
 *
 * Returns: %TRUE if the action itself is sensitive.
 *
 * Since: 2.4
 **/
gboolean
ctk_action_get_sensitive (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), FALSE);

  return action->private_data->sensitive;
}

/**
 * ctk_action_set_sensitive:
 * @action: the action object
 * @sensitive: %TRUE to make the action sensitive
 * 
 * Sets the :sensitive property of the action to @sensitive. Note that 
 * this doesn’t necessarily mean effective sensitivity. See 
 * ctk_action_is_sensitive() 
 * for that.
 *
 * Since: 2.6
 **/
void
ctk_action_set_sensitive (CtkAction *action,
			  gboolean   sensitive)
{
  g_return_if_fail (CTK_IS_ACTION (action));

  sensitive = sensitive != FALSE;
  
  if (action->private_data->sensitive != sensitive)
    {
      action->private_data->sensitive = sensitive;

      g_object_notify (G_OBJECT (action), "sensitive");
    }
}

/**
 * ctk_action_is_visible:
 * @action: the action object
 * 
 * Returns whether the action is effectively visible.
 *
 * Returns: %TRUE if the action and its associated action group 
 * are both visible.
 *
 * Since: 2.4
 **/
gboolean
ctk_action_is_visible (CtkAction *action)
{
  CtkActionPrivate *priv;
  g_return_val_if_fail (CTK_IS_ACTION (action), FALSE);

  priv = action->private_data;
  return priv->visible &&
    (priv->action_group == NULL ||
     ctk_action_group_get_visible (priv->action_group));
}

/**
 * ctk_action_get_visible:
 * @action: the action object
 * 
 * Returns whether the action itself is visible. Note that this doesn’t 
 * necessarily mean effective visibility. See ctk_action_is_sensitive() 
 * for that.
 *
 * Returns: %TRUE if the action itself is visible.
 *
 * Since: 2.4
 **/
gboolean
ctk_action_get_visible (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), FALSE);

  return action->private_data->visible;
}

/**
 * ctk_action_set_visible:
 * @action: the action object
 * @visible: %TRUE to make the action visible
 * 
 * Sets the :visible property of the action to @visible. Note that 
 * this doesn’t necessarily mean effective visibility. See 
 * ctk_action_is_visible() 
 * for that.
 *
 * Since: 2.6
 **/
void
ctk_action_set_visible (CtkAction *action,
			gboolean   visible)
{
  g_return_if_fail (CTK_IS_ACTION (action));

  visible = visible != FALSE;
  
  if (action->private_data->visible != visible)
    {
      action->private_data->visible = visible;

      g_object_notify (G_OBJECT (action), "visible");
    }
}
/**
 * ctk_action_set_is_important:
 * @action: the action object
 * @is_important: %TRUE to make the action important
 *
 * Sets whether the action is important, this attribute is used
 * primarily by toolbar items to decide whether to show a label
 * or not.
 *
 * Since: 2.16
 */
void 
ctk_action_set_is_important (CtkAction *action,
			     gboolean   is_important)
{
  g_return_if_fail (CTK_IS_ACTION (action));

  is_important = is_important != FALSE;
  
  if (action->private_data->is_important != is_important)
    {
      action->private_data->is_important = is_important;
      
      g_object_notify (G_OBJECT (action), "is-important");
    }  
}

/**
 * ctk_action_get_is_important:
 * @action: a #CtkAction
 *
 * Checks whether @action is important or not
 * 
 * Returns: whether @action is important
 *
 * Since: 2.16
 */
gboolean 
ctk_action_get_is_important (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), FALSE);

  return action->private_data->is_important;
}

/**
 * ctk_action_set_always_show_image:
 * @action: a #CtkAction
 * @always_show: %TRUE if menuitem proxies should always show their image
 *
 * Sets whether @action's menu item proxies will ignore the
 * #CtkSettings:ctk-menu-images setting and always show their image, if available.
 *
 * Use this if the menu item would be useless or hard to use
 * without their image.
 *
 * Since: 2.20
 */
void
ctk_action_set_always_show_image (CtkAction *action,
                                  gboolean   always_show)
{
  CtkActionPrivate *priv;

  g_return_if_fail (CTK_IS_ACTION (action));

  priv = action->private_data;

  always_show = always_show != FALSE;
  
  if (priv->always_show_image != always_show)
    {
      priv->always_show_image = always_show;

      g_object_notify (G_OBJECT (action), "always-show-image");
    }
}

/**
 * ctk_action_get_always_show_image:
 * @action: a #CtkAction
 *
 * Returns whether @action's menu item proxies will always
 * show their image, if available.
 *
 * Returns: %TRUE if the menu item proxies will always show their image
 *
 * Since: 2.20
 */
gboolean
ctk_action_get_always_show_image  (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), FALSE);

  return action->private_data->always_show_image;
}

/**
 * ctk_action_set_label:
 * @action: a #CtkAction
 * @label: the label text to set
 *
 * Sets the label of @action.
 *
 * Since: 2.16
 */
void 
ctk_action_set_label (CtkAction	  *action,
		      const gchar *label)
{
  gchar *tmp;
  
  g_return_if_fail (CTK_IS_ACTION (action));
  
  tmp = action->private_data->label;
  action->private_data->label = g_strdup (label);
  g_free (tmp);
  action->private_data->label_set = (action->private_data->label != NULL);
  /* if label is unset, then use the label from the stock item */
  if (!action->private_data->label_set && action->private_data->stock_id)
    {
      CtkStockItem stock_item;

      if (ctk_stock_lookup (action->private_data->stock_id, &stock_item))
	action->private_data->label = g_strdup (stock_item.label);
    }

  g_object_notify (G_OBJECT (action), "label");
  
  /* if short_label is unset, set short_label=label */
  if (!action->private_data->short_label_set)
    {
      ctk_action_set_short_label (action, action->private_data->label);
      action->private_data->short_label_set = FALSE;
    }
}

/**
 * ctk_action_get_label:
 * @action: a #CtkAction
 *
 * Gets the label text of @action.
 *
 * Returns: the label text
 *
 * Since: 2.16
 */
const gchar *
ctk_action_get_label (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), NULL);

  return action->private_data->label;
}

/**
 * ctk_action_set_short_label:
 * @action: a #CtkAction
 * @short_label: the label text to set
 *
 * Sets a shorter label text on @action.
 *
 * Since: 2.16
 */
void 
ctk_action_set_short_label (CtkAction   *action,
			    const gchar *short_label)
{
  gchar *tmp;

  g_return_if_fail (CTK_IS_ACTION (action));

  tmp = action->private_data->short_label;
  action->private_data->short_label = g_strdup (short_label);
  g_free (tmp);
  action->private_data->short_label_set = (action->private_data->short_label != NULL);
  /* if short_label is unset, then use the value of label */
  if (!action->private_data->short_label_set)
    action->private_data->short_label = g_strdup (action->private_data->label);

  g_object_notify (G_OBJECT (action), "short-label");
}

/**
 * ctk_action_get_short_label:
 * @action: a #CtkAction
 *
 * Gets the short label text of @action.
 *
 * Returns: the short label text.
 *
 * Since: 2.16
 */
const gchar *
ctk_action_get_short_label (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), NULL);

  return action->private_data->short_label;
}

/**
 * ctk_action_set_visible_horizontal:
 * @action: a #CtkAction
 * @visible_horizontal: whether the action is visible horizontally
 *
 * Sets whether @action is visible when horizontal
 *
 * Since: 2.16
 */
void 
ctk_action_set_visible_horizontal (CtkAction *action,
				   gboolean   visible_horizontal)
{
  g_return_if_fail (CTK_IS_ACTION (action));

  g_return_if_fail (CTK_IS_ACTION (action));

  visible_horizontal = visible_horizontal != FALSE;
  
  if (action->private_data->visible_horizontal != visible_horizontal)
    {
      action->private_data->visible_horizontal = visible_horizontal;
      
      g_object_notify (G_OBJECT (action), "visible-horizontal");
    }  
}

/**
 * ctk_action_get_visible_horizontal:
 * @action: a #CtkAction
 *
 * Checks whether @action is visible when horizontal
 * 
 * Returns: whether @action is visible when horizontal
 *
 * Since: 2.16
 */
gboolean 
ctk_action_get_visible_horizontal (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), FALSE);

  return action->private_data->visible_horizontal;
}

/**
 * ctk_action_set_visible_vertical:
 * @action: a #CtkAction
 * @visible_vertical: whether the action is visible vertically
 *
 * Sets whether @action is visible when vertical 
 *
 * Since: 2.16
 */
void 
ctk_action_set_visible_vertical (CtkAction *action,
				 gboolean   visible_vertical)
{
  g_return_if_fail (CTK_IS_ACTION (action));

  g_return_if_fail (CTK_IS_ACTION (action));

  visible_vertical = visible_vertical != FALSE;
  
  if (action->private_data->visible_vertical != visible_vertical)
    {
      action->private_data->visible_vertical = visible_vertical;
      
      g_object_notify (G_OBJECT (action), "visible-vertical");
    }  
}

/**
 * ctk_action_get_visible_vertical:
 * @action: a #CtkAction
 *
 * Checks whether @action is visible when horizontal
 * 
 * Returns: whether @action is visible when horizontal
 *
 * Since: 2.16
 */
gboolean 
ctk_action_get_visible_vertical (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), FALSE);

  return action->private_data->visible_vertical;
}

/**
 * ctk_action_set_tooltip:
 * @action: a #CtkAction
 * @tooltip: the tooltip text
 *
 * Sets the tooltip text on @action
 *
 * Since: 2.16
 */
void 
ctk_action_set_tooltip (CtkAction   *action,
			const gchar *tooltip)
{
  gchar *tmp;

  g_return_if_fail (CTK_IS_ACTION (action));

  tmp = action->private_data->tooltip;
  action->private_data->tooltip = g_strdup (tooltip);
  g_free (tmp);

  g_object_notify (G_OBJECT (action), "tooltip");
}

/**
 * ctk_action_get_tooltip:
 * @action: a #CtkAction
 *
 * Gets the tooltip text of @action.
 *
 * Returns: the tooltip text
 *
 * Since: 2.16
 */
const gchar *
ctk_action_get_tooltip (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), NULL);

  return action->private_data->tooltip;
}

/**
 * ctk_action_set_stock_id:
 * @action: a #CtkAction
 * @stock_id: the stock id
 *
 * Sets the stock id on @action
 *
 * Since: 2.16
 */
void 
ctk_action_set_stock_id (CtkAction   *action,
			 const gchar *stock_id)
{
  gchar *tmp;

  g_return_if_fail (CTK_IS_ACTION (action));

  g_return_if_fail (CTK_IS_ACTION (action));

  tmp = action->private_data->stock_id;
  action->private_data->stock_id = g_strdup (stock_id);
  g_free (tmp);

  g_object_notify (G_OBJECT (action), "stock-id");
  
  /* update label and short_label if appropriate */
  if (!action->private_data->label_set)
    {
      CtkStockItem stock_item;

      if (action->private_data->stock_id &&
	  ctk_stock_lookup (action->private_data->stock_id, &stock_item))
	ctk_action_set_label (action, stock_item.label);
      else
	ctk_action_set_label (action, NULL);

      action->private_data->label_set = FALSE;
    }
}

/**
 * ctk_action_get_stock_id:
 * @action: a #CtkAction
 *
 * Gets the stock id of @action.
 *
 * Returns: the stock id
 *
 * Since: 2.16
 */
const gchar *
ctk_action_get_stock_id (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), NULL);

  return action->private_data->stock_id;
}

/**
 * ctk_action_set_icon_name:
 * @action: a #CtkAction
 * @icon_name: the icon name to set
 *
 * Sets the icon name on @action
 *
 * Since: 2.16
 */
void 
ctk_action_set_icon_name (CtkAction   *action,
			  const gchar *icon_name)
{
  gchar *tmp;

  g_return_if_fail (CTK_IS_ACTION (action));

  tmp = action->private_data->icon_name;
  action->private_data->icon_name = g_strdup (icon_name);
  g_free (tmp);

  g_object_notify (G_OBJECT (action), "icon-name");
}

/**
 * ctk_action_get_icon_name:
 * @action: a #CtkAction
 *
 * Gets the icon name of @action.
 *
 * Returns: the icon name
 *
 * Since: 2.16
 */
const gchar *
ctk_action_get_icon_name (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), NULL);

  return action->private_data->icon_name;
}

/**
 * ctk_action_set_gicon:
 * @action: a #CtkAction
 * @icon: the #GIcon to set
 *
 * Sets the icon of @action.
 *
 * Since: 2.16
 */
void
ctk_action_set_gicon (CtkAction *action,
                      GIcon     *icon)
{
  g_return_if_fail (CTK_IS_ACTION (action));

  if (action->private_data->gicon)
    g_object_unref (action->private_data->gicon);

  action->private_data->gicon = icon;

  if (action->private_data->gicon)
    g_object_ref (action->private_data->gicon);

  g_object_notify (G_OBJECT (action), "gicon");
}

/**
 * ctk_action_get_gicon:
 * @action: a #CtkAction
 *
 * Gets the gicon of @action.
 *
 * Returns: (transfer none): The action’s #GIcon if one is set.
 *
 * Since: 2.16
 */
GIcon *
ctk_action_get_gicon (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), NULL);

  return action->private_data->gicon;
}

static void
closure_accel_activate (GClosure     *closure,
                        GValue       *return_value,
                        guint         n_param_values G_GNUC_UNUSED,
                        const GValue *param_values G_GNUC_UNUSED,
                        gpointer      invocation_hint G_GNUC_UNUSED,
                        gpointer      marshal_data G_GNUC_UNUSED)
{
  if (ctk_action_is_sensitive (CTK_ACTION (closure->data)))
    {
      _ctk_action_emit_activate (CTK_ACTION (closure->data));
      
      /* we handled the accelerator */
      g_value_set_boolean (return_value, TRUE);
    }
}

static void
ctk_action_set_action_group (CtkAction	    *action,
			     CtkActionGroup *action_group)
{
  if (action->private_data->action_group == NULL)
    g_return_if_fail (CTK_IS_ACTION_GROUP (action_group));
  else
    g_return_if_fail (action_group == NULL);

  action->private_data->action_group = action_group;
}

/**
 * ctk_action_set_accel_path:
 * @action: the action object
 * @accel_path: the accelerator path
 *
 * Sets the accel path for this action.  All proxy widgets associated
 * with the action will have this accel path, so that their
 * accelerators are consistent.
 *
 * Note that @accel_path string will be stored in a #GQuark. Therefore, if you
 * pass a static string, you can save some memory by interning it first with 
 * g_intern_static_string().
 *
 * Since: 2.4
 */
void
ctk_action_set_accel_path (CtkAction   *action, 
			   const gchar *accel_path)
{
  g_return_if_fail (CTK_IS_ACTION (action));

  action->private_data->accel_quark = g_quark_from_string (accel_path);
}

/**
 * ctk_action_get_accel_path:
 * @action: the action object
 *
 * Returns the accel path for this action.  
 *
 * Since: 2.6
 *
 * Returns: the accel path for this action, or %NULL
 *   if none is set. The returned string is owned by CTK+ 
 *   and must not be freed or modified.
 */
const gchar *
ctk_action_get_accel_path (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), NULL);

  if (action->private_data->accel_quark)
    return g_quark_to_string (action->private_data->accel_quark);
  else
    return NULL;
}

/**
 * ctk_action_get_accel_closure:
 * @action: the action object
 *
 * Returns the accel closure for this action.
 *
 * Since: 2.8
 *
 * Returns: (transfer none): the accel closure for this action. The
 *          returned closure is owned by CTK+ and must not be unreffed
 *          or modified.
 */
GClosure *
ctk_action_get_accel_closure (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), NULL);

  return action->private_data->accel_closure;
}


/**
 * ctk_action_set_accel_group:
 * @action: the action object
 * @accel_group: (allow-none): a #CtkAccelGroup or %NULL
 *
 * Sets the #CtkAccelGroup in which the accelerator for this action
 * will be installed.
 *
 * Since: 2.4
 **/
void
ctk_action_set_accel_group (CtkAction     *action,
			    CtkAccelGroup *accel_group)
{
  g_return_if_fail (CTK_IS_ACTION (action));
  g_return_if_fail (accel_group == NULL || CTK_IS_ACCEL_GROUP (accel_group));
  
  if (accel_group)
    g_object_ref (accel_group);
  if (action->private_data->accel_group)
    g_object_unref (action->private_data->accel_group);

  action->private_data->accel_group = accel_group;
}

/**
 * ctk_action_connect_accelerator:
 * @action: a #CtkAction
 * 
 * Installs the accelerator for @action if @action has an
 * accel path and group. See ctk_action_set_accel_path() and 
 * ctk_action_set_accel_group()
 *
 * Since multiple proxies may independently trigger the installation
 * of the accelerator, the @action counts the number of times this
 * function has been called and doesn’t remove the accelerator until
 * ctk_action_disconnect_accelerator() has been called as many times.
 *
 * Since: 2.4
 **/
void 
ctk_action_connect_accelerator (CtkAction *action)
{
  g_return_if_fail (CTK_IS_ACTION (action));

  if (!action->private_data->accel_quark ||
      !action->private_data->accel_group)
    return;

  if (action->private_data->accel_count == 0)
    {
      const gchar *accel_path = 
	g_quark_to_string (action->private_data->accel_quark);
      
      ctk_accel_group_connect_by_path (action->private_data->accel_group,
				       accel_path,
				       action->private_data->accel_closure);
    }

  action->private_data->accel_count++;
}

/**
 * ctk_action_disconnect_accelerator:
 * @action: a #CtkAction
 * 
 * Undoes the effect of one call to ctk_action_connect_accelerator().
 *
 * Since: 2.4
 **/
void 
ctk_action_disconnect_accelerator (CtkAction *action)
{
  g_return_if_fail (CTK_IS_ACTION (action));

  if (!action->private_data->accel_quark ||
      !action->private_data->accel_group)
    return;

  action->private_data->accel_count--;

  if (action->private_data->accel_count == 0)
    ctk_accel_group_disconnect (action->private_data->accel_group,
				action->private_data->accel_closure);
}

/**
 * ctk_action_create_menu:
 * @action: a #CtkAction
 *
 * If @action provides a #CtkMenu widget as a submenu for the menu
 * item or the toolbar item it creates, this function returns an
 * instance of that menu.
 *
 * Returns: (transfer none): the menu item provided by the
 *               action, or %NULL.
 *
 * Since: 2.12
 */
CtkWidget *
ctk_action_create_menu (CtkAction *action)
{
  g_return_val_if_fail (CTK_IS_ACTION (action), NULL);

  if (CTK_ACTION_GET_CLASS (action)->create_menu)
    return CTK_ACTION_GET_CLASS (action)->create_menu (action);

  return NULL;
}
