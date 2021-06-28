/* ctktreeprivate.h
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

#ifndef __CTK_TREE_PRIVATE_H__
#define __CTK_TREE_PRIVATE_H__


#include <ctk/ctktreeview.h>
#include <ctk/ctktreeselection.h>
#include <ctk/ctkrbtree.h>

G_BEGIN_DECLS

#define TREE_VIEW_DRAG_WIDTH 6

typedef enum
{
  CTK_TREE_SELECT_MODE_TOGGLE = 1 << 0,
  CTK_TREE_SELECT_MODE_EXTEND = 1 << 1
}
CtkTreeSelectMode;

/* functions that shouldn't be exported */
void         _ctk_tree_selection_internal_select_node (CtkTreeSelection  *selection,
						       CtkRBNode         *node,
						       CtkRBTree         *tree,
						       CtkTreePath       *path,
                                                       CtkTreeSelectMode  mode,
						       gboolean           override_browse_mode);
void         _ctk_tree_selection_emit_changed         (CtkTreeSelection  *selection);
gboolean     _ctk_tree_view_find_node                 (CtkTreeView       *tree_view,
						       CtkTreePath       *path,
						       CtkRBTree        **tree,
						       CtkRBNode        **node);
gboolean     _ctk_tree_view_get_cursor_node           (CtkTreeView       *tree_view,
						       CtkRBTree        **tree,
						       CtkRBNode        **node);
CtkTreePath *_ctk_tree_path_new_from_rbtree           (CtkRBTree         *tree,
						       CtkRBNode         *node);
void         _ctk_tree_view_queue_draw_node           (CtkTreeView       *tree_view,
						       CtkRBTree         *tree,
						       CtkRBNode         *node,
						       const CdkRectangle *clip_rect);

void         _ctk_tree_view_add_editable              (CtkTreeView       *tree_view,
                                                       CtkTreeViewColumn *column,
                                                       CtkTreePath       *path,
                                                       CtkCellEditable   *cell_editable,
                                                       CdkRectangle      *cell_area);
void         _ctk_tree_view_remove_editable           (CtkTreeView       *tree_view,
                                                       CtkTreeViewColumn *column,
                                                       CtkCellEditable   *cell_editable);

void       _ctk_tree_view_install_mark_rows_col_dirty (CtkTreeView *tree_view,
						       gboolean     install_handler);
void         _ctk_tree_view_column_autosize           (CtkTreeView       *tree_view,
						       CtkTreeViewColumn *column);
gint         _ctk_tree_view_get_header_height         (CtkTreeView       *tree_view);

void         _ctk_tree_view_get_row_separator_func    (CtkTreeView                 *tree_view,
						       CtkTreeViewRowSeparatorFunc *func,
						       gpointer                    *data);
CtkTreePath *_ctk_tree_view_get_anchor_path           (CtkTreeView                 *tree_view);
void         _ctk_tree_view_set_anchor_path           (CtkTreeView                 *tree_view,
						       CtkTreePath                 *anchor_path);
CtkRBTree *  _ctk_tree_view_get_rbtree                (CtkTreeView                 *tree_view);

CtkTreeViewColumn *_ctk_tree_view_get_focus_column    (CtkTreeView                 *tree_view);
void               _ctk_tree_view_set_focus_column    (CtkTreeView                 *tree_view,
						       CtkTreeViewColumn           *column);
CdkWindow         *_ctk_tree_view_get_header_window   (CtkTreeView                 *tree_view);


CtkTreeSelection* _ctk_tree_selection_new                (void);
CtkTreeSelection* _ctk_tree_selection_new_with_tree_view (CtkTreeView      *tree_view);
void              _ctk_tree_selection_set_tree_view      (CtkTreeSelection *selection,
                                                          CtkTreeView      *tree_view);
gboolean          _ctk_tree_selection_row_is_selectable  (CtkTreeSelection *selection,
							  CtkRBNode        *node,
							  CtkTreePath      *path);


void _ctk_tree_view_column_realize_button   (CtkTreeViewColumn *column);
void _ctk_tree_view_column_unrealize_button (CtkTreeViewColumn *column);
 
void _ctk_tree_view_column_set_tree_view    (CtkTreeViewColumn *column,
					     CtkTreeView       *tree_view);
gint _ctk_tree_view_column_request_width    (CtkTreeViewColumn *tree_column);
void _ctk_tree_view_column_allocate         (CtkTreeViewColumn *tree_column,
					     int                x_offset,
					     int                width);
void _ctk_tree_view_column_unset_model      (CtkTreeViewColumn *column,
					     CtkTreeModel      *old_model);
void _ctk_tree_view_column_unset_tree_view  (CtkTreeViewColumn *column);
void _ctk_tree_view_column_start_drag       (CtkTreeView       *tree_view,
					     CtkTreeViewColumn *column,
                                             CdkDevice         *device);
gboolean _ctk_tree_view_column_cell_event   (CtkTreeViewColumn  *tree_column,
					     CdkEvent           *event,
					     const CdkRectangle *cell_area,
					     guint               flags);
gboolean          _ctk_tree_view_column_has_editable_cell(CtkTreeViewColumn  *column);
CtkCellRenderer  *_ctk_tree_view_column_get_edited_cell  (CtkTreeViewColumn  *column);
CtkCellRenderer  *_ctk_tree_view_column_get_cell_at_pos  (CtkTreeViewColumn  *column,
                                                          CdkRectangle       *cell_area,
                                                          CdkRectangle       *background_area,
                                                          gint                x,
                                                          gint                y);
gboolean          _ctk_tree_view_column_is_blank_at_pos  (CtkTreeViewColumn  *column,
                                                          CdkRectangle       *cell_area,
                                                          CdkRectangle       *background_area,
                                                          gint                x,
                                                          gint                y);

void		  _ctk_tree_view_column_cell_render      (CtkTreeViewColumn  *tree_column,
							  cairo_t            *cr,
							  const CdkRectangle *background_area,
							  const CdkRectangle *cell_area,
							  guint               flags,
                                                          gboolean            draw_focus);
void		  _ctk_tree_view_column_cell_set_dirty	 (CtkTreeViewColumn  *tree_column,
							  gboolean            install_handler);
gboolean          _ctk_tree_view_column_cell_get_dirty   (CtkTreeViewColumn  *tree_column);
CdkWindow        *_ctk_tree_view_column_get_window       (CtkTreeViewColumn  *column);

void              _ctk_tree_view_column_push_padding          (CtkTreeViewColumn  *column,
							       gint                padding);
gint              _ctk_tree_view_column_get_requested_width   (CtkTreeViewColumn  *column);
gint              _ctk_tree_view_column_get_drag_x            (CtkTreeViewColumn  *column);
CtkCellAreaContext *_ctk_tree_view_column_get_context         (CtkTreeViewColumn  *column);


G_END_DECLS


#endif /* __CTK_TREE_PRIVATE_H__ */

