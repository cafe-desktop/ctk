/* GDK - The GIMP Drawing Kit
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

#ifndef __GDK_WAYLAND_SELECTION_H__
#define __GDK_WAYLAND_SELECTION_H__

#if !defined (__GDKWAYLAND_H_INSIDE__) && !defined (GDK_COMPILATION)
#error "Only <cdk/cdkwayland.h> can be included directly."
#endif

#include <cdk/cdk.h>

G_BEGIN_DECLS

#if defined (CTK_COMPILATION) || defined (GDK_COMPILATION)
#define cdk_wayland_selection_add_targets cdk_wayland_selection_add_targets_libctk_only
GDK_AVAILABLE_IN_ALL
void
cdk_wayland_selection_add_targets (CdkWindow *window,
                                   CdkAtom    selection,
                                   guint      ntargets,
                                   CdkAtom   *targets);

#define cdk_wayland_selection_clear_targets cdk_wayland_selection_clear_targets_libctk_only
GDK_AVAILABLE_IN_ALL
void
cdk_wayland_selection_clear_targets (CdkDisplay *display, CdkAtom selection);

#endif

G_END_DECLS

#endif /* __GDK_WAYLAND_SELECTION_H__ */
