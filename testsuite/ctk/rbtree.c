/* CtkRBTree tests.
 *
 * Copyright (C) 2011, Red Hat, Inc.
 * Authors: Benjamin Otte <otte@gnome.org>
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

#include <locale.h>

#include "../../ctk/ctkrbtree.h"

/* _ctk_rbtree_test */

static guint
get_total_count (CtkRBNode *node)
{
  guint child_total = 0;

  child_total += (guint) node->left->total_count;
  child_total += (guint) node->right->total_count;

  if (node->children)
    child_total += (guint) node->children->root->total_count;

  return child_total + 1;
}

static guint
count_total (CtkRBTree *tree,
             CtkRBNode *node)
{
  guint res;
  
  if (_ctk_rbtree_is_nil (node))
    return 0;
  
  res =
    count_total (tree, node->left) +
    count_total (tree, node->right) +
    (guint)1 +
    (node->children ? count_total (node->children, node->children->root) : 0);

  if (res != node->total_count)
    g_print ("total count incorrect for node\n");

  if (get_total_count (node) != node->total_count)
    g_error ("Node has incorrect total count %u, should be %u", node->total_count, get_total_count (node));
  
  return res;
}

static gint
_count_nodes (CtkRBTree *tree,
              CtkRBNode *node)
{
  gint res;
  if (_ctk_rbtree_is_nil (node))
    return 0;

  g_assert (node->left);
  g_assert (node->right);

  res = (_count_nodes (tree, node->left) +
         _count_nodes (tree, node->right) + 1);

  if (res != node->count)
    g_print ("Tree failed\n");
  return res;
}

static void
_ctk_rbtree_test_height (CtkRBTree *tree,
                         CtkRBNode *node)
{
  gint computed_offset = 0;

  /* This whole test is sort of a useless truism. */
  
  if (!_ctk_rbtree_is_nil (node->left))
    computed_offset += node->left->offset;

  if (!_ctk_rbtree_is_nil (node->right))
    computed_offset += node->right->offset;

  if (node->children && !_ctk_rbtree_is_nil (node->children->root))
    computed_offset += node->children->root->offset;

  if (CTK_RBNODE_GET_HEIGHT (node) + computed_offset != node->offset)
    g_error ("node has broken offset");

  if (!_ctk_rbtree_is_nil (node->left))
    _ctk_rbtree_test_height (tree, node->left);

  if (!_ctk_rbtree_is_nil (node->right))
    _ctk_rbtree_test_height (tree, node->right);

  if (node->children && !_ctk_rbtree_is_nil (node->children->root))
    _ctk_rbtree_test_height (node->children, node->children->root);
}

static void
_ctk_rbtree_test_dirty (CtkRBTree *tree,
			CtkRBNode *node,
			 gint      expected_dirtyness)
{

  if (expected_dirtyness)
    {
      g_assert (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_COLUMN_INVALID) ||
		CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID) ||
		CTK_RBNODE_FLAG_SET (node->left, CTK_RBNODE_DESCENDANTS_INVALID) ||
		CTK_RBNODE_FLAG_SET (node->right, CTK_RBNODE_DESCENDANTS_INVALID) ||
		(node->children && CTK_RBNODE_FLAG_SET (node->children->root, CTK_RBNODE_DESCENDANTS_INVALID)));
    }
  else
    {
      g_assert (! CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_COLUMN_INVALID) &&
		! CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID));
      if (!_ctk_rbtree_is_nil (node->left))
	g_assert (! CTK_RBNODE_FLAG_SET (node->left, CTK_RBNODE_DESCENDANTS_INVALID));
      if (!_ctk_rbtree_is_nil (node->right))
	g_assert (! CTK_RBNODE_FLAG_SET (node->right, CTK_RBNODE_DESCENDANTS_INVALID));
      if (node->children != NULL)
	g_assert (! CTK_RBNODE_FLAG_SET (node->children->root, CTK_RBNODE_DESCENDANTS_INVALID));
    }

  if (!_ctk_rbtree_is_nil (node->left))
    _ctk_rbtree_test_dirty (tree, node->left, CTK_RBNODE_FLAG_SET (node->left, CTK_RBNODE_DESCENDANTS_INVALID));
  if (!_ctk_rbtree_is_nil (node->right))
    _ctk_rbtree_test_dirty (tree, node->right, CTK_RBNODE_FLAG_SET (node->right, CTK_RBNODE_DESCENDANTS_INVALID));
  if (node->children != NULL && !_ctk_rbtree_is_nil (node->children->root))
    _ctk_rbtree_test_dirty (node->children, node->children->root, CTK_RBNODE_FLAG_SET (node->children->root, CTK_RBNODE_DESCENDANTS_INVALID));
}

static void _ctk_rbtree_test_structure (CtkRBTree *tree);

static guint
_ctk_rbtree_test_structure_helper (CtkRBTree *tree,
				   CtkRBNode *node)
{
  guint left_blacks, right_blacks;

  g_assert (!_ctk_rbtree_is_nil (node));

  g_assert (node->left != NULL);
  g_assert (node->right != NULL);
  g_assert (node->parent != NULL);

  if (!_ctk_rbtree_is_nil (node->left))
    {
      g_assert (node->left->parent == node);
      left_blacks = _ctk_rbtree_test_structure_helper (tree, node->left);
    }
  else
    left_blacks = 0;

  if (!_ctk_rbtree_is_nil (node->right))
    {
      g_assert (node->right->parent == node);
      right_blacks = _ctk_rbtree_test_structure_helper (tree, node->right);
    }
  else
    right_blacks = 0;

  if (node->children != NULL)
    {
      g_assert (node->children->parent_tree == tree);
      g_assert (node->children->parent_node == node);

      _ctk_rbtree_test_structure (node->children);
    }

  g_assert (left_blacks == right_blacks);

  return left_blacks + (CTK_RBNODE_GET_COLOR (node) == CTK_RBNODE_BLACK ? 1 : 0);
}

static void
_ctk_rbtree_test_structure (CtkRBTree *tree)
{
  g_assert (tree->root);
  if (_ctk_rbtree_is_nil (tree->root))
    return;

  g_assert (_ctk_rbtree_is_nil (tree->root->parent));
  _ctk_rbtree_test_structure_helper (tree, tree->root);
}

static void
_ctk_rbtree_test (CtkRBTree *tree)
{
  CtkRBTree *tmp_tree;

  if (tree == NULL)
    return;

  /* Test the entire tree */
  tmp_tree = tree;
  while (tmp_tree->parent_tree)
    tmp_tree = tmp_tree->parent_tree;
  
  if (_ctk_rbtree_is_nil (tmp_tree->root))
    return;

  _ctk_rbtree_test_structure (tmp_tree);

  g_assert ((_count_nodes (tmp_tree, tmp_tree->root->left) +
	     _count_nodes (tmp_tree, tmp_tree->root->right) + 1) == tmp_tree->root->count);
      
  _ctk_rbtree_test_height (tmp_tree, tmp_tree->root);
  _ctk_rbtree_test_dirty (tmp_tree, tmp_tree->root, CTK_RBNODE_FLAG_SET (tmp_tree->root, CTK_RBNODE_DESCENDANTS_INVALID));
  g_assert (count_total (tmp_tree, tmp_tree->root) == tmp_tree->root->total_count);
}

/* ctk_rbtree_print() - unused, for debugging only */

static void
ctk_rbtree_print_node (CtkRBTree *tree,
                       CtkRBNode *node,
                       gint       depth)
{
  gint i;
  for (i = 0; i < depth; i++)
    g_print ("\t");

  g_print ("(%p - %s) (Offset %d) (Parity %d) (Validity %d%d%d)\n",
	   node,
	   (CTK_RBNODE_GET_COLOR (node) == CTK_RBNODE_BLACK)?"BLACK":" RED ",
	   node->offset,
	   node->total_count,
	   (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_DESCENDANTS_INVALID))?1:0,
	   (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID))?1:0,
	   (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_COLUMN_INVALID))?1:0);
  if (node->children != NULL)
    {
      g_print ("Looking at child.\n");
      ctk_rbtree_print_node (node->children, node->children->root, depth + 1);
      g_print ("Done looking at child.\n");
    }
  if (!_ctk_rbtree_is_nil (node->left))
    {
      ctk_rbtree_print_node (tree, node->left, depth+1);
    }
  if (!_ctk_rbtree_is_nil (node->right))
    {
      ctk_rbtree_print_node (tree, node->right, depth+1);
    }
}

/* not static so the debugger finds it. */
void ctk_rbtree_print (CtkRBTree *tree);

void
ctk_rbtree_print (CtkRBTree *tree)
{
  g_return_if_fail (tree != NULL);

  if (_ctk_rbtree_is_nil (tree->root))
    g_print ("Empty tree...\n");
  else
    ctk_rbtree_print_node (tree, tree->root, 0);
}

/* actual tests */

static guint
append_elements (CtkRBTree *tree,
                 guint      depth,
                 guint      elements_per_depth,
                 gboolean   check,
                 guint      height)
{
  CtkRBNode *node;
  guint i;

  g_assert (depth > 0);

  node = NULL;
  depth--;

  for (i = 0; i < elements_per_depth; i++)
    {
      node = _ctk_rbtree_insert_after (tree, node, ++height, TRUE);
      if (depth)
        {
          node->children = _ctk_rbtree_new ();
          node->children->parent_tree = tree;
          node->children->parent_node = node;
          height = append_elements (node->children, depth, elements_per_depth, check, height);
        }
      if (check)
        _ctk_rbtree_test (tree);
    }

  return height;
}

static CtkRBTree *
create_rbtree (guint depth,
               guint elements_per_depth,
               gboolean check)
{
  CtkRBTree *tree;

  tree = _ctk_rbtree_new ();

  append_elements (tree, depth, elements_per_depth, check, 0);

  _ctk_rbtree_test (tree);

  return tree;
}

