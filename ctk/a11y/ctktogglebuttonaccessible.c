/* CTK+ - accessibility implementations
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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
#include "ctktogglebuttonaccessible.h"


G_DEFINE_TYPE (CtkToggleButtonAccessible, ctk_toggle_button_accessible, CTK_TYPE_BUTTON_ACCESSIBLE)

static void
ctk_toggle_button_accessible_toggled (CtkWidget *widget)
{
  AtkObject *accessible;
  CtkToggleButton *toggle_button;

  toggle_button = CTK_TOGGLE_BUTTON (widget);

  accessible = ctk_widget_get_accessible (widget);
  atk_object_notify_state_change (accessible, ATK_STATE_CHECKED,
                                  ctk_toggle_button_get_active (toggle_button));
}

static void
ctk_toggle_button_accessible_initialize (AtkObject *obj,
                                         gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_toggle_button_accessible_parent_class)->initialize (obj, data);

  g_signal_connect (data, "toggled",
                    G_CALLBACK (ctk_toggle_button_accessible_toggled), NULL);

  obj->role = ATK_ROLE_TOGGLE_BUTTON;
}

static void
ctk_toggle_button_accessible_notify_ctk (GObject    *obj,
                                         GParamSpec *pspec)
{
  CtkToggleButton *toggle_button = CTK_TOGGLE_BUTTON (obj);
  AtkObject *atk_obj;
  gboolean sensitive;
  gboolean inconsistent;

  atk_obj = ctk_widget_get_accessible (CTK_WIDGET (toggle_button));
  sensitive = ctk_widget_get_sensitive (CTK_WIDGET (toggle_button));
  inconsistent = ctk_toggle_button_get_inconsistent (toggle_button);

  if (strcmp (pspec->name, "inconsistent") == 0)
    {
      atk_object_notify_state_change (atk_obj, ATK_STATE_INDETERMINATE, inconsistent);
      atk_object_notify_state_change (atk_obj, ATK_STATE_ENABLED, (sensitive && !inconsistent));
    }
  else if (strcmp (pspec->name, "sensitive") == 0)
    {
      /* Need to override gailwidget behavior of notifying for ENABLED */
      atk_object_notify_state_change (atk_obj, ATK_STATE_SENSITIVE, sensitive);
      atk_object_notify_state_change (atk_obj, ATK_STATE_ENABLED, (sensitive && !inconsistent));
    }
  else
    CTK_WIDGET_ACCESSIBLE_CLASS (ctk_toggle_button_accessible_parent_class)->notify_ctk (obj, pspec);
}

static AtkStateSet*
ctk_toggle_button_accessible_ref_state_set (AtkObject *accessible)
{
  AtkStateSet *state_set;
  CtkToggleButton *toggle_button;
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return NULL;

  state_set = ATK_OBJECT_CLASS (ctk_toggle_button_accessible_parent_class)->ref_state_set (accessible);
  toggle_button = CTK_TOGGLE_BUTTON (widget);

  if (ctk_toggle_button_get_active (toggle_button))
    atk_state_set_add_state (state_set, ATK_STATE_CHECKED);

  if (ctk_toggle_button_get_inconsistent (toggle_button))
    {
      atk_state_set_remove_state (state_set, ATK_STATE_ENABLED);
      atk_state_set_add_state (state_set, ATK_STATE_INDETERMINATE);
    }

  return state_set;
}

static void
ctk_toggle_button_accessible_class_init (CtkToggleButtonAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);
  CtkWidgetAccessibleClass *widget_class = (CtkWidgetAccessibleClass*)klass;

  widget_class->notify_ctk = ctk_toggle_button_accessible_notify_ctk;

  class->ref_state_set = ctk_toggle_button_accessible_ref_state_set;
  class->initialize = ctk_toggle_button_accessible_initialize;
}

static void
ctk_toggle_button_accessible_init (CtkToggleButtonAccessible *button)
{
}
