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

#include "ctkorientable.h"

#include "ctkvpaned.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * SECTION:ctkvpaned
 * @Short_description: A container with two panes arranged vertically
 * @Title: CtkVPaned
 *
 * The VPaned widget is a container widget with two
 * children arranged vertically. The division between
 * the two panes is adjustable by the user by dragging
 * a handle. See #CtkPaned for details.
 *
 * CtkVPaned has been deprecated, use #CtkPaned instead.
 */

G_DEFINE_TYPE (CtkVPaned, ctk_vpaned, CTK_TYPE_PANED)

static void
ctk_vpaned_class_init (CtkVPanedClass *class)
{
}

static void
ctk_vpaned_init (CtkVPaned *vpaned)
{
  ctk_orientable_set_orientation (CTK_ORIENTABLE (vpaned),
                                  CTK_ORIENTATION_VERTICAL);
}

/**
 * ctk_vpaned_new:
 *
 * Create a new #CtkVPaned
 *
 * Returns: the new #CtkVPaned
 *
 * Deprecated: 3.2: Use ctk_paned_new() with %CTK_ORIENTATION_VERTICAL instead
 */
CtkWidget *
ctk_vpaned_new (void)
{
  return g_object_new (CTK_TYPE_VPANED, NULL);
}
