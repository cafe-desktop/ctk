/*
 * cdkwin32monitor.h
 *
 * Copyright 2016 Red Hat, Inc.
 *
 * Matthias Clasen <mclasen@redhat.com>
 * Руслан Ижбулатов <lrn1986@gmail.com>
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

#ifndef __CDK_WIN32_MONITOR_H__
#define __CDK_WIN32_MONITOR_H__

#if !defined (__CDKWIN32_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkwin32.h> can be included directly."
#endif

#include <cdk/cdkmonitor.h>

G_BEGIN_DECLS

#define CDK_TYPE_WIN32_MONITOR           (cdk_win32_monitor_get_type ())
#define CDK_WIN32_MONITOR(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_WIN32_MONITOR, CdkWin32Monitor))
#define CDK_IS_WIN32_MONITOR(object)     (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_WIN32_MONITOR))

#ifdef CDK_COMPILATION
typedef struct _CdkWin32Monitor      CdkWin32Monitor;
#else
typedef CdkMonitor CdkWin32Monitor;
#endif
typedef struct _CdkWin32MonitorClass CdkWin32MonitorClass;

CDK_AVAILABLE_IN_3_22
GType             cdk_win32_monitor_get_type            (void) G_GNUC_CONST;

G_END_DECLS

#endif  /* __CDK_WIN32_MONITOR_H__ */
