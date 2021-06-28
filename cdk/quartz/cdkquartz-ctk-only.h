/* cdkquartz-ctk-only.h
 *
 * Copyright (C) 2005-2007 Imendio AB
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

#ifndef __GDK_QUARTZ_CTK_ONLY_H__
#define __GDK_QUARTZ_CTK_ONLY_H__

#if !(defined (CTK_COMPILATION) || defined (GDK_COMPILATION))
#error "This API is for use only in Ctk internal code."
#endif

#include <AppKit/AppKit.h>
#include <cdk/cdk.h>
#include <cdk/quartz/cdkquartz.h>

/* Drag and Drop/Clipboard */
GDK_AVAILABLE_IN_ALL
CdkAtom   cdk_quartz_pasteboard_type_to_atom_libctk_only        (NSString       *type);
GDK_AVAILABLE_IN_ALL
NSString *cdk_quartz_target_to_pasteboard_type_libctk_only      (const gchar    *target);
GDK_AVAILABLE_IN_ALL
NSString *cdk_quartz_atom_to_pasteboard_type_libctk_only        (CdkAtom         atom);

/* Utilities */
GDK_AVAILABLE_IN_ALL
NSImage  *cdk_quartz_pixbuf_to_ns_image_libctk_only (CdkPixbuf *pixbuf);
GDK_AVAILABLE_IN_ALL
NSEvent  *cdk_quartz_event_get_nsevent              (CdkEvent  *event);

/* Window */
GDK_AVAILABLE_IN_ALL
NSWindow *cdk_quartz_window_get_nswindow            (CdkWindow *window);
GDK_AVAILABLE_IN_ALL
NSView   *cdk_quartz_window_get_nsview              (CdkWindow *window);

#endif
