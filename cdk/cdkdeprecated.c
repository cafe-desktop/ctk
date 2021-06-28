/* GDK - The GIMP Drawing Kit
 * cdkdeprecated.c
 * 
 * Copyright 1995-2011 Red Hat Inc.
 *
 * Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#define GDK_DISABLE_DEPRECATION_WARNINGS

#include "config.h"

#include "cdkdisplay.h"
#include "cdkmain.h"
#include "cdkwindow.h"

/**
 * cdk_pointer_ungrab:
 * @time_: a timestamp from a #GdkEvent, or %GDK_CURRENT_TIME if no 
 *  timestamp is available.
 *
 * Ungrabs the pointer on the default display, if it is grabbed by this 
 * application.
 *
 * Deprecated: 3.0: Use cdk_device_ungrab(), together with cdk_device_grab()
 *             instead.
 **/
void
cdk_pointer_ungrab (guint32 time)
{
  cdk_display_pointer_ungrab (cdk_display_get_default (), time);
}

/**
 * cdk_pointer_is_grabbed:
 * 
 * Returns %TRUE if the pointer on the default display is currently 
 * grabbed by this application.
 *
 * Note that this does not take the inmplicit pointer grab on button
 * presses into account.
 *
 * Returns: %TRUE if the pointer is currently grabbed by this application.
 *
 * Deprecated: 3.0: Use cdk_display_device_is_grabbed() instead.
 **/
gboolean
cdk_pointer_is_grabbed (void)
{
  return cdk_display_pointer_is_grabbed (cdk_display_get_default ());
}

/**
 * cdk_keyboard_ungrab:
 * @time_: a timestamp from a #GdkEvent, or %GDK_CURRENT_TIME if no
 *        timestamp is available.
 * 
 * Ungrabs the keyboard on the default display, if it is grabbed by this 
 * application.
 *
 * Deprecated: 3.0: Use cdk_device_ungrab(), together with cdk_device_grab()
 *             instead.
 **/
void
cdk_keyboard_ungrab (guint32 time)
{
  cdk_display_keyboard_ungrab (cdk_display_get_default (), time);
}

/**
 * cdk_window_at_pointer:
 * @win_x: (out) (allow-none): return location for origin of the window under the pointer
 * @win_y: (out) (allow-none): return location for origin of the window under the pointer
 *
 * Obtains the window underneath the mouse pointer, returning the
 * location of that window in @win_x, @win_y. Returns %NULL if the
 * window under the mouse pointer is not known to GDK (if the window
 * belongs to another application and a #GdkWindow hasn’t been created
 * for it with cdk_window_foreign_new())
 *
 * NOTE: For multihead-aware widgets or applications use
 * cdk_display_get_window_at_pointer() instead.
 *
 * Returns: (transfer none): window under the mouse pointer
 *
 * Deprecated: 3.0: Use cdk_device_get_window_at_position() instead.
 **/
GdkWindow*
cdk_window_at_pointer (gint *win_x,
		       gint *win_y)
{
  return cdk_display_get_window_at_pointer (cdk_display_get_default (), win_x, win_y);
}

