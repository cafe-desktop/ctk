/* ctktreestore.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ctktreemodelcssnode.h"
#include "ctk/ctkcsstransientnodeprivate.h"
#include "fallback-memdup.h"

struct _CtkTreeModelCssNodePrivate
{
  CtkTreeModelCssNodeGetFunc    get_func;
  gint                          n_columns;
  GType                        *column_types;

  CtkCssNode                   *root;
};

static void ctk_tree_model_css_node_connect_node    (CtkTreeModelCssNode *model,
                                                     CtkCssNode          *node,
                                                     gboolean             emit_signal);

static void ctk_tree_model_css_node_disconnect_node (CtkTreeModelCssNode *model,
                                                     CtkCssNode          *node,
                                                     gboolean             emit_signal,
                                                     CtkCssNode          *parent,
                                                     CtkCssNode          *previous);

static void ctk_tree_model_css_node_tree_model_init (CtkTreeModelIface   *iface);

G_DEFINE_TYPE_WITH_CODE (CtkTreeModelCssNode, ctk_tree_model_css_node, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (CtkTreeModelCssNode)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_TREE_MODEL,
						ctk_tree_model_css_node_tree_model_init))

static CtkCssNode *
get_nth_child (CtkCssNode *node,
               gint        i)
{
  for (node = ctk_css_node_get_first_child (node);
       node != NULL && i > 0;
       node = ctk_css_node_get_next_sibling (node))
    i--;

  return node;
}

static int
get_node_index (CtkCssNode *node)
{
  int result = 0;

  while ((node = ctk_css_node_get_previous_sibling (node)))
    result++;

  return result;
}

static CtkTreeModelFlags
ctk_tree_model_css_node_get_flags (CtkTreeModel *tree_model)
{
  return CTK_TREE_MODEL_ITERS_PERSIST;
}

static gint
ctk_tree_model_css_node_get_n_columns (CtkTreeModel *tree_model)
{
  CtkTreeModelCssNode *nodemodel = CTK_TREE_MODEL_CSS_NODE (tree_model);
  CtkTreeModelCssNodePrivate *priv = nodemodel->priv;

  return priv->n_columns;
}

static GType
ctk_tree_model_css_node_get_column_type (CtkTreeModel *tree_model,
                                         gint          column)
{
  CtkTreeModelCssNode *nodemodel = CTK_TREE_MODEL_CSS_NODE (tree_model);
  CtkTreeModelCssNodePrivate *priv = nodemodel->priv;

  g_return_val_if_fail (column < priv->n_columns, G_TYPE_INVALID);

  return priv->column_types[column];
}

static gboolean
ctk_tree_model_css_node_get_iter (CtkTreeModel *tree_model,
                                  CtkTreeIter  *iter,
                                  CtkTreePath  *path)
{
  CtkTreeModelCssNode *nodemodel = CTK_TREE_MODEL_CSS_NODE (tree_model);
  CtkTreeModelCssNodePrivate *priv = nodemodel->priv;
  CtkCssNode *node;
  int *indices;
  int depth, i;

  if (priv->root == NULL)
    return FALSE;

  indices = ctk_tree_path_get_indices (path);
  depth = ctk_tree_path_get_depth (path);

  if (depth < 1 || indices[0] != 0)
    return FALSE;

  node = priv->root;
  for (i = 1; i < depth; i++)
    {
      node = get_nth_child (node, indices[i]);
      if (node == NULL)
        return FALSE;
    }

  ctk_tree_model_css_node_get_iter_from_node (nodemodel, iter, node);
  return TRUE;
}

static CtkTreePath *
ctk_tree_model_css_node_get_path (CtkTreeModel *tree_model,
			          CtkTreeIter  *iter)
{
  CtkTreeModelCssNode *nodemodel = CTK_TREE_MODEL_CSS_NODE (tree_model);
  CtkTreeModelCssNodePrivate *priv = nodemodel->priv;
  CtkCssNode *node;
  CtkTreePath *path;

  g_return_val_if_fail (priv->root != NULL, NULL);

  path = ctk_tree_path_new ();
  node = ctk_tree_model_css_node_get_node_from_iter (nodemodel, iter);

  while (node != priv->root)
    {
      ctk_tree_path_prepend_index (path, get_node_index (node));
      node = ctk_css_node_get_parent (node);
    }

  ctk_tree_path_prepend_index (path, 0);

  return path;
}

static void
ctk_tree_model_css_node_get_value (CtkTreeModel *tree_model,
                                   CtkTreeIter  *iter,
                                   gint          column,
                                   GValue       *value)
{
  CtkTreeModelCssNode *nodemodel = CTK_TREE_MODEL_CSS_NODE (tree_model);
  CtkTreeModelCssNodePrivate *priv = nodemodel->priv;

  g_value_init (value, priv->column_types[column]);
  priv->get_func (nodemodel,
                  ctk_tree_model_css_node_get_node_from_iter (nodemodel, iter),
                  column,
                  value);
}

static gboolean
ctk_tree_model_css_node_iter_next (CtkTreeModel  *tree_model,
			           CtkTreeIter   *iter)
{
  CtkTreeModelCssNode *nodemodel = CTK_TREE_MODEL_CSS_NODE (tree_model);
  CtkTreeModelCssNodePrivate *priv = nodemodel->priv;
  CtkCssNode *node;

  node = ctk_tree_model_css_node_get_node_from_iter (nodemodel, iter);
  if (node == priv->root)
    return FALSE;
  
  node = ctk_css_node_get_next_sibling (node);
  if (node == NULL)
    return FALSE;

  ctk_tree_model_css_node_get_iter_from_node (nodemodel, iter, node);
  return TRUE;
}

static gboolean
ctk_tree_model_css_node_iter_previous (CtkTreeModel  *tree_model,
			               CtkTreeIter   *iter)
{
  CtkTreeModelCssNode *nodemodel = CTK_TREE_MODEL_CSS_NODE (tree_model);
  CtkTreeModelCssNodePrivate *priv = nodemodel->priv;
  CtkCssNode *node;

  node = ctk_tree_model_css_node_get_node_from_iter (nodemodel, iter);
  if (node == priv->root)
    return FALSE;
  
  node = ctk_css_node_get_previous_sibling (node);
  if (node == NULL)
    return FALSE;

  ctk_tree_model_css_node_get_iter_from_node (nodemodel, iter, node);
  return TRUE;
}

static gboolean
ctk_tree_model_css_node_iter_children (CtkTreeModel *tree_model,
                                       CtkTreeIter  *iter,
                                       CtkTreeIter  *parent)
{
  CtkTreeModelCssNode *nodemodel = CTK_TREE_MODEL_CSS_NODE (tree_model);
  CtkTreeModelCssNodePrivate *priv = nodemodel->priv;
  CtkCssNode *node;

  if (parent == NULL)
    {
      node = priv->root;
    }
  else
    {
      node = ctk_tree_model_css_node_get_node_from_iter (nodemodel, parent);
      node = ctk_css_node_get_first_child (node);
    }
  if (node == NULL)
    return FALSE;

  ctk_tree_model_css_node_get_iter_from_node (nodemodel, iter, node);
  return TRUE;
}

static gboolean
ctk_tree_model_css_node_iter_has_child (CtkTreeModel *tree_model,
			                CtkTreeIter  *iter)
{
  CtkTreeModelCssNode *nodemodel = CTK_TREE_MODEL_CSS_NODE (tree_model);
  CtkCssNode *node;

  node = ctk_tree_model_css_node_get_node_from_iter (nodemodel, iter);

  return ctk_css_node_get_first_child (node) != NULL;
}

static gint
ctk_tree_model_css_node_iter_n_children (CtkTreeModel *tree_model,
                                         CtkTreeIter  *iter)
{
  CtkTreeModelCssNode *nodemodel = CTK_TREE_MODEL_CSS_NODE (tree_model);
  CtkTreeModelCssNodePrivate *priv = nodemodel->priv;
  CtkCssNode *node;

  if (iter == NULL)
    return priv->root ? 1 : 0;

  node = ctk_tree_model_css_node_get_node_from_iter (nodemodel, iter);

  node = ctk_css_node_get_last_child (node);
  if (node == NULL)
    return 0;

  return get_node_index (node) + 1;
}

static gboolean
ctk_tree_model_css_node_iter_nth_child (CtkTreeModel *tree_model,
                                        CtkTreeIter  *iter,
                                        CtkTreeIter  *parent,
                                        gint          n)
{
  CtkTreeModelCssNode *nodemodel = CTK_TREE_MODEL_CSS_NODE (tree_model);
  CtkTreeModelCssNodePrivate *priv = nodemodel->priv;
  CtkCssNode *node;

  if (parent == NULL)
    {
      if (n > 0)
        return FALSE;
      
      node = priv->root;
    }
  else
    {
      node = ctk_tree_model_css_node_get_node_from_iter (nodemodel, parent);
      node = get_nth_child (node, n);
    }

  if (node == NULL)
    return FALSE;
  
  ctk_tree_model_css_node_get_iter_from_node (nodemodel, iter, node);
  return TRUE;
}

static gboolean
ctk_tree_model_css_node_iter_parent (CtkTreeModel *tree_model,
                                     CtkTreeIter  *iter,
                                     CtkTreeIter  *child)
{
  CtkTreeModelCssNode *nodemodel = CTK_TREE_MODEL_CSS_NODE (tree_model);
  CtkTreeModelCssNodePrivate *priv = nodemodel->priv;
  CtkCssNode *node;

  node = ctk_tree_model_css_node_get_node_from_iter (nodemodel, child);
  if (node == priv->root)
    return FALSE;

  node = ctk_css_node_get_parent (node);

  ctk_tree_model_css_node_get_iter_from_node (nodemodel, iter, node);
  return TRUE;
}

static void
ctk_tree_model_css_node_tree_model_init (CtkTreeModelIface *iface)
{
  iface->get_flags = ctk_tree_model_css_node_get_flags;
  iface->get_n_columns = ctk_tree_model_css_node_get_n_columns;
  iface->get_column_type = ctk_tree_model_css_node_get_column_type;
  iface->get_iter = ctk_tree_model_css_node_get_iter;
  iface->get_path = ctk_tree_model_css_node_get_path;
  iface->get_value = ctk_tree_model_css_node_get_value;
  iface->iter_next = ctk_tree_model_css_node_iter_next;
  iface->iter_previous = ctk_tree_model_css_node_iter_previous;
  iface->iter_children = ctk_tree_model_css_node_iter_children;
  iface->iter_has_child = ctk_tree_model_css_node_iter_has_child;
  iface->iter_n_children = ctk_tree_model_css_node_iter_n_children;
  iface->iter_nth_child = ctk_tree_model_css_node_iter_nth_child;
  iface->iter_parent = ctk_tree_model_css_node_iter_parent;
}

static void
ctk_tree_model_css_node_finalize (GObject *object)
{
  CtkTreeModelCssNode *model = CTK_TREE_MODEL_CSS_NODE (object);
  CtkTreeModelCssNodePrivate *priv = model->priv;

  if (priv->root)
    {
      ctk_tree_model_css_node_disconnect_node (model, priv->root, FALSE, NULL, NULL);
      priv->root = NULL;
    }

  G_OBJECT_CLASS (ctk_tree_model_css_node_parent_class)->finalize (object);
}

static void
ctk_tree_model_css_node_class_init (CtkTreeModelCssNodeClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = ctk_tree_model_css_node_finalize;
}

static void
ctk_tree_model_css_node_init (CtkTreeModelCssNode *nodemodel)
{
  nodemodel->priv = ctk_tree_model_css_node_get_instance_private (nodemodel);
}

CtkTreeModel *
ctk_tree_model_css_node_new (CtkTreeModelCssNodeGetFunc get_func,
                             gint                       n_columns,
			     ...)
{
  CtkTreeModel *result;
  va_list args;
  GType *types;
  gint i;

  g_return_val_if_fail (get_func != NULL, NULL);
  g_return_val_if_fail (n_columns > 0, NULL);

  types = g_new (GType, n_columns);
  va_start (args, n_columns);

  for (i = 0; i < n_columns; i++)
    {
      types[i] = va_arg (args, GType);
    }

  va_end (args);

  result = ctk_tree_model_css_node_newv (get_func, n_columns, types);

  g_free (types);

  return result;
}

CtkTreeModel *
ctk_tree_model_css_node_newv (CtkTreeModelCssNodeGetFunc  get_func,
                              gint                        n_columns,
			      GType                      *types)
{
  CtkTreeModelCssNode *result;
  CtkTreeModelCssNodePrivate *priv;

  g_return_val_if_fail (get_func != NULL, NULL);
  g_return_val_if_fail (n_columns > 0, NULL);
  g_return_val_if_fail (types != NULL, NULL);

  result = g_object_new (CTK_TYPE_TREE_MODEL_CSS_NODE, NULL);
  priv = result->priv;

  priv->get_func = get_func;
  priv->n_columns = n_columns;
  priv->column_types = g_memdup2 (types, sizeof (GType) * n_columns);

  return CTK_TREE_MODEL (result);
}

static void
child_added_cb (CtkCssNode          *node,
                CtkCssNode          *child,
                CtkCssNode          *previous,
                CtkTreeModelCssNode *model)
{
  ctk_tree_model_css_node_connect_node (model, child, TRUE);
}

static void
child_removed_cb (CtkCssNode          *node,
                  CtkCssNode          *child,
                  CtkCssNode          *previous,
                  CtkTreeModelCssNode *model)
{
  ctk_tree_model_css_node_disconnect_node (model, child, TRUE, node, previous);
}

static void
notify_cb (CtkCssNode          *node,
           GParamSpec          *pspec,
           CtkTreeModelCssNode *model)
{
  CtkTreeIter iter;
  CtkTreePath *path;

  ctk_tree_model_css_node_get_iter_from_node (model, &iter, node);
  path = ctk_tree_model_css_node_get_path (CTK_TREE_MODEL (model), &iter);

  ctk_tree_model_row_changed (CTK_TREE_MODEL (model), path, &iter);

  ctk_tree_path_free (path);
}

static void
style_changed_cb (CtkCssNode          *node,
                  CtkCssStyleChange   *change,
                  CtkTreeModelCssNode *model)
{
  CtkTreeIter iter;
  CtkTreePath *path;

  ctk_tree_model_css_node_get_iter_from_node (model, &iter, node);
  path = ctk_tree_model_css_node_get_path (CTK_TREE_MODEL (model), &iter);

  ctk_tree_model_row_changed (CTK_TREE_MODEL (model), path, &iter);

  ctk_tree_path_free (path);
}

static void
ctk_tree_model_css_node_connect_node (CtkTreeModelCssNode *model,
                                      CtkCssNode          *node,
                                      gboolean             emit_signal)
{
  CtkCssNode *child;

  if (CTK_IS_CSS_TRANSIENT_NODE (node))
    return;

  g_object_ref (node);

  g_signal_connect_after (node, "node-added", G_CALLBACK (child_added_cb), model);
  g_signal_connect_after (node, "node-removed", G_CALLBACK (child_removed_cb), model);
  g_signal_connect_after (node, "notify", G_CALLBACK (notify_cb), model);
  g_signal_connect_after (node, "style-changed", G_CALLBACK (style_changed_cb), model);

  for (child = ctk_css_node_get_first_child (node);
       child;
       child = ctk_css_node_get_next_sibling (child))
    {
      ctk_tree_model_css_node_connect_node (model, child, FALSE);
    }

  if (emit_signal)
    {
      CtkTreeIter iter;
      CtkTreePath *path;

      if (node != model->priv->root &&
          ctk_css_node_get_previous_sibling (node) == NULL &&
          ctk_css_node_get_next_sibling (node) == NULL)
        {
          /* We're the first child of the parent */
          ctk_tree_model_css_node_get_iter_from_node (model, &iter, ctk_css_node_get_parent (node));
          path = ctk_tree_model_css_node_get_path (CTK_TREE_MODEL (model), &iter);
          ctk_tree_model_row_has_child_toggled (CTK_TREE_MODEL (model), path, &iter);
          ctk_tree_path_free (path);
        }

      ctk_tree_model_css_node_get_iter_from_node (model, &iter, node);
      path = ctk_tree_model_css_node_get_path (CTK_TREE_MODEL (model), &iter);
      ctk_tree_model_row_inserted (CTK_TREE_MODEL (model), path, &iter);
      if (ctk_css_node_get_first_child (node))
        ctk_tree_model_row_has_child_toggled (CTK_TREE_MODEL (model), path, &iter);

      ctk_tree_path_free (path);
    }
}

