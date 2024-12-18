/*
 * Copyright (c) 2014 Red Hat, Inc.
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

#include "actions.h"
#include "action-editor.h"

#include "ctkapplication.h"
#include "ctkapplicationwindow.h"
#include "ctktreeview.h"
#include "ctkliststore.h"
#include "ctkwidgetprivate.h"
#include "ctkpopover.h"
#include "ctklabel.h"

enum
{
  COLUMN_PREFIX,
  COLUMN_NAME,
  COLUMN_ENABLED,
  COLUMN_PARAMETER,
  COLUMN_STATE,
  COLUMN_GROUP
};

struct _CtkInspectorActionsPrivate
{
  CtkListStore *model;
  GHashTable *groups;
  GHashTable *iters;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkInspectorActions, ctk_inspector_actions, CTK_TYPE_BOX)

static void
ctk_inspector_actions_init (CtkInspectorActions *sl)
{
  sl->priv = ctk_inspector_actions_get_instance_private (sl);
  sl->priv->iters = g_hash_table_new_full (g_str_hash,
                                           g_str_equal,
                                           g_free,
                                           (GDestroyNotify) ctk_tree_iter_free);
  sl->priv->groups = g_hash_table_new_full (g_direct_hash,
                                            g_direct_equal,
                                            NULL,
                                            g_free);
  ctk_widget_init_template (CTK_WIDGET (sl));
}

static void
add_action (CtkInspectorActions *sl,
            GActionGroup        *group,
            const gchar         *prefix,
            const gchar         *name)
{
  CtkTreeIter iter;
  gboolean enabled;
  const gchar *parameter;
  GVariant *state;
  gchar *state_string;

  enabled = g_action_group_get_action_enabled (group, name);
  parameter = (const gchar *)g_action_group_get_action_parameter_type (group, name);
  state = g_action_group_get_action_state (group, name);
  if (state)
    state_string = g_variant_print (state, FALSE);
  else
    state_string = g_strdup ("");
  ctk_list_store_append (sl->priv->model, &iter);
  ctk_list_store_set (sl->priv->model, &iter,
                      COLUMN_PREFIX, prefix,
                      COLUMN_NAME, name,
                      COLUMN_ENABLED, enabled,
                      COLUMN_PARAMETER, parameter,
                      COLUMN_STATE, state_string,
                      COLUMN_GROUP, group,
                      -1);
  g_hash_table_insert (sl->priv->iters,
                       g_strconcat (prefix, ".", name, NULL),
                       ctk_tree_iter_copy (&iter));
  g_free (state_string);
}

static void
action_added_cb (GActionGroup        *group,
                 const gchar         *action_name,
                 CtkInspectorActions *sl)
{
  const gchar *prefix;
  prefix = g_hash_table_lookup (sl->priv->groups, group);
  add_action (sl, group, prefix, action_name);
}

static void
action_removed_cb (GActionGroup        *group,
                   const gchar         *action_name,
                   CtkInspectorActions *sl)
{
  const gchar *prefix;
  gchar *key;
  CtkTreeIter *iter;
  prefix = g_hash_table_lookup (sl->priv->groups, group);
  key = g_strconcat (prefix, ".", action_name, NULL);
  iter = g_hash_table_lookup (sl->priv->iters, key);
  ctk_list_store_remove (sl->priv->model, iter);
  g_hash_table_remove (sl->priv->iters, key);
  g_free (key);
}

static void
action_enabled_changed_cb (GActionGroup        *group,
                           const gchar         *action_name,
                           gboolean             enabled,
                           CtkInspectorActions *sl)
{
  const gchar *prefix;
  gchar *key;
  CtkTreeIter *iter;
  prefix = g_hash_table_lookup (sl->priv->groups, group);
  key = g_strconcat (prefix, ".", action_name, NULL);
  iter = g_hash_table_lookup (sl->priv->iters, key);
  ctk_list_store_set (sl->priv->model, iter,
                      COLUMN_ENABLED, enabled,
                      -1);
  g_free (key);
}

static void
action_state_changed_cb (GActionGroup        *group,
                         const gchar         *action_name,
                         GVariant            *state,
                         CtkInspectorActions *sl)
{
  const gchar *prefix;
  gchar *key;
  CtkTreeIter *iter;
  gchar *state_string;
  prefix = g_hash_table_lookup (sl->priv->groups, group);
  key = g_strconcat (prefix, ".", action_name, NULL);
  iter = g_hash_table_lookup (sl->priv->iters, key);
  if (state)
    state_string = g_variant_print (state, FALSE);
  else
    state_string = g_strdup ("");
  ctk_list_store_set (sl->priv->model, iter,
                      COLUMN_STATE, state_string,
                      -1);
  g_free (state_string);
  g_free (key);
}

static void
add_group (CtkInspectorActions *sl,
           GActionGroup        *group,
           const gchar         *prefix)
{
  gint i;
  gchar **names;

  ctk_widget_show (CTK_WIDGET (sl));

  g_signal_connect (group, "action-added", G_CALLBACK (action_added_cb), sl);
  g_signal_connect (group, "action-removed", G_CALLBACK (action_removed_cb), sl);
  g_signal_connect (group, "action-enabled-changed", G_CALLBACK (action_enabled_changed_cb), sl);
  g_signal_connect (group, "action-state-changed", G_CALLBACK (action_state_changed_cb), sl);
  g_hash_table_insert (sl->priv->groups, group, g_strdup (prefix));

  names = g_action_group_list_actions (group);
  for (i = 0; names[i]; i++)
    add_action (sl, group, prefix, names[i]);
  g_strfreev (names);
}

static void
disconnect_group (gpointer key,
                  gpointer value G_GNUC_UNUSED,
                  gpointer data)
{
  GActionGroup *group = key;
  CtkInspectorActions *sl = data;

  g_signal_handlers_disconnect_by_func (group, action_added_cb, sl);
  g_signal_handlers_disconnect_by_func (group, action_removed_cb, sl);
  g_signal_handlers_disconnect_by_func (group, action_enabled_changed_cb, sl);
  g_signal_handlers_disconnect_by_func (group, action_state_changed_cb, sl);
}

void
ctk_inspector_actions_set_object (CtkInspectorActions *sl,
                                  GObject             *object)
{
  ctk_widget_hide (CTK_WIDGET (sl));
  g_hash_table_foreach (sl->priv->groups, disconnect_group, sl);
  g_hash_table_remove_all (sl->priv->groups);
  g_hash_table_remove_all (sl->priv->iters);
  ctk_list_store_clear (sl->priv->model);
  
  if (CTK_IS_APPLICATION (object))
    add_group (sl, G_ACTION_GROUP (object), "app");
  else if (CTK_IS_APPLICATION_WINDOW (object))
    add_group (sl, G_ACTION_GROUP (object), "win");
  else if (CTK_IS_WIDGET (object))
    {
      const gchar **prefixes;
      gint i;

      prefixes = ctk_widget_list_action_prefixes (CTK_WIDGET (object));
      if (prefixes)
        {
          for (i = 0; prefixes[i]; i++)
            {
              GActionGroup *group;

              group = ctk_widget_get_action_group (CTK_WIDGET (object), prefixes[i]);
              add_group (sl, group, prefixes[i]);
            }
          g_free (prefixes);
        }
    }
}

static void
row_activated (CtkTreeView         *tv,
               CtkTreePath         *path,
               CtkTreeViewColumn   *col,
               CtkInspectorActions *sl)
{
  CtkTreeIter iter;
  CdkRectangle rect;
  CtkWidget *popover;
  gchar *prefix;
  gchar *name;
  GActionGroup *group;
  CtkWidget *editor;

  ctk_tree_model_get_iter (CTK_TREE_MODEL (sl->priv->model), &iter, path);
  ctk_tree_model_get (CTK_TREE_MODEL (sl->priv->model),
                      &iter,
                      COLUMN_PREFIX, &prefix,
                      COLUMN_NAME, &name,
                      COLUMN_GROUP, &group,
                      -1);

  ctk_tree_model_get_iter (CTK_TREE_MODEL (sl->priv->model), &iter, path);
  ctk_tree_view_get_cell_area (tv, path, col, &rect);
  ctk_tree_view_convert_bin_window_to_widget_coords (tv, rect.x, rect.y, &rect.x, &rect.y);

  popover = ctk_popover_new (CTK_WIDGET (tv));
  ctk_popover_set_pointing_to (CTK_POPOVER (popover), &rect);

  editor = ctk_inspector_action_editor_new (group, prefix, name);
  ctk_container_add (CTK_CONTAINER (popover), editor);
  ctk_popover_popup (CTK_POPOVER (popover));

  g_signal_connect (popover, "hide", G_CALLBACK (ctk_widget_destroy), NULL);

  g_free (name);
  g_free (prefix);
}

static void
ctk_inspector_actions_class_init (CtkInspectorActionsClass *klass)
{
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/inspector/actions.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorActions, model);
  ctk_widget_class_bind_template_callback (widget_class, row_activated);
}

// vim: set et sw=2 ts=2:
