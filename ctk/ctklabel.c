/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.Free
 */

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include <math.h>
#include <string.h>

#include "ctklabel.h"
#include "ctklabelprivate.h"
#include "ctkaccellabel.h"
#include "ctkbindings.h"
#include "ctkbuildable.h"
#include "ctkbuilderprivate.h"
#include "ctkclipboard.h"
#include "ctkcssshadowsvalueprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkdnd.h"
#include "ctkimage.h"
#include "ctkintl.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkmenuitem.h"
#include "ctkmenushellprivate.h"
#include "ctknotebook.h"
#include "ctkpango.h"
#include "ctkprivate.h"
#include "ctkseparatormenuitem.h"
#include "ctkshow.h"
#include "ctkstylecontextprivate.h"
#include "ctktextutil.h"
#include "ctktooltip.h"
#include "ctktypebuiltins.h"
#include "ctkwidgetprivate.h"
#include "ctkwindow.h"
#include "ctkcssnodeprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkwidgetprivate.h"

#include "a11y/ctklabelaccessibleprivate.h"

/* this is in case rint() is not provided by the compiler, 
 * such as in the case of C89 compilers, like MSVC
 */
#include "fallback-c89.c"

/**
 * SECTION:ctklabel
 * @Short_description: A widget that displays a small to medium amount of text
 * @Title: CtkLabel
 *
 * The #CtkLabel widget displays a small amount of text. As the name
 * implies, most labels are used to label another widget such as a
 * #CtkButton, a #CtkMenuItem, or a #CtkComboBox.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * label
 * ├── [selection]
 * ├── [link]
 * ┊
 * ╰── [link]
 * ]|
 *
 * CtkLabel has a single CSS node with the name label. A wide variety
 * of style classes may be applied to labels, such as .title, .subtitle,
 * .dim-label, etc. In the #CtkShortcutsWindow, labels are used wth the
 * .keycap style class.
 *
 * If the label has a selection, it gets a subnode with name selection.
 *
 * If the label has links, there is one subnode per link. These subnodes
 * carry the link or visited state depending on whether they have been
 * visited.
 *
 * # CtkLabel as CtkBuildable
 *
 * The CtkLabel implementation of the CtkBuildable interface supports a
 * custom <attributes> element, which supports any number of <attribute>
 * elements. The <attribute> element has attributes named “name“, “value“,
 * “start“ and “end“ and allows you to specify #PangoAttribute values for
 * this label.
 *
 * An example of a UI definition fragment specifying Pango attributes:
 * |[
 * <object class="CtkLabel">
 *   <attributes>
 *     <attribute name="weight" value="PANGO_WEIGHT_BOLD"/>
 *     <attribute name="background" value="red" start="5" end="10"/>
 *   </attributes>
 * </object>
 * ]|
 *
 * The start and end attributes specify the range of characters to which the
 * Pango attribute applies. If start and end are not specified, the attribute is
 * applied to the whole text. Note that specifying ranges does not make much
 * sense with translatable attributes. Use markup embedded in the translatable
 * content instead.
 *
 * # Mnemonics
 *
 * Labels may contain “mnemonics”. Mnemonics are
 * underlined characters in the label, used for keyboard navigation.
 * Mnemonics are created by providing a string with an underscore before
 * the mnemonic character, such as `"_File"`, to the
 * functions ctk_label_new_with_mnemonic() or
 * ctk_label_set_text_with_mnemonic().
 *
 * Mnemonics automatically activate any activatable widget the label is
 * inside, such as a #CtkButton; if the label is not inside the
 * mnemonic’s target widget, you have to tell the label about the target
 * using ctk_label_set_mnemonic_widget(). Here’s a simple example where
 * the label is inside a button:
 *
 * |[<!-- language="C" -->
 *   // Pressing Alt+H will activate this button
 *   CtkWidget *button = ctk_button_new ();
 *   CtkWidget *label = ctk_label_new_with_mnemonic ("_Hello");
 *   ctk_container_add (CTK_CONTAINER (button), label);
 * ]|
 *
 * There’s a convenience function to create buttons with a mnemonic label
 * already inside:
 *
 * |[<!-- language="C" -->
 *   // Pressing Alt+H will activate this button
 *   CtkWidget *button = ctk_button_new_with_mnemonic ("_Hello");
 * ]|
 *
 * To create a mnemonic for a widget alongside the label, such as a
 * #CtkEntry, you have to point the label at the entry with
 * ctk_label_set_mnemonic_widget():
 *
 * |[<!-- language="C" -->
 *   // Pressing Alt+H will focus the entry
 *   CtkWidget *entry = ctk_entry_new ();
 *   CtkWidget *label = ctk_label_new_with_mnemonic ("_Hello");
 *   ctk_label_set_mnemonic_widget (CTK_LABEL (label), entry);
 * ]|
 *
 * # Markup (styled text)
 *
 * To make it easy to format text in a label (changing colors,
 * fonts, etc.), label text can be provided in a simple
 * [markup format][PangoMarkupFormat].
 *
 * Here’s how to create a label with a small font:
 * |[<!-- language="C" -->
 *   CtkWidget *label = ctk_label_new (NULL);
 *   ctk_label_set_markup (CTK_LABEL (label), "<small>Small text</small>");
 * ]|
 *
 * (See [complete documentation][PangoMarkupFormat] of available
 * tags in the Pango manual.)
 *
 * The markup passed to ctk_label_set_markup() must be valid; for example,
 * literal <, > and & characters must be escaped as &lt;, &gt;, and &amp;.
 * If you pass text obtained from the user, file, or a network to
 * ctk_label_set_markup(), you’ll want to escape it with
 * g_markup_escape_text() or g_markup_printf_escaped().
 *
 * Markup strings are just a convenient way to set the #PangoAttrList on
 * a label; ctk_label_set_attributes() may be a simpler way to set
 * attributes in some cases. Be careful though; #PangoAttrList tends to
 * cause internationalization problems, unless you’re applying attributes
 * to the entire string (i.e. unless you set the range of each attribute
 * to [0, %G_MAXINT)). The reason is that specifying the start_index and
 * end_index for a #PangoAttribute requires knowledge of the exact string
 * being displayed, so translations will cause problems.
 *
 * # Selectable labels
 *
 * Labels can be made selectable with ctk_label_set_selectable().
 * Selectable labels allow the user to copy the label contents to
 * the clipboard. Only labels that contain useful-to-copy information
 * — such as error messages — should be made selectable.
 *
 * # Text layout # {#label-text-layout}
 *
 * A label can contain any number of paragraphs, but will have
 * performance problems if it contains more than a small number.
 * Paragraphs are separated by newlines or other paragraph separators
 * understood by Pango.
 *
 * Labels can automatically wrap text if you call
 * ctk_label_set_line_wrap().
 *
 * ctk_label_set_justify() sets how the lines in a label align
 * with one another. If you want to set how the label as a whole
 * aligns in its available space, see the #CtkWidget:halign and
 * #CtkWidget:valign properties.
 *
 * The #CtkLabel:width-chars and #CtkLabel:max-width-chars properties
 * can be used to control the size allocation of ellipsized or wrapped
 * labels. For ellipsizing labels, if either is specified (and less
 * than the actual text size), it is used as the minimum width, and the actual
 * text size is used as the natural width of the label. For wrapping labels,
 * width-chars is used as the minimum width, if specified, and max-width-chars
 * is used as the natural width. Even if max-width-chars specified, wrapping
 * labels will be rewrapped to use all of the available width.
 *
 * Note that the interpretation of #CtkLabel:width-chars and
 * #CtkLabel:max-width-chars has changed a bit with the introduction of
 * [width-for-height geometry management.][geometry-management]
 *
 * # Links
 *
 * Since 2.18, CTK+ supports markup for clickable hyperlinks in addition
 * to regular Pango markup. The markup for links is borrowed from HTML,
 * using the `<a>` with “href“ and “title“ attributes. CTK+ renders links
 * similar to the way they appear in web browsers, with colored, underlined
 * text. The “title“ attribute is displayed as a tooltip on the link.
 *
 * An example looks like this:
 *
 * |[<!-- language="C" -->
 * const gchar *text =
 * "Go to the"
 * "<a href=\"http://www.ctk.org title=\"&lt;i&gt;Our&lt;/i&gt; website\">"
 * "CTK+ website</a> for more...";
 * CtkWidget *label = ctk_label_new (NULL);
 * ctk_label_set_markup (CTK_LABEL (label), text);
 * ]|
 *
 * It is possible to implement custom handling for links and their tooltips with
 * the #CtkLabel::activate-link signal and the ctk_label_get_current_uri() function.
 */

struct _CtkLabelPrivate
{
  CtkLabelSelectionInfo *select_info;
  CtkWidget *mnemonic_widget;
  CtkWindow *mnemonic_window;
  CtkCssGadget *gadget;

  PangoAttrList *attrs;
  PangoAttrList *markup_attrs;
  PangoLayout   *layout;

  gchar   *label;
  gchar   *text;

  gdouble  angle;
  gfloat   xalign;
  gfloat   yalign;

  guint    mnemonics_visible  : 1;
  guint    jtype              : 2;
  guint    wrap               : 1;
  guint    use_underline      : 1;
  guint    use_markup         : 1;
  guint    ellipsize          : 3;
  guint    single_line_mode   : 1;
  guint    have_transform     : 1;
  guint    in_click           : 1;
  guint    wrap_mode          : 3;
  guint    pattern_set        : 1;
  guint    track_links        : 1;

  guint    mnemonic_keyval;

  gint     width_chars;
  gint     max_width_chars;
  gint     lines;
};

/* Notes about the handling of links:
 *
 * Links share the CtkLabelSelectionInfo struct with selectable labels.
 * There are some new fields for links. The links field contains the list
 * of CtkLabelLink structs that describe the links which are embedded in
 * the label. The active_link field points to the link under the mouse
 * pointer. For keyboard navigation, the “focus” link is determined by
 * finding the link which contains the selection_anchor position.
 * The link_clicked field is used with button press and release events
 * to ensure that pressing inside a link and releasing outside of it
 * does not activate the link.
 *
 * Links are rendered with the #CTK_STATE_FLAG_LINK/#CTK_STATE_FLAG_VISITED
 * state flags. When the mouse pointer is over a link, the pointer is changed
 * to indicate the link.
 *
 * Labels with links accept keyboard focus, and it is possible to move
 * the focus between the embedded links using Tab/Shift-Tab. The focus
 * is indicated by a focus rectangle that is drawn around the link text.
 * Pressing Enter activates the focused link, and there is a suitable
 * context menu for links that can be opened with the Menu key. Pressing
 * Control-C copies the link URI to the clipboard.
 *
 * In selectable labels with links, link functionality is only available
 * when the selection is empty.
 */
typedef struct
{
  gchar *uri;
  gchar *title;     /* the title attribute, used as tooltip */

  CtkCssNode *cssnode;

  gboolean visited; /* get set when the link is activated; this flag
                     * gets preserved over later set_markup() calls
                     */
  gint start;       /* position of the link in the PangoLayout */
  gint end;
} CtkLabelLink;

struct _CtkLabelSelectionInfo
{
  GdkWindow *window;
  gint selection_anchor;
  gint selection_end;
  CtkWidget *popup_menu;
  CtkCssNode *selection_node;

  GList *links;
  CtkLabelLink *active_link;

  CtkGesture *drag_gesture;
  CtkGesture *multipress_gesture;

  gint drag_start_x;
  gint drag_start_y;

  guint in_drag      : 1;
  guint select_words : 1;
  guint selectable   : 1;
  guint link_clicked : 1;
};

enum {
  MOVE_CURSOR,
  COPY_CLIPBOARD,
  POPULATE_POPUP,
  ACTIVATE_LINK,
  ACTIVATE_CURRENT_LINK,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_LABEL,
  PROP_ATTRIBUTES,
  PROP_USE_MARKUP,
  PROP_USE_UNDERLINE,
  PROP_JUSTIFY,
  PROP_PATTERN,
  PROP_WRAP,
  PROP_WRAP_MODE,
  PROP_SELECTABLE,
  PROP_MNEMONIC_KEYVAL,
  PROP_MNEMONIC_WIDGET,
  PROP_CURSOR_POSITION,
  PROP_SELECTION_BOUND,
  PROP_ELLIPSIZE,
  PROP_WIDTH_CHARS,
  PROP_SINGLE_LINE_MODE,
  PROP_ANGLE,
  PROP_MAX_WIDTH_CHARS,
  PROP_TRACK_VISITED_LINKS,
  PROP_LINES,
  PROP_XALIGN,
  PROP_YALIGN,
  NUM_PROPERTIES
};

static GParamSpec *label_props[NUM_PROPERTIES] = { NULL, };

/* When rotating ellipsizable text we want the natural size to request 
 * more to ensure the label wont ever ellipsize in an allocation of full natural size.
 * */
#define ROTATION_ELLIPSIZE_PADDING 2

static guint signals[LAST_SIGNAL] = { 0 };

static GQuark quark_shortcuts_connected;
static GQuark quark_mnemonic_menu;
static GQuark quark_mnemonics_visible_connected;
static GQuark quark_ctk_signal;
static GQuark quark_link;

static void ctk_label_set_property      (GObject          *object,
					 guint             prop_id,
					 const GValue     *value,
					 GParamSpec       *pspec);
static void ctk_label_get_property      (GObject          *object,
					 guint             prop_id,
					 GValue           *value,
					 GParamSpec       *pspec);
static void ctk_label_finalize          (GObject          *object);
static void ctk_label_destroy           (CtkWidget        *widget);
static void ctk_label_size_allocate     (CtkWidget        *widget,
                                         CtkAllocation    *allocation);
static void ctk_label_state_flags_changed   (CtkWidget        *widget,
                                             CtkStateFlags     prev_state);
static void ctk_label_style_updated     (CtkWidget        *widget);
static gboolean ctk_label_draw          (CtkWidget        *widget,
                                         cairo_t          *cr);
static gboolean ctk_label_focus         (CtkWidget         *widget,
                                         CtkDirectionType   direction);

static void ctk_label_realize           (CtkWidget        *widget);
static void ctk_label_unrealize         (CtkWidget        *widget);
static void ctk_label_map               (CtkWidget        *widget);
static void ctk_label_unmap             (CtkWidget        *widget);

static gboolean ctk_label_motion            (CtkWidget        *widget,
					     GdkEventMotion   *event);
static gboolean ctk_label_leave_notify      (CtkWidget        *widget,
                                             GdkEventCrossing *event);

static void     ctk_label_grab_focus        (CtkWidget        *widget);

static gboolean ctk_label_query_tooltip     (CtkWidget        *widget,
                                             gint              x,
                                             gint              y,
                                             gboolean          keyboard_tip,
                                             CtkTooltip       *tooltip);

static void ctk_label_set_text_internal          (CtkLabel      *label,
						  gchar         *str);
static void ctk_label_set_label_internal         (CtkLabel      *label,
						  gchar         *str);
static gboolean ctk_label_set_use_markup_internal    (CtkLabel  *label,
                                                      gboolean   val);
static gboolean ctk_label_set_use_underline_internal (CtkLabel  *label,
                                                      gboolean   val);
static void ctk_label_set_uline_text_internal    (CtkLabel      *label,
						  const gchar   *str);
static void ctk_label_set_pattern_internal       (CtkLabel      *label,
				                  const gchar   *pattern,
                                                  gboolean       is_mnemonic);
static void ctk_label_set_markup_internal        (CtkLabel      *label,
						  const gchar   *str,
						  gboolean       with_uline);
static void ctk_label_recalculate                (CtkLabel      *label);
static void ctk_label_hierarchy_changed          (CtkWidget     *widget,
						  CtkWidget     *old_toplevel);
static void ctk_label_screen_changed             (CtkWidget     *widget,
						  GdkScreen     *old_screen);
static gboolean ctk_label_popup_menu             (CtkWidget     *widget);

static void ctk_label_create_window       (CtkLabel *label);
static void ctk_label_destroy_window      (CtkLabel *label);
static void ctk_label_ensure_select_info  (CtkLabel *label);
static void ctk_label_clear_select_info   (CtkLabel *label);
static void ctk_label_update_cursor       (CtkLabel *label);
static void ctk_label_clear_layout        (CtkLabel *label);
static void ctk_label_ensure_layout       (CtkLabel *label);
static void ctk_label_select_region_index (CtkLabel *label,
                                           gint      anchor_index,
                                           gint      end_index);

static void ctk_label_update_active_link  (CtkWidget *widget,
                                           gdouble    x,
                                           gdouble    y);

static gboolean ctk_label_mnemonic_activate (CtkWidget         *widget,
					     gboolean           group_cycling);
static void     ctk_label_setup_mnemonic    (CtkLabel          *label,
					     guint              last_key);
static void     ctk_label_drag_data_get     (CtkWidget         *widget,
					     GdkDragContext    *context,
					     CtkSelectionData  *selection_data,
					     guint              info,
					     guint              time);

static void     ctk_label_buildable_interface_init     (CtkBuildableIface *iface);
static gboolean ctk_label_buildable_custom_tag_start   (CtkBuildable     *buildable,
							CtkBuilder       *builder,
							GObject          *child,
							const gchar      *tagname,
							GMarkupParser    *parser,
							gpointer         *data);

static void     ctk_label_buildable_custom_finished    (CtkBuildable     *buildable,
							CtkBuilder       *builder,
							GObject          *child,
							const gchar      *tagname,
							gpointer          user_data);


static void connect_mnemonics_visible_notify    (CtkLabel   *label);
static gboolean      separate_uline_pattern     (const gchar  *str,
                                                 guint        *accel_key,
                                                 gchar       **new_str,
                                                 gchar       **pattern);


/* For selectable labels: */
static void ctk_label_move_cursor        (CtkLabel        *label,
					  CtkMovementStep  step,
					  gint             count,
					  gboolean         extend_selection);
static void ctk_label_copy_clipboard     (CtkLabel        *label);
static void ctk_label_select_all         (CtkLabel        *label);
static void ctk_label_do_popup           (CtkLabel        *label,
					  const GdkEvent  *event);
static gint ctk_label_move_forward_word  (CtkLabel        *label,
					  gint             start);
static gint ctk_label_move_backward_word (CtkLabel        *label,
					  gint             start);

/* For links: */
static void          ctk_label_clear_links      (CtkLabel  *label);
static gboolean      ctk_label_activate_link    (CtkLabel    *label,
                                                 const gchar *uri);
static void          ctk_label_activate_current_link (CtkLabel *label);
static CtkLabelLink *ctk_label_get_current_link (CtkLabel  *label);
static void          emit_activate_link         (CtkLabel     *label,
                                                 CtkLabelLink *link);

/* Event controller callbacks */
static void   ctk_label_multipress_gesture_pressed  (CtkGestureMultiPress *gesture,
                                                     gint                  n_press,
                                                     gdouble               x,
                                                     gdouble               y,
                                                     CtkLabel             *label);
static void   ctk_label_multipress_gesture_released (CtkGestureMultiPress *gesture,
                                                     gint                  n_press,
                                                     gdouble               x,
                                                     gdouble               y,
                                                     CtkLabel             *label);
static void   ctk_label_drag_gesture_begin          (CtkGestureDrag *gesture,
                                                     gdouble         start_x,
                                                     gdouble         start_y,
                                                     CtkLabel       *label);
static void   ctk_label_drag_gesture_update         (CtkGestureDrag *gesture,
                                                     gdouble         offset_x,
                                                     gdouble         offset_y,
                                                     CtkLabel       *label);

static CtkSizeRequestMode ctk_label_get_request_mode                (CtkWidget           *widget);
static void               ctk_label_get_preferred_width             (CtkWidget           *widget,
                                                                     gint                *minimum_size,
                                                                     gint                *natural_size);
static void               ctk_label_get_preferred_height            (CtkWidget           *widget,
                                                                     gint                *minimum_size,
                                                                     gint                *natural_size);
static void               ctk_label_get_preferred_width_for_height  (CtkWidget           *widget,
                                                                     gint                 height,
                                                                     gint                *minimum_width,
                                                                     gint                *natural_width);
static void               ctk_label_get_preferred_height_for_width  (CtkWidget           *widget,
                                                                     gint                 width,
                                                                     gint                *minimum_height,
                                                                     gint                *natural_height);
static void    ctk_label_get_preferred_height_and_baseline_for_width (CtkWidget          *widget,
								      gint                width,
								      gint               *minimum_height,
								      gint               *natural_height,
								      gint               *minimum_baseline,
								      gint               *natural_baseline);

static void     ctk_label_measure (CtkCssGadget   *gadget,
                                   CtkOrientation  orientation,
                                   int             for_size,
                                   int            *minimum,
                                   int            *natural,
                                   int            *minimum_baseline,
                                   int            *natural_baseline,
                                   gpointer        unused);
static gboolean ctk_label_render  (CtkCssGadget   *gadget,
                                   cairo_t        *cr,
                                   int             x,
                                   int             y,
                                   int             width,
                                   int             height,
                                   gpointer        data);

static CtkBuildableIface *buildable_parent_iface = NULL;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
G_DEFINE_TYPE_WITH_CODE (CtkLabel, ctk_label, CTK_TYPE_MISC,
                         G_ADD_PRIVATE (CtkLabel)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_label_buildable_interface_init))
