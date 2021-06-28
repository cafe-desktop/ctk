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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#include <stdarg.h>
#include <string.h>
#include <locale.h>
#include <math.h>

#include <gobject/gvaluecollector.h>
#include <gobject/gobjectnotifyqueue.c>
#include <cairo-gobject.h>

#include "ctkcontainer.h"
#include "ctkaccelmapprivate.h"
#include "ctkaccelgroupprivate.h"
#include "ctkclipboard.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkcssnumbervalueprivate.h"
#include "ctkcssshadowsvalueprivate.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctkselectionprivate.h"
#include "ctksettingsprivate.h"
#include "ctksizegroup-private.h"
#include "ctkwidget.h"
#include "ctkwidgetprivate.h"
#include "ctkwindowprivate.h"
#include "ctkcontainerprivate.h"
#include "ctkbindings.h"
#include "ctkprivate.h"
#include "ctkaccessible.h"
#include "ctktooltipprivate.h"
#include "ctkinvisible.h"
#include "ctkbuildable.h"
#include "ctkbuilderprivate.h"
#include "ctksizerequest.h"
#include "ctkstylecontextprivate.h"
#include "ctkcssprovider.h"
#include "ctkcsswidgetnodeprivate.h"
#include "ctkmodifierstyle.h"
#include "ctkversion.h"
#include "ctkdebug.h"
#include "ctkplug.h"
#include "ctktypebuiltins.h"
#include "a11y/ctkwidgetaccessible.h"
#include "ctkapplicationprivate.h"
#include "ctkgestureprivate.h"
#include "ctkwidgetpathprivate.h"

/* for the use of round() */
#include "fallback-c89.c"

/**
 * SECTION:ctkwidget
 * @Short_description: Base class for all widgets
 * @Title: CtkWidget
 *
 * CtkWidget is the base class all widgets in CTK+ derive from. It manages the
 * widget lifecycle, states and style.
 *
 * # Height-for-width Geometry Management # {#geometry-management}
 *
 * CTK+ uses a height-for-width (and width-for-height) geometry management
 * system. Height-for-width means that a widget can change how much
 * vertical space it needs, depending on the amount of horizontal space
 * that it is given (and similar for width-for-height). The most common
 * example is a label that reflows to fill up the available width, wraps
 * to fewer lines, and therefore needs less height.
 *
 * Height-for-width geometry management is implemented in CTK+ by way
 * of five virtual methods:
 *
 * - #CtkWidgetClass.get_request_mode()
 * - #CtkWidgetClass.get_preferred_width()
 * - #CtkWidgetClass.get_preferred_height()
 * - #CtkWidgetClass.get_preferred_height_for_width()
 * - #CtkWidgetClass.get_preferred_width_for_height()
 * - #CtkWidgetClass.get_preferred_height_and_baseline_for_width()
 *
 * There are some important things to keep in mind when implementing
 * height-for-width and when using it in container implementations.
 *
 * The geometry management system will query a widget hierarchy in
 * only one orientation at a time. When widgets are initially queried
 * for their minimum sizes it is generally done in two initial passes
 * in the #CtkSizeRequestMode chosen by the toplevel.
 *
 * For example, when queried in the normal
 * %CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH mode:
 * First, the default minimum and natural width for each widget
 * in the interface will be computed using ctk_widget_get_preferred_width().
 * Because the preferred widths for each container depend on the preferred
 * widths of their children, this information propagates up the hierarchy,
 * and finally a minimum and natural width is determined for the entire
 * toplevel. Next, the toplevel will use the minimum width to query for the
 * minimum height contextual to that width using
 * ctk_widget_get_preferred_height_for_width(), which will also be a highly
 * recursive operation. The minimum height for the minimum width is normally
 * used to set the minimum size constraint on the toplevel
 * (unless ctk_window_set_geometry_hints() is explicitly used instead).
 *
 * After the toplevel window has initially requested its size in both
 * dimensions it can go on to allocate itself a reasonable size (or a size
 * previously specified with ctk_window_set_default_size()). During the
 * recursive allocation process it’s important to note that request cycles
 * will be recursively executed while container widgets allocate their children.
 * Each container widget, once allocated a size, will go on to first share the
 * space in one orientation among its children and then request each child's
 * height for its target allocated width or its width for allocated height,
 * depending. In this way a #CtkWidget will typically be requested its size
 * a number of times before actually being allocated a size. The size a
 * widget is finally allocated can of course differ from the size it has
 * requested. For this reason, #CtkWidget caches a  small number of results
 * to avoid re-querying for the same sizes in one allocation cycle.
 *
 * See
 * [CtkContainer’s geometry management section][container-geometry-management]
 * to learn more about how height-for-width allocations are performed
 * by container widgets.
 *
 * If a widget does move content around to intelligently use up the
 * allocated size then it must support the request in both
 * #CtkSizeRequestModes even if the widget in question only
 * trades sizes in a single orientation.
 *
 * For instance, a #CtkLabel that does height-for-width word wrapping
 * will not expect to have #CtkWidgetClass.get_preferred_height() called
 * because that call is specific to a width-for-height request. In this
 * case the label must return the height required for its own minimum
 * possible width. By following this rule any widget that handles
 * height-for-width or width-for-height requests will always be allocated
 * at least enough space to fit its own content.
 *
 * Here are some examples of how a %CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH widget
 * generally deals with width-for-height requests, for #CtkWidgetClass.get_preferred_height()
 * it will do:
 *
 * |[<!-- language="C" -->
 * static void
 * foo_widget_get_preferred_height (CtkWidget *widget,
 *                                  gint *min_height,
 *                                  gint *nat_height)
 * {
 *    if (i_am_in_height_for_width_mode)
 *      {
 *        gint min_width, nat_width;
 *
 *        CTK_WIDGET_GET_CLASS (widget)->get_preferred_width (widget,
 *                                                            &min_width,
 *                                                            &nat_width);
 *        CTK_WIDGET_GET_CLASS (widget)->get_preferred_height_for_width
 *                                                           (widget,
 *                                                            min_width,
 *                                                            min_height,
 *                                                            nat_height);
 *      }
 *    else
 *      {
 *         ... some widgets do both. For instance, if a CtkLabel is
 *         rotated to 90 degrees it will return the minimum and
 *         natural height for the rotated label here.
 *      }
 * }
 * ]|
 *
 * And in #CtkWidgetClass.get_preferred_width_for_height() it will simply return
 * the minimum and natural width:
 * |[<!-- language="C" -->
 * static void
 * foo_widget_get_preferred_width_for_height (CtkWidget *widget,
 *                                            gint for_height,
 *                                            gint *min_width,
 *                                            gint *nat_width)
 * {
 *    if (i_am_in_height_for_width_mode)
 *      {
 *        CTK_WIDGET_GET_CLASS (widget)->get_preferred_width (widget,
 *                                                            min_width,
 *                                                            nat_width);
 *      }
 *    else
 *      {
 *         ... again if a widget is sometimes operating in
 *         width-for-height mode (like a rotated CtkLabel) it can go
 *         ahead and do its real width for height calculation here.
 *      }
 * }
 * ]|
 *
 * Often a widget needs to get its own request during size request or
 * allocation. For example, when computing height it may need to also
 * compute width. Or when deciding how to use an allocation, the widget
 * may need to know its natural size. In these cases, the widget should
 * be careful to call its virtual methods directly, like this:
 *
 * |[<!-- language="C" -->
 * CTK_WIDGET_GET_CLASS(widget)->get_preferred_width (widget,
 *                                                    &min,
 *                                                    &natural);
 * ]|
 *
 * It will not work to use the wrapper functions, such as
 * ctk_widget_get_preferred_width() inside your own size request
 * implementation. These return a request adjusted by #CtkSizeGroup
 * and by the #CtkWidgetClass.adjust_size_request() virtual method. If a
 * widget used the wrappers inside its virtual method implementations,
 * then the adjustments (such as widget margins) would be applied
 * twice. CTK+ therefore does not allow this and will warn if you try
 * to do it.
 *
 * Of course if you are getting the size request for
 * another widget, such as a child of a
 * container, you must use the wrapper APIs.
 * Otherwise, you would not properly consider widget margins,
 * #CtkSizeGroup, and so forth.
 *
 * Since 3.10 CTK+ also supports baseline vertical alignment of widgets. This
 * means that widgets are positioned such that the typographical baseline of
 * widgets in the same row are aligned. This happens if a widget supports baselines,
 * has a vertical alignment of %CTK_ALIGN_BASELINE, and is inside a container
 * that supports baselines and has a natural “row” that it aligns to the baseline,
 * or a baseline assigned to it by the grandparent.
 *
 * Baseline alignment support for a widget is done by the #CtkWidgetClass.get_preferred_height_and_baseline_for_width()
 * virtual function. It allows you to report a baseline in combination with the
 * minimum and natural height. If there is no baseline you can return -1 to indicate
 * this. The default implementation of this virtual function calls into the
 * #CtkWidgetClass.get_preferred_height() and #CtkWidgetClass.get_preferred_height_for_width(),
 * so if baselines are not supported it doesn’t need to be implemented.
 *
 * If a widget ends up baseline aligned it will be allocated all the space in the parent
 * as if it was %CTK_ALIGN_FILL, but the selected baseline can be found via ctk_widget_get_allocated_baseline().
 * If this has a value other than -1 you need to align the widget such that the baseline
 * appears at the position.
 *
 * # Style Properties
 *
 * #CtkWidget introduces “style
 * properties” - these are basically object properties that are stored
 * not on the object, but in the style object associated to the widget. Style
 * properties are set in [resource files][ctk3-Resource-Files].
 * This mechanism is used for configuring such things as the location of the
 * scrollbar arrows through the theme, giving theme authors more control over the
 * look of applications without the need to write a theme engine in C.
 *
 * Use ctk_widget_class_install_style_property() to install style properties for
 * a widget class, ctk_widget_class_find_style_property() or
 * ctk_widget_class_list_style_properties() to get information about existing
 * style properties and ctk_widget_style_get_property(), ctk_widget_style_get() or
 * ctk_widget_style_get_valist() to obtain the value of a style property.
 *
 * # CtkWidget as CtkBuildable
 *
 * The CtkWidget implementation of the CtkBuildable interface supports a
 * custom <accelerator> element, which has attributes named ”key”, ”modifiers”
 * and ”signal” and allows to specify accelerators.
 *
 * An example of a UI definition fragment specifying an accelerator:
 * |[
 * <object class="CtkButton">
 *   <accelerator key="q" modifiers="CDK_CONTROL_MASK" signal="clicked"/>
 * </object>
 * ]|
 *
 * In addition to accelerators, CtkWidget also support a custom <accessible>
 * element, which supports actions and relations. Properties on the accessible
 * implementation of an object can be set by accessing the internal child
 * “accessible” of a #CtkWidget.
 *
 * An example of a UI definition fragment specifying an accessible:
 * |[
 * <object class="CtkLabel" id="label1"/>
 *   <property name="label">I am a Label for a Button</property>
 * </object>
 * <object class="CtkButton" id="button1">
 *   <accessibility>
 *     <action action_name="click" translatable="yes">Click the button.</action>
 *     <relation target="label1" type="labelled-by"/>
 *   </accessibility>
 *   <child internal-child="accessible">
 *     <object class="AtkObject" id="a11y-button1">
 *       <property name="accessible-name">Clickable Button</property>
 *     </object>
 *   </child>
 * </object>
 * ]|
 *
 * Finally, CtkWidget allows style information such as style classes to
 * be associated with widgets, using the custom <style> element:
 * |[
 * <object class="CtkButton" id="button1">
 *   <style>
 *     <class name="my-special-button-class"/>
 *     <class name="dark-button"/>
 *   </style>
 * </object>
 * ]|
 *
 * # Building composite widgets from template XML ## {#composite-templates}
 *
 * CtkWidget exposes some facilities to automate the procedure
 * of creating composite widgets using #CtkBuilder interface description
 * language.
 *
 * To create composite widgets with #CtkBuilder XML, one must associate
 * the interface description with the widget class at class initialization
 * time using ctk_widget_class_set_template().
 *
 * The interface description semantics expected in composite template descriptions
 * is slightly different from regular #CtkBuilder XML.
 *
 * Unlike regular interface descriptions, ctk_widget_class_set_template() will
 * expect a <template> tag as a direct child of the toplevel <interface>
 * tag. The <template> tag must specify the “class” attribute which must be
 * the type name of the widget. Optionally, the “parent” attribute may be
 * specified to specify the direct parent type of the widget type, this is
 * ignored by the CtkBuilder but required for Glade to introspect what kind
 * of properties and internal children exist for a given type when the actual
 * type does not exist.
 *
 * The XML which is contained inside the <template> tag behaves as if it were
 * added to the <object> tag defining @widget itself. You may set properties
 * on @widget by inserting <property> tags into the <template> tag, and also
 * add <child> tags to add children and extend @widget in the normal way you
 * would with <object> tags.
 *
 * Additionally, <object> tags can also be added before and after the initial
 * <template> tag in the normal way, allowing one to define auxiliary objects
 * which might be referenced by other widgets declared as children of the
 * <template> tag.
 *
 * An example of a CtkBuilder Template Definition:
 * |[
 * <interface>
 *   <template class="FooWidget" parent="CtkBox">
 *     <property name="orientation">CTK_ORIENTATION_HORIZONTAL</property>
 *     <property name="spacing">4</property>
 *     <child>
 *       <object class="CtkButton" id="hello_button">
 *         <property name="label">Hello World</property>
 *         <signal name="clicked" handler="hello_button_clicked" object="FooWidget" swapped="yes"/>
 *       </object>
 *     </child>
 *     <child>
 *       <object class="CtkButton" id="goodbye_button">
 *         <property name="label">Goodbye World</property>
 *       </object>
 *     </child>
 *   </template>
 * </interface>
 * ]|
 *
 * Typically, you'll place the template fragment into a file that is
 * bundled with your project, using #GResource. In order to load the
 * template, you need to call ctk_widget_class_set_template_from_resource()
 * from the class initialization of your #CtkWidget type:
 *
 * |[<!-- language="C" -->
 * static void
 * foo_widget_class_init (FooWidgetClass *klass)
 * {
 *   // ...
 *
 *   ctk_widget_class_set_template_from_resource (CTK_WIDGET_CLASS (klass),
 *                                                "/com/example/ui/foowidget.ui");
 * }
 * ]|
 *
 * You will also need to call ctk_widget_init_template() from the instance
 * initialization function:
 *
 * |[<!-- language="C" -->
 * static void
 * foo_widget_init (FooWidget *self)
 * {
 *   // ...
 *   ctk_widget_init_template (CTK_WIDGET (self));
 * }
 * ]|
 *
 * You can access widgets defined in the template using the
 * ctk_widget_get_template_child() function, but you will typically declare
 * a pointer in the instance private data structure of your type using the same
 * name as the widget in the template definition, and call
 * ctk_widget_class_bind_template_child_private() with that name, e.g.
 *
 * |[<!-- language="C" -->
 * typedef struct {
 *   CtkWidget *hello_button;
 *   CtkWidget *goodbye_button;
 * } FooWidgetPrivate;
 *
 * G_DEFINE_TYPE_WITH_PRIVATE (FooWidget, foo_widget, CTK_TYPE_BOX)
 *
 * static void
 * foo_widget_class_init (FooWidgetClass *klass)
 * {
 *   // ...
 *   ctk_widget_class_set_template_from_resource (CTK_WIDGET_CLASS (klass),
 *                                                "/com/example/ui/foowidget.ui");
 *   ctk_widget_class_bind_template_child_private (CTK_WIDGET_CLASS (klass),
 *                                                 FooWidget, hello_button);
 *   ctk_widget_class_bind_template_child_private (CTK_WIDGET_CLASS (klass),
 *                                                 FooWidget, goodbye_button);
 * }
 *
 * static void
 * foo_widget_init (FooWidget *widget)
 * {
 *
 * }
 * ]|
 *
 * You can also use ctk_widget_class_bind_template_callback() to connect a signal
 * callback defined in the template with a function visible in the scope of the
 * class, e.g.
 *
 * |[<!-- language="C" -->
 * // the signal handler has the instance and user data swapped
 * // because of the swapped="yes" attribute in the template XML
 * static void
 * hello_button_clicked (FooWidget *self,
 *                       CtkButton *button)
 * {
 *   g_print ("Hello, world!\n");
 * }
 *
 * static void
 * foo_widget_class_init (FooWidgetClass *klass)
 * {
 *   // ...
 *   ctk_widget_class_set_template_from_resource (CTK_WIDGET_CLASS (klass),
 *                                                "/com/example/ui/foowidget.ui");
 *   ctk_widget_class_bind_template_callback (CTK_WIDGET_CLASS (klass), hello_button_clicked);
 * }
 * ]|
 */

#define CTK_STATE_FLAGS_DO_PROPAGATE (CTK_STATE_FLAG_INSENSITIVE|CTK_STATE_FLAG_BACKDROP)

#define WIDGET_CLASS(w)	 CTK_WIDGET_GET_CLASS (w)

typedef struct {
  gchar               *name;           /* Name of the template automatic child */
  gboolean             internal_child; /* Whether the automatic widget should be exported as an <internal-child> */
  gssize               offset;         /* Instance private data offset where to set the automatic child (or 0) */
} AutomaticChildClass;

typedef struct {
  gchar     *callback_name;
  GCallback  callback_symbol;
} CallbackSymbol;

typedef struct {
  GBytes               *data;
  GSList               *children;
  GSList               *callbacks;
  CtkBuilderConnectFunc connect_func;
  gpointer              connect_data;
  GDestroyNotify        destroy_notify;
} CtkWidgetTemplate;

typedef struct {
  CtkEventController *controller;
  guint grab_notify_id;
  guint sequence_state_changed_id;
} EventControllerData;

struct _CtkWidgetClassPrivate
{
  CtkWidgetTemplate *template;
  GType accessible_type;
  AtkRole accessible_role;
  const char *css_name;
};

enum {
  DESTROY,
  SHOW,
  HIDE,
  MAP,
  UNMAP,
  REALIZE,
  UNREALIZE,
  SIZE_ALLOCATE,
  STATE_FLAGS_CHANGED,
  STATE_CHANGED,
  PARENT_SET,
  HIERARCHY_CHANGED,
  STYLE_SET,
  DIRECTION_CHANGED,
  GRAB_NOTIFY,
  CHILD_NOTIFY,
  DRAW,
  MNEMONIC_ACTIVATE,
  GRAB_FOCUS,
  FOCUS,
  MOVE_FOCUS,
  KEYNAV_FAILED,
  EVENT,
  EVENT_AFTER,
  BUTTON_PRESS_EVENT,
  BUTTON_RELEASE_EVENT,
  SCROLL_EVENT,
  MOTION_NOTIFY_EVENT,
  DELETE_EVENT,
  DESTROY_EVENT,
  KEY_PRESS_EVENT,
  KEY_RELEASE_EVENT,
  ENTER_NOTIFY_EVENT,
  LEAVE_NOTIFY_EVENT,
  CONFIGURE_EVENT,
  FOCUS_IN_EVENT,
  FOCUS_OUT_EVENT,
  MAP_EVENT,
  UNMAP_EVENT,
  PROPERTY_NOTIFY_EVENT,
  SELECTION_CLEAR_EVENT,
  SELECTION_REQUEST_EVENT,
  SELECTION_NOTIFY_EVENT,
  SELECTION_GET,
  SELECTION_RECEIVED,
  PROXIMITY_IN_EVENT,
  PROXIMITY_OUT_EVENT,
  VISIBILITY_NOTIFY_EVENT,
  WINDOW_STATE_EVENT,
  DAMAGE_EVENT,
  GRAB_BROKEN_EVENT,
  DRAG_BEGIN,
  DRAG_END,
  DRAG_DATA_DELETE,
  DRAG_LEAVE,
  DRAG_MOTION,
  DRAG_DROP,
  DRAG_DATA_GET,
  DRAG_DATA_RECEIVED,
  POPUP_MENU,
  SHOW_HELP,
  ACCEL_CLOSURES_CHANGED,
  SCREEN_CHANGED,
  CAN_ACTIVATE_ACCEL,
  COMPOSITED_CHANGED,
  QUERY_TOOLTIP,
  DRAG_FAILED,
  STYLE_UPDATED,
  TOUCH_EVENT,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_NAME,
  PROP_PARENT,
  PROP_WIDTH_REQUEST,
  PROP_HEIGHT_REQUEST,
  PROP_VISIBLE,
  PROP_SENSITIVE,
  PROP_APP_PAINTABLE,
  PROP_CAN_FOCUS,
  PROP_HAS_FOCUS,
  PROP_IS_FOCUS,
  PROP_FOCUS_ON_CLICK,
  PROP_CAN_DEFAULT,
  PROP_HAS_DEFAULT,
  PROP_RECEIVES_DEFAULT,
  PROP_COMPOSITE_CHILD,
  PROP_STYLE,
  PROP_EVENTS,
  PROP_NO_SHOW_ALL,
  PROP_HAS_TOOLTIP,
  PROP_TOOLTIP_MARKUP,
  PROP_TOOLTIP_TEXT,
  PROP_WINDOW,
  PROP_OPACITY,
  PROP_DOUBLE_BUFFERED,
  PROP_HALIGN,
  PROP_VALIGN,
  PROP_MARGIN_LEFT,
  PROP_MARGIN_RIGHT,
  PROP_MARGIN_START,
  PROP_MARGIN_END,
  PROP_MARGIN_TOP,
  PROP_MARGIN_BOTTOM,
  PROP_MARGIN,
  PROP_HEXPAND,
  PROP_VEXPAND,
  PROP_HEXPAND_SET,
  PROP_VEXPAND_SET,
  PROP_EXPAND,
  PROP_SCALE_FACTOR,
  NUM_PROPERTIES
};

static GParamSpec *widget_props[NUM_PROPERTIES] = { NULL, };

typedef	struct	_CtkStateData	 CtkStateData;

struct _CtkStateData
{
  guint         flags_to_set;
  guint         flags_to_unset;

  gint          old_scale_factor;
};

/* --- prototypes --- */
static void	ctk_widget_base_class_init	(gpointer            g_class);
static void	ctk_widget_class_init		(CtkWidgetClass     *klass);
static void	ctk_widget_base_class_finalize	(CtkWidgetClass     *klass);
static void     ctk_widget_init                  (GTypeInstance     *instance,
                                                  gpointer           g_class);
static void	ctk_widget_set_property		 (GObject           *object,
						  guint              prop_id,
						  const GValue      *value,
						  GParamSpec        *pspec);
static void	ctk_widget_get_property		 (GObject           *object,
						  guint              prop_id,
						  GValue            *value,
						  GParamSpec        *pspec);
static void	ctk_widget_constructed           (GObject	    *object);
static void	ctk_widget_dispose		 (GObject	    *object);
static void	ctk_widget_real_destroy		 (CtkWidget	    *object);
static void	ctk_widget_finalize		 (GObject	    *object);
static void	ctk_widget_real_show		 (CtkWidget	    *widget);
static void	ctk_widget_real_hide		 (CtkWidget	    *widget);
static void	ctk_widget_real_map		 (CtkWidget	    *widget);
static void	ctk_widget_real_unmap		 (CtkWidget	    *widget);
static void	ctk_widget_real_realize		 (CtkWidget	    *widget);
static void	ctk_widget_real_unrealize	 (CtkWidget	    *widget);
static void	ctk_widget_real_size_allocate	 (CtkWidget	    *widget,
                                                  CtkAllocation	    *allocation);
static void	ctk_widget_real_style_set        (CtkWidget         *widget,
                                                  CtkStyle          *previous_style);
static void	ctk_widget_real_direction_changed(CtkWidget         *widget,
                                                  CtkTextDirection   previous_direction);

static void	ctk_widget_real_grab_focus	 (CtkWidget         *focus_widget);
static gboolean ctk_widget_real_query_tooltip    (CtkWidget         *widget,
						  gint               x,
						  gint               y,
						  gboolean           keyboard_tip,
						  CtkTooltip        *tooltip);
static void     ctk_widget_real_style_updated    (CtkWidget         *widget);
static gboolean ctk_widget_real_show_help        (CtkWidget         *widget,
                                                  CtkWidgetHelpType  help_type);
static gboolean _ctk_widget_run_controllers      (CtkWidget           *widget,
                                                  const CdkEvent      *event,
                                                  CtkPropagationPhase  phase);

static void	ctk_widget_dispatch_child_properties_changed	(CtkWidget        *object,
								 guint             n_pspecs,
								 GParamSpec      **pspecs);
static gboolean         ctk_widget_real_scroll_event            (CtkWidget        *widget,
                                                                 CdkEventScroll   *event);
static gboolean         ctk_widget_real_button_event            (CtkWidget        *widget,
                                                                 CdkEventButton   *event);
static gboolean         ctk_widget_real_motion_event            (CtkWidget        *widget,
                                                                 CdkEventMotion   *event);
static gboolean		ctk_widget_real_key_press_event   	(CtkWidget        *widget,
								 CdkEventKey      *event);
static gboolean		ctk_widget_real_key_release_event 	(CtkWidget        *widget,
								 CdkEventKey      *event);
static gboolean		ctk_widget_real_focus_in_event   	 (CtkWidget       *widget,
								  CdkEventFocus   *event);
static gboolean		ctk_widget_real_focus_out_event   	(CtkWidget        *widget,
								 CdkEventFocus    *event);
static gboolean         ctk_widget_real_touch_event             (CtkWidget        *widget,
                                                                 CdkEventTouch    *event);
static gboolean         ctk_widget_real_grab_broken_event       (CtkWidget          *widget,
                                                                 CdkEventGrabBroken *event);
static gboolean		ctk_widget_real_focus			(CtkWidget        *widget,
								 CtkDirectionType  direction);
static void             ctk_widget_real_move_focus              (CtkWidget        *widget,
                                                                 CtkDirectionType  direction);
static gboolean		ctk_widget_real_keynav_failed		(CtkWidget        *widget,
								 CtkDirectionType  direction);
#ifdef G_ENABLE_CONSISTENCY_CHECKS
static void             ctk_widget_verify_invariants            (CtkWidget        *widget);
static void             ctk_widget_push_verify_invariants       (CtkWidget        *widget);
static void             ctk_widget_pop_verify_invariants        (CtkWidget        *widget);
#else
#define                 ctk_widget_verify_invariants(widget)
#define                 ctk_widget_push_verify_invariants(widget)
#define                 ctk_widget_pop_verify_invariants(widget)
#endif
static PangoContext*	ctk_widget_peek_pango_context		(CtkWidget	  *widget);
static void     	ctk_widget_update_pango_context		(CtkWidget	  *widget);
static void		ctk_widget_propagate_state		(CtkWidget	  *widget,
								 CtkStateData 	  *data);
static void             ctk_widget_update_alpha                 (CtkWidget        *widget);

static gint		ctk_widget_event_internal		(CtkWidget	  *widget,
								 CdkEvent	  *event);
static gboolean		ctk_widget_real_mnemonic_activate	(CtkWidget	  *widget,
								 gboolean	   group_cycling);
static void             ctk_widget_real_get_width               (CtkWidget        *widget,
                                                                 gint             *minimum_size,
                                                                 gint             *natural_size);
static void             ctk_widget_real_get_height              (CtkWidget        *widget,
                                                                 gint             *minimum_size,
                                                                 gint             *natural_size);
static void             ctk_widget_real_get_height_for_width    (CtkWidget        *widget,
                                                                 gint              width,
                                                                 gint             *minimum_height,
                                                                 gint             *natural_height);
static void             ctk_widget_real_get_width_for_height    (CtkWidget        *widget,
                                                                 gint              height,
                                                                 gint             *minimum_width,
                                                                 gint             *natural_width);
static void             ctk_widget_real_state_flags_changed     (CtkWidget        *widget,
                                                                 CtkStateFlags     old_state);
static void             ctk_widget_real_queue_draw_region       (CtkWidget         *widget,
								 const cairo_region_t *region);
static AtkObject*	ctk_widget_real_get_accessible		(CtkWidget	  *widget);
static void		ctk_widget_accessible_interface_init	(AtkImplementorIface *iface);
static AtkObject*	ctk_widget_ref_accessible		(AtkImplementor *implementor);
static void             ctk_widget_invalidate_widget_windows    (CtkWidget        *widget,
								 cairo_region_t        *region);
static CdkScreen *      ctk_widget_get_screen_unchecked         (CtkWidget        *widget);
static gboolean         ctk_widget_real_can_activate_accel      (CtkWidget *widget,
                                                                 guint      signal_id);

static void             ctk_widget_real_set_has_tooltip         (CtkWidget *widget,
								 gboolean   has_tooltip,
								 gboolean   force);
static void             ctk_widget_buildable_interface_init     (CtkBuildableIface *iface);
static void             ctk_widget_buildable_set_name           (CtkBuildable     *buildable,
                                                                 const gchar      *name);
static const gchar *    ctk_widget_buildable_get_name           (CtkBuildable     *buildable);
static GObject *        ctk_widget_buildable_get_internal_child (CtkBuildable *buildable,
								 CtkBuilder   *builder,
								 const gchar  *childname);
static void             ctk_widget_buildable_set_buildable_property (CtkBuildable     *buildable,
								     CtkBuilder       *builder,
								     const gchar      *name,
								     const GValue     *value);
static gboolean         ctk_widget_buildable_custom_tag_start   (CtkBuildable     *buildable,
                                                                 CtkBuilder       *builder,
                                                                 GObject          *child,
                                                                 const gchar      *tagname,
                                                                 GMarkupParser    *parser,
                                                                 gpointer         *data);
static void             ctk_widget_buildable_custom_finished    (CtkBuildable     *buildable,
                                                                 CtkBuilder       *builder,
                                                                 GObject          *child,
                                                                 const gchar      *tagname,
                                                                 gpointer          data);
static void             ctk_widget_buildable_parser_finished    (CtkBuildable     *buildable,
                                                                 CtkBuilder       *builder);

static CtkSizeRequestMode ctk_widget_real_get_request_mode      (CtkWidget         *widget);
static void             ctk_widget_real_get_width               (CtkWidget         *widget,
                                                                 gint              *minimum_size,
                                                                 gint              *natural_size);
static void             ctk_widget_real_get_height              (CtkWidget         *widget,
                                                                 gint              *minimum_size,
                                                                 gint              *natural_size);

static void             ctk_widget_queue_tooltip_query          (CtkWidget *widget);


static void             ctk_widget_real_adjust_size_request     (CtkWidget         *widget,
                                                                 CtkOrientation     orientation,
                                                                 gint              *minimum_size,
                                                                 gint              *natural_size);
static void             ctk_widget_real_adjust_baseline_request (CtkWidget         *widget,
								 gint              *minimum_baseline,
								 gint              *natural_baseline);
static void             ctk_widget_real_adjust_size_allocation  (CtkWidget         *widget,
                                                                 CtkOrientation     orientation,
                                                                 gint              *minimum_size,
                                                                 gint              *natural_size,
                                                                 gint              *allocated_pos,
                                                                 gint              *allocated_size);
static void             ctk_widget_real_adjust_baseline_allocation (CtkWidget         *widget,
								    gint              *baseline);

static void                  template_data_free                 (CtkWidgetTemplate    *template_data);

static void ctk_widget_set_usize_internal (CtkWidget          *widget,
					   gint                width,
					   gint                height);

static void ctk_widget_add_events_internal (CtkWidget *widget,
                                            CdkDevice *device,
                                            gint       events);
static void ctk_widget_set_device_enabled_internal (CtkWidget *widget,
                                                    CdkDevice *device,
                                                    gboolean   recurse,
                                                    gboolean   enabled);

static void ctk_widget_on_frame_clock_update (CdkFrameClock *frame_clock,
                                              CtkWidget     *widget);

static gboolean event_window_is_still_viewable (CdkEvent *event);

static void ctk_widget_update_input_shape (CtkWidget *widget);

/* --- variables --- */
static gint             CtkWidget_private_offset = 0;
static gpointer         ctk_widget_parent_class = NULL;
static guint            widget_signals[LAST_SIGNAL] = { 0 };
static guint            composite_child_stack = 0;
CtkTextDirection ctk_default_direction = CTK_TEXT_DIR_LTR;
static GParamSpecPool  *style_property_spec_pool = NULL;

static GQuark		quark_property_parser = 0;
static GQuark		quark_accel_path = 0;
static GQuark		quark_accel_closures = 0;
static GQuark		quark_event_mask = 0;
static GQuark           quark_device_event_mask = 0;
static GQuark		quark_parent_window = 0;
static GQuark		quark_shape_info = 0;
static GQuark		quark_input_shape_info = 0;
static GQuark		quark_pango_context = 0;
static GQuark		quark_mnemonic_labels = 0;
static GQuark		quark_tooltip_markup = 0;
static GQuark		quark_tooltip_window = 0;
static GQuark		quark_visual = 0;
static GQuark           quark_modifier_style = 0;
static GQuark           quark_enabled_devices = 0;
static GQuark           quark_size_groups = 0;
static GQuark           quark_auto_children = 0;
static GQuark           quark_widget_path = 0;
static GQuark           quark_action_muxer = 0;
static GQuark           quark_font_options = 0;
static GQuark           quark_font_map = 0;

GParamSpecPool         *_ctk_widget_child_property_pool = NULL;
GObjectNotifyContext   *_ctk_widget_child_property_notify_context = NULL;

/* --- functions --- */
GType
ctk_widget_get_type (void)
{
  static GType widget_type = 0;

  if (G_UNLIKELY (widget_type == 0))
    {
      const GTypeInfo widget_info =
      {
	sizeof (CtkWidgetClass),
	ctk_widget_base_class_init,
	(GBaseFinalizeFunc) ctk_widget_base_class_finalize,
	(GClassInitFunc) ctk_widget_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_init */
	sizeof (CtkWidget),
	0,		/* n_preallocs */
	ctk_widget_init,
	NULL,		/* value_table */
      };

      const GInterfaceInfo accessibility_info =
      {
	(GInterfaceInitFunc) ctk_widget_accessible_interface_init,
	(GInterfaceFinalizeFunc) NULL,
	NULL /* interface data */
      };

      const GInterfaceInfo buildable_info =
      {
	(GInterfaceInitFunc) ctk_widget_buildable_interface_init,
	(GInterfaceFinalizeFunc) NULL,
	NULL /* interface data */
      };

      widget_type = g_type_register_static (G_TYPE_INITIALLY_UNOWNED, "CtkWidget",
                                            &widget_info, G_TYPE_FLAG_ABSTRACT);

      g_type_add_class_private (widget_type, sizeof (CtkWidgetClassPrivate));

      CtkWidget_private_offset =
        g_type_add_instance_private (widget_type, sizeof (CtkWidgetPrivate));

      g_type_add_interface_static (widget_type, ATK_TYPE_IMPLEMENTOR,
                                   &accessibility_info) ;
      g_type_add_interface_static (widget_type, CTK_TYPE_BUILDABLE,
                                   &buildable_info) ;
    }

  return widget_type;
}

static inline gpointer
ctk_widget_get_instance_private (CtkWidget *self)
{
  return (G_STRUCT_MEMBER_P (self, CtkWidget_private_offset));
}

static void
ctk_widget_base_class_init (gpointer g_class)
{
  CtkWidgetClass *klass = g_class;

  klass->priv = G_TYPE_CLASS_GET_PRIVATE (g_class, CTK_TYPE_WIDGET, CtkWidgetClassPrivate);
  klass->priv->template = NULL;
}

static void
child_property_notify_dispatcher (GObject     *object,
				  guint        n_pspecs,
				  GParamSpec **pspecs)
{
  CTK_WIDGET_GET_CLASS (object)->dispatch_child_properties_changed (CTK_WIDGET (object), n_pspecs, pspecs);
}

/* We guard against the draw signal callbacks modifying the state of the
 * cairo context by surrounding it with save/restore.
 * Maybe we should also cairo_new_path() just to be sure?
 */
static void
ctk_widget_draw_marshaller (GClosure     *closure,
                            GValue       *return_value,
                            guint         n_param_values,
                            const GValue *param_values,
                            gpointer      invocation_hint,
                            gpointer      marshal_data)
{
  cairo_t *cr = g_value_get_boxed (&param_values[1]);

  cairo_save (cr);

  _ctk_marshal_BOOLEAN__BOXED (closure,
                               return_value,
                               n_param_values,
                               param_values,
                               invocation_hint,
                               marshal_data);


  cairo_restore (cr);
}

static void
ctk_widget_draw_marshallerv (GClosure     *closure,
			     GValue       *return_value,
			     gpointer      instance,
			     va_list       args,
			     gpointer      marshal_data,
			     int           n_params,
			     GType        *param_types)
{
  cairo_t *cr;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  cr = va_arg (args_copy, gpointer);

  cairo_save (cr);

  _ctk_marshal_BOOLEAN__BOXEDv (closure,
				return_value,
				instance,
				args,
				marshal_data,
				n_params,
				param_types);


  cairo_restore (cr);

  va_end (args_copy);
}

static void
ctk_widget_class_init (CtkWidgetClass *klass)
{
  static GObjectNotifyContext cpn_context = { 0, NULL, NULL };
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  CtkBindingSet *binding_set;

  g_type_class_adjust_private_offset (klass, &CtkWidget_private_offset);
  ctk_widget_parent_class = g_type_class_peek_parent (klass);

  quark_property_parser = g_quark_from_static_string ("ctk-rc-property-parser");
  quark_accel_path = g_quark_from_static_string ("ctk-accel-path");
  quark_accel_closures = g_quark_from_static_string ("ctk-accel-closures");
  quark_event_mask = g_quark_from_static_string ("ctk-event-mask");
  quark_device_event_mask = g_quark_from_static_string ("ctk-device-event-mask");
  quark_parent_window = g_quark_from_static_string ("ctk-parent-window");
  quark_shape_info = g_quark_from_static_string ("ctk-shape-info");
  quark_input_shape_info = g_quark_from_static_string ("ctk-input-shape-info");
  quark_pango_context = g_quark_from_static_string ("ctk-pango-context");
  quark_mnemonic_labels = g_quark_from_static_string ("ctk-mnemonic-labels");
  quark_tooltip_markup = g_quark_from_static_string ("ctk-tooltip-markup");
  quark_tooltip_window = g_quark_from_static_string ("ctk-tooltip-window");
  quark_visual = g_quark_from_static_string ("ctk-widget-visual");
  quark_modifier_style = g_quark_from_static_string ("ctk-widget-modifier-style");
  quark_enabled_devices = g_quark_from_static_string ("ctk-widget-enabled-devices");
  quark_size_groups = g_quark_from_static_string ("ctk-widget-size-groups");
  quark_auto_children = g_quark_from_static_string ("ctk-widget-auto-children");
  quark_widget_path = g_quark_from_static_string ("ctk-widget-path");
  quark_action_muxer = g_quark_from_static_string ("ctk-widget-action-muxer");
  quark_font_options = g_quark_from_static_string ("ctk-widget-font-options");
  quark_font_map = g_quark_from_static_string ("ctk-widget-font-map");

  style_property_spec_pool = g_param_spec_pool_new (FALSE);
  _ctk_widget_child_property_pool = g_param_spec_pool_new (TRUE);
  cpn_context.quark_notify_queue = g_quark_from_static_string ("CtkWidget-child-property-notify-queue");
  cpn_context.dispatcher = child_property_notify_dispatcher;
  _ctk_widget_child_property_notify_context = &cpn_context;

  gobject_class->constructed = ctk_widget_constructed;
  gobject_class->dispose = ctk_widget_dispose;
  gobject_class->finalize = ctk_widget_finalize;
  gobject_class->set_property = ctk_widget_set_property;
  gobject_class->get_property = ctk_widget_get_property;

  klass->destroy = ctk_widget_real_destroy;

  klass->activate_signal = 0;
  klass->dispatch_child_properties_changed = ctk_widget_dispatch_child_properties_changed;
  klass->show = ctk_widget_real_show;
  klass->show_all = ctk_widget_show;
  klass->hide = ctk_widget_real_hide;
  klass->map = ctk_widget_real_map;
  klass->unmap = ctk_widget_real_unmap;
  klass->realize = ctk_widget_real_realize;
  klass->unrealize = ctk_widget_real_unrealize;
  klass->size_allocate = ctk_widget_real_size_allocate;
  klass->get_request_mode = ctk_widget_real_get_request_mode;
  klass->get_preferred_width = ctk_widget_real_get_width;
  klass->get_preferred_height = ctk_widget_real_get_height;
  klass->get_preferred_width_for_height = ctk_widget_real_get_width_for_height;
  klass->get_preferred_height_for_width = ctk_widget_real_get_height_for_width;
  klass->get_preferred_height_and_baseline_for_width = NULL;
  klass->state_changed = NULL;
  klass->state_flags_changed = ctk_widget_real_state_flags_changed;
  klass->parent_set = NULL;
  klass->hierarchy_changed = NULL;
  klass->style_set = ctk_widget_real_style_set;
  klass->direction_changed = ctk_widget_real_direction_changed;
  klass->grab_notify = NULL;
  klass->child_notify = NULL;
  klass->draw = NULL;
  klass->mnemonic_activate = ctk_widget_real_mnemonic_activate;
  klass->grab_focus = ctk_widget_real_grab_focus;
  klass->focus = ctk_widget_real_focus;
  klass->move_focus = ctk_widget_real_move_focus;
  klass->keynav_failed = ctk_widget_real_keynav_failed;
  klass->event = NULL;
  klass->scroll_event = ctk_widget_real_scroll_event;
  klass->button_press_event = ctk_widget_real_button_event;
  klass->button_release_event = ctk_widget_real_button_event;
  klass->motion_notify_event = ctk_widget_real_motion_event;
  klass->touch_event = ctk_widget_real_touch_event;
  klass->delete_event = NULL;
  klass->destroy_event = NULL;
  klass->key_press_event = ctk_widget_real_key_press_event;
  klass->key_release_event = ctk_widget_real_key_release_event;
  klass->enter_notify_event = NULL;
  klass->leave_notify_event = NULL;
  klass->configure_event = NULL;
  klass->focus_in_event = ctk_widget_real_focus_in_event;
  klass->focus_out_event = ctk_widget_real_focus_out_event;
  klass->map_event = NULL;
  klass->unmap_event = NULL;
  klass->window_state_event = NULL;
  klass->property_notify_event = _ctk_selection_property_notify;
  klass->selection_clear_event = _ctk_selection_clear;
  klass->selection_request_event = _ctk_selection_request;
  klass->selection_notify_event = _ctk_selection_notify;
  klass->selection_received = NULL;
  klass->proximity_in_event = NULL;
  klass->proximity_out_event = NULL;
  klass->drag_begin = NULL;
  klass->drag_end = NULL;
  klass->drag_data_delete = NULL;
  klass->drag_leave = NULL;
  klass->drag_motion = NULL;
  klass->drag_drop = NULL;
  klass->drag_data_received = NULL;
  klass->screen_changed = NULL;
  klass->can_activate_accel = ctk_widget_real_can_activate_accel;
  klass->grab_broken_event = ctk_widget_real_grab_broken_event;
  klass->query_tooltip = ctk_widget_real_query_tooltip;
  klass->style_updated = ctk_widget_real_style_updated;

  klass->show_help = ctk_widget_real_show_help;

  /* Accessibility support */
  klass->priv->accessible_type = CTK_TYPE_ACCESSIBLE;
  klass->priv->accessible_role = ATK_ROLE_INVALID;
  klass->get_accessible = ctk_widget_real_get_accessible;

  klass->adjust_size_request = ctk_widget_real_adjust_size_request;
  klass->adjust_baseline_request = ctk_widget_real_adjust_baseline_request;
  klass->adjust_size_allocation = ctk_widget_real_adjust_size_allocation;
  klass->adjust_baseline_allocation = ctk_widget_real_adjust_baseline_allocation;
  klass->queue_draw_region = ctk_widget_real_queue_draw_region;

  widget_props[PROP_NAME] =
      g_param_spec_string ("name",
                           P_("Widget name"),
                           P_("The name of the widget"),
                           NULL,
                           CTK_PARAM_READWRITE);

  widget_props[PROP_PARENT] =
      g_param_spec_object ("parent",
                           P_("Parent widget"),
                           P_("The parent widget of this widget. Must be a Container widget"),
                           CTK_TYPE_CONTAINER,
                           CTK_PARAM_READWRITE);

  widget_props[PROP_WIDTH_REQUEST] =
      g_param_spec_int ("width-request",
                        P_("Width request"),
                        P_("Override for width request of the widget, or -1 if natural request should be used"),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  widget_props[PROP_HEIGHT_REQUEST] =
      g_param_spec_int ("height-request",
                        P_("Height request"),
                        P_("Override for height request of the widget, or -1 if natural request should be used"),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  widget_props[PROP_VISIBLE] =
      g_param_spec_boolean ("visible",
                            P_("Visible"),
                            P_("Whether the widget is visible"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  widget_props[PROP_SENSITIVE] =
      g_param_spec_boolean ("sensitive",
                            P_("Sensitive"),
                            P_("Whether the widget responds to input"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  widget_props[PROP_APP_PAINTABLE] =
      g_param_spec_boolean ("app-paintable",
                            P_("Application paintable"),
                            P_("Whether the application will paint directly on the widget"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  widget_props[PROP_CAN_FOCUS] =
      g_param_spec_boolean ("can-focus",
                            P_("Can focus"),
                            P_("Whether the widget can accept the input focus"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  widget_props[PROP_HAS_FOCUS] =
      g_param_spec_boolean ("has-focus",
                            P_("Has focus"),
                            P_("Whether the widget has the input focus"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  widget_props[PROP_IS_FOCUS] =
      g_param_spec_boolean ("is-focus",
                            P_("Is focus"),
                            P_("Whether the widget is the focus widget within the toplevel"),
                            FALSE,
                            CTK_PARAM_READWRITE);

  /**
   * CtkWidget:focus-on-click:
   *
   * Whether the widget should grab focus when it is clicked with the mouse.
   *
   * This property is only relevant for widgets that can take focus.
   *
   * Before 3.20, several widgets (CtkButton, CtkFileChooserButton,
   * CtkComboBox) implemented this property individually.
   *
   * Since: 3.20
   */
  widget_props[PROP_FOCUS_ON_CLICK] =
      g_param_spec_boolean ("focus-on-click",
                            P_("Focus on click"),
                            P_("Whether the widget should grab focus when it is clicked with the mouse"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  widget_props[PROP_CAN_DEFAULT] =
      g_param_spec_boolean ("can-default",
                            P_("Can default"),
                            P_("Whether the widget can be the default widget"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  widget_props[PROP_HAS_DEFAULT] =
      g_param_spec_boolean ("has-default",
                            P_("Has default"),
                            P_("Whether the widget is the default widget"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  widget_props[PROP_RECEIVES_DEFAULT] =
      g_param_spec_boolean ("receives-default",
                            P_("Receives default"),
                            P_("If TRUE, the widget will receive the default action when it is focused"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  widget_props[PROP_COMPOSITE_CHILD] =
      g_param_spec_boolean ("composite-child",
                            P_("Composite child"),
                            P_("Whether the widget is part of a composite widget"),
                            FALSE,
                            CTK_PARAM_READABLE);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

  /**
   * CtkWidget:style:
   *
   * The style of the widget, which contains information about how it will look (colors, etc).
   *
   * Deprecated: Use #CtkStyleContext instead
   */
  widget_props[PROP_STYLE] =
      g_param_spec_object ("style",
                           P_("Style"),
                           P_("The style of the widget, which contains information about how it will look (colors etc)"),
                           CTK_TYPE_STYLE,
                           CTK_PARAM_READWRITE|G_PARAM_DEPRECATED);

G_GNUC_END_IGNORE_DEPRECATIONS

  widget_props[PROP_EVENTS] =
      g_param_spec_flags ("events",
                          P_("Events"),
                          P_("The event mask that decides what kind of CdkEvents this widget gets"),
                          CDK_TYPE_EVENT_MASK,
                          CDK_STRUCTURE_MASK,
                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  widget_props[PROP_NO_SHOW_ALL] =
      g_param_spec_boolean ("no-show-all",
                            P_("No show all"),
                            P_("Whether ctk_widget_show_all() should not affect this widget"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

/**
 * CtkWidget:has-tooltip:
 *
 * Enables or disables the emission of #CtkWidget::query-tooltip on @widget.
 * A value of %TRUE indicates that @widget can have a tooltip, in this case
 * the widget will be queried using #CtkWidget::query-tooltip to determine
 * whether it will provide a tooltip or not.
 *
 * Note that setting this property to %TRUE for the first time will change
 * the event masks of the CdkWindows of this widget to include leave-notify
 * and motion-notify events.  This cannot and will not be undone when the
 * property is set to %FALSE again.
 *
 * Since: 2.12
 */
  widget_props[PROP_HAS_TOOLTIP] =
      g_param_spec_boolean ("has-tooltip",
                            P_("Has tooltip"),
                            P_("Whether this widget has a tooltip"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWidget:tooltip-text:
   *
   * Sets the text of tooltip to be the given string.
   *
   * Also see ctk_tooltip_set_text().
   *
   * This is a convenience property which will take care of getting the
   * tooltip shown if the given string is not %NULL: #CtkWidget:has-tooltip
   * will automatically be set to %TRUE and there will be taken care of
   * #CtkWidget::query-tooltip in the default signal handler.
   *
   * Note that if both #CtkWidget:tooltip-text and #CtkWidget:tooltip-markup
   * are set, the last one wins.
   *
   * Since: 2.12
   */
  widget_props[PROP_TOOLTIP_TEXT] =
      g_param_spec_string ("tooltip-text",
                           P_("Tooltip Text"),
                           P_("The contents of the tooltip for this widget"),
                           NULL,
                           CTK_PARAM_READWRITE);

  /**
   * CtkWidget:tooltip-markup:
   *
   * Sets the text of tooltip to be the given string, which is marked up
   * with the [Pango text markup language][PangoMarkupFormat].
   * Also see ctk_tooltip_set_markup().
   *
   * This is a convenience property which will take care of getting the
   * tooltip shown if the given string is not %NULL: #CtkWidget:has-tooltip
   * will automatically be set to %TRUE and there will be taken care of
   * #CtkWidget::query-tooltip in the default signal handler.
   *
   * Note that if both #CtkWidget:tooltip-text and #CtkWidget:tooltip-markup
   * are set, the last one wins.
   *
   * Since: 2.12
   */
  widget_props[PROP_TOOLTIP_MARKUP] =
      g_param_spec_string ("tooltip-markup",
                           P_("Tooltip markup"),
                           P_("The contents of the tooltip for this widget"),
                           NULL,
                           CTK_PARAM_READWRITE);

  /**
   * CtkWidget:window:
   *
   * The widget's window if it is realized, %NULL otherwise.
   *
   * Since: 2.14
   */
  widget_props[PROP_WINDOW] =
      g_param_spec_object ("window",
                           P_("Window"),
                           P_("The widget's window if it is realized"),
                           CDK_TYPE_WINDOW,
                           CTK_PARAM_READABLE);

  /**
   * CtkWidget:double-buffered:
   *
   * Whether the widget is double buffered.
   *
   * Since: 2.18
   *
   * Deprecated: 3.14: Widgets should not use this property.
   */
  widget_props[PROP_DOUBLE_BUFFERED] =
      g_param_spec_boolean ("double-buffered",
                            P_("Double Buffered"),
                            P_("Whether the widget is double buffered"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  /**
   * CtkWidget:halign:
   *
   * How to distribute horizontal space if widget gets extra space, see #CtkAlign
   *
   * Since: 3.0
   */
  widget_props[PROP_HALIGN] =
      g_param_spec_enum ("halign",
                         P_("Horizontal Alignment"),
                         P_("How to position in extra horizontal space"),
                         CTK_TYPE_ALIGN,
                         CTK_ALIGN_FILL,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWidget:valign:
   *
   * How to distribute vertical space if widget gets extra space, see #CtkAlign
   *
   * Since: 3.0
   */
  widget_props[PROP_VALIGN] =
      g_param_spec_enum ("valign",
                         P_("Vertical Alignment"),
                         P_("How to position in extra vertical space"),
                         CTK_TYPE_ALIGN,
                         CTK_ALIGN_FILL,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWidget:margin-left:
   *
   * Margin on left side of widget.
   *
   * This property adds margin outside of the widget's normal size
   * request, the margin will be added in addition to the size from
   * ctk_widget_set_size_request() for example.
   *
   * Deprecated: 3.12: Use #CtkWidget:margin-start instead.
   *
   * Since: 3.0
   */
  widget_props[PROP_MARGIN_LEFT] =
      g_param_spec_int ("margin-left",
                        P_("Margin on Left"),
                        P_("Pixels of extra space on the left side"),
                        0, G_MAXINT16,
                        0,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  /**
   * CtkWidget:margin-right:
   *
   * Margin on right side of widget.
   *
   * This property adds margin outside of the widget's normal size
   * request, the margin will be added in addition to the size from
   * ctk_widget_set_size_request() for example.
   *
   * Deprecated: 3.12: Use #CtkWidget:margin-end instead.
   *
   * Since: 3.0
   */
  widget_props[PROP_MARGIN_RIGHT] =
      g_param_spec_int ("margin-right",
                        P_("Margin on Right"),
                        P_("Pixels of extra space on the right side"),
                        0, G_MAXINT16,
                        0,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  /**
   * CtkWidget:margin-start:
   *
   * Margin on start of widget, horizontally. This property supports
   * left-to-right and right-to-left text directions.
   *
   * This property adds margin outside of the widget's normal size
   * request, the margin will be added in addition to the size from
   * ctk_widget_set_size_request() for example.
   *
   * Since: 3.12
   */
  widget_props[PROP_MARGIN_START] =
      g_param_spec_int ("margin-start",
                        P_("Margin on Start"),
                        P_("Pixels of extra space on the start"),
                        0, G_MAXINT16,
                        0,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWidget:margin-end:
   *
   * Margin on end of widget, horizontally. This property supports
   * left-to-right and right-to-left text directions.
   *
   * This property adds margin outside of the widget's normal size
   * request, the margin will be added in addition to the size from
   * ctk_widget_set_size_request() for example.
   *
   * Since: 3.12
   */
  widget_props[PROP_MARGIN_END] =
      g_param_spec_int ("margin-end",
                        P_("Margin on End"),
                        P_("Pixels of extra space on the end"),
                        0, G_MAXINT16,
                        0,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWidget:margin-top:
   *
   * Margin on top side of widget.
   *
   * This property adds margin outside of the widget's normal size
   * request, the margin will be added in addition to the size from
   * ctk_widget_set_size_request() for example.
   *
   * Since: 3.0
   */
  widget_props[PROP_MARGIN_TOP] =
      g_param_spec_int ("margin-top",
                        P_("Margin on Top"),
                        P_("Pixels of extra space on the top side"),
                        0, G_MAXINT16,
                        0,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWidget:margin-bottom:
   *
   * Margin on bottom side of widget.
   *
   * This property adds margin outside of the widget's normal size
   * request, the margin will be added in addition to the size from
   * ctk_widget_set_size_request() for example.
   *
   * Since: 3.0
   */
  widget_props[PROP_MARGIN_BOTTOM] =
      g_param_spec_int ("margin-bottom",
                        P_("Margin on Bottom"),
                        P_("Pixels of extra space on the bottom side"),
                        0, G_MAXINT16,
                        0,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWidget:margin:
   *
   * Sets all four sides' margin at once. If read, returns max
   * margin on any side.
   *
   * Since: 3.0
   */
  widget_props[PROP_MARGIN] =
      g_param_spec_int ("margin",
                        P_("All Margins"),
                        P_("Pixels of extra space on all four sides"),
                        0, G_MAXINT16,
                        0,
                        CTK_PARAM_READWRITE);

  /**
   * CtkWidget:hexpand:
   *
   * Whether to expand horizontally. See ctk_widget_set_hexpand().
   *
   * Since: 3.0
   */
  widget_props[PROP_HEXPAND] =
      g_param_spec_boolean ("hexpand",
                            P_("Horizontal Expand"),
                            P_("Whether widget wants more horizontal space"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWidget:hexpand-set:
   *
   * Whether to use the #CtkWidget:hexpand property. See ctk_widget_get_hexpand_set().
   *
   * Since: 3.0
   */
  widget_props[PROP_HEXPAND_SET] =
      g_param_spec_boolean ("hexpand-set",
                            P_("Horizontal Expand Set"),
                            P_("Whether to use the hexpand property"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWidget:vexpand:
   *
   * Whether to expand vertically. See ctk_widget_set_vexpand().
   *
   * Since: 3.0
   */
  widget_props[PROP_VEXPAND] =
      g_param_spec_boolean ("vexpand",
                            P_("Vertical Expand"),
                            P_("Whether widget wants more vertical space"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWidget:vexpand-set:
   *
   * Whether to use the #CtkWidget:vexpand property. See ctk_widget_get_vexpand_set().
   *
   * Since: 3.0
   */
  widget_props[PROP_VEXPAND_SET] =
      g_param_spec_boolean ("vexpand-set",
                            P_("Vertical Expand Set"),
                            P_("Whether to use the vexpand property"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWidget:expand:
   *
   * Whether to expand in both directions. Setting this sets both #CtkWidget:hexpand and #CtkWidget:vexpand
   *
   * Since: 3.0
   */
  widget_props[PROP_EXPAND] =
      g_param_spec_boolean ("expand",
                            P_("Expand Both"),
                            P_("Whether widget wants to expand in both directions"),
                            FALSE,
                            CTK_PARAM_READWRITE);

  /**
   * CtkWidget:opacity:
   *
   * The requested opacity of the widget. See ctk_widget_set_opacity() for
   * more details about window opacity.
   *
   * Before 3.8 this was only available in CtkWindow
   *
   * Since: 3.8
   */
  widget_props[PROP_OPACITY] =
      g_param_spec_double ("opacity",
                           P_("Opacity for Widget"),
                           P_("The opacity of the widget, from 0 to 1"),
                           0.0, 1.0,
                           1.0,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWidget:scale-factor:
   *
   * The scale factor of the widget. See ctk_widget_get_scale_factor() for
   * more details about widget scaling.
   *
   * Since: 3.10
   */
  widget_props[PROP_SCALE_FACTOR] =
      g_param_spec_int ("scale-factor",
                        P_("Scale factor"),
                        P_("The scaling factor of the window"),
                        1, G_MAXINT,
                        1,
                        CTK_PARAM_READABLE);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, widget_props);

  /**
   * CtkWidget::destroy:
   * @object: the object which received the signal
   *
   * Signals that all holders of a reference to the widget should release
   * the reference that they hold. May result in finalization of the widget
   * if all references are released.
   *
   * This signal is not suitable for saving widget state.
   */
  widget_signals[DESTROY] =
    g_signal_new (I_("destroy"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_CLEANUP | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  G_STRUCT_OFFSET (CtkWidgetClass, destroy),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkWidget::show:
   * @widget: the object which received the signal.
   *
   * The ::show signal is emitted when @widget is shown, for example with
   * ctk_widget_show().
   */
  widget_signals[SHOW] =
    g_signal_new (I_("show"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkWidgetClass, show),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkWidget::hide:
   * @widget: the object which received the signal.
   *
   * The ::hide signal is emitted when @widget is hidden, for example with
   * ctk_widget_hide().
   */
  widget_signals[HIDE] =
    g_signal_new (I_("hide"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkWidgetClass, hide),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkWidget::map:
   * @widget: the object which received the signal.
   *
   * The ::map signal is emitted when @widget is going to be mapped, that is
   * when the widget is visible (which is controlled with
   * ctk_widget_set_visible()) and all its parents up to the toplevel widget
   * are also visible. Once the map has occurred, #CtkWidget::map-event will
   * be emitted.
   *
   * The ::map signal can be used to determine whether a widget will be drawn,
   * for instance it can resume an animation that was stopped during the
   * emission of #CtkWidget::unmap.
   */
  widget_signals[MAP] =
    g_signal_new (I_("map"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkWidgetClass, map),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkWidget::unmap:
   * @widget: the object which received the signal.
   *
   * The ::unmap signal is emitted when @widget is going to be unmapped, which
   * means that either it or any of its parents up to the toplevel widget have
   * been set as hidden.
   *
   * As ::unmap indicates that a widget will not be shown any longer, it can be
   * used to, for example, stop an animation on the widget.
   */
  widget_signals[UNMAP] =
    g_signal_new (I_("unmap"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkWidgetClass, unmap),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkWidget::realize:
   * @widget: the object which received the signal.
   *
   * The ::realize signal is emitted when @widget is associated with a
   * #CdkWindow, which means that ctk_widget_realize() has been called or the
   * widget has been mapped (that is, it is going to be drawn).
   */
  widget_signals[REALIZE] =
    g_signal_new (I_("realize"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkWidgetClass, realize),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkWidget::unrealize:
   * @widget: the object which received the signal.
   *
   * The ::unrealize signal is emitted when the #CdkWindow associated with
   * @widget is destroyed, which means that ctk_widget_unrealize() has been
   * called or the widget has been unmapped (that is, it is going to be
   * hidden).
   */
  widget_signals[UNREALIZE] =
    g_signal_new (I_("unrealize"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, unrealize),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkWidget::size-allocate:
   * @widget: the object which received the signal.
   * @allocation: (type Ctk.Allocation): the region which has been
   *   allocated to the widget.
   */
  widget_signals[SIZE_ALLOCATE] =
    g_signal_new (I_("size-allocate"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkWidgetClass, size_allocate),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CDK_TYPE_RECTANGLE | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * CtkWidget::state-changed:
   * @widget: the object which received the signal.
   * @state: the previous state
   *
   * The ::state-changed signal is emitted when the widget state changes.
   * See ctk_widget_get_state().
   *
   * Deprecated: 3.0: Use #CtkWidget::state-flags-changed instead.
   */
  widget_signals[STATE_CHANGED] =
    g_signal_new (I_("state-changed"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_DEPRECATED,
		  G_STRUCT_OFFSET (CtkWidgetClass, state_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_STATE_TYPE);

  /**
   * CtkWidget::state-flags-changed:
   * @widget: the object which received the signal.
   * @flags: The previous state flags.
   *
   * The ::state-flags-changed signal is emitted when the widget state
   * changes, see ctk_widget_get_state_flags().
   *
   * Since: 3.0
   */
  widget_signals[STATE_FLAGS_CHANGED] =
    g_signal_new (I_("state-flags-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkWidgetClass, state_flags_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CTK_TYPE_STATE_FLAGS);

  /**
   * CtkWidget::parent-set:
   * @widget: the object on which the signal is emitted
   * @old_parent: (allow-none): the previous parent, or %NULL if the widget
   *   just got its initial parent.
   *
   * The ::parent-set signal is emitted when a new parent
   * has been set on a widget.
   */
  widget_signals[PARENT_SET] =
    g_signal_new (I_("parent-set"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkWidgetClass, parent_set),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_WIDGET);

  /**
   * CtkWidget::hierarchy-changed:
   * @widget: the object on which the signal is emitted
   * @previous_toplevel: (allow-none): the previous toplevel ancestor, or %NULL
   *   if the widget was previously unanchored
   *
   * The ::hierarchy-changed signal is emitted when the
   * anchored state of a widget changes. A widget is
   * “anchored” when its toplevel
   * ancestor is a #CtkWindow. This signal is emitted when
   * a widget changes from un-anchored to anchored or vice-versa.
   */
  widget_signals[HIERARCHY_CHANGED] =
    g_signal_new (I_("hierarchy-changed"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, hierarchy_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_WIDGET);

  /**
   * CtkWidget::style-set:
   * @widget: the object on which the signal is emitted
   * @previous_style: (allow-none): the previous style, or %NULL if the widget
   *   just got its initial style
   *
   * The ::style-set signal is emitted when a new style has been set
   * on a widget. Note that style-modifying functions like
   * ctk_widget_modify_base() also cause this signal to be emitted.
   *
   * Note that this signal is emitted for changes to the deprecated
   * #CtkStyle. To track changes to the #CtkStyleContext associated
   * with a widget, use the #CtkWidget::style-updated signal.
   *
   * Deprecated:3.0: Use the #CtkWidget::style-updated signal
   */

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

  widget_signals[STYLE_SET] =
    g_signal_new (I_("style-set"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_DEPRECATED,
		  G_STRUCT_OFFSET (CtkWidgetClass, style_set),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_STYLE);

G_GNUC_END_IGNORE_DEPRECATIONS

  /**
   * CtkWidget::style-updated:
   * @widget: the object on which the signal is emitted
   *
   * The ::style-updated signal is a convenience signal that is emitted when the
   * #CtkStyleContext::changed signal is emitted on the @widget's associated
   * #CtkStyleContext as returned by ctk_widget_get_style_context().
   *
   * Note that style-modifying functions like ctk_widget_override_color() also
   * cause this signal to be emitted.
   *
   * Since: 3.0
   */
  widget_signals[STYLE_UPDATED] =
    g_signal_new (I_("style-updated"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkWidgetClass, style_updated),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkWidget::direction-changed:
   * @widget: the object on which the signal is emitted
   * @previous_direction: the previous text direction of @widget
   *
   * The ::direction-changed signal is emitted when the text direction
   * of a widget changes.
   */
  widget_signals[DIRECTION_CHANGED] =
    g_signal_new (I_("direction-changed"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkWidgetClass, direction_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_TEXT_DIRECTION);

  /**
   * CtkWidget::grab-notify:
   * @widget: the object which received the signal
   * @was_grabbed: %FALSE if the widget becomes shadowed, %TRUE
   *               if it becomes unshadowed
   *
   * The ::grab-notify signal is emitted when a widget becomes
   * shadowed by a CTK+ grab (not a pointer or keyboard grab) on
   * another widget, or when it becomes unshadowed due to a grab
   * being removed.
   *
   * A widget is shadowed by a ctk_grab_add() when the topmost
   * grab widget in the grab stack of its window group is not
   * its ancestor.
   */
  widget_signals[GRAB_NOTIFY] =
    g_signal_new (I_("grab-notify"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkWidgetClass, grab_notify),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  G_TYPE_BOOLEAN);

  /**
   * CtkWidget::child-notify:
   * @widget: the object which received the signal
   * @child_property: the #GParamSpec of the changed child property
   *
   * The ::child-notify signal is emitted for each
   * [child property][child-properties]  that has
   * changed on an object. The signal's detail holds the property name.
   */
  widget_signals[CHILD_NOTIFY] =
    g_signal_new (I_("child-notify"),
		   G_TYPE_FROM_CLASS (gobject_class),
		   G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED | G_SIGNAL_NO_HOOKS,
		   G_STRUCT_OFFSET (CtkWidgetClass, child_notify),
		   NULL, NULL,
		   NULL,
		   G_TYPE_NONE, 1,
		   G_TYPE_PARAM);

  /**
   * CtkWidget::draw:
   * @widget: the object which received the signal
   * @cr: the cairo context to draw to
   *
   * This signal is emitted when a widget is supposed to render itself.
   * The @widget's top left corner must be painted at the origin of
   * the passed in context and be sized to the values returned by
   * ctk_widget_get_allocated_width() and
   * ctk_widget_get_allocated_height().
   *
   * Signal handlers connected to this signal can modify the cairo
   * context passed as @cr in any way they like and don't need to
   * restore it. The signal emission takes care of calling cairo_save()
   * before and cairo_restore() after invoking the handler.
   *
   * The signal handler will get a @cr with a clip region already set to the
   * widget's dirty region, i.e. to the area that needs repainting.  Complicated
   * widgets that want to avoid redrawing themselves completely can get the full
   * extents of the clip region with cdk_cairo_get_clip_rectangle(), or they can
   * get a finer-grained representation of the dirty region with
   * cairo_copy_clip_rectangle_list().
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   * %FALSE to propagate the event further.
   *
   * Since: 3.0
   */
  widget_signals[DRAW] =
    g_signal_new (I_("draw"),
		   G_TYPE_FROM_CLASS (gobject_class),
		   G_SIGNAL_RUN_LAST,
		   G_STRUCT_OFFSET (CtkWidgetClass, draw),
                   _ctk_boolean_handled_accumulator, NULL,
                   ctk_widget_draw_marshaller,
		   G_TYPE_BOOLEAN, 1,
		   CAIRO_GOBJECT_TYPE_CONTEXT);
  g_signal_set_va_marshaller (widget_signals[DRAW], G_TYPE_FROM_CLASS (klass),
                              ctk_widget_draw_marshallerv);

  /**
   * CtkWidget::mnemonic-activate:
   * @widget: the object which received the signal.
   * @group_cycling: %TRUE if there are other widgets with the same mnemonic
   *
   * The default handler for this signal activates @widget if @group_cycling
   * is %FALSE, or just makes @widget grab focus if @group_cycling is %TRUE.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   * %FALSE to propagate the event further.
   */
  widget_signals[MNEMONIC_ACTIVATE] =
    g_signal_new (I_("mnemonic-activate"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, mnemonic_activate),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOOLEAN,
		  G_TYPE_BOOLEAN, 1,
		  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (widget_signals[MNEMONIC_ACTIVATE],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _ctk_marshal_BOOLEAN__BOOLEANv);

  /**
   * CtkWidget::grab-focus:
   * @widget: the object which received the signal.
   */
  widget_signals[GRAB_FOCUS] =
    g_signal_new (I_("grab-focus"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkWidgetClass, grab_focus),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkWidget::focus:
   * @widget: the object which received the signal.
   * @direction:
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. %FALSE to propagate the event further.
   */
  widget_signals[FOCUS] =
    g_signal_new (I_("focus"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, focus),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__ENUM,
		  G_TYPE_BOOLEAN, 1,
		  CTK_TYPE_DIRECTION_TYPE);
  g_signal_set_va_marshaller (widget_signals[FOCUS],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _ctk_marshal_BOOLEAN__ENUMv);

  /**
   * CtkWidget::move-focus:
   * @widget: the object which received the signal.
   * @direction:
   */
  widget_signals[MOVE_FOCUS] =
    g_signal_new (I_("move-focus"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkWidgetClass, move_focus),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  CTK_TYPE_DIRECTION_TYPE);

  /**
   * CtkWidget::keynav-failed:
   * @widget: the object which received the signal
   * @direction: the direction of movement
   *
   * Gets emitted if keyboard navigation fails.
   * See ctk_widget_keynav_failed() for details.
   *
   * Returns: %TRUE if stopping keyboard navigation is fine, %FALSE
   *          if the emitting widget should try to handle the keyboard
   *          navigation attempt in its parent container(s).
   *
   * Since: 2.12
   **/
  widget_signals[KEYNAV_FAILED] =
    g_signal_new (I_("keynav-failed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkWidgetClass, keynav_failed),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__ENUM,
                  G_TYPE_BOOLEAN, 1,
                  CTK_TYPE_DIRECTION_TYPE);
  g_signal_set_va_marshaller (widget_signals[KEYNAV_FAILED],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__ENUMv);

  /**
   * CtkWidget::event:
   * @widget: the object which received the signal.
   * @event: the #CdkEvent which triggered this signal
   *
   * The CTK+ main loop will emit three signals for each CDK event delivered
   * to a widget: one generic ::event signal, another, more specific,
   * signal that matches the type of event delivered (e.g.
   * #CtkWidget::key-press-event) and finally a generic
   * #CtkWidget::event-after signal.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event
   * and to cancel the emission of the second specific ::event signal.
   *   %FALSE to propagate the event further and to allow the emission of
   *   the second signal. The ::event-after signal is emitted regardless of
   *   the return value.
   */
  widget_signals[EVENT] =
    g_signal_new (I_("event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::event-after:
   * @widget: the object which received the signal.
   * @event: the #CdkEvent which triggered this signal
   *
   * After the emission of the #CtkWidget::event signal and (optionally)
   * the second more specific signal, ::event-after will be emitted
   * regardless of the previous two signals handlers return values.
   *
   */
  widget_signals[EVENT_AFTER] =
    g_signal_new (I_("event-after"),
		  G_TYPE_FROM_CLASS (klass),
		  0,
		  0,
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * CtkWidget::button-press-event:
   * @widget: the object which received the signal.
   * @event: (type Cdk.EventButton): the #CdkEventButton which triggered
   *   this signal.
   *
   * The ::button-press-event signal will be emitted when a button
   * (typically from a mouse) is pressed.
   *
   * To receive this signal, the #CdkWindow associated to the
   * widget needs to enable the #CDK_BUTTON_PRESS_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[BUTTON_PRESS_EVENT] =
    g_signal_new (I_("button-press-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, button_press_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[BUTTON_PRESS_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::button-release-event:
   * @widget: the object which received the signal.
   * @event: (type Cdk.EventButton): the #CdkEventButton which triggered
   *   this signal.
   *
   * The ::button-release-event signal will be emitted when a button
   * (typically from a mouse) is released.
   *
   * To receive this signal, the #CdkWindow associated to the
   * widget needs to enable the #CDK_BUTTON_RELEASE_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[BUTTON_RELEASE_EVENT] =
    g_signal_new (I_("button-release-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, button_release_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[BUTTON_RELEASE_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  widget_signals[TOUCH_EVENT] =
    g_signal_new (I_("touch-event"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkWidgetClass, touch_event),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__BOXED,
                  G_TYPE_BOOLEAN, 1,
                  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[TOUCH_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::scroll-event:
   * @widget: the object which received the signal.
   * @event: (type Cdk.EventScroll): the #CdkEventScroll which triggered
   *   this signal.
   *
   * The ::scroll-event signal is emitted when a button in the 4 to 7
   * range is pressed. Wheel mice are usually configured to generate
   * button press events for buttons 4 and 5 when the wheel is turned.
   *
   * To receive this signal, the #CdkWindow associated to the widget needs
   * to enable the #CDK_SCROLL_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[SCROLL_EVENT] =
    g_signal_new (I_("scroll-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, scroll_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[SCROLL_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::motion-notify-event:
   * @widget: the object which received the signal.
   * @event: (type Cdk.EventMotion): the #CdkEventMotion which triggered
   *   this signal.
   *
   * The ::motion-notify-event signal is emitted when the pointer moves
   * over the widget's #CdkWindow.
   *
   * To receive this signal, the #CdkWindow associated to the widget
   * needs to enable the #CDK_POINTER_MOTION_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[MOTION_NOTIFY_EVENT] =
    g_signal_new (I_("motion-notify-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, motion_notify_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[MOTION_NOTIFY_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::composited-changed:
   * @widget: the object on which the signal is emitted
   *
   * The ::composited-changed signal is emitted when the composited
   * status of @widgets screen changes.
   * See cdk_screen_is_composited().
   *
   * Deprecated: 3.22: Use CdkScreen::composited-changed instead.
   */
  widget_signals[COMPOSITED_CHANGED] =
    g_signal_new (I_("composited-changed"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION | G_SIGNAL_DEPRECATED,
		  G_STRUCT_OFFSET (CtkWidgetClass, composited_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkWidget::delete-event:
   * @widget: the object which received the signal
   * @event: the event which triggered this signal
   *
   * The ::delete-event signal is emitted if a user requests that
   * a toplevel window is closed. The default handler for this signal
   * destroys the window. Connecting ctk_widget_hide_on_delete() to
   * this signal will cause the window to be hidden instead, so that
   * it can later be shown again without reconstructing it.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[DELETE_EVENT] =
    g_signal_new (I_("delete-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, delete_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[DELETE_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::destroy-event:
   * @widget: the object which received the signal.
   * @event: the event which triggered this signal
   *
   * The ::destroy-event signal is emitted when a #CdkWindow is destroyed.
   * You rarely get this signal, because most widgets disconnect themselves
   * from their window before they destroy it, so no widget owns the
   * window at destroy time.
   *
   * To receive this signal, the #CdkWindow associated to the widget needs
   * to enable the #CDK_STRUCTURE_MASK mask. CDK will enable this mask
   * automatically for all new windows.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[DESTROY_EVENT] =
    g_signal_new (I_("destroy-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, destroy_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[DESTROY_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::key-press-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventKey): the #CdkEventKey which triggered this signal.
   *
   * The ::key-press-event signal is emitted when a key is pressed. The signal
   * emission will reoccur at the key-repeat rate when the key is kept pressed.
   *
   * To receive this signal, the #CdkWindow associated to the widget needs
   * to enable the #CDK_KEY_PRESS_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[KEY_PRESS_EVENT] =
    g_signal_new (I_("key-press-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, key_press_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[KEY_PRESS_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::key-release-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventKey): the #CdkEventKey which triggered this signal.
   *
   * The ::key-release-event signal is emitted when a key is released.
   *
   * To receive this signal, the #CdkWindow associated to the widget needs
   * to enable the #CDK_KEY_RELEASE_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[KEY_RELEASE_EVENT] =
    g_signal_new (I_("key-release-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, key_release_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[KEY_RELEASE_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::enter-notify-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventCrossing): the #CdkEventCrossing which triggered
   *   this signal.
   *
   * The ::enter-notify-event will be emitted when the pointer enters
   * the @widget's window.
   *
   * To receive this signal, the #CdkWindow associated to the widget needs
   * to enable the #CDK_ENTER_NOTIFY_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[ENTER_NOTIFY_EVENT] =
    g_signal_new (I_("enter-notify-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, enter_notify_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[ENTER_NOTIFY_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::leave-notify-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventCrossing): the #CdkEventCrossing which triggered
   *   this signal.
   *
   * The ::leave-notify-event will be emitted when the pointer leaves
   * the @widget's window.
   *
   * To receive this signal, the #CdkWindow associated to the widget needs
   * to enable the #CDK_LEAVE_NOTIFY_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[LEAVE_NOTIFY_EVENT] =
    g_signal_new (I_("leave-notify-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, leave_notify_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[LEAVE_NOTIFY_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::configure-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventConfigure): the #CdkEventConfigure which triggered
   *   this signal.
   *
   * The ::configure-event signal will be emitted when the size, position or
   * stacking of the @widget's window has changed.
   *
   * To receive this signal, the #CdkWindow associated to the widget needs
   * to enable the #CDK_STRUCTURE_MASK mask. CDK will enable this mask
   * automatically for all new windows.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[CONFIGURE_EVENT] =
    g_signal_new (I_("configure-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, configure_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[CONFIGURE_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::focus-in-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventFocus): the #CdkEventFocus which triggered
   *   this signal.
   *
   * The ::focus-in-event signal will be emitted when the keyboard focus
   * enters the @widget's window.
   *
   * To receive this signal, the #CdkWindow associated to the widget needs
   * to enable the #CDK_FOCUS_CHANGE_MASK mask.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[FOCUS_IN_EVENT] =
    g_signal_new (I_("focus-in-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, focus_in_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[FOCUS_IN_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::focus-out-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventFocus): the #CdkEventFocus which triggered this
   *   signal.
   *
   * The ::focus-out-event signal will be emitted when the keyboard focus
   * leaves the @widget's window.
   *
   * To receive this signal, the #CdkWindow associated to the widget needs
   * to enable the #CDK_FOCUS_CHANGE_MASK mask.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[FOCUS_OUT_EVENT] =
    g_signal_new (I_("focus-out-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, focus_out_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[FOCUS_OUT_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::map-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventAny): the #CdkEventAny which triggered this signal.
   *
   * The ::map-event signal will be emitted when the @widget's window is
   * mapped. A window is mapped when it becomes visible on the screen.
   *
   * To receive this signal, the #CdkWindow associated to the widget needs
   * to enable the #CDK_STRUCTURE_MASK mask. CDK will enable this mask
   * automatically for all new windows.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[MAP_EVENT] =
    g_signal_new (I_("map-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, map_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[MAP_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::unmap-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventAny): the #CdkEventAny which triggered this signal
   *
   * The ::unmap-event signal will be emitted when the @widget's window is
   * unmapped. A window is unmapped when it becomes invisible on the screen.
   *
   * To receive this signal, the #CdkWindow associated to the widget needs
   * to enable the #CDK_STRUCTURE_MASK mask. CDK will enable this mask
   * automatically for all new windows.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[UNMAP_EVENT] =
    g_signal_new (I_("unmap-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, unmap_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[UNMAP_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::property-notify-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventProperty): the #CdkEventProperty which triggered
   *   this signal.
   *
   * The ::property-notify-event signal will be emitted when a property on
   * the @widget's window has been changed or deleted.
   *
   * To receive this signal, the #CdkWindow associated to the widget needs
   * to enable the #CDK_PROPERTY_CHANGE_MASK mask.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[PROPERTY_NOTIFY_EVENT] =
    g_signal_new (I_("property-notify-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, property_notify_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[PROPERTY_NOTIFY_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::selection-clear-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventSelection): the #CdkEventSelection which triggered
   *   this signal.
   *
   * The ::selection-clear-event signal will be emitted when the
   * the @widget's window has lost ownership of a selection.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[SELECTION_CLEAR_EVENT] =
    g_signal_new (I_("selection-clear-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, selection_clear_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[SELECTION_CLEAR_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::selection-request-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventSelection): the #CdkEventSelection which triggered
   *   this signal.
   *
   * The ::selection-request-event signal will be emitted when
   * another client requests ownership of the selection owned by
   * the @widget's window.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[SELECTION_REQUEST_EVENT] =
    g_signal_new (I_("selection-request-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, selection_request_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[SELECTION_REQUEST_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::selection-notify-event:
   * @widget: the object which received the signal.
   * @event: (type Cdk.EventSelection):
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. %FALSE to propagate the event further.
   */
  widget_signals[SELECTION_NOTIFY_EVENT] =
    g_signal_new (I_("selection-notify-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, selection_notify_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[SELECTION_NOTIFY_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::selection-received:
   * @widget: the object which received the signal.
   * @data:
   * @time:
   */
  widget_signals[SELECTION_RECEIVED] =
    g_signal_new (I_("selection-received"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, selection_received),
		  NULL, NULL,
		  _ctk_marshal_VOID__BOXED_UINT,
		  G_TYPE_NONE, 2,
		  CTK_TYPE_SELECTION_DATA | G_SIGNAL_TYPE_STATIC_SCOPE,
		  G_TYPE_UINT);
  g_signal_set_va_marshaller (widget_signals[SELECTION_RECEIVED],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__BOXED_UINTv);

  /**
   * CtkWidget::selection-get:
   * @widget: the object which received the signal.
   * @data:
   * @info:
   * @time:
   */
  widget_signals[SELECTION_GET] =
    g_signal_new (I_("selection-get"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, selection_get),
		  NULL, NULL,
		  _ctk_marshal_VOID__BOXED_UINT_UINT,
		  G_TYPE_NONE, 3,
		  CTK_TYPE_SELECTION_DATA | G_SIGNAL_TYPE_STATIC_SCOPE,
		  G_TYPE_UINT,
		  G_TYPE_UINT);
  g_signal_set_va_marshaller (widget_signals[SELECTION_GET],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__BOXED_UINT_UINTv);

  /**
   * CtkWidget::proximity-in-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventProximity): the #CdkEventProximity which triggered
   *   this signal.
   *
   * To receive this signal the #CdkWindow associated to the widget needs
   * to enable the #CDK_PROXIMITY_IN_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[PROXIMITY_IN_EVENT] =
    g_signal_new (I_("proximity-in-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, proximity_in_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[PROXIMITY_IN_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::proximity-out-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventProximity): the #CdkEventProximity which triggered
   *   this signal.
   *
   * To receive this signal the #CdkWindow associated to the widget needs
   * to enable the #CDK_PROXIMITY_OUT_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[PROXIMITY_OUT_EVENT] =
    g_signal_new (I_("proximity-out-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, proximity_out_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[PROXIMITY_OUT_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::drag-leave:
   * @widget: the object which received the signal.
   * @context: the drag context
   * @time: the timestamp of the motion event
   *
   * The ::drag-leave signal is emitted on the drop site when the cursor
   * leaves the widget. A typical reason to connect to this signal is to
   * undo things done in #CtkWidget::drag-motion, e.g. undo highlighting
   * with ctk_drag_unhighlight().
   *
   *
   * Likewise, the #CtkWidget::drag-leave signal is also emitted before the 
   * ::drag-drop signal, for instance to allow cleaning up of a preview item  
   * created in the #CtkWidget::drag-motion signal handler.
   */
  widget_signals[DRAG_LEAVE] =
    g_signal_new (I_("drag-leave"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, drag_leave),
		  NULL, NULL,
		  _ctk_marshal_VOID__OBJECT_UINT,
		  G_TYPE_NONE, 2,
		  CDK_TYPE_DRAG_CONTEXT,
		  G_TYPE_UINT);
  g_signal_set_va_marshaller (widget_signals[DRAG_LEAVE],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__OBJECT_UINTv);

  /**
   * CtkWidget::drag-begin:
   * @widget: the object which received the signal
   * @context: the drag context
   *
   * The ::drag-begin signal is emitted on the drag source when a drag is
   * started. A typical reason to connect to this signal is to set up a
   * custom drag icon with e.g. ctk_drag_source_set_icon_pixbuf().
   *
   * Note that some widgets set up a drag icon in the default handler of
   * this signal, so you may have to use g_signal_connect_after() to
   * override what the default handler did.
   */
  widget_signals[DRAG_BEGIN] =
    g_signal_new (I_("drag-begin"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, drag_begin),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CDK_TYPE_DRAG_CONTEXT);

  /**
   * CtkWidget::drag-end:
   * @widget: the object which received the signal
   * @context: the drag context
   *
   * The ::drag-end signal is emitted on the drag source when a drag is
   * finished.  A typical reason to connect to this signal is to undo
   * things done in #CtkWidget::drag-begin.
   */
  widget_signals[DRAG_END] =
    g_signal_new (I_("drag-end"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, drag_end),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CDK_TYPE_DRAG_CONTEXT);

  /**
   * CtkWidget::drag-data-delete:
   * @widget: the object which received the signal
   * @context: the drag context
   *
   * The ::drag-data-delete signal is emitted on the drag source when a drag
   * with the action %CDK_ACTION_MOVE is successfully completed. The signal
   * handler is responsible for deleting the data that has been dropped. What
   * "delete" means depends on the context of the drag operation.
   */
  widget_signals[DRAG_DATA_DELETE] =
    g_signal_new (I_("drag-data-delete"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, drag_data_delete),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CDK_TYPE_DRAG_CONTEXT);

  /**
   * CtkWidget::drag-failed:
   * @widget: the object which received the signal
   * @context: the drag context
   * @result: the result of the drag operation
   *
   * The ::drag-failed signal is emitted on the drag source when a drag has
   * failed. The signal handler may hook custom code to handle a failed DnD
   * operation based on the type of error, it returns %TRUE is the failure has
   * been already handled (not showing the default "drag operation failed"
   * animation), otherwise it returns %FALSE.
   *
   * Returns: %TRUE if the failed drag operation has been already handled.
   *
   * Since: 2.12
   */
  widget_signals[DRAG_FAILED] =
    g_signal_new (I_("drag-failed"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, drag_failed),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__OBJECT_ENUM,
		  G_TYPE_BOOLEAN, 2,
		  CDK_TYPE_DRAG_CONTEXT,
		  CTK_TYPE_DRAG_RESULT);
  g_signal_set_va_marshaller (widget_signals[DRAG_FAILED],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__OBJECT_ENUMv);

  /**
   * CtkWidget::drag-motion:
   * @widget: the object which received the signal
   * @context: the drag context
   * @x: the x coordinate of the current cursor position
   * @y: the y coordinate of the current cursor position
   * @time: the timestamp of the motion event
   *
   * The ::drag-motion signal is emitted on the drop site when the user
   * moves the cursor over the widget during a drag. The signal handler
   * must determine whether the cursor position is in a drop zone or not.
   * If it is not in a drop zone, it returns %FALSE and no further processing
   * is necessary. Otherwise, the handler returns %TRUE. In this case, the
   * handler is responsible for providing the necessary information for
   * displaying feedback to the user, by calling cdk_drag_status().
   *
   * If the decision whether the drop will be accepted or rejected can't be
   * made based solely on the cursor position and the type of the data, the
   * handler may inspect the dragged data by calling ctk_drag_get_data() and
   * defer the cdk_drag_status() call to the #CtkWidget::drag-data-received
   * handler. Note that you must pass #CTK_DEST_DEFAULT_DROP,
   * #CTK_DEST_DEFAULT_MOTION or #CTK_DEST_DEFAULT_ALL to ctk_drag_dest_set()
   * when using the drag-motion signal that way.
   *
   * Also note that there is no drag-enter signal. The drag receiver has to
   * keep track of whether he has received any drag-motion signals since the
   * last #CtkWidget::drag-leave and if not, treat the drag-motion signal as
   * an "enter" signal. Upon an "enter", the handler will typically highlight
   * the drop site with ctk_drag_highlight().
   * |[<!-- language="C" -->
   * static void
   * drag_motion (CtkWidget      *widget,
   *              CdkDragContext *context,
   *              gint            x,
   *              gint            y,
   *              guint           time)
   * {
   *   CdkAtom target;
   *
   *   PrivateData *private_data = GET_PRIVATE_DATA (widget);
   *
   *   if (!private_data->drag_highlight)
   *    {
   *      private_data->drag_highlight = 1;
   *      ctk_drag_highlight (widget);
   *    }
   *
   *   target = ctk_drag_dest_find_target (widget, context, NULL);
   *   if (target == CDK_NONE)
   *     cdk_drag_status (context, 0, time);
   *   else
   *    {
   *      private_data->pending_status
   *         = cdk_drag_context_get_suggested_action (context);
   *      ctk_drag_get_data (widget, context, target, time);
   *    }
   *
   *   return TRUE;
   * }
   *
   * static void
   * drag_data_received (CtkWidget        *widget,
   *                     CdkDragContext   *context,
   *                     gint              x,
   *                     gint              y,
   *                     CtkSelectionData *selection_data,
   *                     guint             info,
   *                     guint             time)
   * {
   *   PrivateData *private_data = GET_PRIVATE_DATA (widget);
   *
   *   if (private_data->suggested_action)
   *    {
   *      private_data->suggested_action = 0;
   *
   *      // We are getting this data due to a request in drag_motion,
   *      // rather than due to a request in drag_drop, so we are just
   *      // supposed to call cdk_drag_status(), not actually paste in
   *      // the data.
   *
   *      str = ctk_selection_data_get_text (selection_data);
   *      if (!data_is_acceptable (str))
   *        cdk_drag_status (context, 0, time);
   *      else
   *        cdk_drag_status (context,
   *                         private_data->suggested_action,
   *                         time);
   *    }
   *   else
   *    {
   *      // accept the drop
   *    }
   * }
   * ]|
   *
   * Returns: whether the cursor position is in a drop zone
   */
  widget_signals[DRAG_MOTION] =
    g_signal_new (I_("drag-motion"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, drag_motion),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__OBJECT_INT_INT_UINT,
		  G_TYPE_BOOLEAN, 4,
		  CDK_TYPE_DRAG_CONTEXT,
		  G_TYPE_INT,
		  G_TYPE_INT,
		  G_TYPE_UINT);
  g_signal_set_va_marshaller (widget_signals[DRAG_MOTION],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__OBJECT_INT_INT_UINTv);

  /**
   * CtkWidget::drag-drop:
   * @widget: the object which received the signal
   * @context: the drag context
   * @x: the x coordinate of the current cursor position
   * @y: the y coordinate of the current cursor position
   * @time: the timestamp of the motion event
   *
   * The ::drag-drop signal is emitted on the drop site when the user drops
   * the data onto the widget. The signal handler must determine whether
   * the cursor position is in a drop zone or not. If it is not in a drop
   * zone, it returns %FALSE and no further processing is necessary.
   * Otherwise, the handler returns %TRUE. In this case, the handler must
   * ensure that ctk_drag_finish() is called to let the source know that
   * the drop is done. The call to ctk_drag_finish() can be done either
   * directly or in a #CtkWidget::drag-data-received handler which gets
   * triggered by calling ctk_drag_get_data() to receive the data for one
   * or more of the supported targets.
   *
   * Returns: whether the cursor position is in a drop zone
   */
  widget_signals[DRAG_DROP] =
    g_signal_new (I_("drag-drop"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, drag_drop),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__OBJECT_INT_INT_UINT,
		  G_TYPE_BOOLEAN, 4,
		  CDK_TYPE_DRAG_CONTEXT,
		  G_TYPE_INT,
		  G_TYPE_INT,
		  G_TYPE_UINT);
  g_signal_set_va_marshaller (widget_signals[DRAG_DROP],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__OBJECT_INT_INT_UINTv);

  /**
   * CtkWidget::drag-data-get:
   * @widget: the object which received the signal
   * @context: the drag context
   * @data: the #CtkSelectionData to be filled with the dragged data
   * @info: the info that has been registered with the target in the
   *        #CtkTargetList
   * @time: the timestamp at which the data was requested
   *
   * The ::drag-data-get signal is emitted on the drag source when the drop
   * site requests the data which is dragged. It is the responsibility of
   * the signal handler to fill @data with the data in the format which
   * is indicated by @info. See ctk_selection_data_set() and
   * ctk_selection_data_set_text().
   */
  widget_signals[DRAG_DATA_GET] =
    g_signal_new (I_("drag-data-get"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, drag_data_get),
		  NULL, NULL,
		  _ctk_marshal_VOID__OBJECT_BOXED_UINT_UINT,
		  G_TYPE_NONE, 4,
		  CDK_TYPE_DRAG_CONTEXT,
		  CTK_TYPE_SELECTION_DATA | G_SIGNAL_TYPE_STATIC_SCOPE,
		  G_TYPE_UINT,
		  G_TYPE_UINT);
  g_signal_set_va_marshaller (widget_signals[DRAG_DATA_GET],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__OBJECT_BOXED_UINT_UINTv);

  /**
   * CtkWidget::drag-data-received:
   * @widget: the object which received the signal
   * @context: the drag context
   * @x: where the drop happened
   * @y: where the drop happened
   * @data: the received data
   * @info: the info that has been registered with the target in the
   *        #CtkTargetList
   * @time: the timestamp at which the data was received
   *
   * The ::drag-data-received signal is emitted on the drop site when the
   * dragged data has been received. If the data was received in order to
   * determine whether the drop will be accepted, the handler is expected
   * to call cdk_drag_status() and not finish the drag.
   * If the data was received in response to a #CtkWidget::drag-drop signal
   * (and this is the last target to be received), the handler for this
   * signal is expected to process the received data and then call
   * ctk_drag_finish(), setting the @success parameter depending on
   * whether the data was processed successfully.
   *
   * Applications must create some means to determine why the signal was emitted 
   * and therefore whether to call cdk_drag_status() or ctk_drag_finish(). 
   *
   * The handler may inspect the selected action with
   * cdk_drag_context_get_selected_action() before calling
   * ctk_drag_finish(), e.g. to implement %CDK_ACTION_ASK as
   * shown in the following example:
   * |[<!-- language="C" -->
   * void
   * drag_data_received (CtkWidget          *widget,
   *                     CdkDragContext     *context,
   *                     gint                x,
   *                     gint                y,
   *                     CtkSelectionData   *data,
   *                     guint               info,
   *                     guint               time)
   * {
   *   if ((data->length >= 0) && (data->format == 8))
   *     {
   *       CdkDragAction action;
   *
   *       // handle data here
   *
   *       action = cdk_drag_context_get_selected_action (context);
   *       if (action == CDK_ACTION_ASK)
   *         {
   *           CtkWidget *dialog;
   *           gint response;
   *
   *           dialog = ctk_message_dialog_new (NULL,
   *                                            CTK_DIALOG_MODAL |
   *                                            CTK_DIALOG_DESTROY_WITH_PARENT,
   *                                            CTK_MESSAGE_INFO,
   *                                            CTK_BUTTONS_YES_NO,
   *                                            "Move the data ?\n");
   *           response = ctk_dialog_run (CTK_DIALOG (dialog));
   *           ctk_widget_destroy (dialog);
   *
   *           if (response == CTK_RESPONSE_YES)
   *             action = CDK_ACTION_MOVE;
   *           else
   *             action = CDK_ACTION_COPY;
   *          }
   *
   *       ctk_drag_finish (context, TRUE, action == CDK_ACTION_MOVE, time);
   *     }
   *   else
   *     ctk_drag_finish (context, FALSE, FALSE, time);
   *  }
   * ]|
   */
  widget_signals[DRAG_DATA_RECEIVED] =
    g_signal_new (I_("drag-data-received"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, drag_data_received),
		  NULL, NULL,
		  _ctk_marshal_VOID__OBJECT_INT_INT_BOXED_UINT_UINT,
		  G_TYPE_NONE, 6,
		  CDK_TYPE_DRAG_CONTEXT,
		  G_TYPE_INT,
		  G_TYPE_INT,
		  CTK_TYPE_SELECTION_DATA | G_SIGNAL_TYPE_STATIC_SCOPE,
		  G_TYPE_UINT,
		  G_TYPE_UINT);
   g_signal_set_va_marshaller (widget_signals[DRAG_DATA_RECEIVED],
                               G_TYPE_FROM_CLASS (klass),
                               _ctk_marshal_VOID__OBJECT_INT_INT_BOXED_UINT_UINTv);

  /**
   * CtkWidget::visibility-notify-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventVisibility): the #CdkEventVisibility which
   *   triggered this signal.
   *
   * The ::visibility-notify-event will be emitted when the @widget's
   * window is obscured or unobscured.
   *
   * To receive this signal the #CdkWindow associated to the widget needs
   * to enable the #CDK_VISIBILITY_NOTIFY_MASK mask.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   *
   * Deprecated: 3.12: Modern composited windowing systems with pervasive
   *     transparency make it impossible to track the visibility of a window
   *     reliably, so this signal can not be guaranteed to provide useful
   *     information.
   */
  widget_signals[VISIBILITY_NOTIFY_EVENT] =
    g_signal_new (I_("visibility-notify-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_DEPRECATED,
		  G_STRUCT_OFFSET (CtkWidgetClass, visibility_notify_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[VISIBILITY_NOTIFY_EVENT],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::window-state-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventWindowState): the #CdkEventWindowState which
   *   triggered this signal.
   *
   * The ::window-state-event will be emitted when the state of the
   * toplevel window associated to the @widget changes.
   *
   * To receive this signal the #CdkWindow associated to the widget
   * needs to enable the #CDK_STRUCTURE_MASK mask. CDK will enable
   * this mask automatically for all new windows.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the
   *   event. %FALSE to propagate the event further.
   */
  widget_signals[WINDOW_STATE_EVENT] =
    g_signal_new (I_("window-state-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, window_state_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[WINDOW_STATE_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::damage-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventExpose): the #CdkEventExpose event
   *
   * Emitted when a redirected window belonging to @widget gets drawn into.
   * The region/area members of the event shows what area of the redirected
   * drawable was drawn into.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   *
   * Since: 2.14
   */
  widget_signals[DAMAGE_EVENT] =
    g_signal_new (I_("damage-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, damage_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[DAMAGE_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

/**
   * CtkWidget::grab-broken-event:
   * @widget: the object which received the signal
   * @event: (type Cdk.EventGrabBroken): the #CdkEventGrabBroken event
   *
   * Emitted when a pointer or keyboard grab on a window belonging
   * to @widget gets broken.
   *
   * On X11, this happens when the grab window becomes unviewable
   * (i.e. it or one of its ancestors is unmapped), or if the same
   * application grabs the pointer or keyboard again.
   *
   * Returns: %TRUE to stop other handlers from being invoked for
   *   the event. %FALSE to propagate the event further.
   *
   * Since: 2.8
   */
  widget_signals[GRAB_BROKEN_EVENT] =
    g_signal_new (I_("grab-broken-event"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, grab_broken_event),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (widget_signals[GRAB_BROKEN_EVENT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__BOXEDv);

  /**
   * CtkWidget::query-tooltip:
   * @widget: the object which received the signal
   * @x: the x coordinate of the cursor position where the request has
   *     been emitted, relative to @widget's left side
   * @y: the y coordinate of the cursor position where the request has
   *     been emitted, relative to @widget's top
   * @keyboard_mode: %TRUE if the tooltip was triggered using the keyboard
   * @tooltip: a #CtkTooltip
   *
   * Emitted when #CtkWidget:has-tooltip is %TRUE and the hover timeout
   * has expired with the cursor hovering "above" @widget; or emitted when @widget got
   * focus in keyboard mode.
   *
   * Using the given coordinates, the signal handler should determine
   * whether a tooltip should be shown for @widget. If this is the case
   * %TRUE should be returned, %FALSE otherwise.  Note that if
   * @keyboard_mode is %TRUE, the values of @x and @y are undefined and
   * should not be used.
   *
   * The signal handler is free to manipulate @tooltip with the therefore
   * destined function calls.
   *
   * Returns: %TRUE if @tooltip should be shown right now, %FALSE otherwise.
   *
   * Since: 2.12
   */
  widget_signals[QUERY_TOOLTIP] =
    g_signal_new (I_("query-tooltip"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, query_tooltip),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__INT_INT_BOOLEAN_OBJECT,
		  G_TYPE_BOOLEAN, 4,
		  G_TYPE_INT,
		  G_TYPE_INT,
		  G_TYPE_BOOLEAN,
		  CTK_TYPE_TOOLTIP);
  g_signal_set_va_marshaller (widget_signals[QUERY_TOOLTIP],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__INT_INT_BOOLEAN_OBJECTv);

  /**
   * CtkWidget::popup-menu:
   * @widget: the object which received the signal
   *
   * This signal gets emitted whenever a widget should pop up a context
   * menu. This usually happens through the standard key binding mechanism;
   * by pressing a certain key while a widget is focused, the user can cause
   * the widget to pop up a menu.  For example, the #CtkEntry widget creates
   * a menu with clipboard commands. See the
   * [Popup Menu Migration Checklist][checklist-popup-menu]
   * for an example of how to use this signal.
   *
   * Returns: %TRUE if a menu was activated
   */
  widget_signals[POPUP_MENU] =
    g_signal_new (I_("popup-menu"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkWidgetClass, popup_menu),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (widget_signals[POPUP_MENU],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__VOIDv);

  /**
   * CtkWidget::show-help:
   * @widget: the object which received the signal.
   * @help_type:
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   * %FALSE to propagate the event further.
   */
  widget_signals[SHOW_HELP] =
    g_signal_new (I_("show-help"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkWidgetClass, show_help),
		  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__ENUM,
		  G_TYPE_BOOLEAN, 1,
		  CTK_TYPE_WIDGET_HELP_TYPE);
  g_signal_set_va_marshaller (widget_signals[SHOW_HELP],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__ENUMv);

  /**
   * CtkWidget::accel-closures-changed:
   * @widget: the object which received the signal.
   */
  widget_signals[ACCEL_CLOSURES_CHANGED] =
    g_signal_new (I_("accel-closures-changed"),
		  G_TYPE_FROM_CLASS (klass),
		  0,
		  0,
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkWidget::screen-changed:
   * @widget: the object on which the signal is emitted
   * @previous_screen: (allow-none): the previous screen, or %NULL if the
   *   widget was not associated with a screen before
   *
   * The ::screen-changed signal gets emitted when the
   * screen of a widget has changed.
   */
  widget_signals[SCREEN_CHANGED] =
    g_signal_new (I_("screen-changed"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, screen_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CDK_TYPE_SCREEN);

  /**
   * CtkWidget::can-activate-accel:
   * @widget: the object which received the signal
   * @signal_id: the ID of a signal installed on @widget
   *
   * Determines whether an accelerator that activates the signal
   * identified by @signal_id can currently be activated.
   * This signal is present to allow applications and derived
   * widgets to override the default #CtkWidget handling
   * for determining whether an accelerator can be activated.
   *
   * Returns: %TRUE if the signal can be activated.
   */
  widget_signals[CAN_ACTIVATE_ACCEL] =
     g_signal_new (I_("can-activate-accel"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkWidgetClass, can_activate_accel),
                  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__UINT,
                  G_TYPE_BOOLEAN, 1, G_TYPE_UINT);

  binding_set = ctk_binding_set_by_class (klass);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_F10, CDK_SHIFT_MASK,
                                "popup-menu", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Menu, 0,
                                "popup-menu", 0);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_F1, CDK_CONTROL_MASK,
                                "show-help", 1,
                                CTK_TYPE_WIDGET_HELP_TYPE,
                                CTK_WIDGET_HELP_TOOLTIP);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_F1, CDK_CONTROL_MASK,
                                "show-help", 1,
                                CTK_TYPE_WIDGET_HELP_TYPE,
                                CTK_WIDGET_HELP_TOOLTIP);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_F1, CDK_SHIFT_MASK,
                                "show-help", 1,
                                CTK_TYPE_WIDGET_HELP_TYPE,
                                CTK_WIDGET_HELP_WHATS_THIS);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_F1, CDK_SHIFT_MASK,
                                "show-help", 1,
                                CTK_TYPE_WIDGET_HELP_TYPE,
                                CTK_WIDGET_HELP_WHATS_THIS);

  /**
   * CtkWidget:interior-focus:
   *
   * The "interior-focus" style property defines whether
   * to draw the focus indicator inside widgets.
   *
   * Deprecated: 3.14: use the outline CSS properties instead.
   */
  ctk_widget_class_install_style_property (klass,
					   g_param_spec_boolean ("interior-focus",
								 P_("Interior Focus"),
								 P_("Whether to draw the focus indicator inside widgets"),
								 TRUE,
								 CTK_PARAM_READABLE | G_PARAM_DEPRECATED));
  /**
   * CtkWidget:focus-line-width:
   *
   * The "focus-line-width" style property defines the width,
   * in pixels, of the focus indicator line
   *
   * Deprecated: 3.14: use the outline-width and padding CSS properties instead.
   */
  ctk_widget_class_install_style_property (klass,
					   g_param_spec_int ("focus-line-width",
							     P_("Focus linewidth"),
							     P_("Width, in pixels, of the focus indicator line"),
							     0, G_MAXINT, 1,
							     CTK_PARAM_READABLE | G_PARAM_DEPRECATED));
  /**
   * CtkWidget:focus-line-pattern:
   *
   * The "focus-line-pattern" style property defines the dash pattern used to
   * draw the focus indicator. The character values are interpreted as pixel
   * widths of alternating on and off segments of the line.
   *
   * Deprecated: 3.14: use the outline-style CSS property instead.
   */
  ctk_widget_class_install_style_property (klass,
					   g_param_spec_string ("focus-line-pattern",
								P_("Focus line dash pattern"),
								P_("Dash pattern used to draw the focus indicator. The character values are interpreted as pixel widths of alternating on and off segments of the line."),
								"\1\1",
								CTK_PARAM_READABLE | G_PARAM_DEPRECATED));
  /**
   * CtkWidget:focus-padding:
   *
   * The "focus-padding" style property defines the width, in pixels,
   * between focus indicator and the widget 'box'.
   *
   * Deprecated: 3.14: use the outline-offset CSS properties instead.
   */
  ctk_widget_class_install_style_property (klass,
					   g_param_spec_int ("focus-padding",
							     P_("Focus padding"),
							     P_("Width, in pixels, between focus indicator and the widget 'box'"),
							     0, G_MAXINT, 1,
							     CTK_PARAM_READABLE | G_PARAM_DEPRECATED));
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  /**
   * CtkWidget:cursor-color:
   *
   * The color with which to draw the insertion cursor in entries and
   * text views.
   *
   * Deprecated: 3.20: Use the caret-color CSS property
   */
  ctk_widget_class_install_style_property (klass,
					   g_param_spec_boxed ("cursor-color",
							       P_("Cursor color"),
							       P_("Color with which to draw insertion cursor"),
							       CDK_TYPE_COLOR,
							       CTK_PARAM_READABLE|G_PARAM_DEPRECATED));
  /**
   * CtkWidget:secondary-cursor-color:
   *
   * The color with which to draw the secondary insertion cursor in entries and
   * text views when editing mixed right-to-left and left-to-right text.
   *
   * Deprecated: 3.20: Use the -ctk-secondary-caret-color CSS property
   */
  ctk_widget_class_install_style_property (klass,
					   g_param_spec_boxed ("secondary-cursor-color",
							       P_("Secondary cursor color"),
							       P_("Color with which to draw the secondary insertion cursor when editing mixed right-to-left and left-to-right text"),
							       CDK_TYPE_COLOR,
							       CTK_PARAM_READABLE|G_PARAM_DEPRECATED));
G_GNUC_END_IGNORE_DEPRECATIONS
  ctk_widget_class_install_style_property (klass,
					   g_param_spec_float ("cursor-aspect-ratio",
							       P_("Cursor line aspect ratio"),
							       P_("Aspect ratio with which to draw insertion cursor"),
							       0.0, 1.0, 0.04,
							       CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (klass,
                                           g_param_spec_boolean ("window-dragging",
                                                                 P_("Window dragging"),
                                                                 P_("Whether windows can be dragged and maximized by clicking on empty areas"),
                                                                 FALSE,
                                                                 CTK_PARAM_READABLE));

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  /**
   * CtkWidget:link-color:
   *
   * The "link-color" style property defines the color of unvisited links.
   *
   * Since: 2.10
   *
   * Deprecated: 3.12: Links now use a separate state flags for selecting
   *     different theming, this style property is ignored
   */
  ctk_widget_class_install_style_property (klass,
					   g_param_spec_boxed ("link-color",
							       P_("Unvisited Link Color"),
							       P_("Color of unvisited links"),
							       CDK_TYPE_COLOR,
							       CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkWidget:visited-link-color:
   *
   * The "visited-link-color" style property defines the color of visited links.
   *
   * Since: 2.10
   *
   * Deprecated: 3.12: Links now use a separate state flags for selecting
   *     different theming, this style property is ignored
   */
  ctk_widget_class_install_style_property (klass,
					   g_param_spec_boxed ("visited-link-color",
							       P_("Visited Link Color"),
							       P_("Color of visited links"),
							       CDK_TYPE_COLOR,
							       CTK_PARAM_READABLE|G_PARAM_DEPRECATED));
G_GNUC_END_IGNORE_DEPRECATIONS

  /**
   * CtkWidget:wide-separators:
   *
   * The "wide-separators" style property defines whether separators have
   * configurable width and should be drawn using a box instead of a line.
   *
   * Since: 2.10
   *
   * Deprecated: 3.20: Use CSS properties on the separator elements to style
   *   separators; the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (klass,
                                           g_param_spec_boolean ("wide-separators",
                                                                 P_("Wide Separators"),
                                                                 P_("Whether separators have configurable width and should be drawn using a box instead of a line"),
                                                                 FALSE,
                                                                 CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkWidget:separator-width:
   *
   * The "separator-width" style property defines the width of separators.
   * This property only takes effect if the "wide-separators" style property is %TRUE.
   *
   * Since: 2.10
   *
   * Deprecated: 3.20: Use the standard min-width CSS property on the separator
   *   elements to size separators; the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (klass,
                                           g_param_spec_int ("separator-width",
                                                             P_("Separator Width"),
                                                             P_("The width of separators if wide-separators is TRUE"),
                                                             0, G_MAXINT, 0,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkWidget:separator-height:
   *
   * The "separator-height" style property defines the height of separators.
   * This property only takes effect if the "wide-separators" style property is %TRUE.
   *
   * Since: 2.10
   *
   * Deprecated: 3.20: Use the standard min-height CSS property on the separator
   *   elements to size separators; the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (klass,
                                           g_param_spec_int ("separator-height",
                                                             P_("Separator Height"),
                                                             P_("The height of separators if \"wide-separators\" is TRUE"),
                                                             0, G_MAXINT, 0,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkWidget:scroll-arrow-hlength:
   *
   * The "scroll-arrow-hlength" style property defines the length of
   * horizontal scroll arrows.
   *
   * Since: 2.10
   */
  ctk_widget_class_install_style_property (klass,
                                           g_param_spec_int ("scroll-arrow-hlength",
                                                             P_("Horizontal Scroll Arrow Length"),
                                                             P_("The length of horizontal scroll arrows"),
                                                             1, G_MAXINT, 16,
                                                             CTK_PARAM_READABLE));

  /**
   * CtkWidget:scroll-arrow-vlength:
   *
   * The "scroll-arrow-vlength" style property defines the length of
   * vertical scroll arrows.
   *
   * Since: 2.10
   */
  ctk_widget_class_install_style_property (klass,
                                           g_param_spec_int ("scroll-arrow-vlength",
                                                             P_("Vertical Scroll Arrow Length"),
                                                             P_("The length of vertical scroll arrows"),
                                                             1, G_MAXINT, 16,
                                                             CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (klass,
                                           g_param_spec_int ("text-handle-width",
                                                             P_("Width of text selection handles"),
                                                             P_("Width of text selection handles"),
                                                             1, G_MAXINT, 16,
                                                             CTK_PARAM_READABLE));
  ctk_widget_class_install_style_property (klass,
                                           g_param_spec_int ("text-handle-height",
                                                             P_("Height of text selection handles"),
                                                             P_("Height of text selection handles"),
                                                             1, G_MAXINT, 20,
                                                             CTK_PARAM_READABLE));

  ctk_widget_class_set_accessible_type (klass, CTK_TYPE_WIDGET_ACCESSIBLE);
  ctk_widget_class_set_css_name (klass, "widget");
}

static void
ctk_widget_base_class_finalize (CtkWidgetClass *klass)
{
  GList *list, *node;

  list = g_param_spec_pool_list_owned (style_property_spec_pool, G_OBJECT_CLASS_TYPE (klass));
  for (node = list; node; node = node->next)
    {
      GParamSpec *pspec = node->data;

      g_param_spec_pool_remove (style_property_spec_pool, pspec);
      g_param_spec_unref (pspec);
    }
  g_list_free (list);

  template_data_free (klass->priv->template);
}

static void
ctk_widget_set_property (GObject         *object,
			 guint            prop_id,
			 const GValue    *value,
			 GParamSpec      *pspec)
{
  CtkWidget *widget = CTK_WIDGET (object);

  switch (prop_id)
    {
      gboolean tmp;
      gchar *tooltip_markup;
      const gchar *tooltip_text;
      CtkWindow *tooltip_window;

    case PROP_NAME:
      ctk_widget_set_name (widget, g_value_get_string (value));
      break;
    case PROP_PARENT:
      ctk_container_add (CTK_CONTAINER (g_value_get_object (value)), widget);
      break;
    case PROP_WIDTH_REQUEST:
      ctk_widget_set_usize_internal (widget, g_value_get_int (value), -2);
      break;
    case PROP_HEIGHT_REQUEST:
      ctk_widget_set_usize_internal (widget, -2, g_value_get_int (value));
      break;
    case PROP_VISIBLE:
      ctk_widget_set_visible (widget, g_value_get_boolean (value));
      break;
    case PROP_SENSITIVE:
      ctk_widget_set_sensitive (widget, g_value_get_boolean (value));
      break;
    case PROP_APP_PAINTABLE:
      ctk_widget_set_app_paintable (widget, g_value_get_boolean (value));
      break;
    case PROP_CAN_FOCUS:
      ctk_widget_set_can_focus (widget, g_value_get_boolean (value));
      break;
    case PROP_HAS_FOCUS:
      if (g_value_get_boolean (value))
	ctk_widget_grab_focus (widget);
      break;
    case PROP_IS_FOCUS:
      if (g_value_get_boolean (value))
	ctk_widget_grab_focus (widget);
      break;
    case PROP_FOCUS_ON_CLICK:
      ctk_widget_set_focus_on_click (widget, g_value_get_boolean (value));
      break;
    case PROP_CAN_DEFAULT:
      ctk_widget_set_can_default (widget, g_value_get_boolean (value));
      break;
    case PROP_HAS_DEFAULT:
      if (g_value_get_boolean (value))
	ctk_widget_grab_default (widget);
      break;
    case PROP_RECEIVES_DEFAULT:
      ctk_widget_set_receives_default (widget, g_value_get_boolean (value));
      break;
    case PROP_STYLE:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_widget_set_style (widget, g_value_get_object (value));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case PROP_EVENTS:
      if (!_ctk_widget_get_realized (widget) && _ctk_widget_get_has_window (widget))
	ctk_widget_set_events (widget, g_value_get_flags (value));
      break;
    case PROP_NO_SHOW_ALL:
      ctk_widget_set_no_show_all (widget, g_value_get_boolean (value));
      break;
    case PROP_HAS_TOOLTIP:
      ctk_widget_real_set_has_tooltip (widget,
				       g_value_get_boolean (value), FALSE);
      break;
    case PROP_TOOLTIP_MARKUP:
      tooltip_window = g_object_get_qdata (object, quark_tooltip_window);
      tooltip_markup = g_value_dup_string (value);

      /* Treat an empty string as a NULL string,
       * because an empty string would be useless for a tooltip:
       */
      if (tooltip_markup && (strlen (tooltip_markup) == 0))
        {
	  g_free (tooltip_markup);
          tooltip_markup = NULL;
        }

      g_object_set_qdata_full (object, quark_tooltip_markup,
			       tooltip_markup, g_free);

      tmp = (tooltip_window != NULL || tooltip_markup != NULL);
      ctk_widget_real_set_has_tooltip (widget, tmp, FALSE);
      if (_ctk_widget_get_visible (widget))
        ctk_widget_queue_tooltip_query (widget);
      break;
    case PROP_TOOLTIP_TEXT:
      tooltip_window = g_object_get_qdata (object, quark_tooltip_window);

      tooltip_text = g_value_get_string (value);

      /* Treat an empty string as a NULL string,
       * because an empty string would be useless for a tooltip:
       */
      if (tooltip_text && (strlen (tooltip_text) == 0))
        tooltip_text = NULL;

      tooltip_markup = tooltip_text ? g_markup_escape_text (tooltip_text, -1) : NULL;

      g_object_set_qdata_full (object, quark_tooltip_markup,
                               tooltip_markup, g_free);

      tmp = (tooltip_window != NULL || tooltip_markup != NULL);
      ctk_widget_real_set_has_tooltip (widget, tmp, FALSE);
      if (_ctk_widget_get_visible (widget))
        ctk_widget_queue_tooltip_query (widget);
      break;
    case PROP_DOUBLE_BUFFERED:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_widget_set_double_buffered (widget, g_value_get_boolean (value));
      G_GNUC_END_IGNORE_DEPRECATIONS
      break;
    case PROP_HALIGN:
      ctk_widget_set_halign (widget, g_value_get_enum (value));
      break;
    case PROP_VALIGN:
      ctk_widget_set_valign (widget, g_value_get_enum (value));
      break;
    case PROP_MARGIN_LEFT:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_widget_set_margin_left (widget, g_value_get_int (value));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case PROP_MARGIN_RIGHT:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_widget_set_margin_right (widget, g_value_get_int (value));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case PROP_MARGIN_START:
      ctk_widget_set_margin_start (widget, g_value_get_int (value));
      break;
    case PROP_MARGIN_END:
      ctk_widget_set_margin_end (widget, g_value_get_int (value));
      break;
    case PROP_MARGIN_TOP:
      ctk_widget_set_margin_top (widget, g_value_get_int (value));
      break;
    case PROP_MARGIN_BOTTOM:
      ctk_widget_set_margin_bottom (widget, g_value_get_int (value));
      break;
    case PROP_MARGIN:
      g_object_freeze_notify (G_OBJECT (widget));
      ctk_widget_set_margin_start (widget, g_value_get_int (value));
      ctk_widget_set_margin_end (widget, g_value_get_int (value));
      ctk_widget_set_margin_top (widget, g_value_get_int (value));
      ctk_widget_set_margin_bottom (widget, g_value_get_int (value));
      g_object_thaw_notify (G_OBJECT (widget));
      break;
    case PROP_HEXPAND:
      ctk_widget_set_hexpand (widget, g_value_get_boolean (value));
      break;
    case PROP_HEXPAND_SET:
      ctk_widget_set_hexpand_set (widget, g_value_get_boolean (value));
      break;
    case PROP_VEXPAND:
      ctk_widget_set_vexpand (widget, g_value_get_boolean (value));
      break;
    case PROP_VEXPAND_SET:
      ctk_widget_set_vexpand_set (widget, g_value_get_boolean (value));
      break;
    case PROP_EXPAND:
      g_object_freeze_notify (G_OBJECT (widget));
      ctk_widget_set_hexpand (widget, g_value_get_boolean (value));
      ctk_widget_set_vexpand (widget, g_value_get_boolean (value));
      g_object_thaw_notify (G_OBJECT (widget));
      break;
    case PROP_OPACITY:
      ctk_widget_set_opacity (widget, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_widget_get_property (GObject         *object,
			 guint            prop_id,
			 GValue          *value,
			 GParamSpec      *pspec)
{
  CtkWidget *widget = CTK_WIDGET (object);
  CtkWidgetPrivate *priv = widget->priv;

  switch (prop_id)
    {
      gpointer *eventp;

    case PROP_NAME:
      if (priv->name)
	g_value_set_string (value, priv->name);
      else
	g_value_set_static_string (value, "");
      break;
    case PROP_PARENT:
      g_value_set_object (value, priv->parent);
      break;
    case PROP_WIDTH_REQUEST:
      {
        int w;
        ctk_widget_get_size_request (widget, &w, NULL);
        g_value_set_int (value, w);
      }
      break;
    case PROP_HEIGHT_REQUEST:
      {
        int h;
        ctk_widget_get_size_request (widget, NULL, &h);
        g_value_set_int (value, h);
      }
      break;
    case PROP_VISIBLE:
      g_value_set_boolean (value, _ctk_widget_get_visible (widget));
      break;
    case PROP_SENSITIVE:
      g_value_set_boolean (value, ctk_widget_get_sensitive (widget));
      break;
    case PROP_APP_PAINTABLE:
      g_value_set_boolean (value, ctk_widget_get_app_paintable (widget));
      break;
    case PROP_CAN_FOCUS:
      g_value_set_boolean (value, ctk_widget_get_can_focus (widget));
      break;
    case PROP_HAS_FOCUS:
      g_value_set_boolean (value, ctk_widget_has_focus (widget));
      break;
    case PROP_IS_FOCUS:
      g_value_set_boolean (value, ctk_widget_is_focus (widget));
      break;
    case PROP_FOCUS_ON_CLICK:
      g_value_set_boolean (value, ctk_widget_get_focus_on_click (widget));
      break;
    case PROP_CAN_DEFAULT:
      g_value_set_boolean (value, ctk_widget_get_can_default (widget));
      break;
    case PROP_HAS_DEFAULT:
      g_value_set_boolean (value, ctk_widget_has_default (widget));
      break;
    case PROP_RECEIVES_DEFAULT:
      g_value_set_boolean (value, ctk_widget_get_receives_default (widget));
      break;
    case PROP_COMPOSITE_CHILD:
      g_value_set_boolean (value, widget->priv->composite_child);
      break;
    case PROP_STYLE:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      g_value_set_object (value, ctk_widget_get_style (widget));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case PROP_EVENTS:
      eventp = g_object_get_qdata (G_OBJECT (widget), quark_event_mask);
      g_value_set_flags (value, GPOINTER_TO_INT (eventp));
      break;
    case PROP_NO_SHOW_ALL:
      g_value_set_boolean (value, ctk_widget_get_no_show_all (widget));
      break;
    case PROP_HAS_TOOLTIP:
      g_value_set_boolean (value, ctk_widget_get_has_tooltip (widget));
      break;
    case PROP_TOOLTIP_TEXT:
      {
        gchar *escaped = g_object_get_qdata (object, quark_tooltip_markup);
        gchar *text = NULL;

        if (escaped && !pango_parse_markup (escaped, -1, 0, NULL, &text, NULL, NULL))
          g_assert (NULL == text); /* text should still be NULL in case of markup errors */

        g_value_take_string (value, text);
      }
      break;
    case PROP_TOOLTIP_MARKUP:
      g_value_set_string (value, g_object_get_qdata (object, quark_tooltip_markup));
      break;
    case PROP_WINDOW:
      g_value_set_object (value, _ctk_widget_get_window (widget));
      break;
    case PROP_DOUBLE_BUFFERED:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      g_value_set_boolean (value, ctk_widget_get_double_buffered (widget));
      G_GNUC_END_IGNORE_DEPRECATIONS
      break;
    case PROP_HALIGN:
      g_value_set_enum (value, ctk_widget_get_halign (widget));
      break;
    case PROP_VALIGN:
      g_value_set_enum (value, ctk_widget_get_valign_with_baseline (widget));
      break;
    case PROP_MARGIN_LEFT:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      g_value_set_int (value, ctk_widget_get_margin_left (widget));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case PROP_MARGIN_RIGHT:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      g_value_set_int (value, ctk_widget_get_margin_right (widget));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case PROP_MARGIN_START:
      g_value_set_int (value, ctk_widget_get_margin_start (widget));
      break;
    case PROP_MARGIN_END:
      g_value_set_int (value, ctk_widget_get_margin_end (widget));
      break;
    case PROP_MARGIN_TOP:
      g_value_set_int (value, ctk_widget_get_margin_top (widget));
      break;
    case PROP_MARGIN_BOTTOM:
      g_value_set_int (value, ctk_widget_get_margin_bottom (widget));
      break;
    case PROP_MARGIN:
      g_value_set_int (value, MAX (MAX (priv->margin.left,
                                        priv->margin.right),
                                   MAX (priv->margin.top,
                                        priv->margin.bottom)));
      break;
    case PROP_HEXPAND:
      g_value_set_boolean (value, ctk_widget_get_hexpand (widget));
      break;
    case PROP_HEXPAND_SET:
      g_value_set_boolean (value, ctk_widget_get_hexpand_set (widget));
      break;
    case PROP_VEXPAND:
      g_value_set_boolean (value, ctk_widget_get_vexpand (widget));
      break;
    case PROP_VEXPAND_SET:
      g_value_set_boolean (value, ctk_widget_get_vexpand_set (widget));
      break;
    case PROP_EXPAND:
      g_value_set_boolean (value,
                           ctk_widget_get_hexpand (widget) &&
                           ctk_widget_get_vexpand (widget));
      break;
    case PROP_OPACITY:
      g_value_set_double (value, ctk_widget_get_opacity (widget));
      break;
    case PROP_SCALE_FACTOR:
      g_value_set_int (value, ctk_widget_get_scale_factor (widget));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_ctk_widget_emulate_press (CtkWidget      *widget,
                           const CdkEvent *event)
{
  CtkWidget *event_widget, *next_child, *parent;
  CdkEvent *press;

  event_widget = ctk_get_event_widget ((CdkEvent *) event);

  if (event_widget == widget)
    return;

  if (event->type == CDK_TOUCH_BEGIN ||
      event->type == CDK_TOUCH_UPDATE ||
      event->type == CDK_TOUCH_END)
    {
      press = cdk_event_copy (event);
      press->type = CDK_TOUCH_BEGIN;
    }
  else if (event->type == CDK_BUTTON_PRESS ||
           event->type == CDK_BUTTON_RELEASE)
    {
      press = cdk_event_copy (event);
      press->type = CDK_BUTTON_PRESS;
    }
  else if (event->type == CDK_MOTION_NOTIFY)
    {
      press = cdk_event_new (CDK_BUTTON_PRESS);
      press->button.window = g_object_ref (event->motion.window);
      press->button.time = event->motion.time;
      press->button.x = event->motion.x;
      press->button.y = event->motion.y;
      press->button.x_root = event->motion.x_root;
      press->button.y_root = event->motion.y_root;
      press->button.state = event->motion.state;

      press->button.axes = g_memdup (event->motion.axes,
                                     sizeof (gdouble) *
                                     cdk_device_get_n_axes (event->motion.device));

      if (event->motion.state & CDK_BUTTON3_MASK)
        press->button.button = 3;
      else if (event->motion.state & CDK_BUTTON2_MASK)
        press->button.button = 2;
      else
        {
          if ((event->motion.state & CDK_BUTTON1_MASK) == 0)
            g_critical ("Guessing button number 1 on generated button press event");

          press->button.button = 1;
        }

      cdk_event_set_device (press, cdk_event_get_device (event));
      cdk_event_set_source_device (press, cdk_event_get_source_device (event));
    }
  else
    return;

  press->any.send_event = TRUE;
  next_child = event_widget;
  parent = _ctk_widget_get_parent (next_child);

  while (parent != widget)
    {
      next_child = parent;
      parent = _ctk_widget_get_parent (parent);
    }

  /* Perform propagation state starting from the next child in the chain */
  if (!_ctk_propagate_captured_event (event_widget, press, next_child))
    ctk_propagate_event (event_widget, press);

  cdk_event_free (press);
}

static const CdkEvent *
_ctk_widget_get_last_event (CtkWidget        *widget,
                            CdkEventSequence *sequence)
{
  CtkWidgetPrivate *priv = widget->priv;
  EventControllerData *data;
  const CdkEvent *event;
  GList *l;

  for (l = priv->event_controllers; l; l = l->next)
    {
      data = l->data;

      if (!CTK_IS_GESTURE (data->controller))
        continue;

      event = ctk_gesture_get_last_event (CTK_GESTURE (data->controller),
                                          sequence);
      if (event)
        return event;
    }

  return NULL;
}

static gboolean
_ctk_widget_get_emulating_sequence (CtkWidget         *widget,
                                    CdkEventSequence  *sequence,
                                    CdkEventSequence **sequence_out)
{
  CtkWidgetPrivate *priv = widget->priv;
  GList *l;

  *sequence_out = sequence;

  if (sequence)
    {
      const CdkEvent *last_event;

      last_event = _ctk_widget_get_last_event (widget, sequence);

      if (last_event &&
          (last_event->type == CDK_TOUCH_BEGIN ||
           last_event->type == CDK_TOUCH_UPDATE ||
           last_event->type == CDK_TOUCH_END) &&
          last_event->touch.emulating_pointer)
        return TRUE;
    }
  else
    {
      /* For a NULL(pointer) sequence, find the pointer emulating one */
      for (l = priv->event_controllers; l; l = l->next)
        {
          EventControllerData *data = l->data;

          if (!CTK_IS_GESTURE (data->controller))
            continue;

          if (_ctk_gesture_get_pointer_emulating_sequence (CTK_GESTURE (data->controller),
                                                           sequence_out))
            return TRUE;
        }
    }

  return FALSE;
}

static gboolean
ctk_widget_needs_press_emulation (CtkWidget        *widget,
                                  CdkEventSequence *sequence)
{
  CtkWidgetPrivate *priv = widget->priv;
  gboolean sequence_press_handled = FALSE;
  GList *l;

  /* Check whether there is any remaining gesture in
   * the capture phase that handled the press event
   */
  for (l = priv->event_controllers; l; l = l->next)
    {
      EventControllerData *data;
      CtkPropagationPhase phase;
      CtkGesture *gesture;

      data = l->data;
      phase = ctk_event_controller_get_propagation_phase (data->controller);

      if (phase != CTK_PHASE_CAPTURE)
        continue;
      if (!CTK_IS_GESTURE (data->controller))
        continue;

      gesture = CTK_GESTURE (data->controller);
      sequence_press_handled |=
        (ctk_gesture_handles_sequence (gesture, sequence) &&
         _ctk_gesture_handled_sequence_press (gesture, sequence));
    }

  return !sequence_press_handled;
}

static gint
_ctk_widget_set_sequence_state_internal (CtkWidget             *widget,
                                         CdkEventSequence      *sequence,
                                         CtkEventSequenceState  state,
                                         CtkGesture            *emitter)
{
  gboolean emulates_pointer, sequence_handled = FALSE;
  CtkWidgetPrivate *priv = widget->priv;
  const CdkEvent *mimic_event;
  GList *group = NULL, *l;
  CdkEventSequence *seq;
  gint n_handled = 0;

  if (!priv->event_controllers && state != CTK_EVENT_SEQUENCE_CLAIMED)
    return TRUE;

  if (emitter)
    group = ctk_gesture_get_group (emitter);

  emulates_pointer = _ctk_widget_get_emulating_sequence (widget, sequence, &seq);
  mimic_event = _ctk_widget_get_last_event (widget, seq);

  for (l = priv->event_controllers; l; l = l->next)
    {
      CtkEventSequenceState gesture_state;
      EventControllerData *data;
      CtkGesture *gesture;
      gboolean retval;

      seq = sequence;
      data = l->data;
      gesture_state = state;

      if (!CTK_IS_GESTURE (data->controller))
        continue;

      gesture = CTK_GESTURE (data->controller);

      if (gesture == emitter)
        {
          sequence_handled |=
            _ctk_gesture_handled_sequence_press (gesture, sequence);
          n_handled++;
          continue;
        }

      if (seq && emulates_pointer &&
          !ctk_gesture_handles_sequence (gesture, seq))
        seq = NULL;

      if (group && !g_list_find (group, data->controller))
        {
          /* If a group is provided, ensure only gestures pertaining to the group
           * get a "claimed" state, all other claiming gestures must deny the sequence.
           */
          if (gesture_state == CTK_EVENT_SEQUENCE_CLAIMED &&
              ctk_gesture_get_sequence_state (gesture, sequence) == CTK_EVENT_SEQUENCE_CLAIMED)
            gesture_state = CTK_EVENT_SEQUENCE_DENIED;
          else
            continue;
        }
      else if (!group &&
               ctk_gesture_get_sequence_state (gesture, sequence) != CTK_EVENT_SEQUENCE_CLAIMED)
        continue;

      g_signal_handler_block (data->controller, data->sequence_state_changed_id);
      retval = ctk_gesture_set_sequence_state (gesture, seq, gesture_state);
      g_signal_handler_unblock (data->controller, data->sequence_state_changed_id);

      if (retval || gesture == emitter)
        {
          sequence_handled |=
            _ctk_gesture_handled_sequence_press (gesture, seq);
          n_handled++;
        }
    }

  /* If the sequence goes denied, check whether this is a controller attached
   * to the capture phase, that additionally handled the button/touch press (i.e.
   * it was consumed), the corresponding press will be emulated for widgets
   * beneath, so the widgets beneath get a coherent stream of events from now on.
   */
  if (n_handled > 0 && sequence_handled &&
      state == CTK_EVENT_SEQUENCE_DENIED &&
      ctk_widget_needs_press_emulation (widget, sequence))
    _ctk_widget_emulate_press (widget, mimic_event);

  g_list_free (group);

  return n_handled;
}

static gboolean
_ctk_widget_cancel_sequence (CtkWidget        *widget,
                             CdkEventSequence *sequence)
{
  CtkWidgetPrivate *priv = widget->priv;
  gboolean emulates_pointer;
  gboolean handled = FALSE;
  CdkEventSequence *seq;
  GList *l;

  emulates_pointer = _ctk_widget_get_emulating_sequence (widget, sequence, &seq);

  for (l = priv->event_controllers; l; l = l->next)
    {
      EventControllerData *data;
      CtkGesture *gesture;

      seq = sequence;
      data = l->data;

      if (!CTK_IS_GESTURE (data->controller))
        continue;

      gesture = CTK_GESTURE (data->controller);

      if (seq && emulates_pointer &&
          !ctk_gesture_handles_sequence (gesture, seq))
        seq = NULL;

      if (!ctk_gesture_handles_sequence (gesture, seq))
        continue;

      handled |= _ctk_gesture_cancel_sequence (gesture, seq);
    }

  return handled;
}

static void
ctk_widget_init (GTypeInstance *instance, gpointer g_class)
{
  CtkWidget *widget = CTK_WIDGET (instance);
  CtkWidgetPrivate *priv;

  widget->priv = ctk_widget_get_instance_private (widget); 
  priv = widget->priv;

  priv->child_visible = TRUE;
  priv->name = NULL;
  priv->allocation.x = -1;
  priv->allocation.y = -1;
  priv->allocation.width = 1;
  priv->allocation.height = 1;
  priv->user_alpha = 255;
  priv->alpha = 255;
  priv->window = NULL;
  priv->parent = NULL;

  priv->sensitive = TRUE;
  priv->composite_child = composite_child_stack != 0;
  priv->double_buffered = TRUE;
  priv->redraw_on_alloc = TRUE;
  priv->alloc_needed = TRUE;
  priv->alloc_needed_on_child = TRUE;
  priv->focus_on_click = TRUE;
#ifdef G_ENABLE_DEBUG
  priv->highlight_resize = FALSE;
#endif

  switch (_ctk_widget_get_direction (widget))
    {
    case CTK_TEXT_DIR_LTR:
      priv->state_flags = CTK_STATE_FLAG_DIR_LTR;
      break;

    case CTK_TEXT_DIR_RTL:
      priv->state_flags = CTK_STATE_FLAG_DIR_RTL;
      break;

    case CTK_TEXT_DIR_NONE:
    default:
      g_assert_not_reached ();
      break;
    }

  /* this will be set to TRUE if the widget gets a child or if the
   * expand flag is set on the widget, but until one of those happen
   * we know the expand is already properly FALSE.
   *
   * We really want to default FALSE here to avoid computing expand
   * all over the place while initially building a widget tree.
   */
  priv->need_compute_expand = FALSE;

  priv->halign = CTK_ALIGN_FILL;
  priv->valign = CTK_ALIGN_FILL;

  priv->width = -1;
  priv->height = -1;

  _ctk_size_request_cache_init (&priv->requests);

  priv->cssnode = ctk_css_widget_node_new (widget);
  ctk_css_node_set_state (priv->cssnode, priv->state_flags);
  /* need to set correct type here, and only class has the correct type here */
  ctk_css_node_set_widget_type (priv->cssnode, G_TYPE_FROM_CLASS (g_class));
  ctk_css_node_set_name (priv->cssnode, CTK_WIDGET_CLASS (g_class)->priv->css_name);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  priv->style = ctk_widget_get_default_style ();
  G_GNUC_END_IGNORE_DEPRECATIONS;
  g_object_ref (priv->style);
}


static void
ctk_widget_dispatch_child_properties_changed (CtkWidget   *widget,
					      guint        n_pspecs,
					      GParamSpec **pspecs)
{
  CtkWidgetPrivate *priv = widget->priv;
  CtkWidget *container = priv->parent;
  guint i;

  for (i = 0; widget->priv->parent == container && i < n_pspecs; i++)
    g_signal_emit (widget, widget_signals[CHILD_NOTIFY], g_param_spec_get_name_quark (pspecs[i]), pspecs[i]);
}

/**
 * ctk_widget_freeze_child_notify:
 * @widget: a #CtkWidget
 *
 * Stops emission of #CtkWidget::child-notify signals on @widget. The
 * signals are queued until ctk_widget_thaw_child_notify() is called
 * on @widget.
 *
 * This is the analogue of g_object_freeze_notify() for child properties.
 **/
void
ctk_widget_freeze_child_notify (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (!G_OBJECT (widget)->ref_count)
    return;

  g_object_ref (widget);
  g_object_notify_queue_freeze (G_OBJECT (widget), _ctk_widget_child_property_notify_context);
  g_object_unref (widget);
}

/**
 * ctk_widget_child_notify:
 * @widget: a #CtkWidget
 * @child_property: the name of a child property installed on the
 *                  class of @widget’s parent
 *
 * Emits a #CtkWidget::child-notify signal for the
 * [child property][child-properties] @child_property
 * on @widget.
 *
 * This is the analogue of g_object_notify() for child properties.
 *
 * Also see ctk_container_child_notify().
 */
void
ctk_widget_child_notify (CtkWidget    *widget,
                         const gchar  *child_property)
{
  if (widget->priv->parent == NULL)
    return;

  ctk_container_child_notify (CTK_CONTAINER (widget->priv->parent), widget, child_property);
}

/**
 * ctk_widget_thaw_child_notify:
 * @widget: a #CtkWidget
 *
 * Reverts the effect of a previous call to ctk_widget_freeze_child_notify().
 * This causes all queued #CtkWidget::child-notify signals on @widget to be
 * emitted.
 */
void
ctk_widget_thaw_child_notify (CtkWidget *widget)
{
  GObjectNotifyQueue *nqueue;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (!G_OBJECT (widget)->ref_count)
    return;

  g_object_ref (widget);
  nqueue = g_object_notify_queue_from_object (G_OBJECT (widget), _ctk_widget_child_property_notify_context);
  if (!nqueue || !nqueue->freeze_count)
    g_warning (G_STRLOC ": child-property-changed notification for %s(%p) is not frozen",
	       G_OBJECT_TYPE_NAME (widget), widget);
  else
    g_object_notify_queue_thaw (G_OBJECT (widget), nqueue);
  g_object_unref (widget);
}


/**
 * ctk_widget_new:
 * @type: type ID of the widget to create
 * @first_property_name: name of first property to set
 * @...: value of first property, followed by more properties,
 *     %NULL-terminated
 *
 * This is a convenience function for creating a widget and setting
 * its properties in one go. For example you might write:
 * `ctk_widget_new (CTK_TYPE_LABEL, "label", "Hello World", "xalign",
 * 0.0, NULL)` to create a left-aligned label. Equivalent to
 * g_object_new(), but returns a widget so you don’t have to
 * cast the object yourself.
 *
 * Returns: a new #CtkWidget of type @widget_type
 **/
CtkWidget*
ctk_widget_new (GType        type,
		const gchar *first_property_name,
		...)
{
  CtkWidget *widget;
  va_list var_args;

  g_return_val_if_fail (g_type_is_a (type, CTK_TYPE_WIDGET), NULL);

  va_start (var_args, first_property_name);
  widget = (CtkWidget *)g_object_new_valist (type, first_property_name, var_args);
  va_end (var_args);

  return widget;
}

static inline void
ctk_widget_queue_draw_child (CtkWidget *widget)
{
  CtkWidgetPrivate *priv = widget->priv;
  CtkWidget *parent;

  parent = priv->parent;
  if (parent && _ctk_widget_is_drawable (parent))
    ctk_widget_queue_draw_area (parent,
				priv->clip.x,
				priv->clip.y,
				priv->clip.width,
				priv->clip.height);
}

/**
 * ctk_widget_unparent:
 * @widget: a #CtkWidget
 *
 * This function is only for use in widget implementations.
 * Should be called by implementations of the remove method
 * on #CtkContainer, to dissociate a child from the container.
 **/
void
ctk_widget_unparent (CtkWidget *widget)
{
  CtkWidgetPrivate *priv;
  GObjectNotifyQueue *nqueue;
  CtkWidget *toplevel;
  CtkWidget *old_parent;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = widget->priv;

  if (priv->parent == NULL)
    return;

  /* keep this function in sync with ctk_menu_detach() */

  ctk_widget_push_verify_invariants (widget);

  g_object_freeze_notify (G_OBJECT (widget));
  nqueue = g_object_notify_queue_freeze (G_OBJECT (widget), _ctk_widget_child_property_notify_context);

  toplevel = _ctk_widget_get_toplevel (widget);
  if (_ctk_widget_is_toplevel (toplevel))
    _ctk_window_unset_focus_and_default (CTK_WINDOW (toplevel), widget);

  if (ctk_container_get_focus_child (CTK_CONTAINER (priv->parent)) == widget)
    ctk_container_set_focus_child (CTK_CONTAINER (priv->parent), NULL);

  ctk_widget_queue_draw_child (widget);

  /* Reset the width and height here, to force reallocation if we
   * get added back to a new parent. This won't work if our new
   * allocation is smaller than 1x1 and we actually want a size of 1x1...
   * (would 0x0 be OK here?)
   */
  priv->allocation.width = 1;
  priv->allocation.height = 1;

  if (_ctk_widget_get_realized (widget))
    {
      if (priv->in_reparent)
	ctk_widget_unmap (widget);
      else
	ctk_widget_unrealize (widget);
    }

  /* If we are unanchoring the child, we save around the toplevel
   * to emit hierarchy changed
   */
  if (priv->parent->priv->anchored)
    g_object_ref (toplevel);
  else
    toplevel = NULL;

  /* Removing a widget from a container restores the child visible
   * flag to the default state, so it doesn't affect the child
   * in the next parent.
   */
  priv->child_visible = TRUE;

  old_parent = priv->parent;
  priv->parent = NULL;

  /* parent may no longer expand if the removed
   * child was expand=TRUE and could therefore
   * be forcing it to.
   */
  if (_ctk_widget_get_visible (widget) &&
      (priv->need_compute_expand ||
       priv->computed_hexpand ||
       priv->computed_vexpand))
    {
      ctk_widget_queue_compute_expand (old_parent);
    }

  /* Unset BACKDROP since we are no longer inside a toplevel window */
  ctk_widget_unset_state_flags (widget, CTK_STATE_FLAG_BACKDROP);
  if (priv->context)
    ctk_style_context_set_parent (priv->context, NULL);
  ctk_css_node_set_parent (widget->priv->cssnode, NULL);

  _ctk_widget_update_parent_muxer (widget);

  g_signal_emit (widget, widget_signals[PARENT_SET], 0, old_parent);
  if (toplevel)
    {
      _ctk_widget_propagate_hierarchy_changed (widget, toplevel);
      g_object_unref (toplevel);
    }

  /* Now that the parent pointer is nullified and the hierarchy-changed
   * already passed, go ahead and unset the parent window, if we are unparenting
   * an embedded CtkWindow the window will become toplevel again and hierarchy-changed
   * will fire again for the new subhierarchy.
   */
  ctk_widget_set_parent_window (widget, NULL);

  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_PARENT]);
  g_object_thaw_notify (G_OBJECT (widget));
  if (!priv->parent)
    g_object_notify_queue_clear (G_OBJECT (widget), nqueue);
  g_object_notify_queue_thaw (G_OBJECT (widget), nqueue);

  ctk_widget_pop_verify_invariants (widget);
  g_object_unref (widget);
}

/**
 * ctk_widget_destroy:
 * @widget: a #CtkWidget
 *
 * Destroys a widget.
 *
 * When a widget is destroyed all references it holds on other objects
 * will be released:
 *
 *  - if the widget is inside a container, it will be removed from its
 *  parent
 *  - if the widget is a container, all its children will be destroyed,
 *  recursively
 *  - if the widget is a top level, it will be removed from the list
 *  of top level widgets that CTK+ maintains internally
 *
 * It's expected that all references held on the widget will also
 * be released; you should connect to the #CtkWidget::destroy signal
 * if you hold a reference to @widget and you wish to remove it when
 * this function is called. It is not necessary to do so if you are
 * implementing a #CtkContainer, as you'll be able to use the
 * #CtkContainerClass.remove() virtual function for that.
 *
 * It's important to notice that ctk_widget_destroy() will only cause
 * the @widget to be finalized if no additional references, acquired
 * using g_object_ref(), are held on it. In case additional references
 * are in place, the @widget will be in an "inert" state after calling
 * this function; @widget will still point to valid memory, allowing you
 * to release the references you hold, but you may not query the widget's
 * own state.
 *
 * You should typically call this function on top level widgets, and
 * rarely on child widgets.
 *
 * See also: ctk_container_remove()
 */
void
ctk_widget_destroy (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (!widget->priv->in_destruction)
    g_object_run_dispose (G_OBJECT (widget));
}

/**
 * ctk_widget_destroyed:
 * @widget: a #CtkWidget
 * @widget_pointer: (inout) (transfer none): address of a variable that contains @widget
 *
 * This function sets *@widget_pointer to %NULL if @widget_pointer !=
 * %NULL.  It’s intended to be used as a callback connected to the
 * “destroy” signal of a widget. You connect ctk_widget_destroyed()
 * as a signal handler, and pass the address of your widget variable
 * as user data. Then when the widget is destroyed, the variable will
 * be set to %NULL. Useful for example to avoid multiple copies
 * of the same dialog.
 **/
void
ctk_widget_destroyed (CtkWidget      *widget,
		      CtkWidget      **widget_pointer)
{
  /* Don't make any assumptions about the
   *  value of widget!
   *  Even check widget_pointer.
   */
  if (widget_pointer)
    *widget_pointer = NULL;
}

/**
 * ctk_widget_show:
 * @widget: a #CtkWidget
 *
 * Flags a widget to be displayed. Any widget that isn’t shown will
 * not appear on the screen. If you want to show all the widgets in a
 * container, it’s easier to call ctk_widget_show_all() on the
 * container, instead of individually showing the widgets.
 *
 * Remember that you have to show the containers containing a widget,
 * in addition to the widget itself, before it will appear onscreen.
 *
 * When a toplevel container is shown, it is immediately realized and
 * mapped; other shown widgets are realized and mapped when their
 * toplevel container is realized and mapped.
 **/
void
ctk_widget_show (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (!_ctk_widget_get_visible (widget))
    {
      CtkWidget *parent;

      g_object_ref (widget);
      ctk_widget_push_verify_invariants (widget);

      parent = _ctk_widget_get_parent (widget);
      if (parent)
        {
          ctk_widget_queue_resize (parent);

          /* see comment in set_parent() for why this should and can be
           * conditional
           */
          if (widget->priv->need_compute_expand ||
              widget->priv->computed_hexpand ||
              widget->priv->computed_vexpand)
            ctk_widget_queue_compute_expand (parent);
        }

      ctk_css_node_set_visible (widget->priv->cssnode, TRUE);

      g_signal_emit (widget, widget_signals[SHOW], 0);
      g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_VISIBLE]);

      ctk_widget_pop_verify_invariants (widget);
      g_object_unref (widget);
    }
}

static void
ctk_widget_real_show (CtkWidget *widget)
{
  CtkWidgetPrivate *priv = widget->priv;

  if (!_ctk_widget_get_visible (widget))
    {
      priv->visible = TRUE;

      if (priv->parent &&
	  _ctk_widget_get_mapped (priv->parent) &&
          _ctk_widget_get_child_visible (widget) &&
	  !_ctk_widget_get_mapped (widget))
	ctk_widget_map (widget);
    }
}

static void
ctk_widget_show_map_callback (CtkWidget *widget, CdkEvent *event, gint *flag)
{
  *flag = TRUE;
  g_signal_handlers_disconnect_by_func (widget,
					ctk_widget_show_map_callback,
					flag);
}

/**
 * ctk_widget_show_now:
 * @widget: a #CtkWidget
 *
 * Shows a widget. If the widget is an unmapped toplevel widget
 * (i.e. a #CtkWindow that has not yet been shown), enter the main
 * loop and wait for the window to actually be mapped. Be careful;
 * because the main loop is running, anything can happen during
 * this function.
 **/
void
ctk_widget_show_now (CtkWidget *widget)
{
  gint flag = FALSE;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  /* make sure we will get event */
  if (!_ctk_widget_get_mapped (widget) &&
      _ctk_widget_is_toplevel (widget))
    {
      ctk_widget_show (widget);

      g_signal_connect (widget, "map-event",
			G_CALLBACK (ctk_widget_show_map_callback),
			&flag);

      while (!flag)
	ctk_main_iteration ();
    }
  else
    ctk_widget_show (widget);
}

/**
 * ctk_widget_hide:
 * @widget: a #CtkWidget
 *
 * Reverses the effects of ctk_widget_show(), causing the widget to be
 * hidden (invisible to the user).
 **/
void
ctk_widget_hide (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (_ctk_widget_get_visible (widget))
    {
      CtkWidget *toplevel = _ctk_widget_get_toplevel (widget);
      CtkWidget *parent;

      g_object_ref (widget);
      ctk_widget_push_verify_invariants (widget);

      if (toplevel != widget && _ctk_widget_is_toplevel (toplevel))
        _ctk_window_unset_focus_and_default (CTK_WINDOW (toplevel), widget);

      /* a parent may now be expand=FALSE since we're hidden. */
      if (widget->priv->need_compute_expand ||
          widget->priv->computed_hexpand ||
          widget->priv->computed_vexpand)
        {
          ctk_widget_queue_compute_expand (widget);
        }

      ctk_css_node_set_visible (widget->priv->cssnode, FALSE);

      g_signal_emit (widget, widget_signals[HIDE], 0);
      g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_VISIBLE]);

      parent = ctk_widget_get_parent (widget);
      if (parent)
	ctk_widget_queue_resize (parent);

      ctk_widget_queue_allocate (widget);

      ctk_widget_pop_verify_invariants (widget);
      g_object_unref (widget);
    }
}

static void
ctk_widget_real_hide (CtkWidget *widget)
{
  if (_ctk_widget_get_visible (widget))
    {
      widget->priv->visible = FALSE;

      if (_ctk_widget_get_mapped (widget))
	ctk_widget_unmap (widget);
    }
}

/**
 * ctk_widget_hide_on_delete:
 * @widget: a #CtkWidget
 *
 * Utility function; intended to be connected to the #CtkWidget::delete-event
 * signal on a #CtkWindow. The function calls ctk_widget_hide() on its
 * argument, then returns %TRUE. If connected to ::delete-event, the
 * result is that clicking the close button for a window (on the
 * window frame, top right corner usually) will hide but not destroy
 * the window. By default, CTK+ destroys windows when ::delete-event
 * is received.
 *
 * Returns: %TRUE
 **/
gboolean
ctk_widget_hide_on_delete (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  ctk_widget_hide (widget);

  return TRUE;
}

/**
 * ctk_widget_show_all:
 * @widget: a #CtkWidget
 *
 * Recursively shows a widget, and any child widgets (if the widget is
 * a container).
 **/
void
ctk_widget_show_all (CtkWidget *widget)
{
  CtkWidgetClass *class;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (ctk_widget_get_no_show_all (widget))
    return;

  class = CTK_WIDGET_GET_CLASS (widget);

  if (class->show_all)
    class->show_all (widget);
}

/**
 * ctk_widget_map:
 * @widget: a #CtkWidget
 *
 * This function is only for use in widget implementations. Causes
 * a widget to be mapped if it isn’t already.
 **/
void
ctk_widget_map (CtkWidget *widget)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (_ctk_widget_get_visible (widget));
  g_return_if_fail (_ctk_widget_get_child_visible (widget));

  priv = widget->priv;

  if (!_ctk_widget_get_mapped (widget))
    {
      ctk_widget_push_verify_invariants (widget);

      if (!_ctk_widget_get_realized (widget))
        ctk_widget_realize (widget);

      g_signal_emit (widget, widget_signals[MAP], 0);

      if (!_ctk_widget_get_has_window (widget))
        cdk_window_invalidate_rect (priv->window, &priv->clip, FALSE);

      ctk_widget_pop_verify_invariants (widget);
    }
}

/**
 * ctk_widget_unmap:
 * @widget: a #CtkWidget
 *
 * This function is only for use in widget implementations. Causes
 * a widget to be unmapped if it’s currently mapped.
 **/
void
ctk_widget_unmap (CtkWidget *widget)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = widget->priv;

  if (_ctk_widget_get_mapped (widget))
    {
      g_object_ref (widget);
      ctk_widget_push_verify_invariants (widget);

      if (!_ctk_widget_get_has_window (widget))
	cdk_window_invalidate_rect (priv->window, &priv->clip, FALSE);
      _ctk_tooltip_hide (widget);

      g_signal_emit (widget, widget_signals[UNMAP], 0);

      ctk_widget_pop_verify_invariants (widget);
      g_object_unref (widget);
    }
}

static void
_ctk_widget_enable_device_events (CtkWidget *widget)
{
  GHashTable *device_events;
  GHashTableIter iter;
  gpointer key, value;

  device_events = g_object_get_qdata (G_OBJECT (widget), quark_device_event_mask);

  if (!device_events)
    return;

  g_hash_table_iter_init (&iter, device_events);

  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      CdkDevice *device;
      CdkEventMask event_mask;

      device = key;
      event_mask = GPOINTER_TO_UINT (value);
      ctk_widget_add_events_internal (widget, device, event_mask);
    }
}

typedef struct {
  CtkWidget *widget;
  CdkDevice *device;
  gboolean enabled;
} DeviceEnableData;

static void
device_enable_foreach (CtkWidget *widget,
                       gpointer   user_data)
{
  DeviceEnableData *data = user_data;
  ctk_widget_set_device_enabled_internal (widget, data->device, TRUE, data->enabled);
}

static void
device_enable_foreach_window (gpointer win,
                              gpointer user_data)
{
  CdkWindow *window = win;
  DeviceEnableData *data = user_data;
  CdkEventMask events;
  CtkWidget *window_widget;
  GList *window_list;

  cdk_window_get_user_data (window, (gpointer *) &window_widget);
  if (data->widget != window_widget)
    return;

  if (data->enabled)
    events = cdk_window_get_events (window);
  else
    events = 0;

  cdk_window_set_device_events (window, data->device, events);

  window_list = cdk_window_peek_children (window);
  g_list_foreach (window_list, device_enable_foreach_window, data);
}

void
ctk_widget_set_device_enabled_internal (CtkWidget *widget,
                                        CdkDevice *device,
                                        gboolean   recurse,
                                        gboolean   enabled)
{
  DeviceEnableData data;

  data.widget = widget;
  data.device = device;
  data.enabled = enabled;

  if (_ctk_widget_get_has_window (widget))
    {
      CdkWindow *window;

      window = _ctk_widget_get_window (widget);
      device_enable_foreach_window (window, &data);
    }
  else
    {
      GList *window_list;

      window_list = cdk_window_peek_children (_ctk_widget_get_window (widget));
      g_list_foreach (window_list, device_enable_foreach_window, &data);
    }

  if (recurse && CTK_IS_CONTAINER (widget))
    ctk_container_forall (CTK_CONTAINER (widget), device_enable_foreach, &data);
}

static void
ctk_widget_update_devices_mask (CtkWidget *widget,
                                gboolean   recurse)
{
  GList *enabled_devices, *l;

  enabled_devices = g_object_get_qdata (G_OBJECT (widget), quark_enabled_devices);

  for (l = enabled_devices; l; l = l->next)
    ctk_widget_set_device_enabled_internal (widget, CDK_DEVICE (l->data), recurse, TRUE);
}

typedef struct _CtkTickCallbackInfo CtkTickCallbackInfo;

struct _CtkTickCallbackInfo
{
  guint refcount;

  guint id;
  CtkTickCallback callback;
  gpointer user_data;
  GDestroyNotify notify;

  guint destroyed : 1;
};

static void
ref_tick_callback_info (CtkTickCallbackInfo *info)
{
  info->refcount++;
}

static void
unref_tick_callback_info (CtkWidget           *widget,
                          CtkTickCallbackInfo *info,
                          GList               *link)
{
  CtkWidgetPrivate *priv = widget->priv;

  info->refcount--;
  if (info->refcount == 0)
    {
      priv->tick_callbacks = g_list_delete_link (priv->tick_callbacks, link);
      if (info->notify)
        info->notify (info->user_data);
      g_slice_free (CtkTickCallbackInfo, info);
    }

  if (priv->tick_callbacks == NULL && priv->clock_tick_id)
    {
      CdkFrameClock *frame_clock = ctk_widget_get_frame_clock (widget);
      g_signal_handler_disconnect (frame_clock, priv->clock_tick_id);
      priv->clock_tick_id = 0;
      cdk_frame_clock_end_updating (frame_clock);
    }
}

static void
destroy_tick_callback_info (CtkWidget           *widget,
                            CtkTickCallbackInfo *info,
                            GList               *link)
{
  if (!info->destroyed)
    {
      info->destroyed = TRUE;
      unref_tick_callback_info (widget, info, link);
    }
}

static void
destroy_tick_callbacks (CtkWidget *widget)
{
  CtkWidgetPrivate *priv = widget->priv;
  GList *l;

  for (l = priv->tick_callbacks; l;)
    {
      GList *next = l->next;
      destroy_tick_callback_info (widget, l->data, l);
      l = next;
    }
}

static void
ctk_widget_on_frame_clock_update (CdkFrameClock *frame_clock,
                                  CtkWidget     *widget)
{
  CtkWidgetPrivate *priv = widget->priv;
  GList *l;

  g_object_ref (widget);

  for (l = priv->tick_callbacks; l;)
    {
      CtkTickCallbackInfo *info = l->data;
      GList *next;

      ref_tick_callback_info (info);
      if (!info->destroyed)
        {
          if (info->callback (widget,
                              frame_clock,
                              info->user_data) == G_SOURCE_REMOVE)
            {
              destroy_tick_callback_info (widget, info, l);
            }
        }

      next = l->next;
      unref_tick_callback_info (widget, info, l);
      l = next;
    }

  g_object_unref (widget);
}

static guint tick_callback_id;

/**
 * ctk_widget_add_tick_callback:
 * @widget: a #CtkWidget
 * @callback: function to call for updating animations
 * @user_data: data to pass to @callback
 * @notify: function to call to free @user_data when the callback is removed.
 *
 * Queues an animation frame update and adds a callback to be called
 * before each frame. Until the tick callback is removed, it will be
 * called frequently (usually at the frame rate of the output device
 * or as quickly as the application can be repainted, whichever is
 * slower). For this reason, is most suitable for handling graphics
 * that change every frame or every few frames. The tick callback does
 * not automatically imply a relayout or repaint. If you want a
 * repaint or relayout, and aren’t changing widget properties that
 * would trigger that (for example, changing the text of a #CtkLabel),
 * then you will have to call ctk_widget_queue_resize() or
 * ctk_widget_queue_draw_area() yourself.
 *
 * cdk_frame_clock_get_frame_time() should generally be used for timing
 * continuous animations and
 * cdk_frame_timings_get_predicted_presentation_time() if you are
 * trying to display isolated frames at particular times.
 *
 * This is a more convenient alternative to connecting directly to the
 * #CdkFrameClock::update signal of #CdkFrameClock, since you don't
 * have to worry about when a #CdkFrameClock is assigned to a widget.
 *
 * Returns: an id for the connection of this callback. Remove the callback
 *     by passing it to ctk_widget_remove_tick_callback()
 *
 * Since: 3.8
 */
guint
ctk_widget_add_tick_callback (CtkWidget       *widget,
                              CtkTickCallback  callback,
                              gpointer         user_data,
                              GDestroyNotify   notify)
{
  CtkWidgetPrivate *priv;
  CtkTickCallbackInfo *info;
  CdkFrameClock *frame_clock;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);

  priv = widget->priv;

  if (priv->frameclock_connected && !priv->clock_tick_id)
    {
      frame_clock = ctk_widget_get_frame_clock (widget);

      if (frame_clock)
        {
          priv->clock_tick_id = g_signal_connect (frame_clock, "update",
                                                  G_CALLBACK (ctk_widget_on_frame_clock_update),
                                                  widget);
          cdk_frame_clock_begin_updating (frame_clock);
        }
    }

  info = g_slice_new0 (CtkTickCallbackInfo);

  info->refcount = 1;
  info->id = ++tick_callback_id;
  info->callback = callback;
  info->user_data = user_data;
  info->notify = notify;

  priv->tick_callbacks = g_list_prepend (priv->tick_callbacks,
                                         info);

  return info->id;
}

/**
 * ctk_widget_remove_tick_callback:
 * @widget: a #CtkWidget
 * @id: an id returned by ctk_widget_add_tick_callback()
 *
 * Removes a tick callback previously registered with
 * ctk_widget_add_tick_callback().
 *
 * Since: 3.8
 */
void
ctk_widget_remove_tick_callback (CtkWidget *widget,
                                 guint      id)
{
  CtkWidgetPrivate *priv;
  GList *l;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = widget->priv;

  for (l = priv->tick_callbacks; l; l = l->next)
    {
      CtkTickCallbackInfo *info = l->data;
      if (info->id == id)
        {
          destroy_tick_callback_info (widget, info, l);
          return;
        }
    }
}

gboolean
ctk_widget_has_tick_callback (CtkWidget *widget)
{
  return widget->priv->tick_callbacks != NULL;
}

static void
ctk_widget_connect_frame_clock (CtkWidget     *widget,
                                CdkFrameClock *frame_clock)
{
  CtkWidgetPrivate *priv = widget->priv;

  priv->frameclock_connected = TRUE;

  if (CTK_IS_CONTAINER (widget))
    _ctk_container_maybe_start_idle_sizer (CTK_CONTAINER (widget));

  if (priv->tick_callbacks != NULL && !priv->clock_tick_id)
    {
      priv->clock_tick_id = g_signal_connect (frame_clock, "update",
                                              G_CALLBACK (ctk_widget_on_frame_clock_update),
                                              widget);
      cdk_frame_clock_begin_updating (frame_clock);
    }

  ctk_css_node_invalidate_frame_clock (priv->cssnode, FALSE);

  if (priv->context)
    ctk_style_context_set_frame_clock (priv->context, frame_clock);
}

static void
ctk_widget_disconnect_frame_clock (CtkWidget     *widget,
                                   CdkFrameClock *frame_clock)
{
  CtkWidgetPrivate *priv = widget->priv;

  if (CTK_IS_CONTAINER (widget))
    _ctk_container_stop_idle_sizer (CTK_CONTAINER (widget));

  ctk_css_node_invalidate_frame_clock (priv->cssnode, FALSE);

  if (priv->clock_tick_id)
    {
      g_signal_handler_disconnect (frame_clock, priv->clock_tick_id);
      priv->clock_tick_id = 0;
      cdk_frame_clock_end_updating (frame_clock);
    }

  priv->frameclock_connected = FALSE;

  if (priv->context)
    ctk_style_context_set_frame_clock (priv->context, NULL);
}

/**
 * ctk_widget_realize:
 * @widget: a #CtkWidget
 *
 * Creates the CDK (windowing system) resources associated with a
 * widget.  For example, @widget->window will be created when a widget
 * is realized.  Normally realization happens implicitly; if you show
 * a widget and all its parent containers, then the widget will be
 * realized and mapped automatically.
 *
 * Realizing a widget requires all
 * the widget’s parent widgets to be realized; calling
 * ctk_widget_realize() realizes the widget’s parents in addition to
 * @widget itself. If a widget is not yet inside a toplevel window
 * when you realize it, bad things will happen.
 *
 * This function is primarily used in widget implementations, and
 * isn’t very useful otherwise. Many times when you think you might
 * need it, a better approach is to connect to a signal that will be
 * called after the widget is realized automatically, such as
 * #CtkWidget::draw. Or simply g_signal_connect () to the
 * #CtkWidget::realize signal.
 **/
void
ctk_widget_realize (CtkWidget *widget)
{
  CtkWidgetPrivate *priv;
  cairo_region_t *region;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (widget->priv->anchored ||
		    CTK_IS_INVISIBLE (widget));

  priv = widget->priv;

  if (!_ctk_widget_get_realized (widget))
    {
      ctk_widget_push_verify_invariants (widget);

      /*
	if (CTK_IS_CONTAINER (widget) && _ctk_widget_get_has_window (widget))
	  g_message ("ctk_widget_realize(%s)", G_OBJECT_TYPE_NAME (widget));
      */

      if (priv->parent == NULL &&
          !_ctk_widget_is_toplevel (widget))
        g_warning ("Calling ctk_widget_realize() on a widget that isn't "
                   "inside a toplevel window is not going to work very well. "
                   "Widgets must be inside a toplevel container before realizing them.");

      if (priv->parent && !_ctk_widget_get_realized (priv->parent))
	ctk_widget_realize (priv->parent);

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_widget_ensure_style (widget);
      G_GNUC_END_IGNORE_DEPRECATIONS

      g_signal_emit (widget, widget_signals[REALIZE], 0);

      ctk_widget_real_set_has_tooltip (widget, ctk_widget_get_has_tooltip (widget), TRUE);

      if (priv->has_shape_mask)
	{
	  region = g_object_get_qdata (G_OBJECT (widget), quark_shape_info);
	  cdk_window_shape_combine_region (priv->window, region, 0, 0);
	}

      ctk_widget_update_input_shape (widget);

      if (priv->multidevice)
        cdk_window_set_support_multidevice (priv->window, TRUE);

      _ctk_widget_enable_device_events (widget);
      ctk_widget_update_devices_mask (widget, TRUE);

      ctk_widget_update_alpha (widget);

      if (priv->context)
	ctk_style_context_set_scale (priv->context, ctk_widget_get_scale_factor (widget));
      ctk_widget_connect_frame_clock (widget,
                                      ctk_widget_get_frame_clock (widget));

      ctk_widget_pop_verify_invariants (widget);
    }
}

/**
 * ctk_widget_unrealize:
 * @widget: a #CtkWidget
 *
 * This function is only useful in widget implementations.
 * Causes a widget to be unrealized (frees all CDK resources
 * associated with the widget, such as @widget->window).
 **/
void
ctk_widget_unrealize (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  g_object_ref (widget);
  ctk_widget_push_verify_invariants (widget);

  if (widget->priv->has_shape_mask)
    ctk_widget_shape_combine_region (widget, NULL);

  if (g_object_get_qdata (G_OBJECT (widget), quark_input_shape_info))
    ctk_widget_input_shape_combine_region (widget, NULL);

  if (_ctk_widget_get_realized (widget))
    {
      if (widget->priv->mapped)
        ctk_widget_unmap (widget);

      ctk_widget_disconnect_frame_clock (widget,
                                         ctk_widget_get_frame_clock (widget));

      g_signal_emit (widget, widget_signals[UNREALIZE], 0);
      g_assert (!widget->priv->mapped);
      ctk_widget_set_realized (widget, FALSE);
    }

  ctk_widget_pop_verify_invariants (widget);
  g_object_unref (widget);
}

/*****************************************
 * Draw queueing.
 *****************************************/

static void
ctk_widget_real_queue_draw_region (CtkWidget         *widget,
				   const cairo_region_t *region)
{
  CtkWidgetPrivate *priv = widget->priv;

  cdk_window_invalidate_region (priv->window, region, TRUE);
}

/**
 * ctk_widget_queue_draw_region:
 * @widget: a #CtkWidget
 * @region: region to draw
 *
 * Invalidates the area of @widget defined by @region by calling
 * cdk_window_invalidate_region() on the widget’s window and all its
 * child windows. Once the main loop becomes idle (after the current
 * batch of events has been processed, roughly), the window will
 * receive expose events for the union of all regions that have been
 * invalidated.
 *
 * Normally you would only use this function in widget
 * implementations. You might also use it to schedule a redraw of a
 * #CtkDrawingArea or some portion thereof.
 *
 * Since: 3.0
 **/
void
ctk_widget_queue_draw_region (CtkWidget            *widget,
                              const cairo_region_t *region)
{
  CtkWidget *w;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (!_ctk_widget_get_realized (widget))
    return;

  /* Just return if the widget or one of its ancestors isn't mapped */
  for (w = widget; w != NULL; w = w->priv->parent)
    if (!_ctk_widget_get_mapped (w))
      return;

  WIDGET_CLASS (widget)->queue_draw_region (widget, region);
}

/**
 * ctk_widget_queue_draw_area:
 * @widget: a #CtkWidget
 * @x: x coordinate of upper-left corner of rectangle to redraw
 * @y: y coordinate of upper-left corner of rectangle to redraw
 * @width: width of region to draw
 * @height: height of region to draw
 *
 * Convenience function that calls ctk_widget_queue_draw_region() on
 * the region created from the given coordinates.
 *
 * The region here is specified in widget coordinates.
 * Widget coordinates are a bit odd; for historical reasons, they are
 * defined as @widget->window coordinates for widgets that return %TRUE for
 * ctk_widget_get_has_window(), and are relative to @widget->allocation.x,
 * @widget->allocation.y otherwise.
 *
 * @width or @height may be 0, in this case this function does
 * nothing. Negative values for @width and @height are not allowed.
 */
void
ctk_widget_queue_draw_area (CtkWidget *widget,
			    gint       x,
			    gint       y,
			    gint       width,
			    gint       height)
{
  CdkRectangle rect;
  cairo_region_t *region;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (width >= 0);
  g_return_if_fail (height >= 0);

  if (width == 0 || height == 0)
    return;

  rect.x = x;
  rect.y = y;
  rect.width = width;
  rect.height = height;

  region = cairo_region_create_rectangle (&rect);
  ctk_widget_queue_draw_region (widget, region);
  cairo_region_destroy (region);
}

/**
 * ctk_widget_queue_draw:
 * @widget: a #CtkWidget
 *
 * Equivalent to calling ctk_widget_queue_draw_area() for the
 * entire area of a widget.
 **/
void
ctk_widget_queue_draw (CtkWidget *widget)
{
  CdkRectangle rect;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_widget_get_clip (widget, &rect);

  if (!_ctk_widget_get_has_window (widget))
    ctk_widget_queue_draw_area (widget,
                                rect.x, rect.y, rect.width, rect.height);
  else
    ctk_widget_queue_draw_area (widget,
                                0, 0, rect.width, rect.height);
}

static void
ctk_widget_set_alloc_needed (CtkWidget *widget);
/**
 * ctk_widget_queue_allocate:
 * @widget: a #CtkWidget
 *
 * This function is only for use in widget implementations.
 *
 * Flags the widget for a rerun of the CtkWidgetClass::size_allocate
 * function. Use this function instead of ctk_widget_queue_resize()
 * when the @widget's size request didn't change but it wants to
 * reposition its contents.
 *
 * An example user of this function is ctk_widget_set_halign().
 *
 * Since: 3.20
 */
void
ctk_widget_queue_allocate (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (_ctk_widget_get_realized (widget))
    ctk_widget_queue_draw (widget);

  ctk_widget_set_alloc_needed (widget);
}

/**
 * ctk_widget_queue_resize_internal:
 * @widget: a #CtkWidget
 * 
 * Queue a resize on a widget, and on all other widgets grouped with this widget.
 **/
void
ctk_widget_queue_resize_internal (CtkWidget *widget)
{
  GSList *groups, *l, *widgets;

  if (ctk_widget_get_resize_needed (widget))
    return;

  ctk_widget_queue_resize_on_widget (widget);

  groups = _ctk_widget_get_sizegroups (widget);

  for (l = groups; l; l = l->next)
  {

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    if (ctk_size_group_get_ignore_hidden (l->data) && !ctk_widget_is_visible (widget))
      continue;
G_GNUC_END_IGNORE_DEPRECATIONS

    for (widgets = ctk_size_group_get_widgets (l->data); widgets; widgets = widgets->next)
      {
        ctk_widget_queue_resize_internal (widgets->data);
      }
  }

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  if (CTK_IS_RESIZE_CONTAINER (widget))
    {
      ctk_container_queue_resize_handler (CTK_CONTAINER (widget));
G_GNUC_END_IGNORE_DEPRECATIONS;
    }
  else if (_ctk_widget_get_visible (widget))
    {
      CtkWidget *parent = _ctk_widget_get_parent (widget);
      if (parent)
        ctk_widget_queue_resize_internal (parent);
    }
}

/**
 * ctk_widget_queue_resize:
 * @widget: a #CtkWidget
 *
 * This function is only for use in widget implementations.
 * Flags a widget to have its size renegotiated; should
 * be called when a widget for some reason has a new size request.
 * For example, when you change the text in a #CtkLabel, #CtkLabel
 * queues a resize to ensure there’s enough space for the new text.
 *
 * Note that you cannot call ctk_widget_queue_resize() on a widget
 * from inside its implementation of the CtkWidgetClass::size_allocate 
 * virtual method. Calls to ctk_widget_queue_resize() from inside
 * CtkWidgetClass::size_allocate will be silently ignored.
 **/
void
ctk_widget_queue_resize (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (_ctk_widget_get_realized (widget))
    ctk_widget_queue_draw (widget);

  ctk_widget_queue_resize_internal (widget);
}

/**
 * ctk_widget_queue_resize_no_redraw:
 * @widget: a #CtkWidget
 *
 * This function works like ctk_widget_queue_resize(),
 * except that the widget is not invalidated.
 *
 * Since: 2.4
 **/
void
ctk_widget_queue_resize_no_redraw (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_widget_queue_resize_internal (widget);
}

/**
 * ctk_widget_get_frame_clock:
 * @widget: a #CtkWidget
 *
 * Obtains the frame clock for a widget. The frame clock is a global
 * “ticker” that can be used to drive animations and repaints.  The
 * most common reason to get the frame clock is to call
 * cdk_frame_clock_get_frame_time(), in order to get a time to use for
 * animating. For example you might record the start of the animation
 * with an initial value from cdk_frame_clock_get_frame_time(), and
 * then update the animation by calling
 * cdk_frame_clock_get_frame_time() again during each repaint.
 *
 * cdk_frame_clock_request_phase() will result in a new frame on the
 * clock, but won’t necessarily repaint any widgets. To repaint a
 * widget, you have to use ctk_widget_queue_draw() which invalidates
 * the widget (thus scheduling it to receive a draw on the next
 * frame). ctk_widget_queue_draw() will also end up requesting a frame
 * on the appropriate frame clock.
 *
 * A widget’s frame clock will not change while the widget is
 * mapped. Reparenting a widget (which implies a temporary unmap) can
 * change the widget’s frame clock.
 *
 * Unrealized widgets do not have a frame clock.
 *
 * Returns: (nullable) (transfer none): a #CdkFrameClock,
 * or %NULL if widget is unrealized
 *
 * Since: 3.8
 */
CdkFrameClock*
ctk_widget_get_frame_clock (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  if (widget->priv->realized)
    {
      /* We use ctk_widget_get_toplevel() here to make it explicit that
       * the frame clock is a property of the toplevel that a widget
       * is anchored to; cdk_window_get_toplevel() will go up the
       * hierarchy anyways, but should squash any funny business with
       * reparenting windows and widgets.
       */
      CtkWidget *toplevel = _ctk_widget_get_toplevel (widget);
      CdkWindow *window = _ctk_widget_get_window (toplevel);
      g_assert (window != NULL);

      return cdk_window_get_frame_clock (window);
    }
  else
    {
      return NULL;
    }
}

/**
 * ctk_widget_size_request:
 * @widget: a #CtkWidget
 * @requisition: (out): a #CtkRequisition to be filled in
 *
 * This function is typically used when implementing a #CtkContainer
 * subclass.  Obtains the preferred size of a widget. The container
 * uses this information to arrange its child widgets and decide what
 * size allocations to give them with ctk_widget_size_allocate().
 *
 * You can also call this function from an application, with some
 * caveats. Most notably, getting a size request requires the widget
 * to be associated with a screen, because font information may be
 * needed. Multihead-aware applications should keep this in mind.
 *
 * Also remember that the size request is not necessarily the size
 * a widget will actually be allocated.
 *
 * Deprecated: 3.0: Use ctk_widget_get_preferred_size() instead.
 **/
void
ctk_widget_size_request (CtkWidget	*widget,
			 CtkRequisition *requisition)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_widget_get_preferred_size (widget, requisition, NULL);
}

/**
 * ctk_widget_get_child_requisition:
 * @widget: a #CtkWidget
 * @requisition: (out): a #CtkRequisition to be filled in
 *
 * This function is only for use in widget implementations. Obtains
 * @widget->requisition, unless someone has forced a particular
 * geometry on the widget (e.g. with ctk_widget_set_size_request()),
 * in which case it returns that geometry instead of the widget's
 * requisition.
 *
 * This function differs from ctk_widget_size_request() in that
 * it retrieves the last size request value from @widget->requisition,
 * while ctk_widget_size_request() actually calls the "size_request" method
 * on @widget to compute the size request and fill in @widget->requisition,
 * and only then returns @widget->requisition.
 *
 * Because this function does not call the “size_request” method, it
 * can only be used when you know that @widget->requisition is
 * up-to-date, that is, ctk_widget_size_request() has been called
 * since the last time a resize was queued. In general, only container
 * implementations have this information; applications should use
 * ctk_widget_size_request().
 *
 *
 * Deprecated: 3.0: Use ctk_widget_get_preferred_size() instead.
 **/
void
ctk_widget_get_child_requisition (CtkWidget	 *widget,
				  CtkRequisition *requisition)
{
  ctk_widget_get_preferred_size (widget, requisition, NULL);
}

static gboolean
invalidate_predicate (CdkWindow *window,
		      gpointer   data)
{
  gpointer user_data;

  cdk_window_get_user_data (window, &user_data);

  return (user_data == data);
}

/* Invalidate @region in widget->window and all children
 * of widget->window owned by widget. @region is in the
 * same coordinates as widget->allocation and will be
 * modified by this call.
 */
static void
ctk_widget_invalidate_widget_windows (CtkWidget      *widget,
				      cairo_region_t *region)
{
  CtkWidgetPrivate *priv = widget->priv;

  if (!_ctk_widget_get_realized (widget))
    return;

  if (_ctk_widget_get_has_window (widget) && priv->parent)
    {
      int x, y;

      cdk_window_get_position (priv->window, &x, &y);
      cairo_region_translate (region, -x, -y);
    }

  cdk_window_invalidate_maybe_recurse (priv->window, region,
				       invalidate_predicate, widget);
}

/**
 * ctk_widget_size_allocate_with_baseline:
 * @widget: a #CtkWidget
 * @allocation: position and size to be allocated to @widget
 * @baseline: The baseline of the child, or -1
 *
 * This function is only used by #CtkContainer subclasses, to assign a size,
 * position and (optionally) baseline to their child widgets.
 *
 * In this function, the allocation and baseline may be adjusted. It
 * will be forced to a 1x1 minimum size, and the
 * adjust_size_allocation virtual and adjust_baseline_allocation
 * methods on the child will be used to adjust the allocation and
 * baseline. Standard adjustments include removing the widget's
 * margins, and applying the widget’s #CtkWidget:halign and
 * #CtkWidget:valign properties.
 *
 * If the child widget does not have a valign of %CTK_ALIGN_BASELINE the
 * baseline argument is ignored and -1 is used instead.
 *
 * Since: 3.10
 **/
void
ctk_widget_size_allocate_with_baseline (CtkWidget     *widget,
					CtkAllocation *allocation,
					gint           baseline)
{
  CtkWidgetPrivate *priv;
  CdkRectangle real_allocation;
  CdkRectangle old_allocation, old_clip;
  CdkRectangle adjusted_allocation;
  gboolean alloc_needed;
  gboolean size_changed;
  gboolean baseline_changed;
  gboolean position_changed;
  gint natural_width, natural_height, dummy;
  gint min_width, min_height;
  gint old_baseline;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = widget->priv;

  if (!priv->visible && !_ctk_widget_is_toplevel (widget))
    return;

  ctk_widget_push_verify_invariants (widget);

#ifdef G_ENABLE_DEBUG
  if (CTK_DISPLAY_DEBUG_CHECK (ctk_widget_get_display (widget), RESIZE))
    {
      priv->highlight_resize = TRUE;
      ctk_widget_queue_draw (widget);
    }

#ifdef G_ENABLE_CONSISTENCY_CHECKS
  if (ctk_widget_get_resize_needed (widget))
    {
      g_warning ("Allocating size to %s %p without calling ctk_widget_get_preferred_width/height(). "
                 "How does the code know the size to allocate?",
                 ctk_widget_get_name (widget), widget);
    }
#endif

  if (CTK_DEBUG_CHECK (GEOMETRY))
    {
      gint depth;
      CtkWidget *parent;
      const gchar *name;

      depth = 0;
      parent = widget;
      while (parent)
	{
	  depth++;
	  parent = _ctk_widget_get_parent (parent);
	}

      name = g_type_name (G_OBJECT_TYPE (G_OBJECT (widget)));
      g_message ("ctk_widget_size_allocate: %*s%s %d %d %d %d, baseline %d",
                 2 * depth, " ", name,
                 allocation->x, allocation->y,
                 allocation->width, allocation->height,
                 baseline);
    }
#endif /* G_ENABLE_DEBUG */

  /* Never pass a baseline to a child unless it requested it.
     This means containers don't have to manually check for this. */
  if (baseline != -1 &&
      (ctk_widget_get_valign_with_baseline (widget) != CTK_ALIGN_BASELINE ||
       !_ctk_widget_has_baseline_support (widget)))
    baseline = -1;

  alloc_needed = priv->alloc_needed;
  /* Preserve request/allocate ordering */
  priv->alloc_needed = FALSE;

  old_allocation = priv->allocation;
  old_clip = priv->clip;
  old_baseline = priv->allocated_baseline;
  real_allocation = *allocation;

  priv->allocated_size = *allocation;
  priv->allocated_size_baseline = baseline;

  adjusted_allocation = real_allocation;
  if (ctk_widget_get_request_mode (widget) == CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH)
    {
      /* Go ahead and request the height for allocated width, note that the internals
       * of get_height_for_width will internally limit the for_size to natural size
       * when aligning implicitly.
       */
      ctk_widget_get_preferred_width (widget, &min_width, &natural_width);
      ctk_widget_get_preferred_height_for_width (widget, real_allocation.width, &min_height, &natural_height);
    }
  else
    {
      /* Go ahead and request the width for allocated height, note that the internals
       * of get_width_for_height will internally limit the for_size to natural size
       * when aligning implicitly.
       */
      ctk_widget_get_preferred_height (widget, &min_height, &natural_height);
      ctk_widget_get_preferred_width_for_height (widget, real_allocation.height, &min_width, &natural_width);
    }

#ifdef G_ENABLE_CONSISTENCY_CHECKS
  if ((min_width > real_allocation.width || min_height > real_allocation.height) &&
      !CTK_IS_SCROLLABLE (widget))
    g_warning ("ctk_widget_size_allocate(): attempt to underallocate %s%s %s %p. "
               "Allocation is %dx%d, but minimum required size is %dx%d.",
               priv->parent ? G_OBJECT_TYPE_NAME (priv->parent) : "", priv->parent ? "'s child" : "toplevel",
               G_OBJECT_TYPE_NAME (widget), widget,
               real_allocation.width, real_allocation.height,
               min_width, min_height);
#endif
  /* Now that we have the right natural height and width, go ahead and remove any margins from the
   * allocated sizes and possibly limit them to the natural sizes */
  CTK_WIDGET_GET_CLASS (widget)->adjust_size_allocation (widget,
							 CTK_ORIENTATION_HORIZONTAL,
							 &dummy,
							 &natural_width,
							 &adjusted_allocation.x,
							 &adjusted_allocation.width);
  CTK_WIDGET_GET_CLASS (widget)->adjust_size_allocation (widget,
							 CTK_ORIENTATION_VERTICAL,
							 &dummy,
							 &natural_height,
							 &adjusted_allocation.y,
							 &adjusted_allocation.height);
  if (baseline >= 0)
    CTK_WIDGET_GET_CLASS (widget)->adjust_baseline_allocation (widget,
							       &baseline);

  if (adjusted_allocation.x < real_allocation.x ||
      adjusted_allocation.y < real_allocation.y ||
      (adjusted_allocation.x + adjusted_allocation.width) >
      (real_allocation.x + real_allocation.width) ||
      (adjusted_allocation.y + adjusted_allocation.height >
       real_allocation.y + real_allocation.height))
    {
      g_warning ("%s %p attempted to adjust its size allocation from %d,%d %dx%d to %d,%d %dx%d. adjust_size_allocation must keep allocation inside original bounds",
                 G_OBJECT_TYPE_NAME (widget), widget,
                 real_allocation.x, real_allocation.y, real_allocation.width, real_allocation.height,
                 adjusted_allocation.x, adjusted_allocation.y, adjusted_allocation.width, adjusted_allocation.height);
    }
  else
    {
      real_allocation = adjusted_allocation;
    }

  if (real_allocation.width < 0 || real_allocation.height < 0)
    {
      g_warning ("ctk_widget_size_allocate(): attempt to allocate widget with width %d and height %d",
		 real_allocation.width,
		 real_allocation.height);
    }

  real_allocation.width = MAX (real_allocation.width, 1);
  real_allocation.height = MAX (real_allocation.height, 1);

  baseline_changed = old_baseline != baseline;
  size_changed = (old_allocation.width != real_allocation.width ||
		  old_allocation.height != real_allocation.height);
  position_changed = (old_allocation.x != real_allocation.x ||
		      old_allocation.y != real_allocation.y);

  if (!alloc_needed && !size_changed && !position_changed && !baseline_changed)
    goto out;

  priv->allocated_baseline = baseline;
  if (g_signal_has_handler_pending (widget, widget_signals[SIZE_ALLOCATE], 0, FALSE))
    g_signal_emit (widget, widget_signals[SIZE_ALLOCATE], 0, &real_allocation);
  else
    CTK_WIDGET_GET_CLASS (widget)->size_allocate (widget, &real_allocation);

  /* Size allocation is god... after consulting god, no further requests or allocations are needed */
#ifdef G_ENABLE_DEBUG
  if (CTK_DEBUG_CHECK (GEOMETRY) && ctk_widget_get_resize_needed (widget))
    {
      g_warning ("%s %p or a child called ctk_widget_queue_resize() during size_allocate().",
                 ctk_widget_get_name (widget), widget);
    }
#endif
  ctk_widget_ensure_resize (widget);
  priv->alloc_needed = FALSE;
  priv->alloc_needed_on_child = FALSE;

  size_changed |= (old_clip.width != priv->clip.width ||
                   old_clip.height != priv->clip.height);
  position_changed |= (old_clip.x != priv->clip.x ||
                      old_clip.y != priv->clip.y);

  if (_ctk_widget_get_mapped (widget) && priv->redraw_on_alloc)
    {
      if (!_ctk_widget_get_has_window (widget) && position_changed)
	{
	  /* Invalidate union(old_clip,priv->clip) in priv->window
	   */
	  cairo_region_t *invalidate = cairo_region_create_rectangle (&priv->clip);
	  cairo_region_union_rectangle (invalidate, &old_clip);

	  cdk_window_invalidate_region (priv->window, invalidate, FALSE);
	  cairo_region_destroy (invalidate);
	}

      if (size_changed || baseline_changed)
	{
          /* Invalidate union(old_clip,priv->clip) in priv->window and descendants owned by widget
           */
          cairo_region_t *invalidate = cairo_region_create_rectangle (&priv->clip);
          cairo_region_union_rectangle (invalidate, &old_clip);

          ctk_widget_invalidate_widget_windows (widget, invalidate);
          cairo_region_destroy (invalidate);
	}
    }

  if ((size_changed || position_changed || baseline_changed) && priv->parent &&
      _ctk_widget_get_realized (priv->parent) && _ctk_container_get_reallocate_redraws (CTK_CONTAINER (priv->parent)))
    {
      cairo_region_t *invalidate = cairo_region_create_rectangle (&priv->parent->priv->clip);
      ctk_widget_invalidate_widget_windows (priv->parent, invalidate);
      cairo_region_destroy (invalidate);
    }

out:
  if (priv->alloc_needed_on_child)
    ctk_widget_ensure_allocate (widget);

  ctk_widget_pop_verify_invariants (widget);
}


/**
 * ctk_widget_size_allocate:
 * @widget: a #CtkWidget
 * @allocation: position and size to be allocated to @widget
 *
 * This function is only used by #CtkContainer subclasses, to assign a size
 * and position to their child widgets.
 *
 * In this function, the allocation may be adjusted. It will be forced
 * to a 1x1 minimum size, and the adjust_size_allocation virtual
 * method on the child will be used to adjust the allocation. Standard
 * adjustments include removing the widget’s margins, and applying the
 * widget’s #CtkWidget:halign and #CtkWidget:valign properties.
 *
 * For baseline support in containers you need to use ctk_widget_size_allocate_with_baseline()
 * instead.
 **/
void
ctk_widget_size_allocate (CtkWidget	*widget,
			  CtkAllocation *allocation)
{
  ctk_widget_size_allocate_with_baseline (widget, allocation, -1);
}

/**
 * ctk_widget_common_ancestor:
 * @widget_a: a #CtkWidget
 * @widget_b: a #CtkWidget
 *
 * Find the common ancestor of @widget_a and @widget_b that
 * is closest to the two widgets.
 *
 * Returns: (nullable): the closest common ancestor of @widget_a and
 *   @widget_b or %NULL if @widget_a and @widget_b do not
 *   share a common ancestor.
 **/
static CtkWidget *
ctk_widget_common_ancestor (CtkWidget *widget_a,
			    CtkWidget *widget_b)
{
  CtkWidget *parent_a;
  CtkWidget *parent_b;
  gint depth_a = 0;
  gint depth_b = 0;

  parent_a = widget_a;
  while (parent_a->priv->parent)
    {
      parent_a = parent_a->priv->parent;
      depth_a++;
    }

  parent_b = widget_b;
  while (parent_b->priv->parent)
    {
      parent_b = parent_b->priv->parent;
      depth_b++;
    }

  if (parent_a != parent_b)
    return NULL;

  while (depth_a > depth_b)
    {
      widget_a = widget_a->priv->parent;
      depth_a--;
    }

  while (depth_b > depth_a)
    {
      widget_b = widget_b->priv->parent;
      depth_b--;
    }

  while (widget_a != widget_b)
    {
      widget_a = widget_a->priv->parent;
      widget_b = widget_b->priv->parent;
    }

  return widget_a;
}

/**
 * ctk_widget_translate_coordinates:
 * @src_widget:  a #CtkWidget
 * @dest_widget: a #CtkWidget
 * @src_x: X position relative to @src_widget
 * @src_y: Y position relative to @src_widget
 * @dest_x: (out) (optional): location to store X position relative to @dest_widget
 * @dest_y: (out) (optional): location to store Y position relative to @dest_widget
 *
 * Translate coordinates relative to @src_widget’s allocation to coordinates
 * relative to @dest_widget’s allocations. In order to perform this
 * operation, both widgets must be realized, and must share a common
 * toplevel.
 *
 * Returns: %FALSE if either widget was not realized, or there
 *   was no common ancestor. In this case, nothing is stored in
 *   *@dest_x and *@dest_y. Otherwise %TRUE.
 **/
gboolean
ctk_widget_translate_coordinates (CtkWidget  *src_widget,
				  CtkWidget  *dest_widget,
				  gint        src_x,
				  gint        src_y,
				  gint       *dest_x,
				  gint       *dest_y)
{
  CtkWidgetPrivate *src_priv;
  CtkWidgetPrivate *dest_priv;
  CtkWidget *ancestor;
  CdkWindow *window;
  GList *dest_list = NULL;

  g_return_val_if_fail (CTK_IS_WIDGET (src_widget), FALSE);
  g_return_val_if_fail (CTK_IS_WIDGET (dest_widget), FALSE);

  ancestor = ctk_widget_common_ancestor (src_widget, dest_widget);
  if (!ancestor || !_ctk_widget_get_realized (src_widget) || !_ctk_widget_get_realized (dest_widget))
    return FALSE;

  src_priv = src_widget->priv;
  dest_priv = dest_widget->priv;

  /* Translate from allocation relative to window relative */
  if (_ctk_widget_get_has_window (src_widget) && src_priv->parent)
    {
      gint wx, wy;
      cdk_window_get_position (src_priv->window, &wx, &wy);

      src_x -= wx - src_priv->allocation.x;
      src_y -= wy - src_priv->allocation.y;
    }
  else
    {
      src_x += src_priv->allocation.x;
      src_y += src_priv->allocation.y;
    }

  /* Translate to the common ancestor */
  window = src_priv->window;
  while (window != ancestor->priv->window)
    {
      gdouble dx, dy;

      cdk_window_coords_to_parent (window, src_x, src_y, &dx, &dy);

      src_x = dx;
      src_y = dy;

      window = cdk_window_get_effective_parent (window);

      if (!window)		/* Handle CtkHandleBox */
	return FALSE;
    }

  /* And back */
  window = dest_priv->window;
  while (window != ancestor->priv->window)
    {
      dest_list = g_list_prepend (dest_list, window);

      window = cdk_window_get_effective_parent (window);

      if (!window)		/* Handle CtkHandleBox */
        {
          g_list_free (dest_list);
          return FALSE;
        }
    }

  while (dest_list)
    {
      gdouble dx, dy;

      cdk_window_coords_from_parent (dest_list->data, src_x, src_y, &dx, &dy);

      src_x = dx;
      src_y = dy;

      dest_list = g_list_remove (dest_list, dest_list->data);
    }

  /* Translate from window relative to allocation relative */
  if (_ctk_widget_get_has_window (dest_widget) && dest_priv->parent)
    {
      gint wx, wy;
      cdk_window_get_position (dest_priv->window, &wx, &wy);

      src_x += wx - dest_priv->allocation.x;
      src_y += wy - dest_priv->allocation.y;
    }
  else
    {
      src_x -= dest_priv->allocation.x;
      src_y -= dest_priv->allocation.y;
    }

  if (dest_x)
    *dest_x = src_x;
  if (dest_y)
    *dest_y = src_y;

  return TRUE;
}

static void
ctk_widget_real_size_allocate (CtkWidget     *widget,
			       CtkAllocation *allocation)
{
  CtkWidgetPrivate *priv = widget->priv;

  ctk_widget_set_allocation (widget, allocation);

  if (_ctk_widget_get_realized (widget) &&
      _ctk_widget_get_has_window (widget))
     {
	cdk_window_move_resize (priv->window,
				allocation->x, allocation->y,
				allocation->width, allocation->height);
     }
}

/* translate initial/final into start/end */
static CtkAlign
effective_align (CtkAlign         align,
                 CtkTextDirection direction)
{
  switch (align)
    {
    case CTK_ALIGN_START:
      return direction == CTK_TEXT_DIR_RTL ? CTK_ALIGN_END : CTK_ALIGN_START;
    case CTK_ALIGN_END:
      return direction == CTK_TEXT_DIR_RTL ? CTK_ALIGN_START : CTK_ALIGN_END;
    default:
      return align;
    }
}

static void
adjust_for_align (CtkAlign  align,
                  gint     *natural_size,
                  gint     *allocated_pos,
                  gint     *allocated_size)
{
  switch (align)
    {
    case CTK_ALIGN_BASELINE:
    case CTK_ALIGN_FILL:
      /* change nothing */
      break;
    case CTK_ALIGN_START:
      /* keep *allocated_pos where it is */
      *allocated_size = MIN (*allocated_size, *natural_size);
      break;
    case CTK_ALIGN_END:
      if (*allocated_size > *natural_size)
	{
	  *allocated_pos += (*allocated_size - *natural_size);
	  *allocated_size = *natural_size;
	}
      break;
    case CTK_ALIGN_CENTER:
      if (*allocated_size > *natural_size)
	{
	  *allocated_pos += (*allocated_size - *natural_size) / 2;
	  *allocated_size = MIN (*allocated_size, *natural_size);
	}
      break;
    }
}

static void
adjust_for_margin(gint               start_margin,
                  gint               end_margin,
                  gint              *minimum_size,
                  gint              *natural_size,
                  gint              *allocated_pos,
                  gint              *allocated_size)
{
  *minimum_size -= (start_margin + end_margin);
  *natural_size -= (start_margin + end_margin);
  *allocated_pos += start_margin;
  *allocated_size -= (start_margin + end_margin);
}

static void
ctk_widget_real_adjust_size_allocation (CtkWidget         *widget,
                                        CtkOrientation     orientation,
                                        gint              *minimum_size,
                                        gint              *natural_size,
                                        gint              *allocated_pos,
                                        gint              *allocated_size)
{
  CtkWidgetPrivate *priv = widget->priv;

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      adjust_for_margin (priv->margin.left,
                         priv->margin.right,
                         minimum_size, natural_size,
                         allocated_pos, allocated_size);
      adjust_for_align (effective_align (priv->halign, _ctk_widget_get_direction (widget)),
                        natural_size, allocated_pos, allocated_size);
    }
  else
    {
      adjust_for_margin (priv->margin.top,
                         priv->margin.bottom,
                         minimum_size, natural_size,
                         allocated_pos, allocated_size);
      adjust_for_align (effective_align (priv->valign, CTK_TEXT_DIR_NONE),
                        natural_size, allocated_pos, allocated_size);
    }
}

static void
ctk_widget_real_adjust_baseline_allocation (CtkWidget *widget,
					    gint      *baseline)
{
  if (*baseline >= 0)
    *baseline -= widget->priv->margin.top;
}

static gboolean
ctk_widget_real_can_activate_accel (CtkWidget *widget,
                                    guint      signal_id)
{
  CtkWidgetPrivate *priv = widget->priv;

  /* widgets must be onscreen for accels to take effect */
  return ctk_widget_is_sensitive (widget) &&
         _ctk_widget_is_drawable (widget) &&
         cdk_window_is_viewable (priv->window);
}

/**
 * ctk_widget_can_activate_accel:
 * @widget: a #CtkWidget
 * @signal_id: the ID of a signal installed on @widget
 *
 * Determines whether an accelerator that activates the signal
 * identified by @signal_id can currently be activated.
 * This is done by emitting the #CtkWidget::can-activate-accel
 * signal on @widget; if the signal isn’t overridden by a
 * handler or in a derived widget, then the default check is
 * that the widget must be sensitive, and the widget and all
 * its ancestors mapped.
 *
 * Returns: %TRUE if the accelerator can be activated.
 *
 * Since: 2.4
 **/
gboolean
ctk_widget_can_activate_accel (CtkWidget *widget,
                               guint      signal_id)
{
  gboolean can_activate = FALSE;
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);
  g_signal_emit (widget, widget_signals[CAN_ACTIVATE_ACCEL], 0, signal_id, &can_activate);
  return can_activate;
}

typedef struct {
  GClosure   closure;
  guint      signal_id;
} AccelClosure;

static void
closure_accel_activate (GClosure     *closure,
			GValue       *return_value,
			guint         n_param_values,
			const GValue *param_values,
			gpointer      invocation_hint,
			gpointer      marshal_data)
{
  AccelClosure *aclosure = (AccelClosure*) closure;
  gboolean can_activate = ctk_widget_can_activate_accel (closure->data, aclosure->signal_id);

  if (can_activate)
    g_signal_emit (closure->data, aclosure->signal_id, 0);

  /* whether accelerator was handled */
  g_value_set_boolean (return_value, can_activate);
}

static void
closures_destroy (gpointer data)
{
  GSList *slist, *closures = data;

  for (slist = closures; slist; slist = slist->next)
    {
      g_closure_invalidate (slist->data);
      g_closure_unref (slist->data);
    }
  g_slist_free (closures);
}

static GClosure*
widget_new_accel_closure (CtkWidget *widget,
			  guint      signal_id)
{
  AccelClosure *aclosure;
  GClosure *closure = NULL;
  GSList *slist, *closures;

  closures = g_object_steal_qdata (G_OBJECT (widget), quark_accel_closures);
  for (slist = closures; slist; slist = slist->next)
    if (!ctk_accel_group_from_accel_closure (slist->data))
      {
	/* reuse this closure */
	closure = slist->data;
	break;
      }
  if (!closure)
    {
      closure = g_closure_new_object (sizeof (AccelClosure), G_OBJECT (widget));
      closures = g_slist_prepend (closures, g_closure_ref (closure));
      g_closure_sink (closure);
      g_closure_set_marshal (closure, closure_accel_activate);
    }
  g_object_set_qdata_full (G_OBJECT (widget), quark_accel_closures, closures, closures_destroy);

  aclosure = (AccelClosure*) closure;
  g_assert (closure->data == widget);
  g_assert (closure->marshal == closure_accel_activate);
  aclosure->signal_id = signal_id;

  return closure;
}

/**
 * ctk_widget_add_accelerator:
 * @widget:       widget to install an accelerator on
 * @accel_signal: widget signal to emit on accelerator activation
 * @accel_group:  accel group for this widget, added to its toplevel
 * @accel_key:    CDK keyval of the accelerator
 * @accel_mods:   modifier key combination of the accelerator
 * @accel_flags:  flag accelerators, e.g. %CTK_ACCEL_VISIBLE
 *
 * Installs an accelerator for this @widget in @accel_group that causes
 * @accel_signal to be emitted if the accelerator is activated.
 * The @accel_group needs to be added to the widget’s toplevel via
 * ctk_window_add_accel_group(), and the signal must be of type %G_SIGNAL_ACTION.
 * Accelerators added through this function are not user changeable during
 * runtime. If you want to support accelerators that can be changed by the
 * user, use ctk_accel_map_add_entry() and ctk_widget_set_accel_path() or
 * ctk_menu_item_set_accel_path() instead.
 */
void
ctk_widget_add_accelerator (CtkWidget      *widget,
			    const gchar    *accel_signal,
			    CtkAccelGroup  *accel_group,
			    guint           accel_key,
			    CdkModifierType accel_mods,
			    CtkAccelFlags   accel_flags)
{
  GClosure *closure;
  GSignalQuery query;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (accel_signal != NULL);
  g_return_if_fail (CTK_IS_ACCEL_GROUP (accel_group));

  g_signal_query (g_signal_lookup (accel_signal, G_OBJECT_TYPE (widget)), &query);
  if (!query.signal_id ||
      !(query.signal_flags & G_SIGNAL_ACTION) ||
      query.return_type != G_TYPE_NONE ||
      query.n_params)
    {
      /* hmm, should be elaborate enough */
      g_warning (G_STRLOC ": widget '%s' has no activatable signal \"%s\" without arguments",
		 G_OBJECT_TYPE_NAME (widget), accel_signal);
      return;
    }

  closure = widget_new_accel_closure (widget, query.signal_id);

  g_object_ref (widget);

  /* install the accelerator. since we don't map this onto an accel_path,
   * the accelerator will automatically be locked.
   */
  ctk_accel_group_connect (accel_group,
			   accel_key,
			   accel_mods,
			   accel_flags | CTK_ACCEL_LOCKED,
			   closure);

  g_signal_emit (widget, widget_signals[ACCEL_CLOSURES_CHANGED], 0);

  g_object_unref (widget);
}

/**
 * ctk_widget_remove_accelerator:
 * @widget:       widget to install an accelerator on
 * @accel_group:  accel group for this widget
 * @accel_key:    CDK keyval of the accelerator
 * @accel_mods:   modifier key combination of the accelerator
 *
 * Removes an accelerator from @widget, previously installed with
 * ctk_widget_add_accelerator().
 *
 * Returns: whether an accelerator was installed and could be removed
 */
gboolean
ctk_widget_remove_accelerator (CtkWidget      *widget,
			       CtkAccelGroup  *accel_group,
			       guint           accel_key,
			       CdkModifierType accel_mods)
{
  CtkAccelGroupEntry *ag_entry;
  GList *slist, *clist;
  guint n;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (CTK_IS_ACCEL_GROUP (accel_group), FALSE);

  ag_entry = ctk_accel_group_query (accel_group, accel_key, accel_mods, &n);
  clist = ctk_widget_list_accel_closures (widget);
  for (slist = clist; slist; slist = slist->next)
    {
      guint i;

      for (i = 0; i < n; i++)
	if (slist->data == (gpointer) ag_entry[i].closure)
	  {
	    gboolean is_removed = ctk_accel_group_disconnect (accel_group, slist->data);

	    g_signal_emit (widget, widget_signals[ACCEL_CLOSURES_CHANGED], 0);

	    g_list_free (clist);

	    return is_removed;
	  }
    }
  g_list_free (clist);

  g_warning (G_STRLOC ": no accelerator (%u,%u) installed in accel group (%p) for %s (%p)",
	     accel_key, accel_mods, accel_group,
	     G_OBJECT_TYPE_NAME (widget), widget);

  return FALSE;
}

/**
 * ctk_widget_list_accel_closures:
 * @widget:  widget to list accelerator closures for
 *
 * Lists the closures used by @widget for accelerator group connections
 * with ctk_accel_group_connect_by_path() or ctk_accel_group_connect().
 * The closures can be used to monitor accelerator changes on @widget,
 * by connecting to the @CtkAccelGroup::accel-changed signal of the
 * #CtkAccelGroup of a closure which can be found out with
 * ctk_accel_group_from_accel_closure().
 *
 * Returns: (transfer container) (element-type GClosure):
 *     a newly allocated #GList of closures
 */
GList*
ctk_widget_list_accel_closures (CtkWidget *widget)
{
  GSList *slist;
  GList *clist = NULL;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  for (slist = g_object_get_qdata (G_OBJECT (widget), quark_accel_closures); slist; slist = slist->next)
    if (ctk_accel_group_from_accel_closure (slist->data))
      clist = g_list_prepend (clist, slist->data);
  return clist;
}

typedef struct {
  GQuark         path_quark;
  CtkAccelGroup *accel_group;
  GClosure      *closure;
} AccelPath;

static void
destroy_accel_path (gpointer data)
{
  AccelPath *apath = data;

  ctk_accel_group_disconnect (apath->accel_group, apath->closure);

  /* closures_destroy takes care of unrefing the closure */
  g_object_unref (apath->accel_group);

  g_slice_free (AccelPath, apath);
}


/**
 * ctk_widget_set_accel_path:
 * @widget: a #CtkWidget
 * @accel_path: (allow-none): path used to look up the accelerator
 * @accel_group: (allow-none): a #CtkAccelGroup.
 *
 * Given an accelerator group, @accel_group, and an accelerator path,
 * @accel_path, sets up an accelerator in @accel_group so whenever the
 * key binding that is defined for @accel_path is pressed, @widget
 * will be activated.  This removes any accelerators (for any
 * accelerator group) installed by previous calls to
 * ctk_widget_set_accel_path(). Associating accelerators with
 * paths allows them to be modified by the user and the modifications
 * to be saved for future use. (See ctk_accel_map_save().)
 *
 * This function is a low level function that would most likely
 * be used by a menu creation system like #CtkUIManager. If you
 * use #CtkUIManager, setting up accelerator paths will be done
 * automatically.
 *
 * Even when you you aren’t using #CtkUIManager, if you only want to
 * set up accelerators on menu items ctk_menu_item_set_accel_path()
 * provides a somewhat more convenient interface.
 *
 * Note that @accel_path string will be stored in a #GQuark. Therefore, if you
 * pass a static string, you can save some memory by interning it first with
 * g_intern_static_string().
 **/
void
ctk_widget_set_accel_path (CtkWidget     *widget,
			   const gchar   *accel_path,
			   CtkAccelGroup *accel_group)
{
  AccelPath *apath;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CTK_WIDGET_GET_CLASS (widget)->activate_signal != 0);

  if (accel_path)
    {
      g_return_if_fail (CTK_IS_ACCEL_GROUP (accel_group));
      g_return_if_fail (_ctk_accel_path_is_valid (accel_path));

      ctk_accel_map_add_entry (accel_path, 0, 0);
      apath = g_slice_new (AccelPath);
      apath->accel_group = g_object_ref (accel_group);
      apath->path_quark = g_quark_from_string (accel_path);
      apath->closure = widget_new_accel_closure (widget, CTK_WIDGET_GET_CLASS (widget)->activate_signal);
    }
  else
    apath = NULL;

  /* also removes possible old settings */
  g_object_set_qdata_full (G_OBJECT (widget), quark_accel_path, apath, destroy_accel_path);

  if (apath)
    ctk_accel_group_connect_by_path (apath->accel_group, g_quark_to_string (apath->path_quark), apath->closure);

  g_signal_emit (widget, widget_signals[ACCEL_CLOSURES_CHANGED], 0);
}

const gchar*
_ctk_widget_get_accel_path (CtkWidget *widget,
			    gboolean  *locked)
{
  AccelPath *apath;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  apath = g_object_get_qdata (G_OBJECT (widget), quark_accel_path);
  if (locked)
    *locked = apath ? ctk_accel_group_get_is_locked (apath->accel_group) : TRUE;
  return apath ? g_quark_to_string (apath->path_quark) : NULL;
}

/**
 * ctk_widget_mnemonic_activate:
 * @widget: a #CtkWidget
 * @group_cycling: %TRUE if there are other widgets with the same mnemonic
 *
 * Emits the #CtkWidget::mnemonic-activate signal.
 *
 * Returns: %TRUE if the signal has been handled
 */
gboolean
ctk_widget_mnemonic_activate (CtkWidget *widget,
                              gboolean   group_cycling)
{
  gboolean handled;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  group_cycling = group_cycling != FALSE;
  if (!ctk_widget_is_sensitive (widget))
    handled = TRUE;
  else
    g_signal_emit (widget,
		   widget_signals[MNEMONIC_ACTIVATE],
		   0,
		   group_cycling,
		   &handled);
  return handled;
}

static gboolean
ctk_widget_real_mnemonic_activate (CtkWidget *widget,
                                   gboolean   group_cycling)
{
  if (!group_cycling && CTK_WIDGET_GET_CLASS (widget)->activate_signal)
    ctk_widget_activate (widget);
  else if (ctk_widget_get_can_focus (widget))
    ctk_widget_grab_focus (widget);
  else
    {
      g_warning ("widget '%s' isn't suitable for mnemonic activation",
		 G_OBJECT_TYPE_NAME (widget));
      ctk_widget_error_bell (widget);
    }
  return TRUE;
}

static const cairo_user_data_key_t mark_for_draw_key;

static gboolean
ctk_cairo_is_marked_for_draw (cairo_t *cr)
{
  return cairo_get_user_data (cr, &mark_for_draw_key) != NULL;
}

static void
ctk_cairo_set_marked_for_draw (cairo_t  *cr,
                               gboolean  marked)
{
  if (marked)
    cairo_set_user_data (cr, &mark_for_draw_key, GINT_TO_POINTER (1), NULL);
  else
    cairo_set_user_data (cr, &mark_for_draw_key, NULL, NULL);
}

/**
 * ctk_cairo_should_draw_window:
 * @cr: a cairo context
 * @window: the window to check. @window may not be an input-only
 *          window.
 *
 * This function is supposed to be called in #CtkWidget::draw
 * implementations for widgets that support multiple windows.
 * @cr must be untransformed from invoking of the draw function.
 * This function will return %TRUE if the contents of the given
 * @window are supposed to be drawn and %FALSE otherwise. Note
 * that when the drawing was not initiated by the windowing
 * system this function will return %TRUE for all windows, so
 * you need to draw the bottommost window first. Also, do not
 * use “else if” statements to check which window should be drawn.
 *
 * Returns: %TRUE if @window should be drawn
 *
 * Since: 3.0
 */
gboolean
ctk_cairo_should_draw_window (cairo_t   *cr,
                              CdkWindow *window)
{
  CdkDrawingContext *context;
  CdkWindow *tmp;

  g_return_val_if_fail (cr != NULL, FALSE);
  g_return_val_if_fail (CDK_IS_WINDOW (window), FALSE);

  if (ctk_cairo_is_marked_for_draw (cr))
    return TRUE;

  context = cdk_cairo_get_drawing_context (cr);
  if (context == NULL)
    return TRUE;

  tmp = cdk_drawing_context_get_window (context);
  if (tmp == NULL)
    return TRUE;

  while (!cdk_window_has_native (window))
    window = cdk_window_get_parent (window);

  return tmp == window;
}

void
ctk_widget_draw_internal (CtkWidget *widget,
                          cairo_t   *cr,
                          gboolean   clip_to_size)
{
  if (!_ctk_widget_is_drawable (widget))
    return;

  if (clip_to_size)
    {
      cairo_rectangle (cr,
                       widget->priv->clip.x - widget->priv->allocation.x,
                       widget->priv->clip.y - widget->priv->allocation.y,
                       widget->priv->clip.width,
                       widget->priv->clip.height);
      cairo_clip (cr);
    }

  if (cdk_cairo_get_clip_rectangle (cr, NULL))
    {
      CdkWindow *event_window = NULL;
      gboolean result;
      gboolean push_group;

      /* If this was a cairo_t passed via ctk_widget_draw() then we don't
       * require a window; otherwise we check for the window associated
       * to the drawing context and mark it using the clip region of the
       * Cairo context.
       */
      if (!ctk_cairo_is_marked_for_draw (cr))
        {
          CdkDrawingContext *context = cdk_cairo_get_drawing_context (cr);

          if (context != NULL)
            {
              event_window = cdk_drawing_context_get_window (context);
              if (event_window != NULL)
                cdk_window_mark_paint_from_clip (event_window, cr);
            }
        }

      push_group =
        widget->priv->alpha != 255 &&
        (!_ctk_widget_is_toplevel (widget) ||
         ctk_widget_get_visual (widget) == cdk_screen_get_rgba_visual (ctk_widget_get_screen (widget)));

      if (push_group)
        cairo_push_group (cr);

#ifdef G_ENABLE_CONSISTENCY_CHECKS
      if (_ctk_widget_get_alloc_needed (widget))
        g_warning ("%s %p is drawn without a current allocation. This should not happen.", G_OBJECT_TYPE_NAME (widget), widget);
#endif

      if (g_signal_has_handler_pending (widget, widget_signals[DRAW], 0, FALSE))
        {
          g_signal_emit (widget, widget_signals[DRAW],
                         0, cr,
                         &result);
        }
      else if (CTK_WIDGET_GET_CLASS (widget)->draw)
        {
          cairo_save (cr);
          CTK_WIDGET_GET_CLASS (widget)->draw (widget, cr);
          cairo_restore (cr);
        }

#ifdef G_ENABLE_DEBUG
      if (CTK_DISPLAY_DEBUG_CHECK (ctk_widget_get_display (widget), BASELINES))
	{
	  gint baseline = ctk_widget_get_allocated_baseline (widget);
	  gint width = ctk_widget_get_allocated_width (widget);

	  if (baseline != -1)
	    {
	      cairo_save (cr);
	      cairo_new_path (cr);
	      cairo_move_to (cr, 0, baseline+0.5);
	      cairo_line_to (cr, width, baseline+0.5);
	      cairo_set_line_width (cr, 1.0);
	      cairo_set_source_rgba (cr, 1.0, 0, 0, 0.25);
	      cairo_stroke (cr);
	      cairo_restore (cr);
	    }
	}
      if (widget->priv->highlight_resize)
        {
          CtkAllocation alloc;
          ctk_widget_get_allocation (widget, &alloc);

          cairo_rectangle (cr, 0, 0, alloc.width, alloc.height);
          cairo_set_source_rgba (cr, 1, 0, 0, 0.2);
          cairo_fill (cr);

          ctk_widget_queue_draw (widget);

          widget->priv->highlight_resize = FALSE;
        }
#endif

      if (push_group)
        {
          cairo_pop_group_to_source (cr);
          cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
          cairo_paint_with_alpha (cr, widget->priv->alpha / 255.0);
        }

      if (cairo_status (cr) &&
          event_window != NULL)
        {
          /* We check the event so we only warn about internal CTK+ calls.
           * Errors might come from PDF streams having write failures and
           * we don't want to spam stderr in that case.
           * We do want to catch errors from
           */
          g_warning ("drawing failure for widget '%s': %s",
                     G_OBJECT_TYPE_NAME (widget),
                     cairo_status_to_string (cairo_status (cr)));
        }
    }
}

/**
 * ctk_widget_draw:
 * @widget: the widget to draw. It must be drawable (see
 *   ctk_widget_is_drawable()) and a size must have been allocated.
 * @cr: a cairo context to draw to
 *
 * Draws @widget to @cr. The top left corner of the widget will be
 * drawn to the currently set origin point of @cr.
 *
 * You should pass a cairo context as @cr argument that is in an
 * original state. Otherwise the resulting drawing is undefined. For
 * example changing the operator using cairo_set_operator() or the
 * line width using cairo_set_line_width() might have unwanted side
 * effects.
 * You may however change the context’s transform matrix - like with
 * cairo_scale(), cairo_translate() or cairo_set_matrix() and clip
 * region with cairo_clip() prior to calling this function. Also, it
 * is fine to modify the context with cairo_save() and
 * cairo_push_group() prior to calling this function.
 *
 * Note that special-purpose widgets may contain special code for
 * rendering to the screen and might appear differently on screen
 * and when rendered using ctk_widget_draw().
 *
 * Since: 3.0
 **/
void
ctk_widget_draw (CtkWidget *widget,
                 cairo_t   *cr)
{
  gboolean was_marked;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (!widget->priv->alloc_needed);
  g_return_if_fail (!widget->priv->alloc_needed_on_child);
  g_return_if_fail (cr != NULL);

  cairo_save (cr);

  was_marked = ctk_cairo_is_marked_for_draw (cr);

  /* We mark the window so that ctk_cairo_should_draw_window()
   * will always return TRUE, and all CdkWindows get drawn
   */
  ctk_cairo_set_marked_for_draw (cr, TRUE);

  ctk_widget_draw_internal (widget, cr, TRUE);

  ctk_cairo_set_marked_for_draw (cr, was_marked);

  cairo_restore (cr);
}

static gboolean
ctk_widget_real_scroll_event (CtkWidget      *widget,
                              CdkEventScroll *event)
{
  return _ctk_widget_run_controllers (widget, (CdkEvent *) event,
                                      CTK_PHASE_BUBBLE);
}

static gboolean
ctk_widget_real_button_event (CtkWidget      *widget,
                              CdkEventButton *event)
{
  return _ctk_widget_run_controllers (widget, (CdkEvent *) event,
                                      CTK_PHASE_BUBBLE);
}

static gboolean
ctk_widget_real_motion_event (CtkWidget      *widget,
                              CdkEventMotion *event)
{
  return _ctk_widget_run_controllers (widget, (CdkEvent *) event,
                                      CTK_PHASE_BUBBLE);
}

static gboolean
ctk_widget_real_key_press_event (CtkWidget         *widget,
				 CdkEventKey       *event)
{
  if (_ctk_widget_run_controllers (widget, (CdkEvent *) event,
                                   CTK_PHASE_BUBBLE))
    return CDK_EVENT_STOP;

  return ctk_bindings_activate_event (G_OBJECT (widget), event);
}

static gboolean
ctk_widget_real_key_release_event (CtkWidget         *widget,
				   CdkEventKey       *event)
{
  if (_ctk_widget_run_controllers (widget, (CdkEvent *) event,
                                   CTK_PHASE_BUBBLE))
    return CDK_EVENT_STOP;

  return ctk_bindings_activate_event (G_OBJECT (widget), event);
}

static gboolean
ctk_widget_real_focus_in_event (CtkWidget     *widget,
                                CdkEventFocus *event)
{
  ctk_widget_queue_draw (widget);

  return FALSE;
}

static gboolean
ctk_widget_real_focus_out_event (CtkWidget     *widget,
                                 CdkEventFocus *event)
{
  ctk_widget_queue_draw (widget);

  return FALSE;
}

static gboolean
ctk_widget_real_touch_event (CtkWidget     *widget,
                             CdkEventTouch *event)
{
  CdkEvent *bevent;
  gboolean return_val = FALSE;

  if (!event->emulating_pointer)
    return _ctk_widget_run_controllers (widget, (CdkEvent*) event,
                                        CTK_PHASE_BUBBLE);

  if (event->type == CDK_TOUCH_UPDATE ||
      event->type == CDK_TOUCH_BEGIN)
    {
      bevent = cdk_event_new (CDK_MOTION_NOTIFY);
      bevent->any.window = g_object_ref (event->window);
      bevent->any.send_event = FALSE;
      bevent->motion.time = event->time;
      bevent->button.state = event->state;
      bevent->motion.x_root = event->x_root;
      bevent->motion.y_root = event->y_root;
      bevent->motion.x = event->x;
      bevent->motion.y = event->y;
      bevent->motion.device = event->device;
      bevent->motion.is_hint = FALSE;
      bevent->motion.axes = g_memdup (event->axes,
                                      sizeof (gdouble) * cdk_device_get_n_axes (event->device));
      cdk_event_set_source_device (bevent, cdk_event_get_source_device ((CdkEvent*)event));

      if (event->type == CDK_TOUCH_UPDATE)
        bevent->motion.state |= CDK_BUTTON1_MASK;

      g_signal_emit (widget, widget_signals[MOTION_NOTIFY_EVENT], 0, bevent, &return_val);

      cdk_event_free (bevent);
    }

  if (event->type == CDK_TOUCH_BEGIN ||
      event->type == CDK_TOUCH_END)
    {
      CdkEventType type;
      gint signum;

      if (event->type == CDK_TOUCH_BEGIN)
        {
          type = CDK_BUTTON_PRESS;
          signum = BUTTON_PRESS_EVENT;
        }
      else
        {
          type = CDK_BUTTON_RELEASE;
          signum = BUTTON_RELEASE_EVENT;
        }
      bevent = cdk_event_new (type);
      bevent->any.window = g_object_ref (event->window);
      bevent->any.send_event = FALSE;
      bevent->button.time = event->time;
      bevent->button.state = event->state;
      bevent->button.button = 1;
      bevent->button.x_root = event->x_root;
      bevent->button.y_root = event->y_root;
      bevent->button.x = event->x;
      bevent->button.y = event->y;
      bevent->button.device = event->device;
      bevent->button.axes = g_memdup (event->axes,
                                      sizeof (gdouble) * cdk_device_get_n_axes (event->device));
      cdk_event_set_source_device (bevent, cdk_event_get_source_device ((CdkEvent*)event));

      if (event->type == CDK_TOUCH_END)
        bevent->button.state |= CDK_BUTTON1_MASK;

      g_signal_emit (widget, widget_signals[signum], 0, bevent, &return_val);

      cdk_event_free (bevent);
    }

  return return_val;
}

static gboolean
ctk_widget_real_grab_broken_event (CtkWidget          *widget,
                                   CdkEventGrabBroken *event)
{
  return _ctk_widget_run_controllers (widget, (CdkEvent*) event,
                                      CTK_PHASE_BUBBLE);
}

#define WIDGET_REALIZED_FOR_EVENT(widget, event) \
     (event->type == CDK_FOCUS_CHANGE || _ctk_widget_get_realized(widget))

/**
 * ctk_widget_event:
 * @widget: a #CtkWidget
 * @event: a #CdkEvent
 *
 * Rarely-used function. This function is used to emit
 * the event signals on a widget (those signals should never
 * be emitted without using this function to do so).
 * If you want to synthesize an event though, don’t use this function;
 * instead, use ctk_main_do_event() so the event will behave as if
 * it were in the event queue. Don’t synthesize expose events; instead,
 * use cdk_window_invalidate_rect() to invalidate a region of the
 * window.
 *
 * Returns: return from the event signal emission (%TRUE if
 *               the event was handled)
 **/
gboolean
ctk_widget_event (CtkWidget *widget,
		  CdkEvent  *event)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), TRUE);
  g_return_val_if_fail (WIDGET_REALIZED_FOR_EVENT (widget, event), TRUE);

  if (event->type == CDK_EXPOSE)
    {
      g_warning ("Events of type CDK_EXPOSE cannot be synthesized. To get "
		 "the same effect, call cdk_window_invalidate_rect/region(), "
		 "followed by cdk_window_process_updates().");
      return TRUE;
    }

  return ctk_widget_event_internal (widget, event);
}

void
_ctk_widget_set_captured_event_handler (CtkWidget               *widget,
                                        CtkCapturedEventHandler  callback)
{
  g_object_set_data (G_OBJECT (widget), "captured-event-handler", callback);
}

static CdkEventMask
_ctk_widget_get_controllers_evmask (CtkWidget *widget)
{
  EventControllerData *data;
  CdkEventMask evmask = 0;
  CtkWidgetPrivate *priv;
  GList *l;

  priv = widget->priv;

  for (l = priv->event_controllers; l; l = l->next)
    {
      data = l->data;
      if (data->controller)
        evmask |= ctk_event_controller_get_event_mask (CTK_EVENT_CONTROLLER (data->controller));
    }

  return evmask;
}

static gboolean
_ctk_widget_run_controllers (CtkWidget           *widget,
                             const CdkEvent      *event,
                             CtkPropagationPhase  phase)
{
  EventControllerData *data;
  gboolean handled = FALSE;
  CtkWidgetPrivate *priv;
  GList *l;

  priv = widget->priv;
  g_object_ref (widget);

  l = priv->event_controllers;
  while (l != NULL)
    {
      GList *next = l->next;

      if (!WIDGET_REALIZED_FOR_EVENT (widget, event))
        break;

      data = l->data;

      if (data->controller == NULL)
        {
          priv->event_controllers = g_list_delete_link (priv->event_controllers, l);
          g_free (data);
        }
      else
        {
          CtkPropagationPhase controller_phase;

          controller_phase = ctk_event_controller_get_propagation_phase (data->controller);

          if (controller_phase == phase)
            handled |= ctk_event_controller_handle_event (data->controller, event);
        }

      l = next;
    }

  g_object_unref (widget);

  return handled;
}

static void
cancel_event_sequence_on_hierarchy (CtkWidget        *widget,
                                    CtkWidget        *event_widget,
                                    CdkEventSequence *sequence)
{
  gboolean cancel = TRUE;

  while (event_widget)
    {
      if (event_widget == widget)
        cancel = FALSE;
      else if (cancel)
        _ctk_widget_cancel_sequence (event_widget, sequence);
      else
        _ctk_widget_set_sequence_state_internal (event_widget, sequence,
                                                 CTK_EVENT_SEQUENCE_DENIED,
                                                 NULL);

      event_widget = _ctk_widget_get_parent (event_widget);
    }
}

gboolean
_ctk_widget_captured_event (CtkWidget *widget,
                            CdkEvent  *event)
{
  gboolean return_val = FALSE;
  CtkCapturedEventHandler handler;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), TRUE);
  g_return_val_if_fail (WIDGET_REALIZED_FOR_EVENT (widget, event), TRUE);

  if (event->type == CDK_EXPOSE)
    {
      g_warning ("Events of type CDK_EXPOSE cannot be synthesized. To get "
		 "the same effect, call cdk_window_invalidate_rect/region(), "
		 "followed by cdk_window_process_updates().");
      return TRUE;
    }

  if (!event_window_is_still_viewable (event))
    return TRUE;

  return_val = _ctk_widget_run_controllers (widget, event, CTK_PHASE_CAPTURE);

  handler = g_object_get_data (G_OBJECT (widget), "captured-event-handler");
  if (!handler)
    return return_val;

  g_object_ref (widget);

  return_val |= handler (widget, event);
  return_val |= !WIDGET_REALIZED_FOR_EVENT (widget, event);

  /* The widget that was originally to receive the event
   * handles motion hints, but the capturing widget might
   * not, so ensure we get further motion events.
   */
  if (return_val &&
      event->type == CDK_MOTION_NOTIFY &&
      event->motion.is_hint &&
      (cdk_window_get_events (event->any.window) &
       CDK_POINTER_MOTION_HINT_MASK) != 0)
    cdk_event_request_motions (&event->motion);

  g_object_unref (widget);

  return return_val;
}

/* Returns TRUE if a translation should be done */
static gboolean
_ctk_widget_get_translation_to_window (CtkWidget      *widget,
				       CdkWindow      *window,
				       int            *x,
				       int            *y)
{
  CdkWindow *w, *widget_window;

  if (!_ctk_widget_get_has_window (widget))
    {
      *x = -widget->priv->allocation.x;
      *y = -widget->priv->allocation.y;
    }
  else
    {
      *x = 0;
      *y = 0;
    }

  widget_window = _ctk_widget_get_window (widget);

  for (w = window; w && w != widget_window; w = cdk_window_get_parent (w))
    {
      int wx, wy;
      cdk_window_get_position (w, &wx, &wy);
      *x += wx;
      *y += wy;
    }

  if (w == NULL)
    {
      *x = 0;
      *y = 0;
      return FALSE;
    }

  return TRUE;
}


/**
 * ctk_cairo_transform_to_window:
 * @cr: the cairo context to transform
 * @widget: the widget the context is currently centered for
 * @window: the window to transform the context to
 *
 * Transforms the given cairo context @cr that from @widget-relative
 * coordinates to @window-relative coordinates.
 * If the @widget’s window is not an ancestor of @window, no
 * modification will be applied.
 *
 * This is the inverse to the transformation CTK applies when
 * preparing an expose event to be emitted with the #CtkWidget::draw
 * signal. It is intended to help porting multiwindow widgets from
 * CTK+ 2 to the rendering architecture of CTK+ 3.
 *
 * Since: 3.0
 **/
void
ctk_cairo_transform_to_window (cairo_t   *cr,
                               CtkWidget *widget,
                               CdkWindow *window)
{
  int x, y;

  g_return_if_fail (cr != NULL);
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CDK_IS_WINDOW (window));

  if (_ctk_widget_get_translation_to_window (widget, window, &x, &y))
    cairo_translate (cr, x, y);
}

/**
 * ctk_widget_send_expose:
 * @widget: a #CtkWidget
 * @event: a expose #CdkEvent
 *
 * Very rarely-used function. This function is used to emit
 * an expose event on a widget. This function is not normally used
 * directly. The only time it is used is when propagating an expose
 * event to a windowless child widget (ctk_widget_get_has_window() is %FALSE),
 * and that is normally done using ctk_container_propagate_draw().
 *
 * If you want to force an area of a window to be redrawn,
 * use cdk_window_invalidate_rect() or cdk_window_invalidate_region().
 * To cause the redraw to be done immediately, follow that call
 * with a call to cdk_window_process_updates().
 *
 * Returns: return from the event signal emission (%TRUE if
 *   the event was handled)
 *
 * Deprecated: 3.22: Application and widget code should not handle
 *   expose events directly; invalidation should use the #CtkWidget
 *   API, and drawing should only happen inside #CtkWidget::draw
 *   implementations
 */
gint
ctk_widget_send_expose (CtkWidget *widget,
			CdkEvent  *event)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), TRUE);
  g_return_val_if_fail (ctk_widget_get_realized (widget), TRUE);
  g_return_val_if_fail (event != NULL, TRUE);
  g_return_val_if_fail (event->type == CDK_EXPOSE, TRUE);

  ctk_widget_render (widget, event->any.window, event->expose.region);

  return FALSE;
}

static gboolean
event_window_is_still_viewable (CdkEvent *event)
{
  /* Check that we think the event's window is viewable before
   * delivering the event, to prevent surprises. We do this here
   * at the last moment, since the event may have been queued
   * up behind other events, held over a recursive main loop, etc.
   */
  switch (event->type)
    {
    case CDK_EXPOSE:
    case CDK_MOTION_NOTIFY:
    case CDK_BUTTON_PRESS:
    case CDK_2BUTTON_PRESS:
    case CDK_3BUTTON_PRESS:
    case CDK_KEY_PRESS:
    case CDK_ENTER_NOTIFY:
    case CDK_PROXIMITY_IN:
    case CDK_SCROLL:
      return event->any.window && cdk_window_is_viewable (event->any.window);

#if 0
    /* The following events are the second half of paired events;
     * we always deliver them to deal with widgets that clean up
     * on the second half.
     */
    case CDK_BUTTON_RELEASE:
    case CDK_KEY_RELEASE:
    case CDK_LEAVE_NOTIFY:
    case CDK_PROXIMITY_OUT:
#endif

    default:
      /* Remaining events would make sense on an not-viewable window,
       * or don't have an associated window.
       */
      return TRUE;
    }
}

static gint
ctk_widget_event_internal (CtkWidget *widget,
			   CdkEvent  *event)
{
  gboolean return_val = FALSE, handled;

  /* We check only once for is-still-visible; if someone
   * hides the window in on of the signals on the widget,
   * they are responsible for returning TRUE to terminate
   * handling.
   */
  if (!event_window_is_still_viewable (event))
    return TRUE;

  g_object_ref (widget);

  if (widget == ctk_get_event_widget (event))
    return_val |= _ctk_widget_run_controllers (widget, event, CTK_PHASE_TARGET);

  g_signal_emit (widget, widget_signals[EVENT], 0, event, &handled);
  return_val |= handled | !WIDGET_REALIZED_FOR_EVENT (widget, event);
  if (!return_val)
    {
      gint signal_num;

      switch (event->type)
	{
        case CDK_TOUCHPAD_SWIPE:
        case CDK_TOUCHPAD_PINCH:
          return_val |= _ctk_widget_run_controllers (widget, event, CTK_PHASE_BUBBLE);
          /* Fall through */
        case CDK_PAD_BUTTON_PRESS:
        case CDK_PAD_BUTTON_RELEASE:
        case CDK_PAD_RING:
        case CDK_PAD_STRIP:
        case CDK_PAD_GROUP_MODE:
	case CDK_EXPOSE:
	case CDK_NOTHING:
	  signal_num = -1;
	  break;
	case CDK_BUTTON_PRESS:
	case CDK_2BUTTON_PRESS:
	case CDK_3BUTTON_PRESS:
	  signal_num = BUTTON_PRESS_EVENT;
          break;
        case CDK_TOUCH_BEGIN:
        case CDK_TOUCH_UPDATE:
        case CDK_TOUCH_END:
        case CDK_TOUCH_CANCEL:
	  signal_num = TOUCH_EVENT;
	  break;
	case CDK_SCROLL:
	  signal_num = SCROLL_EVENT;
	  break;
	case CDK_BUTTON_RELEASE:
	  signal_num = BUTTON_RELEASE_EVENT;
	  break;
	case CDK_MOTION_NOTIFY:
	  signal_num = MOTION_NOTIFY_EVENT;
	  break;
	case CDK_DELETE:
	  signal_num = DELETE_EVENT;
	  break;
	case CDK_DESTROY:
	  signal_num = DESTROY_EVENT;
	  _ctk_tooltip_hide (widget);
	  break;
	case CDK_KEY_PRESS:
	  signal_num = KEY_PRESS_EVENT;
	  break;
	case CDK_KEY_RELEASE:
	  signal_num = KEY_RELEASE_EVENT;
	  break;
	case CDK_ENTER_NOTIFY:
	  signal_num = ENTER_NOTIFY_EVENT;
	  break;
	case CDK_LEAVE_NOTIFY:
	  signal_num = LEAVE_NOTIFY_EVENT;
	  break;
	case CDK_FOCUS_CHANGE:
	  signal_num = event->focus_change.in ? FOCUS_IN_EVENT : FOCUS_OUT_EVENT;
	  if (event->focus_change.in)
	    _ctk_tooltip_focus_in (widget);
	  else
	    _ctk_tooltip_focus_out (widget);
	  break;
	case CDK_CONFIGURE:
	  signal_num = CONFIGURE_EVENT;
	  break;
	case CDK_MAP:
	  signal_num = MAP_EVENT;
	  break;
	case CDK_UNMAP:
	  signal_num = UNMAP_EVENT;
	  break;
	case CDK_WINDOW_STATE:
	  signal_num = WINDOW_STATE_EVENT;
	  break;
	case CDK_PROPERTY_NOTIFY:
	  signal_num = PROPERTY_NOTIFY_EVENT;
	  break;
	case CDK_SELECTION_CLEAR:
	  signal_num = SELECTION_CLEAR_EVENT;
	  break;
	case CDK_SELECTION_REQUEST:
	  signal_num = SELECTION_REQUEST_EVENT;
	  break;
	case CDK_SELECTION_NOTIFY:
	  signal_num = SELECTION_NOTIFY_EVENT;
	  break;
	case CDK_PROXIMITY_IN:
	  signal_num = PROXIMITY_IN_EVENT;
	  break;
	case CDK_PROXIMITY_OUT:
	  signal_num = PROXIMITY_OUT_EVENT;
	  break;
	case CDK_VISIBILITY_NOTIFY:
	  signal_num = VISIBILITY_NOTIFY_EVENT;
	  break;
	case CDK_GRAB_BROKEN:
	  signal_num = GRAB_BROKEN_EVENT;
	  break;
	case CDK_DAMAGE:
	  signal_num = DAMAGE_EVENT;
	  break;
	default:
	  g_warning ("ctk_widget_event(): unhandled event type: %d", event->type);
	  signal_num = -1;
	  break;
	}
      if (signal_num != -1)
        {
	  g_signal_emit (widget, widget_signals[signal_num], 0, event, &handled);
          return_val |= handled;
        }
    }
  if (WIDGET_REALIZED_FOR_EVENT (widget, event))
    g_signal_emit (widget, widget_signals[EVENT_AFTER], 0, event);
  else
    return_val = TRUE;

  g_object_unref (widget);

  return return_val;
}

/**
 * ctk_widget_activate:
 * @widget: a #CtkWidget that’s activatable
 *
 * For widgets that can be “activated” (buttons, menu items, etc.)
 * this function activates them. Activation is what happens when you
 * press Enter on a widget during key navigation. If @widget isn't
 * activatable, the function returns %FALSE.
 *
 * Returns: %TRUE if the widget was activatable
 **/
gboolean
ctk_widget_activate (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  if (WIDGET_CLASS (widget)->activate_signal)
    {
      /* FIXME: we should eventually check the signals signature here */
      g_signal_emit (widget, WIDGET_CLASS (widget)->activate_signal, 0);

      return TRUE;
    }
  else
    return FALSE;
}

static void
ctk_widget_reparent_subwindows (CtkWidget *widget,
				CdkWindow *new_window)
{
  CtkWidgetPrivate *priv = widget->priv;

  if (!_ctk_widget_get_has_window (widget))
    {
      GList *children = cdk_window_get_children (priv->window);
      GList *tmp_list;

      for (tmp_list = children; tmp_list; tmp_list = tmp_list->next)
	{
	  CdkWindow *window = tmp_list->data;
	  gpointer child;

	  cdk_window_get_user_data (window, &child);
	  while (child && child != widget)
	    child = ((CtkWidget*) child)->priv->parent;

	  if (child)
	    cdk_window_reparent (window, new_window, 0, 0);
	}

      g_list_free (children);
    }
  else
   {
     CdkWindow *parent;
     GList *tmp_list, *children;

     parent = cdk_window_get_parent (priv->window);

     if (parent == NULL)
       cdk_window_reparent (priv->window, new_window, 0, 0);
     else
       {
	 children = cdk_window_get_children (parent);

	 for (tmp_list = children; tmp_list; tmp_list = tmp_list->next)
	   {
	     CdkWindow *window = tmp_list->data;
	     gpointer child;

	     cdk_window_get_user_data (window, &child);

	     if (child == widget)
	       cdk_window_reparent (window, new_window, 0, 0);
	   }

	 g_list_free (children);
       }
   }
}

static void
ctk_widget_reparent_fixup_child (CtkWidget *widget,
				 gpointer   client_data)
{
  CtkWidgetPrivate *priv = widget->priv;

  g_assert (client_data != NULL);

  if (!_ctk_widget_get_has_window (widget))
    {
      if (priv->window)
	g_object_unref (priv->window);
      priv->window = (CdkWindow*) client_data;
      if (priv->window)
	g_object_ref (priv->window);

      if (CTK_IS_CONTAINER (widget))
        ctk_container_forall (CTK_CONTAINER (widget),
                              ctk_widget_reparent_fixup_child,
                              client_data);
    }
}

/**
 * ctk_widget_reparent:
 * @widget: a #CtkWidget
 * @new_parent: a #CtkContainer to move the widget into
 *
 * Moves a widget from one #CtkContainer to another, handling reference
 * count issues to avoid destroying the widget.
 *
 * Deprecated: 3.14: Use ctk_container_remove() and ctk_container_add().
 **/
void
ctk_widget_reparent (CtkWidget *widget,
		     CtkWidget *new_parent)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CTK_IS_CONTAINER (new_parent));
  priv = widget->priv;
  g_return_if_fail (priv->parent != NULL);

  if (priv->parent != new_parent)
    {
      /* First try to see if we can get away without unrealizing
       * the widget as we reparent it. if so we set a flag so
       * that ctk_widget_unparent doesn't unrealize widget
       */
      if (_ctk_widget_get_realized (widget) && _ctk_widget_get_realized (new_parent))
	priv->in_reparent = TRUE;

      g_object_ref (widget);
      ctk_container_remove (CTK_CONTAINER (priv->parent), widget);
      ctk_container_add (CTK_CONTAINER (new_parent), widget);
      g_object_unref (widget);

      if (priv->in_reparent)
	{
          priv->in_reparent = FALSE;

	  ctk_widget_reparent_subwindows (widget, ctk_widget_get_parent_window (widget));
	  ctk_widget_reparent_fixup_child (widget,
					   ctk_widget_get_parent_window (widget));
	}

      g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_PARENT]);
    }
}

/**
 * ctk_widget_intersect:
 * @widget: a #CtkWidget
 * @area: a rectangle
 * @intersection: (out caller-allocates) (optional): rectangle to store
 *   intersection of @widget and @area
 *
 * Computes the intersection of a @widget’s area and @area, storing
 * the intersection in @intersection, and returns %TRUE if there was
 * an intersection.  @intersection may be %NULL if you’re only
 * interested in whether there was an intersection.
 *
 * Returns: %TRUE if there was an intersection
 **/
gboolean
ctk_widget_intersect (CtkWidget	         *widget,
		      const CdkRectangle *area,
		      CdkRectangle       *intersection)
{
  CtkWidgetPrivate *priv;
  CdkRectangle *dest;
  CdkRectangle tmp;
  gint return_val;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (area != NULL, FALSE);

  priv = widget->priv;

  if (intersection)
    dest = intersection;
  else
    dest = &tmp;

  return_val = cdk_rectangle_intersect (&priv->allocation, area, dest);

  if (return_val && intersection && _ctk_widget_get_has_window (widget))
    {
      intersection->x -= priv->allocation.x;
      intersection->y -= priv->allocation.y;
    }

  return return_val;
}

/**
 * ctk_widget_region_intersect:
 * @widget: a #CtkWidget
 * @region: a #cairo_region_t, in the same coordinate system as
 *          @widget->allocation. That is, relative to @widget->window
 *          for widgets which return %FALSE from ctk_widget_get_has_window();
 *          relative to the parent window of @widget->window otherwise.
 *
 * Computes the intersection of a @widget’s area and @region, returning
 * the intersection. The result may be empty, use cairo_region_is_empty() to
 * check.
 *
 * Returns: A newly allocated region holding the intersection of @widget
 *     and @region.
 *
 * Deprecated: 3.14: Use ctk_widget_get_allocation() and
 *     cairo_region_intersect_rectangle() to get the same behavior.
 */
cairo_region_t *
ctk_widget_region_intersect (CtkWidget            *widget,
			     const cairo_region_t *region)
{
  CdkRectangle rect;
  cairo_region_t *dest;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (region != NULL, NULL);

  _ctk_widget_get_allocation (widget, &rect);

  dest = cairo_region_create_rectangle (&rect);

  cairo_region_intersect (dest, region);

  return dest;
}

/**
 * _ctk_widget_grab_notify:
 * @widget: a #CtkWidget
 * @was_grabbed: whether a grab is now in effect
 *
 * Emits the #CtkWidget::grab-notify signal on @widget.
 *
 * Since: 2.6
 **/
void
_ctk_widget_grab_notify (CtkWidget *widget,
			 gboolean   was_grabbed)
{
  g_signal_emit (widget, widget_signals[GRAB_NOTIFY], 0, was_grabbed);
}

/**
 * ctk_widget_grab_focus:
 * @widget: a #CtkWidget
 *
 * Causes @widget to have the keyboard focus for the #CtkWindow it's
 * inside. @widget must be a focusable widget, such as a #CtkEntry;
 * something like #CtkFrame won’t work.
 *
 * More precisely, it must have the %CTK_CAN_FOCUS flag set. Use
 * ctk_widget_set_can_focus() to modify that flag.
 *
 * The widget also needs to be realized and mapped. This is indicated by the
 * related signals. Grabbing the focus immediately after creating the widget
 * will likely fail and cause critical warnings.
 **/
void
ctk_widget_grab_focus (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (!ctk_widget_is_sensitive (widget))
    return;

  g_object_ref (widget);
  g_signal_emit (widget, widget_signals[GRAB_FOCUS], 0);
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_HAS_FOCUS]);
  g_object_unref (widget);
}

static void
reset_focus_recurse (CtkWidget *widget,
		     gpointer   data)
{
  if (CTK_IS_CONTAINER (widget))
    {
      CtkContainer *container;

      container = CTK_CONTAINER (widget);
      ctk_container_set_focus_child (container, NULL);

      ctk_container_foreach (container,
			     reset_focus_recurse,
			     NULL);
    }
}

static void
ctk_widget_real_grab_focus (CtkWidget *focus_widget)
{
  if (ctk_widget_get_can_focus (focus_widget))
    {
      CtkWidget *toplevel;
      CtkWidget *widget;

      /* clear the current focus setting, break if the current widget
       * is the focus widget's parent, since containers above that will
       * be set by the next loop.
       */
      toplevel = _ctk_widget_get_toplevel (focus_widget);
      if (_ctk_widget_is_toplevel (toplevel) && CTK_IS_WINDOW (toplevel))
	{
          widget = ctk_window_get_focus (CTK_WINDOW (toplevel));

	  if (widget == focus_widget)
	    {
	      /* We call _ctk_window_internal_set_focus() here so that the
	       * toplevel window can request the focus if necessary.
	       * This is needed when the toplevel is a CtkPlug
	       */
	      if (!ctk_widget_has_focus (widget))
		_ctk_window_internal_set_focus (CTK_WINDOW (toplevel), focus_widget);

	      return;
	    }

	  if (widget)
	    {
	      CtkWidget *common_ancestor = ctk_widget_common_ancestor (widget, focus_widget);

	      if (widget != common_ancestor)
		{
		  while (widget->priv->parent)
		    {
		      widget = widget->priv->parent;
		      ctk_container_set_focus_child (CTK_CONTAINER (widget), NULL);
		      if (widget == common_ancestor)
		        break;
		    }
		}
	    }
	}
      else if (toplevel != focus_widget)
	{
	  /* ctk_widget_grab_focus() operates on a tree without window...
	   * actually, this is very questionable behavior.
	   */

	  ctk_container_foreach (CTK_CONTAINER (toplevel),
				 reset_focus_recurse,
				 NULL);
	}

      /* now propagate the new focus up the widget tree and finally
       * set it on the window
       */
      widget = focus_widget;
      while (widget->priv->parent)
	{
	  ctk_container_set_focus_child (CTK_CONTAINER (widget->priv->parent), widget);
	  widget = widget->priv->parent;
	}
      if (CTK_IS_WINDOW (widget))
	_ctk_window_internal_set_focus (CTK_WINDOW (widget), focus_widget);
    }
}

static gboolean
ctk_widget_real_query_tooltip (CtkWidget  *widget,
			       gint        x,
			       gint        y,
			       gboolean    keyboard_tip,
			       CtkTooltip *tooltip)
{
  gchar *tooltip_markup;
  gboolean has_tooltip;

  tooltip_markup = g_object_get_qdata (G_OBJECT (widget), quark_tooltip_markup);
  has_tooltip = ctk_widget_get_has_tooltip (widget);

  if (has_tooltip && tooltip_markup)
    {
      ctk_tooltip_set_markup (tooltip, tooltip_markup);
      return TRUE;
    }

  return FALSE;
}

gboolean
ctk_widget_query_tooltip (CtkWidget  *widget,
                          gint        x,
                          gint        y,
                          gboolean    keyboard_mode,
                          CtkTooltip *tooltip)
{
  gboolean retval = FALSE;

  g_signal_emit (widget,
                 widget_signals[QUERY_TOOLTIP],
                 0,
                 x, y,
                 keyboard_mode,
                 tooltip,
                 &retval);

  return retval;
}

static void
ctk_widget_real_state_flags_changed (CtkWidget     *widget,
                                     CtkStateFlags  old_state)
{
}

static void
ctk_widget_real_style_updated (CtkWidget *widget)
{
  CtkWidgetPrivate *priv = widget->priv;

  ctk_widget_update_alpha (widget);

  if (widget->priv->context)
    {
      CtkCssStyleChange *change = ctk_style_context_get_change (widget->priv->context);
      gboolean has_text = ctk_widget_peek_pango_context (widget) != NULL;

      if (change == NULL ||
          (has_text && ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_FONT)))
        ctk_widget_update_pango_context (widget);

      if (widget->priv->anchored)
        {
          if (change == NULL ||
              ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_SIZE) ||
              (has_text && ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_TEXT)))
            ctk_widget_queue_resize (widget);
          else if (ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_CLIP))
            ctk_widget_queue_allocate (widget);
          else if (ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_REDRAW))
            ctk_widget_queue_draw (widget);
        }
    }
  else
    {
      ctk_widget_update_pango_context (widget);

      if (widget->priv->anchored)
        ctk_widget_queue_resize (widget);
    }

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  if (priv->style != NULL &&
      priv->style != ctk_widget_get_default_style ())
    {
      /* Trigger ::style-set for old
       * widgets not listening to this
       */
      g_signal_emit (widget,
                     widget_signals[STYLE_SET],
                     0,
                     widget->priv->style);
    }
  G_GNUC_END_IGNORE_DEPRECATIONS;

}

static gboolean
ctk_widget_real_show_help (CtkWidget        *widget,
                           CtkWidgetHelpType help_type)
{
  if (help_type == CTK_WIDGET_HELP_TOOLTIP)
    {
      _ctk_tooltip_toggle_keyboard_mode (widget);

      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
ctk_widget_real_focus (CtkWidget         *widget,
                       CtkDirectionType   direction)
{
  if (!ctk_widget_get_can_focus (widget))
    return FALSE;

  if (!ctk_widget_is_focus (widget))
    {
      ctk_widget_grab_focus (widget);
      return TRUE;
    }
  else
    return FALSE;
}

static void
ctk_widget_real_move_focus (CtkWidget         *widget,
                            CtkDirectionType   direction)
{
  CtkWidget *toplevel = _ctk_widget_get_toplevel (widget);

  if (widget != toplevel && CTK_IS_WINDOW (toplevel))
    {
      g_signal_emit (toplevel, widget_signals[MOVE_FOCUS], 0,
                     direction);
    }
}

static gboolean
ctk_widget_real_keynav_failed (CtkWidget        *widget,
                               CtkDirectionType  direction)
{
  switch (direction)
    {
    case CTK_DIR_TAB_FORWARD:
    case CTK_DIR_TAB_BACKWARD:
      return FALSE;

    case CTK_DIR_UP:
    case CTK_DIR_DOWN:
    case CTK_DIR_LEFT:
    case CTK_DIR_RIGHT:
      break;
    }

  ctk_widget_error_bell (widget);

  return TRUE;
}

/**
 * ctk_widget_set_can_focus:
 * @widget: a #CtkWidget
 * @can_focus: whether or not @widget can own the input focus.
 *
 * Specifies whether @widget can own the input focus. See
 * ctk_widget_grab_focus() for actually setting the input focus on a
 * widget.
 *
 * Since: 2.18
 **/
void
ctk_widget_set_can_focus (CtkWidget *widget,
                          gboolean   can_focus)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (widget->priv->can_focus != can_focus)
    {
      widget->priv->can_focus = can_focus;

      ctk_widget_queue_resize (widget);
      g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_CAN_FOCUS]);
    }
}

/**
 * ctk_widget_get_can_focus:
 * @widget: a #CtkWidget
 *
 * Determines whether @widget can own the input focus. See
 * ctk_widget_set_can_focus().
 *
 * Returns: %TRUE if @widget can own the input focus, %FALSE otherwise
 *
 * Since: 2.18
 **/
gboolean
ctk_widget_get_can_focus (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->can_focus;
}

/**
 * ctk_widget_has_focus:
 * @widget: a #CtkWidget
 *
 * Determines if the widget has the global input focus. See
 * ctk_widget_is_focus() for the difference between having the global
 * input focus, and only having the focus within a toplevel.
 *
 * Returns: %TRUE if the widget has the global input focus.
 *
 * Since: 2.18
 **/
gboolean
ctk_widget_has_focus (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->has_focus;
}

/**
 * ctk_widget_has_visible_focus:
 * @widget: a #CtkWidget
 *
 * Determines if the widget should show a visible indication that
 * it has the global input focus. This is a convenience function for
 * use in ::draw handlers that takes into account whether focus
 * indication should currently be shown in the toplevel window of
 * @widget. See ctk_window_get_focus_visible() for more information
 * about focus indication.
 *
 * To find out if the widget has the global input focus, use
 * ctk_widget_has_focus().
 *
 * Returns: %TRUE if the widget should display a “focus rectangle”
 *
 * Since: 3.2
 */
gboolean
ctk_widget_has_visible_focus (CtkWidget *widget)
{
  gboolean draw_focus;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  if (widget->priv->has_focus)
    {
      CtkWidget *toplevel;

      toplevel = _ctk_widget_get_toplevel (widget);

      if (CTK_IS_WINDOW (toplevel))
        draw_focus = ctk_window_get_focus_visible (CTK_WINDOW (toplevel));
      else
        draw_focus = TRUE;
    }
  else
    draw_focus = FALSE;

  return draw_focus;
}

/**
 * ctk_widget_is_focus:
 * @widget: a #CtkWidget
 *
 * Determines if the widget is the focus widget within its
 * toplevel. (This does not mean that the #CtkWidget:has-focus property is
 * necessarily set; #CtkWidget:has-focus will only be set if the
 * toplevel widget additionally has the global input focus.)
 *
 * Returns: %TRUE if the widget is the focus widget.
 **/
gboolean
ctk_widget_is_focus (CtkWidget *widget)
{
  CtkWidget *toplevel;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  toplevel = _ctk_widget_get_toplevel (widget);

  if (CTK_IS_WINDOW (toplevel))
    return widget == ctk_window_get_focus (CTK_WINDOW (toplevel));
  else
    return FALSE;
}

/**
 * ctk_widget_set_focus_on_click:
 * @widget: a #CtkWidget
 * @focus_on_click: whether the widget should grab focus when clicked with the mouse
 *
 * Sets whether the widget should grab focus when it is clicked with the mouse.
 * Making mouse clicks not grab focus is useful in places like toolbars where
 * you don’t want the keyboard focus removed from the main area of the
 * application.
 *
 * Since: 3.20
 **/
void
ctk_widget_set_focus_on_click (CtkWidget *widget,
			       gboolean   focus_on_click)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = widget->priv;

  focus_on_click = focus_on_click != FALSE;

  if (priv->focus_on_click != focus_on_click)
    {
      priv->focus_on_click = focus_on_click;

      g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_FOCUS_ON_CLICK]);
    }
}

/**
 * ctk_widget_get_focus_on_click:
 * @widget: a #CtkWidget
 *
 * Returns whether the widget should grab focus when it is clicked with the mouse.
 * See ctk_widget_set_focus_on_click().
 *
 * Returns: %TRUE if the widget should grab focus when it is clicked with
 *               the mouse.
 *
 * Since: 3.20
 **/
gboolean
ctk_widget_get_focus_on_click (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->focus_on_click;
}


/**
 * ctk_widget_set_can_default:
 * @widget: a #CtkWidget
 * @can_default: whether or not @widget can be a default widget.
 *
 * Specifies whether @widget can be a default widget. See
 * ctk_widget_grab_default() for details about the meaning of
 * “default”.
 *
 * Since: 2.18
 **/
void
ctk_widget_set_can_default (CtkWidget *widget,
                            gboolean   can_default)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (widget->priv->can_default != can_default)
    {
      widget->priv->can_default = can_default;

      ctk_widget_queue_resize (widget);
      g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_CAN_DEFAULT]);
    }
}

/**
 * ctk_widget_get_can_default:
 * @widget: a #CtkWidget
 *
 * Determines whether @widget can be a default widget. See
 * ctk_widget_set_can_default().
 *
 * Returns: %TRUE if @widget can be a default widget, %FALSE otherwise
 *
 * Since: 2.18
 **/
gboolean
ctk_widget_get_can_default (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->can_default;
}

/**
 * ctk_widget_has_default:
 * @widget: a #CtkWidget
 *
 * Determines whether @widget is the current default widget within its
 * toplevel. See ctk_widget_set_can_default().
 *
 * Returns: %TRUE if @widget is the current default widget within
 *     its toplevel, %FALSE otherwise
 *
 * Since: 2.18
 */
gboolean
ctk_widget_has_default (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->has_default;
}

void
_ctk_widget_set_has_default (CtkWidget *widget,
                             gboolean   has_default)
{
  CtkStyleContext *context;

  widget->priv->has_default = has_default;

  context = _ctk_widget_get_style_context (widget);

  if (has_default)
    ctk_style_context_add_class (context, CTK_STYLE_CLASS_DEFAULT);
  else
    ctk_style_context_remove_class (context, CTK_STYLE_CLASS_DEFAULT);
}

/**
 * ctk_widget_grab_default:
 * @widget: a #CtkWidget
 *
 * Causes @widget to become the default widget. @widget must be able to be
 * a default widget; typically you would ensure this yourself
 * by calling ctk_widget_set_can_default() with a %TRUE value.
 * The default widget is activated when
 * the user presses Enter in a window. Default widgets must be
 * activatable, that is, ctk_widget_activate() should affect them. Note
 * that #CtkEntry widgets require the “activates-default” property
 * set to %TRUE before they activate the default widget when Enter
 * is pressed and the #CtkEntry is focused.
 **/
void
ctk_widget_grab_default (CtkWidget *widget)
{
  CtkWidget *window;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (ctk_widget_get_can_default (widget));

  window = _ctk_widget_get_toplevel (widget);

  if (window && _ctk_widget_is_toplevel (window))
    ctk_window_set_default (CTK_WINDOW (window), widget);
  else
    g_warning (G_STRLOC ": widget not within a CtkWindow");
}

/**
 * ctk_widget_set_receives_default:
 * @widget: a #CtkWidget
 * @receives_default: whether or not @widget can be a default widget.
 *
 * Specifies whether @widget will be treated as the default widget
 * within its toplevel when it has the focus, even if another widget
 * is the default.
 *
 * See ctk_widget_grab_default() for details about the meaning of
 * “default”.
 *
 * Since: 2.18
 **/
void
ctk_widget_set_receives_default (CtkWidget *widget,
                                 gboolean   receives_default)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (widget->priv->receives_default != receives_default)
    {
      widget->priv->receives_default = receives_default;

      g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_RECEIVES_DEFAULT]);
    }
}

/**
 * ctk_widget_get_receives_default:
 * @widget: a #CtkWidget
 *
 * Determines whether @widget is always treated as the default widget
 * within its toplevel when it has the focus, even if another widget
 * is the default.
 *
 * See ctk_widget_set_receives_default().
 *
 * Returns: %TRUE if @widget acts as the default widget when focused,
 *               %FALSE otherwise
 *
 * Since: 2.18
 **/
gboolean
ctk_widget_get_receives_default (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->receives_default;
}

/**
 * ctk_widget_has_grab:
 * @widget: a #CtkWidget
 *
 * Determines whether the widget is currently grabbing events, so it
 * is the only widget receiving input events (keyboard and mouse).
 *
 * See also ctk_grab_add().
 *
 * Returns: %TRUE if the widget is in the grab_widgets stack
 *
 * Since: 2.18
 **/
gboolean
ctk_widget_has_grab (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->has_grab;
}

void
_ctk_widget_set_has_grab (CtkWidget *widget,
                          gboolean   has_grab)
{
  widget->priv->has_grab = has_grab;
}

/**
 * ctk_widget_device_is_shadowed:
 * @widget: a #CtkWidget
 * @device: a #CdkDevice
 *
 * Returns %TRUE if @device has been shadowed by a CTK+
 * device grab on another widget, so it would stop sending
 * events to @widget. This may be used in the
 * #CtkWidget::grab-notify signal to check for specific
 * devices. See ctk_device_grab_add().
 *
 * Returns: %TRUE if there is an ongoing grab on @device
 *          by another #CtkWidget than @widget.
 *
 * Since: 3.0
 **/
gboolean
ctk_widget_device_is_shadowed (CtkWidget *widget,
                               CdkDevice *device)
{
  CtkWindowGroup *group;
  CtkWidget *grab_widget, *toplevel;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (CDK_IS_DEVICE (device), FALSE);

  if (!_ctk_widget_get_realized (widget))
    return TRUE;

  toplevel = _ctk_widget_get_toplevel (widget);

  if (CTK_IS_WINDOW (toplevel))
    group = ctk_window_get_group (CTK_WINDOW (toplevel));
  else
    group = ctk_window_get_group (NULL);

  grab_widget = ctk_window_group_get_current_device_grab (group, device);

  /* Widget not inside the hierarchy of grab_widget */
  if (grab_widget &&
      widget != grab_widget &&
      !ctk_widget_is_ancestor (widget, grab_widget))
    return TRUE;

  grab_widget = ctk_window_group_get_current_grab (group);
  if (grab_widget && widget != grab_widget &&
      !ctk_widget_is_ancestor (widget, grab_widget))
    return TRUE;

  return FALSE;
}

/**
 * ctk_widget_set_name:
 * @widget: a #CtkWidget
 * @name: name for the widget
 *
 * Widgets can be named, which allows you to refer to them from a
 * CSS file. You can apply a style to widgets with a particular name
 * in the CSS file. See the documentation for the CSS syntax (on the
 * same page as the docs for #CtkStyleContext).
 *
 * Note that the CSS syntax has certain special characters to delimit
 * and represent elements in a selector (period, #, >, *...), so using
 * these will make your widget impossible to match by name. Any combination
 * of alphanumeric symbols, dashes and underscores will suffice.
 */
void
ctk_widget_set_name (CtkWidget	 *widget,
		     const gchar *name)
{
  CtkWidgetPrivate *priv;
  gchar *new_name;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = widget->priv;

  new_name = g_strdup (name);
  g_free (priv->name);
  priv->name = new_name;

  if (priv->context)
    ctk_style_context_set_id (priv->context, priv->name);

  ctk_css_node_set_id (priv->cssnode, priv->name);

  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_NAME]);
}

/**
 * ctk_widget_get_name:
 * @widget: a #CtkWidget
 *
 * Retrieves the name of a widget. See ctk_widget_set_name() for the
 * significance of widget names.
 *
 * Returns: name of the widget. This string is owned by CTK+ and
 * should not be modified or freed
 **/
const gchar*
ctk_widget_get_name (CtkWidget *widget)
{
  CtkWidgetPrivate *priv;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  priv = widget->priv;

  if (priv->name)
    return priv->name;
  return G_OBJECT_TYPE_NAME (widget);
}

static void
ctk_widget_update_state_flags (CtkWidget     *widget,
                               CtkStateFlags  flags_to_set,
                               CtkStateFlags  flags_to_unset)
{
  CtkWidgetPrivate *priv;

  priv = widget->priv;

  /* Handle insensitive first, since it is propagated
   * differently throughout the widget hierarchy.
   */
  if ((priv->state_flags & CTK_STATE_FLAG_INSENSITIVE) && (flags_to_unset & CTK_STATE_FLAG_INSENSITIVE))
    ctk_widget_set_sensitive (widget, TRUE);
  else if (!(priv->state_flags & CTK_STATE_FLAG_INSENSITIVE) && (flags_to_set & CTK_STATE_FLAG_INSENSITIVE))
    ctk_widget_set_sensitive (widget, FALSE);

  flags_to_set &= ~(CTK_STATE_FLAG_INSENSITIVE);
  flags_to_unset &= ~(CTK_STATE_FLAG_INSENSITIVE);

  if (flags_to_set != 0 || flags_to_unset != 0)
    {
      CtkStateData data;

      data.old_scale_factor = ctk_widget_get_scale_factor (widget);
      data.flags_to_set = flags_to_set;
      data.flags_to_unset = flags_to_unset;

      ctk_widget_propagate_state (widget, &data);
    }
}

/**
 * ctk_widget_set_state_flags:
 * @widget: a #CtkWidget
 * @flags: State flags to turn on
 * @clear: Whether to clear state before turning on @flags
 *
 * This function is for use in widget implementations. Turns on flag
 * values in the current widget state (insensitive, prelighted, etc.).
 *
 * This function accepts the values %CTK_STATE_FLAG_DIR_LTR and
 * %CTK_STATE_FLAG_DIR_RTL but ignores them. If you want to set the widget's
 * direction, use ctk_widget_set_direction().
 *
 * It is worth mentioning that any other state than %CTK_STATE_FLAG_INSENSITIVE,
 * will be propagated down to all non-internal children if @widget is a
 * #CtkContainer, while %CTK_STATE_FLAG_INSENSITIVE itself will be propagated
 * down to all #CtkContainer children by different means than turning on the
 * state flag down the hierarchy, both ctk_widget_get_state_flags() and
 * ctk_widget_is_sensitive() will make use of these.
 *
 * Since: 3.0
 **/
void
ctk_widget_set_state_flags (CtkWidget     *widget,
                            CtkStateFlags  flags,
                            gboolean       clear)
{
#define ALLOWED_FLAGS (~(CTK_STATE_FLAG_DIR_LTR | CTK_STATE_FLAG_DIR_RTL))

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (flags < (1 << CTK_STATE_FLAGS_BITS));

  if ((!clear && (widget->priv->state_flags & flags) == flags) ||
      (clear && widget->priv->state_flags == flags))
    return;

  if (clear)
    ctk_widget_update_state_flags (widget, flags & ALLOWED_FLAGS, ~flags & ALLOWED_FLAGS);
  else
    ctk_widget_update_state_flags (widget, flags & ALLOWED_FLAGS, 0);

#undef ALLOWED_FLAGS
}

/**
 * ctk_widget_unset_state_flags:
 * @widget: a #CtkWidget
 * @flags: State flags to turn off
 *
 * This function is for use in widget implementations. Turns off flag
 * values for the current widget state (insensitive, prelighted, etc.).
 * See ctk_widget_set_state_flags().
 *
 * Since: 3.0
 **/
void
ctk_widget_unset_state_flags (CtkWidget     *widget,
                              CtkStateFlags  flags)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (flags < (1 << CTK_STATE_FLAGS_BITS));

  if ((widget->priv->state_flags & flags) == 0)
    return;

  ctk_widget_update_state_flags (widget, 0, flags);
}

/**
 * ctk_widget_get_state_flags:
 * @widget: a #CtkWidget
 *
 * Returns the widget state as a flag set. It is worth mentioning
 * that the effective %CTK_STATE_FLAG_INSENSITIVE state will be
 * returned, that is, also based on parent insensitivity, even if
 * @widget itself is sensitive.
 *
 * Also note that if you are looking for a way to obtain the
 * #CtkStateFlags to pass to a #CtkStyleContext method, you
 * should look at ctk_style_context_get_state().
 *
 * Returns: The state flags for widget
 *
 * Since: 3.0
 **/
CtkStateFlags
ctk_widget_get_state_flags (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);

  return widget->priv->state_flags;
}

/**
 * ctk_widget_set_state:
 * @widget: a #CtkWidget
 * @state: new state for @widget
 *
 * This function is for use in widget implementations. Sets the state
 * of a widget (insensitive, prelighted, etc.) Usually you should set
 * the state using wrapper functions such as ctk_widget_set_sensitive().
 *
 * Deprecated: 3.0: Use ctk_widget_set_state_flags() instead.
 **/
void
ctk_widget_set_state (CtkWidget           *widget,
		      CtkStateType         state)
{
  CtkStateFlags flags;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  if (state == ctk_widget_get_state (widget))
    return;
  G_GNUC_END_IGNORE_DEPRECATIONS;

  switch (state)
    {
    case CTK_STATE_ACTIVE:
      flags = CTK_STATE_FLAG_ACTIVE;
      break;
    case CTK_STATE_PRELIGHT:
      flags = CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags = CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags = CTK_STATE_FLAG_INSENSITIVE;
      break;
    case CTK_STATE_INCONSISTENT:
      flags = CTK_STATE_FLAG_INCONSISTENT;
      break;
    case CTK_STATE_FOCUSED:
      flags = CTK_STATE_FLAG_FOCUSED;
      break;
    case CTK_STATE_NORMAL:
    default:
      flags = 0;
      break;
    }

  ctk_widget_update_state_flags (widget,
                                 flags,
                                 (CTK_STATE_FLAG_ACTIVE | CTK_STATE_FLAG_PRELIGHT | CTK_STATE_FLAG_SELECTED
                                 | CTK_STATE_FLAG_INSENSITIVE | CTK_STATE_FLAG_INCONSISTENT | CTK_STATE_FLAG_FOCUSED) ^ flags);
}

/**
 * ctk_widget_get_state:
 * @widget: a #CtkWidget
 *
 * Returns the widget’s state. See ctk_widget_set_state().
 *
 * Returns: the state of @widget.
 *
 * Since: 2.18
 *
 * Deprecated: 3.0: Use ctk_widget_get_state_flags() instead.
 */
CtkStateType
ctk_widget_get_state (CtkWidget *widget)
{
  CtkStateFlags flags;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), CTK_STATE_NORMAL);

  flags = _ctk_widget_get_state_flags (widget);

  if (flags & CTK_STATE_FLAG_INSENSITIVE)
    return CTK_STATE_INSENSITIVE;
  else if (flags & CTK_STATE_FLAG_ACTIVE)
    return CTK_STATE_ACTIVE;
  else if (flags & CTK_STATE_FLAG_SELECTED)
    return CTK_STATE_SELECTED;
  else if (flags & CTK_STATE_FLAG_PRELIGHT)
    return CTK_STATE_PRELIGHT;
  else
    return CTK_STATE_NORMAL;
}

/**
 * ctk_widget_set_visible:
 * @widget: a #CtkWidget
 * @visible: whether the widget should be shown or not
 *
 * Sets the visibility state of @widget. Note that setting this to
 * %TRUE doesn’t mean the widget is actually viewable, see
 * ctk_widget_get_visible().
 *
 * This function simply calls ctk_widget_show() or ctk_widget_hide()
 * but is nicer to use when the visibility of the widget depends on
 * some condition.
 *
 * Since: 2.18
 **/
void
ctk_widget_set_visible (CtkWidget *widget,
                        gboolean   visible)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (visible)
    ctk_widget_show (widget);
  else
    ctk_widget_hide (widget);
}

void
_ctk_widget_set_visible_flag (CtkWidget *widget,
                              gboolean   visible)
{
  CtkWidgetPrivate *priv = widget->priv;

  priv->visible = visible;

  if (!visible)
    {
      priv->allocation.x = -1;
      priv->allocation.y = -1;
      priv->allocation.width = 1;
      priv->allocation.height = 1;
      memset (&priv->clip, 0, sizeof (priv->clip));
      memset (&priv->allocated_size, 0, sizeof (priv->allocated_size));
      priv->allocated_size_baseline = 0;
    }
}

/**
 * ctk_widget_get_visible:
 * @widget: a #CtkWidget
 *
 * Determines whether the widget is visible. If you want to
 * take into account whether the widget’s parent is also marked as
 * visible, use ctk_widget_is_visible() instead.
 *
 * This function does not check if the widget is obscured in any way.
 *
 * See ctk_widget_set_visible().
 *
 * Returns: %TRUE if the widget is visible
 *
 * Since: 2.18
 **/
gboolean
ctk_widget_get_visible (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->visible;
}

/**
 * ctk_widget_is_visible:
 * @widget: a #CtkWidget
 *
 * Determines whether the widget and all its parents are marked as
 * visible.
 *
 * This function does not check if the widget is obscured in any way.
 *
 * See also ctk_widget_get_visible() and ctk_widget_set_visible()
 *
 * Returns: %TRUE if the widget and all its parents are visible
 *
 * Since: 3.8
 **/
gboolean
ctk_widget_is_visible (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  while (widget)
    {
      CtkWidgetPrivate *priv = widget->priv;

      if (!priv->visible)
        return FALSE;

      widget = priv->parent;
    }

  return TRUE;
}

/**
 * ctk_widget_set_has_window:
 * @widget: a #CtkWidget
 * @has_window: whether or not @widget has a window.
 *
 * Specifies whether @widget has a #CdkWindow of its own. Note that
 * all realized widgets have a non-%NULL “window” pointer
 * (ctk_widget_get_window() never returns a %NULL window when a widget
 * is realized), but for many of them it’s actually the #CdkWindow of
 * one of its parent widgets. Widgets that do not create a %window for
 * themselves in #CtkWidget::realize must announce this by
 * calling this function with @has_window = %FALSE.
 *
 * This function should only be called by widget implementations,
 * and they should call it in their init() function.
 *
 * Since: 2.18
 **/
void
ctk_widget_set_has_window (CtkWidget *widget,
                           gboolean   has_window)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  widget->priv->no_window = !has_window;
}

/**
 * ctk_widget_get_has_window:
 * @widget: a #CtkWidget
 *
 * Determines whether @widget has a #CdkWindow of its own. See
 * ctk_widget_set_has_window().
 *
 * Returns: %TRUE if @widget has a window, %FALSE otherwise
 *
 * Since: 2.18
 **/
gboolean
ctk_widget_get_has_window (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return ! widget->priv->no_window;
}

/**
 * ctk_widget_is_toplevel:
 * @widget: a #CtkWidget
 *
 * Determines whether @widget is a toplevel widget.
 *
 * Currently only #CtkWindow and #CtkInvisible (and out-of-process
 * #CtkPlugs) are toplevel widgets. Toplevel widgets have no parent
 * widget.
 *
 * Returns: %TRUE if @widget is a toplevel, %FALSE otherwise
 *
 * Since: 2.18
 **/
gboolean
ctk_widget_is_toplevel (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->toplevel;
}

void
_ctk_widget_set_is_toplevel (CtkWidget *widget,
                             gboolean   is_toplevel)
{
  widget->priv->toplevel = is_toplevel;
}

/**
 * ctk_widget_is_drawable:
 * @widget: a #CtkWidget
 *
 * Determines whether @widget can be drawn to. A widget can be drawn
 * to if it is mapped and visible.
 *
 * Returns: %TRUE if @widget is drawable, %FALSE otherwise
 *
 * Since: 2.18
 **/
gboolean
ctk_widget_is_drawable (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return (_ctk_widget_get_visible (widget) &&
          _ctk_widget_get_mapped (widget));
}

/**
 * ctk_widget_get_realized:
 * @widget: a #CtkWidget
 *
 * Determines whether @widget is realized.
 *
 * Returns: %TRUE if @widget is realized, %FALSE otherwise
 *
 * Since: 2.20
 **/
gboolean
ctk_widget_get_realized (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->realized;
}

/**
 * ctk_widget_set_realized:
 * @widget: a #CtkWidget
 * @realized: %TRUE to mark the widget as realized
 *
 * Marks the widget as being realized. This function must only be 
 * called after all #CdkWindows for the @widget have been created 
 * and registered.
 *
 * This function should only ever be called in a derived widget's
 * “realize” or “unrealize” implementation.
 *
 * Since: 2.20
 */
void
ctk_widget_set_realized (CtkWidget *widget,
                         gboolean   realized)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  widget->priv->realized = realized;
}

/**
 * ctk_widget_get_mapped:
 * @widget: a #CtkWidget
 *
 * Whether the widget is mapped.
 *
 * Returns: %TRUE if the widget is mapped, %FALSE otherwise.
 *
 * Since: 2.20
 */
gboolean
ctk_widget_get_mapped (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->mapped;
}

/**
 * ctk_widget_set_mapped:
 * @widget: a #CtkWidget
 * @mapped: %TRUE to mark the widget as mapped
 *
 * Marks the widget as being mapped.
 *
 * This function should only ever be called in a derived widget's
 * “map” or “unmap” implementation.
 *
 * Since: 2.20
 */
void
ctk_widget_set_mapped (CtkWidget *widget,
                       gboolean   mapped)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  widget->priv->mapped = mapped;
}

/**
 * ctk_widget_set_app_paintable:
 * @widget: a #CtkWidget
 * @app_paintable: %TRUE if the application will paint on the widget
 *
 * Sets whether the application intends to draw on the widget in
 * an #CtkWidget::draw handler.
 *
 * This is a hint to the widget and does not affect the behavior of
 * the CTK+ core; many widgets ignore this flag entirely. For widgets
 * that do pay attention to the flag, such as #CtkEventBox and #CtkWindow,
 * the effect is to suppress default themed drawing of the widget's
 * background. (Children of the widget will still be drawn.) The application
 * is then entirely responsible for drawing the widget background.
 *
 * Note that the background is still drawn when the widget is mapped.
 **/
void
ctk_widget_set_app_paintable (CtkWidget *widget,
			      gboolean   app_paintable)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  app_paintable = (app_paintable != FALSE);

  if (widget->priv->app_paintable != app_paintable)
    {
      widget->priv->app_paintable = app_paintable;

      if (_ctk_widget_is_drawable (widget))
	ctk_widget_queue_draw (widget);

      g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_APP_PAINTABLE]);
    }
}

/**
 * ctk_widget_get_app_paintable:
 * @widget: a #CtkWidget
 *
 * Determines whether the application intends to draw on the widget in
 * an #CtkWidget::draw handler.
 *
 * See ctk_widget_set_app_paintable()
 *
 * Returns: %TRUE if the widget is app paintable
 *
 * Since: 2.18
 **/
gboolean
ctk_widget_get_app_paintable (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->app_paintable;
}

/**
 * ctk_widget_set_double_buffered:
 * @widget: a #CtkWidget
 * @double_buffered: %TRUE to double-buffer a widget
 *
 * Widgets are double buffered by default; you can use this function
 * to turn off the buffering. “Double buffered” simply means that
 * cdk_window_begin_draw_frame() and cdk_window_end_draw_frame() are called
 * automatically around expose events sent to the
 * widget. cdk_window_begin_draw_frame() diverts all drawing to a widget's
 * window to an offscreen buffer, and cdk_window_end_draw_frame() draws the
 * buffer to the screen. The result is that users see the window
 * update in one smooth step, and don’t see individual graphics
 * primitives being rendered.
 *
 * In very simple terms, double buffered widgets don’t flicker,
 * so you would only use this function to turn off double buffering
 * if you had special needs and really knew what you were doing.
 *
 * Note: if you turn off double-buffering, you have to handle
 * expose events, since even the clearing to the background color or
 * pixmap will not happen automatically (as it is done in
 * cdk_window_begin_draw_frame()).
 *
 * In 3.10 CTK and CDK have been restructured for translucent drawing. Since
 * then expose events for double-buffered widgets are culled into a single
 * event to the toplevel CDK window. If you now unset double buffering, you
 * will cause a separate rendering pass for every widget. This will likely
 * cause rendering problems - in particular related to stacking - and usually
 * increases rendering times significantly.
 *
 * Deprecated: 3.14: This function does not work under non-X11 backends or with
 * non-native windows.
 * It should not be used in newly written code.
 **/
void
ctk_widget_set_double_buffered (CtkWidget *widget,
				gboolean   double_buffered)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  double_buffered = (double_buffered != FALSE);

  if (widget->priv->double_buffered != double_buffered)
    {
      widget->priv->double_buffered = double_buffered;

      g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_DOUBLE_BUFFERED]);
    }
}

/**
 * ctk_widget_get_double_buffered:
 * @widget: a #CtkWidget
 *
 * Determines whether the widget is double buffered.
 *
 * See ctk_widget_set_double_buffered()
 *
 * Returns: %TRUE if the widget is double buffered
 *
 * Since: 2.18
 **/
gboolean
ctk_widget_get_double_buffered (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->double_buffered;
}

/**
 * ctk_widget_set_redraw_on_allocate:
 * @widget: a #CtkWidget
 * @redraw_on_allocate: if %TRUE, the entire widget will be redrawn
 *   when it is allocated to a new size. Otherwise, only the
 *   new portion of the widget will be redrawn.
 *
 * Sets whether the entire widget is queued for drawing when its size
 * allocation changes. By default, this setting is %TRUE and
 * the entire widget is redrawn on every size change. If your widget
 * leaves the upper left unchanged when made bigger, turning this
 * setting off will improve performance.

 * Note that for widgets where ctk_widget_get_has_window() is %FALSE
 * setting this flag to %FALSE turns off all allocation on resizing:
 * the widget will not even redraw if its position changes; this is to
 * allow containers that don’t draw anything to avoid excess
 * invalidations. If you set this flag on a widget with no window that
 * does draw on @widget->window, you are
 * responsible for invalidating both the old and new allocation of the
 * widget when the widget is moved and responsible for invalidating
 * regions newly when the widget increases size.
 **/
void
ctk_widget_set_redraw_on_allocate (CtkWidget *widget,
				   gboolean   redraw_on_allocate)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  widget->priv->redraw_on_alloc = redraw_on_allocate;
}

/**
 * ctk_widget_set_sensitive:
 * @widget: a #CtkWidget
 * @sensitive: %TRUE to make the widget sensitive
 *
 * Sets the sensitivity of a widget. A widget is sensitive if the user
 * can interact with it. Insensitive widgets are “grayed out” and the
 * user can’t interact with them. Insensitive widgets are known as
 * “inactive”, “disabled”, or “ghosted” in some other toolkits.
 **/
void
ctk_widget_set_sensitive (CtkWidget *widget,
			  gboolean   sensitive)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = widget->priv;

  sensitive = (sensitive != FALSE);

  if (priv->sensitive == sensitive)
    return;

  priv->sensitive = sensitive;

  if (priv->parent == NULL
      || ctk_widget_is_sensitive (priv->parent))
    {
      CtkStateData data;

      data.old_scale_factor = ctk_widget_get_scale_factor (widget);

      if (sensitive)
        {
          data.flags_to_set = 0;
          data.flags_to_unset = CTK_STATE_FLAG_INSENSITIVE;
        }
      else
        {
          data.flags_to_set = CTK_STATE_FLAG_INSENSITIVE;
          data.flags_to_unset = 0;
        }

      ctk_widget_propagate_state (widget, &data);
    }

  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_SENSITIVE]);
}

/**
 * ctk_widget_get_sensitive:
 * @widget: a #CtkWidget
 *
 * Returns the widget’s sensitivity (in the sense of returning
 * the value that has been set using ctk_widget_set_sensitive()).
 *
 * The effective sensitivity of a widget is however determined by both its
 * own and its parent widget’s sensitivity. See ctk_widget_is_sensitive().
 *
 * Returns: %TRUE if the widget is sensitive
 *
 * Since: 2.18
 */
gboolean
ctk_widget_get_sensitive (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->sensitive;
}

/**
 * ctk_widget_is_sensitive:
 * @widget: a #CtkWidget
 *
 * Returns the widget’s effective sensitivity, which means
 * it is sensitive itself and also its parent widget is sensitive
 *
 * Returns: %TRUE if the widget is effectively sensitive
 *
 * Since: 2.18
 */
gboolean
ctk_widget_is_sensitive (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return !(widget->priv->state_flags & CTK_STATE_FLAG_INSENSITIVE);
}

/**
 * ctk_widget_set_parent:
 * @widget: a #CtkWidget
 * @parent: parent container
 *
 * This function is useful only when implementing subclasses of
 * #CtkContainer.
 * Sets the container as the parent of @widget, and takes care of
 * some details such as updating the state and style of the child
 * to reflect its new location. The opposite function is
 * ctk_widget_unparent().
 **/
void
ctk_widget_set_parent (CtkWidget *widget,
		       CtkWidget *parent)
{
  CtkStateFlags parent_flags;
  CtkWidgetPrivate *priv;
  CtkStateData data;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CTK_IS_WIDGET (parent));
  g_return_if_fail (widget != parent);

  priv = widget->priv;

  if (priv->parent != NULL)
    {
      g_warning ("Can't set a parent on widget which has a parent");
      return;
    }
  if (_ctk_widget_is_toplevel (widget))
    {
      g_warning ("Can't set a parent on a toplevel widget");
      return;
    }

  data.old_scale_factor = ctk_widget_get_scale_factor (widget);

  /* keep this function in sync with ctk_menu_attach_to_widget()
   */

  g_object_ref_sink (widget);

  ctk_widget_push_verify_invariants (widget);

  priv->parent = parent;

  parent_flags = _ctk_widget_get_state_flags (parent);

  /* Merge both old state and current parent state,
   * making sure to only propagate the right states */
  data.flags_to_set = parent_flags & CTK_STATE_FLAGS_DO_PROPAGATE;
  data.flags_to_unset = 0;
  ctk_widget_propagate_state (widget, &data);

  if (ctk_css_node_get_parent (widget->priv->cssnode) == NULL)
    ctk_css_node_set_parent (widget->priv->cssnode, parent->priv->cssnode);
  if (priv->context)
    ctk_style_context_set_parent (priv->context,
                                  _ctk_widget_get_style_context (parent));

  _ctk_widget_update_parent_muxer (widget);

  g_signal_emit (widget, widget_signals[PARENT_SET], 0, NULL);
  if (priv->parent->priv->anchored)
    _ctk_widget_propagate_hierarchy_changed (widget, NULL);
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_PARENT]);

  /* Enforce realized/mapped invariants
   */
  if (_ctk_widget_get_realized (priv->parent))
    ctk_widget_realize (widget);

  if (_ctk_widget_get_visible (priv->parent) &&
      _ctk_widget_get_visible (widget))
    {
      if (_ctk_widget_get_child_visible (widget) &&
	  _ctk_widget_get_mapped (priv->parent))
	ctk_widget_map (widget);

      ctk_widget_queue_resize (priv->parent);
    }

  /* child may cause parent's expand to change, if the child is
   * expanded. If child is not expanded, then it can't modify the
   * parent's expand. If the child becomes expanded later then it will
   * queue compute_expand then. This optimization plus defaulting
   * newly-constructed widgets to need_compute_expand=FALSE should
   * mean that initially building a widget tree doesn't have to keep
   * walking up setting need_compute_expand on parents over and over.
   *
   * We can't change a parent to need to expand unless we're visible.
   */
  if (_ctk_widget_get_visible (widget) &&
      (priv->need_compute_expand ||
       priv->computed_hexpand ||
       priv->computed_vexpand))
    {
      ctk_widget_queue_compute_expand (parent);
    }

  ctk_widget_pop_verify_invariants (widget);
}

/**
 * ctk_widget_get_parent:
 * @widget: a #CtkWidget
 *
 * Returns the parent container of @widget.
 *
 * Returns: (transfer none) (nullable): the parent container of @widget, or %NULL
 **/
CtkWidget *
ctk_widget_get_parent (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return widget->priv->parent;
}

static CtkModifierStyle *
_ctk_widget_get_modifier_properties (CtkWidget *widget)
{
  CtkModifierStyle *style;

  style = g_object_get_qdata (G_OBJECT (widget), quark_modifier_style);

  if (G_UNLIKELY (!style))
    {
      CtkStyleContext *context;

      style = _ctk_modifier_style_new ();
      g_object_set_qdata_full (G_OBJECT (widget),
                               quark_modifier_style,
                               style,
                               (GDestroyNotify) g_object_unref);

      context = _ctk_widget_get_style_context (widget);

      ctk_style_context_add_provider (context,
                                      CTK_STYLE_PROVIDER (style),
                                      CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

  return style;
}

/**
 * ctk_widget_override_color:
 * @widget: a #CtkWidget
 * @state: the state for which to set the color
 * @color: (allow-none): the color to assign, or %NULL to undo the effect
 *     of previous calls to ctk_widget_override_color()
 *
 * Sets the color to use for a widget.
 *
 * All other style values are left untouched.
 *
 * This function does not act recursively. Setting the color of a
 * container does not affect its children. Note that some widgets that
 * you may not think of as containers, for instance #CtkButtons,
 * are actually containers.
 *
 * This API is mostly meant as a quick way for applications to
 * change a widget appearance. If you are developing a widgets
 * library and intend this change to be themeable, it is better
 * done by setting meaningful CSS classes in your
 * widget/container implementation through ctk_style_context_add_class().
 *
 * This way, your widget library can install a #CtkCssProvider
 * with the %CTK_STYLE_PROVIDER_PRIORITY_FALLBACK priority in order
 * to provide a default styling for those widgets that need so, and
 * this theming may fully overridden by the user’s theme.
 *
 * Note that for complex widgets this may bring in undesired
 * results (such as uniform background color everywhere), in
 * these cases it is better to fully style such widgets through a
 * #CtkCssProvider with the %CTK_STYLE_PROVIDER_PRIORITY_APPLICATION
 * priority.
 *
 * Since: 3.0
 *
 * Deprecated:3.16: Use a custom style provider and style classes instead
 */
void
ctk_widget_override_color (CtkWidget     *widget,
                           CtkStateFlags  state,
                           const CdkRGBA *color)
{
  CtkModifierStyle *style;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  style = _ctk_widget_get_modifier_properties (widget);
  _ctk_modifier_style_set_color (style, state, color);
}

/**
 * ctk_widget_override_background_color:
 * @widget: a #CtkWidget
 * @state: the state for which to set the background color
 * @color: (allow-none): the color to assign, or %NULL to undo the effect
 *     of previous calls to ctk_widget_override_background_color()
 *
 * Sets the background color to use for a widget.
 *
 * All other style values are left untouched.
 * See ctk_widget_override_color().
 *
 * Since: 3.0
 *
 * Deprecated: 3.16: This function is not useful in the context of CSS-based
 *   rendering. If you wish to change the way a widget renders its background
 *   you should use a custom CSS style, through an application-specific
 *   #CtkStyleProvider and a CSS style class. You can also override the default
 *   drawing of a widget through the #CtkWidget::draw signal, and use Cairo to
 *   draw a specific color, regardless of the CSS style.
 */
void
ctk_widget_override_background_color (CtkWidget     *widget,
                                      CtkStateFlags  state,
                                      const CdkRGBA *color)
{
  CtkModifierStyle *style;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  style = _ctk_widget_get_modifier_properties (widget);
  _ctk_modifier_style_set_background_color (style, state, color);
}

/**
 * ctk_widget_override_font:
 * @widget: a #CtkWidget
 * @font_desc: (allow-none): the font description to use, or %NULL to undo
 *     the effect of previous calls to ctk_widget_override_font()
 *
 * Sets the font to use for a widget. All other style values are
 * left untouched. See ctk_widget_override_color().
 *
 * Since: 3.0
 *
 * Deprecated: 3.16: This function is not useful in the context of CSS-based
 *   rendering. If you wish to change the font a widget uses to render its text
 *   you should use a custom CSS style, through an application-specific
 *   #CtkStyleProvider and a CSS style class.
 */
void
ctk_widget_override_font (CtkWidget                  *widget,
                          const PangoFontDescription *font_desc)
{
  CtkModifierStyle *style;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  style = _ctk_widget_get_modifier_properties (widget);
  _ctk_modifier_style_set_font (style, font_desc);
}

/**
 * ctk_widget_override_symbolic_color:
 * @widget: a #CtkWidget
 * @name: the name of the symbolic color to modify
 * @color: (allow-none): the color to assign (does not need
 *     to be allocated), or %NULL to undo the effect of previous
 *     calls to ctk_widget_override_symbolic_color()
 *
 * Sets a symbolic color for a widget.
 *
 * All other style values are left untouched.
 * See ctk_widget_override_color() for overriding the foreground
 * or background color.
 *
 * Since: 3.0
 *
 * Deprecated: 3.16: This function is not useful in the context of CSS-based
 *   rendering. If you wish to change the color used to render symbolic icons
 *   you should use a custom CSS style, through an application-specific
 *   #CtkStyleProvider and a CSS style class.
 */
void
ctk_widget_override_symbolic_color (CtkWidget     *widget,
                                    const gchar   *name,
                                    const CdkRGBA *color)
{
  CtkModifierStyle *style;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  style = _ctk_widget_get_modifier_properties (widget);
  _ctk_modifier_style_map_color (style, name, color);
}

/**
 * ctk_widget_override_cursor:
 * @widget: a #CtkWidget
 * @cursor: (allow-none): the color to use for primary cursor (does not need to be
 *     allocated), or %NULL to undo the effect of previous calls to
 *     of ctk_widget_override_cursor().
 * @secondary_cursor: (allow-none): the color to use for secondary cursor (does not
 *     need to be allocated), or %NULL to undo the effect of previous
 *     calls to of ctk_widget_override_cursor().
 *
 * Sets the cursor color to use in a widget, overriding the
 * cursor-color and secondary-cursor-color
 * style properties. All other style values are left untouched.
 * See also ctk_widget_modify_style().
 *
 * Note that the underlying properties have the #CdkColor type,
 * so the alpha value in @primary and @secondary will be ignored.
 *
 * Since: 3.0
 *
 * Deprecated: 3.16: This function is not useful in the context of CSS-based
 *   rendering. If you wish to change the color used to render the primary
 *   and secondary cursors you should use a custom CSS style, through an
 *   application-specific #CtkStyleProvider and a CSS style class.
 */
void
ctk_widget_override_cursor (CtkWidget     *widget,
                            const CdkRGBA *cursor,
                            const CdkRGBA *secondary_cursor)
{
  CtkModifierStyle *style;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  style = _ctk_widget_get_modifier_properties (widget);
  _ctk_modifier_style_set_color_property (style,
                                          CTK_TYPE_WIDGET,
                                          "cursor-color", cursor);
  _ctk_modifier_style_set_color_property (style,
                                          CTK_TYPE_WIDGET,
                                          "secondary-cursor-color",
                                          secondary_cursor);
}

static void
ctk_widget_real_direction_changed (CtkWidget        *widget,
                                   CtkTextDirection  previous_direction)
{
  ctk_widget_queue_resize (widget);
}

static void
ctk_widget_real_style_set (CtkWidget *widget,
                           CtkStyle  *previous_style)
{
}

typedef struct {
  CtkWidget *previous_toplevel;
  CdkScreen *previous_screen;
  CdkScreen *new_screen;
} HierarchyChangedInfo;

static void
do_screen_change (CtkWidget *widget,
		  CdkScreen *old_screen,
		  CdkScreen *new_screen)
{
  if (old_screen != new_screen)
    {
      CtkWidgetPrivate *priv = widget->priv;

      if (old_screen)
	{
	  PangoContext *context = g_object_get_qdata (G_OBJECT (widget), quark_pango_context);
	  if (context)
	    g_object_set_qdata (G_OBJECT (widget), quark_pango_context, NULL);
	}

      _ctk_tooltip_hide (widget);

      if (new_screen && priv->context)
        ctk_style_context_set_screen (priv->context, new_screen);

      g_signal_emit (widget, widget_signals[SCREEN_CHANGED], 0, old_screen);
    }
}

static void
ctk_widget_propagate_hierarchy_changed_recurse (CtkWidget *widget,
						gpointer   client_data)
{
  CtkWidgetPrivate *priv = widget->priv;
  HierarchyChangedInfo *info = client_data;
  gboolean new_anchored = _ctk_widget_is_toplevel (widget) ||
                 (priv->parent && priv->parent->priv->anchored);

  if (priv->anchored != new_anchored)
    {
      g_object_ref (widget);

      priv->anchored = new_anchored;

      /* This can only happen with ctk_widget_reparent() */
      if (priv->realized)
        {
          if (new_anchored)
            ctk_widget_connect_frame_clock (widget,
                                            ctk_widget_get_frame_clock (widget));
          else
            ctk_widget_disconnect_frame_clock (widget,
                                               ctk_widget_get_frame_clock (info->previous_toplevel));
        }

      g_signal_emit (widget, widget_signals[HIERARCHY_CHANGED], 0, info->previous_toplevel);
      do_screen_change (widget, info->previous_screen, info->new_screen);

      if (CTK_IS_CONTAINER (widget))
	ctk_container_forall (CTK_CONTAINER (widget),
			      ctk_widget_propagate_hierarchy_changed_recurse,
			      client_data);

      g_object_unref (widget);
    }
}

/**
 * _ctk_widget_propagate_hierarchy_changed:
 * @widget: a #CtkWidget
 * @previous_toplevel: Previous toplevel
 *
 * Propagates changes in the anchored state to a widget and all
 * children, unsetting or setting the %ANCHORED flag, and
 * emitting #CtkWidget::hierarchy-changed.
 **/
void
_ctk_widget_propagate_hierarchy_changed (CtkWidget *widget,
					 CtkWidget *previous_toplevel)
{
  CtkWidgetPrivate *priv = widget->priv;
  HierarchyChangedInfo info;

  info.previous_toplevel = previous_toplevel;
  info.previous_screen = previous_toplevel ? ctk_widget_get_screen (previous_toplevel) : NULL;

  if (_ctk_widget_is_toplevel (widget) ||
      (priv->parent && priv->parent->priv->anchored))
    info.new_screen = ctk_widget_get_screen (widget);
  else
    info.new_screen = NULL;

  if (info.previous_screen)
    g_object_ref (info.previous_screen);
  if (previous_toplevel)
    g_object_ref (previous_toplevel);

  ctk_widget_propagate_hierarchy_changed_recurse (widget, &info);

  if (previous_toplevel)
    g_object_unref (previous_toplevel);
  if (info.previous_screen)
    g_object_unref (info.previous_screen);
}

static void
ctk_widget_propagate_screen_changed_recurse (CtkWidget *widget,
					     gpointer   client_data)
{
  HierarchyChangedInfo *info = client_data;

  g_object_ref (widget);

  do_screen_change (widget, info->previous_screen, info->new_screen);

  if (CTK_IS_CONTAINER (widget))
    ctk_container_forall (CTK_CONTAINER (widget),
			  ctk_widget_propagate_screen_changed_recurse,
			  client_data);

  g_object_unref (widget);
}

/**
 * ctk_widget_is_composited:
 * @widget: a #CtkWidget
 *
 * Whether @widget can rely on having its alpha channel
 * drawn correctly. On X11 this function returns whether a
 * compositing manager is running for @widget’s screen.
 *
 * Please note that the semantics of this call will change
 * in the future if used on a widget that has a composited
 * window in its hierarchy (as set by cdk_window_set_composited()).
 *
 * Returns: %TRUE if the widget can rely on its alpha
 * channel being drawn correctly.
 *
 * Since: 2.10
 *
 * Deprecated: 3.22: Use cdk_screen_is_composited() instead.
 */
gboolean
ctk_widget_is_composited (CtkWidget *widget)
{
  CdkScreen *screen;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  screen = ctk_widget_get_screen (widget);

  return cdk_screen_is_composited (screen);
}

static void
propagate_composited_changed (CtkWidget *widget,
			      gpointer dummy)
{
  if (CTK_IS_CONTAINER (widget))
    {
      ctk_container_forall (CTK_CONTAINER (widget),
			    propagate_composited_changed,
			    NULL);
    }

  g_signal_emit (widget, widget_signals[COMPOSITED_CHANGED], 0);
}

void
_ctk_widget_propagate_composited_changed (CtkWidget *widget)
{
  propagate_composited_changed (widget, NULL);
}

/**
 * _ctk_widget_propagate_screen_changed:
 * @widget: a #CtkWidget
 * @previous_screen: Previous screen
 *
 * Propagates changes in the screen for a widget to all
 * children, emitting #CtkWidget::screen-changed.
 **/
void
_ctk_widget_propagate_screen_changed (CtkWidget    *widget,
				      CdkScreen    *previous_screen)
{
  HierarchyChangedInfo info;

  info.previous_screen = previous_screen;
  info.new_screen = ctk_widget_get_screen (widget);

  if (previous_screen)
    g_object_ref (previous_screen);

  ctk_widget_propagate_screen_changed_recurse (widget, &info);

  if (previous_screen)
    g_object_unref (previous_screen);
}

static void
reset_style_recurse (CtkWidget *widget, gpointer data)
{
  _ctk_widget_invalidate_style_context (widget, CTK_CSS_CHANGE_ANY);

  if (CTK_IS_CONTAINER (widget))
    ctk_container_forall (CTK_CONTAINER (widget),
			  reset_style_recurse,
			  NULL);
}

/**
 * ctk_widget_reset_style:
 * @widget: a #CtkWidget
 *
 * Updates the style context of @widget and all descendants
 * by updating its widget path. #CtkContainers may want
 * to use this on a child when reordering it in a way that a different
 * style might apply to it. See also ctk_container_get_path_for_child().
 *
 * Since: 3.0
 */
void
ctk_widget_reset_style (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  reset_style_recurse (widget, NULL);

  g_list_foreach (widget->priv->attached_windows,
                  (GFunc) reset_style_recurse, NULL);
}

#ifdef G_ENABLE_CONSISTENCY_CHECKS

/* Verify invariants, see docs/widget_system.txt for notes on much of
 * this.  Invariants may be temporarily broken while we’re in the
 * process of updating state, of course, so you can only
 * verify_invariants() after a given operation is complete.
 * Use push/pop_verify_invariants to help with that.
 */
static void
ctk_widget_verify_invariants (CtkWidget *widget)
{
  CtkWidget *parent;

  if (widget->priv->verifying_invariants_count > 0)
    return;

  parent = widget->priv->parent;

  if (widget->priv->mapped)
    {
      /* Mapped implies ... */

      if (!widget->priv->realized)
        g_warning ("%s %p is mapped but not realized",
                   G_OBJECT_TYPE_NAME (widget), widget);

      if (!widget->priv->visible)
        g_warning ("%s %p is mapped but not visible",
                   G_OBJECT_TYPE_NAME (widget), widget);

      if (!widget->priv->toplevel)
        {
          if (!widget->priv->child_visible)
            g_warning ("%s %p is mapped but not child_visible",
                       G_OBJECT_TYPE_NAME (widget), widget);
        }
    }
  else
    {
      /* Not mapped implies... */

#if 0
  /* This check makes sense for normal toplevels, but for
   * something like a toplevel that is embedded within a clutter
   * state, mapping may depend on external factors.
   */
      if (widget->priv->toplevel)
        {
          if (widget->priv->visible)
            g_warning ("%s %p toplevel is visible but not mapped",
                       G_OBJECT_TYPE_NAME (widget), widget);
        }
#endif
    }

  /* Parent related checks aren't possible if parent has
   * verifying_invariants_count > 0 because parent needs to recurse
   * children first before the invariants will hold.
   */
  if (parent == NULL || parent->priv->verifying_invariants_count == 0)
    {
      if (parent &&
          parent->priv->realized)
        {
          /* Parent realized implies... */

#if 0
          /* This is in widget_system.txt but appears to fail
           * because there's no ctk_container_realize() that
           * realizes all children... instead we just lazily
           * wait for map to fix things up.
           */
          if (!widget->priv->realized)
            g_warning ("%s %p is realized but child %s %p is not realized",
                       G_OBJECT_TYPE_NAME (parent), parent,
                       G_OBJECT_TYPE_NAME (widget), widget);
#endif
        }
      else if (!widget->priv->toplevel)
        {
          /* No parent or parent not realized on non-toplevel implies... */

          if (widget->priv->realized && !widget->priv->in_reparent)
            g_warning ("%s %p is not realized but child %s %p is realized",
                       parent ? G_OBJECT_TYPE_NAME (parent) : "no parent", parent,
                       G_OBJECT_TYPE_NAME (widget), widget);
        }

      if (parent &&
          parent->priv->mapped &&
          widget->priv->visible &&
          widget->priv->child_visible)
        {
          /* Parent mapped and we are visible implies... */

          if (!widget->priv->mapped)
            g_warning ("%s %p is mapped but visible child %s %p is not mapped",
                       G_OBJECT_TYPE_NAME (parent), parent,
                       G_OBJECT_TYPE_NAME (widget), widget);
        }
      else if (!widget->priv->toplevel)
        {
          /* No parent or parent not mapped on non-toplevel implies... */

          if (widget->priv->mapped && !widget->priv->in_reparent)
            g_warning ("%s %p is mapped but visible=%d child_visible=%d parent %s %p mapped=%d",
                       G_OBJECT_TYPE_NAME (widget), widget,
                       widget->priv->visible,
                       widget->priv->child_visible,
                       parent ? G_OBJECT_TYPE_NAME (parent) : "no parent", parent,
                       parent ? parent->priv->mapped : FALSE);
        }
    }

  if (!widget->priv->realized)
    {
      /* Not realized implies... */

#if 0
      /* widget_system.txt says these hold, but they don't. */
      if (widget->priv->alloc_needed)
        g_warning ("%s %p alloc needed but not realized",
                   G_OBJECT_TYPE_NAME (widget), widget);

      if (widget->priv->width_request_needed)
        g_warning ("%s %p width request needed but not realized",
                   G_OBJECT_TYPE_NAME (widget), widget);

      if (widget->priv->height_request_needed)
        g_warning ("%s %p height request needed but not realized",
                   G_OBJECT_TYPE_NAME (widget), widget);
#endif
    }
}

/* The point of this push/pop is that invariants may not hold while
 * we’re busy making changes. So we only check at the outermost call
 * on the call stack, after we finish updating everything.
 */
static void
ctk_widget_push_verify_invariants (CtkWidget *widget)
{
  widget->priv->verifying_invariants_count += 1;
}

static void
ctk_widget_verify_child_invariants (CtkWidget *widget,
                                    gpointer   client_data)
{
  /* We don't recurse further; this is a one-level check. */
  ctk_widget_verify_invariants (widget);
}

static void
ctk_widget_pop_verify_invariants (CtkWidget *widget)
{
  g_assert (widget->priv->verifying_invariants_count > 0);

  widget->priv->verifying_invariants_count -= 1;

  if (widget->priv->verifying_invariants_count == 0)
    {
      ctk_widget_verify_invariants (widget);

      if (CTK_IS_CONTAINER (widget))
        {
          /* Check one level of children, because our
           * push_verify_invariants() will have prevented some of the
           * checks. This does not recurse because if recursion is
           * needed, it will happen naturally as each child has a
           * push/pop on that child. For example if we're recursively
           * mapping children, we'll push/pop on each child as we map
           * it.
           */
          ctk_container_forall (CTK_CONTAINER (widget),
                                ctk_widget_verify_child_invariants,
                                NULL);
        }
    }
}
#endif /* G_ENABLE_CONSISTENCY_CHECKS */

static PangoContext *
ctk_widget_peek_pango_context (CtkWidget *widget)
{
  return g_object_get_qdata (G_OBJECT (widget), quark_pango_context);
}

/**
 * ctk_widget_get_pango_context:
 * @widget: a #CtkWidget
 *
 * Gets a #PangoContext with the appropriate font map, font description,
 * and base direction for this widget. Unlike the context returned
 * by ctk_widget_create_pango_context(), this context is owned by
 * the widget (it can be used until the screen for the widget changes
 * or the widget is removed from its toplevel), and will be updated to
 * match any changes to the widget’s attributes. This can be tracked
 * by using the #CtkWidget::screen-changed signal on the widget.
 *
 * Returns: (transfer none): the #PangoContext for the widget.
 **/
PangoContext *
ctk_widget_get_pango_context (CtkWidget *widget)
{
  PangoContext *context;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  context = g_object_get_qdata (G_OBJECT (widget), quark_pango_context);
  if (!context)
    {
      context = ctk_widget_create_pango_context (CTK_WIDGET (widget));
      g_object_set_qdata_full (G_OBJECT (widget),
			       quark_pango_context,
			       context,
			       g_object_unref);
    }

  return context;
}

static PangoFontMap *
ctk_widget_get_effective_font_map (CtkWidget *widget)
{
  PangoFontMap *font_map;

  font_map = PANGO_FONT_MAP (g_object_get_qdata (G_OBJECT (widget), quark_font_map));
  if (font_map)
    return font_map;
  else if (widget->priv->parent)
    return ctk_widget_get_effective_font_map (widget->priv->parent);
  else
    return pango_cairo_font_map_get_default ();
}

static void
update_pango_context (CtkWidget    *widget,
                      PangoContext *context)
{
  PangoFontDescription *font_desc;
  CtkStyleContext *style_context;
  CdkScreen *screen;
  cairo_font_options_t *font_options;

  style_context = _ctk_widget_get_style_context (widget);
  ctk_style_context_get (style_context,
                         ctk_style_context_get_state (style_context),
                         "font", &font_desc,
                         NULL);

  pango_context_set_font_description (context, font_desc);

  pango_font_description_free (font_desc);

  pango_context_set_base_dir (context,
			      _ctk_widget_get_direction (widget) == CTK_TEXT_DIR_LTR ?
			      PANGO_DIRECTION_LTR : PANGO_DIRECTION_RTL);

  pango_cairo_context_set_resolution (context,
                                      _ctk_css_number_value_get (
                                          _ctk_style_context_peek_property (style_context,
                                                                            CTK_CSS_PROPERTY_DPI),
                                          100));

  screen = ctk_widget_get_screen_unchecked (widget);
  font_options = (cairo_font_options_t*)g_object_get_qdata (G_OBJECT (widget), quark_font_options);
  if (screen && font_options)
    {
      cairo_font_options_t *options;

      options = cairo_font_options_copy (cdk_screen_get_font_options (screen));
      cairo_font_options_merge (options, font_options);
      pango_cairo_context_set_font_options (context, options);
      cairo_font_options_destroy (options);
    }
  else if (screen)
    {
      pango_cairo_context_set_font_options (context,
                                            cdk_screen_get_font_options (screen));
    }

  pango_context_set_font_map (context, ctk_widget_get_effective_font_map (widget));
}

static void
ctk_widget_update_pango_context (CtkWidget *widget)
{
  PangoContext *context = ctk_widget_peek_pango_context (widget);

  if (context)
    update_pango_context (widget, context);
}

/**
 * ctk_widget_set_font_options:
 * @widget: a #CtkWidget
 * @options: (allow-none): a #cairo_font_options_t, or %NULL to unset any
 *   previously set default font options.
 *
 * Sets the #cairo_font_options_t used for Pango rendering in this widget.
 * When not set, the default font options for the #CdkScreen will be used.
 *
 * Since: 3.18
 **/
void
ctk_widget_set_font_options (CtkWidget                  *widget,
                             const cairo_font_options_t *options)
{
  cairo_font_options_t *font_options;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  font_options = (cairo_font_options_t *)g_object_get_qdata (G_OBJECT (widget), quark_font_options);
  if (font_options != options)
    {
      g_object_set_qdata_full (G_OBJECT (widget),
                               quark_font_options,
                               options ? cairo_font_options_copy (options) : NULL,
                               (GDestroyNotify)cairo_font_options_destroy);

      ctk_widget_update_pango_context (widget);
    }
}

/**
 * ctk_widget_get_font_options:
 * @widget: a #CtkWidget
 *
 * Returns the #cairo_font_options_t used for Pango rendering. When not set,
 * the defaults font options for the #CdkScreen will be used.
 *
 * Returns: (transfer none) (nullable): the #cairo_font_options_t or %NULL if not set
 *
 * Since: 3.18
 **/
const cairo_font_options_t *
ctk_widget_get_font_options (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return (cairo_font_options_t *)g_object_get_qdata (G_OBJECT (widget), quark_font_options);
}

static void
ctk_widget_set_font_map_recurse (CtkWidget *widget, gpointer data)
{
  if (g_object_get_qdata (G_OBJECT (widget), quark_font_map))
    return;

  ctk_widget_update_pango_context (widget);

  if (CTK_IS_CONTAINER (widget))
    ctk_container_forall (CTK_CONTAINER (widget),
                          ctk_widget_set_font_map_recurse,
                          data);
}

/**
 * ctk_widget_set_font_map:
 * @widget: a #CtkWidget
 * @font_map: (allow-none): a #PangoFontMap, or %NULL to unset any previously
 *     set font map
 *
 * Sets the font map to use for Pango rendering. When not set, the widget
 * will inherit the font map from its parent.
 *
 * Since: 3.18
 */
void
ctk_widget_set_font_map (CtkWidget    *widget,
                         PangoFontMap *font_map)
{
  PangoFontMap *map;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  map = PANGO_FONT_MAP (g_object_get_qdata (G_OBJECT (widget), quark_font_map));
  if (map == font_map)
    return;

  g_object_set_qdata_full (G_OBJECT (widget),
                           quark_font_map,
                           g_object_ref (font_map),
                           g_object_unref);

  ctk_widget_update_pango_context (widget);
  if (CTK_IS_CONTAINER (widget))
    ctk_container_forall (CTK_CONTAINER (widget),
                          ctk_widget_set_font_map_recurse,
                          NULL);
}

/**
 * ctk_widget_get_font_map:
 * @widget: a #CtkWidget
 *
 * Gets the font map that has been set with ctk_widget_set_font_map().
 *
 * Returns: (transfer none) (nullable): A #PangoFontMap, or %NULL
 *
 * Since: 3.18
 */
PangoFontMap *
ctk_widget_get_font_map (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return PANGO_FONT_MAP (g_object_get_qdata (G_OBJECT (widget), quark_font_map));
}

/**
 * ctk_widget_create_pango_context:
 * @widget: a #CtkWidget
 *
 * Creates a new #PangoContext with the appropriate font map,
 * font options, font description, and base direction for drawing
 * text for this widget. See also ctk_widget_get_pango_context().
 *
 * Returns: (transfer full): the new #PangoContext
 **/
PangoContext *
ctk_widget_create_pango_context (CtkWidget *widget)
{
  CdkDisplay *display;
  PangoContext *context;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  display = ctk_widget_get_display (widget);
  context = cdk_pango_context_get_for_display (display);
  update_pango_context (widget, context);
  pango_context_set_language (context, ctk_get_default_language ());

  return context;
}

/**
 * ctk_widget_create_pango_layout:
 * @widget: a #CtkWidget
 * @text: (nullable): text to set on the layout (can be %NULL)
 *
 * Creates a new #PangoLayout with the appropriate font map,
 * font description, and base direction for drawing text for
 * this widget.
 *
 * If you keep a #PangoLayout created in this way around, you need
 * to re-create it when the widget #PangoContext is replaced.
 * This can be tracked by using the #CtkWidget::screen-changed signal
 * on the widget.
 *
 * Returns: (transfer full): the new #PangoLayout
 **/
PangoLayout *
ctk_widget_create_pango_layout (CtkWidget   *widget,
				const gchar *text)
{
  PangoLayout *layout;
  PangoContext *context;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  context = ctk_widget_get_pango_context (widget);
  layout = pango_layout_new (context);

  if (text)
    pango_layout_set_text (layout, text, -1);

  return layout;
}

/**
 * ctk_widget_render_icon_pixbuf:
 * @widget: a #CtkWidget
 * @stock_id: a stock ID
 * @size: (type int): a stock size (#CtkIconSize). A size of `(CtkIconSize)-1`
 *     means render at the size of the source and don’t scale (if there are
 *     multiple source sizes, CTK+ picks one of the available sizes).
 *
 * A convenience function that uses the theme engine and style
 * settings for @widget to look up @stock_id and render it to
 * a pixbuf. @stock_id should be a stock icon ID such as
 * #CTK_STOCK_OPEN or #CTK_STOCK_OK. @size should be a size
 * such as #CTK_ICON_SIZE_MENU.
 *
 * The pixels in the returned #CdkPixbuf are shared with the rest of
 * the application and should not be modified. The pixbuf should be freed
 * after use with g_object_unref().
 *
 * Returns: (transfer full) (nullable): a new pixbuf, or %NULL if the
 *     stock ID wasn’t known
 *
 * Since: 3.0
 *
 * Deprecated: 3.10: Use ctk_icon_theme_load_icon() instead.
 **/
CdkPixbuf*
ctk_widget_render_icon_pixbuf (CtkWidget   *widget,
                               const gchar *stock_id,
                               CtkIconSize  size)
{
  CtkStyleContext *context;
  CtkIconSet *icon_set;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  g_return_val_if_fail (size > CTK_ICON_SIZE_INVALID || size == -1, NULL);

  context = _ctk_widget_get_style_context (widget);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  icon_set = ctk_style_context_lookup_icon_set (context, stock_id);

  if (icon_set == NULL)
    return NULL;

  return ctk_icon_set_render_icon_pixbuf (icon_set, context, size);
  G_GNUC_END_IGNORE_DEPRECATIONS;
}

/**
 * ctk_widget_set_parent_window:
 * @widget: a #CtkWidget.
 * @parent_window: the new parent window.
 *
 * Sets a non default parent window for @widget.
 *
 * For #CtkWindow classes, setting a @parent_window effects whether
 * the window is a toplevel window or can be embedded into other
 * widgets.
 *
 * For #CtkWindow classes, this needs to be called before the
 * window is realized.
 */
void
ctk_widget_set_parent_window (CtkWidget *widget,
                              CdkWindow *parent_window)
{
  CdkWindow *old_parent_window;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  old_parent_window = g_object_get_qdata (G_OBJECT (widget),
					  quark_parent_window);

  if (parent_window != old_parent_window)
    {
      gboolean is_plug;

      g_object_set_qdata (G_OBJECT (widget), quark_parent_window,
			  parent_window);
      if (old_parent_window)
	g_object_unref (old_parent_window);
      if (parent_window)
	g_object_ref (parent_window);

      /* Unset toplevel flag when adding a parent window to a widget,
       * this is the primary entry point to allow toplevels to be
       * embeddable.
       */
#ifdef CDK_WINDOWING_X11
      is_plug = CTK_IS_PLUG (widget);
#else
      is_plug = FALSE;
#endif
      if (CTK_IS_WINDOW (widget) && !is_plug)
	_ctk_window_set_is_toplevel (CTK_WINDOW (widget), parent_window == NULL);
    }
}

/**
 * ctk_widget_get_parent_window:
 * @widget: a #CtkWidget.
 *
 * Gets @widget’s parent window, or %NULL if it does not have one.
 *
 * Returns: (transfer none) (nullable): the parent window of @widget, or %NULL
 * if it does not have a parent window.
 **/
CdkWindow *
ctk_widget_get_parent_window (CtkWidget *widget)
{
  CtkWidgetPrivate *priv;
  CdkWindow *parent_window;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  priv = widget->priv;

  parent_window = g_object_get_qdata (G_OBJECT (widget), quark_parent_window);

  return (parent_window != NULL) ? parent_window :
	 (priv->parent != NULL) ? priv->parent->priv->window : NULL;
}


/**
 * ctk_widget_set_child_visible:
 * @widget: a #CtkWidget
 * @is_visible: if %TRUE, @widget should be mapped along with its parent.
 *
 * Sets whether @widget should be mapped along with its when its parent
 * is mapped and @widget has been shown with ctk_widget_show().
 *
 * The child visibility can be set for widget before it is added to
 * a container with ctk_widget_set_parent(), to avoid mapping
 * children unnecessary before immediately unmapping them. However
 * it will be reset to its default state of %TRUE when the widget
 * is removed from a container.
 *
 * Note that changing the child visibility of a widget does not
 * queue a resize on the widget. Most of the time, the size of
 * a widget is computed from all visible children, whether or
 * not they are mapped. If this is not the case, the container
 * can queue a resize itself.
 *
 * This function is only useful for container implementations and
 * never should be called by an application.
 **/
void
ctk_widget_set_child_visible (CtkWidget *widget,
			      gboolean   is_visible)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (!_ctk_widget_is_toplevel (widget));

  priv = widget->priv;

  g_object_ref (widget);
  ctk_widget_verify_invariants (widget);

  if (is_visible)
    priv->child_visible = TRUE;
  else
    {
      CtkWidget *toplevel;

      priv->child_visible = FALSE;

      toplevel = _ctk_widget_get_toplevel (widget);
      if (toplevel != widget && _ctk_widget_is_toplevel (toplevel))
	_ctk_window_unset_focus_and_default (CTK_WINDOW (toplevel), widget);
    }

  if (priv->parent && _ctk_widget_get_realized (priv->parent))
    {
      if (_ctk_widget_get_mapped (priv->parent) &&
	  priv->child_visible &&
	  _ctk_widget_get_visible (widget))
	ctk_widget_map (widget);
      else
	ctk_widget_unmap (widget);
    }

  ctk_widget_verify_invariants (widget);
  g_object_unref (widget);
}

/**
 * ctk_widget_get_child_visible:
 * @widget: a #CtkWidget
 *
 * Gets the value set with ctk_widget_set_child_visible().
 * If you feel a need to use this function, your code probably
 * needs reorganization.
 *
 * This function is only useful for container implementations and
 * never should be called by an application.
 *
 * Returns: %TRUE if the widget is mapped with the parent.
 **/
gboolean
ctk_widget_get_child_visible (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->child_visible;
}

static CdkScreen *
ctk_widget_get_screen_unchecked (CtkWidget *widget)
{
  CtkWidget *toplevel;

  toplevel = _ctk_widget_get_toplevel (widget);

  if (_ctk_widget_is_toplevel (toplevel))
    {
      if (CTK_IS_WINDOW (toplevel))
	return _ctk_window_get_screen (CTK_WINDOW (toplevel));
      else if (CTK_IS_INVISIBLE (toplevel))
	return ctk_invisible_get_screen (CTK_INVISIBLE (widget));
    }

  return NULL;
}

/**
 * ctk_widget_get_screen:
 * @widget: a #CtkWidget
 *
 * Get the #CdkScreen from the toplevel window associated with
 * this widget. This function can only be called after the widget
 * has been added to a widget hierarchy with a #CtkWindow
 * at the top.
 *
 * In general, you should only create screen specific
 * resources when a widget has been realized, and you should
 * free those resources when the widget is unrealized.
 *
 * Returns: (transfer none): the #CdkScreen for the toplevel for this widget.
 *
 * Since: 2.2
 **/
CdkScreen*
ctk_widget_get_screen (CtkWidget *widget)
{
  CdkScreen *screen;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  screen = ctk_widget_get_screen_unchecked (widget);

  if (screen)
    return screen;
  else
    return cdk_screen_get_default ();
}

/**
 * ctk_widget_has_screen:
 * @widget: a #CtkWidget
 *
 * Checks whether there is a #CdkScreen is associated with
 * this widget. All toplevel widgets have an associated
 * screen, and all widgets added into a hierarchy with a toplevel
 * window at the top.
 *
 * Returns: %TRUE if there is a #CdkScreen associated
 *   with the widget.
 *
 * Since: 2.2
 **/
gboolean
ctk_widget_has_screen (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return (ctk_widget_get_screen_unchecked (widget) != NULL);
}

void
_ctk_widget_scale_changed (CtkWidget *widget)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = widget->priv;

  if (priv->context)
    ctk_style_context_set_scale (priv->context, ctk_widget_get_scale_factor (widget));

  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_SCALE_FACTOR]);

  ctk_widget_queue_draw (widget);

  if (CTK_IS_CONTAINER (widget))
    ctk_container_forall (CTK_CONTAINER (widget),
                          (CtkCallback) _ctk_widget_scale_changed,
                          NULL);
}

/**
 * ctk_widget_get_scale_factor:
 * @widget: a #CtkWidget
 *
 * Retrieves the internal scale factor that maps from window coordinates
 * to the actual device pixels. On traditional systems this is 1, on
 * high density outputs, it can be a higher value (typically 2).
 *
 * See cdk_window_get_scale_factor().
 *
 * Returns: the scale factor for @widget
 *
 * Since: 3.10
 */
gint
ctk_widget_get_scale_factor (CtkWidget *widget)
{
  CtkWidget *toplevel;
  CdkDisplay *display;
  CdkMonitor *monitor;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), 1);

  if (_ctk_widget_get_realized (widget))
    return cdk_window_get_scale_factor (_ctk_widget_get_window (widget));

  toplevel = _ctk_widget_get_toplevel (widget);
  if (toplevel && toplevel != widget)
    return ctk_widget_get_scale_factor (toplevel);

  /* else fall back to something that is more likely to be right than
   * just returning 1:
   */
  display = ctk_widget_get_display (widget);
  monitor = cdk_display_get_monitor (display, 0);

  return cdk_monitor_get_scale_factor (monitor);
}

/**
 * ctk_widget_get_display:
 * @widget: a #CtkWidget
 *
 * Get the #CdkDisplay for the toplevel window associated with
 * this widget. This function can only be called after the widget
 * has been added to a widget hierarchy with a #CtkWindow at the top.
 *
 * In general, you should only create display specific
 * resources when a widget has been realized, and you should
 * free those resources when the widget is unrealized.
 *
 * Returns: (transfer none): the #CdkDisplay for the toplevel for this widget.
 *
 * Since: 2.2
 **/
CdkDisplay*
ctk_widget_get_display (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return cdk_screen_get_display (ctk_widget_get_screen (widget));
}

/**
 * ctk_widget_get_root_window:
 * @widget: a #CtkWidget
 *
 * Get the root window where this widget is located. This function can
 * only be called after the widget has been added to a widget
 * hierarchy with #CtkWindow at the top.
 *
 * The root window is useful for such purposes as creating a popup
 * #CdkWindow associated with the window. In general, you should only
 * create display specific resources when a widget has been realized,
 * and you should free those resources when the widget is unrealized.
 *
 * Returns: (transfer none): the #CdkWindow root window for the toplevel for this widget.
 *
 * Since: 2.2
 *
 * Deprecated: 3.12: Use cdk_screen_get_root_window() instead
 */
CdkWindow*
ctk_widget_get_root_window (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return cdk_screen_get_root_window (ctk_widget_get_screen (widget));
}

/**
 * ctk_widget_child_focus:
 * @widget: a #CtkWidget
 * @direction: direction of focus movement
 *
 * This function is used by custom widget implementations; if you're
 * writing an app, you’d use ctk_widget_grab_focus() to move the focus
 * to a particular widget, and ctk_container_set_focus_chain() to
 * change the focus tab order. So you may want to investigate those
 * functions instead.
 *
 * ctk_widget_child_focus() is called by containers as the user moves
 * around the window using keyboard shortcuts. @direction indicates
 * what kind of motion is taking place (up, down, left, right, tab
 * forward, tab backward). ctk_widget_child_focus() emits the
 * #CtkWidget::focus signal; widgets override the default handler
 * for this signal in order to implement appropriate focus behavior.
 *
 * The default ::focus handler for a widget should return %TRUE if
 * moving in @direction left the focus on a focusable location inside
 * that widget, and %FALSE if moving in @direction moved the focus
 * outside the widget. If returning %TRUE, widgets normally
 * call ctk_widget_grab_focus() to place the focus accordingly;
 * if returning %FALSE, they don’t modify the current focus location.
 *
 * Returns: %TRUE if focus ended up inside @widget
 **/
gboolean
ctk_widget_child_focus (CtkWidget       *widget,
                        CtkDirectionType direction)
{
  gboolean return_val;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  if (!_ctk_widget_get_visible (widget) ||
      !ctk_widget_is_sensitive (widget))
    return FALSE;

  /* child widgets must set CAN_FOCUS, containers
   * don't have to though.
   */
  if (!CTK_IS_CONTAINER (widget) &&
      !ctk_widget_get_can_focus (widget))
    return FALSE;

  g_signal_emit (widget,
		 widget_signals[FOCUS],
		 0,
		 direction, &return_val);

  return return_val;
}

/**
 * ctk_widget_keynav_failed:
 * @widget: a #CtkWidget
 * @direction: direction of focus movement
 *
 * This function should be called whenever keyboard navigation within
 * a single widget hits a boundary. The function emits the
 * #CtkWidget::keynav-failed signal on the widget and its return
 * value should be interpreted in a way similar to the return value of
 * ctk_widget_child_focus():
 *
 * When %TRUE is returned, stay in the widget, the failed keyboard
 * navigation is OK and/or there is nowhere we can/should move the
 * focus to.
 *
 * When %FALSE is returned, the caller should continue with keyboard
 * navigation outside the widget, e.g. by calling
 * ctk_widget_child_focus() on the widget’s toplevel.
 *
 * The default ::keynav-failed handler returns %FALSE for
 * %CTK_DIR_TAB_FORWARD and %CTK_DIR_TAB_BACKWARD. For the other
 * values of #CtkDirectionType it returns %TRUE.
 *
 * Whenever the default handler returns %TRUE, it also calls
 * ctk_widget_error_bell() to notify the user of the failed keyboard
 * navigation.
 *
 * A use case for providing an own implementation of ::keynav-failed
 * (either by connecting to it or by overriding it) would be a row of
 * #CtkEntry widgets where the user should be able to navigate the
 * entire row with the cursor keys, as e.g. known from user interfaces
 * that require entering license keys.
 *
 * Returns: %TRUE if stopping keyboard navigation is fine, %FALSE
 *               if the emitting widget should try to handle the keyboard
 *               navigation attempt in its parent container(s).
 *
 * Since: 2.12
 **/
gboolean
ctk_widget_keynav_failed (CtkWidget        *widget,
                          CtkDirectionType  direction)
{
  gboolean return_val;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  g_signal_emit (widget, widget_signals[KEYNAV_FAILED], 0,
		 direction, &return_val);

  return return_val;
}

/**
 * ctk_widget_error_bell:
 * @widget: a #CtkWidget
 *
 * Notifies the user about an input-related error on this widget.
 * If the #CtkSettings:ctk-error-bell setting is %TRUE, it calls
 * cdk_window_beep(), otherwise it does nothing.
 *
 * Note that the effect of cdk_window_beep() can be configured in many
 * ways, depending on the windowing backend and the desktop environment
 * or window manager that is used.
 *
 * Since: 2.12
 **/
void
ctk_widget_error_bell (CtkWidget *widget)
{
  CtkWidgetPrivate *priv;
  CtkSettings* settings;
  gboolean beep;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = widget->priv;

  settings = ctk_widget_get_settings (widget);
  if (!settings)
    return;

  g_object_get (settings,
                "ctk-error-bell", &beep,
                NULL);

  if (beep && priv->window)
    cdk_window_beep (priv->window);
}

static void
ctk_widget_set_usize_internal (CtkWidget          *widget,
			       gint                width,
			       gint                height)
{
  CtkWidgetPrivate *priv = widget->priv;
  gboolean changed = FALSE;

  g_object_freeze_notify (G_OBJECT (widget));

  if (width > -2 && priv->width != width)
    {
      g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_WIDTH_REQUEST]);
      priv->width = width;
      changed = TRUE;
    }
  if (height > -2 && priv->height != height)
    {
      g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_HEIGHT_REQUEST]);
      priv->height = height;
      changed = TRUE;
    }

  if (_ctk_widget_get_visible (widget) && changed)
    {
      ctk_widget_queue_resize (widget);
    }

  g_object_thaw_notify (G_OBJECT (widget));
}

/**
 * ctk_widget_set_size_request:
 * @widget: a #CtkWidget
 * @width: width @widget should request, or -1 to unset
 * @height: height @widget should request, or -1 to unset
 *
 * Sets the minimum size of a widget; that is, the widget’s size
 * request will be at least @width by @height. You can use this 
 * function to force a widget to be larger than it normally would be.
 *
 * In most cases, ctk_window_set_default_size() is a better choice for
 * toplevel windows than this function; setting the default size will
 * still allow users to shrink the window. Setting the size request
 * will force them to leave the window at least as large as the size
 * request. When dealing with window sizes,
 * ctk_window_set_geometry_hints() can be a useful function as well.
 *
 * Note the inherent danger of setting any fixed size - themes,
 * translations into other languages, different fonts, and user action
 * can all change the appropriate size for a given widget. So, it's
 * basically impossible to hardcode a size that will always be
 * correct.
 *
 * The size request of a widget is the smallest size a widget can
 * accept while still functioning well and drawing itself correctly.
 * However in some strange cases a widget may be allocated less than
 * its requested size, and in many cases a widget may be allocated more
 * space than it requested.
 *
 * If the size request in a given direction is -1 (unset), then
 * the “natural” size request of the widget will be used instead.
 *
 * The size request set here does not include any margin from the
 * #CtkWidget properties margin-left, margin-right, margin-top, and
 * margin-bottom, but it does include pretty much all other padding
 * or border properties set by any subclass of #CtkWidget.
 **/
void
ctk_widget_set_size_request (CtkWidget *widget,
                             gint       width,
                             gint       height)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (width >= -1);
  g_return_if_fail (height >= -1);

  if (width == 0)
    width = 1;
  if (height == 0)
    height = 1;

  ctk_widget_set_usize_internal (widget, width, height);
}


/**
 * ctk_widget_get_size_request:
 * @widget: a #CtkWidget
 * @width: (out) (allow-none): return location for width, or %NULL
 * @height: (out) (allow-none): return location for height, or %NULL
 *
 * Gets the size request that was explicitly set for the widget using
 * ctk_widget_set_size_request(). A value of -1 stored in @width or
 * @height indicates that that dimension has not been set explicitly
 * and the natural requisition of the widget will be used instead. See
 * ctk_widget_set_size_request(). To get the size a widget will
 * actually request, call ctk_widget_get_preferred_size() instead of
 * this function.
 **/
void
ctk_widget_get_size_request (CtkWidget *widget,
                             gint      *width,
                             gint      *height)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (width)
    *width = widget->priv->width;

  if (height)
    *height = widget->priv->height;
}

/*< private >
 * ctk_widget_has_size_request:
 * @widget: a #CtkWidget
 *
 * Returns if the widget has a size request set (anything besides -1 for height
 * or width)
 */
gboolean
ctk_widget_has_size_request (CtkWidget *widget)
{
  return !(widget->priv->width == -1 && widget->priv->height == -1);
}

/**
 * ctk_widget_set_events:
 * @widget: a #CtkWidget
 * @events: event mask
 *
 * Sets the event mask (see #CdkEventMask) for a widget. The event
 * mask determines which events a widget will receive. Keep in mind
 * that different widgets have different default event masks, and by
 * changing the event mask you may disrupt a widget’s functionality,
 * so be careful. This function must be called while a widget is
 * unrealized. Consider ctk_widget_add_events() for widgets that are
 * already realized, or if you want to preserve the existing event
 * mask. This function can’t be used with widgets that have no window.
 * (See ctk_widget_get_has_window()).  To get events on those widgets,
 * place them inside a #CtkEventBox and receive events on the event
 * box.
 **/
void
ctk_widget_set_events (CtkWidget *widget,
		       gint	  events)
{
  gint e;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (!_ctk_widget_get_realized (widget));

  e = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (widget), quark_event_mask));
  if (e != events)
    {
      g_object_set_qdata (G_OBJECT (widget), quark_event_mask,
                          GINT_TO_POINTER (events));
      g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_EVENTS]);
    }
}

/**
 * ctk_widget_set_device_events:
 * @widget: a #CtkWidget
 * @device: a #CdkDevice
 * @events: event mask
 *
 * Sets the device event mask (see #CdkEventMask) for a widget. The event
 * mask determines which events a widget will receive from @device. Keep
 * in mind that different widgets have different default event masks, and by
 * changing the event mask you may disrupt a widget’s functionality,
 * so be careful. This function must be called while a widget is
 * unrealized. Consider ctk_widget_add_device_events() for widgets that are
 * already realized, or if you want to preserve the existing event
 * mask. This function can’t be used with windowless widgets (which return
 * %FALSE from ctk_widget_get_has_window());
 * to get events on those widgets, place them inside a #CtkEventBox
 * and receive events on the event box.
 *
 * Since: 3.0
 **/
void
ctk_widget_set_device_events (CtkWidget    *widget,
                              CdkDevice    *device,
                              CdkEventMask  events)
{
  GHashTable *device_events;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CDK_IS_DEVICE (device));
  g_return_if_fail (!_ctk_widget_get_realized (widget));

  device_events = g_object_get_qdata (G_OBJECT (widget), quark_device_event_mask);

  if (G_UNLIKELY (!device_events))
    {
      device_events = g_hash_table_new (NULL, NULL);
      g_object_set_qdata_full (G_OBJECT (widget), quark_device_event_mask, device_events,
                               (GDestroyNotify) g_hash_table_unref);
    }

  g_hash_table_insert (device_events, device, GUINT_TO_POINTER (events));
}

/**
 * ctk_widget_set_device_enabled:
 * @widget: a #CtkWidget
 * @device: a #CdkDevice
 * @enabled: whether to enable the device
 *
 * Enables or disables a #CdkDevice to interact with @widget
 * and all its children.
 *
 * It does so by descending through the #CdkWindow hierarchy
 * and enabling the same mask that is has for core events
 * (i.e. the one that cdk_window_get_events() returns).
 *
 * Since: 3.0
 */
void
ctk_widget_set_device_enabled (CtkWidget *widget,
                               CdkDevice *device,
                               gboolean   enabled)
{
  GList *enabled_devices;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CDK_IS_DEVICE (device));

  enabled_devices = g_object_get_qdata (G_OBJECT (widget), quark_enabled_devices);
  enabled_devices = g_list_append (enabled_devices, device);

  g_object_set_qdata_full (G_OBJECT (widget), quark_enabled_devices,
                           enabled_devices, (GDestroyNotify) g_list_free);;

  if (_ctk_widget_get_realized (widget))
    ctk_widget_set_device_enabled_internal (widget, device, TRUE, enabled);
}

/**
 * ctk_widget_get_device_enabled:
 * @widget: a #CtkWidget
 * @device: a #CdkDevice
 *
 * Returns whether @device can interact with @widget and its
 * children. See ctk_widget_set_device_enabled().
 *
 * Returns: %TRUE is @device is enabled for @widget
 *
 * Since: 3.0
 */
gboolean
ctk_widget_get_device_enabled (CtkWidget *widget,
                               CdkDevice *device)
{
  GList *enabled_devices;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (CDK_IS_DEVICE (device), FALSE);

  enabled_devices = g_object_get_qdata (G_OBJECT (widget), quark_enabled_devices);

  return g_list_find (enabled_devices, device) != NULL;
}

static void
ctk_widget_add_events_internal_list (CtkWidget    *widget,
                                     CdkDevice    *device,
                                     CdkEventMask  events,
                                     GList        *window_list)
{
  CdkEventMask controllers_mask;
  GList *l;

  controllers_mask = _ctk_widget_get_controllers_evmask (widget);

  for (l = window_list; l != NULL; l = l->next)
    {
      CdkWindow *window = l->data;
      CtkWidget *window_widget;

      cdk_window_get_user_data (window, (gpointer *)&window_widget);
      if (window_widget == widget)
        {
          GList *children;

          if (device)
            cdk_window_set_device_events (window, device,
                                          cdk_window_get_events (window)
                                          | events
                                          | controllers_mask);
          else
            cdk_window_set_events (window,
                                   cdk_window_get_events (window)
                                   | events
                                   | controllers_mask);

          children = cdk_window_peek_children (window);
          ctk_widget_add_events_internal_list (widget, device, events, children);
        }
    }
}

static void
ctk_widget_add_events_internal (CtkWidget *widget,
                                CdkDevice *device,
                                gint       events)
{
  CtkWidgetPrivate *priv = widget->priv;
  GList *window_list;
  GList win;

  if (!_ctk_widget_get_has_window (widget))
    window_list = cdk_window_peek_children (priv->window);
  else
    {
      win.data = priv->window;
      win.prev = win.next = NULL;
      window_list = &win;
    }

  ctk_widget_add_events_internal_list (widget, device, events, window_list);
}

/**
 * ctk_widget_add_events:
 * @widget: a #CtkWidget
 * @events: an event mask, see #CdkEventMask
 *
 * Adds the events in the bitfield @events to the event mask for
 * @widget. See ctk_widget_set_events() and the
 * [input handling overview][event-masks] for details.
 **/
void
ctk_widget_add_events (CtkWidget *widget,
		       gint	  events)
{
  gint old_events;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  old_events = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (widget), quark_event_mask));
  g_object_set_qdata (G_OBJECT (widget), quark_event_mask,
                      GINT_TO_POINTER (old_events | events));

  if (_ctk_widget_get_realized (widget))
    {
      ctk_widget_add_events_internal (widget, NULL, events);
      ctk_widget_update_devices_mask (widget, FALSE);
    }

  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_EVENTS]);
}

/**
 * ctk_widget_add_device_events:
 * @widget: a #CtkWidget
 * @device: a #CdkDevice
 * @events: an event mask, see #CdkEventMask
 *
 * Adds the device events in the bitfield @events to the event mask for
 * @widget. See ctk_widget_set_device_events() for details.
 *
 * Since: 3.0
 **/
void
ctk_widget_add_device_events (CtkWidget    *widget,
                              CdkDevice    *device,
                              CdkEventMask  events)
{
  CdkEventMask old_events;
  GHashTable *device_events;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CDK_IS_DEVICE (device));

  old_events = ctk_widget_get_device_events (widget, device);

  device_events = g_object_get_qdata (G_OBJECT (widget), quark_device_event_mask);

  if (G_UNLIKELY (!device_events))
    {
      device_events = g_hash_table_new (NULL, NULL);
      g_object_set_qdata_full (G_OBJECT (widget), quark_device_event_mask, device_events,
                               (GDestroyNotify) g_hash_table_unref);
    }

  g_hash_table_insert (device_events, device,
                       GUINT_TO_POINTER (old_events | events));

  if (_ctk_widget_get_realized (widget))
    ctk_widget_add_events_internal (widget, device, events);

  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_EVENTS]);
}

/**
 * ctk_widget_get_toplevel:
 * @widget: a #CtkWidget
 *
 * This function returns the topmost widget in the container hierarchy
 * @widget is a part of. If @widget has no parent widgets, it will be
 * returned as the topmost widget. No reference will be added to the
 * returned widget; it should not be unreferenced.
 *
 * Note the difference in behavior vs. ctk_widget_get_ancestor();
 * `ctk_widget_get_ancestor (widget, CTK_TYPE_WINDOW)`
 * would return
 * %NULL if @widget wasn’t inside a toplevel window, and if the
 * window was inside a #CtkWindow-derived widget which was in turn
 * inside the toplevel #CtkWindow. While the second case may
 * seem unlikely, it actually happens when a #CtkPlug is embedded
 * inside a #CtkSocket within the same application.
 *
 * To reliably find the toplevel #CtkWindow, use
 * ctk_widget_get_toplevel() and call CTK_IS_WINDOW()
 * on the result. For instance, to get the title of a widget's toplevel
 * window, one might use:
 * |[<!-- language="C" -->
 * static const char *
 * get_widget_toplevel_title (CtkWidget *widget)
 * {
 *   CtkWidget *toplevel = ctk_widget_get_toplevel (widget);
 *   if (CTK_IS_WINDOW (toplevel))
 *     {
 *       return ctk_window_get_title (CTK_WINDOW (toplevel));
 *     }
 *
 *   return NULL;
 * }
 * ]|
 *
 * Returns: (transfer none): the topmost ancestor of @widget, or @widget itself
 *    if there’s no ancestor.
 **/
CtkWidget*
ctk_widget_get_toplevel (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  while (widget->priv->parent)
    widget = widget->priv->parent;

  return widget;
}

/**
 * ctk_widget_get_ancestor:
 * @widget: a #CtkWidget
 * @widget_type: ancestor type
 *
 * Gets the first ancestor of @widget with type @widget_type. For example,
 * `ctk_widget_get_ancestor (widget, CTK_TYPE_BOX)` gets
 * the first #CtkBox that’s an ancestor of @widget. No reference will be
 * added to the returned widget; it should not be unreferenced. See note
 * about checking for a toplevel #CtkWindow in the docs for
 * ctk_widget_get_toplevel().
 *
 * Note that unlike ctk_widget_is_ancestor(), ctk_widget_get_ancestor()
 * considers @widget to be an ancestor of itself.
 *
 * Returns: (transfer none) (nullable): the ancestor widget, or %NULL if not found
 **/
CtkWidget*
ctk_widget_get_ancestor (CtkWidget *widget,
			 GType      widget_type)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  while (widget && !g_type_is_a (G_OBJECT_TYPE (widget), widget_type))
    widget = widget->priv->parent;

  if (!(widget && g_type_is_a (G_OBJECT_TYPE (widget), widget_type)))
    return NULL;

  return widget;
}

/**
 * ctk_widget_set_visual:
 * @widget: a #CtkWidget
 * @visual: (allow-none): visual to be used or %NULL to unset a previous one
 *
 * Sets the visual that should be used for by widget and its children for
 * creating #CdkWindows. The visual must be on the same #CdkScreen as
 * returned by ctk_widget_get_screen(), so handling the
 * #CtkWidget::screen-changed signal is necessary.
 *
 * Setting a new @visual will not cause @widget to recreate its windows,
 * so you should call this function before @widget is realized.
 **/
void
ctk_widget_set_visual (CtkWidget *widget,
                       CdkVisual *visual)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (visual == NULL || CDK_IS_VISUAL (visual));

  if (visual)
    g_return_if_fail (ctk_widget_get_screen (widget) == cdk_visual_get_screen (visual));

  g_object_set_qdata_full (G_OBJECT (widget),
                           quark_visual,
                           visual ? g_object_ref (visual) : NULL,
                           g_object_unref);
}

/**
 * ctk_widget_get_visual:
 * @widget: a #CtkWidget
 *
 * Gets the visual that will be used to render @widget.
 *
 * Returns: (transfer none): the visual for @widget
 **/
CdkVisual*
ctk_widget_get_visual (CtkWidget *widget)
{
  CtkWidget *w;
  CdkVisual *visual;
  CdkScreen *screen;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  if (!_ctk_widget_get_has_window (widget) &&
      widget->priv->window)
    return cdk_window_get_visual (widget->priv->window);

  screen = ctk_widget_get_screen (widget);

  for (w = widget; w != NULL; w = w->priv->parent)
    {
      visual = g_object_get_qdata (G_OBJECT (w), quark_visual);
      if (visual)
        {
          if (cdk_visual_get_screen (visual) == screen)
            return visual;

          g_warning ("Ignoring visual set on widget '%s' that is not on the correct screen.",
                     ctk_widget_get_name (widget));
        }
    }

  return cdk_screen_get_system_visual (screen);
}

/**
 * ctk_widget_get_settings:
 * @widget: a #CtkWidget
 *
 * Gets the settings object holding the settings used for this widget.
 *
 * Note that this function can only be called when the #CtkWidget
 * is attached to a toplevel, since the settings object is specific
 * to a particular #CdkScreen.
 *
 * Returns: (transfer none): the relevant #CtkSettings object
 */
CtkSettings*
ctk_widget_get_settings (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return ctk_settings_get_for_screen (ctk_widget_get_screen (widget));
}

/**
 * ctk_widget_get_events:
 * @widget: a #CtkWidget
 *
 * Returns the event mask (see #CdkEventMask) for the widget. These are the
 * events that the widget will receive.
 *
 * Note: Internally, the widget event mask will be the logical OR of the event
 * mask set through ctk_widget_set_events() or ctk_widget_add_events(), and the
 * event mask necessary to cater for every #CtkEventController created for the
 * widget.
 *
 * Returns: event mask for @widget
 **/
gint
ctk_widget_get_events (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);

  return GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (widget), quark_event_mask)) |
    _ctk_widget_get_controllers_evmask (widget);
}

/**
 * ctk_widget_get_device_events:
 * @widget: a #CtkWidget
 * @device: a #CdkDevice
 *
 * Returns the events mask for the widget corresponding to an specific device. These
 * are the events that the widget will receive when @device operates on it.
 *
 * Returns: device event mask for @widget
 *
 * Since: 3.0
 **/
CdkEventMask
ctk_widget_get_device_events (CtkWidget *widget,
                              CdkDevice *device)
{
  GHashTable *device_events;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);
  g_return_val_if_fail (CDK_IS_DEVICE (device), 0);

  device_events = g_object_get_qdata (G_OBJECT (widget), quark_device_event_mask);

  if (!device_events)
    return 0;

  return GPOINTER_TO_UINT (g_hash_table_lookup (device_events, device));
}

/**
 * ctk_widget_get_pointer:
 * @widget: a #CtkWidget
 * @x: (out) (allow-none): return location for the X coordinate, or %NULL
 * @y: (out) (allow-none): return location for the Y coordinate, or %NULL
 *
 * Obtains the location of the mouse pointer in widget coordinates.
 * Widget coordinates are a bit odd; for historical reasons, they are
 * defined as @widget->window coordinates for widgets that return %TRUE for
 * ctk_widget_get_has_window(); and are relative to @widget->allocation.x,
 * @widget->allocation.y otherwise.
 *
 * Deprecated: 3.4: Use cdk_window_get_device_position() instead.
 **/
void
ctk_widget_get_pointer (CtkWidget *widget,
			gint	  *x,
			gint	  *y)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = widget->priv;

  if (x)
    *x = -1;
  if (y)
    *y = -1;

  if (_ctk_widget_get_realized (widget))
    {
      CdkSeat *seat;

      seat = cdk_display_get_default_seat (ctk_widget_get_display (widget));
      cdk_window_get_device_position (priv->window,
                                      cdk_seat_get_pointer (seat),
                                      x, y, NULL);

      if (!_ctk_widget_get_has_window (widget))
	{
	  if (x)
	    *x -= priv->allocation.x;
	  if (y)
	    *y -= priv->allocation.y;
	}
    }
}

/**
 * ctk_widget_is_ancestor:
 * @widget: a #CtkWidget
 * @ancestor: another #CtkWidget
 *
 * Determines whether @widget is somewhere inside @ancestor, possibly with
 * intermediate containers.
 *
 * Returns: %TRUE if @ancestor contains @widget as a child,
 *    grandchild, great grandchild, etc.
 **/
gboolean
ctk_widget_is_ancestor (CtkWidget *widget,
			CtkWidget *ancestor)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (ancestor != NULL, FALSE);

  while (widget)
    {
      if (widget->priv->parent == ancestor)
	return TRUE;
      widget = widget->priv->parent;
    }

  return FALSE;
}

static GQuark quark_composite_name = 0;

/**
 * ctk_widget_set_composite_name:
 * @widget: a #CtkWidget.
 * @name: the name to set
 *
 * Sets a widgets composite name. The widget must be
 * a composite child of its parent; see ctk_widget_push_composite_child().
 *
 * Deprecated: 3.10: Use ctk_widget_class_set_template(), or don’t use this API at all.
 **/
void
ctk_widget_set_composite_name (CtkWidget   *widget,
			       const gchar *name)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (widget->priv->composite_child);
  g_return_if_fail (name != NULL);

  if (!quark_composite_name)
    quark_composite_name = g_quark_from_static_string ("ctk-composite-name");

  g_object_set_qdata_full (G_OBJECT (widget),
			   quark_composite_name,
			   g_strdup (name),
			   g_free);
}

/**
 * ctk_widget_get_composite_name:
 * @widget: a #CtkWidget
 *
 * Obtains the composite name of a widget.
 *
 * Returns: the composite name of @widget, or %NULL if @widget is not
 *   a composite child. The string should be freed when it is no
 *   longer needed.
 *
 * Deprecated: 3.10: Use ctk_widget_class_set_template(), or don’t use this API at all.
 **/
gchar*
ctk_widget_get_composite_name (CtkWidget *widget)
{
  CtkWidgetPrivate *priv;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  priv = widget->priv;

  if (widget->priv->composite_child && priv->parent)
    return _ctk_container_child_composite_name (CTK_CONTAINER (priv->parent),
					       widget);
  else
    return NULL;
}

/**
 * ctk_widget_push_composite_child:
 *
 * Makes all newly-created widgets as composite children until
 * the corresponding ctk_widget_pop_composite_child() call.
 *
 * A composite child is a child that’s an implementation detail of the
 * container it’s inside and should not be visible to people using the
 * container. Composite children aren’t treated differently by CTK+ (but
 * see ctk_container_foreach() vs. ctk_container_forall()), but e.g. GUI
 * builders might want to treat them in a different way.
 *
 * Deprecated: 3.10: This API never really worked well and was mostly unused, now
 * we have a more complete mechanism for composite children, see ctk_widget_class_set_template().
 **/
void
ctk_widget_push_composite_child (void)
{
  composite_child_stack++;
}

/**
 * ctk_widget_pop_composite_child:
 *
 * Cancels the effect of a previous call to ctk_widget_push_composite_child().
 *
 * Deprecated: 3.10: Use ctk_widget_class_set_template(), or don’t use this API at all.
 **/
void
ctk_widget_pop_composite_child (void)
{
  if (composite_child_stack)
    composite_child_stack--;
}

static void
ctk_widget_emit_direction_changed (CtkWidget        *widget,
                                   CtkTextDirection  old_dir)
{
  CtkTextDirection direction;
  CtkStateFlags state;

  ctk_widget_update_pango_context (widget);

  direction = _ctk_widget_get_direction (widget);

  switch (direction)
    {
    case CTK_TEXT_DIR_LTR:
      state = CTK_STATE_FLAG_DIR_LTR;
      break;

    case CTK_TEXT_DIR_RTL:
      state = CTK_STATE_FLAG_DIR_RTL;
      break;

    case CTK_TEXT_DIR_NONE:
    default:
      g_assert_not_reached ();
      break;
    }

  ctk_widget_update_state_flags (widget,
                                 state,
                                 state ^ (CTK_STATE_FLAG_DIR_LTR | CTK_STATE_FLAG_DIR_RTL));

  g_signal_emit (widget, widget_signals[DIRECTION_CHANGED], 0, old_dir);
}

/**
 * ctk_widget_set_direction:
 * @widget: a #CtkWidget
 * @dir:    the new direction
 *
 * Sets the reading direction on a particular widget. This direction
 * controls the primary direction for widgets containing text,
 * and also the direction in which the children of a container are
 * packed. The ability to set the direction is present in order
 * so that correct localization into languages with right-to-left
 * reading directions can be done. Generally, applications will
 * let the default reading direction present, except for containers
 * where the containers are arranged in an order that is explicitly
 * visual rather than logical (such as buttons for text justification).
 *
 * If the direction is set to %CTK_TEXT_DIR_NONE, then the value
 * set by ctk_widget_set_default_direction() will be used.
 **/
void
ctk_widget_set_direction (CtkWidget        *widget,
                          CtkTextDirection  dir)
{
  CtkTextDirection old_dir;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (dir >= CTK_TEXT_DIR_NONE && dir <= CTK_TEXT_DIR_RTL);

  old_dir = _ctk_widget_get_direction (widget);

  widget->priv->direction = dir;

  if (old_dir != _ctk_widget_get_direction (widget))
    ctk_widget_emit_direction_changed (widget, old_dir);
}

/**
 * ctk_widget_get_direction:
 * @widget: a #CtkWidget
 *
 * Gets the reading direction for a particular widget. See
 * ctk_widget_set_direction().
 *
 * Returns: the reading direction for the widget.
 **/
CtkTextDirection
ctk_widget_get_direction (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), CTK_TEXT_DIR_LTR);

  if (widget->priv->direction == CTK_TEXT_DIR_NONE)
    return ctk_default_direction;
  else
    return widget->priv->direction;
}

static void
ctk_widget_set_default_direction_recurse (CtkWidget *widget, gpointer data)
{
  CtkTextDirection old_dir = GPOINTER_TO_UINT (data);

  g_object_ref (widget);

  if (widget->priv->direction == CTK_TEXT_DIR_NONE)
    ctk_widget_emit_direction_changed (widget, old_dir);

  if (CTK_IS_CONTAINER (widget))
    ctk_container_forall (CTK_CONTAINER (widget),
			  ctk_widget_set_default_direction_recurse,
			  data);

  g_object_unref (widget);
}

/**
 * ctk_widget_set_default_direction:
 * @dir: the new default direction. This cannot be
 *        %CTK_TEXT_DIR_NONE.
 *
 * Sets the default reading direction for widgets where the
 * direction has not been explicitly set by ctk_widget_set_direction().
 **/
void
ctk_widget_set_default_direction (CtkTextDirection dir)
{
  g_return_if_fail (dir == CTK_TEXT_DIR_RTL || dir == CTK_TEXT_DIR_LTR);

  if (dir != ctk_default_direction)
    {
      GList *toplevels, *tmp_list;
      CtkTextDirection old_dir = ctk_default_direction;

      ctk_default_direction = dir;

      tmp_list = toplevels = ctk_window_list_toplevels ();
      g_list_foreach (toplevels, (GFunc)g_object_ref, NULL);

      while (tmp_list)
	{
	  ctk_widget_set_default_direction_recurse (tmp_list->data,
						    GUINT_TO_POINTER (old_dir));
	  g_object_unref (tmp_list->data);
	  tmp_list = tmp_list->next;
	}

      g_list_free (toplevels);
    }
}

/**
 * ctk_widget_get_default_direction:
 *
 * Obtains the current default reading direction. See
 * ctk_widget_set_default_direction().
 *
 * Returns: the current default direction.
 **/
CtkTextDirection
ctk_widget_get_default_direction (void)
{
  return ctk_default_direction;
}

static void
ctk_widget_constructed (GObject *object)
{
  CtkWidget *widget = CTK_WIDGET (object);
  CtkWidgetPath *path;

  /* As strange as it may seem, this may happen on object construction.
   * init() implementations of parent types may eventually call this function,
   * each with its corresponding GType, which could leave a child
   * implementation with a wrong widget type in the widget path
   */
  path = (CtkWidgetPath*)g_object_get_qdata (object, quark_widget_path);
  if (path && G_OBJECT_TYPE (widget) != ctk_widget_path_get_object_type (path))
    g_object_set_qdata (object, quark_widget_path, NULL);

  G_OBJECT_CLASS (ctk_widget_parent_class)->constructed (object);
}

static void
ctk_widget_dispose (GObject *object)
{
  CtkWidget *widget = CTK_WIDGET (object);
  CtkWidgetPrivate *priv = widget->priv;
  GSList *sizegroups;

  if (priv->parent)
    ctk_container_remove (CTK_CONTAINER (priv->parent), widget);
  else if (_ctk_widget_get_visible (widget))
    ctk_widget_hide (widget);

  priv->visible = FALSE;
  if (_ctk_widget_get_realized (widget))
    ctk_widget_unrealize (widget);

  if (!priv->in_destruction)
    {
      priv->in_destruction = TRUE;
      g_signal_emit (object, widget_signals[DESTROY], 0);
      priv->in_destruction = FALSE;
    }

  sizegroups = _ctk_widget_get_sizegroups (widget);
  while (sizegroups)
    {
      CtkSizeGroup *size_group;

      size_group = sizegroups->data;
      sizegroups = sizegroups->next;
      ctk_size_group_remove_widget (size_group, widget);
    }

  g_object_set_qdata (object, quark_action_muxer, NULL);

  while (priv->attached_windows)
    ctk_window_set_attached_to (priv->attached_windows->data, NULL);

  G_OBJECT_CLASS (ctk_widget_parent_class)->dispose (object);
}

#ifdef G_ENABLE_CONSISTENCY_CHECKS
typedef struct {
  AutomaticChildClass *child_class;
  GType                widget_type;
  GObject             *object;
  gboolean             did_finalize;
} FinalizeAssertion;

static void
finalize_assertion_weak_ref (gpointer data,
			     GObject *where_the_object_was)
{
  FinalizeAssertion *assertion = (FinalizeAssertion *)data;
  assertion->did_finalize = TRUE;
}

static FinalizeAssertion *
finalize_assertion_new (CtkWidget           *widget,
			GType                widget_type,
			AutomaticChildClass *child_class)
{
  FinalizeAssertion *assertion = NULL;
  GObject           *object;

  object = ctk_widget_get_template_child (widget, widget_type, child_class->name);

  /* We control the hash table entry, the object should never be NULL
   */
  g_assert (object);
  if (!G_IS_OBJECT (object))
    g_critical ("Automated component '%s' of class '%s' seems to have been prematurely finalized",
		child_class->name, g_type_name (widget_type));
  else
    {
      assertion = g_slice_new0 (FinalizeAssertion);
      assertion->child_class = child_class;
      assertion->widget_type = widget_type;
      assertion->object = object;

      g_object_weak_ref (object, finalize_assertion_weak_ref, assertion);
    }

  return assertion;
}

static GSList *
build_finalize_assertion_list (CtkWidget *widget)
{
  GType class_type;
  CtkWidgetClass *class;
  GSList *l, *list = NULL;

  for (class = CTK_WIDGET_GET_CLASS (widget);
       CTK_IS_WIDGET_CLASS (class);
       class = g_type_class_peek_parent (class))
    {
      if (!class->priv->template)
	continue;

      class_type = G_OBJECT_CLASS_TYPE (class);

      for (l = class->priv->template->children; l; l = l->next)
	{
	  AutomaticChildClass *child_class = l->data;
	  FinalizeAssertion *assertion;

	  assertion = finalize_assertion_new (widget, class_type, child_class);
	  list = g_slist_prepend (list, assertion);
	}
    }

  return list;
}
#endif /* G_ENABLE_CONSISTENCY_CHECKS */

static void
ctk_widget_real_destroy (CtkWidget *object)
{
  /* ctk_object_destroy() will already hold a refcount on object */
  CtkWidget *widget = CTK_WIDGET (object);
  CtkWidgetPrivate *priv = widget->priv;

  if (g_object_get_qdata (G_OBJECT (widget), quark_auto_children))
    {
      CtkWidgetClass *class;
      GSList *l;

#ifdef G_ENABLE_CONSISTENCY_CHECKS
      GSList *assertions = NULL;

      /* Note, CTK_WIDGET_ASSERT_COMPONENTS is very useful
       * to catch ref counting bugs, but can only be used in
       * test cases which simply create and destroy a composite
       * widget.
       *
       * This is because some API can expose components explicitly,
       * and so we cannot assert that a component is expected to finalize
       * in a full application ecosystem.
       */
      if (g_getenv ("CTK_WIDGET_ASSERT_COMPONENTS") != NULL)
	assertions = build_finalize_assertion_list (widget);
#endif /* G_ENABLE_CONSISTENCY_CHECKS */

      /* Release references to all automated children */
      g_object_set_qdata (G_OBJECT (widget), quark_auto_children, NULL);

#ifdef G_ENABLE_CONSISTENCY_CHECKS
      for (l = assertions; l; l = l->next)
	{
	  FinalizeAssertion *assertion = l->data;

	  if (!assertion->did_finalize)
	    g_critical ("Automated component '%s' of class '%s' did not finalize in ctk_widget_destroy(). "
			"Current reference count is %d",
			assertion->child_class->name,
			g_type_name (assertion->widget_type),
			assertion->object->ref_count);

	  g_slice_free (FinalizeAssertion, assertion);
	}
      g_slist_free (assertions);
#endif /* G_ENABLE_CONSISTENCY_CHECKS */

      /* Set any automatic private data pointers to NULL */
      for (class = CTK_WIDGET_GET_CLASS (widget);
	   CTK_IS_WIDGET_CLASS (class);
	   class = g_type_class_peek_parent (class))
	{
	  if (!class->priv->template)
	    continue;

	  for (l = class->priv->template->children; l; l = l->next)
	    {
	      AutomaticChildClass *child_class = l->data;

	      if (child_class->offset != 0)
		{
		  gpointer field_p;

		  /* Nullify instance private data for internal children */
		  field_p = G_STRUCT_MEMBER_P (widget, child_class->offset);
		  (* (gpointer *) field_p) = NULL;
		}
	    }
	}
    }

  if (priv->accessible)
    {
      ctk_accessible_set_widget (CTK_ACCESSIBLE (priv->accessible), NULL);
      g_object_unref (priv->accessible);
      priv->accessible = NULL;
    }

  /* wipe accelerator closures (keep order) */
  g_object_set_qdata (G_OBJECT (widget), quark_accel_path, NULL);
  g_object_set_qdata (G_OBJECT (widget), quark_accel_closures, NULL);

  /* Callers of add_mnemonic_label() should disconnect on ::destroy */
  g_object_set_qdata (G_OBJECT (widget), quark_mnemonic_labels, NULL);

  ctk_grab_remove (widget);

  destroy_tick_callbacks (widget);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  if (priv->style)
    g_object_unref (priv->style);
  priv->style = ctk_widget_get_default_style ();
  g_object_ref (priv->style);
  G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
ctk_widget_finalize (GObject *object)
{
  CtkWidget *widget = CTK_WIDGET (object);
  CtkWidgetPrivate *priv = widget->priv;
  GList *l;

  ctk_grab_remove (widget);

  g_clear_object (&priv->style);

  g_free (priv->name);

  g_clear_object (&priv->accessible);

  ctk_widget_clear_path (widget);

  ctk_css_widget_node_widget_destroyed (CTK_CSS_WIDGET_NODE (priv->cssnode));
  g_object_unref (priv->cssnode);

  g_clear_object (&priv->context);

  _ctk_size_request_cache_free (&priv->requests);

  for (l = priv->event_controllers; l; l = l->next)
    {
      EventControllerData *data = l->data;
      if (data->controller)
        _ctk_widget_remove_controller (widget, data->controller);
    }
  g_list_free_full (priv->event_controllers, g_free);
  priv->event_controllers = NULL;

  if (g_object_is_floating (object))
    g_warning ("A floating object was finalized. This means that someone\n"
               "called g_object_unref() on an object that had only a floating\n"
               "reference; the initial floating reference is not owned by anyone\n"
               "and must be removed with g_object_ref_sink().");

  G_OBJECT_CLASS (ctk_widget_parent_class)->finalize (object);
}

/*****************************************
 * ctk_widget_real_map:
 *
 *   arguments:
 *
 *   results:
 *****************************************/

static void
ctk_widget_real_map (CtkWidget *widget)
{
  CtkWidgetPrivate *priv = widget->priv;

  g_assert (_ctk_widget_get_realized (widget));

  if (!_ctk_widget_get_mapped (widget))
    {
      ctk_widget_set_mapped (widget, TRUE);

      if (_ctk_widget_get_has_window (widget))
	cdk_window_show (priv->window);
    }
}

/*****************************************
 * ctk_widget_real_unmap:
 *
 *   arguments:
 *
 *   results:
 *****************************************/

static void
ctk_widget_real_unmap (CtkWidget *widget)
{
  CtkWidgetPrivate *priv = widget->priv;

  if (_ctk_widget_get_mapped (widget))
    {
      ctk_widget_set_mapped (widget, FALSE);

      if (_ctk_widget_get_has_window (widget))
	cdk_window_hide (priv->window);
    }
}

/*****************************************
 * ctk_widget_real_realize:
 *
 *   arguments:
 *
 *   results:
 *****************************************/

static void
ctk_widget_real_realize (CtkWidget *widget)
{
  CtkWidgetPrivate *priv = widget->priv;

  g_assert (!_ctk_widget_get_has_window (widget));

  ctk_widget_set_realized (widget, TRUE);
  if (priv->parent)
    {
      priv->window = ctk_widget_get_parent_window (widget);
      g_object_ref (priv->window);
    }
}

/*****************************************
 * ctk_widget_real_unrealize:
 *
 *   arguments:
 *
 *   results:
 *****************************************/

static void
ctk_widget_real_unrealize (CtkWidget *widget)
{
  CtkWidgetPrivate *priv = widget->priv;

  g_assert (!widget->priv->mapped);

   /* We must do unrealize child widget BEFORE container widget.
    * cdk_window_destroy() destroys specified xwindow and its sub-xwindows.
    * So, unrealizing container widget before its children causes the problem
    * (for example, cdk_ic_destroy () with destroyed window causes crash.)
    */

  if (CTK_IS_CONTAINER (widget))
    ctk_container_forall (CTK_CONTAINER (widget),
			  (CtkCallback) ctk_widget_unrealize,
			  NULL);

  if (_ctk_widget_get_has_window (widget))
    {
      ctk_widget_unregister_window (widget, priv->window);
      cdk_window_destroy (priv->window);
      priv->window = NULL;
    }
  else
    {
      g_object_unref (priv->window);
      priv->window = NULL;
    }

  ctk_selection_remove_all (widget);

  ctk_widget_set_realized (widget, FALSE);
}

static void
ctk_widget_real_adjust_size_request (CtkWidget      *widget,
                                     CtkOrientation  orientation,
                                     gint           *minimum_size,
                                     gint           *natural_size)
{
  CtkWidgetPrivate *priv = widget->priv;

  if (orientation == CTK_ORIENTATION_HORIZONTAL && priv->width > 0)
    *minimum_size = MAX (*minimum_size, priv->width);
  else if (orientation == CTK_ORIENTATION_VERTICAL && priv->height > 0)
    *minimum_size = MAX (*minimum_size, priv->height);

  /* Fix it if set_size_request made natural size smaller than min size.
   * This would also silently fix broken widgets, but we warn about them
   * in ctksizerequest.c when calling their size request vfuncs.
   */
  *natural_size = MAX (*natural_size, *minimum_size);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      *minimum_size += priv->margin.left + priv->margin.right;
      *natural_size += priv->margin.left + priv->margin.right;
    }
  else
    {
      *minimum_size += priv->margin.top + priv->margin.bottom;
      *natural_size += priv->margin.top + priv->margin.bottom;
    }
}

static void
ctk_widget_real_adjust_baseline_request (CtkWidget *widget,
					 gint      *minimum_baseline,
					 gint      *natural_baseline)
{
  CtkWidgetPrivate *priv = widget->priv;

  if (priv->height >= 0)
    {
      /* No baseline support for explicitly set height */
      *minimum_baseline = -1;
      *natural_baseline = -1;
    }
  else
    {
      *minimum_baseline += priv->margin.top;
      *natural_baseline += priv->margin.top;
    }
}

static gboolean
is_my_window (CtkWidget *widget,
              CdkWindow *window)
{
  gpointer user_data;

  cdk_window_get_user_data (window, &user_data);
  return (user_data == widget);
}

/*
 * _ctk_widget_get_device_window:
 * @widget: a #CtkWidget
 * @device: a #CdkDevice
 *
 * Returns: (nullable): the window of @widget that @device is in, or %NULL
 */
CdkWindow *
_ctk_widget_get_device_window (CtkWidget *widget,
                               CdkDevice *device)
{
  CdkWindow *window;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (CDK_IS_DEVICE (device), NULL);

  if (cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD)
    return NULL;

  window = cdk_device_get_last_event_window (device);
  if (window && is_my_window (widget, window))
    return window;
  else
    return NULL;
}

static void
list_devices (CtkWidget        *widget,
              CdkDeviceManager *device_manager,
              CdkDeviceType     device_type,
              GList           **result)
{
  GList *devices;
  GList *l;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  devices = cdk_device_manager_list_devices (device_manager, device_type);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  for (l = devices; l; l = l->next)
    {
      CdkDevice *device = l->data;
      if (cdk_device_get_source (device) != CDK_SOURCE_KEYBOARD)
        {
          CdkWindow *window = cdk_device_get_last_event_window (device);
          if (window && is_my_window (widget, window))
            *result = g_list_prepend (*result, device);
        }
    }
  g_list_free (devices);
}

/*
 * _ctk_widget_list_devices:
 * @widget: a #CtkWidget
 *
 * Returns the list of #CdkDevices that is currently on top
 * of any window belonging to @widget.
 * Free the list with g_list_free(), the elements are owned
 * by CTK+ and must not be freed.
 */
GList *
_ctk_widget_list_devices (CtkWidget *widget)
{
  CdkDisplay *display;
  CdkDeviceManager *device_manager;
  GList *result = NULL;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  if (!_ctk_widget_get_mapped (widget))
    return NULL;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  display = ctk_widget_get_display (widget);
  device_manager = cdk_display_get_device_manager (display);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  list_devices (widget, device_manager, CDK_DEVICE_TYPE_MASTER, &result);
  /* Rare, but we can get events for grabbed slave devices */
  list_devices (widget, device_manager, CDK_DEVICE_TYPE_SLAVE, &result);

  return result;
}

static void
synth_crossing (CtkWidget       *widget,
                CdkEventType     type,
                CdkWindow       *window,
                CdkDevice       *device,
                CdkCrossingMode  mode,
                CdkNotifyType    detail)
{
  CdkEvent *event;

  event = cdk_event_new (type);

  event->crossing.window = g_object_ref (window);
  event->crossing.send_event = TRUE;
  event->crossing.subwindow = g_object_ref (window);
  event->crossing.time = CDK_CURRENT_TIME;
  cdk_device_get_position_double (device,
                                  NULL,
                                  &event->crossing.x_root,
                                  &event->crossing.y_root);
  cdk_window_get_device_position_double (window,
                                         device,
                                         &event->crossing.x,
                                         &event->crossing.y,
                                         NULL);
  event->crossing.mode = mode;
  event->crossing.detail = detail;
  event->crossing.focus = FALSE;
  event->crossing.state = 0;
  cdk_event_set_device (event, device);

  if (!widget)
    widget = ctk_get_event_widget (event);

  if (widget)
    ctk_widget_event_internal (widget, event);

  cdk_event_free (event);
}

/*
 * _ctk_widget_synthesize_crossing:
 * @from: the #CtkWidget the virtual pointer is leaving.
 * @to: the #CtkWidget the virtual pointer is moving to.
 * @mode: the #CdkCrossingMode to place on the synthesized events.
 *
 * Generate crossing event(s) on widget state (sensitivity) or CTK+ grab change.
 *
 * The real pointer window is the window that most recently received an enter notify
 * event.  Windows that don’t select for crossing events can’t become the real
 * pointer window.  The real pointer widget that owns the real pointer window.  The
 * effective pointer window is the same as the real pointer window unless the real
 * pointer widget is either insensitive or there is a grab on a widget that is not
 * an ancestor of the real pointer widget (in which case the effective pointer
 * window should be the root window).
 *
 * When the effective pointer window is the same as the real pointer window, we
 * receive crossing events from the windowing system.  When the effective pointer
 * window changes to become different from the real pointer window we synthesize
 * crossing events, attempting to follow X protocol rules:
 *
 * When the root window becomes the effective pointer window:
 *   - leave notify on real pointer window, detail Ancestor
 *   - leave notify on all of its ancestors, detail Virtual
 *   - enter notify on root window, detail Inferior
 *
 * When the root window ceases to be the effective pointer window:
 *   - leave notify on root window, detail Inferior
 *   - enter notify on all ancestors of real pointer window, detail Virtual
 *   - enter notify on real pointer window, detail Ancestor
 */
void
_ctk_widget_synthesize_crossing (CtkWidget       *from,
				 CtkWidget       *to,
                                 CdkDevice       *device,
				 CdkCrossingMode  mode)
{
  CdkWindow *from_window = NULL, *to_window = NULL;

  g_return_if_fail (from != NULL || to != NULL);

  if (from != NULL)
    {
      from_window = _ctk_widget_get_device_window (from, device);

      if (!from_window)
        from_window = from->priv->window;
    }

  if (to != NULL)
    {
      to_window = _ctk_widget_get_device_window (to, device);

      if (!to_window)
        to_window = to->priv->window;
    }

  if (from_window == NULL && to_window == NULL)
    ;
  else if (from_window != NULL && to_window == NULL)
    {
      GList *from_ancestors = NULL, *list;
      CdkWindow *from_ancestor = from_window;

      while (from_ancestor != NULL)
	{
	  from_ancestor = cdk_window_get_effective_parent (from_ancestor);
          if (from_ancestor == NULL)
            break;
          from_ancestors = g_list_prepend (from_ancestors, from_ancestor);
	}

      synth_crossing (from, CDK_LEAVE_NOTIFY, from_window,
		      device, mode, CDK_NOTIFY_ANCESTOR);
      for (list = g_list_last (from_ancestors); list; list = list->prev)
	{
	  synth_crossing (NULL, CDK_LEAVE_NOTIFY, (CdkWindow *) list->data,
			  device, mode, CDK_NOTIFY_VIRTUAL);
	}

      /* XXX: enter/inferior on root window? */

      g_list_free (from_ancestors);
    }
  else if (from_window == NULL && to_window != NULL)
    {
      GList *to_ancestors = NULL, *list;
      CdkWindow *to_ancestor = to_window;

      while (to_ancestor != NULL)
	{
	  to_ancestor = cdk_window_get_effective_parent (to_ancestor);
	  if (to_ancestor == NULL)
            break;
          to_ancestors = g_list_prepend (to_ancestors, to_ancestor);
        }

      /* XXX: leave/inferior on root window? */

      for (list = to_ancestors; list; list = list->next)
	{
	  synth_crossing (NULL, CDK_ENTER_NOTIFY, (CdkWindow *) list->data,
			  device, mode, CDK_NOTIFY_VIRTUAL);
	}
      synth_crossing (to, CDK_ENTER_NOTIFY, to_window,
		      device, mode, CDK_NOTIFY_ANCESTOR);

      g_list_free (to_ancestors);
    }
  else if (from_window == to_window)
    ;
  else
    {
      GList *from_ancestors = NULL, *to_ancestors = NULL, *list;
      CdkWindow *from_ancestor = from_window, *to_ancestor = to_window;

      while (from_ancestor != NULL || to_ancestor != NULL)
	{
	  if (from_ancestor != NULL)
	    {
	      from_ancestor = cdk_window_get_effective_parent (from_ancestor);
	      if (from_ancestor == to_window)
		break;
              if (from_ancestor)
	        from_ancestors = g_list_prepend (from_ancestors, from_ancestor);
	    }
	  if (to_ancestor != NULL)
	    {
	      to_ancestor = cdk_window_get_effective_parent (to_ancestor);
	      if (to_ancestor == from_window)
		break;
              if (to_ancestor)
	        to_ancestors = g_list_prepend (to_ancestors, to_ancestor);
	    }
	}
      if (to_ancestor == from_window)
	{
	  if (mode != CDK_CROSSING_CTK_UNGRAB)
	    synth_crossing (from, CDK_LEAVE_NOTIFY, from_window,
			    device, mode, CDK_NOTIFY_INFERIOR);
	  for (list = to_ancestors; list; list = list->next)
	    synth_crossing (NULL, CDK_ENTER_NOTIFY, (CdkWindow *) list->data,
			    device, mode, CDK_NOTIFY_VIRTUAL);
	  synth_crossing (to, CDK_ENTER_NOTIFY, to_window,
			  device, mode, CDK_NOTIFY_ANCESTOR);
	}
      else if (from_ancestor == to_window)
	{
	  synth_crossing (from, CDK_LEAVE_NOTIFY, from_window,
			  device, mode, CDK_NOTIFY_ANCESTOR);
	  for (list = g_list_last (from_ancestors); list; list = list->prev)
	    {
	      synth_crossing (NULL, CDK_LEAVE_NOTIFY, (CdkWindow *) list->data,
			      device, mode, CDK_NOTIFY_VIRTUAL);
	    }
	  if (mode != CDK_CROSSING_CTK_GRAB)
	    synth_crossing (to, CDK_ENTER_NOTIFY, to_window,
			    device, mode, CDK_NOTIFY_INFERIOR);
	}
      else
	{
	  while (from_ancestors != NULL && to_ancestors != NULL
		 && from_ancestors->data == to_ancestors->data)
	    {
	      from_ancestors = g_list_delete_link (from_ancestors,
						   from_ancestors);
	      to_ancestors = g_list_delete_link (to_ancestors, to_ancestors);
	    }

	  synth_crossing (from, CDK_LEAVE_NOTIFY, from_window,
			  device, mode, CDK_NOTIFY_NONLINEAR);

	  for (list = g_list_last (from_ancestors); list; list = list->prev)
	    {
	      synth_crossing (NULL, CDK_LEAVE_NOTIFY, (CdkWindow *) list->data,
			      device, mode, CDK_NOTIFY_NONLINEAR_VIRTUAL);
	    }
	  for (list = to_ancestors; list; list = list->next)
	    {
	      synth_crossing (NULL, CDK_ENTER_NOTIFY, (CdkWindow *) list->data,
			      device, mode, CDK_NOTIFY_NONLINEAR_VIRTUAL);
	    }
	  synth_crossing (to, CDK_ENTER_NOTIFY, to_window,
			  device, mode, CDK_NOTIFY_NONLINEAR);
	}
      g_list_free (from_ancestors);
      g_list_free (to_ancestors);
    }
}

static void
ctk_widget_propagate_state (CtkWidget    *widget,
                            CtkStateData *data)
{
  CtkWidgetPrivate *priv = widget->priv;
  CtkStateFlags new_flags, old_flags = priv->state_flags;
  CtkStateType old_state;
  gint new_scale_factor = ctk_widget_get_scale_factor (widget);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  old_state = ctk_widget_get_state (widget);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  priv->state_flags |= data->flags_to_set;
  priv->state_flags &= ~(data->flags_to_unset);

  /* make insensitivity unoverridable */
  if (!priv->sensitive)
    priv->state_flags |= CTK_STATE_FLAG_INSENSITIVE;

  if (ctk_widget_is_focus (widget) && !ctk_widget_is_sensitive (widget))
    {
      CtkWidget *window;

      window = _ctk_widget_get_toplevel (widget);

      if (window && _ctk_widget_is_toplevel (window))
        ctk_window_set_focus (CTK_WINDOW (window), NULL);
    }

  new_flags = priv->state_flags;

  if (data->old_scale_factor != new_scale_factor)
    _ctk_widget_scale_changed (widget);

  if (old_flags != new_flags)
    {
      g_object_ref (widget);

      if (!ctk_widget_is_sensitive (widget) && ctk_widget_has_grab (widget))
        ctk_grab_remove (widget);

      ctk_style_context_set_state (_ctk_widget_get_style_context (widget), new_flags);

      g_signal_emit (widget, widget_signals[STATE_CHANGED], 0, old_state);
      g_signal_emit (widget, widget_signals[STATE_FLAGS_CHANGED], 0, old_flags);

      if (!priv->shadowed &&
          (new_flags & CTK_STATE_FLAG_INSENSITIVE) != (old_flags & CTK_STATE_FLAG_INSENSITIVE))
        {
          GList *event_windows = NULL;
          GList *devices, *d;

          devices = _ctk_widget_list_devices (widget);

          for (d = devices; d; d = d->next)
            {
              CdkWindow *window;
              CdkDevice *device;

              device = d->data;
              window = _ctk_widget_get_device_window (widget, device);

              /* Do not propagate more than once to the
               * same window if non-multidevice aware.
               */
              if (!cdk_window_get_support_multidevice (window) &&
                  g_list_find (event_windows, window))
                continue;

              if (!ctk_widget_is_sensitive (widget))
                _ctk_widget_synthesize_crossing (widget, NULL, d->data,
                                                 CDK_CROSSING_STATE_CHANGED);
              else
                _ctk_widget_synthesize_crossing (NULL, widget, d->data,
                                                 CDK_CROSSING_STATE_CHANGED);

              event_windows = g_list_prepend (event_windows, window);
            }

          g_list_free (event_windows);
          g_list_free (devices);
        }

      if (!ctk_widget_is_sensitive (widget))
        ctk_widget_reset_controllers (widget);

      if (CTK_IS_CONTAINER (widget))
        {
          CtkStateData child_data;

          /* Make sure to only propagate the right states further */
          child_data.old_scale_factor = new_scale_factor;
          child_data.flags_to_set = data->flags_to_set & CTK_STATE_FLAGS_DO_PROPAGATE;
          child_data.flags_to_unset = data->flags_to_unset & CTK_STATE_FLAGS_DO_PROPAGATE;

          ctk_container_forall (CTK_CONTAINER (widget),
                                (CtkCallback) ctk_widget_propagate_state,
                                &child_data);
        }

      g_object_unref (widget);
    }
}

/**
 * ctk_widget_shape_combine_region:
 * @widget: a #CtkWidget
 * @region: (allow-none): shape to be added, or %NULL to remove an existing shape
 *
 * Sets a shape for this widget’s CDK window. This allows for
 * transparent windows etc., see cdk_window_shape_combine_region()
 * for more information.
 *
 * Since: 3.0
 **/
void
ctk_widget_shape_combine_region (CtkWidget *widget,
                                 cairo_region_t *region)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  /*  set_shape doesn't work on widgets without CDK window */
  g_return_if_fail (_ctk_widget_get_has_window (widget));

  priv = widget->priv;

  if (region == NULL)
    {
      priv->has_shape_mask = FALSE;

      if (priv->window)
	cdk_window_shape_combine_region (priv->window, NULL, 0, 0);

      g_object_set_qdata (G_OBJECT (widget), quark_shape_info, NULL);
    }
  else
    {
      priv->has_shape_mask = TRUE;

      g_object_set_qdata_full (G_OBJECT (widget), quark_shape_info,
                               cairo_region_copy (region),
			       (GDestroyNotify) cairo_region_destroy);

      /* set shape if widget has a CDK window already.
       * otherwise the shape is scheduled to be set by ctk_widget_realize().
       */
      if (priv->window)
	cdk_window_shape_combine_region (priv->window, region, 0, 0);
    }
}

static void
ctk_widget_update_input_shape (CtkWidget *widget)
{
  CtkWidgetPrivate *priv = widget->priv;

  /* set shape if widget has a CDK window already.
   * otherwise the shape is scheduled to be set by ctk_widget_realize().
   */
  if (priv->window)
    {
      cairo_region_t *region;
      cairo_region_t *csd_region;
      cairo_region_t *app_region;
      gboolean free_region;

      app_region = g_object_get_qdata (G_OBJECT (widget), quark_input_shape_info);
      csd_region = g_object_get_data (G_OBJECT (widget), "csd-region");

      free_region = FALSE;

      if (app_region && csd_region)
        {
          free_region = TRUE;
          region = cairo_region_copy (app_region);
          cairo_region_intersect (region, csd_region);
        }
      else if (app_region)
        region = app_region;
      else if (csd_region)
        region = csd_region;
      else
        region = NULL;

      cdk_window_input_shape_combine_region (priv->window, region, 0, 0);

      if (free_region)
        cairo_region_destroy (region);
    }
}

void
ctk_widget_set_csd_input_shape (CtkWidget            *widget,
                                const cairo_region_t *region)
{
  if (region == NULL)
    g_object_set_data (G_OBJECT (widget), "csd-region", NULL);
  else
    g_object_set_data_full (G_OBJECT (widget), "csd-region",
                            cairo_region_copy (region),
                            (GDestroyNotify) cairo_region_destroy);
  ctk_widget_update_input_shape (widget);
}

/**
 * ctk_widget_input_shape_combine_region:
 * @widget: a #CtkWidget
 * @region: (allow-none): shape to be added, or %NULL to remove an existing shape
 *
 * Sets an input shape for this widget’s CDK window. This allows for
 * windows which react to mouse click in a nonrectangular region, see
 * cdk_window_input_shape_combine_region() for more information.
 *
 * Since: 3.0
 **/
void
ctk_widget_input_shape_combine_region (CtkWidget      *widget,
                                       cairo_region_t *region)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  /*  set_shape doesn't work on widgets without CDK window */
  g_return_if_fail (_ctk_widget_get_has_window (widget));

  if (region == NULL)
    g_object_set_qdata (G_OBJECT (widget), quark_input_shape_info, NULL);
  else
    g_object_set_qdata_full (G_OBJECT (widget), quark_input_shape_info,
                             cairo_region_copy (region),
                             (GDestroyNotify) cairo_region_destroy);
  ctk_widget_update_input_shape (widget);
}


/* style properties
 */

/**
 * ctk_widget_class_install_style_property_parser: (skip)
 * @klass: a #CtkWidgetClass
 * @pspec: the #GParamSpec for the style property
 * @parser: the parser for the style property
 *
 * Installs a style property on a widget class.
 **/
void
ctk_widget_class_install_style_property_parser (CtkWidgetClass     *klass,
						GParamSpec         *pspec,
						CtkRcPropertyParser parser)
{
  g_return_if_fail (CTK_IS_WIDGET_CLASS (klass));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (pspec->flags & G_PARAM_READABLE);
  g_return_if_fail (!(pspec->flags & (G_PARAM_CONSTRUCT_ONLY | G_PARAM_CONSTRUCT)));

  if (g_param_spec_pool_lookup (style_property_spec_pool, pspec->name, G_OBJECT_CLASS_TYPE (klass), FALSE))
    {
      g_warning (G_STRLOC ": class '%s' already contains a style property named '%s'",
		 G_OBJECT_CLASS_NAME (klass),
		 pspec->name);
      return;
    }

  g_param_spec_ref_sink (pspec);
  g_param_spec_set_qdata (pspec, quark_property_parser, (gpointer) parser);
  g_param_spec_pool_insert (style_property_spec_pool, pspec, G_OBJECT_CLASS_TYPE (klass));
}

/**
 * ctk_widget_class_install_style_property:
 * @klass: a #CtkWidgetClass
 * @pspec: the #GParamSpec for the property
 *
 * Installs a style property on a widget class. The parser for the
 * style property is determined by the value type of @pspec.
 **/
void
ctk_widget_class_install_style_property (CtkWidgetClass *klass,
					 GParamSpec     *pspec)
{
  CtkRcPropertyParser parser;

  g_return_if_fail (CTK_IS_WIDGET_CLASS (klass));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  parser = _ctk_rc_property_parser_from_type (G_PARAM_SPEC_VALUE_TYPE (pspec));

  ctk_widget_class_install_style_property_parser (klass, pspec, parser);
}

/**
 * ctk_widget_class_find_style_property:
 * @klass: a #CtkWidgetClass
 * @property_name: the name of the style property to find
 *
 * Finds a style property of a widget class by name.
 *
 * Returns: (transfer none): the #GParamSpec of the style property or
 *   %NULL if @class has no style property with that name.
 *
 * Since: 2.2
 */
GParamSpec*
ctk_widget_class_find_style_property (CtkWidgetClass *klass,
				      const gchar    *property_name)
{
  g_return_val_if_fail (property_name != NULL, NULL);

  return g_param_spec_pool_lookup (style_property_spec_pool,
				   property_name,
				   G_OBJECT_CLASS_TYPE (klass),
				   TRUE);
}

/**
 * ctk_widget_class_list_style_properties:
 * @klass: a #CtkWidgetClass
 * @n_properties: (out): location to return the number of style properties found
 *
 * Returns all style properties of a widget class.
 *
 * Returns: (array length=n_properties) (transfer container): a
 *     newly allocated array of #GParamSpec*. The array must be
 *     freed with g_free().
 *
 * Since: 2.2
 */
GParamSpec**
ctk_widget_class_list_style_properties (CtkWidgetClass *klass,
					guint          *n_properties)
{
  GParamSpec **pspecs;
  guint n;

  pspecs = g_param_spec_pool_list (style_property_spec_pool,
				   G_OBJECT_CLASS_TYPE (klass),
				   &n);
  if (n_properties)
    *n_properties = n;

  return pspecs;
}

/**
 * ctk_widget_style_get_property:
 * @widget: a #CtkWidget
 * @property_name: the name of a style property
 * @value: location to return the property value
 *
 * Gets the value of a style property of @widget.
 */
void
ctk_widget_style_get_property (CtkWidget   *widget,
			       const gchar *property_name,
			       GValue      *value)
{
  GParamSpec *pspec;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  g_object_ref (widget);
  pspec = g_param_spec_pool_lookup (style_property_spec_pool,
				    property_name,
				    G_OBJECT_TYPE (widget),
				    TRUE);
  if (!pspec)
    g_warning ("%s: widget class '%s' has no property named '%s'",
	       G_STRLOC,
	       G_OBJECT_TYPE_NAME (widget),
	       property_name);
  else
    {
      CtkStyleContext *context;
      const GValue *peek_value;

      context = _ctk_widget_get_style_context (widget);

      peek_value = _ctk_style_context_peek_style_property (context,
                                                           G_OBJECT_TYPE (widget),
                                                           pspec);

      /* auto-conversion of the caller's value type
       */
      if (G_VALUE_TYPE (value) == G_PARAM_SPEC_VALUE_TYPE (pspec))
	g_value_copy (peek_value, value);
      else if (g_value_type_transformable (G_PARAM_SPEC_VALUE_TYPE (pspec), G_VALUE_TYPE (value)))
	g_value_transform (peek_value, value);
      else
	g_warning ("can't retrieve style property '%s' of type '%s' as value of type '%s'",
		   pspec->name,
		   g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
		   G_VALUE_TYPE_NAME (value));
    }
  g_object_unref (widget);
}

/**
 * ctk_widget_style_get_valist:
 * @widget: a #CtkWidget
 * @first_property_name: the name of the first property to get
 * @var_args: a va_list of pairs of property names and
 *     locations to return the property values, starting with the location
 *     for @first_property_name.
 *
 * Non-vararg variant of ctk_widget_style_get(). Used primarily by language
 * bindings.
 */
void
ctk_widget_style_get_valist (CtkWidget   *widget,
			     const gchar *first_property_name,
			     va_list      var_args)
{
  CtkStyleContext *context;
  const gchar *name;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  g_object_ref (widget);
  context = _ctk_widget_get_style_context (widget);

  name = first_property_name;
  while (name)
    {
      const GValue *peek_value;
      GParamSpec *pspec;
      gchar *error;

      pspec = g_param_spec_pool_lookup (style_property_spec_pool,
					name,
					G_OBJECT_TYPE (widget),
					TRUE);
      if (!pspec)
	{
	  g_warning ("%s: widget class '%s' has no property named '%s'",
		     G_STRLOC,
		     G_OBJECT_TYPE_NAME (widget),
		     name);
	  break;
	}
      /* style pspecs are always readable so we can spare that check here */

      peek_value = _ctk_style_context_peek_style_property (context,
                                                           G_OBJECT_TYPE (widget),
                                                           pspec);

      G_VALUE_LCOPY (peek_value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);
	  break;
	}

      name = va_arg (var_args, gchar*);
    }

  g_object_unref (widget);
}

/**
 * ctk_widget_style_get:
 * @widget: a #CtkWidget
 * @first_property_name: the name of the first property to get
 * @...: pairs of property names and locations to return the
 *     property values, starting with the location for
 *     @first_property_name, terminated by %NULL.
 *
 * Gets the values of a multiple style properties of @widget.
 */
void
ctk_widget_style_get (CtkWidget   *widget,
		      const gchar *first_property_name,
		      ...)
{
  va_list var_args;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  va_start (var_args, first_property_name);
  ctk_widget_style_get_valist (widget, first_property_name, var_args);
  va_end (var_args);
}

/**
 * ctk_requisition_new:
 *
 * Allocates a new #CtkRequisition-struct and initializes its elements to zero.
 *
 * Returns: a new empty #CtkRequisition. The newly allocated #CtkRequisition should
 *   be freed with ctk_requisition_free().
 *
 * Since: 3.0
 */
CtkRequisition *
ctk_requisition_new (void)
{
  return g_slice_new0 (CtkRequisition);
}

/**
 * ctk_requisition_copy:
 * @requisition: a #CtkRequisition
 *
 * Copies a #CtkRequisition.
 *
 * Returns: a copy of @requisition
 **/
CtkRequisition *
ctk_requisition_copy (const CtkRequisition *requisition)
{
  return g_slice_dup (CtkRequisition, requisition);
}

/**
 * ctk_requisition_free:
 * @requisition: a #CtkRequisition
 *
 * Frees a #CtkRequisition.
 **/
void
ctk_requisition_free (CtkRequisition *requisition)
{
  g_slice_free (CtkRequisition, requisition);
}

G_DEFINE_BOXED_TYPE (CtkRequisition, ctk_requisition,
                     ctk_requisition_copy,
                     ctk_requisition_free)

/**
 * ctk_widget_class_set_accessible_type:
 * @widget_class: class to set the accessible type for
 * @type: The object type that implements the accessible for @widget_class
 *
 * Sets the type to be used for creating accessibles for widgets of
 * @widget_class. The given @type must be a subtype of the type used for
 * accessibles of the parent class.
 *
 * This function should only be called from class init functions of widgets.
 *
 * Since: 3.2
 **/
void
ctk_widget_class_set_accessible_type (CtkWidgetClass *widget_class,
                                      GType           type)
{
  CtkWidgetClassPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET_CLASS (widget_class));
  g_return_if_fail (g_type_is_a (type, widget_class->priv->accessible_type));

  priv = widget_class->priv;

  priv->accessible_type = type;
  /* reset this - honoring the type's role is better. */
  priv->accessible_role = ATK_ROLE_INVALID;
}

/**
 * ctk_widget_class_set_accessible_role:
 * @widget_class: class to set the accessible role for
 * @role: The role to use for accessibles created for @widget_class
 *
 * Sets the default #AtkRole to be set on accessibles created for
 * widgets of @widget_class. Accessibles may decide to not honor this
 * setting if their role reporting is more refined. Calls to 
 * ctk_widget_class_set_accessible_type() will reset this value.
 *
 * In cases where you want more fine-grained control over the role of
 * accessibles created for @widget_class, you should provide your own
 * accessible type and use ctk_widget_class_set_accessible_type()
 * instead.
 *
 * If @role is #ATK_ROLE_INVALID, the default role will not be changed
 * and the accessible’s default role will be used instead.
 *
 * This function should only be called from class init functions of widgets.
 *
 * Since: 3.2
 **/
void
ctk_widget_class_set_accessible_role (CtkWidgetClass *widget_class,
                                      AtkRole         role)
{
  CtkWidgetClassPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET_CLASS (widget_class));

  priv = widget_class->priv;

  priv->accessible_role = role;
}

/**
 * _ctk_widget_peek_accessible:
 * @widget: a #CtkWidget
 *
 * Gets the accessible for @widget, if it has been created yet.
 * Otherwise, this function returns %NULL. If the @widget’s implementation
 * does not use the default way to create accessibles, %NULL will always be
 * returned.
 *
 * Returns: (nullable): the accessible for @widget or %NULL if none has been
 *     created yet.
 **/
AtkObject *
_ctk_widget_peek_accessible (CtkWidget *widget)
{
  return widget->priv->accessible;
}

/**
 * ctk_widget_get_accessible:
 * @widget: a #CtkWidget
 *
 * Returns the accessible object that describes the widget to an
 * assistive technology.
 *
 * If accessibility support is not available, this #AtkObject
 * instance may be a no-op. Likewise, if no class-specific #AtkObject
 * implementation is available for the widget instance in question,
 * it will inherit an #AtkObject implementation from the first ancestor
 * class for which such an implementation is defined.
 *
 * The documentation of the
 * [ATK](http://developer.gnome.org/atk/stable/)
 * library contains more information about accessible objects and their uses.
 *
 * Returns: (transfer none): the #AtkObject associated with @widget
 */
AtkObject*
ctk_widget_get_accessible (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return CTK_WIDGET_GET_CLASS (widget)->get_accessible (widget);
}

static AtkObject*
ctk_widget_real_get_accessible (CtkWidget *widget)
{
  AtkObject* accessible;

  accessible = widget->priv->accessible;

  if (!accessible)
    {
      CtkWidgetClass *widget_class;
      CtkWidgetClassPrivate *priv;
      AtkObjectFactory *factory;
      AtkRegistry *default_registry;

      widget_class = CTK_WIDGET_GET_CLASS (widget);
      priv = widget_class->priv;

      if (priv->accessible_type == CTK_TYPE_ACCESSIBLE)
        {
          default_registry = atk_get_default_registry ();
          factory = atk_registry_get_factory (default_registry,
                                              G_TYPE_FROM_INSTANCE (widget));
          accessible = atk_object_factory_create_accessible (factory, G_OBJECT (widget));

          if (priv->accessible_role != ATK_ROLE_INVALID)
            atk_object_set_role (accessible, priv->accessible_role);

          widget->priv->accessible = accessible;
        }
      else
        {
          accessible = g_object_new (priv->accessible_type,
                                     "widget", widget,
                                     NULL);
          if (priv->accessible_role != ATK_ROLE_INVALID)
            atk_object_set_role (accessible, priv->accessible_role);

          widget->priv->accessible = accessible;

          atk_object_initialize (accessible, widget);

          /* Set the role again, since we don't want a role set
           * in some parent initialize() function to override
           * our own.
           */
          if (priv->accessible_role != ATK_ROLE_INVALID)
            atk_object_set_role (accessible, priv->accessible_role);
        }

    }

  return accessible;
}

/*
 * Initialize a AtkImplementorIface instance’s virtual pointers as
 * appropriate to this implementor’s class (CtkWidget).
 */
static void
ctk_widget_accessible_interface_init (AtkImplementorIface *iface)
{
  iface->ref_accessible = ctk_widget_ref_accessible;
}

static AtkObject*
ctk_widget_ref_accessible (AtkImplementor *implementor)
{
  AtkObject *accessible;

  accessible = ctk_widget_get_accessible (CTK_WIDGET (implementor));
  if (accessible)
    g_object_ref (accessible);
  return accessible;
}

/*
 * Expand flag management
 */

static void
ctk_widget_update_computed_expand (CtkWidget *widget)
{
  CtkWidgetPrivate *priv;

  priv = widget->priv;

  if (priv->need_compute_expand)
    {
      gboolean h, v;

      if (priv->hexpand_set)
        h = priv->hexpand;
      else
        h = FALSE;

      if (priv->vexpand_set)
        v = priv->vexpand;
      else
        v = FALSE;

      /* we don't need to use compute_expand if both expands are
       * forced by the app
       */
      if (!(priv->hexpand_set && priv->vexpand_set))
        {
          if (CTK_WIDGET_GET_CLASS (widget)->compute_expand != NULL)
            {
              gboolean ignored;

              CTK_WIDGET_GET_CLASS (widget)->compute_expand (widget,
                                                             priv->hexpand_set ? &ignored : &h,
                                                             priv->vexpand_set ? &ignored : &v);
            }
        }

      priv->need_compute_expand = FALSE;
      priv->computed_hexpand = h != FALSE;
      priv->computed_vexpand = v != FALSE;
    }
}

/**
 * ctk_widget_queue_compute_expand:
 * @widget: a #CtkWidget
 *
 * Mark @widget as needing to recompute its expand flags. Call
 * this function when setting legacy expand child properties
 * on the child of a container.
 *
 * See ctk_widget_compute_expand().
 */
void
ctk_widget_queue_compute_expand (CtkWidget *widget)
{
  CtkWidget *parent;
  gboolean changed_anything;

  if (widget->priv->need_compute_expand)
    return;

  changed_anything = FALSE;
  parent = widget;
  while (parent != NULL)
    {
      if (!parent->priv->need_compute_expand)
        {
          parent->priv->need_compute_expand = TRUE;
          changed_anything = TRUE;
        }

      /* Note: if we had an invariant that "if a child needs to
       * compute expand, its parents also do" then we could stop going
       * up when we got to a parent that already needed to
       * compute. However, in general we compute expand lazily (as
       * soon as we see something in a subtree that is expand, we know
       * we're expanding) and so this invariant does not hold and we
       * have to always walk all the way up in case some ancestor
       * is not currently need_compute_expand.
       */

      parent = parent->priv->parent;
    }

  /* recomputing expand always requires
   * a relayout as well
   */
  if (changed_anything)
    ctk_widget_queue_resize (widget);
}

/**
 * ctk_widget_compute_expand:
 * @widget: the widget
 * @orientation: expand direction
 *
 * Computes whether a container should give this widget extra space
 * when possible. Containers should check this, rather than
 * looking at ctk_widget_get_hexpand() or ctk_widget_get_vexpand().
 *
 * This function already checks whether the widget is visible, so
 * visibility does not need to be checked separately. Non-visible
 * widgets are not expanded.
 *
 * The computed expand value uses either the expand setting explicitly
 * set on the widget itself, or, if none has been explicitly set,
 * the widget may expand if some of its children do.
 *
 * Returns: whether widget tree rooted here should be expanded
 */
gboolean
ctk_widget_compute_expand (CtkWidget      *widget,
                           CtkOrientation  orientation)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  /* We never make a widget expand if not even showing. */
  if (!_ctk_widget_get_visible (widget))
    return FALSE;

  ctk_widget_update_computed_expand (widget);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    return widget->priv->computed_hexpand;
  else
    return widget->priv->computed_vexpand;
}

static void
ctk_widget_set_expand (CtkWidget     *widget,
                       CtkOrientation orientation,
                       gboolean       expand)
{
  gint expand_prop;
  gint expand_set_prop;
  gboolean was_both;
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = widget->priv;

  expand = expand != FALSE;

  was_both = priv->hexpand && priv->vexpand;

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (priv->hexpand_set &&
          priv->hexpand == expand)
        return;

      priv->hexpand_set = TRUE;
      priv->hexpand = expand;

      expand_prop = PROP_HEXPAND;
      expand_set_prop = PROP_HEXPAND_SET;
    }
  else
    {
      if (priv->vexpand_set &&
          priv->vexpand == expand)
        return;

      priv->vexpand_set = TRUE;
      priv->vexpand = expand;

      expand_prop = PROP_VEXPAND;
      expand_set_prop = PROP_VEXPAND_SET;
    }

  ctk_widget_queue_compute_expand (widget);

  g_object_freeze_notify (G_OBJECT (widget));
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[expand_prop]);
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[expand_set_prop]);
  if (was_both != (priv->hexpand && priv->vexpand))
    g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_EXPAND]);
  g_object_thaw_notify (G_OBJECT (widget));
}

static void
ctk_widget_set_expand_set (CtkWidget      *widget,
                           CtkOrientation  orientation,
                           gboolean        set)
{
  CtkWidgetPrivate *priv;
  gint prop;

  priv = widget->priv;

  set = set != FALSE;

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (set == priv->hexpand_set)
        return;

      priv->hexpand_set = set;
      prop = PROP_HEXPAND_SET;
    }
  else
    {
      if (set == priv->vexpand_set)
        return;

      priv->vexpand_set = set;
      prop = PROP_VEXPAND_SET;
    }

  ctk_widget_queue_compute_expand (widget);

  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[prop]);
}

/**
 * ctk_widget_get_hexpand:
 * @widget: the widget
 *
 * Gets whether the widget would like any available extra horizontal
 * space. When a user resizes a #CtkWindow, widgets with expand=TRUE
 * generally receive the extra space. For example, a list or
 * scrollable area or document in your window would often be set to
 * expand.
 *
 * Containers should use ctk_widget_compute_expand() rather than
 * this function, to see whether a widget, or any of its children,
 * has the expand flag set. If any child of a widget wants to
 * expand, the parent may ask to expand also.
 *
 * This function only looks at the widget’s own hexpand flag, rather
 * than computing whether the entire widget tree rooted at this widget
 * wants to expand.
 *
 * Returns: whether hexpand flag is set
 */
gboolean
ctk_widget_get_hexpand (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->hexpand;
}

/**
 * ctk_widget_set_hexpand:
 * @widget: the widget
 * @expand: whether to expand
 *
 * Sets whether the widget would like any available extra horizontal
 * space. When a user resizes a #CtkWindow, widgets with expand=TRUE
 * generally receive the extra space. For example, a list or
 * scrollable area or document in your window would often be set to
 * expand.
 *
 * Call this function to set the expand flag if you would like your
 * widget to become larger horizontally when the window has extra
 * room.
 *
 * By default, widgets automatically expand if any of their children
 * want to expand. (To see if a widget will automatically expand given
 * its current children and state, call ctk_widget_compute_expand(). A
 * container can decide how the expandability of children affects the
 * expansion of the container by overriding the compute_expand virtual
 * method on #CtkWidget.).
 *
 * Setting hexpand explicitly with this function will override the
 * automatic expand behavior.
 *
 * This function forces the widget to expand or not to expand,
 * regardless of children.  The override occurs because
 * ctk_widget_set_hexpand() sets the hexpand-set property (see
 * ctk_widget_set_hexpand_set()) which causes the widget’s hexpand
 * value to be used, rather than looking at children and widget state.
 */
void
ctk_widget_set_hexpand (CtkWidget      *widget,
                        gboolean        expand)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_widget_set_expand (widget, CTK_ORIENTATION_HORIZONTAL, expand);
}

/**
 * ctk_widget_get_hexpand_set:
 * @widget: the widget
 *
 * Gets whether ctk_widget_set_hexpand() has been used to
 * explicitly set the expand flag on this widget.
 *
 * If hexpand is set, then it overrides any computed
 * expand value based on child widgets. If hexpand is not
 * set, then the expand value depends on whether any
 * children of the widget would like to expand.
 *
 * There are few reasons to use this function, but it’s here
 * for completeness and consistency.
 *
 * Returns: whether hexpand has been explicitly set
 */
gboolean
ctk_widget_get_hexpand_set (CtkWidget      *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->hexpand_set;
}

/**
 * ctk_widget_set_hexpand_set:
 * @widget: the widget
 * @set: value for hexpand-set property
 *
 * Sets whether the hexpand flag (see ctk_widget_get_hexpand()) will
 * be used.
 *
 * The hexpand-set property will be set automatically when you call
 * ctk_widget_set_hexpand() to set hexpand, so the most likely
 * reason to use this function would be to unset an explicit expand
 * flag.
 *
 * If hexpand is set, then it overrides any computed
 * expand value based on child widgets. If hexpand is not
 * set, then the expand value depends on whether any
 * children of the widget would like to expand.
 *
 * There are few reasons to use this function, but it’s here
 * for completeness and consistency.
 */
void
ctk_widget_set_hexpand_set (CtkWidget      *widget,
                            gboolean        set)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_widget_set_expand_set (widget, CTK_ORIENTATION_HORIZONTAL, set);
}


/**
 * ctk_widget_get_vexpand:
 * @widget: the widget
 *
 * Gets whether the widget would like any available extra vertical
 * space.
 *
 * See ctk_widget_get_hexpand() for more detail.
 *
 * Returns: whether vexpand flag is set
 */
gboolean
ctk_widget_get_vexpand (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->vexpand;
}

/**
 * ctk_widget_set_vexpand:
 * @widget: the widget
 * @expand: whether to expand
 *
 * Sets whether the widget would like any available extra vertical
 * space.
 *
 * See ctk_widget_set_hexpand() for more detail.
 */
void
ctk_widget_set_vexpand (CtkWidget      *widget,
                        gboolean        expand)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_widget_set_expand (widget, CTK_ORIENTATION_VERTICAL, expand);
}

/**
 * ctk_widget_get_vexpand_set:
 * @widget: the widget
 *
 * Gets whether ctk_widget_set_vexpand() has been used to
 * explicitly set the expand flag on this widget.
 *
 * See ctk_widget_get_hexpand_set() for more detail.
 *
 * Returns: whether vexpand has been explicitly set
 */
gboolean
ctk_widget_get_vexpand_set (CtkWidget      *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->vexpand_set;
}

/**
 * ctk_widget_set_vexpand_set:
 * @widget: the widget
 * @set: value for vexpand-set property
 *
 * Sets whether the vexpand flag (see ctk_widget_get_vexpand()) will
 * be used.
 *
 * See ctk_widget_set_hexpand_set() for more detail.
 */
void
ctk_widget_set_vexpand_set (CtkWidget      *widget,
                            gboolean        set)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_widget_set_expand_set (widget, CTK_ORIENTATION_VERTICAL, set);
}

/*
 * CtkBuildable implementation
 */
static GQuark		 quark_builder_has_default = 0;
static GQuark		 quark_builder_has_focus = 0;
static GQuark		 quark_builder_atk_relations = 0;
static GQuark            quark_builder_set_name = 0;

static void
ctk_widget_buildable_interface_init (CtkBuildableIface *iface)
{
  quark_builder_has_default = g_quark_from_static_string ("ctk-builder-has-default");
  quark_builder_has_focus = g_quark_from_static_string ("ctk-builder-has-focus");
  quark_builder_atk_relations = g_quark_from_static_string ("ctk-builder-atk-relations");
  quark_builder_set_name = g_quark_from_static_string ("ctk-builder-set-name");

  iface->set_name = ctk_widget_buildable_set_name;
  iface->get_name = ctk_widget_buildable_get_name;
  iface->get_internal_child = ctk_widget_buildable_get_internal_child;
  iface->set_buildable_property = ctk_widget_buildable_set_buildable_property;
  iface->parser_finished = ctk_widget_buildable_parser_finished;
  iface->custom_tag_start = ctk_widget_buildable_custom_tag_start;
  iface->custom_finished = ctk_widget_buildable_custom_finished;
}

static void
ctk_widget_buildable_set_name (CtkBuildable *buildable,
			       const gchar  *name)
{
  g_object_set_qdata_full (G_OBJECT (buildable), quark_builder_set_name,
                           g_strdup (name), g_free);
}

static const gchar *
ctk_widget_buildable_get_name (CtkBuildable *buildable)
{
  return g_object_get_qdata (G_OBJECT (buildable), quark_builder_set_name);
}

static GObject *
ctk_widget_buildable_get_internal_child (CtkBuildable *buildable,
					 CtkBuilder   *builder,
					 const gchar  *childname)
{
  CtkWidgetClass *class;
  GSList *l;
  GType internal_child_type = 0;

  if (strcmp (childname, "accessible") == 0)
    return G_OBJECT (ctk_widget_get_accessible (CTK_WIDGET (buildable)));

  /* Find a widget type which has declared an automated child as internal by
   * the name 'childname', if any.
   */
  for (class = CTK_WIDGET_GET_CLASS (buildable);
       CTK_IS_WIDGET_CLASS (class);
       class = g_type_class_peek_parent (class))
    {
      CtkWidgetTemplate *template = class->priv->template;

      if (!template)
	continue;

      for (l = template->children; l && internal_child_type == 0; l = l->next)
	{
	  AutomaticChildClass *child_class = l->data;

	  if (child_class->internal_child && strcmp (childname, child_class->name) == 0)
	    internal_child_type = G_OBJECT_CLASS_TYPE (class);
	}
    }

  /* Now return the 'internal-child' from the class which declared it, note
   * that ctk_widget_get_template_child() an API used to access objects
   * which are in the private scope of a given class.
   */
  if (internal_child_type != 0)
    return ctk_widget_get_template_child (CTK_WIDGET (buildable), internal_child_type, childname);

  return NULL;
}

static void
ctk_widget_buildable_set_buildable_property (CtkBuildable *buildable,
					     CtkBuilder   *builder,
					     const gchar  *name,
					     const GValue *value)
{
  if (strcmp (name, "has-default") == 0 && g_value_get_boolean (value))
      g_object_set_qdata (G_OBJECT (buildable), quark_builder_has_default,
			  GINT_TO_POINTER (TRUE));
  else if (strcmp (name, "has-focus") == 0 && g_value_get_boolean (value))
      g_object_set_qdata (G_OBJECT (buildable), quark_builder_has_focus,
			  GINT_TO_POINTER (TRUE));
  else
    g_object_set_property (G_OBJECT (buildable), name, value);
}

typedef struct
{
  gchar *action_name;
  GString *description;
  gchar *context;
  gboolean translatable;
} AtkActionData;

typedef struct
{
  gchar *target;
  AtkRelationType type;
  gint line;
  gint col;
} AtkRelationData;

static void
free_action (AtkActionData *data, gpointer user_data)
{
  g_free (data->action_name);
  g_string_free (data->description, TRUE);
  g_free (data->context);
  g_slice_free (AtkActionData, data);
}

static void
free_relation (AtkRelationData *data, gpointer user_data)
{
  g_free (data->target);
  g_slice_free (AtkRelationData, data);
}

static void
ctk_widget_buildable_parser_finished (CtkBuildable *buildable,
				      CtkBuilder   *builder)
{
  GSList *atk_relations;

  if (g_object_get_qdata (G_OBJECT (buildable), quark_builder_has_default))
    {
      ctk_widget_grab_default (CTK_WIDGET (buildable));
      g_object_steal_qdata (G_OBJECT (buildable), quark_builder_has_default);
    }

  if (g_object_get_qdata (G_OBJECT (buildable), quark_builder_has_focus))
    {
      ctk_widget_grab_focus (CTK_WIDGET (buildable));
      g_object_steal_qdata (G_OBJECT (buildable), quark_builder_has_focus);
    }

  atk_relations = g_object_get_qdata (G_OBJECT (buildable),
				      quark_builder_atk_relations);
  if (atk_relations)
    {
      AtkObject *accessible;
      AtkRelationSet *relation_set;
      GSList *l;
      GObject *target;
      AtkObject *target_accessible;

      accessible = ctk_widget_get_accessible (CTK_WIDGET (buildable));
      relation_set = atk_object_ref_relation_set (accessible);

      for (l = atk_relations; l; l = l->next)
	{
	  AtkRelationData *relation = (AtkRelationData*)l->data;

	  target = _ctk_builder_lookup_object (builder, relation->target, relation->line, relation->col);
	  if (!target)
	    continue;
	  target_accessible = ctk_widget_get_accessible (CTK_WIDGET (target));
	  g_assert (target_accessible != NULL);

	  atk_relation_set_add_relation_by_type (relation_set, relation->type, target_accessible);
	}
      g_object_unref (relation_set);

      g_slist_free_full (atk_relations, (GDestroyNotify) free_relation);
      g_object_steal_qdata (G_OBJECT (buildable), quark_builder_atk_relations);
    }
}

typedef struct
{
  CtkBuilder *builder;
  GSList *actions;
  GSList *relations;
} AccessibilitySubParserData;

static void
accessibility_start_element (GMarkupParseContext  *context,
                             const gchar          *element_name,
                             const gchar         **names,
                             const gchar         **values,
                             gpointer              user_data,
                             GError              **error)
{
  AccessibilitySubParserData *data = (AccessibilitySubParserData*)user_data;

  if (strcmp (element_name, "relation") == 0)
    {
      gchar *target = NULL;
      gchar *type = NULL;
      AtkRelationData *relation;
      AtkRelationType relation_type;

      if (!_ctk_builder_check_parent (data->builder, context, "accessibility", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "target", &target,
                                        G_MARKUP_COLLECT_STRING, "type", &type,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      relation_type = atk_relation_type_for_name (type);
      if (relation_type == ATK_RELATION_NULL)
        {
          g_set_error (error,
                       CTK_BUILDER_ERROR,
                       CTK_BUILDER_ERROR_INVALID_VALUE,
                       "No such relation type: '%s'", type);
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      relation = g_slice_new (AtkRelationData);
      relation->target = g_strdup (target);
      relation->type = relation_type;

      data->relations = g_slist_prepend (data->relations, relation);
    }
  else if (strcmp (element_name, "action") == 0)
    {
      const gchar *action_name;
      const gchar *description = NULL;
      const gchar *msg_context = NULL;
      gboolean translatable = FALSE;
      AtkActionData *action;

      if (!_ctk_builder_check_parent (data->builder, context, "accessibility", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "action_name", &action_name,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "description", &description,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "comments", NULL,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "context", &msg_context,
                                        G_MARKUP_COLLECT_BOOLEAN|G_MARKUP_COLLECT_OPTIONAL, "translatable", &translatable,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      action = g_slice_new (AtkActionData);
      action->action_name = g_strdup (action_name);
      action->description = g_string_new (description);
      action->context = g_strdup (msg_context);
      action->translatable = translatable;

      data->actions = g_slist_prepend (data->actions, action);
    }
  else if (strcmp (element_name, "accessibility") == 0)
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
                                        "CtkWidget", element_name,
                                        error);
    }
}

static void
accessibility_text (GMarkupParseContext  *context,
                    const gchar          *text,
                    gsize                 text_len,
                    gpointer              user_data,
                    GError              **error)
{
  AccessibilitySubParserData *data = (AccessibilitySubParserData*)user_data;

  if (strcmp (g_markup_parse_context_get_element (context), "action") == 0)
    {
      AtkActionData *action = data->actions->data;

      g_string_append_len (action->description, text, text_len);
    }
}

static const GMarkupParser accessibility_parser =
  {
    accessibility_start_element,
    NULL,
    accessibility_text,
  };

typedef struct
{
  GObject *object;
  CtkBuilder *builder;
  guint    key;
  guint    modifiers;
  gchar   *signal;
} AccelGroupParserData;

static void
accel_group_start_element (GMarkupParseContext  *context,
                           const gchar          *element_name,
                           const gchar         **names,
                           const gchar         **values,
                           gpointer              user_data,
                           GError              **error)
{
  AccelGroupParserData *data = (AccelGroupParserData*)user_data;

  if (strcmp (element_name, "accelerator") == 0)
    {
      const gchar *key_str = NULL;
      const gchar *signal = NULL;
      const gchar *modifiers_str = NULL;
      guint key = 0;
      guint modifiers = 0;

      if (!_ctk_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "key", &key_str,
                                        G_MARKUP_COLLECT_STRING, "signal", &signal,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "modifiers", &modifiers_str,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      key = cdk_keyval_from_name (key_str);
      if (key == 0)
        {
          g_set_error (error,
                       CTK_BUILDER_ERROR, CTK_BUILDER_ERROR_INVALID_VALUE,
                       "Could not parse key '%s'", key_str);
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      if (modifiers_str != NULL)
        {
          GFlagsValue aliases[2] = {
            { 0, "primary", "primary" },
            { 0, NULL, NULL }
          };

          aliases[0].value = _ctk_get_primary_accel_mod ();

          if (!_ctk_builder_flags_from_string (CDK_TYPE_MODIFIER_TYPE, aliases,
                                               modifiers_str, &modifiers, error))
            {
              _ctk_builder_prefix_error (data->builder, context, error);
	      return;
            }
        }

      data->key = key;
      data->modifiers = modifiers;
      data->signal = g_strdup (signal);
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkWidget", element_name,
                                        error);
    }
}

static const GMarkupParser accel_group_parser =
  {
    accel_group_start_element,
  };

typedef struct
{
  CtkBuilder *builder;
  GSList *classes;
} StyleParserData;

static void
style_start_element (GMarkupParseContext  *context,
                     const gchar          *element_name,
                     const gchar         **names,
                     const gchar         **values,
                     gpointer              user_data,
                     GError              **error)
{
  StyleParserData *data = (StyleParserData *)user_data;

  if (strcmp (element_name, "class") == 0)
    {
      const gchar *name;

      if (!_ctk_builder_check_parent (data->builder, context, "style", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      data->classes = g_slist_prepend (data->classes, g_strdup (name));
    }
  else if (strcmp (element_name, "style") == 0)
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
                                        "CtkWidget", element_name,
                                        error);
    }
}

static const GMarkupParser style_parser =
  {
    style_start_element,
  };

static gboolean
ctk_widget_buildable_custom_tag_start (CtkBuildable     *buildable,
                                       CtkBuilder       *builder,
                                       GObject          *child,
                                       const gchar      *tagname,
                                       GMarkupParser    *parser,
                                       gpointer         *parser_data)
{
  if (strcmp (tagname, "accelerator") == 0)
    {
      AccelGroupParserData *data;

      data = g_slice_new0 (AccelGroupParserData);
      data->object = G_OBJECT (g_object_ref (buildable));
      data->builder = builder;

      *parser = accel_group_parser;
      *parser_data = data;

      return TRUE;
    }

  if (strcmp (tagname, "accessibility") == 0)
    {
      AccessibilitySubParserData *data;

      data = g_slice_new0 (AccessibilitySubParserData);
      data->builder = builder;

      *parser = accessibility_parser;
      *parser_data = data;

      return TRUE;
    }

  if (strcmp (tagname, "style") == 0)
    {
      StyleParserData *data;

      data = g_slice_new0 (StyleParserData);
      data->builder = builder;

      *parser = style_parser;
      *parser_data = data;

      return TRUE;
    }

  return FALSE;
}

void
_ctk_widget_buildable_finish_accelerator (CtkWidget *widget,
                                          CtkWidget *toplevel,
                                          gpointer   user_data)
{
  AccelGroupParserData *accel_data;
  GSList *accel_groups;
  CtkAccelGroup *accel_group;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CTK_IS_WIDGET (toplevel));
  g_return_if_fail (user_data != NULL);

  accel_data = (AccelGroupParserData*)user_data;
  accel_groups = ctk_accel_groups_from_object (G_OBJECT (toplevel));
  if (g_slist_length (accel_groups) == 0)
    {
      accel_group = ctk_accel_group_new ();
      ctk_window_add_accel_group (CTK_WINDOW (toplevel), accel_group);
    }
  else
    {
      g_assert (g_slist_length (accel_groups) == 1);
      accel_group = g_slist_nth_data (accel_groups, 0);
    }

  ctk_widget_add_accelerator (CTK_WIDGET (accel_data->object),
			      accel_data->signal,
			      accel_group,
			      accel_data->key,
			      accel_data->modifiers,
			      CTK_ACCEL_VISIBLE);

  g_object_unref (accel_data->object);
  g_free (accel_data->signal);
  g_slice_free (AccelGroupParserData, accel_data);
}

static void
ctk_widget_buildable_custom_finished (CtkBuildable *buildable,
                                      CtkBuilder   *builder,
                                      GObject      *child,
                                      const gchar  *tagname,
                                      gpointer      user_data)
{
  if (strcmp (tagname, "accelerator") == 0)
    {
      AccelGroupParserData *accel_data;
      CtkWidget *toplevel;

      accel_data = (AccelGroupParserData*)user_data;
      g_assert (accel_data->object);

      toplevel = _ctk_widget_get_toplevel (CTK_WIDGET (accel_data->object));

      _ctk_widget_buildable_finish_accelerator (CTK_WIDGET (buildable), toplevel, user_data);
    }
  else if (strcmp (tagname, "accessibility") == 0)
    {
      AccessibilitySubParserData *a11y_data;

      a11y_data = (AccessibilitySubParserData*)user_data;

      if (a11y_data->actions)
	{
	  AtkObject *accessible;
	  AtkAction *action;
	  gint i, n_actions;
	  GSList *l;

	  accessible = ctk_widget_get_accessible (CTK_WIDGET (buildable));

          if (ATK_IS_ACTION (accessible))
            {
	      action = ATK_ACTION (accessible);
	      n_actions = atk_action_get_n_actions (action);

	      for (l = a11y_data->actions; l; l = l->next)
	        {
	          AtkActionData *action_data = (AtkActionData*)l->data;

	          for (i = 0; i < n_actions; i++)
		    if (strcmp (atk_action_get_name (action, i),
		  	        action_data->action_name) == 0)
		      break;

	          if (i < n_actions)
                    {
                      const gchar *description;

                      if (action_data->translatable && action_data->description->len)
                        description = _ctk_builder_parser_translate (ctk_builder_get_translation_domain (builder),
                                                                     action_data->context,
                                                                     action_data->description->str);
                      else
                        description = action_data->description->str;

		      atk_action_set_description (action, i, description);
                    }
                }
	    }
          else
            g_warning ("accessibility action on a widget that does not implement AtkAction");

	  g_slist_free_full (a11y_data->actions, (GDestroyNotify) free_action);
	}

      if (a11y_data->relations)
	g_object_set_qdata (G_OBJECT (buildable), quark_builder_atk_relations,
			    a11y_data->relations);

      g_slice_free (AccessibilitySubParserData, a11y_data);
    }
  else if (strcmp (tagname, "style") == 0)
    {
      StyleParserData *style_data = (StyleParserData *)user_data;
      CtkStyleContext *context;
      GSList *l;

      context = _ctk_widget_get_style_context (CTK_WIDGET (buildable));

      for (l = style_data->classes; l; l = l->next)
        ctk_style_context_add_class (context, (const gchar *)l->data);

      ctk_widget_reset_style (CTK_WIDGET (buildable));

      g_slist_free_full (style_data->classes, g_free);
      g_slice_free (StyleParserData, style_data);
    }
}

static CtkSizeRequestMode
ctk_widget_real_get_request_mode (CtkWidget *widget)
{
  /* By default widgets don't trade size at all. */
  return CTK_SIZE_REQUEST_CONSTANT_SIZE;
}

static void
ctk_widget_real_get_width (CtkWidget *widget,
			   gint      *minimum_size,
			   gint      *natural_size)
{
  *minimum_size = 0;
  *natural_size = 0;
}

static void
ctk_widget_real_get_height (CtkWidget *widget,
			    gint      *minimum_size,
			    gint      *natural_size)
{
  *minimum_size = 0;
  *natural_size = 0;
}

static void
ctk_widget_real_get_height_for_width (CtkWidget *widget,
                                      gint       width,
                                      gint      *minimum_height,
                                      gint      *natural_height)
{
  CTK_WIDGET_GET_CLASS (widget)->get_preferred_height (widget, minimum_height, natural_height);
}

static void
ctk_widget_real_get_width_for_height (CtkWidget *widget,
                                      gint       height,
                                      gint      *minimum_width,
                                      gint      *natural_width)
{
  CTK_WIDGET_GET_CLASS (widget)->get_preferred_width (widget, minimum_width, natural_width);
}

/**
 * ctk_widget_get_halign:
 * @widget: a #CtkWidget
 *
 * Gets the value of the #CtkWidget:halign property.
 *
 * For backwards compatibility reasons this method will never return
 * %CTK_ALIGN_BASELINE, but instead it will convert it to
 * %CTK_ALIGN_FILL. Baselines are not supported for horizontal
 * alignment.
 *
 * Returns: the horizontal alignment of @widget
 */
CtkAlign
ctk_widget_get_halign (CtkWidget *widget)
{
  CtkAlign align;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), CTK_ALIGN_FILL);

  align = widget->priv->halign;
  if (align == CTK_ALIGN_BASELINE)
    return CTK_ALIGN_FILL;
  return align;
}

/**
 * ctk_widget_set_halign:
 * @widget: a #CtkWidget
 * @align: the horizontal alignment
 *
 * Sets the horizontal alignment of @widget.
 * See the #CtkWidget:halign property.
 */
void
ctk_widget_set_halign (CtkWidget *widget,
                       CtkAlign   align)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (widget->priv->halign == align)
    return;

  widget->priv->halign = align;
  ctk_widget_queue_allocate (widget);
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_HALIGN]);
}

/**
 * ctk_widget_get_valign_with_baseline:
 * @widget: a #CtkWidget
 *
 * Gets the value of the #CtkWidget:valign property, including
 * %CTK_ALIGN_BASELINE.
 *
 * Returns: the vertical alignment of @widget
 *
 * Since: 3.10
 */
CtkAlign
ctk_widget_get_valign_with_baseline (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), CTK_ALIGN_FILL);
  return widget->priv->valign;
}

/**
 * ctk_widget_get_valign:
 * @widget: a #CtkWidget
 *
 * Gets the value of the #CtkWidget:valign property.
 *
 * For backwards compatibility reasons this method will never return
 * %CTK_ALIGN_BASELINE, but instead it will convert it to
 * %CTK_ALIGN_FILL. If your widget want to support baseline aligned
 * children it must use ctk_widget_get_valign_with_baseline(), or
 * `g_object_get (widget, "valign", &value, NULL)`, which will
 * also report the true value.
 *
 * Returns: the vertical alignment of @widget, ignoring baseline alignment
 */
CtkAlign
ctk_widget_get_valign (CtkWidget *widget)
{
  CtkAlign align;

  align = ctk_widget_get_valign_with_baseline (widget);
  if (align == CTK_ALIGN_BASELINE)
    return CTK_ALIGN_FILL;
  return align;
}

/**
 * ctk_widget_set_valign:
 * @widget: a #CtkWidget
 * @align: the vertical alignment
 *
 * Sets the vertical alignment of @widget.
 * See the #CtkWidget:valign property.
 */
void
ctk_widget_set_valign (CtkWidget *widget,
                       CtkAlign   align)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  if (widget->priv->valign == align)
    return;

  widget->priv->valign = align;
  ctk_widget_queue_allocate (widget);
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_VALIGN]);
}

/**
 * ctk_widget_get_margin_left:
 * @widget: a #CtkWidget
 *
 * Gets the value of the #CtkWidget:margin-left property.
 *
 * Returns: The left margin of @widget
 *
 * Deprecated: 3.12: Use ctk_widget_get_margin_start() instead.
 *
 * Since: 3.0
 */
gint
ctk_widget_get_margin_left (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);

  return widget->priv->margin.left;
}

/**
 * ctk_widget_set_margin_left:
 * @widget: a #CtkWidget
 * @margin: the left margin
 *
 * Sets the left margin of @widget.
 * See the #CtkWidget:margin-left property.
 *
 * Deprecated: 3.12: Use ctk_widget_set_margin_start() instead.
 *
 * Since: 3.0
 */
void
ctk_widget_set_margin_left (CtkWidget *widget,
                            gint       margin)
{
  gboolean rtl;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (margin <= G_MAXINT16);

  rtl = _ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL;

  if (widget->priv->margin.left == margin)
    return;

  widget->priv->margin.left = margin;
  ctk_widget_queue_resize (widget);
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_MARGIN_LEFT]);
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[rtl ? PROP_MARGIN_END : PROP_MARGIN_START]);
}

/**
 * ctk_widget_get_margin_right:
 * @widget: a #CtkWidget
 *
 * Gets the value of the #CtkWidget:margin-right property.
 *
 * Returns: The right margin of @widget
 *
 * Deprecated: 3.12: Use ctk_widget_get_margin_end() instead.
 *
 * Since: 3.0
 */
gint
ctk_widget_get_margin_right (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);

  return widget->priv->margin.right;
}

/**
 * ctk_widget_set_margin_right:
 * @widget: a #CtkWidget
 * @margin: the right margin
 *
 * Sets the right margin of @widget.
 * See the #CtkWidget:margin-right property.
 *
 * Deprecated: 3.12: Use ctk_widget_set_margin_end() instead.
 *
 * Since: 3.0
 */
void
ctk_widget_set_margin_right (CtkWidget *widget,
                             gint       margin)
{
  gboolean rtl;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (margin <= G_MAXINT16);

  rtl = _ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL;

  if (widget->priv->margin.right == margin)
    return;

  widget->priv->margin.right = margin;
  ctk_widget_queue_resize (widget);
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_MARGIN_RIGHT]);
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[rtl ? PROP_MARGIN_START : PROP_MARGIN_END]);
}

/**
 * ctk_widget_get_margin_start:
 * @widget: a #CtkWidget
 *
 * Gets the value of the #CtkWidget:margin-start property.
 *
 * Returns: The start margin of @widget
 *
 * Since: 3.12
 */
gint
ctk_widget_get_margin_start (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);

  if (_ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL)
    return widget->priv->margin.right;
  else
    return widget->priv->margin.left;
}

/**
 * ctk_widget_set_margin_start:
 * @widget: a #CtkWidget
 * @margin: the start margin
 *
 * Sets the start margin of @widget.
 * See the #CtkWidget:margin-start property.
 *
 * Since: 3.12
 */
void
ctk_widget_set_margin_start (CtkWidget *widget,
                             gint       margin)
{
  gint16 *start;
  gboolean rtl;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (margin <= G_MAXINT16);

  rtl = _ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL;

  if (rtl)
    start = &widget->priv->margin.right;
  else
    start = &widget->priv->margin.left;

  if (*start == margin)
    return;

  *start = margin;
  ctk_widget_queue_resize (widget);
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_MARGIN_START]);
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[rtl ? PROP_MARGIN_RIGHT : PROP_MARGIN_LEFT]);
}

/**
 * ctk_widget_get_margin_end:
 * @widget: a #CtkWidget
 *
 * Gets the value of the #CtkWidget:margin-end property.
 *
 * Returns: The end margin of @widget
 *
 * Since: 3.12
 */
gint
ctk_widget_get_margin_end (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);

  if (_ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL)
    return widget->priv->margin.left;
  else
    return widget->priv->margin.right;
}

/**
 * ctk_widget_set_margin_end:
 * @widget: a #CtkWidget
 * @margin: the end margin
 *
 * Sets the end margin of @widget.
 * See the #CtkWidget:margin-end property.
 *
 * Since: 3.12
 */
void
ctk_widget_set_margin_end (CtkWidget *widget,
                           gint       margin)
{
  gint16 *end;
  gboolean rtl;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (margin <= G_MAXINT16);

  rtl = _ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL;

  if (rtl)
    end = &widget->priv->margin.left;
  else
    end = &widget->priv->margin.right;

  if (*end == margin)
    return;

  *end = margin;
  ctk_widget_queue_resize (widget);
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_MARGIN_END]);
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[rtl ? PROP_MARGIN_LEFT : PROP_MARGIN_RIGHT]);
}

/**
 * ctk_widget_get_margin_top:
 * @widget: a #CtkWidget
 *
 * Gets the value of the #CtkWidget:margin-top property.
 *
 * Returns: The top margin of @widget
 *
 * Since: 3.0
 */
gint
ctk_widget_get_margin_top (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);

  return widget->priv->margin.top;
}

/**
 * ctk_widget_set_margin_top:
 * @widget: a #CtkWidget
 * @margin: the top margin
 *
 * Sets the top margin of @widget.
 * See the #CtkWidget:margin-top property.
 *
 * Since: 3.0
 */
void
ctk_widget_set_margin_top (CtkWidget *widget,
                           gint       margin)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (margin <= G_MAXINT16);

  if (widget->priv->margin.top == margin)
    return;

  widget->priv->margin.top = margin;
  ctk_widget_queue_resize (widget);
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_MARGIN_TOP]);
}

/**
 * ctk_widget_get_margin_bottom:
 * @widget: a #CtkWidget
 *
 * Gets the value of the #CtkWidget:margin-bottom property.
 *
 * Returns: The bottom margin of @widget
 *
 * Since: 3.0
 */
gint
ctk_widget_get_margin_bottom (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);

  return widget->priv->margin.bottom;
}

/**
 * ctk_widget_set_margin_bottom:
 * @widget: a #CtkWidget
 * @margin: the bottom margin
 *
 * Sets the bottom margin of @widget.
 * See the #CtkWidget:margin-bottom property.
 *
 * Since: 3.0
 */
void
ctk_widget_set_margin_bottom (CtkWidget *widget,
                              gint       margin)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (margin <= G_MAXINT16);

  if (widget->priv->margin.bottom == margin)
    return;

  widget->priv->margin.bottom = margin;
  ctk_widget_queue_resize (widget);
  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_MARGIN_BOTTOM]);
}

/**
 * ctk_widget_get_clipboard:
 * @widget: a #CtkWidget
 * @selection: a #CdkAtom which identifies the clipboard
 *             to use. %CDK_SELECTION_CLIPBOARD gives the
 *             default clipboard. Another common value
 *             is %CDK_SELECTION_PRIMARY, which gives
 *             the primary X selection.
 *
 * Returns the clipboard object for the given selection to
 * be used with @widget. @widget must have a #CdkDisplay
 * associated with it, so must be attached to a toplevel
 * window.
 *
 * Returns: (transfer none): the appropriate clipboard object. If no
 *             clipboard already exists, a new one will
 *             be created. Once a clipboard object has
 *             been created, it is persistent for all time.
 *
 * Since: 2.2
 **/
CtkClipboard *
ctk_widget_get_clipboard (CtkWidget *widget, CdkAtom selection)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (ctk_widget_has_screen (widget), NULL);

  return ctk_clipboard_get_for_display (ctk_widget_get_display (widget),
					selection);
}

/**
 * ctk_widget_list_mnemonic_labels:
 * @widget: a #CtkWidget
 *
 * Returns a newly allocated list of the widgets, normally labels, for
 * which this widget is the target of a mnemonic (see for example,
 * ctk_label_set_mnemonic_widget()).

 * The widgets in the list are not individually referenced. If you
 * want to iterate through the list and perform actions involving
 * callbacks that might destroy the widgets, you
 * must call `g_list_foreach (result,
 * (GFunc)g_object_ref, NULL)` first, and then unref all the
 * widgets afterwards.

 * Returns: (element-type CtkWidget) (transfer container): the list of
 *  mnemonic labels; free this list
 *  with g_list_free() when you are done with it.
 *
 * Since: 2.4
 **/
GList *
ctk_widget_list_mnemonic_labels (CtkWidget *widget)
{
  GList *list = NULL;
  GSList *l;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  for (l = g_object_get_qdata (G_OBJECT (widget), quark_mnemonic_labels); l; l = l->next)
    list = g_list_prepend (list, l->data);

  return list;
}

/**
 * ctk_widget_add_mnemonic_label:
 * @widget: a #CtkWidget
 * @label: a #CtkWidget that acts as a mnemonic label for @widget
 *
 * Adds a widget to the list of mnemonic labels for
 * this widget. (See ctk_widget_list_mnemonic_labels()). Note the
 * list of mnemonic labels for the widget is cleared when the
 * widget is destroyed, so the caller must make sure to update
 * its internal state at this point as well, by using a connection
 * to the #CtkWidget::destroy signal or a weak notifier.
 *
 * Since: 2.4
 **/
void
ctk_widget_add_mnemonic_label (CtkWidget *widget,
                               CtkWidget *label)
{
  GSList *old_list, *new_list;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CTK_IS_WIDGET (label));

  old_list = g_object_steal_qdata (G_OBJECT (widget), quark_mnemonic_labels);
  new_list = g_slist_prepend (old_list, label);

  g_object_set_qdata_full (G_OBJECT (widget), quark_mnemonic_labels,
			   new_list, (GDestroyNotify) g_slist_free);
}

/**
 * ctk_widget_remove_mnemonic_label:
 * @widget: a #CtkWidget
 * @label: a #CtkWidget that was previously set as a mnemonic label for
 *         @widget with ctk_widget_add_mnemonic_label().
 *
 * Removes a widget from the list of mnemonic labels for
 * this widget. (See ctk_widget_list_mnemonic_labels()). The widget
 * must have previously been added to the list with
 * ctk_widget_add_mnemonic_label().
 *
 * Since: 2.4
 **/
void
ctk_widget_remove_mnemonic_label (CtkWidget *widget,
                                  CtkWidget *label)
{
  GSList *old_list, *new_list;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CTK_IS_WIDGET (label));

  old_list = g_object_steal_qdata (G_OBJECT (widget), quark_mnemonic_labels);
  new_list = g_slist_remove (old_list, label);

  if (new_list)
    g_object_set_qdata_full (G_OBJECT (widget), quark_mnemonic_labels,
			     new_list, (GDestroyNotify) g_slist_free);
}

/**
 * ctk_widget_get_no_show_all:
 * @widget: a #CtkWidget
 *
 * Returns the current value of the #CtkWidget:no-show-all property,
 * which determines whether calls to ctk_widget_show_all()
 * will affect this widget.
 *
 * Returns: the current value of the “no-show-all” property.
 *
 * Since: 2.4
 **/
gboolean
ctk_widget_get_no_show_all (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->no_show_all;
}

/**
 * ctk_widget_set_no_show_all:
 * @widget: a #CtkWidget
 * @no_show_all: the new value for the “no-show-all” property
 *
 * Sets the #CtkWidget:no-show-all property, which determines whether
 * calls to ctk_widget_show_all() will affect this widget.
 *
 * This is mostly for use in constructing widget hierarchies with externally
 * controlled visibility, see #CtkUIManager.
 *
 * Since: 2.4
 **/
void
ctk_widget_set_no_show_all (CtkWidget *widget,
			    gboolean   no_show_all)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  no_show_all = (no_show_all != FALSE);

  if (widget->priv->no_show_all != no_show_all)
    {
      widget->priv->no_show_all = no_show_all;

      g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_NO_SHOW_ALL]);
    }
}


static void
ctk_widget_real_set_has_tooltip (CtkWidget *widget,
			         gboolean   has_tooltip,
			         gboolean   force)
{
  CtkWidgetPrivate *priv = widget->priv;

  if (priv->has_tooltip != has_tooltip || force)
    {
      priv->has_tooltip = has_tooltip;

      if (priv->has_tooltip)
        {
	  if (_ctk_widget_get_realized (widget) && !_ctk_widget_get_has_window (widget))
	    cdk_window_set_events (priv->window,
				   cdk_window_get_events (priv->window) |
				   CDK_LEAVE_NOTIFY_MASK |
				   CDK_POINTER_MOTION_MASK);

	  if (_ctk_widget_get_has_window (widget))
	      ctk_widget_add_events (widget,
				     CDK_LEAVE_NOTIFY_MASK |
				     CDK_POINTER_MOTION_MASK);
	}

      g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_HAS_TOOLTIP]);
    }
}

/**
 * ctk_widget_set_tooltip_window:
 * @widget: a #CtkWidget
 * @custom_window: (allow-none): a #CtkWindow, or %NULL
 *
 * Replaces the default window used for displaying
 * tooltips with @custom_window. CTK+ will take care of showing and
 * hiding @custom_window at the right moment, to behave likewise as
 * the default tooltip window. If @custom_window is %NULL, the default
 * tooltip window will be used.
 *
 * Since: 2.12
 */
void
ctk_widget_set_tooltip_window (CtkWidget *widget,
			       CtkWindow *custom_window)
{
  gboolean has_tooltip;
  gchar *tooltip_markup;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (custom_window == NULL || CTK_IS_WINDOW (custom_window));

  tooltip_markup = g_object_get_qdata (G_OBJECT (widget), quark_tooltip_markup);

  if (custom_window)
    g_object_ref (custom_window);

  g_object_set_qdata_full (G_OBJECT (widget), quark_tooltip_window,
			   custom_window, g_object_unref);

  has_tooltip = (custom_window != NULL || tooltip_markup != NULL);
  ctk_widget_real_set_has_tooltip (widget, has_tooltip, FALSE);

  if (has_tooltip && _ctk_widget_get_visible (widget))
    ctk_widget_queue_tooltip_query (widget);
}

/**
 * ctk_widget_get_tooltip_window:
 * @widget: a #CtkWidget
 *
 * Returns the #CtkWindow of the current tooltip. This can be the
 * CtkWindow created by default, or the custom tooltip window set
 * using ctk_widget_set_tooltip_window().
 *
 * Returns: (transfer none): The #CtkWindow of the current tooltip.
 *
 * Since: 2.12
 */
CtkWindow *
ctk_widget_get_tooltip_window (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return g_object_get_qdata (G_OBJECT (widget), quark_tooltip_window);
}

/**
 * ctk_widget_trigger_tooltip_query:
 * @widget: a #CtkWidget
 *
 * Triggers a tooltip query on the display where the toplevel of @widget
 * is located. See ctk_tooltip_trigger_tooltip_query() for more
 * information.
 *
 * Since: 2.12
 */
void
ctk_widget_trigger_tooltip_query (CtkWidget *widget)
{
  ctk_tooltip_trigger_tooltip_query (ctk_widget_get_display (widget));
}

static guint tooltip_query_id;
static GSList *tooltip_query_displays;

static gboolean
tooltip_query_idle (gpointer data)
{
  g_slist_foreach (tooltip_query_displays, (GFunc)ctk_tooltip_trigger_tooltip_query, NULL);
  g_slist_free_full (tooltip_query_displays, g_object_unref);

  tooltip_query_displays = NULL;
  tooltip_query_id = 0;

  return FALSE;
}

static void
ctk_widget_queue_tooltip_query (CtkWidget *widget)
{
  CdkDisplay *display;

  display = ctk_widget_get_display (widget);

  if (!g_slist_find (tooltip_query_displays, display))
    tooltip_query_displays = g_slist_prepend (tooltip_query_displays, g_object_ref (display));

  if (tooltip_query_id == 0)
    {
      tooltip_query_id = cdk_threads_add_idle (tooltip_query_idle, NULL);
      g_source_set_name_by_id (tooltip_query_id, "[ctk+] tooltip_query_idle");
    }
}

/**
 * ctk_widget_set_tooltip_text:
 * @widget: a #CtkWidget
 * @text: (allow-none): the contents of the tooltip for @widget
 *
 * Sets @text as the contents of the tooltip. This function will take
 * care of setting #CtkWidget:has-tooltip to %TRUE and of the default
 * handler for the #CtkWidget::query-tooltip signal.
 *
 * See also the #CtkWidget:tooltip-text property and ctk_tooltip_set_text().
 *
 * Since: 2.12
 */
void
ctk_widget_set_tooltip_text (CtkWidget   *widget,
                             const gchar *text)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  g_object_set (G_OBJECT (widget), "tooltip-text", text, NULL);
}

/**
 * ctk_widget_get_tooltip_text:
 * @widget: a #CtkWidget
 *
 * Gets the contents of the tooltip for @widget.
 *
 * Returns: (nullable): the tooltip text, or %NULL. You should free the
 *   returned string with g_free() when done.
 *
 * Since: 2.12
 */
gchar *
ctk_widget_get_tooltip_text (CtkWidget *widget)
{
  gchar *text = NULL;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  g_object_get (G_OBJECT (widget), "tooltip-text", &text, NULL);

  return text;
}

/**
 * ctk_widget_set_tooltip_markup:
 * @widget: a #CtkWidget
 * @markup: (allow-none): the contents of the tooltip for @widget, or %NULL
 *
 * Sets @markup as the contents of the tooltip, which is marked up with
 *  the [Pango text markup language][PangoMarkupFormat].
 *
 * This function will take care of setting #CtkWidget:has-tooltip to %TRUE
 * and of the default handler for the #CtkWidget::query-tooltip signal.
 *
 * See also the #CtkWidget:tooltip-markup property and
 * ctk_tooltip_set_markup().
 *
 * Since: 2.12
 */
void
ctk_widget_set_tooltip_markup (CtkWidget   *widget,
                               const gchar *markup)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  g_object_set (G_OBJECT (widget), "tooltip-markup", markup, NULL);
}

/**
 * ctk_widget_get_tooltip_markup:
 * @widget: a #CtkWidget
 *
 * Gets the contents of the tooltip for @widget.
 *
 * Returns: (nullable): the tooltip text, or %NULL. You should free the
 *   returned string with g_free() when done.
 *
 * Since: 2.12
 */
gchar *
ctk_widget_get_tooltip_markup (CtkWidget *widget)
{
  gchar *text = NULL;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  g_object_get (G_OBJECT (widget), "tooltip-markup", &text, NULL);

  return text;
}

/**
 * ctk_widget_set_has_tooltip:
 * @widget: a #CtkWidget
 * @has_tooltip: whether or not @widget has a tooltip.
 *
 * Sets the has-tooltip property on @widget to @has_tooltip.  See
 * #CtkWidget:has-tooltip for more information.
 *
 * Since: 2.12
 */
void
ctk_widget_set_has_tooltip (CtkWidget *widget,
			    gboolean   has_tooltip)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_widget_real_set_has_tooltip (widget, has_tooltip, FALSE);
}

/**
 * ctk_widget_get_has_tooltip:
 * @widget: a #CtkWidget
 *
 * Returns the current value of the has-tooltip property.  See
 * #CtkWidget:has-tooltip for more information.
 *
 * Returns: current value of has-tooltip on @widget.
 *
 * Since: 2.12
 */
gboolean
ctk_widget_get_has_tooltip (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->has_tooltip;
}

/**
 * ctk_widget_get_clip:
 * @widget: a #CtkWidget
 * @clip: (out): a pointer to a #CtkAllocation to copy to
 *
 * Retrieves the widget’s clip area.
 *
 * The clip area is the area in which all of @widget's drawing will
 * happen. Other toolkits call it the bounding box.
 *
 * Historically, in CTK+ the clip area has been equal to the allocation
 * retrieved via ctk_widget_get_allocation().
 *
 * Since: 3.14
 */
void
ctk_widget_get_clip (CtkWidget     *widget,
                     CtkAllocation *clip)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (clip != NULL);

  priv = widget->priv;

  *clip = priv->clip;
}

/**
 * ctk_widget_set_clip:
 * @widget: a #CtkWidget
 * @clip: a pointer to a #CtkAllocation to copy from
 *
 * Sets the widget’s clip.  This must not be used directly,
 * but from within a widget’s size_allocate method.
 * It must be called after ctk_widget_set_allocation() (or after chaining up
 * to the parent class), because that function resets the clip.
 *
 * The clip set should be the area that @widget draws on. If @widget is a
 * #CtkContainer, the area must contain all children's clips.
 *
 * If this function is not called by @widget during a ::size-allocate handler,
 * the clip will be set to @widget's allocation.
 *
 * Since: 3.14
 */
void
ctk_widget_set_clip (CtkWidget           *widget,
                     const CtkAllocation *clip)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (_ctk_widget_get_visible (widget) || _ctk_widget_is_toplevel (widget));
  g_return_if_fail (clip != NULL);

  priv = widget->priv;

#ifdef G_ENABLE_DEBUG
  if (CTK_DEBUG_CHECK (GEOMETRY))
    {
      gint depth;
      CtkWidget *parent;
      const gchar *name;

      depth = 0;
      parent = widget;
      while (parent)
	{
	  depth++;
	  parent = _ctk_widget_get_parent (parent);
	}

      name = g_type_name (G_OBJECT_TYPE (G_OBJECT (widget)));
      g_message ("ctk_widget_set_clip:      %*s%s %d %d %d %d",
	         2 * depth, " ", name,
	         clip->x, clip->y,
	         clip->width, clip->height);
    }
#endif /* G_ENABLE_DEBUG */

  priv->clip = *clip;

  while (priv->parent &&
         _ctk_widget_get_window (widget) == _ctk_widget_get_window (priv->parent))
    {
      CtkWidgetPrivate *parent_priv = priv->parent->priv;
      CdkRectangle union_rect;

      cdk_rectangle_union (&priv->clip,
                           &parent_priv->clip,
                           &union_rect);

      if (cdk_rectangle_equal (&parent_priv->clip, &union_rect))
        break;

      parent_priv->clip = union_rect;
      priv = parent_priv;
    }
}

/*
 * _ctk_widget_set_simple_clip:
 * @widget: a #CtkWidget
 * @content_clip: (nullable): Clipping area of the contents
 *     or %NULL, if the contents
 *     do not extent the allocation.
 *
 * This is a convenience function for ctk_widget_set_clip(), if you
 * just want to set the clip for @widget based on its allocation,
 * CSS properties and - if the widget is a #CtkContainer - its
 * children. ctk_widget_set_allocation() must have been called
 * and all children must have been allocated with
 * ctk_widget_size_allocate() before calling this function.
 * It is therefore a good idea to call this function last in
 * your implementation of CtkWidget::size_allocate().
 *
 * If your widget overdraws its contents, you cannot use this
 * function and must call ctk_widget_set_clip() yourself.
 **/
void
_ctk_widget_set_simple_clip (CtkWidget     *widget,
                             CtkAllocation *content_clip)
{
  CtkStyleContext *context;
  CtkAllocation clip, allocation;
  CtkBorder extents;

  context = _ctk_widget_get_style_context (widget);

  _ctk_widget_get_allocation (widget, &allocation);

  _ctk_css_shadows_value_get_extents (_ctk_style_context_peek_property (context,
                                                                        CTK_CSS_PROPERTY_BOX_SHADOW),
                                      &extents);

  clip = allocation;
  clip.x -= extents.left;
  clip.y -= extents.top;
  clip.width += extents.left + extents.right;
  clip.height += extents.top + extents.bottom;

  if (content_clip)
    cdk_rectangle_union (content_clip, &clip, &clip);

  if (CTK_IS_CONTAINER (widget))
    {
      CdkRectangle children_clip;

      ctk_container_get_children_clip (CTK_CONTAINER (widget), &children_clip);

      if (_ctk_widget_get_has_window (widget))
        {
          children_clip.x += allocation.x;
          children_clip.y += allocation.y;
        }

      cdk_rectangle_union (&children_clip, &clip, &clip);
    }

  ctk_widget_set_clip (widget, &clip);
}

/**
 * ctk_widget_get_allocated_size:
 * @widget: a #CtkWidget
 * @allocation: (out): a pointer to a #CtkAllocation to copy to
 * @baseline: (out) (allow-none): a pointer to an integer to copy to
 *
 * Retrieves the widget’s allocated size.
 *
 * This function returns the last values passed to
 * ctk_widget_size_allocate_with_baseline(). The value differs from
 * the size returned in ctk_widget_get_allocation() in that functions
 * like ctk_widget_set_halign() can adjust the allocation, but not
 * the value returned by this function.
 *
 * If a widget is not visible, its allocated size is 0.
 *
 * Since: 3.20
 */
void
ctk_widget_get_allocated_size (CtkWidget     *widget,
                               CtkAllocation *allocation,
                               int           *baseline)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (allocation != NULL);

  priv = widget->priv;

  *allocation = priv->allocated_size;

  if (baseline)
    *baseline = priv->allocated_size_baseline;
}

/**
 * ctk_widget_get_allocation:
 * @widget: a #CtkWidget
 * @allocation: (out): a pointer to a #CtkAllocation to copy to
 *
 * Retrieves the widget’s allocation.
 *
 * Note, when implementing a #CtkContainer: a widget’s allocation will
 * be its “adjusted” allocation, that is, the widget’s parent
 * container typically calls ctk_widget_size_allocate() with an
 * allocation, and that allocation is then adjusted (to handle margin
 * and alignment for example) before assignment to the widget.
 * ctk_widget_get_allocation() returns the adjusted allocation that
 * was actually assigned to the widget. The adjusted allocation is
 * guaranteed to be completely contained within the
 * ctk_widget_size_allocate() allocation, however. So a #CtkContainer
 * is guaranteed that its children stay inside the assigned bounds,
 * but not that they have exactly the bounds the container assigned.
 * There is no way to get the original allocation assigned by
 * ctk_widget_size_allocate(), since it isn’t stored; if a container
 * implementation needs that information it will have to track it itself.
 *
 * Since: 2.18
 */
void
ctk_widget_get_allocation (CtkWidget     *widget,
                           CtkAllocation *allocation)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (allocation != NULL);

  priv = widget->priv;

  *allocation = priv->allocation;
}

/**
 * ctk_widget_set_allocation:
 * @widget: a #CtkWidget
 * @allocation: a pointer to a #CtkAllocation to copy from
 *
 * Sets the widget’s allocation.  This should not be used
 * directly, but from within a widget’s size_allocate method.
 *
 * The allocation set should be the “adjusted” or actual
 * allocation. If you’re implementing a #CtkContainer, you want to use
 * ctk_widget_size_allocate() instead of ctk_widget_set_allocation().
 * The CtkWidgetClass::adjust_size_allocation virtual method adjusts the
 * allocation inside ctk_widget_size_allocate() to create an adjusted
 * allocation.
 *
 * Since: 2.18
 */
void
ctk_widget_set_allocation (CtkWidget           *widget,
                           const CtkAllocation *allocation)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (_ctk_widget_get_visible (widget) || _ctk_widget_is_toplevel (widget));
  g_return_if_fail (allocation != NULL);

  priv = widget->priv;

  priv->allocation = *allocation;
  priv->clip = *allocation;
}

/**
 * ctk_widget_get_allocated_width:
 * @widget: the widget to query
 *
 * Returns the width that has currently been allocated to @widget.
 * This function is intended to be used when implementing handlers
 * for the #CtkWidget::draw function.
 *
 * Returns: the width of the @widget
 **/
int
ctk_widget_get_allocated_width (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);

  return widget->priv->allocation.width;
}

/**
 * ctk_widget_get_allocated_height:
 * @widget: the widget to query
 *
 * Returns the height that has currently been allocated to @widget.
 * This function is intended to be used when implementing handlers
 * for the #CtkWidget::draw function.
 *
 * Returns: the height of the @widget
 **/
int
ctk_widget_get_allocated_height (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);

  return widget->priv->allocation.height;
}

/**
 * ctk_widget_get_allocated_baseline:
 * @widget: the widget to query
 *
 * Returns the baseline that has currently been allocated to @widget.
 * This function is intended to be used when implementing handlers
 * for the #CtkWidget::draw function, and when allocating child
 * widgets in #CtkWidget::size_allocate.
 *
 * Returns: the baseline of the @widget, or -1 if none
 *
 * Since: 3.10
 **/
int
ctk_widget_get_allocated_baseline (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);

  return widget->priv->allocated_baseline;
}

/**
 * ctk_widget_get_requisition:
 * @widget: a #CtkWidget
 * @requisition: (out): a pointer to a #CtkRequisition to copy to
 *
 * Retrieves the widget’s requisition.
 *
 * This function should only be used by widget implementations in
 * order to figure whether the widget’s requisition has actually
 * changed after some internal state change (so that they can call
 * ctk_widget_queue_resize() instead of ctk_widget_queue_draw()).
 *
 * Normally, ctk_widget_size_request() should be used.
 *
 * Since: 2.20
 *
 * Deprecated: 3.0: The #CtkRequisition cache on the widget was
 * removed, If you need to cache sizes across requests and allocations,
 * add an explicit cache to the widget in question instead.
 */
void
ctk_widget_get_requisition (CtkWidget      *widget,
                            CtkRequisition *requisition)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (requisition != NULL);

  ctk_widget_get_preferred_size (widget, requisition, NULL);
}

/**
 * ctk_widget_set_window:
 * @widget: a #CtkWidget
 * @window: (transfer full): a #CdkWindow
 *
 * Sets a widget’s window. This function should only be used in a
 * widget’s #CtkWidget::realize implementation. The %window passed is
 * usually either new window created with cdk_window_new(), or the
 * window of its parent widget as returned by
 * ctk_widget_get_parent_window().
 *
 * Widgets must indicate whether they will create their own #CdkWindow
 * by calling ctk_widget_set_has_window(). This is usually done in the
 * widget’s init() function.
 *
 * Note that this function does not add any reference to @window.
 *
 * Since: 2.18
 */
void
ctk_widget_set_window (CtkWidget *widget,
                       CdkWindow *window)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (window == NULL || CDK_IS_WINDOW (window));

  priv = widget->priv;

  if (priv->window != window)
    {
      priv->window = window;

      g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_WINDOW]);
    }
}

/**
 * ctk_widget_register_window:
 * @widget: a #CtkWidget
 * @window: a #CdkWindow
 *
 * Registers a #CdkWindow with the widget and sets it up so that
 * the widget receives events for it. Call ctk_widget_unregister_window()
 * when destroying the window.
 *
 * Before 3.8 you needed to call cdk_window_set_user_data() directly to set
 * this up. This is now deprecated and you should use ctk_widget_register_window()
 * instead. Old code will keep working as is, although some new features like
 * transparency might not work perfectly.
 *
 * Since: 3.8
 */
void
ctk_widget_register_window (CtkWidget    *widget,
			    CdkWindow    *window)
{
  CtkWidgetPrivate *priv;
  gpointer user_data;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CDK_IS_WINDOW (window));

  cdk_window_get_user_data (window, &user_data);
  g_assert (user_data == NULL);

  priv = widget->priv;

  cdk_window_set_user_data (window, widget);
  priv->registered_windows = g_list_prepend (priv->registered_windows, window);
}

/**
 * ctk_widget_unregister_window:
 * @widget: a #CtkWidget
 * @window: a #CdkWindow
 *
 * Unregisters a #CdkWindow from the widget that was previously set up with
 * ctk_widget_register_window(). You need to call this when the window is
 * no longer used by the widget, such as when you destroy it.
 *
 * Since: 3.8
 */
void
ctk_widget_unregister_window (CtkWidget    *widget,
			      CdkWindow    *window)
{
  CtkWidgetPrivate *priv;
  gpointer user_data;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CDK_IS_WINDOW (window));

  priv = widget->priv;

  cdk_window_get_user_data (window, &user_data);
  g_assert (user_data == widget);
  cdk_window_set_user_data (window, NULL);
  priv->registered_windows = g_list_remove (priv->registered_windows, window);
}

/**
 * ctk_widget_get_window:
 * @widget: a #CtkWidget
 *
 * Returns the widget’s window if it is realized, %NULL otherwise
 *
 * Returns: (transfer none) (nullable): @widget’s window.
 *
 * Since: 2.14
 */
CdkWindow*
ctk_widget_get_window (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return widget->priv->window;
}

/**
 * ctk_widget_get_support_multidevice:
 * @widget: a #CtkWidget
 *
 * Returns %TRUE if @widget is multiple pointer aware. See
 * ctk_widget_set_support_multidevice() for more information.
 *
 * Returns: %TRUE if @widget is multidevice aware.
 **/
gboolean
ctk_widget_get_support_multidevice (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return widget->priv->multidevice;
}

/**
 * ctk_widget_set_support_multidevice:
 * @widget: a #CtkWidget
 * @support_multidevice: %TRUE to support input from multiple devices.
 *
 * Enables or disables multiple pointer awareness. If this setting is %TRUE,
 * @widget will start receiving multiple, per device enter/leave events. Note
 * that if custom #CdkWindows are created in #CtkWidget::realize,
 * cdk_window_set_support_multidevice() will have to be called manually on them.
 *
 * Since: 3.0
 **/
void
ctk_widget_set_support_multidevice (CtkWidget *widget,
                                    gboolean   support_multidevice)
{
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = widget->priv;
  priv->multidevice = (support_multidevice == TRUE);

  if (_ctk_widget_get_realized (widget))
    cdk_window_set_support_multidevice (priv->window, support_multidevice);
}

/* There are multiple alpha related sources. First of all the user can specify alpha
 * in ctk_widget_set_opacity, secondly we can get it from the CSS opacity. These two
 * are multiplied together to form the total alpha. Secondly, the user can specify
 * an opacity group for a widget, which means we must essentially handle it as having alpha.
 */

static void
ctk_widget_update_alpha (CtkWidget *widget)
{
  CtkWidgetPrivate *priv;
  CtkStyleContext *context;
  gdouble opacity;
  guint8 alpha;

  priv = widget->priv;

  context = _ctk_widget_get_style_context (widget);
  opacity =
    _ctk_css_number_value_get (_ctk_style_context_peek_property (context,
                                                                 CTK_CSS_PROPERTY_OPACITY),
                               100);
  opacity = CLAMP (opacity, 0.0, 1.0);
  alpha = round (priv->user_alpha * opacity);

  if (alpha == priv->alpha)
    return;

  priv->alpha = alpha;

  if (_ctk_widget_get_realized (widget))
    {
      if (_ctk_widget_is_toplevel (widget) &&
          ctk_widget_get_visual (widget) != cdk_screen_get_rgba_visual (ctk_widget_get_screen (widget)))
	cdk_window_set_opacity (priv->window, priv->alpha / 255.0);

      ctk_widget_queue_draw (widget);
    }
}

/**
 * ctk_widget_set_opacity:
 * @widget: a #CtkWidget
 * @opacity: desired opacity, between 0 and 1
 *
 * Request the @widget to be rendered partially transparent,
 * with opacity 0 being fully transparent and 1 fully opaque. (Opacity values
 * are clamped to the [0,1] range.).
 * This works on both toplevel widget, and child widgets, although there
 * are some limitations:
 *
 * For toplevel widgets this depends on the capabilities of the windowing
 * system. On X11 this has any effect only on X screens with a compositing manager
 * running. See ctk_widget_is_composited(). On Windows it should work
 * always, although setting a window’s opacity after the window has been
 * shown causes it to flicker once on Windows.
 *
 * For child widgets it doesn’t work if any affected widget has a native window, or
 * disables double buffering.
 *
 * Since: 3.8
 **/
void
ctk_widget_set_opacity (CtkWidget *widget,
                        gdouble    opacity)
{
  CtkWidgetPrivate *priv;
  guint8 alpha;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = widget->priv;

  opacity = CLAMP (opacity, 0.0, 1.0);

  alpha = round (opacity * 255);

  if (alpha == priv->user_alpha)
    return;

  priv->user_alpha = alpha;

  ctk_widget_update_alpha (widget);

  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_OPACITY]);
}

/**
 * ctk_widget_get_opacity:
 * @widget: a #CtkWidget
 *
 * Fetches the requested opacity for this widget.
 * See ctk_widget_set_opacity().
 *
 * Returns: the requested opacity for this widget.
 *
 * Since: 3.8
 **/
gdouble
ctk_widget_get_opacity (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0.0);

  return widget->priv->user_alpha / 255.0;
}

static void
_ctk_widget_set_has_focus (CtkWidget *widget,
                           gboolean   has_focus)
{
  widget->priv->has_focus = has_focus;

  if (has_focus)
    ctk_widget_set_state_flags (widget, CTK_STATE_FLAG_FOCUSED, FALSE);
  else
    ctk_widget_unset_state_flags (widget, CTK_STATE_FLAG_FOCUSED);
}

/**
 * ctk_widget_send_focus_change:
 * @widget: a #CtkWidget
 * @event: a #CdkEvent of type CDK_FOCUS_CHANGE
 *
 * Sends the focus change @event to @widget
 *
 * This function is not meant to be used by applications. The only time it
 * should be used is when it is necessary for a #CtkWidget to assign focus
 * to a widget that is semantically owned by the first widget even though
 * it’s not a direct child - for instance, a search entry in a floating
 * window similar to the quick search in #CtkTreeView.
 *
 * An example of its usage is:
 *
 * |[<!-- language="C" -->
 *   CdkEvent *fevent = cdk_event_new (CDK_FOCUS_CHANGE);
 *
 *   fevent->focus_change.type = CDK_FOCUS_CHANGE;
 *   fevent->focus_change.in = TRUE;
 *   fevent->focus_change.window = _ctk_widget_get_window (widget);
 *   if (fevent->focus_change.window != NULL)
 *     g_object_ref (fevent->focus_change.window);
 *
 *   ctk_widget_send_focus_change (widget, fevent);
 *
 *   cdk_event_free (event);
 * ]|
 *
 * Returns: the return value from the event signal emission: %TRUE
 *   if the event was handled, and %FALSE otherwise
 *
 * Since: 2.20
 */
gboolean
ctk_widget_send_focus_change (CtkWidget *widget,
                              CdkEvent  *event)
{
  gboolean res;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (event != NULL && event->type == CDK_FOCUS_CHANGE, FALSE);

  g_object_ref (widget);

  _ctk_widget_set_has_focus (widget, event->focus_change.in);

  res = ctk_widget_event (widget, event);

  g_object_notify_by_pspec (G_OBJECT (widget), widget_props[PROP_HAS_FOCUS]);

  g_object_unref (widget);

  return res;
}

/**
 * ctk_widget_in_destruction:
 * @widget: a #CtkWidget
 *
 * Returns whether the widget is currently being destroyed.
 * This information can sometimes be used to avoid doing
 * unnecessary work.
 *
 * Returns: %TRUE if @widget is being destroyed
 */
gboolean
ctk_widget_in_destruction (CtkWidget *widget)
{
  return widget->priv->in_destruction;
}

gboolean
_ctk_widget_get_in_reparent (CtkWidget *widget)
{
  return widget->priv->in_reparent;
}

void
_ctk_widget_set_in_reparent (CtkWidget *widget,
                             gboolean   in_reparent)
{
  widget->priv->in_reparent = in_reparent;
}

gboolean
_ctk_widget_get_anchored (CtkWidget *widget)
{
  return widget->priv->anchored;
}

void
_ctk_widget_set_anchored (CtkWidget *widget,
                          gboolean   anchored)
{
  widget->priv->anchored = anchored;
}

gboolean
_ctk_widget_get_shadowed (CtkWidget *widget)
{
  return widget->priv->shadowed;
}

void
_ctk_widget_set_shadowed (CtkWidget *widget,
                          gboolean   shadowed)
{
  widget->priv->shadowed = shadowed;
}

gboolean
_ctk_widget_get_alloc_needed (CtkWidget *widget)
{
  return widget->priv->alloc_needed;
}

static void
ctk_widget_set_alloc_needed (CtkWidget *widget)
{
  CtkWidgetPrivate *priv = widget->priv;

  priv->alloc_needed = TRUE;

  do
    {
      if (priv->alloc_needed_on_child)
        break;

      priv->alloc_needed_on_child = TRUE;

      if (!priv->visible)
        break;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      if (CTK_IS_RESIZE_CONTAINER (widget))
        {
          ctk_container_queue_resize_handler (CTK_CONTAINER (widget));
          break;
        }
G_GNUC_END_IGNORE_DEPRECATIONS;

      widget = priv->parent;
      if (widget == NULL)
        break;

      priv = widget->priv;
    }
  while (TRUE);
}

gboolean
ctk_widget_needs_allocate (CtkWidget *widget)
{
  CtkWidgetPrivate *priv = widget->priv;

  if (!priv->visible || !priv->child_visible)
    return FALSE;

  if (priv->resize_needed || priv->alloc_needed || priv->alloc_needed_on_child)
    return TRUE;

  return FALSE;
}

void
ctk_widget_ensure_allocate (CtkWidget *widget)
{
  CtkWidgetPrivate *priv = widget->priv;

  if (!ctk_widget_needs_allocate (widget))
    return;

  ctk_widget_ensure_resize (widget);

  /*  This code assumes that we only reach here if the previous
   *  allocation is still valid (ie no resize was queued).
   *  If that wasn't true, the parent would have taken care of
   *  things.
   */
  if (priv->alloc_needed)
    {
      CtkAllocation allocation;
      int baseline;

      ctk_widget_get_allocated_size (widget, &allocation, &baseline);
      ctk_widget_size_allocate_with_baseline (widget, &allocation, baseline);
    }
  else if (priv->alloc_needed_on_child)
    {
      priv->alloc_needed_on_child = FALSE;

      if (CTK_IS_CONTAINER (widget))
        ctk_container_forall (CTK_CONTAINER (widget),
                              (CtkCallback) ctk_widget_ensure_allocate,
                              NULL);
    }
}

void
ctk_widget_queue_resize_on_widget (CtkWidget *widget)
{
  CtkWidgetPrivate *priv = widget->priv;

  priv->resize_needed = TRUE;
  ctk_widget_set_alloc_needed (widget);
}

void
ctk_widget_ensure_resize (CtkWidget *widget)
{
  CtkWidgetPrivate *priv = widget->priv;

  if (!priv->resize_needed)
    return;

  priv->resize_needed = FALSE;
  _ctk_size_request_cache_clear (&priv->requests);
}

void
_ctk_widget_add_sizegroup (CtkWidget    *widget,
			   gpointer      group)
{
  GSList *groups;

  groups = g_object_get_qdata (G_OBJECT (widget), quark_size_groups);
  groups = g_slist_prepend (groups, group);
  g_object_set_qdata (G_OBJECT (widget), quark_size_groups, groups);

  widget->priv->have_size_groups = TRUE;
}

void
_ctk_widget_remove_sizegroup (CtkWidget    *widget,
			      gpointer      group)
{
  GSList *groups;

  groups = g_object_get_qdata (G_OBJECT (widget), quark_size_groups);
  groups = g_slist_remove (groups, group);
  g_object_set_qdata (G_OBJECT (widget), quark_size_groups, groups);

  widget->priv->have_size_groups = groups != NULL;
}

GSList *
_ctk_widget_get_sizegroups (CtkWidget    *widget)
{
  if (widget->priv->have_size_groups)
    return g_object_get_qdata (G_OBJECT (widget), quark_size_groups);

  return NULL;
}

void
_ctk_widget_add_attached_window (CtkWidget    *widget,
                                 CtkWindow    *window)
{
  widget->priv->attached_windows = g_list_prepend (widget->priv->attached_windows, window);
}

void
_ctk_widget_remove_attached_window (CtkWidget    *widget,
                                    CtkWindow    *window)
{
  widget->priv->attached_windows = g_list_remove (widget->priv->attached_windows, window);
}

/**
 * ctk_widget_path_append_for_widget:
 * @path: a widget path
 * @widget: the widget to append to the widget path
 *
 * Appends the data from @widget to the widget hierarchy represented
 * by @path. This function is a shortcut for adding information from
 * @widget to the given @path. This includes setting the name or
 * adding the style classes from @widget.
 *
 * Returns: the position where the data was inserted
 *
 * Since: 3.2
 */
gint
ctk_widget_path_append_for_widget (CtkWidgetPath *path,
                                   CtkWidget     *widget)
{
  const GQuark *classes;
  guint n_classes, i;
  gint pos;

  g_return_val_if_fail (path != NULL, 0);
  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);

  pos = ctk_widget_path_append_type (path, ctk_css_node_get_widget_type (widget->priv->cssnode));
  ctk_widget_path_iter_set_object_name (path, pos, ctk_css_node_get_name (widget->priv->cssnode));

  if (widget->priv->name)
    ctk_widget_path_iter_set_name (path, pos, widget->priv->name);

  ctk_widget_path_iter_set_state (path, pos, widget->priv->state_flags);

  classes = ctk_css_node_list_classes (widget->priv->cssnode, &n_classes);

  for (i = n_classes; i-- > 0;)
    ctk_widget_path_iter_add_qclass (path, pos, classes[i]);

  return pos;
}

CtkWidgetPath *
_ctk_widget_create_path (CtkWidget *widget)
{
  CtkWidget *parent;

  parent = widget->priv->parent;

  if (parent)
    return ctk_container_get_path_for_child (CTK_CONTAINER (parent), widget);
  else
    {
      /* Widget is either toplevel or unparented, treat both
       * as toplevels style wise, since there are situations
       * where style properties might be retrieved on that
       * situation.
       */
      CtkWidget *attach_widget = NULL;
      CtkWidgetPath *result;

      if (CTK_IS_WINDOW (widget))
        attach_widget = ctk_window_get_attached_to (CTK_WINDOW (widget));

      if (attach_widget != NULL)
        result = ctk_widget_path_copy (ctk_widget_get_path (attach_widget));
      else
        result = ctk_widget_path_new ();

      ctk_widget_path_append_for_widget (result, widget);

      return result;
    }
}

/**
 * ctk_widget_get_path:
 * @widget: a #CtkWidget
 *
 * Returns the #CtkWidgetPath representing @widget, if the widget
 * is not connected to a toplevel widget, a partial path will be
 * created.
 *
 * Returns: (transfer none): The #CtkWidgetPath representing @widget
 **/
CtkWidgetPath *
ctk_widget_get_path (CtkWidget *widget)
{
  CtkWidgetPath *path;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  path = (CtkWidgetPath*)g_object_get_qdata (G_OBJECT (widget), quark_widget_path);
  if (!path)
    {
      path = _ctk_widget_create_path (widget);
      g_object_set_qdata_full (G_OBJECT (widget),
                               quark_widget_path,
                               path,
                               (GDestroyNotify)ctk_widget_path_free);
    }

  return path;
}

void
ctk_widget_clear_path (CtkWidget *widget)
{
  g_object_set_qdata (G_OBJECT (widget), quark_widget_path, NULL);
}

/**
 * ctk_widget_class_set_css_name:
 * @widget_class: class to set the name on
 * @name: name to use
 *
 * Sets the name to be used for CSS matching of widgets.
 *
 * If this function is not called for a given class, the name
 * of the parent class is used.
 *
 * Since: 3.20
 */
void
ctk_widget_class_set_css_name (CtkWidgetClass *widget_class,
                               const char     *name)
{
  CtkWidgetClassPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET_CLASS (widget_class));
  g_return_if_fail (name != NULL);

  priv = widget_class->priv;

  priv->css_name = g_intern_string (name);
}

/**
 * ctk_widget_class_get_css_name:
 * @widget_class: class to set the name on
 *
 * Gets the name used by this class for matching in CSS code. See
 * ctk_widget_class_set_css_name() for details.
 *
 * Returns: the CSS name of the given class
 *
 * Since: 3.20
 */
const char *
ctk_widget_class_get_css_name (CtkWidgetClass *widget_class)
{
  g_return_val_if_fail (CTK_IS_WIDGET_CLASS (widget_class), NULL);

  return widget_class->priv->css_name;
}

void
_ctk_widget_style_context_invalidated (CtkWidget *widget)
{
  g_signal_emit (widget, widget_signals[STYLE_UPDATED], 0);
}

CtkCssNode *
ctk_widget_get_css_node (CtkWidget *widget)
{
  return widget->priv->cssnode;
}

CtkStyleContext *
_ctk_widget_peek_style_context (CtkWidget *widget)
{
  return widget->priv->context;
}


/**
 * ctk_widget_get_style_context:
 * @widget: a #CtkWidget
 *
 * Returns the style context associated to @widget. The returned object is
 * guaranteed to be the same for the lifetime of @widget.
 *
 * Returns: (transfer none): a #CtkStyleContext. This memory is owned by @widget and
 *          must not be freed.
 **/
CtkStyleContext *
ctk_widget_get_style_context (CtkWidget *widget)
{
  CtkWidgetPrivate *priv;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  priv = widget->priv;

  if (G_UNLIKELY (priv->context == NULL))
    {
      CdkScreen *screen;
      CdkFrameClock *frame_clock;

      priv->context = ctk_style_context_new_for_node (priv->cssnode);

      ctk_style_context_set_id (priv->context, priv->name);
      ctk_style_context_set_state (priv->context, priv->state_flags);
      ctk_style_context_set_scale (priv->context, ctk_widget_get_scale_factor (widget));

      screen = ctk_widget_get_screen (widget);
      if (screen)
        ctk_style_context_set_screen (priv->context, screen);

      frame_clock = ctk_widget_get_frame_clock (widget);
      if (frame_clock)
        ctk_style_context_set_frame_clock (priv->context, frame_clock);

      if (priv->parent)
        ctk_style_context_set_parent (priv->context,
                                      _ctk_widget_get_style_context (priv->parent));
    }

  return widget->priv->context;
}

void
_ctk_widget_invalidate_style_context (CtkWidget    *widget,
                                      CtkCssChange  change)
{
  ctk_css_node_invalidate (widget->priv->cssnode, change);
}

/**
 * ctk_widget_get_modifier_mask:
 * @widget: a #CtkWidget
 * @intent: the use case for the modifier mask
 *
 * Returns the modifier mask the @widget’s windowing system backend
 * uses for a particular purpose.
 *
 * See cdk_keymap_get_modifier_mask().
 *
 * Returns: the modifier mask used for @intent.
 *
 * Since: 3.4
 **/
CdkModifierType
ctk_widget_get_modifier_mask (CtkWidget         *widget,
                              CdkModifierIntent  intent)
{
  CdkDisplay *display;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);

  display = ctk_widget_get_display (widget);

  return cdk_keymap_get_modifier_mask (cdk_keymap_get_for_display (display),
                                       intent);
}

CtkStyle *
_ctk_widget_get_style (CtkWidget *widget)
{
  return widget->priv->style;
}

void
_ctk_widget_set_style (CtkWidget *widget,
                       CtkStyle  *style)
{
  widget->priv->style = style;
  g_signal_emit (widget, widget_signals[STYLE_SET], 0, widget->priv->style);
}

CtkActionMuxer *
_ctk_widget_get_parent_muxer (CtkWidget *widget,
                              gboolean   create)
{
  CtkWidget *parent;

  if (CTK_IS_WINDOW (widget))
    return ctk_application_get_parent_muxer_for_window (CTK_WINDOW (widget));

  if (CTK_IS_MENU (widget))
    parent = ctk_menu_get_attach_widget (CTK_MENU (widget));
  else if (CTK_IS_POPOVER (widget))
    parent = ctk_popover_get_relative_to (CTK_POPOVER (widget));
  else
    parent = _ctk_widget_get_parent (widget);

  if (parent)
    return _ctk_widget_get_action_muxer (parent, create);

  return NULL;
}

void
_ctk_widget_update_parent_muxer (CtkWidget *widget)
{
  CtkActionMuxer *muxer;

  muxer = (CtkActionMuxer*)g_object_get_qdata (G_OBJECT (widget), quark_action_muxer);
  if (muxer == NULL)
    return;

  ctk_action_muxer_set_parent (muxer,
                               _ctk_widget_get_parent_muxer (widget, TRUE));
}

CtkActionMuxer *
_ctk_widget_get_action_muxer (CtkWidget *widget,
                              gboolean   create)
{
  CtkActionMuxer *muxer;

  muxer = (CtkActionMuxer*)g_object_get_qdata (G_OBJECT (widget), quark_action_muxer);
  if (muxer)
    return muxer;

  if (create)
    {
      muxer = ctk_action_muxer_new ();
      g_object_set_qdata_full (G_OBJECT (widget),
                               quark_action_muxer,
                               muxer,
                               g_object_unref);
      _ctk_widget_update_parent_muxer (widget);

      return muxer;
    }
  else
    return _ctk_widget_get_parent_muxer (widget, FALSE);
}

/**
 * ctk_widget_insert_action_group:
 * @widget: a #CtkWidget
 * @name: the prefix for actions in @group
 * @group: (allow-none): a #GActionGroup, or %NULL
 *
 * Inserts @group into @widget. Children of @widget that implement
 * #CtkActionable can then be associated with actions in @group by
 * setting their “action-name” to
 * @prefix.`action-name`.
 *
 * If @group is %NULL, a previously inserted group for @name is removed
 * from @widget.
 *
 * Since: 3.6
 */
void
ctk_widget_insert_action_group (CtkWidget    *widget,
                                const gchar  *name,
                                GActionGroup *group)
{
  CtkActionMuxer *muxer;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (name != NULL);

  muxer = _ctk_widget_get_action_muxer (widget, TRUE);

  if (group)
    ctk_action_muxer_insert (muxer, name, group);
  else
    ctk_action_muxer_remove (muxer, name);
}

/****************************************************************
 *                 CtkBuilder automated templates               *
 ****************************************************************/
static AutomaticChildClass *
template_child_class_new (const gchar *name,
                          gboolean     internal_child,
                          gssize       offset)
{
  AutomaticChildClass *child_class = g_slice_new0 (AutomaticChildClass);

  child_class->name = g_strdup (name);
  child_class->internal_child = internal_child;
  child_class->offset = offset;

  return child_class;
}

static void
template_child_class_free (AutomaticChildClass *child_class)
{
  if (child_class)
    {
      g_free (child_class->name);
      g_slice_free (AutomaticChildClass, child_class);
    }
}

static CallbackSymbol *
callback_symbol_new (const gchar *name,
		     GCallback    callback)
{
  CallbackSymbol *cb = g_slice_new0 (CallbackSymbol);

  cb->callback_name = g_strdup (name);
  cb->callback_symbol = callback;

  return cb;
}

static void
callback_symbol_free (CallbackSymbol *callback)
{
  if (callback)
    {
      g_free (callback->callback_name);
      g_slice_free (CallbackSymbol, callback);
    }
}

static void
template_data_free (CtkWidgetTemplate *template_data)
{
  if (template_data)
    {
      g_bytes_unref (template_data->data);
      g_slist_free_full (template_data->children, (GDestroyNotify)template_child_class_free);
      g_slist_free_full (template_data->callbacks, (GDestroyNotify)callback_symbol_free);

      if (template_data->connect_data &&
	  template_data->destroy_notify)
	template_data->destroy_notify (template_data->connect_data);

      g_slice_free (CtkWidgetTemplate, template_data);
    }
}

static GHashTable *
get_auto_child_hash (CtkWidget *widget,
		     GType      type,
		     gboolean   create)
{
  GHashTable *auto_children;
  GHashTable *auto_child_hash;

  auto_children = (GHashTable *)g_object_get_qdata (G_OBJECT (widget), quark_auto_children);
  if (auto_children == NULL)
    {
      if (!create)
        return NULL;

      auto_children = g_hash_table_new_full (g_direct_hash,
                                             NULL,
			                     NULL, (GDestroyNotify)g_hash_table_destroy);
      g_object_set_qdata_full (G_OBJECT (widget),
                               quark_auto_children,
                               auto_children,
                               (GDestroyNotify)g_hash_table_destroy);
    }

  auto_child_hash =
    g_hash_table_lookup (auto_children, GSIZE_TO_POINTER (type));

  if (!auto_child_hash && create)
    {
      auto_child_hash = g_hash_table_new_full (g_str_hash,
					       g_str_equal,
					       NULL,
					       (GDestroyNotify)g_object_unref);

      g_hash_table_insert (auto_children,
			   GSIZE_TO_POINTER (type),
			   auto_child_hash);
    }

  return auto_child_hash;
}

static gboolean
setup_template_child (CtkWidgetTemplate   *template_data,
                      GType                class_type,
                      AutomaticChildClass *child_class,
                      CtkWidget           *widget,
                      CtkBuilder          *builder)
{
  GHashTable *auto_child_hash;
  GObject    *object;

  object = ctk_builder_get_object (builder, child_class->name);
  if (!object)
    {
      g_critical ("Unable to retrieve object '%s' from class template for type '%s' while building a '%s'",
		  child_class->name, g_type_name (class_type), G_OBJECT_TYPE_NAME (widget));
      return FALSE;
    }

  /* Insert into the hash so that it can be fetched with
   * ctk_widget_get_template_child() and also in automated
   * implementations of CtkBuildable.get_internal_child()
   */
  auto_child_hash = get_auto_child_hash (widget, class_type, TRUE);
  g_hash_table_insert (auto_child_hash, child_class->name, g_object_ref (object));

  if (child_class->offset != 0)
    {
      gpointer field_p;

      /* Assign 'object' to the specified offset in the instance (or private) data */
      field_p = G_STRUCT_MEMBER_P (widget, child_class->offset);
      (* (gpointer *) field_p) = object;
    }

  return TRUE;
}

/**
 * ctk_widget_init_template:
 * @widget: a #CtkWidget
 *
 * Creates and initializes child widgets defined in templates. This
 * function must be called in the instance initializer for any
 * class which assigned itself a template using ctk_widget_class_set_template()
 *
 * It is important to call this function in the instance initializer
 * of a #CtkWidget subclass and not in #GObject.constructed() or
 * #GObject.constructor() for two reasons.
 *
 * One reason is that generally derived widgets will assume that parent
 * class composite widgets have been created in their instance
 * initializers.
 *
 * Another reason is that when calling g_object_new() on a widget with
 * composite templates, it’s important to build the composite widgets
 * before the construct properties are set. Properties passed to g_object_new()
 * should take precedence over properties set in the private template XML.
 *
 * Since: 3.10
 */
void
ctk_widget_init_template (CtkWidget *widget)
{
  CtkWidgetTemplate *template;
  CtkBuilder *builder;
  GError *error = NULL;
  GObject *object;
  GSList *l;
  GType class_type;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  object = G_OBJECT (widget);
  class_type = G_OBJECT_TYPE (widget);

  template = CTK_WIDGET_GET_CLASS (widget)->priv->template;
  g_return_if_fail (template != NULL);

  builder = ctk_builder_new ();

  /* Add any callback symbols declared for this GType to the CtkBuilder namespace */
  for (l = template->callbacks; l; l = l->next)
    {
      CallbackSymbol *callback = l->data;

      ctk_builder_add_callback_symbol (builder, callback->callback_name, callback->callback_symbol);
    }

  /* This will build the template XML as children to the widget instance, also it
   * will validate that the template is created for the correct GType and assert that
   * there is no infinite recursion.
   */
  if (!ctk_builder_extend_with_template  (builder, widget, class_type,
					  (const gchar *)g_bytes_get_data (template->data, NULL),
					  g_bytes_get_size (template->data),
					  &error))
    {
      g_critical ("Error building template class '%s' for an instance of type '%s': %s",
		  g_type_name (class_type), G_OBJECT_TYPE_NAME (object), error->message);
      g_error_free (error);

      /* This should never happen, if the template XML cannot be built
       * then it is a critical programming error.
       */
      g_object_unref (builder);
      return;
    }

  /* Build the automatic child data
   */
  for (l = template->children; l; l = l->next)
    {
      AutomaticChildClass *child_class = l->data;

      /* This will setup the pointer of an automated child, and cause
       * it to be available in any CtkBuildable.get_internal_child()
       * invocations which may follow by reference in child classes.
       */
      if (!setup_template_child (template,
				  class_type,
				  child_class,
				  widget,
				  builder))
	{
	  g_object_unref (builder);
	  return;
	}
    }

  /* Connect signals. All signal data from a template receive the 
   * template instance as user data automatically.
   *
   * A CtkBuilderConnectFunc can be provided to ctk_widget_class_set_signal_connect_func()
   * in order for templates to be usable by bindings.
   */
  if (template->connect_func)
    ctk_builder_connect_signals_full (builder, template->connect_func, template->connect_data);
  else
    ctk_builder_connect_signals (builder, object);

  g_object_unref (builder);
}

/**
 * ctk_widget_class_set_template:
 * @widget_class: A #CtkWidgetClass
 * @template_bytes: A #GBytes holding the #CtkBuilder XML 
 *
 * This should be called at class initialization time to specify
 * the CtkBuilder XML to be used to extend a widget.
 *
 * For convenience, ctk_widget_class_set_template_from_resource() is also provided.
 *
 * Note that any class that installs templates must call ctk_widget_init_template()
 * in the widget’s instance initializer.
 *
 * Since: 3.10
 */
void
ctk_widget_class_set_template (CtkWidgetClass    *widget_class,
			       GBytes            *template_bytes)
{
  g_return_if_fail (CTK_IS_WIDGET_CLASS (widget_class));
  g_return_if_fail (widget_class->priv->template == NULL);
  g_return_if_fail (template_bytes != NULL);

  widget_class->priv->template = g_slice_new0 (CtkWidgetTemplate);
  widget_class->priv->template->data = g_bytes_ref (template_bytes);
}

/**
 * ctk_widget_class_set_template_from_resource:
 * @widget_class: A #CtkWidgetClass
 * @resource_name: The name of the resource to load the template from
 *
 * A convenience function to call ctk_widget_class_set_template().
 *
 * Note that any class that installs templates must call ctk_widget_init_template()
 * in the widget’s instance initializer.
 *
 * Since: 3.10
 */
void
ctk_widget_class_set_template_from_resource (CtkWidgetClass    *widget_class,
					     const gchar       *resource_name)
{
  GError *error = NULL;
  GBytes *bytes = NULL;

  g_return_if_fail (CTK_IS_WIDGET_CLASS (widget_class));
  g_return_if_fail (widget_class->priv->template == NULL);
  g_return_if_fail (resource_name && resource_name[0]);

  /* This is a hack, because class initializers now access resources
   * and GIR/gtk-doc initializes classes without initializing CTK+,
   * we ensure that our base resources are registered here and
   * avoid warnings which building GIRs/documentation.
   */
  _ctk_ensure_resources ();

  bytes = g_resources_lookup_data (resource_name, 0, &error);
  if (!bytes)
    {
      g_critical ("Unable to load resource for composite template for type '%s': %s",
		  G_OBJECT_CLASS_NAME (widget_class), error->message);
      g_error_free (error);
      return;
    }

  ctk_widget_class_set_template (widget_class, bytes);
  g_bytes_unref (bytes);
}

/**
 * ctk_widget_class_bind_template_callback_full:
 * @widget_class: A #CtkWidgetClass
 * @callback_name: The name of the callback as expected in the template XML
 * @callback_symbol: (scope async): The callback symbol
 *
 * Declares a @callback_symbol to handle @callback_name from the template XML
 * defined for @widget_type. See ctk_builder_add_callback_symbol().
 *
 * Note that this must be called from a composite widget classes class
 * initializer after calling ctk_widget_class_set_template().
 *
 * Since: 3.10
 */
void
ctk_widget_class_bind_template_callback_full (CtkWidgetClass *widget_class,
                                              const gchar    *callback_name,
                                              GCallback       callback_symbol)
{
  CallbackSymbol *cb;

  g_return_if_fail (CTK_IS_WIDGET_CLASS (widget_class));
  g_return_if_fail (widget_class->priv->template != NULL);
  g_return_if_fail (callback_name && callback_name[0]);
  g_return_if_fail (callback_symbol != NULL);

  cb = callback_symbol_new (callback_name, callback_symbol);
  widget_class->priv->template->callbacks = g_slist_prepend (widget_class->priv->template->callbacks, cb);
}

/**
 * ctk_widget_class_set_connect_func:
 * @widget_class: A #CtkWidgetClass
 * @connect_func: The #CtkBuilderConnectFunc to use when connecting signals in the class template
 * @connect_data: The data to pass to @connect_func
 * @connect_data_destroy: The #GDestroyNotify to free @connect_data, this will only be used at
 *                        class finalization time, when no classes of type @widget_type are in use anymore.
 *
 * For use in language bindings, this will override the default #CtkBuilderConnectFunc to be
 * used when parsing CtkBuilder XML from this class’s template data.
 *
 * Note that this must be called from a composite widget classes class
 * initializer after calling ctk_widget_class_set_template().
 *
 * Since: 3.10
 */
void
ctk_widget_class_set_connect_func (CtkWidgetClass        *widget_class,
				   CtkBuilderConnectFunc  connect_func,
				   gpointer               connect_data,
				   GDestroyNotify         connect_data_destroy)
{
  g_return_if_fail (CTK_IS_WIDGET_CLASS (widget_class));
  g_return_if_fail (widget_class->priv->template != NULL);

  /* Defensive, destroy any previously set data */
  if (widget_class->priv->template->connect_data &&
      widget_class->priv->template->destroy_notify)
    widget_class->priv->template->destroy_notify (widget_class->priv->template->connect_data);

  widget_class->priv->template->connect_func   = connect_func;
  widget_class->priv->template->connect_data   = connect_data;
  widget_class->priv->template->destroy_notify = connect_data_destroy;
}

/**
 * ctk_widget_class_bind_template_child_full:
 * @widget_class: A #CtkWidgetClass
 * @name: The “id” of the child defined in the template XML
 * @internal_child: Whether the child should be accessible as an “internal-child”
 *                  when this class is used in CtkBuilder XML
 * @struct_offset: The structure offset into the composite widget’s instance public or private structure
 *                 where the automated child pointer should be set, or 0 to not assign the pointer.
 *
 * Automatically assign an object declared in the class template XML to be set to a location
 * on a freshly built instance’s private data, or alternatively accessible via ctk_widget_get_template_child().
 *
 * The struct can point either into the public instance, then you should use G_STRUCT_OFFSET(WidgetType, member)
 * for @struct_offset,  or in the private struct, then you should use G_PRIVATE_OFFSET(WidgetType, member).
 *
 * An explicit strong reference will be held automatically for the duration of your
 * instance’s life cycle, it will be released automatically when #GObjectClass.dispose() runs
 * on your instance and if a @struct_offset that is != 0 is specified, then the automatic location
 * in your instance public or private data will be set to %NULL. You can however access an automated child
 * pointer the first time your classes #GObjectClass.dispose() runs, or alternatively in
 * #CtkWidgetClass.destroy().
 *
 * If @internal_child is specified, #CtkBuildableIface.get_internal_child() will be automatically
 * implemented by the #CtkWidget class so there is no need to implement it manually.
 *
 * The wrapper macros ctk_widget_class_bind_template_child(), ctk_widget_class_bind_template_child_internal(),
 * ctk_widget_class_bind_template_child_private() and ctk_widget_class_bind_template_child_internal_private()
 * might be more convenient to use.
 *
 * Note that this must be called from a composite widget classes class
 * initializer after calling ctk_widget_class_set_template().
 *
 * Since: 3.10
 */
void
ctk_widget_class_bind_template_child_full (CtkWidgetClass *widget_class,
                                           const gchar    *name,
                                           gboolean        internal_child,
                                           gssize          struct_offset)
{
  AutomaticChildClass *child_class;

  g_return_if_fail (CTK_IS_WIDGET_CLASS (widget_class));
  g_return_if_fail (widget_class->priv->template != NULL);
  g_return_if_fail (name && name[0]);

  child_class = template_child_class_new (name,
                                          internal_child,
                                          struct_offset);
  widget_class->priv->template->children =
    g_slist_prepend (widget_class->priv->template->children, child_class);
}

/**
 * ctk_widget_get_template_child:
 * @widget: A #CtkWidget
 * @widget_type: The #GType to get a template child for
 * @name: The “id” of the child defined in the template XML
 *
 * Fetch an object build from the template XML for @widget_type in this @widget instance.
 *
 * This will only report children which were previously declared with
 * ctk_widget_class_bind_template_child_full() or one of its
 * variants.
 *
 * This function is only meant to be called for code which is private to the @widget_type which
 * declared the child and is meant for language bindings which cannot easily make use
 * of the GObject structure offsets.
 *
 * Returns: (transfer none): The object built in the template XML with the id @name
 */
GObject *
ctk_widget_get_template_child (CtkWidget   *widget,
                               GType        widget_type,
                               const gchar *name)
{
  GHashTable *auto_child_hash;
  GObject *ret = NULL;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (g_type_name (widget_type) != NULL, NULL);
  g_return_val_if_fail (name && name[0], NULL);

  auto_child_hash = get_auto_child_hash (widget, widget_type, FALSE);

  if (auto_child_hash)
    ret = g_hash_table_lookup (auto_child_hash, name);

  return ret;
}

/**
 * ctk_widget_list_action_prefixes:
 * @widget: A #CtkWidget
 *
 * Retrieves a %NULL-terminated array of strings containing the prefixes of
 * #GActionGroup's available to @widget.
 *
 * Returns: (transfer container): a %NULL-terminated array of strings.
 *
 * Since: 3.16
 */
const gchar **
ctk_widget_list_action_prefixes (CtkWidget *widget)
{
  CtkActionMuxer *muxer;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  muxer = _ctk_widget_get_action_muxer (widget, FALSE);
  if (muxer)
    return ctk_action_muxer_list_prefixes (muxer);

  return g_new0 (const gchar *, 1);
}

/**
 * ctk_widget_get_action_group:
 * @widget: A #CtkWidget
 * @prefix: The “prefix” of the action group.
 *
 * Retrieves the #GActionGroup that was registered using @prefix. The resulting
 * #GActionGroup may have been registered to @widget or any #CtkWidget in its
 * ancestry.
 *
 * If no action group was found matching @prefix, then %NULL is returned.
 *
 * Returns: (transfer none) (nullable): A #GActionGroup or %NULL.
 *
 * Since: 3.16
 */
GActionGroup *
ctk_widget_get_action_group (CtkWidget   *widget,
                             const gchar *prefix)
{
  CtkActionMuxer *muxer;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (prefix, NULL);

  muxer = _ctk_widget_get_action_muxer (widget, FALSE);
  if (muxer)
    return ctk_action_muxer_lookup (muxer, prefix);

  return NULL;
}

static void
event_controller_grab_notify (CtkWidget           *widget,
                              gboolean             was_grabbed,
                              EventControllerData *data)
{
  CdkDevice *device = NULL;

  if (CTK_IS_GESTURE (data->controller))
    device = ctk_gesture_get_device (CTK_GESTURE (data->controller));

  if (!device || !ctk_widget_device_is_shadowed (widget, device))
    return;

  ctk_event_controller_reset (data->controller);
}

static void
_ctk_widget_update_evmask (CtkWidget *widget)
{
  if (_ctk_widget_get_realized (widget))
    {
      gint events = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (widget),
                                                         quark_event_mask));
      ctk_widget_add_events_internal (widget, NULL, events);
    }
}

static void
event_controller_sequence_state_changed (CtkGesture            *gesture,
                                         CdkEventSequence      *sequence,
                                         CtkEventSequenceState  state,
                                         CtkWidget             *widget)
{
  gboolean handled = FALSE;
  CtkWidget *event_widget;
  const CdkEvent *event;

  handled = _ctk_widget_set_sequence_state_internal (widget, sequence,
                                                     state, gesture);

  if (!handled || state != CTK_EVENT_SEQUENCE_CLAIMED)
    return;

  event = _ctk_widget_get_last_event (widget, sequence);

  if (!event)
    return;

  event_widget = ctk_get_event_widget ((CdkEvent *) event);
  cancel_event_sequence_on_hierarchy (widget, event_widget, sequence);
}

static EventControllerData *
_ctk_widget_has_controller (CtkWidget          *widget,
                            CtkEventController *controller)
{
  EventControllerData *data;
  CtkWidgetPrivate *priv;
  GList *l;

  priv = widget->priv;

  for (l = priv->event_controllers; l; l = l->next)
    {
      data = l->data;

      if (data->controller == controller)
        return data;
    }

  return NULL;
}

void
_ctk_widget_add_controller (CtkWidget          *widget,
                            CtkEventController *controller)
{
  EventControllerData *data;
  CtkWidgetPrivate *priv;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CTK_IS_EVENT_CONTROLLER (controller));
  g_return_if_fail (widget == ctk_event_controller_get_widget (controller));

  priv = widget->priv;
  data = _ctk_widget_has_controller (widget, controller);

  if (data)
    return;

  data = g_new0 (EventControllerData, 1);
  data->controller = controller;
  data->grab_notify_id =
    g_signal_connect (widget, "grab-notify",
                      G_CALLBACK (event_controller_grab_notify), data);

  g_object_add_weak_pointer (G_OBJECT (data->controller), (gpointer *) &data->controller);

  if (CTK_IS_GESTURE (controller))
    {
      data->sequence_state_changed_id =
        g_signal_connect (controller, "sequence-state-changed",
                          G_CALLBACK (event_controller_sequence_state_changed),
                          widget);
    }

  priv->event_controllers = g_list_prepend (priv->event_controllers, data);
  _ctk_widget_update_evmask (widget);
}

void
_ctk_widget_remove_controller (CtkWidget          *widget,
                               CtkEventController *controller)
{
  EventControllerData *data;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CTK_IS_EVENT_CONTROLLER (controller));

  data = _ctk_widget_has_controller (widget, controller);

  if (!data)
    return;

  g_object_remove_weak_pointer (G_OBJECT (data->controller), (gpointer *) &data->controller);

  if (g_signal_handler_is_connected (widget, data->grab_notify_id))
    g_signal_handler_disconnect (widget, data->grab_notify_id);

  if (data->sequence_state_changed_id)
    g_signal_handler_disconnect (data->controller, data->sequence_state_changed_id);

  data->controller = NULL;
}

GList *
_ctk_widget_list_controllers (CtkWidget           *widget,
                              CtkPropagationPhase  phase)
{
  EventControllerData *data;
  CtkWidgetPrivate *priv;
  GList *l, *retval = NULL;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  priv = widget->priv;

  for (l = priv->event_controllers; l; l = l->next)
    {
      data = l->data;

      if (data->controller != NULL &&
          phase == ctk_event_controller_get_propagation_phase (data->controller))
        retval = g_list_prepend (retval, data->controller);
    }

  return retval;
}

gboolean
_ctk_widget_consumes_motion (CtkWidget        *widget,
                             CdkEventSequence *sequence)
{
  EventControllerData *data;
  CtkWidgetPrivate *priv;
  GList *l;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  priv = widget->priv;

  for (l = priv->event_controllers; l; l = l->next)
    {
      data = l->data;

      if (data->controller == NULL)
        continue;

      if ((!CTK_IS_GESTURE_SINGLE (data->controller) ||
           CTK_IS_GESTURE_DRAG (data->controller) ||
           CTK_IS_GESTURE_SWIPE (data->controller)) &&
          ctk_gesture_handles_sequence (CTK_GESTURE (data->controller), sequence))
        return TRUE;
    }

  return FALSE;
}

void
ctk_widget_reset_controllers (CtkWidget *widget)
{
  EventControllerData *controller_data;
  CtkWidgetPrivate *priv = widget->priv;
  GList *l;

  /* Reset all controllers */
  for (l = priv->event_controllers; l; l = l->next)
    {
      controller_data = l->data;

      if (controller_data->controller == NULL)
        continue;

      ctk_event_controller_reset (controller_data->controller);
    }
}

void
ctk_widget_render (CtkWidget            *widget,
                   CdkWindow            *window,
                   const cairo_region_t *region)
{
  CtkWidgetPrivate *priv = ctk_widget_get_instance_private (widget);
  CdkDrawingContext *context;
  gboolean do_clip;
  cairo_t *cr;
  int x, y;
  gboolean is_double_buffered;

  /* We take the value here, in case somebody manages to changes
   * the double_buffered value inside a ::draw call, and ends up
   * breaking everything.
   */
  is_double_buffered = priv->double_buffered;
  if (is_double_buffered)
    {
      /* We only render double buffered on native windows */
      if (!cdk_window_has_native (window))
        return;

      context = cdk_window_begin_draw_frame (window, region);
      cr = cdk_drawing_context_get_cairo_context (context);
    }
  else
    {
      /* This is annoying, but it has to stay because Firefox
       * disables double buffering on a top-level CdkWindow,
       * which breaks the drawing context.
       *
       * Candidate for deletion in the next major API bump.
       */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      cr = cdk_cairo_create (window);
G_GNUC_END_IGNORE_DEPRECATIONS
    }

  do_clip = _ctk_widget_get_translation_to_window (widget, window, &x, &y);
  cairo_translate (cr, -x, -y);

  ctk_widget_draw_internal (widget, cr, do_clip);

  if (is_double_buffered)
    cdk_window_end_draw_frame (window, context);
  else
    cairo_destroy (cr);
}
