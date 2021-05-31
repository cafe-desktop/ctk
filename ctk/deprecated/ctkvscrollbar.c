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

#include "ctkvscrollbar.h"

#include "ctkadjustment.h"
#include "ctkintl.h"
#include "ctkorientable.h"
#include "ctkscrollbar.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * SECTION:ctkvscrollbar
 * @Short_description: A vertical scrollbar
 * @Title: CtkVScrollbar
 * @See_also:#CtkScrollbar, #CtkScrolledWindow
 *
 * The #CtkVScrollbar widget is a widget arranged vertically creating a
 * scrollbar. See #CtkScrollbar for details on
 * scrollbars. #CtkAdjustment pointers may be added to handle the
 * adjustment of the scrollbar or it may be left %NULL in which case one
 * will be created for you. See #CtkScrollbar for a description of what the
 * fields in an adjustment represent for a scrollbar.
 *
 * CtkVScrollbar has been deprecated, use #CtkScrollbar instead.
 */

G_DEFINE_TYPE (CtkVScrollbar, ctk_vscrollbar, CTK_TYPE_SCROLLBAR)

static void
ctk_vscrollbar_class_init (CtkVScrollbarClass *class)
{
  CTK_RANGE_CLASS (class)->stepper_detail = "vscrollbar";
}

static void
ctk_vscrollbar_init (CtkVScrollbar *vscrollbar)
{
  ctk_orientable_set_orientation (CTK_ORIENTABLE (vscrollbar),
                                  CTK_ORIENTATION_VERTICAL);
}

/**
 * ctk_vscrollbar_new:
 * @adjustment: (allow-none): the #CtkAdjustment to use, or %NULL to create a new adjustment
 *
 * Creates a new vertical scrollbar.
 *
 * Returns: the new #CtkVScrollbar
 *
 * Deprecated: 3.2: Use ctk_scrollbar_new() with %CTK_ORIENTATION_VERTICAL instead
 */
CtkWidget *
ctk_vscrollbar_new (CtkAdjustment *adjustment)
{
  g_return_val_if_fail (adjustment == NULL || CTK_IS_ADJUSTMENT (adjustment),
                        NULL);

  return g_object_new (CTK_TYPE_VSCROLLBAR,
                       "adjustment", adjustment,
                       NULL);
}
