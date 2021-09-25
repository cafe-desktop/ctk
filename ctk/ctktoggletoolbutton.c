 /* ctktoggletoolbutton.c
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
#include "ctktoggletoolbutton.h"
#include "ctkcheckmenuitem.h"
#include "ctklabel.h"
#include "ctktogglebutton.h"
#include "ctkstock.h"
#include "ctkintl.h"
#include "ctkradiotoolbutton.h"
#include "ctktoggleaction.h"
#include "deprecated/ctkactivatable.h"
#include "ctkprivate.h"


/**
 * SECTION:ctktoggletoolbutton
 * @Short_description: A CtkToolItem containing a toggle button
 * @Title: CtkToggleToolButton
 * @See_also: #CtkToolbar, #CtkToolButton, #CtkSeparatorToolItem
 *
 * A #CtkToggleToolButton is a #CtkToolItem that contains a toggle
 * button.
 *
 * Use ctk_toggle_tool_button_new() to create a new CtkToggleToolButton.
 *
 * # CSS nodes
 *
 * CtkToggleToolButton has a single CSS node with name togglebutton.
 */


#define MENU_ID "ctk-toggle-tool-button-menu-id"

enum {
  TOGGLED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_ACTIVE
};


struct _CtkToggleToolButtonPrivate
{
  guint active : 1;
};
  

static void     ctk_toggle_tool_button_set_property        (GObject      *object,
							    guint         prop_id,
							    const GValue *value,
							    GParamSpec   *pspec);
static void     ctk_toggle_tool_button_get_property        (GObject      *object,
							    guint         prop_id,
							    GValue       *value,
							    GParamSpec   *pspec);

static gboolean ctk_toggle_tool_button_create_menu_proxy (CtkToolItem *button);

static void button_toggled      (CtkWidget           *widget,
				 CtkToggleToolButton *button);
static void menu_item_activated (CtkWidget           *widget,
				 CtkToggleToolButton *button);


static void ctk_toggle_tool_button_activatable_interface_init (CtkActivatableIface  *iface);
static void ctk_toggle_tool_button_update                     (CtkActivatable       *activatable,
							       CtkAction            *action,
							       const gchar          *property_name);
static void ctk_toggle_tool_button_sync_action_properties     (CtkActivatable       *activatable,
							       CtkAction            *action);

static CtkActivatableIface *parent_activatable_iface;
static guint                toggle_signals[LAST_SIGNAL] = { 0 };

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
G_DEFINE_TYPE_WITH_CODE (CtkToggleToolButton, ctk_toggle_tool_button, CTK_TYPE_TOOL_BUTTON,
                         G_ADD_PRIVATE (CtkToggleToolButton)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIVATABLE,
						ctk_toggle_tool_button_activatable_interface_init))
G_GNUC_END_IGNORE_DEPRECATIONS;

