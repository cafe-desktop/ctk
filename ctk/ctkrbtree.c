/* ctkrbtree.c
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
#include "ctkrbtree.h"
#include "ctkdebug.h"

static CtkRBNode * _ctk_rbnode_new                (CtkRBTree  *tree,
						   gint        height);
static void        _ctk_rbnode_free               (CtkRBNode  *node);
static void        _ctk_rbnode_rotate_left        (CtkRBTree  *tree,
						   CtkRBNode  *node);
static void        _ctk_rbnode_rotate_right       (CtkRBTree  *tree,
						   CtkRBNode  *node);
static void        _ctk_rbtree_insert_fixup       (CtkRBTree  *tree,
						   CtkRBNode  *node);
static void        _ctk_rbtree_remove_node_fixup  (CtkRBTree  *tree,
						   CtkRBNode  *node,
                                                   CtkRBNode  *parent);
static inline void _fixup_validation              (CtkRBTree  *tree,
						   CtkRBNode  *node);
static inline void _fixup_total_count             (CtkRBTree  *tree,
						   CtkRBNode  *node);
#ifdef G_ENABLE_DEBUG
static void        _ctk_rbtree_test               (const gchar *where,
                                                   CtkRBTree  *tree);
static void        _ctk_rbtree_debug_spew         (CtkRBTree  *tree,
                                                   GString    *s);
#endif

static const CtkRBNode nil = {
  /* .flags = */ CTK_RBNODE_BLACK,

  /* rest is NULL */
};

gboolean
_ctk_rbtree_is_nil (CtkRBNode *node)
{
  return node == &nil;
}

static CtkRBNode *
_ctk_rbnode_new (CtkRBTree *tree,
		 gint       height)
{
  CtkRBNode *node = g_slice_new (CtkRBNode);

  node->left = (CtkRBNode *) &nil;
  node->right = (CtkRBNode *) &nil;
  node->parent = (CtkRBNode *) &nil;
  node->flags = CTK_RBNODE_RED;
  node->total_count = 1;
  node->count = 1;
  node->children = NULL;
  node->offset = height;
  return node;
}

static void
_ctk_rbnode_free (CtkRBNode *node)
{
#ifdef G_ENABLE_DEBUG
  if (CTK_DEBUG_CHECK (TREE))
    {
      node->left = (gpointer) 0xdeadbeef;
      node->right = (gpointer) 0xdeadbeef;
      node->parent = (gpointer) 0xdeadbeef;
      node->total_count = 56789;
      node->offset = 56789;
      node->count = 56789;
      node->flags = 0;
    }
#endif
  g_slice_free (CtkRBNode, node);
}

static void
_ctk_rbnode_rotate_left (CtkRBTree *tree,
			 CtkRBNode *node)
{
  gint node_height, right_height;
  CtkRBNode *right;

  g_return_if_fail (!_ctk_rbtree_is_nil (node));
  g_return_if_fail (!_ctk_rbtree_is_nil (node->right));

  right = node->right;

  node_height = CTK_RBNODE_GET_HEIGHT (node);
  right_height = CTK_RBNODE_GET_HEIGHT (right);
  node->right = right->left;
  if (!_ctk_rbtree_is_nil (right->left))
    right->left->parent = node;

  right->parent = node->parent;
  if (!_ctk_rbtree_is_nil (node->parent))
    {
      if (node == node->parent->left)
	node->parent->left = right;
      else
	node->parent->right = right;
    } else {
      tree->root = right;
    }

  right->left = node;
  node->parent = right;

  node->count = 1 + node->left->count + node->right->count;
  right->count = 1 + right->left->count + right->right->count;

  node->offset = node_height + node->left->offset + node->right->offset +
                 (node->children ? node->children->root->offset : 0);
  right->offset = right_height + right->left->offset + right->right->offset +
                  (right->children ? right->children->root->offset : 0);

  _fixup_validation (tree, node);
  _fixup_validation (tree, right);
  _fixup_total_count (tree, node);
  _fixup_total_count (tree, right);
}

static void
_ctk_rbnode_rotate_right (CtkRBTree *tree,
			  CtkRBNode *node)
{
  gint node_height, left_height;
  CtkRBNode *left;

  g_return_if_fail (!_ctk_rbtree_is_nil (node));
  g_return_if_fail (!_ctk_rbtree_is_nil (node->left));

  left = node->left;

  node_height = CTK_RBNODE_GET_HEIGHT (node);
  left_height = CTK_RBNODE_GET_HEIGHT (left);
  
  node->left = left->right;
  if (!_ctk_rbtree_is_nil (left->right))
    left->right->parent = node;

  left->parent = node->parent;
  if (!_ctk_rbtree_is_nil (node->parent))
    {
      if (node == node->parent->right)
	node->parent->right = left;
      else
	node->parent->left = left;
    }
  else
    {
      tree->root = left;
    }

  /* link node and left */
  left->right = node;
  node->parent = left;

  node->count = 1 + node->left->count + node->right->count;
  left->count = 1 + left->left->count + left->right->count;

  node->offset = node_height + node->left->offset + node->right->offset +
                 (node->children ? node->children->root->offset : 0);
  left->offset = left_height + left->left->offset + left->right->offset +
                 (left->children?left->children->root->offset:0);

  _fixup_validation (tree, node);
  _fixup_validation (tree, left);
  _fixup_total_count (tree, node);
  _fixup_total_count (tree, left);
}

