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

#ifndef __CDK_DEVICE_MANAGER_PRIVATE_CORE_H__
#define __CDK_DEVICE_MANAGER_PRIVATE_CORE_H__

#include <X11/Xlib.h>

#include "cdkx11devicemanager-core.h"
#include "cdkdevicemanagerprivate.h"

G_BEGIN_DECLS

struct _CdkX11DeviceManagerCore
{
  CdkDeviceManager parent_object;
  CdkDevice *core_pointer;
  CdkDevice *core_keyboard;
};

struct _CdkX11DeviceManagerCoreClass
{
  CdkDeviceManagerClass parent_class;
};

void            _cdk_device_manager_core_handle_focus           (CdkWindow   *window,
                                                                 Window       original,
                                                                 CdkDevice   *device,
                                                                 CdkDevice   *source_device,
                                                                 gboolean     focus_in,
                                                                 int          detail,
                                                                 gboolean     in);

G_END_DECLS

#endif /* __CDK_DEVICE_MANAGER_PRIVATE_CORE_H__ */
