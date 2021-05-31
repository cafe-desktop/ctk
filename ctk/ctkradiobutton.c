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

#include "ctkradiobutton.h"

#include "ctkcontainerprivate.h"
#include "ctkbuttonprivate.h"
#include "ctktogglebuttonprivate.h"
#include "ctkcheckbuttonprivate.h"
#include "ctklabel.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "a11y/ctkradiobuttonaccessible.h"
#include "ctkstylecontextprivate.h"

/**
 * SECTION:ctkradiobutton
 * @Short_description: A choice from multiple check buttons
 * @Title: CtkRadioButton
 * @See_also: #CtkComboBox
 *
 * A single radio button performs the same basic function as a #CtkCheckButton,
 * as its position in the object hierarchy reflects. It is only when multiple
 * radio buttons are grouped together that they become a different user
 * interface component in their own right.
 *
 * Every radio button is a member of some group of radio buttons. When one is
 * selected, all other radio buttons in the same group are deselected. A
 * #CtkRadioButton is one way of giving the user a choice from many options.
 *
 * Radio button widgets are created with ctk_radio_button_new(), passing %NULL
 * as the argument if this is the first radio button in a group. In subsequent
 * calls, the group you wish to add this button to should be passed as an
 * argument. Optionally, ctk_radio_button_new_with_label() can be used if you
 * want a text label on the radio button.
 *
 * Alternatively, when adding widgets to an existing group of radio buttons,
 * use ctk_radio_button_new_from_widget() with a #CtkRadioButton that already
 * has a group assigned to it. The convenience function
 * ctk_radio_button_new_with_label_from_widget() is also provided.
 *
 * To retrieve the group a #CtkRadioButton is assigned to, use
 * ctk_radio_button_get_group().
 *
 * To remove a #CtkRadioButton from one group and make it part of a new one,
 * use ctk_radio_button_set_group().
 *
 * The group list does not need to be freed, as each #CtkRadioButton will remove
 * itself and its list item when it is destroyed.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * radiobutton
 * ├── radio
 * ╰── <child>
 * ]|
 *
 * A CtkRadioButton with indicator (see ctk_toggle_button_set_mode()) has a
 * main CSS node with name radiobutton and a subnode with name radio.
 *
 * |[<!-- language="plain" -->
 * button.radio
 * ├── radio
 * ╰── <child>
 * ]|
 *
 * A CtkRadioButton without indicator changes the name of its main node
 * to button and adds a .radio style class to it. The subnode is invisible
 * in this case.
 *
 * ## How to create a group of two radio buttons.
 *
 * |[<!-- language="C" -->
 * void create_radio_buttons (void) {
 *
 *    CtkWidget *window, *radio1, *radio2, *box, *entry;
 *    window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
 *    box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
 *    ctk_box_set_homogeneous (CTK_BOX (box), TRUE);
 *
 *    // Create a radio button with a CtkEntry widget
 *    radio1 = ctk_radio_button_new (NULL);
 *    entry = ctk_entry_new ();
 *    ctk_container_add (CTK_CONTAINER (radio1), entry);
 *
 *
 *    // Create a radio button with a label
 *    radio2 = ctk_radio_button_new_with_label_from_widget (CTK_RADIO_BUTTON (radio1),
 *                                                          "I’m the second radio button.");
 *
 *    // Pack them into a box, then show all the widgets
 *    ctk_box_pack_start (CTK_BOX (box), radio1);
 *    ctk_box_pack_start (CTK_BOX (box), radio2);
 *    ctk_container_add (CTK_CONTAINER (window), box);
 *    ctk_widget_show_all (window);
 *    return;
 * }
 * ]|
 *
 * When an unselected button in the group is clicked the clicked button
 * receives the #CtkToggleButton::toggled signal, as does the previously
 * selected button.
 * Inside the #CtkToggleButton::toggled handler, ctk_toggle_button_get_active()
 * can be used to determine if the button has been selected or deselected.
 */


struct _CtkRadioButtonPrivate
{
  GSList *group;
};

enum {
  PROP_0,
  PROP_GROUP,
  LAST_PROP
};

static GParamSpec *radio_button_props[LAST_PROP] = { NULL, };

static void     ctk_radio_button_destroy        (CtkWidget           *widget);
static gboolean ctk_radio_button_focus          (CtkWidget           *widget,
						 CtkDirectionType     direction);
static void     ctk_radio_button_clicked        (CtkButton           *button);
static void     ctk_radio_button_set_property   (GObject             *object,
						 guint                prop_id,
						 const GValue        *value,
						 GParamSpec          *pspec);