static void
_ctk_rbtree_insert_fixup (CtkRBTree *tree,
			  CtkRBNode *node)
{

  /* check Red-Black properties */
  while (node != tree->root && CTK_RBNODE_GET_COLOR (node->parent) == CTK_RBNODE_RED)
    {
      /* we have a violation */
      if (node->parent == node->parent->parent->left)
	{
	  CtkRBNode *y = node->parent->parent->right;
	  if (CTK_RBNODE_GET_COLOR (y) == CTK_RBNODE_RED)
	    {
				/* uncle is CTK_RBNODE_RED */
	      CTK_RBNODE_SET_COLOR (node->parent, CTK_RBNODE_BLACK);
	      CTK_RBNODE_SET_COLOR (y, CTK_RBNODE_BLACK);
	      CTK_RBNODE_SET_COLOR (node->parent->parent, CTK_RBNODE_RED);
	      node = node->parent->parent;
	    }
	  else
	    {
				/* uncle is CTK_RBNODE_BLACK */
	      if (node == node->parent->right)
		{
		  /* make node a left child */
		  node = node->parent;
		  _ctk_rbnode_rotate_left (tree, node);
		}

				/* recolor and rotate */
	      CTK_RBNODE_SET_COLOR (node->parent, CTK_RBNODE_BLACK);
	      CTK_RBNODE_SET_COLOR (node->parent->parent, CTK_RBNODE_RED);
	      _ctk_rbnode_rotate_right(tree, node->parent->parent);
	    }
	}
      else
	{
	  /* mirror image of above code */
	  CtkRBNode *y = node->parent->parent->left;
	  if (CTK_RBNODE_GET_COLOR (y) == CTK_RBNODE_RED)
	    {
				/* uncle is CTK_RBNODE_RED */
	      CTK_RBNODE_SET_COLOR (node->parent, CTK_RBNODE_BLACK);
	      CTK_RBNODE_SET_COLOR (y, CTK_RBNODE_BLACK);
	      CTK_RBNODE_SET_COLOR (node->parent->parent, CTK_RBNODE_RED);
	      node = node->parent->parent;
	    }
	  else
	    {
				/* uncle is CTK_RBNODE_BLACK */
	      if (node == node->parent->left)
		{
		  node = node->parent;
		  _ctk_rbnode_rotate_right (tree, node);
		}
	      CTK_RBNODE_SET_COLOR (node->parent, CTK_RBNODE_BLACK);
	      CTK_RBNODE_SET_COLOR (node->parent->parent, CTK_RBNODE_RED);
	      _ctk_rbnode_rotate_left (tree, node->parent->parent);
	    }
	}
    }
  CTK_RBNODE_SET_COLOR (tree->root, CTK_RBNODE_BLACK);
}

static void
_ctk_rbtree_remove_node_fixup (CtkRBTree *tree,
			       CtkRBNode *node,
                               CtkRBNode *parent)
{
  while (node != tree->root && CTK_RBNODE_GET_COLOR (node) == CTK_RBNODE_BLACK)
    {
      if (node == parent->left)
	{
	  CtkRBNode *w = parent->right;
	  if (CTK_RBNODE_GET_COLOR (w) == CTK_RBNODE_RED)
	    {
	      CTK_RBNODE_SET_COLOR (w, CTK_RBNODE_BLACK);
	      CTK_RBNODE_SET_COLOR (parent, CTK_RBNODE_RED);
	      _ctk_rbnode_rotate_left (tree, parent);
	      w = parent->right;
	    }
	  if (CTK_RBNODE_GET_COLOR (w->left) == CTK_RBNODE_BLACK && CTK_RBNODE_GET_COLOR (w->right) == CTK_RBNODE_BLACK)
	    {
	      CTK_RBNODE_SET_COLOR (w, CTK_RBNODE_RED);
	      node = parent;
	    }
	  else
	    {
	      if (CTK_RBNODE_GET_COLOR (w->right) == CTK_RBNODE_BLACK)
		{
		  CTK_RBNODE_SET_COLOR (w->left, CTK_RBNODE_BLACK);
		  CTK_RBNODE_SET_COLOR (w, CTK_RBNODE_RED);
		  _ctk_rbnode_rotate_right (tree, w);
		  w = parent->right;
		}
	      CTK_RBNODE_SET_COLOR (w, CTK_RBNODE_GET_COLOR (parent));
	      CTK_RBNODE_SET_COLOR (parent, CTK_RBNODE_BLACK);
	      CTK_RBNODE_SET_COLOR (w->right, CTK_RBNODE_BLACK);
	      _ctk_rbnode_rotate_left (tree, parent);
	      node = tree->root;
	    }
	}
      else
	{
	  CtkRBNode *w = parent->left;
	  if (CTK_RBNODE_GET_COLOR (w) == CTK_RBNODE_RED)
	    {
	      CTK_RBNODE_SET_COLOR (w, CTK_RBNODE_BLACK);
	      CTK_RBNODE_SET_COLOR (parent, CTK_RBNODE_RED);
	      _ctk_rbnode_rotate_right (tree, parent);
	      w = parent->left;
	    }
	  if (CTK_RBNODE_GET_COLOR (w->right) == CTK_RBNODE_BLACK && CTK_RBNODE_GET_COLOR (w->left) == CTK_RBNODE_BLACK)
	    {
	      CTK_RBNODE_SET_COLOR (w, CTK_RBNODE_RED);
	      node = parent;
	    }
	  else
	    {
	      if (CTK_RBNODE_GET_COLOR (w->left) == CTK_RBNODE_BLACK)
		{
		  CTK_RBNODE_SET_COLOR (w->right, CTK_RBNODE_BLACK);
		  CTK_RBNODE_SET_COLOR (w, CTK_RBNODE_RED);
		  _ctk_rbnode_rotate_left (tree, w);
		  w = parent->left;
		}
	      CTK_RBNODE_SET_COLOR (w, CTK_RBNODE_GET_COLOR (parent));
	      CTK_RBNODE_SET_COLOR (parent, CTK_RBNODE_BLACK);
	      CTK_RBNODE_SET_COLOR (w->left, CTK_RBNODE_BLACK);
	      _ctk_rbnode_rotate_right (tree, parent);
	      node = tree->root;
	    }
	}

      parent = node->parent;
    }
  CTK_RBNODE_SET_COLOR (node, CTK_RBNODE_BLACK);
}

