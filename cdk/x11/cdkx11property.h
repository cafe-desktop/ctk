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

#ifndef __CDK_X11_PROPERTY_H__
#define __CDK_X11_PROPERTY_H__

#if !defined (__CDKX_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkx.h> can be included directly."
#endif

#include <cdk/cdk.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

G_BEGIN_DECLS

/* Functions to get the X Atom equivalent to the CdkAtom */
CDK_AVAILABLE_IN_ALL
Atom                  cdk_x11_atom_to_xatom_for_display (CdkDisplay  *display,
                                                         CdkAtom      atom);
CDK_AVAILABLE_IN_ALL
CdkAtom               cdk_x11_xatom_to_atom_for_display (CdkDisplay  *display,
                                                         Atom         xatom);
CDK_AVAILABLE_IN_ALL
Atom                  cdk_x11_get_xatom_by_name_for_display (CdkDisplay  *display,
                                                             const gchar *atom_name);
CDK_AVAILABLE_IN_ALL
const gchar *         cdk_x11_get_xatom_name_for_display (CdkDisplay  *display,
                                                          Atom         xatom);
CDK_AVAILABLE_IN_ALL
Atom                  cdk_x11_atom_to_xatom     (CdkAtom      atom);
CDK_AVAILABLE_IN_ALL
CdkAtom               cdk_x11_xatom_to_atom     (Atom         xatom);
CDK_AVAILABLE_IN_ALL
Atom                  cdk_x11_get_xatom_by_name (const gchar *atom_name);
CDK_AVAILABLE_IN_ALL
const gchar *         cdk_x11_get_xatom_name    (Atom         xatom);

G_END_DECLS

#endif /* __CDK_X11_PROPERTY_H__ */
