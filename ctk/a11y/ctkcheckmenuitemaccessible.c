/* CTK+ - accessibility implementations
 * Copyright 2002 Sun Microsystems Inc.
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

#include <string.h>
#include <ctk/ctk.h>
#include "ctkcheckmenuitemaccessible.h"


G_DEFINE_TYPE (CtkCheckMenuItemAccessible, ctk_check_menu_item_accessible, CTK_TYPE_MENU_ITEM_ACCESSIBLE)

static void
toggled_cb (CtkWidget *widget)
{
  AtkObject *accessible;
  CtkCheckMenuItem *check_menu_item;
  gboolean active;

  check_menu_item = CTK_CHECK_MENU_ITEM (widget);
  active = ctk_check_menu_item_get_active (check_menu_item);

  accessible = ctk_widget_get_accessible (widget);
  atk_object_notify_state_change (accessible, ATK_STATE_CHECKED, active);
}

static void
ctk_check_menu_item_accessible_initialize (AtkObject *obj,
                                              gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_check_menu_item_accessible_parent_class)->initialize (obj, data);

  g_signal_connect (data, "toggled", G_CALLBACK (toggled_cb), NULL);

  obj->role = ATK_ROLE_CHECK_MENU_ITEM;
}

static AtkStateSet *
ctk_check_menu_item_accessible_ref_state_set (AtkObject *accessible)
{
  AtkStateSet *state_set;
  CtkCheckMenuItem *check_menu_item;
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return NULL;

  state_set = ATK_OBJECT_CLASS (ctk_check_menu_item_accessible_parent_class)->ref_state_set (accessible);

  check_menu_item = CTK_CHECK_MENU_ITEM (widget);

  if (ctk_check_menu_item_get_active (check_menu_item))
    atk_state_set_add_state (state_set, ATK_STATE_CHECKED);

  if (ctk_check_menu_item_get_inconsistent (check_menu_item))
    {
      atk_state_set_remove_state (state_set, ATK_STATE_ENABLED);
      atk_state_set_add_state (state_set, ATK_STATE_INDETERMINATE);
    }

  return state_set;
}

static void
ctk_check_menu_item_accessible_notify_ctk (GObject    *obj,
                                           GParamSpec *pspec)
{
  CtkCheckMenuItem *check_menu_item = CTK_CHECK_MENU_ITEM (obj);
  AtkObject *atk_obj;
  gboolean sensitive;
  gboolean inconsistent;
  gboolean active;

  atk_obj = ctk_widget_get_accessible (CTK_WIDGET (check_menu_item));
  sensitive = ctk_widget_get_sensitive (CTK_WIDGET (check_menu_item));
  inconsistent = ctk_check_menu_item_get_inconsistent (check_menu_item);
  active = ctk_check_menu_item_get_active (check_menu_item);

  if (strcmp (pspec->name, "inconsistent") == 0)
    {
      atk_object_notify_state_change (atk_obj, ATK_STATE_INDETERMINATE, inconsistent);
      atk_object_notify_state_change (atk_obj, ATK_STATE_ENABLED, (sensitive && !inconsistent));
    }
  else if (strcmp (pspec->name, "sensitive") == 0)
    {
      /* Need to override cailwidget behavior of notifying for ENABLED */
      atk_object_notify_state_change (atk_obj, ATK_STATE_SENSITIVE, sensitive);
      atk_object_notify_state_change (atk_obj, ATK_STATE_ENABLED, (sensitive && !inconsistent));
    }
  else if (strcmp (pspec->name, "active") == 0)
    {
      atk_object_notify_state_change (atk_obj, ATK_STATE_CHECKED, active);
    }
  else
    CTK_WIDGET_ACCESSIBLE_CLASS (ctk_check_menu_item_accessible_parent_class)->notify_ctk (obj, pspec);
}

static void
ctk_check_menu_item_accessible_class_init (CtkCheckMenuItemAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);
  CtkWidgetAccessibleClass *widget_class = (CtkWidgetAccessibleClass*)klass;

  widget_class->notify_ctk = ctk_check_menu_item_accessible_notify_ctk;

  class->ref_state_set = ctk_check_menu_item_accessible_ref_state_set;
  class->initialize = ctk_check_menu_item_accessible_initialize;
}

static void
ctk_check_menu_item_accessible_init (CtkCheckMenuItemAccessible *item G_GNUC_UNUSED)
{
}
