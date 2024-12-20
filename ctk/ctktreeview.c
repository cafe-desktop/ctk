/* ctktreeview.c
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

#include <math.h>
#include <string.h>

#include "ctktreeview.h"

#include "ctkadjustmentprivate.h"
#include "ctkcssnumbervalueprivate.h"
#include "ctkrbtree.h"
#include "ctktreednd.h"
#include "ctktreeprivate.h"
#include "ctkcellrenderer.h"
#include "ctkmarshalers.h"
#include "ctkbuildable.h"
#include "ctkbutton.h"
#include "ctklabel.h"
#include "ctkbox.h"
#include "ctkintl.h"
#include "ctkbindings.h"
#include "ctkcontainer.h"
#include "ctkentry.h"
#include "ctkframe.h"
#include "ctkmain.h"
#include "ctktreemodelsort.h"
#include "ctktooltip.h"
#include "ctkscrollable.h"
#include "ctkcelllayout.h"
#include "ctkprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkentryprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkcssrgbavalueprivate.h"
#include "ctktypebuiltins.h"
#include "ctkmain.h"
#include "ctksettingsprivate.h"
#include "ctkwidgetpath.h"
#include "ctkpixelcacheprivate.h"
#include "a11y/ctktreeviewaccessibleprivate.h"


/**
 * SECTION:ctktreeview
 * @Short_description: A widget for displaying both trees and lists
 * @Title: CtkTreeView
 * @See_also: #CtkTreeViewColumn, #CtkTreeSelection, #CtkTreeModel,
 *   [CtkTreeView drag-and-drop][ctk3-CtkTreeView-drag-and-drop],
 *   #CtkTreeSortable, #CtkTreeModelSort, #CtkListStore, #CtkTreeStore,
 *   #CtkCellRenderer, #CtkCellEditable, #CtkCellRendererPixbuf,
 *   #CtkCellRendererText, #CtkCellRendererToggle
 *
 * Widget that displays any object that implements the #CtkTreeModel interface.
 *
 * Please refer to the
 * [tree widget conceptual overview][TreeWidget]
 * for an overview of all the objects and data types related
 * to the tree widget and how they work together.
 *
 * Several different coordinate systems are exposed in the CtkTreeView API.
 * These are:
 *
 * ![](tree-view-coordinates.png)
 *
 * Coordinate systems in CtkTreeView API:
 *
 * - Widget coordinates: Coordinates relative to the widget (usually `widget->window`).
 *
 * - Bin window coordinates: Coordinates relative to the window that CtkTreeView renders to.
 *
 * - Tree coordinates: Coordinates relative to the entire scrollable area of CtkTreeView. These
 *   coordinates start at (0, 0) for row 0 of the tree.
 *
 * Several functions are available for converting between the different
 * coordinate systems.  The most common translations are between widget and bin
 * window coordinates and between bin window and tree coordinates. For the
 * former you can use ctk_tree_view_convert_widget_to_bin_window_coords()
 * (and vice versa), for the latter ctk_tree_view_convert_bin_window_to_tree_coords()
 * (and vice versa).
 *
 * # CtkTreeView as CtkBuildable
 *
 * The CtkTreeView implementation of the CtkBuildable interface accepts
 * #CtkTreeViewColumn objects as <child> elements and exposes the internal
 * #CtkTreeSelection in UI definitions.
 *
 * An example of a UI definition fragment with CtkTreeView:
 * |[
 * <object class="CtkTreeView" id="treeview">
 *   <property name="model">liststore1</property>
 *   <child>
 *     <object class="CtkTreeViewColumn" id="test-column">
 *       <property name="title">Test</property>
 *       <child>
 *         <object class="CtkCellRendererText" id="test-renderer"/>
 *         <attributes>
 *           <attribute name="text">1</attribute>
 *         </attributes>
 *       </child>
 *     </object>
 *   </child>
 *   <child internal-child="selection">
 *     <object class="CtkTreeSelection" id="selection">
 *       <signal name="changed" handler="on_treeview_selection_changed"/>
 *     </object>
 *   </child>
 * </object>
 * ]|
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * treeview.view
 * ├── header
 * │   ├── <column header>
 * ┊   ┊
 * │   ╰── <column header>
 * │
 * ╰── [rubberband]
 * ]|
 *
 * CtkTreeView has a main CSS node with name treeview and style class .view.
 * It has a subnode with name header, which is the parent for all the column
 * header widgets' CSS nodes.
 * For rubberband selection, a subnode with name rubberband is used.
 */

enum
{
  DRAG_COLUMN_WINDOW_STATE_UNSET = 0,
  DRAG_COLUMN_WINDOW_STATE_ORIGINAL = 1,
  DRAG_COLUMN_WINDOW_STATE_ARROW = 2,
  DRAG_COLUMN_WINDOW_STATE_ARROW_LEFT = 3,
  DRAG_COLUMN_WINDOW_STATE_ARROW_RIGHT = 4
};

enum
{
  RUBBER_BAND_OFF = 0,
  RUBBER_BAND_MAYBE_START = 1,
  RUBBER_BAND_ACTIVE = 2
};

typedef enum {
  CLEAR_AND_SELECT = (1 << 0),
  CLAMP_NODE       = (1 << 1),
  CURSOR_INVALID   = (1 << 2)
} SetCursorFlags;

 /* This lovely little value is used to determine how far away from the title bar
  * you can move the mouse and still have a column drag work.
  */
#define TREE_VIEW_COLUMN_DRAG_DEAD_MULTIPLIER(tree_view) (10*ctk_tree_view_get_effective_header_height(tree_view))

#ifdef __GNUC__

#define TREE_VIEW_INTERNAL_ASSERT(expr, ret)     G_STMT_START{          \
     if (!(expr))                                                       \
       {                                                                \
         g_log (G_LOG_DOMAIN,                                           \
                G_LOG_LEVEL_CRITICAL,                                   \
		"%s (%s): assertion `%s' failed.\n"                     \
	        "There is a disparity between the internal view of the CtkTreeView,\n"    \
		"and the CtkTreeModel.  This generally means that the model has changed\n"\
		"without letting the view know.  Any display from now on is likely to\n"  \
		"be incorrect.\n",                                                        \
                G_STRLOC,                                               \
                G_STRFUNC,                                              \
                #expr);                                                 \
         return ret;                                                    \
       };                               }G_STMT_END

#define TREE_VIEW_INTERNAL_ASSERT_VOID(expr)     G_STMT_START{          \
     if (!(expr))                                                       \
       {                                                                \
         g_log (G_LOG_DOMAIN,                                           \
                G_LOG_LEVEL_CRITICAL,                                   \
		"%s (%s): assertion `%s' failed.\n"                     \
	        "There is a disparity between the internal view of the CtkTreeView,\n"    \
		"and the CtkTreeModel.  This generally means that the model has changed\n"\
		"without letting the view know.  Any display from now on is likely to\n"  \
		"be incorrect.\n",                                                        \
                G_STRLOC,                                               \
                G_STRFUNC,                                              \
                #expr);                                                 \
         return;                                                        \
       };                               }G_STMT_END

#else

#define TREE_VIEW_INTERNAL_ASSERT(expr, ret)     G_STMT_START{          \
     if (!(expr))                                                       \
       {                                                                \
         g_log (G_LOG_DOMAIN,                                           \
                G_LOG_LEVEL_CRITICAL,                                   \
		"file %s: line %d: assertion `%s' failed.\n"       \
	        "There is a disparity between the internal view of the CtkTreeView,\n"    \
		"and the CtkTreeModel.  This generally means that the model has changed\n"\
		"without letting the view know.  Any display from now on is likely to\n"  \
		"be incorrect.\n",                                                        \
                __FILE__,                                               \
                __LINE__,                                               \
                #expr);                                                 \
         return ret;                                                    \
       };                               }G_STMT_END

#define TREE_VIEW_INTERNAL_ASSERT_VOID(expr)     G_STMT_START{          \
     if (!(expr))                                                       \
       {                                                                \
         g_log (G_LOG_DOMAIN,                                           \
                G_LOG_LEVEL_CRITICAL,                                   \
		"file %s: line %d: assertion '%s' failed.\n"            \
	        "There is a disparity between the internal view of the CtkTreeView,\n"    \
		"and the CtkTreeModel.  This generally means that the model has changed\n"\
		"without letting the view know.  Any display from now on is likely to\n"  \
		"be incorrect.\n",                                                        \
                __FILE__,                                               \
                __LINE__,                                               \
                #expr);                                                 \
         return;                                                        \
       };                               }G_STMT_END
#endif

#define CTK_TREE_VIEW_PRIORITY_VALIDATE (CDK_PRIORITY_REDRAW + 5)
#define CTK_TREE_VIEW_PRIORITY_SCROLL_SYNC (CTK_TREE_VIEW_PRIORITY_VALIDATE + 2)
/* 3/5 of cdkframeclockidle.c's FRAME_INTERVAL (16667 microsecs) */
#define CTK_TREE_VIEW_TIME_MS_PER_IDLE 10
#define SCROLL_EDGE_SIZE 15
#define CTK_TREE_VIEW_SEARCH_DIALOG_TIMEOUT 5000
#define AUTO_EXPAND_TIMEOUT 500

/* Translate from bin_window coordinates to rbtree (tree coordinates) and
 * vice versa.
 */
#define TREE_WINDOW_Y_TO_RBTREE_Y(tree_view,y) ((y) + tree_view->priv->dy)
#define RBTREE_Y_TO_TREE_WINDOW_Y(tree_view,y) ((y) - tree_view->priv->dy)

typedef struct _CtkTreeViewColumnReorder CtkTreeViewColumnReorder;
struct _CtkTreeViewColumnReorder
{
  gint left_align;
  gint right_align;
  CtkTreeViewColumn *left_column;
  CtkTreeViewColumn *right_column;
};

typedef struct _CtkTreeViewChild CtkTreeViewChild;
struct _CtkTreeViewChild
{
  CtkWidget *widget;
  CtkRBNode *node;
  CtkRBTree *tree;
  CtkTreeViewColumn *column;
  CtkBorder border;
};


typedef struct _TreeViewDragInfo TreeViewDragInfo;
struct _TreeViewDragInfo
{
  CdkModifierType start_button_mask;
  CtkTargetList *_unused_source_target_list;
  CdkDragAction source_actions;

  CtkTargetList *_unused_dest_target_list;

  guint source_set : 1;
  guint dest_set : 1;
};


struct _CtkTreeViewPrivate
{
  CtkTreeModel *model;

  /* tree information */
  CtkRBTree *tree;

  /* Container info */
  GList *children;
  gint width;

  guint presize_handler_tick_cb;

  /* Adjustments */
  CtkAdjustment *hadjustment;
  CtkAdjustment *vadjustment;
  gint           min_display_width;
  gint           min_display_height;

  /* Sub windows */
  CdkWindow *bin_window;
  CdkWindow *header_window;

  CtkPixelCache *pixel_cache;

  /* CSS nodes */
  CtkCssNode *header_node;

  /* Scroll position state keeping */
  CtkTreeRowReference *top_row;
  gint top_row_dy;
  /* dy == y pos of top_row + top_row_dy */
  /* we cache it for simplicity of the code */
  gint dy;

  guint validate_rows_timer;
  guint scroll_sync_timer;

  /* Indentation and expander layout */
  CtkTreeViewColumn *expander_column;

  gint level_indentation;

  /* Key navigation (focus), selection */
  gint cursor_offset;

  CtkTreeRowReference *anchor;
  CtkRBNode *cursor_node;
  CtkRBTree *cursor_tree;

  CtkTreeViewColumn *focus_column;

  /* Current pressed node, previously pressed, prelight */
  CtkRBNode *button_pressed_node;
  CtkRBTree *button_pressed_tree;

  gint press_start_x;
  gint press_start_y;

  gint event_last_x;
  gint event_last_y;

  CtkRBNode *prelight_node;
  CtkRBTree *prelight_tree;

  /* Cell Editing */
  CtkTreeViewColumn *edited_column;

  /* Auto expand/collapse timeout in hover mode */
  guint auto_expand_timeout;

  /* Selection information */
  CtkTreeSelection *selection;

  /* Header information */
  gint header_height;
  gint n_columns;
  GList *columns;

  CtkTreeViewColumnDropFunc column_drop_func;
  gpointer column_drop_func_data;
  GDestroyNotify column_drop_func_data_destroy;
  GList *column_drag_info;
  CtkTreeViewColumnReorder *cur_reorder;

  gint prev_width_before_expander;

  /* Scroll timeout (e.g. during dnd, rubber banding) */
  guint scroll_timeout;

  /* Interactive Header reordering */
  CdkWindow *drag_window;
  CdkWindow *drag_highlight_window;
  CtkTreeViewColumn *drag_column;
  gint drag_column_x;

  /* Interactive Header Resizing */
  gint drag_pos;
  gint x_drag;

  /* Non-interactive Header Resizing, expand flag support */
  gint last_extra_space;
  gint last_extra_space_per_column;
  gint last_number_of_expand_columns;

  /* ATK Hack */
  CtkTreeDestroyCountFunc destroy_count_func;
  gpointer destroy_count_data;
  GDestroyNotify destroy_count_destroy;

  /* Row drag-and-drop */
  CtkTreeRowReference *drag_dest_row;
  CtkTreeViewDropPosition drag_dest_pos;
  guint open_dest_timeout;

  /* Rubber banding */
  gint rubber_band_status;
  gint rubber_band_x;
  gint rubber_band_y;
  gint rubber_band_extend;
  gint rubber_band_modify;

  /* fixed height */
  gint fixed_height;

  CtkRBNode *rubber_band_start_node;
  CtkRBTree *rubber_band_start_tree;

  CtkRBNode *rubber_band_end_node;
  CtkRBTree *rubber_band_end_tree;
  CtkCssNode *rubber_band_cssnode;

  /* Scroll-to functionality when unrealized */
  CtkTreeRowReference *scroll_to_path;
  CtkTreeViewColumn *scroll_to_column;
  gfloat scroll_to_row_align;
  gfloat scroll_to_col_align;

  /* Interactive search */
  gint selected_iter;
  gint search_column;
  CtkTreeViewSearchPositionFunc search_position_func;
  CtkTreeViewSearchEqualFunc search_equal_func;
  gpointer search_user_data;
  GDestroyNotify search_destroy;
  gpointer search_position_user_data;
  GDestroyNotify search_position_destroy;
  CtkWidget *search_window;
  CtkWidget *search_entry;
  gulong search_entry_changed_id;
  guint typeselect_flush_timeout;

  /* Grid and tree lines */
  CtkTreeViewGridLines grid_lines;
  double grid_line_dashes[2];
  int grid_line_width;

  gboolean tree_lines_enabled;
  double tree_line_dashes[2];
  int tree_line_width;

  /* Row separators */
  CtkTreeViewRowSeparatorFunc row_separator_func;
  gpointer row_separator_data;
  GDestroyNotify row_separator_destroy;

  /* Gestures */
  CtkGesture *multipress_gesture;
  CtkGesture *column_multipress_gesture;
  CtkGesture *drag_gesture; /* Rubberbanding, row DnD */
  CtkGesture *column_drag_gesture; /* Column reordering, resizing */

  /* Tooltip support */
  gint tooltip_column;

  /* Here comes the bitfield */
  guint scroll_to_use_align : 1;

  guint fixed_height_mode : 1;
  guint fixed_height_check : 1;

  guint activate_on_single_click : 1;
  guint reorderable : 1;
  guint header_has_focus : 1;
  guint drag_column_window_state : 3;
  /* hint to display rows in alternating colors */
  guint has_rules : 1;
  guint mark_rows_col_dirty : 1;

  /* for DnD */
  guint empty_view_drop : 1;

  guint modify_selection_pressed : 1;
  guint extend_selection_pressed : 1;

  guint init_hadjust_value : 1;

  guint in_top_row_to_dy : 1;

  /* interactive search */
  guint enable_search : 1;
  guint disable_popdown : 1;
  guint search_custom_entry_set : 1;
  
  guint hover_selection : 1;
  guint hover_expand : 1;
  guint imcontext_changed : 1;

  guint rubber_banding_enable : 1;

  guint in_grab : 1;

  guint post_validation_flag : 1;

  /* Whether our key press handler is to avoid sending an unhandled binding to the search entry */
  guint search_entry_avoid_unhandled_binding : 1;

  /* CtkScrollablePolicy needs to be checked when
   * driving the scrollable adjustment values */
  guint hscroll_policy : 1;
  guint vscroll_policy : 1;

  /* CtkTreeView flags */
  guint is_list : 1;
  guint show_expanders : 1;
  guint in_column_resize : 1;
  guint arrow_prelit : 1;
  guint headers_visible : 1;
  guint draw_keyfocus : 1;
  guint model_setup : 1;
  guint in_column_drag : 1;
};


/* Signals */
enum
{
  ROW_ACTIVATED,
  TEST_EXPAND_ROW,
  TEST_COLLAPSE_ROW,
  ROW_EXPANDED,
  ROW_COLLAPSED,
  COLUMNS_CHANGED,
  CURSOR_CHANGED,
  MOVE_CURSOR,
  SELECT_ALL,
  UNSELECT_ALL,
  SELECT_CURSOR_ROW,
  TOGGLE_CURSOR_ROW,
  EXPAND_COLLAPSE_CURSOR_ROW,
  SELECT_CURSOR_PARENT,
  START_INTERACTIVE_SEARCH,
  LAST_SIGNAL
};

/* Properties */
enum {
  PROP_0,
  PROP_MODEL,
  PROP_HEADERS_VISIBLE,
  PROP_HEADERS_CLICKABLE,
  PROP_EXPANDER_COLUMN,
  PROP_REORDERABLE,
  PROP_RULES_HINT,
  PROP_ENABLE_SEARCH,
  PROP_SEARCH_COLUMN,
  PROP_FIXED_HEIGHT_MODE,
  PROP_HOVER_SELECTION,
  PROP_HOVER_EXPAND,
  PROP_SHOW_EXPANDERS,
  PROP_LEVEL_INDENTATION,
  PROP_RUBBER_BANDING,
  PROP_ENABLE_GRID_LINES,
  PROP_ENABLE_TREE_LINES,
  PROP_TOOLTIP_COLUMN,
  PROP_ACTIVATE_ON_SINGLE_CLICK,
  LAST_PROP,
  /* overridden */
  PROP_HADJUSTMENT = LAST_PROP,
  PROP_VADJUSTMENT,
  PROP_HSCROLL_POLICY,
  PROP_VSCROLL_POLICY,
};

/* object signals */
static void     ctk_tree_view_finalize             (GObject          *object);
static void     ctk_tree_view_set_property         (GObject         *object,
						    guint            prop_id,
						    const GValue    *value,
						    GParamSpec      *pspec);
static void     ctk_tree_view_get_property         (GObject         *object,
						    guint            prop_id,
						    GValue          *value,
						    GParamSpec      *pspec);

/* ctkwidget signals */
static void     ctk_tree_view_destroy              (CtkWidget        *widget);
static void     ctk_tree_view_realize              (CtkWidget        *widget);
static void     ctk_tree_view_unrealize            (CtkWidget        *widget);
static void     ctk_tree_view_map                  (CtkWidget        *widget);
static void     ctk_tree_view_unmap                (CtkWidget        *widget);
static void     ctk_tree_view_get_preferred_width  (CtkWidget        *widget,
						    gint             *minimum,
						    gint             *natural);
static void     ctk_tree_view_get_preferred_height (CtkWidget        *widget,
						    gint             *minimum,
						    gint             *natural);
static void     ctk_tree_view_size_allocate        (CtkWidget        *widget,
						    CtkAllocation    *allocation);
static gboolean ctk_tree_view_draw                 (CtkWidget        *widget,
                                                    cairo_t          *cr);
static gboolean ctk_tree_view_key_press            (CtkWidget        *widget,
						    CdkEventKey      *event);
static gboolean ctk_tree_view_key_release          (CtkWidget        *widget,
						    CdkEventKey      *event);
static gboolean ctk_tree_view_motion               (CtkWidget        *widget,
						    CdkEventMotion   *event);
static gboolean ctk_tree_view_enter_notify         (CtkWidget        *widget,
						    CdkEventCrossing *event);
static gboolean ctk_tree_view_leave_notify         (CtkWidget        *widget,
						    CdkEventCrossing *event);

static void     ctk_tree_view_set_focus_child      (CtkContainer     *container,
						    CtkWidget        *child);
static gint     ctk_tree_view_focus_out            (CtkWidget        *widget,
						    CdkEventFocus    *event);
static gint     ctk_tree_view_focus                (CtkWidget        *widget,
						    CtkDirectionType  direction);
static void     ctk_tree_view_grab_focus           (CtkWidget        *widget);
static void     ctk_tree_view_style_updated        (CtkWidget        *widget);

/* container signals */
static void     ctk_tree_view_remove               (CtkContainer     *container,
						    CtkWidget        *widget);
static void     ctk_tree_view_forall               (CtkContainer     *container,
						    gboolean          include_internals,
						    CtkCallback       callback,
						    gpointer          callback_data);

/* Source side drag signals */
static void ctk_tree_view_drag_begin       (CtkWidget        *widget,
                                            CdkDragContext   *context);
static void ctk_tree_view_drag_end         (CtkWidget        *widget,
                                            CdkDragContext   *context);
static void ctk_tree_view_drag_data_get    (CtkWidget        *widget,
                                            CdkDragContext   *context,
                                            CtkSelectionData *selection_data,
                                            guint             info,
                                            guint             time);
static void ctk_tree_view_drag_data_delete (CtkWidget        *widget,
                                            CdkDragContext   *context);

/* Target side drag signals */
static void     ctk_tree_view_drag_leave         (CtkWidget        *widget,
                                                  CdkDragContext   *context,
                                                  guint             time);
static gboolean ctk_tree_view_drag_motion        (CtkWidget        *widget,
                                                  CdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  guint             time);
static gboolean ctk_tree_view_drag_drop          (CtkWidget        *widget,
                                                  CdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  guint             time);
static void     ctk_tree_view_drag_data_received (CtkWidget        *widget,
                                                  CdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  CtkSelectionData *selection_data,
                                                  guint             info,
                                                  guint             time);

/* tree_model signals */
static gboolean ctk_tree_view_real_move_cursor            (CtkTreeView     *tree_view,
							   CtkMovementStep  step,
							   gint             count);
static gboolean ctk_tree_view_real_select_all             (CtkTreeView     *tree_view);
static gboolean ctk_tree_view_real_unselect_all           (CtkTreeView     *tree_view);
static gboolean ctk_tree_view_real_select_cursor_row      (CtkTreeView     *tree_view,
							   gboolean         start_editing);
static gboolean ctk_tree_view_real_toggle_cursor_row      (CtkTreeView     *tree_view);
static gboolean ctk_tree_view_real_expand_collapse_cursor_row (CtkTreeView     *tree_view,
							       gboolean         logical,
							       gboolean         expand,
							       gboolean         open_all);
static gboolean ctk_tree_view_real_select_cursor_parent   (CtkTreeView     *tree_view);
static void ctk_tree_view_row_changed                     (CtkTreeModel    *model,
							   CtkTreePath     *path,
							   CtkTreeIter     *iter,
							   gpointer         data);
static void ctk_tree_view_row_inserted                    (CtkTreeModel    *model,
							   CtkTreePath     *path,
							   CtkTreeIter     *iter,
							   gpointer         data);
static void ctk_tree_view_row_has_child_toggled           (CtkTreeModel    *model,
							   CtkTreePath     *path,
							   CtkTreeIter     *iter,
							   gpointer         data);
static void ctk_tree_view_row_deleted                     (CtkTreeModel    *model,
							   CtkTreePath     *path,
							   gpointer         data);
static void ctk_tree_view_rows_reordered                  (CtkTreeModel    *model,
							   CtkTreePath     *parent,
							   CtkTreeIter     *iter,
							   gint            *new_order,
							   gpointer         data);

/* Incremental reflow */
static gboolean validate_row             (CtkTreeView *tree_view,
					  CtkRBTree   *tree,
					  CtkRBNode   *node,
					  CtkTreeIter *iter,
					  CtkTreePath *path);
static void     validate_visible_area    (CtkTreeView *tree_view);
static gboolean do_validate_rows         (CtkTreeView *tree_view,
					  gboolean     queue_resize);
static gboolean validate_rows            (CtkTreeView *tree_view);
static void     install_presize_handler  (CtkTreeView *tree_view);
static void     install_scroll_sync_handler (CtkTreeView *tree_view);
static void     ctk_tree_view_set_top_row   (CtkTreeView *tree_view,
					     CtkTreePath *path,
					     gint         offset);
static void	ctk_tree_view_dy_to_top_row (CtkTreeView *tree_view);
static void     ctk_tree_view_top_row_to_dy (CtkTreeView *tree_view);
static void     invalidate_empty_focus      (CtkTreeView *tree_view);

/* Internal functions */
static gboolean ctk_tree_view_is_expander_column             (CtkTreeView        *tree_view,
							      CtkTreeViewColumn  *column);
static inline gboolean ctk_tree_view_draw_expanders          (CtkTreeView        *tree_view);
static void     ctk_tree_view_add_move_binding               (CtkBindingSet      *binding_set,
							      guint               keyval,
							      guint               modmask,
							      gboolean            add_shifted_binding,
							      CtkMovementStep     step,
							      gint                count);
static gint     ctk_tree_view_unref_and_check_selection_tree (CtkTreeView        *tree_view,
							      CtkRBTree          *tree);
static void     ctk_tree_view_queue_draw_path                (CtkTreeView        *tree_view,
							      CtkTreePath        *path,
							      const CdkRectangle *clip_rect);
static void     ctk_tree_view_queue_draw_arrow               (CtkTreeView        *tree_view,
							      CtkRBTree          *tree,
							      CtkRBNode          *node);
static void     ctk_tree_view_draw_arrow                     (CtkTreeView        *tree_view,
                                                              cairo_t            *cr,
							      CtkRBTree          *tree,
							      CtkRBNode          *node);
static void     ctk_tree_view_get_arrow_xrange               (CtkTreeView        *tree_view,
							      CtkRBTree          *tree,
							      gint               *x1,
							      gint               *x2);
static void     ctk_tree_view_adjustment_changed             (CtkAdjustment      *adjustment,
							      CtkTreeView        *tree_view);
static void     ctk_tree_view_build_tree                     (CtkTreeView        *tree_view,
							      CtkRBTree          *tree,
							      CtkTreeIter        *iter,
							      gint                depth,
							      gboolean            recurse);
static void     ctk_tree_view_clamp_node_visible             (CtkTreeView        *tree_view,
							      CtkRBTree          *tree,
							      CtkRBNode          *node);
static void     ctk_tree_view_clamp_column_visible           (CtkTreeView        *tree_view,
							      CtkTreeViewColumn  *column,
							      gboolean            focus_to_cell);
static gboolean ctk_tree_view_maybe_begin_dragging_row       (CtkTreeView        *tree_view);
static void     ctk_tree_view_focus_to_cursor                (CtkTreeView        *tree_view);
static void     ctk_tree_view_move_cursor_up_down            (CtkTreeView        *tree_view,
							      gint                count);
static void     ctk_tree_view_move_cursor_page_up_down       (CtkTreeView        *tree_view,
							      gint                count);
static void     ctk_tree_view_move_cursor_left_right         (CtkTreeView        *tree_view,
							      gint                count);
static void     ctk_tree_view_move_cursor_start_end          (CtkTreeView        *tree_view,
							      gint                count);
static gboolean ctk_tree_view_real_collapse_row              (CtkTreeView        *tree_view,
							      CtkTreePath        *path,
							      CtkRBTree          *tree,
							      CtkRBNode          *node,
							      gboolean            animate);
static gboolean ctk_tree_view_real_expand_row                (CtkTreeView        *tree_view,
							      CtkTreePath        *path,
							      CtkRBTree          *tree,
							      CtkRBNode          *node,
							      gboolean            open_all,
							      gboolean            animate);
static void     ctk_tree_view_real_set_cursor                (CtkTreeView        *tree_view,
							      CtkTreePath        *path,
                                                              SetCursorFlags      flags);
static gboolean ctk_tree_view_has_can_focus_cell             (CtkTreeView        *tree_view);
static void     column_sizing_notify                         (GObject            *object,
                                                              GParamSpec         *pspec,
                                                              gpointer            data);
static void     ctk_tree_view_stop_rubber_band               (CtkTreeView        *tree_view);
static void     update_prelight                              (CtkTreeView        *tree_view,
                                                              int                 x,
                                                              int                 y);
static void     ctk_tree_view_queue_draw_region              (CtkWidget          *widget,
							      const cairo_region_t *region);

static inline gint ctk_tree_view_get_effective_header_height (CtkTreeView *tree_view);

static inline gint ctk_tree_view_get_cell_area_y_offset      (CtkTreeView *tree_view,
                                                              CtkRBTree   *tree,
                                                              CtkRBNode   *node,
                                                              gint         vertical_separator);
static inline gint ctk_tree_view_get_cell_area_height        (CtkTreeView *tree_view,
                                                              CtkRBNode   *node,
                                                              gint         vertical_separator);

static inline gint ctk_tree_view_get_row_y_offset            (CtkTreeView *tree_view,
                                                              CtkRBTree   *tree,
                                                              CtkRBNode   *node);
static inline gint ctk_tree_view_get_row_height              (CtkTreeView *tree_view,
                                                              CtkRBNode   *node);

/* interactive search */
static void     ctk_tree_view_ensure_interactive_directory (CtkTreeView *tree_view);
static void     ctk_tree_view_search_window_hide        (CtkWidget        *search_window,
                                                         CtkTreeView      *tree_view,
                                                         CdkDevice        *device);
static void     ctk_tree_view_search_position_func      (CtkTreeView      *tree_view,
							 CtkWidget        *search_window,
							 gpointer          user_data);
static void     ctk_tree_view_search_disable_popdown    (CtkEntry         *entry,
							 CtkMenu          *menu,
							 gpointer          data);
static void     ctk_tree_view_search_preedit_changed    (CtkIMContext     *im_context,
							 CtkTreeView      *tree_view);
static void     ctk_tree_view_search_commit             (CtkIMContext     *im_context,
                                                         gchar            *buf,
                                                         CtkTreeView      *tree_view);
static void     ctk_tree_view_search_activate           (CtkEntry         *entry,
							 CtkTreeView      *tree_view);
static gboolean ctk_tree_view_real_search_enable_popdown(gpointer          data);
static void     ctk_tree_view_search_enable_popdown     (CtkWidget        *widget,
							 gpointer          data);
static gboolean ctk_tree_view_search_delete_event       (CtkWidget        *widget,
							 CdkEventAny      *event,
							 CtkTreeView      *tree_view);
static gboolean ctk_tree_view_search_button_press_event (CtkWidget        *widget,
							 CdkEventButton   *event,
							 CtkTreeView      *tree_view);
static gboolean ctk_tree_view_search_scroll_event       (CtkWidget        *entry,
							 CdkEventScroll   *event,
							 CtkTreeView      *tree_view);
static gboolean ctk_tree_view_search_key_press_event    (CtkWidget        *entry,
							 CdkEventKey      *event,
							 CtkTreeView      *tree_view);
static gboolean ctk_tree_view_search_move               (CtkWidget        *window,
							 CtkTreeView      *tree_view,
							 gboolean          up);
static gboolean ctk_tree_view_search_equal_func         (CtkTreeModel     *model,
							 gint              column,
							 const gchar      *key,
							 CtkTreeIter      *iter,
							 gpointer          search_data);
static gboolean ctk_tree_view_search_iter               (CtkTreeModel     *model,
							 CtkTreeSelection *selection,
							 CtkTreeIter      *iter,
							 const gchar      *text,
							 gint             *count,
							 gint              n);
static void     ctk_tree_view_search_init               (CtkWidget        *entry,
							 CtkTreeView      *tree_view);
static void     ctk_tree_view_put                       (CtkTreeView      *tree_view,
							 CtkWidget        *child_widget,
                                                         CtkTreePath      *path,
                                                         CtkTreeViewColumn*column,
                                                         const CtkBorder  *border);
static gboolean ctk_tree_view_start_editing             (CtkTreeView      *tree_view,
							 CtkTreePath      *cursor_path,
							 gboolean          edit_only);
static void ctk_tree_view_stop_editing                  (CtkTreeView *tree_view,
							 gboolean     cancel_editing);
static gboolean ctk_tree_view_real_start_interactive_search (CtkTreeView *tree_view,
                                                             CdkDevice   *device,
							     gboolean     keybinding);
static gboolean ctk_tree_view_start_interactive_search      (CtkTreeView *tree_view);
static CtkTreeViewColumn *ctk_tree_view_get_drop_column (CtkTreeView       *tree_view,
							 CtkTreeViewColumn *column,
							 gint               drop_position);

/* CtkBuildable */
static void     ctk_tree_view_buildable_add_child          (CtkBuildable      *tree_view,
							    CtkBuilder        *builder,
							    GObject           *child,
							    const gchar       *type);
static GObject *ctk_tree_view_buildable_get_internal_child (CtkBuildable      *buildable,
							    CtkBuilder        *builder,
							    const gchar       *childname);
static void     ctk_tree_view_buildable_init               (CtkBuildableIface *iface);

/* CtkScrollable */
static void     ctk_tree_view_scrollable_init              (CtkScrollableInterface *iface);

static CtkAdjustment *ctk_tree_view_do_get_hadjustment (CtkTreeView   *tree_view);
static void           ctk_tree_view_do_set_hadjustment (CtkTreeView   *tree_view,
                                                        CtkAdjustment *adjustment);
static CtkAdjustment *ctk_tree_view_do_get_vadjustment (CtkTreeView   *tree_view);
static void           ctk_tree_view_do_set_vadjustment (CtkTreeView   *tree_view,
                                                        CtkAdjustment *adjustment);

static gboolean scroll_row_timeout                   (gpointer     data);
static void     add_scroll_timeout                   (CtkTreeView *tree_view);
static void     remove_scroll_timeout                (CtkTreeView *tree_view);

static void     grab_focus_and_unset_draw_keyfocus   (CtkTreeView *tree_view);

/* Gestures */
static void ctk_tree_view_column_multipress_gesture_pressed (CtkGestureMultiPress *gesture,
                                                             gint                  n_press,
                                                             gdouble               x,
                                                             gdouble               y,
                                                             CtkTreeView          *tree_view);

static void ctk_tree_view_multipress_gesture_pressed        (CtkGestureMultiPress *gesture,
                                                             gint                  n_press,
                                                             gdouble               x,
                                                             gdouble               y,
                                                             CtkTreeView          *tree_view);
static void ctk_tree_view_multipress_gesture_released       (CtkGestureMultiPress *gesture,
                                                             gint                  n_press,
                                                             gdouble               x,
                                                             gdouble               y,
                                                             CtkTreeView          *tree_view);

static void ctk_tree_view_column_drag_gesture_begin         (CtkGestureDrag *gesture,
                                                             gdouble         start_x,
                                                             gdouble         start_y,
                                                             CtkTreeView    *tree_view);
static void ctk_tree_view_column_drag_gesture_update        (CtkGestureDrag *gesture,
                                                             gdouble         offset_x,
                                                             gdouble         offset_y,
                                                             CtkTreeView    *tree_view);
static void ctk_tree_view_column_drag_gesture_end           (CtkGestureDrag *gesture,
                                                             gdouble         offset_x,
                                                             gdouble         offset_y,
                                                             CtkTreeView    *tree_view);

static void ctk_tree_view_drag_gesture_begin                (CtkGestureDrag *gesture,
                                                             gdouble         start_x,
                                                             gdouble         start_y,
                                                             CtkTreeView    *tree_view);
static void ctk_tree_view_drag_gesture_update               (CtkGestureDrag *gesture,
                                                             gdouble         offset_x,
                                                             gdouble         offset_y,
                                                             CtkTreeView    *tree_view);
static void ctk_tree_view_drag_gesture_end                  (CtkGestureDrag *gesture,
                                                             gdouble         offset_x,
                                                             gdouble         offset_y,
                                                             CtkTreeView    *tree_view);

static guint tree_view_signals [LAST_SIGNAL] = { 0 };
static GParamSpec *tree_view_props [LAST_PROP] = { NULL };



/* GType Methods
 */

G_DEFINE_TYPE_WITH_CODE (CtkTreeView, ctk_tree_view, CTK_TYPE_CONTAINER,
                         G_ADD_PRIVATE (CtkTreeView)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
						ctk_tree_view_buildable_init)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_SCROLLABLE,
                                                ctk_tree_view_scrollable_init))

static void
ctk_tree_view_class_init (CtkTreeViewClass *class)
{
  GObjectClass *o_class;
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;
  CtkBindingSet *binding_set;

  binding_set = ctk_binding_set_by_class (class);

  o_class = (GObjectClass *) class;
  widget_class = (CtkWidgetClass *) class;
  container_class = (CtkContainerClass *) class;

  /* GObject signals */
  o_class->set_property = ctk_tree_view_set_property;
  o_class->get_property = ctk_tree_view_get_property;
  o_class->finalize = ctk_tree_view_finalize;

  /* CtkWidget signals */
  widget_class->destroy = ctk_tree_view_destroy;
  widget_class->map = ctk_tree_view_map;
  widget_class->unmap = ctk_tree_view_unmap;
  widget_class->realize = ctk_tree_view_realize;
  widget_class->unrealize = ctk_tree_view_unrealize;
  widget_class->get_preferred_width = ctk_tree_view_get_preferred_width;
  widget_class->get_preferred_height = ctk_tree_view_get_preferred_height;
  widget_class->size_allocate = ctk_tree_view_size_allocate;
  widget_class->motion_notify_event = ctk_tree_view_motion;
  widget_class->draw = ctk_tree_view_draw;
  widget_class->key_press_event = ctk_tree_view_key_press;
  widget_class->key_release_event = ctk_tree_view_key_release;
  widget_class->enter_notify_event = ctk_tree_view_enter_notify;
  widget_class->leave_notify_event = ctk_tree_view_leave_notify;
  widget_class->focus_out_event = ctk_tree_view_focus_out;
  widget_class->drag_begin = ctk_tree_view_drag_begin;
  widget_class->drag_end = ctk_tree_view_drag_end;
  widget_class->drag_data_get = ctk_tree_view_drag_data_get;
  widget_class->drag_data_delete = ctk_tree_view_drag_data_delete;
  widget_class->drag_leave = ctk_tree_view_drag_leave;
  widget_class->drag_motion = ctk_tree_view_drag_motion;
  widget_class->drag_drop = ctk_tree_view_drag_drop;
  widget_class->drag_data_received = ctk_tree_view_drag_data_received;
  widget_class->focus = ctk_tree_view_focus;
  widget_class->grab_focus = ctk_tree_view_grab_focus;
  widget_class->style_updated = ctk_tree_view_style_updated;
  widget_class->queue_draw_region = ctk_tree_view_queue_draw_region;

  /* CtkContainer signals */
  container_class->remove = ctk_tree_view_remove;
  container_class->forall = ctk_tree_view_forall;
  container_class->set_focus_child = ctk_tree_view_set_focus_child;

  class->move_cursor = ctk_tree_view_real_move_cursor;
  class->select_all = ctk_tree_view_real_select_all;
  class->unselect_all = ctk_tree_view_real_unselect_all;
  class->select_cursor_row = ctk_tree_view_real_select_cursor_row;
  class->toggle_cursor_row = ctk_tree_view_real_toggle_cursor_row;
  class->expand_collapse_cursor_row = ctk_tree_view_real_expand_collapse_cursor_row;
  class->select_cursor_parent = ctk_tree_view_real_select_cursor_parent;
  class->start_interactive_search = ctk_tree_view_start_interactive_search;

  /* Properties */

  g_object_class_override_property (o_class, PROP_HADJUSTMENT,    "hadjustment");
  g_object_class_override_property (o_class, PROP_VADJUSTMENT,    "vadjustment");
  g_object_class_override_property (o_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (o_class, PROP_VSCROLL_POLICY, "vscroll-policy");

  tree_view_props[PROP_MODEL] =
      g_param_spec_object ("model",
                           P_("TreeView Model"),
                           P_("The model for the tree view"),
                           CTK_TYPE_TREE_MODEL,
                           CTK_PARAM_READWRITE);

  tree_view_props[PROP_HEADERS_VISIBLE] =
      g_param_spec_boolean ("headers-visible",
                            P_("Headers Visible"),
                            P_("Show the column header buttons"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_view_props[PROP_HEADERS_CLICKABLE] =
      g_param_spec_boolean ("headers-clickable",
                            P_("Headers Clickable"),
                            P_("Column headers respond to click events"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_view_props[PROP_EXPANDER_COLUMN] =
      g_param_spec_object ("expander-column",
                           P_("Expander Column"),
                           P_("Set the column for the expander column"),
                           CTK_TYPE_TREE_VIEW_COLUMN,
                           CTK_PARAM_READWRITE);

  tree_view_props[PROP_REORDERABLE] =
      g_param_spec_boolean ("reorderable",
                            P_("Reorderable"),
                            P_("View is reorderable"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkTreeView:rules-hint:
   *
   * Sets a hint to the theme to draw rows in alternating colors.
   *
   * Deprecated: 3.14: The theme is responsible for drawing rows
   *   using zebra striping
   */
  tree_view_props[PROP_RULES_HINT] =
      g_param_spec_boolean ("rules-hint",
                            P_("Rules Hint"),
                            P_("Set a hint to the theme engine to draw rows in alternating colors"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  tree_view_props[PROP_ENABLE_SEARCH] =
      g_param_spec_boolean ("enable-search",
                            P_("Enable Search"),
                            P_("View allows user to search through columns interactively"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_view_props[PROP_SEARCH_COLUMN] =
      g_param_spec_int ("search-column",
                        P_("Search Column"),
                        P_("Model column to search through during interactive search"),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkTreeView:fixed-height-mode:
   *
   * Setting the ::fixed-height-mode property to %TRUE speeds up 
   * #CtkTreeView by assuming that all rows have the same height. 
   * Only enable this option if all rows are the same height.  
   * Please see ctk_tree_view_set_fixed_height_mode() for more 
   * information on this option.
   *
   * Since: 2.4
   */
  tree_view_props[PROP_FIXED_HEIGHT_MODE] =
      g_param_spec_boolean ("fixed-height-mode",
                            P_("Fixed Height Mode"),
                            P_("Speeds up CtkTreeView by assuming that all rows have the same height"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkTreeView:hover-selection:
   * 
   * Enables or disables the hover selection mode of @tree_view.
   * Hover selection makes the selected row follow the pointer.
   * Currently, this works only for the selection modes 
   * %CTK_SELECTION_SINGLE and %CTK_SELECTION_BROWSE.
   *
   * This mode is primarily intended for treeviews in popups, e.g.
   * in #CtkComboBox or #CtkEntryCompletion.
   *
   * Since: 2.6
   */
  tree_view_props[PROP_HOVER_SELECTION] =
      g_param_spec_boolean ("hover-selection",
                            P_("Hover Selection"),
                            P_("Whether the selection should follow the pointer"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkTreeView:hover-expand:
   * 
   * Enables or disables the hover expansion mode of @tree_view.
   * Hover expansion makes rows expand or collapse if the pointer moves 
   * over them.
   *
   * This mode is primarily intended for treeviews in popups, e.g.
   * in #CtkComboBox or #CtkEntryCompletion.
   *
   * Since: 2.6
   */
  tree_view_props[PROP_HOVER_EXPAND] =
      g_param_spec_boolean ("hover-expand",
                            P_("Hover Expand"),
                            P_("Whether rows should be expanded/collapsed when the pointer moves over them"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkTreeView:show-expanders:
   *
   * %TRUE if the view has expanders.
   *
   * Since: 2.12
   */
  tree_view_props[PROP_SHOW_EXPANDERS] =
      g_param_spec_boolean ("show-expanders",
                            P_("Show Expanders"),
                            P_("View has expanders"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkTreeView:level-indentation:
   *
   * Extra indentation for each level.
   *
   * Since: 2.12
   */
  tree_view_props[PROP_LEVEL_INDENTATION] =
      g_param_spec_int ("level-indentation",
                        P_("Level Indentation"),
                        P_("Extra indentation for each level"),
                        0, G_MAXINT,
                        0,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_view_props[PROP_RUBBER_BANDING] =
      g_param_spec_boolean ("rubber-banding",
                            P_("Rubber Banding"),
                            P_("Whether to enable selection of multiple items by dragging the mouse pointer"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_view_props[PROP_ENABLE_GRID_LINES] =
      g_param_spec_enum ("enable-grid-lines",
                         P_("Enable Grid Lines"),
                         P_("Whether grid lines should be drawn in the tree view"),
                         CTK_TYPE_TREE_VIEW_GRID_LINES,
                         CTK_TREE_VIEW_GRID_LINES_NONE,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_view_props[PROP_ENABLE_TREE_LINES] =
      g_param_spec_boolean ("enable-tree-lines",
                            P_("Enable Tree Lines"),
                            P_("Whether tree lines should be drawn in the tree view"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_view_props[PROP_TOOLTIP_COLUMN] =
      g_param_spec_int ("tooltip-column",
                        P_("Tooltip Column"),
                        P_("The column in the model containing the tooltip texts for the rows"),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkTreeView:activate-on-single-click:
   *
   * The activate-on-single-click property specifies whether the "row-activated" signal
   * will be emitted after a single click.
   *
   * Since: 3.8
   */
  tree_view_props[PROP_ACTIVATE_ON_SINGLE_CLICK] =
      g_param_spec_boolean ("activate-on-single-click",
                            P_("Activate on Single Click"),
                            P_("Activate row on a single click"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (o_class, LAST_PROP, tree_view_props);

  /* Style properties */
#define _TREE_VIEW_EXPANDER_SIZE 14
#define _TREE_VIEW_VERTICAL_SEPARATOR 2
#define _TREE_VIEW_HORIZONTAL_SEPARATOR 2

  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("expander-size",
							     P_("Expander Size"),
							     P_("Size of the expander arrow"),
							     0,
							     G_MAXINT,
							     _TREE_VIEW_EXPANDER_SIZE,
							     CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("vertical-separator",
							     P_("Vertical Separator Width"),
							     P_("Vertical space between cells.  Must be an even number"),
							     0,
							     G_MAXINT,
							     _TREE_VIEW_VERTICAL_SEPARATOR,
							     CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("horizontal-separator",
							     P_("Horizontal Separator Width"),
							     P_("Horizontal space between cells.  Must be an even number"),
							     0,
							     G_MAXINT,
							     _TREE_VIEW_HORIZONTAL_SEPARATOR,
							     CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("allow-rules",
								 P_("Allow Rules"),
								 P_("Allow drawing of alternating color rows"),
								 TRUE,
								 CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("indent-expanders",
								 P_("Indent Expanders"),
								 P_("Make the expanders indented"),
								 TRUE,
								 CTK_PARAM_READABLE));
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boxed ("even-row-color",
                                                               P_("Even Row Color"),
                                                               P_("Color to use for even rows"),
							       CDK_TYPE_COLOR,
							       CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boxed ("odd-row-color",
                                                               P_("Odd Row Color"),
                                                               P_("Color to use for odd rows"),
							       CDK_TYPE_COLOR,
							       CTK_PARAM_READABLE));
G_GNUC_END_IGNORE_DEPRECATIONS

  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("grid-line-width",
							     P_("Grid line width"),
							     P_("Width, in pixels, of the tree view grid lines"),
							     0, G_MAXINT, 1,
							     CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("tree-line-width",
							     P_("Tree line width"),
							     P_("Width, in pixels, of the tree view lines"),
							     0, G_MAXINT, 1,
							     CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_string ("grid-line-pattern",
								P_("Grid line pattern"),
								P_("Dash pattern used to draw the tree view grid lines"),
								"\1\1",
								CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_string ("tree-line-pattern",
								P_("Tree line pattern"),
								P_("Dash pattern used to draw the tree view lines"),
								"\1\1",
								CTK_PARAM_READABLE));

  /* Signals */
  /**
   * CtkTreeView::row-activated:
   * @tree_view: the object on which the signal is emitted
   * @path: the #CtkTreePath for the activated row
   * @column: the #CtkTreeViewColumn in which the activation occurred
   *
   * The "row-activated" signal is emitted when the method
   * ctk_tree_view_row_activated() is called, when the user double
   * clicks a treeview row with the "activate-on-single-click"
   * property set to %FALSE, or when the user single clicks a row when
   * the "activate-on-single-click" property set to %TRUE. It is also
   * emitted when a non-editable row is selected and one of the keys:
   * Space, Shift+Space, Return or Enter is pressed.
   *
   * For selection handling refer to the
   * [tree widget conceptual overview][TreeWidget]
   * as well as #CtkTreeSelection.
   */
  tree_view_signals[ROW_ACTIVATED] =
    g_signal_new (I_("row-activated"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTreeViewClass, row_activated),
		  NULL, NULL,
		  _ctk_marshal_VOID__BOXED_OBJECT,
		  G_TYPE_NONE, 2,
		  CTK_TYPE_TREE_PATH,
		  CTK_TYPE_TREE_VIEW_COLUMN);
  g_signal_set_va_marshaller (tree_view_signals[ROW_ACTIVATED],
                              G_TYPE_FROM_CLASS (o_class),
                              _ctk_marshal_VOID__BOXED_OBJECTv);

  /**
   * CtkTreeView::test-expand-row:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the row to expand
   * @path: a tree path that points to the row 
   * 
   * The given row is about to be expanded (show its children nodes). Use this
   * signal if you need to control the expandability of individual rows.
   *
   * Returns: %FALSE to allow expansion, %TRUE to reject
   */
  tree_view_signals[TEST_EXPAND_ROW] =
    g_signal_new (I_("test-expand-row"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkTreeViewClass, test_expand_row),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED_BOXED,
		  G_TYPE_BOOLEAN, 2,
		  CTK_TYPE_TREE_ITER,
		  CTK_TYPE_TREE_PATH);
  g_signal_set_va_marshaller (tree_view_signals[TEST_EXPAND_ROW],
                              G_TYPE_FROM_CLASS (o_class),
                              _ctk_marshal_BOOLEAN__BOXED_BOXEDv);

  /**
   * CtkTreeView::test-collapse-row:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the row to collapse
   * @path: a tree path that points to the row 
   * 
   * The given row is about to be collapsed (hide its children nodes). Use this
   * signal if you need to control the collapsibility of individual rows.
   *
   * Returns: %FALSE to allow collapsing, %TRUE to reject
   */
  tree_view_signals[TEST_COLLAPSE_ROW] =
    g_signal_new (I_("test-collapse-row"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkTreeViewClass, test_collapse_row),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED_BOXED,
		  G_TYPE_BOOLEAN, 2,
		  CTK_TYPE_TREE_ITER,
		  CTK_TYPE_TREE_PATH);
  g_signal_set_va_marshaller (tree_view_signals[TEST_COLLAPSE_ROW],
                              G_TYPE_FROM_CLASS (o_class),
                              _ctk_marshal_BOOLEAN__BOXED_BOXEDv);

  /**
   * CtkTreeView::row-expanded:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the expanded row
   * @path: a tree path that points to the row 
   * 
   * The given row has been expanded (child nodes are shown).
   */
  tree_view_signals[ROW_EXPANDED] =
    g_signal_new (I_("row-expanded"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkTreeViewClass, row_expanded),
		  NULL, NULL,
		  _ctk_marshal_VOID__BOXED_BOXED,
		  G_TYPE_NONE, 2,
		  CTK_TYPE_TREE_ITER,
		  CTK_TYPE_TREE_PATH);
  g_signal_set_va_marshaller (tree_view_signals[ROW_EXPANDED],
                              G_TYPE_FROM_CLASS (o_class),
                              _ctk_marshal_VOID__BOXED_BOXEDv);

  /**
   * CtkTreeView::row-collapsed:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the collapsed row
   * @path: a tree path that points to the row 
   * 
   * The given row has been collapsed (child nodes are hidden).
   */
  tree_view_signals[ROW_COLLAPSED] =
    g_signal_new (I_("row-collapsed"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkTreeViewClass, row_collapsed),
		  NULL, NULL,
		  _ctk_marshal_VOID__BOXED_BOXED,
		  G_TYPE_NONE, 2,
		  CTK_TYPE_TREE_ITER,
		  CTK_TYPE_TREE_PATH);
  g_signal_set_va_marshaller (tree_view_signals[ROW_COLLAPSED],
                              G_TYPE_FROM_CLASS (o_class),
                              _ctk_marshal_VOID__BOXED_BOXEDv);

  /**
   * CtkTreeView::columns-changed:
   * @tree_view: the object on which the signal is emitted 
   * 
   * The number of columns of the treeview has changed.
   */
  tree_view_signals[COLUMNS_CHANGED] =
    g_signal_new (I_("columns-changed"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkTreeViewClass, columns_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkTreeView::cursor-changed:
   * @tree_view: the object on which the signal is emitted
   * 
   * The position of the cursor (focused cell) has changed.
   */
  tree_view_signals[CURSOR_CHANGED] =
    g_signal_new (I_("cursor-changed"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkTreeViewClass, cursor_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkTreeView::move-cursor:
   * @tree_view: the object on which the signal is emitted.
   * @step: the granularity of the move, as a
   * #CtkMovementStep. %CTK_MOVEMENT_LOGICAL_POSITIONS,
   * %CTK_MOVEMENT_VISUAL_POSITIONS, %CTK_MOVEMENT_DISPLAY_LINES,
   * %CTK_MOVEMENT_PAGES and %CTK_MOVEMENT_BUFFER_ENDS are
   * supported. %CTK_MOVEMENT_LOGICAL_POSITIONS and
   * %CTK_MOVEMENT_VISUAL_POSITIONS are treated identically.
   * @direction: the direction to move: +1 to move forwards;
   * -1 to move backwards. The resulting movement is
   * undefined for all other values.
   *
   * The #CtkTreeView::move-cursor signal is a [keybinding
   * signal][CtkBindingSignal] which gets emitted when the user
   * presses one of the cursor keys.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control the cursor
   * programmatically. In contrast to ctk_tree_view_set_cursor() and
   * ctk_tree_view_set_cursor_on_cell() when moving horizontally
   * #CtkTreeView::move-cursor does not reset the current selection.
   *
   * Returns: %TRUE if @step is supported, %FALSE otherwise.
   */
  tree_view_signals[MOVE_CURSOR] =
    g_signal_new (I_("move-cursor"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTreeViewClass, move_cursor),
		  NULL, NULL,
		  _ctk_marshal_BOOLEAN__ENUM_INT,
		  G_TYPE_BOOLEAN, 2,
		  CTK_TYPE_MOVEMENT_STEP,
		  G_TYPE_INT);
  g_signal_set_va_marshaller (tree_view_signals[MOVE_CURSOR],
                              G_TYPE_FROM_CLASS (o_class),
                              _ctk_marshal_BOOLEAN__ENUM_INTv);

  tree_view_signals[SELECT_ALL] =
    g_signal_new (I_("select-all"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTreeViewClass, select_all),
		  NULL, NULL,
		  _ctk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (tree_view_signals[SELECT_ALL],
                              G_TYPE_FROM_CLASS (o_class),
                              _ctk_marshal_BOOLEAN__VOIDv);

  tree_view_signals[UNSELECT_ALL] =
    g_signal_new (I_("unselect-all"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTreeViewClass, unselect_all),
		  NULL, NULL,
		  _ctk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (tree_view_signals[UNSELECT_ALL],
                              G_TYPE_FROM_CLASS (o_class),
                              _ctk_marshal_BOOLEAN__VOIDv);

  tree_view_signals[SELECT_CURSOR_ROW] =
    g_signal_new (I_("select-cursor-row"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTreeViewClass, select_cursor_row),
		  NULL, NULL,
		  _ctk_marshal_BOOLEAN__BOOLEAN,
		  G_TYPE_BOOLEAN, 1,
		  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (tree_view_signals[SELECT_CURSOR_ROW],
                              G_TYPE_FROM_CLASS (o_class),
                              _ctk_marshal_BOOLEAN__BOOLEANv);

  tree_view_signals[TOGGLE_CURSOR_ROW] =
    g_signal_new (I_("toggle-cursor-row"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTreeViewClass, toggle_cursor_row),
		  NULL, NULL,
		  _ctk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (tree_view_signals[TOGGLE_CURSOR_ROW],
                              G_TYPE_FROM_CLASS (o_class),
                              _ctk_marshal_BOOLEAN__VOIDv);

  tree_view_signals[EXPAND_COLLAPSE_CURSOR_ROW] =
    g_signal_new (I_("expand-collapse-cursor-row"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTreeViewClass, expand_collapse_cursor_row),
		  NULL, NULL,
		  _ctk_marshal_BOOLEAN__BOOLEAN_BOOLEAN_BOOLEAN,
		  G_TYPE_BOOLEAN, 3,
		  G_TYPE_BOOLEAN,
		  G_TYPE_BOOLEAN,
		  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (tree_view_signals[EXPAND_COLLAPSE_CURSOR_ROW],
                              G_TYPE_FROM_CLASS (o_class),
                              _ctk_marshal_BOOLEAN__BOOLEAN_BOOLEAN_BOOLEANv);

  tree_view_signals[SELECT_CURSOR_PARENT] =
    g_signal_new (I_("select-cursor-parent"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTreeViewClass, select_cursor_parent),
		  NULL, NULL,
		  _ctk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (tree_view_signals[SELECT_CURSOR_PARENT],
                              G_TYPE_FROM_CLASS (o_class),
                              _ctk_marshal_BOOLEAN__VOIDv);

  tree_view_signals[START_INTERACTIVE_SEARCH] =
    g_signal_new (I_("start-interactive-search"),
		  G_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTreeViewClass, start_interactive_search),
		  NULL, NULL,
		  _ctk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (tree_view_signals[START_INTERACTIVE_SEARCH],
                              G_TYPE_FROM_CLASS (o_class),
                              _ctk_marshal_BOOLEAN__VOIDv);

  /* Key bindings */
  ctk_tree_view_add_move_binding (binding_set, CDK_KEY_Up, 0, TRUE,
				  CTK_MOVEMENT_DISPLAY_LINES, -1);
  ctk_tree_view_add_move_binding (binding_set, CDK_KEY_KP_Up, 0, TRUE,
				  CTK_MOVEMENT_DISPLAY_LINES, -1);

  ctk_tree_view_add_move_binding (binding_set, CDK_KEY_Down, 0, TRUE,
				  CTK_MOVEMENT_DISPLAY_LINES, 1);
  ctk_tree_view_add_move_binding (binding_set, CDK_KEY_KP_Down, 0, TRUE,
				  CTK_MOVEMENT_DISPLAY_LINES, 1);

  ctk_tree_view_add_move_binding (binding_set, CDK_KEY_p, CDK_CONTROL_MASK, FALSE,
				  CTK_MOVEMENT_DISPLAY_LINES, -1);

  ctk_tree_view_add_move_binding (binding_set, CDK_KEY_n, CDK_CONTROL_MASK, FALSE,
				  CTK_MOVEMENT_DISPLAY_LINES, 1);

  ctk_tree_view_add_move_binding (binding_set, CDK_KEY_Home, 0, TRUE,
				  CTK_MOVEMENT_BUFFER_ENDS, -1);
  ctk_tree_view_add_move_binding (binding_set, CDK_KEY_KP_Home, 0, TRUE,
				  CTK_MOVEMENT_BUFFER_ENDS, -1);

  ctk_tree_view_add_move_binding (binding_set, CDK_KEY_End, 0, TRUE,
				  CTK_MOVEMENT_BUFFER_ENDS, 1);
  ctk_tree_view_add_move_binding (binding_set, CDK_KEY_KP_End, 0, TRUE,
				  CTK_MOVEMENT_BUFFER_ENDS, 1);

  ctk_tree_view_add_move_binding (binding_set, CDK_KEY_Page_Up, 0, TRUE,
				  CTK_MOVEMENT_PAGES, -1);
  ctk_tree_view_add_move_binding (binding_set, CDK_KEY_KP_Page_Up, 0, TRUE,
				  CTK_MOVEMENT_PAGES, -1);

  ctk_tree_view_add_move_binding (binding_set, CDK_KEY_Page_Down, 0, TRUE,
				  CTK_MOVEMENT_PAGES, 1);
  ctk_tree_view_add_move_binding (binding_set, CDK_KEY_KP_Page_Down, 0, TRUE,
				  CTK_MOVEMENT_PAGES, 1);


  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Right, 0, "move-cursor", 2,
				G_TYPE_ENUM, CTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, 1);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Left, 0, "move-cursor", 2,
				G_TYPE_ENUM, CTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, -1);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Right, 0, "move-cursor", 2,
				G_TYPE_ENUM, CTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, 1);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Left, 0, "move-cursor", 2,
				G_TYPE_ENUM, CTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, -1);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Right, CDK_CONTROL_MASK,
                                "move-cursor", 2,
				G_TYPE_ENUM, CTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, 1);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Left, CDK_CONTROL_MASK,
                                "move-cursor", 2,
				G_TYPE_ENUM, CTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, -1);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Right, CDK_CONTROL_MASK,
                                "move-cursor", 2,
				G_TYPE_ENUM, CTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, 1);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Left, CDK_CONTROL_MASK,
                                "move-cursor", 2,
				G_TYPE_ENUM, CTK_MOVEMENT_VISUAL_POSITIONS,
				G_TYPE_INT, -1);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_space, CDK_CONTROL_MASK, "toggle-cursor-row", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Space, CDK_CONTROL_MASK, "toggle-cursor-row", 0);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_a, CDK_CONTROL_MASK, "select-all", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_slash, CDK_CONTROL_MASK, "select-all", 0);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_A, CDK_CONTROL_MASK | CDK_SHIFT_MASK, "unselect-all", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_backslash, CDK_CONTROL_MASK, "unselect-all", 0);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_space, CDK_SHIFT_MASK, "select-cursor-row", 1,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Space, CDK_SHIFT_MASK, "select-cursor-row", 1,
				G_TYPE_BOOLEAN, TRUE);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_space, 0, "select-cursor-row", 1,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Space, 0, "select-cursor-row", 1,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Return, 0, "select-cursor-row", 1,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_ISO_Enter, 0, "select-cursor-row", 1,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Enter, 0, "select-cursor-row", 1,
				G_TYPE_BOOLEAN, TRUE);

  /* expand and collapse rows */
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_plus, 0, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, FALSE);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_asterisk, 0,
                                "expand-collapse-cursor-row", 3,
                                G_TYPE_BOOLEAN, TRUE,
                                G_TYPE_BOOLEAN, TRUE,
                                G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Multiply, 0,
                                "expand-collapse-cursor-row", 3,
                                G_TYPE_BOOLEAN, TRUE,
                                G_TYPE_BOOLEAN, TRUE,
                                G_TYPE_BOOLEAN, TRUE);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_slash, 0,
                                "expand-collapse-cursor-row", 3,
                                G_TYPE_BOOLEAN, TRUE,
                                G_TYPE_BOOLEAN, FALSE,
                                G_TYPE_BOOLEAN, FALSE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Divide, 0,
                                "expand-collapse-cursor-row", 3,
                                G_TYPE_BOOLEAN, TRUE,
                                G_TYPE_BOOLEAN, FALSE,
                                G_TYPE_BOOLEAN, FALSE);

  /* Not doable on US keyboards */
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_plus, CDK_SHIFT_MASK, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Add, 0, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, FALSE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Add, CDK_SHIFT_MASK, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Add, CDK_SHIFT_MASK, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Right, CDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Right, CDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Right,
                                CDK_CONTROL_MASK | CDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Right,
                                CDK_CONTROL_MASK | CDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, TRUE);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_minus, 0, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, FALSE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_minus, CDK_SHIFT_MASK, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Subtract, 0, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, FALSE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Subtract, CDK_SHIFT_MASK, "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, TRUE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Left, CDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Left, CDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Left,
                                CDK_CONTROL_MASK | CDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Left,
                                CDK_CONTROL_MASK | CDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, FALSE,
				G_TYPE_BOOLEAN, TRUE);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_BackSpace, 0, "select-cursor-parent", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_BackSpace, CDK_CONTROL_MASK, "select-cursor-parent", 0);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_f, CDK_CONTROL_MASK, "start-interactive-search", 0);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_F, CDK_CONTROL_MASK, "start-interactive-search", 0);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_TREE_VIEW_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "treeview");
}

static void
ctk_tree_view_init (CtkTreeView *tree_view)
{
  CtkTreeViewPrivate *priv;
  CtkCssNode *widget_node;

  priv = tree_view->priv = ctk_tree_view_get_instance_private (tree_view);

  ctk_widget_set_can_focus (CTK_WIDGET (tree_view), TRUE);

  priv->show_expanders = TRUE;
  priv->draw_keyfocus = TRUE;
  priv->headers_visible = TRUE;
  priv->activate_on_single_click = FALSE;

  priv->pixel_cache = _ctk_pixel_cache_new ();

  /* We need some padding */
  priv->dy = 0;
  priv->cursor_offset = 0;
  priv->n_columns = 0;
  priv->header_height = 1;
  priv->x_drag = 0;
  priv->drag_pos = -1;
  priv->header_has_focus = FALSE;
  priv->press_start_x = -1;
  priv->press_start_y = -1;
  priv->reorderable = FALSE;
  priv->presize_handler_tick_cb = 0;
  priv->scroll_sync_timer = 0;
  priv->fixed_height = -1;
  priv->fixed_height_mode = FALSE;
  priv->fixed_height_check = 0;
  priv->selection = _ctk_tree_selection_new_with_tree_view (tree_view);
  priv->enable_search = TRUE;
  priv->search_column = -1;
  priv->search_position_func = ctk_tree_view_search_position_func;
  priv->search_equal_func = ctk_tree_view_search_equal_func;
  priv->search_custom_entry_set = FALSE;
  priv->typeselect_flush_timeout = 0;
  priv->init_hadjust_value = TRUE;    
  priv->width = 0;
          
  priv->hover_selection = FALSE;
  priv->hover_expand = FALSE;

  priv->level_indentation = 0;

  priv->rubber_banding_enable = FALSE;

  priv->grid_lines = CTK_TREE_VIEW_GRID_LINES_NONE;
  priv->tree_lines_enabled = FALSE;

  priv->tooltip_column = -1;

  priv->post_validation_flag = FALSE;

  priv->event_last_x = -10000;
  priv->event_last_y = -10000;

  ctk_tree_view_do_set_vadjustment (tree_view, NULL);
  ctk_tree_view_do_set_hadjustment (tree_view, NULL);

  ctk_style_context_add_class (ctk_widget_get_style_context (CTK_WIDGET (tree_view)),
                               CTK_STYLE_CLASS_VIEW);

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (tree_view));
  priv->header_node = ctk_css_node_new ();
  ctk_css_node_set_name (priv->header_node, I_("header"));
  ctk_css_node_set_parent (priv->header_node, widget_node);
  ctk_css_node_set_state (priv->header_node, ctk_css_node_get_state (widget_node));
  g_object_unref (priv->header_node);

  priv->multipress_gesture = ctk_gesture_multi_press_new (CTK_WIDGET (tree_view));
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (priv->multipress_gesture), 0);
  g_signal_connect (priv->multipress_gesture, "pressed",
                    G_CALLBACK (ctk_tree_view_multipress_gesture_pressed), tree_view);
  g_signal_connect (priv->multipress_gesture, "released",
                    G_CALLBACK (ctk_tree_view_multipress_gesture_released), tree_view);

  priv->column_multipress_gesture = ctk_gesture_multi_press_new (CTK_WIDGET (tree_view));
  g_signal_connect (priv->column_multipress_gesture, "pressed",
                    G_CALLBACK (ctk_tree_view_column_multipress_gesture_pressed), tree_view);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->column_multipress_gesture),
                                              CTK_PHASE_CAPTURE);

  priv->drag_gesture = ctk_gesture_drag_new (CTK_WIDGET (tree_view));
  g_signal_connect (priv->drag_gesture, "drag-begin",
                    G_CALLBACK (ctk_tree_view_drag_gesture_begin), tree_view);
  g_signal_connect (priv->drag_gesture, "drag-update",
                    G_CALLBACK (ctk_tree_view_drag_gesture_update), tree_view);
  g_signal_connect (priv->drag_gesture, "drag-end",
                    G_CALLBACK (ctk_tree_view_drag_gesture_end), tree_view);

  priv->column_drag_gesture = ctk_gesture_drag_new (CTK_WIDGET (tree_view));
  g_signal_connect (priv->column_drag_gesture, "drag-begin",
                    G_CALLBACK (ctk_tree_view_column_drag_gesture_begin), tree_view);
  g_signal_connect (priv->column_drag_gesture, "drag-update",
                    G_CALLBACK (ctk_tree_view_column_drag_gesture_update), tree_view);
  g_signal_connect (priv->column_drag_gesture, "drag-end",
                    G_CALLBACK (ctk_tree_view_column_drag_gesture_end), tree_view);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->column_drag_gesture),
                                              CTK_PHASE_CAPTURE);
}



/* GObject Methods
 */

static void
ctk_tree_view_set_property (GObject         *object,
			    guint            prop_id,
			    const GValue    *value,
			    GParamSpec      *pspec)
{
  CtkTreeView *tree_view;

  tree_view = CTK_TREE_VIEW (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      ctk_tree_view_set_model (tree_view, g_value_get_object (value));
      break;
    case PROP_HADJUSTMENT:
      ctk_tree_view_do_set_hadjustment (tree_view, g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      ctk_tree_view_do_set_vadjustment (tree_view, g_value_get_object (value));
      break;
    case PROP_HSCROLL_POLICY:
      if (tree_view->priv->hscroll_policy != g_value_get_enum (value))
        {
          tree_view->priv->hscroll_policy = g_value_get_enum (value);
          ctk_widget_queue_resize (CTK_WIDGET (tree_view));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_VSCROLL_POLICY:
      if (tree_view->priv->vscroll_policy != g_value_get_enum (value))
        {
          tree_view->priv->vscroll_policy = g_value_get_enum (value);
          ctk_widget_queue_resize (CTK_WIDGET (tree_view));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_HEADERS_VISIBLE:
      ctk_tree_view_set_headers_visible (tree_view, g_value_get_boolean (value));
      break;
    case PROP_HEADERS_CLICKABLE:
      ctk_tree_view_set_headers_clickable (tree_view, g_value_get_boolean (value));
      break;
    case PROP_EXPANDER_COLUMN:
      ctk_tree_view_set_expander_column (tree_view, g_value_get_object (value));
      break;
    case PROP_REORDERABLE:
      ctk_tree_view_set_reorderable (tree_view, g_value_get_boolean (value));
      break;
    case PROP_RULES_HINT:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_tree_view_set_rules_hint (tree_view, g_value_get_boolean (value));
G_GNUC_END_IGNORE_DEPRECATIONS
      break;
    case PROP_ENABLE_SEARCH:
      ctk_tree_view_set_enable_search (tree_view, g_value_get_boolean (value));
      break;
    case PROP_SEARCH_COLUMN:
      ctk_tree_view_set_search_column (tree_view, g_value_get_int (value));
      break;
    case PROP_FIXED_HEIGHT_MODE:
      ctk_tree_view_set_fixed_height_mode (tree_view, g_value_get_boolean (value));
      break;
    case PROP_HOVER_SELECTION:
      if (tree_view->priv->hover_selection != g_value_get_boolean (value))
        {
          tree_view->priv->hover_selection = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_HOVER_EXPAND:
      if (tree_view->priv->hover_expand != g_value_get_boolean (value))
        {
          tree_view->priv->hover_expand = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_SHOW_EXPANDERS:
      ctk_tree_view_set_show_expanders (tree_view, g_value_get_boolean (value));
      break;
    case PROP_LEVEL_INDENTATION:
      if (tree_view->priv->level_indentation != g_value_get_int (value))
        {
          tree_view->priv->level_indentation = g_value_get_int (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_RUBBER_BANDING:
      if (tree_view->priv->rubber_banding_enable != g_value_get_boolean (value))
        {
          tree_view->priv->rubber_banding_enable = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_ENABLE_GRID_LINES:
      ctk_tree_view_set_grid_lines (tree_view, g_value_get_enum (value));
      break;
    case PROP_ENABLE_TREE_LINES:
      ctk_tree_view_set_enable_tree_lines (tree_view, g_value_get_boolean (value));
      break;
    case PROP_TOOLTIP_COLUMN:
      ctk_tree_view_set_tooltip_column (tree_view, g_value_get_int (value));
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      ctk_tree_view_set_activate_on_single_click (tree_view, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_tree_view_get_property (GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
  CtkTreeView *tree_view;

  tree_view = CTK_TREE_VIEW (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, tree_view->priv->model);
      break;
    case PROP_HADJUSTMENT:
      g_value_set_object (value, tree_view->priv->hadjustment);
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value, tree_view->priv->vadjustment);
      break;
    case PROP_HSCROLL_POLICY:
      g_value_set_enum (value, tree_view->priv->hscroll_policy);
      break;
    case PROP_VSCROLL_POLICY:
      g_value_set_enum (value, tree_view->priv->vscroll_policy);
      break;
    case PROP_HEADERS_VISIBLE:
      g_value_set_boolean (value, ctk_tree_view_get_headers_visible (tree_view));
      break;
    case PROP_HEADERS_CLICKABLE:
      g_value_set_boolean (value, ctk_tree_view_get_headers_clickable (tree_view));
      break;
    case PROP_EXPANDER_COLUMN:
      g_value_set_object (value, tree_view->priv->expander_column);
      break;
    case PROP_REORDERABLE:
      g_value_set_boolean (value, tree_view->priv->reorderable);
      break;
    case PROP_RULES_HINT:
      g_value_set_boolean (value, tree_view->priv->has_rules);
      break;
    case PROP_ENABLE_SEARCH:
      g_value_set_boolean (value, tree_view->priv->enable_search);
      break;
    case PROP_SEARCH_COLUMN:
      g_value_set_int (value, tree_view->priv->search_column);
      break;
    case PROP_FIXED_HEIGHT_MODE:
      g_value_set_boolean (value, tree_view->priv->fixed_height_mode);
      break;
    case PROP_HOVER_SELECTION:
      g_value_set_boolean (value, tree_view->priv->hover_selection);
      break;
    case PROP_HOVER_EXPAND:
      g_value_set_boolean (value, tree_view->priv->hover_expand);
      break;
    case PROP_SHOW_EXPANDERS:
      g_value_set_boolean (value, tree_view->priv->show_expanders);
      break;
    case PROP_LEVEL_INDENTATION:
      g_value_set_int (value, tree_view->priv->level_indentation);
      break;
    case PROP_RUBBER_BANDING:
      g_value_set_boolean (value, tree_view->priv->rubber_banding_enable);
      break;
    case PROP_ENABLE_GRID_LINES:
      g_value_set_enum (value, tree_view->priv->grid_lines);
      break;
    case PROP_ENABLE_TREE_LINES:
      g_value_set_boolean (value, tree_view->priv->tree_lines_enabled);
      break;
    case PROP_TOOLTIP_COLUMN:
      g_value_set_int (value, tree_view->priv->tooltip_column);
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      g_value_set_boolean (value, tree_view->priv->activate_on_single_click);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_tree_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (ctk_tree_view_parent_class)->finalize (object);
}


static CtkBuildableIface *parent_buildable_iface;

static void
ctk_tree_view_buildable_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = ctk_tree_view_buildable_add_child;
  iface->get_internal_child = ctk_tree_view_buildable_get_internal_child;
}

static void
ctk_tree_view_buildable_add_child (CtkBuildable *tree_view,
				   CtkBuilder  *builder G_GNUC_UNUSED,
				   GObject     *child,
				   const gchar *type G_GNUC_UNUSED)
{
  ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), CTK_TREE_VIEW_COLUMN (child));
}

static GObject *
ctk_tree_view_buildable_get_internal_child (CtkBuildable      *buildable,
					    CtkBuilder        *builder,
					    const gchar       *childname)
{
    if (strcmp (childname, "selection") == 0)
      return G_OBJECT (CTK_TREE_VIEW (buildable)->priv->selection);
    
    return parent_buildable_iface->get_internal_child (buildable,
						       builder,
						       childname);
}

/* CtkWidget Methods
 */

static void
ctk_tree_view_free_rbtree (CtkTreeView *tree_view)
{
  _ctk_rbtree_free (tree_view->priv->tree);

  tree_view->priv->tree = NULL;
  tree_view->priv->button_pressed_node = NULL;
  tree_view->priv->button_pressed_tree = NULL;
  tree_view->priv->prelight_tree = NULL;
  tree_view->priv->prelight_node = NULL;
}

static void
ctk_tree_view_destroy_search_window (CtkTreeView *tree_view)
{
  ctk_widget_destroy (tree_view->priv->search_window);

  tree_view->priv->search_window = NULL;
  tree_view->priv->search_entry = NULL;
  tree_view->priv->search_entry_changed_id = 0;
}

static void
ctk_tree_view_destroy (CtkWidget *widget)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  GList *list;

  ctk_tree_view_stop_editing (tree_view, TRUE);
  ctk_tree_view_stop_rubber_band (tree_view);

  if (tree_view->priv->columns != NULL)
    {
      list = tree_view->priv->columns;
      while (list)
	{
	  CtkTreeViewColumn *column;
	  column = CTK_TREE_VIEW_COLUMN (list->data);
	  list = list->next;
	  ctk_tree_view_remove_column (tree_view, column);
	}
      tree_view->priv->columns = NULL;
    }

  if (tree_view->priv->tree != NULL)
    {
      ctk_tree_view_unref_and_check_selection_tree (tree_view, tree_view->priv->tree);

      ctk_tree_view_free_rbtree (tree_view);
    }

  if (tree_view->priv->selection != NULL)
    {
      _ctk_tree_selection_set_tree_view (tree_view->priv->selection, NULL);
      g_object_unref (tree_view->priv->selection);
      tree_view->priv->selection = NULL;
    }

  if (tree_view->priv->scroll_to_path != NULL)
    {
      ctk_tree_row_reference_free (tree_view->priv->scroll_to_path);
      tree_view->priv->scroll_to_path = NULL;
    }

  if (tree_view->priv->drag_dest_row != NULL)
    {
      ctk_tree_row_reference_free (tree_view->priv->drag_dest_row);
      tree_view->priv->drag_dest_row = NULL;
    }

  if (tree_view->priv->top_row != NULL)
    {
      ctk_tree_row_reference_free (tree_view->priv->top_row);
      tree_view->priv->top_row = NULL;
    }

  if (tree_view->priv->column_drop_func_data &&
      tree_view->priv->column_drop_func_data_destroy)
    {
      tree_view->priv->column_drop_func_data_destroy (tree_view->priv->column_drop_func_data);
      tree_view->priv->column_drop_func_data = NULL;
    }

  if (tree_view->priv->destroy_count_destroy &&
      tree_view->priv->destroy_count_data)
    {
      tree_view->priv->destroy_count_destroy (tree_view->priv->destroy_count_data);
      tree_view->priv->destroy_count_data = NULL;
    }

  ctk_tree_row_reference_free (tree_view->priv->anchor);
  tree_view->priv->anchor = NULL;

  /* destroy interactive search dialog */
  if (tree_view->priv->search_window)
    {
      ctk_tree_view_destroy_search_window (tree_view);
      if (tree_view->priv->typeselect_flush_timeout)
	{
	  g_source_remove (tree_view->priv->typeselect_flush_timeout);
	  tree_view->priv->typeselect_flush_timeout = 0;
	}
    }

  if (tree_view->priv->search_custom_entry_set)
    {
      g_signal_handlers_disconnect_by_func (tree_view->priv->search_entry,
                                            G_CALLBACK (ctk_tree_view_search_init),
                                            tree_view);
      g_signal_handlers_disconnect_by_func (tree_view->priv->search_entry,
                                            G_CALLBACK (ctk_tree_view_search_key_press_event),
                                            tree_view);

      g_object_unref (tree_view->priv->search_entry);

      tree_view->priv->search_entry = NULL;
      tree_view->priv->search_custom_entry_set = FALSE;
    }

  if (tree_view->priv->search_destroy && tree_view->priv->search_user_data)
    {
      tree_view->priv->search_destroy (tree_view->priv->search_user_data);
      tree_view->priv->search_user_data = NULL;
    }

  if (tree_view->priv->search_position_destroy && tree_view->priv->search_position_user_data)
    {
      tree_view->priv->search_position_destroy (tree_view->priv->search_position_user_data);
      tree_view->priv->search_position_user_data = NULL;
    }

  if (tree_view->priv->row_separator_destroy && tree_view->priv->row_separator_data)
    {
      tree_view->priv->row_separator_destroy (tree_view->priv->row_separator_data);
      tree_view->priv->row_separator_data = NULL;
    }
  
  ctk_tree_view_set_model (tree_view, NULL);

  if (tree_view->priv->hadjustment)
    {
      g_object_unref (tree_view->priv->hadjustment);
      tree_view->priv->hadjustment = NULL;
    }
  if (tree_view->priv->vadjustment)
    {
      g_object_unref (tree_view->priv->vadjustment);
      tree_view->priv->vadjustment = NULL;
    }

  if (tree_view->priv->pixel_cache)
    _ctk_pixel_cache_free (tree_view->priv->pixel_cache);
  tree_view->priv->pixel_cache = NULL;

  g_clear_object (&tree_view->priv->multipress_gesture);
  g_clear_object (&tree_view->priv->drag_gesture);
  g_clear_object (&tree_view->priv->column_multipress_gesture);
  g_clear_object (&tree_view->priv->column_drag_gesture);

  CTK_WIDGET_CLASS (ctk_tree_view_parent_class)->destroy (widget);
}

/* CtkWidget::map helper */
static void
ctk_tree_view_map_buttons (CtkTreeView *tree_view)
{
  GList *list;

  g_return_if_fail (ctk_widget_get_mapped (CTK_WIDGET (tree_view)));

  if (tree_view->priv->headers_visible)
    {
      CtkTreeViewColumn *column;
      CtkWidget         *button;
      CdkWindow         *window;

      for (list = tree_view->priv->columns; list; list = list->next)
	{
	  column = list->data;
	  button = ctk_tree_view_column_get_button (column);

          if (ctk_tree_view_column_get_visible (column) && button)
            ctk_widget_show_now (button);

          if (ctk_widget_get_visible (button) &&
              !ctk_widget_get_mapped (button))
            ctk_widget_map (button);
	}
      for (list = tree_view->priv->columns; list; list = list->next)
	{
	  column = list->data;
	  if (ctk_tree_view_column_get_visible (column) == FALSE)
	    continue;

	  window = _ctk_tree_view_column_get_window (column);
	  if (ctk_tree_view_column_get_resizable (column))
	    {
	      cdk_window_raise (window);
	      cdk_window_show (window);
	    }
	  else
	    cdk_window_hide (window);
	}
      cdk_window_show (tree_view->priv->header_window);
    }
}

static void
ctk_tree_view_map (CtkWidget *widget)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  GList *tmp_list;

  _ctk_pixel_cache_map (tree_view->priv->pixel_cache);

  ctk_widget_set_mapped (widget, TRUE);

  tmp_list = tree_view->priv->children;
  while (tmp_list)
    {
      CtkTreeViewChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      if (ctk_widget_get_visible (child->widget))
	{
	  if (!ctk_widget_get_mapped (child->widget))
	    ctk_widget_map (child->widget);
	}
    }
  cdk_window_show (tree_view->priv->bin_window);

  ctk_tree_view_map_buttons (tree_view);

  cdk_window_show (ctk_widget_get_window (widget));
}

static void
ctk_tree_view_unmap (CtkWidget *widget)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);

  CTK_WIDGET_CLASS (ctk_tree_view_parent_class)->unmap (widget);

  _ctk_pixel_cache_unmap (tree_view->priv->pixel_cache);
}

static void
ctk_tree_view_bin_window_invalidate_handler (CdkWindow *window,
					     cairo_region_t *region)
{
  gpointer widget;
  CtkTreeView *tree_view;
  int y;

  cdk_window_get_user_data (window, &widget);
  tree_view = CTK_TREE_VIEW (widget);

  y = ctk_adjustment_get_value (tree_view->priv->vadjustment);
  cairo_region_translate (region,
			  0, y);
  _ctk_pixel_cache_invalidate (tree_view->priv->pixel_cache, region);
  cairo_region_translate (region,
			  0, -y);
}

static void
ctk_tree_view_queue_draw_region (CtkWidget *widget,
				 const cairo_region_t *region)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);

  /* There is no way we can know if a region targets the
     not-currently-visible but in pixel cache region, so we
     always just invalidate the whole thing whenever the
     tree view gets a queue draw. This doesn't normally happen
     in normal scrolling cases anyway. */
  _ctk_pixel_cache_invalidate (tree_view->priv->pixel_cache, NULL);

  CTK_WIDGET_CLASS (ctk_tree_view_parent_class)->queue_draw_region (widget,
								    region);
}

static void
ctk_tree_view_realize (CtkWidget *widget)
{
  CtkAllocation allocation;
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  CdkWindow *window;
  CdkWindowAttr attributes;
  GList *tmp_list;
  gint attributes_mask;

  ctk_widget_set_realized (widget, TRUE);

  ctk_widget_get_allocation (widget, &allocation);

  /* Make the main, clipping window */
  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = CDK_INPUT_OUTPUT;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.event_mask = CDK_VISIBILITY_NOTIFY_MASK;

  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;

  window = cdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_set_window (widget, window);
  ctk_widget_register_window (widget, window);

  ctk_widget_get_allocation (widget, &allocation);

  /* Make the window for the tree */
  attributes.x = 0;
  attributes.y = ctk_tree_view_get_effective_header_height (tree_view);
  attributes.width = MAX (tree_view->priv->width, allocation.width);
  attributes.height = allocation.height;
  attributes.event_mask = (CDK_SCROLL_MASK |
                           CDK_SMOOTH_SCROLL_MASK |
                           CDK_POINTER_MOTION_MASK |
                           CDK_ENTER_NOTIFY_MASK |
                           CDK_LEAVE_NOTIFY_MASK |
                           CDK_BUTTON_PRESS_MASK |
                           CDK_BUTTON_RELEASE_MASK |
                           ctk_widget_get_events (widget));

  tree_view->priv->bin_window = cdk_window_new (window,
						&attributes, attributes_mask);
  ctk_widget_register_window (widget, tree_view->priv->bin_window);
  cdk_window_set_invalidate_handler (tree_view->priv->bin_window,
				     ctk_tree_view_bin_window_invalidate_handler);

  ctk_widget_get_allocation (widget, &allocation);

  /* Make the column header window */
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = MAX (tree_view->priv->width, allocation.width);
  attributes.height = tree_view->priv->header_height;
  attributes.event_mask = (CDK_SCROLL_MASK |
                           CDK_ENTER_NOTIFY_MASK |
                           CDK_LEAVE_NOTIFY_MASK |
                           CDK_BUTTON_PRESS_MASK |
                           CDK_BUTTON_RELEASE_MASK |
                           CDK_KEY_PRESS_MASK |
                           CDK_KEY_RELEASE_MASK |
                           ctk_widget_get_events (widget));

  tree_view->priv->header_window = cdk_window_new (window,
						   &attributes, attributes_mask);
  ctk_widget_register_window (widget, tree_view->priv->header_window);

  tmp_list = tree_view->priv->children;
  while (tmp_list)
    {
      CtkTreeViewChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      ctk_widget_set_parent_window (child->widget, tree_view->priv->bin_window);
    }

  for (tmp_list = tree_view->priv->columns; tmp_list; tmp_list = tmp_list->next)
    _ctk_tree_view_column_realize_button (CTK_TREE_VIEW_COLUMN (tmp_list->data));

  /* Need to call those here, since they create GCs */
  ctk_tree_view_set_grid_lines (tree_view, tree_view->priv->grid_lines);
  ctk_tree_view_set_enable_tree_lines (tree_view, tree_view->priv->tree_lines_enabled);

  install_presize_handler (tree_view); 

  ctk_gesture_set_window (tree_view->priv->multipress_gesture,
                          tree_view->priv->bin_window);
  ctk_gesture_set_window (tree_view->priv->drag_gesture,
                          tree_view->priv->bin_window);
}

static void
ctk_tree_view_unrealize (CtkWidget *widget)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  CtkTreeViewPrivate *priv = tree_view->priv;
  GList *list;

  if (priv->scroll_timeout != 0)
    {
      g_source_remove (priv->scroll_timeout);
      priv->scroll_timeout = 0;
    }

  if (priv->auto_expand_timeout != 0)
    {
      g_source_remove (priv->auto_expand_timeout);
      priv->auto_expand_timeout = 0;
    }

  if (priv->open_dest_timeout != 0)
    {
      g_source_remove (priv->open_dest_timeout);
      priv->open_dest_timeout = 0;
    }

  if (priv->presize_handler_tick_cb != 0)
    {
      ctk_widget_remove_tick_callback (widget, priv->presize_handler_tick_cb);
      priv->presize_handler_tick_cb = 0;
    }

  if (priv->validate_rows_timer != 0)
    {
      g_source_remove (priv->validate_rows_timer);
      priv->validate_rows_timer = 0;
    }

  if (priv->scroll_sync_timer != 0)
    {
      g_source_remove (priv->scroll_sync_timer);
      priv->scroll_sync_timer = 0;
    }

  if (priv->typeselect_flush_timeout)
    {
      g_source_remove (priv->typeselect_flush_timeout);
      priv->typeselect_flush_timeout = 0;
    }
  
  for (list = priv->columns; list; list = list->next)
    _ctk_tree_view_column_unrealize_button (CTK_TREE_VIEW_COLUMN (list->data));

  ctk_widget_unregister_window (widget, priv->bin_window);
  cdk_window_destroy (priv->bin_window);
  priv->bin_window = NULL;

  ctk_widget_unregister_window (widget, priv->header_window);
  cdk_window_destroy (priv->header_window);
  priv->header_window = NULL;

  if (priv->drag_window)
    {
      ctk_widget_unregister_window (widget, priv->drag_window);
      cdk_window_destroy (priv->drag_window);
      priv->drag_window = NULL;
    }

  if (priv->drag_highlight_window)
    {
      ctk_widget_unregister_window (widget, priv->drag_highlight_window);
      cdk_window_destroy (priv->drag_highlight_window);
      priv->drag_highlight_window = NULL;
    }

  ctk_gesture_set_window (tree_view->priv->multipress_gesture, NULL);
  ctk_gesture_set_window (tree_view->priv->drag_gesture, NULL);

  CTK_WIDGET_CLASS (ctk_tree_view_parent_class)->unrealize (widget);
}

/* CtkWidget::get_preferred_height helper */
static void
ctk_tree_view_update_height (CtkTreeView *tree_view)
{
  GList *list;

  tree_view->priv->header_height = 0;

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      CtkRequisition     requisition;
      CtkTreeViewColumn *column = list->data;
      CtkWidget         *button = ctk_tree_view_column_get_button (column);

      if (button == NULL)
        continue;

      ctk_widget_get_preferred_size (button, &requisition, NULL);
      tree_view->priv->header_height = MAX (tree_view->priv->header_height, requisition.height);
    }
}

static gint
ctk_tree_view_get_height (CtkTreeView *tree_view)
{
  if (tree_view->priv->tree == NULL)
    return 0;
  else
    return tree_view->priv->tree->root->offset;
}

static void
ctk_tree_view_get_preferred_width (CtkWidget *widget,
				   gint      *minimum,
				   gint      *natural)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  GList *list;
  CtkTreeViewColumn *column;
  gint width = 0;

  /* we validate some rows initially just to make sure we have some size.
   * In practice, with a lot of static lists, this should get a good width.
   */
  do_validate_rows (tree_view, FALSE);

  /* keep this in sync with size_allocate below */
  for (list = tree_view->priv->columns; list; list = list->next)
    {
      column = list->data;
      if (!ctk_tree_view_column_get_visible (column) || column == tree_view->priv->drag_column)
	continue;

      width += _ctk_tree_view_column_request_width (column);
    }

  *minimum = *natural = width;
}

static void
ctk_tree_view_get_preferred_height (CtkWidget *widget,
				    gint      *minimum,
				    gint      *natural)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  gint height;

  ctk_tree_view_update_height (tree_view);

  height = ctk_tree_view_get_height (tree_view) + ctk_tree_view_get_effective_header_height (tree_view);

  *minimum = *natural = height;
}

static int
ctk_tree_view_calculate_width_before_expander (CtkTreeView *tree_view)
{
  int width = 0;
  GList *list;
  gboolean rtl;

  rtl = (ctk_widget_get_direction (CTK_WIDGET (tree_view)) == CTK_TEXT_DIR_RTL);
  for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
       list->data != tree_view->priv->expander_column;
       list = (rtl ? list->prev : list->next))
    {
      CtkTreeViewColumn *column = list->data;

      width += ctk_tree_view_column_get_width (column);
    }

  return width;
}

/* CtkWidget::size_allocate helper */
static void
ctk_tree_view_size_allocate_columns (CtkWidget *widget,
				     gboolean  *width_changed)
{
  CtkTreeView *tree_view;
  GList *list, *first_column, *last_column;
  CtkTreeViewColumn *column;
  CtkAllocation widget_allocation;
  gint width = 0;
  gint extra, extra_per_column, extra_for_last;
  gint full_requested_width = 0;
  gint number_of_expand_columns = 0;
  gboolean rtl;
  gboolean update_expand;
  
  tree_view = CTK_TREE_VIEW (widget);

  for (last_column = g_list_last (tree_view->priv->columns);
       last_column &&
       !(ctk_tree_view_column_get_visible (CTK_TREE_VIEW_COLUMN (last_column->data)));
       last_column = last_column->prev)
    ;
  if (last_column == NULL)
    return;

  for (first_column = g_list_first (tree_view->priv->columns);
       first_column &&
       !(ctk_tree_view_column_get_visible (CTK_TREE_VIEW_COLUMN (first_column->data)));
       first_column = first_column->next)
    ;

  if (first_column == NULL)
    return;

  rtl = (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL);

  /* find out how many extra space and expandable columns we have */
  for (list = tree_view->priv->columns; list != last_column->next; list = list->next)
    {
      column = (CtkTreeViewColumn *)list->data;

      if (!ctk_tree_view_column_get_visible (column) || column == tree_view->priv->drag_column)
	continue;

      full_requested_width += _ctk_tree_view_column_request_width (column);

      if (ctk_tree_view_column_get_expand (column))
	number_of_expand_columns++;
    }

  /* Only update the expand value if the width of the widget has changed,
   * or the number of expand columns has changed, or if there are no expand
   * columns, or if we didn't have an size-allocation yet after the
   * last validated node.
   */
  update_expand = (width_changed && *width_changed == TRUE)
      || number_of_expand_columns != tree_view->priv->last_number_of_expand_columns
      || number_of_expand_columns == 0
      || tree_view->priv->post_validation_flag == TRUE;

  tree_view->priv->post_validation_flag = FALSE;

  ctk_widget_get_allocation (widget, &widget_allocation);
  if (!update_expand)
    {
      extra = tree_view->priv->last_extra_space;
      extra_for_last = MAX (widget_allocation.width - full_requested_width - extra, 0);
    }
  else
    {
      extra = MAX (widget_allocation.width - full_requested_width, 0);
      extra_for_last = 0;

      tree_view->priv->last_extra_space = extra;
    }

  if (number_of_expand_columns > 0)
    extra_per_column = extra/number_of_expand_columns;
  else
    extra_per_column = 0;

  if (update_expand)
    {
      tree_view->priv->last_extra_space_per_column = extra_per_column;
      tree_view->priv->last_number_of_expand_columns = number_of_expand_columns;
    }

  for (list = (rtl ? last_column : first_column); 
       list != (rtl ? first_column->prev : last_column->next);
       list = (rtl ? list->prev : list->next)) 
    {
      gint column_width;

      column = list->data;

      if (!ctk_tree_view_column_get_visible (column) || column == tree_view->priv->drag_column)
	continue;

      column_width = _ctk_tree_view_column_request_width (column);

      if (ctk_tree_view_column_get_expand (column))
	{
	  if (number_of_expand_columns == 1)
	    {
	      /* We add the remander to the last column as
	       * */
	      column_width += extra;
	    }
	  else
	    {
	      column_width += extra_per_column;
	      extra -= extra_per_column;
	      number_of_expand_columns --;
	    }
	}
      else if (number_of_expand_columns == 0 &&
	       list == last_column)
	{
	  column_width += extra;
	}

      /* In addition to expand, the last column can get even more
       * extra space so all available space is filled up.
       */
      if (extra_for_last > 0 && list == last_column)
	column_width += extra_for_last;

      _ctk_tree_view_column_allocate (column, width, column_width);

      width += column_width;
    }

  /* We change the width here.  The user might have been resizing columns,
   * which changes the total width of the tree view.  This is of
   * importance for getting the horizontal scroll bar right.
   */
  if (tree_view->priv->width != width)
    {
      tree_view->priv->width = width;
      if (width_changed)
        *width_changed = TRUE;
    }
}

/* CtkWidget::size_allocate helper */
static void
ctk_tree_view_size_allocate_drag_column (CtkWidget *widget)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  CtkAllocation drag_allocation;
  CtkWidget *button;

  if (tree_view->priv->drag_column == NULL)
    return;

  button = ctk_tree_view_column_get_button (tree_view->priv->drag_column);

  drag_allocation.x = 0;
  drag_allocation.y = 0;
  drag_allocation.width = cdk_window_get_width (tree_view->priv->drag_window);
  drag_allocation.height = cdk_window_get_height (tree_view->priv->drag_window);
  ctk_widget_size_allocate (button, &drag_allocation);
}

static void
ctk_tree_view_size_allocate (CtkWidget     *widget,
			     CtkAllocation *allocation)
{
  CtkAllocation widget_allocation;
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  GList *tmp_list;
  gboolean width_changed = FALSE;
  gint old_width;
  double page_size;

  ctk_widget_get_allocation (widget, &widget_allocation);
  old_width = widget_allocation.width;
  if (allocation->width != widget_allocation.width)
    width_changed = TRUE;

  ctk_widget_set_allocation (widget, allocation);

  /* We size-allocate the columns first because the width of the
   * tree view (used in updating the adjustments below) might change.
   */
  ctk_tree_view_size_allocate_columns (widget, &width_changed);
  ctk_tree_view_size_allocate_drag_column (widget);

  g_object_freeze_notify (G_OBJECT (tree_view->priv->hadjustment));
  ctk_adjustment_set_page_size (tree_view->priv->hadjustment,
                                allocation->width);
  ctk_adjustment_set_page_increment (tree_view->priv->hadjustment,
                                     allocation->width * 0.9);
  ctk_adjustment_set_step_increment (tree_view->priv->hadjustment,
                                     allocation->width * 0.1);
  ctk_adjustment_set_lower (tree_view->priv->hadjustment, 0);
  ctk_adjustment_set_upper (tree_view->priv->hadjustment,
                            MAX (ctk_adjustment_get_page_size (tree_view->priv->hadjustment),
                                 tree_view->priv->width));
  g_object_thaw_notify (G_OBJECT (tree_view->priv->hadjustment));

  if (ctk_widget_get_direction(widget) == CTK_TEXT_DIR_RTL)   
    {
      if (allocation->width < tree_view->priv->width)
        {
	  if (tree_view->priv->init_hadjust_value)
	    {
	      ctk_adjustment_set_value (tree_view->priv->hadjustment,
                                        MAX (tree_view->priv->width -
                                             allocation->width, 0));
	      tree_view->priv->init_hadjust_value = FALSE;
	    }
	  else if (allocation->width != old_width)
	    {
	      ctk_adjustment_set_value (tree_view->priv->hadjustment,
                                        CLAMP (ctk_adjustment_get_value (tree_view->priv->hadjustment) - allocation->width + old_width,
                                               0,
                                               tree_view->priv->width - allocation->width));
	    }
	}
      else
        {
          ctk_adjustment_set_value (tree_view->priv->hadjustment, 0);
	  tree_view->priv->init_hadjust_value = TRUE;
	}
    }
  else
    if (ctk_adjustment_get_value (tree_view->priv->hadjustment) + allocation->width > tree_view->priv->width)
      ctk_adjustment_set_value (tree_view->priv->hadjustment,
                                MAX (tree_view->priv->width -
                                     allocation->width, 0));

  page_size = allocation->height - ctk_tree_view_get_effective_header_height (tree_view);
  ctk_adjustment_configure (tree_view->priv->vadjustment,
                            ctk_adjustment_get_value (tree_view->priv->vadjustment),
                            0,
                            MAX (page_size, ctk_tree_view_get_height (tree_view)),
                            page_size * 0.1,
                            page_size * 0.9,
                            page_size);
 
  /* now the adjustments and window sizes are in sync, we can sync toprow/dy again */
  if (ctk_tree_row_reference_valid (tree_view->priv->top_row))
    ctk_tree_view_top_row_to_dy (tree_view);
  else
    ctk_tree_view_dy_to_top_row (tree_view);
  
  if (ctk_widget_get_realized (widget))
    {
      cdk_window_move_resize (ctk_widget_get_window (widget),
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);
      cdk_window_move_resize (tree_view->priv->header_window,
			      - (gint) ctk_adjustment_get_value (tree_view->priv->hadjustment),
			      0,
			      MAX (tree_view->priv->width, allocation->width),
			      tree_view->priv->header_height);
      cdk_window_move_resize (tree_view->priv->bin_window,
			      - (gint) ctk_adjustment_get_value (tree_view->priv->hadjustment),
			      ctk_tree_view_get_effective_header_height (tree_view),
			      MAX (tree_view->priv->width, allocation->width),
			      allocation->height - ctk_tree_view_get_effective_header_height (tree_view));

      if (tree_view->priv->tree == NULL)
        invalidate_empty_focus (tree_view);

      if (width_changed && tree_view->priv->expander_column)
        {
          /* Might seem awkward, but is the best heuristic I could come up
           * with.  Only if the width of the columns before the expander
           * changes, we will update the prelight status.  It is this
           * width that makes the expander move vertically.  Always updating
           * prelight status causes trouble with hover selections.
           */
          gint width_before_expander;

          width_before_expander = ctk_tree_view_calculate_width_before_expander (tree_view);

          if (tree_view->priv->prev_width_before_expander
              != width_before_expander)
              update_prelight (tree_view,
                               tree_view->priv->event_last_x,
                               tree_view->priv->event_last_y);

          tree_view->priv->prev_width_before_expander = width_before_expander;
        }
    }

  for (tmp_list = tree_view->priv->children; tmp_list; tmp_list = tmp_list->next)
    {
      CtkTreeViewChild *child = tmp_list->data;
      CtkTreePath *path;
      CdkRectangle child_rect;
      int min_x, max_x, min_y, max_y;
      int size;
      CtkTextDirection direction;

      direction = ctk_widget_get_direction (child->widget);
      path = _ctk_tree_path_new_from_rbtree (child->tree, child->node);
      ctk_tree_view_get_cell_area (tree_view, path, child->column, &child_rect);
      child_rect.x += child->border.left;
      child_rect.y += child->border.top;
      child_rect.width -= child->border.left + child->border.right;
      child_rect.height -= child->border.top + child->border.bottom;

      ctk_widget_get_preferred_width (CTK_WIDGET (child->widget), &size, NULL);

      if (size > child_rect.width)
        {
          /* Enlarge the child, extending it to the left (RTL) */
          if (direction == CTK_TEXT_DIR_RTL)
            child_rect.x -= (size - child_rect.width);
          /* or to the right (LTR) */
          else
            child_rect.x += 0;

          child_rect.width = size;
        }

      ctk_widget_get_preferred_height_for_width (CTK_WIDGET (child->widget),
                                                 child_rect.width,
                                                 &size, NULL);
      if (size > child_rect.height)
        {
          /* Enlarge the child, extending in both directions equally */
          child_rect.y -= (size - child_rect.height) / 2;
          child_rect.height = size;
        }

      /* push the rect back in the visible area if needed,
       * preferring the top left corner (for RTL)
       * or top right corner (for LTR)
       */
      min_x = ctk_adjustment_get_value (tree_view->priv->hadjustment);
      max_x = min_x + allocation->width - child_rect.width;
      min_y = 0;
      max_y = min_y + allocation->height - ctk_tree_view_get_effective_header_height (tree_view) - child_rect.height;

      if (direction == CTK_TEXT_DIR_LTR)
        /* Ensure that child's right edge is not sticking to the right
         * (if (child_rect.x > max_x) child_rect.x = max_x),
         * then ensure that child's left edge is visible and is not sticking to the left
         * (if (child_rect.x < min_x) child_rect.x = min_x).
         */
        child_rect.x = MAX (min_x, MIN (max_x, child_rect.x));
      else
        /* Ensure that child's left edge is not sticking to the left
         * (if (child_rect.x < min_x) child_rect.x = min_x),
         * then ensure that child's right edge is visible and is not sticking to the right
         * (if (child_rect.x > max_x) child_rect.x = max_x).
         */
        child_rect.x = MIN (max_x, MAX (min_x, child_rect.x));

      child_rect.y = MAX (min_y, MIN (max_y, child_rect.y));

      ctk_tree_path_free (path);
      ctk_widget_size_allocate (child->widget, &child_rect);
    }
}

/* Grabs the focus and unsets the CTK_TREE_VIEW_DRAW_KEYFOCUS flag */
static void
grab_focus_and_unset_draw_keyfocus (CtkTreeView *tree_view)
{
  CtkWidget *widget = CTK_WIDGET (tree_view);

  if (ctk_widget_get_can_focus (widget) &&
      !ctk_widget_has_focus (widget) &&
      !_ctk_widget_get_shadowed (widget))
    ctk_widget_grab_focus (widget);

  tree_view->priv->draw_keyfocus = 0;
}

static inline gboolean
row_is_separator (CtkTreeView *tree_view,
		  CtkTreeIter *iter,
		  CtkTreePath *path)
{
  gboolean is_separator = FALSE;

  if (tree_view->priv->row_separator_func)
    {
      CtkTreeIter tmpiter;

      if (iter)
        tmpiter = *iter;
      else
        {
          if (!ctk_tree_model_get_iter (tree_view->priv->model, &tmpiter, path))
            return FALSE;
        }

      is_separator = tree_view->priv->row_separator_func (tree_view->priv->model,
                                                          &tmpiter,
                                                          tree_view->priv->row_separator_data);
    }

  return is_separator;
}

static int
ctk_tree_view_get_expander_size (CtkTreeView *tree_view)
{
  gint expander_size;
  gint horizontal_separator;

  ctk_widget_style_get (CTK_WIDGET (tree_view),
			"expander-size", &expander_size,
                        "horizontal-separator", &horizontal_separator,
			NULL);

  return expander_size + (horizontal_separator / 2);
}

static void
get_current_selection_modifiers (CtkWidget *widget,
                                 gboolean  *modify,
                                 gboolean  *extend)
{
  CdkModifierType state = 0;
  CdkModifierType mask;

  *modify = FALSE;
  *extend = FALSE;

  if (ctk_get_current_event_state (&state))
    {
      mask = ctk_widget_get_modifier_mask (widget, CDK_MODIFIER_INTENT_MODIFY_SELECTION);
      if ((state & mask) == mask)
        *modify = TRUE;
      mask = ctk_widget_get_modifier_mask (widget, CDK_MODIFIER_INTENT_EXTEND_SELECTION);
      if ((state & mask) == mask)
        *extend = TRUE;
    }
}

static void
ctk_tree_view_multipress_gesture_pressed (CtkGestureMultiPress *gesture,
                                          gint                  n_press,
                                          gdouble               x,
                                          gdouble               y,
                                          CtkTreeView          *tree_view)
{
  gint vertical_separator, horizontal_separator;
  CtkWidget *widget = CTK_WIDGET (tree_view);
  CdkRectangle background_area, cell_area;
  CtkTreeViewColumn *column = NULL;
  CdkEventSequence *sequence;
  CdkModifierType modifiers;
  const CdkEvent *event;
  gint new_y, y_offset;
  gint bin_x, bin_y;
  CtkTreePath *path;
  CtkRBNode *node;
  CtkRBTree *tree;
  gint depth;
  guint button;
  GList *list;
  gboolean rtl;

  rtl = (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL);
  ctk_tree_view_stop_editing (tree_view, FALSE);
  ctk_widget_style_get (widget,
			"vertical-separator", &vertical_separator,
			"horizontal-separator", &horizontal_separator,
			NULL);
  button = ctk_gesture_single_get_current_button (CTK_GESTURE_SINGLE (gesture));

  if (button > 3)
    {
      ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_DENIED);
      return;
    }

  /* Because grab_focus can cause reentrancy, we delay grab_focus until after
   * we're done handling the button press.
   */
  ctk_tree_view_convert_widget_to_bin_window_coords (tree_view, x, y,
                                                     &bin_x, &bin_y);
  ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_CLAIMED);

  if (n_press > 1)
    ctk_gesture_set_state (tree_view->priv->drag_gesture,
                           CTK_EVENT_SEQUENCE_DENIED);

  /* Empty tree? */
  if (tree_view->priv->tree == NULL)
    {
      grab_focus_and_unset_draw_keyfocus (tree_view);
      return;
    }

  /* are we in an arrow? */
  if (tree_view->priv->prelight_node &&
      tree_view->priv->arrow_prelit &&
      ctk_tree_view_draw_expanders (tree_view))
    {
      if (button == CDK_BUTTON_PRIMARY)
        {
          tree_view->priv->button_pressed_node = tree_view->priv->prelight_node;
          tree_view->priv->button_pressed_tree = tree_view->priv->prelight_tree;
          ctk_tree_view_queue_draw_arrow (tree_view,
                                          tree_view->priv->prelight_tree,
                                          tree_view->priv->prelight_node);
        }

      grab_focus_and_unset_draw_keyfocus (tree_view);
      return;
    }

  /* find the node that was clicked */
  new_y = TREE_WINDOW_Y_TO_RBTREE_Y(tree_view, bin_y);
  if (new_y < 0)
    new_y = 0;
  y_offset = -_ctk_rbtree_find_offset (tree_view->priv->tree, new_y, &tree, &node);

  if (node == NULL)
    {
      /* We clicked in dead space */
      grab_focus_and_unset_draw_keyfocus (tree_view);
      return;
    }

  /* Get the path and the node */
  path = _ctk_tree_path_new_from_rbtree (tree, node);

  if (row_is_separator (tree_view, NULL, path))
    {
      ctk_tree_path_free (path);
      grab_focus_and_unset_draw_keyfocus (tree_view);
      return;
    }

  depth = ctk_tree_path_get_depth (path);
  background_area.y = y_offset + bin_y;
  background_area.height = ctk_tree_view_get_row_height (tree_view, node);
  background_area.x = 0;

  /* Let the column have a chance at selecting it. */
  rtl = (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL);
  for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
       list; list = (rtl ? list->prev : list->next))
    {
      CtkTreeViewColumn *candidate = list->data;

      if (!ctk_tree_view_column_get_visible (candidate))
        continue;

      background_area.width = ctk_tree_view_column_get_width (candidate);
      if ((background_area.x > bin_x) ||
          (background_area.x + background_area.width <= bin_x))
        {
          background_area.x += background_area.width;
          continue;
        }

      /* we found the focus column */
      column = candidate;
      cell_area = background_area;
      cell_area.width -= horizontal_separator;
      cell_area.height -= vertical_separator;
      cell_area.x += horizontal_separator/2;
      cell_area.y += vertical_separator/2;
      if (ctk_tree_view_is_expander_column (tree_view, column))
        {
          if (!rtl)
            cell_area.x += (depth - 1) * tree_view->priv->level_indentation;
          cell_area.width -= (depth - 1) * tree_view->priv->level_indentation;

          if (ctk_tree_view_draw_expanders (tree_view))
            {
              gint expander_size = ctk_tree_view_get_expander_size (tree_view);
              if (!rtl)
                cell_area.x += depth * expander_size;
              cell_area.width -= depth * expander_size;
            }
        }
      break;
    }

  if (column == NULL)
    {
      ctk_tree_path_free (path);
      grab_focus_and_unset_draw_keyfocus (tree_view);
      ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_DENIED);
      return;
    }

  _ctk_tree_view_set_focus_column (tree_view, column);

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);
  cdk_event_get_state (event, &modifiers);

  /* decide if we edit */
  if (button == CDK_BUTTON_PRIMARY &&
      !(modifiers & ctk_accelerator_get_default_mod_mask ()))
    {
      CtkTreePath *anchor;
      CtkTreeIter iter;

      ctk_tree_model_get_iter (tree_view->priv->model, &iter, path);
      ctk_tree_view_column_cell_set_cell_data (column,
                                               tree_view->priv->model,
                                               &iter,
                                               CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_PARENT),
                                               node->children?TRUE:FALSE);

      if (tree_view->priv->anchor)
        anchor = ctk_tree_row_reference_get_path (tree_view->priv->anchor);
      else
        anchor = NULL;

      if ((anchor && !ctk_tree_path_compare (anchor, path))
          || !_ctk_tree_view_column_has_editable_cell (column))
        {
          CtkCellEditable *cell_editable = NULL;

          /* FIXME: get the right flags */
          guint flags = 0;

          if (_ctk_tree_view_column_cell_event (column,
                                                (CdkEvent *)event,
                                                &cell_area, flags))
            {
              CtkCellArea *area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (column));
              cell_editable = ctk_cell_area_get_edit_widget (area);

              if (cell_editable != NULL)
                {
                  ctk_tree_path_free (path);
                  ctk_tree_path_free (anchor);
                  return;
                }
            }
        }
      if (anchor)
        ctk_tree_path_free (anchor);
    }

  /* we only handle selection modifications on the first button press
   */
  if (n_press == 1)
    {
      CtkCellRenderer *focus_cell;
      gboolean modify, extend;

      get_current_selection_modifiers (widget, &modify, &extend);
      tree_view->priv->modify_selection_pressed = modify;
      tree_view->priv->extend_selection_pressed = extend;

      /* We update the focus cell here, this is also needed if the
       * column does not contain an editable cell.  In this case,
       * CtkCellArea did not receive the event for processing (and
       * could not update the focus cell).
       */
      focus_cell = _ctk_tree_view_column_get_cell_at_pos (column,
                                                          &cell_area,
                                                          &background_area,
                                                          bin_x, bin_y);

      if (focus_cell)
        ctk_tree_view_column_focus_cell (column, focus_cell);

      if (modify)
        {
          ctk_tree_view_real_set_cursor (tree_view, path, CLAMP_NODE);
          ctk_tree_view_real_toggle_cursor_row (tree_view);
        }
      else if (extend)
        {
          ctk_tree_view_real_set_cursor (tree_view, path, CLAMP_NODE);
          ctk_tree_view_real_select_cursor_row (tree_view, FALSE);
        }
      else
        {
          ctk_tree_view_real_set_cursor (tree_view, path, CLEAR_AND_SELECT | CLAMP_NODE);
        }

      tree_view->priv->modify_selection_pressed = FALSE;
      tree_view->priv->extend_selection_pressed = FALSE;
    }

  if (button == CDK_BUTTON_PRIMARY && n_press == 2)
    ctk_tree_view_row_activated (tree_view, path, column);
  else
    {
      if (n_press == 1)
        {
          tree_view->priv->button_pressed_node = tree_view->priv->prelight_node;
          tree_view->priv->button_pressed_tree = tree_view->priv->prelight_tree;
        }

      grab_focus_and_unset_draw_keyfocus (tree_view);
    }

  ctk_tree_path_free (path);

  if (n_press >= 2)
    ctk_event_controller_reset (CTK_EVENT_CONTROLLER (gesture));
}

static void
ctk_tree_view_drag_gesture_begin (CtkGestureDrag *gesture,
                                  gdouble         start_x,
                                  gdouble         start_y,
                                  CtkTreeView    *tree_view)
{
  gint bin_x, bin_y;
  CtkRBTree *tree;
  CtkRBNode *node;

  if (tree_view->priv->tree == NULL)
    {
      ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_DENIED);
      return;
    }

  ctk_tree_view_convert_widget_to_bin_window_coords (tree_view, start_x, start_y,
                                                     &bin_x, &bin_y);
  tree_view->priv->press_start_x = tree_view->priv->rubber_band_x = bin_x;
  tree_view->priv->press_start_y = tree_view->priv->rubber_band_y = bin_y;
  _ctk_rbtree_find_offset (tree_view->priv->tree, bin_y + tree_view->priv->dy,
                           &tree, &node);

  if (tree_view->priv->rubber_banding_enable
      && !CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED)
      && ctk_tree_selection_get_mode (tree_view->priv->selection) == CTK_SELECTION_MULTIPLE)
    {
      gboolean modify, extend;

      tree_view->priv->press_start_y += tree_view->priv->dy;
      tree_view->priv->rubber_band_y += tree_view->priv->dy;
      tree_view->priv->rubber_band_status = RUBBER_BAND_MAYBE_START;

      get_current_selection_modifiers (CTK_WIDGET (tree_view), &modify, &extend);
      tree_view->priv->rubber_band_modify = modify;
      tree_view->priv->rubber_band_extend = extend;
    }
}

static void
ctk_tree_view_column_multipress_gesture_pressed (CtkGestureMultiPress *gesture,
                                                 gint                  n_press,
                                                 gdouble               x G_GNUC_UNUSED,
                                                 gdouble               y G_GNUC_UNUSED,
                                                 CtkTreeView          *tree_view)
{
  CdkEventSequence *sequence;
  CtkTreeViewColumn *column;
  const CdkEvent *event;
  GList *list;
  gint i;

  if (n_press != 2)
    return;

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);

  for (i = 0, list = tree_view->priv->columns; list; list = list->next, i++)
    {
      column = list->data;

      if (event->any.window != _ctk_tree_view_column_get_window (column) ||
	  !ctk_tree_view_column_get_resizable (column))
        continue;

      if (ctk_tree_view_column_get_sizing (column) != CTK_TREE_VIEW_COLUMN_AUTOSIZE)
        {
          ctk_tree_view_column_set_fixed_width (column, -1);
          ctk_tree_view_column_set_expand (column, FALSE);
          _ctk_tree_view_column_autosize (tree_view, column);
        }

      ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_CLAIMED);
      break;
    }
}

static void
ctk_tree_view_column_drag_gesture_begin (CtkGestureDrag *gesture,
                                         gdouble         start_x,
                                         gdouble         start_y G_GNUC_UNUSED,
                                         CtkTreeView    *tree_view)
{
  CdkEventSequence *sequence;
  CtkTreeViewColumn *column;
  const CdkEvent *event;
  CdkWindow *window;
  gboolean rtl;
  GList *list;
  gint i;

  rtl = (ctk_widget_get_direction (CTK_WIDGET (tree_view)) == CTK_TEXT_DIR_RTL);
  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);
  window = event->any.window;

  for (i = 0, list = tree_view->priv->columns; list; list = list->next, i++)
    {
      gpointer drag_data;
      gint column_width;

      column = list->data;

      if (window != _ctk_tree_view_column_get_window (column))
        continue;

      if (!ctk_tree_view_column_get_resizable (column))
        break;

      tree_view->priv->in_column_resize = TRUE;

      /* block attached dnd signal handler */
      drag_data = g_object_get_data (G_OBJECT (tree_view), "ctk-site-data");
      if (drag_data)
        g_signal_handlers_block_matched (tree_view,
                                         G_SIGNAL_MATCH_DATA,
                                         0, 0, NULL, NULL,
                                         drag_data);

      column_width = ctk_tree_view_column_get_width (column);
      ctk_tree_view_column_set_fixed_width (column, column_width);
      ctk_tree_view_column_set_expand (column, FALSE);

      tree_view->priv->drag_pos = i;
      tree_view->priv->x_drag = start_x + (rtl ? column_width : -column_width);

      if (!ctk_widget_has_focus (CTK_WIDGET (tree_view)))
        ctk_widget_grab_focus (CTK_WIDGET (tree_view));

      ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_CLAIMED);
      return;
    }
}

static void
ctk_tree_view_update_button_position (CtkTreeView       *tree_view,
                                      CtkTreeViewColumn *column)
{
  CtkTreeViewPrivate *priv = tree_view->priv;
  GList *column_el;

  column_el = g_list_find (priv->columns, column);
  g_return_if_fail (column_el != NULL);

  ctk_css_node_insert_after (priv->header_node,
                             ctk_widget_get_css_node (ctk_tree_view_column_get_button (column)),
                             column_el->prev ? ctk_widget_get_css_node (
                                ctk_tree_view_column_get_button (column_el->prev->data)) : NULL);
}

/* column drag gesture helper */
static gboolean
ctk_tree_view_button_release_drag_column (CtkTreeView *tree_view)
{
  CtkWidget *button, *widget = CTK_WIDGET (tree_view);
  GList *l;
  gboolean rtl;
  CtkStyleContext *context;

  rtl = (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL);

  /* Move the button back */
  button = ctk_tree_view_column_get_button (tree_view->priv->drag_column);

  context = ctk_widget_get_style_context (button);
  ctk_style_context_remove_class (context, CTK_STYLE_CLASS_DND);

  g_object_ref (button);
  ctk_container_remove (CTK_CONTAINER (tree_view), button);
  ctk_widget_set_parent_window (button, tree_view->priv->header_window);
  ctk_tree_view_update_button_position (tree_view, tree_view->priv->drag_column);
  ctk_widget_set_parent (button, CTK_WIDGET (tree_view));
  g_object_unref (button);
  ctk_widget_queue_resize (widget);
  if (ctk_tree_view_column_get_resizable (tree_view->priv->drag_column))
    {
      cdk_window_raise (_ctk_tree_view_column_get_window (tree_view->priv->drag_column));
      cdk_window_show (_ctk_tree_view_column_get_window (tree_view->priv->drag_column));
    }
  else
    cdk_window_hide (_ctk_tree_view_column_get_window (tree_view->priv->drag_column));

  ctk_widget_grab_focus (button);

  if (rtl)
    {
      if (tree_view->priv->cur_reorder &&
	  tree_view->priv->cur_reorder->right_column != tree_view->priv->drag_column)
	ctk_tree_view_move_column_after (tree_view, tree_view->priv->drag_column,
					 tree_view->priv->cur_reorder->right_column);
    }
  else
    {
      if (tree_view->priv->cur_reorder &&
	  tree_view->priv->cur_reorder->left_column != tree_view->priv->drag_column)
	ctk_tree_view_move_column_after (tree_view, tree_view->priv->drag_column,
					 tree_view->priv->cur_reorder->left_column);
    }
  tree_view->priv->drag_column = NULL;
  ctk_widget_unregister_window (widget, tree_view->priv->drag_window);
  cdk_window_destroy (tree_view->priv->drag_window);
  tree_view->priv->drag_window = NULL;

  for (l = tree_view->priv->column_drag_info; l != NULL; l = l->next)
    g_slice_free (CtkTreeViewColumnReorder, l->data);
  g_list_free (tree_view->priv->column_drag_info);
  tree_view->priv->column_drag_info = NULL;
  tree_view->priv->cur_reorder = NULL;

  if (tree_view->priv->drag_highlight_window)
    cdk_window_hide (tree_view->priv->drag_highlight_window);

  /* Reset our flags */
  tree_view->priv->drag_column_window_state = DRAG_COLUMN_WINDOW_STATE_UNSET;
  tree_view->priv->in_column_drag = FALSE;

  return TRUE;
}

/* column drag gesture helper */
static gboolean
ctk_tree_view_button_release_column_resize (CtkTreeView *tree_view)
{
  gpointer drag_data;

  tree_view->priv->drag_pos = -1;

  /* unblock attached dnd signal handler */
  drag_data = g_object_get_data (G_OBJECT (tree_view), "ctk-site-data");
  if (drag_data)
    g_signal_handlers_unblock_matched (tree_view,
				       G_SIGNAL_MATCH_DATA,
				       0, 0, NULL, NULL,
				       drag_data);

  tree_view->priv->in_column_resize = FALSE;
  return TRUE;
}

static void
ctk_tree_view_column_drag_gesture_end (CtkGestureDrag *gesture,
                                       gdouble         offset_x G_GNUC_UNUSED,
                                       gdouble         offset_y G_GNUC_UNUSED,
                                       CtkTreeView    *tree_view)
{
  CdkEventSequence *sequence;

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));

  /* Cancel reorder if the drag got cancelled */
  if (!ctk_gesture_handles_sequence (CTK_GESTURE (gesture), sequence))
    tree_view->priv->cur_reorder = NULL;

  if (tree_view->priv->in_column_drag)
    {
      CdkDevice *device;

      ctk_tree_view_button_release_drag_column (tree_view);
      device = ctk_gesture_get_device (CTK_GESTURE (gesture));
      cdk_seat_ungrab (cdk_device_get_seat (device));
    }
  else if (tree_view->priv->in_column_resize)
    ctk_tree_view_button_release_column_resize (tree_view);
}

static void
ctk_tree_view_drag_gesture_end (CtkGestureDrag *gesture G_GNUC_UNUSED,
                                gdouble         offset_x G_GNUC_UNUSED,
                                gdouble         offset_y G_GNUC_UNUSED,
                                CtkTreeView    *tree_view)
{
  ctk_tree_view_stop_rubber_band (tree_view);
}

static void
ctk_tree_view_multipress_gesture_released (CtkGestureMultiPress *gesture,
                                           gint                  n_press G_GNUC_UNUSED,
                                           gdouble               x G_GNUC_UNUSED,
                                           gdouble               y G_GNUC_UNUSED,
                                           CtkTreeView          *tree_view)
{
  gboolean modify, extend;
  guint button;

  button = ctk_gesture_single_get_current_button (CTK_GESTURE_SINGLE (gesture));

  if (button != CDK_BUTTON_PRIMARY ||
      tree_view->priv->button_pressed_node == NULL ||
      tree_view->priv->button_pressed_node != tree_view->priv->prelight_node)
    return;

  get_current_selection_modifiers (CTK_WIDGET (tree_view), &modify, &extend);

  if (tree_view->priv->arrow_prelit)
    {
      CtkTreePath *path = NULL;

      path = _ctk_tree_path_new_from_rbtree (tree_view->priv->button_pressed_tree,
                                             tree_view->priv->button_pressed_node);
      /* Actually activate the node */
      if (tree_view->priv->button_pressed_node->children == NULL)
        ctk_tree_view_real_expand_row (tree_view, path,
                                       tree_view->priv->button_pressed_tree,
                                       tree_view->priv->button_pressed_node,
                                       FALSE, TRUE);
      else
        ctk_tree_view_real_collapse_row (tree_view, path,
                                         tree_view->priv->button_pressed_tree,
                                         tree_view->priv->button_pressed_node, TRUE);
      ctk_tree_path_free (path);
    }
  else if (tree_view->priv->activate_on_single_click && !modify && !extend)
    {
      CtkTreePath *path = NULL;

      path = _ctk_tree_path_new_from_rbtree (tree_view->priv->button_pressed_tree,
                                             tree_view->priv->button_pressed_node);
      ctk_tree_view_row_activated (tree_view, path, tree_view->priv->focus_column);
      ctk_tree_path_free (path);
    }

  tree_view->priv->button_pressed_tree = NULL;
  tree_view->priv->button_pressed_node = NULL;
}

/* CtkWidget::motion_event function set.
 */

static gboolean
coords_are_over_arrow (CtkTreeView *tree_view,
                       CtkRBTree   *tree,
                       CtkRBNode   *node,
                       /* these are in bin window coords */
                       gint         x,
                       gint         y)
{
  CdkRectangle arrow;
  gint x2;

  if (!ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    return FALSE;

  if ((node->flags & CTK_RBNODE_IS_PARENT) == 0)
    return FALSE;

  arrow.y = ctk_tree_view_get_row_y_offset (tree_view, tree, node);
  arrow.height = ctk_tree_view_get_row_height (tree_view, node);

  ctk_tree_view_get_arrow_xrange (tree_view, tree, &arrow.x, &x2);

  arrow.width = x2 - arrow.x;

  return (x >= arrow.x &&
          x < (arrow.x + arrow.width) &&
	  y >= arrow.y &&
	  y < (arrow.y + arrow.height));
}

static gboolean
auto_expand_timeout (gpointer data)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (data);
  CtkTreePath *path;

  if (tree_view->priv->prelight_node)
    {
      path = _ctk_tree_path_new_from_rbtree (tree_view->priv->prelight_tree,
				             tree_view->priv->prelight_node);

      if (tree_view->priv->prelight_node->children)
	ctk_tree_view_collapse_row (tree_view, path);
      else
	ctk_tree_view_expand_row (tree_view, path, FALSE);

      ctk_tree_path_free (path);
    }

  tree_view->priv->auto_expand_timeout = 0;

  return FALSE;
}

static void
remove_auto_expand_timeout (CtkTreeView *tree_view)
{
  if (tree_view->priv->auto_expand_timeout != 0)
    {
      g_source_remove (tree_view->priv->auto_expand_timeout);
      tree_view->priv->auto_expand_timeout = 0;
    }
}

static void
do_prelight (CtkTreeView *tree_view,
             CtkRBTree   *tree,
             CtkRBNode   *node,
	     /* these are in bin_window coords */
             gint         x,
             gint         y)
{
  if (tree_view->priv->prelight_tree == tree &&
      tree_view->priv->prelight_node == node)
    {
      /*  We are still on the same node,
	  but we might need to take care of the arrow  */

      if (tree && node && ctk_tree_view_draw_expanders (tree_view))
	{
	  gboolean over_arrow;

	  over_arrow = coords_are_over_arrow (tree_view, tree, node, x, y);

	  if (over_arrow != tree_view->priv->arrow_prelit)
	    {
	      if (over_arrow)
                tree_view->priv->arrow_prelit = TRUE;
	      else
                tree_view->priv->arrow_prelit = FALSE;

	      ctk_tree_view_queue_draw_arrow (tree_view, tree, node);
	    }
	}

      return;
    }

  if (tree_view->priv->prelight_tree && tree_view->priv->prelight_node)
    {
      /*  Unprelight the old node and arrow  */

      CTK_RBNODE_UNSET_FLAG (tree_view->priv->prelight_node,
			     CTK_RBNODE_IS_PRELIT);

      if (tree_view->priv->arrow_prelit
	  && ctk_tree_view_draw_expanders (tree_view))
	{
          tree_view->priv->arrow_prelit = FALSE;
	  
	  ctk_tree_view_queue_draw_arrow (tree_view,
                                          tree_view->priv->prelight_tree,
                                          tree_view->priv->prelight_node);
	}

      _ctk_tree_view_queue_draw_node (tree_view,
				      tree_view->priv->prelight_tree,
				      tree_view->priv->prelight_node,
				      NULL);
    }


  if (tree_view->priv->hover_expand)
    remove_auto_expand_timeout (tree_view);

  /*  Set the new prelight values  */
  tree_view->priv->prelight_node = node;
  tree_view->priv->prelight_tree = tree;

  if (!node || !tree)
    return;

  /*  Prelight the new node and arrow  */

  if (ctk_tree_view_draw_expanders (tree_view)
      && coords_are_over_arrow (tree_view, tree, node, x, y))
    {
      tree_view->priv->arrow_prelit = TRUE;

      ctk_tree_view_queue_draw_arrow (tree_view, tree, node);
    }

  CTK_RBNODE_SET_FLAG (node, CTK_RBNODE_IS_PRELIT);

  _ctk_tree_view_queue_draw_node (tree_view, tree, node, NULL);

  if (tree_view->priv->hover_expand)
    {
      tree_view->priv->auto_expand_timeout = 
	cdk_threads_add_timeout (AUTO_EXPAND_TIMEOUT, auto_expand_timeout, tree_view);
      g_source_set_name_by_id (tree_view->priv->auto_expand_timeout, "[ctk+] auto_expand_timeout");
    }
}

static void
prelight_or_select (CtkTreeView *tree_view,
		    CtkRBTree   *tree,
		    CtkRBNode   *node,
		    /* these are in bin_window coords */
		    gint         x,
		    gint         y)
{
  CtkSelectionMode mode = ctk_tree_selection_get_mode (tree_view->priv->selection);
  
  if (tree_view->priv->hover_selection &&
      (mode == CTK_SELECTION_SINGLE || mode == CTK_SELECTION_BROWSE) &&
      !(tree_view->priv->edited_column &&
	ctk_cell_area_get_edit_widget 
	(ctk_cell_layout_get_area (CTK_CELL_LAYOUT (tree_view->priv->edited_column)))))
    {
      if (node)
	{
	  if (!CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
	    {
	      CtkTreePath *path;
	      
	      path = _ctk_tree_path_new_from_rbtree (tree, node);
	      ctk_tree_selection_select_path (tree_view->priv->selection, path);
	      if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
		{
                  tree_view->priv->draw_keyfocus = FALSE;
		  ctk_tree_view_real_set_cursor (tree_view, path, 0);
		}
	      ctk_tree_path_free (path);
	    }
	}

      else if (mode == CTK_SELECTION_SINGLE)
	ctk_tree_selection_unselect_all (tree_view->priv->selection);
    }

    do_prelight (tree_view, tree, node, x, y);
}

static void
ensure_unprelighted (CtkTreeView *tree_view)
{
  do_prelight (tree_view,
	       NULL, NULL,
	       -1000, -1000); /* coords not possibly over an arrow */

  g_assert (tree_view->priv->prelight_node == NULL);
}

static void
update_prelight (CtkTreeView *tree_view,
                 gint         x,
                 gint         y)
{
  int new_y;
  CtkRBTree *tree;
  CtkRBNode *node;

  if (tree_view->priv->tree == NULL)
    return;

  if (x == -10000)
    {
      ensure_unprelighted (tree_view);
      return;
    }

  new_y = TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, y);
  if (new_y < 0)
    new_y = 0;

  _ctk_rbtree_find_offset (tree_view->priv->tree,
                           new_y, &tree, &node);

  if (node)
    prelight_or_select (tree_view, tree, node, x, y);
}




/* Our motion arrow is either a box (in the case of the original spot)
 * or an arrow.  It is expander_size wide.
 */
/*
 * 11111111111111
 * 01111111111110
 * 00111111111100
 * 00011111111000
 * 00001111110000
 * 00000111100000
 * 00000111100000
 * 00000111100000
 * ~ ~ ~ ~ ~ ~ ~
 * 00000111100000
 * 00000111100000
 * 00000111100000
 * 00001111110000
 * 00011111111000
 * 00111111111100
 * 01111111111110
 * 11111111111111
 */

static void
ctk_tree_view_motion_draw_column_motion_arrow (CtkTreeView *tree_view)
{
  CtkTreeViewColumnReorder *reorder = tree_view->priv->cur_reorder;
  CtkWidget *widget = CTK_WIDGET (tree_view);
  cairo_surface_t *mask_image;
  cairo_region_t *mask_region;
  gint x;
  gint y;
  gint width;
  gint height;
  gint arrow_type = DRAG_COLUMN_WINDOW_STATE_UNSET;
  CdkWindowAttr attributes;
  guint attributes_mask;
  cairo_t *cr;

  if (!reorder ||
      reorder->left_column == tree_view->priv->drag_column ||
      reorder->right_column == tree_view->priv->drag_column)
    arrow_type = DRAG_COLUMN_WINDOW_STATE_ORIGINAL;
  else if (reorder->left_column || reorder->right_column)
    {
      CtkAllocation left_allocation, right_allocation;
      CdkRectangle visible_rect;
      CtkWidget *button;

      ctk_tree_view_get_visible_rect (tree_view, &visible_rect);
      if (reorder->left_column)
        {
	  button = ctk_tree_view_column_get_button (reorder->left_column);
          ctk_widget_get_allocation (button, &left_allocation);
          x = left_allocation.x + left_allocation.width;
        }
      else
        {
	  button = ctk_tree_view_column_get_button (reorder->right_column);
          ctk_widget_get_allocation (button, &right_allocation);
          x = right_allocation.x;
        }

      if (x < visible_rect.x)
	arrow_type = DRAG_COLUMN_WINDOW_STATE_ARROW_LEFT;
      else if (x > visible_rect.x + visible_rect.width)
	arrow_type = DRAG_COLUMN_WINDOW_STATE_ARROW_RIGHT;
      else
        arrow_type = DRAG_COLUMN_WINDOW_STATE_ARROW;
    }

  /* We want to draw the rectangle over the initial location. */
  if (arrow_type == DRAG_COLUMN_WINDOW_STATE_ORIGINAL)
    {
      if (tree_view->priv->drag_column_window_state != DRAG_COLUMN_WINDOW_STATE_ORIGINAL)
	{
          CtkAllocation drag_allocation;
	  CtkWidget    *button;

	  if (tree_view->priv->drag_highlight_window)
	    {
	      ctk_widget_unregister_window (CTK_WIDGET (tree_view), tree_view->priv->drag_highlight_window);
	      cdk_window_destroy (tree_view->priv->drag_highlight_window);
	    }

	  button = ctk_tree_view_column_get_button (tree_view->priv->drag_column);
	  attributes.window_type = CDK_WINDOW_CHILD;
	  attributes.wclass = CDK_INPUT_OUTPUT;
          attributes.x = tree_view->priv->drag_column_x;
          attributes.y = 0;
          ctk_widget_get_allocation (button, &drag_allocation);
	  width = attributes.width = drag_allocation.width;
	  height = attributes.height = drag_allocation.height;
	  attributes.visual = cdk_screen_get_rgba_visual (ctk_widget_get_screen (widget));
	  attributes.event_mask = CDK_VISIBILITY_NOTIFY_MASK | CDK_POINTER_MOTION_MASK;
	  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;
	  tree_view->priv->drag_highlight_window = cdk_window_new (tree_view->priv->header_window, &attributes, attributes_mask);
	  ctk_widget_register_window (CTK_WIDGET (tree_view), tree_view->priv->drag_highlight_window);

	  tree_view->priv->drag_column_window_state = DRAG_COLUMN_WINDOW_STATE_ORIGINAL;
	}
    }
  else if (arrow_type == DRAG_COLUMN_WINDOW_STATE_ARROW)
    {
      CtkAllocation button_allocation;
      CtkWidget    *button;

      width = ctk_tree_view_get_expander_size (tree_view);

      /* Get x, y, width, height of arrow */
      cdk_window_get_origin (tree_view->priv->header_window, &x, &y);
      if (reorder->left_column)
	{
	  button = ctk_tree_view_column_get_button (reorder->left_column);
          ctk_widget_get_allocation (button, &button_allocation);
	  x += button_allocation.x + button_allocation.width - width/2;
	  height = button_allocation.height;
	}
      else
	{
	  button = ctk_tree_view_column_get_button (reorder->right_column);
          ctk_widget_get_allocation (button, &button_allocation);
	  x += button_allocation.x - width/2;
	  height = button_allocation.height;
	}
      y -= width/2; /* The arrow takes up only half the space */
      height += width;

      /* Create the new window */
      if (tree_view->priv->drag_column_window_state != DRAG_COLUMN_WINDOW_STATE_ARROW)
	{
	  if (tree_view->priv->drag_highlight_window)
	    {
	      ctk_widget_unregister_window (CTK_WIDGET (tree_view), tree_view->priv->drag_highlight_window);
	      cdk_window_destroy (tree_view->priv->drag_highlight_window);
	    }

	  attributes.window_type = CDK_WINDOW_TEMP;
	  attributes.wclass = CDK_INPUT_OUTPUT;
	  attributes.visual = ctk_widget_get_visual (CTK_WIDGET (tree_view));
	  attributes.event_mask = CDK_VISIBILITY_NOTIFY_MASK | CDK_POINTER_MOTION_MASK;
	  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;
          attributes.x = x;
          attributes.y = y;
	  attributes.width = width;
	  attributes.height = height;
	  tree_view->priv->drag_highlight_window = cdk_window_new (cdk_screen_get_root_window (ctk_widget_get_screen (widget)),
								   &attributes, attributes_mask);
	  ctk_widget_register_window (CTK_WIDGET (tree_view), tree_view->priv->drag_highlight_window);

	  mask_image = cairo_image_surface_create (CAIRO_FORMAT_A1, width, height);

          cr = cairo_create (mask_image);
          cairo_move_to (cr, 0, 0);
          cairo_line_to (cr, width, 0);
          cairo_line_to (cr, width / 2., width / 2);
          cairo_move_to (cr, 0, height);
          cairo_line_to (cr, width, height);
          cairo_line_to (cr, width / 2., height - width / 2.);
          cairo_fill (cr);
          cairo_destroy (cr);

          mask_region = cdk_cairo_region_create_from_surface (mask_image);
	  cdk_window_shape_combine_region (tree_view->priv->drag_highlight_window,
					   mask_region, 0, 0);

          cairo_region_destroy (mask_region);
          cairo_surface_destroy (mask_image);
	}

      tree_view->priv->drag_column_window_state = DRAG_COLUMN_WINDOW_STATE_ARROW;
      cdk_window_move (tree_view->priv->drag_highlight_window, x, y);
    }
  else if (arrow_type == DRAG_COLUMN_WINDOW_STATE_ARROW_LEFT ||
	   arrow_type == DRAG_COLUMN_WINDOW_STATE_ARROW_RIGHT)
    {
      CtkAllocation allocation;
      CtkWidget    *button;
      gint          expander_size;

      expander_size = ctk_tree_view_get_expander_size (tree_view);

      /* Get x, y, width, height of arrow */
      width = expander_size/2; /* remember, the arrow only takes half the available width */
      cdk_window_get_origin (ctk_widget_get_window (widget),
                             &x, &y);
      if (arrow_type == DRAG_COLUMN_WINDOW_STATE_ARROW_RIGHT)
        {
          ctk_widget_get_allocation (widget, &allocation);
          x += allocation.width - width;
        }

      if (reorder->left_column)
        {
	  button = ctk_tree_view_column_get_button (reorder->left_column);
          ctk_widget_get_allocation (button, &allocation);
          height = allocation.height;
        }
      else
        {
	  button = ctk_tree_view_column_get_button (reorder->right_column);
          ctk_widget_get_allocation (button, &allocation);
          height = allocation.height;
        }

      y -= expander_size;
      height += 2 * expander_size;

      /* Create the new window */
      if (tree_view->priv->drag_column_window_state != DRAG_COLUMN_WINDOW_STATE_ARROW_LEFT &&
	  tree_view->priv->drag_column_window_state != DRAG_COLUMN_WINDOW_STATE_ARROW_RIGHT)
	{
	  if (tree_view->priv->drag_highlight_window)
	    {
	      ctk_widget_unregister_window (CTK_WIDGET (tree_view), tree_view->priv->drag_highlight_window);
	      cdk_window_destroy (tree_view->priv->drag_highlight_window);
	    }

	  attributes.window_type = CDK_WINDOW_TEMP;
	  attributes.wclass = CDK_INPUT_OUTPUT;
	  attributes.visual = ctk_widget_get_visual (CTK_WIDGET (tree_view));
	  attributes.event_mask = CDK_VISIBILITY_NOTIFY_MASK | CDK_POINTER_MOTION_MASK;
	  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;
          attributes.x = x;
          attributes.y = y;
	  attributes.width = width;
	  attributes.height = height;
	  tree_view->priv->drag_highlight_window = cdk_window_new (cdk_screen_get_root_window (ctk_widget_get_screen (widget)), &attributes, attributes_mask);
	  ctk_widget_register_window (CTK_WIDGET (tree_view), tree_view->priv->drag_highlight_window);

	  mask_image = cairo_image_surface_create (CAIRO_FORMAT_A1, width, height);

          cr = cairo_create (mask_image);
          /* mirror if we're on the left */
          if (arrow_type == DRAG_COLUMN_WINDOW_STATE_ARROW_LEFT)
            {
              cairo_translate (cr, width, 0);
              cairo_scale (cr, -1, 1);
            }
          cairo_move_to (cr, 0, 0);
          cairo_line_to (cr, width, width);
          cairo_line_to (cr, 0, expander_size);
          cairo_move_to (cr, 0, height);
          cairo_line_to (cr, width, height - width);
          cairo_line_to (cr, 0, height - expander_size);
          cairo_fill (cr);
          cairo_destroy (cr);

          mask_region = cdk_cairo_region_create_from_surface (mask_image);
	  cdk_window_shape_combine_region (tree_view->priv->drag_highlight_window,
					   mask_region, 0, 0);

          cairo_region_destroy (mask_region);
          cairo_surface_destroy (mask_image);
	}

      tree_view->priv->drag_column_window_state = arrow_type;
      cdk_window_move (tree_view->priv->drag_highlight_window, x, y);
   }
  else
    {
      g_warning (G_STRLOC"Invalid CtkTreeViewColumnReorder struct");
      cdk_window_hide (tree_view->priv->drag_highlight_window);
      return;
    }

  cdk_window_show (tree_view->priv->drag_highlight_window);
  cdk_window_raise (tree_view->priv->drag_highlight_window);
}

static gboolean
ctk_tree_view_motion_resize_column (CtkTreeView *tree_view,
                                    gdouble      x,
                                    gdouble      y G_GNUC_UNUSED)
{
  gint new_width;
  CtkTreeViewColumn *column;

  column = ctk_tree_view_get_column (tree_view, tree_view->priv->drag_pos);

  if (ctk_widget_get_direction (CTK_WIDGET (tree_view)) == CTK_TEXT_DIR_RTL)
    new_width = MAX (tree_view->priv->x_drag - x, 0);
  else
    new_width = MAX (x - tree_view->priv->x_drag, 0);

  if (new_width != ctk_tree_view_column_get_fixed_width (column))
    ctk_tree_view_column_set_fixed_width (column, new_width);

  return FALSE;
}

static void
ctk_tree_view_update_current_reorder (CtkTreeView *tree_view)
{
  CtkTreeViewColumnReorder *reorder = NULL;
  CdkEventSequence *sequence;
  GList *list;
  gdouble x;

  sequence = ctk_gesture_single_get_current_sequence
    (CTK_GESTURE_SINGLE (tree_view->priv->column_drag_gesture));
  ctk_gesture_get_point (tree_view->priv->column_drag_gesture,
                         sequence, &x, NULL);
  x += ctk_adjustment_get_value (tree_view->priv->hadjustment);

  for (list = tree_view->priv->column_drag_info; list; list = list->next)
    {
      reorder = (CtkTreeViewColumnReorder *) list->data;
      if (x >= reorder->left_align && x < reorder->right_align)
	break;
      reorder = NULL;
    }

  tree_view->priv->cur_reorder = reorder;
  ctk_tree_view_motion_draw_column_motion_arrow (tree_view);
}

static void
ctk_tree_view_vertical_autoscroll (CtkTreeView *tree_view)
{
  CdkRectangle visible_rect;
  gint y;
  gint offset;

  if (ctk_gesture_is_recognized (tree_view->priv->drag_gesture))
    {
      CdkEventSequence *sequence;
      gdouble py;

      sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (tree_view->priv->drag_gesture));
      ctk_gesture_get_point (tree_view->priv->drag_gesture, sequence, NULL, &py);
      ctk_tree_view_convert_widget_to_bin_window_coords (tree_view, 0, py,
                                                         NULL, &y);
    }
  else
    {
      y = tree_view->priv->event_last_y;
      ctk_tree_view_convert_widget_to_bin_window_coords (tree_view, 0, y, NULL, &y);
    }

  y += tree_view->priv->dy;
  ctk_tree_view_get_visible_rect (tree_view, &visible_rect);

  /* see if we are near the edge. */
  offset = y - (visible_rect.y + 2 * SCROLL_EDGE_SIZE);
  if (offset > 0)
    {
      offset = y - (visible_rect.y + visible_rect.height - 2 * SCROLL_EDGE_SIZE);
      if (offset < 0)
	return;
    }

  ctk_adjustment_set_value (tree_view->priv->vadjustment,
                            MAX (ctk_adjustment_get_value (tree_view->priv->vadjustment) + offset, 0.0));
}

static gboolean
ctk_tree_view_horizontal_autoscroll (CtkTreeView *tree_view)
{
  CdkEventSequence *sequence;
  CdkRectangle visible_rect;
  gdouble x;
  gint offset;

  sequence = ctk_gesture_single_get_current_sequence
    (CTK_GESTURE_SINGLE (tree_view->priv->column_drag_gesture));
  ctk_gesture_get_point (tree_view->priv->column_drag_gesture,
                         sequence, &x, NULL);
  ctk_tree_view_get_visible_rect (tree_view, &visible_rect);

  x += ctk_adjustment_get_value (tree_view->priv->hadjustment);

  /* See if we are near the edge. */
  offset = x - (visible_rect.x + SCROLL_EDGE_SIZE);
  if (offset > 0)
    {
      offset = x - (visible_rect.x + visible_rect.width - SCROLL_EDGE_SIZE);
      if (offset < 0)
	return TRUE;
    }
  offset = offset/3;

  ctk_adjustment_set_value (tree_view->priv->hadjustment,
                            MAX (ctk_adjustment_get_value (tree_view->priv->hadjustment) + offset, 0.0));

  return TRUE;

}

static gboolean
ctk_tree_view_motion_drag_column (CtkTreeView *tree_view,
                                  gdouble      x,
                                  gdouble      y G_GNUC_UNUSED)
{
  CtkAllocation allocation, button_allocation;
  CtkTreeViewColumn *column = tree_view->priv->drag_column;
  CtkWidget *button;
  gint win_x, win_y;

  button = ctk_tree_view_column_get_button (column);
  x += ctk_adjustment_get_value (tree_view->priv->hadjustment);

  /* Handle moving the header */
  cdk_window_get_position (tree_view->priv->drag_window, &win_x, &win_y);
  ctk_widget_get_allocation (CTK_WIDGET (tree_view), &allocation);
  ctk_widget_get_allocation (button, &button_allocation);
  win_x = CLAMP (x - _ctk_tree_view_column_get_drag_x (column), 0,
                 MAX (tree_view->priv->width, allocation.width) - button_allocation.width);
  cdk_window_move (tree_view->priv->drag_window, win_x, win_y);
  cdk_window_raise (tree_view->priv->drag_window);

  /* autoscroll, if needed */
  ctk_tree_view_horizontal_autoscroll (tree_view);
  /* Update the current reorder position and arrow; */
  ctk_tree_view_update_current_reorder (tree_view);

  return TRUE;
}

static void
ctk_tree_view_stop_rubber_band (CtkTreeView *tree_view)
{
  remove_scroll_timeout (tree_view);

  if (tree_view->priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    {
      CtkTreePath *tmp_path;

      ctk_widget_queue_draw (CTK_WIDGET (tree_view));

      /* The anchor path should be set to the start path */
      if (tree_view->priv->rubber_band_start_node)
        {
          tmp_path = _ctk_tree_path_new_from_rbtree (tree_view->priv->rubber_band_start_tree,
                                                     tree_view->priv->rubber_band_start_node);

          if (tree_view->priv->anchor)
            ctk_tree_row_reference_free (tree_view->priv->anchor);

          tree_view->priv->anchor =
            ctk_tree_row_reference_new_proxy (G_OBJECT (tree_view),
                                              tree_view->priv->model,
                                              tmp_path);

          ctk_tree_path_free (tmp_path);
        }

      /* ... and the cursor to the end path */
      if (tree_view->priv->rubber_band_end_node)
        {
          tmp_path = _ctk_tree_path_new_from_rbtree (tree_view->priv->rubber_band_end_tree,
                                                     tree_view->priv->rubber_band_end_node);
          ctk_tree_view_real_set_cursor (CTK_TREE_VIEW (tree_view), tmp_path, 0);
          ctk_tree_path_free (tmp_path);
        }

      _ctk_tree_selection_emit_changed (tree_view->priv->selection);

      ctk_css_node_set_parent (tree_view->priv->rubber_band_cssnode, NULL);
      tree_view->priv->rubber_band_cssnode = NULL;
    }

  /* Clear status variables */
  tree_view->priv->rubber_band_status = RUBBER_BAND_OFF;
  tree_view->priv->rubber_band_extend = FALSE;
  tree_view->priv->rubber_band_modify = FALSE;

  tree_view->priv->rubber_band_start_node = NULL;
  tree_view->priv->rubber_band_start_tree = NULL;
  tree_view->priv->rubber_band_end_node = NULL;
  tree_view->priv->rubber_band_end_tree = NULL;
}

static void
ctk_tree_view_update_rubber_band_selection_range (CtkTreeView *tree_view,
						 CtkRBTree   *start_tree,
						 CtkRBNode   *start_node,
						 CtkRBTree   *end_tree G_GNUC_UNUSED,
						 CtkRBNode   *end_node,
						 gboolean     select,
						 gboolean     skip_start,
						 gboolean     skip_end)
{
  if (start_node == end_node)
    return;

  /* We skip the first node and jump inside the loop */
  if (skip_start)
    goto skip_first;

  do
    {
      /* Small optimization by assuming insensitive nodes are never
       * selected.
       */
      if (!CTK_RBNODE_FLAG_SET (start_node, CTK_RBNODE_IS_SELECTED))
        {
	  CtkTreePath *path;
	  gboolean selectable;

	  path = _ctk_tree_path_new_from_rbtree (start_tree, start_node);
	  selectable = _ctk_tree_selection_row_is_selectable (tree_view->priv->selection, start_node, path);
	  ctk_tree_path_free (path);

	  if (!selectable)
	    goto node_not_selectable;
	}

      if (select)
        {
	  if (tree_view->priv->rubber_band_extend)
            CTK_RBNODE_SET_FLAG (start_node, CTK_RBNODE_IS_SELECTED);
	  else if (tree_view->priv->rubber_band_modify)
	    {
	      /* Toggle the selection state */
	      if (CTK_RBNODE_FLAG_SET (start_node, CTK_RBNODE_IS_SELECTED))
		CTK_RBNODE_UNSET_FLAG (start_node, CTK_RBNODE_IS_SELECTED);
	      else
		CTK_RBNODE_SET_FLAG (start_node, CTK_RBNODE_IS_SELECTED);
	    }
	  else
	    CTK_RBNODE_SET_FLAG (start_node, CTK_RBNODE_IS_SELECTED);
	}
      else
        {
	  /* Mirror the above */
	  if (tree_view->priv->rubber_band_extend)
	    CTK_RBNODE_UNSET_FLAG (start_node, CTK_RBNODE_IS_SELECTED);
	  else if (tree_view->priv->rubber_band_modify)
	    {
	      /* Toggle the selection state */
	      if (CTK_RBNODE_FLAG_SET (start_node, CTK_RBNODE_IS_SELECTED))
		CTK_RBNODE_UNSET_FLAG (start_node, CTK_RBNODE_IS_SELECTED);
	      else
		CTK_RBNODE_SET_FLAG (start_node, CTK_RBNODE_IS_SELECTED);
	    }
	  else
	    CTK_RBNODE_UNSET_FLAG (start_node, CTK_RBNODE_IS_SELECTED);
	}

      _ctk_tree_view_queue_draw_node (tree_view, start_tree, start_node, NULL);

node_not_selectable:
      if (start_node == end_node)
	break;

skip_first:

      if (start_node->children)
        {
	  start_tree = start_node->children;
          start_node = _ctk_rbtree_first (start_tree);
	}
      else
        {
	  _ctk_rbtree_next_full (start_tree, start_node, &start_tree, &start_node);

	  if (!start_tree)
	    /* Ran out of tree */
	    break;
	}

      if (skip_end && start_node == end_node)
	break;
    }
  while (TRUE);
}

static void
ctk_tree_view_update_rubber_band_selection (CtkTreeView *tree_view)
{
  CtkRBTree *start_tree, *end_tree;
  CtkRBNode *start_node, *end_node;
  gdouble start_y, offset_y;
  gint bin_y;

  if (!ctk_gesture_is_active (tree_view->priv->drag_gesture))
    return;

  ctk_gesture_drag_get_offset (CTK_GESTURE_DRAG (tree_view->priv->drag_gesture),
                               NULL, &offset_y);
  ctk_gesture_drag_get_start_point (CTK_GESTURE_DRAG (tree_view->priv->drag_gesture),
                                    NULL, &start_y);
  ctk_tree_view_convert_widget_to_bin_window_coords (tree_view, 0, start_y,
                                                     NULL, &bin_y);
  bin_y = MAX (0, bin_y + offset_y + tree_view->priv->dy);

  _ctk_rbtree_find_offset (tree_view->priv->tree, MIN (tree_view->priv->press_start_y, bin_y), &start_tree, &start_node);
  _ctk_rbtree_find_offset (tree_view->priv->tree, MAX (tree_view->priv->press_start_y, bin_y), &end_tree, &end_node);

  /* Handle the start area first */
  if (!start_node && !end_node)
    {
      if (tree_view->priv->rubber_band_start_node)
        {
          CtkRBNode *node = tree_view->priv->rubber_band_start_node;
          CtkRBTree *tree = tree_view->priv->rubber_band_start_tree;

	  if (tree_view->priv->rubber_band_modify)
	    {
	      /* Toggle the selection state */
	      if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
		CTK_RBNODE_UNSET_FLAG (node, CTK_RBNODE_IS_SELECTED);
	      else
		CTK_RBNODE_SET_FLAG (node, CTK_RBNODE_IS_SELECTED);
	    }
          else
            CTK_RBNODE_UNSET_FLAG (node, CTK_RBNODE_IS_SELECTED);

          _ctk_tree_view_queue_draw_node (tree_view, tree, node, NULL);
        }
    }
  if (!tree_view->priv->rubber_band_start_node || !start_node)
    {
      ctk_tree_view_update_rubber_band_selection_range (tree_view,
						       start_tree,
						       start_node,
						       end_tree,
						       end_node,
						       TRUE,
						       FALSE,
						       FALSE);
    }
  else if (_ctk_rbtree_node_find_offset (start_tree, start_node) <
           _ctk_rbtree_node_find_offset (tree_view->priv->rubber_band_start_tree, tree_view->priv->rubber_band_start_node))
    {
      /* New node is above the old one; selection became bigger */
      ctk_tree_view_update_rubber_band_selection_range (tree_view,
						       start_tree,
						       start_node,
						       tree_view->priv->rubber_band_start_tree,
						       tree_view->priv->rubber_band_start_node,
						       TRUE,
						       FALSE,
						       TRUE);
    }
  else if (_ctk_rbtree_node_find_offset (start_tree, start_node) >
           _ctk_rbtree_node_find_offset (tree_view->priv->rubber_band_start_tree, tree_view->priv->rubber_band_start_node))
    {
      /* New node is below the old one; selection became smaller */
      ctk_tree_view_update_rubber_band_selection_range (tree_view,
						       tree_view->priv->rubber_band_start_tree,
						       tree_view->priv->rubber_band_start_node,
						       start_tree,
						       start_node,
						       FALSE,
						       FALSE,
						       TRUE);
    }

  tree_view->priv->rubber_band_start_tree = start_tree;
  tree_view->priv->rubber_band_start_node = start_node;

  /* Next, handle the end area */
  if (!tree_view->priv->rubber_band_end_node)
    {
      /* In the event this happens, start_node was also NULL; this case is
       * handled above.
       */
    }
  else if (!end_node)
    {
      /* Find the last node in the tree */
      _ctk_rbtree_find_offset (tree_view->priv->tree, ctk_tree_view_get_height (tree_view) - 1,
			       &end_tree, &end_node);

      /* Selection reached end of the tree */
      ctk_tree_view_update_rubber_band_selection_range (tree_view,
						       tree_view->priv->rubber_band_end_tree,
						       tree_view->priv->rubber_band_end_node,
						       end_tree,
						       end_node,
						       TRUE,
						       TRUE,
						       FALSE);
    }
  else if (_ctk_rbtree_node_find_offset (end_tree, end_node) >
           _ctk_rbtree_node_find_offset (tree_view->priv->rubber_band_end_tree, tree_view->priv->rubber_band_end_node))
    {
      /* New node is below the old one; selection became bigger */
      ctk_tree_view_update_rubber_band_selection_range (tree_view,
						       tree_view->priv->rubber_band_end_tree,
						       tree_view->priv->rubber_band_end_node,
						       end_tree,
						       end_node,
						       TRUE,
						       TRUE,
						       FALSE);
    }
  else if (_ctk_rbtree_node_find_offset (end_tree, end_node) <
           _ctk_rbtree_node_find_offset (tree_view->priv->rubber_band_end_tree, tree_view->priv->rubber_band_end_node))
    {
      /* New node is above the old one; selection became smaller */
      ctk_tree_view_update_rubber_band_selection_range (tree_view,
						       end_tree,
						       end_node,
						       tree_view->priv->rubber_band_end_tree,
						       tree_view->priv->rubber_band_end_node,
						       FALSE,
						       TRUE,
						       FALSE);
    }

  tree_view->priv->rubber_band_end_tree = end_tree;
  tree_view->priv->rubber_band_end_node = end_node;
}

static void
ctk_tree_view_update_rubber_band (CtkTreeView *tree_view)
{
  gdouble start_x, start_y, offset_x, offset_y, x, y;
  CdkRectangle old_area;
  CdkRectangle new_area;
  cairo_region_t *invalid_region;
  gint bin_x, bin_y;

  if (!ctk_gesture_is_recognized (tree_view->priv->drag_gesture))
    return;

  old_area.x = MIN (tree_view->priv->press_start_x, tree_view->priv->rubber_band_x);
  old_area.y = MIN (tree_view->priv->press_start_y, tree_view->priv->rubber_band_y) - tree_view->priv->dy;
  old_area.width = ABS (tree_view->priv->rubber_band_x - tree_view->priv->press_start_x) + 1;
  old_area.height = ABS (tree_view->priv->rubber_band_y - tree_view->priv->press_start_y) + 1;

  ctk_gesture_drag_get_offset (CTK_GESTURE_DRAG (tree_view->priv->drag_gesture),
                               &offset_x, &offset_y);
  ctk_gesture_drag_get_start_point (CTK_GESTURE_DRAG (tree_view->priv->drag_gesture),
                                    &start_x, &start_y);
  ctk_tree_view_convert_widget_to_bin_window_coords (tree_view, start_x, start_y,
                                                     &bin_x, &bin_y);
  bin_y += tree_view->priv->dy;

  x = MAX (bin_x + offset_x, 0);
  y = MAX (bin_y + offset_y, 0);

  new_area.x = MIN (tree_view->priv->press_start_x, x);
  new_area.y = MIN (tree_view->priv->press_start_y, y) - tree_view->priv->dy;
  new_area.width = ABS (x - tree_view->priv->press_start_x) + 1;
  new_area.height = ABS (y - tree_view->priv->press_start_y) + 1;

  invalid_region = cairo_region_create_rectangle (&old_area);
  cairo_region_union_rectangle (invalid_region, &new_area);

  cdk_window_invalidate_region (tree_view->priv->bin_window, invalid_region, TRUE);

  cairo_region_destroy (invalid_region);

  tree_view->priv->rubber_band_x = x;
  tree_view->priv->rubber_band_y = y;

  ctk_tree_view_update_rubber_band_selection (tree_view);
}

static void
ctk_tree_view_paint_rubber_band (CtkTreeView  *tree_view,
                                 cairo_t      *cr)
{
  gdouble start_x, start_y, offset_x, offset_y;
  CdkRectangle rect;
  CtkStyleContext *context;
  gint bin_x, bin_y;

  if (!ctk_gesture_is_recognized (tree_view->priv->drag_gesture))
    return;

  ctk_gesture_drag_get_offset (CTK_GESTURE_DRAG (tree_view->priv->drag_gesture),
                               &offset_x, &offset_y);
  ctk_gesture_drag_get_start_point (CTK_GESTURE_DRAG (tree_view->priv->drag_gesture),
                                    &start_x, &start_y);
  ctk_tree_view_convert_widget_to_bin_window_coords (tree_view, start_x, start_y,
                                                     &bin_x, &bin_y);
  bin_x = MAX (0, bin_x + offset_x);
  bin_y = MAX (0, bin_y + offset_y + tree_view->priv->dy);

  cairo_save (cr);

  context = ctk_widget_get_style_context (CTK_WIDGET (tree_view));

  ctk_style_context_save_to_node (context, tree_view->priv->rubber_band_cssnode);

  rect.x = MIN (tree_view->priv->press_start_x, bin_x);
  rect.y = MIN (tree_view->priv->press_start_y, bin_y) - tree_view->priv->dy;
  rect.width = ABS (tree_view->priv->press_start_x - bin_x) + 1;
  rect.height = ABS (tree_view->priv->press_start_y - bin_y) + 1;

  cdk_cairo_rectangle (cr, &rect);
  cairo_clip (cr);

  ctk_render_background (context, cr,
                         rect.x, rect.y,
                         rect.width, rect.height);
  ctk_render_frame (context, cr,
                    rect.x, rect.y,
                    rect.width, rect.height);

  ctk_style_context_restore (context);
  cairo_restore (cr);
}

static void
ctk_tree_view_column_drag_gesture_update (CtkGestureDrag *gesture,
                                          gdouble         offset_x,
                                          gdouble         offset_y,
                                          CtkTreeView    *tree_view)
{
  gdouble start_x, start_y, x, y;
  CdkEventSequence *sequence;

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));

  if (ctk_gesture_get_sequence_state (CTK_GESTURE (gesture), sequence) != CTK_EVENT_SEQUENCE_CLAIMED)
    return;

  ctk_gesture_drag_get_start_point (gesture, &start_x, &start_y);
  x = start_x + offset_x;
  y = start_y + offset_y;

  if (tree_view->priv->in_column_resize)
    ctk_tree_view_motion_resize_column (tree_view, x, y);
  else if (tree_view->priv->in_column_drag)
    ctk_tree_view_motion_drag_column (tree_view, x, y);
}

static void
ctk_tree_view_drag_gesture_update (CtkGestureDrag *gesture,
                                   gdouble         offset_x G_GNUC_UNUSED,
                                   gdouble         offset_y G_GNUC_UNUSED,
                                   CtkTreeView    *tree_view)
{
  if (tree_view->priv->tree == NULL)
    {
      ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_DENIED);
      return;
    }

  if (tree_view->priv->rubber_band_status == RUBBER_BAND_MAYBE_START)
    {
      CtkCssNode *widget_node;

      widget_node = ctk_widget_get_css_node (CTK_WIDGET (tree_view));
      tree_view->priv->rubber_band_cssnode = ctk_css_node_new ();
      ctk_css_node_set_name (tree_view->priv->rubber_band_cssnode, I_("rubberband"));
      ctk_css_node_set_parent (tree_view->priv->rubber_band_cssnode, widget_node);
      ctk_css_node_set_state (tree_view->priv->rubber_band_cssnode, ctk_css_node_get_state (widget_node));
      g_object_unref (tree_view->priv->rubber_band_cssnode);

      ctk_tree_view_update_rubber_band (tree_view);

      tree_view->priv->rubber_band_status = RUBBER_BAND_ACTIVE;
      ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_CLAIMED);
    }
  else if (tree_view->priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    {
      ctk_tree_view_update_rubber_band (tree_view);

      add_scroll_timeout (tree_view);
    }
  else if (!tree_view->priv->rubber_band_status)
    {
      if (ctk_tree_view_maybe_begin_dragging_row (tree_view))
        ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_DENIED);
    }
}

static gboolean
ctk_tree_view_motion (CtkWidget      *widget,
		      CdkEventMotion *event)
{
  CtkTreeView *tree_view;
  CtkRBTree *tree;
  CtkRBNode *node;
  gint new_y;

  tree_view = (CtkTreeView *) widget;

  if (tree_view->priv->tree)
    {
      /* If we are currently pressing down a button, we don't want to prelight anything else. */
      if (ctk_gesture_is_active (tree_view->priv->drag_gesture) ||
          ctk_gesture_is_active (tree_view->priv->multipress_gesture))
        node = NULL;

      new_y = MAX (0, TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, event->y));

      _ctk_rbtree_find_offset (tree_view->priv->tree, new_y, &tree, &node);

      tree_view->priv->event_last_x = event->x;
      tree_view->priv->event_last_y = event->y;
      prelight_or_select (tree_view, tree, node, event->x, event->y);
    }

  return CTK_WIDGET_CLASS (ctk_tree_view_parent_class)->motion_notify_event (widget, event);
}

/* Invalidate the focus rectangle near the edge of the bin_window; used when
 * the tree is empty.
 */
static void
invalidate_empty_focus (CtkTreeView *tree_view)
{
  CdkRectangle area;

  if (!ctk_widget_has_focus (CTK_WIDGET (tree_view)))
    return;

  area.x = 0;
  area.y = 0;
  area.width = cdk_window_get_width (tree_view->priv->bin_window);
  area.height = cdk_window_get_height (tree_view->priv->bin_window);
  cdk_window_invalidate_rect (tree_view->priv->bin_window, &area, FALSE);
}

/* Draws background and a focus rectangle near the edge of the bin_window;
 * used when the tree is empty.
 */
static void
draw_empty (CtkTreeView *tree_view,
            cairo_t     *cr)
{
  CtkWidget *widget = CTK_WIDGET (tree_view);
  CtkStyleContext *context;
  gint width, height;

  context = ctk_widget_get_style_context (widget);

  width = cdk_window_get_width (tree_view->priv->bin_window);
  height = cdk_window_get_height (tree_view->priv->bin_window);

  ctk_render_background (context, cr, 0, 0, width, height);

  if (ctk_widget_has_visible_focus (widget))
    ctk_render_focus (context, cr, 0, 0, width, height);
}

typedef enum {
  CTK_TREE_VIEW_GRID_LINE,
  CTK_TREE_VIEW_TREE_LINE,
  CTK_TREE_VIEW_FOREGROUND_LINE
} CtkTreeViewLineType;

static void
ctk_tree_view_draw_line (CtkTreeView         *tree_view,
                         cairo_t             *cr,
                         CtkTreeViewLineType  type,
                         int                  x1,
                         int                  y1,
                         int                  x2,
                         int                  y2)
{
  CtkStyleContext *context;

  cairo_save (cr);

  context = ctk_widget_get_style_context (CTK_WIDGET (tree_view));

  switch (type)
    {
    case CTK_TREE_VIEW_TREE_LINE:
      {
        const CdkRGBA *color;

        color = _ctk_css_rgba_value_get_rgba (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_BORDER_LEFT_COLOR));

        cdk_cairo_set_source_rgba (cr, color);
        cairo_set_line_width (cr, tree_view->priv->tree_line_width);
        if (tree_view->priv->tree_line_dashes[0])
          cairo_set_dash (cr, tree_view->priv->tree_line_dashes, 2, 0.5);
      }
      break;

    case CTK_TREE_VIEW_GRID_LINE:
      {
        const CdkRGBA *color;

        color = _ctk_css_rgba_value_get_rgba (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_BORDER_TOP_COLOR));

        cdk_cairo_set_source_rgba (cr, color);
        cairo_set_line_width (cr, tree_view->priv->grid_line_width);
        if (tree_view->priv->grid_line_dashes[0])
          cairo_set_dash (cr, tree_view->priv->grid_line_dashes, 2, 0.5);
      }
      break;

    case CTK_TREE_VIEW_FOREGROUND_LINE:
      {
        CdkRGBA color;

        cairo_set_line_width (cr, 1.0);
        ctk_style_context_get_color (context, ctk_style_context_get_state (context), &color);
        cdk_cairo_set_source_rgba (cr, &color);
      }
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  cairo_move_to (cr, x1 + 0.5, y1 + 0.5);
  cairo_line_to (cr, x2 + 0.5, y2 + 0.5);
  cairo_stroke (cr);

  cairo_restore (cr);
}
                         
static void
ctk_tree_view_draw_grid_lines (CtkTreeView    *tree_view,
			       cairo_t        *cr)
{
  GList *list, *first, *last;
  gboolean rtl;
  gint current_x = 0;

  if (tree_view->priv->grid_lines != CTK_TREE_VIEW_GRID_LINES_VERTICAL
      && tree_view->priv->grid_lines != CTK_TREE_VIEW_GRID_LINES_BOTH)
    return;

  rtl = (ctk_widget_get_direction (CTK_WIDGET (tree_view)) == CTK_TEXT_DIR_RTL);

  first = g_list_first (tree_view->priv->columns);
  last = g_list_last (tree_view->priv->columns);

  for (list = (rtl ? last : first);
       list;
       list = (rtl ? list->prev : list->next))
    {
      CtkTreeViewColumn *column = list->data;

      /* We don't want a line for the last column */
      if (column == (rtl ? first->data : last->data))
        break;

      if (!ctk_tree_view_column_get_visible (column))
        continue;

      current_x += ctk_tree_view_column_get_width (column);

      ctk_tree_view_draw_line (tree_view, cr,
                               CTK_TREE_VIEW_GRID_LINE,
                               current_x - 1, 0,
                               current_x - 1, ctk_tree_view_get_height (tree_view));
    }
}

/* Warning: Very scary function.
 * Modify at your own risk
 *
 * KEEP IN SYNC WITH ctk_tree_view_create_row_drag_icon()!
 * FIXME: It’s not...
 */
static gboolean
ctk_tree_view_bin_draw (CtkWidget      *widget,
			cairo_t        *cr)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  CtkTreePath *path;
  CtkRBTree *tree;
  GList *list;
  CtkRBNode *node;
  CtkRBNode *drag_highlight = NULL;
  CtkRBTree *drag_highlight_tree = NULL;
  CtkTreeIter iter;
  gint new_y;
  gint y_offset, cell_offset;
  gint max_height;
  gint depth;
  CdkRectangle background_area;
  CdkRectangle cell_area;
  CdkRectangle clip;
  guint flags;
  gint bin_window_width;
  gint bin_window_height;
  CtkTreePath *drag_dest_path = NULL;
  GList *first_column, *last_column;
  gint vertical_separator;
  gint horizontal_separator;
  gboolean allow_rules;
  gboolean has_can_focus_cell;
  gboolean rtl;
  gint n_visible_columns;
  gint grid_line_width;
  gint expander_size;
  gboolean draw_vgrid_lines, draw_hgrid_lines;
  CtkStyleContext *context;
  gboolean parity;

  rtl = (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL);
  context = ctk_widget_get_style_context (widget);

  ctk_widget_style_get (widget,
			"horizontal-separator", &horizontal_separator,
			"vertical-separator", &vertical_separator,
			"allow-rules", &allow_rules,
			NULL);

  if (tree_view->priv->tree == NULL)
    {
      draw_empty (tree_view, cr);
      return TRUE;
    }

  bin_window_width = cdk_window_get_width (tree_view->priv->bin_window);
  bin_window_height = cdk_window_get_height (tree_view->priv->bin_window);
  if (!cdk_cairo_get_clip_rectangle (cr, &clip))
    return TRUE;

  new_y = TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, clip.y);

  if (new_y < 0)
    new_y = 0;
  y_offset = -_ctk_rbtree_find_offset (tree_view->priv->tree, new_y, &tree, &node);

  if (ctk_tree_view_get_height (tree_view) < bin_window_height)
    {
      ctk_style_context_save (context);
      ctk_style_context_add_class (context, CTK_STYLE_CLASS_CELL);

      ctk_render_background (context, cr,
                             0, ctk_tree_view_get_height (tree_view),
                             bin_window_width,
                             bin_window_height - ctk_tree_view_get_height (tree_view));

      ctk_style_context_restore (context);
    }

  if (node == NULL)
    goto done;

  /* find the path for the node */
  path = _ctk_tree_path_new_from_rbtree (tree, node);
  ctk_tree_model_get_iter (tree_view->priv->model,
			   &iter,
			   path);
  depth = ctk_tree_path_get_depth (path);
  ctk_tree_path_free (path);

  if (tree_view->priv->drag_dest_row)
    drag_dest_path = ctk_tree_row_reference_get_path (tree_view->priv->drag_dest_row);

  if (drag_dest_path)
    _ctk_tree_view_find_node (tree_view, drag_dest_path,
                              &drag_highlight_tree, &drag_highlight);

  draw_vgrid_lines =
    tree_view->priv->grid_lines == CTK_TREE_VIEW_GRID_LINES_VERTICAL
    || tree_view->priv->grid_lines == CTK_TREE_VIEW_GRID_LINES_BOTH;
  draw_hgrid_lines =
    tree_view->priv->grid_lines == CTK_TREE_VIEW_GRID_LINES_HORIZONTAL
    || tree_view->priv->grid_lines == CTK_TREE_VIEW_GRID_LINES_BOTH;
  expander_size = ctk_tree_view_get_expander_size (tree_view);

  if (draw_vgrid_lines || draw_hgrid_lines)
    ctk_widget_style_get (widget, "grid-line-width", &grid_line_width, NULL);
  
  n_visible_columns = 0;
  for (list = tree_view->priv->columns; list; list = list->next)
    {
      if (!ctk_tree_view_column_get_visible (CTK_TREE_VIEW_COLUMN (list->data)))
	continue;
      n_visible_columns ++;
    }

  /* Find the last column */
  for (last_column = g_list_last (tree_view->priv->columns);
       last_column &&
       !(ctk_tree_view_column_get_visible (CTK_TREE_VIEW_COLUMN (last_column->data)));
       last_column = last_column->prev)
    ;

  /* and the first */
  for (first_column = g_list_first (tree_view->priv->columns);
       first_column &&
       !(ctk_tree_view_column_get_visible (CTK_TREE_VIEW_COLUMN (first_column->data)));
       first_column = first_column->next)
    ;

  /* Actually process the expose event.  To do this, we want to
   * start at the first node of the event, and walk the tree in
   * order, drawing each successive node.
   */
  
  parity = !(_ctk_rbtree_node_get_index (tree, node) % 2);

  do
    {
      gboolean is_separator = FALSE;
      gint n_col = 0;

      parity = !parity;
      is_separator = row_is_separator (tree_view, &iter, NULL);

      max_height = ctk_tree_view_get_row_height (tree_view, node);

      cell_offset = 0;

      background_area.y = y_offset + clip.y;
      background_area.height = max_height;

      flags = 0;

      if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_PRELIT))
	flags |= CTK_CELL_RENDERER_PRELIT;

      if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
        flags |= CTK_CELL_RENDERER_SELECTED;

      /* we *need* to set cell data on all cells before the call
       * to _has_can_focus_cell, else _has_can_focus_cell() does not
       * return a correct value.
       */
      for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
	   list;
	   list = (rtl ? list->prev : list->next))
        {
	  CtkTreeViewColumn *column = list->data;
	  ctk_tree_view_column_cell_set_cell_data (column,
						   tree_view->priv->model,
						   &iter,
						   CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_PARENT),
						   node->children?TRUE:FALSE);
        }

      has_can_focus_cell = ctk_tree_view_has_can_focus_cell (tree_view);

      for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
	   list;
	   list = (rtl ? list->prev : list->next))
	{
	  CtkTreeViewColumn *column = list->data;
	  CtkStateFlags state = 0;
          gint width;
          gboolean draw_focus;

	  if (!ctk_tree_view_column_get_visible (column))
            continue;

          n_col++;
          width = ctk_tree_view_column_get_width (column);

	  if (cell_offset > clip.x + clip.width ||
	      cell_offset + width < clip.x)
	    {
	      cell_offset += width;
	      continue;
	    }

          if (ctk_tree_view_column_get_sort_indicator (column))
	    flags |= CTK_CELL_RENDERER_SORTED;
          else
            flags &= ~CTK_CELL_RENDERER_SORTED;

	  if (tree_view->priv->cursor_node == node)
            flags |= CTK_CELL_RENDERER_FOCUSED;
          else
            flags &= ~CTK_CELL_RENDERER_FOCUSED;

          if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_PARENT))
            flags |= CTK_CELL_RENDERER_EXPANDABLE;
          else
            flags &= ~CTK_CELL_RENDERER_EXPANDABLE;

          if (node->children)
            flags |= CTK_CELL_RENDERER_EXPANDED;
          else
            flags &= ~CTK_CELL_RENDERER_EXPANDED;

	  background_area.x = cell_offset;
	  background_area.width = width;

          cell_area = background_area;
          cell_area.y += vertical_separator / 2;
          cell_area.x += horizontal_separator / 2;
          cell_area.height -= vertical_separator;
	  cell_area.width -= horizontal_separator;

	  if (draw_vgrid_lines)
	    {
	      if (list == first_column)
	        {
		  cell_area.width -= grid_line_width / 2;
		}
	      else if (list == last_column)
	        {
		  cell_area.x += grid_line_width / 2;
		  cell_area.width -= grid_line_width / 2;
		}
	      else
	        {
	          cell_area.x += grid_line_width / 2;
	          cell_area.width -= grid_line_width;
		}
	    }

	  if (draw_hgrid_lines)
	    {
	      cell_area.y += grid_line_width / 2;
	      cell_area.height -= grid_line_width;
	    }

	  if (!cdk_rectangle_intersect (&clip, &background_area, NULL))
	    {
	      cell_offset += ctk_tree_view_column_get_width (column);
	      continue;
	    }

	  ctk_tree_view_column_cell_set_cell_data (column,
						   tree_view->priv->model,
						   &iter,
						   CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_PARENT),
						   node->children?TRUE:FALSE);

          ctk_style_context_save (context);

          state = ctk_cell_renderer_get_state (NULL, widget, flags);
          ctk_style_context_set_state (context, state);

          ctk_style_context_add_class (context, CTK_STYLE_CLASS_CELL);

	  if (node == tree_view->priv->cursor_node && has_can_focus_cell
              && ((column == tree_view->priv->focus_column
                   && tree_view->priv->draw_keyfocus &&
                   ctk_widget_has_visible_focus (widget))
                  || (column == tree_view->priv->edited_column)))
            draw_focus = TRUE;
          else
            draw_focus = FALSE;

	  /* Draw background */
          ctk_render_background (context, cr,
                                 background_area.x,
                                 background_area.y,
                                 background_area.width,
                                 background_area.height);

          /* Draw frame */
          ctk_render_frame (context, cr,
                            background_area.x,
                            background_area.y,
                            background_area.width,
                            background_area.height);

	  if (ctk_tree_view_is_expander_column (tree_view, column))
	    {
	      if (!rtl)
		cell_area.x += (depth - 1) * tree_view->priv->level_indentation;
	      cell_area.width -= (depth - 1) * tree_view->priv->level_indentation;

              if (ctk_tree_view_draw_expanders (tree_view))
	        {
	          if (!rtl)
		    cell_area.x += depth * expander_size;
		  cell_area.width -= depth * expander_size;
		}

	      if (is_separator)
                {
                  ctk_style_context_save (context);
                  ctk_style_context_add_class (context, CTK_STYLE_CLASS_SEPARATOR);

                  ctk_render_line (context, cr,
                                   cell_area.x,
                                   cell_area.y + cell_area.height / 2,
                                   cell_area.x + cell_area.width,
                                   cell_area.y + cell_area.height / 2);

                  ctk_style_context_restore (context);
                }
	      else
                {
                  _ctk_tree_view_column_cell_render (column,
                                                     cr,
                                                     &background_area,
                                                     &cell_area,
                                                     flags,
                                                     draw_focus);
                }

	      if (ctk_tree_view_draw_expanders (tree_view)
		  && (node->flags & CTK_RBNODE_IS_PARENT) == CTK_RBNODE_IS_PARENT)
		{
		  ctk_tree_view_draw_arrow (CTK_TREE_VIEW (widget),
                                            cr,
					    tree,
					    node);
		}
	    }
	  else
	    {
	      if (is_separator)
                {
                  ctk_style_context_save (context);
                  ctk_style_context_add_class (context, CTK_STYLE_CLASS_SEPARATOR);

                  ctk_render_line (context, cr,
                                   cell_area.x,
                                   cell_area.y + cell_area.height / 2,
                                   cell_area.x + cell_area.width,
                                   cell_area.y + cell_area.height / 2);

                  ctk_style_context_restore (context);
                }
	      else
		_ctk_tree_view_column_cell_render (column,
						   cr,
						   &background_area,
						   &cell_area,
						   flags,
                                                   draw_focus);
	    }

	  if (draw_hgrid_lines)
	    {
	      if (background_area.y >= clip.y)
                ctk_tree_view_draw_line (tree_view, cr,
                                         CTK_TREE_VIEW_GRID_LINE,
                                         background_area.x, background_area.y,
                                         background_area.x + background_area.width,
			                 background_area.y);

	      if (background_area.y + max_height < clip.y + clip.height)
                ctk_tree_view_draw_line (tree_view, cr,
                                         CTK_TREE_VIEW_GRID_LINE,
                                         background_area.x, background_area.y + max_height,
                                         background_area.x + background_area.width,
			                 background_area.y + max_height);
	    }

	  if (ctk_tree_view_is_expander_column (tree_view, column) &&
	      tree_view->priv->tree_lines_enabled)
	    {
	      gint x = background_area.x;
	      gint mult = rtl ? -1 : 1;
	      gint y0 = background_area.y;
	      gint y1 = background_area.y + background_area.height/2;
	      gint y2 = background_area.y + background_area.height;

	      if (rtl)
		x += background_area.width - 1;

	      if ((node->flags & CTK_RBNODE_IS_PARENT) == CTK_RBNODE_IS_PARENT
		  && depth > 1)
	        {
                  ctk_tree_view_draw_line (tree_view, cr,
                                           CTK_TREE_VIEW_TREE_LINE,
                                           x + expander_size * (depth - 1.5) * mult,
                                           y1,
                                           x + expander_size * (depth - 1.1) * mult,
                                           y1);
	        }
	      else if (depth > 1)
	        {
                  ctk_tree_view_draw_line (tree_view, cr,
                                           CTK_TREE_VIEW_TREE_LINE,
                                           x + expander_size * (depth - 1.5) * mult,
                                           y1,
                                           x + expander_size * (depth - 0.5) * mult,
                                           y1);
		}

	      if (depth > 1)
	        {
		  gint i;
		  CtkRBNode *tmp_node;
		  CtkRBTree *tmp_tree;

	          if (!_ctk_rbtree_next (tree, node))
                    ctk_tree_view_draw_line (tree_view, cr,
                                             CTK_TREE_VIEW_TREE_LINE,
                                             x + expander_size * (depth - 1.5) * mult,
                                             y0,
                                             x + expander_size * (depth - 1.5) * mult,
                                             y1);
		  else
                    ctk_tree_view_draw_line (tree_view, cr,
                                             CTK_TREE_VIEW_TREE_LINE,
                                             x + expander_size * (depth - 1.5) * mult,
                                             y0,
                                             x + expander_size * (depth - 1.5) * mult,
                                             y2);

		  tmp_node = tree->parent_node;
		  tmp_tree = tree->parent_tree;

		  for (i = depth - 2; i > 0; i--)
		    {
	              if (_ctk_rbtree_next (tmp_tree, tmp_node))
                        ctk_tree_view_draw_line (tree_view, cr,
                                                 CTK_TREE_VIEW_TREE_LINE,
                                                 x + expander_size * (i - 0.5) * mult,
                                                 y0,
                                                 x + expander_size * (i - 0.5) * mult,
                                                 y2);

		      tmp_node = tmp_tree->parent_node;
		      tmp_tree = tmp_tree->parent_tree;
		    }
		}
	    }

          ctk_style_context_restore (context);
	  cell_offset += ctk_tree_view_column_get_width (column);
	}

      if (node == drag_highlight)
        {
          /* Draw indicator for the drop
           */
	  CtkRBTree *drag_tree = NULL;
	  CtkRBNode *drag_node = NULL;

          ctk_style_context_save (context);
          ctk_style_context_set_state (context, ctk_style_context_get_state (context) | CTK_STATE_FLAG_DROP_ACTIVE);

          switch (tree_view->priv->drag_dest_pos)
            {
            case CTK_TREE_VIEW_DROP_BEFORE:
              ctk_style_context_add_class (context, "before");
              break;

            case CTK_TREE_VIEW_DROP_AFTER:
              ctk_style_context_add_class (context, "after");
              break;

            case CTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
            case CTK_TREE_VIEW_DROP_INTO_OR_AFTER:
              ctk_style_context_add_class (context, "into");
              break;
            }

          _ctk_tree_view_find_node (tree_view, drag_dest_path, &drag_tree, &drag_node);
          if (drag_tree != NULL)
             ctk_render_frame (context, cr,
                               0, ctk_tree_view_get_row_y_offset (tree_view, drag_tree, drag_node),
                               cdk_window_get_width (tree_view->priv->bin_window),
                               ctk_tree_view_get_row_height (tree_view, drag_node));

          ctk_style_context_restore (context);
        }

      /* draw the big row-spanning focus rectangle, if needed */
      if (!has_can_focus_cell && node == tree_view->priv->cursor_node &&
          tree_view->priv->draw_keyfocus &&
	  ctk_widget_has_visible_focus (widget))
        {
	  gint tmp_y, tmp_height;
	  CtkStateFlags focus_rect_state = 0;

          ctk_style_context_save (context);

          focus_rect_state = ctk_cell_renderer_get_state (NULL, widget, flags);
          ctk_style_context_set_state (context, focus_rect_state);

	  if (draw_hgrid_lines)
	    {
	      tmp_y = ctk_tree_view_get_row_y_offset (tree_view, tree, node) + grid_line_width / 2;
              tmp_height = ctk_tree_view_get_row_height (tree_view, node) - grid_line_width;
	    }
	  else
	    {
	      tmp_y = ctk_tree_view_get_row_y_offset (tree_view, tree, node);
              tmp_height = ctk_tree_view_get_row_height (tree_view, node);
	    }

          ctk_render_focus (context, cr,
                            0, tmp_y,
                            cdk_window_get_width (tree_view->priv->bin_window),
                            tmp_height);

          ctk_style_context_restore (context);
        }

      y_offset += max_height;
      if (node->children)
	{
	  CtkTreeIter parent = iter;
	  gboolean has_child;

	  tree = node->children;
          node = _ctk_rbtree_first (tree);

	  has_child = ctk_tree_model_iter_children (tree_view->priv->model,
						    &iter,
						    &parent);
	  depth++;

	  /* Sanity Check! */
	  TREE_VIEW_INTERNAL_ASSERT (has_child, FALSE);
	}
      else
	{
	  gboolean done = FALSE;

	  do
	    {
	      node = _ctk_rbtree_next (tree, node);
	      if (node != NULL)
		{
		  gboolean has_next = ctk_tree_model_iter_next (tree_view->priv->model, &iter);
		  done = TRUE;

		  /* Sanity Check! */
		  TREE_VIEW_INTERNAL_ASSERT (has_next, FALSE);
		}
	      else
		{
		  CtkTreeIter parent_iter = iter;
		  gboolean has_parent;

		  node = tree->parent_node;
		  tree = tree->parent_tree;
		  if (tree == NULL)
		    /* we should go to done to free some memory */
		    goto done;
		  has_parent = ctk_tree_model_iter_parent (tree_view->priv->model,
							   &iter,
							   &parent_iter);
		  depth--;

		  /* Sanity check */
		  TREE_VIEW_INTERNAL_ASSERT (has_parent, FALSE);
		}
	    }
	  while (!done);
	}
    }
  while (y_offset < clip.height);

done:
  ctk_tree_view_draw_grid_lines (tree_view, cr);

  if (tree_view->priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    ctk_tree_view_paint_rubber_band (tree_view, cr);

  if (drag_dest_path)
    ctk_tree_path_free (drag_dest_path);

  return FALSE;
}

static void
draw_bin (cairo_t *cr,
	  gpointer user_data)
{
  CtkWidget *widget = CTK_WIDGET (user_data);
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  GList *tmp_list;

  cairo_save (cr);

  ctk_cairo_transform_to_window (cr, widget, tree_view->priv->bin_window);
  ctk_tree_view_bin_draw (widget, cr);

  cairo_restore (cr);

  /* We can't just chain up to Container::draw as it will try to send the
   * event to the headers, so we handle propagating it to our children
   * (eg. widgets being edited) ourselves.
   */
  tmp_list = tree_view->priv->children;
  while (tmp_list)
    {
      CtkTreeViewChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      ctk_container_propagate_draw (CTK_CONTAINER (tree_view), child->widget, cr);
    }
}

static gboolean
ctk_tree_view_draw (CtkWidget *widget,
                    cairo_t   *cr)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  CtkWidget   *button;
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);

  if (ctk_cairo_should_draw_window (cr, tree_view->priv->bin_window))
    {
      cairo_rectangle_int_t view_rect;
      cairo_rectangle_int_t canvas_rect;

      view_rect.x = 0;
      view_rect.y = ctk_tree_view_get_effective_header_height (tree_view);
      view_rect.width = ctk_widget_get_allocated_width (widget);
      view_rect.height = ctk_widget_get_allocated_height (widget) - view_rect.y;

      cdk_window_get_position (tree_view->priv->bin_window, &canvas_rect.x, &canvas_rect.y);
      canvas_rect.y = -ctk_adjustment_get_value (tree_view->priv->vadjustment);
      canvas_rect.width = cdk_window_get_width (tree_view->priv->bin_window);
      canvas_rect.height = ctk_tree_view_get_height (tree_view);

      _ctk_pixel_cache_draw (tree_view->priv->pixel_cache, cr, tree_view->priv->bin_window,
			     &view_rect, &canvas_rect,
			     draw_bin, widget);
    }
  else if (tree_view->priv->drag_highlight_window &&
           ctk_cairo_should_draw_window (cr, tree_view->priv->drag_highlight_window))
    {
      CdkRGBA color;

      ctk_style_context_get_color (context, ctk_style_context_get_state (context), &color);
      cairo_save (cr);
      ctk_cairo_transform_to_window (cr, CTK_WIDGET (tree_view), tree_view->priv->drag_highlight_window);
      if (tree_view->priv->drag_column_window_state == DRAG_COLUMN_WINDOW_STATE_ORIGINAL)
        {
          cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.0);
          cairo_paint (cr);
          cdk_cairo_set_source_rgba (cr, &color);
          cairo_rectangle (cr,
                           1, 1,
                           cdk_window_get_width (tree_view->priv->drag_highlight_window) - 2,
                           cdk_window_get_height (tree_view->priv->drag_highlight_window) - 2);
          cairo_stroke (cr);
        }
      else
        {
          cdk_cairo_set_source_rgba (cr, &color);
          cairo_paint (cr);
        }
      cairo_restore (cr);
    }
  else
    {
      ctk_render_background (context, cr,
                             0, 0,
                             ctk_widget_get_allocated_width (widget),
                             ctk_widget_get_allocated_height (widget));
    }

  ctk_style_context_save (context);
  ctk_style_context_remove_class (context, CTK_STYLE_CLASS_VIEW);

  if (ctk_cairo_should_draw_window (cr, tree_view->priv->header_window))
    {
      GList *list;
      
      for (list = tree_view->priv->columns; list != NULL; list = list->next)
	{
	  CtkTreeViewColumn *column = list->data;

	  if (column == tree_view->priv->drag_column)
	    continue;

	  if (ctk_tree_view_column_get_visible (column))
	    {
	      button = ctk_tree_view_column_get_button (column);
	      ctk_container_propagate_draw (CTK_CONTAINER (tree_view),
					    button, cr);
	    }
	}
    }
  
  if (tree_view->priv->drag_window &&
      ctk_cairo_should_draw_window (cr, tree_view->priv->drag_window))
    {
      button = ctk_tree_view_column_get_button (tree_view->priv->drag_column);
      ctk_container_propagate_draw (CTK_CONTAINER (tree_view),
                                    button, cr);
    }

  ctk_style_context_restore (context);

  return FALSE;
}

enum
{
  DROP_HOME,
  DROP_RIGHT,
  DROP_LEFT,
  DROP_END
};

/* returns 0x1 when no column has been found -- yes it's hackish */
static CtkTreeViewColumn *
ctk_tree_view_get_drop_column (CtkTreeView       *tree_view,
			       CtkTreeViewColumn *column,
			       gint               drop_position)
{
  CtkTreeViewColumn *left_column = NULL;
  CtkTreeViewColumn *cur_column = NULL;
  GList *tmp_list;

  if (!ctk_tree_view_column_get_reorderable (column))
    return (CtkTreeViewColumn *)0x1;

  switch (drop_position)
    {
      case DROP_HOME:
	/* find first column where we can drop */
	tmp_list = tree_view->priv->columns;
	if (column == CTK_TREE_VIEW_COLUMN (tmp_list->data))
	  return (CtkTreeViewColumn *)0x1;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    cur_column = CTK_TREE_VIEW_COLUMN (tmp_list->data);
	    tmp_list = tmp_list->next;

	    if (left_column &&
                ctk_tree_view_column_get_visible (left_column) == FALSE)
	      continue;

	    if (!tree_view->priv->column_drop_func)
	      return left_column;

	    if (!tree_view->priv->column_drop_func (tree_view, column, left_column, cur_column, tree_view->priv->column_drop_func_data))
	      {
		left_column = cur_column;
		continue;
	      }

	    return left_column;
	  }

	if (!tree_view->priv->column_drop_func)
	  return left_column;

	if (tree_view->priv->column_drop_func (tree_view, column, left_column, NULL, tree_view->priv->column_drop_func_data))
	  return left_column;
	else
	  return (CtkTreeViewColumn *)0x1;
	break;

      case DROP_RIGHT:
	/* find first column after column where we can drop */
	tmp_list = tree_view->priv->columns;

	for (; tmp_list; tmp_list = tmp_list->next)
	  if (CTK_TREE_VIEW_COLUMN (tmp_list->data) == column)
	    break;

	if (!tmp_list || !tmp_list->next)
	  return (CtkTreeViewColumn *)0x1;

	tmp_list = tmp_list->next;
	left_column = CTK_TREE_VIEW_COLUMN (tmp_list->data);
	tmp_list = tmp_list->next;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    cur_column = CTK_TREE_VIEW_COLUMN (tmp_list->data);
	    tmp_list = tmp_list->next;

	    if (left_column &&
                ctk_tree_view_column_get_visible (left_column) == FALSE)
	      {
		left_column = cur_column;
		if (tmp_list)
		  tmp_list = tmp_list->next;
	        continue;
	      }

	    if (!tree_view->priv->column_drop_func)
	      return left_column;

	    if (!tree_view->priv->column_drop_func (tree_view, column, left_column, cur_column, tree_view->priv->column_drop_func_data))
	      {
		left_column = cur_column;
		continue;
	      }

	    return left_column;
	  }

	if (!tree_view->priv->column_drop_func)
	  return left_column;

	if (tree_view->priv->column_drop_func (tree_view, column, left_column, NULL, tree_view->priv->column_drop_func_data))
	  return left_column;
	else
	  return (CtkTreeViewColumn *)0x1;
	break;

      case DROP_LEFT:
	/* find first column before column where we can drop */
	tmp_list = tree_view->priv->columns;

	for (; tmp_list; tmp_list = tmp_list->next)
	  if (CTK_TREE_VIEW_COLUMN (tmp_list->data) == column)
	    break;

	if (!tmp_list || !tmp_list->prev)
	  return (CtkTreeViewColumn *)0x1;

	tmp_list = tmp_list->prev;
	cur_column = CTK_TREE_VIEW_COLUMN (tmp_list->data);
	tmp_list = tmp_list->prev;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    left_column = CTK_TREE_VIEW_COLUMN (tmp_list->data);

	    if (left_column &&
                ctk_tree_view_column_get_visible (left_column) == FALSE)
	      {
		/*if (!tmp_list->prev)
		  return (CtkTreeViewColumn *)0x1;
		  */
/*
		cur_column = CTK_TREE_VIEW_COLUMN (tmp_list->prev->data);
		tmp_list = tmp_list->prev->prev;
		continue;*/

		cur_column = left_column;
		if (tmp_list)
		  tmp_list = tmp_list->prev;
		continue;
	      }

	    if (!tree_view->priv->column_drop_func)
	      return left_column;

	    if (tree_view->priv->column_drop_func (tree_view, column, left_column, cur_column, tree_view->priv->column_drop_func_data))
	      return left_column;

	    cur_column = left_column;
	    tmp_list = tmp_list->prev;
	  }

	if (!tree_view->priv->column_drop_func)
	  return NULL;

	if (tree_view->priv->column_drop_func (tree_view, column, NULL, cur_column, tree_view->priv->column_drop_func_data))
	  return NULL;
	else
	  return (CtkTreeViewColumn *)0x1;
	break;

      case DROP_END:
	/* same as DROP_HOME case, but doing it backwards */
	tmp_list = g_list_last (tree_view->priv->columns);
	cur_column = NULL;

	if (column == CTK_TREE_VIEW_COLUMN (tmp_list->data))
	  return (CtkTreeViewColumn *)0x1;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    left_column = CTK_TREE_VIEW_COLUMN (tmp_list->data);

	    if (left_column &&
                ctk_tree_view_column_get_visible (left_column) == FALSE)
	      {
		cur_column = left_column;
		tmp_list = tmp_list->prev;
	      }

	    if (!tree_view->priv->column_drop_func)
	      return left_column;

	    if (tree_view->priv->column_drop_func (tree_view, column, left_column, cur_column, tree_view->priv->column_drop_func_data))
	      return left_column;

	    cur_column = left_column;
	    tmp_list = tmp_list->prev;
	  }

	if (!tree_view->priv->column_drop_func)
	  return NULL;

	if (tree_view->priv->column_drop_func (tree_view, column, NULL, cur_column, tree_view->priv->column_drop_func_data))
	  return NULL;
	else
	  return (CtkTreeViewColumn *)0x1;
	break;
    }

  return (CtkTreeViewColumn *)0x1;
}

static gboolean
ctk_tree_view_search_key_cancels_search (guint keyval)
{
  return keyval == CDK_KEY_Escape
      || keyval == CDK_KEY_Tab
      || keyval == CDK_KEY_KP_Tab
      || keyval == CDK_KEY_ISO_Left_Tab;
}

static gboolean
ctk_tree_view_key_press (CtkWidget   *widget,
			 CdkEventKey *event)
{
  CtkTreeView *tree_view = (CtkTreeView *) widget;
  CtkWidget   *button;

  if (tree_view->priv->rubber_band_status)
    {
      if (event->keyval == CDK_KEY_Escape)
	ctk_tree_view_stop_rubber_band (tree_view);

      return TRUE;
    }

  if (tree_view->priv->in_column_drag)
    {
      if (event->keyval == CDK_KEY_Escape)
        ctk_gesture_set_state (CTK_GESTURE (tree_view->priv->column_drag_gesture),
                               CTK_EVENT_SEQUENCE_DENIED);
      return TRUE;
    }

  if (tree_view->priv->headers_visible)
    {
      GList *focus_column;
      gboolean rtl;

      rtl = (ctk_widget_get_direction (CTK_WIDGET (tree_view)) == CTK_TEXT_DIR_RTL);

      for (focus_column = tree_view->priv->columns;
           focus_column;
           focus_column = focus_column->next)
        {
          CtkTreeViewColumn *column = CTK_TREE_VIEW_COLUMN (focus_column->data);
	  
	  button = ctk_tree_view_column_get_button (column);
          if (ctk_widget_has_focus (button))
            break;
        }

      if (focus_column &&
          (event->state & CDK_SHIFT_MASK) && (event->state & CDK_MOD1_MASK) &&
          (event->keyval == CDK_KEY_Left || event->keyval == CDK_KEY_KP_Left
           || event->keyval == CDK_KEY_Right || event->keyval == CDK_KEY_KP_Right))
        {
          CtkTreeViewColumn *column = CTK_TREE_VIEW_COLUMN (focus_column->data);
          gint column_width;

          if (!ctk_tree_view_column_get_resizable (column))
            {
              ctk_widget_error_bell (widget);
              return TRUE;
            }

	  column_width = ctk_tree_view_column_get_width (column);

          if (event->keyval == (rtl ? CDK_KEY_Right : CDK_KEY_Left)
              || event->keyval == (rtl ? CDK_KEY_KP_Right : CDK_KEY_KP_Left))
            {
	      column_width = MAX (column_width - 2, 0);
            }
          else if (event->keyval == (rtl ? CDK_KEY_Left : CDK_KEY_Right)
                   || event->keyval == (rtl ? CDK_KEY_KP_Left : CDK_KEY_KP_Right))
            {
	      column_width = column_width + 2;
            }

	  ctk_tree_view_column_set_fixed_width (column, column_width);
	  ctk_tree_view_column_set_expand (column, FALSE);
          return TRUE;
        }

      if (focus_column &&
          (event->state & CDK_MOD1_MASK) &&
          (event->keyval == CDK_KEY_Left || event->keyval == CDK_KEY_KP_Left
           || event->keyval == CDK_KEY_Right || event->keyval == CDK_KEY_KP_Right
           || event->keyval == CDK_KEY_Home || event->keyval == CDK_KEY_KP_Home
           || event->keyval == CDK_KEY_End || event->keyval == CDK_KEY_KP_End))
        {
          CtkTreeViewColumn *column = CTK_TREE_VIEW_COLUMN (focus_column->data);

          if (event->keyval == (rtl ? CDK_KEY_Right : CDK_KEY_Left)
              || event->keyval == (rtl ? CDK_KEY_KP_Right : CDK_KEY_KP_Left))
            {
              CtkTreeViewColumn *col;
              col = ctk_tree_view_get_drop_column (tree_view, column, DROP_LEFT);
              if (col != (CtkTreeViewColumn *)0x1)
                ctk_tree_view_move_column_after (tree_view, column, col);
              else
                ctk_widget_error_bell (widget);
            }
          else if (event->keyval == (rtl ? CDK_KEY_Left : CDK_KEY_Right)
                   || event->keyval == (rtl ? CDK_KEY_KP_Left : CDK_KEY_KP_Right))
            {
              CtkTreeViewColumn *col;
              col = ctk_tree_view_get_drop_column (tree_view, column, DROP_RIGHT);
              if (col != (CtkTreeViewColumn *)0x1)
                ctk_tree_view_move_column_after (tree_view, column, col);
              else
                ctk_widget_error_bell (widget);
            }
          else if (event->keyval == CDK_KEY_Home || event->keyval == CDK_KEY_KP_Home)
            {
              CtkTreeViewColumn *col;
              col = ctk_tree_view_get_drop_column (tree_view, column, DROP_HOME);
              if (col != (CtkTreeViewColumn *)0x1)
                ctk_tree_view_move_column_after (tree_view, column, col);
              else
                ctk_widget_error_bell (widget);
            }
          else if (event->keyval == CDK_KEY_End || event->keyval == CDK_KEY_KP_End)
            {
              CtkTreeViewColumn *col;
              col = ctk_tree_view_get_drop_column (tree_view, column, DROP_END);
              if (col != (CtkTreeViewColumn *)0x1)
                ctk_tree_view_move_column_after (tree_view, column, col);
              else
                ctk_widget_error_bell (widget);
            }

          return TRUE;
        }
    }

  /* Chain up to the parent class.  It handles the keybindings. */
  if (CTK_WIDGET_CLASS (ctk_tree_view_parent_class)->key_press_event (widget, event))
    return TRUE;

  if (tree_view->priv->search_entry_avoid_unhandled_binding)
    {
      tree_view->priv->search_entry_avoid_unhandled_binding = FALSE;
      return FALSE;
    }

  /* Initially, before the search window is visible, we pass the event to the
   * IM context of the search entry box. If it triggers a commit or a preedit,
   * we then show the search window without loosing tree view focus.
   * If the seach window is already visible, we forward the events to it,
   * keeping the focus on the tree view.
   */
  if (ctk_widget_has_focus (CTK_WIDGET (tree_view))
      && tree_view->priv->enable_search
      && !tree_view->priv->search_custom_entry_set
      && !ctk_tree_view_search_key_cancels_search (event->keyval))
    {
      CtkWidget *search_window;

      ctk_tree_view_ensure_interactive_directory (tree_view);

      search_window = tree_view->priv->search_window;
      if (!ctk_widget_is_visible (search_window))
        {
          CtkIMContext *im_context =
            _ctk_entry_get_im_context (CTK_ENTRY (tree_view->priv->search_entry));

          tree_view->priv->imcontext_changed = FALSE;
          ctk_im_context_filter_keypress (im_context, event);

          if (tree_view->priv->imcontext_changed)
            {
              CdkDevice *device;

              device = cdk_event_get_device ((CdkEvent *) event);
              if (ctk_tree_view_real_start_interactive_search (tree_view,
                                                               device,
                                                               FALSE))
                {
                  ctk_widget_grab_focus (CTK_WIDGET (tree_view));
                  return TRUE;
                }
              else
                {
                  ctk_entry_set_text (CTK_ENTRY (tree_view->priv->search_entry), "");
                  return FALSE;
                }
            }
        }
      else
        {
          CdkEvent *new_event;
          gulong popup_menu_id;

          new_event = cdk_event_copy ((CdkEvent *) event);
          g_object_unref (((CdkEventKey *) new_event)->window);
          ((CdkEventKey *) new_event)->window =
            g_object_ref (ctk_widget_get_window (search_window));
          ctk_widget_realize (search_window);

          popup_menu_id = g_signal_connect (tree_view->priv->search_entry,
                                            "popup-menu", G_CALLBACK (ctk_true),
                                            NULL);

          /* Because we keep the focus on the treeview, we need to forward the
           * key events to the entry, when it is visible. */
          ctk_widget_event (search_window, new_event);
          cdk_event_free (new_event);

          g_signal_handler_disconnect (tree_view->priv->search_entry,
                                       popup_menu_id);

        }
    }

  return FALSE;
}

static gboolean
ctk_tree_view_key_release (CtkWidget   *widget,
			   CdkEventKey *event)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);

  if (tree_view->priv->rubber_band_status)
    return TRUE;

  return CTK_WIDGET_CLASS (ctk_tree_view_parent_class)->key_release_event (widget, event);
}

/* FIXME Is this function necessary? Can I get an enter_notify event
 * w/o either an expose event or a mouse motion event?
 */
static gboolean
ctk_tree_view_enter_notify (CtkWidget        *widget,
			    CdkEventCrossing *event)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  CtkRBTree *tree;
  CtkRBNode *node;
  gint new_y;

  /* Sanity check it */
  if (event->window != tree_view->priv->bin_window)
    return FALSE;

  if (tree_view->priv->tree == NULL)
    return FALSE;

  /* find the node internally */
  new_y = TREE_WINDOW_Y_TO_RBTREE_Y(tree_view, event->y);
  if (new_y < 0)
    new_y = 0;
  _ctk_rbtree_find_offset (tree_view->priv->tree, new_y, &tree, &node);

  tree_view->priv->event_last_x = event->x;
  tree_view->priv->event_last_y = event->y;

  if ((tree_view->priv->button_pressed_node == NULL) ||
      (tree_view->priv->button_pressed_node == node))
    prelight_or_select (tree_view, tree, node, event->x, event->y);

  return TRUE;
}

static gboolean
ctk_tree_view_leave_notify (CtkWidget        *widget,
			    CdkEventCrossing *event G_GNUC_UNUSED)
{
  CtkTreeView *tree_view;

  tree_view = CTK_TREE_VIEW (widget);

  if (tree_view->priv->prelight_node)
    _ctk_tree_view_queue_draw_node (tree_view,
                                   tree_view->priv->prelight_tree,
                                   tree_view->priv->prelight_node,
                                   NULL);

  tree_view->priv->event_last_x = -10000;
  tree_view->priv->event_last_y = -10000;

  prelight_or_select (tree_view,
		      NULL, NULL,
		      -1000, -1000); /* coords not possibly over an arrow */

  return TRUE;
}


static gint
ctk_tree_view_focus_out (CtkWidget     *widget,
			 CdkEventFocus *event)
{
  CtkTreeView *tree_view;

  tree_view = CTK_TREE_VIEW (widget);

  ctk_widget_queue_draw (widget);

  /* destroy interactive search dialog */
  if (tree_view->priv->search_window)
    ctk_tree_view_search_window_hide (tree_view->priv->search_window, tree_view,
                                      cdk_event_get_device ((CdkEvent *) event));
  return FALSE;
}


/* Incremental Reflow
 */

static void
ctk_tree_view_node_queue_redraw (CtkTreeView *tree_view,
				 CtkRBTree   *tree,
				 CtkRBNode   *node)
{
  CdkRectangle rect;

  rect.x = 0;
  rect.y =
    _ctk_rbtree_node_find_offset (tree, node)
    - ctk_adjustment_get_value (tree_view->priv->vadjustment);
  rect.width = ctk_widget_get_allocated_width (CTK_WIDGET (tree_view));
  rect.height = CTK_RBNODE_GET_HEIGHT (node);

  cdk_window_invalidate_rect (tree_view->priv->bin_window,
			      &rect, TRUE);
}

static gboolean
node_is_visible (CtkTreeView *tree_view,
		 CtkRBTree   *tree,
		 CtkRBNode   *node)
{
  int y;
  int height;

  y = _ctk_rbtree_node_find_offset (tree, node);
  height = ctk_tree_view_get_row_height (tree_view, node);

  if (y >= ctk_adjustment_get_value (tree_view->priv->vadjustment) &&
      y + height <= (ctk_adjustment_get_value (tree_view->priv->vadjustment)
	             + ctk_adjustment_get_page_size (tree_view->priv->vadjustment)))
    return TRUE;

  return FALSE;
}

static gint
get_separator_height (CtkTreeView *tree_view)
{
  CtkStyleContext *context;
  CtkCssStyle *style;
  gdouble d;
  gint min_size;

  context = ctk_widget_get_style_context (CTK_WIDGET (tree_view));
  ctk_style_context_save (context);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_SEPARATOR);

  style = ctk_style_context_lookup_style (context);
  d = _ctk_css_number_value_get
    (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_MIN_HEIGHT), 100);

  if (d < 1)
    min_size = ceil (d);
  else
    min_size = floor (d);

  ctk_style_context_restore (context);

  return min_size;
}

/* Returns TRUE if it updated the size
 */
static gboolean
validate_row (CtkTreeView *tree_view,
	      CtkRBTree   *tree,
	      CtkRBNode   *node,
	      CtkTreeIter *iter,
	      CtkTreePath *path)
{
  CtkTreeViewColumn *column;
  CtkStyleContext *context;
  GList *list, *first_column, *last_column;
  gint height = 0;
  gint horizontal_separator;
  gint vertical_separator;
  gint depth = ctk_tree_path_get_depth (path);
  gboolean retval = FALSE;
  gboolean is_separator = FALSE;
  gboolean draw_vgrid_lines, draw_hgrid_lines;
  gint grid_line_width;
  gint expander_size;

  /* double check the row needs validating */
  if (! CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID) &&
      ! CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_COLUMN_INVALID))
    return FALSE;

  is_separator = row_is_separator (tree_view, iter, NULL);

  ctk_widget_style_get (CTK_WIDGET (tree_view),
			"horizontal-separator", &horizontal_separator,
			"vertical-separator", &vertical_separator,
			"grid-line-width", &grid_line_width,
			NULL);
  
  draw_vgrid_lines =
    tree_view->priv->grid_lines == CTK_TREE_VIEW_GRID_LINES_VERTICAL
    || tree_view->priv->grid_lines == CTK_TREE_VIEW_GRID_LINES_BOTH;
  draw_hgrid_lines =
    tree_view->priv->grid_lines == CTK_TREE_VIEW_GRID_LINES_HORIZONTAL
    || tree_view->priv->grid_lines == CTK_TREE_VIEW_GRID_LINES_BOTH;
  expander_size = ctk_tree_view_get_expander_size (tree_view);

  for (last_column = g_list_last (tree_view->priv->columns);
       last_column &&
       !(ctk_tree_view_column_get_visible (CTK_TREE_VIEW_COLUMN (last_column->data)));
       last_column = last_column->prev)
    ;

  for (first_column = g_list_first (tree_view->priv->columns);
       first_column &&
       !(ctk_tree_view_column_get_visible (CTK_TREE_VIEW_COLUMN (first_column->data)));
       first_column = first_column->next)
    ;

  context = ctk_widget_get_style_context (CTK_WIDGET (tree_view));
  ctk_style_context_save (context);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_CELL);

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      gint padding = 0;
      gint original_width;
      gint new_width;
      gint row_height;

      column = list->data;

      if (!ctk_tree_view_column_get_visible (column))
	continue;

      if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_COLUMN_INVALID) && 
	  !_ctk_tree_view_column_cell_get_dirty (column))
	continue;

      original_width = _ctk_tree_view_column_get_requested_width (column);

      ctk_tree_view_column_cell_set_cell_data (column, tree_view->priv->model, iter,
					       CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_PARENT),
					       node->children?TRUE:FALSE);
      ctk_tree_view_column_cell_get_size (column,
					  NULL, NULL, NULL,
					  NULL, &row_height);

      if (is_separator)
        {
          height = get_separator_height (tree_view);
          /* ctk_tree_view_get_row_height() assumes separator nodes are > 0 */
          height = MAX (height, 1);
        }
      else
        {
          row_height += vertical_separator;
          height = MAX (height, row_height);
          height = MAX (height, expander_size);
        }

      if (ctk_tree_view_is_expander_column (tree_view, column))
        {
	  padding += horizontal_separator + (depth - 1) * tree_view->priv->level_indentation;

	  if (ctk_tree_view_draw_expanders (tree_view))
	    padding += depth * expander_size;
	}
      else
	padding += horizontal_separator;

      if (draw_vgrid_lines)
        {
	  if (list->data == first_column || list->data == last_column)
	    padding += grid_line_width / 2.0;
	  else
	    padding += grid_line_width;
	}

      /* Update the padding for the column */
      _ctk_tree_view_column_push_padding (column, padding);
      new_width = _ctk_tree_view_column_get_requested_width (column);

      if (new_width > original_width)
	retval = TRUE;
    }

  ctk_style_context_restore (context);

  if (draw_hgrid_lines)
    height += grid_line_width;

  if (height != CTK_RBNODE_GET_HEIGHT (node))
    {
      retval = TRUE;
      _ctk_rbtree_node_set_height (tree, node, height);
    }
  _ctk_rbtree_node_mark_valid (tree, node);
  tree_view->priv->post_validation_flag = TRUE;

  return retval;
}


static void
validate_visible_area (CtkTreeView *tree_view)
{
  CtkAllocation allocation;
  CtkTreePath *path = NULL;
  CtkTreePath *above_path = NULL;
  CtkTreeIter iter;
  CtkRBTree *tree = NULL;
  CtkRBNode *node = NULL;
  gboolean need_redraw = FALSE;
  gboolean size_changed = FALSE;
  gint total_height;
  gint area_above = 0;
  gint area_below = 0;

  if (tree_view->priv->tree == NULL)
    return;

  if (! CTK_RBNODE_FLAG_SET (tree_view->priv->tree->root, CTK_RBNODE_DESCENDANTS_INVALID) &&
      tree_view->priv->scroll_to_path == NULL)
    return;

  ctk_widget_get_allocation (CTK_WIDGET (tree_view), &allocation);
  total_height = allocation.height - ctk_tree_view_get_effective_header_height (tree_view);

  if (total_height == 0)
    return;

  /* First, we check to see if we need to scroll anywhere
   */
  if (tree_view->priv->scroll_to_path)
    {
      path = ctk_tree_row_reference_get_path (tree_view->priv->scroll_to_path);
      if (path && !_ctk_tree_view_find_node (tree_view, path, &tree, &node))
	{
          /* we are going to scroll, and will update dy */
	  ctk_tree_model_get_iter (tree_view->priv->model, &iter, path);
	  if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID) ||
	      CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_COLUMN_INVALID))
	    {
	      _ctk_tree_view_queue_draw_node (tree_view, tree, node, NULL);
	      if (validate_row (tree_view, tree, node, &iter, path))
		size_changed = TRUE;
	    }

	  if (tree_view->priv->scroll_to_use_align)
	    {
	      gint height = ctk_tree_view_get_row_height (tree_view, node);
	      area_above = (total_height - height) *
		tree_view->priv->scroll_to_row_align;
	      area_below = total_height - area_above - height;
	      area_above = MAX (area_above, 0);
	      area_below = MAX (area_below, 0);
	    }
	  else
	    {
	      /* two cases:
	       * 1) row not visible
	       * 2) row visible
	       */
	      gint dy;
	      gint height = ctk_tree_view_get_row_height (tree_view, node);

	      dy = _ctk_rbtree_node_find_offset (tree, node);

	      if (dy >= ctk_adjustment_get_value (tree_view->priv->vadjustment) &&
		  dy + height <= (ctk_adjustment_get_value (tree_view->priv->vadjustment)
		                  + ctk_adjustment_get_page_size (tree_view->priv->vadjustment)))
	        {
		  /* row visible: keep the row at the same position */
		  area_above = dy - ctk_adjustment_get_value (tree_view->priv->vadjustment);
		  area_below = (ctk_adjustment_get_value (tree_view->priv->vadjustment) +
		                ctk_adjustment_get_page_size (tree_view->priv->vadjustment))
		               - dy - height;
		}
	      else
	        {
		  /* row not visible */
		  if (dy >= 0
		      && dy + height <= ctk_adjustment_get_page_size (tree_view->priv->vadjustment))
		    {
		      /* row at the beginning -- fixed */
		      area_above = dy;
		      area_below = ctk_adjustment_get_page_size (tree_view->priv->vadjustment)
				   - area_above - height;
		    }
		  else if (dy >= (ctk_adjustment_get_upper (tree_view->priv->vadjustment) -
			          ctk_adjustment_get_page_size (tree_view->priv->vadjustment)))
		    {
		      /* row at the end -- fixed */
		      area_above = dy - (ctk_adjustment_get_upper (tree_view->priv->vadjustment) -
			           ctk_adjustment_get_page_size (tree_view->priv->vadjustment));
                      area_below = ctk_adjustment_get_page_size (tree_view->priv->vadjustment) -
                                   area_above - height;

                      if (area_below < 0)
                        {
			  area_above = ctk_adjustment_get_page_size (tree_view->priv->vadjustment) - height;
                          area_below = 0;
                        }
		    }
		  else
		    {
		      /* row somewhere in the middle, bring it to the top
		       * of the view
		       */
		      area_above = 0;
		      area_below = total_height - height;
		    }
		}
	    }
	}
      else
	/* the scroll to isn't valid; ignore it.
	 */
	{
	  if (tree_view->priv->scroll_to_path && !path)
	    {
	      ctk_tree_row_reference_free (tree_view->priv->scroll_to_path);
	      tree_view->priv->scroll_to_path = NULL;
	    }
	  if (path)
	    ctk_tree_path_free (path);
	  path = NULL;
	}      
    }

  /* We didn't have a scroll_to set, so we just handle things normally
   */
  if (path == NULL)
    {
      gint offset;

      offset = _ctk_rbtree_find_offset (tree_view->priv->tree,
					TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, 0),
					&tree, &node);
      if (node == NULL)
	{
	  /* In this case, nothing has been validated */
	  path = ctk_tree_path_new_first ();
	  _ctk_tree_view_find_node (tree_view, path, &tree, &node);
	}
      else
	{
	  path = _ctk_tree_path_new_from_rbtree (tree, node);
	  total_height += offset;
	}

      ctk_tree_model_get_iter (tree_view->priv->model, &iter, path);

      if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID) ||
	  CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_COLUMN_INVALID))
	{
	  _ctk_tree_view_queue_draw_node (tree_view, tree, node, NULL);
	  if (validate_row (tree_view, tree, node, &iter, path))
	    size_changed = TRUE;
	}
      area_above = 0;
      area_below = total_height - ctk_tree_view_get_row_height (tree_view, node);
    }

  above_path = ctk_tree_path_copy (path);

  /* if we do not validate any row above the new top_row, we will make sure
   * that the row immediately above top_row has been validated. (if we do not
   * do this, _ctk_rbtree_find_offset will find the row above top_row, because
   * when invalidated that row's height will be zero. and this will mess up
   * scrolling).
   */
  if (area_above == 0)
    {
      CtkRBTree *tmptree;
      CtkRBNode *tmpnode;

      _ctk_tree_view_find_node (tree_view, above_path, &tmptree, &tmpnode);
      _ctk_rbtree_prev_full (tmptree, tmpnode, &tmptree, &tmpnode);

      if (tmpnode)
        {
	  CtkTreePath *tmppath;
	  CtkTreeIter tmpiter;

	  tmppath = _ctk_tree_path_new_from_rbtree (tmptree, tmpnode);
	  ctk_tree_model_get_iter (tree_view->priv->model, &tmpiter, tmppath);

	  if (CTK_RBNODE_FLAG_SET (tmpnode, CTK_RBNODE_INVALID) ||
	      CTK_RBNODE_FLAG_SET (tmpnode, CTK_RBNODE_COLUMN_INVALID))
	    {
	      _ctk_tree_view_queue_draw_node (tree_view, tmptree, tmpnode, NULL);
	      if (validate_row (tree_view, tmptree, tmpnode, &tmpiter, tmppath))
		size_changed = TRUE;
	    }

	  ctk_tree_path_free (tmppath);
	}
    }

  /* Now, we walk forwards and backwards, measuring rows. Unfortunately,
   * backwards is much slower then forward, as there is no iter_prev function.
   * We go forwards first in case we run out of tree.  Then we go backwards to
   * fill out the top.
   */
  while (node && area_below > 0)
    {
      if (node->children)
	{
	  CtkTreeIter parent = iter;
	  gboolean has_child;

	  tree = node->children;
          node = _ctk_rbtree_first (tree);

	  has_child = ctk_tree_model_iter_children (tree_view->priv->model,
						    &iter,
						    &parent);
	  TREE_VIEW_INTERNAL_ASSERT_VOID (has_child);
	  ctk_tree_path_down (path);
	}
      else
	{
	  gboolean done = FALSE;
	  do
	    {
	      node = _ctk_rbtree_next (tree, node);
	      if (node != NULL)
		{
		  gboolean has_next = ctk_tree_model_iter_next (tree_view->priv->model, &iter);
		  done = TRUE;
		  ctk_tree_path_next (path);

		  /* Sanity Check! */
		  TREE_VIEW_INTERNAL_ASSERT_VOID (has_next);
		}
	      else
		{
		  CtkTreeIter parent_iter = iter;
		  gboolean has_parent;

		  node = tree->parent_node;
		  tree = tree->parent_tree;
		  if (tree == NULL)
		    break;
		  has_parent = ctk_tree_model_iter_parent (tree_view->priv->model,
							   &iter,
							   &parent_iter);
		  ctk_tree_path_up (path);

		  /* Sanity check */
		  TREE_VIEW_INTERNAL_ASSERT_VOID (has_parent);
		}
	    }
	  while (!done);
	}

      if (!node)
        break;

      if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID) ||
	  CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_COLUMN_INVALID))
	{
	  _ctk_tree_view_queue_draw_node (tree_view, tree, node, NULL);
	  if (validate_row (tree_view, tree, node, &iter, path))
	      size_changed = TRUE;
	}

      area_below -= ctk_tree_view_get_row_height (tree_view, node);
    }
  ctk_tree_path_free (path);

  /* If we ran out of tree, and have extra area_below left, we need to add it
   * to area_above */
  if (area_below > 0)
    area_above += area_below;

  _ctk_tree_view_find_node (tree_view, above_path, &tree, &node);

  /* We walk backwards */
  while (area_above > 0)
    {
      _ctk_rbtree_prev_full (tree, node, &tree, &node);

      /* Always find the new path in the tree.  We cannot just assume
       * a ctk_tree_path_prev() is enough here, as there might be children
       * in between this node and the previous sibling node.  If this
       * appears to be a performance hotspot in profiles, we can look into
       * intrigate logic for keeping path, node and iter in sync like
       * we do for forward walks.  (Which will be hard because of the lacking
       * iter_prev).
       */

      if (node == NULL)
	break;

      ctk_tree_path_free (above_path);
      above_path = _ctk_tree_path_new_from_rbtree (tree, node);

      ctk_tree_model_get_iter (tree_view->priv->model, &iter, above_path);

      if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID) ||
	  CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_COLUMN_INVALID))
	{
	  _ctk_tree_view_queue_draw_node (tree_view, tree, node, NULL);
	  if (validate_row (tree_view, tree, node, &iter, above_path))
	    size_changed = TRUE;
	}
      area_above -= ctk_tree_view_get_row_height (tree_view, node);
    }

  /* if we scrolled to a path, we need to set the dy here,
   * and sync the top row accordingly
   */
  if (tree_view->priv->scroll_to_path)
    {
      ctk_tree_view_set_top_row (tree_view, above_path, -area_above);
      ctk_tree_view_top_row_to_dy (tree_view);

      need_redraw = TRUE;
    }
  else if (ctk_tree_view_get_height (tree_view) <= ctk_adjustment_get_page_size (tree_view->priv->vadjustment))
    {
      /* when we are not scrolling, we should never set dy to something
       * else than zero. we update top_row to be in sync with dy = 0.
       */
      ctk_adjustment_set_value (CTK_ADJUSTMENT (tree_view->priv->vadjustment), 0);
      ctk_tree_view_dy_to_top_row (tree_view);
    }
  else if (ctk_adjustment_get_value (tree_view->priv->vadjustment) + ctk_adjustment_get_page_size (tree_view->priv->vadjustment) > ctk_tree_view_get_height (tree_view))
    {
      ctk_adjustment_set_value (CTK_ADJUSTMENT (tree_view->priv->vadjustment), ctk_tree_view_get_height (tree_view) - ctk_adjustment_get_page_size (tree_view->priv->vadjustment));
      ctk_tree_view_dy_to_top_row (tree_view);
    }
  else
    ctk_tree_view_top_row_to_dy (tree_view);

  /* update width/height and queue a resize */
  if (size_changed)
    {
      CtkRequisition requisition;

      /* We temporarily guess a size, under the assumption that it will be the
       * same when we get our next size_allocate.  If we don't do this, we'll be
       * in an inconsistent state if we call top_row_to_dy. */

      ctk_widget_get_preferred_size (CTK_WIDGET (tree_view),
                                     &requisition, NULL);
      ctk_adjustment_set_upper (tree_view->priv->hadjustment,
                                MAX (ctk_adjustment_get_upper (tree_view->priv->hadjustment), requisition.width));
      ctk_adjustment_set_upper (tree_view->priv->vadjustment,
                                MAX (ctk_adjustment_get_upper (tree_view->priv->vadjustment), requisition.height));
      ctk_widget_queue_resize (CTK_WIDGET (tree_view));
    }

  if (tree_view->priv->scroll_to_path)
    {
      ctk_tree_row_reference_free (tree_view->priv->scroll_to_path);
      tree_view->priv->scroll_to_path = NULL;
    }

  if (above_path)
    ctk_tree_path_free (above_path);

  if (tree_view->priv->scroll_to_column)
    {
      tree_view->priv->scroll_to_column = NULL;
    }
  if (need_redraw)
    ctk_widget_queue_draw (CTK_WIDGET (tree_view));
}

static void
initialize_fixed_height_mode (CtkTreeView *tree_view)
{
  if (!tree_view->priv->tree)
    return;

  if (tree_view->priv->fixed_height < 0)
    {
      CtkTreeIter iter;
      CtkTreePath *path;

      CtkRBTree *tree = NULL;
      CtkRBNode *node = NULL;

      tree = tree_view->priv->tree;
      node = tree->root;

      path = _ctk_tree_path_new_from_rbtree (tree, node);
      ctk_tree_model_get_iter (tree_view->priv->model, &iter, path);

      validate_row (tree_view, tree, node, &iter, path);

      ctk_tree_path_free (path);

      tree_view->priv->fixed_height = ctk_tree_view_get_row_height (tree_view, node);
    }

   _ctk_rbtree_set_fixed_height (tree_view->priv->tree,
                                 tree_view->priv->fixed_height, TRUE);
}

/* Our strategy for finding nodes to validate is a little convoluted.  We find
 * the left-most uninvalidated node.  We then try walking right, validating
 * nodes.  Once we find a valid node, we repeat the previous process of finding
 * the first invalid node.
 */

static gboolean
do_validate_rows (CtkTreeView *tree_view, gboolean queue_resize)
{
  static gboolean prevent_recursion_hack = FALSE;

  CtkRBTree *tree = NULL;
  CtkRBNode *node = NULL;
  gboolean validated_area = FALSE;
  gint retval = TRUE;
  CtkTreePath *path = NULL;
  CtkTreeIter iter;
  GTimer *timer;
  gint i = 0;

  gint y = -1;
  gint prev_height = -1;
  gboolean fixed_height = TRUE;

  g_assert (tree_view);

  /* prevent infinite recursion via get_preferred_width() */
  if (prevent_recursion_hack)
    return FALSE;

  if (tree_view->priv->tree == NULL)
      return FALSE;

  if (tree_view->priv->fixed_height_mode)
    {
      if (tree_view->priv->fixed_height < 0)
        initialize_fixed_height_mode (tree_view);

      return FALSE;
    }

  timer = g_timer_new ();
  g_timer_start (timer);

  do
    {
      gboolean changed = FALSE;

      if (! CTK_RBNODE_FLAG_SET (tree_view->priv->tree->root, CTK_RBNODE_DESCENDANTS_INVALID))
	{
	  retval = FALSE;
	  goto done;
	}

      if (path != NULL)
	{
	  node = _ctk_rbtree_next (tree, node);
	  if (node != NULL)
	    {
	      TREE_VIEW_INTERNAL_ASSERT (ctk_tree_model_iter_next (tree_view->priv->model, &iter), FALSE);
	      ctk_tree_path_next (path);
	    }
	  else
	    {
	      ctk_tree_path_free (path);
	      path = NULL;
	    }
	}

      if (path == NULL)
	{
	  tree = tree_view->priv->tree;
	  node = tree_view->priv->tree->root;

	  g_assert (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_DESCENDANTS_INVALID));

	  do
	    {
	      if (!_ctk_rbtree_is_nil (node->left) &&
		  CTK_RBNODE_FLAG_SET (node->left, CTK_RBNODE_DESCENDANTS_INVALID))
		{
		  node = node->left;
		}
              else if (!_ctk_rbtree_is_nil (node->right) &&
		       CTK_RBNODE_FLAG_SET (node->right, CTK_RBNODE_DESCENDANTS_INVALID))
		{
		  node = node->right;
		}
	      else if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID) ||
		       CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_COLUMN_INVALID))
		{
		  break;
		}
	      else if (node->children != NULL)
		{
		  tree = node->children;
		  node = tree->root;
		}
	      else
		/* RBTree corruption!  All bad */
		g_assert_not_reached ();
	    }
	  while (TRUE);
	  path = _ctk_tree_path_new_from_rbtree (tree, node);
	  ctk_tree_model_get_iter (tree_view->priv->model, &iter, path);
	}

      changed = validate_row (tree_view, tree, node, &iter, path);
      validated_area = changed || validated_area;

      if (changed)
        {
          gint offset = ctk_tree_view_get_row_y_offset (tree_view, tree, node);

          if (y == -1 || y > offset)
            y = offset;
        }

      if (!tree_view->priv->fixed_height_check)
        {
	  gint height;

	  height = ctk_tree_view_get_row_height (tree_view, node);
	  if (prev_height < 0)
	    prev_height = height;
	  else if (prev_height != height)
	    fixed_height = FALSE;
	}

      i++;
    }
  while (g_timer_elapsed (timer, NULL) < CTK_TREE_VIEW_TIME_MS_PER_IDLE / 1000.);

  if (!tree_view->priv->fixed_height_check)
   {
     if (fixed_height)
       _ctk_rbtree_set_fixed_height (tree_view->priv->tree, prev_height, FALSE);

     tree_view->priv->fixed_height_check = 1;
   }
  
 done:
  if (validated_area)
    {
      CtkRequisition requisition;
      gint dummy;

      /* We temporarily guess a size, under the assumption that it will be the
       * same when we get our next size_allocate.  If we don't do this, we'll be
       * in an inconsistent state when we call top_row_to_dy. */

      /* FIXME: This is called from size_request, for some reason it is not infinitely
       * recursing, we cannot call ctk_widget_get_preferred_size() here because that's
       * not allowed (from inside ->get_preferred_width/height() implementations, one
       * should call the vfuncs directly). However what is desired here is the full
       * size including any margins and limited by any alignment (i.e. after 
       * CtkWidget:adjust_size_request() is called).
       *
       * Currently bypassing this but the real solution is to not update the scroll adjustments
       * untill we've recieved an allocation (never update scroll adjustments from size-requests).
       */
      prevent_recursion_hack = TRUE;
      ctk_tree_view_get_preferred_width (CTK_WIDGET (tree_view), &requisition.width, &dummy);
      ctk_tree_view_get_preferred_height (CTK_WIDGET (tree_view), &requisition.height, &dummy);
      prevent_recursion_hack = FALSE;

      /* If rows above the current position have changed height, this has
       * affected the current view and thus needs a redraw.
       */
      if (y != -1 && y < ctk_adjustment_get_value (tree_view->priv->vadjustment))
        ctk_widget_queue_draw (CTK_WIDGET (tree_view));

      ctk_adjustment_set_upper (tree_view->priv->hadjustment,
                                MAX (ctk_adjustment_get_upper (tree_view->priv->hadjustment), requisition.width));
      ctk_adjustment_set_upper (tree_view->priv->vadjustment,
                                MAX (ctk_adjustment_get_upper (tree_view->priv->vadjustment), requisition.height));

      if (queue_resize)
        ctk_widget_queue_resize_no_redraw (CTK_WIDGET (tree_view));
    }

  if (path) ctk_tree_path_free (path);
  g_timer_destroy (timer);

  if (!retval && ctk_widget_get_mapped (CTK_WIDGET (tree_view)))
    update_prelight (tree_view,
                     tree_view->priv->event_last_x,
                     tree_view->priv->event_last_y);

  return retval;
}

static void
disable_adjustment_animation (CtkTreeView *tree_view)
{
  ctk_adjustment_enable_animation (tree_view->priv->vadjustment,
                                   NULL,
                                   ctk_adjustment_get_animation_duration (tree_view->priv->vadjustment));
}

static void
maybe_reenable_adjustment_animation (CtkTreeView *tree_view)
{
  if (tree_view->priv->presize_handler_tick_cb != 0 ||
      tree_view->priv->validate_rows_timer != 0)
    return;

  ctk_adjustment_enable_animation (tree_view->priv->vadjustment,
                                   ctk_widget_get_frame_clock (CTK_WIDGET (tree_view)),
                                   ctk_adjustment_get_animation_duration (tree_view->priv->vadjustment));
}

static gboolean
do_presize_handler (CtkTreeView *tree_view)
{
  if (tree_view->priv->mark_rows_col_dirty)
   {
      if (tree_view->priv->tree)
	_ctk_rbtree_column_invalid (tree_view->priv->tree);
      tree_view->priv->mark_rows_col_dirty = FALSE;
    }
  validate_visible_area (tree_view);
  if (tree_view->priv->presize_handler_tick_cb != 0)
    {
      ctk_widget_remove_tick_callback (CTK_WIDGET (tree_view), tree_view->priv->presize_handler_tick_cb);
      tree_view->priv->presize_handler_tick_cb = 0;
    }

  if (tree_view->priv->fixed_height_mode)
    {
      CtkRequisition requisition;

      ctk_widget_get_preferred_size (CTK_WIDGET (tree_view),
                                     &requisition, NULL);

      ctk_adjustment_set_upper (tree_view->priv->hadjustment,
                                MAX (ctk_adjustment_get_upper (tree_view->priv->hadjustment), requisition.width));
      ctk_adjustment_set_upper (tree_view->priv->vadjustment,
                                MAX (ctk_adjustment_get_upper (tree_view->priv->vadjustment), requisition.height));
      ctk_widget_queue_resize (CTK_WIDGET (tree_view));
    }

  maybe_reenable_adjustment_animation (tree_view);

  return FALSE;
}

static gboolean
presize_handler_callback (CtkWidget     *widget,
                          CdkFrameClock *clock G_GNUC_UNUSED,
                          gpointer       unused G_GNUC_UNUSED)
{
  do_presize_handler (CTK_TREE_VIEW (widget));
		   
  return G_SOURCE_REMOVE;
}

static gboolean
validate_rows (CtkTreeView *tree_view)
{
  gboolean retval;
  
  if (tree_view->priv->presize_handler_tick_cb)
    {
      do_presize_handler (tree_view);
      return G_SOURCE_CONTINUE;
    }

  retval = do_validate_rows (tree_view, TRUE);
  
  if (! retval && tree_view->priv->validate_rows_timer)
    {
      g_source_remove (tree_view->priv->validate_rows_timer);
      tree_view->priv->validate_rows_timer = 0;
      maybe_reenable_adjustment_animation (tree_view);
    }

  return retval;
}

static void
install_presize_handler (CtkTreeView *tree_view)
{
  if (! ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    return;

  disable_adjustment_animation (tree_view);

  if (! tree_view->priv->presize_handler_tick_cb)
    {
      tree_view->priv->presize_handler_tick_cb =
	ctk_widget_add_tick_callback (CTK_WIDGET (tree_view), presize_handler_callback, NULL, NULL);
    }
  if (! tree_view->priv->validate_rows_timer)
    {
      tree_view->priv->validate_rows_timer =
	cdk_threads_add_idle_full (CTK_TREE_VIEW_PRIORITY_VALIDATE, (GSourceFunc) validate_rows, tree_view, NULL);
      g_source_set_name_by_id (tree_view->priv->validate_rows_timer, "[ctk+] validate_rows");
    }
}

static gboolean
scroll_sync_handler (CtkTreeView *tree_view)
{
  if (ctk_tree_view_get_height (tree_view) <= ctk_adjustment_get_page_size (tree_view->priv->vadjustment))
    ctk_adjustment_set_value (CTK_ADJUSTMENT (tree_view->priv->vadjustment), 0);
  else if (ctk_tree_row_reference_valid (tree_view->priv->top_row))
    ctk_tree_view_top_row_to_dy (tree_view);
  else
    ctk_tree_view_dy_to_top_row (tree_view);

  tree_view->priv->scroll_sync_timer = 0;

  return FALSE;
}

static void
install_scroll_sync_handler (CtkTreeView *tree_view)
{
  if (!ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    return;

  if (!tree_view->priv->scroll_sync_timer)
    {
      tree_view->priv->scroll_sync_timer =
	cdk_threads_add_idle_full (CTK_TREE_VIEW_PRIORITY_SCROLL_SYNC, (GSourceFunc) scroll_sync_handler, tree_view, NULL);
      g_source_set_name_by_id (tree_view->priv->scroll_sync_timer, "[ctk+] scroll_sync_handler");
    }
}

static void
ctk_tree_view_set_top_row (CtkTreeView *tree_view,
			   CtkTreePath *path,
			   gint         offset)
{
  ctk_tree_row_reference_free (tree_view->priv->top_row);

  if (!path)
    {
      tree_view->priv->top_row = NULL;
      tree_view->priv->top_row_dy = 0;
    }
  else
    {
      tree_view->priv->top_row = ctk_tree_row_reference_new_proxy (G_OBJECT (tree_view), tree_view->priv->model, path);
      tree_view->priv->top_row_dy = offset;
    }
}

/* Always call this iff dy is in the visible range.  If the tree is empty, then
 * it’s set to be NULL, and top_row_dy is 0;
 */
static void
ctk_tree_view_dy_to_top_row (CtkTreeView *tree_view)
{
  gint offset;
  CtkTreePath *path;
  CtkRBTree *tree;
  CtkRBNode *node;

  if (tree_view->priv->tree == NULL)
    {
      ctk_tree_view_set_top_row (tree_view, NULL, 0);
    }
  else
    {
      offset = _ctk_rbtree_find_offset (tree_view->priv->tree,
					tree_view->priv->dy,
					&tree, &node);

      if (tree == NULL)
        {
	  ctk_tree_view_set_top_row (tree_view, NULL, 0);
	}
      else
        {
	  path = _ctk_tree_path_new_from_rbtree (tree, node);
	  ctk_tree_view_set_top_row (tree_view, path, offset);
	  ctk_tree_path_free (path);
	}
    }
}

static void
ctk_tree_view_top_row_to_dy (CtkTreeView *tree_view)
{
  CtkTreePath *path;
  CtkRBTree *tree;
  CtkRBNode *node;
  int new_dy;

  /* Avoid recursive calls */
  if (tree_view->priv->in_top_row_to_dy)
    return;

  if (tree_view->priv->top_row)
    path = ctk_tree_row_reference_get_path (tree_view->priv->top_row);
  else
    path = NULL;

  if (!path)
    tree = NULL;
  else
    _ctk_tree_view_find_node (tree_view, path, &tree, &node);

  if (path)
    ctk_tree_path_free (path);

  if (tree == NULL)
    {
      /* keep dy and set new toprow */
      ctk_tree_row_reference_free (tree_view->priv->top_row);
      tree_view->priv->top_row = NULL;
      tree_view->priv->top_row_dy = 0;
      /* DO NOT install the idle handler */
      ctk_tree_view_dy_to_top_row (tree_view);
      return;
    }

  if (ctk_tree_view_get_row_height (tree_view, node)
      < tree_view->priv->top_row_dy)
    {
      /* new top row -- do NOT install the idle handler */
      ctk_tree_view_dy_to_top_row (tree_view);
      return;
    }

  new_dy = _ctk_rbtree_node_find_offset (tree, node);
  new_dy += tree_view->priv->top_row_dy;

  if (new_dy + ctk_adjustment_get_page_size (tree_view->priv->vadjustment) > ctk_tree_view_get_height (tree_view))
    new_dy = ctk_tree_view_get_height (tree_view) - ctk_adjustment_get_page_size (tree_view->priv->vadjustment);

  new_dy = MAX (0, new_dy);

  tree_view->priv->in_top_row_to_dy = TRUE;
  ctk_adjustment_set_value (tree_view->priv->vadjustment, (gdouble)new_dy);
  tree_view->priv->in_top_row_to_dy = FALSE;
}


void
_ctk_tree_view_install_mark_rows_col_dirty (CtkTreeView *tree_view,
					    gboolean     install_handler)
{
  tree_view->priv->mark_rows_col_dirty = TRUE;

  if (install_handler)
    install_presize_handler (tree_view);
}

/*
 * This function works synchronously (due to the while (validate_rows...)
 * loop).
 *
 * There was a check for column_type != CTK_TREE_VIEW_COLUMN_AUTOSIZE
 * here. You now need to check that yourself.
 */
void
_ctk_tree_view_column_autosize (CtkTreeView *tree_view,
			        CtkTreeViewColumn *column)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (column));

  _ctk_tree_view_column_cell_set_dirty (column, FALSE);

  do_presize_handler (tree_view);
  while (validate_rows (tree_view));

  ctk_widget_queue_resize (CTK_WIDGET (tree_view));
}

/* Drag-and-drop */

static void
set_source_row (CdkDragContext *context,
                CtkTreeModel   *model,
                CtkTreePath    *source_row)
{
  g_object_set_data_full (G_OBJECT (context),
                          I_("ctk-tree-view-source-row"),
                          source_row ? ctk_tree_row_reference_new (model, source_row) : NULL,
                          (GDestroyNotify) (source_row ? ctk_tree_row_reference_free : NULL));
}

static CtkTreePath*
get_source_row (CdkDragContext *context)
{
  CtkTreeRowReference *ref =
    g_object_get_data (G_OBJECT (context), "ctk-tree-view-source-row");

  if (ref)
    return ctk_tree_row_reference_get_path (ref);
  else
    return NULL;
}

typedef struct
{
  CtkTreeRowReference *dest_row;
  guint                path_down_mode   : 1;
  guint                empty_view_drop  : 1;
  guint                drop_append_mode : 1;
}
DestRow;

static void
dest_row_free (gpointer data)
{
  DestRow *dr = (DestRow *)data;

  ctk_tree_row_reference_free (dr->dest_row);
  g_slice_free (DestRow, dr);
}

static void
set_dest_row (CdkDragContext *context,
              CtkTreeModel   *model,
              CtkTreePath    *dest_row,
              gboolean        path_down_mode,
              gboolean        empty_view_drop,
              gboolean        drop_append_mode)
{
  DestRow *dr;

  if (!dest_row)
    {
      g_object_set_data_full (G_OBJECT (context), I_("ctk-tree-view-dest-row"),
                              NULL, NULL);
      return;
    }

  dr = g_slice_new (DestRow);

  dr->dest_row = ctk_tree_row_reference_new (model, dest_row);
  dr->path_down_mode = path_down_mode != FALSE;
  dr->empty_view_drop = empty_view_drop != FALSE;
  dr->drop_append_mode = drop_append_mode != FALSE;

  g_object_set_data_full (G_OBJECT (context), I_("ctk-tree-view-dest-row"),
                          dr, (GDestroyNotify) dest_row_free);
}

static CtkTreePath*
get_dest_row (CdkDragContext *context,
              gboolean       *path_down_mode)
{
  DestRow *dr =
    g_object_get_data (G_OBJECT (context), "ctk-tree-view-dest-row");

  if (dr)
    {
      CtkTreePath *path = NULL;

      if (path_down_mode)
        *path_down_mode = dr->path_down_mode;

      if (dr->dest_row)
        path = ctk_tree_row_reference_get_path (dr->dest_row);
      else if (dr->empty_view_drop)
        path = ctk_tree_path_new_from_indices (0, -1);
      else
        path = NULL;

      if (path && dr->drop_append_mode)
        ctk_tree_path_next (path);

      return path;
    }
  else
    return NULL;
}

/* Get/set whether drag_motion requested the drag data and
 * drag_data_received should thus not actually insert the data,
 * since the data doesn’t result from a drop.
 */
static void
set_status_pending (CdkDragContext *context,
                    CdkDragAction   suggested_action)
{
  g_object_set_data (G_OBJECT (context),
                     I_("ctk-tree-view-status-pending"),
                     GINT_TO_POINTER (suggested_action));
}

static CdkDragAction
get_status_pending (CdkDragContext *context)
{
  return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (context),
                                             "ctk-tree-view-status-pending"));
}

static TreeViewDragInfo*
get_info (CtkTreeView *tree_view)
{
  return g_object_get_data (G_OBJECT (tree_view), "ctk-tree-view-drag-info");
}

static void
destroy_info (TreeViewDragInfo *di)
{
  g_slice_free (TreeViewDragInfo, di);
}

static TreeViewDragInfo*
ensure_info (CtkTreeView *tree_view)
{
  TreeViewDragInfo *di;

  di = get_info (tree_view);

  if (di == NULL)
    {
      di = g_slice_new0 (TreeViewDragInfo);

      g_object_set_data_full (G_OBJECT (tree_view),
                              I_("ctk-tree-view-drag-info"),
                              di,
                              (GDestroyNotify) destroy_info);
    }

  return di;
}

static void
remove_info (CtkTreeView *tree_view)
{
  g_object_set_data (G_OBJECT (tree_view), I_("ctk-tree-view-drag-info"), NULL);
}

#if 0
static gint
drag_scan_timeout (gpointer data)
{
  CtkTreeView *tree_view;
  gint x, y;
  CdkModifierType state;
  CtkTreePath *path = NULL;
  CtkTreeViewColumn *column = NULL;
  CdkRectangle visible_rect;
  CdkSeat *seat;

  cdk_threads_enter ();

  tree_view = CTK_TREE_VIEW (data);

  seat = cdk_display_get_default_seat (ctk_widget_get_display (CTK_WIDGET (tree_view)));
  cdk_window_get_device_position (tree_view->priv->bin_window,
                                  cdk_seat_get_pointer (seat),
                                  &x, &y, &state);

  ctk_tree_view_get_visible_rect (tree_view, &visible_rect);

  /* See if we are near the edge. */
  if ((x - visible_rect.x) < SCROLL_EDGE_SIZE ||
      (visible_rect.x + visible_rect.width - x) < SCROLL_EDGE_SIZE ||
      (y - visible_rect.y) < SCROLL_EDGE_SIZE ||
      (visible_rect.y + visible_rect.height - y) < SCROLL_EDGE_SIZE)
    {
      ctk_tree_view_get_path_at_pos (tree_view,
                                     tree_view->priv->bin_window,
                                     x, y,
                                     &path,
                                     &column,
                                     NULL,
                                     NULL);

      if (path != NULL)
        {
          ctk_tree_view_scroll_to_cell (tree_view,
                                        path,
                                        column,
					TRUE,
                                        0.5, 0.5);

          ctk_tree_path_free (path);
        }
    }

  cdk_threads_leave ();

  return TRUE;
}
#endif /* 0 */

static void
add_scroll_timeout (CtkTreeView *tree_view)
{
  if (tree_view->priv->scroll_timeout == 0)
    {
      tree_view->priv->scroll_timeout =
	cdk_threads_add_timeout (150, scroll_row_timeout, tree_view);
      g_source_set_name_by_id (tree_view->priv->scroll_timeout, "[ctk+] scroll_row_timeout");
    }
}

static void
remove_scroll_timeout (CtkTreeView *tree_view)
{
  if (tree_view->priv->scroll_timeout != 0)
    {
      g_source_remove (tree_view->priv->scroll_timeout);
      tree_view->priv->scroll_timeout = 0;
    }
}

static gboolean
check_model_dnd (CtkTreeModel *model,
                 GType         required_iface,
                 const gchar  *signal)
{
  if (model == NULL || !G_TYPE_CHECK_INSTANCE_TYPE ((model), required_iface))
    {
      g_warning ("You must override the default '%s' handler "
                 "on CtkTreeView when using models that don't support "
                 "the %s interface and enabling drag-and-drop. The simplest way to do this "
                 "is to connect to '%s' and call "
                 "g_signal_stop_emission_by_name() in your signal handler to prevent "
                 "the default handler from running. Look at the source code "
                 "for the default handler in ctktreeview.c to get an idea what "
                 "your handler should do. (ctktreeview.c is in the CTK source "
                 "code.) If you're using CTK from a language other than C, "
                 "there may be a more natural way to override default handlers, e.g. via derivation.",
                 signal, g_type_name (required_iface), signal);
      return FALSE;
    }
  else
    return TRUE;
}

static void
remove_open_timeout (CtkTreeView *tree_view)
{
  if (tree_view->priv->open_dest_timeout != 0)
    {
      g_source_remove (tree_view->priv->open_dest_timeout);
      tree_view->priv->open_dest_timeout = 0;
    }
}


static gint
open_row_timeout (gpointer data)
{
  CtkTreeView *tree_view = data;
  CtkTreePath *dest_path = NULL;
  CtkTreeViewDropPosition pos;
  gboolean result = FALSE;

  ctk_tree_view_get_drag_dest_row (tree_view,
                                   &dest_path,
                                   &pos);

  if (dest_path &&
      (pos == CTK_TREE_VIEW_DROP_INTO_OR_AFTER ||
       pos == CTK_TREE_VIEW_DROP_INTO_OR_BEFORE))
    {
      ctk_tree_view_expand_row (tree_view, dest_path, FALSE);
      tree_view->priv->open_dest_timeout = 0;

      ctk_tree_path_free (dest_path);
    }
  else
    {
      if (dest_path)
        ctk_tree_path_free (dest_path);

      result = TRUE;
    }

  return result;
}

static gboolean
scroll_row_timeout (gpointer data)
{
  CtkTreeView *tree_view = data;

  ctk_tree_view_vertical_autoscroll (tree_view);

  if (tree_view->priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    ctk_tree_view_update_rubber_band (tree_view);

  return TRUE;
}

/* Returns TRUE if event should not be propagated to parent widgets */
static gboolean
set_destination_row (CtkTreeView    *tree_view,
                     CdkDragContext *context,
                     /* coordinates relative to the widget */
                     gint            x,
                     gint            y,
                     CdkDragAction  *suggested_action,
                     CdkAtom        *target)
{
  CtkTreePath *path = NULL;
  CtkTreeViewDropPosition pos;
  CtkTreeViewDropPosition old_pos;
  TreeViewDragInfo *di;
  CtkWidget *widget;
  CtkTreePath *old_dest_path = NULL;
  gboolean can_drop = FALSE;

  *suggested_action = 0;
  *target = CDK_NONE;

  widget = CTK_WIDGET (tree_view);

  di = get_info (tree_view);

  if (di == NULL || y - ctk_tree_view_get_effective_header_height (tree_view) < 0)
    {
      /* someone unset us as a drag dest, note that if
       * we return FALSE drag_leave isn't called
       */

      ctk_tree_view_set_drag_dest_row (tree_view,
                                       NULL,
                                       CTK_TREE_VIEW_DROP_BEFORE);

      remove_scroll_timeout (CTK_TREE_VIEW (widget));
      remove_open_timeout (CTK_TREE_VIEW (widget));

      return FALSE; /* no longer a drop site */
    }

  *target = ctk_drag_dest_find_target (widget, context,
                                       ctk_drag_dest_get_target_list (widget));
  if (*target == CDK_NONE)
    {
      return FALSE;
    }

  if (!ctk_tree_view_get_dest_row_at_pos (tree_view,
                                          x, y,
                                          &path,
                                          &pos))
    {
      gint n_children;
      CtkTreeModel *model;

      remove_open_timeout (tree_view);

      /* the row got dropped on empty space, let's setup a special case
       */

      if (path)
	ctk_tree_path_free (path);

      model = ctk_tree_view_get_model (tree_view);

      n_children = ctk_tree_model_iter_n_children (model, NULL);
      if (n_children)
        {
          pos = CTK_TREE_VIEW_DROP_AFTER;
          path = ctk_tree_path_new_from_indices (n_children - 1, -1);
        }
      else
        {
          pos = CTK_TREE_VIEW_DROP_BEFORE;
          path = ctk_tree_path_new_from_indices (0, -1);
        }

      can_drop = TRUE;

      goto out;
    }

  g_assert (path);

  /* If we left the current row's "open" zone, unset the timeout for
   * opening the row
   */
  ctk_tree_view_get_drag_dest_row (tree_view,
                                   &old_dest_path,
                                   &old_pos);

  if (old_dest_path &&
      (ctk_tree_path_compare (path, old_dest_path) != 0 ||
       !(pos == CTK_TREE_VIEW_DROP_INTO_OR_AFTER ||
         pos == CTK_TREE_VIEW_DROP_INTO_OR_BEFORE)))
    remove_open_timeout (tree_view);

  if (old_dest_path)
    ctk_tree_path_free (old_dest_path);

  if (TRUE /* FIXME if the location droppable predicate */)
    {
      can_drop = TRUE;
    }

out:
  if (can_drop)
    {
      CtkWidget *source_widget;

      *suggested_action = cdk_drag_context_get_suggested_action (context);
      source_widget = ctk_drag_get_source_widget (context);

      if (source_widget == widget)
        {
          /* Default to MOVE, unless the user has
           * pressed ctrl or shift to affect available actions
           */
          if ((cdk_drag_context_get_actions (context) & CDK_ACTION_MOVE) != 0)
            *suggested_action = CDK_ACTION_MOVE;
        }

      ctk_tree_view_set_drag_dest_row (CTK_TREE_VIEW (widget),
                                       path, pos);
    }
  else
    {
      /* can't drop here */
      remove_open_timeout (tree_view);

      ctk_tree_view_set_drag_dest_row (CTK_TREE_VIEW (widget),
                                       NULL,
                                       CTK_TREE_VIEW_DROP_BEFORE);
    }

  if (path)
    ctk_tree_path_free (path);

  return TRUE;
}

static CtkTreePath*
get_logical_dest_row (CtkTreeView *tree_view,
                      gboolean    *path_down_mode,
                      gboolean    *drop_append_mode)
{
  /* adjust path to point to the row the drop goes in front of */
  CtkTreePath *path = NULL;
  CtkTreeViewDropPosition pos;

  g_return_val_if_fail (path_down_mode != NULL, NULL);
  g_return_val_if_fail (drop_append_mode != NULL, NULL);

  *path_down_mode = FALSE;
  *drop_append_mode = 0;

  ctk_tree_view_get_drag_dest_row (tree_view, &path, &pos);

  if (path == NULL)
    return NULL;

  if (pos == CTK_TREE_VIEW_DROP_BEFORE)
    ; /* do nothing */
  else if (pos == CTK_TREE_VIEW_DROP_INTO_OR_BEFORE ||
           pos == CTK_TREE_VIEW_DROP_INTO_OR_AFTER)
    *path_down_mode = TRUE;
  else
    {
      CtkTreeIter iter;
      CtkTreeModel *model = ctk_tree_view_get_model (tree_view);

      g_assert (pos == CTK_TREE_VIEW_DROP_AFTER);

      if (!ctk_tree_model_get_iter (model, &iter, path) ||
          !ctk_tree_model_iter_next (model, &iter))
        *drop_append_mode = 1;
      else
        {
          *drop_append_mode = 0;
          ctk_tree_path_next (path);
        }
    }

  return path;
}

static gboolean
ctk_tree_view_maybe_begin_dragging_row (CtkTreeView *tree_view)
{
  CtkWidget *widget = CTK_WIDGET (tree_view);
  gdouble start_x, start_y, offset_x, offset_y;
  CdkEventSequence *sequence;
  const CdkEvent *event;
  CdkDragContext *context;
  TreeViewDragInfo *di;
  CtkTreePath *path = NULL;
  gint button;
  CtkTreeModel *model;
  gboolean retval = FALSE;
  gint bin_x, bin_y;

  di = get_info (tree_view);

  if (di == NULL || !di->source_set)
    goto out;

  if (!ctk_gesture_is_recognized (tree_view->priv->drag_gesture))
    goto out;

  ctk_gesture_drag_get_start_point (CTK_GESTURE_DRAG (tree_view->priv->drag_gesture),
                                    &start_x, &start_y);
  ctk_gesture_drag_get_offset (CTK_GESTURE_DRAG (tree_view->priv->drag_gesture),
                               &offset_x, &offset_y);

  if (!ctk_drag_check_threshold (widget, 0, 0, offset_x, offset_y))
    goto out;

  model = ctk_tree_view_get_model (tree_view);

  if (model == NULL)
    goto out;

  button = ctk_gesture_single_get_current_button (CTK_GESTURE_SINGLE (tree_view->priv->drag_gesture));

  /* Deny the multipress gesture */
  ctk_gesture_set_state (CTK_GESTURE (tree_view->priv->multipress_gesture),
                         CTK_EVENT_SEQUENCE_DENIED);

  ctk_tree_view_convert_widget_to_bin_window_coords (tree_view, start_x, start_y,
                                                     &bin_x, &bin_y);
  ctk_tree_view_get_path_at_pos (tree_view, bin_x, bin_y, &path,
                                 NULL, NULL, NULL);

  if (path == NULL)
    goto out;

  if (!CTK_IS_TREE_DRAG_SOURCE (model) ||
      !ctk_tree_drag_source_row_draggable (CTK_TREE_DRAG_SOURCE (model),
					   path))
    goto out;

  if (!(CDK_BUTTON1_MASK << (button - 1) & di->start_button_mask))
    goto out;

  retval = TRUE;

  /* Now we can begin the drag */
  ctk_gesture_set_state (CTK_GESTURE (tree_view->priv->drag_gesture),
                         CTK_EVENT_SEQUENCE_CLAIMED);
  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (tree_view->priv->drag_gesture));
  event = ctk_gesture_get_last_event (CTK_GESTURE (tree_view->priv->drag_gesture), sequence);

  context = ctk_drag_begin_with_coordinates (widget,
                                             ctk_drag_source_get_target_list (widget),
                                             di->source_actions,
                                             button,
                                             (CdkEvent*)event,
                                             start_x, start_y);

  set_source_row (context, model, path);

 out:
  if (path)
    ctk_tree_path_free (path);

  return retval;
}


static void
ctk_tree_view_drag_begin (CtkWidget      *widget,
                          CdkDragContext *context)
{
  CtkTreeView *tree_view;
  CtkTreePath *path = NULL;
  gint cell_x, cell_y;
  cairo_surface_t *row_pix;
  TreeViewDragInfo *di;
  double sx, sy;

  tree_view = CTK_TREE_VIEW (widget);

  /* if the user uses a custom DND source impl, we don't set the icon here */
  di = get_info (tree_view);

  if (di == NULL || !di->source_set)
    return;

  ctk_tree_view_get_path_at_pos (tree_view,
                                 tree_view->priv->press_start_x,
                                 tree_view->priv->press_start_y,
                                 &path,
                                 NULL,
                                 &cell_x,
                                 &cell_y);

  /* If path is NULL, there's nothing we can drag.  For now, we silently
   * bail out.  Actually, dragging should not be possible from an empty
   * tree view, but there's no way we can cancel that from here.
   * Automatically unsetting the tree view as drag source for empty models
   * is something that would likely break other people's code ...
   */
  if (!path)
    return;

  row_pix = ctk_tree_view_create_row_drag_icon (tree_view,
                                                path);
  cairo_surface_get_device_scale (row_pix, &sx, &sy);
  cairo_surface_set_device_offset (row_pix,
                                   /* the + 1 is for the black border in the icon */
                                   - (tree_view->priv->press_start_x + 1) * sx,
                                   - (cell_y + 1) * sy);

  ctk_drag_set_icon_surface (context, row_pix);

  cairo_surface_destroy (row_pix);
  ctk_tree_path_free (path);
}

static void
ctk_tree_view_drag_end (CtkWidget      *widget,
                        CdkDragContext *context G_GNUC_UNUSED)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);

  tree_view->priv->event_last_x = -10000;
  tree_view->priv->event_last_y = -10000;
}

/* Default signal implementations for the drag signals */
static void
ctk_tree_view_drag_data_get (CtkWidget        *widget,
                             CdkDragContext   *context,
                             CtkSelectionData *selection_data,
                             guint             info G_GNUC_UNUSED,
                             guint             time G_GNUC_UNUSED)
{
  CtkTreeView *tree_view;
  CtkTreeModel *model;
  TreeViewDragInfo *di;
  CtkTreePath *source_row;

  tree_view = CTK_TREE_VIEW (widget);

  model = ctk_tree_view_get_model (tree_view);

  if (model == NULL)
    return;

  di = get_info (CTK_TREE_VIEW (widget));

  if (di == NULL)
    return;

  source_row = get_source_row (context);

  if (source_row == NULL)
    return;

  /* We can implement the CTK_TREE_MODEL_ROW target generically for
   * any model; for DragSource models there are some other targets
   * we also support.
   */

  if (CTK_IS_TREE_DRAG_SOURCE (model) &&
      ctk_tree_drag_source_drag_data_get (CTK_TREE_DRAG_SOURCE (model),
                                          source_row,
                                          selection_data))
    goto done;

  /* If drag_data_get does nothing, try providing row data. */
  if (ctk_selection_data_get_target (selection_data) == cdk_atom_intern_static_string ("CTK_TREE_MODEL_ROW"))
    {
      ctk_tree_set_row_drag_data (selection_data,
				  model,
				  source_row);
    }

 done:
  ctk_tree_path_free (source_row);
}


static void
ctk_tree_view_drag_data_delete (CtkWidget      *widget,
                                CdkDragContext *context)
{
  TreeViewDragInfo *di;
  CtkTreeModel *model;
  CtkTreeView *tree_view;
  CtkTreePath *source_row;

  tree_view = CTK_TREE_VIEW (widget);
  model = ctk_tree_view_get_model (tree_view);

  if (!check_model_dnd (model, CTK_TYPE_TREE_DRAG_SOURCE, "drag_data_delete"))
    return;

  di = get_info (tree_view);

  if (di == NULL)
    return;

  source_row = get_source_row (context);

  if (source_row == NULL)
    return;

  ctk_tree_drag_source_drag_data_delete (CTK_TREE_DRAG_SOURCE (model),
                                         source_row);

  ctk_tree_path_free (source_row);

  set_source_row (context, NULL, NULL);
}

static void
ctk_tree_view_drag_leave (CtkWidget      *widget,
                          CdkDragContext *context G_GNUC_UNUSED,
                          guint           time G_GNUC_UNUSED)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);

  /* unset any highlight row */
  ctk_tree_view_set_drag_dest_row (CTK_TREE_VIEW (widget),
                                   NULL,
                                   CTK_TREE_VIEW_DROP_BEFORE);

  remove_scroll_timeout (CTK_TREE_VIEW (widget));
  remove_open_timeout (CTK_TREE_VIEW (widget));

  tree_view->priv->event_last_x = -10000;
  tree_view->priv->event_last_y = -10000;
}


static gboolean
ctk_tree_view_drag_motion (CtkWidget        *widget,
                           CdkDragContext   *context,
			   /* coordinates relative to the widget */
                           gint              x,
                           gint              y,
                           guint             time)
{
  gboolean empty;
  CtkTreePath *path = NULL;
  CtkTreeViewDropPosition pos;
  CtkTreeView *tree_view;
  CdkDragAction suggested_action = 0;
  CdkAtom target;

  tree_view = CTK_TREE_VIEW (widget);

  if (!set_destination_row (tree_view, context, x, y, &suggested_action, &target))
    return FALSE;

  tree_view->priv->event_last_x = x;
  tree_view->priv->event_last_y = y;

  ctk_tree_view_get_drag_dest_row (tree_view, &path, &pos);

  /* we only know this *after* set_desination_row */
  empty = tree_view->priv->empty_view_drop;

  if (path == NULL && !empty)
    {
      /* Can't drop here. */
      cdk_drag_status (context, 0, time);
    }
  else
    {
      if (tree_view->priv->open_dest_timeout == 0 &&
          (pos == CTK_TREE_VIEW_DROP_INTO_OR_AFTER ||
           pos == CTK_TREE_VIEW_DROP_INTO_OR_BEFORE))
        {
          tree_view->priv->open_dest_timeout =
            cdk_threads_add_timeout (AUTO_EXPAND_TIMEOUT, open_row_timeout, tree_view);
          g_source_set_name_by_id (tree_view->priv->open_dest_timeout, "[ctk+] open_row_timeout");
        }
      else
        {
	  add_scroll_timeout (tree_view);
	}

      if (target == cdk_atom_intern_static_string ("CTK_TREE_MODEL_ROW"))
        {
          /* Request data so we can use the source row when
           * determining whether to accept the drop
           */
          set_status_pending (context, suggested_action);
          ctk_drag_get_data (widget, context, target, time);
        }
      else
        {
          set_status_pending (context, 0);
          cdk_drag_status (context, suggested_action, time);
        }
    }

  if (path)
    ctk_tree_path_free (path);

  return TRUE;
}


static gboolean
ctk_tree_view_drag_drop (CtkWidget        *widget,
                         CdkDragContext   *context,
			 /* coordinates relative to the widget */
                         gint              x,
                         gint              y,
                         guint             time)
{
  CtkTreeView *tree_view;
  CtkTreePath *path;
  CdkDragAction suggested_action = 0;
  CdkAtom target = CDK_NONE;
  TreeViewDragInfo *di;
  CtkTreeModel *model;
  gboolean path_down_mode;
  gboolean drop_append_mode;

  tree_view = CTK_TREE_VIEW (widget);

  model = ctk_tree_view_get_model (tree_view);

  remove_scroll_timeout (CTK_TREE_VIEW (widget));
  remove_open_timeout (CTK_TREE_VIEW (widget));

  di = get_info (tree_view);

  if (di == NULL)
    return FALSE;

  if (!check_model_dnd (model, CTK_TYPE_TREE_DRAG_DEST, "drag_drop"))
    return FALSE;

  if (!set_destination_row (tree_view, context, x, y, &suggested_action, &target))
    return FALSE;

  path = get_logical_dest_row (tree_view, &path_down_mode, &drop_append_mode);

  if (target != CDK_NONE && path != NULL)
    {
      /* in case a motion had requested drag data, change things so we
       * treat drag data receives as a drop.
       */
      set_status_pending (context, 0);
      set_dest_row (context, model, path,
                    path_down_mode, tree_view->priv->empty_view_drop,
                    drop_append_mode);
    }

  if (path)
    ctk_tree_path_free (path);

  /* Unset this thing */
  ctk_tree_view_set_drag_dest_row (CTK_TREE_VIEW (widget),
                                   NULL,
                                   CTK_TREE_VIEW_DROP_BEFORE);

  if (target != CDK_NONE)
    {
      ctk_drag_get_data (widget, context, target, time);
      return TRUE;
    }
  else
    return FALSE;
}

static void
ctk_tree_view_drag_data_received (CtkWidget        *widget,
                                  CdkDragContext   *context,
				  /* coordinates relative to the widget */
                                  gint              x G_GNUC_UNUSED,
                                  gint              y G_GNUC_UNUSED,
                                  CtkSelectionData *selection_data,
                                  guint             info G_GNUC_UNUSED,
                                  guint             time)
{
  CtkTreePath *path;
  TreeViewDragInfo *di;
  gboolean accepted = FALSE;
  CtkTreeModel *model;
  CtkTreeView *tree_view;
  CtkTreePath *dest_row;
  CdkDragAction suggested_action;
  gboolean path_down_mode;
  gboolean drop_append_mode;

  tree_view = CTK_TREE_VIEW (widget);

  model = ctk_tree_view_get_model (tree_view);

  if (!check_model_dnd (model, CTK_TYPE_TREE_DRAG_DEST, "drag_data_received"))
    return;

  di = get_info (tree_view);

  if (di == NULL)
    return;

  suggested_action = get_status_pending (context);

  if (suggested_action)
    {
      /* We are getting this data due to a request in drag_motion,
       * rather than due to a request in drag_drop, so we are just
       * supposed to call drag_status, not actually paste in the
       * data.
       */
      path = get_logical_dest_row (tree_view, &path_down_mode,
                                   &drop_append_mode);

      if (path == NULL)
        suggested_action = 0;
      else if (path_down_mode)
        ctk_tree_path_down (path);

      if (suggested_action)
        {
	  if (!ctk_tree_drag_dest_row_drop_possible (CTK_TREE_DRAG_DEST (model),
						     path,
						     selection_data))
            {
              if (path_down_mode)
                {
                  path_down_mode = FALSE;
                  ctk_tree_path_up (path);

                  if (!ctk_tree_drag_dest_row_drop_possible (CTK_TREE_DRAG_DEST (model),
                                                             path,
                                                             selection_data))
                    suggested_action = 0;
                }
              else
	        suggested_action = 0;
            }
        }

      cdk_drag_status (context, suggested_action, time);

      if (path)
        ctk_tree_path_free (path);

      /* If you can't drop, remove user drop indicator until the next motion */
      if (suggested_action == 0)
        ctk_tree_view_set_drag_dest_row (CTK_TREE_VIEW (widget),
                                         NULL,
                                         CTK_TREE_VIEW_DROP_BEFORE);

      return;
    }

  dest_row = get_dest_row (context, &path_down_mode);

  if (dest_row == NULL)
    return;

  if (ctk_selection_data_get_length (selection_data) >= 0)
    {
      if (path_down_mode)
        {
          ctk_tree_path_down (dest_row);
          if (!ctk_tree_drag_dest_row_drop_possible (CTK_TREE_DRAG_DEST (model),
                                                     dest_row, selection_data))
            ctk_tree_path_up (dest_row);
        }
    }

  if (ctk_selection_data_get_length (selection_data) >= 0)
    {
      if (ctk_tree_drag_dest_drag_data_received (CTK_TREE_DRAG_DEST (model),
                                                 dest_row,
                                                 selection_data))
        accepted = TRUE;
    }

  ctk_drag_finish (context,
                   accepted,
                   (cdk_drag_context_get_selected_action (context) == CDK_ACTION_MOVE),
                   time);

  if (ctk_tree_path_get_depth (dest_row) == 1 &&
      ctk_tree_path_get_indices (dest_row)[0] == 0 &&
      ctk_tree_model_iter_n_children (tree_view->priv->model, NULL) != 0)
    {
      /* special special case drag to "0", scroll to first item */
      if (!tree_view->priv->scroll_to_path)
        ctk_tree_view_scroll_to_cell (tree_view, dest_row, NULL, FALSE, 0.0, 0.0);
    }

  ctk_tree_path_free (dest_row);

  /* drop dest_row */
  set_dest_row (context, NULL, NULL, FALSE, FALSE, FALSE);
}



/* CtkContainer Methods
 */


static void
ctk_tree_view_remove (CtkContainer *container,
		      CtkWidget    *widget)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (container);
  CtkTreeViewChild *child = NULL;
  GList *tmp_list;

  tmp_list = tree_view->priv->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      if (child->widget == widget)
	{
	  ctk_widget_unparent (widget);

	  tree_view->priv->children = g_list_remove_link (tree_view->priv->children, tmp_list);
	  g_list_free_1 (tmp_list);
	  g_slice_free (CtkTreeViewChild, child);
	  return;
	}

      tmp_list = tmp_list->next;
    }

  tmp_list = tree_view->priv->columns;

  while (tmp_list)
    {
      CtkTreeViewColumn *column;
      CtkWidget         *button;

      column = tmp_list->data;
      button = ctk_tree_view_column_get_button (column);

      if (button == widget)
	{
	  ctk_widget_unparent (widget);
	  return;
	}
      tmp_list = tmp_list->next;
    }
}

static void
ctk_tree_view_forall (CtkContainer *container,
		      gboolean      include_internals,
		      CtkCallback   callback,
		      gpointer      callback_data)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (container);
  CtkTreeViewChild *child = NULL;
  CtkTreeViewColumn *column;
  CtkWidget *button;
  GList *tmp_list;

  tmp_list = tree_view->priv->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      tmp_list = tmp_list->next;

      (* callback) (child->widget, callback_data);
    }
  if (include_internals == FALSE)
    return;

  for (tmp_list = tree_view->priv->columns; tmp_list; tmp_list = tmp_list->next)
    {
      column = tmp_list->data;
      button = ctk_tree_view_column_get_button (column);

      if (button)
	(* callback) (button, callback_data);
    }
}

/* Returns TRUE is any of the columns contains a cell that can-focus.
 * If this is not the case, a column-spanning focus rectangle will be
 * drawn.
 */
static gboolean
ctk_tree_view_has_can_focus_cell (CtkTreeView *tree_view)
{
  GList *list;

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      CtkTreeViewColumn *column = list->data;

      if (!ctk_tree_view_column_get_visible (column))
	continue;
      if (ctk_cell_area_is_activatable (ctk_cell_layout_get_area (CTK_CELL_LAYOUT (column))))
	return TRUE;
    }

  return FALSE;
}

static void
column_sizing_notify (GObject    *object,
                      GParamSpec *pspec G_GNUC_UNUSED,
                      gpointer    data)
{
  CtkTreeViewColumn *c = CTK_TREE_VIEW_COLUMN (object);

  if (ctk_tree_view_column_get_sizing (c) != CTK_TREE_VIEW_COLUMN_FIXED)
    /* disable fixed height mode */
    g_object_set (data, "fixed-height-mode", FALSE, NULL);
}

/**
 * ctk_tree_view_set_fixed_height_mode:
 * @tree_view: a #CtkTreeView 
 * @enable: %TRUE to enable fixed height mode
 * 
 * Enables or disables the fixed height mode of @tree_view. 
 * Fixed height mode speeds up #CtkTreeView by assuming that all 
 * rows have the same height. 
 * Only enable this option if all rows are the same height and all
 * columns are of type %CTK_TREE_VIEW_COLUMN_FIXED.
 *
 * Since: 2.6 
 **/
void
ctk_tree_view_set_fixed_height_mode (CtkTreeView *tree_view,
                                     gboolean     enable)
{
  GList *l;
  
  enable = enable != FALSE;

  if (enable == tree_view->priv->fixed_height_mode)
    return;

  if (!enable)
    {
      tree_view->priv->fixed_height_mode = 0;
      tree_view->priv->fixed_height = -1;
    }
  else 
    {
      /* make sure all columns are of type FIXED */
      for (l = tree_view->priv->columns; l; l = l->next)
	{
	  CtkTreeViewColumn *c = l->data;
	  
	  g_return_if_fail (ctk_tree_view_column_get_sizing (c) == CTK_TREE_VIEW_COLUMN_FIXED);
	}
      
      /* yes, we really have to do this is in a separate loop */
      for (l = tree_view->priv->columns; l; l = l->next)
	g_signal_connect (l->data, "notify::sizing",
			  G_CALLBACK (column_sizing_notify), tree_view);
      
      tree_view->priv->fixed_height_mode = 1;
      tree_view->priv->fixed_height = -1;
    }

  /* force a revalidation */
  install_presize_handler (tree_view);

  g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_FIXED_HEIGHT_MODE]);
}

/**
 * ctk_tree_view_get_fixed_height_mode:
 * @tree_view: a #CtkTreeView
 * 
 * Returns whether fixed height mode is turned on for @tree_view.
 * 
 * Returns: %TRUE if @tree_view is in fixed height mode
 * 
 * Since: 2.6
 **/
gboolean
ctk_tree_view_get_fixed_height_mode (CtkTreeView *tree_view)
{
  return tree_view->priv->fixed_height_mode;
}

/* Returns TRUE if the focus is within the headers, after the focus operation is
 * done
 */
static gboolean
ctk_tree_view_header_focus (CtkTreeView      *tree_view,
			    CtkDirectionType  dir,
			    gboolean          clamp_column_visible)
{
  CtkTreeViewColumn *column;
  CtkWidget *button;
  CtkWidget *focus_child;
  GList *last_column, *first_column;
  GList *tmp_list;
  gboolean rtl;

  if (! tree_view->priv->headers_visible)
    return FALSE;

  focus_child = ctk_container_get_focus_child (CTK_CONTAINER (tree_view));

  first_column = tree_view->priv->columns;
  while (first_column)
    {
      column = CTK_TREE_VIEW_COLUMN (first_column->data);
      button = ctk_tree_view_column_get_button (column);

      if (ctk_widget_get_can_focus (button) &&
          ctk_tree_view_column_get_visible (column) &&
          (ctk_tree_view_column_get_clickable (column) ||
           ctk_tree_view_column_get_reorderable (column)))
	break;
      first_column = first_column->next;
    }

  /* No headers are visible, or are focusable.  We can't focus in or out.
   */
  if (first_column == NULL)
    return FALSE;

  last_column = g_list_last (tree_view->priv->columns);
  while (last_column)
    {
      column = CTK_TREE_VIEW_COLUMN (last_column->data);
      button = ctk_tree_view_column_get_button (column);

      if (ctk_widget_get_can_focus (button) &&
          ctk_tree_view_column_get_visible (column) &&
          (ctk_tree_view_column_get_clickable (column) ||
           ctk_tree_view_column_get_reorderable (column)))
	break;
      last_column = last_column->prev;
    }


  rtl = (ctk_widget_get_direction (CTK_WIDGET (tree_view)) == CTK_TEXT_DIR_RTL);

  switch (dir)
    {
    case CTK_DIR_TAB_BACKWARD:
    case CTK_DIR_TAB_FORWARD:
    case CTK_DIR_UP:
    case CTK_DIR_DOWN:
      if (focus_child == NULL)
	{
	  if (tree_view->priv->focus_column != NULL)
	    button = ctk_tree_view_column_get_button (tree_view->priv->focus_column);
	  else 
	    button = NULL;

	  if (button && ctk_widget_get_can_focus (button))
	    focus_child = button;
	  else
	    focus_child = ctk_tree_view_column_get_button (CTK_TREE_VIEW_COLUMN (first_column->data));

	  ctk_widget_grab_focus (focus_child);
	  break;
	}
      return FALSE;

    case CTK_DIR_LEFT:
    case CTK_DIR_RIGHT:
      if (focus_child == NULL)
	{
	  if (tree_view->priv->focus_column != NULL)
	    focus_child = ctk_tree_view_column_get_button (tree_view->priv->focus_column);
	  else if (dir == CTK_DIR_LEFT)
	    focus_child = ctk_tree_view_column_get_button (CTK_TREE_VIEW_COLUMN (last_column->data));
	  else
	    focus_child = ctk_tree_view_column_get_button (CTK_TREE_VIEW_COLUMN (first_column->data));

	  ctk_widget_grab_focus (focus_child);
	  break;
	}

      if (ctk_widget_child_focus (focus_child, dir))
	{
	  /* The focus moves inside the button. */
	  /* This is probably a great example of bad UI */
	  break;
	}

      /* We need to move the focus among the row of buttons. */
      for (tmp_list = tree_view->priv->columns; tmp_list; tmp_list = tmp_list->next)
	if (ctk_tree_view_column_get_button (CTK_TREE_VIEW_COLUMN (tmp_list->data)) == focus_child)
	  break;

      if ((tmp_list == first_column && dir == (rtl ? CTK_DIR_RIGHT : CTK_DIR_LEFT))
	  || (tmp_list == last_column && dir == (rtl ? CTK_DIR_LEFT : CTK_DIR_RIGHT)))
        {
	  ctk_widget_error_bell (CTK_WIDGET (tree_view));
	  break;
	}

      while (tmp_list)
	{
	  if (dir == (rtl ? CTK_DIR_LEFT : CTK_DIR_RIGHT))
	    tmp_list = tmp_list->next;
	  else
	    tmp_list = tmp_list->prev;

	  if (tmp_list == NULL)
	    {
	      g_warning ("Internal button not found");
	      break;
	    }
	  column = tmp_list->data;
	  button = ctk_tree_view_column_get_button (column);
	  if (button &&
	      ctk_tree_view_column_get_visible (column) &&
	      ctk_widget_get_can_focus (button))
	    {
	      focus_child = button;
	      ctk_widget_grab_focus (button);
	      break;
	    }
	}
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  /* if focus child is non-null, we assume it's been set to the current focus child
   */
  if (focus_child)
    {
      for (tmp_list = tree_view->priv->columns; tmp_list; tmp_list = tmp_list->next)
	if (ctk_tree_view_column_get_button (CTK_TREE_VIEW_COLUMN (tmp_list->data)) == focus_child)
	  {
            _ctk_tree_view_set_focus_column (tree_view, CTK_TREE_VIEW_COLUMN (tmp_list->data));
	    break;
	  }

      if (clamp_column_visible)
        {
	  ctk_tree_view_clamp_column_visible (tree_view,
					      tree_view->priv->focus_column,
					      FALSE);
	}
    }

  return (focus_child != NULL);
}

/* This function returns in 'path' the first focusable path, if the given path
 * is already focusable, it’s the returned one.
 */
static gboolean
search_first_focusable_path (CtkTreeView  *tree_view,
			     CtkTreePath **path,
			     gboolean      search_forward,
			     CtkRBTree   **new_tree,
			     CtkRBNode   **new_node)
{
  CtkRBTree *tree = NULL;
  CtkRBNode *node = NULL;

  if (!path || !*path)
    return FALSE;

  _ctk_tree_view_find_node (tree_view, *path, &tree, &node);

  if (!tree || !node)
    return FALSE;

  while (node && row_is_separator (tree_view, NULL, *path))
    {
      if (search_forward)
	_ctk_rbtree_next_full (tree, node, &tree, &node);
      else
	_ctk_rbtree_prev_full (tree, node, &tree, &node);

      if (*path)
	ctk_tree_path_free (*path);

      if (node)
	*path = _ctk_tree_path_new_from_rbtree (tree, node);
      else
	*path = NULL;
    }

  if (new_tree)
    *new_tree = tree;

  if (new_node)
    *new_node = node;

  return (*path != NULL);
}

static gint
ctk_tree_view_focus (CtkWidget        *widget,
		     CtkDirectionType  direction)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  CtkContainer *container = CTK_CONTAINER (widget);
  CtkWidget *focus_child;

  if (!ctk_widget_is_sensitive (widget) || !ctk_widget_get_can_focus (widget))
    return FALSE;

  focus_child = ctk_container_get_focus_child (container);

  ctk_tree_view_stop_editing (CTK_TREE_VIEW (widget), FALSE);
  /* Case 1.  Headers currently have focus. */
  if (focus_child)
    {
      switch (direction)
	{
	case CTK_DIR_LEFT:
	case CTK_DIR_RIGHT:
	  ctk_tree_view_header_focus (tree_view, direction, TRUE);
	  return TRUE;
	case CTK_DIR_TAB_BACKWARD:
	case CTK_DIR_UP:
	  return FALSE;
	case CTK_DIR_TAB_FORWARD:
	case CTK_DIR_DOWN:
	  ctk_widget_grab_focus (widget);
	  return TRUE;
	default:
	  g_assert_not_reached ();
	  return FALSE;
	}
    }

  /* Case 2. We don't have focus at all. */
  if (!ctk_widget_has_focus (widget))
    {
      ctk_widget_grab_focus (widget);
      return TRUE;
    }

  /* Case 3. We have focus already. */
  if (direction == CTK_DIR_TAB_BACKWARD)
    return (ctk_tree_view_header_focus (tree_view, direction, FALSE));
  else if (direction == CTK_DIR_TAB_FORWARD)
    return FALSE;

  /* Other directions caught by the keybindings */
  ctk_widget_grab_focus (widget);
  return TRUE;
}

static void
ctk_tree_view_grab_focus (CtkWidget *widget)
{
  CTK_WIDGET_CLASS (ctk_tree_view_parent_class)->grab_focus (widget);

  ctk_tree_view_focus_to_cursor (CTK_TREE_VIEW (widget));
}

static void
ctk_tree_view_style_updated (CtkWidget *widget)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  GList *list;
  CtkTreeViewColumn *column;
  CtkStyleContext *style_context;
  CtkCssStyleChange *change;

  CTK_WIDGET_CLASS (ctk_tree_view_parent_class)->style_updated (widget);

  if (ctk_widget_get_realized (widget))
    {
      ctk_tree_view_set_grid_lines (tree_view, tree_view->priv->grid_lines);
      ctk_tree_view_set_enable_tree_lines (tree_view, tree_view->priv->tree_lines_enabled);
    }

  style_context = ctk_widget_get_style_context (widget);
  change = ctk_style_context_get_change (style_context);

  if (change == NULL || ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_SIZE | CTK_CSS_AFFECTS_CLIP))
    {
      for (list = tree_view->priv->columns; list; list = list->next)
	{
	  column = list->data;
	  _ctk_tree_view_column_cell_set_dirty (column, TRUE);
	}

      tree_view->priv->fixed_height = -1;
      _ctk_rbtree_mark_invalid (tree_view->priv->tree);
    }
}


static void
ctk_tree_view_set_focus_child (CtkContainer *container,
			       CtkWidget    *child)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (container);
  GList *list;

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      if (ctk_tree_view_column_get_button (CTK_TREE_VIEW_COLUMN (list->data)) == child)
	{
          _ctk_tree_view_set_focus_column (tree_view, CTK_TREE_VIEW_COLUMN (list->data));
	  break;
	}
    }

  CTK_CONTAINER_CLASS (ctk_tree_view_parent_class)->set_focus_child (container, child);
}

static gboolean
ctk_tree_view_real_move_cursor (CtkTreeView       *tree_view,
				CtkMovementStep    step,
				gint               count)
{
  CdkModifierType state;

  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);
  g_return_val_if_fail (step == CTK_MOVEMENT_LOGICAL_POSITIONS ||
			step == CTK_MOVEMENT_VISUAL_POSITIONS ||
			step == CTK_MOVEMENT_DISPLAY_LINES ||
			step == CTK_MOVEMENT_PAGES ||
			step == CTK_MOVEMENT_BUFFER_ENDS, FALSE);

  if (tree_view->priv->tree == NULL)
    return FALSE;
  if (!ctk_widget_has_focus (CTK_WIDGET (tree_view)))
    return FALSE;

  ctk_tree_view_stop_editing (tree_view, FALSE);
  tree_view->priv->draw_keyfocus = TRUE;
  ctk_widget_grab_focus (CTK_WIDGET (tree_view));

  if (ctk_get_current_event_state (&state))
    {
      CdkModifierType extend_mod_mask;
      CdkModifierType modify_mod_mask;

      extend_mod_mask =
        ctk_widget_get_modifier_mask (CTK_WIDGET (tree_view),
                                      CDK_MODIFIER_INTENT_EXTEND_SELECTION);

      modify_mod_mask =
        ctk_widget_get_modifier_mask (CTK_WIDGET (tree_view),
                                      CDK_MODIFIER_INTENT_MODIFY_SELECTION);

      if ((state & modify_mod_mask) == modify_mod_mask)
        tree_view->priv->modify_selection_pressed = TRUE;
      if ((state & extend_mod_mask) == extend_mod_mask)
        tree_view->priv->extend_selection_pressed = TRUE;
    }
  /* else we assume not pressed */

  switch (step)
    {
      /* currently we make no distinction.  When we go bi-di, we need to */
    case CTK_MOVEMENT_LOGICAL_POSITIONS:
    case CTK_MOVEMENT_VISUAL_POSITIONS:
      ctk_tree_view_move_cursor_left_right (tree_view, count);
      break;
    case CTK_MOVEMENT_DISPLAY_LINES:
      ctk_tree_view_move_cursor_up_down (tree_view, count);
      break;
    case CTK_MOVEMENT_PAGES:
      ctk_tree_view_move_cursor_page_up_down (tree_view, count);
      break;
    case CTK_MOVEMENT_BUFFER_ENDS:
      ctk_tree_view_move_cursor_start_end (tree_view, count);
      break;
    default:
      g_assert_not_reached ();
    }

  tree_view->priv->modify_selection_pressed = FALSE;
  tree_view->priv->extend_selection_pressed = FALSE;

  return TRUE;
}

static void
ctk_tree_view_put (CtkTreeView       *tree_view,
		   CtkWidget         *child_widget,
                   CtkTreePath       *path,
                   CtkTreeViewColumn *column,
                   const CtkBorder   *border)
{
  CtkTreeViewChild *child;
  
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (CTK_IS_WIDGET (child_widget));

  child = g_slice_new (CtkTreeViewChild);

  child->widget = child_widget;
  if (_ctk_tree_view_find_node (tree_view,
				path,
				&child->tree,
				&child->node))
    {
      g_assert_not_reached ();
    }
  child->column = column;
  child->border = *border;

  tree_view->priv->children = g_list_append (tree_view->priv->children, child);

  if (ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    ctk_widget_set_parent_window (child->widget, tree_view->priv->bin_window);
  
  ctk_widget_set_parent (child_widget, CTK_WIDGET (tree_view));
}

/* TreeModel Callbacks
 */

static void
ctk_tree_view_row_changed (CtkTreeModel *model,
			   CtkTreePath  *path,
			   CtkTreeIter  *iter,
			   gpointer      data)
{
  CtkTreeView *tree_view = (CtkTreeView *)data;
  CtkRBTree *tree;
  CtkRBNode *node;
  gboolean free_path = FALSE;
  GList *list;
  CtkTreePath *cursor_path;

  g_return_if_fail (path != NULL || iter != NULL);

  if (tree_view->priv->cursor_node != NULL)
    cursor_path = _ctk_tree_path_new_from_rbtree (tree_view->priv->cursor_tree,
                                                  tree_view->priv->cursor_node);
  else
    cursor_path = NULL;

  if (tree_view->priv->edited_column &&
      (cursor_path == NULL || ctk_tree_path_compare (cursor_path, path) == 0))
    ctk_tree_view_stop_editing (tree_view, TRUE);

  if (cursor_path != NULL)
    ctk_tree_path_free (cursor_path);

  if (path == NULL)
    {
      path = ctk_tree_model_get_path (model, iter);
      free_path = TRUE;
    }
  else if (iter == NULL)
    ctk_tree_model_get_iter (model, iter, path);

  if (_ctk_tree_view_find_node (tree_view,
				path,
				&tree,
				&node))
    /* We aren't actually showing the node */
    goto done;

  if (tree == NULL)
    goto done;

  _ctk_tree_view_accessible_changed (tree_view, tree, node);

  if (tree_view->priv->fixed_height_mode
      && tree_view->priv->fixed_height >= 0)
    {
      _ctk_rbtree_node_set_height (tree, node, tree_view->priv->fixed_height);
      if (ctk_widget_get_realized (CTK_WIDGET (tree_view)))
	ctk_tree_view_node_queue_redraw (tree_view, tree, node);
    }
  else
    {
      _ctk_rbtree_node_mark_invalid (tree, node);
      for (list = tree_view->priv->columns; list; list = list->next)
        {
          CtkTreeViewColumn *column;

          column = list->data;
          if (!ctk_tree_view_column_get_visible (column))
            continue;

          if (ctk_tree_view_column_get_sizing (column) == CTK_TREE_VIEW_COLUMN_AUTOSIZE)
            {
              _ctk_tree_view_column_cell_set_dirty (column, TRUE);
            }
        }
    }

 done:
  if (!tree_view->priv->fixed_height_mode &&
      ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    install_presize_handler (tree_view);
  if (free_path)
    ctk_tree_path_free (path);
}

static void
ctk_tree_view_row_inserted (CtkTreeModel *model,
			    CtkTreePath  *path,
			    CtkTreeIter  *iter,
			    gpointer      data)
{
  CtkTreeView *tree_view = (CtkTreeView *) data;
  gint *indices;
  CtkRBTree *tree;
  CtkRBNode *tmpnode = NULL;
  gint depth;
  gint i = 0;
  gint height;
  gboolean free_path = FALSE;
  gboolean node_visible = TRUE;

  g_return_if_fail (path != NULL || iter != NULL);

  if (tree_view->priv->fixed_height_mode
      && tree_view->priv->fixed_height >= 0)
    height = tree_view->priv->fixed_height;
  else
    height = 0;

  if (path == NULL)
    {
      path = ctk_tree_model_get_path (model, iter);
      free_path = TRUE;
    }
  else if (iter == NULL)
    ctk_tree_model_get_iter (model, iter, path);

  if (tree_view->priv->tree == NULL)
    tree_view->priv->tree = _ctk_rbtree_new ();

  tree = tree_view->priv->tree;

  /* Update all row-references */
  ctk_tree_row_reference_inserted (G_OBJECT (data), path);
  depth = ctk_tree_path_get_depth (path);
  indices = ctk_tree_path_get_indices (path);

  /* First, find the parent tree */
  while (i < depth - 1)
    {
      if (tree == NULL)
	{
	  /* We aren't showing the node */
	  node_visible = FALSE;
          goto done;
	}

      tmpnode = _ctk_rbtree_find_count (tree, indices[i] + 1);
      if (tmpnode == NULL)
	{
	  g_warning ("A node was inserted with a parent that's not in the tree.\n" \
		     "This possibly means that a CtkTreeModel inserted a child node\n" \
		     "before the parent was inserted.");
          goto done;
	}
      else if (!CTK_RBNODE_FLAG_SET (tmpnode, CTK_RBNODE_IS_PARENT))
	{
          /* FIXME enforce correct behavior on model, probably */
	  /* In theory, the model should have emitted has_child_toggled here.  We
	   * try to catch it anyway, just to be safe, in case the model hasn't.
	   */
	  CtkTreePath *tmppath = _ctk_tree_path_new_from_rbtree (tree, tmpnode);
	  ctk_tree_view_row_has_child_toggled (model, tmppath, NULL, data);
	  ctk_tree_path_free (tmppath);
          goto done;
	}

      tree = tmpnode->children;
      i++;
    }

  if (tree == NULL)
    {
      node_visible = FALSE;
      goto done;
    }

  /* ref the node */
  ctk_tree_model_ref_node (tree_view->priv->model, iter);
  if (indices[depth - 1] == 0)
    {
      tmpnode = _ctk_rbtree_find_count (tree, 1);
      tmpnode = _ctk_rbtree_insert_before (tree, tmpnode, height, FALSE);
    }
  else
    {
      tmpnode = _ctk_rbtree_find_count (tree, indices[depth - 1]);
      tmpnode = _ctk_rbtree_insert_after (tree, tmpnode, height, FALSE);
    }

  _ctk_tree_view_accessible_add (tree_view, tree, tmpnode);

 done:
  if (height > 0)
    {
      if (tree)
        _ctk_rbtree_node_mark_valid (tree, tmpnode);

      if (node_visible && node_is_visible (tree_view, tree, tmpnode))
	ctk_widget_queue_resize (CTK_WIDGET (tree_view));
      else
	ctk_widget_queue_resize_no_redraw (CTK_WIDGET (tree_view));
    }
  else
    install_presize_handler (tree_view);
  if (free_path)
    ctk_tree_path_free (path);
}

static void
ctk_tree_view_row_has_child_toggled (CtkTreeModel *model,
				     CtkTreePath  *path,
				     CtkTreeIter  *iter,
				     gpointer      data)
{
  CtkTreeView *tree_view = (CtkTreeView *)data;
  CtkTreeIter real_iter;
  gboolean has_child;
  CtkRBTree *tree;
  CtkRBNode *node;
  gboolean free_path = FALSE;

  g_return_if_fail (path != NULL || iter != NULL);

  if (iter)
    real_iter = *iter;

  if (path == NULL)
    {
      path = ctk_tree_model_get_path (model, iter);
      free_path = TRUE;
    }
  else if (iter == NULL)
    ctk_tree_model_get_iter (model, &real_iter, path);

  if (_ctk_tree_view_find_node (tree_view,
				path,
				&tree,
				&node))
    /* We aren't actually showing the node */
    goto done;

  if (tree == NULL)
    goto done;

  has_child = ctk_tree_model_iter_has_child (model, &real_iter);
  /* Sanity check.
   */
  if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_PARENT) == has_child)
    goto done;

  if (has_child)
    {
      CTK_RBNODE_SET_FLAG (node, CTK_RBNODE_IS_PARENT);
      _ctk_tree_view_accessible_add_state (tree_view, tree, node, CTK_CELL_RENDERER_EXPANDABLE);
    }
  else
    {
      CTK_RBNODE_UNSET_FLAG (node, CTK_RBNODE_IS_PARENT);
      _ctk_tree_view_accessible_remove_state (tree_view, tree, node, CTK_CELL_RENDERER_EXPANDABLE);
    }

  if (has_child && tree_view->priv->is_list)
    {
      tree_view->priv->is_list = FALSE;
      if (tree_view->priv->show_expanders)
	{
	  GList *list;

	  for (list = tree_view->priv->columns; list; list = list->next)
	    if (ctk_tree_view_column_get_visible (CTK_TREE_VIEW_COLUMN (list->data)))
	      {
		_ctk_tree_view_column_cell_set_dirty (CTK_TREE_VIEW_COLUMN (list->data), TRUE);
		break;
	      }
	}
      ctk_widget_queue_resize (CTK_WIDGET (tree_view));
    }
  else
    {
      _ctk_tree_view_queue_draw_node (tree_view, tree, node, NULL);
    }

 done:
  if (free_path)
    ctk_tree_path_free (path);
}

static void
count_children_helper (CtkRBTree *tree G_GNUC_UNUSED,
		       CtkRBNode *node,
		       gpointer   data)
{
  if (node->children)
    _ctk_rbtree_traverse (node->children, node->children->root, G_POST_ORDER, count_children_helper, data);
  (*((gint *)data))++;
}

static void
check_selection_helper (CtkRBTree *tree G_GNUC_UNUSED,
                        CtkRBNode *node,
                        gpointer   data)
{
  gint *value = (gint *)data;

  *value |= CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED);

  if (node->children && !*value)
    _ctk_rbtree_traverse (node->children, node->children->root, G_POST_ORDER, check_selection_helper, data);
}

static void
ctk_tree_view_row_deleted (CtkTreeModel *model G_GNUC_UNUSED,
			   CtkTreePath  *path,
			   gpointer      data)
{
  CtkTreeView *tree_view = (CtkTreeView *)data;
  CtkRBTree *tree;
  CtkRBNode *node;
  GList *list;
  gboolean selection_changed = FALSE, cursor_changed = FALSE;
  CtkRBTree *cursor_tree = NULL;
  CtkRBNode *cursor_node = NULL;

  g_return_if_fail (path != NULL);

  ctk_tree_row_reference_deleted (G_OBJECT (data), path);

  if (_ctk_tree_view_find_node (tree_view, path, &tree, &node))
    return;

  if (tree == NULL)
    return;

  /* check if the selection has been changed */
  _ctk_rbtree_traverse (tree, node, G_POST_ORDER,
                        check_selection_helper, &selection_changed);

  for (list = tree_view->priv->columns; list; list = list->next)
    if (ctk_tree_view_column_get_visible (CTK_TREE_VIEW_COLUMN (list->data)) &&
	ctk_tree_view_column_get_sizing (CTK_TREE_VIEW_COLUMN (list->data)) == CTK_TREE_VIEW_COLUMN_AUTOSIZE)
      _ctk_tree_view_column_cell_set_dirty ((CtkTreeViewColumn *)list->data, TRUE);

  /* Ensure we don't have a dangling pointer to a dead node */
  ensure_unprelighted (tree_view);

  /* Cancel editting if we've started */
  ctk_tree_view_stop_editing (tree_view, TRUE);

  /* If the cursor row got deleted, move the cursor to the next row */
  if (tree_view->priv->cursor_node &&
      (tree_view->priv->cursor_node == node ||
       (node->children && (tree_view->priv->cursor_tree == node->children ||
                           _ctk_rbtree_contains (node->children, tree_view->priv->cursor_tree)))))
    {
      CtkTreePath *cursor_path;

      cursor_tree = tree;
      cursor_node = _ctk_rbtree_next (tree, node);
      /* find the first node that is not going to be deleted */
      while (cursor_node == NULL && cursor_tree->parent_tree)
        {
          cursor_node = _ctk_rbtree_next (cursor_tree->parent_tree,
                                          cursor_tree->parent_node);
          cursor_tree = cursor_tree->parent_tree;
        }

      if (cursor_node != NULL)
        cursor_path = _ctk_tree_path_new_from_rbtree (cursor_tree, cursor_node);
      else
        cursor_path = NULL;

      if (cursor_path == NULL ||
          ! search_first_focusable_path (tree_view, &cursor_path, TRUE, 
                                         &cursor_tree, &cursor_node))
        {
          /* It looks like we reached the end of the view without finding
           * a focusable row.  We will step backwards to find the last
           * focusable row.
           */
          _ctk_rbtree_prev_full (tree, node, &cursor_tree, &cursor_node);
          if (cursor_node)
            {
              cursor_path = _ctk_tree_path_new_from_rbtree (cursor_tree, cursor_node);
              if (! search_first_focusable_path (tree_view, &cursor_path, FALSE,
                                                 &cursor_tree, &cursor_node))
                cursor_node = NULL;
              ctk_tree_path_free (cursor_path);
            }
        }
      else if (cursor_path)
        ctk_tree_path_free (cursor_path);

      cursor_changed = TRUE;
    }

  if (tree_view->priv->destroy_count_func)
    {
      gint child_count = 0;
      if (node->children)
	_ctk_rbtree_traverse (node->children, node->children->root, G_POST_ORDER, count_children_helper, &child_count);
      tree_view->priv->destroy_count_func (tree_view, path, child_count, tree_view->priv->destroy_count_data);
    }

  if (tree->root->count == 1)
    {
      if (tree_view->priv->tree == tree)
	tree_view->priv->tree = NULL;

      _ctk_tree_view_accessible_remove_state (tree_view,
                                              tree->parent_tree, tree->parent_node,
                                              CTK_CELL_RENDERER_EXPANDED);
      _ctk_tree_view_accessible_remove (tree_view, tree, NULL);
      _ctk_rbtree_remove (tree);
    }
  else
    {
      _ctk_tree_view_accessible_remove (tree_view, tree, node);
      _ctk_rbtree_remove_node (tree, node);
    }

  if (! ctk_tree_row_reference_valid (tree_view->priv->top_row))
    {
      ctk_tree_row_reference_free (tree_view->priv->top_row);
      tree_view->priv->top_row = NULL;
    }

  install_scroll_sync_handler (tree_view);

  ctk_widget_queue_resize (CTK_WIDGET (tree_view));

  if (cursor_changed)
    {
      if (cursor_node)
        {
          CtkTreePath *cursor_path = _ctk_tree_path_new_from_rbtree (cursor_tree, cursor_node);
          ctk_tree_view_real_set_cursor (tree_view, cursor_path, CLEAR_AND_SELECT | CURSOR_INVALID);
          ctk_tree_path_free (cursor_path);
        }
      else
        ctk_tree_view_real_set_cursor (tree_view, NULL, CLEAR_AND_SELECT | CURSOR_INVALID);
    }
  if (selection_changed)
    g_signal_emit_by_name (tree_view->priv->selection, "changed");
}

static void
ctk_tree_view_rows_reordered (CtkTreeModel *model,
			      CtkTreePath  *parent,
			      CtkTreeIter  *iter,
			      gint         *new_order,
			      gpointer      data)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (data);
  CtkRBTree *tree;
  CtkRBNode *node;
  gint len;

  len = ctk_tree_model_iter_n_children (model, iter);

  if (len < 2)
    return;

  ctk_tree_row_reference_reordered (G_OBJECT (data),
				    parent,
				    iter,
				    new_order);

  if (_ctk_tree_view_find_node (tree_view,
				parent,
				&tree,
				&node))
    return;

  /* We need to special case the parent path */
  if (tree == NULL)
    tree = tree_view->priv->tree;
  else
    tree = node->children;

  if (tree == NULL)
    return;

  if (tree_view->priv->edited_column)
    ctk_tree_view_stop_editing (tree_view, TRUE);

  /* we need to be unprelighted */
  ensure_unprelighted (tree_view);

  _ctk_rbtree_reorder (tree, new_order, len);

  _ctk_tree_view_accessible_reorder (tree_view);

  ctk_widget_queue_draw (CTK_WIDGET (tree_view));

  ctk_tree_view_dy_to_top_row (tree_view);
}


/* Internal tree functions
 */


static void
ctk_tree_view_get_background_xrange (CtkTreeView       *tree_view,
                                     CtkRBTree         *tree G_GNUC_UNUSED,
                                     CtkTreeViewColumn *column,
                                     gint              *x1,
                                     gint              *x2)
{
  CtkTreeViewColumn *tmp_column = NULL;
  gint total_width;
  GList *list;
  gboolean rtl;

  if (x1)
    *x1 = 0;

  if (x2)
    *x2 = 0;

  rtl = (ctk_widget_get_direction (CTK_WIDGET (tree_view)) == CTK_TEXT_DIR_RTL);

  total_width = 0;
  for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
       list;
       list = (rtl ? list->prev : list->next))
    {
      tmp_column = list->data;

      if (tmp_column == column)
        break;

      if (ctk_tree_view_column_get_visible (tmp_column))
        total_width += ctk_tree_view_column_get_width (tmp_column);
    }

  if (tmp_column != column)
    {
      g_warning (G_STRLOC": passed-in column isn't in the tree");
      return;
    }

  if (x1)
    *x1 = total_width;

  if (x2)
    {
      if (ctk_tree_view_column_get_visible (column))
        *x2 = total_width + ctk_tree_view_column_get_width (column);
      else
        *x2 = total_width; /* width of 0 */
    }
}

static void
ctk_tree_view_get_arrow_xrange (CtkTreeView *tree_view,
				CtkRBTree   *tree,
                                gint        *x1,
                                gint        *x2)
{
  gint x_offset = 0;
  GList *list;
  CtkTreeViewColumn *tmp_column = NULL;
  gint total_width;
  gint expander_size, expander_render_size;
  gint horizontal_separator;
  gboolean indent_expanders;
  gboolean rtl;

  ctk_widget_style_get (CTK_WIDGET (tree_view),
			"indent-expanders", &indent_expanders,
                        "horizontal-separator", &horizontal_separator,
			NULL);

  rtl = (ctk_widget_get_direction (CTK_WIDGET (tree_view)) == CTK_TEXT_DIR_RTL);
  expander_size = ctk_tree_view_get_expander_size (tree_view);
  expander_render_size = expander_size - (horizontal_separator / 2);

  total_width = 0;
  for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
       list;
       list = (rtl ? list->prev : list->next))
    {
      tmp_column = list->data;

      if (ctk_tree_view_is_expander_column (tree_view, tmp_column))
        {
	  if (rtl)
	    x_offset = total_width + ctk_tree_view_column_get_width (tmp_column) - expander_size;
	  else
	    x_offset = total_width;
          break;
        }

      if (ctk_tree_view_column_get_visible (tmp_column))
        total_width += ctk_tree_view_column_get_width (tmp_column);
    }

  x_offset += (expander_size - expander_render_size);

  if (indent_expanders)
    {
      if (rtl)
	x_offset -= expander_size * _ctk_rbtree_get_depth (tree);
      else
	x_offset += expander_size * _ctk_rbtree_get_depth (tree);
    }

  *x1 = x_offset;

  if (tmp_column &&
      ctk_tree_view_column_get_visible (tmp_column))
    *x2 = *x1 + expander_render_size;
  else
    *x2 = *x1;
}

static void
ctk_tree_view_build_tree (CtkTreeView *tree_view,
			  CtkRBTree   *tree,
			  CtkTreeIter *iter,
			  gint         depth,
			  gboolean     recurse)
{
  CtkRBNode *temp = NULL;
  CtkTreePath *path = NULL;

  do
    {
      ctk_tree_model_ref_node (tree_view->priv->model, iter);
      temp = _ctk_rbtree_insert_after (tree, temp, 0, FALSE);

      if (tree_view->priv->fixed_height > 0)
        {
          if (CTK_RBNODE_FLAG_SET (temp, CTK_RBNODE_INVALID))
	    {
              _ctk_rbtree_node_set_height (tree, temp, tree_view->priv->fixed_height);
	      _ctk_rbtree_node_mark_valid (tree, temp);
	    }
        }

      if (tree_view->priv->is_list)
        continue;

      if (recurse)
	{
	  CtkTreeIter child;

	  if (!path)
	    path = ctk_tree_model_get_path (tree_view->priv->model, iter);
	  else
	    ctk_tree_path_next (path);

	  if (ctk_tree_model_iter_has_child (tree_view->priv->model, iter))
	    {
	      gboolean expand;

	      g_signal_emit (tree_view, tree_view_signals[TEST_EXPAND_ROW], 0, iter, path, &expand);

	      if (ctk_tree_model_iter_children (tree_view->priv->model, &child, iter)
		  && !expand)
	        {
	          temp->children = _ctk_rbtree_new ();
	          temp->children->parent_tree = tree;
	          temp->children->parent_node = temp;
	          ctk_tree_view_build_tree (tree_view, temp->children, &child, depth + 1, recurse);
		}
	    }
	}

      if (ctk_tree_model_iter_has_child (tree_view->priv->model, iter))
	{
	  if ((temp->flags&CTK_RBNODE_IS_PARENT) != CTK_RBNODE_IS_PARENT)
	    temp->flags ^= CTK_RBNODE_IS_PARENT;
	}
    }
  while (ctk_tree_model_iter_next (tree_view->priv->model, iter));

  if (path)
    ctk_tree_path_free (path);
}

/* Make sure the node is visible vertically */
static void
ctk_tree_view_clamp_node_visible (CtkTreeView *tree_view,
				  CtkRBTree   *tree,
				  CtkRBNode   *node)
{
  gint node_dy, height;
  CtkTreePath *path = NULL;

  if (!ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    return;

  /* just return if the node is visible, avoiding a costly expose */
  node_dy = _ctk_rbtree_node_find_offset (tree, node);
  height = ctk_tree_view_get_row_height (tree_view, node);
  if (! CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_INVALID)
      && node_dy >= ctk_adjustment_get_value (tree_view->priv->vadjustment)
      && node_dy + height <= (ctk_adjustment_get_value (tree_view->priv->vadjustment)
                              + ctk_adjustment_get_page_size (tree_view->priv->vadjustment)))
    return;

  path = _ctk_tree_path_new_from_rbtree (tree, node);
  if (path)
    {
      ctk_tree_view_scroll_to_cell (tree_view, path, NULL, FALSE, 0.0, 0.0);
      ctk_tree_path_free (path);
    }
}

static void
ctk_tree_view_clamp_column_visible (CtkTreeView       *tree_view,
				    CtkTreeViewColumn *column,
				    gboolean           focus_to_cell)
{
  CtkAllocation allocation;
  gint x, width;

  if (column == NULL)
    return;

  ctk_widget_get_allocation (ctk_tree_view_column_get_button (column), &allocation);
  x = allocation.x;
  width = allocation.width;

  if (width > ctk_adjustment_get_page_size (tree_view->priv->hadjustment))
    {
      /* The column is larger than the horizontal page size.  If the
       * column has cells which can be focused individually, then we make
       * sure the cell which gets focus is fully visible (if even the
       * focus cell is bigger than the page size, we make sure the
       * left-hand side of the cell is visible).
       *
       * If the column does not have an activatable cell, we
       * make sure the left-hand side of the column is visible.
       */

      if (focus_to_cell && ctk_tree_view_has_can_focus_cell (tree_view))
        {
          CtkCellArea *cell_area;
          CtkCellRenderer *focus_cell;

          cell_area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (column));
          focus_cell = ctk_cell_area_get_focus_cell (cell_area);

          if (ctk_tree_view_column_cell_get_position (column, focus_cell,
                                                      &x, &width))
            {
              if (width < ctk_adjustment_get_page_size (tree_view->priv->hadjustment))
                {
                  if (ctk_adjustment_get_value (tree_view->priv->hadjustment) + ctk_adjustment_get_page_size (tree_view->priv->hadjustment) < x + width)
                    ctk_adjustment_set_value (tree_view->priv->hadjustment,
                                              x + width - ctk_adjustment_get_page_size (tree_view->priv->hadjustment));
                  else if (ctk_adjustment_get_value (tree_view->priv->hadjustment) > x)
                    ctk_adjustment_set_value (tree_view->priv->hadjustment, x);
                }
            }
        }

      ctk_adjustment_set_value (tree_view->priv->hadjustment, x);
    }
  else
    {
      if ((ctk_adjustment_get_value (tree_view->priv->hadjustment) + ctk_adjustment_get_page_size (tree_view->priv->hadjustment)) < (x + width))
	  ctk_adjustment_set_value (tree_view->priv->hadjustment,
				    x + width - ctk_adjustment_get_page_size (tree_view->priv->hadjustment));
      else if (ctk_adjustment_get_value (tree_view->priv->hadjustment) > x)
	ctk_adjustment_set_value (tree_view->priv->hadjustment, x);
  }
}

/* This function could be more efficient.  I'll optimize it if profiling seems
 * to imply that it is important */
CtkTreePath *
_ctk_tree_path_new_from_rbtree (CtkRBTree   *tree,
			        CtkRBNode   *node)
{
  CtkTreePath *path;
  CtkRBTree *tmp_tree;
  CtkRBNode *tmp_node, *last;
  gint count;

  path = ctk_tree_path_new ();

  g_return_val_if_fail (node != NULL, path);

  count = 1 + node->left->count;

  last = node;
  tmp_node = node->parent;
  tmp_tree = tree;
  while (tmp_tree)
    {
      while (!_ctk_rbtree_is_nil (tmp_node))
	{
	  if (tmp_node->right == last)
	    count += 1 + tmp_node->left->count;
	  last = tmp_node;
	  tmp_node = tmp_node->parent;
	}
      ctk_tree_path_prepend_index (path, count - 1);
      last = tmp_tree->parent_node;
      tmp_tree = tmp_tree->parent_tree;
      if (last)
	{
	  count = 1 + last->left->count;
	  tmp_node = last->parent;
	}
    }
  return path;
}

/* Returns TRUE if we ran out of tree before finding the path.  If the path is
 * invalid (ie. points to a node that’s not in the tree), *tree and *node are
 * both set to NULL.
 */
gboolean
_ctk_tree_view_find_node (CtkTreeView  *tree_view,
			  CtkTreePath  *path,
			  CtkRBTree   **tree,
			  CtkRBNode   **node)
{
  CtkRBNode *tmpnode = NULL;
  CtkRBTree *tmptree = tree_view->priv->tree;
  gint *indices = ctk_tree_path_get_indices (path);
  gint depth = ctk_tree_path_get_depth (path);
  gint i = 0;

  *node = NULL;
  *tree = NULL;

  if (depth == 0 || tmptree == NULL)
    return FALSE;
  do
    {
      tmpnode = _ctk_rbtree_find_count (tmptree, indices[i] + 1);
      ++i;
      if (tmpnode == NULL)
	{
	  *tree = NULL;
	  *node = NULL;
	  return FALSE;
	}
      if (i >= depth)
	{
	  *tree = tmptree;
	  *node = tmpnode;
	  return FALSE;
	}
      *tree = tmptree;
      *node = tmpnode;
      tmptree = tmpnode->children;
      if (tmptree == NULL)
	return TRUE;
    }
  while (1);
}

static gboolean
ctk_tree_view_is_expander_column (CtkTreeView       *tree_view,
				  CtkTreeViewColumn *column)
{
  GList *list;

  if (tree_view->priv->is_list)
    return FALSE;

  if (tree_view->priv->expander_column != NULL)
    {
      if (tree_view->priv->expander_column == column)
	return TRUE;
      return FALSE;
    }
  else
    {
      for (list = tree_view->priv->columns;
	   list;
	   list = list->next)
	if (ctk_tree_view_column_get_visible (CTK_TREE_VIEW_COLUMN (list->data)))
	  break;
      if (list && list->data == column)
	return TRUE;
    }
  return FALSE;
}

static inline gboolean
ctk_tree_view_draw_expanders (CtkTreeView *tree_view)
{
  if (!tree_view->priv->is_list && tree_view->priv->show_expanders)
    return TRUE;
  /* else */
  return FALSE;
}

static void
ctk_tree_view_add_move_binding (CtkBindingSet  *binding_set,
				guint           keyval,
				guint           modmask,
				gboolean        add_shifted_binding,
				CtkMovementStep step,
				gint            count)
{
  
  ctk_binding_entry_add_signal (binding_set, keyval, modmask,
                                "move-cursor", 2,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count);

  if (add_shifted_binding)
    ctk_binding_entry_add_signal (binding_set, keyval, CDK_SHIFT_MASK,
				  "move-cursor", 2,
				  G_TYPE_ENUM, step,
				  G_TYPE_INT, count);

  if ((modmask & CDK_CONTROL_MASK) == CDK_CONTROL_MASK)
   return;

  ctk_binding_entry_add_signal (binding_set, keyval, CDK_CONTROL_MASK | CDK_SHIFT_MASK,
                                "move-cursor", 2,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count);

  ctk_binding_entry_add_signal (binding_set, keyval, CDK_CONTROL_MASK,
                                "move-cursor", 2,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count);
}

static gint
ctk_tree_view_unref_tree_helper (CtkTreeModel *model,
				 CtkTreeIter  *iter,
				 CtkRBTree    *tree,
				 CtkRBNode    *node)
{
  gint retval = FALSE;
  do
    {
      g_return_val_if_fail (node != NULL, FALSE);

      if (node->children)
	{
	  CtkTreeIter child;
	  CtkRBTree *new_tree;
	  CtkRBNode *new_node;

	  new_tree = node->children;
          new_node = _ctk_rbtree_first (new_tree);

	  if (!ctk_tree_model_iter_children (model, &child, iter))
	    return FALSE;

	  retval = ctk_tree_view_unref_tree_helper (model, &child, new_tree, new_node) | retval;
	}

      if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
	retval = TRUE;
      ctk_tree_model_unref_node (model, iter);
      node = _ctk_rbtree_next (tree, node);
    }
  while (ctk_tree_model_iter_next (model, iter));

  return retval;
}

static gint
ctk_tree_view_unref_and_check_selection_tree (CtkTreeView *tree_view,
					      CtkRBTree   *tree)
{
  CtkTreeIter iter;
  CtkTreePath *path;
  CtkRBNode *node;
  gint retval;

  if (!tree)
    return FALSE;

  node = _ctk_rbtree_first (tree);

  g_return_val_if_fail (node != NULL, FALSE);
  path = _ctk_tree_path_new_from_rbtree (tree, node);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (tree_view->priv->model),
			   &iter, path);
  retval = ctk_tree_view_unref_tree_helper (CTK_TREE_MODEL (tree_view->priv->model), &iter, tree, node);
  ctk_tree_path_free (path);

  return retval;
}

static void
ctk_tree_view_set_column_drag_info (CtkTreeView       *tree_view,
				    CtkTreeViewColumn *column)
{
  CtkTreeViewColumn *left_column;
  CtkTreeViewColumn *cur_column = NULL;
  CtkTreeViewColumnReorder *reorder;
  gboolean rtl;
  GList *tmp_list;
  gint left;

  /* We want to precalculate the motion list such that we know what column slots
   * are available.
   */
  left_column = NULL;
  rtl = (ctk_widget_get_direction (CTK_WIDGET (tree_view)) == CTK_TEXT_DIR_RTL);

  /* First, identify all possible drop spots */
  if (rtl)
    tmp_list = g_list_last (tree_view->priv->columns);
  else
    tmp_list = g_list_first (tree_view->priv->columns);

  while (tmp_list)
    {
      cur_column = CTK_TREE_VIEW_COLUMN (tmp_list->data);
      tmp_list = rtl ? tmp_list->prev : tmp_list->next;

      if (ctk_tree_view_column_get_visible (cur_column) == FALSE)
	continue;

      /* If it's not the column moving and func tells us to skip over the column, we continue. */
      if (left_column != column && cur_column != column &&
	  tree_view->priv->column_drop_func &&
	  ! tree_view->priv->column_drop_func (tree_view, column, left_column, cur_column, tree_view->priv->column_drop_func_data))
	{
	  left_column = cur_column;
	  continue;
	}
      reorder = g_slice_new0 (CtkTreeViewColumnReorder);
      reorder->left_column = left_column;
      left_column = reorder->right_column = cur_column;

      tree_view->priv->column_drag_info = g_list_append (tree_view->priv->column_drag_info, reorder);
    }

  /* Add the last one */
  if (tree_view->priv->column_drop_func == NULL ||
      ((left_column != column) &&
       tree_view->priv->column_drop_func (tree_view, column, left_column, NULL, tree_view->priv->column_drop_func_data)))
    {
      reorder = g_slice_new0 (CtkTreeViewColumnReorder);
      reorder->left_column = left_column;
      reorder->right_column = NULL;
      tree_view->priv->column_drag_info = g_list_append (tree_view->priv->column_drag_info, reorder);
    }

  /* We quickly check to see if it even makes sense to reorder columns. */
  /* If there is nothing that can be moved, then we return */

  if (tree_view->priv->column_drag_info == NULL)
    return;

  /* We know there are always 2 slots possbile, as you can always return column. */
  /* If that's all there is, return */
  if (tree_view->priv->column_drag_info->next == NULL || 
      (tree_view->priv->column_drag_info->next->next == NULL &&
       ((CtkTreeViewColumnReorder *)tree_view->priv->column_drag_info->data)->right_column == column &&
       ((CtkTreeViewColumnReorder *)tree_view->priv->column_drag_info->next->data)->left_column == column))
    {
      for (tmp_list = tree_view->priv->column_drag_info; tmp_list; tmp_list = tmp_list->next)
	g_slice_free (CtkTreeViewColumnReorder, tmp_list->data);
      g_list_free (tree_view->priv->column_drag_info);
      tree_view->priv->column_drag_info = NULL;
      return;
    }
  /* We fill in the ranges for the columns, now that we've isolated them */
  left = - TREE_VIEW_COLUMN_DRAG_DEAD_MULTIPLIER (tree_view);

  for (tmp_list = tree_view->priv->column_drag_info; tmp_list; tmp_list = tmp_list->next)
    {
      reorder = (CtkTreeViewColumnReorder *) tmp_list->data;

      reorder->left_align = left;
      if (tmp_list->next != NULL)
	{
          CtkAllocation right_allocation, left_allocation;
	  CtkWidget    *left_button, *right_button;

	  g_assert (tmp_list->next->data);

	  right_button = ctk_tree_view_column_get_button (reorder->right_column);
	  left_button  = ctk_tree_view_column_get_button
	    (((CtkTreeViewColumnReorder *)tmp_list->next->data)->left_column);

          ctk_widget_get_allocation (right_button, &right_allocation);
          ctk_widget_get_allocation (left_button, &left_allocation);
	  left = reorder->right_align = (right_allocation.x + right_allocation.width + left_allocation.x) / 2;
	}
      else
	{
	  reorder->right_align = cdk_window_get_width (tree_view->priv->header_window)
                                 + TREE_VIEW_COLUMN_DRAG_DEAD_MULTIPLIER (tree_view);
	}
    }
}

void
_ctk_tree_view_column_start_drag (CtkTreeView       *tree_view,
				  CtkTreeViewColumn *column,
                                  CdkDevice         *device)
{
  CtkAllocation allocation;
  CtkAllocation button_allocation;
  CtkWidget *button;
  CdkWindowAttr attributes;
  guint attributes_mask;
  CtkStyleContext *context;

  g_return_if_fail (tree_view->priv->column_drag_info == NULL);
  g_return_if_fail (tree_view->priv->cur_reorder == NULL);
  g_return_if_fail (tree_view->priv->drag_window == NULL);

  ctk_tree_view_set_column_drag_info (tree_view, column);

  if (tree_view->priv->column_drag_info == NULL)
    return;

  button = ctk_tree_view_column_get_button (column);

  context = ctk_widget_get_style_context (button);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_DND);

  ctk_widget_get_allocation (button, &button_allocation);

  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.wclass = CDK_INPUT_OUTPUT;
  attributes.x = button_allocation.x;
  attributes.y = 0;
  attributes.width = button_allocation.width;
  attributes.height = button_allocation.height;
  attributes.visual = ctk_widget_get_visual (CTK_WIDGET (tree_view));
  attributes.event_mask = CDK_VISIBILITY_NOTIFY_MASK | CDK_POINTER_MOTION_MASK;
  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;

  tree_view->priv->drag_window = cdk_window_new (tree_view->priv->header_window,
                                                 &attributes,
                                                 attributes_mask);
  ctk_widget_register_window (CTK_WIDGET (tree_view), tree_view->priv->drag_window);

  /* Kids, don't try this at home */
  g_object_ref (button);
  ctk_container_remove (CTK_CONTAINER (tree_view), button);
  ctk_widget_set_parent_window (button, tree_view->priv->drag_window);
  ctk_widget_set_parent (button, CTK_WIDGET (tree_view));
  g_object_unref (button);

  ctk_widget_get_allocation (button, &button_allocation);
  tree_view->priv->drag_column_x = button_allocation.x;
  allocation = button_allocation;
  allocation.x = 0;
  ctk_widget_size_allocate (button, &allocation);

  tree_view->priv->drag_column = column;
  cdk_window_show (tree_view->priv->drag_window);

  ctk_widget_grab_focus (CTK_WIDGET (tree_view));

  tree_view->priv->in_column_drag = TRUE;

  /* Widget reparenting above unmaps and indirectly breaks
   * the implicit grab, replace it with an active one.
   */
  cdk_seat_grab (cdk_device_get_seat (device),
                 tree_view->priv->drag_window,
                 CDK_SEAT_CAPABILITY_ALL, FALSE,
                 NULL, NULL, NULL, NULL);

  ctk_gesture_set_state (tree_view->priv->column_drag_gesture,
                         CTK_EVENT_SEQUENCE_CLAIMED);
}

static void
ctk_tree_view_queue_draw_arrow (CtkTreeView        *tree_view,
				CtkRBTree          *tree,
				CtkRBNode          *node)
{
  CtkAllocation allocation;
  CdkRectangle rect;

  if (!ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    return;

  ctk_widget_get_allocation (CTK_WIDGET (tree_view), &allocation);
  rect.x = 0;
  rect.width = ctk_tree_view_get_expander_size (tree_view);
  rect.width = MAX (rect.width, MAX (tree_view->priv->width, allocation.width));

  rect.y = ctk_tree_view_get_row_y_offset (tree_view, tree, node);
  rect.height = ctk_tree_view_get_row_height (tree_view, node);

  cdk_window_invalidate_rect (tree_view->priv->bin_window, &rect, TRUE);
}

void
_ctk_tree_view_queue_draw_node (CtkTreeView        *tree_view,
				CtkRBTree          *tree,
				CtkRBNode          *node,
				const CdkRectangle *clip_rect)
{
  CtkAllocation allocation;
  CdkRectangle rect;

  if (!ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    return;

  ctk_widget_get_allocation (CTK_WIDGET (tree_view), &allocation);
  rect.x = 0;
  rect.width = MAX (tree_view->priv->width, allocation.width);

  rect.y = ctk_tree_view_get_row_y_offset (tree_view, tree, node);
  rect.height = ctk_tree_view_get_row_height (tree_view, node);

  if (clip_rect)
    {
      CdkRectangle new_rect;

      cdk_rectangle_intersect (clip_rect, &rect, &new_rect);

      cdk_window_invalidate_rect (tree_view->priv->bin_window, &new_rect, TRUE);
    }
  else
    {
      cdk_window_invalidate_rect (tree_view->priv->bin_window, &rect, TRUE);
    }
}

static inline gint
ctk_tree_view_get_effective_header_height (CtkTreeView *tree_view)
{
  if (tree_view->priv->headers_visible)
    return tree_view->priv->header_height;
  /* else */
  return 0;
}

gint
_ctk_tree_view_get_header_height (CtkTreeView *tree_view)
{
  return tree_view->priv->header_height;
}

void
_ctk_tree_view_get_row_separator_func (CtkTreeView                 *tree_view,
				       CtkTreeViewRowSeparatorFunc *func,
				       gpointer                    *data)
{
  *func = tree_view->priv->row_separator_func;
  *data = tree_view->priv->row_separator_data;
}

CtkTreePath *
_ctk_tree_view_get_anchor_path (CtkTreeView *tree_view)
{
  if (tree_view->priv->anchor)
    return ctk_tree_row_reference_get_path (tree_view->priv->anchor);

  return NULL;
}

void
_ctk_tree_view_set_anchor_path (CtkTreeView *tree_view,
				CtkTreePath *anchor_path)
{
  if (tree_view->priv->anchor)
    {
      ctk_tree_row_reference_free (tree_view->priv->anchor);
      tree_view->priv->anchor = NULL;
    }

  if (anchor_path && tree_view->priv->model)
    tree_view->priv->anchor =
      ctk_tree_row_reference_new_proxy (G_OBJECT (tree_view), 
					tree_view->priv->model, anchor_path);
}

CtkRBTree *
_ctk_tree_view_get_rbtree (CtkTreeView *tree_view)
{
  return tree_view->priv->tree;
}

gboolean
_ctk_tree_view_get_cursor_node (CtkTreeView  *tree_view,
                                CtkRBTree   **tree,
                                CtkRBNode   **node)
{
  CtkTreeViewPrivate *priv;

  priv = tree_view->priv;

  if (priv->cursor_node == NULL)
    return FALSE;

  *tree = priv->cursor_tree;
  *node = priv->cursor_node;

  return TRUE;
}

CdkWindow *
_ctk_tree_view_get_header_window (CtkTreeView *tree_view)
{
  return tree_view->priv->header_window;
}

CtkTreeViewColumn *
_ctk_tree_view_get_focus_column (CtkTreeView *tree_view)
{
  return tree_view->priv->focus_column;
}

void
_ctk_tree_view_set_focus_column (CtkTreeView       *tree_view,
				 CtkTreeViewColumn *column)
{
  CtkTreeViewColumn *old_column = tree_view->priv->focus_column;

  if (old_column == column)
    return;

  tree_view->priv->focus_column = column;

  _ctk_tree_view_accessible_update_focus_column (tree_view, 
                                                 old_column,
                                                 column);
}


static void
ctk_tree_view_queue_draw_path (CtkTreeView        *tree_view,
                               CtkTreePath        *path,
                               const CdkRectangle *clip_rect)
{
  CtkRBTree *tree = NULL;
  CtkRBNode *node = NULL;

  _ctk_tree_view_find_node (tree_view, path, &tree, &node);

  if (tree)
    _ctk_tree_view_queue_draw_node (tree_view, tree, node, clip_rect);
}

/* x and y are the mouse position
 */
static void
ctk_tree_view_draw_arrow (CtkTreeView *tree_view,
                          cairo_t     *cr,
                          CtkRBTree   *tree,
			  CtkRBNode   *node)
{
  CdkRectangle area;
  CtkStateFlags state = 0;
  CtkStyleContext *context;
  CtkWidget *widget;
  gint x_offset = 0;
  gint x2;
  gint vertical_separator;
  CtkCellRendererState flags = 0;

  widget = CTK_WIDGET (tree_view);
  context = ctk_widget_get_style_context (widget);

  ctk_widget_style_get (widget,
                        "vertical-separator", &vertical_separator,
                        NULL);

  if (! CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_PARENT))
    return;

  ctk_tree_view_get_arrow_xrange (tree_view, tree, &x_offset, &x2);

  area.x = x_offset;
  area.y = ctk_tree_view_get_cell_area_y_offset (tree_view, tree, node,
                                                 vertical_separator);
  area.width = x2 - x_offset;
  area.height = ctk_tree_view_get_cell_area_height (tree_view, node,
                                                    vertical_separator);

  if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
    flags |= CTK_CELL_RENDERER_SELECTED;

  if (node == tree_view->priv->prelight_node &&
      tree_view->priv->arrow_prelit)
    flags |= CTK_CELL_RENDERER_PRELIT;

  state = ctk_cell_renderer_get_state (NULL, widget, flags);

  if (node->children != NULL)
    state |= CTK_STATE_FLAG_CHECKED;
  else
    state &= ~(CTK_STATE_FLAG_CHECKED);

  ctk_style_context_save (context);

  ctk_style_context_set_state (context, state);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_EXPANDER);

  /* Make sure area.height has the same parity as the "expander-size" style
   * property (which area.width is assumed to be exactly equal to). This is done
   * to avoid the arrow being vertically centered in a half-pixel, which would
   * result in a fuzzy rendering.
   */
  if (area.height % 2 != area.width % 2)
    {
      area.y += 1;
      area.height -= 1;
    }

  ctk_render_expander (context, cr,
                       area.x, area.y,
                       area.width, area.height);

  ctk_style_context_restore (context);
}

static void
ctk_tree_view_focus_to_cursor (CtkTreeView *tree_view)

{
  CtkTreePath *cursor_path;

  if ((tree_view->priv->tree == NULL) ||
      (! ctk_widget_get_realized (CTK_WIDGET (tree_view))))
    return;

  cursor_path = NULL;
  if (tree_view->priv->cursor_node)
    cursor_path = _ctk_tree_path_new_from_rbtree (tree_view->priv->cursor_tree,
                                                  tree_view->priv->cursor_node);

  if (cursor_path == NULL)
    {
      /* Consult the selection before defaulting to the
       * first focusable element
       */
      GList *selected_rows;
      CtkTreeModel *model;
      CtkTreeSelection *selection;

      selection = ctk_tree_view_get_selection (tree_view);
      selected_rows = ctk_tree_selection_get_selected_rows (selection, &model);

      if (selected_rows)
	{
          cursor_path = ctk_tree_path_copy((const CtkTreePath *)(selected_rows->data));
	  g_list_free_full (selected_rows, (GDestroyNotify) ctk_tree_path_free);
        }
      else
	{
	  cursor_path = ctk_tree_path_new_first ();
	  search_first_focusable_path (tree_view, &cursor_path,
				       TRUE, NULL, NULL);
	}

      if (cursor_path)
	{
	  if (ctk_tree_selection_get_mode (tree_view->priv->selection) == CTK_SELECTION_MULTIPLE)
	    ctk_tree_view_real_set_cursor (tree_view, cursor_path, 0);
	  else
	    ctk_tree_view_real_set_cursor (tree_view, cursor_path, CLEAR_AND_SELECT);
	}
    }

  if (cursor_path)
    {
      tree_view->priv->draw_keyfocus = TRUE;

      ctk_tree_view_queue_draw_path (tree_view, cursor_path, NULL);
      ctk_tree_path_free (cursor_path);

      if (tree_view->priv->focus_column == NULL)
	{
	  GList *list;
	  for (list = tree_view->priv->columns; list; list = list->next)
	    {
	      if (ctk_tree_view_column_get_visible (CTK_TREE_VIEW_COLUMN (list->data)))
		{
		  CtkCellArea *cell_area;

                  _ctk_tree_view_set_focus_column (tree_view, CTK_TREE_VIEW_COLUMN (list->data));

		  /* This happens when the treeview initially grabs focus and there
		   * is no column in focus, here we explicitly focus into the first cell */
		  cell_area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (tree_view->priv->focus_column));
		  if (!ctk_cell_area_get_focus_cell (cell_area))
                    {
                      gboolean rtl;

                      rtl = (ctk_widget_get_direction (CTK_WIDGET (tree_view)) == CTK_TEXT_DIR_RTL);
                      ctk_cell_area_focus (cell_area,
                                           rtl ? CTK_DIR_LEFT : CTK_DIR_RIGHT);
                    }

		  break;
		}
	    }
	}
    }
}

static void
ctk_tree_view_move_cursor_up_down (CtkTreeView *tree_view,
				   gint         count)
{
  gint selection_count;
  CtkRBTree *new_cursor_tree = NULL;
  CtkRBNode *new_cursor_node = NULL;
  CtkTreePath *cursor_path = NULL;
  gboolean grab_focus = TRUE;
  gboolean selectable;
  CtkDirectionType direction;
  CtkCellArea *cell_area = NULL;
  CtkCellRenderer *last_focus_cell = NULL;
  CtkTreeIter iter;

  if (! ctk_widget_has_focus (CTK_WIDGET (tree_view)))
    return;

  if (tree_view->priv->cursor_node == NULL)
    return;

  cursor_path = _ctk_tree_path_new_from_rbtree (tree_view->priv->cursor_tree,
                                                tree_view->priv->cursor_node);

  direction = count < 0 ? CTK_DIR_UP : CTK_DIR_DOWN;

  if (tree_view->priv->focus_column)
    cell_area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (tree_view->priv->focus_column));

  /* If focus stays in the area for this row, then just return for this round */
  if (cell_area && (count == -1 || count == 1) &&
      ctk_tree_model_get_iter (tree_view->priv->model, &iter, cursor_path))
    {
      ctk_tree_view_column_cell_set_cell_data (tree_view->priv->focus_column,
					       tree_view->priv->model,
                                               &iter,
                                               CTK_RBNODE_FLAG_SET (tree_view->priv->cursor_node, CTK_RBNODE_IS_PARENT),
					       tree_view->priv->cursor_node->children ? TRUE : FALSE);

      /* Save the last cell that had focus, if we hit the end of the view we'll give
       * focus back to it. */
      last_focus_cell = ctk_cell_area_get_focus_cell (cell_area);

      /* If focus stays in the area, no need to change the cursor row */
      if (ctk_cell_area_focus (cell_area, direction))
	return;
    }

  selection_count = ctk_tree_selection_count_selected_rows (tree_view->priv->selection);
  selectable = _ctk_tree_selection_row_is_selectable (tree_view->priv->selection,
						      tree_view->priv->cursor_node,
						      cursor_path);

  if (selection_count == 0
      && ctk_tree_selection_get_mode (tree_view->priv->selection) != CTK_SELECTION_NONE
      && !tree_view->priv->modify_selection_pressed
      && selectable)
    {
      /* Don't move the cursor, but just select the current node */
      new_cursor_tree = tree_view->priv->cursor_tree;
      new_cursor_node = tree_view->priv->cursor_node;
    }
  else
    {
      if (count == -1)
	_ctk_rbtree_prev_full (tree_view->priv->cursor_tree, tree_view->priv->cursor_node,
			       &new_cursor_tree, &new_cursor_node);
      else
	_ctk_rbtree_next_full (tree_view->priv->cursor_tree, tree_view->priv->cursor_node,
			       &new_cursor_tree, &new_cursor_node);
    }

  ctk_tree_path_free (cursor_path);

  if (new_cursor_node)
    {
      cursor_path = _ctk_tree_path_new_from_rbtree (new_cursor_tree, new_cursor_node);

      search_first_focusable_path (tree_view, &cursor_path,
				   (count != -1),
				   &new_cursor_tree,
				   &new_cursor_node);

      if (cursor_path)
	ctk_tree_path_free (cursor_path);
    }

  /*
   * If the list has only one item and multi-selection is set then select
   * the row (if not yet selected).
   */
  if (ctk_tree_selection_get_mode (tree_view->priv->selection) == CTK_SELECTION_MULTIPLE &&
      new_cursor_node == NULL)
    {
      if (count == -1)
        _ctk_rbtree_next_full (tree_view->priv->cursor_tree, tree_view->priv->cursor_node,
    			       &new_cursor_tree, &new_cursor_node);
      else
        _ctk_rbtree_prev_full (tree_view->priv->cursor_tree, tree_view->priv->cursor_node,
			       &new_cursor_tree, &new_cursor_node);

      if (new_cursor_node == NULL
	  && !CTK_RBNODE_FLAG_SET (tree_view->priv->cursor_node, CTK_RBNODE_IS_SELECTED))
        {
          new_cursor_node = tree_view->priv->cursor_node;
          new_cursor_tree = tree_view->priv->cursor_tree;
        }
      else
        {
          new_cursor_tree = NULL;
          new_cursor_node = NULL;
        }
    }

  if (new_cursor_node)
    {
      cursor_path = _ctk_tree_path_new_from_rbtree (new_cursor_tree, new_cursor_node);
      ctk_tree_view_real_set_cursor (tree_view, cursor_path, CLEAR_AND_SELECT | CLAMP_NODE);
      ctk_tree_path_free (cursor_path);

      /* Give focus to the area in the new row */
      if (cell_area)
	ctk_cell_area_focus (cell_area, direction);
    }
  else
    {
      ctk_tree_view_clamp_node_visible (tree_view, 
                                        tree_view->priv->cursor_tree,
                                        tree_view->priv->cursor_node);

      if (!tree_view->priv->extend_selection_pressed)
        {
          if (! ctk_widget_keynav_failed (CTK_WIDGET (tree_view),
                                          count < 0 ?
                                          CTK_DIR_UP : CTK_DIR_DOWN))
            {
              CtkWidget *toplevel = ctk_widget_get_toplevel (CTK_WIDGET (tree_view));

              if (toplevel)
                ctk_widget_child_focus (toplevel,
                                        count < 0 ?
                                        CTK_DIR_TAB_BACKWARD :
                                        CTK_DIR_TAB_FORWARD);

              grab_focus = FALSE;
            }
        }
      else
        {
          ctk_widget_error_bell (CTK_WIDGET (tree_view));
        }

      if (cell_area)
	ctk_cell_area_set_focus_cell (cell_area, last_focus_cell);
    }

  if (grab_focus)
    ctk_widget_grab_focus (CTK_WIDGET (tree_view));
}

static void
ctk_tree_view_move_cursor_page_up_down (CtkTreeView *tree_view,
					gint         count)
{
  CtkTreePath *old_cursor_path = NULL;
  CtkTreePath *cursor_path = NULL;
  CtkRBTree *start_cursor_tree = NULL;
  CtkRBNode *start_cursor_node = NULL;
  CtkRBTree *cursor_tree;
  CtkRBNode *cursor_node;
  gint y;
  gint window_y;
  gint vertical_separator;

  if (!ctk_widget_has_focus (CTK_WIDGET (tree_view)))
    return;

  if (tree_view->priv->cursor_node == NULL)
    return;

  old_cursor_path = _ctk_tree_path_new_from_rbtree (tree_view->priv->cursor_tree,
                                                    tree_view->priv->cursor_node);

  ctk_widget_style_get (CTK_WIDGET (tree_view), "vertical-separator", &vertical_separator, NULL);

  y = _ctk_rbtree_node_find_offset (tree_view->priv->cursor_tree, tree_view->priv->cursor_node);
  window_y = RBTREE_Y_TO_TREE_WINDOW_Y (tree_view, y);
  y += tree_view->priv->cursor_offset;
  y += count * (int)ctk_adjustment_get_page_increment (tree_view->priv->vadjustment);
  y = CLAMP (y, (gint)ctk_adjustment_get_lower (tree_view->priv->vadjustment),  (gint)ctk_adjustment_get_upper (tree_view->priv->vadjustment) - vertical_separator);

  if (y >= ctk_tree_view_get_height (tree_view))
    y = ctk_tree_view_get_height (tree_view) - 1;

  tree_view->priv->cursor_offset =
    _ctk_rbtree_find_offset (tree_view->priv->tree, y,
			     &cursor_tree, &cursor_node);

  if (cursor_tree == NULL)
    {
      /* FIXME: we lost the cursor.  Should we try to get one? */
      ctk_tree_path_free (old_cursor_path);
      return;
    }

  if (tree_view->priv->cursor_offset
      > ctk_tree_view_get_row_height (tree_view, cursor_node))
    {
      _ctk_rbtree_next_full (cursor_tree, cursor_node,
			     &cursor_tree, &cursor_node);
      tree_view->priv->cursor_offset -= ctk_tree_view_get_row_height (tree_view, cursor_node);
    }

  y -= tree_view->priv->cursor_offset;
  cursor_path = _ctk_tree_path_new_from_rbtree (cursor_tree, cursor_node);

  start_cursor_tree = cursor_tree;
  start_cursor_node = cursor_node;

  if (! search_first_focusable_path (tree_view, &cursor_path,
				     (count != -1),
				     &cursor_tree, &cursor_node))
    {
      /* It looks like we reached the end of the view without finding
       * a focusable row.  We will step backwards to find the last
       * focusable row.
       */
      cursor_tree = start_cursor_tree;
      cursor_node = start_cursor_node;
      cursor_path = _ctk_tree_path_new_from_rbtree (cursor_tree, cursor_node);

      search_first_focusable_path (tree_view, &cursor_path,
				   (count == -1),
				   &cursor_tree, &cursor_node);
    }

  if (!cursor_path)
    goto cleanup;

  /* update y */
  y = _ctk_rbtree_node_find_offset (cursor_tree, cursor_node);

  ctk_tree_view_real_set_cursor (tree_view, cursor_path, CLEAR_AND_SELECT);

  y -= window_y;
  ctk_tree_view_scroll_to_point (tree_view, -1, y);
  ctk_tree_view_clamp_node_visible (tree_view, cursor_tree, cursor_node);
  _ctk_tree_view_queue_draw_node (tree_view, cursor_tree, cursor_node, NULL);

  if (!ctk_tree_path_compare (old_cursor_path, cursor_path))
    ctk_widget_error_bell (CTK_WIDGET (tree_view));

  ctk_widget_grab_focus (CTK_WIDGET (tree_view));

cleanup:
  ctk_tree_path_free (old_cursor_path);
  ctk_tree_path_free (cursor_path);
}

static void
ctk_tree_view_move_cursor_left_right (CtkTreeView *tree_view,
				      gint         count)
{
  CtkTreePath *cursor_path = NULL;
  CtkTreeViewColumn *column;
  CtkTreeIter iter;
  GList *list;
  gboolean found_column = FALSE;
  gboolean rtl;
  CtkDirectionType direction;
  CtkCellArea     *cell_area;
  CtkCellRenderer *last_focus_cell = NULL;
  CtkCellArea     *last_focus_area = NULL;

  rtl = (ctk_widget_get_direction (CTK_WIDGET (tree_view)) == CTK_TEXT_DIR_RTL);

  if (!ctk_widget_has_focus (CTK_WIDGET (tree_view)))
    return;

  if (tree_view->priv->cursor_node == NULL)
    return;

  cursor_path = _ctk_tree_path_new_from_rbtree (tree_view->priv->cursor_tree,
                                                tree_view->priv->cursor_node);

  if (ctk_tree_model_get_iter (tree_view->priv->model, &iter, cursor_path) == FALSE)
    {
      ctk_tree_path_free (cursor_path);
      return;
    }
  ctk_tree_path_free (cursor_path);

  list = rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns);
  if (tree_view->priv->focus_column)
    {
      /* Save the cell/area we are moving focus from, if moving the cursor
       * by one step hits the end we'll set focus back here */
      last_focus_area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (tree_view->priv->focus_column));
      last_focus_cell = ctk_cell_area_get_focus_cell (last_focus_area);

      for (; list; list = (rtl ? list->prev : list->next))
	{
	  if (list->data == tree_view->priv->focus_column)
	    break;
	}
    }

  direction = count > 0 ? CTK_DIR_RIGHT : CTK_DIR_LEFT;

  while (list)
    {
      column = list->data;
      if (ctk_tree_view_column_get_visible (column) == FALSE)
	goto loop_end;

      ctk_tree_view_column_cell_set_cell_data (column,
					       tree_view->priv->model,
					       &iter,
					       CTK_RBNODE_FLAG_SET (tree_view->priv->cursor_node, CTK_RBNODE_IS_PARENT),
					       tree_view->priv->cursor_node->children ? TRUE : FALSE);

      cell_area = ctk_cell_layout_get_area (CTK_CELL_LAYOUT (column));
      if (ctk_cell_area_focus (cell_area, direction))
	{
          _ctk_tree_view_set_focus_column (tree_view, column);
	  found_column = TRUE;
	  break;
	}

    loop_end:
      if (count == 1)
	list = rtl ? list->prev : list->next;
      else
	list = rtl ? list->next : list->prev;
    }

  if (found_column)
    {
      if (!ctk_tree_view_has_can_focus_cell (tree_view))
	_ctk_tree_view_queue_draw_node (tree_view,
				        tree_view->priv->cursor_tree,
				        tree_view->priv->cursor_node,
				        NULL);
      g_signal_emit (tree_view, tree_view_signals[CURSOR_CHANGED], 0);
      ctk_widget_grab_focus (CTK_WIDGET (tree_view));
    }
  else
    {
      ctk_widget_error_bell (CTK_WIDGET (tree_view));

      if (last_focus_area)
	ctk_cell_area_set_focus_cell (last_focus_area, last_focus_cell);
    }

  ctk_tree_view_clamp_column_visible (tree_view,
				      tree_view->priv->focus_column, TRUE);
}

static void
ctk_tree_view_move_cursor_start_end (CtkTreeView *tree_view,
				     gint         count)
{
  CtkRBTree *cursor_tree;
  CtkRBNode *cursor_node;
  CtkTreePath *path;
  CtkTreePath *old_path;

  if (!ctk_widget_has_focus (CTK_WIDGET (tree_view)))
    return;

  g_return_if_fail (tree_view->priv->tree != NULL);

  ctk_tree_view_get_cursor (tree_view, &old_path, NULL);

  cursor_tree = tree_view->priv->tree;

  if (count == -1)
    {
      cursor_node = _ctk_rbtree_first (cursor_tree);

      /* Now go forward to find the first focusable row. */
      path = _ctk_tree_path_new_from_rbtree (cursor_tree, cursor_node);
      search_first_focusable_path (tree_view, &path,
				   TRUE, &cursor_tree, &cursor_node);
    }
  else
    {
      cursor_node = cursor_tree->root;

      do
	{
	  while (cursor_node && !_ctk_rbtree_is_nil (cursor_node->right))
	    cursor_node = cursor_node->right;
	  if (cursor_node->children == NULL)
	    break;

	  cursor_tree = cursor_node->children;
	  cursor_node = cursor_tree->root;
	}
      while (1);

      /* Now go backwards to find last focusable row. */
      path = _ctk_tree_path_new_from_rbtree (cursor_tree, cursor_node);
      search_first_focusable_path (tree_view, &path,
				   FALSE, &cursor_tree, &cursor_node);
    }

  if (!path)
    goto cleanup;

  if (ctk_tree_path_compare (old_path, path))
    {
      ctk_tree_view_real_set_cursor (tree_view, path, CLEAR_AND_SELECT | CLAMP_NODE);
      ctk_widget_grab_focus (CTK_WIDGET (tree_view));
    }
  else
    {
      ctk_widget_error_bell (CTK_WIDGET (tree_view));
    }

cleanup:
  ctk_tree_path_free (old_path);
  ctk_tree_path_free (path);
}

static gboolean
ctk_tree_view_real_select_all (CtkTreeView *tree_view)
{
  if (!ctk_widget_has_focus (CTK_WIDGET (tree_view)))
    return FALSE;

  if (ctk_tree_selection_get_mode (tree_view->priv->selection) != CTK_SELECTION_MULTIPLE)
    return FALSE;

  ctk_tree_selection_select_all (tree_view->priv->selection);

  return TRUE;
}

static gboolean
ctk_tree_view_real_unselect_all (CtkTreeView *tree_view)
{
  if (!ctk_widget_has_focus (CTK_WIDGET (tree_view)))
    return FALSE;

  if (ctk_tree_selection_get_mode (tree_view->priv->selection) != CTK_SELECTION_MULTIPLE)
    return FALSE;

  ctk_tree_selection_unselect_all (tree_view->priv->selection);

  return TRUE;
}

static gboolean
ctk_tree_view_real_select_cursor_row (CtkTreeView *tree_view,
				      gboolean     start_editing)
{
  CtkRBTree *new_tree = NULL;
  CtkRBNode *new_node = NULL;
  CtkRBTree *cursor_tree = NULL;
  CtkRBNode *cursor_node = NULL;
  CtkTreePath *cursor_path = NULL;
  CtkTreeSelectMode mode = 0;

  if (!ctk_widget_has_focus (CTK_WIDGET (tree_view)))
    return FALSE;

  if (tree_view->priv->cursor_node == NULL)
    return FALSE;

  cursor_path = _ctk_tree_path_new_from_rbtree (tree_view->priv->cursor_tree,
                                                tree_view->priv->cursor_node);

  _ctk_tree_view_find_node (tree_view, cursor_path,
			    &cursor_tree, &cursor_node);

  if (cursor_tree == NULL)
    {
      ctk_tree_path_free (cursor_path);
      return FALSE;
    }

  if (!tree_view->priv->extend_selection_pressed && start_editing &&
      tree_view->priv->focus_column)
    {
      if (ctk_tree_view_start_editing (tree_view, cursor_path, FALSE))
	{
	  ctk_tree_path_free (cursor_path);
	  return TRUE;
	}
    }

  if (tree_view->priv->modify_selection_pressed)
    mode |= CTK_TREE_SELECT_MODE_TOGGLE;
  if (tree_view->priv->extend_selection_pressed)
    mode |= CTK_TREE_SELECT_MODE_EXTEND;

  _ctk_tree_selection_internal_select_node (tree_view->priv->selection,
					    cursor_node,
					    cursor_tree,
					    cursor_path,
                                            mode,
					    FALSE);

  /* We bail out if the original (tree, node) don't exist anymore after
   * handling the selection-changed callback.  We do return TRUE because
   * the key press has been handled at this point.
   */
  _ctk_tree_view_find_node (tree_view, cursor_path, &new_tree, &new_node);

  if (cursor_tree != new_tree || cursor_node != new_node)
    return FALSE;

  ctk_tree_view_clamp_node_visible (tree_view, cursor_tree, cursor_node);

  ctk_widget_grab_focus (CTK_WIDGET (tree_view));
  _ctk_tree_view_queue_draw_node (tree_view, cursor_tree, cursor_node, NULL);

  if (!tree_view->priv->extend_selection_pressed)
    ctk_tree_view_row_activated (tree_view, cursor_path,
                                 tree_view->priv->focus_column);
    
  ctk_tree_path_free (cursor_path);

  return TRUE;
}

static gboolean
ctk_tree_view_real_toggle_cursor_row (CtkTreeView *tree_view)
{
  CtkRBTree *new_tree = NULL;
  CtkRBNode *new_node = NULL;
  CtkTreePath *cursor_path = NULL;

  if (!ctk_widget_has_focus (CTK_WIDGET (tree_view)))
    return FALSE;

  if (tree_view->priv->cursor_node == NULL)
    return FALSE;

  cursor_path = _ctk_tree_path_new_from_rbtree (tree_view->priv->cursor_tree,
                                                tree_view->priv->cursor_node);

  _ctk_tree_selection_internal_select_node (tree_view->priv->selection,
					    tree_view->priv->cursor_node,
					    tree_view->priv->cursor_tree,
					    cursor_path,
                                            CTK_TREE_SELECT_MODE_TOGGLE,
					    FALSE);

  /* We bail out if the original (tree, node) don't exist anymore after
   * handling the selection-changed callback.  We do return TRUE because
   * the key press has been handled at this point.
   */
  _ctk_tree_view_find_node (tree_view, cursor_path, &new_tree, &new_node);

  if (tree_view->priv->cursor_node != new_node)
    return FALSE;

  ctk_tree_view_clamp_node_visible (tree_view,
                                    tree_view->priv->cursor_tree,
                                    tree_view->priv->cursor_node);

  ctk_widget_grab_focus (CTK_WIDGET (tree_view));
  ctk_tree_view_queue_draw_path (tree_view, cursor_path, NULL);
  ctk_tree_path_free (cursor_path);

  return TRUE;
}

static gboolean
ctk_tree_view_real_expand_collapse_cursor_row (CtkTreeView *tree_view,
					       gboolean     logical,
					       gboolean     expand,
					       gboolean     open_all)
{
  CtkTreePath *cursor_path = NULL;

  if (!ctk_widget_has_focus (CTK_WIDGET (tree_view)))
    return FALSE;

  if (tree_view->priv->cursor_node == NULL)
    return FALSE;

  cursor_path = _ctk_tree_path_new_from_rbtree (tree_view->priv->cursor_tree,
                                                tree_view->priv->cursor_node);

  /* Don't handle the event if we aren't an expander */
  if (!CTK_RBNODE_FLAG_SET (tree_view->priv->cursor_node, CTK_RBNODE_IS_PARENT))
    return FALSE;

  if (!logical
      && ctk_widget_get_direction (CTK_WIDGET (tree_view)) == CTK_TEXT_DIR_RTL)
    expand = !expand;

  if (expand)
    ctk_tree_view_real_expand_row (tree_view,
                                   cursor_path,
                                   tree_view->priv->cursor_tree,
                                   tree_view->priv->cursor_node,
                                   open_all,
                                   TRUE);
  else
    ctk_tree_view_real_collapse_row (tree_view,
                                     cursor_path,
                                     tree_view->priv->cursor_tree,
                                     tree_view->priv->cursor_node,
                                     TRUE);

  ctk_tree_path_free (cursor_path);

  return TRUE;
}

static gboolean
ctk_tree_view_real_select_cursor_parent (CtkTreeView *tree_view)
{
  CtkTreePath *cursor_path = NULL;
  CdkModifierType state;

  if (!ctk_widget_has_focus (CTK_WIDGET (tree_view)))
    goto out;

  if (tree_view->priv->cursor_node == NULL)
    goto out;

  cursor_path = _ctk_tree_path_new_from_rbtree (tree_view->priv->cursor_tree,
                                                tree_view->priv->cursor_node);

  if (tree_view->priv->cursor_tree->parent_node)
    {
      ctk_tree_view_queue_draw_path (tree_view, cursor_path, NULL);

      ctk_tree_path_up (cursor_path);

      if (ctk_get_current_event_state (&state))
	{
          CdkModifierType modify_mod_mask;

          modify_mod_mask =
            ctk_widget_get_modifier_mask (CTK_WIDGET (tree_view),
                                          CDK_MODIFIER_INTENT_MODIFY_SELECTION);

	  if ((state & modify_mod_mask) == modify_mod_mask)
	    tree_view->priv->modify_selection_pressed = TRUE;
	}

      ctk_tree_view_real_set_cursor (tree_view, cursor_path, CLEAR_AND_SELECT | CLAMP_NODE);
      ctk_tree_path_free (cursor_path);

      ctk_widget_grab_focus (CTK_WIDGET (tree_view));

      tree_view->priv->modify_selection_pressed = FALSE;

      return TRUE;
    }

 out:

  tree_view->priv->search_entry_avoid_unhandled_binding = TRUE;
  return FALSE;
}

static gboolean
ctk_tree_view_search_entry_flush_timeout (CtkTreeView *tree_view)
{
  ctk_tree_view_search_window_hide (tree_view->priv->search_window, tree_view, NULL);
  tree_view->priv->typeselect_flush_timeout = 0;

  return FALSE;
}

/* Cut and paste from ctkwindow.c */
static void
send_focus_change (CtkWidget *widget,
                   CdkDevice *device,
		   gboolean   in)
{
  CdkDeviceManager *device_manager;
  GList *devices, *d;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  device_manager = cdk_display_get_device_manager (ctk_widget_get_display (widget));
  devices = cdk_device_manager_list_devices (device_manager, CDK_DEVICE_TYPE_MASTER);
  devices = g_list_concat (devices, cdk_device_manager_list_devices (device_manager, CDK_DEVICE_TYPE_SLAVE));
  devices = g_list_concat (devices, cdk_device_manager_list_devices (device_manager, CDK_DEVICE_TYPE_FLOATING));
  G_GNUC_END_IGNORE_DEPRECATIONS;

  for (d = devices; d; d = d->next)
    {
      CdkDevice *dev = d->data;
      CdkEvent *fevent;
      CdkWindow *window;

      if (cdk_device_get_source (dev) != CDK_SOURCE_KEYBOARD)
        continue;

      window = ctk_widget_get_window (widget);

      /* Skip non-master keyboards that haven't
       * selected for events from this window
       */
      if (cdk_device_get_device_type (dev) != CDK_DEVICE_TYPE_MASTER &&
          !cdk_window_get_device_events (window, dev))
        continue;

      fevent = cdk_event_new (CDK_FOCUS_CHANGE);

      fevent->focus_change.type = CDK_FOCUS_CHANGE;
      fevent->focus_change.window = g_object_ref (window);
      fevent->focus_change.in = in;
      cdk_event_set_device (fevent, device);

      ctk_widget_send_focus_change (widget, fevent);

      cdk_event_free (fevent);
    }

  g_list_free (devices);
}

static void
ctk_tree_view_ensure_interactive_directory (CtkTreeView *tree_view)
{
  CtkWidget *frame, *vbox, *toplevel;
  CdkScreen *screen;

  if (tree_view->priv->search_custom_entry_set)
    return;

  toplevel = ctk_widget_get_toplevel (CTK_WIDGET (tree_view));
  screen = ctk_widget_get_screen (CTK_WIDGET (tree_view));

   if (tree_view->priv->search_window != NULL)
     {
       if (ctk_window_has_group (CTK_WINDOW (toplevel)))
         ctk_window_group_add_window (ctk_window_get_group (CTK_WINDOW (toplevel)),
                                      CTK_WINDOW (tree_view->priv->search_window));
       else if (ctk_window_has_group (CTK_WINDOW (tree_view->priv->search_window)))
         ctk_window_group_remove_window (ctk_window_get_group (CTK_WINDOW (tree_view->priv->search_window)),
                                         CTK_WINDOW (tree_view->priv->search_window));

       ctk_window_set_screen (CTK_WINDOW (tree_view->priv->search_window), screen);

       return;
     }
   
  tree_view->priv->search_window = ctk_window_new (CTK_WINDOW_POPUP);
  ctk_window_set_screen (CTK_WINDOW (tree_view->priv->search_window), screen);

  if (ctk_window_has_group (CTK_WINDOW (toplevel)))
    ctk_window_group_add_window (ctk_window_get_group (CTK_WINDOW (toplevel)),
				 CTK_WINDOW (tree_view->priv->search_window));

  ctk_window_set_type_hint (CTK_WINDOW (tree_view->priv->search_window),
                            CDK_WINDOW_TYPE_HINT_UTILITY);
  ctk_window_set_modal (CTK_WINDOW (tree_view->priv->search_window), TRUE);
  ctk_window_set_transient_for (CTK_WINDOW (tree_view->priv->search_window),
                                CTK_WINDOW (toplevel));

  g_signal_connect (tree_view->priv->search_window, "delete-event",
		    G_CALLBACK (ctk_tree_view_search_delete_event),
		    tree_view);
  g_signal_connect (tree_view->priv->search_window, "key-press-event",
		    G_CALLBACK (ctk_tree_view_search_key_press_event),
		    tree_view);
  g_signal_connect (tree_view->priv->search_window, "button-press-event",
		    G_CALLBACK (ctk_tree_view_search_button_press_event),
		    tree_view);
  g_signal_connect (tree_view->priv->search_window, "scroll-event",
		    G_CALLBACK (ctk_tree_view_search_scroll_event),
		    tree_view);

  frame = ctk_frame_new (NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_ETCHED_IN);
  ctk_widget_show (frame);
  ctk_container_add (CTK_CONTAINER (tree_view->priv->search_window), frame);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_widget_show (vbox);
  ctk_container_add (CTK_CONTAINER (frame), vbox);
  ctk_container_set_border_width (CTK_CONTAINER (vbox), 3);

  /* add entry */
  tree_view->priv->search_entry = ctk_entry_new ();
  ctk_widget_show (tree_view->priv->search_entry);
  g_signal_connect (tree_view->priv->search_entry, "populate-popup",
		    G_CALLBACK (ctk_tree_view_search_disable_popdown),
		    tree_view);
  g_signal_connect (tree_view->priv->search_entry,
		    "activate", G_CALLBACK (ctk_tree_view_search_activate),
		    tree_view);

  g_signal_connect (_ctk_entry_get_im_context (CTK_ENTRY (tree_view->priv->search_entry)),
		    "preedit-changed",
		    G_CALLBACK (ctk_tree_view_search_preedit_changed),
		    tree_view);
  g_signal_connect (_ctk_entry_get_im_context (CTK_ENTRY (tree_view->priv->search_entry)),
		    "commit",
		    G_CALLBACK (ctk_tree_view_search_commit),
		    tree_view);

  ctk_container_add (CTK_CONTAINER (vbox),
		     tree_view->priv->search_entry);

  ctk_widget_realize (tree_view->priv->search_entry);
}

/* Pops up the interactive search entry.  If keybinding is TRUE then the user
 * started this by typing the start_interactive_search keybinding.  Otherwise, it came from 
 */
static gboolean
ctk_tree_view_real_start_interactive_search (CtkTreeView *tree_view,
                                             CdkDevice   *device,
					     gboolean     keybinding)
{
  /* We only start interactive search if we have focus or the columns
   * have focus.  If one of our children have focus, we don't want to
   * start the search.
   */
  GList *list;
  gboolean found_focus = FALSE;

  if (!tree_view->priv->enable_search && !keybinding)
    return FALSE;

  if (tree_view->priv->search_custom_entry_set)
    return FALSE;

  if (tree_view->priv->search_window != NULL &&
      ctk_widget_get_visible (tree_view->priv->search_window))
    return TRUE;

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      CtkTreeViewColumn *column;
      CtkWidget         *button;

      column = list->data;
      if (!ctk_tree_view_column_get_visible (column))
	continue;

      button = ctk_tree_view_column_get_button (column);
      if (ctk_widget_has_focus (button))
	{
	  found_focus = TRUE;
	  break;
	}
    }
  
  if (ctk_widget_has_focus (CTK_WIDGET (tree_view)))
    found_focus = TRUE;

  if (!found_focus)
    return FALSE;

  if (tree_view->priv->search_column < 0)
    return FALSE;

  ctk_tree_view_ensure_interactive_directory (tree_view);

  if (keybinding)
    ctk_entry_set_text (CTK_ENTRY (tree_view->priv->search_entry), "");

  /* done, show it */
  tree_view->priv->search_position_func (tree_view, tree_view->priv->search_window, tree_view->priv->search_position_user_data);

  /* Grab focus without selecting all the text. */
  ctk_entry_grab_focus_without_selecting (CTK_ENTRY (tree_view->priv->search_entry));

  ctk_widget_show (tree_view->priv->search_window);
  if (tree_view->priv->search_entry_changed_id == 0)
    {
      tree_view->priv->search_entry_changed_id =
	g_signal_connect (tree_view->priv->search_entry, "changed",
			  G_CALLBACK (ctk_tree_view_search_init),
			  tree_view);
    }

  tree_view->priv->typeselect_flush_timeout =
    cdk_threads_add_timeout (CTK_TREE_VIEW_SEARCH_DIALOG_TIMEOUT,
		   (GSourceFunc) ctk_tree_view_search_entry_flush_timeout,
		   tree_view);
  g_source_set_name_by_id (tree_view->priv->typeselect_flush_timeout, "[ctk+] ctk_tree_view_search_entry_flush_timeout");

  /* send focus-in event */
  send_focus_change (tree_view->priv->search_entry, device, TRUE);

  /* search first matching iter */
  ctk_tree_view_search_init (tree_view->priv->search_entry, tree_view);

  return TRUE;
}

static gboolean
ctk_tree_view_start_interactive_search (CtkTreeView *tree_view)
{
  return ctk_tree_view_real_start_interactive_search (tree_view,
                                                      ctk_get_current_event_device (),
                                                      TRUE);
}

/* Callbacks */
static void
ctk_tree_view_adjustment_changed (CtkAdjustment *adjustment G_GNUC_UNUSED,
				  CtkTreeView   *tree_view)
{
  if (ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    {
      gint dy;
	
      cdk_window_move (tree_view->priv->bin_window,
		       - ctk_adjustment_get_value (tree_view->priv->hadjustment),
		       ctk_tree_view_get_effective_header_height (tree_view));
      cdk_window_move (tree_view->priv->header_window,
		       - ctk_adjustment_get_value (tree_view->priv->hadjustment),
		       0);
      dy = tree_view->priv->dy - (int) ctk_adjustment_get_value (tree_view->priv->vadjustment);
      cdk_window_scroll (tree_view->priv->bin_window, 0, dy);

      if (dy != 0)
        {
          /* update our dy and top_row */
          tree_view->priv->dy = (int) ctk_adjustment_get_value (tree_view->priv->vadjustment);

          update_prelight (tree_view,
                           tree_view->priv->event_last_x,
                           tree_view->priv->event_last_y);

          if (!tree_view->priv->in_top_row_to_dy)
            ctk_tree_view_dy_to_top_row (tree_view);

        }
    }
}



/* Public methods
 */

/**
 * ctk_tree_view_new:
 *
 * Creates a new #CtkTreeView widget.
 *
 * Returns: A newly created #CtkTreeView widget.
 **/
CtkWidget *
ctk_tree_view_new (void)
{
  return g_object_new (CTK_TYPE_TREE_VIEW, NULL);
}

/**
 * ctk_tree_view_new_with_model:
 * @model: the model.
 *
 * Creates a new #CtkTreeView widget with the model initialized to @model.
 *
 * Returns: A newly created #CtkTreeView widget.
 **/
CtkWidget *
ctk_tree_view_new_with_model (CtkTreeModel *model)
{
  return g_object_new (CTK_TYPE_TREE_VIEW, "model", model, NULL);
}

/* Public Accessors
 */

/**
 * ctk_tree_view_get_model:
 * @tree_view: a #CtkTreeView
 *
 * Returns the model the #CtkTreeView is based on.  Returns %NULL if the
 * model is unset.
 *
 * Returns: (transfer none) (nullable): A #CtkTreeModel, or %NULL if
 * none is currently being used.
 **/
CtkTreeModel *
ctk_tree_view_get_model (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), NULL);

  return tree_view->priv->model;
}

/**
 * ctk_tree_view_set_model:
 * @tree_view: A #CtkTreeView.
 * @model: (allow-none): The model.
 *
 * Sets the model for a #CtkTreeView.  If the @tree_view already has a model
 * set, it will remove it before setting the new model.  If @model is %NULL,
 * then it will unset the old model.
 **/
void
ctk_tree_view_set_model (CtkTreeView  *tree_view,
			 CtkTreeModel *model)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (model == NULL || CTK_IS_TREE_MODEL (model));

  if (model == tree_view->priv->model)
    return;

  if (tree_view->priv->scroll_to_path)
    {
      ctk_tree_row_reference_free (tree_view->priv->scroll_to_path);
      tree_view->priv->scroll_to_path = NULL;
    }

  if (tree_view->priv->rubber_band_status)
    ctk_tree_view_stop_rubber_band (tree_view);

  if (tree_view->priv->model)
    {
      GList *tmplist = tree_view->priv->columns;

      ctk_tree_view_unref_and_check_selection_tree (tree_view, tree_view->priv->tree);
      ctk_tree_view_stop_editing (tree_view, TRUE);

      g_signal_handlers_disconnect_by_func (tree_view->priv->model,
					    ctk_tree_view_row_changed,
					    tree_view);
      g_signal_handlers_disconnect_by_func (tree_view->priv->model,
					    ctk_tree_view_row_inserted,
					    tree_view);
      g_signal_handlers_disconnect_by_func (tree_view->priv->model,
					    ctk_tree_view_row_has_child_toggled,
					    tree_view);
      g_signal_handlers_disconnect_by_func (tree_view->priv->model,
					    ctk_tree_view_row_deleted,
					    tree_view);
      g_signal_handlers_disconnect_by_func (tree_view->priv->model,
					    ctk_tree_view_rows_reordered,
					    tree_view);

      for (; tmplist; tmplist = tmplist->next)
	_ctk_tree_view_column_unset_model (tmplist->data,
					   tree_view->priv->model);

      if (tree_view->priv->tree)
	ctk_tree_view_free_rbtree (tree_view);

      ctk_tree_row_reference_free (tree_view->priv->drag_dest_row);
      tree_view->priv->drag_dest_row = NULL;
      ctk_tree_row_reference_free (tree_view->priv->anchor);
      tree_view->priv->anchor = NULL;
      ctk_tree_row_reference_free (tree_view->priv->top_row);
      tree_view->priv->top_row = NULL;
      ctk_tree_row_reference_free (tree_view->priv->scroll_to_path);
      tree_view->priv->scroll_to_path = NULL;

      tree_view->priv->scroll_to_column = NULL;

      g_object_unref (tree_view->priv->model);

      tree_view->priv->search_column = -1;
      tree_view->priv->fixed_height_check = 0;
      tree_view->priv->fixed_height = -1;
      tree_view->priv->dy = tree_view->priv->top_row_dy = 0;
    }

  tree_view->priv->model = model;

  if (tree_view->priv->model)
    {
      gint i;
      CtkTreePath *path;
      CtkTreeIter iter;
      CtkTreeModelFlags flags;

      if (tree_view->priv->search_column == -1)
	{
	  for (i = 0; i < ctk_tree_model_get_n_columns (model); i++)
	    {
	      GType type = ctk_tree_model_get_column_type (model, i);

	      if (g_value_type_transformable (type, G_TYPE_STRING))
		{
		  tree_view->priv->search_column = i;
		  break;
		}
	    }
	}

      g_object_ref (tree_view->priv->model);
      g_signal_connect (tree_view->priv->model,
			"row-changed",
			G_CALLBACK (ctk_tree_view_row_changed),
			tree_view);
      g_signal_connect (tree_view->priv->model,
			"row-inserted",
			G_CALLBACK (ctk_tree_view_row_inserted),
			tree_view);
      g_signal_connect (tree_view->priv->model,
			"row-has-child-toggled",
			G_CALLBACK (ctk_tree_view_row_has_child_toggled),
			tree_view);
      g_signal_connect (tree_view->priv->model,
			"row-deleted",
			G_CALLBACK (ctk_tree_view_row_deleted),
			tree_view);
      g_signal_connect (tree_view->priv->model,
			"rows-reordered",
			G_CALLBACK (ctk_tree_view_rows_reordered),
			tree_view);

      flags = ctk_tree_model_get_flags (tree_view->priv->model);
      if ((flags & CTK_TREE_MODEL_LIST_ONLY) == CTK_TREE_MODEL_LIST_ONLY)
        tree_view->priv->is_list = TRUE;
      else
        tree_view->priv->is_list = FALSE;

      path = ctk_tree_path_new_first ();
      if (ctk_tree_model_get_iter (tree_view->priv->model, &iter, path))
	{
	  tree_view->priv->tree = _ctk_rbtree_new ();
	  ctk_tree_view_build_tree (tree_view, tree_view->priv->tree, &iter, 1, FALSE);
          _ctk_tree_view_accessible_add (tree_view, tree_view->priv->tree, NULL);
	}
      ctk_tree_path_free (path);

      /*  FIXME: do I need to do this? ctk_tree_view_create_buttons (tree_view); */
      install_presize_handler (tree_view);
    }

  ctk_tree_view_real_set_cursor (tree_view, NULL, CURSOR_INVALID);

  g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_MODEL]);

  if (tree_view->priv->selection)
  _ctk_tree_selection_emit_changed (tree_view->priv->selection);

  if (tree_view->priv->pixel_cache != NULL)
    _ctk_pixel_cache_set_always_cache (tree_view->priv->pixel_cache, (model != NULL));

  if (ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    ctk_widget_queue_resize (CTK_WIDGET (tree_view));
}

/**
 * ctk_tree_view_get_selection:
 * @tree_view: A #CtkTreeView.
 *
 * Gets the #CtkTreeSelection associated with @tree_view.
 *
 * Returns: (transfer none): A #CtkTreeSelection object.
 **/
CtkTreeSelection *
ctk_tree_view_get_selection (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), NULL);

  return tree_view->priv->selection;
}

/**
 * ctk_tree_view_get_hadjustment:
 * @tree_view: A #CtkTreeView
 *
 * Gets the #CtkAdjustment currently being used for the horizontal aspect.
 *
 * Returns: (transfer none): A #CtkAdjustment object, or %NULL
 *     if none is currently being used.
 *
 * Deprecated: 3.0: Use ctk_scrollable_get_hadjustment()
 **/
CtkAdjustment *
ctk_tree_view_get_hadjustment (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), NULL);

  return ctk_tree_view_do_get_hadjustment (tree_view);
}

static CtkAdjustment *
ctk_tree_view_do_get_hadjustment (CtkTreeView *tree_view)
{
  return tree_view->priv->hadjustment;
}

/**
 * ctk_tree_view_set_hadjustment:
 * @tree_view: A #CtkTreeView
 * @adjustment: (allow-none): The #CtkAdjustment to set, or %NULL
 *
 * Sets the #CtkAdjustment for the current horizontal aspect.
 *
 * Deprecated: 3.0: Use ctk_scrollable_set_hadjustment()
 **/
void
ctk_tree_view_set_hadjustment (CtkTreeView   *tree_view,
                               CtkAdjustment *adjustment)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (adjustment == NULL || CTK_IS_ADJUSTMENT (adjustment));

  ctk_tree_view_do_set_hadjustment (tree_view, adjustment);
}

static void
ctk_tree_view_do_set_hadjustment (CtkTreeView   *tree_view,
                                  CtkAdjustment *adjustment)
{
  CtkTreeViewPrivate *priv = tree_view->priv;

  if (adjustment && priv->hadjustment == adjustment)
    return;

  if (priv->hadjustment != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->hadjustment,
                                            ctk_tree_view_adjustment_changed,
                                            tree_view);
      g_object_unref (priv->hadjustment);
    }

  if (adjustment == NULL)
    adjustment = ctk_adjustment_new (0.0, 0.0, 0.0,
                                     0.0, 0.0, 0.0);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (ctk_tree_view_adjustment_changed), tree_view);
  priv->hadjustment = g_object_ref_sink (adjustment);
  /* FIXME: Adjustment should probably be populated here with fresh values, but
   * internal details are too complicated for me to decipher right now.
   */
  ctk_tree_view_adjustment_changed (NULL, tree_view);

  g_object_notify (G_OBJECT (tree_view), "hadjustment");
}

/**
 * ctk_tree_view_get_vadjustment:
 * @tree_view: A #CtkTreeView
 *
 * Gets the #CtkAdjustment currently being used for the vertical aspect.
 *
 * Returns: (transfer none): A #CtkAdjustment object, or %NULL
 *     if none is currently being used.
 *
 * Deprecated: 3.0: Use ctk_scrollable_get_vadjustment()
 **/
CtkAdjustment *
ctk_tree_view_get_vadjustment (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), NULL);

  return ctk_tree_view_do_get_vadjustment (tree_view);
}

static CtkAdjustment *
ctk_tree_view_do_get_vadjustment (CtkTreeView *tree_view)
{
  return tree_view->priv->vadjustment;
}

/**
 * ctk_tree_view_set_vadjustment:
 * @tree_view: A #CtkTreeView
 * @adjustment: (allow-none): The #CtkAdjustment to set, or %NULL
 *
 * Sets the #CtkAdjustment for the current vertical aspect.
 *
 * Deprecated: 3.0: Use ctk_scrollable_set_vadjustment()
 **/
void
ctk_tree_view_set_vadjustment (CtkTreeView   *tree_view,
                               CtkAdjustment *adjustment)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (adjustment == NULL || CTK_IS_ADJUSTMENT (adjustment));

  ctk_tree_view_do_set_vadjustment (tree_view, adjustment);
}

static void
ctk_tree_view_do_set_vadjustment (CtkTreeView   *tree_view,
                                  CtkAdjustment *adjustment)
{
  CtkTreeViewPrivate *priv = tree_view->priv;

  if (adjustment && priv->vadjustment == adjustment)
    return;

  if (priv->vadjustment != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->vadjustment,
                                            ctk_tree_view_adjustment_changed,
                                            tree_view);
      g_object_unref (priv->vadjustment);
    }

  if (adjustment == NULL)
    adjustment = ctk_adjustment_new (0.0, 0.0, 0.0,
                                     0.0, 0.0, 0.0);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (ctk_tree_view_adjustment_changed), tree_view);
  priv->vadjustment = g_object_ref_sink (adjustment);
  /* FIXME: Adjustment should probably be populated here with fresh values, but
   * internal details are too complicated for me to decipher right now.
   */
  ctk_tree_view_adjustment_changed (NULL, tree_view);
  g_object_notify (G_OBJECT (tree_view), "vadjustment");
}

/* Column and header operations */

/**
 * ctk_tree_view_get_headers_visible:
 * @tree_view: A #CtkTreeView.
 *
 * Returns %TRUE if the headers on the @tree_view are visible.
 *
 * Returns: Whether the headers are visible or not.
 **/
gboolean
ctk_tree_view_get_headers_visible (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);

  return tree_view->priv->headers_visible;
}

/**
 * ctk_tree_view_set_headers_visible:
 * @tree_view: A #CtkTreeView.
 * @headers_visible: %TRUE if the headers are visible
 *
 * Sets the visibility state of the headers.
 **/
void
ctk_tree_view_set_headers_visible (CtkTreeView *tree_view,
				   gboolean     headers_visible)
{
  gint x, y;
  GList *list;
  CtkTreeViewColumn *column;
  CtkAllocation allocation;
  CtkWidget *button;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  headers_visible = !! headers_visible;

  if (tree_view->priv->headers_visible == headers_visible)
    return;

  tree_view->priv->headers_visible = headers_visible == TRUE;

  if (ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    {
      cdk_window_get_position (tree_view->priv->bin_window, &x, &y);
      if (headers_visible)
	{
          ctk_widget_get_allocation (CTK_WIDGET (tree_view), &allocation);
	  cdk_window_move_resize (tree_view->priv->bin_window,
                                  x, y  + ctk_tree_view_get_effective_header_height (tree_view),
                                  tree_view->priv->width, allocation.height -  + ctk_tree_view_get_effective_header_height (tree_view));

          if (ctk_widget_get_mapped (CTK_WIDGET (tree_view)))
            ctk_tree_view_map_buttons (tree_view);
 	}
      else
	{
	  cdk_window_move_resize (tree_view->priv->bin_window, x, y, tree_view->priv->width, ctk_tree_view_get_height (tree_view));

	  for (list = tree_view->priv->columns; list; list = list->next)
	    {
	      column = list->data;
	      button = ctk_tree_view_column_get_button (column);

              ctk_widget_hide (button);
	      ctk_widget_unmap (button);
	    }
	  cdk_window_hide (tree_view->priv->header_window);
	}
    }

  ctk_widget_get_allocation (CTK_WIDGET (tree_view), &allocation);
  ctk_adjustment_configure (tree_view->priv->vadjustment,
                            ctk_adjustment_get_value (tree_view->priv->vadjustment),
                            0,
                            ctk_tree_view_get_height (tree_view),
                            ctk_adjustment_get_step_increment (tree_view->priv->vadjustment),
                            (allocation.height - ctk_tree_view_get_effective_header_height (tree_view)) / 2,
                            allocation.height - ctk_tree_view_get_effective_header_height (tree_view));

  ctk_widget_queue_resize (CTK_WIDGET (tree_view));

  g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_HEADERS_VISIBLE]);
}

/**
 * ctk_tree_view_columns_autosize:
 * @tree_view: A #CtkTreeView.
 *
 * Resizes all columns to their optimal width. Only works after the
 * treeview has been realized.
 **/
void
ctk_tree_view_columns_autosize (CtkTreeView *tree_view)
{
  gboolean dirty = FALSE;
  GList *list;
  CtkTreeViewColumn *column;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      column = list->data;
      if (ctk_tree_view_column_get_sizing (column) == CTK_TREE_VIEW_COLUMN_AUTOSIZE)
	continue;
      _ctk_tree_view_column_cell_set_dirty (column, TRUE);
      dirty = TRUE;
    }

  if (dirty)
    ctk_widget_queue_resize (CTK_WIDGET (tree_view));
}

/**
 * ctk_tree_view_set_headers_clickable:
 * @tree_view: A #CtkTreeView.
 * @setting: %TRUE if the columns are clickable.
 *
 * Allow the column title buttons to be clicked.
 **/
void
ctk_tree_view_set_headers_clickable (CtkTreeView *tree_view,
				     gboolean   setting)
{
  GList *list;
  gboolean changed = FALSE;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      if (ctk_tree_view_column_get_clickable (CTK_TREE_VIEW_COLUMN (list->data)) != setting)
        {
          ctk_tree_view_column_set_clickable (CTK_TREE_VIEW_COLUMN (list->data), setting);
          changed = TRUE;
        }
    }

  if (changed)
    g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_HEADERS_CLICKABLE]);
}


/**
 * ctk_tree_view_get_headers_clickable:
 * @tree_view: A #CtkTreeView.
 *
 * Returns whether all header columns are clickable.
 *
 * Returns: %TRUE if all header columns are clickable, otherwise %FALSE
 *
 * Since: 2.10
 **/
gboolean 
ctk_tree_view_get_headers_clickable (CtkTreeView *tree_view)
{
  GList *list;
  
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);

  for (list = tree_view->priv->columns; list; list = list->next)
    if (!ctk_tree_view_column_get_clickable (CTK_TREE_VIEW_COLUMN (list->data)))
      return FALSE;

  return TRUE;
}

/**
 * ctk_tree_view_set_rules_hint:
 * @tree_view: a #CtkTreeView
 * @setting: %TRUE if the tree requires reading across rows
 *
 * Sets a hint for the theme to draw even/odd rows in the @tree_view
 * with different colors, also known as "zebra striping".
 *
 * This function tells the CTK+ theme that the user interface for your
 * application requires users to read across tree rows and associate
 * cells with one another.
 *
 * Do not use it just because you prefer the appearance of the ruled
 * tree; that’s a question for the theme. Some themes will draw tree
 * rows in alternating colors even when rules are turned off, and
 * users who prefer that appearance all the time can choose those
 * themes. You should call this function only as a semantic hint to
 * the theme engine that your tree makes alternating colors useful
 * from a functional standpoint (since it has lots of columns,
 * generally).
 *
 * Deprecated: 3.14
 */
void
ctk_tree_view_set_rules_hint (CtkTreeView  *tree_view,
                              gboolean      setting)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  setting = setting != FALSE;

  if (tree_view->priv->has_rules != setting)
    {
      tree_view->priv->has_rules = setting;
      ctk_widget_queue_draw (CTK_WIDGET (tree_view));
      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_RULES_HINT]);
    }
}

/**
 * ctk_tree_view_get_rules_hint:
 * @tree_view: a #CtkTreeView
 *
 * Gets the setting set by ctk_tree_view_set_rules_hint().
 *
 * Returns: %TRUE if the hint is set
 *
 * Deprecated: 3.14
 */
gboolean
ctk_tree_view_get_rules_hint (CtkTreeView  *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);

  return tree_view->priv->has_rules;
}


/**
 * ctk_tree_view_set_activate_on_single_click:
 * @tree_view: a #CtkTreeView
 * @single: %TRUE to emit row-activated on a single click
 *
 * Cause the #CtkTreeView::row-activated signal to be emitted
 * on a single click instead of a double click.
 *
 * Since: 3.8
 **/
void
ctk_tree_view_set_activate_on_single_click (CtkTreeView *tree_view,
                                            gboolean     single)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  single = single != FALSE;

  if (tree_view->priv->activate_on_single_click == single)
    return;

  tree_view->priv->activate_on_single_click = single;
  g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_ACTIVATE_ON_SINGLE_CLICK]);
}

/**
 * ctk_tree_view_get_activate_on_single_click:
 * @tree_view: a #CtkTreeView
 *
 * Gets the setting set by ctk_tree_view_set_activate_on_single_click().
 *
 * Returns: %TRUE if row-activated will be emitted on a single click
 *
 * Since: 3.8
 **/
gboolean
ctk_tree_view_get_activate_on_single_click (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);

  return tree_view->priv->activate_on_single_click;
}

/* Public Column functions
 */

/**
 * ctk_tree_view_append_column:
 * @tree_view: A #CtkTreeView.
 * @column: The #CtkTreeViewColumn to add.
 *
 * Appends @column to the list of columns. If @tree_view has “fixed_height”
 * mode enabled, then @column must have its “sizing” property set to be
 * CTK_TREE_VIEW_COLUMN_FIXED.
 *
 * Returns: The number of columns in @tree_view after appending.
 **/
gint
ctk_tree_view_append_column (CtkTreeView       *tree_view,
			     CtkTreeViewColumn *column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), -1);
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (column), -1);
  g_return_val_if_fail (ctk_tree_view_column_get_tree_view (column) == NULL, -1);

  return ctk_tree_view_insert_column (tree_view, column, -1);
}

/**
 * ctk_tree_view_remove_column:
 * @tree_view: A #CtkTreeView.
 * @column: The #CtkTreeViewColumn to remove.
 *
 * Removes @column from @tree_view.
 *
 * Returns: The number of columns in @tree_view after removing.
 **/
gint
ctk_tree_view_remove_column (CtkTreeView       *tree_view,
                             CtkTreeViewColumn *column)
{
  guint position;

  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), -1);
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (column), -1);
  g_return_val_if_fail (ctk_tree_view_column_get_tree_view (column) == CTK_WIDGET (tree_view), -1);

  if (tree_view->priv->focus_column == column)
    _ctk_tree_view_set_focus_column (tree_view, NULL);

  if (tree_view->priv->edited_column == column)
    {
      ctk_tree_view_stop_editing (tree_view, TRUE);

      /* no need to, but just to be sure ... */
      tree_view->priv->edited_column = NULL;
    }

  if (tree_view->priv->expander_column == column)
    tree_view->priv->expander_column = NULL;

  g_signal_handlers_disconnect_by_func (column,
                                        G_CALLBACK (column_sizing_notify),
                                        tree_view);

  position = g_list_index (tree_view->priv->columns, column);

  if (ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    _ctk_tree_view_column_unrealize_button (column);

  _ctk_tree_view_column_unset_tree_view (column);

  tree_view->priv->columns = g_list_remove (tree_view->priv->columns, column);
  tree_view->priv->n_columns--;

  if (ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    {
      GList *list;

      for (list = tree_view->priv->columns; list; list = list->next)
	{
	  CtkTreeViewColumn *tmp_column;

	  tmp_column = CTK_TREE_VIEW_COLUMN (list->data);
	  if (ctk_tree_view_column_get_visible (tmp_column))
            _ctk_tree_view_column_cell_set_dirty (tmp_column, TRUE);
	}

      if (tree_view->priv->n_columns == 0 &&
	  ctk_tree_view_get_headers_visible (tree_view))
	cdk_window_hide (tree_view->priv->header_window);

      ctk_widget_queue_resize (CTK_WIDGET (tree_view));
    }

  _ctk_tree_view_accessible_remove_column (tree_view, column, position);

  g_object_unref (column);
  g_signal_emit (tree_view, tree_view_signals[COLUMNS_CHANGED], 0);

  return tree_view->priv->n_columns;
}

/**
 * ctk_tree_view_insert_column:
 * @tree_view: A #CtkTreeView.
 * @column: The #CtkTreeViewColumn to be inserted.
 * @position: The position to insert @column in.
 *
 * This inserts the @column into the @tree_view at @position.  If @position is
 * -1, then the column is inserted at the end. If @tree_view has
 * “fixed_height” mode enabled, then @column must have its “sizing” property
 * set to be CTK_TREE_VIEW_COLUMN_FIXED.
 *
 * Returns: The number of columns in @tree_view after insertion.
 **/
gint
ctk_tree_view_insert_column (CtkTreeView       *tree_view,
                             CtkTreeViewColumn *column,
                             gint               position)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), -1);
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (column), -1);
  g_return_val_if_fail (ctk_tree_view_column_get_tree_view (column) == NULL, -1);

  if (tree_view->priv->fixed_height_mode)
    g_return_val_if_fail (ctk_tree_view_column_get_sizing (column)
                          == CTK_TREE_VIEW_COLUMN_FIXED, -1);

  if (position < 0 || position > tree_view->priv->n_columns)
    position = tree_view->priv->n_columns;

  g_object_ref_sink (column);

  if (tree_view->priv->n_columns == 0 &&
      ctk_widget_get_realized (CTK_WIDGET (tree_view)) &&
      ctk_tree_view_get_headers_visible (tree_view))
    {
      cdk_window_show (tree_view->priv->header_window);
    }

  g_signal_connect (column, "notify::sizing",
                    G_CALLBACK (column_sizing_notify), tree_view);

  tree_view->priv->columns = g_list_insert (tree_view->priv->columns,
					    column, position);
  tree_view->priv->n_columns++;

  _ctk_tree_view_column_set_tree_view (column, tree_view);

  ctk_tree_view_update_button_position (tree_view, column);

  if (ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    {
      GList *list;

      _ctk_tree_view_column_realize_button (column);

      for (list = tree_view->priv->columns; list; list = list->next)
	{
	  column = CTK_TREE_VIEW_COLUMN (list->data);
	  if (ctk_tree_view_column_get_visible (column))
            _ctk_tree_view_column_cell_set_dirty (column, TRUE);
	}
      ctk_widget_queue_resize (CTK_WIDGET (tree_view));
    }

  _ctk_tree_view_accessible_add_column (tree_view, column, position);

  g_signal_emit (tree_view, tree_view_signals[COLUMNS_CHANGED], 0);

  return tree_view->priv->n_columns;
}

/**
 * ctk_tree_view_insert_column_with_attributes:
 * @tree_view: A #CtkTreeView
 * @position: The position to insert the new column in
 * @title: The title to set the header to
 * @cell: The #CtkCellRenderer
 * @...: A %NULL-terminated list of attributes
 *
 * Creates a new #CtkTreeViewColumn and inserts it into the @tree_view at
 * @position.  If @position is -1, then the newly created column is inserted at
 * the end.  The column is initialized with the attributes given. If @tree_view
 * has “fixed_height” mode enabled, then the new column will have its sizing
 * property set to be CTK_TREE_VIEW_COLUMN_FIXED.
 *
 * Returns: The number of columns in @tree_view after insertion.
 **/
gint
ctk_tree_view_insert_column_with_attributes (CtkTreeView     *tree_view,
					     gint             position,
					     const gchar     *title,
					     CtkCellRenderer *cell,
					     ...)
{
  CtkTreeViewColumn *column;
  gchar *attribute;
  va_list args;
  gint column_id;

  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), -1);

  column = ctk_tree_view_column_new ();
  if (tree_view->priv->fixed_height_mode)
    ctk_tree_view_column_set_sizing (column, CTK_TREE_VIEW_COLUMN_FIXED);

  ctk_tree_view_column_set_title (column, title);
  ctk_tree_view_column_pack_start (column, cell, TRUE);

  va_start (args, cell);

  attribute = va_arg (args, gchar *);

  while (attribute != NULL)
    {
      column_id = va_arg (args, gint);
      ctk_tree_view_column_add_attribute (column, cell, attribute, column_id);
      attribute = va_arg (args, gchar *);
    }

  va_end (args);

  return ctk_tree_view_insert_column (tree_view, column, position);
}

/**
 * ctk_tree_view_insert_column_with_data_func:
 * @tree_view: a #CtkTreeView
 * @position: Position to insert, -1 for append
 * @title: column title
 * @cell: cell renderer for column
 * @func: function to set attributes of cell renderer
 * @data: data for @func
 * @dnotify: destroy notifier for @data
 *
 * Convenience function that inserts a new column into the #CtkTreeView
 * with the given cell renderer and a #CtkTreeCellDataFunc to set cell renderer
 * attributes (normally using data from the model). See also
 * ctk_tree_view_column_set_cell_data_func(), ctk_tree_view_column_pack_start().
 * If @tree_view has “fixed_height” mode enabled, then the new column will have its
 * “sizing” property set to be CTK_TREE_VIEW_COLUMN_FIXED.
 *
 * Returns: number of columns in the tree view post-insert
 **/
gint
ctk_tree_view_insert_column_with_data_func  (CtkTreeView               *tree_view,
                                             gint                       position,
                                             const gchar               *title,
                                             CtkCellRenderer           *cell,
                                             CtkTreeCellDataFunc        func,
                                             gpointer                   data,
                                             GDestroyNotify             dnotify)
{
  CtkTreeViewColumn *column;

  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), -1);

  column = ctk_tree_view_column_new ();
  if (tree_view->priv->fixed_height_mode)
    ctk_tree_view_column_set_sizing (column, CTK_TREE_VIEW_COLUMN_FIXED);

  ctk_tree_view_column_set_title (column, title);
  ctk_tree_view_column_pack_start (column, cell, TRUE);
  ctk_tree_view_column_set_cell_data_func (column, cell, func, data, dnotify);

  return ctk_tree_view_insert_column (tree_view, column, position);
}

/**
 * ctk_tree_view_get_n_columns:
 * @tree_view: a #CtkTreeView
 *
 * Queries the number of columns in the given @tree_view.
 *
 * Returns: The number of columns in the @tree_view
 *
 * Since: 3.4
 **/
guint
ctk_tree_view_get_n_columns (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), 0);

  return tree_view->priv->n_columns;
}

/**
 * ctk_tree_view_get_column:
 * @tree_view: A #CtkTreeView.
 * @n: The position of the column, counting from 0.
 *
 * Gets the #CtkTreeViewColumn at the given position in the #tree_view.
 *
 * Returns: (nullable) (transfer none): The #CtkTreeViewColumn, or %NULL if the
 * position is outside the range of columns.
 **/
CtkTreeViewColumn *
ctk_tree_view_get_column (CtkTreeView *tree_view,
			  gint         n)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), NULL);

  if (n < 0 || n >= tree_view->priv->n_columns)
    return NULL;

  if (tree_view->priv->columns == NULL)
    return NULL;

  return CTK_TREE_VIEW_COLUMN (g_list_nth (tree_view->priv->columns, n)->data);
}

/**
 * ctk_tree_view_get_columns:
 * @tree_view: A #CtkTreeView
 *
 * Returns a #GList of all the #CtkTreeViewColumn s currently in @tree_view.
 * The returned list must be freed with g_list_free ().
 *
 * Returns: (element-type CtkTreeViewColumn) (transfer container): A list of #CtkTreeViewColumn s
 **/
GList *
ctk_tree_view_get_columns (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), NULL);

  return g_list_copy (tree_view->priv->columns);
}

/**
 * ctk_tree_view_move_column_after:
 * @tree_view: A #CtkTreeView
 * @column: The #CtkTreeViewColumn to be moved.
 * @base_column: (allow-none): The #CtkTreeViewColumn to be moved relative to, or %NULL.
 *
 * Moves @column to be after to @base_column.  If @base_column is %NULL, then
 * @column is placed in the first position.
 **/
void
ctk_tree_view_move_column_after (CtkTreeView       *tree_view,
				 CtkTreeViewColumn *column,
				 CtkTreeViewColumn *base_column)
{
  GList *column_list_el, *base_el = NULL;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  column_list_el = g_list_find (tree_view->priv->columns, column);
  g_return_if_fail (column_list_el != NULL);

  if (base_column)
    {
      base_el = g_list_find (tree_view->priv->columns, base_column);
      g_return_if_fail (base_el != NULL);
    }

  if (column_list_el->prev == base_el)
    return;

  tree_view->priv->columns = g_list_remove_link (tree_view->priv->columns, column_list_el);
  if (base_el == NULL)
    {
      column_list_el->prev = NULL;
      column_list_el->next = tree_view->priv->columns;
      if (column_list_el->next)
	column_list_el->next->prev = column_list_el;
      tree_view->priv->columns = column_list_el;
    }
  else
    {
      column_list_el->prev = base_el;
      column_list_el->next = base_el->next;
      if (column_list_el->next)
	column_list_el->next->prev = column_list_el;
      base_el->next = column_list_el;
    }

  ctk_tree_view_update_button_position (tree_view, column);

  if (ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    {
      ctk_widget_queue_resize (CTK_WIDGET (tree_view));
      ctk_tree_view_size_allocate_columns (CTK_WIDGET (tree_view), NULL);
    }

  _ctk_tree_view_accessible_reorder_column (tree_view, column);

  g_signal_emit (tree_view, tree_view_signals[COLUMNS_CHANGED], 0);
}

/**
 * ctk_tree_view_set_expander_column:
 * @tree_view: A #CtkTreeView
 * @column: (nullable): %NULL, or the column to draw the expander arrow at.
 *
 * Sets the column to draw the expander arrow at. It must be in @tree_view.  
 * If @column is %NULL, then the expander arrow is always at the first 
 * visible column.
 *
 * If you do not want expander arrow to appear in your tree, set the 
 * expander column to a hidden column.
 **/
void
ctk_tree_view_set_expander_column (CtkTreeView       *tree_view,
                                   CtkTreeViewColumn *column)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (column == NULL || CTK_IS_TREE_VIEW_COLUMN (column));
  g_return_if_fail (column == NULL || ctk_tree_view_column_get_tree_view (column) == CTK_WIDGET (tree_view));

  if (tree_view->priv->expander_column != column)
    {
      tree_view->priv->expander_column = column;
      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_EXPANDER_COLUMN]);
    }
}

/**
 * ctk_tree_view_get_expander_column:
 * @tree_view: A #CtkTreeView
 *
 * Returns the column that is the current expander column.
 * This column has the expander arrow drawn next to it.
 *
 * Returns: (transfer none): The expander column.
 **/
CtkTreeViewColumn *
ctk_tree_view_get_expander_column (CtkTreeView *tree_view)
{
  GList *list;

  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), NULL);

  for (list = tree_view->priv->columns; list; list = list->next)
    if (ctk_tree_view_is_expander_column (tree_view, CTK_TREE_VIEW_COLUMN (list->data)))
      return (CtkTreeViewColumn *) list->data;
  return NULL;
}


/**
 * ctk_tree_view_set_column_drag_function:
 * @tree_view: A #CtkTreeView.
 * @func: (allow-none): A function to determine which columns are reorderable, or %NULL.
 * @user_data: (allow-none): User data to be passed to @func, or %NULL
 * @destroy: (allow-none): Destroy notifier for @user_data, or %NULL
 *
 * Sets a user function for determining where a column may be dropped when
 * dragged.  This function is called on every column pair in turn at the
 * beginning of a column drag to determine where a drop can take place.  The
 * arguments passed to @func are: the @tree_view, the #CtkTreeViewColumn being
 * dragged, the two #CtkTreeViewColumn s determining the drop spot, and
 * @user_data.  If either of the #CtkTreeViewColumn arguments for the drop spot
 * are %NULL, then they indicate an edge.  If @func is set to be %NULL, then
 * @tree_view reverts to the default behavior of allowing all columns to be
 * dropped everywhere.
 **/
void
ctk_tree_view_set_column_drag_function (CtkTreeView               *tree_view,
					CtkTreeViewColumnDropFunc  func,
					gpointer                   user_data,
					GDestroyNotify             destroy)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  if (tree_view->priv->column_drop_func_data_destroy)
    tree_view->priv->column_drop_func_data_destroy (tree_view->priv->column_drop_func_data);

  tree_view->priv->column_drop_func = func;
  tree_view->priv->column_drop_func_data = user_data;
  tree_view->priv->column_drop_func_data_destroy = destroy;
}

/**
 * ctk_tree_view_scroll_to_point:
 * @tree_view: a #CtkTreeView
 * @tree_x: X coordinate of new top-left pixel of visible area, or -1
 * @tree_y: Y coordinate of new top-left pixel of visible area, or -1
 *
 * Scrolls the tree view such that the top-left corner of the visible
 * area is @tree_x, @tree_y, where @tree_x and @tree_y are specified
 * in tree coordinates.  The @tree_view must be realized before
 * this function is called.  If it isn't, you probably want to be
 * using ctk_tree_view_scroll_to_cell().
 *
 * If either @tree_x or @tree_y are -1, then that direction isn’t scrolled.
 **/
void
ctk_tree_view_scroll_to_point (CtkTreeView *tree_view,
                               gint         tree_x,
                               gint         tree_y)
{
  CtkAdjustment *hadj;
  CtkAdjustment *vadj;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (ctk_widget_get_realized (CTK_WIDGET (tree_view)));

  hadj = tree_view->priv->hadjustment;
  vadj = tree_view->priv->vadjustment;

  if (tree_x != -1)
    ctk_adjustment_animate_to_value (hadj, tree_x);
  if (tree_y != -1)
    ctk_adjustment_animate_to_value (vadj, tree_y);
}

/**
 * ctk_tree_view_scroll_to_cell:
 * @tree_view: A #CtkTreeView.
 * @path: (allow-none): The path of the row to move to, or %NULL.
 * @column: (allow-none): The #CtkTreeViewColumn to move horizontally to, or %NULL.
 * @use_align: whether to use alignment arguments, or %FALSE.
 * @row_align: The vertical alignment of the row specified by @path.
 * @col_align: The horizontal alignment of the column specified by @column.
 *
 * Moves the alignments of @tree_view to the position specified by @column and
 * @path.  If @column is %NULL, then no horizontal scrolling occurs.  Likewise,
 * if @path is %NULL no vertical scrolling occurs.  At a minimum, one of @column
 * or @path need to be non-%NULL.  @row_align determines where the row is
 * placed, and @col_align determines where @column is placed.  Both are expected
 * to be between 0.0 and 1.0. 0.0 means left/top alignment, 1.0 means
 * right/bottom alignment, 0.5 means center.
 *
 * If @use_align is %FALSE, then the alignment arguments are ignored, and the
 * tree does the minimum amount of work to scroll the cell onto the screen.
 * This means that the cell will be scrolled to the edge closest to its current
 * position.  If the cell is currently visible on the screen, nothing is done.
 *
 * This function only works if the model is set, and @path is a valid row on the
 * model.  If the model changes before the @tree_view is realized, the centered
 * path will be modified to reflect this change.
 **/
void
ctk_tree_view_scroll_to_cell (CtkTreeView       *tree_view,
                              CtkTreePath       *path,
                              CtkTreeViewColumn *column,
			      gboolean           use_align,
                              gfloat             row_align,
                              gfloat             col_align)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (tree_view->priv->model != NULL);
  g_return_if_fail (tree_view->priv->tree != NULL);
  g_return_if_fail (row_align >= 0.0 && row_align <= 1.0);
  g_return_if_fail (col_align >= 0.0 && col_align <= 1.0);
  g_return_if_fail (path != NULL || column != NULL);

  row_align = CLAMP (row_align, 0.0, 1.0);
  col_align = CLAMP (col_align, 0.0, 1.0);


  /* Note: Despite the benefits that come from having one code path for the
   * scrolling code, we short-circuit validate_visible_area's immplementation as
   * it is much slower than just going to the point.
   */
  if (!ctk_widget_get_visible (CTK_WIDGET (tree_view)) ||
      !ctk_widget_get_realized (CTK_WIDGET (tree_view)) ||
      _ctk_widget_get_alloc_needed (CTK_WIDGET (tree_view)) ||
      CTK_RBNODE_FLAG_SET (tree_view->priv->tree->root, CTK_RBNODE_DESCENDANTS_INVALID))
    {
      if (tree_view->priv->scroll_to_path)
	ctk_tree_row_reference_free (tree_view->priv->scroll_to_path);

      tree_view->priv->scroll_to_path = NULL;
      tree_view->priv->scroll_to_column = NULL;

      if (path)
	tree_view->priv->scroll_to_path = ctk_tree_row_reference_new_proxy (G_OBJECT (tree_view), tree_view->priv->model, path);
      if (column)
	tree_view->priv->scroll_to_column = column;
      tree_view->priv->scroll_to_use_align = use_align;
      tree_view->priv->scroll_to_row_align = row_align;
      tree_view->priv->scroll_to_col_align = col_align;

      install_presize_handler (tree_view);
    }
  else
    {
      CdkRectangle cell_rect;
      CdkRectangle vis_rect;
      gint dest_x, dest_y;

      ctk_tree_view_get_background_area (tree_view, path, column, &cell_rect);
      ctk_tree_view_get_visible_rect (tree_view, &vis_rect);

      cell_rect.y = TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, cell_rect.y);

      dest_x = vis_rect.x;
      dest_y = vis_rect.y;

      if (column)
	{
	  if (use_align)
	    {
	      dest_x = cell_rect.x - ((vis_rect.width - cell_rect.width) * col_align);
	    }
	  else
	    {
	      if (cell_rect.x < vis_rect.x)
		dest_x = cell_rect.x;
	      if (cell_rect.x + cell_rect.width > vis_rect.x + vis_rect.width)
		dest_x = cell_rect.x + cell_rect.width - vis_rect.width;
	    }
	}

      if (path)
	{
	  if (use_align)
	    {
	      dest_y = cell_rect.y - ((vis_rect.height - cell_rect.height) * row_align);
	      dest_y = MAX (dest_y, 0);
	    }
	  else
	    {
	      if (cell_rect.y < vis_rect.y)
		dest_y = cell_rect.y;
	      if (cell_rect.y + cell_rect.height > vis_rect.y + vis_rect.height)
		dest_y = cell_rect.y + cell_rect.height - vis_rect.height;
	    }
	}

      ctk_tree_view_scroll_to_point (tree_view, dest_x, dest_y);
    }
}

/**
 * ctk_tree_view_row_activated:
 * @tree_view: A #CtkTreeView
 * @path: The #CtkTreePath to be activated.
 * @column: The #CtkTreeViewColumn to be activated.
 *
 * Activates the cell determined by @path and @column.
 **/
void
ctk_tree_view_row_activated (CtkTreeView       *tree_view,
			     CtkTreePath       *path,
			     CtkTreeViewColumn *column)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  g_signal_emit (tree_view, tree_view_signals[ROW_ACTIVATED], 0, path, column);
}


static void
ctk_tree_view_expand_all_emission_helper (CtkRBTree *tree,
                                          CtkRBNode *node,
                                          gpointer   data)
{
  CtkTreeView *tree_view = data;

  if ((node->flags & CTK_RBNODE_IS_PARENT) == CTK_RBNODE_IS_PARENT &&
      node->children)
    {
      CtkTreePath *path;
      CtkTreeIter iter;

      path = _ctk_tree_path_new_from_rbtree (tree, node);
      ctk_tree_model_get_iter (tree_view->priv->model, &iter, path);

      g_signal_emit (tree_view, tree_view_signals[ROW_EXPANDED], 0, &iter, path);

      ctk_tree_path_free (path);
    }

  if (node->children)
    _ctk_rbtree_traverse (node->children,
                          node->children->root,
                          G_PRE_ORDER,
                          ctk_tree_view_expand_all_emission_helper,
                          tree_view);
}

/**
 * ctk_tree_view_expand_all:
 * @tree_view: A #CtkTreeView.
 *
 * Recursively expands all nodes in the @tree_view.
 **/
void
ctk_tree_view_expand_all (CtkTreeView *tree_view)
{
  CtkTreePath *path;
  CtkRBTree *tree;
  CtkRBNode *node;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  if (tree_view->priv->tree == NULL)
    return;

  path = ctk_tree_path_new_first ();
  _ctk_tree_view_find_node (tree_view, path, &tree, &node);

  while (node)
    {
      ctk_tree_view_real_expand_row (tree_view, path, tree, node, TRUE, FALSE);
      node = _ctk_rbtree_next (tree, node);
      ctk_tree_path_next (path);
  }

  ctk_tree_path_free (path);
}

/**
 * ctk_tree_view_collapse_all:
 * @tree_view: A #CtkTreeView.
 *
 * Recursively collapses all visible, expanded nodes in @tree_view.
 **/
void
ctk_tree_view_collapse_all (CtkTreeView *tree_view)
{
  CtkRBTree *tree;
  CtkRBNode *node;
  CtkTreePath *path;
  gint *indices;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  if (tree_view->priv->tree == NULL)
    return;

  path = ctk_tree_path_new ();
  ctk_tree_path_down (path);
  indices = ctk_tree_path_get_indices (path);

  tree = tree_view->priv->tree;
  node = _ctk_rbtree_first (tree);

  while (node)
    {
      if (node->children)
	ctk_tree_view_real_collapse_row (tree_view, path, tree, node, FALSE);
      indices[0]++;
      node = _ctk_rbtree_next (tree, node);
    }

  ctk_tree_path_free (path);
}

/**
 * ctk_tree_view_expand_to_path:
 * @tree_view: A #CtkTreeView.
 * @path: path to a row.
 *
 * Expands the row at @path. This will also expand all parent rows of
 * @path as necessary.
 *
 * Since: 2.2
 **/
void
ctk_tree_view_expand_to_path (CtkTreeView *tree_view,
			      CtkTreePath *path)
{
  gint i, depth;
  gint *indices;
  CtkTreePath *tmp;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (path != NULL);

  depth = ctk_tree_path_get_depth (path);
  indices = ctk_tree_path_get_indices (path);

  tmp = ctk_tree_path_new ();
  g_return_if_fail (tmp != NULL);

  for (i = 0; i < depth; i++)
    {
      ctk_tree_path_append_index (tmp, indices[i]);
      ctk_tree_view_expand_row (tree_view, tmp, FALSE);
    }

  ctk_tree_path_free (tmp);
}

/* FIXME the bool return values for expand_row and collapse_row are
 * not analagous; they should be TRUE if the row had children and
 * was not already in the requested state.
 */


static gboolean
ctk_tree_view_real_expand_row (CtkTreeView *tree_view,
			       CtkTreePath *path,
			       CtkRBTree   *tree,
			       CtkRBNode   *node,
			       gboolean     open_all,
			       gboolean     animate)
{
  CtkTreeIter iter;
  CtkTreeIter temp;
  gboolean expand;

  if (animate)
    animate = ctk_settings_get_enable_animations (ctk_widget_get_settings (CTK_WIDGET (tree_view)));

  remove_auto_expand_timeout (tree_view);

  if (node->children && !open_all)
    return FALSE;

  if (! CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_PARENT))
    return FALSE;

  ctk_tree_model_get_iter (tree_view->priv->model, &iter, path);
  if (! ctk_tree_model_iter_has_child (tree_view->priv->model, &iter))
    return FALSE;


   if (node->children && open_all)
    {
      gboolean retval = FALSE;
      CtkTreePath *tmp_path = ctk_tree_path_copy (path);

      ctk_tree_path_append_index (tmp_path, 0);
      tree = node->children;
      node = _ctk_rbtree_first (tree);
      /* try to expand the children */
      do
        {
         gboolean t;
	 t = ctk_tree_view_real_expand_row (tree_view, tmp_path, tree, node,
					    TRUE, animate);
         if (t)
           retval = TRUE;

         ctk_tree_path_next (tmp_path);
	 node = _ctk_rbtree_next (tree, node);
       }
      while (node != NULL);

      ctk_tree_path_free (tmp_path);

      return retval;
    }

  g_signal_emit (tree_view, tree_view_signals[TEST_EXPAND_ROW], 0, &iter, path, &expand);

  if (!ctk_tree_model_iter_has_child (tree_view->priv->model, &iter))
    return FALSE;

  if (expand)
    return FALSE;

  node->children = _ctk_rbtree_new ();
  node->children->parent_tree = tree;
  node->children->parent_node = node;

  ctk_tree_model_iter_children (tree_view->priv->model, &temp, &iter);

  ctk_tree_view_build_tree (tree_view,
			    node->children,
			    &temp,
			    ctk_tree_path_get_depth (path) + 1,
			    open_all);

  _ctk_tree_view_accessible_add (tree_view, node->children, NULL);
  _ctk_tree_view_accessible_add_state (tree_view,
                                       tree, node,
                                       CTK_CELL_RENDERER_EXPANDED);

  install_presize_handler (tree_view);

  g_signal_emit (tree_view, tree_view_signals[ROW_EXPANDED], 0, &iter, path);
  if (open_all && node->children)
    {
      _ctk_rbtree_traverse (node->children,
                            node->children->root,
                            G_PRE_ORDER,
                            ctk_tree_view_expand_all_emission_helper,
                            tree_view);
    }
  return TRUE;
}


/**
 * ctk_tree_view_expand_row:
 * @tree_view: a #CtkTreeView
 * @path: path to a row
 * @open_all: whether to recursively expand, or just expand immediate children
 *
 * Opens the row so its children are visible.
 *
 * Returns: %TRUE if the row existed and had children
 **/
gboolean
ctk_tree_view_expand_row (CtkTreeView *tree_view,
			  CtkTreePath *path,
			  gboolean     open_all)
{
  CtkRBTree *tree;
  CtkRBNode *node;

  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);
  g_return_val_if_fail (tree_view->priv->model != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  if (_ctk_tree_view_find_node (tree_view,
				path,
				&tree,
				&node))
    return FALSE;

  if (tree != NULL)
    return ctk_tree_view_real_expand_row (tree_view, path, tree, node, open_all, FALSE);
  else
    return FALSE;
}

static gboolean
ctk_tree_view_real_collapse_row (CtkTreeView *tree_view,
				 CtkTreePath *path,
				 CtkRBTree   *tree,
				 CtkRBNode   *node,
				 gboolean     animate)
{
  CtkTreeIter iter;
  CtkTreeIter children;
  gboolean collapse;
  GList *list;
  gboolean selection_changed, cursor_changed;

  if (animate)
    animate = ctk_settings_get_enable_animations (ctk_widget_get_settings (CTK_WIDGET (tree_view)));

  remove_auto_expand_timeout (tree_view);

  if (node->children == NULL)
    return FALSE;
  ctk_tree_model_get_iter (tree_view->priv->model, &iter, path);

  g_signal_emit (tree_view, tree_view_signals[TEST_COLLAPSE_ROW], 0, &iter, path, &collapse);

  if (collapse)
    return FALSE;

  /* if the prelighted node is a child of us, we want to unprelight it.  We have
   * a chance to prelight the correct node below */

  if (tree_view->priv->prelight_tree)
    {
      CtkRBTree *parent_tree;
      CtkRBNode *parent_node;

      parent_tree = tree_view->priv->prelight_tree->parent_tree;
      parent_node = tree_view->priv->prelight_tree->parent_node;
      while (parent_tree)
	{
	  if (parent_tree == tree && parent_node == node)
	    {
	      ensure_unprelighted (tree_view);
	      break;
	    }
	  parent_node = parent_tree->parent_node;
	  parent_tree = parent_tree->parent_tree;
	}
    }

  TREE_VIEW_INTERNAL_ASSERT (ctk_tree_model_iter_children (tree_view->priv->model, &children, &iter), FALSE);

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      CtkTreeViewColumn *column = list->data;

      if (ctk_tree_view_column_get_visible (column) == FALSE)
	continue;
      if (ctk_tree_view_column_get_sizing (column) == CTK_TREE_VIEW_COLUMN_AUTOSIZE)
	_ctk_tree_view_column_cell_set_dirty (column, TRUE);
    }

  if (tree_view->priv->destroy_count_func)
    {
      CtkTreePath *child_path;
      gint child_count = 0;
      child_path = ctk_tree_path_copy (path);
      ctk_tree_path_down (child_path);
      if (node->children)
	_ctk_rbtree_traverse (node->children, node->children->root, G_POST_ORDER, count_children_helper, &child_count);
      tree_view->priv->destroy_count_func (tree_view, child_path, child_count, tree_view->priv->destroy_count_data);
      ctk_tree_path_free (child_path);
    }

  if (tree_view->priv->cursor_node)
    {
      cursor_changed = (node->children == tree_view->priv->cursor_tree)
                       || _ctk_rbtree_contains (node->children, tree_view->priv->cursor_tree);
    }
  else
    cursor_changed = FALSE;

  if (ctk_tree_row_reference_valid (tree_view->priv->anchor))
    {
      CtkTreePath *anchor_path = ctk_tree_row_reference_get_path (tree_view->priv->anchor);
      if (ctk_tree_path_is_ancestor (path, anchor_path))
	{
	  ctk_tree_row_reference_free (tree_view->priv->anchor);
	  tree_view->priv->anchor = NULL;
	}
      ctk_tree_path_free (anchor_path);
    }

  selection_changed = ctk_tree_view_unref_and_check_selection_tree (tree_view, node->children);
  
  /* Stop a pending double click */
  ctk_event_controller_reset (CTK_EVENT_CONTROLLER (tree_view->priv->multipress_gesture));

  _ctk_tree_view_accessible_remove (tree_view, node->children, NULL);
  _ctk_tree_view_accessible_remove_state (tree_view,
                                          tree, node,
                                          CTK_CELL_RENDERER_EXPANDED);

  _ctk_rbtree_remove (node->children);

  if (cursor_changed)
    ctk_tree_view_real_set_cursor (tree_view, path, CLEAR_AND_SELECT | CURSOR_INVALID);
  if (selection_changed)
    g_signal_emit_by_name (tree_view->priv->selection, "changed");

  if (ctk_widget_get_mapped (CTK_WIDGET (tree_view)))
    {
      ctk_widget_queue_resize (CTK_WIDGET (tree_view));
    }

  g_signal_emit (tree_view, tree_view_signals[ROW_COLLAPSED], 0, &iter, path);
  
  if (ctk_widget_get_mapped (CTK_WIDGET (tree_view)))
    update_prelight (tree_view,
                     tree_view->priv->event_last_x,
                     tree_view->priv->event_last_y);

  return TRUE;
}

/**
 * ctk_tree_view_collapse_row:
 * @tree_view: a #CtkTreeView
 * @path: path to a row in the @tree_view
 *
 * Collapses a row (hides its child rows, if they exist).
 *
 * Returns: %TRUE if the row was collapsed.
 **/
gboolean
ctk_tree_view_collapse_row (CtkTreeView *tree_view,
			    CtkTreePath *path)
{
  CtkRBTree *tree;
  CtkRBNode *node;

  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);
  g_return_val_if_fail (tree_view->priv->tree != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  if (_ctk_tree_view_find_node (tree_view,
				path,
				&tree,
				&node))
    return FALSE;

  if (tree == NULL || node->children == NULL)
    return FALSE;

  return ctk_tree_view_real_collapse_row (tree_view, path, tree, node, FALSE);
}

static void
ctk_tree_view_map_expanded_rows_helper (CtkTreeView            *tree_view,
					CtkRBTree              *tree,
					CtkTreePath            *path,
					CtkTreeViewMappingFunc  func,
					gpointer                user_data)
{
  CtkRBNode *node;

  if (tree == NULL || tree->root == NULL)
    return;

  node = _ctk_rbtree_first (tree);

  while (node)
    {
      if (node->children)
	{
	  (* func) (tree_view, path, user_data);
	  ctk_tree_path_down (path);
	  ctk_tree_view_map_expanded_rows_helper (tree_view, node->children, path, func, user_data);
	  ctk_tree_path_up (path);
	}
      ctk_tree_path_next (path);
      node = _ctk_rbtree_next (tree, node);
    }
}

/**
 * ctk_tree_view_map_expanded_rows:
 * @tree_view: A #CtkTreeView
 * @func: (scope call): A function to be called
 * @data: User data to be passed to the function.
 *
 * Calls @func on all expanded rows.
 **/
void
ctk_tree_view_map_expanded_rows (CtkTreeView            *tree_view,
				 CtkTreeViewMappingFunc  func,
				 gpointer                user_data)
{
  CtkTreePath *path;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (func != NULL);

  path = ctk_tree_path_new_first ();

  ctk_tree_view_map_expanded_rows_helper (tree_view,
					  tree_view->priv->tree,
					  path, func, user_data);

  ctk_tree_path_free (path);
}

/**
 * ctk_tree_view_row_expanded:
 * @tree_view: A #CtkTreeView.
 * @path: A #CtkTreePath to test expansion state.
 *
 * Returns %TRUE if the node pointed to by @path is expanded in @tree_view.
 *
 * Returns: %TRUE if #path is expanded.
 **/
gboolean
ctk_tree_view_row_expanded (CtkTreeView *tree_view,
			    CtkTreePath *path)
{
  CtkRBTree *tree;
  CtkRBNode *node;

  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  _ctk_tree_view_find_node (tree_view, path, &tree, &node);

  if (node == NULL)
    return FALSE;

  return (node->children != NULL);
}

/**
 * ctk_tree_view_get_reorderable:
 * @tree_view: a #CtkTreeView
 *
 * Retrieves whether the user can reorder the tree via drag-and-drop. See
 * ctk_tree_view_set_reorderable().
 *
 * Returns: %TRUE if the tree can be reordered.
 **/
gboolean
ctk_tree_view_get_reorderable (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);

  return tree_view->priv->reorderable;
}

/**
 * ctk_tree_view_set_reorderable:
 * @tree_view: A #CtkTreeView.
 * @reorderable: %TRUE, if the tree can be reordered.
 *
 * This function is a convenience function to allow you to reorder
 * models that support the #CtkTreeDragSourceIface and the
 * #CtkTreeDragDestIface.  Both #CtkTreeStore and #CtkListStore support
 * these.  If @reorderable is %TRUE, then the user can reorder the
 * model by dragging and dropping rows. The developer can listen to
 * these changes by connecting to the model’s #CtkTreeModel::row-inserted
 * and #CtkTreeModel::row-deleted signals. The reordering is implemented
 * by setting up the tree view as a drag source and destination.
 * Therefore, drag and drop can not be used in a reorderable view for any
 * other purpose.
 *
 * This function does not give you any degree of control over the order -- any
 * reordering is allowed.  If more control is needed, you should probably
 * handle drag and drop manually.
 **/
void
ctk_tree_view_set_reorderable (CtkTreeView *tree_view,
			       gboolean     reorderable)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  reorderable = reorderable != FALSE;

  if (tree_view->priv->reorderable == reorderable)
    return;

  if (reorderable)
    {
      const CtkTargetEntry row_targets[] = {
        { "CTK_TREE_MODEL_ROW", CTK_TARGET_SAME_WIDGET, 0 }
      };

      ctk_tree_view_enable_model_drag_source (tree_view,
					      CDK_BUTTON1_MASK,
					      row_targets,
					      G_N_ELEMENTS (row_targets),
					      CDK_ACTION_MOVE);
      ctk_tree_view_enable_model_drag_dest (tree_view,
					    row_targets,
					    G_N_ELEMENTS (row_targets),
					    CDK_ACTION_MOVE);
    }
  else
    {
      ctk_tree_view_unset_rows_drag_source (tree_view);
      ctk_tree_view_unset_rows_drag_dest (tree_view);
    }

  tree_view->priv->reorderable = reorderable;

  g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_REORDERABLE]);
}

static void
ctk_tree_view_real_set_cursor (CtkTreeView     *tree_view,
			       CtkTreePath     *path,
                               SetCursorFlags   flags)
{
  if (!(flags & CURSOR_INVALID) && tree_view->priv->cursor_node)
    {
      _ctk_tree_view_accessible_remove_state (tree_view,
                                              tree_view->priv->cursor_tree,
                                              tree_view->priv->cursor_node,
                                              CTK_CELL_RENDERER_FOCUSED);
      _ctk_tree_view_queue_draw_node (tree_view,
                                      tree_view->priv->cursor_tree,
                                      tree_view->priv->cursor_node,
                                      NULL);
    }

  /* One cannot set the cursor on a separator.   Also, if
   * _ctk_tree_view_find_node returns TRUE, it ran out of tree
   * before finding the tree and node belonging to path.  The
   * path maps to a non-existing path and we will silently bail out.
   * We unset tree and node to avoid further processing.
   */
  if (path == NULL || 
      row_is_separator (tree_view, NULL, path)
      || _ctk_tree_view_find_node (tree_view,
                                   path,
                                   &tree_view->priv->cursor_tree,
                                   &tree_view->priv->cursor_node))
    {
      tree_view->priv->cursor_tree = NULL;
      tree_view->priv->cursor_node = NULL;
    }

  if (tree_view->priv->cursor_node != NULL)
    {
      CtkRBTree *new_tree = NULL;
      CtkRBNode *new_node = NULL;

      if ((flags & CLEAR_AND_SELECT) && !tree_view->priv->modify_selection_pressed)
        {
          CtkTreeSelectMode mode = 0;

          if (tree_view->priv->extend_selection_pressed)
            mode |= CTK_TREE_SELECT_MODE_EXTEND;

          _ctk_tree_selection_internal_select_node (tree_view->priv->selection,
                                                    tree_view->priv->cursor_node,
                                                    tree_view->priv->cursor_tree,
                                                    path,
                                                    mode,
                                                    FALSE);
        }

      /* We have to re-find tree and node here again, somebody might have
       * cleared the node or the whole tree in the CtkTreeSelection::changed
       * callback. If the nodes differ we bail out here.
       */
      _ctk_tree_view_find_node (tree_view, path, &new_tree, &new_node);

      if (tree_view->priv->cursor_node == NULL ||
          tree_view->priv->cursor_node != new_node)
        return;

      if (flags & CLAMP_NODE)
        {
	  ctk_tree_view_clamp_node_visible (tree_view,
                                            tree_view->priv->cursor_tree,
                                            tree_view->priv->cursor_node);
	  _ctk_tree_view_queue_draw_node (tree_view,
                                          tree_view->priv->cursor_tree,
                                          tree_view->priv->cursor_node,
                                          NULL);
	}

      _ctk_tree_view_accessible_add_state (tree_view,
                                           tree_view->priv->cursor_tree,
                                           tree_view->priv->cursor_node,
                                           CTK_CELL_RENDERER_FOCUSED);
    }

  if (!ctk_widget_in_destruction (CTK_WIDGET (tree_view)))
    g_signal_emit (tree_view, tree_view_signals[CURSOR_CHANGED], 0);
}

/**
 * ctk_tree_view_get_cursor:
 * @tree_view: A #CtkTreeView
 * @path: (out) (transfer full) (optional) (nullable): A pointer to be
 *   filled with the current cursor path, or %NULL
 * @focus_column: (out) (transfer none) (optional) (nullable): A
 *   pointer to be filled with the current focus column, or %NULL
 *
 * Fills in @path and @focus_column with the current path and focus column.  If
 * the cursor isn’t currently set, then *@path will be %NULL.  If no column
 * currently has focus, then *@focus_column will be %NULL.
 *
 * The returned #CtkTreePath must be freed with ctk_tree_path_free() when
 * you are done with it.
 **/
void
ctk_tree_view_get_cursor (CtkTreeView        *tree_view,
			  CtkTreePath       **path,
			  CtkTreeViewColumn **focus_column)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  if (path)
    {
      if (tree_view->priv->cursor_node)
        *path = _ctk_tree_path_new_from_rbtree (tree_view->priv->cursor_tree,
                                                tree_view->priv->cursor_node);
      else
	*path = NULL;
    }

  if (focus_column)
    {
      *focus_column = tree_view->priv->focus_column;
    }
}

/**
 * ctk_tree_view_set_cursor:
 * @tree_view: A #CtkTreeView
 * @path: A #CtkTreePath
 * @focus_column: (allow-none): A #CtkTreeViewColumn, or %NULL
 * @start_editing: %TRUE if the specified cell should start being edited.
 *
 * Sets the current keyboard focus to be at @path, and selects it.  This is
 * useful when you want to focus the user’s attention on a particular row.  If
 * @focus_column is not %NULL, then focus is given to the column specified by 
 * it. Additionally, if @focus_column is specified, and @start_editing is 
 * %TRUE, then editing should be started in the specified cell.  
 * This function is often followed by @ctk_widget_grab_focus (@tree_view) 
 * in order to give keyboard focus to the widget.  Please note that editing 
 * can only happen when the widget is realized.
 *
 * If @path is invalid for @model, the current cursor (if any) will be unset
 * and the function will return without failing.
 **/
void
ctk_tree_view_set_cursor (CtkTreeView       *tree_view,
			  CtkTreePath       *path,
			  CtkTreeViewColumn *focus_column,
			  gboolean           start_editing)
{
  ctk_tree_view_set_cursor_on_cell (tree_view, path, focus_column,
				    NULL, start_editing);
}

/**
 * ctk_tree_view_set_cursor_on_cell:
 * @tree_view: A #CtkTreeView
 * @path: A #CtkTreePath
 * @focus_column: (allow-none): A #CtkTreeViewColumn, or %NULL
 * @focus_cell: (allow-none): A #CtkCellRenderer, or %NULL
 * @start_editing: %TRUE if the specified cell should start being edited.
 *
 * Sets the current keyboard focus to be at @path, and selects it.  This is
 * useful when you want to focus the user’s attention on a particular row.  If
 * @focus_column is not %NULL, then focus is given to the column specified by
 * it. If @focus_column and @focus_cell are not %NULL, and @focus_column
 * contains 2 or more editable or activatable cells, then focus is given to
 * the cell specified by @focus_cell. Additionally, if @focus_column is
 * specified, and @start_editing is %TRUE, then editing should be started in
 * the specified cell.  This function is often followed by
 * @ctk_widget_grab_focus (@tree_view) in order to give keyboard focus to the
 * widget.  Please note that editing can only happen when the widget is
 * realized.
 *
 * If @path is invalid for @model, the current cursor (if any) will be unset
 * and the function will return without failing.
 *
 * Since: 2.2
 **/
void
ctk_tree_view_set_cursor_on_cell (CtkTreeView       *tree_view,
				  CtkTreePath       *path,
				  CtkTreeViewColumn *focus_column,
				  CtkCellRenderer   *focus_cell,
				  gboolean           start_editing)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (path != NULL);
  g_return_if_fail (focus_column == NULL || CTK_IS_TREE_VIEW_COLUMN (focus_column));

  if (!tree_view->priv->model)
    return;

  if (focus_cell)
    {
      g_return_if_fail (focus_column);
      g_return_if_fail (CTK_IS_CELL_RENDERER (focus_cell));
    }

  /* cancel the current editing, if it exists */
  if (tree_view->priv->edited_column &&
      ctk_cell_area_get_edit_widget
      (ctk_cell_layout_get_area (CTK_CELL_LAYOUT (tree_view->priv->edited_column))))
    ctk_tree_view_stop_editing (tree_view, TRUE);

  ctk_tree_view_real_set_cursor (tree_view, path, CLEAR_AND_SELECT | CLAMP_NODE);

  if (focus_column &&
      ctk_tree_view_column_get_visible (focus_column))
    {
      GList *list;
      gboolean column_in_tree = FALSE;

      for (list = tree_view->priv->columns; list; list = list->next)
	if (list->data == focus_column)
	  {
	    column_in_tree = TRUE;
	    break;
	  }
      g_return_if_fail (column_in_tree);
      _ctk_tree_view_set_focus_column (tree_view, focus_column);
      if (focus_cell)
	ctk_tree_view_column_focus_cell (focus_column, focus_cell);
      if (start_editing)
	ctk_tree_view_start_editing (tree_view, path, TRUE);
    }
}

/**
 * ctk_tree_view_get_bin_window:
 * @tree_view: A #CtkTreeView
 *
 * Returns the window that @tree_view renders to.
 * This is used primarily to compare to `event->window`
 * to confirm that the event on @tree_view is on the right window.
 *
 * Returns: (nullable) (transfer none): A #CdkWindow, or %NULL when @tree_view
 * hasn’t been realized yet.
 **/
CdkWindow *
ctk_tree_view_get_bin_window (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), NULL);

  return tree_view->priv->bin_window;
}

/**
 * ctk_tree_view_get_path_at_pos:
 * @tree_view: A #CtkTreeView.
 * @x: The x position to be identified (relative to bin_window).
 * @y: The y position to be identified (relative to bin_window).
 * @path: (out) (optional) (nullable): A pointer to a #CtkTreePath
 *   pointer to be filled in, or %NULL
 * @column: (out) (transfer none) (optional) (nullable): A pointer to
 *   a #CtkTreeViewColumn pointer to be filled in, or %NULL
 * @cell_x: (out) (optional): A pointer where the X coordinate
 *   relative to the cell can be placed, or %NULL
 * @cell_y: (out) (optional): A pointer where the Y coordinate
 *   relative to the cell can be placed, or %NULL
 *
 * Finds the path at the point (@x, @y), relative to bin_window coordinates
 * (please see ctk_tree_view_get_bin_window()).
 * That is, @x and @y are relative to an events coordinates. @x and @y must
 * come from an event on the @tree_view only where `event->window ==
 * ctk_tree_view_get_bin_window ()`. It is primarily for
 * things like popup menus. If @path is non-%NULL, then it will be filled
 * with the #CtkTreePath at that point.  This path should be freed with
 * ctk_tree_path_free().  If @column is non-%NULL, then it will be filled
 * with the column at that point.  @cell_x and @cell_y return the coordinates
 * relative to the cell background (i.e. the @background_area passed to
 * ctk_cell_renderer_render()).  This function is only meaningful if
 * @tree_view is realized.  Therefore this function will always return %FALSE
 * if @tree_view is not realized or does not have a model.
 *
 * For converting widget coordinates (eg. the ones you get from
 * CtkWidget::query-tooltip), please see
 * ctk_tree_view_convert_widget_to_bin_window_coords().
 *
 * Returns: %TRUE if a row exists at that coordinate.
 **/
gboolean
ctk_tree_view_get_path_at_pos (CtkTreeView        *tree_view,
			       gint                x,
			       gint                y,
			       CtkTreePath       **path,
			       CtkTreeViewColumn **column,
                               gint               *cell_x,
                               gint               *cell_y)
{
  CtkRBTree *tree;
  CtkRBNode *node;
  gint y_offset;

  g_return_val_if_fail (tree_view != NULL, FALSE);

  if (path)
    *path = NULL;
  if (column)
    *column = NULL;

  if (tree_view->priv->bin_window == NULL)
    return FALSE;

  if (tree_view->priv->tree == NULL)
    return FALSE;

  if (x > ctk_adjustment_get_upper (tree_view->priv->hadjustment))
    return FALSE;

  if (x < 0 || y < 0)
    return FALSE;

  if (column || cell_x)
    {
      CtkTreeViewColumn *tmp_column;
      CtkTreeViewColumn *last_column = NULL;
      GList *list;
      gint remaining_x = x;
      gboolean found = FALSE;
      gboolean rtl;
      gint width;

      rtl = (ctk_widget_get_direction (CTK_WIDGET (tree_view)) == CTK_TEXT_DIR_RTL);
      for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
	   list;
	   list = (rtl ? list->prev : list->next))
	{
	  tmp_column = list->data;

	  if (ctk_tree_view_column_get_visible (tmp_column) == FALSE)
	    continue;

	  last_column = tmp_column;
          width = ctk_tree_view_column_get_width (tmp_column);
	  if (remaining_x < width)
	    {
              found = TRUE;

              if (column)
                *column = tmp_column;

              if (cell_x)
                *cell_x = remaining_x;

	      break;
	    }
	  remaining_x -= width;
	}

      /* If found is FALSE and there is a last_column, then it the remainder
       * space is in that area
       */
      if (!found)
        {
	  if (last_column)
	    {
	      if (column)
		*column = last_column;
	      
	      if (cell_x)
		*cell_x = ctk_tree_view_column_get_width (last_column) + remaining_x;
	    }
	  else
	    {
	      return FALSE;
	    }
	}
    }

  y_offset = _ctk_rbtree_find_offset (tree_view->priv->tree,
				      TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, y),
				      &tree, &node);

  if (tree == NULL)
    return FALSE;

  if (cell_y)
    *cell_y = y_offset;

  if (path)
    *path = _ctk_tree_path_new_from_rbtree (tree, node);

  return TRUE;
}


static inline gint
ctk_tree_view_get_cell_area_height (CtkTreeView *tree_view,
                                    CtkRBNode   *node,
                                    gint         vertical_separator)
{
  int expander_size = ctk_tree_view_get_expander_size (tree_view);
  int height;

  /* The "cell" areas are the cell_area passed in to ctk_cell_renderer_render(),
   * i.e. just the cells, no spacing.
   *
   * The cell area height is at least expander_size - vertical_separator.
   * For regular nodes, the height is then at least expander_size. We should
   * be able to enforce the expander_size minimum here, because this
   * function will not be called for irregular (e.g. separator) rows.
   */
  height = ctk_tree_view_get_row_height (tree_view, node);
  if (height < expander_size)
    height = expander_size;

  return height - vertical_separator;
}

static inline gint
ctk_tree_view_get_cell_area_y_offset (CtkTreeView *tree_view,
                                      CtkRBTree   *tree,
                                      CtkRBNode   *node,
                                      gint         vertical_separator)
{
  int offset;

  offset = ctk_tree_view_get_row_y_offset (tree_view, tree, node);
  offset += vertical_separator / 2;

  return offset;
}

/**
 * ctk_tree_view_get_cell_area:
 * @tree_view: a #CtkTreeView
 * @path: (allow-none): a #CtkTreePath for the row, or %NULL to get only horizontal coordinates
 * @column: (allow-none): a #CtkTreeViewColumn for the column, or %NULL to get only vertical coordinates
 * @rect: (out): rectangle to fill with cell rect
 *
 * Fills the bounding rectangle in bin_window coordinates for the cell at the
 * row specified by @path and the column specified by @column.  If @path is
 * %NULL, or points to a path not currently displayed, the @y and @height fields
 * of the rectangle will be filled with 0. If @column is %NULL, the @x and @width
 * fields will be filled with 0.  The sum of all cell rects does not cover the
 * entire tree; there are extra pixels in between rows, for example. The
 * returned rectangle is equivalent to the @cell_area passed to
 * ctk_cell_renderer_render().  This function is only valid if @tree_view is
 * realized.
 **/
void
ctk_tree_view_get_cell_area (CtkTreeView        *tree_view,
                             CtkTreePath        *path,
                             CtkTreeViewColumn  *column,
                             CdkRectangle       *rect)
{
  CtkRBTree *tree = NULL;
  CtkRBNode *node = NULL;
  gint vertical_separator;
  gint horizontal_separator;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (column == NULL || CTK_IS_TREE_VIEW_COLUMN (column));
  g_return_if_fail (rect != NULL);
  g_return_if_fail (!column || ctk_tree_view_column_get_tree_view (column) == (CtkWidget *) tree_view);
  g_return_if_fail (ctk_widget_get_realized (CTK_WIDGET (tree_view)));

  ctk_widget_style_get (CTK_WIDGET (tree_view),
			"vertical-separator", &vertical_separator,
			"horizontal-separator", &horizontal_separator,
			NULL);

  rect->x = 0;
  rect->y = 0;
  rect->width = 0;
  rect->height = 0;

  if (column)
    {
      rect->x = ctk_tree_view_column_get_x_offset (column) + horizontal_separator/2;
      rect->width = ctk_tree_view_column_get_width (column) - horizontal_separator;
    }

  if (path)
    {
      gboolean ret = _ctk_tree_view_find_node (tree_view, path, &tree, &node);

      /* Get vertical coords */
      if ((!ret && tree == NULL) || ret)
	return;

      if (row_is_separator (tree_view, NULL, path))
        {
          /* There isn't really a "cell area" for separator, so we
           * return the y, height values for background area instead.
           */
          rect->y = ctk_tree_view_get_row_y_offset (tree_view, tree, node);
          rect->height = ctk_tree_view_get_row_height (tree_view, node);
        }
      else
        {
          rect->y = ctk_tree_view_get_cell_area_y_offset (tree_view, tree, node,
                                                          vertical_separator);
          rect->height = ctk_tree_view_get_cell_area_height (tree_view, node,
                                                             vertical_separator);
        }

      if (column &&
	  ctk_tree_view_is_expander_column (tree_view, column))
	{
	  gint depth = ctk_tree_path_get_depth (path);
	  gboolean rtl;

	  rtl = ctk_widget_get_direction (CTK_WIDGET (tree_view)) == CTK_TEXT_DIR_RTL;

	  if (!rtl)
	    rect->x += (depth - 1) * tree_view->priv->level_indentation;
	  rect->width -= (depth - 1) * tree_view->priv->level_indentation;

	  if (ctk_tree_view_draw_expanders (tree_view))
	    {
              int expander_size = ctk_tree_view_get_expander_size (tree_view);
	      if (!rtl)
		rect->x += depth * expander_size;
	      rect->width -= depth * expander_size;
	    }

	  rect->width = MAX (rect->width, 0);
	}
    }
}

static inline gint
ctk_tree_view_get_row_height (CtkTreeView *tree_view,
                              CtkRBNode   *node)
{
  int expander_size = ctk_tree_view_get_expander_size (tree_view);
  int height;

  /* The "background" areas of all rows/cells add up to cover the entire tree.
   * The background includes all inter-row and inter-cell spacing.
   *
   * If the row pointed at by node does not have a height set, we default
   * to expander_size, which is the minimum height for regular nodes.
   * Non-regular nodes (e.g. separators) can have a height set smaller
   * than expander_size and should not be overruled here.
   */
  height = CTK_RBNODE_GET_HEIGHT (node);
  if (height <= 0)
    height = expander_size;

  return height;
}

static inline gint
ctk_tree_view_get_row_y_offset (CtkTreeView *tree_view,
                                CtkRBTree   *tree,
                                CtkRBNode   *node)
{
  int offset;

  offset = _ctk_rbtree_node_find_offset (tree, node);

  return RBTREE_Y_TO_TREE_WINDOW_Y (tree_view, offset);
}

/**
 * ctk_tree_view_get_background_area:
 * @tree_view: a #CtkTreeView
 * @path: (allow-none): a #CtkTreePath for the row, or %NULL to get only horizontal coordinates
 * @column: (allow-none): a #CtkTreeViewColumn for the column, or %NULL to get only vertical coordiantes
 * @rect: (out): rectangle to fill with cell background rect
 *
 * Fills the bounding rectangle in bin_window coordinates for the cell at the
 * row specified by @path and the column specified by @column.  If @path is
 * %NULL, or points to a node not found in the tree, the @y and @height fields of
 * the rectangle will be filled with 0. If @column is %NULL, the @x and @width
 * fields will be filled with 0.  The returned rectangle is equivalent to the
 * @background_area passed to ctk_cell_renderer_render().  These background
 * areas tile to cover the entire bin window.  Contrast with the @cell_area,
 * returned by ctk_tree_view_get_cell_area(), which returns only the cell
 * itself, excluding surrounding borders and the tree expander area.
 *
 **/
void
ctk_tree_view_get_background_area (CtkTreeView        *tree_view,
                                   CtkTreePath        *path,
                                   CtkTreeViewColumn  *column,
                                   CdkRectangle       *rect)
{
  CtkRBTree *tree = NULL;
  CtkRBNode *node = NULL;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (column == NULL || CTK_IS_TREE_VIEW_COLUMN (column));
  g_return_if_fail (rect != NULL);

  rect->x = 0;
  rect->y = 0;
  rect->width = 0;
  rect->height = 0;

  if (path)
    {
      /* Get vertical coords */

      if (!_ctk_tree_view_find_node (tree_view, path, &tree, &node) &&
	  tree == NULL)
	return;

      rect->y = ctk_tree_view_get_row_y_offset (tree_view, tree, node);
      rect->height = ctk_tree_view_get_row_height (tree_view, node);
    }

  if (column)
    {
      gint x2 = 0;

      ctk_tree_view_get_background_xrange (tree_view, tree, column, &rect->x, &x2);
      rect->width = x2 - rect->x;
    }
}

/**
 * ctk_tree_view_get_visible_rect:
 * @tree_view: a #CtkTreeView
 * @visible_rect: (out): rectangle to fill
 *
 * Fills @visible_rect with the currently-visible region of the
 * buffer, in tree coordinates. Convert to bin_window coordinates with
 * ctk_tree_view_convert_tree_to_bin_window_coords().
 * Tree coordinates start at 0,0 for row 0 of the tree, and cover the entire
 * scrollable area of the tree.
 **/
void
ctk_tree_view_get_visible_rect (CtkTreeView  *tree_view,
                                CdkRectangle *visible_rect)
{
  CtkAllocation allocation;
  CtkWidget *widget;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  widget = CTK_WIDGET (tree_view);

  if (visible_rect)
    {
      ctk_widget_get_allocation (widget, &allocation);
      visible_rect->x = ctk_adjustment_get_value (tree_view->priv->hadjustment);
      visible_rect->y = ctk_adjustment_get_value (tree_view->priv->vadjustment);
      visible_rect->width = allocation.width;
      visible_rect->height = allocation.height - ctk_tree_view_get_effective_header_height (tree_view);
    }
}

/**
 * ctk_tree_view_convert_widget_to_tree_coords:
 * @tree_view: a #CtkTreeView
 * @wx: X coordinate relative to the widget
 * @wy: Y coordinate relative to the widget
 * @tx: (out): return location for tree X coordinate
 * @ty: (out): return location for tree Y coordinate
 *
 * Converts widget coordinates to coordinates for the
 * tree (the full scrollable area of the tree).
 *
 * Since: 2.12
 **/
void
ctk_tree_view_convert_widget_to_tree_coords (CtkTreeView *tree_view,
                                             gint         wx,
                                             gint         wy,
                                             gint        *tx,
                                             gint        *ty)
{
  gint x, y;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  ctk_tree_view_convert_widget_to_bin_window_coords (tree_view,
						     wx, wy,
						     &x, &y);
  ctk_tree_view_convert_bin_window_to_tree_coords (tree_view,
						   x, y,
						   tx, ty);
}

/**
 * ctk_tree_view_convert_tree_to_widget_coords:
 * @tree_view: a #CtkTreeView
 * @tx: X coordinate relative to the tree
 * @ty: Y coordinate relative to the tree
 * @wx: (out): return location for widget X coordinate
 * @wy: (out): return location for widget Y coordinate
 *
 * Converts tree coordinates (coordinates in full scrollable area of the tree)
 * to widget coordinates.
 *
 * Since: 2.12
 **/
void
ctk_tree_view_convert_tree_to_widget_coords (CtkTreeView *tree_view,
                                             gint         tx,
                                             gint         ty,
                                             gint        *wx,
                                             gint        *wy)
{
  gint x, y;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  ctk_tree_view_convert_tree_to_bin_window_coords (tree_view,
						   tx, ty,
						   &x, &y);
  ctk_tree_view_convert_bin_window_to_widget_coords (tree_view,
						     x, y,
						     wx, wy);
}

/**
 * ctk_tree_view_convert_widget_to_bin_window_coords:
 * @tree_view: a #CtkTreeView
 * @wx: X coordinate relative to the widget
 * @wy: Y coordinate relative to the widget
 * @bx: (out): return location for bin_window X coordinate
 * @by: (out): return location for bin_window Y coordinate
 *
 * Converts widget coordinates to coordinates for the bin_window
 * (see ctk_tree_view_get_bin_window()).
 *
 * Since: 2.12
 **/
void
ctk_tree_view_convert_widget_to_bin_window_coords (CtkTreeView *tree_view,
                                                   gint         wx,
                                                   gint         wy,
                                                   gint        *bx,
                                                   gint        *by)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  if (bx)
    *bx = wx + ctk_adjustment_get_value (tree_view->priv->hadjustment);
  if (by)
    *by = wy - ctk_tree_view_get_effective_header_height (tree_view);
}

/**
 * ctk_tree_view_convert_bin_window_to_widget_coords:
 * @tree_view: a #CtkTreeView
 * @bx: bin_window X coordinate
 * @by: bin_window Y coordinate
 * @wx: (out): return location for widget X coordinate
 * @wy: (out): return location for widget Y coordinate
 *
 * Converts bin_window coordinates (see ctk_tree_view_get_bin_window())
 * to widget relative coordinates.
 *
 * Since: 2.12
 **/
void
ctk_tree_view_convert_bin_window_to_widget_coords (CtkTreeView *tree_view,
                                                   gint         bx,
                                                   gint         by,
                                                   gint        *wx,
                                                   gint        *wy)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  if (wx)
    *wx = bx - ctk_adjustment_get_value (tree_view->priv->hadjustment);
  if (wy)
    *wy = by + ctk_tree_view_get_effective_header_height (tree_view);
}

/**
 * ctk_tree_view_convert_tree_to_bin_window_coords:
 * @tree_view: a #CtkTreeView
 * @tx: tree X coordinate
 * @ty: tree Y coordinate
 * @bx: (out): return location for X coordinate relative to bin_window
 * @by: (out): return location for Y coordinate relative to bin_window
 *
 * Converts tree coordinates (coordinates in full scrollable area of the tree)
 * to bin_window coordinates.
 *
 * Since: 2.12
 **/
void
ctk_tree_view_convert_tree_to_bin_window_coords (CtkTreeView *tree_view,
                                                 gint         tx,
                                                 gint         ty,
                                                 gint        *bx,
                                                 gint        *by)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  if (bx)
    *bx = tx;
  if (by)
    *by = ty - tree_view->priv->dy;
}

/**
 * ctk_tree_view_convert_bin_window_to_tree_coords:
 * @tree_view: a #CtkTreeView
 * @bx: X coordinate relative to bin_window
 * @by: Y coordinate relative to bin_window
 * @tx: (out): return location for tree X coordinate
 * @ty: (out): return location for tree Y coordinate
 *
 * Converts bin_window coordinates to coordinates for the
 * tree (the full scrollable area of the tree).
 *
 * Since: 2.12
 **/
void
ctk_tree_view_convert_bin_window_to_tree_coords (CtkTreeView *tree_view,
                                                 gint         bx,
                                                 gint         by,
                                                 gint        *tx,
                                                 gint        *ty)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  if (tx)
    *tx = bx;
  if (ty)
    *ty = by + tree_view->priv->dy;
}



/**
 * ctk_tree_view_get_visible_range:
 * @tree_view: A #CtkTreeView
 * @start_path: (out) (allow-none): Return location for start of region,
 *              or %NULL.
 * @end_path: (out) (allow-none): Return location for end of region, or %NULL.
 *
 * Sets @start_path and @end_path to be the first and last visible path.
 * Note that there may be invisible paths in between.
 *
 * The paths should be freed with ctk_tree_path_free() after use.
 *
 * Returns: %TRUE, if valid paths were placed in @start_path and @end_path.
 *
 * Since: 2.8
 **/
gboolean
ctk_tree_view_get_visible_range (CtkTreeView  *tree_view,
                                 CtkTreePath **start_path,
                                 CtkTreePath **end_path)
{
  CtkRBTree *tree;
  CtkRBNode *node;
  gboolean retval;
  
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);

  if (!tree_view->priv->tree)
    return FALSE;

  retval = TRUE;

  if (start_path)
    {
      _ctk_rbtree_find_offset (tree_view->priv->tree,
                               TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, 0),
                               &tree, &node);
      if (node)
        *start_path = _ctk_tree_path_new_from_rbtree (tree, node);
      else
        retval = FALSE;
    }

  if (end_path)
    {
      gint y;

      if (ctk_tree_view_get_height (tree_view) < ctk_adjustment_get_page_size (tree_view->priv->vadjustment))
        y = ctk_tree_view_get_height (tree_view) - 1;
      else
        y = TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, ctk_adjustment_get_page_size (tree_view->priv->vadjustment)) - 1;

      _ctk_rbtree_find_offset (tree_view->priv->tree, y, &tree, &node);
      if (node)
        *end_path = _ctk_tree_path_new_from_rbtree (tree, node);
      else
        retval = FALSE;
    }

  return retval;
}

/**
 * ctk_tree_view_is_blank_at_pos:
 * @tree_view: A #CtkTreeView
 * @x: The x position to be identified (relative to bin_window)
 * @y: The y position to be identified (relative to bin_window)
 * @path: (out) (optional) (nullable): A pointer to a #CtkTreePath pointer to
 *   be filled in, or %NULL
 * @column: (out) (transfer none) (optional) (nullable): A pointer to a
 *   #CtkTreeViewColumn pointer to be filled in, or %NULL
 * @cell_x: (out) (optional): A pointer where the X coordinate relative to the
 *   cell can be placed, or %NULL
 * @cell_y: (out) (optional): A pointer where the Y coordinate relative to the
 *   cell can be placed, or %NULL
 *
 * Determine whether the point (@x, @y) in @tree_view is blank, that is no
 * cell content nor an expander arrow is drawn at the location. If so, the
 * location can be considered as the background. You might wish to take
 * special action on clicks on the background, such as clearing a current
 * selection, having a custom context menu or starting rubber banding.
 *
 * The @x and @y coordinate that are provided must be relative to bin_window
 * coordinates.  That is, @x and @y must come from an event on @tree_view
 * where `event->window == ctk_tree_view_get_bin_window ()`.
 *
 * For converting widget coordinates (eg. the ones you get from
 * CtkWidget::query-tooltip), please see
 * ctk_tree_view_convert_widget_to_bin_window_coords().
 *
 * The @path, @column, @cell_x and @cell_y arguments will be filled in
 * likewise as for ctk_tree_view_get_path_at_pos().  Please see
 * ctk_tree_view_get_path_at_pos() for more information.
 *
 * Returns: %TRUE if the area at the given coordinates is blank,
 * %FALSE otherwise.
 *
 * Since: 3.0
 */
gboolean
ctk_tree_view_is_blank_at_pos (CtkTreeView       *tree_view,
                               gint                x,
                               gint                y,
                               CtkTreePath       **path,
                               CtkTreeViewColumn **column,
                               gint               *cell_x,
                               gint               *cell_y)
{
  CtkRBTree *tree;
  CtkRBNode *node;
  CtkTreeIter iter;
  CtkTreePath *real_path;
  CtkTreeViewColumn *real_column;
  CdkRectangle cell_area, background_area;

  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);

  if (!ctk_tree_view_get_path_at_pos (tree_view, x, y,
                                      &real_path, &real_column,
                                      cell_x, cell_y))
    /* If there's no path here, it is blank */
    return TRUE;

  if (path)
    *path = real_path;

  if (column)
    *column = real_column;

  ctk_tree_model_get_iter (tree_view->priv->model, &iter, real_path);
  _ctk_tree_view_find_node (tree_view, real_path, &tree, &node);

  /* Check if there's an expander arrow at (x, y) */
  if (real_column == tree_view->priv->expander_column
      && ctk_tree_view_draw_expanders (tree_view))
    {
      gboolean over_arrow;

      over_arrow = coords_are_over_arrow (tree_view, tree, node, x, y);

      if (over_arrow)
        {
          if (!path)
            ctk_tree_path_free (real_path);
          return FALSE;
        }
    }

  /* Otherwise, have the column see if there's a cell at (x, y) */
  ctk_tree_view_column_cell_set_cell_data (real_column,
                                           tree_view->priv->model,
                                           &iter,
                                           CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_PARENT),
                                           node->children ? TRUE : FALSE);

  ctk_tree_view_get_background_area (tree_view, real_path, real_column,
                                     &background_area);
  ctk_tree_view_get_cell_area (tree_view, real_path, real_column,
                               &cell_area);

  if (!path)
    ctk_tree_path_free (real_path);

  return _ctk_tree_view_column_is_blank_at_pos (real_column,
                                                &cell_area,
                                                &background_area,
                                                x, y);
}

static void
unset_reorderable (CtkTreeView *tree_view)
{
  if (tree_view->priv->reorderable)
    {
      tree_view->priv->reorderable = FALSE;
      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_REORDERABLE]);
    }
}

/**
 * ctk_tree_view_enable_model_drag_source:
 * @tree_view: a #CtkTreeView
 * @start_button_mask: Mask of allowed buttons to start drag
 * @targets: (array length=n_targets): the table of targets that the drag will support
 * @n_targets: the number of items in @targets
 * @actions: the bitmask of possible actions for a drag from this
 *    widget
 *
 * Turns @tree_view into a drag source for automatic DND. Calling this
 * method sets #CtkTreeView:reorderable to %FALSE.
 **/
void
ctk_tree_view_enable_model_drag_source (CtkTreeView              *tree_view,
					CdkModifierType           start_button_mask,
					const CtkTargetEntry     *targets,
					gint                      n_targets,
					CdkDragAction             actions)
{
  TreeViewDragInfo *di;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  ctk_drag_source_set (CTK_WIDGET (tree_view),
		       0,
		       targets,
		       n_targets,
		       actions);

  di = ensure_info (tree_view);

  di->start_button_mask = start_button_mask;
  di->source_actions = actions;
  di->source_set = TRUE;

  unset_reorderable (tree_view);
}

/**
 * ctk_tree_view_enable_model_drag_dest:
 * @tree_view: a #CtkTreeView
 * @targets: (array length=n_targets): the table of targets that
 *           the drag will support
 * @n_targets: the number of items in @targets
 * @actions: the bitmask of possible actions for a drag from this
 *    widget
 * 
 * Turns @tree_view into a drop destination for automatic DND. Calling
 * this method sets #CtkTreeView:reorderable to %FALSE.
 **/
void
ctk_tree_view_enable_model_drag_dest (CtkTreeView              *tree_view,
				      const CtkTargetEntry     *targets,
				      gint                      n_targets,
				      CdkDragAction             actions)
{
  TreeViewDragInfo *di;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  ctk_drag_dest_set (CTK_WIDGET (tree_view),
                     0,
                     targets,
                     n_targets,
                     actions);

  di = ensure_info (tree_view);
  di->dest_set = TRUE;

  unset_reorderable (tree_view);
}

/**
 * ctk_tree_view_unset_rows_drag_source:
 * @tree_view: a #CtkTreeView
 *
 * Undoes the effect of
 * ctk_tree_view_enable_model_drag_source(). Calling this method sets
 * #CtkTreeView:reorderable to %FALSE.
 **/
void
ctk_tree_view_unset_rows_drag_source (CtkTreeView *tree_view)
{
  TreeViewDragInfo *di;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  di = get_info (tree_view);

  if (di)
    {
      if (di->source_set)
        {
          ctk_drag_source_unset (CTK_WIDGET (tree_view));
          di->source_set = FALSE;
        }

      if (!di->dest_set && !di->source_set)
        remove_info (tree_view);
    }
  
  unset_reorderable (tree_view);
}

/**
 * ctk_tree_view_unset_rows_drag_dest:
 * @tree_view: a #CtkTreeView
 *
 * Undoes the effect of
 * ctk_tree_view_enable_model_drag_dest(). Calling this method sets
 * #CtkTreeView:reorderable to %FALSE.
 **/
void
ctk_tree_view_unset_rows_drag_dest (CtkTreeView *tree_view)
{
  TreeViewDragInfo *di;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  di = get_info (tree_view);

  if (di)
    {
      if (di->dest_set)
        {
          ctk_drag_dest_unset (CTK_WIDGET (tree_view));
          di->dest_set = FALSE;
        }

      if (!di->dest_set && !di->source_set)
        remove_info (tree_view);
    }

  unset_reorderable (tree_view);
}

/**
 * ctk_tree_view_set_drag_dest_row:
 * @tree_view: a #CtkTreeView
 * @path: (allow-none): The path of the row to highlight, or %NULL
 * @pos: Specifies whether to drop before, after or into the row
 *
 * Sets the row that is highlighted for feedback.
 * If @path is %NULL, an existing highlight is removed.
 */
void
ctk_tree_view_set_drag_dest_row (CtkTreeView            *tree_view,
                                 CtkTreePath            *path,
                                 CtkTreeViewDropPosition pos)
{
  CtkTreePath *current_dest;

  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  current_dest = NULL;

  if (tree_view->priv->drag_dest_row)
    {
      current_dest = ctk_tree_row_reference_get_path (tree_view->priv->drag_dest_row);
      ctk_tree_row_reference_free (tree_view->priv->drag_dest_row);
    }

  /* special case a drop on an empty model */
  tree_view->priv->empty_view_drop = 0;

  if (pos == CTK_TREE_VIEW_DROP_BEFORE && path
      && ctk_tree_path_get_depth (path) == 1
      && ctk_tree_path_get_indices (path)[0] == 0)
    {
      gint n_children;

      n_children = ctk_tree_model_iter_n_children (tree_view->priv->model,
                                                   NULL);

      if (!n_children)
        tree_view->priv->empty_view_drop = 1;
    }

  tree_view->priv->drag_dest_pos = pos;

  if (path)
    {
      tree_view->priv->drag_dest_row =
        ctk_tree_row_reference_new_proxy (G_OBJECT (tree_view), tree_view->priv->model, path);
      ctk_tree_view_queue_draw_path (tree_view, path, NULL);
    }
  else
    tree_view->priv->drag_dest_row = NULL;

  if (current_dest)
    {
      CtkRBTree *tree, *new_tree;
      CtkRBNode *node, *new_node;

      _ctk_tree_view_find_node (tree_view, current_dest, &tree, &node);
      _ctk_tree_view_queue_draw_node (tree_view, tree, node, NULL);

      if (tree && node)
	{
	  _ctk_rbtree_next_full (tree, node, &new_tree, &new_node);
	  if (new_tree && new_node)
	    _ctk_tree_view_queue_draw_node (tree_view, new_tree, new_node, NULL);

	  _ctk_rbtree_prev_full (tree, node, &new_tree, &new_node);
	  if (new_tree && new_node)
	    _ctk_tree_view_queue_draw_node (tree_view, new_tree, new_node, NULL);
	}
      ctk_tree_path_free (current_dest);
    }
}

/**
 * ctk_tree_view_get_drag_dest_row:
 * @tree_view: a #CtkTreeView
 * @path: (out) (optional) (nullable): Return location for the path of the highlighted row, or %NULL.
 * @pos: (out) (optional): Return location for the drop position, or %NULL
 * 
 * Gets information about the row that is highlighted for feedback.
 **/
void
ctk_tree_view_get_drag_dest_row (CtkTreeView              *tree_view,
                                 CtkTreePath             **path,
                                 CtkTreeViewDropPosition  *pos)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  if (path)
    {
      if (tree_view->priv->drag_dest_row)
        *path = ctk_tree_row_reference_get_path (tree_view->priv->drag_dest_row);
      else
        {
          if (tree_view->priv->empty_view_drop)
            *path = ctk_tree_path_new_from_indices (0, -1);
          else
            *path = NULL;
        }
    }

  if (pos)
    *pos = tree_view->priv->drag_dest_pos;
}

/**
 * ctk_tree_view_get_dest_row_at_pos:
 * @tree_view: a #CtkTreeView
 * @drag_x: the position to determine the destination row for
 * @drag_y: the position to determine the destination row for
 * @path: (out) (optional) (nullable): Return location for the path of
 *   the highlighted row, or %NULL.
 * @pos: (out) (optional): Return location for the drop position, or
 *   %NULL
 * 
 * Determines the destination row for a given position.  @drag_x and
 * @drag_y are expected to be in widget coordinates.  This function is only
 * meaningful if @tree_view is realized.  Therefore this function will always
 * return %FALSE if @tree_view is not realized or does not have a model.
 * 
 * Returns: whether there is a row at the given position, %TRUE if this
 * is indeed the case.
 **/
gboolean
ctk_tree_view_get_dest_row_at_pos (CtkTreeView             *tree_view,
                                   gint                     drag_x,
                                   gint                     drag_y,
                                   CtkTreePath            **path,
                                   CtkTreeViewDropPosition *pos)
{
  gint cell_y;
  gint bin_x, bin_y;
  gdouble offset_into_row;
  gdouble fourth;
  CdkRectangle cell;
  CtkTreeViewColumn *column = NULL;
  CtkTreePath *tmp_path = NULL;

  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_val_if_fail (tree_view != NULL, FALSE);
  g_return_val_if_fail (drag_x >= 0, FALSE);
  g_return_val_if_fail (drag_y >= 0, FALSE);

  if (path)
    *path = NULL;

  if (tree_view->priv->bin_window == NULL)
    return FALSE;

  if (tree_view->priv->tree == NULL)
    return FALSE;

  /* If in the top fourth of a row, we drop before that row; if
   * in the bottom fourth, drop after that row; if in the middle,
   * and the row has children, drop into the row.
   */
  ctk_tree_view_convert_widget_to_bin_window_coords (tree_view, drag_x, drag_y,
						     &bin_x, &bin_y);

  if (!ctk_tree_view_get_path_at_pos (tree_view,
				      bin_x,
				      bin_y,
                                      &tmp_path,
                                      &column,
                                      NULL,
                                      &cell_y))
    return FALSE;

  ctk_tree_view_get_background_area (tree_view, tmp_path, column,
                                     &cell);

  offset_into_row = cell_y;

  if (path)
    *path = tmp_path;
  else
    ctk_tree_path_free (tmp_path);

  tmp_path = NULL;

  fourth = cell.height / 4.0;

  if (pos)
    {
      if (offset_into_row < fourth)
        {
          *pos = CTK_TREE_VIEW_DROP_BEFORE;
        }
      else if (offset_into_row < (cell.height / 2.0))
        {
          *pos = CTK_TREE_VIEW_DROP_INTO_OR_BEFORE;
        }
      else if (offset_into_row < cell.height - fourth)
        {
          *pos = CTK_TREE_VIEW_DROP_INTO_OR_AFTER;
        }
      else
        {
          *pos = CTK_TREE_VIEW_DROP_AFTER;
        }
    }

  return TRUE;
}



/* KEEP IN SYNC WITH CTK_TREE_VIEW_BIN_EXPOSE */
/**
 * ctk_tree_view_create_row_drag_icon:
 * @tree_view: a #CtkTreeView
 * @path: a #CtkTreePath in @tree_view
 *
 * Creates a #cairo_surface_t representation of the row at @path.  
 * This image is used for a drag icon.
 *
 * Returns: (transfer full): a newly-allocated surface of the drag icon.
 **/
cairo_surface_t *
ctk_tree_view_create_row_drag_icon (CtkTreeView  *tree_view,
                                    CtkTreePath  *path)
{
  CtkTreeIter   iter;
  CtkRBTree    *tree;
  CtkRBNode    *node;
  CtkStyleContext *context;
  gint cell_offset;
  GList *list;
  CdkRectangle background_area;
  CtkWidget *widget;
  gint depth;
  /* start drawing inside the black outline */
  gint x = 1, y = 1;
  cairo_surface_t *surface;
  gint bin_window_width;
  gboolean is_separator = FALSE;
  gboolean rtl;
  cairo_t *cr;

  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  widget = CTK_WIDGET (tree_view);

  if (!ctk_widget_get_realized (widget))
    return NULL;

  depth = ctk_tree_path_get_depth (path);

  _ctk_tree_view_find_node (tree_view,
                            path,
                            &tree,
                            &node);

  if (tree == NULL)
    return NULL;

  if (!ctk_tree_model_get_iter (tree_view->priv->model,
                                &iter,
                                path))
    return NULL;

  context = ctk_widget_get_style_context (widget);

  is_separator = row_is_separator (tree_view, &iter, NULL);

  cell_offset = x;

  background_area.y = y;
  background_area.height = ctk_tree_view_get_row_height (tree_view, node);

  bin_window_width = cdk_window_get_width (tree_view->priv->bin_window);

  surface = cdk_window_create_similar_surface (tree_view->priv->bin_window,
                                               CAIRO_CONTENT_COLOR,
                                               bin_window_width + 2,
                                               background_area.height + 2);

  cr = cairo_create (surface);

  ctk_render_background (context, cr, 0, 0,
                         bin_window_width + 2,
                         background_area.height + 2);

  rtl = ctk_widget_get_direction (CTK_WIDGET (tree_view)) == CTK_TEXT_DIR_RTL;

  for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
      list;
      list = (rtl ? list->prev : list->next))
    {
      CtkTreeViewColumn *column = list->data;
      CdkRectangle cell_area;
      gint vertical_separator;

      if (!ctk_tree_view_column_get_visible (column))
        continue;

      ctk_tree_view_column_cell_set_cell_data (column, tree_view->priv->model, &iter,
					       CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_PARENT),
					       node->children?TRUE:FALSE);

      background_area.x = cell_offset;
      background_area.width = ctk_tree_view_column_get_width (column);

      ctk_widget_style_get (widget,
			    "vertical-separator", &vertical_separator,
			    NULL);

      cell_area = background_area;

      cell_area.y += vertical_separator / 2;
      cell_area.height -= vertical_separator;

      if (ctk_tree_view_is_expander_column (tree_view, column))
        {
	  if (!rtl)
	    cell_area.x += (depth - 1) * tree_view->priv->level_indentation;
	  cell_area.width -= (depth - 1) * tree_view->priv->level_indentation;

          if (ctk_tree_view_draw_expanders (tree_view))
	    {
              int expander_size = ctk_tree_view_get_expander_size (tree_view);
	      if (!rtl)
		cell_area.x += depth * expander_size;
	      cell_area.width -= depth * expander_size;
	    }
        }

      if (ctk_tree_view_column_cell_is_visible (column))
	{
	  if (is_separator)
            {
              ctk_style_context_save (context);
              ctk_style_context_add_class (context, CTK_STYLE_CLASS_SEPARATOR);

              ctk_render_line (context, cr,
                               cell_area.x,
                               cell_area.y + cell_area.height / 2,
                               cell_area.x + cell_area.width,
                               cell_area.y + cell_area.height / 2);

              ctk_style_context_restore (context);
            }
	  else
            {
              _ctk_tree_view_column_cell_render (column,
                                                 cr,
                                                 &background_area,
                                                 &cell_area,
                                                 0, FALSE);
            }
	}
      cell_offset += ctk_tree_view_column_get_width (column);
    }

  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_rectangle (cr, 
                   0.5, 0.5, 
                   bin_window_width + 1,
                   background_area.height + 1);
  cairo_set_line_width (cr, 1.0);
  cairo_stroke (cr);

  cairo_destroy (cr);

  cairo_surface_set_device_offset (surface, 2, 2);

  return surface;
}


/**
 * ctk_tree_view_set_destroy_count_func:
 * @tree_view: A #CtkTreeView
 * @func: (allow-none): Function to be called when a view row is destroyed, or %NULL
 * @data: (allow-none): User data to be passed to @func, or %NULL
 * @destroy: (allow-none): Destroy notifier for @data, or %NULL
 *
 * This function should almost never be used.  It is meant for private use by
 * ATK for determining the number of visible children that are removed when the
 * user collapses a row, or a row is deleted.
 *
 * Deprecated: 3.4: Accessibility does not need the function anymore.
 **/
void
ctk_tree_view_set_destroy_count_func (CtkTreeView             *tree_view,
				      CtkTreeDestroyCountFunc  func,
				      gpointer                 data,
				      GDestroyNotify           destroy)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  if (tree_view->priv->destroy_count_destroy)
    tree_view->priv->destroy_count_destroy (tree_view->priv->destroy_count_data);

  tree_view->priv->destroy_count_func = func;
  tree_view->priv->destroy_count_data = data;
  tree_view->priv->destroy_count_destroy = destroy;
}


/*
 * Interactive search
 */

/**
 * ctk_tree_view_set_enable_search:
 * @tree_view: A #CtkTreeView
 * @enable_search: %TRUE, if the user can search interactively
 *
 * If @enable_search is set, then the user can type in text to search through
 * the tree interactively (this is sometimes called "typeahead find").
 * 
 * Note that even if this is %FALSE, the user can still initiate a search 
 * using the “start-interactive-search” key binding.
 */
void
ctk_tree_view_set_enable_search (CtkTreeView *tree_view,
				 gboolean     enable_search)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  enable_search = !!enable_search;
  
  if (tree_view->priv->enable_search != enable_search)
    {
       tree_view->priv->enable_search = enable_search;
       g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_ENABLE_SEARCH]);
    }
}

/**
 * ctk_tree_view_get_enable_search:
 * @tree_view: A #CtkTreeView
 *
 * Returns whether or not the tree allows to start interactive searching 
 * by typing in text.
 *
 * Returns: whether or not to let the user search interactively
 */
gboolean
ctk_tree_view_get_enable_search (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);

  return tree_view->priv->enable_search;
}


/**
 * ctk_tree_view_get_search_column:
 * @tree_view: A #CtkTreeView
 *
 * Gets the column searched on by the interactive search code.
 *
 * Returns: the column the interactive search code searches in.
 */
gint
ctk_tree_view_get_search_column (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), -1);

  return (tree_view->priv->search_column);
}

/**
 * ctk_tree_view_set_search_column:
 * @tree_view: A #CtkTreeView
 * @column: the column of the model to search in, or -1 to disable searching
 *
 * Sets @column as the column where the interactive search code should
 * search in for the current model. 
 * 
 * If the search column is set, users can use the “start-interactive-search”
 * key binding to bring up search popup. The enable-search property controls
 * whether simply typing text will also start an interactive search.
 *
 * Note that @column refers to a column of the current model. The search 
 * column is reset to -1 when the model is changed.
 */
void
ctk_tree_view_set_search_column (CtkTreeView *tree_view,
				 gint         column)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (column >= -1);

  if (tree_view->priv->search_column == column)
    return;

  tree_view->priv->search_column = column;
  g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_SEARCH_COLUMN]);
}

/**
 * ctk_tree_view_get_search_equal_func: (skip)
 * @tree_view: A #CtkTreeView
 *
 * Returns the compare function currently in use.
 *
 * Returns: the currently used compare function for the search code.
 */

CtkTreeViewSearchEqualFunc
ctk_tree_view_get_search_equal_func (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), NULL);

  return tree_view->priv->search_equal_func;
}

/**
 * ctk_tree_view_set_search_equal_func:
 * @tree_view: A #CtkTreeView
 * @search_equal_func: the compare function to use during the search
 * @search_user_data: (allow-none): user data to pass to @search_equal_func, or %NULL
 * @search_destroy: (allow-none): Destroy notifier for @search_user_data, or %NULL
 *
 * Sets the compare function for the interactive search capabilities; note
 * that somewhat like strcmp() returning 0 for equality
 * #CtkTreeViewSearchEqualFunc returns %FALSE on matches.
 **/
void
ctk_tree_view_set_search_equal_func (CtkTreeView                *tree_view,
				     CtkTreeViewSearchEqualFunc  search_equal_func,
				     gpointer                    search_user_data,
				     GDestroyNotify              search_destroy)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (search_equal_func != NULL);

  if (tree_view->priv->search_destroy)
    tree_view->priv->search_destroy (tree_view->priv->search_user_data);

  tree_view->priv->search_equal_func = search_equal_func;
  tree_view->priv->search_user_data = search_user_data;
  tree_view->priv->search_destroy = search_destroy;
  if (tree_view->priv->search_equal_func == NULL)
    tree_view->priv->search_equal_func = ctk_tree_view_search_equal_func;
}

/**
 * ctk_tree_view_get_search_entry:
 * @tree_view: A #CtkTreeView
 *
 * Returns the #CtkEntry which is currently in use as interactive search
 * entry for @tree_view.  In case the built-in entry is being used, %NULL
 * will be returned.
 *
 * Returns: (transfer none): the entry currently in use as search entry.
 *
 * Since: 2.10
 */
CtkEntry *
ctk_tree_view_get_search_entry (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), NULL);

  if (tree_view->priv->search_custom_entry_set)
    return CTK_ENTRY (tree_view->priv->search_entry);

  return NULL;
}

/**
 * ctk_tree_view_set_search_entry:
 * @tree_view: A #CtkTreeView
 * @entry: (allow-none): the entry the interactive search code of @tree_view should use or %NULL
 *
 * Sets the entry which the interactive search code will use for this
 * @tree_view.  This is useful when you want to provide a search entry
 * in our interface at all time at a fixed position.  Passing %NULL for
 * @entry will make the interactive search code use the built-in popup
 * entry again.
 *
 * Since: 2.10
 */
void
ctk_tree_view_set_search_entry (CtkTreeView *tree_view,
				CtkEntry    *entry)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (entry == NULL || CTK_IS_ENTRY (entry));

  if (tree_view->priv->search_custom_entry_set)
    {
      if (tree_view->priv->search_entry_changed_id)
        {
	  g_signal_handler_disconnect (tree_view->priv->search_entry,
				       tree_view->priv->search_entry_changed_id);
	  tree_view->priv->search_entry_changed_id = 0;
	}
      g_signal_handlers_disconnect_by_func (tree_view->priv->search_entry,
					    G_CALLBACK (ctk_tree_view_search_key_press_event),
					    tree_view);

      g_object_unref (tree_view->priv->search_entry);
    }
  else if (tree_view->priv->search_window)
    {
      ctk_tree_view_destroy_search_window (tree_view);
    }

  if (entry)
    {
      tree_view->priv->search_entry = CTK_WIDGET (g_object_ref (entry));
      tree_view->priv->search_custom_entry_set = TRUE;

      if (tree_view->priv->search_entry_changed_id == 0)
        {
          tree_view->priv->search_entry_changed_id =
	    g_signal_connect (tree_view->priv->search_entry, "changed",
			      G_CALLBACK (ctk_tree_view_search_init),
			      tree_view);
	}
      
        g_signal_connect (tree_view->priv->search_entry, "key-press-event",
		          G_CALLBACK (ctk_tree_view_search_key_press_event),
		          tree_view);

	ctk_tree_view_search_init (tree_view->priv->search_entry, tree_view);
    }
  else
    {
      tree_view->priv->search_entry = NULL;
      tree_view->priv->search_custom_entry_set = FALSE;
    }
}

/**
 * ctk_tree_view_set_search_position_func:
 * @tree_view: A #CtkTreeView
 * @func: (allow-none): the function to use to position the search dialog, or %NULL
 *    to use the default search position function
 * @data: (allow-none): user data to pass to @func, or %NULL
 * @destroy: (allow-none): Destroy notifier for @data, or %NULL
 *
 * Sets the function to use when positioning the search dialog.
 *
 * Since: 2.10
 **/
void
ctk_tree_view_set_search_position_func (CtkTreeView                   *tree_view,
				        CtkTreeViewSearchPositionFunc  func,
				        gpointer                       user_data,
				        GDestroyNotify                 destroy)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  if (tree_view->priv->search_position_destroy)
    tree_view->priv->search_position_destroy (tree_view->priv->search_position_user_data);

  tree_view->priv->search_position_func = func;
  tree_view->priv->search_position_user_data = user_data;
  tree_view->priv->search_position_destroy = destroy;
  if (tree_view->priv->search_position_func == NULL)
    tree_view->priv->search_position_func = ctk_tree_view_search_position_func;
}

/**
 * ctk_tree_view_get_search_position_func: (skip)
 * @tree_view: A #CtkTreeView
 *
 * Returns the positioning function currently in use.
 *
 * Returns: the currently used function for positioning the search dialog.
 *
 * Since: 2.10
 */
CtkTreeViewSearchPositionFunc
ctk_tree_view_get_search_position_func (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), NULL);

  return tree_view->priv->search_position_func;
}


static void
ctk_tree_view_search_window_hide (CtkWidget   *search_window,
                                  CtkTreeView *tree_view,
                                  CdkDevice   *device)
{
  if (tree_view->priv->disable_popdown)
    return;

  if (tree_view->priv->search_entry_changed_id)
    {
      g_signal_handler_disconnect (tree_view->priv->search_entry,
				   tree_view->priv->search_entry_changed_id);
      tree_view->priv->search_entry_changed_id = 0;
    }
  if (tree_view->priv->typeselect_flush_timeout)
    {
      g_source_remove (tree_view->priv->typeselect_flush_timeout);
      tree_view->priv->typeselect_flush_timeout = 0;
    }
	
  if (ctk_widget_get_visible (search_window))
    {
      /* send focus-in event */
      send_focus_change (CTK_WIDGET (tree_view->priv->search_entry), device, FALSE);
      ctk_widget_hide (search_window);
      ctk_entry_set_text (CTK_ENTRY (tree_view->priv->search_entry), "");
      send_focus_change (CTK_WIDGET (tree_view), device, TRUE);
    }
}

static void
ctk_tree_view_search_position_func (CtkTreeView *tree_view,
				    CtkWidget   *search_window,
				    gpointer     user_data G_GNUC_UNUSED)
{
  gint x, y;
  gint tree_x, tree_y;
  gint tree_width, tree_height;
  CdkDisplay *display;
  CdkMonitor *monitor;
  CdkRectangle workarea;
  CdkWindow *tree_window = ctk_widget_get_window (CTK_WIDGET (tree_view));
  CtkRequisition requisition;

  ctk_widget_realize (search_window);

  display = ctk_widget_get_display (CTK_WIDGET (tree_view));
  monitor = cdk_display_get_monitor_at_window (display, tree_window);
  cdk_monitor_get_workarea (monitor, &workarea);

  cdk_window_get_origin (tree_window, &tree_x, &tree_y);
  tree_width = cdk_window_get_width (tree_window);
  tree_height = cdk_window_get_height (tree_window);
  ctk_widget_get_preferred_size (search_window, &requisition, NULL);

  if (tree_x + tree_width > workarea.x + workarea.width)
    x = workarea.x + workarea.width - requisition.width;
  else if (tree_x + tree_width - requisition.width < workarea.x)
    x = workarea.x;
  else
    x = tree_x + tree_width - requisition.width;

  if (tree_y + tree_height + requisition.height > workarea.y + workarea.height)
    y = workarea.y + workarea.height - requisition.height;
  else if (tree_y + tree_height < workarea.y) /* isn't really possible ... */
    y = workarea.y;
  else
    y = tree_y + tree_height;

  ctk_window_move (CTK_WINDOW (search_window), x, y);
}

static void
ctk_tree_view_search_disable_popdown (CtkEntry *entry G_GNUC_UNUSED,
				      CtkMenu  *menu,
				      gpointer  data)
{
  CtkTreeView *tree_view = (CtkTreeView *)data;

  tree_view->priv->disable_popdown = 1;
  g_signal_connect (menu, "hide",
		    G_CALLBACK (ctk_tree_view_search_enable_popdown), data);
}

/* Because we're visible but offscreen, we just set a flag in the preedit
 * callback.
 */
static void
ctk_tree_view_search_preedit_changed (CtkIMContext *im_context G_GNUC_UNUSED,
				      CtkTreeView  *tree_view)
{
  tree_view->priv->imcontext_changed = 1;
  if (tree_view->priv->typeselect_flush_timeout)
    {
      g_source_remove (tree_view->priv->typeselect_flush_timeout);
      tree_view->priv->typeselect_flush_timeout =
	cdk_threads_add_timeout (CTK_TREE_VIEW_SEARCH_DIALOG_TIMEOUT,
		       (GSourceFunc) ctk_tree_view_search_entry_flush_timeout,
		       tree_view);
      g_source_set_name_by_id (tree_view->priv->typeselect_flush_timeout, "[ctk+] ctk_tree_view_search_entry_flush_timeout");
    }

}

static void
ctk_tree_view_search_commit (CtkIMContext *im_context G_GNUC_UNUSED,
                             gchar        *buf G_GNUC_UNUSED,
                             CtkTreeView  *tree_view)
{
  tree_view->priv->imcontext_changed = 1;
}

static void
ctk_tree_view_search_activate (CtkEntry    *entry G_GNUC_UNUSED,
			       CtkTreeView *tree_view)
{
  CtkTreePath *path;

  ctk_tree_view_search_window_hide (tree_view->priv->search_window,
                                    tree_view,
                                    ctk_get_current_event_device ());

  /* If we have a row selected and it's the cursor row, we activate
   * the row XXX */
  if (tree_view->priv->cursor_node &&
      CTK_RBNODE_FLAG_SET (tree_view->priv->cursor_node, CTK_RBNODE_IS_SELECTED))
    {
      path = _ctk_tree_path_new_from_rbtree (tree_view->priv->cursor_tree,
                                             tree_view->priv->cursor_node);
      
      ctk_tree_view_row_activated (tree_view, path, tree_view->priv->focus_column);
      
      ctk_tree_path_free (path);
    }
}

static gboolean
ctk_tree_view_real_search_enable_popdown (gpointer data)
{
  CtkTreeView *tree_view = (CtkTreeView *)data;

  tree_view->priv->disable_popdown = 0;

  return FALSE;
}

static void
ctk_tree_view_search_enable_popdown (CtkWidget *widget G_GNUC_UNUSED,
				     gpointer   data)
{
  guint id;
  id = cdk_threads_add_timeout_full (G_PRIORITY_HIGH, 200, ctk_tree_view_real_search_enable_popdown, g_object_ref (data), g_object_unref);
  g_source_set_name_by_id (id, "[ctk+] ctk_tree_view_real_search_enable_popdown");
}

static gboolean
ctk_tree_view_search_delete_event (CtkWidget   *widget,
				   CdkEventAny *event G_GNUC_UNUSED,
				   CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  ctk_tree_view_search_window_hide (widget, tree_view, NULL);

  return TRUE;
}

static gboolean
ctk_tree_view_search_button_press_event (CtkWidget *widget,
					 CdkEventButton *event,
					 CtkTreeView *tree_view)
{
  CdkDevice *keyb_device;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  keyb_device = cdk_device_get_associated_device (event->device);
  ctk_tree_view_search_window_hide (widget, tree_view, keyb_device);

  return TRUE;
}

static gboolean
ctk_tree_view_search_scroll_event (CtkWidget *widget,
				   CdkEventScroll *event,
				   CtkTreeView *tree_view)
{
  gboolean retval = FALSE;

  if (event->direction == CDK_SCROLL_UP)
    {
      ctk_tree_view_search_move (widget, tree_view, TRUE);
      retval = TRUE;
    }
  else if (event->direction == CDK_SCROLL_DOWN)
    {
      ctk_tree_view_search_move (widget, tree_view, FALSE);
      retval = TRUE;
    }

  /* renew the flush timeout */
  if (retval && tree_view->priv->typeselect_flush_timeout
      && !tree_view->priv->search_custom_entry_set)
    {
      g_source_remove (tree_view->priv->typeselect_flush_timeout);
      tree_view->priv->typeselect_flush_timeout =
	cdk_threads_add_timeout (CTK_TREE_VIEW_SEARCH_DIALOG_TIMEOUT,
		       (GSourceFunc) ctk_tree_view_search_entry_flush_timeout,
		       tree_view);
      g_source_set_name_by_id (tree_view->priv->typeselect_flush_timeout, "[ctk+] ctk_tree_view_search_entry_flush_timeout");
    }

  return retval;
}

static gboolean
ctk_tree_view_search_key_press_event (CtkWidget *widget,
				      CdkEventKey *event,
				      CtkTreeView *tree_view)
{
  CdkModifierType default_accel;
  gboolean        retval = FALSE;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);

  /* close window and cancel the search */
  if (!tree_view->priv->search_custom_entry_set
      && ctk_tree_view_search_key_cancels_search (event->keyval))
    {
      ctk_tree_view_search_window_hide (widget, tree_view,
                                        cdk_event_get_device ((CdkEvent *) event));
      return TRUE;
    }

  default_accel = ctk_widget_get_modifier_mask (widget,
                                                CDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR);

  /* select previous matching iter */
  if (event->keyval == CDK_KEY_Up || event->keyval == CDK_KEY_KP_Up)
    {
      if (!ctk_tree_view_search_move (widget, tree_view, TRUE))
        ctk_widget_error_bell (widget);

      retval = TRUE;
    }

  if (((event->state & (default_accel | CDK_SHIFT_MASK)) == (default_accel | CDK_SHIFT_MASK))
      && (event->keyval == CDK_KEY_g || event->keyval == CDK_KEY_G))
    {
      if (!ctk_tree_view_search_move (widget, tree_view, TRUE))
        ctk_widget_error_bell (widget);

      retval = TRUE;
    }

  /* select next matching iter */
  if (event->keyval == CDK_KEY_Down || event->keyval == CDK_KEY_KP_Down)
    {
      if (!ctk_tree_view_search_move (widget, tree_view, FALSE))
        ctk_widget_error_bell (widget);

      retval = TRUE;
    }

  if (((event->state & (default_accel | CDK_SHIFT_MASK)) == default_accel)
      && (event->keyval == CDK_KEY_g || event->keyval == CDK_KEY_G))
    {
      if (!ctk_tree_view_search_move (widget, tree_view, FALSE))
        ctk_widget_error_bell (widget);

      retval = TRUE;
    }

  /* renew the flush timeout */
  if (retval && tree_view->priv->typeselect_flush_timeout
      && !tree_view->priv->search_custom_entry_set)
    {
      g_source_remove (tree_view->priv->typeselect_flush_timeout);
      tree_view->priv->typeselect_flush_timeout =
	cdk_threads_add_timeout (CTK_TREE_VIEW_SEARCH_DIALOG_TIMEOUT,
		       (GSourceFunc) ctk_tree_view_search_entry_flush_timeout,
		       tree_view);
      g_source_set_name_by_id (tree_view->priv->typeselect_flush_timeout, "[ctk+] ctk_tree_view_search_entry_flush_timeout");
    }

  return retval;
}

/*  this function returns FALSE if there is a search string but
 *  nothing was found, and TRUE otherwise.
 */
static gboolean
ctk_tree_view_search_move (CtkWidget   *window G_GNUC_UNUSED,
			   CtkTreeView *tree_view,
			   gboolean     up)
{
  gboolean ret;
  gint len;
  gint count = 0;
  const gchar *text;
  CtkTreeIter iter;
  CtkTreeModel *model;
  CtkTreeSelection *selection;

  text = ctk_entry_get_text (CTK_ENTRY (tree_view->priv->search_entry));

  g_return_val_if_fail (text != NULL, FALSE);

  len = strlen (text);

  if (up && tree_view->priv->selected_iter == 1)
    return len < 1;

  if (len < 1)
    return TRUE;

  model = ctk_tree_view_get_model (tree_view);
  selection = ctk_tree_view_get_selection (tree_view);

  /* search */
  ctk_tree_selection_unselect_all (selection);
  if (!ctk_tree_model_get_iter_first (model, &iter))
    return TRUE;

  ret = ctk_tree_view_search_iter (model, selection, &iter, text,
				   &count, up?((tree_view->priv->selected_iter) - 1):((tree_view->priv->selected_iter + 1)));

  if (ret)
    {
      /* found */
      tree_view->priv->selected_iter += up?(-1):(1);
      return TRUE;
    }
  else
    {
      /* return to old iter */
      count = 0;
      ctk_tree_model_get_iter_first (model, &iter);
      ctk_tree_view_search_iter (model, selection,
				 &iter, text,
				 &count, tree_view->priv->selected_iter);
      return FALSE;
    }
}

static gboolean
ctk_tree_view_search_equal_func (CtkTreeModel *model,
				 gint          column,
				 const gchar  *key,
				 CtkTreeIter  *iter,
				 gpointer      search_data G_GNUC_UNUSED)
{
  gboolean retval = TRUE;
  const gchar *str;
  gchar *normalized_string;
  gchar *normalized_key;
  gchar *case_normalized_string = NULL;
  gchar *case_normalized_key = NULL;
  GValue value = G_VALUE_INIT;
  GValue transformed = G_VALUE_INIT;

  ctk_tree_model_get_value (model, iter, column, &value);

  g_value_init (&transformed, G_TYPE_STRING);

  if (!g_value_transform (&value, &transformed))
    {
      g_value_unset (&value);
      return TRUE;
    }

  g_value_unset (&value);

  str = g_value_get_string (&transformed);
  if (!str)
    {
      g_value_unset (&transformed);
      return TRUE;
    }

  normalized_string = g_utf8_normalize (str, -1, G_NORMALIZE_ALL);
  normalized_key = g_utf8_normalize (key, -1, G_NORMALIZE_ALL);

  if (normalized_string && normalized_key)
    {
      case_normalized_string = g_utf8_casefold (normalized_string, -1);
      case_normalized_key = g_utf8_casefold (normalized_key, -1);

      if (strncmp (case_normalized_key, case_normalized_string, strlen (case_normalized_key)) == 0)
        retval = FALSE;
    }

  g_value_unset (&transformed);
  g_free (normalized_key);
  g_free (normalized_string);
  g_free (case_normalized_key);
  g_free (case_normalized_string);

  return retval;
}

static gboolean
ctk_tree_view_search_iter (CtkTreeModel     *model,
			   CtkTreeSelection *selection,
			   CtkTreeIter      *iter,
			   const gchar      *text,
			   gint             *count,
			   gint              n)
{
  CtkRBTree *tree = NULL;
  CtkRBNode *node = NULL;
  CtkTreePath *path;

  CtkTreeView *tree_view = ctk_tree_selection_get_tree_view (selection);

  path = ctk_tree_model_get_path (model, iter);
  _ctk_tree_view_find_node (tree_view, path, &tree, &node);

  do
    {
      if (! tree_view->priv->search_equal_func (model, tree_view->priv->search_column, text, iter, tree_view->priv->search_user_data))
        {
          (*count)++;
          if (*count == n)
            {
              ctk_tree_view_scroll_to_cell (tree_view, path, NULL,
					    TRUE, 0.5, 0.0);
              ctk_tree_selection_select_iter (selection, iter);
              ctk_tree_view_real_set_cursor (tree_view, path, CLAMP_NODE);

	      if (path)
		ctk_tree_path_free (path);

              return TRUE;
            }
        }

      if (node->children)
	{
	  gboolean has_child;
	  CtkTreeIter tmp;

	  tree = node->children;
          node = _ctk_rbtree_first (tree);

	  tmp = *iter;
	  has_child = ctk_tree_model_iter_children (model, iter, &tmp);
	  ctk_tree_path_down (path);

	  /* sanity check */
	  TREE_VIEW_INTERNAL_ASSERT (has_child, FALSE);
	}
      else
	{
	  gboolean done = FALSE;

	  do
	    {
	      node = _ctk_rbtree_next (tree, node);

	      if (node)
		{
		  gboolean has_next;

		  has_next = ctk_tree_model_iter_next (model, iter);

		  done = TRUE;
		  ctk_tree_path_next (path);

		  /* sanity check */
		  TREE_VIEW_INTERNAL_ASSERT (has_next, FALSE);
		}
	      else
		{
		  gboolean has_parent;
		  CtkTreeIter tmp_iter = *iter;

		  node = tree->parent_node;
		  tree = tree->parent_tree;

		  if (!tree)
		    {
		      if (path)
			ctk_tree_path_free (path);

		      /* we've run out of tree, done with this func */
		      return FALSE;
		    }

		  has_parent = ctk_tree_model_iter_parent (model,
							   iter,
							   &tmp_iter);
		  ctk_tree_path_up (path);

		  /* sanity check */
		  TREE_VIEW_INTERNAL_ASSERT (has_parent, FALSE);
		}
	    }
	  while (!done);
	}
    }
  while (1);

  return FALSE;
}

static void
ctk_tree_view_search_init (CtkWidget   *entry,
			   CtkTreeView *tree_view)
{
  gint ret;
  gint count = 0;
  const gchar *text;
  CtkTreeIter iter;
  CtkTreeModel *model;
  CtkTreeSelection *selection;

  g_return_if_fail (CTK_IS_ENTRY (entry));
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  text = ctk_entry_get_text (CTK_ENTRY (entry));

  model = ctk_tree_view_get_model (tree_view);
  selection = ctk_tree_view_get_selection (tree_view);

  /* search */
  ctk_tree_selection_unselect_all (selection);
  if (tree_view->priv->typeselect_flush_timeout
      && !tree_view->priv->search_custom_entry_set)
    {
      g_source_remove (tree_view->priv->typeselect_flush_timeout);
      tree_view->priv->typeselect_flush_timeout =
	cdk_threads_add_timeout (CTK_TREE_VIEW_SEARCH_DIALOG_TIMEOUT,
		       (GSourceFunc) ctk_tree_view_search_entry_flush_timeout,
		       tree_view);
      g_source_set_name_by_id (tree_view->priv->typeselect_flush_timeout, "[ctk+] ctk_tree_view_search_entry_flush_timeout");
    }

  if (*text == '\0')
    return;

  if (!ctk_tree_model_get_iter_first (model, &iter))
    return;

  ret = ctk_tree_view_search_iter (model, selection,
				   &iter, text,
				   &count, 1);

  if (ret)
    tree_view->priv->selected_iter = 1;
}

void
_ctk_tree_view_remove_editable (CtkTreeView       *tree_view,
                                CtkTreeViewColumn *column,
                                CtkCellEditable   *cell_editable)
{
  if (tree_view->priv->edited_column == NULL)
    return;

  g_return_if_fail (column == tree_view->priv->edited_column);

  tree_view->priv->edited_column = NULL;

  if (ctk_widget_has_focus (CTK_WIDGET (cell_editable)))
    ctk_widget_grab_focus (CTK_WIDGET (tree_view));

  ctk_container_remove (CTK_CONTAINER (tree_view),
                        CTK_WIDGET (cell_editable));

  /* FIXME should only redraw a single node */
  ctk_widget_queue_draw (CTK_WIDGET (tree_view));
}

static gboolean
ctk_tree_view_start_editing (CtkTreeView *tree_view,
			     CtkTreePath *cursor_path,
			     gboolean     edit_only)
{
  CtkTreeIter iter;
  CdkRectangle cell_area;
  CtkTreeViewColumn *focus_column;
  guint flags = 0; /* can be 0, as the flags are primarily for rendering */
  gint retval = FALSE;
  CtkRBTree *cursor_tree;
  CtkRBNode *cursor_node;

  g_assert (tree_view->priv->focus_column);
  focus_column = tree_view->priv->focus_column;

  if (!ctk_widget_get_realized (CTK_WIDGET (tree_view)))
    return FALSE;

  if (_ctk_tree_view_find_node (tree_view, cursor_path, &cursor_tree, &cursor_node) ||
      cursor_node == NULL)
    return FALSE;

  ctk_tree_model_get_iter (tree_view->priv->model, &iter, cursor_path);

  validate_row (tree_view, cursor_tree, cursor_node, &iter, cursor_path);

  ctk_tree_view_column_cell_set_cell_data (focus_column,
                                           tree_view->priv->model,
                                           &iter,
                                           CTK_RBNODE_FLAG_SET (cursor_node, CTK_RBNODE_IS_PARENT),
                                           cursor_node->children ? TRUE : FALSE);
  ctk_tree_view_get_cell_area (tree_view,
                               cursor_path,
                               focus_column,
                               &cell_area);

  if (ctk_cell_area_activate (ctk_cell_layout_get_area (CTK_CELL_LAYOUT (focus_column)),
                              _ctk_tree_view_column_get_context (focus_column),
                              CTK_WIDGET (tree_view),
                              &cell_area,
                              flags, edit_only))
    retval = TRUE;

  return retval;
}

void
_ctk_tree_view_add_editable (CtkTreeView       *tree_view,
                             CtkTreeViewColumn *column,
                             CtkTreePath       *path,
                             CtkCellEditable   *cell_editable,
                             CdkRectangle      *cell_area)
{
  CdkRectangle full_area;
  CtkBorder border;

  tree_view->priv->edited_column = column;

  ctk_tree_view_real_set_cursor (tree_view, path, CLAMP_NODE);

  tree_view->priv->draw_keyfocus = TRUE;

  ctk_tree_view_get_cell_area (tree_view, path, column, &full_area);
  border.left = cell_area->x - full_area.x;
  border.top = cell_area->y - full_area.y;
  border.right = (full_area.x + full_area.width) - (cell_area->x + cell_area->width);
  border.bottom = (full_area.y + full_area.height) - (cell_area->y + cell_area->height);

  ctk_tree_view_put (tree_view,
                     CTK_WIDGET (cell_editable),
                     path,
                     column,
                     &border);
}

static void
ctk_tree_view_stop_editing (CtkTreeView *tree_view,
			    gboolean     cancel_editing)
{
  CtkTreeViewColumn *column;

  if (tree_view->priv->edited_column == NULL)
    return;

  /*
   * This is very evil. We need to do this, because
   * ctk_cell_editable_editing_done may trigger ctk_tree_view_row_changed
   * later on. If ctk_tree_view_row_changed notices
   * tree_view->priv->edited_column != NULL, it'll call
   * ctk_tree_view_stop_editing again. Bad things will happen then.
   *
   * Please read that again if you intend to modify anything here.
   */

  column = tree_view->priv->edited_column;
  ctk_cell_area_stop_editing (ctk_cell_layout_get_area (CTK_CELL_LAYOUT (column)), cancel_editing);
  tree_view->priv->edited_column = NULL;
}


/**
 * ctk_tree_view_set_hover_selection:
 * @tree_view: a #CtkTreeView
 * @hover: %TRUE to enable hover selection mode
 *
 * Enables or disables the hover selection mode of @tree_view.
 * Hover selection makes the selected row follow the pointer.
 * Currently, this works only for the selection modes 
 * %CTK_SELECTION_SINGLE and %CTK_SELECTION_BROWSE.
 * 
 * Since: 2.6
 **/
void     
ctk_tree_view_set_hover_selection (CtkTreeView *tree_view,
				   gboolean     hover)
{
  hover = hover != FALSE;

  if (hover != tree_view->priv->hover_selection)
    {
      tree_view->priv->hover_selection = hover;

      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_HOVER_SELECTION]);
    }
}

/**
 * ctk_tree_view_get_hover_selection:
 * @tree_view: a #CtkTreeView
 * 
 * Returns whether hover selection mode is turned on for @tree_view.
 * 
 * Returns: %TRUE if @tree_view is in hover selection mode
 *
 * Since: 2.6 
 **/
gboolean 
ctk_tree_view_get_hover_selection (CtkTreeView *tree_view)
{
  return tree_view->priv->hover_selection;
}

/**
 * ctk_tree_view_set_hover_expand:
 * @tree_view: a #CtkTreeView
 * @expand: %TRUE to enable hover selection mode
 *
 * Enables or disables the hover expansion mode of @tree_view.
 * Hover expansion makes rows expand or collapse if the pointer 
 * moves over them.
 * 
 * Since: 2.6
 **/
void     
ctk_tree_view_set_hover_expand (CtkTreeView *tree_view,
				gboolean     expand)
{
  expand = expand != FALSE;

  if (expand != tree_view->priv->hover_expand)
    {
      tree_view->priv->hover_expand = expand;

      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_HOVER_EXPAND]);
    }
}

/**
 * ctk_tree_view_get_hover_expand:
 * @tree_view: a #CtkTreeView
 * 
 * Returns whether hover expansion mode is turned on for @tree_view.
 * 
 * Returns: %TRUE if @tree_view is in hover expansion mode
 *
 * Since: 2.6 
 **/
gboolean 
ctk_tree_view_get_hover_expand (CtkTreeView *tree_view)
{
  return tree_view->priv->hover_expand;
}

/**
 * ctk_tree_view_set_rubber_banding:
 * @tree_view: a #CtkTreeView
 * @enable: %TRUE to enable rubber banding
 *
 * Enables or disables rubber banding in @tree_view.  If the selection mode
 * is #CTK_SELECTION_MULTIPLE, rubber banding will allow the user to select
 * multiple rows by dragging the mouse.
 * 
 * Since: 2.10
 **/
void
ctk_tree_view_set_rubber_banding (CtkTreeView *tree_view,
				  gboolean     enable)
{
  enable = enable != FALSE;

  if (enable != tree_view->priv->rubber_banding_enable)
    {
      tree_view->priv->rubber_banding_enable = enable;

      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_RUBBER_BANDING]);
    }
}

/**
 * ctk_tree_view_get_rubber_banding:
 * @tree_view: a #CtkTreeView
 * 
 * Returns whether rubber banding is turned on for @tree_view.  If the
 * selection mode is #CTK_SELECTION_MULTIPLE, rubber banding will allow the
 * user to select multiple rows by dragging the mouse.
 * 
 * Returns: %TRUE if rubber banding in @tree_view is enabled.
 *
 * Since: 2.10
 **/
gboolean
ctk_tree_view_get_rubber_banding (CtkTreeView *tree_view)
{
  return tree_view->priv->rubber_banding_enable;
}

/**
 * ctk_tree_view_is_rubber_banding_active:
 * @tree_view: a #CtkTreeView
 * 
 * Returns whether a rubber banding operation is currently being done
 * in @tree_view.
 *
 * Returns: %TRUE if a rubber banding operation is currently being
 * done in @tree_view.
 *
 * Since: 2.12
 **/
gboolean
ctk_tree_view_is_rubber_banding_active (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);

  if (tree_view->priv->rubber_banding_enable
      && tree_view->priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    return TRUE;

  return FALSE;
}

/**
 * ctk_tree_view_get_row_separator_func: (skip)
 * @tree_view: a #CtkTreeView
 * 
 * Returns the current row separator function.
 * 
 * Returns: the current row separator function.
 *
 * Since: 2.6
 **/
CtkTreeViewRowSeparatorFunc 
ctk_tree_view_get_row_separator_func (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), NULL);

  return tree_view->priv->row_separator_func;
}

/**
 * ctk_tree_view_set_row_separator_func:
 * @tree_view: a #CtkTreeView
 * @func: (allow-none): a #CtkTreeViewRowSeparatorFunc
 * @data: (allow-none): user data to pass to @func, or %NULL
 * @destroy: (allow-none): destroy notifier for @data, or %NULL
 * 
 * Sets the row separator function, which is used to determine
 * whether a row should be drawn as a separator. If the row separator
 * function is %NULL, no separators are drawn. This is the default value.
 *
 * Since: 2.6
 **/
void
ctk_tree_view_set_row_separator_func (CtkTreeView                 *tree_view,
				      CtkTreeViewRowSeparatorFunc  func,
				      gpointer                     data,
				      GDestroyNotify               destroy)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  if (tree_view->priv->row_separator_destroy)
    tree_view->priv->row_separator_destroy (tree_view->priv->row_separator_data);

  tree_view->priv->row_separator_func = func;
  tree_view->priv->row_separator_data = data;
  tree_view->priv->row_separator_destroy = destroy;

  /* Have the tree recalculate heights */
  _ctk_rbtree_mark_invalid (tree_view->priv->tree);
  ctk_widget_queue_resize (CTK_WIDGET (tree_view));
}

/**
 * ctk_tree_view_get_grid_lines:
 * @tree_view: a #CtkTreeView
 *
 * Returns which grid lines are enabled in @tree_view.
 *
 * Returns: a #CtkTreeViewGridLines value indicating which grid lines
 * are enabled.
 *
 * Since: 2.10
 */
CtkTreeViewGridLines
ctk_tree_view_get_grid_lines (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), 0);

  return tree_view->priv->grid_lines;
}

/**
 * ctk_tree_view_set_grid_lines:
 * @tree_view: a #CtkTreeView
 * @grid_lines: a #CtkTreeViewGridLines value indicating which grid lines to
 * enable.
 *
 * Sets which grid lines to draw in @tree_view.
 *
 * Since: 2.10
 */
void
ctk_tree_view_set_grid_lines (CtkTreeView           *tree_view,
			      CtkTreeViewGridLines   grid_lines)
{
  CtkTreeViewPrivate *priv;
  CtkWidget *widget;
  CtkTreeViewGridLines old_grid_lines;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  priv = tree_view->priv;
  widget = CTK_WIDGET (tree_view);

  old_grid_lines = priv->grid_lines;
  priv->grid_lines = grid_lines;
  
  if (ctk_widget_get_realized (widget))
    {
      if (grid_lines == CTK_TREE_VIEW_GRID_LINES_NONE &&
	  priv->grid_line_width)
	{
	  priv->grid_line_width = 0;
	}
      
      if (grid_lines != CTK_TREE_VIEW_GRID_LINES_NONE && 
	  !priv->grid_line_width)
	{
	  gint8 *dash_list;

	  ctk_widget_style_get (widget,
				"grid-line-width", &priv->grid_line_width,
				"grid-line-pattern", (gchar *)&dash_list,
				NULL);
      
          if (dash_list)
            {
              priv->grid_line_dashes[0] = dash_list[0];
              if (dash_list[0])
                priv->grid_line_dashes[1] = dash_list[1];
	      
              g_free (dash_list);
            }
          else
            {
              priv->grid_line_dashes[0] = 1;
              priv->grid_line_dashes[1] = 1;
            }
	}      
    }

  if (old_grid_lines != grid_lines)
    {
      ctk_widget_queue_draw (CTK_WIDGET (tree_view));
      
      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_ENABLE_GRID_LINES]);
    }
}

/**
 * ctk_tree_view_get_enable_tree_lines:
 * @tree_view: a #CtkTreeView.
 *
 * Returns whether or not tree lines are drawn in @tree_view.
 *
 * Returns: %TRUE if tree lines are drawn in @tree_view, %FALSE
 * otherwise.
 *
 * Since: 2.10
 */
gboolean
ctk_tree_view_get_enable_tree_lines (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);

  return tree_view->priv->tree_lines_enabled;
}

/**
 * ctk_tree_view_set_enable_tree_lines:
 * @tree_view: a #CtkTreeView
 * @enabled: %TRUE to enable tree line drawing, %FALSE otherwise.
 *
 * Sets whether to draw lines interconnecting the expanders in @tree_view.
 * This does not have any visible effects for lists.
 *
 * Since: 2.10
 */
void
ctk_tree_view_set_enable_tree_lines (CtkTreeView *tree_view,
				     gboolean     enabled)
{
  CtkTreeViewPrivate *priv;
  CtkWidget *widget;
  gboolean was_enabled;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  enabled = enabled != FALSE;

  priv = tree_view->priv;
  widget = CTK_WIDGET (tree_view);

  was_enabled = priv->tree_lines_enabled;

  priv->tree_lines_enabled = enabled;

  if (ctk_widget_get_realized (widget))
    {
      if (!enabled && priv->tree_line_width)
	{
          priv->tree_line_width = 0;
	}
      
      if (enabled && !priv->tree_line_width)
	{
	  gint8 *dash_list;
	  ctk_widget_style_get (widget,
				"tree-line-width", &priv->tree_line_width,
				"tree-line-pattern", (gchar *)&dash_list,
				NULL);
	  
          if (dash_list)
            {
              priv->tree_line_dashes[0] = dash_list[0];
              if (dash_list[0])
                priv->tree_line_dashes[1] = dash_list[1];
	      
              g_free (dash_list);
            }
          else
            {
              priv->tree_line_dashes[0] = 1;
              priv->tree_line_dashes[1] = 1;
            }
	}
    }

  if (was_enabled != enabled)
    {
      ctk_widget_queue_draw (CTK_WIDGET (tree_view));

      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_ENABLE_TREE_LINES]);
    }
}


/**
 * ctk_tree_view_set_show_expanders:
 * @tree_view: a #CtkTreeView
 * @enabled: %TRUE to enable expander drawing, %FALSE otherwise.
 *
 * Sets whether to draw and enable expanders and indent child rows in
 * @tree_view.  When disabled there will be no expanders visible in trees
 * and there will be no way to expand and collapse rows by default.  Also
 * note that hiding the expanders will disable the default indentation.  You
 * can set a custom indentation in this case using
 * ctk_tree_view_set_level_indentation().
 * This does not have any visible effects for lists.
 *
 * Since: 2.12
 */
void
ctk_tree_view_set_show_expanders (CtkTreeView *tree_view,
				  gboolean     enabled)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  enabled = enabled != FALSE;
  if (tree_view->priv->show_expanders != enabled)
    {
      tree_view->priv->show_expanders = enabled;
      ctk_widget_queue_draw (CTK_WIDGET (tree_view));
      g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_SHOW_EXPANDERS]);
    }
}

/**
 * ctk_tree_view_get_show_expanders:
 * @tree_view: a #CtkTreeView.
 *
 * Returns whether or not expanders are drawn in @tree_view.
 *
 * Returns: %TRUE if expanders are drawn in @tree_view, %FALSE
 * otherwise.
 *
 * Since: 2.12
 */
gboolean
ctk_tree_view_get_show_expanders (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);

  return tree_view->priv->show_expanders;
}

/**
 * ctk_tree_view_set_level_indentation:
 * @tree_view: a #CtkTreeView
 * @indentation: the amount, in pixels, of extra indentation in @tree_view.
 *
 * Sets the amount of extra indentation for child levels to use in @tree_view
 * in addition to the default indentation.  The value should be specified in
 * pixels, a value of 0 disables this feature and in this case only the default
 * indentation will be used.
 * This does not have any visible effects for lists.
 *
 * Since: 2.12
 */
void
ctk_tree_view_set_level_indentation (CtkTreeView *tree_view,
				     gint         indentation)
{
  tree_view->priv->level_indentation = indentation;

  ctk_widget_queue_draw (CTK_WIDGET (tree_view));
}

/**
 * ctk_tree_view_get_level_indentation:
 * @tree_view: a #CtkTreeView.
 *
 * Returns the amount, in pixels, of extra indentation for child levels
 * in @tree_view.
 *
 * Returns: the amount of extra indentation for child levels in
 * @tree_view.  A return value of 0 means that this feature is disabled.
 *
 * Since: 2.12
 */
gint
ctk_tree_view_get_level_indentation (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), 0);

  return tree_view->priv->level_indentation;
}

/**
 * ctk_tree_view_set_tooltip_row:
 * @tree_view: a #CtkTreeView
 * @tooltip: a #CtkTooltip
 * @path: a #CtkTreePath
 *
 * Sets the tip area of @tooltip to be the area covered by the row at @path.
 * See also ctk_tree_view_set_tooltip_column() for a simpler alternative.
 * See also ctk_tooltip_set_tip_area().
 *
 * Since: 2.12
 */
void
ctk_tree_view_set_tooltip_row (CtkTreeView *tree_view,
			       CtkTooltip  *tooltip,
			       CtkTreePath *path)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (CTK_IS_TOOLTIP (tooltip));

  ctk_tree_view_set_tooltip_cell (tree_view, tooltip, path, NULL, NULL);
}

/**
 * ctk_tree_view_set_tooltip_cell:
 * @tree_view: a #CtkTreeView
 * @tooltip: a #CtkTooltip
 * @path: (allow-none): a #CtkTreePath or %NULL
 * @column: (allow-none): a #CtkTreeViewColumn or %NULL
 * @cell: (allow-none): a #CtkCellRenderer or %NULL
 *
 * Sets the tip area of @tooltip to the area @path, @column and @cell have
 * in common.  For example if @path is %NULL and @column is set, the tip
 * area will be set to the full area covered by @column.  See also
 * ctk_tooltip_set_tip_area().
 *
 * Note that if @path is not specified and @cell is set and part of a column
 * containing the expander, the tooltip might not show and hide at the correct
 * position.  In such cases @path must be set to the current node under the
 * mouse cursor for this function to operate correctly.
 *
 * See also ctk_tree_view_set_tooltip_column() for a simpler alternative.
 *
 * Since: 2.12
 */
void
ctk_tree_view_set_tooltip_cell (CtkTreeView       *tree_view,
				CtkTooltip        *tooltip,
				CtkTreePath       *path,
				CtkTreeViewColumn *column,
				CtkCellRenderer   *cell)
{
  CtkAllocation allocation;
  CdkRectangle rect;

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (CTK_IS_TOOLTIP (tooltip));
  g_return_if_fail (column == NULL || CTK_IS_TREE_VIEW_COLUMN (column));
  g_return_if_fail (cell == NULL || CTK_IS_CELL_RENDERER (cell));

  /* Determine x values. */
  if (column && cell)
    {
      CdkRectangle tmp;
      gint start, width;

      /* We always pass in path here, whether it is NULL or not.
       * For cells in expander columns path must be specified so that
       * we can correctly account for the indentation.  This also means
       * that the tooltip is constrained vertically by the "Determine y
       * values" code below; this is not a real problem since cells actually
       * don't stretch vertically in constrast to columns.
       */
      ctk_tree_view_get_cell_area (tree_view, path, column, &tmp);
      ctk_tree_view_column_cell_get_position (column, cell, &start, &width);

      ctk_tree_view_convert_bin_window_to_widget_coords (tree_view,
							 tmp.x + start, 0,
							 &rect.x, NULL);
      rect.width = width;
    }
  else if (column)
    {
      CdkRectangle tmp;

      ctk_tree_view_get_background_area (tree_view, NULL, column, &tmp);
      ctk_tree_view_convert_bin_window_to_widget_coords (tree_view,
							 tmp.x, 0,
							 &rect.x, NULL);
      rect.width = tmp.width;
    }
  else
    {
      ctk_widget_get_allocation (CTK_WIDGET (tree_view), &allocation);
      rect.x = 0;
      rect.width = allocation.width;
    }

  /* Determine y values. */
  if (path)
    {
      CdkRectangle tmp;

      ctk_tree_view_get_background_area (tree_view, path, NULL, &tmp);
      ctk_tree_view_convert_bin_window_to_widget_coords (tree_view,
							 0, tmp.y,
							 NULL, &rect.y);
      rect.height = tmp.height;
    }
  else
    {
      rect.y = 0;
      rect.height = ctk_adjustment_get_page_size (tree_view->priv->vadjustment);
    }

  ctk_tooltip_set_tip_area (tooltip, &rect);
}

/**
 * ctk_tree_view_get_tooltip_context:
 * @tree_view: a #CtkTreeView
 * @x: (inout): the x coordinate (relative to widget coordinates)
 * @y: (inout): the y coordinate (relative to widget coordinates)
 * @keyboard_tip: whether this is a keyboard tooltip or not
 * @model: (out) (optional) (nullable) (transfer none): a pointer to
 *         receive a #CtkTreeModel or %NULL
 * @path: (out) (optional): a pointer to receive a #CtkTreePath or %NULL
 * @iter: (out) (optional): a pointer to receive a #CtkTreeIter or %NULL
 *
 * This function is supposed to be used in a #CtkWidget::query-tooltip
 * signal handler for #CtkTreeView.  The @x, @y and @keyboard_tip values
 * which are received in the signal handler, should be passed to this
 * function without modification.
 *
 * The return value indicates whether there is a tree view row at the given
 * coordinates (%TRUE) or not (%FALSE) for mouse tooltips.  For keyboard
 * tooltips the row returned will be the cursor row.  When %TRUE, then any of
 * @model, @path and @iter which have been provided will be set to point to
 * that row and the corresponding model.  @x and @y will always be converted
 * to be relative to @tree_view’s bin_window if @keyboard_tooltip is %FALSE.
 *
 * Returns: whether or not the given tooltip context points to a row.
 *
 * Since: 2.12
 */
gboolean
ctk_tree_view_get_tooltip_context (CtkTreeView   *tree_view,
				   gint          *x,
				   gint          *y,
				   gboolean       keyboard_tip,
				   CtkTreeModel **model,
				   CtkTreePath  **path,
				   CtkTreeIter   *iter)
{
  CtkTreePath *tmppath = NULL;

  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), FALSE);
  g_return_val_if_fail (x != NULL, FALSE);
  g_return_val_if_fail (y != NULL, FALSE);

  if (keyboard_tip)
    {
      ctk_tree_view_get_cursor (tree_view, &tmppath, NULL);

      if (!tmppath)
	return FALSE;
    }
  else
    {
      ctk_tree_view_convert_widget_to_bin_window_coords (tree_view, *x, *y,
							 x, y);

      if (!ctk_tree_view_get_path_at_pos (tree_view, *x, *y,
					  &tmppath, NULL, NULL, NULL))
	return FALSE;
    }

  if (model)
    *model = ctk_tree_view_get_model (tree_view);

  if (iter)
    ctk_tree_model_get_iter (ctk_tree_view_get_model (tree_view),
			     iter, tmppath);

  if (path)
    *path = tmppath;
  else
    ctk_tree_path_free (tmppath);

  return TRUE;
}

static gboolean
ctk_tree_view_set_tooltip_query_cb (CtkWidget  *widget,
				    gint        x,
				    gint        y,
				    gboolean    keyboard_tip,
				    CtkTooltip *tooltip,
				    gpointer    data G_GNUC_UNUSED)
{
  GValue value = G_VALUE_INIT;
  GValue transformed = G_VALUE_INIT;
  CtkTreeIter iter;
  CtkTreePath *path;
  CtkTreeModel *model;
  CtkTreeView *tree_view = CTK_TREE_VIEW (widget);
  const char *transformed_str = NULL;

  if (!ctk_tree_view_get_tooltip_context (CTK_TREE_VIEW (widget),
					  &x, &y,
					  keyboard_tip,
					  &model, &path, &iter))
    return FALSE;

  ctk_tree_model_get_value (model, &iter,
                            tree_view->priv->tooltip_column, &value);

  g_value_init (&transformed, G_TYPE_STRING);

  if (!g_value_transform (&value, &transformed))
    {
      g_value_unset (&value);
      ctk_tree_path_free (path);

      return FALSE;
    }

  g_value_unset (&value);

  transformed_str = g_value_get_string (&transformed);
  if (transformed_str == NULL || *transformed_str == '\0')
    {
      g_value_unset (&transformed);
      ctk_tree_path_free (path);

      return FALSE;
    }

  ctk_tooltip_set_markup (tooltip, transformed_str);
  ctk_tree_view_set_tooltip_row (tree_view, tooltip, path);

  ctk_tree_path_free (path);
  g_value_unset (&transformed);

  return TRUE;
}

/**
 * ctk_tree_view_set_tooltip_column:
 * @tree_view: a #CtkTreeView
 * @column: an integer, which is a valid column number for @tree_view’s model
 *
 * If you only plan to have simple (text-only) tooltips on full rows, you
 * can use this function to have #CtkTreeView handle these automatically
 * for you. @column should be set to the column in @tree_view’s model
 * containing the tooltip texts, or -1 to disable this feature.
 *
 * When enabled, #CtkWidget:has-tooltip will be set to %TRUE and
 * @tree_view will connect a #CtkWidget::query-tooltip signal handler.
 *
 * Note that the signal handler sets the text with ctk_tooltip_set_markup(),
 * so &, <, etc have to be escaped in the text.
 *
 * Since: 2.12
 */
void
ctk_tree_view_set_tooltip_column (CtkTreeView *tree_view,
			          gint         column)
{
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  if (column == tree_view->priv->tooltip_column)
    return;

  if (column == -1)
    {
      g_signal_handlers_disconnect_by_func (tree_view,
	  				    ctk_tree_view_set_tooltip_query_cb,
					    NULL);
      ctk_widget_set_has_tooltip (CTK_WIDGET (tree_view), FALSE);
    }
  else
    {
      if (tree_view->priv->tooltip_column == -1)
        {
          g_signal_connect (tree_view, "query-tooltip",
		            G_CALLBACK (ctk_tree_view_set_tooltip_query_cb), NULL);
          ctk_widget_set_has_tooltip (CTK_WIDGET (tree_view), TRUE);
        }
    }

  tree_view->priv->tooltip_column = column;
  g_object_notify_by_pspec (G_OBJECT (tree_view), tree_view_props[PROP_TOOLTIP_COLUMN]);
}

/**
 * ctk_tree_view_get_tooltip_column:
 * @tree_view: a #CtkTreeView
 *
 * Returns the column of @tree_view’s model which is being used for
 * displaying tooltips on @tree_view’s rows.
 *
 * Returns: the index of the tooltip column that is currently being
 * used, or -1 if this is disabled.
 *
 * Since: 2.12
 */
gint
ctk_tree_view_get_tooltip_column (CtkTreeView *tree_view)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), 0);

  return tree_view->priv->tooltip_column;
}

static gboolean
ctk_tree_view_get_border (CtkScrollable *scrollable,
                          CtkBorder     *border)
{
  border->top = _ctk_tree_view_get_header_height (CTK_TREE_VIEW (scrollable));

  return TRUE;
}

static void
ctk_tree_view_scrollable_init (CtkScrollableInterface *iface)
{
  iface->get_border = ctk_tree_view_get_border;
}
