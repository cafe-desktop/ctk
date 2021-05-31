/* CTK+ - accessibility implementations
 * Copyright (C) 2010  Intel Corporation
 * Copyright (C) 2010  RedHat, Inc.
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
 *
 * Author:
 *      Emmanuele Bassi <ebassi@linux.intel.com>
 *      Matthias Clasen <mclasen@redhat.com>
 *
 * Based on similar code from Mx.
 */

#include "config.h"

#include <glib/gi18n-lib.h>
#include <ctk/ctk.h>
#include "ctkintl.h"
#include "ctkswitchaccessible.h"


static void atk_action_interface_init (AtkActionIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkSwitchAccessible, ctk_switch_accessible, CTK_TYPE_WIDGET_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, atk_action_interface_init))

static AtkStateSet *
ctk_switch_accessible_ref_state_set (AtkObject *accessible)
{
  AtkStateSet *state_set;
  CtkWidget *widget;

  state_set = ATK_OBJECT_CLASS (ctk_switch_accessible_parent_class)->ref_state_set (accessible);

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return state_set;

  if (ctk_switch_get_active (CTK_SWITCH (widget)))
    atk_state_set_add_state (state_set, ATK_STATE_CHECKED);

  return state_set;
}

static void
ctk_switch_accessible_class_init (CtkSwitchAccessibleClass *klass)
{
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  atk_class->ref_state_set = ctk_switch_accessible_ref_state_set;
}

static void
ctk_switch_accessible_init (CtkSwitchAccessible *self)
{
}

static gint
ctk_switch_action_get_n_actions (AtkAction *action)
{
  return 1;
}

static const gchar *
ctk_switch_action_get_name (AtkAction *action,
                            gint       i)
{
  if (i == 0)
    return "toggle";
  return NULL;
}

static const gchar *
ctk_switch_action_get_localized_name (AtkAction *action,
                                      gint       i)
{
  if (i == 0)
    return C_("Action name", "Toggle");
  return NULL;
}

static const gchar *
ctk_switch_action_get_description (AtkAction *action,
                                   gint       i)
{
  if (i == 0)
    return C_("Action description", "Toggles the switch");
  return NULL;
}

static gboolean
ctk_switch_action_do_action (AtkAction *action,
                             gint       i)
{
  CtkSwitch *sw;
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (action));
  if (widget == NULL)
    return FALSE;

  if (i != 0)
    return FALSE;

  if (!ctk_widget_is_sensitive (widget) || !ctk_widget_get_visible (widget))
    return FALSE;

  sw = CTK_SWITCH (widget);
  ctk_switch_set_active (sw, !ctk_switch_get_active (sw));

  return TRUE;
}

static void
atk_action_interface_init (AtkActionIface *iface)
{
  iface->do_action = ctk_switch_action_do_action;
  iface->get_n_actions = ctk_switch_action_get_n_actions;
  iface->get_name = ctk_switch_action_get_name;
  iface->get_localized_name = ctk_switch_action_get_localized_name;
  iface->get_description = ctk_switch_action_get_description;
}
