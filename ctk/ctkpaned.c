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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include "ctkpaned.h"

#include "ctkbindings.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkorientable.h"
#include "ctkwindow.h"
#include "ctktypebuiltins.h"
#include "ctkorientableprivate.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctkwidgetprivate.h"
#include "ctkcontainerprivate.h"
#include "a11y/ctkpanedaccessible.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkcssnodeprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkcssnumbervalueprivate.h"

#include <math.h>

/**
 * SECTION:ctkpaned
 * @Short_description: A widget with two adjustable panes
 * @Title: CtkPaned
 *
 * #CtkPaned has two panes, arranged either
 * horizontally or vertically. The division between
 * the two panes is adjustable by the user by dragging
 * a handle.
 *
 * Child widgets are
 * added to the panes of the widget with ctk_paned_pack1() and
 * ctk_paned_pack2(). The division between the two children is set by default
 * from the size requests of the children, but it can be adjusted by the
 * user.
 *
 * A paned widget draws a separator between the two child widgets and a
 * small handle that the user can drag to adjust the division. It does not
 * draw any relief around the children or around the separator. (The space
 * in which the separator is called the gutter.) Often, it is useful to put
 * each child inside a #CtkFrame with the shadow type set to %CTK_SHADOW_IN
 * so that the gutter appears as a ridge. No separator is drawn if one of
 * the children is missing.
 *
 * Each child has two options that can be set, @resize and @shrink. If
 * @resize is true, then when the #CtkPaned is resized, that child will
 * expand or shrink along with the paned widget. If @shrink is true, then
 * that child can be made smaller than its requisition by the user.
 * Setting @shrink to %FALSE allows the application to set a minimum size.
 * If @resize is false for both children, then this is treated as if
 * @resize is true for both children.
 *
 * The application can set the position of the slider as if it were set
 * by the user, by calling ctk_paned_set_position().
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * paned
 * ├── <child>
 * ├── separator[.wide]
 * ╰── <child>
 * ]|
 *
 * CtkPaned has a main CSS node with name paned, and a subnode for
 * the separator with name separator. The subnode gets a .wide style
 * class when the paned is supposed to be wide.
 *
 * In horizontal orientation, the nodes of the children are always arranged
 * from left to right. So :first-child will always select the leftmost child,
 * regardless of text direction.
 *
 * ## Creating a paned widget with minimum sizes.
 *
 * |[<!-- language="C" -->
 * CtkWidget *hpaned = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);
 * CtkWidget *frame1 = ctk_frame_new (NULL);
 * CtkWidget *frame2 = ctk_frame_new (NULL);
 * ctk_frame_set_shadow_type (CTK_FRAME (frame1), CTK_SHADOW_IN);
 * ctk_frame_set_shadow_type (CTK_FRAME (frame2), CTK_SHADOW_IN);
 *
 * ctk_widget_set_size_request (hpaned, 200, -1);
 *
 * ctk_paned_pack1 (CTK_PANED (hpaned), frame1, TRUE, FALSE);
 * ctk_widget_set_size_request (frame1, 50, -1);
 *
 * ctk_paned_pack2 (CTK_PANED (hpaned), frame2, FALSE, FALSE);
 * ctk_widget_set_size_request (frame2, 50, -1);
 * ]|
 */

enum {
  CHILD1,
  CHILD2
};

struct _CtkPanedPrivate
{
  CtkPaned       *first_paned;
  CtkWidget      *child1;
  CtkWidget      *child2;
  GdkWindow      *child1_window;
  GdkWindow      *child2_window;
  CtkWidget      *last_child1_focus;
  CtkWidget      *last_child2_focus;
  CtkWidget      *saved_focus;
  CtkOrientation  orientation;

  GdkRectangle   handle_pos;
  GdkWindow     *handle;

  CtkCssGadget  *gadget;
  CtkCssGadget  *handle_gadget;

  CtkGesture    *pan_gesture;  /* Used for touch */
  CtkGesture    *drag_gesture; /* Used for mice */

  gint          child1_size;
  gint          drag_pos;
  gint          last_allocation;
  gint          max_position;
  gint          min_position;
  gint          original_position;

  guint         handle_prelit : 1;
  guint         in_recursion  : 1;
  guint         child1_resize : 1;
  guint         child1_shrink : 1;
  guint         child2_resize : 1;
  guint         child2_shrink : 1;
  guint         position_set  : 1;
  guint         panning       : 1;
};

enum {
  PROP_0,
  PROP_ORIENTATION,
  PROP_POSITION,
  PROP_POSITION_SET,
  PROP_MIN_POSITION,
  PROP_MAX_POSITION,
  PROP_WIDE_HANDLE
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_RESIZE,
  CHILD_PROP_SHRINK
};

enum {
  CYCLE_CHILD_FOCUS,
  TOGGLE_HANDLE_FOCUS,
  MOVE_HANDLE,
  CYCLE_HANDLE_FOCUS,
  ACCEPT_POSITION,
  CANCEL_POSITION,
  LAST_SIGNAL
};

static void     ctk_paned_set_property          (GObject          *object,
						 guint             prop_id,
						 const GValue     *value,
						 GParamSpec       *pspec);
static void     ctk_paned_get_property          (GObject          *object,
						 guint             prop_id,
						 GValue           *value,
						 GParamSpec       *pspec);
static void     ctk_paned_set_child_property    (CtkContainer     *container,
                                                 CtkWidget        *child,
                                                 guint             property_id,
                                                 const GValue     *value,
                                                 GParamSpec       *pspec);
static void     ctk_paned_get_child_property    (CtkContainer     *container,
                                                 CtkWidget        *child,
                                                 guint             property_id,
                                                 GValue           *value,
                                                 GParamSpec       *pspec);
static void     ctk_paned_finalize              (GObject          *object);

static void     ctk_paned_get_preferred_width   (CtkWidget        *widget,
                                                 gint             *minimum,
                                                 gint             *natural);
static void     ctk_paned_get_preferred_height  (CtkWidget        *widget,
                                                 gint             *minimum,
                                                 gint             *natural);
static void     ctk_paned_get_preferred_width_for_height
                                                (CtkWidget        *widget,
                                                 gint              height,
                                                 gint             *minimum,
                                                 gint             *natural);
static void     ctk_paned_get_preferred_height_for_width
                                                (CtkWidget        *widget,
                                                 gint              width,
                                                 gint             *minimum,
                                                 gint              *natural);

static void     ctk_paned_size_allocate         (CtkWidget        *widget,
                                                 CtkAllocation    *allocation);
static void     ctk_paned_realize               (CtkWidget        *widget);
static void     ctk_paned_unrealize             (CtkWidget        *widget);
static void     ctk_paned_map                   (CtkWidget        *widget);
static void     ctk_paned_unmap                 (CtkWidget        *widget);
static void     ctk_paned_state_flags_changed   (CtkWidget        *widget,
                                                 CtkStateFlags     previous_state);
static void     ctk_paned_direction_changed     (CtkWidget        *widget,
                                                 CtkTextDirection  previous_direction);
static gboolean ctk_paned_draw                  (CtkWidget        *widget,
						 cairo_t          *cr);
static gboolean ctk_paned_enter                 (CtkWidget        *widget,
						 GdkEventCrossing *event);
static gboolean ctk_paned_leave                 (CtkWidget        *widget,
						 GdkEventCrossing *event);
static gboolean ctk_paned_focus                 (CtkWidget        *widget,
						 CtkDirectionType  direction);
static void     ctk_paned_add                   (CtkContainer     *container,
						 CtkWidget        *widget);
static void     ctk_paned_remove                (CtkContainer     *container,
						 CtkWidget        *widget);
static void     ctk_paned_forall                (CtkContainer     *container,
						 gboolean          include_internals,
						 CtkCallback       callback,
						 gpointer          callback_data);
static void     ctk_paned_calc_position         (CtkPaned         *paned,
                                                 gint              allocation,
                                                 gint              child1_req,
                                                 gint              child2_req);
static void     ctk_paned_set_focus_child       (CtkContainer     *container,
						 CtkWidget        *child);
static void     ctk_paned_set_saved_focus       (CtkPaned         *paned,
						 CtkWidget        *widget);
static void     ctk_paned_set_first_paned       (CtkPaned         *paned,
						 CtkPaned         *first_paned);
static void     ctk_paned_set_last_child1_focus (CtkPaned         *paned,
						 CtkWidget        *widget);
static void     ctk_paned_set_last_child2_focus (CtkPaned         *paned,
						 CtkWidget        *widget);
static gboolean ctk_paned_cycle_child_focus     (CtkPaned         *paned,
						 gboolean          reverse);
static gboolean ctk_paned_cycle_handle_focus    (CtkPaned         *paned,
						 gboolean          reverse);
static gboolean ctk_paned_move_handle           (CtkPaned         *paned,
						 CtkScrollType     scroll);
static gboolean ctk_paned_accept_position       (CtkPaned         *paned);
static gboolean ctk_paned_cancel_position       (CtkPaned         *paned);
static gboolean ctk_paned_toggle_handle_focus   (CtkPaned         *paned);

static GType    ctk_paned_child_type            (CtkContainer     *container);

static void     update_drag                     (CtkPaned         *paned,
                                                 int               xpos,
                                                 int               ypos);

G_DEFINE_TYPE_WITH_CODE (CtkPaned, ctk_paned, CTK_TYPE_CONTAINER,
                         G_ADD_PRIVATE (CtkPaned)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ORIENTABLE,
                                                NULL))

static guint signals[LAST_SIGNAL] = { 0 };


static void
add_tab_bindings (CtkBindingSet    *binding_set,
		  GdkModifierType   modifiers)
{
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_Tab, modifiers,
                                "toggle-handle-focus", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Tab, modifiers,
				"toggle-handle-focus", 0);
}

static void
add_move_binding (CtkBindingSet   *binding_set,
		  guint            keyval,
		  GdkModifierType  mask,
		  CtkScrollType    scroll)
{
  ctk_binding_entry_add_signal (binding_set, keyval, mask,
				"move-handle", 1,
				CTK_TYPE_SCROLL_TYPE, scroll);
}

