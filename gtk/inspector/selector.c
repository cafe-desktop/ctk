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

#include "selector.h"

#include "gtktreeselection.h"
#include "gtktreestore.h"
#include "gtktreeview.h"
#include "gtkwidgetpath.h"
#include "gtklabel.h"


enum
{
  COLUMN_SELECTOR
};

struct _GtkInspectorSelectorPrivate
{
  GtkTreeStore *model;
  GtkTreeView *tree;
};

G_DEFINE_TYPE_WITH_PRIVATE (GtkInspectorSelector, ctk_inspector_selector, GTK_TYPE_BOX)

static void
ctk_inspector_selector_init (GtkInspectorSelector *oh)
{
  oh->priv = ctk_inspector_selector_get_instance_private (oh);
  ctk_widget_init_template (GTK_WIDGET (oh));
}

static void
ctk_inspector_selector_class_init (GtkInspectorSelectorClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  ctk_widget_class_set_template_from_resource (widget_class, "/org/gtk/libgtk/inspector/selector.ui");
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorSelector, model);
  ctk_widget_class_bind_template_child_private (widget_class, GtkInspectorSelector, tree);
}

void
ctk_inspector_selector_set_object (GtkInspectorSelector *oh,
                                   GObject              *object)
{
  GtkTreeIter iter, parent;
  gint i;
  GtkWidget *widget;
  gchar *path, **words;

  ctk_tree_store_clear (oh->priv->model);

  if (!GTK_IS_WIDGET (object))
    {
      ctk_widget_hide (GTK_WIDGET (oh));
      return;
    }

  widget = GTK_WIDGET (object);

  path = ctk_widget_path_to_string (ctk_widget_get_path (widget));
  words = g_strsplit (path, " ", 0);

  for (i = 0; words[i]; i++)
    {
      ctk_tree_store_append (oh->priv->model, &iter, i ? &parent : NULL);
      ctk_tree_store_set (oh->priv->model, &iter,
                          COLUMN_SELECTOR, words[i],
                          -1);
      parent = iter;
    }

  g_strfreev (words);
  g_free (path);

  ctk_tree_view_expand_all (oh->priv->tree);
  ctk_tree_selection_select_iter (ctk_tree_view_get_selection (oh->priv->tree), &iter);

  ctk_widget_show (GTK_WIDGET (oh));
}

// vim: set et sw=2 ts=2:
