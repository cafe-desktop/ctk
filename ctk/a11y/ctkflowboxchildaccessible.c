/*
 * Copyright (C) 2013 Red Hat, Inc.
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

#include "ctkflowboxchildaccessible.h"

#include "ctk/ctkflowbox.h"


G_DEFINE_TYPE (CtkFlowBoxChildAccessible, ctk_flow_box_child_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE)

static void
ctk_flow_box_child_accessible_init (CtkFlowBoxChildAccessible *accessible G_GNUC_UNUSED)
{
}

static void
ctk_flow_box_child_accessible_initialize (AtkObject *obj,
                                          gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_flow_box_child_accessible_parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_TABLE_CELL;
}

static AtkStateSet *
ctk_flow_box_child_accessible_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set;
  CtkWidget *widget;

  state_set = ATK_OBJECT_CLASS (ctk_flow_box_child_accessible_parent_class)->ref_state_set (obj);

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget != NULL)
    {
      CtkWidget *parent;

      parent = ctk_widget_get_parent (widget);
      if (ctk_flow_box_get_selection_mode (CTK_FLOW_BOX (parent)) != CTK_SELECTION_NONE)
        atk_state_set_add_state (state_set, ATK_STATE_SELECTABLE);

      if (ctk_flow_box_child_is_selected (CTK_FLOW_BOX_CHILD (widget)))
        atk_state_set_add_state (state_set, ATK_STATE_SELECTED);
    }

  return state_set;
}

static void
ctk_flow_box_child_accessible_class_init (CtkFlowBoxChildAccessibleClass *klass)
{
  AtkObjectClass *object_class = ATK_OBJECT_CLASS (klass);

  object_class->initialize = ctk_flow_box_child_accessible_initialize;
  object_class->ref_state_set = ctk_flow_box_child_accessible_ref_state_set;
}