G_GNUC_END_IGNORE_DEPRECATIONS

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
ctk_label_class_init (CtkLabelClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);
  CtkBindingSet *binding_set;

  gobject_class->set_property = ctk_label_set_property;
  gobject_class->get_property = ctk_label_get_property;
  gobject_class->finalize = ctk_label_finalize;

  widget_class->destroy = ctk_label_destroy;
  widget_class->size_allocate = ctk_label_size_allocate;
  widget_class->state_flags_changed = ctk_label_state_flags_changed;
  widget_class->style_updated = ctk_label_style_updated;
  widget_class->query_tooltip = ctk_label_query_tooltip;
  widget_class->draw = ctk_label_draw;
  widget_class->realize = ctk_label_realize;
  widget_class->unrealize = ctk_label_unrealize;
  widget_class->map = ctk_label_map;
  widget_class->unmap = ctk_label_unmap;
  widget_class->motion_notify_event = ctk_label_motion;
  widget_class->leave_notify_event = ctk_label_leave_notify;
  widget_class->hierarchy_changed = ctk_label_hierarchy_changed;
  widget_class->screen_changed = ctk_label_screen_changed;
  widget_class->mnemonic_activate = ctk_label_mnemonic_activate;
  widget_class->drag_data_get = ctk_label_drag_data_get;
  widget_class->grab_focus = ctk_label_grab_focus;
  widget_class->popup_menu = ctk_label_popup_menu;
  widget_class->focus = ctk_label_focus;
  widget_class->get_request_mode = ctk_label_get_request_mode;
  widget_class->get_preferred_width = ctk_label_get_preferred_width;
  widget_class->get_preferred_height = ctk_label_get_preferred_height;
  widget_class->get_preferred_width_for_height = ctk_label_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = ctk_label_get_preferred_height_for_width;
  widget_class->get_preferred_height_and_baseline_for_width = ctk_label_get_preferred_height_and_baseline_for_width;

  class->move_cursor = ctk_label_move_cursor;
  class->copy_clipboard = ctk_label_copy_clipboard;
  class->activate_link = ctk_label_activate_link;

  /**
   * CtkLabel::move-cursor:
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
		  G_STRUCT_OFFSET (CtkLabelClass, move_cursor),
		  NULL, NULL,
		  _ctk_marshal_VOID__ENUM_INT_BOOLEAN,
		  G_TYPE_NONE, 3,
		  CTK_TYPE_MOVEMENT_STEP,
		  G_TYPE_INT,
		  G_TYPE_BOOLEAN);

   /**
   * CtkLabel::copy-clipboard:
   * @label: the object which received the signal
   *
   * The ::copy-clipboard signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to copy the selection to the clipboard.
   *
   * The default binding for this signal is Ctrl-c.
   */ 
  signals[COPY_CLIPBOARD] =
    g_signal_new (I_("copy-clipboard"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkLabelClass, copy_clipboard),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
  
  /**
   * CtkLabel::populate-popup:
   * @label: The label on which the signal is emitted
   * @menu: the menu that is being populated
   *
   * The ::populate-popup signal gets emitted before showing the
   * context menu of the label. Note that only selectable labels
   * have context menus.
   *
   * If you need to add items to the context menu, connect
   * to this signal and append your menuitems to the @menu.
   */
  signals[POPULATE_POPUP] =
    g_signal_new (I_("populate-popup"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkLabelClass, populate_popup),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_MENU);

    /**
     * CtkLabel::activate-current-link:
     * @label: The label on which the signal was emitted
     *
     * A [keybinding signal][CtkBindingSignal]
     * which gets emitted when the user activates a link in the label.
     *
     * Applications may also emit the signal with g_signal_emit_by_name()
     * if they need to control activation of URIs programmatically.
     *
     * The default bindings for this signal are all forms of the Enter key.
     *
     * Since: 2.18
     */
    signals[ACTIVATE_CURRENT_LINK] =
      g_signal_new_class_handler (I_("activate-current-link"),
                                  G_TYPE_FROM_CLASS (gobject_class),
                                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                  G_CALLBACK (ctk_label_activate_current_link),
                                  NULL, NULL,
                                  NULL,
                                  G_TYPE_NONE, 0);

    /**
     * CtkLabel::activate-link:
     * @label: The label on which the signal was emitted
     * @uri: the URI that is activated
     *
     * The signal which gets emitted to activate a URI.
     * Applications may connect to it to override the default behaviour,
     * which is to call ctk_show_uri_on_window().
     *
     * Returns: %TRUE if the link has been activated
     *
     * Since: 2.18
     */
    signals[ACTIVATE_LINK] =
      g_signal_new (I_("activate-link"),
                    G_TYPE_FROM_CLASS (gobject_class),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (CtkLabelClass, activate_link),
                    _ctk_boolean_handled_accumulator, NULL,
                    _ctk_marshal_BOOLEAN__STRING,
                    G_TYPE_BOOLEAN, 1, G_TYPE_STRING);

  /**
   * CtkLabel:label:
   *
   * The contents of the label.
   *
   * If the string contains [Pango XML markup][PangoMarkupFormat], you will
   * have to set the #CtkLabel:use-markup property to %TRUE in order for the
   * label to display the markup attributes. See also ctk_label_set_markup()
   * for a convenience function that sets both this property and the
   * #CtkLabel:use-markup property at the same time.
   *
   * If the string contains underlines acting as mnemonics, you will have to
   * set the #CtkLabel:use-underline property to %TRUE in order for the label
   * to display them.
   */
  label_props[PROP_LABEL] =
      g_param_spec_string ("label",
                           P_("Label"),
                           P_("The text of the label"),
                           "",
                           CTK_PARAM_READWRITE);

  label_props[PROP_ATTRIBUTES] =
      g_param_spec_boxed ("attributes",
                          P_("Attributes"),
                          P_("A list of style attributes to apply to the text of the label"),
                          PANGO_TYPE_ATTR_LIST,
                          CTK_PARAM_READWRITE);

  label_props[PROP_USE_MARKUP] =
      g_param_spec_boolean ("use-markup",
                            P_("Use markup"),
                            P_("The text of the label includes XML markup. See pango_parse_markup()"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  label_props[PROP_USE_UNDERLINE] =
      g_param_spec_boolean ("use-underline",
                            P_("Use underline"),
                            P_("If set, an underline in the text indicates the next character should be used for the mnemonic accelerator key"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  label_props[PROP_JUSTIFY] =
      g_param_spec_enum ("justify",
                         P_("Justification"),
                         P_("The alignment of the lines in the text of the label relative to each other. This does NOT affect the alignment of the label within its allocation. See CtkLabel:xalign for that"),
                         CTK_TYPE_JUSTIFICATION,
                         CTK_JUSTIFY_LEFT,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkLabel:xalign:
   *
   * The xalign property determines the horizontal aligment of the label text
   * inside the labels size allocation. Compare this to #CtkWidget:halign,
   * which determines how the labels size allocation is positioned in the
   * space available for the label.
   *
   * Since: 3.16
   */
  label_props[PROP_XALIGN] =
      g_param_spec_float ("xalign",
                          P_("X align"),
                          P_("The horizontal alignment, from 0 (left) to 1 (right). Reversed for RTL layouts."),
                          0.0, 1.0,
                          0.5,
                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkLabel:yalign:
   *
   * The yalign property determines the vertical aligment of the label text
   * inside the labels size allocation. Compare this to #CtkWidget:valign,
   * which determines how the labels size allocation is positioned in the
   * space available for the label.
   *
   * Since: 3.16
   */
  label_props[PROP_YALIGN] =
      g_param_spec_float ("yalign",
                          P_("Y align"),
                          P_("The vertical alignment, from 0 (top) to 1 (bottom)"),
                          0.0, 1.0,
                          0.5,
                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  label_props[PROP_PATTERN] =
      g_param_spec_string ("pattern",
                           P_("Pattern"),
                           P_("A string with _ characters in positions correspond to characters in the text to underline"),
                           NULL,
                           CTK_PARAM_WRITABLE);

  label_props[PROP_WRAP] =
      g_param_spec_boolean ("wrap",
                            P_("Line wrap"),
                            P_("If set, wrap lines if the text becomes too wide"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkLabel:wrap-mode:
   *
   * If line wrapping is on (see the #CtkLabel:wrap property) this controls
   * how the line wrapping is done. The default is %PANGO_WRAP_WORD, which
   * means wrap on word boundaries.
   *
   * Since: 2.10
   */
  label_props[PROP_WRAP_MODE] =
      g_param_spec_enum ("wrap-mode",
                         P_("Line wrap mode"),
                         P_("If wrap is set, controls how linewrapping is done"),
                         PANGO_TYPE_WRAP_MODE,
                         PANGO_WRAP_WORD,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  label_props[PROP_SELECTABLE] =
      g_param_spec_boolean ("selectable",
                            P_("Selectable"),
                            P_("Whether the label text can be selected with the mouse"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  label_props[PROP_MNEMONIC_KEYVAL] =
      g_param_spec_uint ("mnemonic-keyval",
                         P_("Mnemonic key"),
                         P_("The mnemonic accelerator key for this label"),
                         0, G_MAXUINT,
                         GDK_KEY_VoidSymbol,
                         CTK_PARAM_READABLE);

  label_props[PROP_MNEMONIC_WIDGET] =
      g_param_spec_object ("mnemonic-widget",
                           P_("Mnemonic widget"),
                           P_("The widget to be activated when the label's mnemonic key is pressed"),
                           CTK_TYPE_WIDGET,
                           CTK_PARAM_READWRITE);

  label_props[PROP_CURSOR_POSITION] =
      g_param_spec_int ("cursor-position",
                        P_("Cursor Position"),
                        P_("The current position of the insertion cursor in chars"),
                        0, G_MAXINT,
                        0,
                        CTK_PARAM_READABLE);

  label_props[PROP_SELECTION_BOUND] =
      g_param_spec_int ("selection-bound",
                        P_("Selection Bound"),
                        P_("The position of the opposite end of the selection from the cursor in chars"),
                        0, G_MAXINT,
                        0,
                        CTK_PARAM_READABLE);

  /**
   * CtkLabel:ellipsize:
   *
   * The preferred place to ellipsize the string, if the label does
   * not have enough room to display the entire string, specified as a
   * #PangoEllipsizeMode.
   *
   * Note that setting this property to a value other than
   * %PANGO_ELLIPSIZE_NONE has the side-effect that the label requests
   * only enough space to display the ellipsis "...". In particular, this
   * means that ellipsizing labels do not work well in notebook tabs, unless
   * the #CtkNotebook tab-expand child property is set to %TRUE. Other ways
   * to set a label's width are ctk_widget_set_size_request() and
   * ctk_label_set_width_chars().
   *
   * Since: 2.6
   */
  label_props[PROP_ELLIPSIZE] =
      g_param_spec_enum ("ellipsize",
                         P_("Ellipsize"),
                         P_("The preferred place to ellipsize the string, if the label does not have enough room to display the entire string"),
                         PANGO_TYPE_ELLIPSIZE_MODE,
                         PANGO_ELLIPSIZE_NONE,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkLabel:width-chars:
   *
   * The desired width of the label, in characters. If this property is set to
   * -1, the width will be calculated automatically.
   *
   * See the section on [text layout][label-text-layout]
   * for details of how #CtkLabel:width-chars and #CtkLabel:max-width-chars
   * determine the width of ellipsized and wrapped labels.
   *
   * Since: 2.6
   **/
  label_props[PROP_WIDTH_CHARS] =
      g_param_spec_int ("width-chars",
                        P_("Width In Characters"),
                        P_("The desired width of the label, in characters"),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkLabel:single-line-mode:
   *
   * Whether the label is in single line mode. In single line mode,
   * the height of the label does not depend on the actual text, it
   * is always set to ascent + descent of the font. This can be an
   * advantage in situations where resizing the label because of text
   * changes would be distracting, e.g. in a statusbar.
   *
   * Since: 2.6
   **/
  label_props[PROP_SINGLE_LINE_MODE] =
      g_param_spec_boolean ("single-line-mode",
                            P_("Single Line Mode"),
                            P_("Whether the label is in single line mode"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkLabel:angle:
   * 
   * The angle that the baseline of the label makes with the horizontal,
   * in degrees, measured counterclockwise. An angle of 90 reads from
   * from bottom to top, an angle of 270, from top to bottom. Ignored
   * if the label is selectable.
   *
   * Since: 2.6
   **/
  label_props[PROP_ANGLE] =
      g_param_spec_double ("angle",
                           P_("Angle"),
                           P_("Angle at which the label is rotated"),
                           0.0, 360.0,
                           0.0,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkLabel:max-width-chars:
   *
   * The desired maximum width of the label, in characters. If this property
   * is set to -1, the width will be calculated automatically.
   *
   * See the section on [text layout][label-text-layout]
   * for details of how #CtkLabel:width-chars and #CtkLabel:max-width-chars
   * determine the width of ellipsized and wrapped labels.
   *
   * Since: 2.6
   **/
  label_props[PROP_MAX_WIDTH_CHARS] =
      g_param_spec_int ("max-width-chars",
                        P_("Maximum Width In Characters"),
                        P_("The desired maximum width of the label, in characters"),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkLabel:track-visited-links:
   *
   * Set this property to %TRUE to make the label track which links
   * have been visited. It will then apply the #CTK_STATE_FLAG_VISITED
   * when rendering this link, in addition to #CTK_STATE_FLAG_LINK.
   *
   * Since: 2.18
   */
  label_props[PROP_TRACK_VISITED_LINKS] =
      g_param_spec_boolean ("track-visited-links",
                            P_("Track visited links"),
                            P_("Whether visited links should be tracked"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkLabel:lines:
   *
   * The number of lines to which an ellipsized, wrapping label
   * should be limited. This property has no effect if the
   * label is not wrapping or ellipsized. Set this property to
   * -1 if you don't want to limit the number of lines.
   *
   * Since: 3.10
   */
  label_props[PROP_LINES] =
      g_param_spec_int ("lines",
                        P_("Number of lines"),
                        P_("The desired number of lines, when ellipsizing a wrapping label"),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, label_props);

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
  
  add_move_binding (binding_set, GDK_KEY_f, GDK_CONTROL_MASK,
		    CTK_MOVEMENT_LOGICAL_POSITIONS, 1);
  
  add_move_binding (binding_set, GDK_KEY_b, GDK_CONTROL_MASK,
		    CTK_MOVEMENT_LOGICAL_POSITIONS, -1);
  
  add_move_binding (binding_set, GDK_KEY_Right, GDK_CONTROL_MASK,
		    CTK_MOVEMENT_WORDS, 1);

  add_move_binding (binding_set, GDK_KEY_Left, GDK_CONTROL_MASK,
		    CTK_MOVEMENT_WORDS, -1);

  add_move_binding (binding_set, GDK_KEY_KP_Right, GDK_CONTROL_MASK,
		    CTK_MOVEMENT_WORDS, 1);

  add_move_binding (binding_set, GDK_KEY_KP_Left, GDK_CONTROL_MASK,
		    CTK_MOVEMENT_WORDS, -1);

  /* select all */
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_a, GDK_CONTROL_MASK,
				"move-cursor", 3,
				G_TYPE_ENUM, CTK_MOVEMENT_PARAGRAPH_ENDS,
				G_TYPE_INT, -1,
				G_TYPE_BOOLEAN, FALSE);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_a, GDK_CONTROL_MASK,
				"move-cursor", 3,
				G_TYPE_ENUM, CTK_MOVEMENT_PARAGRAPH_ENDS,
				G_TYPE_INT, 1,
				G_TYPE_BOOLEAN, TRUE);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_slash, GDK_CONTROL_MASK,
				"move-cursor", 3,
				G_TYPE_ENUM, CTK_MOVEMENT_PARAGRAPH_ENDS,
				G_TYPE_INT, -1,
				G_TYPE_BOOLEAN, FALSE);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_slash, GDK_CONTROL_MASK,
				"move-cursor", 3,
				G_TYPE_ENUM, CTK_MOVEMENT_PARAGRAPH_ENDS,
				G_TYPE_INT, 1,
				G_TYPE_BOOLEAN, TRUE);

  /* unselect all */
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_a, GDK_SHIFT_MASK | GDK_CONTROL_MASK,
				"move-cursor", 3,
				G_TYPE_ENUM, CTK_MOVEMENT_PARAGRAPH_ENDS,
				G_TYPE_INT, 0,
				G_TYPE_BOOLEAN, FALSE);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_backslash, GDK_CONTROL_MASK,
				"move-cursor", 3,
				G_TYPE_ENUM, CTK_MOVEMENT_PARAGRAPH_ENDS,
				G_TYPE_INT, 0,
				G_TYPE_BOOLEAN, FALSE);

  add_move_binding (binding_set, GDK_KEY_f, GDK_MOD1_MASK,
		    CTK_MOVEMENT_WORDS, 1);

  add_move_binding (binding_set, GDK_KEY_b, GDK_MOD1_MASK,
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

  /* copy */
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_c, GDK_CONTROL_MASK,
				"copy-clipboard", 0);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_Return, 0,
				"activate-current-link", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_ISO_Enter, 0,
				"activate-current-link", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Enter, 0,
				"activate-current-link", 0);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_LABEL_ACCESSIBLE);

  ctk_widget_class_set_css_name (widget_class, "label");

  quark_shortcuts_connected = g_quark_from_static_string ("ctk-label-shortcuts-connected");
  quark_mnemonic_menu = g_quark_from_static_string ("ctk-mnemonic-menu");
  quark_mnemonics_visible_connected = g_quark_from_static_string ("ctk-label-mnemonics-visible-connected");
  quark_ctk_signal = g_quark_from_static_string ("ctk-signal");
  quark_link = g_quark_from_static_string ("link");
}

static void 
ctk_label_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  CtkLabel *label = CTK_LABEL (object);

  switch (prop_id)
    {
    case PROP_LABEL:
      ctk_label_set_label (label, g_value_get_string (value));
      break;
    case PROP_ATTRIBUTES:
      ctk_label_set_attributes (label, g_value_get_boxed (value));
      break;
    case PROP_USE_MARKUP:
      ctk_label_set_use_markup (label, g_value_get_boolean (value));
      break;
    case PROP_USE_UNDERLINE:
      ctk_label_set_use_underline (label, g_value_get_boolean (value));
      break;
    case PROP_JUSTIFY:
      ctk_label_set_justify (label, g_value_get_enum (value));
      break;
    case PROP_PATTERN:
      ctk_label_set_pattern (label, g_value_get_string (value));
      break;
    case PROP_WRAP:
      ctk_label_set_line_wrap (label, g_value_get_boolean (value));
      break;	  
    case PROP_WRAP_MODE:
      ctk_label_set_line_wrap_mode (label, g_value_get_enum (value));
      break;	  
    case PROP_SELECTABLE:
      ctk_label_set_selectable (label, g_value_get_boolean (value));
      break;	  
    case PROP_MNEMONIC_WIDGET:
      ctk_label_set_mnemonic_widget (label, (CtkWidget*) g_value_get_object (value));
      break;
    case PROP_ELLIPSIZE:
      ctk_label_set_ellipsize (label, g_value_get_enum (value));
      break;
    case PROP_WIDTH_CHARS:
      ctk_label_set_width_chars (label, g_value_get_int (value));
      break;
    case PROP_SINGLE_LINE_MODE:
      ctk_label_set_single_line_mode (label, g_value_get_boolean (value));
      break;	  
    case PROP_ANGLE:
      ctk_label_set_angle (label, g_value_get_double (value));
      break;
    case PROP_MAX_WIDTH_CHARS:
      ctk_label_set_max_width_chars (label, g_value_get_int (value));
      break;
    case PROP_TRACK_VISITED_LINKS:
      ctk_label_set_track_visited_links (label, g_value_get_boolean (value));
      break;
    case PROP_LINES:
      ctk_label_set_lines (label, g_value_get_int (value));
      break;
    case PROP_XALIGN:
      ctk_label_set_xalign (label, g_value_get_float (value));
      break;
    case PROP_YALIGN:
      ctk_label_set_yalign (label, g_value_get_float (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
ctk_label_get_property (GObject     *object,
			guint        prop_id,
			GValue      *value,
			GParamSpec  *pspec)
{
  CtkLabel *label = CTK_LABEL (object);
  CtkLabelPrivate *priv = label->priv;

  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, priv->label);
      break;
    case PROP_ATTRIBUTES:
      g_value_set_boxed (value, priv->attrs);
      break;
    case PROP_USE_MARKUP:
      g_value_set_boolean (value, priv->use_markup);
      break;
    case PROP_USE_UNDERLINE:
      g_value_set_boolean (value, priv->use_underline);
      break;
    case PROP_JUSTIFY:
      g_value_set_enum (value, priv->jtype);
      break;
    case PROP_WRAP:
      g_value_set_boolean (value, priv->wrap);
      break;
    case PROP_WRAP_MODE:
      g_value_set_enum (value, priv->wrap_mode);
      break;
    case PROP_SELECTABLE:
      g_value_set_boolean (value, ctk_label_get_selectable (label));
      break;
    case PROP_MNEMONIC_KEYVAL:
      g_value_set_uint (value, priv->mnemonic_keyval);
      break;
    case PROP_MNEMONIC_WIDGET:
      g_value_set_object (value, (GObject*) priv->mnemonic_widget);
      break;
    case PROP_CURSOR_POSITION:
      g_value_set_int (value, _ctk_label_get_cursor_position (label));
      break;
    case PROP_SELECTION_BOUND:
      g_value_set_int (value, _ctk_label_get_selection_bound (label));
      break;
    case PROP_ELLIPSIZE:
      g_value_set_enum (value, priv->ellipsize);
      break;
    case PROP_WIDTH_CHARS:
      g_value_set_int (value, ctk_label_get_width_chars (label));
      break;
    case PROP_SINGLE_LINE_MODE:
      g_value_set_boolean (value, ctk_label_get_single_line_mode (label));
      break;
    case PROP_ANGLE:
      g_value_set_double (value, ctk_label_get_angle (label));
      break;
    case PROP_MAX_WIDTH_CHARS:
      g_value_set_int (value, ctk_label_get_max_width_chars (label));
      break;
    case PROP_TRACK_VISITED_LINKS:
      g_value_set_boolean (value, ctk_label_get_track_visited_links (label));
      break;
    case PROP_LINES:
      g_value_set_int (value, ctk_label_get_lines (label));
      break;
    case PROP_XALIGN:
      g_value_set_float (value, ctk_label_get_xalign (label));
      break;
    case PROP_YALIGN:
      g_value_set_float (value, ctk_label_get_yalign (label));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_label_init (CtkLabel *label)
{
  CtkLabelPrivate *priv;

  label->priv = ctk_label_get_instance_private (label);
  priv = label->priv;

  ctk_widget_set_has_window (CTK_WIDGET (label), FALSE);

  priv->width_chars = -1;
  priv->max_width_chars = -1;
  priv->label = g_strdup ("");
  priv->lines = -1;

  priv->xalign = 0.5;
  priv->yalign = 0.5;

  priv->jtype = CTK_JUSTIFY_LEFT;
  priv->wrap = FALSE;
  priv->wrap_mode = PANGO_WRAP_WORD;
  priv->ellipsize = PANGO_ELLIPSIZE_NONE;

  priv->use_underline = FALSE;
  priv->use_markup = FALSE;
  priv->pattern_set = FALSE;
  priv->track_links = TRUE;

  priv->mnemonic_keyval = GDK_KEY_VoidSymbol;
  priv->layout = NULL;
  priv->text = g_strdup ("");
  priv->attrs = NULL;

  priv->mnemonic_widget = NULL;
  priv->mnemonic_window = NULL;

  priv->mnemonics_visible = TRUE;

  priv->gadget = ctk_css_custom_gadget_new_for_node (ctk_widget_get_css_node (CTK_WIDGET (label)),
                                                     CTK_WIDGET (label),
                                                     ctk_label_measure,
                                                     NULL,
                                                     ctk_label_render,
                                                     NULL,
                                                     NULL);
}


static void
ctk_label_buildable_interface_init (CtkBuildableIface *iface)
{
  buildable_parent_iface = g_type_interface_peek_parent (iface);

  iface->custom_tag_start = ctk_label_buildable_custom_tag_start;
  iface->custom_finished = ctk_label_buildable_custom_finished;
}

typedef struct {
  CtkBuilder    *builder;
  GObject       *object;
  PangoAttrList *attrs;
} PangoParserData;

static PangoAttribute *
attribute_from_text (CtkBuilder   *builder,
		     const gchar  *name,
		     const gchar  *value,
		     GError      **error)
{
  PangoAttribute *attribute = NULL;
  PangoAttrType   type;
  PangoLanguage  *language;
  PangoFontDescription *font_desc;
  GdkColor       *color;
  GValue          val = G_VALUE_INIT;

  if (!ctk_builder_value_from_string_type (builder, PANGO_TYPE_ATTR_TYPE, name, &val, error))
    return NULL;

  type = g_value_get_enum (&val);
  g_value_unset (&val);

  switch (type)
    {
      /* PangoAttrLanguage */
    case PANGO_ATTR_LANGUAGE:
      if ((language = pango_language_from_string (value)))
	{
	  attribute = pango_attr_language_new (language);
	  g_value_init (&val, G_TYPE_INT);
	}
      break;
      /* PangoAttrInt */
    case PANGO_ATTR_STYLE:
      if (ctk_builder_value_from_string_type (builder, PANGO_TYPE_STYLE, value, &val, error))
	attribute = pango_attr_style_new (g_value_get_enum (&val));
      break;
    case PANGO_ATTR_WEIGHT:
      if (ctk_builder_value_from_string_type (builder, PANGO_TYPE_WEIGHT, value, &val, error))
	attribute = pango_attr_weight_new (g_value_get_enum (&val));
      break;
    case PANGO_ATTR_VARIANT:
      if (ctk_builder_value_from_string_type (builder, PANGO_TYPE_VARIANT, value, &val, error))
	attribute = pango_attr_variant_new (g_value_get_enum (&val));
      break;
    case PANGO_ATTR_STRETCH:
      if (ctk_builder_value_from_string_type (builder, PANGO_TYPE_STRETCH, value, &val, error))
	attribute = pango_attr_stretch_new (g_value_get_enum (&val));
      break;
    case PANGO_ATTR_UNDERLINE:
      if (ctk_builder_value_from_string_type (builder, PANGO_TYPE_UNDERLINE, value, &val, NULL))
	attribute = pango_attr_underline_new (g_value_get_enum (&val));
      else
        {
          /* XXX: allow boolean for backwards compat, so ignore error */
          /* Deprecate this somehow */
          g_value_unset (&val);
          if (ctk_builder_value_from_string_type (builder, G_TYPE_BOOLEAN, value, &val, error))
            attribute = pango_attr_underline_new (g_value_get_boolean (&val));
        }
      break;
    case PANGO_ATTR_STRIKETHROUGH:
      if (ctk_builder_value_from_string_type (builder, G_TYPE_BOOLEAN, value, &val, error))
	attribute = pango_attr_strikethrough_new (g_value_get_boolean (&val));
      break;
    case PANGO_ATTR_GRAVITY:
      if (ctk_builder_value_from_string_type (builder, PANGO_TYPE_GRAVITY, value, &val, error))
	attribute = pango_attr_gravity_new (g_value_get_enum (&val));
      break;
    case PANGO_ATTR_GRAVITY_HINT:
      if (ctk_builder_value_from_string_type (builder, PANGO_TYPE_GRAVITY_HINT, value, &val, error))
	attribute = pango_attr_gravity_hint_new (g_value_get_enum (&val));
      break;
      /* PangoAttrString */
    case PANGO_ATTR_FAMILY:
      attribute = pango_attr_family_new (value);
      g_value_init (&val, G_TYPE_INT);
      break;

      /* PangoAttrSize */
    case PANGO_ATTR_SIZE:
      if (ctk_builder_value_from_string_type (builder, G_TYPE_INT, value, &val, error))
	attribute = pango_attr_size_new (g_value_get_int (&val));
      break;
    case PANGO_ATTR_ABSOLUTE_SIZE:
      if (ctk_builder_value_from_string_type (builder, G_TYPE_INT, value, &val, error))
	attribute = pango_attr_size_new_absolute (g_value_get_int (&val));
      break;

      /* PangoAttrFontDesc */
    case PANGO_ATTR_FONT_DESC:
      if ((font_desc = pango_font_description_from_string (value)))
	{
	  attribute = pango_attr_font_desc_new (font_desc);
	  pango_font_description_free (font_desc);
	  g_value_init (&val, G_TYPE_INT);
	}
      break;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

      /* PangoAttrColor */
    case PANGO_ATTR_FOREGROUND:
      if (ctk_builder_value_from_string_type (builder, GDK_TYPE_COLOR, value, &val, error))
	{
	  color = g_value_get_boxed (&val);
	  attribute = pango_attr_foreground_new (color->red, color->green, color->blue);
	}
      break;
    case PANGO_ATTR_BACKGROUND:
      if (ctk_builder_value_from_string_type (builder, GDK_TYPE_COLOR, value, &val, error))
	{
	  color = g_value_get_boxed (&val);
	  attribute = pango_attr_background_new (color->red, color->green, color->blue);
	}
      break;
    case PANGO_ATTR_UNDERLINE_COLOR:
      if (ctk_builder_value_from_string_type (builder, GDK_TYPE_COLOR, value, &val, error))
	{
	  color = g_value_get_boxed (&val);
	  attribute = pango_attr_underline_color_new (color->red, color->green, color->blue);
	}
      break;
    case PANGO_ATTR_STRIKETHROUGH_COLOR:
      if (ctk_builder_value_from_string_type (builder, GDK_TYPE_COLOR, value, &val, error))
	{
	  color = g_value_get_boxed (&val);
	  attribute = pango_attr_strikethrough_color_new (color->red, color->green, color->blue);
	}
      break;

G_GNUC_END_IGNORE_DEPRECATIONS

      /* PangoAttrShape */
    case PANGO_ATTR_SHAPE:
      /* Unsupported for now */
      break;
      /* PangoAttrFloat */
    case PANGO_ATTR_SCALE:
      if (ctk_builder_value_from_string_type (builder, G_TYPE_DOUBLE, value, &val, error))
	attribute = pango_attr_scale_new (g_value_get_double (&val));
      break;
    case PANGO_ATTR_LETTER_SPACING:
      if (ctk_builder_value_from_string_type (builder, G_TYPE_INT, value, &val, error))
        attribute = pango_attr_letter_spacing_new (g_value_get_int (&val));
      break;
    case PANGO_ATTR_RISE:
      if (ctk_builder_value_from_string_type (builder, G_TYPE_INT, value, &val, error))
        attribute = pango_attr_rise_new (g_value_get_int (&val));
      break;
    case PANGO_ATTR_FALLBACK:
      if (ctk_builder_value_from_string_type (builder, G_TYPE_BOOLEAN, value, &val, error))
        attribute = pango_attr_fallback_new (g_value_get_boolean (&val));
      break;
    case PANGO_ATTR_FONT_FEATURES:
      attribute = pango_attr_font_features_new (value);
      break;
    case PANGO_ATTR_FOREGROUND_ALPHA:
      if (ctk_builder_value_from_string_type (builder, G_TYPE_INT, value, &val, error))
        attribute = pango_attr_foreground_alpha_new ((guint16)g_value_get_int (&val));
      break;
    case PANGO_ATTR_BACKGROUND_ALPHA:
      if (ctk_builder_value_from_string_type (builder, G_TYPE_INT, value, &val, error))
        attribute = pango_attr_background_alpha_new ((guint16)g_value_get_int (&val));
      break;
    case PANGO_ATTR_INVALID:
    default:
      break;
    }

  g_value_unset (&val);

  return attribute;
}


static void
pango_start_element (GMarkupParseContext *context,
		     const gchar         *element_name,
		     const gchar        **names,
		     const gchar        **values,
		     gpointer             user_data,
		     GError             **error)
{
  PangoParserData *data = (PangoParserData*)user_data;

  if (strcmp (element_name, "attribute") == 0)
    {
      PangoAttribute *attr = NULL;
      const gchar *name = NULL;
      const gchar *value = NULL;
      const gchar *start = NULL;
      const gchar *end = NULL;
      guint start_val = 0;
      guint end_val = G_MAXUINT;
      GValue val = G_VALUE_INIT;

      if (!_ctk_builder_check_parent (data->builder, context, "attributes", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_STRING, "value", &value,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "start", &start,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "end", &end,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      if (start)
        {
          if (!ctk_builder_value_from_string_type (data->builder, G_TYPE_UINT, start, &val, error))
            {
              _ctk_builder_prefix_error (data->builder, context, error);
              return;
            }
          start_val = g_value_get_uint (&val);
          g_value_unset (&val);
        }

      if (end)
        {
          if (!ctk_builder_value_from_string_type (data->builder, G_TYPE_UINT, end, &val, error))
            {
              _ctk_builder_prefix_error (data->builder, context, error);
              return;
            }
          end_val = g_value_get_uint (&val);
          g_value_unset (&val);
        }

      attr = attribute_from_text (data->builder, name, value, error);
      if (!attr)
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      attr->start_index = start_val;
      attr->end_index = end_val;

      if (!data->attrs)
        data->attrs = pango_attr_list_new ();

      pango_attr_list_insert (data->attrs, attr);
    }
  else if (strcmp (element_name, "attributes") == 0)
    {
      if (!_ctk_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _ctk_builder_prefix_error (data->builder, context, error);
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkContainer", element_name,
                                        error);
    }
}

static const GMarkupParser pango_parser =
  {
    pango_start_element,
  };

static gboolean
ctk_label_buildable_custom_tag_start (CtkBuildable     *buildable,
				      CtkBuilder       *builder,
				      GObject          *child,
				      const gchar      *tagname,
				      GMarkupParser    *parser,
				      gpointer         *data)
{
  if (buildable_parent_iface->custom_tag_start (buildable, builder, child,
						tagname, parser, data))
    return TRUE;

  if (strcmp (tagname, "attributes") == 0)
    {
      PangoParserData *parser_data;

      parser_data = g_slice_new0 (PangoParserData);
      parser_data->builder = g_object_ref (builder);
      parser_data->object = G_OBJECT (g_object_ref (buildable));
      *parser = pango_parser;
      *data = parser_data;
      return TRUE;
    }
  return FALSE;
}

static void
ctk_label_buildable_custom_finished (CtkBuildable *buildable,
				     CtkBuilder   *builder,
				     GObject      *child,
				     const gchar  *tagname,
				     gpointer      user_data)
{
  PangoParserData *data;

  buildable_parent_iface->custom_finished (buildable, builder, child,
					   tagname, user_data);

  if (strcmp (tagname, "attributes") == 0)
    {
      data = (PangoParserData*)user_data;

      if (data->attrs)
	{
	  ctk_label_set_attributes (CTK_LABEL (buildable), data->attrs);
	  pango_attr_list_unref (data->attrs);
	}

      g_object_unref (data->object);
      g_object_unref (data->builder);
      g_slice_free (PangoParserData, data);
    }
}


/**
 * ctk_label_new:
 * @str: (nullable): The text of the label
 *
 * Creates a new label with the given text inside it. You can
 * pass %NULL to get an empty label widget.
 *
 * Returns: the new #CtkLabel
 **/
CtkWidget*
ctk_label_new (const gchar *str)
{
  CtkLabel *label;
  
  label = g_object_new (CTK_TYPE_LABEL, NULL);

  if (str && *str)
    ctk_label_set_text (label, str);
  
  return CTK_WIDGET (label);
}

/**
 * ctk_label_new_with_mnemonic:
 * @str: (nullable): The text of the label, with an underscore in front of the
 *       mnemonic character
 *
 * Creates a new #CtkLabel, containing the text in @str.
 *
 * If characters in @str are preceded by an underscore, they are
 * underlined. If you need a literal underscore character in a label, use
 * '__' (two underscores). The first underlined character represents a 
 * keyboard accelerator called a mnemonic. The mnemonic key can be used 
 * to activate another widget, chosen automatically, or explicitly using
 * ctk_label_set_mnemonic_widget().
 * 
 * If ctk_label_set_mnemonic_widget() is not called, then the first 
 * activatable ancestor of the #CtkLabel will be chosen as the mnemonic 
 * widget. For instance, if the label is inside a button or menu item, 
 * the button or menu item will automatically become the mnemonic widget 
 * and be activated by the mnemonic.
 *
 * Returns: the new #CtkLabel
 **/
CtkWidget*
ctk_label_new_with_mnemonic (const gchar *str)
{
  CtkLabel *label;
  
  label = g_object_new (CTK_TYPE_LABEL, NULL);

  if (str && *str)
    ctk_label_set_text_with_mnemonic (label, str);
  
  return CTK_WIDGET (label);
}

static gboolean
ctk_label_mnemonic_activate (CtkWidget *widget,
			     gboolean   group_cycling)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;
  CtkWidget *parent;

  if (priv->mnemonic_widget)
    return ctk_widget_mnemonic_activate (priv->mnemonic_widget, group_cycling);

  /* Try to find the widget to activate by traversing the
   * widget's ancestry.
   */
  parent = ctk_widget_get_parent (widget);

  if (CTK_IS_NOTEBOOK (parent))
    return FALSE;
  
  while (parent)
    {
      if (ctk_widget_get_can_focus (parent) ||
	  (!group_cycling && CTK_WIDGET_GET_CLASS (parent)->activate_signal) ||
          CTK_IS_NOTEBOOK (ctk_widget_get_parent (parent)) ||
	  CTK_IS_MENU_ITEM (parent))
	return ctk_widget_mnemonic_activate (parent, group_cycling);
      parent = ctk_widget_get_parent (parent);
    }

  /* barf if there was nothing to activate */
  g_warning ("Couldn't find a target for a mnemonic activation.");
  ctk_widget_error_bell (widget);

  return FALSE;
}

static void
ctk_label_setup_mnemonic (CtkLabel *label,
			  guint     last_key)
{
  CtkLabelPrivate *priv = label->priv;
  CtkWidget *widget = CTK_WIDGET (label);
  CtkWidget *toplevel;
  CtkWidget *mnemonic_menu;
  
  mnemonic_menu = g_object_get_qdata (G_OBJECT (label), quark_mnemonic_menu);
  
  if (last_key != GDK_KEY_VoidSymbol)
    {
      if (priv->mnemonic_window)
	{
	  ctk_window_remove_mnemonic  (priv->mnemonic_window,
				       last_key,
				       widget);
	  priv->mnemonic_window = NULL;
	}
      if (mnemonic_menu)
	{
	  _ctk_menu_shell_remove_mnemonic (CTK_MENU_SHELL (mnemonic_menu),
					   last_key,
					   widget);
	  mnemonic_menu = NULL;
	}
    }
  
  if (priv->mnemonic_keyval == GDK_KEY_VoidSymbol)
      goto done;

  connect_mnemonics_visible_notify (CTK_LABEL (widget));

  toplevel = ctk_widget_get_toplevel (widget);
  if (ctk_widget_is_toplevel (toplevel))
    {
      CtkWidget *menu_shell;
      
      menu_shell = ctk_widget_get_ancestor (widget,
					    CTK_TYPE_MENU_SHELL);

      if (menu_shell)
	{
	  _ctk_menu_shell_add_mnemonic (CTK_MENU_SHELL (menu_shell),
					priv->mnemonic_keyval,
					widget);
	  mnemonic_menu = menu_shell;
	}
      
      if (!CTK_IS_MENU (menu_shell))
	{
	  ctk_window_add_mnemonic (CTK_WINDOW (toplevel),
				   priv->mnemonic_keyval,
				   widget);
	  priv->mnemonic_window = CTK_WINDOW (toplevel);
	}
    }
  
 done:
  g_object_set_qdata (G_OBJECT (label), quark_mnemonic_menu, mnemonic_menu);
}

static void
ctk_label_hierarchy_changed (CtkWidget *widget,
			     CtkWidget *old_toplevel)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;

  ctk_label_setup_mnemonic (label, priv->mnemonic_keyval);
}

static void
label_shortcut_setting_apply (CtkLabel *label)
{
  ctk_label_recalculate (label);
  if (CTK_IS_ACCEL_LABEL (label))
    ctk_accel_label_refetch (CTK_ACCEL_LABEL (label));
}

static void
label_shortcut_setting_traverse_container (CtkWidget *widget,
                                           gpointer   data)
{
  if (CTK_IS_LABEL (widget))
    label_shortcut_setting_apply (CTK_LABEL (widget));
  else if (CTK_IS_CONTAINER (widget))
    ctk_container_forall (CTK_CONTAINER (widget),
                          label_shortcut_setting_traverse_container, data);
}

static void
label_shortcut_setting_changed (CtkSettings *settings)
{
  GList *list, *l;

  list = ctk_window_list_toplevels ();

  for (l = list; l ; l = l->next)
    {
      CtkWidget *widget = l->data;

      if (ctk_widget_get_settings (widget) == settings)
        ctk_container_forall (CTK_CONTAINER (widget),
                              label_shortcut_setting_traverse_container, NULL);
    }

  g_list_free (list);
}

static void
mnemonics_visible_apply (CtkWidget *widget,
                         gboolean   mnemonics_visible)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;

  mnemonics_visible = mnemonics_visible != FALSE;

  if (priv->mnemonics_visible != mnemonics_visible)
    {
      priv->mnemonics_visible = mnemonics_visible;

      ctk_label_recalculate (label);
    }
}

static void
label_mnemonics_visible_traverse_container (CtkWidget *widget,
                                            gpointer   data)
{
  gboolean mnemonics_visible = GPOINTER_TO_INT (data);

  _ctk_label_mnemonics_visible_apply_recursively (widget, mnemonics_visible);
}

void
_ctk_label_mnemonics_visible_apply_recursively (CtkWidget *widget,
                                                gboolean   mnemonics_visible)
{
  if (CTK_IS_LABEL (widget))
    mnemonics_visible_apply (widget, mnemonics_visible);
  else if (CTK_IS_CONTAINER (widget))
    ctk_container_forall (CTK_CONTAINER (widget),
                          label_mnemonics_visible_traverse_container,
                          GINT_TO_POINTER (mnemonics_visible));
}

static void
label_mnemonics_visible_changed (CtkWindow  *window,
                                 GParamSpec *pspec,
                                 gpointer    data)
{
  gboolean mnemonics_visible;

  g_object_get (window, "mnemonics-visible", &mnemonics_visible, NULL);

  ctk_container_forall (CTK_CONTAINER (window),
                        label_mnemonics_visible_traverse_container,
                        GINT_TO_POINTER (mnemonics_visible));
}

static void
ctk_label_screen_changed (CtkWidget *widget,
			  GdkScreen *old_screen)
{
  CtkSettings *settings;
  gboolean shortcuts_connected;

  /* The PangoContext is replaced when the screen changes, so clear the layouts */
  ctk_label_clear_layout (CTK_LABEL (widget));

  if (!ctk_widget_has_screen (widget))
    return;

  settings = ctk_widget_get_settings (widget);

  shortcuts_connected =
    GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (settings), quark_shortcuts_connected));

  if (! shortcuts_connected)
    {
      g_signal_connect (settings, "notify::ctk-enable-mnemonics",
                        G_CALLBACK (label_shortcut_setting_changed),
                        NULL);
      g_signal_connect (settings, "notify::ctk-enable-accels",
                        G_CALLBACK (label_shortcut_setting_changed),
                        NULL);

      g_object_set_qdata (G_OBJECT (settings), quark_shortcuts_connected,
                         GINT_TO_POINTER (TRUE));
    }

  label_shortcut_setting_apply (CTK_LABEL (widget));
}


static void
label_mnemonic_widget_weak_notify (gpointer      data,
				   GObject      *where_the_object_was)
{
  CtkLabel *label = data;
  CtkLabelPrivate *priv = label->priv;

  priv->mnemonic_widget = NULL;
  g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_MNEMONIC_WIDGET]);
}

/**
 * ctk_label_set_mnemonic_widget:
 * @label: a #CtkLabel
 * @widget: (nullable): the target #CtkWidget, or %NULL to unset
 *
 * If the label has been set so that it has an mnemonic key (using
 * i.e. ctk_label_set_markup_with_mnemonic(),
 * ctk_label_set_text_with_mnemonic(), ctk_label_new_with_mnemonic()
 * or the “use_underline” property) the label can be associated with a
 * widget that is the target of the mnemonic. When the label is inside
 * a widget (like a #CtkButton or a #CtkNotebook tab) it is
 * automatically associated with the correct widget, but sometimes
 * (i.e. when the target is a #CtkEntry next to the label) you need to
 * set it explicitly using this function.
 *
 * The target widget will be accelerated by emitting the 
 * CtkWidget::mnemonic-activate signal on it. The default handler for 
 * this signal will activate the widget if there are no mnemonic collisions 
 * and toggle focus between the colliding widgets otherwise.
 **/
void
ctk_label_set_mnemonic_widget (CtkLabel  *label,
			       CtkWidget *widget)
{
  CtkLabelPrivate *priv;

  g_return_if_fail (CTK_IS_LABEL (label));

  priv = label->priv;

  if (widget)
    g_return_if_fail (CTK_IS_WIDGET (widget));

  if (priv->mnemonic_widget)
    {
      ctk_widget_remove_mnemonic_label (priv->mnemonic_widget, CTK_WIDGET (label));
      g_object_weak_unref (G_OBJECT (priv->mnemonic_widget),
			   label_mnemonic_widget_weak_notify,
			   label);
    }
  priv->mnemonic_widget = widget;
  if (priv->mnemonic_widget)
    {
      g_object_weak_ref (G_OBJECT (priv->mnemonic_widget),
		         label_mnemonic_widget_weak_notify,
		         label);
      ctk_widget_add_mnemonic_label (priv->mnemonic_widget, CTK_WIDGET (label));
    }
  
  g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_MNEMONIC_WIDGET]);
}

/**
 * ctk_label_get_mnemonic_widget:
 * @label: a #CtkLabel
 *
 * Retrieves the target of the mnemonic (keyboard shortcut) of this
 * label. See ctk_label_set_mnemonic_widget().
 *
 * Returns: (nullable) (transfer none): the target of the label’s mnemonic,
 *     or %NULL if none has been set and the default algorithm will be used.
 **/
CtkWidget *
ctk_label_get_mnemonic_widget (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), NULL);

  return label->priv->mnemonic_widget;
}

/**
 * ctk_label_get_mnemonic_keyval:
 * @label: a #CtkLabel
 *
 * If the label has been set so that it has an mnemonic key this function
 * returns the keyval used for the mnemonic accelerator. If there is no
 * mnemonic set up it returns #GDK_KEY_VoidSymbol.
 *
 * Returns: GDK keyval usable for accelerators, or #GDK_KEY_VoidSymbol
 **/
guint
ctk_label_get_mnemonic_keyval (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), GDK_KEY_VoidSymbol);

  return label->priv->mnemonic_keyval;
}

static void
ctk_label_set_text_internal (CtkLabel *label,
                             gchar    *str)
{
  CtkLabelPrivate *priv = label->priv;

  if (g_strcmp0 (priv->text, str) == 0)
    {
      g_free (str);
      return;
    }

  _ctk_label_accessible_text_deleted (label);
  g_free (priv->text);
  priv->text = str;

  _ctk_label_accessible_text_inserted (label);

  ctk_label_select_region_index (label, 0, 0);
}

static void
ctk_label_set_label_internal (CtkLabel *label,
			      gchar    *str)
{
  CtkLabelPrivate *priv = label->priv;

  g_free (priv->label);

  priv->label = str;

  g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_LABEL]);
}

static gboolean
ctk_label_set_use_markup_internal (CtkLabel *label,
                                   gboolean  val)
{
  CtkLabelPrivate *priv = label->priv;

  val = val != FALSE;
  if (priv->use_markup != val)
    {
      priv->use_markup = val;

      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_USE_MARKUP]);

      return TRUE;
    }

  return FALSE;
}

static gboolean
ctk_label_set_use_underline_internal (CtkLabel *label,
                                      gboolean  val)
{
  CtkLabelPrivate *priv = label->priv;

  val = val != FALSE;
  if (priv->use_underline != val)
    {
      priv->use_underline = val;

      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_USE_UNDERLINE]);

      return TRUE;
    }

  return FALSE;
}

