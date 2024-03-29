/* CtkTreeModel ref counting tests
 * Copyright (C) 2011  Kristian Rietveld  <kris@gtk.org>
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

#include "ctktreemodelrefcount.h"
#include "treemodel.h"

/* And the tests themselves */

static void
test_list_no_reference (void)
{
  CtkTreeIter iter;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);

  assert_root_level_unreferenced (ref_model);

  g_object_unref (ref_model);
}

static void
test_list_reference_during_creation (void)
{
  CtkTreeIter iter;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);
  tree_view = ctk_tree_view_new_with_model (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);

  assert_root_level_referenced (ref_model, 1);

  ctk_widget_destroy (tree_view);

  assert_root_level_unreferenced (ref_model);

  g_object_unref (ref_model);
}

static void
test_list_reference_after_creation (void)
{
  CtkTreeIter iter;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  assert_root_level_unreferenced (ref_model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);

  tree_view = ctk_tree_view_new_with_model (model);

  assert_root_level_referenced (ref_model, 1);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);

  assert_root_level_referenced (ref_model, 1);

  ctk_widget_destroy (tree_view);

  assert_root_level_unreferenced (ref_model);

  g_object_unref (ref_model);
}

static void
test_list_reference_reordered (void)
{
  CtkTreeIter iter1, iter2, iter3, iter4, iter5;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  assert_root_level_unreferenced (ref_model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter3, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter4, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter5, NULL);

  tree_view = ctk_tree_view_new_with_model (model);

  assert_root_level_referenced (ref_model, 1);

  ctk_tree_store_move_after (CTK_TREE_STORE (model),
                             &iter1, &iter5);

  assert_root_level_referenced (ref_model, 1);

  ctk_tree_store_move_after (CTK_TREE_STORE (model),
                             &iter3, &iter4);

  assert_root_level_referenced (ref_model, 1);

  ctk_widget_destroy (tree_view);

  assert_root_level_unreferenced (ref_model);

  g_object_unref (ref_model);
}


static void
test_tree_no_reference (void)
{
  CtkTreeIter iter, child;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);

  assert_entire_model_unreferenced (ref_model);

  g_object_unref (ref_model);
}

static void
test_tree_reference_during_creation (void)
{
  CtkTreeIter iter, child;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);
  tree_view = ctk_tree_view_new_with_model (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);

  assert_root_level_referenced (ref_model, 1);
  assert_not_entire_model_referenced (ref_model, 1);
  assert_level_unreferenced (ref_model, &child);

  ctk_widget_destroy (tree_view);

  assert_entire_model_unreferenced (ref_model);

  g_object_unref (ref_model);
}

static void
test_tree_reference_after_creation (void)
{
  CtkTreeIter iter, child;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);

  assert_entire_model_unreferenced (ref_model);

  tree_view = ctk_tree_view_new_with_model (model);

  assert_root_level_referenced (ref_model, 1);
  assert_not_entire_model_referenced (ref_model, 1);
  assert_level_unreferenced (ref_model, &child);

  ctk_widget_destroy (tree_view);

  assert_entire_model_unreferenced (ref_model);

  g_object_unref (ref_model);
}

static void
test_tree_reference_reordered (void)
{
  CtkTreeIter parent;
  CtkTreeIter iter1, iter2, iter3, iter4, iter5;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  assert_root_level_unreferenced (ref_model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &parent, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter1, &parent);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter2, &parent);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter3, &parent);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter4, &parent);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter5, &parent);

  tree_view = ctk_tree_view_new_with_model (model);
  ctk_tree_view_expand_all (CTK_TREE_VIEW (tree_view));

  assert_entire_model_referenced (ref_model, 1);

  ctk_tree_store_move_after (CTK_TREE_STORE (model),
                             &iter1, &iter5);

  assert_entire_model_referenced (ref_model, 1);

  ctk_tree_store_move_after (CTK_TREE_STORE (model),
                             &iter3, &iter4);

  assert_entire_model_referenced (ref_model, 1);

  ctk_widget_destroy (tree_view);

  assert_entire_model_unreferenced (ref_model);

  g_object_unref (ref_model);
}

