/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2013 Jan Arne Petersen
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

#ifndef __CDK_WAYLAND_DEVICE_H__
#define __CDK_WAYLAND_DEVICE_H__

#if !defined (__CDKWAYLAND_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkwayland.h> can be included directly."
#endif

#include <cdk/cdk.h>

#include <wayland-client.h>

G_BEGIN_DECLS

#ifdef CDK_COMPILATION
typedef struct _CdkWaylandDevice CdkWaylandDevice;
#else
typedef CdkDevice CdkWaylandDevice;
#endif
typedef struct _CdkWaylandDeviceClass CdkWaylandDeviceClass;

#define CDK_TYPE_WAYLAND_DEVICE         (cdk_wayland_device_get_type ())
#define CDK_WAYLAND_DEVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CDK_TYPE_WAYLAND_DEVICE, CdkWaylandDevice))
#define CDK_WAYLAND_DEVICE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), CDK_TYPE_WAYLAND_DEVICE, CdkWaylandDeviceClass))
#define CDK_IS_WAYLAND_DEVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CDK_TYPE_WAYLAND_DEVICE))
#define CDK_IS_WAYLAND_DEVICE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), CDK_TYPE_WAYLAND_DEVICE))
#define CDK_WAYLAND_DEVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CDK_TYPE_WAYLAND_DEVICE, CdkWaylandDeviceClass))

CDK_AVAILABLE_IN_ALL
GType                cdk_wayland_device_get_type            (void);

CDK_AVAILABLE_IN_ALL
struct wl_seat      *cdk_wayland_device_get_wl_seat         (CdkDevice *device);
CDK_AVAILABLE_IN_ALL
struct wl_pointer   *cdk_wayland_device_get_wl_pointer      (CdkDevice *device);
CDK_AVAILABLE_IN_ALL
struct wl_keyboard  *cdk_wayland_device_get_wl_keyboard     (CdkDevice *device);

CDK_AVAILABLE_IN_3_20
struct wl_seat      *cdk_wayland_seat_get_wl_seat           (CdkSeat   *seat);

CDK_AVAILABLE_IN_3_22
const gchar         *cdk_wayland_device_get_node_path       (CdkDevice *device);

CDK_AVAILABLE_IN_3_22
void                 cdk_wayland_device_pad_set_feedback (CdkDevice           *device,
                                                          CdkDevicePadFeature  element,
                                                          guint                idx,
                                                          const gchar         *label);

G_END_DECLS

#endif /* __CDK_WAYLAND_DEVICE_H__ */
