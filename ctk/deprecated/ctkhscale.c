/* CTK - The GIMP Toolkit
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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include <math.h>
#include <stdlib.h>

#include "ctkhscale.h"

#include "ctkadjustment.h"
#include "ctkorientable.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * SECTION:ctkhscale
 * @Short_description: A horizontal slider widget for selecting a value from a range
 * @Title: CtkHScale
 *
 * The #CtkHScale widget is used to allow the user to select a value using
 * a horizontal slider. To create one, use ctk_hscale_new_with_range().
 *
 * The position to show the current value, and the number of decimal places
 * shown can be set using the parent #CtkScale class’s functions.
 *
 * CtkHScale has been deprecated, use #CtkScale instead.
 */


G_DEFINE_TYPE (CtkHScale, ctk_hscale, CTK_TYPE_SCALE)

static void
ctk_hscale_class_init (CtkHScaleClass *class)
{
  CtkRangeClass *range_class = CTK_RANGE_CLASS (class);

  range_class->slider_detail = "hscale";
}

static void
ctk_hscale_init (CtkHScale *hscale)
{
  ctk_orientable_set_orientation (CTK_ORIENTABLE (hscale),
                                  CTK_ORIENTATION_HORIZONTAL);
}

/**
 * ctk_hscale_new:
 * @adjustment: (nullable): the #CtkAdjustment which sets the range of
 * the scale.
 *
 * Creates a new #CtkHScale.
 *
 * Returns: a new #CtkHScale.
 *
 * Deprecated: 3.2: Use ctk_scale_new() with %CTK_ORIENTATION_HORIZONTAL instead
 */
CtkWidget *
ctk_hscale_new (CtkAdjustment *adjustment)
{
  g_return_val_if_fail (adjustment == NULL || CTK_IS_ADJUSTMENT (adjustment),
                        NULL);

  return g_object_new (CTK_TYPE_HSCALE,
                       "adjustment", adjustment,
                       NULL);
}

/**
 * ctk_hscale_new_with_range:
 * @min: minimum value
 * @max: maximum value
 * @step: step increment (tick size) used with keyboard shortcuts
 *
 * Creates a new horizontal scale widget that lets the user input a
 * number between @min and @max (including @min and @max) with the
 * increment @step.  @step must be nonzero; it’s the distance the
 * slider moves when using the arrow keys to adjust the scale value.
 *
 * Note that the way in which the precision is derived works best if @step
 * is a power of ten. If the resulting precision is not suitable for your
 * needs, use ctk_scale_set_digits() to correct it.
 *
 * Returns: a new #CtkHScale
 *
 * Deprecated: 3.2: Use ctk_scale_new_with_range() with %CTK_ORIENTATION_HORIZONTAL instead
 **/
CtkWidget *
ctk_hscale_new_with_range (gdouble min,
                           gdouble max,
                           gdouble step)
{
  CtkAdjustment *adj;
  CtkScale *scale;
  gint digits;

  g_return_val_if_fail (min < max, NULL);
  g_return_val_if_fail (step != 0.0, NULL);

  adj = ctk_adjustment_new (min, min, max, step, 10 * step, 0);

  if (fabs (step) >= 1.0 || step == 0.0)
    {
      digits = 0;
    }
  else
    {
      digits = abs ((gint) floor (log10 (fabs (step))));
      if (digits > 5)
        digits = 5;
    }

  scale = g_object_new (CTK_TYPE_HSCALE,
                        "adjustment", adj,
                        "digits", digits,
                        NULL);

  return CTK_WIDGET (scale);
}
