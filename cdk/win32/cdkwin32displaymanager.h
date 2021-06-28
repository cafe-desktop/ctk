/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef __CDK_WIN32_DISPLAY_MANAGER_H__
#define __CDK_WIN32_DISPLAY_MANAGER_H__

#if !defined (__CDKWIN32_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkwin32.h> can be included directly."
#endif

#include <cdk/cdk.h>

G_BEGIN_DECLS

#ifdef CDK_COMPILATION
typedef struct _CdkWin32DisplayManager CdkWin32DisplayManager;
#else
typedef CdkDisplayManager CdkWin32DisplayManager;
#endif
typedef struct _CdkWin32DisplayManagerClass CdkWin32DisplayManagerClass;

#define CDK_TYPE_WIN32_DISPLAY_MANAGER              (cdk_win32_display_manager_get_type())
#define CDK_WIN32_DISPLAY_MANAGER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_WIN32_DISPLAY_MANAGER, CdkWin32DisplayManager))
#define CDK_WIN32_DISPLAY_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_WIN32_DISPLAY_MANAGER, CdkWin32DisplayManagerClass))
#define CDK_IS_WIN32_DISPLAY_MANAGER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_WIN32_DISPLAY_MANAGER))
#define CDK_IS_WIN32_DISPLAY_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_WIN32_DISPLAY_MANAGER))
#define CDK_WIN32_DISPLAY_MANAGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_WIN32_DISPLAY_MANAGER, CdkWin32DisplayManagerClass))

CDK_AVAILABLE_IN_ALL
GType      cdk_win32_display_manager_get_type            (void);

G_END_DECLS

#endif /* __CDK_WIN32_DISPLAY_MANAGER_H__ */
