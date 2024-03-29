/*
 * Copyright (c) 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "object-hierarchy.h"

#include "ctktreeselection.h"
#include "ctktreestore.h"
#include "ctktreeview.h"
#include "ctklabel.h"


enum
{
  COLUMN_OBJECT_NAME
};

struct _CtkInspectorObjectHierarchyPrivate
{
  CtkTreeStore *model;
  CtkTreeView *tree;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkInspectorObjectHierarchy, ctk_inspector_object_hierarchy, CTK_TYPE_BOX)

static void
ctk_inspector_object_hierarchy_init (CtkInspectorObjectHierarchy *oh)
{
  oh->priv = ctk_inspector_object_hierarchy_get_instance_private (oh);
  ctk_widget_init_template (CTK_WIDGET (oh));
}

static void
ctk_inspector_object_hierarchy_class_init (CtkInspectorObjectHierarchyClass *klass)
{
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/inspector/object-hierarchy.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorObjectHierarchy, model);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorObjectHierarchy, tree);
}

void
ctk_inspector_object_hierarchy_set_object (CtkInspectorObjectHierarchy *oh,
                                           GObject                     *object)
{
  GType type;
  const gchar *class_name;
  CtkTreeIter iter, parent;
  GList *list = NULL, *l;
  GHashTable *interfaces;
  GHashTableIter hit;
  gint i;

  ctk_tree_store_clear (oh->priv->model);

  if (object == NULL)
    return;

  interfaces = g_hash_table_new (g_str_hash, g_str_equal);
  type = ((GTypeInstance*)object)->g_class->g_type;

  do
    {
      GType *ifaces;

      class_name = g_type_name (type);
      list = g_list_append (list, (gpointer)class_name);
      ifaces = g_type_interfaces (type, NULL);
      for (i = 0; ifaces[i]; i++)
        g_hash_table_add (interfaces, (gchar *)g_type_name (ifaces[i]));
      g_free (ifaces);
    }
  while ((type = g_type_parent (type)));

  if (g_hash_table_size (interfaces) > 0)
    {
      ctk_tree_store_append (oh->priv->model, &iter, NULL);
      ctk_tree_store_set (oh->priv->model, &iter,
                          COLUMN_OBJECT_NAME, "GInterface",
                          -1);
      parent = iter;
    }
  g_hash_table_iter_init (&hit, interfaces);
  while (g_hash_table_iter_next (&hit, (gpointer *)&class_name, NULL))
    {
      ctk_tree_store_append (oh->priv->model, &iter, &parent);
      ctk_tree_store_set (oh->priv->model, &iter,
                          COLUMN_OBJECT_NAME, class_name,
                          -1);
    }
  g_hash_table_unref (interfaces);

  list = g_list_reverse (list);
  for (l = list; l; l = l->next)
    {
      ctk_tree_store_append (oh->priv->model, &iter, l == list ? NULL : &parent);
      ctk_tree_store_set (oh->priv->model, &iter,
                          COLUMN_OBJECT_NAME, l->data,
                          -1);
      parent = iter;
    }

  g_list_free (list);

  ctk_tree_view_expand_all (oh->priv->tree);
  ctk_tree_selection_select_iter (ctk_tree_view_get_selection (oh->priv->tree), &iter);
}

// vim: set et sw=2 ts=2:
