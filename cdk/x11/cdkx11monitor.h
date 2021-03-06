/*
 * cdkx11monitor.h
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

#ifndef __CDK_X11_MONITOR_H__
#define __CDK_X11_MONITOR_H__

#if !defined (__CDKX_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkx.h> can be included directly."
#endif

#include <cdk/cdkmonitor.h>

G_BEGIN_DECLS

#define CDK_TYPE_X11_MONITOR           (cdk_x11_monitor_get_type ())
#define CDK_X11_MONITOR(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_X11_MONITOR, CdkX11Monitor))
#define CDK_IS_X11_MONITOR(object)     (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_X11_MONITOR))

typedef struct _CdkX11Monitor      CdkX11Monitor;
typedef struct _CdkX11MonitorClass CdkX11MonitorClass;

CDK_AVAILABLE_IN_3_22
GType             cdk_x11_monitor_get_type            (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_22
XID               cdk_x11_monitor_get_output          (CdkMonitor *monitor);

G_END_DECLS

#endif  /* __CDK_X11_MONITOR_H__ */