static void
test_create (void)
{
  CtkRBTree *tree;

  tree = create_rbtree (5, 5, TRUE);

  _ctk_rbtree_free (tree);
}

static void
test_insert_after (void)
{
  guint i;
  CtkRBTree *tree;
  CtkRBNode *node;

  tree = _ctk_rbtree_new ();
  node = NULL;

  for (i = 1; i <= 100; i++)
    {
      node = _ctk_rbtree_insert_after (tree, node, i, TRUE);
      _ctk_rbtree_test (tree);
      g_assert (tree->root->count == i);
      g_assert (tree->root->total_count == i);
      g_assert (tree->root->offset == i * (i + 1) / 2);
    }

  _ctk_rbtree_free (tree);
}

static void
test_insert_before (void)
{
  guint i;
  CtkRBTree *tree;
  CtkRBNode *node;

  tree = _ctk_rbtree_new ();
  node = NULL;

  for (i = 1; i <= 100; i++)
    {
      node = _ctk_rbtree_insert_before (tree, node, i, TRUE);
      _ctk_rbtree_test (tree);
      g_assert (tree->root->count == i);
      g_assert (tree->root->total_count == i);
      g_assert (tree->root->offset == i * (i + 1) / 2);
    }

  _ctk_rbtree_free (tree);
}

static void
test_remove_node (void)
{
  CtkRBTree *tree;

  tree = create_rbtree (3, 16, g_test_thorough ());

  while (tree->root->count > 1)
    {
      CtkRBTree *find_tree;
      CtkRBNode *find_node;
      guint i;
      
      i = g_test_rand_int_range (0, tree->root->total_count);
      if (!_ctk_rbtree_find_index (tree, i, &find_tree, &find_node))
        {
          /* We search an available index, so we mustn't fail. */
          g_assert_not_reached ();
        }
      
      _ctk_rbtree_test (find_tree);

      if (find_tree->root->count == 1)
        {
          _ctk_rbtree_remove (find_tree);
        }
      else
        _ctk_rbtree_remove_node (find_tree, find_node);
      _ctk_rbtree_test (tree);
    }

  _ctk_rbtree_free (tree);
}

static void
test_remove_root (void)
{
  CtkRBTree *tree;
  CtkRBNode *node;

  tree = _ctk_rbtree_new ();
  
  node = _ctk_rbtree_insert_after (tree, NULL, 1, TRUE);
  _ctk_rbtree_insert_after (tree, node, 2, TRUE);
  _ctk_rbtree_insert_before (tree, node, 3, TRUE);

  _ctk_rbtree_remove_node (tree, node);

  _ctk_rbtree_free (tree);
}

static gint *
fisher_yates_shuffle (guint n_items)
{
  gint *list;
  guint i, j;

  list = g_new (gint, n_items);

  for (i = 0; i < n_items; i++)
    {
      j = g_random_int_range (0, i + 1);
      list[i] = list[j];
      list[j] = i;
    }

  return list;
}

static CtkRBTree *
create_unsorted_tree (gint  *order,
                      guint  n)
{
  CtkRBTree *tree;
  CtkRBNode *node;
  guint i;

  tree = _ctk_rbtree_new ();
  node = NULL;

  for (i = 0; i < n; i++)
    {
      node = _ctk_rbtree_insert_after (tree, node, 0, TRUE);
    }

  for (i = 0; i < n; i++)
    {
      node = _ctk_rbtree_find_count (tree, order[i] + 1);
      _ctk_rbtree_node_set_height (tree, node, i);
    }

  _ctk_rbtree_test (tree);

  return tree;
}

static void
test_reorder (void)
{
  guint n = g_test_perf () ? 1000000 : 100;
  CtkRBTree *tree;
  CtkRBNode *node;
  gint *reorder;
  guint i;
  double elapsed;

  reorder = fisher_yates_shuffle (n);
  tree = create_unsorted_tree (reorder, n);

  g_test_timer_start ();

  _ctk_rbtree_reorder (tree, reorder, n);

  elapsed = g_test_timer_elapsed ();
  if (g_test_perf ())
    g_test_minimized_result (elapsed, "reordering rbtree with %u items: %gsec", n, elapsed);

  _ctk_rbtree_test (tree);

  for (node = _ctk_rbtree_first (tree), i = 0;
       node != NULL;
       node = _ctk_rbtree_next (tree, node), i++)
    {
      g_assert (CTK_RBNODE_GET_HEIGHT (node) == i);
    }
  g_assert (i == n);

  _ctk_rbtree_free (tree);

  g_free (reorder);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  setlocale (LC_ALL, "C");
  g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=%s");

  g_test_add_func ("/rbtree/create", test_create);
  g_test_add_func ("/rbtree/insert_after", test_insert_after);
  g_test_add_func ("/rbtree/insert_before", test_insert_before);
  g_test_add_func ("/rbtree/remove_node", test_remove_node);
  g_test_add_func ("/rbtree/remove_root", test_remove_root);
  g_test_add_func ("/rbtree/reorder", test_reorder);

  return g_test_run ();
}
