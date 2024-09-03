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

#include "ctkorientable.h"

#include "ctkvseparator.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * SECTION:ctkvseparator
 * @Short_description: A vertical separator
 * @Title: CtkVSeparator
 * @See_also: #CtkHSeparator
 *
 * The #CtkVSeparator widget is a vertical separator, used to group the
 * widgets within a window. It displays a vertical line with a shadow to
 * make it appear sunken into the interface.
 *
 * CtkVSeparator has been deprecated, use #CtkSeparator instead.
 */

G_DEFINE_TYPE (CtkVSeparator, ctk_vseparator, CTK_TYPE_SEPARATOR)

static void
ctk_vseparator_class_init (CtkVSeparatorClass *klass G_GNUC_UNUSED)
{
}

static void
ctk_vseparator_init (CtkVSeparator *vseparator)
{
  ctk_orientable_set_orientation (CTK_ORIENTABLE (vseparator),
                                  CTK_ORIENTATION_VERTICAL);
}

/**
 * ctk_vseparator_new:
 *
 * Creates a new #CtkVSeparator.
 *
 * Returns: a new #CtkVSeparator.
 *
 * Deprecated: 3.2: Use ctk_separator_new() with %CTK_ORIENTATION_VERTICAL instead
 */
CtkWidget *
ctk_vseparator_new (void)
{
  return g_object_new (CTK_TYPE_VSEPARATOR, NULL);
}