/* Calculates text, attrs and mnemonic_keyval from
 * label, use_underline and use_markup
 */
static void
ctk_label_recalculate (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;
  guint keyval = priv->mnemonic_keyval;

  ctk_label_clear_links (label);

  if (priv->use_markup)
    ctk_label_set_markup_internal (label, priv->label, priv->use_underline);
  else if (priv->use_underline)
    ctk_label_set_uline_text_internal (label, priv->label);
  else
    {
      if (!priv->pattern_set)
        {
          if (priv->markup_attrs)
            pango_attr_list_unref (priv->markup_attrs);
          priv->markup_attrs = NULL;
        }
      ctk_label_set_text_internal (label, g_strdup (priv->label));
    }

  if (!priv->use_underline)
    priv->mnemonic_keyval = GDK_KEY_VoidSymbol;

  if (keyval != priv->mnemonic_keyval)
    {
      ctk_label_setup_mnemonic (label, keyval);
      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_MNEMONIC_KEYVAL]);
    }

  ctk_label_clear_layout (label);
  ctk_label_clear_select_info (label);
  ctk_widget_queue_resize (CTK_WIDGET (label));
}

/**
 * ctk_label_set_text:
 * @label: a #CtkLabel
 * @str: The text you want to set
 *
 * Sets the text within the #CtkLabel widget. It overwrites any text that
 * was there before.  
 *
 * This function will clear any previously set mnemonic accelerators, and
 * set the #CtkLabel:use-underline property to %FALSE as a side effect.
 *
 * This function will set the #CtkLabel:use-markup property to %FALSE
 * as a side effect.
 *
 * See also: ctk_label_set_markup()
 **/
void
ctk_label_set_text (CtkLabel    *label,
		    const gchar *str)
{
  g_return_if_fail (CTK_IS_LABEL (label));
  
  g_object_freeze_notify (G_OBJECT (label));

  ctk_label_set_label_internal (label, g_strdup (str ? str : ""));
  ctk_label_set_use_markup_internal (label, FALSE);
  ctk_label_set_use_underline_internal (label, FALSE);
  
  ctk_label_recalculate (label);

  g_object_thaw_notify (G_OBJECT (label));
}

/**
 * ctk_label_set_attributes:
 * @label: a #CtkLabel
 * @attrs: (nullable): a #PangoAttrList, or %NULL
 *
 * Sets a #PangoAttrList; the attributes in the list are applied to the
 * label text.
 *
 * The attributes set with this function will be applied
 * and merged with any other attributes previously effected by way
 * of the #CtkLabel:use-underline or #CtkLabel:use-markup properties.
 * While it is not recommended to mix markup strings with manually set
 * attributes, if you must; know that the attributes will be applied
 * to the label after the markup string is parsed.
 **/
void
ctk_label_set_attributes (CtkLabel         *label,
                          PangoAttrList    *attrs)
{
  CtkLabelPrivate *priv = label->priv;

  g_return_if_fail (CTK_IS_LABEL (label));

  if (attrs)
    pango_attr_list_ref (attrs);

  if (priv->attrs)
    pango_attr_list_unref (priv->attrs);
  priv->attrs = attrs;

  g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_ATTRIBUTES]);

  ctk_label_clear_layout (label);
  ctk_widget_queue_resize (CTK_WIDGET (label));
}

/**
 * ctk_label_get_attributes:
 * @label: a #CtkLabel
 *
 * Gets the attribute list that was set on the label using
 * ctk_label_set_attributes(), if any. This function does
 * not reflect attributes that come from the labels markup
 * (see ctk_label_set_markup()). If you want to get the
 * effective attributes for the label, use
 * pango_layout_get_attribute (ctk_label_get_layout (label)).
 *
 * Returns: (nullable) (transfer none): the attribute list, or %NULL
 *     if none was set.
 **/
PangoAttrList *
ctk_label_get_attributes (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), NULL);

  return label->priv->attrs;
}

/**
 * ctk_label_set_label:
 * @label: a #CtkLabel
 * @str: the new text to set for the label
 *
 * Sets the text of the label. The label is interpreted as
 * including embedded underlines and/or Pango markup depending
 * on the values of the #CtkLabel:use-underline and
 * #CtkLabel:use-markup properties.
 **/
void
ctk_label_set_label (CtkLabel    *label,
		     const gchar *str)
{
  g_return_if_fail (CTK_IS_LABEL (label));

  g_object_freeze_notify (G_OBJECT (label));

  ctk_label_set_label_internal (label, g_strdup (str ? str : ""));
  ctk_label_recalculate (label);

  g_object_thaw_notify (G_OBJECT (label));
}

/**
 * ctk_label_get_label:
 * @label: a #CtkLabel
 *
 * Fetches the text from a label widget including any embedded
 * underlines indicating mnemonics and Pango markup. (See
 * ctk_label_get_text()).
 *
 * Returns: the text of the label widget. This string is
 *   owned by the widget and must not be modified or freed.
 **/
const gchar *
ctk_label_get_label (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), NULL);

  return label->priv->label;
}

typedef struct
{
  CtkLabel *label;
  GList *links;
  GString *new_str;
  gsize text_len;
} UriParserData;

