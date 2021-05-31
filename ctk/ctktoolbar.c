/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * CtkToolbar copyright (C) Federico Mena
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@gnome.org>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
 * Copyright (C) 2003, 2004 Soeren Sandmann <sandmann@daimi.au.dk>
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

#include <math.h>
#include <string.h>

#include "ctktoolbar.h"
#include "ctktoolbarprivate.h"

#include "ctkbindings.h"
#include "ctkbox.h"
#include "ctkcontainerprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkcssnodeprivate.h"
#include "ctkimage.h"
#include "ctkintl.h"
#include "ctklabel.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkmenu.h"
#include "ctkorientable.h"
#include "ctkorientableprivate.h"
#include "ctkprivate.h"
#include "ctkradiobutton.h"
#include "ctkradiotoolbutton.h"
#include "ctkrender.h"
#include "ctkseparatormenuitem.h"
#include "ctkseparatortoolitem.h"
#include "ctktoolshell.h"
#include "ctktypebuiltins.h"
#include "ctkwidgetpath.h"
#include "ctkwidgetprivate.h"
#include "ctkwindowprivate.h"


/**
 * SECTION:ctktoolbar
 * @Short_description: Create bars of buttons and other widgets
 * @Title: CtkToolbar
 * @See_also: #CtkToolItem
 *
 * A toolbar is created with a call to ctk_toolbar_new().
 *
 * A toolbar can contain instances of a subclass of #CtkToolItem. To add
 * a #CtkToolItem to the a toolbar, use ctk_toolbar_insert(). To remove
 * an item from the toolbar use ctk_container_remove(). To add a button
 * to the toolbar, add an instance of #CtkToolButton.
 *
 * Toolbar items can be visually grouped by adding instances of
 * #CtkSeparatorToolItem to the toolbar. If the CtkToolbar child property
 * “expand” is #TRUE and the property #CtkSeparatorToolItem:draw is set to
 * #FALSE, the effect is to force all following items to the end of the toolbar.
 *
 * By default, a toolbar can be shrunk, upon which it will add an arrow button
 * to show an overflow menu offering access to any #CtkToolItem child that has
 * a proxy menu item. To disable this and request enough size for all children,
 * call ctk_toolbar_set_show_arrow() to set #CtkToolbar:show-arrow to %FALSE.
 *
 * Creating a context menu for the toolbar can be done by connecting to
 * the #CtkToolbar::popup-context-menu signal.
 *
 * # CSS nodes
 *
 * CtkToolbar has a single CSS node with name toolbar.
 */


typedef struct _ToolbarContent ToolbarContent;

#define DEFAULT_SPACE_SIZE  12
#define DEFAULT_SPACE_STYLE CTK_TOOLBAR_SPACE_LINE
#define SPACE_LINE_DIVISION 10.0
#define SPACE_LINE_START    2.0
#define SPACE_LINE_END      8.0

#define DEFAULT_ICON_SIZE CTK_ICON_SIZE_LARGE_TOOLBAR
#define DEFAULT_TOOLBAR_STYLE CTK_TOOLBAR_BOTH_HORIZ
#define DEFAULT_ANIMATION_STATE TRUE

#define MAX_HOMOGENEOUS_N_CHARS 13 /* Items that are wider than this do not participate
				    * in the homogeneous game. In units of
				    * pango_font_get_estimated_char_width().
				    */
#define SLIDE_SPEED 600.0	   /* How fast the items slide, in pixels per second */
#define ACCEL_THRESHOLD 0.18	   /* After how much time in seconds will items start speeding up */


struct _CtkToolbarPrivate
{
  CtkMenu         *menu;
  CtkSettings     *settings;

  CtkIconSize      icon_size;
  CtkToolbarStyle  style;

  CtkToolItem     *highlight_tool_item;
  CtkWidget       *arrow;
  CtkWidget       *arrow_button;

  GdkWindow       *event_window;

  CtkCssGadget    *gadget;
  CtkAllocation    prev_allocation;

  GList           *content;

  GTimer          *timer;

  gulong           settings_connection;

  gint             idle_id;
  gint             button_maxw;         /* maximum width of homogeneous children */
  gint             button_maxh;         /* maximum height of homogeneous children */
  gint             max_homogeneous_pixels;
  gint             num_children;

  CtkOrientation   orientation;

  guint            animation : 1;
  guint            icon_size_set : 1;
  guint            is_sliding : 1;
  guint            need_rebuild : 1;  /* whether the overflow menu should be regenerated */
  guint            need_sync : 1;
  guint            show_arrow : 1;
  guint            style_set     : 1;
};

/* Properties */
enum {
  PROP_0,
  PROP_ORIENTATION,
  PROP_TOOLBAR_STYLE,
  PROP_SHOW_ARROW,
  PROP_TOOLTIPS,
  PROP_ICON_SIZE,
  PROP_ICON_SIZE_SET
};

/* Child properties */
enum {
  CHILD_PROP_0,
  CHILD_PROP_EXPAND,
  CHILD_PROP_HOMOGENEOUS
};

/* Signals */
enum {
  ORIENTATION_CHANGED,
  STYLE_CHANGED,
  POPUP_CONTEXT_MENU,
  FOCUS_HOME_OR_END,
  LAST_SIGNAL
};

typedef enum {
  NOT_ALLOCATED,
  NORMAL,
  HIDDEN,
  OVERFLOWN
} ItemState;


static void       ctk_toolbar_set_property         (GObject             *object,
						    guint                prop_id,
						    const GValue        *value,
						    GParamSpec          *pspec);
static void       ctk_toolbar_get_property         (GObject             *object,
						    guint                prop_id,
						    GValue              *value,
						    GParamSpec          *pspec);
static gint       ctk_toolbar_draw                 (CtkWidget           *widget,
                                                    cairo_t             *cr);
static void       ctk_toolbar_realize              (CtkWidget           *widget);
static void       ctk_toolbar_unrealize            (CtkWidget           *widget);
static void       ctk_toolbar_get_preferred_width  (CtkWidget           *widget,
                                                    gint                *minimum,
                                                    gint                *natural);
static void       ctk_toolbar_get_preferred_height (CtkWidget           *widget,
                                                    gint                *minimum,
                                                    gint                *natural);

static void       ctk_toolbar_size_allocate        (CtkWidget           *widget,
						    CtkAllocation       *allocation);
static void       ctk_toolbar_style_updated        (CtkWidget           *widget);
static gboolean   ctk_toolbar_focus                (CtkWidget           *widget,
						    CtkDirectionType     dir);
static void       ctk_toolbar_move_focus           (CtkWidget           *widget,
						    CtkDirectionType     dir);
static void       ctk_toolbar_screen_changed       (CtkWidget           *widget,
						    GdkScreen           *previous_screen);
static void       ctk_toolbar_map                  (CtkWidget           *widget);
static void       ctk_toolbar_unmap                (CtkWidget           *widget);
static void       ctk_toolbar_set_child_property   (CtkContainer        *container,
						    CtkWidget           *child,
						    guint                property_id,
						    const GValue        *value,
						    GParamSpec          *pspec);
static void       ctk_toolbar_get_child_property   (CtkContainer        *container,
						    CtkWidget           *child,
						    guint                property_id,
						    GValue              *value,
						    GParamSpec          *pspec);
static void       ctk_toolbar_finalize             (GObject             *object);
static void       ctk_toolbar_dispose              (GObject             *object);
static void       ctk_toolbar_show_all             (CtkWidget           *widget);
static void       ctk_toolbar_add                  (CtkContainer        *container,
						    CtkWidget           *widget);
static void       ctk_toolbar_remove               (CtkContainer        *container,
						    CtkWidget           *widget);
static void       ctk_toolbar_forall               (CtkContainer        *container,
						    gboolean             include_internals,
						    CtkCallback          callback,
						    gpointer             callback_data);
static GType      ctk_toolbar_child_type           (CtkContainer        *container);

static void       ctk_toolbar_direction_changed    (CtkWidget           *widget,
                                                    CtkTextDirection     previous_direction);
static void       ctk_toolbar_orientation_changed  (CtkToolbar          *toolbar,
						    CtkOrientation       orientation);
static void       ctk_toolbar_real_style_changed   (CtkToolbar          *toolbar,
						    CtkToolbarStyle      style);
static gboolean   ctk_toolbar_focus_home_or_end    (CtkToolbar          *toolbar,
						    gboolean             focus_home);
static gboolean   ctk_toolbar_button_press         (CtkWidget           *toolbar,
						    GdkEventButton      *event);
static gboolean   ctk_toolbar_arrow_button_press   (CtkWidget           *button,
						    GdkEventButton      *event,
						    CtkToolbar          *toolbar);
static void       ctk_toolbar_arrow_button_clicked (CtkWidget           *button,
						    CtkToolbar          *toolbar);
static void       ctk_toolbar_update_button_relief (CtkToolbar          *toolbar);
static gboolean   ctk_toolbar_popup_menu           (CtkWidget           *toolbar);
static void       ctk_toolbar_reconfigured         (CtkToolbar          *toolbar);

static void       ctk_toolbar_allocate             (CtkCssGadget        *gadget,
                                                    const CtkAllocation *allocation,
                                                    int                  baseline,
                                                    CtkAllocation       *out_clip,
                                                    gpointer             data);
static void       ctk_toolbar_measure              (CtkCssGadget   *gadget,
                                                    CtkOrientation  orientation,
                                                    int             for_size,
                                                    int            *minimum,
                                                    int            *natural,
                                                    int            *minimum_baseline,
                                                    int            *natural_baseline,
                                                    gpointer        data);
static gboolean   ctk_toolbar_render               (CtkCssGadget *gadget,
                                                    cairo_t      *cr,
                                                    int           x,
                                                    int           y,
                                                    int           width,
                                                    int           height,
                                                    gpointer      data);

static CtkReliefStyle       get_button_relief    (CtkToolbar *toolbar);
static gint                 get_max_child_expand (CtkToolbar *toolbar);

/* methods on ToolbarContent 'class' */
static ToolbarContent *toolbar_content_new_tool_item        (CtkToolbar          *toolbar,
							     CtkToolItem         *item,
							     gboolean             is_placeholder,
							     gint                 pos);
static void            toolbar_content_remove               (ToolbarContent      *content,
							     CtkToolbar          *toolbar);
static void            toolbar_content_free                 (ToolbarContent      *content);
static void            toolbar_content_draw                 (ToolbarContent      *content,
							     CtkContainer        *container,
                                                             cairo_t             *cr);
static gboolean        toolbar_content_visible              (ToolbarContent      *content,
							     CtkToolbar          *toolbar);
static void            toolbar_content_size_request         (ToolbarContent      *content,
							     CtkToolbar          *toolbar,
							     CtkRequisition      *requisition);
static gboolean        toolbar_content_is_homogeneous       (ToolbarContent      *content,
							     CtkToolbar          *toolbar);
static gboolean        toolbar_content_is_placeholder       (ToolbarContent      *content);
static gboolean        toolbar_content_disappearing         (ToolbarContent      *content);
static ItemState       toolbar_content_get_state            (ToolbarContent      *content);
static gboolean        toolbar_content_child_visible        (ToolbarContent      *content);
static void            toolbar_content_get_goal_allocation  (ToolbarContent      *content,
							     CtkAllocation       *allocation);
static void            toolbar_content_get_allocation       (ToolbarContent      *content,
							     CtkAllocation       *allocation);
static void            toolbar_content_set_start_allocation (ToolbarContent      *content,
							     CtkAllocation       *new_start_allocation);
static void            toolbar_content_get_start_allocation (ToolbarContent      *content,
							     CtkAllocation       *start_allocation);
static gboolean        toolbar_content_get_expand           (ToolbarContent      *content);
static void            toolbar_content_set_goal_allocation  (ToolbarContent      *content,
							     CtkAllocation       *allocation);
static void            toolbar_content_set_child_visible    (ToolbarContent      *content,
							     CtkToolbar          *toolbar,
							     gboolean             visible);
static void            toolbar_content_size_allocate        (ToolbarContent      *content,
							     CtkAllocation       *allocation);
static void            toolbar_content_set_state            (ToolbarContent      *content,
							     ItemState            new_state);
static CtkWidget *     toolbar_content_get_widget           (ToolbarContent      *content);
static void            toolbar_content_set_disappearing     (ToolbarContent      *content,
							     gboolean             disappearing);
static void            toolbar_content_set_size_request     (ToolbarContent      *content,
							     gint                 width,
							     gint                 height);
static void            toolbar_content_toolbar_reconfigured (ToolbarContent      *content,
							     CtkToolbar          *toolbar);
static CtkWidget *     toolbar_content_retrieve_menu_item   (ToolbarContent      *content);
static gboolean        toolbar_content_has_proxy_menu_item  (ToolbarContent	 *content);
static gboolean        toolbar_content_is_separator         (ToolbarContent      *content);
static void            toolbar_content_show_all             (ToolbarContent      *content);
static void	       toolbar_content_set_expand	    (ToolbarContent      *content,
							     gboolean		  expand);

