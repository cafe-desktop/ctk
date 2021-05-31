/* ctktreeview.h
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

#ifndef __CTK_TREE_VIEW_H__
#define __CTK_TREE_VIEW_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>
#include <ctk/ctktreemodel.h>
#include <ctk/ctktreeviewcolumn.h>
#include <ctk/ctkdnd.h>
#include <ctk/ctkentry.h>

G_BEGIN_DECLS

/**
 * CtkTreeViewDropPosition:
 * @CTK_TREE_VIEW_DROP_BEFORE: dropped row is inserted before
 * @CTK_TREE_VIEW_DROP_AFTER: dropped row is inserted after
 * @CTK_TREE_VIEW_DROP_INTO_OR_BEFORE: dropped row becomes a child or is inserted before
 * @CTK_TREE_VIEW_DROP_INTO_OR_AFTER: dropped row becomes a child or is inserted after
 *
 * An enum for determining where a dropped row goes.
 */
typedef enum
{
  /* drop before/after this row */
  CTK_TREE_VIEW_DROP_BEFORE,
  CTK_TREE_VIEW_DROP_AFTER,
  /* drop as a child of this row (with fallback to before or after
   * if into is not possible)
   */
  CTK_TREE_VIEW_DROP_INTO_OR_BEFORE,
  CTK_TREE_VIEW_DROP_INTO_OR_AFTER
} CtkTreeViewDropPosition;

#define CTK_TYPE_TREE_VIEW		(ctk_tree_view_get_type ())
#define CTK_TREE_VIEW(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TREE_VIEW, CtkTreeView))
#define CTK_TREE_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TREE_VIEW, CtkTreeViewClass))
#define CTK_IS_TREE_VIEW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TREE_VIEW))
#define CTK_IS_TREE_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TREE_VIEW))
#define CTK_TREE_VIEW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TREE_VIEW, CtkTreeViewClass))

typedef struct _CtkTreeView           CtkTreeView;
typedef struct _CtkTreeViewClass      CtkTreeViewClass;
typedef struct _CtkTreeViewPrivate    CtkTreeViewPrivate;
typedef struct _CtkTreeSelection      CtkTreeSelection;
typedef struct _CtkTreeSelectionClass CtkTreeSelectionClass;

struct _CtkTreeView
{
  CtkContainer parent;

  /*< private >*/
  CtkTreeViewPrivate *priv;
};

struct _CtkTreeViewClass
{
  CtkContainerClass parent_class;

  void     (* row_activated)              (CtkTreeView       *tree_view,
				           CtkTreePath       *path,
					   CtkTreeViewColumn *column);
  gboolean (* test_expand_row)            (CtkTreeView       *tree_view,
				           CtkTreeIter       *iter,
				           CtkTreePath       *path);
  gboolean (* test_collapse_row)          (CtkTreeView       *tree_view,
				           CtkTreeIter       *iter,
				           CtkTreePath       *path);
  void     (* row_expanded)               (CtkTreeView       *tree_view,
				           CtkTreeIter       *iter,
				           CtkTreePath       *path);
  void     (* row_collapsed)              (CtkTreeView       *tree_view,
				           CtkTreeIter       *iter,
				           CtkTreePath       *path);
  void     (* columns_changed)            (CtkTreeView       *tree_view);
  void     (* cursor_changed)             (CtkTreeView       *tree_view);

  /* Key Binding signals */
  gboolean (* move_cursor)                (CtkTreeView       *tree_view,
				           CtkMovementStep    step,
				           gint               count);
  gboolean (* select_all)                 (CtkTreeView       *tree_view);
  gboolean (* unselect_all)               (CtkTreeView       *tree_view);
  gboolean (* select_cursor_row)          (CtkTreeView       *tree_view,
					   gboolean           start_editing);
  gboolean (* toggle_cursor_row)          (CtkTreeView       *tree_view);
  gboolean (* expand_collapse_cursor_row) (CtkTreeView       *tree_view,
					   gboolean           logical,
					   gboolean           expand,
					   gboolean           open_all);
  gboolean (* select_cursor_parent)       (CtkTreeView       *tree_view);
  gboolean (* start_interactive_search)   (CtkTreeView       *tree_view);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
  void (*_ctk_reserved5) (void);
  void (*_ctk_reserved6) (void);
  void (*_ctk_reserved7) (void);
  void (*_ctk_reserved8) (void);
};

