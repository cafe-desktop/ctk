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

#ifndef __CDK_DEVICE_VIRTUAL_H__
#define __CDK_DEVICE_VIRTUAL_H__

#include <cdk/cdkdeviceprivate.h>

G_BEGIN_DECLS

#define CDK_TYPE_DEVICE_VIRTUAL         (cdk_device_virtual_get_type ())
#define CDK_DEVICE_VIRTUAL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CDK_TYPE_DEVICE_VIRTUAL, CdkDeviceVirtual))
#define CDK_DEVICE_VIRTUAL_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), CDK_TYPE_DEVICE_VIRTUAL, CdkDeviceVirtualClass))
#define CDK_IS_DEVICE_VIRTUAL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CDK_TYPE_DEVICE_VIRTUAL))
#define CDK_IS_DEVICE_VIRTUAL_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), CDK_TYPE_DEVICE_VIRTUAL))
#define CDK_DEVICE_VIRTUAL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CDK_TYPE_DEVICE_VIRTUAL, CdkDeviceVirtualClass))

typedef struct _CdkDeviceVirtual CdkDeviceVirtual;
typedef struct _CdkDeviceVirtualClass CdkDeviceVirtualClass;

struct _CdkDeviceVirtual
{
  CdkDevice parent_instance;
  CdkDevice *active_device;
};

struct _CdkDeviceVirtualClass
{
  CdkDeviceClass parent_class;
};

GType cdk_device_virtual_get_type (void) G_GNUC_CONST;

void _cdk_device_virtual_set_active (CdkDevice *device,
				     CdkDevice *new_active);


G_END_DECLS

#endif /* __CDK_DEVICE_VIRTUAL_H__ */