static void
start_element_handler (GMarkupParseContext  *context,
                       const gchar          *element_name,
                       const gchar         **attribute_names,
                       const gchar         **attribute_values,
                       gpointer              user_data,
                       GError              **error)
{
  CtkLabelPrivate *priv;
  UriParserData *pdata = user_data;

  if (strcmp (element_name, "a") == 0)
    {
      CtkLabelLink *link;
      const gchar *uri = NULL;
      const gchar *title = NULL;
      gboolean visited = FALSE;
      gint line_number;
      gint char_number;
      gint i;
      CtkCssNode *widget_node;
      CtkStateFlags state;

      g_markup_parse_context_get_position (context, &line_number, &char_number);

      for (i = 0; attribute_names[i] != NULL; i++)
        {
          const gchar *attr = attribute_names[i];

          if (strcmp (attr, "href") == 0)
            uri = attribute_values[i];
          else if (strcmp (attr, "title") == 0)
            title = attribute_values[i];
          else
            {
              g_set_error (error,
                           G_MARKUP_ERROR,
                           G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                           "Attribute '%s' is not allowed on the <a> tag "
                           "on line %d char %d",
                            attr, line_number, char_number);
              return;
            }
        }

      if (uri == NULL)
        {
          g_set_error (error,
                       G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       "Attribute 'href' was missing on the <a> tag "
                       "on line %d char %d",
                       line_number, char_number);
          return;
        }

      visited = FALSE;
      priv = pdata->label->priv;
      if (priv->track_links && priv->select_info)
        {
          GList *l;
          for (l = priv->select_info->links; l; l = l->next)
            {
              link = l->data;
              if (strcmp (uri, link->uri) == 0)
                {
                  visited = link->visited;
                  break;
                }
            }
        }

      link = g_new0 (CtkLabelLink, 1);
      link->uri = g_strdup (uri);
      link->title = g_strdup (title);

      widget_node = ctk_widget_get_css_node (CTK_WIDGET (pdata->label));
      link->cssnode = ctk_css_node_new ();
      ctk_css_node_set_name (link->cssnode, I_("link"));
      ctk_css_node_set_parent (link->cssnode, widget_node);
      state = ctk_css_node_get_state (widget_node);
      if (visited)
        state |= CTK_STATE_FLAG_VISITED;
      else
        state |= CTK_STATE_FLAG_LINK;
      ctk_css_node_set_state (link->cssnode, state);
      g_object_unref (link->cssnode);

      link->visited = visited;
      link->start = pdata->text_len;
      pdata->links = g_list_prepend (pdata->links, link);
    }
  else
    {
      gint i;

      g_string_append_c (pdata->new_str, '<');
      g_string_append (pdata->new_str, element_name);

      for (i = 0; attribute_names[i] != NULL; i++)
        {
          const gchar *attr  = attribute_names[i];
          const gchar *value = attribute_values[i];
          gchar *newvalue;

          newvalue = g_markup_escape_text (value, -1);

          g_string_append_c (pdata->new_str, ' ');
          g_string_append (pdata->new_str, attr);
          g_string_append (pdata->new_str, "=\"");
          g_string_append (pdata->new_str, newvalue);
          g_string_append_c (pdata->new_str, '\"');

          g_free (newvalue);
        }
      g_string_append_c (pdata->new_str, '>');
    }
}

static void
end_element_handler (GMarkupParseContext  *context,
                     const gchar          *element_name,
                     gpointer              user_data,
                     GError              **error)
{
  UriParserData *pdata = user_data;

  if (!strcmp (element_name, "a"))
    {
      CtkLabelLink *link = pdata->links->data;
      link->end = pdata->text_len;
    }
  else
    {
      g_string_append (pdata->new_str, "</");
      g_string_append (pdata->new_str, element_name);
      g_string_append_c (pdata->new_str, '>');
    }
}

static void
text_handler (GMarkupParseContext  *context,
              const gchar          *text,
              gsize                 text_len,
              gpointer              user_data,
              GError              **error)
{
  UriParserData *pdata = user_data;
  gchar *newtext;

  newtext = g_markup_escape_text (text, text_len);
  g_string_append (pdata->new_str, newtext);
  pdata->text_len += text_len;
  g_free (newtext);
}

static const GMarkupParser markup_parser =
{
  start_element_handler,
  end_element_handler,
  text_handler,
  NULL,
  NULL
};

static gboolean
xml_isspace (gchar c)
{
  return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static void
link_free (CtkLabelLink *link)
{
  ctk_css_node_set_parent (link->cssnode, NULL);
  g_free (link->uri);
  g_free (link->title);
  g_free (link);
}


static gboolean
parse_uri_markup (CtkLabel     *label,
                  const gchar  *str,
                  gchar       **new_str,
                  GList       **links,
                  GError      **error)
{
  GMarkupParseContext *context = NULL;
  const gchar *p, *end;
  gboolean needs_root = TRUE;
  gsize length;
  UriParserData pdata;

  length = strlen (str);
  p = str;
  end = str + length;

  pdata.label = label;
  pdata.links = NULL;
  pdata.new_str = g_string_sized_new (length);
  pdata.text_len = 0;

  while (p != end && xml_isspace (*p))
    p++;

  if (end - p >= 8 && strncmp (p, "<markup>", 8) == 0)
    needs_root = FALSE;

  context = g_markup_parse_context_new (&markup_parser, 0, &pdata, NULL);

  if (needs_root)
    {
      if (!g_markup_parse_context_parse (context, "<markup>", -1, error))
        goto failed;
    }

  if (!g_markup_parse_context_parse (context, str, length, error))
    goto failed;

  if (needs_root)
    {
      if (!g_markup_parse_context_parse (context, "</markup>", -1, error))
        goto failed;
    }

  if (!g_markup_parse_context_end_parse (context, error))
    goto failed;

  g_markup_parse_context_free (context);

  *new_str = g_string_free (pdata.new_str, FALSE);
  *links = pdata.links;

  return TRUE;

failed:
  g_markup_parse_context_free (context);
  g_string_free (pdata.new_str, TRUE);
  g_list_free_full (pdata.links, (GDestroyNotify) link_free);

  return FALSE;
}

static void
ctk_label_ensure_has_tooltip (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;
  GList *l;
  gboolean has_tooltip = FALSE;

  for (l = priv->select_info->links; l; l = l->next)
    {
      CtkLabelLink *link = l->data;
      if (link->title)
        {
          has_tooltip = TRUE;
          break;
        }
    }

  ctk_widget_set_has_tooltip (CTK_WIDGET (label), has_tooltip);
}

static void
ctk_label_set_markup_internal (CtkLabel    *label,
                               const gchar *str,
                               gboolean     with_uline)
{
  CtkLabelPrivate *priv = label->priv;
  gchar *text = NULL;
  GError *error = NULL;
  PangoAttrList *attrs = NULL;
  gunichar accel_char = 0;
  gchar *str_for_display = NULL;
  gchar *str_for_accel = NULL;
  GList *links = NULL;

  if (!parse_uri_markup (label, str, &str_for_display, &links, &error))
    {
      g_warning ("Failed to set text '%s' from markup due to error parsing markup: %s",
                 str, error->message);
      g_error_free (error);
      return;
    }

  str_for_accel = g_strdup (str_for_display);

  if (links)
    {
      ctk_label_ensure_select_info (label);
      priv->select_info->links = g_list_reverse (links);
      _ctk_label_accessible_update_links (label);
      ctk_label_ensure_has_tooltip (label);
    }

  if (with_uline)
    {
      gboolean enable_mnemonics = TRUE;
      gboolean auto_mnemonics = TRUE;

      g_object_get (ctk_widget_get_settings (CTK_WIDGET (label)),
                    "ctk-enable-mnemonics", &enable_mnemonics,
                    NULL);

      if (!(enable_mnemonics && priv->mnemonics_visible &&
            (!auto_mnemonics ||
             (ctk_widget_is_sensitive (CTK_WIDGET (label)) &&
              (!priv->mnemonic_widget ||
               ctk_widget_is_sensitive (priv->mnemonic_widget))))))
        {
          gchar *tmp;
          gchar *pattern;
          guint key;

          if (separate_uline_pattern (str_for_display, &key, &tmp, &pattern))
            {
              g_free (str_for_display);
              str_for_display = tmp;
              g_free (pattern);
            }
        }
    }

  /* Extract the text to display */
  if (!pango_parse_markup (str_for_display,
                           -1,
                           with_uline ? '_' : 0,
                           &attrs,
                           &text,
                           NULL,
                           &error))
    {
      g_warning ("Failed to set text '%s' from markup due to error parsing markup: %s",
                 str_for_display, error->message);
      g_free (str_for_display);
      g_free (str_for_accel);
      g_error_free (error);
      return;
    }

  /* Extract the accelerator character */
  if (with_uline && !pango_parse_markup (str_for_accel,
					 -1,
					 '_',
					 NULL,
					 NULL,
					 &accel_char,
					 &error))
    {
      g_warning ("Failed to set text from markup due to error parsing markup: %s",
                 error->message);
      g_free (str_for_display);
      g_free (str_for_accel);
      g_error_free (error);
      return;
    }

  g_free (str_for_display);
  g_free (str_for_accel);

  if (text)
    ctk_label_set_text_internal (label, text);

  if (attrs)
    {
      if (priv->markup_attrs)
	pango_attr_list_unref (priv->markup_attrs);
      priv->markup_attrs = attrs;
    }

  if (accel_char != 0)
    priv->mnemonic_keyval = cdk_keyval_to_lower (cdk_unicode_to_keyval (accel_char));
  else
    priv->mnemonic_keyval = GDK_KEY_VoidSymbol;
}

/**
 * ctk_label_set_markup:
 * @label: a #CtkLabel
 * @str: a markup string (see [Pango markup format][PangoMarkupFormat])
 *
 * Parses @str which is marked up with the
 * [Pango text markup language][PangoMarkupFormat], setting the
 * label’s text and attribute list based on the parse results.
 *
 * If the @str is external data, you may need to escape it with
 * g_markup_escape_text() or g_markup_printf_escaped():
 *
 * |[<!-- language="C" -->
 * CtkWidget *label = ctk_label_new (NULL);
 * const char *str = "some text";
 * const char *format = "<span style=\"italic\">\%s</span>";
 * char *markup;
 *
 * markup = g_markup_printf_escaped (format, str);
 * ctk_label_set_markup (CTK_LABEL (label), markup);
 * g_free (markup);
 * ]|
 *
 * This function will set the #CtkLabel:use-markup property to %TRUE as
 * a side effect.
 *
 * If you set the label contents using the #CtkLabel:label property you
 * should also ensure that you set the #CtkLabel:use-markup property
 * accordingly.
 *
 * See also: ctk_label_set_text()
 **/
void
ctk_label_set_markup (CtkLabel    *label,
                      const gchar *str)
{
  g_return_if_fail (CTK_IS_LABEL (label));

  g_object_freeze_notify (G_OBJECT (label));

  ctk_label_set_label_internal (label, g_strdup (str ? str : ""));
  ctk_label_set_use_markup_internal (label, TRUE);
  ctk_label_set_use_underline_internal (label, FALSE);

  ctk_label_recalculate (label);

  g_object_thaw_notify (G_OBJECT (label));
}

/**
 * ctk_label_set_markup_with_mnemonic:
 * @label: a #CtkLabel
 * @str: a markup string (see
 *     [Pango markup format][PangoMarkupFormat])
 *
 * Parses @str which is marked up with the
 * [Pango text markup language][PangoMarkupFormat],
 * setting the label’s text and attribute list based on the parse results.
 * If characters in @str are preceded by an underscore, they are underlined
 * indicating that they represent a keyboard accelerator called a mnemonic.
 *
 * The mnemonic key can be used to activate another widget, chosen
 * automatically, or explicitly using ctk_label_set_mnemonic_widget().
 */
void
ctk_label_set_markup_with_mnemonic (CtkLabel    *label,
                                    const gchar *str)
{
  g_return_if_fail (CTK_IS_LABEL (label));

  g_object_freeze_notify (G_OBJECT (label));

  ctk_label_set_label_internal (label, g_strdup (str ? str : ""));
  ctk_label_set_use_markup_internal (label, TRUE);
  ctk_label_set_use_underline_internal (label, TRUE);

  ctk_label_recalculate (label);

  g_object_thaw_notify (G_OBJECT (label));
}

/**
 * ctk_label_get_text:
 * @label: a #CtkLabel
 * 
 * Fetches the text from a label widget, as displayed on the
 * screen. This does not include any embedded underlines
 * indicating mnemonics or Pango markup. (See ctk_label_get_label())
 * 
 * Returns: the text in the label widget. This is the internal
 *   string used by the label, and must not be modified.
 **/
const gchar *
ctk_label_get_text (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), NULL);

  return label->priv->text;
}

static PangoAttrList *
ctk_label_pattern_to_attrs (CtkLabel      *label,
			    const gchar   *pattern)
{
  CtkLabelPrivate *priv = label->priv;
  const char *start;
  const char *p = priv->text;
  const char *q = pattern;
  PangoAttrList *attrs;

  attrs = pango_attr_list_new ();

  while (1)
    {
      while (*p && *q && *q != '_')
	{
	  p = g_utf8_next_char (p);
	  q++;
	}
      start = p;
      while (*p && *q && *q == '_')
	{
	  p = g_utf8_next_char (p);
	  q++;
	}
      
      if (p > start)
	{
	  PangoAttribute *attr = pango_attr_underline_new (PANGO_UNDERLINE_LOW);
	  attr->start_index = start - priv->text;
	  attr->end_index = p - priv->text;
	  
	  pango_attr_list_insert (attrs, attr);
	}
      else
	break;
    }

  return attrs;
}

static void
ctk_label_set_pattern_internal (CtkLabel    *label,
				const gchar *pattern,
                                gboolean     is_mnemonic)
{
  CtkLabelPrivate *priv = label->priv;
  PangoAttrList *attrs;
  gboolean enable_mnemonics = TRUE;
  gboolean auto_mnemonics = TRUE;

  if (priv->pattern_set)
    return;

  if (is_mnemonic)
    {
      g_object_get (ctk_widget_get_settings (CTK_WIDGET (label)),
                    "ctk-enable-mnemonics", &enable_mnemonics,
                    NULL);

      if (enable_mnemonics && priv->mnemonics_visible && pattern &&
          (!auto_mnemonics ||
           (ctk_widget_is_sensitive (CTK_WIDGET (label)) &&
            (!priv->mnemonic_widget ||
             ctk_widget_is_sensitive (priv->mnemonic_widget)))))
        attrs = ctk_label_pattern_to_attrs (label, pattern);
      else
        attrs = NULL;
    }
  else
    attrs = ctk_label_pattern_to_attrs (label, pattern);

  if (priv->markup_attrs)
    pango_attr_list_unref (priv->markup_attrs);
  priv->markup_attrs = attrs;
}

/**
 * ctk_label_set_pattern:
 * @label: The #CtkLabel you want to set the pattern to.
 * @pattern: The pattern as described above.
 *
 * The pattern of underlines you want under the existing text within the
 * #CtkLabel widget.  For example if the current text of the label says
 * “FooBarBaz” passing a pattern of “___   ___” will underline
 * “Foo” and “Baz” but not “Bar”.
 */
void
ctk_label_set_pattern (CtkLabel	   *label,
		       const gchar *pattern)
{
  CtkLabelPrivate *priv;

  g_return_if_fail (CTK_IS_LABEL (label));

  priv = label->priv;

  priv->pattern_set = FALSE;

  if (pattern)
    {
      ctk_label_set_pattern_internal (label, pattern, FALSE);
      priv->pattern_set = TRUE;
    }
  else
    ctk_label_recalculate (label);

  ctk_label_clear_layout (label);
  ctk_widget_queue_resize (CTK_WIDGET (label));
}


/**
 * ctk_label_set_justify:
 * @label: a #CtkLabel
 * @jtype: a #CtkJustification
 *
 * Sets the alignment of the lines in the text of the label relative to
 * each other. %CTK_JUSTIFY_LEFT is the default value when the widget is
 * first created with ctk_label_new(). If you instead want to set the
 * alignment of the label as a whole, use ctk_widget_set_halign() instead.
 * ctk_label_set_justify() has no effect on labels containing only a
 * single line.
 */
void
ctk_label_set_justify (CtkLabel        *label,
		       CtkJustification jtype)
{
  CtkLabelPrivate *priv;

  g_return_if_fail (CTK_IS_LABEL (label));
  g_return_if_fail (jtype >= CTK_JUSTIFY_LEFT && jtype <= CTK_JUSTIFY_FILL);

  priv = label->priv;

  if ((CtkJustification) priv->jtype != jtype)
    {
      priv->jtype = jtype;

      /* No real need to be this drastic, but easier than duplicating the code */
      ctk_label_clear_layout (label);
      
      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_JUSTIFY]);
      ctk_widget_queue_resize (CTK_WIDGET (label));
    }
}

/**
 * ctk_label_get_justify:
 * @label: a #CtkLabel
 *
 * Returns the justification of the label. See ctk_label_set_justify().
 *
 * Returns: #CtkJustification
 **/
CtkJustification
ctk_label_get_justify (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), 0);

  return label->priv->jtype;
}

/**
 * ctk_label_set_ellipsize:
 * @label: a #CtkLabel
 * @mode: a #PangoEllipsizeMode
 *
 * Sets the mode used to ellipsize (add an ellipsis: "...") to the text 
 * if there is not enough space to render the entire string.
 *
 * Since: 2.6
 **/
void
ctk_label_set_ellipsize (CtkLabel          *label,
			 PangoEllipsizeMode mode)
{
  CtkLabelPrivate *priv;

  g_return_if_fail (CTK_IS_LABEL (label));
  g_return_if_fail (mode >= PANGO_ELLIPSIZE_NONE && mode <= PANGO_ELLIPSIZE_END);

  priv = label->priv;

  if ((PangoEllipsizeMode) priv->ellipsize != mode)
    {
      priv->ellipsize = mode;

      /* No real need to be this drastic, but easier than duplicating the code */
      ctk_label_clear_layout (label);

      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_ELLIPSIZE]);
      ctk_widget_queue_resize (CTK_WIDGET (label));
    }
}

/**
 * ctk_label_get_ellipsize:
 * @label: a #CtkLabel
 *
 * Returns the ellipsizing position of the label. See ctk_label_set_ellipsize().
 *
 * Returns: #PangoEllipsizeMode
 *
 * Since: 2.6
 **/
PangoEllipsizeMode
ctk_label_get_ellipsize (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), PANGO_ELLIPSIZE_NONE);

  return label->priv->ellipsize;
}

/**
 * ctk_label_set_width_chars:
 * @label: a #CtkLabel
 * @n_chars: the new desired width, in characters.
 * 
 * Sets the desired width in characters of @label to @n_chars.
 * 
 * Since: 2.6
 **/
void
ctk_label_set_width_chars (CtkLabel *label,
			   gint      n_chars)
{
  CtkLabelPrivate *priv;

  g_return_if_fail (CTK_IS_LABEL (label));

  priv = label->priv;

  if (priv->width_chars != n_chars)
    {
      priv->width_chars = n_chars;
      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_WIDTH_CHARS]);
      ctk_widget_queue_resize (CTK_WIDGET (label));
    }
}

/**
 * ctk_label_get_width_chars:
 * @label: a #CtkLabel
 * 
 * Retrieves the desired width of @label, in characters. See
 * ctk_label_set_width_chars().
 * 
 * Returns: the width of the label in characters.
 * 
 * Since: 2.6
 **/
gint
ctk_label_get_width_chars (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), -1);

  return label->priv->width_chars;
}

/**
 * ctk_label_set_max_width_chars:
 * @label: a #CtkLabel
 * @n_chars: the new desired maximum width, in characters.
 * 
 * Sets the desired maximum width in characters of @label to @n_chars.
 * 
 * Since: 2.6
 **/
void
ctk_label_set_max_width_chars (CtkLabel *label,
			       gint      n_chars)
{
  CtkLabelPrivate *priv;

  g_return_if_fail (CTK_IS_LABEL (label));

  priv = label->priv;

  if (priv->max_width_chars != n_chars)
    {
      priv->max_width_chars = n_chars;

      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_MAX_WIDTH_CHARS]);
      ctk_widget_queue_resize (CTK_WIDGET (label));
    }
}

/**
 * ctk_label_get_max_width_chars:
 * @label: a #CtkLabel
 * 
 * Retrieves the desired maximum width of @label, in characters. See
 * ctk_label_set_width_chars().
 * 
 * Returns: the maximum width of the label in characters.
 * 
 * Since: 2.6
 **/
gint
ctk_label_get_max_width_chars (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), -1);

  return label->priv->max_width_chars;
}

/**
 * ctk_label_set_line_wrap:
 * @label: a #CtkLabel
 * @wrap: the setting
 *
 * Toggles line wrapping within the #CtkLabel widget. %TRUE makes it break
 * lines if text exceeds the widget’s size. %FALSE lets the text get cut off
 * by the edge of the widget if it exceeds the widget size.
 *
 * Note that setting line wrapping to %TRUE does not make the label
 * wrap at its parent container’s width, because CTK+ widgets
 * conceptually can’t make their requisition depend on the parent
 * container’s size. For a label that wraps at a specific position,
 * set the label’s width using ctk_widget_set_size_request().
 **/
void
ctk_label_set_line_wrap (CtkLabel *label,
			 gboolean  wrap)
{
  CtkLabelPrivate *priv;

  g_return_if_fail (CTK_IS_LABEL (label));

  priv = label->priv;

  wrap = wrap != FALSE;

  if (priv->wrap != wrap)
    {
      priv->wrap = wrap;

      ctk_label_clear_layout (label);
      ctk_widget_queue_resize (CTK_WIDGET (label));
      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_WRAP]);
    }
}

/**
 * ctk_label_get_line_wrap:
 * @label: a #CtkLabel
 *
 * Returns whether lines in the label are automatically wrapped. 
 * See ctk_label_set_line_wrap().
 *
 * Returns: %TRUE if the lines of the label are automatically wrapped.
 */
gboolean
ctk_label_get_line_wrap (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), FALSE);

  return label->priv->wrap;
}

/**
 * ctk_label_set_line_wrap_mode:
 * @label: a #CtkLabel
 * @wrap_mode: the line wrapping mode
 *
 * If line wrapping is on (see ctk_label_set_line_wrap()) this controls how
 * the line wrapping is done. The default is %PANGO_WRAP_WORD which means
 * wrap on word boundaries.
 *
 * Since: 2.10
 **/
void
ctk_label_set_line_wrap_mode (CtkLabel *label,
			      PangoWrapMode wrap_mode)
{
  CtkLabelPrivate *priv;

  g_return_if_fail (CTK_IS_LABEL (label));

  priv = label->priv;

  if (priv->wrap_mode != wrap_mode)
    {
      priv->wrap_mode = wrap_mode;
      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_WRAP_MODE]);

      ctk_widget_queue_resize (CTK_WIDGET (label));
    }
}

/**
 * ctk_label_get_line_wrap_mode:
 * @label: a #CtkLabel
 *
 * Returns line wrap mode used by the label. See ctk_label_set_line_wrap_mode().
 *
 * Returns: %TRUE if the lines of the label are automatically wrapped.
 *
 * Since: 2.10
 */
PangoWrapMode
ctk_label_get_line_wrap_mode (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), FALSE);

  return label->priv->wrap_mode;
}

static void
ctk_label_destroy (CtkWidget *widget)
{
  CtkLabel *label = CTK_LABEL (widget);

  ctk_label_set_mnemonic_widget (label, NULL);

  CTK_WIDGET_CLASS (ctk_label_parent_class)->destroy (widget);
}

static void
ctk_label_finalize (GObject *object)
{
  CtkLabel *label = CTK_LABEL (object);
  CtkLabelPrivate *priv = label->priv;

  g_free (priv->label);
  g_free (priv->text);

  g_clear_object (&priv->layout);
  g_clear_pointer (&priv->attrs, pango_attr_list_unref);
  g_clear_pointer (&priv->markup_attrs, pango_attr_list_unref);

  if (priv->select_info)
    {
      g_object_unref (priv->select_info->drag_gesture);
      g_object_unref (priv->select_info->multipress_gesture);
    }

  ctk_label_clear_links (label);
  g_free (priv->select_info);

  g_clear_object (&priv->gadget);

  G_OBJECT_CLASS (ctk_label_parent_class)->finalize (object);
}

static void
ctk_label_clear_layout (CtkLabel *label)
{
  g_clear_object (&label->priv->layout);
}

/**
 * ctk_label_get_measuring_layout:
 * @label: the label
 * @existing_layout: %NULL or an existing layout already in use.
 * @width: the width to measure with in pango units, or -1 for infinite
 *
 * Gets a layout that can be used for measuring sizes. The returned
 * layout will be identical to the label’s layout except for the
 * layout’s width, which will be set to @width. Do not modify the returned
 * layout.
 *
 * Returns: a new reference to a pango layout
 **/
static PangoLayout *
ctk_label_get_measuring_layout (CtkLabel *   label,
                                PangoLayout *existing_layout,
                                int          width)
{
  CtkLabelPrivate *priv = label->priv;
  PangoRectangle rect;
  PangoLayout *copy;

  if (existing_layout != NULL)
    {
      if (existing_layout != priv->layout)
        {
          pango_layout_set_width (existing_layout, width);
          return existing_layout;
        }

      g_object_unref (existing_layout);
    }

  ctk_label_ensure_layout (label);

  if (pango_layout_get_width (priv->layout) == width)
    {
      g_object_ref (priv->layout);
      return priv->layout;
    }

  /* We can use the label's own layout if we're not allocated a size yet,
   * because we don't need it to be properly setup at that point.
   * This way we can make use of caching upon the label's creation.
   */
  if (ctk_widget_get_allocated_width (CTK_WIDGET (label)) <= 1)
    {
      g_object_ref (priv->layout);
      pango_layout_set_width (priv->layout, width);
      return priv->layout;
    }

  /* oftentimes we want to measure a width that is far wider than the current width,
   * even though the layout would not change if we made it wider. In that case, we
   * can just return the current layout, because for measuring purposes, it will be
   * identical.
   */
  pango_layout_get_extents (priv->layout, NULL, &rect);
  if ((width == -1 || rect.width <= width) &&
      !pango_layout_is_wrapped (priv->layout) &&
      !pango_layout_is_ellipsized (priv->layout))
    {
      g_object_ref (priv->layout);
      return priv->layout;
    }

  copy = pango_layout_copy (priv->layout);
  pango_layout_set_width (copy, width);
  return copy;
}

