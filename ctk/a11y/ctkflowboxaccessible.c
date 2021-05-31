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

#include "ctkflowboxaccessibleprivate.h"

#include "ctk/ctkflowbox.h"

static void atk_selection_interface_init (AtkSelectionIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkFlowBoxAccessible, ctk_flow_box_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_SELECTION, atk_selection_interface_init))

static void
ctk_flow_box_accessible_init (CtkFlowBoxAccessible *accessible)
{
}

static void
ctk_flow_box_accessible_initialize (AtkObject *obj,
                                    gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_flow_box_accessible_parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_TABLE;
}

static AtkStateSet*
ctk_flow_box_accessible_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set;
  CtkWidget *widget;

  state_set = ATK_OBJECT_CLASS (ctk_flow_box_accessible_parent_class)->ref_state_set (obj);
  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));

  if (widget != NULL)
    atk_state_set_add_state (state_set, ATK_STATE_MANAGES_DESCENDANTS);

  return state_set;
}

static void
ctk_flow_box_accessible_class_init (CtkFlowBoxAccessibleClass *klass)
{
  AtkObjectClass *object_class = ATK_OBJECT_CLASS (klass);

  object_class->initialize = ctk_flow_box_accessible_initialize;
  object_class->ref_state_set = ctk_flow_box_accessible_ref_state_set;
}

static gboolean
ctk_flow_box_accessible_add_selection (AtkSelection *selection,
                                       gint          idx)
{
  CtkWidget *box;
  GList *children;
  CtkWidget *child;

  box = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (box == NULL)
    return FALSE;

  children = ctk_container_get_children (CTK_CONTAINER (box));
  child = g_list_nth_data (children, idx);
  g_list_free (children);
  if (child)
    {
      ctk_flow_box_select_child (CTK_FLOW_BOX (box), CTK_FLOW_BOX_CHILD (child));
      return TRUE;
    }
  return FALSE;
}

static gboolean
ctk_flow_box_accessible_remove_selection (AtkSelection *selection,
                                          gint          idx)
{
  CtkWidget *box;
  GList *children;
  CtkWidget *child;

  box = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (box == NULL)
    return FALSE;

  children = ctk_container_get_children (CTK_CONTAINER (box));
  child = g_list_nth_data (children, idx);
  g_list_free (children);
  if (child)
    {
      ctk_flow_box_unselect_child (CTK_FLOW_BOX (box), CTK_FLOW_BOX_CHILD (child));
      return TRUE;
    }
  return FALSE;
}

static gboolean
ctk_flow_box_accessible_clear_selection (AtkSelection *selection)
{
  CtkWidget *box;

  box = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (box == NULL)
    return FALSE;

  ctk_flow_box_unselect_all (CTK_FLOW_BOX (box));
  return TRUE;
}

static gboolean
ctk_flow_box_accessible_select_all (AtkSelection *selection)
{
  CtkWidget *box;

  box = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (box == NULL)
    return FALSE;

  ctk_flow_box_select_all (CTK_FLOW_BOX (box));
  return TRUE;
}

typedef struct
{
  gint idx;
  CtkWidget *child;
} FindSelectedData;

static void
find_selected_child (CtkFlowBox      *box,
                     CtkFlowBoxChild *child,
                     gpointer         data)
{
  FindSelectedData *d = data;

  if (d->idx == 0)
    {
      if (d->child == NULL)
        d->child = CTK_WIDGET (child);
    }
  else
    d->idx -= 1;
}

static AtkObject *
ctk_flow_box_accessible_ref_selection (AtkSelection *selection,
                                       gint          idx)
{
  CtkWidget *box;
  AtkObject *accessible;
  FindSelectedData data;

  box = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (box == NULL)
    return NULL;

  data.idx = idx;
  data.child = NULL;
  ctk_flow_box_selected_foreach (CTK_FLOW_BOX (box), find_selected_child, &data);

  if (data.child == NULL)
    return NULL;

  accessible = ctk_widget_get_accessible (data.child);
  g_object_ref (accessible);
  return accessible;
}

static void
count_selected (CtkFlowBox      *box,
                CtkFlowBoxChild *child,
                gpointer         data)
{
  gint *count = data;
  *count += 1;
}

static gint
ctk_flow_box_accessible_get_selection_count (AtkSelection *selection)
{
  CtkWidget *box;
  gint count;

  box = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (box == NULL)
    return 0;

  count = 0;
  ctk_flow_box_selected_foreach (CTK_FLOW_BOX (box), count_selected, &count);

  return count;
}

static gboolean
ctk_flow_box_accessible_is_child_selected (AtkSelection *selection,
                                           gint          idx)
{
  CtkWidget *box;
  CtkFlowBoxChild *child;

  box = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (box == NULL)
    return FALSE;

  child = ctk_flow_box_get_child_at_index (CTK_FLOW_BOX (box), idx);

  return ctk_flow_box_child_is_selected (child);
}

static void atk_selection_interface_init (AtkSelectionIface *iface)
{
  iface->add_selection = ctk_flow_box_accessible_add_selection;
  iface->remove_selection = ctk_flow_box_accessible_remove_selection;
  iface->clear_selection = ctk_flow_box_accessible_clear_selection;
  iface->ref_selection = ctk_flow_box_accessible_ref_selection;
  iface->get_selection_count = ctk_flow_box_accessible_get_selection_count;
  iface->is_child_selected = ctk_flow_box_accessible_is_child_selected;
  iface->select_all_selection = ctk_flow_box_accessible_select_all;
}

void
_ctk_flow_box_accessible_selection_changed (CtkWidget *box)
{
  AtkObject *accessible;
  accessible = ctk_widget_get_accessible (box);
  g_signal_emit_by_name (accessible, "selection-changed");
}

void
_ctk_flow_box_accessible_update_cursor (CtkWidget *box,
                                        CtkWidget *child)
{
  AtkObject *accessible;
  AtkObject *descendant;
  accessible = ctk_widget_get_accessible (box);
  descendant = child ? ctk_widget_get_accessible (child) : NULL;
  g_signal_emit_by_name (accessible, "active-descendant-changed", descendant);
}