static void
test_tree_reference_expand_all (void)
{
  CtkTreeIter iter, child;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);

  assert_entire_model_unreferenced (ref_model);

  tree_view = ctk_tree_view_new_with_model (model);

  assert_root_level_referenced (ref_model, 1);
  assert_not_entire_model_referenced (ref_model, 1);
  assert_level_unreferenced (ref_model, &child);

  ctk_tree_view_expand_all (CTK_TREE_VIEW (tree_view));

  assert_entire_model_referenced (ref_model, 1);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);

  assert_root_level_referenced (ref_model, 1);
  assert_not_entire_model_referenced (ref_model, 1);
  assert_level_unreferenced (ref_model, &child);

  ctk_widget_destroy (tree_view);

  assert_entire_model_unreferenced (ref_model);

  g_object_unref (ref_model);
}

static void
test_tree_reference_collapse_all (void)
{
  CtkTreeIter iter, child;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &iter);

  assert_entire_model_unreferenced (ref_model);

  tree_view = ctk_tree_view_new_with_model (model);
  ctk_tree_view_expand_all (CTK_TREE_VIEW (tree_view));

  assert_entire_model_referenced (ref_model, 1);

  ctk_tree_view_collapse_all (CTK_TREE_VIEW (tree_view));

  assert_root_level_referenced (ref_model, 1);
  assert_not_entire_model_referenced (ref_model, 1);
  assert_level_unreferenced (ref_model, &child);

  ctk_widget_destroy (tree_view);

  assert_entire_model_unreferenced (ref_model);

  g_object_unref (ref_model);
}

static void
test_tree_reference_expand_collapse (void)
{
  CtkTreeIter parent1, parent2, child;
  CtkTreePath *path1, *path2;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);
  tree_view = ctk_tree_view_new_with_model (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &parent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &parent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &parent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &parent2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child, &parent2);

  path1 = ctk_tree_model_get_path (model, &parent1);
  path2 = ctk_tree_model_get_path (model, &parent2);

  assert_level_unreferenced (ref_model, &parent1);
  assert_level_unreferenced (ref_model, &parent2);

  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path1, FALSE);

  assert_level_referenced (ref_model, 1, &parent1);
  assert_level_unreferenced (ref_model, &parent2);

  ctk_tree_view_collapse_row (CTK_TREE_VIEW (tree_view), path1);

  assert_level_unreferenced (ref_model, &parent1);
  assert_level_unreferenced (ref_model, &parent2);

  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path2, FALSE);

  assert_level_unreferenced (ref_model, &parent1);
  assert_level_referenced (ref_model, 1, &parent2);

  ctk_tree_view_collapse_row (CTK_TREE_VIEW (tree_view), path2);

  assert_level_unreferenced (ref_model, &parent1);
  assert_level_unreferenced (ref_model, &parent2);

  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path2, FALSE);

  assert_level_unreferenced (ref_model, &parent1);
  assert_level_referenced (ref_model, 1, &parent2);

  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path1, FALSE);

  assert_level_referenced (ref_model, 1, &parent1);
  assert_level_referenced (ref_model, 1, &parent2);

  ctk_tree_path_free (path1);
  ctk_tree_path_free (path2);

  ctk_widget_destroy (tree_view);
  g_object_unref (ref_model);
}

static void
test_row_reference_list (void)
{
  CtkTreeIter iter0, iter1, iter2;
  CtkTreePath *path;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeRowReference *row_ref;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter0, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter2, NULL);

  assert_root_level_unreferenced (ref_model);

  /* create and remove a row ref and check reference counts */
  path = ctk_tree_path_new_from_indices (1, -1);
  row_ref = ctk_tree_row_reference_new (model, path);

  assert_node_ref_count (ref_model, &iter0, 0);
  assert_node_ref_count (ref_model, &iter1, 1);
  assert_node_ref_count (ref_model, &iter2, 0);

  ctk_tree_row_reference_free (row_ref);

  assert_root_level_unreferenced (ref_model);

  /* the same, but then also with a tree view monitoring the model */
  tree_view = ctk_tree_view_new_with_model (model);

  assert_root_level_referenced (ref_model, 1);

  row_ref = ctk_tree_row_reference_new (model, path);

  assert_node_ref_count (ref_model, &iter0, 1);
  assert_node_ref_count (ref_model, &iter1, 2);
  assert_node_ref_count (ref_model, &iter2, 1);

  ctk_widget_destroy (tree_view);

  assert_node_ref_count (ref_model, &iter0, 0);
  assert_node_ref_count (ref_model, &iter1, 1);
  assert_node_ref_count (ref_model, &iter2, 0);

  ctk_tree_row_reference_free (row_ref);

  assert_root_level_unreferenced (ref_model);

  ctk_tree_path_free (path);

  g_object_unref (ref_model);
}

