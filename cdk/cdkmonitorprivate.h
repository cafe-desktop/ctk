/*
 * cdkmonitorprivate.h
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

#ifndef __CDK_MONITOR_PRIVATE_H__
#define __CDK_MONITOR_PRIVATE_H__

#include "cdkmonitor.h"

G_BEGIN_DECLS

#define CDK_MONITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_MONITOR, CdkMonitorClass))
#define CDK_IS_MONITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_MONITOR))
#define CDK_MONITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_MONITOR, CdkMonitorClass))

struct _CdkMonitor {
  GObject parent;

  CdkDisplay *display;
  char *manufacturer;
  char *model;
  char *connector;
  CdkRectangle geometry;
  int width_mm;
  int height_mm;
  int scale_factor;
  int refresh_rate;
  CdkSubpixelLayout subpixel_layout;
};

struct _CdkMonitorClass {
  GObjectClass parent_class;

  void (* get_workarea) (CdkMonitor   *monitor,
                         CdkRectangle *geometry);
};

CdkMonitor *    cdk_monitor_new                 (CdkDisplay *display);

void            cdk_monitor_set_manufacturer    (CdkMonitor *monitor,
                                                 const char *manufacturer);
void            cdk_monitor_set_model           (CdkMonitor *monitor,
                                                 const char *model);
void            cdk_monitor_set_connector       (CdkMonitor *monitor,
                                                 const char *connector);
const char *    cdk_monitor_get_connector       (CdkMonitor *monitor);
void            cdk_monitor_set_position        (CdkMonitor *monitor,
                                                 int         x,
                                                 int         y);
void            cdk_monitor_set_size            (CdkMonitor *monitor,
                                                 int         width,
                                                 int         height);
void            cdk_monitor_set_physical_size   (CdkMonitor *monitor,
                                                 int         width_mm,
                                                 int         height_mm);
void            cdk_monitor_set_scale_factor    (CdkMonitor *monitor,
                                                 int         scale);
void            cdk_monitor_set_refresh_rate    (CdkMonitor *monitor,
                                                 int         refresh_rate);
void            cdk_monitor_set_subpixel_layout (CdkMonitor        *monitor,
                                                 CdkSubpixelLayout  subpixel);
void            cdk_monitor_invalidate          (CdkMonitor *monitor);

G_END_DECLS

#endif  /* __CDK_MONITOR_PRIVATE_H__ */
