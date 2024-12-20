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

#include "ctkscrolledwindow.h"

#include "ctkadjustment.h"
#include "ctkadjustmentprivate.h"
#include "ctkbindings.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkdnd.h"
#include "ctkintl.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctkscrollable.h"
#include "ctkscrollbar.h"
#include "ctkrangeprivate.h"
#include "ctktypebuiltins.h"
#include "ctkviewport.h"
#include "ctkwidgetprivate.h"
#include "ctkwindow.h"
#include "ctkkineticscrolling.h"
#include "a11y/ctkscrolledwindowaccessible.h"
#include "ctkstylecontextprivate.h"
#include "ctkprogresstrackerprivate.h"
#include "ctksettingsprivate.h"

#include <math.h>

/**
 * SECTION:ctkscrolledwindow
 * @Short_description: Adds scrollbars to its child widget
 * @Title: CtkScrolledWindow
 * @See_also: #CtkScrollable, #CtkViewport, #CtkAdjustment
 *
 * CtkScrolledWindow is a container that accepts a single child widget, makes
 * that child scrollable using either internally added scrollbars or externally
 * associated adjustments, and optionally draws a frame around the child.
 *
 * Widgets with native scrolling support, i.e. those whose classes implement the
 * #CtkScrollable interface, are added directly. For other types of widget, the
 * class #CtkViewport acts as an adaptor, giving scrollability to other widgets.
 * CtkScrolledWindow’s implementation of ctk_container_add() intelligently
 * accounts for whether or not the added child is a #CtkScrollable. If it isn’t,
 * #CtkScrolledWindow wraps the child in a #CtkViewport and adds that for you.
 * Therefore, you can just add any child widget and not worry about the details.
 *
 * If ctk_container_add() has added a #CtkViewport for you, you can remove
 * both your added child widget from the #CtkViewport, and the #CtkViewport
 * from the CtkScrolledWindow, like this:
 *
 * |[<!-- language="C" -->
 * CtkWidget *scrolled_window = ctk_scrolled_window_new (NULL, NULL);
 * CtkWidget *child_widget = ctk_button_new ();
 *
 * // CtkButton is not a CtkScrollable, so CtkScrolledWindow will automatically
 * // add a CtkViewport.
 * ctk_container_add (CTK_CONTAINER (scrolled_window),
 *                    child_widget);
 *
 * // Either of these will result in child_widget being unparented:
 * ctk_container_remove (CTK_CONTAINER (scrolled_window),
 *                       child_widget);
 * // or
 * ctk_container_remove (CTK_CONTAINER (scrolled_window),
 *                       ctk_bin_get_child (CTK_BIN (scrolled_window)));
 * ]|
 *
 * Unless #CtkScrolledWindow:policy is CTK_POLICY_NEVER or CTK_POLICY_EXTERNAL,
 * CtkScrolledWindow adds internal #CtkScrollbar widgets around its child. The
 * scroll position of the child, and if applicable the scrollbars, is controlled
 * by the #CtkScrolledWindow:hadjustment and #CtkScrolledWindow:vadjustment
 * that are associated with the CtkScrolledWindow. See the docs on #CtkScrollbar
 * for the details, but note that the “step_increment” and “page_increment”
 * fields are only effective if the policy causes scrollbars to be present.
 *
 * If a CtkScrolledWindow doesn’t behave quite as you would like, or
 * doesn’t have exactly the right layout, it’s very possible to set up
 * your own scrolling with #CtkScrollbar and for example a #CtkGrid.
 *
 * # Touch support
 *
 * CtkScrolledWindow has built-in support for touch devices. When a
 * touchscreen is used, swiping will move the scrolled window, and will
 * expose 'kinetic' behavior. This can be turned off with the
 * #CtkScrolledWindow:kinetic-scrolling property if it is undesired.
 *
 * CtkScrolledWindow also displays visual 'overshoot' indication when
 * the content is pulled beyond the end, and this situation can be
 * captured with the #CtkScrolledWindow::edge-overshot signal.
 *
 * If no mouse device is present, the scrollbars will overlayed as
 * narrow, auto-hiding indicators over the content. If traditional
 * scrollbars are desired although no mouse is present, this behaviour
 * can be turned off with the #CtkScrolledWindow:overlay-scrolling
 * property.
 *
 * # CSS nodes
 *
 * CtkScrolledWindow has a main CSS node with name scrolledwindow.
 *
 * It uses subnodes with names overshoot and undershoot to
 * draw the overflow and underflow indications. These nodes get
 * the .left, .right, .top or .bottom style class added depending
 * on where the indication is drawn.
 *
 * CtkScrolledWindow also sets the positional style classes (.left,
 * .right, .top, .bottom) and style classes related to overlay
 * scrolling (.overlay-indicator, .dragging, .hovering) on its scrollbars.
 *
 * If both scrollbars are visible, the area where they meet is drawn
 * with a subnode named junction.
 */


/* scrolled window policy and size requisition handling:
 *
 * ctk size requisition works as follows:
 *   a widget upon size-request reports the width and height that it finds
 *   to be best suited to display its contents, including children.
 *   the width and/or height reported from a widget upon size requisition
 *   may be overidden by the user by specifying a width and/or height
 *   other than 0 through ctk_widget_set_size_request().
 *
 * a scrolled window needs (for implementing all three policy types) to
 * request its width and height based on two different rationales.
 * 1)   the user wants the scrolled window to just fit into the space
 *      that it gets allocated for a specifc dimension.
 * 1.1) this does not apply if the user specified a concrete value
 *      value for that specific dimension by either specifying usize for the
 *      scrolled window or for its child.
 * 2)   the user wants the scrolled window to take as much space up as
 *      is desired by the child for a specifc dimension (i.e. POLICY_NEVER).
 *
 * also, kinda obvious:
 * 3)   a user would certainly not have choosen a scrolled window as a container
 *      for the child, if the resulting allocation takes up more space than the
 *      child would have allocated without the scrolled window.
 *
 * conclusions:
 * A) from 1) follows: the scrolled window shouldn’t request more space for a
 *    specifc dimension than is required at minimum.
 * B) from 1.1) follows: the requisition may be overidden by usize of the scrolled
 *    window (done automatically) or by usize of the child (needs to be checked).
 * C) from 2) follows: for POLICY_NEVER, the scrolled window simply reports the
 *    child’s dimension.
 * D) from 3) follows: the scrolled window child’s minimum width and minimum height
 *    under A) at least correspond to the space taken up by its scrollbars.
 */

#define DEFAULT_SCROLLBAR_SPACING  3
#define TOUCH_BYPASS_CAPTURED_THRESHOLD 30

/* Kinetic scrolling */
#define MAX_OVERSHOOT_DISTANCE 100
#define DECELERATION_FRICTION 4
#define OVERSHOOT_FRICTION 20
#define SCROLL_CAPTURE_THRESHOLD_MS 150
#define VELOCITY_ACCUMULATION_FLOOR 0.33
#define VELOCITY_ACCUMULATION_CEIL 1.0
#define VELOCITY_ACCUMULATION_MAX 6.0

/* Animated scrolling */
#define ANIMATION_DURATION 200

/* Overlay scrollbars */
#define INDICATOR_FADE_OUT_DELAY 2000
#define INDICATOR_FADE_OUT_DURATION 1000
#define INDICATOR_FADE_OUT_TIME 500
#define INDICATOR_CLOSE_DISTANCE 5
#define INDICATOR_FAR_DISTANCE 10

/* Scrolled off indication */
#define UNDERSHOOT_SIZE 40

typedef struct
{
  CtkWidget *scrollbar;
  CdkWindow *window;
  gboolean   over; /* either mouse over, or while dragging */
  gint64     last_scroll_time;
  guint      conceil_timer;

  gdouble    current_pos;
  gdouble    source_pos;
  gdouble    target_pos;
  CtkProgressTracker tracker;
  guint      tick_id;
  guint      over_timeout_id;
} Indicator;

typedef struct
{
  gdouble dx;
  gdouble dy;
  guint32 evtime;
} ScrollHistoryElem;

struct _CtkScrolledWindowPrivate
{
  CtkWidget     *hscrollbar;
  CtkWidget     *vscrollbar;

  CtkCssGadget  *gadget;
  CtkCssNode    *overshoot_node[4];
  CtkCssNode    *undershoot_node[4];

  Indicator hindicator;
  Indicator vindicator;

  CtkCornerType  window_placement;
  guint16  shadow_type;

  guint    hscrollbar_policy        : 2;
  guint    vscrollbar_policy        : 2;
  guint    hscrollbar_visible       : 1;
  guint    vscrollbar_visible       : 1;
  guint    focus_out                : 1; /* used by ::move-focus-out implementation */
  guint    overlay_scrolling        : 1;
  guint    use_indicators           : 1;
  guint    auto_added_viewport      : 1;
  guint    propagate_natural_width  : 1;
  guint    propagate_natural_height : 1;

  gint     min_content_width;
  gint     min_content_height;
  gint     max_content_width;
  gint     max_content_height;

  guint scroll_events_overshoot_id;

  /* Kinetic scrolling */
  CtkGesture *long_press_gesture;
  CtkGesture *swipe_gesture;
  CtkKineticScrolling *hscrolling;
  CtkKineticScrolling *vscrolling;
  gint64 last_deceleration_time;

  GArray *scroll_history;
  CdkDevice *scroll_device;
  CdkWindow *scroll_window;
  CdkCursor *scroll_cursor;

  /* These two gestures are mutually exclusive */
  CtkGesture *drag_gesture;
  CtkGesture *pan_gesture;

  gdouble drag_start_x;
  gdouble drag_start_y;

  CdkDevice             *drag_device;
  guint                  kinetic_scrolling         : 1;
  guint                  capture_button_press      : 1;
  guint                  in_drag                   : 1;

  guint                  deceleration_id;

  gdouble                x_velocity;
  gdouble                y_velocity;

  gdouble                unclamped_hadj_value;
  gdouble                unclamped_vadj_value;
};

enum {
  PROP_0,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_HSCROLLBAR_POLICY,
  PROP_VSCROLLBAR_POLICY,
  PROP_WINDOW_PLACEMENT,
  PROP_WINDOW_PLACEMENT_SET,
  PROP_SHADOW_TYPE,
  PROP_MIN_CONTENT_WIDTH,
  PROP_MIN_CONTENT_HEIGHT,
  PROP_KINETIC_SCROLLING,
  PROP_OVERLAY_SCROLLING,
  PROP_MAX_CONTENT_WIDTH,
  PROP_MAX_CONTENT_HEIGHT,
  PROP_PROPAGATE_NATURAL_WIDTH,
  PROP_PROPAGATE_NATURAL_HEIGHT,
  NUM_PROPERTIES
};

/* Signals */
enum
{
  SCROLL_CHILD,
  MOVE_FOCUS_OUT,
  EDGE_OVERSHOT,
  EDGE_REACHED,
  LAST_SIGNAL
};

static void     ctk_scrolled_window_set_property       (GObject           *object,
                                                        guint              prop_id,
                                                        const GValue      *value,
                                                        GParamSpec        *pspec);
static void     ctk_scrolled_window_get_property       (GObject           *object,
                                                        guint              prop_id,
                                                        GValue            *value,
                                                        GParamSpec        *pspec);
static void     ctk_scrolled_window_finalize           (GObject           *object);

static void     ctk_scrolled_window_destroy            (CtkWidget         *widget);
static gboolean ctk_scrolled_window_draw               (CtkWidget         *widget,
                                                        cairo_t           *cr);
static void     ctk_scrolled_window_size_allocate      (CtkWidget         *widget,
                                                        CtkAllocation     *allocation);
static gboolean ctk_scrolled_window_scroll_event       (CtkWidget         *widget,
                                                        CdkEventScroll    *event);
static gboolean ctk_scrolled_window_focus              (CtkWidget         *widget,
                                                        CtkDirectionType   direction);
static void     ctk_scrolled_window_add                (CtkContainer      *container,
                                                        CtkWidget         *widget);
static void     ctk_scrolled_window_remove             (CtkContainer      *container,
                                                        CtkWidget         *widget);
static void     ctk_scrolled_window_forall             (CtkContainer      *container,
                                                        gboolean           include_internals,
                                                        CtkCallback        callback,
                                                        gpointer           callback_data);
static gboolean ctk_scrolled_window_scroll_child       (CtkScrolledWindow *scrolled_window,
                                                        CtkScrollType      scroll,
                                                        gboolean           horizontal);
static void     ctk_scrolled_window_move_focus_out     (CtkScrolledWindow *scrolled_window,
                                                        CtkDirectionType   direction_type);

static void     ctk_scrolled_window_relative_allocation(CtkWidget         *widget,
                                                        CtkAllocation     *allocation);
static void     ctk_scrolled_window_inner_allocation   (CtkWidget         *widget,
                                                        CtkAllocation     *rect);
static void     ctk_scrolled_window_allocate_scrollbar (CtkScrolledWindow *scrolled_window,
                                                        CtkWidget         *scrollbar,
                                                        CtkAllocation     *allocation);
static void     ctk_scrolled_window_allocate_child     (CtkScrolledWindow *swindow,
                                                        CtkAllocation     *relative_allocation);
static void     ctk_scrolled_window_adjustment_changed (CtkAdjustment     *adjustment,
                                                        gpointer           data);
static void     ctk_scrolled_window_adjustment_value_changed (CtkAdjustment     *adjustment,
                                                              gpointer           data);
static gboolean ctk_widget_should_animate              (CtkWidget           *widget);

static void  ctk_scrolled_window_get_preferred_width   (CtkWidget           *widget,
							gint                *minimum_size,
							gint                *natural_size);
static void  ctk_scrolled_window_get_preferred_height  (CtkWidget           *widget,
							gint                *minimum_size,
							gint                *natural_size);
static void  ctk_scrolled_window_get_preferred_height_for_width  (CtkWidget           *layout,
							gint                 width,
							gint                *minimum_height,
							gint                *natural_height);
static void  ctk_scrolled_window_get_preferred_width_for_height  (CtkWidget           *layout,
							gint                 width,
							gint                *minimum_height,
							gint                *natural_height);

static void  ctk_scrolled_window_map                   (CtkWidget           *widget);
static void  ctk_scrolled_window_unmap                 (CtkWidget           *widget);
static void  ctk_scrolled_window_realize               (CtkWidget           *widget);
static void  ctk_scrolled_window_unrealize             (CtkWidget           *widget);

static void  ctk_scrolled_window_grab_notify           (CtkWidget           *widget,
                                                        gboolean             was_grabbed);

static void _ctk_scrolled_window_set_adjustment_value  (CtkScrolledWindow *scrolled_window,
                                                        CtkAdjustment     *adjustment,
                                                        gdouble            value);

static void ctk_scrolled_window_cancel_deceleration (CtkScrolledWindow *scrolled_window);

static gboolean _ctk_scrolled_window_get_overshoot (CtkScrolledWindow *scrolled_window,
                                                    gint              *overshoot_x,
                                                    gint              *overshoot_y);

static void     ctk_scrolled_window_start_deceleration (CtkScrolledWindow *scrolled_window);
static gint     _ctk_scrolled_window_get_scrollbar_spacing (CtkScrolledWindow *scrolled_window);

static void     ctk_scrolled_window_update_use_indicators (CtkScrolledWindow *scrolled_window);
static void     remove_indicator     (CtkScrolledWindow *sw,
                                      Indicator         *indicator);
static void     indicator_stop_fade  (Indicator         *indicator);
static gboolean maybe_hide_indicator (gpointer data);

static void     indicator_start_fade (Indicator *indicator,
                                      gdouble    pos);
static void     indicator_set_over   (Indicator *indicator,
                                      gboolean   over);
static void     uninstall_scroll_cursor (CtkScrolledWindow *scrolled_window);


static guint signals[LAST_SIGNAL] = {0};
static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE_WITH_PRIVATE (CtkScrolledWindow, ctk_scrolled_window, CTK_TYPE_BIN)

static void
add_scroll_binding (CtkBindingSet  *binding_set,
		    guint           keyval,
		    CdkModifierType mask,
		    CtkScrollType   scroll,
		    gboolean        horizontal)
{
  guint keypad_keyval = keyval - CDK_KEY_Left + CDK_KEY_KP_Left;
  
  ctk_binding_entry_add_signal (binding_set, keyval, mask,
                                "scroll-child", 2,
                                CTK_TYPE_SCROLL_TYPE, scroll,
				G_TYPE_BOOLEAN, horizontal);
  ctk_binding_entry_add_signal (binding_set, keypad_keyval, mask,
                                "scroll-child", 2,
                                CTK_TYPE_SCROLL_TYPE, scroll,
				G_TYPE_BOOLEAN, horizontal);
}

static void
add_tab_bindings (CtkBindingSet    *binding_set,
		  CdkModifierType   modifiers,
		  CtkDirectionType  direction)
{
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Tab, modifiers,
                                "move-focus-out", 1,
                                CTK_TYPE_DIRECTION_TYPE, direction);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Tab, modifiers,
                                "move-focus-out", 1,
                                CTK_TYPE_DIRECTION_TYPE, direction);
}

static gboolean
ctk_scrolled_window_leave_notify (CtkWidget        *widget,
                                  CdkEventCrossing *event)
{
  CtkScrolledWindowPrivate *priv = CTK_SCROLLED_WINDOW (widget)->priv;

  if (priv->use_indicators && event->detail != CDK_NOTIFY_INFERIOR)
    {
      indicator_set_over (&priv->hindicator, FALSE);
      indicator_set_over (&priv->vindicator, FALSE);
    }

  return CDK_EVENT_PROPAGATE;
}

static void
update_scrollbar_positions (CtkScrolledWindow *scrolled_window)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CtkStyleContext *context;
  gboolean is_rtl;

  if (priv->hscrollbar != NULL)
    {
      context = ctk_widget_get_style_context (priv->hscrollbar);
      if (priv->window_placement == CTK_CORNER_TOP_LEFT ||
          priv->window_placement == CTK_CORNER_TOP_RIGHT)
        {
          ctk_style_context_add_class (context, CTK_STYLE_CLASS_BOTTOM);
          ctk_style_context_remove_class (context, CTK_STYLE_CLASS_TOP);
        }
      else
        {
          ctk_style_context_remove_class (context, CTK_STYLE_CLASS_BOTTOM);
          ctk_style_context_add_class (context, CTK_STYLE_CLASS_TOP);
        }
    }

  if (priv->vscrollbar != NULL)
    {
      context = ctk_widget_get_style_context (priv->vscrollbar);
      is_rtl = ctk_widget_get_direction (CTK_WIDGET (scrolled_window)) == CTK_TEXT_DIR_RTL;
      if ((is_rtl &&
          (priv->window_placement == CTK_CORNER_TOP_RIGHT ||
           priv->window_placement == CTK_CORNER_BOTTOM_RIGHT)) ||
         (!is_rtl &&
          (priv->window_placement == CTK_CORNER_TOP_LEFT ||
           priv->window_placement == CTK_CORNER_BOTTOM_LEFT)))
        {
          ctk_style_context_add_class (context, CTK_STYLE_CLASS_RIGHT);
          ctk_style_context_remove_class (context, CTK_STYLE_CLASS_LEFT);
        }
      else
        {
          ctk_style_context_remove_class (context, CTK_STYLE_CLASS_RIGHT);
          ctk_style_context_add_class (context, CTK_STYLE_CLASS_LEFT);
        }
    }
}

static void
ctk_scrolled_window_direction_changed (CtkWidget        *widget,
                                       CtkTextDirection  previous_dir)
{
  CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (widget);

  update_scrollbar_positions (scrolled_window);

  CTK_WIDGET_CLASS (ctk_scrolled_window_parent_class)->direction_changed (widget, previous_dir);
}

static void
ctk_scrolled_window_class_init (CtkScrolledWindowClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;
  CtkBindingSet *binding_set;

  widget_class = (CtkWidgetClass*) class;
  container_class = (CtkContainerClass*) class;

  gobject_class->set_property = ctk_scrolled_window_set_property;
  gobject_class->get_property = ctk_scrolled_window_get_property;
  gobject_class->finalize = ctk_scrolled_window_finalize;

  widget_class->destroy = ctk_scrolled_window_destroy;
  widget_class->draw = ctk_scrolled_window_draw;
  widget_class->size_allocate = ctk_scrolled_window_size_allocate;
  widget_class->scroll_event = ctk_scrolled_window_scroll_event;
  widget_class->focus = ctk_scrolled_window_focus;
  widget_class->get_preferred_width = ctk_scrolled_window_get_preferred_width;
  widget_class->get_preferred_height = ctk_scrolled_window_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_scrolled_window_get_preferred_height_for_width;
  widget_class->get_preferred_width_for_height = ctk_scrolled_window_get_preferred_width_for_height;
  widget_class->map = ctk_scrolled_window_map;
  widget_class->unmap = ctk_scrolled_window_unmap;
  widget_class->grab_notify = ctk_scrolled_window_grab_notify;
  widget_class->realize = ctk_scrolled_window_realize;
  widget_class->unrealize = ctk_scrolled_window_unrealize;
  widget_class->leave_notify_event = ctk_scrolled_window_leave_notify;
  widget_class->direction_changed = ctk_scrolled_window_direction_changed;

  container_class->add = ctk_scrolled_window_add;
  container_class->remove = ctk_scrolled_window_remove;
  container_class->forall = ctk_scrolled_window_forall;
  ctk_container_class_handle_border_width (container_class);

  class->scrollbar_spacing = -1;

  class->scroll_child = ctk_scrolled_window_scroll_child;
  class->move_focus_out = ctk_scrolled_window_move_focus_out;

  properties[PROP_HADJUSTMENT] =
      g_param_spec_object ("hadjustment",
                           P_("Horizontal Adjustment"),
                           P_("The CtkAdjustment for the horizontal position"),
                           CTK_TYPE_ADJUSTMENT,
                           CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT);

  properties[PROP_VADJUSTMENT] =
      g_param_spec_object ("vadjustment",
                           P_("Vertical Adjustment"),
                           P_("The CtkAdjustment for the vertical position"),
                           CTK_TYPE_ADJUSTMENT,
                           CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT);

  properties[PROP_HSCROLLBAR_POLICY] =
      g_param_spec_enum ("hscrollbar-policy",
                         P_("Horizontal Scrollbar Policy"),
                         P_("When the horizontal scrollbar is displayed"),
                         CTK_TYPE_POLICY_TYPE,
                         CTK_POLICY_AUTOMATIC,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_VSCROLLBAR_POLICY] =
      g_param_spec_enum ("vscrollbar-policy",
                         P_("Vertical Scrollbar Policy"),
                         P_("When the vertical scrollbar is displayed"),
			CTK_TYPE_POLICY_TYPE,
			CTK_POLICY_AUTOMATIC,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_WINDOW_PLACEMENT] =
      g_param_spec_enum ("window-placement",
                         P_("Window Placement"),
                         P_("Where the contents are located with respect to the scrollbars."),
			CTK_TYPE_CORNER_TYPE,
			CTK_CORNER_TOP_LEFT,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkScrolledWindow:window-placement-set:
   *
   * Whether "window-placement" should be used to determine the location
   * of the contents with respect to the scrollbars.
   *
   * Since: 2.10
   *
   * Deprecated: 3.10: This value is ignored and
   * #CtkScrolledWindow:window-placement value is always honored.
   */
  properties[PROP_WINDOW_PLACEMENT_SET] =
      g_param_spec_boolean ("window-placement-set",
                            P_("Window Placement Set"),
                            P_("Whether \"window-placement\" should be used to determine the location of the contents with respect to the scrollbars."),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_SHADOW_TYPE] =
      g_param_spec_enum ("shadow-type",
                         P_("Shadow Type"),
                         P_("Style of bevel around the contents"),
			CTK_TYPE_SHADOW_TYPE,
			CTK_SHADOW_NONE,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkScrolledWindow:scrollbars-within-bevel:
   *
   * Whether to place scrollbars within the scrolled window's bevel.
   *
   * Since: 2.12
   *
   * Deprecated: 3.20: the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("scrollbars-within-bevel",
							         P_("Scrollbars within bevel"),
							         P_("Place scrollbars within the scrolled window's bevel"),
							         FALSE,
							         CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("scrollbar-spacing",
							     P_("Scrollbar spacing"),
							     P_("Number of pixels between the scrollbars and the scrolled window"),
							     0,
							     G_MAXINT,
							     DEFAULT_SCROLLBAR_SPACING,
							     CTK_PARAM_READABLE));

  /**
   * CtkScrolledWindow:min-content-width:
   *
   * The minimum content width of @scrolled_window, or -1 if not set.
   *
   * Since: 3.0
   */
  properties[PROP_MIN_CONTENT_WIDTH] =
      g_param_spec_int ("min-content-width",
                        P_("Minimum Content Width"),
                        P_("The minimum width that the scrolled window will allocate to its content"),
                        -1, G_MAXINT, -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkScrolledWindow:min-content-height:
   *
   * The minimum content height of @scrolled_window, or -1 if not set.
   *
   * Since: 3.0
   */
  properties[PROP_MIN_CONTENT_HEIGHT] =
      g_param_spec_int ("min-content-height",
                        P_("Minimum Content Height"),
                        P_("The minimum height that the scrolled window will allocate to its content"),
                        -1, G_MAXINT, -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkScrolledWindow:kinetic-scrolling:
   *
   * Whether kinetic scrolling is enabled or not. Kinetic scrolling
   * only applies to devices with source %CDK_SOURCE_TOUCHSCREEN.
   *
   * Since: 3.4
   */
  properties[PROP_KINETIC_SCROLLING] =
      g_param_spec_boolean ("kinetic-scrolling",
                            P_("Kinetic Scrolling"),
                            P_("Kinetic scrolling mode."),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkScrolledWindow:overlay-scrolling:
   *
   * Whether overlay scrolling is enabled or not. If it is, the
   * scrollbars are only added as traditional widgets when a mouse
   * is present. Otherwise, they are overlayed on top of the content,
   * as narrow indicators.
   *
   * Note that overlay scrolling can also be globally disabled, with
   * the #CtkSettings::ctk-overlay-scrolling setting.
   *
   * Since: 3.16
   */
  properties[PROP_OVERLAY_SCROLLING] =
      g_param_spec_boolean ("overlay-scrolling",
                            P_("Overlay Scrolling"),
                            P_("Overlay scrolling mode"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkScrolledWindow:max-content-width:
   *
   * The maximum content width of @scrolled_window, or -1 if not set.
   *
   * Since: 3.22
   */
  properties[PROP_MAX_CONTENT_WIDTH] =
      g_param_spec_int ("max-content-width",
                        P_("Maximum Content Width"),
                        P_("The maximum width that the scrolled window will allocate to its content"),
                        -1, G_MAXINT, -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkScrolledWindow:max-content-height:
   *
   * The maximum content height of @scrolled_window, or -1 if not set.
   *
   * Since: 3.22
   */
  properties[PROP_MAX_CONTENT_HEIGHT] =
      g_param_spec_int ("max-content-height",
                        P_("Maximum Content Height"),
                        P_("The maximum height that the scrolled window will allocate to its content"),
                        -1, G_MAXINT, -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkScrolledWindow:propagate-natural-width:
   *
   * Whether the natural width of the child should be calculated and propagated
   * through the scrolled window’s requested natural width.
   *
   * This is useful in cases where an attempt should be made to allocate exactly
   * enough space for the natural size of the child.
   *
   * Since: 3.22
   */
  properties[PROP_PROPAGATE_NATURAL_WIDTH] =
      g_param_spec_boolean ("propagate-natural-width",
                            P_("Propagate Natural Width"),
                            P_("Propagate Natural Width"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkScrolledWindow:propagate-natural-height:
   *
   * Whether the natural height of the child should be calculated and propagated
   * through the scrolled window’s requested natural height.
   *
   * This is useful in cases where an attempt should be made to allocate exactly
   * enough space for the natural size of the child.
   *
   * Since: 3.22
   */
  properties[PROP_PROPAGATE_NATURAL_HEIGHT] =
      g_param_spec_boolean ("propagate-natural-height",
                            P_("Propagate Natural Height"),
                            P_("Propagate Natural Height"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, properties);

  /**
   * CtkScrolledWindow::scroll-child:
   * @scrolled_window: a #CtkScrolledWindow
   * @scroll: a #CtkScrollType describing how much to scroll
   * @horizontal: whether the keybinding scrolls the child
   *   horizontally or not
   *
   * The ::scroll-child signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted when a keybinding that scrolls is pressed.
   * The horizontal or vertical adjustment is updated which triggers a
   * signal that the scrolled window’s child may listen to and scroll itself.
   */
  signals[SCROLL_CHILD] =
    g_signal_new (I_("scroll-child"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkScrolledWindowClass, scroll_child),
                  NULL, NULL,
                  _ctk_marshal_BOOLEAN__ENUM_BOOLEAN,
                  G_TYPE_BOOLEAN, 2,
                  CTK_TYPE_SCROLL_TYPE,
		  G_TYPE_BOOLEAN);

  /**
   * CtkScrolledWindow::move-focus-out:
   * @scrolled_window: a #CtkScrolledWindow
   * @direction_type: either %CTK_DIR_TAB_FORWARD or
   *   %CTK_DIR_TAB_BACKWARD
   *
   * The ::move-focus-out signal is a
   * [keybinding signal][CtkBindingSignal] which gets
   * emitted when focus is moved away from the scrolled window by a
   * keybinding. The #CtkWidget::move-focus signal is emitted with
   * @direction_type on this scrolled window’s toplevel parent in the
   * container hierarchy. The default bindings for this signal are
   * `Ctrl + Tab` to move forward and `Ctrl + Shift + Tab` to move backward.
   */
  signals[MOVE_FOCUS_OUT] =
    g_signal_new (I_("move-focus-out"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkScrolledWindowClass, move_focus_out),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CTK_TYPE_DIRECTION_TYPE);

  /**
   * CtkScrolledWindow::edge-overshot:
   * @scrolled_window: a #CtkScrolledWindow
   * @pos: edge side that was hit
   *
   * The ::edge-overshot signal is emitted whenever user initiated scrolling
   * makes the scrolled window firmly surpass (i.e. with some edge resistance)
   * the lower or upper limits defined by the adjustment in that orientation.
   *
   * A similar behavior without edge resistance is provided by the
   * #CtkScrolledWindow::edge-reached signal.
   *
   * Note: The @pos argument is LTR/RTL aware, so callers should be aware too
   * if intending to provide behavior on horizontal edges.
   *
   * Since: 3.16
   */
  signals[EDGE_OVERSHOT] =
    g_signal_new (I_("edge-overshot"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST, 0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, CTK_TYPE_POSITION_TYPE);

  /**
   * CtkScrolledWindow::edge-reached:
   * @scrolled_window: a #CtkScrolledWindow
   * @pos: edge side that was reached
   *
   * The ::edge-reached signal is emitted whenever user-initiated scrolling
   * makes the scrolled window exactly reach the lower or upper limits
   * defined by the adjustment in that orientation.
   *
   * A similar behavior with edge resistance is provided by the
   * #CtkScrolledWindow::edge-overshot signal.
   *
   * Note: The @pos argument is LTR/RTL aware, so callers should be aware too
   * if intending to provide behavior on horizontal edges.
   *
   * Since: 3.16
   */
  signals[EDGE_REACHED] =
    g_signal_new (I_("edge-reached"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST, 0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, CTK_TYPE_POSITION_TYPE);

  binding_set = ctk_binding_set_by_class (class);

  add_scroll_binding (binding_set, CDK_KEY_Left,  CDK_CONTROL_MASK, CTK_SCROLL_STEP_BACKWARD, TRUE);
  add_scroll_binding (binding_set, CDK_KEY_Right, CDK_CONTROL_MASK, CTK_SCROLL_STEP_FORWARD,  TRUE);
  add_scroll_binding (binding_set, CDK_KEY_Up,    CDK_CONTROL_MASK, CTK_SCROLL_STEP_BACKWARD, FALSE);
  add_scroll_binding (binding_set, CDK_KEY_Down,  CDK_CONTROL_MASK, CTK_SCROLL_STEP_FORWARD,  FALSE);

  add_scroll_binding (binding_set, CDK_KEY_Page_Up,   CDK_CONTROL_MASK, CTK_SCROLL_PAGE_BACKWARD, TRUE);
  add_scroll_binding (binding_set, CDK_KEY_Page_Down, CDK_CONTROL_MASK, CTK_SCROLL_PAGE_FORWARD,  TRUE);
  add_scroll_binding (binding_set, CDK_KEY_Page_Up,   0,                CTK_SCROLL_PAGE_BACKWARD, FALSE);
  add_scroll_binding (binding_set, CDK_KEY_Page_Down, 0,                CTK_SCROLL_PAGE_FORWARD,  FALSE);

  add_scroll_binding (binding_set, CDK_KEY_Home, CDK_CONTROL_MASK, CTK_SCROLL_START, TRUE);
  add_scroll_binding (binding_set, CDK_KEY_End,  CDK_CONTROL_MASK, CTK_SCROLL_END,   TRUE);
  add_scroll_binding (binding_set, CDK_KEY_Home, 0,                CTK_SCROLL_START, FALSE);
  add_scroll_binding (binding_set, CDK_KEY_End,  0,                CTK_SCROLL_END,   FALSE);

  add_tab_bindings (binding_set, CDK_CONTROL_MASK, CTK_DIR_TAB_FORWARD);
  add_tab_bindings (binding_set, CDK_CONTROL_MASK | CDK_SHIFT_MASK, CTK_DIR_TAB_BACKWARD);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_SCROLLED_WINDOW_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "scrolledwindow");
}

static gboolean
may_hscroll (CtkScrolledWindow *scrolled_window)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  return priv->hscrollbar_visible || priv->hscrollbar_policy == CTK_POLICY_EXTERNAL;
}

static gboolean
may_vscroll (CtkScrolledWindow *scrolled_window)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  return priv->vscrollbar_visible || priv->vscrollbar_policy == CTK_POLICY_EXTERNAL;
}

static inline gboolean
policy_may_be_visible (CtkPolicyType policy)
{
  return policy == CTK_POLICY_ALWAYS || policy == CTK_POLICY_AUTOMATIC;
}

static void
scrolled_window_drag_begin_cb (CtkScrolledWindow *scrolled_window,
                               gdouble            start_x G_GNUC_UNUSED,
                               gdouble            start_y G_GNUC_UNUSED,
                               CtkGesture        *gesture)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CtkEventSequenceState state;
  CdkEventSequence *sequence;
  CtkWidget *event_widget;
  const CdkEvent *event;

  priv->in_drag = FALSE;
  priv->drag_start_x = priv->unclamped_hadj_value;
  priv->drag_start_y = priv->unclamped_vadj_value;
  ctk_scrolled_window_cancel_deceleration (scrolled_window);
  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (gesture, sequence);
  event_widget = ctk_get_event_widget ((CdkEvent *) event);

  if (event_widget == priv->vscrollbar || event_widget == priv->hscrollbar ||
      (!may_hscroll (scrolled_window) && !may_vscroll (scrolled_window)))
    state = CTK_EVENT_SEQUENCE_DENIED;
  else if (priv->capture_button_press)
    state = CTK_EVENT_SEQUENCE_CLAIMED;
  else
    return;

  ctk_gesture_set_sequence_state (gesture, sequence, state);
}

static void
ctk_scrolled_window_invalidate_overshoot (CtkScrolledWindow *scrolled_window)
{
  CtkAllocation child_allocation;
  gint overshoot_x, overshoot_y;
  CdkRectangle rect;

  if (!_ctk_scrolled_window_get_overshoot (scrolled_window, &overshoot_x, &overshoot_y))
    return;

  ctk_scrolled_window_relative_allocation (CTK_WIDGET (scrolled_window),
                                           &child_allocation);
  if (overshoot_x != 0)
    {
      if (overshoot_x < 0)
        rect.x = child_allocation.x;
      else
        rect.x = child_allocation.x + child_allocation.width - MAX_OVERSHOOT_DISTANCE;

      rect.y = child_allocation.y;
      rect.width = MAX_OVERSHOOT_DISTANCE;
      rect.height = child_allocation.height;

      cdk_window_invalidate_rect (ctk_widget_get_window (CTK_WIDGET (scrolled_window)),
                                  &rect, TRUE);
    }

  if (overshoot_y != 0)
    {
      if (overshoot_y < 0)
        rect.y = child_allocation.y;
      else
        rect.y = child_allocation.y + child_allocation.height - MAX_OVERSHOOT_DISTANCE;

      rect.x = child_allocation.x;
      rect.width = child_allocation.width;
      rect.height = MAX_OVERSHOOT_DISTANCE;

      cdk_window_invalidate_rect (ctk_widget_get_window (CTK_WIDGET (scrolled_window)),
                                  &rect, TRUE);
    }
}

static void
scrolled_window_drag_update_cb (CtkScrolledWindow *scrolled_window,
                                gdouble            offset_x,
                                gdouble            offset_y,
                                CtkGesture        *gesture)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CtkAdjustment *hadjustment;
  CtkAdjustment *vadjustment;
  gdouble dx, dy;

  ctk_scrolled_window_invalidate_overshoot (scrolled_window);

  if (!priv->capture_button_press)
    {
      CdkEventSequence *sequence;

      sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
      ctk_gesture_set_sequence_state (gesture, sequence,
                                      CTK_EVENT_SEQUENCE_CLAIMED);
    }

  hadjustment = ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar));
  if (hadjustment && may_hscroll (scrolled_window))
    {
      dx = priv->drag_start_x - offset_x;
      _ctk_scrolled_window_set_adjustment_value (scrolled_window,
                                                 hadjustment, dx);
    }

  vadjustment = ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar));
  if (vadjustment && may_vscroll (scrolled_window))
    {
      dy = priv->drag_start_y - offset_y;
      _ctk_scrolled_window_set_adjustment_value (scrolled_window,
                                                 vadjustment, dy);
    }

  ctk_scrolled_window_invalidate_overshoot (scrolled_window);
}

static void
scrolled_window_drag_end_cb (CtkScrolledWindow *scrolled_window,
                             CdkEventSequence  *sequence,
                             CtkGesture        *gesture)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  if (!priv->in_drag || !ctk_gesture_handles_sequence (gesture, sequence))
    ctk_gesture_set_state (gesture, CTK_EVENT_SEQUENCE_DENIED);
}

static void
ctk_scrolled_window_decelerate (CtkScrolledWindow *scrolled_window,
                                gdouble            x_velocity,
                                gdouble            y_velocity)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  gboolean overshoot;

  overshoot = _ctk_scrolled_window_get_overshoot (scrolled_window, NULL, NULL);
  priv->x_velocity = x_velocity;
  priv->y_velocity = y_velocity;

  /* Zero out vector components for which we don't scroll */
  if (!may_hscroll (scrolled_window))
    priv->x_velocity = 0;
  if (!may_vscroll (scrolled_window))
    priv->y_velocity = 0;

  if (priv->x_velocity != 0 || priv->y_velocity != 0 || overshoot)
    {
      ctk_scrolled_window_start_deceleration (scrolled_window);
      priv->x_velocity = priv->y_velocity = 0;
    }
}

static void
scrolled_window_swipe_cb (CtkScrolledWindow *scrolled_window,
                          gdouble            x_velocity,
                          gdouble            y_velocity)
{
  ctk_scrolled_window_decelerate (scrolled_window, -x_velocity, -y_velocity);
}

static void
scrolled_window_long_press_cb (CtkScrolledWindow *scrolled_window G_GNUC_UNUSED,
                               gdouble            x G_GNUC_UNUSED,
                               gdouble            y G_GNUC_UNUSED,
                               CtkGesture        *gesture)
{
  CdkEventSequence *sequence;

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  ctk_gesture_set_sequence_state (gesture, sequence,
                                  CTK_EVENT_SEQUENCE_DENIED);
}

static void
scrolled_window_long_press_cancelled_cb (CtkScrolledWindow *scrolled_window,
                                         CtkGesture        *gesture)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CdkEventSequence *sequence;
  const CdkEvent *event;

  sequence = ctk_gesture_get_last_updated_sequence (gesture);
  event = ctk_gesture_get_last_event (gesture, sequence);

  if (event->type == CDK_TOUCH_BEGIN ||
      event->type == CDK_BUTTON_PRESS)
    ctk_gesture_set_sequence_state (gesture, sequence,
                                    CTK_EVENT_SEQUENCE_DENIED);
  else if (event->type != CDK_TOUCH_END &&
           event->type != CDK_BUTTON_RELEASE)
    priv->in_drag = TRUE;
}

static void
ctk_scrolled_window_check_attach_pan_gesture (CtkScrolledWindow *sw)
{
  CtkPropagationPhase phase = CTK_PHASE_NONE;
  CtkScrolledWindowPrivate *priv = sw->priv;

  if (priv->kinetic_scrolling &&
      ((may_hscroll (sw) && !may_vscroll (sw)) ||
       (!may_hscroll (sw) && may_vscroll (sw))))
    {
      CtkOrientation orientation;

      if (may_hscroll (sw))
        orientation = CTK_ORIENTATION_HORIZONTAL;
      else
        orientation = CTK_ORIENTATION_VERTICAL;

      ctk_gesture_pan_set_orientation (CTK_GESTURE_PAN (priv->pan_gesture),
                                       orientation);
      phase = CTK_PHASE_CAPTURE;
    }

  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->pan_gesture), phase);
}

static void
indicator_set_over (Indicator *indicator,
                    gboolean   over)
{
  CtkStyleContext *context;

  if (indicator->over_timeout_id)
    {
      g_source_remove (indicator->over_timeout_id);
      indicator->over_timeout_id = 0;
    }

  if (indicator->over == over)
    return;

  context = ctk_widget_get_style_context (indicator->scrollbar);
  indicator->over = over;

  if (indicator->over)
    ctk_style_context_add_class (context, "hovering");
  else
    ctk_style_context_remove_class (context, "hovering");

  ctk_widget_queue_resize (indicator->scrollbar);
}

