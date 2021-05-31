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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include "ctkhscrollbar.h"

#include "ctkadjustment.h"
#include "ctkintl.h"
#include "ctkorientable.h"
#include "ctkscrollbar.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * SECTION:ctkhscrollbar
 * @Short_description: A horizontal scrollbar
 * @Title: CtkHScrollbar
 * @See_also: #CtkScrollbar, #CtkScrolledWindow
 *
 * The #CtkHScrollbar widget is a widget arranged horizontally creating a
 * scrollbar. See #CtkScrollbar for details on
 * scrollbars. #CtkAdjustment pointers may be added to handle the
 * adjustment of the scrollbar or it may be left %NULL in which case one
 * will be created for you. See #CtkScrollbar for a description of what the
 * fields in an adjustment represent for a scrollbar.
 *
 * CtkHScrollbar has been deprecated, use #CtkScrollbar instead.
 */


G_DEFINE_TYPE (CtkHScrollbar, ctk_hscrollbar, CTK_TYPE_SCROLLBAR)

static void
ctk_hscrollbar_class_init (CtkHScrollbarClass *class)
{
  CTK_RANGE_CLASS (class)->stepper_detail = "hscrollbar";
}

static void
ctk_hscrollbar_init (CtkHScrollbar *hscrollbar)
{
  ctk_orientable_set_orientation (CTK_ORIENTABLE (hscrollbar),
                                  CTK_ORIENTATION_HORIZONTAL);
}

/**
 * ctk_hscrollbar_new:
 * @adjustment: (allow-none): the #CtkAdjustment to use, or %NULL to create a new adjustment
 *
 * Creates a new horizontal scrollbar.
 *
 * Returns: the new #CtkHScrollbar
 *
 * Deprecated: 3.2: Use ctk_scrollbar_new() with %CTK_ORIENTATION_HORIZONTAL instead
 */
CtkWidget *
ctk_hscrollbar_new (CtkAdjustment *adjustment)
{
  g_return_val_if_fail (adjustment == NULL || CTK_IS_ADJUSTMENT (adjustment),
                        NULL);

  return g_object_new (CTK_TYPE_HSCROLLBAR,
                       "adjustment", adjustment,
                       NULL);
}
