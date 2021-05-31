/* GTK - The GIMP Toolkit
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#include "ctkcheckbutton.h"

#include "ctkbuttonprivate.h"
#include "ctklabel.h"

#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkrender.h"
#include "ctkwidgetprivate.h"
#include "ctkbuiltiniconprivate.h"
#include "ctkcssnodeprivate.h"
#include "ctkboxgadgetprivate.h"
#include "ctkcontainerprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkcssnumbervalueprivate.h"
#include "ctkradiobutton.h"


/**
 * SECTION:ctkcheckbutton
 * @Short_description: Create widgets with a discrete toggle button
 * @Title: GtkCheckButton
 * @See_also: #GtkCheckMenuItem, #GtkButton, #GtkToggleButton, #GtkRadioButton
 *
 * A #GtkCheckButton places a discrete #GtkToggleButton next to a widget,
 * (usually a #GtkLabel). See the section on #GtkToggleButton widgets for
 * more information about toggle/check buttons.
 *
 * The important signal ( #GtkToggleButton::toggled ) is also inherited from
 * #GtkToggleButton.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * checkbutton
 * ├── check
 * ╰── <child>
 * ]|
 *
 * A GtkCheckButton with indicator (see ctk_toggle_button_set_mode()) has a
 * main CSS node with name checkbutton and a subnode with name check.
 *
 * |[<!-- language="plain" -->
 * button.check
 * ├── check
 * ╰── <child>
 * ]|
 *
 * A GtkCheckButton without indicator changes the name of its main node
 * to button and adds a .check style class to it. The subnode is invisible
 * in this case.
 */


#define INDICATOR_SIZE     16
#define INDICATOR_SPACING  2


static void ctk_check_button_get_preferred_width                         (GtkWidget          *widget,
                                                                          gint               *minimum,
                                                                          gint               *natural);
static void ctk_check_button_get_preferred_width_for_height              (GtkWidget          *widget,
                                                                          gint                height,
                                                                          gint               *minimum,
                                                                          gint               *natural);
static void ctk_check_button_get_preferred_height                        (GtkWidget          *widget,
                                                                          gint               *minimum,
                                                                          gint               *natural);
static void ctk_check_button_get_preferred_height_for_width              (GtkWidget          *widget,
                                                                          gint                width,
                                                                          gint               *minimum,
                                                                          gint               *natural);
static void ctk_check_button_get_preferred_height_and_baseline_for_width (GtkWidget          *widget,
									  gint                width,
									  gint               *minimum,
									  gint               *natural,
									  gint               *minimum_baseline,
									  gint               *natural_baseline);
static void ctk_check_button_size_allocate       (GtkWidget           *widget,
						  GtkAllocation       *allocation);
static gboolean ctk_check_button_draw            (GtkWidget           *widget,
						  cairo_t             *cr);

typedef struct {
  GtkCssGadget *gadget;
  GtkCssGadget *indicator_gadget;
} GtkCheckButtonPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GtkCheckButton, ctk_check_button, CTK_TYPE_TOGGLE_BUTTON)

static void
ctk_check_button_update_node_state (GtkWidget *widget)
{
  GtkCheckButtonPrivate *priv = ctk_check_button_get_instance_private (CTK_CHECK_BUTTON (widget));
  GtkCssImageBuiltinType image_type;
  GtkStateFlags state;

  state = ctk_widget_get_state_flags (widget);

  /* XXX: This is somewhat awkward here, but there's no better
   * way to update the icon
   */
  if (state & CTK_STATE_FLAG_CHECKED)
    image_type = CTK_IS_RADIO_BUTTON (widget) ? CTK_CSS_IMAGE_BUILTIN_OPTION : CTK_CSS_IMAGE_BUILTIN_CHECK;
  else if (state & CTK_STATE_FLAG_INCONSISTENT)
    image_type = CTK_IS_RADIO_BUTTON (widget) ? CTK_CSS_IMAGE_BUILTIN_OPTION_INCONSISTENT : CTK_CSS_IMAGE_BUILTIN_CHECK_INCONSISTENT;
  else
    image_type = CTK_CSS_IMAGE_BUILTIN_NONE;
  ctk_builtin_icon_set_image (CTK_BUILTIN_ICON (priv->indicator_gadget), image_type);

  ctk_css_gadget_set_state (priv->indicator_gadget, state);
}


static void
ctk_check_button_state_flags_changed (GtkWidget     *widget,
				      GtkStateFlags  previous_state_flags)
{
  ctk_check_button_update_node_state (widget);

  CTK_WIDGET_CLASS (ctk_check_button_parent_class)->state_flags_changed (widget, previous_state_flags);
}

