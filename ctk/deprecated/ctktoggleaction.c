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

#include "ctkintl.h"
#include "ctktoggleaction.h"
#include "ctktoggletoolbutton.h"
#include "ctktogglebutton.h"
#include "ctkcheckmenuitem.h"
#include "ctkprivate.h"


/**
 * SECTION:ctktoggleaction
 * @Short_description: An action which can be toggled between two states
 * @Title: CtkToggleAction
 *
 * A #CtkToggleAction corresponds roughly to a #CtkCheckMenuItem. It has an
 * “active” state specifying whether the action has been checked or not.
 */

struct _CtkToggleActionPrivate
{
  guint active        : 1;
  guint draw_as_radio : 1;
};

enum 
{
  TOGGLED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_DRAW_AS_RADIO,
  PROP_ACTIVE
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkToggleAction, ctk_toggle_action, CTK_TYPE_ACTION)

static void ctk_toggle_action_activate     (CtkAction       *action);
static void set_property                   (GObject         *object,
					    guint            prop_id,
					    const GValue    *value,
					    GParamSpec      *pspec);
static void get_property                   (GObject         *object,
					    guint            prop_id,
					    GValue          *value,
					    GParamSpec      *pspec);
static CtkWidget *create_menu_item         (CtkAction       *action);


static guint         action_signals[LAST_SIGNAL] = { 0 };

