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
#include "ctkaccellabel.h"
#include "ctkcheckmenuitemprivate.h"
#include "ctkmarshalers.h"
#include "ctkradiomenuitem.h"
#include "ctkactivatable.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "a11y/ctkradiomenuitemaccessible.h"

/**
 * SECTION:ctkradiomenuitem
 * @Short_description: A choice from multiple check menu items
 * @Title: CtkRadioMenuItem
 * @See_also: #CtkMenuItem, #CtkCheckMenuItem
 *
 * A radio menu item is a check menu item that belongs to a group. At each
 * instant exactly one of the radio menu items from a group is selected.
 *
 * The group list does not need to be freed, as each #CtkRadioMenuItem will
 * remove itself and its list item when it is destroyed.
 *
 * The correct way to create a group of radio menu items is approximatively
 * this:
 *
 * ## How to create a group of radio menu items.
 *
 * |[<!-- language="C" -->
 * GSList *group = NULL;
 * CtkWidget *item;
 * gint i;
 *
 * for (i = 0; i < 5; i++)
 * {
 *   item = ctk_radio_menu_item_new_with_label (group, "This is an example");
 *   group = ctk_radio_menu_item_get_group (CTK_RADIO_MENU_ITEM (item));
 *   if (i == 1)
 *     ctk_check_menu_item_set_active (CTK_CHECK_MENU_ITEM (item), TRUE);
 * }
 * ]|
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * menuitem
 * ├── radio.left
 * ╰── <child>
 * ]|
 *
 * CtkRadioMenuItem has a main CSS node with name menuitem, and a subnode
 * with name radio, which gets the .left or .right style class.
 */

struct _CtkRadioMenuItemPrivate
{
  GSList *group;
};

enum {
  PROP_0,
  PROP_GROUP
};


static void ctk_radio_menu_item_destroy        (CtkWidget             *widget);
static void ctk_radio_menu_item_activate       (CtkMenuItem           *menu_item);
static void ctk_radio_menu_item_set_property   (GObject               *object,
						guint                  prop_id,
						const GValue          *value,
						GParamSpec            *pspec);
static void ctk_radio_menu_item_get_property   (GObject               *object,
						guint                  prop_id,
						GValue                *value,
						GParamSpec            *pspec);

static guint group_changed_signal = 0;

G_DEFINE_TYPE_WITH_PRIVATE (CtkRadioMenuItem, ctk_radio_menu_item, CTK_TYPE_CHECK_MENU_ITEM)

/**
 * ctk_radio_menu_item_new:
 * @group: (element-type CtkRadioMenuItem) (allow-none): the group to which the
 *   radio menu item is to be attached, or %NULL
 *
 * Creates a new #CtkRadioMenuItem.
 *
 * Returns: a new #CtkRadioMenuItem
 */
CtkWidget*
ctk_radio_menu_item_new (GSList *group)
{
  CtkRadioMenuItem *radio_menu_item;

  radio_menu_item = g_object_new (CTK_TYPE_RADIO_MENU_ITEM, NULL);

  ctk_radio_menu_item_set_group (radio_menu_item, group);

  return CTK_WIDGET (radio_menu_item);
}