static void            toolbar_tool_shell_iface_init        (CtkToolShellIface   *iface);
static CtkIconSize     toolbar_get_icon_size                (CtkToolShell        *shell);
static CtkOrientation  toolbar_get_orientation              (CtkToolShell        *shell);
static CtkToolbarStyle toolbar_get_style                    (CtkToolShell        *shell);
static CtkReliefStyle  toolbar_get_relief_style             (CtkToolShell        *shell);
static void            toolbar_rebuild_menu                 (CtkToolShell        *shell);


G_DEFINE_TYPE_WITH_CODE (CtkToolbar, ctk_toolbar, CTK_TYPE_CONTAINER,
                         G_ADD_PRIVATE (CtkToolbar)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_TOOL_SHELL,
                                                toolbar_tool_shell_iface_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ORIENTABLE,
                                                NULL))

static guint toolbar_signals[LAST_SIGNAL] = { 0 };


static void
add_arrow_bindings (CtkBindingSet   *binding_set,
		    guint            keysym,
		    CtkDirectionType dir)
{
  guint keypad_keysym = keysym - GDK_KEY_Left + GDK_KEY_KP_Left;
  
  ctk_binding_entry_add_signal (binding_set, keysym, 0,
                                "move-focus", 1,
                                CTK_TYPE_DIRECTION_TYPE, dir);
  ctk_binding_entry_add_signal (binding_set, keypad_keysym, 0,
                                "move-focus", 1,
                                CTK_TYPE_DIRECTION_TYPE, dir);
}

static void
add_ctrl_tab_bindings (CtkBindingSet    *binding_set,
		       GdkModifierType   modifiers,
		       CtkDirectionType  direction)
{
  ctk_binding_entry_add_signal (binding_set,
				GDK_KEY_Tab, GDK_CONTROL_MASK | modifiers,
				"move-focus", 1,
				CTK_TYPE_DIRECTION_TYPE, direction);
  ctk_binding_entry_add_signal (binding_set,
				GDK_KEY_KP_Tab, GDK_CONTROL_MASK | modifiers,
				"move-focus", 1,
				CTK_TYPE_DIRECTION_TYPE, direction);
}

static void
ctk_toolbar_class_init (CtkToolbarClass *klass)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;
  CtkBindingSet *binding_set;
  
  gobject_class = (GObjectClass *)klass;
  widget_class = (CtkWidgetClass *)klass;
  container_class = (CtkContainerClass *)klass;
  
  gobject_class->set_property = ctk_toolbar_set_property;
  gobject_class->get_property = ctk_toolbar_get_property;
  gobject_class->finalize = ctk_toolbar_finalize;
  gobject_class->dispose = ctk_toolbar_dispose;
  
  widget_class->button_press_event = ctk_toolbar_button_press;
  widget_class->draw = ctk_toolbar_draw;
  widget_class->get_preferred_width = ctk_toolbar_get_preferred_width;
  widget_class->get_preferred_height = ctk_toolbar_get_preferred_height;
  widget_class->size_allocate = ctk_toolbar_size_allocate;
  widget_class->style_updated = ctk_toolbar_style_updated;
  widget_class->focus = ctk_toolbar_focus;

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_TOOL_BAR);

  /* need to override the base class function via override_class_handler,
   * because the signal slot is not available in CtkWidgetClass
   */
  g_signal_override_class_handler ("move-focus",
                                   CTK_TYPE_TOOLBAR,
                                   G_CALLBACK (ctk_toolbar_move_focus));

  widget_class->screen_changed = ctk_toolbar_screen_changed;
  widget_class->realize = ctk_toolbar_realize;
  widget_class->unrealize = ctk_toolbar_unrealize;
  widget_class->map = ctk_toolbar_map;
  widget_class->unmap = ctk_toolbar_unmap;
  widget_class->popup_menu = ctk_toolbar_popup_menu;
  widget_class->show_all = ctk_toolbar_show_all;
  widget_class->direction_changed = ctk_toolbar_direction_changed;
  
  container_class->add    = ctk_toolbar_add;
  container_class->remove = ctk_toolbar_remove;
  container_class->forall = ctk_toolbar_forall;
  container_class->child_type = ctk_toolbar_child_type;
  container_class->get_child_property = ctk_toolbar_get_child_property;
  container_class->set_child_property = ctk_toolbar_set_child_property;

  ctk_container_class_handle_border_width (container_class);

  klass->orientation_changed = ctk_toolbar_orientation_changed;
  klass->style_changed = ctk_toolbar_real_style_changed;
  
  /**
   * CtkToolbar::orientation-changed:
   * @toolbar: the object which emitted the signal
   * @orientation: the new #CtkOrientation of the toolbar
   *
   * Emitted when the orientation of the toolbar changes.
   */
  toolbar_signals[ORIENTATION_CHANGED] =
    g_signal_new (I_("orientation-changed"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkToolbarClass, orientation_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_ORIENTATION);
  /**
   * CtkToolbar::style-changed:
   * @toolbar: The #CtkToolbar which emitted the signal
   * @style: the new #CtkToolbarStyle of the toolbar
   *
   * Emitted when the style of the toolbar changes. 
   */
  toolbar_signals[STYLE_CHANGED] =
    g_signal_new (I_("style-changed"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkToolbarClass, style_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_TOOLBAR_STYLE);
  /**
   * CtkToolbar::popup-context-menu:
   * @toolbar: the #CtkToolbar which emitted the signal
   * @x: the x coordinate of the point where the menu should appear
   * @y: the y coordinate of the point where the menu should appear
   * @button: the mouse button the user pressed, or -1
   *
   * Emitted when the user right-clicks the toolbar or uses the
   * keybinding to display a popup menu.
   *
   * Application developers should handle this signal if they want
   * to display a context menu on the toolbar. The context-menu should
   * appear at the coordinates given by @x and @y. The mouse button
   * number is given by the @button parameter. If the menu was popped
   * up using the keybaord, @button is -1.
   *
   * Returns: return %TRUE if the signal was handled, %FALSE if not
   */
  toolbar_signals[POPUP_CONTEXT_MENU] =
    g_signal_new (I_("popup-context-menu"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkToolbarClass, popup_context_menu),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__INT_INT_INT,
		  G_TYPE_BOOLEAN, 3,
		  G_TYPE_INT, G_TYPE_INT,
		  G_TYPE_INT);

  /**
   * CtkToolbar::focus-home-or-end:
   * @toolbar: the #CtkToolbar which emitted the signal
   * @focus_home: %TRUE if the first item should be focused
   *
   * A keybinding signal used internally by CTK+. This signal can't
   * be used in application code
   *
   * Returns: %TRUE if the signal was handled, %FALSE if not
   */
  toolbar_signals[FOCUS_HOME_OR_END] =
    g_signal_new_class_handler (I_("focus-home-or-end"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (ctk_toolbar_focus_home_or_end),
                                NULL, NULL,
                                _ctk_marshal_BOOLEAN__BOOLEAN,
                                G_TYPE_BOOLEAN, 1,
                                G_TYPE_BOOLEAN);

  /* properties */
  g_object_class_override_property (gobject_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  g_object_class_install_property (gobject_class,
				   PROP_TOOLBAR_STYLE,
				   g_param_spec_enum ("toolbar-style",
 						      P_("Toolbar Style"),
 						      P_("How to draw the toolbar"),
 						      CTK_TYPE_TOOLBAR_STYLE,
 						      DEFAULT_TOOLBAR_STYLE,
 						      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  g_object_class_install_property (gobject_class,
				   PROP_SHOW_ARROW,
				   g_param_spec_boolean ("show-arrow",
							 P_("Show Arrow"),
							 P_("If an arrow should be shown if the toolbar doesn't fit"),
							 TRUE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkToolbar:icon-size:
   *
   * The size of the icons in a toolbar is normally determined by
   * the toolbar-icon-size setting. When this property is set, it 
   * overrides the setting. 
   * 
   * This should only be used for special-purpose toolbars, normal
   * application toolbars should respect the user preferences for the
   * size of icons.
   *
   * Since: 2.10
   */
  g_object_class_install_property (gobject_class,
				   PROP_ICON_SIZE,
				   g_param_spec_enum ("icon-size",
                                                      P_("Icon size"),
                                                      P_("Size of icons in this toolbar"),
                                                      CTK_TYPE_ICON_SIZE,
                                                      DEFAULT_ICON_SIZE,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkToolbar:icon-size-set:
   *
   * Is %TRUE if the icon-size property has been set.
   *
   * Since: 2.10
   */
  g_object_class_install_property (gobject_class,
				   PROP_ICON_SIZE_SET,
				   g_param_spec_boolean ("icon-size-set",
							 P_("Icon size set"),
							 P_("Whether the icon-size property has been set"),
							 FALSE,
							 CTK_PARAM_READWRITE));  

  /* child properties */
  ctk_container_class_install_child_property (container_class,
					      CHILD_PROP_EXPAND,
					      g_param_spec_boolean ("expand", 
								    P_("Expand"), 
								    P_("Whether the item should receive extra space when the toolbar grows"),
								    FALSE,
								    CTK_PARAM_READWRITE));
  
  ctk_container_class_install_child_property (container_class,
					      CHILD_PROP_HOMOGENEOUS,
					      g_param_spec_boolean ("homogeneous", 
								    P_("Homogeneous"), 
								    P_("Whether the item should be the same size as other homogeneous items"),
								    FALSE,
								    CTK_PARAM_READWRITE));
  
  /**
   * CtkToolbar:space-size:
   *
   * Size of toolbar spacers.
   *
   * Deprecated: 3.20: Use the standard margin/padding CSS properties on the
   *   separator elements; the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("space-size",
							     P_("Spacer size"),
							     P_("Size of spacers"),
							     0,
							     G_MAXINT,
                                                             DEFAULT_SPACE_SIZE,
							     CTK_PARAM_READABLE|G_PARAM_DEPRECATED));
  
  /**
   * CtkToolbar:internal-padding:
   *
   * Amount of border space between the toolbar shadow and the buttons.
   *
   * Deprecated: 3.6: Use the standard padding CSS property
   *   (through objects like #CtkStyleContext and #CtkCssProvider); the value
   *   of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("internal-padding",
							     P_("Internal padding"),
							     P_("Amount of border space between the toolbar shadow and the buttons"),
							     0,
							     G_MAXINT,
                                                             0,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("max-child-expand",
                                                             P_("Maximum child expand"),
                                                             P_("Maximum amount of space an expandable item will be given"),
                                                             0,
                                                             G_MAXINT,
                                                             G_MAXINT,
                                                             CTK_PARAM_READABLE));

  /**
   * CtkToolbar:space-style:
   *
   * Style of toolbar spacers.
   *
   * Deprecated: 3.20: Use CSS properties on the separator elements to style
   *   toolbar spacers; the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_enum ("space-style",
							      P_("Space style"),
							      P_("Whether spacers are vertical lines or just blank"),
                                                              CTK_TYPE_TOOLBAR_SPACE_STYLE,
                                                              DEFAULT_SPACE_STYLE,
                                                              CTK_PARAM_READABLE|G_PARAM_DEPRECATED));
  
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_enum ("button-relief",
							      P_("Button relief"),
							      P_("Type of bevel around toolbar buttons"),
                                                              CTK_TYPE_RELIEF_STYLE,
                                                              CTK_RELIEF_NONE,
                                                              CTK_PARAM_READABLE));
  /**
   * CtkToolbar:shadow-type:
   *
   * Style of bevel around the toolbar.
   *
   * Deprecated: 3.6: Use the standard border CSS property
   *   (through objects like #CtkStyleContext and #CtkCssProvider); the value
   *   of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("shadow-type",
                                                              P_("Shadow type"),
                                                              P_("Style of bevel around the toolbar"),
                                                              CTK_TYPE_SHADOW_TYPE,
                                                              CTK_SHADOW_OUT,
                                                              CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  binding_set = ctk_binding_set_by_class (klass);
  
  add_arrow_bindings (binding_set, GDK_KEY_Left, CTK_DIR_LEFT);
  add_arrow_bindings (binding_set, GDK_KEY_Right, CTK_DIR_RIGHT);
  add_arrow_bindings (binding_set, GDK_KEY_Up, CTK_DIR_UP);
  add_arrow_bindings (binding_set, GDK_KEY_Down, CTK_DIR_DOWN);
  
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Home, 0,
                                "focus-home-or-end", 1,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_Home, 0,
                                "focus-home-or-end", 1,
				G_TYPE_BOOLEAN, TRUE);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_End, 0,
                                "focus-home-or-end", 1,
				G_TYPE_BOOLEAN, FALSE);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_End, 0,
                                "focus-home-or-end", 1,
				G_TYPE_BOOLEAN, FALSE);
  
  add_ctrl_tab_bindings (binding_set, 0, CTK_DIR_TAB_FORWARD);
  add_ctrl_tab_bindings (binding_set, GDK_SHIFT_MASK, CTK_DIR_TAB_BACKWARD);

  ctk_widget_class_set_css_name (widget_class, "toolbar");
}

static void
toolbar_tool_shell_iface_init (CtkToolShellIface *iface)
{
  iface->get_icon_size    = toolbar_get_icon_size;
  iface->get_orientation  = toolbar_get_orientation;
  iface->get_style        = toolbar_get_style;
  iface->get_relief_style = toolbar_get_relief_style;
  iface->rebuild_menu     = toolbar_rebuild_menu;
}

static void
ctk_toolbar_init (CtkToolbar *toolbar)
{
  CtkToolbarPrivate *priv;
  CtkWidget *widget;
  CtkCssNode *widget_node;

  widget = CTK_WIDGET (toolbar);
  toolbar->priv = ctk_toolbar_get_instance_private (toolbar);
  priv = toolbar->priv;

  ctk_widget_set_can_focus (widget, FALSE);
  ctk_widget_set_has_window (widget, FALSE);

  priv->orientation = CTK_ORIENTATION_HORIZONTAL;
  priv->style = DEFAULT_TOOLBAR_STYLE;
  priv->icon_size = DEFAULT_ICON_SIZE;
  priv->animation = DEFAULT_ANIMATION_STATE;

  _ctk_orientable_set_style_classes (CTK_ORIENTABLE (toolbar));

  widget_node = ctk_widget_get_css_node (widget);
  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     widget,
                                                     ctk_toolbar_measure,
                                                     ctk_toolbar_allocate,
                                                     ctk_toolbar_render,
                                                     NULL, NULL);

  priv->arrow_button = ctk_toggle_button_new ();
  g_signal_connect (priv->arrow_button, "button-press-event",
		    G_CALLBACK (ctk_toolbar_arrow_button_press), toolbar);
  g_signal_connect (priv->arrow_button, "clicked",
		    G_CALLBACK (ctk_toolbar_arrow_button_clicked), toolbar);
  ctk_button_set_relief (CTK_BUTTON (priv->arrow_button),
			 get_button_relief (toolbar));

  ctk_widget_set_focus_on_click (priv->arrow_button, FALSE);

  priv->arrow = ctk_image_new_from_icon_name ("pan-down-symbolic", CTK_ICON_SIZE_BUTTON);
  ctk_widget_set_name (priv->arrow, "ctk-toolbar-arrow");
  ctk_widget_show (priv->arrow);
  ctk_container_add (CTK_CONTAINER (priv->arrow_button), priv->arrow);
  
  ctk_widget_set_parent (priv->arrow_button, widget);
  
  /* which child position a drop will occur at */
  priv->menu = NULL;
  priv->show_arrow = TRUE;
  priv->settings = NULL;
  
  priv->max_homogeneous_pixels = -1;
  
  priv->timer = g_timer_new ();
}

static void
ctk_toolbar_set_property (GObject      *object,
			  guint         prop_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (object);
  CtkToolbarPrivate *priv = toolbar->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_signal_emit (toolbar, toolbar_signals[ORIENTATION_CHANGED], 0,
                     g_value_get_enum (value));
      break;
    case PROP_TOOLBAR_STYLE:
      ctk_toolbar_set_style (toolbar, g_value_get_enum (value));
      break;
    case PROP_SHOW_ARROW:
      ctk_toolbar_set_show_arrow (toolbar, g_value_get_boolean (value));
      break;
    case PROP_ICON_SIZE:
      ctk_toolbar_set_icon_size (toolbar, g_value_get_enum (value));
      break;
    case PROP_ICON_SIZE_SET:
      if (g_value_get_boolean (value))
	priv->icon_size_set = TRUE;
      else
	ctk_toolbar_unset_icon_size (toolbar);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_toolbar_get_property (GObject    *object,
			  guint       prop_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (object);
  CtkToolbarPrivate *priv = toolbar->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    case PROP_TOOLBAR_STYLE:
      g_value_set_enum (value, priv->style);
      break;
    case PROP_SHOW_ARROW:
      g_value_set_boolean (value, priv->show_arrow);
      break;
    case PROP_ICON_SIZE:
      g_value_set_enum (value, ctk_toolbar_get_icon_size (toolbar));
      break;
    case PROP_ICON_SIZE_SET:
      g_value_set_boolean (value, priv->icon_size_set);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_toolbar_map (CtkWidget *widget)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkToolbarPrivate *priv = toolbar->priv;

  CTK_WIDGET_CLASS (ctk_toolbar_parent_class)->map (widget);

  if (priv->event_window)
    gdk_window_show_unraised (priv->event_window);
}

static void
ctk_toolbar_unmap (CtkWidget *widget)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkToolbarPrivate *priv = toolbar->priv;

  if (priv->event_window)
    gdk_window_hide (priv->event_window);
  
  CTK_WIDGET_CLASS (ctk_toolbar_parent_class)->unmap (widget);
}

static void
ctk_toolbar_realize (CtkWidget *widget)
{
  CtkAllocation allocation;
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkToolbarPrivate *priv = toolbar->priv;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;

  ctk_widget_set_realized (widget, TRUE);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.wclass = GDK_INPUT_ONLY;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.event_mask = ctk_widget_get_events (widget);
  attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK |
			    GDK_POINTER_MOTION_MASK |
			    GDK_ENTER_NOTIFY_MASK |
			    GDK_LEAVE_NOTIFY_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y;

  window = ctk_widget_get_parent_window (widget);
  ctk_widget_set_window (widget, window);
  g_object_ref (window);

  priv->event_window = gdk_window_new (ctk_widget_get_parent_window (widget),
				       &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->event_window);
}

static void
ctk_toolbar_unrealize (CtkWidget *widget)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkToolbarPrivate *priv = toolbar->priv;

  if (priv->event_window)
    {
      ctk_widget_unregister_window (widget, priv->event_window);
      gdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  CTK_WIDGET_CLASS (ctk_toolbar_parent_class)->unrealize (widget);
}

static gboolean
ctk_toolbar_render (CtkCssGadget *gadget,
                    cairo_t      *cr,
                    int           x,
                    int           y,
                    int           width,
                    int           height,
                    gpointer      data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkToolbarPrivate *priv = toolbar->priv;
  GList *list;

  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      toolbar_content_draw (content, CTK_CONTAINER (widget), cr);
    }
  
  ctk_container_propagate_draw (CTK_CONTAINER (widget),
				priv->arrow_button,
				cr);

  return FALSE;
}

static gint
ctk_toolbar_draw (CtkWidget *widget,
                  cairo_t   *cr)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkToolbarPrivate *priv = toolbar->priv;

  ctk_css_gadget_draw (priv->gadget, cr);

  return FALSE;
}

static void
ctk_toolbar_measure (CtkCssGadget   *gadget,
                     CtkOrientation  orientation,
                     int             for_size,
                     int            *minimum,
                     int            *natural,
                     int            *minimum_baseline,
                     int            *natural_baseline,
                     gpointer        data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkToolbarPrivate *priv = toolbar->priv;
  GList *list;
  gint max_child_height;
  gint max_child_width;
  gint max_homogeneous_child_width;
  gint max_homogeneous_child_height;
  gint homogeneous_size;
  gint pack_front_size;
  CtkRequisition arrow_requisition, min_requisition, nat_requisition;

  max_homogeneous_child_width = 0;
  max_homogeneous_child_height = 0;
  max_child_width = 0;
  max_child_height = 0;
  for (list = priv->content; list != NULL; list = list->next)
    {
      CtkRequisition requisition;
      ToolbarContent *content = list->data;
      
      if (!toolbar_content_visible (content, toolbar))
	continue;
      
      toolbar_content_size_request (content, toolbar, &requisition);

      max_child_width = MAX (max_child_width, requisition.width);
      max_child_height = MAX (max_child_height, requisition.height);
      
      if (toolbar_content_is_homogeneous (content, toolbar))
	{
	  max_homogeneous_child_width = MAX (max_homogeneous_child_width, requisition.width);
	  max_homogeneous_child_height = MAX (max_homogeneous_child_height, requisition.height);
	}
    }
  
  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    homogeneous_size = max_homogeneous_child_width;
  else
    homogeneous_size = max_homogeneous_child_height;
  
  pack_front_size = 0;
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      guint size;
      
      if (!toolbar_content_visible (content, toolbar))
	continue;

      if (toolbar_content_is_homogeneous (content, toolbar))
	{
	  size = homogeneous_size;
	}
      else
	{
	  CtkRequisition requisition;
	  
	  toolbar_content_size_request (content, toolbar, &requisition);
	  
	  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
	    size = requisition.width;
	  else
	    size = requisition.height;
	}

      pack_front_size += size;
    }

  arrow_requisition.height = 0;
  arrow_requisition.width = 0;
  
  if (priv->show_arrow)
    ctk_widget_get_preferred_size (priv->arrow_button,
                                     &arrow_requisition, NULL);
  
  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      nat_requisition.width = pack_front_size;
      nat_requisition.height = MAX (max_child_height, arrow_requisition.height);

      min_requisition.width = priv->show_arrow ? arrow_requisition.width : nat_requisition.width;
      min_requisition.height = nat_requisition.height;
    }
  else
    {
      nat_requisition.width = MAX (max_child_width, arrow_requisition.width);
      nat_requisition.height = pack_front_size;

      min_requisition.width = nat_requisition.width;
      min_requisition.height = priv->show_arrow ? arrow_requisition.height : nat_requisition.height;
    }

  priv->button_maxw = max_homogeneous_child_width;
  priv->button_maxh = max_homogeneous_child_height;

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      *minimum = min_requisition.width;
      *natural = nat_requisition.width;
    }
  else
    {
      *minimum = min_requisition.height;
      *natural = nat_requisition.height;
    }
}

static void
ctk_toolbar_get_preferred_width (CtkWidget *widget,
                                 gint      *minimum,
                                 gint      *natural)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkToolbarPrivate *priv = toolbar->priv;

  ctk_css_gadget_get_preferred_size (priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_toolbar_get_preferred_height (CtkWidget *widget,
                                  gint      *minimum,
                                  gint      *natural)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkToolbarPrivate *priv = toolbar->priv;

  ctk_css_gadget_get_preferred_size (priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static gint
position (CtkToolbar *toolbar,
          gint        from,
          gint        to,
          gdouble     elapsed)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  gint n_pixels;

  if (!priv->animation)
    return to;

  if (elapsed <= ACCEL_THRESHOLD)
    {
      n_pixels = SLIDE_SPEED * elapsed;
    }
  else
    {
      /* The formula is a second degree polynomial in
       * @elapsed that has the line SLIDE_SPEED * @elapsed
       * as tangent for @elapsed == ACCEL_THRESHOLD.
       * This makes @n_pixels a smooth function of elapsed time.
       */
      n_pixels = (SLIDE_SPEED / ACCEL_THRESHOLD) * elapsed * elapsed -
	SLIDE_SPEED * elapsed + SLIDE_SPEED * ACCEL_THRESHOLD;
    }

  if (to > from)
    return MIN (from + n_pixels, to);
  else
    return MAX (from - n_pixels, to);
}

static void
compute_intermediate_allocation (CtkToolbar          *toolbar,
				 const CtkAllocation *start,
				 const CtkAllocation *goal,
				 CtkAllocation       *intermediate)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  gdouble elapsed = g_timer_elapsed (priv->timer, NULL);

  intermediate->x      = position (toolbar, start->x, goal->x, elapsed);
  intermediate->y      = position (toolbar, start->y, goal->y, elapsed);
  intermediate->width  = position (toolbar, start->x + start->width,
                                   goal->x + goal->width,
                                   elapsed) - intermediate->x;
  intermediate->height = position (toolbar, start->y + start->height,
                                   goal->y + goal->height,
                                   elapsed) - intermediate->y;
}

static void
fixup_allocation_for_rtl (gint           total_size,
			  CtkAllocation *allocation)
{
  allocation->x += (total_size - (2 * allocation->x + allocation->width));
}

static void
fixup_allocation_for_vertical (CtkAllocation *allocation)
{
  gint tmp;
  
  tmp = allocation->x;
  allocation->x = allocation->y;
  allocation->y = tmp;
  
  tmp = allocation->width;
  allocation->width = allocation->height;
  allocation->height = tmp;
}

static gint
get_item_size (CtkToolbar     *toolbar,
	       ToolbarContent *content)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  CtkRequisition requisition;
  
  toolbar_content_size_request (content, toolbar, &requisition);

  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (toolbar_content_is_homogeneous (content, toolbar))
	return priv->button_maxw;
      else
	return requisition.width;
    }
  else
    {
      if (toolbar_content_is_homogeneous (content, toolbar))
	return priv->button_maxh;
      else
	return requisition.height;
    }
}

static gboolean
slide_idle_handler (gpointer data)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (data);
  CtkToolbarPrivate *priv = toolbar->priv;
  GList *list;

  if (priv->need_sync)
    {
      gdk_display_flush (ctk_widget_get_display (data));
      priv->need_sync = FALSE;
    }
  
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      ItemState state;
      CtkAllocation goal_allocation;
      CtkAllocation allocation;
      gboolean cont;

      state = toolbar_content_get_state (content);
      toolbar_content_get_goal_allocation (content, &goal_allocation);
      toolbar_content_get_allocation (content, &allocation);
      
      cont = FALSE;
      
      if (state == NOT_ALLOCATED)
	{
	  /* an unallocated item means that size allocate has to
	   * called at least once more
	   */
	  cont = TRUE;
	}

      /* An invisible item with a goal allocation of
       * 0 is already at its goal.
       */
      if ((state == NORMAL || state == OVERFLOWN) &&
	  ((goal_allocation.width != 0 &&
	    goal_allocation.height != 0) ||
	   toolbar_content_child_visible (content)))
	{
	  if ((goal_allocation.x != allocation.x ||
	       goal_allocation.y != allocation.y ||
	       goal_allocation.width != allocation.width ||
	       goal_allocation.height != allocation.height))
	    {
	      /* An item is not in its right position yet. Note
	       * that OVERFLOWN items do get an allocation in
	       * ctk_toolbar_size_allocate(). This way you can see
	       * them slide back in when you drag an item off the
	       * toolbar.
	       */
	      cont = TRUE;
	    }
	}

      if (toolbar_content_is_placeholder (content) &&
	  toolbar_content_disappearing (content) &&
	  toolbar_content_child_visible (content))
	{
	  /* A disappearing placeholder is still visible.
	   */
	     
	  cont = TRUE;
	}
      
      if (cont)
	{
	  ctk_widget_queue_resize_no_redraw (CTK_WIDGET (toolbar));
	  
	  return TRUE;
	}
    }
  
  ctk_widget_queue_resize_no_redraw (CTK_WIDGET (toolbar));

  priv->is_sliding = FALSE;
  priv->idle_id = 0;

  return FALSE;
}

