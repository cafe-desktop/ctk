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
 * Modified by the CTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

/**
 * SECTION:ctkbutton
 * @Short_description: A widget that emits a signal when clicked on
 * @Title: CtkButton
 *
 * The #CtkButton widget is generally used to trigger a callback function that is
 * called when the button is pressed.  The various signals and how to use them
 * are outlined below.
 *
 * The #CtkButton widget can hold any valid child widget.  That is, it can hold
 * almost any other standard #CtkWidget.  The most commonly used child is the
 * #CtkLabel.
 *
 * # CSS nodes
 *
 * CtkButton has a single CSS node with name button. The node will get the
 * style classes .image-button or .text-button, if the content is just an
 * image or label, respectively. It may also receive the .flat style class.
 *
 * Other style classes that are commonly used with CtkButton include
 * .suggested-action and .destructive-action. In special cases, buttons
 * can be made round by adding the .circular style class.
 *
 * Button-like widgets like #CtkToggleButton, #CtkMenuButton, #CtkVolumeButton,
 * #CtkLockButton, #CtkColorButton, #CtkFontButton or #CtkFileChooserButton use
 * style classes such as .toggle, .popup, .scale, .lock, .color, .font, .file
 * to differentiate themselves from a plain CtkButton.
 */

#include "config.h"

#include "ctkbutton.h"
#include "ctkbuttonprivate.h"

#include <string.h>
#include "ctkalignment.h"
#include "ctklabel.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkimage.h"
#include "ctkbox.h"
#include "ctkstock.h"
#include "ctkactivatable.h"
#include "ctksizerequest.h"
#include "ctktypebuiltins.h"
#include "ctkwidgetprivate.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "a11y/ctkbuttonaccessible.h"
#include "ctkapplicationprivate.h"
#include "ctkactionhelper.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkcontainerprivate.h"

/* Time out before giving up on getting a key release when animating
 * the close button.
 */
#define ACTIVATE_TIMEOUT 250


enum {
  PRESSED,
  RELEASED,
  CLICKED,
  ENTER,
  LEAVE,
  ACTIVATE,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_LABEL,
  PROP_IMAGE,
  PROP_RELIEF,
  PROP_USE_UNDERLINE,
  PROP_USE_STOCK,
  PROP_XALIGN,
  PROP_YALIGN,
  PROP_IMAGE_POSITION,
  PROP_ALWAYS_SHOW_IMAGE,

  /* actionable properties */
  PROP_ACTION_NAME,
  PROP_ACTION_TARGET,

  /* activatable properties */
  PROP_ACTIVATABLE_RELATED_ACTION,
  PROP_ACTIVATABLE_USE_ACTION_APPEARANCE,
  LAST_PROP = PROP_ACTION_NAME
};


static void ctk_button_finalize       (GObject            *object);
static void ctk_button_dispose        (GObject            *object);
static void ctk_button_set_property   (GObject            *object,
                                       guint               prop_id,
                                       const GValue       *value,
                                       GParamSpec         *pspec);
static void ctk_button_get_property   (GObject            *object,
                                       guint               prop_id,
                                       GValue             *value,
                                       GParamSpec         *pspec);
static void ctk_button_screen_changed (CtkWidget          *widget,
				       CdkScreen          *previous_screen);
static void ctk_button_realize (CtkWidget * widget);
static void ctk_button_unrealize (CtkWidget * widget);
static void ctk_button_map (CtkWidget * widget);
static void ctk_button_unmap (CtkWidget * widget);
static void ctk_button_style_updated (CtkWidget * widget);
static void ctk_button_size_allocate (CtkWidget * widget,
				      CtkAllocation * allocation);
static gint ctk_button_draw (CtkWidget * widget, cairo_t *cr);
static gint ctk_button_grab_broken (CtkWidget * widget,
				    CdkEventGrabBroken * event);
static gint ctk_button_key_release (CtkWidget * widget, CdkEventKey * event);
static gint ctk_button_enter_notify (CtkWidget * widget,
				     CdkEventCrossing * event);
static gint ctk_button_leave_notify (CtkWidget * widget,
				     CdkEventCrossing * event);
static void ctk_real_button_pressed (CtkButton * button);
static void ctk_real_button_released (CtkButton * button);
static void ctk_real_button_clicked (CtkButton * button);
static void ctk_real_button_activate  (CtkButton          *button);
static void ctk_button_update_state   (CtkButton          *button);
static void ctk_button_enter_leave    (CtkButton          *button);
static void ctk_button_add            (CtkContainer       *container,
			               CtkWidget          *widget);
static GType ctk_button_child_type    (CtkContainer       *container);
static void ctk_button_finish_activate (CtkButton         *button,
					gboolean           do_it);

static void ctk_button_constructed (GObject *object);
static void ctk_button_construct_child (CtkButton             *button);
static void ctk_button_state_changed   (CtkWidget             *widget,
					CtkStateType           previous_state);
static void ctk_button_grab_notify     (CtkWidget             *widget,
					gboolean               was_grabbed);
static void ctk_button_do_release      (CtkButton             *button,
                                        gboolean               emit_clicked);

static void ctk_button_actionable_iface_init     (CtkActionableInterface *iface);
static void ctk_button_activatable_interface_init(CtkActivatableIface  *iface);
static void ctk_button_update                    (CtkActivatable       *activatable,
				                  CtkAction            *action,
			                          const gchar          *property_name);
static void ctk_button_sync_action_properties    (CtkActivatable       *activatable,
                                                  CtkAction            *action);
static void ctk_button_set_related_action        (CtkButton            *button,
					          CtkAction            *action);
static void ctk_button_set_use_action_appearance (CtkButton            *button,
						  gboolean              use_appearance);

static void ctk_button_get_preferred_width             (CtkWidget           *widget,
                                                        gint                *minimum_size,
                                                        gint                *natural_size);
static void ctk_button_get_preferred_height            (CtkWidget           *widget,
                                                        gint                *minimum_size,
                                                        gint                *natural_size);
static void ctk_button_get_preferred_width_for_height  (CtkWidget           *widget,
                                                        gint                 for_size,
                                                        gint                *minimum_size,
                                                        gint                *natural_size);
static void ctk_button_get_preferred_height_for_width  (CtkWidget           *widget,
                                                        gint                 for_size,
                                                        gint                *minimum_size,
                                                        gint                *natural_size);
static void ctk_button_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
								    gint       width,
								    gint      *minimum_size,
								    gint      *natural_size,
								    gint      *minimum_baseline,
								    gint      *natural_baseline);

static void     ctk_button_measure  (CtkCssGadget        *gadget,
                                     CtkOrientation       orientation,
                                     int                  for_size,
                                     int                 *minimum_size,
                                     int                 *natural_size,
                                     int                 *minimum_baseline,
                                     int                 *natural_baseline,
                                     gpointer             data);
static void     ctk_button_allocate (CtkCssGadget        *gadget,
                                     const CtkAllocation *allocation,
                                     int                  baseline,
                                     CtkAllocation       *out_clip,
                                     gpointer             data);
static gboolean ctk_button_render   (CtkCssGadget        *gadget,
                                     cairo_t             *cr,
                                     int                  x,
                                     int                  y,
                                     int                  width,
                                     int                  height,
                                     gpointer             data);

static GParamSpec *props[LAST_PROP] = { NULL, };
static guint button_signals[LAST_SIGNAL] = { 0 };

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
G_DEFINE_TYPE_WITH_CODE (CtkButton, ctk_button, CTK_TYPE_BIN,
                         G_ADD_PRIVATE (CtkButton)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIONABLE, ctk_button_actionable_iface_init)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIVATABLE,
						ctk_button_activatable_interface_init))
G_GNUC_END_IGNORE_DEPRECATIONS;

