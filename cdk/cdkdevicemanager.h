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

#ifndef __CDK_DEVICE_MANAGER_H__
#define __CDK_DEVICE_MANAGER_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdktypes.h>
#include <cdk/cdkdevice.h>

G_BEGIN_DECLS

#define CDK_TYPE_DEVICE_MANAGER         (cdk_device_manager_get_type ())
#define CDK_DEVICE_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CDK_TYPE_DEVICE_MANAGER, CdkDeviceManager))
#define CDK_IS_DEVICE_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CDK_TYPE_DEVICE_MANAGER))


CDK_AVAILABLE_IN_ALL
GType        cdk_device_manager_get_type           (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CdkDisplay * cdk_device_manager_get_display        (CdkDeviceManager *device_manager);
CDK_DEPRECATED_IN_3_20
GList *      cdk_device_manager_list_devices       (CdkDeviceManager *device_manager,
                                                    CdkDeviceType     type);
CDK_DEPRECATED_IN_3_20
CdkDevice *  cdk_device_manager_get_client_pointer (CdkDeviceManager *device_manager);

G_END_DECLS

#endif /* __CDK_DEVICE_MANAGER_H__ */
