/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2004-2006 Christian Hammond
 * Copyright (C) 2008 Cody Russell
 * Copyright (C) 2008 Red Hat, Inc.
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

#include "ctkbindings.h"
#include "ctkcelleditable.h"
#include "ctkclipboard.h"
#include "ctkdebug.h"
#include "ctkdnd.h"
#include "ctkdndprivate.h"
#include "ctkentry.h"
#include "ctkentrybuffer.h"
#include "ctkiconhelperprivate.h"
#include "ctkemojichooser.h"
#include "ctkemojicompletion.h"
#include "ctkentrybuffer.h"
#include "ctkimcontextsimple.h"
#include "ctkimmulticontext.h"
#include "ctkintl.h"
#include "ctklabel.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkmenu.h"
#include "ctkmenuitem.h"
#include "ctkpango.h"
#include "ctkseparatormenuitem.h"
#include "ctkselection.h"
#include "ctksettings.h"
#include "ctkspinbutton.h"
#include "ctktextutil.h"
#include "ctkwindow.h"
#include "ctktreeview.h"
#include "ctktreeselection.h"
#include "ctktypebuiltins.h"
#include "ctkprivate.h"
#include "ctkentryprivate.h"
#include "ctkcelllayout.h"
#include "ctktooltip.h"
#include "ctkicontheme.h"
#include "ctkwidgetprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctktexthandleprivate.h"
#include "ctkpopover.h"
#include "ctktoolbar.h"
#include "ctkmagnifierprivate.h"
#include "ctkcssnodeprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkprogresstrackerprivate.h"
#include "ctkemojichooser.h"
#include "ctkwindow.h"

#include "a11y/ctkentryaccessible.h"

#include "fallback-c89.c"

/**
 * SECTION:ctkentry
 * @Short_description: A single line text entry field
 * @Title: CtkEntry
 * @See_also: #CtkTextView, #CtkEntryCompletion
 *
 * The #CtkEntry widget is a single line text entry
 * widget. A fairly large set of key bindings are supported
 * by default. If the entered text is longer than the allocation
 * of the widget, the widget will scroll so that the cursor
 * position is visible.
 *
 * When using an entry for passwords and other sensitive information,
 * it can be put into “password mode” using ctk_entry_set_visibility().
 * In this mode, entered text is displayed using a “invisible” character.
 * By default, CTK+ picks the best invisible character that is available
 * in the current font, but it can be changed with
 * ctk_entry_set_invisible_char(). Since 2.16, CTK+ displays a warning
 * when Caps Lock or input methods might interfere with entering text in
 * a password entry. The warning can be turned off with the
 * #CtkEntry:caps-lock-warning property.
 *
 * Since 2.16, CtkEntry has the ability to display progress or activity
 * information behind the text. To make an entry display such information,
 * use ctk_entry_set_progress_fraction() or ctk_entry_set_progress_pulse_step().
 *
 * Additionally, CtkEntry can show icons at either side of the entry. These
 * icons can be activatable by clicking, can be set up as drag source and
 * can have tooltips. To add an icon, use ctk_entry_set_icon_from_gicon() or
 * one of the various other functions that set an icon from a stock id, an
 * icon name or a pixbuf. To trigger an action when the user clicks an icon,
 * connect to the #CtkEntry::icon-press signal. To allow DND operations
 * from an icon, use ctk_entry_set_icon_drag_source(). To set a tooltip on
 * an icon, use ctk_entry_set_icon_tooltip_text() or the corresponding function
 * for markup.
 *
 * Note that functionality or information that is only available by clicking
 * on an icon in an entry may not be accessible at all to users which are not
 * able to use a mouse or other pointing device. It is therefore recommended
 * that any such functionality should also be available by other means, e.g.
 * via the context menu of the entry.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * entry[.read-only][.flat][.warning][.error]
 * ├── image.left
 * ├── image.right
 * ├── undershoot.left
 * ├── undershoot.right
 * ├── [selection]
 * ├── [progress[.pulse]]
 * ╰── [window.popup]
 * ]|
 *
 * CtkEntry has a main node with the name entry. Depending on the properties
 * of the entry, the style classes .read-only and .flat may appear. The style
 * classes .warning and .error may also be used with entries.
 *
 * When the entry shows icons, it adds subnodes with the name image and the
 * style class .left or .right, depending on where the icon appears.
 *
 * When the entry has a selection, it adds a subnode with the name selection.
 *
 * When the entry shows progress, it adds a subnode with the name progress.
 * The node has the style class .pulse when the shown progress is pulsing.
 *
 * The CSS node for a context menu is added as a subnode below entry as well.
 *
 * The undershoot nodes are used to draw the underflow indication when content
 * is scrolled out of view. These nodes get the .left and .right style classes
 * added depending on where the indication is drawn.
 *
 * When touch is used and touch selection handles are shown, they are using
 * CSS nodes with name cursor-handle. They get the .top or .bottom style class
 * depending on where they are shown in relation to the selection. If there is
 * just a single handle for the text cursor, it gets the style class
 * .insertion-cursor.
 */

#define MIN_ENTRY_WIDTH  150

#define MAX_ICONS 2

#define IS_VALID_ICON_POSITION(pos)               \
  ((pos) == CTK_ENTRY_ICON_PRIMARY ||                   \
   (pos) == CTK_ENTRY_ICON_SECONDARY)

static GQuark          quark_inner_border   = 0;
static GQuark          quark_password_hint  = 0;
static GQuark          quark_cursor_hadjustment = 0;
static GQuark          quark_capslock_feedback = 0;
static GQuark          quark_ctk_signal = 0;
static GQuark          quark_entry_completion = 0;

typedef struct _EntryIconInfo EntryIconInfo;
typedef struct _CtkEntryPasswordHint CtkEntryPasswordHint;
typedef struct _CtkEntryCapslockFeedback CtkEntryCapslockFeedback;

struct _CtkEntryPrivate
{
  EntryIconInfo         *icons[MAX_ICONS];

  CtkEntryBuffer        *buffer;
  CtkIMContext          *im_context;
  CtkWidget             *popup_menu;

  GdkWindow             *text_area;
  CtkAllocation          text_allocation;
  int                    text_baseline;

  PangoLayout           *cached_layout;
  PangoAttrList         *attrs;
  PangoTabArray         *tabs;

  gchar        *im_module;

  gdouble       progress_fraction;
  gdouble       progress_pulse_fraction;
  gdouble       progress_pulse_current;

  guint              tick_id;
  CtkProgressTracker tracker;
  gint64             pulse1;
  gint64             pulse2;
  gdouble            last_iteration;

  gchar        *placeholder_text;

  CtkTextHandle *text_handle;
  CtkWidget     *selection_bubble;
  guint          selection_bubble_timeout_id;

  CtkWidget     *magnifier_popover;
  CtkWidget     *magnifier;

  CtkGesture    *drag_gesture;
  CtkGesture    *multipress_gesture;

  CtkCssGadget  *gadget;
  CtkCssGadget  *progress_gadget;
  CtkCssNode    *selection_node;
  CtkCssNode    *undershoot_node[2];

  gfloat        xalign;

  gint          ascent;                     /* font ascent in pango units  */
  gint          current_pos;
  gint          descent;                    /* font descent in pango units */
  gint          dnd_position;               /* In chars, -1 == no DND cursor */
  gint          drag_start_x;
  gint          drag_start_y;
  gint          insert_pos;
  gint          selection_bound;
  gint          scroll_offset;
  gint          start_x;
  gint          start_y;
  gint          width_chars;
  gint          max_width_chars;

  gunichar      invisible_char;

  guint         blink_time;                  /* time in msec the cursor has blinked since last user event */
  guint         blink_timeout;

  guint16       preedit_length;              /* length of preedit string, in bytes */
  guint16	preedit_cursor;	             /* offset of cursor within preedit string, in chars */

  gint64        handle_place_time;

  guint         shadow_type             : 4;
  guint         editable                : 1;
  guint         show_emoji_icon         : 1;
  guint         enable_emoji_completion : 1;
  guint         in_drag                 : 1;
  guint         overwrite_mode          : 1;
  guint         visible                 : 1;

  guint         activates_default       : 1;
  guint         cache_includes_preedit  : 1;
  guint         caps_lock_warning       : 1;
  guint         caps_lock_warning_shown : 1;
  guint         change_count            : 8;
  guint         cursor_visible          : 1;
  guint         editing_canceled        : 1; /* Only used by CtkCellRendererText */
  guint         in_click                : 1; /* Flag so we don't select all when clicking in entry to focus in */
  guint         invisible_char_set      : 1;
  guint         mouse_cursor_obscured   : 1;
  guint         need_im_reset           : 1;
  guint         progress_pulse_mode     : 1;
  guint         progress_pulse_way_back : 1;
  guint         real_changed            : 1;
  guint         resolved_dir            : 4; /* PangoDirection */
  guint         select_words            : 1;
  guint         select_lines            : 1;
  guint         truncate_multiline      : 1;
  guint         cursor_handle_dragged   : 1;
  guint         selection_handle_dragged : 1;
  guint         populate_all            : 1;
  guint         handling_key_event      : 1;
};

struct _EntryIconInfo
{
  GdkWindow *window;
  gchar *tooltip;
  guint insensitive    : 1;
  guint nonactivatable : 1;
  guint prelight       : 1;
  guint in_drag        : 1;
  guint pressed        : 1;

  GdkDragAction actions;
  CtkTargetList *target_list;
  CtkCssGadget *gadget;
  GdkEventSequence *current_sequence;
  GdkDevice *device;
};

struct _CtkEntryPasswordHint
{
  gint position;      /* Position (in text) of the last password hint */
  guint source_id;    /* Timeout source id */
};

struct _CtkEntryCapslockFeedback
{
  CtkWidget *entry;
  CtkWidget *window;
  CtkWidget *label;
};

enum {
  ACTIVATE,
  POPULATE_POPUP,
  MOVE_CURSOR,
  INSERT_AT_CURSOR,
  DELETE_FROM_CURSOR,
  BACKSPACE,
  CUT_CLIPBOARD,
  COPY_CLIPBOARD,
  PASTE_CLIPBOARD,
  TOGGLE_OVERWRITE,
  ICON_PRESS,
  ICON_RELEASE,
  PREEDIT_CHANGED,
  INSERT_EMOJI,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_BUFFER,
  PROP_CURSOR_POSITION,
  PROP_SELECTION_BOUND,
  PROP_EDITABLE,
  PROP_MAX_LENGTH,
  PROP_VISIBILITY,
  PROP_HAS_FRAME,
  PROP_INNER_BORDER,
  PROP_INVISIBLE_CHAR,
  PROP_ACTIVATES_DEFAULT,
  PROP_WIDTH_CHARS,
  PROP_MAX_WIDTH_CHARS,
  PROP_SCROLL_OFFSET,
  PROP_TEXT,
  PROP_XALIGN,
  PROP_TRUNCATE_MULTILINE,
  PROP_SHADOW_TYPE,
  PROP_OVERWRITE_MODE,
  PROP_TEXT_LENGTH,
  PROP_INVISIBLE_CHAR_SET,
  PROP_CAPS_LOCK_WARNING,
  PROP_PROGRESS_FRACTION,
  PROP_PROGRESS_PULSE_STEP,
  PROP_PIXBUF_PRIMARY,
  PROP_PIXBUF_SECONDARY,
  PROP_STOCK_PRIMARY,
  PROP_STOCK_SECONDARY,
  PROP_ICON_NAME_PRIMARY,
  PROP_ICON_NAME_SECONDARY,
  PROP_GICON_PRIMARY,
  PROP_GICON_SECONDARY,
  PROP_STORAGE_TYPE_PRIMARY,
  PROP_STORAGE_TYPE_SECONDARY,
  PROP_ACTIVATABLE_PRIMARY,
  PROP_ACTIVATABLE_SECONDARY,
  PROP_SENSITIVE_PRIMARY,
  PROP_SENSITIVE_SECONDARY,
  PROP_TOOLTIP_TEXT_PRIMARY,
  PROP_TOOLTIP_TEXT_SECONDARY,
  PROP_TOOLTIP_MARKUP_PRIMARY,
  PROP_TOOLTIP_MARKUP_SECONDARY,
  PROP_IM_MODULE,
  PROP_PLACEHOLDER_TEXT,
  PROP_COMPLETION,
  PROP_INPUT_PURPOSE,
  PROP_INPUT_HINTS,
  PROP_ATTRIBUTES,
  PROP_POPULATE_ALL,
  PROP_TABS,
  PROP_SHOW_EMOJI_ICON,
  PROP_ENABLE_EMOJI_COMPLETION,
  PROP_EDITING_CANCELED,
  NUM_PROPERTIES = PROP_EDITING_CANCELED
};

static GParamSpec *entry_props[NUM_PROPERTIES] = { NULL, };

static guint signals[LAST_SIGNAL] = { 0 };

typedef enum {
  CURSOR_STANDARD,
  CURSOR_DND
} CursorType;

typedef enum
{
  DISPLAY_NORMAL,       /* The entry text is being shown */
  DISPLAY_INVISIBLE,    /* In invisible mode, text replaced by (eg) bullets */
  DISPLAY_BLANK         /* In invisible mode, nothing shown at all */
} DisplayMode;

/* GObject methods
 */
static void   ctk_entry_editable_init        (CtkEditableInterface *iface);
static void   ctk_entry_cell_editable_init   (CtkCellEditableIface *iface);
static void   ctk_entry_set_property         (GObject          *object,
                                              guint             prop_id,
                                              const GValue     *value,
                                              GParamSpec       *pspec);
static void   ctk_entry_get_property         (GObject          *object,
                                              guint             prop_id,
                                              GValue           *value,
                                              GParamSpec       *pspec);
static void   ctk_entry_finalize             (GObject          *object);
static void   ctk_entry_dispose              (GObject          *object);

/* CtkWidget methods
 */
static void   ctk_entry_destroy              (CtkWidget        *widget);
static void   ctk_entry_realize              (CtkWidget        *widget);
static void   ctk_entry_unrealize            (CtkWidget        *widget);
static void   ctk_entry_map                  (CtkWidget        *widget);
static void   ctk_entry_unmap                (CtkWidget        *widget);
static void   ctk_entry_get_preferred_width  (CtkWidget        *widget,
                                              gint             *minimum,
                                              gint             *natural);
static void   ctk_entry_get_preferred_height (CtkWidget        *widget,
                                              gint             *minimum,
                                              gint             *natural);
static void  ctk_entry_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
                                                                    gint       width,
                                                                    gint      *minimum_height,
                                                                    gint      *natural_height,
                                                                    gint      *minimum_baseline,
                                                                    gint      *natural_baseline);
static void   ctk_entry_size_allocate        (CtkWidget        *widget,
					      CtkAllocation    *allocation);
static gint   ctk_entry_draw                 (CtkWidget        *widget,
                                              cairo_t          *cr);
static gboolean ctk_entry_event              (CtkWidget        *widget,
                                              GdkEvent         *event);
static gint   ctk_entry_enter_notify         (CtkWidget        *widget,
                                              GdkEventCrossing *event);
static gint   ctk_entry_leave_notify         (CtkWidget        *widget,
                                              GdkEventCrossing *event);
static gint   ctk_entry_key_press            (CtkWidget        *widget,
					      GdkEventKey      *event);
static gint   ctk_entry_key_release          (CtkWidget        *widget,
					      GdkEventKey      *event);
static gint   ctk_entry_focus_in             (CtkWidget        *widget,
					      GdkEventFocus    *event);
static gint   ctk_entry_focus_out            (CtkWidget        *widget,
					      GdkEventFocus    *event);
static void   ctk_entry_grab_focus           (CtkWidget        *widget);
static void   ctk_entry_style_updated        (CtkWidget        *widget);
static gboolean ctk_entry_query_tooltip      (CtkWidget        *widget,
                                              gint              x,
                                              gint              y,
                                              gboolean          keyboard_tip,
                                              CtkTooltip       *tooltip);
static void   ctk_entry_direction_changed    (CtkWidget        *widget,
					      CtkTextDirection  previous_dir);
static void   ctk_entry_state_flags_changed  (CtkWidget        *widget,
					      CtkStateFlags     previous_state);
static void   ctk_entry_screen_changed       (CtkWidget        *widget,
					      GdkScreen        *old_screen);

static gboolean ctk_entry_drag_drop          (CtkWidget        *widget,
                                              GdkDragContext   *context,
                                              gint              x,
                                              gint              y,
                                              guint             time);
static gboolean ctk_entry_drag_motion        (CtkWidget        *widget,
					      GdkDragContext   *context,
					      gint              x,
					      gint              y,
					      guint             time);
static void     ctk_entry_drag_leave         (CtkWidget        *widget,
					      GdkDragContext   *context,
					      guint             time);
static void     ctk_entry_drag_data_received (CtkWidget        *widget,
					      GdkDragContext   *context,
					      gint              x,
					      gint              y,
					      CtkSelectionData *selection_data,
					      guint             info,
					      guint             time);
static void     ctk_entry_drag_data_get      (CtkWidget        *widget,
					      GdkDragContext   *context,
					      CtkSelectionData *selection_data,
					      guint             info,
					      guint             time);
static void     ctk_entry_drag_data_delete   (CtkWidget        *widget,
					      GdkDragContext   *context);
static void     ctk_entry_drag_begin         (CtkWidget        *widget,
                                              GdkDragContext   *context);
static void     ctk_entry_drag_end           (CtkWidget        *widget,
                                              GdkDragContext   *context);


/* CtkEditable method implementations
 */
static void     ctk_entry_insert_text          (CtkEditable *editable,
						const gchar *new_text,
						gint         new_text_length,
						gint        *position);
static void     ctk_entry_delete_text          (CtkEditable *editable,
						gint         start_pos,
						gint         end_pos);
static gchar *  ctk_entry_get_chars            (CtkEditable *editable,
						gint         start_pos,
						gint         end_pos);
static void     ctk_entry_real_set_position    (CtkEditable *editable,
						gint         position);
static gint     ctk_entry_get_position         (CtkEditable *editable);
static void     ctk_entry_set_selection_bounds (CtkEditable *editable,
						gint         start,
						gint         end);
static gboolean ctk_entry_get_selection_bounds (CtkEditable *editable,
						gint        *start,
						gint        *end);

/* CtkCellEditable method implementations
 */
static void ctk_entry_start_editing (CtkCellEditable *cell_editable,
				     GdkEvent        *event);

/* Default signal handlers
 */
static void ctk_entry_real_insert_text   (CtkEditable     *editable,
					  const gchar     *new_text,
					  gint             new_text_length,
					  gint            *position);
static void ctk_entry_real_delete_text   (CtkEditable     *editable,
					  gint             start_pos,
					  gint             end_pos);
static void ctk_entry_move_cursor        (CtkEntry        *entry,
					  CtkMovementStep  step,
					  gint             count,
					  gboolean         extend_selection);
static void ctk_entry_insert_at_cursor   (CtkEntry        *entry,
					  const gchar     *str);
static void ctk_entry_delete_from_cursor (CtkEntry        *entry,
					  CtkDeleteType    type,
					  gint             count);
static void ctk_entry_backspace          (CtkEntry        *entry);
static void ctk_entry_cut_clipboard      (CtkEntry        *entry);
static void ctk_entry_copy_clipboard     (CtkEntry        *entry);
static void ctk_entry_paste_clipboard    (CtkEntry        *entry);
static void ctk_entry_toggle_overwrite   (CtkEntry        *entry);
static void ctk_entry_insert_emoji       (CtkEntry        *entry);
static void ctk_entry_select_all         (CtkEntry        *entry);
static void ctk_entry_real_activate      (CtkEntry        *entry);
static gboolean ctk_entry_popup_menu     (CtkWidget       *widget);

static void keymap_direction_changed     (GdkKeymap       *keymap,
					  CtkEntry        *entry);
static void keymap_state_changed         (GdkKeymap       *keymap,
					  CtkEntry        *entry);
static void remove_capslock_feedback     (CtkEntry        *entry);

/* IM Context Callbacks
 */
static void     ctk_entry_commit_cb               (CtkIMContext *context,
						   const gchar  *str,
						   CtkEntry     *entry);
static void     ctk_entry_preedit_changed_cb      (CtkIMContext *context,
						   CtkEntry     *entry);
static gboolean ctk_entry_retrieve_surrounding_cb (CtkIMContext *context,
						   CtkEntry     *entry);
static gboolean ctk_entry_delete_surrounding_cb   (CtkIMContext *context,
						   gint          offset,
						   gint          n_chars,
						   CtkEntry     *entry);

/* Event controller callbacks */
static void   ctk_entry_multipress_gesture_pressed (CtkGestureMultiPress *gesture,
                                                    gint                  n_press,
                                                    gdouble               x,
                                                    gdouble               y,
                                                    CtkEntry             *entry);
static void   ctk_entry_drag_gesture_update        (CtkGestureDrag *gesture,
                                                    gdouble         offset_x,
                                                    gdouble         offset_y,
                                                    CtkEntry       *entry);
static void   ctk_entry_drag_gesture_end           (CtkGestureDrag *gesture,
                                                    gdouble         offset_x,
                                                    gdouble         offset_y,
                                                    CtkEntry       *entry);

/* Internal routines
 */
static void         ctk_entry_draw_text                (CtkEntry       *entry,
                                                        cairo_t        *cr);
static void         ctk_entry_draw_cursor              (CtkEntry       *entry,
                                                        cairo_t        *cr,
							CursorType      type);
static PangoLayout *ctk_entry_ensure_layout            (CtkEntry       *entry,
                                                        gboolean        include_preedit);
static void         ctk_entry_reset_layout             (CtkEntry       *entry);
static void         ctk_entry_recompute                (CtkEntry       *entry);
static gint         ctk_entry_find_position            (CtkEntry       *entry,
							gint            x);
static void         ctk_entry_get_cursor_locations     (CtkEntry       *entry,
							CursorType      type,
							gint           *strong_x,
							gint           *weak_x);
static void         ctk_entry_adjust_scroll            (CtkEntry       *entry);
static gint         ctk_entry_move_visually            (CtkEntry       *editable,
							gint            start,
							gint            count);
static gint         ctk_entry_move_logically           (CtkEntry       *entry,
							gint            start,
							gint            count);
static gint         ctk_entry_move_forward_word        (CtkEntry       *entry,
							gint            start,
                                                        gboolean        allow_whitespace);
static gint         ctk_entry_move_backward_word       (CtkEntry       *entry,
							gint            start,
                                                        gboolean        allow_whitespace);
static void         ctk_entry_delete_whitespace        (CtkEntry       *entry);
static void         ctk_entry_select_word              (CtkEntry       *entry);
static void         ctk_entry_select_line              (CtkEntry       *entry);
static void         ctk_entry_paste                    (CtkEntry       *entry,
							GdkAtom         selection);
static void         ctk_entry_update_primary_selection (CtkEntry       *entry);
static void         ctk_entry_do_popup                 (CtkEntry       *entry,
							const GdkEvent *event);
static gboolean     ctk_entry_mnemonic_activate        (CtkWidget      *widget,
							gboolean        group_cycling);
static void         ctk_entry_grab_notify              (CtkWidget      *widget,
                                                        gboolean        was_grabbed);
static void         ctk_entry_check_cursor_blink       (CtkEntry       *entry);
static void         ctk_entry_pend_cursor_blink        (CtkEntry       *entry);
static void         ctk_entry_reset_blink_time         (CtkEntry       *entry);
static void         ctk_entry_get_text_area_size       (CtkEntry       *entry,
							gint           *x,
							gint           *y,
							gint           *width,
							gint           *height);
static void         ctk_entry_get_frame_size           (CtkEntry       *entry,
							gint           *x,
							gint           *y,
							gint           *width,
							gint           *height);
static void         get_frame_size                     (CtkEntry       *entry,
                                                        gboolean        relative_to_window,
							gint           *x,
							gint           *y,
							gint           *width,
							gint           *height);
static void         ctk_entry_move_adjustments         (CtkEntry             *entry);
static void         ctk_entry_update_cached_style_values(CtkEntry      *entry);
static gboolean     get_middle_click_paste             (CtkEntry *entry);
static void         ctk_entry_get_scroll_limits        (CtkEntry       *entry,
                                                        gint           *min_offset,
                                                        gint           *max_offset);

/* CtkTextHandle handlers */
static void         ctk_entry_handle_drag_started      (CtkTextHandle         *handle,
                                                        CtkTextHandlePosition  pos,
                                                        CtkEntry              *entry);
static void         ctk_entry_handle_dragged           (CtkTextHandle         *handle,
                                                        CtkTextHandlePosition  pos,
                                                        gint                   x,
                                                        gint                   y,
                                                        CtkEntry              *entry);
static void         ctk_entry_handle_drag_finished     (CtkTextHandle         *handle,
                                                        CtkTextHandlePosition  pos,
                                                        CtkEntry              *entry);

static void         ctk_entry_selection_bubble_popup_set   (CtkEntry *entry);
static void         ctk_entry_selection_bubble_popup_unset (CtkEntry *entry);

static void         begin_change                       (CtkEntry *entry);
static void         end_change                         (CtkEntry *entry);
static void         emit_changed                       (CtkEntry *entry);

static void         buffer_inserted_text               (CtkEntryBuffer *buffer, 
                                                        guint           position,
                                                        const gchar    *chars,
                                                        guint           n_chars,
                                                        CtkEntry       *entry);
static void         buffer_deleted_text                (CtkEntryBuffer *buffer, 
                                                        guint           position,
                                                        guint           n_chars,
                                                        CtkEntry       *entry);
static void         buffer_notify_text                 (CtkEntryBuffer *buffer, 
                                                        GParamSpec     *spec,
                                                        CtkEntry       *entry);
static void         buffer_notify_length               (CtkEntryBuffer *buffer, 
                                                        GParamSpec     *spec,
                                                        CtkEntry       *entry);
static void         buffer_notify_max_length           (CtkEntryBuffer *buffer, 
                                                        GParamSpec     *spec,
                                                        CtkEntry       *entry);
static void         buffer_connect_signals             (CtkEntry       *entry);
static void         buffer_disconnect_signals          (CtkEntry       *entry);
static CtkEntryBuffer *get_buffer                      (CtkEntry       *entry);
static void         set_show_emoji_icon                (CtkEntry       *entry,
                                                        gboolean        value);
static void         set_enable_emoji_completion        (CtkEntry       *entry,
                                                        gboolean        value);

static void     ctk_entry_measure  (CtkCssGadget        *gadget,
                                    CtkOrientation       orientation,
                                    int                  for_size,
                                    int                 *minimum,
                                    int                 *natural,
                                    int                 *minimum_baseline,
                                    int                 *natural_baseline,
                                    gpointer             data);
static void     ctk_entry_allocate (CtkCssGadget        *gadget,
                                    const CtkAllocation *allocation,
                                    int                  baseline,
                                    CtkAllocation       *out_clip,
                                    gpointer             data);
static gboolean ctk_entry_render   (CtkCssGadget        *gadget,
                                    cairo_t             *cr,
                                    int                  x,
                                    int                  y,
                                    int                  width,
                                    int                  height,
                                    gpointer             data);

G_DEFINE_TYPE_WITH_CODE (CtkEntry, ctk_entry, CTK_TYPE_WIDGET,
                         G_ADD_PRIVATE (CtkEntry)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_EDITABLE,
                                                ctk_entry_editable_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_CELL_EDITABLE,
                                                ctk_entry_cell_editable_init))

static void
add_move_binding (CtkBindingSet  *binding_set,
		  guint           keyval,
		  guint           modmask,
		  CtkMovementStep step,
		  gint            count)
{
  g_return_if_fail ((modmask & GDK_SHIFT_MASK) == 0);
  
  ctk_binding_entry_add_signal (binding_set, keyval, modmask,
				"move-cursor", 3,
				G_TYPE_ENUM, step,
				G_TYPE_INT, count,
				G_TYPE_BOOLEAN, FALSE);

  /* Selection-extending version */
  ctk_binding_entry_add_signal (binding_set, keyval, modmask | GDK_SHIFT_MASK,
				"move-cursor", 3,
				G_TYPE_ENUM, step,
				G_TYPE_INT, count,
				G_TYPE_BOOLEAN, TRUE);
}

static void
ctk_entry_class_init (CtkEntryClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class;
  CtkBindingSet *binding_set;

  widget_class = (CtkWidgetClass*) class;

  gobject_class->dispose = ctk_entry_dispose;
  gobject_class->finalize = ctk_entry_finalize;
  gobject_class->set_property = ctk_entry_set_property;
  gobject_class->get_property = ctk_entry_get_property;

  widget_class->destroy = ctk_entry_destroy;
  widget_class->map = ctk_entry_map;
  widget_class->unmap = ctk_entry_unmap;
  widget_class->realize = ctk_entry_realize;
  widget_class->unrealize = ctk_entry_unrealize;
  widget_class->get_preferred_width = ctk_entry_get_preferred_width;
  widget_class->get_preferred_height = ctk_entry_get_preferred_height;
  widget_class->get_preferred_height_and_baseline_for_width = ctk_entry_get_preferred_height_and_baseline_for_width;
  widget_class->size_allocate = ctk_entry_size_allocate;
  widget_class->draw = ctk_entry_draw;
  widget_class->enter_notify_event = ctk_entry_enter_notify;
  widget_class->leave_notify_event = ctk_entry_leave_notify;
  widget_class->event = ctk_entry_event;
  widget_class->key_press_event = ctk_entry_key_press;
  widget_class->key_release_event = ctk_entry_key_release;
  widget_class->focus_in_event = ctk_entry_focus_in;
  widget_class->focus_out_event = ctk_entry_focus_out;
  widget_class->grab_focus = ctk_entry_grab_focus;
  widget_class->style_updated = ctk_entry_style_updated;
  widget_class->query_tooltip = ctk_entry_query_tooltip;
  widget_class->drag_begin = ctk_entry_drag_begin;
  widget_class->drag_end = ctk_entry_drag_end;
  widget_class->direction_changed = ctk_entry_direction_changed;
  widget_class->state_flags_changed = ctk_entry_state_flags_changed;
  widget_class->screen_changed = ctk_entry_screen_changed;
  widget_class->mnemonic_activate = ctk_entry_mnemonic_activate;
  widget_class->grab_notify = ctk_entry_grab_notify;

  widget_class->drag_drop = ctk_entry_drag_drop;
  widget_class->drag_motion = ctk_entry_drag_motion;
  widget_class->drag_leave = ctk_entry_drag_leave;
  widget_class->drag_data_received = ctk_entry_drag_data_received;
  widget_class->drag_data_get = ctk_entry_drag_data_get;
  widget_class->drag_data_delete = ctk_entry_drag_data_delete;

  widget_class->popup_menu = ctk_entry_popup_menu;

  class->move_cursor = ctk_entry_move_cursor;
  class->insert_at_cursor = ctk_entry_insert_at_cursor;
  class->delete_from_cursor = ctk_entry_delete_from_cursor;
  class->backspace = ctk_entry_backspace;
  class->cut_clipboard = ctk_entry_cut_clipboard;
  class->copy_clipboard = ctk_entry_copy_clipboard;
  class->paste_clipboard = ctk_entry_paste_clipboard;
  class->toggle_overwrite = ctk_entry_toggle_overwrite;
  class->insert_emoji = ctk_entry_insert_emoji;
  class->activate = ctk_entry_real_activate;
  class->get_text_area_size = ctk_entry_get_text_area_size;
  class->get_frame_size = ctk_entry_get_frame_size;
  
  quark_inner_border = g_quark_from_static_string ("ctk-entry-inner-border");
  quark_password_hint = g_quark_from_static_string ("ctk-entry-password-hint");
  quark_cursor_hadjustment = g_quark_from_static_string ("ctk-hadjustment");
  quark_capslock_feedback = g_quark_from_static_string ("ctk-entry-capslock-feedback");
  quark_ctk_signal = g_quark_from_static_string ("ctk-signal");
  quark_entry_completion = g_quark_from_static_string ("ctk-entry-completion-key");

  g_object_class_override_property (gobject_class,
                                    PROP_EDITING_CANCELED,
                                    "editing-canceled");

  entry_props[PROP_BUFFER] =
      g_param_spec_object ("buffer",
                           P_("Text Buffer"),
                           P_("Text buffer object which actually stores entry text"),
                           CTK_TYPE_ENTRY_BUFFER,
                           CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  entry_props[PROP_CURSOR_POSITION] =
      g_param_spec_int ("cursor-position",
                        P_("Cursor Position"),
                        P_("The current position of the insertion cursor in chars"),
                        0, CTK_ENTRY_BUFFER_MAX_SIZE,
                        0,
                        CTK_PARAM_READABLE);

  entry_props[PROP_SELECTION_BOUND] =
      g_param_spec_int ("selection-bound",
                        P_("Selection Bound"),
                        P_("The position of the opposite end of the selection from the cursor in chars"),
                        0, CTK_ENTRY_BUFFER_MAX_SIZE,
                        0,
                        CTK_PARAM_READABLE);

  entry_props[PROP_EDITABLE] =
      g_param_spec_boolean ("editable",
                            P_("Editable"),
                            P_("Whether the entry contents can be edited"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  entry_props[PROP_MAX_LENGTH] =
      g_param_spec_int ("max-length",
                        P_("Maximum length"),
                        P_("Maximum number of characters for this entry. Zero if no maximum"),
                        0, CTK_ENTRY_BUFFER_MAX_SIZE,
                        0,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  entry_props[PROP_VISIBILITY] =
      g_param_spec_boolean ("visibility",
                            P_("Visibility"),
                            P_("FALSE displays the \"invisible char\" instead of the actual text (password mode)"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  entry_props[PROP_HAS_FRAME] =
      g_param_spec_boolean ("has-frame",
                            P_("Has Frame"),
                            P_("FALSE removes outside bevel from entry"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:inner-border:
   *
   * Sets the text area's border between the text and the frame.
   *
   * Deprecated: 3.4: Use the standard border and padding CSS properties
   *   (through objects like #CtkStyleContext and #CtkCssProvider); the value
   *   of this style property is ignored.
   */
  entry_props[PROP_INNER_BORDER] =
      g_param_spec_boxed ("inner-border",
                          P_("Inner Border"),
                          P_("Border between text and frame. Overrides the inner-border style property"),
                          CTK_TYPE_BORDER,
                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  entry_props[PROP_INVISIBLE_CHAR] =
      g_param_spec_unichar ("invisible-char",
                            P_("Invisible character"),
                            P_("The character to use when masking entry contents (in \"password mode\")"),
                            '*',
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  entry_props[PROP_ACTIVATES_DEFAULT] =
      g_param_spec_boolean ("activates-default",
                            P_("Activates default"),
                            P_("Whether to activate the default widget (such as the default button in a dialog) when Enter is pressed"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  entry_props[PROP_WIDTH_CHARS] =
      g_param_spec_int ("width-chars",
                        P_("Width in chars"),
                        P_("Number of characters to leave space for in the entry"),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:max-width-chars:
   *
   * The desired maximum width of the entry, in characters.
   * If this property is set to -1, the width will be calculated
   * automatically.
   *
   * Since: 3.12
   */
  entry_props[PROP_MAX_WIDTH_CHARS] =
      g_param_spec_int ("max-width-chars",
                        P_("Maximum width in characters"),
                        P_("The desired maximum width of the entry, in characters"),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  entry_props[PROP_SCROLL_OFFSET] =
      g_param_spec_int ("scroll-offset",
                        P_("Scroll offset"),
                        P_("Number of pixels of the entry scrolled off the screen to the left"),
                        0, G_MAXINT,
                        0,
                        CTK_PARAM_READABLE|G_PARAM_EXPLICIT_NOTIFY);

  entry_props[PROP_TEXT] =
      g_param_spec_string ("text",
                           P_("Text"),
                           P_("The contents of the entry"),
                           "",
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:xalign:
   *
   * The horizontal alignment, from 0 (left) to 1 (right).
   * Reversed for RTL layouts.
   *
   * Since: 2.4
   */
  entry_props[PROP_XALIGN] =
      g_param_spec_float ("xalign",
                          P_("X align"),
                          P_("The horizontal alignment, from 0 (left) to 1 (right). Reversed for RTL layouts."),
                          0.0, 1.0,
                          0.0,
                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:truncate-multiline:
   *
   * When %TRUE, pasted multi-line text is truncated to the first line.
   *
   * Since: 2.10
   */
  entry_props[PROP_TRUNCATE_MULTILINE] =
      g_param_spec_boolean ("truncate-multiline",
                            P_("Truncate multiline"),
                            P_("Whether to truncate multiline pastes to one line."),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:shadow-type:
   *
   * Which kind of shadow to draw around the entry when
   * #CtkEntry:has-frame is set to %TRUE.
   *
   * Deprecated: 3.20: Use CSS to determine the style of the border;
   *     the value of this style property is ignored.
   *
   * Since: 2.12
   */
  entry_props[PROP_SHADOW_TYPE] =
      g_param_spec_enum ("shadow-type",
                         P_("Shadow type"),
                         P_("Which kind of shadow to draw around the entry when has-frame is set"),
                         CTK_TYPE_SHADOW_TYPE,
                         CTK_SHADOW_IN,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  /**
   * CtkEntry:overwrite-mode:
   *
   * If text is overwritten when typing in the #CtkEntry.
   *
   * Since: 2.14
   */
  entry_props[PROP_OVERWRITE_MODE] =
      g_param_spec_boolean ("overwrite-mode",
                            P_("Overwrite mode"),
                            P_("Whether new text overwrites existing text"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:text-length:
   *
   * The length of the text in the #CtkEntry.
   *
   * Since: 2.14
   */
  entry_props[PROP_TEXT_LENGTH] =
      g_param_spec_uint ("text-length",
                         P_("Text length"),
                         P_("Length of the text currently in the entry"),
                         0, G_MAXUINT16,
                         0,
                         CTK_PARAM_READABLE);

  /**
   * CtkEntry:invisible-char-set:
   *
   * Whether the invisible char has been set for the #CtkEntry.
   *
   * Since: 2.16
   */
  entry_props[PROP_INVISIBLE_CHAR_SET] =
      g_param_spec_boolean ("invisible-char-set",
                            P_("Invisible character set"),
                            P_("Whether the invisible character has been set"),
                            FALSE,
                            CTK_PARAM_READWRITE);

  /**
   * CtkEntry:caps-lock-warning:
   *
   * Whether password entries will show a warning when Caps Lock is on.
   *
   * Note that the warning is shown using a secondary icon, and thus
   * does not work if you are using the secondary icon position for some
   * other purpose.
   *
   * Since: 2.16
   */
  entry_props[PROP_CAPS_LOCK_WARNING] =
      g_param_spec_boolean ("caps-lock-warning",
                            P_("Caps Lock warning"),
                            P_("Whether password entries will show a warning when Caps Lock is on"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:progress-fraction:
   *
   * The current fraction of the task that's been completed.
   *
   * Since: 2.16
   */
  entry_props[PROP_PROGRESS_FRACTION] =
      g_param_spec_double ("progress-fraction",
                           P_("Progress Fraction"),
                           P_("The current fraction of the task that's been completed"),
                           0.0, 1.0,
                           0.0,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:progress-pulse-step:
   *
   * The fraction of total entry width to move the progress
   * bouncing block for each call to ctk_entry_progress_pulse().
   *
   * Since: 2.16
   */
  entry_props[PROP_PROGRESS_PULSE_STEP] =
      g_param_spec_double ("progress-pulse-step",
                           P_("Progress Pulse Step"),
                           P_("The fraction of total entry width to move the progress bouncing block for each call to ctk_entry_progress_pulse()"),
                           0.0, 1.0,
                           0.1,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
  * CtkEntry:placeholder-text:
  *
  * The text that will be displayed in the #CtkEntry when it is empty
  * and unfocused.
  *
  * Since: 3.2
  */
  entry_props[PROP_PLACEHOLDER_TEXT] =
      g_param_spec_string ("placeholder-text",
                           P_("Placeholder text"),
                           P_("Show text in the entry when it's empty and unfocused"),
                           NULL,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

   /**
   * CtkEntry:primary-icon-pixbuf:
   *
   * A pixbuf to use as the primary icon for the entry.
   *
   * Since: 2.16
   */
  entry_props[PROP_PIXBUF_PRIMARY] =
      g_param_spec_object ("primary-icon-pixbuf",
                           P_("Primary pixbuf"),
                           P_("Primary pixbuf for the entry"),
                           GDK_TYPE_PIXBUF,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:secondary-icon-pixbuf:
   *
   * An pixbuf to use as the secondary icon for the entry.
   *
   * Since: 2.16
   */
  entry_props[PROP_PIXBUF_SECONDARY] =
      g_param_spec_object ("secondary-icon-pixbuf",
                           P_("Secondary pixbuf"),
                           P_("Secondary pixbuf for the entry"),
                           GDK_TYPE_PIXBUF,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:primary-icon-stock:
   *
   * The stock id to use for the primary icon for the entry.
   *
   * Since: 2.16
   *
   * Deprecated: 3.10: Use #CtkEntry:primary-icon-name instead.
   */
  entry_props[PROP_STOCK_PRIMARY] =
    g_param_spec_string ("primary-icon-stock",
                         P_("Primary stock ID"),
                         P_("Stock ID for primary icon"),
                         NULL,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  /**
   * CtkEntry:secondary-icon-stock:
   *
   * The stock id to use for the secondary icon for the entry.
   *
   * Since: 2.16
   *
   * Deprecated: 3.10: Use #CtkEntry:secondary-icon-name instead.
   */
  entry_props[PROP_STOCK_SECONDARY] =
      g_param_spec_string ("secondary-icon-stock",
                           P_("Secondary stock ID"),
                           P_("Stock ID for secondary icon"),
                           NULL,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  /**
   * CtkEntry:primary-icon-name:
   *
   * The icon name to use for the primary icon for the entry.
   *
   * Since: 2.16
   */
  entry_props[PROP_ICON_NAME_PRIMARY] =
      g_param_spec_string ("primary-icon-name",
                           P_("Primary icon name"),
                           P_("Icon name for primary icon"),
                           NULL,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:secondary-icon-name:
   *
   * The icon name to use for the secondary icon for the entry.
   *
   * Since: 2.16
   */
  entry_props[PROP_ICON_NAME_SECONDARY] =
      g_param_spec_string ("secondary-icon-name",
                           P_("Secondary icon name"),
                           P_("Icon name for secondary icon"),
                           NULL,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:primary-icon-gicon:
   *
   * The #GIcon to use for the primary icon for the entry.
   *
   * Since: 2.16
   */
  entry_props[PROP_GICON_PRIMARY] =
      g_param_spec_object ("primary-icon-gicon",
                           P_("Primary GIcon"),
                           P_("GIcon for primary icon"),
                           G_TYPE_ICON,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:secondary-icon-gicon:
   *
   * The #GIcon to use for the secondary icon for the entry.
   *
   * Since: 2.16
   */
  entry_props[PROP_GICON_SECONDARY] =
      g_param_spec_object ("secondary-icon-gicon",
                           P_("Secondary GIcon"),
                           P_("GIcon for secondary icon"),
                           G_TYPE_ICON,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:primary-icon-storage-type:
   *
   * The representation which is used for the primary icon of the entry.
   *
   * Since: 2.16
   */
  entry_props[PROP_STORAGE_TYPE_PRIMARY] =
      g_param_spec_enum ("primary-icon-storage-type",
                         P_("Primary storage type"),
                         P_("The representation being used for primary icon"),
                         CTK_TYPE_IMAGE_TYPE,
                         CTK_IMAGE_EMPTY,
                         CTK_PARAM_READABLE);

  /**
   * CtkEntry:secondary-icon-storage-type:
   *
   * The representation which is used for the secondary icon of the entry.
   *
   * Since: 2.16
   */
  entry_props[PROP_STORAGE_TYPE_SECONDARY] =
      g_param_spec_enum ("secondary-icon-storage-type",
                         P_("Secondary storage type"),
                         P_("The representation being used for secondary icon"),
                         CTK_TYPE_IMAGE_TYPE,
                         CTK_IMAGE_EMPTY,
                         CTK_PARAM_READABLE);

  /**
   * CtkEntry:primary-icon-activatable:
   *
   * Whether the primary icon is activatable.
   *
   * CTK+ emits the #CtkEntry::icon-press and #CtkEntry::icon-release
   * signals only on sensitive, activatable icons.
   *
   * Sensitive, but non-activatable icons can be used for purely
   * informational purposes.
   *
   * Since: 2.16
   */
  entry_props[PROP_ACTIVATABLE_PRIMARY] =
      g_param_spec_boolean ("primary-icon-activatable",
                            P_("Primary icon activatable"),
                            P_("Whether the primary icon is activatable"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:secondary-icon-activatable:
   *
   * Whether the secondary icon is activatable.
   *
   * CTK+ emits the #CtkEntry::icon-press and #CtkEntry::icon-release
   * signals only on sensitive, activatable icons.
   *
   * Sensitive, but non-activatable icons can be used for purely
   * informational purposes.
   *
   * Since: 2.16
   */
  entry_props[PROP_ACTIVATABLE_SECONDARY] =
      g_param_spec_boolean ("secondary-icon-activatable",
                            P_("Secondary icon activatable"),
                            P_("Whether the secondary icon is activatable"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:primary-icon-sensitive:
   *
   * Whether the primary icon is sensitive.
   *
   * An insensitive icon appears grayed out. CTK+ does not emit the
   * #CtkEntry::icon-press and #CtkEntry::icon-release signals and
   * does not allow DND from insensitive icons.
   *
   * An icon should be set insensitive if the action that would trigger
   * when clicked is currently not available.
   *
   * Since: 2.16
   */
  entry_props[PROP_SENSITIVE_PRIMARY] =
      g_param_spec_boolean ("primary-icon-sensitive",
                            P_("Primary icon sensitive"),
                            P_("Whether the primary icon is sensitive"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:secondary-icon-sensitive:
   *
   * Whether the secondary icon is sensitive.
   *
   * An insensitive icon appears grayed out. CTK+ does not emit the
   * #CtkEntry::icon-press and #CtkEntry::icon-release signals and
   * does not allow DND from insensitive icons.
   *
   * An icon should be set insensitive if the action that would trigger
   * when clicked is currently not available.
   *
   * Since: 2.16
   */
  entry_props[PROP_SENSITIVE_SECONDARY] =
      g_param_spec_boolean ("secondary-icon-sensitive",
                            P_("Secondary icon sensitive"),
                            P_("Whether the secondary icon is sensitive"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:primary-icon-tooltip-text:
   *
   * The contents of the tooltip on the primary icon.
   *
   * Also see ctk_entry_set_icon_tooltip_text().
   *
   * Since: 2.16
   */
  entry_props[PROP_TOOLTIP_TEXT_PRIMARY] =
      g_param_spec_string ("primary-icon-tooltip-text",
                           P_("Primary icon tooltip text"),
                           P_("The contents of the tooltip on the primary icon"),
                           NULL,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:secondary-icon-tooltip-text:
   *
   * The contents of the tooltip on the secondary icon.
   *
   * Also see ctk_entry_set_icon_tooltip_text().
   *
   * Since: 2.16
   */
  entry_props[PROP_TOOLTIP_TEXT_SECONDARY] =
      g_param_spec_string ("secondary-icon-tooltip-text",
                           P_("Secondary icon tooltip text"),
                           P_("The contents of the tooltip on the secondary icon"),
                           NULL,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:primary-icon-tooltip-markup:
   *
   * The contents of the tooltip on the primary icon, which is marked up
   * with the [Pango text markup language][PangoMarkupFormat].
   *
   * Also see ctk_entry_set_icon_tooltip_markup().
   *
   * Since: 2.16
   */
  entry_props[PROP_TOOLTIP_MARKUP_PRIMARY] =
      g_param_spec_string ("primary-icon-tooltip-markup",
                           P_("Primary icon tooltip markup"),
                           P_("The contents of the tooltip on the primary icon"),
                           NULL,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:secondary-icon-tooltip-markup:
   *
   * The contents of the tooltip on the secondary icon, which is marked up
   * with the [Pango text markup language][PangoMarkupFormat].
   *
   * Also see ctk_entry_set_icon_tooltip_markup().
   *
   * Since: 2.16
   */
  entry_props[PROP_TOOLTIP_MARKUP_SECONDARY] =
      g_param_spec_string ("secondary-icon-tooltip-markup",
                           P_("Secondary icon tooltip markup"),
                           P_("The contents of the tooltip on the secondary icon"),
                           NULL,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:im-module:
   *
   * Which IM (input method) module should be used for this entry.
   * See #CtkIMContext.
   *
   * Setting this to a non-%NULL value overrides the
   * system-wide IM module setting. See the CtkSettings
   * #CtkSettings:ctk-im-module property.
   *
   * Since: 2.16
   */
  entry_props[PROP_IM_MODULE] =
      g_param_spec_string ("im-module",
                           P_("IM module"),
                           P_("Which IM module should be used"),
                           NULL,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:completion:
   *
   * The auxiliary completion object to use with the entry.
   *
   * Since: 3.2
   */
  entry_props[PROP_COMPLETION] =
      g_param_spec_object ("completion",
                           P_("Completion"),
                           P_("The auxiliary completion object"),
                           CTK_TYPE_ENTRY_COMPLETION,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:input-purpose:
   *
   * The purpose of this text field.
   *
   * This property can be used by on-screen keyboards and other input
   * methods to adjust their behaviour.
   *
   * Note that setting the purpose to %CTK_INPUT_PURPOSE_PASSWORD or
   * %CTK_INPUT_PURPOSE_PIN is independent from setting
   * #CtkEntry:visibility.
   *
   * Since: 3.6
   */
  entry_props[PROP_INPUT_PURPOSE] =
      g_param_spec_enum ("input-purpose",
                         P_("Purpose"),
                         P_("Purpose of the text field"),
                         CTK_TYPE_INPUT_PURPOSE,
                         CTK_INPUT_PURPOSE_FREE_FORM,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:input-hints:
   *
   * Additional hints (beyond #CtkEntry:input-purpose) that
   * allow input methods to fine-tune their behaviour.
   *
   * Since: 3.6
   */
  entry_props[PROP_INPUT_HINTS] =
      g_param_spec_flags ("input-hints",
                          P_("hints"),
                          P_("Hints for the text field behaviour"),
                          CTK_TYPE_INPUT_HINTS,
                          CTK_INPUT_HINT_NONE,
                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:attributes:
   *
   * A list of Pango attributes to apply to the text of the entry.
   *
   * This is mainly useful to change the size or weight of the text.
   *
   * The #PangoAttribute's @start_index and @end_index must refer to the
   * #CtkEntryBuffer text, i.e. without the preedit string.
   *
   * Since: 3.6
   */
  entry_props[PROP_ATTRIBUTES] =
      g_param_spec_boxed ("attributes",
                          P_("Attributes"),
                          P_("A list of style attributes to apply to the text of the label"),
                          PANGO_TYPE_ATTR_LIST,
                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry:populate-all:
   *
   * If :populate-all is %TRUE, the #CtkEntry::populate-popup
   * signal is also emitted for touch popups.
   *
   * Since: 3.8
   */
  entry_props[PROP_POPULATE_ALL] =
      g_param_spec_boolean ("populate-all",
                            P_("Populate all"),
                            P_("Whether to emit ::populate-popup for touch popups"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry::tabs:
   *
   * A list of tabstops to apply to the text of the entry.
   *
   * Since: 3.8
   */
  entry_props[PROP_TABS] =
      g_param_spec_boxed ("tabs",
                          P_("Tabs"),
                          P_("A list of tabstop locations to apply to the text of the entry"),
                          PANGO_TYPE_TAB_ARRAY,
                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkEntry::show-emoji-icon:
   *
   * When this is %TRUE, the entry will show an emoji icon in the secondary
   * icon position that brings up the Emoji chooser when clicked.
   *
   * Since: 3.22.19
   */
  entry_props[PROP_SHOW_EMOJI_ICON] =
      g_param_spec_boolean ("show-emoji-icon",
                            P_("Emoji icon"),
                            P_("Whether to show an icon for Emoji"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  entry_props[PROP_ENABLE_EMOJI_COMPLETION] =
      g_param_spec_boolean ("enable-emoji-completion",
                            P_("Enable Emoji completion"),
                            P_("Whether to suggest Emoji replacements"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, entry_props);

  /**
   * CtkEntry:icon-prelight:
   *
   * The prelight style property determines whether activatable
   * icons prelight on mouseover.
   *
   * Since: 2.16
   *
   * Deprecated: 3.20: Use CSS to control the appearance of prelighted icons;
   *     the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("icon-prelight",
                                                                 P_("Icon Prelight"),
                                                                 P_("Whether activatable icons should prelight when hovered"),
                                                                 TRUE,
                                                                 CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkEntry:progress-border:
   *
   * The border around the progress bar in the entry.
   *
   * Since: 2.16
   *
   * Deprecated: 3.4: Use the standard margin CSS property (through objects
   *   like #CtkStyleContext and #CtkCssProvider); the value of this style
   *   property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("progress-border",
                                                               P_("Progress Border"),
                                                               P_("Border around the progress bar"),
                                                               CTK_TYPE_BORDER,
                                                               CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkEntry:invisible-char:
   *
   * The invisible character is used when masking entry contents (in
   * \"password mode\")"). When it is not explicitly set with the
   * #CtkEntry:invisible-char property, CTK+ determines the character
   * to use from a list of possible candidates, depending on availability
   * in the current font.
   *
   * This style property allows the theme to prepend a character
   * to the list of candidates.
   *
   * Since: 2.18
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_unichar ("invisible-char",
					    		         P_("Invisible character"),
							         P_("The character to use when masking entry contents (in \"password mode\")"),
							         0,
							         CTK_PARAM_READABLE));
  
  /**
   * CtkEntry::populate-popup:
   * @entry: The entry on which the signal is emitted
   * @widget: the container that is being populated
   *
   * The ::populate-popup signal gets emitted before showing the
   * context menu of the entry.
   *
   * If you need to add items to the context menu, connect
   * to this signal and append your items to the @widget, which
   * will be a #CtkMenu in this case.
   *
   * If #CtkEntry:populate-all is %TRUE, this signal will
   * also be emitted to populate touch popups. In this case,
   * @widget will be a different container, e.g. a #CtkToolbar.
   * The signal handler should not make assumptions about the
   * type of @widget.
   */
  signals[POPULATE_POPUP] =
    g_signal_new (I_("populate-popup"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkEntryClass, populate_popup),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_WIDGET);
  
 /* Action signals */
  
  /**
   * CtkEntry::activate:
   * @entry: The entry on which the signal is emitted
   *
   * The ::activate signal is emitted when the user hits
   * the Enter key.
   *
   * While this signal is used as a
   * [keybinding signal][CtkBindingSignal],
   * it is also commonly used by applications to intercept
   * activation of entries.
   *
   * The default bindings for this signal are all forms of the Enter key.
   */
  signals[ACTIVATE] =
    g_signal_new (I_("activate"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkEntryClass, activate),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
  widget_class->activate_signal = signals[ACTIVATE];

  /**
   * CtkEntry::move-cursor:
   * @entry: the object which received the signal
   * @step: the granularity of the move, as a #CtkMovementStep
   * @count: the number of @step units to move
   * @extend_selection: %TRUE if the move should extend the selection
   *
   * The ::move-cursor signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user initiates a cursor movement.
   * If the cursor is not visible in @entry, this signal causes
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
   */
  signals[MOVE_CURSOR] = 
    g_signal_new (I_("move-cursor"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkEntryClass, move_cursor),
		  NULL, NULL,
		  _ctk_marshal_VOID__ENUM_INT_BOOLEAN,
		  G_TYPE_NONE, 3,
		  CTK_TYPE_MOVEMENT_STEP,
		  G_TYPE_INT,
		  G_TYPE_BOOLEAN);

  /**
   * CtkEntry::insert-at-cursor:
   * @entry: the object which received the signal
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
		  G_STRUCT_OFFSET (CtkEntryClass, insert_at_cursor),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  G_TYPE_STRING);

  /**
   * CtkEntry::delete-from-cursor:
   * @entry: the object which received the signal
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
   * Delete for deleting a character and Ctrl-Delete for
   * deleting a word.
   */
  signals[DELETE_FROM_CURSOR] = 
    g_signal_new (I_("delete-from-cursor"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkEntryClass, delete_from_cursor),
		  NULL, NULL,
		  _ctk_marshal_VOID__ENUM_INT,
		  G_TYPE_NONE, 2,
		  CTK_TYPE_DELETE_TYPE,
		  G_TYPE_INT);

  /**
   * CtkEntry::backspace:
   * @entry: the object which received the signal
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
		  G_STRUCT_OFFSET (CtkEntryClass, backspace),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkEntry::cut-clipboard:
   * @entry: the object which received the signal
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
		  G_STRUCT_OFFSET (CtkEntryClass, cut_clipboard),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkEntry::copy-clipboard:
   * @entry: the object which received the signal
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
		  G_STRUCT_OFFSET (CtkEntryClass, copy_clipboard),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkEntry::paste-clipboard:
   * @entry: the object which received the signal
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
		  G_STRUCT_OFFSET (CtkEntryClass, paste_clipboard),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkEntry::toggle-overwrite:
   * @entry: the object which received the signal
   *
   * The ::toggle-overwrite signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to toggle the overwrite mode of the entry.
   *
   * The default bindings for this signal is Insert.
   */
  signals[TOGGLE_OVERWRITE] =
    g_signal_new (I_("toggle-overwrite"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkEntryClass, toggle_overwrite),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkEntry::icon-press:
   * @entry: The entry on which the signal is emitted
   * @icon_pos: The position of the clicked icon
   * @event: the button press event
   *
   * The ::icon-press signal is emitted when an activatable icon
   * is clicked.
   *
   * Since: 2.16
   */
  signals[ICON_PRESS] =
    g_signal_new (I_("icon-press"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  _ctk_marshal_VOID__ENUM_BOXED,
                  G_TYPE_NONE, 2,
                  CTK_TYPE_ENTRY_ICON_POSITION,
                  GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  
  /**
   * CtkEntry::icon-release:
   * @entry: The entry on which the signal is emitted
   * @icon_pos: The position of the clicked icon
   * @event: the button release event
   *
   * The ::icon-release signal is emitted on the button release from a
   * mouse click over an activatable icon.
   *
   * Since: 2.16
   */
  signals[ICON_RELEASE] =
    g_signal_new (I_("icon-release"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  _ctk_marshal_VOID__ENUM_BOXED,
                  G_TYPE_NONE, 2,
                  CTK_TYPE_ENTRY_ICON_POSITION,
                  GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * CtkEntry::preedit-changed:
   * @entry: the object which received the signal
   * @preedit: the current preedit string
   *
   * If an input method is used, the typed text will not immediately
   * be committed to the buffer. So if you are interested in the text,
   * connect to this signal.
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
   * CtkEntry::insert-emoji:
   * @entry: the object which received the signal
   *
   * The ::insert-emoji signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to present the Emoji chooser for the @entry.
   *
   * The default bindings for this signal are Ctrl-. and Ctrl-;
   *
   * Since: 3.22.27
   */
  signals[INSERT_EMOJI] =
    g_signal_new (I_("insert-emoji"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkEntryClass, insert_emoji),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /*
   * Key bindings
   */

  binding_set = ctk_binding_set_by_class (class);

  /* Moving the insertion point */
  add_move_binding (binding_set, GDK_KEY_Right, 0,
		    CTK_MOVEMENT_VISUAL_POSITIONS, 1);
  
  add_move_binding (binding_set, GDK_KEY_Left, 0,
		    CTK_MOVEMENT_VISUAL_POSITIONS, -1);

  add_move_binding (binding_set, GDK_KEY_KP_Right, 0,
		    CTK_MOVEMENT_VISUAL_POSITIONS, 1);
  
  add_move_binding (binding_set, GDK_KEY_KP_Left, 0,
		    CTK_MOVEMENT_VISUAL_POSITIONS, -1);
  
  add_move_binding (binding_set, GDK_KEY_Right, GDK_CONTROL_MASK,
		    CTK_MOVEMENT_WORDS, 1);

  add_move_binding (binding_set, GDK_KEY_Left, GDK_CONTROL_MASK,
		    CTK_MOVEMENT_WORDS, -1);

  add_move_binding (binding_set, GDK_KEY_KP_Right, GDK_CONTROL_MASK,
		    CTK_MOVEMENT_WORDS, 1);

  add_move_binding (binding_set, GDK_KEY_KP_Left, GDK_CONTROL_MASK,
		    CTK_MOVEMENT_WORDS, -1);
  
  add_move_binding (binding_set, GDK_KEY_Home, 0,
		    CTK_MOVEMENT_DISPLAY_LINE_ENDS, -1);

  add_move_binding (binding_set, GDK_KEY_End, 0,
		    CTK_MOVEMENT_DISPLAY_LINE_ENDS, 1);

  add_move_binding (binding_set, GDK_KEY_KP_Home, 0,
		    CTK_MOVEMENT_DISPLAY_LINE_ENDS, -1);

  add_move_binding (binding_set, GDK_KEY_KP_End, 0,
		    CTK_MOVEMENT_DISPLAY_LINE_ENDS, 1);
  
  add_move_binding (binding_set, GDK_KEY_Home, GDK_CONTROL_MASK,
		    CTK_MOVEMENT_BUFFER_ENDS, -1);

  add_move_binding (binding_set, GDK_KEY_End, GDK_CONTROL_MASK,
		    CTK_MOVEMENT_BUFFER_ENDS, 1);

  add_move_binding (binding_set, GDK_KEY_KP_Home, GDK_CONTROL_MASK,
		    CTK_MOVEMENT_BUFFER_ENDS, -1);

  add_move_binding (binding_set, GDK_KEY_KP_End, GDK_CONTROL_MASK,
		    CTK_MOVEMENT_BUFFER_ENDS, 1);

  /* Select all
   */
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_a, GDK_CONTROL_MASK,
                                "move-cursor", 3,
                                CTK_TYPE_MOVEMENT_STEP, CTK_MOVEMENT_BUFFER_ENDS,
                                G_TYPE_INT, -1,
				G_TYPE_BOOLEAN, FALSE);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_a, GDK_CONTROL_MASK,
                                "move-cursor", 3,
                                CTK_TYPE_MOVEMENT_STEP, CTK_MOVEMENT_BUFFER_ENDS,
                                G_TYPE_INT, 1,
				G_TYPE_BOOLEAN, TRUE);  

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_slash, GDK_CONTROL_MASK,
                                "move-cursor", 3,
                                CTK_TYPE_MOVEMENT_STEP, CTK_MOVEMENT_BUFFER_ENDS,
                                G_TYPE_INT, -1,
				G_TYPE_BOOLEAN, FALSE);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_slash, GDK_CONTROL_MASK,
                                "move-cursor", 3,
                                CTK_TYPE_MOVEMENT_STEP, CTK_MOVEMENT_BUFFER_ENDS,
                                G_TYPE_INT, 1,
				G_TYPE_BOOLEAN, TRUE);  
  /* Unselect all 
   */
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_backslash, GDK_CONTROL_MASK,
                                "move-cursor", 3,
                                CTK_TYPE_MOVEMENT_STEP, CTK_MOVEMENT_VISUAL_POSITIONS,
                                G_TYPE_INT, 0,
				G_TYPE_BOOLEAN, FALSE);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_a, GDK_SHIFT_MASK | GDK_CONTROL_MASK,
                                "move-cursor", 3,
                                CTK_TYPE_MOVEMENT_STEP, CTK_MOVEMENT_VISUAL_POSITIONS,
                                G_TYPE_INT, 0,
				G_TYPE_BOOLEAN, FALSE);

  /* Activate
   */
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_Return, 0,
				"activate", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_ISO_Enter, 0,
				"activate", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Enter, 0,
				"activate", 0);
  
  /* Deleting text */
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_Delete, 0,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, CTK_DELETE_CHARS,
				G_TYPE_INT, 1);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Delete, 0,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, CTK_DELETE_CHARS,
				G_TYPE_INT, 1);
  
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_BackSpace, 0,
				"backspace", 0);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_u, GDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, CTK_DELETE_PARAGRAPH_ENDS,
				G_TYPE_INT, -1);

  /* Make this do the same as Backspace, to help with mis-typing */
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_BackSpace, GDK_SHIFT_MASK,
				"backspace", 0);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_Delete, GDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, CTK_DELETE_WORD_ENDS,
				G_TYPE_INT, 1);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Delete, GDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, CTK_DELETE_WORD_ENDS,
				G_TYPE_INT, 1);
  
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_BackSpace, GDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, CTK_DELETE_WORD_ENDS,
				G_TYPE_INT, -1);

  /* Cut/copy/paste */

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_x, GDK_CONTROL_MASK,
				"cut-clipboard", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_c, GDK_CONTROL_MASK,
				"copy-clipboard", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_v, GDK_CONTROL_MASK,
				"paste-clipboard", 0);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_Delete, GDK_SHIFT_MASK,
				"cut-clipboard", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_Insert, GDK_CONTROL_MASK,
				"copy-clipboard", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_Insert, GDK_SHIFT_MASK,
				"paste-clipboard", 0);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Delete, GDK_SHIFT_MASK,
                                "cut-clipboard", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Insert, GDK_CONTROL_MASK,
                                "copy-clipboard", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Insert, GDK_SHIFT_MASK,
                                "paste-clipboard", 0);

  /* Overwrite */
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_Insert, 0,
				"toggle-overwrite", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Insert, 0,
				"toggle-overwrite", 0);

  /**
   * CtkEntry:inner-border:
   *
   * Sets the text area's border between the text and the frame.
   *
   * Since: 2.10
   *
   * Deprecated: 3.4: Use the standard border and padding CSS properties
   *   (through objects like #CtkStyleContext and #CtkCssProvider); the value
   *   of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("inner-border",
                                                               P_("Inner Border"),
                                                               P_("Border between text and frame."),
                                                               CTK_TYPE_BORDER,
                                                               CTK_PARAM_READABLE |
                                                               G_PARAM_DEPRECATED));
  /* Emoji */
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_period, GDK_CONTROL_MASK,
                                "insert-emoji", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_semicolon, GDK_CONTROL_MASK,
                                "insert-emoji", 0);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_ENTRY_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "entry");
}

static void
ctk_entry_editable_init (CtkEditableInterface *iface)
{
  iface->do_insert_text = ctk_entry_insert_text;
  iface->do_delete_text = ctk_entry_delete_text;
  iface->insert_text = ctk_entry_real_insert_text;
  iface->delete_text = ctk_entry_real_delete_text;
  iface->get_chars = ctk_entry_get_chars;
  iface->set_selection_bounds = ctk_entry_set_selection_bounds;
  iface->get_selection_bounds = ctk_entry_get_selection_bounds;
  iface->set_position = ctk_entry_real_set_position;
  iface->get_position = ctk_entry_get_position;
}

static void
ctk_entry_cell_editable_init (CtkCellEditableIface *iface)
{
  iface->start_editing = ctk_entry_start_editing;
}

/* for deprecated properties */
static void
ctk_entry_do_set_inner_border (CtkEntry *entry,
                               const CtkBorder *border)
{
  if (border)
    g_object_set_qdata_full (G_OBJECT (entry), quark_inner_border,
                             ctk_border_copy (border),
                             (GDestroyNotify) ctk_border_free);
  else
    g_object_set_qdata (G_OBJECT (entry), quark_inner_border, NULL);

  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_INNER_BORDER]);
}

static const CtkBorder *
ctk_entry_do_get_inner_border (CtkEntry *entry)
{
  return g_object_get_qdata (G_OBJECT (entry), quark_inner_border);
}

static void
ctk_entry_set_property (GObject         *object,
                        guint            prop_id,
                        const GValue    *value,
                        GParamSpec      *pspec)
{
  CtkEntry *entry = CTK_ENTRY (object);
  CtkEntryPrivate *priv = entry->priv;

  switch (prop_id)
    {
    case PROP_BUFFER:
      ctk_entry_set_buffer (entry, g_value_get_object (value));
      break;

    case PROP_EDITABLE:
      {
        gboolean new_value = g_value_get_boolean (value);
        CtkStyleContext *context = ctk_widget_get_style_context (CTK_WIDGET (entry));

        if (new_value != priv->editable)
	  {
            CtkWidget *widget = CTK_WIDGET (entry);

	    if (!new_value)
	      {
		ctk_entry_reset_im_context (entry);
		if (ctk_widget_has_focus (widget))
		  ctk_im_context_focus_out (priv->im_context);

		priv->preedit_length = 0;
		priv->preedit_cursor = 0;

                ctk_style_context_remove_class (context, CTK_STYLE_CLASS_READ_ONLY);
	      }
            else
              {
                ctk_style_context_add_class (context, CTK_STYLE_CLASS_READ_ONLY);
              }

	    priv->editable = new_value;

	    if (new_value && ctk_widget_has_focus (widget))
	      ctk_im_context_focus_in (priv->im_context);

            g_object_notify_by_pspec (object, pspec);
	    ctk_widget_queue_draw (widget);
	  }
      }
      break;

    case PROP_MAX_LENGTH:
      ctk_entry_set_max_length (entry, g_value_get_int (value));
      break;

    case PROP_VISIBILITY:
      ctk_entry_set_visibility (entry, g_value_get_boolean (value));
      break;

    case PROP_HAS_FRAME:
      ctk_entry_set_has_frame (entry, g_value_get_boolean (value));
      break;

    case PROP_INNER_BORDER:
      ctk_entry_do_set_inner_border (entry, g_value_get_boxed (value));
      break;

    case PROP_INVISIBLE_CHAR:
      ctk_entry_set_invisible_char (entry, g_value_get_uint (value));
      break;

    case PROP_ACTIVATES_DEFAULT:
      ctk_entry_set_activates_default (entry, g_value_get_boolean (value));
      break;

    case PROP_WIDTH_CHARS:
      ctk_entry_set_width_chars (entry, g_value_get_int (value));
      break;

    case PROP_MAX_WIDTH_CHARS:
      ctk_entry_set_max_width_chars (entry, g_value_get_int (value));
      break;

    case PROP_TEXT:
      ctk_entry_set_text (entry, g_value_get_string (value));
      break;

    case PROP_XALIGN:
      ctk_entry_set_alignment (entry, g_value_get_float (value));
      break;

    case PROP_TRUNCATE_MULTILINE:
      if (priv->truncate_multiline != g_value_get_boolean (value))
        {
          priv->truncate_multiline = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_SHADOW_TYPE:
      if (priv->shadow_type != g_value_get_enum (value))
        {
          priv->shadow_type = g_value_get_enum (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_OVERWRITE_MODE:
      ctk_entry_set_overwrite_mode (entry, g_value_get_boolean (value));
      break;

    case PROP_INVISIBLE_CHAR_SET:
      if (g_value_get_boolean (value))
        priv->invisible_char_set = TRUE;
      else
        ctk_entry_unset_invisible_char (entry);
      break;

    case PROP_CAPS_LOCK_WARNING:
      if (priv->caps_lock_warning != g_value_get_boolean (value))
        {
          priv->caps_lock_warning = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_PROGRESS_FRACTION:
      ctk_entry_set_progress_fraction (entry, g_value_get_double (value));
      break;

    case PROP_PROGRESS_PULSE_STEP:
      ctk_entry_set_progress_pulse_step (entry, g_value_get_double (value));
      break;

    case PROP_PLACEHOLDER_TEXT:
      ctk_entry_set_placeholder_text (entry, g_value_get_string (value));
      break;

    case PROP_PIXBUF_PRIMARY:
      ctk_entry_set_icon_from_pixbuf (entry,
                                      CTK_ENTRY_ICON_PRIMARY,
                                      g_value_get_object (value));
      break;

    case PROP_PIXBUF_SECONDARY:
      ctk_entry_set_icon_from_pixbuf (entry,
                                      CTK_ENTRY_ICON_SECONDARY,
                                      g_value_get_object (value));
      break;

    case PROP_STOCK_PRIMARY:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_entry_set_icon_from_stock (entry,
                                     CTK_ENTRY_ICON_PRIMARY,
                                     g_value_get_string (value));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;

    case PROP_STOCK_SECONDARY:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_entry_set_icon_from_stock (entry,
                                     CTK_ENTRY_ICON_SECONDARY,
                                     g_value_get_string (value));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;

    case PROP_ICON_NAME_PRIMARY:
      ctk_entry_set_icon_from_icon_name (entry,
                                         CTK_ENTRY_ICON_PRIMARY,
                                         g_value_get_string (value));
      break;

    case PROP_ICON_NAME_SECONDARY:
      ctk_entry_set_icon_from_icon_name (entry,
                                         CTK_ENTRY_ICON_SECONDARY,
                                         g_value_get_string (value));
      break;

    case PROP_GICON_PRIMARY:
      ctk_entry_set_icon_from_gicon (entry,
                                     CTK_ENTRY_ICON_PRIMARY,
                                     g_value_get_object (value));
      break;

    case PROP_GICON_SECONDARY:
      ctk_entry_set_icon_from_gicon (entry,
                                     CTK_ENTRY_ICON_SECONDARY,
                                     g_value_get_object (value));
      break;

    case PROP_ACTIVATABLE_PRIMARY:
      ctk_entry_set_icon_activatable (entry,
                                      CTK_ENTRY_ICON_PRIMARY,
                                      g_value_get_boolean (value));
      break;

    case PROP_ACTIVATABLE_SECONDARY:
      ctk_entry_set_icon_activatable (entry,
                                      CTK_ENTRY_ICON_SECONDARY,
                                      g_value_get_boolean (value));
      break;

    case PROP_SENSITIVE_PRIMARY:
      ctk_entry_set_icon_sensitive (entry,
                                    CTK_ENTRY_ICON_PRIMARY,
                                    g_value_get_boolean (value));
      break;

    case PROP_SENSITIVE_SECONDARY:
      ctk_entry_set_icon_sensitive (entry,
                                    CTK_ENTRY_ICON_SECONDARY,
                                    g_value_get_boolean (value));
      break;

    case PROP_TOOLTIP_TEXT_PRIMARY:
      ctk_entry_set_icon_tooltip_text (entry,
                                       CTK_ENTRY_ICON_PRIMARY,
                                       g_value_get_string (value));
      break;

    case PROP_TOOLTIP_TEXT_SECONDARY:
      ctk_entry_set_icon_tooltip_text (entry,
                                       CTK_ENTRY_ICON_SECONDARY,
                                       g_value_get_string (value));
      break;

    case PROP_TOOLTIP_MARKUP_PRIMARY:
      ctk_entry_set_icon_tooltip_markup (entry,
                                         CTK_ENTRY_ICON_PRIMARY,
                                         g_value_get_string (value));
      break;

    case PROP_TOOLTIP_MARKUP_SECONDARY:
      ctk_entry_set_icon_tooltip_markup (entry,
                                         CTK_ENTRY_ICON_SECONDARY,
                                         g_value_get_string (value));
      break;

    case PROP_IM_MODULE:
      g_free (priv->im_module);
      priv->im_module = g_value_dup_string (value);
      if (CTK_IS_IM_MULTICONTEXT (priv->im_context))
        ctk_im_multicontext_set_context_id (CTK_IM_MULTICONTEXT (priv->im_context), priv->im_module);
      g_object_notify_by_pspec (object, pspec);
      break;

    case PROP_EDITING_CANCELED:
      if (priv->editing_canceled != g_value_get_boolean (value))
        {
          priv->editing_canceled = g_value_get_boolean (value);
          g_object_notify (object, "editing-canceled");
        }
      break;

    case PROP_COMPLETION:
      ctk_entry_set_completion (entry, CTK_ENTRY_COMPLETION (g_value_get_object (value)));
      break;

    case PROP_INPUT_PURPOSE:
      ctk_entry_set_input_purpose (entry, g_value_get_enum (value));
      break;

    case PROP_INPUT_HINTS:
      ctk_entry_set_input_hints (entry, g_value_get_flags (value));
      break;

    case PROP_ATTRIBUTES:
      ctk_entry_set_attributes (entry, g_value_get_boxed (value));
      break;

    case PROP_POPULATE_ALL:
      if (entry->priv->populate_all != g_value_get_boolean (value))
        {
          entry->priv->populate_all = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_TABS:
      ctk_entry_set_tabs (entry, g_value_get_boxed (value));
      break;

    case PROP_SHOW_EMOJI_ICON:
      set_show_emoji_icon (entry, g_value_get_boolean (value));
      break;

    case PROP_ENABLE_EMOJI_COMPLETION:
      set_enable_emoji_completion (entry, g_value_get_boolean (value));
      break;

    case PROP_SCROLL_OFFSET:
    case PROP_CURSOR_POSITION:
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_entry_get_property (GObject         *object,
                        guint            prop_id,
                        GValue          *value,
                        GParamSpec      *pspec)
{
  CtkEntry *entry = CTK_ENTRY (object);
  CtkEntryPrivate *priv = entry->priv;

  switch (prop_id)
    {
    case PROP_BUFFER:
      g_value_set_object (value, ctk_entry_get_buffer (entry));
      break;

    case PROP_CURSOR_POSITION:
      g_value_set_int (value, priv->current_pos);
      break;

    case PROP_SELECTION_BOUND:
      g_value_set_int (value, priv->selection_bound);
      break;

    case PROP_EDITABLE:
      g_value_set_boolean (value, priv->editable);
      break;

    case PROP_MAX_LENGTH:
      g_value_set_int (value, ctk_entry_buffer_get_max_length (get_buffer (entry)));
      break;

    case PROP_VISIBILITY:
      g_value_set_boolean (value, priv->visible);
      break;

    case PROP_HAS_FRAME:
      g_value_set_boolean (value, ctk_entry_get_has_frame (entry));
      break;

    case PROP_INNER_BORDER:
      g_value_set_boxed (value, ctk_entry_do_get_inner_border (entry));
      break;

    case PROP_INVISIBLE_CHAR:
      g_value_set_uint (value, priv->invisible_char);
      break;

    case PROP_ACTIVATES_DEFAULT:
      g_value_set_boolean (value, priv->activates_default);
      break;

    case PROP_WIDTH_CHARS:
      g_value_set_int (value, priv->width_chars);
      break;

    case PROP_MAX_WIDTH_CHARS:
      g_value_set_int (value, priv->max_width_chars);
      break;

    case PROP_SCROLL_OFFSET:
      g_value_set_int (value, priv->scroll_offset);
      break;

    case PROP_TEXT:
      g_value_set_string (value, ctk_entry_get_text (entry));
      break;

    case PROP_XALIGN:
      g_value_set_float (value, ctk_entry_get_alignment (entry));
      break;

    case PROP_TRUNCATE_MULTILINE:
      g_value_set_boolean (value, priv->truncate_multiline);
      break;

    case PROP_SHADOW_TYPE:
      g_value_set_enum (value, priv->shadow_type);
      break;

    case PROP_OVERWRITE_MODE:
      g_value_set_boolean (value, priv->overwrite_mode);
      break;

    case PROP_TEXT_LENGTH:
      g_value_set_uint (value, ctk_entry_buffer_get_length (get_buffer (entry)));
      break;

    case PROP_INVISIBLE_CHAR_SET:
      g_value_set_boolean (value, priv->invisible_char_set);
      break;

    case PROP_IM_MODULE:
      g_value_set_string (value, priv->im_module);
      break;

    case PROP_CAPS_LOCK_WARNING:
      g_value_set_boolean (value, priv->caps_lock_warning);
      break;

    case PROP_PROGRESS_FRACTION:
      g_value_set_double (value, priv->progress_fraction);
      break;

    case PROP_PROGRESS_PULSE_STEP:
      g_value_set_double (value, priv->progress_pulse_fraction);
      break;

    case PROP_PLACEHOLDER_TEXT:
      g_value_set_string (value, ctk_entry_get_placeholder_text (entry));
      break;

    case PROP_PIXBUF_PRIMARY:
      g_value_set_object (value,
                          ctk_entry_get_icon_pixbuf (entry,
                                                     CTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_PIXBUF_SECONDARY:
      g_value_set_object (value,
                          ctk_entry_get_icon_pixbuf (entry,
                                                     CTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_STOCK_PRIMARY:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      g_value_set_string (value,
                          ctk_entry_get_icon_stock (entry,
                                                    CTK_ENTRY_ICON_PRIMARY));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;

    case PROP_STOCK_SECONDARY:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      g_value_set_string (value,
                          ctk_entry_get_icon_stock (entry,
                                                    CTK_ENTRY_ICON_SECONDARY));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;

    case PROP_ICON_NAME_PRIMARY:
      g_value_set_string (value,
                          ctk_entry_get_icon_name (entry,
                                                   CTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_ICON_NAME_SECONDARY:
      g_value_set_string (value,
                          ctk_entry_get_icon_name (entry,
                                                   CTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_GICON_PRIMARY:
      g_value_set_object (value,
                          ctk_entry_get_icon_gicon (entry,
                                                    CTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_GICON_SECONDARY:
      g_value_set_object (value,
                          ctk_entry_get_icon_gicon (entry,
                                                    CTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_STORAGE_TYPE_PRIMARY:
      g_value_set_enum (value,
                        ctk_entry_get_icon_storage_type (entry, 
                                                         CTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_STORAGE_TYPE_SECONDARY:
      g_value_set_enum (value,
                        ctk_entry_get_icon_storage_type (entry, 
                                                         CTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_ACTIVATABLE_PRIMARY:
      g_value_set_boolean (value,
                           ctk_entry_get_icon_activatable (entry, CTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_ACTIVATABLE_SECONDARY:
      g_value_set_boolean (value,
                           ctk_entry_get_icon_activatable (entry, CTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_SENSITIVE_PRIMARY:
      g_value_set_boolean (value,
                           ctk_entry_get_icon_sensitive (entry, CTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_SENSITIVE_SECONDARY:
      g_value_set_boolean (value,
                           ctk_entry_get_icon_sensitive (entry, CTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_TOOLTIP_TEXT_PRIMARY:
      g_value_take_string (value,
                           ctk_entry_get_icon_tooltip_text (entry, CTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_TOOLTIP_TEXT_SECONDARY:
      g_value_take_string (value,
                           ctk_entry_get_icon_tooltip_text (entry, CTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_TOOLTIP_MARKUP_PRIMARY:
      g_value_take_string (value,
                           ctk_entry_get_icon_tooltip_markup (entry, CTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_TOOLTIP_MARKUP_SECONDARY:
      g_value_take_string (value,
                           ctk_entry_get_icon_tooltip_markup (entry, CTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_EDITING_CANCELED:
      g_value_set_boolean (value,
                           priv->editing_canceled);
      break;

    case PROP_COMPLETION:
      g_value_set_object (value, G_OBJECT (ctk_entry_get_completion (entry)));
      break;

    case PROP_INPUT_PURPOSE:
      g_value_set_enum (value, ctk_entry_get_input_purpose (entry));
      break;

    case PROP_INPUT_HINTS:
      g_value_set_flags (value, ctk_entry_get_input_hints (entry));
      break;

    case PROP_ATTRIBUTES:
      g_value_set_boxed (value, priv->attrs);
      break;

    case PROP_POPULATE_ALL:
      g_value_set_boolean (value, priv->populate_all);
      break;

    case PROP_TABS:
      g_value_set_boxed (value, priv->tabs);
      break;

    case PROP_SHOW_EMOJI_ICON:
      g_value_set_boolean (value, priv->show_emoji_icon);
      break;

    case PROP_ENABLE_EMOJI_COMPLETION:
      g_value_set_boolean (value, priv->enable_emoji_completion);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gunichar
find_invisible_char (CtkWidget *widget)
{
  PangoLayout *layout;
  PangoAttrList *attr_list;
  gint i;
  gunichar invisible_chars [] = {
    0,
    0x25cf, /* BLACK CIRCLE */
    0x2022, /* BULLET */
    0x2731, /* HEAVY ASTERISK */
    0x273a  /* SIXTEEN POINTED ASTERISK */
  };

  ctk_widget_style_get (widget,
                        "invisible-char", &invisible_chars[0],
                        NULL);

  layout = ctk_widget_create_pango_layout (widget, NULL);

  attr_list = pango_attr_list_new ();
  pango_attr_list_insert (attr_list, pango_attr_fallback_new (FALSE));

  pango_layout_set_attributes (layout, attr_list);
  pango_attr_list_unref (attr_list);

  for (i = (invisible_chars[0] != 0 ? 0 : 1); i < G_N_ELEMENTS (invisible_chars); i++)
    {
      gchar text[7] = { 0, };
      gint len, count;

      len = g_unichar_to_utf8 (invisible_chars[i], text);
      pango_layout_set_text (layout, text, len);

      count = pango_layout_get_unknown_glyphs_count (layout);

      if (count == 0)
        {
          g_object_unref (layout);
          return invisible_chars[i];
        }
    }

  g_object_unref (layout);

  return '*';
}

static void
ctk_entry_init (CtkEntry *entry)
{
  CtkEntryPrivate *priv;
  CtkCssNode *widget_node;
  gint i;

  entry->priv = ctk_entry_get_instance_private (entry);
  priv = entry->priv;

  ctk_widget_set_can_focus (CTK_WIDGET (entry), TRUE);
  ctk_widget_set_has_window (CTK_WIDGET (entry), FALSE);

  priv->editable = TRUE;
  priv->visible = TRUE;
  priv->dnd_position = -1;
  priv->width_chars = -1;
  priv->max_width_chars = -1;
  priv->editing_canceled = FALSE;
  priv->truncate_multiline = FALSE;
  priv->shadow_type = CTK_SHADOW_IN;
  priv->xalign = 0.0;
  priv->caps_lock_warning = TRUE;
  priv->caps_lock_warning_shown = FALSE;
  priv->progress_fraction = 0.0;
  priv->progress_pulse_fraction = 0.1;

  ctk_drag_dest_set (CTK_WIDGET (entry), 0, NULL, 0,
                     GDK_ACTION_COPY | GDK_ACTION_MOVE);
  ctk_drag_dest_add_text_targets (CTK_WIDGET (entry));

  /* This object is completely private. No external entity can gain a reference
   * to it; so we create it here and destroy it in finalize().
   */
  priv->im_context = ctk_im_multicontext_new ();

  g_signal_connect (priv->im_context, "commit",
		    G_CALLBACK (ctk_entry_commit_cb), entry);
  g_signal_connect (priv->im_context, "preedit-changed",
		    G_CALLBACK (ctk_entry_preedit_changed_cb), entry);
  g_signal_connect (priv->im_context, "retrieve-surrounding",
		    G_CALLBACK (ctk_entry_retrieve_surrounding_cb), entry);
  g_signal_connect (priv->im_context, "delete-surrounding",
		    G_CALLBACK (ctk_entry_delete_surrounding_cb), entry);

  ctk_entry_update_cached_style_values (entry);

  priv->drag_gesture = ctk_gesture_drag_new (CTK_WIDGET (entry));
  g_signal_connect (priv->drag_gesture, "drag-update",
                    G_CALLBACK (ctk_entry_drag_gesture_update), entry);
  g_signal_connect (priv->drag_gesture, "drag-end",
                    G_CALLBACK (ctk_entry_drag_gesture_end), entry);
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (priv->drag_gesture), 0);
  ctk_gesture_single_set_exclusive (CTK_GESTURE_SINGLE (priv->drag_gesture), TRUE);

  priv->multipress_gesture = ctk_gesture_multi_press_new (CTK_WIDGET (entry));
  g_signal_connect (priv->multipress_gesture, "pressed",
                    G_CALLBACK (ctk_entry_multipress_gesture_pressed), entry);
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (priv->multipress_gesture), 0);
  ctk_gesture_single_set_exclusive (CTK_GESTURE_SINGLE (priv->multipress_gesture), TRUE);

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (entry));
  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     CTK_WIDGET (entry),
                                                     ctk_entry_measure,
                                                     ctk_entry_allocate,
                                                     ctk_entry_render,
                                                     NULL,
                                                     NULL);

  for (i = 0; i < 2; i++)
    {
      priv->undershoot_node[i] = ctk_css_node_new ();
      ctk_css_node_set_name (priv->undershoot_node[i], I_("undershoot"));
      ctk_css_node_add_class (priv->undershoot_node[i], g_quark_from_static_string (i == 0 ? CTK_STYLE_CLASS_LEFT : CTK_STYLE_CLASS_RIGHT));
      ctk_css_node_set_parent (priv->undershoot_node[i], widget_node);
      ctk_css_node_set_state (priv->undershoot_node[i], ctk_css_node_get_state (widget_node) & ~CTK_STATE_FLAG_DROP_ACTIVE);
      g_object_unref (priv->undershoot_node[i]);
    }
}

static void
ctk_entry_ensure_magnifier (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->magnifier_popover)
    return;

  priv->magnifier = _ctk_magnifier_new (CTK_WIDGET (entry));
  ctk_widget_set_size_request (priv->magnifier, 100, 60);
  _ctk_magnifier_set_magnification (CTK_MAGNIFIER (priv->magnifier), 2.0);
  priv->magnifier_popover = ctk_popover_new (CTK_WIDGET (entry));
  ctk_style_context_add_class (ctk_widget_get_style_context (priv->magnifier_popover),
                               "magnifier");
  ctk_popover_set_modal (CTK_POPOVER (priv->magnifier_popover), FALSE);
  ctk_container_add (CTK_CONTAINER (priv->magnifier_popover),
                     priv->magnifier);
  ctk_container_set_border_width (CTK_CONTAINER (priv->magnifier_popover), 4);
  ctk_widget_show (priv->magnifier);
}

static void
ctk_entry_ensure_text_handles (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->text_handle)
    return;

  priv->text_handle = _ctk_text_handle_new (CTK_WIDGET (entry));
  g_signal_connect (priv->text_handle, "drag-started",
                    G_CALLBACK (ctk_entry_handle_drag_started), entry);
  g_signal_connect (priv->text_handle, "handle-dragged",
                    G_CALLBACK (ctk_entry_handle_dragged), entry);
  g_signal_connect (priv->text_handle, "drag-finished",
                    G_CALLBACK (ctk_entry_handle_drag_finished), entry);
}

static gint
get_icon_width (CtkEntry             *entry,
                CtkEntryIconPosition  icon_pos)
{
  EntryIconInfo *icon_info = entry->priv->icons[icon_pos];
  gint width;

  if (!icon_info)
    return 0;

  ctk_css_gadget_get_preferred_size (icon_info->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     &width, NULL,
                                     NULL, NULL);

  return width;
}

static void
begin_change (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  priv->change_count++;

  g_object_freeze_notify (G_OBJECT (entry));
}

static void
end_change (CtkEntry *entry)
{
  CtkEditable *editable = CTK_EDITABLE (entry);
  CtkEntryPrivate *priv = entry->priv;

  g_return_if_fail (priv->change_count > 0);

  g_object_thaw_notify (G_OBJECT (entry));

  priv->change_count--;

  if (priv->change_count == 0)
    {
       if (priv->real_changed)
         {
           g_signal_emit_by_name (editable, "changed");
           priv->real_changed = FALSE;
         }
    }
}

static void
emit_changed (CtkEntry *entry)
{
  CtkEditable *editable = CTK_EDITABLE (entry);
  CtkEntryPrivate *priv = entry->priv;

  if (priv->change_count == 0)
    g_signal_emit_by_name (editable, "changed");
  else
    priv->real_changed = TRUE;
}

static void
ctk_entry_destroy (CtkWidget *widget)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;

  priv->current_pos = priv->selection_bound = 0;
  ctk_entry_reset_im_context (entry);
  ctk_entry_reset_layout (entry);

  if (priv->blink_timeout)
    {
      g_source_remove (priv->blink_timeout);
      priv->blink_timeout = 0;
    }

  if (priv->magnifier)
    _ctk_magnifier_set_inspected (CTK_MAGNIFIER (priv->magnifier), NULL);

  CTK_WIDGET_CLASS (ctk_entry_parent_class)->destroy (widget);
}

static void
ctk_entry_dispose (GObject *object)
{
  CtkEntry *entry = CTK_ENTRY (object);
  CtkEntryPrivate *priv = entry->priv;
  GdkKeymap *keymap;

  ctk_entry_set_icon_from_pixbuf (entry, CTK_ENTRY_ICON_PRIMARY, NULL);
  ctk_entry_set_icon_tooltip_markup (entry, CTK_ENTRY_ICON_PRIMARY, NULL);
  ctk_entry_set_icon_from_pixbuf (entry, CTK_ENTRY_ICON_SECONDARY, NULL);
  ctk_entry_set_icon_tooltip_markup (entry, CTK_ENTRY_ICON_SECONDARY, NULL);
  ctk_entry_set_completion (entry, NULL);

  priv->current_pos = 0;

  if (priv->buffer)
    {
      buffer_disconnect_signals (entry);
      g_object_unref (priv->buffer);
      priv->buffer = NULL;
    }

  keymap = gdk_keymap_get_for_display (ctk_widget_get_display (CTK_WIDGET (object)));
  g_signal_handlers_disconnect_by_func (keymap, keymap_state_changed, entry);
  g_signal_handlers_disconnect_by_func (keymap, keymap_direction_changed, entry);
  G_OBJECT_CLASS (ctk_entry_parent_class)->dispose (object);
}

static void
ctk_entry_finalize (GObject *object)
{
  CtkEntry *entry = CTK_ENTRY (object);
  CtkEntryPrivate *priv = entry->priv;
  EntryIconInfo *icon_info = NULL;
  gint i;

  if (priv->tick_id)
    ctk_widget_remove_tick_callback (CTK_WIDGET (entry), priv->tick_id);

  for (i = 0; i < MAX_ICONS; i++)
    {
      icon_info = priv->icons[i];
      if (icon_info == NULL)
        continue;

      if (icon_info->target_list != NULL)
        ctk_target_list_unref (icon_info->target_list);

      g_clear_object (&icon_info->gadget);

      g_slice_free (EntryIconInfo, icon_info);
    }

  if (priv->cached_layout)
    g_object_unref (priv->cached_layout);

  g_object_unref (priv->im_context);

  if (priv->blink_timeout)
    g_source_remove (priv->blink_timeout);

  if (priv->selection_bubble)
    ctk_widget_destroy (priv->selection_bubble);

  if (priv->magnifier_popover)
    ctk_widget_destroy (priv->magnifier_popover);

  if (priv->text_handle)
    g_object_unref (priv->text_handle);
  g_free (priv->placeholder_text);
  g_free (priv->im_module);

  g_clear_object (&priv->drag_gesture);
  g_clear_object (&priv->multipress_gesture);

  if (priv->tabs)
    pango_tab_array_free (priv->tabs);

  if (priv->attrs)
    pango_attr_list_unref (priv->attrs);

  g_clear_object (&priv->progress_gadget);
  g_clear_object (&priv->gadget);

  G_OBJECT_CLASS (ctk_entry_parent_class)->finalize (object);
}

static DisplayMode
ctk_entry_get_display_mode (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->visible)
    return DISPLAY_NORMAL;

  if (priv->invisible_char == 0 && priv->invisible_char_set)
    return DISPLAY_BLANK;

  return DISPLAY_INVISIBLE;
}

gchar*
_ctk_entry_get_display_text (CtkEntry *entry,
                             gint      start_pos,
                             gint      end_pos)
{
  CtkEntryPasswordHint *password_hint;
  CtkEntryPrivate *priv;
  gunichar invisible_char;
  const gchar *start;
  const gchar *end;
  const gchar *text;
  gchar char_str[7];
  gint char_len;
  GString *str;
  guint length;
  gint i;

  priv = entry->priv;
  text = ctk_entry_buffer_get_text (get_buffer (entry));
  length = ctk_entry_buffer_get_length (get_buffer (entry));

  if (end_pos < 0 || end_pos > length)
    end_pos = length;
  if (start_pos > length)
    start_pos = length;

  if (end_pos <= start_pos)
      return g_strdup ("");
  else if (priv->visible)
    {
      start = g_utf8_offset_to_pointer (text, start_pos);
      end = g_utf8_offset_to_pointer (start, end_pos - start_pos);
      return g_strndup (start, end - start);
    }
  else
    {
      str = g_string_sized_new (length * 2);

      /* Figure out what our invisible char is and encode it */
      if (!priv->invisible_char)
          invisible_char = priv->invisible_char_set ? ' ' : '*';
      else
          invisible_char = priv->invisible_char;
      char_len = g_unichar_to_utf8 (invisible_char, char_str);

      /*
       * Add hidden characters for each character in the text
       * buffer. If there is a password hint, then keep that
       * character visible.
       */

      password_hint = g_object_get_qdata (G_OBJECT (entry), quark_password_hint);
      for (i = start_pos; i < end_pos; ++i)
        {
          if (password_hint && i == password_hint->position)
            {
              start = g_utf8_offset_to_pointer (text, i);
              g_string_append_len (str, start, g_utf8_next_char (start) - start);
            }
          else
            {
              g_string_append_len (str, char_str, char_len);
            }
        }

      return g_string_free (str, FALSE);
    }
}

static void
update_cursors (CtkWidget *widget)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  EntryIconInfo *icon_info = NULL;
  GdkDisplay *display;
  GdkCursor *cursor;
  gint i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      if ((icon_info = priv->icons[i]) != NULL)
        {
          if (!_ctk_icon_helper_get_is_empty (CTK_ICON_HELPER (icon_info->gadget)) && 
              icon_info->window != NULL)
            gdk_window_show_unraised (icon_info->window);

          /* The icon windows are not children of the visible entry window,
           * thus we can't just inherit the xterm cursor. Slight complication 
           * here is that for the entry, insensitive => arrow cursor, but for 
           * an icon in a sensitive entry, insensitive => xterm cursor.
           */
          if (ctk_widget_is_sensitive (widget) &&
              (icon_info->insensitive || 
               (icon_info->nonactivatable && icon_info->target_list == NULL)))
            {
              display = ctk_widget_get_display (widget);
              cursor = gdk_cursor_new_from_name (display, "text");
              gdk_window_set_cursor (icon_info->window, cursor);
              g_clear_object (&cursor);
            }
          else
            {
              gdk_window_set_cursor (icon_info->window, NULL);
            }
        }
    }
}

static void
realize_icon_info (CtkWidget            *widget, 
                   CtkEntryIconPosition  icon_pos)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  EntryIconInfo *icon_info = priv->icons[icon_pos];
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (icon_info != NULL);

  attributes.x = 0;
  attributes.y = 0;
  attributes.width = 1;
  attributes.height = 1;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = ctk_widget_get_events (widget);
  attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
                                GDK_BUTTON_RELEASE_MASK |
                                GDK_BUTTON1_MOTION_MASK |
                                GDK_BUTTON3_MOTION_MASK |
                                GDK_POINTER_MOTION_MASK |
                                GDK_ENTER_NOTIFY_MASK |
                            GDK_LEAVE_NOTIFY_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y;

  icon_info->window = gdk_window_new (ctk_widget_get_window (widget),
                                      &attributes,
                                      attributes_mask);
  ctk_widget_register_window (widget, icon_info->window);

  ctk_widget_queue_resize (widget);
}

static void
update_icon_style (CtkWidget            *widget,
                   CtkEntryIconPosition  icon_pos)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  EntryIconInfo *icon_info = priv->icons[icon_pos];
  const gchar *sides[2] = { CTK_STYLE_CLASS_LEFT, CTK_STYLE_CLASS_RIGHT };

  if (icon_info == NULL)
    return;

  if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL)
    icon_pos = 1 - icon_pos;

  ctk_css_gadget_add_class (icon_info->gadget, sides[icon_pos]);
  ctk_css_gadget_remove_class (icon_info->gadget, sides[1 - icon_pos]);
}

static void
update_icon_state (CtkWidget            *widget,
                   CtkEntryIconPosition  icon_pos)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  EntryIconInfo *icon_info = priv->icons[icon_pos];
  CtkStateFlags state;

  if (icon_info == NULL)
    return;

  state = ctk_widget_get_state_flags (widget);
  state &= ~(CTK_STATE_FLAG_PRELIGHT | CTK_STATE_FLAG_DROP_ACTIVE);

  if ((state & CTK_STATE_FLAG_INSENSITIVE) || icon_info->insensitive)
    state |= CTK_STATE_FLAG_INSENSITIVE;
  else if (icon_info->prelight)
    state |= CTK_STATE_FLAG_PRELIGHT;

  ctk_css_gadget_set_state (icon_info->gadget, state);
}

static void
update_node_state (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkStateFlags state;

  state = ctk_widget_get_state_flags (CTK_WIDGET (entry));
  state &= ~CTK_STATE_FLAG_DROP_ACTIVE;

  if (priv->progress_gadget)
    ctk_css_gadget_set_state (priv->progress_gadget, state);
  if (priv->selection_node)
    ctk_css_node_set_state (priv->selection_node, state);

  ctk_css_node_set_state (priv->undershoot_node[0], state);
  ctk_css_node_set_state (priv->undershoot_node[1], state);
}

static void
update_node_ordering (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  EntryIconInfo *icon_info;
  CtkEntryIconPosition icon_pos;
  CtkCssNode *sibling, *parent;

  if (priv->progress_gadget)
    {
      ctk_css_node_insert_before (ctk_css_gadget_get_node (priv->gadget),
                                  ctk_css_gadget_get_node (priv->progress_gadget),
                                  NULL);
    }

  if (ctk_widget_get_direction (CTK_WIDGET (entry)) == CTK_TEXT_DIR_RTL)
    icon_pos = CTK_ENTRY_ICON_SECONDARY;
  else
    icon_pos = CTK_ENTRY_ICON_PRIMARY;

  icon_info = priv->icons[icon_pos];
  if (icon_info)
    {
      CtkCssNode *node;

      node = ctk_css_gadget_get_node (icon_info->gadget);
      parent = ctk_css_node_get_parent (node);
      sibling = ctk_css_node_get_first_child (parent);
      if (node != sibling)
        ctk_css_node_insert_before (parent, node, sibling);
    }
}

static EntryIconInfo*
construct_icon_info (CtkWidget            *widget,
                     CtkEntryIconPosition  icon_pos)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  EntryIconInfo *icon_info;
  CtkCssNode *widget_node;

  g_return_val_if_fail (priv->icons[icon_pos] == NULL, NULL);

  icon_info = g_slice_new0 (EntryIconInfo);
  priv->icons[icon_pos] = icon_info;

  widget_node = ctk_css_gadget_get_node (priv->gadget);
  icon_info->gadget = ctk_icon_helper_new_named ("image", widget);
  _ctk_icon_helper_set_force_scale_pixbuf (CTK_ICON_HELPER (icon_info->gadget), TRUE);
  ctk_css_node_set_parent (ctk_css_gadget_get_node (icon_info->gadget), widget_node);

  update_icon_state (widget, icon_pos);
  update_icon_style (widget, icon_pos);
  update_node_ordering (entry);

  if (ctk_widget_get_realized (widget))
    realize_icon_info (widget, icon_pos);

  return icon_info;
}

static void
ctk_entry_map (CtkWidget *widget)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  EntryIconInfo *icon_info = NULL;
  gint i;

  CTK_WIDGET_CLASS (ctk_entry_parent_class)->map (widget);

  gdk_window_show (priv->text_area);

  for (i = 0; i < MAX_ICONS; i++)
    {
      if ((icon_info = priv->icons[i]) != NULL)
        {
          if (!_ctk_icon_helper_get_is_empty (CTK_ICON_HELPER (icon_info->gadget)) &&
              icon_info->window != NULL)
            gdk_window_show (icon_info->window);
        }
    }

  update_cursors (widget);
}

static void
ctk_entry_unmap (CtkWidget *widget)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  EntryIconInfo *icon_info = NULL;
  gint i;

  if (priv->text_handle)
    _ctk_text_handle_set_mode (priv->text_handle,
                               CTK_TEXT_HANDLE_MODE_NONE);

  for (i = 0; i < MAX_ICONS; i++)
    {
      if ((icon_info = priv->icons[i]) != NULL)
        {
          if (!_ctk_icon_helper_get_is_empty (CTK_ICON_HELPER (icon_info->gadget)) && 
              icon_info->window != NULL)
            gdk_window_hide (icon_info->window);
        }
    }

  gdk_window_hide (priv->text_area);

  CTK_WIDGET_CLASS (ctk_entry_parent_class)->unmap (widget);
}

static void
ctk_entry_realize (CtkWidget *widget)
{
  CtkEntry *entry;
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;
  GdkWindowAttr attributes;
  gint attributes_mask;
  int i;

  CTK_WIDGET_CLASS (ctk_entry_parent_class)->realize (widget);

  entry = CTK_ENTRY (widget);
  priv = entry->priv;

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = ctk_widget_get_events (widget);
  attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK |
			    GDK_BUTTON1_MOTION_MASK |
			    GDK_BUTTON3_MOTION_MASK |
			    GDK_POINTER_MOTION_MASK |
                            GDK_ENTER_NOTIFY_MASK |
			    GDK_LEAVE_NOTIFY_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y;

  attributes.x = priv->text_allocation.x;
  attributes.y = priv->text_allocation.y;
  attributes.width = priv->text_allocation.width;
  attributes.height = priv->text_allocation.height;

  if (ctk_widget_is_sensitive (widget))
    {
      attributes.cursor = gdk_cursor_new_from_name (ctk_widget_get_display (widget), "text");
      attributes_mask |= GDK_WA_CURSOR;
    }

  priv->text_area = gdk_window_new (ctk_widget_get_window (widget),
                                    &attributes,
                                    attributes_mask);

  ctk_widget_register_window (widget, priv->text_area);

  if (attributes_mask & GDK_WA_CURSOR)
    g_clear_object (&attributes.cursor);

  ctk_im_context_set_client_window (priv->im_context, priv->text_area);

  ctk_entry_adjust_scroll (entry);
  ctk_entry_update_primary_selection (entry);

  /* If the icon positions are already setup, create their windows.
   * Otherwise if they don't exist yet, then construct_icon_info()
   * will create the windows once the widget is already realized.
   */
  for (i = 0; i < MAX_ICONS; i++)
    {
      if ((icon_info = priv->icons[i]) != NULL)
        {
          if (icon_info->window == NULL)
            realize_icon_info (widget, i);
        }
    }
}

static void
ctk_entry_unrealize (CtkWidget *widget)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  CtkClipboard *clipboard;
  EntryIconInfo *icon_info;
  gint i;

  ctk_entry_reset_layout (entry);
  
  ctk_im_context_set_client_window (priv->im_context, NULL);

  clipboard = ctk_widget_get_clipboard (widget, GDK_SELECTION_PRIMARY);
  if (ctk_clipboard_get_owner (clipboard) == G_OBJECT (entry))
    ctk_clipboard_clear (clipboard);
  
  if (priv->text_area)
    {
      ctk_widget_unregister_window (widget, priv->text_area);
      gdk_window_destroy (priv->text_area);
      priv->text_area = NULL;
    }

  if (priv->popup_menu)
    {
      ctk_widget_destroy (priv->popup_menu);
      priv->popup_menu = NULL;
    }

  CTK_WIDGET_CLASS (ctk_entry_parent_class)->unrealize (widget);

  for (i = 0; i < MAX_ICONS; i++)
    {
      if ((icon_info = priv->icons[i]) != NULL)
        {
          if (icon_info->window != NULL)
            {
              ctk_widget_unregister_window (widget, icon_info->window);
              gdk_window_destroy (icon_info->window);
              icon_info->window = NULL;
            }
        }
    }
}

static void
ctk_entry_get_preferred_width (CtkWidget *widget,
                               gint      *minimum,
                               gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_ENTRY (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_entry_get_preferred_height (CtkWidget *widget,
                                gint      *minimum,
                                gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_ENTRY (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_entry_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
						       gint       width,
						       gint      *minimum,
						       gint      *natural,
						       gint      *minimum_baseline,
						       gint      *natural_baseline)
{
  ctk_css_gadget_get_preferred_size (CTK_ENTRY (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     minimum_baseline, natural_baseline);
}

static void
ctk_entry_measure (CtkCssGadget   *gadget,
                   CtkOrientation  orientation,
                   int             for_size,
                   int            *minimum,
                   int            *natural,
                   int            *minimum_baseline,
                   int            *natural_baseline,
                   gpointer        unused)
{
  CtkWidget *widget;
  CtkEntry *entry;
  CtkEntryPrivate *priv;
  PangoContext *context;
  PangoFontMetrics *metrics;

  widget = ctk_css_gadget_get_owner (gadget);
  entry = CTK_ENTRY (widget);
  priv = entry->priv;

  context = ctk_widget_get_pango_context (widget);
  metrics = pango_context_get_metrics (context,
                                       pango_context_get_font_description (context),
                                       pango_context_get_language (context));

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      gint icon_width, i;
      gint min, nat;
      gint char_width;
      gint digit_width;
      gint char_pixels;

      char_width = pango_font_metrics_get_approximate_char_width (metrics);
      digit_width = pango_font_metrics_get_approximate_digit_width (metrics);
      char_pixels = (MAX (char_width, digit_width) + PANGO_SCALE - 1) / PANGO_SCALE;

      if (priv->width_chars < 0)
        {
          if (CTK_IS_SPIN_BUTTON (entry))
            min = ctk_spin_button_get_text_width (CTK_SPIN_BUTTON (entry));
          else
            min = MIN_ENTRY_WIDTH;
        }
      else
        {
          min = char_pixels * priv->width_chars;
        }

      if (priv->max_width_chars < 0)
        nat = min;
      else
        nat = char_pixels * priv->max_width_chars;

      icon_width = 0;
      for (i = 0; i < MAX_ICONS; i++)
        icon_width += get_icon_width (entry, i);

      min = MAX (min, icon_width);
      nat = MAX (min, nat);

      *minimum = min;
      *natural = nat;
    }
  else
    {
      gint height, baseline;
      gint icon_height, i;
      PangoLayout *layout;

      layout = ctk_entry_ensure_layout (entry, TRUE);

      priv->ascent = pango_font_metrics_get_ascent (metrics);
      priv->descent = pango_font_metrics_get_descent (metrics);

      pango_layout_get_pixel_size (layout, NULL, &height);

      height = MAX (height, PANGO_PIXELS (priv->ascent + priv->descent));

      baseline = pango_layout_get_baseline (layout) / PANGO_SCALE;

      icon_height = 0;
      for (i = 0; i < MAX_ICONS; i++)
        {
          EntryIconInfo *icon_info = priv->icons[i];
          gint h;

          if (!icon_info)
            continue;

          ctk_css_gadget_get_preferred_size (icon_info->gadget,
                                             CTK_ORIENTATION_VERTICAL,
                                             -1,
                                             NULL, &h,
                                             NULL, NULL);
          icon_height = MAX (icon_height, h);
        }

      *minimum = MAX (height, icon_height);
      *natural = MAX (height, icon_height);

      if (icon_height > height)
        baseline += (icon_height - height) / 2;

      if (minimum_baseline)
        *minimum_baseline = baseline;
      if (natural_baseline)
        *natural_baseline = baseline;
    }

  pango_font_metrics_unref (metrics);

  if (priv->progress_gadget && ctk_css_gadget_get_visible (priv->progress_gadget))
    {
      int prog_min, prog_nat;

      ctk_css_gadget_get_preferred_size (priv->progress_gadget,
                                         orientation,
                                         for_size,
                                         &prog_min, &prog_nat,
                                         NULL, NULL);

      *minimum = MAX (*minimum, prog_min);
      *natural = MAX (*natural, prog_nat);
    }
}

static void
place_windows (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  EntryIconInfo *icon_info;

  icon_info = priv->icons[CTK_ENTRY_ICON_PRIMARY];
  if (icon_info)
    {
      CtkAllocation primary;

      ctk_css_gadget_get_border_allocation (icon_info->gadget, &primary, NULL);
      gdk_window_move_resize (icon_info->window,
                              primary.x, primary.y,
                              primary.width, primary.height);
    }

  icon_info = priv->icons[CTK_ENTRY_ICON_SECONDARY];
  if (icon_info)
    {
      CtkAllocation secondary;

      ctk_css_gadget_get_border_allocation (icon_info->gadget, &secondary, NULL);
      gdk_window_move_resize (icon_info->window,
                              secondary.x, secondary.y,
                              secondary.width, secondary.height);
    }

  gdk_window_move_resize (priv->text_area,
                          priv->text_allocation.x, priv->text_allocation.y,
                          priv->text_allocation.width, priv->text_allocation.height);
}

static void
ctk_entry_get_text_area_size (CtkEntry *entry,
                              gint     *x,
			      gint     *y,
			      gint     *width,
			      gint     *height)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkAllocation allocation, widget_allocation;
  int baseline;

  ctk_css_gadget_get_content_allocation (priv->gadget, &allocation, &baseline);
  ctk_widget_get_allocation (CTK_WIDGET (entry), &widget_allocation);

  if (x)
    *x = allocation.x - widget_allocation.x;

  if (y)
    *y = allocation.y - widget_allocation.y;

  if (width)
    *width = allocation.width;

  if (height)
    *height = allocation.height;

  priv->text_baseline = baseline;
}

static void
ctk_entry_get_frame_size (CtkEntry *entry,
                          gint     *x,
                          gint     *y,
                          gint     *width,
                          gint     *height)
{
  CtkAllocation allocation;

  ctk_css_gadget_get_content_allocation (entry->priv->gadget, &allocation, NULL);

  if (x)
    *x = allocation.x;

  if (y)
    *y = allocation.y;

  if (width)
    *width = allocation.width;

  if (height)
    *height = allocation.height;
}

static G_GNUC_UNUSED void
get_frame_size (CtkEntry *entry,
                gboolean  relative_to_window,
                gint     *x,
                gint     *y,
                gint     *width,
                gint     *height)
{
  CtkEntryClass *class = CTK_ENTRY_GET_CLASS (entry);

  g_assert (class->get_frame_size != NULL);
  class->get_frame_size (entry, x, y, width, height);

  if (!relative_to_window)
    {
      CtkAllocation allocation;

      ctk_widget_get_allocation (CTK_WIDGET (entry), &allocation);

      if (x)
        *x -= allocation.x;
      if (y)
        *y -= allocation.y;
    }
}

static void
ctk_entry_size_allocate (CtkWidget     *widget,
			 CtkAllocation *allocation)
{
  GdkRectangle clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (CTK_ENTRY (widget)->priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_entry_allocate (CtkCssGadget        *gadget,
                    const CtkAllocation *allocation,
                    int                  baseline,
                    CtkAllocation       *out_clip,
                    gpointer             data)
{
  CtkEntry *entry;
  CtkWidget *widget;
  CtkEntryPrivate *priv;
  CtkAllocation widget_allocation;
  gint i;

  widget = ctk_css_gadget_get_owner (gadget);
  entry = CTK_ENTRY (widget);
  priv = entry->priv;

  priv->text_baseline = -1;
  CTK_ENTRY_GET_CLASS (entry)->get_text_area_size (entry,
                                                   &priv->text_allocation.x,
                                                   &priv->text_allocation.y,
                                                   &priv->text_allocation.width,
                                                   &priv->text_allocation.height);
  ctk_widget_get_allocation (widget, &widget_allocation);
  priv->text_allocation.x += widget_allocation.x;
  priv->text_allocation.y += widget_allocation.y;

  out_clip->x = 0;
  out_clip->y = 0;
  out_clip->width = 0;
  out_clip->height = 0;

  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];
      CtkAllocation icon_alloc;
      GdkRectangle clip;
      gint dummy, width, height;

      if (!icon_info)
        continue;

      ctk_css_gadget_get_preferred_size (icon_info->gadget,
                                         CTK_ORIENTATION_HORIZONTAL,
                                         -1,
                                         &dummy, &width,
                                         NULL, NULL);
      ctk_css_gadget_get_preferred_size (icon_info->gadget,
                                         CTK_ORIENTATION_VERTICAL,
                                         -1,
                                         &dummy, &height,
                                         NULL, NULL);

      if ((ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL && i == CTK_ENTRY_ICON_PRIMARY) ||
          (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_LTR && i == CTK_ENTRY_ICON_SECONDARY))
        {
          icon_alloc.x = priv->text_allocation.x + priv->text_allocation.width - width;
        }
      else
        {
          icon_alloc.x = priv->text_allocation.x;
          priv->text_allocation.x += width;
        }
      icon_alloc.y = priv->text_allocation.y + (priv->text_allocation.height - height) / 2;
      icon_alloc.width = width;
      icon_alloc.height = height;
      priv->text_allocation.width -= width;

      ctk_css_gadget_allocate (icon_info->gadget,
                               &icon_alloc,
                               baseline,
                               &clip);

      gdk_rectangle_union (out_clip, &clip, out_clip);
    }

  if (priv->progress_gadget && ctk_css_gadget_get_visible (priv->progress_gadget))
    {
      int extra_width, req_width;
      CtkAllocation progress_alloc;
      GdkRectangle clip;

      ctk_css_gadget_get_preferred_size (priv->progress_gadget,
                                         CTK_ORIENTATION_HORIZONTAL,
                                         allocation->height,
                                         &req_width, NULL,
                                         NULL, NULL);
      extra_width = allocation->width - req_width;

      progress_alloc = *allocation;

      if (priv->progress_pulse_mode)
        {
          gdouble value = priv->progress_pulse_current;

          progress_alloc.x += (gint) floor (value * extra_width);
          progress_alloc.width = req_width + (gint) ceil (priv->progress_pulse_fraction * extra_width);
        }
      else
        {
          gdouble value = priv->progress_fraction;

          progress_alloc.width = req_width + (gint) nearbyint (value * extra_width);
          if (ctk_widget_get_direction (CTK_WIDGET (entry)) == CTK_TEXT_DIR_RTL)
            progress_alloc.x += allocation->width - progress_alloc.width;
        }

      ctk_css_gadget_allocate (priv->progress_gadget, &progress_alloc, baseline, &clip);

      gdk_rectangle_union (out_clip, &clip, out_clip);
    }

  /* Do this here instead of ctk_entry_size_allocate() so it works
   * inside spinbuttons, which don't chain up.
   */
  if (ctk_widget_get_realized (widget))
    {
      CtkEntryCompletion *completion;

      place_windows (entry);
      ctk_entry_recompute (entry);

      completion = ctk_entry_get_completion (entry);
      if (completion)
        _ctk_entry_completion_resize_popup (completion);
    }
}

static gboolean
should_prelight (CtkEntry             *entry,
                 CtkEntryIconPosition  icon_pos)
{
  CtkEntryPrivate *priv = entry->priv;
  EntryIconInfo *icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return FALSE;

  if (icon_info->nonactivatable && icon_info->target_list == NULL)
    return FALSE;

  if (icon_info->pressed)
    return FALSE;

  return TRUE;
}

static gboolean
ctk_entry_draw (CtkWidget *widget,
		cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_ENTRY (widget)->priv->gadget, cr);

  return GDK_EVENT_PROPAGATE;
}

#define UNDERSHOOT_SIZE 20

static void
ctk_entry_draw_undershoot (CtkEntry *entry,
                           cairo_t  *cr)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkStyleContext *context;
  gint min_offset, max_offset;
  CtkAllocation allocation;
  GdkRectangle rect;
  gboolean rtl;

  context = ctk_widget_get_style_context (CTK_WIDGET (entry));
  rtl = ctk_widget_get_direction (CTK_WIDGET (entry)) == CTK_TEXT_DIR_RTL;

  ctk_entry_get_scroll_limits (entry, &min_offset, &max_offset);

  ctk_css_gadget_get_content_allocation (priv->gadget, &rect, NULL);
  ctk_widget_get_allocation (CTK_WIDGET (entry), &allocation);
  rect.x -= allocation.x;
  rect.y -= allocation.y;

  if (priv->scroll_offset > min_offset)
    {
      int icon_width = 0;
      int icon_idx = rtl ? 1 : 0;
      if (priv->icons[icon_idx] != NULL)
        {
           ctk_css_gadget_get_preferred_size (priv->icons[icon_idx]->gadget,
                                              CTK_ORIENTATION_HORIZONTAL,
                                              -1,
                                              &icon_width, NULL,
                                              NULL, NULL);
        }

      ctk_style_context_save_to_node (context, priv->undershoot_node[0]);
      ctk_render_background (context, cr, rect.x + icon_width - 1, rect.y, UNDERSHOOT_SIZE, rect.height);
      ctk_render_frame (context, cr, rect.x + icon_width - 1, rect.y, UNDERSHOOT_SIZE, rect.height);
      ctk_style_context_restore (context);
    }

  if (priv->scroll_offset < max_offset)
    {
      int icon_width = 0;
      int icon_idx = rtl ? 0 : 1;
      if (priv->icons[icon_idx] != NULL)
        {
           ctk_css_gadget_get_preferred_size (priv->icons[icon_idx]->gadget,
                                              CTK_ORIENTATION_HORIZONTAL,
                                              -1,
                                              &icon_width, NULL,
                                              NULL, NULL);
        }
      ctk_style_context_save_to_node (context, priv->undershoot_node[1]);
      ctk_render_background (context, cr, rect.x + rect.width - UNDERSHOOT_SIZE - icon_width + 1, rect.y, UNDERSHOOT_SIZE, rect.height);
      ctk_render_frame (context, cr, rect.x + rect.width - UNDERSHOOT_SIZE - icon_width + 1, rect.y, UNDERSHOOT_SIZE, rect.height);
      ctk_style_context_restore (context);
    }
}

static gboolean
ctk_entry_render (CtkCssGadget *gadget,
                  cairo_t      *cr,
                  int           x,
                  int           y,
                  int           width,
                  int           height,
                  gpointer      data)
{
  CtkWidget *widget;
  CtkEntry *entry;
  CtkEntryPrivate *priv;
  int i;

  widget = ctk_css_gadget_get_owner (gadget);
  entry = CTK_ENTRY (widget);
  priv = entry->priv;

  /* Draw progress */
  if (priv->progress_gadget && ctk_css_gadget_get_visible (priv->progress_gadget))
    ctk_css_gadget_draw (priv->progress_gadget, cr);

  /* Draw text and cursor */
  cairo_save (cr);

  if (priv->dnd_position != -1)
    ctk_entry_draw_cursor (CTK_ENTRY (widget), cr, CURSOR_DND);

  ctk_entry_draw_text (CTK_ENTRY (widget), cr);

  /* When no text is being displayed at all, don't show the cursor */
  if (ctk_entry_get_display_mode (entry) != DISPLAY_BLANK &&
      ctk_widget_has_focus (widget) &&
      priv->selection_bound == priv->current_pos && priv->cursor_visible)
    ctk_entry_draw_cursor (CTK_ENTRY (widget), cr, CURSOR_STANDARD);

  cairo_restore (cr);

  /* Draw icons */
  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];

      if (icon_info != NULL)
        ctk_css_gadget_draw (icon_info->gadget, cr);
    }

  ctk_entry_draw_undershoot (entry, cr);

  return FALSE;
}

static gint
ctk_entry_enter_notify (CtkWidget        *widget,
                        GdkEventCrossing *event)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  gint i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];

      if (icon_info != NULL && event->window == icon_info->window)
        {
          if (should_prelight (entry, i))
            {
              icon_info->prelight = TRUE;
              update_icon_state (widget, i);
              ctk_widget_queue_draw (widget);
            }

          break;
        }
    }

    return GDK_EVENT_PROPAGATE;
}

static gint
ctk_entry_leave_notify (CtkWidget        *widget,
                        GdkEventCrossing *event)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  gint i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];

      if (icon_info != NULL && event->window == icon_info->window)
        {
          /* a grab means that we may never see the button release */
          if (event->mode == GDK_CROSSING_GRAB || event->mode == GDK_CROSSING_CTK_GRAB)
            icon_info->pressed = FALSE;

          if (should_prelight (entry, i))
            {
              icon_info->prelight = FALSE;
              update_icon_state (widget, i);
              ctk_widget_queue_draw (widget);
            }

          break;
        }
    }

  return GDK_EVENT_PROPAGATE;
}

static void
ctk_entry_get_pixel_ranges (CtkEntry  *entry,
			    gint     **ranges,
			    gint      *n_ranges)
{
  gint start_char, end_char;

  if (ctk_editable_get_selection_bounds (CTK_EDITABLE (entry), &start_char, &end_char))
    {
      PangoLayout *layout = ctk_entry_ensure_layout (entry, TRUE);
      PangoLayoutLine *line = pango_layout_get_lines_readonly (layout)->data;
      const char *text = pango_layout_get_text (layout);
      gint start_index = g_utf8_offset_to_pointer (text, start_char) - text;
      gint end_index = g_utf8_offset_to_pointer (text, end_char) - text;
      gint real_n_ranges, i;

      pango_layout_line_get_x_ranges (line, start_index, end_index, ranges, &real_n_ranges);

      if (ranges)
	{
	  gint *r = *ranges;
	  
	  for (i = 0; i < real_n_ranges; ++i)
	    {
	      r[2 * i + 1] = (r[2 * i + 1] - r[2 * i]) / PANGO_SCALE;
	      r[2 * i] = r[2 * i] / PANGO_SCALE;
	    }
	}
      
      if (n_ranges)
	*n_ranges = real_n_ranges;
    }
  else
    {
      if (n_ranges)
	*n_ranges = 0;
      if (ranges)
	*ranges = NULL;
    }
}

static gboolean
in_selection (CtkEntry *entry,
	      gint	x)
{
  gint *ranges;
  gint n_ranges, i;
  gint retval = FALSE;

  ctk_entry_get_pixel_ranges (entry, &ranges, &n_ranges);

  for (i = 0; i < n_ranges; ++i)
    {
      if (x >= ranges[2 * i] && x < ranges[2 * i] + ranges[2 * i + 1])
	{
	  retval = TRUE;
	  break;
	}
    }

  g_free (ranges);
  return retval;
}

static void
ctk_entry_move_handle (CtkEntry              *entry,
                       CtkTextHandlePosition  pos,
                       gint                   x,
                       gint                   y,
                       gint                   height)
{
  CtkEntryPrivate *priv = entry->priv;

  if (!_ctk_text_handle_get_is_dragged (priv->text_handle, pos) &&
      (x < 0 || x > priv->text_allocation.width))
    {
      /* Hide the handle if it's not being manipulated
       * and fell outside of the visible text area.
       */
      _ctk_text_handle_set_visible (priv->text_handle, pos, FALSE);
    }
  else
    {
      CtkAllocation allocation;
      GdkRectangle rect;

      ctk_widget_get_allocation (CTK_WIDGET (entry), &allocation);
      rect.x = x + priv->text_allocation.x - allocation.x;
      rect.y = y + priv->text_allocation.y - allocation.y;
      rect.width = 1;
      rect.height = height;

      _ctk_text_handle_set_visible (priv->text_handle, pos, TRUE);
      _ctk_text_handle_set_position (priv->text_handle, pos, &rect);
      _ctk_text_handle_set_direction (priv->text_handle, pos, priv->resolved_dir);
    }
}

static gint
ctk_entry_get_selection_bound_location (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  PangoLayout *layout;
  PangoRectangle pos;
  gint x;
  const gchar *text;
  gint index;

  layout = ctk_entry_ensure_layout (entry, FALSE);
  text = pango_layout_get_text (layout);
  index = g_utf8_offset_to_pointer (text, priv->selection_bound) - text;
  pango_layout_index_to_pos (layout, index, &pos);

  if (ctk_widget_get_direction (CTK_WIDGET (entry)) == CTK_TEXT_DIR_RTL)
    x = (pos.x + pos.width) / PANGO_SCALE;
  else
    x = pos.x / PANGO_SCALE;

  return x;
}

static void
ctk_entry_update_handles (CtkEntry          *entry,
                          CtkTextHandleMode  mode)
{
  CtkEntryPrivate *priv = entry->priv;
  gint strong_x, height;
  gint cursor, bound;

  _ctk_text_handle_set_mode (priv->text_handle, mode);

  height = gdk_window_get_height (priv->text_area);

  ctk_entry_get_cursor_locations (entry, CURSOR_STANDARD, &strong_x, NULL);
  cursor = strong_x - priv->scroll_offset;

  if (mode == CTK_TEXT_HANDLE_MODE_SELECTION)
    {
      gint start, end;

      bound = ctk_entry_get_selection_bound_location (entry) - priv->scroll_offset;

      if (priv->selection_bound > priv->current_pos)
        {
          start = cursor;
          end = bound;
        }
      else
        {
          start = bound;
          end = cursor;
        }

      /* Update start selection bound */
      ctk_entry_move_handle (entry, CTK_TEXT_HANDLE_POSITION_SELECTION_START,
                             start, 0, height);
      ctk_entry_move_handle (entry, CTK_TEXT_HANDLE_POSITION_SELECTION_END,
                             end, 0, height);
    }
  else
    ctk_entry_move_handle (entry, CTK_TEXT_HANDLE_POSITION_CURSOR,
                           cursor, 0, height);
}

static gboolean
ctk_entry_event (CtkWidget *widget,
                 GdkEvent  *event)
{
  CtkEntryPrivate *priv = CTK_ENTRY (widget)->priv;
  EntryIconInfo *icon_info = NULL;
  GdkEventSequence *sequence;
  GdkDevice *device;
  gdouble x, y;
  gint i;

  if (event->type == GDK_MOTION_NOTIFY &&
      priv->mouse_cursor_obscured &&
      event->any.window == priv->text_area)
    {
      GdkCursor *cursor;

      cursor = gdk_cursor_new_from_name (ctk_widget_get_display (widget), "text");
      gdk_window_set_cursor (priv->text_area, cursor);
      g_object_unref (cursor);
      priv->mouse_cursor_obscured = FALSE;
      return GDK_EVENT_PROPAGATE;
    }

  for (i = 0; i < MAX_ICONS; i++)
    {
      if (priv->icons[i] &&
          event->any.window != NULL &&
          priv->icons[i]->window == event->any.window)
        {
          icon_info = priv->icons[i];
          break;
        }
    }

  if (!icon_info)
    return GDK_EVENT_PROPAGATE;

  if (icon_info->insensitive)
    return GDK_EVENT_STOP;

  sequence = gdk_event_get_event_sequence (event);
  device = gdk_event_get_device (event);
  gdk_event_get_coords (event, &x, &y);

  switch (event->type)
    {
    case GDK_TOUCH_BEGIN:
      if (icon_info->current_sequence)
        break;

      icon_info->current_sequence = sequence;
      /* Fall through */
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
      if (should_prelight (CTK_ENTRY (widget), i))
        {
          icon_info->prelight = FALSE;
          update_icon_state (widget, i);
          ctk_widget_queue_draw (widget);
        }

      priv->start_x = x;
      priv->start_y = y;
      icon_info->pressed = TRUE;
      icon_info->device = device;

      if (!icon_info->nonactivatable)
        g_signal_emit (widget, signals[ICON_PRESS], 0, i, event);

      break;
    case GDK_TOUCH_UPDATE:
      if (icon_info->device != device ||
          icon_info->current_sequence != sequence)
        break;
      /* Fall through */
    case GDK_MOTION_NOTIFY:
      if (icon_info->pressed &&
          icon_info->target_list != NULL &&
              ctk_drag_check_threshold (widget,
                                        priv->start_x,
                                        priv->start_y,
                                        x, y))
        {
          icon_info->in_drag = TRUE;
          ctk_drag_begin_with_coordinates (widget,
                                           icon_info->target_list,
                                           icon_info->actions,
                                           1,
                                           event,
                                           priv->start_x,
                                           priv->start_y);
        }

      break;
    case GDK_TOUCH_END:
      if (icon_info->device != device ||
          icon_info->current_sequence != sequence)
        break;

      icon_info->current_sequence = NULL;
      /* Fall through */
    case GDK_BUTTON_RELEASE:
      icon_info->pressed = FALSE;
      icon_info->device = NULL;

      if (should_prelight (CTK_ENTRY (widget), i) &&
          x >= 0 && y >= 0 &&
          x < gdk_window_get_width (icon_info->window) &&
          y < gdk_window_get_height (icon_info->window))
        {
          icon_info->prelight = TRUE;
          update_icon_state (widget, i);
          ctk_widget_queue_draw (widget);
        }

      if (!icon_info->nonactivatable)
        g_signal_emit (widget, signals[ICON_RELEASE], 0, i, event);

      break;
    default:
      return GDK_EVENT_PROPAGATE;
    }

  return GDK_EVENT_STOP;
}

static void
gesture_get_current_point_in_layout (CtkGestureSingle *gesture,
                                     CtkEntry         *entry,
                                     gint             *x,
                                     gint             *y)
{
  gint tx, ty;
  GdkEventSequence *sequence;
  gdouble px, py;

  sequence = ctk_gesture_single_get_current_sequence (gesture);
  ctk_gesture_get_point (CTK_GESTURE (gesture), sequence, &px, &py);
  ctk_entry_get_layout_offsets (entry, &tx, &ty);

  if (x)
    *x = px - tx;
  if (y)
    *y = py - ty;
}

static void
ctk_entry_multipress_gesture_pressed (CtkGestureMultiPress *gesture,
                                      gint                  n_press,
                                      gdouble               widget_x,
                                      gdouble               widget_y,
                                      CtkEntry             *entry)
{
  CtkEditable *editable = CTK_EDITABLE (entry);
  CtkWidget *widget = CTK_WIDGET (entry);
  CtkEntryPrivate *priv = entry->priv;
  GdkEventSequence *current;
  const GdkEvent *event;
  gint x, y, sel_start, sel_end;
  guint button;
  gint tmp_pos;

  button = ctk_gesture_single_get_current_button (CTK_GESTURE_SINGLE (gesture));
  current = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), current);

  ctk_gesture_set_sequence_state (CTK_GESTURE (gesture), current,
                                  CTK_EVENT_SEQUENCE_CLAIMED);
  gesture_get_current_point_in_layout (CTK_GESTURE_SINGLE (gesture), entry, &x, &y);
  ctk_entry_reset_blink_time (entry);

  if (!ctk_widget_has_focus (widget))
    {
      priv->in_click = TRUE;
      ctk_widget_grab_focus (widget);
      priv->in_click = FALSE;
    }

  tmp_pos = ctk_entry_find_position (entry, x);

  if (gdk_event_triggers_context_menu (event))
    {
      ctk_entry_do_popup (entry, event);
    }
  else if (n_press == 1 && button == GDK_BUTTON_MIDDLE &&
           get_middle_click_paste (entry))
    {
      if (priv->editable)
        {
          priv->insert_pos = tmp_pos;
          ctk_entry_paste (entry, GDK_SELECTION_PRIMARY);
        }
      else
        {
          ctk_widget_error_bell (widget);
        }
    }
  else if (button == GDK_BUTTON_PRIMARY)
    {
      gboolean have_selection = ctk_editable_get_selection_bounds (editable, &sel_start, &sel_end);
      CtkTextHandleMode mode;
      gboolean is_touchscreen, extend_selection;
      GdkDevice *source;

      source = gdk_event_get_source_device (event);
      is_touchscreen = ctk_simulate_touchscreen () ||
                       gdk_device_get_source (source) == GDK_SOURCE_TOUCHSCREEN;

      if (!is_touchscreen)
        mode = CTK_TEXT_HANDLE_MODE_NONE;
      else if (have_selection)
        mode = CTK_TEXT_HANDLE_MODE_SELECTION;
      else
        mode = CTK_TEXT_HANDLE_MODE_CURSOR;

      if (is_touchscreen)
        ctk_entry_ensure_text_handles (entry);

      priv->in_drag = FALSE;
      priv->select_words = FALSE;
      priv->select_lines = FALSE;

      extend_selection =
        (event->button.state &
         ctk_widget_get_modifier_mask (widget,
                                       GDK_MODIFIER_INTENT_EXTEND_SELECTION));

      if (extend_selection)
        ctk_entry_reset_im_context (entry);

      switch (n_press)
        {
        case 1:
          if (in_selection (entry, x))
            {
              if (is_touchscreen)
                {
                  if (entry->priv->selection_bubble &&
                      ctk_widget_get_visible (entry->priv->selection_bubble))
                    ctk_entry_selection_bubble_popup_unset (entry);
                  else
                    ctk_entry_selection_bubble_popup_set (entry);
                }
              else if (extend_selection)
                {
                  /* Truncate current selection, but keep it as big as possible */
                  if (tmp_pos - sel_start > sel_end - tmp_pos)
                    ctk_entry_set_positions (entry, sel_start, tmp_pos);
                  else
                    ctk_entry_set_positions (entry, tmp_pos, sel_end);

                  /* all done, so skip the extend_to_left stuff later */
                  extend_selection = FALSE;
                }
              else
                {
                  /* We'll either start a drag, or clear the selection */
                  priv->in_drag = TRUE;
                  priv->drag_start_x = x;
                  priv->drag_start_y = y;
                }
            }
          else
            {
              ctk_entry_selection_bubble_popup_unset (entry);

              if (!extend_selection)
                {
                  ctk_editable_set_position (editable, tmp_pos);
                  priv->handle_place_time = g_get_monotonic_time ();
                }
              else
                {
                  /* select from the current position to the clicked position */
                  if (!have_selection)
                    sel_start = sel_end = priv->current_pos;

                  ctk_entry_set_positions (entry, tmp_pos, tmp_pos);
                }
            }

          break;

        case 2:
          priv->select_words = TRUE;
          ctk_entry_select_word (entry);
          if (is_touchscreen)
            mode = CTK_TEXT_HANDLE_MODE_SELECTION;
          break;

        case 3:
          priv->select_lines = TRUE;
          ctk_entry_select_line (entry);
          if (is_touchscreen)
            mode = CTK_TEXT_HANDLE_MODE_SELECTION;
          break;

        default:
          break;
        }

      if (extend_selection)
        {
          gboolean extend_to_left;
          gint start, end;

          start = MIN (priv->current_pos, priv->selection_bound);
          start = MIN (sel_start, start);

          end = MAX (priv->current_pos, priv->selection_bound);
          end = MAX (sel_end, end);

          if (tmp_pos == sel_start || tmp_pos == sel_end)
            extend_to_left = (tmp_pos == start);
          else
            extend_to_left = (end == sel_end);

          if (extend_to_left)
            ctk_entry_set_positions (entry, start, end);
          else
            ctk_entry_set_positions (entry, end, start);
        }

      ctk_gesture_set_state (priv->drag_gesture,
                             CTK_EVENT_SEQUENCE_CLAIMED);

      if (priv->text_handle)
        ctk_entry_update_handles (entry, mode);
    }

  if (n_press >= 3)
    ctk_event_controller_reset (CTK_EVENT_CONTROLLER (gesture));
}

static gchar *
_ctk_entry_get_selected_text (CtkEntry *entry)
{
  CtkEditable *editable = CTK_EDITABLE (entry);
  gint         start_text, end_text;
  gchar       *text = NULL;

  if (ctk_editable_get_selection_bounds (editable, &start_text, &end_text))
    text = ctk_editable_get_chars (editable, start_text, end_text);

  return text;
}

static void
ctk_entry_show_magnifier (CtkEntry *entry,
                          gint      x,
                          gint      y)
{
  CtkAllocation allocation;
  cairo_rectangle_int_t rect;
  CtkEntryPrivate *priv;

  ctk_entry_ensure_magnifier (entry);

  ctk_widget_get_allocation (CTK_WIDGET (entry), &allocation);

  priv = entry->priv;
  rect.x = x + priv->text_allocation.x - allocation.x;
  rect.width = 1;
  rect.y = priv->text_allocation.y - allocation.y;
  rect.height = priv->text_allocation.height;

  _ctk_magnifier_set_coords (CTK_MAGNIFIER (priv->magnifier), rect.x,
                             rect.y + rect.height / 2);
  rect.x = CLAMP (rect.x, 0, allocation.width);
  ctk_popover_set_pointing_to (CTK_POPOVER (priv->magnifier_popover),
                               &rect);
  ctk_popover_popup (CTK_POPOVER (priv->magnifier_popover));
}

static void
ctk_entry_drag_gesture_update (CtkGestureDrag *gesture,
                               gdouble         offset_x,
                               gdouble         offset_y,
                               CtkEntry       *entry)
{
  CtkWidget *widget = CTK_WIDGET (entry);
  CtkEntryPrivate *priv = entry->priv;
  GdkEventSequence *sequence;
  const GdkEvent *event;
  gint x, y;

  ctk_entry_selection_bubble_popup_unset (entry);

  gesture_get_current_point_in_layout (CTK_GESTURE_SINGLE (gesture), entry, &x, &y);
  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);

  if (priv->mouse_cursor_obscured)
    {
      GdkCursor *cursor;

      cursor = gdk_cursor_new_from_name (ctk_widget_get_display (widget), "text");
      gdk_window_set_cursor (priv->text_area, cursor);
      g_object_unref (cursor);
      priv->mouse_cursor_obscured = FALSE;
    }

  if (priv->select_lines)
    return;

  if (priv->in_drag)
    {
      if (ctk_entry_get_display_mode (entry) == DISPLAY_NORMAL &&
          ctk_drag_check_threshold (widget,
                                    priv->drag_start_x, priv->drag_start_y,
                                    x, y))
        {
          gint *ranges;
          gint n_ranges;
          CtkTargetList  *target_list = ctk_target_list_new (NULL, 0);
          guint actions = priv->editable ? GDK_ACTION_COPY | GDK_ACTION_MOVE : GDK_ACTION_COPY;
          guint button;

          ctk_target_list_add_text_targets (target_list, 0);

          ctk_entry_get_pixel_ranges (entry, &ranges, &n_ranges);

          button = ctk_gesture_single_get_current_button (CTK_GESTURE_SINGLE (gesture));
          ctk_drag_begin_with_coordinates (widget, target_list, actions,
                                           button, (GdkEvent*) event,
                                           priv->drag_start_x + ranges[0],
                                           priv->drag_start_y);
          g_free (ranges);

          priv->in_drag = FALSE;

          ctk_target_list_unref (target_list);
        }
    }
  else
    {
      GdkInputSource input_source;
      GdkDevice *source;
      guint length;
      gint tmp_pos;

      length = ctk_entry_buffer_get_length (get_buffer (entry));

      if (y < 0)
	tmp_pos = 0;
      else if (y >= gdk_window_get_height (priv->text_area))
	tmp_pos = length;
      else
	tmp_pos = ctk_entry_find_position (entry, x);

      source = gdk_event_get_source_device (event);
      input_source = gdk_device_get_source (source);

      if (priv->select_words)
	{
	  gint min, max;
	  gint old_min, old_max;
	  gint pos, bound;

	  min = ctk_entry_move_backward_word (entry, tmp_pos, TRUE);
	  max = ctk_entry_move_forward_word (entry, tmp_pos, TRUE);

	  pos = priv->current_pos;
	  bound = priv->selection_bound;

	  old_min = MIN (priv->current_pos, priv->selection_bound);
	  old_max = MAX (priv->current_pos, priv->selection_bound);

	  if (min < old_min)
	    {
	      pos = min;
	      bound = old_max;
	    }
	  else if (old_max < max)
	    {
	      pos = max;
	      bound = old_min;
	    }
	  else if (pos == old_min)
	    {
	      if (priv->current_pos != min)
		pos = max;
	    }
	  else
	    {
	      if (priv->current_pos != max)
		pos = min;
	    }

	  ctk_entry_set_positions (entry, pos, bound);
	}
      else
        ctk_entry_set_positions (entry, tmp_pos, -1);

      /* Update touch handles' position */
      if (ctk_simulate_touchscreen () ||
          input_source == GDK_SOURCE_TOUCHSCREEN)
        {
          ctk_entry_ensure_text_handles (entry);
          ctk_entry_update_handles (entry,
                                    (priv->current_pos == priv->selection_bound) ?
                                    CTK_TEXT_HANDLE_MODE_CURSOR :
                                    CTK_TEXT_HANDLE_MODE_SELECTION);
          ctk_entry_show_magnifier (entry, x - priv->scroll_offset, y);
        }
    }
}

static void
ctk_entry_drag_gesture_end (CtkGestureDrag *gesture,
                            gdouble         offset_x,
                            gdouble         offset_y,
                            CtkEntry       *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  gboolean in_drag, is_touchscreen;
  GdkEventSequence *sequence;
  const GdkEvent *event;
  GdkDevice *source;

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  in_drag = priv->in_drag;
  priv->in_drag = FALSE;

  if (priv->magnifier_popover)
    ctk_popover_popdown (CTK_POPOVER (priv->magnifier_popover));

  /* Check whether the drag was cancelled rather than finished */
  if (!ctk_gesture_handles_sequence (CTK_GESTURE (gesture), sequence))
    return;

  event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);
  source = gdk_event_get_source_device (event);
  is_touchscreen = ctk_simulate_touchscreen () ||
                   gdk_device_get_source (source) == GDK_SOURCE_TOUCHSCREEN;

  if (in_drag)
    {
      gint tmp_pos = ctk_entry_find_position (entry, priv->drag_start_x);

      ctk_editable_set_position (CTK_EDITABLE (entry), tmp_pos);
    }

  if (is_touchscreen &&
      !ctk_editable_get_selection_bounds (CTK_EDITABLE (entry), NULL, NULL))
    ctk_entry_update_handles (entry, CTK_TEXT_HANDLE_MODE_CURSOR);

  ctk_entry_update_primary_selection (entry);
}

static void
set_invisible_cursor (GdkWindow *window)
{
  GdkCursor *cursor;

  cursor = gdk_cursor_new_from_name (gdk_window_get_display (window), "none");
  gdk_window_set_cursor (window, cursor);
  g_object_unref (cursor);
}

static void
ctk_entry_obscure_mouse_cursor (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->mouse_cursor_obscured)
    return;

  if (priv->text_area)
    {
      set_invisible_cursor (priv->text_area);
      priv->mouse_cursor_obscured = TRUE;
    }
}

static gint
ctk_entry_key_press (CtkWidget   *widget,
		     GdkEventKey *event)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  gboolean retval = FALSE;

  priv->handling_key_event = TRUE;

  ctk_entry_reset_blink_time (entry);
  ctk_entry_pend_cursor_blink (entry);

  ctk_entry_selection_bubble_popup_unset (entry);

  if (!event->send_event && priv->text_handle)
    _ctk_text_handle_set_mode (priv->text_handle,
                               CTK_TEXT_HANDLE_MODE_NONE);

  if (priv->editable)
    {
      if (ctk_im_context_filter_keypress (priv->im_context, event))
	{
	  priv->need_im_reset = TRUE;
	  retval = TRUE;
          goto out;
	}
    }

  if (event->keyval == GDK_KEY_Return ||
      event->keyval == GDK_KEY_KP_Enter ||
      event->keyval == GDK_KEY_ISO_Enter ||
      event->keyval == GDK_KEY_Escape)
    ctk_entry_reset_im_context (entry);

  if (CTK_WIDGET_CLASS (ctk_entry_parent_class)->key_press_event (widget, event))
    {
      /* Activate key bindings */
      retval = TRUE;
      goto out;
    }

  if (!priv->editable && event->length)
    ctk_widget_error_bell (widget);

out:
  priv->handling_key_event = FALSE;

  return retval;
}

static gint
ctk_entry_key_release (CtkWidget   *widget,
		       GdkEventKey *event)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  gboolean retval = FALSE;

  priv->handling_key_event = TRUE;

  if (priv->editable)
    {
      if (ctk_im_context_filter_keypress (priv->im_context, event))
	{
	  priv->need_im_reset = TRUE;
	  retval = TRUE;
          goto out;
	}
    }

  retval = CTK_WIDGET_CLASS (ctk_entry_parent_class)->key_release_event (widget, event);

out:
  priv->handling_key_event = FALSE;
  return retval;
}

static gint
ctk_entry_focus_in (CtkWidget     *widget,
		    GdkEventFocus *event)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  GdkKeymap *keymap;

  ctk_widget_queue_draw (widget);

  keymap = gdk_keymap_get_for_display (ctk_widget_get_display (widget));

  if (priv->editable)
    {
      priv->need_im_reset = TRUE;
      ctk_im_context_focus_in (priv->im_context);
      keymap_state_changed (keymap, entry);
      g_signal_connect (keymap, "state-changed", 
                        G_CALLBACK (keymap_state_changed), entry);
    }

  g_signal_connect (keymap, "direction-changed",
		    G_CALLBACK (keymap_direction_changed), entry);

  if (ctk_entry_buffer_get_bytes (get_buffer (entry)) == 0 &&
      priv->placeholder_text != NULL)
    {
      ctk_entry_recompute (entry);
    }
  else
    {
      ctk_entry_reset_blink_time (entry);
      ctk_entry_check_cursor_blink (entry);
    }

  return GDK_EVENT_PROPAGATE;
}

static gint
ctk_entry_focus_out (CtkWidget     *widget,
		     GdkEventFocus *event)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  CtkEntryCompletion *completion;
  GdkKeymap *keymap;

  ctk_entry_selection_bubble_popup_unset (entry);

  if (priv->text_handle)
    _ctk_text_handle_set_mode (priv->text_handle,
                               CTK_TEXT_HANDLE_MODE_NONE);

  ctk_widget_queue_draw (widget);

  keymap = gdk_keymap_get_for_display (ctk_widget_get_display (widget));

  if (priv->editable)
    {
      priv->need_im_reset = TRUE;
      ctk_im_context_focus_out (priv->im_context);
      remove_capslock_feedback (entry);
    }

  if (ctk_entry_buffer_get_bytes (get_buffer (entry)) == 0 &&
      priv->placeholder_text != NULL)
    {
      ctk_entry_recompute (entry);
    }
  else
    {
      ctk_entry_check_cursor_blink (entry);
    }

  g_signal_handlers_disconnect_by_func (keymap, keymap_state_changed, entry);
  g_signal_handlers_disconnect_by_func (keymap, keymap_direction_changed, entry);

  completion = ctk_entry_get_completion (entry);
  if (completion)
    _ctk_entry_completion_popdown (completion);

  return GDK_EVENT_PROPAGATE;
}

void
_ctk_entry_grab_focus (CtkEntry  *entry,
                       gboolean   select_all)
{
  if (!ctk_widget_get_can_focus (CTK_WIDGET (entry)))
    return;

  if (!ctk_widget_is_sensitive (CTK_WIDGET (entry)))
    return;

  CTK_WIDGET_CLASS (ctk_entry_parent_class)->grab_focus (CTK_WIDGET (entry));

  if (select_all)
    ctk_editable_select_region (CTK_EDITABLE (entry), 0, -1);
}

static void
ctk_entry_grab_focus (CtkWidget *widget)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  gboolean select_on_focus;

  if (priv->editable && !priv->in_click)
    {
      g_object_get (ctk_widget_get_settings (widget),
                    "ctk-entry-select-on-focus",
                    &select_on_focus,
                    NULL);

      _ctk_entry_grab_focus (entry, select_on_focus);
    }
  else
    {
      _ctk_entry_grab_focus (entry, FALSE);
    }
}

/**
 * ctk_entry_grab_focus_without_selecting:
 * @entry: a #CtkEntry
 *
 * Causes @entry to have keyboard focus.
 *
 * It behaves like ctk_widget_grab_focus(),
 * except that it doesn't select the contents of the entry.
 * You only want to call this on some special entries
 * which the user usually doesn't want to replace all text in,
 * such as search-as-you-type entries.
 *
 * Since: 3.16
 */
void
ctk_entry_grab_focus_without_selecting (CtkEntry *entry)
{
  g_return_if_fail (CTK_IS_ENTRY (entry));

  _ctk_entry_grab_focus (entry, FALSE);
}

static void
ctk_entry_direction_changed (CtkWidget        *widget,
                             CtkTextDirection  previous_dir)
{
  CtkEntry *entry = CTK_ENTRY (widget);

  ctk_entry_recompute (entry);

  update_icon_style (widget, CTK_ENTRY_ICON_PRIMARY);
  update_icon_style (widget, CTK_ENTRY_ICON_SECONDARY);

  update_node_ordering (entry);

  CTK_WIDGET_CLASS (ctk_entry_parent_class)->direction_changed (widget, previous_dir);
}

static void
ctk_entry_state_flags_changed (CtkWidget     *widget,
                               CtkStateFlags  previous_state)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  GdkCursor *cursor;

  if (ctk_widget_get_realized (widget))
    {
      if (ctk_widget_is_sensitive (widget))
        cursor = gdk_cursor_new_from_name (ctk_widget_get_display (widget), "text");
      else
        cursor = NULL;

      gdk_window_set_cursor (priv->text_area, cursor);

      if (cursor)
        g_object_unref (cursor);

      priv->mouse_cursor_obscured = FALSE;

      update_cursors (widget);
    }

  if (!ctk_widget_is_sensitive (widget))
    {
      /* Clear any selection */
      ctk_editable_select_region (CTK_EDITABLE (entry), priv->current_pos, priv->current_pos);
    }

  update_node_state (entry);
  update_icon_state (widget, CTK_ENTRY_ICON_PRIMARY);
  update_icon_state (widget, CTK_ENTRY_ICON_SECONDARY);

  ctk_entry_update_cached_style_values (entry);
}

static void
ctk_entry_screen_changed (CtkWidget *widget,
			  GdkScreen *old_screen)
{
  ctk_entry_recompute (CTK_ENTRY (widget));
}

/* CtkEditable method implementations
 */
static void
ctk_entry_insert_text (CtkEditable *editable,
		       const gchar *new_text,
		       gint         new_text_length,
		       gint        *position)
{
  g_object_ref (editable);

  /*
   * The incoming text may a password or other secret. We make sure
   * not to copy it into temporary buffers.
   */

  g_signal_emit_by_name (editable, "insert-text", new_text, new_text_length, position);

  g_object_unref (editable);
}

static void
ctk_entry_delete_text (CtkEditable *editable,
		       gint         start_pos,
		       gint         end_pos)
{
  g_object_ref (editable);

  g_signal_emit_by_name (editable, "delete-text", start_pos, end_pos);

  g_object_unref (editable);
}

static gchar *    
ctk_entry_get_chars      (CtkEditable   *editable,
			  gint           start_pos,
			  gint           end_pos)
{
  CtkEntry *entry = CTK_ENTRY (editable);
  const gchar *text;
  gint text_length;
  gint start_index, end_index;

  text = ctk_entry_buffer_get_text (get_buffer (entry));
  text_length = ctk_entry_buffer_get_length (get_buffer (entry));

  if (end_pos < 0)
    end_pos = text_length;

  start_pos = MIN (text_length, start_pos);
  end_pos = MIN (text_length, end_pos);

  start_index = g_utf8_offset_to_pointer (text, start_pos) - text;
  end_index = g_utf8_offset_to_pointer (text, end_pos) - text;

  return g_strndup (text + start_index, end_index - start_index);
}

static void
ctk_entry_real_set_position (CtkEditable *editable,
			     gint         position)
{
  CtkEntry *entry = CTK_ENTRY (editable);
  CtkEntryPrivate *priv = entry->priv;

  guint length;

  length = ctk_entry_buffer_get_length (get_buffer (entry));
  if (position < 0 || position > length)
    position = length;

  if (position != priv->current_pos ||
      position != priv->selection_bound)
    {
      ctk_entry_reset_im_context (entry);
      ctk_entry_set_positions (entry, position, position);
    }
}

static gint
ctk_entry_get_position (CtkEditable *editable)
{
  CtkEntry *entry = CTK_ENTRY (editable);
  CtkEntryPrivate *priv = entry->priv;

  return priv->current_pos;
}

static void
ctk_entry_set_selection_bounds (CtkEditable *editable,
				gint         start,
				gint         end)
{
  CtkEntry *entry = CTK_ENTRY (editable);
  guint length;

  length = ctk_entry_buffer_get_length (get_buffer (entry));
  if (start < 0)
    start = length;
  if (end < 0)
    end = length;

  ctk_entry_reset_im_context (entry);

  ctk_entry_set_positions (entry,
			   MIN (end, length),
			   MIN (start, length));

  ctk_entry_update_primary_selection (entry);
}

static gboolean
ctk_entry_get_selection_bounds (CtkEditable *editable,
				gint        *start,
				gint        *end)
{
  CtkEntry *entry = CTK_ENTRY (editable);
  CtkEntryPrivate *priv = entry->priv;

  *start = priv->selection_bound;
  *end = priv->current_pos;

  return (priv->selection_bound != priv->current_pos);
}

static void
ctk_entry_update_cached_style_values (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (!priv->invisible_char_set)
    {
      gunichar ch = find_invisible_char (CTK_WIDGET (entry));

      if (priv->invisible_char != ch)
        {
          priv->invisible_char = ch;
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_INVISIBLE_CHAR]);
        }
    }
}

static void 
ctk_entry_style_updated (CtkWidget *widget)
{
  CtkEntry *entry = CTK_ENTRY (widget);

  CTK_WIDGET_CLASS (ctk_entry_parent_class)->style_updated (widget);

  ctk_entry_update_cached_style_values (entry);
}

/* CtkCellEditable method implementations
 */
static void
ctk_cell_editable_entry_activated (CtkEntry *entry, gpointer data)
{
  ctk_cell_editable_editing_done (CTK_CELL_EDITABLE (entry));
  ctk_cell_editable_remove_widget (CTK_CELL_EDITABLE (entry));
}

static gboolean
ctk_cell_editable_key_press_event (CtkEntry    *entry,
				   GdkEventKey *key_event,
				   gpointer     data)
{
  CtkEntryPrivate *priv = entry->priv;

  if (key_event->keyval == GDK_KEY_Escape)
    {
      priv->editing_canceled = TRUE;
      ctk_cell_editable_editing_done (CTK_CELL_EDITABLE (entry));
      ctk_cell_editable_remove_widget (CTK_CELL_EDITABLE (entry));

      return GDK_EVENT_STOP;
    }

  /* override focus */
  if (key_event->keyval == GDK_KEY_Up || key_event->keyval == GDK_KEY_Down)
    {
      ctk_cell_editable_editing_done (CTK_CELL_EDITABLE (entry));
      ctk_cell_editable_remove_widget (CTK_CELL_EDITABLE (entry));

      return GDK_EVENT_STOP;
    }

  return GDK_EVENT_PROPAGATE;
}

static void
ctk_entry_start_editing (CtkCellEditable *cell_editable,
			 GdkEvent        *event)
{
  g_signal_connect (cell_editable, "activate",
		    G_CALLBACK (ctk_cell_editable_entry_activated), NULL);
  g_signal_connect (cell_editable, "key-press-event",
		    G_CALLBACK (ctk_cell_editable_key_press_event), NULL);
}

static void
ctk_entry_password_hint_free (CtkEntryPasswordHint *password_hint)
{
  if (password_hint->source_id)
    g_source_remove (password_hint->source_id);

  g_slice_free (CtkEntryPasswordHint, password_hint);
}


static gboolean
ctk_entry_remove_password_hint (gpointer data)
{
  CtkEntryPasswordHint *password_hint = g_object_get_qdata (data, quark_password_hint);
  password_hint->position = -1;

  /* Force the string to be redrawn, but now without a visible character */
  ctk_entry_recompute (CTK_ENTRY (data));
  return G_SOURCE_REMOVE;
}

/* Default signal handlers
 */
static void
ctk_entry_real_insert_text (CtkEditable *editable,
			    const gchar *new_text,
			    gint         new_text_length,
			    gint        *position)
{
  guint n_inserted;
  gint n_chars;

  n_chars = g_utf8_strlen (new_text, new_text_length);

  /*
   * The actual insertion into the buffer. This will end up firing the
   * following signal handlers: buffer_inserted_text(), buffer_notify_display_text(),
   * buffer_notify_text(), buffer_notify_length()
   */
  begin_change (CTK_ENTRY (editable));

  n_inserted = ctk_entry_buffer_insert_text (get_buffer (CTK_ENTRY (editable)), *position, new_text, n_chars);

  end_change (CTK_ENTRY (editable));

  if (n_inserted != n_chars)
      ctk_widget_error_bell (CTK_WIDGET (editable));

  *position += n_inserted;
}

static void
ctk_entry_real_delete_text (CtkEditable *editable,
                            gint         start_pos,
                            gint         end_pos)
{
  /*
   * The actual deletion from the buffer. This will end up firing the
   * following signal handlers: buffer_deleted_text(), buffer_notify_display_text(),
   * buffer_notify_text(), buffer_notify_length()
   */

  begin_change (CTK_ENTRY (editable));

  ctk_entry_buffer_delete_text (get_buffer (CTK_ENTRY (editable)), start_pos, end_pos - start_pos);

  end_change (CTK_ENTRY (editable));
}

/* CtkEntryBuffer signal handlers
 */
static void
buffer_inserted_text (CtkEntryBuffer *buffer,
                      guint           position,
                      const gchar    *chars,
                      guint           n_chars,
                      CtkEntry       *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  guint password_hint_timeout;
  guint current_pos;
  gint selection_bound;

  current_pos = priv->current_pos;
  if (current_pos > position)
    current_pos += n_chars;

  selection_bound = priv->selection_bound;
  if (selection_bound > position)
    selection_bound += n_chars;

  ctk_entry_set_positions (entry, current_pos, selection_bound);
  ctk_entry_recompute (entry);

  /* Calculate the password hint if it needs to be displayed. */
  if (n_chars == 1 && !priv->visible)
    {
      g_object_get (ctk_widget_get_settings (CTK_WIDGET (entry)),
                    "ctk-entry-password-hint-timeout", &password_hint_timeout,
                    NULL);

      if (password_hint_timeout > 0)
        {
          CtkEntryPasswordHint *password_hint = g_object_get_qdata (G_OBJECT (entry),
                                                                    quark_password_hint);
          if (!password_hint)
            {
              password_hint = g_slice_new0 (CtkEntryPasswordHint);
              g_object_set_qdata_full (G_OBJECT (entry), quark_password_hint, password_hint,
                                       (GDestroyNotify)ctk_entry_password_hint_free);
            }

          password_hint->position = position;
          if (password_hint->source_id)
            g_source_remove (password_hint->source_id);
          password_hint->source_id = gdk_threads_add_timeout (password_hint_timeout,
                                                              (GSourceFunc)ctk_entry_remove_password_hint, entry);
          g_source_set_name_by_id (password_hint->source_id, "[ctk+] ctk_entry_remove_password_hint");
        }
    }
}

static void
buffer_deleted_text (CtkEntryBuffer *buffer,
                     guint           position,
                     guint           n_chars,
                     CtkEntry       *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  guint end_pos = position + n_chars;
  gint selection_bound;
  guint current_pos;

  current_pos = priv->current_pos;
  if (current_pos > position)
    current_pos -= MIN (current_pos, end_pos) - position;

  selection_bound = priv->selection_bound;
  if (selection_bound > position)
    selection_bound -= MIN (selection_bound, end_pos) - position;

  ctk_entry_set_positions (entry, current_pos, selection_bound);
  ctk_entry_recompute (entry);

  /* We might have deleted the selection */
  ctk_entry_update_primary_selection (entry);

  /* Disable the password hint if one exists. */
  if (!priv->visible)
    {
      CtkEntryPasswordHint *password_hint = g_object_get_qdata (G_OBJECT (entry),
                                                                quark_password_hint);
      if (password_hint)
        {
          if (password_hint->source_id)
            g_source_remove (password_hint->source_id);
          password_hint->source_id = 0;
          password_hint->position = -1;
        }
    }
}

static void
buffer_notify_text (CtkEntryBuffer *buffer,
                    GParamSpec     *spec,
                    CtkEntry       *entry)
{
  if (entry->priv->handling_key_event)
    ctk_entry_obscure_mouse_cursor (entry);

  emit_changed (entry);
  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_TEXT]);
}

static void
buffer_notify_length (CtkEntryBuffer *buffer,
                      GParamSpec     *spec,
                      CtkEntry       *entry)
{
  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_TEXT_LENGTH]);
}

static void
buffer_notify_max_length (CtkEntryBuffer *buffer,
                          GParamSpec     *spec,
                          CtkEntry       *entry)
{
  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_MAX_LENGTH]);
}

static void
buffer_connect_signals (CtkEntry *entry)
{
  g_signal_connect (get_buffer (entry), "inserted-text", G_CALLBACK (buffer_inserted_text), entry);
  g_signal_connect (get_buffer (entry), "deleted-text", G_CALLBACK (buffer_deleted_text), entry);
  g_signal_connect (get_buffer (entry), "notify::text", G_CALLBACK (buffer_notify_text), entry);
  g_signal_connect (get_buffer (entry), "notify::length", G_CALLBACK (buffer_notify_length), entry);
  g_signal_connect (get_buffer (entry), "notify::max-length", G_CALLBACK (buffer_notify_max_length), entry);
}

static void
buffer_disconnect_signals (CtkEntry *entry)
{
  g_signal_handlers_disconnect_by_func (get_buffer (entry), buffer_inserted_text, entry);
  g_signal_handlers_disconnect_by_func (get_buffer (entry), buffer_deleted_text, entry);
  g_signal_handlers_disconnect_by_func (get_buffer (entry), buffer_notify_text, entry);
  g_signal_handlers_disconnect_by_func (get_buffer (entry), buffer_notify_length, entry);
  g_signal_handlers_disconnect_by_func (get_buffer (entry), buffer_notify_max_length, entry);
}

/* Compute the X position for an offset that corresponds to the "more important
 * cursor position for that offset. We use this when trying to guess to which
 * end of the selection we should go to when the user hits the left or
 * right arrow key.
 */
static gint
get_better_cursor_x (CtkEntry *entry,
		     gint      offset)
{
  CtkEntryPrivate *priv = entry->priv;
  GdkKeymap *keymap = gdk_keymap_get_for_display (ctk_widget_get_display (CTK_WIDGET (entry)));
  PangoDirection keymap_direction = gdk_keymap_get_direction (keymap);
  gboolean split_cursor;
  
  PangoLayout *layout = ctk_entry_ensure_layout (entry, TRUE);
  const gchar *text = pango_layout_get_text (layout);
  gint index = g_utf8_offset_to_pointer (text, offset) - text;
  
  PangoRectangle strong_pos, weak_pos;
  
  g_object_get (ctk_widget_get_settings (CTK_WIDGET (entry)),
		"ctk-split-cursor", &split_cursor,
		NULL);

  pango_layout_get_cursor_pos (layout, index, &strong_pos, &weak_pos);

  if (split_cursor)
    return strong_pos.x / PANGO_SCALE;
  else
    return (keymap_direction == priv->resolved_dir) ? strong_pos.x / PANGO_SCALE : weak_pos.x / PANGO_SCALE;
}

static void
ctk_entry_move_cursor (CtkEntry       *entry,
		       CtkMovementStep step,
		       gint            count,
		       gboolean        extend_selection)
{
  CtkEntryPrivate *priv = entry->priv;
  gint new_pos = priv->current_pos;

  ctk_entry_reset_im_context (entry);

  if (priv->current_pos != priv->selection_bound && !extend_selection)
    {
      /* If we have a current selection and aren't extending it, move to the
       * start/or end of the selection as appropriate
       */
      switch (step)
	{
	case CTK_MOVEMENT_VISUAL_POSITIONS:
	  {
	    gint current_x = get_better_cursor_x (entry, priv->current_pos);
	    gint bound_x = get_better_cursor_x (entry, priv->selection_bound);

	    if (count <= 0)
	      new_pos = current_x < bound_x ? priv->current_pos : priv->selection_bound;
	    else 
	      new_pos = current_x > bound_x ? priv->current_pos : priv->selection_bound;
	  }
	  break;

	case CTK_MOVEMENT_WORDS:
          if (priv->resolved_dir == PANGO_DIRECTION_RTL)
            count *= -1;
          /* Fall through */

	case CTK_MOVEMENT_LOGICAL_POSITIONS:
	  if (count < 0)
	    new_pos = MIN (priv->current_pos, priv->selection_bound);
	  else
	    new_pos = MAX (priv->current_pos, priv->selection_bound);

	  break;

	case CTK_MOVEMENT_DISPLAY_LINE_ENDS:
	case CTK_MOVEMENT_PARAGRAPH_ENDS:
	case CTK_MOVEMENT_BUFFER_ENDS:
	  new_pos = count < 0 ? 0 : ctk_entry_buffer_get_length (get_buffer (entry));
	  break;

	case CTK_MOVEMENT_DISPLAY_LINES:
	case CTK_MOVEMENT_PARAGRAPHS:
	case CTK_MOVEMENT_PAGES:
	case CTK_MOVEMENT_HORIZONTAL_PAGES:
	  break;
	}
    }
  else
    {
      switch (step)
	{
	case CTK_MOVEMENT_LOGICAL_POSITIONS:
	  new_pos = ctk_entry_move_logically (entry, new_pos, count);
	  break;

	case CTK_MOVEMENT_VISUAL_POSITIONS:
	  new_pos = ctk_entry_move_visually (entry, new_pos, count);

          if (priv->current_pos == new_pos)
            {
              if (!extend_selection)
                {
                  if (!ctk_widget_keynav_failed (CTK_WIDGET (entry),
                                                 count > 0 ?
                                                 CTK_DIR_RIGHT : CTK_DIR_LEFT))
                    {
                      CtkWidget *toplevel = ctk_widget_get_toplevel (CTK_WIDGET (entry));

                      if (toplevel)
                        ctk_widget_child_focus (toplevel,
                                                count > 0 ?
                                                CTK_DIR_RIGHT : CTK_DIR_LEFT);
                    }
                }
              else
                {
                  ctk_widget_error_bell (CTK_WIDGET (entry));
                }
            }
	  break;

	case CTK_MOVEMENT_WORDS:
          if (priv->resolved_dir == PANGO_DIRECTION_RTL)
            count *= -1;

	  while (count > 0)
	    {
	      new_pos = ctk_entry_move_forward_word (entry, new_pos, FALSE);
	      count--;
	    }

	  while (count < 0)
	    {
	      new_pos = ctk_entry_move_backward_word (entry, new_pos, FALSE);
	      count++;
	    }

          if (priv->current_pos == new_pos)
            ctk_widget_error_bell (CTK_WIDGET (entry));

	  break;

	case CTK_MOVEMENT_DISPLAY_LINE_ENDS:
	case CTK_MOVEMENT_PARAGRAPH_ENDS:
	case CTK_MOVEMENT_BUFFER_ENDS:
	  new_pos = count < 0 ? 0 : ctk_entry_buffer_get_length (get_buffer (entry));

          if (priv->current_pos == new_pos)
            ctk_widget_error_bell (CTK_WIDGET (entry));

	  break;

	case CTK_MOVEMENT_DISPLAY_LINES:
	case CTK_MOVEMENT_PARAGRAPHS:
	case CTK_MOVEMENT_PAGES:
	case CTK_MOVEMENT_HORIZONTAL_PAGES:
	  break;
	}
    }

  if (extend_selection)
    ctk_editable_select_region (CTK_EDITABLE (entry), priv->selection_bound, new_pos);
  else
    ctk_editable_set_position (CTK_EDITABLE (entry), new_pos);
  
  ctk_entry_pend_cursor_blink (entry);
}

static void
ctk_entry_insert_at_cursor (CtkEntry    *entry,
			    const gchar *str)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkEditable *editable = CTK_EDITABLE (entry);
  gint pos = priv->current_pos;

  if (priv->editable)
    {
      ctk_entry_reset_im_context (entry);
      ctk_editable_insert_text (editable, str, -1, &pos);
      ctk_editable_set_position (editable, pos);
    }
}

static void
ctk_entry_delete_from_cursor (CtkEntry       *entry,
			      CtkDeleteType   type,
			      gint            count)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkEditable *editable = CTK_EDITABLE (entry);
  gint start_pos = priv->current_pos;
  gint end_pos = priv->current_pos;
  gint old_n_bytes = ctk_entry_buffer_get_bytes (get_buffer (entry));

  ctk_entry_reset_im_context (entry);

  if (!priv->editable)
    {
      ctk_widget_error_bell (CTK_WIDGET (entry));
      return;
    }

  if (priv->selection_bound != priv->current_pos)
    {
      ctk_editable_delete_selection (editable);
      return;
    }
  
  switch (type)
    {
    case CTK_DELETE_CHARS:
      end_pos = ctk_entry_move_logically (entry, priv->current_pos, count);
      ctk_editable_delete_text (editable, MIN (start_pos, end_pos), MAX (start_pos, end_pos));
      break;

    case CTK_DELETE_WORDS:
      if (count < 0)
	{
	  /* Move to end of current word, or if not on a word, end of previous word */
	  end_pos = ctk_entry_move_backward_word (entry, end_pos, FALSE);
	  end_pos = ctk_entry_move_forward_word (entry, end_pos, FALSE);
	}
      else if (count > 0)
	{
	  /* Move to beginning of current word, or if not on a word, begining of next word */
	  start_pos = ctk_entry_move_forward_word (entry, start_pos, FALSE);
	  start_pos = ctk_entry_move_backward_word (entry, start_pos, FALSE);
	}
	
      /* Fall through */
    case CTK_DELETE_WORD_ENDS:
      while (count < 0)
	{
	  start_pos = ctk_entry_move_backward_word (entry, start_pos, FALSE);
	  count++;
	}

      while (count > 0)
	{
	  end_pos = ctk_entry_move_forward_word (entry, end_pos, FALSE);
	  count--;
	}

      ctk_editable_delete_text (editable, start_pos, end_pos);
      break;

    case CTK_DELETE_DISPLAY_LINE_ENDS:
    case CTK_DELETE_PARAGRAPH_ENDS:
      if (count < 0)
	ctk_editable_delete_text (editable, 0, priv->current_pos);
      else
	ctk_editable_delete_text (editable, priv->current_pos, -1);

      break;

    case CTK_DELETE_DISPLAY_LINES:
    case CTK_DELETE_PARAGRAPHS:
      ctk_editable_delete_text (editable, 0, -1);  
      break;

    case CTK_DELETE_WHITESPACE:
      ctk_entry_delete_whitespace (entry);
      break;
    }

  if (ctk_entry_buffer_get_bytes (get_buffer (entry)) == old_n_bytes)
    ctk_widget_error_bell (CTK_WIDGET (entry));

  ctk_entry_pend_cursor_blink (entry);
}

static void
ctk_entry_backspace (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkEditable *editable = CTK_EDITABLE (entry);
  gint prev_pos;

  ctk_entry_reset_im_context (entry);

  if (!priv->editable)
    {
      ctk_widget_error_bell (CTK_WIDGET (entry));
      return;
    }

  if (priv->selection_bound != priv->current_pos)
    {
      ctk_editable_delete_selection (editable);
      return;
    }

  prev_pos = ctk_entry_move_logically (entry, priv->current_pos, -1);

  if (prev_pos < priv->current_pos)
    {
      PangoLayout *layout = ctk_entry_ensure_layout (entry, FALSE);
      PangoLogAttr *log_attrs;
      gint n_attrs;

      pango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);

      /* Deleting parts of characters */
      if (log_attrs[priv->current_pos].backspace_deletes_character)
	{
	  gchar *cluster_text;
	  gchar *normalized_text;
          glong  len;

	  cluster_text = _ctk_entry_get_display_text (entry, prev_pos,
                                                      priv->current_pos);
	  normalized_text = g_utf8_normalize (cluster_text,
			  		      strlen (cluster_text),
					      G_NORMALIZE_NFD);
          len = g_utf8_strlen (normalized_text, -1);

          ctk_editable_delete_text (editable, prev_pos, priv->current_pos);
	  if (len > 1)
	    {
	      gint pos = priv->current_pos;

	      ctk_editable_insert_text (editable, normalized_text,
					g_utf8_offset_to_pointer (normalized_text, len - 1) - normalized_text,
					&pos);
	      ctk_editable_set_position (editable, pos);
	    }

	  g_free (normalized_text);
	  g_free (cluster_text);
	}
      else
	{
          ctk_editable_delete_text (editable, prev_pos, priv->current_pos);
	}
      
      g_free (log_attrs);
    }
  else
    {
      ctk_widget_error_bell (CTK_WIDGET (entry));
    }

  ctk_entry_pend_cursor_blink (entry);
}

static void
ctk_entry_copy_clipboard (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkEditable *editable = CTK_EDITABLE (entry);
  gint start, end;
  gchar *str;

  if (ctk_editable_get_selection_bounds (editable, &start, &end))
    {
      if (!priv->visible)
        {
          ctk_widget_error_bell (CTK_WIDGET (entry));
          return;
        }

      str = _ctk_entry_get_display_text (entry, start, end);
      ctk_clipboard_set_text (ctk_widget_get_clipboard (CTK_WIDGET (entry),
							GDK_SELECTION_CLIPBOARD),
			      str, -1);
      g_free (str);
    }
}

static void
ctk_entry_cut_clipboard (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkEditable *editable = CTK_EDITABLE (entry);
  gint start, end;

  if (!priv->visible)
    {
      ctk_widget_error_bell (CTK_WIDGET (entry));
      return;
    }

  ctk_entry_copy_clipboard (entry);

  if (priv->editable)
    {
      if (ctk_editable_get_selection_bounds (editable, &start, &end))
	ctk_editable_delete_text (editable, start, end);
    }
  else
    {
      ctk_widget_error_bell (CTK_WIDGET (entry));
    }

  ctk_entry_selection_bubble_popup_unset (entry);

  if (priv->text_handle)
    {
      CtkTextHandleMode handle_mode;

      handle_mode = _ctk_text_handle_get_mode (priv->text_handle);

      if (handle_mode != CTK_TEXT_HANDLE_MODE_NONE)
        ctk_entry_update_handles (entry, CTK_TEXT_HANDLE_MODE_CURSOR);
    }
}

static void
ctk_entry_paste_clipboard (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->editable)
    ctk_entry_paste (entry, GDK_SELECTION_CLIPBOARD);
  else
    ctk_widget_error_bell (CTK_WIDGET (entry));

  if (priv->text_handle)
    {
      CtkTextHandleMode handle_mode;

      handle_mode = _ctk_text_handle_get_mode (priv->text_handle);

      if (handle_mode != CTK_TEXT_HANDLE_MODE_NONE)
        ctk_entry_update_handles (entry, CTK_TEXT_HANDLE_MODE_CURSOR);
    }
}

static void
ctk_entry_delete_cb (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkEditable *editable = CTK_EDITABLE (entry);
  gint start, end;

  if (priv->editable)
    {
      if (ctk_editable_get_selection_bounds (editable, &start, &end))
	ctk_editable_delete_text (editable, start, end);
    }
}

static void
ctk_entry_toggle_overwrite (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  priv->overwrite_mode = !priv->overwrite_mode;
  ctk_entry_pend_cursor_blink (entry);
  ctk_widget_queue_draw (CTK_WIDGET (entry));
}

static void
ctk_entry_select_all (CtkEntry *entry)
{
  ctk_entry_select_line (entry);
}

static void
ctk_entry_real_activate (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkWindow *window;
  CtkWidget *default_widget, *focus_widget;
  CtkWidget *toplevel;
  CtkWidget *widget;

  widget = CTK_WIDGET (entry);

  if (priv->activates_default)
    {
      toplevel = ctk_widget_get_toplevel (widget);
      if (CTK_IS_WINDOW (toplevel))
	{
	  window = CTK_WINDOW (toplevel);

	  if (window)
            {
              default_widget = ctk_window_get_default_widget (window);
              focus_widget = ctk_window_get_focus (window);
              if (widget != default_widget &&
                  !(widget == focus_widget && (!default_widget || !ctk_widget_get_sensitive (default_widget))))
	        ctk_window_activate_default (window);
            }
	}
    }
}

static void
keymap_direction_changed (GdkKeymap *keymap,
                          CtkEntry  *entry)
{
  ctk_entry_recompute (entry);
}

/* IM Context Callbacks
 */

static void
ctk_entry_commit_cb (CtkIMContext *context,
		     const gchar  *str,
		     CtkEntry     *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->editable)
    ctk_entry_enter_text (entry, str);
}

static void 
ctk_entry_preedit_changed_cb (CtkIMContext *context,
			      CtkEntry     *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->editable)
    {
      gchar *preedit_string;
      gint cursor_pos;

      ctk_im_context_get_preedit_string (priv->im_context,
                                         &preedit_string, NULL,
                                         &cursor_pos);
      g_signal_emit (entry, signals[PREEDIT_CHANGED], 0, preedit_string);
      priv->preedit_length = strlen (preedit_string);
      cursor_pos = CLAMP (cursor_pos, 0, g_utf8_strlen (preedit_string, -1));
      priv->preedit_cursor = cursor_pos;
      g_free (preedit_string);
    
      ctk_entry_recompute (entry);
    }
}

static gboolean
ctk_entry_retrieve_surrounding_cb (CtkIMContext *context,
                                   CtkEntry     *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  gchar *text;

  /* XXXX ??? does this even make sense when text is not visible? Should we return FALSE? */
  text = _ctk_entry_get_display_text (entry, 0, -1);
  ctk_im_context_set_surrounding (context, text, strlen (text), /* Length in bytes */
				  g_utf8_offset_to_pointer (text, priv->current_pos) - text);
  g_free (text);

  return TRUE;
}

static gboolean
ctk_entry_delete_surrounding_cb (CtkIMContext *slave,
				 gint          offset,
				 gint          n_chars,
				 CtkEntry     *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->editable)
    ctk_editable_delete_text (CTK_EDITABLE (entry),
                              priv->current_pos + offset,
                              priv->current_pos + offset + n_chars);

  return TRUE;
}

/* Internal functions
 */

/* Used for im_commit_cb and inserting Unicode chars */
void
ctk_entry_enter_text (CtkEntry       *entry,
                      const gchar    *str)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkEditable *editable = CTK_EDITABLE (entry);
  gint tmp_pos;
  gboolean old_need_im_reset;
  guint text_length;

  old_need_im_reset = priv->need_im_reset;
  priv->need_im_reset = FALSE;

  if (ctk_editable_get_selection_bounds (editable, NULL, NULL))
    ctk_editable_delete_selection (editable);
  else
    {
      if (priv->overwrite_mode)
        {
          text_length = ctk_entry_buffer_get_length (get_buffer (entry));
          if (priv->current_pos < text_length)
            ctk_entry_delete_from_cursor (entry, CTK_DELETE_CHARS, 1);
        }
    }

  tmp_pos = priv->current_pos;
  ctk_editable_insert_text (editable, str, strlen (str), &tmp_pos);
  ctk_editable_set_position (editable, tmp_pos);

  priv->need_im_reset = old_need_im_reset;
}

/* All changes to priv->current_pos and priv->selection_bound
 * should go through this function.
 */
void
ctk_entry_set_positions (CtkEntry *entry,
			 gint      current_pos,
			 gint      selection_bound)
{
  CtkEntryPrivate *priv = entry->priv;
  gboolean changed = FALSE;

  g_object_freeze_notify (G_OBJECT (entry));

  if (current_pos != -1 &&
      priv->current_pos != current_pos)
    {
      priv->current_pos = current_pos;
      changed = TRUE;

      g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_CURSOR_POSITION]);
    }

  if (selection_bound != -1 &&
      priv->selection_bound != selection_bound)
    {
      priv->selection_bound = selection_bound;
      changed = TRUE;

      g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_SELECTION_BOUND]);
    }

  g_object_thaw_notify (G_OBJECT (entry));

  if (priv->current_pos != priv->selection_bound)
    {
      if (!priv->selection_node)
        {
          CtkCssNode *widget_node;

          widget_node = ctk_css_gadget_get_node (priv->gadget);
          priv->selection_node = ctk_css_node_new ();
          ctk_css_node_set_name (priv->selection_node, I_("selection"));
          ctk_css_node_set_parent (priv->selection_node, widget_node);
          ctk_css_node_set_state (priv->selection_node, ctk_css_node_get_state (widget_node));
          g_object_unref (priv->selection_node);
        }
    }
  else
    {
      if (priv->selection_node)
        {
          ctk_css_node_set_parent (priv->selection_node, NULL);
          priv->selection_node = NULL;
        }
    }

  if (changed)
    {
      ctk_entry_move_adjustments (entry);
      ctk_entry_recompute (entry);
    }
}

static void
ctk_entry_reset_layout (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->cached_layout)
    {
      g_object_unref (priv->cached_layout);
      priv->cached_layout = NULL;
    }
}

static void
update_im_cursor_location (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  GdkRectangle area;
  gint strong_x;
  gint strong_xoffset;
  gint area_width, area_height;

  ctk_entry_get_cursor_locations (entry, CURSOR_STANDARD, &strong_x, NULL);
  ctk_entry_get_text_area_size (entry, NULL, NULL, &area_width, &area_height);

  strong_xoffset = strong_x - priv->scroll_offset;
  if (strong_xoffset < 0)
    {
      strong_xoffset = 0;
    }
  else if (strong_xoffset > area_width)
    {
      strong_xoffset = area_width;
    }
  area.x = strong_xoffset;
  area.y = 0;
  area.width = 0;
  area.height = area_height;

  ctk_im_context_set_cursor_location (priv->im_context, &area);
}

static void
ctk_entry_recompute (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkTextHandleMode handle_mode;

  ctk_entry_reset_layout (entry);
  ctk_entry_check_cursor_blink (entry);

  ctk_entry_adjust_scroll (entry);

  update_im_cursor_location (entry);

  if (priv->text_handle)
    {
      handle_mode = _ctk_text_handle_get_mode (priv->text_handle);

      if (handle_mode != CTK_TEXT_HANDLE_MODE_NONE)
        ctk_entry_update_handles (entry, handle_mode);
    }

  ctk_widget_queue_draw (CTK_WIDGET (entry));
}

static void
ctk_entry_get_placeholder_text_color (CtkEntry   *entry,
                                      PangoColor *color)
{
  CtkWidget *widget = CTK_WIDGET (entry);
  CtkStyleContext *context;
  GdkRGBA fg = { 0.5, 0.5, 0.5 };

  context = ctk_widget_get_style_context (widget);
  ctk_style_context_lookup_color (context, "placeholder_text_color", &fg);

  color->red = CLAMP (fg.red * 65535. + 0.5, 0, 65535);
  color->green = CLAMP (fg.green * 65535. + 0.5, 0, 65535);
  color->blue = CLAMP (fg.blue * 65535. + 0.5, 0, 65535);
}

static inline gboolean
show_placeholder_text (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (!ctk_widget_has_focus (CTK_WIDGET (entry)) &&
      ctk_entry_buffer_get_bytes (get_buffer (entry)) == 0 &&
      priv->placeholder_text != NULL)
    return TRUE;

  return FALSE;
}

static PangoLayout *
ctk_entry_create_layout (CtkEntry *entry,
			 gboolean  include_preedit)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkWidget *widget = CTK_WIDGET (entry);
  CtkStyleContext *context;
  PangoLayout *layout;
  PangoAttrList *tmp_attrs;
  gboolean placeholder_layout;

  gchar *preedit_string = NULL;
  gint preedit_length = 0;
  PangoAttrList *preedit_attrs = NULL;

  gchar *display_text;
  guint n_bytes;

  context = ctk_widget_get_style_context (widget);

  layout = ctk_widget_create_pango_layout (widget, NULL);
  pango_layout_set_single_paragraph_mode (layout, TRUE);

  tmp_attrs = _ctk_style_context_get_pango_attributes (context);
  tmp_attrs = _ctk_pango_attr_list_merge (tmp_attrs, priv->attrs);
  if (!tmp_attrs)
    tmp_attrs = pango_attr_list_new ();

  placeholder_layout = show_placeholder_text (entry);
  if (placeholder_layout)
    display_text = g_strdup (priv->placeholder_text);
  else
    display_text = _ctk_entry_get_display_text (entry, 0, -1);

  n_bytes = strlen (display_text);

  if (!placeholder_layout && include_preedit)
    {
      ctk_im_context_get_preedit_string (priv->im_context,
					 &preedit_string, &preedit_attrs, NULL);
      preedit_length = priv->preedit_length;
    }
  else if (placeholder_layout)
    {
      PangoColor color;
      PangoAttribute *attr;

      ctk_entry_get_placeholder_text_color (entry, &color);
      attr = pango_attr_foreground_new (color.red, color.green, color.blue);
      attr->start_index = 0;
      attr->end_index = G_MAXINT;
      pango_attr_list_insert (tmp_attrs, attr);
      pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
    }

  if (preedit_length)
    {
      GString *tmp_string = g_string_new (display_text);
      gint pos;

      pos = g_utf8_offset_to_pointer (display_text, priv->current_pos) - display_text;
      g_string_insert (tmp_string, pos, preedit_string);
      pango_layout_set_text (layout, tmp_string->str, tmp_string->len);
      pango_attr_list_splice (tmp_attrs, preedit_attrs, pos, preedit_length);
      g_string_free (tmp_string, TRUE);
    }
  else
    {
      PangoDirection pango_dir;

      if (ctk_entry_get_display_mode (entry) == DISPLAY_NORMAL)
	pango_dir = _ctk_pango_find_base_dir (display_text, n_bytes);
      else
	pango_dir = PANGO_DIRECTION_NEUTRAL;

      if (pango_dir == PANGO_DIRECTION_NEUTRAL)
        {
          if (ctk_widget_has_focus (widget))
	    {
	      GdkDisplay *display = ctk_widget_get_display (widget);
	      GdkKeymap *keymap = gdk_keymap_get_for_display (display);
	      if (gdk_keymap_get_direction (keymap) == PANGO_DIRECTION_RTL)
		pango_dir = PANGO_DIRECTION_RTL;
	      else
		pango_dir = PANGO_DIRECTION_LTR;
	    }
          else
	    {
	      if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL)
		pango_dir = PANGO_DIRECTION_RTL;
	      else
		pango_dir = PANGO_DIRECTION_LTR;
	    }
        }

      pango_context_set_base_dir (ctk_widget_get_pango_context (widget), pango_dir);

      priv->resolved_dir = pango_dir;

      pango_layout_set_text (layout, display_text, n_bytes);
    }

  pango_layout_set_attributes (layout, tmp_attrs);

  if (priv->tabs)
    pango_layout_set_tabs (layout, priv->tabs);

  g_free (preedit_string);
  g_free (display_text);

  if (preedit_attrs)
    pango_attr_list_unref (preedit_attrs);

  pango_attr_list_unref (tmp_attrs);

  return layout;
}

static PangoLayout *
ctk_entry_ensure_layout (CtkEntry *entry,
                         gboolean  include_preedit)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->preedit_length > 0 &&
      !include_preedit != !priv->cache_includes_preedit)
    ctk_entry_reset_layout (entry);

  if (!priv->cached_layout)
    {
      priv->cached_layout = ctk_entry_create_layout (entry, include_preedit);
      priv->cache_includes_preedit = include_preedit;
    }

  return priv->cached_layout;
}

static void
get_layout_position (CtkEntry *entry,
                     gint     *x,
                     gint     *y)
{
  CtkEntryPrivate *priv = entry->priv;
  PangoLayout *layout;
  PangoRectangle logical_rect;
  gint y_pos, area_height;
  PangoLayoutLine *line;

  layout = ctk_entry_ensure_layout (entry, TRUE);

  area_height = PANGO_SCALE * priv->text_allocation.height;

  line = pango_layout_get_lines_readonly (layout)->data;
  pango_layout_line_get_extents (line, NULL, &logical_rect);

  /* Align primarily for locale's ascent/descent */
  if (priv->text_baseline < 0)
    y_pos = ((area_height - priv->ascent - priv->descent) / 2 +
             priv->ascent + logical_rect.y);
  else
    y_pos = PANGO_SCALE * priv->text_baseline - pango_layout_get_baseline (layout);

  /* Now see if we need to adjust to fit in actual drawn string */
  if (logical_rect.height > area_height)
    y_pos = (area_height - logical_rect.height) / 2;
  else if (y_pos < 0)
    y_pos = 0;
  else if (y_pos + logical_rect.height > area_height)
    y_pos = area_height - logical_rect.height;
  
  y_pos = y_pos / PANGO_SCALE;

  if (x)
    *x = - priv->scroll_offset;

  if (y)
    *y = y_pos;
}

static void
ctk_entry_draw_text (CtkEntry *entry,
                     cairo_t  *cr)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkWidget *widget = CTK_WIDGET (entry);
  CtkStyleContext *context;
  PangoLayout *layout;
  gint x, y;
  gint start_pos, end_pos;
  CtkAllocation allocation;

  /* Nothing to display at all */
  if (ctk_entry_get_display_mode (entry) == DISPLAY_BLANK)
    return;

  context = ctk_widget_get_style_context (widget);
  ctk_widget_get_allocation (CTK_WIDGET (entry), &allocation);
  layout = ctk_entry_ensure_layout (entry, TRUE);

  cairo_save (cr);

  cairo_rectangle (cr,
                   priv->text_allocation.x - allocation.x,
                   priv->text_allocation.y - allocation.y,
                   priv->text_allocation.width,
                   priv->text_allocation.height);
  cairo_clip (cr);

  ctk_entry_get_layout_offsets (entry, &x, &y);

  if (show_placeholder_text (entry))
    pango_layout_set_width (layout, PANGO_SCALE * priv->text_allocation.width);

  ctk_render_layout (context, cr, x, y, layout);

  if (ctk_editable_get_selection_bounds (CTK_EDITABLE (entry), &start_pos, &end_pos))
    {
      const char *text = pango_layout_get_text (layout);
      gint start_index = g_utf8_offset_to_pointer (text, start_pos) - text;
      gint end_index = g_utf8_offset_to_pointer (text, end_pos) - text;
      cairo_region_t *clip;
      gint range[2];

      range[0] = MIN (start_index, end_index);
      range[1] = MAX (start_index, end_index);

      ctk_style_context_save_to_node (context, priv->selection_node);

      clip = gdk_pango_layout_get_clip_region (layout, x, y, range, 1);
      gdk_cairo_region (cr, clip);
      cairo_clip (cr);
      cairo_region_destroy (clip);

      ctk_render_background (context, cr,
                             0, 0,
                             allocation.width, allocation.height);

      ctk_render_layout (context, cr, x, y, layout);

      ctk_style_context_restore (context);
    }

  cairo_restore (cr);
}

static void
ctk_entry_draw_cursor (CtkEntry  *entry,
                       cairo_t   *cr,
		       CursorType type)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkWidget *widget = CTK_WIDGET (entry);
  CtkStyleContext *context;
  PangoRectangle cursor_rect;
  gint cursor_index;
  gboolean block;
  gboolean block_at_line_end;
  PangoLayout *layout;
  const char *text;
  gint x, y;

  context = ctk_widget_get_style_context (widget);

  layout = ctk_entry_ensure_layout (entry, TRUE);
  text = pango_layout_get_text (layout);
  ctk_entry_get_layout_offsets (entry, &x, &y);

  if (type == CURSOR_DND)
    cursor_index = g_utf8_offset_to_pointer (text, priv->dnd_position) - text;
  else
    cursor_index = g_utf8_offset_to_pointer (text, priv->current_pos + priv->preedit_cursor) - text;

  if (!priv->overwrite_mode)
    block = FALSE;
  else
    block = _ctk_text_util_get_block_cursor_location (layout,
                                                      cursor_index, &cursor_rect, &block_at_line_end);

  if (!block)
    {
      ctk_render_insertion_cursor (context, cr,
                                   x, y,
                                   layout, cursor_index, priv->resolved_dir);
    }
  else /* overwrite_mode */
    {
      GdkRGBA cursor_color;
      GdkRectangle rect;

      cairo_save (cr);

      rect.x = PANGO_PIXELS (cursor_rect.x) + x;
      rect.y = PANGO_PIXELS (cursor_rect.y) + y;
      rect.width = PANGO_PIXELS (cursor_rect.width);
      rect.height = PANGO_PIXELS (cursor_rect.height);

      _ctk_style_context_get_cursor_color (context, &cursor_color, NULL);
      gdk_cairo_set_source_rgba (cr, &cursor_color);
      gdk_cairo_rectangle (cr, &rect);
      cairo_fill (cr);

      if (!block_at_line_end)
        {
          GdkRGBA color;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          ctk_style_context_get_background_color (context,
                                                  ctk_style_context_get_state (context),
                                                  &color);
G_GNUC_END_IGNORE_DEPRECATIONS

          gdk_cairo_rectangle (cr, &rect);
          cairo_clip (cr);
          cairo_move_to (cr, x, y);
          gdk_cairo_set_source_rgba (cr, &color);
          pango_cairo_show_layout (cr, layout);
        }

      cairo_restore (cr);
    }
}

static void
ctk_entry_handle_dragged (CtkTextHandle         *handle,
                          CtkTextHandlePosition  pos,
                          gint                   x,
                          gint                   y,
                          CtkEntry              *entry)
{
  gint cursor_pos, selection_bound_pos, tmp_pos;
  CtkEntryPrivate *priv = entry->priv;
  CtkTextHandleMode mode;
  gint *min, *max;

  ctk_entry_selection_bubble_popup_unset (entry);

  cursor_pos = priv->current_pos;
  selection_bound_pos = priv->selection_bound;
  mode = _ctk_text_handle_get_mode (handle);

  tmp_pos = ctk_entry_find_position (entry, x + priv->scroll_offset);

  if (mode == CTK_TEXT_HANDLE_MODE_CURSOR ||
      cursor_pos >= selection_bound_pos)
    {
      max = &cursor_pos;
      min = &selection_bound_pos;
    }
  else
    {
      max = &selection_bound_pos;
      min = &cursor_pos;
    }

  if (pos == CTK_TEXT_HANDLE_POSITION_SELECTION_END)
    {
      if (mode == CTK_TEXT_HANDLE_MODE_SELECTION)
        {
          gint min_pos;

          min_pos = MAX (*min + 1, 0);
          tmp_pos = MAX (tmp_pos, min_pos);
        }

      *max = tmp_pos;
    }
  else
    {
      if (mode == CTK_TEXT_HANDLE_MODE_SELECTION)
        {
          gint max_pos;

          max_pos = *max - 1;
          *min = MIN (tmp_pos, max_pos);
        }
    }

  if (cursor_pos != priv->current_pos ||
      selection_bound_pos != priv->selection_bound)
    {
      if (mode == CTK_TEXT_HANDLE_MODE_CURSOR)
        {
          entry->priv->cursor_handle_dragged = TRUE;
          ctk_entry_set_positions (entry, cursor_pos, cursor_pos);
        }
      else
        {
          entry->priv->selection_handle_dragged = TRUE;
          ctk_entry_set_positions (entry, cursor_pos, selection_bound_pos);
        }

      ctk_entry_update_handles (entry, mode);
    }

  ctk_entry_show_magnifier (entry, x, y);
}

static void
ctk_entry_handle_drag_started (CtkTextHandle         *handle,
                               CtkTextHandlePosition  pos,
                               CtkEntry              *entry)
{
  entry->priv->cursor_handle_dragged = FALSE;
  entry->priv->selection_handle_dragged = FALSE;
}

static void
ctk_entry_handle_drag_finished (CtkTextHandle         *handle,
                                CtkTextHandlePosition  pos,
                                CtkEntry              *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (!priv->cursor_handle_dragged && !priv->selection_handle_dragged)
    {
      CtkSettings *settings;
      guint double_click_time;

      settings = ctk_widget_get_settings (CTK_WIDGET (entry));
      g_object_get (settings, "ctk-double-click-time", &double_click_time, NULL);
      if (g_get_monotonic_time() - priv->handle_place_time < double_click_time * 1000)
        {
          ctk_entry_select_word (entry);
          ctk_entry_update_handles (entry, CTK_TEXT_HANDLE_MODE_SELECTION);
        }
      else
        ctk_entry_selection_bubble_popup_set (entry);
    }

  if (priv->magnifier_popover)
    ctk_popover_popdown (CTK_POPOVER (priv->magnifier_popover));
}


/**
 * ctk_entry_reset_im_context:
 * @entry: a #CtkEntry
 *
 * Reset the input method context of the entry if needed.
 *
 * This can be necessary in the case where modifying the buffer
 * would confuse on-going input method behavior.
 *
 * Since: 2.22
 */
void
ctk_entry_reset_im_context (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  g_return_if_fail (CTK_IS_ENTRY (entry));

  if (priv->need_im_reset)
    {
      priv->need_im_reset = FALSE;
      ctk_im_context_reset (priv->im_context);
    }
}

/**
 * ctk_entry_im_context_filter_keypress:
 * @entry: a #CtkEntry
 * @event: (type Gdk.EventKey): the key event
 *
 * Allow the #CtkEntry input method to internally handle key press
 * and release events. If this function returns %TRUE, then no further
 * processing should be done for this key event. See
 * ctk_im_context_filter_keypress().
 *
 * Note that you are expected to call this function from your handler
 * when overriding key event handling. This is needed in the case when
 * you need to insert your own key handling between the input method
 * and the default key event handling of the #CtkEntry.
 * See ctk_text_view_reset_im_context() for an example of use.
 *
 * Returns: %TRUE if the input method handled the key event.
 *
 * Since: 2.22
 */
gboolean
ctk_entry_im_context_filter_keypress (CtkEntry    *entry,
                                      GdkEventKey *event)
{
  CtkEntryPrivate *priv;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), FALSE);

  priv = entry->priv;

  return ctk_im_context_filter_keypress (priv->im_context, event);
}

CtkIMContext*
_ctk_entry_get_im_context (CtkEntry *entry)
{
  return entry->priv->im_context;
}

CtkCssGadget *
ctk_entry_get_gadget (CtkEntry  *entry)
{
  return entry->priv->gadget;
}

static gint
ctk_entry_find_position (CtkEntry *entry,
			 gint      x)
{
  CtkEntryPrivate *priv = entry->priv;
  PangoLayout *layout;
  PangoLayoutLine *line;
  gint index;
  gint pos;
  gint trailing;
  const gchar *text;
  gint cursor_index;
  
  layout = ctk_entry_ensure_layout (entry, TRUE);
  text = pango_layout_get_text (layout);
  cursor_index = g_utf8_offset_to_pointer (text, priv->current_pos) - text;

  line = pango_layout_get_lines_readonly (layout)->data;
  pango_layout_line_x_to_index (line, x * PANGO_SCALE, &index, &trailing);

  if (index >= cursor_index && priv->preedit_length)
    {
      if (index >= cursor_index + priv->preedit_length)
	index -= priv->preedit_length;
      else
	{
	  index = cursor_index;
	  trailing = 0;
	}
    }

  pos = g_utf8_pointer_to_offset (text, text + index);
  pos += trailing;

  return pos;
}

static void
ctk_entry_get_cursor_locations (CtkEntry   *entry,
				CursorType  type,
				gint       *strong_x,
				gint       *weak_x)
{
  CtkEntryPrivate *priv = entry->priv;
  DisplayMode mode = ctk_entry_get_display_mode (entry);

  /* Nothing to display at all, so no cursor is relevant */
  if (mode == DISPLAY_BLANK)
    {
      if (strong_x)
	*strong_x = 0;
      
      if (weak_x)
	*weak_x = 0;
    }
  else
    {
      PangoLayout *layout = ctk_entry_ensure_layout (entry, TRUE);
      const gchar *text = pango_layout_get_text (layout);
      PangoRectangle strong_pos, weak_pos;
      gint index;
  
      if (type == CURSOR_STANDARD)
	{
	  index = g_utf8_offset_to_pointer (text, priv->current_pos + priv->preedit_cursor) - text;
	}
      else /* type == CURSOR_DND */
	{
	  index = g_utf8_offset_to_pointer (text, priv->dnd_position) - text;

	  if (priv->dnd_position > priv->current_pos)
	    {
	      if (mode == DISPLAY_NORMAL)
		index += priv->preedit_length;
	      else
		{
		  gint preedit_len_chars = g_utf8_strlen (text, -1) - ctk_entry_buffer_get_length (get_buffer (entry));
		  index += preedit_len_chars * g_unichar_to_utf8 (priv->invisible_char, NULL);
		}
	    }
	}
      
      pango_layout_get_cursor_pos (layout, index, &strong_pos, &weak_pos);
      
      if (strong_x)
	*strong_x = strong_pos.x / PANGO_SCALE;
      
      if (weak_x)
	*weak_x = weak_pos.x / PANGO_SCALE;
    }
}

static gboolean
ctk_entry_get_is_selection_handle_dragged (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkTextHandlePosition pos;

  if (!priv->text_handle)
    return FALSE;

  if (_ctk_text_handle_get_mode (priv->text_handle) != CTK_TEXT_HANDLE_MODE_SELECTION)
    return FALSE;

  if (priv->current_pos >= priv->selection_bound)
    pos = CTK_TEXT_HANDLE_POSITION_SELECTION_START;
  else
    pos = CTK_TEXT_HANDLE_POSITION_SELECTION_END;

  return _ctk_text_handle_get_is_dragged (priv->text_handle, pos);
}

static void
ctk_entry_get_scroll_limits (CtkEntry *entry,
                             gint     *min_offset,
                             gint     *max_offset)
{
  CtkEntryPrivate *priv = entry->priv;
  gfloat xalign;
  PangoLayout *layout;
  PangoLayoutLine *line;
  PangoRectangle logical_rect;
  gint text_width;

  layout = ctk_entry_ensure_layout (entry, TRUE);
  line = pango_layout_get_lines_readonly (layout)->data;

  pango_layout_line_get_extents (line, NULL, &logical_rect);

  /* Display as much text as we can */

  if (priv->resolved_dir == PANGO_DIRECTION_LTR)
      xalign = priv->xalign;
  else
      xalign = 1.0 - priv->xalign;

  text_width = PANGO_PIXELS(logical_rect.width);

  if (text_width > priv->text_allocation.width)
    {
      *min_offset = 0;
      *max_offset = text_width - priv->text_allocation.width;
    }
  else
    {
      *min_offset = (text_width - priv->text_allocation.width) * xalign;
      *max_offset = *min_offset;
    }
}

static void
ctk_entry_adjust_scroll (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  gint min_offset, max_offset;
  gint strong_x, weak_x;
  gint strong_xoffset, weak_xoffset;
  CtkTextHandleMode handle_mode;

  if (!ctk_widget_get_realized (CTK_WIDGET (entry)))
    return;

  ctk_entry_get_scroll_limits (entry, &min_offset, &max_offset);

  priv->scroll_offset = CLAMP (priv->scroll_offset, min_offset, max_offset);

  if (ctk_entry_get_is_selection_handle_dragged (entry))
    {
      /* The text handle corresponding to the selection bound is
       * being dragged, ensure it stays onscreen even if we scroll
       * cursors away, this is so both handles can cause content
       * to scroll.
       */
      strong_x = weak_x = ctk_entry_get_selection_bound_location (entry);
    }
  else
    {
      /* And make sure cursors are on screen. Note that the cursor is
       * actually drawn one pixel into the INNER_BORDER space on
       * the right, when the scroll is at the utmost right. This
       * looks better to to me than confining the cursor inside the
       * border entirely, though it means that the cursor gets one
       * pixel closer to the edge of the widget on the right than
       * on the left. This might need changing if one changed
       * INNER_BORDER from 2 to 1, as one would do on a
       * small-screen-real-estate display.
       *
       * We always make sure that the strong cursor is on screen, and
       * put the weak cursor on screen if possible.
       */
      ctk_entry_get_cursor_locations (entry, CURSOR_STANDARD, &strong_x, &weak_x);
    }

  strong_xoffset = strong_x - priv->scroll_offset;

  if (strong_xoffset < 0)
    {
      priv->scroll_offset += strong_xoffset;
      strong_xoffset = 0;
    }
  else if (strong_xoffset > priv->text_allocation.width)
    {
      priv->scroll_offset += strong_xoffset - priv->text_allocation.width;
      strong_xoffset = priv->text_allocation.width;
    }

  weak_xoffset = weak_x - priv->scroll_offset;

  if (weak_xoffset < 0 && strong_xoffset - weak_xoffset <= priv->text_allocation.width)
    {
      priv->scroll_offset += weak_xoffset;
    }
  else if (weak_xoffset > priv->text_allocation.width &&
	   strong_xoffset - (weak_xoffset - priv->text_allocation.width) >= 0)
    {
      priv->scroll_offset += weak_xoffset - priv->text_allocation.width;
    }

  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_SCROLL_OFFSET]);

  if (priv->text_handle)
    {
      handle_mode = _ctk_text_handle_get_mode (priv->text_handle);

      if (handle_mode != CTK_TEXT_HANDLE_MODE_NONE)
        ctk_entry_update_handles (entry, handle_mode);
    }
}

static void
ctk_entry_move_adjustments (CtkEntry *entry)
{
  CtkWidget *widget = CTK_WIDGET (entry);
  CtkAllocation allocation;
  CtkAdjustment *adjustment;
  PangoContext *context;
  PangoFontMetrics *metrics;
  gint x, layout_x;
  gint char_width;

  adjustment = g_object_get_qdata (G_OBJECT (entry), quark_cursor_hadjustment);
  if (!adjustment)
    return;

  ctk_css_gadget_get_content_allocation (entry->priv->gadget, &allocation, NULL);

  /* Cursor/char position, layout offset, border width, and widget allocation */
  ctk_entry_get_cursor_locations (entry, CURSOR_STANDARD, &x, NULL);
  get_layout_position (entry, &layout_x, NULL);
  x += allocation.x + layout_x;

  /* Approximate width of a char, so user can see what is ahead/behind */
  context = ctk_widget_get_pango_context (widget);

  metrics = pango_context_get_metrics (context,
                                       pango_context_get_font_description (context),
				       pango_context_get_language (context));
  char_width = pango_font_metrics_get_approximate_char_width (metrics) / PANGO_SCALE;

  /* Scroll it */
  ctk_adjustment_clamp_page (adjustment, 
  			     x - (char_width + 1),   /* one char + one pixel before */
			     x + (char_width + 2));  /* one char + cursor + one pixel after */
}

static gint
ctk_entry_move_visually (CtkEntry *entry,
			 gint      start,
			 gint      count)
{
  CtkEntryPrivate *priv = entry->priv;
  gint index;
  PangoLayout *layout = ctk_entry_ensure_layout (entry, FALSE);
  const gchar *text;

  text = pango_layout_get_text (layout);
  
  index = g_utf8_offset_to_pointer (text, start) - text;

  while (count != 0)
    {
      int new_index, new_trailing;
      gboolean split_cursor;
      gboolean strong;

      g_object_get (ctk_widget_get_settings (CTK_WIDGET (entry)),
		    "ctk-split-cursor", &split_cursor,
		    NULL);

      if (split_cursor)
	strong = TRUE;
      else
	{
	  GdkKeymap *keymap = gdk_keymap_get_for_display (ctk_widget_get_display (CTK_WIDGET (entry)));
	  PangoDirection keymap_direction = gdk_keymap_get_direction (keymap);

	  strong = keymap_direction == priv->resolved_dir;
	}
      
      if (count > 0)
	{
	  pango_layout_move_cursor_visually (layout, strong, index, 0, 1, &new_index, &new_trailing);
	  count--;
	}
      else
	{
	  pango_layout_move_cursor_visually (layout, strong, index, 0, -1, &new_index, &new_trailing);
	  count++;
	}

      if (new_index < 0)
        index = 0;
      else if (new_index != G_MAXINT)
        index = new_index;
      
      while (new_trailing--)
	index = g_utf8_next_char (text + index) - text;
    }
  
  return g_utf8_pointer_to_offset (text, text + index);
}

static gint
ctk_entry_move_logically (CtkEntry *entry,
			  gint      start,
			  gint      count)
{
  gint new_pos = start;
  guint length;

  length = ctk_entry_buffer_get_length (get_buffer (entry));

  /* Prevent any leak of information */
  if (ctk_entry_get_display_mode (entry) != DISPLAY_NORMAL)
    {
      new_pos = CLAMP (start + count, 0, length);
    }
  else
    {
      PangoLayout *layout = ctk_entry_ensure_layout (entry, FALSE);
      PangoLogAttr *log_attrs;
      gint n_attrs;

      pango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);

      while (count > 0 && new_pos < length)
	{
	  do
	    new_pos++;
	  while (new_pos < length && !log_attrs[new_pos].is_cursor_position);
	  
	  count--;
	}
      while (count < 0 && new_pos > 0)
	{
	  do
	    new_pos--;
	  while (new_pos > 0 && !log_attrs[new_pos].is_cursor_position);
	  
	  count++;
	}
      
      g_free (log_attrs);
    }

  return new_pos;
}

static gint
ctk_entry_move_forward_word (CtkEntry *entry,
			     gint      start,
                             gboolean  allow_whitespace)
{
  gint new_pos = start;
  guint length;

  length = ctk_entry_buffer_get_length (get_buffer (entry));

  /* Prevent any leak of information */
  if (ctk_entry_get_display_mode (entry) != DISPLAY_NORMAL)
    {
      new_pos = length;
    }
  else if (new_pos < length)
    {
      PangoLayout *layout = ctk_entry_ensure_layout (entry, FALSE);
      PangoLogAttr *log_attrs;
      gint n_attrs;

      pango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);
      
      /* Find the next word boundary */
      new_pos++;
      while (new_pos < n_attrs - 1 && !(log_attrs[new_pos].is_word_end ||
                                        (log_attrs[new_pos].is_word_start && allow_whitespace)))
	new_pos++;

      g_free (log_attrs);
    }

  return new_pos;
}


static gint
ctk_entry_move_backward_word (CtkEntry *entry,
			      gint      start,
                              gboolean  allow_whitespace)
{
  gint new_pos = start;

  /* Prevent any leak of information */
  if (ctk_entry_get_display_mode (entry) != DISPLAY_NORMAL)
    {
      new_pos = 0;
    }
  else if (start > 0)
    {
      PangoLayout *layout = ctk_entry_ensure_layout (entry, FALSE);
      PangoLogAttr *log_attrs;
      gint n_attrs;

      pango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);

      new_pos = start - 1;

      /* Find the previous word boundary */
      while (new_pos > 0 && !(log_attrs[new_pos].is_word_start || 
                              (log_attrs[new_pos].is_word_end && allow_whitespace)))
	new_pos--;

      g_free (log_attrs);
    }

  return new_pos;
}

static void
ctk_entry_delete_whitespace (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  PangoLayout *layout = ctk_entry_ensure_layout (entry, FALSE);
  PangoLogAttr *log_attrs;
  gint n_attrs;
  gint start, end;

  pango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);

  start = end = priv->current_pos;
  
  while (start > 0 && log_attrs[start-1].is_white)
    start--;

  while (end < n_attrs && log_attrs[end].is_white)
    end++;

  g_free (log_attrs);

  if (start != end)
    ctk_editable_delete_text (CTK_EDITABLE (entry), start, end);
}


static void
ctk_entry_select_word (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  gint start_pos = ctk_entry_move_backward_word (entry, priv->current_pos, TRUE);
  gint end_pos = ctk_entry_move_forward_word (entry, priv->current_pos, TRUE);

  ctk_editable_select_region (CTK_EDITABLE (entry), start_pos, end_pos);
}

static void
ctk_entry_select_line (CtkEntry *entry)
{
  ctk_editable_select_region (CTK_EDITABLE (entry), 0, -1);
}

static gint
truncate_multiline (const gchar *text)
{
  gint length;

  for (length = 0;
       text[length] && text[length] != '\n' && text[length] != '\r';
       length++);

  return length;
}

static void
paste_received (CtkClipboard *clipboard,
		const gchar  *text,
		gpointer      data)
{
  CtkEntry *entry = CTK_ENTRY (data);
  CtkEditable *editable = CTK_EDITABLE (entry);
  CtkEntryPrivate *priv = entry->priv;
  guint button;

  button = ctk_gesture_single_get_current_button (CTK_GESTURE_SINGLE (priv->multipress_gesture));

  if (button == GDK_BUTTON_MIDDLE)
    {
      gint pos, start, end;
      pos = priv->insert_pos;
      ctk_editable_get_selection_bounds (editable, &start, &end);
      if (!((start <= pos && pos <= end) || (end <= pos && pos <= start)))
	ctk_editable_select_region (editable, pos, pos);
    }
      
  if (text)
    {
      gint pos, start, end;
      gint length = -1;
      gboolean popup_completion;
      CtkEntryCompletion *completion;

      completion = ctk_entry_get_completion (entry);

      if (priv->truncate_multiline)
        length = truncate_multiline (text);

      /* only complete if the selection is at the end */
      popup_completion = (ctk_entry_buffer_get_length (get_buffer (entry)) ==
                          MAX (priv->current_pos, priv->selection_bound));

      if (completion)
	{
	  if (ctk_widget_get_mapped (completion->priv->popup_window))
	    _ctk_entry_completion_popdown (completion);

          if (!popup_completion && completion->priv->changed_id > 0)
            g_signal_handler_block (entry, completion->priv->changed_id);
	}

      begin_change (entry);
      if (ctk_editable_get_selection_bounds (editable, &start, &end))
        ctk_editable_delete_text (editable, start, end);

      pos = priv->current_pos;
      ctk_editable_insert_text (editable, text, length, &pos);
      ctk_editable_set_position (editable, pos);
      end_change (entry);

      if (completion &&
          !popup_completion && completion->priv->changed_id > 0)
	g_signal_handler_unblock (entry, completion->priv->changed_id);
    }

  g_object_unref (entry);
}

static void
ctk_entry_paste (CtkEntry *entry,
		 GdkAtom   selection)
{
  g_object_ref (entry);
  ctk_clipboard_request_text (ctk_widget_get_clipboard (CTK_WIDGET (entry), selection),
			      paste_received, entry);
}

static void
primary_get_cb (CtkClipboard     *clipboard,
		CtkSelectionData *selection_data,
		guint             info,
		gpointer          data)
{
  CtkEntry *entry = CTK_ENTRY (data);
  gint start, end;
  
  if (ctk_editable_get_selection_bounds (CTK_EDITABLE (entry), &start, &end))
    {
      gchar *str = _ctk_entry_get_display_text (entry, start, end);
      ctk_selection_data_set_text (selection_data, str, -1);
      g_free (str);
    }
}

static void
primary_clear_cb (CtkClipboard *clipboard,
		  gpointer      data)
{
  CtkEntry *entry = CTK_ENTRY (data);
  CtkEntryPrivate *priv = entry->priv;

  ctk_editable_select_region (CTK_EDITABLE (entry), priv->current_pos, priv->current_pos);
}

static void
ctk_entry_update_primary_selection (CtkEntry *entry)
{
  CtkTargetList *list;
  CtkTargetEntry *targets;
  CtkClipboard *clipboard;
  gint start, end;
  gint n_targets;

  if (!ctk_widget_get_realized (CTK_WIDGET (entry)))
    return;

  list = ctk_target_list_new (NULL, 0);
  ctk_target_list_add_text_targets (list, 0);

  targets = ctk_target_table_new_from_list (list, &n_targets);

  clipboard = ctk_widget_get_clipboard (CTK_WIDGET (entry), GDK_SELECTION_PRIMARY);
  
  if (ctk_editable_get_selection_bounds (CTK_EDITABLE (entry), &start, &end))
    {
      ctk_clipboard_set_with_owner (clipboard, targets, n_targets,
                                    primary_get_cb, primary_clear_cb, G_OBJECT (entry));
    }
  else
    {
      if (ctk_clipboard_get_owner (clipboard) == G_OBJECT (entry))
	ctk_clipboard_clear (clipboard);
    }

  ctk_target_table_free (targets, n_targets);
  ctk_target_list_unref (list);
}

static void
ctk_entry_clear_icon (CtkEntry             *entry,
                      CtkEntryIconPosition  icon_pos)
{
  CtkEntryPrivate *priv = entry->priv;
  EntryIconInfo *icon_info = priv->icons[icon_pos];
  CtkIconHelper *icon_helper;
  CtkImageType storage_type;

  if (icon_info == NULL)
    return;

  icon_helper = CTK_ICON_HELPER (icon_info->gadget);
  if (_ctk_icon_helper_get_is_empty (icon_helper))
    return;

  g_object_freeze_notify (G_OBJECT (entry));

  /* Explicitly check, as the pointer may become invalidated
   * during destruction.
   */
  if (GDK_IS_WINDOW (icon_info->window))
    gdk_window_hide (icon_info->window);

  storage_type = _ctk_icon_helper_get_storage_type (icon_helper);

  switch (storage_type)
    {
    case CTK_IMAGE_PIXBUF:
      g_object_notify_by_pspec (G_OBJECT (entry),
                                entry_props[icon_pos == CTK_ENTRY_ICON_PRIMARY
                                            ? PROP_PIXBUF_PRIMARY
                                            : PROP_PIXBUF_SECONDARY]);
      break;

    case CTK_IMAGE_STOCK:
      g_object_notify_by_pspec (G_OBJECT (entry),
                                entry_props[icon_pos == CTK_ENTRY_ICON_PRIMARY
                                            ? PROP_STOCK_PRIMARY
                                            : PROP_STOCK_SECONDARY]);
      break;

    case CTK_IMAGE_ICON_NAME:
      g_object_notify_by_pspec (G_OBJECT (entry),
                                entry_props[icon_pos == CTK_ENTRY_ICON_PRIMARY
                                            ? PROP_ICON_NAME_PRIMARY
                                            : PROP_ICON_NAME_SECONDARY]);
      break;

    case CTK_IMAGE_GICON:
      g_object_notify_by_pspec (G_OBJECT (entry),
                                entry_props[icon_pos == CTK_ENTRY_ICON_PRIMARY
                                            ? PROP_GICON_PRIMARY
                                            : PROP_GICON_SECONDARY]);
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  _ctk_icon_helper_clear (icon_helper);

  g_object_notify_by_pspec (G_OBJECT (entry),
                            entry_props[icon_pos == CTK_ENTRY_ICON_PRIMARY
                            ? PROP_STORAGE_TYPE_PRIMARY
                            : PROP_STORAGE_TYPE_SECONDARY]);

  g_object_thaw_notify (G_OBJECT (entry));
}

/* Public API
 */

/**
 * ctk_entry_new:
 *
 * Creates a new entry.
 *
 * Returns: a new #CtkEntry.
 */
CtkWidget*
ctk_entry_new (void)
{
  return g_object_new (CTK_TYPE_ENTRY, NULL);
}

/**
 * ctk_entry_new_with_buffer:
 * @buffer: The buffer to use for the new #CtkEntry.
 *
 * Creates a new entry with the specified text buffer.
 *
 * Returns: a new #CtkEntry
 *
 * Since: 2.18
 */
CtkWidget*
ctk_entry_new_with_buffer (CtkEntryBuffer *buffer)
{
  g_return_val_if_fail (CTK_IS_ENTRY_BUFFER (buffer), NULL);
  return g_object_new (CTK_TYPE_ENTRY, "buffer", buffer, NULL);
}

static CtkEntryBuffer*
get_buffer (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->buffer == NULL)
    {
      CtkEntryBuffer *buffer;
      buffer = ctk_entry_buffer_new (NULL, 0);
      ctk_entry_set_buffer (entry, buffer);
      g_object_unref (buffer);
    }

  return priv->buffer;
}

/**
 * ctk_entry_get_buffer:
 * @entry: a #CtkEntry
 *
 * Get the #CtkEntryBuffer object which holds the text for
 * this widget.
 *
 * Since: 2.18
 *
 * Returns: (transfer none): A #CtkEntryBuffer object.
 */
CtkEntryBuffer*
ctk_entry_get_buffer (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), NULL);

  return get_buffer (entry);
}

/**
 * ctk_entry_set_buffer:
 * @entry: a #CtkEntry
 * @buffer: a #CtkEntryBuffer
 *
 * Set the #CtkEntryBuffer object which holds the text for
 * this widget.
 *
 * Since: 2.18
 */
void
ctk_entry_set_buffer (CtkEntry       *entry,
                      CtkEntryBuffer *buffer)
{
  CtkEntryPrivate *priv;
  GObject *obj;
  gboolean had_buffer = FALSE;

  g_return_if_fail (CTK_IS_ENTRY (entry));

  priv = entry->priv;

  if (buffer)
    {
      g_return_if_fail (CTK_IS_ENTRY_BUFFER (buffer));
      g_object_ref (buffer);
    }

  if (priv->buffer)
    {
      had_buffer = TRUE;
      buffer_disconnect_signals (entry);
      g_object_unref (priv->buffer);
    }

  priv->buffer = buffer;

  if (priv->buffer)
     buffer_connect_signals (entry);

  obj = G_OBJECT (entry);
  g_object_freeze_notify (obj);
  g_object_notify_by_pspec (obj, entry_props[PROP_BUFFER]);
  g_object_notify_by_pspec (obj, entry_props[PROP_TEXT]);
  g_object_notify_by_pspec (obj, entry_props[PROP_TEXT_LENGTH]);
  g_object_notify_by_pspec (obj, entry_props[PROP_MAX_LENGTH]);
  g_object_notify_by_pspec (obj, entry_props[PROP_VISIBILITY]);
  g_object_notify_by_pspec (obj, entry_props[PROP_INVISIBLE_CHAR]);
  g_object_notify_by_pspec (obj, entry_props[PROP_INVISIBLE_CHAR_SET]);
  g_object_thaw_notify (obj);

  if (had_buffer)
    {
      ctk_editable_set_position (CTK_EDITABLE (entry), 0);
      ctk_entry_recompute (entry);
    }
}

/**
 * ctk_entry_get_text_area:
 * @entry: a #CtkEntry
 * @text_area: (out): Return location for the text area.
 *
 * Gets the area where the entry’s text is drawn. This function is
 * useful when drawing something to the entry in a draw callback.
 *
 * If the entry is not realized, @text_area is filled with zeros.
 *
 * See also ctk_entry_get_icon_area().
 *
 * Since: 3.0
 **/
void
ctk_entry_get_text_area (CtkEntry     *entry,
                         GdkRectangle *text_area)
{
  CtkEntryPrivate *priv;

  g_return_if_fail (CTK_IS_ENTRY (entry));
  g_return_if_fail (text_area != NULL);

  priv = entry->priv;

  if (ctk_widget_get_realized (CTK_WIDGET (entry)))
    {
      CtkAllocation allocation;

      *text_area = priv->text_allocation;

      ctk_widget_get_allocation (CTK_WIDGET (entry), &allocation);
      text_area->x -= allocation.x;
      text_area->y -= allocation.y;
    }
  else
    {
      text_area->x = 0;
      text_area->y = 0;
      text_area->width = 0;
      text_area->height = 0;
    }
}

/**
 * ctk_entry_set_text:
 * @entry: a #CtkEntry
 * @text: the new text
 *
 * Sets the text in the widget to the given
 * value, replacing the current contents.
 *
 * See ctk_entry_buffer_set_text().
 */
void
ctk_entry_set_text (CtkEntry    *entry,
		    const gchar *text)
{
  gint tmp_pos;
  CtkEntryCompletion *completion;

  g_return_if_fail (CTK_IS_ENTRY (entry));
  g_return_if_fail (text != NULL);

  /* Actually setting the text will affect the cursor and selection;
   * if the contents don't actually change, this will look odd to the user.
   */
  if (strcmp (ctk_entry_buffer_get_text (get_buffer (entry)), text) == 0)
    return;

  completion = ctk_entry_get_completion (entry);
  if (completion && completion->priv->changed_id > 0)
    g_signal_handler_block (entry, completion->priv->changed_id);

  begin_change (entry);
  ctk_editable_delete_text (CTK_EDITABLE (entry), 0, -1);
  tmp_pos = 0;
  ctk_editable_insert_text (CTK_EDITABLE (entry), text, strlen (text), &tmp_pos);
  end_change (entry);

  if (completion && completion->priv->changed_id > 0)
    g_signal_handler_unblock (entry, completion->priv->changed_id);
}

/**
 * ctk_entry_set_visibility:
 * @entry: a #CtkEntry
 * @visible: %TRUE if the contents of the entry are displayed
 *           as plaintext
 *
 * Sets whether the contents of the entry are visible or not.
 * When visibility is set to %FALSE, characters are displayed
 * as the invisible char, and will also appear that way when
 * the text in the entry widget is copied elsewhere.
 *
 * By default, CTK+ picks the best invisible character available
 * in the current font, but it can be changed with
 * ctk_entry_set_invisible_char().
 *
 * Note that you probably want to set #CtkEntry:input-purpose
 * to %CTK_INPUT_PURPOSE_PASSWORD or %CTK_INPUT_PURPOSE_PIN to
 * inform input methods about the purpose of this entry,
 * in addition to setting visibility to %FALSE.
 */
void
ctk_entry_set_visibility (CtkEntry *entry,
			  gboolean visible)
{
  CtkEntryPrivate *priv;

  g_return_if_fail (CTK_IS_ENTRY (entry));

  priv = entry->priv;

  visible = visible != FALSE;

  if (priv->visible != visible)
    {
      priv->visible = visible;

      g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_VISIBILITY]);
      ctk_entry_recompute (entry);
    }
}

/**
 * ctk_entry_get_visibility:
 * @entry: a #CtkEntry
 *
 * Retrieves whether the text in @entry is visible. See
 * ctk_entry_set_visibility().
 *
 * Returns: %TRUE if the text is currently visible
 **/
gboolean
ctk_entry_get_visibility (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), FALSE);

  return entry->priv->visible;
}

/**
 * ctk_entry_set_invisible_char:
 * @entry: a #CtkEntry
 * @ch: a Unicode character
 * 
 * Sets the character to use in place of the actual text when
 * ctk_entry_set_visibility() has been called to set text visibility
 * to %FALSE. i.e. this is the character used in “password mode” to
 * show the user how many characters have been typed. By default, CTK+
 * picks the best invisible char available in the current font. If you
 * set the invisible char to 0, then the user will get no feedback
 * at all; there will be no text on the screen as they type.
 **/
void
ctk_entry_set_invisible_char (CtkEntry *entry,
                              gunichar  ch)
{
  CtkEntryPrivate *priv;

  g_return_if_fail (CTK_IS_ENTRY (entry));

  priv = entry->priv;

  if (!priv->invisible_char_set)
    {
      priv->invisible_char_set = TRUE;
      g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_INVISIBLE_CHAR_SET]);
    }

  if (ch == priv->invisible_char)
    return;

  priv->invisible_char = ch;
  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_INVISIBLE_CHAR]);
  ctk_entry_recompute (entry);
}

/**
 * ctk_entry_get_invisible_char:
 * @entry: a #CtkEntry
 *
 * Retrieves the character displayed in place of the real characters
 * for entries with visibility set to false. See ctk_entry_set_invisible_char().
 *
 * Returns: the current invisible char, or 0, if the entry does not
 *               show invisible text at all. 
 **/
gunichar
ctk_entry_get_invisible_char (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), 0);

  return entry->priv->invisible_char;
}

/**
 * ctk_entry_unset_invisible_char:
 * @entry: a #CtkEntry
 *
 * Unsets the invisible char previously set with
 * ctk_entry_set_invisible_char(). So that the
 * default invisible char is used again.
 *
 * Since: 2.16
 **/
void
ctk_entry_unset_invisible_char (CtkEntry *entry)
{
  CtkEntryPrivate *priv;
  gunichar ch;

  g_return_if_fail (CTK_IS_ENTRY (entry));

  priv = entry->priv;

  if (!priv->invisible_char_set)
    return;

  priv->invisible_char_set = FALSE;
  ch = find_invisible_char (CTK_WIDGET (entry));

  if (priv->invisible_char != ch)
    {
      priv->invisible_char = ch;
      g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_INVISIBLE_CHAR]);
    }

  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_INVISIBLE_CHAR_SET]);
  ctk_entry_recompute (entry);
}

/**
 * ctk_entry_set_overwrite_mode:
 * @entry: a #CtkEntry
 * @overwrite: new value
 *
 * Sets whether the text is overwritten when typing in the #CtkEntry.
 *
 * Since: 2.14
 **/
void
ctk_entry_set_overwrite_mode (CtkEntry *entry,
                              gboolean  overwrite)
{
  CtkEntryPrivate *priv = entry->priv;

  g_return_if_fail (CTK_IS_ENTRY (entry));

  if (priv->overwrite_mode == overwrite)
    return;

  ctk_entry_toggle_overwrite (entry);

  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_OVERWRITE_MODE]);
}

/**
 * ctk_entry_get_overwrite_mode:
 * @entry: a #CtkEntry
 *
 * Gets the value set by ctk_entry_set_overwrite_mode().
 *
 * Returns: whether the text is overwritten when typing.
 *
 * Since: 2.14
 **/
gboolean
ctk_entry_get_overwrite_mode (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), FALSE);

  return entry->priv->overwrite_mode;
}

/**
 * ctk_entry_get_text:
 * @entry: a #CtkEntry
 *
 * Retrieves the contents of the entry widget.
 * See also ctk_editable_get_chars().
 *
 * This is equivalent to getting @entry's #CtkEntryBuffer and calling
 * ctk_entry_buffer_get_text() on it.
 *
 * Returns: a pointer to the contents of the widget as a
 *      string. This string points to internally allocated
 *      storage in the widget and must not be freed, modified or
 *      stored.
 **/
const gchar*
ctk_entry_get_text (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), NULL);

  return ctk_entry_buffer_get_text (get_buffer (entry));
}

/**
 * ctk_entry_set_max_length:
 * @entry: a #CtkEntry
 * @max: the maximum length of the entry, or 0 for no maximum.
 *   (other than the maximum length of entries.) The value passed in will
 *   be clamped to the range 0-65536.
 *
 * Sets the maximum allowed length of the contents of the widget. If
 * the current contents are longer than the given length, then they
 * will be truncated to fit.
 *
 * This is equivalent to getting @entry's #CtkEntryBuffer and
 * calling ctk_entry_buffer_set_max_length() on it.
 * ]|
 **/
void
ctk_entry_set_max_length (CtkEntry     *entry,
                          gint          max)
{
  g_return_if_fail (CTK_IS_ENTRY (entry));
  ctk_entry_buffer_set_max_length (get_buffer (entry), max);
}

/**
 * ctk_entry_get_max_length:
 * @entry: a #CtkEntry
 *
 * Retrieves the maximum allowed length of the text in
 * @entry. See ctk_entry_set_max_length().
 *
 * This is equivalent to getting @entry's #CtkEntryBuffer and
 * calling ctk_entry_buffer_get_max_length() on it.
 *
 * Returns: the maximum allowed number of characters
 *               in #CtkEntry, or 0 if there is no maximum.
 **/
gint
ctk_entry_get_max_length (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), 0);

  return ctk_entry_buffer_get_max_length (get_buffer (entry));
}

/**
 * ctk_entry_get_text_length:
 * @entry: a #CtkEntry
 *
 * Retrieves the current length of the text in
 * @entry. 
 *
 * This is equivalent to getting @entry's #CtkEntryBuffer and
 * calling ctk_entry_buffer_get_length() on it.

 *
 * Returns: the current number of characters
 *               in #CtkEntry, or 0 if there are none.
 *
 * Since: 2.14
 **/
guint16
ctk_entry_get_text_length (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), 0);

  return ctk_entry_buffer_get_length (get_buffer (entry));
}

/**
 * ctk_entry_set_activates_default:
 * @entry: a #CtkEntry
 * @setting: %TRUE to activate window’s default widget on Enter keypress
 *
 * If @setting is %TRUE, pressing Enter in the @entry will activate the default
 * widget for the window containing the entry. This usually means that
 * the dialog box containing the entry will be closed, since the default
 * widget is usually one of the dialog buttons.
 *
 * (For experts: if @setting is %TRUE, the entry calls
 * ctk_window_activate_default() on the window containing the entry, in
 * the default handler for the #CtkEntry::activate signal.)
 **/
void
ctk_entry_set_activates_default (CtkEntry *entry,
                                 gboolean  setting)
{
  CtkEntryPrivate *priv;

  g_return_if_fail (CTK_IS_ENTRY (entry));

  priv = entry->priv;

  setting = setting != FALSE;

  if (setting != priv->activates_default)
    {
      priv->activates_default = setting;
      g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_ACTIVATES_DEFAULT]);
    }
}

/**
 * ctk_entry_get_activates_default:
 * @entry: a #CtkEntry
 *
 * Retrieves the value set by ctk_entry_set_activates_default().
 *
 * Returns: %TRUE if the entry will activate the default widget
 */
gboolean
ctk_entry_get_activates_default (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), FALSE);

  return entry->priv->activates_default;
}

/**
 * ctk_entry_set_width_chars:
 * @entry: a #CtkEntry
 * @n_chars: width in chars
 *
 * Changes the size request of the entry to be about the right size
 * for @n_chars characters. Note that it changes the size
 * request, the size can still be affected by
 * how you pack the widget into containers. If @n_chars is -1, the
 * size reverts to the default entry size.
 **/
void
ctk_entry_set_width_chars (CtkEntry *entry,
                           gint      n_chars)
{
  CtkEntryPrivate *priv;

  g_return_if_fail (CTK_IS_ENTRY (entry));

  priv = entry->priv;

  if (priv->width_chars != n_chars)
    {
      priv->width_chars = n_chars;
      g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_WIDTH_CHARS]);
      ctk_widget_queue_resize (CTK_WIDGET (entry));
    }
}

/**
 * ctk_entry_get_width_chars:
 * @entry: a #CtkEntry
 * 
 * Gets the value set by ctk_entry_set_width_chars().
 * 
 * Returns: number of chars to request space for, or negative if unset
 **/
gint
ctk_entry_get_width_chars (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), 0);

  return entry->priv->width_chars;
}

/**
 * ctk_entry_set_max_width_chars:
 * @entry: a #CtkEntry
 * @n_chars: the new desired maximum width, in characters
 *
 * Sets the desired maximum width in characters of @entry.
 *
 * Since: 3.12
 */
void
ctk_entry_set_max_width_chars (CtkEntry *entry,
                               gint      n_chars)
{
  CtkEntryPrivate *priv;

  g_return_if_fail (CTK_IS_ENTRY (entry));

  priv = entry->priv;

  if (priv->max_width_chars != n_chars)
    {
      priv->max_width_chars = n_chars;
      g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_MAX_WIDTH_CHARS]);
      ctk_widget_queue_resize (CTK_WIDGET (entry));
    }
}

/**
 * ctk_entry_get_max_width_chars:
 * @entry: a #CtkEntry
 *
 * Retrieves the desired maximum width of @entry, in characters.
 * See ctk_entry_set_max_width_chars().
 *
 * Returns: the maximum width of the entry, in characters
 *
 * Since: 3.12
 */
gint
ctk_entry_get_max_width_chars (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), 0);

  return entry->priv->max_width_chars;
}

/**
 * ctk_entry_set_has_frame:
 * @entry: a #CtkEntry
 * @setting: new value
 * 
 * Sets whether the entry has a beveled frame around it.
 **/
void
ctk_entry_set_has_frame (CtkEntry *entry,
                         gboolean  setting)
{
  CtkStyleContext *context;

  g_return_if_fail (CTK_IS_ENTRY (entry));

  setting = (setting != FALSE);

  if (setting == ctk_entry_get_has_frame (entry))
    return;

  context = ctk_widget_get_style_context (CTK_WIDGET (entry));
  if (setting)
    ctk_style_context_remove_class (context, CTK_STYLE_CLASS_FLAT);
  else
    ctk_style_context_add_class (context, CTK_STYLE_CLASS_FLAT);
  ctk_widget_queue_draw (CTK_WIDGET (entry));
  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_HAS_FRAME]);
}

/**
 * ctk_entry_get_has_frame:
 * @entry: a #CtkEntry
 * 
 * Gets the value set by ctk_entry_set_has_frame().
 * 
 * Returns: whether the entry has a beveled frame
 **/
gboolean
ctk_entry_get_has_frame (CtkEntry *entry)
{
  CtkStyleContext *context;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), FALSE);

  context = ctk_widget_get_style_context (CTK_WIDGET (entry));

  return !ctk_style_context_has_class (context, CTK_STYLE_CLASS_FLAT);
}

/**
 * ctk_entry_set_inner_border:
 * @entry: a #CtkEntry
 * @border: (allow-none): a #CtkBorder, or %NULL
 *
 * Sets %entry’s inner-border property to @border, or clears it if %NULL
 * is passed. The inner-border is the area around the entry’s text, but
 * inside its frame.
 *
 * If set, this property overrides the inner-border style property.
 * Overriding the style-provided border is useful when you want to do
 * in-place editing of some text in a canvas or list widget, where
 * pixel-exact positioning of the entry is important.
 *
 * Since: 2.10
 *
 * Deprecated: 3.4: Use the standard border and padding CSS properties (through
 *   objects like #CtkStyleContext and #CtkCssProvider); the value set with
 *   this function is ignored by #CtkEntry.
 **/
void
ctk_entry_set_inner_border (CtkEntry        *entry,
                            const CtkBorder *border)
{
  g_return_if_fail (CTK_IS_ENTRY (entry));

  ctk_entry_do_set_inner_border (entry, border);
}

/**
 * ctk_entry_get_inner_border:
 * @entry: a #CtkEntry
 *
 * This function returns the entry’s #CtkEntry:inner-border property. See
 * ctk_entry_set_inner_border() for more information.
 *
 * Returns: (nullable) (transfer none): the entry’s #CtkBorder, or
 *   %NULL if none was set.
 *
 * Since: 2.10
 *
 * Deprecated: 3.4: Use the standard border and padding CSS properties (through
 *   objects like #CtkStyleContext and #CtkCssProvider); the value returned by
 *   this function is ignored by #CtkEntry.
 **/
const CtkBorder *
ctk_entry_get_inner_border (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), NULL);

  return ctk_entry_do_get_inner_border (entry);
}

/**
 * ctk_entry_get_layout:
 * @entry: a #CtkEntry
 * 
 * Gets the #PangoLayout used to display the entry.
 * The layout is useful to e.g. convert text positions to
 * pixel positions, in combination with ctk_entry_get_layout_offsets().
 * The returned layout is owned by the entry and must not be 
 * modified or freed by the caller.
 *
 * Keep in mind that the layout text may contain a preedit string, so
 * ctk_entry_layout_index_to_text_index() and
 * ctk_entry_text_index_to_layout_index() are needed to convert byte
 * indices in the layout to byte indices in the entry contents.
 *
 * Returns: (transfer none): the #PangoLayout for this entry
 **/
PangoLayout*
ctk_entry_get_layout (CtkEntry *entry)
{
  PangoLayout *layout;
  
  g_return_val_if_fail (CTK_IS_ENTRY (entry), NULL);

  layout = ctk_entry_ensure_layout (entry, TRUE);

  return layout;
}


/**
 * ctk_entry_layout_index_to_text_index:
 * @entry: a #CtkEntry
 * @layout_index: byte index into the entry layout text
 *
 * Converts from a position in the entry’s #PangoLayout (returned by
 * ctk_entry_get_layout()) to a position in the entry contents
 * (returned by ctk_entry_get_text()).
 *
 * Returns: byte index into the entry contents
 **/
gint
ctk_entry_layout_index_to_text_index (CtkEntry *entry,
                                      gint      layout_index)
{
  CtkEntryPrivate *priv;
  PangoLayout *layout;
  const gchar *text;
  gint cursor_index;
  
  g_return_val_if_fail (CTK_IS_ENTRY (entry), 0);

  priv = entry->priv;

  layout = ctk_entry_ensure_layout (entry, TRUE);
  text = pango_layout_get_text (layout);
  cursor_index = g_utf8_offset_to_pointer (text, priv->current_pos) - text;

  if (layout_index >= cursor_index && priv->preedit_length)
    {
      if (layout_index >= cursor_index + priv->preedit_length)
	layout_index -= priv->preedit_length;
      else
        layout_index = cursor_index;
    }

  return layout_index;
}

/**
 * ctk_entry_text_index_to_layout_index:
 * @entry: a #CtkEntry
 * @text_index: byte index into the entry contents
 *
 * Converts from a position in the entry contents (returned
 * by ctk_entry_get_text()) to a position in the
 * entry’s #PangoLayout (returned by ctk_entry_get_layout(),
 * with text retrieved via pango_layout_get_text()).
 *
 * Returns: byte index into the entry layout text
 **/
gint
ctk_entry_text_index_to_layout_index (CtkEntry *entry,
                                      gint      text_index)
{
  CtkEntryPrivate *priv;
  PangoLayout *layout;
  const gchar *text;
  gint cursor_index;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), 0);

  priv = entry->priv;

  layout = ctk_entry_ensure_layout (entry, TRUE);
  text = pango_layout_get_text (layout);
  cursor_index = g_utf8_offset_to_pointer (text, priv->current_pos) - text;

  if (text_index > cursor_index)
    text_index += priv->preedit_length;

  return text_index;
}

/**
 * ctk_entry_get_layout_offsets:
 * @entry: a #CtkEntry
 * @x: (out) (allow-none): location to store X offset of layout, or %NULL
 * @y: (out) (allow-none): location to store Y offset of layout, or %NULL
 *
 *
 * Obtains the position of the #PangoLayout used to render text
 * in the entry, in widget coordinates. Useful if you want to line
 * up the text in an entry with some other text, e.g. when using the
 * entry to implement editable cells in a sheet widget.
 *
 * Also useful to convert mouse events into coordinates inside the
 * #PangoLayout, e.g. to take some action if some part of the entry text
 * is clicked.
 * 
 * Note that as the user scrolls around in the entry the offsets will
 * change; you’ll need to connect to the “notify::scroll-offset”
 * signal to track this. Remember when using the #PangoLayout
 * functions you need to convert to and from pixels using
 * PANGO_PIXELS() or #PANGO_SCALE.
 *
 * Keep in mind that the layout text may contain a preedit string, so
 * ctk_entry_layout_index_to_text_index() and
 * ctk_entry_text_index_to_layout_index() are needed to convert byte
 * indices in the layout to byte indices in the entry contents.
 **/
void
ctk_entry_get_layout_offsets (CtkEntry *entry,
                              gint     *x,
                              gint     *y)
{
  CtkEntryPrivate *priv;
  CtkAllocation allocation;

  g_return_if_fail (CTK_IS_ENTRY (entry));

  priv = entry->priv;
  ctk_widget_get_allocation (CTK_WIDGET (entry), &allocation);

  /* this gets coords relative to text area */
  get_layout_position (entry, x, y);

  /* convert to widget coords */
  if (x)
    *x += priv->text_allocation.x - allocation.x;

  if (y)
    *y += priv->text_allocation.y - allocation.y;
}


/**
 * ctk_entry_set_alignment:
 * @entry: a #CtkEntry
 * @xalign: The horizontal alignment, from 0 (left) to 1 (right).
 *          Reversed for RTL layouts
 * 
 * Sets the alignment for the contents of the entry. This controls
 * the horizontal positioning of the contents when the displayed
 * text is shorter than the width of the entry.
 *
 * Since: 2.4
 **/
void
ctk_entry_set_alignment (CtkEntry *entry, gfloat xalign)
{
  CtkEntryPrivate *priv;

  g_return_if_fail (CTK_IS_ENTRY (entry));

  priv = entry->priv;

  if (xalign < 0.0)
    xalign = 0.0;
  else if (xalign > 1.0)
    xalign = 1.0;

  if (xalign != priv->xalign)
    {
      priv->xalign = xalign;
      ctk_entry_recompute (entry);
      g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_XALIGN]);
    }
}

/**
 * ctk_entry_get_alignment:
 * @entry: a #CtkEntry
 *
 * Gets the value set by ctk_entry_set_alignment().
 *
 * Returns: the alignment
 *
 * Since: 2.4
 **/
gfloat
ctk_entry_get_alignment (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), 0.0);

  return entry->priv->xalign;
}

/**
 * ctk_entry_set_icon_from_pixbuf:
 * @entry: a #CtkEntry
 * @icon_pos: Icon position
 * @pixbuf: (allow-none): A #GdkPixbuf, or %NULL
 *
 * Sets the icon shown in the specified position using a pixbuf.
 *
 * If @pixbuf is %NULL, no icon will be shown in the specified position.
 *
 * Since: 2.16
 */
void
ctk_entry_set_icon_from_pixbuf (CtkEntry             *entry,
                                CtkEntryIconPosition  icon_pos,
                                GdkPixbuf            *pixbuf)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (CTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = entry->priv;

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (CTK_WIDGET (entry), icon_pos);

  g_object_freeze_notify (G_OBJECT (entry));

  if (pixbuf)
    g_object_ref (pixbuf);

  if (pixbuf)
    {
      _ctk_icon_helper_set_pixbuf (CTK_ICON_HELPER (icon_info->gadget), pixbuf);
      _ctk_icon_helper_set_icon_size (CTK_ICON_HELPER (icon_info->gadget),
                                      CTK_ICON_SIZE_MENU);

      if (icon_pos == CTK_ENTRY_ICON_PRIMARY)
        {
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_PIXBUF_PRIMARY]);
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STORAGE_TYPE_PRIMARY]);
        }
      else
        {
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_PIXBUF_SECONDARY]);
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STORAGE_TYPE_SECONDARY]);
        }

      if (ctk_widget_get_mapped (CTK_WIDGET (entry)))
          gdk_window_show_unraised (icon_info->window);

      g_object_unref (pixbuf);
    }
  else
    ctk_entry_clear_icon (entry, icon_pos);

  if (ctk_widget_get_visible (CTK_WIDGET (entry)))
    ctk_widget_queue_resize (CTK_WIDGET (entry));

  g_object_thaw_notify (G_OBJECT (entry));
}

/**
 * ctk_entry_set_icon_from_stock:
 * @entry: A #CtkEntry
 * @icon_pos: Icon position
 * @stock_id: (allow-none): The name of the stock item, or %NULL
 *
 * Sets the icon shown in the entry at the specified position from
 * a stock image.
 *
 * If @stock_id is %NULL, no icon will be shown in the specified position.
 *
 * Since: 2.16
 *
 * Deprecated: 3.10: Use ctk_entry_set_icon_from_icon_name() instead.
 */
void
ctk_entry_set_icon_from_stock (CtkEntry             *entry,
                               CtkEntryIconPosition  icon_pos,
                               const gchar          *stock_id)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (CTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = entry->priv;

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (CTK_WIDGET (entry), icon_pos);

  g_object_freeze_notify (G_OBJECT (entry));

  if (stock_id != NULL)
    {
      _ctk_icon_helper_set_stock_id (CTK_ICON_HELPER (icon_info->gadget), stock_id, CTK_ICON_SIZE_MENU);

      if (icon_pos == CTK_ENTRY_ICON_PRIMARY)
        {
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STOCK_PRIMARY]);
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STORAGE_TYPE_PRIMARY]);
        }
      else
        {
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STOCK_SECONDARY]);
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STORAGE_TYPE_SECONDARY]);
        }

      if (ctk_widget_get_mapped (CTK_WIDGET (entry)))
          gdk_window_show_unraised (icon_info->window);
    }
  else
    ctk_entry_clear_icon (entry, icon_pos);

  if (ctk_widget_get_visible (CTK_WIDGET (entry)))
    ctk_widget_queue_resize (CTK_WIDGET (entry));

  g_object_thaw_notify (G_OBJECT (entry));
}

/**
 * ctk_entry_set_icon_from_icon_name:
 * @entry: A #CtkEntry
 * @icon_pos: The position at which to set the icon
 * @icon_name: (allow-none): An icon name, or %NULL
 *
 * Sets the icon shown in the entry at the specified position
 * from the current icon theme.
 *
 * If the icon name isn’t known, a “broken image” icon will be displayed
 * instead.
 *
 * If @icon_name is %NULL, no icon will be shown in the specified position.
 *
 * Since: 2.16
 */
void
ctk_entry_set_icon_from_icon_name (CtkEntry             *entry,
                                   CtkEntryIconPosition  icon_pos,
                                   const gchar          *icon_name)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (CTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = entry->priv;

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (CTK_WIDGET (entry), icon_pos);

  g_object_freeze_notify (G_OBJECT (entry));


  if (icon_name != NULL)
    {
      _ctk_icon_helper_set_icon_name (CTK_ICON_HELPER (icon_info->gadget), icon_name, CTK_ICON_SIZE_MENU);

      if (icon_pos == CTK_ENTRY_ICON_PRIMARY)
        {
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_ICON_NAME_PRIMARY]);
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STORAGE_TYPE_PRIMARY]);
        }
      else
        {
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_ICON_NAME_SECONDARY]);
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STORAGE_TYPE_SECONDARY]);
        }

      if (ctk_widget_get_mapped (CTK_WIDGET (entry)))
          gdk_window_show_unraised (icon_info->window);
    }
  else
    ctk_entry_clear_icon (entry, icon_pos);

  if (ctk_widget_get_visible (CTK_WIDGET (entry)))
    ctk_widget_queue_resize (CTK_WIDGET (entry));

  g_object_thaw_notify (G_OBJECT (entry));
}

/**
 * ctk_entry_set_icon_from_gicon:
 * @entry: A #CtkEntry
 * @icon_pos: The position at which to set the icon
 * @icon: (allow-none): The icon to set, or %NULL
 *
 * Sets the icon shown in the entry at the specified position
 * from the current icon theme.
 * If the icon isn’t known, a “broken image” icon will be displayed
 * instead.
 *
 * If @icon is %NULL, no icon will be shown in the specified position.
 *
 * Since: 2.16
 */
void
ctk_entry_set_icon_from_gicon (CtkEntry             *entry,
                               CtkEntryIconPosition  icon_pos,
                               GIcon                *icon)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (CTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = entry->priv;

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (CTK_WIDGET (entry), icon_pos);

  g_object_freeze_notify (G_OBJECT (entry));

  if (icon)
    {
      _ctk_icon_helper_set_gicon (CTK_ICON_HELPER (icon_info->gadget), icon, CTK_ICON_SIZE_MENU);

      if (icon_pos == CTK_ENTRY_ICON_PRIMARY)
        {
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_GICON_PRIMARY]);
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STORAGE_TYPE_PRIMARY]);
        }
      else
        {
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_GICON_SECONDARY]);
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STORAGE_TYPE_SECONDARY]);
        }

      if (ctk_widget_get_mapped (CTK_WIDGET (entry)))
          gdk_window_show_unraised (icon_info->window);
    }
  else
    ctk_entry_clear_icon (entry, icon_pos);

  if (ctk_widget_get_visible (CTK_WIDGET (entry)))
    ctk_widget_queue_resize (CTK_WIDGET (entry));

  g_object_thaw_notify (G_OBJECT (entry));
}

/**
 * ctk_entry_set_icon_activatable:
 * @entry: A #CtkEntry
 * @icon_pos: Icon position
 * @activatable: %TRUE if the icon should be activatable
 *
 * Sets whether the icon is activatable.
 *
 * Since: 2.16
 */
void
ctk_entry_set_icon_activatable (CtkEntry             *entry,
                                CtkEntryIconPosition  icon_pos,
                                gboolean              activatable)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (CTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = entry->priv;

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (CTK_WIDGET (entry), icon_pos);

  activatable = activatable != FALSE;

  if (icon_info->nonactivatable != !activatable)
    {
      icon_info->nonactivatable = !activatable;

      if (ctk_widget_get_realized (CTK_WIDGET (entry)))
        update_cursors (CTK_WIDGET (entry));

      g_object_notify_by_pspec (G_OBJECT (entry),
                                entry_props[icon_pos == CTK_ENTRY_ICON_PRIMARY
                                            ? PROP_ACTIVATABLE_PRIMARY
                                            : PROP_ACTIVATABLE_SECONDARY]);
    }
}

/**
 * ctk_entry_get_icon_activatable:
 * @entry: a #CtkEntry
 * @icon_pos: Icon position
 *
 * Returns whether the icon is activatable.
 *
 * Returns: %TRUE if the icon is activatable.
 *
 * Since: 2.16
 */
gboolean
ctk_entry_get_icon_activatable (CtkEntry             *entry,
                                CtkEntryIconPosition  icon_pos)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), FALSE);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), FALSE);

  priv = entry->priv;
  icon_info = priv->icons[icon_pos];

  return (!icon_info || !icon_info->nonactivatable);
}

/**
 * ctk_entry_get_icon_pixbuf:
 * @entry: A #CtkEntry
 * @icon_pos: Icon position
 *
 * Retrieves the image used for the icon.
 *
 * Unlike the other methods of setting and getting icon data, this
 * method will work regardless of whether the icon was set using a
 * #GdkPixbuf, a #GIcon, a stock item, or an icon name.
 *
 * Returns: (transfer none) (nullable): A #GdkPixbuf, or %NULL if no icon is
 *     set for this position.
 *
 * Since: 2.16
 */
GdkPixbuf *
ctk_entry_get_icon_pixbuf (CtkEntry             *entry,
                           CtkEntryIconPosition  icon_pos)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;
  cairo_surface_t *surface;
  GdkPixbuf *pixbuf;
  int width, height;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  priv = entry->priv;

  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;

  _ctk_icon_helper_get_size (CTK_ICON_HELPER (icon_info->gadget), &width, &height);
  surface = ctk_icon_helper_load_surface (CTK_ICON_HELPER (icon_info->gadget), 1);

  pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0, width, height);

  cairo_surface_destroy (surface);

  /* HACK: unfortunately this is transfer none, so we attach it somehwere
   * convenient.
   */
  if (pixbuf)
    {
      g_object_set_data_full (G_OBJECT (icon_info->gadget),
                              "ctk-entry-pixbuf",
                              pixbuf,
                              g_object_unref);
    }

  return pixbuf;
}

/**
 * ctk_entry_get_icon_gicon:
 * @entry: A #CtkEntry
 * @icon_pos: Icon position
 *
 * Retrieves the #GIcon used for the icon, or %NULL if there is
 * no icon or if the icon was set by some other method (e.g., by
 * stock, pixbuf, or icon name).
 *
 * Returns: (transfer none) (nullable): A #GIcon, or %NULL if no icon is set
 *     or if the icon is not a #GIcon
 *
 * Since: 2.16
 */
GIcon *
ctk_entry_get_icon_gicon (CtkEntry             *entry,
                          CtkEntryIconPosition  icon_pos)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  priv = entry->priv;
  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;

  return _ctk_icon_helper_peek_gicon (CTK_ICON_HELPER (icon_info->gadget));
}

/**
 * ctk_entry_get_icon_stock:
 * @entry: A #CtkEntry
 * @icon_pos: Icon position
 *
 * Retrieves the stock id used for the icon, or %NULL if there is
 * no icon or if the icon was set by some other method (e.g., by
 * pixbuf, icon name or gicon).
 *
 * Returns: A stock id, or %NULL if no icon is set or if the icon
 *          wasn’t set from a stock id
 *
 * Since: 2.16
 *
 * Deprecated: 3.10: Use ctk_entry_get_icon_name() instead.
 */
const gchar *
ctk_entry_get_icon_stock (CtkEntry             *entry,
                          CtkEntryIconPosition  icon_pos)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  priv = entry->priv;
  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;

  return _ctk_icon_helper_get_stock_id (CTK_ICON_HELPER (icon_info->gadget));
}

/**
 * ctk_entry_get_icon_name:
 * @entry: A #CtkEntry
 * @icon_pos: Icon position
 *
 * Retrieves the icon name used for the icon, or %NULL if there is
 * no icon or if the icon was set by some other method (e.g., by
 * pixbuf, stock or gicon).
 *
 * Returns: (nullable): An icon name, or %NULL if no icon is set or if the icon
 *          wasn’t set from an icon name
 *
 * Since: 2.16
 */
const gchar *
ctk_entry_get_icon_name (CtkEntry             *entry,
                         CtkEntryIconPosition  icon_pos)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  priv = entry->priv;
  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;

  return _ctk_icon_helper_get_icon_name (CTK_ICON_HELPER (icon_info->gadget));
}

/**
 * ctk_entry_set_icon_sensitive:
 * @entry: A #CtkEntry
 * @icon_pos: Icon position
 * @sensitive: Specifies whether the icon should appear
 *             sensitive or insensitive
 *
 * Sets the sensitivity for the specified icon.
 *
 * Since: 2.16
 */
void
ctk_entry_set_icon_sensitive (CtkEntry             *entry,
                              CtkEntryIconPosition  icon_pos,
                              gboolean              sensitive)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (CTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = entry->priv;

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (CTK_WIDGET (entry), icon_pos);

  if (icon_info->insensitive != !sensitive)
    {
      icon_info->insensitive = !sensitive;

      icon_info->pressed = FALSE;
      icon_info->prelight = FALSE;

      if (ctk_widget_get_realized (CTK_WIDGET (entry)))
        update_cursors (CTK_WIDGET (entry));

      update_icon_state (CTK_WIDGET (entry), icon_pos);

      g_object_notify_by_pspec (G_OBJECT (entry),
                                entry_props[icon_pos == CTK_ENTRY_ICON_PRIMARY
                                            ? PROP_SENSITIVE_PRIMARY
                                            : PROP_SENSITIVE_SECONDARY]);
    }
}

/**
 * ctk_entry_get_icon_sensitive:
 * @entry: a #CtkEntry
 * @icon_pos: Icon position
 *
 * Returns whether the icon appears sensitive or insensitive.
 *
 * Returns: %TRUE if the icon is sensitive.
 *
 * Since: 2.16
 */
gboolean
ctk_entry_get_icon_sensitive (CtkEntry             *entry,
                              CtkEntryIconPosition  icon_pos)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), TRUE);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), TRUE);

  priv = entry->priv;

  icon_info = priv->icons[icon_pos];

  return (!icon_info || !icon_info->insensitive);

}

/**
 * ctk_entry_get_icon_storage_type:
 * @entry: a #CtkEntry
 * @icon_pos: Icon position
 *
 * Gets the type of representation being used by the icon
 * to store image data. If the icon has no image data,
 * the return value will be %CTK_IMAGE_EMPTY.
 *
 * Returns: image representation being used
 *
 * Since: 2.16
 **/
CtkImageType
ctk_entry_get_icon_storage_type (CtkEntry             *entry,
                                 CtkEntryIconPosition  icon_pos)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), CTK_IMAGE_EMPTY);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), CTK_IMAGE_EMPTY);

  priv = entry->priv;

  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return CTK_IMAGE_EMPTY;

  return _ctk_icon_helper_get_storage_type (CTK_ICON_HELPER (icon_info->gadget));
}

/**
 * ctk_entry_get_icon_at_pos:
 * @entry: a #CtkEntry
 * @x: the x coordinate of the position to find
 * @y: the y coordinate of the position to find
 *
 * Finds the icon at the given position and return its index. The
 * position’s coordinates are relative to the @entry’s top left corner.
 * If @x, @y doesn’t lie inside an icon, -1 is returned.
 * This function is intended for use in a #CtkWidget::query-tooltip
 * signal handler.
 *
 * Returns: the index of the icon at the given position, or -1
 *
 * Since: 2.16
 */
gint
ctk_entry_get_icon_at_pos (CtkEntry *entry,
                           gint      x,
                           gint      y)
{
  CtkEntryPrivate *priv;
  guint i;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), -1);

  priv = entry->priv;

  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];

      if (icon_info == NULL)
        continue;

      if (ctk_css_gadget_border_box_contains_point (icon_info->gadget, x, y))
        return i;
    }

  return -1;
}

/**
 * ctk_entry_set_icon_drag_source:
 * @entry: a #CtkEntry
 * @icon_pos: icon position
 * @target_list: the targets (data formats) in which the data can be provided
 * @actions: a bitmask of the allowed drag actions
 *
 * Sets up the icon at the given position so that CTK+ will start a drag
 * operation when the user clicks and drags the icon.
 *
 * To handle the drag operation, you need to connect to the usual
 * #CtkWidget::drag-data-get (or possibly #CtkWidget::drag-data-delete)
 * signal, and use ctk_entry_get_current_icon_drag_source() in
 * your signal handler to find out if the drag was started from
 * an icon.
 *
 * By default, CTK+ uses the icon as the drag icon. You can use the 
 * #CtkWidget::drag-begin signal to set a different icon. Note that you 
 * have to use g_signal_connect_after() to ensure that your signal handler
 * gets executed after the default handler.
 *
 * Since: 2.16
 */
void
ctk_entry_set_icon_drag_source (CtkEntry             *entry,
                                CtkEntryIconPosition  icon_pos,
                                CtkTargetList        *target_list,
                                GdkDragAction         actions)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (CTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = entry->priv;

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (CTK_WIDGET (entry), icon_pos);

  if (icon_info->target_list)
    ctk_target_list_unref (icon_info->target_list);
  icon_info->target_list = target_list;
  if (icon_info->target_list)
    ctk_target_list_ref (icon_info->target_list);

  icon_info->actions = actions;
}

/**
 * ctk_entry_get_current_icon_drag_source:
 * @entry: a #CtkEntry
 *
 * Returns the index of the icon which is the source of the current
 * DND operation, or -1.
 *
 * This function is meant to be used in a #CtkWidget::drag-data-get
 * callback.
 *
 * Returns: index of the icon which is the source of the current
 *          DND operation, or -1.
 *
 * Since: 2.16
 */
gint
ctk_entry_get_current_icon_drag_source (CtkEntry *entry)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info = NULL;
  gint i;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), -1);

  priv = entry->priv;

  for (i = 0; i < MAX_ICONS; i++)
    {
      if ((icon_info = priv->icons[i]))
        {
          if (icon_info->in_drag)
            return i;
        }
    }

  return -1;
}

/**
 * ctk_entry_get_icon_area:
 * @entry: A #CtkEntry
 * @icon_pos: Icon position
 * @icon_area: (out): Return location for the icon’s area
 *
 * Gets the area where entry’s icon at @icon_pos is drawn.
 * This function is useful when drawing something to the
 * entry in a draw callback.
 *
 * If the entry is not realized or has no icon at the given position,
 * @icon_area is filled with zeros. Otherwise, @icon_area will be filled
 * with the icon’s allocation, relative to @entry’s allocation.
 *
 * See also ctk_entry_get_text_area()
 *
 * Since: 3.0
 */
void
ctk_entry_get_icon_area (CtkEntry             *entry,
                         CtkEntryIconPosition  icon_pos,
                         GdkRectangle         *icon_area)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (CTK_IS_ENTRY (entry));
  g_return_if_fail (icon_area != NULL);

  priv = entry->priv;

  icon_info = priv->icons[icon_pos];

  if (icon_info)
    {
      CtkAllocation widget_allocation;
      ctk_widget_get_allocation (CTK_WIDGET (entry), &widget_allocation);
      ctk_css_gadget_get_border_allocation (icon_info->gadget, icon_area, NULL);
      icon_area->x -= widget_allocation.x;
      icon_area->y -= widget_allocation.y;
    }
  else
    {
      icon_area->x = 0;
      icon_area->y = 0;
      icon_area->width = 0;
      icon_area->height = 0;
    }
}

static void
ensure_has_tooltip (CtkEntry *entry)
{
  gchar *text = ctk_widget_get_tooltip_text (CTK_WIDGET (entry));
  gboolean has_tooltip = text != NULL;

  if (!has_tooltip)
    {
      CtkEntryPrivate *priv = entry->priv;
      int i;

      for (i = 0; i < MAX_ICONS; i++)
        {
          EntryIconInfo *icon_info = priv->icons[i];

          if (icon_info != NULL && icon_info->tooltip != NULL)
            {
              has_tooltip = TRUE;
              break;
            }
        }
    }
  else
    {
      g_free (text);
    }

  ctk_widget_set_has_tooltip (CTK_WIDGET (entry), has_tooltip);
}

/**
 * ctk_entry_get_icon_tooltip_text:
 * @entry: a #CtkEntry
 * @icon_pos: the icon position
 *
 * Gets the contents of the tooltip on the icon at the specified 
 * position in @entry.
 * 
 * Returns: (nullable): the tooltip text, or %NULL. Free the returned
 *     string with g_free() when done.
 * 
 * Since: 2.16
 */
gchar *
ctk_entry_get_icon_tooltip_text (CtkEntry             *entry,
                                 CtkEntryIconPosition  icon_pos)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;
  gchar *text = NULL;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  priv = entry->priv;

  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;
 
  if (icon_info->tooltip && 
      !pango_parse_markup (icon_info->tooltip, -1, 0, NULL, &text, NULL, NULL))
    g_assert (NULL == text); /* text should still be NULL in case of markup errors */

  return text;
}

/**
 * ctk_entry_set_icon_tooltip_text:
 * @entry: a #CtkEntry
 * @icon_pos: the icon position
 * @tooltip: (allow-none): the contents of the tooltip for the icon, or %NULL
 *
 * Sets @tooltip as the contents of the tooltip for the icon
 * at the specified position.
 *
 * Use %NULL for @tooltip to remove an existing tooltip.
 *
 * See also ctk_widget_set_tooltip_text() and 
 * ctk_entry_set_icon_tooltip_markup().
 *
 * If you unset the widget tooltip via ctk_widget_set_tooltip_text() or
 * ctk_widget_set_tooltip_markup(), this sets CtkWidget:has-tooltip to %FALSE,
 * which suppresses icon tooltips too. You can resolve this by then calling
 * ctk_widget_set_has_tooltip() to set CtkWidget:has-tooltip back to %TRUE, or
 * setting at least one non-empty tooltip on any icon achieves the same result.
 *
 * Since: 2.16
 */
void
ctk_entry_set_icon_tooltip_text (CtkEntry             *entry,
                                 CtkEntryIconPosition  icon_pos,
                                 const gchar          *tooltip)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (CTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = entry->priv;

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (CTK_WIDGET (entry), icon_pos);

  g_free (icon_info->tooltip);

  /* Treat an empty string as a NULL string,
   * because an empty string would be useless for a tooltip:
   */
  if (tooltip && tooltip[0] == '\0')
    tooltip = NULL;

  icon_info->tooltip = tooltip ? g_markup_escape_text (tooltip, -1) : NULL;

  ensure_has_tooltip (entry);

  g_object_notify_by_pspec (G_OBJECT (entry),
                            entry_props[icon_pos == CTK_ENTRY_ICON_PRIMARY
                                        ? PROP_TOOLTIP_TEXT_PRIMARY
                                        : PROP_TOOLTIP_TEXT_SECONDARY]);
}

/**
 * ctk_entry_get_icon_tooltip_markup:
 * @entry: a #CtkEntry
 * @icon_pos: the icon position
 *
 * Gets the contents of the tooltip on the icon at the specified 
 * position in @entry.
 * 
 * Returns: (nullable): the tooltip text, or %NULL. Free the returned
 *     string with g_free() when done.
 * 
 * Since: 2.16
 */
gchar *
ctk_entry_get_icon_tooltip_markup (CtkEntry             *entry,
                                   CtkEntryIconPosition  icon_pos)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  priv = entry->priv;

  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;
 
  return g_strdup (icon_info->tooltip);
}

/**
 * ctk_entry_set_icon_tooltip_markup:
 * @entry: a #CtkEntry
 * @icon_pos: the icon position
 * @tooltip: (allow-none): the contents of the tooltip for the icon, or %NULL
 *
 * Sets @tooltip as the contents of the tooltip for the icon at
 * the specified position. @tooltip is assumed to be marked up with
 * the [Pango text markup language][PangoMarkupFormat].
 *
 * Use %NULL for @tooltip to remove an existing tooltip.
 *
 * See also ctk_widget_set_tooltip_markup() and 
 * ctk_entry_set_icon_tooltip_text().
 *
 * Since: 2.16
 */
void
ctk_entry_set_icon_tooltip_markup (CtkEntry             *entry,
                                   CtkEntryIconPosition  icon_pos,
                                   const gchar          *tooltip)
{
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (CTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = entry->priv;

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (CTK_WIDGET (entry), icon_pos);

  g_free (icon_info->tooltip);

  /* Treat an empty string as a NULL string,
   * because an empty string would be useless for a tooltip:
   */
  if (tooltip && tooltip[0] == '\0')
    tooltip = NULL;

  icon_info->tooltip = g_strdup (tooltip);

  ensure_has_tooltip (entry);

  g_object_notify_by_pspec (G_OBJECT (entry),
                            entry_props[icon_pos == CTK_ENTRY_ICON_PRIMARY
                                        ? PROP_TOOLTIP_MARKUP_PRIMARY
                                        : PROP_TOOLTIP_MARKUP_SECONDARY]);
}

static gboolean
ctk_entry_query_tooltip (CtkWidget  *widget,
                         gint        x,
                         gint        y,
                         gboolean    keyboard_tip,
                         CtkTooltip *tooltip)
{
  CtkEntry *entry;
  CtkEntryPrivate *priv;
  EntryIconInfo *icon_info;
  gint icon_pos;

  entry = CTK_ENTRY (widget);
  priv = entry->priv;

  if (!keyboard_tip)
    {
      icon_pos = ctk_entry_get_icon_at_pos (entry, x, y);
      if (icon_pos != -1)
        {
          if ((icon_info = priv->icons[icon_pos]) != NULL)
            {
              if (icon_info->tooltip)
                {
                  ctk_tooltip_set_markup (tooltip, icon_info->tooltip);
                  return TRUE;
                }

              return FALSE;
            }
        }
    }

  return CTK_WIDGET_CLASS (ctk_entry_parent_class)->query_tooltip (widget,
                                                                   x, y,
                                                                   keyboard_tip,
                                                                   tooltip);
}


/* Quick hack of a popup menu
 */
static void
activate_cb (CtkWidget *menuitem,
	     CtkEntry  *entry)
{
  const gchar *signal;

  signal = g_object_get_qdata (G_OBJECT (menuitem), quark_ctk_signal);
  g_signal_emit_by_name (entry, signal);
}


static gboolean
ctk_entry_mnemonic_activate (CtkWidget *widget,
			     gboolean   group_cycling)
{
  ctk_widget_grab_focus (widget);
  return GDK_EVENT_STOP;
}

static void
check_undo_icon_grab (CtkEntry      *entry,
                      EntryIconInfo *info)
{
  if (!info->device ||
      !ctk_widget_device_is_shadowed (CTK_WIDGET (entry), info->device))
    return;

  info->pressed = FALSE;
  info->current_sequence = NULL;
  info->device = NULL;
}

static void
ctk_entry_grab_notify (CtkWidget *widget,
                       gboolean   was_grabbed)
{
  CtkEntryPrivate *priv;
  gint i;

  priv = CTK_ENTRY (widget)->priv;

  for (i = 0; i < MAX_ICONS; i++)
    {
      if (priv->icons[i])
        check_undo_icon_grab (CTK_ENTRY (widget), priv->icons[i]);
    }
}

static void
append_action_signal (CtkEntry     *entry,
		      CtkWidget    *menu,
		      const gchar  *label,
		      const gchar  *signal,
                      gboolean      sensitive)
{
  CtkWidget *menuitem = ctk_menu_item_new_with_mnemonic (label);

  g_object_set_qdata (G_OBJECT (menuitem), quark_ctk_signal, (char *)signal);
  g_signal_connect (menuitem, "activate",
		    G_CALLBACK (activate_cb), entry);

  ctk_widget_set_sensitive (menuitem, sensitive);
  
  ctk_widget_show (menuitem);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
}
	
static void
popup_menu_detach (CtkWidget *attach_widget,
		   CtkMenu   *menu)
{
  CtkEntry *entry_attach = CTK_ENTRY (attach_widget);
  CtkEntryPrivate *priv_attach = entry_attach->priv;

  priv_attach->popup_menu = NULL;
}

typedef struct
{
  CtkEntry *entry;
  GdkEvent *trigger_event;
} PopupInfo;

static void
popup_targets_received (CtkClipboard     *clipboard,
			CtkSelectionData *data,
			gpointer          user_data)
{
  PopupInfo *info = user_data;
  CtkEntry *entry = info->entry;
  CtkEntryPrivate *info_entry_priv = entry->priv;
  GdkRectangle rect = { 0, 0, 1, 0 };

  if (ctk_widget_get_realized (CTK_WIDGET (entry)))
    {
      DisplayMode mode;
      gboolean clipboard_contains_text;
      CtkWidget *menu;
      CtkWidget *menuitem;

      clipboard_contains_text = ctk_selection_data_targets_include_text (data);
      if (info_entry_priv->popup_menu)
	ctk_widget_destroy (info_entry_priv->popup_menu);

      info_entry_priv->popup_menu = menu = ctk_menu_new ();
      ctk_style_context_add_class (ctk_widget_get_style_context (menu),
                                   CTK_STYLE_CLASS_CONTEXT_MENU);

      ctk_menu_attach_to_widget (CTK_MENU (menu), CTK_WIDGET (entry), popup_menu_detach);

      mode = ctk_entry_get_display_mode (entry);
      append_action_signal (entry, menu, _("Cu_t"), "cut-clipboard",
                            info_entry_priv->editable && mode == DISPLAY_NORMAL &&
                            info_entry_priv->current_pos != info_entry_priv->selection_bound);

      append_action_signal (entry, menu, _("_Copy"), "copy-clipboard",
                            mode == DISPLAY_NORMAL &&
                            info_entry_priv->current_pos != info_entry_priv->selection_bound);

      append_action_signal (entry, menu, _("_Paste"), "paste-clipboard",
                            info_entry_priv->editable && clipboard_contains_text);

      menuitem = ctk_menu_item_new_with_mnemonic (_("_Delete"));
      ctk_widget_set_sensitive (menuitem, info_entry_priv->editable && info_entry_priv->current_pos != info_entry_priv->selection_bound);
      g_signal_connect_swapped (menuitem, "activate",
                                G_CALLBACK (ctk_entry_delete_cb), entry);
      ctk_widget_show (menuitem);
      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);

      menuitem = ctk_separator_menu_item_new ();
      ctk_widget_show (menuitem);
      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);

      menuitem = ctk_menu_item_new_with_mnemonic (_("Select _All"));
      ctk_widget_set_sensitive (menuitem, ctk_entry_buffer_get_length (info_entry_priv->buffer) > 0);
      g_signal_connect_swapped (menuitem, "activate",
                                G_CALLBACK (ctk_entry_select_all), entry);
      ctk_widget_show (menuitem);
      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);

      if (info_entry_priv->show_emoji_icon ||
          (ctk_entry_get_input_hints (entry) & CTK_INPUT_HINT_NO_EMOJI) == 0)
        {
          menuitem = ctk_menu_item_new_with_mnemonic (_("Insert _Emoji"));
          ctk_widget_set_sensitive (menuitem,
                                    mode == DISPLAY_NORMAL &&
                                    info_entry_priv->editable);
          g_signal_connect_swapped (menuitem, "activate",
                                    G_CALLBACK (ctk_entry_insert_emoji), entry);
          ctk_widget_show (menuitem);
          ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
        }

      g_signal_emit (entry, signals[POPULATE_POPUP], 0, menu);

      if (info->trigger_event && gdk_event_triggers_context_menu (info->trigger_event))
        ctk_menu_popup_at_pointer (CTK_MENU (menu), info->trigger_event);
      else
        {
          ctk_entry_get_cursor_locations (entry, CURSOR_STANDARD, &rect.x, NULL);
          rect.x -= info_entry_priv->scroll_offset;
          rect.height = gdk_window_get_height (info_entry_priv->text_area);

          ctk_menu_popup_at_rect (CTK_MENU (menu),
                                  info_entry_priv->text_area,
                                  &rect,
                                  GDK_GRAVITY_SOUTH_EAST,
                                  GDK_GRAVITY_NORTH_WEST,
                                  info->trigger_event);

          ctk_menu_shell_select_first (CTK_MENU_SHELL (menu), FALSE);
        }
    }

  g_clear_pointer (&info->trigger_event, gdk_event_free);
  g_object_unref (entry);
  g_slice_free (PopupInfo, info);
}

static void
ctk_entry_do_popup (CtkEntry       *entry,
                    const GdkEvent *event)
{
  PopupInfo *info = g_slice_new (PopupInfo);

  /* In order to know what entries we should make sensitive, we
   * ask for the current targets of the clipboard, and when
   * we get them, then we actually pop up the menu.
   */
  info->entry = g_object_ref (entry);
  info->trigger_event = event ? gdk_event_copy (event) : ctk_get_current_event ();

  ctk_clipboard_request_contents (ctk_widget_get_clipboard (CTK_WIDGET (entry), GDK_SELECTION_CLIPBOARD),
				  gdk_atom_intern_static_string ("TARGETS"),
				  popup_targets_received,
				  info);
}

static gboolean
ctk_entry_popup_menu (CtkWidget *widget)
{
  ctk_entry_do_popup (CTK_ENTRY (widget), NULL);
  return GDK_EVENT_STOP;
}

static void
show_or_hide_handles (CtkWidget  *popover,
                      GParamSpec *pspec,
                      CtkEntry   *entry)
{
  gboolean visible;
  CtkTextHandle *handle;
  CtkTextHandleMode mode;

  visible = ctk_widget_get_visible (popover);

  handle = entry->priv->text_handle;
  mode = _ctk_text_handle_get_mode (handle);

  if (mode == CTK_TEXT_HANDLE_MODE_CURSOR)
    {
      _ctk_text_handle_set_visible (handle, CTK_TEXT_HANDLE_POSITION_CURSOR, !visible);
    }
  else if (mode == CTK_TEXT_HANDLE_MODE_SELECTION)
    {
      _ctk_text_handle_set_visible (handle, CTK_TEXT_HANDLE_POSITION_SELECTION_START, !visible);
      _ctk_text_handle_set_visible (handle, CTK_TEXT_HANDLE_POSITION_SELECTION_END, !visible);
    }
}

static void
activate_bubble_cb (CtkWidget *item,
	            CtkEntry  *entry)
{
  const gchar *signal;

  signal = g_object_get_qdata (G_OBJECT (item), quark_ctk_signal);
  ctk_widget_hide (entry->priv->selection_bubble);
  if (strcmp (signal, "select-all") == 0)
    ctk_entry_select_all (entry);
  else
    g_signal_emit_by_name (entry, signal);
}

static void
append_bubble_action (CtkEntry     *entry,
                      CtkWidget    *toolbar,
                      const gchar  *label,
                      const gchar  *icon_name,
                      const gchar  *signal,
                      gboolean      sensitive)
{
  CtkWidget *item, *image;

  item = ctk_button_new ();
  ctk_widget_set_focus_on_click (item, FALSE);
  image = ctk_image_new_from_icon_name (icon_name, CTK_ICON_SIZE_MENU);
  ctk_widget_show (image);
  ctk_container_add (CTK_CONTAINER (item), image);
  ctk_widget_set_tooltip_text (item, label);
  ctk_style_context_add_class (ctk_widget_get_style_context (item), "image-button");
  g_object_set_qdata (G_OBJECT (item), quark_ctk_signal, (char *)signal);
  g_signal_connect (item, "clicked", G_CALLBACK (activate_bubble_cb), entry);
  ctk_widget_set_sensitive (CTK_WIDGET (item), sensitive);
  ctk_widget_show (CTK_WIDGET (item));
  ctk_container_add (CTK_CONTAINER (toolbar), item);
}

static void
bubble_targets_received (CtkClipboard     *clipboard,
                         CtkSelectionData *data,
                         gpointer          user_data)
{
  CtkEntry *entry = user_data;
  CtkEntryPrivate *priv = entry->priv;
  cairo_rectangle_int_t rect;
  CtkAllocation allocation;
  gint start_x, end_x;
  gboolean has_selection;
  gboolean has_clipboard;
  gboolean all_selected;
  DisplayMode mode;
  CtkWidget *box;
  CtkWidget *toolbar;
  gint length, start, end;

  has_selection = ctk_editable_get_selection_bounds (CTK_EDITABLE (entry), &start, &end);
  length = ctk_entry_buffer_get_length (get_buffer (entry));
  all_selected = (start == 0) && (end == length);

  if (!has_selection && !priv->editable)
    {
      priv->selection_bubble_timeout_id = 0;
      return;
    }

  if (priv->selection_bubble)
    ctk_widget_destroy (priv->selection_bubble);

  priv->selection_bubble = ctk_popover_new (CTK_WIDGET (entry));
  ctk_style_context_add_class (ctk_widget_get_style_context (priv->selection_bubble),
                               CTK_STYLE_CLASS_TOUCH_SELECTION);
  ctk_popover_set_position (CTK_POPOVER (priv->selection_bubble), CTK_POS_BOTTOM);
  ctk_popover_set_modal (CTK_POPOVER (priv->selection_bubble), FALSE);
  g_signal_connect (priv->selection_bubble, "notify::visible",
                    G_CALLBACK (show_or_hide_handles), entry);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
  g_object_set (box, "margin", 10, NULL);
  ctk_widget_show (box);
  toolbar = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);
  ctk_widget_show (toolbar);
  ctk_container_add (CTK_CONTAINER (priv->selection_bubble), box);
  ctk_container_add (CTK_CONTAINER (box), toolbar);

  has_clipboard = ctk_selection_data_targets_include_text (data);
  mode = ctk_entry_get_display_mode (entry);

  if (mode == DISPLAY_NORMAL)
    append_bubble_action (entry, toolbar, _("Select all"), "edit-select-all-symbolic", "select-all", !all_selected);

  if (priv->editable && has_selection && mode == DISPLAY_NORMAL)
    append_bubble_action (entry, toolbar, _("Cut"), "edit-cut-symbolic", "cut-clipboard", TRUE);

  if (has_selection && mode == DISPLAY_NORMAL)
    append_bubble_action (entry, toolbar, _("Copy"), "edit-copy-symbolic", "copy-clipboard", TRUE);

  if (priv->editable)
    append_bubble_action (entry, toolbar, _("Paste"), "edit-paste-symbolic", "paste-clipboard", has_clipboard);

  if (priv->populate_all)
    g_signal_emit (entry, signals[POPULATE_POPUP], 0, box);

  ctk_widget_get_allocation (CTK_WIDGET (entry), &allocation);

  ctk_entry_get_cursor_locations (entry, CURSOR_STANDARD, &start_x, NULL);

  start_x -= priv->scroll_offset;
  start_x = CLAMP (start_x, 0, priv->text_allocation.width);
  rect.y = priv->text_allocation.y - allocation.y;
  rect.height = priv->text_allocation.height;

  if (has_selection)
    {
      end_x = ctk_entry_get_selection_bound_location (entry) - priv->scroll_offset;
      end_x = CLAMP (end_x, 0, priv->text_allocation.width);

      rect.x = priv->text_allocation.x - allocation.x + MIN (start_x, end_x);
      rect.width = ABS (end_x - start_x);
    }
  else
    {
      rect.x = priv->text_allocation.x - allocation.x + start_x;
      rect.width = 0;
    }

  rect.x -= 5;
  rect.y -= 5;
  rect.width += 10;
  rect.height += 10;

  ctk_popover_set_pointing_to (CTK_POPOVER (priv->selection_bubble), &rect);
  ctk_widget_show (priv->selection_bubble);

  priv->selection_bubble_timeout_id = 0;
}

static gboolean
ctk_entry_selection_bubble_popup_show (gpointer user_data)
{
  CtkEntry *entry = user_data;

  ctk_clipboard_request_contents (ctk_widget_get_clipboard (CTK_WIDGET (entry), GDK_SELECTION_CLIPBOARD),
                                  gdk_atom_intern_static_string ("TARGETS"),
                                  bubble_targets_received,
                                  entry);
  return G_SOURCE_REMOVE;
}

static void
ctk_entry_selection_bubble_popup_unset (CtkEntry *entry)
{
  CtkEntryPrivate *priv;

  priv = entry->priv;

  if (priv->selection_bubble)
    ctk_widget_hide (priv->selection_bubble);

  if (priv->selection_bubble_timeout_id)
    {
      g_source_remove (priv->selection_bubble_timeout_id);
      priv->selection_bubble_timeout_id = 0;
    }
}

static void
ctk_entry_selection_bubble_popup_set (CtkEntry *entry)
{
  CtkEntryPrivate *priv;

  priv = entry->priv;

  if (priv->selection_bubble_timeout_id)
    g_source_remove (priv->selection_bubble_timeout_id);

  priv->selection_bubble_timeout_id =
    gdk_threads_add_timeout (50, ctk_entry_selection_bubble_popup_show, entry);
  g_source_set_name_by_id (priv->selection_bubble_timeout_id, "[ctk+] ctk_entry_selection_bubble_popup_cb");
}

static void
ctk_entry_drag_begin (CtkWidget      *widget,
                      GdkDragContext *context)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  gchar *text;
  gint i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];
      
      if (icon_info != NULL)
        {
          if (icon_info->in_drag) 
            {
              ctk_drag_set_icon_definition (context,
                                            ctk_icon_helper_get_definition (CTK_ICON_HELPER (icon_info->gadget)),
                                            -2, -2);
              return;
            }
        }
    }

  text = _ctk_entry_get_selected_text (entry);

  if (text)
    {
      gint *ranges, n_ranges;
      cairo_surface_t *surface;
      double sx, sy;

      surface = _ctk_text_util_create_drag_icon (widget, text, -1);

      ctk_entry_get_pixel_ranges (entry, &ranges, &n_ranges);
      cairo_surface_get_device_scale (surface, &sx, &sy);
      cairo_surface_set_device_offset (surface,
                                       -(priv->drag_start_x - ranges[0]) * sx,
                                       -(priv->drag_start_y) * sy);
      g_free (ranges);

      ctk_drag_set_icon_surface (context, surface);
      cairo_surface_destroy (surface);
      g_free (text);
    }
}

static void
ctk_entry_drag_end (CtkWidget      *widget,
                    GdkDragContext *context)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  gint i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];

      if (icon_info != NULL)
        icon_info->in_drag = 0;
    }
}

static void
ctk_entry_drag_leave (CtkWidget        *widget,
		      GdkDragContext   *context,
		      guint             time)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;

  ctk_drag_unhighlight (widget);
  priv->dnd_position = -1;
  ctk_widget_queue_draw (widget);
}

static gboolean
ctk_entry_drag_drop  (CtkWidget        *widget,
		      GdkDragContext   *context,
		      gint              x,
		      gint              y,
		      guint             time)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  GdkAtom target = GDK_NONE;

  if (priv->editable)
    target = ctk_drag_dest_find_target (widget, context, NULL);

  if (target != GDK_NONE)
    ctk_drag_get_data (widget, context, target, time);
  else
    ctk_drag_finish (context, FALSE, FALSE, time);
  
  return TRUE;
}

static gboolean
ctk_entry_drag_motion (CtkWidget        *widget,
		       GdkDragContext   *context,
		       gint              x,
		       gint              y,
		       guint             time)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  CtkWidget *source_widget;
  GdkDragAction suggested_action;
  gint new_position, old_position;
  gint sel1, sel2;

  old_position = priv->dnd_position;
  new_position = ctk_entry_find_position (entry, x + priv->scroll_offset);

  if (priv->editable &&
      ctk_drag_dest_find_target (widget, context, NULL) != GDK_NONE)
    {
      source_widget = ctk_drag_get_source_widget (context);
      suggested_action = gdk_drag_context_get_suggested_action (context);

      if (!ctk_editable_get_selection_bounds (CTK_EDITABLE (entry), &sel1, &sel2) ||
          new_position < sel1 || new_position > sel2)
        {
          if (source_widget == widget)
	    {
	      /* Default to MOVE, unless the user has
	       * pressed ctrl or alt to affect available actions
	       */
	      if ((gdk_drag_context_get_actions (context) & GDK_ACTION_MOVE) != 0)
	        suggested_action = GDK_ACTION_MOVE;
	    }

          priv->dnd_position = new_position;
        }
      else
        {
          if (source_widget == widget)
	    suggested_action = 0;	/* Can't drop in selection where drag started */

          priv->dnd_position = -1;
        }
    }
  else
    {
      /* Entry not editable, or no text */
      suggested_action = 0;
      priv->dnd_position = -1;
    }

  if (show_placeholder_text (entry))
    priv->dnd_position = -1;

  gdk_drag_status (context, suggested_action, time);
  if (suggested_action == 0)
    ctk_drag_unhighlight (widget);
  else
    ctk_drag_highlight (widget);

  if (priv->dnd_position != old_position)
    ctk_widget_queue_draw (widget);

  return TRUE;
}

static void
ctk_entry_drag_data_received (CtkWidget        *widget,
			      GdkDragContext   *context,
			      gint              x,
			      gint              y,
			      CtkSelectionData *selection_data,
			      guint             info,
			      guint             time)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  CtkEditable *editable = CTK_EDITABLE (widget);
  gchar *str;

  str = (gchar *) ctk_selection_data_get_text (selection_data);

  if (str && priv->editable)
    {
      gint new_position;
      gint sel1, sel2;
      gint length = -1;

      if (priv->truncate_multiline)
        length = truncate_multiline (str);

      new_position = ctk_entry_find_position (entry, x + priv->scroll_offset);

      if (!ctk_editable_get_selection_bounds (editable, &sel1, &sel2) ||
	  new_position < sel1 || new_position > sel2)
	{
	  ctk_editable_insert_text (editable, str, length, &new_position);
	}
      else
	{
	  /* Replacing selection */
          begin_change (entry);
	  ctk_editable_delete_text (editable, sel1, sel2);
	  ctk_editable_insert_text (editable, str, length, &sel1);
          end_change (entry);
	}
      
      ctk_drag_finish (context, TRUE, gdk_drag_context_get_selected_action (context) == GDK_ACTION_MOVE, time);
    }
  else
    {
      /* Drag and drop didn't happen! */
      ctk_drag_finish (context, FALSE, FALSE, time);
    }

  g_free (str);
}

static void
ctk_entry_drag_data_get (CtkWidget        *widget,
			 GdkDragContext   *context,
			 CtkSelectionData *selection_data,
			 guint             info,
			 guint             time)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  CtkEditable *editable = CTK_EDITABLE (widget);
  gint sel_start, sel_end;
  gint i;

  /* If there is an icon drag going on, exit early. */
  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];

      if (icon_info != NULL)
        {
          if (icon_info->in_drag)
            return;
        }
    }

  if (ctk_editable_get_selection_bounds (editable, &sel_start, &sel_end))
    {
      gchar *str = _ctk_entry_get_display_text (CTK_ENTRY (widget), sel_start, sel_end);

      ctk_selection_data_set_text (selection_data, str, -1);
      
      g_free (str);
    }

}

static void
ctk_entry_drag_data_delete (CtkWidget      *widget,
			    GdkDragContext *context)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  CtkEditable *editable = CTK_EDITABLE (widget);
  gint sel_start, sel_end;
  gint i;

  /* If there is an icon drag going on, exit early. */
  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];

      if (icon_info != NULL)
        {
          if (icon_info->in_drag)
            return;
        }
    }

  if (priv->editable &&
      ctk_editable_get_selection_bounds (editable, &sel_start, &sel_end))
    ctk_editable_delete_text (editable, sel_start, sel_end);
}

/* We display the cursor when
 *
 *  - the selection is empty, AND
 *  - the widget has focus
 */

#define CURSOR_ON_MULTIPLIER 2
#define CURSOR_OFF_MULTIPLIER 1
#define CURSOR_PEND_MULTIPLIER 3
#define CURSOR_DIVIDER 3

static gboolean
cursor_blinks (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (ctk_widget_has_focus (CTK_WIDGET (entry)) &&
      priv->editable &&
      priv->selection_bound == priv->current_pos)
    {
      CtkSettings *settings;
      gboolean blink;

      settings = ctk_widget_get_settings (CTK_WIDGET (entry));
      g_object_get (settings, "ctk-cursor-blink", &blink, NULL);

      return blink;
    }
  else
    return FALSE;
}

static gboolean
get_middle_click_paste (CtkEntry *entry)
{
  CtkSettings *settings;
  gboolean paste;

  settings = ctk_widget_get_settings (CTK_WIDGET (entry));
  g_object_get (settings, "ctk-enable-primary-paste", &paste, NULL);

  return paste;
}

static gint
get_cursor_time (CtkEntry *entry)
{
  CtkSettings *settings = ctk_widget_get_settings (CTK_WIDGET (entry));
  gint time;

  g_object_get (settings, "ctk-cursor-blink-time", &time, NULL);

  return time;
}

static gint
get_cursor_blink_timeout (CtkEntry *entry)
{
  CtkSettings *settings = ctk_widget_get_settings (CTK_WIDGET (entry));
  gint timeout;

  g_object_get (settings, "ctk-cursor-blink-timeout", &timeout, NULL);

  return timeout;
}

static void
show_cursor (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkWidget *widget;

  if (!priv->cursor_visible)
    {
      priv->cursor_visible = TRUE;

      widget = CTK_WIDGET (entry);
      if (ctk_widget_has_focus (widget) && priv->selection_bound == priv->current_pos)
	ctk_widget_queue_draw (widget);
    }
}

static void
hide_cursor (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  CtkWidget *widget;

  if (priv->cursor_visible)
    {
      priv->cursor_visible = FALSE;

      widget = CTK_WIDGET (entry);
      if (ctk_widget_has_focus (widget) && priv->selection_bound == priv->current_pos)
	ctk_widget_queue_draw (widget);
    }
}

/*
 * Blink!
 */
static gint
blink_cb (gpointer data)
{
  CtkEntry *entry;
  CtkEntryPrivate *priv; 
  gint blink_timeout;

  entry = CTK_ENTRY (data);
  priv = entry->priv;
 
  if (!ctk_widget_has_focus (CTK_WIDGET (entry)))
    {
      g_warning ("CtkEntry - did not receive focus-out-event. If you\n"
		 "connect a handler to this signal, it must return\n"
		 "GDK_EVENT_PROPAGATE so the entry gets the event as well");

      ctk_entry_check_cursor_blink (entry);

      return G_SOURCE_REMOVE;
    }
  
  g_assert (priv->selection_bound == priv->current_pos);
  
  blink_timeout = get_cursor_blink_timeout (entry);
  if (priv->blink_time > 1000 * blink_timeout && 
      blink_timeout < G_MAXINT/1000) 
    {
      /* we've blinked enough without the user doing anything, stop blinking */
      show_cursor (entry);
      priv->blink_timeout = 0;
    } 
  else if (priv->cursor_visible)
    {
      hide_cursor (entry);
      priv->blink_timeout = gdk_threads_add_timeout (get_cursor_time (entry) * CURSOR_OFF_MULTIPLIER / CURSOR_DIVIDER,
					    blink_cb,
					    entry);
      g_source_set_name_by_id (priv->blink_timeout, "[ctk+] blink_cb");
    }
  else
    {
      show_cursor (entry);
      priv->blink_time += get_cursor_time (entry);
      priv->blink_timeout = gdk_threads_add_timeout (get_cursor_time (entry) * CURSOR_ON_MULTIPLIER / CURSOR_DIVIDER,
					    blink_cb,
					    entry);
      g_source_set_name_by_id (priv->blink_timeout, "[ctk+] blink_cb");
    }

  return G_SOURCE_REMOVE;
}

static void
ctk_entry_check_cursor_blink (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (cursor_blinks (entry))
    {
      if (!priv->blink_timeout)
	{
	  show_cursor (entry);
	  priv->blink_timeout = gdk_threads_add_timeout (get_cursor_time (entry) * CURSOR_ON_MULTIPLIER / CURSOR_DIVIDER,
						blink_cb,
						entry);
	  g_source_set_name_by_id (priv->blink_timeout, "[ctk+] blink_cb");
	}
    }
  else
    {
      if (priv->blink_timeout)
	{
	  g_source_remove (priv->blink_timeout);
	  priv->blink_timeout = 0;
	}

      priv->cursor_visible = TRUE;
    }
}

static void
ctk_entry_pend_cursor_blink (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (cursor_blinks (entry))
    {
      if (priv->blink_timeout != 0)
	g_source_remove (priv->blink_timeout);

      priv->blink_timeout = gdk_threads_add_timeout (get_cursor_time (entry) * CURSOR_PEND_MULTIPLIER / CURSOR_DIVIDER,
                                                     blink_cb,
                                                     entry);
      g_source_set_name_by_id (priv->blink_timeout, "[ctk+] blink_cb");
      show_cursor (entry);
    }
}

static void
ctk_entry_reset_blink_time (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  priv->blink_time = 0;
}

/**
 * ctk_entry_set_completion:
 * @entry: A #CtkEntry
 * @completion: (allow-none): The #CtkEntryCompletion or %NULL
 *
 * Sets @completion to be the auxiliary completion object to use with @entry.
 * All further configuration of the completion mechanism is done on
 * @completion using the #CtkEntryCompletion API. Completion is disabled if
 * @completion is set to %NULL.
 *
 * Since: 2.4
 */
void
ctk_entry_set_completion (CtkEntry           *entry,
                          CtkEntryCompletion *completion)
{
  CtkEntryCompletion *old;

  g_return_if_fail (CTK_IS_ENTRY (entry));
  g_return_if_fail (!completion || CTK_IS_ENTRY_COMPLETION (completion));

  old = ctk_entry_get_completion (entry);

  if (old == completion)
    return;
  
  if (old)
    {
      _ctk_entry_completion_disconnect (old);
      g_object_unref (old);
    }

  if (!completion)
    {
      g_object_set_qdata (G_OBJECT (entry), quark_entry_completion, NULL);
      return;
    }

  /* hook into the entry */
  g_object_ref (completion);

  _ctk_entry_completion_connect (completion, entry);

  g_object_set_qdata (G_OBJECT (entry), quark_entry_completion, completion);

  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_COMPLETION]);
}

/**
 * ctk_entry_get_completion:
 * @entry: A #CtkEntry
 *
 * Returns the auxiliary completion object currently in use by @entry.
 *
 * Returns: (transfer none): The auxiliary completion object currently
 *     in use by @entry.
 *
 * Since: 2.4
 */
CtkEntryCompletion *
ctk_entry_get_completion (CtkEntry *entry)
{
  CtkEntryCompletion *completion;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), NULL);

  completion = CTK_ENTRY_COMPLETION (g_object_get_qdata (G_OBJECT (entry), quark_entry_completion));

  return completion;
}

/**
 * ctk_entry_set_cursor_hadjustment:
 * @entry: a #CtkEntry
 * @adjustment: (nullable): an adjustment which should be adjusted when the cursor
 *              is moved, or %NULL
 *
 * Hooks up an adjustment to the cursor position in an entry, so that when 
 * the cursor is moved, the adjustment is scrolled to show that position. 
 * See ctk_scrolled_window_get_hadjustment() for a typical way of obtaining 
 * the adjustment.
 *
 * The adjustment has to be in pixel units and in the same coordinate system 
 * as the entry. 
 * 
 * Since: 2.12
 */
void
ctk_entry_set_cursor_hadjustment (CtkEntry      *entry,
                                  CtkAdjustment *adjustment)
{
  g_return_if_fail (CTK_IS_ENTRY (entry));
  if (adjustment)
    g_return_if_fail (CTK_IS_ADJUSTMENT (adjustment));

  if (adjustment)
    g_object_ref (adjustment);

  g_object_set_qdata_full (G_OBJECT (entry), 
                           quark_cursor_hadjustment,
                           adjustment, 
                           g_object_unref);
}

/**
 * ctk_entry_get_cursor_hadjustment:
 * @entry: a #CtkEntry
 *
 * Retrieves the horizontal cursor adjustment for the entry. 
 * See ctk_entry_set_cursor_hadjustment().
 *
 * Returns: (transfer none) (nullable): the horizontal cursor adjustment, or %NULL
 *   if none has been set.
 *
 * Since: 2.12
 */
CtkAdjustment*
ctk_entry_get_cursor_hadjustment (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), NULL);

  return g_object_get_qdata (G_OBJECT (entry), quark_cursor_hadjustment);
}

static gboolean
tick_cb (CtkWidget     *widget,
         GdkFrameClock *frame_clock,
         gpointer       user_data)
{
  CtkEntry *entry = CTK_ENTRY (widget);
  CtkEntryPrivate *priv = entry->priv;
  gint64 frame_time;
  gdouble iteration, pulse_iterations, current_iterations, fraction;

  if (priv->pulse2 == 0 && priv->pulse1 == 0)
    return G_SOURCE_CONTINUE;

  frame_time = gdk_frame_clock_get_frame_time (frame_clock);
  ctk_progress_tracker_advance_frame (&priv->tracker, frame_time);

  g_assert (priv->pulse2 > priv->pulse1);

  pulse_iterations = (priv->pulse2 - priv->pulse1) / (gdouble) G_USEC_PER_SEC;
  current_iterations = (frame_time - priv->pulse1) / (gdouble) G_USEC_PER_SEC;

  iteration = ctk_progress_tracker_get_iteration (&priv->tracker);
  /* Determine the fraction to move the block from one frame
   * to the next when pulse_fraction is how far the block should
   * move between two calls to ctk_entry_progress_pulse().
   */
  fraction = priv->progress_pulse_fraction * (iteration - priv->last_iteration) / MAX (pulse_iterations, current_iterations);
  priv->last_iteration = iteration;

  if (current_iterations > 3 * pulse_iterations)
    return G_SOURCE_CONTINUE;

  /* advance the block */
  if (priv->progress_pulse_way_back)
    {
      priv->progress_pulse_current -= fraction;

      if (priv->progress_pulse_current < 0.0)
        {
          priv->progress_pulse_current = 0.0;
          priv->progress_pulse_way_back = FALSE;
        }
    }
  else
    {
      priv->progress_pulse_current += fraction;

      if (priv->progress_pulse_current > 1.0 - priv->progress_pulse_fraction)
        {
          priv->progress_pulse_current = 1.0 - priv->progress_pulse_fraction;
          priv->progress_pulse_way_back = TRUE;
        }
    }

  ctk_widget_queue_allocate (widget);

  return G_SOURCE_CONTINUE;
}

static void
ctk_entry_ensure_progress_gadget (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->progress_gadget)
    return;

  priv->progress_gadget = ctk_css_custom_gadget_new ("progress",
                                                     CTK_WIDGET (entry),
                                                     priv->gadget,
                                                     NULL,
                                                     NULL,
                                                     NULL,
                                                     NULL,
                                                     NULL,
                                                     NULL);
  ctk_css_gadget_set_state (priv->progress_gadget,
      ctk_css_node_get_state (ctk_widget_get_css_node (CTK_WIDGET (entry))));

  update_node_ordering (entry);
}

static void
ctk_entry_start_pulse_mode (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->progress_pulse_mode)
    return;

  ctk_entry_ensure_progress_gadget (entry);
  ctk_css_gadget_set_visible (priv->progress_gadget, TRUE);
  ctk_css_gadget_add_class (priv->progress_gadget, CTK_STYLE_CLASS_PULSE);

  priv->progress_pulse_mode = TRUE;
  /* How long each pulse should last depends on calls to ctk_entry_progress_pulse.
   * Just start the tracker to repeat forever with iterations every second. */
  ctk_progress_tracker_start (&priv->tracker, G_USEC_PER_SEC, 0, INFINITY);
  priv->tick_id = ctk_widget_add_tick_callback (CTK_WIDGET (entry), tick_cb, NULL, NULL);

  priv->progress_fraction = 0.0;
  priv->progress_pulse_way_back = FALSE;
  priv->progress_pulse_current = 0.0;

  priv->pulse2 = 0;
  priv->pulse1 = 0;
  priv->last_iteration = 0;
}

static void
ctk_entry_stop_pulse_mode (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->progress_pulse_mode)
    {
      ctk_css_gadget_set_visible (priv->progress_gadget, FALSE);
      ctk_css_gadget_remove_class (priv->progress_gadget, CTK_STYLE_CLASS_PULSE);

      priv->progress_pulse_mode = FALSE;
      ctk_widget_remove_tick_callback (CTK_WIDGET (entry), priv->tick_id);
      priv->tick_id = 0;
    }
}

static void
ctk_entry_update_pulse (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  gint64 pulse_time = g_get_monotonic_time ();

  if (priv->pulse2 == pulse_time)
    return;

  priv->pulse1 = priv->pulse2;
  priv->pulse2 = pulse_time;
}

/**
 * ctk_entry_set_progress_fraction:
 * @entry: a #CtkEntry
 * @fraction: fraction of the task that’s been completed
 *
 * Causes the entry’s progress indicator to “fill in” the given
 * fraction of the bar. The fraction should be between 0.0 and 1.0,
 * inclusive.
 *
 * Since: 2.16
 */
void
ctk_entry_set_progress_fraction (CtkEntry *entry,
                                 gdouble   fraction)
{
  CtkWidget       *widget;
  CtkEntryPrivate *private;
  gdouble          old_fraction;

  g_return_if_fail (CTK_IS_ENTRY (entry));

  widget = CTK_WIDGET (entry);
  private = entry->priv;

  if (private->progress_pulse_mode)
    old_fraction = -1;
  else
    old_fraction = private->progress_fraction;

  ctk_entry_stop_pulse_mode (entry);

  ctk_entry_ensure_progress_gadget (entry);

  fraction = CLAMP (fraction, 0.0, 1.0);
  private->progress_fraction = fraction;
  private->progress_pulse_current = 0.0;

  if (fraction != old_fraction)
    {
      ctk_css_gadget_set_visible (private->progress_gadget, fraction > 0);

      g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_PROGRESS_FRACTION]);
      ctk_widget_queue_allocate (widget);
    }
}

/**
 * ctk_entry_get_progress_fraction:
 * @entry: a #CtkEntry
 *
 * Returns the current fraction of the task that’s been completed.
 * See ctk_entry_set_progress_fraction().
 *
 * Returns: a fraction from 0.0 to 1.0
 *
 * Since: 2.16
 */
gdouble
ctk_entry_get_progress_fraction (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), 0.0);

  return entry->priv->progress_fraction;
}

/**
 * ctk_entry_set_progress_pulse_step:
 * @entry: a #CtkEntry
 * @fraction: fraction between 0.0 and 1.0
 *
 * Sets the fraction of total entry width to move the progress
 * bouncing block for each call to ctk_entry_progress_pulse().
 *
 * Since: 2.16
 */
void
ctk_entry_set_progress_pulse_step (CtkEntry *entry,
                                   gdouble   fraction)
{
  CtkEntryPrivate *private;

  g_return_if_fail (CTK_IS_ENTRY (entry));

  private = entry->priv;

  fraction = CLAMP (fraction, 0.0, 1.0);

  if (fraction != private->progress_pulse_fraction)
    {
      private->progress_pulse_fraction = fraction;
      g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_PROGRESS_PULSE_STEP]);
    }
}

/**
 * ctk_entry_get_progress_pulse_step:
 * @entry: a #CtkEntry
 *
 * Retrieves the pulse step set with ctk_entry_set_progress_pulse_step().
 *
 * Returns: a fraction from 0.0 to 1.0
 *
 * Since: 2.16
 */
gdouble
ctk_entry_get_progress_pulse_step (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), 0.0);

  return entry->priv->progress_pulse_fraction;
}

/**
 * ctk_entry_progress_pulse:
 * @entry: a #CtkEntry
 *
 * Indicates that some progress is made, but you don’t know how much.
 * Causes the entry’s progress indicator to enter “activity mode,”
 * where a block bounces back and forth. Each call to
 * ctk_entry_progress_pulse() causes the block to move by a little bit
 * (the amount of movement per pulse is determined by
 * ctk_entry_set_progress_pulse_step()).
 *
 * Since: 2.16
 */
void
ctk_entry_progress_pulse (CtkEntry *entry)
{
  g_return_if_fail (CTK_IS_ENTRY (entry));

  ctk_entry_start_pulse_mode (entry);
  ctk_entry_update_pulse (entry);
}

/**
 * ctk_entry_set_placeholder_text:
 * @entry: a #CtkEntry
 * @text: (nullable): a string to be displayed when @entry is empty and unfocused, or %NULL
 *
 * Sets text to be displayed in @entry when it is empty and unfocused.
 * This can be used to give a visual hint of the expected contents of
 * the #CtkEntry.
 *
 * Note that since the placeholder text gets removed when the entry
 * received focus, using this feature is a bit problematic if the entry
 * is given the initial focus in a window. Sometimes this can be
 * worked around by delaying the initial focus setting until the
 * first key event arrives.
 *
 * Since: 3.2
 **/
void
ctk_entry_set_placeholder_text (CtkEntry    *entry,
                                const gchar *text)
{
  CtkEntryPrivate *priv;

  g_return_if_fail (CTK_IS_ENTRY (entry));

  priv = entry->priv;

  if (g_strcmp0 (priv->placeholder_text, text) == 0)
    return;

  g_free (priv->placeholder_text);
  priv->placeholder_text = g_strdup (text);

  ctk_entry_recompute (entry);

  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_PLACEHOLDER_TEXT]);
}

/**
 * ctk_entry_get_placeholder_text:
 * @entry: a #CtkEntry
 *
 * Retrieves the text that will be displayed when @entry is empty and unfocused
 *
 * Returns: a pointer to the placeholder text as a string. This string points to internally allocated
 * storage in the widget and must not be freed, modified or stored.
 *
 * Since: 3.2
 **/
const gchar *
ctk_entry_get_placeholder_text (CtkEntry *entry)
{
  CtkEntryPrivate *priv;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), NULL);

  priv = entry->priv;

  return priv->placeholder_text;
}

/* Caps Lock warning for password entries */

static void
show_capslock_feedback (CtkEntry    *entry,
                        const gchar *text)
{
  CtkEntryPrivate *priv = entry->priv;

  if (ctk_entry_get_icon_storage_type (entry, CTK_ENTRY_ICON_SECONDARY) == CTK_IMAGE_EMPTY)
    {
      ctk_entry_set_icon_from_icon_name (entry, CTK_ENTRY_ICON_SECONDARY, "caps-lock-symbolic");
      ctk_entry_set_icon_activatable (entry, CTK_ENTRY_ICON_SECONDARY, FALSE);
      priv->caps_lock_warning_shown = TRUE;
    }

  if (priv->caps_lock_warning_shown)
    ctk_entry_set_icon_tooltip_text (entry, CTK_ENTRY_ICON_SECONDARY, text);
  else
    g_warning ("Can't show Caps Lock warning, since secondary icon is set");
}

static void
remove_capslock_feedback (CtkEntry *entry)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->caps_lock_warning_shown)
    {
      ctk_entry_set_icon_from_icon_name (entry, CTK_ENTRY_ICON_SECONDARY, NULL);
      priv->caps_lock_warning_shown = FALSE;
    } 
}

static void
keymap_state_changed (GdkKeymap *keymap, 
                      CtkEntry  *entry)
{
  CtkEntryPrivate *priv = entry->priv;
  char *text = NULL;

  if (ctk_entry_get_display_mode (entry) != DISPLAY_NORMAL && priv->caps_lock_warning)
    { 
      if (gdk_keymap_get_caps_lock_state (keymap))
        text = _("Caps Lock is on");
    }

  if (text)
    show_capslock_feedback (entry, text);
  else
    remove_capslock_feedback (entry);
}

/**
 * ctk_entry_set_input_purpose:
 * @entry: a #CtkEntry
 * @purpose: the purpose
 *
 * Sets the #CtkEntry:input-purpose property which
 * can be used by on-screen keyboards and other input
 * methods to adjust their behaviour.
 *
 * Since: 3.6
 */
void
ctk_entry_set_input_purpose (CtkEntry        *entry,
                             CtkInputPurpose  purpose)

{
  g_return_if_fail (CTK_IS_ENTRY (entry));

  if (ctk_entry_get_input_purpose (entry) != purpose)
    {
      g_object_set (G_OBJECT (entry->priv->im_context),
                    "input-purpose", purpose,
                    NULL);

      g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_INPUT_PURPOSE]);
    }
}

/**
 * ctk_entry_get_input_purpose:
 * @entry: a #CtkEntry
 *
 * Gets the value of the #CtkEntry:input-purpose property.
 *
 * Since: 3.6
 */
CtkInputPurpose
ctk_entry_get_input_purpose (CtkEntry *entry)
{
  CtkInputPurpose purpose;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), CTK_INPUT_PURPOSE_FREE_FORM);

  g_object_get (G_OBJECT (entry->priv->im_context),
                "input-purpose", &purpose,
                NULL);

  return purpose;
}

/**
 * ctk_entry_set_input_hints:
 * @entry: a #CtkEntry
 * @hints: the hints
 *
 * Sets the #CtkEntry:input-hints property, which
 * allows input methods to fine-tune their behaviour.
 *
 * Since: 3.6
 */
void
ctk_entry_set_input_hints (CtkEntry      *entry,
                           CtkInputHints  hints)

{
  g_return_if_fail (CTK_IS_ENTRY (entry));

  if (ctk_entry_get_input_hints (entry) != hints)
    {
      g_object_set (G_OBJECT (entry->priv->im_context),
                    "input-hints", hints,
                    NULL);

      g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_INPUT_HINTS]);
    }
}

/**
 * ctk_entry_get_input_hints:
 * @entry: a #CtkEntry
 *
 * Gets the value of the #CtkEntry:input-hints property.
 *
 * Since: 3.6
 */
CtkInputHints
ctk_entry_get_input_hints (CtkEntry *entry)
{
  CtkInputHints hints;

  g_return_val_if_fail (CTK_IS_ENTRY (entry), CTK_INPUT_HINT_NONE);

  g_object_get (G_OBJECT (entry->priv->im_context),
                "input-hints", &hints,
                NULL);

  return hints;
}

/**
 * ctk_entry_set_attributes:
 * @entry: a #CtkEntry
 * @attrs: a #PangoAttrList
 *
 * Sets a #PangoAttrList; the attributes in the list are applied to the
 * entry text.
 *
 * Since: 3.6
 */
void
ctk_entry_set_attributes (CtkEntry      *entry,
                          PangoAttrList *attrs)
{
  CtkEntryPrivate *priv = entry->priv;
  g_return_if_fail (CTK_IS_ENTRY (entry));

  if (attrs)
    pango_attr_list_ref (attrs);

  if (priv->attrs)
    pango_attr_list_unref (priv->attrs);
  priv->attrs = attrs;

  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_ATTRIBUTES]);

  ctk_entry_recompute (entry);
  ctk_widget_queue_resize (CTK_WIDGET (entry));
}

/**
 * ctk_entry_get_attributes:
 * @entry: a #CtkEntry
 *
 * Gets the attribute list that was set on the entry using
 * ctk_entry_set_attributes(), if any.
 *
 * Returns: (transfer none) (nullable): the attribute list, or %NULL
 *     if none was set.
 *
 * Since: 3.6
 */
PangoAttrList *
ctk_entry_get_attributes (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), NULL);

  return entry->priv->attrs;
}

/**
 * ctk_entry_set_tabs:
 * @entry: a #CtkEntry
 * @tabs: a #PangoTabArray
 *
 * Sets a #PangoTabArray; the tabstops in the array are applied to the entry
 * text.
 *
 * Since: 3.10
 */

void
ctk_entry_set_tabs (CtkEntry      *entry,
                    PangoTabArray *tabs)
{
  CtkEntryPrivate *priv;
  g_return_if_fail (CTK_IS_ENTRY (entry));

  priv = entry->priv;
  if (priv->tabs)
    pango_tab_array_free(priv->tabs);

  if (tabs)
    priv->tabs = pango_tab_array_copy (tabs);
  else
    priv->tabs = NULL;

  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_TABS]);

  ctk_entry_recompute (entry);
  ctk_widget_queue_resize (CTK_WIDGET (entry));
}

/**
 * ctk_entry_get_tabs:
 * @entry: a #CtkEntry
 *
 * Gets the tabstops that were set on the entry using ctk_entry_set_tabs(), if
 * any.
 *
 * Returns: (nullable) (transfer none): the tabstops, or %NULL if none was set.
 *
 * Since: 3.10
 */

PangoTabArray *
ctk_entry_get_tabs (CtkEntry *entry)
{
  g_return_val_if_fail (CTK_IS_ENTRY (entry), NULL);

  return entry->priv->tabs;
}

static void
ctk_entry_insert_emoji (CtkEntry *entry)
{
  CtkWidget *chooser;
  GdkRectangle rect;

  if (ctk_entry_get_input_hints (entry) & CTK_INPUT_HINT_NO_EMOJI)
    return;

  if (ctk_widget_get_ancestor (CTK_WIDGET (entry), CTK_TYPE_EMOJI_CHOOSER) != NULL)
    return;

  chooser = CTK_WIDGET (g_object_get_data (G_OBJECT (entry), "ctk-emoji-chooser"));
  if (!chooser)
    {
      chooser = ctk_emoji_chooser_new ();
      g_object_set_data (G_OBJECT (entry), "ctk-emoji-chooser", chooser);

      ctk_popover_set_relative_to (CTK_POPOVER (chooser), CTK_WIDGET (entry));
      if (entry->priv->show_emoji_icon)
        {
          ctk_entry_get_icon_area (entry, CTK_ENTRY_ICON_SECONDARY, &rect);
          ctk_popover_set_pointing_to (CTK_POPOVER (chooser), &rect);
        }
      g_signal_connect_swapped (chooser, "emoji-picked", G_CALLBACK (ctk_entry_enter_text), entry);
    }

  ctk_popover_popup (CTK_POPOVER (chooser));
}

static void
pick_emoji (CtkEntry *entry,
            int       icon,
            GdkEvent *event,
            gpointer  data)
{
  if (icon == CTK_ENTRY_ICON_SECONDARY)
    ctk_entry_insert_emoji (entry);
}

static void
set_show_emoji_icon (CtkEntry *entry,
                     gboolean  value)
{
  CtkEntryPrivate *priv = entry->priv;

  if (priv->show_emoji_icon == value)
    return;

  priv->show_emoji_icon = value;

  if (priv->show_emoji_icon)
    {
      ctk_entry_set_icon_from_icon_name (entry,
                                         CTK_ENTRY_ICON_SECONDARY,
                                         "face-smile-symbolic");

      ctk_entry_set_icon_sensitive (entry,
                                    CTK_ENTRY_ICON_SECONDARY,
                                    TRUE);

      ctk_entry_set_icon_activatable (entry,
                                      CTK_ENTRY_ICON_SECONDARY,
                                      TRUE);

      ctk_entry_set_icon_tooltip_text (entry,
                                       CTK_ENTRY_ICON_SECONDARY,
                                       _("Insert Emoji"));

      g_signal_connect (entry, "icon-press", G_CALLBACK (pick_emoji), NULL);
    }
  else
    {
      g_signal_handlers_disconnect_by_func (entry, pick_emoji, NULL);

      ctk_entry_set_icon_from_icon_name (entry,
                                         CTK_ENTRY_ICON_SECONDARY,
                                         NULL);

      ctk_entry_set_icon_tooltip_text (entry,
                                       CTK_ENTRY_ICON_SECONDARY,
                                       NULL);
    }

  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_SHOW_EMOJI_ICON]);
  ctk_widget_queue_resize (CTK_WIDGET (entry));
}

static void
set_enable_emoji_completion (CtkEntry *entry,
                             gboolean  value)
{
  CtkEntryPrivate *priv = ctk_entry_get_instance_private (entry);

  if (priv->enable_emoji_completion == value)
    return;

  priv->enable_emoji_completion = value;

  if (priv->enable_emoji_completion)
    g_object_set_data (G_OBJECT (entry), "emoji-completion-popup",
                       ctk_emoji_completion_new (entry));
  else
    g_object_set_data (G_OBJECT (entry), "emoji-completion-popup", NULL);

  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_ENABLE_EMOJI_COMPLETION]);
}
