/* GTK - The GIMP Toolkit
 * Copyright (C) 2005 Ronald S. Bultje
 * Copyright (C) 2006, 2007 Christian Persch
 * Copyright (C) 2006 Jan Arne Petersen
 * Copyright (C) 2005-2007 Red Hat, Inc.
 * Copyright (C) 2014 Red Hat, Inc.
 *
 * Authors:
 * - Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * - Bastien Nocera <bnocera@redhat.com>
 * - Jan Arne Petersen <jpetersen@jpetersen.org>
 * - Christian Persch <chpe@svn.gnome.org>
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
 * Modified by the GTK+ Team and others 2007.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include "ctkscalebutton.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "ctkadjustment.h"
#include "ctkbindings.h"
#include "ctkframe.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkorientable.h"
#include "ctkpopover.h"
#include "ctkprivate.h"
#include "ctkscale.h"
#include "ctkbox.h"
#include "ctkwindow.h"
#include "ctkwindowprivate.h"
#include "ctktypebuiltins.h"
#include "ctkintl.h"
#include "a11y/ctkscalebuttonaccessible.h"

/**
 * SECTION:ctkscalebutton
 * @Short_description: A button which pops up a scale
 * @Title: GtkScaleButton
 *
 * #GtkScaleButton provides a button which pops up a scale widget.
 * This kind of widget is commonly used for volume controls in multimedia
 * applications, and GTK+ provides a #GtkVolumeButton subclass that
 * is tailored for this use case.
 *
 * # CSS nodes
 *
 * GtkScaleButton has a single CSS node with name button. To differentiate
 * it from a plain #GtkButton, it gets the .scale style class.
 *
 * The popup widget that contains the scale has a .scale-popup style class.
 */


#define SCALE_SIZE 100
#define CLICK_TIMEOUT 250

enum
{
  VALUE_CHANGED,
  POPUP,
  POPDOWN,

  LAST_SIGNAL
};

enum
{
  PROP_0,

  PROP_ORIENTATION,
  PROP_VALUE,
  PROP_SIZE,
  PROP_ADJUSTMENT,
  PROP_ICONS
};

struct _GtkScaleButtonPrivate
{
  GtkWidget *plus_button;
  GtkWidget *minus_button;
  GtkWidget *dock;
  GtkWidget *box;
  GtkWidget *scale;
  GtkWidget *image;
  GtkWidget *active_button;

  GtkIconSize size;
  GtkOrientation orientation;
  GtkOrientation applied_orientation;

  guint click_id;

  gchar **icon_list;

  GtkAdjustment *adjustment; /* needed because it must be settable in init() */
};

static void     ctk_scale_button_constructed    (GObject             *object);
static void	ctk_scale_button_dispose	(GObject             *object);
static void     ctk_scale_button_finalize       (GObject             *object);
static void	ctk_scale_button_set_property	(GObject             *object,
						 guint                prop_id,
						 const GValue        *value,
						 GParamSpec          *pspec);
static void	ctk_scale_button_get_property	(GObject             *object,
						 guint                prop_id,
						 GValue              *value,
						 GParamSpec          *pspec);
static void ctk_scale_button_set_orientation_private (GtkScaleButton *button,
                                                      GtkOrientation  orientation);
static gboolean	ctk_scale_button_scroll		(GtkWidget           *widget,
						 GdkEventScroll      *event);
static void     ctk_scale_button_clicked        (GtkButton           *button);
static void     ctk_scale_button_popup          (GtkWidget           *widget);
static void     ctk_scale_button_popdown        (GtkWidget           *widget);
static gboolean cb_button_press			(GtkWidget           *widget,
						 GdkEventButton      *event,
						 gpointer             user_data);
static gboolean cb_button_release		(GtkWidget           *widget,
						 GdkEventButton      *event,
						 gpointer             user_data);
static void     cb_button_clicked               (GtkWidget           *button,
                                                 gpointer             user_data);