static void
ctk_tree_model_css_node_disconnect_node (CtkTreeModelCssNode *model,
                                         CtkCssNode          *node,
                                         gboolean             emit_signal,
                                         CtkCssNode          *parent,
                                         CtkCssNode          *previous)
{
  CtkCssNode *child;

  if (CTK_IS_CSS_TRANSIENT_NODE (node))
    return;

  g_signal_handlers_disconnect_by_func (node, G_CALLBACK (child_added_cb), model);
  g_signal_handlers_disconnect_by_func (node, G_CALLBACK (child_removed_cb), model);
  g_signal_handlers_disconnect_by_func (node, G_CALLBACK (notify_cb), model);
  g_signal_handlers_disconnect_by_func (node, G_CALLBACK (style_changed_cb), model);

  for (child = ctk_css_node_get_first_child (node);
       child;
       child = ctk_css_node_get_next_sibling (child))
    {
      ctk_tree_model_css_node_disconnect_node (model, child, FALSE, NULL, NULL);
    }

  if (emit_signal)
    {
      CtkTreeIter iter;
      CtkTreePath *path;

      if (parent)
        {
          ctk_tree_model_css_node_get_iter_from_node (model, &iter, parent);
          path = ctk_tree_model_css_node_get_path (CTK_TREE_MODEL (model), &iter);
        }
      else
        {
          path = ctk_tree_path_new ();
        }
      if (previous)
        ctk_tree_path_append_index (path, get_node_index (previous) + 1);
      else
        ctk_tree_path_append_index (path, 0);

      ctk_tree_model_row_deleted (CTK_TREE_MODEL (model), path);

      if (parent && ctk_css_node_get_first_child (parent) == NULL)
        {
          ctk_tree_path_up (path);
          ctk_tree_model_row_has_child_toggled (CTK_TREE_MODEL (model), path, &iter);
        }

      ctk_tree_path_free (path);
    }
  
  g_object_unref (node);
}