static void
ctk_label_update_layout_width (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;
  CtkWidget *widget = CTK_WIDGET (label);

  g_assert (priv->layout);

  if (priv->ellipsize || priv->wrap)
    {
      CtkAllocation allocation;
      int xpad, ypad;
      PangoRectangle logical;
      gint width, height;

      ctk_css_gadget_get_content_allocation (priv->gadget, &allocation, NULL);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_misc_get_padding (CTK_MISC (label), &xpad, &ypad);
G_GNUC_END_IGNORE_DEPRECATIONS

      width = allocation.width - 2 * xpad;
      height = allocation.height - 2 * ypad;

      if (priv->have_transform)
        {
          PangoContext *context = ctk_widget_get_pango_context (widget);
          const PangoMatrix *matrix = pango_context_get_matrix (context);
          const gdouble dx = matrix->xx; /* cos (M_PI * angle / 180) */
          const gdouble dy = matrix->xy; /* sin (M_PI * angle / 180) */

          pango_layout_set_width (priv->layout, -1);
          pango_layout_get_pixel_extents (priv->layout, NULL, &logical);

          if (fabs (dy) < 0.01)
            {
              if (logical.width > width)
                pango_layout_set_width (priv->layout, width * PANGO_SCALE);
            }
          else if (fabs (dx) < 0.01)
            {
              if (logical.width > height)
                pango_layout_set_width (priv->layout, height * PANGO_SCALE);
            }
          else
            {
              gdouble x0, y0, x1, y1, length;
              gboolean vertical;
              gint cy;

              x0 = width / 2;
              y0 = dx ? x0 * dy / dx : G_MAXDOUBLE;
              vertical = fabs (y0) > height / 2;

              if (vertical)
                {
                  y0 = height/2;
                  x0 = dy ? y0 * dx / dy : G_MAXDOUBLE;
                }

              length = 2 * sqrt (x0 * x0 + y0 * y0);
              pango_layout_set_width (priv->layout, rint (length * PANGO_SCALE));
              pango_layout_get_pixel_size (priv->layout, NULL, &cy);

              x1 = +dy * cy/2;
              y1 = -dx * cy/2;

              if (vertical)
                {
                  y0 = height/2 + y1 - y0;
                  x0 = -y0 * dx/dy;
                }
              else
                {
                  x0 = width/2 + x1 - x0;
                  y0 = -x0 * dy/dx;
                }

              length = length - sqrt (x0 * x0 + y0 * y0) * 2;
              pango_layout_set_width (priv->layout, rint (length * PANGO_SCALE));
            }
        }
      else
        {
          pango_layout_set_width (priv->layout, width * PANGO_SCALE);
        }
    }
  else
    {
      pango_layout_set_width (priv->layout, -1);
    }
}

static void
ctk_label_update_layout_attributes (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;
  CtkWidget *widget = CTK_WIDGET (label);
  CtkStyleContext *context;
  PangoAttrList *attrs;
  PangoAttrList *style_attrs;

  if (priv->layout == NULL)
    return;

  context = ctk_widget_get_style_context (widget);

  if (priv->select_info && priv->select_info->links)
    {
      GdkRGBA link_color;
      PangoAttribute *attribute;
      GList *list;

      attrs = pango_attr_list_new ();

      for (list = priv->select_info->links; list; list = list->next)
        {
          CtkLabelLink *link = list->data;

          attribute = pango_attr_underline_new (TRUE);
          attribute->start_index = link->start;
          attribute->end_index = link->end;
          pango_attr_list_insert (attrs, attribute);

          ctk_style_context_save_to_node (context, link->cssnode);
          ctk_style_context_get_color (context, ctk_style_context_get_state (context), &link_color);
          ctk_style_context_restore (context);

          attribute = pango_attr_foreground_new (link_color.red * 65535,
                                                 link_color.green * 65535,
                                                 link_color.blue * 65535);
          attribute->start_index = link->start;
          attribute->end_index = link->end;
          pango_attr_list_insert (attrs, attribute);
        }
    }
  else if (priv->markup_attrs && priv->attrs)
    attrs = pango_attr_list_new ();
  else
    attrs = NULL;

  style_attrs = _ctk_style_context_get_pango_attributes (context);

  attrs = _ctk_pango_attr_list_merge (attrs, style_attrs);
  attrs = _ctk_pango_attr_list_merge (attrs, priv->markup_attrs);
  attrs = _ctk_pango_attr_list_merge (attrs, priv->attrs);

  pango_layout_set_attributes (priv->layout, attrs);

  if (attrs)
    pango_attr_list_unref (attrs);
  if (style_attrs)
    pango_attr_list_unref (style_attrs);
}

static void
ctk_label_ensure_layout (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;
  CtkWidget *widget;
  gboolean rtl;

  widget = CTK_WIDGET (label);

  rtl = ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL;

  if (!priv->layout)
    {
      PangoAlignment align = PANGO_ALIGN_LEFT; /* Quiet gcc */
      gdouble angle = ctk_label_get_angle (label);

      if (angle != 0.0 && !priv->select_info)
	{
          PangoMatrix matrix = PANGO_MATRIX_INIT;

	  /* We rotate the standard singleton PangoContext for the widget,
	   * depending on the fact that it's meant pretty much exclusively
	   * for our use.
	   */
	  pango_matrix_rotate (&matrix, angle);

	  pango_context_set_matrix (ctk_widget_get_pango_context (widget), &matrix);

	  priv->have_transform = TRUE;
	}
      else
	{
	  if (priv->have_transform)
	    pango_context_set_matrix (ctk_widget_get_pango_context (widget), NULL);

	  priv->have_transform = FALSE;
	}

      priv->layout = ctk_widget_create_pango_layout (widget, priv->text);

      ctk_label_update_layout_attributes (label);

      switch (priv->jtype)
	{
	case CTK_JUSTIFY_LEFT:
	  align = rtl ? PANGO_ALIGN_RIGHT : PANGO_ALIGN_LEFT;
	  break;
	case CTK_JUSTIFY_RIGHT:
	  align = rtl ? PANGO_ALIGN_LEFT : PANGO_ALIGN_RIGHT;
	  break;
	case CTK_JUSTIFY_CENTER:
	  align = PANGO_ALIGN_CENTER;
	  break;
	case CTK_JUSTIFY_FILL:
	  align = rtl ? PANGO_ALIGN_RIGHT : PANGO_ALIGN_LEFT;
	  pango_layout_set_justify (priv->layout, TRUE);
	  break;
	default:
	  g_assert_not_reached();
	}

      pango_layout_set_alignment (priv->layout, align);
      pango_layout_set_ellipsize (priv->layout, priv->ellipsize);
      pango_layout_set_wrap (priv->layout, priv->wrap_mode);
      pango_layout_set_single_paragraph_mode (priv->layout, priv->single_line_mode);
      if (priv->lines > 0)
        pango_layout_set_height (priv->layout, - priv->lines);

      ctk_label_update_layout_width (label);
    }
}

static CtkSizeRequestMode
ctk_label_get_request_mode (CtkWidget *widget)
{
  CtkLabel *label = CTK_LABEL (widget);
  gdouble angle;

  angle = ctk_label_get_angle (label);

  if (label->priv->wrap)
    return (angle == 90 || angle == 270)
           ? CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT
           : CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;

  return CTK_SIZE_REQUEST_CONSTANT_SIZE;
}


static void
get_size_for_allocation (CtkLabel *label,
                         gint      allocation,
                         gint     *minimum_size,
                         gint     *natural_size,
			 gint     *minimum_baseline,
                         gint     *natural_baseline)
{
  PangoLayout *layout;
  gint text_height, baseline;

  layout = ctk_label_get_measuring_layout (label, NULL, allocation * PANGO_SCALE);

  pango_layout_get_pixel_size (layout, NULL, &text_height);

  *minimum_size = text_height;
  *natural_size = text_height;

  if (minimum_baseline || natural_baseline)
    {
      baseline = pango_layout_get_baseline (layout) / PANGO_SCALE;
      *minimum_baseline = baseline;
      *natural_baseline = baseline;
    }

  g_object_unref (layout);
}

static gint
get_char_pixels (CtkWidget   *label,
                 PangoLayout *layout)
{
  PangoContext *context;
  PangoFontMetrics *metrics;
  gint char_width, digit_width;

  context = pango_layout_get_context (layout);
  metrics = pango_context_get_metrics (context,
                                       pango_context_get_font_description (context),
                                       pango_context_get_language (context));
  char_width = pango_font_metrics_get_approximate_char_width (metrics);
  digit_width = pango_font_metrics_get_approximate_digit_width (metrics);
  pango_font_metrics_unref (metrics);

  return MAX (char_width, digit_width);;
}

static void
ctk_label_get_preferred_layout_size (CtkLabel *label,
                                     PangoRectangle *smallest,
                                     PangoRectangle *widest)
{
  CtkLabelPrivate *priv = label->priv;
  PangoLayout *layout;
  gint char_pixels;

  /* "width-chars" Hard-coded minimum width:
   *    - minimum size should be MAX (width-chars, strlen ("..."));
   *    - natural size should be MAX (width-chars, strlen (priv->text));
   *
   * "max-width-chars" User specified maximum size requisition
   *    - minimum size should be MAX (width-chars, 0)
   *    - natural size should be MIN (max-width-chars, strlen (priv->text))
   *
   *    For ellipsizing labels; if max-width-chars is specified: either it is used as 
   *    a minimum size or the label text as a minimum size (natural size still overflows).
   *
   *    For wrapping labels; A reasonable minimum size is useful to naturally layout
   *    interfaces automatically. In this case if no "width-chars" is specified, the minimum
   *    width will default to the wrap guess that ctk_label_ensure_layout() does.
   */

  /* Start off with the pixel extents of an as-wide-as-possible layout */
  layout = ctk_label_get_measuring_layout (label, NULL, -1);

  if (priv->width_chars > -1 || priv->max_width_chars > -1)
    char_pixels = get_char_pixels (CTK_WIDGET (label), layout);
  else
    char_pixels = 0;
      
  pango_layout_get_extents (layout, NULL, widest);
  widest->width = MAX (widest->width, char_pixels * priv->width_chars);
  widest->x = widest->y = 0;

  if (priv->ellipsize || priv->wrap)
    {
      /* a layout with width 0 will be as small as humanly possible */
      layout = ctk_label_get_measuring_layout (label,
                                               layout,
                                               priv->width_chars > -1 ? char_pixels * priv->width_chars
                                                                      : 0);

      pango_layout_get_extents (layout, NULL, smallest);
      smallest->width = MAX (smallest->width, char_pixels * priv->width_chars);
      smallest->x = smallest->y = 0;

      if (priv->max_width_chars > -1 && widest->width > char_pixels * priv->max_width_chars)
        {
          layout = ctk_label_get_measuring_layout (label,
                                                   layout,
                                                   MAX (smallest->width, char_pixels * priv->max_width_chars));
          pango_layout_get_extents (layout, NULL, widest);
          widest->width = MAX (widest->width, char_pixels * priv->width_chars);
          widest->x = widest->y = 0;
        }
    }
  else
    {
      *smallest = *widest;
    }

  if (widest->width < smallest->width)
    *smallest = *widest;

  g_object_unref (layout);
}

static void
ctk_label_get_preferred_size (CtkWidget      *widget,
                              CtkOrientation  orientation,
                              gint           *minimum_size,
                              gint           *natural_size,
			      gint           *minimum_baseline,
			      gint           *natural_baseline)
{
  CtkLabel      *label = CTK_LABEL (widget);
  CtkLabelPrivate  *priv = label->priv;
  gint xpad, ypad;
  PangoRectangle widest_rect;
  PangoRectangle smallest_rect;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_misc_get_padding (CTK_MISC (label), &xpad, &ypad);
G_GNUC_END_IGNORE_DEPRECATIONS

  ctk_label_get_preferred_layout_size (label, &smallest_rect, &widest_rect);

  /* Now that we have minimum and natural sizes in pango extents, apply a possible transform */
  if (priv->have_transform)
    {
      PangoContext *context;
      const PangoMatrix *matrix;

      context = pango_layout_get_context (priv->layout);
      matrix = pango_context_get_matrix (context);

      pango_matrix_transform_rectangle (matrix, &widest_rect);
      pango_matrix_transform_rectangle (matrix, &smallest_rect);

      /* Bump the size in case of ellipsize to ensure pango has
       * enough space in the angles (note, we could alternatively set the
       * layout to not ellipsize when we know we have been allocated our
       * full size, or it may be that pango needs a fix here).
       */
      if (priv->ellipsize && priv->angle != 0 && priv->angle != 90 && 
          priv->angle != 180 && priv->angle != 270 && priv->angle != 360)
        {
          /* For some reason we only need this at about 110 degrees, and only
           * when gaining in height
           */
          widest_rect.height += ROTATION_ELLIPSIZE_PADDING * 2 * PANGO_SCALE;
          widest_rect.width  += ROTATION_ELLIPSIZE_PADDING * 2 * PANGO_SCALE;
          smallest_rect.height += ROTATION_ELLIPSIZE_PADDING * 2 * PANGO_SCALE;
          smallest_rect.width  += ROTATION_ELLIPSIZE_PADDING * 2 * PANGO_SCALE;
        }
    }

  widest_rect.width  = PANGO_PIXELS_CEIL (widest_rect.width);
  widest_rect.height = PANGO_PIXELS_CEIL (widest_rect.height);

  smallest_rect.width  = PANGO_PIXELS_CEIL (smallest_rect.width);
  smallest_rect.height = PANGO_PIXELS_CEIL (smallest_rect.height);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      /* Note, we cant use get_size_for_allocation() when rotating
       * ellipsized labels.
       */
      if (!(priv->ellipsize && priv->have_transform) &&
          (priv->angle == 90 || priv->angle == 270))
        {
          /* Doing a h4w request on a rotated label here, return the
           * required width for the minimum height.
           */
          get_size_for_allocation (label,
                                   smallest_rect.height,
                                   minimum_size, natural_size,
				   NULL, NULL);

        }
      else
        {
          /* Normal desired width */
          *minimum_size = smallest_rect.width;
          *natural_size = widest_rect.width;
        }

      *minimum_size += xpad * 2;
      *natural_size += xpad * 2;

      if (minimum_baseline)
        *minimum_baseline = -1;

      if (natural_baseline)
        *natural_baseline = -1;
    }
  else /* CTK_ORIENTATION_VERTICAL */
    {
      /* Note, we cant use get_size_for_allocation() when rotating
       * ellipsized labels.
       */
      if (!(priv->ellipsize && priv->have_transform) &&
          (priv->angle == 0 || priv->angle == 180 || priv->angle == 360))
        {
          /* Doing a w4h request on a label here, return the required
           * height for the minimum width.
           */
          get_size_for_allocation (label,
                                   widest_rect.width,
                                   minimum_size, natural_size,
				   minimum_baseline, natural_baseline);

	  if (priv->angle == 180)
	    {
	      if (minimum_baseline)
		*minimum_baseline = *minimum_size - *minimum_baseline;
	      if (natural_baseline)
		*natural_baseline = *natural_size - *natural_baseline;
	    }
        }
      else
        {
          /* A vertically rotated label does w4h, so return the base
           * desired height (text length)
           */
          *minimum_size = MIN (smallest_rect.height, widest_rect.height);
          *natural_size = MAX (smallest_rect.height, widest_rect.height);
        }

      *minimum_size += ypad * 2;
      *natural_size += ypad * 2;
    }
}

static void
ctk_label_measure (CtkCssGadget   *gadget,
                   CtkOrientation  orientation,
                   int             for_size,
                   int            *minimum,
                   int            *natural,
                   int            *minimum_baseline,
                   int            *natural_baseline,
                   gpointer        unused)
{
  CtkWidget *widget;
  CtkLabel *label;
  CtkLabelPrivate *priv;
  gint xpad, ypad;

  widget = ctk_css_gadget_get_owner (gadget);
  label = CTK_LABEL (widget);
  priv = label->priv;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_misc_get_padding (CTK_MISC (label), &xpad, &ypad);
G_GNUC_END_IGNORE_DEPRECATIONS

  if ((orientation == CTK_ORIENTATION_VERTICAL && for_size != -1 && priv->wrap && (priv->angle == 0 || priv->angle == 180 || priv->angle == 360)) ||
      (orientation == CTK_ORIENTATION_HORIZONTAL && priv->wrap && (priv->angle == 90 || priv->angle == 270)))
    {
      gint size;

      if (priv->wrap)
        ctk_label_clear_layout (label);

      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        size = MAX (1, for_size) - 2 * ypad;
      else
        size = MAX (1, for_size) - 2 * xpad;

      get_size_for_allocation (label, size, minimum, natural, minimum_baseline, natural_baseline);

      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          *minimum += 2 * xpad;
          *natural += 2 * xpad;
        }
      else
        {
          *minimum += 2 * ypad;
          *natural += 2 * ypad;
        }
    }
  else
    ctk_label_get_preferred_size (widget, orientation, minimum, natural, minimum_baseline, natural_baseline);
}

static void
ctk_label_get_preferred_width (CtkWidget *widget,
                               gint      *minimum_size,
                               gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_LABEL (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_label_get_preferred_height (CtkWidget *widget,
                                gint      *minimum_size,
                                gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_LABEL (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_label_get_preferred_width_for_height (CtkWidget *widget,
                                          gint       height,
                                          gint      *minimum_width,
                                          gint      *natural_width)
{
  ctk_css_gadget_get_preferred_size (CTK_LABEL (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum_width, natural_width,
                                     NULL, NULL);
}

static void
ctk_label_get_preferred_height_for_width (CtkWidget *widget,
                                          gint       width,
                                          gint      *minimum_height,
                                          gint      *natural_height)
{
  ctk_css_gadget_get_preferred_size (CTK_LABEL (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum_height, natural_height,
                                     NULL, NULL);
}

static void
ctk_label_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
						       gint       width,
						       gint      *minimum_height,
						       gint      *natural_height,
						       gint      *minimum_baseline,
						       gint      *natural_baseline)
{
  ctk_css_gadget_get_preferred_size (CTK_LABEL (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum_height, natural_height,
                                     minimum_baseline, natural_baseline);
}

static void
get_layout_location (CtkLabel  *label,
                     gint      *xp,
                     gint      *yp)
{
  CtkAllocation allocation;
  CtkWidget *widget;
  CtkLabelPrivate *priv;
  gint xpad, ypad;
  gint req_width, x, y;
  gint req_height;
  gfloat xalign, yalign;
  PangoRectangle logical;
  gint baseline, layout_baseline, baseline_offset;

  widget = CTK_WIDGET (label);
  priv   = label->priv;

  xalign = priv->xalign;
  yalign = priv->yalign;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_misc_get_padding (CTK_MISC (label), &xpad, &ypad);
G_GNUC_END_IGNORE_DEPRECATIONS

  if (ctk_widget_get_direction (widget) != CTK_TEXT_DIR_LTR)
    xalign = 1.0 - xalign;

  pango_layout_get_extents (priv->layout, NULL, &logical);

  if (priv->have_transform)
    {
      PangoContext *context = ctk_widget_get_pango_context (widget);
      const PangoMatrix *matrix = pango_context_get_matrix (context);
      pango_matrix_transform_rectangle (matrix, &logical);
    }

  pango_extents_to_pixels (&logical, NULL);

  req_width  = logical.width;
  req_height = logical.height;

  req_width  += 2 * xpad;
  req_height += 2 * ypad;

  ctk_css_gadget_get_content_allocation (priv->gadget,
                                         &allocation,
                                         &baseline);

  x = floor (allocation.x + xpad + xalign * (allocation.width - req_width) - logical.x);

  baseline_offset = 0;
  if (baseline != -1 && !priv->have_transform)
    {
      layout_baseline = pango_layout_get_baseline (priv->layout) / PANGO_SCALE;
      baseline_offset = baseline - layout_baseline;
      yalign = 0.0; /* Can't support yalign while baseline aligning */
    }

  /* bgo#315462 - For single-line labels, *do* align the requisition with
   * respect to the allocation, even if we are under-allocated.  For multi-line
   * labels, always show the top of the text when they are under-allocated.  The
   * rationale is this:
   *
   * - Single-line labels appear in CtkButtons, and it is very easy to get them
   *   to be smaller than their requisition.  The button may clip the label, but
   *   the label will still be able to show most of itself and the focus
   *   rectangle.  Also, it is fairly easy to read a single line of clipped text.
   *
   * - Multi-line labels should not be clipped to showing "something in the
   *   middle".  You want to read the first line, at least, to get some context.
   */
  if (pango_layout_get_line_count (priv->layout) == 1)
    y = floor (allocation.y + ypad + (allocation.height - req_height) * yalign) - logical.y + baseline_offset;
  else
    y = floor (allocation.y + ypad + MAX ((allocation.height - req_height) * yalign, 0)) - logical.y + baseline_offset;

  if (xp)
    *xp = x;

  if (yp)
    *yp = y;
}

static void
ctk_label_get_ink_rect (CtkLabel     *label,
                        GdkRectangle *rect)
{
  CtkLabelPrivate *priv = label->priv;
  CtkStyleContext *context;
  PangoRectangle ink_rect;
  CtkBorder extents;
  int x, y;

  ctk_label_ensure_layout (label);
  get_layout_location (label, &x, &y);
  pango_layout_get_pixel_extents (priv->layout, &ink_rect, NULL);
  context = ctk_widget_get_style_context (CTK_WIDGET (label));
  _ctk_css_shadows_value_get_extents (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_TEXT_SHADOW), &extents);

  rect->x = x + ink_rect.x - extents.left;
  rect->width = ink_rect.width + extents.left + extents.right;
  rect->y = y + ink_rect.y - extents.top;
  rect->height = ink_rect.height + extents.top + extents.bottom;
}

static void
ctk_label_size_allocate (CtkWidget     *widget,
                         CtkAllocation *allocation)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;
  GdkRectangle clip_rect, clip;

  CTK_WIDGET_CLASS (ctk_label_parent_class)->size_allocate (widget, allocation);

  ctk_css_gadget_allocate (priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  if (priv->layout)
    ctk_label_update_layout_width (label);

  if (priv->select_info && priv->select_info->window)
    cdk_window_move_resize (priv->select_info->window,
                            allocation->x,
                            allocation->y,
                            allocation->width,
                            allocation->height);

  ctk_label_get_ink_rect (label, &clip_rect);
  cdk_rectangle_union (&clip_rect, &clip, &clip_rect);
  _ctk_widget_set_simple_clip (widget, &clip_rect);
}

static void
ctk_label_update_cursor (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;
  CtkWidget *widget;

  if (!priv->select_info)
    return;

  widget = CTK_WIDGET (label);

  if (ctk_widget_get_realized (widget))
    {
      GdkDisplay *display;
      GdkCursor *cursor;

      if (ctk_widget_is_sensitive (widget))
        {
          display = ctk_widget_get_display (widget);

          if (priv->select_info->active_link)
            cursor = cdk_cursor_new_from_name (display, "pointer");
          else if (priv->select_info->selectable)
            cursor = cdk_cursor_new_from_name (display, "text");
          else
            cursor = NULL;
        }
      else
        cursor = NULL;

      cdk_window_set_cursor (priv->select_info->window, cursor);

      if (cursor)
        g_object_unref (cursor);
    }
}

static void
update_link_state (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;
  GList *l;
  CtkStateFlags state;

  if (!priv->select_info)
    return;

  for (l = priv->select_info->links; l; l = l->next)
    {
      CtkLabelLink *link = l->data;

      state = ctk_widget_get_state_flags (CTK_WIDGET (label));
      if (link->visited)
        state |= CTK_STATE_FLAG_VISITED;
      else
        state |= CTK_STATE_FLAG_LINK;
      if (link == priv->select_info->active_link)
        {
          if (priv->select_info->link_clicked)
            state |= CTK_STATE_FLAG_ACTIVE;
          else
            state |= CTK_STATE_FLAG_PRELIGHT;
        }
      ctk_css_node_set_state (link->cssnode, state);
    }
}

static void
ctk_label_state_flags_changed (CtkWidget     *widget,
                               CtkStateFlags  prev_state)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;

  if (priv->select_info)
    {
      if (!ctk_widget_is_sensitive (widget))
        ctk_label_select_region (label, 0, 0);

      ctk_label_update_cursor (label);
      update_link_state (label);
    }

  if (CTK_WIDGET_CLASS (ctk_label_parent_class)->state_flags_changed)
    CTK_WIDGET_CLASS (ctk_label_parent_class)->state_flags_changed (widget, prev_state);
}

static void 
ctk_label_style_updated (CtkWidget *widget)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;
  CtkStyleContext *context;
  CtkCssStyleChange *change;

  CTK_WIDGET_CLASS (ctk_label_parent_class)->style_updated (widget);

  context = ctk_widget_get_style_context (widget);
  change = ctk_style_context_get_change (context);

  if (change == NULL || ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_TEXT_ATTRS) ||
      (priv->select_info && priv->select_info->links))
    ctk_label_update_layout_attributes (label);
}

static PangoDirection
get_cursor_direction (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;
  GSList *l;

  g_assert (priv->select_info);

  ctk_label_ensure_layout (label);

  for (l = pango_layout_get_lines_readonly (priv->layout); l; l = l->next)
    {
      PangoLayoutLine *line = l->data;

      /* If priv->select_info->selection_end is at the very end of
       * the line, we don't know if the cursor is on this line or
       * the next without looking ahead at the next line. (End
       * of paragraph is different from line break.) But it's
       * definitely in this paragraph, which is good enough
       * to figure out the resolved direction.
       */
       if (line->start_index + line->length >= priv->select_info->selection_end)
	return line->resolved_dir;
    }

  return PANGO_DIRECTION_LTR;
}

static CtkLabelLink *
ctk_label_get_focus_link (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;
  CtkLabelSelectionInfo *info = priv->select_info;
  GList *l;

  if (!info)
    return NULL;

  if (info->selection_anchor != info->selection_end)
    return NULL;

  for (l = info->links; l; l = l->next)
    {
      CtkLabelLink *link = l->data;
      if (link->start <= info->selection_anchor &&
          info->selection_anchor <= link->end)
        return link;
    }

  return NULL;
}

static gboolean
ctk_label_draw (CtkWidget *widget,
                cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_LABEL (widget)->priv->gadget, cr);

  return FALSE;
}

static void layout_to_window_coords (CtkLabel *label,
                                     gint     *x,
                                     gint     *y);

static gboolean
ctk_label_render (CtkCssGadget *gadget,
                  cairo_t      *cr,
                  int           x,
                  int           y,
                  int           width,
                  int           height,
                  gpointer      data)
{
  CtkWidget *widget;
  CtkLabel *label;
  CtkLabelPrivate *priv;
  CtkLabelSelectionInfo *info;
  CtkStyleContext *context;
  gint lx, ly;

  widget = ctk_css_gadget_get_owner (gadget);
  label = CTK_LABEL (widget);
  priv = label->priv;
  info = priv->select_info;

  ctk_label_ensure_layout (label);

  context = ctk_widget_get_style_context (widget);

  if (CTK_IS_ACCEL_LABEL (widget))
    {
      guint ac_width = ctk_accel_label_get_accel_width (CTK_ACCEL_LABEL (widget));
      width -= ac_width;
      if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL)
        x += ac_width;
    }

  if (priv->text && (*priv->text != '\0'))
    {
      lx = ly = 0;
      layout_to_window_coords (label, &lx, &ly);

      ctk_render_layout (context, cr, lx, ly, priv->layout);

      if (info && (info->selection_anchor != info->selection_end))
        {
          gint range[2];
          cairo_region_t *clip;

          range[0] = info->selection_anchor;
          range[1] = info->selection_end;

          if (range[0] > range[1])
            {
              gint tmp = range[0];
              range[0] = range[1];
              range[1] = tmp;
            }

          clip = cdk_pango_layout_get_clip_region (priv->layout, lx, ly, range, 1);

          cairo_save (cr);
          ctk_style_context_save_to_node (context, info->selection_node);

          cdk_cairo_region (cr, clip);
          cairo_clip (cr);

          ctk_render_background (context, cr, x, y, width, height);
          ctk_render_layout (context, cr, lx, ly, priv->layout);

          ctk_style_context_restore (context);
          cairo_restore (cr);
          cairo_region_destroy (clip);
        }
      else if (info)
        {
          CtkLabelLink *focus_link;
          CtkLabelLink *active_link;
          gint range[2];
          cairo_region_t *clip;
          GdkRectangle rect;

          if (info->selectable &&
              ctk_widget_has_focus (widget) &&
              ctk_widget_is_drawable (widget))
            {
              PangoDirection cursor_direction;

              cursor_direction = get_cursor_direction (label);
              ctk_render_insertion_cursor (context, cr,
                                           lx, ly,
                                           priv->layout, priv->select_info->selection_end,
                                           cursor_direction);
            }

          focus_link = ctk_label_get_focus_link (label);
          active_link = info->active_link;

          if (active_link)
            {
              range[0] = active_link->start;
              range[1] = active_link->end;

              cairo_save (cr);
              ctk_style_context_save_to_node (context, active_link->cssnode);

              clip = cdk_pango_layout_get_clip_region (priv->layout, lx, ly, range, 1);

              cdk_cairo_region (cr, clip);
              cairo_clip (cr);
              cairo_region_destroy (clip);

              ctk_render_background (context, cr, x, y, width, height);
              ctk_render_layout (context, cr, lx, ly, priv->layout);

              ctk_style_context_restore (context);
              cairo_restore (cr);
            }

          if (focus_link && ctk_widget_has_visible_focus (widget))
            {
              range[0] = focus_link->start;
              range[1] = focus_link->end;

              clip = cdk_pango_layout_get_clip_region (priv->layout, lx, ly, range, 1);
              cairo_region_get_extents (clip, &rect);

              ctk_render_focus (context, cr, rect.x, rect.y, rect.width, rect.height);

              cairo_region_destroy (clip);
            }
        }
    }

  return FALSE;
}

