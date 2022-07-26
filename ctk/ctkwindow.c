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

#include "ctkwindow.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "ctkprivate.h"
#include "ctkwindowprivate.h"
#include "ctkaccelgroupprivate.h"
#include "ctkbindings.h"
#include "ctkcsscornervalueprivate.h"
#include "ctkcssiconthemevalueprivate.h"
#include "ctkcssrgbavalueprivate.h"
#include "ctkcssshadowsvalueprivate.h"
#include "ctkkeyhash.h"
#include "ctkmain.h"
#include "ctkmnemonichash.h"
#include "ctkmenubar.h"
#include "ctkmenushellprivate.h"
#include "ctkicontheme.h"
#include "ctkmarshalers.h"
#include "ctkplug.h"
#include "ctkbuildable.h"
#include "ctkbuilderprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkcontainerprivate.h"
#include "ctkintl.h"
#include "ctkstylecontextprivate.h"
#include "ctktypebuiltins.h"
#include "ctkbox.h"
#include "ctkbutton.h"
#include "ctkheaderbar.h"
#include "ctkheaderbarprivate.h"
#include "ctkpopoverprivate.h"
#include "a11y/ctkwindowaccessible.h"
#include "a11y/ctkcontaineraccessibleprivate.h"
#include "ctkapplicationprivate.h"
#include "ctkgestureprivate.h"
#include "inspector/init.h"
#include "inspector/window.h"
#include "ctkcssstylepropertyprivate.h"

#include "cdk/cdk-private.h"

#ifdef CDK_WINDOWING_X11
#include "x11/cdkx.h"
#endif

#ifdef CDK_WINDOWING_WIN32
#include "win32/cdkwin32.h"
#endif

#ifdef CDK_WINDOWING_WAYLAND
#include "wayland/cdkwayland.h"
#endif

#ifdef CDK_WINDOWING_BROADWAY
#include "broadway/cdkbroadway.h"
#endif

/**
 * SECTION:ctkwindow
 * @title: CtkWindow
 * @short_description: Toplevel which can contain other widgets
 *
 * A CtkWindow is a toplevel window which can contain other widgets.
 * Windows normally have decorations that are under the control
 * of the windowing system and allow the user to manipulate the window
 * (resize it, move it, close it,...).
 *
 * # CtkWindow as CtkBuildable
 *
 * The CtkWindow implementation of the #CtkBuildable interface supports a
 * custom <accel-groups> element, which supports any number of <group>
 * elements representing the #CtkAccelGroup objects you want to add to
 * your window (synonymous with ctk_window_add_accel_group().
 *
 * It also supports the <initial-focus> element, whose name property names
 * the widget to receive the focus when the window is mapped.
 *
 * An example of a UI definition fragment with accel groups:
 * |[
 * <object class="CtkWindow">
 *   <accel-groups>
 *     <group name="accelgroup1"/>
 *   </accel-groups>
 *   <initial-focus name="thunderclap"/>
 * </object>
 * 
 * ...
 * 
 * <object class="CtkAccelGroup" id="accelgroup1"/>
 * ]|
 * 
 * The CtkWindow implementation of the #CtkBuildable interface supports
 * setting a child as the titlebar by specifying “titlebar” as the “type”
 * attribute of a <child> element.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * window.background
 * ├── decoration
 * ├── <titlebar child>.titlebar [.default-decoration]
 * ╰── <child>
 * ]|
 *
 * CtkWindow has a main CSS node with name window and style class .background,
 * and a subnode with name decoration.
 *
 * Style classes that are typically used with the main CSS node are .csd (when
 * client-side decorations are in use), .solid-csd (for client-side decorations
 * without invisible borders), .ssd (used by mutter when rendering server-side
 * decorations). CtkWindow also represents window states with the following
 * style classes on the main node: .tiled, .maximized, .fullscreen. Specialized
 * types of window often add their own discriminating style classes, such as
 * .popup or .tooltip.
 *
 * CtkWindow adds the .titlebar and .default-decoration style classes to the
 * widget that is added as a titlebar child.
 */

#define MNEMONICS_DELAY 300 /* ms */
#define NO_CONTENT_CHILD_NAT 200
/* In case the content (excluding header bar and shadows) of the window
 * would be empty, either because there is no visible child widget or only an
 * empty container widget, we use NO_CONTENT_CHILD_NAT as natural width/height
 * instead.
 */

typedef struct _CtkWindowPopover CtkWindowPopover;

struct _CtkWindowPopover
{
  CtkWidget *widget;
  CtkWidget *parent;
  CdkWindow *window;
  CtkPositionType pos;
  cairo_rectangle_int_t rect;
  gulong unmap_id;
  guint clamp_allocation : 1;
};

struct _CtkWindowPrivate
{
  CtkMnemonicHash       *mnemonic_hash;

  CtkWidget             *attach_widget;
  CtkWidget             *default_widget;
  CtkWidget             *initial_focus;
  CtkWidget             *focus_widget;
  CtkWindow             *transient_parent;
  CtkWindowGeometryInfo *geometry_info;
  CtkWindowGroup        *group;
  CdkScreen             *screen;
  CdkDisplay            *display;
  CtkApplication        *application;

  GList                 *popovers;

  CdkModifierType        mnemonic_modifier;

  gchar   *startup_id;
  gchar   *title;
  gchar   *wmclass_class;
  gchar   *wmclass_name;
  gchar   *wm_role;

  guint    keys_changed_handler;
  guint    delete_event_handler;

  guint32  initial_timestamp;

  guint16  configure_request_count;

  guint    mnemonics_display_timeout_id;

  gint     scale;

  gint title_height;
  CtkWidget *title_box;
  CtkWidget *titlebar;
  CtkWidget *popup_menu;

  CdkWindow *border_window[8];
  gint       initial_fullscreen_monitor;
  guint      edge_constraints;

  /* The following flags are initially TRUE (before a window is mapped).
   * They cause us to compute a configure request that involves
   * default-only parameters. Once mapped, we set them to FALSE.
   * Then we set them to TRUE again on unmap (for position)
   * and on unrealize (for size).
   */
  guint    need_default_position     : 1;
  guint    need_default_size         : 1;

  guint    above_initially           : 1;
  guint    accept_focus              : 1;
  guint    below_initially           : 1;
  guint    builder_visible           : 1;
  guint    configure_notify_received : 1;
  guint    decorated                 : 1;
  guint    deletable                 : 1;
  guint    destroy_with_parent       : 1;
  guint    focus_on_map              : 1;
  guint    fullscreen_initially      : 1;
  guint    has_focus                 : 1;
  guint    has_user_ref_count        : 1;
  guint    has_toplevel_focus        : 1;
  guint    hide_titlebar_when_maximized : 1;
  guint    iconify_initially         : 1; /* ctk_window_iconify() called before realization */
  guint    is_active                 : 1;
  guint    maximize_initially        : 1;
  guint    mnemonics_visible         : 1;
  guint    mnemonics_visible_set     : 1;
  guint    focus_visible             : 1;
  guint    modal                     : 1;
  guint    position                  : 3;
  guint    resizable                 : 1;
  guint    skips_pager               : 1;
  guint    skips_taskbar             : 1;
  guint    stick_initially           : 1;
  guint    transient_parent_group    : 1;
  guint    type                      : 4; /* CtkWindowType */
  guint    urgent                    : 1;
  guint    gravity                   : 5; /* CdkGravity */
  guint    csd_requested             : 1;
  guint    client_decorated          : 1; /* Decorations drawn client-side */
  guint    use_client_shadow         : 1; /* Decorations use client-side shadows */
  guint    maximized                 : 1;
  guint    fullscreen                : 1;
  guint    tiled                     : 1;
  guint    unlimited_guessed_size_x  : 1;
  guint    unlimited_guessed_size_y  : 1;
  guint    force_resize              : 1;
  guint    fixate_size               : 1;

  guint    use_subsurface            : 1;

  CdkWindowTypeHint type_hint;

  CtkGesture *multipress_gesture;
  CtkGesture *drag_gesture;

  CdkWindow *hardcoded_window;

  CtkCssNode *decoration_node;
};

#ifdef CDK_WINDOWING_X11
static const CtkTargetEntry dnd_dest_targets [] = {
  { "application/x-rootwindow-drop", 0, 0 },
};
#endif

enum {
  SET_FOCUS,
  ACTIVATE_FOCUS,
  ACTIVATE_DEFAULT,
  KEYS_CHANGED,
  ENABLE_DEBUGGING,
  LAST_SIGNAL
};

enum {
  PROP_0,

  /* Construct */
  PROP_TYPE,

  /* Normal Props */
  PROP_TITLE,
  PROP_ROLE,
  PROP_RESIZABLE,
  PROP_MODAL,
  PROP_WIN_POS,
  PROP_DEFAULT_WIDTH,
  PROP_DEFAULT_HEIGHT,
  PROP_DESTROY_WITH_PARENT,
  PROP_HIDE_TITLEBAR_WHEN_MAXIMIZED,
  PROP_ICON,
  PROP_ICON_NAME,
  PROP_SCREEN,
  PROP_TYPE_HINT,
  PROP_SKIP_TASKBAR_HINT,
  PROP_SKIP_PAGER_HINT,
  PROP_URGENCY_HINT,
  PROP_ACCEPT_FOCUS,
  PROP_FOCUS_ON_MAP,
  PROP_DECORATED,
  PROP_DELETABLE,
  PROP_GRAVITY,
  PROP_TRANSIENT_FOR,
  PROP_ATTACHED_TO,
  PROP_HAS_RESIZE_GRIP,
  PROP_RESIZE_GRIP_VISIBLE,
  PROP_APPLICATION,
  /* Readonly properties */
  PROP_IS_ACTIVE,
  PROP_HAS_TOPLEVEL_FOCUS,

  /* Writeonly properties */
  PROP_STARTUP_ID,

  PROP_MNEMONICS_VISIBLE,
  PROP_FOCUS_VISIBLE,

  PROP_IS_MAXIMIZED,

  LAST_ARG
};

static GParamSpec *window_props[LAST_ARG] = { NULL, };

/* Must be kept in sync with CdkWindowEdge ! */
typedef enum
{
  CTK_WINDOW_REGION_EDGE_NW,
  CTK_WINDOW_REGION_EDGE_N,
  CTK_WINDOW_REGION_EDGE_NE,
  CTK_WINDOW_REGION_EDGE_W,
  CTK_WINDOW_REGION_EDGE_E,
  CTK_WINDOW_REGION_EDGE_SW,
  CTK_WINDOW_REGION_EDGE_S,
  CTK_WINDOW_REGION_EDGE_SE,
  CTK_WINDOW_REGION_CONTENT,
  CTK_WINDOW_REGION_TITLE,
} CtkWindowRegion;

typedef struct
{
  GList     *icon_list;
  gchar     *icon_name;
  guint      realized : 1;
  guint      using_default_icon : 1;
  guint      using_parent_icon : 1;
  guint      using_themed_icon : 1;
} CtkWindowIconInfo;

typedef struct {
  CdkGeometry    geometry; /* Last set of geometry hints we set */
  CdkWindowHints flags;
  CdkRectangle   configure_request;
} CtkWindowLastGeometryInfo;

struct _CtkWindowGeometryInfo
{
  /* Properties that the app has set on the window
   */
  CdkGeometry    geometry;	/* Geometry hints */
  CdkWindowHints mask;
  /* from last ctk_window_resize () - if > 0, indicates that
   * we should resize to this size.
   */
  gint           resize_width;  
  gint           resize_height;

  /* From last ctk_window_move () prior to mapping -
   * only used if initial_pos_set
   */
  gint           initial_x;
  gint           initial_y;
  
  /* Default size - used only the FIRST time we map a window,
   * only if > 0.
   */
  gint           default_width; 
  gint           default_height;
  /* whether to use initial_x, initial_y */
  guint          initial_pos_set : 1;
  /* CENTER_ALWAYS or other position constraint changed since
   * we sent the last configure request.
   */
  guint          position_constraints_changed : 1;

  /* if true, default_width, height should be multiplied by the
   * increments and affect the geometry widget only
   */
  guint          default_is_geometry : 1;

  CtkWindowLastGeometryInfo last;
};


static void ctk_window_constructed        (GObject           *object);
static void ctk_window_dispose            (GObject           *object);
static void ctk_window_finalize           (GObject           *object);
static void ctk_window_destroy            (CtkWidget         *widget);
static void ctk_window_show               (CtkWidget         *widget);
static void ctk_window_hide               (CtkWidget         *widget);
static void ctk_window_map                (CtkWidget         *widget);
static void ctk_window_unmap              (CtkWidget         *widget);
static void ctk_window_realize            (CtkWidget         *widget);
static void ctk_window_unrealize          (CtkWidget         *widget);
static void ctk_window_size_allocate      (CtkWidget         *widget,
					   CtkAllocation     *allocation);
static gboolean ctk_window_map_event      (CtkWidget         *widget,
                                           CdkEventAny       *event);
static gint ctk_window_configure_event    (CtkWidget         *widget,
					   CdkEventConfigure *event);
static gboolean ctk_window_event          (CtkWidget         *widget,
                                           CdkEvent          *event);
static gint ctk_window_key_press_event    (CtkWidget         *widget,
					   CdkEventKey       *event);
static gint ctk_window_key_release_event  (CtkWidget         *widget,
					   CdkEventKey       *event);
static gint ctk_window_focus_in_event     (CtkWidget         *widget,
					   CdkEventFocus     *event);
static gint ctk_window_focus_out_event    (CtkWidget         *widget,
					   CdkEventFocus     *event);
static gboolean ctk_window_state_event    (CtkWidget          *widget,
                                           CdkEventWindowState *event);
static void ctk_window_remove             (CtkContainer      *container,
                                           CtkWidget         *widget);
static void ctk_window_check_resize       (CtkContainer      *container);
static void ctk_window_forall             (CtkContainer   *container,
					   gboolean	include_internals,
					   CtkCallback     callback,
					   gpointer        callback_data);
static gint ctk_window_focus              (CtkWidget        *widget,
				           CtkDirectionType  direction);
static void ctk_window_move_focus         (CtkWidget         *widget,
                                           CtkDirectionType   dir);
static void ctk_window_real_set_focus     (CtkWindow         *window,
					   CtkWidget         *focus);

static void ctk_window_real_activate_default (CtkWindow         *window);
static void ctk_window_real_activate_focus   (CtkWindow         *window);
static void ctk_window_keys_changed          (CtkWindow         *window);
static gboolean ctk_window_enable_debugging  (CtkWindow         *window,
                                              gboolean           toggle);
static gint ctk_window_draw                  (CtkWidget         *widget,
					      cairo_t           *cr);
static void ctk_window_unset_transient_for         (CtkWindow  *window);
static void ctk_window_transient_parent_realized   (CtkWidget  *parent,
						    CtkWidget  *window);
static void ctk_window_transient_parent_unrealized (CtkWidget  *parent,
						    CtkWidget  *window);

static CdkScreen *ctk_window_check_screen (CtkWindow *window);

static CtkWindowGeometryInfo* ctk_window_get_geometry_info         (CtkWindow    *window,
                                                                    gboolean      create);

static gboolean ctk_window_compare_hints             (CdkGeometry  *geometry_a,
                                                      guint         flags_a,
                                                      CdkGeometry  *geometry_b,
                                                      guint         flags_b);
static void     ctk_window_constrain_size            (CtkWindow    *window,
                                                      CdkGeometry  *geometry,
                                                      guint         flags,
                                                      gint          width,
                                                      gint          height,
                                                      gint         *new_width,
                                                      gint         *new_height);
static void     ctk_window_constrain_position        (CtkWindow    *window,
                                                      gint          new_width,
                                                      gint          new_height,
                                                      gint         *x,
                                                      gint         *y);
static void     ctk_window_update_fixed_size         (CtkWindow    *window,
                                                      CdkGeometry  *new_geometry,
                                                      gint          new_width,
                                                      gint          new_height);
static void     ctk_window_compute_hints             (CtkWindow    *window,
                                                      CdkGeometry  *new_geometry,
                                                      guint        *new_flags);
static void     ctk_window_compute_configure_request (CtkWindow    *window,
                                                      CdkRectangle *request,
                                                      CdkGeometry  *geometry,
                                                      guint        *flags);

static void     ctk_window_set_default_size_internal (CtkWindow    *window,
                                                      gboolean      change_width,
                                                      gint          width,
                                                      gboolean      change_height,
                                                      gint          height,
						      gboolean      is_geometry);

static void     update_themed_icon                    (CtkWindow    *window);
static GList   *icon_list_from_theme                  (CtkWindow    *window,
						       const gchar  *name);
static void     ctk_window_realize_icon               (CtkWindow    *window);
static void     ctk_window_unrealize_icon             (CtkWindow    *window);
static void     update_window_buttons                 (CtkWindow    *window);
static void     get_shadow_width                      (CtkWindow    *window,
                                                       CtkBorder    *shadow_width);

static CtkKeyHash *ctk_window_get_key_hash        (CtkWindow   *window);
static void        ctk_window_free_key_hash       (CtkWindow   *window);
static void	   ctk_window_on_composited_changed (CdkScreen *screen,
						     CtkWindow *window);
#ifdef CDK_WINDOWING_X11
static void        ctk_window_on_theme_variant_changed (CtkSettings *settings,
                                                        GParamSpec  *pspec,
                                                        CtkWindow   *window);
#endif
static void        ctk_window_set_theme_variant         (CtkWindow  *window);

static void        ctk_window_do_popup         (CtkWindow      *window,
                                                CdkEventButton *event);

static void ctk_window_get_preferred_width (CtkWidget *widget,
                                            gint      *minimum_size,
                                            gint      *natural_size);
static void ctk_window_get_preferred_width_for_height (CtkWidget *widget,
                                                       gint       height,
                                                       gint      *minimum_size,
                                                       gint      *natural_size);

static void ctk_window_get_preferred_height (CtkWidget *widget,
                                             gint      *minimum_size,
                                             gint      *natural_size);
static void ctk_window_get_preferred_height_for_width (CtkWidget *widget,
                                                       gint       width,
                                                       gint      *minimum_size,
                                                       gint      *natural_size);
static void ctk_window_style_updated (CtkWidget     *widget);
static void ctk_window_state_flags_changed (CtkWidget     *widget,
                                            CtkStateFlags  previous_state);

static void ctk_window_get_remembered_size (CtkWindow *window,
                                            int       *width,
                                            int       *height);

static GSList      *toplevel_list = NULL;
static guint        window_signals[LAST_SIGNAL] = { 0 };
static GList       *default_icon_list = NULL;
static gchar       *default_icon_name = NULL;
static guint        default_icon_serial = 0;
static gboolean     disable_startup_notification = FALSE;

static GQuark       quark_ctk_embedded = 0;
static GQuark       quark_ctk_window_key_hash = 0;
static GQuark       quark_ctk_window_icon_info = 0;
static GQuark       quark_ctk_buildable_accels = 0;

static CtkBuildableIface *parent_buildable_iface;

static void ctk_window_set_property (GObject         *object,
				     guint            prop_id,
				     const GValue    *value,
				     GParamSpec      *pspec);
static void ctk_window_get_property (GObject         *object,
				     guint            prop_id,
				     GValue          *value,
				     GParamSpec      *pspec);

/* CtkBuildable */
static void ctk_window_buildable_interface_init  (CtkBuildableIface *iface);
static void ctk_window_buildable_add_child (CtkBuildable *buildable,
                                            CtkBuilder   *builder,
                                            GObject      *child,
                                            const gchar  *type);
static void ctk_window_buildable_set_buildable_property (CtkBuildable        *buildable,
							 CtkBuilder          *builder,
							 const gchar         *name,
							 const GValue        *value);
static void ctk_window_buildable_parser_finished (CtkBuildable     *buildable,
						  CtkBuilder       *builder);
static gboolean ctk_window_buildable_custom_tag_start (CtkBuildable  *buildable,
						       CtkBuilder    *builder,
						       GObject       *child,
						       const gchar   *tagname,
						       GMarkupParser *parser,
						       gpointer      *data);
static void ctk_window_buildable_custom_finished (CtkBuildable  *buildable,
						      CtkBuilder    *builder,
						      GObject       *child,
						      const gchar   *tagname,
						      gpointer       user_data);

static void ensure_state_flag_backdrop (CtkWidget *widget);
static void unset_titlebar (CtkWindow *window);
static void on_titlebar_title_notify (CtkHeaderBar *titlebar,
                                      GParamSpec   *pspec,
                                      CtkWindow    *self);
static CtkWindowRegion get_active_region_type (CtkWindow   *window,
                                               CdkEventAny *event,
                                               gint         x,
                                               gint         y);

static void ctk_window_update_debugging (void);

G_DEFINE_TYPE_WITH_CODE (CtkWindow, ctk_window, CTK_TYPE_BIN,
                         G_ADD_PRIVATE (CtkWindow)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
						ctk_window_buildable_interface_init))

static void
add_tab_bindings (CtkBindingSet    *binding_set,
		  CdkModifierType   modifiers,
		  CtkDirectionType  direction)
{
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Tab, modifiers,
                                "move-focus", 1,
                                CTK_TYPE_DIRECTION_TYPE, direction);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Tab, modifiers,
                                "move-focus", 1,
                                CTK_TYPE_DIRECTION_TYPE, direction);
}

static void
add_arrow_bindings (CtkBindingSet    *binding_set,
		    guint             keysym,
		    CtkDirectionType  direction)
{
  guint keypad_keysym = keysym - CDK_KEY_Left + CDK_KEY_KP_Left;
  
  ctk_binding_entry_add_signal (binding_set, keysym, 0,
                                "move-focus", 1,
                                CTK_TYPE_DIRECTION_TYPE, direction);
  ctk_binding_entry_add_signal (binding_set, keysym, CDK_CONTROL_MASK,
                                "move-focus", 1,
                                CTK_TYPE_DIRECTION_TYPE, direction);
  ctk_binding_entry_add_signal (binding_set, keypad_keysym, 0,
                                "move-focus", 1,
                                CTK_TYPE_DIRECTION_TYPE, direction);
  ctk_binding_entry_add_signal (binding_set, keypad_keysym, CDK_CONTROL_MASK,
                                "move-focus", 1,
                                CTK_TYPE_DIRECTION_TYPE, direction);
}

static guint32
extract_time_from_startup_id (const gchar* startup_id)
{
  gchar *timestr = g_strrstr (startup_id, "_TIME");
  guint32 retval = CDK_CURRENT_TIME;

  if (timestr)
    {
      gchar *end;
      guint32 timestamp; 
    
      /* Skip past the "_TIME" part */
      timestr += 5;

      end = NULL;
      errno = 0;
      timestamp = g_ascii_strtoull (timestr, &end, 0);
      if (errno == 0 && end != timestr)
        retval = timestamp;
    }

  return retval;
}

static gboolean
startup_id_is_fake (const gchar* startup_id)
{
  return strncmp (startup_id, "_TIME", 5) == 0;
}

static void
ctk_window_class_init (CtkWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;
  CtkBindingSet *binding_set;

  widget_class = (CtkWidgetClass*) klass;
  container_class = (CtkContainerClass*) klass;
  
  quark_ctk_embedded = g_quark_from_static_string ("ctk-embedded");
  quark_ctk_window_key_hash = g_quark_from_static_string ("ctk-window-key-hash");
  quark_ctk_window_icon_info = g_quark_from_static_string ("ctk-window-icon-info");
  quark_ctk_buildable_accels = g_quark_from_static_string ("ctk-window-buildable-accels");

  gobject_class->constructed = ctk_window_constructed;
  gobject_class->dispose = ctk_window_dispose;
  gobject_class->finalize = ctk_window_finalize;

  gobject_class->set_property = ctk_window_set_property;
  gobject_class->get_property = ctk_window_get_property;

  widget_class->destroy = ctk_window_destroy;
  widget_class->show = ctk_window_show;
  widget_class->hide = ctk_window_hide;
  widget_class->map = ctk_window_map;
  widget_class->map_event = ctk_window_map_event;
  widget_class->unmap = ctk_window_unmap;
  widget_class->realize = ctk_window_realize;
  widget_class->unrealize = ctk_window_unrealize;
  widget_class->size_allocate = ctk_window_size_allocate;
  widget_class->configure_event = ctk_window_configure_event;
  widget_class->event = ctk_window_event;
  widget_class->key_press_event = ctk_window_key_press_event;
  widget_class->key_release_event = ctk_window_key_release_event;
  widget_class->focus_in_event = ctk_window_focus_in_event;
  widget_class->focus_out_event = ctk_window_focus_out_event;
  widget_class->focus = ctk_window_focus;
  widget_class->move_focus = ctk_window_move_focus;
  widget_class->draw = ctk_window_draw;
  widget_class->window_state_event = ctk_window_state_event;
  widget_class->get_preferred_width = ctk_window_get_preferred_width;
  widget_class->get_preferred_width_for_height = ctk_window_get_preferred_width_for_height;
  widget_class->get_preferred_height = ctk_window_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_window_get_preferred_height_for_width;
  widget_class->state_flags_changed = ctk_window_state_flags_changed;
  widget_class->style_updated = ctk_window_style_updated;

  container_class->remove = ctk_window_remove;
  container_class->check_resize = ctk_window_check_resize;
  container_class->forall = ctk_window_forall;

  klass->set_focus = ctk_window_real_set_focus;

  klass->activate_default = ctk_window_real_activate_default;
  klass->activate_focus = ctk_window_real_activate_focus;
  klass->keys_changed = ctk_window_keys_changed;
  klass->enable_debugging = ctk_window_enable_debugging;

  window_props[PROP_TYPE] =
      g_param_spec_enum ("type",
                         P_("Window Type"),
                         P_("The type of the window"),
                         CTK_TYPE_WINDOW_TYPE,
                         CTK_WINDOW_TOPLEVEL,
                         CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  window_props[PROP_TITLE] =
      g_param_spec_string ("title",
                           P_("Window Title"),
                           P_("The title of the window"),
                           NULL,
                           CTK_PARAM_READWRITE);

  window_props[PROP_ROLE] =
      g_param_spec_string ("role",
                           P_("Window Role"),
                           P_("Unique identifier for the window to be used when restoring a session"),
                           NULL,
                           CTK_PARAM_READWRITE);

  /**
   * CtkWindow:startup-id:
   *
   * The :startup-id is a write-only property for setting window's
   * startup notification identifier. See ctk_window_set_startup_id()
   * for more details.
   *
   * Since: 2.12
   */
  window_props[PROP_STARTUP_ID] =
      g_param_spec_string ("startup-id",
                           P_("Startup ID"),
                           P_("Unique startup identifier for the window used by startup-notification"),
                           NULL,
                           CTK_PARAM_WRITABLE);

  window_props[PROP_RESIZABLE] =
      g_param_spec_boolean ("resizable",
                            P_("Resizable"),
                            P_("If TRUE, users can resize the window"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  window_props[PROP_MODAL] =
      g_param_spec_boolean ("modal",
                            P_("Modal"),
                            P_("If TRUE, the window is modal (other windows are not usable while this one is up)"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  window_props[PROP_WIN_POS] =
      g_param_spec_enum ("window-position",
                         P_("Window Position"),
                         P_("The initial position of the window"),
                         CTK_TYPE_WINDOW_POSITION,
                         CTK_WIN_POS_NONE,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  window_props[PROP_DEFAULT_WIDTH] =
      g_param_spec_int ("default-width",
                        P_("Default Width"),
                        P_("The default width of the window, used when initially showing the window"),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  window_props[PROP_DEFAULT_HEIGHT] =
      g_param_spec_int ("default-height",
                        P_("Default Height"),
                        P_("The default height of the window, used when initially showing the window"),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  window_props[PROP_DESTROY_WITH_PARENT] =
      g_param_spec_boolean ("destroy-with-parent",
                            P_("Destroy with Parent"),
                            P_("If this window should be destroyed when the parent is destroyed"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWindow:hide-titlebar-when-maximized:
   *
   * Whether the titlebar should be hidden during maximization.
   *
   * Since: 3.4
   */
  window_props[PROP_HIDE_TITLEBAR_WHEN_MAXIMIZED] =
      g_param_spec_boolean ("hide-titlebar-when-maximized",
                            P_("Hide the titlebar during maximization"),
                            P_("If this window's titlebar should be hidden when the window is maximized"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  window_props[PROP_ICON] =
      g_param_spec_object ("icon",
                           P_("Icon"),
                           P_("Icon for this window"),
                           GDK_TYPE_PIXBUF,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWindow:mnemonics-visible:
   *
   * Whether mnemonics are currently visible in this window.
   *
   * This property is maintained by CTK+ based on user input,
   * and should not be set by applications.
   *
   * Since: 2.20
   */
  window_props[PROP_MNEMONICS_VISIBLE] =
      g_param_spec_boolean ("mnemonics-visible",
                            P_("Mnemonics Visible"),
                            P_("Whether mnemonics are currently visible in this window"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWindow:focus-visible:
   *
   * Whether 'focus rectangles' are currently visible in this window.
   *
   * This property is maintained by CTK+ based on user input
   * and should not be set by applications.
   *
   * Since: 2.20
   */
  window_props[PROP_FOCUS_VISIBLE] =
      g_param_spec_boolean ("focus-visible",
                            P_("Focus Visible"),
                            P_("Whether focus rectangles are currently visible in this window"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWindow:icon-name:
   *
   * The :icon-name property specifies the name of the themed icon to
   * use as the window icon. See #CtkIconTheme for more details.
   *
   * Since: 2.6
   */
  window_props[PROP_ICON_NAME] =
      g_param_spec_string ("icon-name",
                           P_("Icon Name"),
                           P_("Name of the themed icon for this window"),
                           NULL,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  window_props[PROP_SCREEN] =
      g_param_spec_object ("screen",
                           P_("Screen"),
                           P_("The screen where this window will be displayed"),
                           CDK_TYPE_SCREEN,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  window_props[PROP_IS_ACTIVE] =
      g_param_spec_boolean ("is-active",
                            P_("Is Active"),
                            P_("Whether the toplevel is the current active window"),
                            FALSE,
                            CTK_PARAM_READABLE);

  window_props[PROP_HAS_TOPLEVEL_FOCUS] =
      g_param_spec_boolean ("has-toplevel-focus",
                            P_("Focus in Toplevel"),
                            P_("Whether the input focus is within this CtkWindow"),
                            FALSE,
                            CTK_PARAM_READABLE);

  window_props[PROP_TYPE_HINT] =
      g_param_spec_enum ("type-hint",
                         P_("Type hint"),
                         P_("Hint to help the desktop environment understand what kind of window this is and how to treat it."),
                         CDK_TYPE_WINDOW_TYPE_HINT,
                         CDK_WINDOW_TYPE_HINT_NORMAL,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  window_props[PROP_SKIP_TASKBAR_HINT] =
      g_param_spec_boolean ("skip-taskbar-hint",
                            P_("Skip taskbar"),
                            P_("TRUE if the window should not be in the task bar."),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  window_props[PROP_SKIP_PAGER_HINT] =
      g_param_spec_boolean ("skip-pager-hint",
                            P_("Skip pager"),
                            P_("TRUE if the window should not be in the pager."),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  window_props[PROP_URGENCY_HINT] =
      g_param_spec_boolean ("urgency-hint",
                            P_("Urgent"),
                            P_("TRUE if the window should be brought to the user's attention."),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWindow:accept-focus:
   *
   * Whether the window should receive the input focus.
   *
   * Since: 2.4
   */
  window_props[PROP_ACCEPT_FOCUS] =
      g_param_spec_boolean ("accept-focus",
                            P_("Accept focus"),
                            P_("TRUE if the window should receive the input focus."),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWindow:focus-on-map:
   *
   * Whether the window should receive the input focus when mapped.
   *
   * Since: 2.6
   */
  window_props[PROP_FOCUS_ON_MAP] =
      g_param_spec_boolean ("focus-on-map",
                            P_("Focus on map"),
                            P_("TRUE if the window should receive the input focus when mapped."),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWindow:decorated:
   *
   * Whether the window should be decorated by the window manager.
   *
   * Since: 2.4
   */
  window_props[PROP_DECORATED] =
      g_param_spec_boolean ("decorated",
                            P_("Decorated"),
                            P_("Whether the window should be decorated by the window manager"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWindow:deletable:
   *
   * Whether the window frame should have a close button.
   *
   * Since: 2.10
   */
  window_props[PROP_DELETABLE] =
      g_param_spec_boolean ("deletable",
                            P_("Deletable"),
                            P_("Whether the window frame should have a close button"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWindow:has-resize-grip:
   *
   * Whether the window has a corner resize grip.
   *
   * Note that the resize grip is only shown if the window is
   * actually resizable and not maximized. Use
   * #CtkWindow:resize-grip-visible to find out if the resize
   * grip is currently shown.
   *
   * Deprecated: 3.14: Resize grips have been removed.
   *
   * Since: 3.0
   */
  window_props[PROP_HAS_RESIZE_GRIP] =
      g_param_spec_boolean ("has-resize-grip",
                            P_("Resize grip"),
                            P_("Specifies whether the window should have a resize grip"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  /**
   * CtkWindow:resize-grip-visible:
   *
   * Whether a corner resize grip is currently shown.
   *
   * Deprecated: 3.14: Resize grips have been removed.
   *
   * Since: 3.0
   */
  window_props[PROP_RESIZE_GRIP_VISIBLE] =
      g_param_spec_boolean ("resize-grip-visible",
                            P_("Resize grip is visible"),
                            P_("Specifies whether the window's resize grip is visible."),
                            FALSE,
                            CTK_PARAM_READABLE|G_PARAM_DEPRECATED);

  /**
   * CtkWindow:gravity:
   *
   * The window gravity of the window. See ctk_window_move() and #CdkGravity for
   * more details about window gravity.
   *
   * Since: 2.4
   */
  window_props[PROP_GRAVITY] =
      g_param_spec_enum ("gravity",
                         P_("Gravity"),
                         P_("The window gravity of the window"),
                         CDK_TYPE_GRAVITY,
                         CDK_GRAVITY_NORTH_WEST,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWindow:transient-for:
   *
   * The transient parent of the window. See ctk_window_set_transient_for() for
   * more details about transient windows.
   *
   * Since: 2.10
   */
  window_props[PROP_TRANSIENT_FOR] =
      g_param_spec_object ("transient-for",
                           P_("Transient for Window"),
                           P_("The transient parent of the dialog"),
                           CTK_TYPE_WINDOW,
                           CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkWindow:attached-to:
   *
   * The widget to which this window is attached.
   * See ctk_window_set_attached_to().
   *
   * Examples of places where specifying this relation is useful are
   * for instance a #CtkMenu created by a #CtkComboBox, a completion
   * popup window created by #CtkEntry or a typeahead search entry
   * created by #CtkTreeView.
   *
   * Since: 3.4
   */
  window_props[PROP_ATTACHED_TO] =
      g_param_spec_object ("attached-to",
                           P_("Attached to Widget"),
                           P_("The widget where the window is attached"),
                           CTK_TYPE_WIDGET,
                           CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  window_props[PROP_IS_MAXIMIZED] =
      g_param_spec_boolean ("is-maximized",
                            P_("Is maximized"),
                            P_("Whether the window is maximized"),
                            FALSE,
                            CTK_PARAM_READABLE);

  /**
   * CtkWindow:application:
   *
   * The #CtkApplication associated with the window.
   *
   * The application will be kept alive for at least as long as it
   * has any windows associated with it (see g_application_hold()
   * for a way to keep it alive without windows).
   *
   * Normally, the connection between the application and the window
   * will remain until the window is destroyed, but you can explicitly
   * remove it by setting the :application property to %NULL.
   *
   * Since: 3.0
   */
  window_props[PROP_APPLICATION] =
      g_param_spec_object ("application",
                           P_("CtkApplication"),
                           P_("The CtkApplication for the window"),
                           CTK_TYPE_APPLICATION,
                           CTK_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, LAST_ARG, window_props);

  /* Style properties.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_string ("decoration-button-layout",
                                                                P_("Decorated button layout"),
                                                                P_("Decorated button layout"),
                                                                "menu:close",
                                                                CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("decoration-resize-handle",
                                                             P_("Decoration resize handle size"),
                                                             P_("Decoration resize handle size"),
                                                             0, G_MAXINT,
                                                             20, CTK_PARAM_READWRITE));

  /**
   * CtkWindow::set-focus:
   * @window: the window which received the signal
   * @widget: (nullable): the newly focused widget (or %NULL for no focus)
   *
   * This signal is emitted whenever the currently focused widget in
   * this window changes.
   *
   * Since: 2.24
   */
  window_signals[SET_FOCUS] =
    g_signal_new (I_("set-focus"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkWindowClass, set_focus),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CTK_TYPE_WIDGET);

  /**
   * CtkWindow::activate-focus:
   * @window: the window which received the signal
   *
   * The ::activate-focus signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user activates the currently
   * focused widget of @window.
   */
  window_signals[ACTIVATE_FOCUS] =
    g_signal_new (I_("activate-focus"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkWindowClass, activate_focus),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  /**
   * CtkWindow::activate-default:
   * @window: the window which received the signal
   *
   * The ::activate-default signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user activates the default widget
   * of @window.
   */
  window_signals[ACTIVATE_DEFAULT] =
    g_signal_new (I_("activate-default"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkWindowClass, activate_default),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  /**
   * CtkWindow::keys-changed:
   * @window: the window which received the signal
   *
   * The ::keys-changed signal gets emitted when the set of accelerators
   * or mnemonics that are associated with @window changes.
   */
  window_signals[KEYS_CHANGED] =
    g_signal_new (I_("keys-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkWindowClass, keys_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  /**
   * CtkWindow::enable-debugging:
   * @window: the window on which the signal is emitted
   * @toggle: toggle the debugger
   *
   * The ::enable-debugging signal is a [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user enables or disables interactive
   * debugging. When @toggle is %TRUE, interactive debugging is toggled
   * on or off, when it is %FALSE, the debugger will be pointed at the
   * widget under the pointer.
   *
   * The default bindings for this signal are Ctrl-Shift-I
   * and Ctrl-Shift-D.
   *
   * Return: %TRUE if the key binding was handled
   */
  window_signals[ENABLE_DEBUGGING] =
    g_signal_new (I_("enable-debugging"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkWindowClass, enable_debugging),
                  NULL, NULL,
                  _ctk_marshal_BOOLEAN__BOOLEAN,
                  G_TYPE_BOOLEAN,
                  1, G_TYPE_BOOLEAN);

  /*
   * Key bindings
   */

  binding_set = ctk_binding_set_by_class (klass);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_space, 0,
                                "activate-focus", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Space, 0,
                                "activate-focus", 0);
  
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Return, 0,
                                "activate-default", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_ISO_Enter, 0,
                                "activate-default", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Enter, 0,
                                "activate-default", 0);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_I, CDK_CONTROL_MASK|CDK_SHIFT_MASK,
                                "enable-debugging", 1,
                                G_TYPE_BOOLEAN, FALSE);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_D, CDK_CONTROL_MASK|CDK_SHIFT_MASK,
                                "enable-debugging", 1,
                                G_TYPE_BOOLEAN, TRUE);

  add_arrow_bindings (binding_set, CDK_KEY_Up, CTK_DIR_UP);
  add_arrow_bindings (binding_set, CDK_KEY_Down, CTK_DIR_DOWN);
  add_arrow_bindings (binding_set, CDK_KEY_Left, CTK_DIR_LEFT);
  add_arrow_bindings (binding_set, CDK_KEY_Right, CTK_DIR_RIGHT);

  add_tab_bindings (binding_set, 0, CTK_DIR_TAB_FORWARD);
  add_tab_bindings (binding_set, CDK_CONTROL_MASK, CTK_DIR_TAB_FORWARD);
  add_tab_bindings (binding_set, CDK_SHIFT_MASK, CTK_DIR_TAB_BACKWARD);
  add_tab_bindings (binding_set, CDK_CONTROL_MASK | CDK_SHIFT_MASK, CTK_DIR_TAB_BACKWARD);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_WINDOW_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "window");
}

/**
 * ctk_window_is_maximized:
 * @window: a #CtkWindow
 *
 * Retrieves the current maximized state of @window.
 *
 * Note that since maximization is ultimately handled by the window
 * manager and happens asynchronously to an application request, you
 * shouldn’t assume the return value of this function changing
 * immediately (or at all), as an effect of calling
 * ctk_window_maximize() or ctk_window_unmaximize().
 *
 * Returns: whether the window has a maximized state.
 *
 * Since: 3.12
 */
gboolean
ctk_window_is_maximized (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;

  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return priv->maximized;
}

void
_ctk_window_toggle_maximized (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;

  if (priv->maximized)
    ctk_window_unmaximize (window);
  else
    ctk_window_maximize (window);
}

static gboolean
send_delete_event (gpointer data)
{
  CtkWidget *window = data;
  CtkWindowPrivate *priv = CTK_WINDOW (window)->priv;
  CdkWindow *cdk_window;

  priv->delete_event_handler = 0;

  cdk_window = _ctk_widget_get_window (window);
  if (cdk_window)
    {
      CdkEvent *event;

      event = cdk_event_new (CDK_DELETE);
      event->any.window = g_object_ref (cdk_window);
      event->any.send_event = TRUE;

      ctk_main_do_event (event);

      cdk_event_free (event);
    }

  return G_SOURCE_REMOVE;
}

/**
 * ctk_window_close:
 * @window: a #CtkWindow
 *
 * Requests that the window is closed, similar to what happens
 * when a window manager close button is clicked.
 *
 * This function can be used with close buttons in custom
 * titlebars.
 *
 * Since: 3.10
 */
void
ctk_window_close (CtkWindow *window)
{
  if (!_ctk_widget_get_realized (CTK_WIDGET (window)))
    return;

  window->priv->delete_event_handler = cdk_threads_add_idle_full (G_PRIORITY_DEFAULT, send_delete_event, window, NULL);
  g_source_set_name_by_id (window->priv->delete_event_handler, "[ctk+] send_delete_event");
}

static void
popover_destroy (CtkWindowPopover *popover)
{
  if (popover->unmap_id)
    {
      g_signal_handler_disconnect (popover->widget, popover->unmap_id);
      popover->unmap_id = 0;
    }

  if (popover->widget && _ctk_widget_get_parent (popover->widget))
    ctk_widget_unparent (popover->widget);

  if (popover->window)
    cdk_window_destroy (popover->window);

  g_free (popover);
}

static gboolean
ctk_window_titlebar_action (CtkWindow      *window,
                            const CdkEvent *event,
                            guint           button,
                            gint            n_press)
{
  CtkSettings *settings;
  gchar *action = NULL;
  gboolean retval = TRUE;

  settings = ctk_widget_get_settings (CTK_WIDGET (window));
  switch (button)
    {
    case CDK_BUTTON_PRIMARY:
      if (n_press == 2)
        g_object_get (settings, "ctk-titlebar-double-click", &action, NULL);
      break;
    case CDK_BUTTON_MIDDLE:
      g_object_get (settings, "ctk-titlebar-middle-click", &action, NULL);
      break;
    case CDK_BUTTON_SECONDARY:
      g_object_get (settings, "ctk-titlebar-right-click", &action, NULL);
      break;
    }

  if (action == NULL)
    retval = FALSE;
  else if (g_str_equal (action, "none"))
    retval = FALSE;
    /* treat all maximization variants the same */
  else if (g_str_has_prefix (action, "toggle-maximize"))
    {
      /*
       * ctk header bar won't show the maximize button if the following
       * properties are not met, apply the same to title bar actions for
       * consistency.
       */
      if (ctk_window_get_resizable (window) &&
          ctk_window_get_type_hint (window) == CDK_WINDOW_TYPE_HINT_NORMAL)
            _ctk_window_toggle_maximized (window);
    }
  else if (g_str_equal (action, "lower"))
    cdk_window_lower (_ctk_widget_get_window (CTK_WIDGET (window)));
  else if (g_str_equal (action, "minimize"))
    cdk_window_iconify (_ctk_widget_get_window (CTK_WIDGET (window)));
  else if (g_str_equal (action, "menu"))
    ctk_window_do_popup (window, (CdkEventButton*) event);
  else
    {
      g_warning ("Unsupported titlebar action %s", action);
      retval = FALSE;
    }

  g_free (action);

  return retval;
}

static void
multipress_gesture_pressed_cb (CtkGestureMultiPress *gesture,
                               gint                  n_press,
                               gdouble               x,
                               gdouble               y,
                               CtkWindow            *window)
{
  CtkWidget *event_widget, *widget;
  CdkEventSequence *sequence;
  CtkWindowRegion region;
  CtkWindowPrivate *priv;
  const CdkEvent *event;
  guint button;
  gboolean window_drag = FALSE;

  widget = CTK_WIDGET (window);
  priv = ctk_window_get_instance_private (window);
  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  button = ctk_gesture_single_get_current_button (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);

  if (!event)
    return;

  if (n_press > 1)
    ctk_gesture_set_state (priv->drag_gesture, CTK_EVENT_SEQUENCE_DENIED);

  region = get_active_region_type (window, (CdkEventAny*) event, x, y);

  if (cdk_display_device_is_grabbed (ctk_widget_get_display (widget),
                                     ctk_gesture_get_device (CTK_GESTURE (gesture))))
    {
      ctk_gesture_set_state (priv->drag_gesture, CTK_EVENT_SEQUENCE_DENIED);
      return;
    }

  if (button == CDK_BUTTON_SECONDARY && region == CTK_WINDOW_REGION_TITLE)
    {
      if (ctk_window_titlebar_action (window, event, button, n_press))
        ctk_gesture_set_sequence_state (CTK_GESTURE (gesture),
                                        sequence, CTK_EVENT_SEQUENCE_CLAIMED);

      ctk_event_controller_reset (CTK_EVENT_CONTROLLER (gesture));
      ctk_event_controller_reset (CTK_EVENT_CONTROLLER (priv->drag_gesture));
      return;
    }
  else if (button == CDK_BUTTON_MIDDLE && region == CTK_WINDOW_REGION_TITLE)
    {
      if (ctk_window_titlebar_action (window, event, button, n_press))
        ctk_gesture_set_sequence_state (CTK_GESTURE (gesture),
                                        sequence, CTK_EVENT_SEQUENCE_CLAIMED);
      return;
    }
  else if (button != CDK_BUTTON_PRIMARY)
    return;

  event_widget = ctk_get_event_widget ((CdkEvent *) event);

  if (region == CTK_WINDOW_REGION_TITLE)
    cdk_window_raise (_ctk_widget_get_window (widget));

  switch (region)
    {
    case CTK_WINDOW_REGION_CONTENT:
      if (event_widget != widget)
        ctk_widget_style_get (event_widget, "window-dragging", &window_drag, NULL);

      if (!window_drag)
        {
          ctk_gesture_set_sequence_state (CTK_GESTURE (gesture),
                                          sequence, CTK_EVENT_SEQUENCE_DENIED);
          return;
        }
      /* fall through */

    case CTK_WINDOW_REGION_TITLE:
      if (n_press == 2)
        ctk_window_titlebar_action (window, event, button, n_press);

      if (ctk_widget_has_grab (widget))
        ctk_gesture_set_sequence_state (CTK_GESTURE (gesture),
                                        sequence, CTK_EVENT_SEQUENCE_CLAIMED);
      break;
    default:
      if (!priv->maximized)
        {
          gdouble x_root, y_root;

          ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_CLAIMED);

          cdk_event_get_root_coords (event, &x_root, &y_root);
          cdk_window_begin_resize_drag_for_device (_ctk_widget_get_window (widget),
                                                   (CdkWindowEdge) region,
                                                   cdk_event_get_device ((CdkEvent *) event),
                                                   CDK_BUTTON_PRIMARY,
                                                   x_root, y_root,
                                                   cdk_event_get_time (event));

          ctk_event_controller_reset (CTK_EVENT_CONTROLLER (gesture));
          ctk_event_controller_reset (CTK_EVENT_CONTROLLER (priv->drag_gesture));
        }

      break;
    }
}

static void
drag_gesture_begin_cb (CtkGestureDrag *gesture,
                       gdouble         x,
                       gdouble         y,
                       CtkWindow      *window)
{
  CdkEventSequence *sequence;
  CtkWindowRegion region;
  CtkWidget *event_widget;
  gboolean widget_drag;
  const CdkEvent *event;

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);

  if (!event)
    return;

  region = get_active_region_type (window, (CdkEventAny*) event, x, y);

  switch (region)
    {
      case CTK_WINDOW_REGION_TITLE:
        /* Claim it */
        break;
      case CTK_WINDOW_REGION_CONTENT:
        event_widget = ctk_get_event_widget ((CdkEvent *) event);

        ctk_widget_style_get (event_widget, "window-dragging", &widget_drag, NULL);

        if (!widget_drag)
          ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_DENIED);

        break;
      default:
        ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_DENIED);
    }
}

static void
drag_gesture_update_cb (CtkGestureDrag *gesture,
                        gdouble         offset_x,
                        gdouble         offset_y,
                        CtkWindow      *window)
{
  CtkWindowPrivate *priv = window->priv;
  gint double_click_distance;
  CtkSettings *settings;

  settings = ctk_widget_get_settings (CTK_WIDGET (window));
  g_object_get (settings,
                "ctk-double-click-distance", &double_click_distance,
                NULL);

  if (ABS (offset_x) > double_click_distance ||
      ABS (offset_y) > double_click_distance)
    {
      CdkEventSequence *sequence;
      gdouble start_x, start_y;
      gint x_root, y_root;
      const CdkEvent *event;
      CtkWidget *event_widget;

      sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
      event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);
      event_widget = ctk_get_event_widget ((CdkEvent *) event);

      /* Check whether the target widget should be left alone at handling
       * the sequence, this is better done late to give room for gestures
       * there to go denied.
       *
       * Besides claiming gestures, we must bail out too if there's gestures
       * in the "none" state at this point, as those are still handling events
       * and can potentially go claimed, and we don't want to stop the target
       * widget from doing anything.
       */
      if (event_widget != CTK_WIDGET (window) &&
          !ctk_widget_has_grab (event_widget) &&
          _ctk_widget_consumes_motion (event_widget, sequence))
        {
          ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_DENIED);
          return;
        }

      ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_CLAIMED);

      ctk_gesture_drag_get_start_point (gesture, &start_x, &start_y);
      cdk_window_get_root_coords (_ctk_widget_get_window (CTK_WIDGET (window)),
                                  start_x, start_y, &x_root, &y_root);

      cdk_window_begin_move_drag_for_device (_ctk_widget_get_window (CTK_WIDGET (window)),
                                             ctk_gesture_get_device (CTK_GESTURE (gesture)),
                                             ctk_gesture_single_get_current_button (CTK_GESTURE_SINGLE (gesture)),
                                             x_root, y_root,
                                             ctk_get_current_event_time ());

      ctk_event_controller_reset (CTK_EVENT_CONTROLLER (gesture));
      ctk_event_controller_reset (CTK_EVENT_CONTROLLER (priv->multipress_gesture));
    }
}

static void
node_style_changed_cb (CtkCssNode        *node,
                       CtkCssStyleChange *change,
                       CtkWidget         *widget)
{
  if (ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_SIZE | CTK_CSS_AFFECTS_CLIP))
    ctk_widget_queue_resize (widget);
  else
    ctk_widget_queue_draw (widget);
}

static void
ctk_window_init (CtkWindow *window)
{
  CtkWindowPrivate *priv;
  CtkWidget *widget;
  CtkCssNode *widget_node;

  widget = CTK_WIDGET (window);

  window->priv = ctk_window_get_instance_private (window);
  priv = window->priv;

  ctk_widget_set_has_window (widget, TRUE);
  _ctk_widget_set_is_toplevel (widget, TRUE);
  _ctk_widget_set_anchored (widget, TRUE);

  ctk_container_set_default_resize_mode (CTK_CONTAINER (window), CTK_RESIZE_QUEUE);

  priv->title = NULL;
  priv->wmclass_name = g_strdup (g_get_prgname ());
  priv->wmclass_class = g_strdup (cdk_get_program_class ());
  priv->wm_role = NULL;
  priv->geometry_info = NULL;
  priv->type = CTK_WINDOW_TOPLEVEL;
  priv->focus_widget = NULL;
  priv->default_widget = NULL;
  priv->configure_request_count = 0;
  priv->resizable = TRUE;
  priv->configure_notify_received = FALSE;
  priv->position = CTK_WIN_POS_NONE;
  priv->need_default_size = TRUE;
  priv->need_default_position = TRUE;
  priv->modal = FALSE;
  priv->gravity = CDK_GRAVITY_NORTH_WEST;
  priv->decorated = TRUE;
  priv->mnemonic_modifier = CDK_MOD1_MASK;
  priv->screen = cdk_screen_get_default ();

  priv->accept_focus = TRUE;
  priv->focus_on_map = TRUE;
  priv->deletable = TRUE;
  priv->type_hint = CDK_WINDOW_TYPE_HINT_NORMAL;
  priv->startup_id = NULL;
  priv->initial_timestamp = CDK_CURRENT_TIME;
  priv->mnemonics_visible = TRUE;
  priv->focus_visible = TRUE;
  priv->initial_fullscreen_monitor = -1;

  g_object_ref_sink (window);
  priv->has_user_ref_count = TRUE;
  toplevel_list = g_slist_prepend (toplevel_list, window);
  ctk_window_update_debugging ();

  if (priv->screen)
    {
      g_signal_connect (priv->screen, "composited-changed",
                        G_CALLBACK (ctk_window_on_composited_changed), window);

#ifdef CDK_WINDOWING_X11
      g_signal_connect (ctk_settings_get_for_screen (priv->screen),
                        "notify::ctk-application-prefer-dark-theme",
                        G_CALLBACK (ctk_window_on_theme_variant_changed), window);
#endif
    }

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (window));
  priv->decoration_node = ctk_css_node_new ();
  ctk_css_node_set_name (priv->decoration_node, I_("decoration"));
  ctk_css_node_set_parent (priv->decoration_node, widget_node);
  ctk_css_node_set_state (priv->decoration_node, ctk_css_node_get_state (widget_node));
  g_signal_connect_object (priv->decoration_node, "style-changed", G_CALLBACK (node_style_changed_cb), window, 0);
  g_object_unref (priv->decoration_node);

  ctk_css_node_add_class (widget_node, g_quark_from_static_string (CTK_STYLE_CLASS_BACKGROUND));

  priv->scale = ctk_widget_get_scale_factor (widget);

#ifdef CDK_WINDOWING_X11
  ctk_drag_dest_set (CTK_WIDGET (window),
                     CTK_DEST_DEFAULT_MOTION | CTK_DEST_DEFAULT_DROP,
                     dnd_dest_targets, G_N_ELEMENTS (dnd_dest_targets),
                     CDK_ACTION_MOVE);
#endif
}

static void
ctk_window_constructed (GObject *object)
{
  CtkWindow *window = CTK_WINDOW (object);
  CtkWindowPrivate *priv = window->priv;
  gboolean is_plug;

  G_OBJECT_CLASS (ctk_window_parent_class)->constructed (object);

#ifdef CDK_WINDOWING_X11
  is_plug = CTK_IS_PLUG (window);
#else
  is_plug = FALSE;
#endif

  if (priv->type == CTK_WINDOW_TOPLEVEL && !is_plug)
    {
      priv->multipress_gesture = ctk_gesture_multi_press_new (CTK_WIDGET (object));
      ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (priv->multipress_gesture), 0);
      ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->multipress_gesture),
                                                  CTK_PHASE_NONE);
      g_signal_connect (priv->multipress_gesture, "pressed",
                        G_CALLBACK (multipress_gesture_pressed_cb), object);

      priv->drag_gesture = ctk_gesture_drag_new (CTK_WIDGET (object));
      ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->drag_gesture),
                                                  CTK_PHASE_CAPTURE);
      g_signal_connect (priv->drag_gesture, "drag-begin",
                        G_CALLBACK (drag_gesture_begin_cb), object);
      g_signal_connect (priv->drag_gesture, "drag-update",
                        G_CALLBACK (drag_gesture_update_cb), object);
    }
}

static void
ctk_window_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  CtkWindow  *window = CTK_WINDOW (object);
  CtkWindowPrivate *priv = window->priv;

  switch (prop_id)
    {
    case PROP_TYPE:
      priv->type = g_value_get_enum (value);
      break;
    case PROP_TITLE:
      ctk_window_set_title (window, g_value_get_string (value));
      break;
    case PROP_ROLE:
      ctk_window_set_role (window, g_value_get_string (value));
      break;
    case PROP_STARTUP_ID:
      ctk_window_set_startup_id (window, g_value_get_string (value));
      break;
    case PROP_RESIZABLE:
      ctk_window_set_resizable (window, g_value_get_boolean (value));
      break;
    case PROP_MODAL:
      ctk_window_set_modal (window, g_value_get_boolean (value));
      break;
    case PROP_WIN_POS:
      ctk_window_set_position (window, g_value_get_enum (value));
      break;
    case PROP_DEFAULT_WIDTH:
      ctk_window_set_default_size_internal (window,
                                            TRUE, g_value_get_int (value),
                                            FALSE, -1, FALSE);
      break;
    case PROP_DEFAULT_HEIGHT:
      ctk_window_set_default_size_internal (window,
                                            FALSE, -1,
                                            TRUE, g_value_get_int (value), FALSE);
      break;
    case PROP_DESTROY_WITH_PARENT:
      ctk_window_set_destroy_with_parent (window, g_value_get_boolean (value));
      break;
    case PROP_HIDE_TITLEBAR_WHEN_MAXIMIZED:
      ctk_window_set_hide_titlebar_when_maximized (window, g_value_get_boolean (value));
      break;
    case PROP_ICON:
      ctk_window_set_icon (window,
                           g_value_get_object (value));
      break;
    case PROP_ICON_NAME:
      ctk_window_set_icon_name (window, g_value_get_string (value));
      break;
    case PROP_SCREEN:
      ctk_window_set_screen (window, g_value_get_object (value));
      break;
    case PROP_TYPE_HINT:
      ctk_window_set_type_hint (window,
                                g_value_get_enum (value));
      break;
    case PROP_SKIP_TASKBAR_HINT:
      ctk_window_set_skip_taskbar_hint (window,
                                        g_value_get_boolean (value));
      break;
    case PROP_SKIP_PAGER_HINT:
      ctk_window_set_skip_pager_hint (window,
                                      g_value_get_boolean (value));
      break;
    case PROP_URGENCY_HINT:
      ctk_window_set_urgency_hint (window,
				   g_value_get_boolean (value));
      break;
    case PROP_ACCEPT_FOCUS:
      ctk_window_set_accept_focus (window,
				   g_value_get_boolean (value));
      break;
    case PROP_FOCUS_ON_MAP:
      ctk_window_set_focus_on_map (window,
				   g_value_get_boolean (value));
      break;
    case PROP_DECORATED:
      ctk_window_set_decorated (window, g_value_get_boolean (value));
      break;
    case PROP_DELETABLE:
      ctk_window_set_deletable (window, g_value_get_boolean (value));
      break;
    case PROP_GRAVITY:
      ctk_window_set_gravity (window, g_value_get_enum (value));
      break;
    case PROP_TRANSIENT_FOR:
      ctk_window_set_transient_for (window, g_value_get_object (value));
      break;
    case PROP_ATTACHED_TO:
      ctk_window_set_attached_to (window, g_value_get_object (value));
      break;
    case PROP_HAS_RESIZE_GRIP:
      /* Do nothing. */
      break;
    case PROP_APPLICATION:
      ctk_window_set_application (window, g_value_get_object (value));
      break;
    case PROP_MNEMONICS_VISIBLE:
      ctk_window_set_mnemonics_visible (window, g_value_get_boolean (value));
      break;
    case PROP_FOCUS_VISIBLE:
      ctk_window_set_focus_visible (window, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_window_get_property (GObject      *object,
			 guint         prop_id,
			 GValue       *value,
			 GParamSpec   *pspec)
{
  CtkWindow  *window = CTK_WINDOW (object);
  CtkWindowPrivate *priv = window->priv;

  switch (prop_id)
    {
      CtkWindowGeometryInfo *info;
    case PROP_TYPE:
      g_value_set_enum (value, priv->type);
      break;
    case PROP_ROLE:
      g_value_set_string (value, priv->wm_role);
      break;
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_RESIZABLE:
      g_value_set_boolean (value, priv->resizable);
      break;
    case PROP_MODAL:
      g_value_set_boolean (value, priv->modal);
      break;
    case PROP_WIN_POS:
      g_value_set_enum (value, priv->position);
      break;
    case PROP_DEFAULT_WIDTH:
      info = ctk_window_get_geometry_info (window, FALSE);
      if (!info)
	g_value_set_int (value, -1);
      else
	g_value_set_int (value, info->default_width);
      break;
    case PROP_DEFAULT_HEIGHT:
      info = ctk_window_get_geometry_info (window, FALSE);
      if (!info)
	g_value_set_int (value, -1);
      else
	g_value_set_int (value, info->default_height);
      break;
    case PROP_DESTROY_WITH_PARENT:
      g_value_set_boolean (value, priv->destroy_with_parent);
      break;
    case PROP_HIDE_TITLEBAR_WHEN_MAXIMIZED:
      g_value_set_boolean (value, priv->hide_titlebar_when_maximized);
      break;
    case PROP_ICON:
      g_value_set_object (value, ctk_window_get_icon (window));
      break;
    case PROP_ICON_NAME:
      g_value_set_string (value, ctk_window_get_icon_name (window));
      break;
    case PROP_SCREEN:
      g_value_set_object (value, priv->screen);
      break;
    case PROP_IS_ACTIVE:
      g_value_set_boolean (value, priv->is_active);
      break;
    case PROP_HAS_TOPLEVEL_FOCUS:
      g_value_set_boolean (value, priv->has_toplevel_focus);
      break;
    case PROP_TYPE_HINT:
      g_value_set_enum (value, priv->type_hint);
      break;
    case PROP_SKIP_TASKBAR_HINT:
      g_value_set_boolean (value,
                           ctk_window_get_skip_taskbar_hint (window));
      break;
    case PROP_SKIP_PAGER_HINT:
      g_value_set_boolean (value,
                           ctk_window_get_skip_pager_hint (window));
      break;
    case PROP_URGENCY_HINT:
      g_value_set_boolean (value,
                           ctk_window_get_urgency_hint (window));
      break;
    case PROP_ACCEPT_FOCUS:
      g_value_set_boolean (value,
                           ctk_window_get_accept_focus (window));
      break;
    case PROP_FOCUS_ON_MAP:
      g_value_set_boolean (value,
                           ctk_window_get_focus_on_map (window));
      break;
    case PROP_DECORATED:
      g_value_set_boolean (value, ctk_window_get_decorated (window));
      break;
    case PROP_DELETABLE:
      g_value_set_boolean (value, ctk_window_get_deletable (window));
      break;
    case PROP_GRAVITY:
      g_value_set_enum (value, ctk_window_get_gravity (window));
      break;
    case PROP_TRANSIENT_FOR:
      g_value_set_object (value, ctk_window_get_transient_for (window));
      break;
    case PROP_ATTACHED_TO:
      g_value_set_object (value, ctk_window_get_attached_to (window));
      break;
    case PROP_HAS_RESIZE_GRIP:
      g_value_set_boolean (value, FALSE);
      break;
    case PROP_RESIZE_GRIP_VISIBLE:
      g_value_set_boolean (value, FALSE);
      break;
    case PROP_APPLICATION:
      g_value_set_object (value, ctk_window_get_application (window));
      break;
    case PROP_MNEMONICS_VISIBLE:
      g_value_set_boolean (value, priv->mnemonics_visible);
      break;
    case PROP_FOCUS_VISIBLE:
      g_value_set_boolean (value, priv->focus_visible);
      break;
    case PROP_IS_MAXIMIZED:
      g_value_set_boolean (value, ctk_window_is_maximized (window));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_window_buildable_interface_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->set_buildable_property = ctk_window_buildable_set_buildable_property;
  iface->parser_finished = ctk_window_buildable_parser_finished;
  iface->custom_tag_start = ctk_window_buildable_custom_tag_start;
  iface->custom_finished = ctk_window_buildable_custom_finished;
  iface->add_child = ctk_window_buildable_add_child;
}

static void
ctk_window_buildable_add_child (CtkBuildable *buildable,
                                CtkBuilder   *builder,
                                GObject      *child,
                                const gchar  *type)
{
  if (type && strcmp (type, "titlebar") == 0)
    ctk_window_set_titlebar (CTK_WINDOW (buildable), CTK_WIDGET (child));
  else if (!type)
    ctk_container_add (CTK_CONTAINER (buildable), CTK_WIDGET (child));
  else
    CTK_BUILDER_WARN_INVALID_CHILD_TYPE (buildable, type);
}

static void
ctk_window_buildable_set_buildable_property (CtkBuildable *buildable,
                                             CtkBuilder   *builder,
                                             const gchar  *name,
                                             const GValue *value)
{
  CtkWindow *window = CTK_WINDOW (buildable);
  CtkWindowPrivate *priv = window->priv;

  if (strcmp (name, "visible") == 0 && g_value_get_boolean (value))
    priv->builder_visible = TRUE;
  else
    parent_buildable_iface->set_buildable_property (buildable, builder, name, value);
}

typedef struct {
  gchar *name;
  gint line;
  gint col;
} ItemData;

static void
item_data_free (gpointer data)
{
  ItemData *item_data = data;

  g_free (item_data->name);
  g_free (item_data);
}

static void
item_list_free (gpointer data)
{
  GSList *list = data;

  g_slist_free_full (list, item_data_free);
}

static void
ctk_window_buildable_parser_finished (CtkBuildable *buildable,
				      CtkBuilder   *builder)
{
  CtkWindow *window = CTK_WINDOW (buildable);
  CtkWindowPrivate *priv = window->priv;
  GObject *object;
  GSList *accels, *l;

  if (priv->builder_visible)
    ctk_widget_show (CTK_WIDGET (buildable));

  accels = g_object_get_qdata (G_OBJECT (buildable), quark_ctk_buildable_accels);
  for (l = accels; l; l = l->next)
    {
      ItemData *data = l->data;

      object = _ctk_builder_lookup_object (builder, data->name, data->line, data->col);
      if (!object)
	continue;
      ctk_window_add_accel_group (CTK_WINDOW (buildable), CTK_ACCEL_GROUP (object));
    }

  g_object_set_qdata (G_OBJECT (buildable), quark_ctk_buildable_accels, NULL);

  parent_buildable_iface->parser_finished (buildable, builder);
}

typedef struct {
  GObject *object;
  CtkBuilder *builder;
  GSList *items;
} GSListSubParserData;

static void
window_start_element (GMarkupParseContext  *context,
                      const gchar          *element_name,
                      const gchar         **names,
                      const gchar         **values,
                      gpointer              user_data,
                      GError              **error)
{
  GSListSubParserData *data = (GSListSubParserData*)user_data;

  if (strcmp (element_name, "group") == 0)
    {
      const gchar *name;
      ItemData *item_data;

      if (!_ctk_builder_check_parent (data->builder, context, "accel-groups", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      item_data = g_new (ItemData, 1);
      item_data->name = g_strdup (name);
      g_markup_parse_context_get_position (context, &item_data->line, &item_data->col);
      data->items = g_slist_prepend (data->items, item_data);
    }
  else if (strcmp (element_name, "accel-groups") == 0)
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
                                        "CtkWindow", element_name,
                                        error);
    }
}

static const GMarkupParser window_parser =
  {
    .start_element = window_start_element
  };

typedef struct {
  GObject *object;
  CtkBuilder *builder;
  gchar *name;
  gint line;
  gint col;
} NameSubParserData;

static void
focus_start_element (GMarkupParseContext  *context,
                     const gchar          *element_name,
                     const gchar         **names,
                     const gchar         **values,
                     gpointer              user_data,
                     GError              **error)
{
  NameSubParserData *data = (NameSubParserData*)user_data;

  if (strcmp (element_name, "initial-focus") == 0)
    {
      const gchar *name;

      if (!_ctk_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      data->name = g_strdup (name);
      g_markup_parse_context_get_position (context, &data->line, &data->col);
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkWindow", element_name,
                                        error);
    }
}

static const GMarkupParser focus_parser =
{
  .start_element = focus_start_element
};

static gboolean
ctk_window_buildable_custom_tag_start (CtkBuildable  *buildable,
                                       CtkBuilder    *builder,
                                       GObject       *child,
                                       const gchar   *tagname,
                                       GMarkupParser *parser,
                                       gpointer      *parser_data)
{
  if (parent_buildable_iface->custom_tag_start (buildable, builder, child,
						tagname, parser, parser_data))
    return TRUE;

  if (strcmp (tagname, "accel-groups") == 0)
    {
      GSListSubParserData *data;

      data = g_slice_new0 (GSListSubParserData);
      data->items = NULL;
      data->object = G_OBJECT (buildable);
      data->builder = builder;

      *parser = window_parser;
      *parser_data = data;

      return TRUE;
    }

  if (strcmp (tagname, "initial-focus") == 0)
    {
      NameSubParserData *data;

      data = g_slice_new0 (NameSubParserData);
      data->name = NULL;
      data->object = G_OBJECT (buildable);
      data->builder = builder;

      *parser = focus_parser;
      *parser_data = data;

      return TRUE;
    }

  return FALSE;
}

static void
ctk_window_buildable_custom_finished (CtkBuildable  *buildable,
                                      CtkBuilder    *builder,
                                      GObject       *child,
                                      const gchar   *tagname,
                                      gpointer       user_data)
{
  parent_buildable_iface->custom_finished (buildable, builder, child,
					   tagname, user_data);

  if (strcmp (tagname, "accel-groups") == 0)
    {
      GSListSubParserData *data = (GSListSubParserData*)user_data;

      g_object_set_qdata_full (G_OBJECT (buildable), quark_ctk_buildable_accels,
                               data->items, (GDestroyNotify) item_list_free);

      g_slice_free (GSListSubParserData, data);
    }

  if (strcmp (tagname, "initial-focus") == 0)
    {
      NameSubParserData *data = (NameSubParserData*)user_data;

      if (data->name)
        {
          GObject *object;

          object = _ctk_builder_lookup_object (builder, data->name, data->line, data->col);
          if (object)
            ctk_window_set_focus (CTK_WINDOW (buildable), CTK_WIDGET (object));
          g_free (data->name);
        }

      g_slice_free (NameSubParserData, data);
    }
}

/**
 * ctk_window_new:
 * @type: type of window
 * 
 * Creates a new #CtkWindow, which is a toplevel window that can
 * contain other widgets. Nearly always, the type of the window should
 * be #CTK_WINDOW_TOPLEVEL. If you’re implementing something like a
 * popup menu from scratch (which is a bad idea, just use #CtkMenu),
 * you might use #CTK_WINDOW_POPUP. #CTK_WINDOW_POPUP is not for
 * dialogs, though in some other toolkits dialogs are called “popups”.
 * In CTK+, #CTK_WINDOW_POPUP means a pop-up menu or pop-up tooltip.
 * On X11, popup windows are not controlled by the
 * [window manager][ctk-X11-arch].
 *
 * If you simply want an undecorated window (no window borders), use
 * ctk_window_set_decorated(), don’t use #CTK_WINDOW_POPUP.
 *
 * All top-level windows created by ctk_window_new() are stored in
 * an internal top-level window list.  This list can be obtained from
 * ctk_window_list_toplevels().  Due to Ctk+ keeping a reference to
 * the window internally, ctk_window_new() does not return a reference
 * to the caller.
 *
 * To delete a #CtkWindow, call ctk_widget_destroy().
 * 
 * Returns: a new #CtkWindow.
 **/
CtkWidget*
ctk_window_new (CtkWindowType type)
{
  CtkWindow *window;

  g_return_val_if_fail (type >= CTK_WINDOW_TOPLEVEL && type <= CTK_WINDOW_POPUP, NULL);

  window = g_object_new (CTK_TYPE_WINDOW, "type", type, NULL);

  return CTK_WIDGET (window);
}

static void
ctk_window_set_title_internal (CtkWindow   *window,
                               const gchar *title,
                               gboolean     update_titlebar)
{
  CtkWindowPrivate *priv;
  CtkWidget *widget;
  char *new_title;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;
  widget = CTK_WIDGET (window);

  new_title = g_strdup (title);
  g_free (priv->title);
  priv->title = new_title;

  if (new_title == NULL)
    new_title = "";

  if (_ctk_widget_get_realized (widget))
    cdk_window_set_title (_ctk_widget_get_window (widget), new_title);

  if (update_titlebar && CTK_IS_HEADER_BAR (priv->title_box))
    ctk_header_bar_set_title (CTK_HEADER_BAR (priv->title_box), new_title);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_TITLE]);
}

/**
 * ctk_window_set_title:
 * @window: a #CtkWindow
 * @title: title of the window
 * 
 * Sets the title of the #CtkWindow. The title of a window will be
 * displayed in its title bar; on the X Window System, the title bar
 * is rendered by the [window manager][ctk-X11-arch],
 * so exactly how the title appears to users may vary
 * according to a user’s exact configuration. The title should help a
 * user distinguish this window from other windows they may have
 * open. A good title might include the application name and current
 * document filename, for example.
 * 
 **/
void
ctk_window_set_title (CtkWindow   *window,
		      const gchar *title)
{
  g_return_if_fail (CTK_IS_WINDOW (window));

  ctk_window_set_title_internal (window, title, TRUE);
}

/**
 * ctk_window_get_title:
 * @window: a #CtkWindow
 *
 * Retrieves the title of the window. See ctk_window_set_title().
 *
 * Returns: (nullable): the title of the window, or %NULL if none has
 * been set explicitly. The returned string is owned by the widget
 * and must not be modified or freed.
 **/
const gchar *
ctk_window_get_title (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), NULL);

  return window->priv->title;
}

/**
 * ctk_window_set_wmclass:
 * @window: a #CtkWindow
 * @wmclass_name: window name hint
 * @wmclass_class: window class hint
 *
 * Don’t use this function. It sets the X Window System “class” and
 * “name” hints for a window.  According to the ICCCM, you should
 * always set these to the same value for all windows in an
 * application, and CTK+ sets them to that value by default, so calling
 * this function is sort of pointless. However, you may want to call
 * ctk_window_set_role() on each window in your application, for the
 * benefit of the session manager. Setting the role allows the window
 * manager to restore window positions when loading a saved session.
 *
 * Deprecated: 3.22
 **/
void
ctk_window_set_wmclass (CtkWindow *window,
			const gchar *wmclass_name,
			const gchar *wmclass_class)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  g_free (priv->wmclass_name);
  priv->wmclass_name = g_strdup (wmclass_name);

  g_free (priv->wmclass_class);
  priv->wmclass_class = g_strdup (wmclass_class);

  if (_ctk_widget_get_realized (CTK_WIDGET (window)))
    g_warning ("ctk_window_set_wmclass: shouldn't set wmclass after window is realized!");
}

/**
 * ctk_window_set_role:
 * @window: a #CtkWindow
 * @role: unique identifier for the window to be used when restoring a session
 *
 * This function is only useful on X11, not with other CTK+ targets.
 * 
 * In combination with the window title, the window role allows a
 * [window manager][ctk-X11-arch] to identify "the
 * same" window when an application is restarted. So for example you
 * might set the “toolbox” role on your app’s toolbox window, so that
 * when the user restarts their session, the window manager can put
 * the toolbox back in the same place.
 *
 * If a window already has a unique title, you don’t need to set the
 * role, since the WM can use the title to identify the window when
 * restoring the session.
 * 
 **/
void
ctk_window_set_role (CtkWindow   *window,
                     const gchar *role)
{
  CtkWindowPrivate *priv;
  CtkWidget *widget;
  char *new_role;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;
  widget = CTK_WIDGET (window);

  new_role = g_strdup (role);
  g_free (priv->wm_role);
  priv->wm_role = new_role;

  if (_ctk_widget_get_realized (widget))
    cdk_window_set_role (_ctk_widget_get_window (widget), priv->wm_role);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_ROLE]);
}

/**
 * ctk_window_set_startup_id:
 * @window: a #CtkWindow
 * @startup_id: a string with startup-notification identifier
 *
 * Startup notification identifiers are used by desktop environment to 
 * track application startup, to provide user feedback and other 
 * features. This function changes the corresponding property on the
 * underlying CdkWindow. Normally, startup identifier is managed 
 * automatically and you should only use this function in special cases
 * like transferring focus from other processes. You should use this
 * function before calling ctk_window_present() or any equivalent
 * function generating a window map event.
 *
 * This function is only useful on X11, not with other CTK+ targets.
 * 
 * Since: 2.12
 **/
void
ctk_window_set_startup_id (CtkWindow   *window,
                           const gchar *startup_id)
{
  CtkWindowPrivate *priv;
  CtkWidget *widget;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;
  widget = CTK_WIDGET (window);

  g_free (priv->startup_id);
  priv->startup_id = g_strdup (startup_id);

  if (_ctk_widget_get_realized (widget))
    {
      CdkWindow *cdk_window;
      guint32 timestamp = extract_time_from_startup_id (priv->startup_id);

      cdk_window = _ctk_widget_get_window (widget);

#ifdef CDK_WINDOWING_X11
      if (timestamp != CDK_CURRENT_TIME && CDK_IS_X11_WINDOW(cdk_window))
	cdk_x11_window_set_user_time (cdk_window, timestamp);
#endif

      /* Here we differentiate real and "fake" startup notification IDs,
       * constructed on purpose just to pass interaction timestamp
       */
      if (startup_id_is_fake (priv->startup_id))
	ctk_window_present_with_time (window, timestamp);
      else 
        {
          cdk_window_set_startup_id (cdk_window,
                                     priv->startup_id);
          
          /* If window is mapped, terminate the startup-notification too */
          if (_ctk_widget_get_mapped (widget) &&
              !disable_startup_notification)
            cdk_notify_startup_complete_with_id (priv->startup_id);
        }
    }

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_STARTUP_ID]);
}

/**
 * ctk_window_get_role:
 * @window: a #CtkWindow
 *
 * Returns the role of the window. See ctk_window_set_role() for
 * further explanation.
 *
 * Returns: (nullable): the role of the window if set, or %NULL. The
 * returned is owned by the widget and must not be modified or freed.
 **/
const gchar *
ctk_window_get_role (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), NULL);

  return window->priv->wm_role;
}

/**
 * ctk_window_set_focus:
 * @window: a #CtkWindow
 * @focus: (allow-none): widget to be the new focus widget, or %NULL to unset
 *   any focus widget for the toplevel window.
 *
 * If @focus is not the current focus widget, and is focusable, sets
 * it as the focus widget for the window. If @focus is %NULL, unsets
 * the focus widget for this window. To set the focus to a particular
 * widget in the toplevel, it is usually more convenient to use
 * ctk_widget_grab_focus() instead of this function.
 **/
void
ctk_window_set_focus (CtkWindow *window,
		      CtkWidget *focus)
{
  CtkWindowPrivate *priv;
  CtkWidget *parent;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  if (focus)
    {
      g_return_if_fail (CTK_IS_WIDGET (focus));
      g_return_if_fail (ctk_widget_get_can_focus (focus));
    }

  if (focus)
    {
      if (!ctk_widget_get_visible (CTK_WIDGET (window)))
        priv->initial_focus = focus;
      else
        ctk_widget_grab_focus (focus);
    }
  else
    {
      /* Clear the existing focus chain, so that when we focus into
       * the window again, we start at the beginnning.
       */
      CtkWidget *widget = priv->focus_widget;
      if (widget)
	{
	  while ((parent = _ctk_widget_get_parent (widget)))
	    {
	      widget = parent;
	      ctk_container_set_focus_child (CTK_CONTAINER (widget), NULL);
	    }
	}
      
      _ctk_window_internal_set_focus (window, NULL);
    }
}

void
_ctk_window_internal_set_focus (CtkWindow *window,
				CtkWidget *focus)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  priv->initial_focus = NULL;
  if ((priv->focus_widget != focus) ||
      (focus && !ctk_widget_has_focus (focus)))
    g_signal_emit (window, window_signals[SET_FOCUS], 0, focus);
}

/**
 * ctk_window_set_default:
 * @window: a #CtkWindow
 * @default_widget: (allow-none): widget to be the default, or %NULL
 *     to unset the default widget for the toplevel
 *
 * The default widget is the widget that’s activated when the user
 * presses Enter in a dialog (for example). This function sets or
 * unsets the default widget for a #CtkWindow. When setting (rather
 * than unsetting) the default widget it’s generally easier to call
 * ctk_widget_grab_default() on the widget. Before making a widget
 * the default widget, you must call ctk_widget_set_can_default() on
 * the widget you’d like to make the default.
 */
void
ctk_window_set_default (CtkWindow *window,
			CtkWidget *default_widget)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  if (default_widget)
    g_return_if_fail (ctk_widget_get_can_default (default_widget));

  if (priv->default_widget != default_widget)
    {
      CtkWidget *old_default_widget = NULL;
      
      if (default_widget)
	g_object_ref (default_widget);

      if (priv->default_widget)
	{
          old_default_widget = priv->default_widget;

          if (priv->focus_widget != priv->default_widget ||
              !ctk_widget_get_receives_default (priv->default_widget))
            _ctk_widget_set_has_default (priv->default_widget, FALSE);

          ctk_widget_queue_draw (priv->default_widget);
	}

      priv->default_widget = default_widget;

      if (priv->default_widget)
	{
          if (priv->focus_widget == NULL ||
              !ctk_widget_get_receives_default (priv->focus_widget))
            _ctk_widget_set_has_default (priv->default_widget, TRUE);

          ctk_widget_queue_draw (priv->default_widget);
	}

      if (old_default_widget)
	g_object_notify (G_OBJECT (old_default_widget), "has-default");
      
      if (default_widget)
	{
	  g_object_notify (G_OBJECT (default_widget), "has-default");
	  g_object_unref (default_widget);
	}
    }
}

/**
 * ctk_window_get_default_widget:
 * @window: a #CtkWindow
 *
 * Returns the default widget for @window. See
 * ctk_window_set_default() for more details.
 *
 * Returns: (nullable) (transfer none): the default widget, or %NULL
 * if there is none.
 *
 * Since: 2.14
 **/
CtkWidget *
ctk_window_get_default_widget (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), NULL);

  return window->priv->default_widget;
}

static gboolean
handle_keys_changed (gpointer data)
{
  CtkWindow *window = CTK_WINDOW (data);
  CtkWindowPrivate *priv = window->priv;

  if (priv->keys_changed_handler)
    {
      g_source_remove (priv->keys_changed_handler);
      priv->keys_changed_handler = 0;
    }

  g_signal_emit (window, window_signals[KEYS_CHANGED], 0);
  
  return FALSE;
}

void
_ctk_window_notify_keys_changed (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;

  if (!priv->keys_changed_handler)
    {
      priv->keys_changed_handler = cdk_threads_add_idle (handle_keys_changed, window);
      g_source_set_name_by_id (priv->keys_changed_handler, "[ctk+] handle_keys_changed");
    }
}

/**
 * ctk_window_add_accel_group:
 * @window: window to attach accelerator group to
 * @accel_group: a #CtkAccelGroup
 *
 * Associate @accel_group with @window, such that calling
 * ctk_accel_groups_activate() on @window will activate accelerators
 * in @accel_group.
 **/
void
ctk_window_add_accel_group (CtkWindow     *window,
			    CtkAccelGroup *accel_group)
{
  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (CTK_IS_ACCEL_GROUP (accel_group));

  _ctk_accel_group_attach (accel_group, G_OBJECT (window));
  g_signal_connect_object (accel_group, "accel-changed",
			   G_CALLBACK (_ctk_window_notify_keys_changed),
			   window, G_CONNECT_SWAPPED);
  _ctk_window_notify_keys_changed (window);
}

/**
 * ctk_window_remove_accel_group:
 * @window: a #CtkWindow
 * @accel_group: a #CtkAccelGroup
 *
 * Reverses the effects of ctk_window_add_accel_group().
 **/
void
ctk_window_remove_accel_group (CtkWindow     *window,
			       CtkAccelGroup *accel_group)
{
  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (CTK_IS_ACCEL_GROUP (accel_group));

  g_signal_handlers_disconnect_by_func (accel_group,
					_ctk_window_notify_keys_changed,
					window);
  _ctk_accel_group_detach (accel_group, G_OBJECT (window));
  _ctk_window_notify_keys_changed (window);
}

static CtkMnemonicHash *
ctk_window_get_mnemonic_hash (CtkWindow *window,
			      gboolean   create)
{
  CtkWindowPrivate *private = window->priv;

  if (!private->mnemonic_hash && create)
    private->mnemonic_hash = _ctk_mnemonic_hash_new ();

  return private->mnemonic_hash;
}

/**
 * ctk_window_add_mnemonic:
 * @window: a #CtkWindow
 * @keyval: the mnemonic
 * @target: the widget that gets activated by the mnemonic
 *
 * Adds a mnemonic to this window.
 */
void
ctk_window_add_mnemonic (CtkWindow *window,
			 guint      keyval,
			 CtkWidget *target)
{
  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (CTK_IS_WIDGET (target));

  _ctk_mnemonic_hash_add (ctk_window_get_mnemonic_hash (window, TRUE),
			  keyval, target);
  _ctk_window_notify_keys_changed (window);
}

/**
 * ctk_window_remove_mnemonic:
 * @window: a #CtkWindow
 * @keyval: the mnemonic
 * @target: the widget that gets activated by the mnemonic
 *
 * Removes a mnemonic from this window.
 */
void
ctk_window_remove_mnemonic (CtkWindow *window,
			    guint      keyval,
			    CtkWidget *target)
{
  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (CTK_IS_WIDGET (target));
  
  _ctk_mnemonic_hash_remove (ctk_window_get_mnemonic_hash (window, TRUE),
			     keyval, target);
  _ctk_window_notify_keys_changed (window);
}

/**
 * ctk_window_mnemonic_activate:
 * @window: a #CtkWindow
 * @keyval: the mnemonic
 * @modifier: the modifiers
 *
 * Activates the targets associated with the mnemonic.
 *
 * Returns: %TRUE if the activation is done.
 */
gboolean
ctk_window_mnemonic_activate (CtkWindow      *window,
			      guint           keyval,
			      CdkModifierType modifier)
{
  CtkWindowPrivate *priv;

  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  priv = window->priv;

  if (priv->mnemonic_modifier == (modifier & ctk_accelerator_get_default_mod_mask ()))
      {
	CtkMnemonicHash *mnemonic_hash = ctk_window_get_mnemonic_hash (window, FALSE);
	if (mnemonic_hash)
	  return _ctk_mnemonic_hash_activate (mnemonic_hash, keyval);
      }

  return FALSE;
}

/**
 * ctk_window_set_mnemonic_modifier:
 * @window: a #CtkWindow
 * @modifier: the modifier mask used to activate
 *               mnemonics on this window.
 *
 * Sets the mnemonic modifier for this window. 
 **/
void
ctk_window_set_mnemonic_modifier (CtkWindow      *window,
				  CdkModifierType modifier)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail ((modifier & ~CDK_MODIFIER_MASK) == 0);

  priv = window->priv;

  priv->mnemonic_modifier = modifier;
  _ctk_window_notify_keys_changed (window);
}

/**
 * ctk_window_get_mnemonic_modifier:
 * @window: a #CtkWindow
 *
 * Returns the mnemonic modifier for this window. See
 * ctk_window_set_mnemonic_modifier().
 *
 * Returns: the modifier mask used to activate
 *               mnemonics on this window.
 **/
CdkModifierType
ctk_window_get_mnemonic_modifier (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), 0);

  return window->priv->mnemonic_modifier;
}

/**
 * ctk_window_set_position:
 * @window: a #CtkWindow.
 * @position: a position constraint.
 *
 * Sets a position constraint for this window. If the old or new
 * constraint is %CTK_WIN_POS_CENTER_ALWAYS, this will also cause
 * the window to be repositioned to satisfy the new constraint. 
 **/
void
ctk_window_set_position (CtkWindow         *window,
			 CtkWindowPosition  position)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  if (position == CTK_WIN_POS_CENTER_ALWAYS ||
      priv->position == CTK_WIN_POS_CENTER_ALWAYS)
    {
      CtkWindowGeometryInfo *info;

      info = ctk_window_get_geometry_info (window, TRUE);

      /* this flag causes us to re-request the CENTER_ALWAYS
       * constraint in ctk_window_move_resize(), see
       * comment in that function.
       */
      info->position_constraints_changed = TRUE;

      ctk_widget_queue_resize_no_redraw (CTK_WIDGET (window));
    }

  if (priv->position != position)
    {
      priv->position = position;
  
      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_WIN_POS]);
    }
}

/**
 * ctk_window_activate_focus:
 * @window: a #CtkWindow
 * 
 * Activates the current focused widget within the window.
 * 
 * Returns: %TRUE if a widget got activated.
 **/
gboolean 
ctk_window_activate_focus (CtkWindow *window)
{
  CtkWindowPrivate *priv;

  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  priv = window->priv;

  if (priv->focus_widget && ctk_widget_is_sensitive (priv->focus_widget))
    return ctk_widget_activate (priv->focus_widget);

  return FALSE;
}

/**
 * ctk_window_get_focus:
 * @window: a #CtkWindow
 * 
 * Retrieves the current focused widget within the window.
 * Note that this is the widget that would have the focus
 * if the toplevel window focused; if the toplevel window
 * is not focused then  `ctk_widget_has_focus (widget)` will
 * not be %TRUE for the widget.
 *
 * Returns: (nullable) (transfer none): the currently focused widget,
 * or %NULL if there is none.
 **/
CtkWidget *
ctk_window_get_focus (CtkWindow *window)
{
  CtkWindowPrivate *priv;

  g_return_val_if_fail (CTK_IS_WINDOW (window), NULL);

  priv = window->priv;

  if (priv->initial_focus)
    return priv->initial_focus;
  else
    return priv->focus_widget;
}

/**
 * ctk_window_activate_default:
 * @window: a #CtkWindow
 * 
 * Activates the default widget for the window, unless the current 
 * focused widget has been configured to receive the default action 
 * (see ctk_widget_set_receives_default()), in which case the
 * focused widget is activated. 
 * 
 * Returns: %TRUE if a widget got activated.
 **/
gboolean
ctk_window_activate_default (CtkWindow *window)
{
  CtkWindowPrivate *priv;

  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  priv = window->priv;

  if (priv->default_widget && ctk_widget_is_sensitive (priv->default_widget) &&
      (!priv->focus_widget || !ctk_widget_get_receives_default (priv->focus_widget)))
    return ctk_widget_activate (priv->default_widget);
  else if (priv->focus_widget && ctk_widget_is_sensitive (priv->focus_widget))
    return ctk_widget_activate (priv->focus_widget);

  return FALSE;
}

/**
 * ctk_window_set_modal:
 * @window: a #CtkWindow
 * @modal: whether the window is modal
 * 
 * Sets a window modal or non-modal. Modal windows prevent interaction
 * with other windows in the same application. To keep modal dialogs
 * on top of main application windows, use
 * ctk_window_set_transient_for() to make the dialog transient for the
 * parent; most [window managers][ctk-X11-arch]
 * will then disallow lowering the dialog below the parent.
 * 
 * 
 **/
void
ctk_window_set_modal (CtkWindow *window,
		      gboolean   modal)
{
  CtkWindowPrivate *priv;
  CtkWidget *widget;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  modal = modal != FALSE;
  if (priv->modal == modal)
    return;

  priv->modal = modal;
  widget = CTK_WIDGET (window);
  
  /* adjust desired modality state */
  if (_ctk_widget_get_realized (widget))
    cdk_window_set_modal_hint (_ctk_widget_get_window (widget), priv->modal);

  if (ctk_widget_get_visible (widget))
    {
      if (priv->modal)
	ctk_grab_add (widget);
      else
	ctk_grab_remove (widget);
    }

  update_window_buttons (window);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_MODAL]);
}

/**
 * ctk_window_get_modal:
 * @window: a #CtkWindow
 * 
 * Returns whether the window is modal. See ctk_window_set_modal().
 *
 * Returns: %TRUE if the window is set to be modal and
 *               establishes a grab when shown
 **/
gboolean
ctk_window_get_modal (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return window->priv->modal;
}

/**
 * ctk_window_list_toplevels:
 * 
 * Returns a list of all existing toplevel windows. The widgets
 * in the list are not individually referenced. If you want
 * to iterate through the list and perform actions involving
 * callbacks that might destroy the widgets, you must call
 * `g_list_foreach (result, (GFunc)g_object_ref, NULL)` first, and
 * then unref all the widgets afterwards.
 *
 * Returns: (element-type CtkWidget) (transfer container): list of toplevel widgets
 **/
GList*
ctk_window_list_toplevels (void)
{
  GList *list = NULL;
  GSList *slist;

  for (slist = toplevel_list; slist; slist = slist->next)
    list = g_list_prepend (list, slist->data);

  return list;
}

static void
remove_attach_widget (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;

  if (priv->attach_widget)
    {
      _ctk_widget_remove_attached_window (priv->attach_widget, window);

      priv->attach_widget = NULL;
    }
}

static void
ctk_window_dispose (GObject *object)
{
  CtkWindow *window = CTK_WINDOW (object);
  CtkWindowPrivate *priv = window->priv;

  ctk_window_set_focus (window, NULL);
  ctk_window_set_default (window, NULL);
  remove_attach_widget (window);

  G_OBJECT_CLASS (ctk_window_parent_class)->dispose (object);
  unset_titlebar (window);

  while (priv->popovers)
    {
      CtkWindowPopover *popover = priv->popovers->data;
      priv->popovers = g_list_delete_link (priv->popovers, priv->popovers);
      popover_destroy (popover);
    }

}

static void
parent_destroyed_callback (CtkWindow *parent, CtkWindow *child)
{
  ctk_widget_destroy (CTK_WIDGET (child));
}

static void
connect_parent_destroyed (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;

  if (priv->transient_parent)
    {
      g_signal_connect (priv->transient_parent,
                        "destroy",
                        G_CALLBACK (parent_destroyed_callback),
                        window);
    }  
}

static void
disconnect_parent_destroyed (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;

  if (priv->transient_parent)
    {
      g_signal_handlers_disconnect_by_func (priv->transient_parent,
					    parent_destroyed_callback,
					    window);
    }
}

static void
ctk_window_transient_parent_realized (CtkWidget *parent,
				      CtkWidget *window)
{
  if (_ctk_widget_get_realized (window))
    cdk_window_set_transient_for (_ctk_widget_get_window (window),
                                  _ctk_widget_get_window (parent));
}

static void
ctk_window_transient_parent_unrealized (CtkWidget *parent,
					CtkWidget *window)
{
  if (_ctk_widget_get_realized (window))
    cdk_property_delete (_ctk_widget_get_window (window),
			 cdk_atom_intern_static_string ("WM_TRANSIENT_FOR"));
}

static void
ctk_window_transient_parent_screen_changed (CtkWindow	*parent,
					    GParamSpec	*pspec,
					    CtkWindow   *window)
{
  ctk_window_set_screen (window, parent->priv->screen);
}

static void       
ctk_window_unset_transient_for  (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;

  if (priv->transient_parent)
    {
      g_signal_handlers_disconnect_by_func (priv->transient_parent,
					    ctk_window_transient_parent_realized,
					    window);
      g_signal_handlers_disconnect_by_func (priv->transient_parent,
					    ctk_window_transient_parent_unrealized,
					    window);
      g_signal_handlers_disconnect_by_func (priv->transient_parent,
					    ctk_window_transient_parent_screen_changed,
					    window);
      g_signal_handlers_disconnect_by_func (priv->transient_parent,
					    ctk_widget_destroyed,
					    &priv->transient_parent);

      if (priv->destroy_with_parent)
        disconnect_parent_destroyed (window);

      priv->transient_parent = NULL;

      if (priv->transient_parent_group)
	{
	  priv->transient_parent_group = FALSE;
	  ctk_window_group_remove_window (priv->group,
					  window);
	}
    }
}

/**
 * ctk_window_set_transient_for:
 * @window: a #CtkWindow
 * @parent: (allow-none): parent window, or %NULL
 *
 * Dialog windows should be set transient for the main application
 * window they were spawned from. This allows
 * [window managers][ctk-X11-arch] to e.g. keep the
 * dialog on top of the main window, or center the dialog over the
 * main window. ctk_dialog_new_with_buttons() and other convenience
 * functions in CTK+ will sometimes call
 * ctk_window_set_transient_for() on your behalf.
 *
 * Passing %NULL for @parent unsets the current transient window.
 *
 * On Wayland, this function can also be used to attach a new
 * #CTK_WINDOW_POPUP to a #CTK_WINDOW_TOPLEVEL parent already mapped
 * on screen so that the #CTK_WINDOW_POPUP will be created as a
 * subsurface-based window #CDK_WINDOW_SUBSURFACE which can be
 * positioned at will relatively to the #CTK_WINDOW_TOPLEVEL surface.
 *
 * On Windows, this function puts the child window on top of the parent,
 * much as the window manager would have done on X.
 */
void
ctk_window_set_transient_for  (CtkWindow *window,
			       CtkWindow *parent)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (parent == NULL || CTK_IS_WINDOW (parent));
  g_return_if_fail (window != parent);

  priv = window->priv;

  if (priv->transient_parent)
    {
      if (_ctk_widget_get_realized (CTK_WIDGET (window)) &&
          _ctk_widget_get_realized (CTK_WIDGET (priv->transient_parent)) &&
          (!parent || !_ctk_widget_get_realized (CTK_WIDGET (parent))))
	ctk_window_transient_parent_unrealized (CTK_WIDGET (priv->transient_parent),
						CTK_WIDGET (window));

      ctk_window_unset_transient_for (window);
    }

  priv->transient_parent = parent;

  if (parent)
    {
      g_signal_connect (parent, "destroy",
			G_CALLBACK (ctk_widget_destroyed),
			&priv->transient_parent);
      g_signal_connect (parent, "realize",
			G_CALLBACK (ctk_window_transient_parent_realized),
			window);
      g_signal_connect (parent, "unrealize",
			G_CALLBACK (ctk_window_transient_parent_unrealized),
			window);
      g_signal_connect (parent, "notify::screen",
			G_CALLBACK (ctk_window_transient_parent_screen_changed),
			window);

      ctk_window_set_screen (window, parent->priv->screen);

      if (priv->destroy_with_parent)
        connect_parent_destroyed (window);
      
      if (_ctk_widget_get_realized (CTK_WIDGET (window)) &&
	  _ctk_widget_get_realized (CTK_WIDGET (parent)))
	ctk_window_transient_parent_realized (CTK_WIDGET (parent),
					      CTK_WIDGET (window));

      if (parent->priv->group)
	{
	  ctk_window_group_add_window (parent->priv->group, window);
	  priv->transient_parent_group = TRUE;
	}
    }

  update_window_buttons (window);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_TRANSIENT_FOR]);
}

/**
 * ctk_window_get_transient_for:
 * @window: a #CtkWindow
 *
 * Fetches the transient parent for this window. See
 * ctk_window_set_transient_for().
 *
 * Returns: (nullable) (transfer none): the transient parent for this
 * window, or %NULL if no transient parent has been set.
 **/
CtkWindow *
ctk_window_get_transient_for (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), NULL);

  return window->priv->transient_parent;
}

/**
 * ctk_window_set_attached_to:
 * @window: a #CtkWindow
 * @attach_widget: (allow-none): a #CtkWidget, or %NULL
 *
 * Marks @window as attached to @attach_widget. This creates a logical binding
 * between the window and the widget it belongs to, which is used by CTK+ to
 * propagate information such as styling or accessibility to @window as if it
 * was a children of @attach_widget.
 *
 * Examples of places where specifying this relation is useful are for instance
 * a #CtkMenu created by a #CtkComboBox, a completion popup window
 * created by #CtkEntry or a typeahead search entry created by #CtkTreeView.
 *
 * Note that this function should not be confused with
 * ctk_window_set_transient_for(), which specifies a window manager relation
 * between two toplevels instead.
 *
 * Passing %NULL for @attach_widget detaches the window.
 *
 * Since: 3.4
 **/
void
ctk_window_set_attached_to (CtkWindow *window,
                            CtkWidget *attach_widget)
{
  CtkStyleContext *context;
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (CTK_WIDGET (window) != attach_widget);

  priv = window->priv;

  if (priv->attach_widget == attach_widget)
    return;

  remove_attach_widget (window);

  priv->attach_widget = attach_widget;

  if (priv->attach_widget)
    {
      _ctk_widget_add_attached_window (priv->attach_widget, window);
    }

  /* Update the style, as the widget path might change. */
  context = ctk_widget_get_style_context (CTK_WIDGET (window));
  if (priv->attach_widget)
    ctk_style_context_set_parent (context, ctk_widget_get_style_context (priv->attach_widget));
  else
    ctk_style_context_set_parent (context, NULL);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_ATTACHED_TO]);
}

/**
 * ctk_window_get_attached_to:
 * @window: a #CtkWindow
 *
 * Fetches the attach widget for this window. See
 * ctk_window_set_attached_to().
 *
 * Returns: (nullable) (transfer none): the widget where the window
 * is attached, or %NULL if the window is not attached to any widget.
 *
 * Since: 3.4
 **/
CtkWidget *
ctk_window_get_attached_to (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), NULL);

  return window->priv->attach_widget;
}

/**
 * ctk_window_set_opacity:
 * @window: a #CtkWindow
 * @opacity: desired opacity, between 0 and 1
 *
 * Request the windowing system to make @window partially transparent,
 * with opacity 0 being fully transparent and 1 fully opaque. (Values
 * of the opacity parameter are clamped to the [0,1] range.) On X11
 * this has any effect only on X screens with a compositing manager
 * running. See ctk_widget_is_composited(). On Windows it should work
 * always.
 * 
 * Note that setting a window’s opacity after the window has been
 * shown causes it to flicker once on Windows.
 *
 * Since: 2.12
 * Deprecated: 3.8: Use ctk_widget_set_opacity instead.
 **/
void       
ctk_window_set_opacity  (CtkWindow *window, 
			 gdouble    opacity)
{
  ctk_widget_set_opacity (CTK_WIDGET (window), opacity);
}

/**
 * ctk_window_get_opacity:
 * @window: a #CtkWindow
 *
 * Fetches the requested opacity for this window. See
 * ctk_window_set_opacity().
 *
 * Returns: the requested opacity for this window.
 *
 * Since: 2.12
 * Deprecated: 3.8: Use ctk_widget_get_opacity instead.
 **/
gdouble
ctk_window_get_opacity (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), 0.0);

  return ctk_widget_get_opacity (CTK_WIDGET (window));
}

/**
 * ctk_window_get_application:
 * @window: a #CtkWindow
 *
 * Gets the #CtkApplication associated with the window (if any).
 *
 * Returns: (nullable) (transfer none): a #CtkApplication, or %NULL
 *
 * Since: 3.0
 **/
CtkApplication *
ctk_window_get_application (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), NULL);

  return window->priv->application;
}

static void
ctk_window_release_application (CtkWindow *window)
{
  if (window->priv->application)
    {
      CtkApplication *application;

      /* steal reference into temp variable */
      application = window->priv->application;
      window->priv->application = NULL;

      ctk_application_remove_window (application, window);
      g_object_unref (application);
    }
}

/**
 * ctk_window_set_application:
 * @window: a #CtkWindow
 * @application: (allow-none): a #CtkApplication, or %NULL to unset
 *
 * Sets or unsets the #CtkApplication associated with the window.
 *
 * The application will be kept alive for at least as long as it has any windows
 * associated with it (see g_application_hold() for a way to keep it alive
 * without windows).
 *
 * Normally, the connection between the application and the window will remain
 * until the window is destroyed, but you can explicitly remove it by setting
 * the @application to %NULL.
 *
 * This is equivalent to calling ctk_application_remove_window() and/or
 * ctk_application_add_window() on the old/new applications as relevant.
 *
 * Since: 3.0
 **/
void
ctk_window_set_application (CtkWindow      *window,
                            CtkApplication *application)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;
  if (priv->application != application)
    {
      ctk_window_release_application (window);

      priv->application = application;

      if (priv->application != NULL)
        {
          g_object_ref (priv->application);

          ctk_application_add_window (priv->application, window);
        }

      _ctk_widget_update_parent_muxer (CTK_WIDGET (window));

      _ctk_window_notify_keys_changed (window);

      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_APPLICATION]);
    }
}

/**
 * ctk_window_set_type_hint:
 * @window: a #CtkWindow
 * @hint: the window type
 *
 * By setting the type hint for the window, you allow the window
 * manager to decorate and handle the window in a way which is
 * suitable to the function of the window in your application.
 *
 * This function should be called before the window becomes visible.
 *
 * ctk_dialog_new_with_buttons() and other convenience functions in CTK+
 * will sometimes call ctk_window_set_type_hint() on your behalf.
 * 
 **/
void
ctk_window_set_type_hint (CtkWindow           *window, 
			  CdkWindowTypeHint    hint)
{
  CtkWindowPrivate *priv;
  CdkWindow *cdk_window;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  if (priv->type_hint == hint)
    return;

  priv->type_hint = hint;

  cdk_window = _ctk_widget_get_window (CTK_WIDGET (window));
  if (cdk_window)
    cdk_window_set_type_hint (cdk_window, hint);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_TYPE_HINT]);

  update_window_buttons (window);
}

/**
 * ctk_window_get_type_hint:
 * @window: a #CtkWindow
 *
 * Gets the type hint for this window. See ctk_window_set_type_hint().
 *
 * Returns: the type hint for @window.
 **/
CdkWindowTypeHint
ctk_window_get_type_hint (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), CDK_WINDOW_TYPE_HINT_NORMAL);

  return window->priv->type_hint;
}

/**
 * ctk_window_set_skip_taskbar_hint:
 * @window: a #CtkWindow 
 * @setting: %TRUE to keep this window from appearing in the task bar
 * 
 * Windows may set a hint asking the desktop environment not to display
 * the window in the task bar. This function sets this hint.
 * 
 * Since: 2.2
 **/
void
ctk_window_set_skip_taskbar_hint (CtkWindow *window,
                                  gboolean   setting)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  setting = setting != FALSE;

  if (priv->skips_taskbar != setting)
    {
      priv->skips_taskbar = setting;
      if (_ctk_widget_get_realized (CTK_WIDGET (window)))
        cdk_window_set_skip_taskbar_hint (_ctk_widget_get_window (CTK_WIDGET (window)),
                                          priv->skips_taskbar);
      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_SKIP_TASKBAR_HINT]);
    }
}

/**
 * ctk_window_get_skip_taskbar_hint:
 * @window: a #CtkWindow
 * 
 * Gets the value set by ctk_window_set_skip_taskbar_hint()
 * 
 * Returns: %TRUE if window shouldn’t be in taskbar
 * 
 * Since: 2.2
 **/
gboolean
ctk_window_get_skip_taskbar_hint (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return window->priv->skips_taskbar;
}

/**
 * ctk_window_set_skip_pager_hint:
 * @window: a #CtkWindow 
 * @setting: %TRUE to keep this window from appearing in the pager
 * 
 * Windows may set a hint asking the desktop environment not to display
 * the window in the pager. This function sets this hint.
 * (A "pager" is any desktop navigation tool such as a workspace
 * switcher that displays a thumbnail representation of the windows
 * on the screen.)
 * 
 * Since: 2.2
 **/
void
ctk_window_set_skip_pager_hint (CtkWindow *window,
                                gboolean   setting)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  setting = setting != FALSE;

  if (priv->skips_pager != setting)
    {
      priv->skips_pager = setting;
      if (_ctk_widget_get_realized (CTK_WIDGET (window)))
        cdk_window_set_skip_pager_hint (_ctk_widget_get_window (CTK_WIDGET (window)),
                                        priv->skips_pager);
      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_SKIP_PAGER_HINT]);
    }
}

/**
 * ctk_window_get_skip_pager_hint:
 * @window: a #CtkWindow
 * 
 * Gets the value set by ctk_window_set_skip_pager_hint().
 * 
 * Returns: %TRUE if window shouldn’t be in pager
 * 
 * Since: 2.2
 **/
gboolean
ctk_window_get_skip_pager_hint (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return window->priv->skips_pager;
}

/**
 * ctk_window_set_urgency_hint:
 * @window: a #CtkWindow 
 * @setting: %TRUE to mark this window as urgent
 * 
 * Windows may set a hint asking the desktop environment to draw
 * the users attention to the window. This function sets this hint.
 * 
 * Since: 2.8
 **/
void
ctk_window_set_urgency_hint (CtkWindow *window,
			     gboolean   setting)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  setting = setting != FALSE;

  if (priv->urgent != setting)
    {
      priv->urgent = setting;
      if (_ctk_widget_get_realized (CTK_WIDGET (window)))
        cdk_window_set_urgency_hint (_ctk_widget_get_window (CTK_WIDGET (window)),
				     priv->urgent);
      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_URGENCY_HINT]);
    }
}

/**
 * ctk_window_get_urgency_hint:
 * @window: a #CtkWindow
 * 
 * Gets the value set by ctk_window_set_urgency_hint()
 * 
 * Returns: %TRUE if window is urgent
 * 
 * Since: 2.8
 **/
gboolean
ctk_window_get_urgency_hint (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return window->priv->urgent;
}

/**
 * ctk_window_set_accept_focus:
 * @window: a #CtkWindow 
 * @setting: %TRUE to let this window receive input focus
 * 
 * Windows may set a hint asking the desktop environment not to receive
 * the input focus. This function sets this hint.
 * 
 * Since: 2.4
 **/
void
ctk_window_set_accept_focus (CtkWindow *window,
			     gboolean   setting)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  setting = setting != FALSE;

  if (priv->accept_focus != setting)
    {
      priv->accept_focus = setting;
      if (_ctk_widget_get_realized (CTK_WIDGET (window)))
        cdk_window_set_accept_focus (_ctk_widget_get_window (CTK_WIDGET (window)),
				     priv->accept_focus);
      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_ACCEPT_FOCUS]);
    }
}

/**
 * ctk_window_get_accept_focus:
 * @window: a #CtkWindow
 * 
 * Gets the value set by ctk_window_set_accept_focus().
 * 
 * Returns: %TRUE if window should receive the input focus
 * 
 * Since: 2.4
 **/
gboolean
ctk_window_get_accept_focus (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return window->priv->accept_focus;
}

/**
 * ctk_window_set_focus_on_map:
 * @window: a #CtkWindow 
 * @setting: %TRUE to let this window receive input focus on map
 * 
 * Windows may set a hint asking the desktop environment not to receive
 * the input focus when the window is mapped.  This function sets this
 * hint.
 * 
 * Since: 2.6
 **/
void
ctk_window_set_focus_on_map (CtkWindow *window,
			     gboolean   setting)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  setting = setting != FALSE;

  if (priv->focus_on_map != setting)
    {
      priv->focus_on_map = setting;
      if (_ctk_widget_get_realized (CTK_WIDGET (window)))
        cdk_window_set_focus_on_map (_ctk_widget_get_window (CTK_WIDGET (window)),
				     priv->focus_on_map);
      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_FOCUS_ON_MAP]);
    }
}

/**
 * ctk_window_get_focus_on_map:
 * @window: a #CtkWindow
 * 
 * Gets the value set by ctk_window_set_focus_on_map().
 * 
 * Returns: %TRUE if window should receive the input focus when
 * mapped.
 * 
 * Since: 2.6
 **/
gboolean
ctk_window_get_focus_on_map (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return window->priv->focus_on_map;
}

/**
 * ctk_window_set_destroy_with_parent:
 * @window: a #CtkWindow
 * @setting: whether to destroy @window with its transient parent
 * 
 * If @setting is %TRUE, then destroying the transient parent of @window
 * will also destroy @window itself. This is useful for dialogs that
 * shouldn’t persist beyond the lifetime of the main window they're
 * associated with, for example.
 **/
void
ctk_window_set_destroy_with_parent  (CtkWindow *window,
                                     gboolean   setting)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  if (priv->destroy_with_parent == (setting != FALSE))
    return;

  if (priv->destroy_with_parent)
    {
      disconnect_parent_destroyed (window);
    }
  else
    {
      connect_parent_destroyed (window);
    }

  priv->destroy_with_parent = setting;

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_DESTROY_WITH_PARENT]);
}

/**
 * ctk_window_get_destroy_with_parent:
 * @window: a #CtkWindow
 * 
 * Returns whether the window will be destroyed with its transient parent. See
 * ctk_window_set_destroy_with_parent ().
 *
 * Returns: %TRUE if the window will be destroyed with its transient parent.
 **/
gboolean
ctk_window_get_destroy_with_parent (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return window->priv->destroy_with_parent;
}

static void
ctk_window_apply_hide_titlebar_when_maximized (CtkWindow *window)
{
#ifdef CDK_WINDOWING_X11
  CdkWindow *cdk_window;
  gboolean setting;

  setting = window->priv->hide_titlebar_when_maximized;
  cdk_window = _ctk_widget_get_window (CTK_WIDGET (window));

  if (CDK_IS_X11_WINDOW (cdk_window))
    cdk_x11_window_set_hide_titlebar_when_maximized (cdk_window, setting);
#endif
}

/**
 * ctk_window_set_hide_titlebar_when_maximized:
 * @window: a #CtkWindow
 * @setting: whether to hide the titlebar when @window is maximized
 *
 * If @setting is %TRUE, then @window will request that it’s titlebar
 * should be hidden when maximized.
 * This is useful for windows that don’t convey any information other
 * than the application name in the titlebar, to put the available
 * screen space to better use. If the underlying window system does not
 * support the request, the setting will not have any effect.
 *
 * Note that custom titlebars set with ctk_window_set_titlebar() are
 * not affected by this. The application is in full control of their
 * content and visibility anyway.
 * 
 * Since: 3.4
 **/
void
ctk_window_set_hide_titlebar_when_maximized (CtkWindow *window,
                                             gboolean   setting)
{
  g_return_if_fail (CTK_IS_WINDOW (window));

  if (window->priv->hide_titlebar_when_maximized == setting)
    return;

  window->priv->hide_titlebar_when_maximized = setting;
  ctk_window_apply_hide_titlebar_when_maximized (window);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_HIDE_TITLEBAR_WHEN_MAXIMIZED]);
}

/**
 * ctk_window_get_hide_titlebar_when_maximized:
 * @window: a #CtkWindow
 *
 * Returns whether the window has requested to have its titlebar hidden
 * when maximized. See ctk_window_set_hide_titlebar_when_maximized ().
 *
 * Returns: %TRUE if the window has requested to have its titlebar
 *               hidden when maximized
 *
 * Since: 3.4
 **/
gboolean
ctk_window_get_hide_titlebar_when_maximized (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return window->priv->hide_titlebar_when_maximized;
}

static CtkWindowGeometryInfo*
ctk_window_get_geometry_info (CtkWindow *window,
			      gboolean   create)
{
  CtkWindowPrivate *priv = window->priv;
  CtkWindowGeometryInfo *info;

  info = priv->geometry_info;
  if (!info && create)
    {
      info = g_new0 (CtkWindowGeometryInfo, 1);

      info->default_width = -1;
      info->default_height = -1;
      info->resize_width = -1;
      info->resize_height = -1;
      info->initial_x = 0;
      info->initial_y = 0;
      info->initial_pos_set = FALSE;
      info->default_is_geometry = FALSE;
      info->position_constraints_changed = FALSE;
      info->last.configure_request.x = 0;
      info->last.configure_request.y = 0;
      info->last.configure_request.width = -1;
      info->last.configure_request.height = -1;
      info->mask = 0;
      priv->geometry_info = info;
    }

  return info;
}

/**
 * ctk_window_set_geometry_hints:
 * @window: a #CtkWindow
 * @geometry_widget: (allow-none): widget the geometry hints used to be applied to
 *   or %NULL. Since 3.20 this argument is ignored and CTK behaves as if %NULL was
 *   set.
 * @geometry: (allow-none): struct containing geometry information or %NULL
 * @geom_mask: mask indicating which struct fields should be paid attention to
 *
 * This function sets up hints about how a window can be resized by
 * the user.  You can set a minimum and maximum size; allowed resize
 * increments (e.g. for xterm, you can only resize by the size of a
 * character); aspect ratios; and more. See the #CdkGeometry struct.
 */
void
ctk_window_set_geometry_hints (CtkWindow       *window,
			       CtkWidget       *geometry_widget,
			       CdkGeometry     *geometry,
			       CdkWindowHints   geom_mask)
{
  CtkWindowGeometryInfo *info;

  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (geometry_widget == NULL || CTK_IS_WIDGET (geometry_widget));

  info = ctk_window_get_geometry_info (window, TRUE);

  if (geometry)
    info->geometry = *geometry;

  /* We store gravity in priv->gravity not in the hints. */
  info->mask = geom_mask & ~(CDK_HINT_WIN_GRAVITY);

  if (geometry_widget)
    info->mask &= ~(CDK_HINT_BASE_SIZE | CDK_HINT_RESIZE_INC);

  if (geom_mask & CDK_HINT_WIN_GRAVITY)
    ctk_window_set_gravity (window, geometry->win_gravity);

  ctk_widget_queue_resize_no_redraw (CTK_WIDGET (window));
}

static void
unset_titlebar (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;

  if (priv->title_box != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->title_box,
                                            on_titlebar_title_notify,
                                            window);
      ctk_widget_unparent (priv->title_box);
      priv->title_box = NULL;
      priv->titlebar = NULL;
    }
}

static gboolean
ctk_window_supports_client_shadow (CtkWindow *window)
{
  CdkDisplay *display;
  CdkScreen *screen;
  CdkVisual *visual;

  screen = _ctk_window_get_screen (window);
  display = cdk_screen_get_display (screen);

#ifdef CDK_WINDOWING_X11
  if (CDK_IS_X11_DISPLAY (display))
    {
      if (!cdk_screen_is_composited (screen))
        return FALSE;

      if (!cdk_x11_screen_supports_net_wm_hint (screen, cdk_atom_intern_static_string ("_CTK_FRAME_EXTENTS")))
        return FALSE;

      /* We need a visual with alpha */
      visual = cdk_screen_get_rgba_visual (screen);
      if (!visual)
        return FALSE;
    }
#endif

#ifdef CDK_WINDOWING_WIN32
  if (CDK_IS_WIN32_DISPLAY (display))
    {
      if (!cdk_screen_is_composited (screen))
        return FALSE;

      /* We need a visual with alpha */
      visual = cdk_screen_get_rgba_visual (screen);
      if (!visual)
        return FALSE;
    }
#endif

  return TRUE;
}

static void
ctk_window_enable_csd (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;
  CtkWidget *widget = CTK_WIDGET (window);
  CdkVisual *visual;

  /* We need a visual with alpha for client shadows */
  if (priv->use_client_shadow)
    {
      visual = cdk_screen_get_rgba_visual (ctk_widget_get_screen (widget));
      if (visual != NULL)
        ctk_widget_set_visual (widget, visual);

      ctk_style_context_add_class (ctk_widget_get_style_context (widget), CTK_STYLE_CLASS_CSD);
    }
  else
    {
      ctk_style_context_add_class (ctk_widget_get_style_context (widget), "solid-csd");
    }

  priv->client_decorated = TRUE;
}

static void
on_titlebar_title_notify (CtkHeaderBar *titlebar,
                          GParamSpec   *pspec,
                          CtkWindow    *self)
{
  const gchar *title;

  title = ctk_header_bar_get_title (titlebar);
  ctk_window_set_title_internal (self, title, FALSE);
}

/**
 * ctk_window_set_titlebar:
 * @window: a #CtkWindow
 * @titlebar: (allow-none): the widget to use as titlebar
 *
 * Sets a custom titlebar for @window.
 *
 * A typical widget used here is #CtkHeaderBar, as it provides various features
 * expected of a titlebar while allowing the addition of child widgets to it.
 *
 * If you set a custom titlebar, CTK+ will do its best to convince
 * the window manager not to put its own titlebar on the window.
 * Depending on the system, this function may not work for a window
 * that is already visible, so you set the titlebar before calling
 * ctk_widget_show().
 *
 * Since: 3.10
 */
void
ctk_window_set_titlebar (CtkWindow *window,
                         CtkWidget *titlebar)
{
  CtkWidget *widget = CTK_WIDGET (window);
  CtkWindowPrivate *priv = window->priv;
  gboolean was_mapped;

  g_return_if_fail (CTK_IS_WINDOW (window));

  if ((!priv->title_box && titlebar) || (priv->title_box && !titlebar))
    {
      was_mapped = _ctk_widget_get_mapped (widget);
      if (_ctk_widget_get_realized (widget))
        {
          g_warning ("ctk_window_set_titlebar() called on a realized window");
          ctk_widget_unrealize (widget);
        }
    }
  else
    was_mapped = FALSE;

  unset_titlebar (window);

  if (titlebar == NULL)
    {
      priv->client_decorated = FALSE;
      ctk_style_context_remove_class (ctk_widget_get_style_context (widget), CTK_STYLE_CLASS_CSD);

      goto out;
    }

  priv->use_client_shadow = ctk_window_supports_client_shadow (window);

  ctk_window_enable_csd (window);
  priv->title_box = titlebar;
  ctk_widget_set_parent (priv->title_box, widget);
  if (CTK_IS_HEADER_BAR (titlebar))
    {
      g_signal_connect (titlebar, "notify::title",
                        G_CALLBACK (on_titlebar_title_notify), window);
      on_titlebar_title_notify (CTK_HEADER_BAR (titlebar), NULL, window);
    }

  ctk_style_context_add_class (ctk_widget_get_style_context (titlebar),
                               CTK_STYLE_CLASS_TITLEBAR);

out:
  if (was_mapped)
    ctk_widget_map (widget);
}

/**
 * ctk_window_get_titlebar:
 * @window: a #CtkWindow
 *
 * Returns the custom titlebar that has been set with
 * ctk_window_set_titlebar().
 *
 * Returns: (nullable) (transfer none): the custom titlebar, or %NULL
 *
 * Since: 3.16
 */
CtkWidget *
ctk_window_get_titlebar (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;

  g_return_val_if_fail (CTK_IS_WINDOW (window), NULL);

  /* Don't return the internal titlebar */
  if (priv->title_box == priv->titlebar)
    return NULL;

  return priv->title_box;
}

gboolean
_ctk_window_titlebar_shows_app_menu (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;

  if (CTK_IS_HEADER_BAR (priv->title_box))
    return _ctk_header_bar_shows_app_menu (CTK_HEADER_BAR (priv->title_box));

  return FALSE;
}

/**
 * ctk_window_set_decorated:
 * @window: a #CtkWindow
 * @setting: %TRUE to decorate the window
 *
 * By default, windows are decorated with a title bar, resize
 * controls, etc.  Some [window managers][ctk-X11-arch]
 * allow CTK+ to disable these decorations, creating a
 * borderless window. If you set the decorated property to %FALSE
 * using this function, CTK+ will do its best to convince the window
 * manager not to decorate the window. Depending on the system, this
 * function may not have any effect when called on a window that is
 * already visible, so you should call it before calling ctk_widget_show().
 *
 * On Windows, this function always works, since there’s no window manager
 * policy involved.
 * 
 **/
void
ctk_window_set_decorated (CtkWindow *window,
                          gboolean   setting)
{
  CtkWindowPrivate *priv;
  CdkWindow *cdk_window;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  setting = setting != FALSE;

  if (setting == priv->decorated)
    return;

  priv->decorated = setting;

  cdk_window = _ctk_widget_get_window (CTK_WIDGET (window));
  if (cdk_window)
    {
      if (priv->decorated)
        {
          if (priv->client_decorated)
            cdk_window_set_decorations (cdk_window, 0);
          else
            cdk_window_set_decorations (cdk_window, CDK_DECOR_ALL);
        }
      else
        cdk_window_set_decorations (cdk_window, 0);
    }

  update_window_buttons (window);
  ctk_widget_queue_resize (CTK_WIDGET (window));

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_DECORATED]);
}

/**
 * ctk_window_get_decorated:
 * @window: a #CtkWindow
 *
 * Returns whether the window has been set to have decorations
 * such as a title bar via ctk_window_set_decorated().
 *
 * Returns: %TRUE if the window has been set to have decorations
 **/
gboolean
ctk_window_get_decorated (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), TRUE);

  return window->priv->decorated;
}

/**
 * ctk_window_set_deletable:
 * @window: a #CtkWindow
 * @setting: %TRUE to decorate the window as deletable
 *
 * By default, windows have a close button in the window frame. Some 
 * [window managers][ctk-X11-arch] allow CTK+ to 
 * disable this button. If you set the deletable property to %FALSE
 * using this function, CTK+ will do its best to convince the window
 * manager not to show a close button. Depending on the system, this
 * function may not have any effect when called on a window that is
 * already visible, so you should call it before calling ctk_widget_show().
 *
 * On Windows, this function always works, since there’s no window manager
 * policy involved.
 *
 * Since: 2.10
 */
void
ctk_window_set_deletable (CtkWindow *window,
			  gboolean   setting)
{
  CtkWindowPrivate *priv;
  CdkWindow *cdk_window;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  setting = setting != FALSE;

  if (setting == priv->deletable)
    return;

  priv->deletable = setting;

  cdk_window = _ctk_widget_get_window (CTK_WIDGET (window));
  if (cdk_window)
    {
      if (priv->deletable)
        cdk_window_set_functions (cdk_window,
				  CDK_FUNC_ALL);
      else
        cdk_window_set_functions (cdk_window,
				  CDK_FUNC_ALL | CDK_FUNC_CLOSE);
    }

  update_window_buttons (window);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_DELETABLE]);
}

/**
 * ctk_window_get_deletable:
 * @window: a #CtkWindow
 *
 * Returns whether the window has been set to have a close button
 * via ctk_window_set_deletable().
 *
 * Returns: %TRUE if the window has been set to have a close button
 *
 * Since: 2.10
 **/
gboolean
ctk_window_get_deletable (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), TRUE);

  return window->priv->deletable;
}

static CtkWindowIconInfo*
get_icon_info (CtkWindow *window)
{
  return g_object_get_qdata (G_OBJECT (window), quark_ctk_window_icon_info);
}
     
static void
free_icon_info (CtkWindowIconInfo *info)
{
  g_free (info->icon_name);
  g_slice_free (CtkWindowIconInfo, info);
}


static CtkWindowIconInfo*
ensure_icon_info (CtkWindow *window)
{
  CtkWindowIconInfo *info;

  info = get_icon_info (window);
  
  if (info == NULL)
    {
      info = g_slice_new0 (CtkWindowIconInfo);
      g_object_set_qdata_full (G_OBJECT (window),
                              quark_ctk_window_icon_info,
                              info,
                              (GDestroyNotify)free_icon_info);
    }

  return info;
}

static GList *
icon_list_from_theme (CtkWindow   *window,
		      const gchar *name)
{
  GList *list;

  CtkIconTheme *icon_theme;
  GdkPixbuf *icon;
  gint *sizes;
  gint i;

  icon_theme = ctk_css_icon_theme_value_get_icon_theme
    (_ctk_style_context_peek_property (ctk_widget_get_style_context (CTK_WIDGET (window)),
                                       CTK_CSS_PROPERTY_ICON_THEME));

  sizes = ctk_icon_theme_get_icon_sizes (icon_theme, name);

  list = NULL;
  for (i = 0; sizes[i]; i++)
    {
      /* FIXME
       * We need an EWMH extension to handle scalable icons 
       * by passing their name to the WM. For now just use a 
       * fixed size of 48.
       */ 
      if (sizes[i] == -1)
	icon = ctk_icon_theme_load_icon (icon_theme, name,
					 48, 0, NULL);
      else
	icon = ctk_icon_theme_load_icon (icon_theme, name,
					 sizes[i], 0, NULL);
      if (icon)
	list = g_list_append (list, icon);
    }

  g_free (sizes);

  return list;
}

static void
ctk_window_realize_icon (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;
  CtkWidget *widget;
  CtkWindowIconInfo *info;
  CdkWindow *cdk_window;
  GList *icon_list;

  widget = CTK_WIDGET (window);
  cdk_window = _ctk_widget_get_window (widget);

  g_return_if_fail (cdk_window != NULL);

  /* no point setting an icon on override-redirect */
  if (priv->type == CTK_WINDOW_POPUP)
    return;

  icon_list = NULL;
  
  info = ensure_icon_info (window);

  if (info->realized)
    return;

  info->using_default_icon = FALSE;
  info->using_parent_icon = FALSE;
  info->using_themed_icon = FALSE;
  
  icon_list = info->icon_list;

  /* Look up themed icon */
  if (icon_list == NULL && info->icon_name) 
    {
      icon_list = icon_list_from_theme (window, info->icon_name);
      if (icon_list)
	info->using_themed_icon = TRUE;
    }

  /* Inherit from transient parent */
  if (icon_list == NULL && priv->transient_parent)
    {
      icon_list = ensure_icon_info (priv->transient_parent)->icon_list;
      if (icon_list)
        info->using_parent_icon = TRUE;
    }      

  /* Inherit from default */
  if (icon_list == NULL)
    {
      icon_list = default_icon_list;
      if (icon_list)
        info->using_default_icon = TRUE;
    }

  /* Look up themed icon */
  if (icon_list == NULL && default_icon_name) 
    {
      icon_list = icon_list_from_theme (window, default_icon_name);
      info->using_default_icon = TRUE;
      info->using_themed_icon = TRUE;
    }

  info->realized = TRUE;

  cdk_window_set_icon_list (cdk_window, icon_list);
  if (CTK_IS_HEADER_BAR (priv->title_box))
    _ctk_header_bar_update_window_icon (CTK_HEADER_BAR (priv->title_box), window);

  if (info->using_themed_icon) 
    {
      g_list_free_full (icon_list, g_object_unref);
    }
}

static GdkPixbuf *
icon_from_list (GList *list,
                gint   size)
{
  GdkPixbuf *best;
  GdkPixbuf *pixbuf;
  GList *l;

  best = NULL;
  for (l = list; l; l = l->next)
    {
      pixbuf = list->data;
      if (gdk_pixbuf_get_width (pixbuf) <= size &&
          gdk_pixbuf_get_height (pixbuf) <= size)
        {
          best = g_object_ref (pixbuf);
          break;
        }
    }

  if (best == NULL)
    best = gdk_pixbuf_scale_simple (GDK_PIXBUF (list->data), size, size, GDK_INTERP_BILINEAR);

  return best;
}

static GdkPixbuf *
icon_from_name (const gchar *name,
                gint         size)
{
  return ctk_icon_theme_load_icon (ctk_icon_theme_get_default (),
                                   name, size,
                                   CTK_ICON_LOOKUP_FORCE_SIZE, NULL);
}

GdkPixbuf *
ctk_window_get_icon_for_size (CtkWindow *window,
                              gint       size)
{
  CtkWindowPrivate *priv = window->priv;
  CtkWindowIconInfo *info;
  const gchar *name;

  info = ensure_icon_info (window);

  if (info->icon_list != NULL)
    return icon_from_list (info->icon_list, size);

  name = ctk_window_get_icon_name (window);
  if (name != NULL)
    return icon_from_name (name, size);

  if (priv->transient_parent != NULL)
    {
      info = ensure_icon_info (priv->transient_parent);
      if (info->icon_list)
        return icon_from_list (info->icon_list, size);
    }

  if (default_icon_list != NULL)
    return icon_from_list (default_icon_list, size);

  if (default_icon_name != NULL)
    return icon_from_name (default_icon_name, size);

  return NULL;
}

static void
ctk_window_unrealize_icon (CtkWindow *window)
{
  CtkWindowIconInfo *info;

  info = get_icon_info (window);

  if (info == NULL)
    return;
  
  /* We don't clear the properties on the window, just figure the
   * window is going away.
   */

  info->realized = FALSE;

}

/**
 * ctk_window_set_icon_list:
 * @window: a #CtkWindow
 * @list: (element-type GdkPixbuf): list of #GdkPixbuf
 *
 * Sets up the icon representing a #CtkWindow. The icon is used when
 * the window is minimized (also known as iconified).  Some window
 * managers or desktop environments may also place it in the window
 * frame, or display it in other contexts. On others, the icon is not
 * used at all, so your mileage may vary.
 *
 * ctk_window_set_icon_list() allows you to pass in the same icon in
 * several hand-drawn sizes. The list should contain the natural sizes
 * your icon is available in; that is, don’t scale the image before
 * passing it to CTK+. Scaling is postponed until the last minute,
 * when the desired final size is known, to allow best quality.
 *
 * By passing several sizes, you may improve the final image quality
 * of the icon, by reducing or eliminating automatic image scaling.
 *
 * Recommended sizes to provide: 16x16, 32x32, 48x48 at minimum, and
 * larger images (64x64, 128x128) if you have them.
 *
 * See also ctk_window_set_default_icon_list() to set the icon
 * for all windows in your application in one go.
 *
 * Note that transient windows (those who have been set transient for another
 * window using ctk_window_set_transient_for()) will inherit their
 * icon from their transient parent. So there’s no need to explicitly
 * set the icon on transient windows.
 **/
void
ctk_window_set_icon_list (CtkWindow  *window,
                          GList      *list)
{
  CtkWindowIconInfo *info;

  g_return_if_fail (CTK_IS_WINDOW (window));

  info = ensure_icon_info (window);

  if (info->icon_list == list) /* check for NULL mostly */
    return;

  g_list_foreach (list,
                  (GFunc) g_object_ref, NULL);

  g_list_free_full (info->icon_list, g_object_unref);

  info->icon_list = g_list_copy (list);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_ICON]);
  
  ctk_window_unrealize_icon (window);
  
  if (_ctk_widget_get_realized (CTK_WIDGET (window)))
    ctk_window_realize_icon (window);

  /* We could try to update our transient children, but I don't think
   * it's really worth it. If we did it, the best way would probably
   * be to have children connect to notify::icon-list
   */
}

/**
 * ctk_window_get_icon_list:
 * @window: a #CtkWindow
 * 
 * Retrieves the list of icons set by ctk_window_set_icon_list().
 * The list is copied, but the reference count on each
 * member won’t be incremented.
 *
 * Returns: (element-type GdkPixbuf) (transfer container): copy of window’s icon list
 **/
GList*
ctk_window_get_icon_list (CtkWindow  *window)
{
  CtkWindowIconInfo *info;
  
  g_return_val_if_fail (CTK_IS_WINDOW (window), NULL);

  info = get_icon_info (window);

  if (info)
    return g_list_copy (info->icon_list);
  else
    return NULL;  
}

/**
 * ctk_window_set_icon:
 * @window: a #CtkWindow
 * @icon: (allow-none): icon image, or %NULL
 *
 * Sets up the icon representing a #CtkWindow. This icon is used when
 * the window is minimized (also known as iconified).  Some window
 * managers or desktop environments may also place it in the window
 * frame, or display it in other contexts. On others, the icon is not
 * used at all, so your mileage may vary.
 *
 * The icon should be provided in whatever size it was naturally
 * drawn; that is, don’t scale the image before passing it to
 * CTK+. Scaling is postponed until the last minute, when the desired
 * final size is known, to allow best quality.
 *
 * If you have your icon hand-drawn in multiple sizes, use
 * ctk_window_set_icon_list(). Then the best size will be used.
 *
 * This function is equivalent to calling ctk_window_set_icon_list()
 * with a 1-element list.
 *
 * See also ctk_window_set_default_icon_list() to set the icon
 * for all windows in your application in one go.
 **/
void
ctk_window_set_icon (CtkWindow  *window,
                     GdkPixbuf  *icon)
{
  GList *list;
  
  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (icon == NULL || GDK_IS_PIXBUF (icon));

  list = NULL;

  if (icon)
    list = g_list_append (list, icon);
  
  ctk_window_set_icon_list (window, list);
  g_list_free (list);  
}


static void 
update_themed_icon (CtkWindow *window)
{
  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_ICON_NAME]);
  
  ctk_window_unrealize_icon (window);
  
  if (_ctk_widget_get_realized (CTK_WIDGET (window)))
    ctk_window_realize_icon (window);  
}

/**
 * ctk_window_set_icon_name:
 * @window: a #CtkWindow
 * @name: (allow-none): the name of the themed icon
 *
 * Sets the icon for the window from a named themed icon.
 * See the docs for #CtkIconTheme for more details.
 * On some platforms, the window icon is not used at all.
 *
 * Note that this has nothing to do with the WM_ICON_NAME 
 * property which is mentioned in the ICCCM.
 *
 * Since: 2.6
 */
void 
ctk_window_set_icon_name (CtkWindow   *window,
			  const gchar *name)
{
  CtkWindowIconInfo *info;
  gchar *tmp;
  
  g_return_if_fail (CTK_IS_WINDOW (window));

  info = ensure_icon_info (window);

  if (g_strcmp0 (info->icon_name, name) == 0)
    return;

  tmp = info->icon_name;
  info->icon_name = g_strdup (name);
  g_free (tmp);

  g_list_free_full (info->icon_list, g_object_unref);
  info->icon_list = NULL;
  
  update_themed_icon (window);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_ICON_NAME]);
}

/**
 * ctk_window_get_icon_name:
 * @window: a #CtkWindow
 *
 * Returns the name of the themed icon for the window,
 * see ctk_window_set_icon_name().
 *
 * Returns: (nullable): the icon name or %NULL if the window has
 * no themed icon
 *
 * Since: 2.6
 */
const gchar *
ctk_window_get_icon_name (CtkWindow *window)
{
  CtkWindowIconInfo *info;

  g_return_val_if_fail (CTK_IS_WINDOW (window), NULL);

  info = ensure_icon_info (window);

  return info->icon_name;
}

/**
 * ctk_window_get_icon:
 * @window: a #CtkWindow
 * 
 * Gets the value set by ctk_window_set_icon() (or if you've
 * called ctk_window_set_icon_list(), gets the first icon in
 * the icon list).
 *
 * Returns: (transfer none) (nullable): icon for window or %NULL if none
 **/
GdkPixbuf*
ctk_window_get_icon (CtkWindow  *window)
{
  CtkWindowIconInfo *info;

  g_return_val_if_fail (CTK_IS_WINDOW (window), NULL);

  info = get_icon_info (window);
  if (info && info->icon_list)
    return GDK_PIXBUF (info->icon_list->data);
  else
    return NULL;
}

/* Load pixbuf, printing warning on failure if error == NULL
 */
static GdkPixbuf *
load_pixbuf_verbosely (const char *filename,
		       GError    **err)
{
  GError *local_err = NULL;
  GdkPixbuf *pixbuf;

  pixbuf = gdk_pixbuf_new_from_file (filename, &local_err);

  if (!pixbuf)
    {
      if (err)
	*err = local_err;
      else
	{
	  g_warning ("Error loading icon from file '%s':\n\t%s",
		     filename, local_err->message);
	  g_error_free (local_err);
	}
    }

  return pixbuf;
}

/**
 * ctk_window_set_icon_from_file:
 * @window: a #CtkWindow
 * @filename: (type filename): location of icon file
 * @err: (allow-none): location to store error, or %NULL.
 *
 * Sets the icon for @window.
 * Warns on failure if @err is %NULL.
 *
 * This function is equivalent to calling ctk_window_set_icon()
 * with a pixbuf created by loading the image from @filename.
 *
 * Returns: %TRUE if setting the icon succeeded.
 *
 * Since: 2.2
 **/
gboolean
ctk_window_set_icon_from_file (CtkWindow   *window,
			       const gchar *filename,
			       GError     **err)
{
  GdkPixbuf *pixbuf = load_pixbuf_verbosely (filename, err);

  if (pixbuf)
    {
      ctk_window_set_icon (window, pixbuf);
      g_object_unref (pixbuf);
      
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * ctk_window_set_default_icon_list:
 * @list: (element-type GdkPixbuf) (transfer container): a list of #GdkPixbuf
 *
 * Sets an icon list to be used as fallback for windows that haven't
 * had ctk_window_set_icon_list() called on them to set up a
 * window-specific icon list. This function allows you to set up the
 * icon for all windows in your app at once.
 *
 * See ctk_window_set_icon_list() for more details.
 * 
 **/
void
ctk_window_set_default_icon_list (GList *list)
{
  GList *toplevels;
  GList *tmp_list;
  if (list == default_icon_list)
    return;

  /* Update serial so we don't used cached pixmaps/masks
   */
  default_icon_serial++;
  
  g_list_foreach (list,
                  (GFunc) g_object_ref, NULL);

  g_list_free_full (default_icon_list, g_object_unref);

  default_icon_list = g_list_copy (list);
  
  /* Update all toplevels */
  toplevels = ctk_window_list_toplevels ();
  tmp_list = toplevels;
  while (tmp_list != NULL)
    {
      CtkWindowIconInfo *info;
      CtkWindow *w = tmp_list->data;
      
      info = get_icon_info (w);
      if (info && info->using_default_icon)
        {
          ctk_window_unrealize_icon (w);
          if (_ctk_widget_get_realized (CTK_WIDGET (w)))
            ctk_window_realize_icon (w);
        }

      tmp_list = tmp_list->next;
    }
  g_list_free (toplevels);
}

/**
 * ctk_window_set_default_icon:
 * @icon: the icon
 *
 * Sets an icon to be used as fallback for windows that haven't
 * had ctk_window_set_icon() called on them from a pixbuf.
 *
 * Since: 2.4
 **/
void
ctk_window_set_default_icon (GdkPixbuf *icon)
{
  GList *list;
  
  g_return_if_fail (GDK_IS_PIXBUF (icon));

  list = g_list_prepend (NULL, icon);
  ctk_window_set_default_icon_list (list);
  g_list_free (list);
}

/**
 * ctk_window_set_default_icon_name:
 * @name: the name of the themed icon
 *
 * Sets an icon to be used as fallback for windows that haven't
 * had ctk_window_set_icon_list() called on them from a named
 * themed icon, see ctk_window_set_icon_name().
 *
 * Since: 2.6
 **/
void
ctk_window_set_default_icon_name (const gchar *name)
{
  GList *tmp_list;
  GList *toplevels;

  /* Update serial so we don't used cached pixmaps/masks
   */
  default_icon_serial++;

  g_free (default_icon_name);
  default_icon_name = g_strdup (name);

  g_list_free_full (default_icon_list, g_object_unref);
  default_icon_list = NULL;
  
  /* Update all toplevels */
  toplevels = ctk_window_list_toplevels ();
  tmp_list = toplevels;
  while (tmp_list != NULL)
    {
      CtkWindowIconInfo *info;
      CtkWindow *w = tmp_list->data;
      
      info = get_icon_info (w);
      if (info && info->using_default_icon && info->using_themed_icon)
        {
          ctk_window_unrealize_icon (w);
          if (_ctk_widget_get_realized (CTK_WIDGET (w)))
            ctk_window_realize_icon (w);
        }

      tmp_list = tmp_list->next;
    }
  g_list_free (toplevels);
}

/**
 * ctk_window_get_default_icon_name:
 *
 * Returns the fallback icon name for windows that has been set
 * with ctk_window_set_default_icon_name(). The returned
 * string is owned by CTK+ and should not be modified. It
 * is only valid until the next call to
 * ctk_window_set_default_icon_name().
 *
 * Returns: the fallback icon name for windows
 *
 * Since: 2.16
 */
const gchar *
ctk_window_get_default_icon_name (void)
{
  return default_icon_name;
}

/**
 * ctk_window_set_default_icon_from_file:
 * @filename: (type filename): location of icon file
 * @err: (allow-none): location to store error, or %NULL.
 *
 * Sets an icon to be used as fallback for windows that haven't
 * had ctk_window_set_icon_list() called on them from a file
 * on disk. Warns on failure if @err is %NULL.
 *
 * Returns: %TRUE if setting the icon succeeded.
 *
 * Since: 2.2
 **/
gboolean
ctk_window_set_default_icon_from_file (const gchar *filename,
				       GError     **err)
{
  GdkPixbuf *pixbuf = load_pixbuf_verbosely (filename, err);

  if (pixbuf)
    {
      ctk_window_set_default_icon (pixbuf);
      g_object_unref (pixbuf);
      
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * ctk_window_get_default_icon_list:
 * 
 * Gets the value set by ctk_window_set_default_icon_list().
 * The list is a copy and should be freed with g_list_free(),
 * but the pixbufs in the list have not had their reference count
 * incremented.
 * 
 * Returns: (element-type GdkPixbuf) (transfer container): copy of default icon list 
 **/
GList*
ctk_window_get_default_icon_list (void)
{
  return g_list_copy (default_icon_list);
}

#define INCLUDE_CSD_SIZE 1
#define EXCLUDE_CSD_SIZE -1

static void
ctk_window_update_csd_size (CtkWindow *window,
                            gint      *width,
                            gint      *height,
                            gint       apply)
{
  CtkWindowPrivate *priv = window->priv;
  CtkBorder window_border = { 0 };
  gint w, h;

  if (priv->type != CTK_WINDOW_TOPLEVEL)
    return;

  if (!priv->decorated ||
      priv->fullscreen)
    return;

  get_shadow_width (window, &window_border);
  w = *width + apply * (window_border.left + window_border.right);
  h = *height + apply * (window_border.top + window_border.bottom);

  if (priv->title_box != NULL &&
      ctk_widget_get_visible (priv->title_box) &&
      ctk_widget_get_child_visible (priv->title_box))
    {
      gint minimum_height;
      gint natural_height;

      ctk_widget_get_preferred_height (priv->title_box, &minimum_height, &natural_height);
      h += apply * natural_height;
    }

  /* Make sure the size remains acceptable */
  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;

  /* Only update given size if not negative */
  if (*width > -1)
    *width = w;
  if (*height > -1)
    *height = h;
}

static void
ctk_window_set_default_size_internal (CtkWindow    *window,
                                      gboolean      change_width,
                                      gint          width,
                                      gboolean      change_height,
                                      gint          height,
				      gboolean      is_geometry)
{
  CtkWindowGeometryInfo *info;

  g_return_if_fail (change_width == FALSE || width >= -1);
  g_return_if_fail (change_height == FALSE || height >= -1);

  info = ctk_window_get_geometry_info (window, TRUE);

  g_object_freeze_notify (G_OBJECT (window));

  info->default_is_geometry = is_geometry != FALSE;

  if (change_width)
    {
      if (width == 0)
        width = 1;

      if (width < 0)
        width = -1;

      if (info->default_width != width)
        {
          info->default_width = width;
          g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_DEFAULT_WIDTH]);
        }
    }

  if (change_height)
    {
      if (height == 0)
        height = 1;

      if (height < 0)
        height = -1;

      if (info->default_height != height)
        {
          info->default_height = height;
          g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_DEFAULT_HEIGHT]);
        }
    }
  
  g_object_thaw_notify (G_OBJECT (window));
  
  ctk_widget_queue_resize_no_redraw (CTK_WIDGET (window));
}

/**
 * ctk_window_set_default_size:
 * @window: a #CtkWindow
 * @width: width in pixels, or -1 to unset the default width
 * @height: height in pixels, or -1 to unset the default height
 *
 * Sets the default size of a window. If the window’s “natural” size
 * (its size request) is larger than the default, the default will be
 * ignored. More generally, if the default size does not obey the
 * geometry hints for the window (ctk_window_set_geometry_hints() can
 * be used to set these explicitly), the default size will be clamped
 * to the nearest permitted size.
 *
 * Unlike ctk_widget_set_size_request(), which sets a size request for
 * a widget and thus would keep users from shrinking the window, this
 * function only sets the initial size, just as if the user had
 * resized the window themselves. Users can still shrink the window
 * again as they normally would. Setting a default size of -1 means to
 * use the “natural” default size (the size request of the window).
 *
 * For more control over a window’s initial size and how resizing works,
 * investigate ctk_window_set_geometry_hints().
 *
 * For some uses, ctk_window_resize() is a more appropriate function.
 * ctk_window_resize() changes the current size of the window, rather
 * than the size to be used on initial display. ctk_window_resize() always
 * affects the window itself, not the geometry widget.
 *
 * The default size of a window only affects the first time a window is
 * shown; if a window is hidden and re-shown, it will remember the size
 * it had prior to hiding, rather than using the default size.
 *
 * Windows can’t actually be 0x0 in size, they must be at least 1x1, but
 * passing 0 for @width and @height is OK, resulting in a 1x1 default size.
 *
 * If you use this function to reestablish a previously saved window size,
 * note that the appropriate size to save is the one returned by
 * ctk_window_get_size(). Using the window allocation directly will not
 * work in all circumstances and can lead to growing or shrinking windows.
 */
void       
ctk_window_set_default_size (CtkWindow   *window,
			     gint         width,
			     gint         height)
{
  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (width >= -1);
  g_return_if_fail (height >= -1);

  ctk_window_set_default_size_internal (window, TRUE, width, TRUE, height, FALSE);
}

/**
 * ctk_window_set_default_geometry:
 * @window: a #CtkWindow
 * @width: width in resize increments, or -1 to unset the default width
 * @height: height in resize increments, or -1 to unset the default height
 *
 * Like ctk_window_set_default_size(), but @width and @height are interpreted
 * in terms of the base size and increment set with
 * ctk_window_set_geometry_hints.
 *
 * Since: 3.0
 *
 * Deprecated: 3.20: This function does nothing. If you want to set a default
 *     size, use ctk_window_set_default_size() instead.
 */
void
ctk_window_set_default_geometry (CtkWindow *window,
				 gint       width,
				 gint       height)
{
  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (width >= -1);
  g_return_if_fail (height >= -1);

  ctk_window_set_default_size_internal (window, TRUE, width, TRUE, height, TRUE);
}

/**
 * ctk_window_get_default_size:
 * @window: a #CtkWindow
 * @width: (out) (allow-none): location to store the default width, or %NULL
 * @height: (out) (allow-none): location to store the default height, or %NULL
 *
 * Gets the default size of the window. A value of -1 for the width or
 * height indicates that a default size has not been explicitly set
 * for that dimension, so the “natural” size of the window will be
 * used.
 * 
 **/
void
ctk_window_get_default_size (CtkWindow *window,
			     gint      *width,
			     gint      *height)
{
  CtkWindowGeometryInfo *info;

  g_return_if_fail (CTK_IS_WINDOW (window));

  info = ctk_window_get_geometry_info (window, FALSE);

  if (width)
    *width = info ? info->default_width : -1;

  if (height)
    *height = info ? info->default_height : -1;
}

/**
 * ctk_window_resize:
 * @window: a #CtkWindow
 * @width: width in pixels to resize the window to
 * @height: height in pixels to resize the window to
 *
 * Resizes the window as if the user had done so, obeying geometry
 * constraints. The default geometry constraint is that windows may
 * not be smaller than their size request; to override this
 * constraint, call ctk_widget_set_size_request() to set the window's
 * request to a smaller value.
 *
 * If ctk_window_resize() is called before showing a window for the
 * first time, it overrides any default size set with
 * ctk_window_set_default_size().
 *
 * Windows may not be resized smaller than 1 by 1 pixels.
 * 
 * When using client side decorations, CTK+ will do its best to adjust
 * the given size so that the resulting window size matches the
 * requested size without the title bar, borders and shadows added for
 * the client side decorations, but there is no guarantee that the
 * result will be totally accurate because these widgets added for
 * client side decorations depend on the theme and may not be realized
 * or visible at the time ctk_window_resize() is issued.
 *
 * If the CtkWindow has a titlebar widget (see ctk_window_set_titlebar()), then
 * typically, ctk_window_resize() will compensate for the height of the titlebar
 * widget only if the height is known when the resulting CtkWindow configuration
 * is issued.
 * For example, if new widgets are added after the CtkWindow configuration
 * and cause the titlebar widget to grow in height, this will result in a
 * window content smaller that specified by ctk_window_resize() and not
 * a larger window.
 *
 **/
void
ctk_window_resize (CtkWindow *window,
                   gint       width,
                   gint       height)
{
  CtkWindowGeometryInfo *info;
  
  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (width > 0);
  g_return_if_fail (height > 0);

  info = ctk_window_get_geometry_info (window, TRUE);

  info->resize_width = width;
  info->resize_height = height;

  ctk_widget_queue_resize_no_redraw (CTK_WIDGET (window));
}

/**
 * ctk_window_resize_to_geometry:
 * @window: a #CtkWindow
 * @width: width in resize increments to resize the window to
 * @height: height in resize increments to resize the window to
 *
 * Like ctk_window_resize(), but @width and @height are interpreted
 * in terms of the base size and increment set with
 * ctk_window_set_geometry_hints.
 *
 * Since: 3.0
 *
 * Deprecated: 3.20: This function does nothing. Use 
 *    ctk_window_resize() and compute the geometry yourself.
 */
void
ctk_window_resize_to_geometry (CtkWindow *window,
			       gint       width,
			       gint       height)
{
  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (width > 0);
  g_return_if_fail (height > 0);
}

/**
 * ctk_window_get_size:
 * @window: a #CtkWindow
 * @width: (out) (nullable): return location for width, or %NULL
 * @height: (out) (nullable): return location for height, or %NULL
 *
 * Obtains the current size of @window.
 *
 * If @window is not visible on screen, this function return the size CTK+
 * will suggest to the [window manager][ctk-X11-arch] for the initial window
 * size (but this is not reliably the same as the size the window manager
 * will actually select). See: ctk_window_set_default_size().
 *
 * Depending on the windowing system and the window manager constraints,
 * the size returned by this function may not match the size set using
 * ctk_window_resize(); additionally, since ctk_window_resize() may be
 * implemented as an asynchronous operation, CTK+ cannot guarantee in any
 * way that this code:
 *
 * |[<!-- language="C" -->
 *   // width and height are set elsewhere
 *   ctk_window_resize (window, width, height);
 *
 *   int new_width, new_height;
 *   ctk_window_get_size (window, &new_width, &new_height);
 * ]|
 *
 * will result in `new_width` and `new_height` matching `width` and
 * `height`, respectively.
 *
 * This function will return the logical size of the #CtkWindow,
 * excluding the widgets used in client side decorations; there is,
 * however, no guarantee that the result will be completely accurate
 * because client side decoration may include widgets that depend on
 * the user preferences and that may not be visibile at the time you
 * call this function.
 *
 * The dimensions returned by this function are suitable for being
 * stored across sessions; use ctk_window_set_default_size() to
 * restore them when before showing the window.
 *
 * To avoid potential race conditions, you should only call this
 * function in response to a size change notification, for instance
 * inside a handler for the #CtkWidget::size-allocate signal, or
 * inside a handler for the #CtkWidget::configure-event signal:
 *
 * |[<!-- language="C" -->
 * static void
 * on_size_allocate (CtkWidget *widget, CtkAllocation *allocation)
 * {
 *   int new_width, new_height;
 *
 *   ctk_window_get_size (CTK_WINDOW (widget), &new_width, &new_height);
 *
 *   ...
 * }
 * ]|
 *
 * Note that, if you connect to the #CtkWidget::size-allocate signal,
 * you should not use the dimensions of the #CtkAllocation passed to
 * the signal handler, as the allocation may contain client side
 * decorations added by CTK+, depending on the windowing system in
 * use.
 *
 * If you are getting a window size in order to position the window
 * on the screen, you should, instead, simply set the window’s semantic
 * type with ctk_window_set_type_hint(), which allows the window manager
 * to e.g. center dialogs. Also, if you set the transient parent of
 * dialogs with ctk_window_set_transient_for() window managers will
 * often center the dialog over its parent window. It's much preferred
 * to let the window manager handle these cases rather than doing it
 * yourself, because all apps will behave consistently and according to
 * user or system preferences, if the window manager handles it. Also,
 * the window manager can take into account the size of the window
 * decorations and border that it may add, and of which CTK+ has no
 * knowledge. Additionally, positioning windows in global screen coordinates
 * may not be allowed by the windowing system. For more information,
 * see: ctk_window_set_position().
 */
void
ctk_window_get_size (CtkWindow *window,
                     gint      *width,
                     gint      *height)
{
  gint w, h;
  
  g_return_if_fail (CTK_IS_WINDOW (window));

  if (width == NULL && height == NULL)
    return;

  if (_ctk_widget_get_mapped (CTK_WIDGET (window)))
    {
      w = cdk_window_get_width (_ctk_widget_get_window (CTK_WIDGET (window)));
      h = cdk_window_get_height (_ctk_widget_get_window (CTK_WIDGET (window)));
    }
  else
    {
      CdkRectangle configure_request;

      ctk_window_compute_configure_request (window,
                                            &configure_request,
                                            NULL, NULL);

      w = configure_request.width;
      h = configure_request.height;
    }

  ctk_window_update_csd_size (window, &w, &h, EXCLUDE_CSD_SIZE);

  if (width)
    *width = w;
  if (height)
    *height = h;
}

static void
ctk_window_translate_csd_pos (CtkWindow *window,
                              gint      *root_x,
                              gint      *root_y,
                              gint       apply)
{
  CtkWindowPrivate *priv = window->priv;

  if (priv->type != CTK_WINDOW_TOPLEVEL)
    return;

  if (priv->decorated &&
      !priv->fullscreen)
    {
      CtkBorder window_border = { 0 };
      gint title_height = 0;
      gint dx;
      gint dy;

      get_shadow_width (window, &window_border);
      if (priv->title_box != NULL &&
          ctk_widget_get_visible (priv->title_box) &&
          ctk_widget_get_child_visible (priv->title_box))
        {
          gint minimum_height;

          ctk_widget_get_preferred_height (priv->title_box, &minimum_height, &title_height);
        }

      switch (priv->gravity)
        {
        case CDK_GRAVITY_NORTH:
        case CDK_GRAVITY_CENTER:
        case CDK_GRAVITY_SOUTH:
          dx = (window_border.left + window_border.right) / 2;
          break;

        case CDK_GRAVITY_NORTH_WEST:
        case CDK_GRAVITY_WEST:
        case CDK_GRAVITY_SOUTH_WEST:
        case CDK_GRAVITY_SOUTH_EAST:
        case CDK_GRAVITY_EAST:
        case CDK_GRAVITY_NORTH_EAST:
          dx = window_border.left;
          break;

        default:
          dx = 0;
          break;
        }

      switch (priv->gravity)
        {
        case CDK_GRAVITY_WEST:
        case CDK_GRAVITY_CENTER:
        case CDK_GRAVITY_EAST:
          dy = (window_border.top + title_height + window_border.bottom) / 2;
          break;

        case CDK_GRAVITY_NORTH_WEST:
        case CDK_GRAVITY_NORTH:
        case CDK_GRAVITY_NORTH_EAST:
          dy = window_border.top;
          break;

        case CDK_GRAVITY_SOUTH_WEST:
        case CDK_GRAVITY_SOUTH:
        case CDK_GRAVITY_SOUTH_EAST:
          dy = window_border.top + title_height;
          break;

        default:
          dy = 0;
          break;
        }

      if (root_x)
        *root_x = *root_x + (dx * apply);
      if (root_y)
        *root_y = *root_y + (dy * apply);
    }
}

/**
 * ctk_window_move:
 * @window: a #CtkWindow
 * @x: X coordinate to move window to
 * @y: Y coordinate to move window to
 *
 * Asks the [window manager][ctk-X11-arch] to move
 * @window to the given position.  Window managers are free to ignore
 * this; most window managers ignore requests for initial window
 * positions (instead using a user-defined placement algorithm) and
 * honor requests after the window has already been shown.
 *
 * Note: the position is the position of the gravity-determined
 * reference point for the window. The gravity determines two things:
 * first, the location of the reference point in root window
 * coordinates; and second, which point on the window is positioned at
 * the reference point.
 *
 * By default the gravity is #CDK_GRAVITY_NORTH_WEST, so the reference
 * point is simply the @x, @y supplied to ctk_window_move(). The
 * top-left corner of the window decorations (aka window frame or
 * border) will be placed at @x, @y.  Therefore, to position a window
 * at the top left of the screen, you want to use the default gravity
 * (which is #CDK_GRAVITY_NORTH_WEST) and move the window to 0,0.
 *
 * To position a window at the bottom right corner of the screen, you
 * would set #CDK_GRAVITY_SOUTH_EAST, which means that the reference
 * point is at @x + the window width and @y + the window height, and
 * the bottom-right corner of the window border will be placed at that
 * reference point. So, to place a window in the bottom right corner
 * you would first set gravity to south east, then write:
 * `ctk_window_move (window, cdk_screen_width () - window_width,
 * cdk_screen_height () - window_height)` (note that this
 * example does not take multi-head scenarios into account).
 *
 * The [Extended Window Manager Hints Specification](http://www.freedesktop.org/Standards/wm-spec)
 * has a nice table of gravities in the “implementation notes” section.
 *
 * The ctk_window_get_position() documentation may also be relevant.
 */
void
ctk_window_move (CtkWindow *window,
                 gint       x,
                 gint       y)
{
  CtkWindowGeometryInfo *info;
  CtkWidget *widget;

  g_return_if_fail (CTK_IS_WINDOW (window));

  widget = CTK_WIDGET (window);

  info = ctk_window_get_geometry_info (window, TRUE);  
  ctk_window_translate_csd_pos (window, &x, &y, EXCLUDE_CSD_SIZE);

  if (_ctk_widget_get_mapped (widget))
    {
      CtkAllocation allocation;

      _ctk_widget_get_allocation (widget, &allocation);

      /* we have now sent a request with this position
       * with currently-active constraints, so toggle flag.
       */
      info->position_constraints_changed = FALSE;

      /* we only constrain if mapped - if not mapped,
       * then ctk_window_compute_configure_request()
       * will apply the constraints later, and we
       * don't want to lose information about
       * what position the user set before then.
       * i.e. if you do a move() then turn off POS_CENTER
       * then show the window, your move() will work.
       */
      ctk_window_constrain_position (window,
                                     allocation.width, allocation.height,
                                     &x, &y);

      /* Note that this request doesn't go through our standard request
       * framework, e.g. doesn't increment configure_request_count,
       * doesn't set info->last, etc.; that's because
       * we don't save the info needed to arrive at this same request
       * again.
       *
       * To ctk_window_move_resize(), this will end up looking exactly
       * the same as the position being changed by the window
       * manager.
       */
      cdk_window_move (_ctk_widget_get_window (CTK_WIDGET (window)), x, y);
    }
  else
    {
      /* Save this position to apply on mapping */
      ctk_widget_queue_resize (widget);
      info->initial_x = x;
      info->initial_y = y;
      info->initial_pos_set = TRUE;
    }
}

/**
 * ctk_window_get_position:
 * @window: a #CtkWindow
 * @root_x: (out) (allow-none): return location for X coordinate of
 *     gravity-determined reference point, or %NULL
 * @root_y: (out) (allow-none): return location for Y coordinate of
 *     gravity-determined reference point, or %NULL
 *
 * This function returns the position you need to pass to
 * ctk_window_move() to keep @window in its current position.
 * This means that the meaning of the returned value varies with
 * window gravity. See ctk_window_move() for more details.
 *
 * The reliability of this function depends on the windowing system
 * currently in use. Some windowing systems, such as Wayland, do not
 * support a global coordinate system, and thus the position of the
 * window will always be (0, 0). Others, like X11, do not have a reliable
 * way to obtain the geometry of the decorations of a window if they are
 * provided by the window manager. Additionally, on X11, window manager
 * have been known to mismanage window gravity, which result in windows
 * moving even if you use the coordinates of the current position as
 * returned by this function.
 *
 * If you haven’t changed the window gravity, its gravity will be
 * #CDK_GRAVITY_NORTH_WEST. This means that ctk_window_get_position()
 * gets the position of the top-left corner of the window manager
 * frame for the window. ctk_window_move() sets the position of this
 * same top-left corner.
 *
 * If a window has gravity #CDK_GRAVITY_STATIC the window manager
 * frame is not relevant, and thus ctk_window_get_position() will
 * always produce accurate results. However you can’t use static
 * gravity to do things like place a window in a corner of the screen,
 * because static gravity ignores the window manager decorations.
 *
 * Ideally, this function should return appropriate values if the
 * window has client side decorations, assuming that the windowing
 * system supports global coordinates.
 *
 * In practice, saving the window position should not be left to
 * applications, as they lack enough knowledge of the windowing
 * system and the window manager state to effectively do so. The
 * appropriate way to implement saving the window position is to
 * use a platform-specific protocol, wherever that is available.
 */
void
ctk_window_get_position (CtkWindow *window,
                         gint      *root_x,
                         gint      *root_y)
{
  CtkWindowPrivate *priv;
  CtkWidget *widget;
  CdkWindow *cdk_window;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;
  widget = CTK_WIDGET (window);
  cdk_window = _ctk_widget_get_window (widget);

  if (priv->gravity == CDK_GRAVITY_STATIC)
    {
      if (_ctk_widget_get_mapped (widget))
        {
          /* This does a server round-trip, which is sort of wrong;
           * but a server round-trip is inevitable for
           * cdk_window_get_frame_extents() in the usual
           * NorthWestGravity case below, so not sure what else to
           * do. We should likely be consistent about whether we get
           * the client-side info or the server-side info.
           */
          cdk_window_get_origin (cdk_window, root_x, root_y);
        }
      else
        {
          CdkRectangle configure_request;
          
          ctk_window_compute_configure_request (window,
                                                &configure_request,
                                                NULL, NULL);
          
          *root_x = configure_request.x;
          *root_y = configure_request.y;
        }
      ctk_window_translate_csd_pos (window, root_x, root_y, INCLUDE_CSD_SIZE);
    }
  else
    {
      CdkRectangle frame_extents;
      
      gint x, y;
      gint w, h;
      
      if (_ctk_widget_get_mapped (widget))
        {
          cdk_window_get_frame_extents (cdk_window, &frame_extents);
          x = frame_extents.x;
          y = frame_extents.y;
          ctk_window_get_size (window, &w, &h);
          /* ctk_window_get_size() will have already taken into account
           * the padding added by the CSD shadow and title bar, so we need
           * to revert it here, otherwise we'll end up counting it twice...
           */
          ctk_window_update_csd_size (window, &w, &h, INCLUDE_CSD_SIZE);
        }
      else
        {
          /* We just say the frame has 0 size on all sides.
           * Not sure what else to do.
           */
          ctk_window_compute_configure_request (window,
                                                &frame_extents,
                                                NULL, NULL);
          x = frame_extents.x;
          y = frame_extents.y;
          w = frame_extents.width;
          h = frame_extents.height;
        }
      
      ctk_window_translate_csd_pos (window, &x, &y, INCLUDE_CSD_SIZE);
      switch (priv->gravity)
        {
        case CDK_GRAVITY_NORTH:
        case CDK_GRAVITY_CENTER:
        case CDK_GRAVITY_SOUTH:
          /* Find center of frame. */
          x += frame_extents.width / 2;
          /* Center client window on that point. */
          x -= w / 2;
          break;

        case CDK_GRAVITY_SOUTH_EAST:
        case CDK_GRAVITY_EAST:
        case CDK_GRAVITY_NORTH_EAST:
          /* Find right edge of frame */
          x += frame_extents.width;
          /* Align left edge of client at that point. */
          x -= w;
          break;
        default:
          break;
        }

      switch (priv->gravity)
        {
        case CDK_GRAVITY_WEST:
        case CDK_GRAVITY_CENTER:
        case CDK_GRAVITY_EAST:
          /* Find center of frame. */
          y += frame_extents.height / 2;
          /* Center client window there. */
          y -= h / 2;
          break;
        case CDK_GRAVITY_SOUTH_WEST:
        case CDK_GRAVITY_SOUTH:
        case CDK_GRAVITY_SOUTH_EAST:
          /* Find south edge of frame */
          y += frame_extents.height;
          /* Place bottom edge of client there */
          y -= h;
          break;
        default:
          break;
        }
      
      if (root_x)
        *root_x = x;
      if (root_y)
        *root_y = y;
    }
}

/**
 * ctk_window_reshow_with_initial_size:
 * @window: a #CtkWindow
 * 
 * Hides @window, then reshows it, resetting the
 * default size and position of the window. Used
 * by GUI builders only.
 * 
 * Deprecated: 3.10: GUI builders can call ctk_widget_hide(),
 *   ctk_widget_unrealize() and then ctk_widget_show() on @window
 *   themselves, if they still need this functionality.
 **/
void
ctk_window_reshow_with_initial_size (CtkWindow *window)
{
  CtkWidget *widget;
  
  g_return_if_fail (CTK_IS_WINDOW (window));

  widget = CTK_WIDGET (window);
  
  ctk_widget_hide (widget);
  ctk_widget_unrealize (widget);
  ctk_widget_show (widget);
}

static void
ctk_window_destroy (CtkWidget *widget)
{
  CtkWindow *window = CTK_WINDOW (widget);
  CtkWindowPrivate *priv = window->priv;

  ctk_window_release_application (window);

  toplevel_list = g_slist_remove (toplevel_list, window);
  ctk_window_update_debugging ();

  if (priv->transient_parent)
    ctk_window_set_transient_for (window, NULL);

  remove_attach_widget (window);

  /* frees the icons */
  ctk_window_set_icon_list (window, NULL);

  if (priv->has_user_ref_count)
    {
      priv->has_user_ref_count = FALSE;
      g_object_unref (window);
    }

  if (priv->group)
    ctk_window_group_remove_window (priv->group, window);

   ctk_window_free_key_hash (window);

  CTK_WIDGET_CLASS (ctk_window_parent_class)->destroy (widget);
}

static void
ctk_window_finalize (GObject *object)
{
  CtkWindow *window = CTK_WINDOW (object);
  CtkWindowPrivate *priv = window->priv;
  CtkMnemonicHash *mnemonic_hash;

  g_free (priv->title);
  g_free (priv->wmclass_name);
  g_free (priv->wmclass_class);
  g_free (priv->wm_role);
  ctk_window_release_application (window);

  mnemonic_hash = ctk_window_get_mnemonic_hash (window, FALSE);
  if (mnemonic_hash)
    _ctk_mnemonic_hash_free (mnemonic_hash);

  if (priv->geometry_info)
    {
      g_free (priv->geometry_info);
    }

  if (priv->keys_changed_handler)
    {
      g_source_remove (priv->keys_changed_handler);
      priv->keys_changed_handler = 0;
    }

  if (priv->delete_event_handler)
    {
      g_source_remove (priv->delete_event_handler);
      priv->delete_event_handler = 0;
    }

  if (priv->screen)
    {
      g_signal_handlers_disconnect_by_func (priv->screen,
                                            ctk_window_on_composited_changed, window);
#ifdef CDK_WINDOWING_X11
      g_signal_handlers_disconnect_by_func (ctk_settings_get_for_screen (priv->screen),
                                            ctk_window_on_theme_variant_changed,
                                            window);
#endif
    }

  g_free (priv->startup_id);

  if (priv->mnemonics_display_timeout_id)
    {
      g_source_remove (priv->mnemonics_display_timeout_id);
      priv->mnemonics_display_timeout_id = 0;
    }

  if (priv->multipress_gesture)
    g_object_unref (priv->multipress_gesture);

  if (priv->drag_gesture)
    g_object_unref (priv->drag_gesture);

  G_OBJECT_CLASS (ctk_window_parent_class)->finalize (object);
}

/* copied from cdkwindow-x11.c */
static const gchar *
get_default_title (void)
{
  const gchar *title;

  title = g_get_application_name ();
  if (!title)
    title = g_get_prgname ();
  if (!title)
    title = "";

  return title;
}

static gboolean
update_csd_visibility (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;
  gboolean visible;

  if (priv->title_box == NULL)
    return FALSE;

  visible = priv->decorated &&
            !priv->fullscreen &&
            !(priv->titlebar == priv->title_box &&
              priv->maximized &&
              priv->hide_titlebar_when_maximized);
  ctk_widget_set_child_visible (priv->title_box, visible);

  return visible;
}

static void
update_window_buttons (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;

  if (!update_csd_visibility (window))
    return;

  if (CTK_IS_HEADER_BAR (priv->title_box))
    _ctk_header_bar_update_window_buttons (CTK_HEADER_BAR (priv->title_box));
}

static CtkWidget *
create_titlebar (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;
  CtkWidget *titlebar;
  CtkStyleContext *context;

  titlebar = ctk_header_bar_new ();
  g_object_set (titlebar,
                "title", priv->title ? priv->title : get_default_title (),
                "has-subtitle", FALSE,
                "show-close-button", TRUE,
                NULL);
  context = ctk_widget_get_style_context (titlebar);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_TITLEBAR);
  ctk_style_context_add_class (context, "default-decoration");

  return titlebar;
}

void
_ctk_window_request_csd (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;

  priv->csd_requested = TRUE;
}

static gboolean
ctk_window_should_use_csd (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;
  const gchar *csd_env;

  if (priv->csd_requested)
    return TRUE;

  if (!priv->decorated)
    return FALSE;

  if (priv->type == CTK_WINDOW_POPUP)
    return FALSE;

  csd_env = g_getenv ("CTK_CSD");

#ifdef CDK_WINDOWING_BROADWAY
  if (CDK_IS_BROADWAY_DISPLAY (ctk_widget_get_display (CTK_WIDGET (window))))
    return TRUE;
#endif

#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (ctk_widget_get_display (CTK_WIDGET (window))))
    {
      CdkDisplay *cdk_display = ctk_widget_get_display (CTK_WIDGET (window));
      return !cdk_wayland_display_prefers_ssd (cdk_display);
    }
#endif

#ifdef CDK_WINDOWING_WIN32
  if (g_strcmp0 (csd_env, "0") != 0 &&
      CDK_IS_WIN32_DISPLAY (ctk_widget_get_display (CTK_WIDGET (window))))
    return TRUE;
#endif

  return (g_strcmp0 (csd_env, "1") == 0);
}

static void
create_decoration (CtkWidget *widget)
{
  CtkWindow *window = CTK_WINDOW (widget);
  CtkWindowPrivate *priv = window->priv;

  priv->use_client_shadow = ctk_window_supports_client_shadow (window);
  if (!priv->use_client_shadow)
    return;

  ctk_window_enable_csd (window);

  if (priv->type == CTK_WINDOW_POPUP)
    return;

  if (priv->title_box == NULL)
    {
      priv->titlebar = create_titlebar (window);
      ctk_widget_set_parent (priv->titlebar, widget);
      ctk_widget_show_all (priv->titlebar);
      priv->title_box = priv->titlebar;
    }

  update_window_buttons (window);
}

static void
ctk_window_show (CtkWidget *widget)
{
  CtkWindow *window = CTK_WINDOW (widget);
  CtkWindowPrivate *priv = window->priv;
  CtkContainer *container = CTK_CONTAINER (window);
  gboolean is_plug;

  if (!_ctk_widget_is_toplevel (CTK_WIDGET (widget)))
    {
      CTK_WIDGET_CLASS (ctk_window_parent_class)->show (widget);
      return;
    }

  _ctk_widget_set_visible_flag (widget, TRUE);

  ctk_css_node_validate (ctk_widget_get_css_node (widget));

  ctk_widget_realize (widget);

  ctk_container_check_resize (container);

  ctk_widget_map (widget);

  /* Try to make sure that we have some focused widget
   */
#ifdef CDK_WINDOWING_X11
  is_plug = CDK_IS_X11_WINDOW (_ctk_widget_get_window (widget)) &&
    CTK_IS_PLUG (window);
#else
  is_plug = FALSE;
#endif
  if (!priv->focus_widget && !is_plug)
    {
      if (priv->initial_focus)
        ctk_window_set_focus (window, priv->initial_focus);
      else
        ctk_window_move_focus (widget, CTK_DIR_TAB_FORWARD);
    }
  
  if (priv->modal)
    ctk_grab_add (widget);
}

static void
ctk_window_hide (CtkWidget *widget)
{
  CtkWindow *window = CTK_WINDOW (widget);
  CtkWindowPrivate *priv = window->priv;

  if (!_ctk_widget_is_toplevel (CTK_WIDGET (widget)))
    {
      CTK_WIDGET_CLASS (ctk_window_parent_class)->hide (widget);
      return;
    }

  _ctk_widget_set_visible_flag (widget, FALSE);
  ctk_widget_unmap (widget);

  if (priv->modal)
    ctk_grab_remove (widget);
}

static void
popover_unmap (CtkWidget        *widget,
               CtkWindowPopover *popover)
{
  if (popover->unmap_id)
    {
      g_signal_handler_disconnect (widget, popover->unmap_id);
      popover->unmap_id = 0;
    }

  if (popover->window)
    {
      cdk_window_hide (popover->window);
      ctk_widget_unmap (popover->widget);
    }
}

static void
popover_map (CtkWidget        *widget,
             CtkWindowPopover *popover)
{
  if (popover->window && ctk_widget_get_visible (popover->widget))
    {
      cdk_window_show_unraised (popover->window);
      ctk_widget_map (popover->widget);
      popover->unmap_id = g_signal_connect (popover->widget, "unmap",
                                            G_CALLBACK (popover_unmap), popover);
    }
}

static void
ctk_window_map (CtkWidget *widget)
{
  CtkWidget *child;
  CtkWindow *window = CTK_WINDOW (widget);
  CtkWindowPrivate *priv = window->priv;
  CdkWindow *cdk_window;
  GList *link;
  CdkDisplay *display;

  if (!_ctk_widget_is_toplevel (widget))
    {
      CTK_WIDGET_CLASS (ctk_window_parent_class)->map (widget);
      return;
    }

  display = ctk_widget_get_display (widget);
  if (priv->initial_fullscreen_monitor > cdk_display_get_n_monitors (display))
    priv->initial_fullscreen_monitor = -1;

  ctk_widget_set_mapped (widget, TRUE);

  child = ctk_bin_get_child (&(window->bin));
  if (child != NULL && ctk_widget_get_visible (child))
    ctk_widget_map (child);

  if (priv->title_box != NULL &&
      ctk_widget_get_visible (priv->title_box) &&
      ctk_widget_get_child_visible (priv->title_box))
    ctk_widget_map (priv->title_box);

  cdk_window = _ctk_widget_get_window (widget);

  if (priv->maximize_initially)
    cdk_window_maximize (cdk_window);
  else
    cdk_window_unmaximize (cdk_window);

  if (priv->stick_initially)
    cdk_window_stick (cdk_window);
  else
    cdk_window_unstick (cdk_window);

  if (priv->iconify_initially)
    cdk_window_iconify (cdk_window);
  else
    cdk_window_deiconify (cdk_window);

  if (priv->fullscreen_initially)
    {
      if (priv->initial_fullscreen_monitor < 0)
        cdk_window_fullscreen (cdk_window);
      else
        cdk_window_fullscreen_on_monitor (cdk_window, 
                                          priv->initial_fullscreen_monitor);
    }
  else
    cdk_window_unfullscreen (cdk_window);

  cdk_window_set_keep_above (cdk_window, priv->above_initially);

  cdk_window_set_keep_below (cdk_window, priv->below_initially);

  if (priv->type == CTK_WINDOW_TOPLEVEL)
    {
      ctk_window_set_theme_variant (window);
      ctk_window_apply_hide_titlebar_when_maximized (window);
    }

  /* No longer use the default settings */
  priv->need_default_size = FALSE;
  priv->need_default_position = FALSE;

  cdk_window_show (cdk_window);

  if (!disable_startup_notification &&
      !CTK_IS_OFFSCREEN_WINDOW (window) &&
      priv->type != CTK_WINDOW_POPUP)
    {
      /* Do we have a custom startup-notification id? */
      if (priv->startup_id != NULL)
        {
          /* Make sure we have a "real" id */
          if (!startup_id_is_fake (priv->startup_id))
            cdk_notify_startup_complete_with_id (priv->startup_id);

          g_free (priv->startup_id);
          priv->startup_id = NULL;
        }
      else
        {
          cdk_notify_startup_complete ();
        }
    }

  /* if mnemonics visible is not already set
   * (as in the case of popup menus), then hide mnemonics initially
   */
  if (!priv->mnemonics_visible_set)
    ctk_window_set_mnemonics_visible (window, FALSE);

  /* inherit from transient parent, so that a dialog that is
   * opened via keynav shows focus initially
   */
  if (priv->transient_parent)
    ctk_window_set_focus_visible (window, ctk_window_get_focus_visible (priv->transient_parent));
  else
    ctk_window_set_focus_visible (window, FALSE);

  if (priv->application)
    ctk_application_handle_window_map (priv->application, window);

  link = priv->popovers;

  while (link)
    {
      CtkWindowPopover *popover = link->data;
      link = link->next;
      popover_map (popover->widget, popover);
    }
}

static gboolean
ctk_window_map_event (CtkWidget   *widget,
                      CdkEventAny *event)
{
  if (!_ctk_widget_get_mapped (widget))
    {
      /* we should be be unmapped, but are getting a MapEvent, this may happen
       * to toplevel XWindows if mapping was intercepted by a window manager
       * and an unmap request occoured while the MapRequestEvent was still
       * being handled. we work around this situaiton here by re-requesting
       * the window being unmapped. more details can be found in:
       *   http://bugzilla.gnome.org/show_bug.cgi?id=316180
       */
      cdk_window_hide (_ctk_widget_get_window (widget));
    }
  return FALSE;
}

static void
ctk_window_unmap (CtkWidget *widget)
{
  CtkWindow *window = CTK_WINDOW (widget);
  CtkWindowPrivate *priv = window->priv;
  CtkWidget *child;
  CtkWindowGeometryInfo *info;
  CdkWindow *cdk_window;
  CdkWindowState state;
  GList *link;

  if (!_ctk_widget_is_toplevel (CTK_WIDGET (widget)))
    {
      CTK_WIDGET_CLASS (ctk_window_parent_class)->unmap (widget);
      return;
    }

  link = priv->popovers;

  while (link)
    {
      CtkWindowPopover *popover = link->data;
      link = link->next;
      popover_unmap (popover->widget, popover);
    }

  cdk_window = _ctk_widget_get_window (widget);

  ctk_widget_set_mapped (widget, FALSE);
  cdk_window_withdraw (cdk_window);

  while (priv->configure_request_count > 0)
    {
      priv->configure_request_count--;
      CDK_PRIVATE_CALL (cdk_window_thaw_toplevel_updates) (_ctk_widget_get_window (widget));
    }
  priv->configure_notify_received = FALSE;

  /* on unmap, we reset the default positioning of the window,
   * so it's placed again, but we don't reset the default
   * size of the window, so it's remembered.
   */
  priv->need_default_position = TRUE;

  priv->fixate_size = FALSE;

  info = ctk_window_get_geometry_info (window, FALSE);
  if (info)
    {
      info->initial_pos_set = FALSE;
      info->position_constraints_changed = FALSE;
    }

  state = cdk_window_get_state (cdk_window);
  priv->iconify_initially = (state & CDK_WINDOW_STATE_ICONIFIED) != 0;
  priv->maximize_initially = (state & CDK_WINDOW_STATE_MAXIMIZED) != 0;
  priv->stick_initially = (state & CDK_WINDOW_STATE_STICKY) != 0;
  priv->above_initially = (state & CDK_WINDOW_STATE_ABOVE) != 0;
  priv->below_initially = (state & CDK_WINDOW_STATE_BELOW) != 0;

  if (priv->title_box != NULL)
    ctk_widget_unmap (priv->title_box);

  child = ctk_bin_get_child (&(window->bin));
  if (child != NULL)
    ctk_widget_unmap (child);
}

void
ctk_window_set_unlimited_guessed_size (CtkWindow *window,
                                       gboolean   x,
                                       gboolean   y)
{
  CtkWindowPrivate *priv = window->priv;

  priv->unlimited_guessed_size_x = x;
  priv->unlimited_guessed_size_y = y;
}

void
ctk_window_force_resize (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;

  priv->force_resize = TRUE;
}

void
ctk_window_fixate_size (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;

  priv->fixate_size = TRUE;
}

/* (Note: Replace "size" with "width" or "height". Also, the request
 * mode is honoured.)
 * For selecting the default window size, the following conditions
 * should hold (in order of importance):
 * - the size is not below the minimum size
 *   Windows cannot be resized below their minimum size, so we must
 *   ensure we don’t do that either.
 * - the size is not above the natural size
 *   It seems weird to allocate more than this in an initial guess.
 * - the size does not exceed that of a maximized window
 *   We want to see the whole window after all.
 *   (Note that this may not be possible to achieve due to imperfect
 *    information from the windowing system.)
 */

static void
ctk_window_guess_default_size (CtkWindow *window,
                               gint      *width,
                               gint      *height)
{
  CtkWidget *widget;
  CdkDisplay *display;
  CdkWindow *cdkwindow;
  CdkMonitor *monitor;
  CdkRectangle workarea;
  int minimum, natural;

  widget = CTK_WIDGET (window);
  display = ctk_widget_get_display (widget);
  cdkwindow = _ctk_widget_get_window (widget);

  if (window->priv->fixate_size)
    {
      g_assert (cdkwindow);
      ctk_window_get_remembered_size (window, width, height);
      return;
    }

  if (cdkwindow)
    monitor = cdk_display_get_monitor_at_window (display, cdkwindow);
  else
    monitor = cdk_display_get_monitor (display, 0);

  cdk_monitor_get_workarea (monitor, &workarea);

  if (window->priv->unlimited_guessed_size_x)
    *width = INT_MAX;
  else
    *width = workarea.width;

  if (window->priv->unlimited_guessed_size_y)
    *height = INT_MAX;
  else
    *height = workarea.height;

  if (ctk_widget_get_request_mode (widget) == CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT)
    {
      ctk_widget_get_preferred_height (widget, &minimum, &natural);
      *height = MAX (minimum, MIN (*height, natural));

      ctk_widget_get_preferred_width_for_height (widget, *height, &minimum, &natural);
      *width = MAX (minimum, MIN (*width, natural));
    }
  else /* CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH or CONSTANT_SIZE */
    {
      ctk_widget_get_preferred_width (widget, &minimum, &natural);
      *width = MAX (minimum, MIN (*width, natural));

      ctk_widget_get_preferred_height_for_width (widget, *width, &minimum, &natural);
      *height = MAX (minimum, MIN (*height, natural));
    }
}

static void
ctk_window_get_remembered_size (CtkWindow *window,
                                int       *width,
                                int       *height)
{
  CtkWindowGeometryInfo *info;
  CdkWindow *cdk_window;

  *width = 0;
  *height = 0;

  cdk_window = _ctk_widget_get_window (CTK_WIDGET (window));
  if (cdk_window)
    {
      *width = cdk_window_get_width (cdk_window);
      *height = cdk_window_get_height (cdk_window);
      return;
    }

  info = ctk_window_get_geometry_info (window, FALSE);
  if (info)
    {
      /* MAX() works even if the last request is unset with -1 */
      *width = MAX (*width, info->last.configure_request.width);
      *height = MAX (*height, info->last.configure_request.height);
    }
}

static void
popover_get_rect (CtkWindowPopover      *popover,
                  CtkWindow             *window,
                  cairo_rectangle_int_t *rect)
{
  CtkAllocation win_alloc;
  CtkRequisition req;
  CtkBorder win_border;
  gdouble min, max;

  ctk_widget_get_preferred_size (popover->widget, NULL, &req);
  _ctk_widget_get_allocation (CTK_WIDGET (window), &win_alloc);

  get_shadow_width (window, &win_border);
  win_alloc.x += win_border.left;
  win_alloc.y += win_border.top;
  win_alloc.width -= win_border.left + win_border.right;
  win_alloc.height -= win_border.top + win_border.bottom;

  rect->width = req.width;
  rect->height = req.height;

  if (popover->pos == CTK_POS_LEFT || popover->pos == CTK_POS_RIGHT)
    {
      if (req.height < win_alloc.height &&
          ctk_widget_get_vexpand (popover->widget))
        {
          rect->y = win_alloc.y;
          rect->height = win_alloc.height;
        }
      else
        {
          min = 0;
          max = win_alloc.y + win_alloc.height + win_border.bottom - req.height;

          if (popover->clamp_allocation)
            {
              min += win_border.top;
              max -= win_border.bottom;
            }

          rect->y = CLAMP (popover->rect.y + (popover->rect.height / 2) -
                           (req.height / 2), min, max);
        }

      if ((popover->pos == CTK_POS_LEFT) ==
          (ctk_widget_get_direction (popover->widget) == CTK_TEXT_DIR_LTR))
        {
          rect->x = popover->rect.x - req.width;

          if (rect->x > win_alloc.x && ctk_widget_get_hexpand (popover->widget))
            {
              rect->x = win_alloc.x;
              rect->width = popover->rect.x;
            }
        }
      else
        {
          rect->x = popover->rect.x + popover->rect.width;

          if (rect->x + rect->width < win_alloc.x + win_alloc.width &&
              ctk_widget_get_hexpand (popover->widget))
            rect->width = win_alloc.x + win_alloc.width - rect->x;
        }
    }
  else if (popover->pos == CTK_POS_TOP || popover->pos == CTK_POS_BOTTOM)
    {
      if (req.width < win_alloc.width &&
          ctk_widget_get_hexpand (popover->widget))
        {
          rect->x = win_alloc.x;
          rect->width = win_alloc.width;
        }
      else
        {
          min = 0;
          max = win_alloc.x + win_alloc.width + win_border.right - req.width;

          if (popover->clamp_allocation)
            {
              min += win_border.left;
              max -= win_border.right;
            }

          rect->x = CLAMP (popover->rect.x + (popover->rect.width / 2) -
                           (req.width / 2), min, max);
        }

      if (popover->pos == CTK_POS_TOP)
        {
          rect->y = popover->rect.y - req.height;

          if (rect->y > win_alloc.y &&
              ctk_widget_get_vexpand (popover->widget))
            {
              rect->y = win_alloc.y;
              rect->height = popover->rect.y;
            }
        }
      else
        {
          rect->y = popover->rect.y + popover->rect.height;

          if (rect->y + rect->height < win_alloc.y + win_alloc.height &&
              ctk_widget_get_vexpand (popover->widget))
            rect->height = win_alloc.y + win_alloc.height - rect->y;
        }
    }
}

static void
popover_realize (CtkWidget        *widget,
                 CtkWindowPopover *popover,
                 CtkWindow        *window)
{
  cairo_rectangle_int_t rect;
  CdkWindow *parent_window;
  CdkWindowAttr attributes;
  gint attributes_mask;

  if (popover->window)
    return;

  popover_get_rect (popover, window, &rect);

#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (ctk_widget_get_display (widget)))
    {
      attributes.window_type = CDK_WINDOW_SUBSURFACE;
      parent_window = cdk_screen_get_root_window (_ctk_window_get_screen (window));
    }
  else
#endif
    {
      attributes.window_type = CDK_WINDOW_CHILD;
      parent_window = _ctk_widget_get_window (CTK_WIDGET (window));
    }

  attributes.wclass = CDK_INPUT_OUTPUT;
  attributes.x = rect.x;
  attributes.y = rect.y;
  attributes.width = rect.width;
  attributes.height = rect.height;
  attributes.visual = ctk_widget_get_visual (CTK_WIDGET (window));
  attributes.event_mask = ctk_widget_get_events (popover->widget) |
    CDK_EXPOSURE_MASK;
  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;

  popover->window = cdk_window_new (parent_window, &attributes, attributes_mask);
  ctk_widget_register_window (CTK_WIDGET (window), popover->window);

#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (ctk_widget_get_display (widget)))
    cdk_window_set_transient_for (popover->window,
                                  _ctk_widget_get_window (CTK_WIDGET (window)));
#endif

  ctk_widget_set_parent_window (popover->widget, popover->window);
}

static void
check_scale_changed (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;
  CtkWidget *widget = CTK_WIDGET (window);
  int old_scale;

  old_scale = priv->scale;
  priv->scale = ctk_widget_get_scale_factor (widget);
  if (old_scale != priv->scale)
    _ctk_widget_scale_changed (widget);
}

static void
sum_borders (CtkBorder *one,
             CtkBorder *two)
{
  one->top += two->top;
  one->right += two->right;
  one->bottom += two->bottom;
  one->left += two->left;
}

static void
max_borders (CtkBorder *one,
             CtkBorder *two)
{
  one->top = MAX (one->top, two->top);
  one->right = MAX (one->right, two->right);
  one->bottom = MAX (one->bottom, two->bottom);
  one->left = MAX (one->left, two->left);
}

static void
subtract_borders (CtkBorder *one,
                  CtkBorder *two)
{
  one->top -= two->top;
  one->right -= two->right;
  one->bottom -= two->bottom;
  one->left -= two->left;
}

static void
get_shadow_width (CtkWindow *window,
                  CtkBorder *shadow_width)
{
  CtkWindowPrivate *priv = window->priv;
  CtkBorder border = { 0 };
  CtkBorder d = { 0 };
  CtkBorder margin;
  CtkStyleContext *context;
  CtkStateFlags s;
  CtkCssValue *shadows;

  *shadow_width = border;

  if (!priv->decorated)
    return;

  if (!priv->client_decorated &&
      !(ctk_window_should_use_csd (window) &&
        ctk_window_supports_client_shadow (window)))
    return;

  if (priv->maximized ||
      priv->fullscreen)
    return;

  if (!_ctk_widget_is_toplevel (CTK_WIDGET (window)))
    return;

  context = _ctk_widget_get_style_context (CTK_WIDGET (window));

  ctk_style_context_save_to_node (context, priv->decoration_node);
  s = ctk_style_context_get_state (context);

  /* Always sum border + padding */
  ctk_style_context_get_border (context, s, &border);
  ctk_style_context_get_padding (context, s, &d);
  sum_borders (&d, &border);

  /* Calculate the size of the drop shadows ... */
  shadows = _ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_BOX_SHADOW);
  _ctk_css_shadows_value_get_extents (shadows, &border);

  if (priv->type != CTK_WINDOW_POPUP)
    {
      /* ... and compare it to the margin size, which we use for resize grips */
      ctk_style_context_get_margin (context, s, &margin);
      max_borders (&border, &margin);
    }

  sum_borders (&d, &border);
  *shadow_width = d;

  ctk_style_context_restore (context);
}

static void
update_corner_windows (CtkWindow *window,
                       CtkBorder  border,
                       CtkBorder  window_border,
                       gint       width,
                       gint       height,
                       gint       handle_h,
                       gint       handle_v,
                       gboolean   resize_n,
                       gboolean   resize_e,
                       gboolean   resize_s,
                       gboolean   resize_w)
{
  CtkWindowPrivate *priv = window->priv;
  cairo_rectangle_int_t rect;
  cairo_region_t *region;

  /* North-West */
  if (resize_n && resize_w)
    {
      cdk_window_move_resize (priv->border_window[CDK_WINDOW_EDGE_NORTH_WEST],
                              window_border.left - border.left, window_border.top - border.top,
                              border.left + handle_h, border.top + handle_v);

      rect.x = 0;
      rect.y = 0;
      rect.width = border.left + handle_h;
      rect.height = border.top + handle_v;
      region = cairo_region_create_rectangle (&rect);
      rect.x = border.left;
      rect.y = border.top;
      rect.width = handle_h;
      rect.height = handle_v;
      cairo_region_subtract_rectangle (region, &rect);
      cdk_window_shape_combine_region (priv->border_window[CDK_WINDOW_EDGE_NORTH_WEST],
                                       region, 0, 0);
      cairo_region_destroy (region);

      cdk_window_show_unraised (priv->border_window[CDK_WINDOW_EDGE_NORTH_WEST]);
    }
  else
    {
      cdk_window_hide (priv->border_window[CDK_WINDOW_EDGE_NORTH_WEST]);
    }


  /* North-East */
  if (resize_n && resize_e)
    {
      cdk_window_move_resize (priv->border_window[CDK_WINDOW_EDGE_NORTH_EAST],
                              window_border.left + width - handle_h, window_border.top - border.top,
                              border.right + handle_h, border.top + handle_v);

      rect.x = 0;
      rect.y = 0;
      rect.width = border.right + handle_h;
      rect.height = border.top + handle_v;
      region = cairo_region_create_rectangle (&rect);
      rect.x = 0;
      rect.y = border.top;
      rect.width = handle_h;
      rect.height = handle_v;
      cairo_region_subtract_rectangle (region, &rect);
      cdk_window_shape_combine_region (priv->border_window[CDK_WINDOW_EDGE_NORTH_EAST],
                                       region, 0, 0);
      cairo_region_destroy (region);

      cdk_window_show_unraised (priv->border_window[CDK_WINDOW_EDGE_NORTH_EAST]);
    }
  else
    {
      cdk_window_hide (priv->border_window[CDK_WINDOW_EDGE_NORTH_EAST]);
    }

  /* South-West */
  if (resize_s && resize_w)
    {
      cdk_window_move_resize (priv->border_window[CDK_WINDOW_EDGE_SOUTH_WEST],
                              window_border.left - border.left, window_border.top + height - handle_v,
                              border.left + handle_h, border.bottom + handle_v);

      rect.x = 0;
      rect.y = 0;
      rect.width = border.left + handle_h;
      rect.height = border.bottom + handle_v;
      region = cairo_region_create_rectangle (&rect);
      rect.x = border.left;
      rect.y = 0;
      rect.width = handle_h;
      rect.height = handle_v;
      cairo_region_subtract_rectangle (region, &rect);
      cdk_window_shape_combine_region (priv->border_window[CDK_WINDOW_EDGE_SOUTH_WEST],
                                       region, 0, 0);
      cairo_region_destroy (region);

      cdk_window_show_unraised (priv->border_window[CDK_WINDOW_EDGE_SOUTH_WEST]);
    }
  else
    {
      cdk_window_hide (priv->border_window[CDK_WINDOW_EDGE_SOUTH_WEST]);
    }

  /* South-East */
  if (resize_s && resize_e)
    {
      cdk_window_move_resize (priv->border_window[CDK_WINDOW_EDGE_SOUTH_EAST],
                              window_border.left + width - handle_h, window_border.top + height - handle_v,
                              border.right + handle_h, border.bottom + handle_v);

      rect.x = 0;
      rect.y = 0;
      rect.width = border.right + handle_h;
      rect.height = border.bottom + handle_v;
      region = cairo_region_create_rectangle (&rect);
      rect.x = 0;
      rect.y = 0;
      rect.width = handle_h;
      rect.height = handle_v;
      cairo_region_subtract_rectangle (region, &rect);
      cdk_window_shape_combine_region (priv->border_window[CDK_WINDOW_EDGE_SOUTH_EAST],
                                       region, 0, 0);
      cairo_region_destroy (region);

      cdk_window_show_unraised (priv->border_window[CDK_WINDOW_EDGE_SOUTH_EAST]);
    }
  else
    {
      cdk_window_hide (priv->border_window[CDK_WINDOW_EDGE_SOUTH_EAST]);
    }
}

/* We're placing 8 input-only windows around
 * the window content as resize handles, as
 * follows:
 *
 * +-----------------------------------+
 * | +------+-----------------+------+ |
 * | |      |                 |      | |
 * | |   +--+-----------------+--+   | |
 * | |   |                       |   | |
 * | +---+                       +---+ |
 * | |   |                       |   | |
 * | |   |                       |   | |
 * | |   |                       |   | |
 * | +---+                       +---+ |
 * | |   |                       |   | |
 * | |   +--+-----------------+--+   | |
 * | |      |                 |      | |
 * | +------+-----------------+------+ |
 * +-----------------------------------+
 *
 * The corner windows are shaped to allow them
 * to extend into the edges. If the window is
 * not resizable in both dimensions, we hide
 * the corner windows and the edge windows in
 * the nonresizable dimension and make the
 * remaining edge window extend all the way.
 *
 * The border are where we place the resize handles
 * is also used to draw the window shadow, which may
 * extend out farther than the handles (or the other
 * way around).
 */
static void
update_border_windows (CtkWindow *window)
{
  CtkWidget *widget = (CtkWidget *)window;
  CtkWindowPrivate *priv = window->priv;
  gboolean resize_n, resize_e, resize_s, resize_w;
  gint handle, handle_h, handle_v;
  cairo_region_t *region;
  cairo_rectangle_int_t rect;
  gint width, height;
  gint x, w;
  gint y, h;
  CtkBorder border, tmp;
  CtkBorder window_border;
  CtkStyleContext *context;

  if (!priv->client_decorated)
    return;

  context = _ctk_widget_get_style_context (widget);

  ctk_style_context_save_to_node (context, priv->decoration_node);
  ctk_style_context_get_margin (context, ctk_style_context_get_state (context), &border);
  ctk_style_context_get_border (context, ctk_style_context_get_state (context), &tmp);
  sum_borders (&border, &tmp);
  ctk_style_context_get_padding (context, ctk_style_context_get_state (context), &tmp);
  sum_borders (&border, &tmp);
  ctk_widget_style_get (widget,
                        "decoration-resize-handle", &handle,
                        NULL);
  ctk_style_context_restore (context);
  get_shadow_width (window, &window_border);

  if (priv->border_window[0] == NULL)
    goto shape;

  if (!priv->resizable ||
      priv->fullscreen ||
      priv->maximized)
    {
      resize_n = resize_s = resize_e = resize_w = FALSE;
    }
  else if (priv->tiled || priv->edge_constraints)
    {
      /* Per-edge information is preferred when both priv->tiled and
       * priv->edge_constraints are set.
       */
      if (priv->edge_constraints)
        {
          resize_n = priv->edge_constraints & CDK_WINDOW_STATE_TOP_RESIZABLE;
          resize_e = priv->edge_constraints & CDK_WINDOW_STATE_RIGHT_RESIZABLE;
          resize_s = priv->edge_constraints & CDK_WINDOW_STATE_BOTTOM_RESIZABLE;
          resize_w = priv->edge_constraints & CDK_WINDOW_STATE_LEFT_RESIZABLE;
        }
      else
        {
          resize_n = resize_s = resize_e = resize_w = FALSE;
        }
    }
  else
    {
      resize_n = resize_s = resize_e = resize_w = TRUE;
      if (priv->geometry_info)
        {
          CdkGeometry *geometry = &priv->geometry_info->geometry;
          CdkWindowHints flags = priv->geometry_info->mask;

          if ((flags & CDK_HINT_MIN_SIZE) && (flags & CDK_HINT_MAX_SIZE))
            {
              resize_e = resize_w = geometry->min_width != geometry->max_width;
              resize_n = resize_s = geometry->min_height != geometry->max_height;
            }
        }
    }

  width = ctk_widget_get_allocated_width (widget) - (window_border.left + window_border.right);
  height = ctk_widget_get_allocated_height (widget) - (window_border.top + window_border.bottom);

  handle_h = MIN (handle, width / 2);
  handle_v = MIN (handle, height / 2);

  /*
   * Taking the following abstract representation of the resizable
   * edges:
   *
   * +-------------------------------------------+
   * | NW |              North              | NE |
   * |----+---------------------------------+----|
   * |    |                               x |    |
   * |    |                                 |    |
   * | W  |        Window contents          |  E |
   * |    |                                 |    |
   * |    |                                 |    |
   * |----+---------------------------------+----|
   * | SW |              South              | SE |
   * +-------------------------------------------+
   *
   * The idea behind the following math is if, say, the North edge is
   * resizable, East & West edges are moved down (i.e. y += North edge
   * size). If the South edge is resizable, only shrink East and West
   * heights. The same logic applies to the horizontal edges.
   *
   * The corner windows are only visible if both touching edges are
   * visible, for example, the NE window is visible if both N and E
   * edges are resizable.
   */

  x = 0;
  y = 0;
  w = width + window_border.left + window_border.right;
  h = height + window_border.top + window_border.bottom;

  if (resize_n)
    {
      y += window_border.top + handle_v;
      h -= window_border.top + handle_v;
    }

  if (resize_w)
    {
      x += window_border.left + handle_h;
      w -= window_border.left + handle_h;
    }

  if (resize_s)
    h -= window_border.bottom + handle_v;

  if (resize_e)
    w -= window_border.right + handle_h;

  /* North */
  if (resize_n)
    {
      cdk_window_move_resize (priv->border_window[CDK_WINDOW_EDGE_NORTH],
                              x, window_border.top - border.top,
                              w, border.top);

      cdk_window_show_unraised (priv->border_window[CDK_WINDOW_EDGE_NORTH]);
    }
  else
    {
      cdk_window_hide (priv->border_window[CDK_WINDOW_EDGE_NORTH]);
    }

  /* South */
  if (resize_s)
    {
      cdk_window_move_resize (priv->border_window[CDK_WINDOW_EDGE_SOUTH],
                              x, window_border.top + height,
                              w, border.bottom);

      cdk_window_show_unraised (priv->border_window[CDK_WINDOW_EDGE_SOUTH]);
    }
  else
    {
      cdk_window_hide (priv->border_window[CDK_WINDOW_EDGE_SOUTH]);
    }

  /* East */
  if (resize_e)
    {
      cdk_window_move_resize (priv->border_window[CDK_WINDOW_EDGE_EAST],
                              window_border.left + width, y,
                              border.right, h);

      cdk_window_show_unraised (priv->border_window[CDK_WINDOW_EDGE_EAST]);
    }
  else
    {
      cdk_window_hide (priv->border_window[CDK_WINDOW_EDGE_EAST]);
    }

  /* West */
  if (resize_w)
    {
      cdk_window_move_resize (priv->border_window[CDK_WINDOW_EDGE_WEST],
                              window_border.left - border.left, y,
                              border.left, h);

      cdk_window_show_unraised (priv->border_window[CDK_WINDOW_EDGE_WEST]);
    }
  else
    {
      cdk_window_hide (priv->border_window[CDK_WINDOW_EDGE_WEST]);
    }

  update_corner_windows (window, border, window_border, width, height, handle_h, handle_v,
                         resize_n, resize_e, resize_s, resize_w);

shape:
  /* we also update the input shape, which makes it so that clicks
   * outside the border windows go through
   */

  if (priv->type != CTK_WINDOW_POPUP)
    subtract_borders (&window_border, &border);

  rect.x = window_border.left;
  rect.y = window_border.top;
  rect.width = ctk_widget_get_allocated_width (widget) - window_border.left - window_border.right;
  rect.height = ctk_widget_get_allocated_height (widget) - window_border.top - window_border.bottom;
  region = cairo_region_create_rectangle (&rect);
  ctk_widget_set_csd_input_shape (widget, region);
  cairo_region_destroy (region);
}

static void
update_shadow_width (CtkWindow *window,
                     CtkBorder *border)
{
  CdkWindow *cdk_window;

  cdk_window = _ctk_widget_get_window (CTK_WIDGET (window));

  if (cdk_window)
    cdk_window_set_shadow_width (cdk_window,
                                 border->left,
                                 border->right,
                                 border->top,
                                 border->bottom);
}

static void
corner_rect (cairo_rectangle_int_t *rect,
             const CtkCssValue     *value)
{
  rect->width = _ctk_css_corner_value_get_x (value, 100);
  rect->height = _ctk_css_corner_value_get_y (value, 100);
}

static void
subtract_decoration_corners_from_region (cairo_region_t        *region,
                                         cairo_rectangle_int_t *extents,
                                         CtkStyleContext       *context,
                                         CtkWindow             *window)
{
  CtkWindowPrivate *priv = window->priv;
  cairo_rectangle_int_t rect;

  if (!priv->client_decorated ||
      !priv->decorated ||
      priv->fullscreen ||
      priv->maximized)
    return;

  ctk_style_context_save_to_node (context, window->priv->decoration_node);

  corner_rect (&rect, _ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_BORDER_TOP_LEFT_RADIUS));
  rect.x = extents->x;
  rect.y = extents->y;
  cairo_region_subtract_rectangle (region, &rect);

  corner_rect (&rect, _ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_BORDER_TOP_RIGHT_RADIUS));
  rect.x = extents->x + extents->width - rect.width;
  rect.y = extents->y;
  cairo_region_subtract_rectangle (region, &rect);

  corner_rect (&rect, _ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_BORDER_BOTTOM_LEFT_RADIUS));
  rect.x = extents->x;
  rect.y = extents->y + extents->height - rect.height;
  cairo_region_subtract_rectangle (region, &rect);

  corner_rect (&rect, _ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_BORDER_BOTTOM_RIGHT_RADIUS));
  rect.x = extents->x + extents->width - rect.width;
  rect.y = extents->y + extents->height - rect.height;
  cairo_region_subtract_rectangle (region, &rect);

  ctk_style_context_restore (context);
}

static void
update_opaque_region (CtkWindow           *window,
                      const CtkBorder     *border,
                      const CtkAllocation *allocation)
{
  CtkWidget *widget = CTK_WIDGET (window);
  cairo_region_t *opaque_region;
  CtkStyleContext *context;
  gboolean is_opaque = FALSE;

  if (!_ctk_widget_get_realized (widget))
      return;

  context = ctk_widget_get_style_context (widget);

  if (!ctk_widget_get_app_paintable (widget))
    {
      const CdkRGBA *color;
      color = _ctk_css_rgba_value_get_rgba (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_BACKGROUND_COLOR));
      is_opaque = (color->alpha >= 1.0);
    }

  if (ctk_widget_get_opacity (widget) < 1.0)
    is_opaque = FALSE;

  if (is_opaque)
    {
      cairo_rectangle_int_t rect;

      rect.x = border->left;
      rect.y = border->top;
      rect.width = allocation->width - border->left - border->right;
      rect.height = allocation->height - border->top - border->bottom;

      opaque_region = cairo_region_create_rectangle (&rect);

      subtract_decoration_corners_from_region (opaque_region, &rect, context, window);
    }
  else
    {
      opaque_region = NULL;
    }

  cdk_window_set_opaque_region (_ctk_widget_get_window (widget), opaque_region);

  cairo_region_destroy (opaque_region);
}

static void
update_realized_window_properties (CtkWindow     *window,
                                   CtkAllocation *child_allocation,
                                   CtkBorder     *window_border)
{
  CtkWindowPrivate *priv = window->priv;

  if (!_ctk_widget_is_toplevel (CTK_WIDGET (window)))
    return;

  if (priv->client_decorated && priv->use_client_shadow)
    update_shadow_width (window, window_border);

  update_opaque_region (window, window_border, child_allocation);

  update_border_windows (window);
}

static void
ctk_window_realize (CtkWidget *widget)
{
  CtkAllocation allocation;
  CtkAllocation child_allocation;
  CtkWindow *window;
  CdkWindow *parent_window;
  CdkWindow *cdk_window;
  CdkWindowAttr attributes;
  CtkBorder window_border;
  gint attributes_mask;
  CtkWindowPrivate *priv;
  gint i;
  GList *link;

  window = CTK_WINDOW (widget);
  priv = window->priv;

  if (!priv->client_decorated && ctk_window_should_use_csd (window))
    create_decoration (widget);

  _ctk_widget_get_allocation (widget, &allocation);

  if (ctk_widget_get_parent_window (widget))
    {
      ctk_container_set_default_resize_mode (CTK_CONTAINER (widget), CTK_RESIZE_PARENT);

      attributes.x = allocation.x;
      attributes.y = allocation.y;
      attributes.width = allocation.width;
      attributes.height = allocation.height;
      attributes.window_type = CDK_WINDOW_CHILD;

      attributes.event_mask = ctk_widget_get_events (widget) | CDK_EXPOSURE_MASK | CDK_STRUCTURE_MASK;

      attributes.visual = ctk_widget_get_visual (widget);
      attributes.wclass = CDK_INPUT_OUTPUT;

      attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;

      cdk_window = cdk_window_new (ctk_widget_get_parent_window (widget),
				   &attributes, attributes_mask);
      ctk_widget_set_window (widget, cdk_window);
      ctk_widget_register_window (widget, cdk_window);
      ctk_widget_set_realized (widget, TRUE);

      link = priv->popovers;

      while (link)
        {
          CtkWindowPopover *popover = link->data;
          link = link->next;
          popover_realize (popover->widget, popover, window);
        }

      return;
    }

  ctk_container_set_default_resize_mode (CTK_CONTAINER (window), CTK_RESIZE_QUEUE);

  /* ensure widget tree is properly size allocated */
  if (allocation.x == -1 &&
      allocation.y == -1 &&
      allocation.width == 1 &&
      allocation.height == 1)
    {
      CdkRectangle request;

      ctk_window_compute_configure_request (window, &request, NULL, NULL);

      allocation.x = 0;
      allocation.y = 0;
      allocation.width = request.width;
      allocation.height = request.height;
      ctk_widget_size_allocate (widget, &allocation);

      ctk_widget_queue_resize (widget);

      g_return_if_fail (!_ctk_widget_get_realized (widget));
    }

  if (priv->hardcoded_window)
    {
      cdk_window = priv->hardcoded_window;
      _ctk_widget_get_allocation (widget, &allocation);
      cdk_window_resize (cdk_window, allocation.width, allocation.height);
    }
  else
    {
      switch (priv->type)
        {
        case CTK_WINDOW_TOPLEVEL:
          attributes.window_type = CDK_WINDOW_TOPLEVEL;
          break;
        case CTK_WINDOW_POPUP:
          attributes.window_type = CDK_WINDOW_TEMP;
          break;
        default:
          g_warning (G_STRLOC": Unknown window type %d!", priv->type);
          break;
        }

#ifdef CDK_WINDOWING_WAYLAND
      if (priv->use_subsurface &&
          CDK_IS_WAYLAND_DISPLAY (ctk_widget_get_display (widget)))
        attributes.window_type = CDK_WINDOW_SUBSURFACE;
#endif

      attributes.title = priv->title;
      attributes.wmclass_name = priv->wmclass_name;
      attributes.wmclass_class = priv->wmclass_class;
      attributes.wclass = CDK_INPUT_OUTPUT;
      attributes.visual = ctk_widget_get_visual (widget);

      attributes_mask = 0;
      parent_window = cdk_screen_get_root_window (_ctk_window_get_screen (window));

      _ctk_widget_get_allocation (widget, &allocation);
      attributes.width = allocation.width;
      attributes.height = allocation.height;
      attributes.event_mask = ctk_widget_get_events (widget);
      attributes.event_mask |= (CDK_EXPOSURE_MASK |
                                CDK_BUTTON_PRESS_MASK |
                                CDK_BUTTON_RELEASE_MASK |
                                CDK_BUTTON_MOTION_MASK |
                                CDK_KEY_PRESS_MASK |
                                CDK_KEY_RELEASE_MASK |
                                CDK_ENTER_NOTIFY_MASK |
                                CDK_LEAVE_NOTIFY_MASK |
                                CDK_FOCUS_CHANGE_MASK |
                                CDK_STRUCTURE_MASK);

      if (priv->decorated && priv->client_decorated)
        attributes.event_mask |= CDK_POINTER_MOTION_MASK;

      attributes.type_hint = priv->type_hint;

      attributes_mask |= CDK_WA_VISUAL | CDK_WA_TYPE_HINT;
      attributes_mask |= (priv->title ? CDK_WA_TITLE : 0);
      attributes_mask |= (priv->wmclass_name ? CDK_WA_WMCLASS : 0);

      cdk_window = cdk_window_new (parent_window, &attributes, attributes_mask);
    }

  ctk_widget_set_window (widget, cdk_window);
  ctk_widget_register_window (widget, cdk_window);
  ctk_widget_set_realized (widget, TRUE);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = CDK_WINDOW_CHILD;

  attributes.event_mask = ctk_widget_get_events (widget) | CDK_EXPOSURE_MASK | CDK_STRUCTURE_MASK;

  attributes.visual = ctk_widget_get_visual (widget);
  attributes.wclass = CDK_INPUT_OUTPUT;

  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;

  if (priv->client_decorated && priv->type == CTK_WINDOW_TOPLEVEL)
    {
      const gchar *cursor[8] = {
        "nw-resize", "n-resize", "ne-resize",
        "w-resize",               "e-resize",
        "sw-resize", "s-resize", "se-resize"
      };

      attributes.wclass = CDK_INPUT_ONLY;
      attributes.width = 1;
      attributes.height = 1;
      attributes.event_mask = CDK_BUTTON_PRESS_MASK;
      attributes_mask = CDK_WA_CURSOR;

      for (i = 0; i < 8; i++)
        {
          attributes.cursor = cdk_cursor_new_from_name (ctk_widget_get_display (widget), cursor[i]);
          priv->border_window[i] = cdk_window_new (cdk_window, &attributes, attributes_mask);
          g_clear_object (&attributes.cursor);

          cdk_window_show (priv->border_window[i]);
          ctk_widget_register_window (widget, priv->border_window[i]);
        }
    }

  if (priv->transient_parent &&
      _ctk_widget_get_realized (CTK_WIDGET (priv->transient_parent)))
    cdk_window_set_transient_for (cdk_window,
                                  _ctk_widget_get_window (CTK_WIDGET (priv->transient_parent)));

  if (priv->wm_role)
    cdk_window_set_role (cdk_window, priv->wm_role);

  if (!priv->decorated || priv->client_decorated)
    cdk_window_set_decorations (cdk_window, 0);

#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_WINDOW (cdk_window))
    {
      if (priv->client_decorated)
        cdk_wayland_window_announce_csd (cdk_window);
      else
        cdk_wayland_window_announce_ssd (cdk_window);
    }
#endif

  if (!priv->deletable)
    cdk_window_set_functions (cdk_window, CDK_FUNC_ALL | CDK_FUNC_CLOSE);

  if (ctk_window_get_skip_pager_hint (window))
    cdk_window_set_skip_pager_hint (cdk_window, TRUE);

  if (ctk_window_get_skip_taskbar_hint (window))
    cdk_window_set_skip_taskbar_hint (cdk_window, TRUE);

  if (ctk_window_get_accept_focus (window))
    cdk_window_set_accept_focus (cdk_window, TRUE);
  else
    cdk_window_set_accept_focus (cdk_window, FALSE);

  if (ctk_window_get_focus_on_map (window))
    cdk_window_set_focus_on_map (cdk_window, TRUE);
  else
    cdk_window_set_focus_on_map (cdk_window, FALSE);

  if (priv->modal)
    cdk_window_set_modal_hint (cdk_window, TRUE);
  else
    cdk_window_set_modal_hint (cdk_window, FALSE);

  if (priv->startup_id)
    {
#ifdef CDK_WINDOWING_X11
      if (CDK_IS_X11_WINDOW (cdk_window))
        {
          guint32 timestamp = extract_time_from_startup_id (priv->startup_id);
          if (timestamp != CDK_CURRENT_TIME)
            cdk_x11_window_set_user_time (cdk_window, timestamp);
        }
#endif
      if (!startup_id_is_fake (priv->startup_id))
        cdk_window_set_startup_id (cdk_window, priv->startup_id);
    }

#ifdef CDK_WINDOWING_X11
  if (priv->initial_timestamp != CDK_CURRENT_TIME)
    {
      if (CDK_IS_X11_WINDOW (cdk_window))
        cdk_x11_window_set_user_time (cdk_window, priv->initial_timestamp);
    }
#endif

  child_allocation.x = 0;
  child_allocation.y = 0;
  child_allocation.width = allocation.width;
  child_allocation.height = allocation.height;

  get_shadow_width (window, &window_border);

  update_realized_window_properties (window, &child_allocation, &window_border);

  if (priv->application)
    ctk_application_handle_window_realize (priv->application, window);

  /* Icons */
  ctk_window_realize_icon (window);

  link = priv->popovers;

  while (link)
    {
      CtkWindowPopover *popover = link->data;
      link = link->next;
      popover_realize (popover->widget, popover, window);
    }

  check_scale_changed (window);
}

static void
popover_unrealize (CtkWidget        *widget,
                   CtkWindowPopover *popover,
                   CtkWindow        *window)
{
#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (ctk_widget_get_display (widget)))
    cdk_window_set_transient_for (popover->window, NULL);
#endif

  ctk_widget_unregister_window (CTK_WIDGET (window), popover->window);
  ctk_widget_unrealize (popover->widget);
  cdk_window_destroy (popover->window);
  popover->window = NULL;
}

static void
ctk_window_unrealize (CtkWidget *widget)
{
  CtkWindow *window = CTK_WINDOW (widget);
  CtkWindowPrivate *priv = window->priv;
  CtkWindowGeometryInfo *info;
  GList *link;
  gint i;

  /* On unrealize, we reset the size of the window such
   * that we will re-apply the default sizing stuff
   * next time we show the window.
   *
   * Default positioning is reset on unmap, instead of unrealize.
   */
  priv->need_default_size = TRUE;
  info = ctk_window_get_geometry_info (window, FALSE);
  if (info)
    {
      info->resize_width = -1;
      info->resize_height = -1;
      info->last.configure_request.x = 0;
      info->last.configure_request.y = 0;
      info->last.configure_request.width = -1;
      info->last.configure_request.height = -1;
      /* be sure we reset geom hints on re-realize */
      info->last.flags = 0;
    }

  if (priv->popup_menu)
    {
      ctk_widget_destroy (priv->popup_menu);
      priv->popup_menu = NULL;
    }

  /* Icons */
  ctk_window_unrealize_icon (window);

  if (priv->border_window[0] != NULL)
    {
      for (i = 0; i < 8; i++)
        {
          ctk_widget_unregister_window (widget, priv->border_window[i]);
          cdk_window_destroy (priv->border_window[i]);
          priv->border_window[i] = NULL;
        }
    }

  link = priv->popovers;

  while (link)
    {
      CtkWindowPopover *popover = link->data;
      link = link->next;
      popover_unrealize (popover->widget, popover, window);
    }

  CTK_WIDGET_CLASS (ctk_window_parent_class)->unrealize (widget);

  priv->hardcoded_window = NULL;
}

static void
update_window_style_classes (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (CTK_WIDGET (window));

  if (priv->tiled)
    ctk_style_context_add_class (context, "tiled");
  else
    ctk_style_context_remove_class (context, "tiled");

  if (priv->edge_constraints != 0)
    {
      guint edge_constraints = priv->edge_constraints;

      if (edge_constraints & CDK_WINDOW_STATE_TOP_TILED)
        ctk_style_context_add_class (context, "tiled-top");
      else
        ctk_style_context_remove_class (context, "tiled-top");

      if (edge_constraints & CDK_WINDOW_STATE_RIGHT_TILED)
        ctk_style_context_add_class (context, "tiled-right");
      else
        ctk_style_context_remove_class (context, "tiled-right");

      if (edge_constraints & CDK_WINDOW_STATE_BOTTOM_TILED)
        ctk_style_context_add_class (context, "tiled-bottom");
      else
        ctk_style_context_remove_class (context, "tiled-bottom");

      if (edge_constraints & CDK_WINDOW_STATE_LEFT_TILED)
        ctk_style_context_add_class (context, "tiled-left");
      else
        ctk_style_context_remove_class (context, "tiled-left");
    }

  if (priv->maximized)
    ctk_style_context_add_class (context, "maximized");
  else
    ctk_style_context_remove_class (context, "maximized");

  if (priv->fullscreen)
    ctk_style_context_add_class (context, "fullscreen");
  else
    ctk_style_context_remove_class (context, "fullscreen");
}

static void
popover_size_allocate (CtkWidget        *widget,
                       CtkWindowPopover *popover,
                       CtkWindow        *window)
{
  cairo_rectangle_int_t rect;

  if (!popover->window)
    return;

  if (CTK_IS_POPOVER (popover->widget))
    ctk_popover_update_position (CTK_POPOVER (popover->widget));

  popover_get_rect (popover, window, &rect);
  cdk_window_move_resize (popover->window, rect.x, rect.y,
                          rect.width, rect.height);
  rect.x = rect.y = 0;
  ctk_widget_size_allocate (widget, &rect);

  if (ctk_widget_is_drawable (CTK_WIDGET (window)) &&
      ctk_widget_is_visible (widget))
    {
      if (!cdk_window_is_visible (popover->window))
        cdk_window_show_unraised (popover->window);
    }
  else if (cdk_window_is_visible (popover->window))
    cdk_window_hide (popover->window);
}

/* _ctk_window_set_allocation:
 * @window: a #CtkWindow
 * @allocation: the original allocation for the window
 * @allocation_out: @allocation taking decorations into
 * consideration
 *
 * This function is like ctk_widget_set_allocation()
 * but does the necessary extra work to update
 * the resize grip positioning, etc.
 *
 * Call this instead of ctk_widget_set_allocation()
 * when overriding ::size_allocate in a CtkWindow
 * subclass without chaining up.
 *
 * The @allocation parameter will be adjusted to
 * reflect any internal decorations that the window
 * may have. That revised allocation will then be
 * returned in the @allocation_out parameter.
 */
void
_ctk_window_set_allocation (CtkWindow           *window,
                            const CtkAllocation *allocation,
                            CtkAllocation       *allocation_out)
{
  CtkWidget *widget = (CtkWidget *)window;
  CtkWindowPrivate *priv = window->priv;
  CtkAllocation child_allocation;
  gint border_width;
  CtkBorder window_border = { 0 };
  GList *link;

  g_assert (allocation != NULL);
  g_assert (allocation_out != NULL);

  ctk_widget_set_allocation (widget, allocation);

  child_allocation.x = 0;
  child_allocation.y = 0;
  child_allocation.width = allocation->width;
  child_allocation.height = allocation->height;

  get_shadow_width (window, &window_border);

  if (_ctk_widget_get_realized (widget))
    update_realized_window_properties (window, &child_allocation, &window_border);

  priv->title_height = 0;

  if (priv->title_box != NULL &&
      ctk_widget_get_visible (priv->title_box) &&
      ctk_widget_get_child_visible (priv->title_box) &&
      priv->decorated &&
      !priv->fullscreen)
    {
      CtkAllocation title_allocation;

      title_allocation.x = window_border.left;
      title_allocation.y = window_border.top;
      title_allocation.width =
        MAX (1, (gint) allocation->width -
             window_border.left - window_border.right);

      ctk_widget_get_preferred_height_for_width (priv->title_box,
                                                 title_allocation.width,
                                                 NULL,
                                                 &priv->title_height);

      title_allocation.height = priv->title_height;

      ctk_widget_size_allocate (priv->title_box, &title_allocation);
    }

  if (priv->decorated &&
      !priv->fullscreen)
    {
      child_allocation.x += window_border.left;
      child_allocation.y += window_border.top + priv->title_height;
      child_allocation.width -= window_border.left + window_border.right;
      child_allocation.height -= window_border.top + window_border.bottom +
                                 priv->title_height;
    }

  if (!_ctk_widget_is_toplevel (widget) && _ctk_widget_get_realized (widget))
    {
      cdk_window_move_resize (_ctk_widget_get_window (widget),
                              allocation->x, allocation->y,
                              allocation->width, allocation->height);
    }

  border_width = ctk_container_get_border_width (CTK_CONTAINER (window));
  child_allocation.x += border_width;
  child_allocation.y += border_width;
  child_allocation.width = MAX (1, child_allocation.width - border_width * 2);
  child_allocation.height = MAX (1, child_allocation.height - border_width * 2);

  *allocation_out = child_allocation;

  link = priv->popovers;
  while (link)
    {
      CtkWindowPopover *popover = link->data;
      link = link->next;
      popover_size_allocate (popover->widget, popover, window);
    }

}

static void
ctk_window_restack_popovers (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;
  GList *link = priv->popovers;

  while (link)
    {
      CtkWindowPopover *popover = link->data;
      link = link->next;

      if (popover->window)
        cdk_window_raise (popover->window);
    }
}

static void
ctk_window_size_allocate (CtkWidget     *widget,
                          CtkAllocation *allocation)
{
  CtkWindow *window = CTK_WINDOW (widget);
  CtkWidget *child;
  CtkAllocation child_allocation;

  _ctk_window_set_allocation (window, allocation, &child_allocation);

  child = ctk_bin_get_child (CTK_BIN (window));
  if (child && ctk_widget_get_visible (child))
    ctk_widget_size_allocate (child, &child_allocation);

  ctk_window_restack_popovers (window);
}

static gint
ctk_window_configure_event (CtkWidget         *widget,
			    CdkEventConfigure *event)
{
  CtkAllocation allocation;
  CtkWindow *window = CTK_WINDOW (widget);
  CtkWindowPrivate *priv = window->priv;

  check_scale_changed (window);

  if (!_ctk_widget_is_toplevel (widget))
    return FALSE;

  if (_ctk_widget_get_window (widget) != event->window)
    return TRUE;

  /* If this is a gratuitous ConfigureNotify that's already
   * the same as our allocation, then we can fizzle it out.
   * This is the case for dragging windows around.
   *
   * We can't do this for a ConfigureRequest, since it might
   * have been a queued resize from child widgets, and so we
   * need to reallocate our children in case *they* changed.
   */
  _ctk_widget_get_allocation (widget, &allocation);
  if (priv->configure_request_count == 0 &&
      (allocation.width == event->width &&
       allocation.height == event->height))
    {
      return TRUE;
    }

  /* priv->configure_request_count incremented for each
   * configure request, and decremented to a min of 0 for
   * each configure notify.
   *
   * All it means is that we know we will get at least
   * priv->configure_request_count more configure notifies.
   * We could get more configure notifies than that; some
   * of the configure notifies we get may be unrelated to
   * the configure requests. But we will get at least
   * priv->configure_request_count notifies.
   */

  if (priv->configure_request_count > 0)
    {
      priv->configure_request_count -= 1;

      CDK_PRIVATE_CALL (cdk_window_thaw_toplevel_updates) (_ctk_widget_get_window (widget));
    }

  /*
   * If we do need to resize, we do that by:
   *   - setting configure_notify_received to TRUE
   *     for use in ctk_window_move_resize()
   *   - queueing a resize, leading to invocation of
   *     ctk_window_move_resize() in an idle handler
   *
   */

  priv->configure_notify_received = TRUE;

  ctk_widget_queue_allocate (widget);
  ctk_container_queue_resize_handler (CTK_CONTAINER (widget));

  return TRUE;
}

static void
update_edge_constraints (CtkWindow           *window,
                         CdkEventWindowState *event)
{
  CtkWindowPrivate *priv = window->priv;
  CdkWindowState state = event->new_window_state;

  priv->edge_constraints = (state & CDK_WINDOW_STATE_TOP_TILED) |
                           (state & CDK_WINDOW_STATE_TOP_RESIZABLE) |
                           (state & CDK_WINDOW_STATE_RIGHT_TILED) |
                           (state & CDK_WINDOW_STATE_RIGHT_RESIZABLE) |
                           (state & CDK_WINDOW_STATE_BOTTOM_TILED) |
                           (state & CDK_WINDOW_STATE_BOTTOM_RESIZABLE) |
                           (state & CDK_WINDOW_STATE_LEFT_TILED) |
                           (state & CDK_WINDOW_STATE_LEFT_RESIZABLE);

  priv->tiled = (state & CDK_WINDOW_STATE_TILED) ? 1 : 0;
}

static gboolean
ctk_window_state_event (CtkWidget           *widget,
                        CdkEventWindowState *event)
{
  CtkWindow *window = CTK_WINDOW (widget);
  CtkWindowPrivate *priv = window->priv;

  if (event->changed_mask & CDK_WINDOW_STATE_FOCUSED)
    ensure_state_flag_backdrop (widget);

  if (event->changed_mask & CDK_WINDOW_STATE_FULLSCREEN)
    {
      priv->fullscreen =
        (event->new_window_state & CDK_WINDOW_STATE_FULLSCREEN) ? 1 : 0;
    }

  if (event->changed_mask & CDK_WINDOW_STATE_MAXIMIZED)
    {
      priv->maximized =
        (event->new_window_state & CDK_WINDOW_STATE_MAXIMIZED) ? 1 : 0;
      g_object_notify_by_pspec (G_OBJECT (widget), window_props[PROP_IS_MAXIMIZED]);
    }

  update_edge_constraints (window, event);

  if (event->changed_mask & (CDK_WINDOW_STATE_FULLSCREEN |
                             CDK_WINDOW_STATE_MAXIMIZED |
                             CDK_WINDOW_STATE_TILED |
                             CDK_WINDOW_STATE_TOP_TILED |
                             CDK_WINDOW_STATE_RIGHT_TILED |
                             CDK_WINDOW_STATE_BOTTOM_TILED |
                             CDK_WINDOW_STATE_LEFT_TILED))
    {
      update_window_style_classes (window);
      update_window_buttons (window);
      ctk_widget_queue_resize (widget);
    }

  return FALSE;
}

/**
 * ctk_window_set_has_resize_grip:
 * @window: a #CtkWindow
 * @value: %TRUE to allow a resize grip
 *
 * Sets whether @window has a corner resize grip.
 *
 * Note that the resize grip is only shown if the window
 * is actually resizable and not maximized. Use
 * ctk_window_resize_grip_is_visible() to find out if the
 * resize grip is currently shown.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14: Resize grips have been removed.
 */
void
ctk_window_set_has_resize_grip (CtkWindow *window,
                                gboolean   value)
{
  g_return_if_fail (CTK_IS_WINDOW (window));
}

/**
 * ctk_window_resize_grip_is_visible:
 * @window: a #CtkWindow
 *
 * Determines whether a resize grip is visible for the specified window.
 *
 * Returns: %TRUE if a resize grip exists and is visible
 *
 * Since: 3.0
 *
 * Deprecated: 3.14: Resize grips have been removed.
 */
gboolean
ctk_window_resize_grip_is_visible (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return FALSE;
}

/**
 * ctk_window_get_has_resize_grip:
 * @window: a #CtkWindow
 *
 * Determines whether the window may have a resize grip.
 *
 * Returns: %TRUE if the window has a resize grip
 *
 * Since: 3.0
 *
 * Deprecated: 3.14: Resize grips have been removed.
 */
gboolean
ctk_window_get_has_resize_grip (CtkWindow *window)

{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return FALSE;
}

/**
 * ctk_window_get_resize_grip_area:
 * @window: a #CtkWindow
 * @rect: (out): a pointer to a #CdkRectangle which we should store
 *     the resize grip area
 *
 * If a window has a resize grip, this will retrieve the grip
 * position, width and height into the specified #CdkRectangle.
 *
 * Returns: %TRUE if the resize grip’s area was retrieved
 *
 * Since: 3.0
 *
 * Deprecated: 3.14: Resize grips have been removed.
 */
gboolean
ctk_window_get_resize_grip_area (CtkWindow    *window,
                                 CdkRectangle *rect)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return FALSE;
}

/* the accel_key and accel_mods fields of the key have to be setup
 * upon calling this function. it’ll then return whether that key
 * is at all used as accelerator, and if so will OR in the
 * accel_flags member of the key.
 */
gboolean
_ctk_window_query_nonaccels (CtkWindow      *window,
			     guint           accel_key,
			     CdkModifierType accel_mods)
{
  CtkWindowPrivate *priv;

  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  priv = window->priv;

  /* movement keys are considered locked accels */
  if (!accel_mods)
    {
      static const guint bindings[] = {
	CDK_KEY_space, CDK_KEY_KP_Space, CDK_KEY_Return, CDK_KEY_ISO_Enter, CDK_KEY_KP_Enter, CDK_KEY_Up, CDK_KEY_KP_Up, CDK_KEY_Down, CDK_KEY_KP_Down,
	CDK_KEY_Left, CDK_KEY_KP_Left, CDK_KEY_Right, CDK_KEY_KP_Right, CDK_KEY_Tab, CDK_KEY_KP_Tab, CDK_KEY_ISO_Left_Tab,
      };
      guint i;
      
      for (i = 0; i < G_N_ELEMENTS (bindings); i++)
	if (bindings[i] == accel_key)
	  return TRUE;
    }

  /* mnemonics are considered locked accels */
  if (accel_mods == priv->mnemonic_modifier)
    {
      CtkMnemonicHash *mnemonic_hash = ctk_window_get_mnemonic_hash (window, FALSE);
      if (mnemonic_hash && _ctk_mnemonic_hash_lookup (mnemonic_hash, accel_key))
	return TRUE;
    }

  return FALSE;
}

/**
 * ctk_window_propagate_key_event:
 * @window:  a #CtkWindow
 * @event:   a #CdkEventKey
 *
 * Propagate a key press or release event to the focus widget and
 * up the focus container chain until a widget handles @event.
 * This is normally called by the default ::key_press_event and
 * ::key_release_event handlers for toplevel windows,
 * however in some cases it may be useful to call this directly when
 * overriding the standard key handling for a toplevel window.
 *
 * Returns: %TRUE if a widget in the focus chain handled the event.
 *
 * Since: 2.4
 */
gboolean
ctk_window_propagate_key_event (CtkWindow        *window,
                                CdkEventKey      *event)
{
  CtkWindowPrivate *priv = window->priv;
  gboolean handled = FALSE;
  CtkWidget *widget, *focus;

  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  widget = CTK_WIDGET (window);

  focus = priv->focus_widget;
  if (focus)
    g_object_ref (focus);
  
  while (!handled &&
         focus && focus != widget &&
         ctk_widget_get_toplevel (focus) == widget)
    {
      CtkWidget *parent;
      
      if (ctk_widget_is_sensitive (focus))
        {
          handled = ctk_widget_event (focus, (CdkEvent*) event);
          if (handled)
            break;
        }

      parent = _ctk_widget_get_parent (focus);
      if (parent)
        g_object_ref (parent);
      
      g_object_unref (focus);
      
      focus = parent;
    }
  
  if (focus)
    g_object_unref (focus);

  return handled;
}

static gint
ctk_window_key_press_event (CtkWidget   *widget,
			    CdkEventKey *event)
{
  CtkWindow *window = CTK_WINDOW (widget);
  gboolean handled = FALSE;

  /* handle mnemonics and accelerators */
  if (!handled)
    handled = ctk_window_activate_key (window, event);

  /* handle focus widget key events */
  if (!handled)
    handled = ctk_window_propagate_key_event (window, event);

  /* Chain up, invokes binding set */
  if (!handled)
    handled = CTK_WIDGET_CLASS (ctk_window_parent_class)->key_press_event (widget, event);

  return handled;
}

static gint
ctk_window_key_release_event (CtkWidget   *widget,
			      CdkEventKey *event)
{
  CtkWindow *window = CTK_WINDOW (widget);
  gboolean handled = FALSE;

  /* handle focus widget key events */
  if (!handled)
    handled = ctk_window_propagate_key_event (window, event);

  /* Chain up, invokes binding set */
  if (!handled)
    handled = CTK_WIDGET_CLASS (ctk_window_parent_class)->key_release_event (widget, event);

  return handled;
}

static CtkWindowRegion
get_active_region_type (CtkWindow *window, CdkEventAny *event, gint x, gint y)
{
  CtkWindowPrivate *priv = window->priv;
  CtkAllocation allocation;
  gint i;

  for (i = 0; i < 8; i++)
    {
      if (event->window == priv->border_window[i])
        return i;
    }

  if (priv->title_box != NULL &&
      ctk_widget_get_visible (priv->title_box) &&
      ctk_widget_get_child_visible (priv->title_box))
    {
      _ctk_widget_get_allocation (priv->title_box, &allocation);
      if (allocation.x <= x && allocation.x + allocation.width > x &&
          allocation.y <= y && allocation.y + allocation.height > y)
        return CTK_WINDOW_REGION_TITLE;
    }

  return CTK_WINDOW_REGION_CONTENT;
}

static gboolean
controller_handle_wm_event (CtkGesture     *gesture,
                            const CdkEvent *event)
{
  CdkEventSequence *seq;
  gboolean retval;

  seq = cdk_event_get_event_sequence (event);
  retval = ctk_event_controller_handle_event (CTK_EVENT_CONTROLLER (gesture),
                                              event);

  /* Reset immediately the gestures, here we don't get many guarantees
   * about whether the target window event mask will be complete enough
   * to keep gestures consistent, or whether any widget across the
   * hierarchy will be inconsistent about event handler return values.
   */
  if (ctk_gesture_get_sequence_state (gesture, seq) == CTK_EVENT_SEQUENCE_DENIED)
    ctk_event_controller_reset (CTK_EVENT_CONTROLLER (gesture));

  return retval;
}

static gboolean
ctk_window_handle_wm_event (CtkWindow *window,
                            CdkEvent  *event,
                            gboolean   run_drag)
{
  gboolean retval = CDK_EVENT_PROPAGATE;
  CtkWindowPrivate *priv;

  if (event->type == CDK_BUTTON_PRESS || event->type == CDK_BUTTON_RELEASE ||
      event->type == CDK_TOUCH_BEGIN || event->type == CDK_TOUCH_UPDATE ||
      event->type == CDK_MOTION_NOTIFY || event->type == CDK_TOUCH_END)
    {
      priv = window->priv;

      if (run_drag && priv->drag_gesture)
        retval |= controller_handle_wm_event (priv->drag_gesture,
                                              (const CdkEvent*) event);

      if (priv->multipress_gesture)
        retval |= controller_handle_wm_event (priv->multipress_gesture,
                                              (const CdkEvent*) event);
    }

  return retval;
}

gboolean
_ctk_window_check_handle_wm_event (CdkEvent *event)
{
  CtkWindowPrivate *priv;
  CtkWidget *widget;

  widget = ctk_get_event_widget (event);

  if (!CTK_IS_WINDOW (widget))
    widget = ctk_widget_get_toplevel (widget);

  if (!CTK_IS_WINDOW (widget))
    return CDK_EVENT_PROPAGATE;

  priv = CTK_WINDOW (widget)->priv;

  if (!priv->multipress_gesture)
    return CDK_EVENT_PROPAGATE;

  if (event->type != CDK_BUTTON_PRESS && event->type != CDK_BUTTON_RELEASE &&
      event->type != CDK_MOTION_NOTIFY && event->type != CDK_TOUCH_BEGIN &&
      event->type != CDK_TOUCH_END && event->type != CDK_TOUCH_UPDATE)
    return CDK_EVENT_PROPAGATE;

  if (ctk_widget_event (widget, event))
    return CDK_EVENT_STOP;

  return ctk_window_handle_wm_event (CTK_WINDOW (widget), event, TRUE);
}

static gboolean
ctk_window_event (CtkWidget *widget,
                  CdkEvent  *event)
{
  if (widget != ctk_get_event_widget (event))
    return ctk_window_handle_wm_event (CTK_WINDOW (widget), event, FALSE);

  return CDK_EVENT_PROPAGATE;
}

static void
ctk_window_real_activate_default (CtkWindow *window)
{
  ctk_window_activate_default (window);
}

static void
ctk_window_real_activate_focus (CtkWindow *window)
{
  ctk_window_activate_focus (window);
}

static void
do_focus_change (CtkWidget *widget,
		 gboolean   in)
{
  CdkWindow *window;
  CdkDeviceManager *device_manager;
  GList *devices, *d;

  g_object_ref (widget);

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

      if (cdk_device_get_source (dev) != CDK_SOURCE_KEYBOARD)
        continue;

      /* Skip non-master keyboards that haven't
       * selected for events from this window
       */
      window = _ctk_widget_get_window (widget);
      if (cdk_device_get_device_type (dev) != CDK_DEVICE_TYPE_MASTER &&
          window && !cdk_window_get_device_events (window, dev))
        continue;

      fevent = cdk_event_new (CDK_FOCUS_CHANGE);

      fevent->focus_change.type = CDK_FOCUS_CHANGE;
      fevent->focus_change.window = window;
      if (window)
        g_object_ref (window);
      fevent->focus_change.in = in;
      cdk_event_set_device (fevent, dev);

      ctk_widget_send_focus_change (widget, fevent);

      cdk_event_free (fevent);
    }

  g_list_free (devices);
  g_object_unref (widget);
}

static gboolean
ctk_window_has_mnemonic_modifier_pressed (CtkWindow *window)
{
  GList *seats, *s;
  gboolean retval = FALSE;

  if (!window->priv->mnemonic_modifier)
    return FALSE;

  seats = cdk_display_list_seats (ctk_widget_get_display (CTK_WIDGET (window)));

  for (s = seats; s; s = s->next)
    {
      CdkDevice *dev = cdk_seat_get_pointer (s->data);
      CdkModifierType mask;

      cdk_device_get_state (dev, _ctk_widget_get_window (CTK_WIDGET (window)),
                            NULL, &mask);
      if (window->priv->mnemonic_modifier == (mask & ctk_accelerator_get_default_mod_mask ()))
        {
          retval = TRUE;
          break;
        }
    }

  g_list_free (seats);

  return retval;
}

static gint
ctk_window_focus_in_event (CtkWidget     *widget,
			   CdkEventFocus *event)
{
  CtkWindow *window = CTK_WINDOW (widget);

  /* It appears spurious focus in events can occur when
   *  the window is hidden. So we'll just check to see if
   *  the window is visible before actually handling the
   *  event
   */
  if (ctk_widget_get_visible (widget))
    {
      _ctk_window_set_has_toplevel_focus (window, TRUE);
      _ctk_window_set_is_active (window, TRUE);

      if (ctk_window_has_mnemonic_modifier_pressed (window))
        _ctk_window_schedule_mnemonics_visible (window);
    }

  return FALSE;
}

static gint
ctk_window_focus_out_event (CtkWidget     *widget,
			    CdkEventFocus *event)
{
  CtkWindow *window = CTK_WINDOW (widget);

  _ctk_window_set_has_toplevel_focus (window, FALSE);
  _ctk_window_set_is_active (window, FALSE);

  /* set the mnemonic-visible property to false */
  ctk_window_set_mnemonics_visible (window, FALSE);

  return FALSE;
}

static CtkWindowPopover *
_ctk_window_has_popover (CtkWindow *window,
                         CtkWidget *widget)
{
  CtkWindowPrivate *priv = window->priv;
  GList *link;

  for (link = priv->popovers; link; link = link->next)
    {
      CtkWindowPopover *popover = link->data;

      if (popover->widget == widget)
        return popover;
    }

  return NULL;
}

static void
ctk_window_remove (CtkContainer *container,
                  CtkWidget     *widget)
{
  CtkWindow *window = CTK_WINDOW (container);

  if (widget == window->priv->title_box)
    unset_titlebar (window);
  else if (_ctk_window_has_popover (window, widget))
    _ctk_window_remove_popover (window, widget);
  else
    CTK_CONTAINER_CLASS (ctk_window_parent_class)->remove (container, widget);
}

static void
ctk_window_check_resize (CtkContainer *container)
{
  /* If the window is not toplevel anymore than it's embedded somewhere,
   * so handle it like a normal window */
  if (!_ctk_widget_is_toplevel (CTK_WIDGET (container)))
    CTK_CONTAINER_CLASS (ctk_window_parent_class)->check_resize (container);
  else if (!_ctk_widget_get_alloc_needed (CTK_WIDGET (container)))
    CTK_CONTAINER_CLASS (ctk_window_parent_class)->check_resize (container);
  else if (ctk_widget_get_visible (CTK_WIDGET (container)))
    ctk_window_move_resize (CTK_WINDOW (container));
}

static void
ctk_window_forall (CtkContainer *container,
                   gboolean	 include_internals,
                   CtkCallback   callback,
                   gpointer      callback_data)
{
  CtkWindow *window = CTK_WINDOW (container);
  CtkWindowPrivate *priv = window->priv;
  CtkWidget *child;

  if (include_internals)
    {
      GList *l;

      for (l = priv->popovers; l; l = l->next)
        {
          CtkWindowPopover *data = l->data;
          (* callback) (data->widget, callback_data);
        }
    }

  child = ctk_bin_get_child (CTK_BIN (container));
  if (child != NULL)
    (* callback) (child, callback_data);

  if (priv->title_box != NULL &&
      (priv->titlebar == NULL || include_internals))
    (* callback) (priv->title_box, callback_data);
}

static gboolean
ctk_window_focus (CtkWidget        *widget,
		  CtkDirectionType  direction)
{
  CtkWindowPrivate *priv;
  CtkBin *bin;
  CtkWindow *window;
  CtkContainer *container;
  CtkWidget *child;
  CtkWidget *old_focus_child;
  CtkWidget *parent;

  if (!_ctk_widget_is_toplevel (widget))
    return CTK_WIDGET_CLASS (ctk_window_parent_class)->focus (widget, direction);

  container = CTK_CONTAINER (widget);
  window = CTK_WINDOW (widget);
  priv = window->priv;
  bin = CTK_BIN (widget);

  old_focus_child = ctk_container_get_focus_child (container);

  /* We need a special implementation here to deal properly with wrapping
   * around in the tab chain without the danger of going into an
   * infinite loop.
   */
  if (old_focus_child)
    {
      if (ctk_widget_child_focus (old_focus_child, direction))
        return TRUE;
    }

  if (priv->focus_widget)
    {
      if (direction == CTK_DIR_LEFT ||
	  direction == CTK_DIR_RIGHT ||
	  direction == CTK_DIR_UP ||
	  direction == CTK_DIR_DOWN)
	{
	  return FALSE;
	}
      
      /* Wrapped off the end, clear the focus setting for the toplpevel */
      parent = _ctk_widget_get_parent (priv->focus_widget);
      while (parent)
	{
	  ctk_container_set_focus_child (CTK_CONTAINER (parent), NULL);
	  parent = _ctk_widget_get_parent (parent);
	}
      
      ctk_window_set_focus (CTK_WINDOW (container), NULL);
    }

  /* Now try to focus the first widget in the window,
   * taking care to hook titlebar widgets into the
   * focus chain.
  */
  if (priv->title_box != NULL &&
      old_focus_child != NULL &&
      priv->title_box != old_focus_child)
    child = priv->title_box;
  else
    child = ctk_bin_get_child (bin);

  if (child)
    {
      if (ctk_widget_child_focus (child, direction))
        return TRUE;
      else if (priv->title_box != NULL &&
               priv->title_box != child &&
               ctk_widget_child_focus (priv->title_box, direction))
        return TRUE;
      else if (priv->title_box == child &&
               ctk_widget_child_focus (ctk_bin_get_child (bin), direction))
        return TRUE;
    }

  return FALSE;
}

static void
ctk_window_move_focus (CtkWidget        *widget,
                       CtkDirectionType  dir)
{
  if (!_ctk_widget_is_toplevel (widget))
    {
      CTK_WIDGET_CLASS (ctk_window_parent_class)->move_focus (widget, dir);
      return;
    }

  ctk_widget_child_focus (widget, dir);

  if (! ctk_container_get_focus_child (CTK_CONTAINER (widget)))
    ctk_window_set_focus (CTK_WINDOW (widget), NULL);
}

static void
ctk_window_real_set_focus (CtkWindow *window,
			   CtkWidget *focus)
{
  CtkWindowPrivate *priv = window->priv;
  CtkWidget *old_focus = priv->focus_widget;
  gboolean had_default = FALSE;
  gboolean focus_had_default = FALSE;
  gboolean old_focus_had_default = FALSE;

  if (old_focus)
    {
      g_object_ref (old_focus);
      g_object_freeze_notify (G_OBJECT (old_focus));
      old_focus_had_default = ctk_widget_has_default (old_focus);
    }
  if (focus)
    {
      g_object_ref (focus);
      g_object_freeze_notify (G_OBJECT (focus));
      focus_had_default = ctk_widget_has_default (focus);
    }

  if (priv->default_widget)
    had_default = ctk_widget_has_default (priv->default_widget);

  if (priv->focus_widget)
    {
      if (ctk_widget_get_receives_default (priv->focus_widget) &&
	  (priv->focus_widget != priv->default_widget))
        {
          _ctk_widget_set_has_default (priv->focus_widget, FALSE);
	  ctk_widget_queue_draw (priv->focus_widget);

	  if (priv->default_widget)
            _ctk_widget_set_has_default (priv->default_widget, TRUE);
	}

      priv->focus_widget = NULL;

      if (priv->has_focus)
	do_focus_change (old_focus, FALSE);

      g_object_notify (G_OBJECT (old_focus), "is-focus");
    }

  /* The above notifications may have set a new focus widget,
   * if so, we don't want to override it.
   */
  if (focus && !priv->focus_widget)
    {
      priv->focus_widget = focus;

      if (ctk_widget_get_receives_default (priv->focus_widget) &&
	  (priv->focus_widget != priv->default_widget))
	{
	  if (ctk_widget_get_can_default (priv->focus_widget))
            _ctk_widget_set_has_default (priv->focus_widget, TRUE);

	  if (priv->default_widget)
            _ctk_widget_set_has_default (priv->default_widget, FALSE);
	}

      if (priv->has_focus)
	do_focus_change (priv->focus_widget, TRUE);

      /* It's possible for do_focus_change() above to have callbacks
       * that clear priv->focus_widget here.
       */
      if (priv->focus_widget)
        g_object_notify (G_OBJECT (priv->focus_widget), "is-focus");
    }

  /* If the default widget changed, a redraw will have been queued
   * on the old and new default widgets by ctk_window_set_default(), so
   * we only have to worry about the case where it didn't change.
   * We'll sometimes queue a draw twice on the new widget but that
   * is harmless.
   */
  if (priv->default_widget &&
      (had_default != ctk_widget_has_default (priv->default_widget)))
    ctk_widget_queue_draw (priv->default_widget);
  
  if (old_focus)
    {
      if (old_focus_had_default != ctk_widget_has_default (old_focus))
	ctk_widget_queue_draw (old_focus);
	
      g_object_thaw_notify (G_OBJECT (old_focus));
      g_object_unref (old_focus);
    }
  if (focus)
    {
      if (focus_had_default != ctk_widget_has_default (focus))
	ctk_widget_queue_draw (focus);

      g_object_thaw_notify (G_OBJECT (focus));
      g_object_unref (focus);
    }
}

static void
ctk_window_get_preferred_width (CtkWidget *widget,
                                gint      *minimum_size,
                                gint      *natural_size)
{
  CtkWindow *window;
  CtkWidget *child;
  CtkWindowPrivate *priv;
  guint border_width;
  gint title_min = 0, title_nat = 0;
  gint child_min = 0, child_nat = 0;
  CtkBorder window_border = { 0 };
  gboolean has_size_request;

  window = CTK_WINDOW (widget);
  priv = window->priv;
  child  = ctk_bin_get_child (CTK_BIN (window));
  has_size_request = ctk_widget_has_size_request (widget);

  border_width = ctk_container_get_border_width (CTK_CONTAINER (window));

  if (priv->decorated &&
      !priv->fullscreen)
    {
      get_shadow_width (window, &window_border);

      if (priv->title_box != NULL &&
          ctk_widget_get_visible (priv->title_box) &&
          ctk_widget_get_child_visible (priv->title_box))
        ctk_widget_get_preferred_width (priv->title_box,
                                        &title_min, &title_nat);

      title_min += window_border.left + window_border.right;
      title_nat += window_border.left + window_border.right;
    }

  if (child && ctk_widget_get_visible (child))
    {
      ctk_widget_get_preferred_width (child, &child_min, &child_nat);

      if (child_nat == 0 && !has_size_request)
        child_nat = NO_CONTENT_CHILD_NAT;
      child_min += border_width * 2 +
                   window_border.left + window_border.right;
      child_nat += border_width * 2 +
                   window_border.left + window_border.right;
    }
  else if (!has_size_request)
    {
      child_nat = NO_CONTENT_CHILD_NAT;
    }

  *minimum_size = MAX (title_min, child_min);
  *natural_size = MAX (title_nat, child_nat);
}


static void
ctk_window_get_preferred_width_for_height (CtkWidget *widget,
                                           gint       height,
                                           gint      *minimum_size,
                                           gint      *natural_size)
{
  CtkWindow *window;
  CtkWidget *child;
  CtkWindowPrivate *priv;
  guint border_width;
  gint title_min = 0, title_nat = 0;
  gint child_min = 0, child_nat = 0;
  gint title_height = 0;
  CtkBorder window_border = { 0 };
  gboolean has_size_request;

  window = CTK_WINDOW (widget);
  priv = window->priv;
  child  = ctk_bin_get_child (CTK_BIN (window));
  has_size_request = ctk_widget_has_size_request (widget);

  border_width = ctk_container_get_border_width (CTK_CONTAINER (window));

  height -= 2 * border_width;

  if (priv->decorated &&
      !priv->fullscreen)
    {
      get_shadow_width (window, &window_border);

      height -= window_border.top + window_border.bottom;

      if (priv->title_box != NULL &&
          ctk_widget_get_visible (priv->title_box) &&
          ctk_widget_get_child_visible (priv->title_box))
        {
          ctk_widget_get_preferred_height (priv->title_box,
                                           NULL, &title_height);
          ctk_widget_get_preferred_width_for_height (priv->title_box,
                                                     title_height,
                                                     &title_min, &title_nat);
          height -= title_height;
        }

      title_min += window_border.left + window_border.right;
      title_nat += window_border.left + window_border.right;
    }

  if (child && ctk_widget_get_visible (child))
    {
      ctk_widget_get_preferred_width_for_height (child,
                                                 MAX (height, 0),
                                                 &child_min, &child_nat);

      if (child_nat == 0 && height == 0 && !has_size_request)
        child_nat = NO_CONTENT_CHILD_NAT;
      child_min += border_width * 2 +
                   window_border.left + window_border.right;
      child_nat += border_width * 2 +
                   window_border.left + window_border.right;
    }
  else if (!has_size_request)
    {
      child_nat = NO_CONTENT_CHILD_NAT;
    }

  *minimum_size = MAX (title_min, child_min);
  *natural_size = MAX (title_nat, child_nat);
}

static void
ctk_window_get_preferred_height (CtkWidget *widget,
                                 gint      *minimum_size,
                                 gint      *natural_size)
{
  CtkWindow *window;
  CtkWindowPrivate *priv;
  CtkWidget *child;
  guint border_width;
  int title_min = 0;
  int title_height = 0;
  CtkBorder window_border = { 0 };
  gboolean has_size_request;

  window = CTK_WINDOW (widget);
  priv = window->priv;
  child  = ctk_bin_get_child (CTK_BIN (window));
  has_size_request = ctk_widget_has_size_request (widget);

  *minimum_size = 0;
  *natural_size = 0;

  border_width = ctk_container_get_border_width (CTK_CONTAINER (window));

  if (priv->decorated &&
      !priv->fullscreen)
    {
      get_shadow_width (window, &window_border);

      if (priv->title_box != NULL &&
          ctk_widget_get_visible (priv->title_box) &&
          ctk_widget_get_child_visible (priv->title_box))
        ctk_widget_get_preferred_height (priv->title_box,
                                         &title_min,
                                         &title_height);

      *minimum_size = title_min +
                      window_border.top + window_border.bottom;

      *natural_size = title_height +
                      window_border.top + window_border.bottom;
    }

  if (child && ctk_widget_get_visible (child))
    {
      gint child_min, child_nat;
      ctk_widget_get_preferred_height (child, &child_min, &child_nat);

      if (child_nat == 0 && !has_size_request)
        child_nat = NO_CONTENT_CHILD_NAT;
      *minimum_size += child_min + 2 * border_width;
      *natural_size += child_nat + 2 * border_width;
    }
  else if (!has_size_request)
    {
      *natural_size += NO_CONTENT_CHILD_NAT;
    }
}


static void
ctk_window_get_preferred_height_for_width (CtkWidget *widget,
                                           gint       width,
                                           gint      *minimum_size,
                                           gint      *natural_size)
{
  CtkWindow *window;
  CtkWindowPrivate *priv;
  CtkWidget *child;
  guint border_width;
  int title_min = 0;
  int title_height = 0;
  CtkBorder window_border = { 0 };
  gboolean has_size_request;

  window = CTK_WINDOW (widget);
  priv = window->priv;
  child  = ctk_bin_get_child (CTK_BIN (window));
  has_size_request = ctk_widget_has_size_request (widget);

  *minimum_size = 0;
  *natural_size = 0;

  border_width = ctk_container_get_border_width (CTK_CONTAINER (window));

  width -= 2 * border_width;

  if (priv->decorated &&
      !priv->fullscreen)
    {
      get_shadow_width (window, &window_border);

      width -= window_border.left + window_border.right;

      if (priv->title_box != NULL &&
          ctk_widget_get_visible (priv->title_box) &&
          ctk_widget_get_child_visible (priv->title_box))
        ctk_widget_get_preferred_height_for_width (priv->title_box,
                                                   MAX (width, 0),
                                                   &title_min,
                                                   &title_height);

      *minimum_size = title_min +
                      window_border.top + window_border.bottom;

      *natural_size = title_height +
                      window_border.top + window_border.bottom;
    }

  if (child && ctk_widget_get_visible (child))
    {
      gint child_min, child_nat;
      ctk_widget_get_preferred_height_for_width (child, MAX (width, 0),
                                                 &child_min, &child_nat);

      if (child_nat == 0 && width == 0 && !has_size_request)
        child_nat = NO_CONTENT_CHILD_NAT;
      *minimum_size += child_min + 2 * border_width;
      *natural_size += child_nat + 2 * border_width;
    }
  else if (!has_size_request)
    {
      *natural_size += NO_CONTENT_CHILD_NAT;
    }
}

static void
ctk_window_state_flags_changed (CtkWidget     *widget,
                                CtkStateFlags  previous_state)
{
  CtkWindow *window = CTK_WINDOW (widget);
  CtkWindowPrivate *priv = window->priv;
  CtkStateFlags state;

  state = ctk_widget_get_state_flags (widget);
  ctk_css_node_set_state (priv->decoration_node, state);

  CTK_WIDGET_CLASS (ctk_window_parent_class)->state_flags_changed (widget, previous_state);
}

static void
ctk_window_style_updated (CtkWidget *widget)
{
  CtkCssStyleChange *change = ctk_style_context_get_change (ctk_widget_get_style_context (widget));
  CtkWindow *window = CTK_WINDOW (widget);

  CTK_WIDGET_CLASS (ctk_window_parent_class)->style_updated (widget);

  if (!_ctk_widget_get_alloc_needed (widget) &&
      (change == NULL || ctk_css_style_change_changes_property (change, CTK_CSS_PROPERTY_BACKGROUND_COLOR)))
    {
      CtkAllocation allocation;
      CtkBorder window_border;

      _ctk_widget_get_allocation (widget, &allocation);
      get_shadow_width (window, &window_border);

      update_opaque_region (window, &window_border, &allocation);
    }

  if (change == NULL || ctk_css_style_change_changes_property (change, CTK_CSS_PROPERTY_ICON_THEME))
    update_themed_icon (window);
}

/**
 * _ctk_window_unset_focus_and_default:
 * @window: a #CtkWindow
 * @widget: a widget inside of @window
 * 
 * Checks whether the focus and default widgets of @window are
 * @widget or a descendent of @widget, and if so, unset them.
 *
 * If @widget is a #CtkPopover then nothing will be done with
 * respect to the default widget of @window, the reason being that
 * popovers already have specific logic for clearing/restablishing
 * the default widget of its enclosing window.
 **/
void
_ctk_window_unset_focus_and_default (CtkWindow *window,
				     CtkWidget *widget)

{
  CtkWindowPrivate *priv = window->priv;
  CtkWidget *child;
  CtkWidget *parent;

  g_object_ref (window);
  g_object_ref (widget);

  parent = _ctk_widget_get_parent (widget);
  if (ctk_container_get_focus_child (CTK_CONTAINER (parent)) == widget)
    {
      child = priv->focus_widget;
      
      while (child && child != widget)
        child = _ctk_widget_get_parent (child);

      if (child == widget)
	ctk_window_set_focus (CTK_WINDOW (window), NULL);
    }
  
  if (!CTK_IS_POPOVER (widget))
    {
      child = priv->default_widget;

      while (child && child != widget)
        child = _ctk_widget_get_parent (child);

      if (child == widget)
        ctk_window_set_default (window, NULL);
    }

  g_object_unref (widget);
  g_object_unref (window);
}

static void
popup_menu_detach (CtkWidget *widget,
                   CtkMenu   *menu)
{
  CTK_WINDOW (widget)->priv->popup_menu = NULL;
}

static CdkWindowState
ctk_window_get_state (CtkWindow *window)
{
  CdkWindowState state;
  CdkWindow *cdk_window;

  cdk_window = ctk_widget_get_window (CTK_WIDGET (window));

  state = 0;

  if (cdk_window)
    state = cdk_window_get_state (cdk_window);

  return state;
}

static void
restore_window_clicked (CtkMenuItem *menuitem,
                        gpointer     user_data)
{
  CtkWindow *window = CTK_WINDOW (user_data);
  CtkWindowPrivate *priv = window->priv;
  CdkWindowState state;

  if (priv->maximized)
    {
      ctk_window_unmaximize (window);

      return;
    }

  state = ctk_window_get_state (window);

  if (state & CDK_WINDOW_STATE_ICONIFIED)
    ctk_window_deiconify (window);
}

static void
move_window_clicked (CtkMenuItem *menuitem,
                     gpointer     user_data)
{
  CtkWindow *window = CTK_WINDOW (user_data);

  ctk_window_begin_move_drag (window,
                              0, /* 0 means "use keyboard" */
                              0, 0,
                              CDK_CURRENT_TIME);
}

static void
resize_window_clicked (CtkMenuItem *menuitem,
                       gpointer     user_data)
{
  CtkWindow *window = CTK_WINDOW (user_data);

  ctk_window_begin_resize_drag  (window,
                                 0,
                                 0, /* 0 means "use keyboard" */
                                 0, 0,
                                 CDK_CURRENT_TIME);
}

static void
minimize_window_clicked (CtkMenuItem *menuitem,
                         gpointer     user_data)
{
  CtkWindow *window = CTK_WINDOW (user_data);
  CtkWindowPrivate *priv = window->priv;

  /* Turns out, we can't iconify a maximized window */
  if (priv->maximized)
    ctk_window_unmaximize (window);

  ctk_window_iconify (window);
}

static void
maximize_window_clicked (CtkMenuItem *menuitem,
                         gpointer     user_data)
{
  CtkWindow *window = CTK_WINDOW (user_data);
  CdkWindowState state;

  state = ctk_window_get_state (window);

  if (state & CDK_WINDOW_STATE_ICONIFIED)
    ctk_window_deiconify (window);

  ctk_window_maximize (window);
}

static void
ontop_window_clicked (CtkMenuItem *menuitem,
                      gpointer     user_data)
{
  CtkWindow *window = (CtkWindow *)user_data;

  ctk_window_set_keep_above (window, !window->priv->above_initially);
}

static void
close_window_clicked (CtkMenuItem *menuitem,
                      gpointer     user_data)
{
  CtkWindow *window = (CtkWindow *)user_data;

  if (window->priv->delete_event_handler == 0)
    send_delete_event (window);
}

static void
ctk_window_do_popup_fallback (CtkWindow      *window,
                              CdkEventButton *event)
{
  CtkWindowPrivate *priv = window->priv;
  CtkWidget *menuitem;
  CdkWindowState state;
  gboolean maximized, iconified;

  if (priv->popup_menu)
    ctk_widget_destroy (priv->popup_menu);

  state = ctk_window_get_state (window);

  iconified = (state & CDK_WINDOW_STATE_ICONIFIED) == CDK_WINDOW_STATE_ICONIFIED;
  maximized = priv->maximized && !iconified;

  priv->popup_menu = ctk_menu_new ();
  ctk_style_context_add_class (ctk_widget_get_style_context (priv->popup_menu),
                               CTK_STYLE_CLASS_CONTEXT_MENU);

  ctk_menu_attach_to_widget (CTK_MENU (priv->popup_menu),
                             CTK_WIDGET (window),
                             popup_menu_detach);

  menuitem = ctk_menu_item_new_with_label (_("Restore"));
  ctk_widget_show (menuitem);
  /* "Restore" means "Unmaximize" or "Unminimize"
   * (yes, some WMs allow window menu to be shown for minimized windows).
   * Not restorable:
   *   - visible windows that are not maximized or minimized
   *   - non-resizable windows that are not minimized
   *   - non-normal windows
   */
  if ((ctk_widget_is_visible (CTK_WIDGET (window)) &&
       !(maximized || iconified)) ||
      (!iconified && !priv->resizable) ||
      priv->type_hint != CDK_WINDOW_TYPE_HINT_NORMAL)
    ctk_widget_set_sensitive (menuitem, FALSE);
  g_signal_connect (G_OBJECT (menuitem), "activate",
                    G_CALLBACK (restore_window_clicked), window);
  ctk_menu_shell_append (CTK_MENU_SHELL (priv->popup_menu), menuitem);

  menuitem = ctk_menu_item_new_with_label (_("Move"));
  ctk_widget_show (menuitem);
  if (maximized || iconified)
    ctk_widget_set_sensitive (menuitem, FALSE);
  g_signal_connect (G_OBJECT (menuitem), "activate",
                    G_CALLBACK (move_window_clicked), window);
  ctk_menu_shell_append (CTK_MENU_SHELL (priv->popup_menu), menuitem);

  menuitem = ctk_menu_item_new_with_label (_("Resize"));
  ctk_widget_show (menuitem);
  if (!priv->resizable || maximized || iconified)
    ctk_widget_set_sensitive (menuitem, FALSE);
  g_signal_connect (G_OBJECT (menuitem), "activate",
                    G_CALLBACK (resize_window_clicked), window);
  ctk_menu_shell_append (CTK_MENU_SHELL (priv->popup_menu), menuitem);

  menuitem = ctk_menu_item_new_with_label (_("Minimize"));
  ctk_widget_show (menuitem);
  if (iconified ||
      priv->type_hint != CDK_WINDOW_TYPE_HINT_NORMAL)
    ctk_widget_set_sensitive (menuitem, FALSE);
  g_signal_connect (G_OBJECT (menuitem), "activate",
                    G_CALLBACK (minimize_window_clicked), window);
  ctk_menu_shell_append (CTK_MENU_SHELL (priv->popup_menu), menuitem);

  menuitem = ctk_menu_item_new_with_label (_("Maximize"));
  ctk_widget_show (menuitem);
  if (maximized ||
      !priv->resizable ||
      priv->type_hint != CDK_WINDOW_TYPE_HINT_NORMAL)
    ctk_widget_set_sensitive (menuitem, FALSE);
  g_signal_connect (G_OBJECT (menuitem), "activate",
                    G_CALLBACK (maximize_window_clicked), window);
  ctk_menu_shell_append (CTK_MENU_SHELL (priv->popup_menu), menuitem);

  menuitem = ctk_separator_menu_item_new ();
  ctk_widget_show (menuitem);
  ctk_menu_shell_append (CTK_MENU_SHELL (priv->popup_menu), menuitem);

  menuitem = ctk_check_menu_item_new_with_label (_("Always on Top"));
  ctk_check_menu_item_set_active (CTK_CHECK_MENU_ITEM (menuitem), priv->above_initially);
  if (maximized)
    ctk_widget_set_sensitive (menuitem, FALSE);
  ctk_widget_show (menuitem);
  g_signal_connect (G_OBJECT (menuitem), "activate",
                    G_CALLBACK (ontop_window_clicked), window);
  ctk_menu_shell_append (CTK_MENU_SHELL (priv->popup_menu), menuitem);

  menuitem = ctk_separator_menu_item_new ();
  ctk_widget_show (menuitem);
  ctk_menu_shell_append (CTK_MENU_SHELL (priv->popup_menu), menuitem);

  menuitem = ctk_menu_item_new_with_label (_("Close"));
  ctk_widget_show (menuitem);
  if (!priv->deletable)
    ctk_widget_set_sensitive (menuitem, FALSE);
  g_signal_connect (G_OBJECT (menuitem), "activate",
                    G_CALLBACK (close_window_clicked), window);
  ctk_menu_shell_append (CTK_MENU_SHELL (priv->popup_menu), menuitem);
  ctk_menu_popup_at_pointer (CTK_MENU (priv->popup_menu), (CdkEvent *) event);
}

static void
ctk_window_do_popup (CtkWindow      *window,
                     CdkEventButton *event)
{
  if (!cdk_window_show_window_menu (_ctk_widget_get_window (CTK_WIDGET (window)),
                                    (CdkEvent *) event))
    ctk_window_do_popup_fallback (window, event);
}

/*********************************
 * Functions related to resizing *
 *********************************/

static void
geometry_size_to_pixels (CdkGeometry *geometry,
			 guint        flags,
			 gint        *width,
			 gint        *height)
{
  gint base_width = 0;
  gint base_height = 0;
  gint min_width = 0;
  gint min_height = 0;
  gint width_inc = 1;
  gint height_inc = 1;

  if (flags & CDK_HINT_BASE_SIZE)
    {
      base_width = geometry->base_width;
      base_height = geometry->base_height;
    }
  if (flags & CDK_HINT_MIN_SIZE)
    {
      min_width = geometry->min_width;
      min_height = geometry->min_height;
    }
  if (flags & CDK_HINT_RESIZE_INC)
    {
      width_inc = geometry->width_inc;
      height_inc = geometry->height_inc;
    }

  if (width)
    *width = MAX (*width * width_inc + base_width, min_width);
  if (height)
    *height = MAX (*height * height_inc + base_height, min_height);
}

/* This function doesn't constrain to geometry hints */
static void 
ctk_window_compute_configure_request_size (CtkWindow   *window,
                                           CdkGeometry *geometry,
                                           guint        flags,
                                           gint        *width,
                                           gint        *height)
{
  CtkWindowPrivate *priv = window->priv;
  CtkWindowGeometryInfo *info;
  int w, h;

  /* Preconditions:
   *  - we've done a size request
   */
  
  info = ctk_window_get_geometry_info (window, FALSE);

  if ((priv->need_default_size || priv->force_resize) &&
      !priv->maximized &&
      !priv->fullscreen)
    {
      ctk_window_guess_default_size (window, width, height);
      ctk_window_get_remembered_size (window, &w, &h);
      *width = MAX (*width, w);
      *height = MAX (*height, h);

      /* Override with default size */
      if (info)
        {
          /* Take width of shadows/headerbar into account. We want to set the
           * default size of the content area and not the window area.
           */
          gint default_width_csd = info->default_width;
          gint default_height_csd = info->default_height;
          ctk_window_update_csd_size (window,
                                      &default_width_csd, &default_height_csd,
                                      INCLUDE_CSD_SIZE);

          if (info->default_width > 0)
            *width = default_width_csd;
          if (info->default_height > 0)
            *height = default_height_csd;

          if (info->default_is_geometry)
            geometry_size_to_pixels (geometry, flags,
                                  info->default_width > 0 ? width : NULL,
                                  info->default_height > 0 ? height : NULL);
        }
    }
  else
    {
      /* Default to keeping current size */
      ctk_window_get_remembered_size (window, width, height);
    }

  if (info)
    {
      gint resize_width_csd = info->resize_width;
      gint resize_height_csd = info->resize_height;
      ctk_window_update_csd_size (window,
                                  &resize_width_csd, &resize_height_csd,
                                  INCLUDE_CSD_SIZE);

      if (info->resize_width > 0)
        *width = resize_width_csd;
      if (info->resize_height > 0)
        *height = resize_height_csd;
    }

  /* Don't ever request zero width or height, it's not supported by
     cdk. The size allocation code will round it to 1 anyway but if
     we do it then the value returned from this function will is
     not comparable to the size allocation read from the CtkWindow. */
  *width = MAX (*width, 1);
  *height = MAX (*height, 1);
}

static CtkWindowPosition
get_effective_position (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;
  CtkWindowPosition pos = priv->position;

  if (pos == CTK_WIN_POS_CENTER_ON_PARENT &&
      (priv->transient_parent == NULL ||
       !_ctk_widget_get_mapped (CTK_WIDGET (priv->transient_parent))))
    pos = CTK_WIN_POS_NONE;

  return pos;
}

static CdkMonitor *
get_center_monitor_of_window (CtkWindow *window)
{
  CdkDisplay *display;

  /* We could try to sort out the relative positions of the monitors and
   * stuff, or we could just be losers and assume you have a row
   * or column of monitors.
   */
  display = cdk_screen_get_display (ctk_window_check_screen (window));
  return cdk_display_get_monitor (display, cdk_display_get_n_monitors (display) / 2);
}

static CdkMonitor *
get_monitor_containing_pointer (CtkWindow *window)
{
  gint px, py;
  CdkDisplay *display;
  CdkDevice *pointer;

  display = cdk_screen_get_display (ctk_window_check_screen (window));
  pointer = cdk_seat_get_pointer (cdk_display_get_default_seat (display));
  cdk_device_get_position (pointer, NULL, &px, &py);

  return cdk_display_get_monitor_at_point (display, px, py);
}

static void
center_window_on_monitor (CtkWindow *window,
                          gint       w,
                          gint       h,
                          gint      *x,
                          gint      *y)
{
  CdkRectangle area;
  CdkMonitor *monitor;

  monitor = get_monitor_containing_pointer (window);

  if (monitor == NULL)
    monitor = get_center_monitor_of_window (window);

 cdk_monitor_get_workarea (monitor, &area);

  *x = (area.width - w) / 2 + area.x;
  *y = (area.height - h) / 2 + area.y;

  /* Be sure we aren't off the monitor, ignoring _NET_WM_STRUT
   * and WM decorations.
   */
  if (*x < area.x)
    *x = area.x;
  if (*y < area.y)
    *y = area.y;
}

static void
clamp (gint *base,
       gint  extent,
       gint  clamp_base,
       gint  clamp_extent)
{
  if (extent > clamp_extent)
    /* Center */
    *base = clamp_base + clamp_extent/2 - extent/2;
  else if (*base < clamp_base)
    *base = clamp_base;
  else if (*base + extent > clamp_base + clamp_extent)
    *base = clamp_base + clamp_extent - extent;
}

static void
clamp_window_to_rectangle (gint               *x,
                           gint               *y,
                           gint                w,
                           gint                h,
                           const CdkRectangle *rect)
{
  /* If it is too large, center it. If it fits on the monitor but is
   * partially outside, move it to the closest edge. Do this
   * separately in x and y directions.
   */
  clamp (x, w, rect->x, rect->width);
  clamp (y, h, rect->y, rect->height);
}


static void
ctk_window_compute_configure_request (CtkWindow    *window,
                                      CdkRectangle *request,
                                      CdkGeometry  *geometry,
                                      guint        *flags)
{
  CtkWindowPrivate *priv = window->priv;
  CdkGeometry new_geometry;
  guint new_flags;
  int w, h;
  CtkWindowPosition pos;
  CtkWidget *parent_widget;
  CtkWindowGeometryInfo *info;
  CdkScreen *screen;
  int x, y;

  screen = ctk_window_check_screen (window);

  ctk_window_compute_hints (window, &new_geometry, &new_flags);
  ctk_window_compute_configure_request_size (window,
                                             &new_geometry, new_flags,
                                             &w, &h);
  ctk_window_update_fixed_size (window, &new_geometry, w, h);
  ctk_window_constrain_size (window,
                             &new_geometry, new_flags,
                             w, h,
                             &w, &h);

  parent_widget = (CtkWidget*) priv->transient_parent;

  pos = get_effective_position (window);
  info = ctk_window_get_geometry_info (window, FALSE);

  /* by default, don't change position requested */
  if (info)
    {
      x = info->last.configure_request.x;
      y = info->last.configure_request.y;
    }
  else
    {
      x = 0;
      y = 0;
    }


  if (priv->need_default_position)
    {

      /* FIXME this all interrelates with window gravity.
       * For most of them I think we want to set GRAVITY_CENTER.
       *
       * Not sure how to go about that.
       */
      switch (pos)
        {
          /* here we are only handling CENTER_ALWAYS
           * as it relates to default positioning,
           * where it's equivalent to simply CENTER
           */
        case CTK_WIN_POS_CENTER_ALWAYS:
        case CTK_WIN_POS_CENTER:
          center_window_on_monitor (window, w, h, &x, &y);
          break;

        case CTK_WIN_POS_CENTER_ON_PARENT:
          {
            CdkDisplay *display;
            CtkAllocation allocation;
            CdkWindow *cdk_window;
            CdkMonitor *monitor;
            CdkRectangle area;
            gint ox, oy;

            g_assert (_ctk_widget_get_mapped (parent_widget)); /* established earlier */

            display = cdk_screen_get_display (screen);
            cdk_window = _ctk_widget_get_window (parent_widget);
            monitor = cdk_display_get_monitor_at_window (display, cdk_window);

            cdk_window_get_origin (cdk_window, &ox, &oy);

            _ctk_widget_get_allocation (parent_widget, &allocation);
            x = ox + (allocation.width - w) / 2;
            y = oy + (allocation.height - h) / 2;

            /* Clamp onto current monitor, ignoring _NET_WM_STRUT and
             * WM decorations. If parent wasn't on a monitor, just
             * give up.
             */
            if (monitor != NULL)
              {
                cdk_monitor_get_geometry (monitor, &area);
                clamp_window_to_rectangle (&x, &y, w, h, &area);
              }
          }
          break;

        case CTK_WIN_POS_MOUSE:
          {
            CdkRectangle area;
            CdkDisplay *display;
            CdkDevice *pointer;
            CdkMonitor *monitor;
            gint px, py;

            display = cdk_screen_get_display (screen);
            pointer = cdk_seat_get_pointer (cdk_display_get_default_seat (display));

            cdk_device_get_position (pointer, NULL, &px, &py);
            monitor = cdk_display_get_monitor_at_point (display, px, py);

            x = px - w / 2;
            y = py - h / 2;

            /* Clamp onto current monitor, ignoring _NET_WM_STRUT and
             * WM decorations.
             */
            cdk_monitor_get_geometry (monitor, &area);
            clamp_window_to_rectangle (&x, &y, w, h, &area);
          }
          break;

        default:
          break;
        }
    } /* if (priv->need_default_position) */

  if (priv->need_default_position && info &&
      info->initial_pos_set)
    {
      x = info->initial_x;
      y = info->initial_y;
      ctk_window_constrain_position (window, w, h, &x, &y);
    }

  request->x = x;
  request->y = y;
  request->width = w;
  request->height = h;

  if (geometry)
    *geometry = new_geometry;
  if (flags)
    *flags = new_flags;
}

static void
ctk_window_constrain_position (CtkWindow    *window,
                               gint          new_width,
                               gint          new_height,
                               gint         *x,
                               gint         *y)
{
  CtkWindowPrivate *priv = window->priv;

  /* See long comments in ctk_window_move_resize()
   * on when it's safe to call this function.
   */
  if (priv->position == CTK_WIN_POS_CENTER_ALWAYS)
    {
      gint center_x, center_y;

      center_window_on_monitor (window, new_width, new_height, &center_x, &center_y);
      
      *x = center_x;
      *y = center_y;
    }
}

void
ctk_window_move_resize (CtkWindow *window)
{
  /* Overview:
   *
   * First we determine whether any information has changed that would
   * cause us to revise our last configure request.  If we would send
   * a different configure request from last time, then
   * configure_request_size_changed = TRUE or
   * configure_request_pos_changed = TRUE. configure_request_size_changed
   * may be true due to new hints, a ctk_window_resize(), or whatever.
   * configure_request_pos_changed may be true due to ctk_window_set_position()
   * or ctk_window_move().
   *
   * If the configure request has changed, we send off a new one.  To
   * ensure CTK+ invariants are maintained (resize queue does what it
   * should), we go ahead and size_allocate the requested size in this
   * function.
   *
   * If the configure request has not changed, we don't ever resend
   * it, because it could mean fighting the user or window manager.
   *
   *   To prepare the configure request, we come up with a base size/pos:
   *      - the one from ctk_window_move()/ctk_window_resize()
   *      - else default_width, default_height if we haven't ever
   *        been mapped
   *      - else the size request if we haven't ever been mapped,
   *        as a substitute default size
   *      - else the current size of the window, as received from
   *        configure notifies (i.e. the current allocation)
   *
   *   If CTK_WIN_POS_CENTER_ALWAYS is active, we constrain
   *   the position request to be centered.
   */
  CtkWindowPrivate *priv = window->priv;
  CtkWidget *widget;
  CtkContainer *container;
  CtkWindowGeometryInfo *info;
  CdkGeometry new_geometry;
  CdkWindow *cdk_window;
  guint new_flags;
  CdkRectangle new_request;
  gboolean configure_request_size_changed;
  gboolean configure_request_pos_changed;
  gboolean hints_changed; /* do we need to send these again */
  CtkWindowLastGeometryInfo saved_last_info;
  int current_width, current_height;

  widget = CTK_WIDGET (window);

  cdk_window = _ctk_widget_get_window (widget);
  container = CTK_CONTAINER (widget);
  info = ctk_window_get_geometry_info (window, TRUE);
  
  configure_request_size_changed = FALSE;
  configure_request_pos_changed = FALSE;
  
  ctk_window_compute_configure_request (window, &new_request,
                                        &new_geometry, &new_flags);  
  
  /* This check implies the invariant that we never set info->last
   * without setting the hints and sending off a configure request.
   *
   * If we change info->last without sending the request, we may
   * miss a request.
   */
  if (info->last.configure_request.x != new_request.x ||
      info->last.configure_request.y != new_request.y)
    configure_request_pos_changed = TRUE;

  if (priv->force_resize ||
      (info->last.configure_request.width != new_request.width ||
       info->last.configure_request.height != new_request.height))
    {
      priv->force_resize = FALSE;
      configure_request_size_changed = TRUE;
    }
  
  hints_changed = FALSE;
  
  if (!ctk_window_compare_hints (&info->last.geometry, info->last.flags,
				 &new_geometry, new_flags))
    {
      hints_changed = TRUE;
    }
  
  /* Position Constraints
   * ====================
   * 
   * POS_CENTER_ALWAYS is conceptually a constraint rather than
   * a default. The other POS_ values are used only when the
   * window is shown, not after that.
   * 
   * However, we can't implement a position constraint as
   * "anytime the window size changes, center the window"
   * because this may well end up fighting the WM or user.  In
   * fact it gets in an infinite loop with at least one WM.
   *
   * Basically, applications are in no way in a position to
   * constrain the position of a window, with one exception:
   * override redirect windows. (Really the intended purpose
   * of CENTER_ALWAYS anyhow, I would think.)
   *
   * So the way we implement this "constraint" is to say that when WE
   * cause a move or resize, i.e. we make a configure request changing
   * window size, we recompute the CENTER_ALWAYS position to reflect
   * the new window size, and include it in our request.  Also, if we
   * just turned on CENTER_ALWAYS we snap to center with a new
   * request.  Otherwise, if we are just NOTIFIED of a move or resize
   * done by someone else e.g. the window manager, we do NOT send a
   * new configure request.
   *
   * For override redirect windows, this works fine; all window
   * sizes are from our configure requests. For managed windows,
   * it is at least semi-sane, though who knows what the
   * app author is thinking.
   */

  /* This condition should be kept in sync with the condition later on
   * that determines whether we send a configure request.  i.e. we
   * should do this position constraining anytime we were going to
   * send a configure request anyhow, plus when constraints have
   * changed.
   */
  if (configure_request_pos_changed ||
      configure_request_size_changed ||
      hints_changed ||
      info->position_constraints_changed)
    {
      /* We request the constrained position if:
       *  - we were changing position, and need to clamp
       *    the change to the constraint
       *  - we're changing the size anyway
       *  - set_position() was called to toggle CENTER_ALWAYS on
       */

      ctk_window_constrain_position (window,
                                     new_request.width,
                                     new_request.height,
                                     &new_request.x,
                                     &new_request.y);
      
      /* Update whether we need to request a move */
      if (info->last.configure_request.x != new_request.x ||
          info->last.configure_request.y != new_request.y)
        configure_request_pos_changed = TRUE;
      else
        configure_request_pos_changed = FALSE;
    }

#if 0
  if (priv->type == CTK_WINDOW_TOPLEVEL)
    {
      CtkAllocation alloc;

      ctk_widget_get_allocation (widget, &alloc);

      g_message ("--- %s ---\n"
		 "last  : %d,%d\t%d x %d\n"
		 "this  : %d,%d\t%d x %d\n"
		 "alloc : %d,%d\t%d x %d\n"
		 "resize:      \t%d x %d\n" 
		 "size_changed: %d pos_changed: %d hints_changed: %d\n"
		 "configure_notify_received: %d\n"
		 "configure_request_count: %d\n"
		 "position_constraints_changed: %d",
		 priv->title ? priv->title : "(no title)",
		 info->last.configure_request.x,
		 info->last.configure_request.y,
		 info->last.configure_request.width,
		 info->last.configure_request.height,
		 new_request.x,
		 new_request.y,
		 new_request.width,
		 new_request.height,
		 alloc.x,
		 alloc.y,
		 alloc.width,
		 alloc.height,
		 info->resize_width,
		 info->resize_height,
		 configure_request_size_changed,
		 configure_request_pos_changed,
		 hints_changed,
		 priv->configure_notify_received,
		 priv->configure_request_count,
		 info->position_constraints_changed);
    }
#endif
  
  saved_last_info = info->last;
  info->last.geometry = new_geometry;
  info->last.flags = new_flags;
  info->last.configure_request = new_request;
  
  /* need to set PPosition so the WM will look at our position,
   * but we don't want to count PPosition coming and going as a hints
   * change for future iterations. So we saved info->last prior to
   * this.
   */
  
  /* Also, if the initial position was explicitly set, then we always
   * toggle on PPosition. This makes ctk_window_move(window, 0, 0)
   * work.
   */

  /* Also, we toggle on PPosition if CTK_WIN_POS_ is in use and
   * this is an initial map
   */
  
  if ((configure_request_pos_changed ||
       info->initial_pos_set ||
       (priv->need_default_position &&
        get_effective_position (window) != CTK_WIN_POS_NONE)) &&
      (new_flags & CDK_HINT_POS) == 0)
    {
      new_flags |= CDK_HINT_POS;
      hints_changed = TRUE;
    }
  
  /* Set hints if necessary
   */
  if (hints_changed)
    cdk_window_set_geometry_hints (cdk_window,
				   &new_geometry,
				   new_flags);

  current_width = cdk_window_get_width (cdk_window);
  current_height = cdk_window_get_height (cdk_window);

  /* handle resizing/moving and widget tree allocation
   */
  if (priv->configure_notify_received)
    { 
      CtkAllocation allocation;

      /* If we have received a configure event since
       * the last time in this function, we need to
       * accept our new size and size_allocate child widgets.
       * (see ctk_window_configure_event() for more details).
       *
       * 1 or more configure notifies may have been received.
       * Also, configure_notify_received will only be TRUE
       * if all expected configure notifies have been received
       * (one per configure request), as an optimization.
       *
       */
      priv->configure_notify_received = FALSE;

      allocation.x = 0;
      allocation.y = 0;
      allocation.width = current_width;
      allocation.height = current_height;

      ctk_widget_size_allocate (widget, &allocation);

      /* If the configure request changed, it means that
       * we either:
       *   1) coincidentally changed hints or widget properties
       *      impacting the configure request before getting
       *      a configure notify, or
       *   2) some broken widget is changing its size request
       *      during size allocation, resulting in
       *      a false appearance of changed configure request.
       *
       * For 1), we could just go ahead and ask for the
       * new size right now, but doing that for 2)
       * might well be fighting the user (and can even
       * trigger a loop). Since we really don't want to
       * do that, we requeue a resize in hopes that
       * by the time it gets handled, the child has seen
       * the light and is willing to go along with the
       * new size. (this happens for the zvt widget, since
       * the size_allocate() above will have stored the
       * requisition corresponding to the new size in the
       * zvt widget)
       *
       * This doesn't buy us anything for 1), but it shouldn't
       * hurt us too badly, since it is what would have
       * happened if we had gotten the configure event before
       * the new size had been set.
       */

      if (configure_request_size_changed ||
          configure_request_pos_changed)
        {
          /* Don't change the recorded last info after all, because we
           * haven't actually updated to the new info yet - we decided
           * to postpone our configure request until later.
           */
	  info->last = saved_last_info;
	  ctk_widget_queue_resize_no_redraw (widget); /* might recurse for CTK_RESIZE_IMMEDIATE */
	}

      return;			/* Bail out, we didn't really process the move/resize */
    }
  else if ((configure_request_size_changed || hints_changed) &&
           (current_width != new_request.width || current_height != new_request.height))
    {
      /* We are in one of the following situations:
       * A. configure_request_size_changed
       *    our requisition has changed and we need a different window size,
       *    so we request it from the window manager.
       * B. !configure_request_size_changed && hints_changed
       *    the window manager rejects our size, but we have just changed the
       *    window manager hints, so there's a chance our request will
       *    be honoured this time, so we try again.
       *
       * However, if the new requisition is the same as the current allocation,
       * we don't request it again, since we won't get a ConfigureNotify back from
       * the window manager unless it decides to change our requisition. If
       * we don't get the ConfigureNotify back, the resize queue will never be run.
       */

      /* Now send the configure request */
      if (configure_request_pos_changed)
        {
          cdk_window_move_resize (cdk_window,
                                  new_request.x, new_request.y,
                                  new_request.width, new_request.height);
        }
      else  /* only size changed */
        {
          cdk_window_resize (cdk_window,
                             new_request.width, new_request.height);
        }

      if (priv->type == CTK_WINDOW_POPUP)
        {
	  CtkAllocation allocation;

	  /* Directly size allocate for override redirect (popup) windows. */
          allocation.x = 0;
	  allocation.y = 0;
	  allocation.width = new_request.width;
	  allocation.height = new_request.height;

	  ctk_widget_size_allocate (widget, &allocation);

          G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
	  if (ctk_container_get_resize_mode (container) == CTK_RESIZE_QUEUE)
	    ctk_widget_queue_draw (widget);
          G_GNUC_END_IGNORE_DEPRECATIONS;
	}
      else
        {
	  /* Increment the number of have-not-yet-received-notify requests */
	  priv->configure_request_count += 1;

          CDK_PRIVATE_CALL (cdk_window_freeze_toplevel_updates) (cdk_window);

	  /* for CTK_RESIZE_QUEUE toplevels, we are now awaiting a new
	   * configure event in response to our resizing request.
	   * the configure event will cause a new resize with
	   * ->configure_notify_received=TRUE.
	   * until then, we want to
	   * - discard expose events
	   * - coalesce resizes for our children
	   * - defer any window resizes until the configure event arrived
	   * to achieve this, we queue a resize for the window, but remove its
	   * resizing handler, so resizing will not be handled from the next
	   * idle handler but when the configure event arrives.
	   *
	   * FIXME: we should also dequeue the pending redraws here, since
	   * we handle those ourselves upon ->configure_notify_received==TRUE.
	   */
	}
    }
  else
    {
      CtkAllocation allocation;

      /* Handle any position changes.
       */
      if (configure_request_pos_changed)
        {
          cdk_window_move (cdk_window,
                           new_request.x, new_request.y);
        }

      /* Our configure request didn't change size, but maybe some of
       * our child widgets have. Run a size allocate with our current
       * size to make sure that we re-layout our child widgets. */
      allocation.x = 0;
      allocation.y = 0;
      allocation.width = current_width;
      allocation.height = current_height;

      ctk_widget_size_allocate (widget, &allocation);
    }
  
  /* We have now processed a move/resize since the last position
   * constraint change, setting of the initial position, or resize.
   * (Not resetting these flags here can lead to infinite loops for
   * CTK_RESIZE_IMMEDIATE containers)
   */
  info->position_constraints_changed = FALSE;
  info->initial_pos_set = FALSE;
  info->resize_width = -1;
  info->resize_height = -1;
}

/* Compare two sets of Geometry hints for equality.
 */
static gboolean
ctk_window_compare_hints (CdkGeometry *geometry_a,
			  guint        flags_a,
			  CdkGeometry *geometry_b,
			  guint        flags_b)
{
  if (flags_a != flags_b)
    return FALSE;
  
  if ((flags_a & CDK_HINT_MIN_SIZE) &&
      (geometry_a->min_width != geometry_b->min_width ||
       geometry_a->min_height != geometry_b->min_height))
    return FALSE;

  if ((flags_a & CDK_HINT_MAX_SIZE) &&
      (geometry_a->max_width != geometry_b->max_width ||
       geometry_a->max_height != geometry_b->max_height))
    return FALSE;

  if ((flags_a & CDK_HINT_BASE_SIZE) &&
      (geometry_a->base_width != geometry_b->base_width ||
       geometry_a->base_height != geometry_b->base_height))
    return FALSE;

  if ((flags_a & CDK_HINT_ASPECT) &&
      (geometry_a->min_aspect != geometry_b->min_aspect ||
       geometry_a->max_aspect != geometry_b->max_aspect))
    return FALSE;

  if ((flags_a & CDK_HINT_RESIZE_INC) &&
      (geometry_a->width_inc != geometry_b->width_inc ||
       geometry_a->height_inc != geometry_b->height_inc))
    return FALSE;

  if ((flags_a & CDK_HINT_WIN_GRAVITY) &&
      geometry_a->win_gravity != geometry_b->win_gravity)
    return FALSE;

  return TRUE;
}

static void 
ctk_window_constrain_size (CtkWindow   *window,
			   CdkGeometry *geometry,
			   guint        flags,
			   gint         width,
			   gint         height,
			   gint        *new_width,
			   gint        *new_height)
{
  CtkWindowPrivate *priv = window->priv;
  guint geometry_flags;

  /* ignore size increments for windows that fit in a fixed space */
  if (priv->maximized || priv->fullscreen || priv->tiled)
    geometry_flags = flags & ~CDK_HINT_RESIZE_INC;
  else
    geometry_flags = flags;

  cdk_window_constrain_size (geometry, geometry_flags, width, height,
                             new_width, new_height);
}

/* For non-resizable windows, make sure the given width/height fits
 * in the geometry contrains and update the geometry hints to match
 * the given width/height if not.
 * This is to make sure that non-resizable windows get the default
 * width/height if set, but can still grow if their content requires.
 *
 * Note: Fixed size windows with a default size set will not shrink
 * smaller than the default size when their content requires less size.
 */
static void
ctk_window_update_fixed_size (CtkWindow   *window,
                              CdkGeometry *new_geometry,
                              gint         new_width,
                              gint         new_height)
{
  CtkWindowPrivate *priv = window->priv;
  CtkWindowGeometryInfo *info;
  gboolean has_size_request;

  /* Adjust the geometry hints for non-resizable windows only */
  has_size_request = ctk_widget_has_size_request (CTK_WIDGET (window));
  if (priv->resizable || has_size_request)
    return;

  info = ctk_window_get_geometry_info (window, FALSE);
  if (info)
    {
      gint default_width_csd = info->default_width;
      gint default_height_csd = info->default_height;

      ctk_window_update_csd_size (window,
                                  &default_width_csd, &default_height_csd,
                                  INCLUDE_CSD_SIZE);

      if (info->default_width > -1)
        {
          gint w = MAX (MAX (default_width_csd, new_width), new_geometry->min_width);
          new_geometry->min_width = w;
          new_geometry->max_width = w;
        }

      if (info->default_height > -1)
        {
          gint h = MAX (MAX (default_height_csd, new_height), new_geometry->min_height);
          new_geometry->min_height = h;
          new_geometry->max_height = h;
        }
    }
}

/* Compute the set of geometry hints and flags for a window
 * based on the application set geometry, and requisition
 * of the window. ctk_widget_get_preferred_size() must have been
 * called first.
 */
static void
ctk_window_compute_hints (CtkWindow   *window,
			  CdkGeometry *new_geometry,
			  guint       *new_flags)
{
  CtkWindowPrivate *priv = window->priv;
  CtkWidget *widget;
  gint extra_width = 0;
  gint extra_height = 0;
  CtkWindowGeometryInfo *geometry_info;
  CtkRequisition requisition;

  widget = CTK_WIDGET (window);

  ctk_widget_get_preferred_size (widget, &requisition, NULL);
  geometry_info = ctk_window_get_geometry_info (CTK_WINDOW (widget), FALSE);

  if (geometry_info)
    {
      *new_flags = geometry_info->mask;
      *new_geometry = geometry_info->geometry;
    }
  else
    {
      *new_flags = 0;
    }
  
  /* We don't want to set CDK_HINT_POS in here, we just set it
   * in ctk_window_move_resize() when we want the position
   * honored.
   */
  
  if (*new_flags & CDK_HINT_BASE_SIZE)
    {
      new_geometry->base_width += extra_width;
      new_geometry->base_height += extra_height;
    }
  else
    {
      /* For simplicity, we always set the base hint, even when we
       * don't expect it to have any visible effect.
       * (Note: geometry_size_to_pixels() depends on this.)
       */
      *new_flags |= CDK_HINT_BASE_SIZE;

      new_geometry->base_width = extra_width;
      new_geometry->base_height = extra_height;

      /* As for X, if BASE_SIZE is not set but MIN_SIZE is set, then the
       * base size is the minimum size */
      if (*new_flags & CDK_HINT_MIN_SIZE)
	{
	  if (new_geometry->min_width > 0)
	    new_geometry->base_width += new_geometry->min_width;
	  if (new_geometry->min_height > 0)
	    new_geometry->base_height += new_geometry->min_height;
	}
    }

  /* Please use a good size for unresizable widgets, not the minimum one. */
  if (!priv->resizable)
    ctk_window_guess_default_size (window, &requisition.width, &requisition.height);

  if (*new_flags & CDK_HINT_MIN_SIZE)
    {
      if (new_geometry->min_width < 0)
	new_geometry->min_width = requisition.width;
      else
        new_geometry->min_width = MAX (requisition.width, new_geometry->min_width + extra_width);

      if (new_geometry->min_height < 0)
	new_geometry->min_height = requisition.height;
      else
	new_geometry->min_height = MAX (requisition.height, new_geometry->min_height + extra_height);
    }
  else
    {
      *new_flags |= CDK_HINT_MIN_SIZE;
      
      new_geometry->min_width = requisition.width;
      new_geometry->min_height = requisition.height;
    }
  
  if (*new_flags & CDK_HINT_MAX_SIZE)
    {
      if (new_geometry->max_width >= 0)
	new_geometry->max_width += extra_width;
      new_geometry->max_width = MAX (new_geometry->max_width, new_geometry->min_width);

      if (new_geometry->max_height >= 0)
	new_geometry->max_height += extra_height;

      new_geometry->max_height = MAX (new_geometry->max_height, new_geometry->min_height);
    }
  else if (!priv->resizable)
    {
      *new_flags |= CDK_HINT_MAX_SIZE;

      new_geometry->max_width = new_geometry->min_width;
      new_geometry->max_height = new_geometry->min_height;
    }

  *new_flags |= CDK_HINT_WIN_GRAVITY;
  new_geometry->win_gravity = priv->gravity;
}

#undef INCLUDE_CSD_SIZE
#undef EXCLUDE_CSD_SIZE

/***********************
 * Redrawing functions *
 ***********************/

static gboolean
ctk_window_draw (CtkWidget *widget,
		 cairo_t   *cr)
{
  CtkWindowPrivate *priv = CTK_WINDOW (widget)->priv;
  CtkStyleContext *context;
  gboolean ret = FALSE;
  CtkAllocation allocation;
  CtkBorder window_border;
  gint title_height;

  context = ctk_widget_get_style_context (widget);

  get_shadow_width (CTK_WINDOW (widget), &window_border);
  _ctk_widget_get_allocation (widget, &allocation);

  if (ctk_cairo_should_draw_window (cr, _ctk_widget_get_window (widget)))
    {
      if (priv->client_decorated &&
          priv->decorated &&
          !priv->fullscreen &&
          !priv->maximized)
        {
          ctk_style_context_save_to_node (context, priv->decoration_node);

          if (priv->use_client_shadow)
            {
              CtkBorder padding, border;

              ctk_style_context_get_padding (context, ctk_style_context_get_state (context), &padding);
              ctk_style_context_get_border (context, ctk_style_context_get_state (context), &border);
              sum_borders (&border, &padding);

              ctk_render_background (context, cr,
                                     window_border.left - border.left, window_border.top - border.top,
                                     allocation.width -
                                     (window_border.left + window_border.right - border.left - border.right),
                                     allocation.height -
                                     (window_border.top + window_border.bottom - border.top - border.bottom));
              ctk_render_frame (context, cr,
                                window_border.left - border.left, window_border.top - border.top,
                                allocation.width -
                                (window_border.left + window_border.right - border.left - border.right),
                                allocation.height -
                                (window_border.top + window_border.bottom - border.top - border.bottom));
            }
          else
            {
              ctk_render_background (context, cr, 0, 0,
                                     allocation.width,
                                     allocation.height);

              ctk_render_frame (context, cr, 0, 0,
                                allocation.width,
                                allocation.height);
            }

          ctk_style_context_restore (context);
        }

      if (!ctk_widget_get_app_paintable (widget))
        {
           if (priv->title_box &&
               ctk_widget_get_visible (priv->title_box) &&
               ctk_widget_get_child_visible (priv->title_box))
             title_height = priv->title_height;
           else
             title_height = 0;

           ctk_render_background (context, cr,
                                  window_border.left,
                                  window_border.top + title_height,
                                  allocation.width -
                                  (window_border.left + window_border.right),
                                  allocation.height -
                                  (window_border.top + window_border.bottom +
                                   title_height));
           ctk_render_frame (context, cr,
                             window_border.left,
                             window_border.top + title_height,
                             allocation.width -
                             (window_border.left + window_border.right),
                             allocation.height -
                             (window_border.top + window_border.bottom +
                              title_height));
        }
    }

  if (CTK_WIDGET_CLASS (ctk_window_parent_class)->draw)
    ret = CTK_WIDGET_CLASS (ctk_window_parent_class)->draw (widget, cr);

  return ret;
}

/**
 * ctk_window_present:
 * @window: a #CtkWindow
 *
 * Presents a window to the user. This function should not be used
 * as when it is called, it is too late to gather a valid timestamp
 * to allow focus stealing prevention to work correctly.
 **/
void
ctk_window_present (CtkWindow *window)
{
  ctk_window_present_with_time (window, CDK_CURRENT_TIME);
}

/**
 * ctk_window_present_with_time:
 * @window: a #CtkWindow
 * @timestamp: the timestamp of the user interaction (typically a 
 *   button or key press event) which triggered this call
 *
 * Presents a window to the user. This may mean raising the window
 * in the stacking order, deiconifying it, moving it to the current
 * desktop, and/or giving it the keyboard focus, possibly dependent
 * on the user’s platform, window manager, and preferences.
 *
 * If @window is hidden, this function calls ctk_widget_show()
 * as well.
 *
 * This function should be used when the user tries to open a window
 * that’s already open. Say for example the preferences dialog is
 * currently open, and the user chooses Preferences from the menu
 * a second time; use ctk_window_present() to move the already-open dialog
 * where the user can see it.
 *
 * Presents a window to the user in response to a user interaction. The
 * timestamp should be gathered when the window was requested to be shown
 * (when clicking a link for example), rather than once the window is
 * ready to be shown.
 *
 * Since: 2.8
 **/
void
ctk_window_present_with_time (CtkWindow *window,
			      guint32    timestamp)
{
  CtkWindowPrivate *priv;
  CtkWidget *widget;
  CdkWindow *cdk_window;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;
  widget = CTK_WIDGET (window);

  if (ctk_widget_get_visible (widget))
    {
      cdk_window = _ctk_widget_get_window (widget);

      g_assert (cdk_window != NULL);

      cdk_window_show (cdk_window);

      /* Translate a timestamp of CDK_CURRENT_TIME appropriately */
      if (timestamp == CDK_CURRENT_TIME)
        {
#ifdef CDK_WINDOWING_X11
	  if (CDK_IS_X11_WINDOW(cdk_window))
	    {
	      CdkDisplay *display;

	      display = ctk_widget_get_display (widget);
	      timestamp = cdk_x11_display_get_user_time (display);
	    }
	  else
#endif
	    timestamp = ctk_get_current_event_time ();
        }

      cdk_window_focus (cdk_window, timestamp);
    }
  else
    {
      priv->initial_timestamp = timestamp;
      ctk_widget_show (widget);
    }
}

/**
 * ctk_window_iconify:
 * @window: a #CtkWindow
 *
 * Asks to iconify (i.e. minimize) the specified @window. Note that
 * you shouldn’t assume the window is definitely iconified afterward,
 * because other entities (e.g. the user or
 * [window manager][ctk-X11-arch]) could deiconify it
 * again, or there may not be a window manager in which case
 * iconification isn’t possible, etc. But normally the window will end
 * up iconified. Just don’t write code that crashes if not.
 *
 * It’s permitted to call this function before showing a window,
 * in which case the window will be iconified before it ever appears
 * onscreen.
 *
 * You can track iconification via the “window-state-event” signal
 * on #CtkWidget.
 **/
void
ctk_window_iconify (CtkWindow *window)
{
  CdkWindow *toplevel;

  g_return_if_fail (CTK_IS_WINDOW (window));

  window->priv->iconify_initially = TRUE;

  toplevel = _ctk_widget_get_window (CTK_WIDGET (window));

  if (toplevel != NULL)
    cdk_window_iconify (toplevel);
}

/**
 * ctk_window_deiconify:
 * @window: a #CtkWindow
 *
 * Asks to deiconify (i.e. unminimize) the specified @window. Note
 * that you shouldn’t assume the window is definitely deiconified
 * afterward, because other entities (e.g. the user or
 * [window manager][ctk-X11-arch])) could iconify it
 * again before your code which assumes deiconification gets to run.
 *
 * You can track iconification via the “window-state-event” signal
 * on #CtkWidget.
 **/
void
ctk_window_deiconify (CtkWindow *window)
{
  CdkWindow *toplevel;

  g_return_if_fail (CTK_IS_WINDOW (window));

  window->priv->iconify_initially = FALSE;

  toplevel = _ctk_widget_get_window (CTK_WIDGET (window));

  if (toplevel != NULL)
    cdk_window_deiconify (toplevel);
}

/**
 * ctk_window_stick:
 * @window: a #CtkWindow
 *
 * Asks to stick @window, which means that it will appear on all user
 * desktops. Note that you shouldn’t assume the window is definitely
 * stuck afterward, because other entities (e.g. the user or
 * [window manager][ctk-X11-arch] could unstick it
 * again, and some window managers do not support sticking
 * windows. But normally the window will end up stuck. Just don't
 * write code that crashes if not.
 *
 * It’s permitted to call this function before showing a window.
 *
 * You can track stickiness via the “window-state-event” signal
 * on #CtkWidget.
 **/
void
ctk_window_stick (CtkWindow *window)
{
  CdkWindow *toplevel;

  g_return_if_fail (CTK_IS_WINDOW (window));

  window->priv->stick_initially = TRUE;

  toplevel = _ctk_widget_get_window (CTK_WIDGET (window));

  if (toplevel != NULL)
    cdk_window_stick (toplevel);
}

/**
 * ctk_window_unstick:
 * @window: a #CtkWindow
 *
 * Asks to unstick @window, which means that it will appear on only
 * one of the user’s desktops. Note that you shouldn’t assume the
 * window is definitely unstuck afterward, because other entities
 * (e.g. the user or [window manager][ctk-X11-arch]) could
 * stick it again. But normally the window will
 * end up stuck. Just don’t write code that crashes if not.
 *
 * You can track stickiness via the “window-state-event” signal
 * on #CtkWidget.
 **/
void
ctk_window_unstick (CtkWindow *window)
{
  CdkWindow *toplevel;

  g_return_if_fail (CTK_IS_WINDOW (window));

  window->priv->stick_initially = FALSE;

  toplevel = _ctk_widget_get_window (CTK_WIDGET (window));

  if (toplevel != NULL)
    cdk_window_unstick (toplevel);
}

/**
 * ctk_window_maximize:
 * @window: a #CtkWindow
 *
 * Asks to maximize @window, so that it becomes full-screen. Note that
 * you shouldn’t assume the window is definitely maximized afterward,
 * because other entities (e.g. the user or
 * [window manager][ctk-X11-arch]) could unmaximize it
 * again, and not all window managers support maximization. But
 * normally the window will end up maximized. Just don’t write code
 * that crashes if not.
 *
 * It’s permitted to call this function before showing a window,
 * in which case the window will be maximized when it appears onscreen
 * initially.
 *
 * You can track maximization via the “window-state-event” signal
 * on #CtkWidget, or by listening to notifications on the
 * #CtkWindow:is-maximized property.
 **/
void
ctk_window_maximize (CtkWindow *window)
{
  CdkWindow *toplevel;

  g_return_if_fail (CTK_IS_WINDOW (window));

  window->priv->maximize_initially = TRUE;

  toplevel = _ctk_widget_get_window (CTK_WIDGET (window));

  if (toplevel != NULL)
    cdk_window_maximize (toplevel);
}

/**
 * ctk_window_unmaximize:
 * @window: a #CtkWindow
 *
 * Asks to unmaximize @window. Note that you shouldn’t assume the
 * window is definitely unmaximized afterward, because other entities
 * (e.g. the user or [window manager][ctk-X11-arch])
 * could maximize it again, and not all window
 * managers honor requests to unmaximize. But normally the window will
 * end up unmaximized. Just don’t write code that crashes if not.
 *
 * You can track maximization via the “window-state-event” signal
 * on #CtkWidget.
 **/
void
ctk_window_unmaximize (CtkWindow *window)
{
  CdkWindow *toplevel;

  g_return_if_fail (CTK_IS_WINDOW (window));

  window->priv->maximize_initially = FALSE;

  toplevel = _ctk_widget_get_window (CTK_WIDGET (window));

  if (toplevel != NULL)
    cdk_window_unmaximize (toplevel);
}

/**
 * ctk_window_fullscreen:
 * @window: a #CtkWindow
 *
 * Asks to place @window in the fullscreen state. Note that you
 * shouldn’t assume the window is definitely full screen afterward,
 * because other entities (e.g. the user or
 * [window manager][ctk-X11-arch]) could unfullscreen it
 * again, and not all window managers honor requests to fullscreen
 * windows. But normally the window will end up fullscreen. Just
 * don’t write code that crashes if not.
 *
 * You can track the fullscreen state via the “window-state-event” signal
 * on #CtkWidget.
 *
 * Since: 2.2
 **/
void
ctk_window_fullscreen (CtkWindow *window)
{
  CdkWindow *toplevel;

  g_return_if_fail (CTK_IS_WINDOW (window));

  window->priv->fullscreen_initially = TRUE;

  toplevel = _ctk_widget_get_window (CTK_WIDGET (window));

  if (toplevel != NULL)
    cdk_window_fullscreen (toplevel);
}

/**
 * ctk_window_fullscreen_on_monitor:
 * @window: a #CtkWindow
 * @screen: a #CdkScreen to draw to
 * @monitor: which monitor to go fullscreen on
 *
 * Asks to place @window in the fullscreen state. Note that you shouldn't assume
 * the window is definitely full screen afterward.
 *
 * You can track the fullscreen state via the "window-state-event" signal
 * on #CtkWidget.
 *
 * Since: 3.18
 */
void
ctk_window_fullscreen_on_monitor (CtkWindow *window,
                                  CdkScreen *screen,
                                  gint monitor)
{
  CtkWindowPrivate *priv;
  CtkWidget *widget;
  CdkWindow *toplevel;

  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (CDK_IS_SCREEN (screen));
  g_return_if_fail (cdk_display_get_monitor (cdk_screen_get_display (screen), monitor) != NULL);

  priv = window->priv;
  widget = CTK_WIDGET (window);

  ctk_window_set_screen (window, screen);

  priv->initial_fullscreen_monitor = monitor;
  priv->fullscreen_initially = TRUE;

  toplevel = _ctk_widget_get_window (widget);

  if (toplevel != NULL)
    cdk_window_fullscreen_on_monitor (toplevel, monitor);
}

/**
 * ctk_window_unfullscreen:
 * @window: a #CtkWindow
 *
 * Asks to toggle off the fullscreen state for @window. Note that you
 * shouldn’t assume the window is definitely not full screen
 * afterward, because other entities (e.g. the user or
 * [window manager][ctk-X11-arch]) could fullscreen it
 * again, and not all window managers honor requests to unfullscreen
 * windows. But normally the window will end up restored to its normal
 * state. Just don’t write code that crashes if not.
 *
 * You can track the fullscreen state via the “window-state-event” signal
 * on #CtkWidget.
 *
 * Since: 2.2
 **/
void
ctk_window_unfullscreen (CtkWindow *window)
{
  CdkWindow *toplevel;

  g_return_if_fail (CTK_IS_WINDOW (window));

  window->priv->initial_fullscreen_monitor = -1;
  window->priv->fullscreen_initially = FALSE;

  toplevel = _ctk_widget_get_window (CTK_WIDGET (window));

  if (toplevel != NULL)
    cdk_window_unfullscreen (toplevel);
}

/**
 * ctk_window_set_keep_above:
 * @window: a #CtkWindow
 * @setting: whether to keep @window above other windows
 *
 * Asks to keep @window above, so that it stays on top. Note that
 * you shouldn’t assume the window is definitely above afterward,
 * because other entities (e.g. the user or
 * [window manager][ctk-X11-arch]) could not keep it above,
 * and not all window managers support keeping windows above. But
 * normally the window will end kept above. Just don’t write code
 * that crashes if not.
 *
 * It’s permitted to call this function before showing a window,
 * in which case the window will be kept above when it appears onscreen
 * initially.
 *
 * You can track the above state via the “window-state-event” signal
 * on #CtkWidget.
 *
 * Note that, according to the
 * [Extended Window Manager Hints Specification](http://www.freedesktop.org/Standards/wm-spec),
 * the above state is mainly meant for user preferences and should not
 * be used by applications e.g. for drawing attention to their
 * dialogs.
 *
 * Since: 2.4
 **/
void
ctk_window_set_keep_above (CtkWindow *window,
			   gboolean   setting)
{
  CdkWindow *toplevel;

  g_return_if_fail (CTK_IS_WINDOW (window));

  setting = setting != FALSE;

  window->priv->above_initially = setting;
  window->priv->below_initially &= !setting;

  toplevel = _ctk_widget_get_window (CTK_WIDGET (window));

  if (toplevel != NULL)
    cdk_window_set_keep_above (toplevel, setting);
}

/**
 * ctk_window_set_keep_below:
 * @window: a #CtkWindow
 * @setting: whether to keep @window below other windows
 *
 * Asks to keep @window below, so that it stays in bottom. Note that
 * you shouldn’t assume the window is definitely below afterward,
 * because other entities (e.g. the user or
 * [window manager][ctk-X11-arch]) could not keep it below,
 * and not all window managers support putting windows below. But
 * normally the window will be kept below. Just don’t write code
 * that crashes if not.
 *
 * It’s permitted to call this function before showing a window,
 * in which case the window will be kept below when it appears onscreen
 * initially.
 *
 * You can track the below state via the “window-state-event” signal
 * on #CtkWidget.
 *
 * Note that, according to the
 * [Extended Window Manager Hints Specification](http://www.freedesktop.org/Standards/wm-spec),
 * the above state is mainly meant for user preferences and should not
 * be used by applications e.g. for drawing attention to their
 * dialogs.
 *
 * Since: 2.4
 **/
void
ctk_window_set_keep_below (CtkWindow *window,
			   gboolean   setting)
{
  CdkWindow *toplevel;

  g_return_if_fail (CTK_IS_WINDOW (window));

  setting = setting != FALSE;

  window->priv->below_initially = setting;
  window->priv->above_initially &= !setting;

  toplevel = _ctk_widget_get_window (CTK_WIDGET (window));

  if (toplevel != NULL)
    cdk_window_set_keep_below (toplevel, setting);
}

/**
 * ctk_window_set_resizable:
 * @window: a #CtkWindow
 * @resizable: %TRUE if the user can resize this window
 *
 * Sets whether the user can resize a window. Windows are user resizable
 * by default.
 **/
void
ctk_window_set_resizable (CtkWindow *window,
                          gboolean   resizable)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  resizable = (resizable != FALSE);

  if (priv->resizable != resizable)
    {
      priv->resizable = resizable;

      update_window_buttons (window);

      ctk_widget_queue_resize_no_redraw (CTK_WIDGET (window));

      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_RESIZABLE]);
    }
}

/**
 * ctk_window_get_resizable:
 * @window: a #CtkWindow
 *
 * Gets the value set by ctk_window_set_resizable().
 *
 * Returns: %TRUE if the user can resize the window
 **/
gboolean
ctk_window_get_resizable (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return window->priv->resizable;
}

/**
 * ctk_window_set_gravity:
 * @window: a #CtkWindow
 * @gravity: window gravity
 *
 * Window gravity defines the meaning of coordinates passed to
 * ctk_window_move(). See ctk_window_move() and #CdkGravity for
 * more details.
 *
 * The default window gravity is #CDK_GRAVITY_NORTH_WEST which will
 * typically “do what you mean.”
 *
 **/
void
ctk_window_set_gravity (CtkWindow *window,
			CdkGravity gravity)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  if (gravity != priv->gravity)
    {
      priv->gravity = gravity;

      /* ctk_window_move_resize() will adapt gravity
       */
      ctk_widget_queue_resize_no_redraw (CTK_WIDGET (window));

      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_GRAVITY]);
    }
}

/**
 * ctk_window_get_gravity:
 * @window: a #CtkWindow
 *
 * Gets the value set by ctk_window_set_gravity().
 *
 * Returns: (transfer none): window gravity
 **/
CdkGravity
ctk_window_get_gravity (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), 0);

  return window->priv->gravity;
}

/**
 * ctk_window_begin_resize_drag:
 * @window: a #CtkWindow
 * @button: mouse button that initiated the drag
 * @edge: position of the resize control
 * @root_x: X position where the user clicked to initiate the drag, in root window coordinates
 * @root_y: Y position where the user clicked to initiate the drag
 * @timestamp: timestamp from the click event that initiated the drag
 *
 * Starts resizing a window. This function is used if an application
 * has window resizing controls. When CDK can support it, the resize
 * will be done using the standard mechanism for the
 * [window manager][ctk-X11-arch] or windowing
 * system. Otherwise, CDK will try to emulate window resizing,
 * potentially not all that well, depending on the windowing system.
 */
void
ctk_window_begin_resize_drag  (CtkWindow     *window,
                               CdkWindowEdge  edge,
                               gint           button,
                               gint           root_x,
                               gint           root_y,
                               guint32        timestamp)
{
  CtkWidget *widget;
  CdkWindow *toplevel;

  g_return_if_fail (CTK_IS_WINDOW (window));
  widget = CTK_WIDGET (window);
  g_return_if_fail (ctk_widget_get_visible (widget));

  toplevel = _ctk_widget_get_window (widget);

  cdk_window_begin_resize_drag (toplevel,
                                edge, button,
                                root_x, root_y,
                                timestamp);
}

/**
 * ctk_window_begin_move_drag:
 * @window: a #CtkWindow
 * @button: mouse button that initiated the drag
 * @root_x: X position where the user clicked to initiate the drag, in root window coordinates
 * @root_y: Y position where the user clicked to initiate the drag
 * @timestamp: timestamp from the click event that initiated the drag
 *
 * Starts moving a window. This function is used if an application has
 * window movement grips. When CDK can support it, the window movement
 * will be done using the standard mechanism for the
 * [window manager][ctk-X11-arch] or windowing
 * system. Otherwise, CDK will try to emulate window movement,
 * potentially not all that well, depending on the windowing system.
 */
void
ctk_window_begin_move_drag  (CtkWindow *window,
                             gint       button,
                             gint       root_x,
                             gint       root_y,
                             guint32    timestamp)
{
  CtkWidget *widget;
  CdkWindow *toplevel;

  g_return_if_fail (CTK_IS_WINDOW (window));
  widget = CTK_WIDGET (window);
  g_return_if_fail (ctk_widget_get_visible (widget));

  toplevel = _ctk_widget_get_window (widget);

  cdk_window_begin_move_drag (toplevel,
                              button,
                              root_x, root_y,
                              timestamp);
}

/**
 * ctk_window_set_screen:
 * @window: a #CtkWindow.
 * @screen: a #CdkScreen.
 *
 * Sets the #CdkScreen where the @window is displayed; if
 * the window is already mapped, it will be unmapped, and
 * then remapped on the new screen.
 *
 * Since: 2.2
 */
void
ctk_window_set_screen (CtkWindow *window,
		       CdkScreen *screen)
{
  CtkWindowPrivate *priv;
  CtkWidget *widget;
  CdkScreen *previous_screen;
  gboolean was_rgba;
  gboolean was_mapped;

  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (CDK_IS_SCREEN (screen));

  priv = window->priv;

  if (screen == priv->screen)
    return;

  /* reset initial_fullscreen_monitor since they are relative to the screen */
  priv->initial_fullscreen_monitor = -1;
  
  widget = CTK_WIDGET (window);

  previous_screen = priv->screen;

  if (cdk_screen_get_rgba_visual (previous_screen) == ctk_widget_get_visual (widget))
    was_rgba = TRUE;
  else
    was_rgba = FALSE;

  was_mapped = _ctk_widget_get_mapped (widget);

  if (was_mapped)
    ctk_widget_unmap (widget);
  if (_ctk_widget_get_realized (widget))
    ctk_widget_unrealize (widget);

  ctk_window_free_key_hash (window);
  priv->screen = screen;
  if (screen != previous_screen)
    {
      if (previous_screen)
        {
          g_signal_handlers_disconnect_by_func (previous_screen,
                                                ctk_window_on_composited_changed, window);
#ifdef CDK_WINDOWING_X11
          g_signal_handlers_disconnect_by_func (ctk_settings_get_for_screen (previous_screen),
                                                ctk_window_on_theme_variant_changed, window);
#endif
        }
      g_signal_connect (screen, "composited-changed",
                        G_CALLBACK (ctk_window_on_composited_changed), window);
#ifdef CDK_WINDOWING_X11
      g_signal_connect (ctk_settings_get_for_screen (screen),
                        "notify::ctk-application-prefer-dark-theme",
                        G_CALLBACK (ctk_window_on_theme_variant_changed), window);
#endif

      _ctk_widget_propagate_screen_changed (widget, previous_screen);
      _ctk_widget_propagate_composited_changed (widget);
    }
  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_SCREEN]);

  if (was_rgba && priv->use_client_shadow)
    {
      CdkVisual *visual;

      visual = cdk_screen_get_rgba_visual (screen);
      if (visual)
        ctk_widget_set_visual (widget, visual);
    }

  if (was_mapped)
    ctk_widget_map (widget);

  check_scale_changed (window);
}

static void
ctk_window_set_theme_variant (CtkWindow *window)
{
#ifdef CDK_WINDOWING_X11
  CdkWindow *cdk_window;
  gboolean   dark_theme_requested;

  g_object_get (ctk_settings_get_for_screen (window->priv->screen),
                "ctk-application-prefer-dark-theme", &dark_theme_requested,
                NULL);

  cdk_window = _ctk_widget_get_window (CTK_WIDGET (window));

  if (CDK_IS_X11_WINDOW (cdk_window))
    cdk_x11_window_set_theme_variant (cdk_window,
                                      dark_theme_requested ? "dark" : NULL);
#endif
}

#ifdef CDK_WINDOWING_X11
static void
ctk_window_on_theme_variant_changed (CtkSettings *settings,
                                     GParamSpec  *pspec,
                                     CtkWindow   *window)
{
  if (window->priv->type == CTK_WINDOW_TOPLEVEL)
    ctk_window_set_theme_variant (window);
}
#endif

static void
ctk_window_on_composited_changed (CdkScreen *screen,
				  CtkWindow *window)
{
  CtkWidget *widget = CTK_WIDGET (window);

  ctk_widget_queue_draw (widget);
  _ctk_widget_propagate_composited_changed (widget);
}

static CdkScreen *
ctk_window_check_screen (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;

  if (priv->screen)
    return priv->screen;
  else
    {
      g_warning ("Screen for CtkWindow not set; you must always set\n"
		 "a screen for a CtkWindow before using the window");
      return NULL;
    }
}

/**
 * ctk_window_get_screen:
 * @window: a #CtkWindow.
 *
 * Returns the #CdkScreen associated with @window.
 *
 * Returns: (transfer none): a #CdkScreen.
 *
 * Since: 2.2
 */
CdkScreen*
ctk_window_get_screen (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), NULL);

  return window->priv->screen;
}

CdkScreen *
_ctk_window_get_screen (CtkWindow *window)
{
  return window->priv->screen;
}

/**
 * ctk_window_is_active:
 * @window: a #CtkWindow
 * 
 * Returns whether the window is part of the current active toplevel.
 * (That is, the toplevel window receiving keystrokes.)
 * The return value is %TRUE if the window is active toplevel
 * itself, but also if it is, say, a #CtkPlug embedded in the active toplevel.
 * You might use this function if you wanted to draw a widget
 * differently in an active window from a widget in an inactive window.
 * See ctk_window_has_toplevel_focus()
 * 
 * Returns: %TRUE if the window part of the current active window.
 *
 * Since: 2.4
 **/
gboolean
ctk_window_is_active (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return window->priv->is_active;
}

/**
 * ctk_window_has_toplevel_focus:
 * @window: a #CtkWindow
 * 
 * Returns whether the input focus is within this CtkWindow.
 * For real toplevel windows, this is identical to ctk_window_is_active(),
 * but for embedded windows, like #CtkPlug, the results will differ.
 * 
 * Returns: %TRUE if the input focus is within this CtkWindow
 *
 * Since: 2.4
 **/
gboolean
ctk_window_has_toplevel_focus (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return window->priv->has_toplevel_focus;
}

/**
 * ctk_window_get_group:
 * @window: (allow-none): a #CtkWindow, or %NULL
 *
 * Returns the group for @window or the default group, if
 * @window is %NULL or if @window does not have an explicit
 * window group.
 *
 * Returns: (transfer none): the #CtkWindowGroup for a window or the default group
 *
 * Since: 2.10
 */
CtkWindowGroup *
ctk_window_get_group (CtkWindow *window)
{
  if (window && window->priv->group)
    return window->priv->group;
  else
    {
      static CtkWindowGroup *default_group = NULL;

      if (!default_group)
	default_group = ctk_window_group_new ();

      return default_group;
    }
}

/**
 * ctk_window_has_group:
 * @window: a #CtkWindow
 *
 * Returns whether @window has an explicit window group.
 *
 * Returns: %TRUE if @window has an explicit window group.
 *
 * Since 2.22
 **/
gboolean
ctk_window_has_group (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return window->priv->group != NULL;
}

CtkWindowGroup *
_ctk_window_get_window_group (CtkWindow *window)
{
  return window->priv->group;
}

void
_ctk_window_set_window_group (CtkWindow      *window,
                              CtkWindowGroup *group)
{
  window->priv->group = group;
}

/*
  Derived from XParseGeometry() in XFree86  

  Copyright 1985, 1986, 1987,1998  The Open Group

  All Rights Reserved.

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.

  Except as contained in this notice, the name of The Open Group shall
  not be used in advertising or otherwise to promote the sale, use or
  other dealings in this Software without prior written authorization
  from The Open Group.
*/


/*
 *    XParseGeometry parses strings of the form
 *   "=<width>x<height>{+-}<xoffset>{+-}<yoffset>", where
 *   width, height, xoffset, and yoffset are unsigned integers.
 *   Example:  “=80x24+300-49”
 *   The equal sign is optional.
 *   It returns a bitmask that indicates which of the four values
 *   were actually found in the string.  For each value found,
 *   the corresponding argument is updated;  for each value
 *   not found, the corresponding argument is left unchanged. 
 */

/* The following code is from Xlib, and is minimally modified, so we
 * can track any upstream changes if required.  Don’t change this
 * code. Or if you do, put in a huge comment marking which thing
 * changed.
 */

static int
read_int (gchar   *string,
          gchar  **next)
{
  int result = 0;
  int sign = 1;
  
  if (*string == '+')
    string++;
  else if (*string == '-')
    {
      string++;
      sign = -1;
    }

  for (; (*string >= '0') && (*string <= '9'); string++)
    {
      result = (result * 10) + (*string - '0');
    }

  *next = string;

  if (sign >= 0)
    return (result);
  else
    return (-result);
}

/* 
 * Bitmask returned by XParseGeometry().  Each bit tells if the corresponding
 * value (x, y, width, height) was found in the parsed string.
 */
#define NoValue         0x0000
#define XValue          0x0001
#define YValue          0x0002
#define WidthValue      0x0004
#define HeightValue     0x0008
#define AllValues       0x000F
#define XNegative       0x0010
#define YNegative       0x0020

/* Try not to reformat/modify, so we can compare/sync with X sources */
static int
ctk_XParseGeometry (const char   *string,
                    int          *x,
                    int          *y,
                    unsigned int *width,   
                    unsigned int *height)  
{
  int mask = NoValue;
  char *strind;
  unsigned int tempWidth, tempHeight;
  int tempX, tempY;
  char *nextCharacter;

  /* These initializations are just to silence gcc */
  tempWidth = 0;
  tempHeight = 0;
  tempX = 0;
  tempY = 0;
  
  if ( (string == NULL) || (*string == '\0')) return(mask);
  if (*string == '=')
    string++;  /* ignore possible '=' at beg of geometry spec */

  strind = (char *)string;
  if (*strind != '+' && *strind != '-' && *strind != 'x') {
    tempWidth = read_int(strind, &nextCharacter);
    if (strind == nextCharacter) 
      return (0);
    strind = nextCharacter;
    mask |= WidthValue;
  }

  if (*strind == 'x' || *strind == 'X') {	
    strind++;
    tempHeight = read_int(strind, &nextCharacter);
    if (strind == nextCharacter)
      return (0);
    strind = nextCharacter;
    mask |= HeightValue;
  }

  if ((*strind == '+') || (*strind == '-')) {
    if (*strind == '-') {
      strind++;
      tempX = -read_int(strind, &nextCharacter);
      if (strind == nextCharacter)
        return (0);
      strind = nextCharacter;
      mask |= XNegative;

    }
    else
      {	strind++;
      tempX = read_int(strind, &nextCharacter);
      if (strind == nextCharacter)
        return(0);
      strind = nextCharacter;
      }
    mask |= XValue;
    if ((*strind == '+') || (*strind == '-')) {
      if (*strind == '-') {
        strind++;
        tempY = -read_int(strind, &nextCharacter);
        if (strind == nextCharacter)
          return(0);
        strind = nextCharacter;
        mask |= YNegative;

      }
      else
        {
          strind++;
          tempY = read_int(strind, &nextCharacter);
          if (strind == nextCharacter)
            return(0);
          strind = nextCharacter;
        }
      mask |= YValue;
    }
  }
	
  /* If strind isn't at the end of the string the it's an invalid
		geometry specification. */

  if (*strind != '\0') return (0);

  if (mask & XValue)
    *x = tempX;
  if (mask & YValue)
    *y = tempY;
  if (mask & WidthValue)
    *width = tempWidth;
  if (mask & HeightValue)
    *height = tempHeight;
  return (mask);
}

/**
 * ctk_window_parse_geometry:
 * @window: a #CtkWindow
 * @geometry: geometry string
 *
 * Parses a standard X Window System geometry string - see the
 * manual page for X (type “man X”) for details on this.
 * ctk_window_parse_geometry() does work on all CTK+ ports
 * including Win32 but is primarily intended for an X environment.
 *
 * If either a size or a position can be extracted from the
 * geometry string, ctk_window_parse_geometry() returns %TRUE
 * and calls ctk_window_set_default_size() and/or ctk_window_move()
 * to resize/move the window.
 *
 * If ctk_window_parse_geometry() returns %TRUE, it will also
 * set the #CDK_HINT_USER_POS and/or #CDK_HINT_USER_SIZE hints
 * indicating to the window manager that the size/position of
 * the window was user-specified. This causes most window
 * managers to honor the geometry.
 *
 * Note that for ctk_window_parse_geometry() to work as expected, it has
 * to be called when the window has its “final” size, i.e. after calling
 * ctk_widget_show_all() on the contents and ctk_window_set_geometry_hints()
 * on the window.
 * |[<!-- language="C" -->
 * #include <ctk/ctk.h>
 *
 * static void
 * fill_with_content (CtkWidget *vbox)
 * {
 *   // fill with content...
 * }
 *
 * int
 * main (int argc, char *argv[])
 * {
 *   CtkWidget *window, *vbox;
 *   CdkGeometry size_hints = {
 *     100, 50, 0, 0, 100, 50, 10,
 *     10, 0.0, 0.0, CDK_GRAVITY_NORTH_WEST
 *   };
 *
 *   ctk_init (&argc, &argv);
 *
 *   window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
 *   vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
 *
 *   ctk_container_add (CTK_CONTAINER (window), vbox);
 *   fill_with_content (vbox);
 *   ctk_widget_show_all (vbox);
 *
 *   ctk_window_set_geometry_hints (CTK_WINDOW (window),
 * 	  			    NULL,
 * 				    &size_hints,
 * 				    CDK_HINT_MIN_SIZE |
 * 				    CDK_HINT_BASE_SIZE |
 * 				    CDK_HINT_RESIZE_INC);
 *
 *   if (argc > 1)
 *     {
 *       gboolean res;
 *       res = ctk_window_parse_geometry (CTK_WINDOW (window),
 *                                        argv[1]);
 *       if (! res)
 *         fprintf (stderr,
 *                  "Failed to parse “%s”\n",
 *                  argv[1]);
 *     }
 *
 *   ctk_widget_show_all (window);
 *   ctk_main ();
 *
 *   return 0;
 * }
 * ]|
 *
 * Returns: %TRUE if string was parsed successfully
 *
 * Deprecated: 3.20: Geometry handling in CTK is deprecated.
 **/
gboolean
ctk_window_parse_geometry (CtkWindow   *window,
                           const gchar *geometry)
{
  gint result, x = 0, y = 0;
  guint w, h;
  CtkWidget *child;
  CdkGravity grav;
  gboolean size_set, pos_set;
  CdkScreen *screen;
  
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);
  g_return_val_if_fail (geometry != NULL, FALSE);

  child = ctk_bin_get_child (CTK_BIN (window));
  if (!child || !ctk_widget_get_visible (child))
    g_warning ("ctk_window_parse_geometry() called on a window with no "
	       "visible children; the window should be set up before "
	       "ctk_window_parse_geometry() is called.");

  screen = ctk_window_check_screen (window);
  
  result = ctk_XParseGeometry (geometry, &x, &y, &w, &h);

  size_set = FALSE;
  if ((result & WidthValue) || (result & HeightValue))
    {
      ctk_window_set_default_size_internal (window, 
					    TRUE, result & WidthValue ? w : -1,
					    TRUE, result & HeightValue ? h : -1, 
					    TRUE);
      size_set = TRUE;
    }

  ctk_window_get_size (window, (gint *)&w, (gint *)&h);
  
  grav = CDK_GRAVITY_NORTH_WEST;

  if ((result & XNegative) && (result & YNegative))
    grav = CDK_GRAVITY_SOUTH_EAST;
  else if (result & XNegative)
    grav = CDK_GRAVITY_NORTH_EAST;
  else if (result & YNegative)
    grav = CDK_GRAVITY_SOUTH_WEST;

  if ((result & XValue) == 0)
    x = 0;

  if ((result & YValue) == 0)
    y = 0;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (grav == CDK_GRAVITY_SOUTH_WEST ||
      grav == CDK_GRAVITY_SOUTH_EAST)
    y = cdk_screen_get_height (screen) - h + y;

  if (grav == CDK_GRAVITY_SOUTH_EAST ||
      grav == CDK_GRAVITY_NORTH_EAST)
    x = cdk_screen_get_width (screen) - w + x;
G_GNUC_END_IGNORE_DEPRECATIONS

  /* we don't let you put a window offscreen; maybe some people would
   * prefer to be able to, but it's kind of a bogus thing to do.
   */
  if (y < 0)
    y = 0;

  if (x < 0)
    x = 0;

  pos_set = FALSE;
  if ((result & XValue) || (result & YValue))
    {
      ctk_window_set_gravity (window, grav);
      ctk_window_move (window, x, y);
      pos_set = TRUE;
    }

  if (size_set || pos_set)
    {
      /* Set USSize, USPosition hints */
      CtkWindowGeometryInfo *info;

      info = ctk_window_get_geometry_info (window, TRUE);

      if (pos_set)
        info->mask |= CDK_HINT_USER_POS;
      if (size_set)
        info->mask |= CDK_HINT_USER_SIZE;
    }
  
  return result != 0;
}

static gboolean
ctk_window_activate_menubar (CtkWindow   *window,
                             CdkEventKey *event)
{
  CtkWindowPrivate *priv = window->priv;
  gchar *accel = NULL;
  guint keyval = 0;
  CdkModifierType mods = 0;

  g_object_get (ctk_widget_get_settings (CTK_WIDGET (window)),
                "ctk-menu-bar-accel", &accel,
                NULL);

  if (accel == NULL || *accel == 0)
    return FALSE;

  ctk_accelerator_parse (accel, &keyval, &mods);

  if (keyval == 0)
    {
      g_warning ("Failed to parse menu bar accelerator '%s'", accel);
      g_free (accel);
      return FALSE;
    }

  g_free (accel);

  /* FIXME this is wrong, needs to be in the global accel resolution
   * thing, to properly consider i18n etc., but that probably requires
   * AccelGroup changes etc.
   */
  if (event->keyval == keyval &&
      ((event->state & ctk_accelerator_get_default_mod_mask ()) ==
       (mods & ctk_accelerator_get_default_mod_mask ())))
    {
      GList *tmp_menubars;
      GList *menubars;
      CtkMenuShell *menu_shell;
      CtkWidget *focus;

      focus = ctk_window_get_focus (window);

      if (priv->title_box != NULL &&
          (focus == NULL || !ctk_widget_is_ancestor (focus, priv->title_box)) &&
          ctk_widget_child_focus (priv->title_box, CTK_DIR_TAB_FORWARD))
        return TRUE;

      tmp_menubars = _ctk_menu_bar_get_viewable_menu_bars (window);
      if (tmp_menubars == NULL)
        return FALSE;

      menubars = _ctk_container_focus_sort (CTK_CONTAINER (window), tmp_menubars,
                                            CTK_DIR_TAB_FORWARD, NULL);
      g_list_free (tmp_menubars);

      if (menubars == NULL)
        return FALSE;

      menu_shell = CTK_MENU_SHELL (menubars->data);

      _ctk_menu_shell_set_keyboard_mode (menu_shell, TRUE);
      ctk_menu_shell_select_first (menu_shell, FALSE);

      g_list_free (menubars);

      return TRUE;
    }
  return FALSE;
}

static void
ctk_window_mnemonic_hash_foreach (guint      keyval,
				  GSList    *targets,
				  gpointer   data)
{
  struct {
    CtkWindow *window;
    CtkWindowKeysForeachFunc func;
    gpointer func_data;
  } *info = data;

  (*info->func) (info->window, keyval, info->window->priv->mnemonic_modifier, TRUE, info->func_data);
}

void
_ctk_window_keys_foreach (CtkWindow                *window,
			  CtkWindowKeysForeachFunc func,
			  gpointer                 func_data)
{
  GSList *groups;
  CtkMnemonicHash *mnemonic_hash;

  struct {
    CtkWindow *window;
    CtkWindowKeysForeachFunc func;
    gpointer func_data;
  } info;

  info.window = window;
  info.func = func;
  info.func_data = func_data;

  mnemonic_hash = ctk_window_get_mnemonic_hash (window, FALSE);
  if (mnemonic_hash)
    _ctk_mnemonic_hash_foreach (mnemonic_hash,
				ctk_window_mnemonic_hash_foreach, &info);

  groups = ctk_accel_groups_from_object (G_OBJECT (window));
  while (groups)
    {
      CtkAccelGroup *group = groups->data;
      gint i;

      for (i = 0; i < group->priv->n_accels; i++)
	{
	  CtkAccelKey *key = &group->priv->priv_accels[i].key;
	  
	  if (key->accel_key)
	    (*func) (window, key->accel_key, key->accel_mods, FALSE, func_data);
	}
      
      groups = groups->next;
    }

  if (window->priv->application)
    {
      CtkApplicationAccels *app_accels;

      app_accels = ctk_application_get_application_accels (window->priv->application);
      ctk_application_accels_foreach_key (app_accels, window, func, func_data);
    }
}

static void
ctk_window_keys_changed (CtkWindow *window)
{
  ctk_window_free_key_hash (window);
  ctk_window_get_key_hash (window);
}

typedef struct _CtkWindowKeyEntry CtkWindowKeyEntry;

struct _CtkWindowKeyEntry
{
  guint keyval;
  guint modifiers;
  guint is_mnemonic : 1;
};

static void 
window_key_entry_destroy (gpointer data)
{
  g_slice_free (CtkWindowKeyEntry, data);
}

static void
add_to_key_hash (CtkWindow      *window,
		 guint           keyval,
		 CdkModifierType modifiers,
		 gboolean        is_mnemonic,
		 gpointer        data)
{
  CtkKeyHash *key_hash = data;

  CtkWindowKeyEntry *entry = g_slice_new (CtkWindowKeyEntry);

  entry->keyval = keyval;
  entry->modifiers = modifiers;
  entry->is_mnemonic = is_mnemonic;

  /* CtkAccelGroup stores lowercased accelerators. To deal
   * with this, if <Shift> was specified, uppercase.
   */
  if (modifiers & CDK_SHIFT_MASK)
    {
      if (keyval == CDK_KEY_Tab)
	keyval = CDK_KEY_ISO_Left_Tab;
      else
	keyval = cdk_keyval_to_upper (keyval);
    }
  
  _ctk_key_hash_add_entry (key_hash, keyval, entry->modifiers, entry);
}

static CtkKeyHash *
ctk_window_get_key_hash (CtkWindow *window)
{
  CdkScreen *screen = ctk_window_check_screen (window);
  CtkKeyHash *key_hash = g_object_get_qdata (G_OBJECT (window), quark_ctk_window_key_hash);
  
  if (key_hash)
    return key_hash;
  
  key_hash = _ctk_key_hash_new (cdk_keymap_get_for_display (cdk_screen_get_display (screen)),
				(GDestroyNotify)window_key_entry_destroy);
  _ctk_window_keys_foreach (window, add_to_key_hash, key_hash);
  g_object_set_qdata (G_OBJECT (window), quark_ctk_window_key_hash, key_hash);

  return key_hash;
}

static void
ctk_window_free_key_hash (CtkWindow *window)
{
  CtkKeyHash *key_hash = g_object_get_qdata (G_OBJECT (window), quark_ctk_window_key_hash);
  if (key_hash)
    {
      _ctk_key_hash_free (key_hash);
      g_object_set_qdata (G_OBJECT (window), quark_ctk_window_key_hash, NULL);
    }
}

/**
 * ctk_window_activate_key:
 * @window:  a #CtkWindow
 * @event:   a #CdkEventKey
 *
 * Activates mnemonics and accelerators for this #CtkWindow. This is normally
 * called by the default ::key_press_event handler for toplevel windows,
 * however in some cases it may be useful to call this directly when
 * overriding the standard key handling for a toplevel window.
 *
 * Returns: %TRUE if a mnemonic or accelerator was found and activated.
 *
 * Since: 2.4
 */
gboolean
ctk_window_activate_key (CtkWindow   *window,
			 CdkEventKey *event)
{
  CtkKeyHash *key_hash;
  CtkWindowKeyEntry *found_entry = NULL;
  gboolean enable_accels;
  gboolean enable_mnemonics;

  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  key_hash = ctk_window_get_key_hash (window);

  if (key_hash)
    {
      GSList *tmp_list;
      GSList *entries = _ctk_key_hash_lookup (key_hash,
					      event->hardware_keycode,
					      event->state,
					      ctk_accelerator_get_default_mod_mask (),
					      event->group);

      g_object_get (ctk_widget_get_settings (CTK_WIDGET (window)),
                    "ctk-enable-mnemonics", &enable_mnemonics,
                    "ctk-enable-accels", &enable_accels,
                    NULL);

      for (tmp_list = entries; tmp_list; tmp_list = tmp_list->next)
	{
	  CtkWindowKeyEntry *entry = tmp_list->data;
	  if (entry->is_mnemonic)
            {
              if( enable_mnemonics)
              {
                found_entry = entry;
                break;
              }
            }
          else 
            {
              if (enable_accels && !found_entry)
                {
	          found_entry = entry;
                }
            }
	}

      g_slist_free (entries);
    }

  if (found_entry)
    {
      if (found_entry->is_mnemonic)
        {
          if( enable_mnemonics)
            return ctk_window_mnemonic_activate (window, found_entry->keyval,
                                               found_entry->modifiers);
        }
      else
        {
          if (enable_accels)
            {
              if (ctk_accel_groups_activate (G_OBJECT (window), found_entry->keyval, found_entry->modifiers))
                return TRUE;

              if (window->priv->application)
                {
                  CtkWidget *focused_widget;
                  CtkActionMuxer *muxer;
                  CtkApplicationAccels *app_accels;

                  focused_widget = ctk_window_get_focus (window);

                  if (focused_widget)
                    muxer = _ctk_widget_get_action_muxer (focused_widget, FALSE);
                  else
                    muxer = _ctk_widget_get_action_muxer (CTK_WIDGET (window), FALSE);

                  if (muxer == NULL)
                    return FALSE;

                  app_accels = ctk_application_get_application_accels (window->priv->application);
                  return ctk_application_accels_activate (app_accels,
                                                          G_ACTION_GROUP (muxer),
                                                          found_entry->keyval, found_entry->modifiers);
                }
            }
        }
    }

  return ctk_window_activate_menubar (window, event);
}

static void
window_update_has_focus (CtkWindow *window)
{
  CtkWindowPrivate *priv = window->priv;
  CtkWidget *widget = CTK_WIDGET (window);
  gboolean has_focus = priv->has_toplevel_focus && priv->is_active;

  if (has_focus != priv->has_focus)
    {
      priv->has_focus = has_focus;

      if (has_focus)
	{
	  if (priv->focus_widget &&
	      priv->focus_widget != widget &&
	      !ctk_widget_has_focus (priv->focus_widget))
	    do_focus_change (priv->focus_widget, TRUE);
	}
      else
	{
	  if (priv->focus_widget &&
	      priv->focus_widget != widget &&
	      ctk_widget_has_focus (priv->focus_widget))
	    do_focus_change (priv->focus_widget, FALSE);
	}
    }
}

/**
 * _ctk_window_set_is_active:
 * @window: a #CtkWindow
 * @is_active: %TRUE if the window is in the currently active toplevel
 * 
 * Internal function that sets whether the #CtkWindow is part
 * of the currently active toplevel window (taking into account inter-process
 * embedding.)
 **/
void
_ctk_window_set_is_active (CtkWindow *window,
			   gboolean   is_active)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  is_active = is_active != FALSE;

  if (is_active != priv->is_active)
    {
      priv->is_active = is_active;
      window_update_has_focus (window);

      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_IS_ACTIVE]);
    }
}

/**
 * _ctk_window_set_is_toplevel:
 * @window: a #CtkWindow
 * @is_toplevel: %TRUE if the window is still a real toplevel (nominally a
 * child of the root window); %FALSE if it is not (for example, for an
 * in-process, parented CtkPlug)
 *
 * Internal function used by #CtkPlug when it gets parented/unparented by a
 * #CtkSocket.  This keeps the @window’s #CTK_WINDOW_TOPLEVEL flag in sync
 * with the global list of toplevel windows.
 */
void
_ctk_window_set_is_toplevel (CtkWindow *window,
                             gboolean   is_toplevel)
{
  CtkWidget *widget;
  CtkWidget *toplevel;

  widget = CTK_WIDGET (window);

  if (_ctk_widget_is_toplevel (widget))
    g_assert (g_slist_find (toplevel_list, window) != NULL);
  else
    g_assert (g_slist_find (toplevel_list, window) == NULL);

  if (is_toplevel == _ctk_widget_is_toplevel (widget))
    return;

  if (is_toplevel)
    {
      /* Pass through regular pathways of an embedded toplevel
       * to go through unmapping and hiding the widget before
       * becomming a toplevel again.
       *
       * We remain hidden after becomming toplevel in order to
       * avoid problems during an embedded toplevel's dispose cycle
       * (When a toplevel window is shown it tries to grab focus again,
       * this causes problems while disposing).
       */
      ctk_widget_hide (widget);

      /* Save the toplevel this widget was previously anchored into before
       * propagating a hierarchy-changed.
       *
       * Usually this happens by way of ctk_widget_unparent() and we are
       * already unanchored at this point, just adding this clause incase
       * things happen differently.
       */
      toplevel = _ctk_widget_get_toplevel (widget);
      if (!_ctk_widget_is_toplevel (toplevel))
        toplevel = NULL;

      _ctk_widget_set_is_toplevel (widget, TRUE);

      /* When a window becomes toplevel after being embedded and anchored
       * into another window we need to unset its anchored flag so that
       * the hierarchy changed signal kicks in properly.
       */
      _ctk_widget_set_anchored (widget, FALSE);
      _ctk_widget_propagate_hierarchy_changed (widget, toplevel);

      toplevel_list = g_slist_prepend (toplevel_list, window);
    }
  else
    {
      _ctk_widget_set_is_toplevel (widget, FALSE);
      toplevel_list = g_slist_remove (toplevel_list, window);
      _ctk_widget_propagate_hierarchy_changed (widget, widget);
    }
  ctk_window_update_debugging ();
}

/**
 * _ctk_window_set_has_toplevel_focus:
 * @window: a #CtkWindow
 * @has_toplevel_focus: %TRUE if the in
 * 
 * Internal function that sets whether the keyboard focus for the
 * toplevel window (taking into account inter-process embedding.)
 **/
void
_ctk_window_set_has_toplevel_focus (CtkWindow *window,
				   gboolean   has_toplevel_focus)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  has_toplevel_focus = has_toplevel_focus != FALSE;

  if (has_toplevel_focus != priv->has_toplevel_focus)
    {
      priv->has_toplevel_focus = has_toplevel_focus;
      window_update_has_focus (window);

      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_HAS_TOPLEVEL_FOCUS]);
    }
}

/**
 * ctk_window_set_auto_startup_notification:
 * @setting: %TRUE to automatically do startup notification
 *
 * By default, after showing the first #CtkWindow, CTK+ calls 
 * cdk_notify_startup_complete().  Call this function to disable 
 * the automatic startup notification. You might do this if your 
 * first window is a splash screen, and you want to delay notification 
 * until after your real main window has been shown, for example.
 *
 * In that example, you would disable startup notification
 * temporarily, show your splash screen, then re-enable it so that
 * showing the main window would automatically result in notification.
 * 
 * Since: 2.2
 **/
void
ctk_window_set_auto_startup_notification (gboolean setting)
{
  disable_startup_notification = !setting;
}

/**
 * ctk_window_get_window_type:
 * @window: a #CtkWindow
 *
 * Gets the type of the window. See #CtkWindowType.
 *
 * Returns: the type of the window
 *
 * Since: 2.20
 **/
CtkWindowType
ctk_window_get_window_type (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), CTK_WINDOW_TOPLEVEL);

  return window->priv->type;
}

/**
 * ctk_window_get_mnemonics_visible:
 * @window: a #CtkWindow
 *
 * Gets the value of the #CtkWindow:mnemonics-visible property.
 *
 * Returns: %TRUE if mnemonics are supposed to be visible
 * in this window.
 *
 * Since: 2.20
 */
gboolean
ctk_window_get_mnemonics_visible (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return window->priv->mnemonics_visible;
}

/**
 * ctk_window_set_mnemonics_visible:
 * @window: a #CtkWindow
 * @setting: the new value
 *
 * Sets the #CtkWindow:mnemonics-visible property.
 *
 * Since: 2.20
 */
void
ctk_window_set_mnemonics_visible (CtkWindow *window,
                                  gboolean   setting)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  setting = setting != FALSE;

  if (priv->mnemonics_visible != setting)
    {
      priv->mnemonics_visible = setting;
      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_MNEMONICS_VISIBLE]);
    }

  if (priv->mnemonics_display_timeout_id)
    {
      g_source_remove (priv->mnemonics_display_timeout_id);
      priv->mnemonics_display_timeout_id = 0;
    }

  priv->mnemonics_visible_set = TRUE;
}

static gboolean
schedule_mnemonics_visible_cb (gpointer data)
{
  CtkWindow *window = data;

  window->priv->mnemonics_display_timeout_id = 0;

  ctk_window_set_mnemonics_visible (window, TRUE);

  return FALSE;
}

void
_ctk_window_schedule_mnemonics_visible (CtkWindow *window)
{
  g_return_if_fail (CTK_IS_WINDOW (window));

  if (window->priv->mnemonics_display_timeout_id)
    return;

  window->priv->mnemonics_display_timeout_id =
    cdk_threads_add_timeout (MNEMONICS_DELAY, schedule_mnemonics_visible_cb, window);
  g_source_set_name_by_id (window->priv->mnemonics_display_timeout_id, "[ctk+] schedule_mnemonics_visible_cb");
}

/**
 * ctk_window_get_focus_visible:
 * @window: a #CtkWindow
 *
 * Gets the value of the #CtkWindow:focus-visible property.
 *
 * Returns: %TRUE if “focus rectangles” are supposed to be visible
 *     in this window.
 *
 * Since: 3.2
 */
gboolean
ctk_window_get_focus_visible (CtkWindow *window)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);

  return window->priv->focus_visible;
}

/**
 * ctk_window_set_focus_visible:
 * @window: a #CtkWindow
 * @setting: the new value
 *
 * Sets the #CtkWindow:focus-visible property.
 *
 * Since: 3.2
 */
void
ctk_window_set_focus_visible (CtkWindow *window,
                              gboolean   setting)
{
  CtkWindowPrivate *priv;

  g_return_if_fail (CTK_IS_WINDOW (window));

  priv = window->priv;

  setting = setting != FALSE;

  if (priv->focus_visible != setting)
    {
      priv->focus_visible = setting;
      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_FOCUS_VISIBLE]);
    }
}

void
_ctk_window_get_wmclass (CtkWindow  *window,
                         gchar     **wmclass_name,
                         gchar     **wmclass_class)
{
  CtkWindowPrivate *priv = window->priv;

  *wmclass_name = priv->wmclass_name;
  *wmclass_class = priv->wmclass_class;
}

/**
 * ctk_window_set_has_user_ref_count:
 * @window: a #CtkWindow
 * @setting: the new value
 *
 * Tells CTK+ whether to drop its extra reference to the window
 * when ctk_widget_destroy() is called.
 *
 * This function is only exported for the benefit of language
 * bindings which may need to keep the window alive until their
 * wrapper object is garbage collected. There is no justification
 * for ever calling this function in an application.
 *
 * Since: 3.0
 */
void
ctk_window_set_has_user_ref_count (CtkWindow *window,
                                   gboolean   setting)
{
  g_return_if_fail (CTK_IS_WINDOW (window));

  window->priv->has_user_ref_count = setting;
}

static void
ensure_state_flag_backdrop (CtkWidget *widget)
{
  CdkWindow *window;
  gboolean window_focused = TRUE;

  window = _ctk_widget_get_window (widget);

  window_focused = cdk_window_get_state (window) & CDK_WINDOW_STATE_FOCUSED;

  if (!window_focused)
    ctk_widget_set_state_flags (widget, CTK_STATE_FLAG_BACKDROP, FALSE);
  else
    ctk_widget_unset_state_flags (widget, CTK_STATE_FLAG_BACKDROP);
}

void
_ctk_window_get_shadow_width (CtkWindow *window,
                              CtkBorder *border)
{
  get_shadow_width (window, border);
}

void
_ctk_window_add_popover (CtkWindow *window,
                         CtkWidget *popover,
                         CtkWidget *parent,
                         gboolean   clamp_allocation)
{
  CtkWindowPrivate *priv;
  CtkWindowPopover *data;
  AtkObject *accessible;

  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (CTK_IS_WIDGET (popover));
  g_return_if_fail (CTK_IS_WIDGET (parent));
  g_return_if_fail (_ctk_widget_get_parent (popover) == NULL);
  g_return_if_fail (ctk_widget_is_ancestor (parent, CTK_WIDGET (window)));

  priv = window->priv;

  if (_ctk_window_has_popover (window, popover))
    return;

  data = g_new0 (CtkWindowPopover, 1);
  data->widget = popover;
  data->parent = parent;
  data->clamp_allocation = !!clamp_allocation;
  priv->popovers = g_list_prepend (priv->popovers, data);

  if (_ctk_widget_get_realized (CTK_WIDGET (window)))
    popover_realize (popover, data, window);

  ctk_widget_set_parent (popover, CTK_WIDGET (window));

  accessible = ctk_widget_get_accessible (CTK_WIDGET (window));
  _ctk_container_accessible_add_child (CTK_CONTAINER_ACCESSIBLE (accessible),
                                       ctk_widget_get_accessible (popover), -1);
}

void
_ctk_window_remove_popover (CtkWindow *window,
                            CtkWidget *popover)
{
  CtkWindowPrivate *priv;
  CtkWindowPopover *data;
  AtkObject *accessible;

  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (CTK_IS_WIDGET (popover));

  priv = window->priv;

  data = _ctk_window_has_popover (window, popover);

  if (!data)
    return;

  g_object_ref (popover);
  ctk_widget_unparent (popover);

  popover_unmap (popover, data);

  if (_ctk_widget_get_realized (CTK_WIDGET (popover)))
    popover_unrealize (popover, data, window);

  priv->popovers = g_list_remove (priv->popovers, data);

  accessible = ctk_widget_get_accessible (CTK_WIDGET (window));
  _ctk_container_accessible_remove_child (CTK_CONTAINER_ACCESSIBLE (accessible),
                                          ctk_widget_get_accessible (popover), -1);
  popover_destroy (data);
  g_object_unref (popover);
}

void
_ctk_window_set_popover_position (CtkWindow                   *window,
                                  CtkWidget                   *popover,
                                  CtkPositionType              pos,
                                  const cairo_rectangle_int_t *rect)
{
  gboolean need_resize;
  gboolean need_move;
  CtkWindowPopover *data;

  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (CTK_IS_WIDGET (popover));

  data = _ctk_window_has_popover (window, popover);

  if (!data)
    {
      g_warning ("Widget %s(%p) is not a popover of window %s(%p)",
                 ctk_widget_get_name (popover), popover,
                 ctk_widget_get_name (CTK_WIDGET (window)), window);
      return;
    }


  /* Don't queue a resize if the position as well as the size are the same */
  need_move = data->pos    != pos ||
              data->rect.x != rect->x ||
              data->rect.y != rect->y;

  need_resize = data->pos != pos ||
                data->rect.width  != rect->width ||
                data->rect.height != rect->height;

  data->rect = *rect;
  data->pos = pos;

  if (ctk_widget_is_visible (popover) && !data->window &&
      ctk_widget_get_realized (CTK_WIDGET (window)))
    {
      popover_realize (popover, data, window);
      popover_map (popover, data);
    }

  if (need_resize)
    {
      ctk_widget_queue_resize (popover);
    }
  else if (need_move)
    {
      cairo_rectangle_int_t new_size;
      popover_get_rect (data, window, &new_size);
      cdk_window_move (data->window, new_size.x, new_size.y);
    }
}

void
_ctk_window_get_popover_position (CtkWindow             *window,
                                  CtkWidget             *popover,
                                  CtkPositionType       *pos,
                                  cairo_rectangle_int_t *rect)
{
  CtkWindowPopover *data;

  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (CTK_IS_WIDGET (popover));

  data = _ctk_window_has_popover (window, popover);

  if (!data)
    {
      g_warning ("Widget %s(%p) is not a popover of window %s(%p)",
                 ctk_widget_get_name (popover), popover,
                 ctk_widget_get_name (CTK_WIDGET (window)), window);
      return;
    }

  if (pos)
    *pos = data->pos;

  if (rect)
    *rect = data->rect;
}

/*<private>
 * _ctk_window_get_popover_parent:
 * @window: A #CtkWindow
 * @popover: A popover #CtkWidget
 *
 * Returns the conceptual parent of this popover, the real
 * parent will always be @window.
 *
 * Returns: (nullable): The conceptual parent widget, or %NULL.
 **/
CtkWidget *
_ctk_window_get_popover_parent (CtkWindow *window,
                                CtkWidget *popover)
{
  CtkWindowPopover *data;

  g_return_val_if_fail (CTK_IS_WINDOW (window), NULL);
  g_return_val_if_fail (CTK_IS_WIDGET (popover), NULL);

  data = _ctk_window_has_popover (window, popover);

  if (data && data->parent)
    return data->parent;

  return NULL;
}

/*<private>
 * _ctk_window_is_popover_widget:
 * @window: A #CtkWindow
 * @possible_popover: A possible popover of @window
 *
 * Returns #TRUE if @possible_popover is a popover of @window.
 *
 * Returns: Whether the widget is a popover of @window
 **/
gboolean
_ctk_window_is_popover_widget (CtkWindow *window,
                               CtkWidget *possible_popover)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), FALSE);
  g_return_val_if_fail (CTK_IS_WIDGET (possible_popover), FALSE);

  return _ctk_window_has_popover (window, possible_popover) != NULL;
}

void
_ctk_window_raise_popover (CtkWindow *window,
                           CtkWidget *widget)
{
  CtkWindowPrivate *priv = window->priv;
  GList *link;

  for (link = priv->popovers; link; link = link->next)
    {
      CtkWindowPopover *popover = link->data;

      if (popover->widget != widget)
        continue;

      priv->popovers = g_list_remove_link (priv->popovers, link);
      priv->popovers = g_list_append (priv->popovers, link->data);
      g_list_free (link);
      break;
    }
  ctk_window_restack_popovers (window);
}

static CtkWidget *inspector_window = NULL;

static guint ctk_window_update_debugging_id;

static void set_warn_again (gboolean warn);

static void
warn_response (CtkDialog *dialog,
               gint       response)
{
  CtkWidget *check;
  gboolean remember;

  check = g_object_get_data (G_OBJECT (dialog), "check");
  remember = ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (check));

  ctk_widget_destroy (CTK_WIDGET (dialog));
  g_object_set_data (G_OBJECT (inspector_window), "warning_dialog", NULL);
  if (response == CTK_RESPONSE_NO)
    {
      CtkWidget *window;

      if (ctk_window_update_debugging_id)
        {
          g_source_remove (ctk_window_update_debugging_id);
          ctk_window_update_debugging_id = 0;
        }

      /* Steal reference into temp variable, so not to mess up with
       * inspector_window during ctk_widget_destroy().
       */
      window = inspector_window;
      inspector_window = NULL;
      ctk_widget_destroy (window);
    }
  else
    {
      set_warn_again (!remember);
    }
}

static gboolean
update_debugging (gpointer data)
{
  ctk_inspector_window_rescan (inspector_window);
  ctk_window_update_debugging_id = 0;
  return G_SOURCE_REMOVE;
}

static void
ctk_window_update_debugging (void)
{
  if (inspector_window &&
      ctk_window_update_debugging_id == 0)
    {
      ctk_window_update_debugging_id = cdk_threads_add_idle (update_debugging, NULL);
      g_source_set_name_by_id (ctk_window_update_debugging_id, "[ctk+] ctk_window_update_debugging");
    }
}

static void
ctk_window_set_debugging (gboolean enable,
                          gboolean select,
                          gboolean warn)
{
  CtkWidget *dialog = NULL;
  CtkWidget *area;
  CtkWidget *check;

  if (inspector_window == NULL)
    {
      ctk_inspector_init ();
      inspector_window = ctk_inspector_window_new ();
      g_signal_connect (inspector_window, "delete-event",
                        G_CALLBACK (ctk_widget_hide_on_delete), NULL);

      if (warn)
        {
          dialog = ctk_message_dialog_new (CTK_WINDOW (inspector_window),
                                           CTK_DIALOG_MODAL|CTK_DIALOG_DESTROY_WITH_PARENT,
                                           CTK_MESSAGE_QUESTION,
                                           CTK_BUTTONS_NONE,
                                           _("Do you want to use CTK+ Inspector?"));
          ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (dialog),
              _("CTK+ Inspector is an interactive debugger that lets you explore and "
                "modify the internals of any CTK+ application. Using it may cause the "
                "application to break or crash."));

          area = ctk_message_dialog_get_message_area (CTK_MESSAGE_DIALOG (dialog));
          check = ctk_check_button_new_with_label (_("Don't show this message again"));
          ctk_widget_set_margin_start (check, 10);
          ctk_widget_show (check);
          ctk_container_add (CTK_CONTAINER (area), check);
          g_object_set_data (G_OBJECT (dialog), "check", check);
          ctk_dialog_add_button (CTK_DIALOG (dialog), _("_Cancel"), CTK_RESPONSE_NO);
          ctk_dialog_add_button (CTK_DIALOG (dialog), _("_OK"), CTK_RESPONSE_YES);
          g_signal_connect (dialog, "response", G_CALLBACK (warn_response), NULL);
          g_object_set_data (G_OBJECT (inspector_window), "warning_dialog", dialog);
        }
    }

  dialog = g_object_get_data (G_OBJECT (inspector_window), "warning_dialog");

  if (enable)
    {
      ctk_window_present (CTK_WINDOW (inspector_window));

      if (dialog)
        ctk_widget_show (dialog);

      if (select)
        ctk_inspector_window_select_widget_under_pointer (CTK_INSPECTOR_WINDOW (inspector_window));
    }
  else
    {
      if (dialog)
        ctk_widget_hide (dialog);
      ctk_widget_hide (inspector_window);
    }
}

/**
 * ctk_window_set_interactive_debugging:
 * @enable: %TRUE to enable interactive debugging
 *
 * Opens or closes the [interactive debugger][interactive-debugging],
 * which offers access to the widget hierarchy of the application
 * and to useful debugging tools.
 *
 * Since: 3.14
 */
void
ctk_window_set_interactive_debugging (gboolean enable)
{
  ctk_window_set_debugging (enable, FALSE, FALSE);
}

static gboolean
inspector_keybinding_enabled (gboolean *warn)
{
  GSettingsSchema *schema;
  GSettings *settings;
  gboolean enabled;

  enabled = FALSE;
  *warn = FALSE;

  schema = g_settings_schema_source_lookup (g_settings_schema_source_get_default (),
                                            "org.ctk.Settings.Debug",
                                            TRUE);

  if (schema)
    {
      settings = g_settings_new_full (schema, NULL, NULL);
      enabled = g_settings_get_boolean (settings, "enable-inspector-keybinding");
      *warn = g_settings_get_boolean (settings, "inspector-warning");
      g_object_unref (settings);
      g_settings_schema_unref (schema);
    }

  return enabled;
}

static void
set_warn_again (gboolean warn)
{
  GSettingsSchema *schema;
  GSettings *settings;

  schema = g_settings_schema_source_lookup (g_settings_schema_source_get_default (),
                                            "org.ctk.Settings.Debug",
                                            TRUE);

  if (schema)
    {
      settings = g_settings_new_full (schema, NULL, NULL);
      g_settings_set_boolean (settings, "inspector-warning", warn);
      g_object_unref (settings);
      g_settings_schema_unref (schema);
    }
}

static gboolean
ctk_window_enable_debugging (CtkWindow *window,
                             gboolean   toggle)
{
  gboolean warn;

  if (!inspector_keybinding_enabled (&warn))
    return FALSE;

  if (toggle)
    {
      if (CTK_IS_WIDGET (inspector_window) &&
          ctk_widget_is_visible (inspector_window))
        ctk_window_set_debugging (FALSE, FALSE, FALSE);
      else
        ctk_window_set_debugging (TRUE, FALSE, warn);
    }
  else
    ctk_window_set_debugging (TRUE, TRUE, warn);

  return TRUE;
}

void
ctk_window_set_use_subsurface (CtkWindow *window,
                               gboolean   use_subsurface)
{
  CtkWindowPrivate *priv = window->priv;

  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (!_ctk_widget_get_realized (CTK_WIDGET (window)));

  priv->use_subsurface = use_subsurface;
}

void
ctk_window_set_hardcoded_window (CtkWindow *window,
                                 CdkWindow *cdk_window)
{
  CtkWindowPrivate *priv = window->priv;

  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (!_ctk_widget_get_realized (CTK_WIDGET (window)));

  g_set_object (&priv->hardcoded_window, cdk_window);
}

#ifdef CDK_WINDOWING_WAYLAND
typedef struct {
  CtkWindow *window;
  CtkWindowHandleExported callback;
  gpointer user_data;
} WaylandWindowHandleExportedData;

static void
wayland_window_handle_exported (CdkWindow  *window,
                                const char *wayland_handle_str,
                                gpointer    user_data)
{
  WaylandWindowHandleExportedData *data = user_data;
  char *handle_str;

  handle_str = g_strdup_printf ("wayland:%s", wayland_handle_str);
  data->callback (data->window, handle_str, data->user_data);
  g_free (handle_str);
}
#endif

gboolean
ctk_window_export_handle (CtkWindow               *window,
                          CtkWindowHandleExported  callback,
                          gpointer                 user_data)
{

#ifdef CDK_WINDOWING_X11
  if (CDK_IS_X11_DISPLAY (ctk_widget_get_display (CTK_WIDGET (window))))
    {
      CdkWindow *cdk_window = ctk_widget_get_window (CTK_WIDGET (window));
      char *handle_str;
      guint32 xid = (guint32) cdk_x11_window_get_xid (cdk_window);

      handle_str = g_strdup_printf ("x11:%x", xid);
      callback (window, handle_str, user_data);

      return TRUE;
    }
#endif
#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (ctk_widget_get_display (CTK_WIDGET (window))))
    {
      CdkWindow *cdk_window = ctk_widget_get_window (CTK_WIDGET (window));
      WaylandWindowHandleExportedData *data;

      data = g_new0 (WaylandWindowHandleExportedData, 1);
      data->window = window;
      data->callback = callback;
      data->user_data = user_data;

      if (!cdk_wayland_window_export_handle (cdk_window,
                                             wayland_window_handle_exported,
                                             data,
                                             g_free))
        {
          g_free (data);
          return FALSE;
        }
      else
        {
          return TRUE;
        }
    }
#endif

  g_warning ("Couldn't export handle, unsupported windowing system");

  return FALSE;
}

void
ctk_window_unexport_handle (CtkWindow *window)
{
#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (ctk_widget_get_display (CTK_WIDGET (window))))
    {
      CdkWindow *cdk_window = ctk_widget_get_window (CTK_WIDGET (window));

      cdk_wayland_window_unexport_handle (cdk_window);
    }
#endif
}