static void
ctk_check_button_direction_changed (GtkWidget        *widget,
                                    GtkTextDirection  previous_direction)
{
  GtkCheckButtonPrivate *priv = ctk_check_button_get_instance_private (CTK_CHECK_BUTTON (widget));

  ctk_box_gadget_reverse_children (CTK_BOX_GADGET (priv->gadget));
  ctk_box_gadget_set_allocate_reverse (CTK_BOX_GADGET (priv->gadget),
                                       ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL);
  ctk_box_gadget_set_align_reverse (CTK_BOX_GADGET (priv->gadget),
                                    ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL);

  CTK_WIDGET_CLASS (ctk_check_button_parent_class)->direction_changed (widget, previous_direction);
}

static void
ctk_check_button_finalize (GObject *object)
{
  GtkCheckButtonPrivate *priv = ctk_check_button_get_instance_private (CTK_CHECK_BUTTON (object));

  g_clear_object (&priv->gadget);
  g_clear_object (&priv->indicator_gadget);

  G_OBJECT_CLASS (ctk_check_button_parent_class)->finalize (object);
}

static void
ctk_check_button_add (GtkContainer *container,
                      GtkWidget    *widget)
{
  GtkCheckButtonPrivate *priv = ctk_check_button_get_instance_private (CTK_CHECK_BUTTON (container));
  int pos;

  CTK_CONTAINER_CLASS (ctk_check_button_parent_class)->add (container, widget);

  pos = ctk_widget_get_direction (CTK_WIDGET (container)) == CTK_TEXT_DIR_RTL ? 0 : 1;
  ctk_box_gadget_insert_widget (CTK_BOX_GADGET (priv->gadget), pos, widget);
  ctk_box_gadget_set_gadget_expand (CTK_BOX_GADGET (priv->gadget), G_OBJECT (widget), TRUE);
}

static void
ctk_check_button_remove (GtkContainer *container,
                         GtkWidget    *widget)
{
  GtkCheckButtonPrivate *priv = ctk_check_button_get_instance_private (CTK_CHECK_BUTTON (container));

  ctk_box_gadget_remove_widget (CTK_BOX_GADGET (priv->gadget), widget);

  CTK_CONTAINER_CLASS (ctk_check_button_parent_class)->remove (container, widget);
}

