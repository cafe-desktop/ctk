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

#include "statistics.h"

#include "graphdata.h"
#include "ctkstack.h"
#include "ctktreeview.h"
#include "ctkcellrenderertext.h"
#include "ctkcelllayout.h"
#include "ctksearchbar.h"
#include "ctklabel.h"

enum
{
  PROP_0,
  PROP_BUTTON
};

struct _GtkInspectorStatisticsPrivate
{
  GtkWidget *stack;
  GtkWidget *excuse;
  GtkTreeModel *model;
  GtkTreeView  *view;
  GtkWidget *button;
  GHashTable *data;
  GtkTreeViewColumn *column_self1;
  GtkCellRenderer *renderer_self1;
  GtkTreeViewColumn *column_cumulative1;
  GtkCellRenderer *renderer_cumulative1;
  GtkTreeViewColumn *column_self2;
  GtkCellRenderer *renderer_self2;
  GtkTreeViewColumn *column_cumulative2;
  GtkCellRenderer *renderer_cumulative2;
  GHashTable *counts;
  guint update_source_id;
  GtkWidget *search_entry;
  GtkWidget *search_bar;
};

typedef struct {
  GType type;
  GtkTreeIter treeiter;
  GtkGraphData *self;
  GtkGraphData *cumulative;
} TypeData;

enum
{
  COLUMN_TYPE,
  COLUMN_TYPE_NAME,
  COLUMN_SELF1,
  COLUMN_CUMULATIVE1,
  COLUMN_SELF2,
  COLUMN_CUMULATIVE2,
  COLUMN_SELF_DATA,
  COLUMN_CUMULATIVE_DATA
};

G_DEFINE_TYPE_WITH_PRIVATE (GtkInspectorStatistics, ctk_inspector_statistics, CTK_TYPE_BOX)

static gint
add_type_count (GtkInspectorStatistics *sl, GType type)
{
  gint cumulative;
  gint self;
  GType *children;
  guint n_children;
  gint i;
  TypeData *data;

  cumulative = 0;

  children = g_type_children (type, &n_children);
  for (i = 0; i < n_children; i++)
    cumulative += add_type_count (sl, children[i]);

  data = g_hash_table_lookup (sl->priv->counts, GSIZE_TO_POINTER (type));
  if (!data)
    {
      data = g_new0 (TypeData, 1);
      data->type = type;
      data->self = ctk_graph_data_new (60);
      data->cumulative = ctk_graph_data_new (60);
      ctk_list_store_append (CTK_LIST_STORE (sl->priv->model), &data->treeiter);
      ctk_list_store_set (CTK_LIST_STORE (sl->priv->model), &data->treeiter,
                          COLUMN_TYPE, data->type,
                          COLUMN_TYPE_NAME, g_type_name (data->type),
                          COLUMN_SELF_DATA, data->self,
                          COLUMN_CUMULATIVE_DATA, data->cumulative,
                          -1);
      g_hash_table_insert (sl->priv->counts, GSIZE_TO_POINTER (type), data);
    }

  self = g_type_get_instance_count (type);
  cumulative += self;

  ctk_graph_data_prepend_value (data->self, self);
  ctk_graph_data_prepend_value (data->cumulative, cumulative);

  ctk_list_store_set (CTK_LIST_STORE (sl->priv->model), &data->treeiter,
                      COLUMN_SELF1, (int) ctk_graph_data_get_value (data->self, 1),
                      COLUMN_CUMULATIVE1, (int) ctk_graph_data_get_value (data->cumulative, 1),
                      COLUMN_SELF2, (int) ctk_graph_data_get_value (data->self, 0),
                      COLUMN_CUMULATIVE2, (int) ctk_graph_data_get_value (data->cumulative, 0),
                      -1);
  return cumulative;
}

static gboolean
update_type_counts (gpointer data)
{
  GtkInspectorStatistics *sl = data;
  GType type;
  gpointer class;

  for (type = G_TYPE_INTERFACE; type <= G_TYPE_FUNDAMENTAL_MAX; type += (1 << G_TYPE_FUNDAMENTAL_SHIFT))
    {
      class = g_type_class_peek (type);
      if (class == NULL)
        continue;

      if (!G_TYPE_IS_INSTANTIATABLE (type))
        continue;

      add_type_count (sl, type);
    }

  return TRUE;
}