static gboolean
rect_within (CtkAllocation *a1,
	     CtkAllocation *a2)
{
  return (a1->x >= a2->x                         &&
	  a1->x + a1->width <= a2->x + a2->width &&
	  a1->y >= a2->y			 &&
	  a1->y + a1->height <= a2->y + a2->height);
}

static void
ctk_toolbar_begin_sliding (CtkToolbar *toolbar)
{
  CtkAllocation content_allocation;
  CtkWidget *widget = CTK_WIDGET (toolbar);
  CtkToolbarPrivate *priv = toolbar->priv;
  GList *list;
  gint cur_x;
  gint cur_y;
  gboolean rtl;
  gboolean vertical;
  
  /* Start the sliding. This function copies the allocation of every
   * item into content->start_allocation. For items that haven't
   * been allocated yet, we calculate their position and save that
   * in start_allocatino along with zero width and zero height.
   *
   * FIXME: It would be nice if we could share this code with
   * the equivalent in ctk_widget_size_allocate().
   */
  priv->is_sliding = TRUE;
  
  if (!priv->idle_id)
    {
      priv->idle_id = gdk_threads_add_idle (slide_idle_handler, toolbar);
      g_source_set_name_by_id (priv->idle_id, "[ctk+] slide_idle_handler");
    }

  ctk_css_gadget_get_content_allocation (priv->gadget,
                                         &content_allocation, NULL);

  rtl = (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL);
  vertical = (priv->orientation == CTK_ORIENTATION_VERTICAL);

  if (rtl)
    {
      cur_x = content_allocation.width;
      cur_y = content_allocation.height;
    }
  else
    {
      cur_x = 0;
      cur_y = 0;
    }

  cur_x += content_allocation.x;
  cur_y += content_allocation.y;

  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      CtkAllocation new_start_allocation;
      CtkAllocation item_allocation;
      ItemState state;
      
      state = toolbar_content_get_state (content);
      toolbar_content_get_allocation (content, &item_allocation);
      
      if ((state == NORMAL &&
	   rect_within (&item_allocation, &content_allocation)) ||
	  state == OVERFLOWN)
	{
	  new_start_allocation = item_allocation;
	}
      else
	{
	  new_start_allocation.x = cur_x;
	  new_start_allocation.y = cur_y;
	  
	  if (vertical)
	    {
	      new_start_allocation.width = content_allocation.width;
	      new_start_allocation.height = 0;
	    }
	  else
	    {
	      new_start_allocation.width = 0;
	      new_start_allocation.height = content_allocation.height;
	    }
	}
      
      if (vertical)
	cur_y = new_start_allocation.y + new_start_allocation.height;
      else if (rtl)
	cur_x = new_start_allocation.x;
      else
	cur_x = new_start_allocation.x + new_start_allocation.width;
      
      toolbar_content_set_start_allocation (content, &new_start_allocation);
    }

  /* This resize will run before the first idle handler. This
   * will make sure that items get the right goal allocation
   * so that the idle handler will not immediately return
   * FALSE
   */
  ctk_widget_queue_resize_no_redraw (CTK_WIDGET (toolbar));
  g_timer_reset (priv->timer);
}

static void
ctk_toolbar_stop_sliding (CtkToolbar *toolbar)
{
  CtkToolbarPrivate *priv = toolbar->priv;

  if (priv->is_sliding)
    {
      GList *list;
      
      priv->is_sliding = FALSE;
      
      if (priv->idle_id)
	{
	  g_source_remove (priv->idle_id);
	  priv->idle_id = 0;
	}
      
      list = priv->content;
      while (list)
	{
	  ToolbarContent *content = list->data;
	  list = list->next;

	  if (toolbar_content_is_placeholder (content))
	    {
	      toolbar_content_remove (content, toolbar);
	      toolbar_content_free (content);
	    }
	}
      
      ctk_widget_queue_resize_no_redraw (CTK_WIDGET (toolbar));
    }
}

static void
remove_item (CtkWidget *menu_item,
	     gpointer   data)
{
  ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (menu_item)),
                        menu_item);
}

static void
menu_deactivated (CtkWidget  *menu,
		  CtkToolbar *toolbar)
{
  CtkToolbarPrivate *priv = toolbar->priv;

  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (priv->arrow_button), FALSE);
}

static void
menu_detached (CtkWidget  *widget,
	       CtkMenu    *menu)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkToolbarPrivate *priv = toolbar->priv;

  priv->menu = NULL;
}

static void
rebuild_menu (CtkToolbar *toolbar)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  GList *list, *children;

  if (!priv->menu)
    {
      priv->menu = CTK_MENU (ctk_menu_new ());
      ctk_menu_attach_to_widget (priv->menu,
				 CTK_WIDGET (toolbar),
				 menu_detached);

      g_signal_connect (priv->menu, "deactivate",
                        G_CALLBACK (menu_deactivated), toolbar);
    }

  ctk_container_foreach (CTK_CONTAINER (priv->menu), remove_item, NULL);
  
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      if (toolbar_content_get_state (content) == OVERFLOWN &&
	  !toolbar_content_is_placeholder (content))
	{
	  CtkWidget *menu_item = toolbar_content_retrieve_menu_item (content);
	  
	  if (menu_item)
	    {
	      g_assert (CTK_IS_MENU_ITEM (menu_item));
	      ctk_menu_shell_append (CTK_MENU_SHELL (priv->menu), menu_item);
	    }
	}
    }

  /* Remove leading and trailing separator items */
  children = ctk_container_get_children (CTK_CONTAINER (priv->menu));
  
  list = children;
  while (list && CTK_IS_SEPARATOR_MENU_ITEM (list->data))
    {
      CtkWidget *child = list->data;
      
      ctk_container_remove (CTK_CONTAINER (priv->menu), child);
      list = list->next;
    }
  g_list_free (children);

  /* Regenerate the list of children so we don't try to remove items twice */
  children = ctk_container_get_children (CTK_CONTAINER (priv->menu));

  list = g_list_last (children);
  while (list && CTK_IS_SEPARATOR_MENU_ITEM (list->data))
    {
      CtkWidget *child = list->data;

      ctk_container_remove (CTK_CONTAINER (priv->menu), child);
      list = list->prev;
    }
  g_list_free (children);

  priv->need_rebuild = FALSE;
}

static void
ctk_toolbar_allocate (CtkCssGadget        *gadget,
                      const CtkAllocation *allocation,
                      int                  baseline,
                      CtkAllocation       *out_clip,
                      gpointer             data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkToolbarPrivate *priv = toolbar->priv;
  CtkAllocation arrow_allocation, item_area, widget_allocation;
  CtkAllocation *allocations;
  ItemState *new_states;
  gint arrow_size;
  gint size, pos, short_size;
  GList *list;
  gint i;
  gboolean need_arrow;
  gint n_expand_items;
  gint available_size;
  gint n_items;
  gint needed_size;
  CtkRequisition arrow_requisition;
  gboolean overflowing;
  gboolean size_changed;

  ctk_widget_get_allocation (widget, &widget_allocation);
  size_changed = FALSE;
  if (widget_allocation.x != priv->prev_allocation.x ||
      widget_allocation.y != priv->prev_allocation.y ||
      widget_allocation.width != priv->prev_allocation.width ||
      widget_allocation.height != priv->prev_allocation.height)
    {
      size_changed = TRUE;
    }

  if (size_changed)
    ctk_toolbar_stop_sliding (toolbar);

  ctk_widget_get_preferred_size (priv->arrow_button,
                                 &arrow_requisition, NULL);

  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      available_size = size = allocation->width;
      short_size = allocation->height;
      arrow_size = arrow_requisition.width;
    }
  else
    {
      available_size = size = allocation->height;
      short_size = allocation->width;
      arrow_size = arrow_requisition.height;
    }

  n_items = g_list_length (priv->content);
  allocations = g_new0 (CtkAllocation, n_items);
  new_states = g_new0 (ItemState, n_items);

  needed_size = 0;
  need_arrow = FALSE;
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;

      if (toolbar_content_visible (content, toolbar))
        {
          needed_size += get_item_size (toolbar, content);

          /* Do we need an arrow?
           *
           * Assume we don't, and see if any non-separator item
           * with a proxy menu item is then going to overflow.
           */
          if (needed_size > available_size &&
              !need_arrow &&
              priv->show_arrow &&
              toolbar_content_has_proxy_menu_item (content) &&
              !toolbar_content_is_separator (content))
            {
              need_arrow = TRUE;
            }
        }
    }

  if (need_arrow)
    size = available_size - arrow_size;
  else
    size = available_size;

  /* calculate widths and states of items */
  overflowing = FALSE;
  for (list = priv->content, i = 0; list != NULL; list = list->next, ++i)
    {
      ToolbarContent *content = list->data;
      gint item_size;

      if (!toolbar_content_visible (content, toolbar))
        {
          new_states[i] = HIDDEN;
          continue;
        }

      item_size = get_item_size (toolbar, content);
      if (item_size <= size && !overflowing)
        {
          size -= item_size;
          allocations[i].width = item_size;
          new_states[i] = NORMAL;
        }
      else
        {
          overflowing = TRUE;
          new_states[i] = OVERFLOWN;
          allocations[i].width = item_size;
        }
    }

  /* calculate width of arrow */
  if (need_arrow)
    {
      arrow_allocation.width = arrow_size;
      arrow_allocation.height = MAX (short_size, 1);
    }

  /* expand expandable items */

  /* We don't expand when there is an overflow menu,
   * because that leads to weird jumps when items get
   * moved to the overflow menu and the expanding
   * items suddenly get a lot of extra space
   */
  if (!overflowing)
    {
      gint max_child_expand;
      n_expand_items = 0;

      for (i = 0, list = priv->content; list != NULL; list = list->next, ++i)
        {
          ToolbarContent *content = list->data;

          if (toolbar_content_get_expand (content) && new_states[i] == NORMAL)
            n_expand_items++;
        }

      max_child_expand = get_max_child_expand (toolbar);
      for (list = priv->content, i = 0; list != NULL; list = list->next, ++i)
        {
          ToolbarContent *content = list->data;

          if (toolbar_content_get_expand (content) && new_states[i] == NORMAL)
            {
              gint extra = size / n_expand_items;
              if (size % n_expand_items != 0)
                extra++;

              if (extra > max_child_expand)
                extra = max_child_expand;

              allocations[i].width += extra;
              size -= extra;
              n_expand_items--;
            }
        }

      g_assert (n_expand_items == 0);
    }

  /* position items */
  pos = 0;
  for (list = priv->content, i = 0; list != NULL; list = list->next, ++i)
    {
      /* Both NORMAL and OVERFLOWN items get a position.
       * This ensures that sliding will work for OVERFLOWN items too.
       */
      if (new_states[i] == NORMAL || new_states[i] == OVERFLOWN)
        {
          allocations[i].x = pos;
          allocations[i].y = 0;
          allocations[i].height = short_size;

          pos += allocations[i].width;
        }
    }

  /* position arrow */
  if (need_arrow)
    {
      arrow_allocation.x = available_size - arrow_allocation.width;
      arrow_allocation.y = 0;
    }

  item_area.x = 0;
  item_area.y = 0;
  item_area.width = available_size - (need_arrow? arrow_size : 0);
  item_area.height = short_size;

  /* fix up allocations in the vertical or RTL cases */
  if (priv->orientation == CTK_ORIENTATION_VERTICAL)
    {
      for (i = 0; i < n_items; ++i)
        fixup_allocation_for_vertical (&(allocations[i]));

      if (need_arrow)
        fixup_allocation_for_vertical (&arrow_allocation);

      fixup_allocation_for_vertical (&item_area);
    }
  else if (ctk_widget_get_direction (CTK_WIDGET (toolbar)) == CTK_TEXT_DIR_RTL)
    {
      for (i = 0; i < n_items; ++i)
        fixup_allocation_for_rtl (available_size, &(allocations[i]));

      if (need_arrow)
        fixup_allocation_for_rtl (available_size, &arrow_allocation);

      fixup_allocation_for_rtl (available_size, &item_area);
    }

  /* translate the items by allocation->(x,y) */
  for (i = 0; i < n_items; ++i)
    {
      allocations[i].x += allocation->x;
      allocations[i].y += allocation->y;
    }

  if (need_arrow)
    {
      arrow_allocation.x += allocation->x;
      arrow_allocation.y += allocation->y;
    }

  item_area.x += allocation->x;
  item_area.y += allocation->y;

  /* did anything change? */
  for (list = priv->content, i = 0; list != NULL; list = list->next, i++)
    {
      ToolbarContent *content = list->data;

      if (toolbar_content_get_state (content) == NORMAL &&
          new_states[i] != NORMAL)
        {
          /* an item disappeared and we didn't change size, so begin sliding */
          if (!size_changed)
            ctk_toolbar_begin_sliding (toolbar);
        }
    }

  /* finally allocate the items */
  if (priv->is_sliding)
    {
      for (list = priv->content, i = 0; list != NULL; list = list->next, i++)
        {
          ToolbarContent *content = list->data;

          toolbar_content_set_goal_allocation (content, &(allocations[i]));
        }
    }

  for (list = priv->content, i = 0; list != NULL; list = list->next, ++i)
    {
      ToolbarContent *content = list->data;

      if (new_states[i] == OVERFLOWN || new_states[i] == NORMAL)
        {
          CtkAllocation alloc;
          CtkAllocation start_allocation = { 0, };
          CtkAllocation goal_allocation;

          if (priv->is_sliding)
            {
              toolbar_content_get_start_allocation (content, &start_allocation);
              toolbar_content_get_goal_allocation (content, &goal_allocation);

              compute_intermediate_allocation (toolbar,
                                               &start_allocation,
                                               &goal_allocation,
                                               &alloc);

              priv->need_sync = TRUE;
            }
          else
            {
              alloc = allocations[i];
            }

          if (alloc.width <= 0 || alloc.height <= 0)
            {
              toolbar_content_set_child_visible (content, toolbar, FALSE);
            }
          else
            {
              if (!rect_within (&alloc, &item_area))
                {
                  toolbar_content_set_child_visible (content, toolbar, FALSE);
                  toolbar_content_size_allocate (content, &alloc);
                }
              else
                {
                  toolbar_content_set_child_visible (content, toolbar, TRUE);
                  toolbar_content_size_allocate (content, &alloc);
                }
            }
        }
      else
        {
          toolbar_content_set_child_visible (content, toolbar, FALSE);
        }

      toolbar_content_set_state (content, new_states[i]);
    }

  if (priv->menu && priv->need_rebuild)
    rebuild_menu (toolbar);

  if (need_arrow)
    {
      ctk_widget_size_allocate (CTK_WIDGET (priv->arrow_button),
                                &arrow_allocation);
      ctk_widget_show (CTK_WIDGET (priv->arrow_button));
    }
  else
    {
      ctk_widget_hide (CTK_WIDGET (priv->arrow_button));

      if (priv->menu && ctk_widget_get_visible (CTK_WIDGET (priv->menu)))
        ctk_menu_shell_deactivate (CTK_MENU_SHELL (priv->menu));
    }

  g_free (allocations);
  g_free (new_states);
}