CtkRBTree *
_ctk_rbtree_new (void)
{
  CtkRBTree *retval;

  retval = g_new (CtkRBTree, 1);
  retval->parent_tree = NULL;
  retval->parent_node = NULL;

  retval->root = (CtkRBNode *) &nil;

  return retval;
}

static void
_ctk_rbtree_free_helper (CtkRBTree  *tree,
			 CtkRBNode  *node,
			 gpointer    data)
{
  if (node->children)
    _ctk_rbtree_free (node->children);

  _ctk_rbnode_free (node);
}

void
_ctk_rbtree_free (CtkRBTree *tree)
{
  _ctk_rbtree_traverse (tree,
			tree->root,
			G_POST_ORDER,
			_ctk_rbtree_free_helper,
			NULL);

  if (tree->parent_node &&
      tree->parent_node->children == tree)
    tree->parent_node->children = NULL;
  g_free (tree);
}

static void
ctk_rbnode_adjust (CtkRBTree *tree,
                   CtkRBNode *node,
                   int        count_diff,
                   int        total_count_diff,
                   int        offset_diff)
{
  while (tree && node && !_ctk_rbtree_is_nil (node))
    {
      _fixup_validation (tree, node);
      node->offset += offset_diff;
      node->count += count_diff;
      node->total_count += total_count_diff;
      
      node = node->parent;
      if (_ctk_rbtree_is_nil (node))
	{
	  node = tree->parent_node;
	  tree = tree->parent_tree;
          count_diff = 0;
	}
    }
}

void
_ctk_rbtree_remove (CtkRBTree *tree)
{
#ifdef G_ENABLE_DEBUG
  CtkRBTree *tmp_tree;

  if (CTK_DEBUG_CHECK (TREE))
    _ctk_rbtree_test (G_STRLOC, tree);
#endif
  
  /* ugly hack to make _fixup_validation work in the first iteration of the
   * loop below */
  CTK_RBNODE_UNSET_FLAG (tree->root, CTK_RBNODE_DESCENDANTS_INVALID);
  
  ctk_rbnode_adjust (tree->parent_tree, 
                     tree->parent_node,
                     0,
                     - (int) tree->root->total_count,
                     - tree->root->offset);

#ifdef G_ENABLE_DEBUG
  tmp_tree = tree->parent_tree;
#endif

  _ctk_rbtree_free (tree);

#ifdef G_ENABLE_DEBUG
  if (CTK_DEBUG_CHECK (TREE))
    _ctk_rbtree_test (G_STRLOC, tmp_tree);
#endif
}


CtkRBNode *
_ctk_rbtree_insert_after (CtkRBTree *tree,
			  CtkRBNode *current,
			  gint       height,
			  gboolean   valid)
{
  CtkRBNode *node;
  gboolean right = TRUE;

#ifdef G_ENABLE_DEBUG
  if (CTK_DEBUG_CHECK (TREE))
    {
      GString *s;

      s = g_string_new ("");
      g_string_append_printf (s, "_ctk_rbtree_insert_after: %p\n", current);
      _ctk_rbtree_debug_spew (tree, s);
      g_message ("%s", s->str);
      g_string_free (s, TRUE);
      _ctk_rbtree_test (G_STRLOC, tree);
    }
#endif

  if (current != NULL && !_ctk_rbtree_is_nil (current->right))
    {
      current = current->right;
      while (!_ctk_rbtree_is_nil (current->left))
	current = current->left;
      right = FALSE;
    }
  /* setup new node */
  node = _ctk_rbnode_new (tree, height);

  /* insert node in tree */
  if (current)
    {
      node->parent = current;
      if (right)
	current->right = node;
      else
	current->left = node;
      ctk_rbnode_adjust (tree, node->parent,
                         1, 1, height);
    }
  else
    {
      g_assert (_ctk_rbtree_is_nil (tree->root));
      tree->root = node;
      ctk_rbnode_adjust (tree->parent_tree, tree->parent_node,
                         0, 1, height);
    }

  if (valid)
    _ctk_rbtree_node_mark_valid (tree, node);
  else
    _ctk_rbtree_node_mark_invalid (tree, node);

  _ctk_rbtree_insert_fixup (tree, node);

#ifdef G_ENABLE_DEBUG
  if (CTK_DEBUG_CHECK (TREE))
    {
      GString *s;

      s = g_string_new ("_ctk_rbtree_insert_after finished...\n");
      _ctk_rbtree_debug_spew (tree, s);
      g_message ("%s", s->str);
      g_string_free (s, TRUE);
      _ctk_rbtree_test (G_STRLOC, tree);
    }
#endif

  return node;
}

CtkRBNode *
_ctk_rbtree_insert_before (CtkRBTree *tree,
			   CtkRBNode *current,
			   gint       height,
			   gboolean   valid)
{
  CtkRBNode *node;
  gboolean left = TRUE;

#ifdef G_ENABLE_DEBUG
  if (CTK_DEBUG_CHECK (TREE))
    {
      GString *s;

      s = g_string_new ("");
      g_string_append_printf (s, "_ctk_rbtree_insert_before: %p\n", current);
      _ctk_rbtree_debug_spew (tree, s);
      g_message ("%s", s->str);
      g_string_free (s, TRUE);
      _ctk_rbtree_test (G_STRLOC, tree);
    }
#endif

  if (current != NULL && !_ctk_rbtree_is_nil (current->left))
    {
      current = current->left;
      while (!_ctk_rbtree_is_nil (current->right))
	current = current->right;
      left = FALSE;
    }

  /* setup new node */
  node = _ctk_rbnode_new (tree, height);

  /* insert node in tree */
  if (current)
    {
      node->parent = current;
      if (left)
	current->left = node;
      else
	current->right = node;
      ctk_rbnode_adjust (tree, node->parent,
                         1, 1, height);
    }
  else
    {
      g_assert (_ctk_rbtree_is_nil (tree->root));
      tree->root = node;
      ctk_rbnode_adjust (tree->parent_tree, tree->parent_node,
                         0, 1, height);
    }

  if (valid)
    _ctk_rbtree_node_mark_valid (tree, node);
  else
    _ctk_rbtree_node_mark_invalid (tree, node);

  _ctk_rbtree_insert_fixup (tree, node);

#ifdef G_ENABLE_DEBUG
  if (CTK_DEBUG_CHECK (TREE))
    {
      GString *s;

      s = g_string_new ("_ctk_rbtree_insert_before finished...\n");
      _ctk_rbtree_debug_spew (tree, s);
      g_message ("%s", s->str);
      g_string_free (s, TRUE);
      _ctk_rbtree_test (G_STRLOC, tree);
    }
#endif

  return node;
}

