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

#ifndef __GDK_WIN32_SCREEN_H__
#define __GDK_WIN32_SCREEN_H__

#if !defined (__GDKWIN32_H_INSIDE__) && !defined (GDK_COMPILATION)
#error "Only <cdk/cdkwin32.h> can be included directly."
#endif

#include <cdk/cdk.h>

G_BEGIN_DECLS

#define GDK_TYPE_WIN32_SCREEN              (cdk_win32_screen_get_type ())
#define GDK_WIN32_SCREEN(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_WIN32_SCREEN, CdkWin32Screen))
#define GDK_WIN32_SCREEN_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_WIN32_SCREEN, CdkWin32ScreenClass))
#define GDK_IS_WIN32_SCREEN(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_WIN32_SCREEN))
#define GDK_IS_WIN32_SCREEN_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_WIN32_SCREEN))
#define GDK_WIN32_SCREEN_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_WIN32_SCREEN, CdkWin32ScreenClass))

#ifdef GDK_COMPILATION
typedef struct _CdkWin32Screen CdkWin32Screen;
#else
typedef CdkScreen CdkWin32Screen;
#endif
typedef struct _CdkWin32ScreenClass CdkWin32ScreenClass;

GDK_AVAILABLE_IN_ALL
GType    cdk_win32_screen_get_type          (void);

G_END_DECLS

#endif /* __GDK_WIN32_SCREEN_H__ */
