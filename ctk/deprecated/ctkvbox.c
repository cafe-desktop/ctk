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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include "ctkboxprivate.h"
#include "ctkorientable.h"

#include "ctkvbox.h"
#include "ctkboxprivate.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * SECTION:ctkvbox
 * @Short_description: A vertical container box
 * @Title: CtkVBox
 * @See_also: #CtkHBox
 *
 * A #CtkVBox is a container that organizes child widgets into a single column.
 *
 * Use the #CtkBox packing interface to determine the arrangement,
 * spacing, height, and alignment of #CtkVBox children.
 *
 * All children are allocated the same width.
 *
 * CtkVBox has been deprecated. You can use #CtkBox with a #CtkOrientable:orientation
 * set to %CTK_ORIENTATION_VERTICAL instead when calling ctk_box_new(),
 * which is a very quick and easy change.
 *
 * If you have derived your own classes from CtkVBox, you can change the
 * inheritance to derive directly from #CtkBox, and set the #CtkOrientable:orientation
 * property to %CTK_ORIENTATION_VERTICAL in your instance init function,
 * with a call like:
 *
 * |[<!-- language="C" -->
 *   ctk_orientable_set_orientation (CTK_ORIENTABLE (object),
 *                                   CTK_ORIENTATION_VERTICAL);
 * ]|
 *
 * If you have a grid-like layout composed of nested boxes, and you don’t
 * need first-child or last-child styling, the recommendation is to switch
 * to #CtkGrid. For more information about migrating to #CtkGrid, see
 * [Migrating from other containers to CtkGrid][ctk-migrating-CtkGrid].
 */

G_DEFINE_TYPE (CtkVBox, ctk_vbox, CTK_TYPE_BOX)

static void
ctk_vbox_class_init (CtkVBoxClass *class G_GNUC_UNUSED)
{
}

static void
ctk_vbox_init (CtkVBox *vbox)
{
  ctk_orientable_set_orientation (CTK_ORIENTABLE (vbox),
                                  CTK_ORIENTATION_VERTICAL);

  _ctk_box_set_old_defaults (CTK_BOX (vbox));
}

/**
 * ctk_vbox_new:
 * @homogeneous: %TRUE if all children are to be given equal space allotments.
 * @spacing: the number of pixels to place by default between children.
 *
 * Creates a new #CtkVBox.
 *
 * Returns: a new #CtkVBox.
 *
 * Deprecated: 3.2: You can use ctk_box_new() with %CTK_ORIENTATION_VERTICAL instead,
 *   which is a quick and easy change. But the recommendation is to switch to
 *   #CtkGrid, since #CtkBox is going to go away eventually.
 *   See [Migrating from other containers to CtkGrid][ctk-migrating-CtkGrid].
 */
CtkWidget *
ctk_vbox_new (gboolean homogeneous,
	      gint     spacing)
{
  return g_object_new (CTK_TYPE_VBOX,
                       "spacing",     spacing,
                       "homogeneous", homogeneous ? TRUE : FALSE,
                       NULL);
}