static void
translate_to_widget (CtkWidget *widget,
                     CdkEvent  *event,
                     gint      *x,
                     gint      *y)
{
  CtkWidget *event_widget;
  CdkWindow *event_widget_window;
  CdkWindow *window;
  gdouble event_x, event_y;
  gint wx, wy;
  CtkAllocation allocation;

  event_widget = ctk_get_event_widget (event);
  event_widget_window = ctk_widget_get_window (event_widget);
  cdk_event_get_coords (event, &event_x, &event_y);
  window = event->any.window;
  while (window && window != event_widget_window)
    {
      cdk_window_get_position (window, &wx, &wy);
      event_x += wx;
      event_y += wy;
      window = cdk_window_get_effective_parent (window);
    }

  if (!ctk_widget_get_has_window (event_widget))
    {
      ctk_widget_get_allocation (event_widget, &allocation);
      event_x -= allocation.x;
      event_y -= allocation.y;
    }

  ctk_widget_translate_coordinates (event_widget, widget,
                                    (gint)event_x, (gint)event_y,
                                    x, y);
}

static gboolean
event_close_to_indicator (CtkScrolledWindow *sw,
                          Indicator         *indicator,
                          CdkEvent          *event)
{
  CtkScrolledWindowPrivate *priv;
  CtkAllocation indicator_alloc;
  gint x, y;
  gint distance;
  gint win_x, win_y;

  priv = sw->priv;

  ctk_widget_get_allocation (indicator->scrollbar, &indicator_alloc);
  cdk_window_get_position (indicator->window, &win_x, &win_y);
  translate_to_widget (CTK_WIDGET (sw), event, &x, &y);

  if (indicator->over)
    distance = INDICATOR_FAR_DISTANCE;
  else
    distance = INDICATOR_CLOSE_DISTANCE;

  if (indicator == &priv->hindicator)
    {
       if (y >= win_y - distance &&
           y < win_y + indicator_alloc.height + distance)
         return TRUE;
    }
  else if (indicator == &priv->vindicator)
    {
      if (x >= win_x - distance &&
          x < win_x + indicator_alloc.width + distance)
        return TRUE;
    }

  return FALSE;
}

static gboolean
enable_over_timeout_cb (gpointer user_data)
{
  Indicator *indicator = user_data;

  indicator_set_over (indicator, TRUE);
  return G_SOURCE_REMOVE;
}

static gboolean
check_update_scrollbar_proximity (CtkScrolledWindow *sw,
                                  Indicator         *indicator,
                                  CdkEvent          *event)
{
  CtkScrolledWindowPrivate *priv = sw->priv;
  gboolean indicator_close, on_scrollbar, on_other_scrollbar;
  CtkWidget *event_widget;

  event_widget = ctk_get_event_widget (event);

  indicator_close = event_close_to_indicator (sw, indicator, event);
  on_scrollbar = (event_widget == indicator->scrollbar &&
                  event->type != CDK_LEAVE_NOTIFY);
  on_other_scrollbar = (!on_scrollbar &&
                        event->type != CDK_LEAVE_NOTIFY &&
                        (event_widget == priv->hindicator.scrollbar ||
                         event_widget == priv->vindicator.scrollbar));

  if (indicator->over_timeout_id)
    {
      g_source_remove (indicator->over_timeout_id);
      indicator->over_timeout_id = 0;
    }

  if (on_scrollbar)
    indicator_set_over (indicator, TRUE);
  else if (indicator_close && !on_other_scrollbar)
    indicator->over_timeout_id = cdk_threads_add_timeout (30, enable_over_timeout_cb, indicator);
  else
    indicator_set_over (indicator, FALSE);

  return indicator_close;
}

static gdouble
get_scroll_unit (CtkScrolledWindow *sw,
                 CtkOrientation     orientation)
{
  gdouble scroll_unit;

#ifndef CDK_WINDOWING_QUARTZ
  CtkScrolledWindowPrivate *priv = sw->priv;
  CtkRange *scrollbar;
  CtkAdjustment *adj;
  gdouble page_size;
  gdouble pow_unit;

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    scrollbar = CTK_RANGE (priv->hscrollbar);
  else
    scrollbar = CTK_RANGE (priv->vscrollbar);

  if (!scrollbar)
    return 0;

  adj = ctk_range_get_adjustment (scrollbar);
  page_size = ctk_adjustment_get_page_size (adj);

  /* see comment in _ctk_range_get_wheel_delta() */
  pow_unit = pow (page_size, 2.0 / 3.0);
  scroll_unit = MIN (pow_unit, page_size / 2.0);
#else
  scroll_unit = 1;
#endif

  return scroll_unit;
}

static void
scroll_history_push (CtkScrolledWindow *sw,
                     CdkEventScroll    *event,
                     gboolean           shifted)
{
  CtkScrolledWindowPrivate *priv = sw->priv;
  ScrollHistoryElem new_item;
  guint i;

  if (event->direction != CDK_SCROLL_SMOOTH)
    return;

  for (i = 0; i < priv->scroll_history->len; i++)
    {
      ScrollHistoryElem *elem;

      elem = &g_array_index (priv->scroll_history, ScrollHistoryElem, i);

      if (elem->evtime >= event->time - SCROLL_CAPTURE_THRESHOLD_MS)
        break;
    }

  if (i > 0)
    g_array_remove_range (priv->scroll_history, 0, i);

  if (shifted)
    {
      new_item.dx = event->delta_y;
      new_item.dy = event->delta_x;
    }
  else
    {
      new_item.dx = event->delta_x;
      new_item.dy = event->delta_y;
    }
  new_item.evtime = event->time;
  g_array_append_val (priv->scroll_history, new_item);
}

static void
scroll_history_reset (CtkScrolledWindow *sw)
{
  CtkScrolledWindowPrivate *priv = sw->priv;

  if (priv->scroll_history->len == 0)
    return;

  g_array_remove_range (priv->scroll_history, 0,
                        priv->scroll_history->len);
}

static gboolean
scroll_history_finish (CtkScrolledWindow *sw,
                       gdouble           *velocity_x,
                       gdouble           *velocity_y)
{
  CtkScrolledWindowPrivate *priv = sw->priv;
  gdouble accum_dx = 0, accum_dy = 0;
  guint32 first = 0, last = 0;
  gdouble xunit, yunit;
  guint i;

  if (priv->scroll_history->len == 0)
    return FALSE;

  for (i = 0; i < priv->scroll_history->len; i++)
    {
      ScrollHistoryElem *elem;

      elem = &g_array_index (priv->scroll_history, ScrollHistoryElem, i);
      accum_dx += elem->dx;
      accum_dy += elem->dy;
      last = elem->evtime;

      if (i == 0)
        first = elem->evtime;
    }

  if (last == first)
    {
      scroll_history_reset (sw);
      return FALSE;
    }

  xunit = get_scroll_unit (sw, CTK_ORIENTATION_HORIZONTAL);
  yunit = get_scroll_unit (sw, CTK_ORIENTATION_VERTICAL);
  *velocity_x = (accum_dx * 1000 * xunit) / (last - first);
  *velocity_y = (accum_dy * 1000 * yunit) / (last - first);
  scroll_history_reset (sw);

  return TRUE;
}

static void uninstall_scroll_cursor (CtkScrolledWindow *scrolled_window);

static gboolean
captured_event_cb (CtkWidget *widget,
                   CdkEvent  *event)
{
  CtkScrolledWindowPrivate *priv;
  CtkScrolledWindow *sw;
  CdkInputSource input_source;
  CdkDevice *source_device;
  CtkWidget *event_widget;
  gboolean on_scrollbar;

  sw = CTK_SCROLLED_WINDOW (widget);
  priv = sw->priv;
  source_device = cdk_event_get_source_device (event);

  if (event->type == CDK_SCROLL)
    {
      CtkWidget *scrollable_child = ctk_bin_get_child (CTK_BIN (widget));

      ctk_scrolled_window_cancel_deceleration (sw);

      /* If a nested widget takes over the scroll, unset our scrolling cursor */
      if (ctk_get_event_widget (event) != scrollable_child)
        uninstall_scroll_cursor (sw);

      return CDK_EVENT_PROPAGATE;
    }

  if (!priv->use_indicators)
    return CDK_EVENT_PROPAGATE;

  if (event->type != CDK_MOTION_NOTIFY &&
      event->type != CDK_LEAVE_NOTIFY)
    return CDK_EVENT_PROPAGATE;

  input_source = cdk_device_get_source (source_device);

  if (input_source == CDK_SOURCE_KEYBOARD ||
      input_source == CDK_SOURCE_TOUCHSCREEN)
    return CDK_EVENT_PROPAGATE;

  event_widget = ctk_get_event_widget (event);
  on_scrollbar = (event_widget == priv->hindicator.scrollbar ||
                  event_widget == priv->vindicator.scrollbar);

  if (event->type == CDK_MOTION_NOTIFY)
    {
      if (priv->hscrollbar_visible)
        indicator_start_fade (&priv->hindicator, 1.0);
      if (priv->vscrollbar_visible)
        indicator_start_fade (&priv->vindicator, 1.0);

      if (!on_scrollbar &&
           (event->motion.state &
            (CDK_BUTTON1_MASK | CDK_BUTTON2_MASK | CDK_BUTTON3_MASK)) != 0)
        {
          indicator_set_over (&priv->hindicator, FALSE);
          indicator_set_over (&priv->vindicator, FALSE);
        }
      else if (input_source == CDK_SOURCE_PEN ||
               input_source == CDK_SOURCE_ERASER ||
               input_source == CDK_SOURCE_TRACKPOINT)
        {
          indicator_set_over (&priv->hindicator, TRUE);
          indicator_set_over (&priv->vindicator, TRUE);
        }
      else
        {
          if (!check_update_scrollbar_proximity (sw, &priv->vindicator, event))
            check_update_scrollbar_proximity (sw, &priv->hindicator, event);
          else
            indicator_set_over (&priv->hindicator, FALSE);
        }
    }
  else if (event->type == CDK_LEAVE_NOTIFY && on_scrollbar &&
           event->crossing.mode == CDK_CROSSING_UNGRAB)
    {
      check_update_scrollbar_proximity (sw, &priv->vindicator, event);
      check_update_scrollbar_proximity (sw, &priv->hindicator, event);
    }

  return CDK_EVENT_PROPAGATE;
}

/*
 * _ctk_scrolled_window_get_spacing:
 * @scrolled_window: a scrolled window
 *
 * Gets the spacing between the scrolled window’s scrollbars and
 * the scrolled widget. Used by CtkCombo
 *
 * Returns: the spacing, in pixels.
 */
static gint
_ctk_scrolled_window_get_scrollbar_spacing (CtkScrolledWindow *scrolled_window)
{
  CtkScrolledWindowClass *class;

  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), 0);

  class = CTK_SCROLLED_WINDOW_GET_CLASS (scrolled_window);

  if (class->scrollbar_spacing >= 0)
    return class->scrollbar_spacing;
  else
    {
      gint scrollbar_spacing;

      ctk_widget_style_get (CTK_WIDGET (scrolled_window),
			    "scrollbar-spacing", &scrollbar_spacing,
			    NULL);

      return scrollbar_spacing;
    }
}

static void
ctk_scrolled_window_allocate (CtkCssGadget        *gadget,
                              const CtkAllocation *allocation,
                              int                  baseline G_GNUC_UNUSED,
                              CtkAllocation       *out_clip G_GNUC_UNUSED,
                              gpointer             data G_GNUC_UNUSED)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (widget);
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CtkBin *bin;
  CtkAllocation relative_allocation;
  CtkAllocation child_allocation;
  CtkWidget *child;
  gint sb_spacing;
  gint sb_width;
  gint sb_height;

  bin = CTK_BIN (scrolled_window);

  /* Get possible scrollbar dimensions */
  sb_spacing = _ctk_scrolled_window_get_scrollbar_spacing (scrolled_window);
  ctk_widget_get_preferred_height (priv->hscrollbar, &sb_height, NULL);
  ctk_widget_get_preferred_width (priv->vscrollbar, &sb_width, NULL);

  if (priv->hscrollbar_policy == CTK_POLICY_ALWAYS)
    priv->hscrollbar_visible = TRUE;
  else if (priv->hscrollbar_policy == CTK_POLICY_NEVER ||
           priv->hscrollbar_policy == CTK_POLICY_EXTERNAL)
    priv->hscrollbar_visible = FALSE;

  if (priv->vscrollbar_policy == CTK_POLICY_ALWAYS)
    priv->vscrollbar_visible = TRUE;
  else if (priv->vscrollbar_policy == CTK_POLICY_NEVER ||
           priv->vscrollbar_policy == CTK_POLICY_EXTERNAL)
    priv->vscrollbar_visible = FALSE;

  child = ctk_bin_get_child (bin);
  if (child && ctk_widget_get_visible (child))
    {
      gint child_scroll_width;
      gint child_scroll_height;
      gboolean previous_hvis;
      gboolean previous_vvis;
      guint count = 0;
      CtkScrollable *scrollable_child = CTK_SCROLLABLE (child);
      CtkScrollablePolicy hscroll_policy = ctk_scrollable_get_hscroll_policy (scrollable_child);
      CtkScrollablePolicy vscroll_policy = ctk_scrollable_get_vscroll_policy (scrollable_child);

      /* Determine scrollbar visibility first via hfw apis */
      if (ctk_widget_get_request_mode (child) == CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH)
	{
	  if (hscroll_policy == CTK_SCROLL_MINIMUM)
	    ctk_widget_get_preferred_width (child, &child_scroll_width, NULL);
	  else
	    ctk_widget_get_preferred_width (child, NULL, &child_scroll_width);

	  if (priv->vscrollbar_policy == CTK_POLICY_AUTOMATIC)
	    {
	      /* First try without a vertical scrollbar if the content will fit the height
	       * given the extra width of the scrollbar */
	      if (vscroll_policy == CTK_SCROLL_MINIMUM)
		ctk_widget_get_preferred_height_for_width (child,
							   MAX (allocation->width, child_scroll_width),
							   &child_scroll_height, NULL);
	      else
		ctk_widget_get_preferred_height_for_width (child,
							   MAX (allocation->width, child_scroll_width),
							   NULL, &child_scroll_height);

	      if (priv->hscrollbar_policy == CTK_POLICY_AUTOMATIC)
		{
		  /* Does the content height fit the allocation height ? */
		  priv->vscrollbar_visible = child_scroll_height > allocation->height;

		  /* Does the content width fit the allocation with minus a possible scrollbar ? */
		  priv->hscrollbar_visible =
		    child_scroll_width > allocation->width -
		    (priv->vscrollbar_visible && !priv->use_indicators ? sb_width + sb_spacing : 0);

		  /* Now that we've guessed the hscrollbar, does the content height fit
		   * the possible new allocation height ?
		   */
		  priv->vscrollbar_visible =
		    child_scroll_height > allocation->height -
		    (priv->hscrollbar_visible && !priv->use_indicators ? sb_height + sb_spacing : 0);

		  /* Now that we've guessed the vscrollbar, does the content width fit
		   * the possible new allocation width ?
		   */
		  priv->hscrollbar_visible =
		    child_scroll_width > allocation->width -
		    (priv->vscrollbar_visible && !priv->use_indicators ? sb_width + sb_spacing : 0);
		}
	      else /* priv->hscrollbar_policy != CTK_POLICY_AUTOMATIC */
		{
		  priv->hscrollbar_visible = policy_may_be_visible (priv->hscrollbar_policy);
		  priv->vscrollbar_visible = child_scroll_height > allocation->height -
		    (priv->hscrollbar_visible && !priv->use_indicators ? sb_height + sb_spacing : 0);
		}
	    }
	  else /* priv->vscrollbar_policy != CTK_POLICY_AUTOMATIC */
	    {
	      priv->vscrollbar_visible = policy_may_be_visible (priv->vscrollbar_policy);

	      if (priv->hscrollbar_policy == CTK_POLICY_AUTOMATIC)
		priv->hscrollbar_visible =
		  child_scroll_width > allocation->width -
		  (priv->vscrollbar_visible && !priv->use_indicators ? 0 : sb_width + sb_spacing);
	      else
		priv->hscrollbar_visible = policy_may_be_visible (priv->hscrollbar_policy);
	    }
	}
      else /* CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT */
	{
	  if (vscroll_policy == CTK_SCROLL_MINIMUM)
	    ctk_widget_get_preferred_height (child, &child_scroll_height, NULL);
	  else
	    ctk_widget_get_preferred_height (child, NULL, &child_scroll_height);

	  if (priv->hscrollbar_policy == CTK_POLICY_AUTOMATIC)
	    {
	      /* First try without a horizontal scrollbar if the content will fit the width
	       * given the extra height of the scrollbar */
	      if (hscroll_policy == CTK_SCROLL_MINIMUM)
		ctk_widget_get_preferred_width_for_height (child,
							   MAX (allocation->height, child_scroll_height),
							   &child_scroll_width, NULL);
	      else
		ctk_widget_get_preferred_width_for_height (child,
							   MAX (allocation->height, child_scroll_height),
							   NULL, &child_scroll_width);

	      if (priv->vscrollbar_policy == CTK_POLICY_AUTOMATIC)
		{
		  /* Does the content width fit the allocation width ? */
		  priv->hscrollbar_visible = child_scroll_width > allocation->width;

		  /* Does the content height fit the allocation with minus a possible scrollbar ? */
		  priv->vscrollbar_visible =
		    child_scroll_height > allocation->height -
		    (priv->hscrollbar_visible && !priv->use_indicators ? sb_height + sb_spacing : 0);

		  /* Now that we've guessed the vscrollbar, does the content width fit
		   * the possible new allocation width ?
		   */
		  priv->hscrollbar_visible =
		    child_scroll_width > allocation->width -
		    (priv->vscrollbar_visible && !priv->use_indicators ? sb_width + sb_spacing : 0);

		  /* Now that we've guessed the hscrollbar, does the content height fit
		   * the possible new allocation height ?
		   */
		  priv->vscrollbar_visible =
		    child_scroll_height > allocation->height -
		    (priv->hscrollbar_visible && !priv->use_indicators ? sb_height + sb_spacing : 0);
		}
	      else /* priv->vscrollbar_policy != CTK_POLICY_AUTOMATIC */
		{
		  priv->vscrollbar_visible = policy_may_be_visible (priv->vscrollbar_policy);
		  priv->hscrollbar_visible = child_scroll_width > allocation->width -
		    (priv->vscrollbar_visible && !priv->use_indicators ? sb_width + sb_spacing : 0);
		}
	    }
	  else /* priv->hscrollbar_policy != CTK_POLICY_AUTOMATIC */
	    {
	      priv->hscrollbar_visible = policy_may_be_visible (priv->hscrollbar_policy);

	      if (priv->vscrollbar_policy == CTK_POLICY_AUTOMATIC)
		priv->vscrollbar_visible =
		  child_scroll_height > allocation->height -
		  (priv->hscrollbar_visible && !priv->use_indicators ? sb_height + sb_spacing : 0);
	      else
		priv->vscrollbar_visible = policy_may_be_visible (priv->vscrollbar_policy);
	    }
	}

      /* Now after guessing scrollbar visibility; fall back on the allocation loop which
       * observes the adjustments to detect scrollbar visibility and also avoids
       * infinite recursion
       */
      do
	{
	  previous_hvis = priv->hscrollbar_visible;
	  previous_vvis = priv->vscrollbar_visible;
	  ctk_scrolled_window_allocate_child (scrolled_window, &relative_allocation);

	  /* Explicitly force scrollbar visibility checks.
	   *
	   * Since we make a guess above, the child might not decide to update the adjustments
	   * if they logically did not change since the last configuration
	   */
	  if (priv->hscrollbar)
	    ctk_scrolled_window_adjustment_changed
              (ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar)), scrolled_window);

	  if (priv->vscrollbar)
	    ctk_scrolled_window_adjustment_changed
              (ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar)), scrolled_window);

	  /* If, after the first iteration, the hscrollbar and the
	   * vscrollbar flip visiblity... or if one of the scrollbars flip
	   * on each itteration indefinitly/infinitely, then we just need both
	   * at this size.
	   */
	  if ((count &&
	       previous_hvis != priv->hscrollbar_visible &&
	       previous_vvis != priv->vscrollbar_visible) || count > 3)
	    {
	      priv->hscrollbar_visible = TRUE;
	      priv->vscrollbar_visible = TRUE;

	      ctk_scrolled_window_allocate_child (scrolled_window, &relative_allocation);

	      break;
	    }

	  count++;
	}
      while (previous_hvis != priv->hscrollbar_visible ||
	     previous_vvis != priv->vscrollbar_visible);
    }
  else
    {
      priv->hscrollbar_visible = priv->hscrollbar_policy == CTK_POLICY_ALWAYS;
      priv->vscrollbar_visible = priv->vscrollbar_policy == CTK_POLICY_ALWAYS;
    }

  ctk_widget_set_child_visible (priv->hscrollbar, priv->hscrollbar_visible);
  if (priv->hscrollbar_visible)
    {
      ctk_scrolled_window_allocate_scrollbar (scrolled_window,
                                              priv->hscrollbar,
                                              &child_allocation);
      if (priv->use_indicators)
        {
	  if (ctk_widget_get_realized (widget))
	    cdk_window_move_resize (priv->hindicator.window,
				    child_allocation.x,
				    child_allocation.y,
				    child_allocation.width,
				    child_allocation.height);
          child_allocation.x = 0;
          child_allocation.y = 0;
        }
      ctk_widget_size_allocate (priv->hscrollbar, &child_allocation);
    }

  ctk_widget_set_child_visible (priv->vscrollbar, priv->vscrollbar_visible);
  if (priv->vscrollbar_visible)
    {
      ctk_scrolled_window_allocate_scrollbar (scrolled_window,
                                              priv->vscrollbar,
                                              &child_allocation);
      if (priv->use_indicators)
        {
	  if (ctk_widget_get_realized (widget))
	    cdk_window_move_resize (priv->vindicator.window,
				    child_allocation.x,
				    child_allocation.y,
				    child_allocation.width,
				    child_allocation.height);
          child_allocation.x = 0;
          child_allocation.y = 0;
        }
      ctk_widget_size_allocate (priv->vscrollbar, &child_allocation);
    }

  ctk_scrolled_window_check_attach_pan_gesture (scrolled_window);
}