static void
ctk_radio_menu_item_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
  CtkRadioMenuItem *radio_menu_item;

  radio_menu_item = CTK_RADIO_MENU_ITEM (object);

  switch (prop_id)
    {
      GSList *slist;

    case PROP_GROUP:
      slist = g_value_get_object (value);
      if (slist)
        slist = ctk_radio_menu_item_get_group ((CtkRadioMenuItem*) g_value_get_object (value));
      ctk_radio_menu_item_set_group (radio_menu_item, slist);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_radio_menu_item_get_property (GObject    *object,
				  guint       prop_id,
				  GValue     *value G_GNUC_UNUSED,
				  GParamSpec *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * ctk_radio_menu_item_set_group:
 * @radio_menu_item: a #CtkRadioMenuItem.
 * @group: (element-type CtkRadioMenuItem) (allow-none): the new group, or %NULL.
 *
 * Sets the group of a radio menu item, or changes it.
 */
void
ctk_radio_menu_item_set_group (CtkRadioMenuItem *radio_menu_item,
			       GSList           *group)
{
  CtkRadioMenuItemPrivate *priv;
  CtkWidget *old_group_singleton = NULL;
  CtkWidget *new_group_singleton = NULL;

  g_return_if_fail (CTK_IS_RADIO_MENU_ITEM (radio_menu_item));

  priv = radio_menu_item->priv;

  if (priv->group == group)
    return;

  if (priv->group)
    {
      GSList *slist;

      priv->group = g_slist_remove (priv->group, radio_menu_item);

      if (priv->group && !priv->group->next)
	old_group_singleton = g_object_ref (priv->group->data);

      for (slist = priv->group; slist; slist = slist->next)
	{
	  CtkRadioMenuItem *tmp_item;
	  
	  tmp_item = slist->data;

	  tmp_item->priv->group = priv->group;
	}
    }
  
  if (group && !group->next)
    new_group_singleton = g_object_ref (group->data);

  priv->group = g_slist_prepend (group, radio_menu_item);

  if (group)
    {
      GSList *slist;
      
      for (slist = group; slist; slist = slist->next)
	{
	  CtkRadioMenuItem *tmp_item;
	  
	  tmp_item = slist->data;

	  tmp_item->priv->group = priv->group;
	}

      _ctk_check_menu_item_set_active (CTK_CHECK_MENU_ITEM (radio_menu_item), FALSE);
    }
  else
    {
      _ctk_check_menu_item_set_active (CTK_CHECK_MENU_ITEM (radio_menu_item), TRUE);
      /* ctk_widget_set_state (CTK_WIDGET (radio_menu_item), CTK_STATE_ACTIVE);
       */
    }

  g_object_ref (radio_menu_item);

  g_object_notify (G_OBJECT (radio_menu_item), "group");
  g_signal_emit (radio_menu_item, group_changed_signal, 0);
  if (old_group_singleton)
    {
      g_signal_emit (old_group_singleton, group_changed_signal, 0);
      g_object_unref (old_group_singleton);
    }
  if (new_group_singleton)
    {
      g_signal_emit (new_group_singleton, group_changed_signal, 0);
      g_object_unref (new_group_singleton);
    }

  g_object_unref (radio_menu_item);
}


/**
 * ctk_radio_menu_item_new_with_label:
 * @group: (element-type CtkRadioMenuItem) (allow-none):
 *         group the radio menu item is inside, or %NULL
 * @label: the text for the label
 *
 * Creates a new #CtkRadioMenuItem whose child is a simple #CtkLabel.
 *
 * Returns: (transfer none): A new #CtkRadioMenuItem
 */
CtkWidget*
ctk_radio_menu_item_new_with_label (GSList *group,
				    const gchar *label)
{
  return g_object_new (CTK_TYPE_RADIO_MENU_ITEM,
          "group", (group) ? group->data : NULL,
          "label", label,
          NULL);
}


/**
 * ctk_radio_menu_item_new_with_mnemonic:
 * @group: (element-type CtkRadioMenuItem) (allow-none):
 *         group the radio menu item is inside, or %NULL
 * @label: the text of the button, with an underscore in front of the
 *         mnemonic character
 *
 * Creates a new #CtkRadioMenuItem containing a label. The label
 * will be created using ctk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the menu item.
 *
 * Returns: a new #CtkRadioMenuItem
 */
CtkWidget*
ctk_radio_menu_item_new_with_mnemonic (GSList *group,
				       const gchar *label)
{
  return g_object_new (CTK_TYPE_RADIO_MENU_ITEM,
          "group", (group) ? group->data : NULL,
          "label", label,
          "use-underline", TRUE,
          NULL);
}

/**
 * ctk_radio_menu_item_new_from_widget: (constructor)
 * @group: (allow-none): An existing #CtkRadioMenuItem
 *
 * Creates a new #CtkRadioMenuItem adding it to the same group as @group.
 *
 * Returns: (transfer none): The new #CtkRadioMenuItem
 *
 * Since: 2.4
 **/
CtkWidget *
ctk_radio_menu_item_new_from_widget (CtkRadioMenuItem *group)
{
  GSList *list = NULL;
  
  g_return_val_if_fail (group == NULL || CTK_IS_RADIO_MENU_ITEM (group), NULL);

  if (group)
    list = ctk_radio_menu_item_get_group (group);
  
  return ctk_radio_menu_item_new (list);
}

/**
 * ctk_radio_menu_item_new_with_mnemonic_from_widget: (constructor)
 * @group: (allow-none): An existing #CtkRadioMenuItem
 * @label: (allow-none): the text of the button, with an underscore in front of the
 *         mnemonic character
 *
 * Creates a new CtkRadioMenuItem containing a label. The label will be
 * created using ctk_label_new_with_mnemonic(), so underscores in label
 * indicate the mnemonic for the menu item.
 *
 * The new #CtkRadioMenuItem is added to the same group as @group.
 *
 * Returns: (transfer none): The new #CtkRadioMenuItem
 *
 * Since: 2.4
 **/
CtkWidget *
ctk_radio_menu_item_new_with_mnemonic_from_widget (CtkRadioMenuItem *group,
						   const gchar *label)
{
  GSList *list = NULL;

  g_return_val_if_fail (group == NULL || CTK_IS_RADIO_MENU_ITEM (group), NULL);

  if (group)
    list = ctk_radio_menu_item_get_group (group);

  return ctk_radio_menu_item_new_with_mnemonic (list, label);
}

/**
 * ctk_radio_menu_item_new_with_label_from_widget: (constructor)
 * @group: (allow-none): an existing #CtkRadioMenuItem
 * @label: (allow-none): the text for the label
 *
 * Creates a new CtkRadioMenuItem whose child is a simple CtkLabel.
 * The new #CtkRadioMenuItem is added to the same group as @group.
 *
 * Returns: (transfer none): The new #CtkRadioMenuItem
 *
 * Since: 2.4
 **/
CtkWidget *
ctk_radio_menu_item_new_with_label_from_widget (CtkRadioMenuItem *group,
						const gchar *label)
{
  GSList *list = NULL;

  g_return_val_if_fail (group == NULL || CTK_IS_RADIO_MENU_ITEM (group), NULL);

  if (group)
    list = ctk_radio_menu_item_get_group (group);

  return ctk_radio_menu_item_new_with_label (list, label);
}

/**
 * ctk_radio_menu_item_get_group:
 * @radio_menu_item: a #CtkRadioMenuItem
 *
 * Returns the group to which the radio menu item belongs, as a #GList of
 * #CtkRadioMenuItem. The list belongs to CTK+ and should not be freed.
 *
 * Returns: (element-type CtkRadioMenuItem) (transfer none): the group
 *     of @radio_menu_item
 */
GSList*
ctk_radio_menu_item_get_group (CtkRadioMenuItem *radio_menu_item)
{
  g_return_val_if_fail (CTK_IS_RADIO_MENU_ITEM (radio_menu_item), NULL);

  return radio_menu_item->priv->group;
}

static void
ctk_radio_menu_item_class_init (CtkRadioMenuItemClass *klass)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;
  CtkMenuItemClass *menu_item_class;

  gobject_class = G_OBJECT_CLASS (klass);
  widget_class = CTK_WIDGET_CLASS (klass);
  menu_item_class = CTK_MENU_ITEM_CLASS (klass);

  gobject_class->set_property = ctk_radio_menu_item_set_property;
  gobject_class->get_property = ctk_radio_menu_item_get_property;

  widget_class->destroy = ctk_radio_menu_item_destroy;

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_RADIO_MENU_ITEM_ACCESSIBLE);

  menu_item_class->activate = ctk_radio_menu_item_activate;

  /**
   * CtkRadioMenuItem:group:
   * 
   * The radio menu item whose group this widget belongs to.
   * 
   * Since: 2.8
   */
  g_object_class_install_property (gobject_class,
				   PROP_GROUP,
				   g_param_spec_object ("group",
							P_("Group"),
							P_("The radio menu item whose group this widget belongs to."),
							CTK_TYPE_RADIO_MENU_ITEM,
							CTK_PARAM_WRITABLE));

  /**
   * CtkStyle::group-changed:
   * @style: the object which received the signal
   *
   * Emitted when the group of radio menu items that a radio menu item belongs
   * to changes. This is emitted when a radio menu item switches from
   * being alone to being part of a group of 2 or more menu items, or
   * vice-versa, and when a button is moved from one group of 2 or
   * more menu items ton a different one, but not when the composition
   * of the group that a menu item belongs to changes.
   *
   * Since: 2.4
   */
  group_changed_signal = g_signal_new (I_("group-changed"),
				       G_OBJECT_CLASS_TYPE (gobject_class),
				       G_SIGNAL_RUN_FIRST,
				       G_STRUCT_OFFSET (CtkRadioMenuItemClass, group_changed),
				       NULL, NULL,
				       NULL,
				       G_TYPE_NONE, 0);
}