static void
ctk_check_button_class_init (GtkCheckButtonClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = CTK_CONTAINER_CLASS (class);

  object_class->finalize = ctk_check_button_finalize;

  widget_class->get_preferred_width = ctk_check_button_get_preferred_width;
  widget_class->get_preferred_width_for_height = ctk_check_button_get_preferred_width_for_height;
  widget_class->get_preferred_height = ctk_check_button_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_check_button_get_preferred_height_for_width;
  widget_class->get_preferred_height_and_baseline_for_width = ctk_check_button_get_preferred_height_and_baseline_for_width;
  widget_class->size_allocate = ctk_check_button_size_allocate;
  widget_class->draw = ctk_check_button_draw;
  widget_class->state_flags_changed = ctk_check_button_state_flags_changed;
  widget_class->direction_changed = ctk_check_button_direction_changed;

  container_class->add = ctk_check_button_add;
  container_class->remove = ctk_check_button_remove;

  /**
   * GtkCheckButton:indicator-size:
   *
   * The size of the indicator.
   *
   * Deprecated: 3.20: Use CSS min-width and min-height on the indicator node.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("indicator-size",
							     P_("Indicator Size"),
							     P_("Size of check or radio indicator"),
							     0,
							     G_MAXINT,
							     INDICATOR_SIZE,
							     CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * GtkCheckButton:indicator-spacing:
   *
   * The spacing around the indicator.
   *
   * Deprecated: 3.20: Use CSS margins of the indicator node,
   *    the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("indicator-spacing",
							     P_("Indicator Spacing"),
							     P_("Spacing around check or radio indicator"),
							     0,
							     G_MAXINT,
							     INDICATOR_SPACING,
							     CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_CHECK_BOX);
  ctk_widget_class_set_css_name (widget_class, "checkbutton");
}

static void
draw_indicator_changed (GObject    *object,
                        GParamSpec *pspec,
                        gpointer    user_data)
{
  GtkButton *button = CTK_BUTTON (object);
  GtkCheckButtonPrivate *priv = ctk_check_button_get_instance_private (CTK_CHECK_BUTTON (button));
  GtkCssNode *widget_node;
  GtkCssNode *indicator_node;

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (button));
  indicator_node = ctk_css_gadget_get_node (priv->indicator_gadget);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (ctk_toggle_button_get_mode (CTK_TOGGLE_BUTTON (button)))
    {
      ctk_button_set_alignment (button, 0.0, 0.5);
      ctk_css_node_set_visible (indicator_node, TRUE);
      if (CTK_IS_RADIO_BUTTON (button))
        {
          ctk_css_node_remove_class (widget_node, g_quark_from_static_string ("radio"));
          ctk_css_node_set_name (widget_node, I_("radiobutton"));
        }
      else if (CTK_IS_CHECK_BUTTON (button))
        {
          ctk_css_node_remove_class (widget_node, g_quark_from_static_string ("check"));
          ctk_css_node_set_name (widget_node, I_("checkbutton"));
        }
    }
  else
    {
      ctk_button_set_alignment (button, 0.5, 0.5);
      ctk_css_node_set_visible (indicator_node, FALSE);
      if (CTK_IS_RADIO_BUTTON (button))
        {
          ctk_css_node_add_class (widget_node, g_quark_from_static_string ("radio"));
          ctk_css_node_set_name (widget_node, I_("button"));
        }
      else if (CTK_IS_CHECK_BUTTON (button))
        {
          ctk_css_node_add_class (widget_node, g_quark_from_static_string ("check"));
          ctk_css_node_set_name (widget_node, I_("button"));
        }
    }
G_GNUC_END_IGNORE_DEPRECATIONS
}

static void
ctk_check_button_init (GtkCheckButton *check_button)
{
  GtkCheckButtonPrivate *priv = ctk_check_button_get_instance_private (check_button);
  GtkCssNode *widget_node;

  ctk_widget_set_receives_default (CTK_WIDGET (check_button), FALSE);
  g_signal_connect (check_button, "notify::draw-indicator", G_CALLBACK (draw_indicator_changed), NULL);
  ctk_toggle_button_set_mode (CTK_TOGGLE_BUTTON (check_button), TRUE);

  ctk_style_context_remove_class (ctk_widget_get_style_context (CTK_WIDGET (check_button)), "toggle");

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (check_button));
  priv->gadget = ctk_box_gadget_new_for_node (widget_node, CTK_WIDGET (check_button));
  ctk_box_gadget_set_orientation (CTK_BOX_GADGET (priv->gadget), CTK_ORIENTATION_HORIZONTAL);
  ctk_box_gadget_set_draw_focus (CTK_BOX_GADGET (priv->gadget), TRUE);
  priv->indicator_gadget = ctk_builtin_icon_new ("check",
                                                 CTK_WIDGET (check_button),
                                                 priv->gadget,
                                                 NULL);
  ctk_builtin_icon_set_default_size_property (CTK_BUILTIN_ICON (priv->indicator_gadget), "indicator-size");
  ctk_box_gadget_insert_gadget (CTK_BOX_GADGET (priv->gadget), 0, priv->indicator_gadget, FALSE, CTK_ALIGN_BASELINE);

  ctk_check_button_update_node_state (CTK_WIDGET (check_button));
}

/**
 * ctk_check_button_new:
 *
 * Creates a new #GtkCheckButton.
 *
 * Returns: a #GtkWidget.
 */
GtkWidget*
ctk_check_button_new (void)
{
  return g_object_new (CTK_TYPE_CHECK_BUTTON, NULL);
}


/**
 * ctk_check_button_new_with_label:
 * @label: the text for the check button.
 *
 * Creates a new #GtkCheckButton with a #GtkLabel to the right of it.
 *
 * Returns: a #GtkWidget.
 */
GtkWidget*
ctk_check_button_new_with_label (const gchar *label)
{
  return g_object_new (CTK_TYPE_CHECK_BUTTON, "label", label, NULL);
}

/**
 * ctk_check_button_new_with_mnemonic:
 * @label: The text of the button, with an underscore in front of the
 *   mnemonic character
 *
 * Creates a new #GtkCheckButton containing a label. The label
 * will be created using ctk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the check button.
 *
 * Returns: a new #GtkCheckButton
 */
GtkWidget*
ctk_check_button_new_with_mnemonic (const gchar *label)
{
  return g_object_new (CTK_TYPE_CHECK_BUTTON, 
                       "label", label, 
                       "use-underline", TRUE, 
                       NULL);
}

static void
ctk_check_button_get_preferred_width_for_height (GtkWidget *widget,
                                                 gint       height,
                                                 gint      *minimum,
                                                 gint      *natural)
{
  GtkCheckButtonPrivate *priv = ctk_check_button_get_instance_private (CTK_CHECK_BUTTON (widget));
  GtkCssGadget *gadget;

  if (ctk_toggle_button_get_mode (CTK_TOGGLE_BUTTON (widget)))
    gadget = priv->gadget;
  else
    gadget = CTK_BUTTON (widget)->priv->gadget;

  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_check_button_get_preferred_width (GtkWidget *widget,
                                      gint      *minimum,
                                      gint      *natural)
{
  GtkCheckButtonPrivate *priv = ctk_check_button_get_instance_private (CTK_CHECK_BUTTON (widget));
  GtkCssGadget *gadget;

  if (ctk_toggle_button_get_mode (CTK_TOGGLE_BUTTON (widget)))
    gadget = priv->gadget;
  else
    gadget = CTK_BUTTON (widget)->priv->gadget;

  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_check_button_get_preferred_height_and_baseline_for_width (GtkWidget *widget,
							      gint       width,
							      gint      *minimum,
							      gint      *natural,
							      gint      *minimum_baseline,
							      gint      *natural_baseline)
{
  GtkCheckButtonPrivate *priv = ctk_check_button_get_instance_private (CTK_CHECK_BUTTON (widget));
  GtkCssGadget *gadget;

  if (ctk_toggle_button_get_mode (CTK_TOGGLE_BUTTON (widget)))
    gadget = priv->gadget;
  else
    gadget = CTK_BUTTON (widget)->priv->gadget;

  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     minimum_baseline, natural_baseline);
}

static void
ctk_check_button_get_preferred_height_for_width (GtkWidget *widget,
                                                 gint       width,
                                                 gint      *minimum,
                                                 gint      *natural)
{
  GtkCheckButtonPrivate *priv = ctk_check_button_get_instance_private (CTK_CHECK_BUTTON (widget));
  GtkCssGadget *gadget;

  if (ctk_toggle_button_get_mode (CTK_TOGGLE_BUTTON (widget)))
    gadget = priv->gadget;
  else
    gadget = CTK_BUTTON (widget)->priv->gadget;

  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_check_button_get_preferred_height (GtkWidget *widget,
                                       gint      *minimum,
                                       gint      *natural)
{
  GtkCheckButtonPrivate *priv = ctk_check_button_get_instance_private (CTK_CHECK_BUTTON (widget));
  GtkCssGadget *gadget;

  if (ctk_toggle_button_get_mode (CTK_TOGGLE_BUTTON (widget)))
    gadget = priv->gadget;
  else
    gadget = CTK_BUTTON (widget)->priv->gadget;

  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_check_button_size_allocate (GtkWidget     *widget,
				GtkAllocation *allocation)
{
  GtkCheckButtonPrivate *priv = ctk_check_button_get_instance_private (CTK_CHECK_BUTTON (widget));
  GtkButton *button = CTK_BUTTON (widget);
  GtkCssGadget *gadget;
  GdkRectangle clip;
  PangoContext *pango_context;
  PangoFontMetrics *metrics;

  if (ctk_toggle_button_get_mode (CTK_TOGGLE_BUTTON (widget)))
    gadget = priv->gadget;
  else
    gadget = button->priv->gadget;

  ctk_widget_set_allocation (widget, allocation);
  ctk_css_gadget_allocate (gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);

  pango_context = ctk_widget_get_pango_context (widget);
  metrics = pango_context_get_metrics (pango_context,
                                       pango_context_get_font_description (pango_context),
                                       pango_context_get_language (pango_context));
  button->priv->baseline_align =
      (double)pango_font_metrics_get_ascent (metrics) /
      (pango_font_metrics_get_ascent (metrics) + pango_font_metrics_get_descent (metrics));
  pango_font_metrics_unref (metrics);

  if (ctk_widget_get_realized (widget))
    {
      GtkAllocation border_allocation;
      ctk_css_gadget_get_border_allocation (gadget, &border_allocation, NULL);
      gdk_window_move_resize (CTK_BUTTON (widget)->priv->event_window,
                              border_allocation.x,
                              border_allocation.y,
                              border_allocation.width,
                              border_allocation.height);
    }
}

static gint
ctk_check_button_draw (GtkWidget *widget,
                       cairo_t   *cr)
{
  GtkCheckButtonPrivate *priv = ctk_check_button_get_instance_private (CTK_CHECK_BUTTON (widget));
  GtkCssGadget *gadget;

  if (ctk_toggle_button_get_mode (CTK_TOGGLE_BUTTON (widget)))
    gadget = priv->gadget;
  else
    gadget = CTK_BUTTON (widget)->priv->gadget;

  ctk_css_gadget_draw (gadget, cr);

  return FALSE;
}

GtkCssNode *
ctk_check_button_get_indicator_node (GtkCheckButton *check_button)
{
  GtkCheckButtonPrivate *priv = ctk_check_button_get_instance_private (check_button);

  return ctk_css_gadget_get_node (priv->indicator_gadget);
}
