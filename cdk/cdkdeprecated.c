/* CDK - The GIMP Drawing Kit
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

#define CDK_DISABLE_DEPRECATION_WARNINGS

#include "config.h"

#include "cdkdisplay.h"
#include "cdkmain.h"
#include "cdkwindow.h"

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