static void     ctk_radio_button_get_property   (GObject             *object,
						 guint                prop_id,
						 GValue              *value,
						 GParamSpec          *pspec);

G_DEFINE_TYPE_WITH_PRIVATE (CtkRadioButton, ctk_radio_button, CTK_TYPE_CHECK_BUTTON)

static guint group_changed_signal = 0;

static void
ctk_radio_button_class_init (CtkRadioButtonClass *class)
{
  GObjectClass *gobject_class;
  CtkButtonClass *button_class;
  CtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (class);
  widget_class = (CtkWidgetClass*) class;
  button_class = (CtkButtonClass*) class;

  gobject_class->set_property = ctk_radio_button_set_property;
  gobject_class->get_property = ctk_radio_button_get_property;

  /**
   * CtkRadioButton:group:
   *
   * Sets a new group for a radio button.
   */
  radio_button_props[PROP_GROUP] =
      g_param_spec_object ("group",
                           P_("Group"),
                           P_("The radio button whose group this widget belongs to."),
                           CTK_TYPE_RADIO_BUTTON,
                           CTK_PARAM_WRITABLE);

  g_object_class_install_properties (gobject_class, LAST_PROP, radio_button_props);

  widget_class->destroy = ctk_radio_button_destroy;
  widget_class->focus = ctk_radio_button_focus;

  button_class->clicked = ctk_radio_button_clicked;

  class->group_changed = NULL;

  /**
   * CtkRadioButton::group-changed:
   * @button: the object which received the signal
   *
   * Emitted when the group of radio buttons that a radio button belongs
   * to changes. This is emitted when a radio button switches from
   * being alone to being part of a group of 2 or more buttons, or
   * vice-versa, and when a button is moved from one group of 2 or
   * more buttons to a different one, but not when the composition
   * of the group that a button belongs to changes.
   *
   * Since: 2.4
   */
  group_changed_signal = g_signal_new (I_("group-changed"),
				       G_OBJECT_CLASS_TYPE (gobject_class),
				       G_SIGNAL_RUN_FIRST,
				       G_STRUCT_OFFSET (CtkRadioButtonClass, group_changed),
				       NULL, NULL,
				       NULL,
				       G_TYPE_NONE, 0);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_RADIO_BUTTON_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "radiobutton");
}

static void
ctk_radio_button_init (CtkRadioButton *radio_button)
{
  CtkRadioButtonPrivate *priv;
  CtkCssNode *css_node;

  radio_button->priv = ctk_radio_button_get_instance_private (radio_button);
  priv = radio_button->priv;

  ctk_widget_set_receives_default (CTK_WIDGET (radio_button), FALSE);

  _ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (radio_button), TRUE);

  priv->group = g_slist_prepend (NULL, radio_button);

  css_node = ctk_check_button_get_indicator_node (CTK_CHECK_BUTTON (radio_button));
  ctk_css_node_set_name (css_node, I_("radio"));
}

