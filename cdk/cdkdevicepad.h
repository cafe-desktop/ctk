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

#ifndef __CDK_DEVICE_PAD_H__
#define __CDK_DEVICE_PAD_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdkversionmacros.h>
#include <cdk/cdktypes.h>

G_BEGIN_DECLS

#define CDK_TYPE_DEVICE_PAD         (cdk_device_pad_get_type ())
#define CDK_DEVICE_PAD(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CDK_TYPE_DEVICE_PAD, CdkDevicePad))
#define CDK_IS_DEVICE_PAD(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CDK_TYPE_DEVICE_PAD))

typedef struct _CdkDevicePad CdkDevicePad;
typedef struct _CdkDevicePadInterface CdkDevicePadInterface;

/**
 * CdkDevicePadFeature:
 * @CDK_DEVICE_PAD_FEATURE_BUTTON: a button
 * @CDK_DEVICE_PAD_FEATURE_RING: a ring-shaped interactive area
 * @CDK_DEVICE_PAD_FEATURE_STRIP: a straight interactive area
 *
 * A pad feature.
 */
typedef enum {
  CDK_DEVICE_PAD_FEATURE_BUTTON,
  CDK_DEVICE_PAD_FEATURE_RING,
  CDK_DEVICE_PAD_FEATURE_STRIP
} CdkDevicePadFeature;

CDK_AVAILABLE_IN_3_22
GType cdk_device_pad_get_type          (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_22
gint  cdk_device_pad_get_n_groups      (CdkDevicePad *pad);

CDK_AVAILABLE_IN_3_22
gint  cdk_device_pad_get_group_n_modes (CdkDevicePad *pad,
                                        gint          group_idx);

CDK_AVAILABLE_IN_3_22
gint  cdk_device_pad_get_n_features    (CdkDevicePad        *pad,
                                        CdkDevicePadFeature  feature);

CDK_AVAILABLE_IN_3_22
gint  cdk_device_pad_get_feature_group (CdkDevicePad        *pad,
                                        CdkDevicePadFeature  feature,
                                        gint                 feature_idx);

G_END_DECLS

#endif /* __CDK_DEVICE_PAD_H__ */