/**
 * CtkTreeViewColumnDropFunc:
 * @tree_view: A #CtkTreeView
 * @column: The #CtkTreeViewColumn being dragged
 * @prev_column: A #CtkTreeViewColumn on one side of @column
 * @next_column: A #CtkTreeViewColumn on the other side of @column
 * @data: (closure): user data
 *
 * Function type for determining whether @column can be dropped in a
 * particular spot (as determined by @prev_column and @next_column).  In
 * left to right locales, @prev_column is on the left of the potential drop
 * spot, and @next_column is on the right.  In right to left mode, this is
 * reversed.  This function should return %TRUE if the spot is a valid drop
 * spot.  Please note that returning %TRUE does not actually indicate that
 * the column drop was made, but is meant only to indicate a possible drop
 * spot to the user.
 *
 * Returns: %TRUE, if @column can be dropped in this spot
 */
typedef gboolean (* CtkTreeViewColumnDropFunc) (CtkTreeView             *tree_view,
						CtkTreeViewColumn       *column,
						CtkTreeViewColumn       *prev_column,
						CtkTreeViewColumn       *next_column,
						gpointer                 data);

/**
 * CtkTreeViewMappingFunc:
 * @tree_view: A #CtkTreeView
 * @path: The path that’s expanded
 * @user_data: user data
 *
 * Function used for ctk_tree_view_map_expanded_rows().
 */
typedef void     (* CtkTreeViewMappingFunc)    (CtkTreeView             *tree_view,
						CtkTreePath             *path,
						gpointer                 user_data);

/**
 * CtkTreeViewSearchEqualFunc:
 * @model: the #CtkTreeModel being searched
 * @column: the search column set by ctk_tree_view_set_search_column()
 * @key: the key string to compare with
 * @iter: a #CtkTreeIter pointing the row of @model that should be compared
 *  with @key.
 * @search_data: (closure): user data from ctk_tree_view_set_search_equal_func()
 *
 * A function used for checking whether a row in @model matches
 * a search key string entered by the user. Note the return value
 * is reversed from what you would normally expect, though it
 * has some similarity to strcmp() returning 0 for equal strings.
 *
 * Returns: %FALSE if the row matches, %TRUE otherwise.
 */
typedef gboolean (*CtkTreeViewSearchEqualFunc) (CtkTreeModel            *model,
						gint                     column,
						const gchar             *key,
						CtkTreeIter             *iter,
						gpointer                 search_data);

/**
 * CtkTreeViewRowSeparatorFunc:
 * @model: the #CtkTreeModel
 * @iter: a #CtkTreeIter pointing at a row in @model
 * @data: (closure): user data
 *
 * Function type for determining whether the row pointed to by @iter should
 * be rendered as a separator. A common way to implement this is to have a
 * boolean column in the model, whose values the #CtkTreeViewRowSeparatorFunc
 * returns.
 *
 * Returns: %TRUE if the row is a separator
 */
typedef gboolean (*CtkTreeViewRowSeparatorFunc) (CtkTreeModel      *model,
						 CtkTreeIter       *iter,
						 gpointer           data);
typedef void     (*CtkTreeViewSearchPositionFunc) (CtkTreeView  *tree_view,
						   CtkWidget    *search_dialog,
						   gpointer      user_data);