static void
ctk_radio_button_set_property (GObject      *object,
			       guint         prop_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  CtkRadioButton *radio_button;

  radio_button = CTK_RADIO_BUTTON (object);

  switch (prop_id)
    {
      GSList *slist;
      CtkRadioButton *button;

    case PROP_GROUP:
        button = g_value_get_object (value);

      if (button)
	slist = ctk_radio_button_get_group (button);
      else
	slist = NULL;
      ctk_radio_button_set_group (radio_button, slist);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_radio_button_get_property (GObject    *object,
			       guint       prop_id,
			       GValue     *value,
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
 * ctk_radio_button_set_group:
 * @radio_button: a #CtkRadioButton.
 * @group: (element-type CtkRadioButton) (allow-none): an existing radio
 *     button group, such as one returned from ctk_radio_button_get_group(), or %NULL.
 *
 * Sets a #CtkRadioButton’s group. It should be noted that this does not change
 * the layout of your interface in any way, so if you are changing the group,
 * it is likely you will need to re-arrange the user interface to reflect these
 * changes.
 */
void
ctk_radio_button_set_group (CtkRadioButton *radio_button,
			    GSList         *group)
{
  CtkRadioButtonPrivate *priv;
  CtkWidget *old_group_singleton = NULL;
  CtkWidget *new_group_singleton = NULL;

  g_return_if_fail (CTK_IS_RADIO_BUTTON (radio_button));

  if (g_slist_find (group, radio_button))
    return;

  priv = radio_button->priv;

  if (priv->group)
    {
      GSList *slist;

      priv->group = g_slist_remove (priv->group, radio_button);

      if (priv->group && !priv->group->next)
	old_group_singleton = g_object_ref (priv->group->data);

      for (slist = priv->group; slist; slist = slist->next)
	{
	  CtkRadioButton *tmp_button;
	  
	  tmp_button = slist->data;

	  tmp_button->priv->group = priv->group;
	}
    }
  
  if (group && !group->next)
    new_group_singleton = g_object_ref (group->data);

  priv->group = g_slist_prepend (group, radio_button);

  if (group)
    {
      GSList *slist;
      
      for (slist = group; slist; slist = slist->next)
	{
	  CtkRadioButton *tmp_button;
	  
	  tmp_button = slist->data;

	  tmp_button->priv->group = priv->group;
	}
    }

  g_object_ref (radio_button);
  
  g_object_notify_by_pspec (G_OBJECT (radio_button), radio_button_props[PROP_GROUP]);
  g_signal_emit (radio_button, group_changed_signal, 0);
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

  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (radio_button), group == NULL);

  g_object_unref (radio_button);
}

/**
 * ctk_radio_button_join_group:
 * @radio_button: the #CtkRadioButton object
 * @group_source: (allow-none): a radio button object whos group we are 
 *   joining, or %NULL to remove the radio button from its group
 *
 * Joins a #CtkRadioButton object to the group of another #CtkRadioButton object
 *
 * Use this in language bindings instead of the ctk_radio_button_get_group() 
 * and ctk_radio_button_set_group() methods
 *
 * A common way to set up a group of radio buttons is the following:
 * |[<!-- language="C" -->
 *   CtkRadioButton *radio_button;
 *   CtkRadioButton *last_button;
 *
 *   while (some_condition)
 *     {
 *        radio_button = ctk_radio_button_new (NULL);
 *
 *        ctk_radio_button_join_group (radio_button, last_button);
 *        last_button = radio_button;
 *     }
 * ]|
 *
 * Since: 3.0
 */
void
ctk_radio_button_join_group (CtkRadioButton *radio_button, 
			     CtkRadioButton *group_source)
{
  g_return_if_fail (CTK_IS_RADIO_BUTTON (radio_button));
  g_return_if_fail (group_source == NULL || CTK_IS_RADIO_BUTTON (group_source));

  if (group_source)
    {
      GSList *group;
      group = ctk_radio_button_get_group (group_source);

      if (!group)
        {
          /* if we are not already part of a group we need to set up a new one
             and then get the newly created group */
          ctk_radio_button_set_group (group_source, NULL);
          group = ctk_radio_button_get_group (group_source);
        }

      ctk_radio_button_set_group (radio_button, group);
    }
  else
    {
      ctk_radio_button_set_group (radio_button, NULL);
    }
}

/**
 * ctk_radio_button_new:
 * @group: (element-type CtkRadioButton) (allow-none): an existing
 *         radio button group, or %NULL if you are creating a new group.
 *
 * Creates a new #CtkRadioButton. To be of any practical value, a widget should
 * then be packed into the radio button.
 *
 * Returns: a new radio button
 */
CtkWidget*
ctk_radio_button_new (GSList *group)
{
  CtkRadioButton *radio_button;

  radio_button = g_object_new (CTK_TYPE_RADIO_BUTTON, NULL);

  if (group)
    ctk_radio_button_set_group (radio_button, group);

  return CTK_WIDGET (radio_button);
}

/**
 * ctk_radio_button_new_with_label:
 * @group: (element-type CtkRadioButton) (allow-none): an existing
 *         radio button group, or %NULL if you are creating a new group.
 * @label: the text label to display next to the radio button.
 *
 * Creates a new #CtkRadioButton with a text label.
 *
 * Returns: a new radio button.
 */
CtkWidget*
ctk_radio_button_new_with_label (GSList      *group,
				 const gchar *label)
{
  CtkWidget *radio_button;

  radio_button = g_object_new (CTK_TYPE_RADIO_BUTTON, "label", label, NULL) ;

  if (group)
    ctk_radio_button_set_group (CTK_RADIO_BUTTON (radio_button), group);

  return radio_button;
}


/**
 * ctk_radio_button_new_with_mnemonic:
 * @group: (element-type CtkRadioButton) (allow-none): the radio button
 *         group, or %NULL
 * @label: the text of the button, with an underscore in front of the
 *         mnemonic character
 *
 * Creates a new #CtkRadioButton containing a label, adding it to the same
 * group as @group. The label will be created using
 * ctk_label_new_with_mnemonic(), so underscores in @label indicate the
 * mnemonic for the button.
 *
 * Returns: a new #CtkRadioButton
 */
CtkWidget*
ctk_radio_button_new_with_mnemonic (GSList      *group,
				    const gchar *label)
{
  CtkWidget *radio_button;

  radio_button = g_object_new (CTK_TYPE_RADIO_BUTTON, 
			       "label", label, 
			       "use-underline", TRUE, 
			       NULL);

  if (group)
    ctk_radio_button_set_group (CTK_RADIO_BUTTON (radio_button), group);

  return radio_button;
}

/**
 * ctk_radio_button_new_from_widget: (constructor)
 * @radio_group_member: (allow-none): an existing #CtkRadioButton.
 *
 * Creates a new #CtkRadioButton, adding it to the same group as
 * @radio_group_member. As with ctk_radio_button_new(), a widget
 * should be packed into the radio button.
 *
 * Returns: (transfer none): a new radio button.
 */
CtkWidget*
ctk_radio_button_new_from_widget (CtkRadioButton *radio_group_member)
{
  GSList *l = NULL;
  if (radio_group_member)
    l = ctk_radio_button_get_group (radio_group_member);
  return ctk_radio_button_new (l);
}

/**
 * ctk_radio_button_new_with_label_from_widget: (constructor)
 * @radio_group_member: (allow-none): widget to get radio group from or %NULL
 * @label: a text string to display next to the radio button.
 *
 * Creates a new #CtkRadioButton with a text label, adding it to
 * the same group as @radio_group_member.
 *
 * Returns: (transfer none): a new radio button.
 */
CtkWidget*
ctk_radio_button_new_with_label_from_widget (CtkRadioButton *radio_group_member,
					     const gchar    *label)
{
  GSList *l = NULL;
  if (radio_group_member)
    l = ctk_radio_button_get_group (radio_group_member);
  return ctk_radio_button_new_with_label (l, label);
}

/**
 * ctk_radio_button_new_with_mnemonic_from_widget: (constructor)
 * @radio_group_member: (allow-none): widget to get radio group from or %NULL
 * @label: the text of the button, with an underscore in front of the
 *         mnemonic character
 *
 * Creates a new #CtkRadioButton containing a label. The label
 * will be created using ctk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the button.
 *
 * Returns: (transfer none): a new #CtkRadioButton
 **/
CtkWidget*
ctk_radio_button_new_with_mnemonic_from_widget (CtkRadioButton *radio_group_member,
					        const gchar    *label)
{
  GSList *l = NULL;
  if (radio_group_member)
    l = ctk_radio_button_get_group (radio_group_member);
  return ctk_radio_button_new_with_mnemonic (l, label);
}


/**
 * ctk_radio_button_get_group:
 * @radio_button: a #CtkRadioButton.
 *
 * Retrieves the group assigned to a radio button.
 *
 * Returns: (element-type CtkRadioButton) (transfer none): a linked list
 * containing all the radio buttons in the same group
 * as @radio_button. The returned list is owned by the radio button
 * and must not be modified or freed.
 */
GSList*
ctk_radio_button_get_group (CtkRadioButton *radio_button)
{
  g_return_val_if_fail (CTK_IS_RADIO_BUTTON (radio_button), NULL);

  return radio_button->priv->group;
}


static void
ctk_radio_button_destroy (CtkWidget *widget)
{
  CtkWidget *old_group_singleton = NULL;
  CtkRadioButton *radio_button = CTK_RADIO_BUTTON (widget);
  CtkRadioButtonPrivate *priv = radio_button->priv;
  CtkRadioButton *tmp_button;
  GSList *tmp_list;
  gboolean was_in_group;

  was_in_group = priv->group && priv->group->next;

  priv->group = g_slist_remove (priv->group, radio_button);
  if (priv->group && !priv->group->next)
    old_group_singleton = priv->group->data;

  tmp_list = priv->group;

  while (tmp_list)
    {
      tmp_button = tmp_list->data;
      tmp_list = tmp_list->next;

      tmp_button->priv->group = priv->group;
    }

  /* this button is no longer in the group */
  priv->group = NULL;

  if (old_group_singleton)
    g_signal_emit (old_group_singleton, group_changed_signal, 0);
  if (was_in_group)
    g_signal_emit (radio_button, group_changed_signal, 0);

  CTK_WIDGET_CLASS (ctk_radio_button_parent_class)->destroy (widget);
}

static gboolean
ctk_radio_button_focus (CtkWidget         *widget,
			CtkDirectionType   direction)
{
  CtkRadioButton *radio_button = CTK_RADIO_BUTTON (widget);
  CtkRadioButtonPrivate *priv = radio_button->priv;
  GSList *tmp_slist;

  /* Radio buttons with draw_indicator unset focus "normally", since
   * they look like buttons to the user.
   */
  if (!ctk_toggle_button_get_mode (CTK_TOGGLE_BUTTON (widget)))
    return CTK_WIDGET_CLASS (ctk_radio_button_parent_class)->focus (widget, direction);

  if (ctk_widget_is_focus (widget))
    {
      GList *children, *focus_list, *tmp_list;
      CtkWidget *toplevel;
      CtkWidget *new_focus = NULL;
      GSList *l;

      if (direction == CTK_DIR_TAB_FORWARD ||
          direction == CTK_DIR_TAB_BACKWARD)
        return FALSE;

      toplevel = ctk_widget_get_toplevel (widget);
      children = NULL;
      for (l = priv->group; l; l = l->next)
        children = g_list_prepend (children, l->data);

      focus_list = _ctk_container_focus_sort (CTK_CONTAINER (toplevel), children, direction, widget);
      tmp_list = g_list_find (focus_list, widget);

      if (tmp_list)
	{
	  tmp_list = tmp_list->next;

	  while (tmp_list)
	    {
	      CtkWidget *child = tmp_list->data;

	      if (ctk_widget_get_mapped (child) && ctk_widget_is_sensitive (child))
		{
		  new_focus = child;
		  break;
		}

	      tmp_list = tmp_list->next;
	    }
	}

      if (!new_focus)
	{
	  tmp_list = focus_list;

	  while (tmp_list)
	    {
	      CtkWidget *child = tmp_list->data;

	      if (ctk_widget_get_mapped (child) && ctk_widget_is_sensitive (child))
		{
		  new_focus = child;
		  break;
		}

	      tmp_list = tmp_list->next;
	    }
	}

      g_list_free (focus_list);
      g_list_free (children);

      if (new_focus)
	{
	  ctk_widget_grab_focus (new_focus);

          ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (new_focus), TRUE);
	}

      return TRUE;
    }
  else
    {
      CtkRadioButton *selected_button = NULL;

      /* We accept the focus if, we don't have the focus and
       *  - we are the currently active button in the group
       *  - there is no currently active radio button.
       */
      tmp_slist = priv->group;
      while (tmp_slist)
	{
	  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (tmp_slist->data)) &&
	      ctk_widget_get_visible (tmp_slist->data))
	    selected_button = tmp_slist->data;
	  tmp_slist = tmp_slist->next;
	}

      if (selected_button && selected_button != radio_button)
	return FALSE;

      ctk_widget_grab_focus (widget);
      return TRUE;
    }
}

