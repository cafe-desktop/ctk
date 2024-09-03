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

#include "ctkhseparator.h"
#include "ctkorientable.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * SECTION:ctkhseparator
 * @Short_description: A horizontal separator
 * @Title: CtkHSeparator
 * @See_also: #CtkSeparator
 *
 * The #CtkHSeparator widget is a horizontal separator, used to group the
 * widgets within a window. It displays a horizontal line with a shadow to
 * make it appear sunken into the interface.
 *
 * > The #CtkHSeparator widget is not used as a separator within menus.
 * > To create a separator in a menu create an empty #CtkSeparatorMenuItem
 * > widget using ctk_separator_menu_item_new() and add it to the menu with
 * > ctk_menu_shell_append().
 *
 * CtkHSeparator has been deprecated, use #CtkSeparator instead.
 */


G_DEFINE_TYPE (CtkHSeparator, ctk_hseparator, CTK_TYPE_SEPARATOR)

static void
ctk_hseparator_class_init (CtkHSeparatorClass *class G_GNUC_UNUSED)
{
}

static void
ctk_hseparator_init (CtkHSeparator *hseparator)
{
  ctk_orientable_set_orientation (CTK_ORIENTABLE (hseparator),
                                  CTK_ORIENTATION_HORIZONTAL);
}

/**
 * ctk_hseparator_new:
 *
 * Creates a new #CtkHSeparator.
 *
 * Returns: a new #CtkHSeparator.
 *
 * Deprecated: 3.2: Use ctk_separator_new() with %CTK_ORIENTATION_HORIZONTAL instead
 */
CtkWidget *
ctk_hseparator_new (void)
{
  return g_object_new (CTK_TYPE_HSEPARATOR, NULL);
}