static void
ctk_toggle_tool_button_class_init (CtkToggleToolButtonClass *klass)
{
  GObjectClass *object_class;
  CtkToolItemClass *toolitem_class;
  CtkToolButtonClass *toolbutton_class;

  object_class = (GObjectClass *)klass;
  toolitem_class = (CtkToolItemClass *)klass;
  toolbutton_class = (CtkToolButtonClass *)klass;

  object_class->set_property = ctk_toggle_tool_button_set_property;
  object_class->get_property = ctk_toggle_tool_button_get_property;

  toolitem_class->create_menu_proxy = ctk_toggle_tool_button_create_menu_proxy;
  toolbutton_class->button_type = CTK_TYPE_TOGGLE_BUTTON;

  /**
   * CtkToggleToolButton:active:
   *
   * If the toggle tool button should be pressed in.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         P_("Active"),
                                                         P_("If the toggle button should be pressed in"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

/**
 * CtkToggleToolButton::toggled:
 * @toggle_tool_button: the object that emitted the signal
 *
 * Emitted whenever the toggle tool button changes state.
 **/
  toggle_signals[TOGGLED] =
    g_signal_new (I_("toggled"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkToggleToolButtonClass, toggled),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
}

static void
ctk_toggle_tool_button_init (CtkToggleToolButton *button)
{
  CtkToolButton *tool_button = CTK_TOOL_BUTTON (button);
  CtkToggleButton *toggle_button = CTK_TOGGLE_BUTTON (_ctk_tool_button_get_button (tool_button));

  button->priv = ctk_toggle_tool_button_get_instance_private (button);

  /* If the real button is a radio button, it may have been
   * active at the time it was created.
   */
  button->priv->active = ctk_toggle_button_get_active (toggle_button);
    
  g_signal_connect_object (toggle_button,
			   "toggled", G_CALLBACK (button_toggled), button, 0);
}

static void
ctk_toggle_tool_button_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec)
{
  CtkToggleToolButton *button = CTK_TOGGLE_TOOL_BUTTON (object);

  switch (prop_id)
    {
      case PROP_ACTIVE:
	ctk_toggle_tool_button_set_active (button, 
					   g_value_get_boolean (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
ctk_toggle_tool_button_get_property (GObject    *object,
				     guint       prop_id,
				     GValue     *value,
				     GParamSpec *pspec)
{
  CtkToggleToolButton *button = CTK_TOGGLE_TOOL_BUTTON (object);

  switch (prop_id)
    {
      case PROP_ACTIVE:
        g_value_set_boolean (value, ctk_toggle_tool_button_get_active (button));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static gboolean
ctk_toggle_tool_button_create_menu_proxy (CtkToolItem *item)
{
  CtkToolButton *tool_button = CTK_TOOL_BUTTON (item);
  CtkToggleToolButton *toggle_tool_button = CTK_TOGGLE_TOOL_BUTTON (item);
  CtkWidget *menu_item = NULL;
  CtkStockItem stock_item;
  gboolean use_mnemonic = TRUE;
  const char *label;
  CtkWidget *label_widget;
  const gchar *label_text;
  const gchar *stock_id;

  if (_ctk_tool_item_create_menu_proxy (item))
    return TRUE;

  label_widget = ctk_tool_button_get_label_widget (tool_button);
  label_text = ctk_tool_button_get_label (tool_button);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  stock_id = ctk_tool_button_get_stock_id (tool_button);

  if (CTK_IS_LABEL (label_widget))
    {
      label = ctk_label_get_label (CTK_LABEL (label_widget));
      use_mnemonic = ctk_label_get_use_underline (CTK_LABEL (label_widget));
    }
  else if (label_text)
    {
      label = label_text;
      use_mnemonic = ctk_tool_button_get_use_underline (tool_button);
    }
  else if (stock_id && ctk_stock_lookup (stock_id, &stock_item))
    {
      label = stock_item.label;
    }
  else
    {
      label = "";
    }

  G_GNUC_END_IGNORE_DEPRECATIONS;

  if (use_mnemonic)
    menu_item = ctk_check_menu_item_new_with_mnemonic (label);
  else
    menu_item = ctk_check_menu_item_new_with_label (label);

  ctk_check_menu_item_set_active (CTK_CHECK_MENU_ITEM (menu_item),
				  toggle_tool_button->priv->active);

  if (CTK_IS_RADIO_TOOL_BUTTON (toggle_tool_button))
    {
      ctk_check_menu_item_set_draw_as_radio (CTK_CHECK_MENU_ITEM (menu_item),
					     TRUE);
    }

  g_signal_connect_closure_by_id (menu_item,
				  g_signal_lookup ("activate", G_OBJECT_TYPE (menu_item)), 0,
				  g_cclosure_new_object (G_CALLBACK (menu_item_activated),
							 G_OBJECT (toggle_tool_button)),
				  FALSE);

  ctk_tool_item_set_proxy_menu_item (item, MENU_ID, menu_item);
  
  return TRUE;
}

/* There are two activatable widgets, a toggle button and a menu item.
 *
 * If a widget is activated and the state of the tool button is the same as
 * the new state of the activated widget, then the other widget was the one
 * that was activated by the user and updated the tool button’s state.
 *
 * If the state of the tool button is not the same as the new state of the
 * activated widget, then the activation was activated by the user, and the
 * widget needs to make sure the tool button is updated before the other
 * widget is activated. This will make sure the other widget a tool button
 * in a state that matches its own new state.
 */
static void
menu_item_activated (CtkWidget           *menu_item,
		     CtkToggleToolButton *toggle_tool_button)
{
  CtkToolButton *tool_button = CTK_TOOL_BUTTON (toggle_tool_button);
  gboolean menu_active = ctk_check_menu_item_get_active (CTK_CHECK_MENU_ITEM (menu_item));

  if (toggle_tool_button->priv->active != menu_active)
    {
      toggle_tool_button->priv->active = menu_active;

      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (_ctk_tool_button_get_button (tool_button)),
				    toggle_tool_button->priv->active);

      g_object_notify (G_OBJECT (toggle_tool_button), "active");
      g_signal_emit (toggle_tool_button, toggle_signals[TOGGLED], 0);
    }
}

static void
button_toggled (CtkWidget           *widget,
		CtkToggleToolButton *toggle_tool_button)
{
  gboolean toggle_active;

  toggle_active = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (widget));

  if (toggle_tool_button->priv->active != toggle_active)
    {
      CtkWidget *menu_item;
      
      toggle_tool_button->priv->active = toggle_active;
       
      if ((menu_item =
	   ctk_tool_item_get_proxy_menu_item (CTK_TOOL_ITEM (toggle_tool_button), MENU_ID)))
	{
	  ctk_check_menu_item_set_active (CTK_CHECK_MENU_ITEM (menu_item),
					  toggle_tool_button->priv->active);
	}

      g_object_notify (G_OBJECT (toggle_tool_button), "active");
      g_signal_emit (toggle_tool_button, toggle_signals[TOGGLED], 0);
    }
}

static void
ctk_toggle_tool_button_activatable_interface_init (CtkActivatableIface *iface)
{
  parent_activatable_iface = g_type_interface_peek_parent (iface);
  iface->update = ctk_toggle_tool_button_update;
  iface->sync_action_properties = ctk_toggle_tool_button_sync_action_properties;
}

static void
ctk_toggle_tool_button_update (CtkActivatable *activatable,
			       CtkAction      *action,
			       const gchar    *property_name)
{
  CtkToggleToolButton *button;

  parent_activatable_iface->update (activatable, action, property_name);

  button = CTK_TOGGLE_TOOL_BUTTON (activatable);

  if (strcmp (property_name, "active") == 0)
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_action_block_activate (action);
      ctk_toggle_tool_button_set_active (button, ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)));
      ctk_action_unblock_activate (action);
      G_GNUC_END_IGNORE_DEPRECATIONS;
    }
}

static void
ctk_toggle_tool_button_sync_action_properties (CtkActivatable *activatable,
					       CtkAction      *action)
{
  CtkToggleToolButton *button;
  gboolean is_toggle_action;

  parent_activatable_iface->sync_action_properties (activatable, action);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  is_toggle_action = CTK_IS_TOGGLE_ACTION (action);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  if (!is_toggle_action)
    return;

  button = CTK_TOGGLE_TOOL_BUTTON (activatable);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  ctk_action_block_activate (action);
  ctk_toggle_tool_button_set_active (button, ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)));
  ctk_action_unblock_activate (action);
  G_GNUC_END_IGNORE_DEPRECATIONS;
}


