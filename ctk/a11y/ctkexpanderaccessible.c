/* CTK+ - accessibility implementations
 * Copyright 2003 Sun Microsystems Inc.
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

#include <glib/gi18n-lib.h>
#include <ctk/ctk.h>
#include "ctkexpanderaccessible.h"

static void atk_action_interface_init (AtkActionIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkExpanderAccessible, ctk_expander_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, atk_action_interface_init))

static const gchar *
ctk_expander_accessible_get_full_text (CtkExpander *widget)
{
  CtkWidget *label_widget;

  label_widget = ctk_expander_get_label_widget (widget);

  if (!CTK_IS_LABEL (label_widget))
    return NULL;

  return ctk_label_get_text (CTK_LABEL (label_widget));
}

static const gchar *
ctk_expander_accessible_get_name (AtkObject *obj)
{
  CtkWidget *widget;
  const gchar *name;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  name = ATK_OBJECT_CLASS (ctk_expander_accessible_parent_class)->get_name (obj);
  if (name != NULL)
    return name;

  return ctk_expander_accessible_get_full_text (CTK_EXPANDER (widget));
}

static gint
ctk_expander_accessible_get_n_children (AtkObject *obj)
{
  CtkWidget *widget;
  GList *children;
  gint count = 0;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return 0;

  children = ctk_container_get_children (CTK_CONTAINER(widget));
  count = g_list_length (children);
  g_list_free (children);

  /* See if there is a label - if there is, reduce our count by 1
   * since we don't want the label included with the children.
   */
  if (ctk_expander_get_label_widget (CTK_EXPANDER (widget)))
    count -= 1;

  return count;
}

static AtkObject *
ctk_expander_accessible_ref_child (AtkObject *obj,
                                   gint       i)
{
  GList *children, *tmp_list;
  AtkObject *accessible;
  CtkWidget *widget;
  CtkWidget *label;
  gint index;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  children = ctk_container_get_children (CTK_CONTAINER (widget));

  /* See if there is a label - if there is, we need to skip it
   * since we don't want the label included with the children.
   */
  label = ctk_expander_get_label_widget (CTK_EXPANDER (widget));
  if (label)
    {
      for (index = 0; index <= i; index++)
        {
          tmp_list = g_list_nth (children, index);
          if (label == CTK_WIDGET (tmp_list->data))
            {
              i += 1;
              break;
            }
        }
    }

  tmp_list = g_list_nth (children, i);
  if (!tmp_list)
    {
      g_list_free (children);
      return NULL;
    }
  accessible = ctk_widget_get_accessible (CTK_WIDGET (tmp_list->data));

  g_list_free (children);
  g_object_ref (accessible);
  return accessible;
}

static void
ctk_expander_accessible_initialize (AtkObject *obj,
                                    gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_expander_accessible_parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_TOGGLE_BUTTON;
}

static void
ctk_expander_accessible_notify_ctk (GObject    *obj,
                                    GParamSpec *pspec)
{
  AtkObject* atk_obj;
  CtkExpander *expander;

  expander = CTK_EXPANDER (obj);
  atk_obj = ctk_widget_get_accessible (CTK_WIDGET (expander));
;
  if (g_strcmp0 (pspec->name, "label") == 0)
    {
      if (atk_obj->name == NULL)
        g_object_notify (G_OBJECT (atk_obj), "accessible-name");
      g_signal_emit_by_name (atk_obj, "visible-data-changed");
    }
  else if (g_strcmp0 (pspec->name, "expanded") == 0)
    {
      atk_object_notify_state_change (atk_obj, ATK_STATE_CHECKED,
                                      ctk_expander_get_expanded (expander));
      atk_object_notify_state_change (atk_obj, ATK_STATE_EXPANDED,
                                      ctk_expander_get_expanded (expander));
      g_signal_emit_by_name (atk_obj, "visible-data-changed");
    }
  else
    CTK_WIDGET_ACCESSIBLE_CLASS (ctk_expander_accessible_parent_class)->notify_ctk (obj, pspec);
}

static AtkStateSet *
ctk_expander_accessible_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set;
  CtkWidget *widget;
  CtkExpander *expander;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  state_set = ATK_OBJECT_CLASS (ctk_expander_accessible_parent_class)->ref_state_set (obj);

  expander = CTK_EXPANDER (widget);

  atk_state_set_add_state (state_set, ATK_STATE_EXPANDABLE);

  if (ctk_expander_get_expanded (expander))
    {
      atk_state_set_add_state (state_set, ATK_STATE_CHECKED);
      atk_state_set_add_state (state_set, ATK_STATE_EXPANDED);
    }

  return state_set;
}

static void
ctk_expander_accessible_class_init (CtkExpanderAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);
  CtkWidgetAccessibleClass *widget_class = (CtkWidgetAccessibleClass*)klass;

  widget_class->notify_ctk = ctk_expander_accessible_notify_ctk;

  class->get_name = ctk_expander_accessible_get_name;
  class->get_n_children = ctk_expander_accessible_get_n_children;
  class->ref_child = ctk_expander_accessible_ref_child;
  class->ref_state_set = ctk_expander_accessible_ref_state_set;

  class->initialize = ctk_expander_accessible_initialize;
}

static void
ctk_expander_accessible_init (CtkExpanderAccessible *expander)
{
}

static gboolean
ctk_expander_accessible_do_action (AtkAction *action,
                                   gint       i)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (action));
  if (widget == NULL)
    return FALSE;

  if (!ctk_widget_is_sensitive (widget) || !ctk_widget_get_visible (widget))
    return FALSE;

  if (i != 0)
    return FALSE;

  ctk_widget_activate (widget);
  return TRUE;
}

static gint
ctk_expander_accessible_get_n_actions (AtkAction *action)
{
  return 1;
}

static const gchar *
ctk_expander_accessible_get_keybinding (AtkAction *action,
                                        gint       i)
{
  gchar *return_value = NULL;
  CtkWidget *widget;
  CtkWidget *label;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (action));
  if (widget == NULL)
    return NULL;

  if (i != 0)
    return NULL;

  label = ctk_expander_get_label_widget (CTK_EXPANDER (widget));
  if (CTK_IS_LABEL (label))
    {
      guint key_val;

      key_val = ctk_label_get_mnemonic_keyval (CTK_LABEL (label));
      if (key_val != GDK_KEY_VoidSymbol)
        return_value = ctk_accelerator_name (key_val, GDK_MOD1_MASK);
    }

  return return_value;
}

static const gchar *
ctk_expander_accessible_action_get_name (AtkAction *action,
                                         gint       i)
{
  if (i == 0)
    return "activate";
  return NULL;
}

static const gchar *
ctk_expander_accessible_action_get_localized_name (AtkAction *action,
                                                   gint       i)
{
  if (i == 0)
    return C_("Action name", "Activate");
  return NULL;
}

static const gchar *
ctk_expander_accessible_action_get_description (AtkAction *action,
                                                gint       i)
{
  if (i == 0)
    return C_("Action description", "Activates the expander");
  return NULL;
}

static void
atk_action_interface_init (AtkActionIface *iface)
{
  iface->do_action = ctk_expander_accessible_do_action;
  iface->get_n_actions = ctk_expander_accessible_get_n_actions;
  iface->get_keybinding = ctk_expander_accessible_get_keybinding;
  iface->get_name = ctk_expander_accessible_action_get_name;
  iface->get_localized_name = ctk_expander_accessible_action_get_localized_name;
  iface->get_description = ctk_expander_accessible_action_get_description;
}
