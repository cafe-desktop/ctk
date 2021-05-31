/* GTK+ - accessibility implementations
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

#include <glib/gi18n-lib.h>
#include <ctk/ctk.h>
#include "ctkcontainercellaccessible.h"
#include "ctkcellaccessibleprivate.h"
#include "ctkcellaccessibleparent.h"

struct _CtkCellAccessiblePrivate
{
  AtkObject *parent;
};

static const struct {
  AtkState atk_state;
  CtkCellRendererState renderer_state;
  gboolean invert;
} state_map[] = {
  { ATK_STATE_SENSITIVE, CTK_CELL_RENDERER_INSENSITIVE, TRUE },
  { ATK_STATE_ENABLED,   CTK_CELL_RENDERER_INSENSITIVE, TRUE },
  { ATK_STATE_SELECTED,  CTK_CELL_RENDERER_SELECTED,    FALSE },
  /* XXX: why do we map ACTIVE here? */
  { ATK_STATE_ACTIVE,    CTK_CELL_RENDERER_FOCUSED,     FALSE },
  { ATK_STATE_FOCUSED,   CTK_CELL_RENDERER_FOCUSED,     FALSE },
  { ATK_STATE_EXPANDABLE,CTK_CELL_RENDERER_EXPANDABLE,  FALSE },
  { ATK_STATE_EXPANDED,  CTK_CELL_RENDERER_EXPANDED,    FALSE },
};

static CtkCellRendererState ctk_cell_accessible_get_state (CtkCellAccessible *cell);
static void atk_action_interface_init    (AtkActionIface    *iface);
static void atk_component_interface_init (AtkComponentIface *iface);
static void atk_table_cell_interface_init    (AtkTableCellIface    *iface);

G_DEFINE_TYPE_WITH_CODE (CtkCellAccessible, ctk_cell_accessible, CTK_TYPE_ACCESSIBLE,
                         G_ADD_PRIVATE (CtkCellAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, atk_action_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT, atk_component_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_TABLE_CELL, atk_table_cell_interface_init))

static gint
ctk_cell_accessible_get_index_in_parent (AtkObject *obj)
{
  CtkCellAccessible *cell;
  AtkObject *parent;

  cell = CTK_CELL_ACCESSIBLE (obj);

  if (CTK_IS_CONTAINER_CELL_ACCESSIBLE (cell->priv->parent))
    return g_list_index (ctk_container_cell_accessible_get_children (CTK_CONTAINER_CELL_ACCESSIBLE (cell->priv->parent)), obj);

  parent = ctk_widget_get_accessible (ctk_accessible_get_widget (CTK_ACCESSIBLE (cell)));
  if (parent == NULL)
    return -1;

  return ctk_cell_accessible_parent_get_child_index (CTK_CELL_ACCESSIBLE_PARENT (cell->priv->parent), cell);
}

static AtkRelationSet *
ctk_cell_accessible_ref_relation_set (AtkObject *object)
{
  CtkCellAccessible *cell;
  AtkRelationSet *relationset;
  AtkObject *parent;

  relationset = ATK_OBJECT_CLASS (ctk_cell_accessible_parent_class)->ref_relation_set (object);
  if (relationset == NULL)
    relationset = atk_relation_set_new ();

  cell = CTK_CELL_ACCESSIBLE (object);
  parent = ctk_widget_get_accessible (ctk_accessible_get_widget (CTK_ACCESSIBLE (cell)));

  ctk_cell_accessible_parent_update_relationset (CTK_CELL_ACCESSIBLE_PARENT (parent),
                                                 cell,
                                                 relationset);

  return relationset;
}

static AtkStateSet *
ctk_cell_accessible_ref_state_set (AtkObject *accessible)
{
  CtkCellAccessible *cell_accessible;
  AtkStateSet *state_set;
  CtkCellRendererState flags;
  guint i;

  cell_accessible = CTK_CELL_ACCESSIBLE (accessible);

  state_set = atk_state_set_new ();

  if (ctk_accessible_get_widget (CTK_ACCESSIBLE (cell_accessible)) == NULL)
    {
      atk_state_set_add_state (state_set, ATK_STATE_DEFUNCT);
      return state_set;
    }

  flags = ctk_cell_accessible_get_state (cell_accessible);

  atk_state_set_add_state (state_set, ATK_STATE_FOCUSABLE);
  atk_state_set_add_state (state_set, ATK_STATE_SELECTABLE);
  atk_state_set_add_state (state_set, ATK_STATE_TRANSIENT);
  atk_state_set_add_state (state_set, ATK_STATE_VISIBLE);

  for (i = 0; i < G_N_ELEMENTS (state_map); i++)
    {
      if (flags & state_map[i].renderer_state)
        {
          if (!state_map[i].invert)
            atk_state_set_add_state (state_set, state_map[i].atk_state);
        }
      else
        {
          if (state_map[i].invert)
            atk_state_set_add_state (state_set, state_map[i].atk_state);
        }
    }

  if (ctk_widget_get_mapped (ctk_accessible_get_widget (CTK_ACCESSIBLE (cell_accessible))))
    atk_state_set_add_state (state_set, ATK_STATE_SHOWING);

  return state_set;
}

