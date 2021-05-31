/* ctkradiotoolbutton.c
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@gnome.og>
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
#include "ctkradiotoolbutton.h"
#include "ctkradiobutton.h"
#include "ctkintl.h"
#include "ctkprivate.h"


/**
 * SECTION:ctkradiotoolbutton
 * @Short_description: A toolbar item that contains a radio button
 * @Title: CtkRadioToolButton
 * @See_also: #CtkToolbar, #CtkToolButton
 *
 * A #CtkRadioToolButton is a #CtkToolItem that contains a radio button,
 * that is, a button that is part of a group of toggle buttons where only
 * one button can be active at a time.
 *
 * Use ctk_radio_tool_button_new() to create a new CtkRadioToolButton. Use
 * ctk_radio_tool_button_new_from_widget() to create a new CtkRadioToolButton
 * that is part of the same group as an existing CtkRadioToolButton.
 *
 * # CSS nodes
 *
 * CtkRadioToolButton has a single CSS node with name toolbutton.
 */


enum {
  PROP_0,
  PROP_GROUP
};

static void ctk_radio_tool_button_set_property (GObject         *object,
						guint            prop_id,
						const GValue    *value,
						GParamSpec      *pspec);

G_DEFINE_TYPE (CtkRadioToolButton, ctk_radio_tool_button, CTK_TYPE_TOGGLE_TOOL_BUTTON)

