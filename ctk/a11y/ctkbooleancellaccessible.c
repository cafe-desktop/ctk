/* CTK+ - accessibility implementations
 * Copyright 2001 Sun Microsystems Inc.
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

#include <ctk/ctk.h>
#include <glib/gi18n-lib.h>
#include "ctkbooleancellaccessible.h"

struct _CtkBooleanCellAccessiblePrivate
{
  gboolean cell_value;
  gboolean cell_sensitive;
};

static AtkActionIface *parent_action_iface;

static gint
ctk_boolean_cell_accessible_get_n_actions (AtkAction *action)
{
  return parent_action_iface->get_n_actions (action) + 1;
}

static const gchar *
ctk_boolean_cell_accessible_get_description (AtkAction *action,
                                             gint       i)
{
  if (i == 0)
    return C_("Action description", "Toggles the cell");

  return parent_action_iface->get_description (action, i - 1);
}

static const gchar *
ctk_boolean_cell_accessible_action_get_name (AtkAction *action,
                                             gint       i)
{
  if (i == 0)
    return "toggle";

  return parent_action_iface->get_description (action, i - 1);
}

static const gchar *
ctk_boolean_cell_accessible_action_get_localized_name (AtkAction *action,
                                                       gint       i)
{
  if (i == 0)
    return C_("Action name", "Toggle");

  return parent_action_iface->get_description (action, i - 1);
}

static gboolean
ctk_boolean_cell_accessible_do_action (AtkAction *action,
                                       gint       i)
{
  if (i == 0)
    return parent_action_iface->do_action (action, 2);

  return parent_action_iface->do_action (action, i - 1);
}

static void
ctk_boolean_cell_accessible_action_interface_init (AtkActionIface *iface)
{
  parent_action_iface = g_type_interface_peek_parent (iface);

  iface->do_action = ctk_boolean_cell_accessible_do_action;
  iface->get_n_actions = ctk_boolean_cell_accessible_get_n_actions;
  iface->get_description = ctk_boolean_cell_accessible_get_description;
  iface->get_name = ctk_boolean_cell_accessible_action_get_name;
  iface->get_localized_name = ctk_boolean_cell_accessible_action_get_localized_name;
}


G_DEFINE_TYPE_WITH_CODE (CtkBooleanCellAccessible, ctk_boolean_cell_accessible, CTK_TYPE_RENDERER_CELL_ACCESSIBLE,
                         G_ADD_PRIVATE (CtkBooleanCellAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, ctk_boolean_cell_accessible_action_interface_init))


static AtkStateSet *
ctk_boolean_cell_accessible_ref_state_set (AtkObject *accessible)
{
  CtkBooleanCellAccessible *cell = CTK_BOOLEAN_CELL_ACCESSIBLE (accessible);
  AtkStateSet *state_set;

  state_set = ATK_OBJECT_CLASS (ctk_boolean_cell_accessible_parent_class)->ref_state_set (accessible);

  if (cell->priv->cell_value)
    atk_state_set_add_state (state_set, ATK_STATE_CHECKED);

  if (cell->priv->cell_sensitive)
    atk_state_set_add_state (state_set, ATK_STATE_SENSITIVE);
  else
    atk_state_set_remove_state (state_set, ATK_STATE_SENSITIVE);

  return state_set;
}

static void
ctk_boolean_cell_accessible_update_cache (CtkCellAccessible *cell,
                                          gboolean            emit_signal)
{
  CtkBooleanCellAccessible *boolean_cell = CTK_BOOLEAN_CELL_ACCESSIBLE (cell);
  gboolean active;
  gboolean sensitive;
  CtkCellRenderer *renderer;

  g_object_get (cell, "renderer", &renderer, NULL);
  g_object_get (renderer,
                "active", &active,
                "sensitive", &sensitive,
                NULL);
  g_object_unref (renderer);

  if (boolean_cell->priv->cell_value != active)
    {
      boolean_cell->priv->cell_value = !boolean_cell->priv->cell_value;

      if (emit_signal)
        atk_object_notify_state_change (ATK_OBJECT (cell), ATK_STATE_CHECKED, active);
    }

  if (boolean_cell->priv->cell_sensitive != sensitive)
    {
      boolean_cell->priv->cell_sensitive = !boolean_cell->priv->cell_sensitive;

      if (emit_signal)
        atk_object_notify_state_change (ATK_OBJECT (cell), ATK_STATE_SENSITIVE, sensitive);
    }
}

static void
ctk_boolean_cell_accessible_class_init (CtkBooleanCellAccessibleClass *klass)
{
  CtkCellAccessibleClass *cell_class = CTK_CELL_ACCESSIBLE_CLASS (klass);
  AtkObjectClass *atkobject_class = ATK_OBJECT_CLASS (klass);

  atkobject_class->ref_state_set = ctk_boolean_cell_accessible_ref_state_set;

  cell_class->update_cache = ctk_boolean_cell_accessible_update_cache;
}

static void
ctk_boolean_cell_accessible_init (CtkBooleanCellAccessible *cell)
{
  cell->priv = ctk_boolean_cell_accessible_get_instance_private (cell);
}