static AtkObject *
ctk_cell_accessible_get_parent (AtkObject *object)
{
  CtkCellAccessible *cell = CTK_CELL_ACCESSIBLE (object);

  return cell->priv->parent;
}

static void
ctk_cell_accessible_class_init (CtkCellAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  class->get_index_in_parent = ctk_cell_accessible_get_index_in_parent;
  class->ref_state_set = ctk_cell_accessible_ref_state_set;
  class->ref_relation_set = ctk_cell_accessible_ref_relation_set;
  class->get_parent = ctk_cell_accessible_get_parent;
}

static void
ctk_cell_accessible_init (CtkCellAccessible *cell)
{
  cell->priv = ctk_cell_accessible_get_instance_private (cell);
}

void
_ctk_cell_accessible_initialize (CtkCellAccessible *cell,
                                 CtkWidget         *widget,
                                 AtkObject         *parent)
{
  ctk_accessible_set_widget (CTK_ACCESSIBLE (cell), widget);
  cell->priv->parent = parent;
}

gboolean
_ctk_cell_accessible_add_state (CtkCellAccessible *cell,
                                AtkStateType       state_type,
                                gboolean           emit_signal)
{
  /* The signal should only be generated if the value changed,
   * not when the cell is set up. So states that are set
   * initially should pass FALSE as the emit_signal argument.
   */
  if (emit_signal)
    {
      atk_object_notify_state_change (ATK_OBJECT (cell), state_type, TRUE);
      /* If state_type is ATK_STATE_VISIBLE, additional notification */
      if (state_type == ATK_STATE_VISIBLE)
        g_signal_emit_by_name (cell, "visible-data-changed");
    }

  /* If the parent is a flyweight container cell, propagate the state
   * change to it also
   */
  if (CTK_IS_CONTAINER_CELL_ACCESSIBLE (cell->priv->parent))
    _ctk_cell_accessible_add_state (CTK_CELL_ACCESSIBLE (cell->priv->parent), state_type, emit_signal);

  return TRUE;
}

gboolean
_ctk_cell_accessible_remove_state (CtkCellAccessible *cell,
                                   AtkStateType       state_type,
                                   gboolean           emit_signal)
{
  /* The signal should only be generated if the value changed,
   * not when the cell is set up.  So states that are set
   * initially should pass FALSE as the emit_signal argument.
   */
  if (emit_signal)
    {
      atk_object_notify_state_change (ATK_OBJECT (cell), state_type, FALSE);
      /* If state_type is ATK_STATE_VISIBLE, additional notification */
      if (state_type == ATK_STATE_VISIBLE)
        g_signal_emit_by_name (cell, "visible-data-changed");
    }

  /* If the parent is a flyweight container cell, propagate the state
   * change to it also
   */
  if (CTK_IS_CONTAINER_CELL_ACCESSIBLE (cell->priv->parent))
    _ctk_cell_accessible_remove_state (CTK_CELL_ACCESSIBLE (cell->priv->parent), state_type, emit_signal);

  return TRUE;
}

static gint
ctk_cell_accessible_action_get_n_actions (AtkAction *action)
{
  return 3;
}

static const gchar *
ctk_cell_accessible_action_get_name (AtkAction *action,
                                     gint       index)
{
  switch (index)
    {
    case 0:
      return "expand or contract";
    case 1:
      return "edit";
    case 2:
      return "activate";
    default:
      return NULL;
    }
}

static const gchar *
ctk_cell_accessible_action_get_localized_name (AtkAction *action,
                                               gint       index)
{
  switch (index)
    {
    case 0:
      return C_("Action name", "Expand or contract");
    case 1:
      return C_("Action name", "Edit");
    case 2:
      return C_("Action name", "Activate");
    default:
      return NULL;
    }
}