static gboolean
separate_uline_pattern (const gchar  *str,
                        guint        *accel_key,
                        gchar       **new_str,
                        gchar       **pattern)
{
  gboolean underscore;
  const gchar *src;
  gchar *dest;
  gchar *pattern_dest;

  *accel_key = GDK_KEY_VoidSymbol;
  *new_str = g_new (gchar, strlen (str) + 1);
  *pattern = g_new (gchar, g_utf8_strlen (str, -1) + 1);

  underscore = FALSE;

  src = str;
  dest = *new_str;
  pattern_dest = *pattern;

  while (*src)
    {
      gunichar c;
      const gchar *next_src;

      c = g_utf8_get_char (src);
      if (c == (gunichar)-1)
	{
	  g_warning ("Invalid input string");
	  g_free (*new_str);
	  g_free (*pattern);

	  return FALSE;
	}
      next_src = g_utf8_next_char (src);

      if (underscore)
	{
	  if (c == '_')
	    *pattern_dest++ = ' ';
	  else
	    {
	      *pattern_dest++ = '_';
	      if (*accel_key == GDK_KEY_VoidSymbol)
		*accel_key = cdk_keyval_to_lower (cdk_unicode_to_keyval (c));
	    }

	  while (src < next_src)
	    *dest++ = *src++;

	  underscore = FALSE;
	}
      else
	{
	  if (c == '_')
	    {
	      underscore = TRUE;
	      src = next_src;
	    }
	  else
	    {
	      while (src < next_src)
		*dest++ = *src++;

	      *pattern_dest++ = ' ';
	    }
	}
    }

  *dest = 0;
  *pattern_dest = 0;

  return TRUE;
}

static void
ctk_label_set_uline_text_internal (CtkLabel    *label,
				   const gchar *str)
{
  CtkLabelPrivate *priv = label->priv;
  guint accel_key = GDK_KEY_VoidSymbol;
  gchar *new_str;
  gchar *pattern;

  g_return_if_fail (CTK_IS_LABEL (label));
  g_return_if_fail (str != NULL);

  /* Split text into the base text and a separate pattern
   * of underscores.
   */
  if (!separate_uline_pattern (str, &accel_key, &new_str, &pattern))
    return;

  ctk_label_set_text_internal (label, new_str);
  ctk_label_set_pattern_internal (label, pattern, TRUE);
  priv->mnemonic_keyval = accel_key;

  g_free (pattern);
}

/**
 * ctk_label_set_text_with_mnemonic:
 * @label: a #CtkLabel
 * @str: a string
 * 
 * Sets the label’s text from the string @str.
 * If characters in @str are preceded by an underscore, they are underlined
 * indicating that they represent a keyboard accelerator called a mnemonic.
 * The mnemonic key can be used to activate another widget, chosen 
 * automatically, or explicitly using ctk_label_set_mnemonic_widget().
 **/
void
ctk_label_set_text_with_mnemonic (CtkLabel    *label,
				  const gchar *str)
{
  g_return_if_fail (CTK_IS_LABEL (label));
  g_return_if_fail (str != NULL);

  g_object_freeze_notify (G_OBJECT (label));

  ctk_label_set_label_internal (label, g_strdup (str));
  ctk_label_set_use_markup_internal (label, FALSE);
  ctk_label_set_use_underline_internal (label, TRUE);
  
  ctk_label_recalculate (label);

  g_object_thaw_notify (G_OBJECT (label));
}

static void
ctk_label_realize (CtkWidget *widget)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;

  CTK_WIDGET_CLASS (ctk_label_parent_class)->realize (widget);

  if (priv->select_info)
    ctk_label_create_window (label);
}

static void
ctk_label_unrealize (CtkWidget *widget)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;

  if (priv->select_info)
    ctk_label_destroy_window (label);

  CTK_WIDGET_CLASS (ctk_label_parent_class)->unrealize (widget);
}

static void
ctk_label_map (CtkWidget *widget)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;

  CTK_WIDGET_CLASS (ctk_label_parent_class)->map (widget);

  if (priv->select_info)
    cdk_window_show (priv->select_info->window);
}

static void
ctk_label_unmap (CtkWidget *widget)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;

  if (priv->select_info)
    {
      cdk_window_hide (priv->select_info->window);

      if (priv->select_info->popup_menu)
        {
          ctk_widget_destroy (priv->select_info->popup_menu);
          priv->select_info->popup_menu = NULL;
        }
    }

  CTK_WIDGET_CLASS (ctk_label_parent_class)->unmap (widget);
}

static void
window_to_layout_coords (CtkLabel *label,
                         gint     *x,
                         gint     *y)
{
  CtkAllocation allocation;
  gint lx, ly;

  /* get layout location in widget->window coords */
  get_layout_location (label, &lx, &ly);
  ctk_widget_get_allocation (CTK_WIDGET (label), &allocation);

  *x += allocation.x; /* go to widget->window */
  *x -= lx;                   /* go to layout */

  *y += allocation.y; /* go to widget->window */
  *y -= ly;                   /* go to layout */
}

static void
layout_to_window_coords (CtkLabel *label,
                         gint     *x,
                         gint     *y)
{
  gint lx, ly;
  CtkAllocation allocation;

  /* get layout location in widget->window coords */
  get_layout_location (label, &lx, &ly);
  ctk_widget_get_allocation (CTK_WIDGET (label), &allocation);

  *x += lx;           /* go to widget->window */
  *x -= allocation.x; /* go to selection window */

  *y += ly;           /* go to widget->window */
  *y -= allocation.y; /* go to selection window */
}

static gboolean
get_layout_index (CtkLabel *label,
                  gint      x,
                  gint      y,
                  gint     *index)
{
  CtkLabelPrivate *priv = label->priv;
  gint trailing = 0;
  const gchar *cluster;
  const gchar *cluster_end;
  gboolean inside;

  *index = 0;

  ctk_label_ensure_layout (label);

  window_to_layout_coords (label, &x, &y);

  x *= PANGO_SCALE;
  y *= PANGO_SCALE;

  inside = pango_layout_xy_to_index (priv->layout,
                                     x, y,
                                     index, &trailing);

  cluster = priv->text + *index;
  cluster_end = cluster;
  while (trailing)
    {
      cluster_end = g_utf8_next_char (cluster_end);
      --trailing;
    }

  *index += (cluster_end - cluster);

  return inside;
}

static gboolean
range_is_in_ellipsis_full (CtkLabel *label,
                           gint      range_start,
                           gint      range_end,
                           gint     *ellipsis_start,
                           gint     *ellipsis_end)
{
  CtkLabelPrivate *priv = label->priv;
  PangoLayoutIter *iter;
  gboolean in_ellipsis;

  if (!priv->ellipsize)
    return FALSE;

  ctk_label_ensure_layout (label);

  if (!pango_layout_is_ellipsized (priv->layout))
    return FALSE;

  iter = pango_layout_get_iter (priv->layout);

  in_ellipsis = FALSE;

  do {
    PangoLayoutRun *run;

    run = pango_layout_iter_get_run_readonly (iter);
    if (run)
      {
        PangoItem *item;

        item = ((PangoGlyphItem*)run)->item;

        if (item->offset <= range_start && range_end <= item->offset + item->length)
          {
            if (item->analysis.flags & PANGO_ANALYSIS_FLAG_IS_ELLIPSIS)
              {
                if (ellipsis_start)
                  *ellipsis_start = item->offset;
                if (ellipsis_end)
                  *ellipsis_end = item->offset + item->length;
                in_ellipsis = TRUE;
              }
            break;
          }
        else if (item->offset + item->length >= range_end)
          break;
      }
  } while (pango_layout_iter_next_run (iter));

  pango_layout_iter_free (iter);

  return in_ellipsis;
}

static gboolean
range_is_in_ellipsis (CtkLabel *label,
                      gint      range_start,
                      gint      range_end)
{
  return range_is_in_ellipsis_full (label, range_start, range_end, NULL, NULL);
}

static void
ctk_label_select_word (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;
  gint min, max;

  gint start_index = ctk_label_move_backward_word (label, priv->select_info->selection_end);
  gint end_index = ctk_label_move_forward_word (label, priv->select_info->selection_end);

  min = MIN (priv->select_info->selection_anchor,
	     priv->select_info->selection_end);
  max = MAX (priv->select_info->selection_anchor,
	     priv->select_info->selection_end);

  min = MIN (min, start_index);
  max = MAX (max, end_index);

  ctk_label_select_region_index (label, min, max);
}

static void
ctk_label_grab_focus (CtkWidget *widget)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;
  gboolean select_on_focus;
  CtkLabelLink *link;
  GList *l;

  if (priv->select_info == NULL)
    return;

  CTK_WIDGET_CLASS (ctk_label_parent_class)->grab_focus (widget);

  if (priv->select_info->selectable)
    {
      g_object_get (ctk_widget_get_settings (widget),
                    "ctk-label-select-on-focus",
                    &select_on_focus,
                    NULL);

      if (select_on_focus && !priv->in_click)
        ctk_label_select_region (label, 0, -1);
    }
  else
    {
      if (priv->select_info->links && !priv->in_click)
        {
          for (l = priv->select_info->links; l; l = l->next)
            {
              link = l->data;
              if (!range_is_in_ellipsis (label, link->start, link->end))
                {
                  priv->select_info->selection_anchor = link->start;
                  priv->select_info->selection_end = link->start;
                  _ctk_label_accessible_focus_link_changed (label);
                  break;
                }
            }
        }
    }
}

static gboolean
ctk_label_focus (CtkWidget        *widget,
                 CtkDirectionType  direction)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;
  CtkLabelSelectionInfo *info = priv->select_info;
  CtkLabelLink *focus_link;
  GList *l;

  if (!ctk_widget_is_focus (widget))
    {
      ctk_widget_grab_focus (widget);
      if (info)
        {
          focus_link = ctk_label_get_focus_link (label);
          if (focus_link && direction == CTK_DIR_TAB_BACKWARD)
            {
              for (l = g_list_last (info->links); l; l = l->prev)
                {
                  focus_link = l->data;
                  if (!range_is_in_ellipsis (label, focus_link->start, focus_link->end))
                    {
                      info->selection_anchor = focus_link->start;
                      info->selection_end = focus_link->start;
                      _ctk_label_accessible_focus_link_changed (label);
                    }
                }
            }
        }

      return TRUE;
    }

  if (!info)
    return FALSE;

  if (info->selectable)
    {
      gint index;

      if (info->selection_anchor != info->selection_end)
        goto out;

      index = info->selection_anchor;

      if (direction == CTK_DIR_TAB_FORWARD)
        for (l = info->links; l; l = l->next)
          {
            CtkLabelLink *link = l->data;

            if (link->start > index)
              {
                if (!range_is_in_ellipsis (label, link->start, link->end))
                  {
                    ctk_label_select_region_index (label, link->start, link->start);
                    _ctk_label_accessible_focus_link_changed (label);
                    return TRUE;
                  }
              }
          }
      else if (direction == CTK_DIR_TAB_BACKWARD)
        for (l = g_list_last (info->links); l; l = l->prev)
          {
            CtkLabelLink *link = l->data;

            if (link->end < index)
              {
                if (!range_is_in_ellipsis (label, link->start, link->end))
                  {
                    ctk_label_select_region_index (label, link->start, link->start);
                    _ctk_label_accessible_focus_link_changed (label);
                    return TRUE;
                  }
              }
          }

      goto out;
    }
  else
    {
      focus_link = ctk_label_get_focus_link (label);
      switch (direction)
        {
        case CTK_DIR_TAB_FORWARD:
          if (focus_link)
            {
              l = g_list_find (info->links, focus_link);
              l = l->next;
            }
          else
            l = info->links;
          for (; l; l = l->next)
            {
              CtkLabelLink *link = l->data;
              if (!range_is_in_ellipsis (label, link->start, link->end))
                break;
            }
          break;

        case CTK_DIR_TAB_BACKWARD:
          if (focus_link)
            {
              l = g_list_find (info->links, focus_link);
              l = l->prev;
            }
          else
            l = g_list_last (info->links);
          for (; l; l = l->prev)
            {
              CtkLabelLink *link = l->data;
              if (!range_is_in_ellipsis (label, link->start, link->end))
                break;
            }
          break;

        default:
          goto out;
        }

      if (l)
        {
          focus_link = l->data;
          info->selection_anchor = focus_link->start;
          info->selection_end = focus_link->start;
          _ctk_label_accessible_focus_link_changed (label);
          ctk_widget_queue_draw (widget);

          return TRUE;
        }
    }

out:

  return FALSE;
}

static void
ctk_label_multipress_gesture_pressed (CtkGestureMultiPress *gesture,
                                      gint                  n_press,
                                      gdouble               widget_x,
                                      gdouble               widget_y,
                                      CtkLabel             *label)
{
  CtkLabelPrivate *priv = label->priv;
  CtkLabelSelectionInfo *info = priv->select_info;
  CtkWidget *widget = CTK_WIDGET (label);
  GdkEventSequence *sequence;
  const GdkEvent *event;
  guint button;

  if (info == NULL)
    {
      ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_DENIED);
      return;
    }

  button = ctk_gesture_single_get_current_button (CTK_GESTURE_SINGLE (gesture));
  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);
  ctk_label_update_active_link (widget, widget_x, widget_y);

  ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_CLAIMED);

  if (info->active_link)
    {
      if (cdk_event_triggers_context_menu (event))
        {
          info->link_clicked = 1;
          update_link_state (label);
          ctk_label_do_popup (label, event);
          return;
        }
      else if (button == GDK_BUTTON_PRIMARY)
        {
          info->link_clicked = 1;
          update_link_state (label);
          ctk_widget_queue_draw (widget);
          if (!info->selectable)
            return;
        }
    }

  if (!info->selectable)
    {
      ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_DENIED);
      return;
    }

  info->in_drag = FALSE;
  info->select_words = FALSE;

  if (cdk_event_triggers_context_menu (event))
    ctk_label_do_popup (label, event);
  else if (button == GDK_BUTTON_PRIMARY)
    {
      if (!ctk_widget_has_focus (widget))
        {
          priv->in_click = TRUE;
          ctk_widget_grab_focus (widget);
          priv->in_click = FALSE;
        }

      if (n_press == 3)
        ctk_label_select_region_index (label, 0, strlen (priv->text));
      else if (n_press == 2)
        {
          info->select_words = TRUE;
          ctk_label_select_word (label);
        }
    }
  else
    {
      ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_DENIED);
      return;
    }

  if (n_press >= 3)
    ctk_event_controller_reset (CTK_EVENT_CONTROLLER (gesture));
}

static void
ctk_label_multipress_gesture_released (CtkGestureMultiPress *gesture,
                                       gint                  n_press,
                                       gdouble               x,
                                       gdouble               y,
                                       CtkLabel             *label)
{
  CtkLabelPrivate *priv = label->priv;
  CtkLabelSelectionInfo *info = priv->select_info;
  GdkEventSequence *sequence;
  gint index;

  if (info == NULL)
    return;

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));

  if (!ctk_gesture_handles_sequence (CTK_GESTURE (gesture), sequence))
    return;

  if (n_press != 1)
    return;

  if (info->in_drag)
    {
      info->in_drag = 0;
      get_layout_index (label, x, y, &index);
      ctk_label_select_region_index (label, index, index);
    }
  else if (info->active_link &&
           info->selection_anchor == info->selection_end &&
           info->link_clicked)
    {
      emit_activate_link (label, info->active_link);
      info->link_clicked = 0;
    }
}

static void
connect_mnemonics_visible_notify (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;
  CtkWidget *toplevel;
  gboolean connected;

  toplevel = ctk_widget_get_toplevel (CTK_WIDGET (label));

  if (!CTK_IS_WINDOW (toplevel))
    return;

  /* always set up this widgets initial value */
  priv->mnemonics_visible =
    ctk_window_get_mnemonics_visible (CTK_WINDOW (toplevel));

  connected =
    GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (toplevel), quark_mnemonics_visible_connected));

  if (!connected)
    {
      g_signal_connect (toplevel,
                        "notify::mnemonics-visible",
                        G_CALLBACK (label_mnemonics_visible_changed),
                        label);
      g_object_set_qdata (G_OBJECT (toplevel),
                          quark_mnemonics_visible_connected,
                          GINT_TO_POINTER (1));
    }
}

static void
drag_begin_cb (CtkWidget      *widget,
               GdkDragContext *context,
               gpointer        data)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;
  cairo_surface_t *surface = NULL;

  g_signal_handlers_disconnect_by_func (widget, drag_begin_cb, NULL);

  if ((priv->select_info->selection_anchor !=
       priv->select_info->selection_end) &&
      priv->text)
    {
      gint start, end;
      gint len;

      start = MIN (priv->select_info->selection_anchor,
                   priv->select_info->selection_end);
      end = MAX (priv->select_info->selection_anchor,
                 priv->select_info->selection_end);

      len = strlen (priv->text);

      if (end > len)
        end = len;

      if (start > len)
        start = len;

      surface = _ctk_text_util_create_drag_icon (widget,
                                                 priv->text + start,
                                                 end - start);
    }

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
ctk_label_drag_gesture_begin (CtkGestureDrag *gesture,
                              gdouble         start_x,
                              gdouble         start_y,
                              CtkLabel       *label)
{
  CtkLabelPrivate *priv = label->priv;
  CtkLabelSelectionInfo *info = priv->select_info;
  GdkModifierType state_mask;
  GdkEventSequence *sequence;
  const GdkEvent *event;
  gint min, max, index;

  if (!info || !info->selectable)
    {
      ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_DENIED);
      return;
    }

  get_layout_index (label, start_x, start_y, &index);
  min = MIN (info->selection_anchor, info->selection_end);
  max = MAX (info->selection_anchor, info->selection_end);

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);
  cdk_event_get_state (event, &state_mask);

  if ((info->selection_anchor != info->selection_end) &&
      (state_mask & GDK_SHIFT_MASK))
    {
      if (index > min && index < max)
        {
          /* truncate selection, but keep it as big as possible */
          if (index - min > max - index)
            max = index;
          else
            min = index;
        }
      else
        {
          /* extend (same as motion) */
          min = MIN (min, index);
          max = MAX (max, index);
        }

      /* ensure the anchor is opposite index */
      if (index == min)
        {
          gint tmp = min;
          min = max;
          max = tmp;
        }

      ctk_label_select_region_index (label, min, max);
    }
  else
    {
      if (min < max && min <= index && index <= max)
        {
          info->in_drag = TRUE;
          info->drag_start_x = start_x;
          info->drag_start_y = start_y;
        }
      else
        /* start a replacement */
        ctk_label_select_region_index (label, index, index);
    }
}

static void
ctk_label_drag_gesture_update (CtkGestureDrag *gesture,
                               gdouble         offset_x,
                               gdouble         offset_y,
                               CtkLabel       *label)
{
  CtkLabelPrivate *priv = label->priv;
  CtkLabelSelectionInfo *info = priv->select_info;
  CtkWidget *widget = CTK_WIDGET (label);
  GdkEventSequence *sequence;
  gdouble x, y;
  gint index;

  if (info == NULL || !info->selectable)
    return;

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  ctk_gesture_get_point (CTK_GESTURE (gesture), sequence, &x, &y);

  if (info->in_drag)
    {
      if (ctk_drag_check_threshold (widget,
				    info->drag_start_x,
				    info->drag_start_y,
				    x, y))
	{
	  CtkTargetList *target_list = ctk_target_list_new (NULL, 0);
          const GdkEvent *event;

          event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);
	  ctk_target_list_add_text_targets (target_list, 0);

          g_signal_connect (widget, "drag-begin",
                            G_CALLBACK (drag_begin_cb), NULL);
	  ctk_drag_begin_with_coordinates (widget, target_list,
                                           GDK_ACTION_COPY,
                                           1, (GdkEvent*) event,
                                           info->drag_start_x,
                                           info->drag_start_y);

	  info->in_drag = FALSE;

	  ctk_target_list_unref (target_list);
	}
    }
  else
    {
      get_layout_index (label, x, y, &index);

      if (index != info->selection_anchor)
        ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_CLAIMED);

      if (info->select_words)
        {
          gint min, max;
          gint old_min, old_max;
          gint anchor, end;

          min = ctk_label_move_backward_word (label, index);
          max = ctk_label_move_forward_word (label, index);

          anchor = info->selection_anchor;
          end = info->selection_end;

          old_min = MIN (anchor, end);
          old_max = MAX (anchor, end);

          if (min < old_min)
            {
              anchor = min;
              end = old_max;
            }
          else if (old_max < max)
            {
              anchor = max;
              end = old_min;
            }
          else if (anchor == old_min)
            {
              if (anchor != min)
                anchor = max;
            }
          else
            {
              if (anchor != max)
                anchor = min;
            }

          ctk_label_select_region_index (label, anchor, end);
        }
      else
        ctk_label_select_region_index (label, info->selection_anchor, index);
    }
}