static void
toggle_record (GtkToggleButton        *button,
               GtkInspectorStatistics *sl)
{
  if (ctk_toggle_button_get_active (button) == (sl->priv->update_source_id != 0))
    return;

  if (ctk_toggle_button_get_active (button))
    {
      sl->priv->update_source_id = gdk_threads_add_timeout_seconds (1,
                                                                    update_type_counts,
                                                                    sl);
      update_type_counts (sl);
    }
  else
    {
      g_source_remove (sl->priv->update_source_id);
      sl->priv->update_source_id = 0;
    }
}

static gboolean
has_instance_counts (void)
{
  return g_type_get_instance_count (CTK_TYPE_LABEL) > 0;
}

static gboolean
instance_counts_enabled (void)
{
  const gchar *string;
  guint flags = 0;

  string = g_getenv ("GOBJECT_DEBUG");
  if (string != NULL)
    {
      GDebugKey debug_keys[] = {
        { "objects", 1 },
        { "instance-count", 2 },
        { "signals", 4 }
      };

     flags = g_parse_debug_string (string, debug_keys, G_N_ELEMENTS (debug_keys));
    }

  return (flags & 2) != 0;
}

static void
cell_data_data (GtkCellLayout   *layout,
                GtkCellRenderer *cell,
                GtkTreeModel    *model,
                GtkTreeIter     *iter,
                gpointer         data)
{
  gint column;
  gint count;
  gchar *text;

  column = GPOINTER_TO_INT (data);

  ctk_tree_model_get (model, iter, column, &count, -1);

  text = g_strdup_printf ("%d", count);
  g_object_set (cell, "text", text, NULL);
  g_free (text);
}

static void
cell_data_delta (GtkCellLayout   *layout,
                 GtkCellRenderer *cell,
                 GtkTreeModel    *model,
                 GtkTreeIter     *iter,
                 gpointer         data)
{
  gint column;
  gint count1;
  gint count2;
  gchar *text;

  column = GPOINTER_TO_INT (data);

  ctk_tree_model_get (model, iter, column - 2, &count1, column, &count2, -1);

  if (count2 > count1)
    text = g_strdup_printf ("%d (↗ %d)", count2, count2 - count1);
  else if (count2 < count1)
    text = g_strdup_printf ("%d (↘ %d)", count2, count1 - count2);
  else
    text = g_strdup_printf ("%d", count2);
  g_object_set (cell, "text", text, NULL);
  g_free (text);
}

static void
type_data_free (gpointer data)
{
  TypeData *type_data = data;

  g_object_unref (type_data->self);
  g_object_unref (type_data->cumulative);

  g_free (type_data);
}

static gboolean
key_press_event (GtkWidget              *window,
                 GdkEvent               *event,
                 GtkInspectorStatistics *sl)
{
  if (ctk_widget_get_mapped (CTK_WIDGET (sl)))
    {
      if (event->key.keyval == GDK_KEY_Return ||
          event->key.keyval == GDK_KEY_ISO_Enter ||
          event->key.keyval == GDK_KEY_KP_Enter)
        {
          GtkTreeSelection *selection;
          GtkTreeModel *model;
          GtkTreeIter iter;
          GtkTreePath *path;

          selection = ctk_tree_view_get_selection (sl->priv->view);
          if (ctk_tree_selection_get_selected (selection, &model, &iter))
            {
              path = ctk_tree_model_get_path (model, &iter);
              ctk_tree_view_row_activated (sl->priv->view, path, NULL);
              ctk_tree_path_free (path);

              return GDK_EVENT_STOP;
            }
          else
            return GDK_EVENT_PROPAGATE;
        }

      return ctk_search_bar_handle_event (CTK_SEARCH_BAR (sl->priv->search_bar), event);
    }
  else
    return GDK_EVENT_PROPAGATE;
}

static gboolean
match_string (const gchar *string,
              const gchar *text)
{
  gchar *lower;
  gboolean match = FALSE;

  if (string)
    {
      lower = g_ascii_strdown (string, -1);
      match = g_str_has_prefix (lower, text);
      g_free (lower);
    }

  return match;
}

static gboolean
match_row (GtkTreeModel *model,
           gint          column,
           const gchar  *key,
           GtkTreeIter  *iter,
           gpointer      data)
{
  gchar *type;
  gboolean match;

  ctk_tree_model_get (model, iter, column, &type, -1);

  match = match_string (type, key);

  g_free (type);

  return !match;
}

