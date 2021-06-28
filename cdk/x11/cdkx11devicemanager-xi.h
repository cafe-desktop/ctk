/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2009 Carlos Garnacho <carlosg@gnome.org>
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

#ifndef __CDK_X11_DEVICE_MANAGER_XI_H__
#define __CDK_X11_DEVICE_MANAGER_XI_H__

#include <cdk/cdk.h>

G_BEGIN_DECLS

#define CDK_TYPE_X11_DEVICE_MANAGER_XI         (cdk_x11_device_manager_xi_get_type ())
#define CDK_X11_DEVICE_MANAGER_XI(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CDK_TYPE_X11_DEVICE_MANAGER_XI, CdkX11DeviceManagerXI))
#define CDK_X11_DEVICE_MANAGER_XI_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), CDK_TYPE_X11_DEVICE_MANAGER_XI, CdkX11DeviceManagerXIClass))
#define CDK_IS_X11_DEVICE_MANAGER_XI(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CDK_TYPE_X11_DEVICE_MANAGER_XI))
#define CDK_IS_X11_DEVICE_MANAGER_XI_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), CDK_TYPE_X11_DEVICE_MANAGER_XI))
#define CDK_X11_DEVICE_MANAGER_XI_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CDK_TYPE_X11_DEVICE_MANAGER_XI, CdkX11DeviceManagerXIClass))

typedef struct _CdkX11DeviceManagerXI CdkX11DeviceManagerXI;
typedef struct _CdkX11DeviceManagerXIClass CdkX11DeviceManagerXIClass;


CDK_AVAILABLE_IN_ALL
GType cdk_x11_device_manager_xi_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __CDK_X11_DEVICE_MANAGER_XI_H__ */
