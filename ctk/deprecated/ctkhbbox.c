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

#include "ctkhbbox.h"
#include "ctkorientable.h"
#include "ctkintl.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * SECTION:ctkhbbox
 * @Short_description: A container for arranging buttons horizontally
 * @Title: CtkHButtonBox
 * @See_also: #CtkBox, #CtkButtonBox, #CtkVButtonBox
 *
 * A button box should be used to provide a consistent layout of buttons
 * throughout your application. The layout/spacing can be altered by the
 * programmer, or if desired, by the user to alter the “feel” of a
 * program to a small degree.
 *
 * A #CtkHButtonBox is created with ctk_hbutton_box_new(). Buttons are
 * packed into a button box the same way widgets are added to any other
 * container, using ctk_container_add(). You can also use
 * ctk_box_pack_start() or ctk_box_pack_end(), but for button boxes both
 * these functions work just like ctk_container_add(), ie., they pack the
 * button in a way that depends on the current layout style and on
 * whether the button has had ctk_button_box_set_child_secondary() called
 * on it.
 *
 * The spacing between buttons can be set with ctk_box_set_spacing(). The
 * arrangement and layout of the buttons can be changed with
 * ctk_button_box_set_layout().
 *
 * CtkHButtonBox has been deprecated, use #CtkButtonBox instead.
 */


G_DEFINE_TYPE (CtkHButtonBox, ctk_hbutton_box, CTK_TYPE_BUTTON_BOX)

static void
ctk_hbutton_box_class_init (CtkHButtonBoxClass *class)
{
}

static void
ctk_hbutton_box_init (CtkHButtonBox *hbutton_box)
{
  ctk_orientable_set_orientation (CTK_ORIENTABLE (hbutton_box),
                                  CTK_ORIENTATION_HORIZONTAL);
}

/**
 * ctk_hbutton_box_new:
 *
 * Creates a new horizontal button box.
 *
 * Returns: a new button box #CtkWidget.
 *
 * Deprecated: 3.2: Use ctk_button_box_new() with %CTK_ORIENTATION_HORIZONTAL instead
 */
CtkWidget *
ctk_hbutton_box_new (void)
{
  return g_object_new (CTK_TYPE_HBUTTON_BOX, NULL);
}

