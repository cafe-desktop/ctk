/* GTK+ - accessibility implementations
 * Copyright 2004 Sun Microsystems Inc.
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
#include <gtk/gtk.h>
#include "gtkcomboboxaccessible.h"

struct _GtkComboBoxAccessiblePrivate
{
  gchar         *name;
  gint           old_selection;
  gboolean       popup_set;
};

static void atk_action_interface_init    (AtkActionIface    *iface);
static void atk_selection_interface_init (AtkSelectionIface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkComboBoxAccessible, ctk_combo_box_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE,
                         G_ADD_PRIVATE (GtkComboBoxAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, atk_action_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_SELECTION, atk_selection_interface_init))

static void
changed_cb (GtkWidget *widget)
{
  GtkComboBox *combo_box;
  AtkObject *obj;
  GtkComboBoxAccessible *accessible;
  gint index;

  combo_box = CTK_COMBO_BOX (widget);

  index = ctk_combo_box_get_active (combo_box);
  obj = ctk_widget_get_accessible (widget);
  accessible = CTK_COMBO_BOX_ACCESSIBLE (obj);
  if (accessible->priv->old_selection != index)
    {
      accessible->priv->old_selection = index;
      g_object_notify (G_OBJECT (obj), "accessible-name");
      g_signal_emit_by_name (obj, "selection-changed");
    }
}

static void
ctk_combo_box_accessible_initialize (AtkObject *obj,
                                     gpointer   data)
{
  GtkComboBox *combo_box;
  GtkComboBoxAccessible *accessible;
  AtkObject *popup;

  ATK_OBJECT_CLASS (ctk_combo_box_accessible_parent_class)->initialize (obj, data);

  combo_box = CTK_COMBO_BOX (data);
  accessible = CTK_COMBO_BOX_ACCESSIBLE (obj);

  g_signal_connect (combo_box, "changed", G_CALLBACK (changed_cb), NULL);
  accessible->priv->old_selection = ctk_combo_box_get_active (combo_box);

  popup = ctk_combo_box_get_popup_accessible (combo_box);
  if (popup)
    {
      atk_object_set_parent (popup, obj);
      accessible->priv->popup_set = TRUE;
    }
  if (ctk_combo_box_get_has_entry (combo_box))
    atk_object_set_parent (ctk_widget_get_accessible (ctk_bin_get_child (CTK_BIN (combo_box))), obj);

  obj->role = ATK_ROLE_COMBO_BOX;
}

static void
ctk_combo_box_accessible_finalize (GObject *object)
{
  GtkComboBoxAccessible *combo_box = CTK_COMBO_BOX_ACCESSIBLE (object);

  g_free (combo_box->priv->name);

  G_OBJECT_CLASS (ctk_combo_box_accessible_parent_class)->finalize (object);
}

static const gchar *
ctk_combo_box_accessible_get_name (AtkObject *obj)
{
  GtkWidget *widget;
  GtkComboBox *combo_box;
  GtkComboBoxAccessible *accessible;
  GtkTreeIter iter;
  const gchar *name;
  GtkTreeModel *model;
  gint n_columns;
  gint i;

  name = ATK_OBJECT_CLASS (ctk_combo_box_accessible_parent_class)->get_name (obj);
  if (name)
    return name;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  combo_box = CTK_COMBO_BOX (widget);
  accessible = CTK_COMBO_BOX_ACCESSIBLE (obj);
  if (ctk_combo_box_get_active_iter (combo_box, &iter))
    {
      model = ctk_combo_box_get_model (combo_box);
      n_columns = ctk_tree_model_get_n_columns (model);
      for (i = 0; i < n_columns; i++)
        {
          GValue value = G_VALUE_INIT;

          ctk_tree_model_get_value (model, &iter, i, &value);
          if (G_VALUE_HOLDS_STRING (&value))
            {
              g_free (accessible->priv->name);
              accessible->priv->name =  g_strdup (g_value_get_string (&value));
              g_value_unset (&value);
              break;
            }
          else
            g_value_unset (&value);
        }
    }
  return accessible->priv->name;
}

static gint
ctk_combo_box_accessible_get_n_children (AtkObject* obj)
{
  gint n_children = 0;
  GtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return 0;

  n_children++;
  if (ctk_combo_box_get_has_entry (CTK_COMBO_BOX (widget)))
    n_children++;

  return n_children;
}

static AtkObject *
ctk_combo_box_accessible_ref_child (AtkObject *obj,
                                    gint       i)
{
  GtkWidget *widget;
  AtkObject *child;
  GtkComboBoxAccessible *box;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  if (i == 0)
    {
      child = ctk_combo_box_get_popup_accessible (CTK_COMBO_BOX (widget));
      box = CTK_COMBO_BOX_ACCESSIBLE (obj);
      if (!box->priv->popup_set)
        {
          atk_object_set_parent (child, obj);
          box->priv->popup_set = TRUE;
        }
    }
  else if (i == 1 && ctk_combo_box_get_has_entry (CTK_COMBO_BOX (widget)))
    {
      child = ctk_widget_get_accessible (ctk_bin_get_child (CTK_BIN (widget)));
    }
  else
    {
      return NULL;
    }

  return g_object_ref (child);
}

static void
ctk_combo_box_accessible_class_init (GtkComboBoxAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = ctk_combo_box_accessible_finalize;

  class->get_name = ctk_combo_box_accessible_get_name;
  class->get_n_children = ctk_combo_box_accessible_get_n_children;
  class->ref_child = ctk_combo_box_accessible_ref_child;
  class->initialize = ctk_combo_box_accessible_initialize;
}

static void
ctk_combo_box_accessible_init (GtkComboBoxAccessible *combo_box)
{
  combo_box->priv = ctk_combo_box_accessible_get_instance_private (combo_box);
  combo_box->priv->old_selection = -1;
  combo_box->priv->name = NULL;
  combo_box->priv->popup_set = FALSE;
}

static gboolean
ctk_combo_box_accessible_do_action (AtkAction *action,
                                    gint       i)
{
  GtkComboBox *combo_box;
  GtkWidget *widget;
  gboolean popup_shown;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (action));
  if (widget == NULL)
    return FALSE;

  if (!ctk_widget_get_sensitive (widget) || !ctk_widget_get_visible (widget))
    return FALSE;

  if (i != 0)
    return FALSE;

  combo_box = CTK_COMBO_BOX (widget);
  g_object_get (combo_box, "popup-shown", &popup_shown, NULL);
  if (popup_shown)
    ctk_combo_box_popdown (combo_box);
  else
    ctk_combo_box_popup (combo_box);

  return TRUE;
}

static gint
ctk_combo_box_accessible_get_n_actions (AtkAction *action)
{
  return 1;
}

static const gchar *
ctk_combo_box_accessible_get_keybinding (AtkAction *action,
                                         gint       i)
{
  GtkComboBoxAccessible *combo_box;
  GtkWidget *widget;
  GtkWidget *label;
  AtkRelationSet *set;
  AtkRelation *relation;
  GPtrArray *target;
  gpointer target_object;
  guint key_val;
  gchar *return_value = NULL;

  if (i != 0)
    return NULL;

  combo_box = CTK_COMBO_BOX_ACCESSIBLE (action);
  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (combo_box));
  if (widget == NULL)
    return NULL;

  set = atk_object_ref_relation_set (ATK_OBJECT (action));
  if (set == NULL)
    return NULL;

  label = NULL;
  relation = atk_relation_set_get_relation_by_type (set, ATK_RELATION_LABELLED_BY);
  if (relation)
    {
      target = atk_relation_get_target (relation);
      target_object = g_ptr_array_index (target, 0);
      label = ctk_accessible_get_widget (CTK_ACCESSIBLE (target_object));
    }
  g_object_unref (set);
  if (CTK_IS_LABEL (label))
    {
      key_val = ctk_label_get_mnemonic_keyval (CTK_LABEL (label));
      if (key_val != GDK_KEY_VoidSymbol)
        return_value = ctk_accelerator_name (key_val, GDK_MOD1_MASK);
    }

  return return_value;
}

static const gchar *
ctk_combo_box_accessible_action_get_name (AtkAction *action,
                                          gint       i)
{
  if (i == 0)
    return "press";
  return NULL;
}

static const gchar *
ctk_combo_box_accessible_action_get_localized_name (AtkAction *action,
                                                    gint       i)
{
  if (i == 0)
    return C_("Action name", "Press");
  return NULL;
}

static const gchar *
ctk_combo_box_accessible_action_get_description (AtkAction *action,
                                                 gint       i)
{
  if (i == 0)
    return C_("Action description", "Presses the combobox");
  return NULL;
}

static void
atk_action_interface_init (AtkActionIface *iface)
{
  iface->do_action = ctk_combo_box_accessible_do_action;
  iface->get_n_actions = ctk_combo_box_accessible_get_n_actions;
  iface->get_keybinding = ctk_combo_box_accessible_get_keybinding;
  iface->get_name = ctk_combo_box_accessible_action_get_name;
  iface->get_localized_name = ctk_combo_box_accessible_action_get_localized_name;
  iface->get_description = ctk_combo_box_accessible_action_get_description;
}

static gboolean
ctk_combo_box_accessible_add_selection (AtkSelection *selection,
                                        gint          i)
{
  GtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), i);

  return TRUE;
}

static gboolean
ctk_combo_box_accessible_clear_selection (AtkSelection *selection)
{
  GtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), -1);

  return TRUE;
}

static AtkObject *
ctk_combo_box_accessible_ref_selection (AtkSelection *selection,
                                        gint          i)
{
  GtkComboBox *combo_box;
  GtkWidget *widget;
  AtkObject *obj;
  gint index;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return NULL;

  if (i != 0)
    return NULL;

  combo_box = CTK_COMBO_BOX (widget);

  obj = ctk_combo_box_get_popup_accessible (combo_box);
  index = ctk_combo_box_get_active (combo_box);

  return atk_object_ref_accessible_child (obj, index);
}

static gint
ctk_combo_box_accessible_get_selection_count (AtkSelection *selection)
{
  GtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return 0;

  return (ctk_combo_box_get_active (CTK_COMBO_BOX (widget)) == -1) ? 0 : 1;
}

static gboolean
ctk_combo_box_accessible_is_child_selected (AtkSelection *selection,
                                            gint          i)
{
  GtkWidget *widget;
  gint j;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));

  if (widget == NULL)
    return FALSE;

  j = ctk_combo_box_get_active (CTK_COMBO_BOX (widget));

  return (j == i);
}

static gboolean
ctk_combo_box_accessible_remove_selection (AtkSelection *selection,
                                           gint          i)
{
  if (atk_selection_is_child_selected (selection, i))
    atk_selection_clear_selection (selection);

  return TRUE;
}

static void
atk_selection_interface_init (AtkSelectionIface *iface)
{
  iface->add_selection = ctk_combo_box_accessible_add_selection;
  iface->clear_selection = ctk_combo_box_accessible_clear_selection;
  iface->ref_selection = ctk_combo_box_accessible_ref_selection;
  iface->get_selection_count = ctk_combo_box_accessible_get_selection_count;
  iface->is_child_selected = ctk_combo_box_accessible_is_child_selected;
  iface->remove_selection = ctk_combo_box_accessible_remove_selection;
}