static const gchar *
ctk_cell_accessible_action_get_description (AtkAction *action,
                                            gint       index)
{
  switch (index)
    {
    case 0:
      return C_("Action description", "Expands or contracts the row in the tree view containing this cell");
    case 1:
      return C_("Action description", "Creates a widget in which the contents of the cell can be edited");
    case 2:
      return C_("Action description", "Activates the cell");
    default:
      return NULL;
    }
}

static const gchar *
ctk_cell_accessible_action_get_keybinding (AtkAction *action,
                                           gint       index)
{
  return NULL;
}

static gboolean
ctk_cell_accessible_action_do_action (AtkAction *action,
                                      gint       index)
{
  CtkCellAccessible *cell = CTK_CELL_ACCESSIBLE (action);
  CtkCellAccessibleParent *parent;

  cell = CTK_CELL_ACCESSIBLE (action);
  if (ctk_accessible_get_widget (CTK_ACCESSIBLE (cell)) == NULL)
    return FALSE;

  parent = CTK_CELL_ACCESSIBLE_PARENT (ctk_widget_get_accessible (ctk_accessible_get_widget (CTK_ACCESSIBLE (cell))));

  switch (index)
    {
    case 0:
      ctk_cell_accessible_parent_expand_collapse (parent, cell);
      break;
    case 1:
      ctk_cell_accessible_parent_edit (parent, cell);
      break;
    case 2:
      ctk_cell_accessible_parent_activate (parent, cell);
      break;
    default:
      return FALSE;
    }

  return TRUE;
}

static void
atk_action_interface_init (AtkActionIface *iface)
{
  iface->get_n_actions = ctk_cell_accessible_action_get_n_actions;
  iface->do_action = ctk_cell_accessible_action_do_action;
  iface->get_name = ctk_cell_accessible_action_get_name;
  iface->get_localized_name = ctk_cell_accessible_action_get_localized_name;
  iface->get_description = ctk_cell_accessible_action_get_description;
  iface->get_keybinding = ctk_cell_accessible_action_get_keybinding;
}

static void
ctk_cell_accessible_get_extents (AtkComponent *component,
                                 gint         *x,
                                 gint         *y,
                                 gint         *width,
                                 gint         *height,
                                 AtkCoordType  coord_type)
{
  CtkCellAccessible *cell;
  AtkObject *parent;

  cell = CTK_CELL_ACCESSIBLE (component);
  parent = ctk_widget_get_accessible (ctk_accessible_get_widget (CTK_ACCESSIBLE (cell)));

  ctk_cell_accessible_parent_get_cell_extents (CTK_CELL_ACCESSIBLE_PARENT (parent),
                                                cell,
                                                x, y, width, height, coord_type);
}

static gboolean
ctk_cell_accessible_grab_focus (AtkComponent *component)
{
  CtkCellAccessible *cell;
  AtkObject *parent;

  cell = CTK_CELL_ACCESSIBLE (component);
  parent = ctk_widget_get_accessible (ctk_accessible_get_widget (CTK_ACCESSIBLE (cell)));

  return ctk_cell_accessible_parent_grab_focus (CTK_CELL_ACCESSIBLE_PARENT (parent), cell);
}

static void
atk_component_interface_init (AtkComponentIface *iface)
{
  iface->get_extents = ctk_cell_accessible_get_extents;
  iface->grab_focus = ctk_cell_accessible_grab_focus;
}

static int
ctk_cell_accessible_get_column_span (AtkTableCell *table_cell)
{
  return 1;
}

static GPtrArray *
ctk_cell_accessible_get_column_header_cells (AtkTableCell *table_cell)
{
  CtkCellAccessible *cell;
  AtkObject *parent;

  cell = CTK_CELL_ACCESSIBLE (table_cell);
  parent = ctk_widget_get_accessible (ctk_accessible_get_widget (CTK_ACCESSIBLE (cell)));

  return ctk_cell_accessible_parent_get_column_header_cells (CTK_CELL_ACCESSIBLE_PARENT (parent),
                                                             cell);
}

static gboolean
ctk_cell_accessible_get_position (AtkTableCell *table_cell,
                                  gint         *row,
                                  gint         *column)
{
  CtkCellAccessible *cell;
  AtkObject *parent;

  cell = CTK_CELL_ACCESSIBLE (table_cell);
  parent = ctk_widget_get_accessible (ctk_accessible_get_widget (CTK_ACCESSIBLE (cell)));

  ctk_cell_accessible_parent_get_cell_position (CTK_CELL_ACCESSIBLE_PARENT (parent),
                                                cell,
                                                row, column);
  return ((row && *row > 0) || (column && *column > 0));
}