static void
ctk_radio_button_clicked (CtkButton *button)
{
  CtkRadioButton *radio_button = CTK_RADIO_BUTTON (button);
  CtkRadioButtonPrivate *priv = radio_button->priv;
  CtkToggleButton *toggle_button = CTK_TOGGLE_BUTTON (button);
  CtkToggleButton *tmp_button;
  GSList *tmp_list;
  gint toggled;

  toggled = FALSE;

  g_object_ref (CTK_WIDGET (button));

  if (ctk_toggle_button_get_active (toggle_button))
    {
      tmp_button = NULL;
      tmp_list = priv->group;

      while (tmp_list)
	{
	  tmp_button = tmp_list->data;
	  tmp_list = tmp_list->next;

          if (tmp_button != toggle_button &&
              ctk_toggle_button_get_active (tmp_button))
	    break;

	  tmp_button = NULL;
	}

      if (tmp_button)
	{
	  toggled = TRUE;
          _ctk_toggle_button_set_active (toggle_button,
                                         !ctk_toggle_button_get_active (toggle_button));
	}
    }
  else
    {
      toggled = TRUE;
      _ctk_toggle_button_set_active (toggle_button,
                                     !ctk_toggle_button_get_active (toggle_button));

      tmp_list = priv->group;
      while (tmp_list)
	{
	  tmp_button = tmp_list->data;
	  tmp_list = tmp_list->next;

	  if (ctk_toggle_button_get_active (tmp_button) && (tmp_button != toggle_button))
	    {
	      ctk_button_clicked (CTK_BUTTON (tmp_button));
	      break;
	    }
	}
    }

  if (toggled)
    {
      ctk_toggle_button_toggled (toggle_button);

      g_object_notify (G_OBJECT (toggle_button), "active");
    }

  ctk_widget_queue_draw (CTK_WIDGET (button));

  g_object_unref (button);
}
