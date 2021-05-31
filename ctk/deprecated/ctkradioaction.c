/*
 * GTK - The GIMP Toolkit
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
 * Modified by the GTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#define GDK_DISABLE_DEPRECATION_WARNINGS

#include "ctkradioaction.h"
#include "ctkradiomenuitem.h"
#include "ctktoggletoolbutton.h"
#include "ctkintl.h"
#include "ctkprivate.h"

/**
 * SECTION:ctkradioaction
 * @Short_description: An action of which only one in a group can be active
 * @Title: CtkRadioAction
 *
 * A #CtkRadioAction is similar to #CtkRadioMenuItem. A number of radio
 * actions can be linked together so that only one may be active at any
 * one time.
 */


struct _CtkRadioActionPrivate 
{
  GSList *group;
  gint    value;
};

enum 
{
  CHANGED,
  LAST_SIGNAL
};

enum 
{
  PROP_0,
  PROP_VALUE,
  PROP_GROUP,
  PROP_CURRENT_VALUE
};

static void ctk_radio_action_finalize     (GObject *object);
static void ctk_radio_action_set_property (GObject         *object,
				           guint            prop_id,
				           const GValue    *value,
				           GParamSpec      *pspec);
static void ctk_radio_action_get_property (GObject         *object,
				           guint            prop_id,
				           GValue          *value,
				           GParamSpec      *pspec);
static void ctk_radio_action_activate     (CtkAction *action);
static CtkWidget *create_menu_item        (CtkAction *action);


G_DEFINE_TYPE_WITH_PRIVATE (CtkRadioAction, ctk_radio_action, CTK_TYPE_TOGGLE_ACTION)

static guint         radio_action_signals[LAST_SIGNAL] = { 0 };