static void
ctk_button_class_init (CtkButtonClass *klass)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;

  gobject_class = G_OBJECT_CLASS (klass);
  widget_class = (CtkWidgetClass*) klass;
  container_class = (CtkContainerClass*) klass;
  
  gobject_class->constructed  = ctk_button_constructed;
  gobject_class->dispose      = ctk_button_dispose;
  gobject_class->finalize     = ctk_button_finalize;
  gobject_class->set_property = ctk_button_set_property;
  gobject_class->get_property = ctk_button_get_property;

  widget_class->get_preferred_width = ctk_button_get_preferred_width;
  widget_class->get_preferred_height = ctk_button_get_preferred_height;
  widget_class->get_preferred_width_for_height = ctk_button_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = ctk_button_get_preferred_height_for_width;
  widget_class->get_preferred_height_and_baseline_for_width = ctk_button_get_preferred_height_and_baseline_for_width;
  widget_class->screen_changed = ctk_button_screen_changed;
  widget_class->realize = ctk_button_realize;
  widget_class->unrealize = ctk_button_unrealize;
  widget_class->map = ctk_button_map;
  widget_class->unmap = ctk_button_unmap;
  widget_class->style_updated = ctk_button_style_updated;
  widget_class->size_allocate = ctk_button_size_allocate;
  widget_class->draw = ctk_button_draw;
  widget_class->grab_broken_event = ctk_button_grab_broken;
  widget_class->key_release_event = ctk_button_key_release;
  widget_class->enter_notify_event = ctk_button_enter_notify;
  widget_class->leave_notify_event = ctk_button_leave_notify;
  widget_class->state_changed = ctk_button_state_changed;
  widget_class->grab_notify = ctk_button_grab_notify;

  container_class->child_type = ctk_button_child_type;
  container_class->add = ctk_button_add;
  ctk_container_class_handle_border_width (container_class);

  klass->pressed = ctk_real_button_pressed;
  klass->released = ctk_real_button_released;
  klass->clicked = NULL;
  klass->enter = ctk_button_enter_leave;
  klass->leave = ctk_button_enter_leave;
  klass->activate = ctk_real_button_activate;

  props[PROP_LABEL] =
    g_param_spec_string ("label",
                         P_("Label"),
                         P_("Text of the label widget inside the button, if the button contains a label widget"),
                         NULL,
                         CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);
  
  props[PROP_USE_UNDERLINE] =
    g_param_spec_boolean ("use-underline",
                          P_("Use underline"),
                          P_("If set, an underline in the text indicates the next character should be used for the mnemonic accelerator key"),
                          FALSE,
                          CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);
  
  /**
   * CtkButton:use-stock:
   *
   * Deprecated: 3.10
   */
  props[PROP_USE_STOCK] =
    g_param_spec_boolean ("use-stock",
                          P_("Use stock"),
                          P_("If set, the label is used to pick a stock item instead of being displayed"),
                          FALSE,
                          CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);
  
  props[PROP_RELIEF] =
    g_param_spec_enum ("relief",
                       P_("Border relief"),
                       P_("The border relief style"),
                       CTK_TYPE_RELIEF_STYLE,
                       CTK_RELIEF_NORMAL,
                       CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);
  
  /**
   * CtkButton:xalign:
   *
   * If the child of the button is a #CtkMisc or #CtkAlignment, this property 
   * can be used to control its horizontal alignment. 0.0 is left aligned, 
   * 1.0 is right aligned.
   *
   * Since: 2.4
   *
   * Deprecated: 3.14: Access the child widget directly if you need to control
   * its alignment.
   */
  props[PROP_XALIGN] =
    g_param_spec_float ("xalign",
                        P_("Horizontal alignment for child"),
                        P_("Horizontal position of child in available space. 0.0 is left aligned, 1.0 is right aligned"),
                        0.0, 1.0, 0.5,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  /**
   * CtkButton:yalign:
   *
   * If the child of the button is a #CtkMisc or #CtkAlignment, this property 
   * can be used to control its vertical alignment. 0.0 is top aligned, 
   * 1.0 is bottom aligned.
   *
   * Since: 2.4
   *
   * Deprecated: 3.14: Access the child widget directly if you need to control
   * its alignment.
   */
  props[PROP_YALIGN] =
    g_param_spec_float ("yalign",
                        P_("Vertical alignment for child"),
                        P_("Vertical position of child in available space. 0.0 is top aligned, 1.0 is bottom aligned"),
                        0.0, 1.0, 0.5,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  /**
   * CtkButton:image:
   *
   * The child widget to appear next to the button text.
   *
   * Since: 2.6
   */
  props[PROP_IMAGE] =
    g_param_spec_object ("image",
                         P_("Image widget"),
                         P_("Child widget to appear next to the button text"),
                         CTK_TYPE_WIDGET,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkButton:image-position:
   *
   * The position of the image relative to the text inside the button.
   *
   * Since: 2.10
   */
  props[PROP_IMAGE_POSITION] =
    g_param_spec_enum ("image-position",
                       P_("Image position"),
                       P_("The position of the image relative to the text"),
                       CTK_TYPE_POSITION_TYPE,
                       CTK_POS_LEFT,
                       CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkButton:always-show-image:
   *
   * If %TRUE, the button will ignore the #CtkSettings:ctk-button-images
   * setting and always show the image, if available.
   *
   * Use this property if the button would be useless or hard to use
   * without the image.
   *
   * Since: 3.6
   */
  props[PROP_ALWAYS_SHOW_IMAGE] =
     g_param_spec_boolean ("always-show-image",
                           P_("Always show image"),
                           P_("Whether the image will always be shown"),
                           FALSE,
                           CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, LAST_PROP, props);

  g_object_class_override_property (gobject_class, PROP_ACTION_NAME, "action-name");
  g_object_class_override_property (gobject_class, PROP_ACTION_TARGET, "action-target");

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  g_object_class_override_property (gobject_class, PROP_ACTIVATABLE_RELATED_ACTION, "related-action");
  g_object_class_override_property (gobject_class, PROP_ACTIVATABLE_USE_ACTION_APPEARANCE, "use-action-appearance");
  G_GNUC_END_IGNORE_DEPRECATIONS;

  /**
   * CtkButton::pressed:
   * @button: the object that received the signal
   *
   * Emitted when the button is pressed.
   *
   * Deprecated: 2.8: Use the #CtkWidget::button-press-event signal.
   */ 
  button_signals[PRESSED] =
    g_signal_new (I_("pressed"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkButtonClass, pressed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkButton::released:
   * @button: the object that received the signal
   *
   * Emitted when the button is released.
   *
   * Deprecated: 2.8: Use the #CtkWidget::button-release-event signal.
   */ 
  button_signals[RELEASED] =
    g_signal_new (I_("released"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkButtonClass, released),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkButton::clicked:
   * @button: the object that received the signal
   *
   * Emitted when the button has been activated (pressed and released).
   */ 
  button_signals[CLICKED] =
    g_signal_new (I_("clicked"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkButtonClass, clicked),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkButton::enter:
   * @button: the object that received the signal
   *
   * Emitted when the pointer enters the button.
   *
   * Deprecated: 2.8: Use the #CtkWidget::enter-notify-event signal.
   */ 
  button_signals[ENTER] =
    g_signal_new (I_("enter"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkButtonClass, enter),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkButton::leave:
   * @button: the object that received the signal
   *
   * Emitted when the pointer leaves the button.
   *
   * Deprecated: 2.8: Use the #CtkWidget::leave-notify-event signal.
   */ 
  button_signals[LEAVE] =
    g_signal_new (I_("leave"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkButtonClass, leave),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkButton::activate:
   * @widget: the object which received the signal.
   *
   * The ::activate signal on CtkButton is an action signal and
   * emitting it causes the button to animate press then release. 
   * Applications should never connect to this signal, but use the
   * #CtkButton::clicked signal.
   */
  button_signals[ACTIVATE] =
    g_signal_new (I_("activate"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkButtonClass, activate),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
  widget_class->activate_signal = button_signals[ACTIVATE];

  /**
   * CtkButton:default-border:
   *
   * The "default-border" style property defines the extra space to add
   * around a button that can become the default widget of its window.
   * For more information about default widgets, see ctk_widget_grab_default().
   *
   * Deprecated: 3.14: Use CSS margins and padding instead;
   *     the value of this style property is ignored.
   */

  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("default-border",
							       P_("Default Spacing"),
							       P_("Extra space to add for CTK_CAN_DEFAULT buttons"),
							       CTK_TYPE_BORDER,
							       CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkButton:default-outside-border:
   *
   * The "default-outside-border" style property defines the extra outside
   * space to add around a button that can become the default widget of its
   * window. Extra outside space is always drawn outside the button border.
   * For more information about default widgets, see ctk_widget_grab_default().
   *
   * Deprecated: 3.14: Use CSS margins and padding instead;
   *     the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("default-outside-border",
							       P_("Default Outside Spacing"),
							       P_("Extra space to add for CTK_CAN_DEFAULT buttons that is always drawn outside the border"),
							       CTK_TYPE_BORDER,
							       CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkButton:child-displacement-x:
   *
   * How far in the x direction to move the child when the button is depressed.
   *
   * Deprecated: 3.20: Use CSS margins and padding instead;
   *     the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("child-displacement-x",
							     P_("Child X Displacement"),
							     P_("How far in the x direction to move the child when the button is depressed"),
							     G_MININT,
							     G_MAXINT,
							     0,
							     CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkButton:child-displacement-y:
   *
   * How far in the y direction to move the child when the button is depressed.
   *
   * Deprecated: 3.20: Use CSS margins and padding instead;
   *     the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("child-displacement-y",
							     P_("Child Y Displacement"),
							     P_("How far in the y direction to move the child when the button is depressed"),
							     G_MININT,
							     G_MAXINT,
							     0,
							     CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkButton:displace-focus:
   *
   * Whether the child_displacement_x/child_displacement_y properties
   * should also affect the focus rectangle.
   *
   * Since: 2.6
   *
   * Deprecated: 3.20: Use CSS margins and padding instead;
   *     the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("displace-focus",
								 P_("Displace focus"),
								 P_("Whether the child_displacement_x/_y properties should also affect the focus rectangle"),
								 FALSE,
								 CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkButton:inner-border:
   *
   * Sets the border between the button edges and child.
   *
   * Since: 2.10
   *
   * Deprecated: 3.4: Use the standard border and padding CSS properties;
   *   the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("inner-border",
                                                               P_("Inner Border"),
                                                               P_("Border between button edges and child."),
                                                               CTK_TYPE_BORDER,
                                                               CTK_PARAM_READABLE | G_PARAM_DEPRECATED));

  /**
   * CtkButton::image-spacing:
   *
   * Spacing in pixels between the image and label.
   *
   * Since: 2.10
   *
   * Deprecated: 3.20: Use CSS margins and padding instead.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("image-spacing",
							     P_("Image spacing"),
							     P_("Spacing in pixels between the image and label"),
							     0,
							     G_MAXINT,
							     2,
							     CTK_PARAM_READABLE | G_PARAM_DEPRECATED));

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_BUTTON_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "button");
}

static void
multipress_pressed_cb (CtkGestureMultiPress *gesture,
                       guint                 n_press,
                       gdouble               x,
                       gdouble               y,
                       CtkWidget            *widget)
{
  CtkButton *button = CTK_BUTTON (widget);
  CtkButtonPrivate *priv = button->priv;

  if (ctk_widget_get_focus_on_click (widget) && !ctk_widget_has_focus (widget))
    ctk_widget_grab_focus (widget);

  priv->in_button = TRUE;
  g_signal_emit (button, button_signals[PRESSED], 0);
  ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_CLAIMED);
}

static void
multipress_released_cb (CtkGestureMultiPress *gesture,
                        guint                 n_press,
                        gdouble               x,
                        gdouble               y,
                        CtkWidget            *widget)
{
  CtkButton *button = CTK_BUTTON (widget);
  CtkButtonPrivate *priv = button->priv;
  CdkEventSequence *sequence;

  g_signal_emit (button, button_signals[RELEASED], 0);

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));

  if (sequence)
    {
      priv->in_button = FALSE;
      ctk_button_update_state (button);
    }
}

static void
multipress_gesture_update_cb (CtkGesture       *gesture,
                              CdkEventSequence *sequence,
                              CtkButton        *button)
{
  CtkButtonPrivate *priv = button->priv;
  CtkAllocation allocation;
  gboolean in_button;
  gdouble x, y;

  if (sequence != ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture)))
    return;

  ctk_widget_get_allocation (CTK_WIDGET (button), &allocation);
  ctk_gesture_get_point (gesture, sequence, &x, &y);

  in_button = (x >= 0 && y >= 0 && x < allocation.width && y < allocation.height);

  if (priv->in_button != in_button)
    {
      priv->in_button = in_button;
      ctk_button_update_state (button);
    }
}

static void
multipress_gesture_cancel_cb (CtkGesture       *gesture,
                              CdkEventSequence *sequence,
                              CtkButton        *button)
{
  ctk_button_do_release (button, FALSE);
}

static void
ctk_button_init (CtkButton *button)
{
  CtkButtonPrivate *priv;

  button->priv = ctk_button_get_instance_private (button);
  priv = button->priv;

  ctk_widget_set_can_focus (CTK_WIDGET (button), TRUE);
  ctk_widget_set_receives_default (CTK_WIDGET (button), TRUE);
  ctk_widget_set_has_window (CTK_WIDGET (button), FALSE);

  priv->label_text = NULL;

  priv->constructed = FALSE;
  priv->in_button = FALSE;
  priv->button_down = FALSE;
  priv->use_stock = FALSE;
  priv->use_underline = FALSE;

  priv->xalign = 0.5;
  priv->yalign = 0.5;
  priv->align_set = 0;
  priv->image_is_stock = TRUE;
  priv->image_position = CTK_POS_LEFT;
  priv->use_action_appearance = TRUE;

  priv->gesture = ctk_gesture_multi_press_new (CTK_WIDGET (button));
  ctk_gesture_single_set_touch_only (CTK_GESTURE_SINGLE (priv->gesture), FALSE);
  ctk_gesture_single_set_exclusive (CTK_GESTURE_SINGLE (priv->gesture), TRUE);
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (priv->gesture), CDK_BUTTON_PRIMARY);
  g_signal_connect (priv->gesture, "pressed", G_CALLBACK (multipress_pressed_cb), button);
  g_signal_connect (priv->gesture, "released", G_CALLBACK (multipress_released_cb), button);
  g_signal_connect (priv->gesture, "update", G_CALLBACK (multipress_gesture_update_cb), button);
  g_signal_connect (priv->gesture, "cancel", G_CALLBACK (multipress_gesture_cancel_cb), button);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->gesture), CTK_PHASE_BUBBLE);

  priv->gadget = ctk_css_custom_gadget_new_for_node (ctk_widget_get_css_node (CTK_WIDGET (button)),
                                                     CTK_WIDGET (button),
                                                     ctk_button_measure,
                                                     ctk_button_allocate,
                                                     ctk_button_render,
                                                     NULL,
                                                     NULL);

}

static void
ctk_button_finalize (GObject *object)
{
  CtkButton *button = CTK_BUTTON (object);
  CtkButtonPrivate *priv = button->priv;

  g_clear_pointer (&priv->label_text, g_free);
  g_clear_object (&priv->gesture);
  g_clear_object (&priv->gadget);

  G_OBJECT_CLASS (ctk_button_parent_class)->finalize (object);
}

static void
ctk_button_constructed (GObject *object)
{
  CtkButton *button = CTK_BUTTON (object);
  CtkButtonPrivate *priv = button->priv;

  G_OBJECT_CLASS (ctk_button_parent_class)->constructed (object);

  priv->constructed = TRUE;

  if (priv->label_text != NULL || priv->image != NULL)
    ctk_button_construct_child (button);
}


static GType
ctk_button_child_type  (CtkContainer     *container)
{
  if (!ctk_bin_get_child (CTK_BIN (container)))
    return CTK_TYPE_WIDGET;
  else
    return G_TYPE_NONE;
}

static void
maybe_set_alignment (CtkButton *button,
		     CtkWidget *widget)
{
  CtkButtonPrivate *priv = button->priv;

  if (!priv->align_set)
    return;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (CTK_IS_LABEL (widget))
    g_object_set (widget, "xalign", priv->xalign, "yalign", priv->yalign, NULL);
  else if (CTK_IS_MISC (widget))
    ctk_misc_set_alignment (CTK_MISC (widget), priv->xalign, priv->yalign);
  else if (CTK_IS_ALIGNMENT (widget))
    g_object_set (widget, "xalign", priv->xalign, "yalign", priv->yalign, NULL);
G_GNUC_END_IGNORE_DEPRECATIONS
}

static void
ctk_button_add (CtkContainer *container,
		CtkWidget    *widget)
{
  maybe_set_alignment (CTK_BUTTON (container), widget);

  CTK_CONTAINER_CLASS (ctk_button_parent_class)->add (container, widget);
}

static void
ctk_button_dispose (GObject *object)
{
  CtkButton *button = CTK_BUTTON (object);
  CtkButtonPrivate *priv = button->priv;

  g_clear_object (&priv->action_helper);

  if (priv->action)
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_activatable_do_set_related_action (CTK_ACTIVATABLE (button), NULL);
      G_GNUC_END_IGNORE_DEPRECATIONS;
      priv->action = NULL;
    }
  G_OBJECT_CLASS (ctk_button_parent_class)->dispose (object);
}

static void
ctk_button_set_action_name (CtkActionable *actionable,
                            const gchar   *action_name)
{
  CtkButton *button = CTK_BUTTON (actionable);

  g_return_if_fail (button->priv->action == NULL);

  if (!button->priv->action_helper)
    button->priv->action_helper = ctk_action_helper_new (actionable);

  g_signal_handlers_disconnect_by_func (button, ctk_real_button_clicked, NULL);
  if (action_name)
    g_signal_connect_after (button, "clicked", G_CALLBACK (ctk_real_button_clicked), NULL);

  ctk_action_helper_set_action_name (button->priv->action_helper, action_name);
}

static void
ctk_button_set_action_target_value (CtkActionable *actionable,
                                    GVariant      *action_target)
{
  CtkButton *button = CTK_BUTTON (actionable);

  if (!button->priv->action_helper)
    button->priv->action_helper = ctk_action_helper_new (actionable);

  ctk_action_helper_set_action_target_value (button->priv->action_helper, action_target);
}

static void
ctk_button_set_property (GObject         *object,
                         guint            prop_id,
                         const GValue    *value,
                         GParamSpec      *pspec)
{
  CtkButton *button = CTK_BUTTON (object);
  CtkButtonPrivate *priv = button->priv;

  switch (prop_id)
    {
    case PROP_LABEL:
      ctk_button_set_label (button, g_value_get_string (value));
      break;
    case PROP_IMAGE:
      ctk_button_set_image (button, (CtkWidget *) g_value_get_object (value));
      break;
    case PROP_ALWAYS_SHOW_IMAGE:
      ctk_button_set_always_show_image (button, g_value_get_boolean (value));
      break;
    case PROP_RELIEF:
      ctk_button_set_relief (button, g_value_get_enum (value));
      break;
    case PROP_USE_UNDERLINE:
      ctk_button_set_use_underline (button, g_value_get_boolean (value));
      break;
    case PROP_USE_STOCK:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_button_set_use_stock (button, g_value_get_boolean (value));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case PROP_XALIGN:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_button_set_alignment (button, g_value_get_float (value), priv->yalign);
G_GNUC_END_IGNORE_DEPRECATIONS
      break;
    case PROP_YALIGN:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_button_set_alignment (button, priv->xalign, g_value_get_float (value));
G_GNUC_END_IGNORE_DEPRECATIONS
      break;
    case PROP_IMAGE_POSITION:
      ctk_button_set_image_position (button, g_value_get_enum (value));
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      ctk_button_set_related_action (button, g_value_get_object (value));
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
      ctk_button_set_use_action_appearance (button, g_value_get_boolean (value));
      break;
    case PROP_ACTION_NAME:
      ctk_button_set_action_name (CTK_ACTIONABLE (button), g_value_get_string (value));
      break;
    case PROP_ACTION_TARGET:
      ctk_button_set_action_target_value (CTK_ACTIONABLE (button), g_value_get_variant (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_button_get_property (GObject         *object,
                         guint            prop_id,
                         GValue          *value,
                         GParamSpec      *pspec)
{
  CtkButton *button = CTK_BUTTON (object);
  CtkButtonPrivate *priv = button->priv;

  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, priv->label_text);
      break;
    case PROP_IMAGE:
      g_value_set_object (value, (GObject *)priv->image);
      break;
    case PROP_ALWAYS_SHOW_IMAGE:
      g_value_set_boolean (value, ctk_button_get_always_show_image (button));
      break;
    case PROP_RELIEF:
      g_value_set_enum (value, ctk_button_get_relief (button));
      break;
    case PROP_USE_UNDERLINE:
      g_value_set_boolean (value, priv->use_underline);
      break;
    case PROP_USE_STOCK:
      g_value_set_boolean (value, priv->use_stock);
      break;
    case PROP_XALIGN:
      g_value_set_float (value, priv->xalign);
      break;
    case PROP_YALIGN:
      g_value_set_float (value, priv->yalign);
      break;
    case PROP_IMAGE_POSITION:
      g_value_set_enum (value, priv->image_position);
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      g_value_set_object (value, priv->action);
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
      g_value_set_boolean (value, priv->use_action_appearance);
      break;
    case PROP_ACTION_NAME:
      g_value_set_string (value, ctk_action_helper_get_action_name (priv->action_helper));
      break;
    case PROP_ACTION_TARGET:
      g_value_set_variant (value, ctk_action_helper_get_action_target_value (priv->action_helper));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static const gchar *
ctk_button_get_action_name (CtkActionable *actionable)
{
  CtkButton *button = CTK_BUTTON (actionable);

  return ctk_action_helper_get_action_name (button->priv->action_helper);
}

static GVariant *
ctk_button_get_action_target_value (CtkActionable *actionable)
{
  CtkButton *button = CTK_BUTTON (actionable);

  return ctk_action_helper_get_action_target_value (button->priv->action_helper);
}

static void
ctk_button_actionable_iface_init (CtkActionableInterface *iface)
{
  iface->get_action_name = ctk_button_get_action_name;
  iface->set_action_name = ctk_button_set_action_name;
  iface->get_action_target_value = ctk_button_get_action_target_value;
  iface->set_action_target_value = ctk_button_set_action_target_value;
}

static void 
ctk_button_activatable_interface_init (CtkActivatableIface  *iface)
{
  iface->update = ctk_button_update;
  iface->sync_action_properties = ctk_button_sync_action_properties;
}

static void
activatable_update_stock_id (CtkButton *button,
			     CtkAction *action)
{
  gboolean use_stock;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  use_stock = ctk_button_get_use_stock (button);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  if (!use_stock)
    return;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  ctk_button_set_label (button, ctk_action_get_stock_id (action));
  G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
activatable_update_short_label (CtkButton *button,
				CtkAction *action)
{
  CtkWidget *child;
  CtkWidget *image;
  gboolean use_stock;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  use_stock = ctk_button_get_use_stock (button);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  if (use_stock)
    return;

  image = ctk_button_get_image (button);

  /* Dont touch custom child... */
  child = ctk_bin_get_child (CTK_BIN (button));
  if (CTK_IS_IMAGE (image) ||
      child == NULL ||
      CTK_IS_LABEL (child))
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_button_set_label (button, ctk_action_get_short_label (action));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      ctk_button_set_use_underline (button, TRUE);
    }
}

static void
activatable_update_icon_name (CtkButton *button,
			      CtkAction *action)
{
  CtkWidget *image;
  gboolean use_stock;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  use_stock = ctk_button_get_use_stock (button);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  if (use_stock)
    return;

  image = ctk_button_get_image (button);

  if (CTK_IS_IMAGE (image) &&
      (ctk_image_get_storage_type (CTK_IMAGE (image)) == CTK_IMAGE_EMPTY ||
       ctk_image_get_storage_type (CTK_IMAGE (image)) == CTK_IMAGE_ICON_NAME))
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_image_set_from_icon_name (CTK_IMAGE (image),
                                    ctk_action_get_icon_name (action), CTK_ICON_SIZE_MENU);
      G_GNUC_END_IGNORE_DEPRECATIONS;
    }
}

static void
activatable_update_gicon (CtkButton *button,
			  CtkAction *action)
{
  CtkWidget *image = ctk_button_get_image (button);
  GIcon *icon;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  icon = ctk_action_get_gicon (action);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  if (CTK_IS_IMAGE (image) &&
      (ctk_image_get_storage_type (CTK_IMAGE (image)) == CTK_IMAGE_EMPTY ||
       ctk_image_get_storage_type (CTK_IMAGE (image)) == CTK_IMAGE_GICON))
    ctk_image_set_from_gicon (CTK_IMAGE (image), icon, CTK_ICON_SIZE_BUTTON);
}

static void 
ctk_button_update (CtkActivatable *activatable,
		   CtkAction      *action,
	           const gchar    *property_name)
{
  CtkButton *button = CTK_BUTTON (activatable);
  CtkButtonPrivate *priv = button->priv;

  if (strcmp (property_name, "visible") == 0)
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      if (ctk_action_is_visible (action))
	ctk_widget_show (CTK_WIDGET (activatable));
      else
	ctk_widget_hide (CTK_WIDGET (activatable));
      G_GNUC_END_IGNORE_DEPRECATIONS;
    }
  else if (strcmp (property_name, "sensitive") == 0)
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_widget_set_sensitive (CTK_WIDGET (activatable), ctk_action_is_sensitive (action));
      G_GNUC_END_IGNORE_DEPRECATIONS;
    }

  if (!priv->use_action_appearance)
    return;

  if (strcmp (property_name, "stock-id") == 0)
    activatable_update_stock_id (CTK_BUTTON (activatable), action);
  else if (strcmp (property_name, "gicon") == 0)
    activatable_update_gicon (CTK_BUTTON (activatable), action);
  else if (strcmp (property_name, "short-label") == 0)
    activatable_update_short_label (CTK_BUTTON (activatable), action);
  else if (strcmp (property_name, "icon-name") == 0)
    activatable_update_icon_name (CTK_BUTTON (activatable), action);
}

static void
ctk_button_sync_action_properties (CtkActivatable *activatable,
			           CtkAction      *action)
{
  CtkButton *button = CTK_BUTTON (activatable);
  CtkButtonPrivate *priv = button->priv;
  gboolean always_show_image;

  if (!action)
    return;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS

  if (ctk_action_is_visible (action))
    ctk_widget_show (CTK_WIDGET (activatable));
  else
    ctk_widget_hide (CTK_WIDGET (activatable));
  
  ctk_widget_set_sensitive (CTK_WIDGET (activatable), ctk_action_is_sensitive (action));
  always_show_image = ctk_action_get_always_show_image (action);
  G_GNUC_END_IGNORE_DEPRECATIONS

  if (priv->use_action_appearance)
    {
      activatable_update_stock_id (CTK_BUTTON (activatable), action);
      activatable_update_short_label (CTK_BUTTON (activatable), action);
      activatable_update_gicon (CTK_BUTTON (activatable), action);
      activatable_update_icon_name (CTK_BUTTON (activatable), action);
    }

  ctk_button_set_always_show_image (button, always_show_image);
}

static void
ctk_button_set_related_action (CtkButton *button,
			       CtkAction *action)
{
  CtkButtonPrivate *priv = button->priv;

  g_return_if_fail (ctk_action_helper_get_action_name (button->priv->action_helper) == NULL);

  if (priv->action == action)
    return;

  /* This should be a default handler, but for compatibility reasons
   * we need to support derived classes that don't chain up their
   * clicked handler.
   */
  g_signal_handlers_disconnect_by_func (button, ctk_real_button_clicked, NULL);
  if (action)
    g_signal_connect_after (button, "clicked",
                            G_CALLBACK (ctk_real_button_clicked), NULL);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_activatable_do_set_related_action (CTK_ACTIVATABLE (button), action);
  G_GNUC_END_IGNORE_DEPRECATIONS

  priv->action = action;
}

static void
ctk_button_set_use_action_appearance (CtkButton *button,
				      gboolean   use_appearance)
{
  CtkButtonPrivate *priv = button->priv;

  if (priv->use_action_appearance != use_appearance)
    {
      priv->use_action_appearance = use_appearance;

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_activatable_sync_action_properties (CTK_ACTIVATABLE (button), priv->action);
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS

    }
}

/**
 * ctk_button_new:
 *
 * Creates a new #CtkButton widget. To add a child widget to the button,
 * use ctk_container_add().
 *
 * Returns: The newly created #CtkButton widget.
 */
CtkWidget*
ctk_button_new (void)
{
  return g_object_new (CTK_TYPE_BUTTON, NULL);
}

static gboolean
show_image (CtkButton *button)
{
  CtkButtonPrivate *priv = button->priv;
  gboolean show;

  if (priv->label_text && !priv->always_show_image)
    {
      CtkSettings *settings;

      settings = ctk_widget_get_settings (CTK_WIDGET (button));        
      g_object_get (settings, "ctk-button-images", &show, NULL);
    }
  else
    show = TRUE;

  return show;
}


static void
ctk_button_construct_child (CtkButton *button)
{
  CtkButtonPrivate *priv = button->priv;
  CtkStyleContext *context;
  CtkStockItem item;
  CtkWidget *child;
  CtkWidget *label;
  CtkWidget *box;
  CtkWidget *align;
  CtkWidget *image = NULL;
  gchar *label_text = NULL;
  gint image_spacing;

  context = ctk_widget_get_style_context (CTK_WIDGET (button));
  ctk_style_context_remove_class (context, "image-button");
  ctk_style_context_remove_class (context, "text-button");

  if (!priv->constructed)
    return;

  if (!priv->label_text && !priv->image)
    return;

  ctk_style_context_get_style (context,
                               "image-spacing", &image_spacing,
                               NULL);

  if (priv->image && !priv->image_is_stock)
    {
      CtkWidget *parent;

      image = g_object_ref (priv->image);

      parent = ctk_widget_get_parent (image);
      if (parent)
	ctk_container_remove (CTK_CONTAINER (parent), image);
    }

  priv->image = NULL;

  child = ctk_bin_get_child (CTK_BIN (button));
  if (child)
    ctk_container_remove (CTK_CONTAINER (button), child);

  if (priv->use_stock &&
      priv->label_text &&
      ctk_stock_lookup (priv->label_text, &item))
    {
      if (!image)
	image = g_object_ref (ctk_image_new_from_stock (priv->label_text, CTK_ICON_SIZE_BUTTON));

      label_text = item.label;
    }
  else
    label_text = priv->label_text;

  if (image)
    {
      priv->image = image;
      g_object_set (priv->image,
		    "visible", show_image (button),
		    "no-show-all", TRUE,
		    NULL);

      if (priv->image_position == CTK_POS_LEFT ||
	  priv->image_position == CTK_POS_RIGHT)
	box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, image_spacing);
      else
	box = ctk_box_new (CTK_ORIENTATION_VERTICAL, image_spacing);

      ctk_widget_set_valign (image, CTK_ALIGN_BASELINE);
      ctk_widget_set_valign (box, CTK_ALIGN_BASELINE);

      if (priv->align_set)
	align = ctk_alignment_new (priv->xalign, priv->yalign, 0.0, 0.0);
      else
	align = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);

      ctk_widget_set_valign (align, CTK_ALIGN_BASELINE);

      if (priv->image_position == CTK_POS_LEFT ||
	  priv->image_position == CTK_POS_TOP)
	ctk_box_pack_start (CTK_BOX (box), priv->image, FALSE, FALSE, 0);
      else
	ctk_box_pack_end (CTK_BOX (box), priv->image, FALSE, FALSE, 0);

      if (label_text)
	{
          if (priv->use_underline || priv->use_stock)
            {
	      label = ctk_label_new_with_mnemonic (label_text);
	      ctk_label_set_mnemonic_widget (CTK_LABEL (label),
                                             CTK_WIDGET (button));
            }
          else
            label = ctk_label_new (label_text);

	  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);

	  if (priv->image_position == CTK_POS_RIGHT ||
	      priv->image_position == CTK_POS_BOTTOM)
	    ctk_box_pack_start (CTK_BOX (box), label, FALSE, FALSE, 0);
	  else
	    ctk_box_pack_end (CTK_BOX (box), label, FALSE, FALSE, 0);
	}
      else
        {
          ctk_style_context_add_class (context, "image-button");
        }

      ctk_container_add (CTK_CONTAINER (button), align);
      ctk_container_add (CTK_CONTAINER (align), box);
      ctk_widget_show_all (align);

      g_object_unref (image);

      return;
    }

  if (priv->use_underline || priv->use_stock)
    {
      label = ctk_label_new_with_mnemonic (priv->label_text);
      ctk_label_set_mnemonic_widget (CTK_LABEL (label), CTK_WIDGET (button));
    }
  else
    label = ctk_label_new (priv->label_text);

  ctk_widget_set_valign (label, CTK_ALIGN_BASELINE);

  maybe_set_alignment (button, label);

  ctk_widget_show (label);
  ctk_container_add (CTK_CONTAINER (button), label);

  ctk_style_context_add_class (context, "text-button");
}


/**
 * ctk_button_new_with_label:
 * @label: The text you want the #CtkLabel to hold.
 *
 * Creates a #CtkButton widget with a #CtkLabel child containing the given
 * text.
 *
 * Returns: The newly created #CtkButton widget.
 */
CtkWidget*
ctk_button_new_with_label (const gchar *label)
{
  return g_object_new (CTK_TYPE_BUTTON, "label", label, NULL);
}

/**
 * ctk_button_new_from_icon_name:
 * @icon_name: (nullable): an icon name or %NULL
 * @size: (type int): an icon size (#CtkIconSize)
 *
 * Creates a new button containing an icon from the current icon theme.
 *
 * If the icon name isn’t known, a “broken image” icon will be
 * displayed instead. If the current icon theme is changed, the icon
 * will be updated appropriately.
 *
 * This function is a convenience wrapper around ctk_button_new() and
 * ctk_button_set_image().
 *
 * Returns: a new #CtkButton displaying the themed icon
 *
 * Since: 3.10
 */
CtkWidget*
ctk_button_new_from_icon_name (const gchar *icon_name,
			       CtkIconSize  size)
{
  CtkWidget *button;
  CtkWidget *image;

  image = ctk_image_new_from_icon_name (icon_name, size);
  button =  g_object_new (CTK_TYPE_BUTTON,
			  "image", image,
			  NULL);

  return button;
}

/**
 * ctk_button_new_from_stock:
 * @stock_id: the name of the stock item 
 *
 * Creates a new #CtkButton containing the image and text from a
 * [stock item][ctkstock].
 * Some stock ids have preprocessor macros like #CTK_STOCK_OK and
 * #CTK_STOCK_APPLY.
 *
 * If @stock_id is unknown, then it will be treated as a mnemonic
 * label (as for ctk_button_new_with_mnemonic()).
 *
 * Returns: a new #CtkButton
 */
CtkWidget*
ctk_button_new_from_stock (const gchar *stock_id)
{
  return g_object_new (CTK_TYPE_BUTTON,
                       "label", stock_id,
                       "use-stock", TRUE,
                       "use-underline", TRUE,
                       NULL);
}

/**
 * ctk_button_new_with_mnemonic:
 * @label: The text of the button, with an underscore in front of the
 *         mnemonic character
 *
 * Creates a new #CtkButton containing a label.
 * If characters in @label are preceded by an underscore, they are underlined.
 * If you need a literal underscore character in a label, use “__” (two
 * underscores). The first underlined character represents a keyboard
 * accelerator called a mnemonic.
 * Pressing Alt and that key activates the button.
 *
 * Returns: a new #CtkButton
 */
CtkWidget*
ctk_button_new_with_mnemonic (const gchar *label)
{
  return g_object_new (CTK_TYPE_BUTTON, "label", label, "use-underline", TRUE,  NULL);
}

/**
 * ctk_button_pressed:
 * @button: The #CtkButton you want to send the signal to.
 *
 * Emits a #CtkButton::pressed signal to the given #CtkButton.
 *
 * Deprecated: 2.20: Use the #CtkWidget::button-press-event signal.
 */
void
ctk_button_pressed (CtkButton *button)
{
  g_return_if_fail (CTK_IS_BUTTON (button));

  g_signal_emit (button, button_signals[PRESSED], 0);
}

/**
 * ctk_button_released:
 * @button: The #CtkButton you want to send the signal to.
 *
 * Emits a #CtkButton::released signal to the given #CtkButton.
 *
 * Deprecated: 2.20: Use the #CtkWidget::button-release-event signal.
 */
void
ctk_button_released (CtkButton *button)
{
  g_return_if_fail (CTK_IS_BUTTON (button));

  g_signal_emit (button, button_signals[RELEASED], 0);
}

/**
 * ctk_button_clicked:
 * @button: The #CtkButton you want to send the signal to.
 *
 * Emits a #CtkButton::clicked signal to the given #CtkButton.
 */
void
ctk_button_clicked (CtkButton *button)
{
  g_return_if_fail (CTK_IS_BUTTON (button));

  g_signal_emit (button, button_signals[CLICKED], 0);
}

/**
 * ctk_button_enter:
 * @button: The #CtkButton you want to send the signal to.
 *
 * Emits a #CtkButton::enter signal to the given #CtkButton.
 *
 * Deprecated: 2.20: Use the #CtkWidget::enter-notify-event signal.
 */
void
ctk_button_enter (CtkButton *button)
{
  g_return_if_fail (CTK_IS_BUTTON (button));

  g_signal_emit (button, button_signals[ENTER], 0);
}

/**
 * ctk_button_leave:
 * @button: The #CtkButton you want to send the signal to.
 *
 * Emits a #CtkButton::leave signal to the given #CtkButton.
 *
 * Deprecated: 2.20: Use the #CtkWidget::leave-notify-event signal.
 */
void
ctk_button_leave (CtkButton *button)
{
  g_return_if_fail (CTK_IS_BUTTON (button));

  g_signal_emit (button, button_signals[LEAVE], 0);
}

/**
 * ctk_button_set_relief:
 * @button: The #CtkButton you want to set relief styles of
 * @relief: The CtkReliefStyle as described above
 *
 * Sets the relief style of the edges of the given #CtkButton widget.
 * Two styles exist, %CTK_RELIEF_NORMAL and %CTK_RELIEF_NONE.
 * The default style is, as one can guess, %CTK_RELIEF_NORMAL.
 * The deprecated value %CTK_RELIEF_HALF behaves the same as
 * %CTK_RELIEF_NORMAL.
 */
void
ctk_button_set_relief (CtkButton      *button,
		       CtkReliefStyle  relief)
{
  CtkStyleContext *context;
  CtkReliefStyle old_relief;

  g_return_if_fail (CTK_IS_BUTTON (button));

  old_relief = ctk_button_get_relief (button);
  if (old_relief != relief)
    {
      context = ctk_widget_get_style_context (CTK_WIDGET (button));
      if (relief == CTK_RELIEF_NONE)
        ctk_style_context_add_class (context, CTK_STYLE_CLASS_FLAT);
      else
        ctk_style_context_remove_class (context, CTK_STYLE_CLASS_FLAT);

      g_object_notify_by_pspec (G_OBJECT (button), props[PROP_RELIEF]);
    }
}

/**
 * ctk_button_get_relief:
 * @button: The #CtkButton you want the #CtkReliefStyle from.
 *
 * Returns the current relief style of the given #CtkButton.
 *
 * Returns: The current #CtkReliefStyle
 */
CtkReliefStyle
ctk_button_get_relief (CtkButton *button)
{
  CtkStyleContext *context;

  g_return_val_if_fail (CTK_IS_BUTTON (button), CTK_RELIEF_NORMAL);

  context = ctk_widget_get_style_context (CTK_WIDGET (button));
  if (ctk_style_context_has_class (context, CTK_STYLE_CLASS_FLAT))
    return CTK_RELIEF_NONE;
  else
    return CTK_RELIEF_NORMAL;
}

static void
ctk_button_realize (CtkWidget *widget)
{
  CtkButton *button = CTK_BUTTON (widget);
  CtkButtonPrivate *priv = button->priv;
  CtkAllocation allocation;
  CdkWindow *window;
  CdkWindowAttr attributes;
  gint attributes_mask;

  ctk_widget_get_allocation (widget, &allocation);

  ctk_widget_set_realized (widget, TRUE);

  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = CDK_INPUT_ONLY;
  attributes.event_mask = ctk_widget_get_events (widget);
  attributes.event_mask |= (CDK_BUTTON_PRESS_MASK |
                            CDK_BUTTON_RELEASE_MASK |
                            CDK_TOUCH_MASK |
                            CDK_ENTER_NOTIFY_MASK |
                            CDK_LEAVE_NOTIFY_MASK);

  attributes_mask = CDK_WA_X | CDK_WA_Y;

  window = ctk_widget_get_parent_window (widget);
  ctk_widget_set_window (widget, window);
  g_object_ref (window);

  priv->event_window = cdk_window_new (window,
                                       &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->event_window);
}

static void
ctk_button_unrealize (CtkWidget *widget)
{
  CtkButton *button = CTK_BUTTON (widget);
  CtkButtonPrivate *priv = button->priv;

  if (priv->activate_timeout)
    ctk_button_finish_activate (button, FALSE);

  if (priv->event_window)
    {
      ctk_widget_unregister_window (widget, priv->event_window);
      cdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  CTK_WIDGET_CLASS (ctk_button_parent_class)->unrealize (widget);
}

static void
ctk_button_map (CtkWidget *widget)
{
  CtkButton *button = CTK_BUTTON (widget);
  CtkButtonPrivate *priv = button->priv;

  CTK_WIDGET_CLASS (ctk_button_parent_class)->map (widget);

  if (priv->event_window)
    cdk_window_show (priv->event_window);
}

static void
ctk_button_unmap (CtkWidget *widget)
{
  CtkButton *button = CTK_BUTTON (widget);
  CtkButtonPrivate *priv = button->priv;

  if (priv->event_window)
    {
      cdk_window_hide (priv->event_window);
      priv->in_button = FALSE;
    }

  CTK_WIDGET_CLASS (ctk_button_parent_class)->unmap (widget);
}

static void
ctk_button_update_image_spacing (CtkButton       *button,
                                 CtkStyleContext *context)
{
  CtkButtonPrivate *priv = button->priv;
  CtkWidget *child; 
  gint spacing;

  /* Keep in sync with ctk_button_construct_child,
   * we only want to update the spacing if the box 
   * was constructed there.
   */
  if (!priv->constructed || !priv->image)
    return;

  child = ctk_bin_get_child (CTK_BIN (button));
  if (CTK_IS_ALIGNMENT (child))
    {
      child = ctk_bin_get_child (CTK_BIN (child));
      if (CTK_IS_BOX (child))
        {
          ctk_style_context_get_style (context,
                                       "image-spacing", &spacing,
                                       NULL);

          ctk_box_set_spacing (CTK_BOX (child), spacing);
        }
    }
}

static void
ctk_button_style_updated (CtkWidget *widget)
{
  CtkStyleContext *context;

  CTK_WIDGET_CLASS (ctk_button_parent_class)->style_updated (widget);

  context = ctk_widget_get_style_context (widget);

  ctk_button_update_image_spacing (CTK_BUTTON (widget), context);
}

static void
ctk_button_size_allocate (CtkWidget     *widget,
			  CtkAllocation *allocation)
{
  CtkButton *button = CTK_BUTTON (widget);
  CtkButtonPrivate *priv = button->priv;
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);
  ctk_css_gadget_allocate (priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_button_allocate (CtkCssGadget        *gadget,
                     const CtkAllocation *allocation,
                     int                  baseline,
                     CtkAllocation       *out_clip,
                     gpointer             unused)
{
  CtkWidget *widget;
  CtkWidget *child;

  widget = ctk_css_gadget_get_owner (gadget);

  child = ctk_bin_get_child (CTK_BIN (widget));
  if (child && ctk_widget_get_visible (child))
    ctk_widget_size_allocate_with_baseline (child, (CtkAllocation *)allocation, baseline);

  if (ctk_widget_get_realized (widget))
    {
      CtkAllocation border_allocation;
      ctk_css_gadget_get_border_allocation (gadget, &border_allocation, NULL);
      cdk_window_move_resize (CTK_BUTTON (widget)->priv->event_window,
                              border_allocation.x,
                              border_allocation.y,
                              border_allocation.width,
                              border_allocation.height);
    }

  ctk_container_get_children_clip (CTK_CONTAINER (widget), out_clip);
}

static gboolean
ctk_button_draw (CtkWidget *widget,
		 cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_BUTTON (widget)->priv->gadget, cr);

  return FALSE;
}

static gboolean
ctk_button_render (CtkCssGadget *gadget,
                   cairo_t      *cr,
                   int           x,
                   int           y,
                   int           width,
                   int           height,
                   gpointer      data)
{
  CtkWidget *widget;

  widget = ctk_css_gadget_get_owner (gadget);

  CTK_WIDGET_CLASS (ctk_button_parent_class)->draw (widget, cr);

  return ctk_widget_has_visible_focus (widget);
}

static void
ctk_button_do_release (CtkButton *button,
                       gboolean   emit_clicked)
{
  CtkButtonPrivate *priv = button->priv;

  if (priv->button_down)
    {
      priv->button_down = FALSE;

      if (priv->activate_timeout)
	return;

      if (emit_clicked)
        ctk_button_clicked (button);

      ctk_button_update_state (button);
    }
}

static gboolean
ctk_button_grab_broken (CtkWidget          *widget,
			CdkEventGrabBroken *event)
{
  CtkButton *button = CTK_BUTTON (widget);
  
  ctk_button_do_release (button, FALSE);

  return TRUE;
}

static gboolean
ctk_button_key_release (CtkWidget   *widget,
			CdkEventKey *event)
{
  CtkButton *button = CTK_BUTTON (widget);
  CtkButtonPrivate *priv = button->priv;

  if (priv->activate_timeout)
    {
      ctk_button_finish_activate (button, TRUE);
      return TRUE;
    }
  else if (CTK_WIDGET_CLASS (ctk_button_parent_class)->key_release_event)
    return CTK_WIDGET_CLASS (ctk_button_parent_class)->key_release_event (widget, event);
  else
    return FALSE;
}

static gboolean
ctk_button_enter_notify (CtkWidget        *widget,
			 CdkEventCrossing *event)
{
  CtkButton *button = CTK_BUTTON (widget);
  CtkButtonPrivate *priv = button->priv;

  if ((event->window == button->priv->event_window) &&
      (event->detail != CDK_NOTIFY_INFERIOR))
    {
      priv->in_button = TRUE;
      g_signal_emit (button, button_signals[ENTER], 0);
    }

  return FALSE;
}

static gboolean
ctk_button_leave_notify (CtkWidget        *widget,
			 CdkEventCrossing *event)
{
  CtkButton *button = CTK_BUTTON (widget);
  CtkButtonPrivate *priv = button->priv;

  if ((event->window == button->priv->event_window) &&
      (event->detail != CDK_NOTIFY_INFERIOR))
    {
      priv->in_button = FALSE;
      g_signal_emit (button, button_signals[LEAVE], 0);
    }

  return FALSE;
}

static void
ctk_real_button_pressed (CtkButton *button)
{
  CtkButtonPrivate *priv = button->priv;

  if (priv->activate_timeout)
    return;

  priv->button_down = TRUE;
  ctk_button_update_state (button);
}

static gboolean
touch_release_in_button (CtkButton *button)
{
  CtkButtonPrivate *priv;
  gint width, height;
  CdkEvent *event;
  gdouble x, y;

  priv = button->priv;
  event = ctk_get_current_event ();

  if (!event)
    return FALSE;

  if (event->type != CDK_TOUCH_END ||
      event->touch.window != priv->event_window)
    {
      cdk_event_free (event);
      return FALSE;
    }

  cdk_event_get_coords (event, &x, &y);
  width = cdk_window_get_width (priv->event_window);
  height = cdk_window_get_height (priv->event_window);

  cdk_event_free (event);

  if (x >= 0 && x <= width &&
      y >= 0 && y <= height)
    return TRUE;

  return FALSE;
}

static void
ctk_real_button_released (CtkButton *button)
{
  ctk_button_do_release (button,
                         ctk_widget_is_sensitive (CTK_WIDGET (button)) &&
                         (button->priv->in_button ||
                          touch_release_in_button (button)));
}

static void 
ctk_real_button_clicked (CtkButton *button)
{
  CtkButtonPrivate *priv = button->priv;

  if (priv->action_helper)
    ctk_action_helper_activate (priv->action_helper);

  if (priv->action)
    ctk_action_activate (priv->action);
}

static gboolean
button_activate_timeout (gpointer data)
{
  ctk_button_finish_activate (data, TRUE);

  return FALSE;
}

static void
ctk_real_button_activate (CtkButton *button)
{
  CtkWidget *widget = CTK_WIDGET (button);
  CtkButtonPrivate *priv = button->priv;
  CdkDevice *device;

  device = ctk_get_current_event_device ();

  if (device && cdk_device_get_source (device) != CDK_SOURCE_KEYBOARD)
    device = cdk_device_get_associated_device (device);

  if (ctk_widget_get_realized (widget) && !priv->activate_timeout)
    {
      /* bgo#626336 - Only grab if we have a device (from an event), not if we
       * were activated programmatically when no event is available.
       */
      if (device && cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD)
	{
          ctk_device_grab_add (widget, device, TRUE);
          priv->grab_keyboard = device;
	}

      priv->activate_timeout = cdk_threads_add_timeout (ACTIVATE_TIMEOUT,
						button_activate_timeout,
						button);
      g_source_set_name_by_id (priv->activate_timeout, "[ctk+] button_activate_timeout");
      priv->button_down = TRUE;
      ctk_button_update_state (button);
    }
}

static void
ctk_button_finish_activate (CtkButton *button,
			    gboolean   do_it)
{
  CtkWidget *widget = CTK_WIDGET (button);
  CtkButtonPrivate *priv = button->priv;

  g_source_remove (priv->activate_timeout);
  priv->activate_timeout = 0;

  if (priv->grab_keyboard)
    {
      ctk_device_grab_remove (widget, priv->grab_keyboard);
      priv->grab_keyboard = NULL;
    }

  priv->button_down = FALSE;

  ctk_button_update_state (button);

  if (do_it)
    ctk_button_clicked (button);
}


static void
ctk_button_measure (CtkCssGadget   *gadget,
		    CtkOrientation  orientation,
                    int             for_size,
		    int            *minimum,
		    int            *natural,
		    int            *minimum_baseline,
		    int            *natural_baseline,
                    gpointer        data)
{
  CtkWidget *widget;
  CtkWidget *child;

  widget = ctk_css_gadget_get_owner (gadget);
  child = ctk_bin_get_child (CTK_BIN (widget));

  if (child && ctk_widget_get_visible (child))
    {
       _ctk_widget_get_preferred_size_for_size (child,
                                                orientation,
                                                for_size,
                                                minimum, natural,
                                                minimum_baseline, natural_baseline);
    }
  else
    {
      *minimum = 0;
      *natural = 0;
      if (minimum_baseline)
        *minimum_baseline = 0;
      if (natural_baseline)
        *natural_baseline = 0;
    }
}

static void
ctk_button_get_preferred_width (CtkWidget *widget,
                                gint      *minimum_size,
                                gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_BUTTON (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_button_get_preferred_height (CtkWidget *widget,
                                 gint      *minimum_size,
                                 gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_BUTTON (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_button_get_preferred_width_for_height (CtkWidget *widget,
                                           gint       for_size,
                                           gint      *minimum_size,
                                           gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_BUTTON (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     for_size,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_button_get_preferred_height_for_width (CtkWidget *widget,
                                           gint       for_size,
                                           gint      *minimum_size,
                                           gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_BUTTON (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     for_size,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_button_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
							gint       for_size,
							gint      *minimum_size,
							gint      *natural_size,
							gint      *minimum_baseline,
							gint      *natural_baseline)
{
  ctk_css_gadget_get_preferred_size (CTK_BUTTON (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     for_size,
                                     minimum_size, natural_size,
                                     minimum_baseline, natural_baseline);
}

/**
 * ctk_button_set_label:
 * @button: a #CtkButton
 * @label: a string
 *
 * Sets the text of the label of the button to @str. This text is
 * also used to select the stock item if ctk_button_set_use_stock()
 * is used.
 *
 * This will also clear any previously set labels.
 */
void
ctk_button_set_label (CtkButton   *button,
		      const gchar *label)
{
  CtkButtonPrivate *priv;
  gchar *new_label;

  g_return_if_fail (CTK_IS_BUTTON (button));

  priv = button->priv;

  new_label = g_strdup (label);
  g_free (priv->label_text);
  priv->label_text = new_label;

  ctk_button_construct_child (button);
  
  g_object_notify_by_pspec (G_OBJECT (button), props[PROP_LABEL]);
}

/**
 * ctk_button_get_label:
 * @button: a #CtkButton
 *
 * Fetches the text from the label of the button, as set by
 * ctk_button_set_label(). If the label text has not 
 * been set the return value will be %NULL. This will be the 
 * case if you create an empty button with ctk_button_new() to 
 * use as a container.
 *
 * Returns: The text of the label widget. This string is owned
 * by the widget and must not be modified or freed.
 */
const gchar *
ctk_button_get_label (CtkButton *button)
{
  g_return_val_if_fail (CTK_IS_BUTTON (button), NULL);

  return button->priv->label_text;
}

/**
 * ctk_button_set_use_underline:
 * @button: a #CtkButton
 * @use_underline: %TRUE if underlines in the text indicate mnemonics
 *
 * If true, an underline in the text of the button label indicates
 * the next character should be used for the mnemonic accelerator key.
 */
void
ctk_button_set_use_underline (CtkButton *button,
			      gboolean   use_underline)
{
  CtkButtonPrivate *priv;

  g_return_if_fail (CTK_IS_BUTTON (button));

  priv = button->priv;

  use_underline = use_underline != FALSE;

  if (use_underline != priv->use_underline)
    {
      priv->use_underline = use_underline;

      ctk_button_construct_child (button);
      
      g_object_notify_by_pspec (G_OBJECT (button), props[PROP_USE_UNDERLINE]);
    }
}

/**
 * ctk_button_get_use_underline:
 * @button: a #CtkButton
 *
 * Returns whether an embedded underline in the button label indicates a
 * mnemonic. See ctk_button_set_use_underline ().
 *
 * Returns: %TRUE if an embedded underline in the button label
 *               indicates the mnemonic accelerator keys.
 */
gboolean
ctk_button_get_use_underline (CtkButton *button)
{
  g_return_val_if_fail (CTK_IS_BUTTON (button), FALSE);

  return button->priv->use_underline;
}

/**
 * ctk_button_set_use_stock:
 * @button: a #CtkButton
 * @use_stock: %TRUE if the button should use a stock item
 *
 * If %TRUE, the label set on the button is used as a
 * stock id to select the stock item for the button.
 *
 * Deprecated: 3.10
 */
void
ctk_button_set_use_stock (CtkButton *button,
			  gboolean   use_stock)
{
  CtkButtonPrivate *priv;

  g_return_if_fail (CTK_IS_BUTTON (button));

  priv = button->priv;

  use_stock = use_stock != FALSE;

  if (use_stock != priv->use_stock)
    {
      priv->use_stock = use_stock;

      ctk_button_construct_child (button);
      
      g_object_notify_by_pspec (G_OBJECT (button), props[PROP_USE_STOCK]);
    }
}

/**
 * ctk_button_get_use_stock:
 * @button: a #CtkButton
 *
 * Returns whether the button label is a stock item.
 *
 * Returns: %TRUE if the button label is used to
 *               select a stock item instead of being
 *               used directly as the label text.
 *
 * Deprecated: 3.10
 */
gboolean
ctk_button_get_use_stock (CtkButton *button)
{
  g_return_val_if_fail (CTK_IS_BUTTON (button), FALSE);

  return button->priv->use_stock;
}

/**
 * ctk_button_set_focus_on_click:
 * @button: a #CtkButton
 * @focus_on_click: whether the button grabs focus when clicked with the mouse
 *
 * Sets whether the button will grab focus when it is clicked with the mouse.
 * Making mouse clicks not grab focus is useful in places like toolbars where
 * you don’t want the keyboard focus removed from the main area of the
 * application.
 *
 * Since: 2.4
 *
 * Deprecated: 3.20: Use ctk_widget_set_focus_on_click() instead
 */
void
ctk_button_set_focus_on_click (CtkButton *button,
			       gboolean   focus_on_click)
{
  g_return_if_fail (CTK_IS_BUTTON (button));

  ctk_widget_set_focus_on_click (CTK_WIDGET (button), focus_on_click);
}

/**
 * ctk_button_get_focus_on_click:
 * @button: a #CtkButton
 *
 * Returns whether the button grabs focus when it is clicked with the mouse.
 * See ctk_button_set_focus_on_click().
 *
 * Returns: %TRUE if the button grabs focus when it is clicked with
 *               the mouse.
 *
 * Since: 2.4
 *
 * Deprecated: 3.20: Use ctk_widget_get_focus_on_click() instead
 */
gboolean
ctk_button_get_focus_on_click (CtkButton *button)
{
  g_return_val_if_fail (CTK_IS_BUTTON (button), FALSE);
  
  return ctk_widget_get_focus_on_click (CTK_WIDGET (button));
}

/**
 * ctk_button_set_alignment:
 * @button: a #CtkButton
 * @xalign: the horizontal position of the child, 0.0 is left aligned, 
 *   1.0 is right aligned
 * @yalign: the vertical position of the child, 0.0 is top aligned, 
 *   1.0 is bottom aligned
 *
 * Sets the alignment of the child. This property has no effect unless 
 * the child is a #CtkMisc or a #CtkAlignment.
 *
 * Since: 2.4
 *
 * Deprecated: 3.14: Access the child widget directly if you need to control
 * its alignment.
 */
void
ctk_button_set_alignment (CtkButton *button,
			  gfloat     xalign,
			  gfloat     yalign)
{
  CtkButtonPrivate *priv;

  g_return_if_fail (CTK_IS_BUTTON (button));
  
  priv = button->priv;

  priv->xalign = xalign;
  priv->yalign = yalign;
  priv->align_set = 1;

  maybe_set_alignment (button, ctk_bin_get_child (CTK_BIN (button)));

  g_object_freeze_notify (G_OBJECT (button));
  g_object_notify_by_pspec (G_OBJECT (button), props[PROP_XALIGN]);
  g_object_notify_by_pspec (G_OBJECT (button), props[PROP_YALIGN]);
  g_object_thaw_notify (G_OBJECT (button));
}

/**
 * ctk_button_get_alignment:
 * @button: a #CtkButton
 * @xalign: (out): return location for horizontal alignment
 * @yalign: (out): return location for vertical alignment
 *
 * Gets the alignment of the child in the button.
 *
 * Since: 2.4
 *
 * Deprecated: 3.14: Access the child widget directly if you need to control
 * its alignment.
 */
void
ctk_button_get_alignment (CtkButton *button,
			  gfloat    *xalign,
			  gfloat    *yalign)
{
  CtkButtonPrivate *priv;

  g_return_if_fail (CTK_IS_BUTTON (button));
  
  priv = button->priv;
 
  if (xalign) 
    *xalign = priv->xalign;

  if (yalign)
    *yalign = priv->yalign;
}

static void
ctk_button_enter_leave (CtkButton *button)
{
  ctk_button_update_state (button);
}

static void
ctk_button_update_state (CtkButton *button)
{
  CtkButtonPrivate *priv = button->priv;
  CtkStateFlags new_state;
  gboolean depressed;

  if (priv->activate_timeout)
    depressed = TRUE;
  else
    depressed = priv->in_button && priv->button_down;

  new_state = ctk_widget_get_state_flags (CTK_WIDGET (button)) &
    ~(CTK_STATE_FLAG_PRELIGHT | CTK_STATE_FLAG_ACTIVE);

  if (priv->in_button)
    new_state |= CTK_STATE_FLAG_PRELIGHT;

  if (depressed)
    new_state |= CTK_STATE_FLAG_ACTIVE;

  ctk_widget_set_state_flags (CTK_WIDGET (button), new_state, TRUE);
}

static void 
show_image_change_notify (CtkButton *button)
{
  CtkButtonPrivate *priv = button->priv;

  if (priv->image) 
    {
      if (show_image (button))
	ctk_widget_show (priv->image);
      else
	ctk_widget_hide (priv->image);
    }
}

static void
traverse_container (CtkWidget *widget,
		    gpointer   data)
{
  if (CTK_IS_BUTTON (widget))
    show_image_change_notify (CTK_BUTTON (widget));
  else if (CTK_IS_CONTAINER (widget))
    ctk_container_forall (CTK_CONTAINER (widget), traverse_container, NULL);
}

static void
ctk_button_setting_changed (CtkSettings *settings)
{
  GList *list, *l;

  list = ctk_window_list_toplevels ();

  for (l = list; l; l = l->next)
    ctk_container_forall (CTK_CONTAINER (l->data), 
			  traverse_container, NULL);

  g_list_free (list);
}

static void
ctk_button_screen_changed (CtkWidget *widget,
			   CdkScreen *previous_screen)
{
  CtkButton *button;
  CtkButtonPrivate *priv;
  CtkSettings *settings;
  gulong show_image_connection;

  if (!ctk_widget_has_screen (widget))
    return;

  button = CTK_BUTTON (widget);
  priv = button->priv;

  /* If the button is being pressed while the screen changes the
    release might never occur, so we reset the state. */
  if (priv->button_down)
    {
      priv->button_down = FALSE;
      ctk_button_update_state (button);
    }

  settings = ctk_widget_get_settings (widget);

  show_image_connection = 
    g_signal_handler_find (settings, G_SIGNAL_MATCH_FUNC, 0, 0,
                           NULL, ctk_button_setting_changed, NULL);
  
  if (show_image_connection)
    return;

  g_signal_connect (settings, "notify::ctk-button-images",
                    G_CALLBACK (ctk_button_setting_changed), NULL);

  show_image_change_notify (button);
}

static void
ctk_button_state_changed (CtkWidget    *widget,
                          CtkStateType  previous_state)
{
  CtkButton *button = CTK_BUTTON (widget);

  if (!ctk_widget_is_sensitive (widget))
    ctk_button_do_release (button, FALSE);
}

static void
ctk_button_grab_notify (CtkWidget *widget,
			gboolean   was_grabbed)
{
  CtkButton *button = CTK_BUTTON (widget);
  CtkButtonPrivate *priv = button->priv;

  if (priv->activate_timeout &&
      priv->grab_keyboard &&
      ctk_widget_device_is_shadowed (widget, priv->grab_keyboard))
    ctk_button_finish_activate (button, FALSE);

  if (!was_grabbed)
    ctk_button_do_release (button, FALSE);
}

/**
 * ctk_button_set_image:
 * @button: a #CtkButton
 * @image: (nullable): a widget to set as the image for the button, or %NULL to unset
 *
 * Set the image of @button to the given widget. The image will be
 * displayed if the label text is %NULL or if
 * #CtkButton:always-show-image is %TRUE. You don’t have to call
 * ctk_widget_show() on @image yourself.
 *
 * Since: 2.6
 */
void
ctk_button_set_image (CtkButton *button,
		      CtkWidget *image)
{
  CtkButtonPrivate *priv;
  CtkWidget *parent;

  g_return_if_fail (CTK_IS_BUTTON (button));
  g_return_if_fail (image == NULL || CTK_IS_WIDGET (image));

  priv = button->priv;

  if (priv->image)
    {
      parent = ctk_widget_get_parent (priv->image);
      if (parent)
        ctk_container_remove (CTK_CONTAINER (parent), priv->image);
    }

  priv->image = image;
  priv->image_is_stock = (image == NULL);

  ctk_button_construct_child (button);

  g_object_notify_by_pspec (G_OBJECT (button), props[PROP_IMAGE]);
}

/**
 * ctk_button_get_image:
 * @button: a #CtkButton
 *
 * Gets the widget that is currenty set as the image of @button.
 * This may have been explicitly set by ctk_button_set_image()
 * or constructed by ctk_button_new_from_stock().
 *
 * Returns: (nullable) (transfer none): a #CtkWidget or %NULL in case
 *     there is no image
 *
 * Since: 2.6
 */
CtkWidget *
ctk_button_get_image (CtkButton *button)
{
  g_return_val_if_fail (CTK_IS_BUTTON (button), NULL);
  
  return button->priv->image;
}

/**
 * ctk_button_set_image_position:
 * @button: a #CtkButton
 * @position: the position
 *
 * Sets the position of the image relative to the text 
 * inside the button.
 *
 * Since: 2.10
 */ 
void
ctk_button_set_image_position (CtkButton       *button,
			       CtkPositionType  position)
{
  CtkButtonPrivate *priv;

  g_return_if_fail (CTK_IS_BUTTON (button));
  g_return_if_fail (position >= CTK_POS_LEFT && position <= CTK_POS_BOTTOM);
  
  priv = button->priv;

  if (priv->image_position != position)
    {
      priv->image_position = position;

      ctk_button_construct_child (button);

      g_object_notify_by_pspec (G_OBJECT (button), props[PROP_IMAGE_POSITION]);
    }
}

/**
 * ctk_button_get_image_position:
 * @button: a #CtkButton
 *
 * Gets the position of the image relative to the text 
 * inside the button.
 *
 * Returns: the position
 *
 * Since: 2.10
 */
CtkPositionType
ctk_button_get_image_position (CtkButton *button)
{
  g_return_val_if_fail (CTK_IS_BUTTON (button), CTK_POS_LEFT);
  
  return button->priv->image_position;
}

/**
 * ctk_button_set_always_show_image:
 * @button: a #CtkButton
 * @always_show: %TRUE if the menuitem should always show the image
 *
 * If %TRUE, the button will ignore the #CtkSettings:ctk-button-images
 * setting and always show the image, if available.
 *
 * Use this property if the button  would be useless or hard to use
 * without the image.
 *
 * Since: 3.6
 */
void
ctk_button_set_always_show_image (CtkButton *button,
                                  gboolean    always_show)
{
  CtkButtonPrivate *priv;

  g_return_if_fail (CTK_IS_BUTTON (button));

  priv = button->priv;

  if (priv->always_show_image != always_show)
    {
      priv->always_show_image = always_show;

      if (priv->image)
        {
          if (show_image (button))
            ctk_widget_show (priv->image);
          else
            ctk_widget_hide (priv->image);
        }

      g_object_notify_by_pspec (G_OBJECT (button), props[PROP_ALWAYS_SHOW_IMAGE]);
    }
}

/**
 * ctk_button_get_always_show_image:
 * @button: a #CtkButton
 *
 * Returns whether the button will ignore the #CtkSettings:ctk-button-images
 * setting and always show the image, if available.
 *
 * Returns: %TRUE if the button will always show the image
 *
 * Since: 3.6
 */
gboolean
ctk_button_get_always_show_image (CtkButton *button)
{
  g_return_val_if_fail (CTK_IS_BUTTON (button), FALSE);

  return button->priv->always_show_image;
}

/**
 * ctk_button_get_event_window:
 * @button: a #CtkButton
 *
 * Returns the button’s event window if it is realized, %NULL otherwise.
 * This function should be rarely needed.
 *
 * Returns: (transfer none): @button’s event window.
 *
 * Since: 2.22
 */
CdkWindow*
ctk_button_get_event_window (CtkButton *button)
{
  g_return_val_if_fail (CTK_IS_BUTTON (button), NULL);

  return button->priv->event_window;
}
