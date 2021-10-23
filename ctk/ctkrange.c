/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2001 Red Hat, Inc.
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
 * Modified by the CTK+ Team and others 1997-2004.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#include <stdio.h>
#include <math.h>

#include "ctkrange.h"
#include "ctkrangeprivate.h"

#include "ctkadjustmentprivate.h"
#include "ctkboxgadgetprivate.h"
#include "ctkbuiltiniconprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkcolorscaleprivate.h"
#include "ctkintl.h"
#include "ctkgesturelongpressprivate.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkorientableprivate.h"
#include "ctkprivate.h"
#include "ctkscale.h"
#include "ctkscrollbar.h"
#include "ctktypebuiltins.h"
#include "ctkwindow.h"
#include "ctkwidgetprivate.h"
#include "a11y/ctkrangeaccessible.h"
#include "ctkcssstylepropertyprivate.h"

/**
 * SECTION:ctkrange
 * @Short_description: Base class for widgets which visualize an adjustment
 * @Title: CtkRange
 *
 * #CtkRange is the common base class for widgets which visualize an
 * adjustment, e.g #CtkScale or #CtkScrollbar.
 *
 * Apart from signals for monitoring the parameters of the adjustment,
 * #CtkRange provides properties and methods for influencing the sensitivity
 * of the “steppers”. It also provides properties and methods for setting a
 * “fill level” on range widgets. See ctk_range_set_fill_level().
 */


#define TIMEOUT_INITIAL     500
#define TIMEOUT_REPEAT      250
#define AUTOSCROLL_FACTOR   20
#define SCROLL_EDGE_SIZE    15
#define MARK_SNAP_LENGTH    12

typedef struct _CtkRangeStepTimer CtkRangeStepTimer;

struct _CtkRangePrivate
{
  CtkCssGadget *mouse_location;
  /* last mouse coords we got, or G_MININT if mouse is outside the range */
  gint  mouse_x;
  gint  mouse_y;
  CtkCssGadget *grab_location;   /* "grabbed" mouse location, NULL for no grab */

  CtkRangeStepTimer *timer;

  CtkAdjustment     *adjustment;
  CtkSensitivityType lower_sensitivity;
  CtkSensitivityType upper_sensitivity;

  CdkWindow         *event_window;

  /* Steppers are: < > ---- < >
   *               a b      c d
   */
  CtkCssGadget *gadget;
  CtkCssGadget *contents_gadget;
  CtkCssGadget *stepper_a_gadget;
  CtkCssGadget *stepper_b_gadget;
  CtkCssGadget *stepper_c_gadget;
  CtkCssGadget *stepper_d_gadget;
  CtkCssGadget *trough_gadget;
  CtkCssGadget *fill_gadget;
  CtkCssGadget *highlight_gadget;
  CtkCssGadget *slider_gadget;

  CtkOrientation     orientation;

  gdouble  fill_level;
  gdouble *marks;

  gint *mark_pos;
  gint  min_slider_size;
  gint  n_marks;
  gint  round_digits;                /* Round off value to this many digits, -1 for no rounding */
  gint  slide_initial_slider_position;
  gint  slide_initial_coordinate_delta;

  guint flippable              : 1;
  guint inverted               : 1;
  guint slider_size_fixed      : 1;
  guint slider_use_min_size    : 1;
  guint trough_click_forward   : 1;  /* trough click was on the forward side of slider */

  /* Stepper sensitivity */
  guint lower_sensitive        : 1;
  guint upper_sensitive        : 1;

  /* The range has an origin, should be drawn differently. Used by CtkScale */
  guint has_origin             : 1;

  /* Whether we're doing fine adjustment */
  guint zoom                   : 1;

  /* Fill level */
  guint show_fill_level        : 1;
  guint restrict_to_fill_level : 1;

  /* Whether dragging is ongoing */
  guint in_drag                : 1;

  CtkGesture *long_press_gesture;
  CtkGesture *multipress_gesture;
  CtkGesture *drag_gesture;

  CtkScrollType autoscroll_mode;
  guint autoscroll_id;
};


enum {
  PROP_0,
  PROP_ADJUSTMENT,
  PROP_INVERTED,
  PROP_LOWER_STEPPER_SENSITIVITY,
  PROP_UPPER_STEPPER_SENSITIVITY,
  PROP_SHOW_FILL_LEVEL,
  PROP_RESTRICT_TO_FILL_LEVEL,
  PROP_FILL_LEVEL,
  PROP_ROUND_DIGITS,
  PROP_ORIENTATION,
  LAST_PROP = PROP_ORIENTATION
};

enum {
  VALUE_CHANGED,
  ADJUST_BOUNDS,
  MOVE_SLIDER,
  CHANGE_VALUE,
  LAST_SIGNAL
};

static void ctk_range_set_property   (GObject          *object,
                                      guint             prop_id,
                                      const GValue     *value,
                                      GParamSpec       *pspec);
static void ctk_range_get_property   (GObject          *object,
                                      guint             prop_id,
                                      GValue           *value,
                                      GParamSpec       *pspec);
static void ctk_range_finalize       (GObject          *object);
static void ctk_range_destroy        (CtkWidget        *widget);
static void ctk_range_get_preferred_width
                                     (CtkWidget        *widget,
                                      gint             *minimum,
                                      gint             *natural);
static void ctk_range_get_preferred_height
                                     (CtkWidget        *widget,
                                      gint             *minimum,
                                      gint             *natural);
static void ctk_range_size_allocate  (CtkWidget        *widget,
                                      CtkAllocation    *allocation);
static void ctk_range_realize        (CtkWidget        *widget);
static void ctk_range_unrealize      (CtkWidget        *widget);
static void ctk_range_map            (CtkWidget        *widget);
static void ctk_range_unmap          (CtkWidget        *widget);
static gboolean ctk_range_draw       (CtkWidget        *widget,
                                      cairo_t          *cr);

static void ctk_range_multipress_gesture_pressed  (CtkGestureMultiPress *gesture,
                                                   guint                 n_press,
                                                   gdouble               x,
                                                   gdouble               y,
                                                   CtkRange             *range);
static void ctk_range_multipress_gesture_released (CtkGestureMultiPress *gesture,
                                                   guint                 n_press,
                                                   gdouble               x,
                                                   gdouble               y,
                                                   CtkRange             *range);
static void ctk_range_drag_gesture_begin          (CtkGestureDrag       *gesture,
                                                   gdouble               offset_x,
                                                   gdouble               offset_y,
                                                   CtkRange             *range);
static void ctk_range_drag_gesture_update         (CtkGestureDrag       *gesture,
                                                   gdouble               offset_x,
                                                   gdouble               offset_y,
                                                   CtkRange             *range);
static void ctk_range_long_press_gesture_pressed  (CtkGestureLongPress  *gesture,
                                                   gdouble               x,
                                                   gdouble               y,
                                                   CtkRange             *range);


static gboolean ctk_range_scroll_event   (CtkWidget        *widget,
                                      CdkEventScroll   *event);
static gboolean ctk_range_event       (CtkWidget       *widget,
                                       CdkEvent        *event);
static void update_slider_position   (CtkRange	       *range,
				      gint              mouse_x,
				      gint              mouse_y);
static void stop_scrolling           (CtkRange         *range);
static void add_autoscroll           (CtkRange         *range);
static void remove_autoscroll        (CtkRange         *range);

/* Range methods */

static void ctk_range_move_slider              (CtkRange         *range,
                                                CtkScrollType     scroll);

/* Internals */
static void          ctk_range_compute_slider_position  (CtkRange      *range,
                                                         gdouble        adjustment_value,
                                                         CdkRectangle  *slider_rect);
static gboolean      ctk_range_scroll                   (CtkRange      *range,
                                                         CtkScrollType  scroll);
static void          ctk_range_update_mouse_location    (CtkRange      *range);
static void          ctk_range_calc_slider              (CtkRange      *range);
static void          ctk_range_calc_stepper_sensitivity (CtkRange      *range);
static void          ctk_range_calc_marks               (CtkRange      *range);
static void          ctk_range_adjustment_value_changed (CtkAdjustment *adjustment,
                                                         gpointer       data);
static void          ctk_range_adjustment_changed       (CtkAdjustment *adjustment,
                                                         gpointer       data);
static void          ctk_range_add_step_timer           (CtkRange      *range,
                                                         CtkScrollType  step);
static void          ctk_range_remove_step_timer        (CtkRange      *range);
static gboolean      ctk_range_real_change_value        (CtkRange      *range,
                                                         CtkScrollType  scroll,
                                                         gdouble        value);
static gboolean      ctk_range_key_press                (CtkWidget     *range,
							 CdkEventKey   *event);
static void          ctk_range_state_flags_changed      (CtkWidget     *widget,
                                                         CtkStateFlags  previous_state);
static void          ctk_range_direction_changed        (CtkWidget     *widget,
                                                         CtkTextDirection  previous_direction);
static void          ctk_range_measure_trough           (CtkCssGadget   *gadget,
                                                         CtkOrientation  orientation,
                                                         gint            for_size,
                                                         gint           *minimum,
                                                         gint           *natural,
                                                         gint           *minimum_baseline,
                                                         gint           *natural_baseline,
                                                         gpointer        user_data);
static void          ctk_range_allocate_trough          (CtkCssGadget        *gadget,
                                                         const CtkAllocation *allocation,
                                                         int                  baseline,
                                                         CtkAllocation       *out_clip,
                                                         gpointer             data);
static gboolean      ctk_range_render_trough            (CtkCssGadget *gadget,
                                                         cairo_t      *cr,
                                                         int           x,
                                                         int           y,
                                                         int           width,
                                                         int           height,
                                                         gpointer      user_data);
static void          ctk_range_measure                  (CtkCssGadget   *gadget,
                                                         CtkOrientation  orientation,
                                                         gint            for_size,
                                                         gint           *minimum,
                                                         gint           *natural,
                                                         gint           *minimum_baseline,
                                                         gint           *natural_baseline,
                                                         gpointer        user_data);
static void          ctk_range_allocate                 (CtkCssGadget        *gadget,
                                                         const CtkAllocation *allocation,
                                                         int                  baseline,
                                                         CtkAllocation       *out_clip,
                                                         gpointer             data);
static gboolean      ctk_range_render                   (CtkCssGadget *gadget,
                                                         cairo_t      *cr,
                                                         int           x,
                                                         int           y,
                                                         int           width,
                                                         int           height,
                                                         gpointer      user_data);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (CtkRange, ctk_range, CTK_TYPE_WIDGET,
                                  G_ADD_PRIVATE (CtkRange)
                                  G_IMPLEMENT_INTERFACE (CTK_TYPE_ORIENTABLE,
                                                         NULL))

static guint signals[LAST_SIGNAL];
static GParamSpec *properties[LAST_PROP];