/* Creators */
GDK_AVAILABLE_IN_ALL
GType                  ctk_tree_view_get_type                      (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkWidget             *ctk_tree_view_new                           (void);
GDK_AVAILABLE_IN_ALL
CtkWidget             *ctk_tree_view_new_with_model                (CtkTreeModel              *model);

/* Accessors */
GDK_AVAILABLE_IN_ALL
CtkTreeModel          *ctk_tree_view_get_model                     (CtkTreeView               *tree_view);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_set_model                     (CtkTreeView               *tree_view,
								    CtkTreeModel              *model);
GDK_AVAILABLE_IN_ALL
CtkTreeSelection      *ctk_tree_view_get_selection                 (CtkTreeView               *tree_view);

GDK_DEPRECATED_IN_3_0_FOR(ctk_scrollable_get_hadjustment)
CtkAdjustment         *ctk_tree_view_get_hadjustment               (CtkTreeView               *tree_view);
GDK_DEPRECATED_IN_3_0_FOR(ctk_scrollable_set_hadjustment)
void                   ctk_tree_view_set_hadjustment               (CtkTreeView               *tree_view,
								    CtkAdjustment             *adjustment);
GDK_DEPRECATED_IN_3_0_FOR(ctk_scrollable_get_vadjustment)
CtkAdjustment         *ctk_tree_view_get_vadjustment               (CtkTreeView               *tree_view);
GDK_DEPRECATED_IN_3_0_FOR(ctk_scrollable_set_vadjustment)
void                   ctk_tree_view_set_vadjustment               (CtkTreeView               *tree_view,
								    CtkAdjustment             *adjustment);


GDK_AVAILABLE_IN_ALL
gboolean               ctk_tree_view_get_headers_visible           (CtkTreeView               *tree_view);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_set_headers_visible           (CtkTreeView               *tree_view,
								    gboolean                   headers_visible);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_columns_autosize              (CtkTreeView               *tree_view);
GDK_AVAILABLE_IN_ALL
gboolean               ctk_tree_view_get_headers_clickable         (CtkTreeView *tree_view);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_set_headers_clickable         (CtkTreeView               *tree_view,
								    gboolean                   setting);
GDK_DEPRECATED_IN_3_14
void                   ctk_tree_view_set_rules_hint                (CtkTreeView               *tree_view,
								    gboolean                   setting);
GDK_DEPRECATED_IN_3_14
gboolean               ctk_tree_view_get_rules_hint                (CtkTreeView               *tree_view);
GDK_AVAILABLE_IN_3_8
gboolean               ctk_tree_view_get_activate_on_single_click  (CtkTreeView               *tree_view);
GDK_AVAILABLE_IN_3_8
void                   ctk_tree_view_set_activate_on_single_click  (CtkTreeView               *tree_view,
								    gboolean                   single);

/* Column funtions */
GDK_AVAILABLE_IN_ALL
gint                   ctk_tree_view_append_column                 (CtkTreeView               *tree_view,
								    CtkTreeViewColumn         *column);
GDK_AVAILABLE_IN_ALL
gint                   ctk_tree_view_remove_column                 (CtkTreeView               *tree_view,
								    CtkTreeViewColumn         *column);
GDK_AVAILABLE_IN_ALL
gint                   ctk_tree_view_insert_column                 (CtkTreeView               *tree_view,
								    CtkTreeViewColumn         *column,
								    gint                       position);
GDK_AVAILABLE_IN_ALL
gint                   ctk_tree_view_insert_column_with_attributes (CtkTreeView               *tree_view,
								    gint                       position,
								    const gchar               *title,
								    CtkCellRenderer           *cell,
								    ...) G_GNUC_NULL_TERMINATED;
GDK_AVAILABLE_IN_ALL
gint                   ctk_tree_view_insert_column_with_data_func  (CtkTreeView               *tree_view,
								    gint                       position,
								    const gchar               *title,
								    CtkCellRenderer           *cell,
                                                                    CtkTreeCellDataFunc        func,
                                                                    gpointer                   data,
                                                                    GDestroyNotify             dnotify);

GDK_AVAILABLE_IN_3_4
guint                  ctk_tree_view_get_n_columns                 (CtkTreeView               *tree_view);
GDK_AVAILABLE_IN_ALL
CtkTreeViewColumn     *ctk_tree_view_get_column                    (CtkTreeView               *tree_view,
								    gint                       n);
GDK_AVAILABLE_IN_ALL
GList                 *ctk_tree_view_get_columns                   (CtkTreeView               *tree_view);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_move_column_after             (CtkTreeView               *tree_view,
								    CtkTreeViewColumn         *column,
								    CtkTreeViewColumn         *base_column);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_set_expander_column           (CtkTreeView               *tree_view,
								    CtkTreeViewColumn         *column);
GDK_AVAILABLE_IN_ALL
CtkTreeViewColumn     *ctk_tree_view_get_expander_column           (CtkTreeView               *tree_view);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_set_column_drag_function      (CtkTreeView               *tree_view,
								    CtkTreeViewColumnDropFunc  func,
								    gpointer                   user_data,
								    GDestroyNotify             destroy);

/* Actions */
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_scroll_to_point               (CtkTreeView               *tree_view,
								    gint                       tree_x,
								    gint                       tree_y);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_scroll_to_cell                (CtkTreeView               *tree_view,
								    CtkTreePath               *path,
								    CtkTreeViewColumn         *column,
								    gboolean                   use_align,
								    gfloat                     row_align,
								    gfloat                     col_align);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_row_activated                 (CtkTreeView               *tree_view,
								    CtkTreePath               *path,
								    CtkTreeViewColumn         *column);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_expand_all                    (CtkTreeView               *tree_view);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_collapse_all                  (CtkTreeView               *tree_view);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_expand_to_path                (CtkTreeView               *tree_view,
								    CtkTreePath               *path);
GDK_AVAILABLE_IN_ALL
gboolean               ctk_tree_view_expand_row                    (CtkTreeView               *tree_view,
								    CtkTreePath               *path,
								    gboolean                   open_all);
GDK_AVAILABLE_IN_ALL
gboolean               ctk_tree_view_collapse_row                  (CtkTreeView               *tree_view,
								    CtkTreePath               *path);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_map_expanded_rows             (CtkTreeView               *tree_view,
								    CtkTreeViewMappingFunc     func,
								    gpointer                   data);
GDK_AVAILABLE_IN_ALL
gboolean               ctk_tree_view_row_expanded                  (CtkTreeView               *tree_view,
								    CtkTreePath               *path);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_set_reorderable               (CtkTreeView               *tree_view,
								    gboolean                   reorderable);
GDK_AVAILABLE_IN_ALL
gboolean               ctk_tree_view_get_reorderable               (CtkTreeView               *tree_view);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_set_cursor                    (CtkTreeView               *tree_view,
								    CtkTreePath               *path,
								    CtkTreeViewColumn         *focus_column,
								    gboolean                   start_editing);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_set_cursor_on_cell            (CtkTreeView               *tree_view,
								    CtkTreePath               *path,
								    CtkTreeViewColumn         *focus_column,
								    CtkCellRenderer           *focus_cell,
								    gboolean                   start_editing);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_get_cursor                    (CtkTreeView               *tree_view,
								    CtkTreePath              **path,
								    CtkTreeViewColumn        **focus_column);


/* Layout information */
GDK_AVAILABLE_IN_ALL
GdkWindow             *ctk_tree_view_get_bin_window                (CtkTreeView               *tree_view);
GDK_AVAILABLE_IN_ALL
gboolean               ctk_tree_view_get_path_at_pos               (CtkTreeView               *tree_view,
								    gint                       x,
								    gint                       y,
								    CtkTreePath              **path,
								    CtkTreeViewColumn        **column,
								    gint                      *cell_x,
								    gint                      *cell_y);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_get_cell_area                 (CtkTreeView               *tree_view,
								    CtkTreePath               *path,
								    CtkTreeViewColumn         *column,
								    GdkRectangle              *rect);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_get_background_area           (CtkTreeView               *tree_view,
								    CtkTreePath               *path,
								    CtkTreeViewColumn         *column,
								    GdkRectangle              *rect);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_get_visible_rect              (CtkTreeView               *tree_view,
								    GdkRectangle              *visible_rect);
GDK_AVAILABLE_IN_ALL
gboolean               ctk_tree_view_get_visible_range             (CtkTreeView               *tree_view,
								    CtkTreePath              **start_path,
								    CtkTreePath              **end_path);
GDK_AVAILABLE_IN_ALL
gboolean               ctk_tree_view_is_blank_at_pos               (CtkTreeView               *tree_view,
                                                                    gint                       x,
                                                                    gint                       y,
                                                                    CtkTreePath              **path,
                                                                    CtkTreeViewColumn        **column,
                                                                    gint                      *cell_x,
                                                                    gint                      *cell_y);

/* Drag-and-Drop support */
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_enable_model_drag_source      (CtkTreeView               *tree_view,
								    GdkModifierType            start_button_mask,
								    const CtkTargetEntry      *targets,
								    gint                       n_targets,
								    GdkDragAction              actions);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_enable_model_drag_dest        (CtkTreeView               *tree_view,
								    const CtkTargetEntry      *targets,
								    gint                       n_targets,
								    GdkDragAction              actions);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_unset_rows_drag_source        (CtkTreeView               *tree_view);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_unset_rows_drag_dest          (CtkTreeView               *tree_view);


/* These are useful to implement your own custom stuff. */
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_set_drag_dest_row             (CtkTreeView               *tree_view,
								    CtkTreePath               *path,
								    CtkTreeViewDropPosition    pos);
GDK_AVAILABLE_IN_ALL
void                   ctk_tree_view_get_drag_dest_row             (CtkTreeView               *tree_view,
								    CtkTreePath              **path,
								    CtkTreeViewDropPosition   *pos);
GDK_AVAILABLE_IN_ALL
gboolean               ctk_tree_view_get_dest_row_at_pos           (CtkTreeView               *tree_view,
								    gint                       drag_x,
								    gint                       drag_y,
								    CtkTreePath              **path,
								    CtkTreeViewDropPosition   *pos);
GDK_AVAILABLE_IN_ALL
cairo_surface_t       *ctk_tree_view_create_row_drag_icon          (CtkTreeView               *tree_view,
								    CtkTreePath               *path);

/* Interactive search */
GDK_AVAILABLE_IN_ALL
void                       ctk_tree_view_set_enable_search     (CtkTreeView                *tree_view,
								gboolean                    enable_search);
GDK_AVAILABLE_IN_ALL
gboolean                   ctk_tree_view_get_enable_search     (CtkTreeView                *tree_view);
GDK_AVAILABLE_IN_ALL
gint                       ctk_tree_view_get_search_column     (CtkTreeView                *tree_view);
GDK_AVAILABLE_IN_ALL
void                       ctk_tree_view_set_search_column     (CtkTreeView                *tree_view,
								gint                        column);
GDK_AVAILABLE_IN_ALL
CtkTreeViewSearchEqualFunc ctk_tree_view_get_search_equal_func (CtkTreeView                *tree_view);
GDK_AVAILABLE_IN_ALL
void                       ctk_tree_view_set_search_equal_func (CtkTreeView                *tree_view,
								CtkTreeViewSearchEqualFunc  search_equal_func,
								gpointer                    search_user_data,
								GDestroyNotify              search_destroy);

GDK_AVAILABLE_IN_ALL
CtkEntry                     *ctk_tree_view_get_search_entry         (CtkTreeView                   *tree_view);
GDK_AVAILABLE_IN_ALL
void                          ctk_tree_view_set_search_entry         (CtkTreeView                   *tree_view,
								      CtkEntry                      *entry);
GDK_AVAILABLE_IN_ALL
CtkTreeViewSearchPositionFunc ctk_tree_view_get_search_position_func (CtkTreeView                   *tree_view);
GDK_AVAILABLE_IN_ALL
void                          ctk_tree_view_set_search_position_func (CtkTreeView                   *tree_view,
								      CtkTreeViewSearchPositionFunc  func,
								      gpointer                       data,
								      GDestroyNotify                 destroy);

/* Convert between the different coordinate systems */
GDK_AVAILABLE_IN_ALL
void ctk_tree_view_convert_widget_to_tree_coords       (CtkTreeView *tree_view,
							gint         wx,
							gint         wy,
							gint        *tx,
							gint        *ty);
GDK_AVAILABLE_IN_ALL
void ctk_tree_view_convert_tree_to_widget_coords       (CtkTreeView *tree_view,
							gint         tx,
							gint         ty,
							gint        *wx,
							gint        *wy);
GDK_AVAILABLE_IN_ALL
void ctk_tree_view_convert_widget_to_bin_window_coords (CtkTreeView *tree_view,
							gint         wx,
							gint         wy,
							gint        *bx,
							gint        *by);
GDK_AVAILABLE_IN_ALL
void ctk_tree_view_convert_bin_window_to_widget_coords (CtkTreeView *tree_view,
							gint         bx,
							gint         by,
							gint        *wx,
							gint        *wy);
GDK_AVAILABLE_IN_ALL
void ctk_tree_view_convert_tree_to_bin_window_coords   (CtkTreeView *tree_view,
							gint         tx,
							gint         ty,
							gint        *bx,
							gint        *by);
GDK_AVAILABLE_IN_ALL
void ctk_tree_view_convert_bin_window_to_tree_coords   (CtkTreeView *tree_view,
							gint         bx,
							gint         by,
							gint        *tx,
							gint        *ty);

/* This function should really never be used.  It is just for use by ATK.
 */
typedef void (* CtkTreeDestroyCountFunc)  (CtkTreeView             *tree_view,
					   CtkTreePath             *path,
					   gint                     children,
					   gpointer                 user_data);
GDK_DEPRECATED_IN_3_4
void ctk_tree_view_set_destroy_count_func (CtkTreeView             *tree_view,
					   CtkTreeDestroyCountFunc  func,
					   gpointer                 data,
					   GDestroyNotify           destroy);

GDK_AVAILABLE_IN_ALL
void     ctk_tree_view_set_fixed_height_mode (CtkTreeView          *tree_view,
					      gboolean              enable);
GDK_AVAILABLE_IN_ALL
gboolean ctk_tree_view_get_fixed_height_mode (CtkTreeView          *tree_view);
GDK_AVAILABLE_IN_ALL
void     ctk_tree_view_set_hover_selection   (CtkTreeView          *tree_view,
					      gboolean              hover);
GDK_AVAILABLE_IN_ALL
gboolean ctk_tree_view_get_hover_selection   (CtkTreeView          *tree_view);
GDK_AVAILABLE_IN_ALL
void     ctk_tree_view_set_hover_expand      (CtkTreeView          *tree_view,
					      gboolean              expand);
GDK_AVAILABLE_IN_ALL
gboolean ctk_tree_view_get_hover_expand      (CtkTreeView          *tree_view);
GDK_AVAILABLE_IN_ALL
void     ctk_tree_view_set_rubber_banding    (CtkTreeView          *tree_view,
					      gboolean              enable);
GDK_AVAILABLE_IN_ALL
gboolean ctk_tree_view_get_rubber_banding    (CtkTreeView          *tree_view);

GDK_AVAILABLE_IN_ALL
gboolean ctk_tree_view_is_rubber_banding_active (CtkTreeView       *tree_view);

GDK_AVAILABLE_IN_ALL
CtkTreeViewRowSeparatorFunc ctk_tree_view_get_row_separator_func (CtkTreeView               *tree_view);
GDK_AVAILABLE_IN_ALL
void                        ctk_tree_view_set_row_separator_func (CtkTreeView                *tree_view,
								  CtkTreeViewRowSeparatorFunc func,
								  gpointer                    data,
								  GDestroyNotify              destroy);

GDK_AVAILABLE_IN_ALL
CtkTreeViewGridLines        ctk_tree_view_get_grid_lines         (CtkTreeView                *tree_view);
GDK_AVAILABLE_IN_ALL
void                        ctk_tree_view_set_grid_lines         (CtkTreeView                *tree_view,
								  CtkTreeViewGridLines        grid_lines);
GDK_AVAILABLE_IN_ALL
gboolean                    ctk_tree_view_get_enable_tree_lines  (CtkTreeView                *tree_view);
GDK_AVAILABLE_IN_ALL
void                        ctk_tree_view_set_enable_tree_lines  (CtkTreeView                *tree_view,
								  gboolean                    enabled);
GDK_AVAILABLE_IN_ALL
void                        ctk_tree_view_set_show_expanders     (CtkTreeView                *tree_view,
								  gboolean                    enabled);
GDK_AVAILABLE_IN_ALL
gboolean                    ctk_tree_view_get_show_expanders     (CtkTreeView                *tree_view);
GDK_AVAILABLE_IN_ALL
void                        ctk_tree_view_set_level_indentation  (CtkTreeView                *tree_view,
								  gint                        indentation);
GDK_AVAILABLE_IN_ALL
gint                        ctk_tree_view_get_level_indentation  (CtkTreeView                *tree_view);

/* Convenience functions for setting tooltips */
GDK_AVAILABLE_IN_ALL
void          ctk_tree_view_set_tooltip_row    (CtkTreeView       *tree_view,
						CtkTooltip        *tooltip,
						CtkTreePath       *path);
GDK_AVAILABLE_IN_ALL
void          ctk_tree_view_set_tooltip_cell   (CtkTreeView       *tree_view,
						CtkTooltip        *tooltip,
						CtkTreePath       *path,
						CtkTreeViewColumn *column,
						CtkCellRenderer   *cell);
GDK_AVAILABLE_IN_ALL
gboolean      ctk_tree_view_get_tooltip_context(CtkTreeView       *tree_view,
						gint              *x,
						gint              *y,
						gboolean           keyboard_tip,
						CtkTreeModel     **model,
						CtkTreePath      **path,
						CtkTreeIter       *iter);
GDK_AVAILABLE_IN_ALL
void          ctk_tree_view_set_tooltip_column (CtkTreeView       *tree_view,
					        gint               column);
GDK_AVAILABLE_IN_ALL
gint          ctk_tree_view_get_tooltip_column (CtkTreeView       *tree_view);

G_END_DECLS


#endif /* __CTK_TREE_VIEW_H__ */
