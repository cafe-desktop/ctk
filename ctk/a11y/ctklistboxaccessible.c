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

#include "ctklistboxaccessibleprivate.h"

#include "ctk/ctklistbox.h"

static void atk_selection_interface_init (AtkSelectionIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkListBoxAccessible, ctk_list_box_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_SELECTION, atk_selection_interface_init))

static void
ctk_list_box_accessible_init (CtkListBoxAccessible *accessible G_GNUC_UNUSED)
{
}

static void
ctk_list_box_accessible_initialize (AtkObject *obj,
                                    gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_list_box_accessible_parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_LIST_BOX;
}

static AtkStateSet*
ctk_list_box_accessible_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set;
  CtkWidget *widget;

  state_set = ATK_OBJECT_CLASS (ctk_list_box_accessible_parent_class)->ref_state_set (obj);
  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));

  if (widget != NULL)
    atk_state_set_add_state (state_set, ATK_STATE_MANAGES_DESCENDANTS);

  return state_set;
}

static void
ctk_list_box_accessible_class_init (CtkListBoxAccessibleClass *klass)
{
  AtkObjectClass *object_class = ATK_OBJECT_CLASS (klass);

  object_class->initialize = ctk_list_box_accessible_initialize;
  object_class->ref_state_set = ctk_list_box_accessible_ref_state_set;
}

static gboolean
ctk_list_box_accessible_add_selection (AtkSelection *selection,
                                       gint          idx)
{
  CtkWidget *box;
  CtkListBoxRow *row;

  box = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (box == NULL)
    return FALSE;

  row = ctk_list_box_get_row_at_index (CTK_LIST_BOX (box), idx);
  if (row)
    {
      ctk_list_box_select_row (CTK_LIST_BOX (box), row);
      return TRUE;
    }
  return FALSE;
}

static gboolean
ctk_list_box_accessible_remove_selection (AtkSelection *selection,
                                          gint          idx)
{
  CtkWidget *box;
  CtkListBoxRow *row;

  box = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (box == NULL)
    return FALSE;

  row = ctk_list_box_get_row_at_index (CTK_LIST_BOX (box), idx);
  if (row)
    {
      ctk_list_box_unselect_row (CTK_LIST_BOX (box), row);
      return TRUE;
    }
  return FALSE;
}

static gboolean
ctk_list_box_accessible_clear_selection (AtkSelection *selection)
{
  CtkWidget *box;

  box = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (box == NULL)
    return FALSE;

  ctk_list_box_unselect_all (CTK_LIST_BOX (box));
  return TRUE;
}

static gboolean
ctk_list_box_accessible_select_all (AtkSelection *selection)
{
  CtkWidget *box;

  box = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (box == NULL)
    return FALSE;

  ctk_list_box_select_all (CTK_LIST_BOX (box));
  return TRUE;
}

typedef struct
{
  gint idx;
  CtkWidget *row;
} FindSelectedData;

static void
find_selected_row (CtkListBox    *box G_GNUC_UNUSED,
                   CtkListBoxRow *row,
                   gpointer       data)
{
  FindSelectedData *d = data;

  if (d->idx == 0)
    {
      if (d->row == NULL)
        d->row = CTK_WIDGET (row);
    }
  else
    d->idx -= 1;
}

static AtkObject *
ctk_list_box_accessible_ref_selection (AtkSelection *selection,
                                       gint          idx)
{
  CtkWidget *box;
  AtkObject *accessible;
  FindSelectedData data;

  box = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (box == NULL)
    return NULL;

  data.idx = idx;
  data.row = NULL;
  ctk_list_box_selected_foreach (CTK_LIST_BOX (box), find_selected_row, &data);

  if (data.row == NULL)
    return NULL;

  accessible = ctk_widget_get_accessible (data.row);
  g_object_ref (accessible);
  return accessible;
}

static void
count_selected (CtkListBox    *box G_GNUC_UNUSED,
                CtkListBoxRow *row G_GNUC_UNUSED,
                gpointer       data)
{
  gint *count = data;
  *count += 1;
}

static gint
ctk_list_box_accessible_get_selection_count (AtkSelection *selection)
{
  CtkWidget *box;
  gint count;

  box = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (box == NULL)
    return 0;

  count = 0;
  ctk_list_box_selected_foreach (CTK_LIST_BOX (box), count_selected, &count);

  return count;
}

static gboolean
ctk_list_box_accessible_is_child_selected (AtkSelection *selection,
                                           gint          idx)
{
  CtkWidget *box;
  CtkListBoxRow *row;

  box = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (box == NULL)
    return FALSE;

  row = ctk_list_box_get_row_at_index (CTK_LIST_BOX (box), idx);

  return ctk_list_box_row_is_selected (row);
}

static void atk_selection_interface_init (AtkSelectionIface *iface)
{
  iface->add_selection = ctk_list_box_accessible_add_selection;
  iface->remove_selection = ctk_list_box_accessible_remove_selection;
  iface->clear_selection = ctk_list_box_accessible_clear_selection;
  iface->ref_selection = ctk_list_box_accessible_ref_selection;
  iface->get_selection_count = ctk_list_box_accessible_get_selection_count;
  iface->is_child_selected = ctk_list_box_accessible_is_child_selected;
  iface->select_all_selection = ctk_list_box_accessible_select_all;
}

void
_ctk_list_box_accessible_selection_changed (CtkListBox *box)
{
  AtkObject *accessible;
  accessible = ctk_widget_get_accessible (CTK_WIDGET (box));
  g_signal_emit_by_name (accessible, "selection-changed");
}

void
_ctk_list_box_accessible_update_cursor (CtkListBox *box,
                                        CtkListBoxRow *row)
{
  AtkObject *accessible;
  AtkObject *descendant;
  accessible = ctk_widget_get_accessible (CTK_WIDGET (box));
  descendant = row ? ctk_widget_get_accessible (CTK_WIDGET (row)) : NULL;
  g_signal_emit_by_name (accessible, "active-descendant-changed", descendant);
}