static void
ctk_radio_action_class_init (CtkRadioActionClass *klass)
{
  GObjectClass *gobject_class;
  CtkActionClass *action_class;

  gobject_class = G_OBJECT_CLASS (klass);
  action_class = CTK_ACTION_CLASS (klass);

  gobject_class->finalize = ctk_radio_action_finalize;
  gobject_class->set_property = ctk_radio_action_set_property;
  gobject_class->get_property = ctk_radio_action_get_property;

  action_class->activate = ctk_radio_action_activate;

  action_class->create_menu_item = create_menu_item;

  /**
   * CtkRadioAction:value:
   *
   * The value is an arbitrary integer which can be used as a
   * convenient way to determine which action in the group is 
   * currently active in an ::activate or ::changed signal handler.
   * See ctk_radio_action_get_current_value() and #CtkRadioActionEntry
   * for convenient ways to get and set this property.
   *
   * Since: 2.4
   *
   * Deprecated: 3.10
   */
  g_object_class_install_property (gobject_class,
				   PROP_VALUE,
				   g_param_spec_int ("value",
						     P_("The value"),
						     P_("The value returned by ctk_radio_action_get_current_value() when this action is the current action of its group."),
						     G_MININT,
						     G_MAXINT,
						     0,
						     CTK_PARAM_READWRITE));

  /**
   * CtkRadioAction:group:
   *
   * Sets a new group for a radio action.
   *
   * Since: 2.4
   *
   * Deprecated: 3.10
   */
  g_object_class_install_property (gobject_class,
				   PROP_GROUP,
				   g_param_spec_object ("group",
							P_("Group"),
							P_("The radio action whose group this action belongs to."),
							CTK_TYPE_RADIO_ACTION,
							CTK_PARAM_WRITABLE));

  /**
   * CtkRadioAction:current-value:
   *
   * The value property of the currently active member of the group to which
   * this action belongs. 
   *
   * Since: 2.10
   *
   * Deprecated: 3.10
   */
  g_object_class_install_property (gobject_class,
				   PROP_CURRENT_VALUE,
                                   g_param_spec_int ("current-value",
						     P_("The current value"),
						     P_("The value property of the currently active member of the group to which this action belongs."),
						     G_MININT,
						     G_MAXINT,
						     0,
						     CTK_PARAM_READWRITE));

  /**
   * CtkRadioAction::changed:
   * @action: the action on which the signal is emitted
   * @current: the member of @action's group which has just been activated
   *
   * The ::changed signal is emitted on every member of a radio group when the
   * active member is changed. The signal gets emitted after the ::activate signals
   * for the previous and current active members.
   *
   * Since: 2.4
   *
   * Deprecated: 3.10
   */
  radio_action_signals[CHANGED] =
    g_signal_new (I_("changed"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		  G_STRUCT_OFFSET (CtkRadioActionClass, changed),  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1, CTK_TYPE_RADIO_ACTION);
}

static void
ctk_radio_action_init (CtkRadioAction *action)
{
  action->private_data = ctk_radio_action_get_instance_private (action);
  action->private_data->group = g_slist_prepend (NULL, action);
  action->private_data->value = 0;

  ctk_toggle_action_set_draw_as_radio (CTK_TOGGLE_ACTION (action), TRUE);
}

/**
 * ctk_radio_action_new:
 * @name: A unique name for the action
 * @label: (allow-none): The label displayed in menu items and on buttons,
 *   or %NULL
 * @tooltip: (allow-none): A tooltip for this action, or %NULL
 * @stock_id: (allow-none): The stock icon to display in widgets representing
 *   this action, or %NULL
 * @value: The value which ctk_radio_action_get_current_value() should
 *   return if this action is selected.
 *
 * Creates a new #CtkRadioAction object. To add the action to
 * a #CtkActionGroup and set the accelerator for the action,
 * call ctk_action_group_add_action_with_accel().
 *
 * Returns: a new #CtkRadioAction
 *
 * Since: 2.4
 *
 * Deprecated: 3.10
 */
CtkRadioAction *
ctk_radio_action_new (const gchar *name,
		      const gchar *label,
		      const gchar *tooltip,
		      const gchar *stock_id,
		      gint value)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (CTK_TYPE_RADIO_ACTION,
                       "name", name,
                       "label", label,
                       "tooltip", tooltip,
                       "stock-id", stock_id,
                       "value", value,
                       NULL);
}

static void
ctk_radio_action_finalize (GObject *object)
{
  CtkRadioAction *action;
  GSList *tmp_list;

  action = CTK_RADIO_ACTION (object);

  action->private_data->group = g_slist_remove (action->private_data->group, action);

  tmp_list = action->private_data->group;

  while (tmp_list)
    {
      CtkRadioAction *tmp_action = tmp_list->data;

      tmp_list = tmp_list->next;
      tmp_action->private_data->group = action->private_data->group;
    }

  G_OBJECT_CLASS (ctk_radio_action_parent_class)->finalize (object);
}