static void
ctk_toolbar_size_allocate (CtkWidget     *widget,
			   CtkAllocation *allocation)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkToolbarPrivate *priv = toolbar->priv;
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  if (ctk_widget_get_realized (widget))
    gdk_window_move_resize (priv->event_window,
                            allocation->x,
                            allocation->y,
                            allocation->width,
                            allocation->height);

  ctk_css_gadget_allocate (priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_toolbar_update_button_relief (CtkToolbar *toolbar)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  CtkReliefStyle relief;

  relief = get_button_relief (toolbar);

  if (relief != ctk_button_get_relief (CTK_BUTTON (priv->arrow_button)))
    {
      ctk_toolbar_reconfigured (toolbar);
  
      ctk_button_set_relief (CTK_BUTTON (priv->arrow_button), relief);
    }
}

static void
ctk_toolbar_style_updated (CtkWidget *widget)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkToolbarPrivate *priv = toolbar->priv;

  CTK_WIDGET_CLASS (ctk_toolbar_parent_class)->style_updated (widget);

  priv->max_homogeneous_pixels = -1;
  ctk_toolbar_update_button_relief (CTK_TOOLBAR (widget));
}

static GList *
ctk_toolbar_list_children_in_focus_order (CtkToolbar       *toolbar,
					  CtkDirectionType  dir)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  GList *result = NULL;
  GList *list;
  gboolean rtl;
  
  /* generate list of children in reverse logical order */
  
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      CtkWidget *widget;
      
      widget = toolbar_content_get_widget (content);
      
      if (widget)
	result = g_list_prepend (result, widget);
    }
  
  result = g_list_prepend (result, priv->arrow_button);
  
  rtl = (ctk_widget_get_direction (CTK_WIDGET (toolbar)) == CTK_TEXT_DIR_RTL);
  
  /* move in logical order when
   *
   *	- dir is TAB_FORWARD
   *
   *	- in RTL mode and moving left or up
   *
   *    - in LTR mode and moving right or down
   */
  if (dir == CTK_DIR_TAB_FORWARD                                        ||
      (rtl  && (dir == CTK_DIR_UP   || dir == CTK_DIR_LEFT))		||
      (!rtl && (dir == CTK_DIR_DOWN || dir == CTK_DIR_RIGHT)))
    {
      result = g_list_reverse (result);
    }
  
  return result;
}

static gboolean
ctk_toolbar_focus_home_or_end (CtkToolbar *toolbar,
			       gboolean    focus_home)
{
  GList *children, *list;
  CtkDirectionType dir = focus_home? CTK_DIR_RIGHT : CTK_DIR_LEFT;
  
  children = ctk_toolbar_list_children_in_focus_order (toolbar, dir);
  
  if (ctk_widget_get_direction (CTK_WIDGET (toolbar)) == CTK_TEXT_DIR_RTL)
    {
      children = g_list_reverse (children);
      
      dir = (dir == CTK_DIR_RIGHT)? CTK_DIR_LEFT : CTK_DIR_RIGHT;
    }
  
  for (list = children; list != NULL; list = list->next)
    {
      CtkWidget *child = list->data;

      if (ctk_container_get_focus_child (CTK_CONTAINER (toolbar)) == child)
	break;
      
      if (ctk_widget_get_mapped (child) && ctk_widget_child_focus (child, dir))
	break;
    }
  
  g_list_free (children);
  
  return TRUE;
}   

/* Keybinding handler. This function is called when the user presses
 * Ctrl TAB or an arrow key.
 */
static void
ctk_toolbar_move_focus (CtkWidget        *widget,
			CtkDirectionType  dir)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkContainer *container = CTK_CONTAINER (toolbar);
  CtkWidget *focus_child;
  GList *list;
  gboolean try_focus = FALSE;
  GList *children;

  focus_child = ctk_container_get_focus_child (container);

  if (focus_child && ctk_widget_child_focus (focus_child, dir))
    return;
  
  children = ctk_toolbar_list_children_in_focus_order (toolbar, dir);
  
  for (list = children; list != NULL; list = list->next)
    {
      CtkWidget *child = list->data;
      
      if (try_focus && ctk_widget_get_mapped (child) && ctk_widget_child_focus (child, dir))
	break;
      
      if (child == focus_child)
	try_focus = TRUE;
    }
  
  g_list_free (children);
}

/* The focus handler for the toolbar. It called when the user presses
 * TAB or otherwise tries to focus the toolbar.
 */
static gboolean
ctk_toolbar_focus (CtkWidget        *widget,
		   CtkDirectionType  dir)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  GList *children, *list;
  gboolean result = FALSE;

  /* if focus is already somewhere inside the toolbar then return FALSE.
   * The only way focus can stay inside the toolbar is when the user presses
   * arrow keys or Ctrl TAB (both of which are handled by the
   * ctk_toolbar_move_focus() keybinding function.
   */
  if (ctk_container_get_focus_child (CTK_CONTAINER (widget)))
    return FALSE;

  children = ctk_toolbar_list_children_in_focus_order (toolbar, dir);

  for (list = children; list != NULL; list = list->next)
    {
      CtkWidget *child = list->data;
      
      if (ctk_widget_get_mapped (child) && ctk_widget_child_focus (child, dir))
	{
	  result = TRUE;
	  break;
	}
    }

  g_list_free (children);

  return result;
}

static CtkSettings *
toolbar_get_settings (CtkToolbar *toolbar)
{
  return toolbar->priv->settings;
}

static void
animation_change_notify (CtkToolbar *toolbar)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  CtkSettings *settings = toolbar_get_settings (toolbar);
  gboolean animation;

  if (settings)
    g_object_get (settings,
                  "ctk-enable-animations", &animation,
                  NULL);
  else
    animation = DEFAULT_ANIMATION_STATE;

  priv->animation = animation;
}

static void
settings_change_notify (CtkSettings      *settings,
                        const GParamSpec *pspec,
                        CtkToolbar       *toolbar)
{
  if (! strcmp (pspec->name, "ctk-enable-animations"))
    animation_change_notify (toolbar);
}

static void
ctk_toolbar_screen_changed (CtkWidget *widget,
			    GdkScreen *previous_screen)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkToolbarPrivate *priv = toolbar->priv;
  CtkSettings *old_settings = toolbar_get_settings (toolbar);
  CtkSettings *settings;
  
  if (ctk_widget_has_screen (CTK_WIDGET (toolbar)))
    settings = ctk_widget_get_settings (CTK_WIDGET (toolbar));
  else
    settings = NULL;
  
  if (settings == old_settings)
    return;
  
  if (old_settings)
    {
      g_signal_handler_disconnect (old_settings, priv->settings_connection);
      priv->settings_connection = 0;
      g_object_unref (old_settings);
    }

  if (settings)
    {
      priv->settings_connection =
	g_signal_connect (settings, "notify",
                          G_CALLBACK (settings_change_notify),
                          toolbar);

      priv->settings = g_object_ref (settings);
    }
  else
    priv->settings = NULL;

  animation_change_notify (toolbar);
}

static int
find_drop_index (CtkToolbar *toolbar,
		 gint        x,
		 gint        y)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  GList *interesting_content;
  GList *list;
  CtkOrientation orientation;
  CtkTextDirection direction;
  gint best_distance = G_MAXINT;
  gint distance;
  gint cursor;
  gint pos;
  ToolbarContent *best_content;
  CtkAllocation allocation;
  
  /* list items we care about wrt. drag and drop */
  interesting_content = NULL;
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      if (toolbar_content_get_state (content) == NORMAL)
	interesting_content = g_list_prepend (interesting_content, content);
    }
  interesting_content = g_list_reverse (interesting_content);
  
  if (!interesting_content)
    return 0;
  
  orientation = priv->orientation;
  direction = ctk_widget_get_direction (CTK_WIDGET (toolbar));
  
  /* distance to first interesting item */
  best_content = interesting_content->data;
  toolbar_content_get_allocation (best_content, &allocation);
  
  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      cursor = x;
      
      if (direction == CTK_TEXT_DIR_LTR)
	pos = allocation.x;
      else
	pos = allocation.x + allocation.width;
    }
  else
    {
      cursor = y;
      pos = allocation.y;
    }
  
  best_content = NULL;
  best_distance = ABS (pos - cursor);
  
  /* distance to far end of each item */
  for (list = interesting_content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      toolbar_content_get_allocation (content, &allocation);
      
      if (orientation == CTK_ORIENTATION_HORIZONTAL)
	{
	  if (direction == CTK_TEXT_DIR_LTR)
	    pos = allocation.x + allocation.width;
	  else
	    pos = allocation.x;
	}
      else
	{
	  pos = allocation.y + allocation.height;
	}
      
      distance = ABS (pos - cursor);
      
      if (distance < best_distance)
	{
	  best_distance = distance;
	  best_content = content;
	}
    }
  
  g_list_free (interesting_content);
  
  if (!best_content)
    return 0;
  else
    return g_list_index (priv->content, best_content) + 1;
}

