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
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include "config.h"

#include "gtkorientable.h"

#include "gtkvseparator.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * SECTION:gtkvseparator
 * @Short_description: A vertical separator
 * @Title: GtkVSeparator
 * @See_also: #GtkHSeparator
 *
 * The #GtkVSeparator widget is a vertical separator, used to group the
 * widgets within a window. It displays a vertical line with a shadow to
 * make it appear sunken into the interface.
 *
 * GtkVSeparator has been deprecated, use #GtkSeparator instead.
 */

G_DEFINE_TYPE (GtkVSeparator, ctk_vseparator, CTK_TYPE_SEPARATOR)

static void
ctk_vseparator_class_init (GtkVSeparatorClass *klass)
{
}

static void
ctk_vseparator_init (GtkVSeparator *vseparator)
{
  ctk_orientable_set_orientation (CTK_ORIENTABLE (vseparator),
                                  CTK_ORIENTATION_VERTICAL);
}

/**
 * ctk_vseparator_new:
 *
 * Creates a new #GtkVSeparator.
 *
 * Returns: a new #GtkVSeparator.
 *
 * Deprecated: 3.2: Use ctk_separator_new() with %CTK_ORIENTATION_VERTICAL instead
 */
GtkWidget *
ctk_vseparator_new (void)
{
  return g_object_new (CTK_TYPE_VSEPARATOR, NULL);
}