static void
ctk_label_update_active_link (CtkWidget *widget,
                              gdouble    x,
                              gdouble    y)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;
  CtkLabelSelectionInfo *info = priv->select_info;
  gint index;

  if (info == NULL)
    return;

  if (info->links && !info->in_drag)
    {
      GList *l;
      CtkLabelLink *link;
      gboolean found = FALSE;

      if (info->selection_anchor == info->selection_end)
        {
          if (get_layout_index (label, x, y, &index))
            {
              for (l = info->links; l != NULL; l = l->next)
                {
                  link = l->data;
                  if (index >= link->start && index <= link->end)
                    {
                      if (!range_is_in_ellipsis (label, link->start, link->end))
                        found = TRUE;
                      break;
                    }
                }
            }
        }

      if (found)
        {
          if (info->active_link != link)
            {
              info->link_clicked = 0;
              info->active_link = link;
              update_link_state (label);
              ctk_label_update_cursor (label);
              ctk_widget_queue_draw (widget);
            }
        }
      else
        {
          if (info->active_link != NULL)
            {
              info->link_clicked = 0;
              info->active_link = NULL;
              update_link_state (label);
              ctk_label_update_cursor (label);
              ctk_widget_queue_draw (widget);
            }
        }
    }
}

static gboolean
ctk_label_motion (CtkWidget      *widget,
                  GdkEventMotion *event)
{
  gdouble x, y;

  cdk_event_get_coords ((GdkEvent *) event, &x, &y);
  ctk_label_update_active_link (widget, x, y);

  return CTK_WIDGET_CLASS (ctk_label_parent_class)->motion_notify_event (widget, event);
}

static gboolean
ctk_label_leave_notify (CtkWidget        *widget,
                        GdkEventCrossing *event)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;

  if (priv->select_info)
    {
      priv->select_info->active_link = NULL;
      ctk_label_update_cursor (label);
      ctk_widget_queue_draw (widget);
    }

  if (CTK_WIDGET_CLASS (ctk_label_parent_class)->leave_notify_event)
    return CTK_WIDGET_CLASS (ctk_label_parent_class)->leave_notify_event (widget, event);

 return FALSE;
}

static void
ctk_label_create_window (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;
  CtkAllocation allocation;
  CtkWidget *widget;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_assert (priv->select_info);
  widget = CTK_WIDGET (label);
  g_assert (ctk_widget_get_realized (widget));

  if (priv->select_info->window)
    return;

  ctk_widget_get_allocation (widget, &allocation);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.override_redirect = TRUE;
  attributes.event_mask = ctk_widget_get_events (widget) |
    GDK_BUTTON_PRESS_MASK        |
    GDK_BUTTON_RELEASE_MASK      |
    GDK_LEAVE_NOTIFY_MASK        |
    GDK_BUTTON_MOTION_MASK       |
    GDK_POINTER_MOTION_MASK;
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_NOREDIR;
  if (ctk_widget_is_sensitive (widget) && priv->select_info->selectable)
    {
      attributes.cursor = cdk_cursor_new_for_display (ctk_widget_get_display (widget),
						      GDK_XTERM);
      attributes_mask |= GDK_WA_CURSOR;
    }


  priv->select_info->window = cdk_window_new (ctk_widget_get_window (widget),
                                               &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->select_info->window);

  if (attributes_mask & GDK_WA_CURSOR)
    g_object_unref (attributes.cursor);
}

static void
ctk_label_destroy_window (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;

  g_assert (priv->select_info);

  if (priv->select_info->window == NULL)
    return;

  ctk_widget_unregister_window (CTK_WIDGET (label), priv->select_info->window);
  cdk_window_destroy (priv->select_info->window);
  priv->select_info->window = NULL;
}

static void
ctk_label_ensure_select_info (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;

  if (priv->select_info == NULL)
    {
      priv->select_info = g_new0 (CtkLabelSelectionInfo, 1);

      ctk_widget_set_can_focus (CTK_WIDGET (label), TRUE);

      if (ctk_widget_get_realized (CTK_WIDGET (label)))
	ctk_label_create_window (label);

      if (ctk_widget_get_mapped (CTK_WIDGET (label)))
        cdk_window_show (priv->select_info->window);

      priv->select_info->drag_gesture = ctk_gesture_drag_new (CTK_WIDGET (label));
      g_signal_connect (priv->select_info->drag_gesture, "drag-begin",
                        G_CALLBACK (ctk_label_drag_gesture_begin), label);
      g_signal_connect (priv->select_info->drag_gesture, "drag-update",
                        G_CALLBACK (ctk_label_drag_gesture_update), label);
      ctk_gesture_single_set_exclusive (CTK_GESTURE_SINGLE (priv->select_info->drag_gesture), TRUE);

      priv->select_info->multipress_gesture = ctk_gesture_multi_press_new (CTK_WIDGET (label));
      g_signal_connect (priv->select_info->multipress_gesture, "pressed",
                        G_CALLBACK (ctk_label_multipress_gesture_pressed), label);
      g_signal_connect (priv->select_info->multipress_gesture, "released",
                        G_CALLBACK (ctk_label_multipress_gesture_released), label);
      ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (priv->select_info->multipress_gesture), 0);
      ctk_gesture_single_set_exclusive (CTK_GESTURE_SINGLE (priv->select_info->multipress_gesture), TRUE);
    }
}

static void
ctk_label_clear_select_info (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;

  if (priv->select_info == NULL)
    return;

  if (!priv->select_info->selectable && !priv->select_info->links)
    {
      ctk_label_destroy_window (label);

      g_object_unref (priv->select_info->drag_gesture);
      g_object_unref (priv->select_info->multipress_gesture);

      g_free (priv->select_info);
      priv->select_info = NULL;

      ctk_widget_set_can_focus (CTK_WIDGET (label), FALSE);
    }
}

/**
 * ctk_label_set_selectable:
 * @label: a #CtkLabel
 * @setting: %TRUE to allow selecting text in the label
 *
 * Selectable labels allow the user to select text from the label, for
 * copy-and-paste.
 **/
void
ctk_label_set_selectable (CtkLabel *label,
                          gboolean  setting)
{
  CtkLabelPrivate *priv;
  gboolean old_setting;

  g_return_if_fail (CTK_IS_LABEL (label));

  priv = label->priv;

  setting = setting != FALSE;
  old_setting = priv->select_info && priv->select_info->selectable;

  if (setting)
    {
      ctk_label_ensure_select_info (label);
      priv->select_info->selectable = TRUE;
      ctk_label_update_cursor (label);
    }
  else
    {
      if (old_setting)
        {
          /* unselect, to give up the selection */
          ctk_label_select_region (label, 0, 0);

          priv->select_info->selectable = FALSE;
          ctk_label_clear_select_info (label);
          ctk_label_update_cursor (label);
        }
    }
  if (setting != old_setting)
    {
      g_object_freeze_notify (G_OBJECT (label));
      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_SELECTABLE]);
      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_CURSOR_POSITION]);
      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_SELECTION_BOUND]);
      g_object_thaw_notify (G_OBJECT (label));
      ctk_widget_queue_draw (CTK_WIDGET (label));
    }
}

/**
 * ctk_label_get_selectable:
 * @label: a #CtkLabel
 * 
 * Gets the value set by ctk_label_set_selectable().
 * 
 * Returns: %TRUE if the user can copy text from the label
 **/
gboolean
ctk_label_get_selectable (CtkLabel *label)
{
  CtkLabelPrivate *priv;

  g_return_val_if_fail (CTK_IS_LABEL (label), FALSE);

  priv = label->priv;

  return priv->select_info && priv->select_info->selectable;
}

/**
 * ctk_label_set_angle:
 * @label: a #CtkLabel
 * @angle: the angle that the baseline of the label makes with
 *   the horizontal, in degrees, measured counterclockwise
 * 
 * Sets the angle of rotation for the label. An angle of 90 reads from
 * from bottom to top, an angle of 270, from top to bottom. The angle
 * setting for the label is ignored if the label is selectable,
 * wrapped, or ellipsized.
 *
 * Since: 2.6
 **/
void
ctk_label_set_angle (CtkLabel *label,
		     gdouble   angle)
{
  CtkLabelPrivate *priv;

  g_return_if_fail (CTK_IS_LABEL (label));

  priv = label->priv;

  /* Canonicalize to [0,360]. We don't canonicalize 360 to 0, because
   * double property ranges are inclusive, and changing 360 to 0 would
   * make a property editor behave strangely.
   */
  if (angle < 0 || angle > 360.0)
    angle = angle - 360. * floor (angle / 360.);

  if (priv->angle != angle)
    {
      priv->angle = angle;
      
      ctk_label_clear_layout (label);
      ctk_widget_queue_resize (CTK_WIDGET (label));

      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_ANGLE]);
    }
}

/**
 * ctk_label_get_angle:
 * @label: a #CtkLabel
 * 
 * Gets the angle of rotation for the label. See
 * ctk_label_set_angle().
 * 
 * Returns: the angle of rotation for the label
 *
 * Since: 2.6
 **/
gdouble
ctk_label_get_angle  (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), 0.0);
  
  return label->priv->angle;
}

static void
ctk_label_set_selection_text (CtkLabel         *label,
			      CtkSelectionData *selection_data)
{
  CtkLabelPrivate *priv = label->priv;

  if (priv->select_info &&
      (priv->select_info->selection_anchor !=
       priv->select_info->selection_end) &&
      priv->text)
    {
      gint start, end;
      gint len;

      start = MIN (priv->select_info->selection_anchor,
                   priv->select_info->selection_end);
      end = MAX (priv->select_info->selection_anchor,
                 priv->select_info->selection_end);

      len = strlen (priv->text);

      if (end > len)
        end = len;

      if (start > len)
        start = len;

      ctk_selection_data_set_text (selection_data,
				   priv->text + start,
				   end - start);
    }
}

static void
ctk_label_drag_data_get (CtkWidget        *widget,
			 GdkDragContext   *context,
			 CtkSelectionData *selection_data,
			 guint             info,
			 guint             time)
{
  ctk_label_set_selection_text (CTK_LABEL (widget), selection_data);
}

static void
get_text_callback (CtkClipboard     *clipboard,
                   CtkSelectionData *selection_data,
                   guint             info,
                   gpointer          user_data_or_owner)
{
  ctk_label_set_selection_text (CTK_LABEL (user_data_or_owner), selection_data);
}

static void
clear_text_callback (CtkClipboard     *clipboard,
                     gpointer          user_data_or_owner)
{
  CtkLabel *label;
  CtkLabelPrivate *priv;

  label = CTK_LABEL (user_data_or_owner);
  priv = label->priv;

  if (priv->select_info)
    {
      priv->select_info->selection_anchor = priv->select_info->selection_end;

      ctk_widget_queue_draw (CTK_WIDGET (label));
    }
}

static void
ctk_label_select_region_index (CtkLabel *label,
                               gint      anchor_index,
                               gint      end_index)
{
  CtkLabelPrivate *priv;

  g_return_if_fail (CTK_IS_LABEL (label));

  priv = label->priv;

  if (priv->select_info && priv->select_info->selectable)
    {
      CtkClipboard *clipboard;
      gint s, e;

      /* Ensure that we treat an ellipsized region like a single
       * character with respect to selection.
       */
      if (anchor_index < end_index)
        {
          if (range_is_in_ellipsis_full (label, anchor_index, anchor_index + 1, &s, &e))
            {
              if (priv->select_info->selection_anchor == s)
                anchor_index = e;
              else
                anchor_index = s;
            }
          if (range_is_in_ellipsis_full (label, end_index - 1, end_index, &s, &e))
            {
              if (priv->select_info->selection_end == e)
                end_index = s;
              else
                end_index = e;
            }
        }
      else if (end_index < anchor_index)
        {
          if (range_is_in_ellipsis_full (label, end_index, end_index + 1, &s, &e))
            {
              if (priv->select_info->selection_end == s)
                end_index = e;
              else
                end_index = s;
            }
          if (range_is_in_ellipsis_full (label, anchor_index - 1, anchor_index, &s, &e))
            {
              if (priv->select_info->selection_anchor == e)
                anchor_index = s;
              else
                anchor_index = e;
            }
        }
      else
        {
          if (range_is_in_ellipsis_full (label, anchor_index, anchor_index, &s, &e))
            {
              if (priv->select_info->selection_anchor == s)
                anchor_index = e;
              else if (priv->select_info->selection_anchor == e)
                anchor_index = s;
              else if (anchor_index - s < e - anchor_index)
                anchor_index = s;
              else
                anchor_index = e;
              end_index = anchor_index;
            }
        }

      if (priv->select_info->selection_anchor == anchor_index &&
          priv->select_info->selection_end == end_index)
        return;

      g_object_freeze_notify (G_OBJECT (label));

      if (priv->select_info->selection_anchor != anchor_index)
        g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_SELECTION_BOUND]);
      if (priv->select_info->selection_end != end_index)
        g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_CURSOR_POSITION]);

      priv->select_info->selection_anchor = anchor_index;
      priv->select_info->selection_end = end_index;

      if (ctk_widget_has_screen (CTK_WIDGET (label)))
        clipboard = ctk_widget_get_clipboard (CTK_WIDGET (label),
                                              GDK_SELECTION_PRIMARY);
      else
        clipboard = NULL;

      if (anchor_index != end_index)
        {
          CtkTargetList *list;
          CtkTargetEntry *targets;
          gint n_targets;

          list = ctk_target_list_new (NULL, 0);
          ctk_target_list_add_text_targets (list, 0);
          targets = ctk_target_table_new_from_list (list, &n_targets);

          if (clipboard)
            ctk_clipboard_set_with_owner (clipboard,
                                          targets, n_targets,
                                          get_text_callback,
                                          clear_text_callback,
                                          G_OBJECT (label));

          ctk_target_table_free (targets, n_targets);
          ctk_target_list_unref (list);

          if (!priv->select_info->selection_node)
            {
              CtkCssNode *widget_node;

              widget_node = ctk_widget_get_css_node (CTK_WIDGET (label));
              priv->select_info->selection_node = ctk_css_node_new ();
              ctk_css_node_set_name (priv->select_info->selection_node, I_("selection"));
              ctk_css_node_set_parent (priv->select_info->selection_node, widget_node);
              ctk_css_node_set_state (priv->select_info->selection_node, ctk_css_node_get_state (widget_node));
              g_object_unref (priv->select_info->selection_node);
            }
        }
      else
        {
          if (clipboard &&
              ctk_clipboard_get_owner (clipboard) == G_OBJECT (label))
            ctk_clipboard_clear (clipboard);

          if (priv->select_info->selection_node)
            {
              ctk_css_node_set_parent (priv->select_info->selection_node, NULL);
              priv->select_info->selection_node = NULL;
            }
        }

      ctk_widget_queue_draw (CTK_WIDGET (label));

      g_object_thaw_notify (G_OBJECT (label));
    }
}

/**
 * ctk_label_select_region:
 * @label: a #CtkLabel
 * @start_offset: start offset (in characters not bytes)
 * @end_offset: end offset (in characters not bytes)
 *
 * Selects a range of characters in the label, if the label is selectable.
 * See ctk_label_set_selectable(). If the label is not selectable,
 * this function has no effect. If @start_offset or
 * @end_offset are -1, then the end of the label will be substituted.
 **/
void
ctk_label_select_region  (CtkLabel *label,
                          gint      start_offset,
                          gint      end_offset)
{
  CtkLabelPrivate *priv;

  g_return_if_fail (CTK_IS_LABEL (label));

  priv = label->priv;

  if (priv->text && priv->select_info)
    {
      if (start_offset < 0)
        start_offset = g_utf8_strlen (priv->text, -1);
      
      if (end_offset < 0)
        end_offset = g_utf8_strlen (priv->text, -1);
      
      ctk_label_select_region_index (label,
                                     g_utf8_offset_to_pointer (priv->text, start_offset) - priv->text,
                                     g_utf8_offset_to_pointer (priv->text, end_offset) - priv->text);
    }
}

/**
 * ctk_label_get_selection_bounds:
 * @label: a #CtkLabel
 * @start: (out): return location for start of selection, as a character offset
 * @end: (out): return location for end of selection, as a character offset
 * 
 * Gets the selected range of characters in the label, returning %TRUE
 * if there’s a selection.
 * 
 * Returns: %TRUE if selection is non-empty
 **/
gboolean
ctk_label_get_selection_bounds (CtkLabel  *label,
                                gint      *start,
                                gint      *end)
{
  CtkLabelPrivate *priv;

  g_return_val_if_fail (CTK_IS_LABEL (label), FALSE);

  priv = label->priv;

  if (priv->select_info == NULL)
    {
      /* not a selectable label */
      if (start)
        *start = 0;
      if (end)
        *end = 0;

      return FALSE;
    }
  else
    {
      gint start_index, end_index;
      gint start_offset, end_offset;
      gint len;
      
      start_index = MIN (priv->select_info->selection_anchor,
                   priv->select_info->selection_end);
      end_index = MAX (priv->select_info->selection_anchor,
                 priv->select_info->selection_end);

      len = strlen (priv->text);

      if (end_index > len)
        end_index = len;

      if (start_index > len)
        start_index = len;
      
      start_offset = g_utf8_strlen (priv->text, start_index);
      end_offset = g_utf8_strlen (priv->text, end_index);

      if (start_offset > end_offset)
        {
          gint tmp = start_offset;
          start_offset = end_offset;
          end_offset = tmp;
        }
      
      if (start)
        *start = start_offset;

      if (end)
        *end = end_offset;

      return start_offset != end_offset;
    }
}


/**
 * ctk_label_get_layout:
 * @label: a #CtkLabel
 * 
 * Gets the #PangoLayout used to display the label.
 * The layout is useful to e.g. convert text positions to
 * pixel positions, in combination with ctk_label_get_layout_offsets().
 * The returned layout is owned by the @label so need not be
 * freed by the caller. The @label is free to recreate its layout at
 * any time, so it should be considered read-only.
 *
 * Returns: (transfer none): the #PangoLayout for this label
 **/
PangoLayout*
ctk_label_get_layout (CtkLabel *label)
{
  CtkLabelPrivate *priv;

  g_return_val_if_fail (CTK_IS_LABEL (label), NULL);

  priv = label->priv;

  ctk_label_ensure_layout (label);

  return priv->layout;
}

/**
 * ctk_label_get_layout_offsets:
 * @label: a #CtkLabel
 * @x: (out) (optional): location to store X offset of layout, or %NULL
 * @y: (out) (optional): location to store Y offset of layout, or %NULL
 *
 * Obtains the coordinates where the label will draw the #PangoLayout
 * representing the text in the label; useful to convert mouse events
 * into coordinates inside the #PangoLayout, e.g. to take some action
 * if some part of the label is clicked. Of course you will need to
 * create a #CtkEventBox to receive the events, and pack the label
 * inside it, since labels are windowless (they return %FALSE from
 * ctk_widget_get_has_window()). Remember
 * when using the #PangoLayout functions you need to convert to
 * and from pixels using PANGO_PIXELS() or #PANGO_SCALE.
 **/
void
ctk_label_get_layout_offsets (CtkLabel *label,
                              gint     *x,
                              gint     *y)
{
  g_return_if_fail (CTK_IS_LABEL (label));

  ctk_label_ensure_layout (label);

  get_layout_location (label, x, y);
}

/**
 * ctk_label_set_use_markup:
 * @label: a #CtkLabel
 * @setting: %TRUE if the label’s text should be parsed for markup.
 *
 * Sets whether the text of the label contains markup in
 * [Pango’s text markup language][PangoMarkupFormat].
 * See ctk_label_set_markup().
 **/
void
ctk_label_set_use_markup (CtkLabel *label,
			  gboolean  setting)
{
  g_return_if_fail (CTK_IS_LABEL (label));

  g_object_freeze_notify (G_OBJECT (label));

  if (ctk_label_set_use_markup_internal (label, setting))
    ctk_label_recalculate (label);

  g_object_thaw_notify (G_OBJECT (label));
}

/**
 * ctk_label_get_use_markup:
 * @label: a #CtkLabel
 *
 * Returns whether the label’s text is interpreted as marked up with
 * the [Pango text markup language][PangoMarkupFormat].
 * See ctk_label_set_use_markup ().
 *
 * Returns: %TRUE if the label’s text will be parsed for markup.
 **/
gboolean
ctk_label_get_use_markup (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), FALSE);

  return label->priv->use_markup;
}

/**
 * ctk_label_set_use_underline:
 * @label: a #CtkLabel
 * @setting: %TRUE if underlines in the text indicate mnemonics
 *
 * If true, an underline in the text indicates the next character should be
 * used for the mnemonic accelerator key.
 */
void
ctk_label_set_use_underline (CtkLabel *label,
			     gboolean  setting)
{
  g_return_if_fail (CTK_IS_LABEL (label));

  g_object_freeze_notify (G_OBJECT (label));

  if (ctk_label_set_use_underline_internal (label, setting))
    ctk_label_recalculate (label);

  g_object_thaw_notify (G_OBJECT (label));
}

/**
 * ctk_label_get_use_underline:
 * @label: a #CtkLabel
 *
 * Returns whether an embedded underline in the label indicates a
 * mnemonic. See ctk_label_set_use_underline().
 *
 * Returns: %TRUE whether an embedded underline in the label indicates
 *               the mnemonic accelerator keys.
 **/
gboolean
ctk_label_get_use_underline (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), FALSE);

  return label->priv->use_underline;
}

/**
 * ctk_label_set_single_line_mode:
 * @label: a #CtkLabel
 * @single_line_mode: %TRUE if the label should be in single line mode
 *
 * Sets whether the label is in single line mode.
 *
 * Since: 2.6
 */
void
ctk_label_set_single_line_mode (CtkLabel *label,
                                gboolean single_line_mode)
{
  CtkLabelPrivate *priv;

  g_return_if_fail (CTK_IS_LABEL (label));

  priv = label->priv;

  single_line_mode = single_line_mode != FALSE;

  if (priv->single_line_mode != single_line_mode)
    {
      priv->single_line_mode = single_line_mode;

      ctk_label_clear_layout (label);
      ctk_widget_queue_resize (CTK_WIDGET (label));

      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_SINGLE_LINE_MODE]);
    }
}

/**
 * ctk_label_get_single_line_mode:
 * @label: a #CtkLabel
 *
 * Returns whether the label is in single line mode.
 *
 * Returns: %TRUE when the label is in single line mode.
 *
 * Since: 2.6
 **/
gboolean
ctk_label_get_single_line_mode  (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), FALSE);

  return label->priv->single_line_mode;
}

/* Compute the X position for an offset that corresponds to the "more important
 * cursor position for that offset. We use this when trying to guess to which
 * end of the selection we should go to when the user hits the left or
 * right arrow key.
 */
static void
get_better_cursor (CtkLabel *label,
		   gint      index,
		   gint      *x,
		   gint      *y)
{
  CtkLabelPrivate *priv = label->priv;
  GdkKeymap *keymap = cdk_keymap_get_for_display (ctk_widget_get_display (CTK_WIDGET (label)));
  PangoDirection keymap_direction = cdk_keymap_get_direction (keymap);
  PangoDirection cursor_direction = get_cursor_direction (label);
  gboolean split_cursor;
  PangoRectangle strong_pos, weak_pos;
  
  g_object_get (ctk_widget_get_settings (CTK_WIDGET (label)),
		"ctk-split-cursor", &split_cursor,
		NULL);

  ctk_label_ensure_layout (label);
  
  pango_layout_get_cursor_pos (priv->layout, index,
			       &strong_pos, &weak_pos);

  if (split_cursor)
    {
      *x = strong_pos.x / PANGO_SCALE;
      *y = strong_pos.y / PANGO_SCALE;
    }
  else
    {
      if (keymap_direction == cursor_direction)
	{
	  *x = strong_pos.x / PANGO_SCALE;
	  *y = strong_pos.y / PANGO_SCALE;
	}
      else
	{
	  *x = weak_pos.x / PANGO_SCALE;
	  *y = weak_pos.y / PANGO_SCALE;
	}
    }
}


static gint
ctk_label_move_logically (CtkLabel *label,
			  gint      start,
			  gint      count)
{
  CtkLabelPrivate *priv = label->priv;
  gint offset = g_utf8_pointer_to_offset (priv->text,
					  priv->text + start);

  if (priv->text)
    {
      PangoLogAttr *log_attrs;
      gint n_attrs;
      gint length;

      ctk_label_ensure_layout (label);
      
      length = g_utf8_strlen (priv->text, -1);

      pango_layout_get_log_attrs (priv->layout, &log_attrs, &n_attrs);

      while (count > 0 && offset < length)
	{
	  do
	    offset++;
	  while (offset < length && !log_attrs[offset].is_cursor_position);
	  
	  count--;
	}
      while (count < 0 && offset > 0)
	{
	  do
	    offset--;
	  while (offset > 0 && !log_attrs[offset].is_cursor_position);
	  
	  count++;
	}
      
      g_free (log_attrs);
    }

  return g_utf8_offset_to_pointer (priv->text, offset) - priv->text;
}

