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

/**
 * SECTION:cdkdevicepad
 * @Short_description: Pad device interface
 * @Title: CtkDevicePad
 *
 * #CdkDevicePad is an interface implemented by devices of type
 * %CDK_SOURCE_TABLET_PAD, it allows querying the features provided
 * by the pad device.
 *
 * Tablet pads may contain one or more groups, each containing a subset
 * of the buttons/rings/strips available. cdk_device_pad_get_n_groups()
 * can be used to obtain the number of groups, cdk_device_pad_get_n_features()
 * and cdk_device_pad_get_feature_group() can be combined to find out the
 * number of buttons/rings/strips the device has, and how are they grouped.
 *
 * Each of those groups have different modes, which may be used to map
 * each individual pad feature to multiple actions. Only one mode is
 * effective (current) for each given group, different groups may have
 * different current modes. The number of available modes in a group can
 * be found out through cdk_device_pad_get_group_n_modes(), and the current
 * mode for a given group will be notified through the #CdkEventPadGroupMode
 * event.
 *
 */

#include "config.h"

#include "cdkdevicepad.h"
#include "cdkdevicepadprivate.h"
#include "cdkdeviceprivate.h"

G_DEFINE_INTERFACE (CdkDevicePad, cdk_device_pad, CDK_TYPE_DEVICE)

static void
cdk_device_pad_default_init (CdkDevicePadInterface *pad G_GNUC_UNUSED)
{
}

/**
 * cdk_device_pad_get_n_groups:
 * @pad: a #CdkDevicePad
 *
 * Returns the number of groups this pad device has. Pads have
 * at least one group. A pad group is a subcollection of
 * buttons/strip/rings that is affected collectively by a same
 * current mode.
 *
 * Returns: The number of button/ring/strip groups in the pad.
 *
 * Since: 3.22
 **/
gint
cdk_device_pad_get_n_groups (CdkDevicePad *pad)
{
  CdkDevicePadInterface *iface = CDK_DEVICE_PAD_GET_IFACE (pad);

  g_return_val_if_fail (CDK_IS_DEVICE_PAD (pad), 0);

  return iface->get_n_groups (pad);
}

/**
 * cdk_device_pad_get_group_n_modes:
 * @pad: a #CdkDevicePad
 * @group_idx: group to get the number of available modes from
 *
 * Returns the number of modes that @group may have.
 *
 * Returns: The number of modes available in @group.
 *
 * Since: 3.22
 **/
gint
cdk_device_pad_get_group_n_modes (CdkDevicePad *pad,
                                  gint          group_idx)
{
  CdkDevicePadInterface *iface = CDK_DEVICE_PAD_GET_IFACE (pad);

  g_return_val_if_fail (CDK_IS_DEVICE_PAD (pad), 0);
  g_return_val_if_fail (group_idx >= 0, 0);

  return iface->get_group_n_modes (pad, group_idx);
}

/**
 * cdk_device_pad_get_n_features:
 * @pad: a #CdkDevicePad
 * @feature: a pad feature
 *
 * Returns the number of features a tablet pad has.
 *
 * Returns: The amount of elements of type @feature that this pad has.
 *
 * Since: 3.22
 **/
gint
cdk_device_pad_get_n_features (CdkDevicePad        *pad,
                               CdkDevicePadFeature  feature)
{
  CdkDevicePadInterface *iface = CDK_DEVICE_PAD_GET_IFACE (pad);

  g_return_val_if_fail (CDK_IS_DEVICE_PAD (pad), 0);

  return iface->get_n_features (pad, feature);
}

/**
 * cdk_device_pad_get_feature_group:
 * @pad: a #CdkDevicePad
 * @feature: the feature type to get the group from
 * @feature_idx: the index of the feature to get the group from
 *
 * Returns the group the given @feature and @idx belong to,
 * or -1 if feature/index do not exist in @pad.
 *
 * Returns: The group number of the queried pad feature.
 *
 * Since: 3.22
 **/
gint
cdk_device_pad_get_feature_group (CdkDevicePad        *pad,
                                  CdkDevicePadFeature  feature,
                                  gint                 idx)
{
  CdkDevicePadInterface *iface = CDK_DEVICE_PAD_GET_IFACE (pad);

  g_return_val_if_fail (CDK_IS_DEVICE_PAD (pad), -1);
  g_return_val_if_fail (idx >= 0, -1);

  return iface->get_feature_group (pad, feature, idx);
}