static void
ctk_range_class_init (CtkRangeClass *class)
{
  GObjectClass   *gobject_class;
  CtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (class);
  widget_class = (CtkWidgetClass*) class;

  gobject_class->set_property = ctk_range_set_property;
  gobject_class->get_property = ctk_range_get_property;
  gobject_class->finalize = ctk_range_finalize;

  widget_class->destroy = ctk_range_destroy;
  widget_class->get_preferred_width = ctk_range_get_preferred_width;
  widget_class->get_preferred_height = ctk_range_get_preferred_height;
  widget_class->size_allocate = ctk_range_size_allocate;
  widget_class->realize = ctk_range_realize;
  widget_class->unrealize = ctk_range_unrealize;
  widget_class->map = ctk_range_map;
  widget_class->unmap = ctk_range_unmap;
  widget_class->draw = ctk_range_draw;
  widget_class->event = ctk_range_event;
  widget_class->scroll_event = ctk_range_scroll_event;
  widget_class->key_press_event = ctk_range_key_press;
  widget_class->state_flags_changed = ctk_range_state_flags_changed;
  widget_class->direction_changed = ctk_range_direction_changed;

  class->move_slider = ctk_range_move_slider;
  class->change_value = ctk_range_real_change_value;

  /**
   * CtkRange::value-changed:
   * @range: the #CtkRange that received the signal
   *
   * Emitted when the range value changes.
   */
  signals[VALUE_CHANGED] =
    g_signal_new (I_("value-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkRangeClass, value_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkRange::adjust-bounds:
   * @range: the #CtkRange that received the signal
   * @value: the value before we clamp
   *
   * Emitted before clamping a value, to give the application a
   * chance to adjust the bounds.
   */
  signals[ADJUST_BOUNDS] =
    g_signal_new (I_("adjust-bounds"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkRangeClass, adjust_bounds),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_DOUBLE);

  /**
   * CtkRange::move-slider:
   * @range: the #CtkRange that received the signal
   * @step: how to move the slider
   *
   * Virtual function that moves the slider. Used for keybindings.
   */
  signals[MOVE_SLIDER] =
    g_signal_new (I_("move-slider"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkRangeClass, move_slider),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CTK_TYPE_SCROLL_TYPE);

  /**
   * CtkRange::change-value:
   * @range: the #CtkRange that received the signal
   * @scroll: the type of scroll action that was performed
   * @value: the new value resulting from the scroll action
   *
   * The #CtkRange::change-value signal is emitted when a scroll action is
   * performed on a range.  It allows an application to determine the
   * type of scroll event that occurred and the resultant new value.
   * The application can handle the event itself and return %TRUE to
   * prevent further processing.  Or, by returning %FALSE, it can pass
   * the event to other handlers until the default CTK+ handler is
   * reached.
   *
   * The value parameter is unrounded.  An application that overrides
   * the CtkRange::change-value signal is responsible for clamping the
   * value to the desired number of decimal digits; the default CTK+
   * handler clamps the value based on #CtkRange:round-digits.
   *
   * Returns: %TRUE to prevent other handlers from being invoked for
   *     the signal, %FALSE to propagate the signal further
   *
   * Since: 2.6
   */
  signals[CHANGE_VALUE] =
    g_signal_new (I_("change-value"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkRangeClass, change_value),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__ENUM_DOUBLE,
                  G_TYPE_BOOLEAN, 2,
                  CTK_TYPE_SCROLL_TYPE,
                  G_TYPE_DOUBLE);

  g_object_class_override_property (gobject_class, PROP_ORIENTATION, "orientation");

  properties[PROP_ADJUSTMENT] =
      g_param_spec_object ("adjustment",
                           P_("Adjustment"),
                           P_("The CtkAdjustment that contains the current value of this range object"),
                           CTK_TYPE_ADJUSTMENT,
                           CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT);

  properties[PROP_INVERTED] =
      g_param_spec_boolean ("inverted",
                            P_("Inverted"),
                            P_("Invert direction slider moves to increase range value"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_LOWER_STEPPER_SENSITIVITY] =
      g_param_spec_enum ("lower-stepper-sensitivity",
                         P_("Lower stepper sensitivity"),
                         P_("The sensitivity policy for the stepper that points to the adjustment's lower side"),
                         CTK_TYPE_SENSITIVITY_TYPE,
                         CTK_SENSITIVITY_AUTO,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  properties[PROP_UPPER_STEPPER_SENSITIVITY] =
      g_param_spec_enum ("upper-stepper-sensitivity",
                         P_("Upper stepper sensitivity"),
                         P_("The sensitivity policy for the stepper that points to the adjustment's upper side"),
                         CTK_TYPE_SENSITIVITY_TYPE,
                         CTK_SENSITIVITY_AUTO,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  /**
   * CtkRange:show-fill-level:
   *
   * The show-fill-level property controls whether fill level indicator
   * graphics are displayed on the trough. See
   * ctk_range_set_show_fill_level().
   *
   * Since: 2.12
   **/
  properties[PROP_SHOW_FILL_LEVEL] =
      g_param_spec_boolean ("show-fill-level",
                            P_("Show Fill Level"),
                            P_("Whether to display a fill level indicator graphics on trough."),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkRange:restrict-to-fill-level:
   *
   * The restrict-to-fill-level property controls whether slider
   * movement is restricted to an upper boundary set by the
   * fill level. See ctk_range_set_restrict_to_fill_level().
   *
   * Since: 2.12
   **/
  properties[PROP_RESTRICT_TO_FILL_LEVEL] =
      g_param_spec_boolean ("restrict-to-fill-level",
                            P_("Restrict to Fill Level"),
                            P_("Whether to restrict the upper boundary to the fill level."),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkRange:fill-level:
   *
   * The fill level (e.g. prebuffering of a network stream).
   * See ctk_range_set_fill_level().
   *
   * Since: 2.12
   **/
  properties[PROP_FILL_LEVEL] =
      g_param_spec_double ("fill-level",
                           P_("Fill Level"),
                           P_("The fill level."),
                           -G_MAXDOUBLE, G_MAXDOUBLE,
                           G_MAXDOUBLE,
                           CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkRange:round-digits:
   *
   * The number of digits to round the value to when
   * it changes, or -1. See #CtkRange::change-value.
   *
   * Since: 2.24
   */
  properties[PROP_ROUND_DIGITS] =
      g_param_spec_int ("round-digits",
                        P_("Round Digits"),
                        P_("The number of digits to round the value to."),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, LAST_PROP, properties);

  /**
   * CtkRange:slider-width:
   *
   * Width of scrollbar or scale thumb.
   *
   * Deprecated: 3.20: Use the min-height/min-width CSS properties on the
   *   slider element. The value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("slider-width",
							     P_("Slider Width"),
							     P_("Width of scrollbar or scale thumb"),
							     0,
							     G_MAXINT,
							     14,
							     CTK_PARAM_READABLE|G_PARAM_DEPRECATED));
  /**
   * CtkRange:trough-border:
   *
   * Spacing between thumb/steppers and outer trough bevel.
   *
   * Deprecated: 3.20: Use the margin/padding CSS properties on the trough and
   *   stepper elements. The value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("trough-border",
                                                             P_("Trough Border"),
                                                             P_("Spacing between thumb/steppers and outer trough bevel"),
                                                             0,
                                                             G_MAXINT,
                                                             1,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));
  /**
   * CtkRange:stepper-size:
   *
   * Length of step buttons at ends.
   *
   * Deprecated: 3.20: Use the min-height/min-width CSS properties on the
   *   stepper elements. The value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("stepper-size",
							     P_("Stepper Size"),
							     P_("Length of step buttons at ends"),
							     0,
							     G_MAXINT,
							     14,
							     CTK_PARAM_READABLE|G_PARAM_DEPRECATED));
  /**
   * CtkRange:stepper-spacing:
   *
   * The spacing between the stepper buttons and thumb. Note that
   * stepper-spacing won't have any effect if there are no steppers.
   *
   * Deprecated: 3.20: Use the margin CSS property on the stepper elements.
   *   The value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("stepper-spacing",
							     P_("Stepper Spacing"),
							     P_("Spacing between step buttons and thumb"),
                                                             0,
							     G_MAXINT,
							     0,
							     CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkRange:arrow-displacement-x:
   *
   * How far in the x direction to move the arrow when the button is depressed.
   *
   * Deprecated: 3.20: The value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("arrow-displacement-x",
							     P_("Arrow X Displacement"),
							     P_("How far in the x direction to move the arrow when the button is depressed"),
							     G_MININT,
							     G_MAXINT,
							     0,
							     CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkRange:arrow-displacement-y:
   *
   * How far in the y direction to move the arrow when the button is depressed.
   *
   * Deprecated: 3.20: The value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("arrow-displacement-y",
							     P_("Arrow Y Displacement"),
							     P_("How far in the y direction to move the arrow when the button is depressed"),
							     G_MININT,
							     G_MAXINT,
							     0,
							     CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkRange:trough-under-steppers:
   *
   * Whether to draw the trough across the full length of the range or
   * to exclude the steppers and their spacing.
   *
   * Since: 2.10
   *
   * Deprecated: 3.20: The value of this style property is ignored, and the
   *   widget will behave as if it was set to %TRUE.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("trough-under-steppers",
                                                                 P_("Trough Under Steppers"),
                                                                 P_("Whether to draw trough for full length of range or exclude the steppers and spacing"),
                                                                 TRUE,
                                                                 CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkRange:arrow-scaling:
   *
   * The arrow size proportion relative to the scroll button size.
   *
   * Since: 2.14
   *
   * Deprecated: 3.20: Use min-width/min-height on the "button" node instead.
   *   The value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_float ("arrow-scaling",
							       P_("Arrow scaling"),
							       P_("Arrow scaling with regard to scroll button size"),
							       0.0, 1.0, 0.5,
							       CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_RANGE_ACCESSIBLE);
}

static void
ctk_range_sync_orientation (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;
  CtkOrientation orientation;

  orientation = ctk_orientable_get_orientation (CTK_ORIENTABLE (range));
  _ctk_orientable_set_style_classes (CTK_ORIENTABLE (range));
  ctk_box_gadget_set_orientation (CTK_BOX_GADGET (priv->contents_gadget), orientation);
}

static void
ctk_range_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  CtkRange *range = CTK_RANGE (object);
  CtkRangePrivate *priv = range->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      if (priv->orientation != g_value_get_enum (value))
        {
          priv->orientation = g_value_get_enum (value);
          ctk_range_sync_orientation (range);
          ctk_widget_queue_resize (CTK_WIDGET (range));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_ADJUSTMENT:
      ctk_range_set_adjustment (range, g_value_get_object (value));
      break;
    case PROP_INVERTED:
      ctk_range_set_inverted (range, g_value_get_boolean (value));
      break;
    case PROP_LOWER_STEPPER_SENSITIVITY:
      ctk_range_set_lower_stepper_sensitivity (range, g_value_get_enum (value));
      break;
    case PROP_UPPER_STEPPER_SENSITIVITY:
      ctk_range_set_upper_stepper_sensitivity (range, g_value_get_enum (value));
      break;
    case PROP_SHOW_FILL_LEVEL:
      ctk_range_set_show_fill_level (range, g_value_get_boolean (value));
      break;
    case PROP_RESTRICT_TO_FILL_LEVEL:
      ctk_range_set_restrict_to_fill_level (range, g_value_get_boolean (value));
      break;
    case PROP_FILL_LEVEL:
      ctk_range_set_fill_level (range, g_value_get_double (value));
      break;
    case PROP_ROUND_DIGITS:
      ctk_range_set_round_digits (range, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_range_get_property (GObject      *object,
			guint         prop_id,
			GValue       *value,
			GParamSpec   *pspec)
{
  CtkRange *range = CTK_RANGE (object);
  CtkRangePrivate *priv = range->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    case PROP_ADJUSTMENT:
      g_value_set_object (value, priv->adjustment);
      break;
    case PROP_INVERTED:
      g_value_set_boolean (value, priv->inverted);
      break;
    case PROP_LOWER_STEPPER_SENSITIVITY:
      g_value_set_enum (value, ctk_range_get_lower_stepper_sensitivity (range));
      break;
    case PROP_UPPER_STEPPER_SENSITIVITY:
      g_value_set_enum (value, ctk_range_get_upper_stepper_sensitivity (range));
      break;
    case PROP_SHOW_FILL_LEVEL:
      g_value_set_boolean (value, ctk_range_get_show_fill_level (range));
      break;
    case PROP_RESTRICT_TO_FILL_LEVEL:
      g_value_set_boolean (value, ctk_range_get_restrict_to_fill_level (range));
      break;
    case PROP_FILL_LEVEL:
      g_value_set_double (value, ctk_range_get_fill_level (range));
      break;
    case PROP_ROUND_DIGITS:
      g_value_set_int (value, ctk_range_get_round_digits (range));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_range_init (CtkRange *range)
{
  CtkRangePrivate *priv;
  CtkCssNode *widget_node;

  range->priv = ctk_range_get_instance_private (range);
  priv = range->priv;

  ctk_widget_set_has_window (CTK_WIDGET (range), FALSE);

  priv->orientation = CTK_ORIENTATION_HORIZONTAL;
  priv->adjustment = NULL;
  priv->inverted = FALSE;
  priv->flippable = FALSE;
  priv->min_slider_size = 1;
  priv->round_digits = -1;
  priv->mouse_x = G_MININT;
  priv->mouse_y = G_MININT;
  priv->lower_sensitivity = CTK_SENSITIVITY_AUTO;
  priv->upper_sensitivity = CTK_SENSITIVITY_AUTO;
  priv->lower_sensitive = TRUE;
  priv->upper_sensitive = TRUE;
  priv->has_origin = FALSE;
  priv->show_fill_level = FALSE;
  priv->restrict_to_fill_level = TRUE;
  priv->fill_level = G_MAXDOUBLE;
  priv->timer = NULL;

  _ctk_orientable_set_style_classes (CTK_ORIENTABLE (range));

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (range));
  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     CTK_WIDGET (range),
                                                     ctk_range_measure,
                                                     ctk_range_allocate,
                                                     ctk_range_render,
                                                     NULL, NULL);
  priv->contents_gadget = ctk_box_gadget_new ("contents",
                                              CTK_WIDGET (range),
                                              priv->gadget, NULL);
  priv->trough_gadget = ctk_css_custom_gadget_new ("trough",
                                                   CTK_WIDGET (range),
                                                   NULL, NULL,
                                                   ctk_range_measure_trough,
                                                   ctk_range_allocate_trough,
                                                   ctk_range_render_trough,
                                                   NULL, NULL);
  ctk_css_gadget_set_state (priv->trough_gadget,
                            ctk_css_node_get_state (widget_node));
  ctk_box_gadget_insert_gadget (CTK_BOX_GADGET (priv->contents_gadget), -1, priv->trough_gadget,
                                TRUE, CTK_ALIGN_CENTER);

  priv->slider_gadget = ctk_builtin_icon_new ("slider",
                                              CTK_WIDGET (range),
                                              priv->trough_gadget, NULL);
  ctk_css_gadget_set_state (priv->slider_gadget,
                            ctk_css_node_get_state (widget_node));

  /* Note: Order is important here.
   * The ::drag-begin handler relies on the state set up by the
   * multipress ::pressed handler. Gestures are handling events
   * in the oppposite order in which they are added to their
   * widget.
   */
  priv->drag_gesture = ctk_gesture_drag_new (CTK_WIDGET (range));
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (priv->drag_gesture), 0);
  g_signal_connect (priv->drag_gesture, "drag-begin",
                    G_CALLBACK (ctk_range_drag_gesture_begin), range);
  g_signal_connect (priv->drag_gesture, "drag-update",
                    G_CALLBACK (ctk_range_drag_gesture_update), range);

  priv->multipress_gesture = ctk_gesture_multi_press_new (CTK_WIDGET (range));
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (priv->multipress_gesture), 0);
  ctk_gesture_group (priv->drag_gesture, priv->multipress_gesture);
  g_signal_connect (priv->multipress_gesture, "pressed",
                    G_CALLBACK (ctk_range_multipress_gesture_pressed), range);
  g_signal_connect (priv->multipress_gesture, "released",
                    G_CALLBACK (ctk_range_multipress_gesture_released), range);

  priv->long_press_gesture = ctk_gesture_long_press_new (CTK_WIDGET (range));
  g_object_set (priv->long_press_gesture, "delay-factor", 2.0, NULL);
  ctk_gesture_group (priv->drag_gesture, priv->long_press_gesture);
  g_signal_connect (priv->long_press_gesture, "pressed",
                    G_CALLBACK (ctk_range_long_press_gesture_pressed), range);
}

/**
 * ctk_range_get_adjustment:
 * @range: a #CtkRange
 * 
 * Get the #CtkAdjustment which is the “model” object for #CtkRange.
 * See ctk_range_set_adjustment() for details.
 * The return value does not have a reference added, so should not
 * be unreferenced.
 * 
 * Returns: (transfer none): a #CtkAdjustment
 **/
CtkAdjustment*
ctk_range_get_adjustment (CtkRange *range)
{
  CtkRangePrivate *priv;

  g_return_val_if_fail (CTK_IS_RANGE (range), NULL);

  priv = range->priv;

  if (!priv->adjustment)
    ctk_range_set_adjustment (range, NULL);

  return priv->adjustment;
}

/**
 * ctk_range_set_adjustment:
 * @range: a #CtkRange
 * @adjustment: a #CtkAdjustment
 *
 * Sets the adjustment to be used as the “model” object for this range
 * widget. The adjustment indicates the current range value, the
 * minimum and maximum range values, the step/page increments used
 * for keybindings and scrolling, and the page size. The page size
 * is normally 0 for #CtkScale and nonzero for #CtkScrollbar, and
 * indicates the size of the visible area of the widget being scrolled.
 * The page size affects the size of the scrollbar slider.
 **/
void
ctk_range_set_adjustment (CtkRange      *range,
			  CtkAdjustment *adjustment)
{
  CtkRangePrivate *priv;

  g_return_if_fail (CTK_IS_RANGE (range));

  priv = range->priv;

  if (!adjustment)
    adjustment = ctk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  else
    g_return_if_fail (CTK_IS_ADJUSTMENT (adjustment));

  if (priv->adjustment != adjustment)
    {
      if (priv->adjustment)
	{
	  g_signal_handlers_disconnect_by_func (priv->adjustment,
						ctk_range_adjustment_changed,
						range);
	  g_signal_handlers_disconnect_by_func (priv->adjustment,
						ctk_range_adjustment_value_changed,
						range);
	  g_object_unref (priv->adjustment);
	}

      priv->adjustment = adjustment;
      g_object_ref_sink (adjustment);
      
      g_signal_connect (adjustment, "changed",
			G_CALLBACK (ctk_range_adjustment_changed),
			range);
      g_signal_connect (adjustment, "value-changed",
			G_CALLBACK (ctk_range_adjustment_value_changed),
			range);
      
      ctk_range_adjustment_changed (adjustment, range);
      g_object_notify_by_pspec (G_OBJECT (range), properties[PROP_ADJUSTMENT]);
    }
}

static gboolean
should_invert (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;

  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    return
      (priv->inverted && !priv->flippable) ||
      (priv->inverted && priv->flippable && ctk_widget_get_direction (CTK_WIDGET (range)) == CTK_TEXT_DIR_LTR) ||
      (!priv->inverted && priv->flippable && ctk_widget_get_direction (CTK_WIDGET (range)) == CTK_TEXT_DIR_RTL);
  else
    return priv->inverted;
}

static gboolean
should_invert_move (CtkRange       *range,
                    CtkOrientation  move_orientation)
{
  CtkRangePrivate *priv = range->priv;

  /* If the move is parallel to the range, use general check for inversion */
  if (move_orientation == priv->orientation)
    return should_invert (range);

  /* H scale/V move: Always invert, so down/up always dec/increase the value */
  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL && CTK_IS_SCALE (range))
    return TRUE;

  /* V range/H move: Left/right always dec/increase the value */
  return FALSE;
}

static void
update_highlight_position (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;

  if (!priv->highlight_gadget)
    return;

  if (should_invert (range))
    {
      ctk_css_gadget_remove_class (priv->highlight_gadget, CTK_STYLE_CLASS_TOP);
      ctk_css_gadget_add_class (priv->highlight_gadget, CTK_STYLE_CLASS_BOTTOM);
    }
  else
    {
      ctk_css_gadget_remove_class (priv->highlight_gadget, CTK_STYLE_CLASS_BOTTOM);
      ctk_css_gadget_add_class (priv->highlight_gadget, CTK_STYLE_CLASS_TOP);
    }
}

static void
update_fill_position (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;

  if (!priv->fill_gadget)
    return;

  if (should_invert (range))
    {
      ctk_css_gadget_remove_class (priv->fill_gadget, CTK_STYLE_CLASS_TOP);
      ctk_css_gadget_add_class (priv->fill_gadget, CTK_STYLE_CLASS_BOTTOM);
    }
  else
    {
      ctk_css_gadget_remove_class (priv->fill_gadget, CTK_STYLE_CLASS_BOTTOM);
      ctk_css_gadget_add_class (priv->fill_gadget, CTK_STYLE_CLASS_TOP);
    }
}

static void
update_stepper_state (CtkRange     *range,
                      CtkCssGadget *gadget)
{
  CtkRangePrivate *priv = range->priv;
  CtkStateFlags state;
  gboolean arrow_sensitive;

  state = ctk_widget_get_state_flags (CTK_WIDGET (range));

  if ((!priv->inverted &&
       (gadget == priv->stepper_a_gadget || gadget == priv->stepper_c_gadget)) ||
      (priv->inverted &&
       (gadget == priv->stepper_b_gadget || gadget == priv->stepper_d_gadget)))
    arrow_sensitive = priv->lower_sensitive;
  else
    arrow_sensitive = priv->upper_sensitive;

  state &= ~(CTK_STATE_FLAG_ACTIVE | CTK_STATE_FLAG_PRELIGHT);

  if ((state & CTK_STATE_FLAG_INSENSITIVE) || !arrow_sensitive)
    {
      state |= CTK_STATE_FLAG_INSENSITIVE;
    }
  else
    {
      if (priv->grab_location == gadget)
        state |= CTK_STATE_FLAG_ACTIVE;
      if (priv->mouse_location == gadget)
        state |= CTK_STATE_FLAG_PRELIGHT;
    }

  ctk_css_gadget_set_state (gadget, state);
}

static void
update_steppers_state (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;

  if (priv->stepper_a_gadget)
    update_stepper_state (range, priv->stepper_a_gadget);
  if (priv->stepper_b_gadget)
    update_stepper_state (range, priv->stepper_b_gadget);
  if (priv->stepper_c_gadget)
    update_stepper_state (range, priv->stepper_c_gadget);
  if (priv->stepper_d_gadget)
    update_stepper_state (range, priv->stepper_d_gadget);
}

/**
 * ctk_range_set_inverted:
 * @range: a #CtkRange
 * @setting: %TRUE to invert the range
 *
 * Ranges normally move from lower to higher values as the
 * slider moves from top to bottom or left to right. Inverted
 * ranges have higher values at the top or on the right rather than
 * on the bottom or left.
 **/
void
ctk_range_set_inverted (CtkRange *range,
                        gboolean  setting)
{
  CtkRangePrivate *priv;

  g_return_if_fail (CTK_IS_RANGE (range));

  priv = range->priv;

  setting = setting != FALSE;

  if (setting != priv->inverted)
    {
      priv->inverted = setting;

      update_steppers_state (range);
      update_fill_position (range);
      update_highlight_position (range);

      ctk_widget_queue_resize (CTK_WIDGET (range));

      g_object_notify_by_pspec (G_OBJECT (range), properties[PROP_INVERTED]);
    }
}

/**
 * ctk_range_get_inverted:
 * @range: a #CtkRange
 * 
 * Gets the value set by ctk_range_set_inverted().
 * 
 * Returns: %TRUE if the range is inverted
 **/
gboolean
ctk_range_get_inverted (CtkRange *range)
{
  g_return_val_if_fail (CTK_IS_RANGE (range), FALSE);

  return range->priv->inverted;
}

/**
 * ctk_range_set_flippable:
 * @range: a #CtkRange
 * @flippable: %TRUE to make the range flippable
 *
 * If a range is flippable, it will switch its direction if it is
 * horizontal and its direction is %CTK_TEXT_DIR_RTL.
 *
 * See ctk_widget_get_direction().
 *
 * Since: 2.18
 **/
void
ctk_range_set_flippable (CtkRange *range,
                         gboolean  flippable)
{
  CtkRangePrivate *priv;

  g_return_if_fail (CTK_IS_RANGE (range));

  priv = range->priv;

  flippable = flippable ? TRUE : FALSE;

  if (flippable != priv->flippable)
    {
      priv->flippable = flippable;
      update_fill_position (range);
      update_highlight_position (range);

      ctk_widget_queue_allocate (CTK_WIDGET (range));
    }
}

/**
 * ctk_range_get_flippable:
 * @range: a #CtkRange
 *
 * Gets the value set by ctk_range_set_flippable().
 *
 * Returns: %TRUE if the range is flippable
 *
 * Since: 2.18
 **/
gboolean
ctk_range_get_flippable (CtkRange *range)
{
  g_return_val_if_fail (CTK_IS_RANGE (range), FALSE);

  return range->priv->flippable;
}

void
ctk_range_set_slider_use_min_size (CtkRange *range,
                                   gboolean  use_min_size)
{
  CtkRangePrivate *priv = range->priv;

  if (use_min_size != priv->slider_use_min_size)
    {
      priv->slider_use_min_size = use_min_size;
      ctk_css_gadget_queue_resize (priv->slider_gadget);
    }
}

/**
 * ctk_range_set_slider_size_fixed:
 * @range: a #CtkRange
 * @size_fixed: %TRUE to make the slider size constant
 *
 * Sets whether the range’s slider has a fixed size, or a size that
 * depends on its adjustment’s page size.
 *
 * This function is useful mainly for #CtkRange subclasses.
 *
 * Since: 2.20
 **/
void
ctk_range_set_slider_size_fixed (CtkRange *range,
                                 gboolean  size_fixed)
{
  CtkRangePrivate *priv;

  g_return_if_fail (CTK_IS_RANGE (range));

  priv = range->priv;

  if (size_fixed != priv->slider_size_fixed)
    {
      priv->slider_size_fixed = size_fixed ? TRUE : FALSE;

      if (priv->adjustment && ctk_widget_get_mapped (CTK_WIDGET (range)))
        ctk_css_gadget_queue_allocate (priv->slider_gadget);
    }
}

/**
 * ctk_range_get_slider_size_fixed:
 * @range: a #CtkRange
 *
 * This function is useful mainly for #CtkRange subclasses.
 *
 * See ctk_range_set_slider_size_fixed().
 *
 * Returns: whether the range’s slider has a fixed size.
 *
 * Since: 2.20
 **/
gboolean
ctk_range_get_slider_size_fixed (CtkRange *range)
{
  g_return_val_if_fail (CTK_IS_RANGE (range), FALSE);

  return range->priv->slider_size_fixed;
}

/**
 * ctk_range_set_min_slider_size:
 * @range: a #CtkRange
 * @min_size: The slider’s minimum size
 *
 * Sets the minimum size of the range’s slider.
 *
 * This function is useful mainly for #CtkRange subclasses.
 *
 * Since: 2.20
 *
 * Deprecated: 3.20: Use the min-height/min-width CSS properties on the slider
 *   node.
 **/
void
ctk_range_set_min_slider_size (CtkRange *range,
                               gint      min_size)
{
  CtkRangePrivate *priv;

  g_return_if_fail (CTK_IS_RANGE (range));
  g_return_if_fail (min_size > 0);

  priv = range->priv;

  if (min_size != priv->min_slider_size)
    {
      priv->min_slider_size = min_size;

      ctk_widget_queue_resize (CTK_WIDGET (range));
    }
}

/**
 * ctk_range_get_min_slider_size:
 * @range: a #CtkRange
 *
 * This function is useful mainly for #CtkRange subclasses.
 *
 * See ctk_range_set_min_slider_size().
 *
 * Returns: The minimum size of the range’s slider.
 *
 * Since: 2.20
 *
 * Deprecated: 3.20: Use the min-height/min-width CSS properties on the slider
 *   node.
 **/
gint
ctk_range_get_min_slider_size (CtkRange *range)
{
  g_return_val_if_fail (CTK_IS_RANGE (range), FALSE);

  return range->priv->min_slider_size;
}

static void
measure_one_gadget (CtkCssGadget *gadget,
                    int          *width_out,
                    int          *height_out)
{
  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_HORIZONTAL, -1,
                                     width_out, NULL,
                                     NULL, NULL);
  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_VERTICAL, -1,
                                     height_out, NULL,
                                     NULL, NULL);
}

/**
 * ctk_range_get_range_rect:
 * @range: a #CtkRange
 * @range_rect: (out): return location for the range rectangle
 *
 * This function returns the area that contains the range’s trough
 * and its steppers, in widget->window coordinates.
 *
 * This function is useful mainly for #CtkRange subclasses.
 *
 * Since: 2.20
 **/
void
ctk_range_get_range_rect (CtkRange     *range,
                          CdkRectangle *range_rect)
{
  CtkRangePrivate *priv;

  g_return_if_fail (CTK_IS_RANGE (range));
  g_return_if_fail (range_rect != NULL);

  priv = range->priv;

  ctk_css_gadget_get_margin_box (priv->contents_gadget, range_rect);
}

/**
 * ctk_range_get_slider_range:
 * @range: a #CtkRange
 * @slider_start: (out) (allow-none): return location for the slider's
 *     start, or %NULL
 * @slider_end: (out) (allow-none): return location for the slider's
 *     end, or %NULL
 *
 * This function returns sliders range along the long dimension,
 * in widget->window coordinates.
 *
 * This function is useful mainly for #CtkRange subclasses.
 *
 * Since: 2.20
 **/
void
ctk_range_get_slider_range (CtkRange *range,
                            gint     *slider_start,
                            gint     *slider_end)
{
  CtkRangePrivate *priv;
  CtkAllocation slider_alloc;

  g_return_if_fail (CTK_IS_RANGE (range));

  priv = range->priv;

  ctk_css_gadget_get_margin_box (priv->slider_gadget, &slider_alloc);

  if (priv->orientation == CTK_ORIENTATION_VERTICAL)
    {
      if (slider_start)
        *slider_start = slider_alloc.y;
      if (slider_end)
        *slider_end = slider_alloc.y + slider_alloc.height;
    }
  else
    {
      if (slider_start)
        *slider_start = slider_alloc.x;
      if (slider_end)
        *slider_end = slider_alloc.x + slider_alloc.width;
    }
}

/**
 * ctk_range_set_lower_stepper_sensitivity:
 * @range:       a #CtkRange
 * @sensitivity: the lower stepper’s sensitivity policy.
 *
 * Sets the sensitivity policy for the stepper that points to the
 * 'lower' end of the CtkRange’s adjustment.
 *
 * Since: 2.10
 **/
void
ctk_range_set_lower_stepper_sensitivity (CtkRange           *range,
                                         CtkSensitivityType  sensitivity)
{
  CtkRangePrivate *priv;

  g_return_if_fail (CTK_IS_RANGE (range));

  priv = range->priv;

  if (priv->lower_sensitivity != sensitivity)
    {
      priv->lower_sensitivity = sensitivity;

      ctk_range_calc_stepper_sensitivity (range);

      g_object_notify_by_pspec (G_OBJECT (range), properties[PROP_LOWER_STEPPER_SENSITIVITY]);
    }
}

/**
 * ctk_range_get_lower_stepper_sensitivity:
 * @range: a #CtkRange
 *
 * Gets the sensitivity policy for the stepper that points to the
 * 'lower' end of the CtkRange’s adjustment.
 *
 * Returns: The lower stepper’s sensitivity policy.
 *
 * Since: 2.10
 **/
CtkSensitivityType
ctk_range_get_lower_stepper_sensitivity (CtkRange *range)
{
  g_return_val_if_fail (CTK_IS_RANGE (range), CTK_SENSITIVITY_AUTO);

  return range->priv->lower_sensitivity;
}

/**
 * ctk_range_set_upper_stepper_sensitivity:
 * @range:       a #CtkRange
 * @sensitivity: the upper stepper’s sensitivity policy.
 *
 * Sets the sensitivity policy for the stepper that points to the
 * 'upper' end of the CtkRange’s adjustment.
 *
 * Since: 2.10
 **/
void
ctk_range_set_upper_stepper_sensitivity (CtkRange           *range,
                                         CtkSensitivityType  sensitivity)
{
  CtkRangePrivate *priv;

  g_return_if_fail (CTK_IS_RANGE (range));

  priv = range->priv;

  if (priv->upper_sensitivity != sensitivity)
    {
      priv->upper_sensitivity = sensitivity;

      ctk_range_calc_stepper_sensitivity (range);

      g_object_notify_by_pspec (G_OBJECT (range), properties[PROP_UPPER_STEPPER_SENSITIVITY]);
    }
}

/**
 * ctk_range_get_upper_stepper_sensitivity:
 * @range: a #CtkRange
 *
 * Gets the sensitivity policy for the stepper that points to the
 * 'upper' end of the CtkRange’s adjustment.
 *
 * Returns: The upper stepper’s sensitivity policy.
 *
 * Since: 2.10
 **/
CtkSensitivityType
ctk_range_get_upper_stepper_sensitivity (CtkRange *range)
{
  g_return_val_if_fail (CTK_IS_RANGE (range), CTK_SENSITIVITY_AUTO);

  return range->priv->upper_sensitivity;
}

/**
 * ctk_range_set_increments:
 * @range: a #CtkRange
 * @step: step size
 * @page: page size
 *
 * Sets the step and page sizes for the range.
 * The step size is used when the user clicks the #CtkScrollbar
 * arrows or moves #CtkScale via arrow keys. The page size
 * is used for example when moving via Page Up or Page Down keys.
 **/
void
ctk_range_set_increments (CtkRange *range,
                          gdouble   step,
                          gdouble   page)
{
  CtkAdjustment *adjustment;

  g_return_if_fail (CTK_IS_RANGE (range));

  adjustment = range->priv->adjustment;

  ctk_adjustment_configure (adjustment,
                            ctk_adjustment_get_value (adjustment),
                            ctk_adjustment_get_lower (adjustment),
                            ctk_adjustment_get_upper (adjustment),
                            step,
                            page,
                            ctk_adjustment_get_page_size (adjustment));
}

/**
 * ctk_range_set_range:
 * @range: a #CtkRange
 * @min: minimum range value
 * @max: maximum range value
 * 
 * Sets the allowable values in the #CtkRange, and clamps the range
 * value to be between @min and @max. (If the range has a non-zero
 * page size, it is clamped between @min and @max - page-size.)
 **/
void
ctk_range_set_range (CtkRange *range,
                     gdouble   min,
                     gdouble   max)
{
  CtkRangePrivate *priv;
  CtkAdjustment *adjustment;
  gdouble value;
  
  g_return_if_fail (CTK_IS_RANGE (range));
  g_return_if_fail (min <= max);

  priv = range->priv;
  adjustment = priv->adjustment;

  value = ctk_adjustment_get_value (adjustment);
  if (priv->restrict_to_fill_level)
    value = MIN (value, MAX (ctk_adjustment_get_lower (adjustment),
                             priv->fill_level));

  ctk_adjustment_configure (adjustment,
                            value,
                            min,
                            max,
                            ctk_adjustment_get_step_increment (adjustment),
                            ctk_adjustment_get_page_increment (adjustment),
                            ctk_adjustment_get_page_size (adjustment));
}

/**
 * ctk_range_set_value:
 * @range: a #CtkRange
 * @value: new value of the range
 *
 * Sets the current value of the range; if the value is outside the
 * minimum or maximum range values, it will be clamped to fit inside
 * them. The range emits the #CtkRange::value-changed signal if the 
 * value changes.
 **/
void
ctk_range_set_value (CtkRange *range,
                     gdouble   value)
{
  CtkRangePrivate *priv;

  g_return_if_fail (CTK_IS_RANGE (range));

  priv = range->priv;

  if (priv->restrict_to_fill_level)
    value = MIN (value, MAX (ctk_adjustment_get_lower (priv->adjustment),
                             priv->fill_level));

  ctk_adjustment_set_value (priv->adjustment, value);
}

/**
 * ctk_range_get_value:
 * @range: a #CtkRange
 * 
 * Gets the current value of the range.
 * 
 * Returns: current value of the range.
 **/
gdouble
ctk_range_get_value (CtkRange *range)
{
  g_return_val_if_fail (CTK_IS_RANGE (range), 0.0);

  return ctk_adjustment_get_value (range->priv->adjustment);
}

/**
 * ctk_range_set_show_fill_level:
 * @range:           A #CtkRange
 * @show_fill_level: Whether a fill level indicator graphics is shown.
 *
 * Sets whether a graphical fill level is show on the trough. See
 * ctk_range_set_fill_level() for a general description of the fill
 * level concept.
 *
 * Since: 2.12
 **/
void
ctk_range_set_show_fill_level (CtkRange *range,
                               gboolean  show_fill_level)
{
  CtkRangePrivate *priv;

  g_return_if_fail (CTK_IS_RANGE (range));

  priv = range->priv;

  show_fill_level = show_fill_level ? TRUE : FALSE;

  if (show_fill_level == priv->show_fill_level)
    return;

  priv->show_fill_level = show_fill_level;

  if (show_fill_level)
    {
      priv->fill_gadget = ctk_css_custom_gadget_new ("fill",
                                                     CTK_WIDGET (range),
                                                     priv->trough_gadget, NULL,
                                                     NULL, NULL, NULL,
                                                     NULL, NULL);
      ctk_css_gadget_set_state (priv->fill_gadget,
                                ctk_css_node_get_state (ctk_css_gadget_get_node (priv->trough_gadget)));

      update_fill_position (range);
    }
  else
    {
      ctk_css_node_set_parent (ctk_css_gadget_get_node (priv->fill_gadget), NULL);
      g_clear_object (&priv->fill_gadget);
    }

  g_object_notify_by_pspec (G_OBJECT (range), properties[PROP_SHOW_FILL_LEVEL]);
  ctk_widget_queue_allocate (CTK_WIDGET (range));
}

/**
 * ctk_range_get_show_fill_level:
 * @range: A #CtkRange
 *
 * Gets whether the range displays the fill level graphically.
 *
 * Returns: %TRUE if @range shows the fill level.
 *
 * Since: 2.12
 **/
gboolean
ctk_range_get_show_fill_level (CtkRange *range)
{
  g_return_val_if_fail (CTK_IS_RANGE (range), FALSE);

  return range->priv->show_fill_level;
}

/**
 * ctk_range_set_restrict_to_fill_level:
 * @range:                  A #CtkRange
 * @restrict_to_fill_level: Whether the fill level restricts slider movement.
 *
 * Sets whether the slider is restricted to the fill level. See
 * ctk_range_set_fill_level() for a general description of the fill
 * level concept.
 *
 * Since: 2.12
 **/
void
ctk_range_set_restrict_to_fill_level (CtkRange *range,
                                      gboolean  restrict_to_fill_level)
{
  CtkRangePrivate *priv;

  g_return_if_fail (CTK_IS_RANGE (range));

  priv = range->priv;

  restrict_to_fill_level = restrict_to_fill_level ? TRUE : FALSE;

  if (restrict_to_fill_level != priv->restrict_to_fill_level)
    {
      priv->restrict_to_fill_level = restrict_to_fill_level;
      g_object_notify_by_pspec (G_OBJECT (range), properties[PROP_RESTRICT_TO_FILL_LEVEL]);

      ctk_range_set_value (range, ctk_range_get_value (range));
    }
}

/**
 * ctk_range_get_restrict_to_fill_level:
 * @range: A #CtkRange
 *
 * Gets whether the range is restricted to the fill level.
 *
 * Returns: %TRUE if @range is restricted to the fill level.
 *
 * Since: 2.12
 **/
gboolean
ctk_range_get_restrict_to_fill_level (CtkRange *range)
{
  g_return_val_if_fail (CTK_IS_RANGE (range), FALSE);

  return range->priv->restrict_to_fill_level;
}

/**
 * ctk_range_set_fill_level:
 * @range: a #CtkRange
 * @fill_level: the new position of the fill level indicator
 *
 * Set the new position of the fill level indicator.
 *
 * The “fill level” is probably best described by its most prominent
 * use case, which is an indicator for the amount of pre-buffering in
 * a streaming media player. In that use case, the value of the range
 * would indicate the current play position, and the fill level would
 * be the position up to which the file/stream has been downloaded.
 *
 * This amount of prebuffering can be displayed on the range’s trough
 * and is themeable separately from the trough. To enable fill level
 * display, use ctk_range_set_show_fill_level(). The range defaults
 * to not showing the fill level.
 *
 * Additionally, it’s possible to restrict the range’s slider position
 * to values which are smaller than the fill level. This is controller
 * by ctk_range_set_restrict_to_fill_level() and is by default
 * enabled.
 *
 * Since: 2.12
 **/
void
ctk_range_set_fill_level (CtkRange *range,
                          gdouble   fill_level)
{
  CtkRangePrivate *priv;

  g_return_if_fail (CTK_IS_RANGE (range));

  priv = range->priv;

  if (fill_level != priv->fill_level)
    {
      priv->fill_level = fill_level;
      g_object_notify_by_pspec (G_OBJECT (range), properties[PROP_FILL_LEVEL]);

      if (priv->show_fill_level)
        ctk_widget_queue_allocate (CTK_WIDGET (range));

      if (priv->restrict_to_fill_level)
        ctk_range_set_value (range, ctk_range_get_value (range));
    }
}

/**
 * ctk_range_get_fill_level:
 * @range: A #CtkRange
 *
 * Gets the current position of the fill level indicator.
 *
 * Returns: The current fill level
 *
 * Since: 2.12
 **/
gdouble
ctk_range_get_fill_level (CtkRange *range)
{
  g_return_val_if_fail (CTK_IS_RANGE (range), 0.0);

  return range->priv->fill_level;
}

static void
ctk_range_destroy (CtkWidget *widget)
{
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;

  ctk_range_remove_step_timer (range);

  if (priv->adjustment)
    {
      g_signal_handlers_disconnect_by_func (priv->adjustment,
					    ctk_range_adjustment_changed,
					    range);
      g_signal_handlers_disconnect_by_func (priv->adjustment,
					    ctk_range_adjustment_value_changed,
					    range);
      g_object_unref (priv->adjustment);
      priv->adjustment = NULL;
    }

  if (priv->n_marks)
    {
      g_free (priv->marks);
      priv->marks = NULL;
      g_free (priv->mark_pos);
      priv->mark_pos = NULL;
      priv->n_marks = 0;
    }

  CTK_WIDGET_CLASS (ctk_range_parent_class)->destroy (widget);
}

static void
ctk_range_finalize (GObject *object)
{
  CtkRange *range = CTK_RANGE (object);
  CtkRangePrivate *priv = range->priv;

  g_clear_object (&priv->drag_gesture);
  g_clear_object (&priv->multipress_gesture);
  g_clear_object (&priv->long_press_gesture);

  g_clear_object (&priv->gadget);
  g_clear_object (&priv->contents_gadget);
  g_clear_object (&priv->trough_gadget);
  g_clear_object (&priv->fill_gadget);
  g_clear_object (&priv->highlight_gadget);
  g_clear_object (&priv->slider_gadget);
  g_clear_object (&priv->stepper_a_gadget);
  g_clear_object (&priv->stepper_b_gadget);
  g_clear_object (&priv->stepper_c_gadget);
  g_clear_object (&priv->stepper_d_gadget);

  G_OBJECT_CLASS (ctk_range_parent_class)->finalize (object);
}

static void
ctk_range_measure_trough (CtkCssGadget   *gadget,
                          CtkOrientation  orientation,
                          gint            for_size,
                          gint           *minimum,
                          gint           *natural,
                          gint           *minimum_baseline,
                          gint           *natural_baseline,
                          gpointer        user_data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;
  gint min, nat;

  ctk_css_gadget_get_preferred_size (priv->slider_gadget,
                                     orientation, -1,
                                     minimum, natural,
                                     NULL, NULL);

  if (priv->fill_gadget)
    {
      ctk_css_gadget_get_preferred_size (priv->fill_gadget,
                                         orientation, for_size,
                                         &min, &nat,
                                         NULL, NULL);
      *minimum = MAX (*minimum, min);
      *natural = MAX (*natural, nat);
    }

  if (priv->highlight_gadget)
    {
      ctk_css_gadget_get_preferred_size (priv->highlight_gadget,
                                         orientation, for_size,
                                         &min, &nat,
                                         NULL, NULL);
      *minimum = MAX (*minimum, min);
      *natural = MAX (*natural, nat);
    }
}

static void
ctk_range_measure (CtkCssGadget   *gadget,
                   CtkOrientation  orientation,
                   gint            for_size,
                   gint           *minimum,
                   gint           *natural,
                   gint           *minimum_baseline,
                   gint           *natural_baseline,
                   gpointer        user_data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;
  CtkBorder border = { 0 };

  /* Measure the main box */
  ctk_css_gadget_get_preferred_size (priv->contents_gadget,
                                     orientation,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);

  if (CTK_RANGE_GET_CLASS (range)->get_range_border)
    CTK_RANGE_GET_CLASS (range)->get_range_border (range, &border);

  /* Add the border */
  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      *minimum += border.left + border.right;
      *natural += border.left + border.right;
    }
  else
    {
      *minimum += border.top + border.bottom;
      *natural += border.top + border.bottom;
    }
}

static void
ctk_range_size_request (CtkWidget      *widget,
                        CtkOrientation  orientation,
                        gint           *minimum,
                        gint           *natural)
{
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;

  ctk_css_gadget_get_preferred_size (priv->gadget, orientation, -1,
                                     minimum, natural,
                                     NULL, NULL);

  if (CTK_RANGE_GET_CLASS (range)->get_range_size_request)
    {
      gint min, nat;

      CTK_RANGE_GET_CLASS (range)->get_range_size_request (range, orientation,
                                                           &min, &nat);

      *minimum = MAX (*minimum, min);
      *natural = MAX (*natural, nat);
    }
}

static void
ctk_range_get_preferred_width (CtkWidget *widget,
                               gint      *minimum,
                               gint      *natural)
{
  ctk_range_size_request (widget, CTK_ORIENTATION_HORIZONTAL,
                          minimum, natural);
}

static void
ctk_range_get_preferred_height (CtkWidget *widget,
                                gint      *minimum,
                                gint      *natural)
{
  ctk_range_size_request (widget, CTK_ORIENTATION_VERTICAL,
                          minimum, natural);
}

static void
ctk_range_allocate_trough (CtkCssGadget        *gadget,
                           const CtkAllocation *allocation,
                           int                  baseline,
                           CtkAllocation       *out_clip,
                           gpointer             data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;
  CtkAllocation slider_alloc, widget_alloc;

  /* Slider */
  ctk_range_calc_marks (range);
  ctk_range_calc_stepper_sensitivity (range);

  ctk_widget_get_allocation (widget, &widget_alloc);
  ctk_range_compute_slider_position (range,
                                     ctk_adjustment_get_value (priv->adjustment),
                                     &slider_alloc);
  slider_alloc.x += widget_alloc.x;
  slider_alloc.y += widget_alloc.y;

  ctk_css_gadget_allocate (priv->slider_gadget,
                           &slider_alloc,
                           ctk_widget_get_allocated_baseline (widget),
                           out_clip);

  if (priv->show_fill_level &&
      ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_page_size (priv->adjustment) -
      ctk_adjustment_get_lower (priv->adjustment) != 0)
    {
      gdouble level, fill;
      CtkAllocation fill_alloc, fill_clip;

      fill_alloc = *allocation;

      level = CLAMP (priv->fill_level,
                     ctk_adjustment_get_lower (priv->adjustment),
                     ctk_adjustment_get_upper (priv->adjustment) -
                     ctk_adjustment_get_page_size (priv->adjustment));

      fill = (level - ctk_adjustment_get_lower (priv->adjustment)) /
        (ctk_adjustment_get_upper (priv->adjustment) -
         ctk_adjustment_get_lower (priv->adjustment) -
         ctk_adjustment_get_page_size (priv->adjustment));

      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          fill_alloc.width *= fill;

          if (should_invert (range))
            fill_alloc.x += allocation->width - fill_alloc.width;
        }
      else
        {
          fill_alloc.height *= fill;

          if (should_invert (range))
            fill_alloc.y += allocation->height - fill_alloc.height;
        }

      ctk_css_gadget_allocate (priv->fill_gadget,
                               &fill_alloc,
                               baseline,
                               &fill_clip);
      cdk_rectangle_union (out_clip, &fill_clip, out_clip);
    }

  if (priv->has_origin)
    {
      CtkAllocation highlight_alloc, highlight_clip;
      int min, nat;

      ctk_css_gadget_get_preferred_size (priv->highlight_gadget,
                                         priv->orientation, -1,
                                         &min, &nat,
                                         NULL, NULL);

      highlight_alloc = *allocation;

      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          int x = slider_alloc.x + slider_alloc.width / 2;

          if (!should_invert (range))
            {
              highlight_alloc.x = allocation->x;
              highlight_alloc.width = MAX (x - allocation->x, min);
            }
          else
            {
              highlight_alloc.width = MAX (allocation->x + allocation->width - x, min);
              highlight_alloc.x = allocation->x + allocation->width - highlight_alloc.width;
            }
        }
      else
        {
          int y = slider_alloc.y + slider_alloc.height / 2;

          if (!should_invert (range))
            {
              highlight_alloc.y = allocation->y;
              highlight_alloc.height = MAX (y - allocation->y, min);
            }
          else
            {
              highlight_alloc.height = MAX (allocation->y + allocation->height - y, min);
              highlight_alloc.y = allocation->y + allocation->height - highlight_alloc.height;
            }
        }

      ctk_css_gadget_allocate (priv->highlight_gadget,
                               &highlight_alloc,
                               baseline,
                               &highlight_clip);
      cdk_rectangle_union (out_clip, &highlight_clip, out_clip);
    }
}

/* Clamp dimensions and border inside allocation, such that we prefer
 * to take space from border not dimensions in all directions, and prefer to
 * give space to border over dimensions in one direction.
 */
static void
clamp_dimensions (const CtkAllocation *allocation,
                  int                 *width,
                  int                 *height,
                  CtkBorder           *border,
                  gboolean             border_expands_horizontally)
{
  gint extra, shortage;

  /* Width */
  extra = allocation->width - border->left - border->right - *width;
  if (extra > 0)
    {
      if (border_expands_horizontally)
        {
          border->left += extra / 2;
          border->right += extra / 2 + extra % 2;
        }
      else
        {
          *width += extra;
        }
    }
  
  /* See if we can fit rect, if not kill the border */
  shortage = *width - allocation->width;
  if (shortage > 0)
    {
      *width = allocation->width;
      /* lose the border */
      border->left = 0;
      border->right = 0;
    }
  else
    {
      /* See if we can fit rect with borders */
      shortage = *width + border->left + border->right - allocation->width;
      if (shortage > 0)
        {
          /* Shrink borders */
          border->left -= shortage / 2;
          border->right -= shortage / 2 + shortage % 2;
        }
    }

  /* Height */
  extra = allocation->height - border->top - border->bottom - *height;
  if (extra > 0)
    {
      if (border_expands_horizontally)
        {
          /* don't expand border vertically */
          *height += extra;
        }
      else
        {
          border->top += extra / 2;
          border->bottom += extra / 2 + extra % 2;
        }
    }
  
  /* See if we can fit rect, if not kill the border */
  shortage = *height - allocation->height;
  if (shortage > 0)
    {
      *height = allocation->height;
      /* lose the border */
      border->top = 0;
      border->bottom = 0;
    }
  else
    {
      /* See if we can fit rect with borders */
      shortage = *height + border->top + border->bottom - allocation->height;
      if (shortage > 0)
        {
          /* Shrink borders */
          border->top -= shortage / 2;
          border->bottom -= shortage / 2 + shortage % 2;
        }
    }
}

static void
ctk_range_allocate (CtkCssGadget        *gadget,
                    const CtkAllocation *allocation,
                    int                  baseline,
                    CtkAllocation       *out_clip,
                    gpointer             data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;
  CtkBorder border = { 0 };
  CtkAllocation box_alloc;
  int box_min_width, box_min_height;

  if (CTK_RANGE_GET_CLASS (range)->get_range_border)
    CTK_RANGE_GET_CLASS (range)->get_range_border (range, &border);

  measure_one_gadget (priv->contents_gadget, &box_min_width, &box_min_height);

  if (priv->orientation == CTK_ORIENTATION_VERTICAL)
    clamp_dimensions (allocation, &box_min_width, &box_min_height, &border, TRUE);
  else
    clamp_dimensions (allocation, &box_min_width, &box_min_height, &border, FALSE);

  box_alloc.x = border.left + allocation->x;
  box_alloc.y = border.top + allocation->y;
  box_alloc.width = box_min_width;
  box_alloc.height = box_min_height;

  ctk_css_gadget_allocate (priv->contents_gadget,
                           &box_alloc,
                           baseline,
                           out_clip);

  /* TODO: we should compute a proper clip from get_range_border(),
   * but this will at least give us outset shadows.
   */
  cdk_rectangle_union (out_clip, allocation, out_clip);
}

static void
ctk_range_size_allocate (CtkWidget     *widget,
                         CtkAllocation *allocation)
{
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  if (ctk_widget_get_realized (widget))
    cdk_window_move_resize (priv->event_window,
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);

  ctk_css_gadget_allocate (priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);
  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_range_realize (CtkWidget *widget)
{
  CtkAllocation allocation;
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;
  CdkWindow *window;
  CdkWindowAttr attributes;
  gint attributes_mask;

  ctk_widget_set_realized (widget, TRUE);

  window = ctk_widget_get_parent_window (widget);
  ctk_widget_set_window (widget, window);
  g_object_ref (window);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = CDK_INPUT_ONLY;
  attributes.event_mask = ctk_widget_get_events (widget);
  attributes.event_mask |= CDK_BUTTON_PRESS_MASK |
                           CDK_BUTTON_RELEASE_MASK |
                           CDK_SCROLL_MASK |
                           CDK_SMOOTH_SCROLL_MASK |
                           CDK_ENTER_NOTIFY_MASK |
                           CDK_LEAVE_NOTIFY_MASK |
                           CDK_POINTER_MOTION_MASK;

  attributes_mask = CDK_WA_X | CDK_WA_Y;

  priv->event_window = cdk_window_new (ctk_widget_get_parent_window (widget),
					&attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->event_window);
}

static void
ctk_range_unrealize (CtkWidget *widget)
{
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;

  ctk_range_remove_step_timer (range);

  ctk_widget_unregister_window (widget, priv->event_window);
  cdk_window_destroy (priv->event_window);
  priv->event_window = NULL;

  CTK_WIDGET_CLASS (ctk_range_parent_class)->unrealize (widget);
}

static void
ctk_range_map (CtkWidget *widget)
{
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;

  cdk_window_show (priv->event_window);

  CTK_WIDGET_CLASS (ctk_range_parent_class)->map (widget);
}

static void
ctk_range_unmap (CtkWidget *widget)
{
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;

  stop_scrolling (range);

  cdk_window_hide (priv->event_window);

  CTK_WIDGET_CLASS (ctk_range_parent_class)->unmap (widget);
}

static void
update_slider_state (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;
  CtkStateFlags state;

  state = ctk_widget_get_state_flags (CTK_WIDGET (range));

  state &= ~(CTK_STATE_FLAG_PRELIGHT | CTK_STATE_FLAG_ACTIVE);

  if (priv->mouse_location == priv->slider_gadget &&
      !(state & CTK_STATE_FLAG_INSENSITIVE))
    state |= CTK_STATE_FLAG_PRELIGHT;

  if (priv->grab_location == priv->slider_gadget)
    state |= CTK_STATE_FLAG_ACTIVE;

  ctk_css_gadget_set_state (priv->slider_gadget, state);
}

static void
update_trough_state (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;
  CtkStateFlags state;

  state = ctk_widget_get_state_flags (CTK_WIDGET (range));

  state &= ~(CTK_STATE_FLAG_PRELIGHT | CTK_STATE_FLAG_ACTIVE);

  ctk_css_gadget_set_state (priv->contents_gadget, state);

  if (priv->mouse_location == priv->trough_gadget &&
      !(state & CTK_STATE_FLAG_INSENSITIVE))
    state |= CTK_STATE_FLAG_PRELIGHT;

  if (priv->grab_location == priv->trough_gadget)
    state |= CTK_STATE_FLAG_ACTIVE;

  ctk_css_gadget_set_state (priv->trough_gadget, state);
  if (priv->highlight_gadget)
    ctk_css_gadget_set_state (priv->highlight_gadget, state);
  if (priv->fill_gadget)
    ctk_css_gadget_set_state (priv->fill_gadget, state);
}

static void
ctk_range_direction_changed (CtkWidget        *widget,
                             CtkTextDirection  previous_direction)
{
  CtkRange *range = CTK_RANGE (widget);

  update_fill_position (range);
  update_highlight_position (range);

  CTK_WIDGET_CLASS (ctk_range_parent_class)->direction_changed (widget, previous_direction);
}

static void
ctk_range_state_flags_changed (CtkWidget     *widget,
                               CtkStateFlags  previous_state)
{
  CtkRange *range = CTK_RANGE (widget);

  update_trough_state (range);
  update_slider_state (range);
  update_steppers_state (range);

  CTK_WIDGET_CLASS (ctk_range_parent_class)->state_flags_changed (widget, previous_state);
}

static gboolean
ctk_range_render_trough (CtkCssGadget *gadget,
                         cairo_t      *cr,
                         int           x,
                         int           y,
                         int           width,
                         int           height,
                         gpointer      user_data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;

  /* HACK: CtkColorScale wants to draw its own trough
   * so we let it...
   */
  if (CTK_IS_COLOR_SCALE (widget))
    ctk_color_scale_draw_trough (CTK_COLOR_SCALE (widget), cr, x, y, width, height);

  if (priv->show_fill_level &&
      ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_page_size (priv->adjustment) -
      ctk_adjustment_get_lower (priv->adjustment) != 0)
    ctk_css_gadget_draw (priv->fill_gadget, cr);

  if (priv->has_origin)
    ctk_css_gadget_draw (priv->highlight_gadget, cr);

  return ctk_widget_has_visible_focus (widget);
}

static gboolean
ctk_range_render (CtkCssGadget *gadget,
                  cairo_t      *cr,
                  int           x,
                  int           y,
                  int           width,
                  int           height,
                  gpointer      user_data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;

  ctk_css_gadget_draw (priv->contents_gadget, cr);

  /* Draw the slider last, so that e.g. the focus ring stays below it */
  ctk_css_gadget_draw (priv->slider_gadget, cr);

  return FALSE;
}

static gboolean
ctk_range_draw (CtkWidget *widget,
                cairo_t   *cr)
{
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;

  ctk_css_gadget_draw (priv->gadget, cr);

  return CDK_EVENT_PROPAGATE;
}

static void
range_grab_add (CtkRange      *range,
                CtkCssGadget  *location)
{
  CtkRangePrivate *priv = range->priv;
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (CTK_WIDGET (range));

  /* Don't perform any CDK/CTK+ grab here. Since a button
   * is down, there's an ongoing implicit grab on
   * priv->event_window, which pretty much guarantees this
   * is the only widget receiving the pointer events.
   */
  priv->grab_location = location;
  ctk_css_gadget_queue_allocate (location);

  update_trough_state (range);
  update_slider_state (range);
  update_steppers_state (range);

  ctk_style_context_add_class (context, "dragging");

  ctk_grab_add (CTK_WIDGET (range));
}

static void
update_zoom_state (CtkRange *range,
                   gboolean  enabled)
{
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (CTK_WIDGET (range));

  if (enabled)
    ctk_style_context_add_class (context, "fine-tune");
  else
    ctk_style_context_remove_class (context, "fine-tune");

  range->priv->zoom = enabled;
}

static void
range_grab_remove (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;
  CtkStyleContext *context;

  if (!priv->grab_location)
    return;

  ctk_grab_remove (CTK_WIDGET (range));
  context = ctk_widget_get_style_context (CTK_WIDGET (range));

  ctk_css_gadget_queue_allocate (priv->grab_location);
  priv->grab_location = NULL;

  ctk_range_update_mouse_location (range);

  update_slider_state (range);
  update_steppers_state (range);
  update_zoom_state (range, FALSE);

  ctk_style_context_remove_class (context, "dragging");
}

static CtkScrollType
range_get_scroll_for_grab (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;
  guint grab_button;
  gboolean invert;

  invert = should_invert (range);
  grab_button = ctk_gesture_single_get_current_button (CTK_GESTURE_SINGLE (range->priv->multipress_gesture));

  if (!priv->grab_location)
    return CTK_SCROLL_NONE;

  /* Backward stepper */
  if (priv->grab_location == priv->stepper_a_gadget ||
      priv->grab_location == priv->stepper_c_gadget)
    {
      switch (grab_button)
        {
        case CDK_BUTTON_PRIMARY:
          return invert ? CTK_SCROLL_STEP_FORWARD : CTK_SCROLL_STEP_BACKWARD;
          break;
        case CDK_BUTTON_SECONDARY:
          return invert ? CTK_SCROLL_PAGE_FORWARD : CTK_SCROLL_PAGE_BACKWARD;
          break;
        case CDK_BUTTON_MIDDLE:
          return invert ? CTK_SCROLL_END : CTK_SCROLL_START;
          break;
        default:
          return CTK_SCROLL_NONE;
        }
    }

  /* Forward stepper */
  if (priv->grab_location == priv->stepper_b_gadget ||
      priv->grab_location == priv->stepper_d_gadget)
    {
      switch (grab_button)
        {
        case CDK_BUTTON_PRIMARY:
          return invert ? CTK_SCROLL_STEP_BACKWARD : CTK_SCROLL_STEP_FORWARD;
          break;
        case CDK_BUTTON_SECONDARY:
          return invert ? CTK_SCROLL_PAGE_BACKWARD : CTK_SCROLL_PAGE_FORWARD;
          break;
        case CDK_BUTTON_MIDDLE:
          return invert ? CTK_SCROLL_START : CTK_SCROLL_END;
          break;
        default:
          return CTK_SCROLL_NONE;
        }
    }

  /* In the trough */
  if (priv->grab_location == priv->trough_gadget)
    {
      if (priv->trough_click_forward)
        return CTK_SCROLL_PAGE_FORWARD;
      else
        return CTK_SCROLL_PAGE_BACKWARD;
    }

  return CTK_SCROLL_NONE;
}

static gdouble
coord_to_value (CtkRange *range,
                gdouble   coord)
{
  CtkRangePrivate *priv = range->priv;
  gdouble frac;
  gdouble value;
  gint    trough_length;
  gint    trough_start;
  gint    slider_length;
  CtkAllocation slider_alloc, trough_alloc;

  ctk_css_gadget_get_margin_box (priv->slider_gadget, &slider_alloc);
  ctk_css_gadget_get_content_box (priv->trough_gadget, &trough_alloc);

  if (priv->orientation == CTK_ORIENTATION_VERTICAL)
    {
      trough_length = trough_alloc.height;
      trough_start  = trough_alloc.y;
      slider_length = slider_alloc.height;
    }
  else
    {
      trough_length = trough_alloc.width;
      trough_start  = trough_alloc.x;
      slider_length = slider_alloc.width;
    }

  if (trough_length == slider_length)
    frac = 1.0;
  else
    frac = (MAX (0, coord - trough_start) /
            (gdouble) (trough_length - slider_length));

  if (should_invert (range))
    frac = 1.0 - frac;

  value = ctk_adjustment_get_lower (priv->adjustment) + frac * (ctk_adjustment_get_upper (priv->adjustment) -
                                            ctk_adjustment_get_lower (priv->adjustment) -
                                            ctk_adjustment_get_page_size (priv->adjustment));
  return value;
}

static gboolean
ctk_range_key_press (CtkWidget   *widget,
		     CdkEventKey *event)
{
  CdkDevice *device;
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;

  device = cdk_event_get_device ((CdkEvent *) event);
  device = cdk_device_get_associated_device (device);

  if (ctk_gesture_is_active (priv->drag_gesture) &&
      device == ctk_gesture_get_device (priv->drag_gesture) &&
      event->keyval == CDK_KEY_Escape &&
      priv->grab_location != NULL)
    {
      stop_scrolling (range);

      return CDK_EVENT_STOP;
    }
  else if (priv->in_drag &&
           (event->keyval == CDK_KEY_Shift_L ||
            event->keyval == CDK_KEY_Shift_R))
    {
      CtkAllocation slider_alloc;

      ctk_css_gadget_get_margin_box (priv->slider_gadget, &slider_alloc);

      if (priv->orientation == CTK_ORIENTATION_VERTICAL)
        priv->slide_initial_slider_position = slider_alloc.y;
      else
        priv->slide_initial_slider_position = slider_alloc.x;
      update_zoom_state (range, !priv->zoom);

      return CDK_EVENT_STOP;
    }

  return CTK_WIDGET_CLASS (ctk_range_parent_class)->key_press_event (widget, event);
}

static void
update_initial_slider_position (CtkRange      *range,
                                gdouble        x,
                                gdouble        y,
                                CtkAllocation *slider_alloc)
{
  CtkRangePrivate *priv = range->priv;

  if (priv->orientation == CTK_ORIENTATION_VERTICAL)
    {
      priv->slide_initial_slider_position = MAX (0, slider_alloc->y);
      priv->slide_initial_coordinate_delta = y - priv->slide_initial_slider_position;
    }
  else
    {
      priv->slide_initial_slider_position = MAX (0, slider_alloc->x);
      priv->slide_initial_coordinate_delta = x - priv->slide_initial_slider_position;
    }
}

static void
ctk_range_long_press_gesture_pressed (CtkGestureLongPress *gesture,
                                      gdouble              x,
                                      gdouble              y,
                                      CtkRange            *range)
{
  CtkRangePrivate *priv = range->priv;

  ctk_range_update_mouse_location (range);

  if (priv->mouse_location == priv->slider_gadget && !priv->zoom)
    {
      CtkAllocation slider_alloc;

      ctk_css_gadget_get_margin_box (priv->slider_gadget, &slider_alloc);
      update_initial_slider_position (range, x, y, &slider_alloc);
      update_zoom_state (range, TRUE);
    }
}

static void
ctk_range_multipress_gesture_pressed (CtkGestureMultiPress *gesture,
                                      guint                 n_press,
                                      gdouble               x,
                                      gdouble               y,
                                      CtkRange             *range)
{
  CtkWidget *widget = CTK_WIDGET (range);
  CtkRangePrivate *priv = range->priv;
  CdkDevice *source_device;
  CdkEventSequence *sequence;
  const CdkEvent *event;
  CdkInputSource source;
  gboolean primary_warps;
  gboolean shift_pressed;
  guint button;
  CdkModifierType state_mask;
  CtkAllocation slider_alloc;

  if (!ctk_widget_has_focus (widget))
    ctk_widget_grab_focus (widget);

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  button = ctk_gesture_single_get_current_button (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);
  cdk_event_get_state (event, &state_mask);
  shift_pressed = (state_mask & CDK_SHIFT_MASK) != 0;

  source_device = cdk_event_get_source_device ((CdkEvent *) event);
  source = cdk_device_get_source (source_device);

  priv->mouse_x = x;
  priv->mouse_y = y;

  ctk_range_update_mouse_location (range);
  ctk_css_gadget_get_margin_box (priv->slider_gadget, &slider_alloc);

  g_object_get (ctk_widget_get_settings (widget),
                "ctk-primary-button-warps-slider", &primary_warps,
                NULL);

  if (priv->mouse_location == priv->slider_gadget &&
      cdk_event_triggers_context_menu (event))
    {
      gboolean handled;

      ctk_gesture_set_state (priv->multipress_gesture, CTK_EVENT_SEQUENCE_CLAIMED);
      g_signal_emit_by_name (widget, "popup-menu", &handled);
      return;
    }

  if (priv->mouse_location == priv->slider_gadget)
    {
      /* Shift-click in the slider = fine adjustment */
      if (shift_pressed)
        update_zoom_state (range, TRUE);

      update_initial_slider_position (range, x, y, &slider_alloc);
      range_grab_add (range, priv->slider_gadget);

      ctk_widget_queue_draw (widget);
    }
  else if (priv->mouse_location == priv->stepper_a_gadget ||
           priv->mouse_location == priv->stepper_b_gadget ||
           priv->mouse_location == priv->stepper_c_gadget ||
           priv->mouse_location == priv->stepper_d_gadget)
    {
      CtkScrollType scroll;

      range_grab_add (range, priv->mouse_location);

      scroll = range_get_scroll_for_grab (range);
      if (scroll == CTK_SCROLL_START || scroll == CTK_SCROLL_END)
        ctk_range_scroll (range, scroll);
      else if (scroll != CTK_SCROLL_NONE)
        {
          remove_autoscroll (range);
          range->priv->autoscroll_mode = scroll;
          add_autoscroll (range);
        }
    }
  else if (priv->mouse_location == priv->trough_gadget &&
           (source == CDK_SOURCE_TOUCHSCREEN ||
            (primary_warps && !shift_pressed && button == CDK_BUTTON_PRIMARY) ||
            (!primary_warps && shift_pressed && button == CDK_BUTTON_PRIMARY) ||
            (!primary_warps && button == CDK_BUTTON_MIDDLE)))
    {
      /* warp to location */
      CdkRectangle slider;
      gdouble slider_low_value, slider_high_value, new_value;

      slider_high_value =
        coord_to_value (range,
                        priv->orientation == CTK_ORIENTATION_VERTICAL ?
                        y : x);
      slider_low_value =
        coord_to_value (range,
                        priv->orientation == CTK_ORIENTATION_VERTICAL ?
                        y - slider_alloc.height :
                        x - slider_alloc.width);

      /* compute new value for warped slider */
      new_value = (slider_low_value + slider_high_value) / 2;

      ctk_range_compute_slider_position (range, new_value, &slider);
      update_initial_slider_position (range, x, y, &slider);

      range_grab_add (range, priv->slider_gadget);

      ctk_widget_queue_draw (widget);

      update_slider_position (range, x, y);
    }
  else if (priv->mouse_location == priv->trough_gadget &&
           ((primary_warps && shift_pressed && button == CDK_BUTTON_PRIMARY) ||
            (!primary_warps && !shift_pressed && button == CDK_BUTTON_PRIMARY) ||
            (primary_warps && button == CDK_BUTTON_MIDDLE)))
    {
      /* jump by pages */
      CtkScrollType scroll;
      gdouble click_value;

      click_value = coord_to_value (range,
                                    priv->orientation == CTK_ORIENTATION_VERTICAL ?
                                    y : x);

      priv->trough_click_forward = click_value > ctk_adjustment_get_value (priv->adjustment);
      range_grab_add (range, priv->trough_gadget);

      scroll = range_get_scroll_for_grab (range);
      ctk_range_add_step_timer (range, scroll);
    }
  else if (priv->mouse_location == priv->trough_gadget &&
           button == CDK_BUTTON_SECONDARY)
    {
      /* autoscroll */
      gdouble click_value;

      click_value = coord_to_value (range,
                                    priv->orientation == CTK_ORIENTATION_VERTICAL ?
                                    y : x);

      priv->trough_click_forward = click_value > ctk_adjustment_get_value (priv->adjustment);
      range_grab_add (range, priv->trough_gadget);

      remove_autoscroll (range);
      range->priv->autoscroll_mode = priv->trough_click_forward ? CTK_SCROLL_END : CTK_SCROLL_START;
      add_autoscroll (range);
    }

  if (priv->grab_location == priv->slider_gadget);
    /* leave it to ::drag-begin to claim the sequence */
  else if (priv->grab_location != NULL)
    ctk_gesture_set_state (priv->multipress_gesture, CTK_EVENT_SEQUENCE_CLAIMED);
}

static void
ctk_range_multipress_gesture_released (CtkGestureMultiPress *gesture,
                                       guint                 n_press,
                                       gdouble               x,
                                       gdouble               y,
                                       CtkRange             *range)
{
  CtkRangePrivate *priv = range->priv;

  priv->mouse_x = x;
  priv->mouse_y = y;
  range->priv->in_drag = FALSE;
  stop_scrolling (range);
}

/* During a slide, move the slider as required given new mouse position */
static void
update_slider_position (CtkRange *range,
                        gint      mouse_x,
                        gint      mouse_y)
{
  CtkRangePrivate *priv = range->priv;
  gdouble delta;
  gdouble c;
  gdouble new_value;
  gboolean handled;
  gdouble next_value;
  gdouble mark_value;
  gdouble mark_delta;
  gdouble zoom;
  gint i;

  if (priv->zoom)
    {
      CtkAllocation trough_alloc;

      ctk_css_gadget_get_margin_box (priv->trough_gadget, &trough_alloc);

      zoom = MIN(1.0, (priv->orientation == CTK_ORIENTATION_VERTICAL ?
                       trough_alloc.height : trough_alloc.width) /
                       (ctk_adjustment_get_upper (priv->adjustment) -
                        ctk_adjustment_get_lower (priv->adjustment) -
                        ctk_adjustment_get_page_size (priv->adjustment)));
      /* the above is ineffective for scales, so just set a zoom factor */
      if (zoom == 1.0)
        zoom = 0.25;
    }
  else
    zoom = 1.0;

  /* recalculate the initial position from the current position */
  if (priv->slide_initial_slider_position == -1)
    {
      CtkAllocation slider_alloc;

      ctk_css_gadget_get_margin_box (priv->slider_gadget, &slider_alloc);

      if (priv->orientation == CTK_ORIENTATION_VERTICAL)
        priv->slide_initial_slider_position = (zoom * (mouse_y - priv->slide_initial_coordinate_delta) - slider_alloc.y) / (zoom - 1.0);
      else
        priv->slide_initial_slider_position = (zoom * (mouse_x - priv->slide_initial_coordinate_delta) - slider_alloc.x) / (zoom - 1.0);
    }

  if (priv->orientation == CTK_ORIENTATION_VERTICAL)
    delta = mouse_y - (priv->slide_initial_coordinate_delta + priv->slide_initial_slider_position);
  else
    delta = mouse_x - (priv->slide_initial_coordinate_delta + priv->slide_initial_slider_position);

  c = priv->slide_initial_slider_position + zoom * delta;

  new_value = coord_to_value (range, c);
  next_value = coord_to_value (range, c + 1);
  mark_delta = fabs (next_value - new_value);

  for (i = 0; i < priv->n_marks; i++)
    {
      mark_value = priv->marks[i];

      if (fabs (ctk_adjustment_get_value (priv->adjustment) - mark_value) < 3 * mark_delta)
        {
          if (fabs (new_value - mark_value) < MARK_SNAP_LENGTH * mark_delta)
            {
              new_value = mark_value;
              break;
            }
        }
    }

  g_signal_emit (range, signals[CHANGE_VALUE], 0, CTK_SCROLL_JUMP, new_value, &handled);
}

static void
remove_autoscroll (CtkRange *range)
{
  if (range->priv->autoscroll_id)
    {
      ctk_widget_remove_tick_callback (CTK_WIDGET (range),
                                       range->priv->autoscroll_id);
      range->priv->autoscroll_id = 0;
    }

  /* unset initial position so it can be calculated */
  range->priv->slide_initial_slider_position = -1;

  range->priv->autoscroll_mode = CTK_SCROLL_NONE;
}

static gboolean
autoscroll_cb (CtkWidget     *widget,
               CdkFrameClock *frame_clock,
               gpointer       data)
{
  CtkRange *range = CTK_RANGE (data);
  CtkRangePrivate *priv = range->priv;
  CtkAdjustment *adj = priv->adjustment;
  gdouble increment;
  gdouble value;
  gboolean handled;
  gdouble step, page;

  step = ctk_adjustment_get_step_increment (adj);
  page = ctk_adjustment_get_page_increment (adj);

  switch (priv->autoscroll_mode)
    {
    case CTK_SCROLL_STEP_FORWARD:
      increment = step / AUTOSCROLL_FACTOR;
      break;
    case CTK_SCROLL_PAGE_FORWARD:
      increment = page / AUTOSCROLL_FACTOR;
      break;
    case CTK_SCROLL_STEP_BACKWARD:
      increment = - step / AUTOSCROLL_FACTOR;
      break;
    case CTK_SCROLL_PAGE_BACKWARD:
      increment = - page / AUTOSCROLL_FACTOR;
      break;
    case CTK_SCROLL_START:
    case CTK_SCROLL_END:
      {
        gdouble x, y;
        gdouble distance, t;

        /* Vary scrolling speed from slow (ie step) to fast (2 * page),
         * based on the distance of the pointer from the widget. We start
         * speeding up if the pointer moves at least 20 pixels away, and
         * we reach maximum speed when it is 220 pixels away.
         */
        if (!ctk_gesture_drag_get_offset (CTK_GESTURE_DRAG (priv->drag_gesture), &x, &y))
          {
            x = 0.0;
            y = 0.0;
          }
        if (ctk_orientable_get_orientation (CTK_ORIENTABLE (range)) == CTK_ORIENTATION_HORIZONTAL)
          distance = fabs (y);
        else
          distance = fabs (x);
        distance = CLAMP (distance - 20, 0.0, 200);
        t = distance / 100.0;
        step = (1 - t) * step + t * page;
        if (priv->autoscroll_mode == CTK_SCROLL_END)
          increment = step / AUTOSCROLL_FACTOR;
        else
          increment = - step / AUTOSCROLL_FACTOR;
      }
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  value = ctk_adjustment_get_value (adj);
  value += increment;

  g_signal_emit (range, signals[CHANGE_VALUE], 0, CTK_SCROLL_JUMP, value, &handled);

  return G_SOURCE_CONTINUE;
}

static void
add_autoscroll (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;

  if (priv->autoscroll_id != 0 ||
      priv->autoscroll_mode == CTK_SCROLL_NONE)
    return;

  priv->autoscroll_id = ctk_widget_add_tick_callback (CTK_WIDGET (range),
                                                      autoscroll_cb, range, NULL);
}

static void
stop_scrolling (CtkRange *range)
{
  range_grab_remove (range);
  ctk_range_remove_step_timer (range);
  remove_autoscroll (range);
}

/**
 * _ctk_range_get_wheel_delta:
 * @range: a #CtkRange
 * @event: A #CdkEventScroll
 *
 * Returns a good step value for the mouse wheel.
 *
 * Returns: A good step value for the mouse wheel.
 *
 * Since: 2.4
 **/
gdouble
_ctk_range_get_wheel_delta (CtkRange       *range,
                            CdkEventScroll *event)
{
  CtkRangePrivate *priv = range->priv;
  CtkAdjustment *adjustment = priv->adjustment;
  gdouble dx, dy;
  gdouble delta = 0;
  gdouble page_size;
  gdouble page_increment;
  gdouble scroll_unit;
  CdkScrollDirection direction;
  CtkOrientation move_orientation;

  page_size = ctk_adjustment_get_page_size (adjustment);
  page_increment = ctk_adjustment_get_page_increment (adjustment);

  if (CTK_IS_SCROLLBAR (range))
    {
      gdouble pow_unit = pow (page_size, 2.0 / 3.0);

      /* for very small page sizes of < 1.0, the effect of pow() is
       * the opposite of what's intended and the scroll steps become
       * unusably large, make sure we never get a scroll_unit larger
       * than page_size / 2.0, which used to be the default before the
       * pow() magic was introduced.
       */
      scroll_unit = MIN (pow_unit, page_size / 2.0);
    }
  else
    scroll_unit = page_increment;

  if (cdk_event_get_scroll_deltas ((CdkEvent *) event, &dx, &dy))
    {
#ifdef CDK_WINDOWING_QUARTZ
      scroll_unit = 1;
#endif

      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL && dx != 0)
        {
          move_orientation = CTK_ORIENTATION_HORIZONTAL;
          delta = dx * scroll_unit;
        }
      else
        {
          move_orientation = CTK_ORIENTATION_VERTICAL;
          delta = dy * scroll_unit;
        }
    }
  else if (cdk_event_get_scroll_direction ((CdkEvent *) event, &direction))
    {
      if (direction == CDK_SCROLL_LEFT || direction == CDK_SCROLL_RIGHT)
        move_orientation = CTK_ORIENTATION_HORIZONTAL;
      else
        move_orientation = CTK_ORIENTATION_VERTICAL;

      if (direction == CDK_SCROLL_LEFT || direction == CDK_SCROLL_UP)
        delta = - scroll_unit;
      else
        delta = scroll_unit;
    }

  if (delta != 0 && should_invert_move (range, move_orientation))
    delta = - delta;

  return delta;
}

static gboolean
ctk_range_scroll_event (CtkWidget      *widget,
			CdkEventScroll *event)
{
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;
  double delta = _ctk_range_get_wheel_delta (range, event);
  gboolean handled;

  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 CTK_SCROLL_JUMP, ctk_adjustment_get_value (priv->adjustment) + delta,
                 &handled);

  return CDK_EVENT_STOP;
}

static void
update_autoscroll_mode (CtkRange *range)
{
  CtkScrollType mode = CTK_SCROLL_NONE;

  if (range->priv->zoom)
    {
      CtkAllocation allocation;
      gint size, pos;

      ctk_widget_get_allocation (CTK_WIDGET (range), &allocation);

      if (range->priv->orientation == CTK_ORIENTATION_VERTICAL)
        {
          size = allocation.height;
          pos = range->priv->mouse_y;
        }
      else
        {
          size = allocation.width;
          pos = range->priv->mouse_x;
        }

      if (pos < SCROLL_EDGE_SIZE)
        mode = range->priv->inverted ? CTK_SCROLL_STEP_FORWARD : CTK_SCROLL_STEP_BACKWARD;
      else if (pos > (size - SCROLL_EDGE_SIZE))
        mode = range->priv->inverted ? CTK_SCROLL_STEP_BACKWARD : CTK_SCROLL_STEP_FORWARD;
    }

  if (mode != range->priv->autoscroll_mode)
    {
      remove_autoscroll (range);
      range->priv->autoscroll_mode = mode;
      add_autoscroll (range);
    }
}

static void
ctk_range_drag_gesture_update (CtkGestureDrag *gesture,
                               gdouble         offset_x,
                               gdouble         offset_y,
                               CtkRange       *range)
{
  CtkRangePrivate *priv = range->priv;
  gdouble start_x, start_y;

  if (priv->grab_location == priv->slider_gadget)
    {
      ctk_gesture_drag_get_start_point (gesture, &start_x, &start_y);
      priv->mouse_x = start_x + offset_x;
      priv->mouse_y = start_y + offset_y;
      priv->in_drag = TRUE;
      update_autoscroll_mode (range);

      if (priv->autoscroll_mode == CTK_SCROLL_NONE)
        update_slider_position (range, priv->mouse_x, priv->mouse_y);
    }
}

static void
ctk_range_drag_gesture_begin (CtkGestureDrag *gesture,
                              gdouble         offset_x,
                              gdouble         offset_y,
                              CtkRange       *range)
{
  CtkRangePrivate *priv = range->priv;

  if (priv->grab_location == priv->slider_gadget)
    ctk_gesture_set_state (priv->drag_gesture, CTK_EVENT_SEQUENCE_CLAIMED);
}

static gboolean
ctk_range_event (CtkWidget *widget,
                 CdkEvent  *event)
{
  CtkRange *range = CTK_RANGE (widget);
  CtkRangePrivate *priv = range->priv;
  gdouble x, y;

  if (event->type == CDK_LEAVE_NOTIFY)
    {
      priv->mouse_x = G_MININT;
      priv->mouse_y = G_MININT;
    }
  else if (cdk_event_get_coords (event, &x, &y))
    {
      priv->mouse_x = x;
      priv->mouse_y = y;
    }

  ctk_range_update_mouse_location (range);

  return CDK_EVENT_PROPAGATE;
}

static void
ctk_range_adjustment_changed (CtkAdjustment *adjustment,
			      gpointer       data)
{
  CtkRange *range = CTK_RANGE (data);

  ctk_range_calc_slider (range);
  ctk_range_calc_stepper_sensitivity (range);

  /* Note that we don't round off to priv->round_digits here.
   * that's because it's really broken to change a value
   * in response to a change signal on that value; round_digits
   * is therefore defined to be a filter on what the CtkRange
   * can input into the adjustment, not a filter that the CtkRange
   * will enforce on the adjustment.
   */
}

static void
ctk_range_adjustment_value_changed (CtkAdjustment *adjustment,
				    gpointer       data)
{
  CtkRange *range = CTK_RANGE (data);

  ctk_range_calc_slider (range);
  ctk_range_calc_stepper_sensitivity (range);
  
  /* now check whether the layout changed  */
  if (CTK_IS_SCALE (range) && ctk_scale_get_draw_value (CTK_SCALE (range)))
    {
      ctk_widget_queue_draw (CTK_WIDGET (range));
    }

  /* Note that we don't round off to priv->round_digits here.
   * that's because it's really broken to change a value
   * in response to a change signal on that value; round_digits
   * is therefore defined to be a filter on what the CtkRange
   * can input into the adjustment, not a filter that the CtkRange
   * will enforce on the adjustment.
   */

  g_signal_emit (range, signals[VALUE_CHANGED], 0);
}

static void
apply_marks (CtkRange *range, 
             gdouble   oldval,
             gdouble  *newval)
{
  CtkRangePrivate *priv = range->priv;
  gint i;
  gdouble mark;

  for (i = 0; i < priv->n_marks; i++)
    {
      mark = priv->marks[i];
      if ((oldval < mark && mark < *newval) ||
          (oldval > mark && mark > *newval))
        {
          *newval = mark;
          return;
        }
    }
}

static void
step_back (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;
  gdouble newval;
  gboolean handled;

  newval = ctk_adjustment_get_value (priv->adjustment) - ctk_adjustment_get_step_increment (priv->adjustment);
  apply_marks (range, ctk_adjustment_get_value (priv->adjustment), &newval);
  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 CTK_SCROLL_STEP_BACKWARD, newval, &handled);
}

static void
step_forward (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;
  gdouble newval;
  gboolean handled;

  newval = ctk_adjustment_get_value (priv->adjustment) + ctk_adjustment_get_step_increment (priv->adjustment);
  apply_marks (range, ctk_adjustment_get_value (priv->adjustment), &newval);
  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 CTK_SCROLL_STEP_FORWARD, newval, &handled);
}


static void
page_back (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;
  gdouble newval;
  gboolean handled;

  newval = ctk_adjustment_get_value (priv->adjustment) - ctk_adjustment_get_page_increment (priv->adjustment);
  apply_marks (range, ctk_adjustment_get_value (priv->adjustment), &newval);
  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 CTK_SCROLL_PAGE_BACKWARD, newval, &handled);
}

static void
page_forward (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;
  gdouble newval;
  gboolean handled;

  newval = ctk_adjustment_get_value (priv->adjustment) + ctk_adjustment_get_page_increment (priv->adjustment);
  apply_marks (range, ctk_adjustment_get_value (priv->adjustment), &newval);
  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 CTK_SCROLL_PAGE_FORWARD, newval, &handled);
}

static void
scroll_begin (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;
  gboolean handled;

  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 CTK_SCROLL_START, ctk_adjustment_get_lower (priv->adjustment),
                 &handled);
}

static void
scroll_end (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;
  gdouble newval;
  gboolean handled;

  newval = ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_page_size (priv->adjustment);
  g_signal_emit (range, signals[CHANGE_VALUE], 0, CTK_SCROLL_END, newval,
                 &handled);
}

static gboolean
ctk_range_scroll (CtkRange     *range,
                  CtkScrollType scroll)
{
  CtkRangePrivate *priv = range->priv;
  gdouble old_value = ctk_adjustment_get_value (priv->adjustment);

  switch (scroll)
    {
    case CTK_SCROLL_STEP_LEFT:
      if (should_invert (range))
        step_forward (range);
      else
        step_back (range);
      break;
                    
    case CTK_SCROLL_STEP_UP:
      if (should_invert (range))
        step_forward (range);
      else
        step_back (range);
      break;

    case CTK_SCROLL_STEP_RIGHT:
      if (should_invert (range))
        step_back (range);
      else
        step_forward (range);
      break;
                    
    case CTK_SCROLL_STEP_DOWN:
      if (should_invert (range))
        step_back (range);
      else
        step_forward (range);
      break;
                  
    case CTK_SCROLL_STEP_BACKWARD:
      step_back (range);
      break;
                  
    case CTK_SCROLL_STEP_FORWARD:
      step_forward (range);
      break;

    case CTK_SCROLL_PAGE_LEFT:
      if (should_invert (range))
        page_forward (range);
      else
        page_back (range);
      break;
                    
    case CTK_SCROLL_PAGE_UP:
      if (should_invert (range))
        page_forward (range);
      else
        page_back (range);
      break;

    case CTK_SCROLL_PAGE_RIGHT:
      if (should_invert (range))
        page_back (range);
      else
        page_forward (range);
      break;
                    
    case CTK_SCROLL_PAGE_DOWN:
      if (should_invert (range))
        page_back (range);
      else
        page_forward (range);
      break;
                  
    case CTK_SCROLL_PAGE_BACKWARD:
      page_back (range);
      break;
                  
    case CTK_SCROLL_PAGE_FORWARD:
      page_forward (range);
      break;

    case CTK_SCROLL_START:
      scroll_begin (range);
      break;

    case CTK_SCROLL_END:
      scroll_end (range);
      break;

    case CTK_SCROLL_JUMP:
      /* Used by CList, range doesn't use it. */
      break;

    case CTK_SCROLL_NONE:
      break;
    }

  return ctk_adjustment_get_value (priv->adjustment) != old_value;
}

static void
ctk_range_move_slider (CtkRange     *range,
                       CtkScrollType scroll)
{
  if (! ctk_range_scroll (range, scroll))
    ctk_widget_error_bell (CTK_WIDGET (range));
}

static gboolean
rectangle_contains_point (CdkRectangle *rect,
                          gint          x,
                          gint          y)
{
  return (x >= rect->x) && (x < rect->x + rect->width) &&
         (y >= rect->y) && (y < rect->y + rect->height);
}

/* Update mouse location, return TRUE if it changes */
static void
ctk_range_update_mouse_location (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;
  gint x, y;
  CtkCssGadget *old_location;
  CtkWidget *widget = CTK_WIDGET (range);
  CdkRectangle trough_alloc, slider_alloc, slider_trace;

  old_location = priv->mouse_location;

  x = priv->mouse_x;
  y = priv->mouse_y;

  ctk_css_gadget_get_border_box (priv->trough_gadget, &trough_alloc);
  ctk_css_gadget_get_border_box (priv->slider_gadget, &slider_alloc);
  cdk_rectangle_union (&slider_alloc, &trough_alloc, &slider_trace);

  if (priv->grab_location != NULL)
    priv->mouse_location = priv->grab_location;
  else if (priv->stepper_a_gadget &&
           ctk_css_gadget_border_box_contains_point (priv->stepper_a_gadget, x, y))
    priv->mouse_location = priv->stepper_a_gadget;
  else if (priv->stepper_b_gadget &&
           ctk_css_gadget_border_box_contains_point (priv->stepper_b_gadget, x, y))
    priv->mouse_location = priv->stepper_b_gadget;
  else if (priv->stepper_c_gadget &&
           ctk_css_gadget_border_box_contains_point (priv->stepper_c_gadget, x, y))
    priv->mouse_location = priv->stepper_c_gadget;
  else if (priv->stepper_d_gadget &&
           ctk_css_gadget_border_box_contains_point (priv->stepper_d_gadget, x, y))
    priv->mouse_location = priv->stepper_d_gadget;
  else if (ctk_css_gadget_border_box_contains_point (priv->slider_gadget, x, y))
    priv->mouse_location = priv->slider_gadget;
  else if (rectangle_contains_point (&slider_trace, x, y))
    priv->mouse_location = priv->trough_gadget;
  else if (ctk_css_gadget_margin_box_contains_point (priv->gadget, x, y))
    priv->mouse_location = priv->gadget;
  else
    priv->mouse_location = NULL;

  if (old_location != priv->mouse_location)
    {
      if (old_location != NULL)
        ctk_css_gadget_queue_allocate (old_location);

      if (priv->mouse_location != NULL)
        {
          ctk_widget_set_state_flags (widget, CTK_STATE_FLAG_PRELIGHT, FALSE);
          ctk_css_gadget_queue_allocate (priv->mouse_location);
        }
      else
        {
          ctk_widget_unset_state_flags (widget, CTK_STATE_FLAG_PRELIGHT);
        }

      update_trough_state (range);
      update_slider_state (range);
      update_steppers_state (range);
    }
}

static void
ctk_range_compute_slider_position (CtkRange     *range,
                                   gdouble       adjustment_value,
                                   CdkRectangle *slider_rect)
{
  CtkRangePrivate *priv = range->priv;
  CtkAllocation trough_content_alloc;
  int slider_width, slider_height, min_slider_size;

  measure_one_gadget (priv->slider_gadget, &slider_width, &slider_height);
  ctk_css_gadget_get_content_box (priv->trough_gadget, &trough_content_alloc);

  min_slider_size = priv->min_slider_size;

  if (priv->orientation == CTK_ORIENTATION_VERTICAL)
    {
      gint y, bottom, top, height;
        
      /* Slider fits into the trough, with stepper_spacing on either side,
       * and the size/position based on the adjustment or fixed, depending.
       */
      slider_rect->x = trough_content_alloc.x + (int) floor ((trough_content_alloc.width - slider_width) / 2);
      slider_rect->width = slider_width;

      if (priv->slider_use_min_size)
        min_slider_size = slider_height;

      /* Compute slider position/length */
      top = trough_content_alloc.y;
      bottom = top + trough_content_alloc.height;

      /* Scale slider half extends over the trough edge */
      if (CTK_IS_SCALE (range))
        {
          top -= min_slider_size / 2;
          bottom += min_slider_size / 2;
        }

      /* slider height is the fraction (page_size /
       * total_adjustment_range) times the trough height in pixels
       */

      if (ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_lower (priv->adjustment) != 0)
        height = ((bottom - top) * (ctk_adjustment_get_page_size (priv->adjustment) /
                                     (ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_lower (priv->adjustment))));
      else
        height = min_slider_size;

      if (height < min_slider_size ||
          priv->slider_size_fixed)
        height = min_slider_size;

      height = MIN (height, trough_content_alloc.height);
      
      y = top;

      if (ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_lower (priv->adjustment) - ctk_adjustment_get_page_size (priv->adjustment) != 0)
        y += (bottom - top - height) * ((adjustment_value - ctk_adjustment_get_lower (priv->adjustment)) /
                                        (ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_lower (priv->adjustment) - ctk_adjustment_get_page_size (priv->adjustment)));

      y = CLAMP (y, top, bottom);
      
      if (should_invert (range))
        y = bottom - (y - top + height);
      
      slider_rect->y = y;
      slider_rect->height = height;
    }
  else
    {
      gint x, left, right, width;
        
      /* Slider fits into the trough, with stepper_spacing on either side,
       * and the size/position based on the adjustment or fixed, depending.
       */
      slider_rect->y = trough_content_alloc.y + (int) floor ((trough_content_alloc.height - slider_height) / 2);
      slider_rect->height = slider_height;

      if (priv->slider_use_min_size)
        min_slider_size = slider_width;

      /* Compute slider position/length */
      left = trough_content_alloc.x;
      right = left + trough_content_alloc.width;

      /* Scale slider half extends over the trough edge */
      if (CTK_IS_SCALE (range))
        {
          left -= min_slider_size / 2;
          right += min_slider_size / 2;
        }

      /* slider width is the fraction (page_size /
       * total_adjustment_range) times the trough width in pixels
       */

      if (ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_lower (priv->adjustment) != 0)
        width = ((right - left) * (ctk_adjustment_get_page_size (priv->adjustment) /
                                 (ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_lower (priv->adjustment))));
      else
        width = min_slider_size;

      if (width < min_slider_size ||
          priv->slider_size_fixed)
        width = min_slider_size;

      width = MIN (width, trough_content_alloc.width);

      x = left;

      if (ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_lower (priv->adjustment) - ctk_adjustment_get_page_size (priv->adjustment) != 0)
        x += (right - left - width) * ((adjustment_value - ctk_adjustment_get_lower (priv->adjustment)) /
                                       (ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_lower (priv->adjustment) - ctk_adjustment_get_page_size (priv->adjustment)));
      
      x = CLAMP (x, left, right);
      
      if (should_invert (range))
        x = right - (x - left + width);
      
      slider_rect->x = x;
      slider_rect->width = width;
    }
}

static void
ctk_range_calc_slider (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;
  gboolean visible;

  if (CTK_IS_SCALE (range) &&
      ctk_adjustment_get_upper (priv->adjustment) == ctk_adjustment_get_lower (priv->adjustment))
    visible = FALSE;
  else
    visible = TRUE;

  ctk_css_gadget_set_visible (priv->slider_gadget, visible);

  ctk_css_gadget_queue_resize (priv->slider_gadget);

  if (priv->has_origin)
    ctk_css_gadget_queue_allocate (priv->trough_gadget);

  ctk_range_update_mouse_location (range);
}

static void
ctk_range_calc_stepper_sensitivity (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;
  gboolean was_upper_sensitive, was_lower_sensitive;

  was_upper_sensitive = priv->upper_sensitive;
  switch (priv->upper_sensitivity)
    {
    case CTK_SENSITIVITY_AUTO:
      priv->upper_sensitive =
        (ctk_adjustment_get_value (priv->adjustment) <
         (ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_page_size (priv->adjustment)));
      break;

    case CTK_SENSITIVITY_ON:
      priv->upper_sensitive = TRUE;
      break;

    case CTK_SENSITIVITY_OFF:
      priv->upper_sensitive = FALSE;
      break;
    }

  was_lower_sensitive = priv->lower_sensitive;
  switch (priv->lower_sensitivity)
    {
    case CTK_SENSITIVITY_AUTO:
      priv->lower_sensitive =
        (ctk_adjustment_get_value (priv->adjustment) > ctk_adjustment_get_lower (priv->adjustment));
      break;

    case CTK_SENSITIVITY_ON:
      priv->lower_sensitive = TRUE;
      break;

    case CTK_SENSITIVITY_OFF:
      priv->lower_sensitive = FALSE;
      break;
    }

  /* Too many side effects can influence which stepper reacts to wat condition.
   * So we just invalidate them all.
   */
  if (was_upper_sensitive != priv->upper_sensitive ||
      was_lower_sensitive != priv->lower_sensitive)
    {
      update_steppers_state (range);

      if (priv->stepper_a_gadget)
        ctk_css_gadget_queue_allocate (priv->stepper_a_gadget);
      if (priv->stepper_b_gadget)
        ctk_css_gadget_queue_allocate (priv->stepper_b_gadget);
      if (priv->stepper_c_gadget)
        ctk_css_gadget_queue_allocate (priv->stepper_c_gadget);
      if (priv->stepper_d_gadget)
        ctk_css_gadget_queue_allocate (priv->stepper_d_gadget);
    }
}

static void
ctk_range_calc_marks (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;
  CdkRectangle slider;
  gint i;

  for (i = 0; i < priv->n_marks; i++)
    {
      ctk_range_compute_slider_position (range, priv->marks[i], &slider);

      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        priv->mark_pos[i] = slider.x + slider.width / 2;
      else
        priv->mark_pos[i] = slider.y + slider.height / 2;
    }
}

static gboolean
ctk_range_real_change_value (CtkRange      *range,
                             CtkScrollType  scroll,
                             gdouble        value)
{
  CtkRangePrivate *priv = range->priv;

  /* potentially adjust the bounds _before_ we clamp */
  g_signal_emit (range, signals[ADJUST_BOUNDS], 0, value);

  if (priv->restrict_to_fill_level)
    value = MIN (value, MAX (ctk_adjustment_get_lower (priv->adjustment),
                             priv->fill_level));

  value = CLAMP (value, ctk_adjustment_get_lower (priv->adjustment),
                 (ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_page_size (priv->adjustment)));

  if (priv->round_digits >= 0)
    {
      gdouble power;
      gint i;

      i = priv->round_digits;
      power = 1;
      while (i--)
        power *= 10;

      value = floor ((value * power) + 0.5) / power;
    }

  if (priv->in_drag || priv->autoscroll_id)
    ctk_adjustment_set_value (priv->adjustment, value);
  else
    ctk_adjustment_animate_to_value (priv->adjustment, value);

  return FALSE;
}

struct _CtkRangeStepTimer
{
  guint timeout_id;
  CtkScrollType step;
};

static gboolean
second_timeout (gpointer data)
{
  CtkRange *range = CTK_RANGE (data);
  CtkRangePrivate *priv = range->priv;

  ctk_range_scroll (range, priv->timer->step);

  return G_SOURCE_CONTINUE;
}

static gboolean
initial_timeout (gpointer data)
{
  CtkRange *range = CTK_RANGE (data);
  CtkRangePrivate *priv = range->priv;

  priv->timer->timeout_id = cdk_threads_add_timeout (TIMEOUT_REPEAT,
                                                     second_timeout,
                                                     range);
  g_source_set_name_by_id (priv->timer->timeout_id, "[ctk+] second_timeout");
  return G_SOURCE_REMOVE;
}

static void
ctk_range_add_step_timer (CtkRange      *range,
                          CtkScrollType  step)
{
  CtkRangePrivate *priv = range->priv;

  g_return_if_fail (priv->timer == NULL);
  g_return_if_fail (step != CTK_SCROLL_NONE);

  priv->timer = g_new (CtkRangeStepTimer, 1);

  priv->timer->timeout_id = cdk_threads_add_timeout (TIMEOUT_INITIAL,
                                                     initial_timeout,
                                                     range);
  g_source_set_name_by_id (priv->timer->timeout_id, "[ctk+] initial_timeout");
  priv->timer->step = step;

  ctk_range_scroll (range, priv->timer->step);
}

static void
ctk_range_remove_step_timer (CtkRange *range)
{
  CtkRangePrivate *priv = range->priv;

  if (priv->timer)
    {
      if (priv->timer->timeout_id != 0)
        g_source_remove (priv->timer->timeout_id);

      g_free (priv->timer);

      priv->timer = NULL;
    }
}

void
_ctk_range_set_has_origin (CtkRange *range,
                           gboolean  has_origin)
{
  CtkRangePrivate *priv = range->priv;

  range->priv->has_origin = has_origin;

  if (has_origin)
    {
      priv->highlight_gadget = ctk_css_custom_gadget_new ("highlight",
                                                          CTK_WIDGET (range),
                                                          priv->trough_gadget, NULL,
                                                          NULL, NULL, NULL,
                                                          NULL, NULL);
      ctk_css_gadget_set_state (priv->highlight_gadget,
                                ctk_css_node_get_state (ctk_css_gadget_get_node (priv->trough_gadget)));

      update_highlight_position (range);
    }
  else
    {
      ctk_css_node_set_parent (ctk_css_gadget_get_node (priv->highlight_gadget), NULL);
      g_clear_object (&priv->highlight_gadget);
    }
}

gboolean
_ctk_range_get_has_origin (CtkRange *range)
{
  return range->priv->has_origin;
}

void
_ctk_range_set_stop_values (CtkRange *range,
                            gdouble  *values,
                            gint      n_values)
{
  CtkRangePrivate *priv = range->priv;
  gint i;

  g_free (priv->marks);
  priv->marks = g_new (gdouble, n_values);

  g_free (priv->mark_pos);
  priv->mark_pos = g_new (gint, n_values);

  priv->n_marks = n_values;

  for (i = 0; i < n_values; i++) 
    priv->marks[i] = values[i];

  ctk_range_calc_marks (range);
}

gint
_ctk_range_get_stop_positions (CtkRange  *range,
                               gint     **values)
{
  CtkRangePrivate *priv = range->priv;

  ctk_range_calc_marks (range);

  if (values)
    *values = g_memdup2 (priv->mark_pos, priv->n_marks * sizeof (gint));

  return priv->n_marks;
}

/**
 * ctk_range_set_round_digits:
 * @range: a #CtkRange
 * @round_digits: the precision in digits, or -1
 *
 * Sets the number of digits to round the value to when
 * it changes. See #CtkRange::change-value.
 *
 * Since: 2.24
 */
void
ctk_range_set_round_digits (CtkRange *range,
                            gint      round_digits)
{
  g_return_if_fail (CTK_IS_RANGE (range));
  g_return_if_fail (round_digits >= -1);

  if (range->priv->round_digits != round_digits)
    {
      range->priv->round_digits = round_digits;
      g_object_notify_by_pspec (G_OBJECT (range), properties[PROP_ROUND_DIGITS]);
    }
}

/**
 * ctk_range_get_round_digits:
 * @range: a #CtkRange
 *
 * Gets the number of digits to round the value to when
 * it changes. See #CtkRange::change-value.
 *
 * Returns: the number of digits to round to
 *
 * Since: 2.24
 */
gint
ctk_range_get_round_digits (CtkRange *range)
{
  g_return_val_if_fail (CTK_IS_RANGE (range), -1);

  return range->priv->round_digits;
}

static void
sync_stepper_gadget (CtkRange                *range,
                     gboolean                 should_have_stepper,
                     CtkCssGadget           **gadget_ptr,
                     const gchar             *class,
                     CtkCssImageBuiltinType   image_type,
                     CtkCssGadget            *prev_sibling)
{
  CtkWidget *widget;
  CtkCssGadget *gadget;
  CtkCssNode *widget_node;
  gboolean has_stepper;
  CtkRangePrivate *priv = range->priv;

  has_stepper = (*gadget_ptr != NULL);
  if (has_stepper == should_have_stepper)
    return;

  if (!should_have_stepper)
    {
      if (*gadget_ptr != NULL)
        {
          if (*gadget_ptr == priv->grab_location)
            stop_scrolling (range);
          if (*gadget_ptr == priv->mouse_location)
            priv->mouse_location = NULL;
          ctk_css_node_set_parent (ctk_css_gadget_get_node (*gadget_ptr), NULL);
          ctk_box_gadget_remove_gadget (CTK_BOX_GADGET (priv->contents_gadget), *gadget_ptr);
        }
      g_clear_object (gadget_ptr);
      return;
    }

  widget = CTK_WIDGET (range);
  widget_node = ctk_widget_get_css_node (widget);
  gadget = ctk_builtin_icon_new ("button",
                                 widget,
                                 NULL, NULL);
  ctk_builtin_icon_set_image (CTK_BUILTIN_ICON (gadget), image_type);
  ctk_css_gadget_add_class (gadget, class);
  ctk_css_gadget_set_state (gadget, ctk_css_node_get_state (widget_node));

  ctk_box_gadget_insert_gadget_after (CTK_BOX_GADGET (priv->contents_gadget), prev_sibling,
                                      gadget, FALSE, CTK_ALIGN_FILL);
  *gadget_ptr = gadget;
}

void
_ctk_range_set_steppers (CtkRange *range,
                         gboolean  has_a,
                         gboolean  has_b,
                         gboolean  has_c,
                         gboolean  has_d)
{
  CtkRangePrivate *priv = range->priv;

  sync_stepper_gadget (range,
                       has_a, &priv->stepper_a_gadget,
                       "up",
                       priv->orientation == CTK_ORIENTATION_VERTICAL ?
                       CTK_CSS_IMAGE_BUILTIN_ARROW_UP : CTK_CSS_IMAGE_BUILTIN_ARROW_LEFT,
                       NULL);

  sync_stepper_gadget (range,
                       has_b, &priv->stepper_b_gadget,
                       "down",
                       priv->orientation == CTK_ORIENTATION_VERTICAL ?
                       CTK_CSS_IMAGE_BUILTIN_ARROW_DOWN : CTK_CSS_IMAGE_BUILTIN_ARROW_RIGHT,
                       priv->stepper_a_gadget);

  sync_stepper_gadget (range,
                       has_c, &priv->stepper_c_gadget,
                       "up",
                       priv->orientation == CTK_ORIENTATION_VERTICAL ?
                       CTK_CSS_IMAGE_BUILTIN_ARROW_UP : CTK_CSS_IMAGE_BUILTIN_ARROW_LEFT,
                       priv->trough_gadget);

  sync_stepper_gadget (range,
                       has_d, &priv->stepper_d_gadget,
                       "down",
                       priv->orientation == CTK_ORIENTATION_VERTICAL ?
                       CTK_CSS_IMAGE_BUILTIN_ARROW_DOWN : CTK_CSS_IMAGE_BUILTIN_ARROW_RIGHT,
                       priv->stepper_c_gadget ? priv->stepper_c_gadget : priv->trough_gadget);

  ctk_widget_queue_resize (CTK_WIDGET (range));
}

CtkCssGadget *
ctk_range_get_slider_gadget (CtkRange *range)
{
  return range->priv->slider_gadget;
}

CtkCssGadget *
ctk_range_get_gadget (CtkRange *range)
{
  return range->priv->gadget;
}
