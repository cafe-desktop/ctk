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

#ifndef __GDK_PIXBUF_H__
#define __GDK_PIXBUF_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cairo.h>
#include <cdk-pixbuf/cdk-pixbuf.h>
#include <cdk/cdktypes.h>
#include <cdk/cdkversionmacros.h>

G_BEGIN_DECLS

CDK_AVAILABLE_IN_ALL
GdkPixbuf *gdk_pixbuf_get_from_window  (CdkWindow       *window,
                                        gint             src_x,
                                        gint             src_y,
                                        gint             width,
                                        gint             height);

CDK_AVAILABLE_IN_ALL
GdkPixbuf *gdk_pixbuf_get_from_surface (cairo_surface_t *surface,
                                        gint             src_x,
                                        gint             src_y,
                                        gint             width,
                                        gint             height);

G_END_DECLS

#endif /* __GDK_PIXBUF_H__ */
