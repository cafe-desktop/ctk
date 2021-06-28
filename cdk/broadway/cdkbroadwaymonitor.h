/*
 * cdkbroadwaymonitor.h
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

#ifndef __CDK_BROADWAY_MONITOR_H__
#define __CDK_BROADWAY_MONITOR_H__

#if !defined (__CDKBROADWAY_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkbroadway.h> can be included directly."
#endif

#include <cdk/cdkmonitor.h>

G_BEGIN_DECLS

#define CDK_TYPE_BROADWAY_MONITOR           (cdk_broadway_monitor_get_type ())
#define CDK_BROADWAY_MONITOR(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_BROADWAY_MONITOR, CdkBroadwayMonitor))
#define CDK_IS_BROADWAY_MONITOR(object)     (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_BROADWAY_MONITOR))

typedef struct _CdkBroadwayMonitor      CdkBroadwayMonitor;
typedef struct _CdkBroadwayMonitorClass CdkBroadwayMonitorClass;

CDK_AVAILABLE_IN_3_22
GType             cdk_broadway_monitor_get_type            (void) G_GNUC_CONST;

G_END_DECLS

#endif  /* __CDK_BROADWAY_MONITOR_H__ */

