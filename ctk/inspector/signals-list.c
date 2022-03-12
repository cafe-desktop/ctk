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

#include "signals-list.h"

#include "ctkcellrenderer.h"
#include "ctkliststore.h"
#include "ctktextbuffer.h"
#include "ctktogglebutton.h"
#include "ctktreeviewcolumn.h"
#include "ctklabel.h"

enum
{
  COLUMN_NAME,
  COLUMN_CLASS,
  COLUMN_CONNECTED,
  COLUMN_COUNT,
  COLUMN_NO_HOOKS,
  COLUMN_SIGNAL_ID,
  COLUMN_HOOK_ID
};

enum
{
  PROP_0,
  PROP_TRACE_BUTTON,
  PROP_CLEAR_BUTTON
};

struct _CtkInspectorSignalsListPrivate
{
  CtkWidget *view;
  CtkListStore *model;
  CtkTextBuffer *text;
  CtkWidget *log_win;
  CtkWidget *trace_button;
  CtkWidget *clear_button;
  CtkTreeViewColumn *count_column;
  CtkCellRenderer *count_renderer;
  GObject *object;
  GHashTable *iters;
  gboolean tracing;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkInspectorSignalsList, ctk_inspector_signals_list, CTK_TYPE_PANED)

static GType *
get_types (GObject *object, guint *length)
{
  GHashTable *seen;
  GType *ret;
  GType type;
  gint i;

  seen = g_hash_table_new (g_direct_hash, g_direct_equal);

  type = ((GTypeInstance*)object)->g_class->g_type;
  while (type)
    {
      GType *iface;

      g_hash_table_add (seen, GSIZE_TO_POINTER (type));
      iface = g_type_interfaces (type, NULL);
      for (i = 0; iface[i]; i++)
        g_hash_table_add (seen, GSIZE_TO_POINTER (iface[i]));
      g_free (iface);
      type = g_type_parent (type);
    }
 
  ret = (GType *)g_hash_table_get_keys_as_array (seen, length); 
  g_hash_table_unref (seen);

  return ret;
}

static void
add_signals (CtkInspectorSignalsList *sl,
             GType                    type,
             GObject                 *object)
{
  guint *ids;
  guint n_ids;
  gint i;
  GSignalQuery query;
  CtkTreeIter iter;
  gboolean has_handler;

  if (!G_TYPE_IS_INSTANTIATABLE (type) && !G_TYPE_IS_INTERFACE (type))
    return;

  ids = g_signal_list_ids (type, &n_ids);
  for (i = 0; i < n_ids; i++)
    {
      g_signal_query (ids[i], &query);
      has_handler = g_signal_has_handler_pending (object, ids[i], 0, TRUE);
      ctk_list_store_append (sl->priv->model, &iter);
      ctk_list_store_set (sl->priv->model, &iter,
                          COLUMN_NAME, query.signal_name,
                          COLUMN_CLASS, g_type_name (type),
                          COLUMN_CONNECTED, has_handler ? _("Yes") : "",
                          COLUMN_COUNT, 0,
                          COLUMN_NO_HOOKS, (query.signal_flags & G_SIGNAL_NO_HOOKS) != 0,
                          COLUMN_SIGNAL_ID, ids[i],
                          COLUMN_HOOK_ID, 0,
                          -1);
      g_hash_table_insert (sl->priv->iters,
                           GINT_TO_POINTER (ids[i]), ctk_tree_iter_copy (&iter));
    }
  g_free (ids);
}

static void
read_signals_from_object (CtkInspectorSignalsList *sl,
                          GObject                 *object)
{
  GType *types;
  guint length;
  gint i;

  types = get_types (object, &length);
  for (i = 0; i < length; i++)
    add_signals (sl, types[i], object);
  g_free (types);
}

static void stop_tracing (CtkInspectorSignalsList *sl);

void
ctk_inspector_signals_list_set_object (CtkInspectorSignalsList *sl,
                                       GObject                 *object)
{
  if (sl->priv->object == object)
    return;

  stop_tracing (sl);
  ctk_list_store_clear (sl->priv->model);
  g_hash_table_remove_all (sl->priv->iters);

  sl->priv->object = object;

  if (object)
    read_signals_from_object (sl, object);
}

static void
render_count (CtkTreeViewColumn *column,
              CtkCellRenderer   *renderer,
              CtkTreeModel      *model,
              CtkTreeIter       *iter,
              gpointer           data)
{
  gint count;
  gboolean no_hooks;
  gchar text[100];

  ctk_tree_model_get (model, iter,
                      COLUMN_COUNT, &count,
                      COLUMN_NO_HOOKS, &no_hooks,
                      -1);
  if (no_hooks)
    {
      g_object_set (renderer, "markup", "<i>(untraceable)</i>", NULL);
    }
  else if (count != 0)
    {
      g_snprintf (text, 100, "%d", count);
      g_object_set (renderer, "text", text, NULL);
    }
  else
    g_object_set (renderer, "text", "", NULL);
}

static void
ctk_inspector_signals_list_init (CtkInspectorSignalsList *sl)
{
  sl->priv = ctk_inspector_signals_list_get_instance_private (sl);
  ctk_widget_init_template (CTK_WIDGET (sl));

  ctk_tree_view_column_set_cell_data_func (sl->priv->count_column,
                                           sl->priv->count_renderer,
                                           render_count,
                                           NULL, NULL);

  sl->priv->iters = g_hash_table_new_full (g_direct_hash, 
                                           g_direct_equal,
                                           NULL,
                                           (GDestroyNotify) ctk_tree_iter_free);
}