static int
ctk_cell_accessible_get_row_span (AtkTableCell *table_cell)
{
  return 1;
}

static GPtrArray *
ctk_cell_accessible_get_row_header_cells (AtkTableCell *table_cell)
{
  CtkCellAccessible *cell;
  AtkObject *parent;

  cell = CTK_CELL_ACCESSIBLE (table_cell);
  parent = ctk_widget_get_accessible (ctk_accessible_get_widget (CTK_ACCESSIBLE (cell)));

  return ctk_cell_accessible_parent_get_row_header_cells (CTK_CELL_ACCESSIBLE_PARENT (parent),
                                                          cell);
}

static AtkObject *
ctk_cell_accessible_get_table (AtkTableCell *table_cell)
{
  AtkObject *obj;

  obj = ATK_OBJECT (table_cell);
  do
    {
      AtkRole role;
      obj = atk_object_get_parent (obj);
      role = atk_object_get_role (obj);
      if (role == ATK_ROLE_TABLE || role == ATK_ROLE_TREE_TABLE)
        break;
    }
  while (obj);
  return obj;
}

static void
atk_table_cell_interface_init (AtkTableCellIface *iface)
{
  iface->get_column_span = ctk_cell_accessible_get_column_span;
  iface->get_column_header_cells = ctk_cell_accessible_get_column_header_cells;
  iface->get_position = ctk_cell_accessible_get_position;
  iface->get_row_span = ctk_cell_accessible_get_row_span;
  iface->get_row_header_cells = ctk_cell_accessible_get_row_header_cells;
  iface->get_table = ctk_cell_accessible_get_table;
}

static CtkCellRendererState
ctk_cell_accessible_get_state (CtkCellAccessible *cell)
{
  AtkObject *parent;

  g_return_val_if_fail (CTK_IS_CELL_ACCESSIBLE (cell), 0);

  parent = ctk_widget_get_accessible (ctk_accessible_get_widget (CTK_ACCESSIBLE (cell)));
  if (parent == NULL)
    return 0;

  return ctk_cell_accessible_parent_get_renderer_state (CTK_CELL_ACCESSIBLE_PARENT (parent), cell);
}

/*
 * ctk_cell_accessible_state_changed:
 * @cell: a #CtkCellAccessible
 * @added: the flags that were added from @cell
 * @removed: the flags that were removed from @cell
 *
 * Notifies @cell of state changes. Multiple states may be added
 * or removed at the same time. A state that is @added may not be
 * @removed at the same time.
 **/
void
_ctk_cell_accessible_state_changed (CtkCellAccessible    *cell,
                                    CtkCellRendererState  added,
                                    CtkCellRendererState  removed)
{
  AtkObject *object;
  guint i;

  g_return_if_fail (CTK_IS_CELL_ACCESSIBLE (cell));
  g_return_if_fail ((added & removed) == 0);

  object = ATK_OBJECT (cell);

  for (i = 0; i < G_N_ELEMENTS (state_map); i++)
    {
      if (added & state_map[i].renderer_state)
        atk_object_notify_state_change (object,
                                        state_map[i].atk_state,
                                        !state_map[i].invert);
      if (removed & state_map[i].renderer_state)
        atk_object_notify_state_change (object,
                                        state_map[i].atk_state,
                                        state_map[i].invert);
    }
}

/*
 * ctk_cell_accessible_update_cache:
 * @cell: the cell that is changed
 * @emit_signal: whether or not to notify the ATK bridge
 *
 * Notifies the cell that the values in the data in the row that
 * is used to feed the cell renderer with has changed. The
 * cell_changed function of @cell is called to send update
 * notifications for the properties it takes from its cell
 * renderer. If @emit_signal is TRUE, also notify the ATK bridge
 * of the change. The bridge should be notified when an existing
 * cell changes; not when a newly-created cell is being set up.
 *
 * Note that there is no higher granularity available about which
 * properties changed, so you will need to make do with this
 * function.
 **/
void
_ctk_cell_accessible_update_cache (CtkCellAccessible *cell,
                                   gboolean           emit_signal)
{
  CtkCellAccessibleClass *klass;

  g_return_if_fail (CTK_CELL_ACCESSIBLE (cell));

  klass = CTK_CELL_ACCESSIBLE_GET_CLASS (cell);

  if (klass->update_cache)
    klass->update_cache (cell, emit_signal);
}
