/* CDK - The GIMP Drawing Kit
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

#include "cdkproperty.h"

#include "cdkmain.h"
#include "cdkprivate.h"
#include "cdkinternals.h"
#include "cdkdisplay-broadway.h"
#include "cdkscreen-broadway.h"
#include "cdkselection.h"

#include <string.h>

gboolean
_cdk_broadway_window_get_property (CdkWindow   *window,
				   CdkAtom      property,
				   CdkAtom      type,
				   gulong       offset,
				   gulong       length,
				   gint         pdelete,
				   CdkAtom     *actual_property_type,
				   gint        *actual_format_type,
				   gint        *actual_length,
				   guchar     **data)
{
  return FALSE;
}

void
_cdk_broadway_window_change_property (CdkWindow    *window,
				      CdkAtom       property,
				      CdkAtom       type,
				      gint          format,
				      CdkPropMode   mode,
				      const guchar *data,
				      gint          nelements)
{
  g_return_if_fail (!window || CDK_WINDOW_IS_BROADWAY (window));
}

void
_cdk_broadway_window_delete_property (CdkWindow *window,
				      CdkAtom    property)
{
  g_return_if_fail (!window || CDK_WINDOW_IS_BROADWAY (window));
}
