/* CTK+ - accessibility implementations
 * Copyright 2001 Sun Microsystems Inc.
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

#ifndef __CTK_TREE_VIEW_ACCESSIBLE_PRIVATE_H__
#define __CTK_TREE_VIEW_ACCESSIBLE_PRIVATE_H__

#include <ctk/a11y/ctktreeviewaccessible.h>

G_BEGIN_DECLS

/* called by treeview code */
void            _ctk_tree_view_accessible_reorder       (CtkTreeView       *treeview);
void            _ctk_tree_view_accessible_add           (CtkTreeView       *treeview,
                                                         CtkRBTree         *tree,
                                                         CtkRBNode         *node);
void            _ctk_tree_view_accessible_remove        (CtkTreeView       *treeview,
                                                         CtkRBTree         *tree,
                                                         CtkRBNode         *node);
void            _ctk_tree_view_accessible_changed       (CtkTreeView       *treeview,
                                                         CtkRBTree         *tree,
                                                         CtkRBNode         *node);

void            _ctk_tree_view_accessible_add_column    (CtkTreeView       *treeview,
                                                         CtkTreeViewColumn *column,
                                                         guint              id);
void            _ctk_tree_view_accessible_remove_column (CtkTreeView       *treeview,
                                                         CtkTreeViewColumn *column,
                                                         guint              id);
void            _ctk_tree_view_accessible_reorder_column
                                                        (CtkTreeView       *treeview,
                                                         CtkTreeViewColumn *column);
void            _ctk_tree_view_accessible_toggle_visibility
                                                        (CtkTreeView       *treeview,
                                                         CtkTreeViewColumn *column);
void            _ctk_tree_view_accessible_update_focus_column
                                                        (CtkTreeView       *treeview,
                                                         CtkTreeViewColumn *old_focus,
                                                         CtkTreeViewColumn *new_focus);

void            _ctk_tree_view_accessible_add_state     (CtkTreeView       *treeview,
                                                         CtkRBTree         *tree,
                                                         CtkRBNode         *node,
                                                         CtkCellRendererState state);
void            _ctk_tree_view_accessible_remove_state  (CtkTreeView       *treeview,
                                                         CtkRBTree         *tree,
                                                         CtkRBNode         *node,
                                                         CtkCellRendererState state);

G_END_DECLS

#endif /* __CTK_TREE_VIEW_ACCESSIBLE_PRIVATE_H__ */