CtkRBNode *
_ctk_rbtree_find_count (CtkRBTree *tree,
			gint       count)
{
  CtkRBNode *node;

  node = tree->root;
  while (!_ctk_rbtree_is_nil (node) && (node->left->count + 1 != count))
    {
      if (node->left->count >= count)
	node = node->left;
      else
	{
	  count -= (node->left->count + 1);
	  node = node->right;
	}
    }
  if (_ctk_rbtree_is_nil (node))
    return NULL;
  return node;
}

void
_ctk_rbtree_node_set_height (CtkRBTree *tree,
			     CtkRBNode *node,
			     gint       height)
{
  gint diff = height - CTK_RBNODE_GET_HEIGHT (node);

  if (diff == 0)
    return;

  ctk_rbnode_adjust (tree, node, 0, 0, diff);

#ifdef G_ENABLE_DEBUG
  if (CTK_DEBUG_CHECK (TREE))
    _ctk_rbtree_test (G_STRLOC, tree);
#endif
}

void
_ctk_rbtree_node_mark_invalid (CtkRBTree *tree,
			       CtkRBNode *node)
{
  if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID))
    return;

  CTK_RBNODE_SET_FLAG (node, CTK_RBNODE_INVALID);
  do
    {
      if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_DESCENDANTS_INVALID))
	return;
      CTK_RBNODE_SET_FLAG (node, CTK_RBNODE_DESCENDANTS_INVALID);
      node = node->parent;
      if (_ctk_rbtree_is_nil (node))
	{
	  node = tree->parent_node;
	  tree = tree->parent_tree;
	}
    }
  while (node);
}

#if 0
/* Draconian version. */
void
_ctk_rbtree_node_mark_invalid (CtkRBTree *tree,
			       CtkRBNode *node)
{
  CTK_RBNODE_SET_FLAG (node, CTK_RBNODE_INVALID);
  do
    {
      _fixup_validation (tree, node);
      node = node->parent;
      if (_ctk_rbtree_is_nil (node))
	{
	  node = tree->parent_node;
	  tree = tree->parent_tree;
	}
    }
  while (node);
}
#endif

void
_ctk_rbtree_node_mark_valid (CtkRBTree *tree,
			     CtkRBNode *node)
{
  if ((!CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID)) &&
      (!CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_COLUMN_INVALID)))
    return;

  CTK_RBNODE_UNSET_FLAG (node, CTK_RBNODE_INVALID);
  CTK_RBNODE_UNSET_FLAG (node, CTK_RBNODE_COLUMN_INVALID);

  do
    {
      if ((CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID)) ||
	  (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_COLUMN_INVALID)) ||
	  (node->children && CTK_RBNODE_FLAG_SET (node->children->root, CTK_RBNODE_DESCENDANTS_INVALID)) ||
	  (CTK_RBNODE_FLAG_SET (node->left, CTK_RBNODE_DESCENDANTS_INVALID)) ||
	  (CTK_RBNODE_FLAG_SET (node->right, CTK_RBNODE_DESCENDANTS_INVALID)))
	return;

      CTK_RBNODE_UNSET_FLAG (node, CTK_RBNODE_DESCENDANTS_INVALID);
      node = node->parent;
      if (_ctk_rbtree_is_nil (node))
	{
	  node = tree->parent_node;
	  tree = tree->parent_tree;
	}
    }
  while (node);
}

#if 0
/* Draconian version */
void
_ctk_rbtree_node_mark_valid (CtkRBTree *tree,
			     CtkRBNode *node)
{
  CTK_RBNODE_UNSET_FLAG (node, CTK_RBNODE_INVALID);
  CTK_RBNODE_UNSET_FLAG (node, CTK_RBNODE_COLUMN_INVALID);

  do
    {
      _fixup_validation (tree, node);
      node = node->parent;
      if (_ctk_rbtree_is_nil (node))
	{
	  node = tree->parent_node;
	  tree = tree->parent_tree;
	}
    }
  while (node);
}
#endif
/* Assume tree is the root node as it doesn't set DESCENDANTS_INVALID above.
 */
void
_ctk_rbtree_column_invalid (CtkRBTree *tree)
{
  CtkRBNode *node;

  if (tree == NULL)
    return;

  node = _ctk_rbtree_first (tree);

  do
    {
      if (! (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID)))
	CTK_RBNODE_SET_FLAG (node, CTK_RBNODE_COLUMN_INVALID);
      CTK_RBNODE_SET_FLAG (node, CTK_RBNODE_DESCENDANTS_INVALID);

      if (node->children)
	_ctk_rbtree_column_invalid (node->children);
    }
  while ((node = _ctk_rbtree_next (tree, node)) != NULL);
}

void
_ctk_rbtree_mark_invalid (CtkRBTree *tree)
{
  CtkRBNode *node;

  if (tree == NULL)
    return;

  node = _ctk_rbtree_first (tree);

  do
    {
      CTK_RBNODE_SET_FLAG (node, CTK_RBNODE_INVALID);
      CTK_RBNODE_SET_FLAG (node, CTK_RBNODE_DESCENDANTS_INVALID);

      if (node->children)
	_ctk_rbtree_mark_invalid (node->children);
    }
  while ((node = _ctk_rbtree_next (tree, node)) != NULL);
}

