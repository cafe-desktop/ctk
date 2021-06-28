/*
 * cdkmonitor.h
 *
 * Copyright 2016 Red Hat, Inc.
 *
 * Matthias Clasen <mclasen@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CDK_MONITOR_H__
#define __CDK_MONITOR_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdkversionmacros.h>
#include <cdk/cdkrectangle.h>
#include <cdk/cdktypes.h>

G_BEGIN_DECLS

#define CDK_TYPE_MONITOR           (cdk_monitor_get_type ())
#define CDK_MONITOR(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_MONITOR, CdkMonitor))
#define CDK_IS_MONITOR(object)     (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_MONITOR))

typedef struct _CdkMonitor      CdkMonitor;
typedef struct _CdkMonitorClass CdkMonitorClass;

/**
 * CdkSubpixelLayout:
 * @CDK_SUBPIXEL_LAYOUT_UNKNOWN: The layout is not known
 * @CDK_SUBPIXEL_LAYOUT_NONE: Not organized in this way
 * @CDK_SUBPIXEL_LAYOUT_HORIZONTAL_RGB: The layout is horizontal, the order is RGB
 * @CDK_SUBPIXEL_LAYOUT_HORIZONTAL_BGR: The layout is horizontal, the order is BGR
 * @CDK_SUBPIXEL_LAYOUT_VERTICAL_RGB: The layout is vertical, the order is RGB
 * @CDK_SUBPIXEL_LAYOUT_VERTICAL_BGR: The layout is vertical, the order is BGR
 *
 * This enumeration describes how the red, green and blue components
 * of physical pixels on an output device are laid out.
 *
 * Since: 3.22
 */
typedef enum {
  CDK_SUBPIXEL_LAYOUT_UNKNOWN,
  CDK_SUBPIXEL_LAYOUT_NONE,
  CDK_SUBPIXEL_LAYOUT_HORIZONTAL_RGB,
  CDK_SUBPIXEL_LAYOUT_HORIZONTAL_BGR,
  CDK_SUBPIXEL_LAYOUT_VERTICAL_RGB,
  CDK_SUBPIXEL_LAYOUT_VERTICAL_BGR
} CdkSubpixelLayout;

CDK_AVAILABLE_IN_3_22
GType             cdk_monitor_get_type            (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_22
CdkDisplay  *     cdk_monitor_get_display         (CdkMonitor   *monitor);
CDK_AVAILABLE_IN_3_22
void              cdk_monitor_get_geometry        (CdkMonitor   *monitor,
                                                   CdkRectangle *geometry);
CDK_AVAILABLE_IN_3_22
void              cdk_monitor_get_workarea        (CdkMonitor   *monitor,
                                                   CdkRectangle *workarea);
CDK_AVAILABLE_IN_3_22
int               cdk_monitor_get_width_mm        (CdkMonitor   *monitor);
CDK_AVAILABLE_IN_3_22
int               cdk_monitor_get_height_mm       (CdkMonitor   *monitor);
CDK_AVAILABLE_IN_3_22
const char *      cdk_monitor_get_manufacturer    (CdkMonitor   *monitor);
CDK_AVAILABLE_IN_3_22
const char *      cdk_monitor_get_model           (CdkMonitor   *monitor);
CDK_AVAILABLE_IN_3_22
int               cdk_monitor_get_scale_factor    (CdkMonitor   *monitor);
CDK_AVAILABLE_IN_3_22
int               cdk_monitor_get_refresh_rate    (CdkMonitor   *monitor);
CDK_AVAILABLE_IN_3_22
CdkSubpixelLayout cdk_monitor_get_subpixel_layout (CdkMonitor   *monitor);
CDK_AVAILABLE_IN_3_22
gboolean          cdk_monitor_is_primary          (CdkMonitor   *monitor);

G_END_DECLS

#endif  /* __CDK_MONITOR_H__ */