static void
reset_all_placeholders (CtkToolbar *toolbar)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  GList *list;
  
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      if (toolbar_content_is_placeholder (content))
	toolbar_content_set_disappearing (content, TRUE);
    }
}

static gint
physical_to_logical (CtkToolbar *toolbar,
		     gint        physical)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  GList *list;
  int logical;
  
  g_assert (physical >= 0);
  
  logical = 0;
  for (list = priv->content; list && physical > 0; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      if (!toolbar_content_is_placeholder (content))
	logical++;
      physical--;
    }
  
  g_assert (physical == 0);
  
  return logical;
}

static gint
logical_to_physical (CtkToolbar *toolbar,
		     gint        logical)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  GList *list;
  gint physical;
  
  g_assert (logical >= 0);
  
  physical = 0;
  for (list = priv->content; list; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      if (!toolbar_content_is_placeholder (content))
	{
	  if (logical == 0)
	    break;
	  logical--;
	}
      
      physical++;
    }
  
  g_assert (logical == 0);
  
  return physical;
}

/**
 * ctk_toolbar_set_drop_highlight_item:
 * @toolbar: a #CtkToolbar
 * @tool_item: (allow-none): a #CtkToolItem, or %NULL to turn of highlighting
 * @index_: a position on @toolbar
 *
 * Highlights @toolbar to give an idea of what it would look like
 * if @item was added to @toolbar at the position indicated by @index_.
 * If @item is %NULL, highlighting is turned off. In that case @index_ 
 * is ignored.
 *
 * The @tool_item passed to this function must not be part of any widget
 * hierarchy. When an item is set as drop highlight item it can not
 * added to any widget hierarchy or used as highlight item for another
 * toolbar.
 * 
 * Since: 2.4
 **/
void
ctk_toolbar_set_drop_highlight_item (CtkToolbar  *toolbar,
				     CtkToolItem *tool_item,
				     gint         index_)
{
  ToolbarContent *content;
  CtkToolbarPrivate *priv;
  gint n_items;
  CtkRequisition requisition;
  CtkRequisition old_requisition;
  gboolean restart_sliding;
  
  g_return_if_fail (CTK_IS_TOOLBAR (toolbar));
  g_return_if_fail (tool_item == NULL || CTK_IS_TOOL_ITEM (tool_item));

  priv = toolbar->priv;

  if (!tool_item)
    {
      if (priv->highlight_tool_item)
	{
	  ctk_widget_unparent (CTK_WIDGET (priv->highlight_tool_item));
	  g_object_unref (priv->highlight_tool_item);
	  priv->highlight_tool_item = NULL;
	}
      
      reset_all_placeholders (toolbar);
      ctk_toolbar_begin_sliding (toolbar);
      return;
    }
  
  n_items = ctk_toolbar_get_n_items (toolbar);
  if (index_ < 0 || index_ > n_items)
    index_ = n_items;
  
  if (tool_item != priv->highlight_tool_item)
    {
      if (priv->highlight_tool_item)
	g_object_unref (priv->highlight_tool_item);
      
      g_object_ref_sink (tool_item);
      
      priv->highlight_tool_item = tool_item;
      
      ctk_widget_set_parent (CTK_WIDGET (priv->highlight_tool_item),
			     CTK_WIDGET (toolbar));
    }
  
  index_ = logical_to_physical (toolbar, index_);
  
  content = g_list_nth_data (priv->content, index_);
  
  if (index_ > 0)
    {
      ToolbarContent *prev_content;
      
      prev_content = g_list_nth_data (priv->content, index_ - 1);
      
      if (prev_content && toolbar_content_is_placeholder (prev_content))
	content = prev_content;
    }
  
  if (!content || !toolbar_content_is_placeholder (content))
    {
      CtkWidget *placeholder;
      
      placeholder = CTK_WIDGET (ctk_separator_tool_item_new ());

      content = toolbar_content_new_tool_item (toolbar,
					       CTK_TOOL_ITEM (placeholder),
					       TRUE, index_);
      ctk_widget_show (placeholder);
    }
  
  g_assert (content);
  g_assert (toolbar_content_is_placeholder (content));

  ctk_widget_get_preferred_size (CTK_WIDGET (priv->highlight_tool_item),
                                 &requisition, NULL);

  toolbar_content_set_expand (content, ctk_tool_item_get_expand (tool_item));
  
  restart_sliding = FALSE;
  toolbar_content_size_request (content, toolbar, &old_requisition);
  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      requisition.height = -1;
      if (requisition.width != old_requisition.width)
	restart_sliding = TRUE;
    }
  else
    {
      requisition.width = -1;
      if (requisition.height != old_requisition.height)
	restart_sliding = TRUE;
    }

  if (toolbar_content_disappearing (content))
    restart_sliding = TRUE;
  
  reset_all_placeholders (toolbar);
  toolbar_content_set_disappearing (content, FALSE);
  
  toolbar_content_set_size_request (content,
				    requisition.width, requisition.height);
  
  if (restart_sliding)
    ctk_toolbar_begin_sliding (toolbar);
}

static void
ctk_toolbar_get_child_property (CtkContainer *container,
				CtkWidget    *child,
				guint         property_id,
				GValue       *value,
				GParamSpec   *pspec)
{
  CtkToolItem *item = CTK_TOOL_ITEM (child);
  
  switch (property_id)
    {
    case CHILD_PROP_HOMOGENEOUS:
      g_value_set_boolean (value, ctk_tool_item_get_homogeneous (item));
      break;
      
    case CHILD_PROP_EXPAND:
      g_value_set_boolean (value, ctk_tool_item_get_expand (item));
      break;
      
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_toolbar_set_child_property (CtkContainer *container,
				CtkWidget    *child,
				guint         property_id,
				const GValue *value,
				GParamSpec   *pspec)
{
  switch (property_id)
    {
    case CHILD_PROP_HOMOGENEOUS:
      ctk_tool_item_set_homogeneous (CTK_TOOL_ITEM (child), g_value_get_boolean (value));
      break;
      
    case CHILD_PROP_EXPAND:
      ctk_tool_item_set_expand (CTK_TOOL_ITEM (child), g_value_get_boolean (value));
      break;
      
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_toolbar_show_all (CtkWidget *widget)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (widget);
  CtkToolbarPrivate *priv = toolbar->priv;
  GList *list;

  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      toolbar_content_show_all (content);
    }
  
  ctk_widget_show (widget);
}

static void
ctk_toolbar_add (CtkContainer *container,
		 CtkWidget    *widget)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (container);

  ctk_toolbar_insert (toolbar, CTK_TOOL_ITEM (widget), -1);
}

static void
ctk_toolbar_remove (CtkContainer *container,
		    CtkWidget    *widget)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (container);
  CtkToolbarPrivate *priv = toolbar->priv;
  ToolbarContent *content_to_remove;
  GList *list;

  content_to_remove = NULL;
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      CtkWidget *child;
      
      child = toolbar_content_get_widget (content);
      if (child && child == widget)
	{
	  content_to_remove = content;
	  break;
	}
    }
  
  g_return_if_fail (content_to_remove != NULL);
  
  toolbar_content_remove (content_to_remove, toolbar);
  toolbar_content_free (content_to_remove);
}

static void
ctk_toolbar_forall (CtkContainer *container,
                    gboolean      include_internals,
                    CtkCallback   callback,
                    gpointer      callback_data)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (container);
  CtkToolbarPrivate *priv = toolbar->priv;
  GList *list;

  g_return_if_fail (callback != NULL);

  list = priv->content;
  while (list)
    {
      ToolbarContent *content = list->data;
      GList *next = list->next;

      if (include_internals || !toolbar_content_is_placeholder (content))
        {
          CtkWidget *child = toolbar_content_get_widget (content);

          if (child)
            callback (child, callback_data);
        }

      list = next;
    }

  if (include_internals && priv->arrow_button)
    callback (priv->arrow_button, callback_data);
}

static GType
ctk_toolbar_child_type (CtkContainer *container)
{
  return CTK_TYPE_TOOL_ITEM;
}

static void
ctk_toolbar_reconfigured (CtkToolbar *toolbar)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  GList *list;
  
  list = priv->content;
  while (list)
    {
      ToolbarContent *content = list->data;
      GList *next = list->next;
      
      toolbar_content_toolbar_reconfigured (content, toolbar);
      
      list = next;
    }
}

static void
ctk_toolbar_orientation_changed (CtkToolbar    *toolbar,
				 CtkOrientation orientation)
{
  CtkToolbarPrivate *priv = toolbar->priv;

  if (priv->orientation != orientation)
    {
      priv->orientation = orientation;
      
      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        ctk_image_set_from_icon_name (CTK_IMAGE (priv->arrow), "pan-down-symbolic", CTK_ICON_SIZE_BUTTON);
      else
        ctk_image_set_from_icon_name (CTK_IMAGE (priv->arrow), "pan-end-symbolic", CTK_ICON_SIZE_BUTTON);
      
      ctk_toolbar_reconfigured (toolbar);
      
      _ctk_orientable_set_style_classes (CTK_ORIENTABLE (toolbar));
      ctk_widget_queue_resize (CTK_WIDGET (toolbar));
      g_object_notify (G_OBJECT (toolbar), "orientation");
    }
}

static void
ctk_toolbar_real_style_changed (CtkToolbar     *toolbar,
				CtkToolbarStyle style)
{
  CtkToolbarPrivate *priv = toolbar->priv;

  if (priv->style != style)
    {
      priv->style = style;

      ctk_toolbar_reconfigured (toolbar);
      
      ctk_widget_queue_resize (CTK_WIDGET (toolbar));
      g_object_notify (G_OBJECT (toolbar), "toolbar-style");
    }
}

static void
show_menu (CtkToolbar     *toolbar,
	   GdkEventButton *event)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  CtkRequisition minimum_size;

  rebuild_menu (toolbar);

  ctk_widget_show_all (CTK_WIDGET (priv->menu));

  switch (priv->orientation)
    {
    case CTK_ORIENTATION_HORIZONTAL:
      ctk_widget_get_preferred_size (priv->arrow_button, &minimum_size, NULL);

      g_object_set (priv->menu,
                    "anchor-hints", (GDK_ANCHOR_FLIP_Y |
                                     GDK_ANCHOR_SLIDE |
                                     GDK_ANCHOR_RESIZE),
                    "menu-type-hint", GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU,
                    "rect-anchor-dx", -minimum_size.width,
                    NULL);

      ctk_menu_popup_at_widget (priv->menu,
                                priv->arrow_button,
                                GDK_GRAVITY_SOUTH_EAST,
                                GDK_GRAVITY_NORTH_WEST,
                                (GdkEvent *) event);

      break;

    case CTK_ORIENTATION_VERTICAL:
      g_object_set (priv->menu,
                    "anchor-hints", (GDK_ANCHOR_FLIP_X |
                                     GDK_ANCHOR_SLIDE |
                                     GDK_ANCHOR_RESIZE),
                    NULL);

      ctk_menu_popup_at_widget (priv->menu,
                                priv->arrow_button,
                                GDK_GRAVITY_NORTH_EAST,
                                GDK_GRAVITY_NORTH_WEST,
                                (GdkEvent *) event);

      break;
    }
}

static void
ctk_toolbar_arrow_button_clicked (CtkWidget  *button,
				  CtkToolbar *toolbar)
{
  CtkToolbarPrivate *priv = toolbar->priv;

  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (priv->arrow_button)) &&
      (!priv->menu || !ctk_widget_get_visible (CTK_WIDGET (priv->menu))))
    {
      /* We only get here when the button is clicked with the keyboard,
       * because mouse button presses result in the menu being shown so
       * that priv->menu would be non-NULL and visible.
       */
      show_menu (toolbar, NULL);
      ctk_menu_shell_select_first (CTK_MENU_SHELL (priv->menu), FALSE);
    }
}

static gboolean
ctk_toolbar_arrow_button_press (CtkWidget      *button,
				GdkEventButton *event,
				CtkToolbar     *toolbar)
{
  show_menu (toolbar, event);
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);

  return TRUE;
}

static gboolean
ctk_toolbar_button_press (CtkWidget      *toolbar,
    			  GdkEventButton *event)
{
  if (gdk_event_triggers_context_menu ((GdkEvent *) event))
    {
      gboolean return_value;

      g_signal_emit (toolbar, toolbar_signals[POPUP_CONTEXT_MENU], 0,
		     (int)event->x_root, (int)event->y_root, event->button,
		     &return_value);

      return return_value;
    }

  return FALSE;
}

static gboolean
ctk_toolbar_popup_menu (CtkWidget *toolbar)
{
  gboolean return_value;
  /* This function is the handler for the "popup menu" keybinding,
   * ie., it is called when the user presses Shift F10
   */
  g_signal_emit (toolbar, toolbar_signals[POPUP_CONTEXT_MENU], 0,
		 -1, -1, -1, &return_value);
  
  return return_value;
}