void
_ctk_rbtree_set_fixed_height (CtkRBTree *tree,
			      gint       height,
			      gboolean   mark_valid)
{
  CtkRBNode *node;

  if (tree == NULL)
    return;

  node = _ctk_rbtree_first (tree);

  do
    {
      if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID))
        {
	  _ctk_rbtree_node_set_height (tree, node, height);
	  if (mark_valid)
	    _ctk_rbtree_node_mark_valid (tree, node);
	}

      if (node->children)
	_ctk_rbtree_set_fixed_height (node->children, height, mark_valid);
    }
  while ((node = _ctk_rbtree_next (tree, node)) != NULL);
}

static void
reorder_prepare (CtkRBTree *tree,
                 CtkRBNode *node,
                 gpointer   data)
{
  node->offset -= node->left->offset + node->right->offset;
  CTK_RBNODE_UNSET_FLAG (node, CTK_RBNODE_DESCENDANTS_INVALID);
}

static void
reorder_fixup (CtkRBTree *tree,
               CtkRBNode *node,
               gpointer   data)
{
  node->offset += node->left->offset + node->right->offset;
  node->count = 1 + node->left->count + node->right->count;
  _fixup_validation (tree, node);
  _fixup_total_count (tree, node);
}

static void
reorder_copy_node (CtkRBTree *tree,
                   CtkRBNode *to,
                   CtkRBNode *from)
{
  to->flags = (to->flags & CTK_RBNODE_NON_COLORS) | CTK_RBNODE_GET_COLOR (from);

  to->left = from->left;
  if (!_ctk_rbtree_is_nil (to->left))
    to->left->parent = to;

  to->right = from->right;
  if (!_ctk_rbtree_is_nil (to->right))
    to->right->parent = to;

  to->parent = from->parent;
  if (_ctk_rbtree_is_nil (to->parent))
    tree->root = to;
  else if (to->parent->left == from)
    to->parent->left = to;
  else if (to->parent->right == from)
    to->parent->right = to;
}

/* It basically pulls everything out of the tree, rearranges it, and puts it
 * back together.  Our strategy is to keep the old RBTree intact, and just
 * rearrange the contents.  When that is done, we go through and update the
 * heights.  There is probably a more elegant way to write this function.  If
 * anyone wants to spend the time writing it, patches will be accepted.
 */
void
_ctk_rbtree_reorder (CtkRBTree *tree,
		     gint      *new_order,
		     gint       length)
{
  CtkRBNode **nodes;
  CtkRBNode *node;
  gint i, j;
  
  g_return_if_fail (tree != NULL);
  g_return_if_fail (length > 0);
  g_return_if_fail (tree->root->count == length);
  
  nodes = g_new (CtkRBNode *, length);

  _ctk_rbtree_traverse (tree, tree->root, G_PRE_ORDER, reorder_prepare, NULL);

  for (node = _ctk_rbtree_first (tree), i = 0;
       node;
       node = _ctk_rbtree_next (tree, node), i++)
    {
      nodes[i] = node;
    }

  for (i = 0; i < length; i++)
    {
      CtkRBNode tmp = { 0, };
      GSList *l, *cycle = NULL;

      tmp.offset = -1;

      /* already swapped */
      if (nodes[i] == NULL)
        continue;
      /* no need to swap */
      if (new_order[i] == i)
        continue;

      /* make a list out of the pending nodes */
      for (j = i; new_order[j] != i; j = new_order[j])
        {
          cycle = g_slist_prepend (cycle, nodes[j]);
          nodes[j] = NULL;
        }

      node = nodes[j];
      reorder_copy_node (tree, &tmp, node);
      for (l = cycle; l; l = l->next)
        {
          reorder_copy_node (tree, node, l->data);
          node = l->data;
        }

      reorder_copy_node (tree, node, &tmp);
      nodes[j] = NULL;
      g_slist_free (cycle);
    }

  _ctk_rbtree_traverse (tree, tree->root, G_POST_ORDER, reorder_fixup, NULL);

  g_free (nodes);
}

/**
 * _ctk_rbtree_contains:
 * @tree: a tree
 * @potential_child: a potential child of @tree
 *
 * Checks if @potential_child is a child (direct or via intermediate
 * trees) of @tree.
 *
 * Returns: %TRUE if @potentitial_child is a child of @tree.
 **/
gboolean
_ctk_rbtree_contains (CtkRBTree *tree,
                      CtkRBTree *potential_child)
{
  g_return_val_if_fail (tree != NULL, FALSE);
  g_return_val_if_fail (potential_child != NULL, FALSE);

  do {
    potential_child = potential_child->parent_tree;
    if (potential_child == tree)
      return TRUE;
  } while (potential_child != NULL);

  return FALSE;
}

gint
_ctk_rbtree_node_find_offset (CtkRBTree *tree,
			      CtkRBNode *node)
{
  gint retval;

  g_assert (node);
  g_assert (node->left);
  
  retval = node->left->offset;

  while (tree && node && !_ctk_rbtree_is_nil (node))
    {
      CtkRBNode *last;

      last = node;
      node = node->parent;

      /* Add left branch, plus children, iff we came from the right */
      if (node->right == last)
	retval += node->offset - node->right->offset;
      
      if (_ctk_rbtree_is_nil (node))
	{
	  node = tree->parent_node;
	  tree = tree->parent_tree;

          /* Add the parent node, plus the left branch. */
	  if (node)
	    retval += node->left->offset + CTK_RBNODE_GET_HEIGHT (node);
	}
    }
  return retval;
}

