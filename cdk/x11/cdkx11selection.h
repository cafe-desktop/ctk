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

#ifndef __CDK_X11_SELECTION_H__
#define __CDK_X11_SELECTION_H__

#if !defined (__CDKX_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkx.h> can be included directly."
#endif

#include <cdk/cdk.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

G_BEGIN_DECLS

CDK_AVAILABLE_IN_ALL
gint     cdk_x11_display_text_property_to_text_list (CdkDisplay   *display,
                                                     CdkAtom       encoding,
                                                     gint          format,
                                                     const guchar *text,
                                                     gint          length,
                                                     gchar      ***list);
CDK_AVAILABLE_IN_ALL
void     cdk_x11_free_text_list                     (gchar       **list);
CDK_AVAILABLE_IN_ALL
gint     cdk_x11_display_string_to_compound_text    (CdkDisplay   *display,
                                                     const gchar  *str,
                                                     CdkAtom      *encoding,
                                                     gint         *format,
                                                     guchar      **ctext,
                                                     gint         *length);
CDK_AVAILABLE_IN_ALL
gboolean cdk_x11_display_utf8_to_compound_text      (CdkDisplay   *display,
                                                     const gchar  *str,
                                                     CdkAtom      *encoding,
                                                     gint         *format,
                                                     guchar      **ctext,
                                                     gint         *length);
CDK_AVAILABLE_IN_ALL
void     cdk_x11_free_compound_text                 (guchar       *ctext);

G_END_DECLS

#endif /* __CDK_X11_SELECTION_H__ */
