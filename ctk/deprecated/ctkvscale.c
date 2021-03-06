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

#include "ctkvscale.h"

#include "ctkadjustment.h"
#include "ctkorientable.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * SECTION:ctkvscale
 * @Short_description: A vertical slider widget for selecting a value from a range
 * @Title: CtkVScale
 *
 * The #CtkVScale widget is used to allow the user to select a value using
 * a vertical slider. To create one, use ctk_hscale_new_with_range().
 *
 * The position to show the current value, and the number of decimal places
 * shown can be set using the parent #CtkScale class’s functions.
 *
 * CtkVScale has been deprecated, use #CtkScale instead.
 */

G_DEFINE_TYPE (CtkVScale, ctk_vscale, CTK_TYPE_SCALE)

static void
ctk_vscale_class_init (CtkVScaleClass *class)
{
  CtkRangeClass *range_class = CTK_RANGE_CLASS (class);

  range_class->slider_detail = "vscale";
}

static void
ctk_vscale_init (CtkVScale *vscale)
{
  ctk_orientable_set_orientation (CTK_ORIENTABLE (vscale),
                                  CTK_ORIENTATION_VERTICAL);
}
/**
 * ctk_vscale_new:
 * @adjustment: the #CtkAdjustment which sets the range of the scale.
 *
 * Creates a new #CtkVScale.
 *
 * Returns: a new #CtkVScale.
 *
 * Deprecated: 3.2: Use ctk_scale_new() with %CTK_ORIENTATION_VERTICAL instead
 */
CtkWidget *
ctk_vscale_new (CtkAdjustment *adjustment)
{
  g_return_val_if_fail (adjustment == NULL || CTK_IS_ADJUSTMENT (adjustment),
                        NULL);

  return g_object_new (CTK_TYPE_VSCALE,
                       "adjustment", adjustment,
                       NULL);
}

/**
 * ctk_vscale_new_with_range:
 * @min: minimum value
 * @max: maximum value
 * @step: step increment (tick size) used with keyboard shortcuts
 *
 * Creates a new vertical scale widget that lets the user input a
 * number between @min and @max (including @min and @max) with the
 * increment @step.  @step must be nonzero; it’s the distance the
 * slider moves when using the arrow keys to adjust the scale value.
 *
 * Note that the way in which the precision is derived works best if @step
 * is a power of ten. If the resulting precision is not suitable for your
 * needs, use ctk_scale_set_digits() to correct it.
 *
 * Returns: a new #CtkVScale
 *
 * Deprecated: 3.2: Use ctk_scale_new_with_range() with %CTK_ORIENTATION_VERTICAL instead
 **/
CtkWidget *
ctk_vscale_new_with_range (gdouble min,
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

  scale = g_object_new (CTK_TYPE_VSCALE,
                        "adjustment", adj,
                        "digits", digits,
                        NULL);

  return CTK_WIDGET (scale);
}