static void
ctk_radio_menu_item_init (CtkRadioMenuItem *radio_menu_item)
{
  CtkRadioMenuItemPrivate *priv;

  radio_menu_item->priv = ctk_radio_menu_item_get_instance_private (radio_menu_item);
  priv = radio_menu_item->priv;

  priv->group = g_slist_prepend (NULL, radio_menu_item);
  ctk_check_menu_item_set_draw_as_radio (CTK_CHECK_MENU_ITEM (radio_menu_item), TRUE);
}

static void
ctk_radio_menu_item_destroy (CtkWidget *widget)
{
  CtkRadioMenuItem *radio_menu_item = CTK_RADIO_MENU_ITEM (widget);
  CtkRadioMenuItemPrivate *priv = radio_menu_item->priv;
  CtkWidget *old_group_singleton = NULL;
  CtkRadioMenuItem *tmp_menu_item;
  GSList *tmp_list;
  gboolean was_in_group;

  was_in_group = priv->group && priv->group->next;

  priv->group = g_slist_remove (priv->group, radio_menu_item);
  if (priv->group && !priv->group->next)
    old_group_singleton = priv->group->data;

  tmp_list = priv->group;

  while (tmp_list)
    {
      tmp_menu_item = tmp_list->data;
      tmp_list = tmp_list->next;

      tmp_menu_item->priv->group = priv->group;
    }

  /* this radio menu item is no longer in the group */
  priv->group = NULL;
  
  if (old_group_singleton)
    g_signal_emit (old_group_singleton, group_changed_signal, 0);
  if (was_in_group)
    g_signal_emit (radio_menu_item, group_changed_signal, 0);

  CTK_WIDGET_CLASS (ctk_radio_menu_item_parent_class)->destroy (widget);
}