static gint
ctk_label_move_visually (CtkLabel *label,
			 gint      start,
			 gint      count)
{
  CtkLabelPrivate *priv = label->priv;
  gint index;

  index = start;
  
  while (count != 0)
    {
      int new_index, new_trailing;
      gboolean split_cursor;
      gboolean strong;

      ctk_label_ensure_layout (label);

      g_object_get (ctk_widget_get_settings (CTK_WIDGET (label)),
		    "ctk-split-cursor", &split_cursor,
		    NULL);

      if (split_cursor)
	strong = TRUE;
      else
	{
	  GdkKeymap *keymap = cdk_keymap_get_for_display (ctk_widget_get_display (CTK_WIDGET (label)));
	  PangoDirection keymap_direction = cdk_keymap_get_direction (keymap);

	  strong = keymap_direction == get_cursor_direction (label);
	}
      
      if (count > 0)
	{
	  pango_layout_move_cursor_visually (priv->layout, strong, index, 0, 1, &new_index, &new_trailing);
	  count--;
	}
      else
	{
	  pango_layout_move_cursor_visually (priv->layout, strong, index, 0, -1, &new_index, &new_trailing);
	  count++;
	}

      if (new_index < 0 || new_index == G_MAXINT)
	break;

      index = new_index;
      
      while (new_trailing--)
	index = g_utf8_next_char (priv->text + new_index) - priv->text;
    }
  
  return index;
}

static gint
ctk_label_move_forward_word (CtkLabel *label,
			     gint      start)
{
  CtkLabelPrivate *priv = label->priv;
  gint new_pos = g_utf8_pointer_to_offset (priv->text,
					   priv->text + start);
  gint length;

  length = g_utf8_strlen (priv->text, -1);
  if (new_pos < length)
    {
      PangoLogAttr *log_attrs;
      gint n_attrs;

      ctk_label_ensure_layout (label);

      pango_layout_get_log_attrs (priv->layout, &log_attrs, &n_attrs);

      /* Find the next word end */
      new_pos++;
      while (new_pos < n_attrs && !log_attrs[new_pos].is_word_end)
	new_pos++;

      g_free (log_attrs);
    }

  return g_utf8_offset_to_pointer (priv->text, new_pos) - priv->text;
}


static gint
ctk_label_move_backward_word (CtkLabel *label,
			      gint      start)
{
  CtkLabelPrivate *priv = label->priv;
  gint new_pos = g_utf8_pointer_to_offset (priv->text,
					   priv->text + start);

  if (new_pos > 0)
    {
      PangoLogAttr *log_attrs;
      gint n_attrs;

      ctk_label_ensure_layout (label);

      pango_layout_get_log_attrs (priv->layout, &log_attrs, &n_attrs);

      new_pos -= 1;

      /* Find the previous word beginning */
      while (new_pos > 0 && !log_attrs[new_pos].is_word_start)
	new_pos--;

      g_free (log_attrs);
    }

  return g_utf8_offset_to_pointer (priv->text, new_pos) - priv->text;
}

static void
ctk_label_move_cursor (CtkLabel       *label,
                       CtkMovementStep step,
                       gint            count,
                       gboolean        extend_selection)
{
  CtkLabelPrivate *priv = label->priv;
  gint old_pos;
  gint new_pos;

  if (priv->select_info == NULL)
    return;

  old_pos = new_pos = priv->select_info->selection_end;

  if (priv->select_info->selection_end != priv->select_info->selection_anchor &&
      !extend_selection)
    {
      /* If we have a current selection and aren't extending it, move to the
       * start/or end of the selection as appropriate
       */
      switch (step)
        {
        case CTK_MOVEMENT_VISUAL_POSITIONS:
          {
            gint end_x, end_y;
            gint anchor_x, anchor_y;
            gboolean end_is_left;

            get_better_cursor (label, priv->select_info->selection_end, &end_x, &end_y);
            get_better_cursor (label, priv->select_info->selection_anchor, &anchor_x, &anchor_y);

            end_is_left = (end_y < anchor_y) || (end_y == anchor_y && end_x < anchor_x);

            if (count < 0)
              new_pos = end_is_left ? priv->select_info->selection_end : priv->select_info->selection_anchor;
            else
              new_pos = !end_is_left ? priv->select_info->selection_end : priv->select_info->selection_anchor;
            break;
          }
        case CTK_MOVEMENT_LOGICAL_POSITIONS:
        case CTK_MOVEMENT_WORDS:
          if (count < 0)
            new_pos = MIN (priv->select_info->selection_end, priv->select_info->selection_anchor);
          else
            new_pos = MAX (priv->select_info->selection_end, priv->select_info->selection_anchor);
          break;
        case CTK_MOVEMENT_DISPLAY_LINE_ENDS:
        case CTK_MOVEMENT_PARAGRAPH_ENDS:
        case CTK_MOVEMENT_BUFFER_ENDS:
          /* FIXME: Can do better here */
          new_pos = count < 0 ? 0 : strlen (priv->text);
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
          new_pos = ctk_label_move_logically (label, new_pos, count);
          break;
        case CTK_MOVEMENT_VISUAL_POSITIONS:
          new_pos = ctk_label_move_visually (label, new_pos, count);
          if (new_pos == old_pos)
            {
              if (!extend_selection)
                {
                  if (!ctk_widget_keynav_failed (CTK_WIDGET (label),
                                                 count > 0 ?
                                                 CTK_DIR_RIGHT : CTK_DIR_LEFT))
                    {
                      CtkWidget *toplevel = ctk_widget_get_toplevel (CTK_WIDGET (label));

                      if (toplevel)
                        ctk_widget_child_focus (toplevel,
                                                count > 0 ?
                                                CTK_DIR_RIGHT : CTK_DIR_LEFT);
                    }
                }
              else
                {
                  ctk_widget_error_bell (CTK_WIDGET (label));
                }
            }
          break;
        case CTK_MOVEMENT_WORDS:
          while (count > 0)
            {
              new_pos = ctk_label_move_forward_word (label, new_pos);
              count--;
            }
          while (count < 0)
            {
              new_pos = ctk_label_move_backward_word (label, new_pos);
              count++;
            }
          if (new_pos == old_pos)
            ctk_widget_error_bell (CTK_WIDGET (label));
          break;
        case CTK_MOVEMENT_DISPLAY_LINE_ENDS:
        case CTK_MOVEMENT_PARAGRAPH_ENDS:
        case CTK_MOVEMENT_BUFFER_ENDS:
          /* FIXME: Can do better here */
          new_pos = count < 0 ? 0 : strlen (priv->text);
          if (new_pos == old_pos)
            ctk_widget_error_bell (CTK_WIDGET (label));
          break;
        case CTK_MOVEMENT_DISPLAY_LINES:
        case CTK_MOVEMENT_PARAGRAPHS:
        case CTK_MOVEMENT_PAGES:
        case CTK_MOVEMENT_HORIZONTAL_PAGES:
          break;
        }
    }

  if (extend_selection)
    ctk_label_select_region_index (label,
                                   priv->select_info->selection_anchor,
                                   new_pos);
  else
    ctk_label_select_region_index (label, new_pos, new_pos);
}

static void
ctk_label_copy_clipboard (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;

  if (priv->text && priv->select_info)
    {
      gint start, end;
      gint len;
      CtkClipboard *clipboard;

      start = MIN (priv->select_info->selection_anchor,
                   priv->select_info->selection_end);
      end = MAX (priv->select_info->selection_anchor,
                 priv->select_info->selection_end);

      len = strlen (priv->text);

      if (end > len)
        end = len;

      if (start > len)
        start = len;

      clipboard = ctk_widget_get_clipboard (CTK_WIDGET (label), GDK_SELECTION_CLIPBOARD);

      if (start != end)
	ctk_clipboard_set_text (clipboard, priv->text + start, end - start);
      else
        {
          CtkLabelLink *link;

          link = ctk_label_get_focus_link (label);
          if (link)
            ctk_clipboard_set_text (clipboard, link->uri, -1);
        }
    }
}

static void
ctk_label_select_all (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;

  ctk_label_select_region_index (label, 0, strlen (priv->text));
}

/* Quick hack of a popup menu
 */
static void
activate_cb (CtkWidget *menuitem,
	     CtkLabel  *label)
{
  const gchar *signal = g_object_get_qdata (G_OBJECT (menuitem), quark_ctk_signal);
  g_signal_emit_by_name (label, signal);
}

static void
append_action_signal (CtkLabel     *label,
		      CtkWidget    *menu,
		      const gchar  *text,
		      const gchar  *signal,
                      gboolean      sensitive)
{
  CtkWidget *menuitem = ctk_menu_item_new_with_mnemonic (text);

  g_object_set_qdata (G_OBJECT (menuitem), quark_ctk_signal, (char *)signal);
  g_signal_connect (menuitem, "activate",
		    G_CALLBACK (activate_cb), label);

  ctk_widget_set_sensitive (menuitem, sensitive);
  
  ctk_widget_show (menuitem);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
}

static void
popup_menu_detach (CtkWidget *attach_widget,
		   CtkMenu   *menu)
{
  CtkLabel *label = CTK_LABEL (attach_widget);
  CtkLabelPrivate *priv = label->priv;

  if (priv->select_info)
    priv->select_info->popup_menu = NULL;
}

static void
open_link_activate_cb (CtkMenuItem *menuitem,
                       CtkLabel    *label)
{
  CtkLabelLink *link;

  link = g_object_get_qdata (G_OBJECT (menuitem), quark_link);
  emit_activate_link (label, link);
}

static void
copy_link_activate_cb (CtkMenuItem *menuitem,
                       CtkLabel    *label)
{
  CtkLabelLink *link;
  CtkClipboard *clipboard;

  link = g_object_get_qdata (G_OBJECT (menuitem), quark_link);
  clipboard = ctk_widget_get_clipboard (CTK_WIDGET (label), GDK_SELECTION_CLIPBOARD);
  ctk_clipboard_set_text (clipboard, link->uri, -1);
}

static gboolean
ctk_label_popup_menu (CtkWidget *widget)
{
  ctk_label_do_popup (CTK_LABEL (widget), NULL);

  return TRUE;
}

static void
ctk_label_do_popup (CtkLabel       *label,
                    const GdkEvent *event)
{
  CtkLabelPrivate *priv = label->priv;
  CtkWidget *menuitem;
  CtkWidget *menu;
  gboolean have_selection;
  CtkLabelLink *link;

  if (!priv->select_info)
    return;

  if (priv->select_info->popup_menu)
    ctk_widget_destroy (priv->select_info->popup_menu);

  priv->select_info->popup_menu = menu = ctk_menu_new ();
  ctk_style_context_add_class (ctk_widget_get_style_context (menu),
                               CTK_STYLE_CLASS_CONTEXT_MENU);

  ctk_menu_attach_to_widget (CTK_MENU (menu), CTK_WIDGET (label), popup_menu_detach);

  have_selection =
    priv->select_info->selection_anchor != priv->select_info->selection_end;

  if (event)
    {
      if (priv->select_info->link_clicked)
        link = priv->select_info->active_link;
      else
        link = NULL;
    }
  else
    link = ctk_label_get_focus_link (label);

  if (!have_selection && link)
    {
      /* Open Link */
      menuitem = ctk_menu_item_new_with_mnemonic (_("_Open Link"));
      g_object_set_qdata (G_OBJECT (menuitem), quark_link, link);
      ctk_widget_show (menuitem);
      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);

      g_signal_connect (G_OBJECT (menuitem), "activate",
                        G_CALLBACK (open_link_activate_cb), label);

      /* Copy Link Address */
      menuitem = ctk_menu_item_new_with_mnemonic (_("Copy _Link Address"));
      g_object_set_qdata (G_OBJECT (menuitem), quark_link, link);
      ctk_widget_show (menuitem);
      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);

      g_signal_connect (G_OBJECT (menuitem), "activate",
                        G_CALLBACK (copy_link_activate_cb), label);
    }
  else
    {
      append_action_signal (label, menu, _("Cu_t"), "cut-clipboard", FALSE);
      append_action_signal (label, menu, _("_Copy"), "copy-clipboard", have_selection);
      append_action_signal (label, menu, _("_Paste"), "paste-clipboard", FALSE);
  
      menuitem = ctk_menu_item_new_with_mnemonic (_("_Delete"));
      ctk_widget_set_sensitive (menuitem, FALSE);
      ctk_widget_show (menuitem);
      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);

      menuitem = ctk_separator_menu_item_new ();
      ctk_widget_show (menuitem);
      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);

      menuitem = ctk_menu_item_new_with_mnemonic (_("Select _All"));
      g_signal_connect_swapped (menuitem, "activate",
			        G_CALLBACK (ctk_label_select_all), label);
      ctk_widget_show (menuitem);
      ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
    }

  g_signal_emit (label, signals[POPULATE_POPUP], 0, menu);

  if (event && cdk_event_triggers_context_menu (event))
    ctk_menu_popup_at_pointer (CTK_MENU (menu), event);
  else
    {
      ctk_menu_popup_at_widget (CTK_MENU (menu),
                                CTK_WIDGET (label),
                                GDK_GRAVITY_SOUTH,
                                GDK_GRAVITY_NORTH_WEST,
                                event);

      ctk_menu_shell_select_first (CTK_MENU_SHELL (menu), FALSE);
    }
}

static void
ctk_label_clear_links (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;

  if (!priv->select_info)
    return;

  g_list_free_full (priv->select_info->links, (GDestroyNotify) link_free);
  priv->select_info->links = NULL;
  priv->select_info->active_link = NULL;

  _ctk_label_accessible_update_links (label);
}

static gboolean
ctk_label_activate_link (CtkLabel    *label,
                         const gchar *uri)
{
  CtkWidget *widget = CTK_WIDGET (label);
  CtkWidget *top_level = ctk_widget_get_toplevel (widget);
  guint32 timestamp = ctk_get_current_event_time ();
  GError *error = NULL;

  if (!ctk_show_uri_on_window (CTK_WINDOW (top_level), uri, timestamp, &error))
    {
      g_warning ("Unable to show '%s': %s", uri, error->message);
      g_error_free (error);
    }

  return TRUE;
}

static void
emit_activate_link (CtkLabel     *label,
                    CtkLabelLink *link)
{
  CtkLabelPrivate *priv = label->priv;
  gboolean handled;
  CtkStateFlags state;

  g_signal_emit (label, signals[ACTIVATE_LINK], 0, link->uri, &handled);
  if (handled && priv->track_links && !link->visited &&
      priv->select_info && priv->select_info->links)
    {
      link->visited = TRUE;
      state = ctk_css_node_get_state (link->cssnode);
      ctk_css_node_set_state (link->cssnode, (state & ~CTK_STATE_FLAG_LINK) | CTK_STATE_FLAG_VISITED);
      /* FIXME: shouldn't have to redo everything here */
      ctk_label_clear_layout (label);
    }
}

static void
ctk_label_activate_current_link (CtkLabel *label)
{
  CtkLabelLink *link;
  CtkWidget *widget = CTK_WIDGET (label);

  link = ctk_label_get_focus_link (label);

  if (link)
    {
      emit_activate_link (label, link);
    }
  else
    {
      CtkWidget *toplevel;
      CtkWindow *window;
      CtkWidget *default_widget, *focus_widget;

      toplevel = ctk_widget_get_toplevel (widget);
      if (CTK_IS_WINDOW (toplevel))
        {
          window = CTK_WINDOW (toplevel);

          if (window)
            {
              default_widget = ctk_window_get_default_widget (window);
              focus_widget = ctk_window_get_focus (window);

              if (default_widget != widget &&
                  !(widget == focus_widget && (!default_widget || !ctk_widget_is_sensitive (default_widget))))
                ctk_window_activate_default (window);
            }
        }
    }
}

static CtkLabelLink *
ctk_label_get_current_link (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;
  CtkLabelLink *link;

  if (!priv->select_info)
    return NULL;

  if (priv->select_info->link_clicked)
    link = priv->select_info->active_link;
  else
    link = ctk_label_get_focus_link (label);

  return link;
}

/**
 * ctk_label_get_current_uri:
 * @label: a #CtkLabel
 *
 * Returns the URI for the currently active link in the label.
 * The active link is the one under the mouse pointer or, in a
 * selectable label, the link in which the text cursor is currently
 * positioned.
 *
 * This function is intended for use in a #CtkLabel::activate-link handler
 * or for use in a #CtkWidget::query-tooltip handler.
 *
 * Returns: the currently active URI. The string is owned by CTK+ and must
 *   not be freed or modified.
 *
 * Since: 2.18
 */
const gchar *
ctk_label_get_current_uri (CtkLabel *label)
{
  CtkLabelLink *link;

  g_return_val_if_fail (CTK_IS_LABEL (label), NULL);

  link = ctk_label_get_current_link (label);

  if (link)
    return link->uri;

  return NULL;
}

/**
 * ctk_label_set_track_visited_links:
 * @label: a #CtkLabel
 * @track_links: %TRUE to track visited links
 *
 * Sets whether the label should keep track of clicked
 * links (and use a different color for them).
 *
 * Since: 2.18
 */
void
ctk_label_set_track_visited_links (CtkLabel *label,
                                   gboolean  track_links)
{
  CtkLabelPrivate *priv;

  g_return_if_fail (CTK_IS_LABEL (label));

  priv = label->priv;

  track_links = track_links != FALSE;

  if (priv->track_links != track_links)
    {
      priv->track_links = track_links;

      /* FIXME: shouldn't have to redo everything here */
      ctk_label_recalculate (label);

      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_TRACK_VISITED_LINKS]);
    }
}

/**
 * ctk_label_get_track_visited_links:
 * @label: a #CtkLabel
 *
 * Returns whether the label is currently keeping track
 * of clicked links.
 *
 * Returns: %TRUE if clicked links are remembered
 *
 * Since: 2.18
 */
gboolean
ctk_label_get_track_visited_links (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), FALSE);

  return label->priv->track_links;
}

static gboolean
ctk_label_query_tooltip (CtkWidget  *widget,
                         gint        x,
                         gint        y,
                         gboolean    keyboard_tip,
                         CtkTooltip *tooltip)
{
  CtkLabel *label = CTK_LABEL (widget);
  CtkLabelPrivate *priv = label->priv;
  CtkLabelSelectionInfo *info = priv->select_info;
  gint index = -1;
  GList *l;

  if (info && info->links)
    {
      if (keyboard_tip)
        {
          if (info->selection_anchor == info->selection_end)
            index = info->selection_anchor;
        }
      else
        {
          if (!get_layout_index (label, x, y, &index))
            index = -1;
        }

      if (index != -1)
        {
          for (l = info->links; l != NULL; l = l->next)
            {
              CtkLabelLink *link = l->data;
              if (index >= link->start && index <= link->end)
                {
                  if (link->title)
                    {
                      ctk_tooltip_set_markup (tooltip, link->title);
                      return TRUE;
                    }
                  break;
                }
            }
        }
    }

  return CTK_WIDGET_CLASS (ctk_label_parent_class)->query_tooltip (widget,
                                                                   x, y,
                                                                   keyboard_tip,
                                                                   tooltip);
}

gint
_ctk_label_get_cursor_position (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;

  if (priv->select_info && priv->select_info->selectable)
    return g_utf8_pointer_to_offset (priv->text,
                                     priv->text + priv->select_info->selection_end);

  return 0;
}

gint
_ctk_label_get_selection_bound (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;

  if (priv->select_info && priv->select_info->selectable)
    return g_utf8_pointer_to_offset (priv->text,
                                     priv->text + priv->select_info->selection_anchor);

  return 0;
}

/**
 * ctk_label_set_lines:
 * @label: a #CtkLabel
 * @lines: the desired number of lines, or -1
 *
 * Sets the number of lines to which an ellipsized, wrapping label
 * should be limited. This has no effect if the label is not wrapping
 * or ellipsized. Set this to -1 if you don’t want to limit the
 * number of lines.
 *
 * Since: 3.10
 */
void
ctk_label_set_lines (CtkLabel *label,
                     gint      lines)
{
  CtkLabelPrivate *priv;

  g_return_if_fail (CTK_IS_LABEL (label));

  priv = label->priv;

  if (priv->lines != lines)
    {
      priv->lines = lines;
      ctk_label_clear_layout (label);
      g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_LINES]);
      ctk_widget_queue_resize (CTK_WIDGET (label));
    }
}

/**
 * ctk_label_get_lines:
 * @label: a #CtkLabel
 *
 * Gets the number of lines to which an ellipsized, wrapping
 * label should be limited. See ctk_label_set_lines().
 *
 * Returns: The number of lines
 *
 * Since: 3.10
 */
gint
ctk_label_get_lines (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), -1);

  return label->priv->lines;
}

gint
_ctk_label_get_n_links (CtkLabel *label)
{
  CtkLabelPrivate *priv = label->priv;

  if (priv->select_info)
    return g_list_length (priv->select_info->links);

  return 0;
}

const gchar *
_ctk_label_get_link_uri (CtkLabel *label,
                         gint      idx)
{
  CtkLabelPrivate *priv = label->priv;

  if (priv->select_info)
    {
      CtkLabelLink *link = g_list_nth_data (priv->select_info->links, idx);
      if (link)
        return link->uri;
    }

  return NULL;
}

void
_ctk_label_get_link_extent (CtkLabel *label,
                            gint      idx,
                            gint     *start,
                            gint     *end)
{
  CtkLabelPrivate *priv = label->priv;
  gint i;
  GList *l;
  CtkLabelLink *link;

  if (priv->select_info)
    for (l = priv->select_info->links, i = 0; l; l = l->next, i++)
      {
        if (i == idx)
          {
            link = l->data;
            *start = link->start;
            *end = link->end;
            return;
          }
      }

  *start = -1;
  *end = -1;
}

gint
_ctk_label_get_link_at (CtkLabel *label, 
                        gint      pos)
{
  CtkLabelPrivate *priv = label->priv;
  gint i;
  GList *l;
  CtkLabelLink *link;

  if (priv->select_info)
    for (l = priv->select_info->links, i = 0; l; l = l->next, i++)
      {
        link = l->data;
        if (link->start <= pos && pos < link->end)
          return i;
      }

  return -1;
}

void
_ctk_label_activate_link (CtkLabel *label,
                          gint      idx)
{
  CtkLabelPrivate *priv = label->priv;

  if (priv->select_info)
    {
      CtkLabelLink *link = g_list_nth_data (priv->select_info->links, idx);

      if (link)
        emit_activate_link (label, link);
    }
}

gboolean
_ctk_label_get_link_visited (CtkLabel *label,
                             gint      idx)
{
  CtkLabelPrivate *priv = label->priv;

  if (priv->select_info)
    {
      CtkLabelLink *link = g_list_nth_data (priv->select_info->links, idx);
      return link ? link->visited : FALSE;
    }

  return FALSE;
}

gboolean
_ctk_label_get_link_focused (CtkLabel *label,
                             gint      idx)
{
  CtkLabelPrivate *priv = label->priv;
  gint i;
  GList *l;
  CtkLabelLink *link;
  CtkLabelSelectionInfo *info = priv->select_info;

  if (!info)
    return FALSE;

  if (info->selection_anchor != info->selection_end)
    return FALSE;

  for (l = info->links, i = 0; l; l = l->next, i++)
    {
      if (i == idx)
        {
          link = l->data;
          if (link->start <= info->selection_anchor &&
              info->selection_anchor <= link->end)
            return TRUE;
        }
    }

  return FALSE;
}

/**
 * ctk_label_set_xalign:
 * @label: a #CtkLabel
 * @xalign: the new xalign value, between 0 and 1
 *
 * Sets the #CtkLabel:xalign property for @label.
 *
 * Since: 3.16
 */
void
ctk_label_set_xalign (CtkLabel *label,
                      gfloat    xalign)
{
  g_return_if_fail (CTK_IS_LABEL (label));

  xalign = CLAMP (xalign, 0.0, 1.0); 

  if (label->priv->xalign == xalign)
    return;

  label->priv->xalign = xalign;

  ctk_widget_queue_draw (CTK_WIDGET (label));
  g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_XALIGN]);
}

/**
 * ctk_label_get_xalign:
 * @label: a #CtkLabel
 *
 * Gets the #CtkLabel:xalign property for @label.
 *
 * Returns: the xalign property
 *
 * Since: 3.16
 */
gfloat
ctk_label_get_xalign (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), 0.5);

  return label->priv->xalign;
}

/**
 * ctk_label_set_yalign:
 * @label: a #CtkLabel
 * @yalign: the new yalign value, between 0 and 1
 *
 * Sets the #CtkLabel:yalign property for @label.
 *
 * Since: 3.16
 */
void
ctk_label_set_yalign (CtkLabel *label,
                      gfloat    yalign)
{
  g_return_if_fail (CTK_IS_LABEL (label));

  yalign = CLAMP (yalign, 0.0, 1.0); 

  if (label->priv->yalign == yalign)
    return;

  label->priv->yalign = yalign;

  ctk_widget_queue_draw (CTK_WIDGET (label));
  g_object_notify_by_pspec (G_OBJECT (label), label_props[PROP_YALIGN]);
}

/**
 * ctk_label_get_yalign:
 * @label: a #CtkLabel
 *
 * Gets the #CtkLabel:yalign property for @label.
 *
 * Returns: the yalign property
 *
 * Since: 3.16
 */
gfloat
ctk_label_get_yalign (CtkLabel *label)
{
  g_return_val_if_fail (CTK_IS_LABEL (label), 0.5);

  return label->priv->yalign;
}