static gboolean
trace_hook (GSignalInvocationHint *ihint,
            guint                  n_param_values,
            const GValue          *param_values,
            gpointer               data)
{
  CtkInspectorSignalsList *sl = data;
  GObject *object;

  object = g_value_get_object (param_values);

  if (object == sl->priv->object)
    {
      gint count;
      CtkTreeIter *iter;

      iter = (CtkTreeIter *)g_hash_table_lookup (sl->priv->iters, GINT_TO_POINTER (ihint->signal_id));

      ctk_tree_model_get (CTK_TREE_MODEL (sl->priv->model), iter, COLUMN_COUNT, &count, -1);
      ctk_list_store_set (sl->priv->model, iter, COLUMN_COUNT, count + 1, -1);
    }

  return TRUE;
}

static gboolean
start_tracing_cb (CtkTreeModel *model,
                  CtkTreePath  *path,
                  CtkTreeIter  *iter,
                  gpointer      data)
{
  CtkInspectorSignalsList *sl = data;
  guint signal_id;
  gulong hook_id;
  gboolean no_hooks;

  ctk_tree_model_get (model, iter,
                      COLUMN_SIGNAL_ID, &signal_id,
                      COLUMN_HOOK_ID, &hook_id,
                      COLUMN_NO_HOOKS, &no_hooks,
                      -1);

  g_assert (signal_id != 0);
  g_assert (hook_id == 0);

  if (!no_hooks)
    {
      hook_id = g_signal_add_emission_hook (signal_id, 0, trace_hook, sl, NULL);

      ctk_list_store_set (CTK_LIST_STORE (model), iter,
                          COLUMN_COUNT, 0,
                          COLUMN_HOOK_ID, hook_id,
                          -1);
    }

  return FALSE;
}

static gboolean
stop_tracing_cb (CtkTreeModel *model,
                 CtkTreePath  *path,
                 CtkTreeIter  *iter,
                 gpointer      data)
{
  guint signal_id;
  gulong hook_id;

  ctk_tree_model_get (model, iter,
                      COLUMN_SIGNAL_ID, &signal_id,
                      COLUMN_HOOK_ID, &hook_id,
                      -1);

  g_assert (signal_id != 0);

  if (hook_id != 0)
    {
      g_signal_remove_emission_hook (signal_id, hook_id);
      ctk_list_store_set (CTK_LIST_STORE (model), iter,
                          COLUMN_HOOK_ID, 0,
                          -1);
    }

  return FALSE;
}

static void
start_tracing (CtkInspectorSignalsList *sl)
{
  sl->priv->tracing = TRUE;
  ctk_tree_model_foreach (CTK_TREE_MODEL (sl->priv->model), start_tracing_cb, sl);
}

static void
stop_tracing (CtkInspectorSignalsList *sl)
{
  sl->priv->tracing = FALSE;
  ctk_tree_model_foreach (CTK_TREE_MODEL (sl->priv->model), stop_tracing_cb, sl);
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (sl->priv->trace_button), FALSE);
}

static void
toggle_tracing (CtkToggleButton *button, CtkInspectorSignalsList *sl)
{
  if (ctk_toggle_button_get_active (button) == sl->priv->tracing)
    return;

  //ctk_widget_show (sl->priv->log_win);

  if (ctk_toggle_button_get_active (button))
    start_tracing (sl);
  else
    stop_tracing (sl);
}

static gboolean
clear_log_cb (CtkTreeModel *model,
              CtkTreePath  *path,
              CtkTreeIter  *iter,
              gpointer      data)
{
  ctk_list_store_set (CTK_LIST_STORE (model), iter, COLUMN_COUNT, 0, -1);
  return FALSE;
}

static void
clear_log (CtkButton *button, CtkInspectorSignalsList *sl)
{
  ctk_text_buffer_set_text (sl->priv->text, "", -1);

  ctk_tree_model_foreach (CTK_TREE_MODEL (sl->priv->model), clear_log_cb, sl);
}

static void
get_property (GObject    *object,
              guint       param_id,
              GValue     *value,
              GParamSpec *pspec)
{
  CtkInspectorSignalsList *sl = CTK_INSPECTOR_SIGNALS_LIST (object);

  switch (param_id)
    {
    case PROP_TRACE_BUTTON:
      g_value_take_object (value, sl->priv->trace_button);
      break;

    case PROP_CLEAR_BUTTON:
      g_value_take_object (value, sl->priv->trace_button);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
set_property (GObject      *object,
              guint         param_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  CtkInspectorSignalsList *sl = CTK_INSPECTOR_SIGNALS_LIST (object);

  switch (param_id)
    {
    case PROP_TRACE_BUTTON:
      sl->priv->trace_button = g_value_get_object (value);
      break;

    case PROP_CLEAR_BUTTON:
      sl->priv->clear_button = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
constructed (GObject *object)
{
  CtkInspectorSignalsList *sl = CTK_INSPECTOR_SIGNALS_LIST (object);

  g_signal_connect (sl->priv->trace_button, "toggled",
                    G_CALLBACK (toggle_tracing), sl);
  g_signal_connect (sl->priv->clear_button, "clicked",
                    G_CALLBACK (clear_log), sl);
}

static void
ctk_inspector_signals_list_class_init (CtkInspectorSignalsListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  object_class->constructed = constructed;
  object_class->get_property = get_property;
  object_class->set_property = set_property;

  g_object_class_install_property (object_class, PROP_TRACE_BUTTON,
      g_param_spec_object ("trace-button", NULL, NULL,
                           CTK_TYPE_WIDGET, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CLEAR_BUTTON,
      g_param_spec_object ("clear-button", NULL, NULL,
                           CTK_TYPE_WIDGET, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/inspector/signals-list.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorSignalsList, view);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorSignalsList, model);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorSignalsList, text);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorSignalsList, log_win);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorSignalsList, count_column);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorSignalsList, count_renderer);
}

// vim: set et sw=2 ts=2:
