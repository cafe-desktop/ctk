/* GTK+ - accessibility implementations
 * Copyright 2012 Red Hat, Inc
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
#include "ctkcolorswatchprivate.h"
#include "ctkcolorswatchaccessibleprivate.h"

static void atk_action_interface_init (AtkActionIface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkColorSwatchAccessible, _ctk_color_swatch_accessible, CTK_TYPE_WIDGET_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, atk_action_interface_init))

static void
state_changed_cb (GtkWidget     *widget,
                  GtkStateFlags  previous_flags)
{
  AtkObject *accessible;
  GtkStateFlags flags;
  gboolean was_selected;
  gboolean selected;

  flags = ctk_widget_get_state_flags (widget);

  was_selected = (previous_flags & CTK_STATE_FLAG_SELECTED) != 0;
  selected = (flags & CTK_STATE_FLAG_SELECTED) != 0;

  accessible = ctk_widget_get_accessible (widget);
  if (selected && !was_selected)
    atk_object_notify_state_change (accessible, ATK_STATE_CHECKED, TRUE);
  else if (!selected && was_selected)
    atk_object_notify_state_change (accessible, ATK_STATE_CHECKED, FALSE);
}

static void
ctk_color_swatch_accessible_initialize (AtkObject *obj,
                                        gpointer   data)
{
  ATK_OBJECT_CLASS (_ctk_color_swatch_accessible_parent_class)->initialize (obj, data);

  g_signal_connect (data, "state-flags-changed",
                    G_CALLBACK (state_changed_cb), NULL);

  atk_object_set_role (obj, ATK_ROLE_RADIO_BUTTON);
}

static AtkStateSet *
ctk_color_swatch_accessible_ref_state_set (AtkObject *accessible)
{
  GtkWidget *widget;
  AtkStateSet *state_set;

  state_set = ATK_OBJECT_CLASS (_ctk_color_swatch_accessible_parent_class)->ref_state_set (accessible);

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget != NULL)
    {
      if ((ctk_widget_get_state_flags (widget) & CTK_STATE_FLAG_SELECTED) != 0)
        atk_state_set_add_state (state_set, ATK_STATE_CHECKED);
    }

  return state_set;
}

static void
ctk_color_swatch_accessible_notify_ctk (GObject    *obj,
                                        GParamSpec *pspec)
{
  GtkWidget *widget = CTK_WIDGET (obj);
  AtkObject *atk_obj = ctk_widget_get_accessible (widget);

  if (strcmp (pspec->name, "selectable") == 0)
    {
      if (ctk_color_swatch_get_selectable (CTK_COLOR_SWATCH (widget)))
        atk_object_set_role (atk_obj, ATK_ROLE_RADIO_BUTTON);
      else
        atk_object_set_role (atk_obj, ATK_ROLE_PUSH_BUTTON);
    }
  else
    CTK_WIDGET_ACCESSIBLE_CLASS (_ctk_color_swatch_accessible_parent_class)->notify_ctk (obj, pspec);
}

static void
_ctk_color_swatch_accessible_class_init (GtkColorSwatchAccessibleClass *klass)
{
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);
  GtkWidgetAccessibleClass *widget_class = (GtkWidgetAccessibleClass *)klass;

  atk_class->initialize = ctk_color_swatch_accessible_initialize;
  atk_class->ref_state_set = ctk_color_swatch_accessible_ref_state_set;

  widget_class->notify_ctk = ctk_color_swatch_accessible_notify_ctk;
}

static void
_ctk_color_swatch_accessible_init (GtkColorSwatchAccessible *scale)
{
}

static gint
ctk_color_swatch_accessible_get_n_actions (AtkAction *action)
{
  return 3;
}

static const gchar *
ctk_color_swatch_accessible_get_keybinding (AtkAction *action,
                                            gint       i)
{
  return NULL;
}

static const gchar *
ctk_color_swatch_accessible_get_name (AtkAction *action,
                                      gint       i)
{
  switch (i)
    {
    case 0: return "select";
    case 1: return "activate";
    case 2: return "customize";
    default: return NULL;
    }
}

static const gchar *
ctk_color_swatch_accessible_get_localized_name (AtkAction *action,
                                                gint       i)
{
  switch (i)
    {
    case 0: return C_("Action name", "Select");
    case 1: return C_("Action name", "Activate");
    case 2: return C_("Action name", "Customize");
    default: return NULL;
    }
}

static const gchar *
ctk_color_swatch_accessible_get_description (AtkAction *action,
                                             gint       i)
{
  switch (i)
    {
    case 0: return C_("Action description", "Selects the color");
    case 1: return C_("Action description", "Activates the color");
    case 2: return C_("Action description", "Customizes the color");
    default: return NULL;
    }
}

static gboolean
ctk_color_swatch_accessible_do_action (AtkAction *action,
                                       gint       i)
{
  GtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (action));
  if (widget == NULL)
    return FALSE;

  switch (i)
    {
    case 0:
      ctk_widget_set_state_flags (widget, CTK_STATE_FLAG_SELECTED, FALSE);
      break;

    case 1:
      g_signal_emit_by_name (widget, "activate");
      break;

    case 2:
      g_signal_emit_by_name (widget, "customize");
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

static void
atk_action_interface_init (AtkActionIface *iface)
{
  iface->do_action = ctk_color_swatch_accessible_do_action;
  iface->get_n_actions = ctk_color_swatch_accessible_get_n_actions;
  iface->get_keybinding = ctk_color_swatch_accessible_get_keybinding;
  iface->get_name = ctk_color_swatch_accessible_get_name;
  iface->get_localized_name = ctk_color_swatch_accessible_get_localized_name;
  iface->get_description = ctk_color_swatch_accessible_get_description;
}
