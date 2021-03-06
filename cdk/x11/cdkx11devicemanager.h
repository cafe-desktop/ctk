/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2011 Carlos Garnacho
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

#ifndef __CDK_X11_DEVICE_MANAGER_H__
#define __CDK_X11_DEVICE_MANAGER_H__

#if !defined (__CDKX_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkx.h> can be included directly."
#endif

#include <cdk/cdk.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

G_BEGIN_DECLS

CDK_AVAILABLE_IN_3_2
CdkDevice * cdk_x11_device_manager_lookup (CdkDeviceManager *device_manager,
                                           gint              device_id);

G_END_DECLS

#endif /* __CDK_X11_DEVICE_MANAGER_H__ */
