/* ctkrbtree.h
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

/* A Red-Black Tree implementation used specifically by CtkTreeView.
 */
#ifndef __CTK_RBTREE_H__
#define __CTK_RBTREE_H__

#include <glib.h>


G_BEGIN_DECLS


typedef enum
{
  CTK_RBNODE_BLACK = 1 << 0,
  CTK_RBNODE_RED = 1 << 1,
  CTK_RBNODE_IS_PARENT = 1 << 2,
  CTK_RBNODE_IS_SELECTED = 1 << 3,
  CTK_RBNODE_IS_PRELIT = 1 << 4,
  CTK_RBNODE_INVALID = 1 << 7,
  CTK_RBNODE_COLUMN_INVALID = 1 << 8,
  CTK_RBNODE_DESCENDANTS_INVALID = 1 << 9,
  CTK_RBNODE_NON_COLORS = CTK_RBNODE_IS_PARENT |
  			  CTK_RBNODE_IS_SELECTED |
  			  CTK_RBNODE_IS_PRELIT |
                          CTK_RBNODE_INVALID |
                          CTK_RBNODE_COLUMN_INVALID |
                          CTK_RBNODE_DESCENDANTS_INVALID
} CtkRBNodeColor;

typedef struct _CtkRBTree CtkRBTree;
typedef struct _CtkRBNode CtkRBNode;
typedef struct _CtkRBTreeView CtkRBTreeView;

typedef void (*CtkRBTreeTraverseFunc) (CtkRBTree  *tree,
                                       CtkRBNode  *node,
                                       gpointer  data);

struct _CtkRBTree
{
  CtkRBNode *root;
  CtkRBTree *parent_tree;
  CtkRBNode *parent_node;
};

struct _CtkRBNode
{
  guint flags : 14;

  /* count is the number of nodes beneath us, plus 1 for ourselves.
   * i.e. node->left->count + node->right->count + 1
   */
  gint count;

  CtkRBNode *left;
  CtkRBNode *right;
  CtkRBNode *parent;

  /* count the number of total nodes beneath us, including nodes
   * of children trees.
   * i.e. node->left->count + node->right->count + node->children->root->count + 1
   */
  guint total_count;
  
  /* this is the total of sizes of
   * node->left, node->right, our own height, and the height
   * of all trees in ->children, iff children exists because
   * the thing is expanded.
   */
  gint offset;

  /* Child trees */
  CtkRBTree *children;
};


#define CTK_RBNODE_GET_COLOR(node)		(node?(((node->flags&CTK_RBNODE_RED)==CTK_RBNODE_RED)?CTK_RBNODE_RED:CTK_RBNODE_BLACK):CTK_RBNODE_BLACK)
#define CTK_RBNODE_SET_COLOR(node,color) 	if((node->flags&color)!=color)node->flags=node->flags^(CTK_RBNODE_RED|CTK_RBNODE_BLACK)
#define CTK_RBNODE_GET_HEIGHT(node) 		(node->offset-(node->left->offset+node->right->offset+(node->children?node->children->root->offset:0)))
#define CTK_RBNODE_SET_FLAG(node, flag)   	G_STMT_START{ (node->flags|=flag); }G_STMT_END
#define CTK_RBNODE_UNSET_FLAG(node, flag) 	G_STMT_START{ (node->flags&=~(flag)); }G_STMT_END
#define CTK_RBNODE_FLAG_SET(node, flag) 	(node?(((node->flags&flag)==flag)?TRUE:FALSE):FALSE)


CtkRBTree *_ctk_rbtree_new              (void);
void       _ctk_rbtree_free             (CtkRBTree              *tree);
void       _ctk_rbtree_remove           (CtkRBTree              *tree);
void       _ctk_rbtree_destroy          (CtkRBTree              *tree);
CtkRBNode *_ctk_rbtree_insert_before    (CtkRBTree              *tree,
					 CtkRBNode              *node,
					 gint                    height,
					 gboolean                valid);
CtkRBNode *_ctk_rbtree_insert_after     (CtkRBTree              *tree,
					 CtkRBNode              *node,
					 gint                    height,
					 gboolean                valid);
void       _ctk_rbtree_remove_node      (CtkRBTree              *tree,
					 CtkRBNode              *node);
gboolean   _ctk_rbtree_is_nil           (CtkRBNode              *node);
void       _ctk_rbtree_reorder          (CtkRBTree              *tree,
					 gint                   *new_order,
					 gint                    length);
gboolean   _ctk_rbtree_contains         (CtkRBTree              *tree,
                                         CtkRBTree              *potential_child);
CtkRBNode *_ctk_rbtree_find_count       (CtkRBTree              *tree,
					 gint                    count);
void       _ctk_rbtree_node_set_height  (CtkRBTree              *tree,
					 CtkRBNode              *node,
					 gint                    height);
void       _ctk_rbtree_node_mark_invalid(CtkRBTree              *tree,
					 CtkRBNode              *node);
void       _ctk_rbtree_node_mark_valid  (CtkRBTree              *tree,
					 CtkRBNode              *node);
void       _ctk_rbtree_column_invalid   (CtkRBTree              *tree);
void       _ctk_rbtree_mark_invalid     (CtkRBTree              *tree);
void       _ctk_rbtree_set_fixed_height (CtkRBTree              *tree,
					 gint                    height,
					 gboolean                mark_valid);
gint       _ctk_rbtree_node_find_offset (CtkRBTree              *tree,
					 CtkRBNode              *node);
guint      _ctk_rbtree_node_get_index   (CtkRBTree              *tree,
					 CtkRBNode              *node);
gboolean   _ctk_rbtree_find_index       (CtkRBTree              *tree,
					 guint                   index,
					 CtkRBTree             **new_tree,
					 CtkRBNode             **new_node);
gint       _ctk_rbtree_find_offset      (CtkRBTree              *tree,
					 gint                    offset,
					 CtkRBTree             **new_tree,
					 CtkRBNode             **new_node);
void       _ctk_rbtree_traverse         (CtkRBTree              *tree,
					 CtkRBNode              *node,
					 GTraverseType           order,
					 CtkRBTreeTraverseFunc   func,
					 gpointer                data);
CtkRBNode *_ctk_rbtree_first            (CtkRBTree              *tree);
CtkRBNode *_ctk_rbtree_next             (CtkRBTree              *tree,
					 CtkRBNode              *node);
CtkRBNode *_ctk_rbtree_prev             (CtkRBTree              *tree,
					 CtkRBNode              *node);
void       _ctk_rbtree_next_full        (CtkRBTree              *tree,
					 CtkRBNode              *node,
					 CtkRBTree             **new_tree,
					 CtkRBNode             **new_node);
void       _ctk_rbtree_prev_full        (CtkRBTree              *tree,
					 CtkRBNode              *node,
					 CtkRBTree             **new_tree,
					 CtkRBNode             **new_node);

gint       _ctk_rbtree_get_depth        (CtkRBTree              *tree);


G_END_DECLS


#endif /* __CTK_RBTREE_H__ */