static void
ctk_radio_action_set_property (GObject         *object,
			       guint            prop_id,
			       const GValue    *value,
			       GParamSpec      *pspec)
{
  CtkRadioAction *radio_action;
  
  radio_action = CTK_RADIO_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      radio_action->private_data->value = g_value_get_int (value);
      break;
    case PROP_GROUP: 
      {
	CtkRadioAction *arg;
	GSList *slist = NULL;
	
	if (G_VALUE_HOLDS_OBJECT (value)) 
	  {
	    arg = CTK_RADIO_ACTION (g_value_get_object (value));
	    if (arg)
	      slist = ctk_radio_action_get_group (arg);
	    ctk_radio_action_set_group (radio_action, slist);
	  }
      }
      break;
    case PROP_CURRENT_VALUE:
      ctk_radio_action_set_current_value (radio_action,
                                          g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_radio_action_get_property (GObject    *object,
			       guint       prop_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  CtkRadioAction *radio_action;

  radio_action = CTK_RADIO_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      g_value_set_int (value, radio_action->private_data->value);
      break;
    case PROP_CURRENT_VALUE:
      g_value_set_int (value,
                       ctk_radio_action_get_current_value (radio_action));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_radio_action_activate (CtkAction *action)
{
  CtkRadioAction *radio_action;
  CtkToggleAction *toggle_action;
  CtkToggleAction *tmp_action;
  GSList *tmp_list;
  gboolean active;

  radio_action = CTK_RADIO_ACTION (action);
  toggle_action = CTK_TOGGLE_ACTION (action);

  active = ctk_toggle_action_get_active (toggle_action);
  if (active)
    {
      tmp_list = radio_action->private_data->group;

      while (tmp_list)
	{
	  tmp_action = tmp_list->data;
	  tmp_list = tmp_list->next;

	  if (ctk_toggle_action_get_active (tmp_action) &&
              (tmp_action != toggle_action))
	    {
              _ctk_toggle_action_set_active (toggle_action, !active);

	      break;
	    }
	}
      g_object_notify (G_OBJECT (action), "active");
    }
  else
    {
      _ctk_toggle_action_set_active (toggle_action, !active);
      g_object_notify (G_OBJECT (action), "active");

      tmp_list = radio_action->private_data->group;
      while (tmp_list)
	{
	  tmp_action = tmp_list->data;
	  tmp_list = tmp_list->next;

          if (ctk_toggle_action_get_active (tmp_action) &&
              (tmp_action != toggle_action))
	    {
	      _ctk_action_emit_activate (CTK_ACTION (tmp_action));
	      break;
	    }
	}

      tmp_list = radio_action->private_data->group;
      while (tmp_list)
	{
	  tmp_action = tmp_list->data;
	  tmp_list = tmp_list->next;
	  
          g_object_notify (G_OBJECT (tmp_action), "current-value");

	  g_signal_emit (tmp_action, radio_action_signals[CHANGED], 0, radio_action);
	}
    }

  ctk_toggle_action_toggled (toggle_action);
}

static CtkWidget *
create_menu_item (CtkAction *action)
{
  return g_object_new (CTK_TYPE_CHECK_MENU_ITEM, 
		       "draw-as-radio", TRUE,
		       NULL);
}

/**
 * ctk_radio_action_get_group:
 * @action: the action object
 *
 * Returns the list representing the radio group for this object.
 * Note that the returned list is only valid until the next change
 * to the group. 
 *
 * A common way to set up a group of radio group is the following:
 * |[<!-- language="C" -->
 *   GSList *group = NULL;
 *   CtkRadioAction *action;
 *  
 *   while ( ...more actions to add... /)
 *     {
 *        action = ctk_radio_action_new (...);
 *        
 *        ctk_radio_action_set_group (action, group);
 *        group = ctk_radio_action_get_group (action);
 *     }
 * ]|
 *
 * Returns:  (element-type CtkRadioAction) (transfer none): the list representing the radio group for this object
 *
 * Since: 2.4
 *
 * Deprecated: 3.10
 */
GSList *
ctk_radio_action_get_group (CtkRadioAction *action)
{
  g_return_val_if_fail (CTK_IS_RADIO_ACTION (action), NULL);

  return action->private_data->group;
}

/**
 * ctk_radio_action_set_group:
 * @action: the action object
 * @group: (element-type CtkRadioAction) (allow-none): a list representing a radio group, or %NULL
 *
 * Sets the radio group for the radio action object.
 *
 * Since: 2.4
 *
 * Deprecated: 3.10
 */
void
ctk_radio_action_set_group (CtkRadioAction *action, 
			    GSList         *group)
{
  g_return_if_fail (CTK_IS_RADIO_ACTION (action));
  g_return_if_fail (!g_slist_find (group, action));

  if (action->private_data->group)
    {
      GSList *slist;

      action->private_data->group = g_slist_remove (action->private_data->group, action);

      for (slist = action->private_data->group; slist; slist = slist->next)
	{
	  CtkRadioAction *tmp_action = slist->data;

	  tmp_action->private_data->group = action->private_data->group;
	}
    }

  action->private_data->group = g_slist_prepend (group, action);

  if (group)
    {
      GSList *slist;

      for (slist = action->private_data->group; slist; slist = slist->next)
	{
	  CtkRadioAction *tmp_action = slist->data;

	  tmp_action->private_data->group = action->private_data->group;
	}
    }
  else
    {
      ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action), TRUE);
    }
}

/**
 * ctk_radio_action_join_group:
 * @action: the action object
 * @group_source: (allow-none): a radio action object whos group we are 
 *   joining, or %NULL to remove the radio action from its group
 *
 * Joins a radio action object to the group of another radio action object.
 *
 * Use this in language bindings instead of the ctk_radio_action_get_group() 
 * and ctk_radio_action_set_group() methods
 *
 * A common way to set up a group of radio actions is the following:
 * |[<!-- language="C" -->
 *   CtkRadioAction *action;
 *   CtkRadioAction *last_action;
 *  
 *   while ( ...more actions to add... /)
 *     {
 *        action = ctk_radio_action_new (...);
 *        
 *        ctk_radio_action_join_group (action, last_action);
 *        last_action = action;
 *     }
 * ]|
 * 
 * Since: 3.0
 *
 * Deprecated: 3.10
 */
void
ctk_radio_action_join_group (CtkRadioAction *action, 
			     CtkRadioAction *group_source)
{
  g_return_if_fail (CTK_IS_RADIO_ACTION (action));
  g_return_if_fail (group_source == NULL || CTK_IS_RADIO_ACTION (group_source));  

  if (group_source)
    {
      GSList *group;
      group = ctk_radio_action_get_group (group_source);
      
      if (!group)
        {
          /* if we are not already part of a group we need to set up a new one
             and then get the newly created group */  
          ctk_radio_action_set_group (group_source, NULL);
          group = ctk_radio_action_get_group (group_source);
        }

      ctk_radio_action_set_group (action, group);
    }
  else
    {
      ctk_radio_action_set_group (action, NULL);
    }
}

/**
 * ctk_radio_action_get_current_value:
 * @action: a #CtkRadioAction
 * 
 * Obtains the value property of the currently active member of 
 * the group to which @action belongs.
 * 
 * Returns: The value of the currently active group member
 *
 * Since: 2.4
 *
 * Deprecated: 3.10
 **/
gint
ctk_radio_action_get_current_value (CtkRadioAction *action)
{
  GSList *slist;

  g_return_val_if_fail (CTK_IS_RADIO_ACTION (action), 0);

  if (action->private_data->group)
    {
      for (slist = action->private_data->group; slist; slist = slist->next)
	{
	  CtkToggleAction *toggle_action = slist->data;

	  if (ctk_toggle_action_get_active (toggle_action))
	    return CTK_RADIO_ACTION (toggle_action)->private_data->value;
	}
    }

  return action->private_data->value;
}

/**
 * ctk_radio_action_set_current_value:
 * @action: a #CtkRadioAction
 * @current_value: the new value
 * 
 * Sets the currently active group member to the member with value
 * property @current_value.
 *
 * Since: 2.10
 *
 * Deprecated: 3.10
 **/
void
ctk_radio_action_set_current_value (CtkRadioAction *action,
                                    gint            current_value)
{
  GSList *slist;

  g_return_if_fail (CTK_IS_RADIO_ACTION (action));

  if (action->private_data->group)
    {
      for (slist = action->private_data->group; slist; slist = slist->next)
	{
	  CtkRadioAction *radio_action = slist->data;

	  if (radio_action->private_data->value == current_value)
            {
              ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (radio_action),
                                            TRUE);
              return;
            }
	}
    }

  if (action->private_data->value == current_value)
    ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action), TRUE);
  else
    g_warning ("Radio group does not contain an action with value '%d'",
	       current_value);
}