static void
ctk_paned_class_init (CtkPanedClass *class)
{
  GObjectClass *object_class;
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;
  CtkPanedClass *paned_class;
  CtkBindingSet *binding_set;

  object_class = (GObjectClass *) class;
  widget_class = (CtkWidgetClass *) class;
  container_class = (CtkContainerClass *) class;
  paned_class = (CtkPanedClass *) class;

  object_class->set_property = ctk_paned_set_property;
  object_class->get_property = ctk_paned_get_property;
  object_class->finalize = ctk_paned_finalize;

  widget_class->get_preferred_width = ctk_paned_get_preferred_width;
  widget_class->get_preferred_height = ctk_paned_get_preferred_height;
  widget_class->get_preferred_width_for_height = ctk_paned_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = ctk_paned_get_preferred_height_for_width;
  widget_class->size_allocate = ctk_paned_size_allocate;
  widget_class->realize = ctk_paned_realize;
  widget_class->unrealize = ctk_paned_unrealize;
  widget_class->map = ctk_paned_map;
  widget_class->unmap = ctk_paned_unmap;
  widget_class->draw = ctk_paned_draw;
  widget_class->focus = ctk_paned_focus;
  widget_class->enter_notify_event = ctk_paned_enter;
  widget_class->leave_notify_event = ctk_paned_leave;
  widget_class->state_flags_changed = ctk_paned_state_flags_changed;
  widget_class->direction_changed = ctk_paned_direction_changed;

  container_class->add = ctk_paned_add;
  container_class->remove = ctk_paned_remove;
  container_class->forall = ctk_paned_forall;
  container_class->child_type = ctk_paned_child_type;
  container_class->set_focus_child = ctk_paned_set_focus_child;
  container_class->set_child_property = ctk_paned_set_child_property;
  container_class->get_child_property = ctk_paned_get_child_property;
  ctk_container_class_handle_border_width (container_class);

  paned_class->cycle_child_focus = ctk_paned_cycle_child_focus;
  paned_class->toggle_handle_focus = ctk_paned_toggle_handle_focus;
  paned_class->move_handle = ctk_paned_move_handle;
  paned_class->cycle_handle_focus = ctk_paned_cycle_handle_focus;
  paned_class->accept_position = ctk_paned_accept_position;
  paned_class->cancel_position = ctk_paned_cancel_position;

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  g_object_class_install_property (object_class,
                                   PROP_POSITION,
                                   g_param_spec_int ("position",
                                                     P_("Position"),
                                                     P_("Position of paned separator in pixels (0 means all the way to the left/top)"),
                                                     0, G_MAXINT, 0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
                                   PROP_POSITION_SET,
                                   g_param_spec_boolean ("position-set",
                                                         P_("Position Set"),
                                                         P_("TRUE if the Position property should be used"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkPaned:min-position:
   *
   * The smallest possible value for the position property.
   * This property is derived from the size and shrinkability
   * of the widget's children.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_MIN_POSITION,
                                   g_param_spec_int ("min-position",
                                                     P_("Minimal Position"),
                                                     P_("Smallest possible value for the \"position\" property"),
                                                     0, G_MAXINT, 0,
                                                     CTK_PARAM_READABLE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkPaned:max-position:
   *
   * The largest possible value for the position property.
   * This property is derived from the size and shrinkability
   * of the widget's children.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_MAX_POSITION,
                                   g_param_spec_int ("max-position",
                                                     P_("Maximal Position"),
                                                     P_("Largest possible value for the \"position\" property"),
                                                     0, G_MAXINT, G_MAXINT,
                                                     CTK_PARAM_READABLE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkPaned:wide-handle:
   *
   * Setting this property to %TRUE indicates that the paned needs
   * to provide stronger visual separation (e.g. because it separates
   * between two notebooks, whose tab rows would otherwise merge visually).
   *
   * Since: 3.16 
   */
  g_object_class_install_property (object_class,
                                   PROP_WIDE_HANDLE,
                                   g_param_spec_boolean ("wide-handle",
                                                         P_("Wide Handle"),
                                                         P_("Whether the paned should have a prominent handle"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkPaned::handle-size:
   *
   * The width of the handle.
   *
   * Deprecated: 3.20: Use CSS min-width and min-height instead.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("handle-size",
							     P_("Handle Size"),
							     P_("Width of handle"),
							     0,
							     G_MAXINT,
							     5,
							     CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkPaned:resize:
   *
   * The "resize" child property determines whether the child expands and
   * shrinks along with the paned widget.
   *
   * Since: 2.4
   */
  ctk_container_class_install_child_property (container_class,
					      CHILD_PROP_RESIZE,
					      g_param_spec_boolean ("resize", 
								    P_("Resize"),
								    P_("If TRUE, the child expands and shrinks along with the paned widget"),
								    TRUE,
								    CTK_PARAM_READWRITE));

  /**
   * CtkPaned:shrink:
   *
   * The "shrink" child property determines whether the child can be made
   * smaller than its requisition.
   *
   * Since: 2.4
   */
  ctk_container_class_install_child_property (container_class,
					      CHILD_PROP_SHRINK,
					      g_param_spec_boolean ("shrink", 
								    P_("Shrink"),
								    P_("If TRUE, the child can be made smaller than its requisition"),
								    TRUE,
								    CTK_PARAM_READWRITE));

  /**
   * CtkPaned::cycle-child-focus:
   * @widget: the object that received the signal
   * @reversed: whether cycling backward or forward
   *
   * The ::cycle-child-focus signal is a 
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to cycle the focus between the children of the paned.
   *
   * The default binding is f6.
   *
   * Since: 2.0
   */
  signals [CYCLE_CHILD_FOCUS] =
    g_signal_new (I_("cycle-child-focus"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkPanedClass, cycle_child_focus),
		  NULL, NULL,
		  _ctk_marshal_BOOLEAN__BOOLEAN,
		  G_TYPE_BOOLEAN, 1,
		  G_TYPE_BOOLEAN);

  /**
   * CtkPaned::toggle-handle-focus:
   * @widget: the object that received the signal
   *
   * The ::toggle-handle-focus is a 
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to accept the current position of the handle and then 
   * move focus to the next widget in the focus chain.
   *
   * The default binding is Tab.
   *
   * Since: 2.0
   */
  signals [TOGGLE_HANDLE_FOCUS] =
    g_signal_new (I_("toggle-handle-focus"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkPanedClass, toggle_handle_focus),
		  NULL, NULL,
		  _ctk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);

  /**
   * CtkPaned::move-handle:
   * @widget: the object that received the signal
   * @scroll_type: a #CtkScrollType
   *
   * The ::move-handle signal is a 
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to move the handle when the user is using key bindings 
   * to move it.
   *
   * Since: 2.0
   */
  signals[MOVE_HANDLE] =
    g_signal_new (I_("move-handle"),
		  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkPanedClass, move_handle),
                  NULL, NULL,
                  _ctk_marshal_BOOLEAN__ENUM,
                  G_TYPE_BOOLEAN, 1,
                  CTK_TYPE_SCROLL_TYPE);

  /**
   * CtkPaned::cycle-handle-focus:
   * @widget: the object that received the signal
   * @reversed: whether cycling backward or forward
   *
   * The ::cycle-handle-focus signal is a 
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to cycle whether the paned should grab focus to allow
   * the user to change position of the handle by using key bindings.
   *
   * The default binding for this signal is f8.
   *
   * Since: 2.0
   */
  signals [CYCLE_HANDLE_FOCUS] =
    g_signal_new (I_("cycle-handle-focus"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkPanedClass, cycle_handle_focus),
		  NULL, NULL,
		  _ctk_marshal_BOOLEAN__BOOLEAN,
		  G_TYPE_BOOLEAN, 1,
		  G_TYPE_BOOLEAN);

  /**
   * CtkPaned::accept-position:
   * @widget: the object that received the signal
   *
   * The ::accept-position signal is a 
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to accept the current position of the handle when 
   * moving it using key bindings.
   *
   * The default binding for this signal is Return or Space.
   *
   * Since: 2.0
   */
  signals [ACCEPT_POSITION] =
    g_signal_new (I_("accept-position"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkPanedClass, accept_position),
		  NULL, NULL,
		  _ctk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);

  /**
   * CtkPaned::cancel-position:
   * @widget: the object that received the signal
   *
   * The ::cancel-position signal is a 
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to cancel moving the position of the handle using key 
   * bindings. The position of the handle will be reset to the value prior to 
   * moving it.
   *
   * The default binding for this signal is Escape.
   *
   * Since: 2.0
   */
  signals [CANCEL_POSITION] =
    g_signal_new (I_("cancel-position"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkPanedClass, cancel_position),
		  NULL, NULL,
		  _ctk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);

  binding_set = ctk_binding_set_by_class (class);

  /* F6 and friends */
  ctk_binding_entry_add_signal (binding_set,
                                GDK_KEY_F6, 0,
                                "cycle-child-focus", 1, 
                                G_TYPE_BOOLEAN, FALSE);
  ctk_binding_entry_add_signal (binding_set,
				GDK_KEY_F6, GDK_SHIFT_MASK,
				"cycle-child-focus", 1,
				G_TYPE_BOOLEAN, TRUE);

  /* F8 and friends */
  ctk_binding_entry_add_signal (binding_set,
				GDK_KEY_F8, 0,
				"cycle-handle-focus", 1,
				G_TYPE_BOOLEAN, FALSE);
 
  ctk_binding_entry_add_signal (binding_set,
				GDK_KEY_F8, GDK_SHIFT_MASK,
				"cycle-handle-focus", 1,
				G_TYPE_BOOLEAN, TRUE);
 
  add_tab_bindings (binding_set, 0);
  add_tab_bindings (binding_set, GDK_CONTROL_MASK);
  add_tab_bindings (binding_set, GDK_SHIFT_MASK);
  add_tab_bindings (binding_set, GDK_CONTROL_MASK | GDK_SHIFT_MASK);

  /* accept and cancel positions */
  ctk_binding_entry_add_signal (binding_set,
				GDK_KEY_Escape, 0,
				"cancel-position", 0);

  ctk_binding_entry_add_signal (binding_set,
				GDK_KEY_Return, 0,
				"accept-position", 0);
  ctk_binding_entry_add_signal (binding_set,
				GDK_KEY_ISO_Enter, 0,
				"accept-position", 0);
  ctk_binding_entry_add_signal (binding_set,
				GDK_KEY_KP_Enter, 0,
				"accept-position", 0);
  ctk_binding_entry_add_signal (binding_set,
				GDK_KEY_space, 0,
				"accept-position", 0);
  ctk_binding_entry_add_signal (binding_set,
				GDK_KEY_KP_Space, 0,
				"accept-position", 0);

  /* move handle */
  add_move_binding (binding_set, GDK_KEY_Left, 0, CTK_SCROLL_STEP_LEFT);
  add_move_binding (binding_set, GDK_KEY_KP_Left, 0, CTK_SCROLL_STEP_LEFT);
  add_move_binding (binding_set, GDK_KEY_Left, GDK_CONTROL_MASK, CTK_SCROLL_PAGE_LEFT);
  add_move_binding (binding_set, GDK_KEY_KP_Left, GDK_CONTROL_MASK, CTK_SCROLL_PAGE_LEFT);

  add_move_binding (binding_set, GDK_KEY_Right, 0, CTK_SCROLL_STEP_RIGHT);
  add_move_binding (binding_set, GDK_KEY_Right, GDK_CONTROL_MASK, CTK_SCROLL_PAGE_RIGHT);
  add_move_binding (binding_set, GDK_KEY_KP_Right, 0, CTK_SCROLL_STEP_RIGHT);
  add_move_binding (binding_set, GDK_KEY_KP_Right, GDK_CONTROL_MASK, CTK_SCROLL_PAGE_RIGHT);

  add_move_binding (binding_set, GDK_KEY_Up, 0, CTK_SCROLL_STEP_UP);
  add_move_binding (binding_set, GDK_KEY_Up, GDK_CONTROL_MASK, CTK_SCROLL_PAGE_UP);
  add_move_binding (binding_set, GDK_KEY_KP_Up, 0, CTK_SCROLL_STEP_UP);
  add_move_binding (binding_set, GDK_KEY_KP_Up, GDK_CONTROL_MASK, CTK_SCROLL_PAGE_UP);
  add_move_binding (binding_set, GDK_KEY_Page_Up, 0, CTK_SCROLL_PAGE_UP);
  add_move_binding (binding_set, GDK_KEY_KP_Page_Up, 0, CTK_SCROLL_PAGE_UP);

  add_move_binding (binding_set, GDK_KEY_Down, 0, CTK_SCROLL_STEP_DOWN);
  add_move_binding (binding_set, GDK_KEY_Down, GDK_CONTROL_MASK, CTK_SCROLL_PAGE_DOWN);
  add_move_binding (binding_set, GDK_KEY_KP_Down, 0, CTK_SCROLL_STEP_DOWN);
  add_move_binding (binding_set, GDK_KEY_KP_Down, GDK_CONTROL_MASK, CTK_SCROLL_PAGE_DOWN);
  add_move_binding (binding_set, GDK_KEY_Page_Down, 0, CTK_SCROLL_PAGE_RIGHT);
  add_move_binding (binding_set, GDK_KEY_KP_Page_Down, 0, CTK_SCROLL_PAGE_RIGHT);

  add_move_binding (binding_set, GDK_KEY_Home, 0, CTK_SCROLL_START);
  add_move_binding (binding_set, GDK_KEY_KP_Home, 0, CTK_SCROLL_START);
  add_move_binding (binding_set, GDK_KEY_End, 0, CTK_SCROLL_END);
  add_move_binding (binding_set, GDK_KEY_KP_End, 0, CTK_SCROLL_END);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_PANED_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "paned");
}

static GType
ctk_paned_child_type (CtkContainer *container)
{
  CtkPaned *paned = CTK_PANED (container);
  CtkPanedPrivate *priv = paned->priv;

  if (!priv->child1 || !priv->child2)
    return CTK_TYPE_WIDGET;
  else
    return G_TYPE_NONE;
}

static gboolean
initiates_touch_drag (CtkPaned *paned,
                      gdouble   start_x,
                      gdouble   start_y)
{
  gint handle_size, handle_pos, drag_pos;
  CtkPanedPrivate *priv = paned->priv;
  CtkAllocation allocation;

#define TOUCH_EXTRA_AREA_WIDTH 50
  ctk_css_gadget_get_content_allocation (priv->gadget, &allocation, NULL);
  ctk_css_gadget_get_preferred_size (priv->handle_gadget,
                                     priv->orientation,
                                     -1,
                                     NULL, &handle_size,
                                     NULL, NULL);

  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      handle_pos = priv->handle_pos.x - allocation.x;
      drag_pos = start_x;
    }
  else
    {
      handle_pos = priv->handle_pos.y - allocation.y;
      drag_pos = start_y;
    }

  if (drag_pos < handle_pos - TOUCH_EXTRA_AREA_WIDTH ||
      drag_pos > handle_pos + handle_size + TOUCH_EXTRA_AREA_WIDTH)
    return FALSE;

#undef TOUCH_EXTRA_AREA_WIDTH

  return TRUE;
}

static void
gesture_drag_begin_cb (CtkGestureDrag *gesture,
                       gdouble         start_x,
                       gdouble         start_y,
                       CtkPaned       *paned)
{
  CtkPanedPrivate *priv = paned->priv;
  GdkEventSequence *sequence;
  CtkAllocation allocation;
  const GdkEvent *event;
  GdkDevice *device;
  gboolean is_touch;

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);
  device = gdk_event_get_source_device (event);
  ctk_css_gadget_get_content_allocation (priv->gadget, &allocation, NULL);
  paned->priv->panning = FALSE;

  is_touch = (event->type == GDK_TOUCH_BEGIN ||
              gdk_device_get_source (device) == GDK_SOURCE_TOUCHSCREEN);

  if ((is_touch && CTK_GESTURE (gesture) == priv->drag_gesture) ||
      (!is_touch && CTK_GESTURE (gesture) == priv->pan_gesture))
    {
      ctk_gesture_set_state (CTK_GESTURE (gesture),
                             CTK_EVENT_SEQUENCE_DENIED);
      return;
    }

  if (event->any.window == priv->handle ||
      (is_touch && initiates_touch_drag (paned, start_x, start_y)))
    {
      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        priv->drag_pos = start_x - (priv->handle_pos.x - allocation.x);
      else
        priv->drag_pos = start_y - (priv->handle_pos.y - allocation.y);

      ctk_gesture_set_state (CTK_GESTURE (gesture),
                             CTK_EVENT_SEQUENCE_CLAIMED);
    }
  else
    {
      ctk_gesture_set_state (CTK_GESTURE (gesture),
                             CTK_EVENT_SEQUENCE_DENIED);
    }
}

static void
gesture_drag_update_cb (CtkGestureDrag   *gesture,
                        gdouble           offset_x,
                        gdouble           offset_y,
                        CtkPaned         *paned)
{
  gdouble start_x, start_y;

  paned->priv->panning = TRUE;

  ctk_gesture_drag_get_start_point (CTK_GESTURE_DRAG (gesture),
                               &start_x, &start_y);
  update_drag (paned, start_x + offset_x, start_y + offset_y);
}

static void
gesture_drag_end_cb (CtkGestureDrag *gesture,
                     gdouble         offset_x,
                     gdouble         offset_y,
                     CtkPaned       *paned)
{
  if (!paned->priv->panning)
    ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_DENIED);
}

static void
ctk_paned_set_property (GObject        *object,
			guint           prop_id,
			const GValue   *value,
			GParamSpec     *pspec)
{
  CtkPaned *paned = CTK_PANED (object);
  CtkPanedPrivate *priv = paned->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      if (priv->orientation != g_value_get_enum (value))
        {
          priv->orientation = g_value_get_enum (value);
          _ctk_orientable_set_style_classes (CTK_ORIENTABLE (paned));

          if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
            ctk_gesture_pan_set_orientation (CTK_GESTURE_PAN (priv->pan_gesture),
                                             CTK_ORIENTATION_HORIZONTAL);
          else
            ctk_gesture_pan_set_orientation (CTK_GESTURE_PAN (priv->pan_gesture),
                                             CTK_ORIENTATION_VERTICAL);

          /* state_flags_changed updates the cursor */
          ctk_paned_state_flags_changed (CTK_WIDGET (paned), 0);
          ctk_widget_queue_resize (CTK_WIDGET (paned));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_POSITION:
      ctk_paned_set_position (paned, g_value_get_int (value));
      break;
    case PROP_POSITION_SET:
      if (priv->position_set != g_value_get_boolean (value))
        {
          priv->position_set = g_value_get_boolean (value);
          ctk_widget_queue_resize_no_redraw (CTK_WIDGET (paned));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_WIDE_HANDLE:
      ctk_paned_set_wide_handle (paned, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_paned_get_property (GObject        *object,
			guint           prop_id,
			GValue         *value,
			GParamSpec     *pspec)
{
  CtkPaned *paned = CTK_PANED (object);
  CtkPanedPrivate *priv = paned->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    case PROP_POSITION:
      g_value_set_int (value, priv->child1_size);
      break;
    case PROP_POSITION_SET:
      g_value_set_boolean (value, priv->position_set);
      break;
    case PROP_MIN_POSITION:
      g_value_set_int (value, priv->min_position);
      break;
    case PROP_MAX_POSITION:
      g_value_set_int (value, priv->max_position);
      break;
    case PROP_WIDE_HANDLE:
      g_value_set_boolean (value, ctk_paned_get_wide_handle (paned));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_paned_set_child_property (CtkContainer    *container,
			      CtkWidget       *child,
			      guint            property_id,
			      const GValue    *value,
			      GParamSpec      *pspec)
{
  CtkPaned *paned = CTK_PANED (container);
  CtkPanedPrivate *priv = paned->priv;
  gboolean old_value, new_value;

  g_assert (child == priv->child1 || child == priv->child2);

  new_value = g_value_get_boolean (value);
  switch (property_id)
    {
    case CHILD_PROP_RESIZE:
      if (child == priv->child1)
	{
	  old_value = priv->child1_resize;
	  priv->child1_resize = new_value;
	}
      else
	{
	  old_value = priv->child2_resize;
	  priv->child2_resize = new_value;
	}
      break;
    case CHILD_PROP_SHRINK:
      if (child == priv->child1)
	{
	  old_value = priv->child1_shrink;
	  priv->child1_shrink = new_value;
	}
      else
	{
	  old_value = priv->child2_shrink;
	  priv->child2_shrink = new_value;
	}
      break;
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      old_value = -1; /* quiet gcc */
      break;
    }
  if (old_value != new_value)
    ctk_widget_queue_resize_no_redraw (CTK_WIDGET (container));
}

static void
ctk_paned_get_child_property (CtkContainer *container,
			      CtkWidget    *child,
			      guint         property_id,
			      GValue       *value,
			      GParamSpec   *pspec)
{
  CtkPaned *paned = CTK_PANED (container);
  CtkPanedPrivate *priv = paned->priv;

  g_assert (child == priv->child1 || child == priv->child2);
  
  switch (property_id)
    {
    case CHILD_PROP_RESIZE:
      if (child == priv->child1)
	g_value_set_boolean (value, priv->child1_resize);
      else
	g_value_set_boolean (value, priv->child2_resize);
      break;
    case CHILD_PROP_SHRINK:
      if (child == priv->child1)
	g_value_set_boolean (value, priv->child1_shrink);
      else
	g_value_set_boolean (value, priv->child2_shrink);
      break;
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_paned_finalize (GObject *object)
{
  CtkPaned *paned = CTK_PANED (object);
  
  ctk_paned_set_saved_focus (paned, NULL);
  ctk_paned_set_first_paned (paned, NULL);

  g_clear_object (&paned->priv->pan_gesture);
  g_clear_object (&paned->priv->drag_gesture);

  g_clear_object (&paned->priv->handle_gadget);
  g_clear_object (&paned->priv->gadget);

  G_OBJECT_CLASS (ctk_paned_parent_class)->finalize (object);
}

static void
ctk_paned_compute_position (CtkPaned *paned,
                            gint      allocation,
                            gint      child1_req,
                            gint      child2_req,
                            gint     *min_pos,
                            gint     *max_pos,
                            gint     *out_pos)
{
  CtkPanedPrivate *priv = paned->priv;
  gint min, max, pos;

  min = priv->child1_shrink ? 0 : child1_req;

  max = allocation;
  if (!priv->child2_shrink)
    max = MAX (1, max - child2_req);
  max = MAX (min, max);

  if (!priv->position_set)
    {
      if (priv->child1_resize && !priv->child2_resize)
	pos = MAX (0, allocation - child2_req);
      else if (!priv->child1_resize && priv->child2_resize)
	pos = child1_req;
      else if (child1_req + child2_req != 0)
	pos = allocation * ((gdouble)child1_req / (child1_req + child2_req)) + 0.5;
      else
	pos = allocation * 0.5 + 0.5;
    }
  else
    {
      /* If the position was set before the initial allocation.
       * (priv->last_allocation <= 0) just clamp it and leave it.
       */
      if (priv->last_allocation > 0)
	{
	  if (priv->child1_resize && !priv->child2_resize)
	    pos = priv->child1_size + allocation - priv->last_allocation;
	  else if (!(!priv->child1_resize && priv->child2_resize))
	    pos = allocation * ((gdouble) priv->child1_size / (priv->last_allocation)) + 0.5;
          else
            pos = priv->child1_size;
	}
      else
        pos = priv->child1_size;
    }

  pos = CLAMP (pos, min, max);
  
  if (min_pos)
    *min_pos = min;
  if (max_pos)
    *max_pos = max;
  if (out_pos)
    *out_pos = pos;
}

static void
ctk_paned_get_preferred_size_for_orientation (CtkWidget      *widget,
                                              gint            size,
                                              gint           *minimum,
                                              gint           *natural)
{
  CtkPaned *paned = CTK_PANED (widget);
  CtkPanedPrivate *priv = paned->priv;
  gint child_min, child_nat;

  *minimum = *natural = 0;

  if (priv->child1 && ctk_widget_get_visible (priv->child1))
    {
      _ctk_widget_get_preferred_size_for_size (priv->child1, priv->orientation, size, &child_min, &child_nat, NULL, NULL);
      if (priv->child1_shrink)
        *minimum = 0;
      else
        *minimum = child_min;
      *natural = child_nat;
    }

  if (priv->child2 && ctk_widget_get_visible (priv->child2))
    {
      _ctk_widget_get_preferred_size_for_size (priv->child2, priv->orientation, size, &child_min, &child_nat, NULL, NULL);

      if (!priv->child2_shrink)
        *minimum += child_min;
      *natural += child_nat;
    }

  if (priv->child1 && ctk_widget_get_visible (priv->child1) &&
      priv->child2 && ctk_widget_get_visible (priv->child2))
    {
      gint handle_size;

      ctk_css_gadget_get_preferred_size (priv->handle_gadget,
                                         priv->orientation,
                                         -1,
                                         NULL, &handle_size,
                                         NULL, NULL);

      *minimum += handle_size;
      *natural += handle_size;
    }
}

static void
ctk_paned_get_preferred_size_for_opposite_orientation (CtkWidget      *widget,
                                                       gint            size,
                                                       gint           *minimum,
                                                       gint           *natural)
{
  CtkPaned *paned = CTK_PANED (widget);
  CtkPanedPrivate *priv = paned->priv;
  gint for_child1, for_child2;
  gint child_min, child_nat;

  if (size > -1 &&
      priv->child1 && ctk_widget_get_visible (priv->child1) &&
      priv->child2 && ctk_widget_get_visible (priv->child2))
    {
      gint child1_req, child2_req;
      gint handle_size;

      ctk_css_gadget_get_preferred_size (priv->handle_gadget,
                                         OPPOSITE_ORIENTATION (priv->orientation),
                                         -1,
                                         NULL, &handle_size,
                                         NULL, NULL);

      _ctk_widget_get_preferred_size_for_size (priv->child1, priv->orientation, -1, &child1_req, NULL, NULL, NULL);
      _ctk_widget_get_preferred_size_for_size (priv->child2, priv->orientation, -1, &child2_req, NULL, NULL, NULL);

      ctk_paned_compute_position (paned,
                                  size - handle_size, child1_req, child2_req,
                                  NULL, NULL, &for_child1);

      for_child2 = size - for_child1 - handle_size;
    }
  else
    {
      for_child1 = size;
      for_child2 = size;
    }

  *minimum = *natural = 0;

  if (priv->child1 && ctk_widget_get_visible (priv->child1))
    {
      _ctk_widget_get_preferred_size_for_size (priv->child1,
                                               OPPOSITE_ORIENTATION (priv->orientation),
                                               for_child1,
                                               &child_min, &child_nat,
                                               NULL, NULL);
      
      *minimum = child_min;
      *natural = child_nat;
    }

  if (priv->child2 && ctk_widget_get_visible (priv->child2))
    {
      _ctk_widget_get_preferred_size_for_size (priv->child2,
                                               OPPOSITE_ORIENTATION (priv->orientation),
                                               for_child2,
                                               &child_min, &child_nat,
                                               NULL, NULL);

      *minimum = MAX (*minimum, child_min);
      *natural = MAX (*natural, child_nat);
    }
}

static gint
get_number (CtkCssStyle *style,
            guint        property)
{
  double d = _ctk_css_number_value_get (ctk_css_style_get_value (style, property), 100.0);

  if (d < 1)
    return ceil (d);
  else
    return floor (d);
}

static void
ctk_paned_measure_handle (CtkCssGadget   *gadget,
                          CtkOrientation  orientation,
                          int             size,
                          int            *minimum,
                          int            *natural,
                          int            *minimum_baseline,
                          int            *natural_baseline,
                          gpointer        data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkCssStyle *style;
  gint min_size;

  style = ctk_css_gadget_get_style (gadget);
  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    min_size = get_number (style, CTK_CSS_PROPERTY_MIN_WIDTH);
  else
    min_size = get_number (style, CTK_CSS_PROPERTY_MIN_HEIGHT);

  if (min_size != 0)
    *minimum = *natural = min_size;
  else
    {
      CtkStyleContext *context;

      context = ctk_widget_get_style_context (widget);
      ctk_style_context_save_to_node (context, ctk_css_gadget_get_node (gadget));
      ctk_widget_style_get (widget, "handle-size", &min_size, NULL);
      ctk_style_context_restore (context);

      *minimum = *natural = min_size;
    }
}

static void
ctk_paned_measure (CtkCssGadget   *gadget,
                   CtkOrientation  orientation,
                   int             size,
                   int            *minimum,
                   int            *natural,
                   int            *minimum_baseline,
                   int            *natural_baseline,
                   gpointer        data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkPaned *paned = CTK_PANED (widget);
  CtkPanedPrivate *priv = paned->priv;

  if (orientation == priv->orientation)
    ctk_paned_get_preferred_size_for_orientation (widget, size, minimum, natural);
  else
    ctk_paned_get_preferred_size_for_opposite_orientation (widget, size, minimum, natural);
}

static void
ctk_paned_get_preferred_width (CtkWidget *widget,
                               gint      *minimum,
                               gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_PANED (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_paned_get_preferred_height (CtkWidget *widget,
                                gint      *minimum,
                                gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_PANED (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_paned_get_preferred_width_for_height (CtkWidget *widget,
                                          gint       height,
                                          gint      *minimum,
                                          gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_PANED (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_paned_get_preferred_height_for_width (CtkWidget *widget,
                                          gint       width,
                                          gint      *minimum,
                                          gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_PANED (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
flip_child (const CtkAllocation *allocation,
            CtkAllocation       *child_pos)
{
  gint x, width;

  x = allocation->x;
  width = allocation->width;

  child_pos->x = 2 * x + width - child_pos->x - child_pos->width;
}

static void
ctk_paned_set_child_visible (CtkPaned  *paned,
                             guint      id,
                             gboolean   visible)
{
  CtkPanedPrivate *priv = paned->priv;
  CtkWidget *child;

  child = id == CHILD1 ? priv->child1 : priv->child2;

  if (child == NULL)
    return;

  ctk_widget_set_child_visible (child, visible);

  if (ctk_widget_get_mapped (CTK_WIDGET (paned)))
    {
      GdkWindow *window = id == CHILD1 ? priv->child1_window : priv->child2_window;

      if (visible != gdk_window_is_visible (window))
        {
          if (visible)
            gdk_window_show (window);
          else
            gdk_window_hide (window);
        }
    }
}

static void
ctk_paned_child_allocate (CtkWidget           *child,
                          GdkWindow           *child_window, /* can be NULL */
                          const CtkAllocation *window_allocation,
                          CtkAllocation       *child_allocation)
{
  if (child_window)
    gdk_window_move_resize (child_window,
                            window_allocation->x, window_allocation->y,
                            window_allocation->width, window_allocation->height);

  ctk_widget_size_allocate (child, child_allocation);
}

static void
ctk_paned_size_allocate (CtkWidget     *widget,
                         CtkAllocation *allocation)
{
  CtkPaned *paned = CTK_PANED (widget);
  CtkPanedPrivate *priv = paned->priv;
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_paned_allocate (CtkCssGadget        *gadget,
                    const CtkAllocation *allocation,
                    int                  baseline,
                    CtkAllocation       *out_clip,
                    gpointer             data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkPaned *paned = CTK_PANED (widget);
  CtkPanedPrivate *priv = paned->priv;
  CtkAllocation clip = { 0 };

  if (priv->child1 && ctk_widget_get_visible (priv->child1) &&
      priv->child2 && ctk_widget_get_visible (priv->child2))
    {
      CtkAllocation child1_allocation, window1_allocation;
      CtkAllocation child2_allocation, window2_allocation;
      CtkAllocation priv_child1_allocation;
      GdkRectangle old_handle_pos;
      gint handle_size;

      ctk_css_gadget_get_preferred_size (priv->handle_gadget,
                                         priv->orientation,
                                         -1,
                                         NULL, &handle_size,
                                         NULL, NULL);

      old_handle_pos = priv->handle_pos;

      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          gint child1_width, child2_width;

          ctk_widget_get_preferred_width_for_height (priv->child1,
                                                     allocation->height,
                                                     &child1_width, NULL);
          ctk_widget_get_preferred_width_for_height (priv->child2,
                                                     allocation->height,
                                                     &child2_width, NULL);

          ctk_paned_calc_position (paned,
                                   MAX (1, allocation->width - handle_size),
                                   child1_width,
                                   child2_width);

          priv->handle_pos.x = allocation->x + priv->child1_size;
          priv->handle_pos.y = allocation->y;
          priv->handle_pos.width = handle_size;
          priv->handle_pos.height = allocation->height;

          window1_allocation.height = window2_allocation.height = allocation->height;
          window1_allocation.width = MAX (1, priv->child1_size);
          window1_allocation.x = allocation->x;
          window1_allocation.y = window2_allocation.y = allocation->y;

          window2_allocation.x = window1_allocation.x + priv->child1_size + priv->handle_pos.width;
          window2_allocation.width = MAX (1, allocation->width - priv->child1_size - priv->handle_pos.width);

          if (ctk_widget_get_direction (CTK_WIDGET (widget)) == CTK_TEXT_DIR_RTL)
            {
              flip_child (allocation, &(window2_allocation));
              flip_child (allocation, &(window1_allocation));
              flip_child (allocation, &(priv->handle_pos));
            }

          child1_allocation.x = child1_allocation.y = 0;
          child1_allocation.width = window1_allocation.width;
          child1_allocation.height = window1_allocation.height;
          if (child1_width > child1_allocation.width)
            {
              if (ctk_widget_get_direction (CTK_WIDGET (widget)) == CTK_TEXT_DIR_LTR)
                child1_allocation.x -= child1_width - child1_allocation.width;
              child1_allocation.width = child1_width;
            }

          child2_allocation.x = child2_allocation.y = 0;
          child2_allocation.width = window2_allocation.width;
          child2_allocation.height = window2_allocation.height;
          if (child2_width > child2_allocation.width)
            {
              if (ctk_widget_get_direction (CTK_WIDGET (widget)) == CTK_TEXT_DIR_RTL)
                child2_allocation.x -= child2_width - child2_allocation.width;
              child2_allocation.width = child2_width;
            }
        }
      else
        {
          gint child1_height, child2_height;

          ctk_widget_get_preferred_height_for_width (priv->child1,
                                                     allocation->width,
                                                     &child1_height, NULL);
          ctk_widget_get_preferred_height_for_width (priv->child2,
                                                     allocation->width,
                                                     &child2_height, NULL);

          ctk_paned_calc_position (paned,
                                   MAX (1, allocation->height - handle_size),
                                   child1_height,
                                   child2_height);

          priv->handle_pos.x = allocation->x;
          priv->handle_pos.y = allocation->y + priv->child1_size;
          priv->handle_pos.width = allocation->width;
          priv->handle_pos.height = handle_size;

          window1_allocation.width = window2_allocation.width = allocation->width;
          window1_allocation.height = MAX (1, priv->child1_size);
          window1_allocation.x = window2_allocation.x = allocation->x;
          window1_allocation.y = allocation->y;

          window2_allocation.y = window1_allocation.y + priv->child1_size + priv->handle_pos.height;
          window2_allocation.height = MAX (1, allocation->y + allocation->height - window2_allocation.y);

          child1_allocation.x = child1_allocation.y = 0;
          child1_allocation.width = window1_allocation.width;
          child1_allocation.height = window1_allocation.height;
          if (child1_height > child1_allocation.height)
            {
              child1_allocation.y -= child1_height - child1_allocation.height;
              child1_allocation.height = child1_height;
            }

          child2_allocation.x = child2_allocation.y = 0;
          child2_allocation.width = window2_allocation.width;
          child2_allocation.height = window2_allocation.height;
          if (child2_height > child2_allocation.height)
            child2_allocation.height = child2_height;
        }

      ctk_css_gadget_allocate (priv->handle_gadget,
                               &priv->handle_pos,
                               -1,
                               &clip);

      if (ctk_widget_get_mapped (widget) &&
          (old_handle_pos.x != priv->handle_pos.x ||
           old_handle_pos.y != priv->handle_pos.y ||
           old_handle_pos.width != priv->handle_pos.width ||
           old_handle_pos.height != priv->handle_pos.height))
        {
          GdkWindow *window;

          window = ctk_widget_get_window (widget);
          gdk_window_invalidate_rect (window, &old_handle_pos, FALSE);
          gdk_window_invalidate_rect (window, &priv->handle_pos, FALSE);
        }

      if (ctk_widget_get_realized (widget))
	{
          CtkAllocation border_alloc;

	  if (ctk_widget_get_mapped (widget))
	    gdk_window_show (priv->handle);

          ctk_css_gadget_get_border_allocation (priv->handle_gadget, &border_alloc, NULL);
          gdk_window_move_resize (priv->handle,
                                  border_alloc.x, border_alloc.y,
                                  border_alloc.width, border_alloc.height);
	}

      /* Now allocate the childen, making sure, when resizing not to
       * overlap the windows
       */
      ctk_widget_get_allocation (priv->child1, &priv_child1_allocation);
      if (ctk_widget_get_mapped (widget) &&
          ((priv->orientation == CTK_ORIENTATION_HORIZONTAL &&
            priv_child1_allocation.width < child1_allocation.width) ||

           (priv->orientation == CTK_ORIENTATION_VERTICAL &&
            priv_child1_allocation.height < child1_allocation.height)))
	{
          ctk_paned_child_allocate (priv->child2,
                                    priv->child2_window,
                                    &window2_allocation,
                                    &child2_allocation);
          ctk_paned_child_allocate (priv->child1,
                                    priv->child1_window,
                                    &window1_allocation,
                                    &child1_allocation);
	}
      else
	{
          ctk_paned_child_allocate (priv->child1,
                                    priv->child1_window,
                                    &window1_allocation,
                                    &child1_allocation);
          ctk_paned_child_allocate (priv->child2,
                                    priv->child2_window,
                                    &window2_allocation,
                                    &child2_allocation);
	}
    }
  else
    {
      CtkAllocation window_allocation, child_allocation;

      if (ctk_widget_get_realized (widget))
	gdk_window_hide (priv->handle);

      window_allocation.x = allocation->x;
      window_allocation.y = allocation->y;
      window_allocation.width = allocation->width;
      window_allocation.height = allocation->height;
      child_allocation.x = child_allocation.y = 0;
      child_allocation.width = allocation->width;
      child_allocation.height = allocation->height;

      if (priv->child1 && ctk_widget_get_visible (priv->child1))
        {
          ctk_paned_set_child_visible (paned, CHILD1, TRUE);
          ctk_paned_set_child_visible (paned, CHILD2, FALSE);

          ctk_paned_child_allocate (priv->child1,
                                    priv->child1_window,
                                    &window_allocation,
                                    &child_allocation);
        }
      else if (priv->child2 && ctk_widget_get_visible (priv->child2))
        {
          ctk_paned_set_child_visible (paned, CHILD1, FALSE);
          ctk_paned_set_child_visible (paned, CHILD2, TRUE);

          ctk_paned_child_allocate (priv->child2,
                                    priv->child2_window,
                                    &window_allocation,
                                    &child_allocation);
        }
      else
        {
          ctk_paned_set_child_visible (paned, CHILD1, FALSE);
          ctk_paned_set_child_visible (paned, CHILD2, FALSE);
        }
    }

  ctk_container_get_children_clip (CTK_CONTAINER (paned), out_clip);
  gdk_rectangle_union (out_clip, &clip, out_clip);
}

static GdkWindow *
ctk_paned_create_child_window (CtkPaned  *paned,
                               CtkWidget *child) /* may be NULL */
{
  CtkWidget *widget = CTK_WIDGET (paned);
  CtkPanedPrivate *priv = paned->priv;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = ctk_widget_get_events (widget);
  attributes.visual = ctk_widget_get_visual (widget);
  if (child)
    {
      CtkAllocation allocation;
      int handle_size;

      ctk_css_gadget_get_preferred_size (priv->handle_gadget,
                                         priv->orientation,
                                         -1,
                                         NULL, &handle_size,
                                         NULL, NULL);

      ctk_css_gadget_get_content_allocation (priv->gadget, &allocation, NULL);
      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL &&
          child == priv->child2 && priv->child1 &&
          ctk_widget_get_visible (priv->child1))
        attributes.x = priv->handle_pos.x + handle_size;
      else
        attributes.x = allocation.x;
      if (priv->orientation == CTK_ORIENTATION_VERTICAL &&
          child == priv->child2 && priv->child1 &&
          ctk_widget_get_visible (priv->child1))
        attributes.y = priv->handle_pos.y + handle_size;
      else
        attributes.y = allocation.y;

      ctk_widget_get_allocation (child, &allocation);
      attributes.width = allocation.width;
      attributes.height = allocation.height;
      attributes_mask = GDK_WA_X | GDK_WA_Y| GDK_WA_VISUAL;
    }
  else
    {
      attributes.width = 1;
      attributes.height = 1;
      attributes_mask = GDK_WA_VISUAL;
    }

  window = gdk_window_new (ctk_widget_get_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_register_window (widget, window);

  if (child)
    ctk_widget_set_parent_window (child, window);

  return window;
}

static void
ctk_paned_realize (CtkWidget *widget)
{
  CtkPaned *paned = CTK_PANED (widget);
  CtkPanedPrivate *priv = paned->priv;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;

  ctk_widget_set_realized (widget, TRUE);

  window = ctk_widget_get_parent_window (widget);
  ctk_widget_set_window (widget, window);
  g_object_ref (window);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.x = priv->handle_pos.x;
  attributes.y = priv->handle_pos.y;
  attributes.width = priv->handle_pos.width;
  attributes.height = priv->handle_pos.height;
  attributes.event_mask = ctk_widget_get_events (widget);
  attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK |
			    GDK_ENTER_NOTIFY_MASK |
			    GDK_LEAVE_NOTIFY_MASK |
			    GDK_POINTER_MOTION_MASK);
  attributes.cursor = NULL;
  attributes_mask = GDK_WA_X | GDK_WA_Y;
  if (ctk_widget_is_sensitive (widget))
    {
      attributes.cursor = gdk_cursor_new_from_name (ctk_widget_get_display (widget),
						    priv->orientation == CTK_ORIENTATION_HORIZONTAL
                                                    ? "col-resize" : "row-resize");
      attributes_mask |= GDK_WA_CURSOR;
    }

  priv->handle = gdk_window_new (window,
                                 &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->handle);
  g_clear_object (&attributes.cursor);

  priv->child1_window = ctk_paned_create_child_window (paned, priv->child1);
  priv->child2_window = ctk_paned_create_child_window (paned, priv->child2);
}

static void
ctk_paned_unrealize (CtkWidget *widget)
{
  CtkPaned *paned = CTK_PANED (widget);
  CtkPanedPrivate *priv = paned->priv;

  if (priv->child2)
    ctk_widget_set_parent_window (priv->child2, NULL);
  ctk_widget_unregister_window (widget, priv->child2_window);
  gdk_window_destroy (priv->child2_window);
  priv->child2_window = NULL;

  if (priv->child1)
    ctk_widget_set_parent_window (priv->child1, NULL);
  ctk_widget_unregister_window (widget, priv->child1_window);
  gdk_window_destroy (priv->child1_window);
  priv->child1_window = NULL;

  if (priv->handle)
    {
      ctk_widget_unregister_window (widget, priv->handle);
      gdk_window_destroy (priv->handle);
      priv->handle = NULL;
    }

  ctk_paned_set_last_child1_focus (paned, NULL);
  ctk_paned_set_last_child2_focus (paned, NULL);
  ctk_paned_set_saved_focus (paned, NULL);
  ctk_paned_set_first_paned (paned, NULL);

  CTK_WIDGET_CLASS (ctk_paned_parent_class)->unrealize (widget);
}

static void
ctk_paned_map (CtkWidget *widget)
{
  CtkPaned *paned = CTK_PANED (widget);
  CtkPanedPrivate *priv = paned->priv;

  if (priv->child1 && ctk_widget_get_visible (priv->child1) && ctk_widget_get_child_visible (priv->child1))
    gdk_window_show (priv->child1_window);
  if (priv->child2 && ctk_widget_get_visible (priv->child2) && ctk_widget_get_child_visible (priv->child2))
    gdk_window_show (priv->child2_window);

  if (priv->child1 && ctk_widget_get_visible (priv->child1) &&
      priv->child2 && ctk_widget_get_visible (priv->child2))
    gdk_window_show (priv->handle);

  CTK_WIDGET_CLASS (ctk_paned_parent_class)->map (widget);
}

static void
ctk_paned_unmap (CtkWidget *widget)
{
  CtkPaned *paned = CTK_PANED (widget);
  CtkPanedPrivate *priv = paned->priv;

  gdk_window_hide (priv->handle);
  
  if (gdk_window_is_visible (priv->child1_window))
    gdk_window_hide (priv->child1_window);
  if (gdk_window_is_visible (priv->child2_window))
    gdk_window_hide (priv->child2_window);

  CTK_WIDGET_CLASS (ctk_paned_parent_class)->unmap (widget);
}

static gboolean
ctk_paned_draw (CtkWidget *widget,
                cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_PANED (widget)->priv->gadget, cr);

  return FALSE;
}

static gboolean
ctk_paned_render (CtkCssGadget *gadget,
                  cairo_t      *cr,
                  int           x,
                  int           y,
                  int           width,
                  int           height,
                  gpointer      data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkPaned *paned = CTK_PANED (widget);
  CtkPanedPrivate *priv = paned->priv;
  CtkAllocation widget_allocation;
  int window_x, window_y;

  ctk_widget_get_allocation (widget, &widget_allocation);
  if (ctk_cairo_should_draw_window (cr, ctk_widget_get_window (widget)) &&
      priv->child1 && ctk_widget_get_visible (priv->child1) &&
      priv->child2 && ctk_widget_get_visible (priv->child2))
    ctk_css_gadget_draw (priv->handle_gadget, cr);

  if (priv->child1 && ctk_widget_get_visible (priv->child1))
    {
      gdk_window_get_position (priv->child1_window, &window_x, &window_y);
      cairo_save (cr);
      cairo_rectangle (cr, 
                       window_x - widget_allocation.x,
                       window_y - widget_allocation.y,
                       gdk_window_get_width (priv->child1_window),
                       gdk_window_get_height (priv->child1_window));
      cairo_clip (cr);
      ctk_container_propagate_draw (CTK_CONTAINER (widget), priv->child1, cr);
      cairo_restore (cr);
    }

  if (priv->child2 && ctk_widget_get_visible (priv->child2))
    {
      gdk_window_get_position (priv->child2_window, &window_x, &window_y);
      cairo_save (cr);
      cairo_rectangle (cr, 
                       window_x - widget_allocation.x,
                       window_y - widget_allocation.y,
                       gdk_window_get_width (priv->child2_window),
                       gdk_window_get_height (priv->child2_window));
      cairo_clip (cr);
      ctk_container_propagate_draw (CTK_CONTAINER (widget), priv->child2, cr);
      cairo_restore (cr);
    }

  return FALSE;
}

static gboolean
ctk_paned_render_handle (CtkCssGadget *gadget,
                         cairo_t      *cr,
                         int           x,
                         int           y,
                         int           width,
                         int           height,
                         gpointer      data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkPaned *paned = CTK_PANED (widget);
  CtkPanedPrivate *priv = paned->priv;
  CtkStyleContext *context = ctk_widget_get_style_context (widget);

  ctk_style_context_save_to_node (context, ctk_css_gadget_get_node (priv->handle_gadget));
  ctk_render_handle (context, cr, x, y, width, height);
  ctk_style_context_restore (context);

  return FALSE;
}

static void
update_node_state (CtkWidget *widget)
{
  CtkPaned *paned = CTK_PANED (widget);
  CtkPanedPrivate *priv = paned->priv;
  CtkStateFlags state;

  state = ctk_widget_get_state_flags (widget);

  if (ctk_widget_is_focus (widget))
    state |= CTK_STATE_FLAG_SELECTED;
  if (priv->handle_prelit)
    state |= CTK_STATE_FLAG_PRELIGHT;

  ctk_css_node_set_state (ctk_css_gadget_get_node (priv->handle_gadget), state);
}

static void
connect_drag_gesture_signals (CtkPaned   *paned,
                              CtkGesture *gesture)
{
  g_signal_connect (gesture, "drag-begin",
                    G_CALLBACK (gesture_drag_begin_cb), paned);
  g_signal_connect (gesture, "drag-update",
                    G_CALLBACK (gesture_drag_update_cb), paned);
  g_signal_connect (gesture, "drag-end",
                    G_CALLBACK (gesture_drag_end_cb), paned);
}

static void
ctk_paned_init (CtkPaned *paned)
{
  CtkPanedPrivate *priv;
  CtkGesture *gesture;
  CtkCssNode *widget_node;

  ctk_widget_set_has_window (CTK_WIDGET (paned), FALSE);
  ctk_widget_set_can_focus (CTK_WIDGET (paned), TRUE);

  paned->priv = ctk_paned_get_instance_private (paned);
  priv = paned->priv;

  priv->orientation = CTK_ORIENTATION_HORIZONTAL;

  priv->child1 = NULL;
  priv->child2 = NULL;
  priv->handle = NULL;

  priv->handle_pos.width = 5;
  priv->handle_pos.height = 5;
  priv->position_set = FALSE;
  priv->last_allocation = -1;

  priv->last_child1_focus = NULL;
  priv->last_child2_focus = NULL;
  priv->in_recursion = FALSE;
  priv->handle_prelit = FALSE;
  priv->original_position = -1;
  priv->max_position = G_MAXINT;

  priv->handle_pos.x = -1;
  priv->handle_pos.y = -1;

  _ctk_orientable_set_style_classes (CTK_ORIENTABLE (paned));

  /* Touch gesture */
  gesture = ctk_gesture_pan_new (CTK_WIDGET (paned),
                                 CTK_ORIENTATION_HORIZONTAL);
  connect_drag_gesture_signals (paned, gesture);
  ctk_gesture_single_set_touch_only (CTK_GESTURE_SINGLE (gesture), TRUE);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (gesture),
                                              CTK_PHASE_CAPTURE);
  priv->pan_gesture = gesture;

  /* Pointer gesture */
  gesture = ctk_gesture_drag_new (CTK_WIDGET (paned));
  connect_drag_gesture_signals (paned, gesture);
  priv->drag_gesture = gesture;

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (paned));
  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     CTK_WIDGET (paned),
                                                     ctk_paned_measure,
                                                     ctk_paned_allocate,
                                                     ctk_paned_render,
                                                     NULL,
                                                     NULL);
  priv->handle_gadget = ctk_css_custom_gadget_new ("separator",
                                                   CTK_WIDGET (paned),
                                                   priv->gadget,
                                                   NULL,
                                                   ctk_paned_measure_handle,
                                                   NULL,
                                                   ctk_paned_render_handle,
                                                   NULL,
                                                   NULL);
  update_node_state (CTK_WIDGET (paned));
}

static gboolean
is_rtl (CtkPaned *paned)
{
  CtkPanedPrivate *priv = paned->priv;

  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL &&
      ctk_widget_get_direction (CTK_WIDGET (paned)) == CTK_TEXT_DIR_RTL)
    {
      return TRUE;
    }

  return FALSE;
}

static void
update_drag (CtkPaned *paned,
             int       xpos,
             int       ypos)
{
  CtkPanedPrivate *priv = paned->priv;
  CtkAllocation allocation;
  gint pos;
  gint handle_size;
  gint size;
  gint x, y;

  gdk_window_get_position (priv->handle, &x, &y);
  ctk_css_gadget_get_content_allocation (priv->gadget, &allocation, NULL);
  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    pos = xpos;
  else
    pos = ypos;

  pos -= priv->drag_pos;

  if (is_rtl (paned))
    {
      ctk_css_gadget_get_preferred_size (priv->handle_gadget,
                                         CTK_ORIENTATION_HORIZONTAL,
                                         -1,
                                         NULL, &handle_size,
                                         NULL, NULL);

      size = allocation.width - pos - handle_size;
    }
  else
    {
      size = pos;
    }

  size = CLAMP (size, priv->min_position, priv->max_position);

  if (size != priv->child1_size)
    ctk_paned_set_position (paned, size);
}

static gboolean
ctk_paned_enter (CtkWidget        *widget,
		 GdkEventCrossing *event)
{
  CtkPaned *paned = CTK_PANED (widget);
  CtkPanedPrivate *priv = paned->priv;

  if (!ctk_gesture_is_active (priv->pan_gesture))
    {
      priv->handle_prelit = TRUE;
      update_node_state (widget);
      ctk_widget_queue_draw_area (widget,
				  priv->handle_pos.x,
				  priv->handle_pos.y,
				  priv->handle_pos.width,
				  priv->handle_pos.height);
    }
  
  return TRUE;
}

static gboolean
ctk_paned_leave (CtkWidget        *widget,
		 GdkEventCrossing *event)
{
  CtkPaned *paned = CTK_PANED (widget);
  CtkPanedPrivate *priv = paned->priv;

  if (!ctk_gesture_is_active (priv->pan_gesture))
    {
      priv->handle_prelit = FALSE;
      update_node_state (widget);
      ctk_widget_queue_draw_area (widget,
				  priv->handle_pos.x,
				  priv->handle_pos.y,
				  priv->handle_pos.width,
				  priv->handle_pos.height);
    }

  return TRUE;
}

static gboolean
ctk_paned_focus (CtkWidget        *widget,
		 CtkDirectionType  direction)

{
  gboolean retval;
  
  /* This is a hack, but how can this be done without
   * excessive cut-and-paste from ctkcontainer.c?
   */

  ctk_widget_set_can_focus (widget, FALSE);
  retval = CTK_WIDGET_CLASS (ctk_paned_parent_class)->focus (widget, direction);
  ctk_widget_set_can_focus (widget, TRUE);

  return retval;
}

static void
ctk_paned_state_flags_changed (CtkWidget     *widget,
                               CtkStateFlags  previous_state)
{
  CtkPaned *paned = CTK_PANED (widget);
  CtkPanedPrivate *priv = paned->priv;
  GdkCursor *cursor;

  if (ctk_widget_get_realized (widget))
    {
      if (ctk_widget_is_sensitive (widget))
        cursor = gdk_cursor_new_from_name (ctk_widget_get_display (widget),
                                           priv->orientation == CTK_ORIENTATION_HORIZONTAL
                                           ? "col-resize" : "row-resize");
      else
        cursor = NULL;

      if (priv->handle)
        gdk_window_set_cursor (priv->handle, cursor);

      if (cursor)
        g_object_unref (cursor);
    }

  update_node_state (widget);

  CTK_WIDGET_CLASS (ctk_paned_parent_class)->state_flags_changed (widget, previous_state);
}

static void
ctk_paned_direction_changed (CtkWidget        *widget,
                             CtkTextDirection  previous_direction)
{
  CtkPaned *paned = CTK_PANED (widget);

  if (paned->priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    ctk_css_node_reverse_children (ctk_widget_get_css_node (widget));
}

/**
 * ctk_paned_new:
 * @orientation: the paned’s orientation.
 *
 * Creates a new #CtkPaned widget.
 *
 * Returns: a new #CtkPaned.
 *
 * Since: 3.0
 **/
CtkWidget *
ctk_paned_new (CtkOrientation orientation)
{
  return g_object_new (CTK_TYPE_PANED,
                       "orientation", orientation,
                       NULL);
}

/**
 * ctk_paned_add1:
 * @paned: a paned widget
 * @child: the child to add
 *
 * Adds a child to the top or left pane with default parameters. This is
 * equivalent to
 * `ctk_paned_pack1 (paned, child, FALSE, TRUE)`.
 */
void
ctk_paned_add1 (CtkPaned  *paned,
		CtkWidget *widget)
{
  ctk_paned_pack1 (paned, widget, FALSE, TRUE);
}

/**
 * ctk_paned_add2:
 * @paned: a paned widget
 * @child: the child to add
 *
 * Adds a child to the bottom or right pane with default parameters. This
 * is equivalent to
 * `ctk_paned_pack2 (paned, child, TRUE, TRUE)`.
 */
void
ctk_paned_add2 (CtkPaned  *paned,
		CtkWidget *widget)
{
  ctk_paned_pack2 (paned, widget, TRUE, TRUE);
}

/**
 * ctk_paned_pack1:
 * @paned: a paned widget
 * @child: the child to add
 * @resize: should this child expand when the paned widget is resized.
 * @shrink: can this child be made smaller than its requisition.
 *
 * Adds a child to the top or left pane.
 */
void
ctk_paned_pack1 (CtkPaned  *paned,
		 CtkWidget *child,
		 gboolean   resize,
		 gboolean   shrink)
{
  CtkPanedPrivate *priv;
  CtkCssNode *widget_node;
  CtkCssNode *child_node;

  g_return_if_fail (CTK_IS_PANED (paned));
  g_return_if_fail (CTK_IS_WIDGET (child));

  priv = paned->priv;

  if (!priv->child1)
    {
      priv->child1 = child;
      priv->child1_resize = resize;
      priv->child1_shrink = shrink;

      widget_node = ctk_widget_get_css_node (CTK_WIDGET (paned));
      child_node = ctk_widget_get_css_node (child);
      if (ctk_widget_get_direction (CTK_WIDGET (paned)) == CTK_TEXT_DIR_RTL)
        ctk_css_node_insert_after (widget_node, child_node, ctk_css_gadget_get_node (priv->handle_gadget));
      else
        ctk_css_node_insert_before (widget_node, child_node, ctk_css_gadget_get_node (priv->handle_gadget));

      ctk_widget_set_parent_window (child, priv->child1_window);
      ctk_widget_set_parent (child, CTK_WIDGET (paned));
    }
}

/**
 * ctk_paned_pack2:
 * @paned: a paned widget
 * @child: the child to add
 * @resize: should this child expand when the paned widget is resized.
 * @shrink: can this child be made smaller than its requisition.
 *
 * Adds a child to the bottom or right pane.
 */
void
ctk_paned_pack2 (CtkPaned  *paned,
		 CtkWidget *child,
		 gboolean   resize,
		 gboolean   shrink)
{
  CtkPanedPrivate *priv;
  CtkCssNode *widget_node;
  CtkCssNode *child_node;

  g_return_if_fail (CTK_IS_PANED (paned));
  g_return_if_fail (CTK_IS_WIDGET (child));

  priv = paned->priv;

  if (!priv->child2)
    {
      priv->child2 = child;
      priv->child2_resize = resize;
      priv->child2_shrink = shrink;

      widget_node = ctk_widget_get_css_node (CTK_WIDGET (paned));
      child_node = ctk_widget_get_css_node (child);
      if (ctk_widget_get_direction (CTK_WIDGET (paned)) == CTK_TEXT_DIR_RTL)
        ctk_css_node_insert_before (widget_node, child_node, ctk_css_gadget_get_node (priv->handle_gadget));
      else
        ctk_css_node_insert_after (widget_node, child_node, ctk_css_gadget_get_node (priv->handle_gadget));

      ctk_widget_set_parent_window (child, priv->child2_window);
      ctk_widget_set_parent (child, CTK_WIDGET (paned));
    }
}


static void
ctk_paned_add (CtkContainer *container,
	       CtkWidget    *widget)
{
  CtkPanedPrivate *priv;
  CtkPaned *paned;

  g_return_if_fail (CTK_IS_PANED (container));

  paned = CTK_PANED (container);
  priv = paned->priv;

  if (!priv->child1)
    ctk_paned_add1 (paned, widget);
  else if (!priv->child2)
    ctk_paned_add2 (paned, widget);
  else
    g_warning ("CtkPaned cannot have more than 2 children");
}

static void
ctk_paned_remove (CtkContainer *container,
		  CtkWidget    *widget)
{
  CtkPaned *paned = CTK_PANED (container);
  CtkPanedPrivate *priv = paned->priv;
  gboolean was_visible;

  was_visible = ctk_widget_get_visible (widget);

  if (priv->child1 == widget)
    {
      if (priv->child1_window && gdk_window_is_visible (priv->child1_window))
        gdk_window_hide (priv->child1_window);

      ctk_widget_unparent (widget);

      priv->child1 = NULL;

      if (was_visible && ctk_widget_get_visible (CTK_WIDGET (container)))
	ctk_widget_queue_resize_no_redraw (CTK_WIDGET (container));
    }
  else if (priv->child2 == widget)
    {
      if (priv->child2_window && gdk_window_is_visible (priv->child2_window))
        gdk_window_hide (priv->child2_window);

      ctk_widget_unparent (widget);

      priv->child2 = NULL;

      if (was_visible && ctk_widget_get_visible (CTK_WIDGET (container)))
	ctk_widget_queue_resize_no_redraw (CTK_WIDGET (container));
    }
}

static void
ctk_paned_forall (CtkContainer *container,
		  gboolean      include_internals,
		  CtkCallback   callback,
		  gpointer      callback_data)
{
  CtkPanedPrivate *priv;
  CtkPaned *paned;

  g_return_if_fail (callback != NULL);

  paned = CTK_PANED (container);
  priv = paned->priv;

  if (priv->child1)
    (*callback) (priv->child1, callback_data);
  if (priv->child2)
    (*callback) (priv->child2, callback_data);
}

/**
 * ctk_paned_get_position:
 * @paned: a #CtkPaned widget
 * 
 * Obtains the position of the divider between the two panes.
 * 
 * Returns: position of the divider
 **/
gint
ctk_paned_get_position (CtkPaned  *paned)
{
  g_return_val_if_fail (CTK_IS_PANED (paned), 0);

  return paned->priv->child1_size;
}

/**
 * ctk_paned_set_position:
 * @paned: a #CtkPaned widget
 * @position: pixel position of divider, a negative value means that the position
 *            is unset.
 * 
 * Sets the position of the divider between the two panes.
 **/
void
ctk_paned_set_position (CtkPaned *paned,
			gint      position)
{
  CtkPanedPrivate *priv;

  g_return_if_fail (CTK_IS_PANED (paned));

  priv = paned->priv;

  g_object_freeze_notify (G_OBJECT (paned));

  if (position >= 0)
    {
      /* We don't clamp here - the assumption is that
       * if the total allocation changes at the same time
       * as the position, the position set is with reference
       * to the new total size. If only the position changes,
       * then clamping will occur in ctk_paned_calc_position()
       */

      if (!priv->position_set)
        g_object_notify (G_OBJECT (paned), "position-set");

      if (priv->child1_size != position)
        {
          g_object_notify (G_OBJECT (paned), "position");
          ctk_widget_queue_resize_no_redraw (CTK_WIDGET (paned));
        }

      priv->child1_size = position;
      priv->position_set = TRUE;
    }
  else
    {
      if (priv->position_set)
        g_object_notify (G_OBJECT (paned), "position-set");

      priv->position_set = FALSE;
    }

  g_object_thaw_notify (G_OBJECT (paned));

#ifdef G_OS_WIN32
  /* Hacky work-around for bug #144269 */
  if (priv->child2 != NULL)
    {
      ctk_widget_queue_draw (priv->child2);
    }
#endif
}

/**
 * ctk_paned_get_child1:
 * @paned: a #CtkPaned widget
 * 
 * Obtains the first child of the paned widget.
 * 
 * Returns: (nullable) (transfer none): first child, or %NULL if it is not set.
 *
 * Since: 2.4
 **/
CtkWidget *
ctk_paned_get_child1 (CtkPaned *paned)
{
  g_return_val_if_fail (CTK_IS_PANED (paned), NULL);

  return paned->priv->child1;
}

/**
 * ctk_paned_get_child2:
 * @paned: a #CtkPaned widget
 * 
 * Obtains the second child of the paned widget.
 * 
 * Returns: (nullable) (transfer none): second child, or %NULL if it is not set.
 *
 * Since: 2.4
 **/
CtkWidget *
ctk_paned_get_child2 (CtkPaned *paned)
{
  g_return_val_if_fail (CTK_IS_PANED (paned), NULL);

  return paned->priv->child2;
}

static void
ctk_paned_calc_position (CtkPaned *paned,
                         gint      allocation,
                         gint      child1_req,
                         gint      child2_req)
{
  CtkPanedPrivate *priv = paned->priv;
  gint old_position;
  gint old_min_position;
  gint old_max_position;

  old_position = priv->child1_size;
  old_min_position = priv->min_position;
  old_max_position = priv->max_position;

  ctk_paned_compute_position (paned,
                              allocation, child1_req, child2_req,
                              &priv->min_position, &priv->max_position,
                              &priv->child1_size);

  ctk_paned_set_child_visible (paned, CHILD1, priv->child1_size != 0);
  ctk_paned_set_child_visible (paned, CHILD2, priv->child1_size != allocation);

  g_object_freeze_notify (G_OBJECT (paned));
  if (priv->child1_size != old_position)
    g_object_notify (G_OBJECT (paned), "position");
  if (priv->min_position != old_min_position)
    g_object_notify (G_OBJECT (paned), "min-position");
  if (priv->max_position != old_max_position)
    g_object_notify (G_OBJECT (paned), "max-position");
  g_object_thaw_notify (G_OBJECT (paned));

  priv->last_allocation = allocation;
}

static void
ctk_paned_set_saved_focus (CtkPaned *paned, CtkWidget *widget)
{
  CtkPanedPrivate *priv = paned->priv;

  if (priv->saved_focus)
    g_object_remove_weak_pointer (G_OBJECT (priv->saved_focus),
				  (gpointer *)&(priv->saved_focus));

  priv->saved_focus = widget;

  if (priv->saved_focus)
    g_object_add_weak_pointer (G_OBJECT (priv->saved_focus),
			       (gpointer *)&(priv->saved_focus));
}

static void
ctk_paned_set_first_paned (CtkPaned *paned, CtkPaned *first_paned)
{
  CtkPanedPrivate *priv = paned->priv;

  if (priv->first_paned)
    g_object_remove_weak_pointer (G_OBJECT (priv->first_paned),
				  (gpointer *)&(priv->first_paned));

  priv->first_paned = first_paned;

  if (priv->first_paned)
    g_object_add_weak_pointer (G_OBJECT (priv->first_paned),
			       (gpointer *)&(priv->first_paned));
}

static void
ctk_paned_set_last_child1_focus (CtkPaned *paned, CtkWidget *widget)
{
  CtkPanedPrivate *priv = paned->priv;

  if (priv->last_child1_focus)
    g_object_remove_weak_pointer (G_OBJECT (priv->last_child1_focus),
				  (gpointer *)&(priv->last_child1_focus));

  priv->last_child1_focus = widget;

  if (priv->last_child1_focus)
    g_object_add_weak_pointer (G_OBJECT (priv->last_child1_focus),
			       (gpointer *)&(priv->last_child1_focus));
}

static void
ctk_paned_set_last_child2_focus (CtkPaned *paned, CtkWidget *widget)
{
  CtkPanedPrivate *priv = paned->priv;

  if (priv->last_child2_focus)
    g_object_remove_weak_pointer (G_OBJECT (priv->last_child2_focus),
				  (gpointer *)&(priv->last_child2_focus));

  priv->last_child2_focus = widget;

  if (priv->last_child2_focus)
    g_object_add_weak_pointer (G_OBJECT (priv->last_child2_focus),
			       (gpointer *)&(priv->last_child2_focus));
}

static CtkWidget *
paned_get_focus_widget (CtkPaned *paned)
{
  CtkWidget *toplevel;

  toplevel = ctk_widget_get_toplevel (CTK_WIDGET (paned));
  if (ctk_widget_is_toplevel (toplevel))
    return ctk_window_get_focus (CTK_WINDOW (toplevel));

  return NULL;
}

static void
ctk_paned_set_focus_child (CtkContainer *container,
			   CtkWidget    *focus_child)
{
  CtkPaned *paned;
  CtkPanedPrivate *priv;
  CtkWidget *container_focus_child;

  g_return_if_fail (CTK_IS_PANED (container));

  paned = CTK_PANED (container);
  priv = paned->priv;

  if (focus_child == NULL)
    {
      CtkWidget *last_focus;
      CtkWidget *w;
      
      last_focus = paned_get_focus_widget (paned);

      if (last_focus)
	{
	  /* If there is one or more paned widgets between us and the
	   * focus widget, we want the topmost of those as last_focus
	   */
	  for (w = last_focus; w != CTK_WIDGET (paned); w = ctk_widget_get_parent (w))
	    if (CTK_IS_PANED (w))
	      last_focus = w;

          container_focus_child = ctk_container_get_focus_child (container);
          if (container_focus_child == priv->child1)
	    ctk_paned_set_last_child1_focus (paned, last_focus);
	  else if (container_focus_child == priv->child2)
	    ctk_paned_set_last_child2_focus (paned, last_focus);
	}
    }

  if (CTK_CONTAINER_CLASS (ctk_paned_parent_class)->set_focus_child)
    CTK_CONTAINER_CLASS (ctk_paned_parent_class)->set_focus_child (container, focus_child);
}

static void
ctk_paned_get_cycle_chain (CtkPaned          *paned,
			   CtkDirectionType   direction,
			   GList            **widgets)
{
  CtkPanedPrivate *priv = paned->priv;
  CtkContainer *container = CTK_CONTAINER (paned);
  CtkWidget *ancestor = NULL;
  CtkWidget *focus_child;
  CtkWidget *parent;
  CtkWidget *widget = CTK_WIDGET (paned);
  GList *temp_list = NULL;
  GList *list;

  if (priv->in_recursion)
    return;

  g_assert (widgets != NULL);

  if (priv->last_child1_focus &&
      !ctk_widget_is_ancestor (priv->last_child1_focus, widget))
    {
      ctk_paned_set_last_child1_focus (paned, NULL);
    }

  if (priv->last_child2_focus &&
      !ctk_widget_is_ancestor (priv->last_child2_focus, widget))
    {
      ctk_paned_set_last_child2_focus (paned, NULL);
    }

  parent = ctk_widget_get_parent (widget);
  if (parent)
    ancestor = ctk_widget_get_ancestor (parent, CTK_TYPE_PANED);

  /* The idea here is that temp_list is a list of widgets we want to cycle
   * to. The list is prioritized so that the first element is our first
   * choice, the next our second, and so on.
   *
   * We can't just use g_list_reverse(), because we want to try
   * priv->last_child?_focus before priv->child?, both when we
   * are going forward and backward.
   */
  focus_child = ctk_container_get_focus_child (container);
  if (direction == CTK_DIR_TAB_FORWARD)
    {
      if (focus_child == priv->child1)
	{
	  temp_list = g_list_append (temp_list, priv->last_child2_focus);
	  temp_list = g_list_append (temp_list, priv->child2);
	  temp_list = g_list_append (temp_list, ancestor);
	}
      else if (focus_child == priv->child2)
	{
	  temp_list = g_list_append (temp_list, ancestor);
	  temp_list = g_list_append (temp_list, priv->last_child1_focus);
	  temp_list = g_list_append (temp_list, priv->child1);
	}
      else
	{
	  temp_list = g_list_append (temp_list, priv->last_child1_focus);
	  temp_list = g_list_append (temp_list, priv->child1);
	  temp_list = g_list_append (temp_list, priv->last_child2_focus);
	  temp_list = g_list_append (temp_list, priv->child2);
	  temp_list = g_list_append (temp_list, ancestor);
	}
    }
  else
    {
      if (focus_child == priv->child1)
	{
	  temp_list = g_list_append (temp_list, ancestor);
	  temp_list = g_list_append (temp_list, priv->last_child2_focus);
	  temp_list = g_list_append (temp_list, priv->child2);
	}
      else if (focus_child == priv->child2)
	{
	  temp_list = g_list_append (temp_list, priv->last_child1_focus);
	  temp_list = g_list_append (temp_list, priv->child1);
	  temp_list = g_list_append (temp_list, ancestor);
	}
      else
	{
	  temp_list = g_list_append (temp_list, priv->last_child2_focus);
	  temp_list = g_list_append (temp_list, priv->child2);
	  temp_list = g_list_append (temp_list, priv->last_child1_focus);
	  temp_list = g_list_append (temp_list, priv->child1);
	  temp_list = g_list_append (temp_list, ancestor);
	}
    }

  /* Walk the list and expand all the paned widgets. */
  for (list = temp_list; list != NULL; list = list->next)
    {
      widget = list->data;

      if (widget)
	{
	  if (CTK_IS_PANED (widget))
	    {
	      priv->in_recursion = TRUE;
	      ctk_paned_get_cycle_chain (CTK_PANED (widget), direction, widgets);
	      priv->in_recursion = FALSE;
	    }
	  else
	    {
	      *widgets = g_list_append (*widgets, widget);
	    }
	}
    }

  g_list_free (temp_list);
}

static gboolean
ctk_paned_cycle_child_focus (CtkPaned *paned,
			     gboolean  reversed)
{
  GList *cycle_chain = NULL;
  GList *list;
  
  CtkDirectionType direction = reversed? CTK_DIR_TAB_BACKWARD : CTK_DIR_TAB_FORWARD;

  /* ignore f6 if the handle is focused */
  if (ctk_widget_is_focus (CTK_WIDGET (paned)))
    return TRUE;
  
  /* we can't just let the event propagate up the hierarchy,
   * because the paned will want to cycle focus _unless_ an
   * ancestor paned handles the event
   */
  ctk_paned_get_cycle_chain (paned, direction, &cycle_chain);

  for (list = cycle_chain; list != NULL; list = list->next)
    if (ctk_widget_child_focus (CTK_WIDGET (list->data), direction))
      break;

  g_list_free (cycle_chain);
  
  return TRUE;
}

static void
get_child_panes (CtkWidget  *widget,
		 GList     **panes)
{
  if (!widget || !ctk_widget_get_realized (widget))
    return;

  if (CTK_IS_PANED (widget))
    {
      CtkPaned *paned = CTK_PANED (widget);
      CtkPanedPrivate *priv = paned->priv;

      get_child_panes (priv->child1, panes);
      *panes = g_list_prepend (*panes, widget);
      get_child_panes (priv->child2, panes);
    }
  else if (CTK_IS_CONTAINER (widget))
    {
      ctk_container_forall (CTK_CONTAINER (widget),
                            (CtkCallback)get_child_panes, panes);
    }
}

static GList *
get_all_panes (CtkPaned *paned)
{
  CtkPaned *topmost = NULL;
  GList *result = NULL;
  CtkWidget *w;

  for (w = CTK_WIDGET (paned); w != NULL; w = ctk_widget_get_parent (w))
    {
      if (CTK_IS_PANED (w))
	topmost = CTK_PANED (w);
    }

  g_assert (topmost);

  get_child_panes (CTK_WIDGET (topmost), &result);

  return g_list_reverse (result);
}

static void
ctk_paned_find_neighbours (CtkPaned  *paned,
			   CtkPaned **next,
			   CtkPaned **prev)
{
  GList *all_panes;
  GList *this_link;

  all_panes = get_all_panes (paned);
  g_assert (all_panes);

  this_link = g_list_find (all_panes, paned);

  g_assert (this_link);
  
  if (this_link->next)
    *next = this_link->next->data;
  else
    *next = all_panes->data;

  if (this_link->prev)
    *prev = this_link->prev->data;
  else
    *prev = g_list_last (all_panes)->data;

  g_list_free (all_panes);
}

static gboolean
ctk_paned_move_handle (CtkPaned      *paned,
		       CtkScrollType  scroll)
{
  CtkPanedPrivate *priv = paned->priv;

  if (ctk_widget_is_focus (CTK_WIDGET (paned)))
    {
      gint old_position;
      gint new_position;
      gint increment;
      
      enum {
	SINGLE_STEP_SIZE = 1,
	PAGE_STEP_SIZE   = 75
      };
      
      new_position = old_position = ctk_paned_get_position (paned);
      increment = 0;
      
      switch (scroll)
	{
	case CTK_SCROLL_STEP_LEFT:
	case CTK_SCROLL_STEP_UP:
	case CTK_SCROLL_STEP_BACKWARD:
	  increment = - SINGLE_STEP_SIZE;
	  break;
	  
	case CTK_SCROLL_STEP_RIGHT:
	case CTK_SCROLL_STEP_DOWN:
	case CTK_SCROLL_STEP_FORWARD:
	  increment = SINGLE_STEP_SIZE;
	  break;
	  
	case CTK_SCROLL_PAGE_LEFT:
	case CTK_SCROLL_PAGE_UP:
	case CTK_SCROLL_PAGE_BACKWARD:
	  increment = - PAGE_STEP_SIZE;
	  break;
	  
	case CTK_SCROLL_PAGE_RIGHT:
	case CTK_SCROLL_PAGE_DOWN:
	case CTK_SCROLL_PAGE_FORWARD:
	  increment = PAGE_STEP_SIZE;
	  break;
	  
	case CTK_SCROLL_START:
	  new_position = priv->min_position;
	  break;
	  
	case CTK_SCROLL_END:
	  new_position = priv->max_position;
	  break;

	default:
	  break;
	}

      if (increment)
	{
	  if (is_rtl (paned))
	    increment = -increment;
	  
	  new_position = old_position + increment;
	}
      
      new_position = CLAMP (new_position, priv->min_position, priv->max_position);
      
      if (old_position != new_position)
	ctk_paned_set_position (paned, new_position);

      return TRUE;
    }

  return FALSE;
}

static void
ctk_paned_restore_focus (CtkPaned *paned)
{
  CtkPanedPrivate *priv = paned->priv;

  if (ctk_widget_is_focus (CTK_WIDGET (paned)))
    {
      if (priv->saved_focus &&
	  ctk_widget_get_sensitive (priv->saved_focus))
	{
	  ctk_widget_grab_focus (priv->saved_focus);
	}
      else
	{
	  /* the saved focus is somehow not available for focusing,
	   * try
	   *   1) tabbing into the paned widget
	   * if that didn't work,
	   *   2) unset focus for the window if there is one
	   */
	  
	  if (!ctk_widget_child_focus (CTK_WIDGET (paned), CTK_DIR_TAB_FORWARD))
	    {
	      CtkWidget *toplevel = ctk_widget_get_toplevel (CTK_WIDGET (paned));
	      
	      if (CTK_IS_WINDOW (toplevel))
		ctk_window_set_focus (CTK_WINDOW (toplevel), NULL);
	    }
	}
      
      ctk_paned_set_saved_focus (paned, NULL);
      ctk_paned_set_first_paned (paned, NULL);
    }
}

static gboolean
ctk_paned_accept_position (CtkPaned *paned)
{
  CtkPanedPrivate *priv = paned->priv;

  if (ctk_widget_is_focus (CTK_WIDGET (paned)))
    {
      priv->original_position = -1;
      ctk_paned_restore_focus (paned);

      return TRUE;
    }

  return FALSE;
}


static gboolean
ctk_paned_cancel_position (CtkPaned *paned)
{
  CtkPanedPrivate *priv = paned->priv;

  if (ctk_widget_is_focus (CTK_WIDGET (paned)))
    {
      if (priv->original_position != -1)
	{
	  ctk_paned_set_position (paned, priv->original_position);
	  priv->original_position = -1;
	}

      ctk_paned_restore_focus (paned);
      return TRUE;
    }

  return FALSE;
}

static gboolean
ctk_paned_cycle_handle_focus (CtkPaned *paned,
			      gboolean  reversed)
{
  CtkPanedPrivate *priv = paned->priv;
  CtkPaned *next, *prev;

  if (ctk_widget_is_focus (CTK_WIDGET (paned)))
    {
      CtkPaned *focus = NULL;

      if (!priv->first_paned)
	{
	  /* The first_pane has disappeared. As an ad-hoc solution,
	   * we make the currently focused paned the first_paned. To the
	   * user this will seem like the paned cycling has been reset.
	   */
	  
	  ctk_paned_set_first_paned (paned, paned);
	}
      
      ctk_paned_find_neighbours (paned, &next, &prev);

      if (reversed && prev &&
	  prev != paned && paned != priv->first_paned)
	{
	  focus = prev;
	}
      else if (!reversed && next &&
	       next != paned && next != priv->first_paned)
	{
	  focus = next;
	}
      else
	{
	  ctk_paned_accept_position (paned);
	  return TRUE;
	}

      g_assert (focus);
      
      ctk_paned_set_saved_focus (focus, priv->saved_focus);
      ctk_paned_set_first_paned (focus, priv->first_paned);
      
      ctk_paned_set_saved_focus (paned, NULL);
      ctk_paned_set_first_paned (paned, NULL);
      
      ctk_widget_grab_focus (CTK_WIDGET (focus));
      
      if (!ctk_widget_is_focus (CTK_WIDGET (paned)))
	{
	  priv->original_position = -1;
	  focus->priv->original_position = ctk_paned_get_position (focus);
	}
    }
  else
    {
      CtkContainer *container = CTK_CONTAINER (paned);
      CtkPaned *focus;
      CtkPaned *first;
      CtkWidget *toplevel;
      CtkWidget *focus_child;

      ctk_paned_find_neighbours (paned, &next, &prev);
      focus_child = ctk_container_get_focus_child (container);

      if (focus_child == priv->child1)
	{
	  if (reversed)
	    {
	      focus = prev;
	      first = paned;
	    }
	  else
	    {
	      focus = paned;
	      first = paned;
	    }
	}
      else if (focus_child == priv->child2)
	{
	  if (reversed)
	    {
	      focus = paned;
	      first = next;
	    }
	  else
	    {
	      focus = next;
	      first = next;
	    }
	}
      else
	{
	  /* Focus is not inside this paned, and we don't have focus.
	   * Presumably this happened because the application wants us
	   * to start keyboard navigating.
	   */
	  focus = paned;

	  if (reversed)
	    first = paned;
	  else
	    first = next;
	}

      toplevel = ctk_widget_get_toplevel (CTK_WIDGET (paned));

      if (CTK_IS_WINDOW (toplevel))
        ctk_paned_set_saved_focus (focus, ctk_window_get_focus (CTK_WINDOW (toplevel)));
      ctk_paned_set_first_paned (focus, first);
      focus->priv->original_position = ctk_paned_get_position (focus);

      ctk_widget_grab_focus (CTK_WIDGET (focus));
   }

  return TRUE;
}

static gboolean
ctk_paned_toggle_handle_focus (CtkPaned *paned)
{
  /* This function/signal has the wrong name. It is called when you
   * press Tab or Shift-Tab and what we do is act as if
   * the user pressed Return and then Tab or Shift-Tab
   */
  if (ctk_widget_is_focus (CTK_WIDGET (paned)))
    ctk_paned_accept_position (paned);

  return FALSE;
}

/**
 * ctk_paned_get_handle_window:
 * @paned: a #CtkPaned
 *
 * Returns the #GdkWindow of the handle. This function is
 * useful when handling button or motion events because it
 * enables the callback to distinguish between the window
 * of the paned, a child and the handle.
 *
 * Returns: (transfer none): the paned’s handle window.
 *
 * Since: 2.20
 **/
GdkWindow *
ctk_paned_get_handle_window (CtkPaned *paned)
{
  g_return_val_if_fail (CTK_IS_PANED (paned), NULL);

  return paned->priv->handle;
}

/**
 * ctk_paned_set_wide_handle:
 * @paned: a #CtkPaned
 * @wide: the new value for the #CtkPaned:wide-handle property
 *
 * Sets the #CtkPaned:wide-handle property.
 *
 * Since: 3.16
 */
void
ctk_paned_set_wide_handle (CtkPaned *paned,
                           gboolean  wide)
{
  gboolean old_wide;

  g_return_if_fail (CTK_IS_PANED (paned));

  old_wide = ctk_paned_get_wide_handle (paned);
  if (old_wide != wide)
    {
      if (wide)
        ctk_css_gadget_add_class (paned->priv->handle_gadget, CTK_STYLE_CLASS_WIDE);
      else
        ctk_css_gadget_remove_class (paned->priv->handle_gadget, CTK_STYLE_CLASS_WIDE);

      ctk_widget_queue_resize (CTK_WIDGET (paned));
      g_object_notify (G_OBJECT (paned), "wide-handle");
    }
}

/**
 * ctk_paned_get_wide_handle:
 * @paned: a #CtkPaned
 *
 * Gets the #CtkPaned:wide-handle property.
 *
 * Returns: %TRUE if the paned should have a wide handle
 *
 * Since: 3.16
 */
gboolean
ctk_paned_get_wide_handle (CtkPaned *paned)
{
  g_return_val_if_fail (CTK_IS_PANED (paned), FALSE);

  if (ctk_css_node_has_class (ctk_css_gadget_get_node (paned->priv->handle_gadget), g_quark_from_static_string (CTK_STYLE_CLASS_WIDE)))
    return TRUE;
  else
    return FALSE;
}
