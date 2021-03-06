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

#ifndef __CDK_X11_DEVICE_XI2_H__
#define __CDK_X11_DEVICE_XI2_H__

#include <cdk/cdk.h>

G_BEGIN_DECLS

#define CDK_TYPE_X11_DEVICE_XI2         (cdk_x11_device_xi2_get_type ())
#define CDK_X11_DEVICE_XI2(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CDK_TYPE_X11_DEVICE_XI2, CdkX11DeviceXI2))
#define CDK_X11_DEVICE_XI2_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), CDK_TYPE_X11_DEVICE_XI2, CdkX11DeviceXI2Class))
#define CDK_IS_X11_DEVICE_XI2(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CDK_TYPE_X11_DEVICE_XI2))
#define CDK_IS_X11_DEVICE_XI2_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), CDK_TYPE_X11_DEVICE_XI2))
#define CDK_X11_DEVICE_XI2_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CDK_TYPE_X11_DEVICE_XI2, CdkX11DeviceXI2Class))

typedef struct _CdkX11DeviceXI2 CdkX11DeviceXI2;
typedef struct _CdkX11DeviceXI2Class CdkX11DeviceXI2Class;

CDK_AVAILABLE_IN_ALL
GType cdk_x11_device_xi2_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __CDK_X11_DEVICE_XI2_H__ */
