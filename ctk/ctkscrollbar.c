/* GTK - The GIMP Toolkit
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include "ctkscrollbar.h"
#include "ctkrangeprivate.h"

#include "ctkadjustment.h"
#include "ctkintl.h"
#include "ctkorientable.h"
#include "ctkprivate.h"


/**
 * SECTION:ctkscrollbar
 * @Short_description: A Scrollbar
 * @Title: CtkScrollbar
 * @See_also: #CtkAdjustment, #CtkScrolledWindow
 *
 * The #CtkScrollbar widget is a horizontal or vertical scrollbar,
 * depending on the value of the #CtkOrientable:orientation property.
 *
 * Its position and movement are controlled by the adjustment that is passed to
 * or created by ctk_scrollbar_new(). See #CtkAdjustment for more details. The
 * #CtkAdjustment:value field sets the position of the thumb and must be between
 * #CtkAdjustment:lower and #CtkAdjustment:upper - #CtkAdjustment:page-size. The
 * #CtkAdjustment:page-size represents the size of the visible scrollable area.
 * The fields #CtkAdjustment:step-increment and #CtkAdjustment:page-increment
 * fields are added to or subtracted from the #CtkAdjustment:value when the user
 * asks to move by a step (using e.g. the cursor arrow keys or, if present, the
 * stepper buttons) or by a page (using e.g. the Page Down/Up keys).
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * scrollbar[.fine-tune]
 * ╰── contents
 *     ├── [button.up]
 *     ├── [button.down]
 *     ├── trough
 *     │   ╰── slider
 *     ├── [button.up]
 *     ╰── [button.down]
 * ]|
 *
 * CtkScrollbar has a main CSS node with name scrollbar and a subnode for its
 * contents, with subnodes named trough and slider.
 *
 * The main node gets the style class .fine-tune added when the scrollbar is
 * in 'fine-tuning' mode.
 *
 * If steppers are enabled, they are represented by up to four additional
 * subnodes with name button. These get the style classes .up and .down to
 * indicate in which direction they are moving.
 *
 * Other style classes that may be added to scrollbars inside #CtkScrolledWindow
 * include the positional classes (.left, .right, .top, .bottom) and style
 * classes related to overlay scrolling (.overlay-indicator, .dragging, .hovering).
 */


static void ctk_scrollbar_style_updated (CtkWidget *widget);

G_DEFINE_TYPE (CtkScrollbar, ctk_scrollbar, CTK_TYPE_RANGE)

static void
ctk_scrollbar_class_init (CtkScrollbarClass *class)
{
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  widget_class->style_updated = ctk_scrollbar_style_updated;

  /**
   * CtkScrollbar:min-slider-length:
   *
   * Minimum length of scrollbar slider.
   *
   * Deprecated: 3.20: Use min-height/min-width CSS properties on the slider
   *   element instead. The value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("min-slider-length",
							     P_("Minimum Slider Length"),
							     P_("Minimum length of scrollbar slider"),
							     0,
							     G_MAXINT,
							     21,
							     CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("fixed-slider-length",
                                                                 P_("Fixed slider size"),
                                                                 P_("Don't change slider size, just lock it to the minimum length"),
                                                                 FALSE,
                                                                 CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("has-backward-stepper",
                                                                 P_("Backward stepper"),
                                                                 P_("Display the standard backward arrow button"),
                                                                 TRUE,
                                                                 CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("has-forward-stepper",
                                                                 P_("Forward stepper"),
                                                                 P_("Display the standard forward arrow button"),
                                                                 TRUE,
                                                                 CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("has-secondary-backward-stepper",
                                                                 P_("Secondary backward stepper"),
                                                                 P_("Display a second backward arrow button on the opposite end of the scrollbar"),
                                                                 FALSE,
                                                                 CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("has-secondary-forward-stepper",
                                                                 P_("Secondary forward stepper"),
                                                                 P_("Display a second forward arrow button on the opposite end of the scrollbar"),
                                                                 FALSE,
                                                                 CTK_PARAM_READABLE));

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_SCROLL_BAR);
  ctk_widget_class_set_css_name (widget_class, "scrollbar");
}

static void
ctk_scrollbar_update_style (CtkScrollbar *scrollbar)
{
  gboolean fixed_size;
  gboolean has_a, has_b, has_c, has_d;
  CtkRange *range = CTK_RANGE (scrollbar);
  CtkWidget *widget = CTK_WIDGET (scrollbar);

  ctk_widget_style_get (widget,
                        "fixed-slider-length", &fixed_size,
                        "has-backward-stepper", &has_a,
                        "has-secondary-forward-stepper", &has_b,
                        "has-secondary-backward-stepper", &has_c,
                        "has-forward-stepper", &has_d,
                        NULL);

  ctk_range_set_slider_size_fixed (range, fixed_size);
  _ctk_range_set_steppers (range, has_a, has_b, has_c, has_d);
}

static void
ctk_scrollbar_init (CtkScrollbar *scrollbar)
{
  ctk_scrollbar_update_style (scrollbar);
  ctk_range_set_slider_use_min_size (CTK_RANGE (scrollbar), TRUE);
}

static void
ctk_scrollbar_style_updated (CtkWidget *widget)
{
  ctk_scrollbar_update_style (CTK_SCROLLBAR (widget));
  CTK_WIDGET_CLASS (ctk_scrollbar_parent_class)->style_updated (widget);
}

/**
 * ctk_scrollbar_new:
 * @orientation: the scrollbar’s orientation.
 * @adjustment: (allow-none): the #CtkAdjustment to use, or %NULL to create a new adjustment.
 *
 * Creates a new scrollbar with the given orientation.
 *
 * Returns:  the new #CtkScrollbar.
 *
 * Since: 3.0
 **/
CtkWidget *
ctk_scrollbar_new (CtkOrientation  orientation,
                   CtkAdjustment  *adjustment)
{
  g_return_val_if_fail (adjustment == NULL || CTK_IS_ADJUSTMENT (adjustment),
                        NULL);

  return g_object_new (CTK_TYPE_SCROLLBAR,
                       "orientation", orientation,
                       "adjustment",  adjustment,
                       NULL);
}
