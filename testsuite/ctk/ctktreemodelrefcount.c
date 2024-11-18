/* ctktreemodelrefcount.c
 * Copyright (C) 2011  Kristian Rietveld <kris@gtk.org>
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
#include "ctktreemodelrefcount.h"


/* The purpose of this CtkTreeModel is to keep record of the reference count
 * of each node.  The reference count does not effect the functioning of
 * the model in any way.  Because this model is a subclass of CtkTreeStore,
 * the CtkTreeStore API should be used to add to and remove nodes from
 * this model.  We depend on the iter format of CtkTreeStore, which means
 * that this model needs to be revised in case the iter format of
 * CtkTreeStore is modified.  Currently, we make use of the fact that
 * the value stored in the user_data field is unique for each node.
 */

struct _CtkTreeModelRefCountPrivate
{
  GHashTable *node_hash;
};

typedef struct
{
  int ref_count;
}
NodeInfo;


static void      ctk_tree_model_ref_count_tree_model_init (CtkTreeModelIface *iface);
static void      ctk_tree_model_ref_count_finalize        (GObject           *object);

static NodeInfo *node_info_new                            (void);
static void      node_info_free                           (NodeInfo          *info);

/* CtkTreeModel interface */
static void      ctk_tree_model_ref_count_ref_node        (CtkTreeModel      *model,
                                                           CtkTreeIter       *iter);
static void      ctk_tree_model_ref_count_unref_node      (CtkTreeModel      *model,
                                                           CtkTreeIter       *iter);


G_DEFINE_TYPE_WITH_CODE (CtkTreeModelRefCount, ctk_tree_model_ref_count, CTK_TYPE_TREE_STORE,
                         G_ADD_PRIVATE (CtkTreeModelRefCount)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_TREE_MODEL,
                                                ctk_tree_model_ref_count_tree_model_init))

static void
row_removed (CtkTreeModelRefCount *ref_model,
             CtkTreePath          *path G_GNUC_UNUSED)
{
  GHashTableIter iter;
  CtkTreeIter tree_iter;

  if (!ctk_tree_model_get_iter_first (CTK_TREE_MODEL (ref_model), &tree_iter))
    {
      g_hash_table_remove_all (ref_model->priv->node_hash);
      return;
    }

  g_hash_table_iter_init (&iter, ref_model->priv->node_hash);

  while (g_hash_table_iter_next (&iter, &tree_iter.user_data, NULL))
    {
      if (!ctk_tree_store_iter_is_valid (CTK_TREE_STORE (ref_model), &tree_iter))
        g_hash_table_iter_remove (&iter);
    }
}

static void
ctk_tree_model_ref_count_init (CtkTreeModelRefCount *ref_model)
{
  ref_model->priv = ctk_tree_model_ref_count_get_instance_private (ref_model); 

  ref_model->priv->node_hash = g_hash_table_new_full (g_direct_hash,
                                                      g_direct_equal,
                                                      NULL,
                                                      (GDestroyNotify)node_info_free);

  g_signal_connect (ref_model, "row-deleted", G_CALLBACK (row_removed), NULL);
}

static void
ctk_tree_model_ref_count_class_init (CtkTreeModelRefCountClass *ref_model_class)
{
  G_OBJECT_CLASS (ref_model_class)->finalize = ctk_tree_model_ref_count_finalize;
}

static void
ctk_tree_model_ref_count_tree_model_init (CtkTreeModelIface *iface)
{
  iface->ref_node = ctk_tree_model_ref_count_ref_node;
  iface->unref_node = ctk_tree_model_ref_count_unref_node;
}

static void
ctk_tree_model_ref_count_finalize (GObject *object)
{
  CtkTreeModelRefCount *ref_model = CTK_TREE_MODEL_REF_COUNT (object);

  if (ref_model->priv->node_hash)
    {
      g_hash_table_destroy (ref_model->priv->node_hash);
      ref_model->priv->node_hash = NULL;
    }

  G_OBJECT_CLASS (ctk_tree_model_ref_count_parent_class)->finalize (object);
}


static NodeInfo *
node_info_new (void)
{
  NodeInfo *info = g_slice_new (NodeInfo);
  info->ref_count = 0;

  return info;
}

static void
node_info_free (NodeInfo *info)
{
  g_slice_free (NodeInfo, info);
}

static void
ctk_tree_model_ref_count_ref_node (CtkTreeModel *model,
                                   CtkTreeIter  *iter)
{
  NodeInfo *info;
  CtkTreeModelRefCount *ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  info = g_hash_table_lookup (ref_model->priv->node_hash, iter->user_data);
  if (!info)
    {
      info = node_info_new ();

      g_hash_table_insert (ref_model->priv->node_hash, iter->user_data, info);
    }

  info->ref_count++;
}

