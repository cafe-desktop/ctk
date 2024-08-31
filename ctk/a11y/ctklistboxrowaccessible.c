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

#include "ctklistboxrowaccessible.h"

#include "ctk/ctklistbox.h"


G_DEFINE_TYPE (CtkListBoxRowAccessible, ctk_list_box_row_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE)

static void
ctk_list_box_row_accessible_init (CtkListBoxRowAccessible *accessible G_GNUC_UNUSED)
{
}

static void
ctk_list_box_row_accessible_initialize (AtkObject *obj,
                                        gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_list_box_row_accessible_parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_LIST_ITEM;
}

static AtkStateSet*
ctk_list_box_row_accessible_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set;
  CtkWidget *widget;

  state_set = ATK_OBJECT_CLASS (ctk_list_box_row_accessible_parent_class)->ref_state_set (obj);

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget != NULL)
    {
      CtkWidget *parent;

      parent = ctk_widget_get_parent (widget);
      if (parent != NULL && 
          CTK_IS_LIST_BOX (parent) &&
          ctk_list_box_get_selection_mode (CTK_LIST_BOX (parent)) != CTK_SELECTION_NONE)
        atk_state_set_add_state (state_set, ATK_STATE_SELECTABLE);

      if (ctk_list_box_row_is_selected (CTK_LIST_BOX_ROW (widget)))
        atk_state_set_add_state (state_set, ATK_STATE_SELECTED);
    }

  return state_set;
}

static void
ctk_list_box_row_accessible_class_init (CtkListBoxRowAccessibleClass *klass)
{
  AtkObjectClass *object_class = ATK_OBJECT_CLASS (klass);

  object_class->initialize = ctk_list_box_row_accessible_initialize;
  object_class->ref_state_set = ctk_list_box_row_accessible_ref_state_set;
}
