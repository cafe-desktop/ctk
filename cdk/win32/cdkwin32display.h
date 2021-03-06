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

#ifndef __CDK_WIN32_DISPLAY_H__
#define __CDK_WIN32_DISPLAY_H__

#if !defined (__CDKWIN32_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkwin32.h> can be included directly."
#endif

#include <cdk/cdk.h>

G_BEGIN_DECLS

#ifdef CDK_COMPILATION
typedef struct _CdkWin32Display CdkWin32Display;
#else
typedef CdkDisplay CdkWin32Display;
#endif
typedef struct _CdkWin32DisplayClass CdkWin32DisplayClass;

#define CDK_TYPE_WIN32_DISPLAY              (cdk_win32_display_get_type())
#define CDK_WIN32_DISPLAY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_WIN32_DISPLAY, CdkWin32Display))
#define CDK_WIN32_DISPLAY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_WIN32_DISPLAY, CdkWin32DisplayClass))
#define CDK_IS_WIN32_DISPLAY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_WIN32_DISPLAY))
#define CDK_IS_WIN32_DISPLAY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_WIN32_DISPLAY))
#define CDK_WIN32_DISPLAY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_WIN32_DISPLAY, CdkWin32DisplayClass))

CDK_AVAILABLE_IN_ALL
GType      cdk_win32_display_get_type            (void);

CDK_AVAILABLE_IN_3_18
void       cdk_win32_display_set_cursor_theme    (CdkDisplay  *display,
                                                  const gchar *name,
                                                  gint         size);

G_END_DECLS

#endif /* __CDK_WIN32_DISPLAY_H__ */
