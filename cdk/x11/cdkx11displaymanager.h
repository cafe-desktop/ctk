/* GDK - The GIMP Drawing Kit
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

#ifndef __GDK_X11_DISPLAY_MANAGER_H__
#define __GDK_X11_DISPLAY_MANAGER_H__

#if !defined (__GDKX_H_INSIDE__) && !defined (GDK_COMPILATION)
#error "Only <cdk/cdkx.h> can be included directly."
#endif

#include <cdk/cdk.h>

G_BEGIN_DECLS

#ifdef GDK_COMPILATION
typedef struct _CdkX11DisplayManager CdkX11DisplayManager;
#else
typedef CdkDisplayManager CdkX11DisplayManager;
#endif
typedef struct _CdkX11DisplayManagerClass CdkX11DisplayManagerClass;

#define GDK_TYPE_X11_DISPLAY_MANAGER              (cdk_x11_display_manager_get_type())
#define GDK_X11_DISPLAY_MANAGER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_X11_DISPLAY_MANAGER, CdkX11DisplayManager))
#define GDK_X11_DISPLAY_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_X11_DISPLAY_MANAGER, CdkX11DisplayManagerClass))
#define GDK_IS_X11_DISPLAY_MANAGER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_X11_DISPLAY_MANAGER))
#define GDK_IS_X11_DISPLAY_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_X11_DISPLAY_MANAGER))
#define GDK_X11_DISPLAY_MANAGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_X11_DISPLAY_MANAGER, CdkX11DisplayManagerClass))

GDK_AVAILABLE_IN_ALL
GType      cdk_x11_display_manager_get_type            (void);

G_END_DECLS

#endif /* __GDK_X11_DISPLAY_MANAGER_H__ */
