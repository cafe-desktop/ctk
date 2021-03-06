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

#include "cdktypes.h"
#include "cdkinternals.h"

#include <stdio.h>

guint               _cdk_debug_flags = 0;
GList              *_cdk_default_filters = NULL;
gchar              *_cdk_display_name = NULL;
gchar              *_cdk_display_arg_name = NULL;
gboolean            _cdk_disable_multidevice = FALSE;
guint               _cdk_gl_flags = 0;
CdkRenderingMode    _cdk_rendering_mode = CDK_RENDERING_MODE_SIMILAR;