static void
test_row_reference_list_remove (void)
{
  CtkTreeIter iter0, iter1, iter2;
  CtkTreePath *path;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeRowReference *row_ref;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter0, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter2, NULL);

  assert_root_level_unreferenced (ref_model);

  /* test creating the row reference and then removing the node */
  path = ctk_tree_path_new_from_indices (1, -1);
  row_ref = ctk_tree_row_reference_new (model, path);

  assert_node_ref_count (ref_model, &iter0, 0);
  assert_node_ref_count (ref_model, &iter1, 1);
  assert_node_ref_count (ref_model, &iter2, 0);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &iter1);

  assert_root_level_unreferenced (ref_model);

  ctk_tree_row_reference_free (row_ref);

  assert_root_level_unreferenced (ref_model);

  /* test creating a row ref, removing another node and then removing
   * the row ref node.
   */
  row_ref = ctk_tree_row_reference_new (model, path);

  assert_node_ref_count (ref_model, &iter0, 0);
  assert_node_ref_count (ref_model, &iter2, 1);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &iter0);

  assert_root_level_referenced (ref_model, 1);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &iter2);

  g_assert (!ctk_tree_model_get_iter_first (model, &iter0));

  ctk_tree_row_reference_free (row_ref);

  ctk_tree_path_free (path);

  g_object_unref (ref_model);
}

static void
test_row_reference_tree (void)
{
  CtkTreeIter iter0, iter1, iter2;
  CtkTreeIter child0, child1, child2;
  CtkTreeIter grandchild0, grandchild1, grandchild2;
  CtkTreePath *path;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeRowReference *row_ref, *row_ref1;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter0, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child0, &iter0);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandchild0, &child0);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child1, &iter1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandchild1, &child1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child2, &iter2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandchild2, &child2);

  assert_entire_model_unreferenced (ref_model);

  /* create and remove a row ref and check reference counts */
  path = ctk_tree_path_new_from_indices (1, 0, 0, -1);
  row_ref = ctk_tree_row_reference_new (model, path);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &iter0, 0);
  assert_node_ref_count (ref_model, &child0, 0);
  assert_node_ref_count (ref_model, &grandchild0, 0);
  assert_node_ref_count (ref_model, &iter1, 1);
  assert_node_ref_count (ref_model, &child1, 1);
  assert_node_ref_count (ref_model, &grandchild1, 1);
  assert_node_ref_count (ref_model, &iter2, 0);
  assert_node_ref_count (ref_model, &child2, 0);
  assert_node_ref_count (ref_model, &grandchild2, 0);

  ctk_tree_row_reference_free (row_ref);

  assert_entire_model_unreferenced (ref_model);

  /* again, with path 1:1 */
  path = ctk_tree_path_new_from_indices (1, 0, -1);
  row_ref = ctk_tree_row_reference_new (model, path);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &iter0, 0);
  assert_node_ref_count (ref_model, &child0, 0);
  assert_node_ref_count (ref_model, &grandchild0, 0);
  assert_node_ref_count (ref_model, &iter1, 1);
  assert_node_ref_count (ref_model, &child1, 1);
  assert_node_ref_count (ref_model, &grandchild1, 0);
  assert_node_ref_count (ref_model, &iter2, 0);
  assert_node_ref_count (ref_model, &child2, 0);
  assert_node_ref_count (ref_model, &grandchild2, 0);

  ctk_tree_row_reference_free (row_ref);

  assert_entire_model_unreferenced (ref_model);

  /* both row refs existent at once and also with a tree view monitoring
   * the model
   */
  tree_view = ctk_tree_view_new_with_model (model);

  assert_root_level_referenced (ref_model, 1);

  path = ctk_tree_path_new_from_indices (1, 0, 0, -1);
  row_ref = ctk_tree_row_reference_new (model, path);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &iter0, 1);
  assert_node_ref_count (ref_model, &child0, 0);
  assert_node_ref_count (ref_model, &grandchild0, 0);
  assert_node_ref_count (ref_model, &iter1, 2);
  assert_node_ref_count (ref_model, &child1, 1);
  assert_node_ref_count (ref_model, &grandchild1, 1);
  assert_node_ref_count (ref_model, &iter2, 1);
  assert_node_ref_count (ref_model, &child2, 0);
  assert_node_ref_count (ref_model, &grandchild2, 0);

  path = ctk_tree_path_new_from_indices (1, 0, -1);
  row_ref1 = ctk_tree_row_reference_new (model, path);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &iter0, 1);
  assert_node_ref_count (ref_model, &child0, 0);
  assert_node_ref_count (ref_model, &grandchild0, 0);
  assert_node_ref_count (ref_model, &iter1, 3);
  assert_node_ref_count (ref_model, &child1, 2);
  assert_node_ref_count (ref_model, &grandchild1, 1);
  assert_node_ref_count (ref_model, &iter2, 1);
  assert_node_ref_count (ref_model, &child2, 0);
  assert_node_ref_count (ref_model, &grandchild2, 0);

  ctk_tree_row_reference_free (row_ref);

  assert_node_ref_count (ref_model, &iter0, 1);
  assert_node_ref_count (ref_model, &child0, 0);
  assert_node_ref_count (ref_model, &grandchild0, 0);
  assert_node_ref_count (ref_model, &iter1, 2);
  assert_node_ref_count (ref_model, &child1, 1);
  assert_node_ref_count (ref_model, &grandchild1, 0);
  assert_node_ref_count (ref_model, &iter2, 1);
  assert_node_ref_count (ref_model, &child2, 0);
  assert_node_ref_count (ref_model, &grandchild2, 0);

  ctk_widget_destroy (tree_view);

  assert_node_ref_count (ref_model, &iter0, 0);
  assert_node_ref_count (ref_model, &child0, 0);
  assert_node_ref_count (ref_model, &grandchild0, 0);
  assert_node_ref_count (ref_model, &iter1, 1);
  assert_node_ref_count (ref_model, &child1, 1);
  assert_node_ref_count (ref_model, &grandchild1, 0);
  assert_node_ref_count (ref_model, &iter2, 0);
  assert_node_ref_count (ref_model, &child2, 0);
  assert_node_ref_count (ref_model, &grandchild2, 0);

  ctk_tree_row_reference_free (row_ref1);

  assert_root_level_unreferenced (ref_model);

  g_object_unref (ref_model);
}