guint
_ctk_rbtree_node_get_index (CtkRBTree *tree,
                            CtkRBNode *node)
{
  guint retval;  
  
  g_assert (node);
  g_assert (node->left);
  
  retval = node->left->total_count;

  while (tree && node && !_ctk_rbtree_is_nil (node))
    {
      CtkRBNode *last;

      last = node;
      node = node->parent;

      /* Add left branch, plus children, iff we came from the right */
      if (node->right == last)
	retval += node->total_count - node->right->total_count;
      
      if (_ctk_rbtree_is_nil (node))
	{
	  node = tree->parent_node;
	  tree = tree->parent_tree;

          /* Add the parent node, plus the left branch. */
	  if (node)
	    retval += node->left->total_count + 1; /* 1 == CTK_RBNODE_GET_PARITY() */
	}
    }
  
  return retval;
}

static gint
ctk_rbtree_real_find_offset (CtkRBTree  *tree,
			     gint        height,
			     CtkRBTree **new_tree,
			     CtkRBNode **new_node)
{
  CtkRBNode *tmp_node;

  g_assert (tree);

  if (height < 0)
    {
      *new_tree = NULL;
      *new_node = NULL;

      return 0;
    }
  
    
  tmp_node = tree->root;
  while (!_ctk_rbtree_is_nil (tmp_node) &&
	 (tmp_node->left->offset > height ||
	  (tmp_node->offset - tmp_node->right->offset) < height))
    {
      if (tmp_node->left->offset > height)
	tmp_node = tmp_node->left;
      else
	{
	  height -= (tmp_node->offset - tmp_node->right->offset);
	  tmp_node = tmp_node->right;
	}
    }
  if (_ctk_rbtree_is_nil (tmp_node))
    {
      *new_tree = NULL;
      *new_node = NULL;
      return 0;
    }
  if (tmp_node->children)
    {
      if ((tmp_node->offset -
	   tmp_node->right->offset -
	   tmp_node->children->root->offset) > height)
	{
	  *new_tree = tree;
	  *new_node = tmp_node;
	  return (height - tmp_node->left->offset);
	}
      return ctk_rbtree_real_find_offset (tmp_node->children,
					  height - tmp_node->left->offset -
					  (tmp_node->offset -
					   tmp_node->left->offset -
					   tmp_node->right->offset -
					   tmp_node->children->root->offset),
					  new_tree,
					  new_node);
    }
  *new_tree = tree;
  *new_node = tmp_node;
  return (height - tmp_node->left->offset);
}

gint
_ctk_rbtree_find_offset (CtkRBTree  *tree,
			 gint        height,
			 CtkRBTree **new_tree,
			 CtkRBNode **new_node)
{
  g_assert (tree);

  if ((height < 0) ||
      (height >= tree->root->offset))
    {
      *new_tree = NULL;
      *new_node = NULL;

      return 0;
    }
  return ctk_rbtree_real_find_offset (tree, height, new_tree, new_node);
}

gboolean
_ctk_rbtree_find_index (CtkRBTree  *tree,
                        guint       index,
                        CtkRBTree **new_tree,
                        CtkRBNode **new_node)
{
  CtkRBNode *tmp_node;

  g_assert (tree);

  tmp_node = tree->root;
  while (!_ctk_rbtree_is_nil (tmp_node))
    {
      if (tmp_node->left->total_count > index)
        {
          tmp_node = tmp_node->left;
        }
      else if (tmp_node->total_count - tmp_node->right->total_count <= index)
        {
          index -= tmp_node->total_count - tmp_node->right->total_count;
          tmp_node = tmp_node->right;
        }
      else
        {
          index -= tmp_node->left->total_count;
          break;
        }
    }
  if (_ctk_rbtree_is_nil (tmp_node))
    {
      *new_tree = NULL;
      *new_node = NULL;
      return FALSE;
    }

  if (index > 0)
    {
      g_assert (tmp_node->children);

      return _ctk_rbtree_find_index (tmp_node->children,
                                     index - 1,
                                     new_tree,
                                     new_node);
    }

  *new_tree = tree;
  *new_node = tmp_node;
  return TRUE;
}