static void
ctk_scrolled_window_measure (CtkCssGadget   *gadget,
                             CtkOrientation  orientation,
                             int             for_size G_GNUC_UNUSED,
                             int            *minimum_size,
                             int            *natural_size,
                             int            *minimum_baseline G_GNUC_UNUSED,
                             int            *natural_baseline G_GNUC_UNUSED,
                             gpointer        data G_GNUC_UNUSED)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (widget);
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CtkBin *bin = CTK_BIN (scrolled_window);
  gint scrollbar_spacing;
  CtkRequisition hscrollbar_requisition;
  CtkRequisition vscrollbar_requisition;
  CtkRequisition minimum_req, natural_req;
  CtkWidget *child;
  gint min_child_size, nat_child_size;
  CtkBorder sborder = { 0 };

  scrollbar_spacing = _ctk_scrolled_window_get_scrollbar_spacing (scrolled_window);

  minimum_req.width = 0;
  minimum_req.height = 0;
  natural_req.width = 0;
  natural_req.height = 0;

  ctk_widget_get_preferred_size (priv->hscrollbar,
                                 &hscrollbar_requisition, NULL);
  ctk_widget_get_preferred_size (priv->vscrollbar,
                                 &vscrollbar_requisition, NULL);

  child = ctk_bin_get_child (bin);

  if (child)
    ctk_scrollable_get_border (CTK_SCROLLABLE (child), &sborder);

  /*
   * First collect the child requisition
   */
  if (child && ctk_widget_get_visible (child))
    {
      if (orientation == CTK_ORIENTATION_HORIZONTAL)
	{
	  ctk_widget_get_preferred_width (child,
                                          &min_child_size,
                                          &nat_child_size);

	  if (priv->propagate_natural_width)
	    natural_req.width += nat_child_size;

	  if (priv->hscrollbar_policy == CTK_POLICY_NEVER)
	    {
	      minimum_req.width += min_child_size;
	    }
	  else
	    {
	      gint min = priv->min_content_width >= 0 ? priv->min_content_width : 0;
	      gint max = priv->max_content_width >= 0 ? priv->max_content_width : G_MAXINT;

	      minimum_req.width = CLAMP (minimum_req.width, min, max);
	      natural_req.width = CLAMP (natural_req.width, min, max);
	    }
	}
      else /* CTK_ORIENTATION_VERTICAL */
	{
	  ctk_widget_get_preferred_height (child,
                                           &min_child_size,
                                           &nat_child_size);

	  if (priv->propagate_natural_height)
	    natural_req.height += nat_child_size;

	  if (priv->vscrollbar_policy == CTK_POLICY_NEVER)
	    {
	      minimum_req.height += min_child_size;
	    }
	  else
	    {
	      gint min = priv->min_content_height >= 0 ? priv->min_content_height : 0;
	      gint max = priv->max_content_height >= 0 ? priv->max_content_height : G_MAXINT;

	      minimum_req.height = CLAMP (minimum_req.height, min, max);
	      natural_req.height = CLAMP (natural_req.height, min, max);
	    }
	}
    }

  /* Ensure we make requests with natural size >= minimum size */
  natural_req.height = MAX (minimum_req.height, natural_req.height);
  natural_req.width  = MAX (minimum_req.width,  natural_req.width);

  /*
   * Now add to the requisition any additional space for surrounding scrollbars
   * and the special scrollable border.
   */
  if (policy_may_be_visible (priv->hscrollbar_policy))
    {
      minimum_req.width = MAX (minimum_req.width, hscrollbar_requisition.width + sborder.left + sborder.right);
      natural_req.width = MAX (natural_req.width, hscrollbar_requisition.width + sborder.left + sborder.right);

      if (!priv->use_indicators && priv->hscrollbar_policy == CTK_POLICY_ALWAYS)
	{
	  minimum_req.height += scrollbar_spacing + hscrollbar_requisition.height;
	  natural_req.height += scrollbar_spacing + hscrollbar_requisition.height;
	}
    }

  if (policy_may_be_visible (priv->vscrollbar_policy))
    {
      minimum_req.height = MAX (minimum_req.height, vscrollbar_requisition.height + sborder.top + sborder.bottom);
      natural_req.height = MAX (natural_req.height, vscrollbar_requisition.height + sborder.top + sborder.bottom);

      if (!priv->use_indicators && priv->vscrollbar_policy == CTK_POLICY_ALWAYS)
	{
	  minimum_req.width += scrollbar_spacing + vscrollbar_requisition.width;
	  natural_req.width += scrollbar_spacing + vscrollbar_requisition.width;
	}
    }

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      *minimum_size = minimum_req.width;
      *natural_size = natural_req.width;
    }
  else
    {
      *minimum_size = minimum_req.height;
      *natural_size = natural_req.height;
    }
}

static void
ctk_scrolled_window_draw_scrollbars_junction (CtkScrolledWindow *scrolled_window,
                                              cairo_t *cr)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CtkWidget *widget = CTK_WIDGET (scrolled_window);
  CtkAllocation content_allocation, hscr_allocation, vscr_allocation;
  CtkStyleContext *context;
  CdkRectangle junction_rect;
  gboolean is_rtl;

  is_rtl = ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL;
  ctk_widget_get_allocation (CTK_WIDGET (priv->hscrollbar), &hscr_allocation);
  ctk_widget_get_allocation (CTK_WIDGET (priv->vscrollbar), &vscr_allocation);
  ctk_css_gadget_get_content_allocation (priv->gadget, &content_allocation,
                                         NULL);

  junction_rect.x = content_allocation.x;
  junction_rect.y = content_allocation.y;
  junction_rect.width = vscr_allocation.width;
  junction_rect.height = hscr_allocation.height;

  if ((is_rtl &&
       (priv->window_placement == CTK_CORNER_TOP_RIGHT ||
        priv->window_placement == CTK_CORNER_BOTTOM_RIGHT)) ||
      (!is_rtl &&
       (priv->window_placement == CTK_CORNER_TOP_LEFT ||
        priv->window_placement == CTK_CORNER_BOTTOM_LEFT)))
    junction_rect.x += hscr_allocation.width;

  if (priv->window_placement == CTK_CORNER_TOP_LEFT ||
      priv->window_placement == CTK_CORNER_TOP_RIGHT)
    junction_rect.y += vscr_allocation.height;

  context = ctk_widget_get_style_context (widget);
  ctk_style_context_save_named (context, "junction");

  ctk_render_background (context, cr,
                         junction_rect.x, junction_rect.y,
                         junction_rect.width, junction_rect.height);
  ctk_render_frame (context, cr,
                    junction_rect.x, junction_rect.y,
                    junction_rect.width, junction_rect.height);

  ctk_style_context_restore (context);
}

static void
ctk_scrolled_window_draw_overshoot (CtkScrolledWindow *scrolled_window,
				    cairo_t           *cr)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CtkWidget *widget = CTK_WIDGET (scrolled_window);
  gint overshoot_x, overshoot_y;
  CtkStyleContext *context;
  CdkRectangle rect;

  if (!_ctk_scrolled_window_get_overshoot (scrolled_window, &overshoot_x, &overshoot_y))
    return;

  context = ctk_widget_get_style_context (widget);
  ctk_scrolled_window_inner_allocation (widget, &rect);

  overshoot_x = CLAMP (overshoot_x, - MAX_OVERSHOOT_DISTANCE, MAX_OVERSHOOT_DISTANCE);
  overshoot_y = CLAMP (overshoot_y, - MAX_OVERSHOOT_DISTANCE, MAX_OVERSHOOT_DISTANCE);

  if (overshoot_x > 0)
    {
      ctk_style_context_save_to_node (context, priv->overshoot_node[CTK_POS_RIGHT]);
      ctk_render_background (context, cr, rect.x + rect.width - overshoot_x, rect.y, overshoot_x, rect.height);
      ctk_render_frame (context, cr, rect.x + rect.width - overshoot_x, rect.y, overshoot_x, rect.height);
      ctk_style_context_restore (context);
    }
  else if (overshoot_x < 0)
    {
      ctk_style_context_save_to_node (context, priv->overshoot_node[CTK_POS_LEFT]);
      ctk_render_background (context, cr, rect.x, rect.y, -overshoot_x, rect.height);
      ctk_render_frame (context, cr, rect.x, rect.y, -overshoot_x, rect.height);
      ctk_style_context_restore (context);
    }

  if (overshoot_y > 0)
    {
      ctk_style_context_save_to_node (context, priv->overshoot_node[CTK_POS_BOTTOM]);
      ctk_render_background (context, cr, rect.x, rect.y + rect.height - overshoot_y, rect.width, overshoot_y);
      ctk_render_frame (context, cr, rect.x, rect.y + rect.height - overshoot_y, rect.width, overshoot_y);
      ctk_style_context_restore (context);
    }
  else if (overshoot_y < 0)
    {
      ctk_style_context_save_to_node (context, priv->overshoot_node[CTK_POS_TOP]);
      ctk_render_background (context, cr, rect.x, rect.y, rect.width, -overshoot_y);
      ctk_render_frame (context, cr, rect.x, rect.y, rect.width, -overshoot_y);
      ctk_style_context_restore (context);
    }
}

static void
ctk_scrolled_window_draw_undershoot (CtkScrolledWindow *scrolled_window,
                                     cairo_t           *cr)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CtkWidget *widget = CTK_WIDGET (scrolled_window);
  CtkStyleContext *context;
  CdkRectangle rect;
  CtkAdjustment *adj;

  context = ctk_widget_get_style_context (widget);
  ctk_scrolled_window_inner_allocation (widget, &rect);

  adj = ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar));
  if (ctk_adjustment_get_value (adj) < ctk_adjustment_get_upper (adj) - ctk_adjustment_get_page_size (adj))
    {
      ctk_style_context_save_to_node (context, priv->undershoot_node[CTK_POS_RIGHT]);
      ctk_render_background (context, cr, rect.x + rect.width - UNDERSHOOT_SIZE, rect.y, UNDERSHOOT_SIZE, rect.height);
      ctk_render_frame (context, cr, rect.x + rect.width - UNDERSHOOT_SIZE, rect.y, UNDERSHOOT_SIZE, rect.height);

      ctk_style_context_restore (context);
    }
  if (ctk_adjustment_get_value (adj) > ctk_adjustment_get_lower (adj))
    {
      ctk_style_context_save_to_node (context, priv->undershoot_node[CTK_POS_LEFT]);
      ctk_render_background (context, cr, rect.x, rect.y, UNDERSHOOT_SIZE, rect.height);
      ctk_render_frame (context, cr, rect.x, rect.y, UNDERSHOOT_SIZE, rect.height);
      ctk_style_context_restore (context);
    }

  adj = ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar));
  if (ctk_adjustment_get_value (adj) < ctk_adjustment_get_upper (adj) - ctk_adjustment_get_page_size (adj))
    {
      ctk_style_context_save_to_node (context, priv->undershoot_node[CTK_POS_BOTTOM]);
      ctk_render_background (context, cr, rect.x, rect.y + rect.height - UNDERSHOOT_SIZE, rect.width, UNDERSHOOT_SIZE);
      ctk_render_frame (context, cr, rect.x, rect.y + rect.height - UNDERSHOOT_SIZE, rect.width, UNDERSHOOT_SIZE);
      ctk_style_context_restore (context);
    }
  if (ctk_adjustment_get_value (adj) > ctk_adjustment_get_lower (adj))
    {
      ctk_style_context_save_to_node (context, priv->undershoot_node[CTK_POS_TOP]);
      ctk_render_background (context, cr, rect.x, rect.y, rect.width, UNDERSHOOT_SIZE);
      ctk_render_frame (context, cr, rect.x, rect.y, rect.width, UNDERSHOOT_SIZE);
      ctk_style_context_restore (context);
    }
}

static gboolean
ctk_scrolled_window_render (CtkCssGadget *gadget,
                            cairo_t      *cr,
                            int           x G_GNUC_UNUSED,
                            int           y G_GNUC_UNUSED,
                            int           width G_GNUC_UNUSED,
                            int           height G_GNUC_UNUSED,
                            gpointer      data G_GNUC_UNUSED)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (widget);
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  if (ctk_cairo_should_draw_window (cr, ctk_widget_get_window (widget)))
    {
      if (priv->hscrollbar_visible &&
          priv->vscrollbar_visible)
        ctk_scrolled_window_draw_scrollbars_junction (scrolled_window, cr);
    }

  CTK_WIDGET_CLASS (ctk_scrolled_window_parent_class)->draw (widget, cr);

  if (ctk_cairo_should_draw_window (cr, ctk_widget_get_window (widget)))
    {
      ctk_scrolled_window_draw_undershoot (scrolled_window, cr);
      ctk_scrolled_window_draw_overshoot (scrolled_window, cr);
    }

  return FALSE;
}

static void
ctk_scrolled_window_init (CtkScrolledWindow *scrolled_window)
{
  CtkWidget *widget = CTK_WIDGET (scrolled_window);
  CtkScrolledWindowPrivate *priv;
  CtkCssNode *widget_node;
  GQuark classes[4] = {
    g_quark_from_static_string (CTK_STYLE_CLASS_LEFT),
    g_quark_from_static_string (CTK_STYLE_CLASS_RIGHT),
    g_quark_from_static_string (CTK_STYLE_CLASS_TOP),
    g_quark_from_static_string (CTK_STYLE_CLASS_BOTTOM),
  };
  gint i;

  scrolled_window->priv = priv =
    ctk_scrolled_window_get_instance_private (scrolled_window);

  ctk_widget_set_has_window (widget, TRUE);
  ctk_widget_set_can_focus (widget, TRUE);

  /* Instantiated by ctk_scrolled_window_set_[hv]adjustment
   * which are both construct properties
   */
  priv->hscrollbar = NULL;
  priv->vscrollbar = NULL;
  priv->hscrollbar_policy = CTK_POLICY_AUTOMATIC;
  priv->vscrollbar_policy = CTK_POLICY_AUTOMATIC;
  priv->hscrollbar_visible = FALSE;
  priv->vscrollbar_visible = FALSE;
  priv->focus_out = FALSE;
  priv->auto_added_viewport = FALSE;
  priv->window_placement = CTK_CORNER_TOP_LEFT;
  priv->min_content_width = -1;
  priv->min_content_height = -1;
  priv->max_content_width = -1;
  priv->max_content_height = -1;

  priv->overlay_scrolling = TRUE;

  priv->drag_gesture = ctk_gesture_drag_new (widget);
  ctk_gesture_single_set_touch_only (CTK_GESTURE_SINGLE (priv->drag_gesture), TRUE);
  g_signal_connect_swapped (priv->drag_gesture, "drag-begin",
                            G_CALLBACK (scrolled_window_drag_begin_cb),
                            scrolled_window);
  g_signal_connect_swapped (priv->drag_gesture, "drag-update",
                            G_CALLBACK (scrolled_window_drag_update_cb),
                            scrolled_window);
  g_signal_connect_swapped (priv->drag_gesture, "end",
                            G_CALLBACK (scrolled_window_drag_end_cb),
                            scrolled_window);

  priv->pan_gesture = ctk_gesture_pan_new (widget, CTK_ORIENTATION_VERTICAL);
  ctk_gesture_group (priv->pan_gesture, priv->drag_gesture);
  ctk_gesture_single_set_touch_only (CTK_GESTURE_SINGLE (priv->pan_gesture), TRUE);

  priv->swipe_gesture = ctk_gesture_swipe_new (widget);
  ctk_gesture_group (priv->swipe_gesture, priv->drag_gesture);
  ctk_gesture_single_set_touch_only (CTK_GESTURE_SINGLE (priv->swipe_gesture), TRUE);
  g_signal_connect_swapped (priv->swipe_gesture, "swipe",
                            G_CALLBACK (scrolled_window_swipe_cb),
                            scrolled_window);
  priv->long_press_gesture = ctk_gesture_long_press_new (widget);
  ctk_gesture_group (priv->long_press_gesture, priv->drag_gesture);
  ctk_gesture_single_set_touch_only (CTK_GESTURE_SINGLE (priv->long_press_gesture), TRUE);
  g_signal_connect_swapped (priv->long_press_gesture, "pressed",
                            G_CALLBACK (scrolled_window_long_press_cb),
                            scrolled_window);
  g_signal_connect_swapped (priv->long_press_gesture, "cancelled",
                            G_CALLBACK (scrolled_window_long_press_cancelled_cb),
                            scrolled_window);

  priv->scroll_history = g_array_new (FALSE, FALSE, sizeof (ScrollHistoryElem));

  ctk_scrolled_window_set_kinetic_scrolling (scrolled_window, TRUE);
  ctk_scrolled_window_set_capture_button_press (scrolled_window, TRUE);

  _ctk_widget_set_captured_event_handler (widget, captured_event_cb);

  widget_node = ctk_widget_get_css_node (widget);
  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     widget,
                                                     ctk_scrolled_window_measure,
                                                     ctk_scrolled_window_allocate,
                                                     ctk_scrolled_window_render,
                                                     NULL, NULL);
  for (i = 0; i < 4; i++)
    {
      priv->overshoot_node[i] = ctk_css_node_new ();
      ctk_css_node_set_name (priv->overshoot_node[i], I_("overshoot"));
      ctk_css_node_add_class (priv->overshoot_node[i], classes[i]);
      ctk_css_node_set_parent (priv->overshoot_node[i], widget_node);
      ctk_css_node_set_state (priv->overshoot_node[i], ctk_css_node_get_state (widget_node));
      g_object_unref (priv->overshoot_node[i]);

      priv->undershoot_node[i] = ctk_css_node_new ();
      ctk_css_node_set_name (priv->undershoot_node[i], I_("undershoot"));
      ctk_css_node_add_class (priv->undershoot_node[i], classes[i]);
      ctk_css_node_set_parent (priv->undershoot_node[i], widget_node);
      ctk_css_node_set_state (priv->undershoot_node[i], ctk_css_node_get_state (widget_node));
      g_object_unref (priv->undershoot_node[i]);
    }

  ctk_scrolled_window_update_use_indicators (scrolled_window);
}

/**
 * ctk_scrolled_window_new:
 * @hadjustment: (nullable): horizontal adjustment
 * @vadjustment: (nullable): vertical adjustment
 *
 * Creates a new scrolled window.
 *
 * The two arguments are the scrolled window’s adjustments; these will be
 * shared with the scrollbars and the child widget to keep the bars in sync
 * with the child. Usually you want to pass %NULL for the adjustments, which
 * will cause the scrolled window to create them for you.
 *
 * Returns: a new scrolled window
 */
CtkWidget*
ctk_scrolled_window_new (CtkAdjustment *hadjustment,
			 CtkAdjustment *vadjustment)
{
  CtkWidget *scrolled_window;

  if (hadjustment)
    g_return_val_if_fail (CTK_IS_ADJUSTMENT (hadjustment), NULL);

  if (vadjustment)
    g_return_val_if_fail (CTK_IS_ADJUSTMENT (vadjustment), NULL);

  scrolled_window = g_object_new (CTK_TYPE_SCROLLED_WINDOW,
				    "hadjustment", hadjustment,
				    "vadjustment", vadjustment,
				    NULL);

  return scrolled_window;
}

/**
 * ctk_scrolled_window_set_hadjustment:
 * @scrolled_window: a #CtkScrolledWindow
 * @hadjustment: (nullable): the #CtkAdjustment to use, or %NULL to create a new one
 *
 * Sets the #CtkAdjustment for the horizontal scrollbar.
 */
