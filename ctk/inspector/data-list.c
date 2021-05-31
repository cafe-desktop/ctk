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

#include "data-list.h"

#include "ctktreeview.h"
#include "ctkcellrenderertext.h"
#include "ctktogglebutton.h"
#include "ctklabel.h"


struct _CtkInspectorDataListPrivate
{
  CtkTreeModel *object;
  CtkTreeModel *types;
  CtkTreeView *view;
  CtkWidget *object_title;
  gboolean show_data;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkInspectorDataList, ctk_inspector_data_list, CTK_TYPE_BOX)

static void
ctk_inspector_data_list_init (CtkInspectorDataList *sl)
{
  sl->priv = ctk_inspector_data_list_get_instance_private (sl);
  ctk_widget_init_template (CTK_WIDGET (sl));
}

static void
cell_data_func (CtkTreeViewColumn *col,
                CtkCellRenderer   *cell,
                CtkTreeModel      *model,
                CtkTreeIter       *iter,
                gpointer           data)
{
  gint num;
  GValue gvalue = { 0, };
  gchar *value;

  num = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (col), "num"));
  ctk_tree_model_get_value (model, iter, num, &gvalue);
  value = g_strdup_value_contents (&gvalue);
  g_object_set (cell, "text", value ? value : "", NULL);
  g_free (value);
  g_value_unset (&gvalue);
}

static void
add_columns (CtkInspectorDataList *sl)
{
  gint n_columns;
  CtkCellRenderer *cell;
  GType type;
  gchar *title;
  CtkTreeViewColumn *col;
  gint i;

  n_columns = ctk_tree_model_get_n_columns (sl->priv->object);
  for (i = 0; i < n_columns; i++)
    {
      cell = ctk_cell_renderer_text_new ();
      type = ctk_tree_model_get_column_type (sl->priv->object, i);
      title = g_strdup_printf ("%d: %s", i, g_type_name (type));
      col = ctk_tree_view_column_new_with_attributes (title, cell, NULL);
      g_object_set_data (G_OBJECT (col), "num", GINT_TO_POINTER (i));
      ctk_tree_view_column_set_cell_data_func (col, cell, cell_data_func, sl, NULL);
      ctk_tree_view_append_column (sl->priv->view, col);
      g_free (title);
    } 
}

static void
show_types (CtkInspectorDataList *sl)
{
  ctk_tree_view_set_model (sl->priv->view, NULL);
  sl->priv->show_data = FALSE;
}

static void
show_data (CtkInspectorDataList *sl)
{
  ctk_tree_view_set_model (sl->priv->view, sl->priv->object);
  sl->priv->show_data = TRUE;
}

static void
clear_view (CtkInspectorDataList *sl)
{
  ctk_tree_view_set_model (sl->priv->view, NULL);
  while (ctk_tree_view_get_n_columns (sl->priv->view) > 0)
    ctk_tree_view_remove_column (sl->priv->view,
                                 ctk_tree_view_get_column (sl->priv->view, 0));
}

void
ctk_inspector_data_list_set_object (CtkInspectorDataList *sl,
                                    GObject              *object)
{
  const gchar *title;

  clear_view (sl);
  sl->priv->object = NULL;
  sl->priv->show_data = FALSE;

  if (!CTK_IS_TREE_MODEL (object))
    {
      ctk_widget_hide (CTK_WIDGET (sl));
      return;
    }

  title = (const gchar *)g_object_get_data (object, "ctk-inspector-object-title");
  ctk_label_set_label (CTK_LABEL (sl->priv->object_title), title);

  ctk_widget_show (CTK_WIDGET (sl));

  sl->priv->object = CTK_TREE_MODEL (object);
  add_columns (sl);
  show_types (sl);
}

static void
toggle_show (CtkToggleButton      *button,
             CtkInspectorDataList *sl)
{
  if (ctk_toggle_button_get_active (button) == sl->priv->show_data)
    return;

  if (ctk_toggle_button_get_active (button))
    show_data (sl);
  else
    show_types (sl);
}

static void
ctk_inspector_data_list_class_init (CtkInspectorDataListClass *klass)
{
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/inspector/data-list.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorDataList, view);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorDataList, object_title);
  ctk_widget_class_bind_template_callback (widget_class, toggle_show);
}

// vim: set et sw=2 ts=2:
