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

#ifndef __CDK_X11_UTILS_H__
#define __CDK_X11_UTILS_H__

#if !defined (__CDKX_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkx.h> can be included directly."
#endif

#include <cdk/cdk.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

G_BEGIN_DECLS

CDK_AVAILABLE_IN_ALL
Window   cdk_x11_get_default_root_xwindow (void);
CDK_AVAILABLE_IN_ALL
Display *cdk_x11_get_default_xdisplay     (void);

/**
 * CDK_ROOT_WINDOW:
 *
 * Obtains the Xlib window id of the root window of the current screen.
 */
#define CDK_ROOT_WINDOW()             (cdk_x11_get_default_root_xwindow ())

/**
 * CDK_XID_TO_POINTER:
 * @xid: XID to stuff into the pointer
 *
 * Converts an XID into a @gpointer. This is useful with data structures
 * that use pointer arguments such as #GHashTable. Use CDK_POINTER_TO_XID()
 * to convert the argument back to an XID.
 */
#define CDK_XID_TO_POINTER(xid) GUINT_TO_POINTER(xid)

/**
 * CDK_POINTER_TO_XID:
 * @pointer: pointer to extract an XID from
 *
 * Converts a @gpointer back to an XID that was previously converted
 * using CDK_XID_TO_POINTER().
 */
#define CDK_POINTER_TO_XID(pointer) GPOINTER_TO_UINT(pointer)

CDK_AVAILABLE_IN_ALL
void          cdk_x11_grab_server    (void);
CDK_AVAILABLE_IN_ALL
void          cdk_x11_ungrab_server  (void);

CDK_DEPRECATED_IN_3_24
cairo_pattern_t *cdk_x11_get_parent_relative_pattern (void);

G_END_DECLS

#endif /* __CDK_X11_UTILS_H__ */