void
_ctk_rbtree_remove_node (CtkRBTree *tree,
			 CtkRBNode *node)
{
  CtkRBNode *x, *y;
  gint y_height;
  guint y_total_count;
  
  g_return_if_fail (tree != NULL);
  g_return_if_fail (node != NULL);

  
#ifdef G_ENABLE_DEBUG
  if (CTK_DEBUG_CHECK (TREE))
    {
      GString *s;

      s = g_string_new ("");
      g_string_append_printf (s, "_ctk_rbtree_remove_node: %p\n", node);
      _ctk_rbtree_debug_spew (tree, s);
      g_message ("%s", s->str);
      g_string_free (s, TRUE);
      _ctk_rbtree_test (G_STRLOC, tree);
    }
#endif
  
  /* make sure we're deleting a node that's actually in the tree */
  for (x = node; !_ctk_rbtree_is_nil (x->parent); x = x->parent)
    ;
  g_return_if_fail (x == tree->root);

#ifdef G_ENABLE_DEBUG
  if (CTK_DEBUG_CHECK (TREE))
    _ctk_rbtree_test (G_STRLOC, tree);
#endif
  
  if (_ctk_rbtree_is_nil (node->left) ||
      _ctk_rbtree_is_nil (node->right))
    {
      y = node;
    }
  else
    {
      y = node->right;

      while (!_ctk_rbtree_is_nil (y->left))
	y = y->left;
    }

  y_height = CTK_RBNODE_GET_HEIGHT (y) 
             + (y->children ? y->children->root->offset : 0);
  y_total_count = 1 + (y->children ? y->children->root->total_count : 0);

  /* x is y's only child, or nil */
  if (!_ctk_rbtree_is_nil (y->left))
    x = y->left;
  else
    x = y->right;

  /* remove y from the parent chain */
  if (!_ctk_rbtree_is_nil (x))
    x->parent = y->parent;
  if (!_ctk_rbtree_is_nil (y->parent))
    {
      if (y == y->parent->left)
	y->parent->left = x;
      else
	y->parent->right = x;
    }
  else
    {
      tree->root = x;
    }

  /* We need to clean up the validity of the tree.
   */
  ctk_rbnode_adjust (tree, y, -1, - y_total_count, - y_height);

  if (CTK_RBNODE_GET_COLOR (y) == CTK_RBNODE_BLACK)
    _ctk_rbtree_remove_node_fixup (tree, x, y->parent);

  if (y != node)
    {
      gint node_height, node_total_count;

      /* We want to see how much we remove from the aggregate values.
       * This is all the children we remove plus the node's values.
       */
      node_height = CTK_RBNODE_GET_HEIGHT (node)
                    + (node->children ? node->children->root->offset : 0);
      node_total_count = 1
                         + (node->children ? node->children->root->total_count : 0);

      /* Move the node over */
      if (CTK_RBNODE_GET_COLOR (node) != CTK_RBNODE_GET_COLOR (y))
	y->flags ^= (CTK_RBNODE_BLACK | CTK_RBNODE_RED);

      y->left = node->left;
      if (!_ctk_rbtree_is_nil (y->left))
        y->left->parent = y;
      y->right = node->right;
      if (!_ctk_rbtree_is_nil (y->right))
        y->right->parent = y;
      y->parent = node->parent;
      if (!_ctk_rbtree_is_nil (y->parent))
        {
          if (y->parent->left == node)
            y->parent->left = y;
          else
            y->parent->right = y;
        }
      else
        {
          tree->root = y;
        }
      y->count = node->count;
      y->total_count = node->total_count;
      y->offset = node->offset;

      ctk_rbnode_adjust (tree, y, 
                         0,
                         y_total_count - node_total_count,
                         y_height - node_height);
    }

  _ctk_rbnode_free (node);

#ifdef G_ENABLE_DEBUG
  if (CTK_DEBUG_CHECK (TREE))
    {
      GString *s;

      s = g_string_new ("_ctk_rbtree_remove_node finished...\n");
      _ctk_rbtree_debug_spew (tree, s);
      g_message ("%s", s->str);
      g_string_free (s, TRUE);
      _ctk_rbtree_test (G_STRLOC, tree);
    }
#endif
}

CtkRBNode *
_ctk_rbtree_first (CtkRBTree *tree)
{
  CtkRBNode *node;

  node = tree->root;

  if (_ctk_rbtree_is_nil (node))
    return NULL;

  while (!_ctk_rbtree_is_nil (node->left))
    node = node->left;

  return node;
}

CtkRBNode *
_ctk_rbtree_next (CtkRBTree *tree,
		  CtkRBNode *node)
{
  g_return_val_if_fail (tree != NULL, NULL);
  g_return_val_if_fail (node != NULL, NULL);

  /* Case 1: the node's below us. */
  if (!_ctk_rbtree_is_nil (node->right))
    {
      node = node->right;
      while (!_ctk_rbtree_is_nil (node->left))
	node = node->left;
      return node;
    }

  /* Case 2: it's an ancestor */
  while (!_ctk_rbtree_is_nil (node->parent))
    {
      if (node->parent->right == node)
	node = node->parent;
      else
	return (node->parent);
    }

  /* Case 3: There is no next node */
  return NULL;
}

CtkRBNode *
_ctk_rbtree_prev (CtkRBTree *tree,
		  CtkRBNode *node)
{
  g_return_val_if_fail (tree != NULL, NULL);
  g_return_val_if_fail (node != NULL, NULL);

  /* Case 1: the node's below us. */
  if (!_ctk_rbtree_is_nil (node->left))
    {
      node = node->left;
      while (!_ctk_rbtree_is_nil (node->right))
	node = node->right;
      return node;
    }

  /* Case 2: it's an ancestor */
  while (!_ctk_rbtree_is_nil (node->parent))
    {
      if (node->parent->left == node)
	node = node->parent;
      else
	return (node->parent);
    }

  /* Case 3: There is no next node */
  return NULL;
}

void
_ctk_rbtree_next_full (CtkRBTree  *tree,
		       CtkRBNode  *node,
		       CtkRBTree **new_tree,
		       CtkRBNode **new_node)
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (node != NULL);
  g_return_if_fail (new_tree != NULL);
  g_return_if_fail (new_node != NULL);

  if (node->children)
    {
      *new_tree = node->children;
      *new_node = (*new_tree)->root;
      while (!_ctk_rbtree_is_nil ((*new_node)->left))
	*new_node = (*new_node)->left;
      return;
    }

  *new_tree = tree;
  *new_node = _ctk_rbtree_next (tree, node);

  while ((*new_node == NULL) &&
	 (*new_tree != NULL))
    {
      *new_node = (*new_tree)->parent_node;
      *new_tree = (*new_tree)->parent_tree;
      if (*new_tree)
	*new_node = _ctk_rbtree_next (*new_tree, *new_node);
    }
}

void
_ctk_rbtree_prev_full (CtkRBTree  *tree,
		       CtkRBNode  *node,
		       CtkRBTree **new_tree,
		       CtkRBNode **new_node)
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (node != NULL);
  g_return_if_fail (new_tree != NULL);
  g_return_if_fail (new_node != NULL);

  *new_tree = tree;
  *new_node = _ctk_rbtree_prev (tree, node);

  if (*new_node == NULL)
    {
      *new_node = (*new_tree)->parent_node;
      *new_tree = (*new_tree)->parent_tree;
    }
  else
    {
      while ((*new_node)->children)
	{
	  *new_tree = (*new_node)->children;
	  *new_node = (*new_tree)->root;
          while (!_ctk_rbtree_is_nil ((*new_node)->right))
	    *new_node = (*new_node)->right;
	}
    }
}

