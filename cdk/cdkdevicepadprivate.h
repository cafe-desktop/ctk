/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2016 Red Hat
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
 *
 * Author: Carlos Garnacho <carlosg@gnome.org>
 */

#ifndef __CDK_DEVICE_PAD_PRIVATE_H__
#define __CDK_DEVICE_PAD_PRIVATE_H__

#include "cdkdevicepad.h"

G_BEGIN_DECLS

#define CDK_DEVICE_PAD_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CDK_TYPE_DEVICE_PAD, CdkDevicePadInterface))

struct _CdkDevicePadInterface {
  GTypeInterface parent_interface;

  gint (* get_n_groups)      (CdkDevicePad        *pad);

  gint (* get_group_n_modes) (CdkDevicePad        *pad,
                              gint                 group);
  gint (* get_n_features)    (CdkDevicePad        *pad,
                              CdkDevicePadFeature  feature);
  gint (* get_feature_group) (CdkDevicePad        *pad,
                              CdkDevicePadFeature  feature,
                              gint                 idx);
};

G_END_DECLS

#endif /* __CDK_DEVICE_PAD_PRIVATE_H__ */