static void
ctk_toggle_action_class_init (CtkToggleActionClass *klass)
{
  GObjectClass *gobject_class;
  CtkActionClass *action_class;

  gobject_class = G_OBJECT_CLASS (klass);
  action_class = CTK_ACTION_CLASS (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  action_class->activate = ctk_toggle_action_activate;

  action_class->menu_item_type = CTK_TYPE_CHECK_MENU_ITEM;
  action_class->toolbar_item_type = CTK_TYPE_TOGGLE_TOOL_BUTTON;

  action_class->create_menu_item = create_menu_item;

  klass->toggled = NULL;

  /**
   * CtkToggleAction:draw-as-radio:
   *
   * Whether the proxies for this action look like radio action proxies.
   *
   * This is an appearance property and thus only applies if 
   * #CtkActivatable:use-action-appearance is %TRUE.
   *
   * Deprecated: 3.10
   */
  g_object_class_install_property (gobject_class,
                                   PROP_DRAW_AS_RADIO,
                                   g_param_spec_boolean ("draw-as-radio",
                                                         P_("Create the same proxies as a radio action"),
                                                         P_("Whether the proxies for this action look like radio action proxies"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE));

  /**
   * CtkToggleAction:active:
   *
   * Whether the toggle action should be active.
   *
   * Since: 2.10
   *
   * Deprecated: 3.10
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         P_("Active"),
                                                         P_("Whether the toggle action should be active"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE));
  /**
   * CtkToggleAction::toggled:
   * @toggleaction: the object which received the signal.
   *
   * Should be connected if you wish to perform an action
   * whenever the #CtkToggleAction state is changed.
   *
   * Deprecated: 3.10
   */
  action_signals[TOGGLED] =
    g_signal_new (I_("toggled"),
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkToggleActionClass, toggled),
		  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
}

static void
ctk_toggle_action_init (CtkToggleAction *action)
{
  action->private_data = ctk_toggle_action_get_instance_private (action);
  action->private_data->active = FALSE;
  action->private_data->draw_as_radio = FALSE;
}

/**
 * ctk_toggle_action_new:
 * @name: A unique name for the action
 * @label: (allow-none): The label displayed in menu items and on buttons,
 *         or %NULL
 * @tooltip: (allow-none): A tooltip for the action, or %NULL
 * @stock_id: (allow-none): The stock icon to display in widgets representing
 *            the action, or %NULL
 *
 * Creates a new #CtkToggleAction object. To add the action to
 * a #CtkActionGroup and set the accelerator for the action,
 * call ctk_action_group_add_action_with_accel().
 *
 * Returns: a new #CtkToggleAction
 *
 * Since: 2.4
 *
 * Deprecated: 3.10
 */
CtkToggleAction *
ctk_toggle_action_new (const gchar *name,
		       const gchar *label,
		       const gchar *tooltip,
		       const gchar *stock_id)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (CTK_TYPE_TOGGLE_ACTION,
		       "name", name,
		       "label", label,
		       "tooltip", tooltip,
		       "stock-id", stock_id,
		       NULL);
}

static void
get_property (GObject     *object,
	      guint        prop_id,
	      GValue      *value,
	      GParamSpec  *pspec)
{
  CtkToggleAction *action = CTK_TOGGLE_ACTION (object);
  
  switch (prop_id)
    {
    case PROP_DRAW_AS_RADIO:
      g_value_set_boolean (value, ctk_toggle_action_get_draw_as_radio (action));
      break;
    case PROP_ACTIVE:
      g_value_set_boolean (value, ctk_toggle_action_get_active (action));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
set_property (GObject      *object,
	      guint         prop_id,
	      const GValue *value,
	      GParamSpec   *pspec)
{
  CtkToggleAction *action = CTK_TOGGLE_ACTION (object);
  
  switch (prop_id)
    {
    case PROP_DRAW_AS_RADIO:
      ctk_toggle_action_set_draw_as_radio (action, g_value_get_boolean (value));
      break;
    case PROP_ACTIVE:
      ctk_toggle_action_set_active (action, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_toggle_action_activate (CtkAction *action)
{
  CtkToggleAction *toggle_action;

  g_return_if_fail (CTK_IS_TOGGLE_ACTION (action));

  toggle_action = CTK_TOGGLE_ACTION (action);

  toggle_action->private_data->active = !toggle_action->private_data->active;

  g_object_notify (G_OBJECT (action), "active");

  ctk_toggle_action_toggled (toggle_action);
}

/**
 * ctk_toggle_action_toggled:
 * @action: the action object
 *
 * Emits the “toggled” signal on the toggle action.
 *
 * Since: 2.4
 *
 * Deprecated: 3.10
 */
void
ctk_toggle_action_toggled (CtkToggleAction *action)
{
  g_return_if_fail (CTK_IS_TOGGLE_ACTION (action));

  g_signal_emit (action, action_signals[TOGGLED], 0);
}

/**
 * ctk_toggle_action_set_active:
 * @action: the action object
 * @is_active: whether the action should be checked or not
 *
 * Sets the checked state on the toggle action.
 *
 * Since: 2.4
 *
 * Deprecated: 3.10
 */
void
ctk_toggle_action_set_active (CtkToggleAction *action, 
			      gboolean         is_active)
{
  g_return_if_fail (CTK_IS_TOGGLE_ACTION (action));

  is_active = is_active != FALSE;

  if (action->private_data->active != is_active)
    _ctk_action_emit_activate (CTK_ACTION (action));
}

/**
 * ctk_toggle_action_get_active:
 * @action: the action object
 *
 * Returns the checked state of the toggle action.
 *
 * Returns: the checked state of the toggle action
 *
 * Since: 2.4
 *
 * Deprecated: 3.10
 */
gboolean
ctk_toggle_action_get_active (CtkToggleAction *action)
{
  g_return_val_if_fail (CTK_IS_TOGGLE_ACTION (action), FALSE);

  return action->private_data->active;
}


/**
 * ctk_toggle_action_set_draw_as_radio:
 * @action: the action object
 * @draw_as_radio: whether the action should have proxies like a radio 
 *    action
 *
 * Sets whether the action should have proxies like a radio action.
 *
 * Since: 2.4
 *
 * Deprecated: 3.10
 */
void
ctk_toggle_action_set_draw_as_radio (CtkToggleAction *action, 
				     gboolean         draw_as_radio)
{
  g_return_if_fail (CTK_IS_TOGGLE_ACTION (action));

  draw_as_radio = draw_as_radio != FALSE;

  if (action->private_data->draw_as_radio != draw_as_radio)
    {
      action->private_data->draw_as_radio = draw_as_radio;
      
      g_object_notify (G_OBJECT (action), "draw-as-radio");      
    }
}

/**
 * ctk_toggle_action_get_draw_as_radio:
 * @action: the action object
 *
 * Returns whether the action should have proxies like a radio action.
 *
 * Returns: whether the action should have proxies like a radio action.
 *
 * Since: 2.4
 *
 * Deprecated: 3.10
 */
gboolean
ctk_toggle_action_get_draw_as_radio (CtkToggleAction *action)
{
  g_return_val_if_fail (CTK_IS_TOGGLE_ACTION (action), FALSE);

  return action->private_data->draw_as_radio;
}

static CtkWidget *
create_menu_item (CtkAction *action)
{
  CtkToggleAction *toggle_action = CTK_TOGGLE_ACTION (action);

  return g_object_new (CTK_TYPE_CHECK_MENU_ITEM, 
		       "draw-as-radio", toggle_action->private_data->draw_as_radio,
		       NULL);
}


/* Private */

/*
 * _ctk_toggle_action_set_active:
 * @toggle_action: a #CtkToggleAction
 * @is_active: whether the action is active or not
 *
 * Sets the #CtkToggleAction:active property directly. This function does
 * not emit signals or notifications: it is left to the caller to do so.
 *
 * Deprecated: 3.10
 */
void
_ctk_toggle_action_set_active (CtkToggleAction *toggle_action,
                               gboolean         is_active)
{
  CtkToggleActionPrivate *priv = toggle_action->private_data;

  priv->active = is_active;
}