static void
test_row_reference_tree_remove (void)
{
  CtkTreeIter iter0, iter1, iter2;
  CtkTreeIter child0, child1, child2;
  CtkTreeIter grandchild0, grandchild1, grandchild2;
  CtkTreePath *path;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeRowReference *row_ref, *row_ref1, *row_ref2;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter0, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child0, &iter0);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandchild0, &child0);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child1, &iter1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandchild1, &child1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child2, &iter2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandchild2, &child2);

  assert_entire_model_unreferenced (ref_model);

  path = ctk_tree_path_new_from_indices (1, 0, 0, -1);
  row_ref = ctk_tree_row_reference_new (model, path);
  ctk_tree_path_free (path);

  path = ctk_tree_path_new_from_indices (2, 0, -1);
  row_ref1 = ctk_tree_row_reference_new (model, path);
  ctk_tree_path_free (path);

  path = ctk_tree_path_new_from_indices (2, -1);
  row_ref2 = ctk_tree_row_reference_new (model, path);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &iter0, 0);
  assert_node_ref_count (ref_model, &child0, 0);
  assert_node_ref_count (ref_model, &grandchild0, 0);
  assert_node_ref_count (ref_model, &iter1, 1);
  assert_node_ref_count (ref_model, &child1, 1);
  assert_node_ref_count (ref_model, &grandchild1, 1);
  assert_node_ref_count (ref_model, &iter2, 2);
  assert_node_ref_count (ref_model, &child2, 1);
  assert_node_ref_count (ref_model, &grandchild2, 0);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &grandchild1);

  assert_node_ref_count (ref_model, &iter0, 0);
  assert_node_ref_count (ref_model, &child0, 0);
  assert_node_ref_count (ref_model, &grandchild0, 0);
  assert_node_ref_count (ref_model, &iter1, 0);
  assert_node_ref_count (ref_model, &child1, 0);
  assert_node_ref_count (ref_model, &iter2, 2);
  assert_node_ref_count (ref_model, &child2, 1);
  assert_node_ref_count (ref_model, &grandchild2, 0);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &child2);

  assert_node_ref_count (ref_model, &iter0, 0);
  assert_node_ref_count (ref_model, &child0, 0);
  assert_node_ref_count (ref_model, &grandchild0, 0);
  assert_node_ref_count (ref_model, &iter1, 0);
  assert_node_ref_count (ref_model, &child1, 0);
  assert_node_ref_count (ref_model, &iter2, 1);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &iter2);

  assert_entire_model_unreferenced (ref_model);

  ctk_tree_row_reference_free (row_ref);
  ctk_tree_row_reference_free (row_ref1);
  ctk_tree_row_reference_free (row_ref2);

  g_object_unref (ref_model);
}