static void
ctk_radio_menu_item_activate (CtkMenuItem *menu_item)
{
  CtkRadioMenuItem *radio_menu_item = CTK_RADIO_MENU_ITEM (menu_item);
  CtkRadioMenuItemPrivate *priv = radio_menu_item->priv;
  CtkCheckMenuItem *check_menu_item = CTK_CHECK_MENU_ITEM (menu_item);
  CtkCheckMenuItem *tmp_menu_item;
  CtkAction        *action;
  GSList *tmp_list;
  gboolean active;
  gint toggled;

  action = ctk_activatable_get_related_action (CTK_ACTIVATABLE (menu_item));
  if (action && ctk_menu_item_get_submenu (menu_item) == NULL)
    ctk_action_activate (action);

  toggled = FALSE;

  active = ctk_check_menu_item_get_active (check_menu_item);
  if (active)
    {
      tmp_menu_item = NULL;
      tmp_list = priv->group;

      while (tmp_list)
	{
	  tmp_menu_item = tmp_list->data;
	  tmp_list = tmp_list->next;

          if (ctk_check_menu_item_get_active (tmp_menu_item) &&
              tmp_menu_item != check_menu_item)
	    break;

	  tmp_menu_item = NULL;
	}

      if (tmp_menu_item)
	{
	  toggled = TRUE;
          _ctk_check_menu_item_set_active (check_menu_item, !active);
	}
    }
  else
    {
      toggled = TRUE;
      _ctk_check_menu_item_set_active (check_menu_item, !active);

      tmp_list = priv->group;
      while (tmp_list)
	{
	  tmp_menu_item = tmp_list->data;
	  tmp_list = tmp_list->next;

          if (ctk_check_menu_item_get_active (tmp_menu_item) &&
              tmp_menu_item != check_menu_item)
	    {
              ctk_menu_item_activate (CTK_MENU_ITEM (tmp_menu_item));
	      break;
	    }
	}
    }

  if (toggled)
    {
      ctk_check_menu_item_toggled (check_menu_item);
    }

  ctk_widget_queue_draw (CTK_WIDGET (radio_menu_item));
}

/**
 * ctk_radio_menu_item_join_group:
 * @radio_menu_item: a #CtkRadioMenuItem
 * @group_source: (allow-none): a #CtkRadioMenuItem whose group we are
 *   joining, or %NULL to remove the @radio_menu_item from its current
 *   group
 *
 * Joins a #CtkRadioMenuItem object to the group of another #CtkRadioMenuItem
 * object.
 *
 * This function should be used by language bindings to avoid the memory
 * manangement of the opaque #GSList of ctk_radio_menu_item_get_group()
 * and ctk_radio_menu_item_set_group().
 *
 * A common way to set up a group of #CtkRadioMenuItem instances is:
 *
 * |[
 *   CtkRadioMenuItem *last_item = NULL;
 *
 *   while ( ...more items to add... )
 *     {
 *       CtkRadioMenuItem *radio_item;
 *
 *       radio_item = ctk_radio_menu_item_new (...);
 *
 *       ctk_radio_menu_item_join_group (radio_item, last_item);
 *       last_item = radio_item;
 *     }
 * ]|
 *
 * Since: 3.18
 */
void
ctk_radio_menu_item_join_group (CtkRadioMenuItem *radio_menu_item,
                                CtkRadioMenuItem *group_source)
{
  g_return_if_fail (CTK_IS_RADIO_MENU_ITEM (radio_menu_item));
  g_return_if_fail (group_source == NULL || CTK_IS_RADIO_MENU_ITEM (group_source));

  if (group_source != NULL)
    {
      GSList *group = ctk_radio_menu_item_get_group (group_source);

      if (group == NULL)
        {
          /* if the group source does not have a group, we force one */
          ctk_radio_menu_item_set_group (group_source, NULL);
          group = ctk_radio_menu_item_get_group (group_source);
        }

      ctk_radio_menu_item_set_group (radio_menu_item, group);
    }
  else
    ctk_radio_menu_item_set_group (radio_menu_item, NULL);
}