static void
ctk_radio_tool_button_class_init (CtkRadioToolButtonClass *klass)
{
  GObjectClass *object_class;
  CtkToolButtonClass *toolbutton_class;

  object_class = (GObjectClass *)klass;
  toolbutton_class = (CtkToolButtonClass *)klass;

  object_class->set_property = ctk_radio_tool_button_set_property;
  
  toolbutton_class->button_type = CTK_TYPE_RADIO_BUTTON;  

  /**
   * CtkRadioToolButton:group:
   *
   * Sets a new group for a radio tool button.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
				   PROP_GROUP,
				   g_param_spec_object ("group",
							P_("Group"),
							P_("The radio tool button whose group this button belongs to."),
							CTK_TYPE_RADIO_TOOL_BUTTON,
							CTK_PARAM_WRITABLE));

}

static void
ctk_radio_tool_button_init (CtkRadioToolButton *button)
{
  CtkToolButton *tool_button = CTK_TOOL_BUTTON (button);
  ctk_toggle_button_set_mode (CTK_TOGGLE_BUTTON (_ctk_tool_button_get_button (tool_button)), FALSE);
}

static void
ctk_radio_tool_button_set_property (GObject         *object,
				    guint            prop_id,
				    const GValue    *value,
				    GParamSpec      *pspec)
{
  CtkRadioToolButton *button;

  button = CTK_RADIO_TOOL_BUTTON (object);

  switch (prop_id)
    {
    case PROP_GROUP:
      {
	CtkRadioToolButton *arg;
	GSList *slist = NULL;
	if (G_VALUE_HOLDS_OBJECT (value)) 
	  {
	    arg = CTK_RADIO_TOOL_BUTTON (g_value_get_object (value));
	    if (arg)
	      slist = ctk_radio_tool_button_get_group (arg);
	    ctk_radio_tool_button_set_group (button, slist);
	  }
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * ctk_radio_tool_button_new:
 * @group: (allow-none) (element-type CtkRadioButton): An
 *   existing radio button group, or %NULL if you are creating a new group
 * 
 * Creates a new #CtkRadioToolButton, adding it to @group.
 * 
 * Returns: The new #CtkRadioToolButton
 * 
 * Since: 2.4
 **/
CtkToolItem *
ctk_radio_tool_button_new (GSList *group)
{
  CtkRadioToolButton *button;
  
  button = g_object_new (CTK_TYPE_RADIO_TOOL_BUTTON,
			 NULL);

  ctk_radio_tool_button_set_group (button, group);
  
  return CTK_TOOL_ITEM (button);
}

/**
 * ctk_radio_tool_button_new_from_stock:
 * @group: (allow-none) (element-type CtkRadioButton): an existing radio button
 *   group, or %NULL if you are creating a new group
 * @stock_id: the name of a stock item
 * 
 * Creates a new #CtkRadioToolButton, adding it to @group. 
 * The new #CtkRadioToolButton will contain an icon and label from the
 * stock item indicated by @stock_id.
 * 
 * Returns: The new #CtkRadioToolButton
 * 
 * Since: 2.4
 *
 * Deprecated: 3.10: Use ctk_radio_tool_button_new() instead.
 **/
CtkToolItem *
ctk_radio_tool_button_new_from_stock (GSList      *group,
				      const gchar *stock_id)
{
  CtkRadioToolButton *button;

  g_return_val_if_fail (stock_id != NULL, NULL);
  
  button = g_object_new (CTK_TYPE_RADIO_TOOL_BUTTON,
			 "stock-id", stock_id,
			 NULL);


  ctk_radio_tool_button_set_group (button, group);
  
  return CTK_TOOL_ITEM (button);
}

/**
 * ctk_radio_tool_button_new_from_widget: (constructor)
 * @group: (allow-none): An existing #CtkRadioToolButton, or %NULL
 *
 * Creates a new #CtkRadioToolButton adding it to the same group as @gruup
 *
 * Returns: (transfer none): The new #CtkRadioToolButton
 *
 * Since: 2.4
 **/
CtkToolItem *
ctk_radio_tool_button_new_from_widget (CtkRadioToolButton *group)
{
  GSList *list = NULL;
  
  g_return_val_if_fail (group == NULL || CTK_IS_RADIO_TOOL_BUTTON (group), NULL);

  if (group != NULL)
    list = ctk_radio_tool_button_get_group (CTK_RADIO_TOOL_BUTTON (group));
  
  return ctk_radio_tool_button_new (list);
}

/**
 * ctk_radio_tool_button_new_with_stock_from_widget: (constructor)
 * @group: (allow-none): An existing #CtkRadioToolButton.
 * @stock_id: the name of a stock item
 *
 * Creates a new #CtkRadioToolButton adding it to the same group as @group.
 * The new #CtkRadioToolButton will contain an icon and label from the
 * stock item indicated by @stock_id.
 *
 * Returns: (transfer none): A new #CtkRadioToolButton
 *
 * Since: 2.4
 *
 * Deprecated: 3.10: ctk_radio_tool_button_new_from_widget
 **/
CtkToolItem *
ctk_radio_tool_button_new_with_stock_from_widget (CtkRadioToolButton *group,
						  const gchar        *stock_id)
{
  GSList *list = NULL;
  CtkToolItem *item;

  g_return_val_if_fail (group == NULL || CTK_IS_RADIO_TOOL_BUTTON (group), NULL);

  if (group != NULL)
    list = ctk_radio_tool_button_get_group (group);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  item = ctk_radio_tool_button_new_from_stock (list, stock_id);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  return item;
}

static CtkRadioButton *
get_radio_button (CtkRadioToolButton *button)
{
  return CTK_RADIO_BUTTON (_ctk_tool_button_get_button (CTK_TOOL_BUTTON (button)));
}

/**
 * ctk_radio_tool_button_get_group:
 * @button: a #CtkRadioToolButton
 *
 * Returns the radio button group @button belongs to.
 *
 * Returns: (transfer none) (element-type CtkRadioButton): The group @button belongs to.
 *
 * Since: 2.4
 */
GSList *
ctk_radio_tool_button_get_group (CtkRadioToolButton *button)
{
  g_return_val_if_fail (CTK_IS_RADIO_TOOL_BUTTON (button), NULL);

  return ctk_radio_button_get_group (get_radio_button (button));
}

/**
 * ctk_radio_tool_button_set_group:
 * @button: a #CtkRadioToolButton
 * @group: (element-type CtkRadioButton) (allow-none): an existing radio button group, or %NULL
 * 
 * Adds @button to @group, removing it from the group it belonged to before.
 * 
 * Since: 2.4
 **/
void
ctk_radio_tool_button_set_group (CtkRadioToolButton *button,
				 GSList             *group)
{
  g_return_if_fail (CTK_IS_RADIO_TOOL_BUTTON (button));

  ctk_radio_button_set_group (get_radio_button (button), group);
}