void
ctk_tree_model_css_node_set_root_node (CtkTreeModelCssNode *model,
                                       CtkCssNode          *node)
{
  CtkTreeModelCssNodePrivate *priv;

  g_return_if_fail (CTK_IS_TREE_MODEL_CSS_NODE (model));
  g_return_if_fail (node == NULL || CTK_IS_CSS_NODE (node));

  priv = model->priv;

  if (priv->root == node)
    return;

  if (priv->root)
    {
      ctk_tree_model_css_node_disconnect_node (model, priv->root, TRUE, NULL, NULL);
      priv->root = NULL;
    }

  if (node)
    {
      priv->root = node;
      ctk_tree_model_css_node_connect_node (model, node, TRUE);
    }
}

CtkCssNode *
ctk_tree_model_css_node_get_root_node (CtkTreeModelCssNode *model)
{
  g_return_val_if_fail (CTK_IS_TREE_MODEL_CSS_NODE (model), NULL);

  return model->priv->root;
}

CtkCssNode *
ctk_tree_model_css_node_get_node_from_iter (CtkTreeModelCssNode *model,
                                            CtkTreeIter         *iter)
{
  g_return_val_if_fail (CTK_IS_TREE_MODEL_CSS_NODE (model), NULL);
  g_return_val_if_fail (iter != NULL, NULL);
  g_return_val_if_fail (iter->user_data == model, NULL);
  g_return_val_if_fail (CTK_IS_CSS_NODE (iter->user_data2), NULL);

  return iter->user_data2;
}

void
ctk_tree_model_css_node_get_iter_from_node (CtkTreeModelCssNode *model,
                                            CtkTreeIter         *iter,
                                            CtkCssNode          *node)
{
  g_return_if_fail (CTK_IS_TREE_MODEL_CSS_NODE (model));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (CTK_IS_CSS_NODE (node));

  iter->user_data = model;
  iter->user_data2 = node;
}