static void
hierarchy_changed (GtkWidget *widget,
                   GtkWidget *previous_toplevel)
{
  if (previous_toplevel)
    g_signal_handlers_disconnect_by_func (previous_toplevel, key_press_event, widget);
  g_signal_connect (ctk_widget_get_toplevel (widget), "key-press-event",
                    G_CALLBACK (key_press_event), widget);
}

static void
ctk_inspector_statistics_init (GtkInspectorStatistics *sl)
{
  sl->priv = ctk_inspector_statistics_get_instance_private (sl);
  ctk_widget_init_template (CTK_WIDGET (sl));
  ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (sl->priv->column_self1),
                                      sl->priv->renderer_self1,
                                      cell_data_data,
                                      GINT_TO_POINTER (COLUMN_SELF1), NULL);
  ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (sl->priv->column_cumulative1),
                                      sl->priv->renderer_cumulative1,
                                      cell_data_data,
                                      GINT_TO_POINTER (COLUMN_CUMULATIVE1), NULL);
  ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (sl->priv->column_self2),
                                      sl->priv->renderer_self2,
                                      cell_data_delta,
                                      GINT_TO_POINTER (COLUMN_SELF2), NULL);
  ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (sl->priv->column_cumulative2),
                                      sl->priv->renderer_cumulative2,
                                      cell_data_delta,
                                      GINT_TO_POINTER (COLUMN_CUMULATIVE2), NULL);
  sl->priv->counts = g_hash_table_new_full (NULL, NULL, NULL, type_data_free);

  ctk_tree_view_set_search_entry (sl->priv->view, CTK_ENTRY (sl->priv->search_entry));
  ctk_tree_view_set_search_equal_func (sl->priv->view, match_row, sl, NULL);
  g_signal_connect (sl, "hierarchy-changed", G_CALLBACK (hierarchy_changed), NULL);
}

static void
constructed (GObject *object)
{
  GtkInspectorStatistics *sl = CTK_INSPECTOR_STATISTICS (object);

  g_signal_connect (sl->priv->button, "toggled",
                    G_CALLBACK (toggle_record), sl);

  if (has_instance_counts ())
    update_type_counts (sl);
  else
    {
      if (instance_counts_enabled ())
        ctk_label_set_text (CTK_LABEL (sl->priv->excuse), _("GLib must be configured with --enable-debug"));
      ctk_stack_set_visible_child_name (CTK_STACK (sl->priv->stack), "excuse");
      ctk_widget_set_sensitive (sl->priv->button, FALSE);
    }
}

static void
finalize (GObject *object)
{
  GtkInspectorStatistics *sl = CTK_INSPECTOR_STATISTICS (object);

  if (sl->priv->update_source_id)
    g_source_remove (sl->priv->update_source_id);

  g_hash_table_unref (sl->priv->counts);

  G_OBJECT_CLASS (ctk_inspector_statistics_parent_class)->finalize (object);
}

static void
get_property (GObject    *object,
              guint       param_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GtkInspectorStatistics *sl = CTK_INSPECTOR_STATISTICS (object);

  switch (param_id)
    {
    case PROP_BUTTON:
      g_value_take_object (value, sl->priv->button);
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
  GtkInspectorStatistics *sl = CTK_INSPECTOR_STATISTICS (object);

  switch (param_id)
    {
    case PROP_BUTTON:
      sl->priv->button = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
ctk_inspector_statistics_class_init (GtkInspectorStatisticsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  object_class->get_property = get_property;
  object_class->set_property = set_property;
  object_class->constructed = constructed;
  object_class->finalize = finalize;

  g_object_class_install_property (object_class, PROP_BUTTON,
      g_param_spec_object ("button", NULL, NULL,
                           CTK_TYPE_WIDGET, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/inspector/statistics.ui");
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorStatistics, view);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorStatistics, stack);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorStatistics, model);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorStatistics, column_self1);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorStatistics, renderer_self1);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorStatistics, column_cumulative1);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorStatistics, renderer_cumulative1);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorStatistics, column_self2);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorStatistics, renderer_self2);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorStatistics, column_cumulative2);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorStatistics, renderer_cumulative2);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorStatistics, search_entry);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorStatistics, search_bar);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorStatistics, excuse);

}

// vim: set et sw=2 ts=2:
