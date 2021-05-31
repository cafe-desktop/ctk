/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * CtkSpinButton widget for GTK+
 * Copyright (C) 1998 Lars Hamann and Stefan Jeske
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include "ctkspinbutton.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <locale.h>

#include "ctkadjustment.h"
#include "ctkbindings.h"
#include "ctkboxgadgetprivate.h"
#include "ctkcssgadgetprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkentryprivate.h"
#include "ctkiconhelperprivate.h"
#include "ctkicontheme.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctkorientable.h"
#include "ctkorientableprivate.h"
#include "ctkprivate.h"
#include "ctksettings.h"
#include "ctktypebuiltins.h"
#include "ctkwidgetpath.h"
#include "ctkwidgetprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkcssstylepropertyprivate.h"

#include "a11y/ctkspinbuttonaccessible.h"

#define MIN_SPIN_BUTTON_WIDTH 30
#define MAX_TIMER_CALLS       5
#define EPSILON               1e-10
#define MAX_DIGITS            20
#define TIMEOUT_INITIAL       500
#define TIMEOUT_REPEAT        50

/**
 * SECTION:ctkspinbutton
 * @Title: CtkSpinButton
 * @Short_description: Retrieve an integer or floating-point number from
 *     the user
 * @See_also: #CtkEntry
 *
 * A #CtkSpinButton is an ideal way to allow the user to set the value of
 * some attribute. Rather than having to directly type a number into a
 * #CtkEntry, CtkSpinButton allows the user to click on one of two arrows
 * to increment or decrement the displayed value. A value can still be
 * typed in, with the bonus that it can be checked to ensure it is in a
 * given range.
 *
 * The main properties of a CtkSpinButton are through an adjustment.
 * See the #CtkAdjustment section for more details about an adjustment's
 * properties. Note that CtkSpinButton will by default make its entry
 * large enough to accomodate the lower and upper bounds of the adjustment,
 * which can lead to surprising results. Best practice is to set both
 * the #CtkEntry:width-chars and #CtkEntry:max-width-chars poperties
 * to the desired number of characters to display in the entry.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * spinbutton.horizontal
 * ├── undershoot.left
 * ├── undershoot.right
 * ├── entry
 * │   ╰── ...
 * ├── button.down
 * ╰── button.up
 * ]|
 *
 * |[<!-- language="plain" -->
 * spinbutton.vertical
 * ├── undershoot.left
 * ├── undershoot.right
 * ├── button.up
 * ├── entry
 * │   ╰── ...
 * ╰── button.down
 * ]|
 *
 * CtkSpinButtons main CSS node has the name spinbutton. It creates subnodes
 * for the entry and the two buttons, with these names. The button nodes have
 * the style classes .up and .down. The CtkEntry subnodes (if present) are put
 * below the entry node. The orientation of the spin button is reflected in
 * the .vertical or .horizontal style class on the main node.
 *
 * ## Using a CtkSpinButton to get an integer
 *
 * |[<!-- language="C" -->
 * // Provides a function to retrieve an integer value from a CtkSpinButton
 * // and creates a spin button to model percentage values.
 *
 * gint
 * grab_int_value (CtkSpinButton *button,
 *                 gpointer       user_data)
 * {
 *   return ctk_spin_button_get_value_as_int (button);
 * }
 *
 * void
 * create_integer_spin_button (void)
 * {
 *
 *   CtkWidget *window, *button;
 *   CtkAdjustment *adjustment;
 *
 *   adjustment = ctk_adjustment_new (50.0, 0.0, 100.0, 1.0, 5.0, 0.0);
 *
 *   window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
 *   ctk_container_set_border_width (CTK_CONTAINER (window), 5);
 *
 *   // creates the spinbutton, with no decimal places
 *   button = ctk_spin_button_new (adjustment, 1.0, 0);
 *   ctk_container_add (CTK_CONTAINER (window), button);
 *
 *   ctk_widget_show_all (window);
 * }
 * ]|
 *
 * ## Using a CtkSpinButton to get a floating point value
 *
 * |[<!-- language="C" -->
 * // Provides a function to retrieve a floating point value from a
 * // CtkSpinButton, and creates a high precision spin button.
 *
 * gfloat
 * grab_float_value (CtkSpinButton *button,
 *                   gpointer       user_data)
 * {
 *   return ctk_spin_button_get_value (button);
 * }
 *
 * void
 * create_floating_spin_button (void)
 * {
 *   CtkWidget *window, *button;
 *   CtkAdjustment *adjustment;
 *
 *   adjustment = ctk_adjustment_new (2.500, 0.0, 5.0, 0.001, 0.1, 0.0);
 *
 *   window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
 *   ctk_container_set_border_width (CTK_CONTAINER (window), 5);
 *
 *   // creates the spinbutton, with three decimal places
 *   button = ctk_spin_button_new (adjustment, 0.001, 3);
 *   ctk_container_add (CTK_CONTAINER (window), button);
 *
 *   ctk_widget_show_all (window);
 * }
 * ]|
 */

enum {
  UP_PANEL,
  DOWN_PANEL
};

struct _CtkSpinButtonPrivate
{
  CtkAdjustment *adjustment;

  GdkWindow     *down_panel;
  GdkWindow     *up_panel;

  CtkCssGadget  *gadget;
  CtkCssGadget  *down_button;
  CtkCssGadget  *up_button;

  GdkWindow     *click_child;
  GdkWindow     *in_child;

  guint32        timer;

  CtkSpinButtonUpdatePolicy update_policy;

  gdouble        climb_rate;
  gdouble        timer_step;

  CtkOrientation orientation;

  CtkGesture *swipe_gesture;

  guint          button        : 2;
  guint          digits        : 10;
  guint          need_timer    : 1;
  guint          numeric       : 1;
  guint          snap_to_ticks : 1;
  guint          timer_calls   : 3;
  guint          wrap          : 1;
};

enum {
  PROP_0,
  PROP_ADJUSTMENT,
  PROP_CLIMB_RATE,
  PROP_DIGITS,
  PROP_SNAP_TO_TICKS,
  PROP_NUMERIC,
  PROP_WRAP,
  PROP_UPDATE_POLICY,
  PROP_VALUE,
  PROP_ORIENTATION
};

/* Signals */
enum
{
  INPUT,
  OUTPUT,
  VALUE_CHANGED,
  CHANGE_VALUE,
  WRAPPED,
  LAST_SIGNAL
};

static void ctk_spin_button_editable_init  (CtkEditableInterface *iface);
static void ctk_spin_button_finalize       (GObject            *object);
static void ctk_spin_button_set_property   (GObject         *object,
                                            guint            prop_id,
                                            const GValue    *value,
                                            GParamSpec      *pspec);
static void ctk_spin_button_get_property   (GObject         *object,
                                            guint            prop_id,
                                            GValue          *value,
                                            GParamSpec      *pspec);
static void ctk_spin_button_destroy        (CtkWidget          *widget);
static void ctk_spin_button_map            (CtkWidget          *widget);
static void ctk_spin_button_unmap          (CtkWidget          *widget);
static void ctk_spin_button_realize        (CtkWidget          *widget);
static void ctk_spin_button_unrealize      (CtkWidget          *widget);
static void ctk_spin_button_get_preferred_width  (CtkWidget          *widget,
                                                  gint               *minimum,
                                                  gint               *natural);
static void ctk_spin_button_get_preferred_height (CtkWidget          *widget,
                                                  gint               *minimum,
                                                  gint               *natural);
static void ctk_spin_button_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
									 gint       width,
									 gint      *minimum,
									 gint      *natural,
									 gint      *minimum_baseline,
									 gint      *natural_baseline);
static void ctk_spin_button_size_allocate  (CtkWidget          *widget,
                                            CtkAllocation      *allocation);
static gint ctk_spin_button_draw           (CtkWidget          *widget,
                                            cairo_t            *cr);
static gint ctk_spin_button_button_press   (CtkWidget          *widget,
                                            GdkEventButton     *event);
static gint ctk_spin_button_button_release (CtkWidget          *widget,
                                            GdkEventButton     *event);
static gint ctk_spin_button_motion_notify  (CtkWidget          *widget,
                                            GdkEventMotion     *event);
static gint ctk_spin_button_enter_notify   (CtkWidget          *widget,
                                            GdkEventCrossing   *event);
static gint ctk_spin_button_leave_notify   (CtkWidget          *widget,
                                            GdkEventCrossing   *event);
static gint ctk_spin_button_focus_out      (CtkWidget          *widget,
                                            GdkEventFocus      *event);
static void ctk_spin_button_grab_notify    (CtkWidget          *widget,
                                            gboolean            was_grabbed);
static void ctk_spin_button_state_flags_changed  (CtkWidget     *widget,
                                                  CtkStateFlags  previous_state);
static gboolean ctk_spin_button_timer          (CtkSpinButton      *spin_button);
static gboolean ctk_spin_button_stop_spinning  (CtkSpinButton      *spin);
static void ctk_spin_button_value_changed  (CtkAdjustment      *adjustment,
                                            CtkSpinButton      *spin_button);
static gint ctk_spin_button_key_release    (CtkWidget          *widget,
                                            GdkEventKey        *event);
static gint ctk_spin_button_scroll         (CtkWidget          *widget,
                                            GdkEventScroll     *event);
static void ctk_spin_button_direction_changed (CtkWidget        *widget,
                                               CtkTextDirection  previous_dir);
static void ctk_spin_button_activate       (CtkEntry           *entry);
static void ctk_spin_button_unset_adjustment (CtkSpinButton *spin_button);
static void ctk_spin_button_set_orientation (CtkSpinButton     *spin_button,
                                             CtkOrientation     orientation);
static void ctk_spin_button_snap           (CtkSpinButton      *spin_button,
                                            gdouble             val);
static void ctk_spin_button_insert_text    (CtkEditable        *editable,
                                            const gchar        *new_text,
                                            gint                new_text_length,
                                            gint               *position);
static void ctk_spin_button_real_spin      (CtkSpinButton      *spin_button,
                                            gdouble             step);
static void ctk_spin_button_real_change_value (CtkSpinButton   *spin,
                                               CtkScrollType    scroll);

static gint ctk_spin_button_default_input  (CtkSpinButton      *spin_button,
                                            gdouble            *new_val);
static void ctk_spin_button_default_output (CtkSpinButton      *spin_button);
static void update_node_state              (CtkSpinButton *spin_button);

static guint spinbutton_signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE_WITH_CODE (CtkSpinButton, ctk_spin_button, CTK_TYPE_ENTRY,
                         G_ADD_PRIVATE (CtkSpinButton)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_EDITABLE,
                                                ctk_spin_button_editable_init))