/**
 * ctk_toggle_tool_button_new:
 * 
 * Returns a new #CtkToggleToolButton
 * 
 * Returns: a newly created #CtkToggleToolButton
 * 
 * Since: 2.4
 **/
CtkToolItem *
ctk_toggle_tool_button_new (void)
{
  CtkToolButton *button;

  button = g_object_new (CTK_TYPE_TOGGLE_TOOL_BUTTON,
			 NULL);
  
  return CTK_TOOL_ITEM (button);
}

/**
 * ctk_toggle_tool_button_new_from_stock:
 * @stock_id: the name of the stock item 
 *
 * Creates a new #CtkToggleToolButton containing the image and text from a
 * stock item. Some stock ids have preprocessor macros like #CTK_STOCK_OK
 * and #CTK_STOCK_APPLY.
 *
 * It is an error if @stock_id is not a name of a stock item.
 * 
 * Returns: A new #CtkToggleToolButton
 * 
 * Since: 2.4
 *
 * Deprecated: 3.10: Use ctk_toggle_tool_button_new() instead.
 **/
CtkToolItem *
ctk_toggle_tool_button_new_from_stock (const gchar *stock_id)
{
  CtkToolButton *button;

  g_return_val_if_fail (stock_id != NULL, NULL);
  
  button = g_object_new (CTK_TYPE_TOGGLE_TOOL_BUTTON,
			 "stock-id", stock_id,
			 NULL);
  
  return CTK_TOOL_ITEM (button);
}

/**
 * ctk_toggle_tool_button_set_active:
 * @button: a #CtkToggleToolButton
 * @is_active: whether @button should be active
 * 
 * Sets the status of the toggle tool button. Set to %TRUE if you
 * want the CtkToggleButton to be “pressed in”, and %FALSE to raise it.
 * This action causes the toggled signal to be emitted.
 * 
 * Since: 2.4
 **/
void
ctk_toggle_tool_button_set_active (CtkToggleToolButton *button,
                                   gboolean             is_active)
{
  g_return_if_fail (CTK_IS_TOGGLE_TOOL_BUTTON (button));

  is_active = is_active != FALSE;

  if (button->priv->active != is_active)
    {
      ctk_button_clicked (CTK_BUTTON (_ctk_tool_button_get_button (CTK_TOOL_BUTTON (button))));
      g_object_notify (G_OBJECT (button), "active");
    }
}

/**
 * ctk_toggle_tool_button_get_active:
 * @button: a #CtkToggleToolButton
 * 
 * Queries a #CtkToggleToolButton and returns its current state.
 * Returns %TRUE if the toggle button is pressed in and %FALSE if it is raised.
 * 
 * Returns: %TRUE if the toggle tool button is pressed in, %FALSE if not
 * 
 * Since: 2.4
 **/
gboolean
ctk_toggle_tool_button_get_active (CtkToggleToolButton *button)
{
  g_return_val_if_fail (CTK_IS_TOGGLE_TOOL_BUTTON (button), FALSE);

  return button->priv->active;
}