void
ctk_scrolled_window_set_hadjustment (CtkScrolledWindow *scrolled_window,
				     CtkAdjustment     *hadjustment)
{
  CtkScrolledWindowPrivate *priv;
  CtkBin *bin;
  CtkWidget *child;

  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));

  if (hadjustment)
    g_return_if_fail (CTK_IS_ADJUSTMENT (hadjustment));
  else
    hadjustment = (CtkAdjustment*) g_object_new (CTK_TYPE_ADJUSTMENT, NULL);

  bin = CTK_BIN (scrolled_window);
  priv = scrolled_window->priv;

  if (!priv->hscrollbar)
    {
      priv->hscrollbar = ctk_scrollbar_new (CTK_ORIENTATION_HORIZONTAL, hadjustment);

      ctk_widget_set_parent (priv->hscrollbar, CTK_WIDGET (scrolled_window));
      ctk_widget_show (priv->hscrollbar);
      update_scrollbar_positions (scrolled_window);
    }
  else
    {
      CtkAdjustment *old_adjustment;

      old_adjustment = ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar));
      if (old_adjustment == hadjustment)
	return;

      g_signal_handlers_disconnect_by_func (old_adjustment,
                                            ctk_scrolled_window_adjustment_changed,
                                            scrolled_window);
      g_signal_handlers_disconnect_by_func (old_adjustment,
                                            ctk_scrolled_window_adjustment_value_changed,
                                            scrolled_window);

      ctk_adjustment_enable_animation (old_adjustment, NULL, 0);
      ctk_range_set_adjustment (CTK_RANGE (priv->hscrollbar), hadjustment);
    }

  hadjustment = ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar));

  g_signal_connect (hadjustment,
                    "changed",
		    G_CALLBACK (ctk_scrolled_window_adjustment_changed),
		    scrolled_window);
  g_signal_connect (hadjustment,
                    "value-changed",
		    G_CALLBACK (ctk_scrolled_window_adjustment_value_changed),
		    scrolled_window);

  ctk_scrolled_window_adjustment_changed (hadjustment, scrolled_window);
  ctk_scrolled_window_adjustment_value_changed (hadjustment, scrolled_window);

  child = ctk_bin_get_child (bin);
  if (child)
    ctk_scrollable_set_hadjustment (CTK_SCROLLABLE (child), hadjustment);

  if (ctk_widget_should_animate (CTK_WIDGET (scrolled_window)))
    ctk_adjustment_enable_animation (hadjustment, ctk_widget_get_frame_clock (CTK_WIDGET (scrolled_window)), ANIMATION_DURATION);

  g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_HADJUSTMENT]);
}

/**
 * ctk_scrolled_window_set_vadjustment:
 * @scrolled_window: a #CtkScrolledWindow
 * @vadjustment: (nullable): the #CtkAdjustment to use, or %NULL to create a new one
 *
 * Sets the #CtkAdjustment for the vertical scrollbar.
 */
void
ctk_scrolled_window_set_vadjustment (CtkScrolledWindow *scrolled_window,
                                     CtkAdjustment     *vadjustment)
{
  CtkScrolledWindowPrivate *priv;
  CtkBin *bin;
  CtkWidget *child;

  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));

  if (vadjustment)
    g_return_if_fail (CTK_IS_ADJUSTMENT (vadjustment));
  else
    vadjustment = (CtkAdjustment*) g_object_new (CTK_TYPE_ADJUSTMENT, NULL);

  bin = CTK_BIN (scrolled_window);
  priv = scrolled_window->priv;

  if (!priv->vscrollbar)
    {
      priv->vscrollbar = ctk_scrollbar_new (CTK_ORIENTATION_VERTICAL, vadjustment);

      ctk_widget_set_parent (priv->vscrollbar, CTK_WIDGET (scrolled_window));
      ctk_widget_show (priv->vscrollbar);
      update_scrollbar_positions (scrolled_window);
    }
  else
    {
      CtkAdjustment *old_adjustment;
      
      old_adjustment = ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar));
      if (old_adjustment == vadjustment)
	return;

      g_signal_handlers_disconnect_by_func (old_adjustment,
                                            ctk_scrolled_window_adjustment_changed,
                                            scrolled_window);
      g_signal_handlers_disconnect_by_func (old_adjustment,
                                            ctk_scrolled_window_adjustment_value_changed,
                                            scrolled_window);

      ctk_adjustment_enable_animation (old_adjustment, NULL, 0);
      ctk_range_set_adjustment (CTK_RANGE (priv->vscrollbar), vadjustment);
    }

  vadjustment = ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar));

  g_signal_connect (vadjustment,
                    "changed",
		    G_CALLBACK (ctk_scrolled_window_adjustment_changed),
		    scrolled_window);
  g_signal_connect (vadjustment,
                    "value-changed",
		    G_CALLBACK (ctk_scrolled_window_adjustment_value_changed),
		    scrolled_window);

  ctk_scrolled_window_adjustment_changed (vadjustment, scrolled_window);
  ctk_scrolled_window_adjustment_value_changed (vadjustment, scrolled_window);

  child = ctk_bin_get_child (bin);
  if (child)
    ctk_scrollable_set_vadjustment (CTK_SCROLLABLE (child), vadjustment);

  if (ctk_widget_should_animate (CTK_WIDGET (scrolled_window)))
    ctk_adjustment_enable_animation (vadjustment, ctk_widget_get_frame_clock (CTK_WIDGET (scrolled_window)), ANIMATION_DURATION);

  g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_VADJUSTMENT]);
}

/**
 * ctk_scrolled_window_get_hadjustment:
 * @scrolled_window: a #CtkScrolledWindow
 *
 * Returns the horizontal scrollbar’s adjustment, used to connect the
 * horizontal scrollbar to the child widget’s horizontal scroll
 * functionality.
 *
 * Returns: (transfer none): the horizontal #CtkAdjustment
 */
CtkAdjustment*
ctk_scrolled_window_get_hadjustment (CtkScrolledWindow *scrolled_window)
{
  CtkScrolledWindowPrivate *priv;

  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), NULL);

  priv = scrolled_window->priv;

  return ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar));
}

/**
 * ctk_scrolled_window_get_vadjustment:
 * @scrolled_window: a #CtkScrolledWindow
 * 
 * Returns the vertical scrollbar’s adjustment, used to connect the
 * vertical scrollbar to the child widget’s vertical scroll functionality.
 * 
 * Returns: (transfer none): the vertical #CtkAdjustment
 */
CtkAdjustment*
ctk_scrolled_window_get_vadjustment (CtkScrolledWindow *scrolled_window)
{
  CtkScrolledWindowPrivate *priv;

  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), NULL);

  priv = scrolled_window->priv;

  return ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar));
}

/**
 * ctk_scrolled_window_get_hscrollbar:
 * @scrolled_window: a #CtkScrolledWindow
 *
 * Returns the horizontal scrollbar of @scrolled_window.
 *
 * Returns: (transfer none): the horizontal scrollbar of the scrolled window.
 *
 * Since: 2.8
 */
CtkWidget*
ctk_scrolled_window_get_hscrollbar (CtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), NULL);

  return scrolled_window->priv->hscrollbar;
}

/**
 * ctk_scrolled_window_get_vscrollbar:
 * @scrolled_window: a #CtkScrolledWindow
 * 
 * Returns the vertical scrollbar of @scrolled_window.
 *
 * Returns: (transfer none): the vertical scrollbar of the scrolled window.
 *
 * Since: 2.8
 */
CtkWidget*
ctk_scrolled_window_get_vscrollbar (CtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), NULL);

  return scrolled_window->priv->vscrollbar;
}

/**
 * ctk_scrolled_window_set_policy:
 * @scrolled_window: a #CtkScrolledWindow
 * @hscrollbar_policy: policy for horizontal bar
 * @vscrollbar_policy: policy for vertical bar
 * 
 * Sets the scrollbar policy for the horizontal and vertical scrollbars.
 *
 * The policy determines when the scrollbar should appear; it is a value
 * from the #CtkPolicyType enumeration. If %CTK_POLICY_ALWAYS, the
 * scrollbar is always present; if %CTK_POLICY_NEVER, the scrollbar is
 * never present; if %CTK_POLICY_AUTOMATIC, the scrollbar is present only
 * if needed (that is, if the slider part of the bar would be smaller
 * than the trough — the display is larger than the page size).
 */
void
ctk_scrolled_window_set_policy (CtkScrolledWindow *scrolled_window,
				CtkPolicyType      hscrollbar_policy,
				CtkPolicyType      vscrollbar_policy)
{
  CtkScrolledWindowPrivate *priv;
  GObject *object = G_OBJECT (scrolled_window);
  
  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));

  priv = scrolled_window->priv;

  if ((priv->hscrollbar_policy != hscrollbar_policy) ||
      (priv->vscrollbar_policy != vscrollbar_policy))
    {
      priv->hscrollbar_policy = hscrollbar_policy;
      priv->vscrollbar_policy = vscrollbar_policy;

      ctk_widget_queue_resize (CTK_WIDGET (scrolled_window));

      g_object_notify_by_pspec (object, properties[PROP_HSCROLLBAR_POLICY]);
      g_object_notify_by_pspec (object, properties[PROP_VSCROLLBAR_POLICY]);
    }
}

/**
 * ctk_scrolled_window_get_policy:
 * @scrolled_window: a #CtkScrolledWindow
 * @hscrollbar_policy: (out) (optional): location to store the policy
 *     for the horizontal scrollbar, or %NULL
 * @vscrollbar_policy: (out) (optional): location to store the policy
 *     for the vertical scrollbar, or %NULL
 * 
 * Retrieves the current policy values for the horizontal and vertical
 * scrollbars. See ctk_scrolled_window_set_policy().
 */
void
ctk_scrolled_window_get_policy (CtkScrolledWindow *scrolled_window,
				CtkPolicyType     *hscrollbar_policy,
				CtkPolicyType     *vscrollbar_policy)
{
  CtkScrolledWindowPrivate *priv;

  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));

  priv = scrolled_window->priv;

  if (hscrollbar_policy)
    *hscrollbar_policy = priv->hscrollbar_policy;
  if (vscrollbar_policy)
    *vscrollbar_policy = priv->vscrollbar_policy;
}

static void
ctk_scrolled_window_set_placement_internal (CtkScrolledWindow *scrolled_window,
					    CtkCornerType      window_placement)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  if (priv->window_placement != window_placement)
    {
      priv->window_placement = window_placement;
      update_scrollbar_positions (scrolled_window);

      ctk_widget_queue_resize (CTK_WIDGET (scrolled_window));

      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_WINDOW_PLACEMENT]);
    }
}

/**
 * ctk_scrolled_window_set_placement:
 * @scrolled_window: a #CtkScrolledWindow
 * @window_placement: position of the child window
 *
 * Sets the placement of the contents with respect to the scrollbars
 * for the scrolled window.
 * 
 * The default is %CTK_CORNER_TOP_LEFT, meaning the child is
 * in the top left, with the scrollbars underneath and to the right.
 * Other values in #CtkCornerType are %CTK_CORNER_TOP_RIGHT,
 * %CTK_CORNER_BOTTOM_LEFT, and %CTK_CORNER_BOTTOM_RIGHT.
 *
 * See also ctk_scrolled_window_get_placement() and
 * ctk_scrolled_window_unset_placement().
 */
void
ctk_scrolled_window_set_placement (CtkScrolledWindow *scrolled_window,
				   CtkCornerType      window_placement)
{
  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));

  ctk_scrolled_window_set_placement_internal (scrolled_window, window_placement);
}

/**
 * ctk_scrolled_window_get_placement:
 * @scrolled_window: a #CtkScrolledWindow
 *
 * Gets the placement of the contents with respect to the scrollbars
 * for the scrolled window. See ctk_scrolled_window_set_placement().
 *
 * Returns: the current placement value.
 *
 * See also ctk_scrolled_window_set_placement() and
 * ctk_scrolled_window_unset_placement().
 **/
CtkCornerType
ctk_scrolled_window_get_placement (CtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), CTK_CORNER_TOP_LEFT);

  return scrolled_window->priv->window_placement;
}

/**
 * ctk_scrolled_window_unset_placement:
 * @scrolled_window: a #CtkScrolledWindow
 *
 * Unsets the placement of the contents with respect to the scrollbars
 * for the scrolled window. If no window placement is set for a scrolled
 * window, it defaults to %CTK_CORNER_TOP_LEFT.
 *
 * See also ctk_scrolled_window_set_placement() and
 * ctk_scrolled_window_get_placement().
 *
 * Since: 2.10
 **/
void
ctk_scrolled_window_unset_placement (CtkScrolledWindow *scrolled_window)
{
  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));

  ctk_scrolled_window_set_placement_internal (scrolled_window, CTK_CORNER_TOP_LEFT);
}

/**
 * ctk_scrolled_window_set_shadow_type:
 * @scrolled_window: a #CtkScrolledWindow
 * @type: kind of shadow to draw around scrolled window contents
 *
 * Changes the type of shadow drawn around the contents of
 * @scrolled_window.
 **/
void
ctk_scrolled_window_set_shadow_type (CtkScrolledWindow *scrolled_window,
				     CtkShadowType      type)
{
  CtkScrolledWindowPrivate *priv;
  CtkStyleContext *context;

  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));
  g_return_if_fail (type >= CTK_SHADOW_NONE && type <= CTK_SHADOW_ETCHED_OUT);

  priv = scrolled_window->priv;

  if (priv->shadow_type != type)
    {
      priv->shadow_type = type;

      context = ctk_widget_get_style_context (CTK_WIDGET (scrolled_window));
      if (type != CTK_SHADOW_NONE)
        ctk_style_context_add_class (context, CTK_STYLE_CLASS_FRAME);
      else
        ctk_style_context_remove_class (context, CTK_STYLE_CLASS_FRAME);

      if (ctk_widget_is_drawable (CTK_WIDGET (scrolled_window)))
	ctk_widget_queue_draw (CTK_WIDGET (scrolled_window));

      ctk_widget_queue_resize (CTK_WIDGET (scrolled_window));

      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_SHADOW_TYPE]);
    }
}

/**
 * ctk_scrolled_window_get_shadow_type:
 * @scrolled_window: a #CtkScrolledWindow
 *
 * Gets the shadow type of the scrolled window. See 
 * ctk_scrolled_window_set_shadow_type().
 *
 * Returns: the current shadow type
 **/
CtkShadowType
ctk_scrolled_window_get_shadow_type (CtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), CTK_SHADOW_NONE);

  return scrolled_window->priv->shadow_type;
}

/**
 * ctk_scrolled_window_set_kinetic_scrolling:
 * @scrolled_window: a #CtkScrolledWindow
 * @kinetic_scrolling: %TRUE to enable kinetic scrolling
 *
 * Turns kinetic scrolling on or off.
 * Kinetic scrolling only applies to devices with source
 * %CDK_SOURCE_TOUCHSCREEN.
 *
 * Since: 3.4
 **/
void
ctk_scrolled_window_set_kinetic_scrolling (CtkScrolledWindow *scrolled_window,
                                           gboolean           kinetic_scrolling)
{
  CtkPropagationPhase phase = CTK_PHASE_NONE;
  CtkScrolledWindowPrivate *priv;

  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));

  priv = scrolled_window->priv;
  if (priv->kinetic_scrolling == kinetic_scrolling)
    return;

  priv->kinetic_scrolling = kinetic_scrolling;
  ctk_scrolled_window_check_attach_pan_gesture (scrolled_window);

  if (priv->kinetic_scrolling)
    phase = CTK_PHASE_CAPTURE;
  else
    ctk_scrolled_window_cancel_deceleration (scrolled_window);

  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->drag_gesture), phase);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->swipe_gesture), phase);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->long_press_gesture), phase);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->pan_gesture), phase);

  g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_KINETIC_SCROLLING]);
}

/**
 * ctk_scrolled_window_get_kinetic_scrolling:
 * @scrolled_window: a #CtkScrolledWindow
 *
 * Returns the specified kinetic scrolling behavior.
 *
 * Returns: the scrolling behavior flags.
 *
 * Since: 3.4
 */
gboolean
ctk_scrolled_window_get_kinetic_scrolling (CtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), FALSE);

  return scrolled_window->priv->kinetic_scrolling;
}

/**
 * ctk_scrolled_window_set_capture_button_press:
 * @scrolled_window: a #CtkScrolledWindow
 * @capture_button_press: %TRUE to capture button presses
 *
 * Changes the behaviour of @scrolled_window with regard to the initial
 * event that possibly starts kinetic scrolling. When @capture_button_press
 * is set to %TRUE, the event is captured by the scrolled window, and
 * then later replayed if it is meant to go to the child widget.
 *
 * This should be enabled if any child widgets perform non-reversible
 * actions on #CtkWidget::button-press-event. If they don't, and handle
 * additionally handle #CtkWidget::grab-broken-event, it might be better
 * to set @capture_button_press to %FALSE.
 *
 * This setting only has an effect if kinetic scrolling is enabled.
 *
 * Since: 3.4
 */
void
ctk_scrolled_window_set_capture_button_press (CtkScrolledWindow *scrolled_window,
                                              gboolean           capture_button_press)
{
  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));

  scrolled_window->priv->capture_button_press = capture_button_press;
}

/**
 * ctk_scrolled_window_get_capture_button_press:
 * @scrolled_window: a #CtkScrolledWindow
 *
 * Return whether button presses are captured during kinetic
 * scrolling. See ctk_scrolled_window_set_capture_button_press().
 *
 * Returns: %TRUE if button presses are captured during kinetic scrolling
 *
 * Since: 3.4
 */
gboolean
ctk_scrolled_window_get_capture_button_press (CtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), FALSE);

  return scrolled_window->priv->capture_button_press;
}

static void
ctk_scrolled_window_destroy (CtkWidget *widget)
{
  CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (widget);
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CtkWidget *child;

  child = ctk_bin_get_child (CTK_BIN (widget));
  if (child)
    ctk_widget_destroy (child);

  remove_indicator (scrolled_window, &priv->hindicator);
  remove_indicator (scrolled_window, &priv->vindicator);
  uninstall_scroll_cursor (scrolled_window);

  if (priv->hscrollbar)
    {
      CtkAdjustment *hadjustment = ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar));

      g_signal_handlers_disconnect_by_data (hadjustment, scrolled_window);
      g_signal_handlers_disconnect_by_data (hadjustment, &priv->hindicator);

      ctk_widget_unparent (priv->hscrollbar);
      priv->hscrollbar = NULL;
    }

  if (priv->vscrollbar)
    {
      CtkAdjustment *vadjustment = ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar));

      g_signal_handlers_disconnect_by_data (vadjustment, scrolled_window);
      g_signal_handlers_disconnect_by_data (vadjustment, &priv->vindicator);

      ctk_widget_unparent (priv->vscrollbar);
      priv->vscrollbar = NULL;
    }

  if (priv->deceleration_id)
    {
      ctk_widget_remove_tick_callback (widget, priv->deceleration_id);
      priv->deceleration_id = 0;
    }

  g_clear_pointer (&priv->hscrolling, ctk_kinetic_scrolling_free);
  g_clear_pointer (&priv->vscrolling, ctk_kinetic_scrolling_free);

  if (priv->scroll_events_overshoot_id)
    {
      g_source_remove (priv->scroll_events_overshoot_id);
      priv->scroll_events_overshoot_id = 0;
    }

  CTK_WIDGET_CLASS (ctk_scrolled_window_parent_class)->destroy (widget);
}

static void
ctk_scrolled_window_finalize (GObject *object)
{
  CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (object);
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  g_clear_object (&priv->drag_gesture);
  g_clear_object (&priv->swipe_gesture);
  g_clear_object (&priv->long_press_gesture);
  g_clear_object (&priv->pan_gesture);
  g_clear_object (&priv->gadget);
  g_clear_pointer (&priv->scroll_history, g_array_unref);

  G_OBJECT_CLASS (ctk_scrolled_window_parent_class)->finalize (object);
}

static void
ctk_scrolled_window_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
  CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (object);
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      ctk_scrolled_window_set_hadjustment (scrolled_window,
					   g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      ctk_scrolled_window_set_vadjustment (scrolled_window,
					   g_value_get_object (value));
      break;
    case PROP_HSCROLLBAR_POLICY:
      ctk_scrolled_window_set_policy (scrolled_window,
				      g_value_get_enum (value),
				      priv->vscrollbar_policy);
      break;
    case PROP_VSCROLLBAR_POLICY:
      ctk_scrolled_window_set_policy (scrolled_window,
				      priv->hscrollbar_policy,
				      g_value_get_enum (value));
      break;
    case PROP_WINDOW_PLACEMENT:
      ctk_scrolled_window_set_placement_internal (scrolled_window,
		      				  g_value_get_enum (value));
      break;
    case PROP_WINDOW_PLACEMENT_SET:
      /* noop */
      break;
    case PROP_SHADOW_TYPE:
      ctk_scrolled_window_set_shadow_type (scrolled_window,
					   g_value_get_enum (value));
      break;
    case PROP_MIN_CONTENT_WIDTH:
      ctk_scrolled_window_set_min_content_width (scrolled_window,
                                                 g_value_get_int (value));
      break;
    case PROP_MIN_CONTENT_HEIGHT:
      ctk_scrolled_window_set_min_content_height (scrolled_window,
                                                  g_value_get_int (value));
      break;
    case PROP_KINETIC_SCROLLING:
      ctk_scrolled_window_set_kinetic_scrolling (scrolled_window,
                                                 g_value_get_boolean (value));
      break;
    case PROP_OVERLAY_SCROLLING:
      ctk_scrolled_window_set_overlay_scrolling (scrolled_window,
                                                 g_value_get_boolean (value));
      break;
    case PROP_MAX_CONTENT_WIDTH:
      ctk_scrolled_window_set_max_content_width (scrolled_window,
                                                 g_value_get_int (value));
      break;
    case PROP_MAX_CONTENT_HEIGHT:
      ctk_scrolled_window_set_max_content_height (scrolled_window,
                                                  g_value_get_int (value));
      break;
    case PROP_PROPAGATE_NATURAL_WIDTH:
      ctk_scrolled_window_set_propagate_natural_width (scrolled_window,
						       g_value_get_boolean (value));
      break;
    case PROP_PROPAGATE_NATURAL_HEIGHT:
      ctk_scrolled_window_set_propagate_natural_height (scrolled_window,
						       g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_scrolled_window_get_property (GObject    *object,
				  guint       prop_id,
				  GValue     *value,
				  GParamSpec *pspec)
{
  CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (object);
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      g_value_set_object (value,
			  G_OBJECT (ctk_scrolled_window_get_hadjustment (scrolled_window)));
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value,
			  G_OBJECT (ctk_scrolled_window_get_vadjustment (scrolled_window)));
      break;
    case PROP_WINDOW_PLACEMENT:
      g_value_set_enum (value, priv->window_placement);
      break;
    case PROP_WINDOW_PLACEMENT_SET:
      g_value_set_boolean (value, TRUE);
      break;
    case PROP_SHADOW_TYPE:
      g_value_set_enum (value, priv->shadow_type);
      break;
    case PROP_HSCROLLBAR_POLICY:
      g_value_set_enum (value, priv->hscrollbar_policy);
      break;
    case PROP_VSCROLLBAR_POLICY:
      g_value_set_enum (value, priv->vscrollbar_policy);
      break;
    case PROP_MIN_CONTENT_WIDTH:
      g_value_set_int (value, priv->min_content_width);
      break;
    case PROP_MIN_CONTENT_HEIGHT:
      g_value_set_int (value, priv->min_content_height);
      break;
    case PROP_KINETIC_SCROLLING:
      g_value_set_boolean (value, priv->kinetic_scrolling);
      break;
    case PROP_OVERLAY_SCROLLING:
      g_value_set_boolean (value, priv->overlay_scrolling);
      break;
    case PROP_MAX_CONTENT_WIDTH:
      g_value_set_int (value, priv->max_content_width);
      break;
    case PROP_MAX_CONTENT_HEIGHT:
      g_value_set_int (value, priv->max_content_height);
      break;
    case PROP_PROPAGATE_NATURAL_WIDTH:
      g_value_set_boolean (value, priv->propagate_natural_width);
      break;
    case PROP_PROPAGATE_NATURAL_HEIGHT:
      g_value_set_boolean (value, priv->propagate_natural_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_scrolled_window_inner_allocation (CtkWidget     *widget,
                                      CtkAllocation *rect)
{
  CtkWidget *child;
  CtkBorder border = { 0 };

  ctk_scrolled_window_relative_allocation (widget, rect);
  child = ctk_bin_get_child (CTK_BIN (widget));
  if (child && ctk_scrollable_get_border (CTK_SCROLLABLE (child), &border))
    {
      rect->x += border.left;
      rect->y += border.top;
      rect->width -= border.left + border.right;
      rect->height -= border.top + border.bottom;
    }
}

static gboolean
ctk_scrolled_window_draw (CtkWidget *widget,
                          cairo_t   *cr)
{
  CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (widget);
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  ctk_css_gadget_draw (priv->gadget, cr);

  return FALSE;
}

static void
ctk_scrolled_window_forall (CtkContainer *container,
			    gboolean	  include_internals,
			    CtkCallback   callback,
			    gpointer      callback_data)
{
  CtkScrolledWindowPrivate *priv;
  CtkScrolledWindow *scrolled_window;

  CTK_CONTAINER_CLASS (ctk_scrolled_window_parent_class)->forall (container,
                                                                  include_internals,
                                                                  callback,
                                                                  callback_data);
  if (include_internals)
    {
      scrolled_window = CTK_SCROLLED_WINDOW (container);
      priv = scrolled_window->priv;

      if (priv->vscrollbar)
        callback (priv->vscrollbar, callback_data);
      if (priv->hscrollbar)
        callback (priv->hscrollbar, callback_data);
    }
}

static gboolean
ctk_scrolled_window_scroll_child (CtkScrolledWindow *scrolled_window,
				  CtkScrollType      scroll,
				  gboolean           horizontal)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CtkAdjustment *adjustment = NULL;
  
  switch (scroll)
    {
    case CTK_SCROLL_STEP_UP:
      scroll = CTK_SCROLL_STEP_BACKWARD;
      horizontal = FALSE;
      break;
    case CTK_SCROLL_STEP_DOWN:
      scroll = CTK_SCROLL_STEP_FORWARD;
      horizontal = FALSE;
      break;
    case CTK_SCROLL_STEP_LEFT:
      scroll = CTK_SCROLL_STEP_BACKWARD;
      horizontal = TRUE;
      break;
    case CTK_SCROLL_STEP_RIGHT:
      scroll = CTK_SCROLL_STEP_FORWARD;
      horizontal = TRUE;
      break;
    case CTK_SCROLL_PAGE_UP:
      scroll = CTK_SCROLL_PAGE_BACKWARD;
      horizontal = FALSE;
      break;
    case CTK_SCROLL_PAGE_DOWN:
      scroll = CTK_SCROLL_PAGE_FORWARD;
      horizontal = FALSE;
      break;
    case CTK_SCROLL_PAGE_LEFT:
      scroll = CTK_SCROLL_STEP_BACKWARD;
      horizontal = TRUE;
      break;
    case CTK_SCROLL_PAGE_RIGHT:
      scroll = CTK_SCROLL_STEP_FORWARD;
      horizontal = TRUE;
      break;
    case CTK_SCROLL_STEP_BACKWARD:
    case CTK_SCROLL_STEP_FORWARD:
    case CTK_SCROLL_PAGE_BACKWARD:
    case CTK_SCROLL_PAGE_FORWARD:
    case CTK_SCROLL_START:
    case CTK_SCROLL_END:
      break;
    default:
      g_warning ("Invalid scroll type %u for CtkScrolledWindow::scroll-child", scroll);
      return FALSE;
    }

  if (horizontal)
    {
      if (may_hscroll (scrolled_window))
        adjustment = ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar));
      else
        return FALSE;
    }
  else
    {
      if (may_vscroll (scrolled_window))
        adjustment = ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar));
      else
        return FALSE;
    }

  if (adjustment)
    {
      gdouble value = ctk_adjustment_get_value (adjustment);
      
      switch (scroll)
	{
	case CTK_SCROLL_STEP_FORWARD:
	  value += ctk_adjustment_get_step_increment (adjustment);
	  break;
	case CTK_SCROLL_STEP_BACKWARD:
	  value -= ctk_adjustment_get_step_increment (adjustment);
	  break;
	case CTK_SCROLL_PAGE_FORWARD:
	  value += ctk_adjustment_get_page_increment (adjustment);
	  break;
	case CTK_SCROLL_PAGE_BACKWARD:
	  value -= ctk_adjustment_get_page_increment (adjustment);
	  break;
	case CTK_SCROLL_START:
	  value = ctk_adjustment_get_lower (adjustment);
	  break;
	case CTK_SCROLL_END:
	  value = ctk_adjustment_get_upper (adjustment);
	  break;
	default:
	  g_assert_not_reached ();
	  break;
	}

      ctk_adjustment_animate_to_value (adjustment, value);

      return TRUE;
    }

  return FALSE;
}

static void
ctk_scrolled_window_move_focus_out (CtkScrolledWindow *scrolled_window,
				    CtkDirectionType   direction_type)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CtkWidget *toplevel;
  
  /* Focus out of the scrolled window entirely. We do this by setting
   * a flag, then propagating the focus motion to the notebook.
   */
  toplevel = ctk_widget_get_toplevel (CTK_WIDGET (scrolled_window));
  if (!ctk_widget_is_toplevel (toplevel))
    return;

  g_object_ref (scrolled_window);

  priv->focus_out = TRUE;
  g_signal_emit_by_name (toplevel, "move-focus", direction_type);
  priv->focus_out = FALSE;

  g_object_unref (scrolled_window);
}

static void
ctk_scrolled_window_relative_allocation (CtkWidget     *widget,
					 CtkAllocation *allocation)
{
  CtkAllocation content_allocation;
  CtkScrolledWindow *scrolled_window;
  CtkScrolledWindowPrivate *priv;
  gint sb_spacing;
  gint sb_width;
  gint sb_height;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (allocation != NULL);

  scrolled_window = CTK_SCROLLED_WINDOW (widget);
  priv = scrolled_window->priv;

  /* Get possible scrollbar dimensions */
  sb_spacing = _ctk_scrolled_window_get_scrollbar_spacing (scrolled_window);
  ctk_widget_get_preferred_height (priv->hscrollbar, &sb_height, NULL);
  ctk_widget_get_preferred_width (priv->vscrollbar, &sb_width, NULL);

  ctk_css_gadget_get_content_allocation (priv->gadget, &content_allocation, NULL);

  allocation->x = content_allocation.x;
  allocation->y = content_allocation.y;
  allocation->width = content_allocation.width;
  allocation->height = content_allocation.height;

  /* Subtract some things from our available allocation size */
  if (priv->vscrollbar_visible && !priv->use_indicators)
    {
      gboolean is_rtl;

      is_rtl = ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL;

      if ((!is_rtl &&
	   (priv->window_placement == CTK_CORNER_TOP_RIGHT ||
	    priv->window_placement == CTK_CORNER_BOTTOM_RIGHT)) ||
	  (is_rtl &&
	   (priv->window_placement == CTK_CORNER_TOP_LEFT ||
	    priv->window_placement == CTK_CORNER_BOTTOM_LEFT)))
	allocation->x += (sb_width +  sb_spacing);

      allocation->width = MAX (1, allocation->width - (sb_width + sb_spacing));
    }

  if (priv->hscrollbar_visible && !priv->use_indicators)
    {

      if (priv->window_placement == CTK_CORNER_BOTTOM_LEFT ||
	  priv->window_placement == CTK_CORNER_BOTTOM_RIGHT)
	allocation->y += (sb_height + sb_spacing);

      allocation->height = MAX (1, allocation->height - (sb_height + sb_spacing));
    }
}

static gboolean
_ctk_scrolled_window_get_overshoot (CtkScrolledWindow *scrolled_window,
                                    gint              *overshoot_x,
                                    gint              *overshoot_y)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CtkAdjustment *vadjustment, *hadjustment;
  gdouble lower, upper, x, y;

  /* Vertical overshoot */
  vadjustment = ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar));
  lower = ctk_adjustment_get_lower (vadjustment);
  upper = ctk_adjustment_get_upper (vadjustment) -
    ctk_adjustment_get_page_size (vadjustment);

  if (priv->unclamped_vadj_value < lower)
    y = priv->unclamped_vadj_value - lower;
  else if (priv->unclamped_vadj_value > upper)
    y = priv->unclamped_vadj_value - upper;
  else
    y = 0;

  /* Horizontal overshoot */
  hadjustment = ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar));
  lower = ctk_adjustment_get_lower (hadjustment);
  upper = ctk_adjustment_get_upper (hadjustment) -
    ctk_adjustment_get_page_size (hadjustment);

  if (priv->unclamped_hadj_value < lower)
    x = priv->unclamped_hadj_value - lower;
  else if (priv->unclamped_hadj_value > upper)
    x = priv->unclamped_hadj_value - upper;
  else
    x = 0;

  if (overshoot_x)
    *overshoot_x = x;

  if (overshoot_y)
    *overshoot_y = y;

  return (x != 0 || y != 0);
}

static void
ctk_scrolled_window_allocate_child (CtkScrolledWindow *swindow,
				    CtkAllocation     *relative_allocation)
{
  CtkWidget     *widget = CTK_WIDGET (swindow), *child;
  CtkAllocation  child_allocation;

  child = ctk_bin_get_child (CTK_BIN (widget));

  ctk_scrolled_window_relative_allocation (widget, relative_allocation);

  child_allocation.x = relative_allocation->x;
  child_allocation.y = relative_allocation->y;
  child_allocation.width = relative_allocation->width;
  child_allocation.height = relative_allocation->height;

  ctk_widget_size_allocate (child, &child_allocation);
}

static void
ctk_scrolled_window_allocate_scrollbar (CtkScrolledWindow *scrolled_window,
                                        CtkWidget         *scrollbar,
                                        CtkAllocation     *allocation)
{
  CtkAllocation child_allocation, content_allocation;
  CtkWidget *widget = CTK_WIDGET (scrolled_window);
  gint sb_spacing, sb_height, sb_width;
  CtkScrolledWindowPrivate *priv;

  priv = scrolled_window->priv;

  ctk_scrolled_window_inner_allocation (widget, &content_allocation);
  sb_spacing = _ctk_scrolled_window_get_scrollbar_spacing (scrolled_window);
  ctk_widget_get_preferred_height (priv->hscrollbar, &sb_height, NULL);
  ctk_widget_get_preferred_width (priv->vscrollbar, &sb_width, NULL);

  if (scrollbar == priv->hscrollbar)
    {
      child_allocation.x = content_allocation.x;

      if (priv->window_placement == CTK_CORNER_TOP_LEFT ||
	  priv->window_placement == CTK_CORNER_TOP_RIGHT)
        {
          if (priv->use_indicators)
	    child_allocation.y = content_allocation.y + content_allocation.height - sb_height;
          else
	    child_allocation.y = content_allocation.y + content_allocation.height + sb_spacing;
        }
      else
        {
          if (priv->use_indicators)
	    child_allocation.y = content_allocation.y;
          else
	    child_allocation.y = content_allocation.y - sb_spacing - sb_height;
        }

      child_allocation.width = content_allocation.width;
      child_allocation.height = sb_height;
    }
  else if (scrollbar == priv->vscrollbar)
    {
      if ((ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL &&
	   (priv->window_placement == CTK_CORNER_TOP_RIGHT ||
	    priv->window_placement == CTK_CORNER_BOTTOM_RIGHT)) ||
	  (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_LTR &&
	   (priv->window_placement == CTK_CORNER_TOP_LEFT ||
	    priv->window_placement == CTK_CORNER_BOTTOM_LEFT)))
        {
          if (priv->use_indicators)
	    child_allocation.x = content_allocation.x + content_allocation.width - sb_width;
          else
	    child_allocation.x = content_allocation.x + content_allocation.width + sb_spacing;
        }
      else
        {
          if (priv->use_indicators)
	    child_allocation.x = content_allocation.x;
          else
	    child_allocation.x = content_allocation.x - sb_spacing - sb_width;
        }

      child_allocation.y = content_allocation.y;
      child_allocation.width = sb_width;
      child_allocation.height = content_allocation.height;
    }

  *allocation = child_allocation;
}

static void
ctk_scrolled_window_size_allocate (CtkWidget     *widget,
				   CtkAllocation *allocation)
{
  CtkScrolledWindow *scrolled_window;
  CtkScrolledWindowPrivate *priv;
  CtkAllocation clip, content_allocation;

  scrolled_window = CTK_SCROLLED_WINDOW (widget);
  priv = scrolled_window->priv;

  ctk_widget_set_allocation (widget, allocation);

  if (ctk_widget_get_realized (widget))
    cdk_window_move_resize (ctk_widget_get_window (widget),
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);

  content_allocation = *allocation;
  content_allocation.x = content_allocation.y = 0;
  ctk_css_gadget_allocate (priv->gadget,
                           &content_allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  clip.x += allocation->x;
  clip.y += allocation->y;
  ctk_widget_set_clip (widget, &clip);
}

static void
clear_scroll_window (CtkScrolledWindow *scrolled_window)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  priv->scroll_window = NULL;
  g_clear_object (&priv->scroll_cursor);
}

static void
finalize_scroll_window (gpointer data,
                        GObject *where_the_object_was G_GNUC_UNUSED)
{
  clear_scroll_window ((CtkScrolledWindow *) data);
}

static void
install_scroll_cursor (CtkScrolledWindow *scrolled_window,
                       CdkWindow         *window)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CdkDisplay *display;
  CdkCursor *cursor;

  if (priv->scroll_window)
    return;

  priv->scroll_window = window;
  g_object_weak_ref (G_OBJECT (priv->scroll_window), finalize_scroll_window, scrolled_window);

  priv->scroll_cursor = cdk_window_get_cursor (priv->scroll_window);
  if (priv->scroll_cursor)
    g_object_ref (priv->scroll_cursor);

  display = cdk_window_get_display (priv->scroll_window);
  cursor = cdk_cursor_new_from_name (display, "all-scroll");
  cdk_window_set_cursor (priv->scroll_window, cursor);
  g_clear_object (&cursor);
}

static void
uninstall_scroll_cursor (CtkScrolledWindow *scrolled_window)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  if (priv->scroll_window)
    {
      cdk_window_set_cursor (priv->scroll_window, priv->scroll_cursor);
      g_object_weak_unref (G_OBJECT (priv->scroll_window), finalize_scroll_window, scrolled_window);
      clear_scroll_window (scrolled_window);
    }
}

static gboolean
start_scroll_deceleration_cb (gpointer user_data)
{
  CtkScrolledWindow *scrolled_window = user_data;
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  priv->scroll_events_overshoot_id = 0;

  if (!priv->deceleration_id)
    {
      uninstall_scroll_cursor (scrolled_window);
      ctk_scrolled_window_start_deceleration (scrolled_window);
    }

  return FALSE;
}


static gboolean
ctk_scrolled_window_scroll_event (CtkWidget      *widget,
				  CdkEventScroll *event)
{
  CtkScrolledWindowPrivate *priv;
  CtkScrolledWindow *scrolled_window;
  gboolean handled = FALSE;
  gdouble delta_x;
  gdouble delta_y;
  CdkScrollDirection direction;
  gboolean shifted, start_deceleration = FALSE;
  CdkDevice *source_device;
  CdkInputSource input_source;

  shifted = (event->state & CDK_SHIFT_MASK) != 0;

  scrolled_window = CTK_SCROLLED_WINDOW (widget);
  priv = scrolled_window->priv;

  ctk_scrolled_window_invalidate_overshoot (scrolled_window);
  source_device = cdk_event_get_source_device ((CdkEvent *) event);
  input_source = cdk_device_get_source (source_device);

  if (cdk_event_get_scroll_deltas ((CdkEvent *) event, &delta_x, &delta_y))
    {
      if (priv->scroll_device != source_device)
        {
          priv->scroll_device = source_device;
          scroll_history_reset (scrolled_window);
        }

      scroll_history_push (scrolled_window, event, shifted);

      if (input_source == CDK_SOURCE_TRACKPOINT ||
          input_source == CDK_SOURCE_TOUCHPAD)
        install_scroll_cursor (scrolled_window, cdk_event_get_window ((CdkEvent *)event));

      if (shifted)
        {
          gdouble delta;

          delta = delta_x;
          delta_x = delta_y;
          delta_y = delta;
        }

      if (delta_x != 0.0 &&
          may_hscroll (scrolled_window))
        {
          CtkAdjustment *adj;
          gdouble new_value;
          gdouble scroll_unit;

          adj = ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar));
          scroll_unit = get_scroll_unit (scrolled_window, CTK_ORIENTATION_HORIZONTAL);

          new_value = priv->unclamped_hadj_value + delta_x * scroll_unit;
          _ctk_scrolled_window_set_adjustment_value (scrolled_window, adj,
                                                     new_value);
          handled = TRUE;
        }

      if (delta_y != 0.0 &&
          may_vscroll (scrolled_window))
        {
          CtkAdjustment *adj;
          gdouble new_value;
          gdouble scroll_unit;

          adj = ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar));
          scroll_unit = get_scroll_unit (scrolled_window, CTK_ORIENTATION_VERTICAL);

          new_value = priv->unclamped_vadj_value + delta_y * scroll_unit;
          _ctk_scrolled_window_set_adjustment_value (scrolled_window, adj,
                                                     new_value);
          handled = TRUE;
        }

      /* The libinput driver may generate a final event with dx=dy=0
       * after scrolling finished, start kinetic scrolling when this
       * happens.
       */
      if (cdk_event_is_scroll_stop_event ((CdkEvent *) event))
        {
          handled = TRUE;
          start_deceleration = TRUE;
        }
    }
  else if (cdk_event_get_scroll_direction ((CdkEvent *)event, &direction))
    {
      CtkWidget *range;
      gboolean may_scroll;

      if ((!shifted && (direction == CDK_SCROLL_UP || direction == CDK_SCROLL_DOWN)) ||
          (shifted && (direction == CDK_SCROLL_LEFT || direction == CDK_SCROLL_RIGHT)))
        {
          range = priv->vscrollbar;
          may_scroll = may_vscroll (scrolled_window);
        }
      else
        {
          range = priv->hscrollbar;
          may_scroll = may_hscroll (scrolled_window);
        }

      if (range && may_scroll)
        {
          CtkAdjustment *adj = ctk_range_get_adjustment (CTK_RANGE (range));
          gdouble new_value;
          gdouble delta;

          delta = _ctk_range_get_wheel_delta (CTK_RANGE (range), event);

          new_value = CLAMP (ctk_adjustment_get_value (adj) + delta,
                             ctk_adjustment_get_lower (adj),
                             ctk_adjustment_get_upper (adj) -
                             ctk_adjustment_get_page_size (adj));

          ctk_adjustment_set_value (adj, new_value);

          handled = TRUE;
        }
    }

  if (handled)
    {
      gdouble vel_x, vel_y;

      ctk_scrolled_window_invalidate_overshoot (scrolled_window);

      if (priv->scroll_events_overshoot_id)
        {
          g_source_remove (priv->scroll_events_overshoot_id);
          priv->scroll_events_overshoot_id = 0;
        }

      if (start_deceleration)
        uninstall_scroll_cursor (scrolled_window);

      if (start_deceleration &&
          scroll_history_finish (scrolled_window, &vel_x, &vel_y))
        ctk_scrolled_window_decelerate (scrolled_window, vel_x, vel_y);
      else if (_ctk_scrolled_window_get_overshoot (scrolled_window, NULL, NULL))
        {
          priv->scroll_events_overshoot_id =
            cdk_threads_add_timeout (50, start_scroll_deceleration_cb, scrolled_window);
          g_source_set_name_by_id (priv->scroll_events_overshoot_id,
                                   "[ctk+] start_scroll_deceleration_cb");
        }
    }

  return handled;
}

static void
_ctk_scrolled_window_set_adjustment_value (CtkScrolledWindow *scrolled_window,
                                           CtkAdjustment     *adjustment,
                                           gdouble            value)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  gdouble lower, upper, *prev_value;
  CtkPositionType edge_pos;
  gboolean vertical;

  lower = ctk_adjustment_get_lower (adjustment) - MAX_OVERSHOOT_DISTANCE;
  upper = ctk_adjustment_get_upper (adjustment) -
    ctk_adjustment_get_page_size (adjustment) + MAX_OVERSHOOT_DISTANCE;

  if (adjustment == ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar)))
    vertical = FALSE;
  else if (adjustment == ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar)))
    vertical = TRUE;
  else
    return;

  if (vertical)
    prev_value = &priv->unclamped_vadj_value;
  else
    prev_value = &priv->unclamped_hadj_value;

  value = CLAMP (value, lower, upper);

  if (*prev_value == value)
    return;

  *prev_value = value;
  ctk_adjustment_set_value (adjustment, value);

  if (value == lower)
    edge_pos = vertical ? CTK_POS_TOP : CTK_POS_LEFT;
  else if (value == upper)
    edge_pos = vertical ? CTK_POS_BOTTOM : CTK_POS_RIGHT;
  else
    return;

  /* Invert horizontal edge position on RTL */
  if (!vertical &&
      ctk_widget_get_direction (CTK_WIDGET (scrolled_window)) == CTK_TEXT_DIR_RTL)
    edge_pos = (edge_pos == CTK_POS_LEFT) ? CTK_POS_RIGHT : CTK_POS_LEFT;

  g_signal_emit (scrolled_window, signals[EDGE_OVERSHOT], 0, edge_pos);
}

static gboolean
scrolled_window_deceleration_cb (CtkWidget     *widget G_GNUC_UNUSED,
                                 CdkFrameClock *frame_clock,
                                 gpointer       user_data)
{
  CtkScrolledWindow *scrolled_window = user_data;
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CtkAdjustment *hadjustment, *vadjustment;
  gint64 current_time;
  gdouble position, elapsed;

  current_time = cdk_frame_clock_get_frame_time (frame_clock);
  elapsed = (current_time - priv->last_deceleration_time) / (double)G_TIME_SPAN_SECOND;
  priv->last_deceleration_time = current_time;

  hadjustment = ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar));
  vadjustment = ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar));

  ctk_scrolled_window_invalidate_overshoot (scrolled_window);

  if (priv->hscrolling &&
      ctk_kinetic_scrolling_tick (priv->hscrolling, elapsed, &position, NULL))
    {
      priv->unclamped_hadj_value = position;
      ctk_adjustment_set_value (hadjustment, position);
    }
  else if (priv->hscrolling)
    g_clear_pointer (&priv->hscrolling, ctk_kinetic_scrolling_free);

  if (priv->vscrolling &&
      ctk_kinetic_scrolling_tick (priv->vscrolling, elapsed, &position, NULL))
    {
      priv->unclamped_vadj_value = position;
      ctk_adjustment_set_value (vadjustment, position);
    }
  else if (priv->vscrolling)
    g_clear_pointer (&priv->vscrolling, ctk_kinetic_scrolling_free);

  if (!priv->hscrolling && !priv->vscrolling)
    {
      ctk_scrolled_window_cancel_deceleration (scrolled_window);
      return G_SOURCE_REMOVE;
    }

  ctk_scrolled_window_invalidate_overshoot (scrolled_window);

  return G_SOURCE_CONTINUE;
}

static void
ctk_scrolled_window_cancel_deceleration (CtkScrolledWindow *scrolled_window)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  if (priv->deceleration_id)
    {
      ctk_widget_remove_tick_callback (CTK_WIDGET (scrolled_window),
                                       priv->deceleration_id);
      priv->deceleration_id = 0;
    }
}

static void
kinetic_scroll_stop_notify (CtkScrolledWindow *scrolled_window)
{
  CtkScrolledWindowPrivate *priv = ctk_scrolled_window_get_instance_private (scrolled_window);
  priv->deceleration_id = 0;
}

static void
ctk_scrolled_window_accumulate_velocity (CtkKineticScrolling **scrolling, double elapsed, double *velocity)
{
    if (!*scrolling)
      return;

    double last_velocity;
    ctk_kinetic_scrolling_tick (*scrolling, elapsed, NULL, &last_velocity);
    if (((*velocity >= 0) == (last_velocity >= 0)) &&
        (fabs (*velocity) >= fabs (last_velocity) * VELOCITY_ACCUMULATION_FLOOR))
      {
        double min_velocity = last_velocity * VELOCITY_ACCUMULATION_FLOOR;
        double max_velocity = last_velocity * VELOCITY_ACCUMULATION_CEIL;
        double accumulation_multiplier = (*velocity - min_velocity) / (max_velocity - min_velocity);
        *velocity += last_velocity * fmin (accumulation_multiplier, VELOCITY_ACCUMULATION_MAX);
      }
    g_clear_pointer (scrolling, ctk_kinetic_scrolling_free);
}

static void
ctk_scrolled_window_start_deceleration (CtkScrolledWindow *scrolled_window)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CdkFrameClock *frame_clock;
  gint64 current_time;
  double elapsed;

  g_return_if_fail (priv->deceleration_id == 0);

  frame_clock = ctk_widget_get_frame_clock (CTK_WIDGET (scrolled_window));

  current_time = cdk_frame_clock_get_frame_time (frame_clock);
  elapsed = (current_time - priv->last_deceleration_time) / (double)G_TIME_SPAN_SECOND;
  priv->last_deceleration_time = current_time;

  if (may_hscroll (scrolled_window))
    {
      gdouble lower,upper;
      CtkAdjustment *hadjustment;

      ctk_scrolled_window_accumulate_velocity (&priv->hscrolling, elapsed, &priv->x_velocity);

      hadjustment = ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar));
      lower = ctk_adjustment_get_lower (hadjustment);
      upper = ctk_adjustment_get_upper (hadjustment);
      upper -= ctk_adjustment_get_page_size (hadjustment);
      priv->hscrolling =
        ctk_kinetic_scrolling_new (lower,
                                   upper,
                                   MAX_OVERSHOOT_DISTANCE,
                                   DECELERATION_FRICTION,
                                   OVERSHOOT_FRICTION,
                                   priv->unclamped_hadj_value,
                                   priv->x_velocity);
    }
  else
    g_clear_pointer (&priv->hscrolling, ctk_kinetic_scrolling_free);

  if (may_vscroll (scrolled_window))
    {
      gdouble lower,upper;
      CtkAdjustment *vadjustment;

      ctk_scrolled_window_accumulate_velocity (&priv->vscrolling, elapsed, &priv->y_velocity);

      vadjustment = ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar));
      lower = ctk_adjustment_get_lower(vadjustment);
      upper = ctk_adjustment_get_upper(vadjustment);
      upper -= ctk_adjustment_get_page_size(vadjustment);
      priv->vscrolling =
        ctk_kinetic_scrolling_new (lower,
                                   upper,
                                   MAX_OVERSHOOT_DISTANCE,
                                   DECELERATION_FRICTION,
                                   OVERSHOOT_FRICTION,
                                   priv->unclamped_vadj_value,
                                   priv->y_velocity);
    }
  else
    g_clear_pointer (&priv->vscrolling, ctk_kinetic_scrolling_free);

  scrolled_window->priv->deceleration_id =
    ctk_widget_add_tick_callback (CTK_WIDGET (scrolled_window),
                                  scrolled_window_deceleration_cb, scrolled_window,
                                  (GDestroyNotify) kinetic_scroll_stop_notify);
}

static gboolean
ctk_scrolled_window_focus (CtkWidget        *widget,
			   CtkDirectionType  direction)
{
  CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (widget);
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CtkWidget *child;
  gboolean had_focus_child;

  had_focus_child = ctk_container_get_focus_child (CTK_CONTAINER (widget)) != NULL;

  if (priv->focus_out)
    {
      priv->focus_out = FALSE; /* Clear this to catch the wrap-around case */
      return FALSE;
    }
  
  if (ctk_widget_is_focus (widget))
    return FALSE;

  /* We only put the scrolled window itself in the focus chain if it
   * isn't possible to focus any children.
   */
  child = ctk_bin_get_child (CTK_BIN (widget));
  if (child)
    {
      if (ctk_widget_child_focus (child, direction))
	return TRUE;
    }

  if (!had_focus_child && ctk_widget_get_can_focus (widget))
    {
      ctk_widget_grab_focus (widget);
      return TRUE;
    }
  else
    return FALSE;
}

static void
ctk_scrolled_window_adjustment_changed (CtkAdjustment *adjustment,
					gpointer       data)
{
  CtkScrolledWindowPrivate *priv;
  CtkScrolledWindow *scrolled_window;

  scrolled_window = CTK_SCROLLED_WINDOW (data);
  priv = scrolled_window->priv;

  if (adjustment == ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar)))
    {
      if (priv->hscrollbar_policy == CTK_POLICY_AUTOMATIC)
	{
	  gboolean visible;

	  visible = priv->hscrollbar_visible;
	  priv->hscrollbar_visible = (ctk_adjustment_get_upper (adjustment) - ctk_adjustment_get_lower (adjustment) >
				      ctk_adjustment_get_page_size (adjustment));

	  if (priv->hscrollbar_visible != visible)
	    ctk_widget_queue_resize (CTK_WIDGET (scrolled_window));
	}
    }
  else if (adjustment == ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar)))
    {
      if (priv->vscrollbar_policy == CTK_POLICY_AUTOMATIC)
	{
	  gboolean visible;

	  visible = priv->vscrollbar_visible;
	  priv->vscrollbar_visible = (ctk_adjustment_get_upper (adjustment) - ctk_adjustment_get_lower (adjustment) >
			              ctk_adjustment_get_page_size (adjustment));

	  if (priv->vscrollbar_visible != visible)
	    ctk_widget_queue_resize (CTK_WIDGET (scrolled_window));
	}
    }
}

static void
maybe_emit_edge_reached (CtkScrolledWindow *scrolled_window,
			 CtkAdjustment *adjustment)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  gdouble value, lower, upper, page_size;
  CtkPositionType edge_pos;
  gboolean vertical;

  if (adjustment == ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar)))
    vertical = FALSE;
  else if (adjustment == ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar)))
    vertical = TRUE;
  else
    return;

  value = ctk_adjustment_get_value (adjustment);
  lower = ctk_adjustment_get_lower (adjustment);
  upper = ctk_adjustment_get_upper (adjustment);
  page_size = ctk_adjustment_get_page_size (adjustment);

  if (value == lower)
    edge_pos = vertical ? CTK_POS_TOP: CTK_POS_LEFT;
  else if (value == upper - page_size)
    edge_pos = vertical ? CTK_POS_BOTTOM : CTK_POS_RIGHT;
  else
    return;

  if (!vertical &&
      ctk_widget_get_direction (CTK_WIDGET (scrolled_window)) == CTK_TEXT_DIR_RTL)
    edge_pos = (edge_pos == CTK_POS_LEFT) ? CTK_POS_RIGHT : CTK_POS_LEFT;

  g_signal_emit (scrolled_window, signals[EDGE_REACHED], 0, edge_pos);
}

static void
ctk_scrolled_window_adjustment_value_changed (CtkAdjustment *adjustment,
                                              gpointer       user_data)
{
  CtkScrolledWindow *scrolled_window = user_data;
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  maybe_emit_edge_reached (scrolled_window, adjustment);

  /* Allow overshooting for kinetic scrolling operations */
  if (priv->drag_device || priv->deceleration_id)
    return;

  /* Ensure CtkAdjustment and unclamped values are in sync */
  if (adjustment == ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar)))
    priv->unclamped_hadj_value = ctk_adjustment_get_value (adjustment);
  else if (adjustment == ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar)))
    priv->unclamped_vadj_value = ctk_adjustment_get_value (adjustment);
}

static void
ctk_scrolled_window_add (CtkContainer *container,
                         CtkWidget    *child)
{
  CtkScrolledWindowPrivate *priv;
  CtkScrolledWindow *scrolled_window;
  CtkBin *bin;
  CtkWidget *child_widget, *scrollable_child;
  CtkAdjustment *hadj, *vadj;

  bin = CTK_BIN (container);
  child_widget = ctk_bin_get_child (bin);
  g_return_if_fail (child_widget == NULL);

  scrolled_window = CTK_SCROLLED_WINDOW (container);
  priv = scrolled_window->priv;

  /* ctk_scrolled_window_set_[hv]adjustment have the side-effect
   * of creating the scrollbars
   */
  if (!priv->hscrollbar)
    ctk_scrolled_window_set_hadjustment (scrolled_window, NULL);

  if (!priv->vscrollbar)
    ctk_scrolled_window_set_vadjustment (scrolled_window, NULL);

  hadj = ctk_range_get_adjustment (CTK_RANGE (priv->hscrollbar));
  vadj = ctk_range_get_adjustment (CTK_RANGE (priv->vscrollbar));

  if (CTK_IS_SCROLLABLE (child))
    {
      scrollable_child = child;
    }
  else
    {
      scrollable_child = ctk_viewport_new (hadj, vadj);
      ctk_widget_show (scrollable_child);
      ctk_container_set_focus_hadjustment (CTK_CONTAINER (scrollable_child),
                                           ctk_scrolled_window_get_hadjustment (CTK_SCROLLED_WINDOW (scrolled_window)));
      ctk_container_set_focus_vadjustment (CTK_CONTAINER (scrollable_child),
                                           ctk_scrolled_window_get_vadjustment (CTK_SCROLLED_WINDOW (scrolled_window)));
      ctk_container_add (CTK_CONTAINER (scrollable_child), child);
      priv->auto_added_viewport = TRUE;
    }

  _ctk_bin_set_child (bin, scrollable_child);
  ctk_widget_set_parent (scrollable_child, CTK_WIDGET (bin));

  g_object_set (scrollable_child, "hadjustment", hadj, "vadjustment", vadj, NULL);
}

static void
ctk_scrolled_window_remove (CtkContainer *container,
			    CtkWidget    *child)
{
  CtkScrolledWindowPrivate *priv;
  CtkScrolledWindow *scrolled_window;
  CtkWidget *scrollable_child;

  scrolled_window = CTK_SCROLLED_WINDOW (container);
  priv = scrolled_window->priv;

  if (!priv->auto_added_viewport)
    {
      scrollable_child = child;
    }
  else
    {
      scrollable_child = ctk_bin_get_child (CTK_BIN (container));
      if (scrollable_child == child)
        {
          /* @child is the automatically added viewport. */
          CtkWidget *grandchild = ctk_bin_get_child (CTK_BIN (child));

          /* Remove the viewport's child, if any. */
          if (grandchild)
            ctk_container_remove (CTK_CONTAINER (child), grandchild);
        }
      else
        {
          /* @child is (assumed to be) the viewport's child. */
          ctk_container_remove (CTK_CONTAINER (scrollable_child), child);
        }
    }

  g_object_set (scrollable_child, "hadjustment", NULL, "vadjustment", NULL, NULL);

  CTK_CONTAINER_CLASS (ctk_scrolled_window_parent_class)->remove (container, scrollable_child);

  priv->auto_added_viewport = FALSE;
}

/**
 * ctk_scrolled_window_add_with_viewport:
 * @scrolled_window: a #CtkScrolledWindow
 * @child: the widget you want to scroll
 *
 * Used to add children without native scrolling capabilities. This
 * is simply a convenience function; it is equivalent to adding the
 * unscrollable child to a viewport, then adding the viewport to the
 * scrolled window. If a child has native scrolling, use
 * ctk_container_add() instead of this function.
 *
 * The viewport scrolls the child by moving its #CdkWindow, and takes
 * the size of the child to be the size of its toplevel #CdkWindow. 
 * This will be very wrong for most widgets that support native scrolling;
 * for example, if you add a widget such as #CtkTreeView with a viewport,
 * the whole widget will scroll, including the column headings. Thus, 
 * widgets with native scrolling support should not be used with the 
 * #CtkViewport proxy.
 *
 * A widget supports scrolling natively if it implements the
 * #CtkScrollable interface.
 */
void
ctk_scrolled_window_add_with_viewport (CtkScrolledWindow *scrolled_window,
				       CtkWidget         *child)
{
  CtkBin *bin;
  CtkWidget *viewport;
  CtkWidget *child_widget;

  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));
  g_return_if_fail (CTK_IS_WIDGET (child));
  g_return_if_fail (ctk_widget_get_parent (child) == NULL);

  bin = CTK_BIN (scrolled_window);
  child_widget = ctk_bin_get_child (bin);

  if (child_widget)
    {
      g_return_if_fail (CTK_IS_VIEWPORT (child_widget));
      g_return_if_fail (ctk_bin_get_child (CTK_BIN (child_widget)) == NULL);

      viewport = child_widget;
    }
  else
    {
      viewport =
        ctk_viewport_new (ctk_scrolled_window_get_hadjustment (scrolled_window),
                          ctk_scrolled_window_get_vadjustment (scrolled_window));
      ctk_container_set_focus_hadjustment (CTK_CONTAINER (viewport),
                                           ctk_scrolled_window_get_hadjustment (CTK_SCROLLED_WINDOW (scrolled_window)));
      ctk_container_set_focus_vadjustment (CTK_CONTAINER (viewport),
                                           ctk_scrolled_window_get_vadjustment (CTK_SCROLLED_WINDOW (scrolled_window)));
      ctk_container_add (CTK_CONTAINER (scrolled_window), viewport);
    }

  ctk_widget_show (viewport);
  ctk_container_add (CTK_CONTAINER (viewport), child);
}