static void
test_row_reference_tree_remove_ancestor (void)
{
  CtkTreeIter iter0, iter1, iter2;
  CtkTreeIter child0, child1, child2;
  CtkTreeIter grandchild0, grandchild1, grandchild2;
  CtkTreePath *path;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeRowReference *row_ref, *row_ref1;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter0, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child0, &iter0);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandchild0, &child0);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child1, &iter1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandchild1, &child1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child2, &iter2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandchild2, &child2);

  assert_entire_model_unreferenced (ref_model);

  path = ctk_tree_path_new_from_indices (1, 0, 0, -1);
  row_ref = ctk_tree_row_reference_new (model, path);
  ctk_tree_path_free (path);

  path = ctk_tree_path_new_from_indices (2, 0, -1);
  row_ref1 = ctk_tree_row_reference_new (model, path);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &iter0, 0);
  assert_node_ref_count (ref_model, &child0, 0);
  assert_node_ref_count (ref_model, &grandchild0, 0);
  assert_node_ref_count (ref_model, &iter1, 1);
  assert_node_ref_count (ref_model, &child1, 1);
  assert_node_ref_count (ref_model, &grandchild1, 1);
  assert_node_ref_count (ref_model, &iter2, 1);
  assert_node_ref_count (ref_model, &child2, 1);
  assert_node_ref_count (ref_model, &grandchild2, 0);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &child1);

  assert_node_ref_count (ref_model, &iter0, 0);
  assert_node_ref_count (ref_model, &child0, 0);
  assert_node_ref_count (ref_model, &grandchild0, 0);
  assert_node_ref_count (ref_model, &iter1, 0);
  assert_node_ref_count (ref_model, &iter2, 1);
  assert_node_ref_count (ref_model, &child2, 1);
  assert_node_ref_count (ref_model, &grandchild2, 0);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &iter2);

  assert_entire_model_unreferenced (ref_model);

  ctk_tree_row_reference_free (row_ref);
  ctk_tree_row_reference_free (row_ref1);

  g_object_unref (ref_model);
}