gint
_ctk_rbtree_get_depth (CtkRBTree *tree)
{
  CtkRBTree *tmp_tree;
  gint depth = 0;

  tmp_tree = tree->parent_tree;
  while (tmp_tree)
    {
      ++depth;
      tmp_tree = tmp_tree->parent_tree;
    }

  return depth;
}

static void
_ctk_rbtree_traverse_pre_order (CtkRBTree             *tree,
				CtkRBNode             *node,
				CtkRBTreeTraverseFunc  func,
				gpointer               data)
{
  if (_ctk_rbtree_is_nil (node))
    return;

  (* func) (tree, node, data);
  _ctk_rbtree_traverse_pre_order (tree, node->left, func, data);
  _ctk_rbtree_traverse_pre_order (tree, node->right, func, data);
}

static void
_ctk_rbtree_traverse_post_order (CtkRBTree             *tree,
				 CtkRBNode             *node,
				 CtkRBTreeTraverseFunc  func,
				 gpointer               data)
{
  if (_ctk_rbtree_is_nil (node))
    return;

  _ctk_rbtree_traverse_post_order (tree, node->left, func, data);
  _ctk_rbtree_traverse_post_order (tree, node->right, func, data);
  (* func) (tree, node, data);
}

void
_ctk_rbtree_traverse (CtkRBTree             *tree,
		      CtkRBNode             *node,
		      GTraverseType          order,
		      CtkRBTreeTraverseFunc  func,
		      gpointer               data)
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (node != NULL);
  g_return_if_fail (func != NULL);
  g_return_if_fail (order <= G_LEVEL_ORDER);

  switch (order)
    {
    case G_PRE_ORDER:
      _ctk_rbtree_traverse_pre_order (tree, node, func, data);
      break;
    case G_POST_ORDER:
      _ctk_rbtree_traverse_post_order (tree, node, func, data);
      break;
    case G_IN_ORDER:
    case G_LEVEL_ORDER:
    default:
      g_warning ("unsupported traversal order.");
      break;
    }
}

static inline
void _fixup_validation (CtkRBTree *tree,
			CtkRBNode *node)
{
  if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID) ||
      CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_COLUMN_INVALID) ||
      CTK_RBNODE_FLAG_SET (node->left, CTK_RBNODE_DESCENDANTS_INVALID) ||
      CTK_RBNODE_FLAG_SET (node->right, CTK_RBNODE_DESCENDANTS_INVALID) ||
      (node->children != NULL && CTK_RBNODE_FLAG_SET (node->children->root, CTK_RBNODE_DESCENDANTS_INVALID)))
    {
      CTK_RBNODE_SET_FLAG (node, CTK_RBNODE_DESCENDANTS_INVALID);
    }
  else
    {
      CTK_RBNODE_UNSET_FLAG (node, CTK_RBNODE_DESCENDANTS_INVALID);
    }
}

static inline
void _fixup_total_count (CtkRBTree *tree,
		    CtkRBNode *node)
{
  node->total_count = 1 +
    (node->children != NULL ? node->children->root->total_count : 0) + 
    node->left->total_count + node->right->total_count;
}

#ifdef G_ENABLE_DEBUG
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
    g_error ("total count incorrect for node");

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
    g_error ("Tree failed");
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

static void
_ctk_rbtree_test_structure_helper (CtkRBTree *tree,
				   CtkRBNode *node)
{
  g_assert (!_ctk_rbtree_is_nil (node));

  g_assert (node->left != NULL);
  g_assert (node->right != NULL);
  g_assert (node->parent != NULL);

  if (!_ctk_rbtree_is_nil (node->left))
    {
      g_assert (node->left->parent == node);
      _ctk_rbtree_test_structure_helper (tree, node->left);
    }
  if (!_ctk_rbtree_is_nil (node->right))
    {
      g_assert (node->right->parent == node);
      _ctk_rbtree_test_structure_helper (tree, node->right);
    }

  if (node->children != NULL)
    {
      g_assert (node->children->parent_tree == tree);
      g_assert (node->children->parent_node == node);

      _ctk_rbtree_test_structure (node->children);
    }
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
_ctk_rbtree_test (const gchar *where,
                  CtkRBTree   *tree)
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

static void
_ctk_rbtree_debug_spew_helper (CtkRBTree *tree,
			       CtkRBNode *node,
                               GString   *s,
			       gint       depth)
{
  gint i;
  for (i = 0; i < depth; i++)
    g_string_append (s, "\t");

  g_string_append_printf (s, "(%p - %s) (Offset %d) (Parity %d) (Validity %d%d%d)\n",
	   node,
	   (CTK_RBNODE_GET_COLOR (node) == CTK_RBNODE_BLACK)?"BLACK":" RED ",
	   node->offset,
	   node->total_count,
	   (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_DESCENDANTS_INVALID))?1:0,
	   (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID))?1:0,
	   (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_COLUMN_INVALID))?1:0);
  if (node->children != NULL)
    {
      g_string_append (s, "Looking at child.\n");
      _ctk_rbtree_debug_spew (node->children, s);
      g_string_append (s, "Done looking at child.\n");
    }
  if (!_ctk_rbtree_is_nil (node->left))
    {
      _ctk_rbtree_debug_spew_helper (tree, node->left, s, depth + 1);
    }
  if (!_ctk_rbtree_is_nil (node->right))
    {
      _ctk_rbtree_debug_spew_helper (tree, node->right, s, depth + 1);
    }
}

static void
_ctk_rbtree_debug_spew (CtkRBTree *tree, GString *s)
{
  g_return_if_fail (tree != NULL);

  if (_ctk_rbtree_is_nil (tree->root))
    g_string_append (s, "Empty tree...");
  else
    _ctk_rbtree_debug_spew_helper (tree, tree->root, s, 0);
}
#endif /* G_ENABLE_DEBUG */