static void
ctk_scrolled_window_get_preferred_width (CtkWidget *widget,
                                         gint      *minimum_size,
                                         gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_SCROLLED_WINDOW (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_scrolled_window_get_preferred_height (CtkWidget *widget,
                                          gint      *minimum_size,
                                          gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_SCROLLED_WINDOW (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_scrolled_window_get_preferred_height_for_width (CtkWidget *widget,
                                                    gint       width G_GNUC_UNUSED,
                                                    gint      *minimum_height,
                                                    gint      *natural_height)
{
  CTK_WIDGET_GET_CLASS (widget)->get_preferred_height (widget, minimum_height, natural_height);
}

static void
ctk_scrolled_window_get_preferred_width_for_height (CtkWidget *widget,
                                                    gint       height G_GNUC_UNUSED,
                                                    gint      *minimum_width,
                                                    gint      *natural_width)
{
  CTK_WIDGET_GET_CLASS (widget)->get_preferred_width (widget, minimum_width, natural_width);
}

static gboolean
ctk_widget_should_animate (CtkWidget *widget)
{
  if (!ctk_widget_get_mapped (widget))
    return FALSE;

  return ctk_settings_get_enable_animations (ctk_widget_get_settings (widget));
}

static void
ctk_scrolled_window_update_animating (CtkScrolledWindow *sw)
{
  CtkAdjustment *adjustment;
  CdkFrameClock *clock = NULL;
  guint duration = 0;

  if (ctk_widget_should_animate (CTK_WIDGET (sw)))
    {
      clock = ctk_widget_get_frame_clock (CTK_WIDGET (sw)),
      duration = ANIMATION_DURATION;
    }

  adjustment = ctk_range_get_adjustment (CTK_RANGE (sw->priv->hscrollbar));
  ctk_adjustment_enable_animation (adjustment, clock, duration);

  adjustment = ctk_range_get_adjustment (CTK_RANGE (sw->priv->vscrollbar));
  ctk_adjustment_enable_animation (adjustment, clock, duration);
}

static void
ctk_scrolled_window_map (CtkWidget *widget)
{
  CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (widget);

  CTK_WIDGET_CLASS (ctk_scrolled_window_parent_class)->map (widget);

  ctk_scrolled_window_update_animating (scrolled_window);
  ctk_scrolled_window_update_use_indicators (scrolled_window);
}

static void
ctk_scrolled_window_unmap (CtkWidget *widget)
{
  CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (widget);

  CTK_WIDGET_CLASS (ctk_scrolled_window_parent_class)->unmap (widget);

  ctk_scrolled_window_update_animating (scrolled_window);

  indicator_stop_fade (&scrolled_window->priv->hindicator);
  indicator_stop_fade (&scrolled_window->priv->vindicator);
}

static CdkWindow *
create_indicator_window (CtkScrolledWindow *scrolled_window,
                         CtkWidget         *child)
{
  CtkWidget *widget = CTK_WIDGET (scrolled_window);
  CdkRGBA transparent = { 0, 0, 0, 0 };
  CtkAllocation allocation;
  CdkWindow *window;
  CdkWindowAttr attributes;
  gint attributes_mask;

  ctk_scrolled_window_allocate_scrollbar (scrolled_window, child, &allocation);

  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.wclass = CDK_INPUT_OUTPUT;

  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;
  attributes.event_mask = ctk_widget_get_events (widget);

  window = cdk_window_new (ctk_widget_get_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_register_window (widget, window);

  cdk_window_set_background_rgba (window, &transparent);

  if (scrolled_window->priv->use_indicators)
    ctk_widget_set_parent_window (child, window);

  return window;
}

static void
indicator_set_fade (Indicator *indicator,
                    gdouble    pos)
{
  gboolean visible, changed;

  changed = indicator->current_pos != pos;
  indicator->current_pos = pos;

  visible = indicator->current_pos != 0.0 || indicator->target_pos != 0.0;

  if (visible && !cdk_window_is_visible (indicator->window))
    {
      cdk_window_show (indicator->window);
      indicator->conceil_timer = g_timeout_add (INDICATOR_FADE_OUT_TIME, maybe_hide_indicator, indicator);
    }
  if (!visible && cdk_window_is_visible (indicator->window) &&
      indicator->conceil_timer != 0)
    {
      cdk_window_hide (indicator->window);
      g_source_remove (indicator->conceil_timer);
      indicator->conceil_timer = 0;
    }

  if (changed)
    {
      ctk_widget_set_opacity (indicator->scrollbar, indicator->current_pos);
      ctk_widget_queue_draw (indicator->scrollbar);
    }
}

static gboolean
indicator_fade_cb (CtkWidget     *widget G_GNUC_UNUSED,
                   CdkFrameClock *frame_clock,
                   gpointer       user_data)
{
  Indicator *indicator = user_data;
  gdouble t;

  ctk_progress_tracker_advance_frame (&indicator->tracker,
                                      cdk_frame_clock_get_frame_time (frame_clock));
  t = ctk_progress_tracker_get_ease_out_cubic (&indicator->tracker, FALSE);

  indicator_set_fade (indicator,
                      indicator->source_pos + (t * (indicator->target_pos - indicator->source_pos)));

  if (ctk_progress_tracker_get_state (&indicator->tracker) == CTK_PROGRESS_STATE_AFTER)
    {
      indicator->tick_id = 0;
      return FALSE;
    }

  return TRUE;
}

static void
indicator_start_fade (Indicator *indicator,
                      gdouble    target)
{
  if (indicator->target_pos == target)
    return;

  indicator->target_pos = target;

  if (target != 0.0)
    indicator->last_scroll_time = g_get_monotonic_time ();

  if (ctk_widget_should_animate (indicator->scrollbar))
    {
      indicator->source_pos = indicator->current_pos;
      ctk_progress_tracker_start (&indicator->tracker, INDICATOR_FADE_OUT_DURATION * 1000, 0, 1.0);
      if (indicator->tick_id == 0)
        indicator->tick_id = ctk_widget_add_tick_callback (indicator->scrollbar, indicator_fade_cb, indicator, NULL);
    }
  else
    indicator_set_fade (indicator, target);
}

static void
indicator_stop_fade (Indicator *indicator)
{
  if (indicator->tick_id != 0)
    {
      indicator_set_fade (indicator, indicator->target_pos);
      ctk_widget_remove_tick_callback (indicator->scrollbar, indicator->tick_id);
      indicator->tick_id = 0;
    }

  if (indicator->conceil_timer)
    {
      g_source_remove (indicator->conceil_timer);
      indicator->conceil_timer = 0;
    }

  cdk_window_hide (indicator->window);
  ctk_progress_tracker_finish (&indicator->tracker);
  indicator->current_pos = indicator->source_pos = indicator->target_pos = 0;
  indicator->last_scroll_time = 0;
}

static gboolean
maybe_hide_indicator (gpointer data)
{
  Indicator *indicator = data;

  if (g_get_monotonic_time () - indicator->last_scroll_time >= INDICATOR_FADE_OUT_DELAY * 1000 &&
      !indicator->over)
    indicator_start_fade (indicator, 0.0);

  return G_SOURCE_CONTINUE;
}

static void
indicator_value_changed (CtkAdjustment *adjustment G_GNUC_UNUSED,
                         Indicator     *indicator)
{
  indicator->last_scroll_time = g_get_monotonic_time ();
  indicator_start_fade (indicator, 1.0);
}

static void
setup_indicator (CtkScrolledWindow *scrolled_window,
                 Indicator         *indicator,
                 CtkWidget         *scrollbar)
{
  CtkStyleContext *context;
  CtkAdjustment *adjustment;

  if (scrollbar == NULL)
    return;

  context = ctk_widget_get_style_context (scrollbar);
  adjustment = ctk_range_get_adjustment (CTK_RANGE (scrollbar));

  indicator->scrollbar = scrollbar;

  g_object_ref (scrollbar);
  ctk_widget_unparent (scrollbar);
  ctk_widget_set_parent_window (scrollbar, indicator->window);
  ctk_widget_set_parent (scrollbar, CTK_WIDGET (scrolled_window));
  g_object_unref (scrollbar);

  ctk_style_context_add_class (context, "overlay-indicator");
  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (indicator_value_changed), indicator);

  cdk_window_hide (indicator->window);
  ctk_widget_set_opacity (scrollbar, 0.0);
  indicator->current_pos = 0.0;
}

static void
remove_indicator (CtkScrolledWindow *scrolled_window,
                  Indicator         *indicator)
{
  CtkWidget *scrollbar;
  CtkStyleContext *context;
  CtkAdjustment *adjustment;

  if (indicator->scrollbar == NULL)
    return;

  scrollbar = indicator->scrollbar;
  indicator->scrollbar = NULL;

  context = ctk_widget_get_style_context (scrollbar);
  ctk_style_context_remove_class (context, "overlay-indicator");

  adjustment = ctk_range_get_adjustment (CTK_RANGE (scrollbar));
  g_signal_handlers_disconnect_by_data (adjustment, indicator);

  if (indicator->conceil_timer)
    {
      g_source_remove (indicator->conceil_timer);
      indicator->conceil_timer = 0;
    }

  if (indicator->over_timeout_id)
    {
      g_source_remove (indicator->over_timeout_id);
      indicator->over_timeout_id = 0;
    }

  if (indicator->tick_id)
    {
      ctk_widget_remove_tick_callback (scrollbar, indicator->tick_id);
      indicator->tick_id = 0;
    }

  g_object_ref (scrollbar);
  ctk_widget_unparent (scrollbar);
  ctk_widget_set_parent (scrollbar, CTK_WIDGET (scrolled_window));
  g_object_unref (scrollbar);

  if (indicator->window)
    cdk_window_hide (indicator->window);

  ctk_widget_set_opacity (scrollbar, 1.0);
  indicator->current_pos = 1.0;
}

static void
ctk_scrolled_window_sync_use_indicators (CtkScrolledWindow *scrolled_window)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  if (priv->use_indicators)
    {
      setup_indicator (scrolled_window, &priv->hindicator, priv->hscrollbar);
      setup_indicator (scrolled_window, &priv->vindicator, priv->vscrollbar);
    }
  else
    {
      remove_indicator (scrolled_window, &priv->hindicator);
      remove_indicator (scrolled_window, &priv->vindicator);
    }
}

static void
ctk_scrolled_window_update_use_indicators (CtkScrolledWindow *scrolled_window)
{
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  gboolean use_indicators;
  CtkSettings *settings = ctk_widget_get_settings (CTK_WIDGET (scrolled_window));
  gboolean overlay_scrolling;

  g_object_get (settings, "ctk-overlay-scrolling", &overlay_scrolling, NULL);

  use_indicators = overlay_scrolling && priv->overlay_scrolling;

  if (g_strcmp0 (g_getenv ("CTK_OVERLAY_SCROLLING"), "0") == 0)
    use_indicators = FALSE;

  if (priv->use_indicators != use_indicators)
    {
      priv->use_indicators = use_indicators;

      if (ctk_widget_get_realized (CTK_WIDGET (scrolled_window)))
        ctk_scrolled_window_sync_use_indicators (scrolled_window);

      ctk_widget_queue_resize (CTK_WIDGET (scrolled_window));
    }
}

static void
ctk_scrolled_window_realize (CtkWidget *widget)
{
  CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (widget);
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;
  CdkWindow *window;
  CtkAllocation allocation;
  CdkWindowAttr attributes;
  gint attributes_mask;

  ctk_widget_get_allocation (widget, &allocation);

  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.wclass = CDK_INPUT_OUTPUT;

  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;
  attributes.event_mask = ctk_widget_get_events (widget) |
    CDK_ENTER_NOTIFY_MASK | CDK_LEAVE_NOTIFY_MASK | CDK_POINTER_MOTION_MASK;

  window = cdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);

  ctk_widget_set_window (widget, window);
  ctk_widget_register_window (widget, window);
  ctk_widget_set_realized (widget, TRUE);

  priv->hindicator.window = create_indicator_window (scrolled_window, priv->hscrollbar);
  priv->vindicator.window = create_indicator_window (scrolled_window, priv->vscrollbar);

  priv->hindicator.scrollbar = priv->hscrollbar;
  priv->vindicator.scrollbar = priv->vscrollbar;

  ctk_scrolled_window_sync_use_indicators (scrolled_window);
}

static void
indicator_reset (Indicator *indicator)
{
  if (indicator->conceil_timer)
    {
      g_source_remove (indicator->conceil_timer);
      indicator->conceil_timer = 0;
    }

  if (indicator->over_timeout_id)
    {
      g_source_remove (indicator->over_timeout_id);
      indicator->over_timeout_id = 0;
    }

  if (indicator->scrollbar && indicator->tick_id)
    {
      ctk_widget_remove_tick_callback (indicator->scrollbar,
                                       indicator->tick_id);
      indicator->tick_id = 0;
    }

  if (indicator->window)
    {
      cdk_window_destroy (indicator->window);
      indicator->window = NULL;
    }

  indicator->scrollbar = NULL;
  indicator->over = FALSE;
  ctk_progress_tracker_finish (&indicator->tracker);
  indicator->current_pos = indicator->source_pos = indicator->target_pos = 0;
  indicator->last_scroll_time = 0;
}

static void
ctk_scrolled_window_unrealize (CtkWidget *widget)
{
  CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (widget);
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  ctk_widget_set_parent_window (priv->hscrollbar, NULL);
  ctk_widget_unregister_window (widget, priv->hindicator.window);
  indicator_reset (&priv->hindicator);

  ctk_widget_set_parent_window (priv->vscrollbar, NULL);
  ctk_widget_unregister_window (widget, priv->vindicator.window);
  indicator_reset (&priv->vindicator);

  CTK_WIDGET_CLASS (ctk_scrolled_window_parent_class)->unrealize (widget);
}

static void
ctk_scrolled_window_grab_notify (CtkWidget *widget,
                                 gboolean   was_grabbed G_GNUC_UNUSED)
{
  CtkScrolledWindow *scrolled_window = CTK_SCROLLED_WINDOW (widget);
  CtkScrolledWindowPrivate *priv = scrolled_window->priv;

  if (priv->drag_device &&
      ctk_widget_device_is_shadowed (widget,
                                     priv->drag_device))
    {
      if (_ctk_scrolled_window_get_overshoot (scrolled_window, NULL, NULL))
        ctk_scrolled_window_start_deceleration (scrolled_window);
      else
        ctk_scrolled_window_cancel_deceleration (scrolled_window);
    }
}

/**
 * ctk_scrolled_window_get_min_content_width:
 * @scrolled_window: a #CtkScrolledWindow
 *
 * Gets the minimum content width of @scrolled_window, or -1 if not set.
 *
 * Returns: the minimum content width
 *
 * Since: 3.0
 */
gint
ctk_scrolled_window_get_min_content_width (CtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), 0);

  return scrolled_window->priv->min_content_width;
}

/**
 * ctk_scrolled_window_set_min_content_width:
 * @scrolled_window: a #CtkScrolledWindow
 * @width: the minimal content width
 *
 * Sets the minimum width that @scrolled_window should keep visible.
 * Note that this can and (usually will) be smaller than the minimum
 * size of the content.
 *
 * It is a programming error to set the minimum content width to a
 * value greater than #CtkScrolledWindow:max-content-width.
 *
 * Since: 3.0
 */
void
ctk_scrolled_window_set_min_content_width (CtkScrolledWindow *scrolled_window,
                                           gint               width)
{
  CtkScrolledWindowPrivate *priv;

  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));

  priv = scrolled_window->priv;

  g_return_if_fail (width == -1 || priv->max_content_width == -1 || width <= priv->max_content_width);

  if (priv->min_content_width != width)
    {
      priv->min_content_width = width;

      ctk_widget_queue_resize (CTK_WIDGET (scrolled_window));

      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_MIN_CONTENT_WIDTH]);
    }
}

/**
 * ctk_scrolled_window_get_min_content_height:
 * @scrolled_window: a #CtkScrolledWindow
 *
 * Gets the minimal content height of @scrolled_window, or -1 if not set.
 *
 * Returns: the minimal content height
 *
 * Since: 3.0
 */
gint
ctk_scrolled_window_get_min_content_height (CtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), 0);

  return scrolled_window->priv->min_content_height;
}

/**
 * ctk_scrolled_window_set_min_content_height:
 * @scrolled_window: a #CtkScrolledWindow
 * @height: the minimal content height
 *
 * Sets the minimum height that @scrolled_window should keep visible.
 * Note that this can and (usually will) be smaller than the minimum
 * size of the content.
 *
 * It is a programming error to set the minimum content height to a
 * value greater than #CtkScrolledWindow:max-content-height.
 *
 * Since: 3.0
 */
void
ctk_scrolled_window_set_min_content_height (CtkScrolledWindow *scrolled_window,
                                            gint               height)
{
  CtkScrolledWindowPrivate *priv;

  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));

  priv = scrolled_window->priv;

  g_return_if_fail (height == -1 || priv->max_content_height == -1 || height <= priv->max_content_height);

  if (priv->min_content_height != height)
    {
      priv->min_content_height = height;

      ctk_widget_queue_resize (CTK_WIDGET (scrolled_window));

      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_MIN_CONTENT_HEIGHT]);
    }
}

/**
 * ctk_scrolled_window_set_overlay_scrolling:
 * @scrolled_window: a #CtkScrolledWindow
 * @overlay_scrolling: whether to enable overlay scrolling
 *
 * Enables or disables overlay scrolling for this scrolled window.
 *
 * Since: 3.16
 */
void
ctk_scrolled_window_set_overlay_scrolling (CtkScrolledWindow *scrolled_window,
                                           gboolean           overlay_scrolling)
{
  CtkScrolledWindowPrivate *priv;

  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));

  priv = scrolled_window->priv;

  if (priv->overlay_scrolling != overlay_scrolling)
    {
      priv->overlay_scrolling = overlay_scrolling;

      ctk_scrolled_window_update_use_indicators (scrolled_window);

      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties[PROP_OVERLAY_SCROLLING]);
    }
}

/**
 * ctk_scrolled_window_get_overlay_scrolling:
 * @scrolled_window: a #CtkScrolledWindow
 *
 * Returns whether overlay scrolling is enabled for this scrolled window.
 *
 * Returns: %TRUE if overlay scrolling is enabled
 *
 * Since: 3.16
 */
gboolean
ctk_scrolled_window_get_overlay_scrolling (CtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), TRUE);

  return scrolled_window->priv->overlay_scrolling;
}

/**
 * ctk_scrolled_window_set_max_content_width:
 * @scrolled_window: a #CtkScrolledWindow
 * @width: the maximum content width
 *
 * Sets the maximum width that @scrolled_window should keep visible. The
 * @scrolled_window will grow up to this width before it starts scrolling
 * the content.
 *
 * It is a programming error to set the maximum content width to a value
 * smaller than #CtkScrolledWindow:min-content-width.
 *
 * Since: 3.22
 */
void
ctk_scrolled_window_set_max_content_width (CtkScrolledWindow *scrolled_window,
                                           gint               width)
{
  CtkScrolledWindowPrivate *priv;

  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));

  priv = scrolled_window->priv;

  g_return_if_fail (width == -1 || priv->min_content_width == -1 || width >= priv->min_content_width);

  if (width != priv->max_content_width)
    {
      priv->max_content_width = width;
      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties [PROP_MAX_CONTENT_WIDTH]);
      ctk_widget_queue_resize (CTK_WIDGET (scrolled_window));
    }
}

/**
 * ctk_scrolled_window_get_max_content_width:
 * @scrolled_window: a #CtkScrolledWindow
 *
 * Returns the maximum content width set.
 *
 * Returns: the maximum content width, or -1
 *
 * Since: 3.22
 */
gint
ctk_scrolled_window_get_max_content_width (CtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), -1);

  return scrolled_window->priv->max_content_width;
}

/**
 * ctk_scrolled_window_set_max_content_height:
 * @scrolled_window: a #CtkScrolledWindow
 * @height: the maximum content height
 *
 * Sets the maximum height that @scrolled_window should keep visible. The
 * @scrolled_window will grow up to this height before it starts scrolling
 * the content.
 *
 * It is a programming error to set the maximum content height to a value
 * smaller than #CtkScrolledWindow:min-content-height.
 *
 * Since: 3.22
 */
void
ctk_scrolled_window_set_max_content_height (CtkScrolledWindow *scrolled_window,
                                            gint               height)
{
  CtkScrolledWindowPrivate *priv;

  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));

  priv = scrolled_window->priv;

  g_return_if_fail (height == -1 || priv->min_content_height == -1 || height >= priv->min_content_height);

  if (height != priv->max_content_height)
    {
      priv->max_content_height = height;
      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties [PROP_MAX_CONTENT_HEIGHT]);
      ctk_widget_queue_resize (CTK_WIDGET (scrolled_window));
    }
}

/**
 * ctk_scrolled_window_get_max_content_height:
 * @scrolled_window: a #CtkScrolledWindow
 *
 * Returns the maximum content height set.
 *
 * Returns: the maximum content height, or -1
 *
 * Since: 3.22
 */
gint
ctk_scrolled_window_get_max_content_height (CtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), -1);

  return scrolled_window->priv->max_content_height;
}

/**
 * ctk_scrolled_window_set_propagate_natural_width:
 * @scrolled_window: a #CtkScrolledWindow
 * @propagate: whether to propagate natural width
 *
 * Sets whether the natural width of the child should be calculated and propagated
 * through the scrolled window’s requested natural width.
 *
 * Since: 3.22
 */
void
ctk_scrolled_window_set_propagate_natural_width (CtkScrolledWindow *scrolled_window,
                                                 gboolean           propagate)
{
  CtkScrolledWindowPrivate *priv;

  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));

  priv = scrolled_window->priv;

  propagate = !!propagate;

  if (priv->propagate_natural_width != propagate)
    {
      priv->propagate_natural_width = propagate;
      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties [PROP_PROPAGATE_NATURAL_WIDTH]);
      ctk_widget_queue_resize (CTK_WIDGET (scrolled_window));
    }
}

/**
 * ctk_scrolled_window_get_propagate_natural_width:
 * @scrolled_window: a #CtkScrolledWindow
 *
 * Reports whether the natural width of the child will be calculated and propagated
 * through the scrolled window’s requested natural width.
 *
 * Returns: whether natural width propagation is enabled.
 *
 * Since: 3.22
 */
gboolean
ctk_scrolled_window_get_propagate_natural_width (CtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), -1);

  return scrolled_window->priv->propagate_natural_width;
}

/**
 * ctk_scrolled_window_set_propagate_natural_height:
 * @scrolled_window: a #CtkScrolledWindow
 * @propagate: whether to propagate natural height
 *
 * Sets whether the natural height of the child should be calculated and propagated
 * through the scrolled window’s requested natural height.
 *
 * Since: 3.22
 */
void
ctk_scrolled_window_set_propagate_natural_height (CtkScrolledWindow *scrolled_window,
                                                  gboolean           propagate)
{
  CtkScrolledWindowPrivate *priv;

  g_return_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window));

  priv = scrolled_window->priv;

  propagate = !!propagate;

  if (priv->propagate_natural_height != propagate)
    {
      priv->propagate_natural_height = propagate;
      g_object_notify_by_pspec (G_OBJECT (scrolled_window), properties [PROP_PROPAGATE_NATURAL_HEIGHT]);
      ctk_widget_queue_resize (CTK_WIDGET (scrolled_window));
    }
}

/**
 * ctk_scrolled_window_get_propagate_natural_height:
 * @scrolled_window: a #CtkScrolledWindow
 *
 * Reports whether the natural height of the child will be calculated and propagated
 * through the scrolled window’s requested natural height.
 *
 * Returns: whether natural height propagation is enabled.
 *
 * Since: 3.22
 */
gboolean
ctk_scrolled_window_get_propagate_natural_height (CtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (CTK_IS_SCROLLED_WINDOW (scrolled_window), -1);

  return scrolled_window->priv->propagate_natural_height;
}
