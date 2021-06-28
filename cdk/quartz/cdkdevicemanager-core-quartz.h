/* cdkdevicemanager-quartz.h
 *
 * Copyright (C) 2009 Carlos Garnacho <carlosg@gnome.org>
 * Copyright (C) 2010  Kristian Rietveld  <kris@ctk.org>
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

#ifndef __GDK_QUARTZ_DEVICE_MANAGER_CORE__
#define __GDK_QUARTZ_DEVICE_MANAGER_CORE__

#include <cdkdevicemanagerprivate.h>
#include "cdkquartzdevicemanager-core.h"

#import <Cocoa/Cocoa.h>

G_BEGIN_DECLS

struct _CdkQuartzDeviceManagerCore
{
  CdkDeviceManager parent_object;
  CdkDevice *core_pointer;
  CdkDevice *core_keyboard;
  GList *known_tablet_devices;
  guint num_active_devices;
};

struct _CdkQuartzDeviceManagerCoreClass
{
  CdkDeviceManagerClass parent_class;
};

void       _cdk_quartz_device_manager_register_device_for_ns_event (CdkDeviceManager *device_manager,
                                                                    NSEvent          *nsevent);

CdkDevice *_cdk_quartz_device_manager_core_device_for_ns_event (CdkDeviceManager *device_manager,
                                                                NSEvent          *ns_event);

G_END_DECLS

#endif /* __GDK_QUARTZ_DEVICE_MANAGER__ */
