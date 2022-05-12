/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* CTK - The GIMP Toolkit
 * ctktextview.c Copyright (C) 2000 Red Hat, Inc.
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include <string.h>

#define CTK_TEXT_USE_INTERNAL_UNSUPPORTED_API
#include "ctkadjustmentprivate.h"
#include "ctkbindings.h"
#include "ctkdnd.h"
#include "ctkdebug.h"
#include "ctkintl.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkmenu.h"
#include "ctkmenuitem.h"
#include "ctkrenderbackgroundprivate.h"
#include "ctkseparatormenuitem.h"
#include "ctksettings.h"
#include "ctkselectionprivate.h"
#include "ctktextbufferrichtext.h"
#include "ctktextdisplay.h"
#include "ctktextview.h"
#include "ctkimmulticontext.h"
#include "ctkprivate.h"
#include "ctktextutil.h"
#include "ctkwidgetprivate.h"
#include "ctkwindow.h"
#include "ctkscrollable.h"
#include "ctktypebuiltins.h"
#include "ctktexthandleprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkpopover.h"
#include "ctktoolbar.h"
#include "ctkpixelcacheprivate.h"
#include "ctkmagnifierprivate.h"
#include "ctkemojichooser.h"
#include "ctkpango.h"

#include "a11y/ctktextviewaccessibleprivate.h"

/**
 * SECTION:ctktextview
 * @Short_description: Widget that displays a CtkTextBuffer
 * @Title: CtkTextView
 * @See_also: #CtkTextBuffer, #CtkTextIter
 *
 * You may wish to begin by reading the
 * [text widget conceptual overview][TextWidget]
 * which gives an overview of all the objects and data
 * types related to the text widget and how they work together.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * textview.view
 * ├── border.top
 * ├── border.left
 * ├── text
 * │   ╰── [selection]
 * ├── border.right
 * ├── border.bottom
 * ╰── [window.popup]
 * ]|
 *
 * CtkTextView has a main css node with name textview and style class .view,
 * and subnodes for each of the border windows, and the main text area,
 * with names border and text, respectively. The border nodes each get
 * one of the style classes .left, .right, .top or .bottom.
 *
 * A node representing the selection will appear below the text node.
 *
 * If a context menu is opened, the window node will appear as a subnode
 * of the main node.
 */


/* How scrolling, validation, exposes, etc. work.
 *
 * The expose_event handler has the invariant that the onscreen lines
 * have been validated.
 *
 * There are two ways that onscreen lines can become invalid. The first
 * is to change which lines are onscreen. This happens when the value
 * of a scroll adjustment changes. So the code path begins in
 * ctk_text_view_value_changed() and goes like this:
 *   - cdk_window_scroll() to reflect the new adjustment value
 *   - validate the lines that were moved onscreen
 *   - cdk_window_process_updates() to handle the exposes immediately
 *
 * The second way is that you get the “invalidated” signal from the layout,
 * indicating that lines have become invalid. This code path begins in
 * invalidated_handler() and goes like this:
 *   - install high-priority idle which does the rest of the steps
 *   - if a scroll is pending from scroll_to_mark(), do the scroll,
 *     jumping to the ctk_text_view_value_changed() code path
 *   - otherwise, validate the onscreen lines
 *   - DO NOT process updates
 *
 * In both cases, validating the onscreen lines can trigger a scroll
 * due to maintaining the first_para on the top of the screen.
 * If validation triggers a scroll, we jump to the top of the code path
 * for value_changed, and bail out of the current code path.
 *
 * Also, in size_allocate, if we invalidate some lines from changing
 * the layout width, we need to go ahead and run the high-priority idle,
 * because CTK sends exposes right after doing the size allocates without
 * returning to the main loop. This is also why the high-priority idle
 * is at a higher priority than resizing.
 *
 */

#if 0
#define DEBUG_VALIDATION_AND_SCROLLING
#endif

#ifdef DEBUG_VALIDATION_AND_SCROLLING
#define DV(x) (x)
#else
#define DV(x)
#endif

#define SCREEN_WIDTH(widget) text_window_get_width (CTK_TEXT_VIEW (widget)->priv->text_window)
#define SCREEN_HEIGHT(widget) text_window_get_height (CTK_TEXT_VIEW (widget)->priv->text_window)

#define SPACE_FOR_CURSOR 1

#define CTK_TEXT_VIEW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CTK_TYPE_TEXT_VIEW, CtkTextViewPrivate))

typedef struct _CtkTextWindow CtkTextWindow;
typedef struct _CtkTextPendingScroll CtkTextPendingScroll;

struct _CtkTextViewPrivate 
{
  CtkTextLayout *layout;
  CtkTextBuffer *buffer;

  guint blink_time;  /* time in msec the cursor has blinked since last user event */
  guint im_spot_idle;
  gchar *im_module;

  gint dnd_x;
  gint dnd_y;

  CtkTextHandle *text_handle;
  CtkWidget *selection_bubble;
  guint selection_bubble_timeout_id;

  CtkWidget *magnifier_popover;
  CtkWidget *magnifier;

  CtkTextWindow *text_window;
  CtkTextWindow *left_window;
  CtkTextWindow *right_window;
  CtkTextWindow *top_window;
  CtkTextWindow *bottom_window;

  CtkAdjustment *hadjustment;
  CtkAdjustment *vadjustment;

  /* X offset between widget coordinates and buffer coordinates
   * taking left_padding in account
   */
  gint xoffset;

  /* Y offset between widget coordinates and buffer coordinates
   * taking top_padding and top_margin in account
   */
  gint yoffset;

  /* Width and height of the buffer */
  gint width;
  gint height;

  /* This is used to monitor the overall size request 
   * and decide whether we need to queue resizes when
   * the buffer content changes. 
   *
   * FIXME: This could be done in a simpler way by 
   * consulting the above width/height of the buffer + some
   * padding values, however all of this request code needs
   * to be changed to use CtkWidget     Iface and deserves
   * more attention.
   */
  CtkRequisition cached_size_request;

  /* The virtual cursor position is normally the same as the
   * actual (strong) cursor position, except in two circumstances:
   *
   * a) When the cursor is moved vertically with the keyboard
   * b) When the text view is scrolled with the keyboard
   *
   * In case a), virtual_cursor_x is preserved, but not virtual_cursor_y
   * In case b), both virtual_cursor_x and virtual_cursor_y are preserved.
   */
  gint virtual_cursor_x;   /* -1 means use actual cursor position */
  gint virtual_cursor_y;   /* -1 means use actual cursor position */

  CtkTextMark *first_para_mark; /* Mark at the beginning of the first onscreen paragraph */
  gint first_para_pixels;       /* Offset of top of screen in the first onscreen paragraph */

  guint blink_timeout;
  guint scroll_timeout;

  guint first_validate_idle;        /* Idle to revalidate onscreen portion, runs before resize */
  guint incremental_validate_idle;  /* Idle to revalidate offscreen portions, runs after redraw */

  CtkTextMark *dnd_mark;

  CtkIMContext *im_context;
  CtkWidget *popup_menu;

  GSList *children;

  CtkTextPendingScroll *pending_scroll;

  CtkPixelCache *pixel_cache;

  CtkGesture *multipress_gesture;
  CtkGesture *drag_gesture;

  CtkCssNode *selection_node;

  /* Default style settings */
  gint pixels_above_lines;
  gint pixels_below_lines;
  gint pixels_inside_wrap;
  CtkWrapMode wrap_mode;
  CtkJustification justify;

  gint left_margin;
  gint right_margin;
  gint top_margin;
  gint bottom_margin;
  gint left_padding;
  gint right_padding;
  gint top_padding;
  gint bottom_padding;
  gint top_border;
  gint bottom_border;
  gint left_border;
  gint right_border;

  gint indent;
  gint64 handle_place_time;
  PangoTabArray *tabs;
  guint editable : 1;

  guint overwrite_mode : 1;
  guint cursor_visible : 1;

  /* if we have reset the IM since the last character entered */
  guint need_im_reset : 1;

  guint accepts_tab : 1;

  guint width_changed : 1;

  /* debug flag - means that we've validated onscreen since the
   * last "invalidate" signal from the layout
   */
  guint onscreen_validated : 1;

  guint mouse_cursor_obscured : 1;

  guint scroll_after_paste : 1;

  /* CtkScrollablePolicy needs to be checked when
   * driving the scrollable adjustment values */
  guint hscroll_policy : 1;
  guint vscroll_policy : 1;
  guint cursor_handle_dragged : 1;
  guint selection_handle_dragged : 1;
  guint populate_all   : 1;

  guint in_scroll : 1;
  guint handling_key_event : 1;
};

struct _CtkTextPendingScroll
{
  CtkTextMark   *mark;
  gdouble        within_margin;
  gboolean       use_align;
  gdouble        xalign;
  gdouble        yalign;
};

typedef enum 
{
  SELECT_CHARACTERS,
  SELECT_WORDS,
  SELECT_LINES
} SelectionGranularity;

enum
{
  POPULATE_POPUP,
  MOVE_CURSOR,
  PAGE_HORIZONTALLY,
  SET_ANCHOR,
  INSERT_AT_CURSOR,
  DELETE_FROM_CURSOR,
  BACKSPACE,
  CUT_CLIPBOARD,
  COPY_CLIPBOARD,
  PASTE_CLIPBOARD,
  TOGGLE_OVERWRITE,
  MOVE_VIEWPORT,
  SELECT_ALL,
  TOGGLE_CURSOR_VISIBLE,
  PREEDIT_CHANGED,
  EXTEND_SELECTION,
  INSERT_EMOJI,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_PIXELS_ABOVE_LINES,
  PROP_PIXELS_BELOW_LINES,
  PROP_PIXELS_INSIDE_WRAP,
  PROP_EDITABLE,
  PROP_WRAP_MODE,
  PROP_JUSTIFICATION,
  PROP_LEFT_MARGIN,
  PROP_RIGHT_MARGIN,
  PROP_TOP_MARGIN,
  PROP_BOTTOM_MARGIN,
  PROP_INDENT,
  PROP_TABS,
  PROP_CURSOR_VISIBLE,
  PROP_BUFFER,
  PROP_OVERWRITE,
  PROP_ACCEPTS_TAB,
  PROP_IM_MODULE,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_HSCROLL_POLICY,
  PROP_VSCROLL_POLICY,
  PROP_INPUT_PURPOSE,
  PROP_INPUT_HINTS,
  PROP_POPULATE_ALL,
  PROP_MONOSPACE
};

static GQuark quark_text_selection_data = 0;
static GQuark quark_ctk_signal = 0;
static GQuark quark_text_view_child = 0;

static void ctk_text_view_finalize             (GObject          *object);
static void ctk_text_view_set_property         (GObject         *object,
						guint            prop_id,
						const GValue    *value,
						GParamSpec      *pspec);
static void ctk_text_view_get_property         (GObject         *object,
						guint            prop_id,
						GValue          *value,
						GParamSpec      *pspec);
static void ctk_text_view_destroy              (CtkWidget        *widget);
static void ctk_text_view_size_request         (CtkWidget        *widget,
                                                CtkRequisition   *requisition);
static void ctk_text_view_get_preferred_width  (CtkWidget        *widget,
						gint             *minimum,
						gint             *natural);
static void ctk_text_view_get_preferred_height (CtkWidget        *widget,
						gint             *minimum,
						gint             *natural);
static void ctk_text_view_size_allocate        (CtkWidget        *widget,
                                                CtkAllocation    *allocation);
static void ctk_text_view_map                  (CtkWidget        *widget);
static void ctk_text_view_unmap                (CtkWidget        *widget);
static void ctk_text_view_realize              (CtkWidget        *widget);
static void ctk_text_view_unrealize            (CtkWidget        *widget);
static void ctk_text_view_style_updated        (CtkWidget        *widget);
static void ctk_text_view_direction_changed    (CtkWidget        *widget,
                                                CtkTextDirection  previous_direction);
static void ctk_text_view_state_flags_changed  (CtkWidget        *widget,
					        CtkStateFlags     previous_state);

static void ctk_text_view_multipress_gesture_pressed (CtkGestureMultiPress *gesture,
                                                      gint                  n_press,
                                                      gdouble               x,
                                                      gdouble               y,
                                                      CtkTextView          *text_view);
static void ctk_text_view_drag_gesture_update        (CtkGestureDrag *gesture,
                                                      gdouble         offset_x,
                                                      gdouble         offset_y,
                                                      CtkTextView    *text_view);
static void ctk_text_view_drag_gesture_end           (CtkGestureDrag *gesture,
                                                      gdouble         offset_x,
                                                      gdouble         offset_y,
                                                      CtkTextView    *text_view);

static gint ctk_text_view_event                (CtkWidget        *widget,
                                                CdkEvent         *event);
static gint ctk_text_view_key_press_event      (CtkWidget        *widget,
                                                CdkEventKey      *event);
static gint ctk_text_view_key_release_event    (CtkWidget        *widget,
                                                CdkEventKey      *event);
static gint ctk_text_view_focus_in_event       (CtkWidget        *widget,
                                                CdkEventFocus    *event);
static gint ctk_text_view_focus_out_event      (CtkWidget        *widget,
                                                CdkEventFocus    *event);
static gint ctk_text_view_motion_event         (CtkWidget        *widget,
                                                CdkEventMotion   *event);
static gint ctk_text_view_draw                 (CtkWidget        *widget,
                                                cairo_t          *cr);
static gboolean ctk_text_view_focus            (CtkWidget        *widget,
                                                CtkDirectionType  direction);
static void ctk_text_view_select_all           (CtkWidget        *widget,
                                                gboolean          select);
static gboolean get_middle_click_paste         (CtkTextView      *text_view);

static CtkTextBuffer* ctk_text_view_create_buffer (CtkTextView   *text_view);

/* Source side drag signals */
static void ctk_text_view_drag_begin       (CtkWidget        *widget,
                                            CdkDragContext   *context);
static void ctk_text_view_drag_end         (CtkWidget        *widget,
                                            CdkDragContext   *context);
static void ctk_text_view_drag_data_get    (CtkWidget        *widget,
                                            CdkDragContext   *context,
                                            CtkSelectionData *selection_data,
                                            guint             info,
                                            guint             time);
static void ctk_text_view_drag_data_delete (CtkWidget        *widget,
                                            CdkDragContext   *context);

/* Target side drag signals */
static void     ctk_text_view_drag_leave         (CtkWidget        *widget,
                                                  CdkDragContext   *context,
                                                  guint             time);
static gboolean ctk_text_view_drag_motion        (CtkWidget        *widget,
                                                  CdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  guint             time);
static gboolean ctk_text_view_drag_drop          (CtkWidget        *widget,
                                                  CdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  guint             time);
static void     ctk_text_view_drag_data_received (CtkWidget        *widget,
                                                  CdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  CtkSelectionData *selection_data,
                                                  guint             info,
                                                  guint             time);

static gboolean ctk_text_view_popup_menu         (CtkWidget     *widget);

static void ctk_text_view_move_cursor       (CtkTextView           *text_view,
                                             CtkMovementStep        step,
                                             gint                   count,
                                             gboolean               extend_selection);
static void ctk_text_view_move_viewport     (CtkTextView           *text_view,
                                             CtkScrollStep          step,
                                             gint                   count);
static void ctk_text_view_set_anchor       (CtkTextView           *text_view);
static gboolean ctk_text_view_scroll_pages (CtkTextView           *text_view,
                                            gint                   count,
                                            gboolean               extend_selection);
static gboolean ctk_text_view_scroll_hpages(CtkTextView           *text_view,
                                            gint                   count,
                                            gboolean               extend_selection);
static void ctk_text_view_insert_at_cursor (CtkTextView           *text_view,
                                            const gchar           *str);
static void ctk_text_view_delete_from_cursor (CtkTextView           *text_view,
                                              CtkDeleteType          type,
                                              gint                   count);
static void ctk_text_view_backspace        (CtkTextView           *text_view);
static void ctk_text_view_cut_clipboard    (CtkTextView           *text_view);
static void ctk_text_view_copy_clipboard   (CtkTextView           *text_view);
static void ctk_text_view_paste_clipboard  (CtkTextView           *text_view);
static void ctk_text_view_toggle_overwrite (CtkTextView           *text_view);
static void ctk_text_view_toggle_cursor_visible (CtkTextView      *text_view);

static void ctk_text_view_unselect         (CtkTextView           *text_view);

static void     ctk_text_view_validate_onscreen     (CtkTextView        *text_view);
static void     ctk_text_view_get_first_para_iter   (CtkTextView        *text_view,
                                                     CtkTextIter        *iter);
static void     ctk_text_view_update_layout_width       (CtkTextView        *text_view);
static void     ctk_text_view_set_attributes_from_style (CtkTextView        *text_view,
                                                         CtkTextAttributes  *values);
static void     ctk_text_view_ensure_layout          (CtkTextView        *text_view);
static void     ctk_text_view_destroy_layout         (CtkTextView        *text_view);
static void     ctk_text_view_check_keymap_direction (CtkTextView        *text_view);
static void     ctk_text_view_start_selection_drag   (CtkTextView          *text_view,
                                                      const CtkTextIter    *iter,
                                                      SelectionGranularity  granularity,
                                                      gboolean              extends);
static gboolean ctk_text_view_end_selection_drag     (CtkTextView        *text_view);
static void     ctk_text_view_start_selection_dnd    (CtkTextView        *text_view,
                                                      const CtkTextIter  *iter,
                                                      const CdkEvent     *event,
                                                      gint                x,
                                                      gint                y);
static void     ctk_text_view_check_cursor_blink     (CtkTextView        *text_view);
static void     ctk_text_view_pend_cursor_blink      (CtkTextView        *text_view);
static void     ctk_text_view_stop_cursor_blink      (CtkTextView        *text_view);
static void     ctk_text_view_reset_blink_time       (CtkTextView        *text_view);

static void     ctk_text_view_value_changed                (CtkAdjustment *adjustment,
							    CtkTextView   *view);
static void     ctk_text_view_commit_handler               (CtkIMContext  *context,
							    const gchar   *str,
							    CtkTextView   *text_view);
static void     ctk_text_view_commit_text                  (CtkTextView   *text_view,
                                                            const gchar   *text);
static void     ctk_text_view_preedit_changed_handler      (CtkIMContext  *context,
							    CtkTextView   *text_view);
static gboolean ctk_text_view_retrieve_surrounding_handler (CtkIMContext  *context,
							    CtkTextView   *text_view);
static gboolean ctk_text_view_delete_surrounding_handler   (CtkIMContext  *context,
							    gint           offset,
							    gint           n_chars,
							    CtkTextView   *text_view);

static void ctk_text_view_mark_set_handler       (CtkTextBuffer     *buffer,
                                                  const CtkTextIter *location,
                                                  CtkTextMark       *mark,
                                                  gpointer           data);
static void ctk_text_view_target_list_notify     (CtkTextBuffer     *buffer,
                                                  const GParamSpec  *pspec,
                                                  gpointer           data);
static void ctk_text_view_paste_done_handler     (CtkTextBuffer     *buffer,
                                                  CtkClipboard      *clipboard,
                                                  gpointer           data);
static void ctk_text_view_buffer_changed_handler (CtkTextBuffer     *buffer,
                                                  gpointer           data);
static void ctk_text_view_get_virtual_cursor_pos (CtkTextView       *text_view,
                                                  CtkTextIter       *cursor,
                                                  gint              *x,
                                                  gint              *y);
static void ctk_text_view_set_virtual_cursor_pos (CtkTextView       *text_view,
                                                  gint               x,
                                                  gint               y);

static void ctk_text_view_do_popup               (CtkTextView       *text_view,
						  const CdkEvent    *event);

static void cancel_pending_scroll                (CtkTextView   *text_view);
static void ctk_text_view_queue_scroll           (CtkTextView   *text_view,
                                                  CtkTextMark   *mark,
                                                  gdouble        within_margin,
                                                  gboolean       use_align,
                                                  gdouble        xalign,
                                                  gdouble        yalign);

static gboolean ctk_text_view_flush_scroll         (CtkTextView *text_view);
static void     ctk_text_view_update_adjustments   (CtkTextView *text_view);
static void     ctk_text_view_invalidate           (CtkTextView *text_view);
static void     ctk_text_view_flush_first_validate (CtkTextView *text_view);

static void     ctk_text_view_set_hadjustment        (CtkTextView   *text_view,
                                                      CtkAdjustment *adjustment);
static void     ctk_text_view_set_vadjustment        (CtkTextView   *text_view,
                                                      CtkAdjustment *adjustment);
static void     ctk_text_view_set_hadjustment_values (CtkTextView   *text_view);
static void     ctk_text_view_set_vadjustment_values (CtkTextView   *text_view);

static void ctk_text_view_update_im_spot_location (CtkTextView *text_view);
static void ctk_text_view_insert_emoji (CtkTextView *text_view);

/* Container methods */
static void ctk_text_view_add    (CtkContainer *container,
                                  CtkWidget    *child);
static void ctk_text_view_remove (CtkContainer *container,
                                  CtkWidget    *child);
static void ctk_text_view_forall (CtkContainer *container,
                                  gboolean      include_internals,
                                  CtkCallback   callback,
                                  gpointer      callback_data);

/* CtkTextHandle handlers */
static void ctk_text_view_handle_drag_started  (CtkTextHandle         *handle,
                                                CtkTextHandlePosition  pos,
                                                CtkTextView           *text_view);
static void ctk_text_view_handle_dragged       (CtkTextHandle         *handle,
                                                CtkTextHandlePosition  pos,
                                                gint                   x,
                                                gint                   y,
                                                CtkTextView           *text_view);
static void ctk_text_view_handle_drag_finished (CtkTextHandle         *handle,
                                                CtkTextHandlePosition  pos,
                                                CtkTextView           *text_view);
static void ctk_text_view_update_handles       (CtkTextView           *text_view,
                                                CtkTextHandleMode      mode);

static void ctk_text_view_selection_bubble_popup_unset (CtkTextView *text_view);
static void ctk_text_view_selection_bubble_popup_set   (CtkTextView *text_view);

static void ctk_text_view_queue_draw_region (CtkWidget            *widget,
                                             const cairo_region_t *region);

static void ctk_text_view_get_rendered_rect (CtkTextView  *text_view,
                                             CdkRectangle *rect);

static gboolean ctk_text_view_extend_selection (CtkTextView            *text_view,
                                                CtkTextExtendSelection  granularity,
                                                const CtkTextIter      *location,
                                                CtkTextIter            *start,
                                                CtkTextIter            *end);
static void extend_selection (CtkTextView          *text_view,
                              SelectionGranularity  granularity,
                              const CtkTextIter    *location,
                              CtkTextIter          *start,
                              CtkTextIter          *end);



/* FIXME probably need the focus methods. */

typedef struct _CtkTextViewChild CtkTextViewChild;

struct _CtkTextViewChild
{
  CtkWidget *widget;

  CtkTextChildAnchor *anchor;

  gint from_top_of_line;
  gint from_left_of_buffer;
  
  /* These are ignored if anchor != NULL */
  CtkTextWindowType type;
  gint x;
  gint y;
};

static CtkTextViewChild* text_view_child_new_anchored      (CtkWidget          *child,
							    CtkTextChildAnchor *anchor,
							    CtkTextLayout      *layout);
static CtkTextViewChild* text_view_child_new_window        (CtkWidget          *child,
							    CtkTextWindowType   type,
							    gint                x,
							    gint                y);
static void              text_view_child_free              (CtkTextViewChild   *child);
static void              text_view_child_set_parent_window (CtkTextView        *text_view,
							    CtkTextViewChild   *child);

struct _CtkTextWindow
{
  CtkTextWindowType type;
  CtkWidget *widget;
  CdkWindow *window;
  CdkWindow *bin_window;
  CtkCssNode *css_node;
  CtkRequisition requisition;
  CdkRectangle allocation;
};

static CtkTextWindow *text_window_new             (CtkTextWindowType  type,
                                                   CtkWidget         *widget,
                                                   gint               width_request,
                                                   gint               height_request);
static void           text_window_free            (CtkTextWindow     *win);
static void           text_window_realize         (CtkTextWindow     *win,
                                                   CtkWidget         *widget);
static void           text_window_unrealize       (CtkTextWindow     *win);
static void           text_window_size_allocate   (CtkTextWindow     *win,
                                                   CdkRectangle      *rect);
static void           text_window_scroll          (CtkTextWindow     *win,
                                                   gint               dx,
                                                   gint               dy);
static void           text_window_invalidate_rect (CtkTextWindow     *win,
                                                   CdkRectangle      *rect);
static void           text_window_invalidate_cursors (CtkTextWindow  *win);

static gint           text_window_get_width       (CtkTextWindow     *win);
static gint           text_window_get_height      (CtkTextWindow     *win);


static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (CtkTextView, ctk_text_view, CTK_TYPE_CONTAINER,
                         G_ADD_PRIVATE (CtkTextView)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_SCROLLABLE, NULL))

static void
add_move_binding (CtkBindingSet  *binding_set,
                  guint           keyval,
                  guint           modmask,
                  CtkMovementStep step,
                  gint            count)
{
  g_assert ((modmask & CDK_SHIFT_MASK) == 0);

  ctk_binding_entry_add_signal (binding_set, keyval, modmask,
                                "move-cursor", 3,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count,
                                G_TYPE_BOOLEAN, FALSE);

  /* Selection-extending version */
  ctk_binding_entry_add_signal (binding_set, keyval, modmask | CDK_SHIFT_MASK,
                                "move-cursor", 3,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count,
                                G_TYPE_BOOLEAN, TRUE);
}

static void
ctk_text_view_class_init (CtkTextViewClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);
  CtkBindingSet *binding_set;

  /* Default handlers and virtual methods
   */
  gobject_class->set_property = ctk_text_view_set_property;
  gobject_class->get_property = ctk_text_view_get_property;
  gobject_class->finalize = ctk_text_view_finalize;

  widget_class->destroy = ctk_text_view_destroy;
  widget_class->map = ctk_text_view_map;
  widget_class->unmap = ctk_text_view_unmap;
  widget_class->realize = ctk_text_view_realize;
  widget_class->unrealize = ctk_text_view_unrealize;
  widget_class->style_updated = ctk_text_view_style_updated;
  widget_class->direction_changed = ctk_text_view_direction_changed;
  widget_class->state_flags_changed = ctk_text_view_state_flags_changed;
  widget_class->get_preferred_width = ctk_text_view_get_preferred_width;
  widget_class->get_preferred_height = ctk_text_view_get_preferred_height;
  widget_class->size_allocate = ctk_text_view_size_allocate;
  widget_class->event = ctk_text_view_event;
  widget_class->key_press_event = ctk_text_view_key_press_event;
  widget_class->key_release_event = ctk_text_view_key_release_event;
  widget_class->focus_in_event = ctk_text_view_focus_in_event;
  widget_class->focus_out_event = ctk_text_view_focus_out_event;
  widget_class->motion_notify_event = ctk_text_view_motion_event;
  widget_class->draw = ctk_text_view_draw;
  widget_class->focus = ctk_text_view_focus;
  widget_class->drag_begin = ctk_text_view_drag_begin;
  widget_class->drag_end = ctk_text_view_drag_end;
  widget_class->drag_data_get = ctk_text_view_drag_data_get;
  widget_class->drag_data_delete = ctk_text_view_drag_data_delete;

  widget_class->drag_leave = ctk_text_view_drag_leave;
  widget_class->drag_motion = ctk_text_view_drag_motion;
  widget_class->drag_drop = ctk_text_view_drag_drop;
  widget_class->drag_data_received = ctk_text_view_drag_data_received;

  widget_class->popup_menu = ctk_text_view_popup_menu;

  widget_class->queue_draw_region = ctk_text_view_queue_draw_region;

  container_class->add = ctk_text_view_add;
  container_class->remove = ctk_text_view_remove;
  container_class->forall = ctk_text_view_forall;

  klass->move_cursor = ctk_text_view_move_cursor;
  klass->set_anchor = ctk_text_view_set_anchor;
  klass->insert_at_cursor = ctk_text_view_insert_at_cursor;
  klass->delete_from_cursor = ctk_text_view_delete_from_cursor;
  klass->backspace = ctk_text_view_backspace;
  klass->cut_clipboard = ctk_text_view_cut_clipboard;
  klass->copy_clipboard = ctk_text_view_copy_clipboard;
  klass->paste_clipboard = ctk_text_view_paste_clipboard;
  klass->toggle_overwrite = ctk_text_view_toggle_overwrite;
  klass->create_buffer = ctk_text_view_create_buffer;
  klass->extend_selection = ctk_text_view_extend_selection;
  klass->insert_emoji = ctk_text_view_insert_emoji;

  /*
   * Properties
   */
 
  g_object_class_install_property (gobject_class,
                                   PROP_PIXELS_ABOVE_LINES,
                                   g_param_spec_int ("pixels-above-lines",
                                                     P_("Pixels Above Lines"),
                                                     P_("Pixels of blank space above paragraphs"),
                                                     0, G_MAXINT, 0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
 
  g_object_class_install_property (gobject_class,
                                   PROP_PIXELS_BELOW_LINES,
                                   g_param_spec_int ("pixels-below-lines",
                                                     P_("Pixels Below Lines"),
                                                     P_("Pixels of blank space below paragraphs"),
                                                     0, G_MAXINT, 0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
 
  g_object_class_install_property (gobject_class,
                                   PROP_PIXELS_INSIDE_WRAP,
                                   g_param_spec_int ("pixels-inside-wrap",
                                                     P_("Pixels Inside Wrap"),
                                                     P_("Pixels of blank space between wrapped lines in a paragraph"),
                                                     0, G_MAXINT, 0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
                                   PROP_EDITABLE,
                                   g_param_spec_boolean ("editable",
                                                         P_("Editable"),
                                                         P_("Whether the text can be modified by the user"),
                                                         TRUE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
                                   PROP_WRAP_MODE,
                                   g_param_spec_enum ("wrap-mode",
                                                      P_("Wrap Mode"),
                                                      P_("Whether to wrap lines never, at word boundaries, or at character boundaries"),
                                                      CTK_TYPE_WRAP_MODE,
                                                      CTK_WRAP_NONE,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
 
  g_object_class_install_property (gobject_class,
                                   PROP_JUSTIFICATION,
                                   g_param_spec_enum ("justification",
                                                      P_("Justification"),
                                                      P_("Left, right, or center justification"),
                                                      CTK_TYPE_JUSTIFICATION,
                                                      CTK_JUSTIFY_LEFT,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkTextView:left-margin:
   *
   * The default left margin for text in the text view.
   * Tags in the buffer may override the default.
   *
   * Note that this property is confusingly named. In CSS terms,
   * the value set here is padding, and it is applied in addition
   * to the padding from the theme.
   *
   * Don't confuse this property with #CtkWidget:margin-left.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_LEFT_MARGIN,
                                   g_param_spec_int ("left-margin",
                                                     P_("Left Margin"),
                                                     P_("Width of the left margin in pixels"),
                                                     0, G_MAXINT, 0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkTextView:right-margin:
   *
   * The default right margin for text in the text view.
   * Tags in the buffer may override the default.
   *
   * Note that this property is confusingly named. In CSS terms,
   * the value set here is padding, and it is applied in addition
   * to the padding from the theme.
   *
   * Don't confuse this property with #CtkWidget:margin-right.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_RIGHT_MARGIN,
                                   g_param_spec_int ("right-margin",
                                                     P_("Right Margin"),
                                                     P_("Width of the right margin in pixels"),
                                                     0, G_MAXINT, 0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkTextView:top-margin:
   *
   * The top margin for text in the text view.
   *
   * Note that this property is confusingly named. In CSS terms,
   * the value set here is padding, and it is applied in addition
   * to the padding from the theme.
   *
   * Don't confuse this property with #CtkWidget:margin-top.
   *
   * Since: 3.18
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TOP_MARGIN,
                                   g_param_spec_int ("top-margin",
                                                     P_("Top Margin"),
                                                     P_("Height of the top margin in pixels"),
                                                     0, G_MAXINT, 0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkTextView:bottom-margin:
   *
   * The bottom margin for text in the text view.
   *
   * Note that this property is confusingly named. In CSS terms,
   * the value set here is padding, and it is applied in addition
   * to the padding from the theme.
   *
   * Don't confuse this property with #CtkWidget:margin-bottom.
   *
   * Since: 3.18
   */
  g_object_class_install_property (gobject_class,
                                   PROP_BOTTOM_MARGIN,
                                   g_param_spec_int ("bottom-margin",
                                                     P_("Bottom Margin"),
                                                     P_("Height of the bottom margin in pixels"),
                                                     0, G_MAXINT, 0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
                                   PROP_INDENT,
                                   g_param_spec_int ("indent",
                                                     P_("Indent"),
                                                     P_("Amount to indent the paragraph, in pixels"),
                                                     G_MININT, G_MAXINT, 0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
                                   PROP_TABS,
                                   g_param_spec_boxed ("tabs",
                                                       P_("Tabs"),
                                                       P_("Custom tabs for this text"),
                                                       PANGO_TYPE_TAB_ARRAY,
						       CTK_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_CURSOR_VISIBLE,
                                   g_param_spec_boolean ("cursor-visible",
                                                         P_("Cursor Visible"),
                                                         P_("If the insertion cursor is shown"),
                                                         TRUE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
                                   PROP_BUFFER,
                                   g_param_spec_object ("buffer",
							P_("Buffer"),
							P_("The buffer which is displayed"),
							CTK_TYPE_TEXT_BUFFER,
							CTK_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_OVERWRITE,
                                   g_param_spec_boolean ("overwrite",
                                                         P_("Overwrite mode"),
                                                         P_("Whether entered text overwrites existing contents"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
                                   PROP_ACCEPTS_TAB,
                                   g_param_spec_boolean ("accepts-tab",
                                                         P_("Accepts tab"),
                                                         P_("Whether Tab will result in a tab character being entered"),
                                                         TRUE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

   /**
    * CtkTextView:im-module:
    *
    * Which IM (input method) module should be used for this text_view. 
    * See #CtkIMContext.
    *
    * Setting this to a non-%NULL value overrides the
    * system-wide IM module setting. See the CtkSettings 
    * #CtkSettings:ctk-im-module property.
    *
    * Since: 2.16
    */
   g_object_class_install_property (gobject_class,
                                    PROP_IM_MODULE,
                                    g_param_spec_string ("im-module",
                                                         P_("IM module"),
                                                         P_("Which IM module should be used"),
                                                         NULL,
                                                         CTK_PARAM_READWRITE));

  /**
   * CtkTextView:input-purpose:
   *
   * The purpose of this text field.
   *
   * This property can be used by on-screen keyboards and other input
   * methods to adjust their behaviour.
   *
   * Since: 3.6
   */
  g_object_class_install_property (gobject_class,
                                   PROP_INPUT_PURPOSE,
                                   g_param_spec_enum ("input-purpose",
                                                      P_("Purpose"),
                                                      P_("Purpose of the text field"),
                                                      CTK_TYPE_INPUT_PURPOSE,
                                                      CTK_INPUT_PURPOSE_FREE_FORM,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));


  /**
   * CtkTextView:input-hints:
   *
   * Additional hints (beyond #CtkTextView:input-purpose) that
   * allow input methods to fine-tune their behaviour.
   *
   * Since: 3.6
   */
  g_object_class_install_property (gobject_class,
                                   PROP_INPUT_HINTS,
                                   g_param_spec_flags ("input-hints",
                                                       P_("hints"),
                                                       P_("Hints for the text field behaviour"),
                                                       CTK_TYPE_INPUT_HINTS,
                                                       CTK_INPUT_HINT_NONE,
                                                       CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkTextView:populate-all:
   *
   * If :populate-all is %TRUE, the #CtkTextView::populate-popup
   * signal is also emitted for touch popups.
   *
   * Since: 3.8
   */
  g_object_class_install_property (gobject_class,
                                   PROP_POPULATE_ALL,
                                   g_param_spec_boolean ("populate-all",
                                                         P_("Populate all"),
                                                         P_("Whether to emit ::populate-popup for touch popups"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkTextview:monospace:
   *
   * If %TRUE, set the %CTK_STYLE_CLASS_MONOSPACE style class on the
   * text view to indicate that a monospace font is desired.
   *
   * Since: 3.16
   */
  g_object_class_install_property (gobject_class,
                                   PROP_MONOSPACE,
                                   g_param_spec_boolean ("monospace",
                                                         P_("Monospace"),
                                                         P_("Whether to use a monospace font"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  

   /* CtkScrollable interface */
   g_object_class_override_property (gobject_class, PROP_HADJUSTMENT,    "hadjustment");
   g_object_class_override_property (gobject_class, PROP_VADJUSTMENT,    "vadjustment");
   g_object_class_override_property (gobject_class, PROP_HSCROLL_POLICY, "hscroll-policy");
   g_object_class_override_property (gobject_class, PROP_VSCROLL_POLICY, "vscroll-policy");

  /*
   * Style properties
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("error-underline-color",
							       P_("Error underline color"),
							       P_("Color with which to draw error-indication underlines"),
							       CDK_TYPE_COLOR,
							       CTK_PARAM_READABLE));
  
  /*
   * Signals
   */

  /**
   * CtkTextView::move-cursor: 
   * @text_view: the object which received the signal
   * @step: the granularity of the move, as a #CtkMovementStep
   * @count: the number of @step units to move
   * @extend_selection: %TRUE if the move should extend the selection
   *  
   * The ::move-cursor signal is a 
   * [keybinding signal][CtkBindingSignal] 
   * which gets emitted when the user initiates a cursor movement. 
   * If the cursor is not visible in @text_view, this signal causes
   * the viewport to be moved instead.
   *
   * Applications should not connect to it, but may emit it with 
   * g_signal_emit_by_name() if they need to control the cursor
   * programmatically.
   *
   * The default bindings for this signal come in two variants,
   * the variant with the Shift modifier extends the selection,
   * the variant without the Shift modifer does not.
   * There are too many key combinations to list them all here.
   * - Arrow keys move by individual characters/lines
   * - Ctrl-arrow key combinations move by words/paragraphs
   * - Home/End keys move to the ends of the buffer
   * - PageUp/PageDown keys move vertically by pages
   * - Ctrl-PageUp/PageDown keys move horizontally by pages
   */
  signals[MOVE_CURSOR] = 
    g_signal_new (I_("move-cursor"),
		  G_OBJECT_CLASS_TYPE (gobject_class), 
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 
		  G_STRUCT_OFFSET (CtkTextViewClass, move_cursor),
		  NULL, NULL, 
		  _ctk_marshal_VOID__ENUM_INT_BOOLEAN, 
		  G_TYPE_NONE, 3,
		  CTK_TYPE_MOVEMENT_STEP, 
		  G_TYPE_INT, 
		  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (signals[MOVE_CURSOR],
                              G_OBJECT_CLASS_TYPE (gobject_class),
                              _ctk_marshal_VOID__ENUM_INT_BOOLEANv);

  /**
   * CtkTextView::move-viewport:
   * @text_view: the object which received the signal
   * @step: the granularity of the movement, as a #CtkScrollStep
   * @count: the number of @step units to move
   *
   * The ::move-viewport signal is a
   * [keybinding signal][CtkBindingSignal]
   * which can be bound to key combinations to allow the user
   * to move the viewport, i.e. change what part of the text view
   * is visible in a containing scrolled window.
   *
   * There are no default bindings for this signal.
   */
  signals[MOVE_VIEWPORT] =
    g_signal_new_class_handler (I_("move-viewport"),
                                G_OBJECT_CLASS_TYPE (gobject_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (ctk_text_view_move_viewport),
                                NULL, NULL,
                                _ctk_marshal_VOID__ENUM_INT,
                                G_TYPE_NONE, 2,
                                CTK_TYPE_SCROLL_STEP,
                                G_TYPE_INT);
  g_signal_set_va_marshaller (signals[MOVE_VIEWPORT],
                              G_OBJECT_CLASS_TYPE (gobject_class),
                              _ctk_marshal_VOID__ENUM_INTv);

  /**
   * CtkTextView::set-anchor:
   * @text_view: the object which received the signal
   *
   * The ::set-anchor signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user initiates setting the "anchor" 
   * mark. The "anchor" mark gets placed at the same position as the
   * "insert" mark.
   *
   * This signal has no default bindings.
   */   
  signals[SET_ANCHOR] =
    g_signal_new (I_("set-anchor"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTextViewClass, set_anchor),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkTextView::insert-at-cursor:
   * @text_view: the object which received the signal
   * @string: the string to insert
   *
   * The ::insert-at-cursor signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user initiates the insertion of a 
   * fixed string at the cursor.
   *
   * This signal has no default bindings.
   */
  signals[INSERT_AT_CURSOR] =
    g_signal_new (I_("insert-at-cursor"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTextViewClass, insert_at_cursor),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  G_TYPE_STRING);

  /**
   * CtkTextView::delete-from-cursor:
   * @text_view: the object which received the signal
   * @type: the granularity of the deletion, as a #CtkDeleteType
   * @count: the number of @type units to delete
   *
   * The ::delete-from-cursor signal is a 
   * [keybinding signal][CtkBindingSignal] 
   * which gets emitted when the user initiates a text deletion.
   *
   * If the @type is %CTK_DELETE_CHARS, CTK+ deletes the selection
   * if there is one, otherwise it deletes the requested number
   * of characters.
   *
   * The default bindings for this signal are
   * Delete for deleting a character, Ctrl-Delete for 
   * deleting a word and Ctrl-Backspace for deleting a word 
   * backwords.
   */
  signals[DELETE_FROM_CURSOR] =
    g_signal_new (I_("delete-from-cursor"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTextViewClass, delete_from_cursor),
		  NULL, NULL,
		  _ctk_marshal_VOID__ENUM_INT,
		  G_TYPE_NONE, 2,
		  CTK_TYPE_DELETE_TYPE,
		  G_TYPE_INT);
  g_signal_set_va_marshaller (signals[DELETE_FROM_CURSOR],
                              G_OBJECT_CLASS_TYPE (gobject_class),
                              _ctk_marshal_VOID__ENUM_INTv);

  /**
   * CtkTextView::backspace:
   * @text_view: the object which received the signal
   *
   * The ::backspace signal is a 
   * [keybinding signal][CtkBindingSignal] 
   * which gets emitted when the user asks for it.
   * 
   * The default bindings for this signal are
   * Backspace and Shift-Backspace.
   */
  signals[BACKSPACE] =
    g_signal_new (I_("backspace"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTextViewClass, backspace),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkTextView::cut-clipboard:
   * @text_view: the object which received the signal
   *
   * The ::cut-clipboard signal is a 
   * [keybinding signal][CtkBindingSignal] 
   * which gets emitted to cut the selection to the clipboard.
   * 
   * The default bindings for this signal are
   * Ctrl-x and Shift-Delete.
   */
  signals[CUT_CLIPBOARD] =
    g_signal_new (I_("cut-clipboard"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTextViewClass, cut_clipboard),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkTextView::copy-clipboard:
   * @text_view: the object which received the signal
   *
   * The ::copy-clipboard signal is a 
   * [keybinding signal][CtkBindingSignal] 
   * which gets emitted to copy the selection to the clipboard.
   * 
   * The default bindings for this signal are
   * Ctrl-c and Ctrl-Insert.
   */
  signals[COPY_CLIPBOARD] =
    g_signal_new (I_("copy-clipboard"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTextViewClass, copy_clipboard),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkTextView::paste-clipboard:
   * @text_view: the object which received the signal
   *
   * The ::paste-clipboard signal is a 
   * [keybinding signal][CtkBindingSignal] 
   * which gets emitted to paste the contents of the clipboard 
   * into the text view.
   * 
   * The default bindings for this signal are
   * Ctrl-v and Shift-Insert.
   */
  signals[PASTE_CLIPBOARD] =
    g_signal_new (I_("paste-clipboard"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTextViewClass, paste_clipboard),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkTextView::toggle-overwrite:
   * @text_view: the object which received the signal
   *
   * The ::toggle-overwrite signal is a 
   * [keybinding signal][CtkBindingSignal] 
   * which gets emitted to toggle the overwrite mode of the text view.
   * 
   * The default bindings for this signal is Insert.
   */ 
  signals[TOGGLE_OVERWRITE] =
    g_signal_new (I_("toggle-overwrite"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkTextViewClass, toggle_overwrite),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkTextView::populate-popup:
   * @text_view: The text view on which the signal is emitted
   * @popup: the container that is being populated
   *
   * The ::populate-popup signal gets emitted before showing the
   * context menu of the text view.
   *
   * If you need to add items to the context menu, connect
   * to this signal and append your items to the @popup, which
   * will be a #CtkMenu in this case.
   *
   * If #CtkTextView:populate-all is %TRUE, this signal will
   * also be emitted to populate touch popups. In this case,
   * @popup will be a different container, e.g. a #CtkToolbar.
   *
   * The signal handler should not make assumptions about the
   * type of @widget, but check whether @popup is a #CtkMenu
   * or #CtkToolbar or another kind of container.
   */
  signals[POPULATE_POPUP] =
    g_signal_new (I_("populate-popup"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkTextViewClass, populate_popup),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_WIDGET);
  
  /**
   * CtkTextView::select-all:
   * @text_view: the object which received the signal
   * @select: %TRUE to select, %FALSE to unselect
   *
   * The ::select-all signal is a 
   * [keybinding signal][CtkBindingSignal] 
   * which gets emitted to select or unselect the complete
   * contents of the text view.
   *
   * The default bindings for this signal are Ctrl-a and Ctrl-/ 
   * for selecting and Shift-Ctrl-a and Ctrl-\ for unselecting.
   */
  signals[SELECT_ALL] =
    g_signal_new_class_handler (I_("select-all"),
                                G_OBJECT_CLASS_TYPE (gobject_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (ctk_text_view_select_all),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

  /**
   * CtkTextView::toggle-cursor-visible:
   * @text_view: the object which received the signal
   *
   * The ::toggle-cursor-visible signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to toggle the #CtkTextView:cursor-visible
   * property.
   *
   * The default binding for this signal is F7.
   */ 
  signals[TOGGLE_CURSOR_VISIBLE] =
    g_signal_new_class_handler (I_("toggle-cursor-visible"),
                                G_OBJECT_CLASS_TYPE (gobject_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (ctk_text_view_toggle_cursor_visible),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 0);

  /**
   * CtkTextView::preedit-changed:
   * @text_view: the object which received the signal
   * @preedit: the current preedit string
   *
   * If an input method is used, the typed text will not immediately
   * be committed to the buffer. So if you are interested in the text,
   * connect to this signal.
   *
   * This signal is only emitted if the text at the given position
   * is actually editable.
   *
   * Since: 2.20
   */
  signals[PREEDIT_CHANGED] =
    g_signal_new_class_handler (I_("preedit-changed"),
                                G_OBJECT_CLASS_TYPE (gobject_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                NULL,
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 1,
                                G_TYPE_STRING);

  /**
   * CtkTextView::extend-selection:
   * @text_view: the object which received the signal
   * @granularity: the granularity type
   * @location: the location where to extend the selection
   * @start: where the selection should start
   * @end: where the selection should end
   *
   * The ::extend-selection signal is emitted when the selection needs to be
   * extended at @location.
   *
   * Returns: %CDK_EVENT_STOP to stop other handlers from being invoked for the
   *   event. %CDK_EVENT_PROPAGATE to propagate the event further.
   * Since: 3.16
   */
  signals[EXTEND_SELECTION] =
    g_signal_new (I_("extend-selection"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkTextViewClass, extend_selection),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__ENUM_BOXED_BOXED_BOXED,
                  G_TYPE_BOOLEAN, 4,
                  CTK_TYPE_TEXT_EXTEND_SELECTION,
                  CTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                  CTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                  CTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (signals[EXTEND_SELECTION],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__ENUM_BOXED_BOXED_BOXEDv);

  /**
   * CtkTextView::insert-emoji:
   * @text_view: the object which received the signal
   *
   * The ::insert-emoji signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to present the Emoji chooser for the @text_view.
   *
   * The default bindings for this signal are Ctrl-. and Ctrl-;
   *
   * Since: 3.22.27
   */
  signals[INSERT_EMOJI] =
    g_signal_new (I_("insert-emoji"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkTextViewClass, insert_emoji),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /*
   * Key bindings
   */

  binding_set = ctk_binding_set_by_class (klass);
  
  /* Moving the insertion point */
  add_move_binding (binding_set, CDK_KEY_Right, 0,
                    CTK_MOVEMENT_VISUAL_POSITIONS, 1);

  add_move_binding (binding_set, CDK_KEY_KP_Right, 0,
                    CTK_MOVEMENT_VISUAL_POSITIONS, 1);
  
  add_move_binding (binding_set, CDK_KEY_Left, 0,
                    CTK_MOVEMENT_VISUAL_POSITIONS, -1);

  add_move_binding (binding_set, CDK_KEY_KP_Left, 0,
                    CTK_MOVEMENT_VISUAL_POSITIONS, -1);
  
  add_move_binding (binding_set, CDK_KEY_Right, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_WORDS, 1);

  add_move_binding (binding_set, CDK_KEY_KP_Right, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_WORDS, 1);
  
  add_move_binding (binding_set, CDK_KEY_Left, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_WORDS, -1);

  add_move_binding (binding_set, CDK_KEY_KP_Left, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_WORDS, -1);
  
  add_move_binding (binding_set, CDK_KEY_Up, 0,
                    CTK_MOVEMENT_DISPLAY_LINES, -1);

  add_move_binding (binding_set, CDK_KEY_KP_Up, 0,
                    CTK_MOVEMENT_DISPLAY_LINES, -1);
  
  add_move_binding (binding_set, CDK_KEY_Down, 0,
                    CTK_MOVEMENT_DISPLAY_LINES, 1);

  add_move_binding (binding_set, CDK_KEY_KP_Down, 0,
                    CTK_MOVEMENT_DISPLAY_LINES, 1);
  
  add_move_binding (binding_set, CDK_KEY_Up, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_PARAGRAPHS, -1);

  add_move_binding (binding_set, CDK_KEY_KP_Up, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_PARAGRAPHS, -1);
  
  add_move_binding (binding_set, CDK_KEY_Down, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_PARAGRAPHS, 1);

  add_move_binding (binding_set, CDK_KEY_KP_Down, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_PARAGRAPHS, 1);
  
  add_move_binding (binding_set, CDK_KEY_Home, 0,
                    CTK_MOVEMENT_DISPLAY_LINE_ENDS, -1);

  add_move_binding (binding_set, CDK_KEY_KP_Home, 0,
                    CTK_MOVEMENT_DISPLAY_LINE_ENDS, -1);
  
  add_move_binding (binding_set, CDK_KEY_End, 0,
                    CTK_MOVEMENT_DISPLAY_LINE_ENDS, 1);

  add_move_binding (binding_set, CDK_KEY_KP_End, 0,
                    CTK_MOVEMENT_DISPLAY_LINE_ENDS, 1);
  
  add_move_binding (binding_set, CDK_KEY_Home, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_BUFFER_ENDS, -1);

  add_move_binding (binding_set, CDK_KEY_KP_Home, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_BUFFER_ENDS, -1);
  
  add_move_binding (binding_set, CDK_KEY_End, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_BUFFER_ENDS, 1);

  add_move_binding (binding_set, CDK_KEY_KP_End, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_BUFFER_ENDS, 1);
  
  add_move_binding (binding_set, CDK_KEY_Page_Up, 0,
                    CTK_MOVEMENT_PAGES, -1);

  add_move_binding (binding_set, CDK_KEY_KP_Page_Up, 0,
                    CTK_MOVEMENT_PAGES, -1);
  
  add_move_binding (binding_set, CDK_KEY_Page_Down, 0,
                    CTK_MOVEMENT_PAGES, 1);

  add_move_binding (binding_set, CDK_KEY_KP_Page_Down, 0,
                    CTK_MOVEMENT_PAGES, 1);

  add_move_binding (binding_set, CDK_KEY_Page_Up, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_HORIZONTAL_PAGES, -1);

  add_move_binding (binding_set, CDK_KEY_KP_Page_Up, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_HORIZONTAL_PAGES, -1);
  
  add_move_binding (binding_set, CDK_KEY_Page_Down, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_HORIZONTAL_PAGES, 1);

  add_move_binding (binding_set, CDK_KEY_KP_Page_Down, CDK_CONTROL_MASK,
                    CTK_MOVEMENT_HORIZONTAL_PAGES, 1);

  /* Select all */
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_a, CDK_CONTROL_MASK,
				"select-all", 1,
  				G_TYPE_BOOLEAN, TRUE);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_slash, CDK_CONTROL_MASK,
				"select-all", 1,
  				G_TYPE_BOOLEAN, TRUE);
  
  /* Unselect all */
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_backslash, CDK_CONTROL_MASK,
				 "select-all", 1,
				 G_TYPE_BOOLEAN, FALSE);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_a, CDK_SHIFT_MASK | CDK_CONTROL_MASK,
				 "select-all", 1,
				 G_TYPE_BOOLEAN, FALSE);

  /* Deleting text */
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Delete, 0,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, CTK_DELETE_CHARS,
				G_TYPE_INT, 1);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Delete, 0,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, CTK_DELETE_CHARS,
				G_TYPE_INT, 1);
  
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_BackSpace, 0,
				"backspace", 0);

  /* Make this do the same as Backspace, to help with mis-typing */
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_BackSpace, CDK_SHIFT_MASK,
				"backspace", 0);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Delete, CDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, CTK_DELETE_WORD_ENDS,
				G_TYPE_INT, 1);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Delete, CDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, CTK_DELETE_WORD_ENDS,
				G_TYPE_INT, 1);
  
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_BackSpace, CDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, CTK_DELETE_WORD_ENDS,
				G_TYPE_INT, -1);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Delete, CDK_SHIFT_MASK | CDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, CTK_DELETE_PARAGRAPH_ENDS,
				G_TYPE_INT, 1);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Delete, CDK_SHIFT_MASK | CDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, CTK_DELETE_PARAGRAPH_ENDS,
				G_TYPE_INT, 1);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_BackSpace, CDK_SHIFT_MASK | CDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, CTK_DELETE_PARAGRAPH_ENDS,
				G_TYPE_INT, -1);

  /* Cut/copy/paste */

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_x, CDK_CONTROL_MASK,
				"cut-clipboard", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_c, CDK_CONTROL_MASK,
				"copy-clipboard", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_v, CDK_CONTROL_MASK,
				"paste-clipboard", 0);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Delete, CDK_SHIFT_MASK,
				"cut-clipboard", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Insert, CDK_CONTROL_MASK,
				"copy-clipboard", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Insert, CDK_SHIFT_MASK,
				"paste-clipboard", 0);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Delete, CDK_SHIFT_MASK,
                                "cut-clipboard", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Insert, CDK_CONTROL_MASK,
                                "copy-clipboard", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Insert, CDK_SHIFT_MASK,
                                "paste-clipboard", 0);

  /* Overwrite */
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Insert, 0,
				"toggle-overwrite", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Insert, 0,
				"toggle-overwrite", 0);

  /* Emoji */
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_period, CDK_CONTROL_MASK,
                                "insert-emoji", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_semicolon, CDK_CONTROL_MASK,
                                "insert-emoji", 0);

  /* Caret mode */
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_F7, 0,
				"toggle-cursor-visible", 0);

  /* Control-tab focus motion */
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Tab, CDK_CONTROL_MASK,
				"move-focus", 1,
				CTK_TYPE_DIRECTION_TYPE, CTK_DIR_TAB_FORWARD);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Tab, CDK_CONTROL_MASK,
				"move-focus", 1,
				CTK_TYPE_DIRECTION_TYPE, CTK_DIR_TAB_FORWARD);
  
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Tab, CDK_SHIFT_MASK | CDK_CONTROL_MASK,
				"move-focus", 1,
				CTK_TYPE_DIRECTION_TYPE, CTK_DIR_TAB_BACKWARD);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Tab, CDK_SHIFT_MASK | CDK_CONTROL_MASK,
				"move-focus", 1,
				CTK_TYPE_DIRECTION_TYPE, CTK_DIR_TAB_BACKWARD);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_TEXT_VIEW_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "textview");

  quark_text_selection_data = g_quark_from_static_string ("ctk-text-view-text-selection-data");
  quark_ctk_signal = g_quark_from_static_string ("ctk-signal");
  quark_text_view_child = g_quark_from_static_string ("ctk-text-view-child");
}

static void
ctk_text_view_init (CtkTextView *text_view)
{
  CtkWidget *widget = CTK_WIDGET (text_view);
  CtkTargetList *target_list;
  CtkTextViewPrivate *priv;
  CtkStyleContext *context;

  text_view->priv = ctk_text_view_get_instance_private (text_view);
  priv = text_view->priv;

  ctk_widget_set_can_focus (widget, TRUE);

  priv->pixel_cache = _ctk_pixel_cache_new ();

  context = ctk_widget_get_style_context (CTK_WIDGET (text_view));
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_VIEW);

  /* Set up default style */
  priv->wrap_mode = CTK_WRAP_NONE;
  priv->pixels_above_lines = 0;
  priv->pixels_below_lines = 0;
  priv->pixels_inside_wrap = 0;
  priv->justify = CTK_JUSTIFY_LEFT;
  priv->indent = 0;
  priv->tabs = NULL;
  priv->editable = TRUE;

  priv->scroll_after_paste = FALSE;

  ctk_drag_dest_set (widget, 0, NULL, 0,
                     CDK_ACTION_COPY | CDK_ACTION_MOVE);

  target_list = ctk_target_list_new (NULL, 0);
  ctk_drag_dest_set_target_list (widget, target_list);
  ctk_target_list_unref (target_list);

  priv->virtual_cursor_x = -1;
  priv->virtual_cursor_y = -1;

  /* This object is completely private. No external entity can gain a reference
   * to it; so we create it here and destroy it in finalize ().
   */
  priv->im_context = ctk_im_multicontext_new ();

  g_signal_connect (priv->im_context, "commit",
                    G_CALLBACK (ctk_text_view_commit_handler), text_view);
  g_signal_connect (priv->im_context, "preedit-changed",
 		    G_CALLBACK (ctk_text_view_preedit_changed_handler), text_view);
  g_signal_connect (priv->im_context, "retrieve-surrounding",
 		    G_CALLBACK (ctk_text_view_retrieve_surrounding_handler), text_view);
  g_signal_connect (priv->im_context, "delete-surrounding",
 		    G_CALLBACK (ctk_text_view_delete_surrounding_handler), text_view);

  priv->cursor_visible = TRUE;

  priv->accepts_tab = TRUE;

  priv->text_window = text_window_new (CTK_TEXT_WINDOW_TEXT,
                                       widget, 200, 200);

  priv->multipress_gesture = ctk_gesture_multi_press_new (widget);
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (priv->multipress_gesture), 0);
  g_signal_connect (priv->multipress_gesture, "pressed",
                    G_CALLBACK (ctk_text_view_multipress_gesture_pressed),
                    widget);

  priv->drag_gesture = ctk_gesture_drag_new (widget);
  g_signal_connect (priv->drag_gesture, "drag-update",
                    G_CALLBACK (ctk_text_view_drag_gesture_update),
                    widget);
  g_signal_connect (priv->drag_gesture, "drag-end",
                    G_CALLBACK (ctk_text_view_drag_gesture_end),
                    widget);

  priv->selection_node = ctk_css_node_new ();
  ctk_css_node_set_name (priv->selection_node, I_("selection"));
  ctk_css_node_set_parent (priv->selection_node, priv->text_window->css_node);
  ctk_css_node_set_state (priv->selection_node,
                          ctk_css_node_get_state (priv->text_window->css_node) & ~CTK_STATE_FLAG_DROP_ACTIVE);
  ctk_css_node_set_visible (priv->selection_node, FALSE);
  g_object_unref (priv->selection_node);
}

CtkCssNode *
ctk_text_view_get_text_node (CtkTextView *text_view)
{
  return text_view->priv->text_window->css_node;
}

CtkCssNode *
ctk_text_view_get_selection_node (CtkTextView *text_view)
{
  return text_view->priv->selection_node;
}

static void
_ctk_text_view_ensure_text_handles (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv = text_view->priv;

  if (priv->text_handle)
    return;

  priv->text_handle = _ctk_text_handle_new (CTK_WIDGET (text_view));
  g_signal_connect (priv->text_handle, "drag-started",
                    G_CALLBACK (ctk_text_view_handle_drag_started), text_view);
  g_signal_connect (priv->text_handle, "handle-dragged",
                    G_CALLBACK (ctk_text_view_handle_dragged), text_view);
  g_signal_connect (priv->text_handle, "drag-finished",
                    G_CALLBACK (ctk_text_view_handle_drag_finished), text_view);
}

static void
_ctk_text_view_ensure_magnifier (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv = text_view->priv;

  if (priv->magnifier_popover)
    return;

  priv->magnifier = _ctk_magnifier_new (CTK_WIDGET (text_view));
  _ctk_magnifier_set_magnification (CTK_MAGNIFIER (priv->magnifier), 2.0);
  priv->magnifier_popover = ctk_popover_new (CTK_WIDGET (text_view));
  ctk_style_context_add_class (ctk_widget_get_style_context (priv->magnifier_popover),
                               "magnifier");
  ctk_popover_set_modal (CTK_POPOVER (priv->magnifier_popover), FALSE);
  ctk_container_add (CTK_CONTAINER (priv->magnifier_popover),
                     priv->magnifier);
  ctk_container_set_border_width (CTK_CONTAINER (priv->magnifier_popover), 4);
  ctk_widget_show (priv->magnifier);
}

/**
 * ctk_text_view_new:
 *
 * Creates a new #CtkTextView. If you don’t call ctk_text_view_set_buffer()
 * before using the text view, an empty default buffer will be created
 * for you. Get the buffer with ctk_text_view_get_buffer(). If you want
 * to specify your own buffer, consider ctk_text_view_new_with_buffer().
 *
 * Returns: a new #CtkTextView
 **/
CtkWidget*
ctk_text_view_new (void)
{
  return g_object_new (CTK_TYPE_TEXT_VIEW, NULL);
}

/**
 * ctk_text_view_new_with_buffer:
 * @buffer: a #CtkTextBuffer
 *
 * Creates a new #CtkTextView widget displaying the buffer
 * @buffer. One buffer can be shared among many widgets.
 * @buffer may be %NULL to create a default buffer, in which case
 * this function is equivalent to ctk_text_view_new(). The
 * text view adds its own reference count to the buffer; it does not
 * take over an existing reference.
 *
 * Returns: a new #CtkTextView.
 **/
CtkWidget*
ctk_text_view_new_with_buffer (CtkTextBuffer *buffer)
{
  CtkTextView *text_view;

  text_view = (CtkTextView*)ctk_text_view_new ();

  ctk_text_view_set_buffer (text_view, buffer);

  return CTK_WIDGET (text_view);
}

/**
 * ctk_text_view_set_buffer:
 * @text_view: a #CtkTextView
 * @buffer: (allow-none): a #CtkTextBuffer
 *
 * Sets @buffer as the buffer being displayed by @text_view. The previous
 * buffer displayed by the text view is unreferenced, and a reference is
 * added to @buffer. If you owned a reference to @buffer before passing it
 * to this function, you must remove that reference yourself; #CtkTextView
 * will not “adopt” it.
 **/
void
ctk_text_view_set_buffer (CtkTextView   *text_view,
                          CtkTextBuffer *buffer)
{
  CtkTextViewPrivate *priv;
  CtkTextBuffer *old_buffer;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (buffer == NULL || CTK_IS_TEXT_BUFFER (buffer));

  priv = text_view->priv;

  if (priv->buffer == buffer)
    return;

  old_buffer = priv->buffer;
  if (priv->buffer != NULL)
    {
      /* Destroy all anchored children */
      GSList *tmp_list;
      GSList *copy;

      copy = g_slist_copy (priv->children);
      tmp_list = copy;
      while (tmp_list != NULL)
        {
          CtkTextViewChild *vc = tmp_list->data;

          if (vc->anchor)
            {
              ctk_widget_destroy (vc->widget);
              /* vc may now be invalid! */
            }

          tmp_list = tmp_list->next;
        }

      g_slist_free (copy);

      g_signal_handlers_disconnect_by_func (priv->buffer,
					    ctk_text_view_mark_set_handler,
					    text_view);
      g_signal_handlers_disconnect_by_func (priv->buffer,
                                            ctk_text_view_target_list_notify,
                                            text_view);
      g_signal_handlers_disconnect_by_func (priv->buffer,
                                            ctk_text_view_paste_done_handler,
                                            text_view);
      g_signal_handlers_disconnect_by_func (priv->buffer,
                                            ctk_text_view_buffer_changed_handler,
                                            text_view);

      if (ctk_widget_get_realized (CTK_WIDGET (text_view)))
	{
	  CtkClipboard *clipboard = ctk_widget_get_clipboard (CTK_WIDGET (text_view),
							      CDK_SELECTION_PRIMARY);
	  ctk_text_buffer_remove_selection_clipboard (priv->buffer, clipboard);
        }

      if (priv->layout)
        ctk_text_layout_set_buffer (priv->layout, NULL);

      priv->dnd_mark = NULL;
      priv->first_para_mark = NULL;
      cancel_pending_scroll (text_view);
    }

  priv->buffer = buffer;

  if (priv->layout)
    ctk_text_layout_set_buffer (priv->layout, buffer);

  if (buffer != NULL)
    {
      CtkTextIter start;

      g_object_ref (buffer);

      ctk_text_buffer_get_iter_at_offset (priv->buffer, &start, 0);

      priv->dnd_mark = ctk_text_buffer_create_mark (priv->buffer,
                                                    "ctk_drag_target",
                                                    &start, FALSE);

      priv->first_para_mark = ctk_text_buffer_create_mark (priv->buffer,
                                                           NULL,
                                                           &start, TRUE);

      priv->first_para_pixels = 0;


      g_signal_connect (priv->buffer, "mark-set",
			G_CALLBACK (ctk_text_view_mark_set_handler),
                        text_view);
      g_signal_connect (priv->buffer, "notify::paste-target-list",
			G_CALLBACK (ctk_text_view_target_list_notify),
                        text_view);
      g_signal_connect (priv->buffer, "paste-done",
			G_CALLBACK (ctk_text_view_paste_done_handler),
                        text_view);
      g_signal_connect (priv->buffer, "changed",
			G_CALLBACK (ctk_text_view_buffer_changed_handler),
                        text_view);

      ctk_text_view_target_list_notify (priv->buffer, NULL, text_view);

      if (ctk_widget_get_realized (CTK_WIDGET (text_view)))
	{
	  CtkClipboard *clipboard = ctk_widget_get_clipboard (CTK_WIDGET (text_view),
							      CDK_SELECTION_PRIMARY);
	  ctk_text_buffer_add_selection_clipboard (priv->buffer, clipboard);
	}

      if (priv->text_handle)
        ctk_text_view_update_handles (text_view, CTK_TEXT_HANDLE_MODE_NONE);
    }

  _ctk_text_view_accessible_set_buffer (text_view, old_buffer);
  if (old_buffer)
    g_object_unref (old_buffer);

  g_object_notify (G_OBJECT (text_view), "buffer");
  
  if (ctk_widget_get_visible (CTK_WIDGET (text_view)))
    ctk_widget_queue_draw (CTK_WIDGET (text_view));

  DV(g_print ("Invalidating due to set_buffer\n"));
  ctk_text_view_invalidate (text_view);
}

static CtkTextBuffer*
ctk_text_view_create_buffer (CtkTextView *text_view)
{
  return ctk_text_buffer_new (NULL);
}

static CtkTextBuffer*
get_buffer (CtkTextView *text_view)
{
  if (text_view->priv->buffer == NULL)
    {
      CtkTextBuffer *b;
      b = CTK_TEXT_VIEW_GET_CLASS (text_view)->create_buffer (text_view);
      ctk_text_view_set_buffer (text_view, b);
      g_object_unref (b);
    }

  return text_view->priv->buffer;
}

/**
 * ctk_text_view_get_buffer:
 * @text_view: a #CtkTextView
 *
 * Returns the #CtkTextBuffer being displayed by this text view.
 * The reference count on the buffer is not incremented; the caller
 * of this function won’t own a new reference.
 *
 * Returns: (transfer none): a #CtkTextBuffer
 **/
CtkTextBuffer*
ctk_text_view_get_buffer (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), NULL);

  return get_buffer (text_view);
}

/**
 * ctk_text_view_get_cursor_locations:
 * @text_view: a #CtkTextView
 * @iter: (allow-none): a #CtkTextIter
 * @strong: (out) (allow-none): location to store the strong
 *     cursor position (may be %NULL)
 * @weak: (out) (allow-none): location to store the weak
 *     cursor position (may be %NULL)
 *
 * Given an @iter within a text layout, determine the positions of the
 * strong and weak cursors if the insertion point is at that
 * iterator. The position of each cursor is stored as a zero-width
 * rectangle. The strong cursor location is the location where
 * characters of the directionality equal to the base direction of the
 * paragraph are inserted.  The weak cursor location is the location
 * where characters of the directionality opposite to the base
 * direction of the paragraph are inserted.
 *
 * If @iter is %NULL, the actual cursor position is used.
 *
 * Note that if @iter happens to be the actual cursor position, and
 * there is currently an IM preedit sequence being entered, the
 * returned locations will be adjusted to account for the preedit
 * cursor’s offset within the preedit sequence.
 *
 * The rectangle position is in buffer coordinates; use
 * ctk_text_view_buffer_to_window_coords() to convert these
 * coordinates to coordinates for one of the windows in the text view.
 *
 * Since: 3.0
 **/
void
ctk_text_view_get_cursor_locations (CtkTextView       *text_view,
                                    const CtkTextIter *iter,
                                    CdkRectangle      *strong,
                                    CdkRectangle      *weak)
{
  CtkTextIter insert;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (iter == NULL ||
                    ctk_text_iter_get_buffer (iter) == get_buffer (text_view));

  ctk_text_view_ensure_layout (text_view);

  if (iter)
    insert = *iter;
  else
    ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &insert,
                                      ctk_text_buffer_get_insert (get_buffer (text_view)));

  ctk_text_layout_get_cursor_locations (text_view->priv->layout, &insert,
                                        strong, weak);
}

/**
 * ctk_text_view_get_iter_at_location:
 * @text_view: a #CtkTextView
 * @iter: (out): a #CtkTextIter
 * @x: x position, in buffer coordinates
 * @y: y position, in buffer coordinates
 *
 * Retrieves the iterator at buffer coordinates @x and @y. Buffer
 * coordinates are coordinates for the entire buffer, not just the
 * currently-displayed portion.  If you have coordinates from an
 * event, you have to convert those to buffer coordinates with
 * ctk_text_view_window_to_buffer_coords().
 *
 * Returns: %TRUE if the position is over text
 */
gboolean
ctk_text_view_get_iter_at_location (CtkTextView *text_view,
                                    CtkTextIter *iter,
                                    gint         x,
                                    gint         y)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  ctk_text_view_ensure_layout (text_view);

  return ctk_text_layout_get_iter_at_pixel (text_view->priv->layout, iter, x, y);
}

/**
 * ctk_text_view_get_iter_at_position:
 * @text_view: a #CtkTextView
 * @iter: (out): a #CtkTextIter
 * @trailing: (out) (allow-none): if non-%NULL, location to store an integer indicating where
 *    in the grapheme the user clicked. It will either be
 *    zero, or the number of characters in the grapheme.
 *    0 represents the trailing edge of the grapheme.
 * @x: x position, in buffer coordinates
 * @y: y position, in buffer coordinates
 *
 * Retrieves the iterator pointing to the character at buffer
 * coordinates @x and @y. Buffer coordinates are coordinates for
 * the entire buffer, not just the currently-displayed portion.
 * If you have coordinates from an event, you have to convert
 * those to buffer coordinates with
 * ctk_text_view_window_to_buffer_coords().
 *
 * Note that this is different from ctk_text_view_get_iter_at_location(),
 * which returns cursor locations, i.e. positions between
 * characters.
 *
 * Returns: %TRUE if the position is over text
 *
 * Since: 2.6
 **/
gboolean
ctk_text_view_get_iter_at_position (CtkTextView *text_view,
                                    CtkTextIter *iter,
                                    gint        *trailing,
                                    gint         x,
                                    gint         y)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  ctk_text_view_ensure_layout (text_view);

  return ctk_text_layout_get_iter_at_position (text_view->priv->layout, iter, trailing, x, y);
}

/**
 * ctk_text_view_get_iter_location:
 * @text_view: a #CtkTextView
 * @iter: a #CtkTextIter
 * @location: (out): bounds of the character at @iter
 *
 * Gets a rectangle which roughly contains the character at @iter.
 * The rectangle position is in buffer coordinates; use
 * ctk_text_view_buffer_to_window_coords() to convert these
 * coordinates to coordinates for one of the windows in the text view.
 **/
void
ctk_text_view_get_iter_location (CtkTextView       *text_view,
                                 const CtkTextIter *iter,
                                 CdkRectangle      *location)
{
  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (ctk_text_iter_get_buffer (iter) == get_buffer (text_view));

  ctk_text_view_ensure_layout (text_view);
  
  ctk_text_layout_get_iter_location (text_view->priv->layout, iter, location);
}

/**
 * ctk_text_view_get_line_yrange:
 * @text_view: a #CtkTextView
 * @iter: a #CtkTextIter
 * @y: (out): return location for a y coordinate
 * @height: (out): return location for a height
 *
 * Gets the y coordinate of the top of the line containing @iter,
 * and the height of the line. The coordinate is a buffer coordinate;
 * convert to window coordinates with ctk_text_view_buffer_to_window_coords().
 **/
void
ctk_text_view_get_line_yrange (CtkTextView       *text_view,
                               const CtkTextIter *iter,
                               gint              *y,
                               gint              *height)
{
  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (ctk_text_iter_get_buffer (iter) == get_buffer (text_view));

  ctk_text_view_ensure_layout (text_view);
  
  ctk_text_layout_get_line_yrange (text_view->priv->layout,
                                   iter,
                                   y,
                                   height);
}

/**
 * ctk_text_view_get_line_at_y:
 * @text_view: a #CtkTextView
 * @target_iter: (out): a #CtkTextIter
 * @y: a y coordinate
 * @line_top: (out): return location for top coordinate of the line
 *
 * Gets the #CtkTextIter at the start of the line containing
 * the coordinate @y. @y is in buffer coordinates, convert from
 * window coordinates with ctk_text_view_window_to_buffer_coords().
 * If non-%NULL, @line_top will be filled with the coordinate of the top
 * edge of the line.
 **/
void
ctk_text_view_get_line_at_y (CtkTextView *text_view,
                             CtkTextIter *target_iter,
                             gint         y,
                             gint        *line_top)
{
  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  ctk_text_view_ensure_layout (text_view);
  
  ctk_text_layout_get_line_at_y (text_view->priv->layout,
                                 target_iter,
                                 y,
                                 line_top);
}

/* Same as ctk_text_view_scroll_to_iter but deal with
 * (top_margin / top_padding) and (bottom_margin / bottom_padding).
 * When with_border == TRUE and you scroll on the edges,
 * all borders are shown for the corresponding edge.
 * When with_border == FALSE, only left margin and right_margin
 * can be seen because they can be can be overwritten by tags.
 */
static gboolean
_ctk_text_view_scroll_to_iter (CtkTextView   *text_view,
                               CtkTextIter   *iter,
                               gdouble        within_margin,
                               gboolean       use_align,
                               gdouble        xalign,
                               gdouble        yalign,
                               gboolean       with_border)
{
  CtkTextViewPrivate *priv = text_view->priv;
  CtkWidget *widget;

  CdkRectangle cursor;
  gint cursor_bottom;
  gint cursor_right;

  CdkRectangle screen;
  CdkRectangle screen_dest;

  gint screen_inner_left;
  gint screen_inner_right;
  gint screen_inner_top;
  gint screen_inner_bottom;

  gint border_xoffset = 0;
  gint border_yoffset = 0;
  gint within_margin_xoffset;
  gint within_margin_yoffset;

  gint buffer_bottom;
  gint buffer_right;

  gboolean retval = FALSE;

  /* FIXME why don't we do the validate-at-scroll-destination thing
   * from flush_scroll in this function? I think it wasn't done before
   * because changed_handler was screwed up, but I could be wrong.
   */
  
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (within_margin >= 0.0 && within_margin < 0.5, FALSE);
  g_return_val_if_fail (xalign >= 0.0 && xalign <= 1.0, FALSE);
  g_return_val_if_fail (yalign >= 0.0 && yalign <= 1.0, FALSE);
  
  widget = CTK_WIDGET (text_view);

  DV(g_print(G_STRLOC"\n"));
  
  ctk_text_layout_get_iter_location (priv->layout,
                                     iter,
                                     &cursor);

  DV (g_print (" target cursor %d,%d  %d x %d\n", cursor.x, cursor.y, cursor.width, cursor.height));

  /* In each direction, *_border are the addition of *_padding and *_margin
   *
   * Vadjustment value:
   * (-priv->top_border) [top padding][top margin] (0) [text][bottom margin][bottom padding]
   *
   * Hadjustment value:
   * (-priv->left_padding) [left padding] (0) [left margin][text][right margin][right padding]
   *
   * Buffer coordinates:
   * on x: (0) [left margin][text][right margin]
   * on y: (0) [text]
   *
   * left margin and right margin are part of the x buffer coordinate
   * because they are part of the pango layout so that they can be
   * overwritten by tags.
   *
   * Canvas coordinates:
   * (the canvas is the virtual window where the content of the buffer is drawn )
   *
   * on x: (-priv->left_padding) [left padding] (0) [left margin][text][right margin][right padding]
   * on y: (-priv->top_border) [top margin][top padding] (0) [text][bottom margin][bottom padding]
   *
   * (priv->xoffset, priv->yoffset) is the origin of the view (visible part of the canvas)
   *  in canvas coordinates.
   * As you can see, canvas coordinates and buffer coordinates are compatible but the canvas
   * can be larger than the buffer depending of the border size.
   */

  cursor_bottom = cursor.y + cursor.height;
  cursor_right = cursor.x + cursor.width;

  /* Current position of the view in canvas coordinates */
  screen.x = priv->xoffset;
  screen.y = priv->yoffset;
  screen.width = SCREEN_WIDTH (widget);
  screen.height = SCREEN_HEIGHT (widget);

  within_margin_xoffset = screen.width * within_margin;
  within_margin_yoffset = screen.height * within_margin;

  screen_inner_left = screen.x + within_margin_xoffset;
  screen_inner_top = screen.y + within_margin_yoffset;
  screen_inner_right = screen.x + screen.width - within_margin_xoffset;
  screen_inner_bottom = screen.y + screen.height - within_margin_yoffset;

  buffer_bottom = priv->height - priv->bottom_border;
  buffer_right = priv->width - priv->right_margin - priv->left_padding - 1;

  screen_dest.x = screen.x;
  screen_dest.y = screen.y;
  screen_dest.width = screen.width - within_margin_xoffset * 2;
  screen_dest.height = screen.height - within_margin_yoffset * 2;

  /* Minimum authorised size check */
  if (screen_dest.width < 1)
    screen_dest.width = 1;
  if (screen_dest.height < 1)
    screen_dest.height = 1;

  /* The alignment affects the point in the target character that we
   * choose to align. If we're doing right/bottom alignment, we align
   * the right/bottom edge of the character the mark is at; if we're
   * doing left/top we align the left/top edge of the character; if
   * we're doing center alignment we align the center of the
   * character.
   *
   * The differents cases handle on each direction:
   *   1. cursor outside of the inner area define by within_margin
   *   2. if use_align == TRUE, alignment with xalign and yalign
   *   3. scrolling on the edges dependent of with_border
   */

  /* Vertical scroll */
  if (use_align)
    {
      gint cursor_y_alignment_offset;

      cursor_y_alignment_offset = (cursor.height * yalign) - (screen_dest.height * yalign);
      screen_dest.y = cursor.y + cursor_y_alignment_offset - within_margin_yoffset;
    }
  else
    {
      /* move minimum to get onscreen, showing the
       * top_border or bottom_border when necessary
       */
      if (cursor.y < screen_inner_top)
        {
          if (cursor.y == 0)
            border_yoffset = (with_border) ? priv->top_padding : 0;

          screen_dest.y = cursor.y - MAX (within_margin_yoffset, border_yoffset);
        }
      else if (cursor_bottom > screen_inner_bottom)
        {
          if (cursor_bottom == buffer_bottom - priv->top_border)
            border_yoffset = (with_border) ? priv->bottom_padding : 0;

          screen_dest.y = cursor_bottom - screen_dest.height +
                          MAX (within_margin_yoffset, border_yoffset);
        }
    }

  if (screen_dest.y != screen.y)
    {
      ctk_adjustment_animate_to_value (priv->vadjustment, screen_dest.y  + priv->top_border);

      DV (g_print (" vert increment %d\n", screen_dest.y - screen.y));
    }

  /* Horizontal scroll */

  if (use_align)
    {
      gint cursor_x_alignment_offset;

      cursor_x_alignment_offset = (cursor.width * xalign) - (screen_dest.width * xalign);
      screen_dest.x = cursor.x + cursor_x_alignment_offset - within_margin_xoffset;
    }
  else
    {
      /* move minimum to get onscreen, showing the
       * left_border or right_border when necessary
       */
      if (cursor.x < screen_inner_left)
        {
          if (cursor.x == priv->left_margin)
            border_xoffset = (with_border) ? priv->left_padding : 0;

          screen_dest.x = cursor.x - MAX (within_margin_xoffset, border_xoffset);
        }
      else if (cursor_right >= screen_inner_right - 1)
        {
          if (cursor.x >= buffer_right - priv->right_padding)
            border_xoffset = (with_border) ? priv->right_padding : 0;

          screen_dest.x = cursor_right - screen_dest.width +
                          MAX (within_margin_xoffset, border_xoffset) + 1;
        }
    }

  if (screen_dest.x != screen.x)
    {
      ctk_adjustment_animate_to_value (priv->hadjustment, screen_dest.x + priv->left_padding);

      DV (g_print (" horiz increment %d\n", screen_dest.x - screen.x));
    }

  retval = (screen.y != screen_dest.y) || (screen.x != screen_dest.x);

  DV(g_print (">%s ("G_STRLOC")\n", retval ? "Actually scrolled" : "Didn't end up scrolling"));
  
  return retval;
}

/**
 * ctk_text_view_scroll_to_iter:
 * @text_view: a #CtkTextView
 * @iter: a #CtkTextIter
 * @within_margin: margin as a [0.0,0.5) fraction of screen size
 * @use_align: whether to use alignment arguments (if %FALSE,
 *    just get the mark onscreen)
 * @xalign: horizontal alignment of mark within visible area
 * @yalign: vertical alignment of mark within visible area
 *
 * Scrolls @text_view so that @iter is on the screen in the position
 * indicated by @xalign and @yalign. An alignment of 0.0 indicates
 * left or top, 1.0 indicates right or bottom, 0.5 means center.
 * If @use_align is %FALSE, the text scrolls the minimal distance to
 * get the mark onscreen, possibly not scrolling at all. The effective
 * screen for purposes of this function is reduced by a margin of size
 * @within_margin.
 *
 * Note that this function uses the currently-computed height of the
 * lines in the text buffer. Line heights are computed in an idle
 * handler; so this function may not have the desired effect if it’s
 * called before the height computations. To avoid oddness, consider
 * using ctk_text_view_scroll_to_mark() which saves a point to be
 * scrolled to after line validation.
 *
 * Returns: %TRUE if scrolling occurred
 **/
gboolean
ctk_text_view_scroll_to_iter (CtkTextView   *text_view,
                              CtkTextIter   *iter,
                              gdouble        within_margin,
                              gboolean       use_align,
                              gdouble        xalign,
                              gdouble        yalign)
{
  return _ctk_text_view_scroll_to_iter (text_view,
                                        iter,
                                        within_margin,
                                        use_align,
                                        xalign,
                                        yalign,
                                        FALSE);
}

static void
free_pending_scroll (CtkTextPendingScroll *scroll)
{
  if (!ctk_text_mark_get_deleted (scroll->mark))
    ctk_text_buffer_delete_mark (ctk_text_mark_get_buffer (scroll->mark),
                                 scroll->mark);
  g_object_unref (scroll->mark);
  g_slice_free (CtkTextPendingScroll, scroll);
}

static void
cancel_pending_scroll (CtkTextView *text_view)
{
  if (text_view->priv->pending_scroll)
    {
      free_pending_scroll (text_view->priv->pending_scroll);
      text_view->priv->pending_scroll = NULL;
    }
}

static void
ctk_text_view_queue_scroll (CtkTextView   *text_view,
                            CtkTextMark   *mark,
                            gdouble        within_margin,
                            gboolean       use_align,
                            gdouble        xalign,
                            gdouble        yalign)
{
  CtkTextIter iter;
  CtkTextPendingScroll *scroll;

  DV(g_print(G_STRLOC"\n"));
  
  scroll = g_slice_new (CtkTextPendingScroll);

  scroll->within_margin = within_margin;
  scroll->use_align = use_align;
  scroll->xalign = xalign;
  scroll->yalign = yalign;
  
  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &iter, mark);

  scroll->mark = ctk_text_buffer_create_mark (get_buffer (text_view),
                                              NULL,
                                              &iter,
                                              ctk_text_mark_get_left_gravity (mark));

  g_object_ref (scroll->mark);
  
  cancel_pending_scroll (text_view);

  text_view->priv->pending_scroll = scroll;
}

static gboolean
ctk_text_view_flush_scroll (CtkTextView *text_view)
{
  CtkAllocation allocation;
  CtkTextIter iter;
  CtkTextPendingScroll *scroll;
  gboolean retval;
  CtkWidget *widget;

  widget = CTK_WIDGET (text_view);
  
  DV(g_print(G_STRLOC"\n"));
  
  if (text_view->priv->pending_scroll == NULL)
    {
      DV (g_print ("in flush scroll, no pending scroll\n"));
      return FALSE;
    }

  scroll = text_view->priv->pending_scroll;

  /* avoid recursion */
  text_view->priv->pending_scroll = NULL;
  
  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &iter, scroll->mark);

  /* Validate area around the scroll destination, so the adjustment
   * can meaningfully point into that area. We must validate
   * enough area to be sure that after we scroll, everything onscreen
   * is valid; otherwise, validation will maintain the first para
   * in one place, but may push the target iter off the bottom of
   * the screen.
   */
  DV(g_print (">Validating scroll destination ("G_STRLOC")\n"));
  ctk_widget_get_allocation (widget, &allocation);
  ctk_text_layout_validate_yrange (text_view->priv->layout, &iter,
                                   -(allocation.height * 2),
                                   allocation.height * 2);

  DV(g_print (">Done validating scroll destination ("G_STRLOC")\n"));

  /* Ensure we have updated width/height */
  ctk_text_view_update_adjustments (text_view);
  
  retval = _ctk_text_view_scroll_to_iter (text_view,
                                          &iter,
                                          scroll->within_margin,
                                          scroll->use_align,
                                          scroll->xalign,
                                          scroll->yalign,
                                          TRUE);

  if (text_view->priv->text_handle)
    ctk_text_view_update_handles (text_view,
                                  _ctk_text_handle_get_mode (text_view->priv->text_handle));

  free_pending_scroll (scroll);

  return retval;
}

static void
ctk_text_view_update_adjustments (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv;
  gint width = 0, height = 0;

  DV(g_print(">Updating adjustments ("G_STRLOC")\n"));

  priv = text_view->priv;

  if (priv->layout)
    ctk_text_layout_get_size (priv->layout, &width, &height);

  /* Make room for the cursor after the last character in the widest line */
  width += SPACE_FOR_CURSOR;
  height += priv->top_border + priv->bottom_border;

  if (priv->width != width || priv->height != height)
    {
      if (priv->width != width)
	priv->width_changed = TRUE;

      priv->width = width;
      priv->height = height;

      ctk_text_view_set_hadjustment_values (text_view);
      ctk_text_view_set_vadjustment_values (text_view);
    }
}

static void
ctk_text_view_update_layout_width (CtkTextView *text_view)
{
  DV(g_print(">Updating layout width ("G_STRLOC")\n"));
  
  ctk_text_view_ensure_layout (text_view);

  ctk_text_layout_set_screen_width (text_view->priv->layout,
                                    MAX (1, SCREEN_WIDTH (text_view) - SPACE_FOR_CURSOR));
}

static void
ctk_text_view_update_im_spot_location (CtkTextView *text_view)
{
  CdkRectangle area;

  if (text_view->priv->layout == NULL)
    return;
  
  ctk_text_view_get_cursor_locations (text_view, NULL, &area, NULL);

  area.x -= text_view->priv->xoffset;
  area.y -= text_view->priv->yoffset;
    
  /* Width returned by Pango indicates direction of cursor,
   * by its sign more than the size of cursor.
   */
  area.width = 0;

  ctk_im_context_set_cursor_location (text_view->priv->im_context, &area);
}

static gboolean
do_update_im_spot_location (gpointer text_view)
{
  CtkTextViewPrivate *priv;

  priv = CTK_TEXT_VIEW (text_view)->priv;
  priv->im_spot_idle = 0;

  ctk_text_view_update_im_spot_location (text_view);
  return FALSE;
}

static void
queue_update_im_spot_location (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv;

  priv = text_view->priv;

  /* Use priority a little higher than CTK_TEXT_VIEW_PRIORITY_VALIDATE,
   * so we don't wait until the entire buffer has been validated. */
  if (!priv->im_spot_idle)
    {
      priv->im_spot_idle = cdk_threads_add_idle_full (CTK_TEXT_VIEW_PRIORITY_VALIDATE - 1,
						      do_update_im_spot_location,
						      text_view,
						      NULL);
      g_source_set_name_by_id (priv->im_spot_idle, "[ctk+] do_update_im_spot_location");
    }
}

static void
flush_update_im_spot_location (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv;

  priv = text_view->priv;

  if (priv->im_spot_idle)
    {
      g_source_remove (priv->im_spot_idle);
      priv->im_spot_idle = 0;
      ctk_text_view_update_im_spot_location (text_view);
    }
}

/**
 * ctk_text_view_scroll_to_mark:
 * @text_view: a #CtkTextView
 * @mark: a #CtkTextMark
 * @within_margin: margin as a [0.0,0.5) fraction of screen size
 * @use_align: whether to use alignment arguments (if %FALSE, just 
 *    get the mark onscreen)
 * @xalign: horizontal alignment of mark within visible area
 * @yalign: vertical alignment of mark within visible area
 *
 * Scrolls @text_view so that @mark is on the screen in the position
 * indicated by @xalign and @yalign. An alignment of 0.0 indicates
 * left or top, 1.0 indicates right or bottom, 0.5 means center. 
 * If @use_align is %FALSE, the text scrolls the minimal distance to 
 * get the mark onscreen, possibly not scrolling at all. The effective 
 * screen for purposes of this function is reduced by a margin of size 
 * @within_margin.
 **/
void
ctk_text_view_scroll_to_mark (CtkTextView *text_view,
                              CtkTextMark *mark,
                              gdouble      within_margin,
                              gboolean     use_align,
                              gdouble      xalign,
                              gdouble      yalign)
{  
  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (CTK_IS_TEXT_MARK (mark));
  g_return_if_fail (within_margin >= 0.0 && within_margin < 0.5);
  g_return_if_fail (xalign >= 0.0 && xalign <= 1.0);
  g_return_if_fail (yalign >= 0.0 && yalign <= 1.0);

  /* We need to verify that the buffer contains the mark, otherwise this
   * can lead to data structure corruption later on.
   */
  g_return_if_fail (get_buffer (text_view) == ctk_text_mark_get_buffer (mark));

  ctk_text_view_queue_scroll (text_view, mark,
                              within_margin,
                              use_align,
                              xalign,
                              yalign);

  /* If no validation is pending, we need to go ahead and force an
   * immediate scroll.
   */
  if (text_view->priv->layout &&
      ctk_text_layout_is_valid (text_view->priv->layout))
    ctk_text_view_flush_scroll (text_view);
}

/**
 * ctk_text_view_scroll_mark_onscreen:
 * @text_view: a #CtkTextView
 * @mark: a mark in the buffer for @text_view
 * 
 * Scrolls @text_view the minimum distance such that @mark is contained
 * within the visible area of the widget.
 **/
void
ctk_text_view_scroll_mark_onscreen (CtkTextView *text_view,
                                    CtkTextMark *mark)
{
  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (CTK_IS_TEXT_MARK (mark));

  /* We need to verify that the buffer contains the mark, otherwise this
   * can lead to data structure corruption later on.
   */
  g_return_if_fail (get_buffer (text_view) == ctk_text_mark_get_buffer (mark));

  ctk_text_view_scroll_to_mark (text_view, mark, 0.0, FALSE, 0.0, 0.0);
}

static gboolean
clamp_iter_onscreen (CtkTextView *text_view, CtkTextIter *iter)
{
  CdkRectangle visible_rect;
  ctk_text_view_get_visible_rect (text_view, &visible_rect);

  return ctk_text_layout_clamp_iter_to_vrange (text_view->priv->layout, iter,
                                               visible_rect.y,
                                               visible_rect.y + visible_rect.height);
}

/**
 * ctk_text_view_move_mark_onscreen:
 * @text_view: a #CtkTextView
 * @mark: a #CtkTextMark
 *
 * Moves a mark within the buffer so that it's
 * located within the currently-visible text area.
 *
 * Returns: %TRUE if the mark moved (wasn’t already onscreen)
 **/
gboolean
ctk_text_view_move_mark_onscreen (CtkTextView *text_view,
                                  CtkTextMark *mark)
{
  CtkTextIter iter;

  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (mark != NULL, FALSE);

  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &iter, mark);

  if (clamp_iter_onscreen (text_view, &iter))
    {
      ctk_text_buffer_move_mark (get_buffer (text_view), mark, &iter);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * ctk_text_view_get_visible_rect:
 * @text_view: a #CtkTextView
 * @visible_rect: (out): rectangle to fill
 *
 * Fills @visible_rect with the currently-visible
 * region of the buffer, in buffer coordinates. Convert to window coordinates
 * with ctk_text_view_buffer_to_window_coords().
 **/
void
ctk_text_view_get_visible_rect (CtkTextView  *text_view,
                                CdkRectangle *visible_rect)
{
  CtkWidget *widget;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  widget = CTK_WIDGET (text_view);

  if (visible_rect)
    {
      visible_rect->x = text_view->priv->xoffset;
      visible_rect->y = text_view->priv->yoffset;
      visible_rect->width = SCREEN_WIDTH (widget);
      visible_rect->height = SCREEN_HEIGHT (widget);

      DV(g_print(" visible rect: %d,%d %d x %d\n",
                 visible_rect->x,
                 visible_rect->y,
                 visible_rect->width,
                 visible_rect->height));
    }
}

/**
 * ctk_text_view_set_wrap_mode:
 * @text_view: a #CtkTextView
 * @wrap_mode: a #CtkWrapMode
 *
 * Sets the line wrapping for the view.
 **/
void
ctk_text_view_set_wrap_mode (CtkTextView *text_view,
                             CtkWrapMode  wrap_mode)
{
  CtkTextViewPrivate *priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  priv = text_view->priv;

  if (priv->wrap_mode != wrap_mode)
    {
      priv->wrap_mode = wrap_mode;

      if (priv->layout && priv->layout->default_style)
        {
          priv->layout->default_style->wrap_mode = wrap_mode;
          ctk_text_layout_default_style_changed (priv->layout);
        }
      g_object_notify (G_OBJECT (text_view), "wrap-mode");
    }
}

/**
 * ctk_text_view_get_wrap_mode:
 * @text_view: a #CtkTextView
 *
 * Gets the line wrapping for the view.
 *
 * Returns: the line wrap setting
 **/
CtkWrapMode
ctk_text_view_get_wrap_mode (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), CTK_WRAP_NONE);

  return text_view->priv->wrap_mode;
}

/**
 * ctk_text_view_set_editable:
 * @text_view: a #CtkTextView
 * @setting: whether it’s editable
 *
 * Sets the default editability of the #CtkTextView. You can override
 * this default setting with tags in the buffer, using the “editable”
 * attribute of tags.
 **/
void
ctk_text_view_set_editable (CtkTextView *text_view,
                            gboolean     setting)
{
  CtkTextViewPrivate *priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  priv = text_view->priv;
  setting = setting != FALSE;

  if (priv->editable != setting)
    {
      if (!setting)
	{
	  ctk_text_view_reset_im_context(text_view);
	  if (ctk_widget_has_focus (CTK_WIDGET (text_view)))
	    ctk_im_context_focus_out (priv->im_context);
	}

      priv->editable = setting;

      if (setting && ctk_widget_has_focus (CTK_WIDGET (text_view)))
	ctk_im_context_focus_in (priv->im_context);

      if (priv->layout && priv->layout->default_style)
        {
	  ctk_text_layout_set_overwrite_mode (priv->layout,
					      priv->overwrite_mode && priv->editable);
          priv->layout->default_style->editable = priv->editable;
          ctk_text_layout_default_style_changed (priv->layout);
        }

      g_object_notify (G_OBJECT (text_view), "editable");
    }
}

/**
 * ctk_text_view_get_editable:
 * @text_view: a #CtkTextView
 *
 * Returns the default editability of the #CtkTextView. Tags in the
 * buffer may override this setting for some ranges of text.
 *
 * Returns: whether text is editable by default
 **/
gboolean
ctk_text_view_get_editable (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);

  return text_view->priv->editable;
}

/**
 * ctk_text_view_set_pixels_above_lines:
 * @text_view: a #CtkTextView
 * @pixels_above_lines: pixels above paragraphs
 * 
 * Sets the default number of blank pixels above paragraphs in @text_view.
 * Tags in the buffer for @text_view may override the defaults.
 **/
void
ctk_text_view_set_pixels_above_lines (CtkTextView *text_view,
                                      gint         pixels_above_lines)
{
  CtkTextViewPrivate *priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  priv = text_view->priv;

  if (priv->pixels_above_lines != pixels_above_lines)
    {
      priv->pixels_above_lines = pixels_above_lines;

      if (priv->layout && priv->layout->default_style)
        {
          priv->layout->default_style->pixels_above_lines = pixels_above_lines;
          ctk_text_layout_default_style_changed (priv->layout);
        }

      g_object_notify (G_OBJECT (text_view), "pixels-above-lines");
    }
}

/**
 * ctk_text_view_get_pixels_above_lines:
 * @text_view: a #CtkTextView
 * 
 * Gets the default number of pixels to put above paragraphs.
 * Adding this function with ctk_text_view_get_pixels_below_lines()
 * is equal to the line space between each paragraph.
 * 
 * Returns: default number of pixels above paragraphs
 **/
gint
ctk_text_view_get_pixels_above_lines (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), 0);

  return text_view->priv->pixels_above_lines;
}

/**
 * ctk_text_view_set_pixels_below_lines:
 * @text_view: a #CtkTextView
 * @pixels_below_lines: pixels below paragraphs 
 *
 * Sets the default number of pixels of blank space
 * to put below paragraphs in @text_view. May be overridden
 * by tags applied to @text_view’s buffer. 
 **/
void
ctk_text_view_set_pixels_below_lines (CtkTextView *text_view,
                                      gint         pixels_below_lines)
{
  CtkTextViewPrivate *priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  priv = text_view->priv;

  if (priv->pixels_below_lines != pixels_below_lines)
    {
      priv->pixels_below_lines = pixels_below_lines;

      if (priv->layout && priv->layout->default_style)
        {
          priv->layout->default_style->pixels_below_lines = pixels_below_lines;
          ctk_text_layout_default_style_changed (priv->layout);
        }

      g_object_notify (G_OBJECT (text_view), "pixels-below-lines");
    }
}

/**
 * ctk_text_view_get_pixels_below_lines:
 * @text_view: a #CtkTextView
 * 
 * Gets the value set by ctk_text_view_set_pixels_below_lines().
 *
 * The line space is the sum of the value returned by this function and the
 * value returned by ctk_text_view_get_pixels_above_lines().
 *
 * Returns: default number of blank pixels below paragraphs
 **/
gint
ctk_text_view_get_pixels_below_lines (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), 0);

  return text_view->priv->pixels_below_lines;
}

/**
 * ctk_text_view_set_pixels_inside_wrap:
 * @text_view: a #CtkTextView
 * @pixels_inside_wrap: default number of pixels between wrapped lines
 *
 * Sets the default number of pixels of blank space to leave between
 * display/wrapped lines within a paragraph. May be overridden by
 * tags in @text_view’s buffer.
 **/
void
ctk_text_view_set_pixels_inside_wrap (CtkTextView *text_view,
                                      gint         pixels_inside_wrap)
{
  CtkTextViewPrivate *priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  priv = text_view->priv;

  if (priv->pixels_inside_wrap != pixels_inside_wrap)
    {
      priv->pixels_inside_wrap = pixels_inside_wrap;

      if (priv->layout && priv->layout->default_style)
        {
          priv->layout->default_style->pixels_inside_wrap = pixels_inside_wrap;
          ctk_text_layout_default_style_changed (priv->layout);
        }

      g_object_notify (G_OBJECT (text_view), "pixels-inside-wrap");
    }
}

/**
 * ctk_text_view_get_pixels_inside_wrap:
 * @text_view: a #CtkTextView
 * 
 * Gets the value set by ctk_text_view_set_pixels_inside_wrap().
 * 
 * Returns: default number of pixels of blank space between wrapped lines
 **/
gint
ctk_text_view_get_pixels_inside_wrap (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), 0);

  return text_view->priv->pixels_inside_wrap;
}

/**
 * ctk_text_view_set_justification:
 * @text_view: a #CtkTextView
 * @justification: justification
 *
 * Sets the default justification of text in @text_view.
 * Tags in the view’s buffer may override the default.
 * 
 **/
void
ctk_text_view_set_justification (CtkTextView     *text_view,
                                 CtkJustification justification)
{
  CtkTextViewPrivate *priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  priv = text_view->priv;

  if (priv->justify != justification)
    {
      priv->justify = justification;

      if (priv->layout && priv->layout->default_style)
        {
          priv->layout->default_style->justification = justification;
          ctk_text_layout_default_style_changed (priv->layout);
        }

      g_object_notify (G_OBJECT (text_view), "justification");
    }
}

/**
 * ctk_text_view_get_justification:
 * @text_view: a #CtkTextView
 * 
 * Gets the default justification of paragraphs in @text_view.
 * Tags in the buffer may override the default.
 * 
 * Returns: default justification
 **/
CtkJustification
ctk_text_view_get_justification (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), CTK_JUSTIFY_LEFT);

  return text_view->priv->justify;
}

/**
 * ctk_text_view_set_left_margin:
 * @text_view: a #CtkTextView
 * @left_margin: left margin in pixels
 *
 * Sets the default left margin for text in @text_view.
 * Tags in the buffer may override the default.
 *
 * Note that this function is confusingly named.
 * In CSS terms, the value set here is padding.
 */
void
ctk_text_view_set_left_margin (CtkTextView *text_view,
                               gint         left_margin)
{
  CtkTextViewPrivate *priv = text_view->priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  if (priv->left_margin != left_margin)
    {
      priv->left_margin = left_margin;
      priv->left_border = left_margin + priv->left_padding;

      if (priv->layout && priv->layout->default_style)
        {
          priv->layout->default_style->left_margin = left_margin;
          ctk_text_layout_default_style_changed (priv->layout);
        }

      g_object_notify (G_OBJECT (text_view), "left-margin");
    }
}

/**
 * ctk_text_view_get_left_margin:
 * @text_view: a #CtkTextView
 *
 * Gets the default left margin size of paragraphs in the @text_view.
 * Tags in the buffer may override the default.
 *
 * Returns: left margin in pixels
 */
gint
ctk_text_view_get_left_margin (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), 0);

  return text_view->priv->left_margin;
}

/**
 * ctk_text_view_set_right_margin:
 * @text_view: a #CtkTextView
 * @right_margin: right margin in pixels
 *
 * Sets the default right margin for text in the text view.
 * Tags in the buffer may override the default.
 *
 * Note that this function is confusingly named.
 * In CSS terms, the value set here is padding.
 */
void
ctk_text_view_set_right_margin (CtkTextView *text_view,
                                gint         right_margin)
{
  CtkTextViewPrivate *priv = text_view->priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  if (priv->right_margin != right_margin)
    {
      priv->right_margin = right_margin;
      priv->right_border = right_margin + priv->right_padding;

      if (priv->layout && priv->layout->default_style)
        {
          priv->layout->default_style->right_margin = right_margin;
          ctk_text_layout_default_style_changed (priv->layout);
        }

      g_object_notify (G_OBJECT (text_view), "right-margin");
    }
}

/**
 * ctk_text_view_get_right_margin:
 * @text_view: a #CtkTextView
 *
 * Gets the default right margin for text in @text_view. Tags
 * in the buffer may override the default.
 *
 * Returns: right margin in pixels
 */
gint
ctk_text_view_get_right_margin (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), 0);

  return text_view->priv->right_margin;
}

/**
 * ctk_text_view_set_top_margin:
 * @text_view: a #CtkTextView
 * @top_margin: top margin in pixels
 *
 * Sets the top margin for text in @text_view.
 *
 * Note that this function is confusingly named.
 * In CSS terms, the value set here is padding.
 *
 * Since: 3.18
 */
void
ctk_text_view_set_top_margin (CtkTextView *text_view,
                              gint         top_margin)
{
  CtkTextViewPrivate *priv = text_view->priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  if (priv->top_margin != top_margin)
    {
      priv->yoffset += priv->top_margin - top_margin;

      priv->top_margin = top_margin;
      priv->top_border = top_margin + priv->top_padding;

      if (priv->layout && priv->layout->default_style)
        ctk_text_layout_default_style_changed (priv->layout);

      ctk_text_view_invalidate (text_view);

      g_object_notify (G_OBJECT (text_view), "top-margin");
    }
}

/**
 * ctk_text_view_get_top_margin:
 * @text_view: a #CtkTextView
 *
 * Gets the top margin for text in the @text_view.
 *
 * Returns: top margin in pixels
 *
 * Since: 3.18
 **/
gint
ctk_text_view_get_top_margin (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), 0);

  return text_view->priv->top_margin;
}

/**
 * ctk_text_view_set_bottom_margin:
 * @text_view: a #CtkTextView
 * @bottom_margin: bottom margin in pixels
 *
 * Sets the bottom margin for text in @text_view.
 *
 * Note that this function is confusingly named.
 * In CSS terms, the value set here is padding.
 *
 * Since: 3.18
 */
void
ctk_text_view_set_bottom_margin (CtkTextView *text_view,
                                 gint         bottom_margin)
{
  CtkTextViewPrivate *priv = text_view->priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  if (priv->bottom_margin != bottom_margin)
    {
      priv->bottom_margin = bottom_margin;
      priv->bottom_border = bottom_margin + priv->bottom_padding;

      if (priv->layout && priv->layout->default_style)
        ctk_text_layout_default_style_changed (priv->layout);

      g_object_notify (G_OBJECT (text_view), "bottom-margin");
    }
}

/**
 * ctk_text_view_get_bottom_margin:
 * @text_view: a #CtkTextView
 *
 * Gets the bottom margin for text in the @text_view.
 *
 * Returns: bottom margin in pixels
 *
 * Since: 3.18
 */
gint
ctk_text_view_get_bottom_margin (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), 0);

  return text_view->priv->bottom_margin;
}

/**
 * ctk_text_view_set_indent:
 * @text_view: a #CtkTextView
 * @indent: indentation in pixels
 *
 * Sets the default indentation for paragraphs in @text_view.
 * Tags in the buffer may override the default.
 **/
void
ctk_text_view_set_indent (CtkTextView *text_view,
                          gint         indent)
{
  CtkTextViewPrivate *priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  priv = text_view->priv;

  if (priv->indent != indent)
    {
      priv->indent = indent;

      if (priv->layout && priv->layout->default_style)
        {
          priv->layout->default_style->indent = indent;
          ctk_text_layout_default_style_changed (priv->layout);
        }

      g_object_notify (G_OBJECT (text_view), "indent");
    }
}

/**
 * ctk_text_view_get_indent:
 * @text_view: a #CtkTextView
 * 
 * Gets the default indentation of paragraphs in @text_view.
 * Tags in the view’s buffer may override the default.
 * The indentation may be negative.
 * 
 * Returns: number of pixels of indentation
 **/
gint
ctk_text_view_get_indent (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), 0);

  return text_view->priv->indent;
}

/**
 * ctk_text_view_set_tabs:
 * @text_view: a #CtkTextView
 * @tabs: tabs as a #PangoTabArray
 *
 * Sets the default tab stops for paragraphs in @text_view.
 * Tags in the buffer may override the default.
 **/
void
ctk_text_view_set_tabs (CtkTextView   *text_view,
                        PangoTabArray *tabs)
{
  CtkTextViewPrivate *priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  priv = text_view->priv;

  if (priv->tabs)
    pango_tab_array_free (priv->tabs);

  priv->tabs = tabs ? pango_tab_array_copy (tabs) : NULL;

  if (priv->layout && priv->layout->default_style)
    {
      /* some unkosher futzing in internal struct details... */
      if (priv->layout->default_style->tabs)
        pango_tab_array_free (priv->layout->default_style->tabs);

      priv->layout->default_style->tabs =
        priv->tabs ? pango_tab_array_copy (priv->tabs) : NULL;

      ctk_text_layout_default_style_changed (priv->layout);
    }

  g_object_notify (G_OBJECT (text_view), "tabs");
}

/**
 * ctk_text_view_get_tabs:
 * @text_view: a #CtkTextView
 * 
 * Gets the default tabs for @text_view. Tags in the buffer may
 * override the defaults. The returned array will be %NULL if
 * “standard” (8-space) tabs are used. Free the return value
 * with pango_tab_array_free().
 * 
 * Returns: (nullable) (transfer full): copy of default tab array, or %NULL if
 *    “standard" tabs are used; must be freed with pango_tab_array_free().
 **/
PangoTabArray*
ctk_text_view_get_tabs (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), NULL);

  return text_view->priv->tabs ? pango_tab_array_copy (text_view->priv->tabs) : NULL;
}

static void
ctk_text_view_toggle_cursor_visible (CtkTextView *text_view)
{
  ctk_text_view_set_cursor_visible (text_view, !text_view->priv->cursor_visible);
}

/**
 * ctk_text_view_set_cursor_visible:
 * @text_view: a #CtkTextView
 * @setting: whether to show the insertion cursor
 *
 * Toggles whether the insertion point should be displayed. A buffer with
 * no editable text probably shouldn’t have a visible cursor, so you may
 * want to turn the cursor off.
 *
 * Note that this property may be overridden by the
 * #CtkSettings:ctk-keynave-use-caret settings.
 */
void
ctk_text_view_set_cursor_visible (CtkTextView *text_view,
				  gboolean     setting)
{
  CtkTextViewPrivate *priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  priv = text_view->priv;
  setting = (setting != FALSE);

  if (priv->cursor_visible != setting)
    {
      priv->cursor_visible = setting;

      if (ctk_widget_has_focus (CTK_WIDGET (text_view)))
        {
          if (priv->layout)
            {
              ctk_text_layout_set_cursor_visible (priv->layout, setting);
	      ctk_text_view_check_cursor_blink (text_view);
            }
        }

      g_object_notify (G_OBJECT (text_view), "cursor-visible");
    }
}

/**
 * ctk_text_view_get_cursor_visible:
 * @text_view: a #CtkTextView
 *
 * Find out whether the cursor should be displayed.
 *
 * Returns: whether the insertion mark is visible
 */
gboolean
ctk_text_view_get_cursor_visible (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);

  return text_view->priv->cursor_visible;
}

/**
 * ctk_text_view_reset_cursor_blink:
 * @text_view: a #CtkTextView
 *
 * Ensures that the cursor is shown (i.e. not in an 'off' blink
 * interval) and resets the time that it will stay blinking (or
 * visible, in case blinking is disabled).
 *
 * This function should be called in response to user input
 * (e.g. from derived classes that override the textview's
 * #CtkWidget::key-press-event handler).
 *
 * Since: 3.20
 */
void
ctk_text_view_reset_cursor_blink (CtkTextView *text_view)
{
  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  ctk_text_view_reset_blink_time (text_view);
  ctk_text_view_pend_cursor_blink (text_view);
}

/**
 * ctk_text_view_place_cursor_onscreen:
 * @text_view: a #CtkTextView
 *
 * Moves the cursor to the currently visible region of the
 * buffer, it it isn’t there already.
 *
 * Returns: %TRUE if the cursor had to be moved.
 **/
gboolean
ctk_text_view_place_cursor_onscreen (CtkTextView *text_view)
{
  CtkTextIter insert;

  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);

  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &insert,
                                    ctk_text_buffer_get_insert (get_buffer (text_view)));

  if (clamp_iter_onscreen (text_view, &insert))
    {
      ctk_text_buffer_place_cursor (get_buffer (text_view), &insert);
      return TRUE;
    }
  else
    return FALSE;
}

static void
ctk_text_view_remove_validate_idles (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv = text_view->priv;

  if (priv->first_validate_idle != 0)
    {
      DV (g_print ("Removing first validate idle: %s\n", G_STRLOC));
      g_source_remove (priv->first_validate_idle);
      priv->first_validate_idle = 0;
    }

  if (priv->incremental_validate_idle != 0)
    {
      g_source_remove (priv->incremental_validate_idle);
      priv->incremental_validate_idle = 0;
    }
}

static void
ctk_text_view_destroy (CtkWidget *widget)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;

  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  ctk_text_view_remove_validate_idles (text_view);
  ctk_text_view_set_buffer (text_view, NULL);
  ctk_text_view_destroy_layout (text_view);

  if (text_view->priv->scroll_timeout)
    {
      g_source_remove (text_view->priv->scroll_timeout);
      text_view->priv->scroll_timeout = 0;
    }

  if (priv->im_spot_idle)
    {
      g_source_remove (priv->im_spot_idle);
      priv->im_spot_idle = 0;
    }

  if (priv->pixel_cache)
    {
      _ctk_pixel_cache_free (priv->pixel_cache);
      priv->pixel_cache = NULL;
    }

  if (priv->magnifier)
    _ctk_magnifier_set_inspected (CTK_MAGNIFIER (priv->magnifier), NULL);

  CTK_WIDGET_CLASS (ctk_text_view_parent_class)->destroy (widget);
}

static void
ctk_text_view_finalize (GObject *object)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;

  text_view = CTK_TEXT_VIEW (object);
  priv = text_view->priv;

  ctk_text_view_destroy_layout (text_view);
  ctk_text_view_set_buffer (text_view, NULL);

  /* at this point, no "notify::buffer" handler should recreate the buffer. */
  g_assert (priv->buffer == NULL);
  
  cancel_pending_scroll (text_view);

  g_object_unref (priv->multipress_gesture);
  g_object_unref (priv->drag_gesture);

  if (priv->tabs)
    pango_tab_array_free (priv->tabs);
  
  if (priv->hadjustment)
    g_object_unref (priv->hadjustment);
  if (priv->vadjustment)
    g_object_unref (priv->vadjustment);

  text_window_free (priv->text_window);

  if (priv->left_window)
    text_window_free (priv->left_window);

  if (priv->top_window)
    text_window_free (priv->top_window);

  if (priv->right_window)
    text_window_free (priv->right_window);

  if (priv->bottom_window)
    text_window_free (priv->bottom_window);

  if (priv->selection_bubble)
    ctk_widget_destroy (priv->selection_bubble);

  if (priv->magnifier_popover)
    ctk_widget_destroy (priv->magnifier_popover);
  if (priv->text_handle)
    g_object_unref (priv->text_handle);
  g_object_unref (priv->im_context);

  g_free (priv->im_module);

  G_OBJECT_CLASS (ctk_text_view_parent_class)->finalize (object);
}

static void
ctk_text_view_set_property (GObject         *object,
			    guint            prop_id,
			    const GValue    *value,
			    GParamSpec      *pspec)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;

  text_view = CTK_TEXT_VIEW (object);
  priv = text_view->priv;

  switch (prop_id)
    {
    case PROP_PIXELS_ABOVE_LINES:
      ctk_text_view_set_pixels_above_lines (text_view, g_value_get_int (value));
      break;

    case PROP_PIXELS_BELOW_LINES:
      ctk_text_view_set_pixels_below_lines (text_view, g_value_get_int (value));
      break;

    case PROP_PIXELS_INSIDE_WRAP:
      ctk_text_view_set_pixels_inside_wrap (text_view, g_value_get_int (value));
      break;

    case PROP_EDITABLE:
      ctk_text_view_set_editable (text_view, g_value_get_boolean (value));
      break;

    case PROP_WRAP_MODE:
      ctk_text_view_set_wrap_mode (text_view, g_value_get_enum (value));
      break;
      
    case PROP_JUSTIFICATION:
      ctk_text_view_set_justification (text_view, g_value_get_enum (value));
      break;

    case PROP_LEFT_MARGIN:
      ctk_text_view_set_left_margin (text_view, g_value_get_int (value));
      break;

    case PROP_RIGHT_MARGIN:
      ctk_text_view_set_right_margin (text_view, g_value_get_int (value));
      break;

    case PROP_TOP_MARGIN:
      ctk_text_view_set_top_margin (text_view, g_value_get_int (value));
      break;

    case PROP_BOTTOM_MARGIN:
      ctk_text_view_set_bottom_margin (text_view, g_value_get_int (value));
      break;

    case PROP_INDENT:
      ctk_text_view_set_indent (text_view, g_value_get_int (value));
      break;

    case PROP_TABS:
      ctk_text_view_set_tabs (text_view, g_value_get_boxed (value));
      break;

    case PROP_CURSOR_VISIBLE:
      ctk_text_view_set_cursor_visible (text_view, g_value_get_boolean (value));
      break;

    case PROP_OVERWRITE:
      ctk_text_view_set_overwrite (text_view, g_value_get_boolean (value));
      break;

    case PROP_BUFFER:
      ctk_text_view_set_buffer (text_view, CTK_TEXT_BUFFER (g_value_get_object (value)));
      break;

    case PROP_ACCEPTS_TAB:
      ctk_text_view_set_accepts_tab (text_view, g_value_get_boolean (value));
      break;
      
    case PROP_IM_MODULE:
      g_free (priv->im_module);
      priv->im_module = g_value_dup_string (value);
      if (CTK_IS_IM_MULTICONTEXT (priv->im_context))
        ctk_im_multicontext_set_context_id (CTK_IM_MULTICONTEXT (priv->im_context), priv->im_module);
      break;

    case PROP_HADJUSTMENT:
      ctk_text_view_set_hadjustment (text_view, g_value_get_object (value));
      break;

    case PROP_VADJUSTMENT:
      ctk_text_view_set_vadjustment (text_view, g_value_get_object (value));
      break;

    case PROP_HSCROLL_POLICY:
      if (priv->hscroll_policy != g_value_get_enum (value))
        {
          priv->hscroll_policy = g_value_get_enum (value);
          ctk_widget_queue_resize (CTK_WIDGET (text_view));
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_VSCROLL_POLICY:
      if (priv->vscroll_policy != g_value_get_enum (value))
        {
          priv->vscroll_policy = g_value_get_enum (value);
          ctk_widget_queue_resize (CTK_WIDGET (text_view));
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_INPUT_PURPOSE:
      ctk_text_view_set_input_purpose (text_view, g_value_get_enum (value));
      break;

    case PROP_INPUT_HINTS:
      ctk_text_view_set_input_hints (text_view, g_value_get_flags (value));
      break;

    case PROP_POPULATE_ALL:
      if (text_view->priv->populate_all != g_value_get_boolean (value))
        {
          text_view->priv->populate_all = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_MONOSPACE:
      ctk_text_view_set_monospace (text_view, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_text_view_get_property (GObject         *object,
			    guint            prop_id,
			    GValue          *value,
			    GParamSpec      *pspec)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;

  text_view = CTK_TEXT_VIEW (object);
  priv = text_view->priv;

  switch (prop_id)
    {
    case PROP_PIXELS_ABOVE_LINES:
      g_value_set_int (value, priv->pixels_above_lines);
      break;

    case PROP_PIXELS_BELOW_LINES:
      g_value_set_int (value, priv->pixels_below_lines);
      break;

    case PROP_PIXELS_INSIDE_WRAP:
      g_value_set_int (value, priv->pixels_inside_wrap);
      break;

    case PROP_EDITABLE:
      g_value_set_boolean (value, priv->editable);
      break;
      
    case PROP_WRAP_MODE:
      g_value_set_enum (value, priv->wrap_mode);
      break;

    case PROP_JUSTIFICATION:
      g_value_set_enum (value, priv->justify);
      break;

    case PROP_LEFT_MARGIN:
      g_value_set_int (value, priv->left_margin);
      break;

    case PROP_RIGHT_MARGIN:
      g_value_set_int (value, priv->right_margin);
      break;

    case PROP_TOP_MARGIN:
      g_value_set_int (value, priv->top_margin);
      break;

    case PROP_BOTTOM_MARGIN:
      g_value_set_int (value, priv->bottom_margin);
      break;

    case PROP_INDENT:
      g_value_set_int (value, priv->indent);
      break;

    case PROP_TABS:
      g_value_set_boxed (value, priv->tabs);
      break;

    case PROP_CURSOR_VISIBLE:
      g_value_set_boolean (value, priv->cursor_visible);
      break;

    case PROP_BUFFER:
      g_value_set_object (value, get_buffer (text_view));
      break;

    case PROP_OVERWRITE:
      g_value_set_boolean (value, priv->overwrite_mode);
      break;

    case PROP_ACCEPTS_TAB:
      g_value_set_boolean (value, priv->accepts_tab);
      break;
      
    case PROP_IM_MODULE:
      g_value_set_string (value, priv->im_module);
      break;

    case PROP_HADJUSTMENT:
      g_value_set_object (value, priv->hadjustment);
      break;

    case PROP_VADJUSTMENT:
      g_value_set_object (value, priv->vadjustment);
      break;

    case PROP_HSCROLL_POLICY:
      g_value_set_enum (value, priv->hscroll_policy);
      break;

    case PROP_VSCROLL_POLICY:
      g_value_set_enum (value, priv->vscroll_policy);
      break;

    case PROP_INPUT_PURPOSE:
      g_value_set_enum (value, ctk_text_view_get_input_purpose (text_view));
      break;

    case PROP_INPUT_HINTS:
      g_value_set_flags (value, ctk_text_view_get_input_hints (text_view));
      break;

    case PROP_POPULATE_ALL:
      g_value_set_boolean (value, priv->populate_all);
      break;

    case PROP_MONOSPACE:
      g_value_set_boolean (value, ctk_text_view_get_monospace (text_view));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_text_view_size_request (CtkWidget      *widget,
                            CtkRequisition *requisition)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  GSList *tmp_list;
  guint border_width;

  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  if (priv->layout)
    {
      priv->text_window->requisition.width = priv->layout->width;
      priv->text_window->requisition.height = priv->layout->height;
    }
  else
    {
      priv->text_window->requisition.width = 0;
      priv->text_window->requisition.height = 0;
    }
  
  requisition->width = priv->text_window->requisition.width;
  requisition->height = priv->text_window->requisition.height;

  if (priv->left_window)
    requisition->width += priv->left_window->requisition.width;

  if (priv->right_window)
    requisition->width += priv->right_window->requisition.width;

  if (priv->top_window)
    requisition->height += priv->top_window->requisition.height;

  if (priv->bottom_window)
    requisition->height += priv->bottom_window->requisition.height;

  border_width = ctk_container_get_border_width (CTK_CONTAINER (text_view));
  requisition->width += border_width * 2;
  requisition->height += border_width * 2;

  requisition->height += priv->top_border + priv->bottom_border;

  tmp_list = priv->children;
  while (tmp_list != NULL)
    {
      CtkTextViewChild *child = tmp_list->data;

      if (child->anchor)
        {
          CtkRequisition child_req;
          CtkRequisition old_req;

          ctk_widget_get_preferred_size (child->widget, &old_req, NULL);

          ctk_widget_get_preferred_size (child->widget, &child_req, NULL);

          /* Invalidate layout lines if required */
          if (priv->layout &&
              (old_req.width != child_req.width ||
               old_req.height != child_req.height))
            ctk_text_child_anchor_queue_resize (child->anchor,
                                                priv->layout);
        }
      else
        {
          CtkRequisition child_req;

          ctk_widget_get_preferred_size (child->widget,
                                         &child_req, NULL);
        }

      tmp_list = tmp_list->next;
    }

  /* Cache the requested size of the text view so we can 
   * compare it in the changed_handler() */
  priv->cached_size_request = *requisition;
}

static void
ctk_text_view_get_preferred_width (CtkWidget *widget,
				   gint      *minimum,
				   gint      *natural)
{
  CtkRequisition requisition;

  ctk_text_view_size_request (widget, &requisition);

  *minimum = *natural = requisition.width;
}

static void
ctk_text_view_get_preferred_height (CtkWidget *widget,
				    gint      *minimum,
				    gint      *natural)
{
  CtkRequisition requisition;

  ctk_text_view_size_request (widget, &requisition);

  *minimum = *natural = requisition.height;
}


static void
ctk_text_view_compute_child_allocation (CtkTextView      *text_view,
                                        CtkTextViewChild *vc,
                                        CtkAllocation    *allocation)
{
  gint buffer_y;
  CtkTextIter iter;
  CtkRequisition req;
  
  ctk_text_buffer_get_iter_at_child_anchor (get_buffer (text_view),
                                            &iter,
                                            vc->anchor);

  ctk_text_layout_get_line_yrange (text_view->priv->layout, &iter,
                                   &buffer_y, NULL);

  buffer_y += vc->from_top_of_line;

  allocation->x = vc->from_left_of_buffer - text_view->priv->xoffset;
  allocation->y = buffer_y - text_view->priv->yoffset;

  ctk_widget_get_preferred_size (vc->widget, &req, NULL);
  allocation->width = req.width;
  allocation->height = req.height;
}

static void
ctk_text_view_update_child_allocation (CtkTextView      *text_view,
                                       CtkTextViewChild *vc)
{
  CtkAllocation allocation;

  ctk_text_view_compute_child_allocation (text_view, vc, &allocation);
  
  ctk_widget_size_allocate (vc->widget, &allocation);

#if 0
  g_print ("allocation for %p allocated to %d,%d yoffset = %d\n",
           vc->widget,
           vc->widget->allocation.x,
           vc->widget->allocation.y,
           text_view->priv->yoffset);
#endif
}

static void
ctk_text_view_child_allocated (CtkTextLayout *layout,
                               CtkWidget     *child,
                               gint           x,
                               gint           y,
                               gpointer       data)
{
  CtkTextViewChild *vc = NULL;
  CtkTextView *text_view = data;
  
  /* x,y is the position of the child from the top of the line, and
   * from the left of the buffer. We have to translate that into text
   * window coordinates, then size_allocate the child.
   */

  vc = g_object_get_qdata (G_OBJECT (child), quark_text_view_child);

  g_assert (vc != NULL);

  DV (g_print ("child allocated at %d,%d\n", x, y));
  
  vc->from_left_of_buffer = x;
  vc->from_top_of_line = y;

  ctk_text_view_update_child_allocation (text_view, vc);
}

static void
ctk_text_view_allocate_children (CtkTextView *text_view)
{
  GSList *tmp_list;

  DV(g_print(G_STRLOC"\n"));
  
  tmp_list = text_view->priv->children;
  while (tmp_list != NULL)
    {
      CtkTextViewChild *child = tmp_list->data;

      g_assert (child != NULL);
          
      if (child->anchor)
        {
          /* We need to force-validate the regions containing
           * children.
           */
          CtkTextIter child_loc;
          ctk_text_buffer_get_iter_at_child_anchor (get_buffer (text_view),
                                                    &child_loc,
                                                    child->anchor);

	  /* Since anchored children are only ever allocated from
           * ctk_text_layout_get_line_display() we have to make sure
	   * that the display line caching in the layout doesn't 
           * get in the way. Invalidating the layout around the anchor
           * achieves this.
	   */ 
	  if (_ctk_widget_get_alloc_needed (child->widget))
	    {
	      CtkTextIter end = child_loc;
	      ctk_text_iter_forward_char (&end);
	      ctk_text_layout_invalidate (text_view->priv->layout, &child_loc, &end);
	    }

          ctk_text_layout_validate_yrange (text_view->priv->layout,
                                           &child_loc,
                                           0, 1);
        }
      else
        {
          CtkAllocation allocation;
          CtkRequisition child_req;
             
          allocation.x = child->x;
          allocation.y = child->y;

          if (child->type == CTK_TEXT_WINDOW_TEXT ||
              child->type == CTK_TEXT_WINDOW_LEFT ||
              child->type == CTK_TEXT_WINDOW_RIGHT)
            allocation.y -= text_view->priv->yoffset;
          if (child->type == CTK_TEXT_WINDOW_TEXT ||
              child->type == CTK_TEXT_WINDOW_TOP ||
              child->type == CTK_TEXT_WINDOW_BOTTOM)
            allocation.x -= text_view->priv->xoffset;

          ctk_widget_get_preferred_size (child->widget, &child_req, NULL);

          allocation.width = child_req.width;
          allocation.height = child_req.height;
          
          ctk_widget_size_allocate (child->widget, &allocation);          
        }

      tmp_list = tmp_list->next;
    }
}

static void
ctk_text_view_size_allocate (CtkWidget *widget,
                             CtkAllocation *allocation)
{
  CtkAllocation widget_allocation;
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  gint width, height;
  CdkRectangle text_rect;
  CdkRectangle left_rect;
  CdkRectangle right_rect;
  CdkRectangle top_rect;
  CdkRectangle bottom_rect;
  guint border_width;
  gboolean size_changed;
  
  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  DV(g_print(G_STRLOC"\n"));

  _ctk_pixel_cache_set_extra_size (priv->pixel_cache, 64,
                                   allocation->height / 2 + priv->top_border);

  ctk_widget_get_allocation (widget, &widget_allocation);
  size_changed =
    widget_allocation.width != allocation->width ||
    widget_allocation.height != allocation->height;

  border_width = ctk_container_get_border_width (CTK_CONTAINER (text_view));

  ctk_widget_set_allocation (widget, allocation);

  if (ctk_widget_get_realized (widget))
    {
      cdk_window_move_resize (ctk_widget_get_window (widget),
                              allocation->x, allocation->y,
                              allocation->width, allocation->height);
    }

  /* distribute width/height among child windows. Ensure all
   * windows get at least a 1x1 allocation.
   */

  width = allocation->width - border_width * 2;

  if (priv->left_window)
    left_rect.width = priv->left_window->requisition.width;
  else
    left_rect.width = 0;

  width -= left_rect.width;

  if (priv->right_window)
    right_rect.width = priv->right_window->requisition.width;
  else
    right_rect.width = 0;

  width -= right_rect.width;

  text_rect.width = MAX (1, width);

  top_rect.width = text_rect.width;
  bottom_rect.width = text_rect.width;

  height = allocation->height - border_width * 2;

  if (priv->top_window)
    top_rect.height = priv->top_window->requisition.height;
  else
    top_rect.height = 0;

  height -= top_rect.height;

  if (priv->bottom_window)
    bottom_rect.height = priv->bottom_window->requisition.height;
  else
    bottom_rect.height = 0;

  height -= bottom_rect.height;

  text_rect.height = MAX (1, height);

  left_rect.height = text_rect.height;
  right_rect.height = text_rect.height;

  /* Origins */
  left_rect.x = border_width;
  top_rect.y = border_width;

  text_rect.x = left_rect.x + left_rect.width;
  text_rect.y = top_rect.y + top_rect.height;

  left_rect.y = text_rect.y;
  right_rect.y = text_rect.y;

  top_rect.x = text_rect.x;
  bottom_rect.x = text_rect.x;

  right_rect.x = text_rect.x + text_rect.width;
  bottom_rect.y = text_rect.y + text_rect.height;

  text_window_size_allocate (priv->text_window,
                             &text_rect);

  if (priv->left_window)
    text_window_size_allocate (priv->left_window,
                               &left_rect);

  if (priv->right_window)
    text_window_size_allocate (priv->right_window,
                               &right_rect);

  if (priv->top_window)
    text_window_size_allocate (priv->top_window,
                               &top_rect);

  if (priv->bottom_window)
    text_window_size_allocate (priv->bottom_window,
                               &bottom_rect);

  ctk_text_view_update_layout_width (text_view);
  
  /* Note that this will do some layout validation */
  ctk_text_view_allocate_children (text_view);

  /* Update adjustments */
  if (!ctk_adjustment_is_animating (priv->hadjustment))
    ctk_text_view_set_hadjustment_values (text_view);
  if (!ctk_adjustment_is_animating (priv->vadjustment))
    ctk_text_view_set_vadjustment_values (text_view);

  /* The CTK resize loop processes all the pending exposes right
   * after doing the resize stuff, so the idle sizer won't have a
   * chance to run. So we do the work here. 
   */
  ctk_text_view_flush_first_validate (text_view);

  /* widget->window doesn't get auto-redrawn as the layout is computed, so has to
   * be invalidated
   */
  if (size_changed && ctk_widget_get_realized (widget))
    cdk_window_invalidate_rect (ctk_widget_get_window (widget), NULL, FALSE);
}

static void
ctk_text_view_get_first_para_iter (CtkTextView *text_view,
                                   CtkTextIter *iter)
{
  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), iter,
                                    text_view->priv->first_para_mark);
}

static void
ctk_text_view_validate_onscreen (CtkTextView *text_view)
{
  CtkWidget *widget;
  CtkTextViewPrivate *priv;

  widget = CTK_WIDGET (text_view);
  priv = text_view->priv;
  
  DV(g_print(">Validating onscreen ("G_STRLOC")\n"));
  
  if (SCREEN_HEIGHT (widget) > 0)
    {
      CtkTextIter first_para;

      /* Be sure we've validated the stuff onscreen; if we
       * scrolled, these calls won't have any effect, because
       * they were called in the recursive validate_onscreen
       */
      ctk_text_view_get_first_para_iter (text_view, &first_para);

      ctk_text_layout_validate_yrange (priv->layout,
                                       &first_para,
                                       0,
                                       priv->first_para_pixels +
                                       SCREEN_HEIGHT (widget));
    }

  priv->onscreen_validated = TRUE;

  DV(g_print(">Done validating onscreen, onscreen_validated = TRUE ("G_STRLOC")\n"));
  
  /* This can have the odd side effect of triggering a scroll, which should
   * flip "onscreen_validated" back to FALSE, but should also get us
   * back into this function to turn it on again.
   */
  ctk_text_view_update_adjustments (text_view);

  g_assert (priv->onscreen_validated);
}

static void
ctk_text_view_flush_first_validate (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv = text_view->priv;

  if (priv->first_validate_idle == 0)
    return;

  /* Do this first, which means that if an "invalidate"
   * occurs during any of this process, a new first_validate_callback
   * will be installed, and we'll start again.
   */
  DV (g_print ("removing first validate in %s\n", G_STRLOC));
  g_source_remove (priv->first_validate_idle);
  priv->first_validate_idle = 0;
  
  /* be sure we have up-to-date screen size set on the
   * layout.
   */
  ctk_text_view_update_layout_width (text_view);

  /* Bail out if we invalidated stuff; scrolling right away will just
   * confuse the issue.
   */
  if (priv->first_validate_idle != 0)
    {
      DV(g_print(">Width change forced requeue ("G_STRLOC")\n"));
    }
  else
    {
      /* scroll to any marks, if that's pending. This can jump us to
       * the validation codepath used for scrolling onscreen, if so we
       * bail out.  It won't jump if already in that codepath since
       * value_changed is not recursive, so also validate if
       * necessary.
       */
      if (!ctk_text_view_flush_scroll (text_view) ||
          !priv->onscreen_validated)
	ctk_text_view_validate_onscreen (text_view);
      
      DV(g_print(">Leaving first validate idle ("G_STRLOC")\n"));
      
      g_assert (priv->onscreen_validated);
    }
}

static gboolean
first_validate_callback (gpointer data)
{
  CtkTextView *text_view = data;

  /* Note that some of this code is duplicated at the end of size_allocate,
   * keep in sync with that.
   */
  
  DV(g_print(G_STRLOC"\n"));

  ctk_text_view_flush_first_validate (text_view);
  
  return FALSE;
}

static gboolean
incremental_validate_callback (gpointer data)
{
  CtkTextView *text_view = data;
  gboolean result = TRUE;

  DV(g_print(G_STRLOC"\n"));
  
  ctk_text_layout_validate (text_view->priv->layout, 2000);

  ctk_text_view_update_adjustments (text_view);
  
  if (ctk_text_layout_is_valid (text_view->priv->layout))
    {
      text_view->priv->incremental_validate_idle = 0;
      result = FALSE;
    }

  return result;
}

static void
ctk_text_view_invalidate (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv = text_view->priv;

  DV (g_print (">Invalidate, onscreen_validated = %d now FALSE ("G_STRLOC")\n",
               priv->onscreen_validated));

  priv->onscreen_validated = FALSE;

  /* We'll invalidate when the layout is created */
  if (priv->layout == NULL)
    return;
  
  if (!priv->first_validate_idle)
    {
      priv->first_validate_idle = cdk_threads_add_idle_full (CTK_PRIORITY_RESIZE - 2, first_validate_callback, text_view, NULL);
      g_source_set_name_by_id (priv->first_validate_idle, "[ctk+] first_validate_callback");
      DV (g_print (G_STRLOC": adding first validate idle %d\n",
                   priv->first_validate_idle));
    }
      
  if (!priv->incremental_validate_idle)
    {
      priv->incremental_validate_idle = cdk_threads_add_idle_full (CTK_TEXT_VIEW_PRIORITY_VALIDATE, incremental_validate_callback, text_view, NULL);
      g_source_set_name_by_id (priv->incremental_validate_idle, "[ctk+] incremental_validate_callback");
      DV (g_print (G_STRLOC": adding incremental validate idle %d\n",
                   priv->incremental_validate_idle));
    }
}

static void
invalidated_handler (CtkTextLayout *layout,
                     gpointer       data)
{
  CtkTextView *text_view;

  text_view = CTK_TEXT_VIEW (data);

  DV (g_print ("Invalidating due to layout invalidate signal\n"));
  ctk_text_view_invalidate (text_view);
}

static void
changed_handler (CtkTextLayout     *layout,
                 gint               start_y,
                 gint               old_height,
                 gint               new_height,
                 gpointer           data)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  CtkWidget *widget;
  CdkRectangle visible_rect;
  CdkRectangle redraw_rect;
  
  text_view = CTK_TEXT_VIEW (data);
  priv = text_view->priv;
  widget = CTK_WIDGET (data);
  
  DV(g_print(">Lines Validated ("G_STRLOC")\n"));

  if (ctk_widget_get_realized (widget))
    {      
      ctk_text_view_get_rendered_rect (text_view, &visible_rect);

      redraw_rect.x = visible_rect.x;
      redraw_rect.width = visible_rect.width;
      redraw_rect.y = start_y;

      if (old_height == new_height)
        redraw_rect.height = old_height;
      else if (start_y + old_height > visible_rect.y)
        redraw_rect.height = MAX (0, visible_rect.y + visible_rect.height - start_y);
      else
        redraw_rect.height = 0;
	
      if (cdk_rectangle_intersect (&redraw_rect, &visible_rect, &redraw_rect))
        {
          /* text_window_invalidate_rect() takes buffer coordinates */
          text_window_invalidate_rect (priv->text_window,
                                       &redraw_rect);

          DV(g_print(" invalidated rect: %d,%d %d x %d\n",
                     redraw_rect.x,
                     redraw_rect.y,
                     redraw_rect.width,
                     redraw_rect.height));
          
          if (priv->left_window)
            text_window_invalidate_rect (priv->left_window,
                                         &redraw_rect);
          if (priv->right_window)
            text_window_invalidate_rect (priv->right_window,
                                         &redraw_rect);
          if (priv->top_window)
            text_window_invalidate_rect (priv->top_window,
                                         &redraw_rect);
          if (priv->bottom_window)
            text_window_invalidate_rect (priv->bottom_window,
                                         &redraw_rect);

          queue_update_im_spot_location (text_view);
        }
    }
  
  if (old_height != new_height)
    {
      GSList *tmp_list;
      int new_first_para_top;
      int old_first_para_top;
      CtkTextIter first;
      
      /* If the bottom of the old area was above the top of the
       * screen, we need to scroll to keep the current top of the
       * screen in place.  Remember that first_para_pixels is the
       * position of the top of the screen in coordinates relative to
       * the first paragraph onscreen.
       *
       * In short we are adding the height delta of the portion of the
       * changed region above first_para_mark to priv->yoffset.
       */
      ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &first,
                                        priv->first_para_mark);

      ctk_text_layout_get_line_yrange (layout, &first, &new_first_para_top, NULL);

      old_first_para_top = priv->yoffset - priv->first_para_pixels + priv->top_border;

      if (new_first_para_top != old_first_para_top)
        {
          priv->yoffset += new_first_para_top - old_first_para_top;
          
          ctk_adjustment_set_value (text_view->priv->vadjustment, priv->yoffset);

          /* If the height changed above our current position, then we
           * need to discard the pixelcache because things wont line nup
           * anymore (even if we adjust the vadjustment).
           *
           * Generally this doesn't happen interactively because we keep
           * the insert cursor on screen when making changes. It can
           * happen when code changes the first line, for example, in an
           * automated fashion.
           *
           * There is one case where this could be optimized out such as
           * when delete-range is followed by insert-text and whole lines
           * are removed. API consumers can always work around that by
           * avoiding the removal of a \n so no effort is made here to
           * handle that.
           */
          if (ctk_widget_get_realized (widget))
            _ctk_pixel_cache_invalidate (text_view->priv->pixel_cache, NULL);
        }

      /* FIXME be smarter about which anchored widgets we update */

      tmp_list = priv->children;
      while (tmp_list != NULL)
        {
          CtkTextViewChild *child = tmp_list->data;

          if (child->anchor)
            ctk_text_view_update_child_allocation (text_view, child);

          tmp_list = tmp_list->next;
        }
    }

  {
    CtkRequisition old_req = priv->cached_size_request;
    CtkRequisition new_req;

    /* Use this instead of ctk_widget_size_request wrapper
     * to avoid the optimization which just returns widget->requisition
     * if a resize hasn't been queued.
     */
    ctk_text_view_size_request (widget, &new_req);

    if (old_req.width != new_req.width ||
        old_req.height != new_req.height)
      {
	ctk_widget_queue_resize_no_redraw (widget);
      }
  }
}

static void
ctk_text_view_realize (CtkWidget *widget)
{
  CtkAllocation allocation;
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  CdkWindow *window;
  CdkWindowAttr attributes;
  gint attributes_mask;
  GSList *tmp_list;

  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  ctk_widget_set_realized (widget, TRUE);

  ctk_widget_get_allocation (widget, &allocation);

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

  text_window_realize (priv->text_window, widget);

  if (priv->left_window)
    text_window_realize (priv->left_window, widget);

  if (priv->top_window)
    text_window_realize (priv->top_window, widget);

  if (priv->right_window)
    text_window_realize (priv->right_window, widget);

  if (priv->bottom_window)
    text_window_realize (priv->bottom_window, widget);

  ctk_text_view_ensure_layout (text_view);
  ctk_text_view_invalidate (text_view);

  if (priv->buffer)
    {
      CtkClipboard *clipboard = ctk_widget_get_clipboard (CTK_WIDGET (text_view),
							  CDK_SELECTION_PRIMARY);
      ctk_text_buffer_add_selection_clipboard (priv->buffer, clipboard);
    }

  tmp_list = priv->children;
  while (tmp_list != NULL)
    {
      CtkTextViewChild *vc = tmp_list->data;
      
      text_view_child_set_parent_window (text_view, vc);
      
      tmp_list = tmp_list->next;
    }

  /* Ensure updating the spot location. */
  ctk_text_view_update_im_spot_location (text_view);
}

static void
ctk_text_view_unrealize (CtkWidget *widget)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;

  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  if (priv->buffer)
    {
      CtkClipboard *clipboard = ctk_widget_get_clipboard (CTK_WIDGET (text_view),
							  CDK_SELECTION_PRIMARY);
      ctk_text_buffer_remove_selection_clipboard (priv->buffer, clipboard);
    }

  ctk_text_view_remove_validate_idles (text_view);

  if (priv->popup_menu)
    {
      ctk_widget_destroy (priv->popup_menu);
      priv->popup_menu = NULL;
    }

  text_window_unrealize (priv->text_window);

  if (priv->left_window)
    text_window_unrealize (priv->left_window);

  if (priv->top_window)
    text_window_unrealize (priv->top_window);

  if (priv->right_window)
    text_window_unrealize (priv->right_window);

  if (priv->bottom_window)
    text_window_unrealize (priv->bottom_window);

  CTK_WIDGET_CLASS (ctk_text_view_parent_class)->unrealize (widget);
}

static void
ctk_text_view_map (CtkWidget *widget)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;

  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  _ctk_pixel_cache_map (priv->pixel_cache);

  CTK_WIDGET_CLASS (ctk_text_view_parent_class)->map (widget);
}

static void
ctk_text_view_unmap (CtkWidget *widget)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;

  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  CTK_WIDGET_CLASS (ctk_text_view_parent_class)->unmap (widget);

  _ctk_pixel_cache_unmap (priv->pixel_cache);
}

static void
text_window_set_padding (CtkTextView     *text_view,
                         CtkStyleContext *context)
{
  CtkTextViewPrivate *priv;
  CtkBorder padding, border;

  priv = text_view->priv;

  ctk_style_context_get_padding (context, ctk_style_context_get_state (context), &padding);
  ctk_style_context_get_border (context, ctk_style_context_get_state (context), &border);
  padding.left += border.left;
  padding.right += border.right;
  padding.top += border.top;
  padding.bottom += border.bottom;

  if (padding.left != priv->left_padding ||
      padding.right != priv->right_padding ||
      padding.top != priv->top_padding ||
      padding.bottom != priv->bottom_padding)
    {
      priv->xoffset += priv->left_padding - padding.left;
      priv->yoffset += priv->top_padding - padding.top;

      priv->left_padding = padding.left;
      priv->right_padding = padding.right;
      priv->top_padding = padding.top;
      priv->bottom_padding = padding.bottom;

      priv->top_border = padding.top + priv->top_margin;
      priv->bottom_border = padding.bottom + priv->bottom_margin;
      priv->left_border = padding.left + priv->left_margin;
      priv->right_border = padding.right + priv->right_margin;

      if (priv->layout && priv->layout->default_style)
        {
          priv->layout->right_padding = priv->right_padding;
          priv->layout->left_padding = priv->left_padding;

          ctk_text_layout_default_style_changed (priv->layout);
        }
    }
}

static void
ctk_text_view_style_updated (CtkWidget *widget)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  PangoContext *ltr_context, *rtl_context;
  CtkStyleContext *style_context;
  CtkCssStyleChange *change;

  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  CTK_WIDGET_CLASS (ctk_text_view_parent_class)->style_updated (widget);

  style_context = ctk_widget_get_style_context (widget);
  change = ctk_style_context_get_change (style_context);

  if ((change == NULL || ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_FONT)) &&
      priv->layout && priv->layout->default_style)
    {
      ctk_text_view_set_attributes_from_style (text_view,
                                               priv->layout->default_style);

      ltr_context = ctk_widget_create_pango_context (widget);
      pango_context_set_base_dir (ltr_context, PANGO_DIRECTION_LTR);
      rtl_context = ctk_widget_create_pango_context (widget);
      pango_context_set_base_dir (rtl_context, PANGO_DIRECTION_RTL);

      ctk_text_layout_set_contexts (priv->layout, ltr_context, rtl_context);

      g_object_unref (ltr_context);
      g_object_unref (rtl_context);
    }
}

static void
ctk_text_view_direction_changed (CtkWidget        *widget,
                                 CtkTextDirection  previous_direction)
{
  CtkTextViewPrivate *priv = CTK_TEXT_VIEW (widget)->priv;

  if (priv->layout && priv->layout->default_style)
    {
      priv->layout->default_style->direction = ctk_widget_get_direction (widget);

      ctk_text_layout_default_style_changed (priv->layout);
    }
}

static void
ctk_text_view_state_flags_changed (CtkWidget     *widget,
                                   CtkStateFlags  previous_state)
{
  CtkTextView *text_view = CTK_TEXT_VIEW (widget);
  CtkTextViewPrivate *priv = text_view->priv;
  CdkCursor *cursor;
  CtkStateFlags state;

  if (ctk_widget_get_realized (widget))
    {
      if (ctk_widget_is_sensitive (widget))
        cursor = cdk_cursor_new_from_name (ctk_widget_get_display (widget), "text");
      else
        cursor = NULL;

      cdk_window_set_cursor (priv->text_window->bin_window, cursor);

      if (cursor)
      g_object_unref (cursor);

      priv->mouse_cursor_obscured = FALSE;
    }

  if (!ctk_widget_is_sensitive (widget))
    {
      /* Clear any selection */
      ctk_text_view_unselect (text_view);
    }

  state = ctk_widget_get_state_flags (widget);
  ctk_css_node_set_state (priv->text_window->css_node, state);

  state &= ~CTK_STATE_FLAG_DROP_ACTIVE;

  ctk_css_node_set_state (priv->selection_node, state);
  if (priv->left_window)
    ctk_css_node_set_state (priv->left_window->css_node, state);
  if (priv->right_window)
    ctk_css_node_set_state (priv->right_window->css_node, state);
  if (priv->top_window)
    ctk_css_node_set_state (priv->top_window->css_node, state);
  if (priv->bottom_window)
    ctk_css_node_set_state (priv->bottom_window->css_node, state);

  ctk_widget_queue_draw (widget);
}

static void
set_invisible_cursor (CdkWindow *window)
{
  CdkDisplay *display;
  CdkCursor *cursor;

  display = cdk_window_get_display (window);
  cursor = cdk_cursor_new_from_name (display, "none");
 
  cdk_window_set_cursor (window, cursor);
  
  g_clear_object (&cursor);
}

static void
ctk_text_view_obscure_mouse_cursor (CtkTextView *text_view)
{
  if (text_view->priv->mouse_cursor_obscured)
    return;

  set_invisible_cursor (text_view->priv->text_window->bin_window);
  
  text_view->priv->mouse_cursor_obscured = TRUE;
}

static void
ctk_text_view_unobscure_mouse_cursor (CtkTextView *text_view)
{
  if (text_view->priv->mouse_cursor_obscured)
    {
      CdkDisplay *display;
      CdkCursor *cursor;

      display = ctk_widget_get_display (CTK_WIDGET (text_view));
      cursor = cdk_cursor_new_from_name (display, "text");
      cdk_window_set_cursor (text_view->priv->text_window->bin_window, cursor);
      g_object_unref (cursor);
      text_view->priv->mouse_cursor_obscured = FALSE;
    }
}

/*
 * Events
 */

static gboolean
get_event_coordinates (CdkEvent *event, gint *x, gint *y)
{
  if (event)
    switch (event->type)
      {
      case CDK_MOTION_NOTIFY:
        *x = event->motion.x;
        *y = event->motion.y;
        return TRUE;
        break;

      case CDK_BUTTON_PRESS:
      case CDK_2BUTTON_PRESS:
      case CDK_3BUTTON_PRESS:
      case CDK_BUTTON_RELEASE:
        *x = event->button.x;
        *y = event->button.y;
        return TRUE;
        break;

      case CDK_KEY_PRESS:
      case CDK_KEY_RELEASE:
      case CDK_ENTER_NOTIFY:
      case CDK_LEAVE_NOTIFY:
      case CDK_PROPERTY_NOTIFY:
      case CDK_SELECTION_CLEAR:
      case CDK_SELECTION_REQUEST:
      case CDK_SELECTION_NOTIFY:
      case CDK_PROXIMITY_IN:
      case CDK_PROXIMITY_OUT:
      case CDK_DRAG_ENTER:
      case CDK_DRAG_LEAVE:
      case CDK_DRAG_MOTION:
      case CDK_DRAG_STATUS:
      case CDK_DROP_START:
      case CDK_DROP_FINISHED:
      default:
        return FALSE;
        break;
      }

  return FALSE;
}

static gint
emit_event_on_tags (CtkWidget   *widget,
                    CdkEvent    *event,
                    CtkTextIter *iter)
{
  GSList *tags;
  GSList *tmp;
  gboolean retval = FALSE;

  tags = ctk_text_iter_get_tags (iter);

  tmp = tags;
  while (tmp != NULL)
    {
      CtkTextTag *tag = tmp->data;

      if (ctk_text_tag_event (tag, G_OBJECT (widget), event, iter))
        {
          retval = TRUE;
          break;
        }

      tmp = tmp->next;
    }

  g_slist_free (tags);

  return retval;
}

static void
_text_window_to_widget_coords (CtkTextView *text_view,
                               gint        *x,
                               gint        *y)
{
  CtkTextViewPrivate *priv = text_view->priv;
  gint border_width = ctk_container_get_border_width (CTK_CONTAINER (text_view));

  *x += border_width;
  *y += border_width;

  if (priv->top_window)
    (*y) += priv->top_window->requisition.height;
  if (priv->left_window)
    (*x) += priv->left_window->requisition.width;
}

static void
_widget_to_text_window_coords (CtkTextView *text_view,
                               gint        *x,
                               gint        *y)
{
  CtkTextViewPrivate *priv = text_view->priv;
  gint border_width = ctk_container_get_border_width (CTK_CONTAINER (text_view));

  *x -= border_width;
  *y -= border_width;

  if (priv->top_window)
    (*y) -= priv->top_window->requisition.height;
  if (priv->left_window)
    (*x) -= priv->left_window->requisition.width;
}

static void
ctk_text_view_set_handle_position (CtkTextView           *text_view,
                                   CtkTextIter           *iter,
                                   CtkTextHandlePosition  pos)
{
  CtkTextViewPrivate *priv;
  CdkRectangle rect;
  gint x, y;

  priv = text_view->priv;
  ctk_text_view_get_cursor_locations (text_view, iter, &rect, NULL);

  x = rect.x - priv->xoffset;
  y = rect.y - priv->yoffset;

  if (!_ctk_text_handle_get_is_dragged (priv->text_handle, pos) &&
      (x < 0 || x > SCREEN_WIDTH (text_view) ||
       y < 0 || y > SCREEN_HEIGHT (text_view)))
    {
      /* Hide the handle if it's not being manipulated
       * and fell outside of the visible text area.
       */
      _ctk_text_handle_set_visible (priv->text_handle, pos, FALSE);
    }
  else
    {
      CtkTextDirection dir = CTK_TEXT_DIR_LTR;
      CtkTextAttributes attributes = { 0 };

      _ctk_text_handle_set_visible (priv->text_handle, pos, TRUE);

      rect.x = CLAMP (x, 0, SCREEN_WIDTH (text_view));
      rect.y = CLAMP (y, 0, SCREEN_HEIGHT (text_view));
      _text_window_to_widget_coords (text_view, &rect.x, &rect.y);

      _ctk_text_handle_set_position (priv->text_handle, pos, &rect);

      if (ctk_text_iter_get_attributes (iter, &attributes))
        dir = attributes.direction;

      _ctk_text_handle_set_direction (priv->text_handle, pos, dir);
    }
}

static void
ctk_text_view_show_magnifier (CtkTextView *text_view,
                              CtkTextIter *iter,
                              gint         x,
                              gint         y)
{
  cairo_rectangle_int_t rect;
  CtkTextViewPrivate *priv;
  CtkAllocation allocation;
  CtkRequisition req;

#define N_LINES 1

  ctk_widget_get_allocation (CTK_WIDGET (text_view), &allocation);

  priv = text_view->priv;
  _ctk_text_view_ensure_magnifier (text_view);

  /* Set size/content depending on iter rect */
  ctk_text_view_get_iter_location (text_view, iter,
                                   (CdkRectangle *) &rect);
  rect.x = x + priv->xoffset;
  ctk_text_view_buffer_to_window_coords (text_view, CTK_TEXT_WINDOW_TEXT,
                                         rect.x, rect.y, &rect.x, &rect.y);
  _text_window_to_widget_coords (text_view, &rect.x, &rect.y);
  req.height = rect.height * N_LINES *
    _ctk_magnifier_get_magnification (CTK_MAGNIFIER (priv->magnifier));
  req.width = MAX ((req.height * 4) / 3, 80);
  ctk_widget_set_size_request (priv->magnifier, req.width, req.height);

  _ctk_magnifier_set_coords (CTK_MAGNIFIER (priv->magnifier),
                             rect.x, rect.y + rect.height / 2);

  rect.x = CLAMP (rect.x, 0, allocation.width);
  rect.y += rect.height / 4;
  rect.height -= rect.height / 4;
  ctk_popover_set_pointing_to (CTK_POPOVER (priv->magnifier_popover),
                               &rect);

  ctk_popover_popup (CTK_POPOVER (priv->magnifier_popover));

#undef N_LINES
}

static void
ctk_text_view_handle_dragged (CtkTextHandle         *handle,
                              CtkTextHandlePosition  pos,
                              gint                   x,
                              gint                   y,
                              CtkTextView           *text_view)
{
  CtkTextViewPrivate *priv;
  CtkTextIter old_cursor, old_bound;
  CtkTextIter cursor, bound, iter;
  CtkTextIter *min, *max;
  CtkTextHandleMode mode;
  CtkTextBuffer *buffer;
  CtkTextHandlePosition cursor_pos;

  priv = text_view->priv;
  buffer = get_buffer (text_view);
  mode = _ctk_text_handle_get_mode (handle);

  _widget_to_text_window_coords (text_view, &x, &y);

  ctk_text_view_selection_bubble_popup_unset (text_view);
  ctk_text_layout_get_iter_at_pixel (priv->layout, &iter,
                                     x + priv->xoffset,
                                     y + priv->yoffset);
  ctk_text_buffer_get_iter_at_mark (buffer, &old_cursor,
                                    ctk_text_buffer_get_insert (buffer));
  ctk_text_buffer_get_iter_at_mark (buffer, &old_bound,
                                    ctk_text_buffer_get_selection_bound (buffer));
  cursor = old_cursor;
  bound = old_bound;

  if (mode == CTK_TEXT_HANDLE_MODE_CURSOR ||
      ctk_text_iter_compare (&cursor, &bound) >= 0)
    {
      cursor_pos = CTK_TEXT_HANDLE_POSITION_CURSOR;
      max = &cursor;
      min = &bound;
    }
  else
    {
      cursor_pos = CTK_TEXT_HANDLE_POSITION_SELECTION_START;
      max = &bound;
      min = &cursor;
    }

  if (pos == CTK_TEXT_HANDLE_POSITION_SELECTION_END)
    {
      if (mode == CTK_TEXT_HANDLE_MODE_SELECTION &&
	  ctk_text_iter_compare (&iter, min) <= 0)
        {
          iter = *min;
          ctk_text_iter_forward_char (&iter);
        }

      *max = iter;
      ctk_text_view_set_handle_position (text_view, &iter, pos);
    }
  else
    {
      if (mode == CTK_TEXT_HANDLE_MODE_SELECTION &&
	  ctk_text_iter_compare (&iter, max) >= 0)
        {
          iter = *max;
          ctk_text_iter_backward_char (&iter);
        }

      *min = iter;
      ctk_text_view_set_handle_position (text_view, &iter, pos);
    }

  if (ctk_text_iter_compare (&old_cursor, &cursor) != 0 ||
      ctk_text_iter_compare (&old_bound, &bound) != 0)
    {
      if (mode == CTK_TEXT_HANDLE_MODE_CURSOR)
        ctk_text_buffer_place_cursor (buffer, &cursor);
      else
        ctk_text_buffer_select_range (buffer, &cursor, &bound);

      if (_ctk_text_handle_get_is_dragged (priv->text_handle, cursor_pos))
        {
          text_view->priv->cursor_handle_dragged = TRUE;
          ctk_text_view_scroll_mark_onscreen (text_view,
                                              ctk_text_buffer_get_insert (buffer));
        }
      else
        {
          text_view->priv->selection_handle_dragged = TRUE;
          ctk_text_view_scroll_mark_onscreen (text_view,
                                              ctk_text_buffer_get_selection_bound (buffer));
        }
    }

  if (_ctk_text_handle_get_is_dragged (priv->text_handle, cursor_pos))
    ctk_text_view_show_magnifier (text_view, &cursor, x, y);
  else
    ctk_text_view_show_magnifier (text_view, &bound, x, y);
}

static void
ctk_text_view_handle_drag_started (CtkTextHandle         *handle,
                                   CtkTextHandlePosition  pos,
                                   CtkTextView           *text_view)
{
  text_view->priv->cursor_handle_dragged = FALSE;
  text_view->priv->selection_handle_dragged = FALSE;
}

static void
ctk_text_view_handle_drag_finished (CtkTextHandle         *handle,
                                    CtkTextHandlePosition  pos,
                                    CtkTextView           *text_view)
{
  CtkTextViewPrivate *priv = text_view->priv;

  if (!priv->cursor_handle_dragged && !priv->selection_handle_dragged)
    {
      CtkTextBuffer *buffer;
      CtkTextIter cursor, start, end;
      CtkSettings *settings;
      guint double_click_time;

      settings = ctk_widget_get_settings (CTK_WIDGET (text_view));
      g_object_get (settings, "ctk-double-click-time", &double_click_time, NULL);
      if (g_get_monotonic_time() - priv->handle_place_time < double_click_time * 1000)
        {
          buffer = get_buffer (text_view);
          ctk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                            ctk_text_buffer_get_insert (buffer));
          extend_selection (text_view, SELECT_WORDS, &cursor, &start, &end);
          ctk_text_buffer_select_range (buffer, &start, &end);

          ctk_text_view_update_handles (text_view, CTK_TEXT_HANDLE_MODE_SELECTION);
        }
      else
        ctk_text_view_selection_bubble_popup_set (text_view);
    }

  if (priv->magnifier_popover)
    ctk_popover_popdown (CTK_POPOVER (priv->magnifier_popover));
}

static gboolean cursor_visible (CtkTextView *text_view);

static void
ctk_text_view_update_handles (CtkTextView       *text_view,
                              CtkTextHandleMode  mode)
{
  CtkTextViewPrivate *priv = text_view->priv;
  CtkTextIter cursor, bound, min, max;
  CtkTextBuffer *buffer;

  buffer = get_buffer (text_view);

  ctk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                    ctk_text_buffer_get_insert (buffer));
  ctk_text_buffer_get_iter_at_mark (buffer, &bound,
                                    ctk_text_buffer_get_selection_bound (buffer));

  if (mode == CTK_TEXT_HANDLE_MODE_SELECTION &&
      ctk_text_iter_compare (&cursor, &bound) == 0)
    {
      mode = CTK_TEXT_HANDLE_MODE_CURSOR;
    }

  if (mode == CTK_TEXT_HANDLE_MODE_CURSOR &&
      (!ctk_widget_is_sensitive (CTK_WIDGET (text_view)) || !cursor_visible (text_view)))
    {
      mode = CTK_TEXT_HANDLE_MODE_NONE;
    }

  _ctk_text_handle_set_mode (priv->text_handle, mode);

  if (ctk_text_iter_compare (&cursor, &bound) >= 0)
    {
      min = bound;
      max = cursor;
    }
  else
    {
      min = cursor;
      max = bound;
    }

  if (mode != CTK_TEXT_HANDLE_MODE_NONE)
    ctk_text_view_set_handle_position (text_view, &max,
                                       CTK_TEXT_HANDLE_POSITION_SELECTION_END);

  if (mode == CTK_TEXT_HANDLE_MODE_SELECTION)
    ctk_text_view_set_handle_position (text_view, &min,
                                       CTK_TEXT_HANDLE_POSITION_SELECTION_START);
}

static gint
ctk_text_view_event (CtkWidget *widget, CdkEvent *event)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  gint x = 0, y = 0;

  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  if (priv->layout == NULL ||
      get_buffer (text_view) == NULL)
    return FALSE;

  if (event->any.window != priv->text_window->bin_window)
    return FALSE;

  if (get_event_coordinates (event, &x, &y))
    {
      CtkTextIter iter;

      x += priv->xoffset;
      y += priv->yoffset;

      /* FIXME this is slow and we do it twice per event.
       * My favorite solution is to have CtkTextLayout cache
       * the last couple lookups.
       */
      ctk_text_layout_get_iter_at_pixel (priv->layout,
                                         &iter,
                                         x, y);

      return emit_event_on_tags (widget, event, &iter);
    }
  else if (event->type == CDK_KEY_PRESS ||
           event->type == CDK_KEY_RELEASE)
    {
      CtkTextIter iter;

      ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &iter,
                                        ctk_text_buffer_get_insert (get_buffer (text_view)));

      return emit_event_on_tags (widget, event, &iter);
    }
  else
    return FALSE;
}

static gint
ctk_text_view_key_press_event (CtkWidget *widget, CdkEventKey *event)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  CtkTextMark *insert;
  CtkTextIter iter;
  gboolean can_insert;
  gboolean retval = FALSE;

  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  if (priv->layout == NULL || get_buffer (text_view) == NULL)
    return FALSE;

  priv->handling_key_event = TRUE;

  /* Make sure input method knows where it is */
  flush_update_im_spot_location (text_view);

  insert = ctk_text_buffer_get_insert (get_buffer (text_view));
  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &iter, insert);
  can_insert = ctk_text_iter_can_insert (&iter, priv->editable);
  if (ctk_im_context_filter_keypress (priv->im_context, event))
    {
      priv->need_im_reset = TRUE;
      if (!can_insert)
        ctk_text_view_reset_im_context (text_view);
      retval = TRUE;
    }
  /* Binding set */
  else if (CTK_WIDGET_CLASS (ctk_text_view_parent_class)->key_press_event (widget, event))
    {
      retval = TRUE;
    }
  /* use overall editability not can_insert, more predictable for users */
  else if (priv->editable &&
           (event->keyval == CDK_KEY_Return ||
            event->keyval == CDK_KEY_ISO_Enter ||
            event->keyval == CDK_KEY_KP_Enter))
    {
      /* this won't actually insert the newline if the cursor isn't
       * editable
       */
      ctk_text_view_reset_im_context (text_view);
      ctk_text_view_commit_text (text_view, "\n");
      retval = TRUE;
    }
  /* Pass through Tab as literal tab, unless Control is held down */
  else if ((event->keyval == CDK_KEY_Tab ||
            event->keyval == CDK_KEY_KP_Tab ||
            event->keyval == CDK_KEY_ISO_Left_Tab) &&
           !(event->state & CDK_CONTROL_MASK))
    {
      /* If the text widget isn't editable overall, or if the application
       * has turned off "accepts_tab", move the focus instead
       */
      if (priv->accepts_tab && priv->editable)
	{
	  ctk_text_view_reset_im_context (text_view);
	  ctk_text_view_commit_text (text_view, "\t");
	}
      else
	g_signal_emit_by_name (text_view, "move-focus",
                               (event->state & CDK_SHIFT_MASK) ?
                               CTK_DIR_TAB_BACKWARD : CTK_DIR_TAB_FORWARD);

      retval = TRUE;
    }
  else
    retval = FALSE;

  ctk_text_view_reset_blink_time (text_view);
  ctk_text_view_pend_cursor_blink (text_view);

  if (!event->send_event && priv->text_handle)
    _ctk_text_handle_set_mode (priv->text_handle,
                               CTK_TEXT_HANDLE_MODE_NONE);

  ctk_text_view_selection_bubble_popup_unset (text_view);

  priv->handling_key_event = FALSE;

  return retval;
}

static gint
ctk_text_view_key_release_event (CtkWidget *widget, CdkEventKey *event)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  CtkTextMark *insert;
  CtkTextIter iter;
  gboolean retval = FALSE;

  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  if (priv->layout == NULL || get_buffer (text_view) == NULL)
    return FALSE;

  priv->handling_key_event = TRUE;

  insert = ctk_text_buffer_get_insert (get_buffer (text_view));
  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &iter, insert);
  if (ctk_text_iter_can_insert (&iter, priv->editable) &&
      ctk_im_context_filter_keypress (priv->im_context, event))
    {
      priv->need_im_reset = TRUE;
      retval = TRUE;
    }
  else
    retval = CTK_WIDGET_CLASS (ctk_text_view_parent_class)->key_release_event (widget, event);

  priv->handling_key_event = FALSE;

  return retval;
}

static gboolean
get_iter_from_gesture (CtkTextView *text_view,
                       CtkGesture  *gesture,
                       CtkTextIter *iter,
                       gint        *x,
                       gint        *y)
{
  CdkEventSequence *sequence;
  CtkTextViewPrivate *priv;
  gint xcoord, ycoord;
  gdouble px, py;

  priv = text_view->priv;
  sequence =
    ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));

  if (!ctk_gesture_get_point (gesture, sequence, &px, &py))
    return FALSE;

  xcoord = px + priv->xoffset;
  ycoord = py + priv->yoffset;
  _widget_to_text_window_coords (text_view, &xcoord, &ycoord);
  ctk_text_layout_get_iter_at_pixel (priv->layout, iter, xcoord, ycoord);

  if (x)
    *x = xcoord;
  if (y)
    *y = ycoord;

  return TRUE;
}

static void
ctk_text_view_multipress_gesture_pressed (CtkGestureMultiPress *gesture,
                                          gint                  n_press,
                                          gdouble               x,
                                          gdouble               y,
                                          CtkTextView          *text_view)
{
  CdkEventSequence *sequence;
  CtkTextViewPrivate *priv;
  const CdkEvent *event;
  gboolean is_touchscreen;
  CdkDevice *device;
  CtkTextIter iter;
  guint button;

  priv = text_view->priv;
  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  button = ctk_gesture_single_get_current_button (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);

  ctk_widget_grab_focus (CTK_WIDGET (text_view));

  if (cdk_event_get_window (event) != priv->text_window->bin_window)
    {
      /* Remove selection if any. */
      ctk_text_view_unselect (text_view);
      return;
    }

  ctk_gesture_set_sequence_state (CTK_GESTURE (gesture), sequence,
                                  CTK_EVENT_SEQUENCE_CLAIMED);
  ctk_text_view_reset_blink_time (text_view);

#if 0
  /* debug hack */
  if (event->button == CDK_BUTTON_SECONDARY && (event->state & CDK_CONTROL_MASK) != 0)
    _ctk_text_buffer_spew (CTK_TEXT_VIEW (widget)->buffer);
  else if (event->button == CDK_BUTTON_SECONDARY)
    ctk_text_layout_spew (CTK_TEXT_VIEW (widget)->layout);
#endif

  device = cdk_event_get_source_device ((CdkEvent *) event);
  is_touchscreen = ctk_simulate_touchscreen () ||
                   cdk_device_get_source (device) == CDK_SOURCE_TOUCHSCREEN;

  if (n_press == 1)
    ctk_text_view_reset_im_context (text_view);

  if (n_press == 1 &&
      cdk_event_triggers_context_menu (event))
    {
      ctk_text_view_do_popup (text_view, event);
    }
  else if (button == CDK_BUTTON_MIDDLE &&
           get_middle_click_paste (text_view))
    {
      get_iter_from_gesture (text_view, priv->multipress_gesture,
                             &iter, NULL, NULL);
      ctk_text_buffer_paste_clipboard (get_buffer (text_view),
                                       ctk_widget_get_clipboard (CTK_WIDGET (text_view),
                                                                 CDK_SELECTION_PRIMARY),
                                       &iter,
                                       priv->editable);
    }
  else if (button == CDK_BUTTON_PRIMARY)
    {
      CtkTextHandleMode handle_mode = CTK_TEXT_HANDLE_MODE_NONE;
      gboolean extends = FALSE;
      CdkModifierType state;

      cdk_event_get_state (event, &state);

      if (state &
          ctk_widget_get_modifier_mask (CTK_WIDGET (text_view),
                                        CDK_MODIFIER_INTENT_EXTEND_SELECTION))
        extends = TRUE;

      switch (n_press)
        {
        case 1:
          {
            /* If we're in the selection, start a drag copy/move of the
             * selection; otherwise, start creating a new selection.
             */
            CtkTextIter start, end;

            if (is_touchscreen)
              handle_mode = CTK_TEXT_HANDLE_MODE_CURSOR;

            get_iter_from_gesture (text_view, priv->multipress_gesture,
                                   &iter, NULL, NULL);

            if (ctk_text_buffer_get_selection_bounds (get_buffer (text_view),
                                                      &start, &end) &&
                ctk_text_iter_in_range (&iter, &start, &end) && !extends)
              {
                if (is_touchscreen)
                  {
                    if (!priv->selection_bubble ||
			!ctk_widget_get_visible (priv->selection_bubble))
                      {
                        ctk_text_view_selection_bubble_popup_set (text_view);
                        handle_mode = CTK_TEXT_HANDLE_MODE_NONE;
                      }
                    else
                      {
                        ctk_text_view_selection_bubble_popup_unset (text_view);
                        handle_mode = CTK_TEXT_HANDLE_MODE_SELECTION;
                      }
                  }
                else
                  {
                    /* Claim the sequence on the drag gesture, but attach no
                     * selection data, this is a special case to start DnD.
                     */
                    ctk_gesture_set_state (priv->drag_gesture,
                                           CTK_EVENT_SEQUENCE_CLAIMED);
                  }
                break;
              }
            else
	      {
                ctk_text_view_selection_bubble_popup_unset (text_view);

		if (is_touchscreen)
                  {
		    ctk_text_buffer_place_cursor (get_buffer (text_view), &iter);
                    priv->handle_place_time = g_get_monotonic_time ();
                  }
		else
		  ctk_text_view_start_selection_drag (text_view, &iter,
						      SELECT_CHARACTERS, extends);
	      }
            break;
          }
        case 2:
        case 3:
          if (is_touchscreen)
            {
              handle_mode = CTK_TEXT_HANDLE_MODE_SELECTION;
              break;
            }
          ctk_text_view_end_selection_drag (text_view);

          get_iter_from_gesture (text_view, priv->multipress_gesture,
                                 &iter, NULL, NULL);
          ctk_text_view_start_selection_drag (text_view, &iter,
                                              n_press == 2 ? SELECT_WORDS : SELECT_LINES,
                                              extends);
          break;
        default:
          break;
        }

      _ctk_text_view_ensure_text_handles (text_view);
      ctk_text_view_update_handles (text_view, handle_mode);
    }

  if (n_press >= 3)
    ctk_event_controller_reset (CTK_EVENT_CONTROLLER (gesture));
}

static void
keymap_direction_changed (CdkKeymap   *keymap,
			  CtkTextView *text_view)
{
  ctk_text_view_check_keymap_direction (text_view);
}

static gint
ctk_text_view_focus_in_event (CtkWidget *widget, CdkEventFocus *event)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;

  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  ctk_widget_queue_draw (widget);

  DV(g_print (G_STRLOC": focus_in_event\n"));

  ctk_text_view_reset_blink_time (text_view);

  if (cursor_visible (text_view) && priv->layout)
    {
      ctk_text_layout_set_cursor_visible (priv->layout, TRUE);
      ctk_text_view_check_cursor_blink (text_view);
    }

  g_signal_connect (cdk_keymap_get_for_display (ctk_widget_get_display (widget)),
		    "direction-changed",
		    G_CALLBACK (keymap_direction_changed), text_view);
  ctk_text_view_check_keymap_direction (text_view);

  if (priv->editable)
    {
      priv->need_im_reset = TRUE;
      ctk_im_context_focus_in (priv->im_context);
    }

  return FALSE;
}

static gint
ctk_text_view_focus_out_event (CtkWidget *widget, CdkEventFocus *event)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;

  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  ctk_text_view_end_selection_drag (text_view);

  ctk_widget_queue_draw (widget);

  DV(g_print (G_STRLOC": focus_out_event\n"));
  
  if (cursor_visible (text_view) && priv->layout)
    {
      ctk_text_view_check_cursor_blink (text_view);
      ctk_text_layout_set_cursor_visible (priv->layout, FALSE);
    }

  g_signal_handlers_disconnect_by_func (cdk_keymap_get_for_display (ctk_widget_get_display (widget)),
					keymap_direction_changed,
					text_view);
  ctk_text_view_selection_bubble_popup_unset (text_view);

  if (priv->text_handle)
    _ctk_text_handle_set_mode (priv->text_handle,
                               CTK_TEXT_HANDLE_MODE_NONE);

  if (priv->editable)
    {
      priv->need_im_reset = TRUE;
      ctk_im_context_focus_out (priv->im_context);
    }

  return FALSE;
}

static gboolean
ctk_text_view_motion_event (CtkWidget *widget, CdkEventMotion *event)
{
  CtkTextView *text_view = CTK_TEXT_VIEW (widget);

  ctk_text_view_unobscure_mouse_cursor (text_view);

  return CTK_WIDGET_CLASS (ctk_text_view_parent_class)->motion_notify_event (widget, event);
}

static void
ctk_text_view_paint (CtkWidget      *widget,
                     cairo_t        *cr)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  
  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  g_return_if_fail (priv->layout != NULL);
  g_return_if_fail (priv->xoffset >= - priv->left_padding);
  g_return_if_fail (priv->yoffset >= - priv->top_border);

  while (priv->first_validate_idle != 0)
    {
      DV (g_print (G_STRLOC": first_validate_idle: %d\n",
                   priv->first_validate_idle));
      ctk_text_view_flush_first_validate (text_view);
    }

  if (!priv->onscreen_validated)
    {
      g_warning (G_STRLOC ": somehow some text lines were modified or scrolling occurred since the last validation of lines on the screen - may be a text widget bug.");
      g_assert_not_reached ();
    }
  
#if 0
  printf ("painting %d,%d  %d x %d\n",
          area->x, area->y,
          area->width, area->height);
#endif

  cairo_save (cr);
  cairo_translate (cr, -priv->xoffset, -priv->yoffset);

  ctk_text_layout_draw (priv->layout,
                        widget,
                        cr,
                        NULL);

  cairo_restore (cr);
}

static void
draw_text (cairo_t  *cr,
           gpointer  user_data)
{
  CtkWidget *widget = user_data;
  CtkTextView *text_view = CTK_TEXT_VIEW (widget);
  CtkTextViewPrivate *priv = text_view->priv;
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);
  ctk_style_context_save_to_node (context, text_view->priv->text_window->css_node);
  ctk_render_background (context, cr,
                         -priv->xoffset, -priv->yoffset - priv->top_border,
                         MAX (SCREEN_WIDTH (text_view), priv->width),
                         MAX (SCREEN_HEIGHT (text_view), priv->height));
  ctk_render_frame (context, cr,
                    -priv->xoffset, -priv->yoffset - priv->top_border,
                    MAX (SCREEN_WIDTH (text_view), priv->width),
                    MAX (SCREEN_HEIGHT (text_view), priv->height));
  ctk_style_context_restore (context);

  if (CTK_TEXT_VIEW_GET_CLASS (text_view)->draw_layer != NULL)
    {
      cairo_save (cr);
      CTK_TEXT_VIEW_GET_CLASS (text_view)->draw_layer (text_view, CTK_TEXT_VIEW_LAYER_BELOW, cr);
      cairo_restore (cr);

      cairo_save (cr);
      cairo_translate (cr, -priv->xoffset, -priv->yoffset);
      CTK_TEXT_VIEW_GET_CLASS (text_view)->draw_layer (text_view, CTK_TEXT_VIEW_LAYER_BELOW_TEXT, cr);
      cairo_restore (cr);
    }

  ctk_text_view_paint (widget, cr);

  if (CTK_TEXT_VIEW_GET_CLASS (text_view)->draw_layer != NULL)
    {
      cairo_save (cr);
      CTK_TEXT_VIEW_GET_CLASS (text_view)->draw_layer (text_view, CTK_TEXT_VIEW_LAYER_ABOVE, cr);
      cairo_restore (cr);

      cairo_save (cr);
      cairo_translate (cr, -priv->xoffset, -priv->yoffset);
      CTK_TEXT_VIEW_GET_CLASS (text_view)->draw_layer (text_view, CTK_TEXT_VIEW_LAYER_ABOVE_TEXT, cr);
      cairo_restore (cr);
    }
}

static void
paint_border_window (CtkTextView     *text_view,
                     cairo_t         *cr,
                     CtkTextWindow   *text_window,
                     CtkStyleContext *context)
{
  CdkWindow *window;

  if (text_window == NULL)
    return;

  window = ctk_text_view_get_window (text_view, text_window->type);
  if (ctk_cairo_should_draw_window (cr, window))
    {
      gint w, h;

      ctk_style_context_save_to_node (context, text_window->css_node);

      w = cdk_window_get_width (window);
      h = cdk_window_get_height (window);

      cairo_save (cr);
      ctk_cairo_transform_to_window (cr, CTK_WIDGET (text_view), window);
      ctk_render_background (context, cr, 0, 0, w, h);
      cairo_restore (cr);

      ctk_style_context_restore (context);
    }
}

static gboolean
ctk_text_view_draw (CtkWidget *widget,
                    cairo_t   *cr)
{
  CtkTextViewPrivate *priv = ((CtkTextView *)widget)->priv;
  GSList *tmp_list;
  CdkWindow *window;
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);

  text_window_set_padding (CTK_TEXT_VIEW (widget), context);

  if (ctk_cairo_should_draw_window (cr, ctk_widget_get_window (widget)))
    {
      ctk_render_background (context, cr,
			     0, 0,
			     ctk_widget_get_allocated_width (widget),
			     ctk_widget_get_allocated_height (widget));
    }

  window = ctk_text_view_get_window (CTK_TEXT_VIEW (widget),
                                     CTK_TEXT_WINDOW_TEXT);
  if (ctk_cairo_should_draw_window (cr, window))
    {
      cairo_rectangle_int_t view_rect;
      cairo_rectangle_int_t canvas_rect;
      CtkAllocation alloc;

      DV(g_print (">Exposed ("G_STRLOC")\n"));

      ctk_widget_get_allocation (widget, &alloc);

      view_rect.x = 0;
      view_rect.y = 0;
      view_rect.width = cdk_window_get_width (window);
      view_rect.height = cdk_window_get_height (window);

      canvas_rect.x = -ctk_adjustment_get_value (priv->hadjustment);
      canvas_rect.y = -ctk_adjustment_get_value (priv->vadjustment);
      canvas_rect.width = priv->width;
      canvas_rect.height = priv->height;

      cairo_save (cr);
      ctk_cairo_transform_to_window (cr, widget, window);
      _ctk_pixel_cache_draw (priv->pixel_cache, cr, window,
                             &view_rect, &canvas_rect,
                             draw_text, widget);
      cairo_restore (cr);
    }

  paint_border_window (CTK_TEXT_VIEW (widget), cr, priv->left_window, context);
  paint_border_window (CTK_TEXT_VIEW (widget), cr, priv->right_window, context);
  paint_border_window (CTK_TEXT_VIEW (widget), cr, priv->top_window, context);
  paint_border_window (CTK_TEXT_VIEW (widget), cr, priv->bottom_window, context);

  /* Propagate exposes to all unanchored children. 
   * Anchored children are handled in ctk_text_view_paint(). 
   */
  tmp_list = CTK_TEXT_VIEW (widget)->priv->children;
  while (tmp_list != NULL)
    {
      CtkTextViewChild *vc = tmp_list->data;

      /* propagate_draw checks that event->window matches
       * child->window
       */
      ctk_container_propagate_draw (CTK_CONTAINER (widget),
                                    vc->widget,
                                    cr);
      
      tmp_list = tmp_list->next;
    }
  
  return FALSE;
}

static gboolean
ctk_text_view_focus (CtkWidget        *widget,
                     CtkDirectionType  direction)
{
  CtkContainer *container;
  gboolean result;

  container = CTK_CONTAINER (widget);

  if (!ctk_widget_is_focus (widget) &&
      ctk_container_get_focus_child (container) == NULL)
    {
      if (ctk_widget_get_can_focus (widget))
        {
          ctk_widget_grab_focus (widget);
          return TRUE;
        }

      return FALSE;
    }
  else
    {
      gboolean can_focus;
      /*
       * Unset CAN_FOCUS flag so that ctk_container_focus() allows
       * children to get the focus
       */
      can_focus = ctk_widget_get_can_focus (widget);
      ctk_widget_set_can_focus (widget, FALSE);
      result = CTK_WIDGET_CLASS (ctk_text_view_parent_class)->focus (widget, direction);
      ctk_widget_set_can_focus (widget, can_focus);

      return result;
    }
}

/*
 * Container
 */

static void
ctk_text_view_add (CtkContainer *container,
                   CtkWidget    *child)
{
  /* This is pretty random. */
  ctk_text_view_add_child_in_window (CTK_TEXT_VIEW (container),
                                     child,
                                     CTK_TEXT_WINDOW_WIDGET,
                                     0, 0);
}

static void
ctk_text_view_remove (CtkContainer *container,
                      CtkWidget    *child)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  CtkTextViewChild *vc;
  GSList *iter;

  text_view = CTK_TEXT_VIEW (container);
  priv = text_view->priv;

  vc = NULL;
  iter = priv->children;

  while (iter != NULL)
    {
      vc = iter->data;

      if (vc->widget == child)
        break;

      iter = iter->next;
    }

  g_assert (iter != NULL); /* be sure we had the child in the list */

  priv->children = g_slist_remove (priv->children, vc);

  ctk_widget_unparent (vc->widget);

  text_view_child_free (vc);
}

static void
ctk_text_view_forall (CtkContainer *container,
                      gboolean      include_internals,
                      CtkCallback   callback,
                      gpointer      callback_data)
{
  GSList *iter;
  CtkTextView *text_view;
  GSList *copy;

  g_return_if_fail (CTK_IS_TEXT_VIEW (container));
  g_return_if_fail (callback != NULL);

  text_view = CTK_TEXT_VIEW (container);

  copy = g_slist_copy (text_view->priv->children);
  iter = copy;

  while (iter != NULL)
    {
      CtkTextViewChild *vc = iter->data;

      (* callback) (vc->widget, callback_data);

      iter = iter->next;
    }

  g_slist_free (copy);
}

#define CURSOR_ON_MULTIPLIER 2
#define CURSOR_OFF_MULTIPLIER 1
#define CURSOR_PEND_MULTIPLIER 3
#define CURSOR_DIVIDER 3

static gboolean
cursor_blinks (CtkTextView *text_view)
{
  CtkSettings *settings = ctk_widget_get_settings (CTK_WIDGET (text_view));
  gboolean blink;

#ifdef DEBUG_VALIDATION_AND_SCROLLING
  return FALSE;
#endif
#ifdef G_ENABLE_DEBUG
  if (CTK_DEBUG_CHECK (UPDATES))
    return FALSE;
#endif

  g_object_get (settings, "ctk-cursor-blink", &blink, NULL);

  if (!blink)
    return FALSE;

  if (text_view->priv->editable)
    {
      CtkTextMark *insert;
      CtkTextIter iter;
      
      insert = ctk_text_buffer_get_insert (get_buffer (text_view));
      ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &iter, insert);
      
      if (ctk_text_iter_editable (&iter, text_view->priv->editable))
	return blink;
    }

  return FALSE;
}

static gboolean
cursor_visible (CtkTextView *text_view)
{
  CtkSettings *settings = ctk_widget_get_settings (CTK_WIDGET (text_view));
  gboolean use_caret;

  g_object_get (settings, "ctk-keynav-use-caret", &use_caret, NULL);

   return use_caret || text_view->priv->cursor_visible;
}

static gboolean
get_middle_click_paste (CtkTextView *text_view)
{
  CtkSettings *settings;
  gboolean paste;

  settings = ctk_widget_get_settings (CTK_WIDGET (text_view));
  g_object_get (settings, "ctk-enable-primary-paste", &paste, NULL);

  return paste;
}

static gint
get_cursor_time (CtkTextView *text_view)
{
  CtkSettings *settings = ctk_widget_get_settings (CTK_WIDGET (text_view));
  gint time;

  g_object_get (settings, "ctk-cursor-blink-time", &time, NULL);

  return time;
}

static gint
get_cursor_blink_timeout (CtkTextView *text_view)
{
  CtkSettings *settings = ctk_widget_get_settings (CTK_WIDGET (text_view));
  gint time;

  g_object_get (settings, "ctk-cursor-blink-timeout", &time, NULL);

  return time;
}


/*
 * Blink!
 */

static gint
blink_cb (gpointer data)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  gboolean visible;
  gint blink_timeout;

  text_view = CTK_TEXT_VIEW (data);
  priv = text_view->priv;

  if (!ctk_widget_has_focus (CTK_WIDGET (text_view)))
    {
      g_warning ("CtkTextView - did not receive focus-out-event. If you\n"
                 "connect a handler to this signal, it must return\n"
                 "FALSE so the text view gets the event as well");

      ctk_text_view_check_cursor_blink (text_view);

      return FALSE;
    }

  g_assert (priv->layout);
  g_assert (cursor_visible (text_view));

  visible = ctk_text_layout_get_cursor_visible (priv->layout);

  blink_timeout = get_cursor_blink_timeout (text_view);
  if (priv->blink_time > 1000 * blink_timeout &&
      blink_timeout < G_MAXINT/1000) 
    {
      /* we've blinked enough without the user doing anything, stop blinking */
      visible = 0;
      priv->blink_timeout = 0;
    } 
  else if (visible)
    {
      priv->blink_timeout = cdk_threads_add_timeout (get_cursor_time (text_view) * CURSOR_OFF_MULTIPLIER / CURSOR_DIVIDER,
						     blink_cb,
						     text_view);
      g_source_set_name_by_id (priv->blink_timeout, "[ctk+] blink_cb");
    }
  else 
    {
      priv->blink_timeout = cdk_threads_add_timeout (get_cursor_time (text_view) * CURSOR_ON_MULTIPLIER / CURSOR_DIVIDER,
						     blink_cb,
						     text_view);
      g_source_set_name_by_id (priv->blink_timeout, "[ctk+] blink_cb");
      priv->blink_time += get_cursor_time (text_view);
    }

  /* Block changed_handler while changing the layout's cursor visibility
   * because it would expose the whole paragraph. Instead, we expose
   * the cursor's area(s) manually below.
   */
  g_signal_handlers_block_by_func (priv->layout,
                                   changed_handler,
                                   text_view);
  ctk_text_layout_set_cursor_visible (priv->layout, !visible);
  g_signal_handlers_unblock_by_func (priv->layout,
                                     changed_handler,
                                     text_view);

  text_window_invalidate_cursors (priv->text_window);

  /* Remove ourselves */
  return FALSE;
}


static void
ctk_text_view_stop_cursor_blink (CtkTextView *text_view)
{
  if (text_view->priv->blink_timeout)
    { 
      g_source_remove (text_view->priv->blink_timeout);
      text_view->priv->blink_timeout = 0;
    }
}

static void
ctk_text_view_check_cursor_blink (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv = text_view->priv;

  if (priv->layout != NULL &&
      cursor_visible (text_view) &&
      ctk_widget_has_focus (CTK_WIDGET (text_view)))
    {
      if (cursor_blinks (text_view))
	{
	  if (priv->blink_timeout == 0)
	    {
	      ctk_text_layout_set_cursor_visible (priv->layout, TRUE);
	      
	      priv->blink_timeout = cdk_threads_add_timeout (get_cursor_time (text_view) * CURSOR_OFF_MULTIPLIER / CURSOR_DIVIDER,
							     blink_cb,
							     text_view);
	      g_source_set_name_by_id (priv->blink_timeout, "[ctk+] blink_cb");
	    }
	}
      else
	{
	  ctk_text_view_stop_cursor_blink (text_view);
	  ctk_text_layout_set_cursor_visible (priv->layout, TRUE);
	}
    }
  else
    {
      ctk_text_view_stop_cursor_blink (text_view);
      ctk_text_layout_set_cursor_visible (priv->layout, FALSE);
    }
}

static void
ctk_text_view_pend_cursor_blink (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv = text_view->priv;

  if (priv->layout != NULL &&
      cursor_visible (text_view) &&
      ctk_widget_has_focus (CTK_WIDGET (text_view)) &&
      cursor_blinks (text_view))
    {
      ctk_text_view_stop_cursor_blink (text_view);
      ctk_text_layout_set_cursor_visible (priv->layout, TRUE);
      
      priv->blink_timeout = cdk_threads_add_timeout (get_cursor_time (text_view) * CURSOR_PEND_MULTIPLIER / CURSOR_DIVIDER,
						     blink_cb,
						     text_view);
      g_source_set_name_by_id (priv->blink_timeout, "[ctk+] blink_cb");
    }
}

static void
ctk_text_view_reset_blink_time (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv = text_view->priv;

  priv->blink_time = 0;
}


/*
 * Key binding handlers
 */

static gboolean
ctk_text_view_move_iter_by_lines (CtkTextView *text_view,
                                  CtkTextIter *newplace,
                                  gint         count)
{
  gboolean ret = TRUE;

  while (count < 0)
    {
      ret = ctk_text_layout_move_iter_to_previous_line (text_view->priv->layout, newplace);
      count++;
    }

  while (count > 0)
    {
      ret = ctk_text_layout_move_iter_to_next_line (text_view->priv->layout, newplace);
      count--;
    }

  return ret;
}

static void
move_cursor (CtkTextView       *text_view,
             const CtkTextIter *new_location,
             gboolean           extend_selection)
{
  if (extend_selection)
    ctk_text_buffer_move_mark_by_name (get_buffer (text_view),
                                       "insert",
                                       new_location);
  else
      ctk_text_buffer_place_cursor (get_buffer (text_view),
				    new_location);
  ctk_text_view_check_cursor_blink (text_view);
}

static gboolean
iter_line_is_rtl (const CtkTextIter *iter)
{
  CtkTextIter start, end;
  char *text;
  PangoDirection direction;

  start = end = *iter;
  ctk_text_iter_set_line_offset (&start, 0);
  ctk_text_iter_forward_line (&end);
  text = ctk_text_iter_get_visible_text (&start, &end);
  direction = _ctk_pango_find_base_dir (text, -1);

  g_free (text);

  return direction == PANGO_DIRECTION_RTL;
}

static void
ctk_text_view_move_cursor (CtkTextView     *text_view,
                           CtkMovementStep  step,
                           gint             count,
                           gboolean         extend_selection)
{
  CtkTextViewPrivate *priv;
  CtkTextIter insert;
  CtkTextIter newplace;
  gboolean cancel_selection = FALSE;
  gint cursor_x_pos = 0;
  CtkDirectionType leave_direction = -1;

  priv = text_view->priv;

  if (!cursor_visible (text_view))
    {
      CtkScrollStep scroll_step;
      gdouble old_xpos, old_ypos;

      switch (step) 
	{
        case CTK_MOVEMENT_VISUAL_POSITIONS:
          leave_direction = count > 0 ? CTK_DIR_RIGHT : CTK_DIR_LEFT;
          /* fall through */
        case CTK_MOVEMENT_LOGICAL_POSITIONS:
        case CTK_MOVEMENT_WORDS:
	  scroll_step = CTK_SCROLL_HORIZONTAL_STEPS;
	  break;
        case CTK_MOVEMENT_DISPLAY_LINE_ENDS:
	  scroll_step = CTK_SCROLL_HORIZONTAL_ENDS;
	  break;	  
        case CTK_MOVEMENT_DISPLAY_LINES:
          leave_direction = count > 0 ? CTK_DIR_DOWN : CTK_DIR_UP;
          /* fall through */
        case CTK_MOVEMENT_PARAGRAPHS:
        case CTK_MOVEMENT_PARAGRAPH_ENDS:
	  scroll_step = CTK_SCROLL_STEPS;
	  break;
	case CTK_MOVEMENT_PAGES:
	  scroll_step = CTK_SCROLL_PAGES;
	  break;
	case CTK_MOVEMENT_HORIZONTAL_PAGES:
	  scroll_step = CTK_SCROLL_HORIZONTAL_PAGES;
	  break;
	case CTK_MOVEMENT_BUFFER_ENDS:
	  scroll_step = CTK_SCROLL_ENDS;
	  break;
	default:
          scroll_step = CTK_SCROLL_PAGES;
          break;
	}

      old_xpos = ctk_adjustment_get_value (priv->hadjustment);
      old_ypos = ctk_adjustment_get_value (priv->vadjustment);
      ctk_text_view_move_viewport (text_view, scroll_step, count);
      if ((old_xpos == ctk_adjustment_get_target_value (priv->hadjustment) &&
           old_ypos == ctk_adjustment_get_target_value (priv->vadjustment)) &&
          leave_direction != (CtkDirectionType)-1 &&
          !ctk_widget_keynav_failed (CTK_WIDGET (text_view),
                                     leave_direction))
        {
          g_signal_emit_by_name (text_view, "move-focus", leave_direction);
        }

      return;
    }

  ctk_text_view_reset_im_context (text_view);

  if (step == CTK_MOVEMENT_PAGES)
    {
      if (!ctk_text_view_scroll_pages (text_view, count, extend_selection))
        ctk_widget_error_bell (CTK_WIDGET (text_view));

      ctk_text_view_check_cursor_blink (text_view);
      ctk_text_view_pend_cursor_blink (text_view);
      return;
    }
  else if (step == CTK_MOVEMENT_HORIZONTAL_PAGES)
    {
      if (!ctk_text_view_scroll_hpages (text_view, count, extend_selection))
        ctk_widget_error_bell (CTK_WIDGET (text_view));

      ctk_text_view_check_cursor_blink (text_view);
      ctk_text_view_pend_cursor_blink (text_view);
      return;
    }

  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &insert,
                                    ctk_text_buffer_get_insert (get_buffer (text_view)));

  if (! extend_selection)
    {
      gboolean move_forward = count > 0;
      CtkTextIter sel_bound;

      ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &sel_bound,
                                        ctk_text_buffer_get_selection_bound (get_buffer (text_view)));

      if (iter_line_is_rtl (&insert))
        move_forward = !move_forward;

      /* if we move forward, assume the cursor is at the end of the selection;
       * if we move backward, assume the cursor is at the start
       */
      if (move_forward)
        ctk_text_iter_order (&sel_bound, &insert);
      else
        ctk_text_iter_order (&insert, &sel_bound);

      /* if we actually have a selection, just move *to* the beginning/end
       * of the selection and not *from* there on LOGICAL_POSITIONS
       * and VISUAL_POSITIONS movement
       */
      if (! ctk_text_iter_equal (&sel_bound, &insert))
        cancel_selection = TRUE;
    }

  newplace = insert;

  if (step == CTK_MOVEMENT_DISPLAY_LINES)
    ctk_text_view_get_virtual_cursor_pos (text_view, &insert, &cursor_x_pos, NULL);

  switch (step)
    {
    case CTK_MOVEMENT_LOGICAL_POSITIONS:
      if (! cancel_selection)
        ctk_text_iter_forward_visible_cursor_positions (&newplace, count);
      break;

    case CTK_MOVEMENT_VISUAL_POSITIONS:
      if (! cancel_selection)
        ctk_text_layout_move_iter_visually (priv->layout,
                                            &newplace, count);
      break;

    case CTK_MOVEMENT_WORDS:
      if (iter_line_is_rtl (&newplace))
        count *= -1;

      if (count < 0)
        ctk_text_iter_backward_visible_word_starts (&newplace, -count);
      else if (count > 0) 
	{
	  if (!ctk_text_iter_forward_visible_word_ends (&newplace, count))
	    ctk_text_iter_forward_to_line_end (&newplace);
	}
      break;

    case CTK_MOVEMENT_DISPLAY_LINES:
      if (count < 0)
        {
          leave_direction = CTK_DIR_UP;

          if (ctk_text_view_move_iter_by_lines (text_view, &newplace, count))
            ctk_text_layout_move_iter_to_x (priv->layout, &newplace, cursor_x_pos);
          else
            ctk_text_iter_set_line_offset (&newplace, 0);
        }
      if (count > 0)
        {
          leave_direction = CTK_DIR_DOWN;

          if (ctk_text_view_move_iter_by_lines (text_view, &newplace, count))
            ctk_text_layout_move_iter_to_x (priv->layout, &newplace, cursor_x_pos);
          else
            ctk_text_iter_forward_to_line_end (&newplace);
        }
      break;

    case CTK_MOVEMENT_DISPLAY_LINE_ENDS:
      if (count > 1)
        ctk_text_view_move_iter_by_lines (text_view, &newplace, --count);
      else if (count < -1)
        ctk_text_view_move_iter_by_lines (text_view, &newplace, ++count);

      if (count != 0)
        ctk_text_layout_move_iter_to_line_end (priv->layout, &newplace, count);
      break;

    case CTK_MOVEMENT_PARAGRAPHS:
      if (count > 0)
        {
          if (!ctk_text_iter_ends_line (&newplace))
            {
              ctk_text_iter_forward_to_line_end (&newplace);
              --count;
            }
          ctk_text_iter_forward_visible_lines (&newplace, count);
          ctk_text_iter_forward_to_line_end (&newplace);
        }
      else if (count < 0)
        {
          if (ctk_text_iter_get_line_offset (&newplace) > 0)
	    ctk_text_iter_set_line_offset (&newplace, 0);
          ctk_text_iter_forward_visible_lines (&newplace, count);
          ctk_text_iter_set_line_offset (&newplace, 0);
        }
      break;

    case CTK_MOVEMENT_PARAGRAPH_ENDS:
      if (count > 0)
        {
          if (!ctk_text_iter_ends_line (&newplace))
            ctk_text_iter_forward_to_line_end (&newplace);
        }
      else if (count < 0)
        {
          ctk_text_iter_set_line_offset (&newplace, 0);
        }
      break;

    case CTK_MOVEMENT_BUFFER_ENDS:
      if (count > 0)
        ctk_text_buffer_get_end_iter (get_buffer (text_view), &newplace);
      else if (count < 0)
        ctk_text_buffer_get_iter_at_offset (get_buffer (text_view), &newplace, 0);
     break;
      
    default:
      break;
    }

  /* call move_cursor() even if the cursor hasn't moved, since it 
     cancels the selection
  */
  move_cursor (text_view, &newplace, extend_selection);

  if (!ctk_text_iter_equal (&insert, &newplace))
    {
      DV(g_print (G_STRLOC": scrolling onscreen\n"));
      ctk_text_view_scroll_mark_onscreen (text_view,
                                          ctk_text_buffer_get_insert (get_buffer (text_view)));

      if (step == CTK_MOVEMENT_DISPLAY_LINES)
        ctk_text_view_set_virtual_cursor_pos (text_view, cursor_x_pos, -1);
    }
  else if (leave_direction != (CtkDirectionType)-1)
    {
      if (!ctk_widget_keynav_failed (CTK_WIDGET (text_view),
                                     leave_direction))
        {
          g_signal_emit_by_name (text_view, "move-focus", leave_direction);
        }
    }
  else if (! cancel_selection)
    {
      ctk_widget_error_bell (CTK_WIDGET (text_view));
    }

  ctk_text_view_check_cursor_blink (text_view);
  ctk_text_view_pend_cursor_blink (text_view);
}

static void
ctk_text_view_move_viewport (CtkTextView     *text_view,
                             CtkScrollStep    step,
                             gint             count)
{
  CtkAdjustment *adjustment;
  gdouble increment;
  
  switch (step) 
    {
    case CTK_SCROLL_STEPS:
    case CTK_SCROLL_PAGES:
    case CTK_SCROLL_ENDS:
      adjustment = text_view->priv->vadjustment;
      break;
    case CTK_SCROLL_HORIZONTAL_STEPS:
    case CTK_SCROLL_HORIZONTAL_PAGES:
    case CTK_SCROLL_HORIZONTAL_ENDS:
      adjustment = text_view->priv->hadjustment;
      break;
    default:
      adjustment = text_view->priv->vadjustment;
      break;
    }

  switch (step) 
    {
    case CTK_SCROLL_STEPS:
    case CTK_SCROLL_HORIZONTAL_STEPS:
      increment = ctk_adjustment_get_step_increment (adjustment);
      break;
    case CTK_SCROLL_PAGES:
    case CTK_SCROLL_HORIZONTAL_PAGES:
      increment = ctk_adjustment_get_page_increment (adjustment);
      break;
    case CTK_SCROLL_ENDS:
    case CTK_SCROLL_HORIZONTAL_ENDS:
      increment = ctk_adjustment_get_upper (adjustment) - ctk_adjustment_get_lower (adjustment);
      break;
    default:
      increment = 0.0;
      break;
    }

  ctk_adjustment_animate_to_value (adjustment, ctk_adjustment_get_value (adjustment) + count * increment);
}

static void
ctk_text_view_set_anchor (CtkTextView *text_view)
{
  CtkTextIter insert;

  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &insert,
                                    ctk_text_buffer_get_insert (get_buffer (text_view)));

  ctk_text_buffer_create_mark (get_buffer (text_view), "anchor", &insert, TRUE);
}

static gboolean
ctk_text_view_scroll_pages (CtkTextView *text_view,
                            gint         count,
                            gboolean     extend_selection)
{
  CtkTextViewPrivate *priv;
  CtkAdjustment *adjustment;
  gint cursor_x_pos, cursor_y_pos;
  CtkTextMark *insert_mark;
  CtkTextIter old_insert;
  CtkTextIter new_insert;
  CtkTextIter anchor;
  gdouble newval;
  gdouble oldval;
  gint y0, y1;

  priv = text_view->priv;

  g_return_val_if_fail (priv->vadjustment != NULL, FALSE);
  
  adjustment = priv->vadjustment;

  insert_mark = ctk_text_buffer_get_insert (get_buffer (text_view));

  /* Make sure we start from the current cursor position, even
   * if it was offscreen, but don't queue more scrolls if we're
   * already behind.
   */
  if (priv->pending_scroll)
    cancel_pending_scroll (text_view);
  else
    ctk_text_view_scroll_mark_onscreen (text_view, insert_mark);

  /* Validate the region that will be brought into view by the cursor motion
   */
  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view),
                                    &old_insert, insert_mark);

  if (count < 0)
    {
      ctk_text_view_get_first_para_iter (text_view, &anchor);
      y0 = ctk_adjustment_get_page_size (adjustment);
      y1 = ctk_adjustment_get_page_size (adjustment) + count * ctk_adjustment_get_page_increment (adjustment);
    }
  else
    {
      ctk_text_view_get_first_para_iter (text_view, &anchor);
      y0 = count * ctk_adjustment_get_page_increment (adjustment) + ctk_adjustment_get_page_size (adjustment);
      y1 = 0;
    }

  ctk_text_layout_validate_yrange (priv->layout, &anchor, y0, y1);
  /* FIXME do we need to update the adjustment ranges here? */

  new_insert = old_insert;

  if (count < 0 && ctk_adjustment_get_value (adjustment) <= (ctk_adjustment_get_lower (adjustment) + 1e-12))
    {
      /* already at top, just be sure we are at offset 0 */
      ctk_text_buffer_get_start_iter (get_buffer (text_view), &new_insert);
      move_cursor (text_view, &new_insert, extend_selection);
    }
  else if (count > 0 && ctk_adjustment_get_value (adjustment) >= (ctk_adjustment_get_upper (adjustment) - ctk_adjustment_get_page_size (adjustment) - 1e-12))
    {
      /* already at bottom, just be sure we are at the end */
      ctk_text_buffer_get_end_iter (get_buffer (text_view), &new_insert);
      move_cursor (text_view, &new_insert, extend_selection);
    }
  else
    {
      ctk_text_view_get_virtual_cursor_pos (text_view, NULL, &cursor_x_pos, &cursor_y_pos);

      oldval = newval = ctk_adjustment_get_target_value (adjustment);
      newval += count * ctk_adjustment_get_page_increment (adjustment);

      ctk_adjustment_animate_to_value (adjustment, newval);
      cursor_y_pos += newval - oldval;

      ctk_text_layout_get_iter_at_pixel (priv->layout, &new_insert, cursor_x_pos, cursor_y_pos);

      move_cursor (text_view, &new_insert, extend_selection);

      ctk_text_view_set_virtual_cursor_pos (text_view, cursor_x_pos, cursor_y_pos);
    }
  
  /* Adjust to have the cursor _entirely_ onscreen, move_mark_onscreen
   * only guarantees 1 pixel onscreen.
   */
  DV(g_print (G_STRLOC": scrolling onscreen\n"));

  return !ctk_text_iter_equal (&old_insert, &new_insert);
}

static gboolean
ctk_text_view_scroll_hpages (CtkTextView *text_view,
                             gint         count,
                             gboolean     extend_selection)
{
  CtkTextViewPrivate *priv;
  CtkAdjustment *adjustment;
  gint cursor_x_pos, cursor_y_pos;
  CtkTextMark *insert_mark;
  CtkTextIter old_insert;
  CtkTextIter new_insert;
  gdouble newval;
  gdouble oldval;
  gint y, height;

  priv = text_view->priv;

  g_return_val_if_fail (priv->hadjustment != NULL, FALSE);

  adjustment = priv->hadjustment;

  insert_mark = ctk_text_buffer_get_insert (get_buffer (text_view));

  /* Make sure we start from the current cursor position, even
   * if it was offscreen, but don't queue more scrolls if we're
   * already behind.
   */
  if (priv->pending_scroll)
    cancel_pending_scroll (text_view);
  else
    ctk_text_view_scroll_mark_onscreen (text_view, insert_mark);

  /* Validate the line that we're moving within.
   */
  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view),
                                    &old_insert, insert_mark);

  ctk_text_layout_get_line_yrange (priv->layout, &old_insert, &y, &height);
  ctk_text_layout_validate_yrange (priv->layout, &old_insert, y, y + height);
  /* FIXME do we need to update the adjustment ranges here? */

  new_insert = old_insert;

  if (count < 0 && ctk_adjustment_get_value (adjustment) <= (ctk_adjustment_get_lower (adjustment) + 1e-12))
    {
      /* already at far left, just be sure we are at offset 0 */
      ctk_text_iter_set_line_offset (&new_insert, 0);
      move_cursor (text_view, &new_insert, extend_selection);
    }
  else if (count > 0 && ctk_adjustment_get_value (adjustment) >= (ctk_adjustment_get_upper (adjustment) - ctk_adjustment_get_page_size (adjustment) - 1e-12))
    {
      /* already at far right, just be sure we are at the end */
      if (!ctk_text_iter_ends_line (&new_insert))
	  ctk_text_iter_forward_to_line_end (&new_insert);
      move_cursor (text_view, &new_insert, extend_selection);
    }
  else
    {
      ctk_text_view_get_virtual_cursor_pos (text_view, NULL, &cursor_x_pos, &cursor_y_pos);

      oldval = newval = ctk_adjustment_get_target_value (adjustment);
      newval += count * ctk_adjustment_get_page_increment (adjustment);

      ctk_adjustment_animate_to_value (adjustment, newval);
      cursor_x_pos += newval - oldval;

      ctk_text_layout_get_iter_at_pixel (priv->layout, &new_insert, cursor_x_pos, cursor_y_pos);
      move_cursor (text_view, &new_insert, extend_selection);

      ctk_text_view_set_virtual_cursor_pos (text_view, cursor_x_pos, cursor_y_pos);
    }

  /*  FIXME for lines shorter than the overall widget width, this results in a
   *  "bounce" effect as we scroll to the right of the widget, then scroll
   *  back to get the end of the line onscreen.
   *      http://bugzilla.gnome.org/show_bug.cgi?id=68963
   */
  
  /* Adjust to have the cursor _entirely_ onscreen, move_mark_onscreen
   * only guarantees 1 pixel onscreen.
   */
  DV(g_print (G_STRLOC": scrolling onscreen\n"));

  return !ctk_text_iter_equal (&old_insert, &new_insert);
}

static gboolean
whitespace (gunichar ch, gpointer user_data)
{
  return (ch == ' ' || ch == '\t');
}

static gboolean
not_whitespace (gunichar ch, gpointer user_data)
{
  return !whitespace (ch, user_data);
}

static gboolean
find_whitepace_region (const CtkTextIter *center,
                       CtkTextIter *start, CtkTextIter *end)
{
  *start = *center;
  *end = *center;

  if (ctk_text_iter_backward_find_char (start, not_whitespace, NULL, NULL))
    ctk_text_iter_forward_char (start); /* we want the first whitespace... */
  if (whitespace (ctk_text_iter_get_char (end), NULL))
    ctk_text_iter_forward_find_char (end, not_whitespace, NULL, NULL);

  return !ctk_text_iter_equal (start, end);
}

static void
ctk_text_view_insert_at_cursor (CtkTextView *text_view,
                                const gchar *str)
{
  if (!ctk_text_buffer_insert_interactive_at_cursor (get_buffer (text_view), str, -1,
                                                     text_view->priv->editable))
    {
      ctk_widget_error_bell (CTK_WIDGET (text_view));
    }
}

static void
ctk_text_view_delete_from_cursor (CtkTextView   *text_view,
                                  CtkDeleteType  type,
                                  gint           count)
{
  CtkTextViewPrivate *priv;
  CtkTextIter insert;
  CtkTextIter start;
  CtkTextIter end;
  gboolean leave_one = FALSE;

  priv = text_view->priv;

  ctk_text_view_reset_im_context (text_view);

  if (type == CTK_DELETE_CHARS)
    {
      /* Char delete deletes the selection, if one exists */
      if (ctk_text_buffer_delete_selection (get_buffer (text_view), TRUE,
                                            priv->editable))
        return;
    }

  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &insert,
                                    ctk_text_buffer_get_insert (get_buffer (text_view)));

  start = insert;
  end = insert;

  switch (type)
    {
    case CTK_DELETE_CHARS:
      ctk_text_iter_forward_cursor_positions (&end, count);
      break;

    case CTK_DELETE_WORD_ENDS:
      if (count > 0)
        ctk_text_iter_forward_word_ends (&end, count);
      else if (count < 0)
        ctk_text_iter_backward_word_starts (&start, 0 - count);
      break;

    case CTK_DELETE_WORDS:
      break;

    case CTK_DELETE_DISPLAY_LINE_ENDS:
      break;

    case CTK_DELETE_DISPLAY_LINES:
      break;

    case CTK_DELETE_PARAGRAPH_ENDS:
      if (count > 0)
        {
          /* If we're already at a newline, we need to
           * simply delete that newline, instead of
           * moving to the next one.
           */
          if (ctk_text_iter_ends_line (&end))
            {
              ctk_text_iter_forward_line (&end);
              --count;
            }

          while (count > 0)
            {
              if (!ctk_text_iter_forward_to_line_end (&end))
                break;

              --count;
            }
        }
      else if (count < 0)
        {
          if (ctk_text_iter_starts_line (&start))
            {
              ctk_text_iter_backward_line (&start);
              if (!ctk_text_iter_ends_line (&end))
                ctk_text_iter_forward_to_line_end (&start);
            }
          else
            {
              ctk_text_iter_set_line_offset (&start, 0);
            }
          ++count;

          ctk_text_iter_backward_lines (&start, -count);
        }
      break;

    case CTK_DELETE_PARAGRAPHS:
      if (count > 0)
        {
          ctk_text_iter_set_line_offset (&start, 0);
          ctk_text_iter_forward_to_line_end (&end);

          /* Do the lines beyond the first. */
          while (count > 1)
            {
              ctk_text_iter_forward_to_line_end (&end);

              --count;
            }
        }

      /* FIXME negative count? */

      break;

    case CTK_DELETE_WHITESPACE:
      {
        find_whitepace_region (&insert, &start, &end);
      }
      break;

    default:
      break;
    }

  if (!ctk_text_iter_equal (&start, &end))
    {
      ctk_text_buffer_begin_user_action (get_buffer (text_view));

      if (ctk_text_buffer_delete_interactive (get_buffer (text_view), &start, &end,
                                              priv->editable))
        {
          if (leave_one)
            ctk_text_buffer_insert_interactive_at_cursor (get_buffer (text_view),
                                                          " ", 1,
                                                          priv->editable);
        }
      else
        {
          ctk_widget_error_bell (CTK_WIDGET (text_view));
        }

      ctk_text_buffer_end_user_action (get_buffer (text_view));
      ctk_text_view_set_virtual_cursor_pos (text_view, -1, -1);

      DV(g_print (G_STRLOC": scrolling onscreen\n"));
      ctk_text_view_scroll_mark_onscreen (text_view,
                                          ctk_text_buffer_get_insert (get_buffer (text_view)));
    }
  else
    {
      ctk_widget_error_bell (CTK_WIDGET (text_view));
    }
}

static void
ctk_text_view_backspace (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv;
  CtkTextIter insert;

  priv = text_view->priv;

  ctk_text_view_reset_im_context (text_view);

  /* Backspace deletes the selection, if one exists */
  if (ctk_text_buffer_delete_selection (get_buffer (text_view), TRUE,
                                        priv->editable))
    return;

  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view),
                                    &insert,
                                    ctk_text_buffer_get_insert (get_buffer (text_view)));

  if (ctk_text_buffer_backspace (get_buffer (text_view), &insert,
				 TRUE, priv->editable))
    {
      ctk_text_view_set_virtual_cursor_pos (text_view, -1, -1);
      DV(g_print (G_STRLOC": scrolling onscreen\n"));
      ctk_text_view_scroll_mark_onscreen (text_view,
                                          ctk_text_buffer_get_insert (get_buffer (text_view)));
    }
  else
    {
      ctk_widget_error_bell (CTK_WIDGET (text_view));
    }
}

static void
ctk_text_view_cut_clipboard (CtkTextView *text_view)
{
  CtkClipboard *clipboard = ctk_widget_get_clipboard (CTK_WIDGET (text_view),
						      CDK_SELECTION_CLIPBOARD);
  
  ctk_text_buffer_cut_clipboard (get_buffer (text_view),
				 clipboard,
				 text_view->priv->editable);
  DV(g_print (G_STRLOC": scrolling onscreen\n"));
  ctk_text_view_scroll_mark_onscreen (text_view,
                                      ctk_text_buffer_get_insert (get_buffer (text_view)));
  ctk_text_view_selection_bubble_popup_unset (text_view);
}

static void
ctk_text_view_copy_clipboard (CtkTextView *text_view)
{
  CtkClipboard *clipboard = ctk_widget_get_clipboard (CTK_WIDGET (text_view),
						      CDK_SELECTION_CLIPBOARD);
  
  ctk_text_buffer_copy_clipboard (get_buffer (text_view),
				  clipboard);

  /* on copy do not scroll, we are already onscreen */
}

static void
ctk_text_view_paste_clipboard (CtkTextView *text_view)
{
  CtkClipboard *clipboard = ctk_widget_get_clipboard (CTK_WIDGET (text_view),
						      CDK_SELECTION_CLIPBOARD);
  
  text_view->priv->scroll_after_paste = TRUE;

  ctk_text_buffer_paste_clipboard (get_buffer (text_view),
				   clipboard,
				   NULL,
				   text_view->priv->editable);
}

static void
ctk_text_view_paste_done_handler (CtkTextBuffer *buffer,
                                  CtkClipboard  *clipboard,
                                  gpointer       data)
{
  CtkTextView *text_view = data;
  CtkTextViewPrivate *priv;

  priv = text_view->priv;

  if (priv->scroll_after_paste)
    {
      DV(g_print (G_STRLOC": scrolling onscreen\n"));
      ctk_text_view_scroll_mark_onscreen (text_view, ctk_text_buffer_get_insert (buffer));
    }

  priv->scroll_after_paste = FALSE;
}

static void
ctk_text_view_buffer_changed_handler (CtkTextBuffer *buffer,
                                      gpointer       data)
{
  CtkTextView *text_view = data;
  CtkTextViewPrivate *priv = text_view->priv;

  if (priv->handling_key_event)
    ctk_text_view_obscure_mouse_cursor (text_view);

  if (priv->text_handle)
    ctk_text_view_update_handles (text_view,
                                  _ctk_text_handle_get_mode (priv->text_handle));
}

static void
ctk_text_view_toggle_overwrite (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv = text_view->priv;

  if (priv->text_window)
    text_window_invalidate_cursors (priv->text_window);

  priv->overwrite_mode = !priv->overwrite_mode;

  if (priv->layout)
    ctk_text_layout_set_overwrite_mode (priv->layout,
					priv->overwrite_mode && priv->editable);

  if (priv->text_window)
    text_window_invalidate_cursors (priv->text_window);

  ctk_text_view_pend_cursor_blink (text_view);

  g_object_notify (G_OBJECT (text_view), "overwrite");
}

/**
 * ctk_text_view_get_overwrite:
 * @text_view: a #CtkTextView
 *
 * Returns whether the #CtkTextView is in overwrite mode or not.
 *
 * Returns: whether @text_view is in overwrite mode or not.
 * 
 * Since: 2.4
 **/
gboolean
ctk_text_view_get_overwrite (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);

  return text_view->priv->overwrite_mode;
}

/**
 * ctk_text_view_set_overwrite:
 * @text_view: a #CtkTextView
 * @overwrite: %TRUE to turn on overwrite mode, %FALSE to turn it off
 *
 * Changes the #CtkTextView overwrite mode.
 *
 * Since: 2.4
 **/
void
ctk_text_view_set_overwrite (CtkTextView *text_view,
			     gboolean     overwrite)
{
  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));
  overwrite = overwrite != FALSE;

  if (text_view->priv->overwrite_mode != overwrite)
    ctk_text_view_toggle_overwrite (text_view);
}

/**
 * ctk_text_view_set_accepts_tab:
 * @text_view: A #CtkTextView
 * @accepts_tab: %TRUE if pressing the Tab key should insert a tab 
 *    character, %FALSE, if pressing the Tab key should move the 
 *    keyboard focus.
 * 
 * Sets the behavior of the text widget when the Tab key is pressed. 
 * If @accepts_tab is %TRUE, a tab character is inserted. If @accepts_tab 
 * is %FALSE the keyboard focus is moved to the next widget in the focus 
 * chain.
 * 
 * Since: 2.4
 **/
void
ctk_text_view_set_accepts_tab (CtkTextView *text_view,
			       gboolean     accepts_tab)
{
  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  accepts_tab = accepts_tab != FALSE;

  if (text_view->priv->accepts_tab != accepts_tab)
    {
      text_view->priv->accepts_tab = accepts_tab;

      g_object_notify (G_OBJECT (text_view), "accepts-tab");
    }
}

/**
 * ctk_text_view_get_accepts_tab:
 * @text_view: A #CtkTextView
 * 
 * Returns whether pressing the Tab key inserts a tab characters.
 * ctk_text_view_set_accepts_tab().
 * 
 * Returns: %TRUE if pressing the Tab key inserts a tab character, 
 *   %FALSE if pressing the Tab key moves the keyboard focus.
 * 
 * Since: 2.4
 **/
gboolean
ctk_text_view_get_accepts_tab (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);

  return text_view->priv->accepts_tab;
}

/*
 * Selections
 */

static void
ctk_text_view_unselect (CtkTextView *text_view)
{
  CtkTextIter insert;

  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &insert,
                                    ctk_text_buffer_get_insert (get_buffer (text_view)));

  ctk_text_buffer_move_mark (get_buffer (text_view),
                             ctk_text_buffer_get_selection_bound (get_buffer (text_view)),
                             &insert);
}

static void
move_mark_to_pointer_and_scroll (CtkTextView    *text_view,
                                 const gchar    *mark_name)
{
  CtkTextIter newplace;
  CtkTextBuffer *buffer;
  CtkTextMark *mark;

  buffer = get_buffer (text_view);
  get_iter_from_gesture (text_view, text_view->priv->drag_gesture,
                         &newplace, NULL, NULL);

  mark = ctk_text_buffer_get_mark (buffer, mark_name);

  /* This may invalidate the layout */
  DV(g_print (G_STRLOC": move mark\n"));

  ctk_text_buffer_move_mark (buffer, mark, &newplace);

  DV(g_print (G_STRLOC": scrolling onscreen\n"));
  ctk_text_view_scroll_mark_onscreen (text_view, mark);

  DV (g_print ("first validate idle leaving %s is %d\n",
               G_STRLOC, text_view->priv->first_validate_idle));
}

static gboolean
selection_scan_timeout (gpointer data)
{
  CtkTextView *text_view;

  text_view = CTK_TEXT_VIEW (data);

  ctk_text_view_scroll_mark_onscreen (text_view, 
				      ctk_text_buffer_get_insert (get_buffer (text_view)));

  return TRUE; /* remain installed. */
}

#define UPPER_OFFSET_ANCHOR 0.8
#define LOWER_OFFSET_ANCHOR 0.2

static gboolean
check_scroll (gdouble offset, CtkAdjustment *adjustment)
{
  if ((offset > UPPER_OFFSET_ANCHOR &&
       ctk_adjustment_get_value (adjustment) + ctk_adjustment_get_page_size (adjustment) < ctk_adjustment_get_upper (adjustment)) ||
      (offset < LOWER_OFFSET_ANCHOR &&
       ctk_adjustment_get_value (adjustment) > ctk_adjustment_get_lower (adjustment)))
    return TRUE;

  return FALSE;
}

static gint
drag_scan_timeout (gpointer data)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  CtkTextIter newplace;
  gdouble pointer_xoffset, pointer_yoffset;

  text_view = CTK_TEXT_VIEW (data);
  priv = text_view->priv;

  ctk_text_layout_get_iter_at_pixel (priv->layout,
                                     &newplace,
                                     priv->dnd_x + priv->xoffset,
                                     priv->dnd_y + priv->yoffset);

  ctk_text_buffer_move_mark (get_buffer (text_view),
                             priv->dnd_mark,
                             &newplace);

  pointer_xoffset = (gdouble) priv->dnd_x / cdk_window_get_width (priv->text_window->bin_window);
  pointer_yoffset = (gdouble) priv->dnd_y / cdk_window_get_height (priv->text_window->bin_window);

  if (check_scroll (pointer_xoffset, priv->hadjustment) ||
      check_scroll (pointer_yoffset, priv->vadjustment))
    {
      /* do not make offsets surpass lower nor upper anchors, this makes
       * scrolling speed relative to the distance of the pointer to the
       * anchors when it moves beyond them.
       */
      pointer_xoffset = CLAMP (pointer_xoffset, LOWER_OFFSET_ANCHOR, UPPER_OFFSET_ANCHOR);
      pointer_yoffset = CLAMP (pointer_yoffset, LOWER_OFFSET_ANCHOR, UPPER_OFFSET_ANCHOR);

      ctk_text_view_scroll_to_mark (text_view,
                                    priv->dnd_mark,
                                    0., TRUE, pointer_xoffset, pointer_yoffset);
    }

  return TRUE;
}

static void
extend_selection (CtkTextView          *text_view,
                  SelectionGranularity  granularity,
                  const CtkTextIter    *location,
                  CtkTextIter          *start,
                  CtkTextIter          *end)
{
  CtkTextExtendSelection extend_selection_granularity;
  gboolean handled = FALSE;

  switch (granularity)
    {
    case SELECT_CHARACTERS:
      *start = *location;
      *end = *location;
      return;

    case SELECT_WORDS:
      extend_selection_granularity = CTK_TEXT_EXTEND_SELECTION_WORD;
      break;

    case SELECT_LINES:
      extend_selection_granularity = CTK_TEXT_EXTEND_SELECTION_LINE;
      break;

    default:
      g_assert_not_reached ();
    }

  g_signal_emit (text_view,
                 signals[EXTEND_SELECTION], 0,
                 extend_selection_granularity,
                 location,
                 start,
                 end,
                 &handled);

  if (!handled)
    {
      *start = *location;
      *end = *location;
    }
}

static gboolean
ctk_text_view_extend_selection (CtkTextView            *text_view,
                                CtkTextExtendSelection  granularity,
                                const CtkTextIter      *location,
                                CtkTextIter            *start,
                                CtkTextIter            *end)
{
  *start = *location;
  *end = *location;

  switch (granularity)
    {
    case CTK_TEXT_EXTEND_SELECTION_WORD:
      if (ctk_text_iter_inside_word (start))
	{
	  if (!ctk_text_iter_starts_word (start))
	    ctk_text_iter_backward_visible_word_start (start);

	  if (!ctk_text_iter_ends_word (end))
	    {
	      if (!ctk_text_iter_forward_visible_word_end (end))
		ctk_text_iter_forward_to_end (end);
	    }
	}
      else
	{
	  CtkTextIter tmp;

          /* @start is not contained in a word: the selection is extended to all
           * the white spaces between the end of the word preceding @start and
           * the start of the one following.
           */

	  tmp = *start;
	  if (ctk_text_iter_backward_visible_word_start (&tmp))
	    ctk_text_iter_forward_visible_word_end (&tmp);

	  if (ctk_text_iter_get_line (&tmp) == ctk_text_iter_get_line (start))
	    *start = tmp;
	  else
	    ctk_text_iter_set_line_offset (start, 0);

	  tmp = *end;
	  if (!ctk_text_iter_forward_visible_word_end (&tmp))
	    ctk_text_iter_forward_to_end (&tmp);

	  if (ctk_text_iter_ends_word (&tmp))
	    ctk_text_iter_backward_visible_word_start (&tmp);

	  if (ctk_text_iter_get_line (&tmp) == ctk_text_iter_get_line (end))
	    *end = tmp;
	  else
	    ctk_text_iter_forward_to_line_end (end);
	}
      break;

    case CTK_TEXT_EXTEND_SELECTION_LINE:
      if (ctk_text_view_starts_display_line (text_view, start))
	{
	  /* If on a display line boundary, we assume the user
	   * clicked off the end of a line and we therefore select
	   * the line before the boundary.
	   */
	  ctk_text_view_backward_display_line_start (text_view, start);
	}
      else
	{
	  /* start isn't on the start of a line, so we move it to the
	   * start, and move end to the end unless it's already there.
	   */
	  ctk_text_view_backward_display_line_start (text_view, start);

	  if (!ctk_text_view_starts_display_line (text_view, end))
	    ctk_text_view_forward_display_line_end (text_view, end);
	}
      break;

    default:
      g_return_val_if_reached (CDK_EVENT_STOP);
    }

  return CDK_EVENT_STOP;
}

typedef struct
{
  SelectionGranularity granularity;
  CtkTextMark *orig_start;
  CtkTextMark *orig_end;
  CtkTextBuffer *buffer;
} SelectionData;

static void
selection_data_free (SelectionData *data)
{
  if (data->orig_start != NULL)
    ctk_text_buffer_delete_mark (data->buffer, data->orig_start);

  if (data->orig_end != NULL)
    ctk_text_buffer_delete_mark (data->buffer, data->orig_end);

  g_object_unref (data->buffer);

  g_slice_free (SelectionData, data);
}

static gboolean
drag_gesture_get_text_window_coords (CtkGestureDrag *gesture,
                                     CtkTextView    *text_view,
                                     gint           *start_x,
                                     gint           *start_y,
                                     gint           *x,
                                     gint           *y)
{
  gdouble sx, sy, ox, oy;

  if (!ctk_gesture_drag_get_start_point (gesture, &sx, &sy) ||
      !ctk_gesture_drag_get_offset (gesture, &ox, &oy))
    return FALSE;

  *start_x = sx;
  *start_y = sy;
  _widget_to_text_window_coords (text_view, start_x, start_y);

  *x = sx + ox;
  *y = sy + oy;
  _widget_to_text_window_coords (text_view, x, y);

  return TRUE;
}

static void
ctk_text_view_drag_gesture_update (CtkGestureDrag *gesture,
                                   gdouble         offset_x,
                                   gdouble         offset_y,
                                   CtkTextView    *text_view)
{
  gint start_x, start_y, x, y;
  CdkEventSequence *sequence;
  gboolean is_touchscreen;
  const CdkEvent *event;
  SelectionData *data;
  CdkDevice *device;
  CtkTextIter cursor;

  data = g_object_get_qdata (G_OBJECT (gesture), quark_text_selection_data);
  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);
  drag_gesture_get_text_window_coords (gesture, text_view,
                                       &start_x, &start_y, &x, &y);

  device = cdk_event_get_source_device (event);

  is_touchscreen = ctk_simulate_touchscreen () ||
                   cdk_device_get_source (device) == CDK_SOURCE_TOUCHSCREEN;

  get_iter_from_gesture (text_view, text_view->priv->drag_gesture,
                         &cursor, NULL, NULL);

  if (!data)
    {
      /* If no data is attached, the initial press happened within the current
       * text selection, check for drag and drop to be initiated.
       */
      if (ctk_drag_check_threshold (CTK_WIDGET (text_view),
				    start_x, start_y, x, y))
        {
          if (!is_touchscreen)
            {
              CtkTextIter iter;
              gint buffer_x, buffer_y;

              ctk_text_view_window_to_buffer_coords (text_view,
                                                     CTK_TEXT_WINDOW_TEXT,
                                                     start_x, start_y,
                                                     &buffer_x,
                                                     &buffer_y);

              ctk_text_layout_get_iter_at_pixel (text_view->priv->layout,
                                                 &iter, buffer_x, buffer_y);

              ctk_text_view_start_selection_dnd (text_view, &iter, event,
                                                 start_x, start_y);
              return;
            }
          else
            {
              ctk_text_view_start_selection_drag (text_view, &cursor,
                                                  SELECT_WORDS, TRUE);
              data = g_object_get_qdata (G_OBJECT (gesture), quark_text_selection_data);
            }
        }
      else
        return;
    }

  /* Text selection */
  if (data->granularity == SELECT_CHARACTERS)
    {
      move_mark_to_pointer_and_scroll (text_view, "insert");
    }
  else
    {
      CtkTextIter start, end;
      CtkTextIter orig_start, orig_end;
      CtkTextBuffer *buffer;

      buffer = get_buffer (text_view);

      ctk_text_buffer_get_iter_at_mark (buffer, &orig_start, data->orig_start);
      ctk_text_buffer_get_iter_at_mark (buffer, &orig_end, data->orig_end);

      get_iter_from_gesture (text_view, text_view->priv->drag_gesture,
                             &cursor, NULL, NULL);

      extend_selection (text_view, data->granularity, &cursor, &start, &end);

      /* either the selection extends to the front, or end (or not) */
      if (ctk_text_iter_compare (&orig_start, &start) < 0)
        start = orig_start;
      if (ctk_text_iter_compare (&orig_end, &end) > 0)
        end = orig_end;
      ctk_text_buffer_select_range (buffer, &start, &end);

      ctk_text_view_scroll_mark_onscreen (text_view,
					  ctk_text_buffer_get_insert (buffer));
    }

  /* If we had to scroll offscreen, insert a timeout to do so
   * again. Note that in the timeout, even if the mouse doesn't
   * move, due to this scroll xoffset/yoffset will have changed
   * and we'll need to scroll again.
   */
  if (text_view->priv->scroll_timeout != 0) /* reset on every motion event */
    g_source_remove (text_view->priv->scroll_timeout);

  text_view->priv->scroll_timeout =
    cdk_threads_add_timeout (50, selection_scan_timeout, text_view);
  g_source_set_name_by_id (text_view->priv->scroll_timeout, "[ctk+] selection_scan_timeout");

  ctk_text_view_selection_bubble_popup_unset (text_view);

  if (is_touchscreen)
    {
      _ctk_text_view_ensure_text_handles (text_view);
      ctk_text_view_update_handles (text_view, CTK_TEXT_HANDLE_MODE_SELECTION);
      ctk_text_view_show_magnifier (text_view, &cursor, x, y);
    }
}

static void
ctk_text_view_drag_gesture_end (CtkGestureDrag *gesture,
                                gdouble         offset_x,
                                gdouble         offset_y,
                                CtkTextView    *text_view)
{
  gboolean is_touchscreen, clicked_in_selection;
  gint start_x, start_y, x, y;
  CdkEventSequence *sequence;
  CtkTextViewPrivate *priv;
  const CdkEvent *event;
  CdkDevice *device;

  priv = text_view->priv;
  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  drag_gesture_get_text_window_coords (gesture, text_view,
                                       &start_x, &start_y, &x, &y);

  clicked_in_selection =
    g_object_get_qdata (G_OBJECT (gesture), quark_text_selection_data) == NULL;
  g_object_set_qdata (G_OBJECT (gesture), quark_text_selection_data, NULL);
  ctk_text_view_unobscure_mouse_cursor (text_view);

  if (priv->scroll_timeout != 0)
    {
      g_source_remove (priv->scroll_timeout);
      priv->scroll_timeout = 0;
    }

  if (priv->magnifier_popover)
    ctk_widget_hide (priv->magnifier_popover);

  /* Check whether the drag was cancelled rather than finished */
  if (!ctk_gesture_handles_sequence (CTK_GESTURE (gesture), sequence))
    return;

  event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);
  device = cdk_event_get_source_device (event);
  is_touchscreen = ctk_simulate_touchscreen () ||
    cdk_device_get_source (device) == CDK_SOURCE_TOUCHSCREEN;

  if (!is_touchscreen && clicked_in_selection &&
      !ctk_drag_check_threshold (CTK_WIDGET (text_view), start_x, start_y, x, y))
    {
      CtkTextHandleMode mode = CTK_TEXT_HANDLE_MODE_NONE;
      CtkTextIter iter;

      /* Unselect everything; we clicked inside selection, but
       * didn't move by the drag threshold, so just clear selection
       * and place cursor.
       */
      ctk_text_layout_get_iter_at_pixel (priv->layout, &iter,
                                         x + priv->xoffset, y + priv->yoffset);

      ctk_text_buffer_place_cursor (get_buffer (text_view), &iter);
      ctk_text_view_check_cursor_blink (text_view);

      if (priv->text_handle)
        {
          if (is_touchscreen)
            mode = CTK_TEXT_HANDLE_MODE_CURSOR;

          ctk_text_view_update_handles (text_view, mode);
        }
    }
}

static void
ctk_text_view_start_selection_drag (CtkTextView          *text_view,
                                    const CtkTextIter    *iter,
                                    SelectionGranularity  granularity,
                                    gboolean              extend)
{
  CtkTextViewPrivate *priv;
  CtkTextIter cursor, ins, bound;
  CtkTextIter orig_start, orig_end;
  CtkTextBuffer *buffer;
  SelectionData *data;

  priv = text_view->priv;
  data = g_slice_new0 (SelectionData);
  data->granularity = granularity;

  buffer = get_buffer (text_view);

  cursor = *iter;
  extend_selection (text_view, data->granularity, &cursor, &ins, &bound);

  orig_start = ins;
  orig_end = bound;

  if (extend)
    {
      /* Extend selection */
      CtkTextIter old_ins, old_bound;
      CtkTextIter old_start, old_end;

      ctk_text_buffer_get_iter_at_mark (buffer, &old_ins, ctk_text_buffer_get_insert (buffer));
      ctk_text_buffer_get_iter_at_mark (buffer, &old_bound, ctk_text_buffer_get_selection_bound (buffer));
      old_start = old_ins;
      old_end = old_bound;
      ctk_text_iter_order (&old_start, &old_end);
      
      /* move the front cursor, if the mouse is in front of the selection. Should the
       * cursor however be inside the selection (this happens on tripple click) then we
       * move the side which was last moved (current insert mark) */
      if (ctk_text_iter_compare (&cursor, &old_start) <= 0 ||
          (ctk_text_iter_compare (&cursor, &old_end) < 0 && 
           ctk_text_iter_compare (&old_ins, &old_bound) <= 0))
        {
          bound = old_end;
        }
      else
        {
          ins = bound;
          bound = old_start;
        }

      /* Store any previous selection */
      if (ctk_text_iter_compare (&old_start, &old_end) != 0)
        {
          orig_start = old_ins;
          orig_end = old_bound;
        }
    }

  ctk_text_buffer_select_range (buffer, &ins, &bound);

  ctk_text_iter_order (&orig_start, &orig_end);
  data->orig_start = ctk_text_buffer_create_mark (buffer, NULL,
                                                  &orig_start, TRUE);
  data->orig_end = ctk_text_buffer_create_mark (buffer, NULL,
                                                &orig_end, TRUE);
  data->buffer = g_object_ref (buffer);
  ctk_text_view_check_cursor_blink (text_view);

  g_object_set_qdata_full (G_OBJECT (priv->drag_gesture),
                           quark_text_selection_data,
                           data, (GDestroyNotify) selection_data_free);
  ctk_gesture_set_state (priv->drag_gesture,
                         CTK_EVENT_SEQUENCE_CLAIMED);
}

/* returns whether we were really dragging */
static gboolean
ctk_text_view_end_selection_drag (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv;

  priv = text_view->priv;

  if (!ctk_gesture_is_active (priv->drag_gesture))
    return FALSE;

  if (priv->scroll_timeout != 0)
    {
      g_source_remove (priv->scroll_timeout);
      priv->scroll_timeout = 0;
    }

  if (priv->magnifier_popover)
    ctk_widget_hide (priv->magnifier_popover);

  return TRUE;
}

/*
 * Layout utils
 */

static void
ctk_text_view_set_attributes_from_style (CtkTextView        *text_view,
                                         CtkTextAttributes  *values)
{
  CtkStyleContext *context;
  CdkRGBA bg_color, fg_color;
  CtkStateFlags state;

  context = ctk_widget_get_style_context (CTK_WIDGET (text_view));
  state = ctk_style_context_get_state (context);

  ctk_style_context_get_background_color (context, state, &bg_color);
  ctk_style_context_get_color (context, state, &fg_color);

  values->appearance.bg_color.red = CLAMP (bg_color.red * 65535. + 0.5, 0, 65535);
  values->appearance.bg_color.green = CLAMP (bg_color.green * 65535. + 0.5, 0, 65535);
  values->appearance.bg_color.blue = CLAMP (bg_color.blue * 65535. + 0.5, 0, 65535);

  values->appearance.fg_color.red = CLAMP (fg_color.red * 65535. + 0.5, 0, 65535);
  values->appearance.fg_color.green = CLAMP (fg_color.green * 65535. + 0.5, 0, 65535);
  values->appearance.fg_color.blue = CLAMP (fg_color.blue * 65535. + 0.5, 0, 65535);

  if (values->font)
    pango_font_description_free (values->font);

  ctk_style_context_get (context, state, "font", &values->font, NULL);
}

static void
ctk_text_view_check_keymap_direction (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv = text_view->priv;

  if (priv->layout)
    {
      CtkSettings *settings = ctk_widget_get_settings (CTK_WIDGET (text_view));
      CdkKeymap *keymap = cdk_keymap_get_for_display (ctk_widget_get_display (CTK_WIDGET (text_view)));
      CtkTextDirection new_cursor_dir;
      CtkTextDirection new_keyboard_dir;
      gboolean split_cursor;

      g_object_get (settings,
		    "ctk-split-cursor", &split_cursor,
		    NULL);
      
      if (cdk_keymap_get_direction (keymap) == PANGO_DIRECTION_RTL)
	new_keyboard_dir = CTK_TEXT_DIR_RTL;
      else
	new_keyboard_dir  = CTK_TEXT_DIR_LTR;
  
      if (split_cursor)
	new_cursor_dir = CTK_TEXT_DIR_NONE;
      else
	new_cursor_dir = new_keyboard_dir;
      
      ctk_text_layout_set_cursor_direction (priv->layout, new_cursor_dir);
      ctk_text_layout_set_keyboard_direction (priv->layout, new_keyboard_dir);
    }
}

static void
ctk_text_view_ensure_layout (CtkTextView *text_view)
{
  CtkWidget *widget;
  CtkTextViewPrivate *priv;

  widget = CTK_WIDGET (text_view);
  priv = text_view->priv;

  if (priv->layout == NULL)
    {
      CtkTextAttributes *style;
      PangoContext *ltr_context, *rtl_context;
      GSList *tmp_list;

      DV(g_print(G_STRLOC"\n"));
      
      priv->layout = ctk_text_layout_new ();

      g_signal_connect (priv->layout,
			"invalidated",
			G_CALLBACK (invalidated_handler),
			text_view);

      g_signal_connect (priv->layout,
			"changed",
			G_CALLBACK (changed_handler),
			text_view);

      g_signal_connect (priv->layout,
			"allocate-child",
			G_CALLBACK (ctk_text_view_child_allocated),
			text_view);
      
      if (get_buffer (text_view))
        ctk_text_layout_set_buffer (priv->layout, get_buffer (text_view));

      if ((ctk_widget_has_focus (widget) && cursor_visible (text_view)))
        ctk_text_view_pend_cursor_blink (text_view);
      else
        ctk_text_layout_set_cursor_visible (priv->layout, FALSE);

      ctk_text_layout_set_overwrite_mode (priv->layout,
					  priv->overwrite_mode && priv->editable);

      ltr_context = ctk_widget_create_pango_context (CTK_WIDGET (text_view));
      pango_context_set_base_dir (ltr_context, PANGO_DIRECTION_LTR);
      rtl_context = ctk_widget_create_pango_context (CTK_WIDGET (text_view));
      pango_context_set_base_dir (rtl_context, PANGO_DIRECTION_RTL);

      ctk_text_layout_set_contexts (priv->layout, ltr_context, rtl_context);

      g_object_unref (ltr_context);
      g_object_unref (rtl_context);

      ctk_text_view_check_keymap_direction (text_view);

      style = ctk_text_attributes_new ();

      ctk_text_view_set_attributes_from_style (text_view, style);

      style->pixels_above_lines = priv->pixels_above_lines;
      style->pixels_below_lines = priv->pixels_below_lines;
      style->pixels_inside_wrap = priv->pixels_inside_wrap;

      style->left_margin = priv->left_margin;
      style->right_margin = priv->right_margin;
      priv->layout->right_padding = priv->right_padding;
      priv->layout->left_padding = priv->left_padding;

      style->indent = priv->indent;
      style->tabs = priv->tabs ? pango_tab_array_copy (priv->tabs) : NULL;

      style->wrap_mode = priv->wrap_mode;
      style->justification = priv->justify;
      style->direction = ctk_widget_get_direction (CTK_WIDGET (text_view));

      ctk_text_layout_set_default_style (priv->layout, style);

      ctk_text_attributes_unref (style);

      /* Set layout for all anchored children */

      tmp_list = priv->children;
      while (tmp_list != NULL)
        {
          CtkTextViewChild *vc = tmp_list->data;

          if (vc->anchor)
            {
              ctk_text_anchored_child_set_layout (vc->widget,
                                                  priv->layout);
              /* vc may now be invalid! */
            }

          tmp_list = tmp_list->next;
        }
    }
}

/**
 * ctk_text_view_get_default_attributes:
 * @text_view: a #CtkTextView
 * 
 * Obtains a copy of the default text attributes. These are the
 * attributes used for text unless a tag overrides them.
 * You’d typically pass the default attributes in to
 * ctk_text_iter_get_attributes() in order to get the
 * attributes in effect at a given text position.
 *
 * The return value is a copy owned by the caller of this function,
 * and should be freed with ctk_text_attributes_unref().
 * 
 * Returns: a new #CtkTextAttributes
 **/
CtkTextAttributes*
ctk_text_view_get_default_attributes (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), NULL);
  
  ctk_text_view_ensure_layout (text_view);

  return ctk_text_attributes_copy (text_view->priv->layout->default_style);
}

static void
ctk_text_view_destroy_layout (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv = text_view->priv;

  if (priv->layout)
    {
      GSList *tmp_list;

      ctk_text_view_remove_validate_idles (text_view);

      g_signal_handlers_disconnect_by_func (priv->layout,
					    invalidated_handler,
					    text_view);
      g_signal_handlers_disconnect_by_func (priv->layout,
					    changed_handler,
					    text_view);

      /* Remove layout from all anchored children */
      tmp_list = priv->children;
      while (tmp_list != NULL)
        {
          CtkTextViewChild *vc = tmp_list->data;

          if (vc->anchor)
            {
              ctk_text_anchored_child_set_layout (vc->widget, NULL);
              /* vc may now be invalid! */
            }

          tmp_list = tmp_list->next;
        }

      ctk_text_view_stop_cursor_blink (text_view);
      ctk_text_view_end_selection_drag (text_view);

      g_object_unref (priv->layout);
      priv->layout = NULL;
    }
}

/**
 * ctk_text_view_reset_im_context:
 * @text_view: a #CtkTextView
 *
 * Reset the input method context of the text view if needed.
 *
 * This can be necessary in the case where modifying the buffer
 * would confuse on-going input method behavior.
 *
 * Since: 2.22
 */
void
ctk_text_view_reset_im_context (CtkTextView *text_view)
{
  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  if (text_view->priv->need_im_reset)
    {
      text_view->priv->need_im_reset = FALSE;
      ctk_im_context_reset (text_view->priv->im_context);
    }
}

/**
 * ctk_text_view_im_context_filter_keypress:
 * @text_view: a #CtkTextView
 * @event: the key event
 *
 * Allow the #CtkTextView input method to internally handle key press
 * and release events. If this function returns %TRUE, then no further
 * processing should be done for this key event. See
 * ctk_im_context_filter_keypress().
 *
 * Note that you are expected to call this function from your handler
 * when overriding key event handling. This is needed in the case when
 * you need to insert your own key handling between the input method
 * and the default key event handling of the #CtkTextView.
 *
 * |[<!-- language="C" -->
 * static gboolean
 * ctk_foo_bar_key_press_event (CtkWidget   *widget,
 *                              CdkEventKey *event)
 * {
 *   guint keyval;
 *
 *   cdk_event_get_keyval ((CdkEvent*)event, &keyval);
 *
 *   if (keyval == CDK_KEY_Return || keyval == CDK_KEY_KP_Enter)
 *     {
 *       if (ctk_text_view_im_context_filter_keypress (CTK_TEXT_VIEW (widget), event))
 *         return TRUE;
 *     }
 *
 *   // Do some stuff
 *
 *   return CTK_WIDGET_CLASS (ctk_foo_bar_parent_class)->key_press_event (widget, event);
 * }
 * ]|
 *
 * Returns: %TRUE if the input method handled the key event.
 *
 * Since: 2.22
 */
gboolean
ctk_text_view_im_context_filter_keypress (CtkTextView  *text_view,
                                          CdkEventKey  *event)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);

  return ctk_im_context_filter_keypress (text_view->priv->im_context, event);
}

/*
 * DND feature
 */

static void
drag_begin_cb (CtkWidget      *widget,
               CdkDragContext *context,
               gpointer        data)
{
  CtkTextView     *text_view = CTK_TEXT_VIEW (widget);
  CtkTextBuffer   *buffer = ctk_text_view_get_buffer (text_view);
  CtkTextIter      start;
  CtkTextIter      end;
  cairo_surface_t *surface = NULL;

  g_signal_handlers_disconnect_by_func (widget, drag_begin_cb, NULL);

  if (ctk_text_buffer_get_selection_bounds (buffer, &start, &end))
    surface = _ctk_text_util_create_rich_drag_icon (widget, buffer, &start, &end);

  if (surface)
    {
      ctk_drag_set_icon_surface (context, surface);
      cairo_surface_destroy (surface);
    }
  else
    {
      ctk_drag_set_icon_default (context);
    }
}

static void
ctk_text_view_start_selection_dnd (CtkTextView       *text_view,
                                   const CtkTextIter *iter,
                                   const CdkEvent    *event,
                                   gint               x,
                                   gint               y)
{
  CtkTargetList *target_list;

  target_list = ctk_text_buffer_get_copy_target_list (get_buffer (text_view));

  g_signal_connect (text_view, "drag-begin",
                    G_CALLBACK (drag_begin_cb), NULL);
  ctk_drag_begin_with_coordinates (CTK_WIDGET (text_view), target_list,
                                   CDK_ACTION_COPY | CDK_ACTION_MOVE,
                                   1, (CdkEvent*) event, x, y);
}

static void
ctk_text_view_drag_begin (CtkWidget        *widget,
                          CdkDragContext   *context)
{
  /* do nothing */
}

static void
ctk_text_view_drag_end (CtkWidget        *widget,
                        CdkDragContext   *context)
{
  CtkTextView *text_view;

  text_view = CTK_TEXT_VIEW (widget);
  text_view->priv->dnd_x = text_view->priv->dnd_y = -1;
}

static void
ctk_text_view_drag_data_get (CtkWidget        *widget,
                             CdkDragContext   *context,
                             CtkSelectionData *selection_data,
                             guint             info,
                             guint             time)
{
  CtkTextView *text_view = CTK_TEXT_VIEW (widget);
  CtkTextBuffer *buffer = ctk_text_view_get_buffer (text_view);

  if (info == CTK_TEXT_BUFFER_TARGET_INFO_BUFFER_CONTENTS)
    {
      ctk_selection_data_set (selection_data,
                              cdk_atom_intern_static_string ("CTK_TEXT_BUFFER_CONTENTS"),
                              8, /* bytes */
                              (void*)&buffer,
                              sizeof (buffer));
    }
  else if (info == CTK_TEXT_BUFFER_TARGET_INFO_RICH_TEXT)
    {
      CtkTextIter start;
      CtkTextIter end;
      guint8 *str = NULL;
      gsize len;

      if (ctk_text_buffer_get_selection_bounds (buffer, &start, &end))
        {
          /* Extract the selected text */
          str = ctk_text_buffer_serialize (buffer, buffer,
                                           ctk_selection_data_get_target (selection_data),
                                           &start, &end,
                                           &len);
        }

      if (str)
        {
          ctk_selection_data_set (selection_data,
                                  ctk_selection_data_get_target (selection_data),
                                  8, /* bytes */
                                  (guchar *) str, len);
          g_free (str);
        }
    }
  else
    {
      CtkTextIter start;
      CtkTextIter end;
      gchar *str = NULL;

      if (ctk_text_buffer_get_selection_bounds (buffer, &start, &end))
        {
          /* Extract the selected text */
          str = ctk_text_iter_get_visible_text (&start, &end);
        }

      if (str)
        {
          ctk_selection_data_set_text (selection_data, str, -1);
          g_free (str);
        }
    }
}

static void
ctk_text_view_drag_data_delete (CtkWidget        *widget,
                                CdkDragContext   *context)
{
  ctk_text_buffer_delete_selection (CTK_TEXT_VIEW (widget)->priv->buffer,
                                    TRUE, CTK_TEXT_VIEW (widget)->priv->editable);
}

static void
ctk_text_view_drag_leave (CtkWidget        *widget,
                          CdkDragContext   *context,
                          guint             time)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;

  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  ctk_text_mark_set_visible (priv->dnd_mark, FALSE);

  priv->dnd_x = priv->dnd_y = -1;

  if (priv->scroll_timeout != 0)
    g_source_remove (priv->scroll_timeout);

  priv->scroll_timeout = 0;

  ctk_drag_unhighlight (widget);
}

static gboolean
ctk_text_view_drag_motion (CtkWidget        *widget,
                           CdkDragContext   *context,
                           gint              x,
                           gint              y,
                           guint             time)
{
  CtkTextIter newplace;
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  CtkTextIter start;
  CtkTextIter end;
  CdkRectangle target_rect;
  gint bx, by;
  CdkAtom target;
  CdkDragAction suggested_action = 0;
  
  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  target_rect = priv->text_window->allocation;
  
  if (x < target_rect.x ||
      y < target_rect.y ||
      x > (target_rect.x + target_rect.width) ||
      y > (target_rect.y + target_rect.height))
    return FALSE; /* outside the text window, allow parent widgets to handle event */

  ctk_text_view_window_to_buffer_coords (text_view,
                                         CTK_TEXT_WINDOW_WIDGET,
                                         x, y,
                                         &bx, &by);

  ctk_text_layout_get_iter_at_pixel (priv->layout,
                                     &newplace,
                                     bx, by);  

  target = ctk_drag_dest_find_target (widget, context,
                                      ctk_drag_dest_get_target_list (widget));

  if (target == CDK_NONE)
    {
      /* can't accept any of the offered targets */
    }                                 
  else if (ctk_text_buffer_get_selection_bounds (get_buffer (text_view),
                                                 &start, &end) &&
           ctk_text_iter_compare (&newplace, &start) >= 0 &&
           ctk_text_iter_compare (&newplace, &end) <= 0)
    {
      /* We're inside the selection. */
    }
  else
    {      
      if (ctk_text_iter_can_insert (&newplace, priv->editable))
        {
          CtkWidget *source_widget;
          
          suggested_action = cdk_drag_context_get_suggested_action (context);
          
          source_widget = ctk_drag_get_source_widget (context);
          
          if (source_widget == widget)
            {
              /* Default to MOVE, unless the user has
               * pressed ctrl or alt to affect available actions
               */
              if ((cdk_drag_context_get_actions (context) & CDK_ACTION_MOVE) != 0)
                suggested_action = CDK_ACTION_MOVE;
            }
        }
      else
        {
          /* Can't drop here. */
        }
    }

  if (suggested_action != 0)
    {
      ctk_text_mark_set_visible (priv->dnd_mark, cursor_visible (text_view));
      cdk_drag_status (context, suggested_action, time);
    }
  else
    {
      cdk_drag_status (context, 0, time);
      ctk_text_mark_set_visible (priv->dnd_mark, FALSE);
    }

  /* DnD uses text window coords, so subtract extra widget
   * coords that happen e.g. when displaying line numbers.
   */
  priv->dnd_x = x - target_rect.x;
  priv->dnd_y = y - target_rect.y;

  if (!priv->scroll_timeout)
  {
    priv->scroll_timeout =
      cdk_threads_add_timeout (100, drag_scan_timeout, text_view);
    g_source_set_name_by_id (text_view->priv->scroll_timeout, "[ctk+] drag_scan_timeout");
  }

  ctk_drag_highlight (widget);

  /* TRUE return means don't propagate the drag motion to parent
   * widgets that may also be drop sites.
   */
  return TRUE;
}

static gboolean
ctk_text_view_drag_drop (CtkWidget        *widget,
                         CdkDragContext   *context,
                         gint              x,
                         gint              y,
                         guint             time)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  CtkTextIter drop_point;
  CdkAtom target = CDK_NONE;

  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  if (priv->scroll_timeout != 0)
    g_source_remove (priv->scroll_timeout);

  priv->scroll_timeout = 0;

  ctk_text_mark_set_visible (priv->dnd_mark, FALSE);

  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view),
                                    &drop_point,
                                    priv->dnd_mark);

  if (ctk_text_iter_can_insert (&drop_point, priv->editable))
    target = ctk_drag_dest_find_target (widget, context, NULL);

  if (target != CDK_NONE)
    ctk_drag_get_data (widget, context, target, time);
  else
    ctk_drag_finish (context, FALSE, FALSE, time);

  return TRUE;
}

static void
insert_text_data (CtkTextView      *text_view,
                  CtkTextIter      *drop_point,
                  CtkSelectionData *selection_data)
{
  guchar *str;

  str = ctk_selection_data_get_text (selection_data);

  if (str)
    {
      if (!ctk_text_buffer_insert_interactive (get_buffer (text_view),
                                               drop_point, (gchar *) str, -1,
                                               text_view->priv->editable))
        {
          ctk_widget_error_bell (CTK_WIDGET (text_view));
        }

      g_free (str);
    }
}

static void
ctk_text_view_drag_data_received (CtkWidget        *widget,
                                  CdkDragContext   *context,
                                  gint              x,
                                  gint              y,
                                  CtkSelectionData *selection_data,
                                  guint             info,
                                  guint             time)
{
  CtkTextIter drop_point;
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  gboolean success = FALSE;
  CtkTextBuffer *buffer = NULL;

  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  if (!priv->dnd_mark)
    goto done;

  buffer = get_buffer (text_view);

  ctk_text_buffer_get_iter_at_mark (buffer,
                                    &drop_point,
                                    priv->dnd_mark);
  
  if (!ctk_text_iter_can_insert (&drop_point, priv->editable))
    goto done;

  success = TRUE;

  ctk_text_buffer_begin_user_action (buffer);

  if (info == CTK_TEXT_BUFFER_TARGET_INFO_BUFFER_CONTENTS)
    {
      CtkTextBuffer *src_buffer = NULL;
      CtkTextIter start, end;
      gboolean copy_tags = TRUE;

      if (ctk_selection_data_get_length (selection_data) != sizeof (src_buffer))
        return;

      memcpy (&src_buffer, ctk_selection_data_get_data (selection_data), sizeof (src_buffer));

      if (src_buffer == NULL)
        return;

      g_return_if_fail (CTK_IS_TEXT_BUFFER (src_buffer));

      if (ctk_text_buffer_get_tag_table (src_buffer) !=
          ctk_text_buffer_get_tag_table (buffer))
        {
          /*  try to find a suitable rich text target instead  */
          CdkAtom *atoms;
          gint     n_atoms;
          GList   *list;
          CdkAtom  target = CDK_NONE;

          copy_tags = FALSE;

          atoms = ctk_text_buffer_get_deserialize_formats (buffer, &n_atoms);

          for (list = cdk_drag_context_list_targets (context); list; list = list->next)
            {
              gint i;

              for (i = 0; i < n_atoms; i++)
                if (GUINT_TO_POINTER (atoms[i]) == list->data)
                  {
                    target = atoms[i];
                    break;
                  }
            }

          g_free (atoms);

          if (target != CDK_NONE)
            {
              ctk_drag_get_data (widget, context, target, time);
              ctk_text_buffer_end_user_action (buffer);
              return;
            }
        }

      if (ctk_text_buffer_get_selection_bounds (src_buffer,
                                                &start,
                                                &end))
        {
          if (copy_tags)
            ctk_text_buffer_insert_range_interactive (buffer,
                                                      &drop_point,
                                                      &start,
                                                      &end,
                                                      priv->editable);
          else
            {
              gchar *str;

              str = ctk_text_iter_get_visible_text (&start, &end);
              ctk_text_buffer_insert_interactive (buffer,
                                                  &drop_point, str, -1,
                                                  priv->editable);
              g_free (str);
            }
        }
    }
  else if (ctk_selection_data_get_length (selection_data) > 0 &&
           info == CTK_TEXT_BUFFER_TARGET_INFO_RICH_TEXT)
    {
      gboolean retval;
      GError *error = NULL;

      retval = ctk_text_buffer_deserialize (buffer, buffer,
                                            ctk_selection_data_get_target (selection_data),
                                            &drop_point,
                                            (guint8 *) ctk_selection_data_get_data (selection_data),
                                            ctk_selection_data_get_length (selection_data),
                                            &error);

      if (!retval)
        {
          g_warning ("error pasting: %s", error->message);
          g_clear_error (&error);
        }
    }
  else
    insert_text_data (text_view, &drop_point, selection_data);

 done:
  ctk_drag_finish (context, success,
		   success && cdk_drag_context_get_selected_action (context) == CDK_ACTION_MOVE,
		   time);

  if (success)
    {
      ctk_text_buffer_get_iter_at_mark (buffer,
                                        &drop_point,
                                        priv->dnd_mark);
      ctk_text_buffer_place_cursor (buffer, &drop_point);

      ctk_text_buffer_end_user_action (buffer);
    }
}

/**
 * ctk_text_view_get_hadjustment:
 * @text_view: a #CtkTextView
 *
 * Gets the horizontal-scrolling #CtkAdjustment.
 *
 * Returns: (transfer none): pointer to the horizontal #CtkAdjustment
 *
 * Since: 2.22
 *
 * Deprecated: 3.0: Use ctk_scrollable_get_hadjustment()
 **/
CtkAdjustment*
ctk_text_view_get_hadjustment (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), NULL);

  return text_view->priv->hadjustment;
}

static void
ctk_text_view_set_hadjustment (CtkTextView   *text_view,
                               CtkAdjustment *adjustment)
{
  CtkTextViewPrivate *priv = text_view->priv;

  if (adjustment && priv->hadjustment == adjustment)
    return;

  if (priv->hadjustment != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->hadjustment,
                                            ctk_text_view_value_changed,
                                            text_view);
      g_object_unref (priv->hadjustment);
    }

  if (adjustment == NULL)
    adjustment = ctk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (ctk_text_view_value_changed), text_view);
  priv->hadjustment = g_object_ref_sink (adjustment);
  ctk_text_view_set_hadjustment_values (text_view);

  g_object_notify (G_OBJECT (text_view), "hadjustment");
}

/**
 * ctk_text_view_get_vadjustment:
 * @text_view: a #CtkTextView
 *
 * Gets the vertical-scrolling #CtkAdjustment.
 *
 * Returns: (transfer none): pointer to the vertical #CtkAdjustment
 *
 * Since: 2.22
 *
 * Deprecated: 3.0: Use ctk_scrollable_get_vadjustment()
 **/
CtkAdjustment*
ctk_text_view_get_vadjustment (CtkTextView *text_view)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), NULL);

  return text_view->priv->vadjustment;
}

static void
ctk_text_view_set_vadjustment (CtkTextView   *text_view,
                               CtkAdjustment *adjustment)
{
  CtkTextViewPrivate *priv = text_view->priv;

  if (adjustment && priv->vadjustment == adjustment)
    return;

  if (priv->vadjustment != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->vadjustment,
                                            ctk_text_view_value_changed,
                                            text_view);
      g_object_unref (priv->vadjustment);
    }

  if (adjustment == NULL)
    adjustment = ctk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (ctk_text_view_value_changed), text_view);
  priv->vadjustment = g_object_ref_sink (adjustment);
  ctk_text_view_set_vadjustment_values (text_view);

  g_object_notify (G_OBJECT (text_view), "vadjustment");
}

static void
ctk_text_view_set_hadjustment_values (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv;
  gint screen_width;
  gdouble old_value;
  gdouble new_value;
  gdouble new_upper;

  priv = text_view->priv;

  screen_width = SCREEN_WIDTH (text_view);
  old_value = ctk_adjustment_get_value (priv->hadjustment);
  new_upper = MAX (screen_width, priv->width);

  g_object_set (priv->hadjustment,
                "lower", 0.0,
                "upper", new_upper,
                "page-size", (gdouble)screen_width,
                "step-increment", screen_width * 0.1,
                "page-increment", screen_width * 0.9,
                NULL);

  new_value = CLAMP (old_value, 0, new_upper - screen_width);
  if (new_value != old_value)
    ctk_adjustment_set_value (priv->hadjustment, new_value);
}

static void
ctk_text_view_set_vadjustment_values (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv;
  CtkTextIter first_para;
  gint screen_height;
  gint y;
  gdouble old_value;
  gdouble new_value;
  gdouble new_upper;

  priv = text_view->priv;

  screen_height = SCREEN_HEIGHT (text_view);
  old_value = ctk_adjustment_get_value (priv->vadjustment);
  new_upper = MAX (screen_height, priv->height);

  g_object_set (priv->vadjustment,
                "lower", 0.0,
                "upper", new_upper,
                "page-size", (gdouble)screen_height,
                "step-increment", screen_height * 0.1,
                "page-increment", screen_height * 0.9,
                NULL);

  /* Now adjust the value of the adjustment to keep the cursor at the
   * same place in the buffer */
  ctk_text_view_ensure_layout (text_view);
  ctk_text_view_get_first_para_iter (text_view, &first_para);
  ctk_text_layout_get_line_yrange (priv->layout, &first_para, &y, NULL);

  y += priv->first_para_pixels;

  new_value = CLAMP (y, 0, new_upper - screen_height);
  if (new_value != old_value)
    ctk_adjustment_set_value (priv->vadjustment, new_value);
 }

static void
adjust_allocation (CtkWidget *widget,
                   int        dx,
                   int        dy)
{
  CtkAllocation allocation;

  if (!ctk_widget_is_drawable (widget))
    return;

  ctk_widget_get_allocation (widget, &allocation);
  allocation.x += dx;
  allocation.y += dy;
  ctk_widget_size_allocate (widget, &allocation);
}

static void
ctk_text_view_value_changed (CtkAdjustment *adjustment,
                             CtkTextView   *text_view)
{
  CtkTextViewPrivate *priv;
  CtkTextIter iter;
  gint line_top;
  gint dx = 0;
  gint dy = 0;

  priv = text_view->priv;

  /* Note that we oddly call this function with adjustment == NULL
   * sometimes
   */
  
  priv->onscreen_validated = FALSE;

  DV(g_print(">Scroll offset changed %s/%g, onscreen_validated = FALSE ("G_STRLOC")\n",
             adjustment == priv->hadjustment ? "hadjustment" : adjustment == priv->vadjustment ? "vadjustment" : "none",
             adjustment ? ctk_adjustment_get_value (adjustment) : 0.0));
  
  if (adjustment == priv->hadjustment)
    {
      dx = priv->xoffset - (gint)ctk_adjustment_get_value (adjustment);
      priv->xoffset = (gint)ctk_adjustment_get_value (adjustment) - priv->left_padding;

      /* If the change is due to a size change we need 
       * to invalidate the entire text window because there might be
       * right-aligned or centered text 
       */
      if (priv->width_changed)
	{
	  if (ctk_widget_get_realized (CTK_WIDGET (text_view)))
	    cdk_window_invalidate_rect (priv->text_window->bin_window, NULL, FALSE);
	  
	  priv->width_changed = FALSE;
	}
    }
  else if (adjustment == priv->vadjustment)
    {
      dy = priv->yoffset - (gint)ctk_adjustment_get_value (adjustment) + priv->top_border ;
      priv->yoffset -= dy;

      if (priv->layout)
        {
          ctk_text_layout_get_line_at_y (priv->layout, &iter, ctk_adjustment_get_value (adjustment), &line_top);

          ctk_text_buffer_move_mark (get_buffer (text_view), priv->first_para_mark, &iter);

          priv->first_para_pixels = ctk_adjustment_get_value (adjustment) - line_top;
        }
    }
  
  if (dx != 0 || dy != 0)
    {
      GSList *tmp_list;

      if (ctk_widget_get_realized (CTK_WIDGET (text_view)))
        {
          if (dy != 0)
            {
              if (priv->left_window)
                text_window_scroll (priv->left_window, 0, dy);
              if (priv->right_window)
                text_window_scroll (priv->right_window, 0, dy);
            }
      
          if (dx != 0)
            {
              if (priv->top_window)
                text_window_scroll (priv->top_window, dx, 0);
              if (priv->bottom_window)
                text_window_scroll (priv->bottom_window, dx, 0);
            }
      
          /* It looks nicer to scroll the main area last, because
           * it takes a while, and making the side areas update
           * afterward emphasizes the slowness of scrolling the
           * main area.
           */
          text_window_scroll (priv->text_window, dx, dy);
        }
      
      /* Children are now "moved" in the text window, poke
       * into widget->allocation for each child
       */
      tmp_list = priv->children;
      while (tmp_list != NULL)
        {
          CtkTextViewChild *child = tmp_list->data;
          gint child_dx = 0, child_dy = 0;

          if (child->anchor)
            {
              child_dx = dx;
              child_dy = dy;
            }
          else
            {
              if (child->type == CTK_TEXT_WINDOW_TEXT ||
                  child->type == CTK_TEXT_WINDOW_LEFT ||
                  child->type == CTK_TEXT_WINDOW_RIGHT)
                child_dy = dy;
              if (child->type == CTK_TEXT_WINDOW_TEXT ||
                  child->type == CTK_TEXT_WINDOW_TOP ||
                  child->type == CTK_TEXT_WINDOW_BOTTOM)
                child_dx = dx;
            }

          if (child_dx != 0 || child_dy != 0)
            adjust_allocation (child->widget, child_dx, child_dy);

          tmp_list = tmp_list->next;
        }
    }

  /* This could result in invalidation, which would install the
   * first_validate_idle, which would validate onscreen;
   * but we're going to go ahead and validate here, so
   * first_validate_idle shouldn't have anything to do.
   */
  ctk_text_view_update_layout_width (text_view);
  
  /* We also update the IM spot location here, since the IM context
   * might do something that leads to validation.
   */
  ctk_text_view_update_im_spot_location (text_view);

  /* note that validation of onscreen could invoke this function
   * recursively, by scrolling to maintain first_para, or in response
   * to updating the layout width, however there is no problem with
   * that, or shouldn't be.
   */
  ctk_text_view_validate_onscreen (text_view);
  
  /* If this got installed, get rid of it, it's just a waste of time. */
  if (priv->first_validate_idle != 0)
    {
      g_source_remove (priv->first_validate_idle);
      priv->first_validate_idle = 0;
    }

  /* Allow to extend selection with mouse scrollwheel. Bug 710612 */
  if (ctk_gesture_is_active (priv->drag_gesture))
    {
      CdkEvent *current_event;
      current_event = ctk_get_current_event ();
      if (current_event != NULL)
        {
          if (current_event->type == CDK_SCROLL)
            move_mark_to_pointer_and_scroll (text_view, "insert");

          cdk_event_free (current_event);
        }
    }

  /* Finally we update the IM cursor location again, to ensure any
   * changes made by the validation are pushed through.
   */
  ctk_text_view_update_im_spot_location (text_view);

  if (priv->text_handle)
    ctk_text_view_update_handles (text_view,
                                  _ctk_text_handle_get_mode (priv->text_handle));

  DV(g_print(">End scroll offset changed handler ("G_STRLOC")\n"));
}

static void
ctk_text_view_commit_handler (CtkIMContext  *context,
                              const gchar   *str,
                              CtkTextView   *text_view)
{
  ctk_text_view_commit_text (text_view, str);
}

static void
ctk_text_view_commit_text (CtkTextView   *text_view,
                           const gchar   *str)
{
  CtkTextViewPrivate *priv;
  gboolean had_selection;

  priv = text_view->priv;

  ctk_text_buffer_begin_user_action (get_buffer (text_view));

  had_selection = ctk_text_buffer_get_selection_bounds (get_buffer (text_view),
                                                        NULL, NULL);
  
  ctk_text_buffer_delete_selection (get_buffer (text_view), TRUE,
                                    priv->editable);

  if (!strcmp (str, "\n"))
    {
      if (!ctk_text_buffer_insert_interactive_at_cursor (get_buffer (text_view), "\n", 1,
                                                         priv->editable))
        {
          ctk_widget_error_bell (CTK_WIDGET (text_view));
        }
    }
  else
    {
      if (!had_selection && priv->overwrite_mode)
	{
	  CtkTextIter insert;

	  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view),
					    &insert,
					    ctk_text_buffer_get_insert (get_buffer (text_view)));
	  if (!ctk_text_iter_ends_line (&insert))
	    ctk_text_view_delete_from_cursor (text_view, CTK_DELETE_CHARS, 1);
	}

      if (!ctk_text_buffer_insert_interactive_at_cursor (get_buffer (text_view), str, -1,
                                                         priv->editable))
        {
          ctk_widget_error_bell (CTK_WIDGET (text_view));
        }
    }

  ctk_text_buffer_end_user_action (get_buffer (text_view));

  ctk_text_view_set_virtual_cursor_pos (text_view, -1, -1);
  DV(g_print (G_STRLOC": scrolling onscreen\n"));
  ctk_text_view_scroll_mark_onscreen (text_view,
                                      ctk_text_buffer_get_insert (get_buffer (text_view)));
}

static void
ctk_text_view_preedit_changed_handler (CtkIMContext *context,
				       CtkTextView  *text_view)
{
  CtkTextViewPrivate *priv;
  gchar *str;
  PangoAttrList *attrs;
  gint cursor_pos;
  CtkTextIter iter;

  priv = text_view->priv;

  ctk_text_buffer_get_iter_at_mark (priv->buffer, &iter,
				    ctk_text_buffer_get_insert (priv->buffer));

  /* Keypress events are passed to input method even if cursor position is
   * not editable; so beep here if it's multi-key input sequence, input
   * method will be reset in key-press-event handler.
   */
  ctk_im_context_get_preedit_string (context, &str, &attrs, &cursor_pos);

  if (str && str[0] && !ctk_text_iter_can_insert (&iter, priv->editable))
    {
      ctk_widget_error_bell (CTK_WIDGET (text_view));
      goto out;
    }

  g_signal_emit (text_view, signals[PREEDIT_CHANGED], 0, str);

  if (priv->layout)
    ctk_text_layout_set_preedit_string (priv->layout, str, attrs, cursor_pos);
  if (ctk_widget_has_focus (CTK_WIDGET (text_view)))
    ctk_text_view_scroll_mark_onscreen (text_view,
					ctk_text_buffer_get_insert (get_buffer (text_view)));

out:
  pango_attr_list_unref (attrs);
  g_free (str);
}

static gboolean
ctk_text_view_retrieve_surrounding_handler (CtkIMContext  *context,
					    CtkTextView   *text_view)
{
  CtkTextIter start;
  CtkTextIter end;
  gint pos;
  gchar *text;

  ctk_text_buffer_get_iter_at_mark (text_view->priv->buffer, &start,
				    ctk_text_buffer_get_insert (text_view->priv->buffer));
  end = start;

  pos = ctk_text_iter_get_line_index (&start);
  ctk_text_iter_set_line_offset (&start, 0);
  ctk_text_iter_forward_to_line_end (&end);

  text = ctk_text_iter_get_slice (&start, &end);
  ctk_im_context_set_surrounding (context, text, -1, pos);
  g_free (text);

  return TRUE;
}

static gboolean
ctk_text_view_delete_surrounding_handler (CtkIMContext  *context,
					  gint           offset,
					  gint           n_chars,
					  CtkTextView   *text_view)
{
  CtkTextViewPrivate *priv;
  CtkTextIter start;
  CtkTextIter end;

  priv = text_view->priv;

  ctk_text_buffer_get_iter_at_mark (priv->buffer, &start,
				    ctk_text_buffer_get_insert (priv->buffer));
  end = start;

  ctk_text_iter_forward_chars (&start, offset);
  ctk_text_iter_forward_chars (&end, offset + n_chars);

  ctk_text_buffer_delete_interactive (priv->buffer, &start, &end,
				      priv->editable);

  return TRUE;
}

static void
ctk_text_view_mark_set_handler (CtkTextBuffer     *buffer,
                                const CtkTextIter *location,
                                CtkTextMark       *mark,
                                gpointer           data)
{
  CtkTextView *text_view = CTK_TEXT_VIEW (data);
  gboolean need_reset = FALSE;
  gboolean has_selection;

  if (mark == ctk_text_buffer_get_insert (buffer))
    {
      text_view->priv->virtual_cursor_x = -1;
      text_view->priv->virtual_cursor_y = -1;
      ctk_text_view_update_im_spot_location (text_view);
      need_reset = TRUE;
    }
  else if (mark == ctk_text_buffer_get_selection_bound (buffer))
    {
      need_reset = TRUE;
    }

  if (need_reset)
    {
      ctk_text_view_reset_im_context (text_view);
      if (text_view->priv->text_handle)
        ctk_text_view_update_handles (text_view,
                                      _ctk_text_handle_get_mode (text_view->priv->text_handle));

      has_selection = ctk_text_buffer_get_selection_bounds (get_buffer (text_view), NULL, NULL);
      ctk_css_node_set_visible (text_view->priv->selection_node, has_selection);
    }
}

static void
ctk_text_view_target_list_notify (CtkTextBuffer    *buffer,
                                  const GParamSpec *pspec,
                                  gpointer          data)
{
  CtkWidget     *widget = CTK_WIDGET (data);
  CtkTargetList *view_list;
  CtkTargetList *buffer_list;
  GList         *list;

  view_list = ctk_drag_dest_get_target_list (widget);
  buffer_list = ctk_text_buffer_get_paste_target_list (buffer);

  if (view_list)
    ctk_target_list_ref (view_list);
  else
    view_list = ctk_target_list_new (NULL, 0);

  list = view_list->list;
  while (list)
    {
      CtkTargetPair *pair = list->data;

      list = list->next; /* get next element before removing */

      if (pair->info >= CTK_TEXT_BUFFER_TARGET_INFO_TEXT &&
          pair->info <= CTK_TEXT_BUFFER_TARGET_INFO_BUFFER_CONTENTS)
        {
          ctk_target_list_remove (view_list, pair->target);
        }
    }

  for (list = buffer_list->list; list; list = list->next)
    {
      CtkTargetPair *pair = list->data;

      ctk_target_list_add (view_list, pair->target, pair->flags, pair->info);
    }

  ctk_drag_dest_set_target_list (widget, view_list);
  ctk_target_list_unref (view_list);
}

static void
ctk_text_view_get_virtual_cursor_pos (CtkTextView *text_view,
                                      CtkTextIter *cursor,
                                      gint        *x,
                                      gint        *y)
{
  CtkTextViewPrivate *priv;
  CtkTextIter insert;
  CdkRectangle pos;

  priv = text_view->priv;

  if (cursor)
    insert = *cursor;
  else
    ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &insert,
                                      ctk_text_buffer_get_insert (get_buffer (text_view)));

  if ((x && priv->virtual_cursor_x == -1) ||
      (y && priv->virtual_cursor_y == -1))
    ctk_text_layout_get_cursor_locations (priv->layout, &insert, &pos, NULL);

  if (x)
    {
      if (priv->virtual_cursor_x != -1)
        *x = priv->virtual_cursor_x;
      else
        *x = pos.x;
    }

  if (y)
    {
      if (priv->virtual_cursor_y != -1)
        *y = priv->virtual_cursor_y;
      else
        *y = pos.y + pos.height / 2;
    }
}

static void
ctk_text_view_set_virtual_cursor_pos (CtkTextView *text_view,
                                      gint         x,
                                      gint         y)
{
  CdkRectangle pos;

  if (!text_view->priv->layout)
    return;

  if (x == -1 || y == -1)
    ctk_text_view_get_cursor_locations (text_view, NULL, &pos, NULL);

  text_view->priv->virtual_cursor_x = (x == -1) ? pos.x : x;
  text_view->priv->virtual_cursor_y = (y == -1) ? pos.y + pos.height / 2 : y;
}

/* Quick hack of a popup menu
 */
static void
activate_cb (CtkWidget   *menuitem,
	     CtkTextView *text_view)
{
  const gchar *signal;

  signal = g_object_get_qdata (G_OBJECT (menuitem), quark_ctk_signal);
  g_signal_emit_by_name (text_view, signal);
}

static void
append_action_signal (CtkTextView  *text_view,
		      CtkWidget    *menu,
		      const gchar  *label,
		      const gchar  *signal,
                      gboolean      sensitive)
{
  CtkWidget *menuitem = ctk_menu_item_new_with_mnemonic (label);

  g_object_set_qdata (G_OBJECT (menuitem), quark_ctk_signal, (char *)signal);
  g_signal_connect (menuitem, "activate",
		    G_CALLBACK (activate_cb), text_view);

  ctk_widget_set_sensitive (menuitem, sensitive);
  
  ctk_widget_show (menuitem);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
}

static void
ctk_text_view_select_all (CtkWidget *widget,
			  gboolean select)
{
  CtkTextView *text_view = CTK_TEXT_VIEW (widget);
  CtkTextBuffer *buffer;
  CtkTextIter start_iter, end_iter, insert;

  buffer = text_view->priv->buffer;
  if (select) 
    {
      ctk_text_buffer_get_bounds (buffer, &start_iter, &end_iter);
      ctk_text_buffer_select_range (buffer, &start_iter, &end_iter);
    }
  else 
    {
      ctk_text_buffer_get_iter_at_mark (buffer, &insert,
					ctk_text_buffer_get_insert (buffer));
      ctk_text_buffer_move_mark_by_name (buffer, "selection_bound", &insert);
    }
}

static void
select_all_cb (CtkWidget   *menuitem,
	       CtkTextView *text_view)
{
  ctk_text_view_select_all (CTK_WIDGET (text_view), TRUE);
}

static void
delete_cb (CtkTextView *text_view)
{
  ctk_text_buffer_delete_selection (get_buffer (text_view), TRUE,
				    text_view->priv->editable);
}

static void
popup_menu_detach (CtkWidget *attach_widget,
		   CtkMenu   *menu)
{
  CTK_TEXT_VIEW (attach_widget)->priv->popup_menu = NULL;
}

typedef struct
{
  CtkTextView *text_view;
  CdkEvent *trigger_event;
} PopupInfo;

static gboolean
range_contains_editable_text (const CtkTextIter *start,
                              const CtkTextIter *end,
                              gboolean default_editability)
{
  CtkTextIter iter = *start;

  while (ctk_text_iter_compare (&iter, end) < 0)
    {
      if (ctk_text_iter_editable (&iter, default_editability))
        return TRUE;

      ctk_text_iter_forward_to_tag_toggle (&iter, NULL);
    }

  return FALSE;
}

static void
popup_targets_received (CtkClipboard     *clipboard,
			CtkSelectionData *data,
			gpointer          user_data)
{
  PopupInfo *info = user_data;
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;

  text_view = info->text_view;
  priv = text_view->priv;

  if (ctk_widget_get_realized (CTK_WIDGET (text_view)))
    {
      /* We implicitely rely here on the fact that if we are pasting ourself, we'll
       * have text targets as well as the private CTK_TEXT_BUFFER_CONTENTS target.
       */
      gboolean clipboard_contains_text;
      CtkWidget *menuitem;
      gboolean have_selection;
      gboolean can_insert;
      CtkTextIter iter;
      CtkTextIter sel_start, sel_end;
      CdkRectangle iter_location;
      CdkRectangle visible_rect;
      gboolean is_visible;

      clipboard_contains_text = ctk_selection_data_targets_include_text (data);

      if (priv->popup_menu)
	ctk_widget_destroy (priv->popup_menu);

      priv->popup_menu = ctk_menu_new ();
      ctk_style_context_add_class (ctk_widget_get_style_context (priv->popup_menu),
                                   CTK_STYLE_CLASS_CONTEXT_MENU);

      ctk_menu_attach_to_widget (CTK_MENU (priv->popup_menu),
				 CTK_WIDGET (text_view),
				 popup_menu_detach);

      have_selection = ctk_text_buffer_get_selection_bounds (get_buffer (text_view),
                                                             &sel_start, &sel_end);

      ctk_text_buffer_get_iter_at_mark (get_buffer (text_view),
					&iter,
					ctk_text_buffer_get_insert (get_buffer (text_view)));

      can_insert = ctk_text_iter_can_insert (&iter, priv->editable);

      append_action_signal (text_view, priv->popup_menu, _("Cu_t"), "cut-clipboard",
			    have_selection &&
                            range_contains_editable_text (&sel_start, &sel_end,
                                                          priv->editable));
      append_action_signal (text_view, priv->popup_menu, _("_Copy"), "copy-clipboard",
			    have_selection);
      append_action_signal (text_view, priv->popup_menu, _("_Paste"), "paste-clipboard",
			    can_insert && clipboard_contains_text);

      menuitem = ctk_menu_item_new_with_mnemonic (_("_Delete"));
      ctk_widget_set_sensitive (menuitem,
				have_selection &&
				range_contains_editable_text (&sel_start, &sel_end,
							      priv->editable));
      g_signal_connect_swapped (menuitem, "activate",
			        G_CALLBACK (delete_cb), text_view);
      ctk_widget_show (menuitem);
      ctk_menu_shell_append (CTK_MENU_SHELL (priv->popup_menu), menuitem);

      menuitem = ctk_separator_menu_item_new ();
      ctk_widget_show (menuitem);
      ctk_menu_shell_append (CTK_MENU_SHELL (priv->popup_menu), menuitem);

      menuitem = ctk_menu_item_new_with_mnemonic (_("Select _All"));
      ctk_widget_set_sensitive (menuitem,
                                ctk_text_buffer_get_char_count (priv->buffer) > 0);
      g_signal_connect (menuitem, "activate",
			G_CALLBACK (select_all_cb), text_view);
      ctk_widget_show (menuitem);
      ctk_menu_shell_append (CTK_MENU_SHELL (priv->popup_menu), menuitem);

      if ((ctk_text_view_get_input_hints (text_view) & CTK_INPUT_HINT_NO_EMOJI) == 0)
        {
          menuitem = ctk_menu_item_new_with_mnemonic (_("Insert _Emoji"));
          ctk_widget_set_sensitive (menuitem, can_insert);
          g_signal_connect_swapped (menuitem, "activate",
                                    G_CALLBACK (ctk_text_view_insert_emoji), text_view);
          ctk_widget_show (menuitem);
          ctk_menu_shell_append (CTK_MENU_SHELL (priv->popup_menu), menuitem);
        }

      g_signal_emit (text_view, signals[POPULATE_POPUP],
		     0, priv->popup_menu);

      if (info->trigger_event && cdk_event_triggers_context_menu (info->trigger_event))
        ctk_menu_popup_at_pointer (CTK_MENU (priv->popup_menu), info->trigger_event);
      else
        {
          ctk_text_view_get_iter_location (text_view, &iter, &iter_location);
          ctk_text_view_get_visible_rect (text_view, &visible_rect);

          is_visible = (iter_location.x + iter_location.width > visible_rect.x &&
                        iter_location.x < visible_rect.x + visible_rect.width &&
                        iter_location.y + iter_location.height > visible_rect.y &&
                        iter_location.y < visible_rect.y + visible_rect.height);

          if (is_visible)
            {
              ctk_text_view_buffer_to_window_coords (text_view,
                                                     CTK_TEXT_WINDOW_WIDGET,
                                                     iter_location.x,
                                                     iter_location.y,
                                                     &iter_location.x,
                                                     &iter_location.y);

              ctk_menu_popup_at_rect (CTK_MENU (priv->popup_menu),
                                      ctk_widget_get_window (CTK_WIDGET (text_view)),
                                      &iter_location,
                                      CDK_GRAVITY_SOUTH_EAST,
                                      CDK_GRAVITY_NORTH_WEST,
                                      info->trigger_event);
            }
          else
            ctk_menu_popup_at_widget (CTK_MENU (priv->popup_menu),
                                      CTK_WIDGET (text_view),
                                      CDK_GRAVITY_CENTER,
                                      CDK_GRAVITY_CENTER,
                                      info->trigger_event);

          ctk_menu_shell_select_first (CTK_MENU_SHELL (priv->popup_menu), FALSE);
        }
    }

  g_clear_pointer (&info->trigger_event, cdk_event_free);
  g_object_unref (text_view);
  g_slice_free (PopupInfo, info);
}

static void
ctk_text_view_do_popup (CtkTextView    *text_view,
                        const CdkEvent *event)
{
  PopupInfo *info = g_slice_new (PopupInfo);

  /* In order to know what entries we should make sensitive, we
   * ask for the current targets of the clipboard, and when
   * we get them, then we actually pop up the menu.
   */
  info->text_view = g_object_ref (text_view);
  info->trigger_event = event ? cdk_event_copy (event) : ctk_get_current_event ();

  ctk_clipboard_request_contents (ctk_widget_get_clipboard (CTK_WIDGET (text_view),
							    CDK_SELECTION_CLIPBOARD),
				  cdk_atom_intern_static_string ("TARGETS"),
				  popup_targets_received,
				  info);
}

static gboolean
ctk_text_view_popup_menu (CtkWidget *widget)
{
  ctk_text_view_do_popup (CTK_TEXT_VIEW (widget), NULL);  
  return TRUE;
}

static void
ctk_text_view_get_selection_rect (CtkTextView           *text_view,
				  cairo_rectangle_int_t *rect)
{
  cairo_rectangle_int_t rect_cursor, rect_bound;
  CtkTextIter cursor, bound;
  CtkTextBuffer *buffer;
  gint x1, y1, x2, y2;

  buffer = get_buffer (text_view);
  ctk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                    ctk_text_buffer_get_insert (buffer));
  ctk_text_buffer_get_iter_at_mark (buffer, &bound,
                                    ctk_text_buffer_get_selection_bound (buffer));

  ctk_text_view_get_cursor_locations (text_view, &cursor, &rect_cursor, NULL);
  ctk_text_view_get_cursor_locations (text_view, &bound, &rect_bound, NULL);

  x1 = MIN (rect_cursor.x, rect_bound.x);
  x2 = MAX (rect_cursor.x, rect_bound.x);
  y1 = MIN (rect_cursor.y, rect_bound.y);
  y2 = MAX (rect_cursor.y + rect_cursor.height, rect_bound.y + rect_bound.height);

  rect->x = x1;
  rect->y = y1;
  rect->width = x2 - x1;
  rect->height = y2 - y1;
}

static void
show_or_hide_handles (CtkWidget   *popover,
                      GParamSpec  *pspec,
                      CtkTextView *text_view)
{
  gboolean visible;
  CtkTextHandle *handle;
  CtkTextHandleMode mode;

  visible = ctk_widget_get_visible (popover);

  handle = text_view->priv->text_handle;
  mode = _ctk_text_handle_get_mode (handle);

  if (!visible)
    ctk_text_view_update_handles (text_view, mode);
  else
    {
      _ctk_text_handle_set_visible (handle, CTK_TEXT_HANDLE_POSITION_SELECTION_START, FALSE);
      _ctk_text_handle_set_visible (handle, CTK_TEXT_HANDLE_POSITION_SELECTION_END, FALSE);
    }
}

static void
activate_bubble_cb (CtkWidget   *item,
                    CtkTextView *text_view)
{
  const gchar *signal;

  signal = g_object_get_qdata (G_OBJECT (item), quark_ctk_signal);
  ctk_widget_hide (text_view->priv->selection_bubble);

  if (strcmp (signal, "select-all") == 0)
    g_signal_emit_by_name (text_view, "select-all", TRUE);
  else
    g_signal_emit_by_name (text_view, signal);
}

static void
append_bubble_action (CtkTextView  *text_view,
                      CtkWidget    *toolbar,
                      const gchar  *label,
                      const gchar  *icon_name,
                      const gchar  *signal,
                      gboolean      sensitive)
{
  CtkWidget *item;

  item = ctk_button_new_from_icon_name (icon_name, CTK_ICON_SIZE_MENU);
  ctk_widget_set_focus_on_click (item, FALSE);
  ctk_widget_set_tooltip_text (item, label);
  g_object_set_qdata (G_OBJECT (item), quark_ctk_signal, (char *)signal);
  g_signal_connect (item, "clicked", G_CALLBACK (activate_bubble_cb), text_view);
  ctk_widget_set_sensitive (CTK_WIDGET (item), sensitive);
  ctk_widget_show (item);
  ctk_container_add (CTK_CONTAINER (toolbar), item);
}

static void
bubble_targets_received (CtkClipboard     *clipboard,
                         CtkSelectionData *data,
                         gpointer          user_data)
{
  CtkTextView *text_view = user_data;
  CtkTextViewPrivate *priv = text_view->priv;
  cairo_rectangle_int_t rect;
  gboolean has_selection;
  gboolean has_clipboard;
  gboolean can_insert;
  gboolean all_selected;
  CtkTextIter iter;
  CtkTextIter sel_start, sel_end;
  CtkTextIter start, end;
  CtkWidget *box;
  CtkWidget *toolbar;

  has_selection = ctk_text_buffer_get_selection_bounds (get_buffer (text_view),
                                                        &sel_start, &sel_end);
  ctk_text_buffer_get_bounds (get_buffer (text_view), &start, &end);

  all_selected = ctk_text_iter_equal (&start, &sel_start) &&
                 ctk_text_iter_equal (&end, &sel_end);

  if (!priv->editable && !has_selection)
    {
      priv->selection_bubble_timeout_id = 0;
      return;
    }

  if (priv->selection_bubble)
    ctk_widget_destroy (priv->selection_bubble);

  priv->selection_bubble = ctk_popover_new (CTK_WIDGET (text_view));
  ctk_style_context_add_class (ctk_widget_get_style_context (priv->selection_bubble),
                               CTK_STYLE_CLASS_TOUCH_SELECTION);
  ctk_popover_set_position (CTK_POPOVER (priv->selection_bubble), CTK_POS_BOTTOM);
  ctk_popover_set_modal (CTK_POPOVER (priv->selection_bubble), FALSE);
  g_signal_connect (priv->selection_bubble, "notify::visible",
                    G_CALLBACK (show_or_hide_handles), text_view);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
  g_object_set (box, "margin", 10, NULL);
  ctk_widget_show (box);
  toolbar = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);
  ctk_widget_show (toolbar);
  ctk_container_add (CTK_CONTAINER (priv->selection_bubble), box);
  ctk_container_add (CTK_CONTAINER (box), toolbar);

  ctk_text_buffer_get_iter_at_mark (get_buffer (text_view), &iter,
                                    ctk_text_buffer_get_insert (get_buffer (text_view)));
  can_insert = ctk_text_iter_can_insert (&iter, priv->editable);
  has_clipboard = ctk_selection_data_targets_include_text (data);

  append_bubble_action (text_view, toolbar, _("Select all"), "edit-select-all-symbolic", "select-all", !all_selected);

  if (range_contains_editable_text (&sel_start, &sel_end, priv->editable) && has_selection)
    append_bubble_action (text_view, toolbar, _("Cut"), "edit-cut-symbolic", "cut-clipboard", TRUE);

  if (has_selection)
    append_bubble_action (text_view, toolbar, _("Copy"), "edit-copy-symbolic", "copy-clipboard", TRUE);

  if (can_insert)
    append_bubble_action (text_view, toolbar, _("Paste"), "edit-paste-symbolic", "paste-clipboard", has_clipboard);

  if (priv->populate_all)
    g_signal_emit (text_view, signals[POPULATE_POPUP], 0, box);

  ctk_text_view_get_selection_rect (text_view, &rect);
  rect.x -= priv->xoffset;
  rect.y -= priv->yoffset;

  _text_window_to_widget_coords (text_view, &rect.x, &rect.y);

  rect.x -= 5;
  rect.y -= 5;
  rect.width += 10;
  rect.height += 10;

  ctk_popover_set_pointing_to (CTK_POPOVER (priv->selection_bubble), &rect);
  ctk_widget_show (priv->selection_bubble);
}

static gboolean
ctk_text_view_selection_bubble_popup_show (gpointer user_data)
{
  CtkTextView *text_view = user_data;
  ctk_clipboard_request_contents (ctk_widget_get_clipboard (CTK_WIDGET (text_view),
							    CDK_SELECTION_CLIPBOARD),
				  cdk_atom_intern_static_string ("TARGETS"),
				  bubble_targets_received,
				  text_view);
  text_view->priv->selection_bubble_timeout_id = 0;

  return G_SOURCE_REMOVE;
}

static void
ctk_text_view_selection_bubble_popup_unset (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv;

  priv = text_view->priv;

  if (priv->selection_bubble)
    ctk_widget_hide (priv->selection_bubble);

  if (priv->selection_bubble_timeout_id)
    {
      g_source_remove (priv->selection_bubble_timeout_id);
      priv->selection_bubble_timeout_id = 0;
    }
}

static void
ctk_text_view_selection_bubble_popup_set (CtkTextView *text_view)
{
  CtkTextViewPrivate *priv;

  priv = text_view->priv;

  if (priv->selection_bubble_timeout_id)
    g_source_remove (priv->selection_bubble_timeout_id);

  priv->selection_bubble_timeout_id =
    cdk_threads_add_timeout (50, ctk_text_view_selection_bubble_popup_show,
                             text_view);
  g_source_set_name_by_id (priv->selection_bubble_timeout_id, "[ctk+] ctk_text_view_selection_bubble_popup_cb");
}

/* Child CdkWindows */

static void
node_style_changed_cb (CtkCssNode        *node,
                       CtkCssStyleChange *change,
                       CtkWidget         *widget)
{
  CtkTextViewPrivate *priv = CTK_TEXT_VIEW (widget)->priv;

  if (ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_SIZE | CTK_CSS_AFFECTS_CLIP))
    ctk_widget_queue_resize (widget);
  else
    ctk_widget_queue_draw (widget);

  if (node == priv->text_window->css_node)
    {
      CtkCssStyle *style = ctk_css_style_change_get_new_style (change);
      ctk_pixel_cache_set_is_opaque (priv->pixel_cache, ctk_css_style_render_background_is_opaque (style));
    }
}

static void
update_node_ordering (CtkWidget *widget)
{
  CtkTextViewPrivate *priv = CTK_TEXT_VIEW (widget)->priv;
  CtkCssNode *widget_node, *sibling;

  if (priv->text_window == NULL)
    return;

  widget_node = ctk_widget_get_css_node (widget);
  sibling = priv->text_window->css_node;

  if (priv->left_window)
    {
      ctk_css_node_insert_before (widget_node, priv->left_window->css_node, sibling);
      sibling = priv->left_window->css_node;
    }
  if (priv->top_window)
    {
      ctk_css_node_insert_before (widget_node, priv->top_window->css_node, sibling);
    }

  sibling = priv->text_window->css_node;
  if (priv->right_window)
    {
      ctk_css_node_insert_after (widget_node, priv->right_window->css_node, sibling);
      sibling = priv->right_window->css_node;
    }
  if (priv->bottom_window)
    {
      ctk_css_node_insert_after (widget_node, priv->bottom_window->css_node, sibling);
    }
}

static CtkTextWindow*
text_window_new (CtkTextWindowType  type,
                 CtkWidget         *widget,
                 gint               width_request,
                 gint               height_request)
{
  CtkTextWindow *win;
  CtkCssNode *widget_node;

  win = g_slice_new (CtkTextWindow);

  win->type = type;
  win->widget = widget;
  win->window = NULL;
  win->bin_window = NULL;
  win->requisition.width = width_request;
  win->requisition.height = height_request;
  win->allocation.width = width_request;
  win->allocation.height = height_request;
  win->allocation.x = 0;
  win->allocation.y = 0;

  widget_node = ctk_widget_get_css_node (widget);
  win->css_node = ctk_css_node_new ();
  ctk_css_node_set_parent (win->css_node, widget_node);
  ctk_css_node_set_state (win->css_node, ctk_css_node_get_state (widget_node));
  g_signal_connect_object (win->css_node, "style-changed", G_CALLBACK (node_style_changed_cb), widget, 0);
  if (type == CTK_TEXT_WINDOW_TEXT)
    {
      ctk_css_node_set_name (win->css_node, I_("text"));
    }
  else
    {
      ctk_css_node_set_name (win->css_node, I_("border"));
      switch (type)
        {
        case CTK_TEXT_WINDOW_LEFT:
          ctk_css_node_add_class (win->css_node, g_quark_from_static_string (CTK_STYLE_CLASS_LEFT));
          break;
        case CTK_TEXT_WINDOW_RIGHT:
          ctk_css_node_add_class (win->css_node, g_quark_from_static_string (CTK_STYLE_CLASS_RIGHT));
          break;
        case CTK_TEXT_WINDOW_TOP:
          ctk_css_node_add_class (win->css_node, g_quark_from_static_string (CTK_STYLE_CLASS_TOP));
          break;
        case CTK_TEXT_WINDOW_BOTTOM:
          ctk_css_node_add_class (win->css_node, g_quark_from_static_string (CTK_STYLE_CLASS_BOTTOM));
          break;
        default: /* no extra style class */ ;
        }
    }
  g_object_unref (win->css_node);

  return win;
}

static void
text_window_free (CtkTextWindow *win)
{
  if (win->window)
    text_window_unrealize (win);

  ctk_css_node_set_parent (win->css_node, NULL);

  g_slice_free (CtkTextWindow, win);
}

static void
ctk_text_view_get_rendered_rect (CtkTextView  *text_view,
                                 CdkRectangle *rect)
{
  CtkTextViewPrivate *priv = text_view->priv;
  CdkWindow *window;
  guint extra_w;
  guint extra_h;

  _ctk_pixel_cache_get_extra_size (priv->pixel_cache, &extra_w, &extra_h);

  window = ctk_text_view_get_window (text_view, CTK_TEXT_WINDOW_TEXT);

  rect->x = ctk_adjustment_get_value (priv->hadjustment) - extra_w;
  rect->y = ctk_adjustment_get_value (priv->vadjustment) - extra_h - priv->top_border;

  rect->height = cdk_window_get_height (window) + (extra_h * 2);
  rect->width = cdk_window_get_width (window) + (extra_w * 2);
}

static void
ctk_text_view_queue_draw_region (CtkWidget            *widget,
                                 const cairo_region_t *region)
{
  CtkTextView *text_view = CTK_TEXT_VIEW (widget);

  /* There is no way we can know if a region targets the
     not-currently-visible but in pixel cache region, so we
     always just invalidate the whole thing whenever the
     text view gets a queue draw. This doesn't normally happen
     in normal scrolling cases anyway. */
  _ctk_pixel_cache_invalidate (text_view->priv->pixel_cache, NULL);

  CTK_WIDGET_CLASS (ctk_text_view_parent_class)->queue_draw_region (widget, region);
}

static void
text_window_invalidate_handler (CdkWindow      *window,
                                cairo_region_t *region)
{
  gpointer widget;
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  int x, y;

  cdk_window_get_user_data (window, &widget);
  text_view = CTK_TEXT_VIEW (widget);
  priv = text_view->priv;

  /* Scrolling will invalidate everything in the bin window,
   * but we already have it in the cache, so we can ignore that */
  if (priv->in_scroll)
    return;

  x = priv->xoffset;
  y = priv->yoffset + priv->top_border;

  cairo_region_translate (region, x, y);
  _ctk_pixel_cache_invalidate (priv->pixel_cache, region);
  cairo_region_translate (region, -x, -y);
}

static void
text_window_realize (CtkTextWindow *win,
                     CtkWidget     *widget)
{
  CdkWindow *window;
  CdkWindowAttr attributes;
  gint attributes_mask;
  CdkDisplay *display;
  CdkCursor *cursor;

  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.x = win->allocation.x;
  attributes.y = win->allocation.y;
  attributes.width = win->allocation.width;
  attributes.height = win->allocation.height;
  attributes.wclass = CDK_INPUT_OUTPUT;
  attributes.visual = ctk_widget_get_visual (win->widget);
  attributes.event_mask = CDK_VISIBILITY_NOTIFY_MASK;

  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;

  window = ctk_widget_get_window (widget);

  win->window = cdk_window_new (window,
                                &attributes, attributes_mask);

  cdk_window_show (win->window);
  ctk_widget_register_window (win->widget, win->window);
  cdk_window_lower (win->window);

  attributes.x = 0;
  attributes.y = 0;
  attributes.width = win->allocation.width;
  attributes.height = win->allocation.height;
  attributes.event_mask = ctk_widget_get_events (win->widget)
                          | CDK_SCROLL_MASK
                          | CDK_SMOOTH_SCROLL_MASK
                          | CDK_KEY_PRESS_MASK
                          | CDK_BUTTON_PRESS_MASK
                          | CDK_BUTTON_RELEASE_MASK
                          | CDK_POINTER_MOTION_MASK;

  win->bin_window = cdk_window_new (win->window,
                                    &attributes,
                                    attributes_mask);

  ctk_widget_register_window (win->widget, win->bin_window);

  if (win->type == CTK_TEXT_WINDOW_TEXT)
    cdk_window_set_invalidate_handler (win->bin_window,
                                       text_window_invalidate_handler);

  cdk_window_show (win->bin_window);

  switch (win->type)
    {
    case CTK_TEXT_WINDOW_TEXT:
      if (ctk_widget_is_sensitive (widget))
        {
          display = cdk_window_get_display (window);
          cursor = cdk_cursor_new_from_name (display, "text");
          cdk_window_set_cursor (win->bin_window, cursor);
          g_clear_object (&cursor);
        }

      ctk_im_context_set_client_window (CTK_TEXT_VIEW (widget)->priv->im_context,
                                        win->window);
      break;
    default:
      break;
    }

  g_object_set_qdata (G_OBJECT (win->window),
                      g_quark_from_static_string ("ctk-text-view-text-window"),
                      win);

  g_object_set_qdata (G_OBJECT (win->bin_window),
                      g_quark_from_static_string ("ctk-text-view-text-window"),
                      win);
}

static void
text_window_unrealize (CtkTextWindow *win)
{
  if (win->type == CTK_TEXT_WINDOW_TEXT)
    {
      ctk_im_context_set_client_window (CTK_TEXT_VIEW (win->widget)->priv->im_context,
                                        NULL);
    }

  ctk_widget_unregister_window (win->widget, win->window);
  ctk_widget_unregister_window (win->widget, win->bin_window);
  cdk_window_destroy (win->bin_window);
  cdk_window_destroy (win->window);
  win->window = NULL;
  win->bin_window = NULL;
}

static void
text_window_size_allocate (CtkTextWindow *win,
                           CdkRectangle  *rect)
{
  win->allocation = *rect;

  if (win->window)
    {
      cdk_window_move_resize (win->window,
                              rect->x, rect->y,
                              rect->width, rect->height);

      cdk_window_resize (win->bin_window,
                         rect->width, rect->height);
    }
}

static void
text_window_scroll        (CtkTextWindow *win,
                           gint           dx,
                           gint           dy)
{
  CtkTextView *view = CTK_TEXT_VIEW (win->widget);
  CtkTextViewPrivate *priv = view->priv;

  if (dx != 0 || dy != 0)
    {
      if (priv->selection_bubble)
        ctk_widget_hide (priv->selection_bubble);
      view->priv->in_scroll = TRUE;
      cdk_window_scroll (win->bin_window, dx, dy);
      view->priv->in_scroll = FALSE;
    }
}

static void
text_window_invalidate_rect (CtkTextWindow *win,
                             CdkRectangle  *rect)
{
  CdkRectangle window_rect;

  if (!win->bin_window)
    return;

  ctk_text_view_buffer_to_window_coords (CTK_TEXT_VIEW (win->widget),
                                         win->type,
                                         rect->x,
                                         rect->y,
                                         &window_rect.x,
                                         &window_rect.y);

  window_rect.width = rect->width;
  window_rect.height = rect->height;
  
  /* Adjust the rect as appropriate */
  
  switch (win->type)
    {
    case CTK_TEXT_WINDOW_TEXT:
      break;

    case CTK_TEXT_WINDOW_LEFT:
    case CTK_TEXT_WINDOW_RIGHT:
      window_rect.x = 0;
      window_rect.width = win->allocation.width;
      break;

    case CTK_TEXT_WINDOW_TOP:
    case CTK_TEXT_WINDOW_BOTTOM:
      window_rect.y = 0;
      window_rect.height = win->allocation.height;
      break;

    default:
      g_warning ("%s: bug!", G_STRFUNC);
      return;
      break;
    }
          
  cdk_window_invalidate_rect (win->bin_window, &window_rect, FALSE);

#if 0
  {
    cairo_t *cr = cdk_cairo_create (win->bin_window);
    cdk_cairo_rectangle (cr, &window_rect);
    cairo_set_source_rgb  (cr, 1.0, 0.0, 0.0);	/* red */
    cairo_fill (cr);
    cairo_destroy (cr);
  }
#endif
}

static void
text_window_invalidate_cursors (CtkTextWindow *win)
{
  CtkTextView *text_view;
  CtkTextViewPrivate *priv;
  CtkTextIter  iter;
  CdkRectangle strong;
  CdkRectangle weak;
  gboolean     draw_arrow;
  gfloat       cursor_aspect_ratio;
  gint         stem_width;
  gint         arrow_width;

  text_view = CTK_TEXT_VIEW (win->widget);
  priv = text_view->priv;

  ctk_text_buffer_get_iter_at_mark (priv->buffer, &iter,
                                    ctk_text_buffer_get_insert (priv->buffer));

  if (_ctk_text_layout_get_block_cursor (priv->layout, &strong))
    {
      text_window_invalidate_rect (win, &strong);
      return;
    }

  ctk_text_layout_get_cursor_locations (priv->layout, &iter,
                                        &strong, &weak);

  /* cursor width calculation as in ctkstylecontext.c:draw_insertion_cursor(),
   * ignoring the text direction be exposing both sides of the cursor
   */

  draw_arrow = (strong.x != weak.x || strong.y != weak.y);

  ctk_widget_style_get (win->widget,
                        "cursor-aspect-ratio", &cursor_aspect_ratio,
                        NULL);
  
  stem_width = strong.height * cursor_aspect_ratio + 1;
  arrow_width = stem_width + 1;

  strong.width = stem_width;

  /* round up to the next even number */
  if (stem_width & 1)
    stem_width++;

  strong.x     -= stem_width / 2;
  strong.width += stem_width;

  if (draw_arrow)
    {
      strong.x     -= arrow_width;
      strong.width += arrow_width * 2;
    }

  text_window_invalidate_rect (win, &strong);

  if (draw_arrow) /* == have weak */
    {
      stem_width = weak.height * cursor_aspect_ratio + 1;
      arrow_width = stem_width + 1;

      weak.width = stem_width;

      /* round up to the next even number */
      if (stem_width & 1)
        stem_width++;

      weak.x     -= stem_width / 2;
      weak.width += stem_width;

      weak.x     -= arrow_width;
      weak.width += arrow_width * 2;

      text_window_invalidate_rect (win, &weak);
    }
}

static gint
text_window_get_width (CtkTextWindow *win)
{
  return win->allocation.width;
}

static gint
text_window_get_height (CtkTextWindow *win)
{
  return win->allocation.height;
}

/* Windows */


/**
 * ctk_text_view_get_window:
 * @text_view: a #CtkTextView
 * @win: window to get
 *
 * Retrieves the #CdkWindow corresponding to an area of the text view;
 * possible windows include the overall widget window, child windows
 * on the left, right, top, bottom, and the window that displays the
 * text buffer. Windows are %NULL and nonexistent if their width or
 * height is 0, and are nonexistent before the widget has been
 * realized.
 *
 * Returns: (nullable) (transfer none): a #CdkWindow, or %NULL
 **/
CdkWindow*
ctk_text_view_get_window (CtkTextView *text_view,
                          CtkTextWindowType win)
{
  CtkTextViewPrivate *priv = text_view->priv;

  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), NULL);

  switch (win)
    {
    case CTK_TEXT_WINDOW_WIDGET:
      return ctk_widget_get_window (CTK_WIDGET (text_view));
      break;

    case CTK_TEXT_WINDOW_TEXT:
      return priv->text_window->bin_window;
      break;

    case CTK_TEXT_WINDOW_LEFT:
      if (priv->left_window)
        return priv->left_window->bin_window;
      else
        return NULL;
      break;

    case CTK_TEXT_WINDOW_RIGHT:
      if (priv->right_window)
        return priv->right_window->bin_window;
      else
        return NULL;
      break;

    case CTK_TEXT_WINDOW_TOP:
      if (priv->top_window)
        return priv->top_window->bin_window;
      else
        return NULL;
      break;

    case CTK_TEXT_WINDOW_BOTTOM:
      if (priv->bottom_window)
        return priv->bottom_window->bin_window;
      else
        return NULL;
      break;

    case CTK_TEXT_WINDOW_PRIVATE:
      g_warning ("%s: You can't get CTK_TEXT_WINDOW_PRIVATE, it has \"PRIVATE\" in the name because it is private.", G_STRFUNC);
      return NULL;
      break;
    }

  g_warning ("%s: Unknown CtkTextWindowType", G_STRFUNC);
  return NULL;
}

static CtkCssNode *
ctk_text_view_get_css_node (CtkTextView       *text_view,
                            CtkTextWindowType  win)
{
  CtkTextViewPrivate *priv = text_view->priv;

  switch (win)
    {
    case CTK_TEXT_WINDOW_WIDGET:
      return ctk_widget_get_css_node (CTK_WIDGET (text_view));

    case CTK_TEXT_WINDOW_TEXT:
      return priv->text_window->css_node;

    case CTK_TEXT_WINDOW_LEFT:
      if (priv->left_window)
        return priv->left_window->css_node;
      break;

    case CTK_TEXT_WINDOW_RIGHT:
      if (priv->right_window)
        return priv->right_window->css_node;
      break;

    case CTK_TEXT_WINDOW_TOP:
      if (priv->top_window)
        return priv->top_window->css_node;
      break;

    case CTK_TEXT_WINDOW_BOTTOM:
      if (priv->bottom_window)
        return priv->bottom_window->css_node;
      break;

    default:
      break;
    }

  return NULL;
}

/**
 * ctk_text_view_get_window_type:
 * @text_view: a #CtkTextView
 * @window: a window type
 *
 * Usually used to find out which window an event corresponds to.
 *
 * If you connect to an event signal on @text_view, this function
 * should be called on `event->window` to see which window it was.
 *
 * Returns: the window type.
 **/
CtkTextWindowType
ctk_text_view_get_window_type (CtkTextView *text_view,
                               CdkWindow   *window)
{
  CtkTextWindow *win;

  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), CTK_TEXT_WINDOW_PRIVATE);
  g_return_val_if_fail (CDK_IS_WINDOW (window), CTK_TEXT_WINDOW_PRIVATE);

  if (window == ctk_widget_get_window (CTK_WIDGET (text_view)))
    return CTK_TEXT_WINDOW_WIDGET;

  win = g_object_get_qdata (G_OBJECT (window),
                            g_quark_try_string ("ctk-text-view-text-window"));

  if (win)
    return win->type;

  return CTK_TEXT_WINDOW_PRIVATE;
}

static void
buffer_to_widget (CtkTextView      *text_view,
                  gint              buffer_x,
                  gint              buffer_y,
                  gint             *window_x,
                  gint             *window_y)
{
  CtkTextViewPrivate *priv = text_view->priv;

  if (window_x)
    {
      *window_x = buffer_x - priv->xoffset;
      *window_x += priv->text_window->allocation.x;
    }

  if (window_y)
    {
      *window_y = buffer_y - priv->yoffset;
      *window_y += priv->text_window->allocation.y;
    }
}

static void
widget_to_text_window (CtkTextWindow *win,
                       gint           widget_x,
                       gint           widget_y,
                       gint          *window_x,
                       gint          *window_y)
{
  if (window_x)
    *window_x = widget_x - win->allocation.x;

  if (window_y)
    *window_y = widget_y - win->allocation.y;
}

static void
buffer_to_text_window (CtkTextView   *text_view,
                       CtkTextWindow *win,
                       gint           buffer_x,
                       gint           buffer_y,
                       gint          *window_x,
                       gint          *window_y)
{
  if (win == NULL)
    {
      g_warning ("Attempt to convert text buffer coordinates to coordinates "
                 "for a nonexistent or private child window of CtkTextView");
      return;
    }

  buffer_to_widget (text_view,
                    buffer_x, buffer_y,
                    window_x, window_y);

  widget_to_text_window (win,
                         window_x ? *window_x : 0,
                         window_y ? *window_y : 0,
                         window_x,
                         window_y);
}

/**
 * ctk_text_view_buffer_to_window_coords:
 * @text_view: a #CtkTextView
 * @win: a #CtkTextWindowType, except %CTK_TEXT_WINDOW_PRIVATE
 * @buffer_x: buffer x coordinate
 * @buffer_y: buffer y coordinate
 * @window_x: (out) (allow-none): window x coordinate return location or %NULL
 * @window_y: (out) (allow-none): window y coordinate return location or %NULL
 *
 * Converts coordinate (@buffer_x, @buffer_y) to coordinates for the window
 * @win, and stores the result in (@window_x, @window_y). 
 *
 * Note that you can’t convert coordinates for a nonexisting window (see 
 * ctk_text_view_set_border_window_size()).
 **/
void
ctk_text_view_buffer_to_window_coords (CtkTextView      *text_view,
                                       CtkTextWindowType win,
                                       gint              buffer_x,
                                       gint              buffer_y,
                                       gint             *window_x,
                                       gint             *window_y)
{
  CtkTextViewPrivate *priv = text_view->priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (win != CTK_TEXT_WINDOW_PRIVATE);

  switch (win)
    {
    case CTK_TEXT_WINDOW_WIDGET:
      buffer_to_widget (text_view,
                        buffer_x, buffer_y,
                        window_x, window_y);
      break;

    case CTK_TEXT_WINDOW_TEXT:
      if (window_x)
        *window_x = buffer_x - priv->xoffset;
      if (window_y)
        *window_y = buffer_y - priv->yoffset;
      break;

    case CTK_TEXT_WINDOW_LEFT:
      buffer_to_text_window (text_view,
                             priv->left_window,
                             buffer_x, buffer_y,
                             window_x, window_y);
      break;

    case CTK_TEXT_WINDOW_RIGHT:
      buffer_to_text_window (text_view,
                             priv->right_window,
                             buffer_x, buffer_y,
                             window_x, window_y);
      break;

    case CTK_TEXT_WINDOW_TOP:
      buffer_to_text_window (text_view,
                             priv->top_window,
                             buffer_x, buffer_y,
                             window_x, window_y);
      break;

    case CTK_TEXT_WINDOW_BOTTOM:
      buffer_to_text_window (text_view,
                             priv->bottom_window,
                             buffer_x, buffer_y,
                             window_x, window_y);
      break;

    case CTK_TEXT_WINDOW_PRIVATE:
      g_warning ("%s: can't get coords for private windows", G_STRFUNC);
      break;

    default:
      g_warning ("%s: Unknown CtkTextWindowType", G_STRFUNC);
      break;
    }
}

static void
widget_to_buffer (CtkTextView *text_view,
                  gint         widget_x,
                  gint         widget_y,
                  gint        *buffer_x,
                  gint        *buffer_y)
{
  CtkTextViewPrivate *priv = text_view->priv;

  if (buffer_x)
    {
      *buffer_x = widget_x + priv->xoffset;
      *buffer_x -= priv->text_window->allocation.x;
    }

  if (buffer_y)
    {
      *buffer_y = widget_y + priv->yoffset;
      *buffer_y -= priv->text_window->allocation.y;
    }
}

static void
text_window_to_widget (CtkTextWindow *win,
                       gint           window_x,
                       gint           window_y,
                       gint          *widget_x,
                       gint          *widget_y)
{
  if (widget_x)
    *widget_x = window_x + win->allocation.x;

  if (widget_y)
    *widget_y = window_y + win->allocation.y;
}

static void
text_window_to_buffer (CtkTextView   *text_view,
                       CtkTextWindow *win,
                       gint           window_x,
                       gint           window_y,
                       gint          *buffer_x,
                       gint          *buffer_y)
{
  if (win == NULL)
    {
      g_warning ("Attempt to convert CtkTextView buffer coordinates into "
                 "coordinates for a nonexistent child window.");
      return;
    }

  text_window_to_widget (win,
                         window_x,
                         window_y,
                         buffer_x,
                         buffer_y);

  widget_to_buffer (text_view,
                    buffer_x ? *buffer_x : 0,
                    buffer_y ? *buffer_y : 0,
                    buffer_x,
                    buffer_y);
}

/**
 * ctk_text_view_window_to_buffer_coords:
 * @text_view: a #CtkTextView
 * @win: a #CtkTextWindowType except %CTK_TEXT_WINDOW_PRIVATE
 * @window_x: window x coordinate
 * @window_y: window y coordinate
 * @buffer_x: (out) (allow-none): buffer x coordinate return location or %NULL
 * @buffer_y: (out) (allow-none): buffer y coordinate return location or %NULL
 *
 * Converts coordinates on the window identified by @win to buffer
 * coordinates, storing the result in (@buffer_x,@buffer_y).
 *
 * Note that you can’t convert coordinates for a nonexisting window (see 
 * ctk_text_view_set_border_window_size()).
 **/
void
ctk_text_view_window_to_buffer_coords (CtkTextView      *text_view,
                                       CtkTextWindowType win,
                                       gint              window_x,
                                       gint              window_y,
                                       gint             *buffer_x,
                                       gint             *buffer_y)
{
  CtkTextViewPrivate *priv = text_view->priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (win != CTK_TEXT_WINDOW_PRIVATE);

  switch (win)
    {
    case CTK_TEXT_WINDOW_WIDGET:
      widget_to_buffer (text_view,
                        window_x, window_y,
                        buffer_x, buffer_y);
      break;

    case CTK_TEXT_WINDOW_TEXT:
      if (buffer_x)
        *buffer_x = window_x + priv->xoffset;
      if (buffer_y)
        *buffer_y = window_y + priv->yoffset;
      break;

    case CTK_TEXT_WINDOW_LEFT:
      text_window_to_buffer (text_view,
                             priv->left_window,
                             window_x, window_y,
                             buffer_x, buffer_y);
      break;

    case CTK_TEXT_WINDOW_RIGHT:
      text_window_to_buffer (text_view,
                             priv->right_window,
                             window_x, window_y,
                             buffer_x, buffer_y);
      break;

    case CTK_TEXT_WINDOW_TOP:
      text_window_to_buffer (text_view,
                             priv->top_window,
                             window_x, window_y,
                             buffer_x, buffer_y);
      break;

    case CTK_TEXT_WINDOW_BOTTOM:
      text_window_to_buffer (text_view,
                             priv->bottom_window,
                             window_x, window_y,
                             buffer_x, buffer_y);
      break;

    case CTK_TEXT_WINDOW_PRIVATE:
      g_warning ("%s: can't get coords for private windows", G_STRFUNC);
      break;

    default:
      g_warning ("%s: Unknown CtkTextWindowType", G_STRFUNC);
      break;
    }
}

static void
set_window_width (CtkTextView      *text_view,
                  gint              width,
                  CtkTextWindowType type,
                  CtkTextWindow   **winp)
{
  if (width == 0)
    {
      if (*winp)
        {
          text_window_free (*winp);
          *winp = NULL;
          ctk_widget_queue_resize (CTK_WIDGET (text_view));
        }
    }
  else
    {
      if (*winp == NULL)
        {
          *winp = text_window_new (type, CTK_WIDGET (text_view), width, 0);
          /* if the widget is already realized we need to realize the child manually */
          if (ctk_widget_get_realized (CTK_WIDGET (text_view)))
            text_window_realize (*winp, CTK_WIDGET (text_view));
          update_node_ordering (CTK_WIDGET (text_view));
        }
      else
        {
          if ((*winp)->requisition.width == width)
            return;

          (*winp)->requisition.width = width;
        }

      ctk_widget_queue_resize (CTK_WIDGET (text_view));
    }
}


static void
set_window_height (CtkTextView      *text_view,
                   gint              height,
                   CtkTextWindowType type,
                   CtkTextWindow   **winp)
{
  if (height == 0)
    {
      if (*winp)
        {
          text_window_free (*winp);
          *winp = NULL;
          ctk_widget_queue_resize (CTK_WIDGET (text_view));
        }
    }
  else
    {
      if (*winp == NULL)
        {
          *winp = text_window_new (type, CTK_WIDGET (text_view), 0, height);

          /* if the widget is already realized we need to realize the child manually */
          if (ctk_widget_get_realized (CTK_WIDGET (text_view)))
            text_window_realize (*winp, CTK_WIDGET (text_view));
          update_node_ordering (CTK_WIDGET (text_view));
        }
      else
        {
          if ((*winp)->requisition.height == height)
            return;

          (*winp)->requisition.height = height;
        }

      ctk_widget_queue_resize (CTK_WIDGET (text_view));
    }
}

/**
 * ctk_text_view_set_border_window_size:
 * @text_view: a #CtkTextView
 * @type: window to affect
 * @size: width or height of the window
 *
 * Sets the width of %CTK_TEXT_WINDOW_LEFT or %CTK_TEXT_WINDOW_RIGHT,
 * or the height of %CTK_TEXT_WINDOW_TOP or %CTK_TEXT_WINDOW_BOTTOM.
 * Automatically destroys the corresponding window if the size is set
 * to 0, and creates the window if the size is set to non-zero.  This
 * function can only be used for the “border windows”, and it won’t
 * work with %CTK_TEXT_WINDOW_WIDGET, %CTK_TEXT_WINDOW_TEXT, or
 * %CTK_TEXT_WINDOW_PRIVATE.
 **/
void
ctk_text_view_set_border_window_size (CtkTextView      *text_view,
                                      CtkTextWindowType type,
                                      gint              size)
{
  CtkTextViewPrivate *priv = text_view->priv;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (type != CTK_TEXT_WINDOW_PRIVATE);
  g_return_if_fail (size >= 0);

  switch (type)
    {
    case CTK_TEXT_WINDOW_LEFT:
      set_window_width (text_view, size, CTK_TEXT_WINDOW_LEFT,
                        &priv->left_window);
      break;

    case CTK_TEXT_WINDOW_RIGHT:
      set_window_width (text_view, size, CTK_TEXT_WINDOW_RIGHT,
                        &priv->right_window);
      break;

    case CTK_TEXT_WINDOW_TOP:
      set_window_height (text_view, size, CTK_TEXT_WINDOW_TOP,
                         &priv->top_window);
      break;

    case CTK_TEXT_WINDOW_BOTTOM:
      set_window_height (text_view, size, CTK_TEXT_WINDOW_BOTTOM,
                         &priv->bottom_window);
      break;

    default:
      g_warning ("Can only set size of left/right/top/bottom border windows with ctk_text_view_set_border_window_size()");
      break;
    }
}

/**
 * ctk_text_view_get_border_window_size:
 * @text_view: a #CtkTextView
 * @type: window to return size from
 *
 * Gets the width of the specified border window. See
 * ctk_text_view_set_border_window_size().
 *
 * Returns: width of window
 **/
gint
ctk_text_view_get_border_window_size (CtkTextView       *text_view,
				      CtkTextWindowType  type)
{
  CtkTextViewPrivate *priv = text_view->priv;

  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), 0);
  
  switch (type)
    {
    case CTK_TEXT_WINDOW_LEFT:
      if (priv->left_window)
        return priv->left_window->requisition.width;
      break;
      
    case CTK_TEXT_WINDOW_RIGHT:
      if (priv->right_window)
        return priv->right_window->requisition.width;
      break;
      
    case CTK_TEXT_WINDOW_TOP:
      if (priv->top_window)
        return priv->top_window->requisition.height;
      break;

    case CTK_TEXT_WINDOW_BOTTOM:
      if (priv->bottom_window)
        return priv->bottom_window->requisition.height;
      break;
      
    default:
      g_warning ("Can only get size of left/right/top/bottom border windows with ctk_text_view_get_border_window_size()");
      break;
    }

  return 0;
}

/*
 * Child widgets
 */

static CtkTextViewChild*
text_view_child_new_anchored (CtkWidget          *child,
                              CtkTextChildAnchor *anchor,
                              CtkTextLayout      *layout)
{
  CtkTextViewChild *vc;

  vc = g_slice_new (CtkTextViewChild);

  vc->type = CTK_TEXT_WINDOW_PRIVATE;
  vc->widget = child;
  vc->anchor = anchor;

  vc->from_top_of_line = 0;
  vc->from_left_of_buffer = 0;
  
  g_object_ref (vc->widget);
  g_object_ref (vc->anchor);

  g_object_set_qdata (G_OBJECT (child), quark_text_view_child, vc);

  ctk_text_child_anchor_register_child (anchor, child, layout);
  
  return vc;
}

static CtkTextViewChild*
text_view_child_new_window (CtkWidget          *child,
                            CtkTextWindowType   type,
                            gint                x,
                            gint                y)
{
  CtkTextViewChild *vc;

  vc = g_slice_new (CtkTextViewChild);

  vc->widget = child;
  vc->anchor = NULL;

  vc->from_top_of_line = 0;
  vc->from_left_of_buffer = 0;
 
  g_object_ref (vc->widget);

  vc->type = type;
  vc->x = x;
  vc->y = y;

  g_object_set_qdata (G_OBJECT (child), quark_text_view_child, vc);
  
  return vc;
}

static void
text_view_child_free (CtkTextViewChild *child)
{
  g_object_set_qdata (G_OBJECT (child->widget), quark_text_view_child, NULL);

  if (child->anchor)
    {
      ctk_text_child_anchor_unregister_child (child->anchor,
                                              child->widget);
      g_object_unref (child->anchor);
    }

  g_object_unref (child->widget);

  g_slice_free (CtkTextViewChild, child);
}

static void
text_view_child_set_parent_window (CtkTextView      *text_view,
				   CtkTextViewChild *vc)
{
  if (vc->anchor)
    ctk_widget_set_parent_window (vc->widget,
                                  text_view->priv->text_window->bin_window);
  else
    {
      CdkWindow *window;
      window = ctk_text_view_get_window (text_view,
                                         vc->type);
      ctk_widget_set_parent_window (vc->widget, window);
    }
}

static void
add_child (CtkTextView      *text_view,
           CtkTextViewChild *vc)
{
  CtkCssNode *parent;

  text_view->priv->children = g_slist_prepend (text_view->priv->children, vc);

  if (ctk_widget_get_realized (CTK_WIDGET (text_view)))
    text_view_child_set_parent_window (text_view, vc);

  parent = ctk_text_view_get_css_node (text_view, vc->type);
  if (parent == NULL)
    parent = ctk_widget_get_css_node (CTK_WIDGET (text_view));

  ctk_css_node_set_parent (ctk_widget_get_css_node (vc->widget), parent);

  ctk_widget_set_parent (vc->widget, CTK_WIDGET (text_view));
}

/**
 * ctk_text_view_add_child_at_anchor:
 * @text_view: a #CtkTextView
 * @child: a #CtkWidget
 * @anchor: a #CtkTextChildAnchor in the #CtkTextBuffer for @text_view
 * 
 * Adds a child widget in the text buffer, at the given @anchor.
 **/
void
ctk_text_view_add_child_at_anchor (CtkTextView          *text_view,
                                   CtkWidget            *child,
                                   CtkTextChildAnchor   *anchor)
{
  CtkTextViewChild *vc;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (CTK_IS_WIDGET (child));
  g_return_if_fail (CTK_IS_TEXT_CHILD_ANCHOR (anchor));
  g_return_if_fail (ctk_widget_get_parent (child) == NULL);

  ctk_text_view_ensure_layout (text_view);

  vc = text_view_child_new_anchored (child, anchor,
                                     text_view->priv->layout);

  add_child (text_view, vc);

  g_assert (vc->widget == child);
  g_assert (ctk_widget_get_parent (child) == CTK_WIDGET (text_view));
}

/**
 * ctk_text_view_add_child_in_window:
 * @text_view: a #CtkTextView
 * @child: a #CtkWidget
 * @which_window: which window the child should appear in
 * @xpos: X position of child in window coordinates
 * @ypos: Y position of child in window coordinates
 *
 * Adds a child at fixed coordinates in one of the text widget's
 * windows.
 *
 * The window must have nonzero size (see
 * ctk_text_view_set_border_window_size()). Note that the child
 * coordinates are given relative to scrolling. When
 * placing a child in #CTK_TEXT_WINDOW_WIDGET, scrolling is
 * irrelevant, the child floats above all scrollable areas. But when
 * placing a child in one of the scrollable windows (border windows or
 * text window) it will move with the scrolling as needed.
 */
void
ctk_text_view_add_child_in_window (CtkTextView       *text_view,
                                   CtkWidget         *child,
                                   CtkTextWindowType  which_window,
                                   gint               xpos,
                                   gint               ypos)
{
  CtkTextViewChild *vc;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (CTK_IS_WIDGET (child));
  g_return_if_fail (ctk_widget_get_parent (child) == NULL);

  vc = text_view_child_new_window (child, which_window,
                                   xpos, ypos);

  add_child (text_view, vc);

  g_assert (vc->widget == child);
  g_assert (ctk_widget_get_parent (child) == CTK_WIDGET (text_view));
}

/**
 * ctk_text_view_move_child:
 * @text_view: a #CtkTextView
 * @child: child widget already added to the text view
 * @xpos: new X position in window coordinates
 * @ypos: new Y position in window coordinates
 *
 * Updates the position of a child, as for ctk_text_view_add_child_in_window().
 **/
void
ctk_text_view_move_child (CtkTextView *text_view,
                          CtkWidget   *child,
                          gint         xpos,
                          gint         ypos)
{
  CtkTextViewChild *vc;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (CTK_IS_WIDGET (child));
  g_return_if_fail (ctk_widget_get_parent (child) == CTK_WIDGET (text_view));

  vc = g_object_get_qdata (G_OBJECT (child), quark_text_view_child);

  g_assert (vc != NULL);

  if (vc->x == xpos &&
      vc->y == ypos)
    return;
  
  vc->x = xpos;
  vc->y = ypos;

  if (ctk_widget_get_visible (child) &&
      ctk_widget_get_visible (CTK_WIDGET (text_view)))
    ctk_widget_queue_resize (child);
}


/* Iterator operations */

/**
 * ctk_text_view_forward_display_line:
 * @text_view: a #CtkTextView
 * @iter: a #CtkTextIter
 * 
 * Moves the given @iter forward by one display (wrapped) line.
 * A display line is different from a paragraph. Paragraphs are
 * separated by newlines or other paragraph separator characters.
 * Display lines are created by line-wrapping a paragraph. If
 * wrapping is turned off, display lines and paragraphs will be the
 * same. Display lines are divided differently for each view, since
 * they depend on the view’s width; paragraphs are the same in all
 * views, since they depend on the contents of the #CtkTextBuffer.
 * 
 * Returns: %TRUE if @iter was moved and is not on the end iterator
 **/
gboolean
ctk_text_view_forward_display_line (CtkTextView *text_view,
                                    CtkTextIter *iter)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  ctk_text_view_ensure_layout (text_view);

  return ctk_text_layout_move_iter_to_next_line (text_view->priv->layout, iter);
}

/**
 * ctk_text_view_backward_display_line:
 * @text_view: a #CtkTextView
 * @iter: a #CtkTextIter
 * 
 * Moves the given @iter backward by one display (wrapped) line.
 * A display line is different from a paragraph. Paragraphs are
 * separated by newlines or other paragraph separator characters.
 * Display lines are created by line-wrapping a paragraph. If
 * wrapping is turned off, display lines and paragraphs will be the
 * same. Display lines are divided differently for each view, since
 * they depend on the view’s width; paragraphs are the same in all
 * views, since they depend on the contents of the #CtkTextBuffer.
 * 
 * Returns: %TRUE if @iter was moved and is not on the end iterator
 **/
gboolean
ctk_text_view_backward_display_line (CtkTextView *text_view,
                                     CtkTextIter *iter)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  ctk_text_view_ensure_layout (text_view);

  return ctk_text_layout_move_iter_to_previous_line (text_view->priv->layout, iter);
}

/**
 * ctk_text_view_forward_display_line_end:
 * @text_view: a #CtkTextView
 * @iter: a #CtkTextIter
 * 
 * Moves the given @iter forward to the next display line end.
 * A display line is different from a paragraph. Paragraphs are
 * separated by newlines or other paragraph separator characters.
 * Display lines are created by line-wrapping a paragraph. If
 * wrapping is turned off, display lines and paragraphs will be the
 * same. Display lines are divided differently for each view, since
 * they depend on the view’s width; paragraphs are the same in all
 * views, since they depend on the contents of the #CtkTextBuffer.
 * 
 * Returns: %TRUE if @iter was moved and is not on the end iterator
 **/
gboolean
ctk_text_view_forward_display_line_end (CtkTextView *text_view,
                                        CtkTextIter *iter)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  ctk_text_view_ensure_layout (text_view);

  return ctk_text_layout_move_iter_to_line_end (text_view->priv->layout, iter, 1);
}

/**
 * ctk_text_view_backward_display_line_start:
 * @text_view: a #CtkTextView
 * @iter: a #CtkTextIter
 * 
 * Moves the given @iter backward to the next display line start.
 * A display line is different from a paragraph. Paragraphs are
 * separated by newlines or other paragraph separator characters.
 * Display lines are created by line-wrapping a paragraph. If
 * wrapping is turned off, display lines and paragraphs will be the
 * same. Display lines are divided differently for each view, since
 * they depend on the view’s width; paragraphs are the same in all
 * views, since they depend on the contents of the #CtkTextBuffer.
 * 
 * Returns: %TRUE if @iter was moved and is not on the end iterator
 **/
gboolean
ctk_text_view_backward_display_line_start (CtkTextView *text_view,
                                           CtkTextIter *iter)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  ctk_text_view_ensure_layout (text_view);

  return ctk_text_layout_move_iter_to_line_end (text_view->priv->layout, iter, -1);
}

/**
 * ctk_text_view_starts_display_line:
 * @text_view: a #CtkTextView
 * @iter: a #CtkTextIter
 * 
 * Determines whether @iter is at the start of a display line.
 * See ctk_text_view_forward_display_line() for an explanation of
 * display lines vs. paragraphs.
 * 
 * Returns: %TRUE if @iter begins a wrapped line
 **/
gboolean
ctk_text_view_starts_display_line (CtkTextView       *text_view,
                                   const CtkTextIter *iter)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  ctk_text_view_ensure_layout (text_view);

  return ctk_text_layout_iter_starts_line (text_view->priv->layout, iter);
}

/**
 * ctk_text_view_move_visually:
 * @text_view: a #CtkTextView
 * @iter: a #CtkTextIter
 * @count: number of characters to move (negative moves left, 
 *    positive moves right)
 *
 * Move the iterator a given number of characters visually, treating
 * it as the strong cursor position. If @count is positive, then the
 * new strong cursor position will be @count positions to the right of
 * the old cursor position. If @count is negative then the new strong
 * cursor position will be @count positions to the left of the old
 * cursor position.
 *
 * In the presence of bi-directional text, the correspondence
 * between logical and visual order will depend on the direction
 * of the current run, and there may be jumps when the cursor
 * is moved off of the end of a run.
 * 
 * Returns: %TRUE if @iter moved and is not on the end iterator
 **/
gboolean
ctk_text_view_move_visually (CtkTextView *text_view,
                             CtkTextIter *iter,
                             gint         count)
{
  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  ctk_text_view_ensure_layout (text_view);

  return ctk_text_layout_move_iter_visually (text_view->priv->layout, iter, count);
}

/**
 * ctk_text_view_set_input_purpose:
 * @text_view: a #CtkTextView
 * @purpose: the purpose
 *
 * Sets the #CtkTextView:input-purpose property which
 * can be used by on-screen keyboards and other input
 * methods to adjust their behaviour.
 *
 * Since: 3.6
 */

void
ctk_text_view_set_input_purpose (CtkTextView     *text_view,
                                 CtkInputPurpose  purpose)

{
  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  if (ctk_text_view_get_input_purpose (text_view) != purpose)
    {
      g_object_set (G_OBJECT (text_view->priv->im_context),
                    "input-purpose", purpose,
                    NULL);

      g_object_notify (G_OBJECT (text_view), "input-purpose");
    }
}

/**
 * ctk_text_view_get_input_purpose:
 * @text_view: a #CtkTextView
 *
 * Gets the value of the #CtkTextView:input-purpose property.
 *
 * Since: 3.6
 */

CtkInputPurpose
ctk_text_view_get_input_purpose (CtkTextView *text_view)
{
  CtkInputPurpose purpose;

  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), CTK_INPUT_PURPOSE_FREE_FORM);

  g_object_get (G_OBJECT (text_view->priv->im_context),
                "input-purpose", &purpose,
                NULL);

  return purpose;
}

/**
 * ctk_text_view_set_input_hints:
 * @text_view: a #CtkTextView
 * @hints: the hints
 *
 * Sets the #CtkTextView:input-hints property, which
 * allows input methods to fine-tune their behaviour.
 *
 * Since: 3.6
 */

void
ctk_text_view_set_input_hints (CtkTextView   *text_view,
                               CtkInputHints  hints)

{
  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  if (ctk_text_view_get_input_hints (text_view) != hints)
    {
      g_object_set (G_OBJECT (text_view->priv->im_context),
                    "input-hints", hints,
                    NULL);

      g_object_notify (G_OBJECT (text_view), "input-hints");
    }
}

/**
 * ctk_text_view_get_input_hints:
 * @text_view: a #CtkTextView
 *
 * Gets the value of the #CtkTextView:input-hints property.
 *
 * Since: 3.6
 */

CtkInputHints
ctk_text_view_get_input_hints (CtkTextView *text_view)
{
  CtkInputHints hints;

  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), CTK_INPUT_HINT_NONE);

  g_object_get (G_OBJECT (text_view->priv->im_context),
                "input-hints", &hints,
                NULL);

  return hints;
}

/**
 * ctk_text_view_set_monospace:
 * @text_view: a #CtkTextView
 * @monospace: %TRUE to request monospace styling
 *
 * Sets the #CtkTextView:monospace property, which
 * indicates that the text view should use monospace
 * fonts.
 *
 * Since: 3.16
 */
void
ctk_text_view_set_monospace (CtkTextView *text_view,
                             gboolean     monospace)
{
  CtkStyleContext *context;
  gboolean has_monospace;

  g_return_if_fail (CTK_IS_TEXT_VIEW (text_view));

  context = ctk_widget_get_style_context (CTK_WIDGET (text_view));
  has_monospace = ctk_style_context_has_class (context, CTK_STYLE_CLASS_MONOSPACE);

  if (has_monospace != monospace)
    {
      if (monospace)
        ctk_style_context_add_class (context, CTK_STYLE_CLASS_MONOSPACE);
      else
        ctk_style_context_remove_class (context, CTK_STYLE_CLASS_MONOSPACE);
      g_object_notify (G_OBJECT (text_view), "monospace");
    }
}

/**
 * ctk_text_view_get_monospace:
 * @text_view: a #CtkTextView
 *
 * Gets the value of the #CtkTextView:monospace property.
 *
 * Return: %TRUE if monospace fonts are desired
 *
 * Since: 3.16
 */
gboolean
ctk_text_view_get_monospace (CtkTextView *text_view)
{
  CtkStyleContext *context;

  g_return_val_if_fail (CTK_IS_TEXT_VIEW (text_view), FALSE);

  context = ctk_widget_get_style_context (CTK_WIDGET (text_view));
  
  return ctk_style_context_has_class (context, CTK_STYLE_CLASS_MONOSPACE);
}

static void
ctk_text_view_insert_emoji (CtkTextView *text_view)
{
  CtkWidget *chooser;
  CtkTextIter iter;
  CdkRectangle rect;
  CtkTextBuffer *buffer;

  if (ctk_text_view_get_input_hints (text_view) & CTK_INPUT_HINT_NO_EMOJI)
    return;

  if (ctk_widget_get_ancestor (CTK_WIDGET (text_view), CTK_TYPE_EMOJI_CHOOSER) != NULL)
    return;

  chooser = CTK_WIDGET (g_object_get_data (G_OBJECT (text_view), "ctk-emoji-chooser"));
  if (!chooser)
    {
      chooser = ctk_emoji_chooser_new ();
      g_object_set_data (G_OBJECT (text_view), "ctk-emoji-chooser", chooser);

      ctk_popover_set_relative_to (CTK_POPOVER (chooser), CTK_WIDGET (text_view));
      g_signal_connect_swapped (chooser, "emoji-picked",
                                G_CALLBACK (ctk_text_view_insert_at_cursor), text_view);
    }

  buffer = get_buffer (text_view);
  ctk_text_buffer_get_iter_at_mark (buffer, &iter,
                                    ctk_text_buffer_get_insert (buffer));

  ctk_text_view_get_iter_location (text_view, &iter, (CdkRectangle *) &rect);
  ctk_text_view_buffer_to_window_coords (text_view, CTK_TEXT_WINDOW_TEXT,
                                         rect.x, rect.y, &rect.x, &rect.y);
  _text_window_to_widget_coords (text_view, &rect.x, &rect.y);

  ctk_popover_set_pointing_to (CTK_POPOVER (chooser), &rect);

  ctk_popover_popup (CTK_POPOVER (chooser));
}