static void     ctk_scale_button_update_icon    (GtkScaleButton      *button);
static void     cb_scale_value_changed          (GtkRange            *range,
                                                 gpointer             user_data);
static void     cb_popup_mapped                 (GtkWidget           *popup,
                                                 gpointer             user_data);

G_DEFINE_TYPE_WITH_CODE (GtkScaleButton, ctk_scale_button, CTK_TYPE_BUTTON,
                         G_ADD_PRIVATE (GtkScaleButton)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ORIENTABLE,
                                                NULL))

static guint signals[LAST_SIGNAL] = { 0, };

static void
ctk_scale_button_class_init (GtkScaleButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  GtkButtonClass *button_class = CTK_BUTTON_CLASS (klass);
  GtkBindingSet *binding_set;

  gobject_class->constructed = ctk_scale_button_constructed;
  gobject_class->finalize = ctk_scale_button_finalize;
  gobject_class->dispose = ctk_scale_button_dispose;
  gobject_class->set_property = ctk_scale_button_set_property;
  gobject_class->get_property = ctk_scale_button_get_property;

  widget_class->scroll_event = ctk_scale_button_scroll;

  button_class->clicked = ctk_scale_button_clicked;

  /**
   * GtkScaleButton:orientation:
   *
   * The orientation of the #GtkScaleButton's popup window.
   *
   * Note that since GTK+ 2.16, #GtkScaleButton implements the
   * #GtkOrientable interface which has its own @orientation
   * property. However we redefine the property here in order to
   * override its default horizontal orientation.
   *
   * Since: 2.14
   **/
  g_object_class_override_property (gobject_class,
				    PROP_ORIENTATION,
				    "orientation");

  g_object_class_install_property (gobject_class,
				   PROP_VALUE,
				   g_param_spec_double ("value",
							P_("Value"),
							P_("The value of the scale"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE,
							0,
							CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
				   PROP_SIZE,
				   g_param_spec_enum ("size",
						      P_("Icon size"),
						      P_("The icon size"),
						      CTK_TYPE_ICON_SIZE,
						      CTK_ICON_SIZE_SMALL_TOOLBAR,
						      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
                                   PROP_ADJUSTMENT,
                                   g_param_spec_object ("adjustment",
							P_("Adjustment"),
							P_("The GtkAdjustment that contains the current value of this scale button object"),
                                                        CTK_TYPE_ADJUSTMENT,
                                                        CTK_PARAM_READWRITE));

  /**
   * GtkScaleButton:icons:
   *
   * The names of the icons to be used by the scale button.
   * The first item in the array will be used in the button
   * when the current value is the lowest value, the second
   * item for the highest value. All the subsequent icons will
   * be used for all the other values, spread evenly over the
   * range of values.
   *
   * If there's only one icon name in the @icons array, it will
   * be used for all the values. If only two icon names are in
   * the @icons array, the first one will be used for the bottom
   * 50% of the scale, and the second one for the top 50%.
   *
   * It is recommended to use at least 3 icons so that the
   * #GtkScaleButton reflects the current value of the scale
   * better for the users.
   *
   * Since: 2.12
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ICONS,
                                   g_param_spec_boxed ("icons",
                                                       P_("Icons"),
                                                       P_("List of icon names"),
                                                       G_TYPE_STRV,
                                                       CTK_PARAM_READWRITE));

  /**
   * GtkScaleButton::value-changed:
   * @button: the object which received the signal
   * @value: the new value
   *
   * The ::value-changed signal is emitted when the value field has
   * changed.
   *
   * Since: 2.12
   */
  signals[VALUE_CHANGED] =
    g_signal_new (I_("value-changed"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GtkScaleButtonClass, value_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1, G_TYPE_DOUBLE);

  /**
   * GtkScaleButton::popup:
   * @button: the object which received the signal
   *
   * The ::popup signal is a
   * [keybinding signal][GtkBindingSignal]
   * which gets emitted to popup the scale widget.
   *
   * The default bindings for this signal are Space, Enter and Return.
   *
   * Since: 2.12
   */
  signals[POPUP] =
    g_signal_new_class_handler (I_("popup"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (ctk_scale_button_popup),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 0);

  /**
   * GtkScaleButton::popdown:
   * @button: the object which received the signal
   *
   * The ::popdown signal is a
   * [keybinding signal][GtkBindingSignal]
   * which gets emitted to popdown the scale widget.
   *
   * The default binding for this signal is Escape.
   *
   * Since: 2.12
   */
  signals[POPDOWN] =
    g_signal_new_class_handler (I_("popdown"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (ctk_scale_button_popdown),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 0);

  /* Key bindings */
  binding_set = ctk_binding_set_by_class (widget_class);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_space, 0,
				"popup", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Space, 0,
				"popup", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_Return, 0,
				"popup", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_ISO_Enter, 0,
				"popup", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Enter, 0,
				"popup", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_Escape, 0,
				"popdown", 0);

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class,
					       "/org/ctk/libctk/ui/ctkscalebutton.ui");

  ctk_widget_class_bind_template_child_internal_private (widget_class, GtkScaleButton, plus_button);
  ctk_widget_class_bind_template_child_internal_private (widget_class, GtkScaleButton, minus_button);
  ctk_widget_class_bind_template_child_private (widget_class, GtkScaleButton, dock);
  ctk_widget_class_bind_template_child_private (widget_class, GtkScaleButton, box);
  ctk_widget_class_bind_template_child_private (widget_class, GtkScaleButton, scale);
  ctk_widget_class_bind_template_child_private (widget_class, GtkScaleButton, image);

  ctk_widget_class_bind_template_callback (widget_class, cb_button_press);
  ctk_widget_class_bind_template_callback (widget_class, cb_button_release);
  ctk_widget_class_bind_template_callback (widget_class, cb_button_clicked);
  ctk_widget_class_bind_template_callback (widget_class, cb_scale_value_changed);
  ctk_widget_class_bind_template_callback (widget_class, cb_popup_mapped);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_SCALE_BUTTON_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "button");
}

static void
ctk_scale_button_init (GtkScaleButton *button)
{
  GtkScaleButtonPrivate *priv;
  GtkStyleContext *context;

  button->priv = priv = ctk_scale_button_get_instance_private (button);

  priv->click_id = 0;
  priv->orientation = CTK_ORIENTATION_VERTICAL;
  priv->applied_orientation = CTK_ORIENTATION_VERTICAL;

  ctk_widget_init_template (CTK_WIDGET (button));
  ctk_popover_set_relative_to (CTK_POPOVER (priv->dock), CTK_WIDGET (button));

  /* Need a local reference to the adjustment */
  priv->adjustment = ctk_adjustment_new (0, 0, 100, 2, 20, 0);
  g_object_ref_sink (priv->adjustment);
  ctk_range_set_adjustment (CTK_RANGE (priv->scale), priv->adjustment);

  ctk_widget_add_events (CTK_WIDGET (button), GDK_SMOOTH_SCROLL_MASK);

  context = ctk_widget_get_style_context (CTK_WIDGET (button));
  ctk_style_context_add_class (context, "scale");
}

static void
ctk_scale_button_constructed (GObject *object)
{
  GtkScaleButton *button = CTK_SCALE_BUTTON (object);
  GtkScaleButtonPrivate *priv = button->priv;

  G_OBJECT_CLASS (ctk_scale_button_parent_class)->constructed (object);

  /* set button text and size */
  priv->size = CTK_ICON_SIZE_SMALL_TOOLBAR;
  ctk_scale_button_update_icon (button);
}

static void
ctk_scale_button_set_property (GObject       *object,
			       guint          prop_id,
			       const GValue  *value,
			       GParamSpec    *pspec)
{
  GtkScaleButton *button = CTK_SCALE_BUTTON (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      ctk_scale_button_set_orientation_private (button, g_value_get_enum (value));
      break;
    case PROP_VALUE:
      ctk_scale_button_set_value (button, g_value_get_double (value));
      break;
    case PROP_SIZE:
      if (button->priv->size != g_value_get_enum (value))
        {
          button->priv->size = g_value_get_enum (value);
          ctk_scale_button_update_icon (button);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_ADJUSTMENT:
      ctk_scale_button_set_adjustment (button, g_value_get_object (value));
      break;
    case PROP_ICONS:
      ctk_scale_button_set_icons (button,
                                  (const gchar **)g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_scale_button_get_property (GObject     *object,
			       guint        prop_id,
			       GValue      *value,
			       GParamSpec  *pspec)
{
  GtkScaleButton *button = CTK_SCALE_BUTTON (object);
  GtkScaleButtonPrivate *priv = button->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    case PROP_VALUE:
      g_value_set_double (value, ctk_scale_button_get_value (button));
      break;
    case PROP_SIZE:
      g_value_set_enum (value, priv->size);
      break;
    case PROP_ADJUSTMENT:
      g_value_set_object (value, ctk_scale_button_get_adjustment (button));
      break;
    case PROP_ICONS:
      g_value_set_boxed (value, priv->icon_list);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_scale_button_finalize (GObject *object)
{
  GtkScaleButton *button = CTK_SCALE_BUTTON (object);
  GtkScaleButtonPrivate *priv = button->priv;

  if (priv->icon_list)
    {
      g_strfreev (priv->icon_list);
      priv->icon_list = NULL;
    }

  if (priv->adjustment)
    {
      g_object_unref (priv->adjustment);
      priv->adjustment = NULL;
    }

  G_OBJECT_CLASS (ctk_scale_button_parent_class)->finalize (object);
}

static void
ctk_scale_button_dispose (GObject *object)
{
  GtkScaleButton *button = CTK_SCALE_BUTTON (object);
  GtkScaleButtonPrivate *priv = button->priv;

  if (priv->dock)
    {
      ctk_widget_destroy (priv->dock);
      priv->dock = NULL;
    }

  if (priv->click_id != 0)
    {
      g_source_remove (priv->click_id);
      priv->click_id = 0;
    }

  G_OBJECT_CLASS (ctk_scale_button_parent_class)->dispose (object);
}

/**
 * ctk_scale_button_new:
 * @size: (type int): a stock icon size (#GtkIconSize)
 * @min: the minimum value of the scale (usually 0)
 * @max: the maximum value of the scale (usually 100)
 * @step: the stepping of value when a scroll-wheel event,
 *        or up/down arrow event occurs (usually 2)
 * @icons: (allow-none) (array zero-terminated=1): a %NULL-terminated
 *         array of icon names, or %NULL if you want to set the list
 *         later with ctk_scale_button_set_icons()
 *
 * Creates a #GtkScaleButton, with a range between @min and @max, with
 * a stepping of @step.
 *
 * Returns: a new #GtkScaleButton
 *
 * Since: 2.12
 */
GtkWidget *
ctk_scale_button_new (GtkIconSize   size,
		      gdouble       min,
		      gdouble       max,
		      gdouble       step,
		      const gchar **icons)
{
  GtkScaleButton *button;
  GtkAdjustment *adjustment;

  adjustment = ctk_adjustment_new (min, min, max, step, 10 * step, 0);

  button = g_object_new (CTK_TYPE_SCALE_BUTTON,
                         "adjustment", adjustment,
                         "icons", icons,
                         "size", size,
                         NULL);

  return CTK_WIDGET (button);
}

/**
 * ctk_scale_button_get_value:
 * @button: a #GtkScaleButton
 *
 * Gets the current value of the scale button.
 *
 * Returns: current value of the scale button
 *
 * Since: 2.12
 */
gdouble
ctk_scale_button_get_value (GtkScaleButton * button)
{
  GtkScaleButtonPrivate *priv;

  g_return_val_if_fail (CTK_IS_SCALE_BUTTON (button), 0);

  priv = button->priv;

  return ctk_adjustment_get_value (priv->adjustment);
}

/**
 * ctk_scale_button_set_value:
 * @button: a #GtkScaleButton
 * @value: new value of the scale button
 *
 * Sets the current value of the scale; if the value is outside
 * the minimum or maximum range values, it will be clamped to fit
 * inside them. The scale button emits the #GtkScaleButton::value-changed
 * signal if the value changes.
 *
 * Since: 2.12
 */
void
ctk_scale_button_set_value (GtkScaleButton *button,
			    gdouble         value)
{
  GtkScaleButtonPrivate *priv;

  g_return_if_fail (CTK_IS_SCALE_BUTTON (button));

  priv = button->priv;

  ctk_range_set_value (CTK_RANGE (priv->scale), value);
  g_object_notify (G_OBJECT (button), "value");
}

/**
 * ctk_scale_button_set_icons:
 * @button: a #GtkScaleButton
 * @icons: (array zero-terminated=1): a %NULL-terminated array of icon names
 *
 * Sets the icons to be used by the scale button.
 * For details, see the #GtkScaleButton:icons property.
 *
 * Since: 2.12
 */
void
ctk_scale_button_set_icons (GtkScaleButton  *button,
			    const gchar    **icons)
{
  GtkScaleButtonPrivate *priv;
  gchar **tmp;

  g_return_if_fail (CTK_IS_SCALE_BUTTON (button));

  priv = button->priv;

  tmp = priv->icon_list;
  priv->icon_list = g_strdupv ((gchar **) icons);
  g_strfreev (tmp);
  ctk_scale_button_update_icon (button);

  g_object_notify (G_OBJECT (button), "icons");
}

/**
 * ctk_scale_button_get_adjustment:
 * @button: a #GtkScaleButton
 *
 * Gets the #GtkAdjustment associated with the #GtkScaleButton’s scale.
 * See ctk_range_get_adjustment() for details.
 *
 * Returns: (transfer none): the adjustment associated with the scale
 *
 * Since: 2.12
 */
GtkAdjustment*
ctk_scale_button_get_adjustment	(GtkScaleButton *button)
{
  g_return_val_if_fail (CTK_IS_SCALE_BUTTON (button), NULL);

  return button->priv->adjustment;
}

/**
 * ctk_scale_button_set_adjustment:
 * @button: a #GtkScaleButton
 * @adjustment: a #GtkAdjustment
 *
 * Sets the #GtkAdjustment to be used as a model
 * for the #GtkScaleButton’s scale.
 * See ctk_range_set_adjustment() for details.
 *
 * Since: 2.12
 */
void
ctk_scale_button_set_adjustment	(GtkScaleButton *button,
				 GtkAdjustment  *adjustment)
{
  g_return_if_fail (CTK_IS_SCALE_BUTTON (button));

  if (!adjustment)
    adjustment = ctk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  else
    g_return_if_fail (CTK_IS_ADJUSTMENT (adjustment));

  if (button->priv->adjustment != adjustment)
    {
      if (button->priv->adjustment)
        g_object_unref (button->priv->adjustment);
      button->priv->adjustment = g_object_ref_sink (adjustment);

      if (button->priv->scale)
        ctk_range_set_adjustment (CTK_RANGE (button->priv->scale), adjustment);

      g_object_notify (G_OBJECT (button), "adjustment");
    }
}

/**
 * ctk_scale_button_get_plus_button:
 * @button: a #GtkScaleButton
 *
 * Retrieves the plus button of the #GtkScaleButton.
 *
 * Returns: (transfer none) (type Gtk.Button): the plus button of the #GtkScaleButton as a #GtkButton
 *
 * Since: 2.14
 */
GtkWidget *
ctk_scale_button_get_plus_button (GtkScaleButton *button)
{
  g_return_val_if_fail (CTK_IS_SCALE_BUTTON (button), NULL);

  return button->priv->plus_button;
}

/**
 * ctk_scale_button_get_minus_button:
 * @button: a #GtkScaleButton
 *
 * Retrieves the minus button of the #GtkScaleButton.
 *
 * Returns: (transfer none) (type Gtk.Button): the minus button of the #GtkScaleButton as a #GtkButton
 *
 * Since: 2.14
 */
GtkWidget *
ctk_scale_button_get_minus_button (GtkScaleButton *button)
{
  g_return_val_if_fail (CTK_IS_SCALE_BUTTON (button), NULL);

  return button->priv->minus_button;
}

/**
 * ctk_scale_button_get_popup:
 * @button: a #GtkScaleButton
 *
 * Retrieves the popup of the #GtkScaleButton.
 *
 * Returns: (transfer none): the popup of the #GtkScaleButton
 *
 * Since: 2.14
 */
GtkWidget *
ctk_scale_button_get_popup (GtkScaleButton *button)
{
  g_return_val_if_fail (CTK_IS_SCALE_BUTTON (button), NULL);

  return button->priv->dock;
}

static void
apply_orientation (GtkScaleButton *button,
                   GtkOrientation  orientation)
{
  GtkScaleButtonPrivate *priv = button->priv;

  if (priv->applied_orientation != orientation)
    {
      priv->applied_orientation = orientation;
      ctk_orientable_set_orientation (CTK_ORIENTABLE (priv->box), orientation);
      ctk_container_child_set (CTK_CONTAINER (priv->box),
                               priv->plus_button,
                               "pack-type",
                               orientation == CTK_ORIENTATION_VERTICAL ?
                               CTK_PACK_START : CTK_PACK_END,
                               NULL);
      ctk_container_child_set (CTK_CONTAINER (priv->box),
                               priv->minus_button,
                               "pack-type",
                               orientation == CTK_ORIENTATION_VERTICAL ?
                               CTK_PACK_END : CTK_PACK_START,
                               NULL);

      ctk_orientable_set_orientation (CTK_ORIENTABLE (priv->scale), orientation);

      if (orientation == CTK_ORIENTATION_VERTICAL)
        {
          ctk_widget_set_size_request (CTK_WIDGET (priv->scale), -1, SCALE_SIZE);
          ctk_range_set_inverted (CTK_RANGE (priv->scale), TRUE);
        }
      else
        {
          ctk_widget_set_size_request (CTK_WIDGET (priv->scale), SCALE_SIZE, -1);
          ctk_range_set_inverted (CTK_RANGE (priv->scale), FALSE);
        }
    }
}

static void
ctk_scale_button_set_orientation_private (GtkScaleButton *button,
                                          GtkOrientation  orientation)
{
  GtkScaleButtonPrivate *priv = button->priv;

  if (priv->orientation != orientation)
    {
      priv->orientation = orientation;
      g_object_notify (G_OBJECT (button), "orientation");
    }
}

/*
 * button callbacks.
 */

static gboolean
ctk_scale_button_scroll (GtkWidget      *widget,
			 GdkEventScroll *event)
{
  GtkScaleButton *button;
  GtkScaleButtonPrivate *priv;
  GtkAdjustment *adjustment;
  gdouble d;

  button = CTK_SCALE_BUTTON (widget);
  priv = button->priv;
  adjustment = priv->adjustment;

  if (event->type != GDK_SCROLL)
    return FALSE;

  d = ctk_scale_button_get_value (button);
  if (event->direction == GDK_SCROLL_UP)
    {
      d += ctk_adjustment_get_step_increment (adjustment);
      if (d > ctk_adjustment_get_upper (adjustment))
	d = ctk_adjustment_get_upper (adjustment);
    }
  else if (event->direction == GDK_SCROLL_DOWN)
    {
      d -= ctk_adjustment_get_step_increment (adjustment);
      if (d < ctk_adjustment_get_lower (adjustment))
	d = ctk_adjustment_get_lower (adjustment);
    }
  else if (event->direction == GDK_SCROLL_SMOOTH)
    {
      d -= event->delta_y * ctk_adjustment_get_step_increment (adjustment);
      d = CLAMP (d, ctk_adjustment_get_lower (adjustment),
                 ctk_adjustment_get_upper (adjustment));
    }
  ctk_scale_button_set_value (button, d);

  return TRUE;
}

static gboolean
ctk_scale_popup (GtkWidget *widget)
{
  GtkScaleButton *button = CTK_SCALE_BUTTON (widget);
  GtkScaleButtonPrivate *priv = button->priv;
  GtkWidget *toplevel;
  GtkBorder border;
  GtkRequisition req;
  gint w, h;
  gint size;

  ctk_popover_popup (CTK_POPOVER (priv->dock));

  toplevel = ctk_widget_get_toplevel (widget);
  _ctk_window_get_shadow_width (CTK_WINDOW (toplevel), &border);
  w = ctk_widget_get_allocated_width (toplevel) - border.left - border.right;
  h = ctk_widget_get_allocated_height (toplevel) - border.top - border.bottom;
  ctk_widget_get_preferred_size (priv->dock, NULL, &req);
  size = MAX (req.width, req.height);

  if (size > w)
    apply_orientation (button, CTK_ORIENTATION_VERTICAL);
  else if (size > h)
    apply_orientation (button, CTK_ORIENTATION_HORIZONTAL);
  else
    apply_orientation (button, priv->orientation);

  return TRUE;
}

static void
ctk_scale_button_popdown (GtkWidget *widget)
{
  GtkScaleButton *button = CTK_SCALE_BUTTON (widget);
  GtkScaleButtonPrivate *priv = button->priv;

  ctk_popover_popdown (CTK_POPOVER (priv->dock));
}

static void
ctk_scale_button_clicked (GtkButton *button)
{
  ctk_scale_popup (CTK_WIDGET (button));
}

static void
ctk_scale_button_popup (GtkWidget *widget)
{
  ctk_scale_popup (widget);
}

/*
 * +/- button callbacks.
 */
static gboolean
button_click (GtkScaleButton *button,
              GtkWidget      *active)
{
  GtkScaleButtonPrivate *priv = button->priv;
  GtkAdjustment *adjustment = priv->adjustment;
  gboolean can_continue = TRUE;
  gdouble val;

  val = ctk_scale_button_get_value (button);

  if (active == priv->plus_button)
    val += ctk_adjustment_get_page_increment (adjustment);
  else
    val -= ctk_adjustment_get_page_increment (adjustment);

  if (val <= ctk_adjustment_get_lower (adjustment))
    {
      can_continue = FALSE;
      val = ctk_adjustment_get_lower (adjustment);
    }
  else if (val > ctk_adjustment_get_upper (adjustment))
    {
      can_continue = FALSE;
      val = ctk_adjustment_get_upper (adjustment);
    }

  ctk_scale_button_set_value (button, val);

  return can_continue;
}

static gboolean
cb_button_timeout (gpointer user_data)
{
  GtkScaleButton *button = CTK_SCALE_BUTTON (user_data);
  GtkScaleButtonPrivate *priv = button->priv;
  gboolean res;

  if (priv->click_id == 0)
    return G_SOURCE_REMOVE;

  res = button_click (button, priv->active_button);
  if (!res)
    {
      g_source_remove (priv->click_id);
      priv->click_id = 0;
    }

  return res;
}

static gboolean
cb_button_press (GtkWidget      *widget,
		 GdkEventButton *event,
		 gpointer        user_data)
{
  GtkScaleButton *button = CTK_SCALE_BUTTON (user_data);
  GtkScaleButtonPrivate *priv = button->priv;
  gint double_click_time;

  if (priv->click_id != 0)
    g_source_remove (priv->click_id);

  priv->active_button = widget;

  g_object_get (ctk_widget_get_settings (widget),
                "ctk-double-click-time", &double_click_time,
                NULL);
  priv->click_id = gdk_threads_add_timeout (double_click_time,
                                            cb_button_timeout,
                                            button);
  g_source_set_name_by_id (priv->click_id, "[ctk+] cb_button_timeout");
  cb_button_timeout (button);

  return TRUE;
}

static gboolean
cb_button_release (GtkWidget      *widget,
		   GdkEventButton *event,
		   gpointer        user_data)
{
  GtkScaleButton *button = CTK_SCALE_BUTTON (user_data);
  GtkScaleButtonPrivate *priv = button->priv;

  if (priv->click_id != 0)
    {
      g_source_remove (priv->click_id);
      priv->click_id = 0;
    }

  return TRUE;
}

static void
cb_button_clicked (GtkWidget *widget,
                   gpointer   user_data)
{
  GtkScaleButton *button = CTK_SCALE_BUTTON (user_data);
  GtkScaleButtonPrivate *priv = button->priv;

  if (priv->click_id != 0)
    return;

  button_click (button, widget);
}

static void
ctk_scale_button_update_icon (GtkScaleButton *button)
{
  GtkScaleButtonPrivate *priv = button->priv;
  GtkAdjustment *adjustment;
  gdouble value;
  const gchar *name;
  guint num_icons;

  if (!priv->icon_list || priv->icon_list[0][0] == '\0')
    {
      ctk_image_set_from_icon_name (CTK_IMAGE (priv->image),
                                    "image-missing",
                                    priv->size);
      return;
    }

  num_icons = g_strv_length (priv->icon_list);

  /* The 1-icon special case */
  if (num_icons == 1)
    {
      ctk_image_set_from_icon_name (CTK_IMAGE (priv->image),
                                    priv->icon_list[0],
                                    priv->size);
      return;
    }

  adjustment = priv->adjustment;
  value = ctk_scale_button_get_value (button);

  /* The 2-icons special case */
  if (num_icons == 2)
    {
      gdouble limit;

      limit = (ctk_adjustment_get_upper (adjustment) - ctk_adjustment_get_lower (adjustment)) / 2 + ctk_adjustment_get_lower (adjustment);
      if (value < limit)
        name = priv->icon_list[0];
      else
        name = priv->icon_list[1];

      ctk_image_set_from_icon_name (CTK_IMAGE (priv->image),
                                    name,
                                    priv->size);
      return;
    }

  /* With 3 or more icons */
  if (value == ctk_adjustment_get_lower (adjustment))
    {
      name = priv->icon_list[0];
    }
  else if (value == ctk_adjustment_get_upper (adjustment))
    {
      name = priv->icon_list[1];
    }
  else
    {
      gdouble step;
      guint i;

      step = (ctk_adjustment_get_upper (adjustment) - ctk_adjustment_get_lower (adjustment)) / (num_icons - 2); i = (guint) ((value - ctk_adjustment_get_lower (adjustment)) / step) + 2;
      g_assert (i < num_icons);
      name = priv->icon_list[i];
    }

  ctk_image_set_from_icon_name (CTK_IMAGE (priv->image),
                                name,
                                priv->size);
}

static void
cb_scale_value_changed (GtkRange *range,
                        gpointer  user_data)
{
  GtkScaleButton *button = user_data;
  gdouble value;
  gdouble upper, lower;

  value = ctk_range_get_value (range);
  upper = ctk_adjustment_get_upper (button->priv->adjustment);
  lower = ctk_adjustment_get_lower (button->priv->adjustment);

  ctk_scale_button_update_icon (button);

  ctk_widget_set_sensitive (button->priv->plus_button, value < upper);
  ctk_widget_set_sensitive (button->priv->minus_button, lower < value);

  g_signal_emit (button, signals[VALUE_CHANGED], 0, value);
  g_object_notify (G_OBJECT (button), "value");
}

static void
cb_popup_mapped (GtkWidget *popup,
                 gpointer   user_data)
{
  GtkScaleButton *button = user_data;
  GtkScaleButtonPrivate *priv = button->priv;

  ctk_widget_grab_focus (priv->scale);
}