static void
test_row_reference_tree_expand (void)
{
  CtkTreeIter iter0, iter1, iter2;
  CtkTreeIter child0, child1, child2;
  CtkTreeIter grandchild0, grandchild1, grandchild2;
  CtkTreePath *path;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeRowReference *row_ref, *row_ref1, *row_ref2;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);
  tree_view = ctk_tree_view_new_with_model (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter0, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child0, &iter0);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandchild0, &child0);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child1, &iter1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandchild1, &child1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &child2, &iter2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandchild2, &child2);

  assert_root_level_referenced (ref_model, 1);

  ctk_tree_view_expand_all (CTK_TREE_VIEW (tree_view));

  assert_entire_model_referenced (ref_model, 1);

  path = ctk_tree_path_new_from_indices (1, 0, 0, -1);
  row_ref = ctk_tree_row_reference_new (model, path);
  ctk_tree_path_free (path);

  path = ctk_tree_path_new_from_indices (2, 0, -1);
  row_ref1 = ctk_tree_row_reference_new (model, path);
  ctk_tree_path_free (path);

  path = ctk_tree_path_new_from_indices (2, -1);
  row_ref2 = ctk_tree_row_reference_new (model, path);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &iter0, 1);
  assert_node_ref_count (ref_model, &child0, 1);
  assert_node_ref_count (ref_model, &grandchild0, 1);
  assert_node_ref_count (ref_model, &iter1, 2);
  assert_node_ref_count (ref_model, &child1, 2);
  assert_node_ref_count (ref_model, &grandchild1, 2);
  assert_node_ref_count (ref_model, &iter2, 3);
  assert_node_ref_count (ref_model, &child2, 2);
  assert_node_ref_count (ref_model, &grandchild2, 1);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &grandchild1);

  assert_node_ref_count (ref_model, &iter0, 1);
  assert_node_ref_count (ref_model, &child0, 1);
  assert_node_ref_count (ref_model, &grandchild0, 1);
  assert_node_ref_count (ref_model, &iter1, 1);
  assert_node_ref_count (ref_model, &child1, 1);
  assert_node_ref_count (ref_model, &iter2, 3);
  assert_node_ref_count (ref_model, &child2, 2);
  assert_node_ref_count (ref_model, &grandchild2, 1);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &child2);

  assert_node_ref_count (ref_model, &iter0, 1);
  assert_node_ref_count (ref_model, &child0, 1);
  assert_node_ref_count (ref_model, &grandchild0, 1);
  assert_node_ref_count (ref_model, &iter1, 1);
  assert_node_ref_count (ref_model, &child1, 1);
  assert_node_ref_count (ref_model, &iter2, 2);

  ctk_tree_view_collapse_all (CTK_TREE_VIEW (tree_view));

  assert_node_ref_count (ref_model, &iter0, 1);
  assert_node_ref_count (ref_model, &child0, 0);
  assert_node_ref_count (ref_model, &grandchild0, 0);
  assert_node_ref_count (ref_model, &iter1, 1);
  assert_node_ref_count (ref_model, &child1, 0);
  assert_node_ref_count (ref_model, &iter2, 2);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &iter2);

  assert_node_ref_count (ref_model, &iter0, 1);
  assert_node_ref_count (ref_model, &child0, 0);
  assert_node_ref_count (ref_model, &grandchild0, 0);
  assert_node_ref_count (ref_model, &iter1, 1);
  assert_node_ref_count (ref_model, &child1, 0);

  ctk_tree_row_reference_free (row_ref);
  ctk_tree_row_reference_free (row_ref1);
  ctk_tree_row_reference_free (row_ref2);

  ctk_widget_destroy (tree_view);
  g_object_unref (ref_model);
}

void
register_model_ref_count_tests (void)
{
  /* lists (though based on CtkTreeStore) */
  g_test_add_func ("/TreeModel/ref-count/list/no-reference",
                   test_list_no_reference);
  g_test_add_func ("/TreeModel/ref-count/list/reference-during-creation",
                   test_list_reference_during_creation);
  g_test_add_func ("/TreeModel/ref-count/list/reference-after-creation",
                   test_list_reference_after_creation);
  g_test_add_func ("/TreeModel/ref-count/list/reference-reordered",
                   test_list_reference_reordered);

  /* trees */
  g_test_add_func ("/TreeModel/ref-count/tree/no-reference",
                   test_tree_no_reference);
  g_test_add_func ("/TreeModel/ref-count/tree/reference-during-creation",
                   test_tree_reference_during_creation);
  g_test_add_func ("/TreeModel/ref-count/tree/reference-after-creation",
                   test_tree_reference_after_creation);
  g_test_add_func ("/TreeModel/ref-count/tree/expand-all",
                   test_tree_reference_expand_all);
  g_test_add_func ("/TreeModel/ref-count/tree/collapse-all",
                   test_tree_reference_collapse_all);
  g_test_add_func ("/TreeModel/ref-count/tree/expand-collapse",
                   test_tree_reference_expand_collapse);
  g_test_add_func ("/TreeModel/ref-count/tree/reference-reordered",
                   test_tree_reference_reordered);

  /* row references */
  g_test_add_func ("/TreeModel/ref-count/row-reference/list",
                   test_row_reference_list);
  g_test_add_func ("/TreeModel/ref-count/row-reference/list-remove",
                   test_row_reference_list_remove);
  g_test_add_func ("/TreeModel/ref-count/row-reference/tree",
                   test_row_reference_tree);
  g_test_add_func ("/TreeModel/ref-count/row-reference/tree-remove",
                   test_row_reference_tree_remove);
  g_test_add_func ("/TreeModel/ref-count/row-reference/tree-remove-ancestor",
                   test_row_reference_tree_remove_ancestor);
  g_test_add_func ("/TreeModel/ref-count/row-reference/tree-expand",
                   test_row_reference_tree_expand);
}
