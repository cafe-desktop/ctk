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
#include "ctkmenushellaccessible.h"


static void atk_selection_interface_init (AtkSelectionIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkMenuShellAccessible, ctk_menu_shell_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_SELECTION, atk_selection_interface_init))

static void
ctk_menu_shell_accessible_initialize (AtkObject *accessible,
                                      gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_menu_shell_accessible_parent_class)->initialize (accessible, data);

  accessible->role = ATK_ROLE_UNKNOWN;
}

static void
ctk_menu_shell_accessible_class_init (CtkMenuShellAccessibleClass *klass)
{
  AtkObjectClass *atk_object_class = ATK_OBJECT_CLASS (klass);

  atk_object_class->initialize = ctk_menu_shell_accessible_initialize;
}

static void
ctk_menu_shell_accessible_init (CtkMenuShellAccessible *menu_shell G_GNUC_UNUSED)
{
}

static gboolean
ctk_menu_shell_accessible_add_selection (AtkSelection *selection,
                                         gint          i)
{
  GList *kids;
  CtkWidget *item;
  guint length;
  CtkWidget *widget;

  widget =  ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  kids = ctk_container_get_children (CTK_CONTAINER (widget));
  length = g_list_length (kids);
  if (i < 0 || i > length)
    {
      g_list_free (kids);
      return FALSE;
    }

  item = g_list_nth_data (kids, i);
  g_list_free (kids);
  g_return_val_if_fail (CTK_IS_MENU_ITEM (item), FALSE);
  ctk_menu_shell_select_item (CTK_MENU_SHELL (widget), item);
  return TRUE;
}

static gboolean
ctk_menu_shell_accessible_clear_selection (AtkSelection *selection)
{
  CtkMenuShell *shell;
  CtkWidget *widget;

  widget =  ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  shell = CTK_MENU_SHELL (widget);

  ctk_menu_shell_deselect (shell);
  return TRUE;
}

static AtkObject *
ctk_menu_shell_accessible_ref_selection (AtkSelection *selection,
                                         gint          i)
{
  CtkMenuShell *shell;
  CtkWidget *widget;
  CtkWidget *item;

  widget =  ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return NULL;

  if (i != 0)
    return NULL;

  shell = CTK_MENU_SHELL (widget);

  item = ctk_menu_shell_get_selected_item (shell);
  if (item != NULL)
    {
      AtkObject *obj;

      obj = ctk_widget_get_accessible (item);
      g_object_ref (obj);
      return obj;
    }
  return NULL;
}

static gint
ctk_menu_shell_accessible_get_selection_count (AtkSelection *selection)
{
  CtkMenuShell *shell;
  CtkWidget *widget;

  widget =  ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return 0;

  shell = CTK_MENU_SHELL (widget);

  if (ctk_menu_shell_get_selected_item (shell) != NULL)
    return 1;

  return 0;
}

static gboolean
ctk_menu_shell_accessible_is_child_selected (AtkSelection *selection,
                                             gint          i)
{
  CtkMenuShell *shell;
  GList *kids;
  gint j;
  CtkWidget *widget;
  CtkWidget *item;

  widget =  ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  shell = CTK_MENU_SHELL (widget);
  item = ctk_menu_shell_get_selected_item (shell);
  if (item == NULL)
    return FALSE;

  kids = ctk_container_get_children (CTK_CONTAINER (shell));
  j = g_list_index (kids, item);
  g_list_free (kids);

  return j==i;
}

static gboolean
ctk_menu_shell_accessible_remove_selection (AtkSelection *selection,
                                            gint          i)
{
  CtkMenuShell *shell;
  CtkWidget *widget;
  CtkWidget *item;

  widget =  ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  if (i != 0)
    return FALSE;

  shell = CTK_MENU_SHELL (widget);

  item = ctk_menu_shell_get_selected_item (shell);
  if (item && ctk_menu_item_get_submenu (CTK_MENU_ITEM (item)))
    ctk_menu_shell_deselect (shell);
  return TRUE;
}

static void
atk_selection_interface_init (AtkSelectionIface *iface)
{
  iface->add_selection = ctk_menu_shell_accessible_add_selection;
  iface->clear_selection = ctk_menu_shell_accessible_clear_selection;
  iface->ref_selection = ctk_menu_shell_accessible_ref_selection;
  iface->get_selection_count = ctk_menu_shell_accessible_get_selection_count;
  iface->is_child_selected = ctk_menu_shell_accessible_is_child_selected;
  iface->remove_selection = ctk_menu_shell_accessible_remove_selection;
}
