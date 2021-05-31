/* gtkrbtree.h
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

/* A Red-Black Tree implementation used specifically by GtkTreeView.
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
} GtkRBNodeColor;

typedef struct _GtkRBTree GtkRBTree;
typedef struct _GtkRBNode GtkRBNode;
typedef struct _GtkRBTreeView GtkRBTreeView;

typedef void (*GtkRBTreeTraverseFunc) (GtkRBTree  *tree,
                                       GtkRBNode  *node,
                                       gpointer  data);

struct _GtkRBTree
{
  GtkRBNode *root;
  GtkRBTree *parent_tree;
  GtkRBNode *parent_node;
};

struct _GtkRBNode
{
  guint flags : 14;

  /* count is the number of nodes beneath us, plus 1 for ourselves.
   * i.e. node->left->count + node->right->count + 1
   */
  gint count;

  GtkRBNode *left;
  GtkRBNode *right;
  GtkRBNode *parent;

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
  GtkRBTree *children;
};


#define CTK_RBNODE_GET_COLOR(node)		(node?(((node->flags&CTK_RBNODE_RED)==CTK_RBNODE_RED)?CTK_RBNODE_RED:CTK_RBNODE_BLACK):CTK_RBNODE_BLACK)
#define CTK_RBNODE_SET_COLOR(node,color) 	if((node->flags&color)!=color)node->flags=node->flags^(CTK_RBNODE_RED|CTK_RBNODE_BLACK)
#define CTK_RBNODE_GET_HEIGHT(node) 		(node->offset-(node->left->offset+node->right->offset+(node->children?node->children->root->offset:0)))
#define CTK_RBNODE_SET_FLAG(node, flag)   	G_STMT_START{ (node->flags|=flag); }G_STMT_END
#define CTK_RBNODE_UNSET_FLAG(node, flag) 	G_STMT_START{ (node->flags&=~(flag)); }G_STMT_END
#define CTK_RBNODE_FLAG_SET(node, flag) 	(node?(((node->flags&flag)==flag)?TRUE:FALSE):FALSE)


GtkRBTree *_ctk_rbtree_new              (void);
void       _ctk_rbtree_free             (GtkRBTree              *tree);
void       _ctk_rbtree_remove           (GtkRBTree              *tree);
void       _ctk_rbtree_destroy          (GtkRBTree              *tree);
GtkRBNode *_ctk_rbtree_insert_before    (GtkRBTree              *tree,
					 GtkRBNode              *node,
					 gint                    height,
					 gboolean                valid);
GtkRBNode *_ctk_rbtree_insert_after     (GtkRBTree              *tree,
					 GtkRBNode              *node,
					 gint                    height,
					 gboolean                valid);
void       _ctk_rbtree_remove_node      (GtkRBTree              *tree,
					 GtkRBNode              *node);
gboolean   _ctk_rbtree_is_nil           (GtkRBNode              *node);
void       _ctk_rbtree_reorder          (GtkRBTree              *tree,
					 gint                   *new_order,
					 gint                    length);
gboolean   _ctk_rbtree_contains         (GtkRBTree              *tree,
                                         GtkRBTree              *potential_child);
GtkRBNode *_ctk_rbtree_find_count       (GtkRBTree              *tree,
					 gint                    count);
void       _ctk_rbtree_node_set_height  (GtkRBTree              *tree,
					 GtkRBNode              *node,
					 gint                    height);
void       _ctk_rbtree_node_mark_invalid(GtkRBTree              *tree,
					 GtkRBNode              *node);
void       _ctk_rbtree_node_mark_valid  (GtkRBTree              *tree,
					 GtkRBNode              *node);
void       _ctk_rbtree_column_invalid   (GtkRBTree              *tree);
void       _ctk_rbtree_mark_invalid     (GtkRBTree              *tree);
void       _ctk_rbtree_set_fixed_height (GtkRBTree              *tree,
					 gint                    height,
					 gboolean                mark_valid);
gint       _ctk_rbtree_node_find_offset (GtkRBTree              *tree,
					 GtkRBNode              *node);
guint      _ctk_rbtree_node_get_index   (GtkRBTree              *tree,
					 GtkRBNode              *node);
gboolean   _ctk_rbtree_find_index       (GtkRBTree              *tree,
					 guint                   index,
					 GtkRBTree             **new_tree,
					 GtkRBNode             **new_node);
gint       _ctk_rbtree_find_offset      (GtkRBTree              *tree,
					 gint                    offset,
					 GtkRBTree             **new_tree,
					 GtkRBNode             **new_node);
void       _ctk_rbtree_traverse         (GtkRBTree              *tree,
					 GtkRBNode              *node,
					 GTraverseType           order,
					 GtkRBTreeTraverseFunc   func,
					 gpointer                data);
GtkRBNode *_ctk_rbtree_first            (GtkRBTree              *tree);
GtkRBNode *_ctk_rbtree_next             (GtkRBTree              *tree,
					 GtkRBNode              *node);
GtkRBNode *_ctk_rbtree_prev             (GtkRBTree              *tree,
					 GtkRBNode              *node);
void       _ctk_rbtree_next_full        (GtkRBTree              *tree,
					 GtkRBNode              *node,
					 GtkRBTree             **new_tree,
					 GtkRBNode             **new_node);
void       _ctk_rbtree_prev_full        (GtkRBTree              *tree,
					 GtkRBNode              *node,
					 GtkRBTree             **new_tree,
					 GtkRBNode             **new_node);

gint       _ctk_rbtree_get_depth        (GtkRBTree              *tree);


G_END_DECLS


#endif /* __CTK_RBTREE_H__ */