/**
 * ctk_toolbar_new:
 * 
 * Creates a new toolbar. 
 
 * Returns: the newly-created toolbar.
 **/
CtkWidget *
ctk_toolbar_new (void)
{
  CtkToolbar *toolbar;
  
  toolbar = g_object_new (CTK_TYPE_TOOLBAR, NULL);
  
  return CTK_WIDGET (toolbar);
}

/**
 * ctk_toolbar_insert:
 * @toolbar: a #CtkToolbar
 * @item: a #CtkToolItem
 * @pos: the position of the new item
 *
 * Insert a #CtkToolItem into the toolbar at position @pos. If @pos is
 * 0 the item is prepended to the start of the toolbar. If @pos is
 * negative, the item is appended to the end of the toolbar.
 *
 * Since: 2.4
 **/
void
ctk_toolbar_insert (CtkToolbar  *toolbar,
		    CtkToolItem *item,
		    gint         pos)
{
  g_return_if_fail (CTK_IS_TOOLBAR (toolbar));
  g_return_if_fail (CTK_IS_TOOL_ITEM (item));

  pos = MIN (pos, (int)g_list_length (toolbar->priv->content));

  if (pos >= 0)
    pos = logical_to_physical (toolbar, pos);

  toolbar_content_new_tool_item (toolbar, item, FALSE, pos);
}

/**
 * ctk_toolbar_get_item_index:
 * @toolbar: a #CtkToolbar
 * @item: a #CtkToolItem that is a child of @toolbar
 * 
 * Returns the position of @item on the toolbar, starting from 0.
 * It is an error if @item is not a child of the toolbar.
 * 
 * Returns: the position of item on the toolbar.
 * 
 * Since: 2.4
 **/
gint
ctk_toolbar_get_item_index (CtkToolbar  *toolbar,
			    CtkToolItem *item)
{
  CtkToolbarPrivate *priv;
  GList *list;
  int n;
  
  g_return_val_if_fail (CTK_IS_TOOLBAR (toolbar), -1);
  g_return_val_if_fail (CTK_IS_TOOL_ITEM (item), -1);
  g_return_val_if_fail (ctk_widget_get_parent (CTK_WIDGET (item)) == CTK_WIDGET (toolbar), -1);

  priv = toolbar->priv;

  n = 0;
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      CtkWidget *widget;
      
      widget = toolbar_content_get_widget (content);
      
      if (item == CTK_TOOL_ITEM (widget))
	break;
      
      ++n;
    }
  
  return physical_to_logical (toolbar, n);
}

/**
 * ctk_toolbar_set_style:
 * @toolbar: a #CtkToolbar.
 * @style: the new style for @toolbar.
 * 
 * Alters the view of @toolbar to display either icons only, text only, or both.
 **/
void
ctk_toolbar_set_style (CtkToolbar      *toolbar,
		       CtkToolbarStyle  style)
{
  CtkToolbarPrivate *priv;

  g_return_if_fail (CTK_IS_TOOLBAR (toolbar));

  priv = toolbar->priv;

  priv->style_set = TRUE;
  g_signal_emit (toolbar, toolbar_signals[STYLE_CHANGED], 0, style);
}

/**
 * ctk_toolbar_get_style:
 * @toolbar: a #CtkToolbar
 *
 * Retrieves whether the toolbar has text, icons, or both . See
 * ctk_toolbar_set_style().
 
 * Returns: the current style of @toolbar
 **/
CtkToolbarStyle
ctk_toolbar_get_style (CtkToolbar *toolbar)
{
  g_return_val_if_fail (CTK_IS_TOOLBAR (toolbar), DEFAULT_TOOLBAR_STYLE);

  return toolbar->priv->style;
}

/**
 * ctk_toolbar_unset_style:
 * @toolbar: a #CtkToolbar
 * 
 * Unsets a toolbar style set with ctk_toolbar_set_style(), so that
 * user preferences will be used to determine the toolbar style.
 **/
void
ctk_toolbar_unset_style (CtkToolbar *toolbar)
{
  CtkToolbarPrivate *priv;
  CtkToolbarStyle style;
  
  g_return_if_fail (CTK_IS_TOOLBAR (toolbar));

  priv = toolbar->priv;

  if (priv->style_set)
    {
      style = DEFAULT_TOOLBAR_STYLE;

      if (style != priv->style)
	g_signal_emit (toolbar, toolbar_signals[STYLE_CHANGED], 0, style);

      priv->style_set = FALSE;
    }
}

/**
 * ctk_toolbar_get_n_items:
 * @toolbar: a #CtkToolbar
 * 
 * Returns the number of items on the toolbar.
 * 
 * Returns: the number of items on the toolbar
 * 
 * Since: 2.4
 **/
gint
ctk_toolbar_get_n_items (CtkToolbar *toolbar)
{
  CtkToolbarPrivate *priv;

  g_return_val_if_fail (CTK_IS_TOOLBAR (toolbar), -1);

  priv = toolbar->priv;

  return physical_to_logical (toolbar, g_list_length (priv->content));
}

/**
 * ctk_toolbar_get_nth_item:
 * @toolbar: a #CtkToolbar
 * @n: A position on the toolbar
 *
 * Returns the @n'th item on @toolbar, or %NULL if the
 * toolbar does not contain an @n'th item.
 *
 * Returns: (nullable) (transfer none): The @n'th #CtkToolItem on @toolbar,
 *     or %NULL if there isn’t an @n'th item.
 *
 * Since: 2.4
 **/
CtkToolItem *
ctk_toolbar_get_nth_item (CtkToolbar *toolbar,
			  gint        n)
{
  CtkToolbarPrivate *priv;
  ToolbarContent *content;
  gint n_items;
  
  g_return_val_if_fail (CTK_IS_TOOLBAR (toolbar), NULL);

  priv = toolbar->priv;

  n_items = ctk_toolbar_get_n_items (toolbar);
  
  if (n < 0 || n >= n_items)
    return NULL;

  content = g_list_nth_data (priv->content, logical_to_physical (toolbar, n));
  
  g_assert (content);
  g_assert (!toolbar_content_is_placeholder (content));
  
  return CTK_TOOL_ITEM (toolbar_content_get_widget (content));
}

/**
 * ctk_toolbar_get_icon_size:
 * @toolbar: a #CtkToolbar
 *
 * Retrieves the icon size for the toolbar. See ctk_toolbar_set_icon_size().
 *
 * Returns: the current icon size for the icons on the toolbar.
 **/
CtkIconSize
ctk_toolbar_get_icon_size (CtkToolbar *toolbar)
{
  g_return_val_if_fail (CTK_IS_TOOLBAR (toolbar), DEFAULT_ICON_SIZE);

  return toolbar->priv->icon_size;
}

/**
 * ctk_toolbar_get_relief_style:
 * @toolbar: a #CtkToolbar
 * 
 * Returns the relief style of buttons on @toolbar. See
 * ctk_button_set_relief().
 * 
 * Returns: The relief style of buttons on @toolbar.
 * 
 * Since: 2.4
 **/
CtkReliefStyle
ctk_toolbar_get_relief_style (CtkToolbar *toolbar)
{
  g_return_val_if_fail (CTK_IS_TOOLBAR (toolbar), CTK_RELIEF_NONE);
  
  return get_button_relief (toolbar);
}

/**
 * ctk_toolbar_set_show_arrow:
 * @toolbar: a #CtkToolbar
 * @show_arrow: Whether to show an overflow menu
 * 
 * Sets whether to show an overflow menu when @toolbar isn’t allocated enough
 * size to show all of its items. If %TRUE, items which can’t fit in @toolbar,
 * and which have a proxy menu item set by ctk_tool_item_set_proxy_menu_item()
 * or #CtkToolItem::create-menu-proxy, will be available in an overflow menu,
 * which can be opened by an added arrow button. If %FALSE, @toolbar will
 * request enough size to fit all of its child items without any overflow.
 * 
 * Since: 2.4
 **/
void
ctk_toolbar_set_show_arrow (CtkToolbar *toolbar,
			    gboolean    show_arrow)
{
  CtkToolbarPrivate *priv;

  g_return_if_fail (CTK_IS_TOOLBAR (toolbar));

  priv = toolbar->priv;

  show_arrow = show_arrow != FALSE;
  
  if (priv->show_arrow != show_arrow)
    {
      priv->show_arrow = show_arrow;
      
      if (!priv->show_arrow)
	ctk_widget_hide (priv->arrow_button);
      
      ctk_widget_queue_resize (CTK_WIDGET (toolbar));      
      g_object_notify (G_OBJECT (toolbar), "show-arrow");
    }
}

/**
 * ctk_toolbar_get_show_arrow:
 * @toolbar: a #CtkToolbar
 * 
 * Returns whether the toolbar has an overflow menu.
 * See ctk_toolbar_set_show_arrow().
 * 
 * Returns: %TRUE if the toolbar has an overflow menu.
 * 
 * Since: 2.4
 **/
gboolean
ctk_toolbar_get_show_arrow (CtkToolbar *toolbar)
{
  g_return_val_if_fail (CTK_IS_TOOLBAR (toolbar), FALSE);

  return toolbar->priv->show_arrow;
}

/**
 * ctk_toolbar_get_drop_index:
 * @toolbar: a #CtkToolbar
 * @x: x coordinate of a point on the toolbar
 * @y: y coordinate of a point on the toolbar
 *
 * Returns the position corresponding to the indicated point on
 * @toolbar. This is useful when dragging items to the toolbar:
 * this function returns the position a new item should be
 * inserted.
 *
 * @x and @y are in @toolbar coordinates.
 * 
 * Returns: The position corresponding to the point (@x, @y) on the toolbar.
 * 
 * Since: 2.4
 **/
gint
ctk_toolbar_get_drop_index (CtkToolbar *toolbar,
			    gint        x,
			    gint        y)
{
  g_return_val_if_fail (CTK_IS_TOOLBAR (toolbar), -1);
  
  return physical_to_logical (toolbar, find_drop_index (toolbar, x, y));
}

static void
ctk_toolbar_dispose (GObject *object)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (object);
  CtkToolbarPrivate *priv = toolbar->priv;

  if (priv->arrow_button)
    {
      ctk_widget_unparent (priv->arrow_button);
      priv->arrow_button = NULL;
    }

  if (priv->menu)
    {
      g_signal_handlers_disconnect_by_func (priv->menu,
                                            menu_deactivated, toolbar);
      ctk_widget_destroy (CTK_WIDGET (priv->menu));
      priv->menu = NULL;
    }

  if (priv->settings_connection > 0)
    {
      g_signal_handler_disconnect (priv->settings, priv->settings_connection);
      priv->settings_connection = 0;
    }

  g_clear_object (&priv->settings);

 G_OBJECT_CLASS (ctk_toolbar_parent_class)->dispose (object);
}

static void
ctk_toolbar_finalize (GObject *object)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (object);
  CtkToolbarPrivate *priv = toolbar->priv;

  g_list_free_full (priv->content, (GDestroyNotify)toolbar_content_free);

  g_timer_destroy (priv->timer);

  if (priv->idle_id)
    g_source_remove (priv->idle_id);

  g_clear_object (&priv->gadget);

  G_OBJECT_CLASS (ctk_toolbar_parent_class)->finalize (object);
}

/**
 * ctk_toolbar_set_icon_size:
 * @toolbar: A #CtkToolbar
 * @icon_size: The #CtkIconSize that stock icons in the toolbar shall have.
 *
 * This function sets the size of stock icons in the toolbar. You
 * can call it both before you add the icons and after they’ve been
 * added. The size you set will override user preferences for the default
 * icon size.
 * 
 * This should only be used for special-purpose toolbars, normal
 * application toolbars should respect the user preferences for the
 * size of icons.
 **/
void
ctk_toolbar_set_icon_size (CtkToolbar  *toolbar,
			   CtkIconSize  icon_size)
{
  CtkToolbarPrivate *priv;

  g_return_if_fail (CTK_IS_TOOLBAR (toolbar));
  g_return_if_fail (icon_size != CTK_ICON_SIZE_INVALID);

  priv = toolbar->priv;

  if (!priv->icon_size_set)
    {
      priv->icon_size_set = TRUE;
      g_object_notify (G_OBJECT (toolbar), "icon-size-set");
    }

  if (priv->icon_size == icon_size)
    return;

  priv->icon_size = icon_size;
  g_object_notify (G_OBJECT (toolbar), "icon-size");
  
  ctk_toolbar_reconfigured (toolbar);
  
  ctk_widget_queue_resize (CTK_WIDGET (toolbar));
}

/**
 * ctk_toolbar_unset_icon_size:
 * @toolbar: a #CtkToolbar
 * 
 * Unsets toolbar icon size set with ctk_toolbar_set_icon_size(), so that
 * user preferences will be used to determine the icon size.
 **/
