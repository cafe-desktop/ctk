/*
 * cdkquartzmonitor.h
 *
 * Copyright 2017 Tom Schoonjans
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

#ifndef __CDK_QUARTZ_MONITOR_H__
#define __CDK_QUARTZ_MONITOR_H__

#if !defined (__CDKQUARTZ_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkquartz.h> can be included directly."
#endif

#include <cdk/cdkmonitor.h>

G_BEGIN_DECLS

#define CDK_TYPE_QUARTZ_MONITOR           (cdk_quartz_monitor_get_type ())
#define CDK_QUARTZ_MONITOR(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_QUARTZ_MONITOR, CdkQuartzMonitor))
#define CDK_IS_QUARTZ_MONITOR(object)     (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_QUARTZ_MONITOR))

typedef struct _CdkQuartzMonitor      CdkQuartzMonitor;
typedef struct _CdkQuartzMonitorClass CdkQuartzMonitorClass;

CDK_AVAILABLE_IN_3_22
GType             cdk_quartz_monitor_get_type            (void) G_GNUC_CONST;


G_END_DECLS

#endif  /* __CDK_QUARTZ_MONITOR_H__ */

