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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __GDK_PRIVATE_H__
#define __GDK_PRIVATE_H__

#include <cdk/cdk.h>

G_BEGIN_DECLS

const char * _cdk_atom_name_const (CdkAtom atom);

GDK_AVAILABLE_IN_ALL
void cdk_window_destroy_notify	     (CdkWindow *window);

GDK_AVAILABLE_IN_ALL
void cdk_synthesize_window_state (CdkWindow     *window,
                                  CdkWindowState unset_flags,
                                  CdkWindowState set_flags);

G_END_DECLS

#endif /* __GDK_PRIVATE_H__ */