void
ctk_toolbar_unset_icon_size (CtkToolbar *toolbar)
{
  CtkToolbarPrivate *priv;
  CtkIconSize size;

  g_return_if_fail (CTK_IS_TOOLBAR (toolbar));

  priv = toolbar->priv;

  if (priv->icon_size_set)
    {
      size = DEFAULT_ICON_SIZE;

      if (size != priv->icon_size)
	{
	  ctk_toolbar_set_icon_size (toolbar, size);
	  g_object_notify (G_OBJECT (toolbar), "icon-size");	  
	}

      priv->icon_size_set = FALSE;
      g_object_notify (G_OBJECT (toolbar), "icon-size-set");      
    }
}

/*
 * ToolbarContent methods
 */
typedef enum {
  UNKNOWN,
  YES,
  NO
} TriState;

struct _ToolbarContent
{
  ItemState      state;

  CtkToolItem   *item;
  CtkAllocation  allocation;
  CtkAllocation  start_allocation;
  CtkAllocation  goal_allocation;
  guint          is_placeholder : 1;
  guint          disappearing : 1;
  guint          has_menu : 2;
};

static ToolbarContent *
toolbar_content_new_tool_item (CtkToolbar  *toolbar,
			       CtkToolItem *item,
			       gboolean     is_placeholder,
			       gint	    pos)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  ToolbarContent *content, *previous;

  content = g_slice_new0 (ToolbarContent);
  
  content->state = NOT_ALLOCATED;
  content->item = item;
  content->is_placeholder = is_placeholder;

  previous = pos > 0 ? g_list_nth_data (priv->content, -1) : NULL;
  priv->content = g_list_insert (priv->content, content, pos);

  if (ctk_widget_get_direction (CTK_WIDGET (toolbar)) == CTK_TEXT_DIR_RTL)
    ctk_css_node_insert_after (ctk_widget_get_css_node (CTK_WIDGET (toolbar)),
                               ctk_widget_get_css_node (CTK_WIDGET (item)),
                               previous ? ctk_widget_get_css_node (CTK_WIDGET (previous->item)) : NULL);
  else
    ctk_css_node_insert_before (ctk_widget_get_css_node (CTK_WIDGET (toolbar)),
                                ctk_widget_get_css_node (CTK_WIDGET (item)),
                                previous ? ctk_widget_get_css_node (CTK_WIDGET (previous->item)) : NULL);

  ctk_widget_set_parent (CTK_WIDGET (item), CTK_WIDGET (toolbar));

  if (!is_placeholder)
    {
      priv->num_children++;

      ctk_toolbar_stop_sliding (toolbar);
    }

  ctk_widget_queue_resize (CTK_WIDGET (toolbar));
  priv->need_rebuild = TRUE;
  
  return content;
}

static void
toolbar_content_remove (ToolbarContent *content,
			CtkToolbar     *toolbar)
{
  CtkToolbarPrivate *priv = toolbar->priv;

  ctk_widget_unparent (CTK_WIDGET (content->item));

  priv->content = g_list_remove (priv->content, content);

  if (!toolbar_content_is_placeholder (content))
    priv->num_children--;

  ctk_widget_queue_resize (CTK_WIDGET (toolbar));
  priv->need_rebuild = TRUE;
}

static void
toolbar_content_free (ToolbarContent *content)
{
  g_slice_free (ToolbarContent, content);
}

static gint
calculate_max_homogeneous_pixels (CtkWidget *widget)
{
  PangoContext *context;
  PangoFontMetrics *metrics;
  gint char_width;
  
  context = ctk_widget_get_pango_context (widget);

  metrics = pango_context_get_metrics (context,
                                       pango_context_get_font_description (context),
				       pango_context_get_language (context));
  char_width = pango_font_metrics_get_approximate_char_width (metrics);
  pango_font_metrics_unref (metrics);
  
  return PANGO_PIXELS (MAX_HOMOGENEOUS_N_CHARS * char_width);
}

static void
toolbar_content_draw (ToolbarContent *content,
                      CtkContainer   *container,
                      cairo_t        *cr)
{
  CtkWidget *widget;

  if (content->is_placeholder)
    return;
  
  widget = CTK_WIDGET (content->item);

  if (widget)
    ctk_container_propagate_draw (container, widget, cr);
}

static gboolean
toolbar_content_visible (ToolbarContent *content,
			 CtkToolbar     *toolbar)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  CtkToolItem *item;

  item = content->item;

  if (!ctk_widget_get_visible (CTK_WIDGET (item)))
    return FALSE;

  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL &&
      ctk_tool_item_get_visible_horizontal (item))
    return TRUE;

  if (priv->orientation == CTK_ORIENTATION_VERTICAL &&
      ctk_tool_item_get_visible_vertical (item))
    return TRUE;
      
  return FALSE;
}

static void
toolbar_content_size_request (ToolbarContent *content,
			      CtkToolbar     *toolbar,
			      CtkRequisition *requisition)
{
  ctk_widget_get_preferred_size (CTK_WIDGET (content->item),
                                 requisition, NULL);
  if (content->is_placeholder &&
      content->disappearing)
    {
      requisition->width = 0;
      requisition->height = 0;
    }
}

static gboolean
toolbar_content_is_homogeneous (ToolbarContent *content,
				CtkToolbar     *toolbar)
{
  CtkToolbarPrivate *priv = toolbar->priv;
  CtkRequisition requisition;
  gboolean result;
  
  if (priv->max_homogeneous_pixels < 0)
    {
      priv->max_homogeneous_pixels =
	calculate_max_homogeneous_pixels (CTK_WIDGET (toolbar));
    }
  
  toolbar_content_size_request (content, toolbar, &requisition);
  
  if (requisition.width > priv->max_homogeneous_pixels)
    return FALSE;

  result = ctk_tool_item_get_homogeneous (content->item) &&
           !CTK_IS_SEPARATOR_TOOL_ITEM (content->item);

  if (ctk_tool_item_get_is_important (content->item) &&
      priv->style == CTK_TOOLBAR_BOTH_HORIZ &&
      priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      result = FALSE;
    }

  return result;
}

static gboolean
toolbar_content_is_placeholder (ToolbarContent *content)
{
  if (content->is_placeholder)
    return TRUE;
  
  return FALSE;
}

static gboolean
toolbar_content_disappearing (ToolbarContent *content)
{
  if (content->disappearing)
    return TRUE;
  
  return FALSE;
}

static ItemState
toolbar_content_get_state (ToolbarContent *content)
{
  return content->state;
}

static gboolean
toolbar_content_child_visible (ToolbarContent *content)
{
  return ctk_widget_get_child_visible (CTK_WIDGET (content->item));
}

static void
toolbar_content_get_goal_allocation (ToolbarContent *content,
				     CtkAllocation  *allocation)
{
  *allocation = content->goal_allocation;
}

static void
toolbar_content_get_allocation (ToolbarContent *content,
				CtkAllocation  *allocation)
{
  *allocation = content->allocation;
}

static void
toolbar_content_set_start_allocation (ToolbarContent *content,
				      CtkAllocation  *allocation)
{
  content->start_allocation = *allocation;
}

static gboolean
toolbar_content_get_expand (ToolbarContent *content)
{
  if (!content->disappearing &&
      ctk_tool_item_get_expand (content->item))
    return TRUE;

  return FALSE;
}

static void
toolbar_content_set_goal_allocation (ToolbarContent *content,
				     CtkAllocation  *allocation)
{
  content->goal_allocation = *allocation;
}

static void
toolbar_content_set_child_visible (ToolbarContent *content,
				   CtkToolbar     *toolbar,
				   gboolean        visible)
{
  ctk_widget_set_child_visible (CTK_WIDGET (content->item),
                                visible);
}

static void
toolbar_content_get_start_allocation (ToolbarContent *content,
				      CtkAllocation  *start_allocation)
{
  *start_allocation = content->start_allocation;
}

static void
toolbar_content_size_allocate (ToolbarContent *content,
			       CtkAllocation  *allocation)
{
  content->allocation = *allocation;
  ctk_widget_size_allocate (CTK_WIDGET (content->item),
                            allocation);
}

static void
toolbar_content_set_state (ToolbarContent *content,
			   ItemState       state)
{
  content->state = state;
}

static CtkWidget *
toolbar_content_get_widget (ToolbarContent *content)
{
  return CTK_WIDGET (content->item);
}


static void
toolbar_content_set_disappearing (ToolbarContent *content,
				  gboolean        disappearing)
{
  content->disappearing = disappearing;
}

static void
toolbar_content_set_size_request (ToolbarContent *content,
				  gint            width,
				  gint            height)
{
  ctk_widget_set_size_request (CTK_WIDGET (content->item),
                               width, height);
}

static void
toolbar_content_toolbar_reconfigured (ToolbarContent *content,
				      CtkToolbar     *toolbar)
{
  ctk_tool_item_toolbar_reconfigured (content->item);
}

static CtkWidget *
toolbar_content_retrieve_menu_item (ToolbarContent *content)
{
  return ctk_tool_item_retrieve_proxy_menu_item (content->item);
}

static gboolean
toolbar_content_has_proxy_menu_item (ToolbarContent *content)
{
  CtkWidget *menu_item;

  if (content->has_menu == YES)
    return TRUE;
  else if (content->has_menu == NO)
    return FALSE;

  menu_item = toolbar_content_retrieve_menu_item (content);

  content->has_menu = menu_item? YES : NO;

  return menu_item != NULL;
}

static void
toolbar_content_set_unknown_menu_status (ToolbarContent *content)
{
  content->has_menu = UNKNOWN;
}

static gboolean
toolbar_content_is_separator (ToolbarContent *content)
{
  return CTK_IS_SEPARATOR_TOOL_ITEM (content->item);
}

static void
toolbar_content_set_expand (ToolbarContent *content,
                            gboolean        expand)
{
  ctk_tool_item_set_expand (content->item, expand);
}

static void
toolbar_content_show_all (ToolbarContent  *content)
{
  CtkWidget *widget;
  
  widget = toolbar_content_get_widget (content);
  if (widget)
    ctk_widget_show_all (widget);
}

/*
 * Getters
 */
static CtkReliefStyle
get_button_relief (CtkToolbar *toolbar)
{
  CtkReliefStyle button_relief = CTK_RELIEF_NORMAL;

  ctk_widget_style_get (CTK_WIDGET (toolbar),
                        "button-relief", &button_relief,
                        NULL);
  
  return button_relief;
}

static gint
get_max_child_expand (CtkToolbar *toolbar)
{
  gint mexpand = G_MAXINT;

  ctk_widget_style_get (CTK_WIDGET (toolbar),
                        "max-child-expand", &mexpand,
                        NULL);
  return mexpand;
}

/* CTK+ internal methods */
gchar *
_ctk_toolbar_elide_underscores (const gchar *original)
{
  gchar *q, *result;
  const gchar *p, *end;
  gsize len;
  gboolean last_underscore;
  
  if (!original)
    return NULL;

  len = strlen (original);
  q = result = g_malloc (len + 1);
  last_underscore = FALSE;
  
  end = original + len;
  for (p = original; p < end; p++)
    {
      if (!last_underscore && *p == '_')
	last_underscore = TRUE;
      else
	{
	  last_underscore = FALSE;
	  if (original + 2 <= p && p + 1 <= end && 
              p[-2] == '(' && p[-1] == '_' && p[0] != '_' && p[1] == ')')
	    {
	      q--;
	      *q = '\0';
	      p++;
	    }
	  else
	    *q++ = *p;
	}
    }

  if (last_underscore)
    *q++ = '_';
  
  *q = '\0';
  
  return result;
}

static CtkIconSize
toolbar_get_icon_size (CtkToolShell *shell)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (shell);
  CtkToolbarPrivate *priv = toolbar->priv;

  return priv->icon_size;
}

static CtkOrientation
toolbar_get_orientation (CtkToolShell *shell)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (shell);
  CtkToolbarPrivate *priv = toolbar->priv;

  return priv->orientation;
}

static CtkToolbarStyle
toolbar_get_style (CtkToolShell *shell)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (shell);
  CtkToolbarPrivate *priv = toolbar->priv;

  return priv->style;
}

static CtkReliefStyle
toolbar_get_relief_style (CtkToolShell *shell)
{
  return get_button_relief (CTK_TOOLBAR (shell));
}

static void
toolbar_rebuild_menu (CtkToolShell *shell)
{
  CtkToolbar *toolbar = CTK_TOOLBAR (shell);
  CtkToolbarPrivate *priv = toolbar->priv;
  GList *list;

  priv->need_rebuild = TRUE;

  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;

      toolbar_content_set_unknown_menu_status (content);
    }
  
  ctk_widget_queue_resize (CTK_WIDGET (shell));
}

static void
ctk_toolbar_direction_changed (CtkWidget        *widget,
                               CtkTextDirection  previous_direction)
{
  CTK_WIDGET_CLASS (ctk_toolbar_parent_class)->direction_changed (widget, previous_direction);

  ctk_css_node_reverse_children (ctk_widget_get_css_node (widget));
}