#define add_spin_binding(binding_set, keyval, mask, scroll)            \
  ctk_binding_entry_add_signal (binding_set, keyval, mask,             \
                                "change-value", 1,                     \
                                CTK_TYPE_SCROLL_TYPE, scroll)

static void
ctk_spin_button_class_init (CtkSpinButtonClass *class)
{
  GObjectClass     *gobject_class = G_OBJECT_CLASS (class);
  CtkWidgetClass   *widget_class = CTK_WIDGET_CLASS (class);
  CtkEntryClass    *entry_class = CTK_ENTRY_CLASS (class);
  CtkBindingSet    *binding_set;

  gobject_class->finalize = ctk_spin_button_finalize;
  gobject_class->set_property = ctk_spin_button_set_property;
  gobject_class->get_property = ctk_spin_button_get_property;

  widget_class->destroy = ctk_spin_button_destroy;
  widget_class->map = ctk_spin_button_map;
  widget_class->unmap = ctk_spin_button_unmap;
  widget_class->realize = ctk_spin_button_realize;
  widget_class->unrealize = ctk_spin_button_unrealize;
  widget_class->get_preferred_width = ctk_spin_button_get_preferred_width;
  widget_class->get_preferred_height = ctk_spin_button_get_preferred_height;
  widget_class->get_preferred_height_and_baseline_for_width = ctk_spin_button_get_preferred_height_and_baseline_for_width;
  widget_class->size_allocate = ctk_spin_button_size_allocate;
  widget_class->draw = ctk_spin_button_draw;
  widget_class->scroll_event = ctk_spin_button_scroll;
  widget_class->button_press_event = ctk_spin_button_button_press;
  widget_class->button_release_event = ctk_spin_button_button_release;
  widget_class->motion_notify_event = ctk_spin_button_motion_notify;
  widget_class->key_release_event = ctk_spin_button_key_release;
  widget_class->enter_notify_event = ctk_spin_button_enter_notify;
  widget_class->leave_notify_event = ctk_spin_button_leave_notify;
  widget_class->focus_out_event = ctk_spin_button_focus_out;
  widget_class->grab_notify = ctk_spin_button_grab_notify;
  widget_class->state_flags_changed = ctk_spin_button_state_flags_changed;
  widget_class->direction_changed = ctk_spin_button_direction_changed;

  entry_class->activate = ctk_spin_button_activate;

  class->input = NULL;
  class->output = NULL;
  class->change_value = ctk_spin_button_real_change_value;

  g_object_class_install_property (gobject_class,
                                   PROP_ADJUSTMENT,
                                   g_param_spec_object ("adjustment",
                                                        P_("Adjustment"),
                                                        P_("The adjustment that holds the value of the spin button"),
                                                        CTK_TYPE_ADJUSTMENT,
                                                        CTK_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_CLIMB_RATE,
                                   g_param_spec_double ("climb-rate",
                                                        P_("Climb Rate"),
                                                        P_("The acceleration rate when you hold down a button or key"),
                                                        0.0, G_MAXDOUBLE, 0.0,
                                                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
                                   PROP_DIGITS,
                                   g_param_spec_uint ("digits",
                                                      P_("Digits"),
                                                      P_("The number of decimal places to display"),
                                                      0, MAX_DIGITS, 0,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
                                   PROP_SNAP_TO_TICKS,
                                   g_param_spec_boolean ("snap-to-ticks",
                                                         P_("Snap to Ticks"),
                                                         P_("Whether erroneous values are automatically changed to a spin button's nearest step increment"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
                                   PROP_NUMERIC,
                                   g_param_spec_boolean ("numeric",
                                                         P_("Numeric"),
                                                         P_("Whether non-numeric characters should be ignored"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
                                   PROP_WRAP,
                                   g_param_spec_boolean ("wrap",
                                                         P_("Wrap"),
                                                         P_("Whether a spin button should wrap upon reaching its limits"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
                                   PROP_UPDATE_POLICY,
                                   g_param_spec_enum ("update-policy",
                                                      P_("Update Policy"),
                                                      P_("Whether the spin button should update always, or only when the value is legal"),
                                                      CTK_TYPE_SPIN_BUTTON_UPDATE_POLICY,
                                                      CTK_UPDATE_ALWAYS,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
                                   PROP_VALUE,
                                   g_param_spec_double ("value",
                                                        P_("Value"),
                                                        P_("Reads the current value, or sets a new value"),
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                                                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_override_property (gobject_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  /**
   * CtkSpinButton:shadow-type:
   *
   * Style of bevel around the spin button.
   *
   * Deprecated: 3.20: Use CSS to determine the style of the border;
   *     the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("shadow-type",
                                                              P_("Shadow Type"),
                                                              P_("Style of bevel around the spin button"),
                                                              CTK_TYPE_SHADOW_TYPE,
                                                              CTK_SHADOW_IN,
                                                              CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkSpinButton::input:
   * @spin_button: the object on which the signal was emitted
   * @new_value: (out) (type double): return location for the new value
   *
   * The ::input signal can be used to influence the conversion of
   * the users input into a double value. The signal handler is
   * expected to use ctk_entry_get_text() to retrieve the text of
   * the entry and set @new_value to the new value.
   *
   * The default conversion uses g_strtod().
   *
   * Returns: %TRUE for a successful conversion, %FALSE if the input
   *     was not handled, and %CTK_INPUT_ERROR if the conversion failed.
   */
  spinbutton_signals[INPUT] =
    g_signal_new (I_("input"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkSpinButtonClass, input),
                  NULL, NULL,
                  _ctk_marshal_INT__POINTER,
                  G_TYPE_INT, 1,
                  G_TYPE_POINTER);

  /**
   * CtkSpinButton::output:
   * @spin_button: the object on which the signal was emitted
   *
   * The ::output signal can be used to change to formatting
   * of the value that is displayed in the spin buttons entry.
   * |[<!-- language="C" -->
   * // show leading zeros
   * static gboolean
   * on_output (CtkSpinButton *spin,
   *            gpointer       data)
   * {
   *    CtkAdjustment *adjustment;
   *    gchar *text;
   *    int value;
   *
   *    adjustment = ctk_spin_button_get_adjustment (spin);
   *    value = (int)ctk_adjustment_get_value (adjustment);
   *    text = g_strdup_printf ("%02d", value);
   *    ctk_entry_set_text (CTK_ENTRY (spin), text);
   *    g_free (text);
   *
   *    return TRUE;
   * }
   * ]|
   *
   * Returns: %TRUE if the value has been displayed
   */
  spinbutton_signals[OUTPUT] =
    g_signal_new (I_("output"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkSpinButtonClass, output),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);

  /**
   * CtkSpinButton::value-changed:
   * @spin_button: the object on which the signal was emitted
   *
   * The ::value-changed signal is emitted when the value represented by
   * @spinbutton changes. Also see the #CtkSpinButton::output signal.
   */
  spinbutton_signals[VALUE_CHANGED] =
    g_signal_new (I_("value-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkSpinButtonClass, value_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkSpinButton::wrapped:
   * @spin_button: the object on which the signal was emitted
   *
   * The ::wrapped signal is emitted right after the spinbutton wraps
   * from its maximum to minimum value or vice-versa.
   *
   * Since: 2.10
   */
  spinbutton_signals[WRAPPED] =
    g_signal_new (I_("wrapped"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkSpinButtonClass, wrapped),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /* Action signals */
  /**
   * CtkSpinButton::change-value:
   * @spin_button: the object on which the signal was emitted
   * @scroll: a #CtkScrollType to specify the speed and amount of change
   *
   * The ::change-value signal is a [keybinding signal][CtkBindingSignal] 
   * which gets emitted when the user initiates a value change. 
   *
   * Applications should not connect to it, but may emit it with 
   * g_signal_emit_by_name() if they need to control the cursor
   * programmatically.
   *
   * The default bindings for this signal are Up/Down and PageUp and/PageDown.
   */
  spinbutton_signals[CHANGE_VALUE] =
    g_signal_new (I_("change-value"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkSpinButtonClass, change_value),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CTK_TYPE_SCROLL_TYPE);

  binding_set = ctk_binding_set_by_class (class);

  add_spin_binding (binding_set, GDK_KEY_Up, 0, CTK_SCROLL_STEP_UP);
  add_spin_binding (binding_set, GDK_KEY_KP_Up, 0, CTK_SCROLL_STEP_UP);
  add_spin_binding (binding_set, GDK_KEY_Down, 0, CTK_SCROLL_STEP_DOWN);
  add_spin_binding (binding_set, GDK_KEY_KP_Down, 0, CTK_SCROLL_STEP_DOWN);
  add_spin_binding (binding_set, GDK_KEY_Page_Up, 0, CTK_SCROLL_PAGE_UP);
  add_spin_binding (binding_set, GDK_KEY_Page_Down, 0, CTK_SCROLL_PAGE_DOWN);
  add_spin_binding (binding_set, GDK_KEY_End, GDK_CONTROL_MASK, CTK_SCROLL_END);
  add_spin_binding (binding_set, GDK_KEY_Home, GDK_CONTROL_MASK, CTK_SCROLL_START);
  add_spin_binding (binding_set, GDK_KEY_Page_Up, GDK_CONTROL_MASK, CTK_SCROLL_END);
  add_spin_binding (binding_set, GDK_KEY_Page_Down, GDK_CONTROL_MASK, CTK_SCROLL_START);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_SPIN_BUTTON_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "spinbutton");
}

static void
ctk_spin_button_editable_init (CtkEditableInterface *iface)
{
  iface->insert_text = ctk_spin_button_insert_text;
}

static void
ctk_spin_button_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  CtkSpinButton *spin_button = CTK_SPIN_BUTTON (object);
  CtkSpinButtonPrivate *priv = spin_button->priv;

  switch (prop_id)
    {
      CtkAdjustment *adjustment;

    case PROP_ADJUSTMENT:
      adjustment = CTK_ADJUSTMENT (g_value_get_object (value));
      ctk_spin_button_set_adjustment (spin_button, adjustment);
      break;
    case PROP_CLIMB_RATE:
      ctk_spin_button_configure (spin_button,
                                 priv->adjustment,
                                 g_value_get_double (value),
                                 priv->digits);
      break;
    case PROP_DIGITS:
      ctk_spin_button_configure (spin_button,
                                 priv->adjustment,
                                 priv->climb_rate,
                                 g_value_get_uint (value));
      break;
    case PROP_SNAP_TO_TICKS:
      ctk_spin_button_set_snap_to_ticks (spin_button, g_value_get_boolean (value));
      break;
    case PROP_NUMERIC:
      ctk_spin_button_set_numeric (spin_button, g_value_get_boolean (value));
      break;
    case PROP_WRAP:
      ctk_spin_button_set_wrap (spin_button, g_value_get_boolean (value));
      break;
    case PROP_UPDATE_POLICY:
      ctk_spin_button_set_update_policy (spin_button, g_value_get_enum (value));
      break;
    case PROP_VALUE:
      ctk_spin_button_set_value (spin_button, g_value_get_double (value));
      break;
    case PROP_ORIENTATION:
      ctk_spin_button_set_orientation (spin_button, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_spin_button_get_property (GObject      *object,
                              guint         prop_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  CtkSpinButton *spin_button = CTK_SPIN_BUTTON (object);
  CtkSpinButtonPrivate *priv = spin_button->priv;

  switch (prop_id)
    {
    case PROP_ADJUSTMENT:
      g_value_set_object (value, priv->adjustment);
      break;
    case PROP_CLIMB_RATE:
      g_value_set_double (value, priv->climb_rate);
      break;
    case PROP_DIGITS:
      g_value_set_uint (value, priv->digits);
      break;
    case PROP_SNAP_TO_TICKS:
      g_value_set_boolean (value, priv->snap_to_ticks);
      break;
    case PROP_NUMERIC:
      g_value_set_boolean (value, priv->numeric);
      break;
    case PROP_WRAP:
      g_value_set_boolean (value, priv->wrap);
      break;
    case PROP_UPDATE_POLICY:
      g_value_set_enum (value, priv->update_policy);
      break;
     case PROP_VALUE:
       g_value_set_double (value, ctk_adjustment_get_value (priv->adjustment));
      break;
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
swipe_gesture_begin (CtkGesture       *gesture,
                     GdkEventSequence *sequence,
                     CtkSpinButton    *spin_button)
{
  GdkEventSequence *current;
  const GdkEvent *event;

  current = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
  event = ctk_gesture_get_last_event (gesture, current);

  if (event->any.window == spin_button->priv->up_panel ||
      event->any.window == spin_button->priv->down_panel)
    ctk_gesture_set_state (gesture, CTK_EVENT_SEQUENCE_DENIED);

  ctk_gesture_set_state (gesture, CTK_EVENT_SEQUENCE_CLAIMED);
  ctk_widget_grab_focus (CTK_WIDGET (spin_button));
}

static void
swipe_gesture_update (CtkGesture       *gesture,
                      GdkEventSequence *sequence,
                      CtkSpinButton    *spin_button)
{
  gdouble vel_y;

  ctk_gesture_swipe_get_velocity (CTK_GESTURE_SWIPE (gesture), NULL, &vel_y);
  ctk_spin_button_real_spin (spin_button, -vel_y / 20);
}

static void
update_node_ordering (CtkSpinButton *spin_button)
{
  CtkSpinButtonPrivate *priv = spin_button->priv;
  int down_button_pos, up_button_pos;

  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (ctk_widget_get_direction (CTK_WIDGET (spin_button)) == CTK_TEXT_DIR_LTR)
        {
          down_button_pos = 1;
          up_button_pos = -1;
        }
      else
        {
          down_button_pos = 1;
          up_button_pos = 0;
        }
    }
  else
    {
      up_button_pos = 0;
      down_button_pos = -1;
    }

  ctk_box_gadget_set_orientation (CTK_BOX_GADGET (priv->gadget), priv->orientation);
  ctk_box_gadget_remove_gadget (CTK_BOX_GADGET (priv->gadget), priv->up_button);
  ctk_box_gadget_remove_gadget (CTK_BOX_GADGET (priv->gadget), priv->down_button);
  ctk_box_gadget_insert_gadget (CTK_BOX_GADGET (priv->gadget),
                                up_button_pos, priv->up_button,
                                FALSE, CTK_ALIGN_FILL);
  ctk_box_gadget_insert_gadget (CTK_BOX_GADGET (priv->gadget),
                                down_button_pos, priv->down_button,
                                FALSE, CTK_ALIGN_FILL);
}

static void
ctk_spin_button_init (CtkSpinButton *spin_button)
{
  CtkSpinButtonPrivate *priv;
  CtkCssNode *widget_node, *entry_node;

  spin_button->priv = ctk_spin_button_get_instance_private (spin_button);
  priv = spin_button->priv;

  priv->adjustment = NULL;
  priv->down_panel = NULL;
  priv->up_panel = NULL;
  priv->timer = 0;
  priv->climb_rate = 0.0;
  priv->timer_step = 0.0;
  priv->update_policy = CTK_UPDATE_ALWAYS;
  priv->in_child = NULL;
  priv->click_child = NULL;
  priv->button = 0;
  priv->need_timer = FALSE;
  priv->timer_calls = 0;
  priv->digits = 0;
  priv->numeric = FALSE;
  priv->wrap = FALSE;
  priv->snap_to_ticks = FALSE;

  priv->orientation = CTK_ORIENTATION_HORIZONTAL;

  _ctk_orientable_set_style_classes (CTK_ORIENTABLE (spin_button));

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (spin_button));

  priv->gadget = ctk_box_gadget_new_for_node (widget_node, CTK_WIDGET (spin_button));

  entry_node = ctk_css_node_new ();
  ctk_css_node_set_name (entry_node, I_("entry"));
  ctk_css_node_set_parent (entry_node, widget_node);
  ctk_css_node_set_state (entry_node, ctk_css_node_get_state (widget_node));
  ctk_css_gadget_set_node (ctk_entry_get_gadget (CTK_ENTRY (spin_button)), entry_node);
  g_object_unref (entry_node);
  ctk_box_gadget_insert_gadget (CTK_BOX_GADGET (priv->gadget),
                                -1, ctk_entry_get_gadget (CTK_ENTRY (spin_button)),
                                TRUE, CTK_ALIGN_FILL);

  priv->down_button = ctk_icon_helper_new_named ("button",
                                                 CTK_WIDGET (spin_button));
  _ctk_icon_helper_set_use_fallback (CTK_ICON_HELPER (priv->down_button), TRUE);
  _ctk_icon_helper_set_icon_name (CTK_ICON_HELPER (priv->down_button), "list-remove-symbolic", CTK_ICON_SIZE_MENU);
  ctk_css_gadget_add_class (priv->down_button, "down");
  ctk_css_node_set_parent (ctk_css_gadget_get_node (priv->down_button), widget_node);
  ctk_css_node_set_state (ctk_css_gadget_get_node (priv->down_button), ctk_css_node_get_state (widget_node));
  ctk_box_gadget_insert_gadget (CTK_BOX_GADGET (priv->gadget),
                                -1, priv->down_button,
                                FALSE, CTK_ALIGN_FILL);

  priv->up_button = ctk_icon_helper_new_named ("button",
                                               CTK_WIDGET (spin_button));
  _ctk_icon_helper_set_use_fallback (CTK_ICON_HELPER (priv->up_button), TRUE);
  _ctk_icon_helper_set_icon_name (CTK_ICON_HELPER (priv->up_button), "list-add-symbolic", CTK_ICON_SIZE_MENU);
  ctk_css_gadget_add_class (priv->up_button, "up");
  ctk_css_node_set_parent (ctk_css_gadget_get_node (priv->up_button), widget_node);
  ctk_css_node_set_state (ctk_css_gadget_get_node (priv->up_button), ctk_css_node_get_state (widget_node));
  ctk_box_gadget_insert_gadget (CTK_BOX_GADGET (priv->gadget),
                                -1, priv->up_button,
                                FALSE, CTK_ALIGN_FILL);

  ctk_spin_button_set_adjustment (spin_button, NULL);

  update_node_ordering (spin_button);
  update_node_state (spin_button);

  ctk_widget_add_events (CTK_WIDGET (spin_button), GDK_SCROLL_MASK);

  priv->swipe_gesture = ctk_gesture_swipe_new (CTK_WIDGET (spin_button));
  ctk_gesture_single_set_touch_only (CTK_GESTURE_SINGLE (priv->swipe_gesture), TRUE);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->swipe_gesture),
                                              CTK_PHASE_CAPTURE);
  g_signal_connect (priv->swipe_gesture, "begin",
                    G_CALLBACK (swipe_gesture_begin), spin_button);
  g_signal_connect (priv->swipe_gesture, "update",
                    G_CALLBACK (swipe_gesture_update), spin_button);
}

static void
ctk_spin_button_finalize (GObject *object)
{
  CtkSpinButton *spin_button = CTK_SPIN_BUTTON (object);
  CtkSpinButtonPrivate *priv = spin_button->priv;

  ctk_spin_button_unset_adjustment (spin_button);
  g_clear_object (&priv->gadget);
  g_clear_object (&priv->down_button);
  g_clear_object (&priv->up_button);

  g_object_unref (priv->swipe_gesture);

  G_OBJECT_CLASS (ctk_spin_button_parent_class)->finalize (object);
}

static void
ctk_spin_button_destroy (CtkWidget *widget)
{
  ctk_spin_button_stop_spinning (CTK_SPIN_BUTTON (widget));

  CTK_WIDGET_CLASS (ctk_spin_button_parent_class)->destroy (widget);
}

static void
ctk_spin_button_map (CtkWidget *widget)
{
  CtkSpinButton *spin_button = CTK_SPIN_BUTTON (widget);
  CtkSpinButtonPrivate *priv = spin_button->priv;

  if (ctk_widget_get_realized (widget) && !ctk_widget_get_mapped (widget))
    {
      CTK_WIDGET_CLASS (ctk_spin_button_parent_class)->map (widget);
      gdk_window_show (priv->down_panel);
      gdk_window_show (priv->up_panel);
    }
}

static void
ctk_spin_button_unmap (CtkWidget *widget)
{
  CtkSpinButton *spin_button = CTK_SPIN_BUTTON (widget);
  CtkSpinButtonPrivate *priv = spin_button->priv;

  if (ctk_widget_get_mapped (widget))
    {
      ctk_spin_button_stop_spinning (CTK_SPIN_BUTTON (widget));

      gdk_window_hide (priv->down_panel);
      gdk_window_hide (priv->up_panel);
      CTK_WIDGET_CLASS (ctk_spin_button_parent_class)->unmap (widget);
    }
}

static gboolean
ctk_spin_button_panel_at_limit (CtkSpinButton *spin_button,
                                gint           panel)
{
  CtkSpinButtonPrivate *priv = spin_button->priv;

  if (priv->wrap)
    return FALSE;

  if (panel == UP_PANEL &&
      (ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_value (priv->adjustment) <= EPSILON))
    return TRUE;

  if (panel == DOWN_PANEL &&
      (ctk_adjustment_get_value (priv->adjustment) - ctk_adjustment_get_lower (priv->adjustment) <= EPSILON))
    return TRUE;

  return FALSE;
}

static CtkStateFlags
ctk_spin_button_panel_get_state (CtkSpinButton *spin_button,
                                 gint           panel)
{
  CtkStateFlags state;
  CtkSpinButtonPrivate *priv = spin_button->priv;

  state = ctk_widget_get_state_flags (CTK_WIDGET (spin_button));

  state &= ~(CTK_STATE_FLAG_ACTIVE | CTK_STATE_FLAG_PRELIGHT | CTK_STATE_FLAG_DROP_ACTIVE);

  if ((state & CTK_STATE_FLAG_INSENSITIVE) ||
      ctk_spin_button_panel_at_limit (spin_button, panel) ||
      !ctk_editable_get_editable (CTK_EDITABLE (spin_button)))
    {
      state |= CTK_STATE_FLAG_INSENSITIVE;
    }
  else
    {
      GdkWindow *panel_win;

      panel_win = panel == UP_PANEL ? priv->up_panel : priv->down_panel;

      if (priv->click_child &&
          priv->click_child == panel_win)
        state |= CTK_STATE_FLAG_ACTIVE;
      else if (priv->in_child &&
               priv->in_child == panel_win &&
               priv->click_child == NULL)
        state |= CTK_STATE_FLAG_PRELIGHT;
    }

  return state;
}

static void
update_node_state (CtkSpinButton *spin_button)
{
  CtkSpinButtonPrivate *priv = spin_button->priv;

  ctk_css_gadget_set_state (priv->up_button, ctk_spin_button_panel_get_state (spin_button, UP_PANEL));
  ctk_css_gadget_set_state (priv->down_button, ctk_spin_button_panel_get_state (spin_button, DOWN_PANEL));
}

static void
ctk_spin_button_realize (CtkWidget *widget)
{
  CtkSpinButton *spin_button = CTK_SPIN_BUTTON (widget);
  CtkSpinButtonPrivate *priv = spin_button->priv;
  CtkAllocation down_allocation, up_allocation;
  GdkWindowAttr attributes;
  gint attributes_mask;
  gboolean return_val;

  ctk_widget_set_events (widget, ctk_widget_get_events (widget) |
                         GDK_KEY_RELEASE_MASK);
  CTK_WIDGET_CLASS (ctk_spin_button_parent_class)->realize (widget);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.event_mask = ctk_widget_get_events (widget);
  attributes.event_mask |= GDK_BUTTON_PRESS_MASK
    | GDK_BUTTON_RELEASE_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK
    | GDK_POINTER_MOTION_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

  ctk_css_gadget_get_border_allocation (priv->up_button, &up_allocation, NULL);
  ctk_css_gadget_get_border_allocation (priv->down_button, &down_allocation, NULL);

  /* create the left panel window */
  attributes.x = down_allocation.x;
  attributes.y = down_allocation.y;
  attributes.width = down_allocation.width;
  attributes.height = down_allocation.height;

  priv->down_panel = gdk_window_new (ctk_widget_get_window (widget),
                                     &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->down_panel);

  /* create the right panel window */
  attributes.x = up_allocation.x;
  attributes.y = up_allocation.y;
  attributes.width = up_allocation.width;
  attributes.height = up_allocation.height;

  priv->up_panel = gdk_window_new (ctk_widget_get_window (widget),
                                      &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->up_panel);

  return_val = FALSE;
  g_signal_emit (spin_button, spinbutton_signals[OUTPUT], 0, &return_val);

  /* If output wasn't processed explicitly by the method connected to the
   * 'output' signal; and if we don't have any explicit 'text' set initially,
   * fallback to the default output. */
  if (!return_val &&
      (spin_button->priv->numeric || ctk_entry_get_text (CTK_ENTRY (spin_button)) == NULL))
    ctk_spin_button_default_output (spin_button);

  ctk_widget_queue_resize (CTK_WIDGET (spin_button));
}

static void
ctk_spin_button_unrealize (CtkWidget *widget)
{
  CtkSpinButton *spin = CTK_SPIN_BUTTON (widget);
  CtkSpinButtonPrivate *priv = spin->priv;

  CTK_WIDGET_CLASS (ctk_spin_button_parent_class)->unrealize (widget);

  if (priv->down_panel)
    {
      ctk_widget_unregister_window (widget, priv->down_panel);
      gdk_window_destroy (priv->down_panel);
      priv->down_panel = NULL;
    }

  if (priv->up_panel)
    {
      ctk_widget_unregister_window (widget, priv->up_panel);
      gdk_window_destroy (priv->up_panel);
      priv->up_panel = NULL;
    }
}

/* Callback used when the spin button's adjustment changes.
 * We need to redraw the arrows when the adjustment’s range
 * changes, and reevaluate our size request.
 */
static void
adjustment_changed_cb (CtkAdjustment *adjustment, gpointer data)
{
  CtkSpinButton *spin_button = CTK_SPIN_BUTTON (data);
  CtkSpinButtonPrivate *priv = spin_button->priv;

  priv->timer_step = ctk_adjustment_get_step_increment (priv->adjustment);
  update_node_state (spin_button);
  ctk_widget_queue_resize (CTK_WIDGET (spin_button));
}

static void
ctk_spin_button_unset_adjustment (CtkSpinButton *spin_button)
{
  CtkSpinButtonPrivate *priv = spin_button->priv;

  if (priv->adjustment)
    {
      g_signal_handlers_disconnect_by_func (priv->adjustment,
                                            ctk_spin_button_value_changed,
                                            spin_button);
      g_signal_handlers_disconnect_by_func (priv->adjustment,
                                            adjustment_changed_cb,
                                            spin_button);
      g_clear_object (&priv->adjustment);
    }
}

static void
ctk_spin_button_set_orientation (CtkSpinButton  *spin,
                                 CtkOrientation  orientation)
{
  CtkEntry *entry = CTK_ENTRY (spin);
  CtkSpinButtonPrivate *priv = spin->priv;

  if (priv->orientation == orientation)
    return;

  priv->orientation = orientation;
  _ctk_orientable_set_style_classes (CTK_ORIENTABLE (spin));

  /* change alignment if it's the default */
  if (priv->orientation == CTK_ORIENTATION_VERTICAL &&
      ctk_entry_get_alignment (entry) == 0.0)
    ctk_entry_set_alignment (entry, 0.5);
  else if (priv->orientation == CTK_ORIENTATION_HORIZONTAL &&
           ctk_entry_get_alignment (entry) == 0.5)
    ctk_entry_set_alignment (entry, 0.0);

  update_node_ordering (spin);

  g_object_notify (G_OBJECT (spin), "orientation");
  ctk_widget_queue_resize (CTK_WIDGET (spin));
}

static gint
measure_string_width (PangoLayout *layout,
                      const gchar *string)
{
  gint width;

  pango_layout_set_text (layout, string, -1);
  pango_layout_get_pixel_size (layout, &width, NULL);

  return width;
}

static gchar *
weed_out_neg_zero (gchar *str,
                   gint   digits)
{
  if (str[0] == '-')
    {
      gchar neg_zero[8];
      g_snprintf (neg_zero, 8, "%0.*f", digits, -0.0);
      if (strcmp (neg_zero, str) == 0)
        memmove (str, str + 1, strlen (str));
    }
  return str;
}

static gchar *
ctk_spin_button_format_for_value (CtkSpinButton *spin_button,
                                  gdouble        value)
{
  CtkSpinButtonPrivate *priv = spin_button->priv;
  gchar *buf = g_strdup_printf ("%0.*f", priv->digits, value);

  return weed_out_neg_zero (buf, priv->digits);
}

gint
ctk_spin_button_get_text_width (CtkSpinButton *spin_button)
{
  CtkSpinButtonPrivate *priv = spin_button->priv;
  gint width, w;
  PangoLayout *layout;
  gchar *str;
  gdouble value;

  layout = pango_layout_copy (ctk_entry_get_layout (CTK_ENTRY (spin_button)));

  /* Get max of MIN_SPIN_BUTTON_WIDTH, size of upper, size of lower */
  width = MIN_SPIN_BUTTON_WIDTH;

  value = CLAMP (ctk_adjustment_get_upper (priv->adjustment), -1e7, 1e7);
  str = ctk_spin_button_format_for_value (spin_button, value);
  w = measure_string_width (layout, str);
  width = MAX (width, w);
  g_free (str);

  value = CLAMP (ctk_adjustment_get_lower (priv->adjustment), -1e7, 1e7);
  str = ctk_spin_button_format_for_value (spin_button, value);
  w = measure_string_width (layout, str);
  width = MAX (width, w);
  g_free (str);

  g_object_unref (layout);

  return width;
}

static void
ctk_spin_button_get_preferred_width (CtkWidget *widget,
                                     gint      *minimum,
                                     gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_SPIN_BUTTON (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_spin_button_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
							     gint       width,
							     gint      *minimum,
							     gint      *natural,
							     gint      *minimum_baseline,
							     gint      *natural_baseline)
{
  ctk_css_gadget_get_preferred_size (CTK_SPIN_BUTTON (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     minimum_baseline, natural_baseline);
}

static void
ctk_spin_button_get_preferred_height (CtkWidget *widget,
                                      gint      *minimum,
                                      gint      *natural)
{
  ctk_spin_button_get_preferred_height_and_baseline_for_width (widget, -1, minimum, natural, NULL, NULL);
}

static void
ctk_spin_button_size_allocate (CtkWidget     *widget,
                               CtkAllocation *allocation)
{
  CtkSpinButton *spin = CTK_SPIN_BUTTON (widget);
  CtkSpinButtonPrivate *priv = spin->priv;
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);

  if (ctk_widget_get_realized (widget))
    {
      CtkAllocation button_alloc;

      ctk_css_gadget_get_border_allocation (priv->down_button, &button_alloc, NULL);
      gdk_window_move_resize (priv->down_panel,
                              button_alloc.x, button_alloc.y,
                              button_alloc.width, button_alloc.height);

      ctk_css_gadget_get_border_allocation (priv->up_button, &button_alloc, NULL);
      gdk_window_move_resize (priv->up_panel,
                              button_alloc.x, button_alloc.y,
                              button_alloc.width, button_alloc.height);
    }
}

static gint
ctk_spin_button_draw (CtkWidget *widget,
                      cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_SPIN_BUTTON(widget)->priv->gadget, cr);

  return GDK_EVENT_PROPAGATE;
}

static gint
ctk_spin_button_enter_notify (CtkWidget        *widget,
                              GdkEventCrossing *event)
{
  CtkSpinButton *spin = CTK_SPIN_BUTTON (widget);
  CtkSpinButtonPrivate *priv = spin->priv;

  if (event->window == priv->down_panel ||
      event->window == priv->up_panel)
    {
      priv->in_child = event->window;
      update_node_state (spin);
      ctk_widget_queue_draw (CTK_WIDGET (spin));
    }

  return CTK_WIDGET_CLASS (ctk_spin_button_parent_class)->enter_notify_event (widget, event);
}

static gint
ctk_spin_button_leave_notify (CtkWidget        *widget,
                              GdkEventCrossing *event)
{
  CtkSpinButton *spin = CTK_SPIN_BUTTON (widget);
  CtkSpinButtonPrivate *priv = spin->priv;

  if (priv->in_child != NULL)
    {
      priv->in_child = NULL;
      update_node_state (spin);
      ctk_widget_queue_draw (CTK_WIDGET (spin));
    }

  return CTK_WIDGET_CLASS (ctk_spin_button_parent_class)->leave_notify_event (widget, event);
}

static gint
ctk_spin_button_focus_out (CtkWidget     *widget,
                           GdkEventFocus *event)
{
  if (ctk_editable_get_editable (CTK_EDITABLE (widget)))
    ctk_spin_button_update (CTK_SPIN_BUTTON (widget));

  return CTK_WIDGET_CLASS (ctk_spin_button_parent_class)->focus_out_event (widget, event);
}

static void
ctk_spin_button_grab_notify (CtkWidget *widget,
                             gboolean   was_grabbed)
{
  CtkSpinButton *spin = CTK_SPIN_BUTTON (widget);

  if (!was_grabbed)
    {
      if (ctk_spin_button_stop_spinning (spin))
        ctk_widget_queue_draw (CTK_WIDGET (spin));
    }
}

static void
ctk_spin_button_state_flags_changed (CtkWidget     *widget,
                                     CtkStateFlags  previous_state)
{
  CtkSpinButton *spin = CTK_SPIN_BUTTON (widget);

  if (!ctk_widget_is_sensitive (widget))
    {
      if (ctk_spin_button_stop_spinning (spin))
        ctk_widget_queue_draw (CTK_WIDGET (spin));
    }

  ctk_css_gadget_set_state (ctk_entry_get_gadget (CTK_ENTRY (widget)), ctk_widget_get_state_flags (widget));
  update_node_state (spin);

  CTK_WIDGET_CLASS (ctk_spin_button_parent_class)->state_flags_changed (widget, previous_state);
}

static gint
ctk_spin_button_scroll (CtkWidget      *widget,
                        GdkEventScroll *event)
{
  CtkSpinButton *spin = CTK_SPIN_BUTTON (widget);
  CtkSpinButtonPrivate *priv = spin->priv;

  if (event->direction == GDK_SCROLL_UP)
    {
      if (!ctk_widget_has_focus (widget))
        ctk_widget_grab_focus (widget);
      ctk_spin_button_real_spin (spin, ctk_adjustment_get_step_increment (priv->adjustment));
    }
  else if (event->direction == GDK_SCROLL_DOWN)
    {
      if (!ctk_widget_has_focus (widget))
        ctk_widget_grab_focus (widget);
      ctk_spin_button_real_spin (spin, -ctk_adjustment_get_step_increment (priv->adjustment));
    }
  else
    return FALSE;

  return TRUE;
}

static gboolean
ctk_spin_button_stop_spinning (CtkSpinButton *spin)
{
  CtkSpinButtonPrivate *priv = spin->priv;
  gboolean did_spin = FALSE;

  if (priv->timer)
    {
      g_source_remove (priv->timer);
      priv->timer = 0;
      priv->need_timer = FALSE;

      did_spin = TRUE;
    }

  priv->button = 0;
  priv->timer_step = ctk_adjustment_get_step_increment (priv->adjustment);
  priv->timer_calls = 0;

  priv->click_child = NULL;

  return did_spin;
}

static void
start_spinning (CtkSpinButton *spin,
                GdkWindow     *click_child,
                gdouble        step)
{
  CtkSpinButtonPrivate *priv;

  priv = spin->priv;

  priv->click_child = click_child;

  if (!priv->timer)
    {
      priv->timer_step = step;
      priv->need_timer = TRUE;
      priv->timer = gdk_threads_add_timeout (TIMEOUT_INITIAL,
                                   (GSourceFunc) ctk_spin_button_timer,
                                   (gpointer) spin);
      g_source_set_name_by_id (priv->timer, "[ctk+] ctk_spin_button_timer");
    }
  ctk_spin_button_real_spin (spin, click_child == priv->up_panel ? step : -step);

  ctk_widget_queue_draw (CTK_WIDGET (spin));
}

static gint
ctk_spin_button_button_press (CtkWidget      *widget,
                              GdkEventButton *event)
{
  CtkSpinButton *spin = CTK_SPIN_BUTTON (widget);
  CtkSpinButtonPrivate *priv = spin->priv;

  if (!priv->button)
    {
      if ((event->window == priv->down_panel) ||
          (event->window == priv->up_panel))
        {
          if (!ctk_widget_has_focus (widget))
            ctk_widget_grab_focus (widget);
          priv->button = event->button;

          if (ctk_editable_get_editable (CTK_EDITABLE (widget))) {
            ctk_spin_button_update (spin);

            if (event->button == GDK_BUTTON_PRIMARY)
              start_spinning (spin, event->window, ctk_adjustment_get_step_increment (priv->adjustment));
            else if (event->button == GDK_BUTTON_MIDDLE)
              start_spinning (spin, event->window, ctk_adjustment_get_page_increment (priv->adjustment));
            else
              priv->click_child = event->window;
          } else
            ctk_widget_error_bell (widget);

          return TRUE;
        }
      else
        {
          return CTK_WIDGET_CLASS (ctk_spin_button_parent_class)->button_press_event (widget, event);
        }
    }
  return FALSE;
}

static gint
ctk_spin_button_button_release (CtkWidget      *widget,
                                GdkEventButton *event)
{
  CtkSpinButton *spin = CTK_SPIN_BUTTON (widget);
  CtkSpinButtonPrivate *priv = spin->priv;

  if (event->button == priv->button)
    {
      GdkWindow *click_child = priv->click_child;

      ctk_spin_button_stop_spinning (spin);

      if (event->button == GDK_BUTTON_SECONDARY)
        {
          gdouble diff;

          if (event->window == priv->down_panel &&
              click_child == event->window)
            {
              diff = ctk_adjustment_get_value (priv->adjustment) - ctk_adjustment_get_lower (priv->adjustment);
              if (diff > EPSILON)
                ctk_spin_button_real_spin (spin, -diff);
            }
          else if (event->window == priv->up_panel &&
                   click_child == event->window)
            {
              diff = ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_value (priv->adjustment);
              if (diff > EPSILON)
                ctk_spin_button_real_spin (spin, diff);
            }
        }

      update_node_state (spin);
      ctk_widget_queue_draw (CTK_WIDGET (spin));

      return TRUE;
    }
  else
    {
      return CTK_WIDGET_CLASS (ctk_spin_button_parent_class)->button_release_event (widget, event);
    }
}

static gint
ctk_spin_button_motion_notify (CtkWidget      *widget,
                               GdkEventMotion *event)
{
  CtkSpinButton *spin = CTK_SPIN_BUTTON (widget);
  CtkSpinButtonPrivate *priv = spin->priv;

  if (priv->button)
    return FALSE;

  if (event->window == priv->down_panel ||
      event->window == priv->up_panel)
    {
      gdk_event_request_motions (event);

      priv->in_child = event->window;
      ctk_widget_queue_draw (widget);

      return FALSE;
    }

  if (ctk_gesture_is_recognized (priv->swipe_gesture))
    return TRUE;

  return CTK_WIDGET_CLASS (ctk_spin_button_parent_class)->motion_notify_event (widget, event);
}

static gint
ctk_spin_button_timer (CtkSpinButton *spin_button)
{
  CtkSpinButtonPrivate *priv = spin_button->priv;
  gboolean retval = FALSE;

  if (priv->timer)
    {
      if (priv->click_child == priv->up_panel)
        ctk_spin_button_real_spin (spin_button, priv->timer_step);
      else
        ctk_spin_button_real_spin (spin_button, -priv->timer_step);

      if (priv->need_timer)
        {
          priv->need_timer = FALSE;
          priv->timer = gdk_threads_add_timeout (TIMEOUT_REPEAT,
                                              (GSourceFunc) ctk_spin_button_timer,
                                              (gpointer) spin_button);
          g_source_set_name_by_id (priv->timer, "[ctk+] ctk_spin_button_timer");
        }
      else
        {
          if (priv->climb_rate > 0.0 && priv->timer_step
              < ctk_adjustment_get_page_increment (priv->adjustment))
            {
              if (priv->timer_calls < MAX_TIMER_CALLS)
                priv->timer_calls++;
              else
                {
                  priv->timer_calls = 0;
                  priv->timer_step += priv->climb_rate;
                }
            }
          retval = TRUE;
        }
    }

  return retval;
}

static void
ctk_spin_button_value_changed (CtkAdjustment *adjustment,
                               CtkSpinButton *spin_button)
{
  gboolean return_val;

  g_return_if_fail (CTK_IS_ADJUSTMENT (adjustment));

  return_val = FALSE;
  g_signal_emit (spin_button, spinbutton_signals[OUTPUT], 0, &return_val);
  if (return_val == FALSE)
    ctk_spin_button_default_output (spin_button);

  g_signal_emit (spin_button, spinbutton_signals[VALUE_CHANGED], 0);

  update_node_state (spin_button);

  ctk_widget_queue_draw (CTK_WIDGET (spin_button));

  g_object_notify (G_OBJECT (spin_button), "value");
}

static void
ctk_spin_button_real_change_value (CtkSpinButton *spin,
                                   CtkScrollType  scroll)
{
  CtkSpinButtonPrivate *priv = spin->priv;
  gdouble old_value;

  if (!ctk_editable_get_editable (CTK_EDITABLE (spin)))
    {
      ctk_widget_error_bell (CTK_WIDGET (spin));
      return;
    }

  /* When the key binding is activated, there may be an outstanding
   * value, so we first have to commit what is currently written in
   * the spin buttons text entry. See #106574
   */
  ctk_spin_button_update (spin);

  old_value = ctk_adjustment_get_value (priv->adjustment);

  switch (scroll)
    {
    case CTK_SCROLL_STEP_BACKWARD:
    case CTK_SCROLL_STEP_DOWN:
    case CTK_SCROLL_STEP_LEFT:
      ctk_spin_button_real_spin (spin, -priv->timer_step);

      if (priv->climb_rate > 0.0 && priv->timer_step
          < ctk_adjustment_get_page_increment (priv->adjustment))
        {
          if (priv->timer_calls < MAX_TIMER_CALLS)
            priv->timer_calls++;
          else
            {
              priv->timer_calls = 0;
              priv->timer_step += priv->climb_rate;
            }
        }
      break;

    case CTK_SCROLL_STEP_FORWARD:
    case CTK_SCROLL_STEP_UP:
    case CTK_SCROLL_STEP_RIGHT:
      ctk_spin_button_real_spin (spin, priv->timer_step);

      if (priv->climb_rate > 0.0 && priv->timer_step
          < ctk_adjustment_get_page_increment (priv->adjustment))
        {
          if (priv->timer_calls < MAX_TIMER_CALLS)
            priv->timer_calls++;
          else
            {
              priv->timer_calls = 0;
              priv->timer_step += priv->climb_rate;
            }
        }
      break;

    case CTK_SCROLL_PAGE_BACKWARD:
    case CTK_SCROLL_PAGE_DOWN:
    case CTK_SCROLL_PAGE_LEFT:
      ctk_spin_button_real_spin (spin, -ctk_adjustment_get_page_increment (priv->adjustment));
      break;

    case CTK_SCROLL_PAGE_FORWARD:
    case CTK_SCROLL_PAGE_UP:
    case CTK_SCROLL_PAGE_RIGHT:
      ctk_spin_button_real_spin (spin, ctk_adjustment_get_page_increment (priv->adjustment));
      break;

    case CTK_SCROLL_START:
      {
        gdouble diff = ctk_adjustment_get_value (priv->adjustment) - ctk_adjustment_get_lower (priv->adjustment);
        if (diff > EPSILON)
          ctk_spin_button_real_spin (spin, -diff);
        break;
      }

    case CTK_SCROLL_END:
      {
        gdouble diff = ctk_adjustment_get_upper (priv->adjustment) - ctk_adjustment_get_value (priv->adjustment);
        if (diff > EPSILON)
          ctk_spin_button_real_spin (spin, diff);
        break;
      }

    default:
      g_warning ("Invalid scroll type %d for CtkSpinButton::change-value", scroll);
      break;
    }

  ctk_spin_button_update (spin);

  if (ctk_adjustment_get_value (priv->adjustment) == old_value)
    ctk_widget_error_bell (CTK_WIDGET (spin));
}

static gint
ctk_spin_button_key_release (CtkWidget   *widget,
                             GdkEventKey *event)
{
  CtkSpinButton *spin = CTK_SPIN_BUTTON (widget);
  CtkSpinButtonPrivate *priv = spin->priv;

  /* We only get a release at the end of a key repeat run, so reset the timer_step */
  priv->timer_step = ctk_adjustment_get_step_increment (priv->adjustment);
  priv->timer_calls = 0;

  return TRUE;
}

static void
ctk_spin_button_snap (CtkSpinButton *spin_button,
                      gdouble        val)
{
  CtkSpinButtonPrivate *priv = spin_button->priv;
  gdouble inc;
  gdouble tmp;

  inc = ctk_adjustment_get_step_increment (priv->adjustment);
  if (inc == 0)
    return;

  tmp = (val - ctk_adjustment_get_lower (priv->adjustment)) / inc;
  if (tmp - floor (tmp) < ceil (tmp) - tmp)
    val = ctk_adjustment_get_lower (priv->adjustment) + floor (tmp) * inc;
  else
    val = ctk_adjustment_get_lower (priv->adjustment) + ceil (tmp) * inc;

  ctk_spin_button_set_value (spin_button, val);
}

static void
ctk_spin_button_activate (CtkEntry *entry)
{
  if (ctk_editable_get_editable (CTK_EDITABLE (entry)))
    ctk_spin_button_update (CTK_SPIN_BUTTON (entry));

  /* Chain up so that entry->activates_default is honored */
  CTK_ENTRY_CLASS (ctk_spin_button_parent_class)->activate (entry);
}

static void
ctk_spin_button_insert_text (CtkEditable *editable,
                             const gchar *new_text,
                             gint         new_text_length,
                             gint        *position)
{
  CtkEntry *entry = CTK_ENTRY (editable);
  CtkSpinButton *spin = CTK_SPIN_BUTTON (editable);
  CtkSpinButtonPrivate *priv = spin->priv;
  CtkEditableInterface *parent_editable_iface;

  parent_editable_iface = g_type_interface_peek (ctk_spin_button_parent_class,
                                                 CTK_TYPE_EDITABLE);

  if (priv->numeric)
    {
      struct lconv *lc;
      gboolean sign;
      gint dotpos = -1;
      gint i;
      guint32 pos_sign;
      guint32 neg_sign;
      gint entry_length;
      const gchar *entry_text;

      entry_length = ctk_entry_get_text_length (entry);
      entry_text = ctk_entry_get_text (entry);

      lc = localeconv ();

      if (*(lc->negative_sign))
        neg_sign = *(lc->negative_sign);
      else
        neg_sign = '-';

      if (*(lc->positive_sign))
        pos_sign = *(lc->positive_sign);
      else
        pos_sign = '+';

#ifdef G_OS_WIN32
      /* Workaround for bug caused by some Windows application messing
       * up the positive sign of the current locale, more specifically
       * HKEY_CURRENT_USER\Control Panel\International\sPositiveSign.
       * See bug #330743 and for instance
       * http://www.msnewsgroups.net/group/microsoft.public.dotnet.languages.csharp/topic36024.aspx
       *
       * I don't know if the positive sign always gets bogusly set to
       * a digit when the above Registry value is corrupted as
       * described. (In my test case, it got set to "8", and in the
       * bug report above it presumably was set ot "0".) Probably it
       * might get set to almost anything? So how to distinguish a
       * bogus value from some correct one for some locale? That is
       * probably hard, but at least we should filter out the
       * digits...
       */
      if (pos_sign >= '0' && pos_sign <= '9')
        pos_sign = '+';
#endif

      for (sign=0, i=0; i<entry_length; i++)
        if ((entry_text[i] == neg_sign) ||
            (entry_text[i] == pos_sign))
          {
            sign = 1;
            break;
          }

      if (sign && !(*position))
        return;

      for (dotpos=-1, i=0; i<entry_length; i++)
        if (entry_text[i] == *(lc->decimal_point))
          {
            dotpos = i;
            break;
          }

      if (dotpos > -1 && *position > dotpos &&
          (gint)priv->digits - entry_length
            + dotpos - new_text_length + 1 < 0)
        return;

      for (i = 0; i < new_text_length; i++)
        {
          if (new_text[i] == neg_sign || new_text[i] == pos_sign)
            {
              if (sign || (*position) || i)
                return;
              sign = TRUE;
            }
          else if (new_text[i] == *(lc->decimal_point))
            {
              if (!priv->digits || dotpos > -1 ||
                  (new_text_length - 1 - i + entry_length
                    - *position > (gint)priv->digits))
                return;
              dotpos = *position + i;
            }
          else if (new_text[i] < 0x30 || new_text[i] > 0x39)
            return;
        }
    }

  parent_editable_iface->insert_text (editable, new_text,
                                      new_text_length, position);
}

static void
ctk_spin_button_real_spin (CtkSpinButton *spin_button,
                           gdouble        increment)
{
  CtkSpinButtonPrivate *priv = spin_button->priv;
  CtkAdjustment *adjustment;
  gdouble new_value = 0.0;
  gboolean wrapped = FALSE;

  adjustment = priv->adjustment;

  new_value = ctk_adjustment_get_value (adjustment) + increment;

  if (increment > 0)
    {
      if (priv->wrap)
        {
          if (fabs (ctk_adjustment_get_value (adjustment) - ctk_adjustment_get_upper (adjustment)) < EPSILON)
            {
              new_value = ctk_adjustment_get_lower (adjustment);
              wrapped = TRUE;
            }
          else if (new_value > ctk_adjustment_get_upper (adjustment))
            new_value = ctk_adjustment_get_upper (adjustment);
        }
      else
        new_value = MIN (new_value, ctk_adjustment_get_upper (adjustment));
    }
  else if (increment < 0)
    {
      if (priv->wrap)
        {
          if (fabs (ctk_adjustment_get_value (adjustment) - ctk_adjustment_get_lower (adjustment)) < EPSILON)
            {
              new_value = ctk_adjustment_get_upper (adjustment);
              wrapped = TRUE;
            }
          else if (new_value < ctk_adjustment_get_lower (adjustment))
            new_value = ctk_adjustment_get_lower (adjustment);
        }
      else
        new_value = MAX (new_value, ctk_adjustment_get_lower (adjustment));
    }

  if (fabs (new_value - ctk_adjustment_get_value (adjustment)) > EPSILON)
    ctk_adjustment_set_value (adjustment, new_value);

  if (wrapped)
    g_signal_emit (spin_button, spinbutton_signals[WRAPPED], 0);

  ctk_widget_queue_draw (CTK_WIDGET (spin_button));
}

static gint
ctk_spin_button_default_input (CtkSpinButton *spin_button,
                               gdouble       *new_val)
{
  gchar *err = NULL;

  *new_val = g_strtod (ctk_entry_get_text (CTK_ENTRY (spin_button)), &err);
  if (*err)
    return CTK_INPUT_ERROR;
  else
    return FALSE;
}

static void
ctk_spin_button_default_output (CtkSpinButton *spin_button)
{
  CtkSpinButtonPrivate *priv = spin_button->priv;
  gchar *buf = ctk_spin_button_format_for_value (spin_button,
                                                 ctk_adjustment_get_value (priv->adjustment));

  if (strcmp (buf, ctk_entry_get_text (CTK_ENTRY (spin_button))))
    ctk_entry_set_text (CTK_ENTRY (spin_button), buf);

  g_free (buf);
}


/***********************************************************
 ***********************************************************
 ***                  Public interface                   ***
 ***********************************************************
 ***********************************************************/


/**
 * ctk_spin_button_configure:
 * @spin_button: a #CtkSpinButton
 * @adjustment: (nullable): a #CtkAdjustment to replace the spin button’s
 *     existing adjustment, or %NULL to leave its current adjustment unchanged
 * @climb_rate: the new climb rate
 * @digits: the number of decimal places to display in the spin button
 *
 * Changes the properties of an existing spin button. The adjustment,
 * climb rate, and number of decimal places are updated accordingly.
 */
void
ctk_spin_button_configure (CtkSpinButton *spin_button,
                           CtkAdjustment *adjustment,
                           gdouble        climb_rate,
                           guint          digits)
{
  CtkSpinButtonPrivate *priv;

  g_return_if_fail (CTK_IS_SPIN_BUTTON (spin_button));

  priv = spin_button->priv;

  if (!adjustment)
    adjustment = priv->adjustment;

  g_object_freeze_notify (G_OBJECT (spin_button));

  if (priv->adjustment != adjustment)
    {
      ctk_spin_button_unset_adjustment (spin_button);

      priv->adjustment = adjustment;
      g_object_ref_sink (adjustment);
      g_signal_connect (adjustment, "value-changed",
                        G_CALLBACK (ctk_spin_button_value_changed),
                        spin_button);
      g_signal_connect (adjustment, "changed",
                        G_CALLBACK (adjustment_changed_cb),
                        spin_button);
      priv->timer_step = ctk_adjustment_get_step_increment (priv->adjustment);

      g_object_notify (G_OBJECT (spin_button), "adjustment");
      ctk_widget_queue_resize (CTK_WIDGET (spin_button));
    }

  if (priv->digits != digits)
    {
      priv->digits = digits;
      g_object_notify (G_OBJECT (spin_button), "digits");
    }

  if (priv->climb_rate != climb_rate)
    {
      priv->climb_rate = climb_rate;
      g_object_notify (G_OBJECT (spin_button), "climb-rate");
    }

  g_object_thaw_notify (G_OBJECT (spin_button));

  ctk_spin_button_value_changed (adjustment, spin_button);
}

/**
 * ctk_spin_button_new:
 * @adjustment: (allow-none): the #CtkAdjustment object that this spin
 *     button should use, or %NULL
 * @climb_rate: specifies by how much the rate of change in the value will
 *     accelerate if you continue to hold down an up/down button or arrow key
 * @digits: the number of decimal places to display
 *
 * Creates a new #CtkSpinButton.
 *
 * Returns: The new spin button as a #CtkWidget
 */
CtkWidget *
ctk_spin_button_new (CtkAdjustment *adjustment,
                     gdouble        climb_rate,
                     guint          digits)
{
  CtkSpinButton *spin;

  if (adjustment)
    g_return_val_if_fail (CTK_IS_ADJUSTMENT (adjustment), NULL);

  spin = g_object_new (CTK_TYPE_SPIN_BUTTON, NULL);

  ctk_spin_button_configure (spin, adjustment, climb_rate, digits);

  return CTK_WIDGET (spin);
}

/**
 * ctk_spin_button_new_with_range:
 * @min: Minimum allowable value
 * @max: Maximum allowable value
 * @step: Increment added or subtracted by spinning the widget
 *
 * This is a convenience constructor that allows creation of a numeric
 * #CtkSpinButton without manually creating an adjustment. The value is
 * initially set to the minimum value and a page increment of 10 * @step
 * is the default. The precision of the spin button is equivalent to the
 * precision of @step.
 *
 * Note that the way in which the precision is derived works best if @step
 * is a power of ten. If the resulting precision is not suitable for your
 * needs, use ctk_spin_button_set_digits() to correct it.
 *
 * Returns: The new spin button as a #CtkWidget
 */
CtkWidget *
ctk_spin_button_new_with_range (gdouble min,
                                gdouble max,
                                gdouble step)
{
  CtkAdjustment *adjustment;
  CtkSpinButton *spin;
  gint digits;

  g_return_val_if_fail (min <= max, NULL);
  g_return_val_if_fail (step != 0.0, NULL);

  spin = g_object_new (CTK_TYPE_SPIN_BUTTON, NULL);

  adjustment = ctk_adjustment_new (min, min, max, step, 10 * step, 0);

  if (fabs (step) >= 1.0 || step == 0.0)
    digits = 0;
  else {
    digits = abs ((gint) floor (log10 (fabs (step))));
    if (digits > MAX_DIGITS)
      digits = MAX_DIGITS;
  }

  ctk_spin_button_configure (spin, adjustment, step, digits);

  ctk_spin_button_set_numeric (spin, TRUE);

  return CTK_WIDGET (spin);
}

/**
 * ctk_spin_button_set_adjustment:
 * @spin_button: a #CtkSpinButton
 * @adjustment: a #CtkAdjustment to replace the existing adjustment
 *
 * Replaces the #CtkAdjustment associated with @spin_button.
 */
void
ctk_spin_button_set_adjustment (CtkSpinButton *spin_button,
                                CtkAdjustment *adjustment)
{
  CtkSpinButtonPrivate *priv;

  g_return_if_fail (CTK_IS_SPIN_BUTTON (spin_button));

  priv = spin_button->priv;

  if (!adjustment)
    adjustment = ctk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  ctk_spin_button_configure (spin_button,
                             adjustment,
                             priv->climb_rate,
                             priv->digits);
}

/**
 * ctk_spin_button_get_adjustment:
 * @spin_button: a #CtkSpinButton
 *
 * Get the adjustment associated with a #CtkSpinButton
 *
 * Returns: (transfer none): the #CtkAdjustment of @spin_button
 **/
CtkAdjustment *
ctk_spin_button_get_adjustment (CtkSpinButton *spin_button)
{
  g_return_val_if_fail (CTK_IS_SPIN_BUTTON (spin_button), NULL);

  return spin_button->priv->adjustment;
}

/**
 * ctk_spin_button_set_digits:
 * @spin_button: a #CtkSpinButton
 * @digits: the number of digits after the decimal point to be displayed for the spin button’s value
 *
 * Set the precision to be displayed by @spin_button. Up to 20 digit precision
 * is allowed.
 **/
void
ctk_spin_button_set_digits (CtkSpinButton *spin_button,
                            guint          digits)
{
  CtkSpinButtonPrivate *priv;

  g_return_if_fail (CTK_IS_SPIN_BUTTON (spin_button));

  priv = spin_button->priv;

  if (priv->digits != digits)
    {
      priv->digits = digits;
      ctk_spin_button_value_changed (priv->adjustment, spin_button);
      g_object_notify (G_OBJECT (spin_button), "digits");

      /* since lower/upper may have changed */
      ctk_widget_queue_resize (CTK_WIDGET (spin_button));
    }
}

/**
 * ctk_spin_button_get_digits:
 * @spin_button: a #CtkSpinButton
 *
 * Fetches the precision of @spin_button. See ctk_spin_button_set_digits().
 *
 * Returns: the current precision
 **/
guint
ctk_spin_button_get_digits (CtkSpinButton *spin_button)
{
  g_return_val_if_fail (CTK_IS_SPIN_BUTTON (spin_button), 0);

  return spin_button->priv->digits;
}

/**
 * ctk_spin_button_set_increments:
 * @spin_button: a #CtkSpinButton
 * @step: increment applied for a button 1 press.
 * @page: increment applied for a button 2 press.
 *
 * Sets the step and page increments for spin_button.  This affects how
 * quickly the value changes when the spin button’s arrows are activated.
 **/
void
ctk_spin_button_set_increments (CtkSpinButton *spin_button,
                                gdouble        step,
                                gdouble        page)
{
  CtkSpinButtonPrivate *priv;

  g_return_if_fail (CTK_IS_SPIN_BUTTON (spin_button));

  priv = spin_button->priv;

  ctk_adjustment_configure (priv->adjustment,
                            ctk_adjustment_get_value (priv->adjustment),
                            ctk_adjustment_get_lower (priv->adjustment),
                            ctk_adjustment_get_upper (priv->adjustment),
                            step,
                            page,
                            ctk_adjustment_get_page_size (priv->adjustment));
}

/**
 * ctk_spin_button_get_increments:
 * @spin_button: a #CtkSpinButton
 * @step: (out) (allow-none): location to store step increment, or %NULL
 * @page: (out) (allow-none): location to store page increment, or %NULL
 *
 * Gets the current step and page the increments used by @spin_button. See
 * ctk_spin_button_set_increments().
 **/
void
ctk_spin_button_get_increments (CtkSpinButton *spin_button,
                                gdouble       *step,
                                gdouble       *page)
{
  CtkSpinButtonPrivate *priv;

  g_return_if_fail (CTK_IS_SPIN_BUTTON (spin_button));

  priv = spin_button->priv;

  if (step)
    *step = ctk_adjustment_get_step_increment (priv->adjustment);
  if (page)
    *page = ctk_adjustment_get_page_increment (priv->adjustment);
}

/**
 * ctk_spin_button_set_range:
 * @spin_button: a #CtkSpinButton
 * @min: minimum allowable value
 * @max: maximum allowable value
 *
 * Sets the minimum and maximum allowable values for @spin_button.
 *
 * If the current value is outside this range, it will be adjusted
 * to fit within the range, otherwise it will remain unchanged.
 */
void
ctk_spin_button_set_range (CtkSpinButton *spin_button,
                           gdouble        min,
                           gdouble        max)
{
  CtkAdjustment *adjustment;

  g_return_if_fail (CTK_IS_SPIN_BUTTON (spin_button));

  adjustment = spin_button->priv->adjustment;

  ctk_adjustment_configure (adjustment,
                            CLAMP (ctk_adjustment_get_value (adjustment), min, max),
                            min,
                            max,
                            ctk_adjustment_get_step_increment (adjustment),
                            ctk_adjustment_get_page_increment (adjustment),
                            ctk_adjustment_get_page_size (adjustment));
}

/**
 * ctk_spin_button_get_range:
 * @spin_button: a #CtkSpinButton
 * @min: (out) (allow-none): location to store minimum allowed value, or %NULL
 * @max: (out) (allow-none): location to store maximum allowed value, or %NULL
 *
 * Gets the range allowed for @spin_button.
 * See ctk_spin_button_set_range().
 */
void
ctk_spin_button_get_range (CtkSpinButton *spin_button,
                           gdouble       *min,
                           gdouble       *max)
{
  CtkSpinButtonPrivate *priv;

  g_return_if_fail (CTK_IS_SPIN_BUTTON (spin_button));

  priv = spin_button->priv;

  if (min)
    *min = ctk_adjustment_get_lower (priv->adjustment);
  if (max)
    *max = ctk_adjustment_get_upper (priv->adjustment);
}

/**
 * ctk_spin_button_get_value:
 * @spin_button: a #CtkSpinButton
 *
 * Get the value in the @spin_button.
 *
 * Returns: the value of @spin_button
 */
gdouble
ctk_spin_button_get_value (CtkSpinButton *spin_button)
{
  g_return_val_if_fail (CTK_IS_SPIN_BUTTON (spin_button), 0.0);

  return ctk_adjustment_get_value (spin_button->priv->adjustment);
}

/**
 * ctk_spin_button_get_value_as_int:
 * @spin_button: a #CtkSpinButton
 *
 * Get the value @spin_button represented as an integer.
 *
 * Returns: the value of @spin_button
 */
gint
ctk_spin_button_get_value_as_int (CtkSpinButton *spin_button)
{
  CtkSpinButtonPrivate *priv;
  gdouble val;

  g_return_val_if_fail (CTK_IS_SPIN_BUTTON (spin_button), 0);

  priv = spin_button->priv;

  val = ctk_adjustment_get_value (priv->adjustment);
  if (val - floor (val) < ceil (val) - val)
    return floor (val);
  else
    return ceil (val);
}

/**
 * ctk_spin_button_set_value:
 * @spin_button: a #CtkSpinButton
 * @value: the new value
 *
 * Sets the value of @spin_button.
 */
void
ctk_spin_button_set_value (CtkSpinButton *spin_button,
                           gdouble        value)
{
  CtkSpinButtonPrivate *priv;

  g_return_if_fail (CTK_IS_SPIN_BUTTON (spin_button));

  priv = spin_button->priv;

  if (fabs (value - ctk_adjustment_get_value (priv->adjustment)) > EPSILON)
    ctk_adjustment_set_value (priv->adjustment, value);
  else
    {
      gint return_val = FALSE;
      g_signal_emit (spin_button, spinbutton_signals[OUTPUT], 0, &return_val);
      if (!return_val)
        ctk_spin_button_default_output (spin_button);
    }
}

/**
 * ctk_spin_button_set_update_policy:
 * @spin_button: a #CtkSpinButton
 * @policy: a #CtkSpinButtonUpdatePolicy value
 *
 * Sets the update behavior of a spin button.
 * This determines whether the spin button is always updated
 * or only when a valid value is set.
 */
void
ctk_spin_button_set_update_policy (CtkSpinButton             *spin_button,
                                   CtkSpinButtonUpdatePolicy  policy)
{
  CtkSpinButtonPrivate *priv;

  g_return_if_fail (CTK_IS_SPIN_BUTTON (spin_button));

  priv = spin_button->priv;

  if (priv->update_policy != policy)
    {
      priv->update_policy = policy;
      g_object_notify (G_OBJECT (spin_button), "update-policy");
    }
}

/**
 * ctk_spin_button_get_update_policy:
 * @spin_button: a #CtkSpinButton
 *
 * Gets the update behavior of a spin button.
 * See ctk_spin_button_set_update_policy().
 *
 * Returns: the current update policy
 */
CtkSpinButtonUpdatePolicy
ctk_spin_button_get_update_policy (CtkSpinButton *spin_button)
{
  g_return_val_if_fail (CTK_IS_SPIN_BUTTON (spin_button), CTK_UPDATE_ALWAYS);

  return spin_button->priv->update_policy;
}

/**
 * ctk_spin_button_set_numeric:
 * @spin_button: a #CtkSpinButton
 * @numeric: flag indicating if only numeric entry is allowed
 *
 * Sets the flag that determines if non-numeric text can be typed
 * into the spin button.
 */
void
ctk_spin_button_set_numeric (CtkSpinButton *spin_button,
                             gboolean       numeric)
{
  CtkSpinButtonPrivate *priv;

  g_return_if_fail (CTK_IS_SPIN_BUTTON (spin_button));

  priv = spin_button->priv;

  numeric = numeric != FALSE;

  if (priv->numeric != numeric)
    {
       priv->numeric = numeric;
       g_object_notify (G_OBJECT (spin_button), "numeric");
    }
}

/**
 * ctk_spin_button_get_numeric:
 * @spin_button: a #CtkSpinButton
 *
 * Returns whether non-numeric text can be typed into the spin button.
 * See ctk_spin_button_set_numeric().
 *
 * Returns: %TRUE if only numeric text can be entered
 */
gboolean
ctk_spin_button_get_numeric (CtkSpinButton *spin_button)
{
  g_return_val_if_fail (CTK_IS_SPIN_BUTTON (spin_button), FALSE);

  return spin_button->priv->numeric;
}

/**
 * ctk_spin_button_set_wrap:
 * @spin_button: a #CtkSpinButton
 * @wrap: a flag indicating if wrapping behavior is performed
 *
 * Sets the flag that determines if a spin button value wraps
 * around to the opposite limit when the upper or lower limit
 * of the range is exceeded.
 */
void
ctk_spin_button_set_wrap (CtkSpinButton  *spin_button,
                          gboolean        wrap)
{
  CtkSpinButtonPrivate *priv;

  g_return_if_fail (CTK_IS_SPIN_BUTTON (spin_button));

  priv = spin_button->priv;

  wrap = wrap != FALSE;

  if (priv->wrap != wrap)
    {
      priv->wrap = wrap;
      g_object_notify (G_OBJECT (spin_button), "wrap");

      update_node_state (spin_button);
    }
}

/**
 * ctk_spin_button_get_wrap:
 * @spin_button: a #CtkSpinButton
 *
 * Returns whether the spin button’s value wraps around to the
 * opposite limit when the upper or lower limit of the range is
 * exceeded. See ctk_spin_button_set_wrap().
 *
 * Returns: %TRUE if the spin button wraps around
 */
gboolean
ctk_spin_button_get_wrap (CtkSpinButton *spin_button)
{
  g_return_val_if_fail (CTK_IS_SPIN_BUTTON (spin_button), FALSE);

  return spin_button->priv->wrap;
}

/**
 * ctk_spin_button_set_snap_to_ticks:
 * @spin_button: a #CtkSpinButton
 * @snap_to_ticks: a flag indicating if invalid values should be corrected
 *
 * Sets the policy as to whether values are corrected to the
 * nearest step increment when a spin button is activated after
 * providing an invalid value.
 */
void
ctk_spin_button_set_snap_to_ticks (CtkSpinButton *spin_button,
                                   gboolean       snap_to_ticks)
{
  CtkSpinButtonPrivate *priv;
  guint new_val;

  g_return_if_fail (CTK_IS_SPIN_BUTTON (spin_button));

  priv = spin_button->priv;

  new_val = (snap_to_ticks != 0);

  if (new_val != priv->snap_to_ticks)
    {
      priv->snap_to_ticks = new_val;
      if (new_val && ctk_editable_get_editable (CTK_EDITABLE (spin_button)))
        ctk_spin_button_update (spin_button);

      g_object_notify (G_OBJECT (spin_button), "snap-to-ticks");
    }
}

/**
 * ctk_spin_button_get_snap_to_ticks:
 * @spin_button: a #CtkSpinButton
 *
 * Returns whether the values are corrected to the nearest step.
 * See ctk_spin_button_set_snap_to_ticks().
 *
 * Returns: %TRUE if values are snapped to the nearest step
 */
gboolean
ctk_spin_button_get_snap_to_ticks (CtkSpinButton *spin_button)
{
  g_return_val_if_fail (CTK_IS_SPIN_BUTTON (spin_button), FALSE);

  return spin_button->priv->snap_to_ticks;
}

/**
 * ctk_spin_button_spin:
 * @spin_button: a #CtkSpinButton
 * @direction: a #CtkSpinType indicating the direction to spin
 * @increment: step increment to apply in the specified direction
 *
 * Increment or decrement a spin button’s value in a specified
 * direction by a specified amount.
 */
void
ctk_spin_button_spin (CtkSpinButton *spin_button,
                      CtkSpinType    direction,
                      gdouble        increment)
{
  CtkSpinButtonPrivate *priv;
  CtkAdjustment *adjustment;
  gdouble diff;

  g_return_if_fail (CTK_IS_SPIN_BUTTON (spin_button));

  priv = spin_button->priv;

  adjustment = priv->adjustment;

  /* for compatibility with the 1.0.x version of this function */
  if (increment != 0 && increment != ctk_adjustment_get_step_increment (adjustment) &&
      (direction == CTK_SPIN_STEP_FORWARD ||
       direction == CTK_SPIN_STEP_BACKWARD))
    {
      if (direction == CTK_SPIN_STEP_BACKWARD && increment > 0)
        increment = -increment;
      direction = CTK_SPIN_USER_DEFINED;
    }

  switch (direction)
    {
    case CTK_SPIN_STEP_FORWARD:

      ctk_spin_button_real_spin (spin_button, ctk_adjustment_get_step_increment (adjustment));
      break;

    case CTK_SPIN_STEP_BACKWARD:

      ctk_spin_button_real_spin (spin_button, -ctk_adjustment_get_step_increment (adjustment));
      break;

    case CTK_SPIN_PAGE_FORWARD:

      ctk_spin_button_real_spin (spin_button, ctk_adjustment_get_page_increment (adjustment));
      break;

    case CTK_SPIN_PAGE_BACKWARD:

      ctk_spin_button_real_spin (spin_button, -ctk_adjustment_get_page_increment (adjustment));
      break;

    case CTK_SPIN_HOME:

      diff = ctk_adjustment_get_value (adjustment) - ctk_adjustment_get_lower (adjustment);
      if (diff > EPSILON)
        ctk_spin_button_real_spin (spin_button, -diff);
      break;

    case CTK_SPIN_END:

      diff = ctk_adjustment_get_upper (adjustment) - ctk_adjustment_get_value (adjustment);
      if (diff > EPSILON)
        ctk_spin_button_real_spin (spin_button, diff);
      break;

    case CTK_SPIN_USER_DEFINED:

      if (increment != 0)
        ctk_spin_button_real_spin (spin_button, increment);
      break;

    default:
      break;
    }
}

/**
 * ctk_spin_button_update:
 * @spin_button: a #CtkSpinButton
 *
 * Manually force an update of the spin button.
 */
void
ctk_spin_button_update (CtkSpinButton *spin_button)
{
  CtkSpinButtonPrivate *priv;
  gdouble val;
  gint error = 0;
  gint return_val;

  g_return_if_fail (CTK_IS_SPIN_BUTTON (spin_button));

  priv = spin_button->priv;

  return_val = FALSE;
  g_signal_emit (spin_button, spinbutton_signals[INPUT], 0, &val, &return_val);
  if (return_val == FALSE)
    {
      return_val = ctk_spin_button_default_input (spin_button, &val);
      error = (return_val == CTK_INPUT_ERROR);
    }
  else if (return_val == CTK_INPUT_ERROR)
    error = 1;

  ctk_widget_queue_draw (CTK_WIDGET (spin_button));

  if (priv->update_policy == CTK_UPDATE_ALWAYS)
    {
      if (val < ctk_adjustment_get_lower (priv->adjustment))
        val = ctk_adjustment_get_lower (priv->adjustment);
      else if (val > ctk_adjustment_get_upper (priv->adjustment))
        val = ctk_adjustment_get_upper (priv->adjustment);
    }
  else if ((priv->update_policy == CTK_UPDATE_IF_VALID) &&
           (error ||
            val < ctk_adjustment_get_lower (priv->adjustment) ||
            val > ctk_adjustment_get_upper (priv->adjustment)))
    {
      ctk_spin_button_value_changed (priv->adjustment, spin_button);
      return;
    }

  if (priv->snap_to_ticks)
    ctk_spin_button_snap (spin_button, val);
  else
    ctk_spin_button_set_value (spin_button, val);
}

void
_ctk_spin_button_get_panels (CtkSpinButton  *spin_button,
                             GdkWindow     **down_panel,
                             GdkWindow     **up_panel)
{
  if (down_panel != NULL)
    *down_panel = spin_button->priv->down_panel;

  if (up_panel != NULL)
    *up_panel = spin_button->priv->up_panel;
}

static void
ctk_spin_button_direction_changed (CtkWidget        *widget,
                                   CtkTextDirection  previous_dir)
{
  update_node_ordering (CTK_SPIN_BUTTON (widget));

  CTK_WIDGET_CLASS (ctk_spin_button_parent_class)->direction_changed (widget, previous_dir);
}