static void
ctk_tree_model_ref_count_unref_node (CtkTreeModel *model,
                                     CtkTreeIter  *iter)
{
  NodeInfo *info;
  CtkTreeModelRefCount *ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  info = g_hash_table_lookup (ref_model->priv->node_hash, iter->user_data);
  g_assert (info != NULL);
  g_assert (info->ref_count > 0);

  info->ref_count--;
}


CtkTreeModel *
ctk_tree_model_ref_count_new (void)
{
  CtkTreeModel *retval;

  retval = g_object_new (ctk_tree_model_ref_count_get_type (), NULL);

  return retval;
}

static void
dump_iter (CtkTreeModelRefCount *ref_model,
           CtkTreeIter          *iter)
{
  gchar *path_str;
  NodeInfo *info;
  CtkTreePath *path;

  path = ctk_tree_model_get_path (CTK_TREE_MODEL (ref_model), iter);
  path_str = ctk_tree_path_to_string (path);
  ctk_tree_path_free (path);

  info = g_hash_table_lookup (ref_model->priv->node_hash, iter->user_data);
  if (!info)
    g_print ("%-16s ref_count=0\n", path_str);
  else
    g_print ("%-16s ref_count=%d\n", path_str, info->ref_count);

  g_free (path_str);
}

static void
ctk_tree_model_ref_count_dump_recurse (CtkTreeModelRefCount *ref_model,
                                       CtkTreeIter          *iter)
{
  do
    {
      CtkTreeIter child;

      dump_iter (ref_model, iter);

      if (ctk_tree_model_iter_children (CTK_TREE_MODEL (ref_model),
                                        &child, iter))
        ctk_tree_model_ref_count_dump_recurse (ref_model, &child);
    }
  while (ctk_tree_model_iter_next (CTK_TREE_MODEL (ref_model), iter));
}

void
ctk_tree_model_ref_count_dump (CtkTreeModelRefCount *ref_model)
{
  CtkTreeIter iter;

  if (!ctk_tree_model_get_iter_first (CTK_TREE_MODEL (ref_model), &iter))
    return;

  ctk_tree_model_ref_count_dump_recurse (ref_model, &iter);
}

static gboolean
check_iter (CtkTreeModelRefCount *ref_model,
            CtkTreeIter          *iter,
            gint                  expected_ref_count,
            gboolean              may_assert)
{
  NodeInfo *info;

  if (may_assert)
    g_assert (ctk_tree_store_iter_is_valid (CTK_TREE_STORE (ref_model), iter));

  info = g_hash_table_lookup (ref_model->priv->node_hash, iter->user_data);
  if (!info)
    {
      if (expected_ref_count == 0)
        return TRUE;
      else
        {
          if (may_assert)
            g_error ("Expected ref count %d, but node has never been referenced.", expected_ref_count);
          return FALSE;
        }
    }

  if (may_assert)
    {
      if (expected_ref_count == 0)
        g_assert_cmpint (expected_ref_count, ==, info->ref_count);
      else
        g_assert_cmpint (expected_ref_count, <=, info->ref_count);
    }

  return expected_ref_count == info->ref_count;
}

gboolean
ctk_tree_model_ref_count_check_level (CtkTreeModelRefCount *ref_model,
                                      CtkTreeIter          *parent,
                                      gint                  expected_ref_count,
                                      gboolean              recurse,
                                      gboolean              may_assert)
{
  CtkTreeIter iter;

  if (!ctk_tree_model_iter_children (CTK_TREE_MODEL (ref_model),
                                     &iter, parent))
    return TRUE;

  do
    {
      if (!check_iter (ref_model, &iter, expected_ref_count, may_assert))
        return FALSE;

      if (recurse &&
          ctk_tree_model_iter_has_child (CTK_TREE_MODEL (ref_model), &iter))
        {
          if (!ctk_tree_model_ref_count_check_level (ref_model, &iter,
                                                     expected_ref_count,
                                                     recurse, may_assert))
            return FALSE;
        }
    }
  while (ctk_tree_model_iter_next (CTK_TREE_MODEL (ref_model), &iter));

  return TRUE;
}

gboolean
ctk_tree_model_ref_count_check_node (CtkTreeModelRefCount *ref_model,
                                     CtkTreeIter          *iter,
                                     gint                  expected_ref_count,
                                     gboolean              may_assert)
{
  return check_iter (ref_model, iter, expected_ref_count, may_assert);
}
